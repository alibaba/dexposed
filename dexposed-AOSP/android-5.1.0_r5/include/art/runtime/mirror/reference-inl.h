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

#ifndef ART_RUNTIME_MIRROR_REFERENCE_INL_H_
#define ART_RUNTIME_MIRROR_REFERENCE_INL_H_

#include "reference.h"

namespace art {
namespace mirror {

inline uint32_t Reference::ClassSize() {
  uint32_t vtable_entries = Object::kVTableLength + 5;
  return Class::ComputeClassSize(false, vtable_entries, 2, 0, 0);
}

inline bool Reference::IsEnqueuable() {
  // Not using volatile reads as an optimization since this is only called with all the mutators
  // suspended.
  const Object* queue = GetFieldObject<mirror::Object>(QueueOffset());
  const Object* queue_next = GetFieldObject<mirror::Object>(QueueNextOffset());
  return queue != nullptr && queue_next == nullptr;
}

}  // namespace mirror
}  // namespace art

#endif  // ART_RUNTIME_MIRROR_REFERENCE_INL_H_
