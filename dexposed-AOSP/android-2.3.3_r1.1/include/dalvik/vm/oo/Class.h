/*
 * Copyright (C) 2008 The Android Open Source Project
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
/*
 * Class loader.
 */
#ifndef DALVIK_OO_CLASS_H_
#define DALVIK_OO_CLASS_H_

/*
 * The classpath and bootclasspath differ in that only the latter is
 * consulted when looking for classes needed by the VM.  When searching
 * for an arbitrary class definition, we start with the bootclasspath,
 * look for optional packages (a/k/a standard extensions), and then try
 * the classpath.
 *
 * In Dalvik, a class can be found in one of two ways:
 *  - in a .dex file
 *  - in a .dex file named specifically "classes.dex", which is held
 *    inside a jar file
 *
 * These two may be freely intermixed in a classpath specification.
 * Ordering is significant.
 */
enum ClassPathEntryKind {
    kCpeUnknown = 0,
    kCpeJar,
    kCpeDex,
    kCpeLastEntry       /* used as sentinel at end of array */
};

struct ClassPathEntry {
    ClassPathEntryKind kind;
    char*   fileName;
    void*   ptr;            /* JarFile* or DexFile* */
};

bool dvmClassStartup(void);
void dvmClassShutdown(void);
bool dvmPrepBootClassPath(bool isNormalStart);

/*
 * Boot class path accessors, for class loader getResources().
 */
int dvmGetBootPathSize(void);
StringObject* dvmGetBootPathResource(const char* name, int idx);
void dvmDumpBootClassPath(void);

/*
 * Determine whether "path" is a member of "cpe".
 */
bool dvmClassPathContains(const ClassPathEntry* cpe, const char* path);

/*
 * Set clazz->serialNumber to the next available value.
 */
void dvmSetClassSerialNumber(ClassObject* clazz);

/*
 * Find the class object representing the primitive type with the
 * given descriptor. This returns NULL if the given type character
 * is invalid.
 */
ClassObject* dvmFindPrimitiveClass(char type);

/*
 * Find the class with the given descriptor.  Load it if it hasn't already
 * been.
 *
 * "loader" is the initiating class loader.
 */
ClassObject* dvmFindClass(const char* descriptor, Object* loader);
ClassObject* dvmFindClassNoInit(const char* descriptor, Object* loader);

/*
 * Like dvmFindClass, but only for system classes.
 */
ClassObject* dvmFindSystemClass(const char* descriptor);
ClassObject* dvmFindSystemClassNoInit(const char* descriptor);

/*
 * Find a loaded class by descriptor. Returns the first one found.
 * Because there can be more than one if class loaders are involved,
 * this is not an especially good API. (Currently only used by the
 * debugger and "checking" JNI.)
 *
 * "descriptor" should have the form "Ljava/lang/Class;" or
 * "[Ljava/lang/Class;", i.e. a descriptor and not an internal-form
 * class name.
 */
ClassObject* dvmFindLoadedClass(const char* descriptor);

/*
 * Load the named class (by descriptor) from the specified DEX file.
 * Used by class loaders to instantiate a class object from a
 * VM-managed DEX.
 */
ClassObject* dvmDefineClass(DvmDex* pDvmDex, const char* descriptor,
    Object* classLoader);

/*
 * Link a loaded class.  Normally done as part of one of the "find class"
 * variations, this is only called explicitly for synthetic class
 * generation (e.g. reflect.Proxy).
 */
bool dvmLinkClass(ClassObject* clazz);

/*
 * Determine if a class has been initialized.
 */
INLINE bool dvmIsClassInitialized(const ClassObject* clazz) {
    return (clazz->status == CLASS_INITIALIZED);
}
bool dvmIsClassInitializing(const ClassObject* clazz);

/*
 * Initialize a class.
 */
extern "C" bool dvmInitClass(ClassObject* clazz);

/*
 * Retrieve the system class loader.
 */
Object* dvmGetSystemClassLoader(void);

/*
 * Utility functions.
 */
ClassObject* dvmLookupClass(const char* descriptor, Object* loader,
    bool unprepOkay);
void dvmFreeClassInnards(ClassObject* clazz);
bool dvmAddClassToHash(ClassObject* clazz);
void dvmAddInitiatingLoader(ClassObject* clazz, Object* loader);
bool dvmLoaderInInitiatingList(const ClassObject* clazz, const Object* loader);

/*
 * Update method's "nativeFunc" and "insns".  If "insns" is NULL, the
 * current method->insns value is not changed.
 */
void dvmSetNativeFunc(Method* method, DalvikBridgeFunc func, const u2* insns);

/*
 * Set the method's "registerMap" field.
 */
void dvmSetRegisterMap(Method* method, const RegisterMap* pMap);

/*
 * Make a method's DexCode (which includes the bytecode) read-write or
 * read-only.  The conversion to read-write may involve making a new copy
 * of the DexCode, and in normal operation the read-only state is not
 * actually enforced.
 */
void dvmMakeCodeReadWrite(Method* meth);
void dvmMakeCodeReadOnly(Method* meth);

/*
 * During DEX optimizing, add an extra DEX to the bootstrap class path.
 */
void dvmSetBootPathExtraDex(DvmDex* pDvmDex);

/*
 * Debugging.
 */
void dvmDumpClass(const ClassObject* clazz, int flags);
void dvmDumpAllClasses(int flags);
void dvmDumpLoaderStats(const char* msg);
int  dvmGetNumLoadedClasses();

/* flags for dvmDumpClass / dvmDumpAllClasses */
#define kDumpClassFullDetail    1
#define kDumpClassClassLoader   (1 << 1)
#define kDumpClassInitialized   (1 << 2)


/*
 * Store a copy of the method prototype descriptor string
 * for the given method into the given DexStringCache, returning the
 * stored string for convenience.
 */
INLINE char* dvmCopyDescriptorStringFromMethod(const Method* method,
        DexStringCache *pCache)
{
    const char* result =
        dexProtoGetMethodDescriptor(&method->prototype, pCache);
    return dexStringCacheEnsureCopy(pCache, result);
}

/*
 * Compute the number of argument words (u4 units) required by the
 * given method's prototype. For example, if the method descriptor is
 * "(IJ)D", this would return 3 (one for the int, two for the long;
 * return value isn't relevant).
 */
INLINE int dvmComputeMethodArgsSize(const Method* method)
{
    return dexProtoComputeArgsSize(&method->prototype);
}

/*
 * Compare the two method prototypes. The two prototypes are compared
 * as if by strcmp() on the result of dexProtoGetMethodDescriptor().
 */
INLINE int dvmCompareMethodProtos(const Method* method1,
        const Method* method2)
{
    return dexProtoCompare(&method1->prototype, &method2->prototype);
}

/*
 * Compare the two method prototypes, considering only the parameters
 * (i.e. ignoring the return types). The two prototypes are compared
 * as if by strcmp() on the result of dexProtoGetMethodDescriptor().
 */
INLINE int dvmCompareMethodParameterProtos(const Method* method1,
        const Method* method2)
{
    return dexProtoCompareParameters(&method1->prototype, &method2->prototype);
}

/*
 * Compare the two method names and prototypes, a la strcmp(). The
 * name is considered the "major" order and the prototype the "minor"
 * order. The prototypes are compared as if by dexProtoGetMethodDescriptor().
 */
int dvmCompareMethodNamesAndProtos(const Method* method1,
        const Method* method2);

/*
 * Compare the two method names and prototypes, a la strcmp(), ignoring
 * the return type. The name is considered the "major" order and the
 * prototype the "minor" order. The prototypes are compared as if by
 * dexProtoGetMethodDescriptor().
 */
int dvmCompareMethodNamesAndParameterProtos(const Method* method1,
        const Method* method2);

/*
 * Compare a method descriptor string with the prototype of a method,
 * as if by converting the descriptor to a DexProto and comparing it
 * with dexProtoCompare().
 */
INLINE int dvmCompareDescriptorAndMethodProto(const char* descriptor,
    const Method* method)
{
    // Sense is reversed.
    return -dexProtoCompareToDescriptor(&method->prototype, descriptor);
}

/*
 * Compare a (name, prototype) pair with the (name, prototype) of
 * a method, a la strcmp(). The name is considered the "major" order and
 * the prototype the "minor" order. The descriptor and prototype are
 * compared as if by dvmCompareDescriptorAndMethodProto().
 */
int dvmCompareNameProtoAndMethod(const char* name,
    const DexProto* proto, const Method* method);

/*
 * Compare a (name, method descriptor) pair with the (name, prototype) of
 * a method, a la strcmp(). The name is considered the "major" order and
 * the prototype the "minor" order. The descriptor and prototype are
 * compared as if by dvmCompareDescriptorAndMethodProto().
 */
int dvmCompareNameDescriptorAndMethod(const char* name,
    const char* descriptor, const Method* method);

/*
 * Returns the size of the given class object in bytes.
 */
size_t dvmClassObjectSize(const ClassObject *clazz);

#endif  // DALVIK_OO_CLASS_H_
