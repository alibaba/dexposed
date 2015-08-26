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

#ifndef ART_RUNTIME_VERIFY_OBJECT_INL_H_
#define ART_RUNTIME_VERIFY_OBJECT_INL_H_

#include "verify_object.h"

#include "gc/heap.h"
#include "mirror/class-inl.h"
#include "mirror/object-inl.h"

namespace art {

inline void VerifyObject(mirror::Object* obj) {
  if (kVerifyObjectSupport > kVerifyObjectModeDisabled && obj != nullptr) {
    if (kVerifyObjectSupport > kVerifyObjectModeFast) {
      // Slow object verification, try the heap right away.
      Runtime::Current()->GetHeap()->VerifyObjectBody(obj);
    } else {
      // Fast object verification, only call the heap if our quick sanity tests fail. The heap will
      // print the diagnostic message.
      bool failed = !IsAligned<kObjectAlignment>(obj);
      if (!failed) {
        mirror::Class* c = obj->GetClass<kVerifyNone>();
        failed = failed || !IsAligned<kObjectAlignment>(c);
        failed = failed || !VerifyClassClass(c);
      }
      if (UNLIKELY(failed)) {
        Runtime::Current()->GetHeap()->VerifyObjectBody(obj);
      }
    }
  }
}

inline bool VerifyClassClass(mirror::Class* c) {
  if (UNLIKELY(c == nullptr)) {
    return false;
  }
  // Note: We pass in flags to ensure that the accessors don't call VerifyObject.
  mirror::Class* c_c = c->GetClass<kVerifyNone>();
  return c_c != nullptr && c_c == c_c->GetClass<kVerifyNone>();
}

}  // namespace art

#endif  // ART_RUNTIME_VERIFY_OBJECT_INL_H_
