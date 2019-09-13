/*	$NetBSD: thrd.c,v 1.3.4.1 2019/09/13 06:54:09 martin Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Kamil Rytarowski.
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
__RCSID("$NetBSD: thrd.c,v 1.3.4.1 2019/09/13 06:54:09 martin Exp $");

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <time.h>
#include <threads.h>

struct __thrd_tramp_data {
	thrd_start_t func;
	void *arg;
};

static void *
__thrd_create_tramp(void *arg)
{
	struct __thrd_tramp_data *cookie;
	int ret;

	_DIAGASSERT(arg != NULL);

	cookie = (struct __thrd_tramp_data *)arg;

	ret = (cookie->func)(cookie->arg);

	free(cookie);

	return (void *)(intptr_t)ret;
}

int
thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
	struct __thrd_tramp_data *cookie;
	int error;

	_DIAGASSERT(thr != NULL);
	_DIAGASSERT(func != NULL);

	cookie = malloc(sizeof(*cookie));
	if (cookie == NULL)
		return thrd_nomem;

	cookie->func = func;
	cookie->arg = arg;

	switch(pthread_create(thr, NULL, __thrd_create_tramp, cookie)) {
	case 0:
		return thrd_success;
	case ENOMEM:
		error = thrd_nomem;
		break;
	default:
		error = thrd_error;
	}

	free(cookie);

	return error;
}

thrd_t
thrd_current(void)
{

	return pthread_self();
}

int
thrd_detach(thrd_t thr)
{

	_DIAGASSERT(thr != NULL);

	if (pthread_detach(thr) == 0)
		return thrd_success;

	return thrd_error;
}

int
thrd_equal(thrd_t t1, thrd_t t2)
{

	_DIAGASSERT(t1 != NULL);
	_DIAGASSERT(t2 != NULL);

	return pthread_equal(t1, t2);
}

__dead void
thrd_exit(int res)
{

	pthread_exit((void *)(intptr_t)res);
}

int
thrd_join(thrd_t thrd, int *res)
{
	void *ptr;

	_DIAGASSERT(thrd != NULL);

	if (pthread_join(thrd, &ptr) == 0) {
		if (res)
			*res = (int)(intptr_t)ptr;

		return thrd_success;
	}

	return thrd_error;
}

int
thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{

	_DIAGASSERT(duration != NULL);

	/* Use clock_nanosleep(3) to skip handling errno */

	switch (clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, duration,
	    remaining)) {
	case 0:
		return 0;
	case EINTR:
		return -1;
	default:
		/* Negative value different than -1 */
		return -2;
	}
}

void
thrd_yield(void)
{

	sched_yield();
}
