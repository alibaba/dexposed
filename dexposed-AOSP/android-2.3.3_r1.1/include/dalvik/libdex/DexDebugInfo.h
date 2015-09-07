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

/*
 * Handling of method debug info in a .dex file.
 */

#ifndef LIBDEX_DEXDEBUGINFO_H_
#define LIBDEX_DEXDEBUGINFO_H_

#include "DexFile.h"

/*
 * Callback for "new position table entry".
 * Returning non-0 causes the decoder to stop early.
 */
typedef int (*DexDebugNewPositionCb)(void *cnxt, u4 address, u4 lineNum);

/*
 * Callback for "new locals table entry". "signature" is an empty string
 * if no signature is available for an entry.
 */
typedef void (*DexDebugNewLocalCb)(void *cnxt, u2 reg, u4 startAddress,
        u4 endAddress, const char *name, const char *descriptor,
        const char *signature);

/*
 * Decode debug info for method.
 *
 * posCb is called in ascending address order.
 * localCb is called in order of ascending end address.
 */
void dexDecodeDebugInfo(
            const DexFile* pDexFile,
            const DexCode* pDexCode,
            const char* classDescriptor,
            u4 protoIdx,
            u4 accessFlags,
            DexDebugNewPositionCb posCb, DexDebugNewLocalCb localCb,
            void* cnxt);

#endif  // LIBDEX_DEXDEBUGINFO_H_
