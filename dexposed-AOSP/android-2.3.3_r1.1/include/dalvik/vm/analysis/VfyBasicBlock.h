/*
 * Copyright (C) 2010 The Android Open Source Project
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
 * Basic block functions, as used by the verifier.  (The names were chosen
 * to avoid conflicts with similar structures used by the compiler.)
 */
#ifndef DALVIK_VFYBASICBLOCK_H_
#define DALVIK_VFYBASICBLOCK_H_

#include "PointerSet.h"

struct VerifierData;


/*
 * Structure representing a basic block.
 *
 * This is used for liveness analysis, which is a reverse-flow algorithm,
 * so we need to mantain a list of predecessors for each block.
 *
 * "liveRegs" indicates the set of registers that are live at the end of
 * the basic block (after the last instruction has executed).  Successor
 * blocks will compare their results with this to see if this block needs
 * to be re-evaluated.  Note that this is not the same as the contents of
 * the RegisterLine for the last instruction in the block (which reflects
 * the state *before* the instruction has executed).
 */
struct VfyBasicBlock {
    u4              firstAddr;      /* address of first instruction */
    u4              lastAddr;       /* address of last instruction */
    PointerSet*     predecessors;   /* set of basic blocks that can flow here */
    BitVector*      liveRegs;       /* liveness for each register */
    bool            changed;        /* input set has changed, must re-eval */
    bool            visited;        /* block has been visited at least once */
};

/*
 * Generate a list of basic blocks.
 */
bool dvmComputeVfyBasicBlocks(struct VerifierData* vdata);

/*
 * Free storage allocated by dvmComputeVfyBasicBlocks.
 */
void dvmFreeVfyBasicBlocks(struct VerifierData* vdata);

#endif  // DALVIK_VFYBASICBLOCK_H_
