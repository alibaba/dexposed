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

#ifndef ART_RUNTIME_THREAD_INL_H_
#define ART_RUNTIME_THREAD_INL_H_

#include "thread.h"

#include <pthread.h>

#include "base/casts.h"
#include "base/mutex-inl.h"
#include "gc/heap.h"
#include "jni_internal.h"

namespace art {

// Quickly access the current thread from a JNIEnv.
static inline Thread* ThreadForEnv(JNIEnv* env) {
  JNIEnvExt* full_env(down_cast<JNIEnvExt*>(env));
  return full_env->self;
}

inline Thread* Thread::Current() {
  // We rely on Thread::Current returning NULL for a detached thread, so it's not obvious
  // that we can replace this with a direct %fs access on x86.
  if (!is_started_) {
    return NULL;
  } else {
    void* thread = pthread_getspecific(Thread::pthread_key_self_);
    return reinterpret_cast<Thread*>(thread);
  }
}

inline ThreadState Thread::SetState(ThreadState new_state) {
  // Cannot use this code to change into Runnable as changing to Runnable should fail if
  // old_state_and_flags.suspend_request is true.
  DCHECK_NE(new_state, kRunnable);
  DCHECK_EQ(this, Thread::Current());
  union StateAndFlags old_state_and_flags;
  old_state_and_flags.as_int = tls32_.state_and_flags.as_int;
  tls32_.state_and_flags.as_struct.state = new_state;
  return static_cast<ThreadState>(old_state_and_flags.as_struct.state);
}

inline void Thread::AssertThreadSuspensionIsAllowable(bool check_locks) const {
  if (kIsDebugBuild) {
    CHECK_EQ(0u, tls32_.no_thread_suspension) << tlsPtr_.last_no_thread_suspension_cause;
    if (check_locks) {
      bool bad_mutexes_held = false;
      for (int i = kLockLevelCount - 1; i >= 0; --i) {
        // We expect no locks except the mutator_lock_ or thread list suspend thread lock.
        if (i != kMutatorLock && i != kThreadListSuspendThreadLock) {
          BaseMutex* held_mutex = GetHeldMutex(static_cast<LockLevel>(i));
          if (held_mutex != NULL) {
            LOG(ERROR) << "holding \"" << held_mutex->GetName()
                      << "\" at point where thread suspension is expected";
            bad_mutexes_held = true;
          }
        }
      }
      CHECK(!bad_mutexes_held);
    }
  }
}

inline void Thread::TransitionFromRunnableToSuspended(ThreadState new_state) {
  AssertThreadSuspensionIsAllowable();
  DCHECK_NE(new_state, kRunnable);
  DCHECK_EQ(this, Thread::Current());
  // Change to non-runnable state, thereby appearing suspended to the system.
  DCHECK_EQ(GetState(), kRunnable);
  union StateAndFlags old_state_and_flags;
  union StateAndFlags new_state_and_flags;
  while (true) {
    old_state_and_flags.as_int = tls32_.state_and_flags.as_int;
    if (UNLIKELY((old_state_and_flags.as_struct.flags & kCheckpointRequest) != 0)) {
      RunCheckpointFunction();
      continue;
    }
    // Change the state but keep the current flags (kCheckpointRequest is clear).
    DCHECK_EQ((old_state_and_flags.as_struct.flags & kCheckpointRequest), 0);
    new_state_and_flags.as_struct.flags = old_state_and_flags.as_struct.flags;
    new_state_and_flags.as_struct.state = new_state;

    // CAS the value without a memory ordering as that is given by the lock release below.
    bool done =
        tls32_.state_and_flags.as_atomic_int.CompareExchangeWeakRelaxed(old_state_and_flags.as_int,
                                                                        new_state_and_flags.as_int);
    if (LIKELY(done)) {
      break;
    }
  }
  // Release share on mutator_lock_.
  Locks::mutator_lock_->SharedUnlock(this);
}

inline ThreadState Thread::TransitionFromSuspendedToRunnable() {
  bool done = false;
  union StateAndFlags old_state_and_flags;
  old_state_and_flags.as_int = tls32_.state_and_flags.as_int;
  int16_t old_state = old_state_and_flags.as_struct.state;
  DCHECK_NE(static_cast<ThreadState>(old_state), kRunnable);
  do {
    Locks::mutator_lock_->AssertNotHeld(this);  // Otherwise we starve GC..
    old_state_and_flags.as_int = tls32_.state_and_flags.as_int;
    DCHECK_EQ(old_state_and_flags.as_struct.state, old_state);
    if (UNLIKELY((old_state_and_flags.as_struct.flags & kSuspendRequest) != 0)) {
      // Wait while our suspend count is non-zero.
      MutexLock mu(this, *Locks::thread_suspend_count_lock_);
      old_state_and_flags.as_int = tls32_.state_and_flags.as_int;
      DCHECK_EQ(old_state_and_flags.as_struct.state, old_state);
      while ((old_state_and_flags.as_struct.flags & kSuspendRequest) != 0) {
        // Re-check when Thread::resume_cond_ is notified.
        Thread::resume_cond_->Wait(this);
        old_state_and_flags.as_int = tls32_.state_and_flags.as_int;
        DCHECK_EQ(old_state_and_flags.as_struct.state, old_state);
      }
      DCHECK_EQ(GetSuspendCount(), 0);
    }
    // Re-acquire shared mutator_lock_ access.
    Locks::mutator_lock_->SharedLock(this);
    // Atomically change from suspended to runnable if no suspend request pending.
    old_state_and_flags.as_int = tls32_.state_and_flags.as_int;
    DCHECK_EQ(old_state_and_flags.as_struct.state, old_state);
    if (LIKELY((old_state_and_flags.as_struct.flags & kSuspendRequest) == 0)) {
      union StateAndFlags new_state_and_flags;
      new_state_and_flags.as_int = old_state_and_flags.as_int;
      new_state_and_flags.as_struct.state = kRunnable;
      // CAS the value without a memory ordering as that is given by the lock acquisition above.
      done =
          tls32_.state_and_flags.as_atomic_int.CompareExchangeWeakRelaxed(old_state_and_flags.as_int,
                                                                          new_state_and_flags.as_int);
    }
    if (UNLIKELY(!done)) {
      // Failed to transition to Runnable. Release shared mutator_lock_ access and try again.
      Locks::mutator_lock_->SharedUnlock(this);
    } else {
      return static_cast<ThreadState>(old_state);
    }
  } while (true);
}

inline void Thread::VerifyStack() {
  if (kVerifyStack) {
    if (Runtime::Current()->GetHeap()->IsObjectValidationEnabled()) {
      VerifyStackImpl();
    }
  }
}

inline size_t Thread::TlabSize() const {
  return tlsPtr_.thread_local_end - tlsPtr_.thread_local_pos;
}

inline mirror::Object* Thread::AllocTlab(size_t bytes) {
  DCHECK_GE(TlabSize(), bytes);
  ++tlsPtr_.thread_local_objects;
  mirror::Object* ret = reinterpret_cast<mirror::Object*>(tlsPtr_.thread_local_pos);
  tlsPtr_.thread_local_pos += bytes;
  return ret;
}

inline bool Thread::PushOnThreadLocalAllocationStack(mirror::Object* obj) {
  DCHECK_LE(tlsPtr_.thread_local_alloc_stack_top, tlsPtr_.thread_local_alloc_stack_end);
  if (tlsPtr_.thread_local_alloc_stack_top < tlsPtr_.thread_local_alloc_stack_end) {
    // There's room.
    DCHECK_LE(reinterpret_cast<byte*>(tlsPtr_.thread_local_alloc_stack_top) +
                  sizeof(mirror::Object*),
              reinterpret_cast<byte*>(tlsPtr_.thread_local_alloc_stack_end));
    DCHECK(*tlsPtr_.thread_local_alloc_stack_top == nullptr);
    *tlsPtr_.thread_local_alloc_stack_top = obj;
    ++tlsPtr_.thread_local_alloc_stack_top;
    return true;
  }
  return false;
}

inline void Thread::SetThreadLocalAllocationStack(mirror::Object** start, mirror::Object** end) {
  DCHECK(Thread::Current() == this) << "Should be called by self";
  DCHECK(start != nullptr);
  DCHECK(end != nullptr);
  DCHECK_ALIGNED(start, sizeof(mirror::Object*));
  DCHECK_ALIGNED(end, sizeof(mirror::Object*));
  DCHECK_LT(start, end);
  tlsPtr_.thread_local_alloc_stack_end = end;
  tlsPtr_.thread_local_alloc_stack_top = start;
}

inline void Thread::RevokeThreadLocalAllocationStack() {
  if (kIsDebugBuild) {
    // Note: self is not necessarily equal to this thread since thread may be suspended.
    Thread* self = Thread::Current();
    DCHECK(this == self || IsSuspended() || GetState() == kWaitingPerformingGc)
        << GetState() << " thread " << this << " self " << self;
  }
  tlsPtr_.thread_local_alloc_stack_end = nullptr;
  tlsPtr_.thread_local_alloc_stack_top = nullptr;
}

}  // namespace art

#endif  // ART_RUNTIME_THREAD_INL_H_
