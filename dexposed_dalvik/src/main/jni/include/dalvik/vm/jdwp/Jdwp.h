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
 * JDWP "public" interface.  The main body of the VM should only use JDWP
 * structures and functions declared here.
 *
 * The JDWP code follows the DalvikVM rules for naming conventions, but
 * attempts to remain independent of VM innards (e.g. it doesn't access VM
 * data structures directly).  All calls go through Debugger.c.
 */
#ifndef DALVIK_JDWP_JDWP_H_
#define DALVIK_JDWP_JDWP_H_

#include "jdwp/JdwpConstants.h"
#include "jdwp/ExpandBuf.h"
#include "Common.h"
#include "Bits.h"
#include <pthread.h>

struct JdwpState;       /* opaque */

/*
 * Fundamental types.
 *
 * ObjectId and RefTypeId must be the same size.
 */
typedef u4 FieldId;     /* static or instance field */
typedef u4 MethodId;    /* any kind of method, including constructors */
typedef u8 ObjectId;    /* any object (threadID, stringID, arrayID, etc) */
typedef u8 RefTypeId;   /* like ObjectID, but unique for Class objects */
typedef u8 FrameId;     /* short-lived stack frame ID */

/*
 * Match these with the type sizes.  This way we don't have to pass
 * a value and a length.
 */
INLINE FieldId dvmReadFieldId(const u1** pBuf)      { return read4BE(pBuf); }
INLINE MethodId dvmReadMethodId(const u1** pBuf)    { return read4BE(pBuf); }
INLINE ObjectId dvmReadObjectId(const u1** pBuf)    { return read8BE(pBuf); }
INLINE RefTypeId dvmReadRefTypeId(const u1** pBuf)  { return read8BE(pBuf); }
INLINE FrameId dvmReadFrameId(const u1** pBuf)      { return read8BE(pBuf); }
INLINE void dvmSetFieldId(u1* buf, FieldId val)     { return set4BE(buf, val); }
INLINE void dvmSetMethodId(u1* buf, MethodId val)   { return set4BE(buf, val); }
INLINE void dvmSetObjectId(u1* buf, ObjectId val)   { return set8BE(buf, val); }
INLINE void dvmSetRefTypeId(u1* buf, RefTypeId val) { return set8BE(buf, val); }
INLINE void dvmSetFrameId(u1* buf, FrameId val)     { return set8BE(buf, val); }
INLINE void expandBufAddFieldId(ExpandBuf* pReply, FieldId id) {
    expandBufAdd4BE(pReply, id);
}
INLINE void expandBufAddMethodId(ExpandBuf* pReply, MethodId id) {
    expandBufAdd4BE(pReply, id);
}
INLINE void expandBufAddObjectId(ExpandBuf* pReply, ObjectId id) {
    expandBufAdd8BE(pReply, id);
}
INLINE void expandBufAddRefTypeId(ExpandBuf* pReply, RefTypeId id) {
    expandBufAdd8BE(pReply, id);
}
INLINE void expandBufAddFrameId(ExpandBuf* pReply, FrameId id) {
    expandBufAdd8BE(pReply, id);
}


/*
 * Holds a JDWP "location".
 */
struct JdwpLocation {
    u1          typeTag;        /* class or interface? */
    RefTypeId   classId;        /* method->clazz */
    MethodId    methodId;       /* method in which "idx" resides */
    u8          idx;            /* relative index into code block */
};

/*
 * How we talk to the debugger.
 */
enum JdwpTransportType {
    kJdwpTransportUnknown = 0,
    kJdwpTransportSocket,       /* transport=dt_socket */
    kJdwpTransportAndroidAdb,   /* transport=dt_android_adb */
};

/*
 * Holds collection of JDWP initialization parameters.
 */
struct JdwpStartupParams {
    JdwpTransportType transport;
    bool        server;
    bool        suspend;
    char        host[64];
    short       port;
    /* more will be here someday */
};

/*
 * Perform one-time initialization.
 *
 * Among other things, this binds to a port to listen for a connection from
 * the debugger.
 *
 * Returns a newly-allocated JdwpState struct on success, or NULL on failure.
 */
JdwpState* dvmJdwpStartup(const JdwpStartupParams* params);

/*
 * Shut everything down.
 */
void dvmJdwpShutdown(JdwpState* state);

/*
 * Returns "true" if a debugger or DDM is connected.
 */
bool dvmJdwpIsActive(JdwpState* state);

/*
 * Return the debugger thread's handle, or 0 if the debugger thread isn't
 * running.
 */
pthread_t dvmJdwpGetDebugThread(JdwpState* state);

/*
 * Get time, in milliseconds, since the last debugger activity.
 */
s8 dvmJdwpLastDebuggerActivity(JdwpState* state);

/*
 * When we hit a debugger event that requires suspension, it's important
 * that we wait for the thread to suspend itself before processing any
 * additional requests.  (Otherwise, if the debugger immediately sends a
 * "resume thread" command, the resume might arrive before the thread has
 * suspended itself.)
 *
 * The thread should call the "set" function before sending the event to
 * the debugger.  The main JDWP handler loop calls "get" before processing
 * an event, and will wait for thread suspension if it's set.  Once the
 * thread has suspended itself, the JDWP handler calls "clear" and
 * continues processing the current event.  This works in the suspend-all
 * case because the event thread doesn't suspend itself until everything
 * else has suspended.
 *
 * It's possible that multiple threads could encounter thread-suspending
 * events at the same time, so we grab a mutex in the "set" call, and
 * release it in the "clear" call.
 */
//ObjectId dvmJdwpGetWaitForEventThread(JdwpState* state);
void dvmJdwpSetWaitForEventThread(JdwpState* state, ObjectId threadId);
void dvmJdwpClearWaitForEventThread(JdwpState* state);

/*
 * Network functions.
 */
bool dvmJdwpCheckConnection(JdwpState* state);
bool dvmJdwpAcceptConnection(JdwpState* state);
bool dvmJdwpEstablishConnection(JdwpState* state);
void dvmJdwpCloseConnection(JdwpState* state);
bool dvmJdwpProcessIncoming(JdwpState* state);


/*
 * These notify the debug code that something interesting has happened.  This
 * could be a thread starting or ending, an exception, or an opportunity
 * for a breakpoint.  These calls do not mean that an event the debugger
 * is interested has happened, just that something has happened that the
 * debugger *might* be interested in.
 *
 * The item of interest may trigger multiple events, some or all of which
 * are grouped together in a single response.
 *
 * The event may cause the current thread or all threads (except the
 * JDWP support thread) to be suspended.
 */

/*
 * The VM has finished initializing.  Only called when the debugger is
 * connected at the time initialization completes.
 */
bool dvmJdwpPostVMStart(JdwpState* state, bool suspend);

/*
 * A location of interest has been reached.  This is used for breakpoints,
 * single-stepping, and method entry/exit.  (JDWP requires that these four
 * events are grouped together in a single response.)
 *
 * In some cases "*pLoc" will just have a method and class name, e.g. when
 * issuing a MethodEntry on a native method.
 *
 * "eventFlags" indicates the types of events that have occurred.
 */
bool dvmJdwpPostLocationEvent(JdwpState* state, const JdwpLocation* pLoc,
    ObjectId thisPtr, int eventFlags);

/*
 * An exception has been thrown.
 *
 * Pass in a zeroed-out "*pCatchLoc" if the exception wasn't caught.
 */
bool dvmJdwpPostException(JdwpState* state, const JdwpLocation* pThrowLoc,
    ObjectId excepId, RefTypeId excepClassId, const JdwpLocation* pCatchLoc,
    ObjectId thisPtr);

/*
 * A thread has started or stopped.
 */
bool dvmJdwpPostThreadChange(JdwpState* state, ObjectId threadId, bool start);

/*
 * Class has been prepared.
 */
bool dvmJdwpPostClassPrepare(JdwpState* state, int tag, RefTypeId refTypeId,
    const char* signature, int status);

/*
 * The VM is about to stop.
 */
bool dvmJdwpPostVMDeath(JdwpState* state);

/*
 * Send up a chunk of DDM data.
 */
void dvmJdwpDdmSendChunkV(JdwpState* state, int type, const struct iovec* iov,
    int iovcnt);

#endif  // DALVIK_JDWP_JDWP_H_
