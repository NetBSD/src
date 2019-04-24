/*	$NetBSD: t_tss.c,v 1.1 2019/04/24 11:43:19 kamil Exp $	*/

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
__RCSID("$NetBSD: t_tss.c,v 1.1 2019/04/24 11:43:19 kamil Exp $");

#include <stdlib.h>
#include <threads.h>

#include <atf-c.h>

ATF_TC(tss_create);
ATF_TC_HEAD(tss_create, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 tss_create(3)");
}

ATF_TC_BODY(tss_create, tc)
{
	tss_t s;

	ATF_REQUIRE_EQ(tss_create(&s, NULL), thrd_success);
	tss_delete(s);
}

ATF_TC(tss_set);
ATF_TC_HEAD(tss_set, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 tss_set(3)");
}

ATF_TC_BODY(tss_set, tc)
{
	tss_t s;
	int b = 5;
	void *v;

	v = (void *)(intptr_t)b;

	ATF_REQUIRE_EQ(tss_create(&s, NULL), thrd_success);
	ATF_REQUIRE_EQ(tss_get(s), NULL);
	ATF_REQUIRE_EQ(tss_set(s, v), thrd_success);
	ATF_REQUIRE_EQ(tss_get(s), v);

	tss_delete(s);
}

ATF_TC(tss_destructor_main_thread);
ATF_TC_HEAD(tss_destructor_main_thread, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test C11 tss(3) destructor in main thread");
}

static void
c_destructor_main(void *arg)
{

	abort();
}

ATF_TC_BODY(tss_destructor_main_thread, tc)
{
	tss_t s;
	int b = 5;
	void *v;

	v = (void *)(intptr_t)b;

	ATF_REQUIRE_EQ(tss_create(&s, c_destructor_main), thrd_success);
	ATF_REQUIRE_EQ(tss_set(s, v), thrd_success);

	tss_delete(s);

	/* Destructor must NOT be called for tss_delete(3) */
}

ATF_TC(tss_destructor_thread_exit);
ATF_TC_HEAD(tss_destructor_thread_exit, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test C11 tss(3) destructor on thread exit");
}

static tss_t s_empty, s_nonempty;
static int c_iter_empty, c_iter_nonempty;

static void
c_destructor_thread_empty(void *arg)
{

	tss_set(s_empty, arg);
	++c_iter_empty;
}

static void
c_destructor_thread_nonempty(void *arg)
{

	tss_set(s_nonempty, arg);
	++c_iter_nonempty;
}

static int
t_func(void *arg __unused)
{
	int b = 5;
	void *v;

	v = (void *)(intptr_t)b;

	ATF_REQUIRE_EQ(tss_set(s_nonempty, v), thrd_success);

	return 0;
}

ATF_TC_BODY(tss_destructor_thread_exit, tc)
{
	thrd_t t;

	ATF_REQUIRE_EQ(tss_create(&s_empty, c_destructor_thread_empty),
	    thrd_success);
	ATF_REQUIRE_EQ(tss_create(&s_nonempty, c_destructor_thread_nonempty),
	    thrd_success);

	ATF_REQUIRE_EQ(thrd_create(&t, t_func, NULL), thrd_success);
	ATF_REQUIRE_EQ(thrd_join(t, NULL), thrd_success);

	ATF_REQUIRE_EQ(c_iter_empty, 0);
	ATF_REQUIRE_EQ(c_iter_nonempty, TSS_DTOR_ITERATIONS);

	tss_delete(s_empty);
	tss_delete(s_nonempty);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, tss_create);
	ATF_TP_ADD_TC(tp, tss_set);
	ATF_TP_ADD_TC(tp, tss_destructor_main_thread);
	ATF_TP_ADD_TC(tp, tss_destructor_thread_exit);

	return atf_no_error();
}
