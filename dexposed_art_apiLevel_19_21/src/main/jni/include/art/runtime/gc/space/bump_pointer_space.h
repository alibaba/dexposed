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

#ifndef ART_RUNTIME_GC_SPACE_BUMP_POINTER_SPACE_H_
#define ART_RUNTIME_GC_SPACE_BUMP_POINTER_SPACE_H_

#include "object_callbacks.h"
#include "space.h"

namespace art {
namespace gc {

namespace collector {
  class MarkSweep;
}  // namespace collector

namespace space {

// A bump pointer space allocates by incrementing a pointer, it doesn't provide a free
// implementation as its intended to be evacuated.
class BumpPointerSpace FINAL : public ContinuousMemMapAllocSpace {
 public:
  typedef void(*WalkCallback)(void *start, void *end, size_t num_bytes, void* callback_arg);

  SpaceType GetType() const OVERRIDE {
    return kSpaceTypeBumpPointerSpace;
  }

  // Create a bump pointer space with the requested sizes. The requested base address is not
  // guaranteed to be granted, if it is required, the caller should call Begin on the returned
  // space to confirm the request was granted.
  static BumpPointerSpace* Create(const std::string& name, size_t capacity, byte* requested_begin);
  static BumpPointerSpace* CreateFromMemMap(const std::string& name, MemMap* mem_map);

  // Allocate num_bytes, returns nullptr if the space is full.
  mirror::Object* Alloc(Thread* self, size_t num_bytes, size_t* bytes_allocated,
                        size_t* usable_size) OVERRIDE;
  // Thread-unsafe allocation for when mutators are suspended, used by the semispace collector.
  mirror::Object* AllocThreadUnsafe(Thread* self, size_t num_bytes, size_t* bytes_allocated,
                                    size_t* usable_size)
      OVERRIDE EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::Object* AllocNonvirtual(size_t num_bytes);
  mirror::Object* AllocNonvirtualWithoutAccounting(size_t num_bytes);

  // Return the storage space required by obj.
  size_t AllocationSize(mirror::Object* obj, size_t* usable_size) OVERRIDE
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return AllocationSizeNonvirtual(obj, usable_size);
  }

  // NOPS unless we support free lists.
  size_t Free(Thread*, mirror::Object*) OVERRIDE {
    return 0;
  }

  size_t FreeList(Thread*, size_t, mirror::Object**) OVERRIDE {
    return 0;
  }

  size_t AllocationSizeNonvirtual(mirror::Object* obj, size_t* usable_size)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Removes the fork time growth limit on capacity, allowing the application to allocate up to the
  // maximum reserved size of the heap.
  void ClearGrowthLimit() {
    growth_end_ = Limit();
  }

  // Override capacity so that we only return the possibly limited capacity
  size_t Capacity() const {
    return growth_end_ - begin_;
  }

  // The total amount of memory reserved for the space.
  size_t NonGrowthLimitCapacity() const {
    return GetMemMap()->Size();
  }

  accounting::ContinuousSpaceBitmap* GetLiveBitmap() const OVERRIDE {
    return nullptr;
  }

  accounting::ContinuousSpaceBitmap* GetMarkBitmap() const OVERRIDE {
    return nullptr;
  }

  // Reset the space to empty.
  void Clear() OVERRIDE LOCKS_EXCLUDED(block_lock_);

  void Dump(std::ostream& os) const;

  void RevokeThreadLocalBuffers(Thread* thread) LOCKS_EXCLUDED(block_lock_);
  void RevokeAllThreadLocalBuffers() LOCKS_EXCLUDED(Locks::runtime_shutdown_lock_,
                                                    Locks::thread_list_lock_);
  void AssertThreadLocalBuffersAreRevoked(Thread* thread) LOCKS_EXCLUDED(block_lock_);
  void AssertAllThreadLocalBuffersAreRevoked() LOCKS_EXCLUDED(Locks::runtime_shutdown_lock_,
                                                              Locks::thread_list_lock_);

  uint64_t GetBytesAllocated() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  uint64_t GetObjectsAllocated() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool IsEmpty() const {
    return Begin() == End();
  }

  bool CanMoveObjects() const OVERRIDE {
    return true;
  }

  bool Contains(const mirror::Object* obj) const {
    const byte* byte_obj = reinterpret_cast<const byte*>(obj);
    return byte_obj >= Begin() && byte_obj < End();
  }

  // TODO: Change this? Mainly used for compacting to a particular region of memory.
  BumpPointerSpace(const std::string& name, byte* begin, byte* limit);

  // Return the object which comes after obj, while ensuring alignment.
  static mirror::Object* GetNextObject(mirror::Object* obj)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Allocate a new TLAB, returns false if the allocation failed.
  bool AllocNewTlab(Thread* self, size_t bytes);

  BumpPointerSpace* AsBumpPointerSpace() OVERRIDE {
    return this;
  }

  // Go through all of the blocks and visit the continuous objects.
  void Walk(ObjectCallback* callback, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  accounting::ContinuousSpaceBitmap::SweepCallback* GetSweepCallback() OVERRIDE;

  // Record objects / bytes freed.
  void RecordFree(int32_t objects, int32_t bytes) {
    objects_allocated_.FetchAndSubSequentiallyConsistent(objects);
    bytes_allocated_.FetchAndSubSequentiallyConsistent(bytes);
  }

  void LogFragmentationAllocFailure(std::ostream& os, size_t failed_alloc_bytes) OVERRIDE
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Object alignment within the space.
  static constexpr size_t kAlignment = 8;

 protected:
  BumpPointerSpace(const std::string& name, MemMap* mem_map);

  // Allocate a raw block of bytes.
  byte* AllocBlock(size_t bytes) EXCLUSIVE_LOCKS_REQUIRED(block_lock_);
  void RevokeThreadLocalBuffersLocked(Thread* thread) EXCLUSIVE_LOCKS_REQUIRED(block_lock_);

  // The main block is an unbounded block where objects go when there are no other blocks. This
  // enables us to maintain tightly packed objects when you are not using thread local buffers for
  // allocation. The main block starts at the space Begin().
  void UpdateMainBlock() EXCLUSIVE_LOCKS_REQUIRED(block_lock_);

  byte* growth_end_;
  AtomicInteger objects_allocated_;  // Accumulated from revoked thread local regions.
  AtomicInteger bytes_allocated_;  // Accumulated from revoked thread local regions.
  Mutex block_lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;
  // The objects at the start of the space are stored in the main block. The main block doesn't
  // have a header, this lets us walk empty spaces which are mprotected.
  size_t main_block_size_ GUARDED_BY(block_lock_);
  // The number of blocks in the space, if it is 0 then the space has one long continuous block
  // which doesn't have an updated header.
  size_t num_blocks_ GUARDED_BY(block_lock_);

 private:
  struct BlockHeader {
    size_t size_;  // Size of the block in bytes, does not include the header.
    size_t unused_;  // Ensures alignment of kAlignment.
  };

  COMPILE_ASSERT(sizeof(BlockHeader) % kAlignment == 0,
                 continuous_block_must_be_kAlignment_aligned);

  friend class collector::MarkSweep;
  DISALLOW_COPY_AND_ASSIGN(BumpPointerSpace);
};

}  // namespace space
}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_SPACE_BUMP_POINTER_SPACE_H_
