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
 * Declaration of the fundamental Object type and refinements thereof, plus
 * some functions for manipulating them.
 */
#ifndef DALVIK_OO_OBJECT_H_
#define DALVIK_OO_OBJECT_H_

#include <stddef.h>
#include "Atomic.h"

/* fwd decl */
struct DataObject;
struct InitiatingLoaderList;
struct ClassObject;
struct StringObject;
struct ArrayObject;
struct Method;
struct ExceptionEntry;
struct LineNumEntry;
struct StaticField;
struct InstField;
struct Field;
struct RegisterMap;

/*
 * Native function pointer type.
 *
 * "args[0]" holds the "this" pointer for virtual methods.
 *
 * The "Bridge" form is a super-set of the "Native" form; in many places
 * they are used interchangeably.  Currently, all functions have all
 * arguments passed in, but some functions only care about the first two.
 * Passing extra arguments to a C function is (mostly) harmless.
 */
typedef void (*DalvikBridgeFunc)(const u4* args, JValue* pResult,
    const Method* method, struct Thread* self);
typedef void (*DalvikNativeFunc)(const u4* args, JValue* pResult);


/* vm-internal access flags and related definitions */
enum AccessFlags {
    ACC_MIRANDA         = 0x8000,       // method (internal to VM)
    JAVA_FLAGS_MASK     = 0xffff,       // bits set from Java sources (low 16)
};

/* Use the top 16 bits of the access flags field for
 * other class flags.  Code should use the *CLASS_FLAG*()
 * macros to set/get these flags.
 */
enum ClassFlags {
    CLASS_ISFINALIZABLE        = (1<<31), // class/ancestor overrides finalize()
    CLASS_ISARRAY              = (1<<30), // class is a "[*"
    CLASS_ISOBJECTARRAY        = (1<<29), // class is a "[L*" or "[[*"
    CLASS_ISCLASS              = (1<<28), // class is *the* class Class

    CLASS_ISREFERENCE          = (1<<27), // class is a soft/weak/phantom ref
                                          // only ISREFERENCE is set --> soft
    CLASS_ISWEAKREFERENCE      = (1<<26), // class is a weak reference
    CLASS_ISFINALIZERREFERENCE = (1<<25), // class is a finalizer reference
    CLASS_ISPHANTOMREFERENCE   = (1<<24), // class is a phantom reference

    CLASS_MULTIPLE_DEFS        = (1<<23), // DEX verifier: defs in multiple DEXs

    /* unlike the others, these can be present in the optimized DEX file */
    CLASS_ISOPTIMIZED          = (1<<17), // class may contain opt instrs
    CLASS_ISPREVERIFIED        = (1<<16), // class has been pre-verified
};

/* bits we can reasonably expect to see set in a DEX access flags field */
#define EXPECTED_FILE_FLAGS \
    (ACC_CLASS_MASK | CLASS_ISPREVERIFIED | CLASS_ISOPTIMIZED)

/*
 * Get/set class flags.
 */
#define SET_CLASS_FLAG(clazz, flag) \
    do { (clazz)->accessFlags |= (flag); } while (0)

#define CLEAR_CLASS_FLAG(clazz, flag) \
    do { (clazz)->accessFlags &= ~(flag); } while (0)

#define IS_CLASS_FLAG_SET(clazz, flag) \
    (((clazz)->accessFlags & (flag)) != 0)

#define GET_CLASS_FLAG_GROUP(clazz, flags) \
    ((u4)((clazz)->accessFlags & (flags)))

/*
 * Use the top 16 bits of the access flags field for other method flags.
 * Code should use the *METHOD_FLAG*() macros to set/get these flags.
 */
enum MethodFlags {
    METHOD_ISWRITABLE       = (1<<31),  // the method's code is writable
};

/*
 * Get/set method flags.
 */
#define SET_METHOD_FLAG(method, flag) \
    do { (method)->accessFlags |= (flag); } while (0)

#define CLEAR_METHOD_FLAG(method, flag) \
    do { (method)->accessFlags &= ~(flag); } while (0)

#define IS_METHOD_FLAG_SET(method, flag) \
    (((method)->accessFlags & (flag)) != 0)

#define GET_METHOD_FLAG_GROUP(method, flags) \
    ((u4)((method)->accessFlags & (flags)))

/* current state of the class, increasing as we progress */
enum ClassStatus {
    CLASS_ERROR         = -1,

    CLASS_NOTREADY      = 0,
    CLASS_IDX           = 1,    /* loaded, DEX idx in super or ifaces */
    CLASS_LOADED        = 2,    /* DEX idx values resolved */
    CLASS_RESOLVED      = 3,    /* part of linking */
    CLASS_VERIFYING     = 4,    /* in the process of being verified */
    CLASS_VERIFIED      = 5,    /* logically part of linking; done pre-init */
    CLASS_INITIALIZING  = 6,    /* class init in progress */
    CLASS_INITIALIZED   = 7,    /* ready to go */
};

/*
 * Definitions for packing refOffsets in ClassObject.
 */
/*
 * A magic value for refOffsets. Ignore the bits and walk the super
 * chain when this is the value.
 * [This is an unlikely "natural" value, since it would be 30 non-ref instance
 * fields followed by 2 ref instance fields.]
 */
#define CLASS_WALK_SUPER ((unsigned int)(3))
#define CLASS_SMALLEST_OFFSET (sizeof(struct Object))
#define CLASS_BITS_PER_WORD (sizeof(unsigned long int) * 8)
#define CLASS_OFFSET_ALIGNMENT 4
#define CLASS_HIGH_BIT ((unsigned int)1 << (CLASS_BITS_PER_WORD - 1))
/*
 * Given an offset, return the bit number which would encode that offset.
 * Local use only.
 */
#define _CLASS_BIT_NUMBER_FROM_OFFSET(byteOffset) \
    (((unsigned int)(byteOffset) - CLASS_SMALLEST_OFFSET) / \
     CLASS_OFFSET_ALIGNMENT)
/*
 * Is the given offset too large to be encoded?
 */
#define CLASS_CAN_ENCODE_OFFSET(byteOffset) \
    (_CLASS_BIT_NUMBER_FROM_OFFSET(byteOffset) < CLASS_BITS_PER_WORD)
/*
 * Return a single bit, encoding the offset.
 * Undefined if the offset is too large, as defined above.
 */
#define CLASS_BIT_FROM_OFFSET(byteOffset) \
    (CLASS_HIGH_BIT >> _CLASS_BIT_NUMBER_FROM_OFFSET(byteOffset))
/*
 * Return an offset, given a bit number as returned from CLZ.
 */
#define CLASS_OFFSET_FROM_CLZ(rshift) \
    (((int)(rshift) * CLASS_OFFSET_ALIGNMENT) + CLASS_SMALLEST_OFFSET)


/*
 * Used for iftable in ClassObject.
 */
struct InterfaceEntry {
    /* pointer to interface class */
    ClassObject*    clazz;

    /*
     * Index into array of vtable offsets.  This points into the ifviPool,
     * which holds the vtables for all interfaces declared by this class.
     */
    int*            methodIndexArray;
};



/*
 * There are three types of objects:
 *  Class objects - an instance of java.lang.Class
 *  Array objects - an object created with a "new array" instruction
 *  Data objects - an object that is neither of the above
 *
 * We also define String objects.  At present they're equivalent to
 * DataObject, but that may change.  (Either way, they make some of the
 * code more obvious.)
 *
 * All objects have an Object header followed by type-specific data.
 */
struct Object {
    /* ptr to class object */
    ClassObject*    clazz;

    /*
     * A word containing either a "thin" lock or a "fat" monitor.  See
     * the comments in Sync.c for a description of its layout.
     */
    u4              lock;
};

/*
 * Properly initialize an Object.
 * void DVM_OBJECT_INIT(Object *obj, ClassObject *clazz_)
 */
#define DVM_OBJECT_INIT(obj, clazz_) \
    dvmSetFieldObject(obj, OFFSETOF_MEMBER(Object, clazz), clazz_)

/*
 * Data objects have an Object header followed by their instance data.
 */
struct DataObject : Object {
    /* variable #of u4 slots; u8 uses 2 slots */
    u4              instanceData[1];
};

/*
 * Strings are used frequently enough that we may want to give them their
 * own unique type.
 *
 * Using a dedicated type object to access the instance data provides a
 * performance advantage but makes the java/lang/String.java implementation
 * fragile.
 *
 * Currently this is just equal to DataObject, and we pull the fields out
 * like we do for any other object.
 */
struct StringObject : Object {
    /* variable #of u4 slots; u8 uses 2 slots */
    u4              instanceData[1];

    /** Returns this string's length in characters. */
    int length() const;

    /**
     * Returns this string's length in bytes when encoded as modified UTF-8.
     * Does not include a terminating NUL byte.
     */
    int utfLength() const;

    /** Returns this string's char[] as an ArrayObject. */
    ArrayObject* array() const;

    /** Returns this string's char[] as a u2*. */
    const u2* chars() const;
};


/*
 * Array objects have these additional fields.
 *
 * We don't currently store the size of each element.  Usually it's implied
 * by the instruction.  If necessary, the width can be derived from
 * the first char of obj->clazz->descriptor.
 */
struct ArrayObject : Object {
    /* number of elements; immutable after init */
    u4              length;

    /*
     * Array contents; actual size is (length * sizeof(type)).  This is
     * declared as u8 so that the compiler inserts any necessary padding
     * (e.g. for EABI); the actual allocation may be smaller than 8 bytes.
     */
    u8              contents[1];
};

/*
 * For classes created early and thus probably in the zygote, the
 * InitiatingLoaderList is kept in gDvm. Later classes use the structure in
 * Object Class. This helps keep zygote pages shared.
 */
struct InitiatingLoaderList {
    /* a list of initiating loader Objects; grown and initialized on demand */
    Object**  initiatingLoaders;
    /* count of loaders in the above list */
    int       initiatingLoaderCount;
};

/*
 * Generic field header.  We pass this around when we want a generic Field
 * pointer (e.g. for reflection stuff).  Testing the accessFlags for
 * ACC_STATIC allows a proper up-cast.
 */
struct Field {
    ClassObject*    clazz;          /* class in which the field is declared */
    const char*     name;
    const char*     signature;      /* e.g. "I", "[C", "Landroid/os/Debug;" */
    u4              accessFlags;
};

/*
 * Static field.
 */
struct StaticField : Field {
    JValue          value;          /* initially set from DEX for primitives */
};

/*
 * Instance field.
 */
struct InstField : Field {
    /*
     * This field indicates the byte offset from the beginning of the
     * (Object *) to the actual instance data; e.g., byteOffset==0 is
     * the same as the object pointer (bug!), and byteOffset==4 is 4
     * bytes farther.
     */
    int             byteOffset;
};

/*
 * This defines the amount of space we leave for field slots in the
 * java.lang.Class definition.  If we alter the class to have more than
 * this many fields, the VM will abort at startup.
 */
#define CLASS_FIELD_SLOTS   4

/*
 * Class objects have many additional fields.  This is used for both
 * classes and interfaces, including synthesized classes (arrays and
 * primitive types).
 *
 * Class objects are unusual in that they have some fields allocated with
 * the system malloc (or LinearAlloc), rather than on the GC heap.  This is
 * handy during initialization, but does require special handling when
 * discarding java.lang.Class objects.
 *
 * The separation of methods (direct vs. virtual) and fields (class vs.
 * instance) used in Dalvik works out pretty well.  The only time it's
 * annoying is when enumerating or searching for things with reflection.
 */
struct ClassObject : Object {
    /* leave space for instance data; we could access fields directly if we
       freeze the definition of java/lang/Class */
    u4              instanceData[CLASS_FIELD_SLOTS];

    /* UTF-8 descriptor for the class; from constant pool, or on heap
       if generated ("[C") */
    const char*     descriptor;
    char*           descriptorAlloc;

    /* access flags; low 16 bits are defined by VM spec */
    u4              accessFlags;

    /* VM-unique class serial number, nonzero, set very early */
    u4              serialNumber;

    /* DexFile from which we came; needed to resolve constant pool entries */
    /* (will be NULL for VM-generated, e.g. arrays and primitive classes) */
    DvmDex*         pDvmDex;

    /* state of class initialization */
    ClassStatus     status;

    /* if class verify fails, we must return same error on subsequent tries */
    ClassObject*    verifyErrorClass;

    /* threadId, used to check for recursive <clinit> invocation */
    u4              initThreadId;

    /*
     * Total object size; used when allocating storage on gc heap.  (For
     * interfaces and abstract classes this will be zero.)
     */
    size_t          objectSize;

    /* arrays only: class object for base element, for instanceof/checkcast
       (for String[][][], this will be String) */
    ClassObject*    elementClass;

    /* arrays only: number of dimensions, e.g. int[][] is 2 */
    int             arrayDim;

    /* primitive type index, or PRIM_NOT (-1); set for generated prim classes */
    PrimitiveType   primitiveType;

    /* superclass, or NULL if this is java.lang.Object */
    ClassObject*    super;

    /* defining class loader, or NULL for the "bootstrap" system loader */
    Object*         classLoader;

    /* initiating class loader list */
    /* NOTE: for classes with low serialNumber, these are unused, and the
       values are kept in a table in gDvm. */
    InitiatingLoaderList initiatingLoaderList;

    /* array of interfaces this class implements directly */
    int             interfaceCount;
    ClassObject**   interfaces;

    /* static, private, and <init> methods */
    int             directMethodCount;
    Method*         directMethods;

    /* virtual methods defined in this class; invoked through vtable */
    int             virtualMethodCount;
    Method*         virtualMethods;

    /*
     * Virtual method table (vtable), for use by "invoke-virtual".  The
     * vtable from the superclass is copied in, and virtual methods from
     * our class either replace those from the super or are appended.
     */
    int             vtableCount;
    Method**        vtable;

    /*
     * Interface table (iftable), one entry per interface supported by
     * this class.  That means one entry for each interface we support
     * directly, indirectly via superclass, or indirectly via
     * superinterface.  This will be null if neither we nor our superclass
     * implement any interfaces.
     *
     * Why we need this: given "class Foo implements Face", declare
     * "Face faceObj = new Foo()".  Invoke faceObj.blah(), where "blah" is
     * part of the Face interface.  We can't easily use a single vtable.
     *
     * For every interface a concrete class implements, we create a list of
     * virtualMethod indices for the methods in the interface.
     */
    int             iftableCount;
    InterfaceEntry* iftable;

    /*
     * The interface vtable indices for iftable get stored here.  By placing
     * them all in a single pool for each class that implements interfaces,
     * we decrease the number of allocations.
     */
    int             ifviPoolCount;
    int*            ifviPool;

    /* instance fields
     *
     * These describe the layout of the contents of a DataObject-compatible
     * Object.  Note that only the fields directly defined by this class
     * are listed in ifields;  fields defined by a superclass are listed
     * in the superclass's ClassObject.ifields.
     *
     * All instance fields that refer to objects are guaranteed to be
     * at the beginning of the field list.  ifieldRefCount specifies
     * the number of reference fields.
     */
    int             ifieldCount;
    int             ifieldRefCount; // number of fields that are object refs
    InstField*      ifields;

    /* bitmap of offsets of ifields */
    u4 refOffsets;

    /* source file name, if known */
    const char*     sourceFile;

    /* static fields */
    int             sfieldCount;
    StaticField     sfields[0]; /* MUST be last item */
};

/*
 * A method.  We create one of these for every method in every class
 * we load, so try to keep the size to a minimum.
 *
 * Much of this comes from and could be accessed in the data held in shared
 * memory.  We hold it all together here for speed.  Everything but the
 * pointers could be held in a shared table generated by the optimizer;
 * if we're willing to convert them to offsets and take the performance
 * hit (e.g. "meth->insns" becomes "baseAddr + meth->insnsOffset") we
 * could move everything but "nativeFunc".
 */
struct Method {
    /* the class we are a part of */
    ClassObject*    clazz;

    /* access flags; low 16 bits are defined by spec (could be u2?) */
    u4              accessFlags;

    /*
     * For concrete virtual methods, this is the offset of the method
     * in "vtable".
     *
     * For abstract methods in an interface class, this is the offset
     * of the method in "iftable[n]->methodIndexArray".
     */
    u2             methodIndex;

    /*
     * Method bounds; not needed for an abstract method.
     *
     * For a native method, we compute the size of the argument list, and
     * set "insSize" and "registerSize" equal to it.
     */
    u2              registersSize;  /* ins + locals */
    u2              outsSize;
    u2              insSize;

    /* method name, e.g. "<init>" or "eatLunch" */
    const char*     name;

    /*
     * Method prototype descriptor string (return and argument types).
     *
     * TODO: This currently must specify the DexFile as well as the proto_ids
     * index, because generated Proxy classes don't have a DexFile.  We can
     * remove the DexFile* and reduce the size of this struct if we generate
     * a DEX for proxies.
     */
    DexProto        prototype;

    /* short-form method descriptor string */
    const char*     shorty;

    /*
     * The remaining items are not used for abstract or native methods.
     * (JNI is currently hijacking "insns" as a function pointer, set
     * after the first call.  For internal-native this stays null.)
     */

    /* the actual code */
    const u2*       insns;          /* instructions, in memory-mapped .dex */

    /* JNI: cached argument and return-type hints */
    int             jniArgInfo;

    /*
     * JNI: native method ptr; could be actual function or a JNI bridge.  We
     * don't currently discriminate between DalvikBridgeFunc and
     * DalvikNativeFunc; the former takes an argument superset (i.e. two
     * extra args) which will be ignored.  If necessary we can use
     * insns==NULL to detect JNI bridge vs. internal native.
     */
    DalvikBridgeFunc nativeFunc;

    /*
     * JNI: true if this static non-synchronized native method (that has no
     * reference arguments) needs a JNIEnv* and jclass/jobject. Libcore
     * uses this.
     */
    bool fastJni;

    /*
     * JNI: true if this method has no reference arguments. This lets the JNI
     * bridge avoid scanning the shorty for direct pointers that need to be
     * converted to local references.
     *
     * TODO: replace this with a list of indexes of the reference arguments.
     */
    bool noRef;

    /*
     * JNI: true if we should log entry and exit. This is the only way
     * developers can log the local references that are passed into their code.
     * Used for debugging JNI problems in third-party code.
     */
    bool shouldTrace;

    /*
     * Register map data, if available.  This will point into the DEX file
     * if the data was computed during pre-verification, or into the
     * linear alloc area if not.
     */
    const RegisterMap* registerMap;

    /* set if method was called during method profiling */
    bool            inProfile;
};


/*
 * Find a method within a class.  The superclass is not searched.
 */
Method* dvmFindDirectMethodByDescriptor(const ClassObject* clazz,
    const char* methodName, const char* signature);
Method* dvmFindVirtualMethodByDescriptor(const ClassObject* clazz,
    const char* methodName, const char* signature);
Method* dvmFindVirtualMethodByName(const ClassObject* clazz,
    const char* methodName);
Method* dvmFindDirectMethod(const ClassObject* clazz, const char* methodName,
    const DexProto* proto);
Method* dvmFindVirtualMethod(const ClassObject* clazz, const char* methodName,
    const DexProto* proto);


/*
 * Find a method within a class hierarchy.
 */
Method* dvmFindDirectMethodHierByDescriptor(const ClassObject* clazz,
    const char* methodName, const char* descriptor);
Method* dvmFindVirtualMethodHierByDescriptor(const ClassObject* clazz,
    const char* methodName, const char* signature);
Method* dvmFindDirectMethodHier(const ClassObject* clazz,
    const char* methodName, const DexProto* proto);
Method* dvmFindVirtualMethodHier(const ClassObject* clazz,
    const char* methodName, const DexProto* proto);
Method* dvmFindMethodHier(const ClassObject* clazz, const char* methodName,
    const DexProto* proto);

/*
 * Find a method in an interface hierarchy.
 */
Method* dvmFindInterfaceMethodHierByDescriptor(const ClassObject* iface,
    const char* methodName, const char* descriptor);
Method* dvmFindInterfaceMethodHier(const ClassObject* iface,
    const char* methodName, const DexProto* proto);

/*
 * Find the implementation of "meth" in "clazz".
 *
 * Returns NULL and throws an exception if not found.
 */
const Method* dvmGetVirtualizedMethod(const ClassObject* clazz,
    const Method* meth);

/*
 * Get the source file associated with a method.
 */
extern "C" const char* dvmGetMethodSourceFile(const Method* meth);

/*
 * Find a field within a class.  The superclass is not searched.
 */
InstField* dvmFindInstanceField(const ClassObject* clazz,
    const char* fieldName, const char* signature);
StaticField* dvmFindStaticField(const ClassObject* clazz,
    const char* fieldName, const char* signature);

/*
 * Find a field in a class/interface hierarchy.
 */
InstField* dvmFindInstanceFieldHier(const ClassObject* clazz,
    const char* fieldName, const char* signature);
StaticField* dvmFindStaticFieldHier(const ClassObject* clazz,
    const char* fieldName, const char* signature);
Field* dvmFindFieldHier(const ClassObject* clazz, const char* fieldName,
    const char* signature);

/*
 * Find a field and return the byte offset from the object pointer.  Only
 * searches the specified class, not the superclass.
 *
 * Returns -1 on failure.
 */
INLINE int dvmFindFieldOffset(const ClassObject* clazz,
    const char* fieldName, const char* signature)
{
    InstField* pField = dvmFindInstanceField(clazz, fieldName, signature);
    if (pField == NULL)
        return -1;
    else
        return pField->byteOffset;
}

/*
 * Helpers.
 */
INLINE bool dvmIsPublicMethod(const Method* method) {
    return (method->accessFlags & ACC_PUBLIC) != 0;
}
INLINE bool dvmIsPrivateMethod(const Method* method) {
    return (method->accessFlags & ACC_PRIVATE) != 0;
}
INLINE bool dvmIsStaticMethod(const Method* method) {
    return (method->accessFlags & ACC_STATIC) != 0;
}
INLINE bool dvmIsSynchronizedMethod(const Method* method) {
    return (method->accessFlags & ACC_SYNCHRONIZED) != 0;
}
INLINE bool dvmIsDeclaredSynchronizedMethod(const Method* method) {
    return (method->accessFlags & ACC_DECLARED_SYNCHRONIZED) != 0;
}
INLINE bool dvmIsFinalMethod(const Method* method) {
    return (method->accessFlags & ACC_FINAL) != 0;
}
INLINE bool dvmIsNativeMethod(const Method* method) {
    return (method->accessFlags & ACC_NATIVE) != 0;
}
INLINE bool dvmIsAbstractMethod(const Method* method) {
    return (method->accessFlags & ACC_ABSTRACT) != 0;
}
INLINE bool dvmIsSyntheticMethod(const Method* method) {
    return (method->accessFlags & ACC_SYNTHETIC) != 0;
}
INLINE bool dvmIsMirandaMethod(const Method* method) {
    return (method->accessFlags & ACC_MIRANDA) != 0;
}
INLINE bool dvmIsConstructorMethod(const Method* method) {
    return *method->name == '<';
}
/* Dalvik puts private, static, and constructors into non-virtual table */
INLINE bool dvmIsDirectMethod(const Method* method) {
    return dvmIsPrivateMethod(method) ||
           dvmIsStaticMethod(method) ||
           dvmIsConstructorMethod(method);
}
/* Get whether the given method has associated bytecode. This is the
 * case for methods which are neither native nor abstract. */
INLINE bool dvmIsBytecodeMethod(const Method* method) {
    return (method->accessFlags & (ACC_NATIVE | ACC_ABSTRACT)) == 0;
}

INLINE bool dvmIsProtectedField(const Field* field) {
    return (field->accessFlags & ACC_PROTECTED) != 0;
}
INLINE bool dvmIsStaticField(const Field* field) {
    return (field->accessFlags & ACC_STATIC) != 0;
}
INLINE bool dvmIsFinalField(const Field* field) {
    return (field->accessFlags & ACC_FINAL) != 0;
}
INLINE bool dvmIsVolatileField(const Field* field) {
    return (field->accessFlags & ACC_VOLATILE) != 0;
}

INLINE bool dvmIsInterfaceClass(const ClassObject* clazz) {
    return (clazz->accessFlags & ACC_INTERFACE) != 0;
}
INLINE bool dvmIsPublicClass(const ClassObject* clazz) {
    return (clazz->accessFlags & ACC_PUBLIC) != 0;
}
INLINE bool dvmIsFinalClass(const ClassObject* clazz) {
    return (clazz->accessFlags & ACC_FINAL) != 0;
}
INLINE bool dvmIsAbstractClass(const ClassObject* clazz) {
    return (clazz->accessFlags & ACC_ABSTRACT) != 0;
}
INLINE bool dvmIsAnnotationClass(const ClassObject* clazz) {
    return (clazz->accessFlags & ACC_ANNOTATION) != 0;
}
INLINE bool dvmIsPrimitiveClass(const ClassObject* clazz) {
    return clazz->primitiveType != PRIM_NOT;
}

/* linked, here meaning prepared and resolved */
INLINE bool dvmIsClassLinked(const ClassObject* clazz) {
    return clazz->status >= CLASS_RESOLVED;
}
/* has class been verified? */
INLINE bool dvmIsClassVerified(const ClassObject* clazz) {
    return clazz->status >= CLASS_VERIFIED;
}

/*
 * Return whether the given object is an instance of Class.
 */
INLINE bool dvmIsClassObject(const Object* obj) {
    assert(obj != NULL);
    assert(obj->clazz != NULL);
    return IS_CLASS_FLAG_SET(obj->clazz, CLASS_ISCLASS);
}

/*
 * Return whether the given object is the class Class (that is, the
 * unique class which is an instance of itself).
 */
INLINE bool dvmIsTheClassClass(const ClassObject* clazz) {
    assert(clazz != NULL);
    return IS_CLASS_FLAG_SET(clazz, CLASS_ISCLASS);
}

/*
 * Get the associated code struct for a method. This returns NULL
 * for non-bytecode methods.
 */
INLINE const DexCode* dvmGetMethodCode(const Method* meth) {
    if (dvmIsBytecodeMethod(meth)) {
        /*
         * The insns field for a bytecode method actually points at
         * &(DexCode.insns), so we can subtract back to get at the
         * DexCode in front.
         */
        return (const DexCode*)
            (((const u1*) meth->insns) - offsetof(DexCode, insns));
    } else {
        return NULL;
    }
}

/*
 * Get the size of the insns associated with a method. This returns 0
 * for non-bytecode methods.
 */
INLINE u4 dvmGetMethodInsnsSize(const Method* meth) {
    const DexCode* pCode = dvmGetMethodCode(meth);
    return (pCode == NULL) ? 0 : pCode->insnsSize;
}

/* debugging */
void dvmDumpObject(const Object* obj);

#endif  // DALVIK_OO_OBJECT_H_
