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
#ifndef ART_RUNTIME_BASE_HISTOGRAM_H_
#define ART_RUNTIME_BASE_HISTOGRAM_H_

#include <vector>
#include <string>

#include "base/logging.h"
#include "utils.h"

namespace art {

// Creates a data histogram  for a better understanding of statistical data.
// Histogram analysis goes beyond simple mean and standard deviation to provide
// percentiles values, describing where the $% of the input data lies.
// Designed to be simple and used with timing logger in art.

template <class Value> class Histogram {
  const double kAdjust;
  const size_t kInitialBucketCount;

 public:
  class CumulativeData {
    friend class Histogram<Value>;
    std::vector<uint64_t> freq_;
    std::vector<double> perc_;
  };

  // Used by the cumulative timing logger to search the histogram set using for an existing split
  // with the same name using CumulativeLogger::HistogramComparator.
  explicit Histogram(const char* name);
  // This is the expected constructor when creating new Histograms.
  Histogram(const char* name, Value initial_bucket_width, size_t max_buckets = 100);
  void AddValue(Value);
  // Builds the cumulative distribution function from the frequency data.
  // Accumulative summation of frequencies.
  // cumulative_freq[i] = sum(frequency[j] : 0 < j < i )
  // Accumulative summation of percentiles; which is the frequency / SampleSize
  // cumulative_perc[i] = sum(frequency[j] / SampleSize : 0 < j < i )
  void CreateHistogram(CumulativeData* data) const;
  // Reset the cumulative values, next time CreateHistogram is called it will recreate the cache.
  void Reset();
  double Mean() const;
  double Variance() const;
  double Percentile(double per, const CumulativeData& data) const;
  void PrintConfidenceIntervals(std::ostream& os, double interval,
                                const CumulativeData& data) const;
  void PrintBins(std::ostream& os, const CumulativeData& data) const;
  Value GetRange(size_t bucket_idx) const;
  size_t GetBucketCount() const;

  uint64_t SampleSize() const {
    return sample_size_;
  }

  Value Sum() const {
    return sum_;
  }

  Value AdjustedSum() const {
    return sum_ * kAdjust;
  }

  Value Min() const {
    return min_value_added_;
  }

  Value Max() const {
    return max_value_added_;
  }

  const std::string& Name() const {
    return name_;
  }

 private:
  void Initialize();
  size_t FindBucket(Value val) const;
  void BucketiseValue(Value val);
  // Add more buckets to the histogram to fill in a new value that exceeded
  // the max_read_value_.
  void GrowBuckets(Value val);
  std::string name_;
  // Maximum number of buckets.
  const size_t max_buckets_;
  // Number of samples placed in histogram.
  size_t sample_size_;
  // Width of the bucket range. The lower the value is the more accurate
  // histogram percentiles are. Grows adaptively when we hit max buckets.
  Value bucket_width_;
  // How many occurrences of values fall within a bucket at index i where i covers the range
  // starting at  min_ + i * bucket_width_ with size bucket_size_.
  std::vector<uint32_t> frequency_;
  // Summation of all the elements inputed by the user.
  Value sum_;
  // Minimum value that can fit in the histogram. Fixed to zero for now.
  Value min_;
  // Maximum value that can fit in the histogram, grows adaptively.
  Value max_;
  // Summation of the values entered. Used to calculate variance.
  Value sum_of_squares_;
  // Maximum value entered in the histogram.
  Value min_value_added_;
  // Minimum value entered in the histogram.
  Value max_value_added_;

  DISALLOW_COPY_AND_ASSIGN(Histogram);
};
}  // namespace art

#endif  // ART_RUNTIME_BASE_HISTOGRAM_H_
