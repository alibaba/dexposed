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
 * Exception handling.
 */
#ifndef DALVIK_EXCEPTION_H_
#define DALVIK_EXCEPTION_H_

/*
 * Create a Throwable and throw an exception in the current thread (where
 * "throwing" just means "set the thread's exception pointer").
 *
 * "msg" and/or "cause" may be NULL.
 *
 * If we have a bad exception hierarchy -- something in Throwable.<init>
 * is missing -- then every attempt to throw an exception will result
 * in another exception.  Exceptions are generally allowed to "chain"
 * to other exceptions, so it's hard to auto-detect this problem.  It can
 * only happen if the system classes are broken, so it's probably not
 * worth spending cycles to detect it.
 *
 * We do have one case to worry about: if the classpath is completely
 * wrong, we'll go into a death spin during startup because we can't find
 * the initial class and then we can't find NoClassDefFoundError.  We have
 * to handle this case.
 */
void dvmThrowChainedException(ClassObject* exceptionClass,
    const char* msg, Object* cause);
INLINE void dvmThrowException(ClassObject* exceptionClass,
    const char* msg)
{
    dvmThrowChainedException(exceptionClass, msg, NULL);
}

/*
 * Like dvmThrowException, but takes printf-style args for the message.
 */
void dvmThrowExceptionFmtV(ClassObject* exceptionClass,
    const char* fmt, va_list args);
void dvmThrowExceptionFmt(ClassObject* exceptionClass,
    const char* fmt, ...)
#if defined(__GNUC__)
    __attribute__ ((format(printf, 2, 3)))
#endif
    ;
INLINE void dvmThrowExceptionFmt(ClassObject* exceptionClass,
    const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    dvmThrowExceptionFmtV(exceptionClass, fmt, args);
    va_end(args);
}

/*
 * Like dvmThrowChainedException, but take a class object
 * instead of a name and turn the given message into the
 * human-readable form for a descriptor.
 */
void dvmThrowChainedExceptionWithClassMessage(
    ClassObject* exceptionClass, const char* messageDescriptor,
    Object* cause);

/*
 * Like dvmThrowException, but take a class object instead of a name
 * and turn the given message into the human-readable form for a descriptor.
 */
INLINE void dvmThrowExceptionWithClassMessage(
    ClassObject* exceptionClass, const char* messageDescriptor)
{
    dvmThrowChainedExceptionWithClassMessage(exceptionClass,
            messageDescriptor, NULL);
}

/*
 * Return the exception being thrown in the current thread, or NULL if
 * no exception is pending.
 */
INLINE Object* dvmGetException(Thread* self) {
    return self->exception;
}

/*
 * Set the exception being thrown in the current thread.
 */
INLINE void dvmSetException(Thread* self, Object* exception)
{
    assert(exception != NULL);
    self->exception = exception;
}

/*
 * Clear the pending exception.
 *
 * (We use this rather than "set(null)" because we may need to have special
 * fixups here for StackOverflowError stuff.  Calling "clear" in the code
 * makes it obvious.)
 */
INLINE void dvmClearException(Thread* self) {
    self->exception = NULL;
}

/*
 * Clear the pending exception.  Used by the optimization and verification
 * code, which has to run with "initializing" set to avoid going into a
 * death-spin if the "class not found" exception can't be found.
 */
void dvmClearOptException(Thread* self);

/*
 * Returns "true" if an exception is pending.  Use this if you have a
 * "self" pointer.
 */
INLINE bool dvmCheckException(Thread* self) {
    return (self->exception != NULL);
}

/*
 * Returns "true" if this is a "checked" exception, i.e. it's a subclass
 * of Throwable (assumed) but not a subclass of RuntimeException or Error.
 */
bool dvmIsCheckedException(const Object* exception);

/*
 * Wrap the now-pending exception in a different exception.
 *
 * If something fails, an (unchecked) exception related to that failure
 * will be pending instead.
 */
void dvmWrapException(const char* newExcepStr);

/*
 * Get the "cause" field from an exception.
 *
 * Returns NULL if the field is null or uninitialized.
 */
Object* dvmGetExceptionCause(const Object* exception);

/*
 * Print the exception stack trace on stderr.  Calls the exception's
 * print function.
 */
void dvmPrintExceptionStackTrace(void);

/*
 * Print the exception stack trace to the log file.  The exception stack
 * trace is computed within the VM.
 */
void dvmLogExceptionStackTrace(void);

/*
 * Search for a catch block that matches "exception".
 *
 * "*newFrame" gets a copy of the new frame pointer.
 *
 * If "doUnroll" is set, we unroll "thread"s stack as we go (and update
 * self->interpSave.curFrame with the same value as in *newFrame).
 *
 * Returns the offset to the catch code on success, or -1 if we couldn't
 * find a catcher.
 */
extern "C" int dvmFindCatchBlock(Thread* self, int relPc, Object* exception,
    bool doUnroll, void** newFrame);

/*
 * Support for saving exception stack traces and converting them to
 * usable form.  Use the "FillIn" function to generate a compact array
 * that represents the stack frames, then "GetStackTrace" to convert it
 * to an array of StackTraceElement objects.
 *
 * Don't call the "Internal" form of the function directly.
 */
void* dvmFillInStackTraceInternal(Thread* thread, bool wantObject, size_t* pCount);
/* return an [I for use by interpreted code */
INLINE Object* dvmFillInStackTrace(Thread* thread) {
    return (Object*) dvmFillInStackTraceInternal(thread, true, NULL);
}
ArrayObject* dvmGetStackTrace(const Object* stackState);
/* return an int* and array count; caller must free() the return value */
INLINE int* dvmFillInStackTraceRaw(Thread* thread, size_t* pCount) {
    return (int*) dvmFillInStackTraceInternal(thread, false, pCount);
}
ArrayObject* dvmGetStackTraceRaw(const int* intVals, size_t stackDepth);
void dvmFillStackTraceElements(const int* intVals, size_t stackDepth, ArrayObject* steArray);

/*
 * Print a formatted version of a raw stack trace to the log file.
 */
void dvmLogRawStackTrace(const int* intVals, int stackDepth);

/**
 * Throw an AbstractMethodError in the current thread, with the given detail
 * message.
 */
void dvmThrowAbstractMethodError(const char* msg);

/**
 * Throw an ArithmeticException in the current thread, with the given detail
 * message.
 */
extern "C" void dvmThrowArithmeticException(const char* msg);

/*
 * Throw an ArrayIndexOutOfBoundsException in the current thread,
 * using the given array length and index in the detail message.
 */
extern "C" void dvmThrowArrayIndexOutOfBoundsException(int length, int index);

/*
 * Throw an ArrayStoreException in the current thread, using the given classes'
 * names in the detail message, indicating that an object of the given type
 * can't be stored into an array of the given type.
 */
extern "C" void dvmThrowArrayStoreExceptionIncompatibleElement(ClassObject* objectType, ClassObject* arrayType);

/*
 * Throw an ArrayStoreException in the current thread, using the given
 * class name and argument label in the detail message, indicating
 * that it is not an array.
 */
void dvmThrowArrayStoreExceptionNotArray(ClassObject* actual, const char* label);

/*
 * Throw an ArrayStoreException in the current thread, using the given
 * classes' names in the detail message, indicating that the arrays
 * aren't compatible (for copying contents).
 */
void dvmThrowArrayStoreExceptionIncompatibleArrays(ClassObject* source, ClassObject* destination);

/*
 * Throw an ArrayStoreException in the current thread, using the given
 * index and classes' names in the detail message, indicating that the
 * object at the given index and of the given type cannot be stored
 * into an array of the given type.
 */
void dvmThrowArrayStoreExceptionIncompatibleArrayElement(s4 index, ClassObject* objectType,
        ClassObject* arrayType);

/**
 * Throw a ClassCastException in the current thread, using the given classes'
 * names in the detail message.
 */
extern "C" void dvmThrowClassCastException(ClassObject* actual, ClassObject* desired);

/**
 * Throw a ClassCircularityError in the current thread, with the
 * human-readable form of the given descriptor as the detail message.
 */
void dvmThrowClassCircularityError(const char* descriptor);

/**
 * Throw a ClassFormatError in the current thread, with the given
 * detail message.
 */
void dvmThrowClassFormatError(const char* msg);

/**
 * Throw a ClassNotFoundException in the current thread, with the given
 * class name as the detail message.
 */
void dvmThrowClassNotFoundException(const char* name);

/**
 * Throw a ClassNotFoundException in the current thread, with the given
 * cause, and the given class name as the detail message.
 */
void dvmThrowChainedClassNotFoundException(const char* name, Object* cause);

/*
 * Throw the VM-spec-mandated error when an exception is thrown during
 * class initialization. Unlike other helper functions, this automatically
 * wraps the current thread's pending exception.
 */
void dvmThrowExceptionInInitializerError(void);

/**
 * Throw a FileNotFoundException in the current thread, with the given
 * detail message.
 */
void dvmThrowFileNotFoundException(const char* msg);

/**
 * Throw an IOException in the current thread, with the given
 * detail message.
 */
void dvmThrowIOException(const char* msg);

/**
 * Throw an IllegalAccessError in the current thread, with the
 * given detail message.
 */
void dvmThrowIllegalAccessError(const char* msg);

/**
 * Throw an IllegalAccessException in the current thread, with the
 * given detail message.
 */
void dvmThrowIllegalAccessException(const char* msg);

/**
 * Throw an IllegalArgumentException in the current thread, with the
 * given detail message.
 */
void dvmThrowIllegalArgumentException(const char* msg);

/**
 * Throw an IllegalMonitorStateException in the current thread, with
 * the given detail message.
 */
void dvmThrowIllegalMonitorStateException(const char* msg);

/**
 * Throw an IllegalStateException in the current thread, with
 * the given detail message.
 */
void dvmThrowIllegalStateException(const char* msg);

/**
 * Throw an IllegalThreadStateException in the current thread, with
 * the given detail message.
 */
void dvmThrowIllegalThreadStateException(const char* msg);

/**
 * Throw an IncompatibleClassChangeError in the current thread,
 * the given detail message.
 */
void dvmThrowIncompatibleClassChangeError(const char* msg);

/**
 * Throw an IncompatibleClassChangeError in the current thread, with the
 * human-readable form of the given descriptor as the detail message.
 */
void dvmThrowIncompatibleClassChangeErrorWithClassMessage(
        const char* descriptor);

/**
 * Throw an InstantiationException in the current thread, with
 * the human-readable form of the given class as the detail message,
 * with optional extra detail appended to the message.
 */
void dvmThrowInstantiationException(ClassObject* clazz,
        const char* extraDetail);

/**
 * Throw an InternalError in the current thread, with the given
 * detail message.
 */
extern "C" void dvmThrowInternalError(const char* msg);

/**
 * Throw an InterruptedException in the current thread, with the given
 * detail message.
 */
void dvmThrowInterruptedException(const char* msg);

/**
 * Throw a LinkageError in the current thread, with the
 * given detail message.
 */
void dvmThrowLinkageError(const char* msg);

/**
 * Throw a NegativeArraySizeException in the current thread, with the
 * given number as the detail message.
 */
extern "C" void dvmThrowNegativeArraySizeException(s4 size);

/**
 * Throw a NoClassDefFoundError in the current thread, with the
 * human-readable form of the given descriptor as the detail message.
 */
extern "C" void dvmThrowNoClassDefFoundError(const char* descriptor);

/**
 * Throw a NoClassDefFoundError in the current thread, with the given
 * cause, and the human-readable form of the given descriptor as the
 * detail message.
 */
void dvmThrowChainedNoClassDefFoundError(const char* descriptor,
        Object* cause);

/**
 * Throw a NoSuchFieldError in the current thread, with the given
 * detail message.
 */
extern "C" void dvmThrowNoSuchFieldError(const char* msg);

/**
 * Throw a NoSuchFieldException in the current thread, with the given
 * detail message.
 */
void dvmThrowNoSuchFieldException(const char* msg);

/**
 * Throw a NoSuchMethodError in the current thread, with the given
 * detail message.
 */
extern "C" void dvmThrowNoSuchMethodError(const char* msg);

/**
 * Throw a NullPointerException in the current thread, with the given
 * detail message.
 */
extern "C" void dvmThrowNullPointerException(const char* msg);

/**
 * Throw an OutOfMemoryError in the current thread, with the given
 * detail message.
 */
void dvmThrowOutOfMemoryError(const char* msg);

/**
 * Throw a RuntimeException in the current thread, with the given detail
 * message.
 */
void dvmThrowRuntimeException(const char* msg);

/**
 * Throw a StaleDexCacheError in the current thread, with
 * the given detail message.
 */
void dvmThrowStaleDexCacheError(const char* msg);

/**
 * Throw a StringIndexOutOfBoundsException in the current thread, with
 * a detail message specifying an actual length as well as a requested
 * index.
 */
void dvmThrowStringIndexOutOfBoundsExceptionWithIndex(jsize stringLength,
        jsize requestIndex);

/**
 * Throw a StringIndexOutOfBoundsException in the current thread, with
 * a detail message specifying an actual length as well as a requested
 * region.
 */
void dvmThrowStringIndexOutOfBoundsExceptionWithRegion(jsize stringLength,
        jsize requestStart, jsize requestLength);

/**
 * Throw a TypeNotPresentException in the current thread, with the
 * human-readable form of the given descriptor as the detail message.
 */
void dvmThrowTypeNotPresentException(const char* descriptor);

/**
 * Throw an UnsatisfiedLinkError in the current thread, with
 * the given detail message.
 */
void dvmThrowUnsatisfiedLinkError(const char* msg);
void dvmThrowUnsatisfiedLinkError(const char* msg, const Method* method);

/**
 * Throw an UnsupportedOperationException in the current thread, with
 * the given detail message.
 */
void dvmThrowUnsupportedOperationException(const char* msg);

/**
 * Throw a VerifyError in the current thread, with the
 * human-readable form of the given descriptor as the detail message.
 */
void dvmThrowVerifyError(const char* descriptor);

/**
 * Throw a VirtualMachineError in the current thread, with
 * the given detail message.
 */
void dvmThrowVirtualMachineError(const char* msg);

#endif  // DALVIK_EXCEPTION_H_
