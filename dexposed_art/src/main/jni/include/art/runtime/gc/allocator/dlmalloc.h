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

#ifndef ART_RUNTIME_GC_ALLOCATOR_DLMALLOC_H_
#define ART_RUNTIME_GC_ALLOCATOR_DLMALLOC_H_

// Configure dlmalloc for mspaces.
// Avoid a collision with one used in llvm.
#undef HAVE_MMAP
#define HAVE_MMAP 0
#define HAVE_MREMAP 0
#define HAVE_MORECORE 1
#define MSPACES 1
#define NO_MALLINFO 1
#define ONLY_MSPACES 1
#define MALLOC_INSPECT_ALL 1

#include "../../bionic/libc/upstream-dlmalloc/malloc.h"

// Define dlmalloc routines from bionic that cannot be included directly because of redefining
// symbols from the include above.
extern "C" void dlmalloc_inspect_all(void(*handler)(void*, void *, size_t, void*), void* arg);
extern "C" int  dlmalloc_trim(size_t);

// Callback for dlmalloc_inspect_all or mspace_inspect_all that will madvise(2) unused
// pages back to the kernel.
extern "C" void DlmallocMadviseCallback(void* start, void* end, size_t used_bytes, void* /*arg*/);

// Callbacks for dlmalloc_inspect_all or mspace_inspect_all that will
// count the number of bytes allocated and objects allocated,
// respectively.
extern "C" void DlmallocBytesAllocatedCallback(void* start, void* end, size_t used_bytes, void* arg);
extern "C" void DlmallocObjectsAllocatedCallback(void* start, void* end, size_t used_bytes, void* arg);

#endif  // ART_RUNTIME_GC_ALLOCATOR_DLMALLOC_H_
