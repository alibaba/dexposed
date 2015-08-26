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
#include "object_callbacks.h"

namespace art {

template<class MirrorType>
class PACKED(4) GcRoot {
 public:
  template<ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  ALWAYS_INLINE MirrorType* Read() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  ALWAYS_INLINE void Assign(MirrorType* value) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void VisitRoot(RootCallback* callback, void* arg, uint32_t thread_id, RootType root_type) {
    callback(reinterpret_cast<mirror::Object**>(&root_), arg, thread_id, root_type);
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
  MirrorType* root_;
};

}  // namespace art

#endif  // ART_RUNTIME_GC_ROOT_H_
