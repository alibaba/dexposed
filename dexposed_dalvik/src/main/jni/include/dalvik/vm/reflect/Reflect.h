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
 * Basic reflection calls and utility functions.
 */
#ifndef DALVIK_REFLECT_REFLECT_H_
#define DALVIK_REFLECT_REFLECT_H_

/*
 * During startup, validate the "box" classes, e.g. java/lang/Integer.
 */
bool dvmValidateBoxClasses();

/*
 * Get all fields declared by a class.
 *
 * Includes both class and instance fields.
 */
ArrayObject* dvmGetDeclaredFields(ClassObject* clazz, bool publicOnly);

/*
 * Get the named field.
 */
Object* dvmGetDeclaredField(ClassObject* clazz, StringObject* nameObj);

/*
 * Get all constructors declared by a class.
 */
ArrayObject* dvmGetDeclaredConstructors(ClassObject* clazz, bool publicOnly);

/*
 * Get all methods declared by a class.
 *
 * This includes both static and virtual methods, and can include private
 * members if "publicOnly" is false.  It does not include Miranda methods,
 * since those weren't declared in the class, or constructors.
 */
ArrayObject* dvmGetDeclaredMethods(ClassObject* clazz, bool publicOnly);

/*
 * Get the named method.
 */
Object* dvmGetDeclaredConstructorOrMethod(ClassObject* clazz,
    StringObject* nameObj, ArrayObject* args);

/*
 * Get all interfaces a class implements. If this is unable to allocate
 * the result array, this raises an OutOfMemoryError and returns NULL.
 */
ArrayObject* dvmGetInterfaces(ClassObject* clazz);

/*
 * Convert slot numbers back to objects.
 */
Field* dvmSlotToField(ClassObject* clazz, int slot);
Method* dvmSlotToMethod(ClassObject* clazz, int slot);

/*
 * Convert a primitive value, performing a widening conversion if necessary.
 */
int dvmConvertPrimitiveValue(PrimitiveType srcType,
    PrimitiveType dstType, const s4* srcPtr, s4* dstPtr);

/*
 * Convert the argument to the specified type.
 *
 * Returns the width of the argument (1 for most types, 2 for J/D, -1 on
 * error).
 */
int dvmConvertArgument(DataObject* arg, ClassObject* type, s4* ins);

/*
 * Box a primitive value into an object.  If "returnType" is
 * not primitive, this just returns "value" cast to an object.
 */
DataObject* dvmBoxPrimitive(JValue value, ClassObject* returnType);

/*
 * Unwrap a boxed primitive.  If "returnType" is not primitive, this just
 * returns "value" cast into a JValue.
 */
bool dvmUnboxPrimitive(Object* value, ClassObject* returnType,
    JValue* pResult);

/*
 * Return the class object that matches the method's signature.  For
 * primitive types, returns the box class.
 */
ClassObject* dvmGetBoxedReturnType(const Method* meth);

/*
 * JNI reflection support.
 */
Field* dvmGetFieldFromReflectObj(Object* obj);
Method* dvmGetMethodFromReflectObj(Object* obj);
Object* dvmCreateReflectObjForField(const ClassObject* clazz, Field* field);
Object* dvmCreateReflectObjForMethod(const ClassObject* clazz, Method* method);

/*
 * Quick test to determine if the method in question is a reflection call.
 * Used for some stack parsing.  Currently defined as "the method's declaring
 * class is java.lang.reflect.Method".
 */
INLINE bool dvmIsReflectionMethod(const Method* method)
{
    return (method->clazz == gDvm.classJavaLangReflectMethod);
}

/*
 * Proxy class generation.
 */
ClassObject* dvmGenerateProxyClass(StringObject* str, ArrayObject* interfaces,
    Object* loader);

/*
 * Create a new java.lang.reflect.Method object based on "meth".
 */
Object* dvmCreateReflectMethodObject(const Method* meth);

/*
 * Return an array of Annotation objects for the specified piece.  For method
 * parameters this is an array of arrays of Annotation objects.
 *
 * Method also applies to Constructor.
 */
ArrayObject* dvmGetClassAnnotations(const ClassObject* clazz);
ArrayObject* dvmGetMethodAnnotations(const Method* method);
ArrayObject* dvmGetFieldAnnotations(const Field* field);
ArrayObject* dvmGetParameterAnnotations(const Method* method);

/*
 * Return the annotation if it exists.
 */
Object* dvmGetClassAnnotation(const ClassObject* clazz, const ClassObject* annotationClazz);
Object* dvmGetMethodAnnotation(const ClassObject* clazz, const Method* method,
        const ClassObject* annotationClazz);
Object* dvmGetFieldAnnotation(const ClassObject* clazz, const Field* method,
        const ClassObject* annotationClazz);

/*
 * Return true if the annotation exists.
 */
bool dvmIsClassAnnotationPresent(const ClassObject* clazz, const ClassObject* annotationClazz);
bool dvmIsMethodAnnotationPresent(const ClassObject* clazz, const Method* method,
        const ClassObject* annotationClazz);
bool dvmIsFieldAnnotationPresent(const ClassObject* clazz, const Field* method,
        const ClassObject* annotationClazz);

/*
 * Find the default value for an annotation member.
 */
Object* dvmGetAnnotationDefaultValue(const Method* method);

/*
 * Get the list of thrown exceptions for a method.  Returns NULL if there
 * are no exceptions listed.
 */
ArrayObject* dvmGetMethodThrows(const Method* method);

/*
 * Get the Signature annotation.
 */
ArrayObject* dvmGetClassSignatureAnnotation(const ClassObject* clazz);
ArrayObject* dvmGetMethodSignatureAnnotation(const Method* method);
ArrayObject* dvmGetFieldSignatureAnnotation(const Field* field);

/*
 * Get the EnclosingMethod attribute from an annotation.  Returns a Method
 * object, or NULL.
 */
Object* dvmGetEnclosingMethod(const ClassObject* clazz);

/*
 * Return clazz's declaring class, or NULL if there isn't one.
 */
ClassObject* dvmGetDeclaringClass(const ClassObject* clazz);

/*
 * Return clazz's enclosing class, or NULL if there isn't one.
 */
ClassObject* dvmGetEnclosingClass(const ClassObject* clazz);

/*
 * Get the EnclosingClass attribute from an annotation.  If found, returns
 * "true".  A String with the original name of the class and the original
 * access flags are returned through the arguments.  (The name will be NULL
 * for an anonymous inner class.)
 */
bool dvmGetInnerClass(const ClassObject* clazz, StringObject** pName,
    int* pAccessFlags);

/*
 * Get an array of class objects from the MemberClasses annotation.  Returns
 * NULL if none found.
 */
ArrayObject* dvmGetDeclaredClasses(const ClassObject* clazz);

/*
 * Used to pass values out of annotation (and encoded array) processing
 * functions.
 */
struct AnnotationValue {
    JValue  value;
    u1      type;
};


/**
 * Iterator structure for iterating over DexEncodedArray instances. The
 * structure should be treated as opaque.
 */
struct EncodedArrayIterator {
    const u1* cursor;                    /* current cursor */
    u4 elementsLeft;                     /* number of elements left to read */
    const DexEncodedArray* encodedArray; /* instance being iterated over */
    u4 size;                             /* number of elements in instance */
    const ClassObject* clazz;            /* class to resolve with respect to */
};

/**
 * Initializes an encoded array iterator.
 *
 * @param iterator iterator to initialize
 * @param encodedArray encoded array to iterate over
 * @param clazz class to use when resolving strings and types
 */
void dvmEncodedArrayIteratorInitialize(EncodedArrayIterator* iterator,
        const DexEncodedArray* encodedArray, const ClassObject* clazz);

/**
 * Returns whether there are more elements to be read.
 */
bool dvmEncodedArrayIteratorHasNext(const EncodedArrayIterator* iterator);

/**
 * Returns the next decoded value from the iterator, advancing its
 * cursor. This returns primitive values in their corresponding union
 * slots, and returns everything else (including nulls) as object
 * references in the "l" union slot.
 *
 * The caller must call dvmReleaseTrackedAlloc() on any returned reference.
 *
 * @param value pointer to store decoded value into
 * @returns true if a value was decoded and the cursor advanced; false if
 * the last value had already been decoded or if there was a problem decoding
 */
bool dvmEncodedArrayIteratorGetNext(EncodedArrayIterator* iterator,
        AnnotationValue* value);

#endif  // DALVIK_REFLECT_REFLECT_H_
