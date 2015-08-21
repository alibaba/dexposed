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
#ifndef ART_RUNTIME_JDWP_JDWP_EXPAND_BUF_H_
#define ART_RUNTIME_JDWP_JDWP_EXPAND_BUF_H_

#include <string>

#include <stddef.h>
#include <stdint.h>

namespace art {

namespace JDWP {

struct ExpandBuf;   /* private */
struct JdwpLocation;

/* create a new struct */
ExpandBuf* expandBufAlloc();
/* free storage */
void expandBufFree(ExpandBuf* pBuf);

/*
 * Accessors.  The buffer pointer and length will only be valid until more
 * data is added.
 */
uint8_t* expandBufGetBuffer(ExpandBuf* pBuf);
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
uint8_t* expandBufAddSpace(ExpandBuf* pBuf, int gapSize);
void expandBufAdd1(ExpandBuf* pBuf, uint8_t val);
void expandBufAdd2BE(ExpandBuf* pBuf, uint16_t val);
void expandBufAdd4BE(ExpandBuf* pBuf, uint32_t val);
void expandBufAdd8BE(ExpandBuf* pBuf, uint64_t val);
void expandBufAddUtf8String(ExpandBuf* pBuf, const char* s);
void expandBufAddUtf8String(ExpandBuf* pBuf, const std::string& s);
void expandBufAddLocation(ExpandBuf* pReply, const JdwpLocation& location);

}  // namespace JDWP

}  // namespace art

#endif  // ART_RUNTIME_JDWP_JDWP_EXPAND_BUF_H_
