/*
 * Original work Copyright (c) 2005-2008, The Android Open Source Project
 * Modified work Copyright (c) 2013, rovo89 and Tungstwenty
 * Modified work Copyright (c) 2015, Alibaba Mobile Infrastructure (Android) Team
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

#ifndef XPOSED_H_
#define XPOSED_H_

#define ANDROID_SMP 0

#if PLATFORM_SDK_VERSION < 14
#ifdef  __cplusplus
extern "C" {
#endif
#endif

#include "Dalvik.h"

#if PLATFORM_SDK_VERSION < 14
#ifdef  __cplusplus
}
#endif
#endif

namespace android {

#define XPOSED_CLASS "com/taobao/android/dexposed/XposedBridge"
#define XPOSED_CLASS_DOTS "com.taobao.android.dexposed.XposedBridge"
#define XPOSED_VERSION "51"

#define NOALOG

#ifndef NOALOG
#define ALOGD LOGD
#define ALOGE LOGE
#define ALOGI LOGI
#define ALOGV LOGV
#else
#define ALOGD
#define ALOGE
#define ALOGI
#define ALOGV
#endif


struct XposedHookInfo {
    struct {
        Method originalMethod;
        // copy a few bytes more than defined for Method in AOSP
        // to accomodate for (rare) extensions by the target ROM
        int dummyForRomExtensions[4];
    } originalMethodStruct;

    Object* reflectedMethod;
    Object* additionalInfo;
};

// called directoy by app_process
void xposedInfo();
bool isRunningDalvik();
bool xposedOnVmCreated(JNIEnv* env, const char* className);
static bool xposedInitMemberOffsets(JNIEnv* env);
static inline void xposedSetObjectArrayElement(const ArrayObject* obj, int index, Object* val);

// handling hooked methods / helpers
static void xposedCallHandler(const u4* args, JValue* pResult, const Method* method, ::Thread* self);
static jobject xposedAddLocalReference(::Thread* self, Object* obj);
static void replaceAsm(uintptr_t function, unsigned const char* newCode, size_t len);
static void patchReturnTrue(uintptr_t function);
static inline bool xposedIsHooked(const Method* method);

// JNI methods
static jboolean com_taobao_android_dexposed_XposedBridge_initNative(JNIEnv* env, jclass clazz);
static void com_taobao_android_dexposed_XposedBridge_hookMethodNative(JNIEnv* env, jclass clazz, jobject reflectedMethodIndirect,
            jobject declaredClassIndirect, jint slot, jobject additionalInfoIndirect);
static void com_taobao_android_dexposed_XposedBridge_invokeOriginalMethodNative(const u4* args, JValue* pResult, const Method* method, ::Thread* self);
static void com_taobao_android_dexposed_XposedBridge_invokeSuperNative(const u4* args, JValue* pResult, const Method* method, ::Thread* self);

static int register_com_taobao_android_dexposed_XposedBridge(JNIEnv* env);
}

#endif  // XPOSED_H_
