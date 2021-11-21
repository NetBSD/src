/* $NetBSD: t_timer.c,v 1.4 2021/11/21 09:35:39 hannken Exp $ */

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_timer.c,v 1.4 2021/11/21 09:35:39 hannken Exp $");

#include <sys/types.h>
#include <sys/event.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <atf-c.h>

#include "isqemu.h"

static bool
check_timespec(struct timespec *ts, time_t seconds)
{
	time_t upper = seconds;
	bool result = true;

	/*
	 * If running under QEMU make sure the upper bound is large
	 * enough for the effect of kern/43997
	 */
	if (isQEMU()) {
		upper *= 4;
	}

	if (ts->tv_sec < seconds - 1 ||
	    (ts->tv_sec == seconds - 1 && ts->tv_nsec < 500000000))
		result = false;
	else if (ts->tv_sec > upper ||
	    (ts->tv_sec == upper && ts->tv_nsec >= 500000000))
		result = false;

	printf("time %" PRId64 ".%09ld %sin [ %" PRId64 ".5, %" PRId64 ".5 )\n",
		ts->tv_sec, ts->tv_nsec, (result ? "" : "not "),
		seconds - 1, upper);

	return result;
}

ATF_TC(basic_timer);
ATF_TC_HEAD(basic_timer, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests basic EVFILT_TIMER functionality");
}

#define	TIME1		1000		/* 1000ms -> 1s */
#define	TIME1_COUNT	5
#define	TIME2		6000		/* 6000ms -> 6s */

#define	TIME1_TOTAL_SEC	((TIME1 * TIME1_COUNT) / 1000)
#define	TIME2_TOTAL_SEC	(TIME2 / 1000)

ATF_TC_BODY(basic_timer, tc)
{
	struct kevent event[2];
	int ntimer1 = 0, ntimer2 = 0;
	struct timespec ots, ts;
	int kq;

	ATF_REQUIRE((kq = kqueue()) >= 0);

	EV_SET(&event[0], 1, EVFILT_TIMER, EV_ADD, 0, TIME1, NULL);
	EV_SET(&event[1], 2, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, TIME2, NULL);

	ATF_REQUIRE(kevent(kq, event, 2, NULL, 0, NULL) == 0);
	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &ots) == 0);

	for (;;) {
		ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, NULL) == 1);
		ATF_REQUIRE(event[0].filter == EVFILT_TIMER);
		ATF_REQUIRE(event[0].ident == 1 ||
			    event[0].ident == 2);
		if (event[0].ident == 1) {
			ATF_REQUIRE(ntimer1 < TIME1_COUNT);
			if (++ntimer1 == TIME1_COUNT) {
				/*
				 * Make sure TIME1_TOTAL_SEC seconds have
				 * elapsed, allowing for a little slop.
				 */
				ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC,
				    &ts) == 0);
				timespecsub(&ts, &ots, &ts);
				ATF_REQUIRE(check_timespec(&ts,
				    TIME1_TOTAL_SEC));
				EV_SET(&event[0], 1, EVFILT_TIMER, EV_DELETE,
				    0, 0, NULL);
				ATF_REQUIRE(kevent(kq, event, 1, NULL, 0,
				    NULL) == 0);
			}
		} else {
			ATF_REQUIRE(ntimer1 == TIME1_COUNT);
			ATF_REQUIRE(ntimer2 == 0);
			ntimer2++;
			/*
			 * Make sure TIME2_TOTAL_SEC seconds have
			 * elapsed, allowing for a little slop.
			 */
			ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC,
			    &ts) == 0);
			timespecsub(&ts, &ots, &ts);
			ATF_REQUIRE(check_timespec(&ts, TIME2_TOTAL_SEC));
			EV_SET(&event[0], 2, EVFILT_TIMER, EV_DELETE,
			    0, 0, NULL);
			ATF_REQUIRE_ERRNO(ENOENT,
			    kevent(kq, event, 1, NULL, 0, NULL) == -1);
			break;
		}
	}

	/*
	 * Now block in kqueue for TIME2_TOTAL_SEC, and ensure we
	 * don't receive any new events.
	 */
	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &ots) == 0);
	ts.tv_sec = TIME2_TOTAL_SEC;
	ts.tv_nsec = 0;
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 0);
	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &ts) == 0);
	timespecsub(&ts, &ots, &ts);
	ATF_REQUIRE(check_timespec(&ts, TIME2_TOTAL_SEC));
}

ATF_TC(count_expirations);
ATF_TC_HEAD(count_expirations, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests counting timer expirations");
}

ATF_TC_BODY(count_expirations, tc)
{
	struct kevent event[1];
	struct timespec ts = { 0, 0 };
	struct timespec sleepts;
	int kq;

	ATF_REQUIRE((kq = kqueue()) >= 0);

	EV_SET(&event[0], 1, EVFILT_TIMER, EV_ADD, 0, TIME1, NULL);
	ATF_REQUIRE(kevent(kq, event, 1, NULL, 0, NULL) == 0);

	/* Sleep a little longer to mitigate timing jitter. */
	sleepts.tv_sec = TIME1_TOTAL_SEC;
	sleepts.tv_nsec = 500000000;
	ATF_REQUIRE(nanosleep(&sleepts, NULL) == 0);

	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].ident == 1);
	ATF_REQUIRE(event[0].data == TIME1_COUNT ||
		    event[0].data == TIME1_COUNT + 1);
}

ATF_TC(modify);
ATF_TC_HEAD(modify, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests modifying a timer");
}

ATF_TC_BODY(modify, tc)
{
	struct kevent event[1];
	struct timespec ts = { 0, 0 };
	struct timespec sleepts;
	int kq;

	ATF_REQUIRE((kq = kqueue()) >= 0);

	/*
	 * Start a 500ms timer, sleep for 5 seconds, and check
	 * the total count.
	 */
	EV_SET(&event[0], 1, EVFILT_TIMER, EV_ADD, 0, 500, NULL);
	ATF_REQUIRE(kevent(kq, event, 1, NULL, 0, NULL) == 0);

	sleepts.tv_sec = 5;
	sleepts.tv_nsec = 0;
	ATF_REQUIRE(nanosleep(&sleepts, NULL) == 0);

	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].ident == 1);
	ATF_REQUIRE(event[0].data >= 9 && event[0].data <= 11);

	/*
	 * Modify to a 4 second timer, sleep for 5 seconds, and check
	 * the total count.
	 */
	EV_SET(&event[0], 1, EVFILT_TIMER, EV_ADD, 0, 4000, NULL);
	ATF_REQUIRE(kevent(kq, event, 1, NULL, 0, NULL) == 0);

	/*
	 * Before we sleep, verify that the knote for this timer is
	 * no longer activated.
	 */
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 0);

	sleepts.tv_sec = 5;
	sleepts.tv_nsec = 0;
	ATF_REQUIRE(nanosleep(&sleepts, NULL) == 0);

	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].ident == 1);
	ATF_REQUIRE(event[0].data == 1);

	/*
	 * Start a 500ms timer, sleep for 2 seconds.
	 */
	EV_SET(&event[0], 1, EVFILT_TIMER, EV_ADD, 0, 500, NULL);
	ATF_REQUIRE(kevent(kq, event, 1, NULL, 0, NULL) == 0);

	sleepts.tv_sec = 2;
	sleepts.tv_nsec = 0;
	ATF_REQUIRE(nanosleep(&sleepts, NULL) == 0);

	/*
	 * Set the SAME timer, sleep for 2 seconds.
	 */
	EV_SET(&event[0], 1, EVFILT_TIMER, EV_ADD, 0, 500, NULL);
	ATF_REQUIRE(kevent(kq, event, 1, NULL, 0, NULL) == 0);

	sleepts.tv_sec = 2;
	sleepts.tv_nsec = 0;
	ATF_REQUIRE(nanosleep(&sleepts, NULL) == 0);

	/*
	 * The kernel should have reset the count when modifying the
	 * timer, so we should only expect to see the expiration count
	 * for the second sleep.
	 */
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].ident == 1);
	ATF_REQUIRE(event[0].data >= 3 && event[0].data <= 5);
}

ATF_TC(abstime);
ATF_TC_HEAD(abstime, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests timers with NOTE_ABSTIME");
}

ATF_TC_BODY(abstime, tc)
{
	struct kevent event[1];
	struct timespec ts, ots;
	time_t seconds;
	int kq;

	ATF_REQUIRE((kq = kqueue()) >= 0);

	ATF_REQUIRE(clock_gettime(CLOCK_REALTIME, &ots) == 0);
	ATF_REQUIRE(ots.tv_sec < INTPTR_MAX - TIME1_TOTAL_SEC);

	seconds = ots.tv_sec + TIME1_TOTAL_SEC;

	EV_SET(&event[0], 1, EVFILT_TIMER, EV_ADD,
	    NOTE_ABSTIME | NOTE_SECONDS, seconds, NULL);
	ATF_REQUIRE(kevent(kq, event, 1, event, 1, NULL) == 1);

	ATF_REQUIRE(clock_gettime(CLOCK_REALTIME, &ts) == 0);
	timespecsub(&ts, &ots, &ts);

	/*
	 * We're not going for precision here; just verify that it was
	 * delivered anywhere between 4.5-6.whatever seconds later.
	 */
	ATF_REQUIRE(check_timespec(&ts, 4) || check_timespec(&ts, 5));

	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
}

#define	PREC_TIMEOUT_SEC	2

static void
do_test_timer_units(const char *which, uint32_t fflag, int64_t data)
{
	struct kevent event[1];
	struct timespec ts, ots;
	int kq;

	ATF_REQUIRE((kq = kqueue()) >= 0);

	EV_SET(&event[0], 1, EVFILT_TIMER, EV_ADD | EV_ONESHOT,
	    fflag, data, NULL);

	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &ots) == 0); 
	ATF_REQUIRE(kevent(kq, event, 1, event, 1, NULL) == 1);
	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &ts) == 0);

	timespecsub(&ts, &ots, &ts);
	ATF_REQUIRE_MSG(check_timespec(&ts, PREC_TIMEOUT_SEC),
	    "units '%s' failed", which);

	(void)close(kq);
}

#define	test_timer_units(fflag, data)				\
	do_test_timer_units(#fflag, fflag, data)

ATF_TC(timer_units);
ATF_TC_HEAD(timer_units, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests timers with NOTE_* units modifiers");
}

ATF_TC_BODY(timer_units, tc)
{
	test_timer_units(NOTE_SECONDS,  PREC_TIMEOUT_SEC);
	test_timer_units(NOTE_MSECONDS, PREC_TIMEOUT_SEC * 1000);
	test_timer_units(NOTE_USECONDS, PREC_TIMEOUT_SEC * 1000000);
	test_timer_units(NOTE_NSECONDS, PREC_TIMEOUT_SEC * 1000000000);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, basic_timer);
	ATF_TP_ADD_TC(tp, count_expirations);
	ATF_TP_ADD_TC(tp, abstime);
	ATF_TP_ADD_TC(tp, timer_units);
	ATF_TP_ADD_TC(tp, modify);

	return atf_no_error();
}
