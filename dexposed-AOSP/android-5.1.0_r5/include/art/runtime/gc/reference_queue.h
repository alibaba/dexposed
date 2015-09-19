/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef ART_RUNTIME_GC_REFERENCE_QUEUE_H_
#define ART_RUNTIME_GC_REFERENCE_QUEUE_H_

#include <iosfwd>
#include <string>
#include <vector>

#include "atomic.h"
#include "base/timing_logger.h"
#include "globals.h"
#include "gtest/gtest.h"
#include "jni.h"
#include "object_callbacks.h"
#include "offsets.h"
#include "thread_pool.h"

namespace art {
namespace mirror {
class Reference;
}  // namespace mirror

namespace gc {

class Heap;

// Used to temporarily store java.lang.ref.Reference(s) during GC and prior to queueing on the
// appropriate java.lang.ref.ReferenceQueue. The linked list is maintained in the
// java.lang.ref.Reference objects.
class ReferenceQueue {
 public:
  explicit ReferenceQueue(Mutex* lock);
  // Enqueue a reference if is not already enqueued. Thread safe to call from multiple threads
  // since it uses a lock to avoid a race between checking for the references presence and adding
  // it.
  void AtomicEnqueueIfNotEnqueued(Thread* self, mirror::Reference* ref)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) LOCKS_EXCLUDED(lock_);
  // Enqueue a reference, unlike EnqueuePendingReference, enqueue reference checks that the
  // reference IsEnqueueable. Not thread safe, used when mutators are paused to minimize lock
  // overhead.
  void EnqueueReference(mirror::Reference* ref) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void EnqueuePendingReference(mirror::Reference* ref) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  mirror::Reference* DequeuePendingReference() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  // Enqueues finalizer references with white referents.  White referents are blackened, moved to the
  // zombie field, and the referent field is cleared.
  void EnqueueFinalizerReferences(ReferenceQueue* cleared_references,
                                  IsHeapReferenceMarkedCallback* is_marked_callback,
                                  MarkObjectCallback* mark_object_callback, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  // Walks the reference list marking any references subject to the reference clearing policy.
  // References with a black referent are removed from the list.  References with white referents
  // biased toward saving are blackened and also removed from the list.
  void ForwardSoftReferences(IsHeapReferenceMarkedCallback* preserve_callback, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  // Unlink the reference list clearing references objects with white referents.  Cleared references
  // registered to a reference queue are scheduled for appending by the heap worker thread.
  void ClearWhiteReferences(ReferenceQueue* cleared_references,
                            IsHeapReferenceMarkedCallback* is_marked_callback, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void Dump(std::ostream& os) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool IsEmpty() const {
    return list_ == nullptr;
  }
  void Clear() {
    list_ = nullptr;
  }
  mirror::Reference* GetList() {
    return list_;
  }
  // Visits list_, currently only used for the mark compact GC.
  void UpdateRoots(IsMarkedCallback* callback, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

 private:
  // Lock, used for parallel GC reference enqueuing. It allows for multiple threads simultaneously
  // calling AtomicEnqueueIfNotEnqueued.
  Mutex* lock_;
  // The actual reference list. Only a root for the mark compact GC since it will be null for other
  // GC types.
  mirror::Reference* list_;
};

}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_REFERENCE_QUEUE_H_
