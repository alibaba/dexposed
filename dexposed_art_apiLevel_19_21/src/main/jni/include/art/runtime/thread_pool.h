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

#ifndef ART_RUNTIME_THREAD_POOL_H_
#define ART_RUNTIME_THREAD_POOL_H_

#include <deque>
#include <vector>

#include "barrier.h"
#include "base/mutex.h"
#include "closure.h"
#include "mem_map.h"

namespace art {

class ThreadPool;

class Task : public Closure {
 public:
  // Called when references reaches 0.
  virtual void Finalize() { }
};

class ThreadPoolWorker {
 public:
  static const size_t kDefaultStackSize = 1 * MB;

  size_t GetStackSize() const {
    DCHECK(stack_.get() != nullptr);
    return stack_->Size();
  }

  virtual ~ThreadPoolWorker();

 protected:
  ThreadPoolWorker(ThreadPool* thread_pool, const std::string& name, size_t stack_size);
  static void* Callback(void* arg) LOCKS_EXCLUDED(Locks::mutator_lock_);
  virtual void Run();

  ThreadPool* const thread_pool_;
  const std::string name_;
  std::unique_ptr<MemMap> stack_;
  pthread_t pthread_;

 private:
  friend class ThreadPool;
  DISALLOW_COPY_AND_ASSIGN(ThreadPoolWorker);
};

class ThreadPool {
 public:
  // Returns the number of threads in the thread pool.
  size_t GetThreadCount() const {
    return threads_.size();
  }

  // Broadcast to the workers and tell them to empty out the work queue.
  void StartWorkers(Thread* self);

  // Do not allow workers to grab any new tasks.
  void StopWorkers(Thread* self);

  // Add a new task, the first available started worker will process it. Does not delete the task
  // after running it, it is the caller's responsibility.
  void AddTask(Thread* self, Task* task);

  explicit ThreadPool(const char* name, size_t num_threads);
  virtual ~ThreadPool();

  // Wait for all tasks currently on queue to get completed.
  void Wait(Thread* self, bool do_work, bool may_hold_locks);

  size_t GetTaskCount(Thread* self);

  // Returns the total amount of workers waited for tasks.
  uint64_t GetWaitTime() const {
    return total_wait_time_;
  }

  // Provides a way to bound the maximum number of worker threads, threads must be less the the
  // thread count of the thread pool.
  void SetMaxActiveWorkers(size_t threads);

 protected:
  // get a task to run, blocks if there are no tasks left
  virtual Task* GetTask(Thread* self);

  // Try to get a task, returning NULL if there is none available.
  Task* TryGetTask(Thread* self);
  Task* TryGetTaskLocked(Thread* self) EXCLUSIVE_LOCKS_REQUIRED(task_queue_lock_);

  // Are we shutting down?
  bool IsShuttingDown() const EXCLUSIVE_LOCKS_REQUIRED(task_queue_lock_) {
    return shutting_down_;
  }

  const std::string name_;
  Mutex task_queue_lock_;
  ConditionVariable task_queue_condition_ GUARDED_BY(task_queue_lock_);
  ConditionVariable completion_condition_ GUARDED_BY(task_queue_lock_);
  volatile bool started_ GUARDED_BY(task_queue_lock_);
  volatile bool shutting_down_ GUARDED_BY(task_queue_lock_);
  // How many worker threads are waiting on the condition.
  volatile size_t waiting_count_ GUARDED_BY(task_queue_lock_);
  std::deque<Task*> tasks_ GUARDED_BY(task_queue_lock_);
  // TODO: make this immutable/const?
  std::vector<ThreadPoolWorker*> threads_;
  // Work balance detection.
  uint64_t start_time_ GUARDED_BY(task_queue_lock_);
  uint64_t total_wait_time_;
  Barrier creation_barier_;
  size_t max_active_workers_ GUARDED_BY(task_queue_lock_);

 private:
  friend class ThreadPoolWorker;
  friend class WorkStealingWorker;
  DISALLOW_COPY_AND_ASSIGN(ThreadPool);
};

class WorkStealingTask : public Task {
 public:
  WorkStealingTask() : ref_count_(0) {}

  size_t GetRefCount() const {
    return ref_count_;
  }

  virtual void StealFrom(Thread* self, WorkStealingTask* source) = 0;

 private:
  // How many people are referencing this task.
  size_t ref_count_;

  friend class WorkStealingWorker;
};

class WorkStealingWorker : public ThreadPoolWorker {
 public:
  virtual ~WorkStealingWorker();

  bool IsRunningTask() const {
    return task_ != NULL;
  }

 protected:
  WorkStealingTask* task_;

  WorkStealingWorker(ThreadPool* thread_pool, const std::string& name, size_t stack_size);
  virtual void Run();

 private:
  friend class WorkStealingThreadPool;
  DISALLOW_COPY_AND_ASSIGN(WorkStealingWorker);
};

class WorkStealingThreadPool : public ThreadPool {
 public:
  explicit WorkStealingThreadPool(const char* name, size_t num_threads);
  virtual ~WorkStealingThreadPool();

 private:
  Mutex work_steal_lock_;
  // Which thread we are stealing from (round robin).
  size_t steal_index_;

  // Find a task to steal from
  WorkStealingTask* FindTaskToStealFrom(Thread* self) EXCLUSIVE_LOCKS_REQUIRED(work_steal_lock_);

  friend class WorkStealingWorker;
};

}  // namespace art

#endif  // ART_RUNTIME_THREAD_POOL_H_
