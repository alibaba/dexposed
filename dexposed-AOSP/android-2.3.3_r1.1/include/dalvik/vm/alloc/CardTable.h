/*
 * Copyright (C) 2010 The Android Open Source Project
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
 * Maintain a card table from the the write barrier. All writes of
 * non-NULL values to heap addresses should go through an entry in
 * WriteBarrier, and from there to here.
 */

#ifndef DALVIK_ALLOC_CARDTABLE_H_
#define DALVIK_ALLOC_CARDTABLE_H_

#define GC_CARD_SHIFT 7
#define GC_CARD_SIZE (1 << GC_CARD_SHIFT)
#define GC_CARD_CLEAN 0
#define GC_CARD_DIRTY 0x70

/*
 * Initializes the card table; must be called before any other
 * dvmCardTable*() functions.
 */
bool dvmCardTableStartup(size_t heapMaximumSize, size_t growthLimit);

/*
 * Tears down the entire CardTable structure.
 */
void dvmCardTableShutdown(void);

/*
 * Resets all of the bytes in the card table to clean.
 */
void dvmClearCardTable(void);

/*
 * Returns the address of the relevent byte in the card table, given
 * an address on the heap.
 */
u1 *dvmCardFromAddr(const void *addr);

/*
 * Returns the first address in the heap which maps to this card.
 */
void *dvmAddrFromCard(const u1 *card);

/*
 * Returns true if addr points to a valid card.
 */
bool dvmIsValidCard(const u1 *card);

/*
 * Set the card associated with the given address to GC_CARD_DIRTY.
 */
void dvmMarkCard(const void *addr);

/*
 * Verifies that all gray objects are on a dirty card.
 */
void dvmVerifyCardTable(void);

#endif  // DALVIK_ALLOC_CARDTABLE_H_
