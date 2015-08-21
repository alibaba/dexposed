/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ART_RUNTIME_GC_ACCOUNTING_ATOMIC_STACK_H_
#define ART_RUNTIME_GC_ACCOUNTING_ATOMIC_STACK_H_

#include <algorithm>
#include <memory>
#include <string>

#include "atomic.h"
#include "base/logging.h"
#include "base/macros.h"
#include "mem_map.h"
#include "utils.h"

namespace art {
namespace gc {
namespace accounting {

template <typename T>
class AtomicStack {
 public:
  // Capacity is how many elements we can store in the stack.
  static AtomicStack* Create(const std::string& name, size_t growth_limit, size_t capacity) {
    std::unique_ptr<AtomicStack> mark_stack(new AtomicStack(name, growth_limit, capacity));
    mark_stack->Init();
    return mark_stack.release();
  }

  ~AtomicStack() {}

  void Reset() {
    DCHECK(mem_map_.get() != nullptr);
    DCHECK(begin_ != NULL);
    front_index_.StoreRelaxed(0);
    back_index_.StoreRelaxed(0);
    debug_is_sorted_ = true;
    mem_map_->MadviseDontNeedAndZero();
  }

  // Beware: Mixing atomic pushes and atomic pops will cause ABA problem.

  // Returns false if we overflowed the stack.
  bool AtomicPushBackIgnoreGrowthLimit(const T& value) {
    return AtomicPushBackInternal(value, capacity_);
  }

  // Returns false if we overflowed the stack.
  bool AtomicPushBack(const T& value) {
    return AtomicPushBackInternal(value, growth_limit_);
  }

  // Atomically bump the back index by the given number of
  // slots. Returns false if we overflowed the stack.
  bool AtomicBumpBack(size_t num_slots, T** start_address, T** end_address) {
    if (kIsDebugBuild) {
      debug_is_sorted_ = false;
    }
    int32_t index;
    int32_t new_index;
    do {
      index = back_index_.LoadRelaxed();
      new_index = index + num_slots;
      if (UNLIKELY(static_cast<size_t>(new_index) >= growth_limit_)) {
        // Stack overflow.
        return false;
      }
    } while (!back_index_.CompareExchangeWeakRelaxed(index, new_index));
    *start_address = &begin_[index];
    *end_address = &begin_[new_index];
    if (kIsDebugBuild) {
      // Sanity check that the memory is zero.
      for (int32_t i = index; i < new_index; ++i) {
        DCHECK_EQ(begin_[i], static_cast<T>(0))
            << "i=" << i << " index=" << index << " new_index=" << new_index;
      }
    }
    return true;
  }

  void AssertAllZero() {
    if (kIsDebugBuild) {
      for (size_t i = 0; i < capacity_; ++i) {
        DCHECK_EQ(begin_[i], static_cast<T>(0)) << "i=" << i;
      }
    }
  }

  void PushBack(const T& value) {
    if (kIsDebugBuild) {
      debug_is_sorted_ = false;
    }
    int32_t index = back_index_.LoadRelaxed();
    DCHECK_LT(static_cast<size_t>(index), growth_limit_);
    back_index_.StoreRelaxed(index + 1);
    begin_[index] = value;
  }

  T PopBack() {
    DCHECK_GT(back_index_.LoadRelaxed(), front_index_.LoadRelaxed());
    // Decrement the back index non atomically.
    back_index_.StoreRelaxed(back_index_.LoadRelaxed() - 1);
    return begin_[back_index_.LoadRelaxed()];
  }

  // Take an item from the front of the stack.
  T PopFront() {
    int32_t index = front_index_.LoadRelaxed();
    DCHECK_LT(index, back_index_.LoadRelaxed());
    front_index_.StoreRelaxed(index + 1);
    return begin_[index];
  }

  // Pop a number of elements.
  void PopBackCount(int32_t n) {
    DCHECK_GE(Size(), static_cast<size_t>(n));
    back_index_.FetchAndSubSequentiallyConsistent(n);
  }

  bool IsEmpty() const {
    return Size() == 0;
  }

  size_t Size() const {
    DCHECK_LE(front_index_.LoadRelaxed(), back_index_.LoadRelaxed());
    return back_index_.LoadRelaxed() - front_index_.LoadRelaxed();
  }

  T* Begin() const {
    return const_cast<T*>(begin_ + front_index_.LoadRelaxed());
  }

  T* End() const {
    return const_cast<T*>(begin_ + back_index_.LoadRelaxed());
  }

  size_t Capacity() const {
    return capacity_;
  }

  // Will clear the stack.
  void Resize(size_t new_capacity) {
    capacity_ = new_capacity;
    growth_limit_ = new_capacity;
    Init();
  }

  void Sort() {
    int32_t start_back_index = back_index_.LoadRelaxed();
    int32_t start_front_index = front_index_.LoadRelaxed();
    std::sort(Begin(), End());
    CHECK_EQ(start_back_index, back_index_.LoadRelaxed());
    CHECK_EQ(start_front_index, front_index_.LoadRelaxed());
    if (kIsDebugBuild) {
      debug_is_sorted_ = true;
    }
  }

  bool ContainsSorted(const T& value) const {
    DCHECK(debug_is_sorted_);
    return std::binary_search(Begin(), End(), value);
  }

  bool Contains(const T& value) const {
    return std::find(Begin(), End(), value) != End();
  }

 private:
  AtomicStack(const std::string& name, size_t growth_limit, size_t capacity)
      : name_(name),
        back_index_(0),
        front_index_(0),
        begin_(nullptr),
        growth_limit_(growth_limit),
        capacity_(capacity),
        debug_is_sorted_(true) {
  }

  // Returns false if we overflowed the stack.
  bool AtomicPushBackInternal(const T& value, size_t limit) ALWAYS_INLINE {
    if (kIsDebugBuild) {
      debug_is_sorted_ = false;
    }
    int32_t index;
    do {
      index = back_index_.LoadRelaxed();
      if (UNLIKELY(static_cast<size_t>(index) >= limit)) {
        // Stack overflow.
        return false;
      }
    } while (!back_index_.CompareExchangeWeakRelaxed(index, index + 1));
    begin_[index] = value;
    return true;
  }

  // Size in number of elements.
  void Init() {
    std::string error_msg;
    mem_map_.reset(MemMap::MapAnonymous(name_.c_str(), NULL, capacity_ * sizeof(T),
                                        PROT_READ | PROT_WRITE, false, &error_msg));
    CHECK(mem_map_.get() != NULL) << "couldn't allocate mark stack.\n" << error_msg;
    byte* addr = mem_map_->Begin();
    CHECK(addr != NULL);
    debug_is_sorted_ = true;
    begin_ = reinterpret_cast<T*>(addr);
    Reset();
  }

  // Name of the mark stack.
  std::string name_;
  // Memory mapping of the atomic stack.
  std::unique_ptr<MemMap> mem_map_;
  // Back index (index after the last element pushed).
  AtomicInteger back_index_;
  // Front index, used for implementing PopFront.
  AtomicInteger front_index_;
  // Base of the atomic stack.
  T* begin_;
  // Current maximum which we can push back to, must be <= capacity_.
  size_t growth_limit_;
  // Maximum number of elements.
  size_t capacity_;
  // Whether or not the stack is sorted, only updated in debug mode to avoid performance overhead.
  bool debug_is_sorted_;

  DISALLOW_COPY_AND_ASSIGN(AtomicStack);
};

typedef AtomicStack<mirror::Object*> ObjectStack;

}  // namespace accounting
}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_ACCOUNTING_ATOMIC_STACK_H_
