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

#ifndef ART_RUNTIME_BASE_TIMING_LOGGER_H_
#define ART_RUNTIME_BASE_TIMING_LOGGER_H_

#include "base/histogram.h"
#include "base/macros.h"
#include "base/mutex.h"

#include <set>
#include <string>
#include <vector>

namespace art {
class TimingLogger;

class CumulativeLogger {
 public:
  explicit CumulativeLogger(const std::string& name);
  ~CumulativeLogger();
  void Start();
  void End() LOCKS_EXCLUDED(lock_);
  void Reset() LOCKS_EXCLUDED(lock_);
  void Dump(std::ostream& os) const LOCKS_EXCLUDED(lock_);
  uint64_t GetTotalNs() const {
    return GetTotalTime() * kAdjust;
  }
  // Allow the name to be modified, particularly when the cumulative logger is a field within a
  // parent class that is unable to determine the "name" of a sub-class.
  void SetName(const std::string& name) LOCKS_EXCLUDED(lock_);
  void AddLogger(const TimingLogger& logger) LOCKS_EXCLUDED(lock_);
  size_t GetIterations() const;

 private:
  class HistogramComparator {
   public:
    bool operator()(const Histogram<uint64_t>* a, const Histogram<uint64_t>* b) const {
      return a->Name() < b->Name();
    }
  };

  static constexpr size_t kLowMemoryBucketCount = 16;
  static constexpr size_t kDefaultBucketCount = 100;
  static constexpr size_t kInitialBucketSize = 50;  // 50 microseconds.

  void AddPair(const std::string &label, uint64_t delta_time)
      EXCLUSIVE_LOCKS_REQUIRED(lock_);
  void DumpHistogram(std::ostream &os) const EXCLUSIVE_LOCKS_REQUIRED(lock_);
  uint64_t GetTotalTime() const {
    return total_time_;
  }
  static const uint64_t kAdjust = 1000;
  std::set<Histogram<uint64_t>*, HistogramComparator> histograms_ GUARDED_BY(lock_);
  std::string name_;
  const std::string lock_name_;
  mutable Mutex lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;
  size_t iterations_ GUARDED_BY(lock_);
  uint64_t total_time_;

  DISALLOW_COPY_AND_ASSIGN(CumulativeLogger);
};

// A timing logger that knows when a split starts for the purposes of logging tools, like systrace.
class TimingLogger {
 public:
  static constexpr size_t kIndexNotFound = static_cast<size_t>(-1);

  class Timing {
   public:
    Timing(uint64_t time, const char* name) : time_(time), name_(name) {
    }
    bool IsStartTiming() const {
      return !IsEndTiming();
    }
    bool IsEndTiming() const {
      return name_ == nullptr;
    }
    uint64_t GetTime() const {
      return time_;
    }
    const char* GetName() const {
      return name_;
    }

   private:
    uint64_t time_;
    const char* name_;
  };

  // Extra data that is only calculated when you call dump to prevent excess allocation.
  class TimingData {
   public:
    TimingData() = default;
    TimingData(TimingData&& other) {
      std::swap(data_, other.data_);
    }
    TimingData& operator=(TimingData&& other) {
      std::swap(data_, other.data_);
      return *this;
    }
    uint64_t GetTotalTime(size_t idx) {
      return data_[idx].total_time;
    }
    uint64_t GetExclusiveTime(size_t idx) {
      return data_[idx].exclusive_time;
    }

   private:
    // Each begin split has a total time and exclusive time. Exclusive time is total time - total
    // time of children nodes.
    struct CalculatedDataPoint {
      CalculatedDataPoint() : total_time(0), exclusive_time(0) {}
      uint64_t total_time;
      uint64_t exclusive_time;
    };
    std::vector<CalculatedDataPoint> data_;
    friend class TimingLogger;
  };

  explicit TimingLogger(const char* name, bool precise, bool verbose);
  ~TimingLogger();
  // Verify that all open timings have related closed timings.
  void Verify();
  // Clears current timings and labels.
  void Reset();
  // Starts a timing.
  void StartTiming(const char* new_split_label);
  // Ends the current timing.
  void EndTiming();
  // End the current timing and start a new timing. Usage not recommended.
  void NewTiming(const char* new_split_label) {
    EndTiming();
    StartTiming(new_split_label);
  }
  // Returns the total duration of the timings (sum of total times).
  uint64_t GetTotalNs() const;
  // Find the index of a timing by name.
  size_t FindTimingIndex(const char* name, size_t start_idx) const;
  void Dump(std::ostream& os, const char* indent_string = "  ") const;

  // Scoped timing splits that can be nested and composed with the explicit split
  // starts and ends.
  class ScopedTiming {
   public:
    explicit ScopedTiming(const char* label, TimingLogger* logger) : logger_(logger) {
      logger_->StartTiming(label);
    }
    ~ScopedTiming() {
      logger_->EndTiming();
    }
    // Closes the current timing and opens a new timing.
    void NewTiming(const char* label) {
      logger_->NewTiming(label);
    }

   private:
    TimingLogger* const logger_;  // The timing logger which the scoped timing is associated with.
    DISALLOW_COPY_AND_ASSIGN(ScopedTiming);
  };

  // Return the time points of when each start / end timings start and finish.
  const std::vector<Timing>& GetTimings() const {
    return timings_;
  }

  TimingData CalculateTimingData() const;

 protected:
  // The name of the timing logger.
  const char* const name_;
  // Do we want to print the exactly recorded split (true) or round down to the time unit being
  // used (false).
  const bool precise_;
  // Verbose logging.
  const bool verbose_;
  // Timing points that are either start or end points. For each starting point ret[i] = location
  // of end split associated with i. If it is and end split ret[i] = i.
  std::vector<Timing> timings_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TimingLogger);
};

}  // namespace art

#endif  // ART_RUNTIME_BASE_TIMING_LOGGER_H_
