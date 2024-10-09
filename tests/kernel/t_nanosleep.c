/* $NetBSD: t_nanosleep.c,v 1.1 2024/10/09 13:02:53 kre Exp $ */

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2024\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_nanosleep.c,v 1.1 2024/10/09 13:02:53 kre Exp $");

#include <sys/types.h>
#include <sys/wait.h>

#include <atf-c.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static void
sacrifice(void)
{
	pause();
}

static void
tester(pid_t victim, clockid_t clock, int flags)
{
	/*
	 * we need this sleep to be long enough that we
	 * can accurately detect when the sleep finishes
	 * early, but not so long that when there's no
	 * bug and things actually sleep this long, that
	 * the execution of a sleep this long, several
	 * times, won't slow down the overall testing
	 * process too much.    Trial and error...
	 */
	struct timespec to_sleep = { 4, 0 };

	struct timespec before, after;
	struct timespec *ts;
	int e;

	if (clock_gettime(clock, &before) != 0)
		exit(1);

	if (flags & TIMER_ABSTIME) {
		timespecadd(&to_sleep, &before, &after);
		ts = &after;
	} else
		ts = &to_sleep;

	printf("Test: Clock=%d Flags=%x, starting at %jd.%.9ld\n",
		(int)clock, flags, (intmax_t)before.tv_sec, before.tv_nsec);
	if (flags & TIMER_ABSTIME)
		printf("Sleeping until %jd.%.9ld\n",
		    (intmax_t)ts->tv_sec, ts->tv_nsec);
	else
		printf("Sleeping for %jd.%.9ld\n",
		    (intmax_t)ts->tv_sec, ts->tv_nsec);

	/* OK, we're ready */

	/* these next two steps need to be as close together as possible */
	if (kill(victim, SIGKILL) == -1)
		exit(2);
	if ((e = clock_nanosleep(clock, flags, ts, &after)) != 0)
		exit(20 + e);

	if (!(flags & TIMER_ABSTIME)) {
		printf("Remaining to sleep: %jd.%.9ld\n",
		    (intmax_t)after.tv_sec, after.tv_nsec);

		if (after.tv_sec != 0 || after.tv_nsec != 0)
			exit(3);
	}

	if (clock_gettime(clock, &after) != 0)
		exit(4);

	printf("Sleep ended at: %jd.%.9ld\n",
		(intmax_t)after.tv_sec, after.tv_nsec);

	timespecadd(&before, &to_sleep, &before);
	if (timespeccmp(&before, &after, >))
		exit(5);

	exit(0);
}

/*
 * The parent of the masochist/victim above, controls everything.
 */
static void
runit(clockid_t clock, int flags)
{
	pid_t v, m, x;
	int status;
	struct timespec brief = { 0, 3 * 100 * 1000 * 1000 };  /* 300 ms */

	ATF_REQUIRE((v = fork()) != -1);
	if (v == 0)
		sacrifice();

	ATF_REQUIRE((m = fork()) != -1);
	if (m == 0)
		tester(v, clock, flags);

	ATF_REQUIRE((x = wait(&status)) != -1);

	if (x == m) {
		/*
		 * This is bad, the murderer shouldn't die first
		 */
		fprintf(stderr, "M exited first, status %#x\n", status);
		(void)kill(v, SIGKILL);	/* just in case */
		atf_tc_fail("2nd child predeceased first");
	}
	if (x != v) {
		fprintf(stderr, "Unknown exit from %d (status: %#x)"
		    "(M=%d V=%d)\n", x, status, m, v);
		(void)kill(m, SIGKILL);
		(void)kill(v, SIGKILL);
		atf_tc_fail("Strange child died");
	}

	/*
	 * OK, the victim died, we don't really care why,
	 * (it should have been because of a SIGKILL, maybe
	 * test for that someday).
	 *
	 * Now we get to proceed to the real test.
	 *
	 * But we want to wait a short whle to try and be sure
	 * that m (the child still running) has a chance to
	 * fall asleep.
	 */
	(void) clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &brief, NULL);

	/*
	 * This is the test, for PR kern/58733
	 *   -  stop a process while in clock_nanosleep()
	 *   -  resume it again
	 *   -  see if it still sleeps as long as was requested (or longer)
	 */
	ATF_REQUIRE(kill(m, SIGSTOP) == 0);
	(void) clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &brief, NULL);
	ATF_REQUIRE(kill(m, SIGCONT) == 0);

	ATF_REQUIRE((x = wait(&status)) != -1);

	if (x != m) {
		fprintf(stderr, "Unknown exit from %d (status: %#x)"
		    "(M=%d V=%d)\n", x, status, m, v);
		(void) kill(m, SIGKILL);
		atf_tc_fail("Strange child died");
	}

	if (status == 0)
		atf_tc_pass();

	/*
	 * Here we should decode the status, and give a better
	 * clue what really went wrong.   Later...
	 */
	fprintf(stderr, "Test failed: status from M: %#x\n", status);
	atf_tc_fail("M exited with non-zero status.  PR kern/58733");
}


ATF_TC(nanosleep_monotonic_absolute);
ATF_TC_HEAD(nanosleep_monotonic_absolute, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks clock_nanosleep(MONO, ABS)");
}
ATF_TC_BODY(nanosleep_monotonic_absolute, tc)
{
	runit(CLOCK_MONOTONIC, TIMER_ABSTIME);
}

ATF_TC(nanosleep_monotonic_relative);
ATF_TC_HEAD(nanosleep_monotonic_relative, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks clock_nanosleep(MONO, REL)");
}
ATF_TC_BODY(nanosleep_monotonic_relative, tc)
{
	runit(CLOCK_MONOTONIC, TIMER_RELTIME);
}

ATF_TC(nanosleep_realtime_absolute);
ATF_TC_HEAD(nanosleep_realtime_absolute, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks clock_nanosleep(REAL, ABS)");
}
ATF_TC_BODY(nanosleep_realtime_absolute, tc)
{
	runit(CLOCK_REALTIME, TIMER_ABSTIME);
}

ATF_TC(nanosleep_realtime_relative);
ATF_TC_HEAD(nanosleep_realtime_relative, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks clock_nanosleep(REAL, REL)");
}
ATF_TC_BODY(nanosleep_realtime_relative, tc)
{
	runit(CLOCK_REALTIME, TIMER_RELTIME);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, nanosleep_monotonic_absolute);
	ATF_TP_ADD_TC(tp, nanosleep_monotonic_relative);
	ATF_TP_ADD_TC(tp, nanosleep_realtime_absolute);
	ATF_TP_ADD_TC(tp, nanosleep_realtime_relative);

	return atf_no_error();
}
