/*	$NetBSD: cancelpoint.h,v 1.1 2025/04/05 11:22:32 riastradh Exp $	*/

/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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

#ifndef	TESTS_LIB_LIBPTHREAD_CANCELPOINT_H
#define	TESTS_LIB_LIBPTHREAD_CANCELPOINT_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#include "h_macros.h"

extern pthread_barrier_t bar;
extern bool cleanup_done;

#if 0
atomic_bool cancelpointreadydone;
#endif

static void
cleanup(void *cookie)
{
	bool *cleanup_donep = cookie;

	*cleanup_donep = true;
}

static void
cancelpointready(void)
{

	(void)pthread_barrier_wait(&bar);
	RL(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));
#if 0
	atomic_store_release(&cancelpointreadydone, true);
#endif
}

static void *
thread_cancelpoint(void *cookie)
{
	void (*cancelpoint)(void) = cookie;

	RL(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL));
	(void)pthread_barrier_wait(&bar);

	pthread_cleanup_push(&cleanup, &cleanup_done);
	(*cancelpoint)();
	pthread_cleanup_pop(/*execute*/0);

	return NULL;
}

static void
test_cancelpoint_before(void (*cancelpoint)(void))
{
	pthread_t t;
	void *result;

	RZ(pthread_barrier_init(&bar, NULL, 2));
	RZ(pthread_create(&t, NULL, &thread_cancelpoint, cancelpoint));

	(void)pthread_barrier_wait(&bar);
	fprintf(stderr, "cancel\n");
	RZ(pthread_cancel(t));
	(void)pthread_barrier_wait(&bar);

	alarm(1);
	RZ(pthread_join(t, &result));
	ATF_CHECK_MSG(result == PTHREAD_CANCELED,
	    "result=%p PTHREAD_CANCELED=%p", result, PTHREAD_CANCELED);
	ATF_CHECK(cleanup_done);
}

#if 0
static void
test_cancelpoint_wakeup(void (*cancelpoint)(void))
{
	pthread_t t;

	RZ(pthread_barrier_init(&bar, NULL, 2));
	RZ(pthread_create(&t, NULL, &cancelpoint_thread, cancelpoint));

	(void)pthread_barrier_wait(&bar);
	while (!atomic_load_acquire(&cancelpointreadydone))
		continue;
	while (!pthread_sleeping(t)) /* XXX find a way to do this */
		continue;
	RZ(pthread_cancel(t));
}
#endif

#define	TEST_CANCELPOINT(CANCELPOINT, XFAIL)				      \
ATF_TC(CANCELPOINT);							      \
ATF_TC_HEAD(CANCELPOINT, tc)						      \
{									      \
	atf_tc_set_md_var(tc, "descr", "Test cancellation point: "	      \
	    #CANCELPOINT);						      \
}									      \
ATF_TC_BODY(CANCELPOINT, tc)						      \
{									      \
	XFAIL;								      \
	test_cancelpoint_before(&CANCELPOINT);				      \
}
#define	ADD_TEST_CANCELPOINT(CANCELPOINT)				      \
	ATF_TP_ADD_TC(tp, CANCELPOINT)

#endif	/* TESTS_LIB_LIBPTHREAD_CANCELPOINT_H */
