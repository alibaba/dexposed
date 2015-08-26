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

#ifndef ART_RUNTIME_METHOD_HELPER_H_
#define ART_RUNTIME_METHOD_HELPER_H_

#include "base/macros.h"
#include "handle.h"
#include "mirror/art_method.h"
#include "primitive.h"

namespace art {

class MethodHelper {
 public:
  explicit MethodHelper(Handle<mirror::ArtMethod> m) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      : method_(m), shorty_(nullptr), shorty_len_(0) {
    SetMethod(m.Get());
  }

  void ChangeMethod(mirror::ArtMethod* new_m) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK(new_m != nullptr);
    SetMethod(new_m);
    shorty_ = nullptr;
  }

  mirror::ArtMethod* GetMethod() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return method_->GetInterfaceMethodIfProxy();
  }

  mirror::String* GetNameAsString(Thread* self) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const char* GetShorty() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    const char* result = shorty_;
    if (result == nullptr) {
      result = method_->GetShorty(&shorty_len_);
      shorty_ = result;
    }
    return result;
  }

  uint32_t GetShortyLength() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    if (shorty_ == nullptr) {
      GetShorty();
    }
    return shorty_len_;
  }

  // Counts the number of references in the parameter list of the corresponding method.
  // Note: Thus does _not_ include "this" for non-static methods.
  uint32_t GetNumberOfReferenceArgsWithoutReceiver() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    const char* shorty = GetShorty();
    uint32_t refs = 0;
    for (uint32_t i = 1; i < shorty_len_ ; ++i) {
      if (shorty[i] == 'L') {
        refs++;
      }
    }

    return refs;
  }

  // May cause thread suspension due to GetClassFromTypeIdx calling ResolveType this caused a large
  // number of bugs at call sites.
  mirror::Class* GetReturnType(bool resolve = true) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  size_t NumArgs() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    // "1 +" because the first in Args is the receiver.
    // "- 1" because we don't count the return type.
    return (method_->IsStatic() ? 0 : 1) + GetShortyLength() - 1;
  }

  // Get the primitive type associated with the given parameter.
  Primitive::Type GetParamPrimitiveType(size_t param) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    CHECK_LT(param, NumArgs());
    if (GetMethod()->IsStatic()) {
      param++;  // 0th argument must skip return value at start of the shorty
    } else if (param == 0) {
      return Primitive::kPrimNot;
    }
    return Primitive::GetType(GetShorty()[param]);
  }

  // Is the specified parameter a long or double, where parameter 0 is 'this' for instance methods.
  bool IsParamALongOrDouble(size_t param) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    Primitive::Type type = GetParamPrimitiveType(param);
    return type == Primitive::kPrimLong || type == Primitive::kPrimDouble;
  }

  // Is the specified parameter a reference, where parameter 0 is 'this' for instance methods.
  bool IsParamAReference(size_t param) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return GetParamPrimitiveType(param) == Primitive::kPrimNot;
  }

  ALWAYS_INLINE bool HasSameNameAndSignature(MethodHelper* other)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool HasSameSignatureWithDifferentClassLoaders(MethodHelper* other)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::Class* GetClassFromTypeIdx(uint16_t type_idx, bool resolve = true)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::String* ResolveString(uint32_t string_idx) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  uint32_t FindDexMethodIndexInOtherDexFile(const DexFile& other_dexfile)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // The name_and_signature_idx MUST point to a MethodId with the same name and signature in the
  // other_dexfile, such as the method index used to resolve this method in the other_dexfile.
  uint32_t FindDexMethodIndexInOtherDexFile(const DexFile& other_dexfile,
                                            uint32_t name_and_signature_idx)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

 private:
  // Set the method_ field, for proxy methods looking up the interface method via the resolved
  // methods table.
  void SetMethod(mirror::ArtMethod* method) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    method_.Assign(method);
  }

  Handle<mirror::ArtMethod> method_;
  const char* shorty_;
  uint32_t shorty_len_;

  DISALLOW_COPY_AND_ASSIGN(MethodHelper);
};

}  // namespace art

#endif  // ART_RUNTIME_METHOD_HELPER_H_
