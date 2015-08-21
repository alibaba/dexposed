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
 * Functions for dealing with method prototypes
 */

#ifndef LIBDEX_DEXPROTO_H_
#define LIBDEX_DEXPROTO_H_

#include "DexFile.h"

/*
 * Single-thread single-string cache. This structure holds a pointer to
 * a string which is semi-automatically manipulated by some of the
 * method prototype functions. Functions which use in this struct
 * generally return a string that is valid until the next
 * time the same DexStringCache is used.
 */
struct DexStringCache {
    char* value;          /* the latest value */
    size_t allocatedSize; /* size of the allocated buffer, if allocated */
    char buffer[120];     /* buffer used to hold small-enough results */
};

/*
 * Make sure that the given cache can hold a string of the given length,
 * including the final '\0' byte.
 */
void dexStringCacheAlloc(DexStringCache* pCache, size_t length);

/*
 * Initialize the given DexStringCache. Use this function before passing
 * one into any other function.
 */
void dexStringCacheInit(DexStringCache* pCache);

/*
 * Release the allocated contents of the given DexStringCache, if any.
 * Use this function after your last use of a DexStringCache.
 */
void dexStringCacheRelease(DexStringCache* pCache);

/*
 * If the given DexStringCache doesn't already point at the given value,
 * make a copy of it into the cache. This always returns a writable
 * pointer to the contents (whether or not a copy had to be made). This
 * function is intended to be used after making a call that at least
 * sometimes doesn't populate a DexStringCache.
 */
char* dexStringCacheEnsureCopy(DexStringCache* pCache, const char* value);

/*
 * Abandon the given DexStringCache, and return a writable copy of the
 * given value (reusing the string cache's allocation if possible).
 * The return value must be free()d by the caller. Use this instead of
 * dexStringCacheRelease() if you want the buffer to survive past the
 * scope of the DexStringCache.
 */
char* dexStringCacheAbandon(DexStringCache* pCache, const char* value);

/*
 * Method prototype structure, which refers to a protoIdx in a
 * particular DexFile.
 */
struct DexProto {
    const DexFile* dexFile;     /* file the idx refers to */
    u4 protoIdx;                /* index into proto_ids table of dexFile */
};

/*
 * Set the given DexProto to refer to the prototype of the given MethodId.
 */
DEX_INLINE void dexProtoSetFromMethodId(DexProto* pProto,
    const DexFile* pDexFile, const DexMethodId* pMethodId)
{
    pProto->dexFile = pDexFile;
    pProto->protoIdx = pMethodId->protoIdx;
}

/*
 * Get the short-form method descriptor for the given prototype. The
 * prototype must be protoIdx-based.
 */
const char* dexProtoGetShorty(const DexProto* pProto);

/*
 * Get the full method descriptor for the given prototype.
 */
const char* dexProtoGetMethodDescriptor(const DexProto* pProto,
    DexStringCache* pCache);

/*
 * Get a copy of the descriptor string associated with the given prototype.
 * The returned pointer must be free()ed by the caller.
 */
char* dexProtoCopyMethodDescriptor(const DexProto* pProto);

/*
 * Get the parameter descriptors for the given prototype. This is the
 * concatenation of all the descriptors for all the parameters, in
 * order, with no other adornment.
 */
const char* dexProtoGetParameterDescriptors(const DexProto* pProto,
    DexStringCache* pCache);

/*
 * Return the utf-8 encoded descriptor string from the proto of a MethodId.
 */
DEX_INLINE const char* dexGetDescriptorFromMethodId(const DexFile* pDexFile,
        const DexMethodId* pMethodId, DexStringCache* pCache)
{
    DexProto proto;

    dexProtoSetFromMethodId(&proto, pDexFile, pMethodId);
    return dexProtoGetMethodDescriptor(&proto, pCache);
}

/*
 * Get a copy of the utf-8 encoded method descriptor string from the
 * proto of a MethodId. The returned pointer must be free()ed by the
 * caller.
 */
DEX_INLINE char* dexCopyDescriptorFromMethodId(const DexFile* pDexFile,
    const DexMethodId* pMethodId)
{
    DexProto proto;

    dexProtoSetFromMethodId(&proto, pDexFile, pMethodId);
    return dexProtoCopyMethodDescriptor(&proto);
}

/*
 * Get the type descriptor for the return type of the given prototype.
 */
const char* dexProtoGetReturnType(const DexProto* pProto);

/*
 * Get the parameter count of the given prototype.
 */
size_t dexProtoGetParameterCount(const DexProto* pProto);

/*
 * Compute the number of parameter words (u4 units) required by the
 * given prototype. For example, if the method takes (int, long) and
 * returns double, this would return 3 (one for the int, two for the
 * long, and the return type isn't relevant).
 */
int dexProtoComputeArgsSize(const DexProto* pProto);

/*
 * Compare the two prototypes. The two prototypes are compared
 * with the return type as the major order, then the first arguments,
 * then second, etc. If two prototypes are identical except that one
 * has extra arguments, then the shorter argument is considered the
 * earlier one in sort order (similar to strcmp()).
 */
int dexProtoCompare(const DexProto* pProto1, const DexProto* pProto2);

/*
 * Compare the two prototypes, ignoring return type. The two
 * prototypes are compared with the first argument as the major order,
 * then second, etc. If two prototypes are identical except that one
 * has extra arguments, then the shorter argument is considered the
 * earlier one in sort order (similar to strcmp()).
 */
int dexProtoCompareParameters(const DexProto* pProto1,
        const DexProto* pProto2);

/*
 * Compare a prototype and a string method descriptor. The comparison
 * is done as if the descriptor were converted to a prototype and compared
 * with dexProtoCompare().
 */
int dexProtoCompareToDescriptor(const DexProto* proto, const char* descriptor);

/*
 * Compare a prototype and a concatenation of type descriptors. The
 * comparison is done as if the descriptors were converted to a
 * prototype and compared with dexProtoCompareParameters().
 */
int dexProtoCompareToParameterDescriptors(const DexProto* proto,
        const char* descriptors);

/*
 * Single-thread prototype parameter iterator. This structure holds a
 * pointer to a prototype and its parts, along with a cursor.
 */
struct DexParameterIterator {
    const DexProto* proto;
    const DexTypeList* parameters;
    int parameterCount;
    int cursor;
};

/*
 * Initialize the given DexParameterIterator to be at the start of the
 * parameters of the given prototype.
 */
void dexParameterIteratorInit(DexParameterIterator* pIterator,
        const DexProto* pProto);

/*
 * Get the type_id index for the next parameter, if any. This returns
 * kDexNoIndex if the last parameter has already been consumed.
 */
u4 dexParameterIteratorNextIndex(DexParameterIterator* pIterator);

/*
 * Get the type descriptor for the next parameter, if any. This returns
 * NULL if the last parameter has already been consumed.
 */
const char* dexParameterIteratorNextDescriptor(
        DexParameterIterator* pIterator);

#endif  // LIBDEX_DEXPROTO_H_
