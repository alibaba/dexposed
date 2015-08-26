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

#ifndef ART_RUNTIME_GC_SPACE_ROSALLOC_SPACE_INL_H_
#define ART_RUNTIME_GC_SPACE_ROSALLOC_SPACE_INL_H_

#include "gc/allocator/rosalloc-inl.h"
#include "rosalloc_space.h"
#include "thread.h"

namespace art {
namespace gc {
namespace space {

inline size_t RosAllocSpace::AllocationSizeNonvirtual(mirror::Object* obj, size_t* usable_size) {
  void* obj_ptr = const_cast<void*>(reinterpret_cast<const void*>(obj));
  // obj is a valid object. Use its class in the header to get the size.
  // Don't use verification since the object may be dead if we are sweeping.
  size_t size = obj->SizeOf<kVerifyNone>();
  size_t size_by_size = rosalloc_->UsableSize(size);
  if (kIsDebugBuild) {
    size_t size_by_ptr = rosalloc_->UsableSize(obj_ptr);
    if (size_by_size != size_by_ptr) {
      LOG(INFO) << "Found a bad sized obj of size " << size
                << " at " << std::hex << reinterpret_cast<intptr_t>(obj_ptr) << std::dec
                << " size_by_size=" << size_by_size << " size_by_ptr=" << size_by_ptr;
    }
    DCHECK_EQ(size_by_size, size_by_ptr);
  }
  if (usable_size != nullptr) {
    *usable_size = size_by_size;
  }
  return size_by_size;
}

template<bool kThreadSafe>
inline mirror::Object* RosAllocSpace::AllocCommon(Thread* self, size_t num_bytes,
                                                  size_t* bytes_allocated, size_t* usable_size) {
  size_t rosalloc_size = 0;
  if (!kThreadSafe) {
    Locks::mutator_lock_->AssertExclusiveHeld(self);
  }
  mirror::Object* result = reinterpret_cast<mirror::Object*>(
      rosalloc_->Alloc<kThreadSafe>(self, num_bytes, &rosalloc_size));
  if (LIKELY(result != NULL)) {
    if (kDebugSpaces) {
      CHECK(Contains(result)) << "Allocation (" << reinterpret_cast<void*>(result)
            << ") not in bounds of allocation space " << *this;
    }
    DCHECK(bytes_allocated != NULL);
    *bytes_allocated = rosalloc_size;
    DCHECK_EQ(rosalloc_size, rosalloc_->UsableSize(result));
    if (usable_size != nullptr) {
      *usable_size = rosalloc_size;
    }
  }
  return result;
}

}  // namespace space
}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_SPACE_ROSALLOC_SPACE_INL_H_
