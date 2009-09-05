/*	$NetBSD: pthread_misc.c,v 1.10.4.1 2009/09/05 12:51:09 bouyer Exp $	*/

/*-
 * Copyright (c) 2001, 2006, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
__RCSID("$NetBSD: pthread_misc.c,v 1.10.4.1 2009/09/05 12:51:09 bouyer Exp $");

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/pset.h>
#include <sys/signal.h>
#include <sys/time.h>

#include <lwp.h>
#include <sched.h>

#include "pthread.h"
#include "pthread_int.h"

int	pthread__sched_yield(void);

int	_sys___sigprocmask14(int, const sigset_t *, sigset_t *);
int	_sys_sched_yield(void);

__strong_alias(__libc_thr_sigsetmask,pthread_sigmask)
__strong_alias(__sigprocmask14,pthread_sigmask)
__strong_alias(__libc_thr_yield,pthread__sched_yield)

int
pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param)
{

	if (pthread__find(thread) != 0)
		return ESRCH;

	if (_sched_getparam(getpid(), thread->pt_lid, policy, param) < 0)
		return errno;

	return 0;
}

int
pthread_setschedparam(pthread_t thread, int policy, 
    const struct sched_param *param)
{
	struct sched_param sp;

	if (pthread__find(thread) != 0)
		return ESRCH;

	memcpy(&sp, param, sizeof(struct sched_param));
	if (_sched_setparam(getpid(), thread->pt_lid, policy, &sp) < 0)
		return errno;

	return 0;
}

int
pthread_getaffinity_np(pthread_t thread, size_t size, cpuset_t *cpuset)
{

	if (pthread__find(thread) != 0)
		return ESRCH;

	if (_sched_getaffinity(getpid(), thread->pt_lid, size, cpuset) < 0)
		return errno;

	return 0;
}

int
pthread_setaffinity_np(pthread_t thread, size_t size, cpuset_t *cpuset)
{

	if (pthread__find(thread) != 0)
		return ESRCH;

	if (_sched_setaffinity(getpid(), thread->pt_lid, size, cpuset) < 0)
		return errno;

	return 0;
}

int
pthread_setschedprio(pthread_t thread, int prio)
{
	struct sched_param sp;

	if (pthread__find(thread) != 0)
		return ESRCH;

	sp.sched_priority = prio;
	if (_sched_setparam(getpid(), thread->pt_lid, SCHED_NONE, &sp) < 0)
		return errno;

	return 0;
}

int
pthread_kill(pthread_t thread, int sig)
{

	if ((sig < 0) || (sig >= _NSIG))
		return EINVAL;
	if (pthread__find(thread) != 0)
		return ESRCH;
	if (_lwp_kill(thread->pt_lid, sig))
		return errno;
	return 0;
}

int
pthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
{

	if (_sys___sigprocmask14(how, set, oset))
		return errno;
	return 0;
}

int
pthread__sched_yield(void)
{
	pthread_t self;
	int error;

	self = pthread__self();

	/* Memory barrier for unlocked mutex release. */
	membar_producer();
	self->pt_blocking++;
	error = _sys_sched_yield();
	self->pt_blocking--;
	membar_sync();

	return error;
}
