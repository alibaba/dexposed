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

#ifndef ART_RUNTIME_MIRROR_OBJECT_INL_H_
#define ART_RUNTIME_MIRROR_OBJECT_INL_H_

#include "object.h"

#include "art_field.h"
#include "art_method.h"
#include "atomic.h"
#include "array-inl.h"
#include "class.h"
#include "lock_word-inl.h"
#include "monitor.h"
#include "object_array-inl.h"
#include "read_barrier-inl.h"
#include "runtime.h"
#include "reference.h"
#include "throwable.h"

namespace art {
namespace mirror {

inline uint32_t Object::ClassSize() {
  uint32_t vtable_entries = kVTableLength;
  return Class::ComputeClassSize(true, vtable_entries, 0, 0, 0);
}

template<VerifyObjectFlags kVerifyFlags, ReadBarrierOption kReadBarrierOption>
inline Class* Object::GetClass() {
  return GetFieldObject<Class, kVerifyFlags, kReadBarrierOption>(
      OFFSET_OF_OBJECT_MEMBER(Object, klass_));
}

template<VerifyObjectFlags kVerifyFlags>
inline void Object::SetClass(Class* new_klass) {
  // new_klass may be NULL prior to class linker initialization.
  // We don't mark the card as this occurs as part of object allocation. Not all objects have
  // backing cards, such as large objects.
  // We use non transactional version since we can't undo this write. We also disable checking as
  // we may run in transaction mode here.
  SetFieldObjectWithoutWriteBarrier<false, false,
      static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis)>(
      OFFSET_OF_OBJECT_MEMBER(Object, klass_), new_klass);
}

inline LockWord Object::GetLockWord(bool as_volatile) {
  if (as_volatile) {
    return LockWord(GetField32Volatile(OFFSET_OF_OBJECT_MEMBER(Object, monitor_)));
  }
  return LockWord(GetField32(OFFSET_OF_OBJECT_MEMBER(Object, monitor_)));
}

inline void Object::SetLockWord(LockWord new_val, bool as_volatile) {
  // Force use of non-transactional mode and do not check.
  if (as_volatile) {
    SetField32Volatile<false, false>(OFFSET_OF_OBJECT_MEMBER(Object, monitor_), new_val.GetValue());
  } else {
    SetField32<false, false>(OFFSET_OF_OBJECT_MEMBER(Object, monitor_), new_val.GetValue());
  }
}

inline bool Object::CasLockWordWeakSequentiallyConsistent(LockWord old_val, LockWord new_val) {
  // Force use of non-transactional mode and do not check.
  return CasFieldWeakSequentiallyConsistent32<false, false>(
      OFFSET_OF_OBJECT_MEMBER(Object, monitor_), old_val.GetValue(), new_val.GetValue());
}

inline bool Object::CasLockWordWeakRelaxed(LockWord old_val, LockWord new_val) {
  // Force use of non-transactional mode and do not check.
  return CasFieldWeakRelaxed32<false, false>(
      OFFSET_OF_OBJECT_MEMBER(Object, monitor_), old_val.GetValue(), new_val.GetValue());
}

inline uint32_t Object::GetLockOwnerThreadId() {
  return Monitor::GetLockOwnerThreadId(this);
}

inline mirror::Object* Object::MonitorEnter(Thread* self) {
  return Monitor::MonitorEnter(self, this);
}

inline bool Object::MonitorExit(Thread* self) {
  return Monitor::MonitorExit(self, this);
}

inline void Object::Notify(Thread* self) {
  Monitor::Notify(self, this);
}

inline void Object::NotifyAll(Thread* self) {
  Monitor::NotifyAll(self, this);
}

inline void Object::Wait(Thread* self) {
  Monitor::Wait(self, this, 0, 0, true, kWaiting);
}

inline void Object::Wait(Thread* self, int64_t ms, int32_t ns) {
  Monitor::Wait(self, this, ms, ns, true, kTimedWaiting);
}

inline Object* Object::GetReadBarrierPointer() {
#ifdef USE_BAKER_OR_BROOKS_READ_BARRIER
  DCHECK(kUseBakerOrBrooksReadBarrier);
  return GetFieldObject<Object, kVerifyNone, kWithoutReadBarrier>(
      OFFSET_OF_OBJECT_MEMBER(Object, x_rb_ptr_));
#else
  LOG(FATAL) << "Unreachable";
  return nullptr;
#endif
}

inline void Object::SetReadBarrierPointer(Object* rb_ptr) {
#ifdef USE_BAKER_OR_BROOKS_READ_BARRIER
  DCHECK(kUseBakerOrBrooksReadBarrier);
  // We don't mark the card as this occurs as part of object allocation. Not all objects have
  // backing cards, such as large objects.
  SetFieldObjectWithoutWriteBarrier<false, false, kVerifyNone>(
      OFFSET_OF_OBJECT_MEMBER(Object, x_rb_ptr_), rb_ptr);
#else
  LOG(FATAL) << "Unreachable";
#endif
}

inline bool Object::AtomicSetReadBarrierPointer(Object* expected_rb_ptr, Object* rb_ptr) {
#ifdef USE_BAKER_OR_BROOKS_READ_BARRIER
  DCHECK(kUseBakerOrBrooksReadBarrier);
  MemberOffset offset = OFFSET_OF_OBJECT_MEMBER(Object, x_rb_ptr_);
  byte* raw_addr = reinterpret_cast<byte*>(this) + offset.SizeValue();
  Atomic<uint32_t>* atomic_rb_ptr = reinterpret_cast<Atomic<uint32_t>*>(raw_addr);
  HeapReference<Object> expected_ref(HeapReference<Object>::FromMirrorPtr(expected_rb_ptr));
  HeapReference<Object> new_ref(HeapReference<Object>::FromMirrorPtr(rb_ptr));
  do {
    if (UNLIKELY(atomic_rb_ptr->LoadRelaxed() != expected_ref.reference_)) {
      // Lost the race.
      return false;
    }
  } while (!atomic_rb_ptr->CompareExchangeWeakSequentiallyConsistent(expected_ref.reference_,
                                                                     new_ref.reference_));
  DCHECK_EQ(new_ref.reference_, atomic_rb_ptr->LoadRelaxed());
  return true;
#else
  LOG(FATAL) << "Unreachable";
  return false;
#endif
}

inline void Object::AssertReadBarrierPointer() const {
  if (kUseBakerReadBarrier) {
    Object* obj = const_cast<Object*>(this);
    DCHECK(obj->GetReadBarrierPointer() == nullptr)
        << "Bad Baker pointer: obj=" << reinterpret_cast<void*>(obj)
        << " ptr=" << reinterpret_cast<void*>(obj->GetReadBarrierPointer());
  } else if (kUseBrooksReadBarrier) {
    Object* obj = const_cast<Object*>(this);
    DCHECK_EQ(obj, obj->GetReadBarrierPointer())
        << "Bad Brooks pointer: obj=" << reinterpret_cast<void*>(obj)
        << " ptr=" << reinterpret_cast<void*>(obj->GetReadBarrierPointer());
  } else {
    LOG(FATAL) << "Unreachable";
  }
}

template<VerifyObjectFlags kVerifyFlags>
inline bool Object::VerifierInstanceOf(Class* klass) {
  DCHECK(klass != NULL);
  DCHECK(GetClass<kVerifyFlags>() != NULL);
  return klass->IsInterface() || InstanceOf(klass);
}

template<VerifyObjectFlags kVerifyFlags>
inline bool Object::InstanceOf(Class* klass) {
  DCHECK(klass != NULL);
  DCHECK(GetClass<kVerifyNone>() != NULL);
  return klass->IsAssignableFrom(GetClass<kVerifyFlags>());
}

template<VerifyObjectFlags kVerifyFlags, ReadBarrierOption kReadBarrierOption>
inline bool Object::IsClass() {
  Class* java_lang_Class = GetClass<kVerifyFlags, kReadBarrierOption>()->
      template GetClass<kVerifyFlags, kReadBarrierOption>();
  return GetClass<static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis),
      kReadBarrierOption>() == java_lang_Class;
}

template<VerifyObjectFlags kVerifyFlags, ReadBarrierOption kReadBarrierOption>
inline Class* Object::AsClass() {
  DCHECK((IsClass<kVerifyFlags, kReadBarrierOption>()));
  return down_cast<Class*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline bool Object::IsObjectArray() {
  constexpr auto kNewFlags = static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis);
  return IsArrayInstance<kVerifyFlags>() &&
      !GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitive();
}

template<class T, VerifyObjectFlags kVerifyFlags>
inline ObjectArray<T>* Object::AsObjectArray() {
  DCHECK(IsObjectArray<kVerifyFlags>());
  return down_cast<ObjectArray<T>*>(this);
}

template<VerifyObjectFlags kVerifyFlags, ReadBarrierOption kReadBarrierOption>
inline bool Object::IsArrayInstance() {
  return GetClass<kVerifyFlags, kReadBarrierOption>()->
      template IsArrayClass<kVerifyFlags, kReadBarrierOption>();
}

template<VerifyObjectFlags kVerifyFlags, ReadBarrierOption kReadBarrierOption>
inline bool Object::IsArtField() {
  return GetClass<kVerifyFlags, kReadBarrierOption>()->
      template IsArtFieldClass<kReadBarrierOption>();
}

template<VerifyObjectFlags kVerifyFlags>
inline ArtField* Object::AsArtField() {
  DCHECK(IsArtField<kVerifyFlags>());
  return down_cast<ArtField*>(this);
}

template<VerifyObjectFlags kVerifyFlags, ReadBarrierOption kReadBarrierOption>
inline bool Object::IsArtMethod() {
  return GetClass<kVerifyFlags, kReadBarrierOption>()->
      template IsArtMethodClass<kReadBarrierOption>();
}

template<VerifyObjectFlags kVerifyFlags>
inline ArtMethod* Object::AsArtMethod() {
  DCHECK(IsArtMethod<kVerifyFlags>());
  return down_cast<ArtMethod*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline bool Object::IsReferenceInstance() {
  return GetClass<kVerifyFlags>()->IsTypeOfReferenceClass();
}

template<VerifyObjectFlags kVerifyFlags>
inline Reference* Object::AsReference() {
  DCHECK(IsReferenceInstance<kVerifyFlags>());
  return down_cast<Reference*>(this);
}

template<VerifyObjectFlags kVerifyFlags, ReadBarrierOption kReadBarrierOption>
inline Array* Object::AsArray() {
  DCHECK((IsArrayInstance<kVerifyFlags, kReadBarrierOption>()));
  return down_cast<Array*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline BooleanArray* Object::AsBooleanArray() {
  constexpr auto kNewFlags = static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis);
  DCHECK(GetClass<kVerifyFlags>()->IsArrayClass());
  DCHECK(GetClass<kNewFlags>()->GetComponentType()->IsPrimitiveBoolean());
  return down_cast<BooleanArray*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline ByteArray* Object::AsByteArray() {
  static const VerifyObjectFlags kNewFlags = static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis);
  DCHECK(GetClass<kVerifyFlags>()->IsArrayClass());
  DCHECK(GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitiveByte());
  return down_cast<ByteArray*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline ByteArray* Object::AsByteSizedArray() {
  constexpr VerifyObjectFlags kNewFlags = static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis);
  DCHECK(GetClass<kVerifyFlags>()->IsArrayClass());
  DCHECK(GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitiveByte() ||
         GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitiveBoolean());
  return down_cast<ByteArray*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline CharArray* Object::AsCharArray() {
  constexpr auto kNewFlags = static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis);
  DCHECK(GetClass<kVerifyFlags>()->IsArrayClass());
  DCHECK(GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitiveChar());
  return down_cast<CharArray*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline ShortArray* Object::AsShortArray() {
  constexpr auto kNewFlags = static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis);
  DCHECK(GetClass<kVerifyFlags>()->IsArrayClass());
  DCHECK(GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitiveShort());
  return down_cast<ShortArray*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline ShortArray* Object::AsShortSizedArray() {
  constexpr auto kNewFlags = static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis);
  DCHECK(GetClass<kVerifyFlags>()->IsArrayClass());
  DCHECK(GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitiveShort() ||
         GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitiveChar());
  return down_cast<ShortArray*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline IntArray* Object::AsIntArray() {
  constexpr auto kNewFlags = static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis);
  DCHECK(GetClass<kVerifyFlags>()->IsArrayClass());
  DCHECK(GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitiveInt() ||
         GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitiveFloat());
  return down_cast<IntArray*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline LongArray* Object::AsLongArray() {
  constexpr auto kNewFlags = static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis);
  DCHECK(GetClass<kVerifyFlags>()->IsArrayClass());
  DCHECK(GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitiveLong() ||
         GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitiveDouble());
  return down_cast<LongArray*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline FloatArray* Object::AsFloatArray() {
  constexpr auto kNewFlags = static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis);
  DCHECK(GetClass<kVerifyFlags>()->IsArrayClass());
  DCHECK(GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitiveFloat());
  return down_cast<FloatArray*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline DoubleArray* Object::AsDoubleArray() {
  constexpr auto kNewFlags = static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis);
  DCHECK(GetClass<kVerifyFlags>()->IsArrayClass());
  DCHECK(GetClass<kNewFlags>()->template GetComponentType<kNewFlags>()->IsPrimitiveDouble());
  return down_cast<DoubleArray*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline String* Object::AsString() {
  DCHECK(GetClass<kVerifyFlags>()->IsStringClass());
  return down_cast<String*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline Throwable* Object::AsThrowable() {
  DCHECK(GetClass<kVerifyFlags>()->IsThrowableClass());
  return down_cast<Throwable*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline bool Object::IsWeakReferenceInstance() {
  return GetClass<kVerifyFlags>()->IsWeakReferenceClass();
}

template<VerifyObjectFlags kVerifyFlags>
inline bool Object::IsSoftReferenceInstance() {
  return GetClass<kVerifyFlags>()->IsSoftReferenceClass();
}

template<VerifyObjectFlags kVerifyFlags>
inline bool Object::IsFinalizerReferenceInstance() {
  return GetClass<kVerifyFlags>()->IsFinalizerReferenceClass();
}

template<VerifyObjectFlags kVerifyFlags>
inline FinalizerReference* Object::AsFinalizerReference() {
  DCHECK(IsFinalizerReferenceInstance<kVerifyFlags>());
  return down_cast<FinalizerReference*>(this);
}

template<VerifyObjectFlags kVerifyFlags>
inline bool Object::IsPhantomReferenceInstance() {
  return GetClass<kVerifyFlags>()->IsPhantomReferenceClass();
}

template<VerifyObjectFlags kVerifyFlags, ReadBarrierOption kReadBarrierOption>
inline size_t Object::SizeOf() {
  size_t result;
  constexpr auto kNewFlags = static_cast<VerifyObjectFlags>(kVerifyFlags & ~kVerifyThis);
  if (IsArrayInstance<kVerifyFlags, kReadBarrierOption>()) {
    result = AsArray<kNewFlags, kReadBarrierOption>()->
        template SizeOf<kNewFlags, kReadBarrierOption>();
  } else if (IsClass<kNewFlags, kReadBarrierOption>()) {
    result = AsClass<kNewFlags, kReadBarrierOption>()->
        template SizeOf<kNewFlags, kReadBarrierOption>();
  } else {
    result = GetClass<kNewFlags, kReadBarrierOption>()->
        template GetObjectSize<kNewFlags, kReadBarrierOption>();
  }
  DCHECK_GE(result, sizeof(Object))
      << " class=" << PrettyTypeOf(GetClass<kNewFlags, kReadBarrierOption>());
  DCHECK(!(IsArtField<kNewFlags, kReadBarrierOption>())  || result == sizeof(ArtField));
  DCHECK(!(IsArtMethod<kNewFlags, kReadBarrierOption>()) || result == sizeof(ArtMethod));
  return result;
}

template<VerifyObjectFlags kVerifyFlags, bool kIsVolatile>
inline int32_t Object::GetField32(MemberOffset field_offset) {
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  const byte* raw_addr = reinterpret_cast<const byte*>(this) + field_offset.Int32Value();
  const int32_t* word_addr = reinterpret_cast<const int32_t*>(raw_addr);
  if (UNLIKELY(kIsVolatile)) {
    return reinterpret_cast<const Atomic<int32_t>*>(word_addr)->LoadSequentiallyConsistent();
  } else {
    return reinterpret_cast<const Atomic<int32_t>*>(word_addr)->LoadJavaData();
  }
}

template<VerifyObjectFlags kVerifyFlags>
inline int32_t Object::GetField32Volatile(MemberOffset field_offset) {
  return GetField32<kVerifyFlags, true>(field_offset);
}

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags,
    bool kIsVolatile>
inline void Object::SetField32(MemberOffset field_offset, int32_t new_value) {
  if (kCheckTransaction) {
    DCHECK_EQ(kTransactionActive, Runtime::Current()->IsActiveTransaction());
  }
  if (kTransactionActive) {
    Runtime::Current()->RecordWriteField32(this, field_offset,
                                           GetField32<kVerifyFlags, kIsVolatile>(field_offset),
                                           kIsVolatile);
  }
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  byte* raw_addr = reinterpret_cast<byte*>(this) + field_offset.Int32Value();
  int32_t* word_addr = reinterpret_cast<int32_t*>(raw_addr);
  if (kIsVolatile) {
    reinterpret_cast<Atomic<int32_t>*>(word_addr)->StoreSequentiallyConsistent(new_value);
  } else {
    reinterpret_cast<Atomic<int32_t>*>(word_addr)->StoreJavaData(new_value);
  }
}

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags>
inline void Object::SetField32Volatile(MemberOffset field_offset, int32_t new_value) {
  SetField32<kTransactionActive, kCheckTransaction, kVerifyFlags, true>(field_offset, new_value);
}

// TODO: Pass memory_order_ and strong/weak as arguments to avoid code duplication?

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags>
inline bool Object::CasFieldWeakSequentiallyConsistent32(MemberOffset field_offset,
                                                         int32_t old_value, int32_t new_value) {
  if (kCheckTransaction) {
    DCHECK_EQ(kTransactionActive, Runtime::Current()->IsActiveTransaction());
  }
  if (kTransactionActive) {
    Runtime::Current()->RecordWriteField32(this, field_offset, old_value, true);
  }
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  byte* raw_addr = reinterpret_cast<byte*>(this) + field_offset.Int32Value();
  AtomicInteger* atomic_addr = reinterpret_cast<AtomicInteger*>(raw_addr);

  return atomic_addr->CompareExchangeWeakSequentiallyConsistent(old_value, new_value);
}

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags>
inline bool Object::CasFieldWeakRelaxed32(MemberOffset field_offset,
                                          int32_t old_value, int32_t new_value) {
  if (kCheckTransaction) {
    DCHECK_EQ(kTransactionActive, Runtime::Current()->IsActiveTransaction());
  }
  if (kTransactionActive) {
    Runtime::Current()->RecordWriteField32(this, field_offset, old_value, true);
  }
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  byte* raw_addr = reinterpret_cast<byte*>(this) + field_offset.Int32Value();
  AtomicInteger* atomic_addr = reinterpret_cast<AtomicInteger*>(raw_addr);

  return atomic_addr->CompareExchangeWeakRelaxed(old_value, new_value);
}

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags>
inline bool Object::CasFieldStrongSequentiallyConsistent32(MemberOffset field_offset,
                                                           int32_t old_value, int32_t new_value) {
  if (kCheckTransaction) {
    DCHECK_EQ(kTransactionActive, Runtime::Current()->IsActiveTransaction());
  }
  if (kTransactionActive) {
    Runtime::Current()->RecordWriteField32(this, field_offset, old_value, true);
  }
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  byte* raw_addr = reinterpret_cast<byte*>(this) + field_offset.Int32Value();
  AtomicInteger* atomic_addr = reinterpret_cast<AtomicInteger*>(raw_addr);

  return atomic_addr->CompareExchangeStrongSequentiallyConsistent(old_value, new_value);
}

template<VerifyObjectFlags kVerifyFlags, bool kIsVolatile>
inline int64_t Object::GetField64(MemberOffset field_offset) {
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  const byte* raw_addr = reinterpret_cast<const byte*>(this) + field_offset.Int32Value();
  const int64_t* addr = reinterpret_cast<const int64_t*>(raw_addr);
  if (kIsVolatile) {
    return reinterpret_cast<const Atomic<int64_t>*>(addr)->LoadSequentiallyConsistent();
  } else {
    return reinterpret_cast<const Atomic<int64_t>*>(addr)->LoadJavaData();
  }
}

template<VerifyObjectFlags kVerifyFlags>
inline int64_t Object::GetField64Volatile(MemberOffset field_offset) {
  return GetField64<kVerifyFlags, true>(field_offset);
}

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags,
    bool kIsVolatile>
inline void Object::SetField64(MemberOffset field_offset, int64_t new_value) {
  if (kCheckTransaction) {
    DCHECK_EQ(kTransactionActive, Runtime::Current()->IsActiveTransaction());
  }
  if (kTransactionActive) {
    Runtime::Current()->RecordWriteField64(this, field_offset,
                                           GetField64<kVerifyFlags, kIsVolatile>(field_offset),
                                           kIsVolatile);
  }
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  byte* raw_addr = reinterpret_cast<byte*>(this) + field_offset.Int32Value();
  int64_t* addr = reinterpret_cast<int64_t*>(raw_addr);
  if (kIsVolatile) {
    reinterpret_cast<Atomic<int64_t>*>(addr)->StoreSequentiallyConsistent(new_value);
  } else {
    reinterpret_cast<Atomic<int64_t>*>(addr)->StoreJavaData(new_value);
  }
}

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags>
inline void Object::SetField64Volatile(MemberOffset field_offset, int64_t new_value) {
  return SetField64<kTransactionActive, kCheckTransaction, kVerifyFlags, true>(field_offset,
                                                                               new_value);
}

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags>
inline bool Object::CasFieldWeakSequentiallyConsistent64(MemberOffset field_offset,
                                                         int64_t old_value, int64_t new_value) {
  if (kCheckTransaction) {
    DCHECK_EQ(kTransactionActive, Runtime::Current()->IsActiveTransaction());
  }
  if (kTransactionActive) {
    Runtime::Current()->RecordWriteField64(this, field_offset, old_value, true);
  }
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  byte* raw_addr = reinterpret_cast<byte*>(this) + field_offset.Int32Value();
  Atomic<int64_t>* atomic_addr = reinterpret_cast<Atomic<int64_t>*>(raw_addr);
  return atomic_addr->CompareExchangeWeakSequentiallyConsistent(old_value, new_value);
}

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags>
inline bool Object::CasFieldStrongSequentiallyConsistent64(MemberOffset field_offset,
                                                           int64_t old_value, int64_t new_value) {
  if (kCheckTransaction) {
    DCHECK_EQ(kTransactionActive, Runtime::Current()->IsActiveTransaction());
  }
  if (kTransactionActive) {
    Runtime::Current()->RecordWriteField64(this, field_offset, old_value, true);
  }
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  byte* raw_addr = reinterpret_cast<byte*>(this) + field_offset.Int32Value();
  Atomic<int64_t>* atomic_addr = reinterpret_cast<Atomic<int64_t>*>(raw_addr);
  return atomic_addr->CompareExchangeStrongSequentiallyConsistent(old_value, new_value);
}

template<class T, VerifyObjectFlags kVerifyFlags, ReadBarrierOption kReadBarrierOption,
         bool kIsVolatile>
inline T* Object::GetFieldObject(MemberOffset field_offset) {
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  byte* raw_addr = reinterpret_cast<byte*>(this) + field_offset.Int32Value();
  HeapReference<T>* objref_addr = reinterpret_cast<HeapReference<T>*>(raw_addr);
  T* result = ReadBarrier::Barrier<T, kReadBarrierOption>(this, field_offset, objref_addr);
  if (kIsVolatile) {
    // TODO: Refactor to use a SequentiallyConsistent load instead.
    QuasiAtomic::ThreadFenceAcquire();  // Ensure visibility of operations preceding store.
  }
  if (kVerifyFlags & kVerifyReads) {
    VerifyObject(result);
  }
  return result;
}

template<class T, VerifyObjectFlags kVerifyFlags, ReadBarrierOption kReadBarrierOption>
inline T* Object::GetFieldObjectVolatile(MemberOffset field_offset) {
  return GetFieldObject<T, kVerifyFlags, kReadBarrierOption, true>(field_offset);
}

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags,
    bool kIsVolatile>
inline void Object::SetFieldObjectWithoutWriteBarrier(MemberOffset field_offset,
                                                      Object* new_value) {
  if (kCheckTransaction) {
    DCHECK_EQ(kTransactionActive, Runtime::Current()->IsActiveTransaction());
  }
  if (kTransactionActive) {
    mirror::Object* obj;
    if (kIsVolatile) {
      obj = GetFieldObjectVolatile<Object>(field_offset);
    } else {
      obj = GetFieldObject<Object>(field_offset);
    }
    Runtime::Current()->RecordWriteFieldReference(this, field_offset, obj, true);
  }
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  if (kVerifyFlags & kVerifyWrites) {
    VerifyObject(new_value);
  }
  byte* raw_addr = reinterpret_cast<byte*>(this) + field_offset.Int32Value();
  HeapReference<Object>* objref_addr = reinterpret_cast<HeapReference<Object>*>(raw_addr);
  if (kIsVolatile) {
    // TODO: Refactor to use a SequentiallyConsistent store instead.
    QuasiAtomic::ThreadFenceRelease();  // Ensure that prior accesses are visible before store.
    objref_addr->Assign(new_value);
    QuasiAtomic::ThreadFenceSequentiallyConsistent();
                                // Ensure this store occurs before any volatile loads.
  } else {
    objref_addr->Assign(new_value);
  }
}

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags,
    bool kIsVolatile>
inline void Object::SetFieldObject(MemberOffset field_offset, Object* new_value) {
  SetFieldObjectWithoutWriteBarrier<kTransactionActive, kCheckTransaction, kVerifyFlags,
      kIsVolatile>(field_offset, new_value);
  if (new_value != nullptr) {
    Runtime::Current()->GetHeap()->WriteBarrierField(this, field_offset, new_value);
    // TODO: Check field assignment could theoretically cause thread suspension, TODO: fix this.
    CheckFieldAssignment(field_offset, new_value);
  }
}

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags>
inline void Object::SetFieldObjectVolatile(MemberOffset field_offset, Object* new_value) {
  SetFieldObject<kTransactionActive, kCheckTransaction, kVerifyFlags, true>(field_offset,
                                                                            new_value);
}

template <VerifyObjectFlags kVerifyFlags>
inline HeapReference<Object>* Object::GetFieldObjectReferenceAddr(MemberOffset field_offset) {
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  return reinterpret_cast<HeapReference<Object>*>(reinterpret_cast<byte*>(this) +
      field_offset.Int32Value());
}

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags>
inline bool Object::CasFieldWeakSequentiallyConsistentObject(MemberOffset field_offset,
                                                             Object* old_value, Object* new_value) {
  if (kCheckTransaction) {
    DCHECK_EQ(kTransactionActive, Runtime::Current()->IsActiveTransaction());
  }
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  if (kVerifyFlags & kVerifyWrites) {
    VerifyObject(new_value);
  }
  if (kVerifyFlags & kVerifyReads) {
    VerifyObject(old_value);
  }
  if (kTransactionActive) {
    Runtime::Current()->RecordWriteFieldReference(this, field_offset, old_value, true);
  }
  HeapReference<Object> old_ref(HeapReference<Object>::FromMirrorPtr(old_value));
  HeapReference<Object> new_ref(HeapReference<Object>::FromMirrorPtr(new_value));
  byte* raw_addr = reinterpret_cast<byte*>(this) + field_offset.Int32Value();
  Atomic<uint32_t>* atomic_addr = reinterpret_cast<Atomic<uint32_t>*>(raw_addr);

  bool success = atomic_addr->CompareExchangeWeakSequentiallyConsistent(old_ref.reference_,
                                                                        new_ref.reference_);

  if (success) {
    Runtime::Current()->GetHeap()->WriteBarrierField(this, field_offset, new_value);
  }
  return success;
}

template<bool kTransactionActive, bool kCheckTransaction, VerifyObjectFlags kVerifyFlags>
inline bool Object::CasFieldStrongSequentiallyConsistentObject(MemberOffset field_offset,
                                                             Object* old_value, Object* new_value) {
  if (kCheckTransaction) {
    DCHECK_EQ(kTransactionActive, Runtime::Current()->IsActiveTransaction());
  }
  if (kVerifyFlags & kVerifyThis) {
    VerifyObject(this);
  }
  if (kVerifyFlags & kVerifyWrites) {
    VerifyObject(new_value);
  }
  if (kVerifyFlags & kVerifyReads) {
    VerifyObject(old_value);
  }
  if (kTransactionActive) {
    Runtime::Current()->RecordWriteFieldReference(this, field_offset, old_value, true);
  }
  HeapReference<Object> old_ref(HeapReference<Object>::FromMirrorPtr(old_value));
  HeapReference<Object> new_ref(HeapReference<Object>::FromMirrorPtr(new_value));
  byte* raw_addr = reinterpret_cast<byte*>(this) + field_offset.Int32Value();
  Atomic<uint32_t>* atomic_addr = reinterpret_cast<Atomic<uint32_t>*>(raw_addr);

  bool success = atomic_addr->CompareExchangeStrongSequentiallyConsistent(old_ref.reference_,
                                                                          new_ref.reference_);

  if (success) {
    Runtime::Current()->GetHeap()->WriteBarrierField(this, field_offset, new_value);
  }
  return success;
}

template<bool kVisitClass, bool kIsStatic, typename Visitor>
inline void Object::VisitFieldsReferences(uint32_t ref_offsets, const Visitor& visitor) {
  if (LIKELY(ref_offsets != CLASS_WALK_SUPER)) {
    if (!kVisitClass) {
     // Mask out the class from the reference offsets.
      ref_offsets ^= kWordHighBitMask;
    }
    DCHECK_EQ(ClassOffset().Uint32Value(), 0U);
    // Found a reference offset bitmap. Visit the specified offsets.
    while (ref_offsets != 0) {
      size_t right_shift = CLZ(ref_offsets);
      MemberOffset field_offset = CLASS_OFFSET_FROM_CLZ(right_shift);
      visitor(this, field_offset, kIsStatic);
      ref_offsets &= ~(CLASS_HIGH_BIT >> right_shift);
    }
  } else {
    // There is no reference offset bitmap.  In the non-static case, walk up the class
    // inheritance hierarchy and find reference offsets the hard way. In the static case, just
    // consider this class.
    for (mirror::Class* klass = kIsStatic ? AsClass() : GetClass(); klass != nullptr;
        klass = kIsStatic ? nullptr : klass->GetSuperClass()) {
      size_t num_reference_fields =
          kIsStatic ? klass->NumReferenceStaticFields() : klass->NumReferenceInstanceFields();
      for (size_t i = 0; i < num_reference_fields; ++i) {
        mirror::ArtField* field = kIsStatic ? klass->GetStaticField(i) : klass->GetInstanceField(i);
        MemberOffset field_offset = field->GetOffset();
        // TODO: Do a simpler check?
        if (kVisitClass || field_offset.Uint32Value() != ClassOffset().Uint32Value()) {
          visitor(this, field_offset, kIsStatic);
        }
      }
    }
  }
}

template<bool kVisitClass, typename Visitor>
inline void Object::VisitInstanceFieldsReferences(mirror::Class* klass, const Visitor& visitor) {
  VisitFieldsReferences<kVisitClass, false>(
      klass->GetReferenceInstanceOffsets<kVerifyNone>(), visitor);
}

template<bool kVisitClass, typename Visitor>
inline void Object::VisitStaticFieldsReferences(mirror::Class* klass, const Visitor& visitor) {
  DCHECK(!klass->IsTemp());
  klass->VisitFieldsReferences<kVisitClass, true>(
      klass->GetReferenceStaticOffsets<kVerifyNone>(), visitor);
}

template <const bool kVisitClass, VerifyObjectFlags kVerifyFlags, typename Visitor,
    typename JavaLangRefVisitor>
inline void Object::VisitReferences(const Visitor& visitor,
                                    const JavaLangRefVisitor& ref_visitor) {
  mirror::Class* klass = GetClass<kVerifyFlags>();
  if (klass == Class::GetJavaLangClass()) {
    AsClass<kVerifyNone>()->VisitReferences<kVisitClass>(klass, visitor);
  } else if (klass->IsArrayClass()) {
    if (klass->IsObjectArrayClass<kVerifyNone>()) {
      AsObjectArray<mirror::Object, kVerifyNone>()->VisitReferences<kVisitClass>(visitor);
    } else if (kVisitClass) {
      visitor(this, ClassOffset(), false);
    }
  } else {
    DCHECK(!klass->IsVariableSize());
    VisitInstanceFieldsReferences<kVisitClass>(klass, visitor);
    if (UNLIKELY(klass->IsTypeOfReferenceClass<kVerifyNone>())) {
      ref_visitor(klass, AsReference());
    }
  }
}

}  // namespace mirror
}  // namespace art

#endif  // ART_RUNTIME_MIRROR_OBJECT_INL_H_
