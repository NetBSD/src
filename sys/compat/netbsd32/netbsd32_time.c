/*	$NetBSD: netbsd32_time.c,v 1.19.2.1 2006/05/24 10:57:31 yamt Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_time.c,v 1.19.2.1 2006/05/24 10:57:31 yamt Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ntp.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/timevar.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/resourcevar.h>
#include <sys/dirent.h>
#include <sys/kauth.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

#ifdef NTP

#ifdef PPS_SYNC
/*
 * The following variables are used only if the PPS signal discipline
 * is configured in the kernel.
 */
extern int pps_shift;		/* interval duration (s) (shift) */
extern long pps_freq;		/* pps frequency offset (scaled ppm) */
extern long pps_jitter;		/* pps jitter (us) */
extern long pps_stabil;		/* pps stability (scaled ppm) */
extern long pps_jitcnt;		/* jitter limit exceeded */
extern long pps_calcnt;		/* calibration intervals */
extern long pps_errcnt;		/* calibration errors */
extern long pps_stbcnt;		/* stability limit exceeded */
#endif /* PPS_SYNC */

int
netbsd32_ntp_gettime(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_ntp_gettime_args /* {
		syscallarg(netbsd32_ntptimevalp_t) ntvp;
	} */ *uap = v;
	struct netbsd32_ntptimeval ntv32;
	struct timeval atv;
	struct ntptimeval ntv;
	int error = 0;
	int s;

	/* The following are NTP variables */
	extern long time_maxerror;
	extern long time_esterror;
	extern int time_status;
	extern int time_state;	/* clock state */
	extern int time_status;	/* clock status bits */

	if (SCARG(uap, ntvp)) {
		s = splclock();
#ifdef EXT_CLOCK
		/*
		 * The microtime() external clock routine returns a
		 * status code. If less than zero, we declare an error
		 * in the clock status word and return the kernel
		 * (software) time variable. While there are other
		 * places that call microtime(), this is the only place
		 * that matters from an application point of view.
		 */
		if (microtime(&atv) < 0) {
			time_status |= STA_CLOCKERR;
			ntv.time = time;
		} else
			time_status &= ~STA_CLOCKERR;
#else /* EXT_CLOCK */
		microtime(&atv);
#endif /* EXT_CLOCK */
		ntv.time = atv;
		ntv.maxerror = time_maxerror;
		ntv.esterror = time_esterror;
		(void) splx(s);

		netbsd32_from_timeval(&ntv.time, &ntv32.time);
		ntv32.maxerror = (netbsd32_long)ntv.maxerror;
		ntv32.esterror = (netbsd32_long)ntv.esterror;
		error = copyout((caddr_t)&ntv32,
		    (caddr_t)NETBSD32PTR64(SCARG(uap, ntvp)), sizeof(ntv32));
	}
	if (!error) {

		/*
		 * Status word error decode. If any of these conditions
		 * occur, an error is returned, instead of the status
		 * word. Most applications will care only about the fact
		 * the system clock may not be trusted, not about the
		 * details.
		 *
		 * Hardware or software error
		 */
		if ((time_status & (STA_UNSYNC | STA_CLOCKERR)) ||

		/*
		 * PPS signal lost when either time or frequency
		 * synchronization requested
		 */
		    (time_status & (STA_PPSFREQ | STA_PPSTIME) &&
		    !(time_status & STA_PPSSIGNAL)) ||

		/*
		 * PPS jitter exceeded when time synchronization
		 * requested
		 */
		    (time_status & STA_PPSTIME &&
		    time_status & STA_PPSJITTER) ||

		/*
		 * PPS wander exceeded or calibration error when
		 * frequency synchronization requested
		 */
		    (time_status & STA_PPSFREQ &&
		    time_status & (STA_PPSWANDER | STA_PPSERROR)))
			*retval = TIME_ERROR;
		else
			*retval = time_state;
	}
	return (error);
}

int
netbsd32_ntp_adjtime(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_ntp_adjtime_args /* {
		syscallarg(netbsd32_timexp_t) tp;
	} */ *uap = v;
	struct netbsd32_timex ntv32;
	struct timex ntv;
	int error = 0;
	int modes;
	int s;
	struct proc *p = l->l_proc;
	extern long time_freq;		/* frequency offset (scaled ppm) */
	extern long time_maxerror;
	extern long time_esterror;
	extern int time_state;	/* clock state */
	extern int time_status;	/* clock status bits */
	extern long time_constant;		/* pll time constant */
	extern long time_offset;		/* time offset (us) */
	extern long time_tolerance;	/* frequency tolerance (scaled ppm) */
	extern long time_precision;	/* clock precision (us) */

	if ((error = copyin((caddr_t)NETBSD32PTR64(SCARG(uap, tp)),
	    (caddr_t)&ntv32, sizeof(ntv32))))
		return (error);
	netbsd32_to_timex(&ntv32, &ntv);

	/*
	 * Update selected clock variables - only the superuser can
	 * change anything. Note that there is no error checking here on
	 * the assumption the superuser should know what it is doing.
	 */
	modes = ntv.modes;
	if (modes != 0 && (error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag)))
		return (error);

	s = splclock();
	if (modes & MOD_FREQUENCY)
#ifdef PPS_SYNC
		time_freq = ntv.freq - pps_freq;
#else /* PPS_SYNC */
		time_freq = ntv.freq;
#endif /* PPS_SYNC */
	if (modes & MOD_MAXERROR)
		time_maxerror = ntv.maxerror;
	if (modes & MOD_ESTERROR)
		time_esterror = ntv.esterror;
	if (modes & MOD_STATUS) {
		time_status &= STA_RONLY;
		time_status |= ntv.status & ~STA_RONLY;
	}
	if (modes & MOD_TIMECONST)
		time_constant = ntv.constant;
	if (modes & MOD_OFFSET)
		hardupdate(ntv.offset);

	/*
	 * Retrieve all clock variables
	 */
	if (time_offset < 0)
		ntv.offset = -(-time_offset >> SHIFT_UPDATE);
	else
		ntv.offset = time_offset >> SHIFT_UPDATE;
#ifdef PPS_SYNC
	ntv.freq = time_freq + pps_freq;
#else /* PPS_SYNC */
	ntv.freq = time_freq;
#endif /* PPS_SYNC */
	ntv.maxerror = time_maxerror;
	ntv.esterror = time_esterror;
	ntv.status = time_status;
	ntv.constant = time_constant;
	ntv.precision = time_precision;
	ntv.tolerance = time_tolerance;
#ifdef PPS_SYNC
	ntv.shift = pps_shift;
	ntv.ppsfreq = pps_freq;
	ntv.jitter = pps_jitter >> PPS_AVG;
	ntv.stabil = pps_stabil;
	ntv.calcnt = pps_calcnt;
	ntv.errcnt = pps_errcnt;
	ntv.jitcnt = pps_jitcnt;
	ntv.stbcnt = pps_stbcnt;
#endif /* PPS_SYNC */
	(void)splx(s);

	netbsd32_from_timex(&ntv, &ntv32);
	error = copyout((caddr_t)&ntv32, (caddr_t)NETBSD32PTR64(SCARG(uap, tp)),
	    sizeof(ntv32));
	if (!error) {

		/*
		 * Status word error decode. See comments in
		 * ntp_gettime() routine.
		 */
		if ((time_status & (STA_UNSYNC | STA_CLOCKERR)) ||
		    (time_status & (STA_PPSFREQ | STA_PPSTIME) &&
		    !(time_status & STA_PPSSIGNAL)) ||
		    (time_status & STA_PPSTIME &&
		    time_status & STA_PPSJITTER) ||
		    (time_status & STA_PPSFREQ &&
		    time_status & (STA_PPSWANDER | STA_PPSERROR)))
			*retval = TIME_ERROR;
		else
			*retval = time_state;
	}
	return error;
}
#else
int
netbsd32_ntp_gettime(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

	return (ENOSYS);
}

int
netbsd32_ntp_adjtime(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

	return (ENOSYS);
}
#endif

int
netbsd32_setitimer(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_setitimer_args /* {
		syscallarg(int) which;
		syscallarg(const netbsd32_itimervalp_t) itv;
		syscallarg(netbsd32_itimervalp_t) oitv;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct netbsd32_itimerval s32it, *itv32;
	int which = SCARG(uap, which);
	struct netbsd32_getitimer_args getargs;
	struct itimerval aitv;
	int error;

	if ((u_int)which > ITIMER_PROF)
		return (EINVAL);
	itv32 = (struct netbsd32_itimerval *)NETBSD32PTR64(SCARG(uap, itv));
	if (itv32) {
		if ((error = copyin(itv32, &s32it, sizeof(s32it))))
			return (error);
		netbsd32_to_itimerval(&s32it, &aitv);
	}
	if (SCARG(uap, oitv) != 0) {
		SCARG(&getargs, which) = which;
		SCARG(&getargs, itv) = SCARG(uap, oitv);
		if ((error = netbsd32_getitimer(l, &getargs, retval)) != 0)
			return (error);
	}
	if (itv32 == 0)
		return 0;

	return dosetitimer(p, which, &aitv);
}

int
netbsd32_getitimer(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_getitimer_args /* {
		syscallarg(int) which;
		syscallarg(netbsd32_itimervalp_t) itv;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct netbsd32_itimerval s32it;
	struct itimerval aitv;
	int error;

	error = dogetitimer(p, SCARG(uap, which), &aitv);
	if (error)
		return error;

	netbsd32_from_itimerval(&aitv, &s32it);
	return (copyout(&s32it, (caddr_t)NETBSD32PTR64(SCARG(uap, itv)),
	    sizeof(s32it)));
}

int
netbsd32_gettimeofday(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_gettimeofday_args /* {
		syscallarg(netbsd32_timevalp_t) tp;
		syscallarg(netbsd32_timezonep_t) tzp;
	} */ *uap = v;
	struct timeval atv;
	struct netbsd32_timeval tv32;
	int error = 0;
	struct netbsd32_timezone tzfake;

	if (SCARG(uap, tp)) {
		microtime(&atv);
		netbsd32_from_timeval(&atv, &tv32);
		error = copyout(&tv32, (caddr_t)NETBSD32PTR64(SCARG(uap, tp)),
		    sizeof(tv32));
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
		error = copyout(&tzfake,
		    (caddr_t)NETBSD32PTR64(SCARG(uap, tzp)), sizeof(tzfake));
	}
	return (error);
}

int
netbsd32_settimeofday(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_settimeofday_args /* {
		syscallarg(const netbsd32_timevalp_t) tv;
		syscallarg(const netbsd32_timezonep_t) tzp;
	} */ *uap = v;
	struct netbsd32_timeval atv32;
	struct timeval atv;
	struct timespec ats;
	int error;
	struct proc *p = l->l_proc;

	/* Verify all parameters before changing time. */
	if ((error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag)) != 0)
		return error;

	/*
	 * NetBSD has no kernel notion of time zone, and only an
	 * obsolete program would try to set it, so we log a warning.
	 */
	if (SCARG(uap, tzp))
		printf("pid %d attempted to set the "
		    "(obsolete) kernel time zone\n", p->p_pid);

	if (SCARG(uap, tv) == 0)
		return 0;

	if ((error = copyin((caddr_t)NETBSD32PTR64(SCARG(uap, tv)), &atv32,
	    sizeof(atv32))) != 0)
		return error;

	netbsd32_to_timeval(&atv32, &atv);
	TIMEVAL_TO_TIMESPEC(&atv, &ats);
	return settime(p, &ats);
}

int
netbsd32_adjtime(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_adjtime_args /* {
		syscallarg(const netbsd32_timevalp_t) delta;
		syscallarg(netbsd32_timevalp_t) olddelta;
	} */ *uap = v;
	struct netbsd32_timeval atv;
	int32_t ndelta, ntickdelta, odelta;
	int s, error;
	struct proc *p = l->l_proc;
	extern long bigadj, timedelta;
	extern int tickdelta;

	if ((error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag)) != 0)
		return (error);

	error = copyin((caddr_t)NETBSD32PTR64(SCARG(uap, delta)), &atv,
	    sizeof(struct timeval));
	if (error)
		return (error);
	/*
	 * Compute the total correction and the rate at which to apply it.
	 * Round the adjustment down to a whole multiple of the per-tick
	 * delta, so that after some number of incremental changes in
	 * hardclock(), tickdelta will become zero, lest the correction
	 * overshoot and start taking us away from the desired final time.
	 */
	ndelta = atv.tv_sec * 1000000 + atv.tv_usec;
	if (ndelta > bigadj)
		ntickdelta = 10 * tickadj;
	else
		ntickdelta = tickadj;
	if (ndelta % ntickdelta)
		ndelta = ndelta / ntickdelta * ntickdelta;

	/*
	 * To make hardclock()'s job easier, make the per-tick delta negative
	 * if we want time to run slower; then hardclock can simply compute
	 * tick + tickdelta, and subtract tickdelta from timedelta.
	 */
	if (ndelta < 0)
		ntickdelta = -ntickdelta;
	s = splclock();
	odelta = timedelta;
	timedelta = ndelta;
	tickdelta = ntickdelta;
	splx(s);

	if (SCARG(uap, olddelta)) {
		atv.tv_sec = odelta / 1000000;
		atv.tv_usec = odelta % 1000000;
		(void) copyout(&atv,
		    (caddr_t)NETBSD32PTR64(SCARG(uap, olddelta)), sizeof(atv));
	}
	return (0);
}

int
netbsd32_clock_gettime(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_clock_gettime_args /* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(netbsd32_timespecp_t) tp;
	} */ *uap = v;
	clockid_t clock_id;
	struct timespec ats;
	struct netbsd32_timespec ts32;

	clock_id = SCARG(uap, clock_id);
	if (clock_id != CLOCK_REALTIME)
		return (EINVAL);

	nanotime(&ats);
	netbsd32_from_timespec(&ats, &ts32);

	return copyout(&ts32, (caddr_t)NETBSD32PTR64(SCARG(uap, tp)),
	    sizeof(ts32));
}

int
netbsd32_clock_settime(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_clock_settime_args /* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(const netbsd32_timespecp_t) tp;
	} */ *uap = v;
	struct netbsd32_timespec ts32;
	clockid_t clock_id;
	struct timespec ats;
	int error;
	struct proc *p = l->l_proc;

	if ((error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag)) != 0)
		return (error);

	clock_id = SCARG(uap, clock_id);
	if (clock_id != CLOCK_REALTIME)
		return (EINVAL);

	if ((error = copyin((caddr_t)NETBSD32PTR64(SCARG(uap, tp)), &ts32,
	    sizeof(ts32))) != 0)
		return (error);

	netbsd32_to_timespec(&ts32, &ats);
	return settime(p, &ats);
}

int
netbsd32_clock_getres(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_clock_getres_args /* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(netbsd32_timespecp_t) tp;
	} */ *uap = v;
	struct netbsd32_timespec ts32;
	clockid_t clock_id;
	struct timespec ts;
	int error = 0;

	clock_id = SCARG(uap, clock_id);
	if (clock_id != CLOCK_REALTIME)
		return (EINVAL);

	if (SCARG(uap, tp)) {
		ts.tv_sec = 0;
		ts.tv_nsec = 1000000000 / hz;

		netbsd32_from_timespec(&ts, &ts32);
		error = copyout(&ts, (caddr_t)NETBSD32PTR64(SCARG(uap, tp)),
		    sizeof(ts));
	}

	return error;
}

int
netbsd32_nanosleep(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_nanosleep_args /* {
		syscallarg(const netbsd32_timespecp_t) rqtp;
		syscallarg(netbsd32_timespecp_t) rmtp;
	} */ *uap = v;
	static int nanowait;
	struct netbsd32_timespec ts32;
	struct timespec rqt;
	struct timespec rmt;
	struct timeval atv, utv;
	int error, s, timo;

	error = copyin((caddr_t)NETBSD32PTR64(SCARG(uap, rqtp)), (caddr_t)&ts32,
	    sizeof(ts32));
	if (error)
		return (error);

	netbsd32_to_timespec(&ts32, &rqt);
	TIMESPEC_TO_TIMEVAL(&atv,&rqt);
	if (itimerfix(&atv))
		return (EINVAL);

	s = splclock();
	timeradd(&atv,&time,&atv);
	timo = hzto(&atv);
	/*
	 * Avoid inadvertantly sleeping forever
	 */
	if (timo == 0)
		timo = 1;
	splx(s);

	error = tsleep(&nanowait, PWAIT | PCATCH, "nanosleep", timo);
	if (error == ERESTART)
		error = EINTR;
	if (error == EWOULDBLOCK)
		error = 0;

	if (SCARG(uap, rmtp)) {
		int error1;

		s = splclock();
		utv = time;
		splx(s);

		timersub(&atv, &utv, &utv);
		if (utv.tv_sec < 0)
			timerclear(&utv);

		TIMEVAL_TO_TIMESPEC(&utv,&rmt);
		netbsd32_from_timespec(&rmt, &ts32);
		error1 = copyout(&ts32,
		    NETBSD32PTR64(SCARG(uap,rmtp)), sizeof(ts32));
		if (error1)
			return (error1);
	}

	return error;
}

static int
netbsd32_timer_create_fetch(const void *src, void *dst, size_t size)
{
	struct sigevent *evp = dst;
	struct netbsd32_sigevent ev32;
	int error;

	error = copyin(src, &ev32, sizeof(ev32));
	if (error)
		return error;

	netbsd32_to_sigevent(&ev32, evp);
	return 0;
}

int
netbsd32_timer_create(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_timer_create_args /* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(netbsd32_sigeventp_t) evp;
		syscallarg(netbsd32_timerp_t) timerid;
	} */ *uap = v;

	return timer_create1(NETBSD32PTR64(SCARG(uap, timerid)),
	    SCARG(uap, clock_id), NETBSD32PTR64(SCARG(uap, evp)),
	    netbsd32_timer_create_fetch, l->l_proc);
}

int
netbsd32_timer_delete(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_timer_delete_args /* {
		syscallarg(netbsd32_timer_t) timerid;
	} */ *uap = v;
	struct sys_timer_delete_args ua;

	NETBSD32TO64_UAP(timerid);
	return sys_timer_delete(l, (void *)&ua, retval);
}

int
netbsd32_timer_settime(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_timer_settime_args /* {
		syscallarg(netbsd32_timer_t) timerid;
		syscallarg(int) flags;
		syscallarg(const netbsd32_itimerspecp_t) value;
		syscallarg(netbsd32_itimerspecp_t) ovalue;
	} */ *uap = v;
	int error;
	struct itimerspec value, ovalue, *ovp = NULL;
	struct netbsd32_itimerspec its32;

	if ((error = copyin(NETBSD32PTR64(SCARG(uap, value)), &its32,
	    sizeof(its32))) != 0)
		return (error);
	netbsd32_to_timespec(&its32.it_interval, &value.it_interval);
	netbsd32_to_timespec(&its32.it_value, &value.it_value);

	if (SCARG(uap, ovalue))
		ovp = &ovalue;

	if ((error = dotimer_settime(SCARG(uap, timerid), &value, ovp,
	    SCARG(uap, flags), l->l_proc)) != 0)
		return error;

	if (ovp) {
		netbsd32_from_timespec(&ovp->it_interval, &its32.it_interval);
		netbsd32_from_timespec(&ovp->it_value, &its32.it_value);
		return copyout(&its32, NETBSD32PTR64(SCARG(uap, ovalue)),
		    sizeof(its32));
	}
	return 0;
}

int
netbsd32_timer_gettime(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_timer_gettime_args /* {
		syscallarg(netbsd32_timer_t) timerid;
		syscallarg(netbsd32_itimerspecp_t) value;
	} */ *uap = v;
	int error;
	struct itimerspec its;
	struct netbsd32_itimerspec its32;

	if ((error = dotimer_gettime(SCARG(uap, timerid), l->l_proc,
	    &its)) != 0)
		return error;

	netbsd32_from_timespec(&its.it_interval, &its32.it_interval);
	netbsd32_from_timespec(&its.it_value, &its32.it_value);

	return copyout(&its32, (caddr_t)NETBSD32PTR64(SCARG(uap, value)),
	    sizeof(its32));
}

int
netbsd32_timer_getoverrun(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_timer_getoverrun_args /* {
		syscallarg(netbsd32_timer_t) timerid;
	} */ *uap = v;
	struct sys_timer_getoverrun_args ua;

	NETBSD32TO64_UAP(timerid);
	return sys_timer_getoverrun(l, (void *)&ua, retval);
}
