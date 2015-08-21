/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef ART_RUNTIME_HANDLE_H_
#define ART_RUNTIME_HANDLE_H_

#include "base/casts.h"
#include "base/logging.h"
#include "base/macros.h"
#include "stack.h"

namespace art {

class Thread;

template<class T> class Handle;

// Handles are memory locations that contain GC roots. As the mirror::Object*s within a handle are
// GC visible then the GC may move the references within them, something that couldn't be done with
// a wrap pointer. Handles are generally allocated within HandleScopes. ConstHandle is a super-class
// of Handle and doesn't support assignment operations.
template<class T>
class ConstHandle {
 public:
  ConstHandle() : reference_(nullptr) {
  }

  ALWAYS_INLINE ConstHandle(const ConstHandle<T>& handle) : reference_(handle.reference_) {
  }

  ALWAYS_INLINE ConstHandle<T>& operator=(const ConstHandle<T>& handle) {
    reference_ = handle.reference_;
    return *this;
  }

  ALWAYS_INLINE explicit ConstHandle(StackReference<T>* reference) : reference_(reference) {
  }

  ALWAYS_INLINE T& operator*() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return *Get();
  }

  ALWAYS_INLINE T* operator->() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return Get();
  }

  ALWAYS_INLINE T* Get() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return reference_->AsMirrorPtr();
  }

  ALWAYS_INLINE jobject ToJObject() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    if (UNLIKELY(reference_->AsMirrorPtr() == nullptr)) {
      // Special case so that we work with NullHandles.
      return nullptr;
    }
    return reinterpret_cast<jobject>(reference_);
  }

 protected:
  StackReference<T>* reference_;

  template<typename S>
  explicit ConstHandle(StackReference<S>* reference)
      : reference_(reinterpret_cast<StackReference<T>*>(reference)) {
  }
  template<typename S>
  explicit ConstHandle(const ConstHandle<S>& handle)
      : reference_(reinterpret_cast<StackReference<T>*>(handle.reference_)) {
  }

  StackReference<T>* GetReference() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) ALWAYS_INLINE {
    return reference_;
  }
  ALWAYS_INLINE const StackReference<T>* GetReference() const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return reference_;
  }

 private:
  friend class BuildGenericJniFrameVisitor;
  template<class S> friend class ConstHandle;
  friend class HandleScope;
  template<class S> friend class HandleWrapper;
  template<size_t kNumReferences> friend class StackHandleScope;
};

// Handles that support assignment.
template<class T>
class Handle : public ConstHandle<T> {
 public:
  Handle() {
  }

  ALWAYS_INLINE Handle(const Handle<T>& handle) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      : ConstHandle<T>(handle.reference_) {
  }

  ALWAYS_INLINE Handle<T>& operator=(const Handle<T>& handle)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    ConstHandle<T>::operator=(handle);
    return *this;
  }

  ALWAYS_INLINE explicit Handle(StackReference<T>* reference)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      : ConstHandle<T>(reference) {
  }

  ALWAYS_INLINE T* Assign(T* reference) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    StackReference<T>* ref = ConstHandle<T>::GetReference();
    T* const old = ref->AsMirrorPtr();
    ref->Assign(reference);
    return old;
  }

  template<typename S>
  explicit Handle(const Handle<S>& handle) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      : ConstHandle<T>(handle) {
  }

 protected:
  template<typename S>
  explicit Handle(StackReference<S>* reference) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      : ConstHandle<T>(reference) {
  }

 private:
  friend class BuildGenericJniFrameVisitor;
  friend class HandleScope;
  template<class S> friend class HandleWrapper;
  template<size_t kNumReferences> friend class StackHandleScope;
};

// A special case of Handle that only holds references to null.
template<class T>
class NullHandle : public Handle<T> {
 public:
  NullHandle() : Handle<T>(&null_ref_) {
  }

 private:
  StackReference<T> null_ref_;
};

}  // namespace art

#endif  // ART_RUNTIME_HANDLE_H_
