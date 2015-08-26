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

#ifndef ART_RUNTIME_GC_ACCOUNTING_CARD_TABLE_H_
#define ART_RUNTIME_GC_ACCOUNTING_CARD_TABLE_H_

#include <memory>

#include "base/mutex.h"
#include "globals.h"
#include "mem_map.h"

namespace art {

namespace mirror {
  class Object;
}  // namespace mirror

namespace gc {

namespace space {
  class ContinuousSpace;
}  // namespace space

class Heap;

namespace accounting {

template<size_t kAlignment> class SpaceBitmap;

// Maintain a card table from the the write barrier. All writes of
// non-NULL values to heap addresses should go through an entry in
// WriteBarrier, and from there to here.
class CardTable {
 public:
  static constexpr size_t kCardShift = 7;
  static constexpr size_t kCardSize = 1 << kCardShift;
  static constexpr uint8_t kCardClean = 0x0;
  static constexpr uint8_t kCardDirty = 0x70;

  static CardTable* Create(const byte* heap_begin, size_t heap_capacity);

  // Set the card associated with the given address to GC_CARD_DIRTY.
  void MarkCard(const void *addr) {
    byte* card_addr = CardFromAddr(addr);
    *card_addr = kCardDirty;
  }

  // Is the object on a dirty card?
  bool IsDirty(const mirror::Object* obj) const {
    return GetCard(obj) == kCardDirty;
  }

  // Return the state of the card at an address.
  byte GetCard(const mirror::Object* obj) const {
    return *CardFromAddr(obj);
  }

  // Visit and clear cards within memory range, only visits dirty cards.
  template <typename Visitor>
  void VisitClear(const void* start, const void* end, const Visitor& visitor) {
    byte* card_start = CardFromAddr(start);
    byte* card_end = CardFromAddr(end);
    for (byte* it = card_start; it != card_end; ++it) {
      if (*it == kCardDirty) {
        *it = kCardClean;
        visitor(it);
      }
    }
  }

  // Returns a value that when added to a heap address >> GC_CARD_SHIFT will address the appropriate
  // card table byte. For convenience this value is cached in every Thread
  byte* GetBiasedBegin() const {
    return biased_begin_;
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
  void ModifyCardsAtomic(byte* scan_begin, byte* scan_end, const Visitor& visitor,
                         const ModifiedVisitor& modified);

  // For every dirty at least minumum age between begin and end invoke the visitor with the
  // specified argument. Returns how many cards the visitor was run on.
  template <typename Visitor>
  size_t Scan(SpaceBitmap<kObjectAlignment>* bitmap, byte* scan_begin, byte* scan_end,
              const Visitor& visitor,
              const byte minimum_age = kCardDirty) const
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Assertion used to check the given address is covered by the card table
  void CheckAddrIsInCardTable(const byte* addr) const;

  // Resets all of the bytes in the card table to clean.
  void ClearCardTable();

  // Resets all of the bytes in the card table which do not map to the image space.
  void ClearSpaceCards(space::ContinuousSpace* space);

  // Returns the first address in the heap which maps to this card.
  void* AddrFromCard(const byte *card_addr) const ALWAYS_INLINE;

  // Returns the address of the relevant byte in the card table, given an address on the heap.
  byte* CardFromAddr(const void *addr) const ALWAYS_INLINE;

  bool AddrIsInCardTable(const void* addr) const;

 private:
  CardTable(MemMap* begin, byte* biased_begin, size_t offset);

  // Returns true iff the card table address is within the bounds of the card table.
  bool IsValidCard(const byte* card_addr) const {
    byte* begin = mem_map_->Begin() + offset_;
    byte* end = mem_map_->End();
    return card_addr >= begin && card_addr < end;
  }

  void CheckCardValid(byte* card) const ALWAYS_INLINE;

  // Verifies that all gray objects are on a dirty card.
  void VerifyCardTable();

  // Mmapped pages for the card table
  std::unique_ptr<MemMap> mem_map_;
  // Value used to compute card table addresses from object addresses, see GetBiasedBegin
  byte* const biased_begin_;
  // Card table doesn't begin at the beginning of the mem_map_, instead it is displaced by offset
  // to allow the byte value of biased_begin_ to equal GC_CARD_DIRTY
  const size_t offset_;
};

}  // namespace accounting
}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_ACCOUNTING_CARD_TABLE_H_
