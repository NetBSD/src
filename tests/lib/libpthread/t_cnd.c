/*	$NetBSD: t_cnd.c,v 1.1 2019/04/24 11:43:19 kamil Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 2019\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_cnd.c,v 1.1 2019/04/24 11:43:19 kamil Exp $");

#include <stdbool.h>
#include <threads.h>
#include <time.h>

#include <atf-c.h>

ATF_TC(cnd_init);
ATF_TC_HEAD(cnd_init, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 cnd_init(3)");
}

ATF_TC_BODY(cnd_init, tc)
{
	cnd_t c;

	ATF_REQUIRE_EQ(cnd_init(&c), thrd_success);
	cnd_destroy(&c);
}

#define B_THREADS 8

static mtx_t b_m;
static cnd_t b_c;
static bool b_finished;

static int
b_func(void *arg)
{
	bool signal;

	signal = (bool)(intptr_t)arg;

	ATF_REQUIRE_EQ(mtx_lock(&b_m), thrd_success);
	while (!b_finished) {
		ATF_REQUIRE_EQ(cnd_wait(&b_c, &b_m), thrd_success);
	}
	ATF_REQUIRE_EQ(mtx_unlock(&b_m), thrd_success);

	if (signal) {
		ATF_REQUIRE_EQ(cnd_signal(&b_c), thrd_success);
	}

	return 0;
}

static void
cnd_notify(bool signal)
{
	size_t i;
	thrd_t t[B_THREADS];
	void *n;

	n = (void *)(intptr_t)signal;

	ATF_REQUIRE_EQ(mtx_init(&b_m, mtx_plain), thrd_success);
	ATF_REQUIRE_EQ(cnd_init(&b_c), thrd_success);

	for (i = 0; i < B_THREADS; i++) {
		ATF_REQUIRE_EQ(thrd_create(&t[i], b_func, n), thrd_success);
	}

	ATF_REQUIRE_EQ(mtx_lock(&b_m), thrd_success);
	b_finished = true;
	ATF_REQUIRE_EQ(mtx_unlock(&b_m), thrd_success);

	if (signal) {
		ATF_REQUIRE_EQ(cnd_signal(&b_c), thrd_success);
	} else {
		ATF_REQUIRE_EQ(cnd_broadcast(&b_c), thrd_success);
	}

	for (i = 0; i < B_THREADS; i++) {
		ATF_REQUIRE_EQ(thrd_join(t[i], NULL), thrd_success);
	}

	mtx_destroy(&b_m);
	cnd_destroy(&b_c);
}

ATF_TC(cnd_broadcast);
ATF_TC_HEAD(cnd_broadcast, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 cnd_broadcast(3)");
}

ATF_TC_BODY(cnd_broadcast, tc)
{

	cnd_notify(false);
}

ATF_TC(cnd_signal);
ATF_TC_HEAD(cnd_signal, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 cnd_signal(3)");
}

ATF_TC_BODY(cnd_signal, tc)
{

	cnd_notify(true);
}

ATF_TC(cnd_timedwait);
ATF_TC_HEAD(cnd_timedwait, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 cnd_timedwait(3)");
}

ATF_TC_BODY(cnd_timedwait, tc)
{
	mtx_t m;
	cnd_t c;
	struct timespec ts;

	ts.tv_sec = 0;
	ts.tv_nsec = 1;

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_plain), thrd_success);
	ATF_REQUIRE_EQ(cnd_init(&c), thrd_success);

	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(cnd_timedwait(&c, &m, &ts), thrd_timedout);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);

	mtx_destroy(&m);
	cnd_destroy(&c);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, cnd_init);
	ATF_TP_ADD_TC(tp, cnd_broadcast);
	ATF_TP_ADD_TC(tp, cnd_signal);
	ATF_TP_ADD_TC(tp, cnd_timedwait);

	return atf_no_error();
}
