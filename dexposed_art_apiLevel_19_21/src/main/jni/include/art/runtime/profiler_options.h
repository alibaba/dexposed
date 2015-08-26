/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef ART_RUNTIME_PROFILER_OPTIONS_H_
#define ART_RUNTIME_PROFILER_OPTIONS_H_

#include <string>
#include <ostream>

namespace art {

enum ProfileDataType {
  kProfilerMethod,          // Method only
  kProfilerBoundedStack,    // Methods with Dex PC on top of the stack
};

class ProfilerOptions {
 public:
  static constexpr bool kDefaultEnabled = false;
  static constexpr uint32_t kDefaultPeriodS = 10;
  static constexpr uint32_t kDefaultDurationS = 20;
  static constexpr uint32_t kDefaultIntervalUs = 500;
  static constexpr double kDefaultBackoffCoefficient = 2.0;
  static constexpr bool kDefaultStartImmediately = false;
  static constexpr double kDefaultTopKThreshold = 90.0;
  static constexpr double kDefaultChangeInTopKThreshold = 10.0;
  static constexpr ProfileDataType kDefaultProfileData = kProfilerMethod;
  static constexpr uint32_t kDefaultMaxStackDepth = 3;

  ProfilerOptions() :
    enabled_(kDefaultEnabled),
    period_s_(kDefaultPeriodS),
    duration_s_(kDefaultDurationS),
    interval_us_(kDefaultIntervalUs),
    backoff_coefficient_(kDefaultBackoffCoefficient),
    start_immediately_(kDefaultStartImmediately),
    top_k_threshold_(kDefaultTopKThreshold),
    top_k_change_threshold_(kDefaultChangeInTopKThreshold),
    profile_type_(kDefaultProfileData),
    max_stack_depth_(kDefaultMaxStackDepth) {}

  ProfilerOptions(bool enabled,
                 uint32_t period_s,
                 uint32_t duration_s,
                 uint32_t interval_us,
                 double backoff_coefficient,
                 bool start_immediately,
                 double top_k_threshold,
                 double top_k_change_threshold,
                 ProfileDataType profile_type,
                 uint32_t max_stack_depth):
    enabled_(enabled),
    period_s_(period_s),
    duration_s_(duration_s),
    interval_us_(interval_us),
    backoff_coefficient_(backoff_coefficient),
    start_immediately_(start_immediately),
    top_k_threshold_(top_k_threshold),
    top_k_change_threshold_(top_k_change_threshold),
    profile_type_(profile_type),
    max_stack_depth_(max_stack_depth) {}

  bool IsEnabled() const {
    return enabled_;
  }

  uint32_t GetPeriodS() const {
    return period_s_;
  }

  uint32_t GetDurationS() const {
    return duration_s_;
  }

  uint32_t GetIntervalUs() const {
    return interval_us_;
  }

  double GetBackoffCoefficient() const {
    return backoff_coefficient_;
  }

  bool GetStartImmediately() const {
    return start_immediately_;
  }

  double GetTopKThreshold() const {
    return top_k_threshold_;
  }

  double GetTopKChangeThreshold() const {
    return top_k_change_threshold_;
  }

  ProfileDataType GetProfileType() const {
    return profile_type_;
  }

  uint32_t GetMaxStackDepth() const {
    return max_stack_depth_;
  }

 private:
  friend std::ostream & operator<<(std::ostream &os, const ProfilerOptions& po) {
    os << "enabled=" << po.enabled_
       << ", period_s=" << po.period_s_
       << ", duration_s=" << po.duration_s_
       << ", interval_us=" << po.interval_us_
       << ", backoff_coefficient=" << po.backoff_coefficient_
       << ", start_immediately=" << po.start_immediately_
       << ", top_k_threshold=" << po.top_k_threshold_
       << ", top_k_change_threshold=" << po.top_k_change_threshold_
       << ", profile_type=" << po.profile_type_
       << ", max_stack_depth=" << po.max_stack_depth_;
    return os;
  }

  friend class ParsedOptions;

  // Whether or not the applications should be profiled.
  bool enabled_;
  // Generate profile every n seconds.
  uint32_t period_s_;
  // Run profile for n seconds.
  uint32_t duration_s_;
  // Microseconds between samples.
  uint32_t interval_us_;
  // Coefficient to exponential backoff.
  double backoff_coefficient_;
  // Whether the profile should start upon app startup or be delayed by some random offset.
  bool start_immediately_;
  // Top K% of samples that are considered relevant when deciding if the app should be recompiled.
  double top_k_threshold_;
  // How much the top K% samples needs to change in order for the app to be recompiled.
  double top_k_change_threshold_;
  // The type of profile data dumped to the disk.
  ProfileDataType profile_type_;
  // The max depth of the stack collected by the profiler
  uint32_t max_stack_depth_;
};

}  // namespace art


#endif  // ART_RUNTIME_PROFILER_OPTIONS_H_
