/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#include <time.h>
#include <signal.h>
#include <sched.h>
#include <limits.h>
#include <sys/types.h>

#if defined(__LP64__)
  #define __RESERVED_INITIALIZER , {0}
#else
  #define __RESERVED_INITIALIZER
#endif

typedef struct {
  int volatile value;
#ifdef __LP64__
  char __reserved[36];
#endif
} pthread_mutex_t;

#define  __PTHREAD_MUTEX_INIT_VALUE            0
#define  __PTHREAD_RECURSIVE_MUTEX_INIT_VALUE  0x4000
#define  __PTHREAD_ERRORCHECK_MUTEX_INIT_VALUE 0x8000

#define  PTHREAD_MUTEX_INITIALIZER             {__PTHREAD_MUTEX_INIT_VALUE __RESERVED_INITIALIZER}
#define  PTHREAD_RECURSIVE_MUTEX_INITIALIZER   {__PTHREAD_RECURSIVE_MUTEX_INIT_VALUE __RESERVED_INITIALIZER}
#define  PTHREAD_ERRORCHECK_MUTEX_INITIALIZER  {__PTHREAD_ERRORCHECK_MUTEX_INIT_VALUE __RESERVED_INITIALIZER}

enum {
    PTHREAD_MUTEX_NORMAL = 0,
    PTHREAD_MUTEX_RECURSIVE = 1,
    PTHREAD_MUTEX_ERRORCHECK = 2,

    PTHREAD_MUTEX_ERRORCHECK_NP = PTHREAD_MUTEX_ERRORCHECK,
    PTHREAD_MUTEX_RECURSIVE_NP  = PTHREAD_MUTEX_RECURSIVE,

    PTHREAD_MUTEX_DEFAULT = PTHREAD_MUTEX_NORMAL
};

typedef struct {
  int volatile value;
#ifdef __LP64__
  char __reserved[44];
#endif
} pthread_cond_t;

#define PTHREAD_COND_INITIALIZER  {0 __RESERVED_INITIALIZER}

typedef struct {
  uint32_t flags;
  void* stack_base;
  size_t stack_size;
  size_t guard_size;
  int32_t sched_policy;
  int32_t sched_priority;
#ifdef __LP64__
  char __reserved[16];
#endif
} pthread_attr_t;

typedef long pthread_mutexattr_t;
typedef long pthread_condattr_t;

typedef long pthread_rwlockattr_t;

typedef struct {
#if !defined(__LP64__)
  pthread_mutex_t __unused_lock;
  pthread_cond_t __unused_cond;
#endif
  volatile int32_t state; // 0=unlock, -1=writer lock, +n=reader lock
  volatile int32_t writer_thread_id;
  volatile int32_t pending_readers;
  volatile int32_t pending_writers;
  int32_t attr;
#ifdef __LP64__
  char __reserved[36];
#else
  char __reserved[12];
#endif

} pthread_rwlock_t;

#ifdef __LP64__
  #define PTHREAD_RWLOCK_INITIALIZER  { 0, 0, 0, 0, 0, { 0 } }
#else
  #define PTHREAD_RWLOCK_INITIALIZER  { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, 0, 0, 0, { 0 } }
#endif

typedef int pthread_key_t;
typedef long pthread_t;

typedef volatile int pthread_once_t;

#define PTHREAD_ONCE_INIT 0

#if defined(__LP64__)
#define PTHREAD_STACK_MIN (4 * PAGE_SIZE)
#else
#define PTHREAD_STACK_MIN (2 * PAGE_SIZE)
#endif

#define PTHREAD_CREATE_DETACHED  0x00000001
#define PTHREAD_CREATE_JOINABLE  0x00000000

#define PTHREAD_PROCESS_PRIVATE  0
#define PTHREAD_PROCESS_SHARED   1

#define PTHREAD_SCOPE_SYSTEM     0
#define PTHREAD_SCOPE_PROCESS    1

__BEGIN_DECLS

int pthread_atfork(void (*)(void), void (*)(void), void(*)(void));

int pthread_attr_destroy(pthread_attr_t*) __nonnull((1));
int pthread_attr_getdetachstate(const pthread_attr_t*, int*) __nonnull((1, 2));
int pthread_attr_getguardsize(const pthread_attr_t*, size_t*) __nonnull((1, 2));
int pthread_attr_getschedparam(const pthread_attr_t*, struct sched_param*) __nonnull((1, 2));
int pthread_attr_getschedpolicy(const pthread_attr_t*, int*) __nonnull((1, 2));
int pthread_attr_getscope(const pthread_attr_t*, int*) __nonnull((1, 2));
int pthread_attr_getstack(const pthread_attr_t*, void**, size_t*) __nonnull((1, 2, 3));
int pthread_attr_getstacksize(const pthread_attr_t*, size_t*) __nonnull((1, 2));
int pthread_attr_init(pthread_attr_t*) __nonnull((1));
int pthread_attr_setdetachstate(pthread_attr_t*, int) __nonnull((1));
int pthread_attr_setguardsize(pthread_attr_t*, size_t) __nonnull((1));
int pthread_attr_setschedparam(pthread_attr_t*, const struct sched_param*) __nonnull((1, 2));
int pthread_attr_setschedpolicy(pthread_attr_t*, int) __nonnull((1));
int pthread_attr_setscope(pthread_attr_t*, int) __nonnull((1));
int pthread_attr_setstack(pthread_attr_t*, void*, size_t) __nonnull((1));
int pthread_attr_setstacksize(pthread_attr_t*, size_t stack_size) __nonnull((1));

int pthread_condattr_destroy(pthread_condattr_t*) __nonnull((1));
int pthread_condattr_getclock(const pthread_condattr_t*, clockid_t*) __nonnull((1, 2));
int pthread_condattr_getpshared(const pthread_condattr_t*, int*) __nonnull((1, 2));
int pthread_condattr_init(pthread_condattr_t*) __nonnull((1));
int pthread_condattr_setclock(pthread_condattr_t*, clockid_t) __nonnull((1));
int pthread_condattr_setpshared(pthread_condattr_t*, int) __nonnull((1));

int pthread_cond_broadcast(pthread_cond_t*) __nonnull((1));
int pthread_cond_destroy(pthread_cond_t*) __nonnull((1));
int pthread_cond_init(pthread_cond_t*, const pthread_condattr_t*) __nonnull((1));
int pthread_cond_signal(pthread_cond_t*) __nonnull((1));
int pthread_cond_timedwait(pthread_cond_t*, pthread_mutex_t*, const struct timespec*) __nonnull((1, 2, 3));
int pthread_cond_wait(pthread_cond_t*, pthread_mutex_t*) __nonnull((1, 2));

int pthread_create(pthread_t*, pthread_attr_t const*, void *(*)(void*), void*) __nonnull((1, 3));
int pthread_detach(pthread_t);
void pthread_exit(void*) __noreturn;

int pthread_equal(pthread_t, pthread_t);

int pthread_getattr_np(pthread_t, pthread_attr_t*) __nonnull((2));

int pthread_getcpuclockid(pthread_t, clockid_t*) __nonnull((2));

int pthread_getschedparam(pthread_t, int*, struct sched_param*) __nonnull((2, 3));

void* pthread_getspecific(pthread_key_t);

pid_t pthread_gettid_np(pthread_t);

int pthread_join(pthread_t, void**);

int pthread_key_create(pthread_key_t*, void (*)(void*)) __nonnull((1));
int pthread_key_delete(pthread_key_t);

int pthread_kill(pthread_t, int);

int pthread_mutexattr_destroy(pthread_mutexattr_t*) __nonnull((1));
int pthread_mutexattr_getpshared(const pthread_mutexattr_t*, int*) __nonnull((1, 2));
int pthread_mutexattr_gettype(const pthread_mutexattr_t*, int*) __nonnull((1, 2));
int pthread_mutexattr_init(pthread_mutexattr_t*) __nonnull((1));
int pthread_mutexattr_setpshared(pthread_mutexattr_t*, int) __nonnull((1));
int pthread_mutexattr_settype(pthread_mutexattr_t*, int) __nonnull((1));

int pthread_mutex_destroy(pthread_mutex_t*) __nonnull((1));
int pthread_mutex_init(pthread_mutex_t*, const pthread_mutexattr_t*) __nonnull((1));
int pthread_mutex_lock(pthread_mutex_t*) /* __nonnull((1)) */;
int pthread_mutex_timedlock(pthread_mutex_t*, const struct timespec*) __nonnull((1, 2));
int pthread_mutex_trylock(pthread_mutex_t*) __nonnull((1));
int pthread_mutex_unlock(pthread_mutex_t*) /* __nonnull((1)) */;

int pthread_once(pthread_once_t*, void (*)(void)) __nonnull((1, 2));

int pthread_rwlockattr_destroy(pthread_rwlockattr_t*) __nonnull((1));
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t*, int*) __nonnull((1, 2));
int pthread_rwlockattr_init(pthread_rwlockattr_t*) __nonnull((1));
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t*, int) __nonnull((1));

int pthread_rwlock_destroy(pthread_rwlock_t*) __nonnull((1));
int pthread_rwlock_init(pthread_rwlock_t*, const pthread_rwlockattr_t*) __nonnull((1));
int pthread_rwlock_rdlock(pthread_rwlock_t*) __nonnull((1));
int pthread_rwlock_timedrdlock(pthread_rwlock_t*, const struct timespec*) __nonnull((1, 2));
int pthread_rwlock_timedwrlock(pthread_rwlock_t*, const struct timespec*) __nonnull((1, 2));
int pthread_rwlock_tryrdlock(pthread_rwlock_t*) __nonnull((1));
int pthread_rwlock_trywrlock(pthread_rwlock_t*) __nonnull((1));
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) __nonnull((1));
int pthread_rwlock_wrlock(pthread_rwlock_t*) __nonnull((1));

pthread_t pthread_self(void) __pure2;

int pthread_setname_np(pthread_t, const char*) __nonnull((2));

int pthread_setschedparam(pthread_t, int, const struct sched_param*) __nonnull((3));

int pthread_setspecific(pthread_key_t, const void*);

int pthread_sigmask(int, const sigset_t*, sigset_t*);

typedef void (*__pthread_cleanup_func_t)(void*);

typedef struct __pthread_cleanup_t {
  struct __pthread_cleanup_t*   __cleanup_prev;
  __pthread_cleanup_func_t      __cleanup_routine;
  void*                         __cleanup_arg;
} __pthread_cleanup_t;

extern void __pthread_cleanup_push(__pthread_cleanup_t* c, __pthread_cleanup_func_t, void*);
extern void __pthread_cleanup_pop(__pthread_cleanup_t*, int);

/* Believe or not, the definitions of pthread_cleanup_push and
 * pthread_cleanup_pop below are correct. Posix states that these
 * can be implemented as macros that might introduce opening and
 * closing braces, and that using setjmp/longjmp/return/break/continue
 * between them results in undefined behavior.
 */
#define  pthread_cleanup_push(routine, arg)                      \
    do {                                                         \
        __pthread_cleanup_t  __cleanup;                          \
        __pthread_cleanup_push( &__cleanup, (routine), (arg) );  \

#define  pthread_cleanup_pop(execute)                  \
        __pthread_cleanup_pop( &__cleanup, (execute)); \
    } while (0);                                       \


#if !defined(__LP64__)

// Bionic additions that are deprecated even in the 32-bit ABI.
//
// TODO: Remove them once chromium_org / NFC have switched over.
int pthread_cond_timedwait_monotonic_np(pthread_cond_t*, pthread_mutex_t*, const struct timespec*);
int pthread_cond_timedwait_monotonic(pthread_cond_t*, pthread_mutex_t*, const struct timespec*);

int pthread_cond_timedwait_relative_np(pthread_cond_t*, pthread_mutex_t*, const struct timespec*) /* TODO: __attribute__((deprecated("use pthread_cond_timedwait instead")))*/;
#define HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE 1 /* TODO: stop defining this to push LP32 off this API sooner. */
int pthread_cond_timeout_np(pthread_cond_t*, pthread_mutex_t*, unsigned) /* TODO: __attribute__((deprecated("use pthread_cond_timedwait instead")))*/;

int pthread_mutex_lock_timeout_np(pthread_mutex_t*, unsigned) __attribute__((deprecated("use pthread_mutex_timedlock instead")));

#endif /* !defined(__LP64__) */

__END_DECLS

#endif /* _PTHREAD_H_ */
