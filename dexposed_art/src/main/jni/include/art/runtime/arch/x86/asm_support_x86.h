/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef ART_RUNTIME_ARCH_X86_ASM_SUPPORT_X86_H_
#define ART_RUNTIME_ARCH_X86_ASM_SUPPORT_X86_H_

#include "asm_support.h"

// Offset of field Thread::self_ verified in InitCpu
#define THREAD_SELF_OFFSET 156
// Offset of field Thread::card_table_ verified in InitCpu
#define THREAD_CARD_TABLE_OFFSET 120
// Offset of field Thread::exception_ verified in InitCpu
#define THREAD_EXCEPTION_OFFSET 124
// Offset of field Thread::thin_lock_thread_id_ verified in InitCpu
#define THREAD_ID_OFFSET 12

#define FRAME_SIZE_SAVE_ALL_CALLEE_SAVE 32
#define FRAME_SIZE_REFS_ONLY_CALLEE_SAVE 32
#define FRAME_SIZE_REFS_AND_ARGS_CALLEE_SAVE 32

// Expected size of a heap reference
#define HEAP_REFERENCE_SIZE 4

#endif  // ART_RUNTIME_ARCH_X86_ASM_SUPPORT_X86_H_
