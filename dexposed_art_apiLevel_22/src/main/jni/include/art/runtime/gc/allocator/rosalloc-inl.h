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

#ifndef ART_RUNTIME_GC_ALLOCATOR_ROSALLOC_INL_H_
#define ART_RUNTIME_GC_ALLOCATOR_ROSALLOC_INL_H_

#include "rosalloc.h"

namespace art {
namespace gc {
namespace allocator {

template<bool kThreadSafe>
inline ALWAYS_INLINE void* RosAlloc::Alloc(Thread* self, size_t size, size_t* bytes_allocated) {
  if (UNLIKELY(size > kLargeSizeThreshold)) {
    return AllocLargeObject(self, size, bytes_allocated);
  }
  void* m;
  if (kThreadSafe) {
    m = AllocFromRun(self, size, bytes_allocated);
  } else {
    m = AllocFromRunThreadUnsafe(self, size, bytes_allocated);
  }
  // Check if the returned memory is really all zero.
  if (kCheckZeroMemory && m != nullptr) {
    byte* bytes = reinterpret_cast<byte*>(m);
    for (size_t i = 0; i < size; ++i) {
      DCHECK_EQ(bytes[i], 0);
    }
  }
  return m;
}

}  // namespace allocator
}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_ALLOCATOR_ROSALLOC_INL_H_
