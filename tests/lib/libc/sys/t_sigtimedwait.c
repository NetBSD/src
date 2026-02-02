/* $NetBSD: t_sigtimedwait.c,v 1.2.36.1 2026/02/02 20:53:20 martin Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_sigtimedwait.c,v 1.2.36.1 2026/02/02 20:53:20 martin Exp $");

#include <sys/time.h>

#include <atf-c.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "h_macros.h"

static void
on_alarm(int signo)
{
	const char msg[] = "SIGALRM delivered\n";

	(void)write(STDERR_FILENO, msg, strlen(msg));
}

ATF_TC(sigtimedwait_all0timeout);

ATF_TC_HEAD(sigtimedwait_all0timeout, tc)
{
	atf_tc_set_md_var(tc, "timeout", "10");
	atf_tc_set_md_var(tc, "descr", "Test for PR kern/47625: sigtimedwait"
	    " with a timeout value of all zero should return imediately");
}

ATF_TC_BODY(sigtimedwait_all0timeout, tc)
{
	sigset_t block;
	struct timespec ts, before, after, len;
	siginfo_t info;
	int signo, error;

	RL(sigemptyset(&block));
	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	RL(clock_gettime(CLOCK_MONOTONIC, &before));
	signo = sigtimedwait(&block, &info, &ts);
	error = errno;
	RL(clock_gettime(CLOCK_MONOTONIC, &after));
	ATF_REQUIRE_MSG(signo == -1, "signo=%d, expected -1/EAGAIN=%d",
	    signo, EAGAIN);
	errno = error;
	ATF_REQUIRE_MSG(errno == EAGAIN, "errno=%d (%s), expected EAGAIN=%d",
	    error, strerror(error), EAGAIN);
	timespecsub(&after, &before, &len);
	ATF_REQUIRE(len.tv_sec < 1);
}

ATF_TC(sigtimedwait_NULL_timeout);

ATF_TC_HEAD(sigtimedwait_NULL_timeout, tc)
{
	atf_tc_set_md_var(tc, "timeout", "10");
	atf_tc_set_md_var(tc, "descr", "Test sigtimedwait() without timeout");
}

ATF_TC_BODY(sigtimedwait_NULL_timeout, tc)
{
	sigset_t sig;
	siginfo_t info;
	struct itimerval it;
	int signo;

	/* arrange for a SIGALRM signal in a few seconds */
	memset(&it, 0, sizeof it);
	it.it_value.tv_sec = 5;
	RL(setitimer(ITIMER_REAL, &it, NULL));

	/* wait without timeout */
	RL(sigemptyset(&sig));
	RL(sigaddset(&sig, SIGALRM));
	RL(signo = sigtimedwait(&sig, &info, NULL));
	ATF_REQUIRE_MSG(signo == SIGALRM, "signo=%d, expected SIGALRM=%d",
	    signo, SIGALRM);
}

ATF_TC(sigtimedwait_small_timeout);

ATF_TC_HEAD(sigtimedwait_small_timeout, tc)
{
	atf_tc_set_md_var(tc, "timeout", "15");
	atf_tc_set_md_var(tc, "descr", "Test sigtimedwait with a small "
	    "timeout");
}

ATF_TC_BODY(sigtimedwait_small_timeout, tc)
{
	sigset_t block;
	struct timespec ts;
	siginfo_t info;
	int signo, error;

	RL(sigemptyset(&block));
	ts.tv_sec = 5;
	ts.tv_nsec = 0;
	signo = sigtimedwait(&block, &info, &ts);
	ATF_REQUIRE_MSG(signo == -1, "signo=%d, expected -1/EAGAIN=%d",
	    signo, EAGAIN);
	error = errno;
	ATF_REQUIRE_MSG(errno == EAGAIN, "errno=%d (%s), expected EAGAIN=%d",
	    error, strerror(error), EAGAIN);
}

ATF_TC(sigtimedwait_small_timeout_alarm);

ATF_TC_HEAD(sigtimedwait_small_timeout_alarm, tc)
{
	atf_tc_set_md_var(tc, "timeout", "15");
	atf_tc_set_md_var(tc, "descr", "Test sigtimedwait with a small "
	    "timeout");
}

ATF_TC_BODY(sigtimedwait_small_timeout_alarm, tc)
{
	sigset_t block;
	struct sigaction sa = {.sa_handler = &on_alarm}; /* no SA_RESTART */
	struct timespec ts;
	siginfo_t info;
	int signo;

	RL(sigaction(SIGALRM, &sa, NULL));

	RL(sigemptyset(&block));
	ts.tv_sec = 5;
	ts.tv_nsec = 0;
	RL(sigaddset(&block, SIGALRM));
	RL(sigprocmask(SIG_BLOCK, &block, NULL));
	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	RL(signo = sigtimedwait(&block, &info, &ts));
	ATF_REQUIRE_MSG(signo == SIGALRM, "signo=%d, expected SIGALRM=%d",
	    signo, SIGALRM);
}

ATF_TC(sigtimedwait_small_timeout_other_sig);

ATF_TC_HEAD(sigtimedwait_small_timeout_other_sig, tc)
{
	atf_tc_set_md_var(tc, "timeout", "15");
	atf_tc_set_md_var(tc, "descr", "Test sigtimedwait interruption "
	    "by a signal it's not waiting for");
}

ATF_TC_BODY(sigtimedwait_small_timeout_other_sig, tc)
{
	sigset_t sig;
	struct sigaction sa = {.sa_handler = &on_alarm}; /* no SA_RESTART */
	struct timespec ts;
	siginfo_t info;
	int signo, error;

	RL(sigaction(SIGALRM, &sa, NULL));

	RL(sigemptyset(&sig));
	ts.tv_sec = 5;
	ts.tv_nsec = 0;
	RL(sigaddset(&sig, SIGUSR1));
	RL(sigprocmask(SIG_BLOCK, &sig, NULL));
	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	/*
	 * This returns 0 sometimes, when it should return -1/EINTR
	 * because some signal unblocked was delivered.
	 */
	signo = sigtimedwait(&sig, &info, &ts);
	ATF_REQUIRE_MSG(signo == -1, "signo=%d, expected -1/EINTR=%d",
	    signo, EINTR);
	error = errno;
	ATF_REQUIRE_MSG(errno == EINTR, "errno=%d (%s), expected EINTR=%d",
	    error, strerror(error), EINTR);
}

ATF_TC(sigwaitinfo_other_sig);

ATF_TC_HEAD(sigwaitinfo_other_sig, tc)
{
	atf_tc_set_md_var(tc, "timeout", "15");
	atf_tc_set_md_var(tc, "descr", "Test sigwaitinfo interruption "
	    "by a signal it's not waiting for");
}

ATF_TC_BODY(sigwaitinfo_other_sig, tc)
{
	sigset_t sig;
	struct sigaction sa = {.sa_handler = &on_alarm}; /* no SA_RESTART */
	siginfo_t info;
	int signo, error;

	RL(sigaction(SIGALRM, &sa, NULL));

	RL(sigemptyset(&sig));
	RL(sigaddset(&sig, SIGUSR1));
	RL(sigprocmask(SIG_BLOCK, &sig, NULL));
	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	signo = sigwaitinfo(&sig, &info);
	ATF_REQUIRE_MSG(signo == -1, "signo=%d, expected -1/EINTR=%d",
	    signo, EINTR);
	error = errno;
	ATF_REQUIRE_MSG(errno == EINTR, "errno=%d (%s), expected EINTR=%d",
	    error, strerror(error), EINTR);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, sigtimedwait_all0timeout);
	ATF_TP_ADD_TC(tp, sigtimedwait_NULL_timeout);
	ATF_TP_ADD_TC(tp, sigtimedwait_small_timeout);
	ATF_TP_ADD_TC(tp, sigtimedwait_small_timeout_alarm);
	ATF_TP_ADD_TC(tp, sigtimedwait_small_timeout_other_sig);
	ATF_TP_ADD_TC(tp, sigwaitinfo_other_sig);

	return atf_no_error();
}
