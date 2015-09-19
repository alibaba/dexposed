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
 * Simple linear memory allocator.
 */
#ifndef DALVIK_LINEARALLOC_H_
#define DALVIK_LINEARALLOC_H_

/*
 * If this is set, we create additional data structures and make many
 * additional mprotect() calls.
 */
#define ENFORCE_READ_ONLY   false

/*
 * Linear allocation state.  We could tuck this into the start of the
 * allocated region, but that would prevent us from sharing the rest of
 * that first page.
 */
struct LinearAllocHdr {
    int     curOffset;          /* offset where next data goes */
    pthread_mutex_t lock;       /* controls updates to this struct */

    char*   mapAddr;            /* start of mmap()ed region */
    int     mapLength;          /* length of region */
    int     firstOffset;        /* for chasing through */

    short*  writeRefCount;      /* for ENFORCE_READ_ONLY */
};


/*
 * Create a new alloc region.
 */
LinearAllocHdr* dvmLinearAllocCreate(Object* classLoader);

/*
 * Destroy a region.
 */
void dvmLinearAllocDestroy(Object* classLoader);

/*
 * Allocate a chunk of memory.  The memory will be zeroed out.
 *
 * For ENFORCE_READ_ONLY, call dvmLinearReadOnly on the result.
 */
void* dvmLinearAlloc(Object* classLoader, size_t size);

/*
 * Reallocate a chunk.  The original storage is not released, but may be
 * erased to aid debugging.
 *
 * For ENFORCE_READ_ONLY, call dvmLinearReadOnly on the result.  Also, the
 * caller should probably mark the "mem" argument read-only before calling.
 */
void* dvmLinearRealloc(Object* classLoader, void* mem, size_t newSize);

/* don't call these directly */
void dvmLinearSetReadOnly(Object* classLoader, void* mem);
void dvmLinearSetReadWrite(Object* classLoader, void* mem);

/*
 * Mark a chunk of memory from Alloc or Realloc as read-only.  This must
 * be done after all changes to the block of memory have been made.  This
 * actually operates on a page granularity.
 */
INLINE void dvmLinearReadOnly(Object* classLoader, void* mem)
{
    if (ENFORCE_READ_ONLY && mem != NULL)
        dvmLinearSetReadOnly(classLoader, mem);
}

/*
 * Make a chunk of memory writable again.
 */
INLINE void dvmLinearReadWrite(Object* classLoader, void* mem)
{
    if (ENFORCE_READ_ONLY && mem != NULL)
        dvmLinearSetReadWrite(classLoader, mem);
}

/*
 * Free a chunk.  Does not increase available storage, but the freed area
 * may be erased to aid debugging.
 */
void dvmLinearFree(Object* classLoader, void* mem);

/*
 * Helper function; allocates new storage and copies "str" into it.
 *
 * For ENFORCE_READ_ONLY, do *not* call dvmLinearReadOnly on the result.
 * This is done automatically.
 */
char* dvmLinearStrdup(Object* classLoader, const char* str);

/*
 * Dump the contents of a linear alloc area.
 */
void dvmLinearAllocDump(Object* classLoader);

/*
 * Determine if [start, start+length) is contained in the in-use area of
 * a single LinearAlloc.  The full set of linear allocators is scanned.
 */
bool dvmLinearAllocContains(const void* start, size_t length);

#endif  // DALVIK_LINEARALLOC_H_
