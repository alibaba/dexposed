/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef ART_RUNTIME_BASE_ALLOCATOR_H_
#define ART_RUNTIME_BASE_ALLOCATOR_H_

#include <map>

#include "atomic.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "utils.h"

namespace art {

static constexpr bool kEnableTrackingAllocator = false;

class Allocator {
 public:
  static Allocator* GetMallocAllocator();
  static Allocator* GetNoopAllocator();

  Allocator() {}
  virtual ~Allocator() {}

  virtual void* Alloc(size_t) = 0;
  virtual void Free(void*) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(Allocator);
};

// Used by TrackedAllocators.
enum AllocatorTag {
  kAllocatorTagHeap,
  kAllocatorTagMonitorList,
  kAllocatorTagClassTable,
  kAllocatorTagInternTable,
  kAllocatorTagMaps,
  kAllocatorTagLOS,
  kAllocatorTagSafeMap,
  kAllocatorTagLOSMaps,
  kAllocatorTagReferenceTable,
  kAllocatorTagHeapBitmap,
  kAllocatorTagHeapBitmapLOS,
  kAllocatorTagMonitorPool,
  kAllocatorTagLOSFreeList,
  kAllocatorTagVerifier,
  kAllocatorTagRememberedSet,
  kAllocatorTagModUnionCardSet,
  kAllocatorTagModUnionReferenceArray,
  kAllocatorTagJNILibrarires,
  kAllocatorTagCompileTimeClassPath,
  kAllocatorTagOatFile,
  kAllocatorTagDexFileVerifier,
  kAllocatorTagCount,  // Must always be last element.
};
std::ostream& operator<<(std::ostream& os, const AllocatorTag& tag);

class TrackedAllocators {
 public:
  static bool Add(uint32_t tag, AtomicInteger* bytes_used);
  static void Dump(std::ostream& os);
  static void RegisterAllocation(AllocatorTag tag, uint64_t bytes) {
    total_bytes_used_[tag].FetchAndAddSequentiallyConsistent(bytes);
    uint64_t new_bytes = bytes_used_[tag].FetchAndAddSequentiallyConsistent(bytes) + bytes;
    max_bytes_used_[tag].StoreRelaxed(std::max(max_bytes_used_[tag].LoadRelaxed(), new_bytes));
  }
  static void RegisterFree(AllocatorTag tag, uint64_t bytes) {
    bytes_used_[tag].FetchAndSubSequentiallyConsistent(bytes);
  }

 private:
  static Atomic<uint64_t> bytes_used_[kAllocatorTagCount];
  static Atomic<uint64_t> max_bytes_used_[kAllocatorTagCount];
  static Atomic<uint64_t> total_bytes_used_[kAllocatorTagCount];
};

// Tracking allocator, tracks how much memory is used.
template<class T, AllocatorTag kTag>
class TrackingAllocatorImpl {
 public:
  typedef typename std::allocator<T>::value_type value_type;
  typedef typename std::allocator<T>::size_type size_type;
  typedef typename std::allocator<T>::difference_type difference_type;
  typedef typename std::allocator<T>::pointer pointer;
  typedef typename std::allocator<T>::const_pointer const_pointer;
  typedef typename std::allocator<T>::reference reference;
  typedef typename std::allocator<T>::const_reference const_reference;

  // Used internally by STL data structures.
  template <class U>
  TrackingAllocatorImpl(const TrackingAllocatorImpl<U, kTag>& alloc) throw() {
  }

  // Used internally by STL data structures.
  TrackingAllocatorImpl() throw() {
    COMPILE_ASSERT(kTag < kAllocatorTagCount, must_be_less_than_count);
  }

  // Enables an allocator for objects of one type to allocate storage for objects of another type.
  // Used internally by STL data structures.
  template <class U>
  struct rebind {
    typedef TrackingAllocatorImpl<U, kTag> other;
  };

  pointer allocate(size_type n, const_pointer hint = 0) {
    const size_t size = n * sizeof(T);
    TrackedAllocators::RegisterAllocation(GetTag(), size);
    return reinterpret_cast<pointer>(malloc(size));
  }

  template <typename PT>
  void deallocate(PT p, size_type n) {
    const size_t size = n * sizeof(T);
    TrackedAllocators::RegisterFree(GetTag(), size);
    free(p);
  }

  static AllocatorTag GetTag() {
    return kTag;
  }
};

template<class T, AllocatorTag kTag>
// C++ doesn't allow template typedefs. This is a workaround template typedef which is
// TrackingAllocatorImpl<T> if kEnableTrackingAllocator is true, std::allocator<T> otherwise.
class TrackingAllocator : public TypeStaticIf<kEnableTrackingAllocator,
                                              TrackingAllocatorImpl<T, kTag>,
                                              std::allocator<T>>::type {
};

template<class Key, class T, AllocatorTag kTag, class Compare = std::less<Key>>
class AllocationTrackingMultiMap : public std::multimap<
    Key, T, Compare, TrackingAllocator<std::pair<Key, T>, kTag>> {
};

}  // namespace art

#endif  // ART_RUNTIME_BASE_ALLOCATOR_H_
