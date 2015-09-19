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
 * General purpose hash table, used for finding classes, methods, etc.
 *
 * When the number of elements reaches a certain percentage of the table's
 * capacity, the table will be resized.
 */
#ifndef DALVIK_HASH_H_
#define DALVIK_HASH_H_

/* compute the hash of an item with a specific type */
typedef u4 (*HashCompute)(const void* item);

/*
 * Compare a hash entry with a "loose" item after their hash values match.
 * Returns { <0, 0, >0 } depending on ordering of items (same semantics
 * as strcmp()).
 */
typedef int (*HashCompareFunc)(const void* tableItem, const void* looseItem);

/*
 * This function will be used to free entries in the table.  This can be
 * NULL if no free is required, free(), or a custom function.
 */
typedef void (*HashFreeFunc)(void* ptr);

/*
 * Used by dvmHashForeach().
 */
typedef int (*HashForeachFunc)(void* data, void* arg);

/*
 * Used by dvmHashForeachRemove().
 */
typedef int (*HashForeachRemoveFunc)(void* data);

/*
 * One entry in the hash table.  "data" values are expected to be (or have
 * the same characteristics as) valid pointers.  In particular, a NULL
 * value for "data" indicates an empty slot, and HASH_TOMBSTONE indicates
 * a no-longer-used slot that must be stepped over during probing.
 *
 * Attempting to add a NULL or tombstone value is an error.
 *
 * When an entry is released, we will call (HashFreeFunc)(entry->data).
 */
struct HashEntry {
    u4 hashValue;
    void* data;
};

#define HASH_TOMBSTONE ((void*) 0xcbcacccd)     // invalid ptr value

/*
 * Expandable hash table.
 *
 * This structure should be considered opaque.
 */
struct HashTable {
    int         tableSize;          /* must be power of 2 */
    int         numEntries;         /* current #of "live" entries */
    int         numDeadEntries;     /* current #of tombstone entries */
    HashEntry*  pEntries;           /* array on heap */
    HashFreeFunc freeFunc;
    pthread_mutex_t lock;
};

/*
 * Create and initialize a HashTable structure, using "initialSize" as
 * a basis for the initial capacity of the table.  (The actual initial
 * table size may be adjusted upward.)  If you know exactly how many
 * elements the table will hold, pass the result from dvmHashSize() in.)
 *
 * Returns "false" if unable to allocate the table.
 */
HashTable* dvmHashTableCreate(size_t initialSize, HashFreeFunc freeFunc);

/*
 * Compute the capacity needed for a table to hold "size" elements.  Use
 * this when you know ahead of time how many elements the table will hold.
 * Pass this value into dvmHashTableCreate() to ensure that you can add
 * all elements without needing to reallocate the table.
 */
size_t dvmHashSize(size_t size);

/*
 * Clear out a hash table, freeing the contents of any used entries.
 */
void dvmHashTableClear(HashTable* pHashTable);

/*
 * Free a hash table.  Performs a "clear" first.
 */
void dvmHashTableFree(HashTable* pHashTable);

/*
 * Exclusive access.  Important when adding items to a table, or when
 * doing any operations on a table that could be added to by another thread.
 */
INLINE void dvmHashTableLock(HashTable* pHashTable) {
    dvmLockMutex(&pHashTable->lock);
}
INLINE void dvmHashTableUnlock(HashTable* pHashTable) {
    dvmUnlockMutex(&pHashTable->lock);
}

/*
 * Get #of entries in hash table.
 */
INLINE int dvmHashTableNumEntries(HashTable* pHashTable) {
    return pHashTable->numEntries;
}

/*
 * Get total size of hash table (for memory usage calculations).
 */
INLINE int dvmHashTableMemUsage(HashTable* pHashTable) {
    return sizeof(HashTable) + pHashTable->tableSize * sizeof(HashEntry);
}

/*
 * Look up an entry in the table, possibly adding it if it's not there.
 *
 * If "item" is not found, and "doAdd" is false, NULL is returned.
 * Otherwise, a pointer to the found or added item is returned.  (You can
 * tell the difference by seeing if return value == item.)
 *
 * An "add" operation may cause the entire table to be reallocated.  Don't
 * forget to lock the table before calling this.
 */
void* dvmHashTableLookup(HashTable* pHashTable, u4 itemHash, void* item,
    HashCompareFunc cmpFunc, bool doAdd);

/*
 * Remove an item from the hash table, given its "data" pointer.  Does not
 * invoke the "free" function; just detaches it from the table.
 */
bool dvmHashTableRemove(HashTable* pHashTable, u4 hash, void* item);

/*
 * Execute "func" on every entry in the hash table.
 *
 * If "func" returns a nonzero value, terminate early and return the value.
 */
int dvmHashForeach(HashTable* pHashTable, HashForeachFunc func, void* arg);

/*
 * Execute "func" on every entry in the hash table.
 *
 * If "func" returns 1 detach the entry from the hash table. Does not invoke
 * the "free" function.
 *
 * Returning values other than 0 or 1 from "func" will abort the routine.
 */
int dvmHashForeachRemove(HashTable* pHashTable, HashForeachRemoveFunc func);

/*
 * An alternative to dvmHashForeach(), using an iterator.
 *
 * Use like this:
 *   HashIter iter;
 *   for (dvmHashIterBegin(hashTable, &iter); !dvmHashIterDone(&iter);
 *       dvmHashIterNext(&iter))
 *   {
 *       MyData* data = (MyData*)dvmHashIterData(&iter);
 *   }
 */
struct HashIter {
    void*       data;
    HashTable*  pHashTable;
    int         idx;
};
INLINE void dvmHashIterNext(HashIter* pIter) {
    int i = pIter->idx +1;
    int lim = pIter->pHashTable->tableSize;
    for ( ; i < lim; i++) {
        void* data = pIter->pHashTable->pEntries[i].data;
        if (data != NULL && data != HASH_TOMBSTONE)
            break;
    }
    pIter->idx = i;
}
INLINE void dvmHashIterBegin(HashTable* pHashTable, HashIter* pIter) {
    pIter->pHashTable = pHashTable;
    pIter->idx = -1;
    dvmHashIterNext(pIter);
}
INLINE bool dvmHashIterDone(HashIter* pIter) {
    return (pIter->idx >= pIter->pHashTable->tableSize);
}
INLINE void* dvmHashIterData(HashIter* pIter) {
    assert(pIter->idx >= 0 && pIter->idx < pIter->pHashTable->tableSize);
    return pIter->pHashTable->pEntries[pIter->idx].data;
}


/*
 * Evaluate hash table performance by examining the number of times we
 * have to probe for an entry.
 *
 * The caller should lock the table beforehand.
 */
typedef u4 (*HashCalcFunc)(const void* item);
void dvmHashTableProbeCount(HashTable* pHashTable, HashCalcFunc calcFunc,
    HashCompareFunc cmpFunc);

#endif  // DALVIK_HASH_H_
