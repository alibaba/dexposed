/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ART_RUNTIME_MONITOR_H_
#define ART_RUNTIME_MONITOR_H_

#include <pthread.h>
#include <stdint.h>

#include <iosfwd>
#include <list>
#include <vector>

#include "atomic.h"
#include "base/allocator.h"
#include "base/mutex.h"
#include "gc_root.h"
#include "object_callbacks.h"
#include "read_barrier_option.h"
#include "thread_state.h"

namespace art {

class LockWord;
template<class T> class Handle;
class Thread;
class StackVisitor;
typedef uint32_t MonitorId;

namespace mirror {
  class ArtMethod;
  class Object;
}  // namespace mirror

class Monitor {
 public:
  // The default number of spins that are done before thread suspension is used to forcibly inflate
  // a lock word. See Runtime::max_spins_before_thin_lock_inflation_.
  constexpr static size_t kDefaultMaxSpinsBeforeThinLockInflation = 50;

  ~Monitor();

  static bool IsSensitiveThread();
  static void Init(uint32_t lock_profiling_threshold, bool (*is_sensitive_thread_hook)());

  // Return the thread id of the lock owner or 0 when there is no owner.
  static uint32_t GetLockOwnerThreadId(mirror::Object* obj)
      NO_THREAD_SAFETY_ANALYSIS;  // TODO: Reading lock owner without holding lock is racy.

  static mirror::Object* MonitorEnter(Thread* thread, mirror::Object* obj)
      EXCLUSIVE_LOCK_FUNCTION(obj)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  static bool MonitorExit(Thread* thread, mirror::Object* obj)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      UNLOCK_FUNCTION(obj);

  static void Notify(Thread* self, mirror::Object* obj)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DoNotify(self, obj, false);
  }
  static void NotifyAll(Thread* self, mirror::Object* obj)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DoNotify(self, obj, true);
  }
  static void Wait(Thread* self, mirror::Object* obj, int64_t ms, int32_t ns,
                   bool interruptShouldThrow, ThreadState why)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static void DescribeWait(std::ostream& os, const Thread* thread)
      LOCKS_EXCLUDED(Locks::thread_suspend_count_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Used to implement JDWP's ThreadReference.CurrentContendedMonitor.
  static mirror::Object* GetContendedMonitor(Thread* thread)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Calls 'callback' once for each lock held in the single stack frame represented by
  // the current state of 'stack_visitor'.
  // The abort_on_failure flag allows to not die when the state of the runtime is unorderly. This
  // is necessary when we have already aborted but want to dump the stack as much as we can.
  static void VisitLocks(StackVisitor* stack_visitor, void (*callback)(mirror::Object*, void*),
                         void* callback_context, bool abort_on_failure = true)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static bool IsValidLockWord(LockWord lock_word);

  template<ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  mirror::Object* GetObject() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return obj_.Read<kReadBarrierOption>();
  }

  void SetObject(mirror::Object* object);

  Thread* GetOwner() const NO_THREAD_SAFETY_ANALYSIS {
    return owner_;
  }

  int32_t GetHashCode();

  bool IsLocked() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool HasHashCode() const {
    return hash_code_.LoadRelaxed() != 0;
  }

  MonitorId GetMonitorId() const {
    return monitor_id_;
  }

  // Inflate the lock on obj. May fail to inflate for spurious reasons, always re-check.
  static void InflateThinLocked(Thread* self, Handle<mirror::Object> obj, LockWord lock_word,
                                uint32_t hash_code) NO_THREAD_SAFETY_ANALYSIS;

  static bool Deflate(Thread* self, mirror::Object* obj)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

 private:
  explicit Monitor(Thread* self, Thread* owner, mirror::Object* obj, int32_t hash_code)
        SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  explicit Monitor(Thread* self, Thread* owner, mirror::Object* obj, int32_t hash_code,
                   MonitorId id) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Install the monitor into its object, may fail if another thread installs a different monitor
  // first.
  bool Install(Thread* self)
      LOCKS_EXCLUDED(monitor_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void AppendToWaitSet(Thread* thread) EXCLUSIVE_LOCKS_REQUIRED(monitor_lock_);
  void RemoveFromWaitSet(Thread* thread) EXCLUSIVE_LOCKS_REQUIRED(monitor_lock_);

  /*
   * Changes the shape of a monitor from thin to fat, preserving the internal lock state. The
   * calling thread must own the lock or the owner must be suspended. There's a race with other
   * threads inflating the lock, installing hash codes and spurious failures. The caller should
   * re-read the lock word following the call.
   */
  static void Inflate(Thread* self, Thread* owner, mirror::Object* obj, int32_t hash_code)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void LogContentionEvent(Thread* self, uint32_t wait_ms, uint32_t sample_percent,
                          const char* owner_filename, uint32_t owner_line_number)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static void FailedUnlock(mirror::Object* obj, Thread* expected_owner, Thread* found_owner, Monitor* mon)
      LOCKS_EXCLUDED(Locks::thread_list_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void Lock(Thread* self)
      LOCKS_EXCLUDED(monitor_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool Unlock(Thread* thread)
      LOCKS_EXCLUDED(monitor_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static void DoNotify(Thread* self, mirror::Object* obj, bool notify_all)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void Notify(Thread* self)
      LOCKS_EXCLUDED(monitor_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void NotifyAll(Thread* self)
      LOCKS_EXCLUDED(monitor_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);


  void Wait(Thread* self, int64_t msec, int32_t nsec, bool interruptShouldThrow, ThreadState why)
      LOCKS_EXCLUDED(monitor_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Translates the provided method and pc into its declaring class' source file and line number.
  void TranslateLocation(mirror::ArtMethod* method, uint32_t pc,
                         const char** source_file, uint32_t* line_number) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  uint32_t GetOwnerThreadId();

  static bool (*is_sensitive_thread_hook_)();
  static uint32_t lock_profiling_threshold_;

  Mutex monitor_lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;

  ConditionVariable monitor_contenders_ GUARDED_BY(monitor_lock_);

  // Number of people waiting on the condition.
  size_t num_waiters_ GUARDED_BY(monitor_lock_);

  // Which thread currently owns the lock?
  Thread* volatile owner_ GUARDED_BY(monitor_lock_);

  // Owner's recursive lock depth.
  int lock_count_ GUARDED_BY(monitor_lock_);

  // What object are we part of. This is a weak root. Do not access
  // this directly, use GetObject() to read it so it will be guarded
  // by a read barrier.
  GcRoot<mirror::Object> obj_;

  // Threads currently waiting on this monitor.
  Thread* wait_set_ GUARDED_BY(monitor_lock_);

  // Stored object hash code, generated lazily by GetHashCode.
  AtomicInteger hash_code_;

  // Method and dex pc where the lock owner acquired the lock, used when lock
  // sampling is enabled. locking_method_ may be null if the lock is currently
  // unlocked, or if the lock is acquired by the system when the stack is empty.
  mirror::ArtMethod* locking_method_ GUARDED_BY(monitor_lock_);
  uint32_t locking_dex_pc_ GUARDED_BY(monitor_lock_);

  // The denser encoded version of this monitor as stored in the lock word.
  MonitorId monitor_id_;

#ifdef __LP64__
  // Free list for monitor pool.
  Monitor* next_free_ GUARDED_BY(Locks::allocated_monitor_ids_lock_);
#endif

  friend class MonitorInfo;
  friend class MonitorList;
  friend class MonitorPool;
  friend class mirror::Object;
  DISALLOW_COPY_AND_ASSIGN(Monitor);
};

class MonitorList {
 public:
  MonitorList();
  ~MonitorList();

  void Add(Monitor* m);

  void SweepMonitorList(IsMarkedCallback* callback, void* arg)
      LOCKS_EXCLUDED(monitor_list_lock_) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void DisallowNewMonitors() LOCKS_EXCLUDED(monitor_list_lock_);
  void AllowNewMonitors() LOCKS_EXCLUDED(monitor_list_lock_);
  // Returns how many monitors were deflated.
  size_t DeflateMonitors() LOCKS_EXCLUDED(monitor_list_lock_)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);

  typedef std::list<Monitor*, TrackingAllocator<Monitor*, kAllocatorTagMonitorList>> Monitors;

 private:
  // During sweeping we may free an object and on a separate thread have an object created using
  // the newly freed memory. That object may then have its lock-word inflated and a monitor created.
  // If we allow new monitor registration during sweeping this monitor may be incorrectly freed as
  // the object wasn't marked when sweeping began.
  bool allow_new_monitors_ GUARDED_BY(monitor_list_lock_);
  Mutex monitor_list_lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;
  ConditionVariable monitor_add_condition_ GUARDED_BY(monitor_list_lock_);
  Monitors list_ GUARDED_BY(monitor_list_lock_);

  friend class Monitor;
  DISALLOW_COPY_AND_ASSIGN(MonitorList);
};

// Collects information about the current state of an object's monitor.
// This is very unsafe, and must only be called when all threads are suspended.
// For use only by the JDWP implementation.
class MonitorInfo {
 public:
  explicit MonitorInfo(mirror::Object* o) EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);

  Thread* owner_;
  size_t entry_count_;
  std::vector<Thread*> waiters_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MonitorInfo);
};

}  // namespace art

#endif  // ART_RUNTIME_MONITOR_H_
