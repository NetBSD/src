/*	$NetBSD: netbsd32_time.c,v 1.27.10.1 2008/01/09 01:51:40 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_time.c,v 1.27.10.1 2008/01/09 01:51:40 matt Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ntp.h"
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/timevar.h>
#include <sys/timetc.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/resourcevar.h>
#include <sys/dirent.h>
#include <sys/kauth.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

#ifdef NTP

int
netbsd32_ntp_gettime(struct lwp *l, const struct netbsd32_ntp_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_ntptimevalp_t) ntvp;
	} */
	struct netbsd32_ntptimeval ntv32;
	struct ntptimeval ntv;
	int error = 0;

	if (SCARG_P32(uap, ntvp)) {
		ntp_gettime(&ntv);

		ntv32.time.tv_sec = ntv.time.tv_sec;
		ntv32.time.tv_nsec = ntv.time.tv_nsec;
		ntv32.maxerror = (netbsd32_long)ntv.maxerror;
		ntv32.esterror = (netbsd32_long)ntv.esterror;
		ntv32.tai = (netbsd32_long)ntv.tai;
		ntv32.time_state = ntv.time_state;
		error = copyout(&ntv32, SCARG_P32(uap, ntvp), sizeof(ntv32));
	}
	if (!error) {
		*retval = ntp_timestatus();
	}

	return (error);
}

#ifdef COMPAT_30
int
compat_30_netbsd32_ntp_gettime(struct lwp *l, const struct compat_30_netbsd32_ntp_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_ntptimevalp_t) ntvp;
	} */
	struct netbsd32_ntptimeval30 ntv32;
	struct ntptimeval ntv;
	int error = 0;

	if (SCARG_P32(uap, ntvp)) {
		ntp_gettime(&ntv);

		ntv32.time.tv_sec = ntv.time.tv_sec;
		ntv32.time.tv_usec = ntv.time.tv_nsec / 1000;
		ntv32.maxerror = (netbsd32_long)ntv.maxerror;
		ntv32.esterror = (netbsd32_long)ntv.esterror;
		error = copyout(&ntv32, SCARG_P32(uap, ntvp), sizeof(ntv32));
	}
	if (!error) {
		*retval = ntp_timestatus();
	}

	return (error);
}
#endif

int
netbsd32_ntp_adjtime(struct lwp *l, const struct netbsd32_ntp_adjtime_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_timexp_t) tp;
	} */
	struct netbsd32_timex ntv32;
	struct timex ntv;
	int error = 0;
	int modes;

	if ((error = copyin(SCARG_P32(uap, tp), &ntv32, sizeof(ntv32))))
		return (error);

	netbsd32_to_timex(&ntv32, &ntv);

	/*
	 * Update selected clock variables - only the superuser can
	 * change anything. Note that there is no error checking here on
	 * the assumption the superuser should know what it is doing.
	 */
	modes = ntv.modes;
	if (modes != 0 && (error = kauth_authorize_system(l->l_cred,
	    KAUTH_SYSTEM_TIME, KAUTH_REQ_SYSTEM_TIME_NTPADJTIME, NULL, NULL,
	    NULL)))
		return (error);

	ntp_adjtime1(&ntv);

	netbsd32_from_timex(&ntv, &ntv32);
	error = copyout(&ntv32, SCARG_P32(uap, tp), sizeof(ntv32));
	if (!error) {
		*retval = ntp_timestatus();
	}
	return error;
}
#else /* !NTP */
int
netbsd32_ntp_gettime(struct lwp *l, const struct netbsd32_ntp_gettime_args *uap, register_t *retval)
{

	return (ENOSYS);
}

#ifdef COMPAT_30
int
compat_30_netbsd32_ntp_gettime(struct lwp *l, const struct compat_30_netbsd32_ntp_gettime_args *uap, register_t *retval)
{

	return (ENOSYS);
}
#endif

int
netbsd32_ntp_adjtime(struct lwp *l, const struct netbsd32_ntp_adjtime_args *uap, register_t *retval)
{

	return (ENOSYS);
}
#endif /* NTP */

int
netbsd32_setitimer(struct lwp *l, const struct netbsd32_setitimer_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(const netbsd32_itimervalp_t) itv;
		syscallarg(netbsd32_itimervalp_t) oitv;
	} */
	struct proc *p = l->l_proc;
	struct netbsd32_itimerval s32it, *itv32;
	int which = SCARG(uap, which);
	struct netbsd32_getitimer_args getargs;
	struct itimerval aitv;
	int error;

	if ((u_int)which > ITIMER_PROF)
		return (EINVAL);
	itv32 = SCARG_P32(uap, itv);
	if (itv32) {
		if ((error = copyin(itv32, &s32it, sizeof(s32it))))
			return (error);
		netbsd32_to_itimerval(&s32it, &aitv);
	}
	if (SCARG_P32(uap, oitv) != 0) {
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
netbsd32_getitimer(struct lwp *l, const struct netbsd32_getitimer_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(netbsd32_itimervalp_t) itv;
	} */
	struct proc *p = l->l_proc;
	struct netbsd32_itimerval s32it;
	struct itimerval aitv;
	int error;

	error = dogetitimer(p, SCARG(uap, which), &aitv);
	if (error)
		return error;

	netbsd32_from_itimerval(&aitv, &s32it);
	return copyout(&s32it, SCARG_P32(uap, itv), sizeof(s32it));
}

int
netbsd32_gettimeofday(struct lwp *l, const struct netbsd32_gettimeofday_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_timevalp_t) tp;
		syscallarg(netbsd32_timezonep_t) tzp;
	} */
	struct timeval atv;
	struct netbsd32_timeval tv32;
	int error = 0;
	struct netbsd32_timezone tzfake;

	if (SCARG_P32(uap, tp)) {
		microtime(&atv);
		netbsd32_from_timeval(&atv, &tv32);
		error = copyout(&tv32, SCARG_P32(uap, tp), sizeof(tv32));
		if (error)
			return (error);
	}
	if (SCARG_P32(uap, tzp)) {
		/*
		 * NetBSD has no kernel notion of time zone, so we just
		 * fake up a timezone struct and return it if demanded.
		 */
		tzfake.tz_minuteswest = 0;
		tzfake.tz_dsttime = 0;
		error = copyout(&tzfake, SCARG_P32(uap, tzp), sizeof(tzfake));
	}
	return (error);
}

int
netbsd32_settimeofday(struct lwp *l, const struct netbsd32_settimeofday_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_timevalp_t) tv;
		syscallarg(const netbsd32_timezonep_t) tzp;
	} */
	struct netbsd32_timeval atv32;
	struct timeval atv;
	struct timespec ats;
	int error;
	struct proc *p = l->l_proc;

	/* Verify all parameters before changing time. */

	/*
	 * NetBSD has no kernel notion of time zone, and only an
	 * obsolete program would try to set it, so we log a warning.
	 */
	if (SCARG_P32(uap, tzp))
		printf("pid %d attempted to set the "
		    "(obsolete) kernel time zone\n", p->p_pid);

	if (SCARG_P32(uap, tv) == 0)
		return 0;

	if ((error = copyin(SCARG_P32(uap, tv), &atv32, sizeof(atv32))) != 0)
		return error;

	netbsd32_to_timeval(&atv32, &atv);
	TIMEVAL_TO_TIMESPEC(&atv, &ats);
	return settime(p, &ats);
}

int
netbsd32_adjtime(struct lwp *l, const struct netbsd32_adjtime_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_timevalp_t) delta;
		syscallarg(netbsd32_timevalp_t) olddelta;
	} */
	struct netbsd32_timeval atv;
	int error;

	if ((error = kauth_authorize_system(l->l_cred,
	    KAUTH_SYSTEM_TIME, KAUTH_REQ_SYSTEM_TIME_ADJTIME, NULL, NULL,
	    NULL)) != 0)
		return (error);

#ifdef __HAVE_TIMECOUNTER
	{
		extern int time_adjusted;     /* in kern_ntptime.c */
		extern int64_t time_adjtime;  /* in kern_ntptime.c */
		if (SCARG_P32(uap, olddelta)) {
			atv.tv_sec = time_adjtime / 1000000;
			atv.tv_usec = time_adjtime % 1000000;
			if (atv.tv_usec < 0) {
				atv.tv_usec += 1000000;
				atv.tv_sec--;
			}
			(void) copyout(&atv,
				       SCARG_P32(uap, olddelta), 
				       sizeof(atv));
			if (error)
				return (error);
		}
	
		if (SCARG_P32(uap, delta)) {
			error = copyin(SCARG_P32(uap, delta), &atv,
				       sizeof(struct timeval));
			if (error)
				return (error);

			time_adjtime = (int64_t)atv.tv_sec * 1000000 +
				atv.tv_usec;

			if (time_adjtime)
				/* We need to save the system time during shutdown */
				time_adjusted |= 1;
		}
	}
#else /* !__HAVE_TIMECOUNTER */
	{
		int32_t ndelta, ntickdelta, odelta;
		extern long bigadj, timedelta;
		extern int tickdelta;
		int s;
		error = copyin(SCARG_P32(uap, delta), &atv,
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

		if (SCARG_P32(uap, olddelta)) {
			atv.tv_sec = odelta / 1000000;
			atv.tv_usec = odelta % 1000000;
			(void) copyout(&atv,
				       SCARG_P32(uap, olddelta), sizeof(atv));
		}
	}
#endif /* !__HAVE_TIMECOUNTER */
	return (0);
}

int
netbsd32_clock_gettime(struct lwp *l, const struct netbsd32_clock_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(netbsd32_timespecp_t) tp;
	} */
	clockid_t clock_id;
	struct timespec ats;
	struct netbsd32_timespec ts32;

	clock_id = SCARG(uap, clock_id);
	if (clock_id != CLOCK_REALTIME)
		return (EINVAL);

	nanotime(&ats);
	netbsd32_from_timespec(&ats, &ts32);

	return copyout(&ts32, SCARG_P32(uap, tp), sizeof(ts32));
}

int
netbsd32_clock_settime(struct lwp *l, const struct netbsd32_clock_settime_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(const netbsd32_timespecp_t) tp;
	} */
	struct netbsd32_timespec ts32;
	clockid_t clock_id;
	struct timespec ats;
	int error;

	clock_id = SCARG(uap, clock_id);
	if (clock_id != CLOCK_REALTIME)
		return (EINVAL);

	if ((error = copyin(SCARG_P32(uap, tp), &ts32, sizeof(ts32))) != 0)
		return (error);

	netbsd32_to_timespec(&ts32, &ats);
	return settime(l->l_proc, &ats);
}

int
netbsd32_clock_getres(struct lwp *l, const struct netbsd32_clock_getres_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(netbsd32_timespecp_t) tp;
	} */
	struct netbsd32_timespec ts32;
	clockid_t clock_id;
	struct timespec ts;
	int error = 0;

	clock_id = SCARG(uap, clock_id);
	if (clock_id != CLOCK_REALTIME)
		return (EINVAL);

	if (SCARG_P32(uap, tp)) {
		ts.tv_sec = 0;
		ts.tv_nsec = 1000000000 / hz;

		netbsd32_from_timespec(&ts, &ts32);
		error = copyout(&ts, SCARG_P32(uap, tp), sizeof(ts));
	}

	return error;
}

int
netbsd32_nanosleep(struct lwp *l, const struct netbsd32_nanosleep_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_timespecp_t) rqtp;
		syscallarg(netbsd32_timespecp_t) rmtp;
	} */
	static int nanowait;
	struct netbsd32_timespec ts32;
	struct timespec rqt;
	struct timespec rmt;
	struct timeval atv, utv, ctime;
	int error, timo;

	error = copyin(SCARG_P32(uap, rqtp), &ts32, sizeof(ts32));
	if (error)
		return (error);

	netbsd32_to_timespec(&ts32, &rqt);
	TIMESPEC_TO_TIMEVAL(&atv,&rqt);
	if (itimerfix(&atv))
		return (EINVAL);

	getmicrotime(&ctime);
	timeradd(&atv,&ctime,&atv);
	timo = hzto(&atv);
	/*
	 * Avoid inadvertantly sleeping forever
	 */
	if (timo == 0)
		timo = 1;

	error = tsleep(&nanowait, PWAIT | PCATCH, "nanosleep", timo);
	if (error == ERESTART)
		error = EINTR;
	if (error == EWOULDBLOCK)
		error = 0;

	if (SCARG_P32(uap, rmtp)) {
		int error1;

		getmicrotime(&utv);

		timersub(&atv, &utv, &utv);
		if (utv.tv_sec < 0)
			timerclear(&utv);

		TIMEVAL_TO_TIMESPEC(&utv,&rmt);
		netbsd32_from_timespec(&rmt, &ts32);
		error1 = copyout(&ts32, SCARG_P32(uap,rmtp), sizeof(ts32));
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
netbsd32_timer_create(struct lwp *l, const struct netbsd32_timer_create_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(netbsd32_sigeventp_t) evp;
		syscallarg(netbsd32_timerp_t) timerid;
	} */

	return timer_create1(SCARG_P32(uap, timerid),
	    SCARG(uap, clock_id), SCARG_P32(uap, evp),
	    netbsd32_timer_create_fetch, l);
}

int
netbsd32_timer_delete(struct lwp *l, const struct netbsd32_timer_delete_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_timer_t) timerid;
	} */
	struct sys_timer_delete_args ua;

	NETBSD32TO64_UAP(timerid);
	return sys_timer_delete(l, (void *)&ua, retval);
}

int
netbsd32_timer_settime(struct lwp *l, const struct netbsd32_timer_settime_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_timer_t) timerid;
		syscallarg(int) flags;
		syscallarg(const netbsd32_itimerspecp_t) value;
		syscallarg(netbsd32_itimerspecp_t) ovalue;
	} */
	int error;
	struct itimerspec value, ovalue, *ovp = NULL;
	struct netbsd32_itimerspec its32;

	if ((error = copyin(SCARG_P32(uap, value), &its32, sizeof(its32))) != 0)
		return (error);
	netbsd32_to_timespec(&its32.it_interval, &value.it_interval);
	netbsd32_to_timespec(&its32.it_value, &value.it_value);

	if (SCARG_P32(uap, ovalue))
		ovp = &ovalue;

	if ((error = dotimer_settime(SCARG(uap, timerid), &value, ovp,
	    SCARG(uap, flags), l->l_proc)) != 0)
		return error;

	if (ovp) {
		netbsd32_from_timespec(&ovp->it_interval, &its32.it_interval);
		netbsd32_from_timespec(&ovp->it_value, &its32.it_value);
		return copyout(&its32, SCARG_P32(uap, ovalue), sizeof(its32));
	}
	return 0;
}

int
netbsd32_timer_gettime(struct lwp *l, const struct netbsd32_timer_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_timer_t) timerid;
		syscallarg(netbsd32_itimerspecp_t) value;
	} */
	int error;
	struct itimerspec its;
	struct netbsd32_itimerspec its32;

	if ((error = dotimer_gettime(SCARG(uap, timerid), l->l_proc,
	    &its)) != 0)
		return error;

	netbsd32_from_timespec(&its.it_interval, &its32.it_interval);
	netbsd32_from_timespec(&its.it_value, &its32.it_value);

	return copyout(&its32, SCARG_P32(uap, value), sizeof(its32));
}

int
netbsd32_timer_getoverrun(struct lwp *l, const struct netbsd32_timer_getoverrun_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_timer_t) timerid;
	} */
	struct sys_timer_getoverrun_args ua;

	NETBSD32TO64_UAP(timerid);
	return sys_timer_getoverrun(l, (void *)&ua, retval);
}
