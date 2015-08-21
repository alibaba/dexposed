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

#ifndef ART_RUNTIME_REFERENCE_TABLE_H_
#define ART_RUNTIME_REFERENCE_TABLE_H_

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include "base/allocator.h"
#include "base/mutex.h"
#include "gc_root.h"
#include "object_callbacks.h"

namespace art {
namespace mirror {
class Object;
}  // namespace mirror

// Maintain a table of references.  Used for JNI monitor references and
// JNI pinned array references.
//
// None of the functions are synchronized.
class ReferenceTable {
 public:
  ReferenceTable(const char* name, size_t initial_size, size_t max_size);
  ~ReferenceTable();

  void Add(mirror::Object* obj) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void Remove(mirror::Object* obj) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  size_t Size() const;

  void Dump(std::ostream& os) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void VisitRoots(RootCallback* visitor, void* arg, uint32_t tid, RootType root_type);

 private:
  typedef std::vector<GcRoot<mirror::Object>,
                      TrackingAllocator<GcRoot<mirror::Object>, kAllocatorTagReferenceTable>> Table;
  static void Dump(std::ostream& os, Table& entries)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  friend class IndirectReferenceTable;  // For Dump.

  std::string name_;
  Table entries_;
  size_t max_size_;
};

}  // namespace art

#endif  // ART_RUNTIME_REFERENCE_TABLE_H_
