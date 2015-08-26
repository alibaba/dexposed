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

#ifndef ART_RUNTIME_WELL_KNOWN_CLASSES_H_
#define ART_RUNTIME_WELL_KNOWN_CLASSES_H_

#include "base/mutex.h"
#include "jni.h"

namespace art {
namespace mirror {
class Class;
}  // namespace mirror

// Various classes used in JNI. We cache them so we don't have to keep looking
// them up. Similar to libcore's JniConstants (except there's no overlap, so
// we keep them separate).

jmethodID CacheMethod(JNIEnv* env, jclass c, bool is_static, const char* name, const char* signature);

struct WellKnownClasses {
 public:
  static void Init(JNIEnv* env);  // Run before native methods are registered.
  static void LateInit(JNIEnv* env);  // Run after native methods are registered.

  static mirror::Class* ToClass(jclass global_jclass)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static jclass com_android_dex_Dex;
  static jclass dalvik_system_DexFile;
  static jclass dalvik_system_DexPathList;
  static jclass dalvik_system_DexPathList$Element;
  static jclass dalvik_system_PathClassLoader;
  static jclass java_lang_BootClassLoader;
  static jclass java_lang_ClassLoader;
  static jclass java_lang_ClassNotFoundException;
  static jclass java_lang_Daemons;
  static jclass java_lang_Error;
  static jclass java_lang_Object;
  static jclass java_lang_reflect_AbstractMethod;
  static jclass java_lang_reflect_ArtMethod;
  static jclass java_lang_reflect_Constructor;
  static jclass java_lang_reflect_Field;
  static jclass java_lang_reflect_Method;
  static jclass java_lang_reflect_Proxy;
  static jclass java_lang_RuntimeException;
  static jclass java_lang_StackOverflowError;
  static jclass java_lang_String;
  static jclass java_lang_System;
  static jclass java_lang_Thread;
  static jclass java_lang_ThreadGroup;
  static jclass java_lang_Thread$UncaughtExceptionHandler;
  static jclass java_lang_Throwable;
  static jclass java_util_Collections;
  static jclass java_nio_DirectByteBuffer;
  static jclass libcore_util_EmptyArray;
  static jclass org_apache_harmony_dalvik_ddmc_Chunk;
  static jclass org_apache_harmony_dalvik_ddmc_DdmServer;

  static jmethodID com_android_dex_Dex_create;
  static jmethodID java_lang_Boolean_valueOf;
  static jmethodID java_lang_Byte_valueOf;
  static jmethodID java_lang_Character_valueOf;
  static jmethodID java_lang_ClassLoader_loadClass;
  static jmethodID java_lang_ClassNotFoundException_init;
  static jmethodID java_lang_Daemons_requestGC;
  static jmethodID java_lang_Daemons_requestHeapTrim;
  static jmethodID java_lang_Daemons_start;
  static jmethodID java_lang_Double_valueOf;
  static jmethodID java_lang_Float_valueOf;
  static jmethodID java_lang_Integer_valueOf;
  static jmethodID java_lang_Long_valueOf;
  static jmethodID java_lang_ref_FinalizerReference_add;
  static jmethodID java_lang_ref_ReferenceQueue_add;
  static jmethodID java_lang_reflect_Proxy_invoke;
  static jmethodID java_lang_Runtime_nativeLoad;
  static jmethodID java_lang_Short_valueOf;
  static jmethodID java_lang_System_runFinalization;
  static jmethodID java_lang_Thread_init;
  static jmethodID java_lang_Thread_run;
  static jmethodID java_lang_Thread$UncaughtExceptionHandler_uncaughtException;
  static jmethodID java_lang_ThreadGroup_removeThread;
  static jmethodID java_nio_DirectByteBuffer_init;
  static jmethodID org_apache_harmony_dalvik_ddmc_DdmServer_broadcast;
  static jmethodID org_apache_harmony_dalvik_ddmc_DdmServer_dispatch;

  static jfieldID dalvik_system_DexFile_cookie;
  static jfieldID dalvik_system_DexPathList_dexElements;
  static jfieldID dalvik_system_DexPathList$Element_dexFile;
  static jfieldID dalvik_system_PathClassLoader_pathList;
  static jfieldID java_lang_reflect_AbstractMethod_artMethod;
  static jfieldID java_lang_reflect_Field_artField;
  static jfieldID java_lang_reflect_Proxy_h;
  static jfieldID java_lang_Thread_daemon;
  static jfieldID java_lang_Thread_group;
  static jfieldID java_lang_Thread_lock;
  static jfieldID java_lang_Thread_name;
  static jfieldID java_lang_Thread_priority;
  static jfieldID java_lang_Thread_uncaughtHandler;
  static jfieldID java_lang_Thread_nativePeer;
  static jfieldID java_lang_ThreadGroup_mainThreadGroup;
  static jfieldID java_lang_ThreadGroup_name;
  static jfieldID java_lang_ThreadGroup_systemThreadGroup;
  static jfieldID java_lang_Throwable_cause;
  static jfieldID java_lang_Throwable_detailMessage;
  static jfieldID java_lang_Throwable_stackTrace;
  static jfieldID java_lang_Throwable_stackState;
  static jfieldID java_lang_Throwable_suppressedExceptions;
  static jfieldID java_nio_DirectByteBuffer_capacity;
  static jfieldID java_nio_DirectByteBuffer_effectiveDirectAddress;
  static jfieldID java_util_Collections_EMPTY_LIST;
  static jfieldID libcore_util_EmptyArray_STACK_TRACE_ELEMENT;
  static jfieldID org_apache_harmony_dalvik_ddmc_Chunk_data;
  static jfieldID org_apache_harmony_dalvik_ddmc_Chunk_length;
  static jfieldID org_apache_harmony_dalvik_ddmc_Chunk_offset;
  static jfieldID org_apache_harmony_dalvik_ddmc_Chunk_type;
};

}  // namespace art

#endif  // ART_RUNTIME_WELL_KNOWN_CLASSES_H_
