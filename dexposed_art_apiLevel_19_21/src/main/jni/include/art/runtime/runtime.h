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

#ifndef ART_RUNTIME_RUNTIME_H_
#define ART_RUNTIME_RUNTIME_H_

#include <jni.h>
#include <stdio.h>

#include <iosfwd>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/allocator.h"
#include "compiler_callbacks.h"
#include "gc_root.h"
#include "instrumentation.h"
#include "instruction_set.h"
#include "jobject_comparator.h"
#include "object_callbacks.h"
#include "offsets.h"
#include "profiler_options.h"
#include "quick/quick_method_frame_info.h"
#include "runtime_stats.h"
#include "safe_map.h"

namespace art {

namespace gc {
  class Heap;
}  // namespace gc
namespace mirror {
  class ArtMethod;
  class ClassLoader;
  class Array;
  template<class T> class ObjectArray;
  template<class T> class PrimitiveArray;
  typedef PrimitiveArray<int8_t> ByteArray;
  class String;
  class Throwable;
}  // namespace mirror
namespace verifier {
class MethodVerifier;
}
class ClassLinker;
class DexFile;
class InternTable;
class JavaVMExt;
class MonitorList;
class MonitorPool;
class NullPointerHandler;
class SignalCatcher;
class StackOverflowHandler;
class SuspensionHandler;
class ThreadList;
class Trace;
class Transaction;

typedef std::vector<std::pair<std::string, const void*>> RuntimeOptions;

// Not all combinations of flags are valid. You may not visit all roots as well as the new roots
// (no logical reason to do this). You also may not start logging new roots and stop logging new
// roots (also no logical reason to do this).
enum VisitRootFlags : uint8_t {
  kVisitRootFlagAllRoots = 0x1,
  kVisitRootFlagNewRoots = 0x2,
  kVisitRootFlagStartLoggingNewRoots = 0x4,
  kVisitRootFlagStopLoggingNewRoots = 0x8,
  kVisitRootFlagClearRootLog = 0x10,
};

class Runtime {
 public:
  // Creates and initializes a new runtime.
  static bool Create(const RuntimeOptions& options, bool ignore_unrecognized)
      SHARED_TRYLOCK_FUNCTION(true, Locks::mutator_lock_);

  bool IsCompiler() const {
    return compiler_callbacks_ != nullptr;
  }

  bool CanRelocate() const {
    return !IsCompiler() || compiler_callbacks_->IsRelocationPossible();
  }

  bool ShouldRelocate() const {
    return must_relocate_ && CanRelocate();
  }

  bool MustRelocateIfPossible() const {
    return must_relocate_;
  }

  bool IsDex2OatEnabled() const {
    return dex2oat_enabled_ && IsImageDex2OatEnabled();
  }

  bool IsImageDex2OatEnabled() const {
    return image_dex2oat_enabled_;
  }

  CompilerCallbacks* GetCompilerCallbacks() {
    return compiler_callbacks_;
  }

  bool IsZygote() const {
    return is_zygote_;
  }

  bool IsExplicitGcDisabled() const {
    return is_explicit_gc_disabled_;
  }

  std::string GetCompilerExecutable() const;
  std::string GetPatchoatExecutable() const;

  const std::vector<std::string>& GetCompilerOptions() const {
    return compiler_options_;
  }

  const std::vector<std::string>& GetImageCompilerOptions() const {
    return image_compiler_options_;
  }

  const std::string& GetImageLocation() const {
    return image_location_;
  }

  const ProfilerOptions& GetProfilerOptions() const {
    return profiler_options_;
  }

  // Starts a runtime, which may cause threads to be started and code to run.
  bool Start() UNLOCK_FUNCTION(Locks::mutator_lock_);

  bool IsShuttingDown(Thread* self);
  bool IsShuttingDownLocked() const EXCLUSIVE_LOCKS_REQUIRED(Locks::runtime_shutdown_lock_) {
    return shutting_down_;
  }

  size_t NumberOfThreadsBeingBorn() const EXCLUSIVE_LOCKS_REQUIRED(Locks::runtime_shutdown_lock_) {
    return threads_being_born_;
  }

  void StartThreadBirth() EXCLUSIVE_LOCKS_REQUIRED(Locks::runtime_shutdown_lock_) {
    threads_being_born_++;
  }

  void EndThreadBirth() EXCLUSIVE_LOCKS_REQUIRED(Locks::runtime_shutdown_lock_);

  bool IsStarted() const {
    return started_;
  }

  bool IsFinishedStarting() const {
    return finished_starting_;
  }

  static Runtime* Current() {
    return instance_;
  }

  // Aborts semi-cleanly. Used in the implementation of LOG(FATAL), which most
  // callers should prefer.
  // This isn't marked ((noreturn)) because then gcc will merge multiple calls
  // in a single function together. This reduces code size slightly, but means
  // that the native stack trace we get may point at the wrong call site.
  static void Abort() LOCKS_EXCLUDED(Locks::abort_lock_);

  // Returns the "main" ThreadGroup, used when attaching user threads.
  jobject GetMainThreadGroup() const;

  // Returns the "system" ThreadGroup, used when attaching our internal threads.
  jobject GetSystemThreadGroup() const;

  // Returns the system ClassLoader which represents the CLASSPATH.
  jobject GetSystemClassLoader() const;

  // Attaches the calling native thread to the runtime.
  bool AttachCurrentThread(const char* thread_name, bool as_daemon, jobject thread_group,
                           bool create_peer);

  void CallExitHook(jint status);

  // Detaches the current native thread from the runtime.
  void DetachCurrentThread() LOCKS_EXCLUDED(Locks::mutator_lock_);

  void DumpForSigQuit(std::ostream& os)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);
  void DumpLockHolders(std::ostream& os);

  ~Runtime();

  const std::string& GetBootClassPathString() const {
    return boot_class_path_string_;
  }

  const std::string& GetClassPathString() const {
    return class_path_string_;
  }

  ClassLinker* GetClassLinker() const {
    return class_linker_;
  }

  size_t GetDefaultStackSize() const {
    return default_stack_size_;
  }

  gc::Heap* GetHeap() const {
    return heap_;
  }

  InternTable* GetInternTable() const {
    DCHECK(intern_table_ != NULL);
    return intern_table_;
  }

  JavaVMExt* GetJavaVM() const {
    return java_vm_;
  }

  size_t GetMaxSpinsBeforeThinkLockInflation() const {
    return max_spins_before_thin_lock_inflation_;
  }

  MonitorList* GetMonitorList() const {
    return monitor_list_;
  }

  MonitorPool* GetMonitorPool() const {
    return monitor_pool_;
  }

  mirror::Throwable* GetPreAllocatedOutOfMemoryError() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::Throwable* GetPreAllocatedNoClassDefFoundError()
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const std::vector<std::string>& GetProperties() const {
    return properties_;
  }

  ThreadList* GetThreadList() const {
    return thread_list_;
  }

  static const char* GetVersion() {
    return "2.1.0";
  }

  void DisallowNewSystemWeaks() EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_);
  void AllowNewSystemWeaks() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Visit all the roots. If only_dirty is true then non-dirty roots won't be visited. If
  // clean_dirty is true then dirty roots will be marked as non-dirty after visiting.
  void VisitRoots(RootCallback* visitor, void* arg, VisitRootFlags flags = kVisitRootFlagAllRoots)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Visit all of the roots we can do safely do concurrently.
  void VisitConcurrentRoots(RootCallback* visitor, void* arg,
                            VisitRootFlags flags = kVisitRootFlagAllRoots)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Visit all of the non thread roots, we can do this with mutators unpaused.
  void VisitNonThreadRoots(RootCallback* visitor, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Visit all other roots which must be done with mutators suspended.
  void VisitNonConcurrentRoots(RootCallback* visitor, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Sweep system weaks, the system weak is deleted if the visitor return nullptr. Otherwise, the
  // system weak is updated to be the visitor's returned value.
  void SweepSystemWeaks(IsMarkedCallback* visitor, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Constant roots are the roots which never change after the runtime is initialized, they only
  // need to be visited once per GC cycle.
  void VisitConstantRoots(RootCallback* callback, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Returns a special method that calls into a trampoline for runtime method resolution
  mirror::ArtMethod* GetResolutionMethod() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool HasResolutionMethod() const {
    return !resolution_method_.IsNull();
  }

  void SetResolutionMethod(mirror::ArtMethod* method) {
    resolution_method_ = GcRoot<mirror::ArtMethod>(method);
  }

  mirror::ArtMethod* CreateResolutionMethod() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Returns a special method that calls into a trampoline for runtime imt conflicts.
  mirror::ArtMethod* GetImtConflictMethod() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool HasImtConflictMethod() const {
    return !imt_conflict_method_.IsNull();
  }

  void SetImtConflictMethod(mirror::ArtMethod* method) {
    imt_conflict_method_ = GcRoot<mirror::ArtMethod>(method);
  }

  mirror::ArtMethod* CreateImtConflictMethod() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Returns an imt with every entry set to conflict, used as default imt for all classes.
  mirror::ObjectArray<mirror::ArtMethod>* GetDefaultImt()
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool HasDefaultImt() const {
    return !default_imt_.IsNull();
  }

  void SetDefaultImt(mirror::ObjectArray<mirror::ArtMethod>* imt) {
    default_imt_ = GcRoot<mirror::ObjectArray<mirror::ArtMethod>>(imt);
  }

  mirror::ObjectArray<mirror::ArtMethod>* CreateDefaultImt(ClassLinker* cl)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Returns a special method that describes all callee saves being spilled to the stack.
  enum CalleeSaveType {
    kSaveAll,
    kRefsOnly,
    kRefsAndArgs,
    kLastCalleeSaveType  // Value used for iteration
  };

  bool HasCalleeSaveMethod(CalleeSaveType type) const {
    return !callee_save_methods_[type].IsNull();
  }

  mirror::ArtMethod* GetCalleeSaveMethod(CalleeSaveType type)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::ArtMethod* GetCalleeSaveMethodUnchecked(CalleeSaveType type)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  QuickMethodFrameInfo GetCalleeSaveMethodFrameInfo(CalleeSaveType type) const {
    return callee_save_method_frame_infos_[type];
  }

  QuickMethodFrameInfo GetRuntimeMethodFrameInfo(mirror::ArtMethod* method)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static size_t GetCalleeSaveMethodOffset(CalleeSaveType type) {
    return OFFSETOF_MEMBER(Runtime, callee_save_methods_[type]);
  }

  InstructionSet GetInstructionSet() const {
    return instruction_set_;
  }

  void SetInstructionSet(InstructionSet instruction_set);

  void SetCalleeSaveMethod(mirror::ArtMethod* method, CalleeSaveType type);

  mirror::ArtMethod* CreateCalleeSaveMethod(CalleeSaveType type)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  int32_t GetStat(int kind);

  RuntimeStats* GetStats() {
    return &stats_;
  }

  bool HasStatsEnabled() const {
    return stats_enabled_;
  }

  void ResetStats(int kinds);

  void SetStatsEnabled(bool new_state) LOCKS_EXCLUDED(Locks::instrument_entrypoints_lock_,
                                                      Locks::mutator_lock_);

  enum class NativeBridgeAction {  // private
    kUnload,
    kInitialize
  };
  void PreZygoteFork();
  bool InitZygote();
  void DidForkFromZygote(JNIEnv* env, NativeBridgeAction action, const char* isa);

  const instrumentation::Instrumentation* GetInstrumentation() const {
    return &instrumentation_;
  }

  instrumentation::Instrumentation* GetInstrumentation() {
    return &instrumentation_;
  }

  bool UseCompileTimeClassPath() const {
    return use_compile_time_class_path_;
  }

  void AddMethodVerifier(verifier::MethodVerifier* verifier) LOCKS_EXCLUDED(method_verifier_lock_);
  void RemoveMethodVerifier(verifier::MethodVerifier* verifier)
      LOCKS_EXCLUDED(method_verifier_lock_);

  const std::vector<const DexFile*>& GetCompileTimeClassPath(jobject class_loader);
  void SetCompileTimeClassPath(jobject class_loader, std::vector<const DexFile*>& class_path);

  void StartProfiler(const char* profile_output_filename);
  void UpdateProfilerState(int state);

  // Transaction support.
  bool IsActiveTransaction() const {
    return preinitialization_transaction_ != nullptr;
  }
  void EnterTransactionMode(Transaction* transaction);
  void ExitTransactionMode();
  void RecordWriteField32(mirror::Object* obj, MemberOffset field_offset, uint32_t value,
                          bool is_volatile) const;
  void RecordWriteField64(mirror::Object* obj, MemberOffset field_offset, uint64_t value,
                          bool is_volatile) const;
  void RecordWriteFieldReference(mirror::Object* obj, MemberOffset field_offset,
                                 mirror::Object* value, bool is_volatile) const;
  void RecordWriteArray(mirror::Array* array, size_t index, uint64_t value) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void RecordStrongStringInsertion(mirror::String* s) const
      EXCLUSIVE_LOCKS_REQUIRED(Locks::intern_table_lock_);
  void RecordWeakStringInsertion(mirror::String* s) const
      EXCLUSIVE_LOCKS_REQUIRED(Locks::intern_table_lock_);
  void RecordStrongStringRemoval(mirror::String* s) const
      EXCLUSIVE_LOCKS_REQUIRED(Locks::intern_table_lock_);
  void RecordWeakStringRemoval(mirror::String* s) const
      EXCLUSIVE_LOCKS_REQUIRED(Locks::intern_table_lock_);

  void SetFaultMessage(const std::string& message);
  // Only read by the signal handler, NO_THREAD_SAFETY_ANALYSIS to prevent lock order violations
  // with the unexpected_signal_lock_.
  const std::string& GetFaultMessage() NO_THREAD_SAFETY_ANALYSIS {
    return fault_message_;
  }

  void AddCurrentRuntimeFeaturesAsDex2OatArguments(std::vector<std::string>* arg_vector) const;

  bool ExplicitNullChecks() const {
    return null_pointer_handler_ == nullptr;
  }

  bool ExplicitSuspendChecks() const {
    return suspend_handler_ == nullptr;
  }

  bool ExplicitStackOverflowChecks() const {
    return stack_overflow_handler_ == nullptr;
  }

  bool IsVerificationEnabled() const {
    return verify_;
  }

  bool RunningOnValgrind() const {
    return running_on_valgrind_;
  }

  void SetTargetSdkVersion(int32_t version) {
    target_sdk_version_ = version;
  }

  int32_t GetTargetSdkVersion() const {
    return target_sdk_version_;
  }

  static const char* GetDefaultInstructionSetFeatures() {
    return kDefaultInstructionSetFeatures;
  }

 private:
  static void InitPlatformSignalHandlers();

  Runtime();

  void BlockSignals();

  bool Init(const RuntimeOptions& options, bool ignore_unrecognized)
      SHARED_TRYLOCK_FUNCTION(true, Locks::mutator_lock_);
  void InitNativeMethods() LOCKS_EXCLUDED(Locks::mutator_lock_);
  void InitThreadGroups(Thread* self);
  void RegisterRuntimeNativeMethods(JNIEnv* env);

  void StartDaemonThreads();
  void StartSignalCatcher();

  // A pointer to the active runtime or NULL.
  static Runtime* instance_;

  static const char* kDefaultInstructionSetFeatures;

  // NOTE: these must match the gc::ProcessState values as they come directly from the framework.
  static constexpr int kProfileForground = 0;
  static constexpr int kProfileBackgrouud = 1;

  GcRoot<mirror::ArtMethod> callee_save_methods_[kLastCalleeSaveType];
  GcRoot<mirror::Throwable> pre_allocated_OutOfMemoryError_;
  GcRoot<mirror::Throwable> pre_allocated_NoClassDefFoundError_;
  GcRoot<mirror::ArtMethod> resolution_method_;
  GcRoot<mirror::ArtMethod> imt_conflict_method_;
  GcRoot<mirror::ObjectArray<mirror::ArtMethod>> default_imt_;

  InstructionSet instruction_set_;
  QuickMethodFrameInfo callee_save_method_frame_infos_[kLastCalleeSaveType];

  CompilerCallbacks* compiler_callbacks_;
  bool is_zygote_;
  bool must_relocate_;
  bool is_concurrent_gc_enabled_;
  bool is_explicit_gc_disabled_;
  bool dex2oat_enabled_;
  bool image_dex2oat_enabled_;

  std::string compiler_executable_;
  std::string patchoat_executable_;
  std::vector<std::string> compiler_options_;
  std::vector<std::string> image_compiler_options_;
  std::string image_location_;

  std::string boot_class_path_string_;
  std::string class_path_string_;
  std::vector<std::string> properties_;

  // The default stack size for managed threads created by the runtime.
  size_t default_stack_size_;

  gc::Heap* heap_;

  // The number of spins that are done before thread suspension is used to forcibly inflate.
  size_t max_spins_before_thin_lock_inflation_;
  MonitorList* monitor_list_;
  MonitorPool* monitor_pool_;

  ThreadList* thread_list_;

  InternTable* intern_table_;

  ClassLinker* class_linker_;

  SignalCatcher* signal_catcher_;
  std::string stack_trace_file_;

  JavaVMExt* java_vm_;

  // Fault message, printed when we get a SIGSEGV.
  Mutex fault_message_lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;
  std::string fault_message_ GUARDED_BY(fault_message_lock_);

  // Method verifier set, used so that we can update their GC roots.
  Mutex method_verifier_lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;
  std::set<verifier::MethodVerifier*> method_verifiers_;

  // A non-zero value indicates that a thread has been created but not yet initialized. Guarded by
  // the shutdown lock so that threads aren't born while we're shutting down.
  size_t threads_being_born_ GUARDED_BY(Locks::runtime_shutdown_lock_);

  // Waited upon until no threads are being born.
  std::unique_ptr<ConditionVariable> shutdown_cond_ GUARDED_BY(Locks::runtime_shutdown_lock_);

  // Set when runtime shutdown is past the point that new threads may attach.
  bool shutting_down_ GUARDED_BY(Locks::runtime_shutdown_lock_);

  // The runtime is starting to shutdown but is blocked waiting on shutdown_cond_.
  bool shutting_down_started_ GUARDED_BY(Locks::runtime_shutdown_lock_);

  bool started_;

  // New flag added which tells us if the runtime has finished starting. If
  // this flag is set then the Daemon threads are created and the class loader
  // is created. This flag is needed for knowing if its safe to request CMS.
  bool finished_starting_;

  // Hooks supported by JNI_CreateJavaVM
  jint (*vfprintf_)(FILE* stream, const char* format, va_list ap);
  void (*exit_)(jint status);
  void (*abort_)();

  bool stats_enabled_;
  RuntimeStats stats_;

  const bool running_on_valgrind_;

  std::string profile_output_filename_;
  ProfilerOptions profiler_options_;
  bool profiler_started_;

  bool method_trace_;
  std::string method_trace_file_;
  size_t method_trace_file_size_;
  instrumentation::Instrumentation instrumentation_;

  typedef AllocationTrackingSafeMap<jobject, std::vector<const DexFile*>,
                                    kAllocatorTagCompileTimeClassPath, JobjectComparator>
      CompileTimeClassPaths;
  CompileTimeClassPaths compile_time_class_paths_;
  bool use_compile_time_class_path_;

  jobject main_thread_group_;
  jobject system_thread_group_;

  // As returned by ClassLoader.getSystemClassLoader().
  jobject system_class_loader_;

  // If true, then we dump the GC cumulative timings on shutdown.
  bool dump_gc_performance_on_shutdown_;

  // Transaction used for pre-initializing classes at compilation time.
  Transaction* preinitialization_transaction_;
  NullPointerHandler* null_pointer_handler_;
  SuspensionHandler* suspend_handler_;
  StackOverflowHandler* stack_overflow_handler_;

  // If false, verification is disabled. True by default.
  bool verify_;

  // Specifies target SDK version to allow workarounds for certain API levels.
  int32_t target_sdk_version_;

  // Implicit checks flags.
  bool implicit_null_checks_;       // NullPointer checks are implicit.
  bool implicit_so_checks_;         // StackOverflow checks are implicit.
  bool implicit_suspend_checks_;    // Thread suspension checks are implicit.

  // The filename to the native bridge library. If this is not empty the native bridge will be
  // initialized and loaded from the given file (initialized and available). An empty value means
  // that there's no native bridge (initialized but not available).
  //
  // The native bridge allows running native code compiled for a foreign ISA. The way it works is,
  // if standard dlopen fails to load native library associated with native activity, it calls to
  // the native bridge to load it and then gets the trampoline for the entry to native activity.
  std::string native_bridge_library_filename_;

  DISALLOW_COPY_AND_ASSIGN(Runtime);
};

}  // namespace art

#endif  // ART_RUNTIME_RUNTIME_H_
