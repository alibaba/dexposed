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

#ifndef ART_RUNTIME_READ_BARRIER_H_
#define ART_RUNTIME_READ_BARRIER_H_

#include "base/mutex.h"
#include "base/macros.h"
#include "offsets.h"
#include "read_barrier_c.h"

// This is a C++ (not C) header file, separate from read_barrier_c.h
// which needs to be a C header file for asm_support.h.

namespace art {
namespace mirror {
  class Object;
  template<typename MirrorType> class HeapReference;
}  // namespace mirror

class ReadBarrier {
 public:
  // It's up to the implementation whether the given field gets
  // updated whereas the return value must be an updated reference.
  template <typename MirrorType, ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  ALWAYS_INLINE static MirrorType* Barrier(
      mirror::Object* obj, MemberOffset offset, mirror::HeapReference<MirrorType>* ref_addr)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // It's up to the implementation whether the given root gets updated
  // whereas the return value must be an updated reference.
  template <typename MirrorType, ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  ALWAYS_INLINE static MirrorType* BarrierForRoot(MirrorType** root)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
};

}  // namespace art

#endif  // ART_RUNTIME_READ_BARRIER_H_
