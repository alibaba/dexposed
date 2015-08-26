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

#ifndef ART_RUNTIME_MIRROR_OBJECT_ARRAY_H_
#define ART_RUNTIME_MIRROR_OBJECT_ARRAY_H_

#include "array.h"

namespace art {
namespace mirror {

template<class T>
class MANAGED ObjectArray: public Array {
 public:
  // The size of Object[].class.
  static uint32_t ClassSize() {
    return Array::ClassSize();
  }

  static ObjectArray<T>* Alloc(Thread* self, Class* object_array_class, int32_t length,
                               gc::AllocatorType allocator_type)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static ObjectArray<T>* Alloc(Thread* self, Class* object_array_class, int32_t length)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  T* Get(int32_t i) ALWAYS_INLINE SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Returns true if the object can be stored into the array. If not, throws
  // an ArrayStoreException and returns false.
  // TODO fix thread safety analysis: should be SHARED_LOCKS_REQUIRED(Locks::mutator_lock_).
  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  bool CheckAssignable(T* object) NO_THREAD_SAFETY_ANALYSIS;

  void Set(int32_t i, T* object) ALWAYS_INLINE SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  // TODO fix thread safety analysis: should be SHARED_LOCKS_REQUIRED(Locks::mutator_lock_).
  template<bool kTransactionActive, bool kCheckTransaction = true,
      VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  void Set(int32_t i, T* object) ALWAYS_INLINE NO_THREAD_SAFETY_ANALYSIS;

  // Set element without bound and element type checks, to be used in limited
  // circumstances, such as during boot image writing.
  // TODO fix thread safety analysis broken by the use of template. This should be
  // SHARED_LOCKS_REQUIRED(Locks::mutator_lock_).
  template<bool kTransactionActive, bool kCheckTransaction = true,
      VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  void SetWithoutChecks(int32_t i, T* object) ALWAYS_INLINE NO_THREAD_SAFETY_ANALYSIS;
  // TODO fix thread safety analysis broken by the use of template. This should be
  // SHARED_LOCKS_REQUIRED(Locks::mutator_lock_).
  template<bool kTransactionActive, bool kCheckTransaction = true,
      VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  void SetWithoutChecksAndWriteBarrier(int32_t i, T* object) ALWAYS_INLINE
      NO_THREAD_SAFETY_ANALYSIS;

  T* GetWithoutChecks(int32_t i) ALWAYS_INLINE SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Copy src into this array (dealing with overlaps as memmove does) without assignability checks.
  void AssignableMemmove(int32_t dst_pos, ObjectArray<T>* src, int32_t src_pos,
                         int32_t count) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Copy src into this array assuming no overlap and without assignability checks.
  void AssignableMemcpy(int32_t dst_pos, ObjectArray<T>* src, int32_t src_pos,
                        int32_t count) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Copy src into this array with assignability checks.
  void AssignableCheckingMemcpy(int32_t dst_pos, ObjectArray<T>* src, int32_t src_pos,
                                int32_t count, bool throw_exception)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  ObjectArray<T>* CopyOf(Thread* self, int32_t new_length)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // TODO fix thread safety analysis broken by the use of template. This should be
  // SHARED_LOCKS_REQUIRED(Locks::mutator_lock_).
  template<const bool kVisitClass, typename Visitor>
  void VisitReferences(const Visitor& visitor) NO_THREAD_SAFETY_ANALYSIS;

  static MemberOffset OffsetOfElement(int32_t i);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectArray);
};

}  // namespace mirror
}  // namespace art

#endif  // ART_RUNTIME_MIRROR_OBJECT_ARRAY_H_
