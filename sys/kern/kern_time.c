/*	$NetBSD: kern_time.c,v 1.214 2022/05/15 16:20:10 riastradh Exp $	*/

/*-
 * Copyright (c) 2000, 2004, 2005, 2007, 2008, 2009, 2020
 *     The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christopher G. Demetriou, by Andrew Doran, and by Jason R. Thorpe.
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
 *	@(#)kern_time.c	8.4 (Berkeley) 5/26/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_time.c,v 1.214 2022/05/15 16:20:10 riastradh Exp $");

#include <sys/param.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/signalvar.h>
#include <sys/syslog.h>
#include <sys/timetc.h>
#include <sys/timex.h>
#include <sys/kauth.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/cpu.h>

kmutex_t	itimer_mutex __cacheline_aligned;	/* XXX static */
static struct itlist itimer_realtime_changed_notify;

static void	ptimer_intr(void *);
static void	*ptimer_sih __read_mostly;
static TAILQ_HEAD(, ptimer) ptimer_queue;

#define	CLOCK_VIRTUAL_P(clockid)	\
	((clockid) == CLOCK_VIRTUAL || (clockid) == CLOCK_PROF)

CTASSERT(ITIMER_REAL == CLOCK_REALTIME);
CTASSERT(ITIMER_VIRTUAL == CLOCK_VIRTUAL);
CTASSERT(ITIMER_PROF == CLOCK_PROF);
CTASSERT(ITIMER_MONOTONIC == CLOCK_MONOTONIC);

#define	DELAYTIMER_MAX	32

/*
 * Initialize timekeeping.
 */
void
time_init(void)
{

	mutex_init(&itimer_mutex, MUTEX_DEFAULT, IPL_SCHED);
	LIST_INIT(&itimer_realtime_changed_notify);

	TAILQ_INIT(&ptimer_queue);
	ptimer_sih = softint_establish(SOFTINT_CLOCK | SOFTINT_MPSAFE,
	    ptimer_intr, NULL);
}

/*
 * Check if the time will wrap if set to ts.
 *
 * ts - timespec describing the new time
 * delta - the delta between the current time and ts
 */
bool
time_wraps(struct timespec *ts, struct timespec *delta)
{

	/*
	 * Don't allow the time to be set forward so far it
	 * will wrap and become negative, thus allowing an
	 * attacker to bypass the next check below.  The
	 * cutoff is 1 year before rollover occurs, so even
	 * if the attacker uses adjtime(2) to move the time
	 * past the cutoff, it will take a very long time
	 * to get to the wrap point.
	 */
	if ((ts->tv_sec > LLONG_MAX - 365*24*60*60) ||
	    (delta->tv_sec < 0 || delta->tv_nsec < 0))
		return true;

	return false;
}

/*
 * itimer_lock:
 *
 *	Acquire the interval timer data lock.
 */
void
itimer_lock(void)
{
	mutex_spin_enter(&itimer_mutex);
}

/*
 * itimer_unlock:
 *
 *	Release the interval timer data lock.
 */
void
itimer_unlock(void)
{
	mutex_spin_exit(&itimer_mutex);
}

/*
 * itimer_lock_held:
 *
 *	Check that the interval timer lock is held for diagnostic
 *	assertions.
 */
inline bool __diagused
itimer_lock_held(void)
{
	return mutex_owned(&itimer_mutex);
}

/*
 * Time of day and interval timer support.
 *
 * These routines provide the kernel entry points to get and set
 * the time-of-day and per-process interval timers.  Subroutines
 * here provide support for adding and subtracting timeval structures
 * and decrementing interval timers, optionally reloading the interval
 * timers when they expire.
 */

/* This function is used by clock_settime and settimeofday */
static int
settime1(struct proc *p, const struct timespec *ts, bool check_kauth)
{
	struct timespec delta, now;

	/*
	 * The time being set to an unreasonable value will cause
	 * unreasonable system behaviour.
	 */
	if (ts->tv_sec < 0 || ts->tv_sec > (1LL << 36))
		return (EINVAL);

	nanotime(&now);
	timespecsub(ts, &now, &delta);

	if (check_kauth && kauth_authorize_system(kauth_cred_get(),
	    KAUTH_SYSTEM_TIME, KAUTH_REQ_SYSTEM_TIME_SYSTEM, __UNCONST(ts),
	    &delta, KAUTH_ARG(check_kauth ? false : true)) != 0) {
		return (EPERM);
	}

#ifdef notyet
	if ((delta.tv_sec < 86400) && securelevel > 0) { /* XXX elad - notyet */
		return (EPERM);
	}
#endif

	tc_setclock(ts);

	resettodr();

	/*
	 * Notify pending CLOCK_REALTIME timers about the real time change.
	 * There may be inactive timers on this list, but this happens
	 * comparatively less often than timers firing, and so it's better
	 * to put the extra checks here than to complicate the other code
	 * path.
	 */
	struct itimer *it;
	itimer_lock();
	LIST_FOREACH(it, &itimer_realtime_changed_notify, it_rtchgq) {
		KASSERT(it->it_ops->ito_realtime_changed != NULL);
		if (timespecisset(&it->it_time.it_value)) {
			(*it->it_ops->ito_realtime_changed)(it);
		}
	}
	itimer_unlock();

	return (0);
}

int
settime(struct proc *p, struct timespec *ts)
{
	return (settime1(p, ts, true));
}

/* ARGSUSED */
int
sys___clock_gettime50(struct lwp *l,
    const struct sys___clock_gettime50_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) clock_id;
		syscallarg(struct timespec *) tp;
	} */
	int error;
	struct timespec ats;

	error = clock_gettime1(SCARG(uap, clock_id), &ats);
	if (error != 0)
		return error;

	return copyout(&ats, SCARG(uap, tp), sizeof(ats));
}

/* ARGSUSED */
int
sys___clock_settime50(struct lwp *l,
    const struct sys___clock_settime50_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) clock_id;
		syscallarg(const struct timespec *) tp;
	} */
	int error;
	struct timespec ats;

	if ((error = copyin(SCARG(uap, tp), &ats, sizeof(ats))) != 0)
		return error;

	return clock_settime1(l->l_proc, SCARG(uap, clock_id), &ats, true);
}


int
clock_settime1(struct proc *p, clockid_t clock_id, const struct timespec *tp,
    bool check_kauth)
{
	int error;

	if (tp->tv_nsec < 0 || tp->tv_nsec >= 1000000000L)
		return EINVAL;

	switch (clock_id) {
	case CLOCK_REALTIME:
		if ((error = settime1(p, tp, check_kauth)) != 0)
			return (error);
		break;
	case CLOCK_MONOTONIC:
		return (EINVAL);	/* read-only clock */
	default:
		return (EINVAL);
	}

	return 0;
}

int
sys___clock_getres50(struct lwp *l, const struct sys___clock_getres50_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(clockid_t) clock_id;
		syscallarg(struct timespec *) tp;
	} */
	struct timespec ts;
	int error;

	if ((error = clock_getres1(SCARG(uap, clock_id), &ts)) != 0)
		return error;

	if (SCARG(uap, tp))
		error = copyout(&ts, SCARG(uap, tp), sizeof(ts));

	return error;
}

int
clock_getres1(clockid_t clock_id, struct timespec *ts)
{

	switch (clock_id) {
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
		ts->tv_sec = 0;
		if (tc_getfrequency() > 1000000000)
			ts->tv_nsec = 1;
		else
			ts->tv_nsec = 1000000000 / tc_getfrequency();
		break;
	default:
		return EINVAL;
	}

	return 0;
}

/* ARGSUSED */
int
sys___nanosleep50(struct lwp *l, const struct sys___nanosleep50_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(struct timespec *) rqtp;
		syscallarg(struct timespec *) rmtp;
	} */
	struct timespec rmt, rqt;
	int error, error1;

	error = copyin(SCARG(uap, rqtp), &rqt, sizeof(struct timespec));
	if (error)
		return (error);

	error = nanosleep1(l, CLOCK_MONOTONIC, 0, &rqt,
	    SCARG(uap, rmtp) ? &rmt : NULL);
	if (SCARG(uap, rmtp) == NULL || (error != 0 && error != EINTR))
		return error;

	error1 = copyout(&rmt, SCARG(uap, rmtp), sizeof(rmt));
	return error1 ? error1 : error;
}

/* ARGSUSED */
int
sys_clock_nanosleep(struct lwp *l, const struct sys_clock_nanosleep_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(clockid_t) clock_id;
		syscallarg(int) flags;
		syscallarg(struct timespec *) rqtp;
		syscallarg(struct timespec *) rmtp;
	} */
	struct timespec rmt, rqt;
	int error, error1;

	error = copyin(SCARG(uap, rqtp), &rqt, sizeof(struct timespec));
	if (error)
		goto out;

	error = nanosleep1(l, SCARG(uap, clock_id), SCARG(uap, flags), &rqt,
	    SCARG(uap, rmtp) ? &rmt : NULL);
	if (SCARG(uap, rmtp) == NULL || (error != 0 && error != EINTR))
		goto out;

	if ((SCARG(uap, flags) & TIMER_ABSTIME) == 0 &&
	    (error1 = copyout(&rmt, SCARG(uap, rmtp), sizeof(rmt))) != 0)
		error = error1;
out:
	*retval = error;
	return 0;
}

int
nanosleep1(struct lwp *l, clockid_t clock_id, int flags, struct timespec *rqt,
    struct timespec *rmt)
{
	struct timespec rmtstart;
	int error, timo;

	if ((error = ts2timo(clock_id, flags, rqt, &timo, &rmtstart)) != 0) {
		if (error == ETIMEDOUT) {
			error = 0;
			if (rmt != NULL)
				rmt->tv_sec = rmt->tv_nsec = 0;
		}
		return error;
	}

	/*
	 * Avoid inadvertently sleeping forever
	 */
	if (timo == 0)
		timo = 1;
again:
	error = kpause("nanoslp", true, timo, NULL);
	if (error == EWOULDBLOCK)
		error = 0;
	if (rmt != NULL || error == 0) {
		struct timespec rmtend;
		struct timespec t0;
		struct timespec *t;
		int err;

		err = clock_gettime1(clock_id, &rmtend);
		if (err != 0)
			return err;

		t = (rmt != NULL) ? rmt : &t0;
		if (flags & TIMER_ABSTIME) {
			timespecsub(rqt, &rmtend, t);
		} else {
			if (timespeccmp(&rmtend, &rmtstart, <))
				timespecclear(t); /* clock wound back */
			else
				timespecsub(&rmtend, &rmtstart, t);
			if (timespeccmp(rqt, t, <))
				timespecclear(t);
			else
				timespecsub(rqt, t, t);
		}
		if (t->tv_sec < 0)
			timespecclear(t);
		if (error == 0) {
			timo = tstohz(t);
			if (timo > 0)
				goto again;
		}
	}

	if (error == ERESTART)
		error = EINTR;

	return error;
}

int
sys_clock_getcpuclockid2(struct lwp *l,
    const struct sys_clock_getcpuclockid2_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(idtype_t idtype;
		syscallarg(id_t id);
		syscallarg(clockid_t *)clock_id;
	} */
	pid_t pid;
	lwpid_t lid;
	clockid_t clock_id;
	id_t id = SCARG(uap, id);

	switch (SCARG(uap, idtype)) {
	case P_PID:
		pid = id == 0 ? l->l_proc->p_pid : id;
		clock_id = CLOCK_PROCESS_CPUTIME_ID | pid;
		break;
	case P_LWPID:
		lid = id == 0 ? l->l_lid : id;
		clock_id = CLOCK_THREAD_CPUTIME_ID | lid;
		break;
	default:
		return EINVAL;
	}
	return copyout(&clock_id, SCARG(uap, clock_id), sizeof(clock_id));
}

/* ARGSUSED */
int
sys___gettimeofday50(struct lwp *l, const struct sys___gettimeofday50_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(struct timeval *) tp;
		syscallarg(void *) tzp;		really "struct timezone *";
	} */
	struct timeval atv;
	int error = 0;
	struct timezone tzfake;

	if (SCARG(uap, tp)) {
		memset(&atv, 0, sizeof(atv));
		microtime(&atv);
		error = copyout(&atv, SCARG(uap, tp), sizeof(atv));
		if (error)
			return (error);
	}
	if (SCARG(uap, tzp)) {
		/*
		 * NetBSD has no kernel notion of time zone, so we just
		 * fake up a timezone struct and return it if demanded.
		 */
		tzfake.tz_minuteswest = 0;
		tzfake.tz_dsttime = 0;
		error = copyout(&tzfake, SCARG(uap, tzp), sizeof(tzfake));
	}
	return (error);
}

/* ARGSUSED */
int
sys___settimeofday50(struct lwp *l, const struct sys___settimeofday50_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const struct timeval *) tv;
		syscallarg(const void *) tzp; really "const struct timezone *";
	} */

	return settimeofday1(SCARG(uap, tv), true, SCARG(uap, tzp), l, true);
}

int
settimeofday1(const struct timeval *utv, bool userspace,
    const void *utzp, struct lwp *l, bool check_kauth)
{
	struct timeval atv;
	struct timespec ts;
	int error;

	/* Verify all parameters before changing time. */

	/*
	 * NetBSD has no kernel notion of time zone, and only an
	 * obsolete program would try to set it, so we log a warning.
	 */
	if (utzp)
		log(LOG_WARNING, "pid %d attempted to set the "
		    "(obsolete) kernel time zone\n", l->l_proc->p_pid);

	if (utv == NULL) 
		return 0;

	if (userspace) {
		if ((error = copyin(utv, &atv, sizeof(atv))) != 0)
			return error;
		utv = &atv;
	}

	if (utv->tv_usec < 0 || utv->tv_usec >= 1000000)
		return EINVAL;

	TIMEVAL_TO_TIMESPEC(utv, &ts);
	return settime1(l->l_proc, &ts, check_kauth);
}

int	time_adjusted;			/* set if an adjustment is made */

/* ARGSUSED */
int
sys___adjtime50(struct lwp *l, const struct sys___adjtime50_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const struct timeval *) delta;
		syscallarg(struct timeval *) olddelta;
	} */
	int error;
	struct timeval atv, oldatv;

	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_TIME,
	    KAUTH_REQ_SYSTEM_TIME_ADJTIME, NULL, NULL, NULL)) != 0)
		return error;

	if (SCARG(uap, delta)) {
		error = copyin(SCARG(uap, delta), &atv,
		    sizeof(*SCARG(uap, delta)));
		if (error)
			return (error);
	}
	adjtime1(SCARG(uap, delta) ? &atv : NULL,
	    SCARG(uap, olddelta) ? &oldatv : NULL, l->l_proc);
	if (SCARG(uap, olddelta))
		error = copyout(&oldatv, SCARG(uap, olddelta),
		    sizeof(*SCARG(uap, olddelta)));
	return error;
}

void
adjtime1(const struct timeval *delta, struct timeval *olddelta, struct proc *p)
{
	extern int64_t time_adjtime;  /* in kern_ntptime.c */

	if (olddelta) {
		memset(olddelta, 0, sizeof(*olddelta));
		mutex_spin_enter(&timecounter_lock);
		olddelta->tv_sec = time_adjtime / 1000000;
		olddelta->tv_usec = time_adjtime % 1000000;
		if (olddelta->tv_usec < 0) {
			olddelta->tv_usec += 1000000;
			olddelta->tv_sec--;
		}
		mutex_spin_exit(&timecounter_lock);
	}

	if (delta) {
		mutex_spin_enter(&timecounter_lock);
		/*
		 * XXX This should maybe just report failure to
		 * userland for nonsense deltas.
		 */
		if (delta->tv_sec > INT64_MAX/1000000 - 1) {
			time_adjtime = INT64_MAX;
		} else if (delta->tv_sec < INT64_MIN/1000000 + 1) {
			time_adjtime = INT64_MIN;
		} else {
			time_adjtime = delta->tv_sec * 1000000
			    + MAX(-999999, MIN(999999, delta->tv_usec));
		}

		if (time_adjtime) {
			/* We need to save the system time during shutdown */
			time_adjusted |= 1;
		}
		mutex_spin_exit(&timecounter_lock);
	}
}

/*
 * Interval timer support.
 *
 * The itimer_*() routines provide generic support for interval timers,
 * both real (CLOCK_REALTIME, CLOCK_MONOTIME), and virtual (CLOCK_VIRTUAL,
 * CLOCK_PROF).
 *
 * Real timers keep their deadline as an absolute time, and are fired
 * by a callout.  Virtual timers are kept as a linked-list of deltas,
 * and are processed by hardclock().
 *
 * Because the real time timer callout may be delayed in real time due
 * to interrupt processing on the system, it is possible for the real
 * time timeout routine (itimer_callout()) run past after its deadline.
 * It does not suffice, therefore, to reload the real timer .it_value
 * from the timer's .it_interval.  Rather, we compute the next deadline
 * in absolute time based on the current time and the .it_interval value,
 * and report any overruns.
 *
 * Note that while the virtual timers are supported in a generic fashion
 * here, they only (currently) make sense as per-process timers, and thus
 * only really work for that case.
 */

/*
 * itimer_init:
 *
 *	Initialize the common data for an interval timer.
 */
void
itimer_init(struct itimer * const it, const struct itimer_ops * const ops,
    clockid_t const id, struct itlist * const itl)
{

	KASSERT(itimer_lock_held());
	KASSERT(ops != NULL);

	timespecclear(&it->it_time.it_value);
	it->it_ops = ops;
	it->it_clockid = id;
	it->it_overruns = 0;
	it->it_dying = false;
	if (!CLOCK_VIRTUAL_P(id)) {
		KASSERT(itl == NULL);
		callout_init(&it->it_ch, CALLOUT_MPSAFE);
		if (id == CLOCK_REALTIME && ops->ito_realtime_changed != NULL) {
			LIST_INSERT_HEAD(&itimer_realtime_changed_notify,
			    it, it_rtchgq);
		}
	} else {
		KASSERT(itl != NULL);
		it->it_vlist = itl;
		it->it_active = false;
	}
}

/*
 * itimer_poison:
 *
 *	Poison an interval timer, preventing it from being scheduled
 *	or processed, in preparation for freeing the timer.
 */
void
itimer_poison(struct itimer * const it)
{

	KASSERT(itimer_lock_held());

	it->it_dying = true;

	/*
	 * For non-virtual timers, stop the callout, or wait for it to
	 * run if it has already fired.  It cannot restart again after
	 * this point: the callout won't restart itself when dying, no
	 * other users holding the lock can restart it, and any other
	 * users waiting for callout_halt concurrently (itimer_settime)
	 * will restart from the top.
	 */
	if (!CLOCK_VIRTUAL_P(it->it_clockid)) {
		callout_halt(&it->it_ch, &itimer_mutex);
		if (it->it_clockid == CLOCK_REALTIME &&
		    it->it_ops->ito_realtime_changed != NULL) {
			LIST_REMOVE(it, it_rtchgq);
		}
	}
}

/*
 * itimer_fini:
 *
 *	Release resources used by an interval timer.
 *
 *	N.B. itimer_lock must be held on entry, and is released on exit.
 */
void
itimer_fini(struct itimer * const it)
{

	KASSERT(itimer_lock_held());

	/* All done with the global state. */
	itimer_unlock();

	/* Destroy the callout, if needed. */
	if (!CLOCK_VIRTUAL_P(it->it_clockid))
		callout_destroy(&it->it_ch);
}

/*
 * itimer_decr:
 *
 *	Decrement an interval timer by a specified number of nanoseconds,
 *	which must be less than a second, i.e. < 1000000000.  If the timer
 *	expires, then reload it.  In this case, carry over (nsec - old value)
 *	to reduce the value reloaded into the timer so that the timer does
 *	not drift.  This routine assumes that it is called in a context where
 *	the timers on which it is operating cannot change in value.
 *
 *	Returns true if the timer has expired.
 */
static bool
itimer_decr(struct itimer *it, int nsec)
{
	struct itimerspec *itp;
	int error __diagused;

	KASSERT(itimer_lock_held());
	KASSERT(CLOCK_VIRTUAL_P(it->it_clockid));

	itp = &it->it_time;
	if (itp->it_value.tv_nsec < nsec) {
		if (itp->it_value.tv_sec == 0) {
			/* expired, and already in next interval */
			nsec -= itp->it_value.tv_nsec;
			goto expire;
		}
		itp->it_value.tv_nsec += 1000000000;
		itp->it_value.tv_sec--;
	}
	itp->it_value.tv_nsec -= nsec;
	nsec = 0;
	if (timespecisset(&itp->it_value))
		return false;
	/* expired, exactly at end of interval */
 expire:
	if (timespecisset(&itp->it_interval)) {
		itp->it_value = itp->it_interval;
		itp->it_value.tv_nsec -= nsec;
		if (itp->it_value.tv_nsec < 0) {
			itp->it_value.tv_nsec += 1000000000;
			itp->it_value.tv_sec--;
		}
		error = itimer_settime(it);
		KASSERT(error == 0); /* virtual, never fails */
	} else
		itp->it_value.tv_nsec = 0;		/* sec is already 0 */
	return true;
}

static void itimer_callout(void *);

/*
 * itimer_arm_real:
 *
 *	Arm a non-virtual timer.
 */
static void
itimer_arm_real(struct itimer * const it)
{
	/*
	 * Don't need to check tshzto() return value, here.
	 * callout_reset() does it for us.
	 */
	callout_reset(&it->it_ch,
	    (it->it_clockid == CLOCK_MONOTONIC
		? tshztoup(&it->it_time.it_value)
		: tshzto(&it->it_time.it_value)),
	    itimer_callout, it);
}

/*
 * itimer_callout:
 *
 *	Callout to expire a non-virtual timer.  Queue it up for processing,
 *	and then reload, if it is configured to do so.
 *
 *	N.B. A delay in processing this callout causes multiple
 *	SIGALRM calls to be compressed into one.
 */
static void
itimer_callout(void *arg)
{
	uint64_t last_val, next_val, interval, now_ns;
	struct timespec now, next;
	struct itimer * const it = arg;
	int backwards;

	itimer_lock();
	(*it->it_ops->ito_fire)(it);

	if (!timespecisset(&it->it_time.it_interval)) {
		timespecclear(&it->it_time.it_value);
		itimer_unlock();
		return;
	}

	if (it->it_clockid == CLOCK_MONOTONIC) {
		getnanouptime(&now);
	} else {
		getnanotime(&now);
	}
	backwards = (timespeccmp(&it->it_time.it_value, &now, >));
	timespecadd(&it->it_time.it_value, &it->it_time.it_interval, &next);
	/* Handle the easy case of non-overflown timers first. */
	if (!backwards && timespeccmp(&next, &now, >)) {
		it->it_time.it_value = next;
	} else {
		now_ns = timespec2ns(&now);
		last_val = timespec2ns(&it->it_time.it_value);
		interval = timespec2ns(&it->it_time.it_interval);

		next_val = now_ns +
		    (now_ns - last_val + interval - 1) % interval;

		if (backwards)
			next_val += interval;
		else
			it->it_overruns += (now_ns - last_val) / interval;

		it->it_time.it_value.tv_sec = next_val / 1000000000;
		it->it_time.it_value.tv_nsec = next_val % 1000000000;
	}

	/*
	 * Reset the callout, if it's not going away.
	 */
	if (!it->it_dying)
		itimer_arm_real(it);
	itimer_unlock();
}

/*
 * itimer_settime:
 *
 *	Set up the given interval timer. The value in it->it_time.it_value
 *	is taken to be an absolute time for CLOCK_REALTIME/CLOCK_MONOTONIC
 *	timers and a relative time for CLOCK_VIRTUAL/CLOCK_PROF timers.
 *
 *	If the callout had already fired but not yet run, fails with
 *	ERESTART -- caller must restart from the top to look up a timer.
 */
int
itimer_settime(struct itimer *it)
{
	struct itimer *itn, *pitn;
	struct itlist *itl;

	KASSERT(itimer_lock_held());

	if (!CLOCK_VIRTUAL_P(it->it_clockid)) {
		/*
		 * Try to stop the callout.  However, if it had already
		 * fired, we have to drop the lock to wait for it, so
		 * the world may have changed and pt may not be there
		 * any more.  In that case, tell the caller to start
		 * over from the top.
		 */
		if (callout_halt(&it->it_ch, &itimer_mutex))
			return ERESTART;

		/* Now we can touch it and start it up again. */
		if (timespecisset(&it->it_time.it_value))
			itimer_arm_real(it);
	} else {
		if (it->it_active) {
			itn = LIST_NEXT(it, it_list);
			LIST_REMOVE(it, it_list);
			for ( ; itn; itn = LIST_NEXT(itn, it_list))
				timespecadd(&it->it_time.it_value,
				    &itn->it_time.it_value,
				    &itn->it_time.it_value);
		}
		if (timespecisset(&it->it_time.it_value)) {
			itl = it->it_vlist;
			for (itn = LIST_FIRST(itl), pitn = NULL;
			     itn && timespeccmp(&it->it_time.it_value,
				 &itn->it_time.it_value, >);
			     pitn = itn, itn = LIST_NEXT(itn, it_list))
				timespecsub(&it->it_time.it_value,
				    &itn->it_time.it_value,
				    &it->it_time.it_value);

			if (pitn)
				LIST_INSERT_AFTER(pitn, it, it_list);
			else
				LIST_INSERT_HEAD(itl, it, it_list);

			for ( ; itn ; itn = LIST_NEXT(itn, it_list))
				timespecsub(&itn->it_time.it_value,
				    &it->it_time.it_value,
				    &itn->it_time.it_value);

			it->it_active = true;
		} else {
			it->it_active = false;
		}
	}

	/* Success!  */
	return 0;
}

/*
 * itimer_gettime:
 *
 *	Return the remaining time of an interval timer.
 */
void
itimer_gettime(const struct itimer *it, struct itimerspec *aits)
{
	struct timespec now;
	struct itimer *itn;

	KASSERT(itimer_lock_held());

	*aits = it->it_time;
	if (!CLOCK_VIRTUAL_P(it->it_clockid)) {
		/*
		 * Convert from absolute to relative time in .it_value
		 * part of real time timer.  If time for real time
		 * timer has passed return 0, else return difference
		 * between current time and time for the timer to go
		 * off.
		 */
		if (timespecisset(&aits->it_value)) {
			if (it->it_clockid == CLOCK_REALTIME) {
				getnanotime(&now);
			} else { /* CLOCK_MONOTONIC */
				getnanouptime(&now);
			}
			if (timespeccmp(&aits->it_value, &now, <))
				timespecclear(&aits->it_value);
			else
				timespecsub(&aits->it_value, &now,
				    &aits->it_value);
		}
	} else if (it->it_active) {
		for (itn = LIST_FIRST(it->it_vlist); itn && itn != it;
		     itn = LIST_NEXT(itn, it_list))
			timespecadd(&aits->it_value,
			    &itn->it_time.it_value, &aits->it_value);
		KASSERT(itn != NULL); /* it should be findable on the list */
	} else
		timespecclear(&aits->it_value);
}

/*
 * Per-process timer support.
 *
 * Both the BSD getitimer() family and the POSIX timer_*() family of
 * routines are supported.
 *
 * All timers are kept in an array pointed to by p_timers, which is
 * allocated on demand - many processes don't use timers at all. The
 * first four elements in this array are reserved for the BSD timers:
 * element 0 is ITIMER_REAL, element 1 is ITIMER_VIRTUAL, element
 * 2 is ITIMER_PROF, and element 3 is ITIMER_MONOTONIC. The rest may be
 * allocated by the timer_create() syscall.
 *
 * These timers are a "sub-class" of interval timer.
 */

/*
 * ptimer_free:
 *
 *	Free the per-process timer at the specified index.
 */
static void
ptimer_free(struct ptimers *pts, int index)
{
	struct itimer *it;
	struct ptimer *pt;

	KASSERT(itimer_lock_held());

	it = pts->pts_timers[index];
	pt = container_of(it, struct ptimer, pt_itimer);
	pts->pts_timers[index] = NULL;
	itimer_poison(it);

	/*
	 * Remove it from the queue to be signalled.  Must be done
	 * after itimer is poisoned, because we may have had to wait
	 * for the callout to complete.
	 */
	if (pt->pt_queued) {
		TAILQ_REMOVE(&ptimer_queue, pt, pt_chain);
		pt->pt_queued = false;
	}

	itimer_fini(it);	/* releases itimer_lock */
	kmem_free(pt, sizeof(*pt));
}

/*
 * ptimers_alloc:
 *
 *	Allocate a ptimers for the specified process.
 */
static struct ptimers *
ptimers_alloc(struct proc *p)
{
	struct ptimers *pts;
	int i;

	pts = kmem_alloc(sizeof(*pts), KM_SLEEP);
	LIST_INIT(&pts->pts_virtual);
	LIST_INIT(&pts->pts_prof);
	for (i = 0; i < TIMER_MAX; i++)
		pts->pts_timers[i] = NULL;
	itimer_lock();
	if (p->p_timers == NULL) {
		p->p_timers = pts;
		itimer_unlock();
		return pts;
	}
	itimer_unlock();
	kmem_free(pts, sizeof(*pts));
	return p->p_timers;
}

/*
 * ptimers_free:
 *
 *	Clean up the per-process timers. If "which" is set to TIMERS_ALL,
 *	then clean up all timers and free all the data structures. If
 *	"which" is set to TIMERS_POSIX, only clean up the timers allocated
 *	by timer_create(), not the BSD setitimer() timers, and only free the
 *	structure if none of those remain.
 *
 *	This function is exported because it is needed in the exec and
 *	exit code paths.
 */
void
ptimers_free(struct proc *p, int which)
{
	struct ptimers *pts;
	struct itimer *itn;
	struct timespec ts;
	int i;

	if (p->p_timers == NULL)
		return;

	pts = p->p_timers;
	itimer_lock();
	if (which == TIMERS_ALL) {
		p->p_timers = NULL;
		i = 0;
	} else {
		timespecclear(&ts);
		for (itn = LIST_FIRST(&pts->pts_virtual);
		     itn && itn != pts->pts_timers[ITIMER_VIRTUAL];
		     itn = LIST_NEXT(itn, it_list)) {
			KASSERT(itn->it_clockid == CLOCK_VIRTUAL);
			timespecadd(&ts, &itn->it_time.it_value, &ts);
		}
		LIST_FIRST(&pts->pts_virtual) = NULL;
		if (itn) {
			KASSERT(itn->it_clockid == CLOCK_VIRTUAL);
			timespecadd(&ts, &itn->it_time.it_value,
			    &itn->it_time.it_value);
			LIST_INSERT_HEAD(&pts->pts_virtual, itn, it_list);
		}
		timespecclear(&ts);
		for (itn = LIST_FIRST(&pts->pts_prof);
		     itn && itn != pts->pts_timers[ITIMER_PROF];
		     itn = LIST_NEXT(itn, it_list)) {
			KASSERT(itn->it_clockid == CLOCK_PROF);
			timespecadd(&ts, &itn->it_time.it_value, &ts);
		}
		LIST_FIRST(&pts->pts_prof) = NULL;
		if (itn) {
			KASSERT(itn->it_clockid == CLOCK_PROF);
			timespecadd(&ts, &itn->it_time.it_value,
			    &itn->it_time.it_value);
			LIST_INSERT_HEAD(&pts->pts_prof, itn, it_list);
		}
		i = TIMER_MIN;
	}
	for ( ; i < TIMER_MAX; i++) {
		if (pts->pts_timers[i] != NULL) {
			/* Free the timer and release the lock.  */
			ptimer_free(pts, i);
			/* Reacquire the lock for the next one.  */
			itimer_lock();
		}
	}
	if (pts->pts_timers[0] == NULL && pts->pts_timers[1] == NULL &&
	    pts->pts_timers[2] == NULL && pts->pts_timers[3] == NULL) {
		p->p_timers = NULL;
		itimer_unlock();
		kmem_free(pts, sizeof(*pts));
	} else
		itimer_unlock();
}

/*
 * ptimer_fire:
 *
 *	Fire a per-process timer.
 */
static void
ptimer_fire(struct itimer *it)
{
	struct ptimer *pt = container_of(it, struct ptimer, pt_itimer);

	KASSERT(itimer_lock_held());

	/*
	 * XXX Can overrun, but we don't do signal queueing yet, anyway.
	 * XXX Relying on the clock interrupt is stupid.
	 */
	if (pt->pt_ev.sigev_notify != SIGEV_SIGNAL) {
		return;
	}

	if (!pt->pt_queued) {
		TAILQ_INSERT_TAIL(&ptimer_queue, pt, pt_chain);
		pt->pt_queued = true;
		softint_schedule(ptimer_sih);
	}
}

/*
 * Operations vector for per-process timers (BSD and POSIX).
 */
static const struct itimer_ops ptimer_itimer_ops = {
	.ito_fire = ptimer_fire,
};

/*
 * sys_timer_create:
 *
 *	System call to create a POSIX timer.
 */
int
sys_timer_create(struct lwp *l, const struct sys_timer_create_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(clockid_t) clock_id;
		syscallarg(struct sigevent *) evp;
		syscallarg(timer_t *) timerid;
	} */

	return timer_create1(SCARG(uap, timerid), SCARG(uap, clock_id),
	    SCARG(uap, evp), copyin, l);
}

int
timer_create1(timer_t *tid, clockid_t id, struct sigevent *evp,
    copyin_t fetch_event, struct lwp *l)
{
	int error;
	timer_t timerid;
	struct itlist *itl;
	struct ptimers *pts;
	struct ptimer *pt;
	struct proc *p;

	p = l->l_proc;

	if ((u_int)id > CLOCK_MONOTONIC)
		return (EINVAL);

	if ((pts = p->p_timers) == NULL)
		pts = ptimers_alloc(p);

	pt = kmem_zalloc(sizeof(*pt), KM_SLEEP);
	if (evp != NULL) {
		if (((error =
		    (*fetch_event)(evp, &pt->pt_ev, sizeof(pt->pt_ev))) != 0) ||
		    ((pt->pt_ev.sigev_notify < SIGEV_NONE) ||
			(pt->pt_ev.sigev_notify > SIGEV_SA)) ||
			(pt->pt_ev.sigev_notify == SIGEV_SIGNAL &&
			 (pt->pt_ev.sigev_signo <= 0 ||
			  pt->pt_ev.sigev_signo >= NSIG))) {
			kmem_free(pt, sizeof(*pt));
			return (error ? error : EINVAL);
		}
	}

	/* Find a free timer slot, skipping those reserved for setitimer(). */
	itimer_lock();
	for (timerid = TIMER_MIN; timerid < TIMER_MAX; timerid++)
		if (pts->pts_timers[timerid] == NULL)
			break;
	if (timerid == TIMER_MAX) {
		itimer_unlock();
		kmem_free(pt, sizeof(*pt));
		return EAGAIN;
	}
	if (evp == NULL) {
		pt->pt_ev.sigev_notify = SIGEV_SIGNAL;
		switch (id) {
		case CLOCK_REALTIME:
		case CLOCK_MONOTONIC:
			pt->pt_ev.sigev_signo = SIGALRM;
			break;
		case CLOCK_VIRTUAL:
			pt->pt_ev.sigev_signo = SIGVTALRM;
			break;
		case CLOCK_PROF:
			pt->pt_ev.sigev_signo = SIGPROF;
			break;
		}
		pt->pt_ev.sigev_value.sival_int = timerid;
	}

	switch (id) {
	case CLOCK_VIRTUAL:
		itl = &pts->pts_virtual;
		break;
	case CLOCK_PROF:
		itl = &pts->pts_prof;
		break;
	default:
		itl = NULL;
	}

	itimer_init(&pt->pt_itimer, &ptimer_itimer_ops, id, itl);
	pt->pt_proc = p;
	pt->pt_poverruns = 0;
	pt->pt_entry = timerid;
	pt->pt_queued = false;

	pts->pts_timers[timerid] = &pt->pt_itimer;
	itimer_unlock();

	return copyout(&timerid, tid, sizeof(timerid));
}

/*
 * sys_timer_delete:
 *
 *	System call to delete a POSIX timer.
 */
int
sys_timer_delete(struct lwp *l, const struct sys_timer_delete_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(timer_t) timerid;
	} */
	struct proc *p = l->l_proc;
	timer_t timerid;
	struct ptimers *pts;
	struct itimer *it, *itn;

	timerid = SCARG(uap, timerid);
	pts = p->p_timers;
	
	if (pts == NULL || timerid < 2 || timerid >= TIMER_MAX)
		return (EINVAL);

	itimer_lock();
	if ((it = pts->pts_timers[timerid]) == NULL) {
		itimer_unlock();
		return (EINVAL);
	}

	if (CLOCK_VIRTUAL_P(it->it_clockid)) {
		if (it->it_active) {
			itn = LIST_NEXT(it, it_list);
			LIST_REMOVE(it, it_list);
			for ( ; itn; itn = LIST_NEXT(itn, it_list))
				timespecadd(&it->it_time.it_value,
				    &itn->it_time.it_value,
				    &itn->it_time.it_value);
			it->it_active = false;
		}
	}

	/* Free the timer and release the lock.  */
	ptimer_free(pts, timerid);

	return (0);
}

/*
 * sys___timer_settime50:
 *
 *	System call to set/arm a POSIX timer.
 */
int
sys___timer_settime50(struct lwp *l,
    const struct sys___timer_settime50_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(timer_t) timerid;
		syscallarg(int) flags;
		syscallarg(const struct itimerspec *) value;
		syscallarg(struct itimerspec *) ovalue;
	} */
	int error;
	struct itimerspec value, ovalue, *ovp = NULL;

	if ((error = copyin(SCARG(uap, value), &value,
	    sizeof(struct itimerspec))) != 0)
		return (error);

	if (SCARG(uap, ovalue))
		ovp = &ovalue;

	if ((error = dotimer_settime(SCARG(uap, timerid), &value, ovp,
	    SCARG(uap, flags), l->l_proc)) != 0)
		return error;

	if (ovp)
		return copyout(&ovalue, SCARG(uap, ovalue),
		    sizeof(struct itimerspec));
	return 0;
}

int
dotimer_settime(int timerid, struct itimerspec *value,
    struct itimerspec *ovalue, int flags, struct proc *p)
{
	struct timespec now;
	struct itimerspec val, oval;
	struct ptimers *pts;
	struct itimer *it;
	int error;

	pts = p->p_timers;

	if (pts == NULL || timerid < 2 || timerid >= TIMER_MAX)
		return EINVAL;
	val = *value;
	if ((error = itimespecfix(&val.it_value)) != 0 ||
	    (error = itimespecfix(&val.it_interval)) != 0)
		return error;

	itimer_lock();
 restart:
	if ((it = pts->pts_timers[timerid]) == NULL) {
		itimer_unlock();
		return EINVAL;
	}

	oval = it->it_time;
	it->it_time = val;

	/*
	 * If we've been passed a relative time for a realtime timer,
	 * convert it to absolute; if an absolute time for a virtual
	 * timer, convert it to relative and make sure we don't set it
	 * to zero, which would cancel the timer, or let it go
	 * negative, which would confuse the comparison tests.
	 */
	if (timespecisset(&it->it_time.it_value)) {
		if (!CLOCK_VIRTUAL_P(it->it_clockid)) {
			if ((flags & TIMER_ABSTIME) == 0) {
				if (it->it_clockid == CLOCK_REALTIME) {
					getnanotime(&now);
				} else { /* CLOCK_MONOTONIC */
					getnanouptime(&now);
				}
				timespecadd(&it->it_time.it_value, &now,
				    &it->it_time.it_value);
			}
		} else {
			if ((flags & TIMER_ABSTIME) != 0) {
				getnanotime(&now);
				timespecsub(&it->it_time.it_value, &now,
				    &it->it_time.it_value);
				if (!timespecisset(&it->it_time.it_value) ||
				    it->it_time.it_value.tv_sec < 0) {
					it->it_time.it_value.tv_sec = 0;
					it->it_time.it_value.tv_nsec = 1;
				}
			}
		}
	}

	error = itimer_settime(it);
	if (error == ERESTART) {
		KASSERT(!CLOCK_VIRTUAL_P(it->it_clockid));
		goto restart;
	}
	KASSERT(error == 0);
	itimer_unlock();

	if (ovalue)
		*ovalue = oval;

	return (0);
}

/*
 * sys___timer_gettime50:
 *
 *	System call to return the time remaining until a POSIX timer fires.
 */
int
sys___timer_gettime50(struct lwp *l,
    const struct sys___timer_gettime50_args *uap, register_t *retval)
{
	/* {
		syscallarg(timer_t) timerid;
		syscallarg(struct itimerspec *) value;
	} */
	struct itimerspec its;
	int error;

	if ((error = dotimer_gettime(SCARG(uap, timerid), l->l_proc,
	    &its)) != 0)
		return error;

	return copyout(&its, SCARG(uap, value), sizeof(its));
}

int
dotimer_gettime(int timerid, struct proc *p, struct itimerspec *its)
{
	struct itimer *it;
	struct ptimers *pts;

	pts = p->p_timers;
	if (pts == NULL || timerid < 2 || timerid >= TIMER_MAX)
		return (EINVAL);
	itimer_lock();
	if ((it = pts->pts_timers[timerid]) == NULL) {
		itimer_unlock();
		return (EINVAL);
	}
	itimer_gettime(it, its);
	itimer_unlock();

	return 0;
}

/*
 * sys_timer_getoverrun:
 *
 *	System call to return the number of times a POSIX timer has
 *	expired while a notification was already pending.  The counter
 *	is reset when a timer expires and a notification can be posted.
 */
int
sys_timer_getoverrun(struct lwp *l, const struct sys_timer_getoverrun_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(timer_t) timerid;
	} */
	struct proc *p = l->l_proc;
	struct ptimers *pts;
	int timerid;
	struct itimer *it;
	struct ptimer *pt;

	timerid = SCARG(uap, timerid);

	pts = p->p_timers;
	if (pts == NULL || timerid < 2 || timerid >= TIMER_MAX)
		return (EINVAL);
	itimer_lock();
	if ((it = pts->pts_timers[timerid]) == NULL) {
		itimer_unlock();
		return (EINVAL);
	}
	pt = container_of(it, struct ptimer, pt_itimer);
	*retval = pt->pt_poverruns;
	if (*retval >= DELAYTIMER_MAX)
		*retval = DELAYTIMER_MAX;
	itimer_unlock();

	return (0);
}

/*
 * sys___getitimer50:
 *
 *	System call to get the time remaining before a BSD timer fires.
 */
int
sys___getitimer50(struct lwp *l, const struct sys___getitimer50_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(struct itimerval *) itv;
	} */
	struct proc *p = l->l_proc;
	struct itimerval aitv;
	int error;

	memset(&aitv, 0, sizeof(aitv));
	error = dogetitimer(p, SCARG(uap, which), &aitv);
	if (error)
		return error;
	return (copyout(&aitv, SCARG(uap, itv), sizeof(struct itimerval)));
}

int
dogetitimer(struct proc *p, int which, struct itimerval *itvp)
{
	struct ptimers *pts;
	struct itimer *it;
	struct itimerspec its;

	if ((u_int)which > ITIMER_MONOTONIC)
		return (EINVAL);

	itimer_lock();
	pts = p->p_timers;
	if (pts == NULL || (it = pts->pts_timers[which]) == NULL) {
		timerclear(&itvp->it_value);
		timerclear(&itvp->it_interval);
	} else {
		itimer_gettime(it, &its);
		TIMESPEC_TO_TIMEVAL(&itvp->it_value, &its.it_value);
		TIMESPEC_TO_TIMEVAL(&itvp->it_interval, &its.it_interval);
	}
	itimer_unlock();

	return 0;
}

/*
 * sys___setitimer50:
 *
 *	System call to set/arm a BSD timer.
 */
int
sys___setitimer50(struct lwp *l, const struct sys___setitimer50_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(const struct itimerval *) itv;
		syscallarg(struct itimerval *) oitv;
	} */
	struct proc *p = l->l_proc;
	int which = SCARG(uap, which);
	struct sys___getitimer50_args getargs;
	const struct itimerval *itvp;
	struct itimerval aitv;
	int error;

	itvp = SCARG(uap, itv);
	if (itvp &&
	    (error = copyin(itvp, &aitv, sizeof(struct itimerval))) != 0)
		return (error);
	if (SCARG(uap, oitv) != NULL) {
		SCARG(&getargs, which) = which;
		SCARG(&getargs, itv) = SCARG(uap, oitv);
		if ((error = sys___getitimer50(l, &getargs, retval)) != 0)
			return (error);
	}
	if (itvp == 0)
		return (0);

	return dosetitimer(p, which, &aitv);
}

int
dosetitimer(struct proc *p, int which, struct itimerval *itvp)
{
	struct timespec now;
	struct ptimers *pts;
	struct ptimer *spare;
	struct itimer *it;
	struct itlist *itl;
	int error;

	if ((u_int)which > ITIMER_MONOTONIC)
		return (EINVAL);
	if (itimerfix(&itvp->it_value) || itimerfix(&itvp->it_interval))
		return (EINVAL);

	/*
	 * Don't bother allocating data structures if the process just
	 * wants to clear the timer.
	 */
	spare = NULL;
	pts = p->p_timers;
 retry:
	if (!timerisset(&itvp->it_value) && (pts == NULL ||
	    pts->pts_timers[which] == NULL))
		return (0);
	if (pts == NULL)
		pts = ptimers_alloc(p);
	itimer_lock();
 restart:
	it = pts->pts_timers[which];
	if (it == NULL) {
		struct ptimer *pt;

		if (spare == NULL) {
			itimer_unlock();
			spare = kmem_zalloc(sizeof(*spare), KM_SLEEP);
			goto retry;
		}
		pt = spare;
		spare = NULL;

		it = &pt->pt_itimer;
		pt->pt_ev.sigev_notify = SIGEV_SIGNAL;
		pt->pt_ev.sigev_value.sival_int = which;

		switch (which) {
		case ITIMER_REAL:
		case ITIMER_MONOTONIC:
			itl = NULL;
			pt->pt_ev.sigev_signo = SIGALRM;
			break;
		case ITIMER_VIRTUAL:
			itl = &pts->pts_virtual;
			pt->pt_ev.sigev_signo = SIGVTALRM;
			break;
		case ITIMER_PROF:
			itl = &pts->pts_prof;
			pt->pt_ev.sigev_signo = SIGPROF;
			break;
		default:
			panic("%s: can't happen %d", __func__, which);
		}
		itimer_init(it, &ptimer_itimer_ops, which, itl);
		pt->pt_proc = p;
		pt->pt_entry = which;

		pts->pts_timers[which] = it;
	}

	TIMEVAL_TO_TIMESPEC(&itvp->it_value, &it->it_time.it_value);
	TIMEVAL_TO_TIMESPEC(&itvp->it_interval, &it->it_time.it_interval);

	if (timespecisset(&it->it_time.it_value)) {
		/* Convert to absolute time */
		/* XXX need to wrap in splclock for timecounters case? */
		switch (which) {
		case ITIMER_REAL:
			getnanotime(&now);
			timespecadd(&it->it_time.it_value, &now,
			    &it->it_time.it_value);
			break;
		case ITIMER_MONOTONIC:
			getnanouptime(&now);
			timespecadd(&it->it_time.it_value, &now,
			    &it->it_time.it_value);
			break;
		default:
			break;
		}
	}
	error = itimer_settime(it);
	if (error == ERESTART) {
		KASSERT(!CLOCK_VIRTUAL_P(it->it_clockid));
		goto restart;
	}
	KASSERT(error == 0);
	itimer_unlock();
	if (spare != NULL)
		kmem_free(spare, sizeof(*spare));

	return (0);
}

/*
 * ptimer_tick:
 *
 *	Called from hardclock() to decrement per-process virtual timers.
 */
void
ptimer_tick(lwp_t *l, bool user)
{
	struct ptimers *pts;
	struct itimer *it;
	proc_t *p;

	p = l->l_proc;
	if (p->p_timers == NULL)
		return;

	itimer_lock();
	if ((pts = l->l_proc->p_timers) != NULL) {
		/*
		 * Run current process's virtual and profile time, as needed.
		 */
		if (user && (it = LIST_FIRST(&pts->pts_virtual)) != NULL)
			if (itimer_decr(it, tick * 1000))
				(*it->it_ops->ito_fire)(it);
		if ((it = LIST_FIRST(&pts->pts_prof)) != NULL)
			if (itimer_decr(it, tick * 1000))
				(*it->it_ops->ito_fire)(it);
	}
	itimer_unlock();
}

/*
 * ptimer_intr:
 *
 *	Software interrupt handler for processing per-process
 *	timer expiration.
 */
static void
ptimer_intr(void *cookie)
{
	ksiginfo_t ksi;
	struct itimer *it;
	struct ptimer *pt;
	proc_t *p;
	
	mutex_enter(&proc_lock);
	itimer_lock();
	while ((pt = TAILQ_FIRST(&ptimer_queue)) != NULL) {
		it = &pt->pt_itimer;

		TAILQ_REMOVE(&ptimer_queue, pt, pt_chain);
		KASSERT(pt->pt_queued);
		pt->pt_queued = false;

		p = pt->pt_proc;
		if (p->p_timers == NULL) {
			/* Process is dying. */
			continue;
		}
		if (pt->pt_ev.sigev_notify != SIGEV_SIGNAL) {
			continue;
		}
		if (sigismember(&p->p_sigpend.sp_set, pt->pt_ev.sigev_signo)) {
			it->it_overruns++;
			continue;
		}

		KSI_INIT(&ksi);
		ksi.ksi_signo = pt->pt_ev.sigev_signo;
		ksi.ksi_code = SI_TIMER;
		ksi.ksi_value = pt->pt_ev.sigev_value;
		pt->pt_poverruns = it->it_overruns;
		it->it_overruns = 0;
		itimer_unlock();
		kpsignal(p, &ksi, NULL);
		itimer_lock();
	}
	itimer_unlock();
	mutex_exit(&proc_lock);
}
