/*
 * Copyright 2014-2015 Marvin Wißfeld
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <jni.h>
#include <android/log.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <dlfcn.h>
#include <pthread.h>

#define LOGV(...)  ((void)__android_log_print(ANDROID_LOG_VERBOSE, "Dexposed_Art", __VA_ARGS__))

#define JNIHOOK_CLASS "com/taobao/android/dexposed/art/Memory"
#define JNIENTRY_CLASS "com/taobao/android/dexposed/art/hook/Entry"

JavaVM* gJVM;
JNIEnv* gEnv;

jclass methodClass;

jobject (*addWeakGloablReference)(JavaVM*, void*, void*);

static jclass gEntryClass;
static jmethodID gMethods[22];
static jclass gObjectClass;

//pthread_mutex_t lock_mutex;

int* switch_entry(int flag) {
    return (int*) gMethods[flag];
}

void init_entries(JNIEnv* env) {

    void* handle = dlopen("libart.so", RTLD_LAZY | RTLD_GLOBAL);
    addWeakGloablReference = (jobject (*)(JavaVM*, void*, void*)) dlsym(handle, "_ZN3art9JavaVMExt22AddWeakGlobalReferenceEPNS_6ThreadEPNS_6mirror6ObjectE");

    void* artInterpreterToInterpreterBridge = dlsym(handle, "artInterpreterToInterpreterBridge");
    void* artInterpreterToCompiledCodeBridge = dlsym(handle, "artInterpreterToCompiledCodeBridge");
    void* art_quick_resolution_trampoline = dlsym(handle, "art_quick_resolution_trampoline");
    //void* GetCompiledCodeToInterpreterBridge = dlsym(handle, "GetCompiledCodeToInterpreterBridge");


    LOGV("artInterpreterToInterpreterBridge: %lx", artInterpreterToInterpreterBridge);
    LOGV("artInterpreterToCompiledCodeBridge: %lx", artInterpreterToCompiledCodeBridge);
    LOGV("art_quick_resolution_trampoline: %lx", art_quick_resolution_trampoline);
    //LOGV("GetCompiledCodeToInterpreterBridge: %lx", GetCompiledCodeToInterpreterBridge);

    jclass cls_method = env->FindClass("java/lang/reflect/Method");
    methodClass  = (jclass) env->NewGlobalRef((jobject) cls_method);

    jclass cls_entry = env->FindClass(JNIENTRY_CLASS);
    gEntryClass  = (jclass) env->NewGlobalRef((jobject) cls_entry);
    jclass cls_object = env->FindClass("java/lang/Object");
    gObjectClass  = (jclass) env->NewGlobalRef((jobject) cls_object);

    gMethods[0] = (env->GetStaticMethodID(gEntryClass, "boxArgs", "([Ljava/lang/Object;ILjava/lang/Object;)I"));
    gMethods[1] = (env->GetStaticMethodID(gEntryClass, "boxArgs", "([Ljava/lang/Object;ID)I"));
    gMethods[2] = (env->GetStaticMethodID(gEntryClass, "boxArgs", "([Ljava/lang/Object;IJ)I"));
    gMethods[3] = (env->GetStaticMethodID(gEntryClass, "boxArgs", "([Ljava/lang/Object;IB)I"));
    gMethods[4] = (env->GetStaticMethodID(gEntryClass, "boxArgs", "([Ljava/lang/Object;IC)I"));
    gMethods[5] = (env->GetStaticMethodID(gEntryClass, "boxArgs", "([Ljava/lang/Object;IZ)I"));
    gMethods[6] = (env->GetStaticMethodID(gEntryClass, "boxArgs", "([Ljava/lang/Object;IF)I"));
    gMethods[7] = (env->GetStaticMethodID(gEntryClass, "boxArgs", "([Ljava/lang/Object;IS)I"));
    gMethods[8] = (env->GetStaticMethodID(gEntryClass, "boxArgs", "([Ljava/lang/Object;II)I"));

    gMethods[9] = (env->GetStaticMethodID(gEntryClass, "isStatic", "(Ljava/lang/reflect/Method;)Z"));
    gMethods[10] = (env->GetStaticMethodID(gEntryClass, "getParamList", "(Ljava/lang/reflect/Method;)[I"));
    gMethods[11] = (env->GetStaticMethodID(gEntryClass, "getReturnType", "(Ljava/lang/reflect/Method;)I"));

    gMethods[12] = (env->GetStaticMethodID(gEntryClass, "onHookObject", "(Ljava/lang/Object;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;"));
    gMethods[13] = (env->GetStaticMethodID(gEntryClass, "onHookDouble", "(Ljava/lang/Object;Ljava/lang/Object;[Ljava/lang/Object;)D"));
    gMethods[14] = (env->GetStaticMethodID(gEntryClass, "onHookLong",   "(Ljava/lang/Object;Ljava/lang/Object;[Ljava/lang/Object;)J"));
    gMethods[15] = (env->GetStaticMethodID(gEntryClass, "onHookByte",   "(Ljava/lang/Object;Ljava/lang/Object;[Ljava/lang/Object;)B"));
    gMethods[16] = (env->GetStaticMethodID(gEntryClass, "onHookChar",   "(Ljava/lang/Object;Ljava/lang/Object;[Ljava/lang/Object;)C"));
    gMethods[17] = (env->GetStaticMethodID(gEntryClass, "onHookBoolean","(Ljava/lang/Object;Ljava/lang/Object;[Ljava/lang/Object;)Z"));
    gMethods[18] = (env->GetStaticMethodID(gEntryClass, "onHookFloat",  "(Ljava/lang/Object;Ljava/lang/Object;[Ljava/lang/Object;)F"));
    gMethods[19] = (env->GetStaticMethodID(gEntryClass, "onHookShort",  "(Ljava/lang/Object;Ljava/lang/Object;[Ljava/lang/Object;)S"));
    gMethods[20] = (env->GetStaticMethodID(gEntryClass, "onHookInt",    "(Ljava/lang/Object;Ljava/lang/Object;[Ljava/lang/Object;)I"));
    gMethods[21] = (env->GetStaticMethodID(gEntryClass, "onHookVoid",   "(Ljava/lang/Object;Ljava/lang/Object;[Ljava/lang/Object;)V"));

}

jboolean dexposed_munprotect(JNIEnv* env, jclass, jlong addr, jlong len) {
    int pagesize = sysconf(_SC_PAGESIZE);
    int alignment = (addr%pagesize);

    int i = mprotect((void *)(addr-alignment), (size_t)(len+alignment), PROT_READ | PROT_WRITE | PROT_EXEC);
    if (i == -1) {
        LOGV("mprotect failed: %s (%d)", strerror(errno), errno);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

void dexposed_memcpy(JNIEnv* env, jclass, jlong src, jlong dest, jint length) {
    char *srcPnt = (char *) src;
    char *destPnt = (char *) dest;
    for (int i = 0; i < length; ++i) {
        destPnt[i] = srcPnt[i];
    }
}

void dexposed_memput(JNIEnv* env, jclass, jbyteArray src, jlong dest) {

    jbyte *srcPnt = env->GetByteArrayElements(src, 0);
    jsize length = env->GetArrayLength(src);
    unsigned char* destPnt = (unsigned char*)dest;
    for(int i = 0; i < length; ++i) {
        destPnt[i] = srcPnt[i];
    }
    env->ReleaseByteArrayElements(src, srcPnt, 0);
}

jbyteArray dexposed_memget(JNIEnv* env, jclass, jlong src, jint length) {

    jbyteArray dest = env->NewByteArray(length);
    if (dest == NULL) {
        return NULL;
    }
    unsigned char *destPnt = (unsigned char*)env->GetByteArrayElements(dest, 0);
    unsigned char *srcPnt = (unsigned char*)src;
    for(int i = 0; i < length; ++i) {
        destPnt[i] = srcPnt[i];
    }
    env->ReleaseByteArrayElements(dest, (jbyte*)destPnt, 0);

    return dest;
}

jlong dexposed_mmap(JNIEnv* env, jclass, jint length) {
    void *space = mmap(0, length, PROT_READ | PROT_WRITE | PROT_EXEC,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (space == MAP_FAILED) {
        LOGV("mmap failed: %d", errno);
        return 0;
    }
    return (jlong) space;
}

void dexposed_munmap(JNIEnv* env, jclass, jlong addr, jint length) {
    int r = munmap((void *) addr, length);
    if (r == -1) {
        LOGV("munmap failed: %d", errno);
    }
}

void dexposed_ptrace(JNIEnv* env, jclass, jint pid) {
    ptrace(PTRACE_ATTACH, (pid_t) pid, 0, 0);
}

jlong dexposed_getMethodAddress(JNIEnv * env, jclass clazz, jobject method) {
    jlong art_method =  (jlong) env->FromReflectedMethod(method);

    LOGV("method jobject 0x%lx", method);
    int* methodstruct = (int*)art_method;
     for(int i=0; i<20; ++i){
         LOGV("method struct 0x%lx sp[%d]: 0x%lx", (int)(methodstruct+i), i, *(methodstruct+i));
     }

    return art_method;
}

void storeArgsFromSP(uint64_t* xargs, int start, int end, int* args, jint* elements){

    for(int i=start; i<end; ++i){
        int token = elements[i];
        switch(token) {
            case 0: {
                xargs[i] = *((jint*) args);
                args = args + 1;
                break;}
            case 1: {
                xargs[i] = *((jdouble*) args);
                args = args + 2;
                break;}
            case 2: {
                xargs[i] = *((jlong*) args);
                args = args + 2;
                break;}
            case 3: {
                xargs[i] = *((jbyte*) args);
                args = args + 1;
                break;}
            case 4: {
                xargs[i] = *((jchar*) args);
                args = args + 1;
                break;}
            case 5: {
                xargs[i] = *((jboolean*) args);
                args = args + 1;
                break;}
            case 6: {
                xargs[i] = *((jfloat*) args);
                args = args + 1;
                break;}
            case 7: {
                xargs[i] = *((jshort*) args);
                args = args + 1;
                break;}
            case 8: {
                xargs[i] = *((jint*) args);
                args = args + 1;
                break;}
        }
    }
}

#ifdef ARME64_V8A
extern "C" uint64_t  artQuickDexposedInvokeHandler(void* art_method, void* arg2, void* arg3, void* arg4, void* arg5,
                                              void* arg6, void* arg7, void* arg8, int* args) {
   // pthread_mutex_lock(&lock_mutex);
    LOGV("artQuickDexposedInvokeHandler aarch64");
    LOGV("artQuickDexposedInvokeHandler method: 0x%lx",art_method);
    LOGV("artQuickDexposedInvokeHandler arg2: 0x%lx",arg2);
    LOGV("artQuickDexposedInvokeHandler arg3: 0x%lx",arg3);
    LOGV("artQuickDexposedInvokeHandler arg4: 0x%lx",arg4);
    LOGV("artQuickDexposedInvokeHandler arg5: 0x%lx",arg5);
    LOGV("artQuickDexposedInvokeHandler arg6: 0x%lx",arg6);
    LOGV("artQuickDexposedInvokeHandler arg7: 0x%lx",arg7);
    LOGV("artQuickDexposedInvokeHandler arg8: 0x%lx",arg8);

    for(int i=0; i<20; ++i){
        LOGV("artQuickDexposedInvokeHandler 0x%lx sp[%d]: 0x%lx", (args+i), i, *(args+i));
    }

    JNIEnv* env = NULL;
    if (gJVM->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    jobject method = env->ToReflectedMethod(methodClass, (jmethodID) art_method, 0);
    jintArray params = (jintArray) env->CallStaticObjectMethod(gEntryClass, gMethods[10], method);
    jsize size = env->GetArrayLength(params);

    LOGV("artQuickDexposedInvokeHandler size:%d", size);

    jint* elements = env->GetIntArrayElements(params, 0);

    uint64_t* xargs = (uint64_t*)malloc(size*sizeof(uint64_t));
    jboolean isStatic = (jboolean) env->CallStaticBooleanMethod(gEntryClass, gMethods[9], method);
    if(isStatic){
        LOGV("artQuickDexposedInvokeHandler isStatic：%d", isStatic);
        xargs[0] = (uint64_t)arg2;
        xargs[1] = (uint64_t)arg3;
        xargs[2] = (uint64_t)arg4;
        xargs[3] = (uint64_t)arg5;
        xargs[4] = (uint64_t)arg6;
        xargs[5] = (uint64_t)arg7;
        xargs[6] = (uint64_t)arg8;
        storeArgsFromSP(xargs, 7, size, args+8, elements);
    } else {
        xargs[0] = (uint64_t)arg3;
        xargs[1] = (uint64_t)arg4;
        xargs[2] = (uint64_t)arg5;
        xargs[3] = (uint64_t)arg6;
        xargs[4] = (uint64_t)arg7;
        xargs[5] = (uint64_t)arg8;
        storeArgsFromSP(xargs, 6, size, args+8, elements);
    }

    for(int i=0; i<20; ++i){
        LOGV("artQuickDexposedInvokeHandler xargs[%d]: 0x%lx", i, *(xargs+i));
    }

    jobjectArray argbox = (jobjectArray)env->NewObjectArray(size, gObjectClass, NULL);

    void* self = (void*)(*((uint64_t*)(args - 54)));
    LOGV("artQuickDexposedInvokeHandler self: 0x%lx", self);

    int index = 0;
    for (; index < size; index++) {
        jint offset = 1;
        int token = elements[index];
        LOGV("token [%d] %d", index, token);
        switch(token) {
            case 0: {
                //jobject arg = (jobject) env->NewWeakGlobalRef(xargs[index]);
                jobject arg = (jobject) addWeakGloablReference(gJVM, self, (void*)xargs[index]);
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 1: {
                jdouble arg = (jdouble) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 2: {
                jlong arg = (jlong) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 3: {
                jbyte arg = (jbyte) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 4: {
                jchar arg = (jchar) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 5: {
                jboolean arg = (jboolean) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 6: {
                jfloat arg = (jfloat) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 7: {
                jshort arg = (jshort) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 8: {
                jint arg = (jint) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
        }
    }

    free(xargs);

    jint returnType = (int)env->CallStaticIntMethod(gEntryClass, gMethods[11], method);
    LOGV("artQuickDexposedInvokeHandler returnType：%d", returnType);

    jobject receiver = 0;
    if(!isStatic){
        receiver = (jobject) addWeakGloablReference(gJVM, self, (uint64_t*) arg2);
        LOGV("artQuickDexposedInvokeHandler callhook entry：%lx", arg2);
    }

    switch(returnType - 12){
        case 0:{
            jobject result = (jobject)env->CallStaticObjectMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:0 result：0x%lx", result);
            return (uint64_t)result;
        }
        case 1:{
            jdouble result = (jdouble)env->CallStaticDoubleMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:1 result：%f", result);
            return (uint64_t)result;
        }
        case 2:{
            jlong result = (jlong)env->CallStaticLongMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:2 result：0x%lx", result);
            return (uint64_t)result;
        }
        case 3:{
            jbyte result = (jbyte)env->CallStaticByteMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:3 result：0x%x", result);
            return (uint64_t)result;
        }
        case 4:{
            jchar result = (jchar)env->CallStaticCharMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:4 result：0x%x", result);
            return (uint64_t)result;
        }
        case 5:{
            jboolean result = (jboolean)env->CallStaticBooleanMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:5 result：0x%x", result);
            return (uint64_t)result;
        }
        case 6:{
            jfloat result = (jfloat)env->CallStaticFloatMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:6 result：%f", result);
            return (uint64_t)result;
        }
        case 7:{
            jshort result = (jshort)env->CallStaticShortMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:7 result：0x%x", result);
            return (uint64_t)result;
        }
        case 8:{
            jint result = (jint)env->CallStaticIntMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:8 result：0x%x", result);
            return (uint64_t)result;
        }
        case 9:{
            env->CallStaticVoidMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:9 result：void");
            return 0;
        }
    }

}
#else
extern "C" uint64_t  artQuickDexposedInvokeHandler(void* art_method, void* arg2, void* arg3, void* sp) {



    LOGV("artQuickDexposedInvokeHandler arm32");
    LOGV("artQuickDexposedInvokeHandler method: 0x%lx",art_method);
    LOGV("artQuickDexposedInvokeHandler arg2: 0x%lx",arg2);
    LOGV("artQuickDexposedInvokeHandler arg3: 0x%lx",arg3);
    LOGV("artQuickDexposedInvokeHandler sp1: 0x%lx",sp);

    int* args = (int*)sp;
    int* self = (int*)(*(args+1));
    void* arg4 = (void*)(*(args+2));

    for(int i=0; i<40; ++i){
        LOGV("artQuickDexposedInvokeHandler1 0x%lx sp[%d]: 0x%lx", (int)(args+i), i, *(args+i));
    }

    JNIEnv* env = NULL;
    if (gJVM->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    jobject method = env->ToReflectedMethod(methodClass, (jmethodID) art_method, 0);
    jintArray params = (jintArray) env->CallStaticObjectMethod(gEntryClass, gMethods[10], method);
    jsize size = env->GetArrayLength(params);

    LOGV("artQuickDexposedInvokeHandler size:%d", size);
    jint* elements = env->GetIntArrayElements(params, 0);

    args = args + 10 + 4;

    uint64_t* xargs = (uint64_t*)malloc(size*sizeof(uint64_t));
    jboolean isStatic = (jboolean) env->CallStaticBooleanMethod(gEntryClass, gMethods[9], method);
    if(isStatic){
        LOGV("artQuickDexposedInvokeHandler isStatic：%d", isStatic);
        xargs[0] = (uint64_t)arg2;
        xargs[1] = (uint64_t)arg3;
        xargs[2] = (uint64_t)arg4;
        storeArgsFromSP(xargs, 3, size, args+4, elements);
    } else {
        xargs[0] = (uint64_t)arg3;
        xargs[1] = (uint64_t)arg4;
        storeArgsFromSP(xargs, 2, size, args+4, elements);
    }

    for(int i=0; i<20; ++i){
        LOGV("artQuickDexposedInvokeHandler xargs[%d]: 0x%lx", i, *(xargs+i));
    }

    jobjectArray argbox = (jobjectArray)env->NewObjectArray(size, gObjectClass, NULL);

    int index = 0;
    for (; index < size; index++) {
        jint offset = 1;
        int token = elements[index];
        LOGV("token [%d] %d", index, token);
        switch(token) {
            case 0: {
                jobject arg = (jobject) addWeakGloablReference(gJVM, self, (void*)xargs[index]);
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 1: {
                jdouble arg = (jdouble) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 2: {
                jlong arg = (jlong) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 3: {
                jbyte arg = (jbyte) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 4: {
                jchar arg = (jchar) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 5: {
                jboolean arg = (jboolean) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 6: {
                jfloat arg = (jfloat) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 7: {
                jshort arg = (jshort) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
            case 8: {
                jint arg = (jint) xargs[index];
                offset = env->CallStaticIntMethod(gEntryClass, gMethods[token], argbox, index, arg);
                break;}
        }
    }

    free(xargs);

    jint returnType = (int)env->CallStaticIntMethod(gEntryClass, gMethods[11], method);
    LOGV("artQuickDexposedInvokeHandler returnType：%d", returnType);

    jobject receiver = 0;
    if(!isStatic){
        receiver = (jobject) addWeakGloablReference(gJVM, self, (uint64_t*) arg2);
        LOGV("artQuickDexposedInvokeHandler callhook entry：%lx", arg2);
    }

    switch(returnType - 12){
        case 0:{
            jobject result = (jobject)env->CallStaticObjectMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:0 result：0x%lx", result);
            return (uint64_t)result;
        }
        case 1:{
            jdouble result = (jdouble)env->CallStaticDoubleMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:1 result：%f", result);
            return (uint64_t)result;
        }
        case 2:{
            jlong result = (jlong)env->CallStaticLongMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:2 result：0x%lx", result);
            return (uint64_t)result;
        }
        case 3:{
            jbyte result = (jbyte)env->CallStaticByteMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:3 result：0x%x", result);
            return (uint64_t)result;
        }
        case 4:{
            jchar result = (jchar)env->CallStaticCharMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:4 result：0x%x", result);
            return (uint64_t)result;
        }
        case 5:{
            jboolean result = (jboolean)env->CallStaticBooleanMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:5 result：0x%x", result);
            return (uint64_t)result;
        }
        case 6:{
            jfloat result = (jfloat)env->CallStaticFloatMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:6 result：%f", result);
            return (uint64_t)result;
        }
        case 7:{
            jshort result = (jshort)env->CallStaticShortMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:7 result：0x%x", result);
            return (uint64_t)result;
        }
        case 8:{
            jint result = (jint)env->CallStaticIntMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:8 result：0x%x", result);
            return (uint64_t)result;
        }
        case 9:{
            env->CallStaticVoidMethod(gEntryClass, gMethods[returnType], method, receiver, argbox);
            LOGV("artQuickDexposedInvokeHandler type:9 result：void");
            return 0;
        }
    }

}
#endif

jlong dexposed_getBridgeFunction(JNIEnv * env, jclass clazz) {

    jlong temp = (jlong)artQuickDexposedInvokeHandler;
    LOGV("dexposed_getBridgeFunction %lx", (int)temp);

    return temp;
}

static JNINativeMethod dexposedMethods[] = {

        {"mmap", "(I)J", (void *) dexposed_mmap},
        {"munmap", "(JI)Z", (void *) dexposed_munmap},
        {"memcpy", "(JJI)V", (void *) dexposed_memcpy},
        {"memput", "([BJ)V", (void *) dexposed_memput},
        {"memget", "(JI)[B", (void *) dexposed_memget},
        {"munprotect", "(JJ)Z", (void *) dexposed_munprotect},
        {"getMethodAddress", "(Ljava/lang/reflect/Method;)J", (void *) dexposed_getMethodAddress},
        {"getBridgeFunction", "()J", (void *) dexposed_getBridgeFunction},
};

static int registerNativeMethods(JNIEnv *env, const char *className,
                                 JNINativeMethod *gMethods, int numMethods) {

    jclass clazz = env->FindClass(className);
    if (clazz == NULL) {
        return JNI_FALSE;
    }

    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {

    JNIEnv *env = NULL;
    gJVM = vm;

    LOGV("JNI_Dexposed_Art_OnLoad");

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    gEnv = env;
    if (!registerNativeMethods(env, JNIHOOK_CLASS, dexposedMethods,
                               sizeof(dexposedMethods) / sizeof(dexposedMethods[0]))) {
        return -1;
    }

    init_entries(env);
    return JNI_VERSION_1_6;
}