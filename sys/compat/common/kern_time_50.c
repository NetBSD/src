/*	$NetBSD: kern_time_50.c,v 1.19.4.1 2012/04/17 00:07:14 yamt Exp $	*/

/*-
 * Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__KERNEL_RCSID(0, "$NetBSD: kern_time_50.c,v 1.19.4.1 2012/04/17 00:07:14 yamt Exp $");

#ifdef _KERNEL_OPT
#include "opt_aio.h"
#include "opt_ntp.h"
#include "opt_mqueue.h"
#endif

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/dirent.h>
#include <sys/malloc.h>
#include <sys/kauth.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/clockctl.h>
#include <sys/aio.h>
#include <sys/poll.h>
#include <sys/syscallargs.h>
#include <sys/resource.h>

#include <compat/common/compat_util.h>
#include <compat/sys/time.h>
#include <compat/sys/timex.h>
#include <compat/sys/resource.h>
#include <compat/sys/clockctl.h>

int
compat_50_sys_clock_gettime(struct lwp *l,
    const struct compat_50_sys_clock_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) clock_id;
		syscallarg(struct timespec50 *) tp;
	} */
	int error;
	struct timespec ats;
	struct timespec50 ats50;

	error = clock_gettime1(SCARG(uap, clock_id), &ats);
	if (error != 0)
		return error;

	timespec_to_timespec50(&ats, &ats50);

	return copyout(&ats50, SCARG(uap, tp), sizeof(ats50));
}

/* ARGSUSED */
int
compat_50_sys_clock_settime(struct lwp *l,
    const struct compat_50_sys_clock_settime_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) clock_id;
		syscallarg(const struct timespec50 *) tp;
	} */
	int error;
	struct timespec ats;
	struct timespec50 ats50;

	error = copyin(SCARG(uap, tp), &ats50, sizeof(ats50));
	if (error)
		return error;
	timespec50_to_timespec(&ats50, &ats);

	return clock_settime1(l->l_proc, SCARG(uap, clock_id), &ats,
	    true);
}


int
compat_50_sys_clock_getres(struct lwp *l,
    const struct compat_50_sys_clock_getres_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) clock_id;
		syscallarg(struct timespec50 *) tp;
	} */
	struct timespec50 ats50;
	struct timespec ats;
	int error = 0;

	error = clock_getres1(SCARG(uap, clock_id), &ats);
	if (error != 0)
		return error;

	if (SCARG(uap, tp)) {
		timespec_to_timespec50(&ats, &ats50);
		error = copyout(&ats50, SCARG(uap, tp), sizeof(ats50));
	}

	return error;
}

/* ARGSUSED */
int
compat_50_sys_nanosleep(struct lwp *l,
    const struct compat_50_sys_nanosleep_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct timespec50 *) rqtp;
		syscallarg(struct timespec50 *) rmtp;
	} */
	struct timespec rmt, rqt;
	struct timespec50 rmt50, rqt50;
	int error, error1;

	error = copyin(SCARG(uap, rqtp), &rqt50, sizeof(rqt50));
	if (error)
		return error;
	timespec50_to_timespec(&rqt50, &rqt);

	error = nanosleep1(l, &rqt, SCARG(uap, rmtp) ? &rmt : NULL);
	if (SCARG(uap, rmtp) == NULL || (error != 0 && error != EINTR))
		return error;

	timespec_to_timespec50(&rmt, &rmt50);
	error1 = copyout(&rmt50, SCARG(uap, rmtp), sizeof(*SCARG(uap, rmtp)));
	return error1 ? error1 : error;
}

/* ARGSUSED */
int
compat_50_sys_gettimeofday(struct lwp *l,
    const struct compat_50_sys_gettimeofday_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct timeval50 *) tp;
		syscallarg(void *) tzp;		really "struct timezone *";
	} */
	struct timeval atv;
	struct timeval50 atv50;
	int error = 0;
	struct timezone tzfake;

	if (SCARG(uap, tp)) {
		microtime(&atv);
		timeval_to_timeval50(&atv, &atv50);
		error = copyout(&atv50, SCARG(uap, tp), sizeof(*SCARG(uap, tp)));
		if (error)
			return error;
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
	return error;
}

/* ARGSUSED */
int
compat_50_sys_settimeofday(struct lwp *l,
    const struct compat_50_sys_settimeofday_args *uap, register_t *retval)
{
	/* {
		syscallarg(const struct timeval50 *) tv;
		syscallarg(const void *) tzp; really "const struct timezone *";
	} */
	struct timeval50 atv50;
	struct timeval atv;
	int error = copyin(SCARG(uap, tv), &atv50, sizeof(atv50));
	if (error)
		return error;
	timeval50_to_timeval(&atv50, &atv);
	return settimeofday1(&atv, false, SCARG(uap, tzp), l, true);
}

/* ARGSUSED */
int
compat_50_sys_adjtime(struct lwp *l,
    const struct compat_50_sys_adjtime_args *uap, register_t *retval)
{
	/* {
		syscallarg(const struct timeval50 *) delta;
		syscallarg(struct timeval50 *) olddelta;
	} */
	int error;
	struct timeval50 delta50, olddelta50;
	struct timeval delta, olddelta;

	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_TIME,
	    KAUTH_REQ_SYSTEM_TIME_ADJTIME, NULL, NULL, NULL)) != 0)
		return error;

	if (SCARG(uap, delta)) {
		error = copyin(SCARG(uap, delta), &delta50,
		    sizeof(*SCARG(uap, delta)));
		if (error)
			return (error);
		timeval50_to_timeval(&delta50, &delta);
	}
	adjtime1(SCARG(uap, delta) ? &delta : NULL,
	    SCARG(uap, olddelta) ? &olddelta : NULL, l->l_proc);
	if (SCARG(uap, olddelta)) {
		timeval_to_timeval50(&olddelta, &olddelta50);
		error = copyout(&olddelta50, SCARG(uap, olddelta),
		    sizeof(*SCARG(uap, olddelta)));
	}
	return error;
}

/* BSD routine to set/arm an interval timer. */
/* ARGSUSED */
int
compat_50_sys_getitimer(struct lwp *l,
    const struct compat_50_sys_getitimer_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(struct itimerval50 *) itv;
	} */
	struct proc *p = l->l_proc;
	struct itimerval aitv;
	struct itimerval50 aitv50;
	int error;

	error = dogetitimer(p, SCARG(uap, which), &aitv);
	if (error)
		return error;
	itimerval_to_itimerval50(&aitv, &aitv50);
	return copyout(&aitv50, SCARG(uap, itv), sizeof(*SCARG(uap, itv)));
}

int
compat_50_sys_setitimer(struct lwp *l,
    const struct compat_50_sys_setitimer_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(const struct itimerval50 *) itv;
		syscallarg(struct itimerval50 *) oitv;
	} */
	struct proc *p = l->l_proc;
	int which = SCARG(uap, which);
	struct compat_50_sys_getitimer_args getargs;
	const struct itimerval50 *itvp;
	struct itimerval50 aitv50;
	struct itimerval aitv;
	int error;

	if ((u_int)which > ITIMER_PROF)
		return (EINVAL);
	itvp = SCARG(uap, itv);
	if (itvp &&
	    (error = copyin(itvp, &aitv50, sizeof(aitv50)) != 0))
		return (error);
	itimerval50_to_itimerval(&aitv50, &aitv);
	if (SCARG(uap, oitv) != NULL) {
		SCARG(&getargs, which) = which;
		SCARG(&getargs, itv) = SCARG(uap, oitv);
		if ((error = compat_50_sys_getitimer(l, &getargs, retval)) != 0)
			return (error);
	}
	if (itvp == 0)
		return (0);

	return dosetitimer(p, which, &aitv);
}

int
compat_50_sys_aio_suspend(struct lwp *l,
    const struct compat_50_sys_aio_suspend_args *uap, register_t *retval)
{
	/* {
		syscallarg(const struct aiocb *const[]) list;
		syscallarg(int) nent;
		syscallarg(const struct timespec50 *) timeout;
	} */
#ifdef AIO
	struct aiocb **list;
	struct timespec ts;
	struct timespec50 ts50;
	int error, nent;

	nent = SCARG(uap, nent);
	if (nent <= 0 || nent > aio_listio_max)
		return EAGAIN;

	if (SCARG(uap, timeout)) {
		/* Convert timespec to ticks */
		error = copyin(SCARG(uap, timeout), &ts50,
		    sizeof(*SCARG(uap, timeout)));
		if (error)
			return error;
		timespec50_to_timespec(&ts50, &ts);
	}
	list = kmem_alloc(nent * sizeof(*list), KM_SLEEP);
	error = copyin(SCARG(uap, list), list, nent * sizeof(*list));
	if (error)
		goto out;
	error = aio_suspend1(l, list, nent, SCARG(uap, timeout) ? &ts : NULL);
out:
	kmem_free(list, nent * sizeof(*list));
	return error;
#else
	return ENOSYS;
#endif
}

int
compat_50_sys__lwp_park(struct lwp *l,
    const struct compat_50_sys__lwp_park_args *uap, register_t *retval)
{
	/* {
		syscallarg(const struct timespec50 *)	ts;
		syscallarg(lwpid_t)			unpark;
		syscallarg(const void *)		hint;
		syscallarg(const void *)		unparkhint;
	} */
	struct timespec ts, *tsp;
	struct timespec50 ts50;
	int error;

	if (SCARG(uap, ts) == NULL)
		tsp = NULL;
	else {
		error = copyin(SCARG(uap, ts), &ts50, sizeof(ts50));
		if (error != 0)
			return error;
		timespec50_to_timespec(&ts50, &ts);
		tsp = &ts;
	}

	if (SCARG(uap, unpark) != 0) {
		error = lwp_unpark(SCARG(uap, unpark), SCARG(uap, unparkhint));
		if (error != 0)
			return error;
	}

	return lwp_park(tsp, SCARG(uap, hint));
}

int
compat_50_sys_mq_timedsend(struct lwp *l,
    const struct compat_50_sys_mq_timedsend_args *uap, register_t *retval)
{
	/* {
		syscallarg(mqd_t) mqdes;
		syscallarg(const char *) msg_ptr;
		syscallarg(size_t) msg_len;
		syscallarg(unsigned) msg_prio;
		syscallarg(const struct timespec50 *) abs_timeout;
	} */
#ifdef MQUEUE
	struct timespec50 ts50;
	struct timespec ts, *tsp;
	int error;

	/* Get and convert time value */
	if (SCARG(uap, abs_timeout)) {
		error = copyin(SCARG(uap, abs_timeout), &ts50, sizeof(ts50));
		if (error)
			return error;
		timespec50_to_timespec(&ts50, &ts);
		tsp = &ts;
	} else {
		tsp = NULL;
	}

	return mq_send1(SCARG(uap, mqdes), SCARG(uap, msg_ptr),
	    SCARG(uap, msg_len), SCARG(uap, msg_prio), tsp);
#else
	return ENOSYS;
#endif
}

int
compat_50_sys_mq_timedreceive(struct lwp *l,
    const struct compat_50_sys_mq_timedreceive_args *uap, register_t *retval)
{
	/* {
		syscallarg(mqd_t) mqdes;
		syscallarg(char *) msg_ptr;
		syscallarg(size_t) msg_len;
		syscallarg(unsigned *) msg_prio;
		syscallarg(const struct timespec50 *) abs_timeout;
	} */
#ifdef MQUEUE
	struct timespec ts, *tsp;
	struct timespec50 ts50;
	ssize_t mlen;
	int error;

	/* Get and convert time value */
	if (SCARG(uap, abs_timeout)) {
		error = copyin(SCARG(uap, abs_timeout), &ts50, sizeof(ts50));
		if (error)
			return error;

		timespec50_to_timespec(&ts50, &ts);
		tsp = &ts;
	} else {
		tsp = NULL;
	}

	error = mq_recv1(SCARG(uap, mqdes), SCARG(uap, msg_ptr),
	    SCARG(uap, msg_len), SCARG(uap, msg_prio), tsp, &mlen);
	if (error == 0)
		*retval = mlen;

	return error;
#else
	return ENOSYS;
#endif
}

static int
tscopyin(const void *u, void *s, size_t len)
{
	struct timespec50 ts50;
	int error;

	KASSERT(len == sizeof(struct timespec));
	error = copyin(u, &ts50, sizeof(ts50));
	if (error)
		return error;
	timespec50_to_timespec(&ts50, s);
	return 0;
}

static int
tscopyout(const void *s, void *u, size_t len)
{
	struct timespec50 ts50;

	KASSERT(len == sizeof(struct timespec));
	timespec_to_timespec50(s, &ts50);
	return copyout(&ts50, u, sizeof(ts50));
}

int
compat_50_sys___sigtimedwait(struct lwp *l,
    const struct compat_50_sys___sigtimedwait_args *uap, register_t *retval)
{
	int res;

	res = sigtimedwait1(l,
	    (const struct sys_____sigtimedwait50_args *)uap, retval, copyin,
	    copyout, tscopyin, tscopyout);
	if (!res)
		*retval = 0; /* XXX NetBSD<=5 was not POSIX compliant */
	return res;
}

void
rusage_to_rusage50(const struct rusage *ru, struct rusage50 *ru50)
{
	(void)memcpy(&ru50->ru_first, &ru->ru_first,
	    (char *)&ru50->ru_last - (char *)&ru50->ru_first +
	    sizeof(ru50->ru_last));
	ru50->ru_maxrss = ru->ru_maxrss;
	timeval_to_timeval50(&ru->ru_utime, &ru50->ru_utime);
	timeval_to_timeval50(&ru->ru_stime, &ru50->ru_stime);
}

int
compat_50_sys_getrusage(struct lwp *l,
    const struct compat_50_sys_getrusage_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) who;
		syscallarg(struct rusage50 *) rusage;
	} */
	struct rusage ru;
	struct rusage50 ru50;
	struct proc *p = l->l_proc;

	switch (SCARG(uap, who)) {
	case RUSAGE_SELF:
		mutex_enter(p->p_lock);
		memcpy(&ru, &p->p_stats->p_ru, sizeof(ru));
		calcru(p, &ru.ru_utime, &ru.ru_stime, NULL, NULL);
		mutex_exit(p->p_lock);
		break;

	case RUSAGE_CHILDREN:
		mutex_enter(p->p_lock);
		memcpy(&ru, &p->p_stats->p_cru, sizeof(ru));
		mutex_exit(p->p_lock);
		break;

	default:
		return EINVAL;
	}
	rusage_to_rusage50(&ru, &ru50);
	return copyout(&ru50, SCARG(uap, rusage), sizeof(ru50));
}


/* Return the time remaining until a POSIX timer fires. */
int
compat_50_sys_timer_gettime(struct lwp *l,
    const struct compat_50_sys_timer_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(timer_t) timerid;
		syscallarg(struct itimerspec50 *) value;
	} */
	struct itimerspec its;
	struct itimerspec50 its50;
	int error;

	if ((error = dotimer_gettime(SCARG(uap, timerid), l->l_proc,
	    &its)) != 0)
		return error;
	itimerspec_to_itimerspec50(&its, &its50);

	return copyout(&its50, SCARG(uap, value), sizeof(its50));
}

/* Set and arm a POSIX realtime timer */
int
compat_50_sys_timer_settime(struct lwp *l,
    const struct compat_50_sys_timer_settime_args *uap, register_t *retval)
{
	/* {
		syscallarg(timer_t) timerid;
		syscallarg(int) flags;
		syscallarg(const struct itimerspec50 *) value;
		syscallarg(struct itimerspec50 *) ovalue;
	} */
	int error;
	struct itimerspec value, ovalue, *ovp = NULL;
	struct itimerspec50 value50, ovalue50;

	if ((error = copyin(SCARG(uap, value), &value50, sizeof(value50))) != 0)
		return error;

	itimerspec50_to_itimerspec(&value50, &value);
	if (SCARG(uap, ovalue))
		ovp = &ovalue;

	if ((error = dotimer_settime(SCARG(uap, timerid), &value, ovp,
	    SCARG(uap, flags), l->l_proc)) != 0)
		return error;

	if (ovp) {
		itimerspec_to_itimerspec50(&ovalue, &ovalue50);
		return copyout(&ovalue50, SCARG(uap, ovalue), sizeof(ovalue50));
	}
	return 0;
}

/*
 * ntp_gettime() - NTP user application interface
 */
int
compat_50_sys___ntp_gettime30(struct lwp *l,
    const struct compat_50_sys___ntp_gettime30_args *uap, register_t *retval)
{
#ifdef NTP
	/* {
		syscallarg(struct ntptimeval *) ntvp;
	} */
	struct ntptimeval ntv;
	struct ntptimeval50 ntv50;
	int error;

	if (SCARG(uap, ntvp)) {
		ntp_gettime(&ntv);
		timespec_to_timespec50(&ntv.time, &ntv50.time);
		ntv50.maxerror = ntv.maxerror;
		ntv50.esterror = ntv.esterror;
		ntv50.tai = ntv.tai;
		ntv50.time_state = ntv.time_state;

		error = copyout(&ntv50, SCARG(uap, ntvp), sizeof(ntv50));
		if (error)
			return error;
	}
	*retval = ntp_timestatus();
	return 0;
#else
	return ENOSYS;
#endif
}
int
compat50_clockctlioctl(dev_t dev, u_long cmd, void *data, int flags,
    struct lwp *l)
{
	int error = 0;
	const struct cdevsw *cd = cdevsw_lookup(dev);

	if (cd == NULL || cd->d_ioctl == NULL)
		return ENXIO;

	switch (cmd) {
	case CLOCKCTL_OSETTIMEOFDAY: {
		struct timeval50 tv50;
		struct timeval tv;
		struct clockctl50_settimeofday *args = data;

		error = copyin(args->tv, &tv50, sizeof(tv50));
		if (error)
			return (error);
		timeval50_to_timeval(&tv50, &tv);
		error = settimeofday1(&tv, false, args->tzp, l, false);
		break;
	}
	case CLOCKCTL_OADJTIME: {
		struct timeval atv, oldatv;
		struct timeval50 atv50;
		struct clockctl50_adjtime *args = data;

		if (args->delta) {
			error = copyin(args->delta, &atv50, sizeof(atv50));
			if (error)
				return (error);
			timeval50_to_timeval(&atv50, &atv);
		}
		adjtime1(args->delta ? &atv : NULL,
		    args->olddelta ? &oldatv : NULL, l->l_proc);
		if (args->olddelta) {
			timeval_to_timeval50(&oldatv, &atv50);
			error = copyout(&atv50, args->olddelta, sizeof(atv50));
		}
		break;
	}
	case CLOCKCTL_OCLOCK_SETTIME: {
		struct timespec50 tp50;
		struct timespec tp;
		struct clockctl50_clock_settime *args = data;

		error = copyin(args->tp, &tp50, sizeof(tp50));
		if (error)
			return (error);
		timespec50_to_timespec(&tp50, &tp);
		error = clock_settime1(l->l_proc, args->clock_id, &tp, true);
		break;
	}
	case CLOCKCTL_ONTP_ADJTIME:
		/* The ioctl number changed but the data did not change. */
		error = (cd->d_ioctl)(dev, CLOCKCTL_NTP_ADJTIME,
		    data, flags, l);
		break;
	default:
		error = EINVAL;
	}

	return (error);
}
int
compat_50_sys_wait4(struct lwp *l, const struct compat_50_sys_wait4_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int)			pid;
		syscallarg(int *)		status;
		syscallarg(int)			options;
		syscallarg(struct rusage50 *)	rusage;
	} */
	int status, error, pid = SCARG(uap, pid);
	struct rusage50 ru50;
	struct rusage ru;

	error = do_sys_wait(&pid, &status, SCARG(uap, options),
	    SCARG(uap, rusage) != NULL ? &ru : NULL);

	retval[0] = pid;
	if (pid == 0)
		return error;

	if (SCARG(uap, rusage)) {
		rusage_to_rusage50(&ru, &ru50);
		error = copyout(&ru50, SCARG(uap, rusage), sizeof(ru50));
	}

	if (error == 0 && SCARG(uap, status))
		error = copyout(&status, SCARG(uap, status), sizeof(status));

	return error;
}
