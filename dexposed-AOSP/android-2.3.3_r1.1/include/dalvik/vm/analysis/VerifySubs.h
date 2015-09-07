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
 * Dalvik bytecode verification subroutines.
 */
#ifndef DALVIK_VERIFYSUBS_H_
#define DALVIK_VERIFYSUBS_H_

/*
 * InsnFlags is a 32-bit integer with the following layout:
 *   0-15  instruction length (or 0 if this address doesn't hold an opcode)
 *  16-31  single bit flags:
 *    InTry: in "try" block; exceptions thrown here may be caught locally
 *    BranchTarget: other instructions can branch to this instruction
 *    GcPoint: this instruction is a GC safe point
 *    Visited: verifier has examined this instruction at least once
 *    Changed: set/cleared as bytecode verifier runs
 */
typedef u4 InsnFlags;

#define kInsnFlagWidthMask      0x0000ffff
#define kInsnFlagInTry          (1 << 16)
#define kInsnFlagBranchTarget   (1 << 17)
#define kInsnFlagGcPoint        (1 << 18)
#define kInsnFlagVisited        (1 << 30)
#define kInsnFlagChanged        (1 << 31)

/* add opcode widths to InsnFlags */
bool dvmComputeCodeWidths(const Method* meth, InsnFlags* insnFlags,
    int* pNewInstanceCount);

/* set the "in try" flag for sections of code wrapped with a "try" block */
bool dvmSetTryFlags(const Method* meth, InsnFlags* insnFlags);

/* verification failure reporting */
#define LOG_VFY(...)                dvmLogVerifyFailure(NULL, __VA_ARGS__)
#define LOG_VFY_METH(_meth, ...)    dvmLogVerifyFailure(_meth, __VA_ARGS__)

/* log verification failure with optional method info */
void dvmLogVerifyFailure(const Method* meth, const char* format, ...)
#if defined(__GNUC__)
    __attribute__ ((format(printf, 2, 3)))
#endif
    ;

/* log verification failure due to resolution trouble */
void dvmLogUnableToResolveClass(const char* missingClassDescr,
    const Method* meth);

/* extract the relative branch offset from a branch instruction */
bool dvmGetBranchOffset(const Method* meth, const InsnFlags* insnFlags,
    int curOffset, s4* pOffset, bool* pConditional);

/* return a RegType enumeration value that "value" just fits into */
char dvmDetermineCat1Const(s4 value);

/* debugging */
bool dvmWantVerboseVerification(const Method* meth);

#endif  // DALVIK_VERIFYSUBS_H_
