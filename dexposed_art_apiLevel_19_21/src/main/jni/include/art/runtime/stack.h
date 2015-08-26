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

#ifndef ART_RUNTIME_STACK_H_
#define ART_RUNTIME_STACK_H_

#include <stdint.h>
#include <string>

#include "dex_file.h"
#include "instruction_set.h"
#include "mirror/object_reference.h"
#include "throw_location.h"
#include "utils.h"
#include "verify_object.h"

namespace art {

namespace mirror {
  class ArtMethod;
  class Object;
}  // namespace mirror

class Context;
class ShadowFrame;
class HandleScope;
class ScopedObjectAccess;
class Thread;

// The kind of vreg being accessed in calls to Set/GetVReg.
enum VRegKind {
  kReferenceVReg,
  kIntVReg,
  kFloatVReg,
  kLongLoVReg,
  kLongHiVReg,
  kDoubleLoVReg,
  kDoubleHiVReg,
  kConstant,
  kImpreciseConstant,
  kUndefined,
};

/**
 * @brief Represents the virtual register numbers that denote special meaning.
 * @details This is used to make some virtual register numbers to have specific
 * semantic meaning. This is done so that the compiler can treat all virtual
 * registers the same way and only special case when needed. For example,
 * calculating SSA does not care whether a virtual register is a normal one or
 * a compiler temporary, so it can deal with them in a consistent manner. But,
 * for example if backend cares about temporaries because it has custom spill
 * location, then it can special case them only then.
 */
enum VRegBaseRegNum : int {
  /**
   * @brief Virtual registers originating from dex have number >= 0.
   */
  kVRegBaseReg = 0,

  /**
   * @brief Invalid virtual register number.
   */
  kVRegInvalid = -1,

  /**
   * @brief Used to denote the base register for compiler temporaries.
   * @details Compiler temporaries are virtual registers not originating
   * from dex but that are created by compiler.  All virtual register numbers
   * that are <= kVRegTempBaseReg are categorized as compiler temporaries.
   */
  kVRegTempBaseReg = -2,

  /**
   * @brief Base register of temporary that holds the method pointer.
   * @details This is a special compiler temporary because it has a specific
   * location on stack.
   */
  kVRegMethodPtrBaseReg = kVRegTempBaseReg,

  /**
   * @brief Base register of non-special compiler temporary.
   * @details A non-special compiler temporary is one whose spill location
   * is flexible.
   */
  kVRegNonSpecialTempBaseReg = -3,
};

// A reference from the shadow stack to a MirrorType object within the Java heap.
template<class MirrorType>
class MANAGED StackReference : public mirror::ObjectReference<false, MirrorType> {
 public:
  StackReference<MirrorType>() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      : mirror::ObjectReference<false, MirrorType>(nullptr) {}

  static StackReference<MirrorType> FromMirrorPtr(MirrorType* p)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return StackReference<MirrorType>(p);
  }

 private:
  StackReference<MirrorType>(MirrorType* p) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      : mirror::ObjectReference<false, MirrorType>(p) {}
};

// ShadowFrame has 3 possible layouts:
//  - portable - a unified array of VRegs and references. Precise references need GC maps.
//  - interpreter - separate VRegs and reference arrays. References are in the reference array.
//  - JNI - just VRegs, but where every VReg holds a reference.
class ShadowFrame {
 public:
  // Compute size of ShadowFrame in bytes assuming it has a reference array.
  static size_t ComputeSize(uint32_t num_vregs) {
    return sizeof(ShadowFrame) + (sizeof(uint32_t) * num_vregs) +
           (sizeof(StackReference<mirror::Object>) * num_vregs);
  }

  // Create ShadowFrame in heap for deoptimization.
  static ShadowFrame* Create(uint32_t num_vregs, ShadowFrame* link,
                             mirror::ArtMethod* method, uint32_t dex_pc) {
    uint8_t* memory = new uint8_t[ComputeSize(num_vregs)];
    return Create(num_vregs, link, method, dex_pc, memory);
  }

  // Create ShadowFrame for interpreter using provided memory.
  static ShadowFrame* Create(uint32_t num_vregs, ShadowFrame* link,
                             mirror::ArtMethod* method, uint32_t dex_pc, void* memory) {
    ShadowFrame* sf = new (memory) ShadowFrame(num_vregs, link, method, dex_pc, true);
    return sf;
  }
  ~ShadowFrame() {}

  bool HasReferenceArray() const {
#if defined(ART_USE_PORTABLE_COMPILER)
    return (number_of_vregs_ & kHasReferenceArray) != 0;
#else
    return true;
#endif
  }

  uint32_t NumberOfVRegs() const {
#if defined(ART_USE_PORTABLE_COMPILER)
    return number_of_vregs_ & ~kHasReferenceArray;
#else
    return number_of_vregs_;
#endif
  }

  void SetNumberOfVRegs(uint32_t number_of_vregs) {
#if defined(ART_USE_PORTABLE_COMPILER)
    number_of_vregs_ = number_of_vregs | (number_of_vregs_ & kHasReferenceArray);
#else
    UNUSED(number_of_vregs);
    UNIMPLEMENTED(FATAL) << "Should only be called when portable is enabled";
#endif
  }

  uint32_t GetDexPC() const {
    return dex_pc_;
  }

  void SetDexPC(uint32_t dex_pc) {
    dex_pc_ = dex_pc;
  }

  ShadowFrame* GetLink() const {
    return link_;
  }

  void SetLink(ShadowFrame* frame) {
    DCHECK_NE(this, frame);
    link_ = frame;
  }

  int32_t GetVReg(size_t i) const {
    DCHECK_LT(i, NumberOfVRegs());
    const uint32_t* vreg = &vregs_[i];
    return *reinterpret_cast<const int32_t*>(vreg);
  }

  float GetVRegFloat(size_t i) const {
    DCHECK_LT(i, NumberOfVRegs());
    // NOTE: Strict-aliasing?
    const uint32_t* vreg = &vregs_[i];
    return *reinterpret_cast<const float*>(vreg);
  }

  int64_t GetVRegLong(size_t i) const {
    DCHECK_LT(i, NumberOfVRegs());
    const uint32_t* vreg = &vregs_[i];
    // Alignment attribute required for GCC 4.8
    typedef const int64_t unaligned_int64 __attribute__ ((aligned (4)));
    return *reinterpret_cast<unaligned_int64*>(vreg);
  }

  double GetVRegDouble(size_t i) const {
    DCHECK_LT(i, NumberOfVRegs());
    const uint32_t* vreg = &vregs_[i];
    // Alignment attribute required for GCC 4.8
    typedef const double unaligned_double __attribute__ ((aligned (4)));
    return *reinterpret_cast<unaligned_double*>(vreg);
  }

  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  mirror::Object* GetVRegReference(size_t i) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK_LT(i, NumberOfVRegs());
    mirror::Object* ref;
    if (HasReferenceArray()) {
      ref = References()[i].AsMirrorPtr();
    } else {
      const uint32_t* vreg_ptr = &vregs_[i];
      ref = reinterpret_cast<const StackReference<mirror::Object>*>(vreg_ptr)->AsMirrorPtr();
    }
    if (kVerifyFlags & kVerifyReads) {
      VerifyObject(ref);
    }
    return ref;
  }

  // Get view of vregs as range of consecutive arguments starting at i.
  uint32_t* GetVRegArgs(size_t i) {
    return &vregs_[i];
  }

  void SetVReg(size_t i, int32_t val) {
    DCHECK_LT(i, NumberOfVRegs());
    uint32_t* vreg = &vregs_[i];
    *reinterpret_cast<int32_t*>(vreg) = val;
    // This is needed for moving collectors since these can update the vreg references if they
    // happen to agree with references in the reference array.
    if (kMovingCollector && HasReferenceArray()) {
      References()[i].Clear();
    }
  }

  void SetVRegFloat(size_t i, float val) {
    DCHECK_LT(i, NumberOfVRegs());
    uint32_t* vreg = &vregs_[i];
    *reinterpret_cast<float*>(vreg) = val;
    // This is needed for moving collectors since these can update the vreg references if they
    // happen to agree with references in the reference array.
    if (kMovingCollector && HasReferenceArray()) {
      References()[i].Clear();
    }
  }

  void SetVRegLong(size_t i, int64_t val) {
    DCHECK_LT(i, NumberOfVRegs());
    uint32_t* vreg = &vregs_[i];
    // Alignment attribute required for GCC 4.8
    typedef int64_t unaligned_int64 __attribute__ ((aligned (4)));
    *reinterpret_cast<unaligned_int64*>(vreg) = val;
    // This is needed for moving collectors since these can update the vreg references if they
    // happen to agree with references in the reference array.
    if (kMovingCollector && HasReferenceArray()) {
      References()[i].Clear();
      References()[i + 1].Clear();
    }
  }

  void SetVRegDouble(size_t i, double val) {
    DCHECK_LT(i, NumberOfVRegs());
    uint32_t* vreg = &vregs_[i];
    // Alignment attribute required for GCC 4.8
    typedef double unaligned_double __attribute__ ((aligned (4)));
    *reinterpret_cast<unaligned_double*>(vreg) = val;
    // This is needed for moving collectors since these can update the vreg references if they
    // happen to agree with references in the reference array.
    if (kMovingCollector && HasReferenceArray()) {
      References()[i].Clear();
      References()[i + 1].Clear();
    }
  }

  template<VerifyObjectFlags kVerifyFlags = kDefaultVerifyFlags>
  void SetVRegReference(size_t i, mirror::Object* val) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK_LT(i, NumberOfVRegs());
    if (kVerifyFlags & kVerifyWrites) {
      VerifyObject(val);
    }
    uint32_t* vreg = &vregs_[i];
    reinterpret_cast<StackReference<mirror::Object>*>(vreg)->Assign(val);
    if (HasReferenceArray()) {
      References()[i].Assign(val);
    }
  }

  mirror::ArtMethod* GetMethod() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK(method_ != nullptr);
    return method_;
  }

  mirror::ArtMethod** GetMethodAddress() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    DCHECK(method_ != nullptr);
    return &method_;
  }

  mirror::Object* GetThisObject() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::Object* GetThisObject(uint16_t num_ins) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  ThrowLocation GetCurrentLocationForThrow() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void SetMethod(mirror::ArtMethod* method) {
#if defined(ART_USE_PORTABLE_COMPILER)
    DCHECK(method != nullptr);
    method_ = method;
#else
    UNUSED(method);
    UNIMPLEMENTED(FATAL) << "Should only be called when portable is enabled";
#endif
  }

  bool Contains(StackReference<mirror::Object>* shadow_frame_entry_obj) const {
    if (HasReferenceArray()) {
      return ((&References()[0] <= shadow_frame_entry_obj) &&
              (shadow_frame_entry_obj <= (&References()[NumberOfVRegs() - 1])));
    } else {
      uint32_t* shadow_frame_entry = reinterpret_cast<uint32_t*>(shadow_frame_entry_obj);
      return ((&vregs_[0] <= shadow_frame_entry) &&
              (shadow_frame_entry <= (&vregs_[NumberOfVRegs() - 1])));
    }
  }

  static size_t LinkOffset() {
    return OFFSETOF_MEMBER(ShadowFrame, link_);
  }

  static size_t MethodOffset() {
    return OFFSETOF_MEMBER(ShadowFrame, method_);
  }

  static size_t DexPCOffset() {
    return OFFSETOF_MEMBER(ShadowFrame, dex_pc_);
  }

  static size_t NumberOfVRegsOffset() {
    return OFFSETOF_MEMBER(ShadowFrame, number_of_vregs_);
  }

  static size_t VRegsOffset() {
    return OFFSETOF_MEMBER(ShadowFrame, vregs_);
  }

 private:
  ShadowFrame(uint32_t num_vregs, ShadowFrame* link, mirror::ArtMethod* method,
              uint32_t dex_pc, bool has_reference_array)
      : number_of_vregs_(num_vregs), link_(link), method_(method), dex_pc_(dex_pc) {
    if (has_reference_array) {
#if defined(ART_USE_PORTABLE_COMPILER)
      CHECK_LT(num_vregs, static_cast<uint32_t>(kHasReferenceArray));
      number_of_vregs_ |= kHasReferenceArray;
#endif
      memset(vregs_, 0, num_vregs * (sizeof(uint32_t) + sizeof(StackReference<mirror::Object>)));
    } else {
      memset(vregs_, 0, num_vregs * sizeof(uint32_t));
    }
  }

  const StackReference<mirror::Object>* References() const {
    DCHECK(HasReferenceArray());
    const uint32_t* vreg_end = &vregs_[NumberOfVRegs()];
    return reinterpret_cast<const StackReference<mirror::Object>*>(vreg_end);
  }

  StackReference<mirror::Object>* References() {
    return const_cast<StackReference<mirror::Object>*>(const_cast<const ShadowFrame*>(this)->References());
  }

#if defined(ART_USE_PORTABLE_COMPILER)
  enum ShadowFrameFlag {
    kHasReferenceArray = 1ul << 31
  };
  // TODO: make const in the portable case.
  uint32_t number_of_vregs_;
#else
  const uint32_t number_of_vregs_;
#endif
  // Link to previous shadow frame or NULL.
  ShadowFrame* link_;
  mirror::ArtMethod* method_;
  uint32_t dex_pc_;
  uint32_t vregs_[0];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ShadowFrame);
};

// The managed stack is used to record fragments of managed code stacks. Managed code stacks
// may either be shadow frames or lists of frames using fixed frame sizes. Transition records are
// necessary for transitions between code using different frame layouts and transitions into native
// code.
class PACKED(4) ManagedStack {
 public:
  ManagedStack()
      : link_(NULL), top_shadow_frame_(NULL), top_quick_frame_(NULL), top_quick_frame_pc_(0) {}

  void PushManagedStackFragment(ManagedStack* fragment) {
    // Copy this top fragment into given fragment.
    memcpy(fragment, this, sizeof(ManagedStack));
    // Clear this fragment, which has become the top.
    memset(this, 0, sizeof(ManagedStack));
    // Link our top fragment onto the given fragment.
    link_ = fragment;
  }

  void PopManagedStackFragment(const ManagedStack& fragment) {
    DCHECK(&fragment == link_);
    // Copy this given fragment back to the top.
    memcpy(this, &fragment, sizeof(ManagedStack));
  }

  ManagedStack* GetLink() const {
    return link_;
  }

  StackReference<mirror::ArtMethod>* GetTopQuickFrame() const {
    return top_quick_frame_;
  }

  void SetTopQuickFrame(StackReference<mirror::ArtMethod>* top) {
    DCHECK(top_shadow_frame_ == NULL);
    top_quick_frame_ = top;
  }

  uintptr_t GetTopQuickFramePc() const {
    return top_quick_frame_pc_;
  }

  void SetTopQuickFramePc(uintptr_t pc) {
    DCHECK(top_shadow_frame_ == NULL);
    top_quick_frame_pc_ = pc;
  }

  static size_t TopQuickFrameOffset() {
    return OFFSETOF_MEMBER(ManagedStack, top_quick_frame_);
  }

  static size_t TopQuickFramePcOffset() {
    return OFFSETOF_MEMBER(ManagedStack, top_quick_frame_pc_);
  }

  ShadowFrame* PushShadowFrame(ShadowFrame* new_top_frame) {
    DCHECK(top_quick_frame_ == NULL);
    ShadowFrame* old_frame = top_shadow_frame_;
    top_shadow_frame_ = new_top_frame;
    new_top_frame->SetLink(old_frame);
    return old_frame;
  }

  ShadowFrame* PopShadowFrame() {
    DCHECK(top_quick_frame_ == NULL);
    CHECK(top_shadow_frame_ != NULL);
    ShadowFrame* frame = top_shadow_frame_;
    top_shadow_frame_ = frame->GetLink();
    return frame;
  }

  ShadowFrame* GetTopShadowFrame() const {
    return top_shadow_frame_;
  }

  void SetTopShadowFrame(ShadowFrame* top) {
    DCHECK(top_quick_frame_ == NULL);
    top_shadow_frame_ = top;
  }

  static size_t TopShadowFrameOffset() {
    return OFFSETOF_MEMBER(ManagedStack, top_shadow_frame_);
  }

  size_t NumJniShadowFrameReferences() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool ShadowFramesContain(StackReference<mirror::Object>* shadow_frame_entry) const;

 private:
  ManagedStack* link_;
  ShadowFrame* top_shadow_frame_;
  StackReference<mirror::ArtMethod>* top_quick_frame_;
  uintptr_t top_quick_frame_pc_;
};

class StackVisitor {
 protected:
  StackVisitor(Thread* thread, Context* context) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

 public:
  virtual ~StackVisitor() {}

  // Return 'true' if we should continue to visit more frames, 'false' to stop.
  virtual bool VisitFrame() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) = 0;

  void WalkStack(bool include_transitions = false)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::ArtMethod* GetMethod() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    if (cur_shadow_frame_ != nullptr) {
      return cur_shadow_frame_->GetMethod();
    } else if (cur_quick_frame_ != nullptr) {
      return cur_quick_frame_->AsMirrorPtr();
    } else {
      return nullptr;
    }
  }

  bool IsShadowFrame() const {
    return cur_shadow_frame_ != nullptr;
  }

  uint32_t GetDexPc(bool abort_on_failure = true) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::Object* GetThisObject() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  size_t GetNativePcOffset() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  uintptr_t* CalleeSaveAddress(int num, size_t frame_size) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    // Callee saves are held at the top of the frame
    DCHECK(GetMethod() != nullptr);
    byte* save_addr =
        reinterpret_cast<byte*>(cur_quick_frame_) + frame_size - ((num + 1) * kPointerSize);
#if defined(__i386__) || defined(__x86_64__)
    save_addr -= kPointerSize;  // account for return address
#endif
    return reinterpret_cast<uintptr_t*>(save_addr);
  }

  // Returns the height of the stack in the managed stack frames, including transitions.
  size_t GetFrameHeight() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return GetNumFrames() - cur_depth_ - 1;
  }

  // Returns a frame ID for JDWP use, starting from 1.
  size_t GetFrameId() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return GetFrameHeight() + 1;
  }

  size_t GetNumFrames() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    if (num_frames_ == 0) {
      num_frames_ = ComputeNumFrames(thread_);
    }
    return num_frames_;
  }

  size_t GetFrameDepth() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return cur_depth_;
  }

  // Get the method and dex pc immediately after the one that's currently being visited.
  bool GetNextMethodAndDexPc(mirror::ArtMethod** next_method, uint32_t* next_dex_pc)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool GetVReg(mirror::ArtMethod* m, uint16_t vreg, VRegKind kind, uint32_t* val) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  uint32_t GetVReg(mirror::ArtMethod* m, uint16_t vreg, VRegKind kind) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    uint32_t val;
    bool success = GetVReg(m, vreg, kind, &val);
    CHECK(success) << "Failed to read vreg " << vreg << " of kind " << kind;
    return val;
  }

  bool GetVRegPair(mirror::ArtMethod* m, uint16_t vreg, VRegKind kind_lo, VRegKind kind_hi,
                   uint64_t* val) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  uint64_t GetVRegPair(mirror::ArtMethod* m, uint16_t vreg, VRegKind kind_lo,
                       VRegKind kind_hi) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    uint64_t val;
    bool success = GetVRegPair(m, vreg, kind_lo, kind_hi, &val);
    CHECK(success) << "Failed to read vreg pair " << vreg
                   << " of kind [" << kind_lo << "," << kind_hi << "]";
    return val;
  }

  bool SetVReg(mirror::ArtMethod* m, uint16_t vreg, uint32_t new_value, VRegKind kind)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool SetVRegPair(mirror::ArtMethod* m, uint16_t vreg, uint64_t new_value,
                   VRegKind kind_lo, VRegKind kind_hi)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  uintptr_t* GetGPRAddress(uint32_t reg) const;

  // This is a fast-path for getting/setting values in a quick frame.
  uint32_t* GetVRegAddr(StackReference<mirror::ArtMethod>* cur_quick_frame,
                        const DexFile::CodeItem* code_item,
                        uint32_t core_spills, uint32_t fp_spills, size_t frame_size,
                        uint16_t vreg) const {
    int offset = GetVRegOffset(code_item, core_spills, fp_spills, frame_size, vreg, kRuntimeISA);
    DCHECK_EQ(cur_quick_frame, GetCurrentQuickFrame());
    byte* vreg_addr = reinterpret_cast<byte*>(cur_quick_frame) + offset;
    return reinterpret_cast<uint32_t*>(vreg_addr);
  }

  uintptr_t GetReturnPc() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void SetReturnPc(uintptr_t new_ret_pc) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  /*
   * Return sp-relative offset for a Dalvik virtual register, compiler
   * spill or Method* in bytes using Method*.
   * Note that (reg >= 0) refers to a Dalvik register, (reg == -1)
   * denotes an invalid Dalvik register, (reg == -2) denotes Method*
   * and (reg <= -3) denotes a compiler temporary. A compiler temporary
   * can be thought of as a virtual register that does not exist in the
   * dex but holds intermediate values to help optimizations and code
   * generation. A special compiler temporary is one whose location
   * in frame is well known while non-special ones do not have a requirement
   * on location in frame as long as code generator itself knows how
   * to access them.
   *
   *     +---------------------------+
   *     | IN[ins-1]                 |  {Note: resides in caller's frame}
   *     |       .                   |
   *     | IN[0]                     |
   *     | caller's ArtMethod        |  ... StackReference<ArtMethod>
   *     +===========================+  {Note: start of callee's frame}
   *     | core callee-save spill    |  {variable sized}
   *     +---------------------------+
   *     | fp callee-save spill      |
   *     +---------------------------+
   *     | filler word               |  {For compatibility, if V[locals-1] used as wide
   *     +---------------------------+
   *     | V[locals-1]               |
   *     | V[locals-2]               |
   *     |      .                    |
   *     |      .                    |  ... (reg == 2)
   *     | V[1]                      |  ... (reg == 1)
   *     | V[0]                      |  ... (reg == 0) <---- "locals_start"
   *     +---------------------------+
   *     | Compiler temp region      |  ... (reg <= -3)
   *     |                           |
   *     |                           |
   *     +---------------------------+
   *     | stack alignment padding   |  {0 to (kStackAlignWords-1) of padding}
   *     +---------------------------+
   *     | OUT[outs-1]               |
   *     | OUT[outs-2]               |
   *     |       .                   |
   *     | OUT[0]                    |
   *     | StackReference<ArtMethod> |  ... (reg == -2) <<== sp, 16-byte aligned
   *     +===========================+
   */
  static int GetVRegOffset(const DexFile::CodeItem* code_item,
                           uint32_t core_spills, uint32_t fp_spills,
                           size_t frame_size, int reg, InstructionSet isa) {
    DCHECK_EQ(frame_size & (kStackAlignment - 1), 0U);
    DCHECK_NE(reg, static_cast<int>(kVRegInvalid));
    int spill_size = POPCOUNT(core_spills) * GetBytesPerGprSpillLocation(isa)
        + POPCOUNT(fp_spills) * GetBytesPerFprSpillLocation(isa)
        + sizeof(uint32_t);  // Filler.
    int num_ins = code_item->ins_size_;
    int num_regs = code_item->registers_size_ - num_ins;
    int locals_start = frame_size - spill_size - num_regs * sizeof(uint32_t);
    if (reg == static_cast<int>(kVRegMethodPtrBaseReg)) {
      // The current method pointer corresponds to special location on stack.
      return 0;
    } else if (reg <= static_cast<int>(kVRegNonSpecialTempBaseReg)) {
      /*
       * Special temporaries may have custom locations and the logic above deals with that.
       * However, non-special temporaries are placed relative to the locals. Since the
       * virtual register numbers for temporaries "grow" in negative direction, reg number
       * will always be <= to the temp base reg. Thus, the logic ensures that the first
       * temp is at offset -4 bytes from locals, the second is at -8 bytes from locals,
       * and so on.
       */
      int relative_offset =
          (reg + std::abs(static_cast<int>(kVRegNonSpecialTempBaseReg)) - 1) * sizeof(uint32_t);
      return locals_start + relative_offset;
    }  else if (reg < num_regs) {
      return locals_start + (reg * sizeof(uint32_t));
    } else {
      // Handle ins.
      return frame_size + ((reg - num_regs) * sizeof(uint32_t)) +
          sizeof(StackReference<mirror::ArtMethod>);
    }
  }

  static int GetOutVROffset(uint16_t out_num, InstructionSet isa) {
    // According to stack model, the first out is above the Method referernce.
    return sizeof(StackReference<mirror::ArtMethod>) + (out_num * sizeof(uint32_t));
  }

  uintptr_t GetCurrentQuickFramePc() const {
    return cur_quick_frame_pc_;
  }

  StackReference<mirror::ArtMethod>* GetCurrentQuickFrame() const {
    return cur_quick_frame_;
  }

  ShadowFrame* GetCurrentShadowFrame() const {
    return cur_shadow_frame_;
  }

  HandleScope* GetCurrentHandleScope() const {
    StackReference<mirror::ArtMethod>* sp = GetCurrentQuickFrame();
    ++sp;  // Skip Method*; handle scope comes next;
    return reinterpret_cast<HandleScope*>(sp);
  }

  std::string DescribeLocation() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static size_t ComputeNumFrames(Thread* thread) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static void DescribeStack(Thread* thread) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

 private:
  // Private constructor known in the case that num_frames_ has already been computed.
  StackVisitor(Thread* thread, Context* context, size_t num_frames)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool GetGPR(uint32_t reg, uintptr_t* val) const;
  bool SetGPR(uint32_t reg, uintptr_t value);
  bool GetFPR(uint32_t reg, uintptr_t* val) const;
  bool SetFPR(uint32_t reg, uintptr_t value);

  void SanityCheckFrame() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  Thread* const thread_;
  ShadowFrame* cur_shadow_frame_;
  StackReference<mirror::ArtMethod>* cur_quick_frame_;
  uintptr_t cur_quick_frame_pc_;
  // Lazily computed, number of frames in the stack.
  size_t num_frames_;
  // Depth of the frame we're currently at.
  size_t cur_depth_;

 protected:
  Context* const context_;
};

}  // namespace art

#endif  // ART_RUNTIME_STACK_H_
