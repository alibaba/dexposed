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

/*
 * VM thread support.
 */
#ifndef DALVIK_THREAD_H_
#define DALVIK_THREAD_H_

#include "jni.h"
#include "interp/InterpState.h"

#include <errno.h>
#include <cutils/sched_policy.h>

#if defined(CHECK_MUTEX) && !defined(__USE_UNIX98)
/* glibc lacks this unless you #define __USE_UNIX98 */
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
enum { PTHREAD_MUTEX_ERRORCHECK = PTHREAD_MUTEX_ERRORCHECK_NP };
#endif

/*
 * Current status; these map to JDWP constants, so don't rearrange them.
 * (If you do alter this, update the strings in dvmDumpThread and the
 * conversion table in VMThread.java.)
 *
 * Note that "suspended" is orthogonal to these values (so says JDWP).
 */
enum ThreadStatus {
    THREAD_UNDEFINED    = -1,       /* makes enum compatible with int32_t */

    /* these match up with JDWP values */
    THREAD_ZOMBIE       = 0,        /* TERMINATED */
    THREAD_RUNNING      = 1,        /* RUNNABLE or running now */
    THREAD_TIMED_WAIT   = 2,        /* TIMED_WAITING in Object.wait() */
    THREAD_MONITOR      = 3,        /* BLOCKED on a monitor */
    THREAD_WAIT         = 4,        /* WAITING in Object.wait() */
    /* non-JDWP states */
    THREAD_INITIALIZING = 5,        /* allocated, not yet running */
    THREAD_STARTING     = 6,        /* started, not yet on thread list */
    THREAD_NATIVE       = 7,        /* off in a JNI native method */
    THREAD_VMWAIT       = 8,        /* waiting on a VM resource */
    THREAD_SUSPENDED    = 9,        /* suspended, usually by GC or debugger */
};

/* thread priorities, from java.lang.Thread */
enum {
    THREAD_MIN_PRIORITY     = 1,
    THREAD_NORM_PRIORITY    = 5,
    THREAD_MAX_PRIORITY     = 10,
};


/* initialization */
bool dvmThreadStartup(void);
void dvmThreadShutdown(void);
void dvmSlayDaemons(void);


#define kJniLocalRefMin         64
#define kJniLocalRefMax         512     /* arbitrary; should be plenty */
#define kInternalRefDefault     32      /* equally arbitrary */
#define kInternalRefMax         4096    /* mainly a sanity check */

#define kMinStackSize       (512 + STACK_OVERFLOW_RESERVE)
#define kDefaultStackSize   (16*1024)   /* four 4K pages */
#define kMaxStackSize       (256*1024 + STACK_OVERFLOW_RESERVE)

/*
 * Interpreter control struction.  Packed into a long long to enable
 * atomic updates.
 */
union InterpBreak {
    volatile int64_t   all;
    struct {
        uint16_t   subMode;
        uint8_t    breakFlags;
        int8_t     unused;   /* for future expansion */
#ifndef DVM_NO_ASM_INTERP
        void* curHandlerTable;
#else
        int32_t    unused1;
#endif
    } ctl;
};

/*
 * Our per-thread data.
 *
 * These are allocated on the system heap.
 */
struct Thread {
    /*
     * Interpreter state which must be preserved across nested
     * interpreter invocations (via JNI callbacks).  Must be the first
     * element in Thread.
     */
    InterpSaveState interpSave;

    /* small unique integer; useful for "thin" locks and debug messages */
    u4          threadId;

    /*
     * Begin interpreter state which does not need to be preserved, but should
     * be located towards the beginning of the Thread structure for
     * efficiency.
     */

    /*
     * interpBreak contains info about the interpreter mode, as well as
     * a count of the number of times the thread has been suspended.  When
     * the count drops to zero, the thread resumes.
     */
    InterpBreak interpBreak;

    /*
     * "dbgSuspendCount" is the portion of the suspend count that the
     * debugger is responsible for.  This has to be tracked separately so
     * that we can recover correctly if the debugger abruptly disconnects
     * (suspendCount -= dbgSuspendCount).  The debugger should not be able
     * to resume GC-suspended threads, because we ignore the debugger while
     * a GC is in progress.
     *
     * Both of these are guarded by gDvm.threadSuspendCountLock.
     *
     * Note the non-debug component will rarely be other than 1 or 0 -- (not
     * sure it's even possible with the way mutexes are currently used.)
     */

    int suspendCount;
    int dbgSuspendCount;

    u1*         cardTable;

    /* current limit of stack; flexes for StackOverflowError */
    const u1*   interpStackEnd;

    /* FP of bottom-most (currently executing) stack frame on interp stack */
    void*       XcurFrame;
    /* current exception, or NULL if nothing pending */
    Object*     exception;

    bool        debugIsMethodEntry;
    /* interpreter stack size; our stacks are fixed-length */
    int         interpStackSize;
    bool        stackOverflowed;

    /* thread handle, as reported by pthread_self() */
    pthread_t   handle;

    /* Assembly interpreter handler tables */
#ifndef DVM_NO_ASM_INTERP
    void*       mainHandlerTable;   // Table of actual instruction handler
    void*       altHandlerTable;    // Table of breakout handlers
#else
    void*       unused0;            // Consume space to keep offsets
    void*       unused1;            //   the same between builds with
#endif

    /*
     * singleStepCount is a countdown timer used with the breakFlag
     * kInterpSingleStep.  If kInterpSingleStep is set in breakFlags,
     * singleStepCount will decremented each instruction execution.
     * Once it reaches zero, the kInterpSingleStep flag in breakFlags
     * will be cleared.  This can be used to temporarily prevent
     * execution from re-entering JIT'd code or force inter-instruction
     * checks by delaying the reset of curHandlerTable to mainHandlerTable.
     */
    int         singleStepCount;

#ifdef WITH_JIT
    struct JitToInterpEntries jitToInterpEntries;
    /*
     * Whether the current top VM frame is in the interpreter or JIT cache:
     *   NULL    : in the interpreter
     *   non-NULL: entry address of the JIT'ed code (the actual value doesn't
     *             matter)
     */
    void*             inJitCodeCache;
    unsigned char*    pJitProfTable;
    int               jitThreshold;
    const void*       jitResumeNPC;     // Translation return point
    const u4*         jitResumeNSP;     // Native SP at return point
    const u2*         jitResumeDPC;     // Dalvik inst following single-step
    JitState    jitState;
    int         icRechainCount;
    const void* pProfileCountdown;
    const ClassObject* callsiteClass;
    const Method*     methodToCall;
#endif

    /* JNI local reference tracking */
    IndirectRefTable jniLocalRefTable;

#if defined(WITH_JIT)
#if defined(WITH_SELF_VERIFICATION)
    /* Buffer for register state during self verification */
    struct ShadowSpace* shadowSpace;
#endif
    int         currTraceRun;
    int         totalTraceLen;  // Number of Dalvik insts in trace
    const u2*   currTraceHead;  // Start of the trace we're building
    const u2*   currRunHead;    // Start of run we're building
    int         currRunLen;     // Length of run in 16-bit words
    const u2*   lastPC;         // Stage the PC for the threaded interpreter
    const Method*  traceMethod; // Starting method of current trace
    intptr_t    threshFilter[JIT_TRACE_THRESH_FILTER_SIZE];
    JitTraceRun trace[MAX_JIT_RUN_LEN];
#endif

    /*
     * Thread's current status.  Can only be changed by the thread itself
     * (i.e. don't mess with this from other threads).
     */
    volatile ThreadStatus status;

    /* thread ID, only useful under Linux */
    pid_t       systemTid;

    /* start (high addr) of interp stack (subtract size to get malloc addr) */
    u1*         interpStackStart;

    /* the java/lang/Thread that we are associated with */
    Object*     threadObj;

    /* the JNIEnv pointer associated with this thread */
    JNIEnv*     jniEnv;

    /* internal reference tracking */
    ReferenceTable  internalLocalRefTable;


    /* JNI native monitor reference tracking (initialized on first use) */
    ReferenceTable  jniMonitorRefTable;

    /* hack to make JNI_OnLoad work right */
    Object*     classLoaderOverride;

    /* mutex to guard the interrupted and the waitMonitor members */
    pthread_mutex_t    waitMutex;

    /* pointer to the monitor lock we're currently waiting on */
    /* guarded by waitMutex */
    /* TODO: consider changing this to Object* for better JDWP interaction */
    Monitor*    waitMonitor;

    /* thread "interrupted" status; stays raised until queried or thrown */
    /* guarded by waitMutex */
    bool        interrupted;

    /* links to the next thread in the wait set this thread is part of */
    struct Thread*     waitNext;

    /* object to sleep on while we are waiting for a monitor */
    pthread_cond_t     waitCond;

    /*
     * Set to true when the thread is in the process of throwing an
     * OutOfMemoryError.
     */
    bool        throwingOOME;

    /* links to rest of thread list; grab global lock before traversing */
    struct Thread* prev;
    struct Thread* next;

    /* used by threadExitCheck when a thread exits without detaching */
    int         threadExitCheckCount;

    /* JDWP invoke-during-breakpoint support */
    DebugInvokeReq  invokeReq;

    /* base time for per-thread CPU timing (used by method profiling) */
    bool        cpuClockBaseSet;
    u8          cpuClockBase;

    /* memory allocation profiling state */
    AllocProfState allocProf;

#ifdef WITH_JNI_STACK_CHECK
    u4          stackCrc;
#endif

#if WITH_EXTRA_GC_CHECKS > 1
    /* PC, saved on every instruction; redundant with StackSaveArea */
    const u2*   currentPc2;
#endif

    /* Safepoint callback state */
    pthread_mutex_t   callbackMutex;
    SafePointCallback callback;
    void*             callbackArg;

#if defined(ARCH_IA32) && defined(WITH_JIT)
    u4 spillRegion[MAX_SPILL_JIT_IA];
#endif
};

/* start point for an internal thread; mimics pthread args */
typedef void* (*InternalThreadStart)(void* arg);

/* args for internal thread creation */
struct InternalStartArgs {
    /* inputs */
    InternalThreadStart func;
    void*       funcArg;
    char*       name;
    Object*     group;
    bool        isDaemon;
    /* result */
    volatile Thread** pThread;
    volatile int*     pCreateStatus;
};

/* finish init */
bool dvmPrepMainForJni(JNIEnv* pEnv);
bool dvmPrepMainThread(void);

/* utility function to get the tid */
pid_t dvmGetSysThreadId(void);

/*
 * Get our Thread* from TLS.
 *
 * Returns NULL if this isn't a thread that the VM is aware of.
 */
Thread* dvmThreadSelf(void);

/* grab the thread list global lock */
void dvmLockThreadList(Thread* self);
/* try to grab the thread list global lock */
bool dvmTryLockThreadList(void);
/* release the thread list global lock */
void dvmUnlockThreadList(void);

/*
 * Thread suspend/resume, used by the GC and debugger.
 */
enum SuspendCause {
    SUSPEND_NOT = 0,
    SUSPEND_FOR_GC,
    SUSPEND_FOR_DEBUG,
    SUSPEND_FOR_DEBUG_EVENT,
    SUSPEND_FOR_STACK_DUMP,
    SUSPEND_FOR_DEX_OPT,
    SUSPEND_FOR_VERIFY,
    SUSPEND_FOR_HPROF,
#if defined(WITH_JIT)
    SUSPEND_FOR_TBL_RESIZE,  // jit-table resize
    SUSPEND_FOR_IC_PATCH,    // polymorphic callsite inline-cache patch
    SUSPEND_FOR_CC_RESET,    // code-cache reset
    SUSPEND_FOR_REFRESH,     // Reload data cached in interpState
#endif
};
void dvmSuspendThread(Thread* thread);
void dvmSuspendSelf(bool jdwpActivity);
void dvmResumeThread(Thread* thread);
void dvmSuspendAllThreads(SuspendCause why);
void dvmResumeAllThreads(SuspendCause why);
void dvmUndoDebuggerSuspensions(void);

/*
 * Check suspend state.  Grab threadListLock before calling.
 */
bool dvmIsSuspended(const Thread* thread);

/*
 * Wait until a thread has suspended.  (Used by debugger support.)
 */
void dvmWaitForSuspend(Thread* thread);

/*
 * Check to see if we should be suspended now.  If so, suspend ourselves
 * by sleeping on a condition variable.
 */
extern "C" bool dvmCheckSuspendPending(Thread* self);

/*
 * Fast test for use in the interpreter.  Returns "true" if our suspend
 * count is nonzero.
 */
INLINE bool dvmCheckSuspendQuick(Thread* self) {
    return (self->interpBreak.ctl.subMode & kSubModeSuspendPending);
}

/*
 * Used when changing thread state.  Threads may only change their own.
 * The "self" argument, which may be NULL, is accepted as an optimization.
 *
 * If you're calling this before waiting on a resource (e.g. THREAD_WAIT
 * or THREAD_MONITOR), do so in the same function as the wait -- this records
 * the current stack depth for the GC.
 *
 * If you're changing to THREAD_RUNNING, this will check for suspension.
 *
 * Returns the old status.
 */
ThreadStatus dvmChangeStatus(Thread* self, ThreadStatus newStatus);

/*
 * Initialize a mutex.
 */
INLINE void dvmInitMutex(pthread_mutex_t* pMutex)
{
#ifdef CHECK_MUTEX
    pthread_mutexattr_t attr;
    int cc;

    pthread_mutexattr_init(&attr);
    cc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
    assert(cc == 0);
    pthread_mutex_init(pMutex, &attr);
    pthread_mutexattr_destroy(&attr);
#else
    pthread_mutex_init(pMutex, NULL);       // default=PTHREAD_MUTEX_FAST_NP
#endif
}

/*
 * Grab a plain mutex.
 */
INLINE void dvmLockMutex(pthread_mutex_t* pMutex)
{
    int cc __attribute__ ((__unused__)) = pthread_mutex_lock(pMutex);
    assert(cc == 0);
}

/*
 * Try grabbing a plain mutex.  Returns 0 if successful.
 */
INLINE int dvmTryLockMutex(pthread_mutex_t* pMutex)
{
    int cc = pthread_mutex_trylock(pMutex);
    assert(cc == 0 || cc == EBUSY);
    return cc;
}

/*
 * Unlock pthread mutex.
 */
INLINE void dvmUnlockMutex(pthread_mutex_t* pMutex)
{
    int cc __attribute__ ((__unused__)) = pthread_mutex_unlock(pMutex);
    assert(cc == 0);
}

/*
 * Destroy a mutex.
 */
INLINE void dvmDestroyMutex(pthread_mutex_t* pMutex)
{
    int cc __attribute__ ((__unused__)) = pthread_mutex_destroy(pMutex);
    assert(cc == 0);
}

INLINE void dvmBroadcastCond(pthread_cond_t* pCond)
{
    int cc __attribute__ ((__unused__)) = pthread_cond_broadcast(pCond);
    assert(cc == 0);
}

INLINE void dvmSignalCond(pthread_cond_t* pCond)
{
    int cc __attribute__ ((__unused__)) = pthread_cond_signal(pCond);
    assert(cc == 0);
}

INLINE void dvmWaitCond(pthread_cond_t* pCond, pthread_mutex_t* pMutex)
{
    int cc __attribute__ ((__unused__)) = pthread_cond_wait(pCond, pMutex);
    assert(cc == 0);
}

/*
 * Create a thread as a result of java.lang.Thread.start().
 */
bool dvmCreateInterpThread(Object* threadObj, int reqStackSize);

/*
 * Create a thread internal to the VM.  It's visible to interpreted code,
 * but found in the "system" thread group rather than "main".
 */
bool dvmCreateInternalThread(pthread_t* pHandle, const char* name,
    InternalThreadStart func, void* funcArg);

/*
 * Attach or detach the current thread from the VM.
 */
bool dvmAttachCurrentThread(const JavaVMAttachArgs* pArgs, bool isDaemon);
void dvmDetachCurrentThread(void);

/*
 * Get the "main" or "system" thread group.
 */
Object* dvmGetMainThreadGroup(void);
Object* dvmGetSystemThreadGroup(void);

/*
 * Given a java/lang/VMThread object, return our Thread.
 */
Thread* dvmGetThreadFromThreadObject(Object* vmThreadObj);

/*
 * Given a pthread handle, return the associated Thread*.
 * Caller must hold the thread list lock.
 *
 * Returns NULL if the thread was not found.
 */
Thread* dvmGetThreadByHandle(pthread_t handle);

/*
 * Given a thread ID, return the associated Thread*.
 * Caller must hold the thread list lock.
 *
 * Returns NULL if the thread was not found.
 */
Thread* dvmGetThreadByThreadId(u4 threadId);

/*
 * Sleep in a thread.  Returns when the sleep timer returns or the thread
 * is interrupted.
 */
void dvmThreadSleep(u8 msec, u4 nsec);

/*
 * Get the name of a thread.
 *
 * For correctness, the caller should hold the thread list lock to ensure
 * that the thread doesn't go away mid-call.
 */
std::string dvmGetThreadName(Thread* thread);

/*
 * Convert ThreadStatus to a string.
 */
const char* dvmGetThreadStatusStr(ThreadStatus status);

/*
 * Return true if a thread is on the internal list.  If it is, the
 * thread is part of the GC's root set.
 */
bool dvmIsOnThreadList(const Thread* thread);

/*
 * Get/set the JNIEnv field.
 */
INLINE JNIEnv* dvmGetThreadJNIEnv(Thread* self) { return self->jniEnv; }
INLINE void dvmSetThreadJNIEnv(Thread* self, JNIEnv* env) { self->jniEnv = env;}

/*
 * Update the priority value of the underlying pthread.
 */
void dvmChangeThreadPriority(Thread* thread, int newPriority);

/* "change flags" values for raise/reset thread priority calls */
#define kChangedPriority    0x01
#define kChangedPolicy      0x02

/*
 * If necessary, raise the thread's priority to nice=0 cgroup=fg.
 *
 * Returns bit flags indicating changes made (zero if nothing was done).
 */
int dvmRaiseThreadPriorityIfNeeded(Thread* thread, int* pSavedThreadPrio,
    SchedPolicy* pSavedThreadPolicy);

/*
 * Drop the thread priority to what it was before an earlier call to
 * dvmRaiseThreadPriorityIfNeeded().
 */
void dvmResetThreadPriority(Thread* thread, int changeFlags,
    int savedThreadPrio, SchedPolicy savedThreadPolicy);

/*
 * Debug: dump information about a single thread.
 */
void dvmDumpThread(Thread* thread, bool isRunning);
void dvmDumpThreadEx(const DebugOutputTarget* target, Thread* thread,
    bool isRunning);

/*
 * Debug: dump information about all threads.
 */
void dvmDumpAllThreads(bool grabLock);
void dvmDumpAllThreadsEx(const DebugOutputTarget* target, bool grabLock);

/*
 * Debug: kill a thread to get a debuggerd stack trace.  Leaves the VM
 * in an uncertain state.
 */
void dvmNukeThread(Thread* thread);

#endif  // DALVIK_THREAD_H_
