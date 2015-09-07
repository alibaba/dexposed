/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef ART_RUNTIME_GC_ALLOCATOR_TYPE_H_
#define ART_RUNTIME_GC_ALLOCATOR_TYPE_H_

namespace art {
namespace gc {

// Different types of allocators.
enum AllocatorType {
  kAllocatorTypeBumpPointer,  // Use BumpPointer allocator, has entrypoints.
  kAllocatorTypeTLAB,  // Use TLAB allocator, has entrypoints.
  kAllocatorTypeRosAlloc,  // Use RosAlloc allocator, has entrypoints.
  kAllocatorTypeDlMalloc,  // Use dlmalloc allocator, has entrypoints.
  kAllocatorTypeNonMoving,  // Special allocator for non moving objects, doesn't have entrypoints.
  kAllocatorTypeLOS,  // Large object space, also doesn't have entrypoints.
};

}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_ALLOCATOR_TYPE_H_
