/*	$NetBSD: pthread_compat.c,v 1.4 2017/12/08 09:24:31 kre Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Andrew Doran.
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

/*
 * libc symbols that are not present before NetBSD 5.0.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_compat.c,v 1.4 2017/12/08 09:24:31 kre Exp $");

#include <sys/param.h>
#include <sys/syscall.h>
#include <sys/aio.h>

#include <lwp.h>
#include <unistd.h>
#include <sched.h>

#include "pthread.h"
#include "pthread_int.h"

static void     __pthread_init(void) __attribute__((__constructor__, __used__));

void	__libc_thr_init(void);
void	__libc_atomic_init(void);

int	_sys_sched_yield(void);
int	_sys_aio_suspend(const struct aiocb * const[], int,
			 const struct timespec *);
int	_sys_mq_send(mqd_t, const char *, size_t, unsigned);
ssize_t	_sys_mq_receive(mqd_t, char *, size_t, unsigned *);
int	_sys_mq_timedsend(mqd_t, const char *, size_t, unsigned,
			  const struct timespec *);
ssize_t	_sys_mq_timedreceive(mqd_t, char *, size_t, unsigned *,
			     const struct timespec *);

static void
__pthread_init(void)
{

	__libc_atomic_init();
	__libc_thr_init();
}

int
_lwp_kill(lwpid_t a, int b)
{

	return syscall(SYS__lwp_kill, a, b);
}

int
_lwp_detach(lwpid_t a)
{

	return syscall(SYS__lwp_detach, a);
}

int
_lwp_park(clockid_t a, int b, const struct timespec *c, lwpid_t d,
    const void *e, const void *f)
{

	struct timespec t = *c;

	return syscall(SYS____lwp_park60, a, b, &t, d, e, f);
}

int
_lwp_unpark(lwpid_t a, const void *b)
{

	return syscall(SYS__lwp_unpark, a, b);
}

ssize_t
_lwp_unpark_all(const lwpid_t *a, size_t b, const void *c)
{

	return (ssize_t)syscall(SYS__lwp_unpark_all, a, b, c);
}

int
_lwp_setname(lwpid_t a, const char *b)
{

	return syscall(SYS__lwp_setname, a, b);
}

int
_lwp_getname(lwpid_t a, char *b, size_t c)
{

	return syscall(SYS__lwp_getname, a, b, c);
}

int
_lwp_ctl(int a, struct lwpctl **b)
{

	return syscall(SYS__lwp_ctl, a, b);
}

int
_sys_sched_yield(void)
{

	return syscall(SYS_sched_yield);
}

int
sched_yield(void)
{

	return syscall(SYS_sched_yield);
}

int
_sched_setaffinity(pid_t a, lwpid_t b, size_t c, const cpuset_t *d)
{

	return syscall(SYS__sched_setaffinity, a, b, c, d);
}

int
_sched_getaffinity(pid_t a, lwpid_t b, size_t c, cpuset_t *d)
{

	return syscall(SYS__sched_getaffinity, a, b, c, d);
}

int
_sched_setparam(pid_t a, lwpid_t b, int c, const struct sched_param *d)
{

	return syscall(SYS__sched_setparam, a, b, c, d);
}

int
_sched_getparam(pid_t a, lwpid_t b, int *c, struct sched_param *d)
{

	return syscall(SYS__sched_getparam, a, b, c, d);
}

int
_sys_aio_suspend(const struct aiocb * const a[], int b,
		 const struct timespec *c)
{

	return syscall(SYS_aio_suspend, a, b, c);
}

int
_sys_mq_send(mqd_t a, const char *b, size_t c, unsigned d)
{

	return syscall(SYS_mq_send, a, b, c, d);
}

ssize_t
_sys_mq_receive(mqd_t a, char *b, size_t c, unsigned *d)
{

	return (ssize_t)syscall(SYS_mq_receive, a, b, c, d);
}

int
_sys_mq_timedsend(mqd_t a, const char *b, size_t c, unsigned d,
		  const struct timespec *e)
{

	return syscall(SYS_mq_timedsend, a, b,c ,d, e);
}

ssize_t
_sys_mq_timedreceive(mqd_t a, char *b, size_t c, unsigned *d,
		     const struct timespec *e)
{

	return (ssize_t)syscall(SYS_mq_timedreceive, a, b, c, d, e);
}
