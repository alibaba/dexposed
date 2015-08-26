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

#ifndef ART_RUNTIME_BASE_MUTEX_H_
#define ART_RUNTIME_BASE_MUTEX_H_

#include <pthread.h>
#include <stdint.h>

#include <iosfwd>
#include <string>

#include "atomic.h"
#include "base/logging.h"
#include "base/macros.h"
#include "globals.h"

#if defined(__APPLE__)
#define ART_USE_FUTEXES 0
#else
#define ART_USE_FUTEXES 1
#endif

// Currently Darwin doesn't support locks with timeouts.
#if !defined(__APPLE__)
#define HAVE_TIMED_RWLOCK 1
#else
#define HAVE_TIMED_RWLOCK 0
#endif

namespace art {

class LOCKABLE ReaderWriterMutex;
class ScopedContentionRecorder;
class Thread;

// LockLevel is used to impose a lock hierarchy [1] where acquisition of a Mutex at a higher or
// equal level to a lock a thread holds is invalid. The lock hierarchy achieves a cycle free
// partial ordering and thereby cause deadlock situations to fail checks.
//
// [1] http://www.drdobbs.com/parallel/use-lock-hierarchies-to-avoid-deadlock/204801163
enum LockLevel {
  kLoggingLock = 0,
  kMemMapsLock,
  kSwapMutexesLock,
  kUnexpectedSignalLock,
  kThreadSuspendCountLock,
  kAbortLock,
  kJdwpSocketLock,
  kReferenceQueueSoftReferencesLock,
  kReferenceQueuePhantomReferencesLock,
  kReferenceQueueFinalizerReferencesLock,
  kReferenceQueueWeakReferencesLock,
  kReferenceQueueClearedReferencesLock,
  kReferenceProcessorLock,
  kRosAllocGlobalLock,
  kRosAllocBracketLock,
  kRosAllocBulkFreeLock,
  kAllocSpaceLock,
  kDexFileMethodInlinerLock,
  kDexFileToMethodInlinerMapLock,
  kMarkSweepMarkStackLock,
  kTransactionLogLock,
  kInternTableLock,
  kOatFileSecondaryLookupLock,
  kDefaultMutexLevel,
  kMarkSweepLargeObjectLock,
  kPinTableLock,
  kLoadLibraryLock,
  kJdwpObjectRegistryLock,
  kModifyLdtLock,
  kAllocatedThreadIdsLock,
  kMonitorPoolLock,
  kClassLinkerClassesLock,
  kBreakpointLock,
  kMonitorLock,
  kMonitorListLock,
  kThreadListLock,
  kBreakpointInvokeLock,
  kAllocTrackerLock,
  kDeoptimizationLock,
  kProfilerLock,
  kJdwpEventListLock,
  kJdwpAttachLock,
  kJdwpStartLock,
  kRuntimeShutdownLock,
  kTraceLock,
  kHeapBitmapLock,
  kMutatorLock,
  kInstrumentEntrypointsLock,
  kThreadListSuspendThreadLock,
  kZygoteCreationLock,

  kLockLevelCount  // Must come last.
};
std::ostream& operator<<(std::ostream& os, const LockLevel& rhs);

const bool kDebugLocking = kIsDebugBuild;

// Record Log contention information, dumpable via SIGQUIT.
#ifdef ART_USE_FUTEXES
// To enable lock contention logging, set this to true.
const bool kLogLockContentions = false;
#else
// Keep this false as lock contention logging is supported only with
// futex.
const bool kLogLockContentions = false;
#endif
const size_t kContentionLogSize = 4;
const size_t kContentionLogDataSize = kLogLockContentions ? 1 : 0;
const size_t kAllMutexDataSize = kLogLockContentions ? 1 : 0;

// Base class for all Mutex implementations
class BaseMutex {
 public:
  const char* GetName() const {
    return name_;
  }

  virtual bool IsMutex() const { return false; }
  virtual bool IsReaderWriterMutex() const { return false; }

  virtual void Dump(std::ostream& os) const = 0;

  static void DumpAll(std::ostream& os);

 protected:
  friend class ConditionVariable;

  BaseMutex(const char* name, LockLevel level);
  virtual ~BaseMutex();
  void RegisterAsLocked(Thread* self);
  void RegisterAsUnlocked(Thread* self);
  void CheckSafeToWait(Thread* self);

  friend class ScopedContentionRecorder;

  void RecordContention(uint64_t blocked_tid, uint64_t owner_tid, uint64_t nano_time_blocked);
  void DumpContention(std::ostream& os) const;

  const LockLevel level_;  // Support for lock hierarchy.
  const char* const name_;

  // A log entry that records contention but makes no guarantee that either tid will be held live.
  struct ContentionLogEntry {
    ContentionLogEntry() : blocked_tid(0), owner_tid(0) {}
    uint64_t blocked_tid;
    uint64_t owner_tid;
    AtomicInteger count;
  };
  struct ContentionLogData {
    ContentionLogEntry contention_log[kContentionLogSize];
    // The next entry in the contention log to be updated. Value ranges from 0 to
    // kContentionLogSize - 1.
    AtomicInteger cur_content_log_entry;
    // Number of times the Mutex has been contended.
    AtomicInteger contention_count;
    // Sum of time waited by all contenders in ns.
    Atomic<uint64_t> wait_time;
    void AddToWaitTime(uint64_t value);
    ContentionLogData() : wait_time(0) {}
  };
  ContentionLogData contention_log_data_[kContentionLogDataSize];

 public:
  bool HasEverContended() const {
    if (kLogLockContentions) {
      return contention_log_data_->contention_count.LoadSequentiallyConsistent() > 0;
    }
    return false;
  }
};

// A Mutex is used to achieve mutual exclusion between threads. A Mutex can be used to gain
// exclusive access to what it guards. A Mutex can be in one of two states:
// - Free - not owned by any thread,
// - Exclusive - owned by a single thread.
//
// The effect of locking and unlocking operations on the state is:
// State     | ExclusiveLock | ExclusiveUnlock
// -------------------------------------------
// Free      | Exclusive     | error
// Exclusive | Block*        | Free
// * Mutex is not reentrant and so an attempt to ExclusiveLock on the same thread will result in
//   an error. Being non-reentrant simplifies Waiting on ConditionVariables.
std::ostream& operator<<(std::ostream& os, const Mutex& mu);
class LOCKABLE Mutex : public BaseMutex {
 public:
  explicit Mutex(const char* name, LockLevel level = kDefaultMutexLevel, bool recursive = false);
  ~Mutex();

  virtual bool IsMutex() const { return true; }

  // Block until mutex is free then acquire exclusive access.
  void ExclusiveLock(Thread* self) EXCLUSIVE_LOCK_FUNCTION();
  void Lock(Thread* self) EXCLUSIVE_LOCK_FUNCTION() {  ExclusiveLock(self); }

  // Returns true if acquires exclusive access, false otherwise.
  bool ExclusiveTryLock(Thread* self) EXCLUSIVE_TRYLOCK_FUNCTION(true);
  bool TryLock(Thread* self) EXCLUSIVE_TRYLOCK_FUNCTION(true) { return ExclusiveTryLock(self); }

  // Release exclusive access.
  void ExclusiveUnlock(Thread* self) UNLOCK_FUNCTION();
  void Unlock(Thread* self) UNLOCK_FUNCTION() {  ExclusiveUnlock(self); }

  // Is the current thread the exclusive holder of the Mutex.
  bool IsExclusiveHeld(const Thread* self) const;

  // Assert that the Mutex is exclusively held by the current thread.
  void AssertExclusiveHeld(const Thread* self) {
    if (kDebugLocking && (gAborting == 0)) {
      CHECK(IsExclusiveHeld(self)) << *this;
    }
  }
  void AssertHeld(const Thread* self) { AssertExclusiveHeld(self); }

  // Assert that the Mutex is not held by the current thread.
  void AssertNotHeldExclusive(const Thread* self) {
    if (kDebugLocking && (gAborting == 0)) {
      CHECK(!IsExclusiveHeld(self)) << *this;
    }
  }
  void AssertNotHeld(const Thread* self) { AssertNotHeldExclusive(self); }

  // Id associated with exclusive owner. No memory ordering semantics if called from a thread other
  // than the owner.
  uint64_t GetExclusiveOwnerTid() const;

  // Returns how many times this Mutex has been locked, it is better to use AssertHeld/NotHeld.
  unsigned int GetDepth() const {
    return recursion_count_;
  }

  virtual void Dump(std::ostream& os) const;

 private:
#if ART_USE_FUTEXES
  // 0 is unheld, 1 is held.
  AtomicInteger state_;
  // Exclusive owner.
  volatile uint64_t exclusive_owner_;
  // Number of waiting contenders.
  AtomicInteger num_contenders_;
#else
  pthread_mutex_t mutex_;
  volatile uint64_t exclusive_owner_;  // Guarded by mutex_.
#endif
  const bool recursive_;  // Can the lock be recursively held?
  unsigned int recursion_count_;
  friend class ConditionVariable;
  DISALLOW_COPY_AND_ASSIGN(Mutex);
};

// A ReaderWriterMutex is used to achieve mutual exclusion between threads, similar to a Mutex.
// Unlike a Mutex a ReaderWriterMutex can be used to gain exclusive (writer) or shared (reader)
// access to what it guards. A flaw in relation to a Mutex is that it cannot be used with a
// condition variable. A ReaderWriterMutex can be in one of three states:
// - Free - not owned by any thread,
// - Exclusive - owned by a single thread,
// - Shared(n) - shared amongst n threads.
//
// The effect of locking and unlocking operations on the state is:
//
// State     | ExclusiveLock | ExclusiveUnlock | SharedLock       | SharedUnlock
// ----------------------------------------------------------------------------
// Free      | Exclusive     | error           | SharedLock(1)    | error
// Exclusive | Block         | Free            | Block            | error
// Shared(n) | Block         | error           | SharedLock(n+1)* | Shared(n-1) or Free
// * for large values of n the SharedLock may block.
std::ostream& operator<<(std::ostream& os, const ReaderWriterMutex& mu);
class LOCKABLE ReaderWriterMutex : public BaseMutex {
 public:
  explicit ReaderWriterMutex(const char* name, LockLevel level = kDefaultMutexLevel);
  ~ReaderWriterMutex();

  virtual bool IsReaderWriterMutex() const { return true; }

  // Block until ReaderWriterMutex is free then acquire exclusive access.
  void ExclusiveLock(Thread* self) EXCLUSIVE_LOCK_FUNCTION();
  void WriterLock(Thread* self) EXCLUSIVE_LOCK_FUNCTION() {  ExclusiveLock(self); }

  // Release exclusive access.
  void ExclusiveUnlock(Thread* self) UNLOCK_FUNCTION();
  void WriterUnlock(Thread* self) UNLOCK_FUNCTION() {  ExclusiveUnlock(self); }

  // Block until ReaderWriterMutex is free and acquire exclusive access. Returns true on success
  // or false if timeout is reached.
#if HAVE_TIMED_RWLOCK
  bool ExclusiveLockWithTimeout(Thread* self, int64_t ms, int32_t ns)
      EXCLUSIVE_TRYLOCK_FUNCTION(true);
#endif

  // Block until ReaderWriterMutex is shared or free then acquire a share on the access.
  void SharedLock(Thread* self) SHARED_LOCK_FUNCTION() ALWAYS_INLINE;
  void ReaderLock(Thread* self) SHARED_LOCK_FUNCTION() { SharedLock(self); }

  // Try to acquire share of ReaderWriterMutex.
  bool SharedTryLock(Thread* self) EXCLUSIVE_TRYLOCK_FUNCTION(true);

  // Release a share of the access.
  void SharedUnlock(Thread* self) UNLOCK_FUNCTION() ALWAYS_INLINE;
  void ReaderUnlock(Thread* self) UNLOCK_FUNCTION() { SharedUnlock(self); }

  // Is the current thread the exclusive holder of the ReaderWriterMutex.
  bool IsExclusiveHeld(const Thread* self) const;

  // Assert the current thread has exclusive access to the ReaderWriterMutex.
  void AssertExclusiveHeld(const Thread* self) {
    if (kDebugLocking && (gAborting == 0)) {
      CHECK(IsExclusiveHeld(self)) << *this;
    }
  }
  void AssertWriterHeld(const Thread* self) { AssertExclusiveHeld(self); }

  // Assert the current thread doesn't have exclusive access to the ReaderWriterMutex.
  void AssertNotExclusiveHeld(const Thread* self) {
    if (kDebugLocking && (gAborting == 0)) {
      CHECK(!IsExclusiveHeld(self)) << *this;
    }
  }
  void AssertNotWriterHeld(const Thread* self) { AssertNotExclusiveHeld(self); }

  // Is the current thread a shared holder of the ReaderWriterMutex.
  bool IsSharedHeld(const Thread* self) const;

  // Assert the current thread has shared access to the ReaderWriterMutex.
  void AssertSharedHeld(const Thread* self) {
    if (kDebugLocking && (gAborting == 0)) {
      // TODO: we can only assert this well when self != NULL.
      CHECK(IsSharedHeld(self) || self == NULL) << *this;
    }
  }
  void AssertReaderHeld(const Thread* self) { AssertSharedHeld(self); }

  // Assert the current thread doesn't hold this ReaderWriterMutex either in shared or exclusive
  // mode.
  void AssertNotHeld(const Thread* self) {
    if (kDebugLocking && (gAborting == 0)) {
      CHECK(!IsSharedHeld(self)) << *this;
    }
  }

  // Id associated with exclusive owner. No memory ordering semantics if called from a thread other
  // than the owner.
  uint64_t GetExclusiveOwnerTid() const;

  virtual void Dump(std::ostream& os) const;

 private:
#if ART_USE_FUTEXES
  // -1 implies held exclusive, +ve shared held by state_ many owners.
  AtomicInteger state_;
  // Exclusive owner. Modification guarded by this mutex.
  volatile uint64_t exclusive_owner_;
  // Number of contenders waiting for a reader share.
  AtomicInteger num_pending_readers_;
  // Number of contenders waiting to be the writer.
  AtomicInteger num_pending_writers_;
#else
  pthread_rwlock_t rwlock_;
  volatile uint64_t exclusive_owner_;  // Guarded by rwlock_.
#endif
  DISALLOW_COPY_AND_ASSIGN(ReaderWriterMutex);
};

// ConditionVariables allow threads to queue and sleep. Threads may then be resumed individually
// (Signal) or all at once (Broadcast).
class ConditionVariable {
 public:
  explicit ConditionVariable(const char* name, Mutex& mutex);
  ~ConditionVariable();

  void Broadcast(Thread* self);
  void Signal(Thread* self);
  // TODO: No thread safety analysis on Wait and TimedWait as they call mutex operations via their
  //       pointer copy, thereby defeating annotalysis.
  void Wait(Thread* self) NO_THREAD_SAFETY_ANALYSIS;
  void TimedWait(Thread* self, int64_t ms, int32_t ns) NO_THREAD_SAFETY_ANALYSIS;
  // Variant of Wait that should be used with caution. Doesn't validate that no mutexes are held
  // when waiting.
  // TODO: remove this.
  void WaitHoldingLocks(Thread* self) NO_THREAD_SAFETY_ANALYSIS;

 private:
  const char* const name_;
  // The Mutex being used by waiters. It is an error to mix condition variables between different
  // Mutexes.
  Mutex& guard_;
#if ART_USE_FUTEXES
  // A counter that is modified by signals and broadcasts. This ensures that when a waiter gives up
  // their Mutex and another thread takes it and signals, the waiting thread observes that sequence_
  // changed and doesn't enter the wait. Modified while holding guard_, but is read by futex wait
  // without guard_ held.
  AtomicInteger sequence_;
  // Number of threads that have come into to wait, not the length of the waiters on the futex as
  // waiters may have been requeued onto guard_. Guarded by guard_.
  volatile int32_t num_waiters_;
#else
  pthread_cond_t cond_;
#endif
  DISALLOW_COPY_AND_ASSIGN(ConditionVariable);
};

// Scoped locker/unlocker for a regular Mutex that acquires mu upon construction and releases it
// upon destruction.
class SCOPED_LOCKABLE MutexLock {
 public:
  explicit MutexLock(Thread* self, Mutex& mu) EXCLUSIVE_LOCK_FUNCTION(mu) : self_(self), mu_(mu) {
    mu_.ExclusiveLock(self_);
  }

  ~MutexLock() UNLOCK_FUNCTION() {
    mu_.ExclusiveUnlock(self_);
  }

 private:
  Thread* const self_;
  Mutex& mu_;
  DISALLOW_COPY_AND_ASSIGN(MutexLock);
};
// Catch bug where variable name is omitted. "MutexLock (lock);" instead of "MutexLock mu(lock)".
#define MutexLock(x) COMPILE_ASSERT(0, mutex_lock_declaration_missing_variable_name)

// Scoped locker/unlocker for a ReaderWriterMutex that acquires read access to mu upon
// construction and releases it upon destruction.
class SCOPED_LOCKABLE ReaderMutexLock {
 public:
  explicit ReaderMutexLock(Thread* self, ReaderWriterMutex& mu) EXCLUSIVE_LOCK_FUNCTION(mu) :
      self_(self), mu_(mu) {
    mu_.SharedLock(self_);
  }

  ~ReaderMutexLock() UNLOCK_FUNCTION() {
    mu_.SharedUnlock(self_);
  }

 private:
  Thread* const self_;
  ReaderWriterMutex& mu_;
  DISALLOW_COPY_AND_ASSIGN(ReaderMutexLock);
};
// Catch bug where variable name is omitted. "ReaderMutexLock (lock);" instead of
// "ReaderMutexLock mu(lock)".
#define ReaderMutexLock(x) COMPILE_ASSERT(0, reader_mutex_lock_declaration_missing_variable_name)

// Scoped locker/unlocker for a ReaderWriterMutex that acquires write access to mu upon
// construction and releases it upon destruction.
class SCOPED_LOCKABLE WriterMutexLock {
 public:
  explicit WriterMutexLock(Thread* self, ReaderWriterMutex& mu) EXCLUSIVE_LOCK_FUNCTION(mu) :
      self_(self), mu_(mu) {
    mu_.ExclusiveLock(self_);
  }

  ~WriterMutexLock() UNLOCK_FUNCTION() {
    mu_.ExclusiveUnlock(self_);
  }

 private:
  Thread* const self_;
  ReaderWriterMutex& mu_;
  DISALLOW_COPY_AND_ASSIGN(WriterMutexLock);
};
// Catch bug where variable name is omitted. "WriterMutexLock (lock);" instead of
// "WriterMutexLock mu(lock)".
#define WriterMutexLock(x) COMPILE_ASSERT(0, writer_mutex_lock_declaration_missing_variable_name)

// Global mutexes corresponding to the levels above.
class Locks {
 public:
  static void Init();

  // There's a potential race for two threads to try to suspend each other and for both of them
  // to succeed and get blocked becoming runnable. This lock ensures that only one thread is
  // requesting suspension of another at any time. As the the thread list suspend thread logic
  // transitions to runnable, if the current thread were tried to be suspended then this thread
  // would block holding this lock until it could safely request thread suspension of the other
  // thread without that thread having a suspension request against this thread. This avoids a
  // potential deadlock cycle.
  static Mutex* thread_list_suspend_thread_lock_;

  // Guards allocation entrypoint instrumenting.
  static Mutex* instrument_entrypoints_lock_ ACQUIRED_AFTER(thread_list_suspend_thread_lock_);

  // The mutator_lock_ is used to allow mutators to execute in a shared (reader) mode or to block
  // mutators by having an exclusive (writer) owner. In normal execution each mutator thread holds
  // a share on the mutator_lock_. The garbage collector may also execute with shared access but
  // at times requires exclusive access to the heap (not to be confused with the heap meta-data
  // guarded by the heap_lock_ below). When the garbage collector requires exclusive access it asks
  // the mutators to suspend themselves which also involves usage of the thread_suspend_count_lock_
  // to cover weaknesses in using ReaderWriterMutexes with ConditionVariables. We use a condition
  // variable to wait upon in the suspension logic as releasing and then re-acquiring a share on
  // the mutator lock doesn't necessarily allow the exclusive user (e.g the garbage collector)
  // chance to acquire the lock.
  //
  // Thread suspension:
  // Shared users                                  | Exclusive user
  // (holding mutator lock and in kRunnable state) |   .. running ..
  //   .. running ..                               | Request thread suspension by:
  //   .. running ..                               |   - acquiring thread_suspend_count_lock_
  //   .. running ..                               |   - incrementing Thread::suspend_count_ on
  //   .. running ..                               |     all mutator threads
  //   .. running ..                               |   - releasing thread_suspend_count_lock_
  //   .. running ..                               | Block trying to acquire exclusive mutator lock
  // Poll Thread::suspend_count_ and enter full    |   .. blocked ..
  // suspend code.                                 |   .. blocked ..
  // Change state to kSuspended                    |   .. blocked ..
  // x: Release share on mutator_lock_             | Carry out exclusive access
  // Acquire thread_suspend_count_lock_            |   .. exclusive ..
  // while Thread::suspend_count_ > 0              |   .. exclusive ..
  //   - wait on Thread::resume_cond_              |   .. exclusive ..
  //     (releases thread_suspend_count_lock_)     |   .. exclusive ..
  //   .. waiting ..                               | Release mutator_lock_
  //   .. waiting ..                               | Request thread resumption by:
  //   .. waiting ..                               |   - acquiring thread_suspend_count_lock_
  //   .. waiting ..                               |   - decrementing Thread::suspend_count_ on
  //   .. waiting ..                               |     all mutator threads
  //   .. waiting ..                               |   - notifying on Thread::resume_cond_
  //    - re-acquire thread_suspend_count_lock_    |   - releasing thread_suspend_count_lock_
  // Release thread_suspend_count_lock_            |  .. running ..
  // Acquire share on mutator_lock_                |  .. running ..
  //  - This could block but the thread still      |  .. running ..
  //    has a state of kSuspended and so this      |  .. running ..
  //    isn't an issue.                            |  .. running ..
  // Acquire thread_suspend_count_lock_            |  .. running ..
  //  - we poll here as we're transitioning into   |  .. running ..
  //    kRunnable and an individual thread suspend |  .. running ..
  //    request (e.g for debugging) won't try      |  .. running ..
  //    to acquire the mutator lock (which would   |  .. running ..
  //    block as we hold the mutator lock). This   |  .. running ..
  //    poll ensures that if the suspender thought |  .. running ..
  //    we were suspended by incrementing our      |  .. running ..
  //    Thread::suspend_count_ and then reading    |  .. running ..
  //    our state we go back to waiting on         |  .. running ..
  //    Thread::resume_cond_.                      |  .. running ..
  // can_go_runnable = Thread::suspend_count_ == 0 |  .. running ..
  // Release thread_suspend_count_lock_            |  .. running ..
  // if can_go_runnable                            |  .. running ..
  //   Change state to kRunnable                   |  .. running ..
  // else                                          |  .. running ..
  //   Goto x                                      |  .. running ..
  //  .. running ..                                |  .. running ..
  static ReaderWriterMutex* mutator_lock_ ACQUIRED_AFTER(instrument_entrypoints_lock_);

  // Allow reader-writer mutual exclusion on the mark and live bitmaps of the heap.
  static ReaderWriterMutex* heap_bitmap_lock_ ACQUIRED_AFTER(mutator_lock_);

  // Guards shutdown of the runtime.
  static Mutex* runtime_shutdown_lock_ ACQUIRED_AFTER(heap_bitmap_lock_);

  // Guards background profiler global state.
  static Mutex* profiler_lock_ ACQUIRED_AFTER(runtime_shutdown_lock_);

  // Guards trace (ie traceview) requests.
  static Mutex* trace_lock_ ACQUIRED_AFTER(profiler_lock_);

  // Guards debugger recent allocation records.
  static Mutex* alloc_tracker_lock_ ACQUIRED_AFTER(trace_lock_);

  // Guards updates to instrumentation to ensure mutual exclusion of
  // events like deoptimization requests.
  // TODO: improve name, perhaps instrumentation_update_lock_.
  static Mutex* deoptimization_lock_ ACQUIRED_AFTER(alloc_tracker_lock_);

  // The thread_list_lock_ guards ThreadList::list_. It is also commonly held to stop threads
  // attaching and detaching.
  static Mutex* thread_list_lock_ ACQUIRED_AFTER(deoptimization_lock_);

  // Guards breakpoints.
  static ReaderWriterMutex* breakpoint_lock_ ACQUIRED_AFTER(trace_lock_);

  // Guards lists of classes within the class linker.
  static ReaderWriterMutex* classlinker_classes_lock_ ACQUIRED_AFTER(breakpoint_lock_);

  // When declaring any Mutex add DEFAULT_MUTEX_ACQUIRED_AFTER to use annotalysis to check the code
  // doesn't try to hold a higher level Mutex.
  #define DEFAULT_MUTEX_ACQUIRED_AFTER ACQUIRED_AFTER(Locks::classlinker_classes_lock_)

  static Mutex* allocated_monitor_ids_lock_ ACQUIRED_AFTER(classlinker_classes_lock_);

  // Guard the allocation/deallocation of thread ids.
  static Mutex* allocated_thread_ids_lock_ ACQUIRED_AFTER(allocated_monitor_ids_lock_);

  // Guards modification of the LDT on x86.
  static Mutex* modify_ldt_lock_ ACQUIRED_AFTER(allocated_thread_ids_lock_);

  // Guards intern table.
  static Mutex* intern_table_lock_ ACQUIRED_AFTER(modify_ldt_lock_);

  // Guards reference processor.
  static Mutex* reference_processor_lock_ ACQUIRED_AFTER(intern_table_lock_);

  // Guards cleared references queue.
  static Mutex* reference_queue_cleared_references_lock_ ACQUIRED_AFTER(reference_processor_lock_);

  // Guards weak references queue.
  static Mutex* reference_queue_weak_references_lock_ ACQUIRED_AFTER(reference_queue_cleared_references_lock_);

  // Guards finalizer references queue.
  static Mutex* reference_queue_finalizer_references_lock_ ACQUIRED_AFTER(reference_queue_weak_references_lock_);

  // Guards phantom references queue.
  static Mutex* reference_queue_phantom_references_lock_ ACQUIRED_AFTER(reference_queue_finalizer_references_lock_);

  // Guards soft references queue.
  static Mutex* reference_queue_soft_references_lock_ ACQUIRED_AFTER(reference_queue_phantom_references_lock_);

  // Have an exclusive aborting thread.
  static Mutex* abort_lock_ ACQUIRED_AFTER(reference_queue_soft_references_lock_);

  // Allow mutual exclusion when manipulating Thread::suspend_count_.
  // TODO: Does the trade-off of a per-thread lock make sense?
  static Mutex* thread_suspend_count_lock_ ACQUIRED_AFTER(abort_lock_);

  // One unexpected signal at a time lock.
  static Mutex* unexpected_signal_lock_ ACQUIRED_AFTER(thread_suspend_count_lock_);

  // Guards the maps in mem_map.
  static Mutex* mem_maps_lock_ ACQUIRED_AFTER(unexpected_signal_lock_);

  // Have an exclusive logging thread.
  static Mutex* logging_lock_ ACQUIRED_AFTER(unexpected_signal_lock_);
};

}  // namespace art

#endif  // ART_RUNTIME_BASE_MUTEX_H_
