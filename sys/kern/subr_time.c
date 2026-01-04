/*	$NetBSD: subr_time.c,v 1.43 2026/01/04 03:21:11 riastradh Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_clock.c	8.5 (Berkeley) 1/21/94
 *	@(#)kern_time.c 8.4 (Berkeley) 5/26/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_time.c,v 1.43 2026/01/04 03:21:11 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/intr.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/sdt.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/timex.h>

/*
 * Compute number of hz until specified time.  Used to compute second
 * argument to callout_reset() from an absolute time.
 */
int
tvhzto(const struct timeval *tvp)
{
	struct timeval now, tv;

	tv = *tvp;	/* Don't modify original tvp. */
	getmicrotime(&now);
	timersub(&tv, &now, &tv);
	return tvtohz(&tv);
}

int
tshzto(const struct timespec *tsp)
{
	struct timespec now, ts;

	ts = *tsp;	/* Don't modify original tsp. */
	getnanotime(&now);
	timespecsub(&ts, &now, &ts);
	return tstohz(&ts);
}

int
tshztoup(const struct timespec *tsp)
{
	struct timespec now, ts;

	ts = *tsp;	/* Don't modify original tsp. */
	getnanouptime(&now);
	timespecsub(&ts, &now, &ts);
	return tstohz(&ts);
}

int
inittimeleft(struct timespec *ts, struct timespec *sleepts)
{

	if (itimespecfix(ts)) {
		return -1;
	}
	KASSERT(ts->tv_sec >= 0);
	getnanouptime(sleepts);
	return 0;
}

int
gettimeleft(struct timespec *ts, struct timespec *sleepts)
{
	struct timespec now, sleptts;

	KASSERT(ts->tv_sec >= 0);

	/*
	 * Reduce ts by elapsed time based on monotonic time scale.
	 */
	getnanouptime(&now);
	KASSERT(timespeccmp(sleepts, &now, <=));
	timespecsub(&now, sleepts, &sleptts);
	*sleepts = now;

	if (timespeccmp(ts, &sleptts, <=)) { /* timed out */
		timespecclear(ts);
		return 0;
	}
	timespecsub(ts, &sleptts, ts);

	return tstohz(ts);
}

void
clock_timeleft(clockid_t clockid, struct timespec *ts, struct timespec *sleepts)
{
	struct timespec sleptts;

	clock_gettime1(clockid, &sleptts);
	timespecadd(ts, sleepts, ts);
	timespecsub(ts, &sleptts, ts);
	*sleepts = sleptts;
}

int
clock_gettime1(clockid_t clock_id, struct timespec *ts)
{
	int error;
	struct proc *p;

#define CPUCLOCK_ID_MASK (~(CLOCK_THREAD_CPUTIME_ID|CLOCK_PROCESS_CPUTIME_ID))
	if (clock_id & CLOCK_PROCESS_CPUTIME_ID) {
		pid_t pid = clock_id & CPUCLOCK_ID_MASK;
		struct timeval cputime;

		mutex_enter(&proc_lock);
		p = pid == 0 ? curproc : proc_find(pid);
		if (p == NULL) {
			mutex_exit(&proc_lock);
			return SET_ERROR(ESRCH);
		}
		mutex_enter(p->p_lock);
		calcru(p, /*usertime*/NULL, /*systime*/NULL, /*intrtime*/NULL,
		    &cputime);
		mutex_exit(p->p_lock);
		mutex_exit(&proc_lock);

		// XXX: Perhaps create a special kauth type
		error = kauth_authorize_process(kauth_cred_get(),
		    KAUTH_PROCESS_PTRACE, p,
		    KAUTH_ARG(KAUTH_REQ_PROCESS_CANSEE_ENTRY), NULL, NULL);
		if (error)
			return error;

		TIMEVAL_TO_TIMESPEC(&cputime, ts);
		return 0;
	} else if (clock_id & CLOCK_THREAD_CPUTIME_ID) {
		struct lwp *l;
		lwpid_t lid = clock_id & CPUCLOCK_ID_MASK;
		struct bintime tm = {0, 0};

		p = curproc;
		mutex_enter(p->p_lock);
		l = lid == 0 ? curlwp : lwp_find(p, lid);
		if (l == NULL) {
			mutex_exit(p->p_lock);
			return SET_ERROR(ESRCH);
		}
		addrulwp(l, &tm);
		mutex_exit(p->p_lock);

		bintime2timespec(&tm, ts);
		return 0;
	}

	switch (clock_id) {
	case CLOCK_REALTIME:
		nanotime(ts);
		break;
	case CLOCK_MONOTONIC:
		nanouptime(ts);
		break;
	default:
		return SET_ERROR(EINVAL);
	}

	return 0;
}

/*
 * Calculate delta and convert from struct timespec to the ticks.
 */
int
ts2timo(clockid_t clock_id, int flags, struct timespec *ts,
    int *timo, struct timespec *start)
{
	int error;
	struct timespec tsd;

	if (ts->tv_nsec < 0 || ts->tv_nsec >= 1000000000L)
		return SET_ERROR(EINVAL);

	if ((flags & TIMER_ABSTIME) != 0 || start != NULL) {
		error = clock_gettime1(clock_id, &tsd);
		if (error != 0)
			return error;
		if (start != NULL)
			*start = tsd;
	}

	if ((flags & TIMER_ABSTIME) != 0) {
		if (!timespecsubok(ts, &tsd))
			return SET_ERROR(EINVAL);
		timespecsub(ts, &tsd, &tsd);
		ts = &tsd;
	}

	error = itimespecfix(ts);
	if (error != 0)
		return error;

	if (ts->tv_sec == 0 && ts->tv_nsec == 0)
		return SET_ERROR(ETIMEDOUT);

	*timo = tstohz(ts);
	KASSERT(*timo > 0);

	return 0;
}
