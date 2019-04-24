/*	$NetBSD: t_thrd.c,v 1.1 2019/04/24 11:43:19 kamil Exp $	*/

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
__RCSID("$NetBSD: t_thrd.c,v 1.1 2019/04/24 11:43:19 kamil Exp $");

#include <signal.h>
#include <stdbool.h>
#include <threads.h>
#include <time.h>

#include <atf-c.h>

ATF_TC(thrd_create);
ATF_TC_HEAD(thrd_create, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 thrd_create(3)");
}

#define TC_ADDON 5

static int
tcr_func(void *arg)
{
	int a;

	a = (int)(intptr_t)arg;

	return a + TC_ADDON;
}

ATF_TC_BODY(thrd_create, tc)
{
	thrd_t t;
	const int a = 5;
	int b;
	void *v;

	v = (void *)(intptr_t)a;

	ATF_REQUIRE_EQ(thrd_create(&t, tcr_func, v), thrd_success);
	ATF_REQUIRE_EQ(thrd_join(t, &b), thrd_success);
	ATF_REQUIRE_EQ(a + TC_ADDON, b);
}

ATF_TC(thrd_current);
ATF_TC_HEAD(thrd_current, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 thrd_current(3)");
}

static int
tcur_func(void *arg __unused)
{

	return 0;
}

ATF_TC_BODY(thrd_current, tc)
{
	thrd_t s, t;

	s = thrd_current();

	ATF_REQUIRE(thrd_equal(s, s) != 0);
	ATF_REQUIRE_EQ(thrd_create(&t, tcur_func, NULL), thrd_success);
	ATF_REQUIRE(thrd_equal(t, s) == 0);
	ATF_REQUIRE(thrd_equal(s, t) == 0);
	ATF_REQUIRE(thrd_equal(t, t) != 0);

	ATF_REQUIRE_EQ(thrd_join(t, NULL), thrd_success);
}

ATF_TC(thrd_detach);
ATF_TC_HEAD(thrd_detach, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 thrd_detach(3)");
}

static int
tdet_func(void *arg __unused)
{

	return 0;
}

ATF_TC_BODY(thrd_detach, tc)
{
	thrd_t t;

	ATF_REQUIRE_EQ(thrd_create(&t, tdet_func, NULL), thrd_success);
	ATF_REQUIRE_EQ(thrd_detach(t), thrd_success);
}

ATF_TC(thrd_exit);
ATF_TC_HEAD(thrd_exit, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 thrd_exit(3)");
}

static void
tex_func2(void)
{

	thrd_exit(1);
}

static int
tex_func(void *arg __unused)
{

	tex_func2();

	return -1;
}

ATF_TC_BODY(thrd_exit, tc)
{
	thrd_t t;
	int b = 0;

	ATF_REQUIRE_EQ(thrd_create(&t, tex_func, NULL), thrd_success);
	ATF_REQUIRE_EQ(thrd_join(t, &b), thrd_success);
	ATF_REQUIRE_EQ(b, 1);
}

ATF_TC(thrd_sleep);
ATF_TC_HEAD(thrd_sleep, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 thrd_sleep(3)");
}

static int alarmed;

static void
alarm_handler(int signum)
{

	ATF_REQUIRE_EQ(signum, SIGALRM);
	++alarmed;
}

ATF_TC_BODY(thrd_sleep, tc)
{
	struct timespec start, stop, diff;
	struct timespec ts, rem;
	struct timespec zero;
	struct sigaction sa;
	struct itimerval timer;

	zero.tv_sec = 0;
	zero.tv_nsec = 0;

	ts.tv_sec = 1;
	ts.tv_nsec = -1;
	ATF_REQUIRE_EQ(!thrd_sleep(&ts, NULL), 0);

	ts.tv_sec = 0;
	ts.tv_nsec = 1000000000/100; /* 1/100 sec */
	ATF_REQUIRE_EQ(clock_gettime(CLOCK_MONOTONIC, &start), 0);
	ATF_REQUIRE_EQ(thrd_sleep(&ts, &rem), 0);
	ATF_REQUIRE_EQ(clock_gettime(CLOCK_MONOTONIC, &stop), 0);
	timespecsub(&stop, &start, &diff);
	ATF_REQUIRE(timespeccmp(&diff, &ts, >=));
	ATF_REQUIRE(timespeccmp(&zero, &rem, ==));

	ts.tv_sec = 1;
	ts.tv_nsec = 0;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = alarm_handler;
	sigaction(SIGALRM, &sa, NULL);
	memset(&timer, 0, sizeof(timer));
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 100000; /* 100 msec */
	ATF_REQUIRE_EQ(setitimer(ITIMER_MONOTONIC, &timer, NULL), 0);
	ATF_REQUIRE_EQ(clock_gettime(CLOCK_MONOTONIC, &start), 0);
	ATF_REQUIRE_EQ(!thrd_sleep(&ts, &rem), 0);
	ATF_REQUIRE_EQ(clock_gettime(CLOCK_MONOTONIC, &stop), 0);
	timespecsub(&stop, &start, &diff);
	ATF_REQUIRE(timespeccmp(&diff, &ts, <));
	ATF_REQUIRE(timespeccmp(&zero, &rem, !=));
	ATF_REQUIRE_EQ(alarmed, 1);
}

ATF_TC(thrd_yield);
ATF_TC_HEAD(thrd_yield, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test C11 thrd_yield(3)");
}

ATF_TC_BODY(thrd_yield, tc)
{

	thrd_yield();
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, thrd_create);
	ATF_TP_ADD_TC(tp, thrd_current);
	ATF_TP_ADD_TC(tp, thrd_detach);
	ATF_TP_ADD_TC(tp, thrd_exit);
	ATF_TP_ADD_TC(tp, thrd_sleep);
	ATF_TP_ADD_TC(tp, thrd_yield);;

	return atf_no_error();
}
