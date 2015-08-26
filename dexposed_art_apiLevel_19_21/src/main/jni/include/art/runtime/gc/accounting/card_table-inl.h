/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef ART_RUNTIME_GC_ACCOUNTING_CARD_TABLE_INL_H_
#define ART_RUNTIME_GC_ACCOUNTING_CARD_TABLE_INL_H_

#include "atomic.h"
#include "base/logging.h"
#include "card_table.h"
#include "space_bitmap.h"
#include "utils.h"

namespace art {
namespace gc {
namespace accounting {

static inline bool byte_cas(byte old_value, byte new_value, byte* address) {
#if defined(__i386__) || defined(__x86_64__)
  Atomic<byte>* byte_atomic = reinterpret_cast<Atomic<byte>*>(address);
  return byte_atomic->CompareExchangeWeakRelaxed(old_value, new_value);
#else
  // Little endian means most significant byte is on the left.
  const size_t shift_in_bytes = reinterpret_cast<uintptr_t>(address) % sizeof(uintptr_t);
  // Align the address down.
  address -= shift_in_bytes;
  const size_t shift_in_bits = shift_in_bytes * kBitsPerByte;
  Atomic<uintptr_t>* word_atomic = reinterpret_cast<Atomic<uintptr_t>*>(address);

  // Word with the byte we are trying to cas cleared.
  const uintptr_t cur_word = word_atomic->LoadRelaxed() &
      ~(static_cast<uintptr_t>(0xFF) << shift_in_bits);
  const uintptr_t old_word = cur_word | (static_cast<uintptr_t>(old_value) << shift_in_bits);
  const uintptr_t new_word = cur_word | (static_cast<uintptr_t>(new_value) << shift_in_bits);
  return word_atomic->CompareExchangeWeakRelaxed(old_word, new_word);
#endif
}

template <typename Visitor>
inline size_t CardTable::Scan(ContinuousSpaceBitmap* bitmap, byte* scan_begin, byte* scan_end,
                              const Visitor& visitor, const byte minimum_age) const {
  DCHECK_GE(scan_begin, reinterpret_cast<byte*>(bitmap->HeapBegin()));
  // scan_end is the byte after the last byte we scan.
  DCHECK_LE(scan_end, reinterpret_cast<byte*>(bitmap->HeapLimit()));
  byte* card_cur = CardFromAddr(scan_begin);
  byte* card_end = CardFromAddr(AlignUp(scan_end, kCardSize));
  CheckCardValid(card_cur);
  CheckCardValid(card_end);
  size_t cards_scanned = 0;

  // Handle any unaligned cards at the start.
  while (!IsAligned<sizeof(word)>(card_cur) && card_cur < card_end) {
    if (*card_cur >= minimum_age) {
      uintptr_t start = reinterpret_cast<uintptr_t>(AddrFromCard(card_cur));
      bitmap->VisitMarkedRange(start, start + kCardSize, visitor);
      ++cards_scanned;
    }
    ++card_cur;
  }

  byte* aligned_end = card_end -
      (reinterpret_cast<uintptr_t>(card_end) & (sizeof(uintptr_t) - 1));

  uintptr_t* word_end = reinterpret_cast<uintptr_t*>(aligned_end);
  for (uintptr_t* word_cur = reinterpret_cast<uintptr_t*>(card_cur); word_cur < word_end;
      ++word_cur) {
    while (LIKELY(*word_cur == 0)) {
      ++word_cur;
      if (UNLIKELY(word_cur >= word_end)) {
        goto exit_for;
      }
    }

    // Find the first dirty card.
    uintptr_t start_word = *word_cur;
    uintptr_t start = reinterpret_cast<uintptr_t>(AddrFromCard(reinterpret_cast<byte*>(word_cur)));
    // TODO: Investigate if processing continuous runs of dirty cards with a single bitmap visit is
    // more efficient.
    for (size_t i = 0; i < sizeof(uintptr_t); ++i) {
      if (static_cast<byte>(start_word) >= minimum_age) {
        auto* card = reinterpret_cast<byte*>(word_cur) + i;
        DCHECK(*card == static_cast<byte>(start_word) || *card == kCardDirty)
            << "card " << static_cast<size_t>(*card) << " word " << (start_word & 0xFF);
        bitmap->VisitMarkedRange(start, start + kCardSize, visitor);
        ++cards_scanned;
      }
      start_word >>= 8;
      start += kCardSize;
    }
  }
  exit_for:

  // Handle any unaligned cards at the end.
  card_cur = reinterpret_cast<byte*>(word_end);
  while (card_cur < card_end) {
    if (*card_cur >= minimum_age) {
      uintptr_t start = reinterpret_cast<uintptr_t>(AddrFromCard(card_cur));
      bitmap->VisitMarkedRange(start, start + kCardSize, visitor);
      ++cards_scanned;
    }
    ++card_cur;
  }

  return cards_scanned;
}

/*
 * Visitor is expected to take in a card and return the new value. When a value is modified, the
 * modify visitor is called.
 * visitor: The visitor which modifies the cards. Returns the new value for a card given an old
 * value.
 * modified: Whenever the visitor modifies a card, this visitor is called on the card. Enables
 * us to know which cards got cleared.
 */
template <typename Visitor, typename ModifiedVisitor>
inline void CardTable::ModifyCardsAtomic(byte* scan_begin, byte* scan_end, const Visitor& visitor,
                                         const ModifiedVisitor& modified) {
  byte* card_cur = CardFromAddr(scan_begin);
  byte* card_end = CardFromAddr(AlignUp(scan_end, kCardSize));
  CheckCardValid(card_cur);
  CheckCardValid(card_end);

  // Handle any unaligned cards at the start.
  while (!IsAligned<sizeof(word)>(card_cur) && card_cur < card_end) {
    byte expected, new_value;
    do {
      expected = *card_cur;
      new_value = visitor(expected);
    } while (expected != new_value && UNLIKELY(!byte_cas(expected, new_value, card_cur)));
    if (expected != new_value) {
      modified(card_cur, expected, new_value);
    }
    ++card_cur;
  }

  // Handle unaligned cards at the end.
  while (!IsAligned<sizeof(word)>(card_end) && card_end > card_cur) {
    --card_end;
    byte expected, new_value;
    do {
      expected = *card_end;
      new_value = visitor(expected);
    } while (expected != new_value && UNLIKELY(!byte_cas(expected, new_value, card_end)));
    if (expected != new_value) {
      modified(card_end, expected, new_value);
    }
  }

  // Now we have the words, we can process words in parallel.
  uintptr_t* word_cur = reinterpret_cast<uintptr_t*>(card_cur);
  uintptr_t* word_end = reinterpret_cast<uintptr_t*>(card_end);
  // TODO: This is not big endian safe.
  union {
    uintptr_t expected_word;
    uint8_t expected_bytes[sizeof(uintptr_t)];
  };
  union {
    uintptr_t new_word;
    uint8_t new_bytes[sizeof(uintptr_t)];
  };

  // TODO: Parallelize.
  while (word_cur < word_end) {
    while (true) {
      expected_word = *word_cur;
      if (LIKELY(expected_word == 0)) {
        break;
      }
      for (size_t i = 0; i < sizeof(uintptr_t); ++i) {
        new_bytes[i] = visitor(expected_bytes[i]);
      }
      Atomic<uintptr_t>* atomic_word = reinterpret_cast<Atomic<uintptr_t>*>(word_cur);
      if (LIKELY(atomic_word->CompareExchangeWeakRelaxed(expected_word, new_word))) {
        for (size_t i = 0; i < sizeof(uintptr_t); ++i) {
          const byte expected_byte = expected_bytes[i];
          const byte new_byte = new_bytes[i];
          if (expected_byte != new_byte) {
            modified(reinterpret_cast<byte*>(word_cur) + i, expected_byte, new_byte);
          }
        }
        break;
      }
    }
    ++word_cur;
  }
}

inline void* CardTable::AddrFromCard(const byte *card_addr) const {
  DCHECK(IsValidCard(card_addr))
    << " card_addr: " << reinterpret_cast<const void*>(card_addr)
    << " begin: " << reinterpret_cast<void*>(mem_map_->Begin() + offset_)
    << " end: " << reinterpret_cast<void*>(mem_map_->End());
  uintptr_t offset = card_addr - biased_begin_;
  return reinterpret_cast<void*>(offset << kCardShift);
}

inline byte* CardTable::CardFromAddr(const void *addr) const {
  byte *card_addr = biased_begin_ + (reinterpret_cast<uintptr_t>(addr) >> kCardShift);
  // Sanity check the caller was asking for address covered by the card table
  DCHECK(IsValidCard(card_addr)) << "addr: " << addr
      << " card_addr: " << reinterpret_cast<void*>(card_addr);
  return card_addr;
}

inline void CardTable::CheckCardValid(byte* card) const {
  DCHECK(IsValidCard(card))
      << " card_addr: " << reinterpret_cast<const void*>(card)
      << " begin: " << reinterpret_cast<void*>(mem_map_->Begin() + offset_)
      << " end: " << reinterpret_cast<void*>(mem_map_->End());
}

}  // namespace accounting
}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_ACCOUNTING_CARD_TABLE_INL_H_
