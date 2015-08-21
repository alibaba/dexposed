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

#ifndef DALVIK_INDIRECTREFTABLE_H_
#define DALVIK_INDIRECTREFTABLE_H_

/*
 * Maintain a table of indirect references.  Used for local/global JNI
 * references.
 *
 * The table contains object references that are part of the GC root set.
 * When an object is added we return an IndirectRef that is not a valid
 * pointer but can be used to find the original value in O(1) time.
 * Conversions to and from indirect refs are performed on JNI method calls
 * in and out of the VM, so they need to be very fast.
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
 * None of the table functions are synchronized.
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
 *
 * For now, we use a serial number.
 */
typedef void* IndirectRef;

/* magic failure value; must not pass dvmIsHeapAddress() */
#define kInvalidIndirectRefObject reinterpret_cast<Object*>(0xdead4321)

#define kClearedJniWeakGlobal reinterpret_cast<Object*>(0xdead1234)

/*
 * Indirect reference kind, used as the two low bits of IndirectRef.
 *
 * For convenience these match up with enum jobjectRefType from jni.h.
 */
enum IndirectRefKind {
    kIndirectKindInvalid    = 0,
    kIndirectKindLocal      = 1,
    kIndirectKindGlobal     = 2,
    kIndirectKindWeakGlobal = 3
};
const char* indirectRefKindToString(IndirectRefKind kind);

/*
 * Determine what kind of indirect reference this is.
 */
INLINE IndirectRefKind indirectRefKind(IndirectRef iref)
{
    return (IndirectRefKind)((u4) iref & 0x03);
}

/*
 * Information we store for each slot in the reference table.
 */
struct IndirectRefSlot {
    Object* obj;        /* object pointer itself, NULL if the slot is unused */
    u4      serial;     /* slot serial number */
};

/* use as initial value for "cookie", and when table has only one segment */
#define IRT_FIRST_SEGMENT   0

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
 * the bottom is passed in as a function argument (the VM keeps it in a
 * slot in the interpreted stack frame).  When we call a native method or
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
 * We need to minimize method call/return overhead.  If we store the
 * "cookie" externally, on the interpreted call stack, the VM can handle
 * pushes and pops with a single 4-byte load and store.  (We could also
 * store it internally in a public structure, but the local JNI refs are
 * logically tied to interpreted stack frames anyway.)
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
 * TODO: if we can guarantee that the underlying storage doesn't move,
 * e.g. by using oversized mmap regions to handle expanding tables, we may
 * be able to avoid having to synchronize lookups.  Might make sense to
 * add a "synchronized lookup" call that takes the mutex as an argument,
 * and either locks or doesn't lock based on internal details.
 */
union IRTSegmentState {
    u4          all;
    struct {
        u4      topIndex:16;            /* index of first unused entry */
        u4      numHoles:16;            /* #of holes in entire table */
    } parts;
};

class iref_iterator {
public:
    explicit iref_iterator(IndirectRefSlot* table, size_t i, size_t capacity) :
            table_(table), i_(i), capacity_(capacity) {
        skipNullsAndTombstones();
    }

    iref_iterator& operator++() {
        ++i_;
        skipNullsAndTombstones();
        return *this;
    }

    Object** operator*() {
        return &table_[i_].obj;
    }

    bool equals(const iref_iterator& rhs) const {
        return (i_ == rhs.i_ && table_ == rhs.table_);
    }

private:
    void skipNullsAndTombstones() {
        // We skip NULLs and tombstones. Clients don't want to see implementation details.
        while (i_ < capacity_ && (table_[i_].obj == NULL
                || table_[i_].obj == kClearedJniWeakGlobal)) {
            ++i_;
        }
    }

    IndirectRefSlot* table_;
    size_t i_;
    size_t capacity_;
};

bool inline operator!=(const iref_iterator& lhs, const iref_iterator& rhs) {
    return !lhs.equals(rhs);
}

struct IndirectRefTable {
public:
    typedef iref_iterator iterator;

    /* semi-public - read/write by interpreter in native call handler */
    IRTSegmentState segmentState;

    /*
     * private:
     *
     * TODO: we can't make these private as long as the interpreter
     * uses offsetof, since private member data makes us non-POD.
     */
    /* bottom of the stack */
    IndirectRefSlot* table_;
    /* bit mask, ORed into all irefs */
    IndirectRefKind kind_;
    /* #of entries we have space for */
    size_t          alloc_entries_;
    /* max #of entries allowed */
    size_t          max_entries_;

    // TODO: want hole-filling stats (#of holes filled, total entries scanned)
    //       for performance evaluation.

    /*
     * Add a new entry.  "obj" must be a valid non-NULL object reference
     * (though it's okay if it's not fully-formed, e.g. the result from
     * dvmMalloc doesn't have obj->clazz set).
     *
     * Returns NULL if the table is full (max entries reached, or alloc
     * failed during expansion).
     */
    IndirectRef add(u4 cookie, Object* obj);

    /*
     * Given an IndirectRef in the table, return the Object it refers to.
     *
     * Returns kInvalidIndirectRefObject if iref is invalid.
     */
    Object* get(IndirectRef iref) const;

    /*
     * Returns true if the table contains a reference to this object.
     */
    bool contains(const Object* obj) const;

    /*
     * Remove an existing entry.
     *
     * If the entry is not between the current top index and the bottom index
     * specified by the cookie, we don't remove anything.  This is the behavior
     * required by JNI's DeleteLocalRef function.
     *
     * Returns "false" if nothing was removed.
     */
    bool remove(u4 cookie, IndirectRef iref);

    /*
     * Initialize an IndirectRefTable.
     *
     * If "initialCount" != "maxCount", the table will expand as required.
     *
     * "kind" should be Local or Global.  The Global table may also hold
     * WeakGlobal refs.
     *
     * Returns "false" if table allocation fails.
     */
    bool init(size_t initialCount, size_t maxCount, IndirectRefKind kind);

    /*
     * Clear out the contents, freeing allocated storage.
     *
     * You must call dvmInitReferenceTable() before you can re-use this table.
     *
     * TODO: this should be a destructor.
     */
    void destroy();

    /*
     * Dump the contents of a reference table to the log file.
     *
     * The caller should lock any external sync before calling.
     *
     * TODO: we should name the table in a constructor and remove
     * the argument here.
     */
    void dump(const char* descr) const;

    /*
     * Return the #of entries in the entire table.  This includes holes, and
     * so may be larger than the actual number of "live" entries.
     */
    size_t capacity() const {
        return segmentState.parts.topIndex;
    }

    iterator begin() {
        return iterator(table_, 0, capacity());
    }

    iterator end() {
        return iterator(table_, capacity(), capacity());
    }

private:
    static inline u4 extractIndex(IndirectRef iref) {
        u4 uref = (u4) iref;
        return (uref >> 2) & 0xffff;
    }

    static inline u4 extractSerial(IndirectRef iref) {
        u4 uref = (u4) iref;
        return uref >> 20;
    }

    static inline u4 nextSerial(u4 serial) {
        return (serial + 1) & 0xfff;
    }

    static inline IndirectRef toIndirectRef(u4 index, u4 serial, IndirectRefKind kind) {
        assert(index < 65536);
        return reinterpret_cast<IndirectRef>((serial << 20) | (index << 2) | kind);
    }
};

#endif  // DALVIK_INDIRECTREFTABLE_H_
