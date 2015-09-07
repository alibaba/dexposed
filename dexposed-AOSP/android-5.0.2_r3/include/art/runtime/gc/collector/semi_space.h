/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef ART_RUNTIME_GC_COLLECTOR_SEMI_SPACE_H_
#define ART_RUNTIME_GC_COLLECTOR_SEMI_SPACE_H_

#include <memory>

#include "atomic.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "garbage_collector.h"
#include "gc/accounting/heap_bitmap.h"
#include "immune_region.h"
#include "mirror/object_reference.h"
#include "object_callbacks.h"
#include "offsets.h"

namespace art {

class Thread;

namespace mirror {
  class Class;
  class Object;
}  // namespace mirror

namespace gc {

class Heap;

namespace accounting {
  template <typename T> class AtomicStack;
  typedef AtomicStack<mirror::Object*> ObjectStack;
}  // namespace accounting

namespace space {
  class ContinuousMemMapAllocSpace;
  class ContinuousSpace;
}  // namespace space

namespace collector {

class SemiSpace : public GarbageCollector {
 public:
  // If true, use remembered sets in the generational mode.
  static constexpr bool kUseRememberedSet = true;

  explicit SemiSpace(Heap* heap, bool generational = false, const std::string& name_prefix = "");

  ~SemiSpace() {}

  virtual void RunPhases() OVERRIDE NO_THREAD_SAFETY_ANALYSIS;
  virtual void InitializePhase();
  virtual void MarkingPhase() EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_)
      LOCKS_EXCLUDED(Locks::heap_bitmap_lock_);
  virtual void ReclaimPhase() EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_)
      LOCKS_EXCLUDED(Locks::heap_bitmap_lock_);
  virtual void FinishPhase() EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);
  void MarkReachableObjects()
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_, Locks::heap_bitmap_lock_);
  virtual GcType GetGcType() const OVERRIDE {
    return kGcTypePartial;
  }
  virtual CollectorType GetCollectorType() const OVERRIDE {
    return generational_ ? kCollectorTypeGSS : kCollectorTypeSS;
  }

  // Sets which space we will be copying objects to.
  void SetToSpace(space::ContinuousMemMapAllocSpace* to_space);

  // Set the space where we copy objects from.
  void SetFromSpace(space::ContinuousMemMapAllocSpace* from_space);

  // Set whether or not we swap the semi spaces in the heap. This needs to be done with mutators
  // suspended.
  void SetSwapSemiSpaces(bool swap_semi_spaces) {
    swap_semi_spaces_ = swap_semi_spaces;
  }

  // Initializes internal structures.
  void Init();

  // Find the default mark bitmap.
  void FindDefaultMarkBitmap();

  // Returns the new address of the object.
  template<bool kPoisonReferences>
  void MarkObject(mirror::ObjectReference<kPoisonReferences, mirror::Object>* obj_ptr)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

  void ScanObject(mirror::Object* obj)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

  void VerifyNoFromSpaceReferences(mirror::Object* obj)
      SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

  // Marks the root set at the start of a garbage collection.
  void MarkRoots()
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

  // Bind the live bits to the mark bits of bitmaps for spaces that are never collected, ie
  // the image. Mark that portion of the heap as immune.
  virtual void BindBitmaps() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      LOCKS_EXCLUDED(Locks::heap_bitmap_lock_);

  void UnBindBitmaps()
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_);

  void ProcessReferences(Thread* self) EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Sweeps unmarked objects to complete the garbage collection.
  virtual void Sweep(bool swap_bitmaps) EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_);

  // Sweeps unmarked objects to complete the garbage collection.
  void SweepLargeObjects(bool swap_bitmaps) EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_);

  void SweepSystemWeaks()
      SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

  static void MarkRootCallback(mirror::Object** root, void* arg, uint32_t /*tid*/,
                               RootType /*root_type*/)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

  static mirror::Object* MarkObjectCallback(mirror::Object* root, void* arg)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

  static void MarkHeapReferenceCallback(mirror::HeapReference<mirror::Object>* obj_ptr, void* arg)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

  static void ProcessMarkStackCallback(void* arg)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_, Locks::heap_bitmap_lock_);

  static void DelayReferenceReferentCallback(mirror::Class* klass, mirror::Reference* ref,
                                             void* arg)
      SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

  virtual mirror::Object* MarkNonForwardedObject(mirror::Object* obj)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

  // Schedules an unmarked object for reference processing.
  void DelayReferenceReferent(mirror::Class* klass, mirror::Reference* reference)
      SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

 protected:
  // Returns null if the object is not marked, otherwise returns the forwarding address (same as
  // object for non movable things).
  mirror::Object* GetMarkedForwardAddress(mirror::Object* object) const
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_)
      SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_);

  static bool HeapReferenceMarkedCallback(mirror::HeapReference<mirror::Object>* object, void* arg)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_)
      SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_);

  static mirror::Object* MarkedForwardingAddressCallback(mirror::Object* object, void* arg)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_)
      SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_);

  // Marks or unmarks a large object based on whether or not set is true. If set is true, then we
  // mark, otherwise we unmark.
  bool MarkLargeObject(const mirror::Object* obj)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Expand mark stack to 2x its current size.
  void ResizeMarkStack(size_t new_size);

  // Returns true if we should sweep the space.
  virtual bool ShouldSweepSpace(space::ContinuousSpace* space) const;

  // Push an object onto the mark stack.
  void MarkStackPush(mirror::Object* obj);

  void UpdateAndMarkModUnion()
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Recursively blackens objects on the mark stack.
  void ProcessMarkStack()
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_, Locks::heap_bitmap_lock_);

  inline mirror::Object* GetForwardingAddressInFromSpace(mirror::Object* obj) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Revoke all the thread-local buffers.
  void RevokeAllThreadLocalBuffers();

  // Current space, we check this space first to avoid searching for the appropriate space for an
  // object.
  accounting::ObjectStack* mark_stack_;

  // Immune region, every object inside the immune region is assumed to be marked.
  ImmuneRegion immune_region_;

  // If true, the large object space is immune.
  bool is_large_object_space_immune_;

  // Destination and source spaces (can be any type of ContinuousMemMapAllocSpace which either has
  // a live bitmap or doesn't).
  space::ContinuousMemMapAllocSpace* to_space_;
  // Cached live bitmap as an optimization.
  accounting::ContinuousSpaceBitmap* to_space_live_bitmap_;
  space::ContinuousMemMapAllocSpace* from_space_;
  // Cached mark bitmap as an optimization.
  accounting::HeapBitmap* mark_bitmap_;

  Thread* self_;

  // When true, the generational mode (promotion and the bump pointer
  // space only collection) is enabled. TODO: move these to a new file
  // as a new garbage collector?
  const bool generational_;

  // Used for the generational mode. the end/top of the bump
  // pointer space at the end of the last collection.
  byte* last_gc_to_space_end_;

  // Used for the generational mode. During a collection, keeps track
  // of how many bytes of objects have been copied so far from the
  // bump pointer space to the non-moving space.
  uint64_t bytes_promoted_;

  // Used for the generational mode. Keeps track of how many bytes of
  // objects have been copied so far from the bump pointer space to
  // the non-moving space, since the last whole heap collection.
  uint64_t bytes_promoted_since_last_whole_heap_collection_;

  // Used for the generational mode. Keeps track of how many bytes of
  // large objects were allocated at the last whole heap collection.
  uint64_t large_object_bytes_allocated_at_last_whole_heap_collection_;

  // Used for generational mode. When true, we only collect the from_space_.
  bool collect_from_space_only_;

  // The space which we are promoting into, only used for GSS.
  space::ContinuousMemMapAllocSpace* promo_dest_space_;

  // The space which we copy to if the to_space_ is full.
  space::ContinuousMemMapAllocSpace* fallback_space_;

  // How many objects and bytes we moved, used so that we don't need to Get the size of the
  // to_space_ when calculating how many objects and bytes we freed.
  size_t bytes_moved_;
  size_t objects_moved_;

  // How many bytes we avoided dirtying.
  size_t saved_bytes_;

  // The name of the collector.
  std::string collector_name_;

  // Used for the generational mode. The default interval of the whole
  // heap collection. If N, the whole heap collection occurs every N
  // collections.
  static constexpr int kDefaultWholeHeapCollectionInterval = 5;

  // Whether or not we swap the semi spaces in the heap during the marking phase.
  bool swap_semi_spaces_;

 private:
  friend class BitmapSetSlowPathVisitor;
  DISALLOW_COPY_AND_ASSIGN(SemiSpace);
};

}  // namespace collector
}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_COLLECTOR_SEMI_SPACE_H_
