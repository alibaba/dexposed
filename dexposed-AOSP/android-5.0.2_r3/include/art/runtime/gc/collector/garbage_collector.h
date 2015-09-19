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

#ifndef ART_RUNTIME_GC_COLLECTOR_GARBAGE_COLLECTOR_H_
#define ART_RUNTIME_GC_COLLECTOR_GARBAGE_COLLECTOR_H_

#include "base/histogram.h"
#include "base/mutex.h"
#include "base/timing_logger.h"
#include "gc/collector_type.h"
#include "gc/gc_cause.h"
#include "gc_type.h"
#include <stdint.h>
#include <vector>

namespace art {
namespace gc {

class Heap;

namespace collector {

struct ObjectBytePair {
  ObjectBytePair(uint64_t num_objects = 0, int64_t num_bytes = 0)
      : objects(num_objects), bytes(num_bytes) {}
  void Add(const ObjectBytePair& other) {
    objects += other.objects;
    bytes += other.bytes;
  }
  // Number of objects which were freed.
  uint64_t objects;
  // Freed bytes are signed since the GC can free negative bytes if it promotes objects to a space
  // which has a larger allocation size.
  int64_t bytes;
};

// A information related single garbage collector iteration. Since we only ever have one GC running
// at any given time, we can have a single iteration info.
class Iteration {
 public:
  Iteration();
  // Returns how long the mutators were paused in nanoseconds.
  const std::vector<uint64_t>& GetPauseTimes() const {
    return pause_times_;
  }
  TimingLogger* GetTimings() {
    return &timings_;
  }
  // Returns how long the GC took to complete in nanoseconds.
  uint64_t GetDurationNs() const {
    return duration_ns_;
  }
  int64_t GetFreedBytes() const {
    return freed_.bytes;
  }
  int64_t GetFreedLargeObjectBytes() const {
    return freed_los_.bytes;
  }
  uint64_t GetFreedObjects() const {
    return freed_.objects;
  }
  uint64_t GetFreedLargeObjects() const {
    return freed_los_.objects;
  }
  void Reset(GcCause gc_cause, bool clear_soft_references);
  // Returns the estimated throughput of the iteration.
  uint64_t GetEstimatedThroughput() const;
  bool GetClearSoftReferences() const {
    return clear_soft_references_;
  }
  void SetClearSoftReferences(bool clear_soft_references) {
    clear_soft_references_ = clear_soft_references;
  }
  GcCause GetGcCause() const {
    return gc_cause_;
  }

 private:
  void SetDurationNs(uint64_t duration) {
    duration_ns_ = duration;
  }

  GcCause gc_cause_;
  bool clear_soft_references_;
  uint64_t duration_ns_;
  TimingLogger timings_;
  ObjectBytePair freed_;
  ObjectBytePair freed_los_;
  std::vector<uint64_t> pause_times_;

  friend class GarbageCollector;
  DISALLOW_COPY_AND_ASSIGN(Iteration);
};

class GarbageCollector {
 public:
  class SCOPED_LOCKABLE ScopedPause {
   public:
    explicit ScopedPause(GarbageCollector* collector) EXCLUSIVE_LOCK_FUNCTION(Locks::mutator_lock_);
    ~ScopedPause() UNLOCK_FUNCTION();

   private:
    const uint64_t start_time_;
    GarbageCollector* const collector_;
  };

  GarbageCollector(Heap* heap, const std::string& name);
  virtual ~GarbageCollector() { }

  const char* GetName() const {
    return name_.c_str();
  }

  virtual GcType GetGcType() const = 0;

  virtual CollectorType GetCollectorType() const = 0;

  // Run the garbage collector.
  void Run(GcCause gc_cause, bool clear_soft_references);

  Heap* GetHeap() const {
    return heap_;
  }
  void RegisterPause(uint64_t nano_length);
  const CumulativeLogger& GetCumulativeTimings() const {
    return cumulative_timings_;
  }

  void ResetCumulativeStatistics();

  // Swap the live and mark bitmaps of spaces that are active for the collector. For partial GC,
  // this is the allocation space, for full GC then we swap the zygote bitmaps too.
  void SwapBitmaps() EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_);
  uint64_t GetTotalPausedTimeNs() const {
    return pause_histogram_.AdjustedSum();
  }
  int64_t GetTotalFreedBytes() const {
    return total_freed_bytes_;
  }
  uint64_t GetTotalFreedObjects() const {
    return total_freed_objects_;
  }
  const Histogram<uint64_t>& GetPauseHistogram() const {
    return pause_histogram_;
  }
  // Reset the cumulative timings and pause histogram.
  void ResetMeasurements();
  // Returns the estimated throughput in bytes / second.
  uint64_t GetEstimatedMeanThroughput() const;
  // Returns how many GC iterations have been run.
  size_t NumberOfIterations() const {
    return GetCumulativeTimings().GetIterations();
  }
  // Returns the current GC iteration and assocated info.
  Iteration* GetCurrentIteration();
  const Iteration* GetCurrentIteration() const;
  TimingLogger* GetTimings() {
    return &GetCurrentIteration()->timings_;
  }
  // Record a free of normal objects.
  void RecordFree(const ObjectBytePair& freed);
  // Record a free of large objects.
  void RecordFreeLOS(const ObjectBytePair& freed);

 protected:
  // Run all of the GC phases.
  virtual void RunPhases() = 0;

  // Revoke all the thread-local buffers.
  virtual void RevokeAllThreadLocalBuffers() = 0;

  static constexpr size_t kPauseBucketSize = 500;
  static constexpr size_t kPauseBucketCount = 32;

  Heap* const heap_;
  std::string name_;
  // Cumulative statistics.
  Histogram<uint64_t> pause_histogram_;
  uint64_t total_time_ns_;
  uint64_t total_freed_objects_;
  int64_t total_freed_bytes_;
  CumulativeLogger cumulative_timings_;
};

}  // namespace collector
}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_COLLECTOR_GARBAGE_COLLECTOR_H_
