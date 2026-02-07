/* $NetBSD: t_getitimer.c,v 1.6 2026/02/07 01:47:23 riastradh Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_getitimer.c,v 1.6 2026/02/07 01:47:23 riastradh Exp $");

#include <sys/time.h>

#include <atf-c.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "h_macros.h"

#define	TIME_MAX	__type_max(time_t)

static sig_atomic_t	fired;
static void		sighandler(int);

static void
sighandler(int signo)
{

	if (signo == SIGALRM || signo == SIGVTALRM)
		fired = 1;
}

ATF_TC(getitimer_empty);
ATF_TC_HEAD(getitimer_empty, tc)
{
	atf_tc_set_md_var(tc, "descr", "getitimer(2) before setitimer(2)");
}

ATF_TC_BODY(getitimer_empty, tc)
{
	struct itimerval it;

	/*
	 * Verify that the passed structure remains
	 * empty after calling getitimer(2) but before
	 * actually arming the timer with setitimer(2).
	 */
	(void)memset(&it, 0, sizeof(struct itimerval));

	ATF_REQUIRE(getitimer(ITIMER_REAL, &it) == 0);

	if (it.it_value.tv_sec != 0 || it.it_value.tv_usec != 0)
		goto fail;

	ATF_REQUIRE(getitimer(ITIMER_VIRTUAL, &it) == 0);

	if (it.it_value.tv_sec != 0 || it.it_value.tv_usec != 0)
		goto fail;

	ATF_REQUIRE(getitimer(ITIMER_PROF, &it) == 0);

	if (it.it_value.tv_sec != 0 || it.it_value.tv_usec != 0)
		goto fail;

	return;

fail:
	atf_tc_fail("getitimer(2) modified the timer before it was armed");
}

ATF_TC(getitimer_err);
ATF_TC_HEAD(getitimer_err, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test errors from getitimer(2)");
}

ATF_TC_BODY(getitimer_err, tc)
{
	struct itimerval it;

	errno = 0;
	ATF_REQUIRE_ERRNO(EINVAL, getitimer(-1, &it) == -1);

	errno = 0;
	ATF_REQUIRE_ERRNO(EINVAL, getitimer(INT_MAX, &it) == -1);

	errno = 0;
	ATF_REQUIRE_ERRNO(EFAULT, getitimer(ITIMER_REAL, (void *)-1) == -1);
}

ATF_TC(setitimer_basic);
ATF_TC_HEAD(setitimer_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of setitimer(2)");
}

ATF_TC_BODY(setitimer_basic, tc)
{
	struct itimerval it;

	it.it_value.tv_sec = 0;
	it.it_value.tv_usec = 100;

	it.it_interval.tv_sec = 0;
	it.it_interval.tv_usec = 0;

	fired = 0;
	ATF_REQUIRE(signal(SIGALRM, sighandler) != SIG_ERR);
	ATF_REQUIRE(setitimer(ITIMER_REAL, &it, NULL) == 0);

	/*
	 * Although the interaction between
	 * setitimer(2) and sleep(3) can be
	 * unspecified, it is assumed that one
	 * second suspension will be enough for
	 * the timer to fire.
	 */
	(void)sleep(1);

	if (!fired)
		atf_tc_fail("timer did not fire");
}

ATF_TC(setitimer_err);
ATF_TC_HEAD(setitimer_err, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test errors from setitimer(2)"
	    " (PR standards/44927)");
}

ATF_TC_BODY(setitimer_err, tc)
{
	struct itimerval it, ot;

	errno = 0;
	ATF_REQUIRE_ERRNO(EINVAL, setitimer(-1, &it, &ot) == -1);

	errno = 0;
	ATF_REQUIRE_ERRNO(EINVAL, setitimer(INT_MAX, &it, &ot) == -1);

	errno = 0;
	ATF_REQUIRE_ERRNO(EFAULT, setitimer(ITIMER_REAL,(void*)-1, &ot) == -1);
}

ATF_TC(setitimer_old);
ATF_TC_HEAD(setitimer_old, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test old values from setitimer(2)");
}

ATF_TC_BODY(setitimer_old, tc)
{
	struct itimerval it, ot;

	/*
	 * Make two calls; the second one should store the old
	 * timer value which should be the same as that set in
	 * the first call, or slightly less due to time passing
	 * between the two calls.
	 */
	it.it_value.tv_sec = 4;
	it.it_value.tv_usec = 999999;

	it.it_interval.tv_sec = 0;
	it.it_interval.tv_usec = 0;

	ATF_REQUIRE(setitimer(ITIMER_REAL, &it, &ot) == 0);

	it.it_value.tv_sec = 2;
	it.it_value.tv_usec = 1;

	it.it_interval.tv_sec = 0;
	it.it_interval.tv_usec = 0;

	ATF_REQUIRE(setitimer(ITIMER_REAL, &it, &ot) == 0);

	/* Check seconds only as microseconds may have decremented */
	if (ot.it_value.tv_sec != 4)
		atf_tc_fail("setitimer(2) did not store old values");
}

ATF_TC(setitimer_invalidtime);
ATF_TC_HEAD(setitimer_invalidtime, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test invalid values in setitimer(2)");
}

ATF_TC_BODY(setitimer_invalidtime, tc)
{
	const struct itimerval it[] = {
		[0] = { .it_value = {-1, 0} },
		[1] = { .it_value = {0, -1} },
		[2] = { .it_value = {0, 1000001} },
		[3] = { .it_value = {1, 0}, .it_interval = {-1, 0} },
		[4] = { .it_value = {1, 0}, .it_interval = {0, -1} },
		[5] = { .it_value = {1, 0}, .it_interval = {0, 1000001} },
		[6] = { .it_value = {TIME_MAX - 1, 0}, .it_interval = {0, 0} },
		[7] = { .it_value = {TIME_MAX, 0}, .it_interval = {0, 0} },
	};
	sigset_t sigs, mask;
	unsigned i;

	RL(sigemptyset(&sigs));
	RL(sigaddset(&sigs, SIGALRM));
	RL(sigprocmask(SIG_BLOCK, &sigs, &mask));

	for (i = 0; i < __arraycount(it); i++) {
		fprintf(stderr, "case %u\n", i);
		ATF_CHECK_ERRNO(EINVAL,
		    setitimer(ITIMER_REAL, &it[i], NULL) == -1);
	}

	/* Wait up to 2sec to make sure no timer got set anyway. */
	ATF_CHECK_ERRNO(EAGAIN,
	    sigtimedwait(&sigs, NULL, &(const struct timespec){2, 0}) == -1);
	RL(sigprocmask(SIG_SETMASK, &mask, NULL));
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, getitimer_empty);
	ATF_TP_ADD_TC(tp, getitimer_err);
	ATF_TP_ADD_TC(tp, setitimer_basic);
	ATF_TP_ADD_TC(tp, setitimer_err);
	ATF_TP_ADD_TC(tp, setitimer_old);
	ATF_TP_ADD_TC(tp, setitimer_invalidtime);

	return atf_no_error();
}
