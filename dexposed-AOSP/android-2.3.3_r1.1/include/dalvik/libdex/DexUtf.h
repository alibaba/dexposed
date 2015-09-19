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

/*
 * Validate and manipulate MUTF-8 (modified UTF-8) encoded string data.
 */

#ifndef LIBDEX_DEXUTF_H_
#define LIBDEX_DEXUTF_H_

#include "DexFile.h"

/*
 * Retrieve the next UTF-16 character from a UTF-8 string.
 *
 * Advances "*pUtf8Ptr" to the start of the next character.
 *
 * WARNING: If a string is corrupted by dropping a '\0' in the middle
 * of a 3-byte sequence, you can end up overrunning the buffer with
 * reads (and possibly with the writes if the length was computed and
 * cached before the damage). For performance reasons, this function
 * assumes that the string being parsed is known to be valid (e.g., by
 * already being verified). Most strings we process here are coming
 * out of dex files or other internal translations, so the only real
 * risk comes from the JNI NewStringUTF call.
 */
DEX_INLINE u2 dexGetUtf16FromUtf8(const char** pUtf8Ptr)
{
    unsigned int one, two, three;

    one = *(*pUtf8Ptr)++;
    if ((one & 0x80) != 0) {
        /* two- or three-byte encoding */
        two = *(*pUtf8Ptr)++;
        if ((one & 0x20) != 0) {
            /* three-byte encoding */
            three = *(*pUtf8Ptr)++;
            return ((one & 0x0f) << 12) |
                   ((two & 0x3f) << 6) |
                   (three & 0x3f);
        } else {
            /* two-byte encoding */
            return ((one & 0x1f) << 6) |
                   (two & 0x3f);
        }
    } else {
        /* one-byte encoding */
        return one;
    }
}

/* Compare two '\0'-terminated modified UTF-8 strings, using Unicode
 * code point values for comparison. This treats different encodings
 * for the same code point as equivalent, except that only a real '\0'
 * byte is considered the string terminator. The return value is as
 * for strcmp(). */
int dexUtf8Cmp(const char* s1, const char* s2);

/* for dexIsValidMemberNameUtf8(), a bit vector indicating valid low ascii */
extern u4 DEX_MEMBER_VALID_LOW_ASCII[4];

/* Helper for dexIsValidMemberUtf8(); do not call directly. */
bool dexIsValidMemberNameUtf8_0(const char** pUtf8Ptr);

/* Return whether the pointed-at modified-UTF-8 encoded character is
 * valid as part of a member name, updating the pointer to point past
 * the consumed character. This will consume two encoded UTF-16 code
 * points if the character is encoded as a surrogate pair. Also, if
 * this function returns false, then the given pointer may only have
 * been partially advanced. */
DEX_INLINE bool dexIsValidMemberNameUtf8(const char** pUtf8Ptr) {
    u1 c = (u1) **pUtf8Ptr;
    if (c <= 0x7f) {
        // It's low-ascii, so check the table.
        u4 wordIdx = c >> 5;
        u4 bitIdx = c & 0x1f;
        (*pUtf8Ptr)++;
        return (DEX_MEMBER_VALID_LOW_ASCII[wordIdx] & (1 << bitIdx)) != 0;
    }

    /*
     * It's a multibyte encoded character. Call a non-inline function
     * for the heavy lifting.
     */
    return dexIsValidMemberNameUtf8_0(pUtf8Ptr);
}

/* Return whether the given string is a valid field or method name. */
bool dexIsValidMemberName(const char* s);

/* Return whether the given string is a valid type descriptor. */
bool dexIsValidTypeDescriptor(const char* s);

/* Return whether the given string is a valid internal-form class
 * name, with components separated either by dots or slashes as
 * specified. A class name is like a type descriptor, except that it
 * can't name a primitive type (including void). In terms of syntax,
 * the form is either (a) the name of the class without adornment
 * (that is, not bracketed by "L" and ";"); or (b) identical to the
 * type descriptor syntax for array types. */
bool dexIsValidClassName(const char* s, bool dotSeparator);

/* Return whether the given string is a valid reference descriptor. This
 * is true if dexIsValidTypeDescriptor() returns true and the descriptor
 * is for a class or array and not a primitive type. */
bool dexIsReferenceDescriptor(const char* s);

/* Return whether the given string is a valid class descriptor. This
 * is true if dexIsValidTypeDescriptor() returns true and the descriptor
 * is for a class and not an array or primitive type. */
bool dexIsClassDescriptor(const char* s);

/* Return whether the given string is a valid field type descriptor. This
 * is true if dexIsValidTypeDescriptor() returns true and the descriptor
 * is for anything but "void". */
bool dexIsFieldDescriptor(const char* s);

#endif  // LIBDEX_DEXUTF_H_
