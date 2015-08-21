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
 * UTF-8 and Unicode string manipulation functions, plus convenience
 * functions for working with java/lang/String.
 */
#ifndef DALVIK_STRING_H_
#define DALVIK_STRING_H_

#include <string>
#include <vector>

/*
 * (This is private to UtfString.c, but we cheat a bit and also use it
 * for InlineNative.c.  Not really worth creating a separate header.)
 *
 * We can avoid poking around in gDvm by hard-coding the expected values of
 * the String field offsets.  This will be annoying if String is in flux
 * or the VM field layout is changing, so we use defines here to make it
 * easy to switch back to the gDvm version.
 *
 * The values are checked for correctness during startup.
 */
//#define USE_GLOBAL_STRING_DEFS
#ifdef USE_GLOBAL_STRING_DEFS
# define STRING_FIELDOFF_VALUE      gDvm.offJavaLangString_value
# define STRING_FIELDOFF_OFFSET     gDvm.offJavaLangString_offset
# define STRING_FIELDOFF_COUNT      gDvm.offJavaLangString_count
# define STRING_FIELDOFF_HASHCODE   gDvm.offJavaLangString_hashCode
#else
# define STRING_FIELDOFF_VALUE      8
# define STRING_FIELDOFF_HASHCODE   12
# define STRING_FIELDOFF_OFFSET     16
# define STRING_FIELDOFF_COUNT      20
#endif

/*
 * Hash function for modified UTF-8 strings.
 */
u4 dvmComputeUtf8Hash(const char* str);

/*
 * Hash function for string objects. Ensures the hash code field is
 * populated and returns its value.
 */
u4 dvmComputeStringHash(StringObject* strObj);

/*
 * Create a java.lang.String[] from a vector of C++ strings.
 *
 * The caller must call dvmReleaseTrackedAlloc() on the returned array,
 * but not on the individual elements.
 *
 * Returns NULL and throws an exception on failure.
 */
ArrayObject* dvmCreateStringArray(const std::vector<std::string>& strings);

/*
 * Create a java/lang/String from a C string.
 *
 * The caller must call dvmReleaseTrackedAlloc() on the return value.
 *
 * Returns NULL and throws an exception on failure.
 */
StringObject* dvmCreateStringFromCstr(const char* utf8Str);

/*
 * Create a java/lang/String from a C++ string.
 *
 * The caller must call dvmReleaseTrackedAlloc() on the return value.
 *
 * Returns NULL and throws an exception on failure.
 */
StringObject* dvmCreateStringFromCstr(const std::string& utf8Str);

/*
 * Create a java/lang/String from a C string, given its UTF-16 length
 * (number of UTF-16 code points).
 *
 * The caller must call dvmReleaseTrackedAlloc() on the return value.
 *
 * Returns NULL and throws an exception on failure.
 */
StringObject* dvmCreateStringFromCstrAndLength(const char* utf8Str,
    u4 utf16Length);

/*
 * Compute the number of characters in a "modified UTF-8" string.  This will
 * match the result from strlen() so long as there are no multi-byte chars.
 */
size_t dvmUtf8Len(const char* utf8Str);

/*
 * Convert a UTF-8 string to UTF-16.  "utf16Str" must have enough room
 * to hold the output.
 */
void dvmConvertUtf8ToUtf16(u2* utf16Str, const char* utf8Str);

/*
 * Create a java/lang/String from a Unicode string.
 *
 * The caller must call dvmReleaseTrackedAlloc() on the return value.
 */
StringObject* dvmCreateStringFromUnicode(const u2* unichars, int len);

/*
 * Create a UTF-8 C string from a java/lang/String.  Caller must free
 * the result.
 *
 * Returns NULL if "jstr" is NULL.
 */
char* dvmCreateCstrFromString(const StringObject* jstr);

/*
 * Create a UTF-8 C string from a region of a java/lang/String.  (Used by
 * the JNI GetStringUTFRegion call.)
 */
void dvmGetStringUtfRegion(const StringObject* jstr,
        int start, int len, char* buf);

/*
 * Compare two string objects.  (This is a dvmHashTableLookup() callback.)
 */
int dvmHashcmpStrings(const void* vstrObj1, const void* vstrObj2);

#endif  // DALVIK_STRING_H_
