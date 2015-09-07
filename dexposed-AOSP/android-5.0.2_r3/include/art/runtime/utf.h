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

#ifndef ART_RUNTIME_UTF_H_
#define ART_RUNTIME_UTF_H_

#include "base/macros.h"
#include "base/mutex.h"

#include <stddef.h>
#include <stdint.h>

/*
 * All UTF-8 in art is actually modified UTF-8. Mostly, this distinction
 * doesn't matter.
 *
 * See http://en.wikipedia.org/wiki/UTF-8#Modified_UTF-8 for the details.
 */
namespace art {

namespace mirror {
  template<class T> class PrimitiveArray;
  typedef PrimitiveArray<uint16_t> CharArray;
}  // namespace mirror

/*
 * Returns the number of UTF-16 characters in the given modified UTF-8 string.
 */
size_t CountModifiedUtf8Chars(const char* utf8);

/*
 * Returns the number of modified UTF-8 bytes needed to represent the given
 * UTF-16 string.
 */
size_t CountUtf8Bytes(const uint16_t* chars, size_t char_count);

/*
 * Convert from Modified UTF-8 to UTF-16.
 */
void ConvertModifiedUtf8ToUtf16(uint16_t* utf16_out, const char* utf8_in);

/*
 * Compare two modified UTF-8 strings as UTF-16 code point values in a non-locale sensitive manner
 */
ALWAYS_INLINE int CompareModifiedUtf8ToModifiedUtf8AsUtf16CodePointValues(const char* utf8_1,
                                                                          const char* utf8_2);

/*
 * Compare a modified UTF-8 string with a UTF-16 string as code point values in a non-locale
 * sensitive manner.
 */
int CompareModifiedUtf8ToUtf16AsCodePointValues(const char* utf8_1, const uint16_t* utf8_2);

/*
 * Convert from UTF-16 to Modified UTF-8. Note that the output is _not_
 * NUL-terminated. You probably need to call CountUtf8Bytes before calling
 * this anyway, so if you want a NUL-terminated string, you know where to
 * put the NUL byte.
 */
void ConvertUtf16ToModifiedUtf8(char* utf8_out, const uint16_t* utf16_in, size_t char_count);

/*
 * The java.lang.String hashCode() algorithm.
 */
int32_t ComputeUtf16Hash(mirror::CharArray* chars, int32_t offset, size_t char_count)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
int32_t ComputeUtf16Hash(const uint16_t* chars, size_t char_count);

// Compute a hash code of a UTF-8 string.
int32_t ComputeUtf8Hash(const char* chars);

/*
 * Retrieve the next UTF-16 character from a UTF-8 string.
 *
 * Advances "*utf8_data_in" to the start of the next character.
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
uint16_t GetUtf16FromUtf8(const char** utf8_data_in);

}  // namespace art

#endif  // ART_RUNTIME_UTF_H_
