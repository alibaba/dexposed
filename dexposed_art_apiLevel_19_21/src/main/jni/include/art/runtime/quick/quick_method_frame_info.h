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

#ifndef ART_RUNTIME_QUICK_QUICK_METHOD_FRAME_INFO_H_
#define ART_RUNTIME_QUICK_QUICK_METHOD_FRAME_INFO_H_

#include <stdint.h>

#include "base/macros.h"

namespace art {

class PACKED(4) QuickMethodFrameInfo {
 public:
  constexpr QuickMethodFrameInfo()
    : frame_size_in_bytes_(0u),
      core_spill_mask_(0u),
      fp_spill_mask_(0u) {
  }

  constexpr QuickMethodFrameInfo(uint32_t frame_size_in_bytes, uint32_t core_spill_mask,
                                 uint32_t fp_spill_mask)
    : frame_size_in_bytes_(frame_size_in_bytes),
      core_spill_mask_(core_spill_mask),
      fp_spill_mask_(fp_spill_mask) {
  }

  uint32_t FrameSizeInBytes() const {
    return frame_size_in_bytes_;
  }

  uint32_t CoreSpillMask() const {
    return core_spill_mask_;
  }

  uint32_t FpSpillMask() const {
    return fp_spill_mask_;
  }

 private:
  uint32_t frame_size_in_bytes_;
  uint32_t core_spill_mask_;
  uint32_t fp_spill_mask_;
};

}  // namespace art

#endif  // ART_RUNTIME_QUICK_QUICK_METHOD_FRAME_INFO_H_
