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

#ifndef ART_RUNTIME_BARRIER_H_
#define ART_RUNTIME_BARRIER_H_

#include <memory>
#include "base/mutex.h"

namespace art {

class Barrier {
 public:
  explicit Barrier(int count);
  virtual ~Barrier();

  // Pass through the barrier, decrements the count but does not block.
  void Pass(Thread* self);

  // Wait on the barrier, decrement the count.
  void Wait(Thread* self);

  // Set the count to a new value, if the value is 0 then everyone waiting on the condition
  // variable is resumed.
  void Init(Thread* self, int count);

  // Increment the count by delta, wait on condition if count is non zero.
  void Increment(Thread* self, int delta);

  // Increment the count by delta, wait on condition if count is non zero, with a timeout
  void Increment(Thread* self, int delta, uint32_t timeout_ms) LOCKS_EXCLUDED(lock_);

 private:
  void SetCountLocked(Thread* self, int count) EXCLUSIVE_LOCKS_REQUIRED(lock_);

  // Counter, when this reaches 0 all people blocked on the barrier are signalled.
  int count_ GUARDED_BY(lock_);

  Mutex lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;
  ConditionVariable condition_ GUARDED_BY(lock_);
};

}  // namespace art
#endif  // ART_RUNTIME_BARRIER_H_
