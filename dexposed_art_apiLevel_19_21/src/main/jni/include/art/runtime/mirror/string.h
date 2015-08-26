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

#ifndef ART_RUNTIME_MIRROR_STRING_H_
#define ART_RUNTIME_MIRROR_STRING_H_

#include <gtest/gtest.h>

#include "gc_root.h"
#include "object.h"
#include "object_callbacks.h"

namespace art {

template<class T> class Handle;
struct StringOffsets;
class StringPiece;

namespace mirror {

// C++ mirror of java.lang.String
class MANAGED String FINAL : public Object {
 public:
  // Size of java.lang.String.class.
  static uint32_t ClassSize();

  // Size of an instance of java.lang.String not including its value array.
  static constexpr uint32_t InstanceSize() {
    return sizeof(String);
  }

  static MemberOffset CountOffset() {
    return OFFSET_OF_OBJECT_MEMBER(String, count_);
  }

  static MemberOffset ValueOffset() {
    return OFFSET_OF_OBJECT_MEMBER(String, array_);
  }

  static MemberOffset OffsetOffset() {
    return OFFSET_OF_OBJECT_MEMBER(String, offset_);
  }

  CharArray* GetCharArray() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  int32_t GetOffset() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    int32_t result = GetField32(OffsetOffset());
    DCHECK_LE(0, result);
    return result;
  }

  int32_t GetLength() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  int32_t GetHashCode() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Computes, stores, and returns the hash code.
  int32_t ComputeHashCode() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  int32_t GetUtfLength() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  uint16_t CharAt(int32_t index) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  String* Intern() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static String* AllocFromUtf16(Thread* self,
                                int32_t utf16_length,
                                const uint16_t* utf16_data_in,
                                int32_t hash_code = 0)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static String* AllocFromModifiedUtf8(Thread* self, const char* utf)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static String* AllocFromModifiedUtf8(Thread* self, int32_t utf16_length,
                                       const char* utf8_data_in)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool Equals(const char* modified_utf8) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // TODO: do we need this overload? give it a more intention-revealing name.
  bool Equals(const StringPiece& modified_utf8)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool Equals(String* that) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Compare UTF-16 code point values not in a locale-sensitive manner
  int Compare(int32_t utf16_length, const char* utf8_data_in);

  // TODO: do we need this overload? give it a more intention-revealing name.
  bool Equals(const uint16_t* that_chars, int32_t that_offset,
              int32_t that_length)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Create a modified UTF-8 encoded std::string from a java/lang/String object.
  std::string ToModifiedUtf8() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  int32_t FastIndexOf(int32_t ch, int32_t start) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  int32_t CompareTo(String* other) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static Class* GetJavaLangString() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK(!java_lang_String_.IsNull());
    return java_lang_String_.Read();
  }

  static void SetClass(Class* java_lang_String);
  static void ResetClass();
  static void VisitRoots(RootCallback* callback, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

 private:
  void SetHashCode(int32_t new_hash_code) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    // Hash code is invariant so use non-transactional mode. Also disable check as we may run inside
    // a transaction.
    DCHECK_EQ(0, GetField32(OFFSET_OF_OBJECT_MEMBER(String, hash_code_)));
    SetField32<false, false>(OFFSET_OF_OBJECT_MEMBER(String, hash_code_), new_hash_code);
  }

  void SetCount(int32_t new_count) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    // Count is invariant so use non-transactional mode. Also disable check as we may run inside
    // a transaction.
    DCHECK_LE(0, new_count);
    SetField32<false, false>(OFFSET_OF_OBJECT_MEMBER(String, count_), new_count);
  }

  void SetOffset(int32_t new_offset) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    // Offset is only used during testing so use non-transactional mode.
    DCHECK_LE(0, new_offset);
    DCHECK_GE(GetLength(), new_offset);
    SetField32<false>(OFFSET_OF_OBJECT_MEMBER(String, offset_), new_offset);
  }

  static String* Alloc(Thread* self, int32_t utf16_length)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static String* Alloc(Thread* self, Handle<CharArray> array)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void SetArray(CharArray* new_array) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Field order required by test "ValidateFieldOrderOfJavaCppUnionClasses".
  HeapReference<CharArray> array_;

  int32_t count_;

  uint32_t hash_code_;

  int32_t offset_;

  static GcRoot<Class> java_lang_String_;

  friend struct art::StringOffsets;  // for verifying offset information
  FRIEND_TEST(ObjectTest, StringLength);  // for SetOffset and SetCount
  DISALLOW_IMPLICIT_CONSTRUCTORS(String);
};

}  // namespace mirror
}  // namespace art

#endif  // ART_RUNTIME_MIRROR_STRING_H_
