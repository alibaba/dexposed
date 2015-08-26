/*
 * Original work Copyright (c) 2005-2008, The Android Open Source Project
 * Modified work Copyright (c) 2013, rovo89 and Tungstwenty
 * Modified work Copyright (c) 2015, Alibaba Mobile Infrastructure (Android) Team
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

#include "callee_save_frame.h"
#include "entrypoints/entrypoint_utils-inl.h"

namespace art {

// Visits the arguments as saved to the stack by a Runtime::kRefAndArgs callee save frame.
class QuickArgumentVisitor {
  // Number of bytes for each out register in the caller method's frame.
  static constexpr size_t kBytesStackArgLocation = 4;
  // Frame size in bytes of a callee-save frame for RefsAndArgs.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_FrameSize =
      GetCalleeSaveFrameSize(kRuntimeISA, Runtime::kRefsAndArgs);
#if defined(__arm__)
  // The callee save frame is pointed to by SP.
  // | argN       |  |
  // | ...        |  |
  // | arg4       |  |
  // | arg3 spill |  |  Caller's frame
  // | arg2 spill |  |
  // | arg1 spill |  |
  // | Method*    | ---
  // | LR         |
  // | ...        |    callee saves
  // | R3         |    arg3
  // | R2         |    arg2
  // | R1         |    arg1
  // | R0         |    padding
  // | Method*    |  <- sp
  static constexpr bool kQuickSoftFloatAbi = true;  // This is a soft float ABI.
  static constexpr size_t kNumQuickGprArgs = 3;  // 3 arguments passed in GPRs.
  static constexpr size_t kNumQuickFprArgs = 0;  // 0 arguments passed in FPRs.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_Fpr1Offset =
      arm::ArmCalleeSaveFpr1Offset(Runtime::kRefsAndArgs);  // Offset of first FPR arg.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_Gpr1Offset =
      arm::ArmCalleeSaveGpr1Offset(Runtime::kRefsAndArgs);  // Offset of first GPR arg.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_LrOffset =
      arm::ArmCalleeSaveLrOffset(Runtime::kRefsAndArgs);  // Offset of return address.
  static size_t GprIndexToGprOffset(uint32_t gpr_index) {
    return gpr_index * GetBytesPerGprSpillLocation(kRuntimeISA);
  }
#elif defined(__aarch64__)
  // The callee save frame is pointed to by SP.
  // | argN       |  |
  // | ...        |  |
  // | arg4       |  |
  // | arg3 spill |  |  Caller's frame
  // | arg2 spill |  |
  // | arg1 spill |  |
  // | Method*    | ---
  // | LR         |
  // | X29        |
  // |  :         |
  // | X20        |
  // | X7         |
  // | :          |
  // | X1         |
  // | D7         |
  // |  :         |
  // | D0         |
  // |            |    padding
  // | Method*    |  <- sp
  static constexpr bool kQuickSoftFloatAbi = false;  // This is a hard float ABI.
  static constexpr size_t kNumQuickGprArgs = 7;  // 7 arguments passed in GPRs.
  static constexpr size_t kNumQuickFprArgs = 8;  // 8 arguments passed in FPRs.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_Fpr1Offset =
      arm64::Arm64CalleeSaveFpr1Offset(Runtime::kRefsAndArgs);  // Offset of first FPR arg.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_Gpr1Offset =
      arm64::Arm64CalleeSaveGpr1Offset(Runtime::kRefsAndArgs);  // Offset of first GPR arg.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_LrOffset =
      arm64::Arm64CalleeSaveLrOffset(Runtime::kRefsAndArgs);  // Offset of return address.
  static size_t GprIndexToGprOffset(uint32_t gpr_index) {
    return gpr_index * GetBytesPerGprSpillLocation(kRuntimeISA);
  }
#elif defined(__mips__)
  // The callee save frame is pointed to by SP.
  // | argN       |  |
  // | ...        |  |
  // | arg4       |  |
  // | arg3 spill |  |  Caller's frame
  // | arg2 spill |  |
  // | arg1 spill |  |
  // | Method*    | ---
  // | RA         |
  // | ...        |    callee saves
  // | A3         |    arg3
  // | A2         |    arg2
  // | A1         |    arg1
  // | A0/Method* |  <- sp
  static constexpr bool kQuickSoftFloatAbi = true;  // This is a soft float ABI.
  static constexpr size_t kNumQuickGprArgs = 3;  // 3 arguments passed in GPRs.
  static constexpr size_t kNumQuickFprArgs = 0;  // 0 arguments passed in FPRs.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_Fpr1Offset = 0;  // Offset of first FPR arg.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_Gpr1Offset = 4;  // Offset of first GPR arg.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_LrOffset = 60;  // Offset of return address.
  static size_t GprIndexToGprOffset(uint32_t gpr_index) {
    return gpr_index * GetBytesPerGprSpillLocation(kRuntimeISA);
  }
#elif defined(__i386__)
  // The callee save frame is pointed to by SP.
  // | argN        |  |
  // | ...         |  |
  // | arg4        |  |
  // | arg3 spill  |  |  Caller's frame
  // | arg2 spill  |  |
  // | arg1 spill  |  |
  // | Method*     | ---
  // | Return      |
  // | EBP,ESI,EDI |    callee saves
  // | EBX         |    arg3
  // | EDX         |    arg2
  // | ECX         |    arg1
  // | EAX/Method* |  <- sp
  static constexpr bool kQuickSoftFloatAbi = true;  // This is a soft float ABI.
  static constexpr size_t kNumQuickGprArgs = 3;  // 3 arguments passed in GPRs.
  static constexpr size_t kNumQuickFprArgs = 0;  // 0 arguments passed in FPRs.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_Fpr1Offset = 0;  // Offset of first FPR arg.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_Gpr1Offset = 4;  // Offset of first GPR arg.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_LrOffset = 28;  // Offset of return address.
  static size_t GprIndexToGprOffset(uint32_t gpr_index) {
    return gpr_index * GetBytesPerGprSpillLocation(kRuntimeISA);
  }
#elif defined(__x86_64__)
  // The callee save frame is pointed to by SP.
  // | argN            |  |
  // | ...             |  |
  // | reg. arg spills |  |  Caller's frame
  // | Method*         | ---
  // | Return          |
  // | R15             |    callee save
  // | R14             |    callee save
  // | R13             |    callee save
  // | R12             |    callee save
  // | R9              |    arg5
  // | R8              |    arg4
  // | RSI/R6          |    arg1
  // | RBP/R5          |    callee save
  // | RBX/R3          |    callee save
  // | RDX/R2          |    arg2
  // | RCX/R1          |    arg3
  // | XMM7            |    float arg 8
  // | XMM6            |    float arg 7
  // | XMM5            |    float arg 6
  // | XMM4            |    float arg 5
  // | XMM3            |    float arg 4
  // | XMM2            |    float arg 3
  // | XMM1            |    float arg 2
  // | XMM0            |    float arg 1
  // | Padding         |
  // | RDI/Method*     |  <- sp
  static constexpr bool kQuickSoftFloatAbi = false;  // This is a hard float ABI.
  static constexpr size_t kNumQuickGprArgs = 5;  // 5 arguments passed in GPRs.
  static constexpr size_t kNumQuickFprArgs = 8;  // 8 arguments passed in FPRs.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_Fpr1Offset = 16;  // Offset of first FPR arg.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_Gpr1Offset = 80 + 4*8;  // Offset of first GPR arg.
  static constexpr size_t kQuickCalleeSaveFrame_RefAndArgs_LrOffset = 168 + 4*8;  // Offset of return address.
  static size_t GprIndexToGprOffset(uint32_t gpr_index) {
    switch (gpr_index) {
      case 0: return (4 * GetBytesPerGprSpillLocation(kRuntimeISA));
      case 1: return (1 * GetBytesPerGprSpillLocation(kRuntimeISA));
      case 2: return (0 * GetBytesPerGprSpillLocation(kRuntimeISA));
      case 3: return (5 * GetBytesPerGprSpillLocation(kRuntimeISA));
      case 4: return (6 * GetBytesPerGprSpillLocation(kRuntimeISA));
      default:
      LOG(FATAL) << "Unexpected GPR index: " << gpr_index;
      return 0;
    }
  }
#else
#error "Unsupported architecture"
#endif

 public:
  static mirror::ArtMethod* GetCallingMethod(StackReference<mirror::ArtMethod>* sp)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK(sp->AsMirrorPtr()->IsCalleeSaveMethod());
    byte* previous_sp = reinterpret_cast<byte*>(sp) + kQuickCalleeSaveFrame_RefAndArgs_FrameSize;
    return reinterpret_cast<StackReference<mirror::ArtMethod>*>(previous_sp)->AsMirrorPtr();
  }

  // For the given quick ref and args quick frame, return the caller's PC.
  static uintptr_t GetCallingPc(StackReference<mirror::ArtMethod>* sp)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK(sp->AsMirrorPtr()->IsCalleeSaveMethod());
    byte* lr = reinterpret_cast<byte*>(sp) + kQuickCalleeSaveFrame_RefAndArgs_LrOffset;
    return *reinterpret_cast<uintptr_t*>(lr);
  }

  QuickArgumentVisitor(StackReference<mirror::ArtMethod>* sp, bool is_static, const char* shorty,
                       uint32_t shorty_len) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) :
          is_static_(is_static), shorty_(shorty), shorty_len_(shorty_len),
          gpr_args_(reinterpret_cast<byte*>(sp) + kQuickCalleeSaveFrame_RefAndArgs_Gpr1Offset),
          fpr_args_(reinterpret_cast<byte*>(sp) + kQuickCalleeSaveFrame_RefAndArgs_Fpr1Offset),
          stack_args_(reinterpret_cast<byte*>(sp) + kQuickCalleeSaveFrame_RefAndArgs_FrameSize
                      + StackArgumentStartFromShorty(is_static, shorty, shorty_len)),
          gpr_index_(0), fpr_index_(0), stack_index_(0), cur_type_(Primitive::kPrimVoid),
          is_split_long_or_double_(false) {}

  virtual ~QuickArgumentVisitor() {}

  virtual void Visit() = 0;

  Primitive::Type GetParamPrimitiveType() const {
    return cur_type_;
  }

  byte* GetParamAddress() const {
    if (!kQuickSoftFloatAbi) {
      Primitive::Type type = GetParamPrimitiveType();
      if (UNLIKELY((type == Primitive::kPrimDouble) || (type == Primitive::kPrimFloat))) {
        if ((kNumQuickFprArgs != 0) && (fpr_index_ + 1 < kNumQuickFprArgs + 1)) {
          return fpr_args_ + (fpr_index_ * GetBytesPerFprSpillLocation(kRuntimeISA));
        }
        return stack_args_ + (stack_index_ * kBytesStackArgLocation);
      }
    }
    if (gpr_index_ < kNumQuickGprArgs) {
      return gpr_args_ + GprIndexToGprOffset(gpr_index_);
    }
    return stack_args_ + (stack_index_ * kBytesStackArgLocation);
  }

  bool IsSplitLongOrDouble() const {
    if ((GetBytesPerGprSpillLocation(kRuntimeISA) == 4) || (GetBytesPerFprSpillLocation(kRuntimeISA) == 4)) {
      return is_split_long_or_double_;
    } else {
      return false;  // An optimization for when GPR and FPRs are 64bit.
    }
  }

  bool IsParamAReference() const {
    return GetParamPrimitiveType() == Primitive::kPrimNot;
  }

  bool IsParamALongOrDouble() const {
    Primitive::Type type = GetParamPrimitiveType();
    return type == Primitive::kPrimLong || type == Primitive::kPrimDouble;
  }

  uint64_t ReadSplitLongParam() const {
    DCHECK(IsSplitLongOrDouble());
    uint64_t low_half = *reinterpret_cast<uint32_t*>(GetParamAddress());
    uint64_t high_half = *reinterpret_cast<uint32_t*>(stack_args_);
    return (low_half & 0xffffffffULL) | (high_half << 32);
  }

  void VisitArguments() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    // This implementation doesn't support reg-spill area for hard float
    // ABI targets such as x86_64 and aarch64. So, for those targets whose
    // 'kQuickSoftFloatAbi' is 'false':
    //     (a) 'stack_args_' should point to the first method's argument
    //     (b) whatever the argument type it is, the 'stack_index_' should
    //         be moved forward along with every visiting.
    gpr_index_ = 0;
    fpr_index_ = 0;
    stack_index_ = 0;
    if (!is_static_) {  // Handle this.
      cur_type_ = Primitive::kPrimNot;
      is_split_long_or_double_ = false;
      Visit();
      if (!kQuickSoftFloatAbi || kNumQuickGprArgs == 0) {
        stack_index_++;
      }
      if (kNumQuickGprArgs > 0) {
        gpr_index_++;
      }
    }
    for (uint32_t shorty_index = 1; shorty_index < shorty_len_; ++shorty_index) {
      cur_type_ = Primitive::GetType(shorty_[shorty_index]);
      switch (cur_type_) {
        case Primitive::kPrimNot:
        case Primitive::kPrimBoolean:
        case Primitive::kPrimByte:
        case Primitive::kPrimChar:
        case Primitive::kPrimShort:
        case Primitive::kPrimInt:
          is_split_long_or_double_ = false;
          Visit();
          if (!kQuickSoftFloatAbi || kNumQuickGprArgs == gpr_index_) {
            stack_index_++;
          }
          if (gpr_index_ < kNumQuickGprArgs) {
            gpr_index_++;
          }
          break;
        case Primitive::kPrimFloat:
          is_split_long_or_double_ = false;
          Visit();
          if (kQuickSoftFloatAbi) {
            if (gpr_index_ < kNumQuickGprArgs) {
              gpr_index_++;
            } else {
              stack_index_++;
            }
          } else {
            if ((kNumQuickFprArgs != 0) && (fpr_index_ + 1 < kNumQuickFprArgs + 1)) {
              fpr_index_++;
            }
            stack_index_++;
          }
          break;
        case Primitive::kPrimDouble:
        case Primitive::kPrimLong:
          if (kQuickSoftFloatAbi || (cur_type_ == Primitive::kPrimLong)) {
            is_split_long_or_double_ = (GetBytesPerGprSpillLocation(kRuntimeISA) == 4) &&
                ((gpr_index_ + 1) == kNumQuickGprArgs);
            Visit();
            if (!kQuickSoftFloatAbi || kNumQuickGprArgs == gpr_index_) {
              if (kBytesStackArgLocation == 4) {
                stack_index_+= 2;
              } else {
                CHECK_EQ(kBytesStackArgLocation, 8U);
                stack_index_++;
              }
            }
            if (gpr_index_ < kNumQuickGprArgs) {
              gpr_index_++;
              if (GetBytesPerGprSpillLocation(kRuntimeISA) == 4) {
                if (gpr_index_ < kNumQuickGprArgs) {
                  gpr_index_++;
                } else if (kQuickSoftFloatAbi) {
                  stack_index_++;
                }
              }
            }
          } else {
            is_split_long_or_double_ = (GetBytesPerFprSpillLocation(kRuntimeISA) == 4) &&
                ((fpr_index_ + 1) == kNumQuickFprArgs);
            Visit();
            if ((kNumQuickFprArgs != 0) && (fpr_index_ + 1 < kNumQuickFprArgs + 1)) {
              fpr_index_++;
              if (GetBytesPerFprSpillLocation(kRuntimeISA) == 4) {
                if ((kNumQuickFprArgs != 0) && (fpr_index_ + 1 < kNumQuickFprArgs + 1)) {
                  fpr_index_++;
                }
              }
            }
            if (kBytesStackArgLocation == 4) {
              stack_index_+= 2;
            } else {
              CHECK_EQ(kBytesStackArgLocation, 8U);
              stack_index_++;
            }
          }
          break;
        default:
          LOG(FATAL) << "Unexpected type: " << cur_type_ << " in " << shorty_;
      }
    }
  }

 private:
  static size_t StackArgumentStartFromShorty(bool is_static, const char* shorty,
                                             uint32_t shorty_len) {
    if (kQuickSoftFloatAbi) {
      CHECK_EQ(kNumQuickFprArgs, 0U);
      return (kNumQuickGprArgs * GetBytesPerGprSpillLocation(kRuntimeISA))
          + sizeof(StackReference<mirror::ArtMethod>) /* StackReference<ArtMethod> */;
    } else {
      // For now, there is no reg-spill area for the targets with
      // hard float ABI. So, the offset pointing to the first method's
      // parameter ('this' for non-static methods) should be returned.
      return sizeof(StackReference<mirror::ArtMethod>);  // Skip StackReference<ArtMethod>.
    }
  }

 protected:
  const bool is_static_;
  const char* const shorty_;
  const uint32_t shorty_len_;

 private:
  byte* const gpr_args_;  // Address of GPR arguments in callee save frame.
  byte* const fpr_args_;  // Address of FPR arguments in callee save frame.
  byte* const stack_args_;  // Address of stack arguments in caller's frame.
  uint32_t gpr_index_;  // Index into spilled GPRs.
  uint32_t fpr_index_;  // Index into spilled FPRs.
  uint32_t stack_index_;  // Index into arguments on the stack.
  // The current type of argument during VisitArguments.
  Primitive::Type cur_type_;
  // Does a 64bit parameter straddle the register and stack arguments?
  bool is_split_long_or_double_;
};

// Visits arguments on the stack placing them into the shadow frame.
class BuildQuickShadowFrameVisitor FINAL : public QuickArgumentVisitor {
 public:
  BuildQuickShadowFrameVisitor(StackReference<mirror::ArtMethod>* sp, bool is_static,
                               const char* shorty, uint32_t shorty_len, ShadowFrame* sf,
                               size_t first_arg_reg) :
      QuickArgumentVisitor(sp, is_static, shorty, shorty_len), sf_(sf), cur_reg_(first_arg_reg) {}

  void Visit() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) OVERRIDE;

 private:
  ShadowFrame* const sf_;
  uint32_t cur_reg_;

  DISALLOW_COPY_AND_ASSIGN(BuildQuickShadowFrameVisitor);
};

void BuildQuickShadowFrameVisitor::Visit() {
  Primitive::Type type = GetParamPrimitiveType();
  switch (type) {
    case Primitive::kPrimLong:  // Fall-through.
    case Primitive::kPrimDouble:
      if (IsSplitLongOrDouble()) {
        sf_->SetVRegLong(cur_reg_, ReadSplitLongParam());
      } else {
        sf_->SetVRegLong(cur_reg_, *reinterpret_cast<jlong*>(GetParamAddress()));
      }
      ++cur_reg_;
      break;
    case Primitive::kPrimNot: {
        StackReference<mirror::Object>* stack_ref =
            reinterpret_cast<StackReference<mirror::Object>*>(GetParamAddress());
        sf_->SetVRegReference(cur_reg_, stack_ref->AsMirrorPtr());
      }
      break;
    case Primitive::kPrimBoolean:  // Fall-through.
    case Primitive::kPrimByte:     // Fall-through.
    case Primitive::kPrimChar:     // Fall-through.
    case Primitive::kPrimShort:    // Fall-through.
    case Primitive::kPrimInt:      // Fall-through.
    case Primitive::kPrimFloat:
      sf_->SetVReg(cur_reg_, *reinterpret_cast<jint*>(GetParamAddress()));
      break;
    case Primitive::kPrimVoid:
      LOG(FATAL) << "UNREACHABLE";
      break;
  }
  ++cur_reg_;
}

// Visits arguments on the stack placing them into the args vector, Object* arguments are converted
// to jobjects.
class BuildQuickArgumentVisitor : public QuickArgumentVisitor {
 public:
  BuildQuickArgumentVisitor(StackReference<mirror::ArtMethod>* sp, bool is_static,
                            const char* shorty, uint32_t shorty_len,
                            ScopedObjectAccessUnchecked* soa, std::vector<jvalue>* args) :
      QuickArgumentVisitor(sp, is_static, shorty, shorty_len), soa_(soa), args_(args) {}

  void Visit() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) OVERRIDE;

  void FixupReferences() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

 private:
  ScopedObjectAccessUnchecked* const soa_;
  std::vector<jvalue>* const args_;
  // References which we must update when exiting in case the GC moved the objects.
  std::vector<std::pair<jobject, StackReference<mirror::Object>*>> references_;

  DISALLOW_COPY_AND_ASSIGN(BuildQuickArgumentVisitor);
};

void BuildQuickArgumentVisitor::Visit() {
  jvalue val;
  Primitive::Type type = GetParamPrimitiveType();
  switch (type) {
    case Primitive::kPrimNot: {
      StackReference<mirror::Object>* stack_ref =
          reinterpret_cast<StackReference<mirror::Object>*>(GetParamAddress());
      val.l = soa_->AddLocalReference<jobject>(stack_ref->AsMirrorPtr());
      references_.push_back(std::make_pair(val.l, stack_ref));
      break;
    }
    case Primitive::kPrimLong:  // Fall-through.
    case Primitive::kPrimDouble:
      if (IsSplitLongOrDouble()) {
        val.j = ReadSplitLongParam();
      } else {
        val.j = *reinterpret_cast<jlong*>(GetParamAddress());
      }
      break;
    case Primitive::kPrimBoolean:  // Fall-through.
    case Primitive::kPrimByte:     // Fall-through.
    case Primitive::kPrimChar:     // Fall-through.
    case Primitive::kPrimShort:    // Fall-through.
    case Primitive::kPrimInt:      // Fall-through.
    case Primitive::kPrimFloat:
      val.i = *reinterpret_cast<jint*>(GetParamAddress());
      break;
    case Primitive::kPrimVoid:
      LOG(FATAL) << "UNREACHABLE";
      val.j = 0;
      break;
  }
  args_->push_back(val);
}

void BuildQuickArgumentVisitor::FixupReferences() {
  // Fixup any references which may have changed.
  for (const auto& pair : references_) {
    pair.second->Assign(soa_->Decode<mirror::Object*>(pair.first));
    soa_->Env()->DeleteLocalRef(pair.first);
  }
}



}  // namespace art
