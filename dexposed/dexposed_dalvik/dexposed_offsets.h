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

/*
    Certain compile time parameters result in different offsets
    for members in structures. This file defines the offsets for
    members which cannot be accessed otherwise and some macros
    to simplify accessing them.
*/

#define MEMBER_OFFSET_ARRAY(type,member) offsets_array_ ## type ## _ ## member
#define MEMBER_OFFSET_VAR(type,member) offset_ ## type ## _ ## member
#define MEMBER_TYPE(type,member) offset_type_ ## type ## _ ## member

#define MEMBER_PTR(obj,type,member) \
    ( (MEMBER_TYPE(type,member)*)  ( (char*)(obj) + MEMBER_OFFSET_VAR(type,member) ) )
#define MEMBER_VAL(obj,type,member) *MEMBER_PTR(obj,type,member)

#define MEMBER_OFFSET_DEFINE(type,member,offsets...) \
    static int MEMBER_OFFSET_ARRAY(type,member)[] = { offsets }; \
    static int MEMBER_OFFSET_VAR(type,member);
#define MEMBER_OFFSET_COPY(type,member) MEMBER_OFFSET_VAR(type,member) = MEMBER_OFFSET_ARRAY(type,member)[offsetMode]


// here are the definitions of the modes and offsets
enum dexposedOffsetModes {
    MEMBER_OFFSET_MODE_WITH_JIT,
    MEMBER_OFFSET_MODE_NO_JIT,
};
static dexposedOffsetModes offsetMode;
const char* dexposedOffsetModesDesc[] = {
    "WITH_JIT",
    "NO_JIT",
};

MEMBER_OFFSET_DEFINE(DvmJitGlobals, codeCacheFull, 120, 0)
#define offset_type_DvmJitGlobals_codeCacheFull bool



// helper to determine the required values (compile with DEXPOSED_SHOW_OFFSET=true)
#ifdef dexposed_SHOW_OFFSETS
    template<int s> struct RESULT;
    #ifdef WITH_JIT
        #pragma message "WITH_JIT is defined"
    #else
       #pragma message "WITH_JIT is not defined"
    #endif
    RESULT<sizeof(Method)> SIZEOF_Method;
    RESULT<sizeof(Thread)> SIZEOF_Thread;
    RESULT<offsetof(DvmJitGlobals, codeCacheFull)> OFFSETOF_DvmJitGlobals_codeCacheFull;
#endif


