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

#ifndef ART_RUNTIME_ARCH_X86_QUICK_METHOD_FRAME_INFO_X86_H_
#define ART_RUNTIME_ARCH_X86_QUICK_METHOD_FRAME_INFO_X86_H_

#include "quick/quick_method_frame_info.h"
#include "registers_x86.h"
#include "runtime.h"  // for Runtime::CalleeSaveType.

namespace art {
namespace x86 {

static constexpr uint32_t kX86CalleeSaveRefSpills =
    (1 << art::x86::EBP) | (1 << art::x86::ESI) | (1 << art::x86::EDI);
static constexpr uint32_t kX86CalleeSaveArgSpills =
    (1 << art::x86::ECX) | (1 << art::x86::EDX) | (1 << art::x86::EBX);

constexpr uint32_t X86CalleeSaveCoreSpills(Runtime::CalleeSaveType type) {
  return kX86CalleeSaveRefSpills | (type == Runtime::kRefsAndArgs ? kX86CalleeSaveArgSpills : 0) |
      (1 << art::x86::kNumberOfCpuRegisters);  // fake return address callee save
}

constexpr uint32_t X86CalleeSaveFrameSize(Runtime::CalleeSaveType type) {
  return RoundUp((POPCOUNT(X86CalleeSaveCoreSpills(type)) /* gprs */ +
                  1 /* Method* */) * kX86PointerSize, kStackAlignment);
}

constexpr QuickMethodFrameInfo X86CalleeSaveMethodFrameInfo(Runtime::CalleeSaveType type) {
  return QuickMethodFrameInfo(X86CalleeSaveFrameSize(type),
                              X86CalleeSaveCoreSpills(type),
                              0u);
}

}  // namespace x86
}  // namespace art

#endif  // ART_RUNTIME_ARCH_X86_QUICK_METHOD_FRAME_INFO_X86_H_
