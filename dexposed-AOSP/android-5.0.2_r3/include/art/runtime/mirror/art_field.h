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

#ifndef ART_RUNTIME_MIRROR_ART_FIELD_H_
#define ART_RUNTIME_MIRROR_ART_FIELD_H_

#include <jni.h>

#include "gc_root.h"
#include "modifiers.h"
#include "object.h"
#include "object_callbacks.h"
#include "primitive.h"
#include "read_barrier_option.h"

namespace art {

struct ArtFieldOffsets;
class DexFile;
class ScopedObjectAccessAlreadyRunnable;

namespace mirror {

class DexCache;

// C++ mirror of java.lang.reflect.ArtField
class MANAGED ArtField FINAL : public Object {
 public:
  // Size of java.lang.reflect.ArtField.class.
  static uint32_t ClassSize();

  // Size of an instance of java.lang.reflect.ArtField not including its value array.
  static constexpr uint32_t InstanceSize() {
    return sizeof(ArtField);
  }

  ALWAYS_INLINE static ArtField* FromReflectedField(const ScopedObjectAccessAlreadyRunnable& soa,
                                                    jobject jlr_field)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  Class* GetDeclaringClass() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void SetDeclaringClass(Class *new_declaring_class) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  uint32_t GetAccessFlags() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void SetAccessFlags(uint32_t new_access_flags) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    // Not called within a transaction.
    SetField32<false>(OFFSET_OF_OBJECT_MEMBER(ArtField, access_flags_), new_access_flags);
  }

  bool IsPublic() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccPublic) != 0;
  }

  bool IsStatic() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccStatic) != 0;
  }

  bool IsFinal() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccFinal) != 0;
  }

  uint32_t GetDexFieldIndex() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return GetField32(OFFSET_OF_OBJECT_MEMBER(ArtField, field_dex_idx_));
  }

  void SetDexFieldIndex(uint32_t new_idx) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    // Not called within a transaction.
    SetField32<false>(OFFSET_OF_OBJECT_MEMBER(ArtField, field_dex_idx_), new_idx);
  }

  // Offset to field within an Object.
  MemberOffset GetOffset() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static MemberOffset OffsetOffset() {
    return MemberOffset(OFFSETOF_MEMBER(ArtField, offset_));
  }

  MemberOffset GetOffsetDuringLinking() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void SetOffset(MemberOffset num_bytes) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // field access, null object for static fields
  bool GetBoolean(Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  template<bool kTransactionActive>
  void SetBoolean(Object* object, bool z) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  int8_t GetByte(Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  template<bool kTransactionActive>
  void SetByte(Object* object, int8_t b) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  uint16_t GetChar(Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  template<bool kTransactionActive>
  void SetChar(Object* object, uint16_t c) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  int16_t GetShort(Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  template<bool kTransactionActive>
  void SetShort(Object* object, int16_t s) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  int32_t GetInt(Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  template<bool kTransactionActive>
  void SetInt(Object* object, int32_t i) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  int64_t GetLong(Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  template<bool kTransactionActive>
  void SetLong(Object* object, int64_t j) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  float GetFloat(Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  template<bool kTransactionActive>
  void SetFloat(Object* object, float f) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  double GetDouble(Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  template<bool kTransactionActive>
  void SetDouble(Object* object, double d) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  Object* GetObject(Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  template<bool kTransactionActive>
  void SetObject(Object* object, Object* l) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Raw field accesses.
  uint32_t Get32(Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  template<bool kTransactionActive>
  void Set32(Object* object, uint32_t new_value) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  uint64_t Get64(Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  template<bool kTransactionActive>
  void Set64(Object* object, uint64_t new_value) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  Object* GetObj(Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  template<bool kTransactionActive>
  void SetObj(Object* object, Object* new_value) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  template<ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  static Class* GetJavaLangReflectArtField()  SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK(!java_lang_reflect_ArtField_.IsNull());
    return java_lang_reflect_ArtField_.Read<kReadBarrierOption>();
  }

  static void SetClass(Class* java_lang_reflect_ArtField);
  static void ResetClass();
  static void VisitRoots(RootCallback* callback, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool IsVolatile() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccVolatile) != 0;
  }

  // Returns an instance field with this offset in the given class or nullptr if not found.
  static ArtField* FindInstanceFieldWithOffset(mirror::Class* klass, uint32_t field_offset)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const char* GetName() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const char* GetTypeDescriptor() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  Primitive::Type GetTypeAsPrimitiveType() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool IsPrimitiveType() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  size_t FieldSize() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::DexCache* GetDexCache() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const DexFile* GetDexFile() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

 private:
  // Field order required by test "ValidateFieldOrderOfJavaCppUnionClasses".
  // The class we are a part of
  HeapReference<Class> declaring_class_;

  uint32_t access_flags_;

  // Dex cache index of field id
  uint32_t field_dex_idx_;

  // Offset of field within an instance or in the Class' static fields
  uint32_t offset_;

  static GcRoot<Class> java_lang_reflect_ArtField_;

  friend struct art::ArtFieldOffsets;  // for verifying offset information
  DISALLOW_IMPLICIT_CONSTRUCTORS(ArtField);
};

}  // namespace mirror
}  // namespace art

#endif  // ART_RUNTIME_MIRROR_ART_FIELD_H_
