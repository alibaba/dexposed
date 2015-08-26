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

#ifndef ART_RUNTIME_REFLECTION_H_
#define ART_RUNTIME_REFLECTION_H_

#include "jni.h"
#include "primitive.h"

namespace art {
namespace mirror {
  class ArtField;
  class ArtMethod;
  class Class;
  class Object;
}  // namespace mirror
union JValue;
class MethodHelper;
class ScopedObjectAccessAlreadyRunnable;
class ShadowFrame;
class ThrowLocation;

mirror::Object* BoxPrimitive(Primitive::Type src_class, const JValue& value)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
bool UnboxPrimitiveForField(mirror::Object* o, mirror::Class* dst_class, mirror::ArtField* f,
                            JValue* unboxed_value)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
bool UnboxPrimitiveForResult(const ThrowLocation& throw_location, mirror::Object* o,
                             mirror::Class* dst_class, JValue* unboxed_value)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

ALWAYS_INLINE bool ConvertPrimitiveValue(const ThrowLocation* throw_location, bool unbox_for_result,
                                         Primitive::Type src_class, Primitive::Type dst_class,
                                         const JValue& src, JValue* dst)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

JValue InvokeWithVarArgs(const ScopedObjectAccessAlreadyRunnable& soa, jobject obj, jmethodID mid,
                         va_list args)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

JValue InvokeWithJValues(const ScopedObjectAccessAlreadyRunnable& soa, mirror::Object* receiver,
                         jmethodID mid, jvalue* args)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

JValue InvokeVirtualOrInterfaceWithJValues(const ScopedObjectAccessAlreadyRunnable& soa,
                                           mirror::Object* receiver, jmethodID mid, jvalue* args)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

JValue InvokeVirtualOrInterfaceWithVarArgs(const ScopedObjectAccessAlreadyRunnable& soa,
                                           jobject obj, jmethodID mid, va_list args)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

void InvokeWithShadowFrame(Thread* self, ShadowFrame* shadow_frame, uint16_t arg_offset,
                           MethodHelper& mh, JValue* result)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

jobject InvokeMethod(const ScopedObjectAccessAlreadyRunnable& soa, jobject method, jobject receiver,
                     jobject args, bool accessible)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

bool VerifyObjectIsClass(mirror::Object* o, mirror::Class* c)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

bool VerifyAccess(Thread* self, mirror::Object* obj, mirror::Class* declaring_class,
                  uint32_t access_flags)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

}  // namespace art

#endif  // ART_RUNTIME_REFLECTION_H_
