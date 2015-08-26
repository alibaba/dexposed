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

#ifndef ART_RUNTIME_HANDLE_SCOPE_H_
#define ART_RUNTIME_HANDLE_SCOPE_H_

#include "base/logging.h"
#include "base/macros.h"
#include "handle.h"
#include "stack.h"
#include "utils.h"

namespace art {
namespace mirror {
class Object;
}

class Thread;

// HandleScopes are scoped objects containing a number of Handles. They are used to allocate
// handles, for these handles (and the objects contained within them) to be visible/roots for the
// GC. It is most common to stack allocate HandleScopes using StackHandleScope.
class PACKED(4) HandleScope {
 public:
  ~HandleScope() {}

  // Number of references contained within this handle scope.
  uint32_t NumberOfReferences() const {
    return number_of_references_;
  }

  // We have versions with and without explicit pointer size of the following. The first two are
  // used at runtime, so OFFSETOF_MEMBER computes the right offsets automatically. The last one
  // takes the pointer size explicitly so that at compile time we can cross-compile correctly.

  // Returns the size of a HandleScope containing num_references handles.
  static size_t SizeOf(uint32_t num_references) {
    size_t header_size = sizeof(HandleScope);
    size_t data_size = sizeof(StackReference<mirror::Object>) * num_references;
    return header_size + data_size;
  }

  // Returns the size of a HandleScope containing num_references handles.
  static size_t SizeOf(size_t pointer_size, uint32_t num_references) {
    // Assume that the layout is packed.
    size_t header_size = pointer_size + sizeof(number_of_references_);
    size_t data_size = sizeof(StackReference<mirror::Object>) * num_references;
    return header_size + data_size;
  }

  // Link to previous HandleScope or null.
  HandleScope* GetLink() const {
    return link_;
  }

  void SetLink(HandleScope* link) {
    DCHECK_NE(this, link);
    link_ = link;
  }

  // Sets the number_of_references_ field for constructing tables out of raw memory. Warning: will
  // not resize anything.
  void SetNumberOfReferences(uint32_t num_references) {
    number_of_references_ = num_references;
  }

  mirror::Object* GetReference(size_t i) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      ALWAYS_INLINE {
    DCHECK_LT(i, number_of_references_);
    return references_[i].AsMirrorPtr();
  }

  Handle<mirror::Object> GetHandle(size_t i) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      ALWAYS_INLINE {
    DCHECK_LT(i, number_of_references_);
    return Handle<mirror::Object>(&references_[i]);
  }

  void SetReference(size_t i, mirror::Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      ALWAYS_INLINE {
    DCHECK_LT(i, number_of_references_);
    references_[i].Assign(object);
  }

  bool Contains(StackReference<mirror::Object>* handle_scope_entry) const {
    // A HandleScope should always contain something. One created by the
    // jni_compiler should have a jobject/jclass as a native method is
    // passed in a this pointer or a class
    DCHECK_GT(number_of_references_, 0U);
    return &references_[0] <= handle_scope_entry &&
        handle_scope_entry <= &references_[number_of_references_ - 1];
  }

  // Offset of link within HandleScope, used by generated code
  static size_t LinkOffset(size_t pointer_size) {
    return 0;
  }

  // Offset of length within handle scope, used by generated code
  static size_t NumberOfReferencesOffset(size_t pointer_size) {
    return pointer_size;
  }

  // Offset of link within handle scope, used by generated code
  static size_t ReferencesOffset(size_t pointer_size) {
    return pointer_size + sizeof(number_of_references_);
  }

 protected:
  explicit HandleScope(size_t number_of_references) :
      link_(nullptr), number_of_references_(number_of_references) {
  }

  HandleScope* link_;
  uint32_t number_of_references_;

  // number_of_references_ are available if this is allocated and filled in by jni_compiler.
  StackReference<mirror::Object> references_[0];

 private:
  template<size_t kNumReferences> friend class StackHandleScope;

  DISALLOW_COPY_AND_ASSIGN(HandleScope);
};

// A wrapper which wraps around Object** and restores the pointer in the destructor.
// TODO: Add more functionality.
template<class T>
class HandleWrapper : public Handle<T> {
 public:
  HandleWrapper(T** obj, const Handle<T>& handle)
     : Handle<T>(handle), obj_(obj) {
  }

  ~HandleWrapper() {
    *obj_ = Handle<T>::Get();
  }

 private:
  T** obj_;
};

// Scoped handle storage of a fixed size that is usually stack allocated.
template<size_t kNumReferences>
class PACKED(4) StackHandleScope FINAL : public HandleScope {
 public:
  explicit StackHandleScope(Thread* self);
  ~StackHandleScope();

  // Currently unused, using this GetReference instead of the one in HandleScope is preferred to
  // avoid compiler optimizations incorrectly optimizing out of bound array accesses.
  // TODO: Remove this when it is un-necessary.
  mirror::Object* GetReference(size_t i) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      ALWAYS_INLINE {
    DCHECK_LT(i, number_of_references_);
    return references_storage_[i].AsMirrorPtr();
  }

  Handle<mirror::Object> GetHandle(size_t i) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      ALWAYS_INLINE {
    DCHECK_LT(i, number_of_references_);
    return Handle<mirror::Object>(&references_storage_[i]);
  }

  void SetReference(size_t i, mirror::Object* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      ALWAYS_INLINE {
    DCHECK_LT(i, number_of_references_);
    references_storage_[i].Assign(object);
  }

  template<class T>
  Handle<T> NewHandle(T* object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    SetReference(pos_, object);
    Handle<T> h(GetHandle(pos_));
    pos_++;
    return h;
  }

  template<class T>
  HandleWrapper<T> NewHandleWrapper(T** object) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    SetReference(pos_, *object);
    Handle<T> h(GetHandle(pos_));
    pos_++;
    return HandleWrapper<T>(object, h);
  }

 private:
  // References_storage_ needs to be first so that it appears in the same location as
  // HandleScope::references_.
  StackReference<mirror::Object> references_storage_[kNumReferences];

  // The thread that the stack handle scope is a linked list upon. The stack handle scope will
  // push and pop itself from this thread.
  Thread* const self_;

  // Position new handles will be created.
  size_t pos_;

  template<size_t kNumRefs> friend class StackHandleScope;
};

}  // namespace art

#endif  // ART_RUNTIME_HANDLE_SCOPE_H_
