/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ART_RUNTIME_SCOPED_THREAD_STATE_CHANGE_H_
#define ART_RUNTIME_SCOPED_THREAD_STATE_CHANGE_H_

#include "base/casts.h"
#include "jni_internal-inl.h"
#include "read_barrier.h"
#include "thread-inl.h"
#include "verify_object.h"

namespace art {

// Scoped change into and out of a particular state. Handles Runnable transitions that require
// more complicated suspension checking. The subclasses ScopedObjectAccessUnchecked and
// ScopedObjectAccess are used to handle the change into Runnable to Get direct access to objects,
// the unchecked variant doesn't aid annotalysis.
class ScopedThreadStateChange {
 public:
  ScopedThreadStateChange(Thread* self, ThreadState new_thread_state)
      LOCKS_EXCLUDED(Locks::thread_suspend_count_lock_) ALWAYS_INLINE
      : self_(self), thread_state_(new_thread_state), expected_has_no_thread_(false) {
    if (UNLIKELY(self_ == NULL)) {
      // Value chosen arbitrarily and won't be used in the destructor since thread_ == NULL.
      old_thread_state_ = kTerminated;
      Runtime* runtime = Runtime::Current();
      CHECK(runtime == NULL || !runtime->IsStarted() || runtime->IsShuttingDown(self_));
    } else {
      DCHECK_EQ(self, Thread::Current());
      // Read state without locks, ok as state is effectively thread local and we're not interested
      // in the suspend count (this will be handled in the runnable transitions).
      old_thread_state_ = self->GetState();
      if (old_thread_state_ != new_thread_state) {
        if (new_thread_state == kRunnable) {
          self_->TransitionFromSuspendedToRunnable();
        } else if (old_thread_state_ == kRunnable) {
          self_->TransitionFromRunnableToSuspended(new_thread_state);
        } else {
          // A suspended transition to another effectively suspended transition, ok to use Unsafe.
          self_->SetState(new_thread_state);
        }
      }
    }
  }

  ~ScopedThreadStateChange() LOCKS_EXCLUDED(Locks::thread_suspend_count_lock_) ALWAYS_INLINE {
    if (UNLIKELY(self_ == NULL)) {
      if (!expected_has_no_thread_) {
        Runtime* runtime = Runtime::Current();
        bool shutting_down = (runtime == NULL) || runtime->IsShuttingDown(nullptr);
        CHECK(shutting_down);
      }
    } else {
      if (old_thread_state_ != thread_state_) {
        if (old_thread_state_ == kRunnable) {
          self_->TransitionFromSuspendedToRunnable();
        } else if (thread_state_ == kRunnable) {
          self_->TransitionFromRunnableToSuspended(old_thread_state_);
        } else {
          // A suspended transition to another effectively suspended transition, ok to use Unsafe.
          self_->SetState(old_thread_state_);
        }
      }
    }
  }

  Thread* Self() const {
    return self_;
  }

 protected:
  // Constructor used by ScopedJniThreadState for an unattached thread that has access to the VM*.
  ScopedThreadStateChange()
      : self_(NULL), thread_state_(kTerminated), old_thread_state_(kTerminated),
        expected_has_no_thread_(true) {}

  Thread* const self_;
  const ThreadState thread_state_;

 private:
  ThreadState old_thread_state_;
  const bool expected_has_no_thread_;

  friend class ScopedObjectAccessUnchecked;
  DISALLOW_COPY_AND_ASSIGN(ScopedThreadStateChange);
};

// Assumes we are already runnable.
class ScopedObjectAccessAlreadyRunnable {
 public:
  Thread* Self() const {
    return self_;
  }

  JNIEnvExt* Env() const {
    return env_;
  }

  JavaVMExt* Vm() const {
    return vm_;
  }

  /*
   * Add a local reference for an object to the indirect reference table associated with the
   * current stack frame.  When the native function returns, the reference will be discarded.
   *
   * We need to allow the same reference to be added multiple times, and cope with NULL.
   *
   * This will be called on otherwise unreferenced objects. We cannot do GC allocations here, and
   * it's best if we don't grab a mutex.
   */
  template<typename T>
  T AddLocalReference(mirror::Object* obj) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    Locks::mutator_lock_->AssertSharedHeld(Self());
    DCHECK(IsRunnable());  // Don't work with raw objects in non-runnable states.
    if (obj == NULL) {
      return NULL;
    }
    DCHECK_NE((reinterpret_cast<uintptr_t>(obj) & 0xffff0000), 0xebad0000);
    return Env()->AddLocalReference<T>(obj);
  }

  template<typename T>
  T Decode(jobject obj) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    Locks::mutator_lock_->AssertSharedHeld(Self());
    DCHECK(IsRunnable());  // Don't work with raw objects in non-runnable states.
    return down_cast<T>(Self()->DecodeJObject(obj));
  }

  mirror::ArtField* DecodeField(jfieldID fid) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    Locks::mutator_lock_->AssertSharedHeld(Self());
    DCHECK(IsRunnable());  // Don't work with raw objects in non-runnable states.
    CHECK(!kMovingFields);
    mirror::ArtField* field = reinterpret_cast<mirror::ArtField*>(fid);
    return ReadBarrier::BarrierForRoot<mirror::ArtField, kWithReadBarrier>(&field);
  }

  jfieldID EncodeField(mirror::ArtField* field) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    Locks::mutator_lock_->AssertSharedHeld(Self());
    DCHECK(IsRunnable());  // Don't work with raw objects in non-runnable states.
    CHECK(!kMovingFields);
    return reinterpret_cast<jfieldID>(field);
  }

  mirror::ArtMethod* DecodeMethod(jmethodID mid) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    Locks::mutator_lock_->AssertSharedHeld(Self());
    DCHECK(IsRunnable());  // Don't work with raw objects in non-runnable states.
    CHECK(!kMovingMethods);
    mirror::ArtMethod* method = reinterpret_cast<mirror::ArtMethod*>(mid);
    return ReadBarrier::BarrierForRoot<mirror::ArtMethod, kWithReadBarrier>(&method);
  }

  jmethodID EncodeMethod(mirror::ArtMethod* method) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    Locks::mutator_lock_->AssertSharedHeld(Self());
    DCHECK(IsRunnable());  // Don't work with raw objects in non-runnable states.
    CHECK(!kMovingMethods);
    return reinterpret_cast<jmethodID>(method);
  }

  bool IsRunnable() const {
    return self_->GetState() == kRunnable;
  }

 protected:
  explicit ScopedObjectAccessAlreadyRunnable(JNIEnv* env)
      LOCKS_EXCLUDED(Locks::thread_suspend_count_lock_) ALWAYS_INLINE
      : self_(ThreadForEnv(env)), env_(down_cast<JNIEnvExt*>(env)), vm_(env_->vm) {
  }

  explicit ScopedObjectAccessAlreadyRunnable(Thread* self)
      LOCKS_EXCLUDED(Locks::thread_suspend_count_lock_) ALWAYS_INLINE
      : self_(self), env_(down_cast<JNIEnvExt*>(self->GetJniEnv())),
        vm_(env_ != nullptr ? env_->vm : nullptr) {
  }

  // Used when we want a scoped JNI thread state but have no thread/JNIEnv. Consequently doesn't
  // change into Runnable or acquire a share on the mutator_lock_.
  explicit ScopedObjectAccessAlreadyRunnable(JavaVM* vm)
      : self_(nullptr), env_(nullptr), vm_(down_cast<JavaVMExt*>(vm)) {}

  // Here purely to force inlining.
  ~ScopedObjectAccessAlreadyRunnable() ALWAYS_INLINE {
  }

  // Self thread, can be null.
  Thread* const self_;
  // The full JNIEnv.
  JNIEnvExt* const env_;
  // The full JavaVM.
  JavaVMExt* const vm_;
};

// Entry/exit processing for transitions from Native to Runnable (ie within JNI functions).
//
// This class performs the necessary thread state switching to and from Runnable and lets us
// amortize the cost of working out the current thread. Additionally it lets us check (and repair)
// apps that are using a JNIEnv on the wrong thread. The class also decodes and encodes Objects
// into jobjects via methods of this class. Performing this here enforces the Runnable thread state
// for use of Object, thereby inhibiting the Object being modified by GC whilst native or VM code
// is also manipulating the Object.
//
// The destructor transitions back to the previous thread state, typically Native. In this state
// GC and thread suspension may occur.
//
// For annotalysis the subclass ScopedObjectAccess (below) makes it explicit that a shared of
// the mutator_lock_ will be acquired on construction.
class ScopedObjectAccessUnchecked : public ScopedObjectAccessAlreadyRunnable {
 public:
  explicit ScopedObjectAccessUnchecked(JNIEnv* env)
      LOCKS_EXCLUDED(Locks::thread_suspend_count_lock_) ALWAYS_INLINE
      : ScopedObjectAccessAlreadyRunnable(env), tsc_(Self(), kRunnable) {
    Self()->VerifyStack();
    Locks::mutator_lock_->AssertSharedHeld(Self());
  }

  explicit ScopedObjectAccessUnchecked(Thread* self)
      LOCKS_EXCLUDED(Locks::thread_suspend_count_lock_) ALWAYS_INLINE
      : ScopedObjectAccessAlreadyRunnable(self), tsc_(self, kRunnable) {
    Self()->VerifyStack();
    Locks::mutator_lock_->AssertSharedHeld(Self());
  }

  // Used when we want a scoped JNI thread state but have no thread/JNIEnv. Consequently doesn't
  // change into Runnable or acquire a share on the mutator_lock_.
  explicit ScopedObjectAccessUnchecked(JavaVM* vm) ALWAYS_INLINE
      : ScopedObjectAccessAlreadyRunnable(vm), tsc_() {}

 private:
  // The scoped thread state change makes sure that we are runnable and restores the thread state
  // in the destructor.
  const ScopedThreadStateChange tsc_;

  DISALLOW_COPY_AND_ASSIGN(ScopedObjectAccessUnchecked);
};

// Annotalysis helping variant of the above.
class ScopedObjectAccess : public ScopedObjectAccessUnchecked {
 public:
  explicit ScopedObjectAccess(JNIEnv* env)
      LOCKS_EXCLUDED(Locks::thread_suspend_count_lock_)
      SHARED_LOCK_FUNCTION(Locks::mutator_lock_) ALWAYS_INLINE
      : ScopedObjectAccessUnchecked(env) {
  }

  explicit ScopedObjectAccess(Thread* self)
      LOCKS_EXCLUDED(Locks::thread_suspend_count_lock_)
      SHARED_LOCK_FUNCTION(Locks::mutator_lock_) ALWAYS_INLINE
      : ScopedObjectAccessUnchecked(self) {
  }

  ~ScopedObjectAccess() UNLOCK_FUNCTION(Locks::mutator_lock_) ALWAYS_INLINE {
    // Base class will release share of lock. Invoked after this destructor.
  }

 private:
  // TODO: remove this constructor. It is used by check JNI's ScopedCheck to make it believe that
  //       routines operating with just a VM are sound, they are not, but when you have just a VM
  //       you cannot call the unsound routines.
  explicit ScopedObjectAccess(JavaVM* vm)
      SHARED_LOCK_FUNCTION(Locks::mutator_lock_)
      : ScopedObjectAccessUnchecked(vm) {}

  friend class ScopedCheck;
  DISALLOW_COPY_AND_ASSIGN(ScopedObjectAccess);
};

}  // namespace art

#endif  // ART_RUNTIME_SCOPED_THREAD_STATE_CHANGE_H_
