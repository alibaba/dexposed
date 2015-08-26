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

#ifndef ART_RUNTIME_GC_SPACE_ROSALLOC_SPACE_H_
#define ART_RUNTIME_GC_SPACE_ROSALLOC_SPACE_H_

#include "gc/allocator/rosalloc.h"
#include "malloc_space.h"
#include "space.h"

namespace art {
namespace gc {

namespace collector {
  class MarkSweep;
}  // namespace collector

namespace space {

// An alloc space implemented using a runs-of-slots memory allocator. Not final as may be
// overridden by a ValgrindMallocSpace.
class RosAllocSpace : public MallocSpace {
 public:
  // Create a RosAllocSpace with the requested sizes. The requested
  // base address is not guaranteed to be granted, if it is required,
  // the caller should call Begin on the returned space to confirm the
  // request was granted.
  static RosAllocSpace* Create(const std::string& name, size_t initial_size, size_t growth_limit,
                               size_t capacity, byte* requested_begin, bool low_memory_mode,
                               bool can_move_objects);
  static RosAllocSpace* CreateFromMemMap(MemMap* mem_map, const std::string& name,
                                         size_t starting_size, size_t initial_size,
                                         size_t growth_limit, size_t capacity,
                                         bool low_memory_mode, bool can_move_objects);

  mirror::Object* AllocWithGrowth(Thread* self, size_t num_bytes, size_t* bytes_allocated,
                                  size_t* usable_size) OVERRIDE LOCKS_EXCLUDED(lock_);
  mirror::Object* Alloc(Thread* self, size_t num_bytes, size_t* bytes_allocated,
                        size_t* usable_size) OVERRIDE {
    return AllocNonvirtual(self, num_bytes, bytes_allocated, usable_size);
  }
  mirror::Object* AllocThreadUnsafe(Thread* self, size_t num_bytes, size_t* bytes_allocated,
                                    size_t* usable_size)
      OVERRIDE EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return AllocNonvirtualThreadUnsafe(self, num_bytes, bytes_allocated, usable_size);
  }
  size_t AllocationSize(mirror::Object* obj, size_t* usable_size) OVERRIDE {
    return AllocationSizeNonvirtual(obj, usable_size);
  }
  size_t Free(Thread* self, mirror::Object* ptr) OVERRIDE
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  size_t FreeList(Thread* self, size_t num_ptrs, mirror::Object** ptrs) OVERRIDE
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::Object* AllocNonvirtual(Thread* self, size_t num_bytes, size_t* bytes_allocated,
                                  size_t* usable_size) {
    // RosAlloc zeroes memory internally.
    return AllocCommon(self, num_bytes, bytes_allocated, usable_size);
  }
  mirror::Object* AllocNonvirtualThreadUnsafe(Thread* self, size_t num_bytes,
                                              size_t* bytes_allocated, size_t* usable_size) {
    // RosAlloc zeroes memory internally. Pass in false for thread unsafe.
    return AllocCommon<false>(self, num_bytes, bytes_allocated, usable_size);
  }

  // TODO: NO_THREAD_SAFETY_ANALYSIS because SizeOf() requires that mutator_lock is held.
  size_t AllocationSizeNonvirtual(mirror::Object* obj, size_t* usable_size)
      NO_THREAD_SAFETY_ANALYSIS;

  allocator::RosAlloc* GetRosAlloc() const {
    return rosalloc_;
  }

  size_t Trim() OVERRIDE;
  void Walk(WalkCallback callback, void* arg) OVERRIDE LOCKS_EXCLUDED(lock_);
  size_t GetFootprint() OVERRIDE;
  size_t GetFootprintLimit() OVERRIDE;
  void SetFootprintLimit(size_t limit) OVERRIDE;

  void Clear() OVERRIDE;

  MallocSpace* CreateInstance(const std::string& name, MemMap* mem_map, void* allocator,
                              byte* begin, byte* end, byte* limit, size_t growth_limit,
                              bool can_move_objects) OVERRIDE;

  uint64_t GetBytesAllocated() OVERRIDE;
  uint64_t GetObjectsAllocated() OVERRIDE;

  void RevokeThreadLocalBuffers(Thread* thread);
  void RevokeAllThreadLocalBuffers();
  void AssertAllThreadLocalBuffersAreRevoked();

  // Returns the class of a recently freed object.
  mirror::Class* FindRecentFreedObject(const mirror::Object* obj);

  bool IsRosAllocSpace() const OVERRIDE {
    return true;
  }

  RosAllocSpace* AsRosAllocSpace() OVERRIDE {
    return this;
  }

  void Verify() EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_) {
    rosalloc_->Verify();
  }

  virtual ~RosAllocSpace();

  void LogFragmentationAllocFailure(std::ostream& os, size_t failed_alloc_bytes) OVERRIDE {
    rosalloc_->LogFragmentationAllocFailure(os, failed_alloc_bytes);
  }

 protected:
  RosAllocSpace(const std::string& name, MemMap* mem_map, allocator::RosAlloc* rosalloc,
                byte* begin, byte* end, byte* limit, size_t growth_limit, bool can_move_objects,
                size_t starting_size, size_t initial_size, bool low_memory_mode);

 private:
  template<bool kThreadSafe = true>
  mirror::Object* AllocCommon(Thread* self, size_t num_bytes, size_t* bytes_allocated,
                              size_t* usable_size);

  void* CreateAllocator(void* base, size_t morecore_start, size_t initial_size,
                        size_t maximum_size, bool low_memory_mode) OVERRIDE {
    return CreateRosAlloc(base, morecore_start, initial_size, maximum_size, low_memory_mode);
  }
  static allocator::RosAlloc* CreateRosAlloc(void* base, size_t morecore_start, size_t initial_size,
                                             size_t maximum_size, bool low_memory_mode);

  void InspectAllRosAlloc(void (*callback)(void *start, void *end, size_t num_bytes, void* callback_arg),
                          void* arg, bool do_null_callback_at_end)
      LOCKS_EXCLUDED(Locks::runtime_shutdown_lock_, Locks::thread_list_lock_);
  void InspectAllRosAllocWithSuspendAll(
      void (*callback)(void *start, void *end, size_t num_bytes, void* callback_arg),
      void* arg, bool do_null_callback_at_end)
      LOCKS_EXCLUDED(Locks::runtime_shutdown_lock_, Locks::thread_list_lock_);

  // Underlying rosalloc.
  allocator::RosAlloc* rosalloc_;

  const bool low_memory_mode_;

  friend class collector::MarkSweep;

  DISALLOW_COPY_AND_ASSIGN(RosAllocSpace);
};

}  // namespace space
}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_SPACE_ROSALLOC_SPACE_H_
