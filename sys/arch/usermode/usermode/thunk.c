/* $NetBSD: thunk.c,v 1.33 2011/09/05 11:09:38 reinoud Exp $ */

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

#include <sys/cdefs.h>
#ifdef __NetBSD__
__RCSID("$NetBSD: thunk.c,v 1.33 2011/09/05 11:09:38 reinoud Exp $");
#endif

#include <sys/types.h>
#include <sys/mman.h>

#include <aio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

#include "../include/thunk.h"

#ifndef __arraycount
#define __arraycount(x)	(sizeof((x)) / sizeof((x)[0]))
#endif

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif

static void
thunk_to_timeval(const struct thunk_timeval *ttv, struct timeval *tv)
{
	tv->tv_sec = ttv->tv_sec;
	tv->tv_usec = ttv->tv_usec;
}

static void
thunk_from_timeval(const struct timeval *tv, struct thunk_timeval *ttv)
{
	ttv->tv_sec = tv->tv_sec;
	ttv->tv_usec = tv->tv_usec;
}

static void
thunk_to_itimerval(const struct thunk_itimerval *tit, struct itimerval *it)
{
	thunk_to_timeval(&tit->it_interval, &it->it_interval);
	thunk_to_timeval(&tit->it_value, &it->it_value);
}

static void
thunk_from_itimerval(const struct itimerval *it, struct thunk_itimerval *tit)
{
	thunk_from_timeval(&it->it_interval, &tit->it_interval);
	thunk_from_timeval(&it->it_value, &tit->it_value);
}

static void
thunk_to_termios(const struct thunk_termios *tt, struct termios *t)
{
	int i;

	t->c_iflag = tt->c_iflag;
	t->c_oflag = tt->c_oflag;
	t->c_cflag = tt->c_cflag;
	t->c_lflag = tt->c_lflag;
	for (i = 0; i < __arraycount(t->c_cc); i++)
		t->c_cc[i] = tt->c_cc[i];
	t->c_ispeed = tt->c_ispeed;
	t->c_ospeed= tt->c_ospeed;
}

static void
thunk_from_termios(const struct termios *t, struct thunk_termios *tt)
{
	int i;

	tt->c_iflag = t->c_iflag;
	tt->c_oflag = t->c_oflag;
	tt->c_cflag = t->c_cflag;
	tt->c_lflag = t->c_lflag;
	for (i = 0; i < __arraycount(tt->c_cc); i++)
		tt->c_cc[i] = t->c_cc[i];
	tt->c_ispeed = t->c_ispeed;
	tt->c_ospeed= t->c_ospeed;
}

static int
thunk_to_native_prot(int prot)
{
	int nprot = PROT_NONE;

	if (prot & THUNK_PROT_READ)
		nprot |= PROT_READ;
	if (prot & THUNK_PROT_WRITE)
		nprot |= PROT_WRITE;
	if (prot & THUNK_PROT_EXEC)
		nprot |= PROT_EXEC;

	return nprot;
}

static int
thunk_to_native_mapflags(int flags)
{
	int nflags = 0;

	if (flags & THUNK_MAP_ANON)
		nflags |= MAP_ANON;
	if (flags & THUNK_MAP_FIXED)
		nflags |= MAP_FIXED;
	if (flags & THUNK_MAP_FILE)
		nflags |= MAP_FILE;
	if (flags & THUNK_MAP_SHARED)
		nflags |= MAP_SHARED;
	if (flags & THUNK_MAP_PRIVATE)
		nflags |= MAP_PRIVATE;

	return nflags;
}

int
thunk_setitimer(int which, const struct thunk_itimerval *value,
    struct thunk_itimerval *ovalue)
{
	struct itimerval it, oit;
	int error;

	thunk_to_itimerval(value, &it);
	error = setitimer(which, &it, &oit);
	if (error)
		return error;
	if (ovalue)
		thunk_from_itimerval(&oit, ovalue);

	return 0;
}

int
thunk_gettimeofday(struct thunk_timeval *tp, void *tzp)
{
	struct timeval tv;
	int error;

	error = gettimeofday(&tv, tzp);
	if (error)
		return error;

	thunk_from_timeval(&tv, tp);

	return 0;
}

unsigned int
thunk_getcounter(void)
{
	struct timespec ts;
	int error;

	error = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (error) {
		perror("clock_gettime CLOCK_MONOTONIC");
		abort();
	}

	return (unsigned int)(ts.tv_sec * 1000000000ULL + ts.tv_nsec);
}

long
thunk_clock_getres_monotonic(void)
{
	struct timespec res;
	int error;

	error = clock_getres(CLOCK_MONOTONIC, &res);
	if (error)
		return -1;

	return res.tv_nsec;
}

int
thunk_usleep(useconds_t microseconds)
{
	return usleep(microseconds);
}

void
thunk_exit(int status)
{
	return exit(status);
}

void
thunk_abort(void)
{
	abort();
}

int
thunk_geterrno(void)
{
	return errno;
}

void
thunk_seterrno(int nerrno)
{
	errno = nerrno;
}

int
thunk_getcontext(ucontext_t *ucp)
{
	return getcontext(ucp);
}

int
thunk_setcontext(const ucontext_t *ucp)
{
	return setcontext(ucp);
}

void
thunk_makecontext(ucontext_t *ucp, void (*func)(void), int argc,
    void (*arg1)(void *), void *arg2)
{
//	assert(argc == 2);

	makecontext(ucp, func, argc, arg1, arg2);
}

void
thunk_makecontext_1(ucontext_t *ucp, void *func, void *arg)
{
	makecontext(ucp, func, 1, arg);
}

int
thunk_swapcontext(ucontext_t *oucp, ucontext_t *ucp)
{
	return swapcontext(oucp, ucp);
}

int
thunk_tcgetattr(int fd, struct thunk_termios *tt)
{
	struct termios t;
	int error;

	error = tcgetattr(fd, &t);
	if (error)
		return error;
	thunk_from_termios(&t, tt);
	return 0;
}

int
thunk_tcsetattr(int fd, int action, const struct thunk_termios *tt)
{
	struct termios t;

	thunk_to_termios(tt, &t);
	return tcsetattr(fd, action, &t);
}

int
thunk_getchar(void)
{
	return getchar();
}

void
thunk_putchar(int c)
{
	char wc = (char) c;
	write(1, &wc, 1);
}

int
thunk_execv(const char *path, char * const argv[])
{
	return execv(path, argv);
}

int
thunk_open(const char *path, int flags, mode_t mode)
{
	return open(path, flags, mode);
}

int
thunk_fstat_getsize(int fd, ssize_t *size, ssize_t *blksize)
{
	struct stat st;
	int error;

	error = fstat(fd, &st);
	if (error)
		return -1;

	if (size)
		*size = st.st_size;
	if (blksize)
		*blksize = st.st_blksize;

	return 0;
}

ssize_t
thunk_pread(int d, void *buf, size_t nbytes, off_t offset)
{
	return pread(d, buf, nbytes, offset);
}

ssize_t
thunk_pwrite(int d, const void *buf, size_t nbytes, off_t offset)
{
	return pwrite(d, buf, nbytes, offset);
}

int
thunk_fsync(int fd)
{
	return fsync(fd);
}

int
thunk_mkstemp(char *template)
{
	return mkstemp(template);
}

int
thunk_unlink(const char *path)
{
	return unlink(path);
}

int
thunk_sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	return sigaction(sig, act, oact);
}

int
thunk_sigaltstack(const stack_t *ss, stack_t *oss)
{
	return sigaltstack(ss, oss);
}

void
thunk_signal(int sig, void (*func)(int))
{
	signal(sig, func);
}

int
thunk_sigblock(int sig)
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, sig);
	return sigprocmask(SIG_BLOCK, &set, NULL);
}

int
thunk_sigunblock(int sig)
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, sig);
	return sigprocmask(SIG_UNBLOCK, &set, NULL);
}

void
thunk_sigaddset(sigset_t *sa_mask, int sig)
{
	int retval;
	retval = sigaddset(sa_mask, sig);
	if (retval < 0) {
		perror("%s: bad signal added");
		abort();
	}
}

int
thunk_atexit(void (*function)(void))
{
	return atexit(function);
}

int
thunk_aio_read(struct aiocb *aiocbp)
{
	return aio_read(aiocbp);
}

int
thunk_aio_write(struct aiocb *aiocbp)
{
	return aio_write(aiocbp);
}

int
thunk_aio_error(const struct aiocb *aiocbp)
{
	return aio_error(aiocbp);
}

int
thunk_aio_return(struct aiocb *aiocbp)
{
	return aio_return(aiocbp);
}

void *
thunk_malloc(size_t len)
{
	return malloc(len);
}

void
thunk_free(void *addr)
{
	free(addr);
}

void *
thunk_sbrk(intptr_t len)
{
	return sbrk(len);
}

void *
thunk_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
	int nflags, nprot;
	void *a;

	nprot = thunk_to_native_prot(prot);
	nflags = thunk_to_native_mapflags(flags);

	a = mmap(addr, len, nprot, nflags, fd, offset);
	if (a == (void *)-1)
		perror("mmap");
	return a;
}

int
thunk_munmap(void *addr, size_t len)
{
	return munmap(addr, len);
}

int
thunk_mprotect(void *addr, size_t len, int prot)
{
	int nprot;

	nprot = thunk_to_native_prot(prot);

	return mprotect(addr, len, nprot);
}

int
thunk_posix_memalign(void **ptr, size_t alignment, size_t size)
{
	return posix_memalign(ptr, alignment, size);
}

char *
thunk_getenv(const char *name)
{
	return getenv(name);
}
