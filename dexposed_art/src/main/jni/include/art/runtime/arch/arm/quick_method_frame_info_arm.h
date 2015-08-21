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

#ifndef ART_RUNTIME_ARCH_ARM_QUICK_METHOD_FRAME_INFO_ARM_H_
#define ART_RUNTIME_ARCH_ARM_QUICK_METHOD_FRAME_INFO_ARM_H_

#include "quick/quick_method_frame_info.h"
#include "registers_arm.h"
#include "runtime.h"  // for Runtime::CalleeSaveType.
#include "utils.h"

namespace art {
namespace arm {

static constexpr uint32_t kArmCalleeSaveRefSpills =
    (1 << art::arm::R5) | (1 << art::arm::R6)  | (1 << art::arm::R7) | (1 << art::arm::R8) |
    (1 << art::arm::R10) | (1 << art::arm::R11);
static constexpr uint32_t kArmCalleeSaveArgSpills =
    (1 << art::arm::R1) | (1 << art::arm::R2) | (1 << art::arm::R3);
static constexpr uint32_t kArmCalleeSaveAllSpills =
    (1 << art::arm::R4) | (1 << art::arm::R9);
static constexpr uint32_t kArmCalleeSaveFpAllSpills =
    (1 << art::arm::S0)  | (1 << art::arm::S1)  | (1 << art::arm::S2)  | (1 << art::arm::S3)  |
    (1 << art::arm::S4)  | (1 << art::arm::S5)  | (1 << art::arm::S6)  | (1 << art::arm::S7)  |
    (1 << art::arm::S8)  | (1 << art::arm::S9)  | (1 << art::arm::S10) | (1 << art::arm::S11) |
    (1 << art::arm::S12) | (1 << art::arm::S13) | (1 << art::arm::S14) | (1 << art::arm::S15) |
    (1 << art::arm::S16) | (1 << art::arm::S17) | (1 << art::arm::S18) | (1 << art::arm::S19) |
    (1 << art::arm::S20) | (1 << art::arm::S21) | (1 << art::arm::S22) | (1 << art::arm::S23) |
    (1 << art::arm::S24) | (1 << art::arm::S25) | (1 << art::arm::S26) | (1 << art::arm::S27) |
    (1 << art::arm::S28) | (1 << art::arm::S29) | (1 << art::arm::S30) | (1 << art::arm::S31);

constexpr uint32_t ArmCalleeSaveCoreSpills(Runtime::CalleeSaveType type) {
  return kArmCalleeSaveRefSpills | (type == Runtime::kRefsAndArgs ? kArmCalleeSaveArgSpills : 0) |
      (type == Runtime::kSaveAll ? kArmCalleeSaveAllSpills : 0) | (1 << art::arm::LR);
}

constexpr uint32_t ArmCalleeSaveFpSpills(Runtime::CalleeSaveType type) {
  return type == Runtime::kSaveAll ? kArmCalleeSaveFpAllSpills : 0;
}

constexpr uint32_t ArmCalleeSaveFrameSize(Runtime::CalleeSaveType type) {
  return RoundUp((POPCOUNT(ArmCalleeSaveCoreSpills(type)) /* gprs */ +
                  POPCOUNT(ArmCalleeSaveFpSpills(type)) /* fprs */ +
                  1 /* Method* */) * kArmPointerSize, kStackAlignment);
}

constexpr QuickMethodFrameInfo ArmCalleeSaveMethodFrameInfo(Runtime::CalleeSaveType type) {
  return QuickMethodFrameInfo(ArmCalleeSaveFrameSize(type),
                              ArmCalleeSaveCoreSpills(type),
                              ArmCalleeSaveFpSpills(type));
}

constexpr size_t ArmCalleeSaveFpr1Offset(Runtime::CalleeSaveType type) {
  return ArmCalleeSaveFrameSize(type) -
         (POPCOUNT(ArmCalleeSaveCoreSpills(type)) +
          POPCOUNT(ArmCalleeSaveFpSpills(type))) * kArmPointerSize;
}

constexpr size_t ArmCalleeSaveGpr1Offset(Runtime::CalleeSaveType type) {
  return ArmCalleeSaveFrameSize(type) -
         POPCOUNT(ArmCalleeSaveCoreSpills(type)) * kArmPointerSize;
}

constexpr size_t ArmCalleeSaveLrOffset(Runtime::CalleeSaveType type) {
  return ArmCalleeSaveFrameSize(type) -
      POPCOUNT(ArmCalleeSaveCoreSpills(type) & (-(1 << LR))) * kArmPointerSize;
}

}  // namespace arm
}  // namespace art

#endif  // ART_RUNTIME_ARCH_ARM_QUICK_METHOD_FRAME_INFO_ARM_H_
