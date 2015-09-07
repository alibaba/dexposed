/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef ART_RUNTIME_BASE_HASH_SET_H_
#define ART_RUNTIME_BASE_HASH_SET_H_

#include <functional>
#include <memory>
#include <stdint.h>
#include <utility>

#include "logging.h"

namespace art {

// Returns true if an item is empty.
template <class T>
class DefaultEmptyFn {
 public:
  void MakeEmpty(T& item) const {
    item = T();
  }
  bool IsEmpty(const T& item) const {
    return item == T();
  }
};

template <class T>
class DefaultEmptyFn<T*> {
 public:
  void MakeEmpty(T*& item) const {
    item = nullptr;
  }
  bool IsEmpty(const T*& item) const {
    return item == nullptr;
  }
};

// Low memory version of a hash set, uses less memory than std::unordered_set since elements aren't
// boxed. Uses linear probing.
// EmptyFn needs to implement two functions MakeEmpty(T& item) and IsEmpty(const T& item)
template <class T, class EmptyFn = DefaultEmptyFn<T>, class HashFn = std::hash<T>,
    class Pred = std::equal_to<T>, class Alloc = std::allocator<T>>
class HashSet {
 public:
  static constexpr double kDefaultMinLoadFactor = 0.5;
  static constexpr double kDefaultMaxLoadFactor = 0.9;
  static constexpr size_t kMinBuckets = 1000;

  class Iterator {
   public:
    Iterator(const Iterator&) = default;
    Iterator(HashSet* hash_set, size_t index) : hash_set_(hash_set), index_(index) {
    }
    Iterator& operator=(const Iterator&) = default;
    bool operator==(const Iterator& other) const {
      return hash_set_ == other.hash_set_ && index_ == other.index_;
    }
    bool operator!=(const Iterator& other) const {
      return !(*this == other);
    }
    Iterator operator++() {  // Value after modification.
      index_ = NextNonEmptySlot(index_);
      return *this;
    }
    Iterator operator++(int) {
      Iterator temp = *this;
      index_ = NextNonEmptySlot(index_);
      return temp;
    }
    T& operator*() {
      DCHECK(!hash_set_->IsFreeSlot(GetIndex()));
      return hash_set_->ElementForIndex(index_);
    }
    const T& operator*() const {
      DCHECK(!hash_set_->IsFreeSlot(GetIndex()));
      return hash_set_->ElementForIndex(index_);
    }
    T* operator->() {
      return &**this;
    }
    const T* operator->() const {
      return &**this;
    }
    // TODO: Operator -- --(int)

   private:
    HashSet* hash_set_;
    size_t index_;

    size_t GetIndex() const {
      return index_;
    }
    size_t NextNonEmptySlot(size_t index) const {
      const size_t num_buckets = hash_set_->NumBuckets();
      DCHECK_LT(index, num_buckets);
      do {
        ++index;
      } while (index < num_buckets && hash_set_->IsFreeSlot(index));
      return index;
    }

    friend class HashSet;
  };

  void Clear() {
    DeallocateStorage();
    AllocateStorage(1);
    num_elements_ = 0;
    elements_until_expand_ = 0;
  }
  HashSet() : num_elements_(0), num_buckets_(0), data_(nullptr),
      min_load_factor_(kDefaultMinLoadFactor), max_load_factor_(kDefaultMaxLoadFactor) {
    Clear();
  }
  HashSet(const HashSet& other) : num_elements_(0), num_buckets_(0), data_(nullptr) {
    *this = other;
  }
  HashSet(HashSet&& other) : num_elements_(0), num_buckets_(0), data_(nullptr) {
    *this = std::move(other);
  }
  ~HashSet() {
    DeallocateStorage();
  }
  HashSet& operator=(HashSet&& other) {
    std::swap(data_, other.data_);
    std::swap(num_buckets_, other.num_buckets_);
    std::swap(num_elements_, other.num_elements_);
    std::swap(elements_until_expand_, other.elements_until_expand_);
    std::swap(min_load_factor_, other.min_load_factor_);
    std::swap(max_load_factor_, other.max_load_factor_);
    return *this;
  }
  HashSet& operator=(const HashSet& other) {
    DeallocateStorage();
    AllocateStorage(other.NumBuckets());
    for (size_t i = 0; i < num_buckets_; ++i) {
      ElementForIndex(i) = other.data_[i];
    }
    num_elements_ = other.num_elements_;
    elements_until_expand_ = other.elements_until_expand_;
    min_load_factor_ = other.min_load_factor_;
    max_load_factor_ = other.max_load_factor_;
    return *this;
  }
  // Lower case for c++11 for each.
  Iterator begin() {
    Iterator ret(this, 0);
    if (num_buckets_ != 0 && IsFreeSlot(ret.GetIndex())) {
      ++ret;  // Skip all the empty slots.
    }
    return ret;
  }
  // Lower case for c++11 for each.
  Iterator end() {
    return Iterator(this, NumBuckets());
  }
  bool Empty() {
    return begin() == end();
  }
  // Erase algorithm:
  // Make an empty slot where the iterator is pointing.
  // Scan fowards until we hit another empty slot.
  // If an element inbetween doesn't rehash to the range from the current empty slot to the
  // iterator. It must be before the empty slot, in that case we can move it to the empty slot
  // and set the empty slot to be the location we just moved from.
  // Relies on maintaining the invariant that there's no empty slots from the 'ideal' index of an
  // element to its actual location/index.
  Iterator Erase(Iterator it) {
    // empty_index is the index that will become empty.
    size_t empty_index = it.GetIndex();
    DCHECK(!IsFreeSlot(empty_index));
    size_t next_index = empty_index;
    bool filled = false;  // True if we filled the empty index.
    while (true) {
      next_index = NextIndex(next_index);
      T& next_element = ElementForIndex(next_index);
      // If the next element is empty, we are done. Make sure to clear the current empty index.
      if (emptyfn_.IsEmpty(next_element)) {
        emptyfn_.MakeEmpty(ElementForIndex(empty_index));
        break;
      }
      // Otherwise try to see if the next element can fill the current empty index.
      const size_t next_hash = hashfn_(next_element);
      // Calculate the ideal index, if it is within empty_index + 1 to next_index then there is
      // nothing we can do.
      size_t next_ideal_index = IndexForHash(next_hash);
      // Loop around if needed for our check.
      size_t unwrapped_next_index = next_index;
      if (unwrapped_next_index < empty_index) {
        unwrapped_next_index += NumBuckets();
      }
      // Loop around if needed for our check.
      size_t unwrapped_next_ideal_index = next_ideal_index;
      if (unwrapped_next_ideal_index < empty_index) {
        unwrapped_next_ideal_index += NumBuckets();
      }
      if (unwrapped_next_ideal_index <= empty_index ||
          unwrapped_next_ideal_index > unwrapped_next_index) {
        // If the target index isn't within our current range it must have been probed from before
        // the empty index.
        ElementForIndex(empty_index) = std::move(next_element);
        filled = true;  // TODO: Optimize
        empty_index = next_index;
      }
    }
    --num_elements_;
    // If we didn't fill the slot then we need go to the next non free slot.
    if (!filled) {
      ++it;
    }
    return it;
  }
  // Find an element, returns end() if not found.
  // Allows custom K types, example of when this is useful.
  // Set of Class* sorted by name, want to find a class with a name but can't allocate a dummy
  // object in the heap for performance solution.
  template <typename K>
  Iterator Find(const K& element) {
    return FindWithHash(element, hashfn_(element));
  }
  template <typename K>
  Iterator FindWithHash(const K& element, size_t hash) {
    DCHECK_EQ(hashfn_(element), hash);
    size_t index = IndexForHash(hash);
    while (true) {
      T& slot = ElementForIndex(index);
      if (emptyfn_.IsEmpty(slot)) {
        return end();
      }
      if (pred_(slot, element)) {
        return Iterator(this, index);
      }
      index = NextIndex(index);
    }
  }
  // Insert an element, allows duplicates.
  void Insert(const T& element) {
    InsertWithHash(element, hashfn_(element));
  }
  void InsertWithHash(const T& element, size_t hash) {
    DCHECK_EQ(hash, hashfn_(element));
    if (num_elements_ >= elements_until_expand_) {
      Expand();
      DCHECK_LT(num_elements_, elements_until_expand_);
    }
    const size_t index = FirstAvailableSlot(IndexForHash(hash));
    data_[index] = element;
    ++num_elements_;
  }
  size_t Size() const {
    return num_elements_;
  }
  void ShrinkToMaximumLoad() {
    Resize(Size() / max_load_factor_);
  }
  // To distance that inserted elements were probed. Used for measuring how good hash functions
  // are.
  size_t TotalProbeDistance() const {
    size_t total = 0;
    for (size_t i = 0; i < NumBuckets(); ++i) {
      const T& element = ElementForIndex(i);
      if (!emptyfn_.IsEmpty(element)) {
        size_t ideal_location = IndexForHash(hashfn_(element));
        if (ideal_location > i) {
          total += i + NumBuckets() - ideal_location;
        } else {
          total += i - ideal_location;
        }
      }
    }
    return total;
  }
  // Calculate the current load factor and return it.
  double CalculateLoadFactor() const {
    return static_cast<double>(Size()) / static_cast<double>(NumBuckets());
  }
  // Make sure that everything reinserts in the right spot. Returns the number of errors.
  size_t Verify() {
    size_t errors = 0;
    for (size_t i = 0; i < num_buckets_; ++i) {
      T& element = data_[i];
      if (!emptyfn_.IsEmpty(element)) {
        T temp;
        emptyfn_.MakeEmpty(temp);
        std::swap(temp, element);
        size_t first_slot = FirstAvailableSlot(IndexForHash(hashfn_(temp)));
        if (i != first_slot) {
          LOG(ERROR) << "Element " << i << " should be in slot " << first_slot;
          ++errors;
        }
        std::swap(temp, element);
      }
    }
    return errors;
  }

 private:
  T& ElementForIndex(size_t index) {
    DCHECK_LT(index, NumBuckets());
    DCHECK(data_ != nullptr);
    return data_[index];
  }
  const T& ElementForIndex(size_t index) const {
    DCHECK_LT(index, NumBuckets());
    DCHECK(data_ != nullptr);
    return data_[index];
  }
  size_t IndexForHash(size_t hash) const {
    return hash % num_buckets_;
  }
  size_t NextIndex(size_t index) const {
    if (UNLIKELY(++index >= num_buckets_)) {
      DCHECK_EQ(index, NumBuckets());
      return 0;
    }
    return index;
  }
  bool IsFreeSlot(size_t index) const {
    return emptyfn_.IsEmpty(ElementForIndex(index));
  }
  size_t NumBuckets() const {
    return num_buckets_;
  }
  // Allocate a number of buckets.
  void AllocateStorage(size_t num_buckets) {
    num_buckets_ = num_buckets;
    data_ = allocfn_.allocate(num_buckets_);
    for (size_t i = 0; i < num_buckets_; ++i) {
      allocfn_.construct(allocfn_.address(data_[i]));
      emptyfn_.MakeEmpty(data_[i]);
    }
  }
  void DeallocateStorage() {
    if (num_buckets_ != 0) {
      for (size_t i = 0; i < NumBuckets(); ++i) {
        allocfn_.destroy(allocfn_.address(data_[i]));
      }
      allocfn_.deallocate(data_, NumBuckets());
      data_ = nullptr;
      num_buckets_ = 0;
    }
  }
  // Expand the set based on the load factors.
  void Expand() {
    size_t min_index = static_cast<size_t>(Size() / min_load_factor_);
    if (min_index < kMinBuckets) {
      min_index = kMinBuckets;
    }
    // Resize based on the minimum load factor.
    Resize(min_index);
    // When we hit elements_until_expand_, we are at the max load factor and must expand again.
    elements_until_expand_ = NumBuckets() * max_load_factor_;
  }
  // Expand / shrink the table to the new specified size.
  void Resize(size_t new_size) {
    DCHECK_GE(new_size, Size());
    T* old_data = data_;
    size_t old_num_buckets = num_buckets_;
    // Reinsert all of the old elements.
    AllocateStorage(new_size);
    for (size_t i = 0; i < old_num_buckets; ++i) {
      T& element = old_data[i];
      if (!emptyfn_.IsEmpty(element)) {
        data_[FirstAvailableSlot(IndexForHash(hashfn_(element)))] = std::move(element);
      }
      allocfn_.destroy(allocfn_.address(element));
    }
    allocfn_.deallocate(old_data, old_num_buckets);
  }
  ALWAYS_INLINE size_t FirstAvailableSlot(size_t index) const {
    while (!emptyfn_.IsEmpty(data_[index])) {
      index = NextIndex(index);
    }
    return index;
  }

  Alloc allocfn_;  // Allocator function.
  HashFn hashfn_;  // Hashing function.
  EmptyFn emptyfn_;  // IsEmpty/SetEmpty function.
  Pred pred_;  // Equals function.
  size_t num_elements_;  // Number of inserted elements.
  size_t num_buckets_;  // Number of hash table buckets.
  size_t elements_until_expand_;  // Maxmimum number of elements until we expand the table.
  T* data_;  // Backing storage.
  double min_load_factor_;
  double max_load_factor_;

  friend class Iterator;
};

}  // namespace art

#endif  // ART_RUNTIME_BASE_HASH_SET_H_
