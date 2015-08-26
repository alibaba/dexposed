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

#ifndef ART_RUNTIME_GC_ROOT_H_
#define ART_RUNTIME_GC_ROOT_H_

#include "base/macros.h"
#include "base/mutex.h"       // For Locks::mutator_lock_.

namespace art {

namespace mirror {
class Object;
}  // namespace mirror

enum RootType {
  kRootUnknown = 0,
  kRootJNIGlobal,
  kRootJNILocal,
  kRootJavaFrame,
  kRootNativeStack,
  kRootStickyClass,
  kRootThreadBlock,
  kRootMonitorUsed,
  kRootThreadObject,
  kRootInternedString,
  kRootDebugger,
  kRootVMInternal,
  kRootJNIMonitor,
};
std::ostream& operator<<(std::ostream& os, const RootType& root_type);

class RootInfo {
 public:
  // Thread id 0 is for non thread roots.
  explicit RootInfo(RootType type, uint32_t thread_id = 0)
     : type_(type), thread_id_(thread_id) {
  }
  virtual ~RootInfo() {
  }
  RootType GetType() const {
    return type_;
  }
  uint32_t GetThreadId() const {
    return thread_id_;
  }
  virtual void Describe(std::ostream& os) const {
    os << "Type=" << type_ << " thread_id=" << thread_id_;
  }

 private:
  const RootType type_;
  const uint32_t thread_id_;
};

// Returns the new address of the object, returns root if it has not moved. tid and root_type are
// only used by hprof.
typedef void (RootCallback)(mirror::Object** root, void* arg, const RootInfo& root_info);

template<class MirrorType>
class PACKED(4) GcRoot {
 public:
  template<ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  ALWAYS_INLINE MirrorType* Read() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void VisitRoot(RootCallback* callback, void* arg, const RootInfo& info) const {
    DCHECK(!IsNull());
    callback(reinterpret_cast<mirror::Object**>(&root_), arg, info);
    DCHECK(!IsNull());
  }

  void VisitRootIfNonNull(RootCallback* callback, void* arg, const RootInfo& info) const {
    if (!IsNull()) {
      VisitRoot(callback, arg, info);
    }
  }

  // This is only used by IrtIterator.
  ALWAYS_INLINE MirrorType** AddressWithoutBarrier() {
    return &root_;
  }

  bool IsNull() const {
    // It's safe to null-check it without a read barrier.
    return root_ == nullptr;
  }

  ALWAYS_INLINE explicit GcRoot<MirrorType>() : root_(nullptr) {
  }

  ALWAYS_INLINE explicit GcRoot<MirrorType>(MirrorType* ref)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) : root_(ref) {
  }

 private:
  mutable MirrorType* root_;
};

}  // namespace art

#endif  // ART_RUNTIME_GC_ROOT_H_
