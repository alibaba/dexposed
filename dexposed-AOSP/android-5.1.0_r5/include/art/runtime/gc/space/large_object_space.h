/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ART_RUNTIME_GC_SPACE_LARGE_OBJECT_SPACE_H_
#define ART_RUNTIME_GC_SPACE_LARGE_OBJECT_SPACE_H_

#include "base/allocator.h"
#include "dlmalloc_space.h"
#include "safe_map.h"
#include "space.h"

#include <set>
#include <vector>

namespace art {
namespace gc {
namespace space {

class AllocationInfo;

// Abstraction implemented by all large object spaces.
class LargeObjectSpace : public DiscontinuousSpace, public AllocSpace {
 public:
  SpaceType GetType() const OVERRIDE {
    return kSpaceTypeLargeObjectSpace;
  }
  void SwapBitmaps();
  void CopyLiveToMarked();
  virtual void Walk(DlMallocSpace::WalkCallback, void* arg) = 0;
  virtual ~LargeObjectSpace() {}

  uint64_t GetBytesAllocated() OVERRIDE {
    return num_bytes_allocated_;
  }
  uint64_t GetObjectsAllocated() OVERRIDE {
    return num_objects_allocated_;
  }
  uint64_t GetTotalBytesAllocated() const {
    return total_bytes_allocated_;
  }
  uint64_t GetTotalObjectsAllocated() const {
    return total_objects_allocated_;
  }
  size_t FreeList(Thread* self, size_t num_ptrs, mirror::Object** ptrs) OVERRIDE;
  // LargeObjectSpaces don't have thread local state.
  void RevokeThreadLocalBuffers(art::Thread*) OVERRIDE {
  }
  void RevokeAllThreadLocalBuffers() OVERRIDE {
  }
  bool IsAllocSpace() const OVERRIDE {
    return true;
  }
  AllocSpace* AsAllocSpace() OVERRIDE {
    return this;
  }
  collector::ObjectBytePair Sweep(bool swap_bitmaps);
  virtual bool CanMoveObjects() const OVERRIDE {
    return false;
  }
  // Current address at which the space begins, which may vary as the space is filled.
  byte* Begin() const {
    return begin_;
  }
  // Current address at which the space ends, which may vary as the space is filled.
  byte* End() const {
    return end_;
  }
  // Current size of space
  size_t Size() const {
    return End() - Begin();
  }
  // Return true if we contain the specified address.
  bool Contains(const mirror::Object* obj) const {
    const byte* byte_obj = reinterpret_cast<const byte*>(obj);
    return Begin() <= byte_obj && byte_obj < End();
  }
  void LogFragmentationAllocFailure(std::ostream& os, size_t failed_alloc_bytes) OVERRIDE
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

 protected:
  explicit LargeObjectSpace(const std::string& name, byte* begin, byte* end);
  static void SweepCallback(size_t num_ptrs, mirror::Object** ptrs, void* arg);

  // Approximate number of bytes which have been allocated into the space.
  uint64_t num_bytes_allocated_;
  uint64_t num_objects_allocated_;
  uint64_t total_bytes_allocated_;
  uint64_t total_objects_allocated_;
  // Begin and end, may change as more large objects are allocated.
  byte* begin_;
  byte* end_;

  friend class Space;

 private:
  DISALLOW_COPY_AND_ASSIGN(LargeObjectSpace);
};

// A discontinuous large object space implemented by individual mmap/munmap calls.
class LargeObjectMapSpace : public LargeObjectSpace {
 public:
  // Creates a large object space. Allocations into the large object space use memory maps instead
  // of malloc.
  static LargeObjectMapSpace* Create(const std::string& name);
  // Return the storage space required by obj.
  size_t AllocationSize(mirror::Object* obj, size_t* usable_size);
  mirror::Object* Alloc(Thread* self, size_t num_bytes, size_t* bytes_allocated,
                        size_t* usable_size);
  size_t Free(Thread* self, mirror::Object* ptr);
  void Walk(DlMallocSpace::WalkCallback, void* arg) OVERRIDE LOCKS_EXCLUDED(lock_);
  // TODO: disabling thread safety analysis as this may be called when we already hold lock_.
  bool Contains(const mirror::Object* obj) const NO_THREAD_SAFETY_ANALYSIS;

 protected:
  explicit LargeObjectMapSpace(const std::string& name);
  virtual ~LargeObjectMapSpace() {}

  // Used to ensure mutual exclusion when the allocation spaces data structures are being modified.
  mutable Mutex lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;
  std::vector<mirror::Object*, TrackingAllocator<mirror::Object*, kAllocatorTagLOS>> large_objects_
      GUARDED_BY(lock_);
  typedef SafeMap<mirror::Object*, MemMap*, std::less<mirror::Object*>,
      TrackingAllocator<std::pair<mirror::Object*, MemMap*>, kAllocatorTagLOSMaps>> MemMaps;
  MemMaps mem_maps_ GUARDED_BY(lock_);
};

// A continuous large object space with a free-list to handle holes.
class FreeListSpace FINAL : public LargeObjectSpace {
 public:
  static constexpr size_t kAlignment = kPageSize;

  virtual ~FreeListSpace();
  static FreeListSpace* Create(const std::string& name, byte* requested_begin, size_t capacity);
  size_t AllocationSize(mirror::Object* obj, size_t* usable_size) OVERRIDE
      EXCLUSIVE_LOCKS_REQUIRED(lock_);
  mirror::Object* Alloc(Thread* self, size_t num_bytes, size_t* bytes_allocated,
                        size_t* usable_size) OVERRIDE;
  size_t Free(Thread* self, mirror::Object* obj) OVERRIDE;
  void Walk(DlMallocSpace::WalkCallback callback, void* arg) OVERRIDE LOCKS_EXCLUDED(lock_);
  void Dump(std::ostream& os) const;

 protected:
  FreeListSpace(const std::string& name, MemMap* mem_map, byte* begin, byte* end);
  size_t GetSlotIndexForAddress(uintptr_t address) const {
    DCHECK(Contains(reinterpret_cast<mirror::Object*>(address)));
    return (address - reinterpret_cast<uintptr_t>(Begin())) / kAlignment;
  }
  size_t GetSlotIndexForAllocationInfo(const AllocationInfo* info) const;
  AllocationInfo* GetAllocationInfoForAddress(uintptr_t address);
  const AllocationInfo* GetAllocationInfoForAddress(uintptr_t address) const;
  uintptr_t GetAllocationAddressForSlot(size_t slot) const {
    return reinterpret_cast<uintptr_t>(Begin()) + slot * kAlignment;
  }
  uintptr_t GetAddressForAllocationInfo(const AllocationInfo* info) const {
    return GetAllocationAddressForSlot(GetSlotIndexForAllocationInfo(info));
  }
  // Removes header from the free blocks set by finding the corresponding iterator and erasing it.
  void RemoveFreePrev(AllocationInfo* info) EXCLUSIVE_LOCKS_REQUIRED(lock_);

  class SortByPrevFree {
   public:
    bool operator()(const AllocationInfo* a, const AllocationInfo* b) const;
  };
  typedef std::set<AllocationInfo*, SortByPrevFree,
                   TrackingAllocator<AllocationInfo*, kAllocatorTagLOSFreeList>> FreeBlocks;

  // There is not footer for any allocations at the end of the space, so we keep track of how much
  // free space there is at the end manually.
  std::unique_ptr<MemMap> mem_map_;
  // Side table for allocation info, one per page.
  std::unique_ptr<MemMap> allocation_info_map_;
  AllocationInfo* allocation_info_;

  Mutex lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;
  // Free bytes at the end of the space.
  size_t free_end_ GUARDED_BY(lock_);
  FreeBlocks free_blocks_ GUARDED_BY(lock_);
};

}  // namespace space
}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_SPACE_LARGE_OBJECT_SPACE_H_
