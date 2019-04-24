/*	$NetBSD: cnd.c,v 1.1 2019/04/24 11:43:19 kamil Exp $	*/

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
__RCSID("$NetBSD: cnd.c,v 1.1 2019/04/24 11:43:19 kamil Exp $");

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <threads.h>

int
cnd_broadcast(cnd_t *cond)
{

	_DIAGASSERT(cond != NULL);

	if (pthread_cond_broadcast(cond) == 0)
		return thrd_success;

	return thrd_error;
}

void
cnd_destroy(cnd_t *cond)
{

	_DIAGASSERT(cond != NULL);

	/*
	 * The cnd_destroy(3) function that conforms to C11 returns no value.
	 */
	(void)pthread_cond_destroy(cond);
}

int
cnd_init(cnd_t *cond)
{

	_DIAGASSERT(cond != NULL);

	if (pthread_cond_init(cond, NULL) == 0)
		return thrd_success;

	return thrd_error;
}

int
cnd_signal(cnd_t *cond)
{

	_DIAGASSERT(cond != NULL);

	if (pthread_cond_signal(cond) == 0)
		return thrd_success;

	return thrd_error;
}

int
cnd_timedwait(cnd_t * __restrict cond, mtx_t * __restrict mtx,
            const struct timespec * __restrict ts)
{

	_DIAGASSERT(cond != NULL);
	_DIAGASSERT(mtx != NULL);
	_DIAGASSERT(ts != NULL);

	switch (pthread_cond_timedwait(cond, mtx, ts)) {
	case 0:
		return thrd_success;
	case ETIMEDOUT:
		return thrd_timedout;
	default:
		return thrd_error;
	}
}

int
cnd_wait(cnd_t *cond, mtx_t *mtx)
{

	_DIAGASSERT(cond != NULL);
	_DIAGASSERT(mtx != NULL);

	if (pthread_cond_wait(cond, mtx) == 0)
		return thrd_success;

	return thrd_error;
}
