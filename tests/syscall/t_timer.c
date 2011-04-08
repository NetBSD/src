/*	$NetBSD: t_timer.c,v 1.2 2011/04/08 11:11:53 yamt Exp $ */

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

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <atf-c.h>

#include "../h_macros.h"

static void	timer_signal_create(clockid_t, int);
static void	timer_signal_handler(int, siginfo_t *, void *);
static int	timer_wait(time_t);

#if 0
/*
 * XXX: SIGEV_THREAD is not yet supported.
 */
static void	timer_thread_create(clockid_t);
static void	timer_thread_handler(sigval_t);
#endif

static timer_t t;
static bool error;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

ATF_TC(timer_create_bogus);
ATF_TC_HEAD(timer_create_bogus, tc)
{

	/* Cf. PR lib/42434. */
	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2)'s error checking");
}

ATF_TC_BODY(timer_create_bogus, tc)
{
	struct sigevent evt;

	(void)memset(&evt, 0, sizeof(struct sigevent));

	evt.sigev_signo = -1;
	evt.sigev_notify = SIGEV_SIGNAL;

	if (timer_create(CLOCK_REALTIME, &evt, &t) == 0)
		goto fail;

	evt.sigev_signo = SIGUSR1;
	evt.sigev_notify = SIGEV_THREAD + 100;

	if (timer_create(CLOCK_REALTIME, &evt, &t) == 0)
		goto fail;

	evt.sigev_signo = SIGUSR1;
	evt.sigev_value.sival_int = 0;
	evt.sigev_notify = SIGEV_SIGNAL;

	if (timer_create(CLOCK_REALTIME + 100, &evt, &t) == 0)
		goto fail;

	t = 0;

	return;

fail:
	atf_tc_fail("timer_create() successful with bogus values");
}

ATF_TC(timer_create_signal_realtime);
ATF_TC_HEAD(timer_create_signal_realtime, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_REALTIME and sigevent(3), "
	    "SIGEV_SIGNAL");
}

ATF_TC_BODY(timer_create_signal_realtime, tc)
{
	int i, signals[6] = {
		SIGALRM, SIGIO, SIGPROF, SIGUSR1, SIGUSR2, -1
	};

	for (i = 0; signals[i] > 0; i++)
		timer_signal_create(CLOCK_REALTIME, signals[i]);
}

ATF_TC(timer_create_signal_monotonic);
ATF_TC_HEAD(timer_create_signal_monotonic, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) with CLOCK_MONOTONIC and sigevent(3), "
	    "SIGEV_SIGNAL");
}

ATF_TC_BODY(timer_create_signal_monotonic, tc)
{
	int i, signals[6] = {
		SIGALRM, SIGIO, SIGPROF, SIGUSR1, SIGUSR2, -1
	};

	for (i = 0; signals[i] > 0; i++)
		timer_signal_create(CLOCK_MONOTONIC, signals[i]);
}

static void
timer_signal_create(clockid_t cid, int sig)
{
	struct itimerspec tim;
	struct sigaction act;
	struct sigevent evt;
	const char *errstr;
	sigset_t set;

	error = true;

	(void)memset(&evt, 0, sizeof(struct sigevent));
	(void)memset(&act, 0, sizeof(struct sigaction));
	(void)memset(&tim, 0, sizeof(struct itimerspec));

	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = timer_signal_handler;

	(void)sigemptyset(&act.sa_mask);

	if (sigaction(sig, &act, NULL) != 0) {
		errstr = "sigaction()";
		goto fail;
	}

	(void)sigemptyset(&set);
	(void)sigaddset(&set, sig);

	if (sigprocmask(SIG_SETMASK, &set, NULL) != 0) {
		errstr = "sigprocmask()";
		goto fail;
	}

	evt.sigev_signo = sig;
	evt.sigev_value.sival_ptr = &t;
	evt.sigev_notify = SIGEV_SIGNAL;

	if (timer_create(cid, &evt, &t) != 0) {
		errstr = "timer_create()";
		goto fail;
	}

	tim.it_value.tv_sec = 0;
	tim.it_value.tv_nsec = 1000 * 1000;

	if (timer_settime(t, 0, &tim, NULL) != 0) {
		errstr = "timer_settime()";
		goto fail;
	}

	if (sigprocmask(SIG_UNBLOCK, &set, NULL) != 0) {
		errstr = "sigprocmask()";
		goto fail;
	}

	errno = timer_wait(1);

	if (errno != 0) {
		errstr = "timer_wait()";
		goto fail;
	}

	return;

fail:
	atf_tc_fail_errno("%s failed (sig %d, clock %d)", errstr, sig, cid);
}

static void
timer_signal_handler(int signo, siginfo_t *si, void *osi)
{
	timer_t *tp;

	if (pthread_mutex_lock(&mtx) != 0)
		return;

	tp = si->si_value.sival_ptr;

	if (*tp == t)
		error = false;

	(void)pthread_cond_signal(&cond);
	(void)pthread_mutex_unlock(&mtx);
	(void)signal(signo, SIG_IGN);
}

static int
timer_wait(time_t wait)
{
	struct timespec ts;
	int rv;

	rv = pthread_mutex_lock(&mtx);

	if (rv != 0)
		return rv;

	errno = 0;

	if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {

		if (errno == 0)
			errno = EFAULT;

		return errno;
	}

	ts.tv_sec += wait;
	rv = pthread_cond_timedwait(&cond, &mtx, &ts);

	if (rv != 0)
		return rv;

	rv = pthread_mutex_unlock(&mtx);

	if (rv != 0)
		return rv;

	if (error != false)
		return EPROCUNAVAIL;

	return timer_delete(t);
}

#if 0
ATF_TC(timer_create_thread);
ATF_TC_HEAD(timer_create_thread, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks timer_create(2) and sigevent(3), SIGEV_THREAD");
}

ATF_TC_BODY(timer_create_thread, tc)
{
	timer_thread_create(CLOCK_REALTIME);
}

static void
timer_thread_create(clockid_t cid)
{
	struct itimerspec tim;
	struct sigevent evt;
	const char *errstr;

	error = true;

	(void)memset(&evt, 0, sizeof(struct sigevent));
	(void)memset(&tim, 0, sizeof(struct itimerspec));

	evt.sigev_notify = SIGEV_THREAD;
	evt.sigev_value.sival_ptr = &t;
	evt.sigev_notify_function = timer_thread_handler;
	evt.sigev_notify_attributes = NULL;

	if (timer_create(cid, &evt, &t) != 0) {
		errstr = "timer_create()";
		goto fail;
	}

	tim.it_value.tv_sec = 1;
	tim.it_value.tv_nsec = 0;

	if (timer_settime(t, 0, &tim, NULL) != 0) {
		errstr = "timer_settime()";
		goto fail;
	}

	errno = timer_wait(3);

	if (errno != 0) {
		errstr = "timer_wait()";
		goto fail;
	}

	return;

fail:
	atf_tc_fail_errno("%s failed (clock %d)", errstr, cid);
}

static void
timer_thread_handler(sigval_t sv)
{
	timer_t *tp;

	if (pthread_mutex_lock(&mtx) != 0)
		return;

	tp = sv.sival_ptr;

	if (*tp == t)
		error = false;

	(void)pthread_cond_signal(&cond);
	(void)pthread_mutex_unlock(&mtx);
}
#endif

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, timer_create_bogus);
	ATF_TP_ADD_TC(tp, timer_create_signal_realtime);
	ATF_TP_ADD_TC(tp, timer_create_signal_monotonic);
     /*	ATF_TP_ADD_TC(tp, timer_create_thread); */

	return atf_no_error();
}
