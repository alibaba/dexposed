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
 * Expanding byte buffer, with primitives for appending basic data types.
 */
#ifndef DALVIK_JDWP_EXPANDBUF_H_
#define DALVIK_JDWP_EXPANDBUF_H_

#include "Common.h"     // need u1/u2/u4/u8 types

struct ExpandBuf;   /* private */

/* create a new struct */
ExpandBuf* expandBufAlloc(void);
/* free storage */
void expandBufFree(ExpandBuf* pBuf);

/*
 * Accessors.  The buffer pointer and length will only be valid until more
 * data is added.
 */
u1* expandBufGetBuffer(ExpandBuf* pBuf);
size_t expandBufGetLength(ExpandBuf* pBuf);

/*
 * The "add" operations allocate additional storage and append the data.
 *
 * There are no "get" operations included with this "class", other than
 * GetBuffer().  If you want to get or set data from a position other
 * than the end, get a pointer to the buffer and use the inline functions
 * defined elsewhere.
 *
 * expandBufAddSpace() returns a pointer to the *start* of the region
 * added.
 */
u1* expandBufAddSpace(ExpandBuf* pBuf, int gapSize);
void expandBufAdd1(ExpandBuf* pBuf, u1 val);
void expandBufAdd2BE(ExpandBuf* pBuf, u2 val);
void expandBufAdd4BE(ExpandBuf* pBuf, u4 val);
void expandBufAdd8BE(ExpandBuf* pBuf, u8 val);
void expandBufAddUtf8String(ExpandBuf* pBuf, const u1* str);

#endif  // DALVIK_JDWP_EXPANDBUF_H_
