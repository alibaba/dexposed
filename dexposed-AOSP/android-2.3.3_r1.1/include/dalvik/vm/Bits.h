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
 * Some handy functions for manipulating bits and bytes.
 *
 * These get inlined, so prefer small size over maximum speed.
 */
#ifndef DALVIK_BITS_H_
#define DALVIK_BITS_H_

#include "Common.h"
#include "Inlines.h"

#include <stdlib.h>
#include <string.h>

/*
 * Get 1 byte.  (Included to make the code more legible.)
 */
INLINE u1 get1(unsigned const char* pSrc)
{
    return *pSrc;
}

/*
 * Get 2 big-endian bytes.
 */
INLINE u2 get2BE(unsigned char const* pSrc)
{
    return (pSrc[0] << 8) | pSrc[1];
}

/*
 * Get 4 big-endian bytes.
 */
INLINE u4 get4BE(unsigned char const* pSrc)
{
    return (pSrc[0] << 24) | (pSrc[1] << 16) | (pSrc[2] << 8) | pSrc[3];
}

/*
 * Get 8 big-endian bytes.
 */
INLINE u8 get8BE(unsigned char const* pSrc)
{
    u4 low, high;

    high = pSrc[0];
    high = (high << 8) | pSrc[1];
    high = (high << 8) | pSrc[2];
    high = (high << 8) | pSrc[3];
    low = pSrc[4];
    low = (low << 8) | pSrc[5];
    low = (low << 8) | pSrc[6];
    low = (low << 8) | pSrc[7];

    return ((u8) high << 32) | (u8) low;
}

/*
 * Get 2 little-endian bytes.
 */
INLINE u2 get2LE(unsigned char const* pSrc)
{
    return pSrc[0] | (pSrc[1] << 8);
}

/*
 * Get 4 little-endian bytes.
 */
INLINE u4 get4LE(unsigned char const* pSrc)
{
    u4 result;

    result = pSrc[0];
    result |= pSrc[1] << 8;
    result |= pSrc[2] << 16;
    result |= pSrc[3] << 24;

    return result;
}

/*
 * Get 8 little-endian bytes.
 */
INLINE u8 get8LE(unsigned char const* pSrc)
{
    u4 low, high;

    low = pSrc[0];
    low |= pSrc[1] << 8;
    low |= pSrc[2] << 16;
    low |= pSrc[3] << 24;
    high = pSrc[4];
    high |= pSrc[5] << 8;
    high |= pSrc[6] << 16;
    high |= pSrc[7] << 24;
    return ((u8) high << 32) | (u8) low;
}

/*
 * Grab 1 byte and advance the data pointer.
 */
INLINE u1 read1(unsigned const char** ppSrc)
{
    return *(*ppSrc)++;
}

/*
 * Grab 2 big-endian bytes and advance the data pointer.
 */
INLINE u2 read2BE(unsigned char const** ppSrc)
{
    const unsigned char* pSrc = *ppSrc;

    *ppSrc = pSrc + 2;
    return pSrc[0] << 8 | pSrc[1];
}

/*
 * Grab 4 big-endian bytes and advance the data pointer.
 */
INLINE u4 read4BE(unsigned char const** ppSrc)
{
    const unsigned char* pSrc = *ppSrc;
    u4 result;

    result = pSrc[0] << 24;
    result |= pSrc[1] << 16;
    result |= pSrc[2] << 8;
    result |= pSrc[3];

    *ppSrc = pSrc + 4;
    return result;
}

/*
 * Get 8 big-endian bytes and advance the data pointer.
 */
INLINE u8 read8BE(unsigned char const** ppSrc)
{
    const unsigned char* pSrc = *ppSrc;
    u4 low, high;

    high = pSrc[0];
    high = (high << 8) | pSrc[1];
    high = (high << 8) | pSrc[2];
    high = (high << 8) | pSrc[3];
    low = pSrc[4];
    low = (low << 8) | pSrc[5];
    low = (low << 8) | pSrc[6];
    low = (low << 8) | pSrc[7];

    *ppSrc = pSrc + 8;
    return ((u8) high << 32) | (u8) low;
}

/*
 * Grab 2 little-endian bytes and advance the data pointer.
 */
INLINE u2 read2LE(unsigned char const** ppSrc)
{
    const unsigned char* pSrc = *ppSrc;
    *ppSrc = pSrc + 2;
    return pSrc[0] | pSrc[1] << 8;
}

/*
 * Grab 4 little-endian bytes and advance the data pointer.
 */
INLINE u4 read4LE(unsigned char const** ppSrc)
{
    const unsigned char* pSrc = *ppSrc;
    u4 result;

    result = pSrc[0];
    result |= pSrc[1] << 8;
    result |= pSrc[2] << 16;
    result |= pSrc[3] << 24;

    *ppSrc = pSrc + 4;
    return result;
}

/*
 * Get 8 little-endian bytes and advance the data pointer.
 */
INLINE u8 read8LE(unsigned char const** ppSrc)
{
    const unsigned char* pSrc = *ppSrc;
    u4 low, high;

    low = pSrc[0];
    low |= pSrc[1] << 8;
    low |= pSrc[2] << 16;
    low |= pSrc[3] << 24;
    high = pSrc[4];
    high |= pSrc[5] << 8;
    high |= pSrc[6] << 16;
    high |= pSrc[7] << 24;

    *ppSrc = pSrc + 8;
    return ((u8) high << 32) | (u8) low;
}

/*
 * Skip over a UTF-8 string (preceded by a 4-byte length).
 */
INLINE void skipUtf8String(unsigned char const** ppSrc)
{
    u4 length = read4BE(ppSrc);

    (*ppSrc) += length;
}

/*
 * Read a UTF-8 string into a fixed-size buffer, and null-terminate it.
 *
 * Returns the length of the original string.
 */
INLINE int readUtf8String(unsigned char const** ppSrc, char* buf, size_t bufLen)
{
    u4 length = read4BE(ppSrc);
    size_t copyLen = (length < bufLen) ? length : bufLen-1;

    memcpy(buf, *ppSrc, copyLen);
    buf[copyLen] = '\0';

    (*ppSrc) += length;
    return length;
}

/*
 * Read a UTF-8 string into newly-allocated storage, and null-terminate it.
 *
 * Returns the string and its length.  (The latter is probably unnecessary
 * for the way we're using UTF8.)
 */
INLINE char* readNewUtf8String(unsigned char const** ppSrc, size_t* pLength)
{
    u4 length = read4BE(ppSrc);
    char* buf;

    buf = (char*) malloc(length+1);

    memcpy(buf, *ppSrc, length);
    buf[length] = '\0';

    (*ppSrc) += length;

    *pLength = length;
    return buf;
}


/*
 * Set 1 byte.  (Included to make code more consistent/legible.)
 */
INLINE void set1(u1* buf, u1 val)
{
    *buf = (u1)(val);
}

/*
 * Set 2 big-endian bytes.
 */
INLINE void set2BE(u1* buf, u2 val)
{
    *buf++ = (u1)(val >> 8);
    *buf = (u1)(val);
}

/*
 * Set 4 big-endian bytes.
 */
INLINE void set4BE(u1* buf, u4 val)
{
    *buf++ = (u1)(val >> 24);
    *buf++ = (u1)(val >> 16);
    *buf++ = (u1)(val >> 8);
    *buf = (u1)(val);
}

/*
 * Set 8 big-endian bytes.
 */
INLINE void set8BE(u1* buf, u8 val)
{
    *buf++ = (u1)(val >> 56);
    *buf++ = (u1)(val >> 48);
    *buf++ = (u1)(val >> 40);
    *buf++ = (u1)(val >> 32);
    *buf++ = (u1)(val >> 24);
    *buf++ = (u1)(val >> 16);
    *buf++ = (u1)(val >> 8);
    *buf = (u1)(val);
}

/*
 * Set 2 little-endian bytes.
 */
INLINE void set2LE(u1* buf, u2 val)
{
    *buf++ = (u1)(val);
    *buf = (u1)(val >> 8);
}

/*
 * Set 4 little-endian bytes.
 */
INLINE void set4LE(u1* buf, u4 val)
{
    *buf++ = (u1)(val);
    *buf++ = (u1)(val >> 8);
    *buf++ = (u1)(val >> 16);
    *buf = (u1)(val >> 24);
}

/*
 * Set 8 little-endian bytes.
 */
INLINE void set8LE(u1* buf, u8 val)
{
    *buf++ = (u1)(val);
    *buf++ = (u1)(val >> 8);
    *buf++ = (u1)(val >> 16);
    *buf++ = (u1)(val >> 24);
    *buf++ = (u1)(val >> 32);
    *buf++ = (u1)(val >> 40);
    *buf++ = (u1)(val >> 48);
    *buf = (u1)(val >> 56);
}

/*
 * Stuff a UTF-8 string into the buffer.
 */
INLINE void setUtf8String(u1* buf, const u1* str)
{
    u4 strLen = strlen((const char*)str);

    set4BE(buf, strLen);
    memcpy(buf + sizeof(u4), str, strLen);
}

#endif  // DALVIK_BITS_H_
