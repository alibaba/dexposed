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

#ifndef ART_RUNTIME_OBJECT_CALLBACKS_H_
#define ART_RUNTIME_OBJECT_CALLBACKS_H_

// For ostream.
#include <ostream>
// For uint32_t.
#include <stdint.h>
// For size_t.
#include <stdlib.h>

#include "base/macros.h"

namespace art {
namespace mirror {
  class Class;
  class Object;
  template<class MirrorType> class HeapReference;
  class Reference;
}  // namespace mirror
class StackVisitor;

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

// Returns the new address of the object, returns root if it has not moved. tid and root_type are
// only used by hprof.
typedef void (RootCallback)(mirror::Object** root, void* arg, uint32_t thread_id,
    RootType root_type);
// A callback for visiting an object in the heap.
typedef void (ObjectCallback)(mirror::Object* obj, void* arg);
// A callback used for marking an object, returns the new address of the object if the object moved.
typedef mirror::Object* (MarkObjectCallback)(mirror::Object* obj, void* arg) WARN_UNUSED;
// A callback for verifying roots.
typedef void (VerifyRootCallback)(const mirror::Object* root, void* arg, size_t vreg,
    const StackVisitor* visitor, RootType root_type);

typedef void (MarkHeapReferenceCallback)(mirror::HeapReference<mirror::Object>* ref, void* arg);
typedef void (DelayReferenceReferentCallback)(mirror::Class* klass, mirror::Reference* ref, void* arg);

// A callback for testing if an object is marked, returns nullptr if not marked, otherwise the new
// address the object (if the object didn't move, returns the object input parameter).
typedef mirror::Object* (IsMarkedCallback)(mirror::Object* object, void* arg) WARN_UNUSED;

// Returns true if the object in the heap reference is marked, if it is marked and has moved the
// callback updates the heap reference contain the new value.
typedef bool (IsHeapReferenceMarkedCallback)(mirror::HeapReference<mirror::Object>* object,
    void* arg) WARN_UNUSED;
typedef void (ProcessMarkStackCallback)(void* arg);

}  // namespace art

#endif  // ART_RUNTIME_OBJECT_CALLBACKS_H_
