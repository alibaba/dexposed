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
#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <sys/cdefs.h>

#include <stddef.h>
#include <string.h>
#include <alloca.h>
#include <strings.h>
#include <memory.h>

__BEGIN_DECLS

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

extern __noreturn void abort(void);
extern __noreturn void exit(int);
extern __noreturn void _Exit(int);
extern int atexit(void (*)(void));

#if __ISO_C_VISIBLE >= 2011 || __cplusplus >= 201103L
int at_quick_exit(void (*)(void));
void quick_exit(int) __noreturn;
#endif

extern char* getenv(const char*);
extern int putenv(char*);
extern int setenv(const char*, const char*, int);
extern int unsetenv(const char*);
extern int clearenv(void);

extern char* mkdtemp(char*);
extern char* mktemp(char*) __warnattr("mktemp possibly used unsafely; consider using mkstemp");
extern int mkstemp(char*);
extern int mkstemp64(char*);

extern long strtol(const char *, char **, int);
extern long long strtoll(const char *, char **, int);
extern unsigned long strtoul(const char *, char **, int);
extern unsigned long long strtoull(const char *, char **, int);

extern int posix_memalign(void **memptr, size_t alignment, size_t size);

extern double atof(const char*);

extern double strtod(const char*, char**) __LIBC_ABI_PUBLIC__;
extern float strtof(const char*, char**) __LIBC_ABI_PUBLIC__;
extern long double strtold(const char*, char**) __LIBC_ABI_PUBLIC__;

extern long double strtold_l(const char *, char **, locale_t) __LIBC_ABI_PUBLIC__;
extern long long strtoll_l(const char *, char **, int, locale_t) __LIBC_ABI_PUBLIC__;
extern unsigned long long strtoull_l(const char *, char **, int, locale_t) __LIBC_ABI_PUBLIC__;

extern int atoi(const char*) __purefunc;
extern long atol(const char*) __purefunc;
extern long long atoll(const char*) __purefunc;

extern int abs(int) __pure2;
extern long labs(long) __pure2;
extern long long llabs(long long) __pure2;

extern char * realpath(const char *path, char *resolved);
extern int system(const char * string);

extern void * bsearch(const void *key, const void *base0,
	size_t nmemb, size_t size,
	int (*compar)(const void *, const void *));

extern void qsort(void *, size_t, size_t, int (*)(const void *, const void *));

extern long jrand48(unsigned short *);
extern long mrand48(void);
extern long nrand48(unsigned short *);
extern long lrand48(void);
extern unsigned short *seed48(unsigned short*);
extern double erand48(unsigned short xsubi[3]);
extern double drand48(void);
extern void srand48(long);

unsigned int arc4random(void);
unsigned int arc4random_uniform(unsigned int);
void arc4random_buf(void*, size_t);

#define RAND_MAX 0x7fffffff

int rand(void);
int rand_r(unsigned int*);
void srand(unsigned int);

char* initstate(unsigned int, char*, size_t);
long random(void);
char* setstate(char*);
void srandom(unsigned int);

int getpt(void);
int grantpt(int);
int posix_openpt(int);
char* ptsname(int) __warnattr("ptsname is not thread-safe; use ptsname_r instead");
int ptsname_r(int, char*, size_t);
int unlockpt(int);

typedef struct {
    int  quot;
    int  rem;
} div_t;

extern div_t   div(int, int) __pure2;

typedef struct {
    long int  quot;
    long int  rem;
} ldiv_t;

extern ldiv_t   ldiv(long, long) __pure2;

typedef struct {
    long long int  quot;
    long long int  rem;
} lldiv_t;

extern lldiv_t   lldiv(long long, long long) __pure2;

/* BSD compatibility. */
extern const char* getprogname(void);
extern void setprogname(const char*);

/* make STLPort happy */
extern int      mblen(const char *, size_t);
extern size_t   mbstowcs(wchar_t *, const char *, size_t);
extern int      mbtowc(wchar_t *, const char *, size_t);

/* Likewise, make libstdc++-v3 happy.  */
extern int	wctomb(char *, wchar_t);
extern size_t	wcstombs(char *, const wchar_t *, size_t);

extern size_t __ctype_get_mb_cur_max(void);
#define MB_CUR_MAX __ctype_get_mb_cur_max()

__END_DECLS

#endif /* _STDLIB_H_ */
