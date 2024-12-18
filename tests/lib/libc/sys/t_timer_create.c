/*	$NetBSD: t_timer_create.c,v 1.6 2024/12/18 22:26:53 riastradh Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#include <atf-c.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "h_macros.h"

static timer_t t;
static sig_atomic_t expired;

enum mode {
	PAST,
	EXPIRE,
	NOEXPIRE,
};

static void
timer_signal_handler(int signo, siginfo_t *si, void *osi __unused)
{
	timer_t *tp;

	tp = si->si_value.sival_ptr;

	if (*tp == t && signo == SIGALRM)
		expired = 1;

	(void)fprintf(stderr, "%s: %s\n", __func__, strsignal(signo));
}

static void
timer_signal_create(clockid_t cid, enum mode mode, int flags)
{
	struct itimerspec tim, rtim, otim;
	struct timespec t0, t1, dt;
	struct sigaction act;
	struct sigevent evt;
	sigset_t set;

	t = 0;
	expired = 0;

	(void)memset(&evt, 0, sizeof(struct sigevent));
	(void)memset(&act, 0, sizeof(struct sigaction));
	(void)memset(&tim, 0, sizeof(struct itimerspec));

	/*
	 * Set handler.
	 */
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = timer_signal_handler;

	ATF_REQUIRE(sigemptyset(&set) == 0);
	ATF_REQUIRE(sigemptyset(&act.sa_mask) == 0);

	/*
	 * Block SIGALRM while configuring the timer.
	 */
	ATF_REQUIRE(sigaction(SIGALRM, &act, NULL) == 0);
	ATF_REQUIRE(sigaddset(&set, SIGALRM) == 0);
	ATF_REQUIRE(sigprocmask(SIG_SETMASK, &set, NULL) == 0);

	/*
	 * Create the timer (SIGEV_SIGNAL).
	 */
	evt.sigev_signo = SIGALRM;
	evt.sigev_value.sival_ptr = &t;
	evt.sigev_notify = SIGEV_SIGNAL;

	ATF_REQUIRE(timer_create(cid, &evt, &t) == 0);

	/*
	 * Configure the timer for -1, 1, or 5 sec from now, depending
	 * on whether we want it to have fired, to fire within 2sec, or
	 * to not fire within 2sec.
	 */
	switch (mode) {
	case PAST:
		tim.it_value.tv_sec = -1;
		break;
	case EXPIRE:
		tim.it_value.tv_sec = 1;
		break;
	case NOEXPIRE:
		tim.it_value.tv_sec = 5;
		break;
	}
	tim.it_value.tv_nsec = 0;

	/*
	 * Save the relative time and adjust for absolute time of
	 * requested.
	 */
	rtim = tim;
	RL(clock_gettime(cid, &t0));
	if (flags & TIMER_ABSTIME)
		timespecadd(&t0, &tim.it_value, &tim.it_value);

	fprintf(stderr, "now is %lld sec %d nsec\n",
	    (long long)t0.tv_sec, (int)t0.tv_nsec);
	fprintf(stderr, "expire at %lld sec %d nsec\n",
	    (long long)tim.it_value.tv_sec, (int)tim.it_value.tv_nsec);
	if (mode == PAST && (flags & TIMER_ABSTIME) == 0) {
		atf_tc_expect_fail("PR kern/58919:"
		    " timer_settime fails to trigger for past times");
	}
	RL(timer_settime(t, flags, &tim, NULL));
	RL(timer_settime(t, flags, &tim, &otim));
	if (mode == PAST && (flags & TIMER_ABSTIME) == 0) {
		atf_tc_expect_pass();
	}

	RL(clock_gettime(cid, &t1));
	timespecsub(&t1, &t0, &dt);
	fprintf(stderr, "%lld sec %d nsec elapsed\n",
	    (long long)dt.tv_sec, (int)dt.tv_nsec);

	/*
	 * Check to make sure the time remaining is at most the
	 * relative time we expected.
	 */
	atf_tc_expect_fail("PR kern/58917:"
	    " timer_settime and timerfd_settime return"
	    " absolute time of next event");
	ATF_CHECK_MSG(timespeccmp(&otim.it_value, &rtim.it_value, <=),
	    "time remaining %lld sec %d nsec,"
	    " expected at most %lld sec %d nsec",
	    (long long)otim.it_value.tv_sec, (int)otim.it_value.tv_nsec,
	    (long long)rtim.it_value.tv_sec, (int)rtim.it_value.tv_nsec);
	atf_tc_expect_pass();

	/*
	 * Until we fix PR kern/58917, adjust it to be relative to the
	 * start time.
	 */
	timespecsub(&otim.it_value, &t0, &otim.it_value);
	fprintf(stderr, "adjust otim to %lld sec %d nsec\n",
	    (long long)otim.it_value.tv_sec, (int)otim.it_value.tv_nsec);

#if 0
	/*
	 * Check to make sure that the amount the time remaining has
	 * gone down is at most the time elapsed.
	 *
	 * XXX Currently the time returned by timer_settime is only
	 * good to the nearest kernel tick (typically 10ms or 1ms), not
	 * to the resolution of the underlying clock -- unlike
	 * clock_gettime.  So we can't set this bound.  Not sure
	 * whether this is a bug or not, hence #if 0 instead of
	 * atf_tc_expect_fail.
	 */
	timespecsub(&t1, &t0, &dt);
	timespecsub(&rtim.it_value, &otim.it_value, &rtim.it_value);
	ATF_CHECK_MSG(timespeccmp(&rtim.it_value, &dt, <=),
	    "time remaining went down by %lld sec %d nsec,"
	    " expected at most %lld sec %d nsec",
	    (long long)rtim.it_value.tv_sec, (int)rtim.it_value.tv_nsec,
	    (long long)dt.tv_sec, (int)dt.tv_nsec);
#endif

	/*
	 * Check to make sure the reload interval is what we set.
	 */
	ATF_CHECK_MSG(timespeccmp(&otim.it_interval, &rtim.it_interval, ==),
	    "interval %lld sec %d nsec,"
	    " expected %lld sec %d nsec",
	    (long long)otim.it_interval.tv_sec, (int)otim.it_interval.tv_nsec,
	    (long long)rtim.it_interval.tv_sec, (int)rtim.it_interval.tv_nsec);

	(void)sigprocmask(SIG_UNBLOCK, &set, NULL);
	switch (mode) {
	case PAST:
		if (flags & TIMER_ABSTIME) {
			atf_tc_expect_fail("PR kern/58919:"
			    " timer_settime fails to trigger for past times");
		}
		ATF_CHECK_MSG(expired, "timer failed to fire immediately");
		if (flags & TIMER_ABSTIME) {
			atf_tc_expect_pass();
		}
		break;
	case EXPIRE:
	case NOEXPIRE:
		ATF_CHECK_MSG(!expired, "timer fired too soon");
		(void)sleep(2);
		switch (mode) {
		case PAST:
			__unreachable();
		case EXPIRE:
			ATF_CHECK_MSG(expired,
			    "timer failed to fire immediately");
			break;
		case NOEXPIRE:
			ATF_CHECK_MSG(!expired, "timer fired too soon");
			break;
		}
		break;
	}

	ATF_REQUIRE(timer_delete(t) == 0);
}

ATF_TC(timer_create_err);
ATF_TC_HEAD(timer_create_err, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Check errors from timer_create(2) (PR lib/42434)");
}

ATF_TC_BODY(timer_create_err, tc)
{
	struct sigevent ev;

	(void)memset(&ev, 0, sizeof(struct sigevent));

	errno = 0;
	ev.sigev_signo = -1;
	ev.sigev_notify = SIGEV_SIGNAL;

	ATF_REQUIRE_ERRNO(EINVAL, timer_create(CLOCK_REALTIME, &ev, &t) == -1);

	errno = 0;
	ev.sigev_signo = SIGUSR1;
	ev.sigev_notify = SIGEV_THREAD + 100;

	ATF_REQUIRE_ERRNO(EINVAL, timer_create(CLOCK_REALTIME, &ev, &t) == -1);
}

ATF_TC(timer_create_real);
ATF_TC_HEAD(timer_create_real, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_REALTIME and sigevent(3), "
	    "SIGEV_SIGNAL");
}

ATF_TC_BODY(timer_create_real, tc)
{
	timer_signal_create(CLOCK_REALTIME, NOEXPIRE, 0);
}

ATF_TC(timer_create_real_abs);
ATF_TC_HEAD(timer_create_real_abs, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_REALTIME and sigevent(3), "
	    "SIGEV_SIGNAL, using absolute time");
}

ATF_TC_BODY(timer_create_real_abs, tc)
{
	timer_signal_create(CLOCK_REALTIME, NOEXPIRE, TIMER_ABSTIME);
}

ATF_TC(timer_create_mono);
ATF_TC_HEAD(timer_create_mono, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_MONOTONIC and sigevent(3), "
	    "SIGEV_SIGNAL");
}

ATF_TC_BODY(timer_create_mono, tc)
{
	timer_signal_create(CLOCK_MONOTONIC, NOEXPIRE, 0);
}

ATF_TC(timer_create_mono_abs);
ATF_TC_HEAD(timer_create_mono_abs, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_MONOTONIC and sigevent(3), "
	    "SIGEV_SIGNAL, using absolute time");
}

ATF_TC_BODY(timer_create_mono_abs, tc)
{
	timer_signal_create(CLOCK_MONOTONIC, NOEXPIRE, TIMER_ABSTIME);
}

ATF_TC(timer_create_real_expire);
ATF_TC_HEAD(timer_create_real_expire, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_REALTIME and sigevent(3), "
	    "SIGEV_SIGNAL, with expiration");
}

ATF_TC_BODY(timer_create_real_expire, tc)
{
	timer_signal_create(CLOCK_REALTIME, EXPIRE, 0);
}

ATF_TC(timer_create_real_expire_abs);
ATF_TC_HEAD(timer_create_real_expire_abs, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_REALTIME and sigevent(3), "
	    "SIGEV_SIGNAL, with expiration, using absolute time");
}

ATF_TC_BODY(timer_create_real_expire_abs, tc)
{
	timer_signal_create(CLOCK_REALTIME, EXPIRE, TIMER_ABSTIME);
}

ATF_TC(timer_create_mono_expire);
ATF_TC_HEAD(timer_create_mono_expire, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_MONOTONIC and sigevent(3), "
	    "SIGEV_SIGNAL, with expiration");
}

ATF_TC_BODY(timer_create_mono_expire, tc)
{
	timer_signal_create(CLOCK_MONOTONIC, EXPIRE, 0);
}

ATF_TC(timer_create_mono_expire_abs);
ATF_TC_HEAD(timer_create_mono_expire_abs, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_MONOTONIC and sigevent(3), "
	    "SIGEV_SIGNAL, with expiration, using absolute time");
}

ATF_TC_BODY(timer_create_mono_expire_abs, tc)
{
	timer_signal_create(CLOCK_MONOTONIC, EXPIRE, TIMER_ABSTIME);
}

ATF_TC(timer_create_real_past);
ATF_TC_HEAD(timer_create_real_past, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_REALTIME and sigevent(3), "
	    "SIGEV_SIGNAL, with expiration passed before timer_settime(2)");
}

ATF_TC_BODY(timer_create_real_past, tc)
{
	timer_signal_create(CLOCK_REALTIME, PAST, 0);
}

ATF_TC(timer_create_real_past_abs);
ATF_TC_HEAD(timer_create_real_past_abs, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_REALTIME and sigevent(3), "
	    "SIGEV_SIGNAL, with expiration passed before timer_settime(2),"
	    " using absolute time");
}

ATF_TC_BODY(timer_create_real_past_abs, tc)
{
	timer_signal_create(CLOCK_REALTIME, PAST, TIMER_ABSTIME);
}

ATF_TC(timer_create_mono_past);
ATF_TC_HEAD(timer_create_mono_past, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_MONOTONIC and sigevent(3), "
	    "SIGEV_SIGNAL, with expiration passed before timer_settime(2)");
}

ATF_TC_BODY(timer_create_mono_past, tc)
{
	timer_signal_create(CLOCK_MONOTONIC, PAST, 0);
}

ATF_TC(timer_create_mono_past_abs);
ATF_TC_HEAD(timer_create_mono_past_abs, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_MONOTONIC and sigevent(3), "
	    "SIGEV_SIGNAL, with expiration passed before timer_settime(2),"
	    " using absolute time");
}

ATF_TC_BODY(timer_create_mono_past_abs, tc)
{
	timer_signal_create(CLOCK_MONOTONIC, PAST, TIMER_ABSTIME);
}

ATF_TC(timer_invalidtime);
ATF_TC_HEAD(timer_invalidtime, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify timer_settime(2) rejects invalid times");
}

ATF_TC_BODY(timer_invalidtime, tc)
{
	const struct itimerspec einval_its[] = {
		[0] = { .it_value = { -1, -1 } },
		[1] = { .it_value = { 1, -1 } },
		[2] = { .it_value = { 0, 1000000001 } },
		[3] = { .it_interval = { -1, -1 } },
		[4] = { .it_interval = { 1, -1 } },
		[5] = { .it_interval = { 0, 1000000001 } },
	};
	struct timespec now;
	unsigned i;

	RL(clock_gettime(CLOCK_MONOTONIC, &now));

	RL(timer_create(CLOCK_MONOTONIC, NULL, &t));

	for (i = 0; i < __arraycount(einval_its); i++) {
		struct itimerspec its;

		ATF_CHECK_ERRNO(EINVAL,
		    timer_settime(t, 0, &einval_its[i], NULL) == -1);

		/* Try the same with an absolute time near now. */
		its.it_value = einval_its[i].it_value;
		its.it_value.tv_sec += now.tv_sec + 60;
		ATF_CHECK_ERRNO(EINVAL,
		    timer_settime(t, TIMER_ABSTIME, &its, NULL) == -1);
	}

	RL(timer_delete(t));
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, timer_create_err);
	ATF_TP_ADD_TC(tp, timer_create_real);
	ATF_TP_ADD_TC(tp, timer_create_real_abs);
	ATF_TP_ADD_TC(tp, timer_create_mono);
	ATF_TP_ADD_TC(tp, timer_create_mono_abs);
	ATF_TP_ADD_TC(tp, timer_create_real_expire);
	ATF_TP_ADD_TC(tp, timer_create_real_expire_abs);
	ATF_TP_ADD_TC(tp, timer_create_mono_expire);
	ATF_TP_ADD_TC(tp, timer_create_mono_expire_abs);
	ATF_TP_ADD_TC(tp, timer_create_real_past);
	ATF_TP_ADD_TC(tp, timer_create_real_past_abs);
	ATF_TP_ADD_TC(tp, timer_create_mono_past);
	ATF_TP_ADD_TC(tp, timer_create_mono_past_abs);
	ATF_TP_ADD_TC(tp, timer_invalidtime);

	return atf_no_error();
}
