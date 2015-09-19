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

#ifndef ART_RUNTIME_MONITOR_POOL_H_
#define ART_RUNTIME_MONITOR_POOL_H_

#include "monitor.h"

#include "base/allocator.h"
#ifdef __LP64__
#include <stdint.h>
#include "atomic.h"
#include "runtime.h"
#else
#include "base/stl_util.h"     // STLDeleteElements
#endif

namespace art {

// Abstraction to keep monitors small enough to fit in a lock word (32bits). On 32bit systems the
// monitor id loses the alignment bits of the Monitor*.
class MonitorPool {
 public:
  static MonitorPool* Create() {
#ifndef __LP64__
    return nullptr;
#else
    return new MonitorPool();
#endif
  }

  static Monitor* CreateMonitor(Thread* self, Thread* owner, mirror::Object* obj, int32_t hash_code)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
#ifndef __LP64__
    return new Monitor(self, owner, obj, hash_code);
#else
    return GetMonitorPool()->CreateMonitorInPool(self, owner, obj, hash_code);
#endif
  }

  static void ReleaseMonitor(Thread* self, Monitor* monitor) {
#ifndef __LP64__
    delete monitor;
#else
    GetMonitorPool()->ReleaseMonitorToPool(self, monitor);
#endif
  }

  static void ReleaseMonitors(Thread* self, MonitorList::Monitors* monitors) {
#ifndef __LP64__
    STLDeleteElements(monitors);
#else
    GetMonitorPool()->ReleaseMonitorsToPool(self, monitors);
#endif
  }

  static Monitor* MonitorFromMonitorId(MonitorId mon_id) {
#ifndef __LP64__
    return reinterpret_cast<Monitor*>(mon_id << 3);
#else
    return GetMonitorPool()->LookupMonitor(mon_id);
#endif
  }

  static MonitorId MonitorIdFromMonitor(Monitor* mon) {
#ifndef __LP64__
    return reinterpret_cast<MonitorId>(mon) >> 3;
#else
    return mon->GetMonitorId();
#endif
  }

  static MonitorId ComputeMonitorId(Monitor* mon, Thread* self) {
#ifndef __LP64__
    return MonitorIdFromMonitor(mon);
#else
    return GetMonitorPool()->ComputeMonitorIdInPool(mon, self);
#endif
  }

  static MonitorPool* GetMonitorPool() {
#ifndef __LP64__
    return nullptr;
#else
    return Runtime::Current()->GetMonitorPool();
#endif
  }

 private:
#ifdef __LP64__
  // When we create a monitor pool, threads have not been initialized, yet, so ignore thread-safety
  // analysis.
  MonitorPool() NO_THREAD_SAFETY_ANALYSIS;

  void AllocateChunk() EXCLUSIVE_LOCKS_REQUIRED(Locks::allocated_monitor_ids_lock_);

  Monitor* CreateMonitorInPool(Thread* self, Thread* owner, mirror::Object* obj, int32_t hash_code)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void ReleaseMonitorToPool(Thread* self, Monitor* monitor);
  void ReleaseMonitorsToPool(Thread* self, MonitorList::Monitors* monitors);

  // Note: This is safe as we do not ever move chunks.
  Monitor* LookupMonitor(MonitorId mon_id) {
    size_t offset = MonitorIdToOffset(mon_id);
    size_t index = offset / kChunkSize;
    size_t offset_in_chunk = offset % kChunkSize;
    uintptr_t base = *(monitor_chunks_.LoadRelaxed()+index);
    return reinterpret_cast<Monitor*>(base + offset_in_chunk);
  }

  static bool IsInChunk(uintptr_t base_addr, Monitor* mon) {
    uintptr_t mon_ptr = reinterpret_cast<uintptr_t>(mon);
    return base_addr <= mon_ptr && (mon_ptr - base_addr < kChunkSize);
  }

  // Note: This is safe as we do not ever move chunks.
  MonitorId ComputeMonitorIdInPool(Monitor* mon, Thread* self) {
    MutexLock mu(self, *Locks::allocated_monitor_ids_lock_);
    for (size_t index = 0; index < num_chunks_; ++index) {
      uintptr_t chunk_addr = *(monitor_chunks_.LoadRelaxed() + index);
      if (IsInChunk(chunk_addr, mon)) {
        return OffsetToMonitorId(reinterpret_cast<uintptr_t>(mon) - chunk_addr + index * kChunkSize);
      }
    }
    LOG(FATAL) << "Did not find chunk that contains monitor.";
    return 0;
  }

  static size_t MonitorIdToOffset(MonitorId id) {
    return id << 3;
  }

  static MonitorId OffsetToMonitorId(size_t offset) {
    return static_cast<MonitorId>(offset >> 3);
  }

  // TODO: There are assumptions in the code that monitor addresses are 8B aligned (>>3).
  static constexpr size_t kMonitorAlignment = 8;
  // Size of a monitor, rounded up to a multiple of alignment.
  static constexpr size_t kAlignedMonitorSize = (sizeof(Monitor) + kMonitorAlignment - 1) &
                                                -kMonitorAlignment;
  // As close to a page as we can get seems a good start.
  static constexpr size_t kChunkCapacity = kPageSize / kAlignedMonitorSize;
  // Chunk size that is referenced in the id. We can collapse this to the actually used storage
  // in a chunk, i.e., kChunkCapacity * kAlignedMonitorSize, but this will mean proper divisions.
  static constexpr size_t kChunkSize = kPageSize;
  // The number of initial chunks storable in monitor_chunks_. The number is large enough to make
  // resizing unlikely, but small enough to not waste too much memory.
  static constexpr size_t kInitialChunkStorage = 8U;

  // List of memory chunks. Each chunk is kChunkSize.
  Atomic<uintptr_t*> monitor_chunks_;
  // Number of chunks stored.
  size_t num_chunks_ GUARDED_BY(Locks::allocated_monitor_ids_lock_);
  // Number of chunks storable.
  size_t capacity_ GUARDED_BY(Locks::allocated_monitor_ids_lock_);

  // To avoid race issues when resizing, we keep all the previous arrays.
  std::vector<uintptr_t*> old_chunk_arrays_ GUARDED_BY(Locks::allocated_monitor_ids_lock_);

  typedef TrackingAllocator<byte, kAllocatorTagMonitorPool> Allocator;
  Allocator allocator_;

  // Start of free list of monitors.
  // Note: these point to the right memory regions, but do *not* denote initialized objects.
  Monitor* first_free_ GUARDED_BY(Locks::allocated_monitor_ids_lock_);
#endif
};

}  // namespace art

#endif  // ART_RUNTIME_MONITOR_POOL_H_
