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

#ifndef ART_RUNTIME_METHOD_HELPER_INL_H_
#define ART_RUNTIME_METHOD_HELPER_INL_H_

#include "method_helper.h"

#include "class_linker.h"
#include "mirror/object_array.h"
#include "runtime.h"
#include "thread-inl.h"

namespace art {

inline bool MethodHelper::HasSameNameAndSignature(MethodHelper* other) {
  const DexFile* dex_file = method_->GetDexFile();
  const DexFile::MethodId& mid = dex_file->GetMethodId(GetMethod()->GetDexMethodIndex());
  if (method_->GetDexCache() == other->method_->GetDexCache()) {
    const DexFile::MethodId& other_mid =
        dex_file->GetMethodId(other->GetMethod()->GetDexMethodIndex());
    return mid.name_idx_ == other_mid.name_idx_ && mid.proto_idx_ == other_mid.proto_idx_;
  }
  const DexFile* other_dex_file = other->method_->GetDexFile();
  const DexFile::MethodId& other_mid =
      other_dex_file->GetMethodId(other->GetMethod()->GetDexMethodIndex());
  if (!DexFileStringEquals(dex_file, mid.name_idx_, other_dex_file, other_mid.name_idx_)) {
    return false;  // Name mismatch.
  }
  return dex_file->GetMethodSignature(mid) == other_dex_file->GetMethodSignature(other_mid);
}

inline mirror::Class* MethodHelper::GetClassFromTypeIdx(uint16_t type_idx, bool resolve) {
  mirror::ArtMethod* method = GetMethod();
  mirror::Class* type = method->GetDexCacheResolvedType(type_idx);
  if (type == nullptr && resolve) {
    type = Runtime::Current()->GetClassLinker()->ResolveType(type_idx, method);
    CHECK(type != nullptr || Thread::Current()->IsExceptionPending());
  }
  return type;
}

inline mirror::Class* MethodHelper::GetReturnType(bool resolve) {
  mirror::ArtMethod* method = GetMethod();
  const DexFile* dex_file = method->GetDexFile();
  const DexFile::MethodId& method_id = dex_file->GetMethodId(method->GetDexMethodIndex());
  const DexFile::ProtoId& proto_id = dex_file->GetMethodPrototype(method_id);
  uint16_t return_type_idx = proto_id.return_type_idx_;
  return GetClassFromTypeIdx(return_type_idx, resolve);
}

inline mirror::String* MethodHelper::ResolveString(uint32_t string_idx) {
  mirror::ArtMethod* method = GetMethod();
  mirror::String* s = method->GetDexCacheStrings()->Get(string_idx);
  if (UNLIKELY(s == nullptr)) {
    StackHandleScope<1> hs(Thread::Current());
    Handle<mirror::DexCache> dex_cache(hs.NewHandle(method->GetDexCache()));
    s = Runtime::Current()->GetClassLinker()->ResolveString(*method->GetDexFile(), string_idx,
                                                            dex_cache);
  }
  return s;
}

}  // namespace art

#endif  // ART_RUNTIME_METHOD_HELPER_INL_H_
