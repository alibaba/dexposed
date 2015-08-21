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

#ifndef ART_RUNTIME_MIRROR_ARRAY_H_
#define ART_RUNTIME_MIRROR_ARRAY_H_

#include "gc_root.h"
#include "gc/allocator_type.h"
#include "object.h"
#include "object_callbacks.h"

namespace art {

template<class T> class Handle;

namespace mirror {

class MANAGED Array : public Object {
 public:
  // The size of a java.lang.Class representing an array.
  static uint32_t ClassSize();

  // Allocates an array with the given properties, if fill_usable is true the array will be of at
  // least component_count size, however, if there's usable space at the end of the allocation the
  // array will fill it.
  template <bool kIsInstrumented>
  static Array* Alloc(Thread* self, Class* array_class, int32_t component_count,
                      size_t component_size, gc::AllocatorType allocator_type,
                      bool fill_usable = false)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static Array* CreateMultiArray(Thread* self, Handle<Class> element_class,
                                 Handle<IntArray> dimensions)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags,
           ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  size_t SizeOf() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  int32_t GetLength() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return GetField32<kVerifyFlags>(OFFSET_OF_OBJECT_MEMBER(Array, length_));
  }

  void SetLength(int32_t length) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    CHECK_GE(length, 0);
    // We use non transactional version since we can't undo this write. We also disable checking
    // since it would fail during a transaction.
    SetField32<false, false, kVerifyNone>(OFFSET_OF_OBJECT_MEMBER(Array, length_), length);
  }

  static MemberOffset LengthOffset() {
    return OFFSET_OF_OBJECT_MEMBER(Array, length_);
  }

  static MemberOffset DataOffset(size_t component_size) {
    if (component_size != sizeof(int64_t)) {
      return OFFSET_OF_OBJECT_MEMBER(Array, first_element_);
    } else {
      // Align longs and doubles.
      return MemberOffset(OFFSETOF_MEMBER(Array, first_element_) + 4);
    }
  }

  void* GetRawData(size_t component_size, int32_t index)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    intptr_t data = reinterpret_cast<intptr_t>(this) + DataOffset(component_size).Int32Value() +
        + (index * component_size);
    return reinterpret_cast<void*>(data);
  }

  const void* GetRawData(size_t component_size, int32_t index) const {
    intptr_t data = reinterpret_cast<intptr_t>(this) + DataOffset(component_size).Int32Value() +
        + (index * component_size);
    return reinterpret_cast<void*>(data);
  }

  // Returns true if the index is valid. If not, throws an ArrayIndexOutOfBoundsException and
  // returns false.
  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  bool CheckIsValidIndex(int32_t index) ALWAYS_INLINE SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

 protected:
  void ThrowArrayStoreException(Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

 private:
  void ThrowArrayIndexOutOfBoundsException(int32_t index)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // The number of array elements.
  int32_t length_;
  // Marker for the data (used by generated code)
  uint32_t first_element_[0];

  DISALLOW_IMPLICIT_CONSTRUCTORS(Array);
};

template<typename T>
class MANAGED PrimitiveArray : public Array {
 public:
  typedef T ElementType;

  static PrimitiveArray<T>* Alloc(Thread* self, size_t length)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const T* GetData() const ALWAYS_INLINE  SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return reinterpret_cast<const T*>(GetRawData(sizeof(T), 0));
  }

  T* GetData() ALWAYS_INLINE SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return reinterpret_cast<T*>(GetRawData(sizeof(T), 0));
  }

  T Get(int32_t i) ALWAYS_INLINE SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  T GetWithoutChecks(int32_t i) ALWAYS_INLINE SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK(CheckIsValidIndex(i));
    return GetData()[i];
  }

  void Set(int32_t i, T value) ALWAYS_INLINE SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // TODO fix thread safety analysis broken by the use of template. This should be
  // SHARED_LOCKS_REQUIRED(Locks::mutator_lock_).
  template<bool kTransactionActive, bool kCheckTransaction = true>
  void Set(int32_t i, T value) ALWAYS_INLINE NO_THREAD_SAFETY_ANALYSIS;

  // TODO fix thread safety analysis broken by the use of template. This should be
  // SHARED_LOCKS_REQUIRED(Locks::mutator_lock_).
  template<bool kTransactionActive, bool kCheckTransaction = true>
  void SetWithoutChecks(int32_t i, T value) ALWAYS_INLINE NO_THREAD_SAFETY_ANALYSIS;

  /*
   * Works like memmove(), except we guarantee not to allow tearing of array values (ie using
   * smaller than element size copies). Arguments are assumed to be within the bounds of the array
   * and the arrays non-null.
   */
  void Memmove(int32_t dst_pos, PrimitiveArray<T>* src, int32_t src_pos, int32_t count)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  /*
   * Works like memcpy(), except we guarantee not to allow tearing of array values (ie using
   * smaller than element size copies). Arguments are assumed to be within the bounds of the array
   * and the arrays non-null.
   */
  void Memcpy(int32_t dst_pos, PrimitiveArray<T>* src, int32_t src_pos, int32_t count)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static void SetArrayClass(Class* array_class) {
    CHECK(array_class_.IsNull());
    CHECK(array_class != nullptr);
    array_class_ = GcRoot<Class>(array_class);
  }

  static Class* GetArrayClass() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK(!array_class_.IsNull());
    return array_class_.Read();
  }

  static void ResetArrayClass() {
    CHECK(!array_class_.IsNull());
    array_class_ = GcRoot<Class>(nullptr);
  }

  static void VisitRoots(RootCallback* callback, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

 private:
  static GcRoot<Class> array_class_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(PrimitiveArray);
};

}  // namespace mirror
}  // namespace art

#endif  // ART_RUNTIME_MIRROR_ARRAY_H_
