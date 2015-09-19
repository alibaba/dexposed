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

#ifndef ART_RUNTIME_ENTRYPOINTS_ENTRYPOINT_UTILS_H_
#define ART_RUNTIME_ENTRYPOINTS_ENTRYPOINT_UTILS_H_

#include <jni.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/mutex.h"
#include "gc/allocator_type.h"
#include "invoke_type.h"
#include "jvalue.h"

namespace art {

namespace mirror {
  class Class;
  class Array;
  class ArtField;
  class ArtMethod;
  class Object;
  class String;
}  // namespace mirror

class ScopedObjectAccessAlreadyRunnable;
class Thread;

template <const bool kAccessCheck>
ALWAYS_INLINE static inline mirror::Class* CheckObjectAlloc(uint32_t type_idx,
                                                            mirror::ArtMethod* method,
                                                            Thread* self, bool* slow_path)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

// TODO: Fix no thread safety analysis when annotalysis is smarter.
ALWAYS_INLINE static inline mirror::Class* CheckClassInitializedForObjectAlloc(mirror::Class* klass,
                                                                               Thread* self, bool* slow_path)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

// Given the context of a calling Method, use its DexCache to resolve a type to a Class. If it
// cannot be resolved, throw an error. If it can, use it to create an instance.
// When verification/compiler hasn't been able to verify access, optionally perform an access
// check.
template <bool kAccessCheck, bool kInstrumented>
ALWAYS_INLINE static inline mirror::Object* AllocObjectFromCode(uint32_t type_idx,
                                                                mirror::ArtMethod* method,
                                                                Thread* self,
                                                                gc::AllocatorType allocator_type)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

// Given the context of a calling Method and a resolved class, create an instance.
template <bool kInstrumented>
ALWAYS_INLINE static inline mirror::Object* AllocObjectFromCodeResolved(mirror::Class* klass,
                                                                        mirror::ArtMethod* method,
                                                                        Thread* self,
                                                                        gc::AllocatorType allocator_type)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

// Given the context of a calling Method and an initialized class, create an instance.
template <bool kInstrumented>
ALWAYS_INLINE static inline mirror::Object* AllocObjectFromCodeInitialized(mirror::Class* klass,
                                                                           mirror::ArtMethod* method,
                                                                           Thread* self,
                                                                           gc::AllocatorType allocator_type)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);


template <bool kAccessCheck>
ALWAYS_INLINE static inline mirror::Class* CheckArrayAlloc(uint32_t type_idx,
                                                           mirror::ArtMethod* method,
                                                           int32_t component_count,
                                                           bool* slow_path)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

// Given the context of a calling Method, use its DexCache to resolve a type to an array Class. If
// it cannot be resolved, throw an error. If it can, use it to create an array.
// When verification/compiler hasn't been able to verify access, optionally perform an access
// check.
template <bool kAccessCheck, bool kInstrumented>
ALWAYS_INLINE static inline mirror::Array* AllocArrayFromCode(uint32_t type_idx,
                                                              mirror::ArtMethod* method,
                                                              int32_t component_count,
                                                              Thread* self,
                                                              gc::AllocatorType allocator_type)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

template <bool kAccessCheck, bool kInstrumented>
ALWAYS_INLINE static inline mirror::Array* AllocArrayFromCodeResolved(mirror::Class* klass,
                                                                      mirror::ArtMethod* method,
                                                                      int32_t component_count,
                                                                      Thread* self,
                                                                      gc::AllocatorType allocator_type)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

extern mirror::Array* CheckAndAllocArrayFromCode(uint32_t type_idx, mirror::ArtMethod* method,
                                                 int32_t component_count, Thread* self,
                                                 bool access_check,
                                                 gc::AllocatorType allocator_type)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

extern mirror::Array* CheckAndAllocArrayFromCodeInstrumented(uint32_t type_idx,
                                                             mirror::ArtMethod* method,
                                                             int32_t component_count, Thread* self,
                                                             bool access_check,
                                                             gc::AllocatorType allocator_type)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

// Type of find field operation for fast and slow case.
enum FindFieldType {
  InstanceObjectRead,
  InstanceObjectWrite,
  InstancePrimitiveRead,
  InstancePrimitiveWrite,
  StaticObjectRead,
  StaticObjectWrite,
  StaticPrimitiveRead,
  StaticPrimitiveWrite,
};

template<FindFieldType type, bool access_check>
static inline mirror::ArtField* FindFieldFromCode(uint32_t field_idx, mirror::ArtMethod* referrer,
                                                  Thread* self, size_t expected_size)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

template<InvokeType type, bool access_check>
static inline mirror::ArtMethod* FindMethodFromCode(uint32_t method_idx,
                                                    mirror::Object** this_object,
                                                    mirror::ArtMethod** referrer, Thread* self)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

// Fast path field resolution that can't initialize classes or throw exceptions.
static inline mirror::ArtField* FindFieldFast(uint32_t field_idx,
                                              mirror::ArtMethod* referrer,
                                              FindFieldType type, size_t expected_size)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

// Fast path method resolution that can't throw exceptions.
static inline mirror::ArtMethod* FindMethodFast(uint32_t method_idx,
                                                mirror::Object* this_object,
                                                mirror::ArtMethod* referrer,
                                                bool access_check, InvokeType type)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

static inline mirror::Class* ResolveVerifyAndClinit(uint32_t type_idx,
                                                    mirror::ArtMethod* referrer,
                                                    Thread* self, bool can_run_clinit,
                                                    bool verify_access)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

extern void ThrowStackOverflowError(Thread* self) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

static inline mirror::String* ResolveStringFromCode(mirror::ArtMethod* referrer,
                                                    uint32_t string_idx)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

// TODO: annotalysis disabled as monitor semantics are maintained in Java code.
static inline void UnlockJniSynchronizedMethod(jobject locked, Thread* self)
    NO_THREAD_SAFETY_ANALYSIS;

void CheckReferenceResult(mirror::Object* o, Thread* self)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

static inline void CheckSuspend(Thread* thread) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

JValue InvokeProxyInvocationHandler(ScopedObjectAccessAlreadyRunnable& soa, const char* shorty,
                                    jobject rcvr_jobj, jobject interface_art_method_jobj,
                                    std::vector<jvalue>& args)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

// Entry point for deoptimization.
extern "C" void art_quick_deoptimize();
static inline uintptr_t GetQuickDeoptimizationEntryPoint() {
  return reinterpret_cast<uintptr_t>(art_quick_deoptimize);
}

// Return address of instrumentation stub.
extern "C" void art_quick_instrumentation_entry(void*);
static inline void* GetQuickInstrumentationEntryPoint() {
  return reinterpret_cast<void*>(art_quick_instrumentation_entry);
}

// The return_pc of instrumentation exit stub.
extern "C" void art_quick_instrumentation_exit();
static inline uintptr_t GetQuickInstrumentationExitPc() {
  return reinterpret_cast<uintptr_t>(art_quick_instrumentation_exit);
}

#if defined(ART_USE_PORTABLE_COMPILER)
extern "C" void art_portable_to_interpreter_bridge(mirror::ArtMethod*);
static inline const void* GetPortableToInterpreterBridge() {
  return reinterpret_cast<void*>(art_portable_to_interpreter_bridge);
}

static inline const void* GetPortableToQuickBridge() {
  // TODO: portable to quick bridge. Bug: 8196384
  return GetPortableToInterpreterBridge();
}
#endif

extern "C" void art_quick_to_interpreter_bridge(mirror::ArtMethod*);
static inline const void* GetQuickToInterpreterBridge() {
  return reinterpret_cast<void*>(art_quick_to_interpreter_bridge);
}

#if defined(ART_USE_PORTABLE_COMPILER)
static inline const void* GetQuickToPortableBridge() {
  // TODO: quick to portable bridge. Bug: 8196384
  return GetQuickToInterpreterBridge();
}

extern "C" void art_portable_proxy_invoke_handler();
static inline const void* GetPortableProxyInvokeHandler() {
  return reinterpret_cast<void*>(art_portable_proxy_invoke_handler);
}
#endif

extern "C" void art_quick_proxy_invoke_handler();
static inline const void* GetQuickProxyInvokeHandler() {
  return reinterpret_cast<void*>(art_quick_proxy_invoke_handler);
}

extern "C" void* art_jni_dlsym_lookup_stub(JNIEnv*, jobject);
static inline void* GetJniDlsymLookupStub() {
  return reinterpret_cast<void*>(art_jni_dlsym_lookup_stub);
}

template <typename INT_TYPE, typename FLOAT_TYPE>
static inline INT_TYPE art_float_to_integral(FLOAT_TYPE f);

}  // namespace art

#endif  // ART_RUNTIME_ENTRYPOINTS_ENTRYPOINT_UTILS_H_
