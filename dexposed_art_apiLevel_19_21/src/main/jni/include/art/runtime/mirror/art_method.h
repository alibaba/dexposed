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

#ifndef ART_RUNTIME_MIRROR_ART_METHOD_H_
#define ART_RUNTIME_MIRROR_ART_METHOD_H_

#include "dex_file.h"
#include "gc_root.h"
#include "invoke_type.h"
#include "modifiers.h"
#include "object.h"
#include "object_callbacks.h"
#include "quick/quick_method_frame_info.h"
#include "read_barrier_option.h"

namespace art {

struct ArtMethodOffsets;
struct ConstructorMethodOffsets;
union JValue;
class MethodHelper;
class ScopedObjectAccessAlreadyRunnable;
class StringPiece;
class ShadowFrame;

namespace mirror {

typedef void (EntryPointFromInterpreter)(Thread* self, MethodHelper& mh,
    const DexFile::CodeItem* code_item, ShadowFrame* shadow_frame, JValue* result);

// C++ mirror of java.lang.reflect.ArtMethod.
class MANAGED ArtMethod FINAL : public Object {
 public:
  // Size of java.lang.reflect.ArtMethod.class.
  static uint32_t ClassSize();

  // Size of an instance of java.lang.reflect.ArtMethod not including its value array.
  static constexpr uint32_t InstanceSize() {
    return sizeof(ArtMethod);
  }

  static ArtMethod* FromReflectedMethod(const ScopedObjectAccessAlreadyRunnable& soa,
                                        jobject jlr_method)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  Class* GetDeclaringClass() ALWAYS_INLINE SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void SetDeclaringClass(Class *new_declaring_class) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static MemberOffset DeclaringClassOffset() {
    return MemberOffset(OFFSETOF_MEMBER(ArtMethod, declaring_class_));
  }

  uint32_t GetAccessFlags() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void SetAccessFlags(uint32_t new_access_flags) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    // Not called within a transaction.
    SetField32<false>(OFFSET_OF_OBJECT_MEMBER(ArtMethod, access_flags_), new_access_flags);
  }

  // Approximate what kind of method call would be used for this method.
  InvokeType GetInvokeType() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Returns true if the method is declared public.
  bool IsPublic() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccPublic) != 0;
  }

  // Returns true if the method is declared private.
  bool IsPrivate() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccPrivate) != 0;
  }

  // Returns true if the method is declared static.
  bool IsStatic() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccStatic) != 0;
  }

  // Returns true if the method is a constructor.
  bool IsConstructor() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccConstructor) != 0;
  }

  // Returns true if the method is a class initializer.
  bool IsClassInitializer() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return IsConstructor() && IsStatic();
  }

  // Returns true if the method is static, private, or a constructor.
  bool IsDirect() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return IsDirect(GetAccessFlags());
  }

  static bool IsDirect(uint32_t access_flags) {
    return (access_flags & (kAccStatic | kAccPrivate | kAccConstructor)) != 0;
  }

  // Returns true if the method is declared synchronized.
  bool IsSynchronized() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    uint32_t synchonized = kAccSynchronized | kAccDeclaredSynchronized;
    return (GetAccessFlags() & synchonized) != 0;
  }

  bool IsFinal() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccFinal) != 0;
  }

  bool IsMiranda() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccMiranda) != 0;
  }

  bool IsNative() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccNative) != 0;
  }

  bool IsFastNative() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    uint32_t mask = kAccFastNative | kAccNative;
    return (GetAccessFlags() & mask) == mask;
  }

  bool IsAbstract() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccAbstract) != 0;
  }

  bool IsSynthetic() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccSynthetic) != 0;
  }

  bool IsProxyMethod() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool IsPreverified() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return (GetAccessFlags() & kAccPreverified) != 0;
  }

  void SetPreverified() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK(!IsPreverified());
    SetAccessFlags(GetAccessFlags() | kAccPreverified);
  }

  bool IsPortableCompiled() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return kUsePortableCompiler && ((GetAccessFlags() & kAccPortableCompiled) != 0);
  }

  void SetIsPortableCompiled() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK(!IsPortableCompiled());
    SetAccessFlags(GetAccessFlags() | kAccPortableCompiled);
  }

  void ClearIsPortableCompiled() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK(IsPortableCompiled());
    SetAccessFlags(GetAccessFlags() & ~kAccPortableCompiled);
  }

  bool CheckIncompatibleClassChange(InvokeType type) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  uint16_t GetMethodIndex() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  size_t GetVtableIndex() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return GetMethodIndex();
  }

  void SetMethodIndex(uint16_t new_method_index) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    // Not called within a transaction.
    SetField32<false>(OFFSET_OF_OBJECT_MEMBER(ArtMethod, method_index_), new_method_index);
  }

  static MemberOffset MethodIndexOffset() {
    return OFFSET_OF_OBJECT_MEMBER(ArtMethod, method_index_);
  }

  uint32_t GetCodeItemOffset() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return GetField32(OFFSET_OF_OBJECT_MEMBER(ArtMethod, dex_code_item_offset_));
  }

  void SetCodeItemOffset(uint32_t new_code_off) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    // Not called within a transaction.
    SetField32<false>(OFFSET_OF_OBJECT_MEMBER(ArtMethod, dex_code_item_offset_), new_code_off);
  }

  // Number of 32bit registers that would be required to hold all the arguments
  static size_t NumArgRegisters(const StringPiece& shorty);

  uint32_t GetDexMethodIndex() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void SetDexMethodIndex(uint32_t new_idx) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    // Not called within a transaction.
    SetField32<false>(OFFSET_OF_OBJECT_MEMBER(ArtMethod, dex_method_index_), new_idx);
  }

  ObjectArray<String>* GetDexCacheStrings() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void SetDexCacheStrings(ObjectArray<String>* new_dex_cache_strings)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static MemberOffset DexCacheStringsOffset() {
    return OFFSET_OF_OBJECT_MEMBER(ArtMethod, dex_cache_strings_);
  }

  static MemberOffset DexCacheResolvedMethodsOffset() {
    return OFFSET_OF_OBJECT_MEMBER(ArtMethod, dex_cache_resolved_methods_);
  }

  static MemberOffset DexCacheResolvedTypesOffset() {
    return OFFSET_OF_OBJECT_MEMBER(ArtMethod, dex_cache_resolved_types_);
  }

  ArtMethod* GetDexCacheResolvedMethod(uint16_t method_idx)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void SetDexCacheResolvedMethod(uint16_t method_idx, ArtMethod* new_method)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void SetDexCacheResolvedMethods(ObjectArray<ArtMethod>* new_dex_cache_methods)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool HasDexCacheResolvedMethods() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool HasSameDexCacheResolvedMethods(ArtMethod* other) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool HasSameDexCacheResolvedMethods(ObjectArray<ArtMethod>* other_cache)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  template <bool kWithCheck = true>
  Class* GetDexCacheResolvedType(uint32_t type_idx) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void SetDexCacheResolvedTypes(ObjectArray<Class>* new_dex_cache_types)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool HasDexCacheResolvedTypes() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool HasSameDexCacheResolvedTypes(ArtMethod* other) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool HasSameDexCacheResolvedTypes(ObjectArray<Class>* other_cache)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Find the method that this method overrides
  ArtMethod* FindOverriddenMethod() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void Invoke(Thread* self, uint32_t* args, uint32_t args_size, JValue* result, const char* shorty)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  EntryPointFromInterpreter* GetEntryPointFromInterpreter()
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return GetFieldPtr<EntryPointFromInterpreter*, kVerifyFlags>(
        OFFSET_OF_OBJECT_MEMBER(ArtMethod, entry_point_from_interpreter_));
  }

  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  void SetEntryPointFromInterpreter(EntryPointFromInterpreter* entry_point_from_interpreter)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    SetFieldPtr<false, true, kVerifyFlags>(
        OFFSET_OF_OBJECT_MEMBER(ArtMethod, entry_point_from_interpreter_),
        entry_point_from_interpreter);
  }

#if defined(ART_USE_PORTABLE_COMPILER)
  static MemberOffset EntryPointFromPortableCompiledCodeOffset() {
    return MemberOffset(OFFSETOF_MEMBER(ArtMethod, entry_point_from_portable_compiled_code_));
  }

  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  const void* GetEntryPointFromPortableCompiledCode() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return GetFieldPtr<const void*, kVerifyFlags>(
        EntryPointFromPortableCompiledCodeOffset());
  }

  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  void SetEntryPointFromPortableCompiledCode(const void* entry_point_from_portable_compiled_code)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    SetFieldPtr<false, true, kVerifyFlags>(
        EntryPointFromPortableCompiledCodeOffset(), entry_point_from_portable_compiled_code);
  }
#endif

  static MemberOffset EntryPointFromQuickCompiledCodeOffset() {
    return MemberOffset(OFFSETOF_MEMBER(ArtMethod, entry_point_from_quick_compiled_code_));
  }

  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  const void* GetEntryPointFromQuickCompiledCode() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return GetFieldPtr<const void*, kVerifyFlags>(EntryPointFromQuickCompiledCodeOffset());
  }

  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  void SetEntryPointFromQuickCompiledCode(const void* entry_point_from_quick_compiled_code)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    SetFieldPtr<false, true, kVerifyFlags>(
        EntryPointFromQuickCompiledCodeOffset(), entry_point_from_quick_compiled_code);
  }

  uint32_t GetCodeSize() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool IsWithinQuickCode(uintptr_t pc) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    uintptr_t code = reinterpret_cast<uintptr_t>(GetEntryPointFromQuickCompiledCode());
    if (code == 0) {
      return pc == 0;
    }
    /*
     * During a stack walk, a return PC may point past-the-end of the code
     * in the case that the last instruction is a call that isn't expected to
     * return.  Thus, we check <= code + GetCodeSize().
     *
     * NOTE: For Thumb both pc and code are offset by 1 indicating the Thumb state.
     */
    return code <= pc && pc <= code + GetCodeSize();
  }

  void AssertPcIsWithinQuickCode(uintptr_t pc) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

#if defined(ART_USE_PORTABLE_COMPILER)
  uint32_t GetPortableOatCodeOffset() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void SetPortableOatCodeOffset(uint32_t code_offset) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
#endif
  uint32_t GetQuickOatCodeOffset() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void SetQuickOatCodeOffset(uint32_t code_offset) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static const void* EntryPointToCodePointer(const void* entry_point) ALWAYS_INLINE {
    uintptr_t code = reinterpret_cast<uintptr_t>(entry_point);
    code &= ~0x1;  // TODO: Make this Thumb2 specific.
    return reinterpret_cast<const void*>(code);
  }

  // Actual entry point pointer to compiled oat code or nullptr.
  const void* GetQuickOatEntryPoint() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  // Actual pointer to compiled oat code or nullptr.
  const void* GetQuickOatCodePointer() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Callers should wrap the uint8_t* in a MappingTable instance for convenient access.
  const uint8_t* GetMappingTable() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  const uint8_t* GetMappingTable(const void* code_pointer)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Callers should wrap the uint8_t* in a VmapTable instance for convenient access.
  const uint8_t* GetVmapTable() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  const uint8_t* GetVmapTable(const void* code_pointer)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const uint8_t* GetNativeGcMap() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return GetFieldPtr<uint8_t*>(OFFSET_OF_OBJECT_MEMBER(ArtMethod, gc_map_));
  }
  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  void SetNativeGcMap(const uint8_t* data) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    SetFieldPtr<false, true, kVerifyFlags>(OFFSET_OF_OBJECT_MEMBER(ArtMethod, gc_map_), data);
  }

  // When building the oat need a convenient place to stuff the offset of the native GC map.
  void SetOatNativeGcMapOffset(uint32_t gc_map_offset) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  uint32_t GetOatNativeGcMapOffset() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  template <bool kCheckFrameSize = true>
  uint32_t GetFrameSizeInBytes() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    uint32_t result = GetQuickFrameInfo().FrameSizeInBytes();
    if (kCheckFrameSize) {
      DCHECK_LE(static_cast<size_t>(kStackAlignment), result);
    }
    return result;
  }

  QuickMethodFrameInfo GetQuickFrameInfo() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  QuickMethodFrameInfo GetQuickFrameInfo(const void* code_pointer)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  size_t GetReturnPcOffsetInBytes() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return GetReturnPcOffsetInBytes(GetFrameSizeInBytes());
  }

  size_t GetReturnPcOffsetInBytes(uint32_t frame_size_in_bytes)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK_EQ(frame_size_in_bytes, GetFrameSizeInBytes());
    return frame_size_in_bytes - kPointerSize;
  }

  size_t GetHandleScopeOffsetInBytes() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return kPointerSize;
  }

  void RegisterNative(Thread* self, const void* native_method, bool is_fast)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void UnregisterNative(Thread* self) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static MemberOffset NativeMethodOffset() {
    return OFFSET_OF_OBJECT_MEMBER(ArtMethod, entry_point_from_jni_);
  }

  const void* GetNativeMethod() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return GetFieldPtr<const void*>(NativeMethodOffset());
  }

  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  void SetNativeMethod(const void*) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static MemberOffset GetMethodIndexOffset() {
    return OFFSET_OF_OBJECT_MEMBER(ArtMethod, method_index_);
  }

  // Is this a CalleSaveMethod or ResolutionMethod and therefore doesn't adhere to normal
  // conventions for a method of managed code. Returns false for Proxy methods.
  bool IsRuntimeMethod() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Is this a hand crafted method used for something like describing callee saves?
  bool IsCalleeSaveMethod() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool IsResolutionMethod() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool IsImtConflictMethod() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  uintptr_t NativePcOffset(const uintptr_t pc) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  uintptr_t NativePcOffset(const uintptr_t pc, const void* quick_entry_point)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Converts a native PC to a dex PC.
  uint32_t ToDexPc(const uintptr_t pc, bool abort_on_failure = true)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Converts a dex PC to a native PC.
  uintptr_t ToNativePc(const uint32_t dex_pc) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Find the catch block for the given exception type and dex_pc. When a catch block is found,
  // indicates whether the found catch block is responsible for clearing the exception or whether
  // a move-exception instruction is present.
  static uint32_t FindCatchBlock(Handle<ArtMethod> h_this, Handle<Class> exception_type,
                                 uint32_t dex_pc, bool* has_no_move_exception)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static void SetClass(Class* java_lang_reflect_ArtMethod);

  template<ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  static Class* GetJavaLangReflectArtMethod() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static void ResetClass();

  static void VisitRoots(RootCallback* callback, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const DexFile* GetDexFile() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const char* GetDeclaringClassDescriptor() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const char* GetShorty() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    uint32_t unused_length;
    return GetShorty(&unused_length);
  }

  const char* GetShorty(uint32_t* out_length) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const Signature GetSignature() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  ALWAYS_INLINE const char* GetName() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const DexFile::CodeItem* GetCodeItem() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool IsResolvedTypeIdx(uint16_t type_idx) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  int32_t GetLineNumFromDexPC(uint32_t dex_pc) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const DexFile::ProtoId& GetPrototype() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const DexFile::TypeList* GetParameterTypeList() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const char* GetDeclaringClassSourceFile() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  uint16_t GetClassDefIndex() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const DexFile::ClassDef& GetClassDef() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const char* GetReturnTypeDescriptor() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const char* GetTypeDescriptorFromTypeIdx(uint16_t type_idx)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::ClassLoader* GetClassLoader() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::DexCache* GetDexCache() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  ALWAYS_INLINE ArtMethod* GetInterfaceMethodIfProxy() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

 protected:
  // Field order required by test "ValidateFieldOrderOfJavaCppUnionClasses".
  // The class we are a part of.
  HeapReference<Class> declaring_class_;

  // Short cuts to declaring_class_->dex_cache_ member for fast compiled code access.
  HeapReference<ObjectArray<ArtMethod>> dex_cache_resolved_methods_;

  // Short cuts to declaring_class_->dex_cache_ member for fast compiled code access.
  HeapReference<ObjectArray<Class>> dex_cache_resolved_types_;

  // Short cuts to declaring_class_->dex_cache_ member for fast compiled code access.
  HeapReference<ObjectArray<String>> dex_cache_strings_;

  // Method dispatch from the interpreter invokes this pointer which may cause a bridge into
  // compiled code.
  uint64_t entry_point_from_interpreter_;

  // Pointer to JNI function registered to this method, or a function to resolve the JNI function.
  uint64_t entry_point_from_jni_;

  // Method dispatch from portable compiled code invokes this pointer which may cause bridging into
  // quick compiled code or the interpreter.
#if defined(ART_USE_PORTABLE_COMPILER)
  uint64_t entry_point_from_portable_compiled_code_;
#endif

  // Method dispatch from quick compiled code invokes this pointer which may cause bridging into
  // portable compiled code or the interpreter.
  uint64_t entry_point_from_quick_compiled_code_;

  // Pointer to a data structure created by the compiler and used by the garbage collector to
  // determine which registers hold live references to objects within the heap. Keyed by native PC
  // offsets for the quick compiler and dex PCs for the portable.
  uint64_t gc_map_;

  // Access flags; low 16 bits are defined by spec.
  uint32_t access_flags_;

  /* Dex file fields. The defining dex file is available via declaring_class_->dex_cache_ */

  // Offset to the CodeItem.
  uint32_t dex_code_item_offset_;

  // Index into method_ids of the dex file associated with this method.
  uint32_t dex_method_index_;

  /* End of dex file fields. */

  // Entry within a dispatch table for this method. For static/direct methods the index is into
  // the declaringClass.directMethods, for virtual methods the vtable and for interface methods the
  // ifTable.
  uint32_t method_index_;

  static GcRoot<Class> java_lang_reflect_ArtMethod_;

 private:
  ObjectArray<ArtMethod>* GetDexCacheResolvedMethods() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  ObjectArray<Class>* GetDexCacheResolvedTypes() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  friend struct art::ArtMethodOffsets;  // for verifying offset information
  DISALLOW_IMPLICIT_CONSTRUCTORS(ArtMethod);
};

}  // namespace mirror
}  // namespace art

#endif  // ART_RUNTIME_MIRROR_ART_METHOD_H_
