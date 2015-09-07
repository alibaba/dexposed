/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef ART_RUNTIME_ARCH_ARM_REGISTERS_ARM_H_
#define ART_RUNTIME_ARCH_ARM_REGISTERS_ARM_H_

#include <iosfwd>

namespace art {
namespace arm {

// Values for registers.
enum Register {
  R0  =  0,
  R1  =  1,
  R2  =  2,
  R3  =  3,
  R4  =  4,
  R5  =  5,
  R6  =  6,
  R7  =  7,
  R8  =  8,
  R9  =  9,
  R10 = 10,
  R11 = 11,
  R12 = 12,
  R13 = 13,
  R14 = 14,
  R15 = 15,
  TR  = 9,  // thread register
  FP  = 11,
  IP  = 12,
  SP  = 13,
  LR  = 14,
  PC  = 15,
  kNumberOfCoreRegisters = 16,
  kNoRegister = -1,
};
std::ostream& operator<<(std::ostream& os, const Register& rhs);


// Values for single-precision floating point registers.
enum SRegister {
  S0  =  0,
  S1  =  1,
  S2  =  2,
  S3  =  3,
  S4  =  4,
  S5  =  5,
  S6  =  6,
  S7  =  7,
  S8  =  8,
  S9  =  9,
  S10 = 10,
  S11 = 11,
  S12 = 12,
  S13 = 13,
  S14 = 14,
  S15 = 15,
  S16 = 16,
  S17 = 17,
  S18 = 18,
  S19 = 19,
  S20 = 20,
  S21 = 21,
  S22 = 22,
  S23 = 23,
  S24 = 24,
  S25 = 25,
  S26 = 26,
  S27 = 27,
  S28 = 28,
  S29 = 29,
  S30 = 30,
  S31 = 31,
  kNumberOfSRegisters = 32,
  kNoSRegister = -1,
};
std::ostream& operator<<(std::ostream& os, const SRegister& rhs);

}  // namespace arm
}  // namespace art

#endif  // ART_RUNTIME_ARCH_ARM_REGISTERS_ARM_H_
