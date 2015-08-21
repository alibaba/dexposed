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
 * Maintain an expanding set of unique pointer values.  The set is
 * kept in sorted order.
 */
#ifndef DALVIK_POINTERSET_H_
#define DALVIK_POINTERSET_H_

struct PointerSet;   /* private */

/*
 * Allocate a new PointerSet.
 *
 * Returns NULL on failure.
 */
PointerSet* dvmPointerSetAlloc(int initialSize);

/*
 * Free up a PointerSet.
 */
void dvmPointerSetFree(PointerSet* pSet);

/*
 * Clear the contents of a pointer set.
 */
void dvmPointerSetClear(PointerSet* pSet);

/*
 * Get the number of pointers currently stored in the list.
 */
int dvmPointerSetGetCount(const PointerSet* pSet);

/*
 * Get the Nth entry from the list.
 */
const void* dvmPointerSetGetEntry(const PointerSet* pSet, int i);

/*
 * Insert a new entry into the list.  If it already exists, this returns
 * without doing anything.
 *
 * Returns "true" if the pointer was added.
 */
bool dvmPointerSetAddEntry(PointerSet* pSet, const void* ptr);

/*
 * Returns "true" if the element was successfully removed.
 */
bool dvmPointerSetRemoveEntry(PointerSet* pSet, const void* ptr);

/*
 * Returns "true" if the value appears, "false" otherwise.  If "pIndex" is
 * non-NULL, it will receive the matching index or the index of a nearby
 * element.
 */
bool dvmPointerSetHas(const PointerSet* pSet, const void* ptr, int* pIndex);

/*
 * Find an entry in the set.  Returns the index, or -1 if not found.
 */
INLINE int dvmPointerSetFind(const PointerSet* pSet, const void* ptr) {
    int idx;
    if (!dvmPointerSetHas(pSet, ptr, &idx))
        idx = -1;
    return idx;
}

/*
 * Compute the intersection of the set and the array of pointers passed in.
 *
 * Any pointer in "pSet" that does not appear in "ptrArray" is removed.
 */
void dvmPointerSetIntersect(PointerSet* pSet, const void** ptrArray, int count);

/*
 * Print the list contents to stdout.  For debugging.
 */
void dvmPointerSetDump(const PointerSet* pSet);

#endif  // DALVIK_POINTERSET_H_
