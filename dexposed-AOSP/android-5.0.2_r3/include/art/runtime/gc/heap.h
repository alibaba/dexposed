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

#ifndef ART_RUNTIME_GC_HEAP_H_
#define ART_RUNTIME_GC_HEAP_H_

#include <iosfwd>
#include <string>
#include <vector>

#include "allocator_type.h"
#include "atomic.h"
#include "base/timing_logger.h"
#include "gc/accounting/atomic_stack.h"
#include "gc/accounting/card_table.h"
#include "gc/gc_cause.h"
#include "gc/collector/garbage_collector.h"
#include "gc/collector/gc_type.h"
#include "gc/collector_type.h"
#include "globals.h"
#include "gtest/gtest.h"
#include "instruction_set.h"
#include "jni.h"
#include "object_callbacks.h"
#include "offsets.h"
#include "reference_processor.h"
#include "safe_map.h"
#include "thread_pool.h"
#include "verify_object.h"

namespace art {

class ConditionVariable;
class Mutex;
class StackVisitor;
class Thread;
class TimingLogger;

namespace mirror {
  class Class;
  class Object;
}  // namespace mirror

namespace gc {

class ReferenceProcessor;

namespace accounting {
  class HeapBitmap;
  class ModUnionTable;
  class RememberedSet;
}  // namespace accounting

namespace collector {
  class ConcurrentCopying;
  class GarbageCollector;
  class MarkCompact;
  class MarkSweep;
  class SemiSpace;
}  // namespace collector

namespace allocator {
  class RosAlloc;
}  // namespace allocator

namespace space {
  class AllocSpace;
  class BumpPointerSpace;
  class DiscontinuousSpace;
  class DlMallocSpace;
  class ImageSpace;
  class LargeObjectSpace;
  class MallocSpace;
  class RosAllocSpace;
  class Space;
  class SpaceTest;
  class ContinuousMemMapAllocSpace;
}  // namespace space

class AgeCardVisitor {
 public:
  byte operator()(byte card) const {
    if (card == accounting::CardTable::kCardDirty) {
      return card - 1;
    } else {
      return 0;
    }
  }
};

enum HomogeneousSpaceCompactResult {
  // Success.
  kSuccess,
  // Reject due to disabled moving GC.
  kErrorReject,
  // System is shutting down.
  kErrorVMShuttingDown,
};

// If true, use rosalloc/RosAllocSpace instead of dlmalloc/DlMallocSpace
static constexpr bool kUseRosAlloc = true;

// If true, use thread-local allocation stack.
static constexpr bool kUseThreadLocalAllocationStack = true;

// The process state passed in from the activity manager, used to determine when to do trimming
// and compaction.
enum ProcessState {
  kProcessStateJankPerceptible = 0,
  kProcessStateJankImperceptible = 1,
};
std::ostream& operator<<(std::ostream& os, const ProcessState& process_state);

class Heap {
 public:
  // If true, measure the total allocation time.
  static constexpr bool kMeasureAllocationTime = false;
  // Primitive arrays larger than this size are put in the large object space.
  static constexpr size_t kDefaultLargeObjectThreshold = 3 * kPageSize;
  static constexpr size_t kDefaultStartingSize = kPageSize;
  static constexpr size_t kDefaultInitialSize = 2 * MB;
  static constexpr size_t kDefaultMaximumSize = 256 * MB;
  static constexpr size_t kDefaultNonMovingSpaceCapacity = 64 * MB;
  static constexpr size_t kDefaultMaxFree = 2 * MB;
  static constexpr size_t kDefaultMinFree = kDefaultMaxFree / 4;
  static constexpr size_t kDefaultLongPauseLogThreshold = MsToNs(5);
  static constexpr size_t kDefaultLongGCLogThreshold = MsToNs(100);
  static constexpr size_t kDefaultTLABSize = 256 * KB;
  static constexpr double kDefaultTargetUtilization = 0.5;
  static constexpr double kDefaultHeapGrowthMultiplier = 2.0;

  // Used so that we don't overflow the allocation time atomic integer.
  static constexpr size_t kTimeAdjust = 1024;

  // How often we allow heap trimming to happen (nanoseconds).
  static constexpr uint64_t kHeapTrimWait = MsToNs(5000);
  // How long we wait after a transition request to perform a collector transition (nanoseconds).
  static constexpr uint64_t kCollectorTransitionWait = MsToNs(5000);

  // Create a heap with the requested sizes. The possible empty
  // image_file_names names specify Spaces to load based on
  // ImageWriter output.
  explicit Heap(size_t initial_size, size_t growth_limit, size_t min_free,
                size_t max_free, double target_utilization,
                double foreground_heap_growth_multiplier, size_t capacity,
                size_t non_moving_space_capacity,
                const std::string& original_image_file_name,
                InstructionSet image_instruction_set,
                CollectorType foreground_collector_type, CollectorType background_collector_type,
                size_t parallel_gc_threads, size_t conc_gc_threads, bool low_memory_mode,
                size_t long_pause_threshold, size_t long_gc_threshold,
                bool ignore_max_footprint, bool use_tlab,
                bool verify_pre_gc_heap, bool verify_pre_sweeping_heap, bool verify_post_gc_heap,
                bool verify_pre_gc_rosalloc, bool verify_pre_sweeping_rosalloc,
                bool verify_post_gc_rosalloc, bool use_homogeneous_space_compaction,
                uint64_t min_interval_homogeneous_space_compaction_by_oom);

  ~Heap();

  // Allocates and initializes storage for an object instance.
  template <bool kInstrumented, typename PreFenceVisitor>
  mirror::Object* AllocObject(Thread* self, mirror::Class* klass, size_t num_bytes,
                              const PreFenceVisitor& pre_fence_visitor)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return AllocObjectWithAllocator<kInstrumented, true>(self, klass, num_bytes,
                                                         GetCurrentAllocator(),
                                                         pre_fence_visitor);
  }

  template <bool kInstrumented, typename PreFenceVisitor>
  mirror::Object* AllocNonMovableObject(Thread* self, mirror::Class* klass, size_t num_bytes,
                                        const PreFenceVisitor& pre_fence_visitor)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    return AllocObjectWithAllocator<kInstrumented, true>(self, klass, num_bytes,
                                                         GetCurrentNonMovingAllocator(),
                                                         pre_fence_visitor);
  }

  template <bool kInstrumented, bool kCheckLargeObject, typename PreFenceVisitor>
  ALWAYS_INLINE mirror::Object* AllocObjectWithAllocator(
      Thread* self, mirror::Class* klass, size_t byte_count, AllocatorType allocator,
      const PreFenceVisitor& pre_fence_visitor)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  AllocatorType GetCurrentAllocator() const {
    return current_allocator_;
  }

  AllocatorType GetCurrentNonMovingAllocator() const {
    return current_non_moving_allocator_;
  }

  // Visit all of the live objects in the heap.
  void VisitObjects(ObjectCallback callback, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

  void CheckPreconditionsForAllocObject(mirror::Class* c, size_t byte_count)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void RegisterNativeAllocation(JNIEnv* env, size_t bytes);
  void RegisterNativeFree(JNIEnv* env, size_t bytes);

  // Change the allocator, updates entrypoints.
  void ChangeAllocator(AllocatorType allocator)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_)
      LOCKS_EXCLUDED(Locks::runtime_shutdown_lock_);

  // Transition the garbage collector during runtime, may copy objects from one space to another.
  void TransitionCollector(CollectorType collector_type);

  // Change the collector to be one of the possible options (MS, CMS, SS).
  void ChangeCollector(CollectorType collector_type)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);

  // The given reference is believed to be to an object in the Java heap, check the soundness of it.
  // TODO: NO_THREAD_SAFETY_ANALYSIS since we call this everywhere and it is impossible to find a
  // proper lock ordering for it.
  void VerifyObjectBody(mirror::Object* o) NO_THREAD_SAFETY_ANALYSIS;

  // Check sanity of all live references.
  void VerifyHeap() LOCKS_EXCLUDED(Locks::heap_bitmap_lock_);
  // Returns how many failures occured.
  size_t VerifyHeapReferences(bool verify_referents = true)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);
  bool VerifyMissingCardMarks()
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

  // A weaker test than IsLiveObject or VerifyObject that doesn't require the heap lock,
  // and doesn't abort on error, allowing the caller to report more
  // meaningful diagnostics.
  bool IsValidObjectAddress(const mirror::Object* obj) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Faster alternative to IsHeapAddress since finding if an object is in the large object space is
  // very slow.
  bool IsNonDiscontinuousSpaceHeapAddress(const mirror::Object* obj) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Returns true if 'obj' is a live heap object, false otherwise (including for invalid addresses).
  // Requires the heap lock to be held.
  bool IsLiveObjectLocked(mirror::Object* obj, bool search_allocation_stack = true,
                          bool search_live_stack = true, bool sorted = false)
      SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_, Locks::mutator_lock_);

  // Returns true if there is any chance that the object (obj) will move.
  bool IsMovableObject(const mirror::Object* obj) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Enables us to compacting GC until objects are released.
  void IncrementDisableMovingGC(Thread* self);
  void DecrementDisableMovingGC(Thread* self);

  // Clear all of the mark bits, doesn't clear bitmaps which have the same live bits as mark bits.
  void ClearMarkedObjects() EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_);

  // Initiates an explicit garbage collection.
  void CollectGarbage(bool clear_soft_references);

  // Does a concurrent GC, should only be called by the GC daemon thread
  // through runtime.
  void ConcurrentGC(Thread* self) LOCKS_EXCLUDED(Locks::runtime_shutdown_lock_);

  // Implements VMDebug.countInstancesOfClass and JDWP VM_InstanceCount.
  // The boolean decides whether to use IsAssignableFrom or == when comparing classes.
  void CountInstances(const std::vector<mirror::Class*>& classes, bool use_is_assignable_from,
                      uint64_t* counts)
      LOCKS_EXCLUDED(Locks::heap_bitmap_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  // Implements JDWP RT_Instances.
  void GetInstances(mirror::Class* c, int32_t max_count, std::vector<mirror::Object*>& instances)
      LOCKS_EXCLUDED(Locks::heap_bitmap_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  // Implements JDWP OR_ReferringObjects.
  void GetReferringObjects(mirror::Object* o, int32_t max_count, std::vector<mirror::Object*>& referring_objects)
      LOCKS_EXCLUDED(Locks::heap_bitmap_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Removes the growth limit on the alloc space so it may grow to its maximum capacity. Used to
  // implement dalvik.system.VMRuntime.clearGrowthLimit.
  void ClearGrowthLimit();

  // Target ideal heap utilization ratio, implements
  // dalvik.system.VMRuntime.getTargetHeapUtilization.
  double GetTargetHeapUtilization() const {
    return target_utilization_;
  }

  // Data structure memory usage tracking.
  void RegisterGCAllocation(size_t bytes);
  void RegisterGCDeAllocation(size_t bytes);

  // Set the heap's private space pointers to be the same as the space based on it's type. Public
  // due to usage by tests.
  void SetSpaceAsDefault(space::ContinuousSpace* continuous_space)
      LOCKS_EXCLUDED(Locks::heap_bitmap_lock_);
  void AddSpace(space::Space* space) LOCKS_EXCLUDED(Locks::heap_bitmap_lock_);
  void RemoveSpace(space::Space* space) LOCKS_EXCLUDED(Locks::heap_bitmap_lock_);

  // Set target ideal heap utilization ratio, implements
  // dalvik.system.VMRuntime.setTargetHeapUtilization.
  void SetTargetHeapUtilization(float target);

  // For the alloc space, sets the maximum number of bytes that the heap is allowed to allocate
  // from the system. Doesn't allow the space to exceed its growth limit.
  void SetIdealFootprint(size_t max_allowed_footprint);

  // Blocks the caller until the garbage collector becomes idle and returns the type of GC we
  // waited for.
  collector::GcType WaitForGcToComplete(GcCause cause, Thread* self)
      LOCKS_EXCLUDED(gc_complete_lock_);

  // Update the heap's process state to a new value, may cause compaction to occur.
  void UpdateProcessState(ProcessState process_state);

  const std::vector<space::ContinuousSpace*>& GetContinuousSpaces() const {
    return continuous_spaces_;
  }

  const std::vector<space::DiscontinuousSpace*>& GetDiscontinuousSpaces() const {
    return discontinuous_spaces_;
  }

  const collector::Iteration* GetCurrentGcIteration() const {
    return &current_gc_iteration_;
  }
  collector::Iteration* GetCurrentGcIteration() {
    return &current_gc_iteration_;
  }

  // Enable verification of object references when the runtime is sufficiently initialized.
  void EnableObjectValidation() {
    verify_object_mode_ = kVerifyObjectSupport;
    if (verify_object_mode_ > kVerifyObjectModeDisabled) {
      VerifyHeap();
    }
  }

  // Disable object reference verification for image writing.
  void DisableObjectValidation() {
    verify_object_mode_ = kVerifyObjectModeDisabled;
  }

  // Other checks may be performed if we know the heap should be in a sane state.
  bool IsObjectValidationEnabled() const {
    return verify_object_mode_ > kVerifyObjectModeDisabled;
  }

  // Returns true if low memory mode is enabled.
  bool IsLowMemoryMode() const {
    return low_memory_mode_;
  }

  // Returns the heap growth multiplier, this affects how much we grow the heap after a GC.
  // Scales heap growth, min free, and max free.
  double HeapGrowthMultiplier() const;

  // Freed bytes can be negative in cases where we copy objects from a compacted space to a
  // free-list backed space.
  void RecordFree(uint64_t freed_objects, int64_t freed_bytes);

  // Must be called if a field of an Object in the heap changes, and before any GC safe-point.
  // The call is not needed if NULL is stored in the field.
  void WriteBarrierField(const mirror::Object* dst, MemberOffset /*offset*/,
                         const mirror::Object* /*new_value*/) {
    card_table_->MarkCard(dst);
  }

  // Write barrier for array operations that update many field positions
  void WriteBarrierArray(const mirror::Object* dst, int /*start_offset*/,
                         size_t /*length TODO: element_count or byte_count?*/) {
    card_table_->MarkCard(dst);
  }

  void WriteBarrierEveryFieldOf(const mirror::Object* obj) {
    card_table_->MarkCard(obj);
  }

  accounting::CardTable* GetCardTable() const {
    return card_table_.get();
  }

  void AddFinalizerReference(Thread* self, mirror::Object** object);

  // Returns the number of bytes currently allocated.
  size_t GetBytesAllocated() const {
    return num_bytes_allocated_.LoadSequentiallyConsistent();
  }

  // Returns the number of objects currently allocated.
  size_t GetObjectsAllocated() const LOCKS_EXCLUDED(Locks::heap_bitmap_lock_);

  // Returns the total number of objects allocated since the heap was created.
  uint64_t GetObjectsAllocatedEver() const;

  // Returns the total number of bytes allocated since the heap was created.
  uint64_t GetBytesAllocatedEver() const;

  // Returns the total number of objects freed since the heap was created.
  uint64_t GetObjectsFreedEver() const {
    return total_objects_freed_ever_;
  }

  // Returns the total number of bytes freed since the heap was created.
  uint64_t GetBytesFreedEver() const {
    return total_bytes_freed_ever_;
  }

  // Implements java.lang.Runtime.maxMemory, returning the maximum amount of memory a program can
  // consume. For a regular VM this would relate to the -Xmx option and would return -1 if no Xmx
  // were specified. Android apps start with a growth limit (small heap size) which is
  // cleared/extended for large apps.
  size_t GetMaxMemory() const {
    // There is some race conditions in the allocation code that can cause bytes allocated to
    // become larger than growth_limit_ in rare cases.
    return std::max(GetBytesAllocated(), growth_limit_);
  }

  // Implements java.lang.Runtime.totalMemory, returning approximate amount of memory currently
  // consumed by an application.
  size_t GetTotalMemory() const;

  // Returns approximately how much free memory we have until the next GC happens.
  size_t GetFreeMemoryUntilGC() const {
    return max_allowed_footprint_ - GetBytesAllocated();
  }

  // Returns approximately how much free memory we have until the next OOME happens.
  size_t GetFreeMemoryUntilOOME() const {
    return growth_limit_ - GetBytesAllocated();
  }

  // Returns how much free memory we have until we need to grow the heap to perform an allocation.
  // Similar to GetFreeMemoryUntilGC. Implements java.lang.Runtime.freeMemory.
  size_t GetFreeMemory() const {
    size_t byte_allocated = num_bytes_allocated_.LoadSequentiallyConsistent();
    size_t total_memory = GetTotalMemory();
    // Make sure we don't get a negative number.
    return total_memory - std::min(total_memory, byte_allocated);
  }

  // get the space that corresponds to an object's address. Current implementation searches all
  // spaces in turn. If fail_ok is false then failing to find a space will cause an abort.
  // TODO: consider using faster data structure like binary tree.
  space::ContinuousSpace* FindContinuousSpaceFromObject(const mirror::Object*, bool fail_ok) const;
  space::DiscontinuousSpace* FindDiscontinuousSpaceFromObject(const mirror::Object*,
                                                              bool fail_ok) const;
  space::Space* FindSpaceFromObject(const mirror::Object*, bool fail_ok) const;

  void DumpForSigQuit(std::ostream& os) EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Do a pending heap transition or trim.
  void DoPendingTransitionOrTrim() LOCKS_EXCLUDED(heap_trim_request_lock_);

  // Trim the managed and native heaps by releasing unused memory back to the OS.
  void Trim() LOCKS_EXCLUDED(heap_trim_request_lock_);

  void RevokeThreadLocalBuffers(Thread* thread);
  void RevokeRosAllocThreadLocalBuffers(Thread* thread);
  void RevokeAllThreadLocalBuffers();
  void AssertAllBumpPointerSpaceThreadLocalBuffersAreRevoked();
  void RosAllocVerification(TimingLogger* timings, const char* name)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);

  accounting::HeapBitmap* GetLiveBitmap() SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_) {
    return live_bitmap_.get();
  }

  accounting::HeapBitmap* GetMarkBitmap() SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_) {
    return mark_bitmap_.get();
  }

  accounting::ObjectStack* GetLiveStack() SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_) {
    return live_stack_.get();
  }

  void PreZygoteFork() NO_THREAD_SAFETY_ANALYSIS;

  // Mark and empty stack.
  void FlushAllocStack()
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_);

  // Revoke all the thread-local allocation stacks.
  void RevokeAllThreadLocalAllocationStacks(Thread* self)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_)
      LOCKS_EXCLUDED(Locks::runtime_shutdown_lock_, Locks::thread_list_lock_);

  // Mark all the objects in the allocation stack in the specified bitmap.
  // TODO: Refactor?
  void MarkAllocStack(accounting::SpaceBitmap<kObjectAlignment>* bitmap1,
                      accounting::SpaceBitmap<kObjectAlignment>* bitmap2,
                      accounting::SpaceBitmap<kLargeObjectAlignment>* large_objects,
                      accounting::ObjectStack* stack)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_);

  // Mark the specified allocation stack as live.
  void MarkAllocStackAsLive(accounting::ObjectStack* stack)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_);

  // Unbind any bound bitmaps.
  void UnBindBitmaps() EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_);

  // DEPRECATED: Should remove in "near" future when support for multiple image spaces is added.
  // Assumes there is only one image space.
  space::ImageSpace* GetImageSpace() const;

  // Permenantly disable moving garbage collection.
  void DisableMovingGc();

  space::DlMallocSpace* GetDlMallocSpace() const {
    return dlmalloc_space_;
  }

  space::RosAllocSpace* GetRosAllocSpace() const {
    return rosalloc_space_;
  }

  // Return the corresponding rosalloc space.
  space::RosAllocSpace* GetRosAllocSpace(gc::allocator::RosAlloc* rosalloc) const;

  space::MallocSpace* GetNonMovingSpace() const {
    return non_moving_space_;
  }

  space::LargeObjectSpace* GetLargeObjectsSpace() const {
    return large_object_space_;
  }

  // Returns the free list space that may contain movable objects (the
  // one that's not the non-moving space), either rosalloc_space_ or
  // dlmalloc_space_.
  space::MallocSpace* GetPrimaryFreeListSpace() {
    if (kUseRosAlloc) {
      DCHECK(rosalloc_space_ != nullptr);
      // reinterpret_cast is necessary as the space class hierarchy
      // isn't known (#included) yet here.
      return reinterpret_cast<space::MallocSpace*>(rosalloc_space_);
    } else {
      DCHECK(dlmalloc_space_ != nullptr);
      return reinterpret_cast<space::MallocSpace*>(dlmalloc_space_);
    }
  }

  std::string DumpSpaces() const WARN_UNUSED;
  void DumpSpaces(std::ostream& stream) const;

  // Dump object should only be used by the signal handler.
  void DumpObject(std::ostream& stream, mirror::Object* obj) NO_THREAD_SAFETY_ANALYSIS;
  // Safe version of pretty type of which check to make sure objects are heap addresses.
  std::string SafeGetClassDescriptor(mirror::Class* klass) NO_THREAD_SAFETY_ANALYSIS;
  std::string SafePrettyTypeOf(mirror::Object* obj) NO_THREAD_SAFETY_ANALYSIS;

  // GC performance measuring
  void DumpGcPerformanceInfo(std::ostream& os);

  // Returns true if we currently care about pause times.
  bool CareAboutPauseTimes() const {
    return process_state_ == kProcessStateJankPerceptible;
  }

  // Thread pool.
  void CreateThreadPool();
  void DeleteThreadPool();
  ThreadPool* GetThreadPool() {
    return thread_pool_.get();
  }
  size_t GetParallelGCThreadCount() const {
    return parallel_gc_threads_;
  }
  size_t GetConcGCThreadCount() const {
    return conc_gc_threads_;
  }
  accounting::ModUnionTable* FindModUnionTableFromSpace(space::Space* space);
  void AddModUnionTable(accounting::ModUnionTable* mod_union_table);

  accounting::RememberedSet* FindRememberedSetFromSpace(space::Space* space);
  void AddRememberedSet(accounting::RememberedSet* remembered_set);
  // Also deletes the remebered set.
  void RemoveRememberedSet(space::Space* space);

  bool IsCompilingBoot() const;
  bool RunningOnValgrind() const {
    return running_on_valgrind_;
  }
  bool HasImageSpace() const;

  ReferenceProcessor* GetReferenceProcessor() {
    return &reference_processor_;
  }

 private:
  // Compact source space to target space.
  void Compact(space::ContinuousMemMapAllocSpace* target_space,
               space::ContinuousMemMapAllocSpace* source_space,
               GcCause gc_cause)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);

  void FinishGC(Thread* self, collector::GcType gc_type) LOCKS_EXCLUDED(gc_complete_lock_);

  // Create a mem map with a preferred base address.
  static MemMap* MapAnonymousPreferredAddress(const char* name, byte* request_begin,
                                              size_t capacity, int prot_flags,
                                              std::string* out_error_str);

  bool SupportHSpaceCompaction() const {
    // Returns true if we can do hspace compaction
    return main_space_backup_ != nullptr;
  }

  static ALWAYS_INLINE bool AllocatorHasAllocationStack(AllocatorType allocator_type) {
    return
        allocator_type != kAllocatorTypeBumpPointer &&
        allocator_type != kAllocatorTypeTLAB;
  }
  static ALWAYS_INLINE bool AllocatorMayHaveConcurrentGC(AllocatorType allocator_type) {
    return AllocatorHasAllocationStack(allocator_type);
  }
  static bool IsMovingGc(CollectorType collector_type) {
    return collector_type == kCollectorTypeSS || collector_type == kCollectorTypeGSS ||
        collector_type == kCollectorTypeCC || collector_type == kCollectorTypeMC ||
        collector_type == kCollectorTypeHomogeneousSpaceCompact;
  }
  bool ShouldAllocLargeObject(mirror::Class* c, size_t byte_count) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  ALWAYS_INLINE void CheckConcurrentGC(Thread* self, size_t new_num_bytes_allocated,
                                       mirror::Object** obj)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  accounting::ObjectStack* GetMarkStack() {
    return mark_stack_.get();
  }

  // We don't force this to be inlined since it is a slow path.
  template <bool kInstrumented, typename PreFenceVisitor>
  mirror::Object* AllocLargeObject(Thread* self, mirror::Class* klass, size_t byte_count,
                                   const PreFenceVisitor& pre_fence_visitor)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Handles Allocate()'s slow allocation path with GC involved after
  // an initial allocation attempt failed.
  mirror::Object* AllocateInternalWithGc(Thread* self, AllocatorType allocator, size_t num_bytes,
                                         size_t* bytes_allocated, size_t* usable_size,
                                         mirror::Class** klass)
      LOCKS_EXCLUDED(Locks::thread_suspend_count_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Allocate into a specific space.
  mirror::Object* AllocateInto(Thread* self, space::AllocSpace* space, mirror::Class* c,
                               size_t bytes)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Need to do this with mutators paused so that somebody doesn't accidentally allocate into the
  // wrong space.
  void SwapSemiSpaces() EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Try to allocate a number of bytes, this function never does any GCs. Needs to be inlined so
  // that the switch statement is constant optimized in the entrypoints.
  template <const bool kInstrumented, const bool kGrow>
  ALWAYS_INLINE mirror::Object* TryToAllocate(Thread* self, AllocatorType allocator_type,
                                              size_t alloc_size, size_t* bytes_allocated,
                                              size_t* usable_size)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void ThrowOutOfMemoryError(Thread* self, size_t byte_count, AllocatorType allocator_type)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  template <bool kGrow>
  bool IsOutOfMemoryOnAllocation(AllocatorType allocator_type, size_t alloc_size);

  // Returns true if the address passed in is within the address range of a continuous space.
  bool IsValidContinuousSpaceObjectAddress(const mirror::Object* obj) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Run the finalizers.
  void RunFinalization(JNIEnv* env);

  // Blocks the caller until the garbage collector becomes idle and returns the type of GC we
  // waited for.
  collector::GcType WaitForGcToCompleteLocked(GcCause cause, Thread* self)
      EXCLUSIVE_LOCKS_REQUIRED(gc_complete_lock_);

  void RequestCollectorTransition(CollectorType desired_collector_type, uint64_t delta_time)
      LOCKS_EXCLUDED(heap_trim_request_lock_);
  void RequestHeapTrim() LOCKS_EXCLUDED(Locks::runtime_shutdown_lock_);
  void RequestConcurrentGCAndSaveObject(Thread* self, mirror::Object** obj)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void RequestConcurrentGC(Thread* self)
      LOCKS_EXCLUDED(Locks::runtime_shutdown_lock_);
  bool IsGCRequestPending() const;

  // Sometimes CollectGarbageInternal decides to run a different Gc than you requested. Returns
  // which type of Gc was actually ran.
  collector::GcType CollectGarbageInternal(collector::GcType gc_plan, GcCause gc_cause,
                                           bool clear_soft_references)
      LOCKS_EXCLUDED(gc_complete_lock_,
                     Locks::heap_bitmap_lock_,
                     Locks::thread_suspend_count_lock_);

  void PreGcVerification(collector::GarbageCollector* gc)
      LOCKS_EXCLUDED(Locks::mutator_lock_);
  void PreGcVerificationPaused(collector::GarbageCollector* gc)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);
  void PrePauseRosAllocVerification(collector::GarbageCollector* gc)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);
  void PreSweepingGcVerification(collector::GarbageCollector* gc)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);
  void PostGcVerification(collector::GarbageCollector* gc)
      LOCKS_EXCLUDED(Locks::mutator_lock_);
  void PostGcVerificationPaused(collector::GarbageCollector* gc)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Update the watermark for the native allocated bytes based on the current number of native
  // bytes allocated and the target utilization ratio.
  void UpdateMaxNativeFootprint();

  // Find a collector based on GC type.
  collector::GarbageCollector* FindCollectorByGcType(collector::GcType gc_type);

  // Create a new alloc space and compact default alloc space to it.
  HomogeneousSpaceCompactResult PerformHomogeneousSpaceCompact();

  // Create the main free list malloc space, either a RosAlloc space or DlMalloc space.
  void CreateMainMallocSpace(MemMap* mem_map, size_t initial_size, size_t growth_limit,
                             size_t capacity);

  // Create a malloc space based on a mem map. Does not set the space as default.
  space::MallocSpace* CreateMallocSpaceFromMemMap(MemMap* mem_map, size_t initial_size,
                                                  size_t growth_limit, size_t capacity,
                                                  const char* name, bool can_move_objects);

  // Given the current contents of the alloc space, increase the allowed heap footprint to match
  // the target utilization ratio.  This should only be called immediately after a full garbage
  // collection.
  void GrowForUtilization(collector::GarbageCollector* collector_ran);

  size_t GetPercentFree();

  static void VerificationCallback(mirror::Object* obj, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_);

  // Swap the allocation stack with the live stack.
  void SwapStacks(Thread* self);

  // Clear cards and update the mod union table.
  void ProcessCards(TimingLogger* timings, bool use_rem_sets);

  // Signal the heap trim daemon that there is something to do, either a heap transition or heap
  // trim.
  void SignalHeapTrimDaemon(Thread* self);

  // Push an object onto the allocation stack.
  void PushOnAllocationStack(Thread* self, mirror::Object** obj)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void PushOnAllocationStackWithInternalGC(Thread* self, mirror::Object** obj)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void PushOnThreadLocalAllocationStackWithInternalGC(Thread* thread, mirror::Object** obj)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // What kind of concurrency behavior is the runtime after? Currently true for concurrent mark
  // sweep GC, false for other GC types.
  bool IsGcConcurrent() const ALWAYS_INLINE {
    return collector_type_ == kCollectorTypeCMS || collector_type_ == kCollectorTypeCC;
  }

  // All-known continuous spaces, where objects lie within fixed bounds.
  std::vector<space::ContinuousSpace*> continuous_spaces_;

  // All-known discontinuous spaces, where objects may be placed throughout virtual memory.
  std::vector<space::DiscontinuousSpace*> discontinuous_spaces_;

  // All-known alloc spaces, where objects may be or have been allocated.
  std::vector<space::AllocSpace*> alloc_spaces_;

  // A space where non-movable objects are allocated, when compaction is enabled it contains
  // Classes, ArtMethods, ArtFields, and non moving objects.
  space::MallocSpace* non_moving_space_;

  // Space which we use for the kAllocatorTypeROSAlloc.
  space::RosAllocSpace* rosalloc_space_;

  // Space which we use for the kAllocatorTypeDlMalloc.
  space::DlMallocSpace* dlmalloc_space_;

  // The main space is the space which the GC copies to and from on process state updates. This
  // space is typically either the dlmalloc_space_ or the rosalloc_space_.
  space::MallocSpace* main_space_;

  // The large object space we are currently allocating into.
  space::LargeObjectSpace* large_object_space_;

  // The card table, dirtied by the write barrier.
  std::unique_ptr<accounting::CardTable> card_table_;

  // A mod-union table remembers all of the references from the it's space to other spaces.
  AllocationTrackingSafeMap<space::Space*, accounting::ModUnionTable*, kAllocatorTagHeap>
      mod_union_tables_;

  // A remembered set remembers all of the references from the it's space to the target space.
  AllocationTrackingSafeMap<space::Space*, accounting::RememberedSet*, kAllocatorTagHeap>
      remembered_sets_;

  // The current collector type.
  CollectorType collector_type_;
  // Which collector we use when the app is in the foreground.
  CollectorType foreground_collector_type_;
  // Which collector we will use when the app is notified of a transition to background.
  CollectorType background_collector_type_;
  // Desired collector type, heap trimming daemon transitions the heap if it is != collector_type_.
  CollectorType desired_collector_type_;

  // Lock which guards heap trim requests.
  Mutex* heap_trim_request_lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;
  // When we want to perform the next heap trim (nano seconds).
  uint64_t last_trim_time_ GUARDED_BY(heap_trim_request_lock_);
  // When we want to perform the next heap transition (nano seconds) or heap trim.
  uint64_t heap_transition_or_trim_target_time_ GUARDED_BY(heap_trim_request_lock_);
  // If we have a heap trim request pending.
  bool heap_trim_request_pending_ GUARDED_BY(heap_trim_request_lock_);

  // How many GC threads we may use for paused parts of garbage collection.
  const size_t parallel_gc_threads_;

  // How many GC threads we may use for unpaused parts of garbage collection.
  const size_t conc_gc_threads_;

  // Boolean for if we are in low memory mode.
  const bool low_memory_mode_;

  // If we get a pause longer than long pause log threshold, then we print out the GC after it
  // finishes.
  const size_t long_pause_log_threshold_;

  // If we get a GC longer than long GC log threshold, then we print out the GC after it finishes.
  const size_t long_gc_log_threshold_;

  // If we ignore the max footprint it lets the heap grow until it hits the heap capacity, this is
  // useful for benchmarking since it reduces time spent in GC to a low %.
  const bool ignore_max_footprint_;

  // Lock which guards zygote space creation.
  Mutex zygote_creation_lock_;

  // If we have a zygote space.
  bool have_zygote_space_;

  // Minimum allocation size of large object.
  size_t large_object_threshold_;

  // Guards access to the state of GC, associated conditional variable is used to signal when a GC
  // completes.
  Mutex* gc_complete_lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;
  std::unique_ptr<ConditionVariable> gc_complete_cond_ GUARDED_BY(gc_complete_lock_);

  // Reference processor;
  ReferenceProcessor reference_processor_;

  // True while the garbage collector is running.
  volatile CollectorType collector_type_running_ GUARDED_BY(gc_complete_lock_);

  // Last Gc type we ran. Used by WaitForConcurrentGc to know which Gc was waited on.
  volatile collector::GcType last_gc_type_ GUARDED_BY(gc_complete_lock_);
  collector::GcType next_gc_type_;

  // Maximum size that the heap can reach.
  const size_t capacity_;

  // The size the heap is limited to. This is initially smaller than capacity, but for largeHeap
  // programs it is "cleared" making it the same as capacity.
  size_t growth_limit_;

  // When the number of bytes allocated exceeds the footprint TryAllocate returns NULL indicating
  // a GC should be triggered.
  size_t max_allowed_footprint_;

  // The watermark at which a concurrent GC is requested by registerNativeAllocation.
  size_t native_footprint_gc_watermark_;

  // Whether or not we need to run finalizers in the next native allocation.
  bool native_need_to_run_finalization_;

  // Whether or not we currently care about pause times.
  ProcessState process_state_;

  // When num_bytes_allocated_ exceeds this amount then a concurrent GC should be requested so that
  // it completes ahead of an allocation failing.
  size_t concurrent_start_bytes_;

  // Since the heap was created, how many bytes have been freed.
  uint64_t total_bytes_freed_ever_;

  // Since the heap was created, how many objects have been freed.
  uint64_t total_objects_freed_ever_;

  // Number of bytes allocated.  Adjusted after each allocation and free.
  Atomic<size_t> num_bytes_allocated_;

  // Bytes which are allocated and managed by native code but still need to be accounted for.
  Atomic<size_t> native_bytes_allocated_;

  // Info related to the current or previous GC iteration.
  collector::Iteration current_gc_iteration_;

  // Heap verification flags.
  const bool verify_missing_card_marks_;
  const bool verify_system_weaks_;
  const bool verify_pre_gc_heap_;
  const bool verify_pre_sweeping_heap_;
  const bool verify_post_gc_heap_;
  const bool verify_mod_union_table_;
  bool verify_pre_gc_rosalloc_;
  bool verify_pre_sweeping_rosalloc_;
  bool verify_post_gc_rosalloc_;

  // RAII that temporarily disables the rosalloc verification during
  // the zygote fork.
  class ScopedDisableRosAllocVerification {
   private:
    Heap* const heap_;
    const bool orig_verify_pre_gc_;
    const bool orig_verify_pre_sweeping_;
    const bool orig_verify_post_gc_;

   public:
    explicit ScopedDisableRosAllocVerification(Heap* heap)
        : heap_(heap),
          orig_verify_pre_gc_(heap_->verify_pre_gc_rosalloc_),
          orig_verify_pre_sweeping_(heap_->verify_pre_sweeping_rosalloc_),
          orig_verify_post_gc_(heap_->verify_post_gc_rosalloc_) {
      heap_->verify_pre_gc_rosalloc_ = false;
      heap_->verify_pre_sweeping_rosalloc_ = false;
      heap_->verify_post_gc_rosalloc_ = false;
    }
    ~ScopedDisableRosAllocVerification() {
      heap_->verify_pre_gc_rosalloc_ = orig_verify_pre_gc_;
      heap_->verify_pre_sweeping_rosalloc_ = orig_verify_pre_sweeping_;
      heap_->verify_post_gc_rosalloc_ = orig_verify_post_gc_;
    }
  };

  // Parallel GC data structures.
  std::unique_ptr<ThreadPool> thread_pool_;

  // The nanosecond time at which the last GC ended.
  uint64_t last_gc_time_ns_;

  // How many bytes were allocated at the end of the last GC.
  uint64_t last_gc_size_;

  // Estimated allocation rate (bytes / second). Computed between the time of the last GC cycle
  // and the start of the current one.
  uint64_t allocation_rate_;

  // For a GC cycle, a bitmap that is set corresponding to the
  std::unique_ptr<accounting::HeapBitmap> live_bitmap_ GUARDED_BY(Locks::heap_bitmap_lock_);
  std::unique_ptr<accounting::HeapBitmap> mark_bitmap_ GUARDED_BY(Locks::heap_bitmap_lock_);

  // Mark stack that we reuse to avoid re-allocating the mark stack.
  std::unique_ptr<accounting::ObjectStack> mark_stack_;

  // Allocation stack, new allocations go here so that we can do sticky mark bits. This enables us
  // to use the live bitmap as the old mark bitmap.
  const size_t max_allocation_stack_size_;
  std::unique_ptr<accounting::ObjectStack> allocation_stack_;

  // Second allocation stack so that we can process allocation with the heap unlocked.
  std::unique_ptr<accounting::ObjectStack> live_stack_;

  // Allocator type.
  AllocatorType current_allocator_;
  const AllocatorType current_non_moving_allocator_;

  // Which GCs we run in order when we an allocation fails.
  std::vector<collector::GcType> gc_plan_;

  // Bump pointer spaces.
  space::BumpPointerSpace* bump_pointer_space_;
  // Temp space is the space which the semispace collector copies to.
  space::BumpPointerSpace* temp_space_;

  // Minimum free guarantees that you always have at least min_free_ free bytes after growing for
  // utilization, regardless of target utilization ratio.
  size_t min_free_;

  // The ideal maximum free size, when we grow the heap for utilization.
  size_t max_free_;

  // Target ideal heap utilization ratio
  double target_utilization_;

  // How much more we grow the heap when we are a foreground app instead of background.
  double foreground_heap_growth_multiplier_;

  // Total time which mutators are paused or waiting for GC to complete.
  uint64_t total_wait_time_;

  // Total number of objects allocated in microseconds.
  AtomicInteger total_allocation_time_;

  // The current state of heap verification, may be enabled or disabled.
  VerifyObjectMode verify_object_mode_;

  // Compacting GC disable count, prevents compacting GC from running iff > 0.
  size_t disable_moving_gc_count_ GUARDED_BY(gc_complete_lock_);

  std::vector<collector::GarbageCollector*> garbage_collectors_;
  collector::SemiSpace* semi_space_collector_;
  collector::MarkCompact* mark_compact_collector_;
  collector::ConcurrentCopying* concurrent_copying_collector_;

  const bool running_on_valgrind_;
  const bool use_tlab_;

  // Pointer to the space which becomes the new main space when we do homogeneous space compaction.
  // Use unique_ptr since the space is only added during the homogeneous compaction phase.
  std::unique_ptr<space::MallocSpace> main_space_backup_;

  // Minimal interval allowed between two homogeneous space compactions caused by OOM.
  uint64_t min_interval_homogeneous_space_compaction_by_oom_;

  // Times of the last homogeneous space compaction caused by OOM.
  uint64_t last_time_homogeneous_space_compaction_by_oom_;

  // Saved OOMs by homogeneous space compaction.
  Atomic<size_t> count_delayed_oom_;

  // Count for requested homogeneous space compaction.
  Atomic<size_t> count_requested_homogeneous_space_compaction_;

  // Count for ignored homogeneous space compaction.
  Atomic<size_t> count_ignored_homogeneous_space_compaction_;

  // Count for performed homogeneous space compaction.
  Atomic<size_t> count_performed_homogeneous_space_compaction_;

  // Whether or not we use homogeneous space compaction to avoid OOM errors.
  bool use_homogeneous_space_compaction_for_oom_;

  friend class collector::GarbageCollector;
  friend class collector::MarkCompact;
  friend class collector::MarkSweep;
  friend class collector::SemiSpace;
  friend class ReferenceQueue;
  friend class VerifyReferenceCardVisitor;
  friend class VerifyReferenceVisitor;
  friend class VerifyObjectVisitor;
  friend class ScopedHeapFill;
  friend class ScopedHeapLock;
  friend class space::SpaceTest;

  class AllocationTimer {
   private:
    Heap* heap_;
    mirror::Object** allocated_obj_ptr_;
    uint64_t allocation_start_time_;
   public:
    AllocationTimer(Heap* heap, mirror::Object** allocated_obj_ptr);
    ~AllocationTimer();
  };

  DISALLOW_IMPLICIT_CONSTRUCTORS(Heap);
};

// ScopedHeapFill changes the bytes allocated counter to be equal to the growth limit. This
// causes the next allocation to perform a GC and possibly an OOM. It can be used to ensure that a
// GC happens in specific methods such as ThrowIllegalMonitorStateExceptionF in Monitor::Wait.
class ScopedHeapFill {
 public:
  explicit ScopedHeapFill(Heap* heap)
      : heap_(heap),
        delta_(heap_->GetMaxMemory() - heap_->GetBytesAllocated()) {
    heap_->num_bytes_allocated_.FetchAndAddSequentiallyConsistent(delta_);
  }
  ~ScopedHeapFill() {
    heap_->num_bytes_allocated_.FetchAndSubSequentiallyConsistent(delta_);
  }

 private:
  Heap* const heap_;
  const int64_t delta_;
};

}  // namespace gc
}  // namespace art

#endif  // ART_RUNTIME_GC_HEAP_H_
