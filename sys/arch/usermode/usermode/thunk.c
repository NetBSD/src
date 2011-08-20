/* $NetBSD: thunk.c,v 1.6 2011/08/20 20:14:04 reinoud Exp $ */

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
__RCSID("$NetBSD: thunk.c,v 1.6 2011/08/20 20:14:04 reinoud Exp $");

#include <machine/thunk.h>

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

int
thunk_setitimer(int which, const struct itimerval *value,
    struct itimerval *ovalue)
{
	return setitimer(which, value, ovalue);
}

int
thunk_gettimeofday(struct timeval *tp, void *tzp)
{
	return gettimeofday(tp, tzp);
}

int
thunk_clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	return clock_gettime(clock_id, tp);
}

int
thunk_clock_getres(clockid_t clock_id, struct timespec *res)
{
	return clock_getres(clock_id, res);
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
thunk_makecontext(ucontext_t *ucp, void (*func)(), int argc,
    void (*arg1)(void *), void *arg2)
{
	assert(argc == 2);

	makecontext(ucp, func, argc, arg1, arg2);
}

int
thunk_swapcontext(ucontext_t *oucp, ucontext_t *ucp)
{
	return swapcontext(oucp, ucp);
}

int
thunk_getchar(void)
{
	return getchar();
}

void
thunk_putchar(int c)
{
	putchar(c);
	fflush(stdout);
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
thunk_fstat(int fd, struct stat *sb)
{
	return fstat(fd, sb);
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
thunk_mkstemp(const char *template)
{
	return mkstemp(template);
}

int
thunk_sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	return sigaction(sig, act, oact);
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
thunk_sbrk(intptr_t len)
{
	return sbrk(len);
}

