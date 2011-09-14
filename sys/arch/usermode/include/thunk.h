/* $NetBSD: thunk.h,v 1.33 2011/09/14 18:26:24 reinoud Exp $ */

/*-
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARCH_USERMODE_INCLUDE_THUNK_H
#define _ARCH_USERMODE_INCLUDE_THUNK_H

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ucontext.h>
#include <sys/signal.h>

struct thunk_timeval {
	int64_t tv_sec;
	int32_t tv_usec;
};

struct thunk_itimerval {
	struct thunk_timeval it_interval;
	struct thunk_timeval it_value;
};

struct thunk_termios {
	uint32_t c_iflag;
	uint32_t c_oflag;
	uint32_t c_cflag;
	uint32_t c_lflag;
	uint8_t c_cc[20];
	int32_t c_ispeed;
	int32_t c_ospeed;
};

#define THUNK_MAP_ANON		0x0001
#define THUNK_MAP_FIXED		0x0002
#define THUNK_MAP_FILE		0x0004
#define THUNK_MAP_SHARED	0x0010
#define THUNK_MAP_PRIVATE	0x0020

#define THUNK_PROT_NONE		0x00
#define THUNK_PROT_READ		0x01
#define THUNK_PROT_WRITE	0x02
#define THUNK_PROT_EXEC		0x04

struct aiocb;

int	thunk_setitimer(int, const struct thunk_itimerval *, struct thunk_itimerval *);
int	thunk_gettimeofday(struct thunk_timeval *, void *);
unsigned int thunk_getcounter(void);
long	thunk_clock_getres_monotonic(void);
int	thunk_usleep(useconds_t);

void	thunk_exit(int);
void	thunk_abort(void);

int	thunk_geterrno(void);
void	thunk_seterrno(int err);

int	thunk_getcontext(ucontext_t *);
int	thunk_setcontext(const ucontext_t *);
void	thunk_makecontext(ucontext_t *ucp, void (*func)(void), 
		int nargs, void *arg1, void *arg2, void *arg3);
int	thunk_swapcontext(ucontext_t *, ucontext_t *);

int	thunk_tcgetattr(int, struct thunk_termios *);
int	thunk_tcsetattr(int, int, const struct thunk_termios *);

int	thunk_getchar(void);
void	thunk_putchar(int);

int	thunk_execv(const char *, char * const []);

int	thunk_open(const char *, int, mode_t);
int	thunk_fstat_getsize(int, ssize_t *, ssize_t *);
ssize_t	thunk_pread(int, void *, size_t, off_t);
ssize_t	thunk_pwrite(int, const void *, size_t, off_t);
int	thunk_fsync(int);
int	thunk_mkstemp(char *);
int	thunk_unlink(const char *);

int	thunk_sigaction(int, const struct sigaction *, struct sigaction *);
int	thunk_sigaltstack(const stack_t *, stack_t *);
void	thunk_signal(int, void (*)(int));
int	thunk_sigblock(int);
int	thunk_sigunblock(int);
int	thunk_sigemptyset(sigset_t *sa_mask);
void	thunk_sigaddset(sigset_t *sa_mask, int sig);
int	thunk_atexit(void (*function)(void));

int	thunk_aio_read(struct aiocb *);
int	thunk_aio_write(struct aiocb *);
int	thunk_aio_error(const struct aiocb *);
int	thunk_aio_return(struct aiocb *);

void *	thunk_malloc(size_t len);
void 	thunk_free(void *addr);
void *	thunk_sbrk(intptr_t len);
void *	thunk_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
int	thunk_munmap(void *addr, size_t len);
int	thunk_mprotect(void *addr, size_t len, int prot);
int	thunk_posix_memalign(void **, size_t, size_t);

char *	thunk_getenv(const char *);
vaddr_t	thunk_get_vm_min_address(void);

int	thunk_sdl_init(unsigned int, unsigned int, unsigned short);
void *	thunk_sdl_getfb(size_t);
int	thunk_sdl_getchar(void);

#endif /* !_ARCH_USERMODE_INCLUDE_THUNK_H */
