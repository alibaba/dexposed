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
#ifndef _UNISTD_H_
#define _UNISTD_H_

#include <stddef.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/sysconf.h>
#include <pathconf.h>

__BEGIN_DECLS

/* Standard file descriptor numbers. */
#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

/* Values for whence in fseek and lseek */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

extern char** environ;

extern __noreturn void _exit(int);

extern pid_t  fork(void);
extern pid_t  vfork(void);
extern pid_t  getpid(void);
extern pid_t  gettid(void) __pure2;
extern pid_t  getpgid(pid_t);
extern int    setpgid(pid_t, pid_t);
extern pid_t  getppid(void);
extern pid_t  getpgrp(void);
extern int    setpgrp(void);
extern pid_t  getsid(pid_t);
extern pid_t  setsid(void);

extern int execv(const char *, char * const *);
extern int execvp(const char *, char * const *);
extern int execvpe(const char *, char * const *, char * const *);
extern int execve(const char *, char * const *, char * const *);
extern int execl(const char *, const char *, ...);
extern int execlp(const char *, const char *, ...);
extern int execle(const char *, const char *, ...);

extern int nice(int);

extern int setuid(uid_t);
extern uid_t getuid(void);
extern int seteuid(uid_t);
extern uid_t geteuid(void);
extern int setgid(gid_t);
extern gid_t getgid(void);
extern int setegid(gid_t);
extern gid_t getegid(void);
extern int getgroups(int, gid_t *);
extern int setgroups(size_t, const gid_t *);
extern int setreuid(uid_t, uid_t);
extern int setregid(gid_t, gid_t);
extern int setresuid(uid_t, uid_t, uid_t);
extern int setresgid(gid_t, gid_t, gid_t);
extern int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
extern int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);
extern char* getlogin(void);
extern char* getusershell(void);
extern void setusershell(void);
extern void endusershell(void);



/* Macros for access() */
#define R_OK  4  /* Read */
#define W_OK  2  /* Write */
#define X_OK  1  /* Execute */
#define F_OK  0  /* Existence */

extern int access(const char*, int);
extern int faccessat(int, const char*, int, int);
extern int link(const char*, const char*);
extern int linkat(int, const char*, int, const char*, int);
extern int unlink(const char*);
extern int unlinkat(int, const char*, int);
extern int chdir(const char *);
extern int fchdir(int);
extern int rmdir(const char *);
extern int pipe(int *);
#ifdef _GNU_SOURCE
extern int pipe2(int *, int);
#endif
extern int chroot(const char *);
extern int symlink(const char*, const char*);
extern int symlinkat(const char*, int, const char*);
extern ssize_t readlink(const char*, char*, size_t);
extern ssize_t readlinkat(int, const char*, char*, size_t);
extern int chown(const char *, uid_t, gid_t);
extern int fchown(int, uid_t, gid_t);
extern int fchownat(int, const char*, uid_t, gid_t, int);
extern int lchown(const char *, uid_t, gid_t);
extern int truncate(const char *, off_t);
extern int truncate64(const char *, off64_t);
extern char *getcwd(char *, size_t);

extern int sync(void);

extern int close(int);
extern off_t lseek(int, off_t, int);
extern off64_t lseek64(int, off64_t, int);

extern ssize_t read(int, void *, size_t);
extern ssize_t write(int, const void *, size_t);
extern ssize_t pread(int, void *, size_t, off_t);
extern ssize_t pread64(int, void *, size_t, off64_t);
extern ssize_t pwrite(int, const void *, size_t, off_t);
extern ssize_t pwrite64(int, const void *, size_t, off64_t);

extern int dup(int);
extern int dup2(int, int);
#ifdef _GNU_SOURCE
extern int dup3(int, int, int);
#endif
extern int fcntl(int, int, ...);
extern int ioctl(int, int, ...);
extern int flock(int, int);
extern int fsync(int);
extern int fdatasync(int);
extern int ftruncate(int, off_t);
extern int ftruncate64(int, off64_t);

extern int pause(void);
extern unsigned int alarm(unsigned int);
extern unsigned int sleep(unsigned int);
extern int usleep(useconds_t);

extern int gethostname(char *, size_t);

extern void *__brk(void *);
extern int brk(void *);
extern void *sbrk(ptrdiff_t);

extern int getopt(int, char * const *, const char *);
extern char *optarg;
extern int optind, opterr, optopt;

extern int isatty(int);
extern char* ttyname(int) __warnattr("ttyname is not thread-safe; use ttyname_r instead");
extern int ttyname_r(int, char*, size_t);

extern int  acct(const char*  filepath);

int getpagesize(void);

extern int sysconf(int  name);

extern int daemon(int, int);

#if defined(__arm__) || (defined(__mips__) && !defined(__LP64__))
extern int cacheflush(long, long, long);
    /* __attribute__((deprecated("use __builtin___clear_cache instead"))); */
#endif

extern pid_t tcgetpgrp(int fd);
extern int   tcsetpgrp(int fd, pid_t _pid);

/* Used to retry syscalls that can return EINTR. */
#define TEMP_FAILURE_RETRY(exp) ({         \
    __typeof__(exp) _rc;                   \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })

#if defined(__BIONIC_FORTIFY)
extern ssize_t __read_chk(int, void*, size_t, size_t);
__errordecl(__read_dest_size_error, "read called with size bigger than destination");
__errordecl(__read_count_toobig_error, "read called with count > SSIZE_MAX");
extern ssize_t __read_real(int, void*, size_t)
    __asm__(__USER_LABEL_PREFIX__ "read");

__BIONIC_FORTIFY_INLINE
ssize_t read(int fd, void* buf, size_t count) {
    size_t bos = __bos0(buf);

#if !defined(__clang__)
    if (__builtin_constant_p(count) && (count > SSIZE_MAX)) {
        __read_count_toobig_error();
    }

    if (bos == __BIONIC_FORTIFY_UNKNOWN_SIZE) {
        return __read_real(fd, buf, count);
    }

    if (__builtin_constant_p(count) && (count > bos)) {
        __read_dest_size_error();
    }

    if (__builtin_constant_p(count) && (count <= bos)) {
        return __read_real(fd, buf, count);
    }
#endif

    return __read_chk(fd, buf, count, bos);
}
#endif /* defined(__BIONIC_FORTIFY) */

__END_DECLS

#endif /* _UNISTD_H_ */
