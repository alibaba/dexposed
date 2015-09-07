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

#ifndef ART_RUNTIME_READ_BARRIER_INL_H_
#define ART_RUNTIME_READ_BARRIER_INL_H_

#include "read_barrier.h"

#include "mirror/object_reference.h"

namespace art {

template <typename MirrorType, ReadBarrierOption kReadBarrierOption>
inline MirrorType* ReadBarrier::Barrier(
    mirror::Object* obj, MemberOffset offset, mirror::HeapReference<MirrorType>* ref_addr) {
  // Unused for now.
  UNUSED(obj);
  UNUSED(offset);
  UNUSED(ref_addr);
  const bool with_read_barrier = kReadBarrierOption == kWithReadBarrier;
  if (with_read_barrier && kUseBakerReadBarrier) {
    // To be implemented.
    return ref_addr->AsMirrorPtr();
  } else if (with_read_barrier && kUseBrooksReadBarrier) {
    // To be implemented.
    return ref_addr->AsMirrorPtr();
  } else {
    // No read barrier.
    return ref_addr->AsMirrorPtr();
  }
}

template <typename MirrorType, ReadBarrierOption kReadBarrierOption>
inline MirrorType* ReadBarrier::BarrierForRoot(MirrorType** root) {
  MirrorType* ref = *root;
  const bool with_read_barrier = kReadBarrierOption == kWithReadBarrier;
  if (with_read_barrier && kUseBakerReadBarrier) {
    // To be implemented.
    return ref;
  } else if (with_read_barrier && kUseBrooksReadBarrier) {
    // To be implemented.
    return ref;
  } else {
    return ref;
  }
}

}  // namespace art

#endif  // ART_RUNTIME_READ_BARRIER_INL_H_
