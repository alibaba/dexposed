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
 * Dalvik classfile verification.
 */
#ifndef DALVIK_DEXVERIFY_H_
#define DALVIK_DEXVERIFY_H_

/*
 * Global verification mode.  These must be in order from least verification
 * to most.  If we're using "exact GC", we may need to perform some of
 * the verification steps anyway.
 */
enum DexClassVerifyMode {
    VERIFY_MODE_UNKNOWN = 0,
    VERIFY_MODE_NONE,
    VERIFY_MODE_REMOTE,
    VERIFY_MODE_ALL
};

/* some verifier counters, for debugging */
struct VerifierStats {
    size_t methodsExamined;    /* number of methods examined */
    size_t monEnterMethods;    /* number of methods with monitor-enter */
    size_t instrsExamined;     /* incr on first visit of instruction */
    size_t instrsReexamined;   /* incr on each repeat visit of instruction */
    size_t copyRegCount;       /* calls from updateRegisters->copyRegisters */
    size_t mergeRegCount;      /* calls from updateRegisters->merge */
    size_t mergeRegChanged;    /* calls from updateRegisters->merge, changed */
    size_t uninitSearches;     /* times we've had to search the uninit table */
    size_t biggestAlloc;       /* largest RegisterLine table alloc */
};

/*
 * Certain types of instructions can be GC points.  To support precise
 * GC, all such instructions must export the PC in the interpreter,
 * or the GC won't be able to identify the current PC for the thread.
 */
#define VERIFY_GC_INST_MASK (kInstrCanBranch | kInstrCanSwitch |\
                             kInstrCanThrow | kInstrCanReturn)

/*
 * Verify a single class.
 */
bool dvmVerifyClass(ClassObject* clazz);

/*
 * Release the storage associated with a RegisterMap.
 */
void dvmFreeRegisterMap(RegisterMap* pMap);

#endif  // DALVIK_DEXVERIFY_H_
