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

#ifndef ART_RUNTIME_INSTRUCTION_SET_H_
#define ART_RUNTIME_INSTRUCTION_SET_H_

#include <iosfwd>
#include <string>

#include "base/logging.h"  // Logging is required for FATAL in the helper functions.
#include "base/macros.h"
#include "globals.h"       // For KB.

namespace art {

enum InstructionSet {
  kNone,
  kArm,
  kArm64,
  kThumb2,
  kX86,
  kX86_64,
  kMips,
  kMips64
};
std::ostream& operator<<(std::ostream& os, const InstructionSet& rhs);

#if defined(__arm__)
static constexpr InstructionSet kRuntimeISA = kArm;
#elif defined(__aarch64__)
static constexpr InstructionSet kRuntimeISA = kArm64;
#elif defined(__mips__)
static constexpr InstructionSet kRuntimeISA = kMips;
#elif defined(__i386__)
static constexpr InstructionSet kRuntimeISA = kX86;
#elif defined(__x86_64__)
static constexpr InstructionSet kRuntimeISA = kX86_64;
#else
static constexpr InstructionSet kRuntimeISA = kNone;
#endif

// Architecture-specific pointer sizes
static constexpr size_t kArmPointerSize = 4;
static constexpr size_t kArm64PointerSize = 8;
static constexpr size_t kMipsPointerSize = 4;
static constexpr size_t kX86PointerSize = 4;
static constexpr size_t kX86_64PointerSize = 8;

// ARM instruction alignment. ARM processors require code to be 4-byte aligned,
// but ARM ELF requires 8..
static constexpr size_t kArmAlignment = 8;

// ARM64 instruction alignment. This is the recommended alignment for maximum performance.
static constexpr size_t kArm64Alignment = 16;

// MIPS instruction alignment.  MIPS processors require code to be 4-byte aligned.
// TODO: Can this be 4?
static constexpr size_t kMipsAlignment = 8;

// X86 instruction alignment. This is the recommended alignment for maximum performance.
static constexpr size_t kX86Alignment = 16;


const char* GetInstructionSetString(InstructionSet isa);

// Note: Returns kNone when the string cannot be parsed to a known value.
InstructionSet GetInstructionSetFromString(const char* instruction_set);

static inline size_t GetInstructionSetPointerSize(InstructionSet isa) {
  switch (isa) {
    case kArm:
      // Fall-through.
    case kThumb2:
      return kArmPointerSize;
    case kArm64:
      return kArm64PointerSize;
    case kX86:
      return kX86PointerSize;
    case kX86_64:
      return kX86_64PointerSize;
    case kMips:
      return kMipsPointerSize;
    case kNone:
      LOG(FATAL) << "ISA kNone does not have pointer size.";
      return 0;
    default:
      LOG(FATAL) << "Unknown ISA " << isa;
      return 0;
  }
}

size_t GetInstructionSetAlignment(InstructionSet isa);

static inline bool Is64BitInstructionSet(InstructionSet isa) {
  switch (isa) {
    case kArm:
    case kThumb2:
    case kX86:
    case kMips:
      return false;

    case kArm64:
    case kX86_64:
      return true;

    case kNone:
      LOG(FATAL) << "ISA kNone does not have bit width.";
      return 0;
    default:
      LOG(FATAL) << "Unknown ISA " << isa;
      return 0;
  }
}

static inline size_t GetBytesPerGprSpillLocation(InstructionSet isa) {
  switch (isa) {
    case kArm:
      // Fall-through.
    case kThumb2:
      return 4;
    case kArm64:
      return 8;
    case kX86:
      return 4;
    case kX86_64:
      return 8;
    case kMips:
      return 4;
    case kNone:
      LOG(FATAL) << "ISA kNone does not have spills.";
      return 0;
    default:
      LOG(FATAL) << "Unknown ISA " << isa;
      return 0;
  }
}

static inline size_t GetBytesPerFprSpillLocation(InstructionSet isa) {
  switch (isa) {
    case kArm:
      // Fall-through.
    case kThumb2:
      return 4;
    case kArm64:
      return 8;
    case kX86:
      return 8;
    case kX86_64:
      return 8;
    case kMips:
      return 4;
    case kNone:
      LOG(FATAL) << "ISA kNone does not have spills.";
      return 0;
    default:
      LOG(FATAL) << "Unknown ISA " << isa;
      return 0;
  }
}

size_t GetStackOverflowReservedBytes(InstructionSet isa);

enum InstructionFeatures {
  kHwDiv  = 0x1,              // Supports hardware divide.
  kHwLpae = 0x2,              // Supports Large Physical Address Extension.
};

// This is a bitmask of supported features per architecture.
class PACKED(4) InstructionSetFeatures {
 public:
  InstructionSetFeatures() : mask_(0) {}
  explicit InstructionSetFeatures(uint32_t mask) : mask_(mask) {}

  static InstructionSetFeatures GuessInstructionSetFeatures();

  bool HasDivideInstruction() const {
      return (mask_ & kHwDiv) != 0;
  }

  void SetHasDivideInstruction(bool v) {
    mask_ = (mask_ & ~kHwDiv) | (v ? kHwDiv : 0);
  }

  bool HasLpae() const {
    return (mask_ & kHwLpae) != 0;
  }

  void SetHasLpae(bool v) {
    mask_ = (mask_ & ~kHwLpae) | (v ? kHwLpae : 0);
  }

  std::string GetFeatureString() const;

  // Other features in here.

  bool operator==(const InstructionSetFeatures &peer) const {
    return mask_ == peer.mask_;
  }

  bool operator!=(const InstructionSetFeatures &peer) const {
    return mask_ != peer.mask_;
  }

  bool operator<=(const InstructionSetFeatures &peer) const {
    return (mask_ & peer.mask_) == mask_;
  }

 private:
  uint32_t mask_;
};

// The following definitions create return types for two word-sized entities that will be passed
// in registers so that memory operations for the interface trampolines can be avoided. The entities
// are the resolved method and the pointer to the code to be invoked.
//
// On x86, ARM32 and MIPS, this is given for a *scalar* 64bit value. The definition thus *must* be
// uint64_t or long long int.
//
// On x86_64 and ARM64, structs are decomposed for allocation, so we can create a structs of two
// size_t-sized values.
//
// We need two operations:
//
// 1) A flag value that signals failure. The assembly stubs expect the lower part to be "0".
//    GetTwoWordFailureValue() will return a value that has lower part == 0.
//
// 2) A value that combines two word-sized values.
//    GetTwoWordSuccessValue() constructs this.
//
// IMPORTANT: If you use this to transfer object pointers, it is your responsibility to ensure
//            that the object does not move or the value is updated. Simple use of this is NOT SAFE
//            when the garbage collector can move objects concurrently. Ensure that required locks
//            are held when using!

#if defined(__i386__) || defined(__arm__) || defined(__mips__)
typedef uint64_t TwoWordReturn;

// Encodes method_ptr==nullptr and code_ptr==nullptr
static inline constexpr TwoWordReturn GetTwoWordFailureValue() {
  return 0;
}

// Use the lower 32b for the method pointer and the upper 32b for the code pointer.
static inline TwoWordReturn GetTwoWordSuccessValue(uintptr_t hi, uintptr_t lo) {
  uint32_t lo32 = static_cast<uint32_t>(lo);
  uint64_t hi64 = static_cast<uint64_t>(hi);
  return ((hi64 << 32) | lo32);
}

#elif defined(__x86_64__) || defined(__aarch64__)
struct TwoWordReturn {
  uintptr_t lo;
  uintptr_t hi;
};

// Encodes method_ptr==nullptr. Leaves random value in code pointer.
static inline TwoWordReturn GetTwoWordFailureValue() {
  TwoWordReturn ret;
  ret.lo = 0;
  return ret;
}

// Write values into their respective members.
static inline TwoWordReturn GetTwoWordSuccessValue(uintptr_t hi, uintptr_t lo) {
  TwoWordReturn ret;
  ret.lo = lo;
  ret.hi = hi;
  return ret;
}
#else
#error "Unsupported architecture"
#endif

}  // namespace art

#endif  // ART_RUNTIME_INSTRUCTION_SET_H_
