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

#ifndef ART_RUNTIME_JNI_INTERNAL_INL_H_
#define ART_RUNTIME_JNI_INTERNAL_INL_H_

#include "jni_internal.h"

#include "utils.h"

namespace art {

template<typename T>
inline T JNIEnvExt::AddLocalReference(mirror::Object* obj) {
  IndirectRef ref = locals.Add(local_ref_cookie, obj);

  // TODO: fix this to understand PushLocalFrame, so we can turn it on.
  if (false) {
    if (check_jni) {
      size_t entry_count = locals.Capacity();
      if (entry_count > 16) {
        locals.Dump(LOG(WARNING) << "Warning: more than 16 JNI local references: "
            << entry_count << " (most recent was a " << PrettyTypeOf(obj) << ")\n");
        // TODO: LOG(FATAL) in a later release?
      }
    }
  }

  return reinterpret_cast<T>(ref);
}

}  // namespace art

#endif  // ART_RUNTIME_JNI_INTERNAL_INL_H_
