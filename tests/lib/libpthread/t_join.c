/* $NetBSD: t_join.c,v 1.4 2010/07/29 12:56:16 hans Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jukka Ruohonen.
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
__RCSID("$NetBSD: t_join.c,v 1.4 2010/07/29 12:56:16 hans Exp $");

#include <errno.h>
#include <pthread.h>

#include <atf-c.h>

#include "h_common.h"

static bool error;

static void *threadfunc1(void *);
static void *threadfunc2(void *);

ATF_TC(pthread_join);
ATF_TC_HEAD(pthread_join, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks basic error conditions in pthread_join(3)");
}

ATF_TC_BODY(pthread_join, tc)
{
	pthread_t thread;

	PTHREAD_REQUIRE(pthread_create(&thread, NULL, threadfunc1, NULL));
	PTHREAD_REQUIRE(pthread_join(thread, NULL));
}

static void *
threadfunc1(void *arg)
{
	pthread_t thread[25];
	pthread_t caller;
	void *val = NULL;
	uintptr_t i;
	int rv;

	caller = pthread_self();

	/*
	 * The behavior is undefined, but should error
	 * out, if we try to join the calling thread.
	 */
	rv = pthread_join(caller, NULL);

	/*
	 * The specification recommends EDEADLK.
	 */
	ATF_REQUIRE(rv != 0);
	ATF_REQUIRE_EQ(rv, EDEADLK);

	for (i = 0; i < __arraycount(thread); i++) {

		error = true;

		rv = pthread_create(&thread[i], NULL, threadfunc2, (void *)i);

		ATF_REQUIRE_EQ(rv, 0);

		/*
		 * Try to join an invalid thread.
		 */
		rv = pthread_join(thread[i + 1], NULL);

		ATF_REQUIRE_EQ(rv, ESRCH);

		/*
		 * Check join and exit condition.
		 */
		PTHREAD_REQUIRE(pthread_join(thread[i], &val));

		ATF_REQUIRE_EQ(error, false);

		ATF_REQUIRE(val != NULL);
		ATF_REQUIRE(val == (void *)(i + 1));

		/*
		 * Once the thread has returned, ESRCH should
		 * again follow if we try to join it again.
		 */
		rv = pthread_join(thread[i], NULL);

		ATF_REQUIRE_EQ(rv, ESRCH);

		/*
		 * Try to detach the exited thread.
		 */
		rv = pthread_detach(thread[i]);

		ATF_REQUIRE(rv != 0);
	}

	pthread_exit(NULL);

	return NULL;
}

static void *
threadfunc2(void *arg)
{
	static uintptr_t i = 0;
	uintptr_t j;

	j = (uintptr_t)arg;

	if (i++ == j)
		error = false;

	pthread_exit((void *)i);

	return NULL;
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, pthread_join);

	return atf_no_error();
}
