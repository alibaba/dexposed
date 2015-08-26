/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ART_RUNTIME_ENTRYPOINTS_JNI_JNI_ENTRYPOINTS_H_
#define ART_RUNTIME_ENTRYPOINTS_JNI_JNI_ENTRYPOINTS_H_

#include "base/macros.h"
#include "offsets.h"

#define JNI_ENTRYPOINT_OFFSET(ptr_size, x) \
    Thread::JniEntryPointOffset<ptr_size>(OFFSETOF_MEMBER(JniEntryPoints, x))

namespace art {

// Pointers to functions that are called by JNI trampolines via thread-local storage.
struct PACKED(4) JniEntryPoints {
  // Called when the JNI method isn't registered.
  void* (*pDlsymLookup)(JNIEnv* env, jobject);
};

}  // namespace art

#endif  // ART_RUNTIME_ENTRYPOINTS_JNI_JNI_ENTRYPOINTS_H_
