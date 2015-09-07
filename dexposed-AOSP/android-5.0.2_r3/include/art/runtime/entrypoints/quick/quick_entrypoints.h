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

#ifndef ART_RUNTIME_ENTRYPOINTS_QUICK_QUICK_ENTRYPOINTS_H_
#define ART_RUNTIME_ENTRYPOINTS_QUICK_QUICK_ENTRYPOINTS_H_

#include <jni.h>

#include "base/macros.h"
#include "offsets.h"

#define QUICK_ENTRYPOINT_OFFSET(ptr_size, x) \
    Thread::QuickEntryPointOffset<ptr_size>(OFFSETOF_MEMBER(QuickEntryPoints, x))

namespace art {

namespace mirror {
class ArtMethod;
class Class;
class Object;
}  // namespace mirror

class Thread;

// Pointers to functions that are called by quick compiler generated code via thread-local storage.
struct PACKED(4) QuickEntryPoints {
#define ENTRYPOINT_ENUM(name, rettype, ...) rettype ( * p ## name )( __VA_ARGS__ );
#include "quick_entrypoints_list.h"
  QUICK_ENTRYPOINT_LIST(ENTRYPOINT_ENUM)
#undef QUICK_ENTRYPOINT_LIST
#undef ENTRYPOINT_ENUM
};


// JNI entrypoints.
// TODO: NO_THREAD_SAFETY_ANALYSIS due to different control paths depending on fast JNI.
extern uint32_t JniMethodStart(Thread* self) NO_THREAD_SAFETY_ANALYSIS HOT_ATTR;
extern uint32_t JniMethodStartSynchronized(jobject to_lock, Thread* self)
    NO_THREAD_SAFETY_ANALYSIS HOT_ATTR;
extern void JniMethodEnd(uint32_t saved_local_ref_cookie, Thread* self)
    NO_THREAD_SAFETY_ANALYSIS HOT_ATTR;
extern void JniMethodEndSynchronized(uint32_t saved_local_ref_cookie, jobject locked,
                                     Thread* self)
    NO_THREAD_SAFETY_ANALYSIS HOT_ATTR;
extern mirror::Object* JniMethodEndWithReference(jobject result, uint32_t saved_local_ref_cookie,
                                                 Thread* self)
    NO_THREAD_SAFETY_ANALYSIS HOT_ATTR;

extern mirror::Object* JniMethodEndWithReferenceSynchronized(jobject result,
                                                             uint32_t saved_local_ref_cookie,
                                                             jobject locked, Thread* self)
    NO_THREAD_SAFETY_ANALYSIS HOT_ATTR;

}  // namespace art

#endif  // ART_RUNTIME_ENTRYPOINTS_QUICK_QUICK_ENTRYPOINTS_H_
