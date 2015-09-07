/*
 * Copyright (C) 2008 The Android Open Source Project
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

/*
 * Dalvik instruction utility functions.
 */
#ifndef LIBDEX_INSTRUTILS_H_
#define LIBDEX_INSTRUTILS_H_

#include "DexFile.h"
#include "DexOpcodes.h"

/*
 * Possible instruction formats associated with Dalvik opcodes.
 *
 * See the file opcode-gen/README.txt for information about updating
 * opcodes and instruction formats.
 */
enum InstructionFormat {
    kFmt00x = 0,    // unknown format (also used for "breakpoint" opcode)
    kFmt10x,        // op
    kFmt12x,        // op vA, vB
    kFmt11n,        // op vA, #+B
    kFmt11x,        // op vAA
    kFmt10t,        // op +AA
    kFmt20bc,       // [opt] op AA, thing@BBBB
    kFmt20t,        // op +AAAA
    kFmt22x,        // op vAA, vBBBB
    kFmt21t,        // op vAA, +BBBB
    kFmt21s,        // op vAA, #+BBBB
    kFmt21h,        // op vAA, #+BBBB00000[00000000]
    kFmt21c,        // op vAA, thing@BBBB
    kFmt23x,        // op vAA, vBB, vCC
    kFmt22b,        // op vAA, vBB, #+CC
    kFmt22t,        // op vA, vB, +CCCC
    kFmt22s,        // op vA, vB, #+CCCC
    kFmt22c,        // op vA, vB, thing@CCCC
    kFmt22cs,       // [opt] op vA, vB, field offset CCCC
    kFmt30t,        // op +AAAAAAAA
    kFmt32x,        // op vAAAA, vBBBB
    kFmt31i,        // op vAA, #+BBBBBBBB
    kFmt31t,        // op vAA, +BBBBBBBB
    kFmt31c,        // op vAA, string@BBBBBBBB
    kFmt35c,        // op {vC,vD,vE,vF,vG}, thing@BBBB
    kFmt35ms,       // [opt] invoke-virtual+super
    kFmt3rc,        // op {vCCCC .. v(CCCC+AA-1)}, thing@BBBB
    kFmt3rms,       // [opt] invoke-virtual+super/range
    kFmt51l,        // op vAA, #+BBBBBBBBBBBBBBBB
    kFmt35mi,       // [opt] inline invoke
    kFmt3rmi,       // [opt] inline invoke/range
};

/*
 * Types of indexed reference that are associated with opcodes whose
 * formats include such an indexed reference (e.g., 21c and 35c).
 */
enum InstructionIndexType {
    kIndexUnknown = 0,
    kIndexNone,         // has no index
    kIndexVaries,       // "It depends." Used for throw-verification-error
    kIndexTypeRef,      // type reference index
    kIndexStringRef,    // string reference index
    kIndexMethodRef,    // method reference index
    kIndexFieldRef,     // field reference index
    kIndexInlineMethod, // inline method index (for inline linked methods)
    kIndexVtableOffset, // vtable offset (for static linked methods)
    kIndexFieldOffset   // field offset (for static linked fields)
};

/*
 * Instruction width implied by an opcode's format; a value in the
 * range 0 to 5. Note that there are special "pseudo-instructions"
 * which are used to encode switch and data tables, and these don't
 * have a fixed width. See dexGetWidthFromInstruction(), below.
 */
typedef u1 InstructionWidth;

/*
 * Opcode control flow flags, used by the verifier and JIT.
 */
typedef u1 OpcodeFlags;
enum OpcodeFlagsBits {
    kInstrCanBranch     = 1,        // conditional or unconditional branch
    kInstrCanContinue   = 1 << 1,   // flow can continue to next statement
    kInstrCanSwitch     = 1 << 2,   // switch statement
    kInstrCanThrow      = 1 << 3,   // could cause an exception to be thrown
    kInstrCanReturn     = 1 << 4,   // returns, no additional statements
    kInstrInvoke        = 1 << 5,   // a flavor of invoke
};

/*
 * Struct that includes a pointer to each of the opcode information
 * tables.
 *
 * Note: We use "u1*" here instead of the names of the enumerated
 * types to guarantee that elements don't use much space. We hold out
 * hope for a standard way to indicate the size of an enumerated type
 * that works for both C and C++, but in the mean time, this will
 * suffice.
 */
struct InstructionInfoTables {
    u1*                formats;    /* InstructionFormat elements */
    u1*                indexTypes; /* InstructionIndexType elements */
    OpcodeFlags*       flags;
    InstructionWidth*  widths;
};

/*
 * Global InstructionInfoTables struct.
 */
extern InstructionInfoTables gDexOpcodeInfo;

/*
 * Holds the contents of a decoded instruction.
 */
struct DecodedInstruction {
    u4      vA;
    u4      vB;
    u8      vB_wide;        /* for kFmt51l */
    u4      vC;
    u4      arg[5];         /* vC/D/E/F/G in invoke or filled-new-array */
    Opcode  opcode;
    InstructionIndexType indexType;
};

/*
 * Return the instruction width of the specified opcode, or 0 if not defined.
 */
DEX_INLINE size_t dexGetWidthFromOpcode(Opcode opcode)
{
    assert((u4) opcode < kNumPackedOpcodes);
    return gDexOpcodeInfo.widths[opcode];
}

/*
 * Return the width of the specified instruction, or 0 if not defined.  Also
 * works for special OP_NOP entries, including switch statement data tables
 * and array data.
 */
size_t dexGetWidthFromInstruction(const u2* insns);

/*
 * Returns the flags for the specified opcode.
 */
DEX_INLINE OpcodeFlags dexGetFlagsFromOpcode(Opcode opcode)
{
    assert((u4) opcode < kNumPackedOpcodes);
    return gDexOpcodeInfo.flags[opcode];
}

/*
 * Returns true if the given flags represent a goto (unconditional branch).
 */
DEX_INLINE bool dexIsGoto(OpcodeFlags flags)
{
    return (flags & (kInstrCanBranch | kInstrCanContinue)) == kInstrCanBranch;
}

/*
 * Return the instruction format for the specified opcode.
 */
DEX_INLINE InstructionFormat dexGetFormatFromOpcode(Opcode opcode)
{
    assert((u4) opcode < kNumPackedOpcodes);
    return (InstructionFormat) gDexOpcodeInfo.formats[opcode];
}

/*
 * Return the instruction index type for the specified opcode.
 */
DEX_INLINE InstructionIndexType dexGetIndexTypeFromOpcode(Opcode opcode)
{
    assert((u4) opcode < kNumPackedOpcodes);
    return (InstructionIndexType) gDexOpcodeInfo.indexTypes[opcode];
}

/*
 * Decode the instruction pointed to by "insns".
 */
void dexDecodeInstruction(const u2* insns, DecodedInstruction* pDec);

#endif  // LIBDEX_INSTRUTILS_H_
