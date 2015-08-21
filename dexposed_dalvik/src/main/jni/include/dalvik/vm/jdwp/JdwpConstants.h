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
 * These come out of the JDWP documentation.
 */
#ifndef DALVIK_JDWP_JDWPCONSTANTS_H_
#define DALVIK_JDWP_JDWPCONSTANTS_H_

/*
 * Error constants.
 */
enum JdwpError {
    ERR_NONE                                        = 0,
    ERR_INVALID_THREAD                              = 10,
    ERR_INVALID_THREAD_GROUP                        = 11,
    ERR_INVALID_PRIORITY                            = 12,
    ERR_THREAD_NOT_SUSPENDED                        = 13,
    ERR_THREAD_SUSPENDED                            = 14,
    ERR_INVALID_OBJECT                              = 20,
    ERR_INVALID_CLASS                               = 21,
    ERR_CLASS_NOT_PREPARED                          = 22,
    ERR_INVALID_METHODID                            = 23,
    ERR_INVALID_LOCATION                            = 24,
    ERR_INVALID_FIELDID                             = 25,
    ERR_INVALID_FRAMEID                             = 30,
    ERR_NO_MORE_FRAMES                              = 31,
    ERR_OPAQUE_FRAME                                = 32,
    ERR_NOT_CURRENT_FRAME                           = 33,
    ERR_TYPE_MISMATCH                               = 34,
    ERR_INVALID_SLOT                                = 35,
    ERR_DUPLICATE                                   = 40,
    ERR_NOT_FOUND                                   = 41,
    ERR_INVALID_MONITOR                             = 50,
    ERR_NOT_MONITOR_OWNER                           = 51,
    ERR_INTERRUPT                                   = 52,
    ERR_INVALID_CLASS_FORMAT                        = 60,
    ERR_CIRCULAR_CLASS_DEFINITION                   = 61,
    ERR_FAILS_VERIFICATION                          = 62,
    ERR_ADD_METHOD_NOT_IMPLEMENTED                  = 63,
    ERR_SCHEMA_CHANGE_NOT_IMPLEMENTED               = 64,
    ERR_INVALID_TYPESTATE                           = 65,
    ERR_HIERARCHY_CHANGE_NOT_IMPLEMENTED            = 66,
    ERR_DELETE_METHOD_NOT_IMPLEMENTED               = 67,
    ERR_UNSUPPORTED_VERSION                         = 68,
    ERR_NAMES_DONT_MATCH                            = 69,
    ERR_CLASS_MODIFIERS_CHANGE_NOT_IMPLEMENTED      = 70,
    ERR_METHOD_MODIFIERS_CHANGE_NOT_IMPLEMENTED     = 71,
    ERR_NOT_IMPLEMENTED                             = 99,
    ERR_NULL_POINTER                                = 100,
    ERR_ABSENT_INFORMATION                          = 101,
    ERR_INVALID_EVENT_TYPE                          = 102,
    ERR_ILLEGAL_ARGUMENT                            = 103,
    ERR_OUT_OF_MEMORY                               = 110,
    ERR_ACCESS_DENIED                               = 111,
    ERR_VM_DEAD                                     = 112,
    ERR_INTERNAL                                    = 113,
    ERR_UNATTACHED_THREAD                           = 115,
    ERR_INVALID_TAG                                 = 500,
    ERR_ALREADY_INVOKING                            = 502,
    ERR_INVALID_INDEX                               = 503,
    ERR_INVALID_LENGTH                              = 504,
    ERR_INVALID_STRING                              = 506,
    ERR_INVALID_CLASS_LOADER                        = 507,
    ERR_INVALID_ARRAY                               = 508,
    ERR_TRANSPORT_LOAD                              = 509,
    ERR_TRANSPORT_INIT                              = 510,
    ERR_NATIVE_METHOD                               = 511,
    ERR_INVALID_COUNT                               = 512,
};
const char* dvmJdwpErrorStr(JdwpError error);


/*
 * ClassStatus constants.  These are bit flags that can be ORed together.
 */
enum JdwpClassStatus {
    CS_VERIFIED             = 0x01,
    CS_PREPARED             = 0x02,
    CS_INITIALIZED          = 0x04,
    CS_ERROR                = 0x08,
};

/*
 * EventKind constants.
 */
enum JdwpEventKind {
    EK_SINGLE_STEP          = 1,
    EK_BREAKPOINT           = 2,
    EK_FRAME_POP            = 3,
    EK_EXCEPTION            = 4,
    EK_USER_DEFINED         = 5,
    EK_THREAD_START         = 6,
    EK_THREAD_END           = 7,
    EK_CLASS_PREPARE        = 8,
    EK_CLASS_UNLOAD         = 9,
    EK_CLASS_LOAD           = 10,
    EK_FIELD_ACCESS         = 20,
    EK_FIELD_MODIFICATION   = 21,
    EK_EXCEPTION_CATCH      = 30,
    EK_METHOD_ENTRY         = 40,
    EK_METHOD_EXIT          = 41,
    EK_VM_INIT              = 90,
    EK_VM_DEATH             = 99,
    EK_VM_DISCONNECTED      = 100,  /* "Never sent across JDWP */
    EK_VM_START             = EK_VM_INIT,
    EK_THREAD_DEATH         = EK_THREAD_END,
};
const char* dvmJdwpEventKindStr(JdwpEventKind kind);

/*
 * Values for "modKind" in EventRequest.Set.
 */
enum JdwpModKind {
    MK_COUNT                = 1,
    MK_CONDITIONAL          = 2,
    MK_THREAD_ONLY          = 3,
    MK_CLASS_ONLY           = 4,
    MK_CLASS_MATCH          = 5,
    MK_CLASS_EXCLUDE        = 6,
    MK_LOCATION_ONLY        = 7,
    MK_EXCEPTION_ONLY       = 8,
    MK_FIELD_ONLY           = 9,
    MK_STEP                 = 10,
    MK_INSTANCE_ONLY        = 11,
};
const char* dvmJdwpModKindStr(JdwpModKind kind);

/*
 * InvokeOptions constants (bit flags).
 */
enum JdwpInvokeOptions {
    INVOKE_SINGLE_THREADED  = 0x01,
    INVOKE_NONVIRTUAL       = 0x02,
};

/*
 * StepDepth constants.
 */
enum JdwpStepDepth {
    SD_INTO                 = 0,    /* step into method calls */
    SD_OVER                 = 1,    /* step over method calls */
    SD_OUT                  = 2,    /* step out of current method */
};
const char* dvmJdwpStepDepthStr(JdwpStepDepth depth);

/*
 * StepSize constants.
 */
enum JdwpStepSize {
    SS_MIN                  = 0,    /* step by minimum (e.g. 1 bytecode inst) */
    SS_LINE                 = 1,    /* if possible, step to next line */
};
const char* dvmJdwpStepSizeStr(JdwpStepSize size);

/*
 * SuspendPolicy constants.
 */
enum JdwpSuspendPolicy {
    SP_NONE                 = 0,    /* suspend no threads */
    SP_EVENT_THREAD         = 1,    /* suspend event thread */
    SP_ALL                  = 2,    /* suspend all threads */
};
const char* dvmJdwpSuspendPolicyStr(JdwpSuspendPolicy policy);

/*
 * SuspendStatus constants.
 */
enum JdwpSuspendStatus {
    SUSPEND_STATUS_NOT_SUSPENDED = 0,
    SUSPEND_STATUS_SUSPENDED     = 1,
};
const char* dvmJdwpSuspendStatusStr(JdwpSuspendStatus status);

/*
 * ThreadStatus constants.
 */
enum JdwpThreadStatus {
    TS_ZOMBIE               = 0,
    TS_RUNNING              = 1,        // RUNNING
    TS_SLEEPING             = 2,        // (in Thread.sleep())
    TS_MONITOR              = 3,        // WAITING (monitor wait)
    TS_WAIT                 = 4,        // (in Object.wait())
};
const char* dvmJdwpThreadStatusStr(JdwpThreadStatus status);

/*
 * TypeTag constants.
 */
enum JdwpTypeTag {
    TT_CLASS                = 1,
    TT_INTERFACE            = 2,
    TT_ARRAY                = 3,
};

/*
 * Tag constants.
 */
enum JdwpType {
    JT_ARRAY                 = '[',
    JT_BYTE                  = 'B',
    JT_CHAR                  = 'C',
    JT_OBJECT                = 'L',
    JT_FLOAT                 = 'F',
    JT_DOUBLE                = 'D',
    JT_INT                   = 'I',
    JT_LONG                  = 'J',
    JT_SHORT                 = 'S',
    JT_VOID                  = 'V',
    JT_BOOLEAN               = 'Z',
    JT_STRING                = 's',
    JT_THREAD                = 't',
    JT_THREAD_GROUP          = 'g',
    JT_CLASS_LOADER          = 'l',
    JT_CLASS_OBJECT          = 'c',
};

#endif  // DALVIK_JDWP_JDWPCONSTANTS_H_
