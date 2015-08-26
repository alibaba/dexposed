/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef ANDROID_CUTILS_ATOMIC_H
#define ANDROID_CUTILS_ATOMIC_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A handful of basic atomic operations.
 * THESE ARE HERE FOR LEGACY REASONS ONLY.  AVOID.
 *
 * PREFERRED ALTERNATIVES:
 * - Use C++/C/pthread locks/mutexes whenever there is not a
 *   convincing reason to do otherwise.  Note that very clever and
 *   complicated, but correct, lock-free code is often slower than
 *   using locks, especially where nontrivial data structures
 *   are involved.
 * - C11 stdatomic.h.
 * - Where supported, C++11 std::atomic<T> .
 *
 * PLEASE STOP READING HERE UNLESS YOU ARE TRYING TO UNDERSTAND
 * OR UPDATE OLD CODE.
 *
 * The "acquire" and "release" terms can be defined intuitively in terms
 * of the placement of memory barriers in a simple lock implementation:
 *   - wait until compare-and-swap(lock-is-free --> lock-is-held) succeeds
 *   - barrier
 *   - [do work]
 *   - barrier
 *   - store(lock-is-free)
 * In very crude terms, the initial (acquire) barrier prevents any of the
 * "work" from happening before the lock is held, and the later (release)
 * barrier ensures that all of the work happens before the lock is released.
 * (Think of cached writes, cache read-ahead, and instruction reordering
 * around the CAS and store instructions.)
 *
 * The barriers must apply to both the compiler and the CPU.  Note it is
 * legal for instructions that occur before an "acquire" barrier to be
 * moved down below it, and for instructions that occur after a "release"
 * barrier to be moved up above it.
 *
 * The ARM-driven implementation we use here is short on subtlety,
 * and actually requests a full barrier from the compiler and the CPU.
 * The only difference between acquire and release is in whether they
 * are issued before or after the atomic operation with which they
 * are associated.  To ease the transition to C/C++ atomic intrinsics,
 * you should not rely on this, and instead assume that only the minimal
 * acquire/release protection is provided.
 *
 * NOTE: all int32_t* values are expected to be aligned on 32-bit boundaries.
 * If they are not, atomicity is not guaranteed.
 */

/*
 * Basic arithmetic and bitwise operations.  These all provide a
 * barrier with "release" ordering, and return the previous value.
 *
 * These have the same characteristics (e.g. what happens on overflow)
 * as the equivalent non-atomic C operations.
 */
int32_t android_atomic_inc(volatile int32_t* addr);
int32_t android_atomic_dec(volatile int32_t* addr);
int32_t android_atomic_add(int32_t value, volatile int32_t* addr);
int32_t android_atomic_and(int32_t value, volatile int32_t* addr);
int32_t android_atomic_or(int32_t value, volatile int32_t* addr);

/*
 * Perform an atomic load with "acquire" or "release" ordering.
 *
 * Note that the notion of a "release" ordering for a load does not
 * really fit into the C11 or C++11 memory model.  The extra ordering
 * is normally observable only by code using memory_order_relaxed
 * atomics, or data races.  In the rare cases in which such ordering
 * is called for, use memory_order_relaxed atomics and a leading
 * atomic_thread_fence (typically with memory_order_acquire,
 * not memory_order_release!) instead.  If you do not understand
 * this comment, you are in the vast majority, and should not be
 * using release loads or replacing them with anything other than
 * locks or default sequentially consistent atomics.
 *
 * This is only necessary if you need the memory barrier.  A 32-bit read
 * from a 32-bit aligned address is atomic on all supported platforms.
 */
int32_t android_atomic_acquire_load(volatile const int32_t* addr);
int32_t android_atomic_release_load(volatile const int32_t* addr);

/*
 * Perform an atomic store with "acquire" or "release" ordering.
 *
 * Note that the notion of a "acquire" ordering for a store does not
 * really fit into the C11 or C++11 memory model.  The extra ordering
 * is normally observable only by code using memory_order_relaxed
 * atomics, or data races.  In the rare cases in which such ordering
 * is called for, use memory_order_relaxed atomics and a trailing
 * atomic_thread_fence (typically with memory_order_release,
 * not memory_order_acquire!) instead.
 *
 * This is only necessary if you need the memory barrier.  A 32-bit write
 * to a 32-bit aligned address is atomic on all supported platforms.
 */
void android_atomic_acquire_store(int32_t value, volatile int32_t* addr);
void android_atomic_release_store(int32_t value, volatile int32_t* addr);

/*
 * Compare-and-set operation with "acquire" or "release" ordering.
 *
 * This returns zero if the new value was successfully stored, which will
 * only happen when *addr == oldvalue.
 *
 * (The return value is inverted from implementations on other platforms,
 * but matches the ARM ldrex/strex result.)
 *
 * Implementations that use the release CAS in a loop may be less efficient
 * than possible, because we re-issue the memory barrier on each iteration.
 */
int android_atomic_acquire_cas(int32_t oldvalue, int32_t newvalue,
        volatile int32_t* addr);
int android_atomic_release_cas(int32_t oldvalue, int32_t newvalue,
        volatile int32_t* addr);

/*
 * Aliases for code using an older version of this header.  These are now
 * deprecated and should not be used.  The definitions will be removed
 * in a future release.
 */
#define android_atomic_write android_atomic_release_store
#define android_atomic_cmpxchg android_atomic_release_cas

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ANDROID_CUTILS_ATOMIC_H
