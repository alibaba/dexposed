/*	$OpenBSD: stdio.h,v 1.35 2006/01/13 18:10:09 miod Exp $	*/
/*	$NetBSD: stdio.h,v 1.18 1996/04/25 18:29:21 jtc Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)stdio.h	5.17 (Berkeley) 6/3/91
 */

#ifndef	_STDIO_H_
#define	_STDIO_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#include <stdarg.h>
#include <stddef.h>

#define __need_NULL
#include <stddef.h>

__BEGIN_DECLS

#define	_FSTDIO			/* Define for new stdio with functions. */

typedef off_t fpos_t;		/* stdio file position type */

/*
 * NB: to fit things in six character monocase externals, the stdio
 * code uses the prefix `__s' for stdio objects, typically followed
 * by a three-character attempt at a mnemonic.
 */

/* stdio buffers */
#if defined(__LP64__)
struct __sbuf {
  unsigned char* _base;
  size_t _size;
};
#else
struct __sbuf {
	unsigned char *_base;
	int	_size;
};
#endif

/*
 * stdio state variables.
 *
 * The following always hold:
 *
 *	if (_flags&(__SLBF|__SWR)) == (__SLBF|__SWR),
 *		_lbfsize is -_bf._size, else _lbfsize is 0
 *	if _flags&__SRD, _w is 0
 *	if _flags&__SWR, _r is 0
 *
 * This ensures that the getc and putc macros (or inline functions) never
 * try to write or read from a file that is in `read' or `write' mode.
 * (Moreover, they can, and do, automatically switch from read mode to
 * write mode, and back, on "r+" and "w+" files.)
 *
 * _lbfsize is used only to make the inline line-buffered output stream
 * code as compact as possible.
 *
 * _ub, _up, and _ur are used when ungetc() pushes back more characters
 * than fit in the current _bf, or when ungetc() pushes back a character
 * that does not match the previous one in _bf.  When this happens,
 * _ub._base becomes non-nil (i.e., a stream has ungetc() data iff
 * _ub._base!=NULL) and _up and _ur save the current values of _p and _r.
 *
 * NOTE: if you change this structure, you also need to update the
 * std() initializer in findfp.c.
 */
typedef	struct __sFILE {
	unsigned char *_p;	/* current position in (some) buffer */
	int	_r;		/* read space left for getc() */
	int	_w;		/* write space left for putc() */
#if defined(__LP64__)
	int	_flags;		/* flags, below; this FILE is free if 0 */
	int	_file;		/* fileno, if Unix descriptor, else -1 */
#else
	short	_flags;		/* flags, below; this FILE is free if 0 */
	short	_file;		/* fileno, if Unix descriptor, else -1 */
#endif
	struct	__sbuf _bf;	/* the buffer (at least 1 byte, if !NULL) */
	int	_lbfsize;	/* 0 or -_bf._size, for inline putc */

	/* operations */
	void	*_cookie;	/* cookie passed to io functions */
	int	(*_close)(void *);
	int	(*_read)(void *, char *, int);
	fpos_t	(*_seek)(void *, fpos_t, int);
	int	(*_write)(void *, const char *, int);

	/* extension data, to avoid further ABI breakage */
	struct	__sbuf _ext;
	/* data for long sequences of ungetc() */
	unsigned char *_up;	/* saved _p when _p is doing ungetc data */
	int	_ur;		/* saved _r when _r is counting ungetc data */

	/* tricks to meet minimum requirements even when malloc() fails */
	unsigned char _ubuf[3];	/* guarantee an ungetc() buffer */
	unsigned char _nbuf[1];	/* guarantee a getc() buffer */

	/* separate buffer for fgetln() when line crosses buffer boundary */
	struct	__sbuf _lb;	/* buffer for fgetln() */

	/* Unix stdio files get aligned to block boundaries on fseek() */
	int	_blksize;	/* stat.st_blksize (may be != _bf._size) */
	fpos_t	_offset;	/* current lseek offset */
} FILE;

extern FILE __sF[];

#define	__SLBF	0x0001		/* line buffered */
#define	__SNBF	0x0002		/* unbuffered */
#define	__SRD	0x0004		/* OK to read */
#define	__SWR	0x0008		/* OK to write */
	/* RD and WR are never simultaneously asserted */
#define	__SRW	0x0010		/* open for reading & writing */
#define	__SEOF	0x0020		/* found EOF */
#define	__SERR	0x0040		/* found error */
#define	__SMBF	0x0080		/* _buf is from malloc */
#define	__SAPP	0x0100		/* fdopen()ed in append mode */
#define	__SSTR	0x0200		/* this is an sprintf/snprintf string */
#define	__SOPT	0x0400		/* do fseek() optimization */
#define	__SNPT	0x0800		/* do not do fseek() optimization */
#define	__SOFF	0x1000		/* set iff _offset is in fact correct */
#define	__SMOD	0x2000		/* true => fgetln modified _p text */
#define	__SALC	0x4000		/* allocate string space dynamically */
#define	__SIGN	0x8000		/* ignore this file in _fwalk */

/*
 * The following three definitions are for ANSI C, which took them
 * from System V, which brilliantly took internal interface macros and
 * made them official arguments to setvbuf(), without renaming them.
 * Hence, these ugly _IOxxx names are *supposed* to appear in user code.
 *
 * Although numbered as their counterparts above, the implementation
 * does not rely on this.
 */
#define	_IOFBF	0		/* setvbuf should set fully buffered */
#define	_IOLBF	1		/* setvbuf should set line buffered */
#define	_IONBF	2		/* setvbuf should set unbuffered */

#define	BUFSIZ	1024		/* size of buffer used by setbuf */
#define	EOF	(-1)

/*
 * FOPEN_MAX is a minimum maximum, and is the number of streams that
 * stdio can provide without attempting to allocate further resources
 * (which could fail).  Do not use this for anything.
 */

#define	FOPEN_MAX	20	/* must be <= OPEN_MAX <sys/syslimits.h> */
#define	FILENAME_MAX	1024	/* must be <= PATH_MAX <sys/syslimits.h> */

/* System V/ANSI C; this is the wrong way to do this, do *not* use these. */
#if __BSD_VISIBLE || __XPG_VISIBLE
#define	P_tmpdir	"/tmp/"
#endif
#define	L_tmpnam	1024	/* XXX must be == PATH_MAX */
#define	TMP_MAX		308915776

/* Always ensure that these are consistent with <fcntl.h> and <unistd.h>! */
#ifndef SEEK_SET
#define	SEEK_SET	0	/* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define	SEEK_END	2	/* set file offset to EOF plus offset */
#endif

#define	stdin	(&__sF[0])
#define	stdout	(&__sF[1])
#define	stderr	(&__sF[2])

/*
 * Functions defined in ANSI C standard.
 */
void	 clearerr(FILE *);
int	 fclose(FILE *);
int	 feof(FILE *);
int	 ferror(FILE *);
int	 fflush(FILE *);
int	 fgetc(FILE *);
char	*fgets(char * __restrict, int, FILE * __restrict);
FILE	*fopen(const char * __restrict , const char * __restrict);
int	 fprintf(FILE * __restrict , const char * __restrict, ...)
		__printflike(2, 3);
int	 fputc(int, FILE *);
int	 fputs(const char * __restrict, FILE * __restrict);
size_t	 fread(void * __restrict, size_t, size_t, FILE * __restrict);
FILE	*freopen(const char * __restrict, const char * __restrict,
	    FILE * __restrict);
int	 fscanf(FILE * __restrict, const char * __restrict, ...)
		__scanflike(2, 3);
int	 fseek(FILE *, long, int);
long	 ftell(FILE *);
size_t	 fwrite(const void * __restrict, size_t, size_t, FILE * __restrict);
int	 getc(FILE *);
int	 getchar(void);
ssize_t	 getdelim(char ** __restrict, size_t * __restrict, int,
	    FILE * __restrict);
ssize_t	 getline(char ** __restrict, size_t * __restrict, FILE * __restrict);

void	 perror(const char *);
int	 printf(const char * __restrict, ...)
		__printflike(1, 2);
int	 putc(int, FILE *);
int	 putchar(int);
int	 puts(const char *);
int	 remove(const char *);
void	 rewind(FILE *);
int	 scanf(const char * __restrict, ...)
		__scanflike(1, 2);
void	 setbuf(FILE * __restrict, char * __restrict);
int	 setvbuf(FILE * __restrict, char * __restrict, int, size_t);
int	 sscanf(const char * __restrict, const char * __restrict, ...)
		__scanflike(2, 3);
FILE	*tmpfile(void);
int	 ungetc(int, FILE *);
int	 vfprintf(FILE * __restrict, const char * __restrict, __va_list)
		__printflike(2, 0);
int	 vprintf(const char * __restrict, __va_list)
		__printflike(1, 0);

int dprintf(int, const char * __restrict, ...) __printflike(2, 3);
int vdprintf(int, const char * __restrict, __va_list) __printflike(2, 0);

#ifndef __AUDIT__
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
char* gets(char*) __warnattr("gets is very unsafe; consider using fgets");
#endif
int sprintf(char* __restrict, const char* __restrict, ...)
    __printflike(2, 3) __warnattr("sprintf is often misused; please use snprintf");
char* tmpnam(char*) __warnattr("tmpnam possibly used unsafely; consider using mkstemp");
int vsprintf(char* __restrict, const char* __restrict, __va_list)
    __printflike(2, 0) __warnattr("vsprintf is often misused; please use vsnprintf");
#if __XPG_VISIBLE
char* tempnam(const char*, const char*)
    __warnattr("tempnam possibly used unsafely; consider using mkstemp");
#endif
#endif

extern int rename(const char*, const char*);
extern int renameat(int, const char*, int, const char*);

int	 fgetpos(FILE * __restrict, fpos_t * __restrict);
int	 fsetpos(FILE *, const fpos_t *);

int	 fseeko(FILE *, off_t, int);
off_t	 ftello(FILE *);

#if __ISO_C_VISIBLE >= 1999 || __BSD_VISIBLE
int	 snprintf(char * __restrict, size_t, const char * __restrict, ...)
		__printflike(3, 4);
int	 vfscanf(FILE * __restrict, const char * __restrict, __va_list)
		__scanflike(2, 0);
int	 vscanf(const char *, __va_list)
		__scanflike(1, 0);
int	 vsnprintf(char * __restrict, size_t, const char * __restrict, __va_list)
		__printflike(3, 0);
int	 vsscanf(const char * __restrict, const char * __restrict, __va_list)
		__scanflike(2, 0);
#endif /* __ISO_C_VISIBLE >= 1999 || __BSD_VISIBLE */

/*
 * Functions defined in POSIX 1003.1.
 */
#if __BSD_VISIBLE || __POSIX_VISIBLE || __XPG_VISIBLE
#define	L_ctermid	1024	/* size for ctermid(); PATH_MAX */

FILE	*fdopen(int, const char *);
int	 fileno(FILE *);

#if (__POSIX_VISIBLE >= 199209)
int	 pclose(FILE *);
FILE	*popen(const char *, const char *);
#endif

#if __POSIX_VISIBLE >= 199506
void	 flockfile(FILE *);
int	 ftrylockfile(FILE *);
void	 funlockfile(FILE *);

/*
 * These are normally used through macros as defined below, but POSIX
 * requires functions as well.
 */
int	 getc_unlocked(FILE *);
int	 getchar_unlocked(void);
int	 putc_unlocked(int, FILE *);
int	 putchar_unlocked(int);
#endif /* __POSIX_VISIBLE >= 199506 */

#endif /* __BSD_VISIBLE || __POSIX_VISIBLE || __XPG_VISIBLE */

/*
 * Routines that are purely local.
 */
#if __BSD_VISIBLE
int	 asprintf(char ** __restrict, const char * __restrict, ...)
		__printflike(2, 3);
char	*fgetln(FILE * __restrict, size_t * __restrict);
int	 fpurge(FILE *);
void	 setbuffer(FILE *, char *, int);
int	 setlinebuf(FILE *);
int	 vasprintf(char ** __restrict, const char * __restrict,
    __va_list)
		__printflike(2, 0);

/*
 * Stdio function-access interface.
 */
FILE	*funopen(const void *,
		int (*)(void *, char *, int),
		int (*)(void *, const char *, int),
		fpos_t (*)(void *, fpos_t, int),
		int (*)(void *));

#define	fropen(cookie, fn) funopen(cookie, fn, 0, 0, 0)
#define	fwopen(cookie, fn) funopen(cookie, 0, fn, 0, 0)
#endif /* __BSD_VISIBLE */

extern char* __fgets_chk(char*, int, FILE*, size_t);
extern char* __fgets_real(char*, int, FILE*) __RENAME(fgets);
__errordecl(__fgets_too_big_error, "fgets called with size bigger than buffer");
__errordecl(__fgets_too_small_error, "fgets called with size less than zero");

#if defined(__BIONIC_FORTIFY)

__BIONIC_FORTIFY_INLINE
__printflike(3, 0)
int vsnprintf(char *dest, size_t size, const char *format, __va_list ap)
{
    return __builtin___vsnprintf_chk(dest, size, 0, __bos(dest), format, ap);
}

__BIONIC_FORTIFY_INLINE
__printflike(2, 0)
int vsprintf(char *dest, const char *format, __va_list ap)
{
    return __builtin___vsprintf_chk(dest, 0, __bos(dest), format, ap);
}

#if defined(__clang__)
  #if !defined(snprintf)
    #define __wrap_snprintf(dest, size, ...) __builtin___snprintf_chk(dest, size, 0, __bos(dest), __VA_ARGS__)
    #define snprintf(...) __wrap_snprintf(__VA_ARGS__)
  #endif
#else
__BIONIC_FORTIFY_INLINE
__printflike(3, 4)
int snprintf(char *dest, size_t size, const char *format, ...)
{
    return __builtin___snprintf_chk(dest, size, 0,
        __bos(dest), format, __builtin_va_arg_pack());
}
#endif

#if defined(__clang__)
  #if !defined(sprintf)
    #define __wrap_sprintf(dest, ...) __builtin___sprintf_chk(dest, 0, __bos(dest), __VA_ARGS__)
    #define sprintf(...) __wrap_sprintf(__VA_ARGS__)
  #endif
#else
__BIONIC_FORTIFY_INLINE
__printflike(2, 3)
int sprintf(char *dest, const char *format, ...)
{
    return __builtin___sprintf_chk(dest, 0,
        __bos(dest), format, __builtin_va_arg_pack());
}
#endif

#if !defined(__clang__)

__BIONIC_FORTIFY_INLINE
char *fgets(char* dest, int size, FILE* stream) {
    size_t bos = __bos(dest);

    // Compiler can prove, at compile time, that the passed in size
    // is always negative. Force a compiler error.
    if (__builtin_constant_p(size) && (size < 0)) {
        __fgets_too_small_error();
    }

    // Compiler doesn't know destination size. Don't call __fgets_chk
    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __fgets_real(dest, size, stream);
    }

    // Compiler can prove, at compile time, that the passed in size
    // is always <= the actual object size. Don't call __fgets_chk
    if (__builtin_constant_p(size) && (size <= (int) bos)) {
        return __fgets_real(dest, size, stream);
    }

    // Compiler can prove, at compile time, that the passed in size
    // is always > the actual object size. Force a compiler error.
    if (__builtin_constant_p(size) && (size > (int) bos)) {
        __fgets_too_big_error();
    }

    return __fgets_chk(dest, size, stream, bos);
}

#endif /* !defined(__clang__) */

#endif /* defined(__BIONIC_FORTIFY) */

__END_DECLS

#endif /* _STDIO_H_ */
