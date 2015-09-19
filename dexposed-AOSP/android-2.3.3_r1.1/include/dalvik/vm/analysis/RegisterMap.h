/*
 * Copyright (C) 2009 The Android Open Source Project
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
 * Declaration of register map data structure and related functions.
 *
 * These structures should be treated as opaque through most of the VM.
 */
#ifndef DALVIK_REGISTERMAP_H_
#define DALVIK_REGISTERMAP_H_

#include "analysis/VerifySubs.h"
#include "analysis/CodeVerify.h"

/*
 * Format enumeration for RegisterMap data area.
 */
enum RegisterMapFormat {
    kRegMapFormatUnknown = 0,
    kRegMapFormatNone,          /* indicates no map data follows */
    kRegMapFormatCompact8,      /* compact layout, 8-bit addresses */
    kRegMapFormatCompact16,     /* compact layout, 16-bit addresses */
    kRegMapFormatDifferential,  /* compressed, differential encoding */

    kRegMapFormatOnHeap = 0x80, /* bit flag, indicates allocation on heap */
};

/*
 * This is a single variable-size structure.  It may be allocated on the
 * heap or mapped out of a (post-dexopt) DEX file.
 *
 * 32-bit alignment of the structure is NOT guaranteed.  This makes it a
 * little awkward to deal with as a structure; to avoid accidents we use
 * only byte types.  Multi-byte values are little-endian.
 *
 * Size of (format==FormatNone): 1 byte
 * Size of (format==FormatCompact8): 4 + (1 + regWidth) * numEntries
 * Size of (format==FormatCompact16): 4 + (2 + regWidth) * numEntries
 */
struct RegisterMap {
    /* header */
    u1      format;         /* enum RegisterMapFormat; MUST be first entry */
    u1      regWidth;       /* bytes per register line, 1+ */
    u1      numEntries[2];  /* number of entries */

    /* raw data starts here; need not be aligned */
    u1      data[1];
};

bool dvmRegisterMapStartup(void);
void dvmRegisterMapShutdown(void);

/*
 * Get the format.
 */
INLINE RegisterMapFormat dvmRegisterMapGetFormat(const RegisterMap* pMap) {
    return (RegisterMapFormat)(pMap->format & ~(kRegMapFormatOnHeap));
}

/*
 * Set the format.
 */
INLINE void dvmRegisterMapSetFormat(RegisterMap* pMap, RegisterMapFormat format)
{
    pMap->format &= kRegMapFormatOnHeap;
    pMap->format |= format;
}

/*
 * Get the "on heap" flag.
 */
INLINE bool dvmRegisterMapGetOnHeap(const RegisterMap* pMap) {
    return (pMap->format & kRegMapFormatOnHeap) != 0;
}

/*
 * Get the register bit vector width, in bytes.
 */
INLINE u1 dvmRegisterMapGetRegWidth(const RegisterMap* pMap) {
    return pMap->regWidth;
}

/*
 * Set the register bit vector width, in bytes.
 */
INLINE void dvmRegisterMapSetRegWidth(RegisterMap* pMap, int regWidth) {
    pMap->regWidth = regWidth;
}

/*
 * Set the "on heap" flag.
 */
INLINE void dvmRegisterMapSetOnHeap(RegisterMap* pMap, bool val) {
    if (val)
        pMap->format |= kRegMapFormatOnHeap;
    else
        pMap->format &= ~(kRegMapFormatOnHeap);
}

/*
 * Get the number of entries in this map.
 */
INLINE u2 dvmRegisterMapGetNumEntries(const RegisterMap* pMap) {
    return pMap->numEntries[0] | (pMap->numEntries[1] << 8);
}

/*
 * Set the number of entries in this map.
 */
INLINE void dvmRegisterMapSetNumEntries(RegisterMap* pMap, u2 numEntries) {
    pMap->numEntries[0] = (u1) numEntries;
    pMap->numEntries[1] = numEntries >> 8;
}

/*
 * Retrieve the bit vector for the specified address.  This is a pointer
 * to the bit data from an uncompressed map, or to a temporary copy of
 * data from a compressed map.
 *
 * The caller must call dvmReleaseRegisterMapLine() with the result.
 *
 * Returns NULL if not found.
 */
const u1* dvmRegisterMapGetLine(const RegisterMap* pMap, int addr);

/*
 * Release "data".
 *
 * If "pMap" points to a compressed map from which we have expanded a
 * single line onto the heap, this will free "data"; otherwise, it does
 * nothing.
 *
 * TODO: decide if this is still a useful concept.
 */
INLINE void dvmReleaseRegisterMapLine(const RegisterMap* pMap, const u1* data)
{}


/*
 * A pool of register maps for methods associated with a single class.
 *
 * Each entry is a 4-byte method index followed by the 32-bit-aligned
 * RegisterMap.  The size of the RegisterMap is determined by parsing
 * the map.  The lack of an index reduces random access speed, but we
 * should be doing that rarely (during class load) and it saves space.
 *
 * These structures are 32-bit aligned.
 */
struct RegisterMapMethodPool {
    u2      methodCount;            /* chiefly used as a sanity check */

    /* stream of per-method data starts here */
    u4      methodData[1];
};

/*
 * Header for the memory-mapped RegisterMap pool in the DEX file.
 *
 * The classDataOffset table provides offsets from the start of the
 * RegisterMapPool structure.  There is one entry per class (including
 * interfaces, which can have static initializers).
 *
 * The offset points to a RegisterMapMethodPool.
 *
 * These structures are 32-bit aligned.
 */
struct RegisterMapClassPool {
    u4      numClasses;

    /* offset table starts here, 32-bit aligned; offset==0 means no data */
    u4      classDataOffset[1];
};

/*
 * Find the register maps for this class.  (Used during class loading.)
 * If "pNumMaps" is non-NULL, it will return the number of maps in the set.
 *
 * Returns NULL if none is available.
 */
const void* dvmRegisterMapGetClassData(const DexFile* pDexFile, u4 classIdx,
    u4* pNumMaps);

/*
 * Get the register map for the next method.  "*pPtr" will be advanced past
 * the end of the map.  (Used during class loading.)
 *
 * This should initially be called with the result from
 * dvmRegisterMapGetClassData().
 */
const RegisterMap* dvmRegisterMapGetNext(const void** pPtr);

/*
 * This holds some meta-data while we construct the set of register maps
 * for a DEX file.
 *
 * In particular, it keeps track of our temporary mmap region so we can
 * free it later.
 */
struct RegisterMapBuilder {
    /* public */
    void*       data;
    size_t      size;

    /* private */
    MemMapping  memMap;
};

/*
 * Generate a register map set for all verified classes in "pDvmDex".
 */
RegisterMapBuilder* dvmGenerateRegisterMaps(DvmDex* pDvmDex);

/*
 * Free the builder.
 */
void dvmFreeRegisterMapBuilder(RegisterMapBuilder* pBuilder);

/*
 * Generate the register map for a method that has just been verified
 * (i.e. we're doing this as part of verification).
 *
 * Returns a pointer to a newly-allocated RegisterMap, or NULL on failure.
 */
RegisterMap* dvmGenerateRegisterMapV(VerifierData* vdata);

/*
 * Get the expanded form of the register map associated with the specified
 * method.  May update method->registerMap, possibly freeing the previous
 * map.
 *
 * Returns NULL on failure (e.g. unable to expand map).
 *
 * NOTE: this function is not synchronized; external locking is mandatory.
 * (This is expected to be called at GC time.)
 */
const RegisterMap* dvmGetExpandedRegisterMap0(Method* method);
INLINE const RegisterMap* dvmGetExpandedRegisterMap(Method* method)
{
    const RegisterMap* curMap = method->registerMap;
    if (curMap == NULL)
        return NULL;
    RegisterMapFormat format = dvmRegisterMapGetFormat(curMap);
    if (format == kRegMapFormatCompact8 || format == kRegMapFormatCompact16) {
        return curMap;
    } else {
        return dvmGetExpandedRegisterMap0(method);
    }
}

/* dump stats gathered during register map creation process */
void dvmRegisterMapDumpStats(void);

#endif  // DALVIK_REGISTERMAP_H_
