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
 * Mutex-free cache for key1+key2=value.
 */
#ifndef DALVIK_ATOMICCACHE_H_
#define DALVIK_ATOMICCACHE_H_

/*
 * If set to "1", gather some stats on our caching success rate.
 */
#define CALC_CACHE_STATS 0


/*
 * One entry in the cache.  We store two keys (e.g. the classes that are
 * arguments to "instanceof") and one result (e.g. a boolean value).
 *
 * Must be exactly 16 bytes.
 */
struct AtomicCacheEntry {
    u4          key1;
    u4          key2;
    u4          value;
    volatile u4 version;    /* version and lock flag */
};

/*
 * One cache.
 *
 * Thought: we might be able to save a few cycles by storing the cache
 * struct and "entries" separately, avoiding an indirection.  (We already
 * handle "numEntries" separately in ATOMIC_CACHE_LOOKUP.)
 */
struct AtomicCache {
    AtomicCacheEntry*   entries;        /* array of entries */
    int         numEntries;             /* #of entries, must be power of 2 */

    void*       entryAlloc;             /* memory allocated for entries */

    /* cache stats; note we don't guarantee atomic increments for these */
    int         trivial;                /* cache access not required */
    int         fail;                   /* contention failure */
    int         hits;                   /* found entry in cache */
    int         misses;                 /* entry was for other keys */
    int         fills;                  /* entry was empty */
};

/*
 * Do a cache lookup.  We need to be able to read and write entries
 * atomically.  There are a couple of ways to do this:
 *  (1) Have a global lock.  A mutex is too heavy, so instead we would use
 *      an atomic flag.  If the flag is set, we could sit and spin,
 *      but if we're a high-priority thread that may cause a lockup.
 *      Better to just ignore the cache and do the full computation.
 *  (2) Have a "version" that gets incremented atomically when a write
 *      begins and again when it completes.  Compare the version before
 *      and after doing reads.  So long as we have memory barriers in the
 *      right place the compiler and CPU will do the right thing, allowing
 *      us to skip atomic ops in the common read case.  The table updates
 *      are expensive, requiring two writes with barriers.  We also need
 *      some sort of lock to ensure that nobody else tries to start an
 *      update while we're in the middle of one.
 *
 * We expect a 95+% hit rate for the things we use this for, so #2 is
 * much better than #1.
 *
 * _cache is an AtomicCache*
 * _cacheSize is _cache->cacheSize (can save a cycle avoiding the lookup)
 * _key1, _key2 are the keys
 *
 * Define a function ATOMIC_CACHE_CALC that returns a 32-bit value.  This
 * will be invoked when we need to compute the value.
 *
 * Returns the value.
 */
#if CALC_CACHE_STATS > 0
# define CACHE_XARG(_value) ,_value
#else
# define CACHE_XARG(_value)
#endif
#define ATOMIC_CACHE_LOOKUP(_cache, _cacheSize, _key1, _key2) ({            \
    AtomicCacheEntry* pEntry;                                               \
    int hash;                                                               \
    u4 firstVersion, secondVersion;                                         \
    u4 value;                                                               \
                                                                            \
    /* simple hash function */                                              \
    hash = (((u4)(_key1) >> 2) ^ (u4)(_key2)) & ((_cacheSize)-1);           \
    pEntry = (_cache)->entries + hash;                                      \
                                                                            \
    firstVersion = android_atomic_acquire_load((int32_t*)&pEntry->version); \
                                                                            \
    if (pEntry->key1 == (u4)(_key1) && pEntry->key2 == (u4)(_key2)) {       \
        /*                                                                  \
         * The fields match.  Get the value, then read the version a        \
         * second time to verify that we didn't catch a partial update.     \
         * We're also hosed if "firstVersion" was odd, indicating that      \
         * an update was in progress before we got here (unlikely).         \
         */                                                                 \
        value = android_atomic_acquire_load((int32_t*) &pEntry->value);     \
        secondVersion = pEntry->version;                                    \
                                                                            \
        if ((firstVersion & 0x01) != 0 || firstVersion != secondVersion) {  \
            /*                                                              \
             * We clashed with another thread.  Instead of sitting and      \
             * spinning, which might not complete if we're a high priority  \
             * thread, just do the regular computation.                     \
             */                                                             \
            if (CALC_CACHE_STATS)                                           \
                (_cache)->fail++;                                           \
            value = (u4) ATOMIC_CACHE_CALC;                                 \
        } else {                                                            \
            /* all good */                                                  \
            if (CALC_CACHE_STATS)                                           \
                (_cache)->hits++;                                           \
        }                                                                   \
    } else {                                                                \
        /*                                                                  \
         * Compute the result and update the cache.  We really want this    \
         * to happen in a different method -- it makes the ARM frame        \
         * setup for this method simpler, which gives us a ~10% speed       \
         * boost.                                                           \
         */                                                                 \
        value = (u4) ATOMIC_CACHE_CALC;                                     \
        dvmUpdateAtomicCache((u4) (_key1), (u4) (_key2), value, pEntry,     \
                    firstVersion CACHE_XARG(_cache) );                      \
    }                                                                       \
    value;                                                                  \
})

/*
 * Allocate a cache.
 */
AtomicCache* dvmAllocAtomicCache(int numEntries);

/*
 * Free a cache.
 */
void dvmFreeAtomicCache(AtomicCache* cache);

/*
 * Update a cache entry.
 *
 * Making the last argument optional, instead of merely unused, saves us
 * a few percent in the ATOMIC_CACHE_LOOKUP time.
 */
void dvmUpdateAtomicCache(u4 key1, u4 key2, u4 value, AtomicCacheEntry* pEntry,
    u4 firstVersion
#if CALC_CACHE_STATS > 0
    , AtomicCache* pCache
#endif
    );

/*
 * Debugging.
 */
void dvmDumpAtomicCacheStats(const AtomicCache* pCache);

#endif  // DALVIK_ATOMICCACHE_H_
