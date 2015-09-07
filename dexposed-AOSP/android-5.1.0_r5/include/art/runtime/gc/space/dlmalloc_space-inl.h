/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef ART_RUNTIME_GC_SPACE_DLMALLOC_SPACE_INL_H_
#define ART_RUNTIME_GC_SPACE_DLMALLOC_SPACE_INL_H_

#include "dlmalloc_space.h"
#include "gc/allocator/dlmalloc.h"
#include "thread.h"

namespace art {
namespace gc {
namespace space {

inline mirror::Object* DlMallocSpace::AllocNonvirtual(Thread* self, size_t num_bytes,
                                                      size_t* bytes_allocated,
                                                      size_t* usable_size) {
  mirror::Object* obj;
  {
    MutexLock mu(self, lock_);
    obj = AllocWithoutGrowthLocked(self, num_bytes, bytes_allocated, usable_size);
  }
  if (LIKELY(obj != NULL)) {
    // Zero freshly allocated memory, done while not holding the space's lock.
    memset(obj, 0, num_bytes);
  }
  return obj;
}

inline size_t DlMallocSpace::AllocationSizeNonvirtual(mirror::Object* obj, size_t* usable_size) {
  void* obj_ptr = const_cast<void*>(reinterpret_cast<const void*>(obj));
  size_t size = mspace_usable_size(obj_ptr);
  if (usable_size != nullptr) {
    *usable_size = size;
  }
  return size + kChunkOverhead;
}

inline mirror::Object* DlMallocSpace::AllocWithoutGrowthLocked(Thread* /*self*/, size_t num_bytes,
                                                               size_t* bytes_allocated,
                                                               size_t* usable_size) {
  mirror::Object* result = reinterpret_cast<mirror::Object*>(mspace_malloc(mspace_, num_bytes));
  if (LIKELY(result != NULL)) {
    if (kDebugSpaces) {
      CHECK(Contains(result)) << "Allocation (" << reinterpret_cast<void*>(result)
            << ") not in bounds of allocation space " << *this;
    }
    size_t allocation_size = AllocationSizeNonvirtual(result, usable_size);
    DCHECK(bytes_allocated != NULL);
    *bytes_allocated = allocation_size;
  }
  return result;
}

}  // namespace space
}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_SPACE_DLMALLOC_SPACE_INL_H_
