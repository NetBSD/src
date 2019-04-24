/*	$NetBSD: t_mtx.c,v 1.1 2019/04/24 11:43:19 kamil Exp $	*/

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
__RCSID("$NetBSD: t_mtx.c,v 1.1 2019/04/24 11:43:19 kamil Exp $");

#include <time.h>
#include <threads.h>

#include <atf-c.h>

ATF_TC(mtx_init);
ATF_TC_HEAD(mtx_init, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 mtx_init(3)");
}

ATF_TC_BODY(mtx_init, tc)
{
	mtx_t m;

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_plain), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_plain | mtx_recursive), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_timed), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_timed | mtx_recursive), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_recursive), thrd_error);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_plain | mtx_timed), thrd_error);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, -1), thrd_error);
	mtx_destroy(&m);
}

ATF_TC(mtx_lock);
ATF_TC_HEAD(mtx_lock, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 mtx_lock(3)");
}

ATF_TC_BODY(mtx_lock, tc)
{
	mtx_t m;

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_plain), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_timed), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_plain | mtx_recursive), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_timed | mtx_recursive), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);
}

ATF_TC(mtx_timedlock);
ATF_TC_HEAD(mtx_timedlock, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 mtx_timedlock(3)");
}

ATF_TC_BODY(mtx_timedlock, tc)
{
	mtx_t m;
	struct timespec ts;

	ts.tv_sec = 0;
	ts.tv_nsec = 1;

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_timed), thrd_success);
	ATF_REQUIRE_EQ(mtx_timedlock(&m, &ts), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_timed), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_timedlock(&m, &ts), thrd_timedout);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_timed | mtx_recursive), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_timedlock(&m, &ts), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_timed | mtx_recursive), thrd_success);
	ATF_REQUIRE_EQ(mtx_timedlock(&m, &ts), thrd_success);
	ATF_REQUIRE_EQ(mtx_timedlock(&m, &ts), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_timed | mtx_recursive), thrd_success);
	ATF_REQUIRE_EQ(mtx_timedlock(&m, &ts), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);
}

ATF_TC(mtx_trylock);
ATF_TC_HEAD(mtx_trylock, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 mtx_trylock(3)");
}

ATF_TC_BODY(mtx_trylock, tc)
{
	mtx_t m;

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_plain), thrd_success);
	ATF_REQUIRE_EQ(mtx_trylock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_plain), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_trylock(&m), thrd_busy);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_plain | mtx_recursive), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_trylock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_timed), thrd_success);
	ATF_REQUIRE_EQ(mtx_trylock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_timed), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_trylock(&m), thrd_busy);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);

	ATF_REQUIRE_EQ(mtx_init(&m, mtx_timed | mtx_recursive), thrd_success);
	ATF_REQUIRE_EQ(mtx_lock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_trylock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	ATF_REQUIRE_EQ(mtx_unlock(&m), thrd_success);
	mtx_destroy(&m);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, mtx_init);
	ATF_TP_ADD_TC(tp, mtx_lock);
	ATF_TP_ADD_TC(tp, mtx_timedlock);
	ATF_TP_ADD_TC(tp, mtx_trylock);

	return atf_no_error();
}
