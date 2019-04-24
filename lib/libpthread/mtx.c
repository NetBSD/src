/*	$NetBSD: mtx.c,v 1.1 2019/04/24 11:43:19 kamil Exp $	*/

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
__RCSID("$NetBSD: mtx.c,v 1.1 2019/04/24 11:43:19 kamil Exp $");

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <threads.h>

void
mtx_destroy(mtx_t *mtx)
{

	_DIAGASSERT(mtx != NULL);

	/*
	 * The cnd_destroy(3) function that conforms to C11 returns no value.
	 */
	(void)pthread_mutex_destroy(mtx);
}

static inline int
mtx_init_default(mtx_t *mtx)
{

	_DIAGASSERT(mtx != NULL);

	if (pthread_mutex_init(mtx, NULL) != 0)
		return thrd_error;

        return thrd_success;
}

static inline int
mtx_init_recursive(mtx_t *mtx)
{
	pthread_mutexattr_t attr;

	_DIAGASSERT(mtx != NULL);

	if (pthread_mutexattr_init(&attr) != 0)
		return thrd_error;

	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
		pthread_mutexattr_destroy(&attr);

		return thrd_error;
	}

	if (pthread_mutex_init(mtx, &attr) == 0)
		return thrd_success;

	pthread_mutexattr_destroy(&attr);

	return thrd_error;
}

int
mtx_init(mtx_t *mtx, int type)
{

	_DIAGASSERT(mtx != NULL);

	switch (type) {
	case mtx_plain:
	case mtx_timed:
		return mtx_init_default(mtx);
	case mtx_plain | mtx_recursive:
	case mtx_timed | mtx_recursive:
		return mtx_init_recursive(mtx);
	default:
		return thrd_error;
	}
}

int
mtx_lock(mtx_t *mtx)
{

	_DIAGASSERT(mtx != NULL);

	if (pthread_mutex_lock(mtx) == 0)
		return thrd_success;

	return thrd_error;
}

int
mtx_timedlock(mtx_t *__restrict mtx, const struct timespec *__restrict ts)
{

	_DIAGASSERT(mtx != NULL);
	_DIAGASSERT(ts != NULL);

	switch(pthread_mutex_timedlock(mtx, ts)) {
	case 0:
		return thrd_success;
	case ETIMEDOUT:
		return thrd_timedout;
	default:
		return thrd_error;
	}
}

int
mtx_trylock(mtx_t *mtx)
{

	_DIAGASSERT(mtx != NULL);

	switch(pthread_mutex_trylock(mtx)) {
	case 0:
		return thrd_success;
	case EBUSY:
		return thrd_busy;
	default:
		return thrd_error;
	}
}

int
mtx_unlock(mtx_t *mtx)
{

	_DIAGASSERT(mtx != NULL);

	if (pthread_mutex_unlock(mtx) == 0)
		return thrd_success;

	return thrd_error;
}
