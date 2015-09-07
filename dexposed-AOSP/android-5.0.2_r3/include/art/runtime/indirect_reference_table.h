/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_RUNTIME_INDIRECT_REFERENCE_TABLE_H_
#define ART_RUNTIME_INDIRECT_REFERENCE_TABLE_H_

#include <stdint.h>

#include <iosfwd>
#include <string>

#include "base/logging.h"
#include "base/mutex.h"
#include "gc_root.h"
#include "mem_map.h"
#include "object_callbacks.h"
#include "offsets.h"
#include "read_barrier_option.h"

namespace art {
namespace mirror {
class Object;
}  // namespace mirror

/*
 * Maintain a table of indirect references.  Used for local/global JNI
 * references.
 *
 * The table contains object references that are part of the GC root set.
 * When an object is added we return an IndirectRef that is not a valid
 * pointer but can be used to find the original value in O(1) time.
 * Conversions to and from indirect references are performed on upcalls
 * and downcalls, so they need to be very fast.
 *
 * To be efficient for JNI local variable storage, we need to provide
 * operations that allow us to operate on segments of the table, where
 * segments are pushed and popped as if on a stack.  For example, deletion
 * of an entry should only succeed if it appears in the current segment,
 * and we want to be able to strip off the current segment quickly when
 * a method returns.  Additions to the table must be made in the current
 * segment even if space is available in an earlier area.
 *
 * A new segment is created when we call into native code from interpreted
 * code, or when we handle the JNI PushLocalFrame function.
 *
 * The GC must be able to scan the entire table quickly.
 *
 * In summary, these must be very fast:
 *  - adding or removing a segment
 *  - adding references to a new segment
 *  - converting an indirect reference back to an Object
 * These can be a little slower, but must still be pretty quick:
 *  - adding references to a "mature" segment
 *  - removing individual references
 *  - scanning the entire table straight through
 *
 * If there's more than one segment, we don't guarantee that the table
 * will fill completely before we fail due to lack of space.  We do ensure
 * that the current segment will pack tightly, which should satisfy JNI
 * requirements (e.g. EnsureLocalCapacity).
 *
 * To make everything fit nicely in 32-bit integers, the maximum size of
 * the table is capped at 64K.
 *
 * Only SynchronizedGet is synchronized.
 */

/*
 * Indirect reference definition.  This must be interchangeable with JNI's
 * jobject, and it's convenient to let null be null, so we use void*.
 *
 * We need a 16-bit table index and a 2-bit reference type (global, local,
 * weak global).  Real object pointers will have zeroes in the low 2 or 3
 * bits (4- or 8-byte alignment), so it's useful to put the ref type
 * in the low bits and reserve zero as an invalid value.
 *
 * The remaining 14 bits can be used to detect stale indirect references.
 * For example, if objects don't move, we can use a hash of the original
 * Object* to make sure the entry hasn't been re-used.  (If the Object*
 * we find there doesn't match because of heap movement, we could do a
 * secondary check on the preserved hash value; this implies that creating
 * a global/local ref queries the hash value and forces it to be saved.)
 *
 * A more rigorous approach would be to put a serial number in the extra
 * bits, and keep a copy of the serial number in a parallel table.  This is
 * easier when objects can move, but requires 2x the memory and additional
 * memory accesses on add/get.  It will catch additional problems, e.g.:
 * create iref1 for obj, delete iref1, create iref2 for same obj, lookup
 * iref1.  A pattern based on object bits will miss this.
 */
typedef void* IndirectRef;

// Magic failure values; must not pass Heap::ValidateObject() or Heap::IsHeapAddress().
static mirror::Object* const kInvalidIndirectRefObject = reinterpret_cast<mirror::Object*>(0xdead4321);
static mirror::Object* const kClearedJniWeakGlobal = reinterpret_cast<mirror::Object*>(0xdead1234);

/*
 * Indirect reference kind, used as the two low bits of IndirectRef.
 *
 * For convenience these match up with enum jobjectRefType from jni.h.
 */
enum IndirectRefKind {
  kHandleScopeOrInvalid = 0,  // <<stack indirect reference table or invalid reference>>
  kLocal         = 1,  // <<local reference>>
  kGlobal        = 2,  // <<global reference>>
  kWeakGlobal    = 3   // <<weak global reference>>
};
std::ostream& operator<<(std::ostream& os, const IndirectRefKind& rhs);

/*
 * Determine what kind of indirect reference this is.
 */
static inline IndirectRefKind GetIndirectRefKind(IndirectRef iref) {
  return static_cast<IndirectRefKind>(reinterpret_cast<uintptr_t>(iref) & 0x03);
}

/*
 * Extended debugging structure.  We keep a parallel array of these, one
 * per slot in the table.
 */
static const size_t kIRTPrevCount = 4;
struct IndirectRefSlot {
  uint32_t serial;
  const mirror::Object* previous[kIRTPrevCount];
};

/* use as initial value for "cookie", and when table has only one segment */
static const uint32_t IRT_FIRST_SEGMENT = 0;

/*
 * Table definition.
 *
 * For the global reference table, the expected common operations are
 * adding a new entry and removing a recently-added entry (usually the
 * most-recently-added entry).  For JNI local references, the common
 * operations are adding a new entry and removing an entire table segment.
 *
 * If "alloc_entries_" is not equal to "max_entries_", the table may expand
 * when entries are added, which means the memory may move.  If you want
 * to keep pointers into "table" rather than offsets, you must use a
 * fixed-size table.
 *
 * If we delete entries from the middle of the list, we will be left with
 * "holes".  We track the number of holes so that, when adding new elements,
 * we can quickly decide to do a trivial append or go slot-hunting.
 *
 * When the top-most entry is removed, any holes immediately below it are
 * also removed.  Thus, deletion of an entry may reduce "topIndex" by more
 * than one.
 *
 * To get the desired behavior for JNI locals, we need to know the bottom
 * and top of the current "segment".  The top is managed internally, and
 * the bottom is passed in as a function argument.  When we call a native method or
 * push a local frame, the current top index gets pushed on, and serves
 * as the new bottom.  When we pop a frame off, the value from the stack
 * becomes the new top index, and the value stored in the previous frame
 * becomes the new bottom.
 *
 * To avoid having to re-scan the table after a pop, we want to push the
 * number of holes in the table onto the stack.  Because of our 64K-entry
 * cap, we can combine the two into a single unsigned 32-bit value.
 * Instead of a "bottom" argument we take a "cookie", which includes the
 * bottom index and the count of holes below the bottom.
 *
 * Common alternative implementation: make IndirectRef a pointer to the
 * actual reference slot.  Instead of getting a table and doing a lookup,
 * the lookup can be done instantly.  Operations like determining the
 * type and deleting the reference are more expensive because the table
 * must be hunted for (i.e. you have to do a pointer comparison to see
 * which table it's in), you can't move the table when expanding it (so
 * realloc() is out), and tricks like serial number checking to detect
 * stale references aren't possible (though we may be able to get similar
 * benefits with other approaches).
 *
 * TODO: consider a "lastDeleteIndex" for quick hole-filling when an
 * add immediately follows a delete; must invalidate after segment pop
 * (which could increase the cost/complexity of method call/return).
 * Might be worth only using it for JNI globals.
 *
 * TODO: may want completely different add/remove algorithms for global
 * and local refs to improve performance.  A large circular buffer might
 * reduce the amortized cost of adding global references.
 *
 */
union IRTSegmentState {
  uint32_t          all;
  struct {
    uint32_t      topIndex:16;            /* index of first unused entry */
    uint32_t      numHoles:16;            /* #of holes in entire table */
  } parts;
};

class IrtIterator {
 public:
  explicit IrtIterator(GcRoot<mirror::Object>* table, size_t i, size_t capacity)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      : table_(table), i_(i), capacity_(capacity) {
    SkipNullsAndTombstones();
  }

  IrtIterator& operator++() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    ++i_;
    SkipNullsAndTombstones();
    return *this;
  }

  mirror::Object** operator*() {
    // This does not have a read barrier as this is used to visit roots.
    return table_[i_].AddressWithoutBarrier();
  }

  bool equals(const IrtIterator& rhs) const {
    return (i_ == rhs.i_ && table_ == rhs.table_);
  }

 private:
  void SkipNullsAndTombstones() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    // We skip NULLs and tombstones. Clients don't want to see implementation details.
    while (i_ < capacity_ &&
           (table_[i_].IsNull() ||
            table_[i_].Read<kWithoutReadBarrier>() == kClearedJniWeakGlobal)) {
      ++i_;
    }
  }

  GcRoot<mirror::Object>* const table_;
  size_t i_;
  size_t capacity_;
};

bool inline operator==(const IrtIterator& lhs, const IrtIterator& rhs) {
  return lhs.equals(rhs);
}

bool inline operator!=(const IrtIterator& lhs, const IrtIterator& rhs) {
  return !lhs.equals(rhs);
}

class IndirectReferenceTable {
 public:
  IndirectReferenceTable(size_t initialCount, size_t maxCount, IndirectRefKind kind);

  ~IndirectReferenceTable();

  /*
   * Add a new entry.  "obj" must be a valid non-NULL object reference.
   *
   * Returns NULL if the table is full (max entries reached, or alloc
   * failed during expansion).
   */
  IndirectRef Add(uint32_t cookie, mirror::Object* obj)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  /*
   * Given an IndirectRef in the table, return the Object it refers to.
   *
   * Returns kInvalidIndirectRefObject if iref is invalid.
   */
  template<ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  mirror::Object* Get(IndirectRef iref) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      ALWAYS_INLINE;

  // Synchronized get which reads a reference, acquiring a lock if necessary.
  template<ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  mirror::Object* SynchronizedGet(Thread* /*self*/, ReaderWriterMutex* /*mutex*/,
                                  IndirectRef iref) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return Get<kReadBarrierOption>(iref);
  }

  /*
   * Remove an existing entry.
   *
   * If the entry is not between the current top index and the bottom index
   * specified by the cookie, we don't remove anything.  This is the behavior
   * required by JNI's DeleteLocalRef function.
   *
   * Returns "false" if nothing was removed.
   */
  bool Remove(uint32_t cookie, IndirectRef iref);

  void AssertEmpty();

  void Dump(std::ostream& os) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  /*
   * Return the #of entries in the entire table.  This includes holes, and
   * so may be larger than the actual number of "live" entries.
   */
  size_t Capacity() const {
    return segment_state_.parts.topIndex;
  }

  // Note IrtIterator does not have a read barrier as it's used to visit roots.
  IrtIterator begin() {
    return IrtIterator(table_, 0, Capacity());
  }

  IrtIterator end() {
    return IrtIterator(table_, Capacity(), Capacity());
  }

  void VisitRoots(RootCallback* callback, void* arg, uint32_t tid, RootType root_type)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  uint32_t GetSegmentState() const {
    return segment_state_.all;
  }

  void SetSegmentState(uint32_t new_state) {
    segment_state_.all = new_state;
  }

  static Offset SegmentStateOffset() {
    return Offset(OFFSETOF_MEMBER(IndirectReferenceTable, segment_state_));
  }

 private:
  /*
   * Extract the table index from an indirect reference.
   */
  static uint32_t ExtractIndex(IndirectRef iref) {
    uintptr_t uref = reinterpret_cast<uintptr_t>(iref);
    return (uref >> 2) & 0xffff;
  }

  /*
   * The object pointer itself is subject to relocation in some GC
   * implementations, so we shouldn't really be using it here.
   */
  IndirectRef ToIndirectRef(uint32_t tableIndex) const {
    DCHECK_LT(tableIndex, 65536U);
    uint32_t serialChunk = slot_data_[tableIndex].serial;
    uintptr_t uref = serialChunk << 20 | (tableIndex << 2) | kind_;
    return reinterpret_cast<IndirectRef>(uref);
  }

  /*
   * Update extended debug info when an entry is added.
   *
   * We advance the serial number, invalidating any outstanding references to
   * this slot.
   */
  void UpdateSlotAdd(const mirror::Object* obj, int slot) {
    if (slot_data_ != NULL) {
      IndirectRefSlot* pSlot = &slot_data_[slot];
      pSlot->serial++;
      pSlot->previous[pSlot->serial % kIRTPrevCount] = obj;
    }
  }

  // Abort if check_jni is not enabled.
  static void AbortIfNoCheckJNI();

  /* extra debugging checks */
  bool GetChecked(IndirectRef) const;
  bool CheckEntry(const char*, IndirectRef, int) const;

  /* semi-public - read/write by jni down calls */
  IRTSegmentState segment_state_;

  // Mem map where we store the indirect refs.
  std::unique_ptr<MemMap> table_mem_map_;
  // Mem map where we store the extended debugging info.
  std::unique_ptr<MemMap> slot_mem_map_;
  // bottom of the stack. Do not directly access the object references
  // in this as they are roots. Use Get() that has a read barrier.
  GcRoot<mirror::Object>* table_;
  /* bit mask, ORed into all irefs */
  IndirectRefKind kind_;
  /* extended debugging info */
  IndirectRefSlot* slot_data_;
  /* #of entries we have space for */
  size_t alloc_entries_;
  /* max #of entries allowed */
  size_t max_entries_;
};

}  // namespace art

#endif  // ART_RUNTIME_INDIRECT_REFERENCE_TABLE_H_
