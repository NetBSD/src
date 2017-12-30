/* $NetBSD: t_timeleft.c,v 1.2 2017/12/30 17:06:27 martin Exp $ */

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__COPYRIGHT("@(#) Copyright (c) 2008\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_timeleft.c,v 1.2 2017/12/30 17:06:27 martin Exp $");

#include <sys/types.h>
#include <sys/select.h>

#include <atf-c.h>
#include <time.h>
#include <errno.h>
#include <lwp.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>

static void
sighandler(int signo __unused)
{
}

struct info {
	void (*fun)(struct timespec *);
	struct timespec ts;
};

static void
timeleft__lwp_park(struct timespec *ts)
{
	ATF_REQUIRE_ERRNO(EINTR, _lwp_park(CLOCK_MONOTONIC, TIMER_RELTIME,
	    ts, 0, ts, NULL) == -1);
}

#if 0
static void
timeleft_pselect(struct timespec *ts)
{
	ATF_REQUIRE_ERRNO(EINTR, pselect(1, NULL, NULL, NULL, ts, NULL));
}
#endif

static void *
runner(void *arg)
{
	struct info *i = arg;
	(*i->fun)(&i->ts);
	return NULL;
}

static void
tester(void (*fun)(struct timespec *))
{
	const struct timespec ts = { 5, 0 };
	const struct timespec sts = { 0, 2000000 };
	struct info i = { fun, ts };
	pthread_t thr;

	ATF_REQUIRE(signal(SIGINT, sighandler) == 0);
	ATF_REQUIRE(pthread_create(&thr, NULL, runner, &i) == 0);

	nanosleep(&sts, NULL);
	pthread_kill(thr, SIGINT);
	printf("Orig time %ju.%lu\n", (intmax_t)ts.tv_sec, ts.tv_nsec);
	printf("Time left %ju.%lu\n", (intmax_t)i.ts.tv_sec, i.ts.tv_nsec);
	ATF_REQUIRE(timespeccmp(&i.ts, &ts, <));
}

ATF_TC(timeleft__lwp_park);
ATF_TC_HEAD(timeleft__lwp_park, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks that _lwp_park(2) returns "
	    "the time left to sleep after interrupted");
}

ATF_TC_BODY(timeleft__lwp_park, tc)
{
	tester(timeleft__lwp_park);
}

#if 0
ATF_TC(timeleft_pselect);
ATF_TC_HEAD(timeleft_pselect, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks that pselect(2) returns "
	    "the time left to sleep after interrupted");
}

ATF_TC_BODY(timeleft_pselect, tc)
{
	tester(timeleft_pselect);
}
#endif

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, timeleft__lwp_park);
#if 0
	ATF_TP_ADD_TC(tp, timeleft_pselect);
#endif

	return atf_no_error();
}
