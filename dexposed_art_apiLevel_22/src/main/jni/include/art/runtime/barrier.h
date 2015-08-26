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

// CAUTION: THIS IS NOT A FULLY GENERAL BARRIER API.

// It may either be used as a "latch" or single-use barrier, or it may be reused under
// very limited conditions, e.g. if only Pass(), but not Wait() is called.  Unlike a standard
// latch API, it is possible to initialize the latch to a count of zero, repeatedly call
// Pass() or Wait(), and only then set the count using the Increment() method.  Threads at
// a Wait() are only awoken if the count reaches zero AFTER the decrement is applied.
// This works because, also unlike most latch APIs, there is no way to Wait() without
// decrementing the count, and thus nobody can spuriosly wake up on the initial zero.

#ifndef ART_RUNTIME_BARRIER_H_
#define ART_RUNTIME_BARRIER_H_

#include <memory>
#include "base/mutex.h"

namespace art {

// TODO: Maybe give this a better name.
class Barrier {
 public:
  explicit Barrier(int count);
  virtual ~Barrier();

  // Pass through the barrier, decrement the count but do not block.
  void Pass(Thread* self);

  // Wait on the barrier, decrement the count.
  void Wait(Thread* self);

  // The following three calls are only safe if we somehow know that no other thread both
  // - has been woken up, and
  // - has not left the Wait() or Increment() call.
  // If these calls are made in that situation, the offending thread is likely to go back
  // to sleep, resulting in a deadlock.

  // Increment the count by delta, wait on condition if count is non zero.
  void Increment(Thread* self, int delta);

  // Increment the count by delta, wait on condition if count is non zero, with a timeout
  void Increment(Thread* self, int delta, uint32_t timeout_ms) LOCKS_EXCLUDED(lock_);

  // Set the count to a new value.  This should only be used if there is no possibility that
  // another thread is still in Wait().  See above.
  void Init(Thread* self, int count);

 private:
  void SetCountLocked(Thread* self, int count) EXCLUSIVE_LOCKS_REQUIRED(lock_);

  // Counter, when this reaches 0 all people blocked on the barrier are signalled.
  int count_ GUARDED_BY(lock_);

  Mutex lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;
  ConditionVariable condition_ GUARDED_BY(lock_);
};

}  // namespace art
#endif  // ART_RUNTIME_BARRIER_H_
