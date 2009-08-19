/*	$NetBSD: linux32_time.c,v 1.19.2.2 2009/08/19 18:46:59 yamt Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: linux32_time.c,v 1.19.2.2 2009/08/19 18:46:59 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/fstypes.h>
#include <sys/signal.h>
#include <sys/dirent.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/namei.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/ucred.h>
#include <sys/swap.h>
#include <sys/vfs_syscalls.h>
#include <sys/timetc.h>

#include <machine/types.h>

#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_oldolduname.h>
#include <compat/linux/common/linux_sched.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_machdep.h>
#include <compat/linux32/common/linux32_sysctl.h>
#include <compat/linux32/common/linux32_socketcall.h>
#include <compat/linux32/linux32_syscallargs.h>

extern struct timezone linux_sys_tz;

static __inline void
native_to_linux32_timespec(struct linux32_timespec *, struct timespec *);
static __inline void
linux32_to_native_timespec(struct timespec *, struct linux32_timespec *);

int
linux32_sys_gettimeofday(struct lwp *l, const struct linux32_sys_gettimeofday_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_timeval50p_t) tp;
		syscallarg(netbsd32_timezonep_t) tzp;
	} */
	struct timeval tv;
	struct netbsd32_timeval50 tv32;
	int error;

	if (SCARG_P32(uap, tp) != NULL) {
		microtime(&tv);
		netbsd32_from_timeval50(&tv, &tv32);
		if ((error = copyout(&tv32, SCARG_P32(uap, tp), 
		    sizeof(tv32))) != 0)
			return error;
	}

	/* timezone size does not change */
	if (SCARG_P32(uap, tzp) != NULL) {
		if ((error = copyout(&linux_sys_tz, SCARG_P32(uap, tzp), 
		    sizeof(linux_sys_tz))) != 0)
			return error;
	}

	return 0;
}

int
linux32_sys_settimeofday(struct lwp *l, const struct linux32_sys_settimeofday_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_timeval50p_t) tp;
		syscallarg(netbsd32_timezonep_t) tzp;
	} */
	struct linux_sys_settimeofday_args ua;

	NETBSD32TOP_UAP(tp, struct timeval50);
	NETBSD32TOP_UAP(tzp, struct timezone);

	return linux_sys_settimeofday(l, &ua, retval);
}

int
linux32_sys_time(struct lwp *l, const struct linux32_sys_time_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_timep_t) t;
	} */
        struct timeval atv;
        linux32_time_t tt;
        int error;
 
        microtime(&atv);

        tt = (linux32_time_t)atv.tv_sec;

        if (SCARG_P32(uap, t) && (error = copyout(&tt, 
	    SCARG_P32(uap, t), sizeof(tt))))
                return error;

        retval[0] = tt;

        return 0;
}


#define	CONVTCK(r)	(r.tv_sec * hz + r.tv_usec / (1000000 / hz))

int
linux32_sys_times(struct lwp *l, const struct linux32_sys_times_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_tmsp_t) tms;
	} */
	struct proc *p = l->l_proc;
	struct timeval t;
	int error;

	if (SCARG_P32(uap, tms)) {
		struct linux32_tms ltms32;
		struct rusage ru;

		mutex_enter(p->p_lock);
		calcru(p, &ru.ru_utime, &ru.ru_stime, NULL, NULL);
		ltms32.ltms32_utime = CONVTCK(ru.ru_utime);
		ltms32.ltms32_stime = CONVTCK(ru.ru_stime);
		ltms32.ltms32_cutime = CONVTCK(p->p_stats->p_cru.ru_utime);
		ltms32.ltms32_cstime = CONVTCK(p->p_stats->p_cru.ru_stime);
		mutex_exit(p->p_lock);

		error = copyout(&ltms32, SCARG_P32(uap, tms), sizeof(ltms32));
		if (error)
			return error;
	}

	getmicrouptime(&t);

	retval[0] = ((linux32_clock_t)(CONVTCK(t)));
	return 0;
}

#undef CONVTCK

int
linux32_sys_stime(struct lwp *l, const struct linux32_sys_stime_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_timep_t) t;
	} */
	struct timespec ts;
	linux32_time_t tt32;
	int error;
	
	if ((error = copyin(SCARG_P32(uap, t), &tt32, sizeof tt32)) != 0)
		return error;

	ts.tv_sec = (long)tt32;
	ts.tv_nsec = 0;

	return settime(l->l_proc, &ts);
}

int
linux32_sys_utime(struct lwp *l, const struct linux32_sys_utime_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(linux32_utimbufp_t) times;
	} */
        struct timeval tv[2], *tvp;
        struct linux32_utimbuf lut;
        int error;

        if (SCARG_P32(uap, times) != NULL) {
                if ((error = copyin(SCARG_P32(uap, times), &lut, sizeof lut)))
                        return error;

                tv[0].tv_sec = (long)lut.l_actime;
                tv[0].tv_usec = 0;
                tv[1].tv_sec = (long)lut.l_modtime;
		tv[1].tv_usec = 0;
                tvp = tv;
        } else {
		tvp = NULL;
	}
                     
        return do_sys_utimes(l, NULL, SCARG_P32(uap, path), FOLLOW,
			    tvp, UIO_SYSSPACE);
}

static __inline void
native_to_linux32_timespec(struct linux32_timespec *ltp, struct timespec *ntp)
{
	ltp->tv_sec = ntp->tv_sec;
	ltp->tv_nsec = ntp->tv_nsec;
}

static __inline void
linux32_to_native_timespec(struct timespec *ntp, struct linux32_timespec *ltp)
{
	ntp->tv_sec = ltp->tv_sec;
	ntp->tv_nsec = ltp->tv_nsec;
}

int
linux32_sys_nanosleep(struct lwp *l,
    const struct linux32_sys_nanosleep_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_timespecp_t) rqtp;
		syscallarg(linux32_timespecp_t) rmtp;
	} */
	struct timespec rqts, rmts;
	struct linux32_timespec lrqts, lrmts;
	int error, error1;

	error = copyin(SCARG_P32(uap, rqtp), &lrqts, sizeof(lrqts));
	if (error != 0)
		return error;
	linux32_to_native_timespec(&rqts, &lrqts);

	error = nanosleep1(l, &rqts, SCARG_P32(uap, rmtp) ? &rmts : NULL);
	if (SCARG_P32(uap, rmtp) == NULL || (error != 0 && error != EINTR))
		return error;

	native_to_linux32_timespec(&lrmts, &rmts);
	error1 = copyout(&lrmts, SCARG_P32(uap, rmtp), sizeof(lrmts));
	return error1 ? error1 : error;
}

int
linux32_sys_clock_settime(struct lwp *l,
    const struct linux32_sys_clock_settime_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) which;
		syscallarg(linux32_timespecp_t) tp;
	} */
	int error;
	struct timespec ts;
	struct linux32_timespec lts;

	switch (SCARG(uap, which)) {
	case LINUX_CLOCK_REALTIME:
		break;
	default:
		return EINVAL;
	}

	if ((error = copyin(SCARG_P32(uap, tp), &lts, sizeof lts)))
		return error;

	linux32_to_native_timespec(&ts, &lts);
	return settime(l->l_proc, &ts);
}

int
linux32_sys_clock_gettime(struct lwp *l,
    const struct linux32_sys_clock_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) which;
		syscallarg(linux32_timespecp_t) tp;
	} */
	struct timespec ts;
	struct linux32_timespec lts;

	switch (SCARG(uap, which)) {
	case LINUX_CLOCK_REALTIME:
		nanotime(&ts);
		break;
	case LINUX_CLOCK_MONOTONIC:
		nanouptime(&ts);
		break;
	default:
		return EINVAL;
	}

	native_to_linux32_timespec(&lts, &ts);
	return copyout(&lts, SCARG_P32(uap, tp), sizeof lts);
}

int
linux32_sys_clock_getres(struct lwp *l,
    const struct linux32_sys_clock_getres_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) which;
		syscallarg(linux32_timespecp_t) tp;
	} */
	int error;
	clockid_t id;
	struct timespec ts;
	struct linux32_timespec lts;

	error = linux_to_native_clockid(&id, SCARG(uap, which));
	if (error != 0 || SCARG_P32(uap, tp) == NULL)
		return error;

	ts.tv_sec = 0;
	ts.tv_nsec = 1000000000 / tc_getfrequency();
	native_to_linux32_timespec(&lts, &ts);
	return copyout(&lts, SCARG_P32(uap, tp), sizeof lts);
}

int
linux32_sys_clock_nanosleep(struct lwp *l, 
    const struct linux32_sys_clock_nanosleep_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) which;
		syscallarg(int) flags;
		syscallarg(linux32_timespecp_t) rqtp;
		syscallarg(linux32_timespecp_t) rmtp;
	} */
	struct linux32_timespec lrqts, lrmts;
	struct timespec rqts, rmts;
	int error, error1;

	if (SCARG(uap, flags) != 0)
		return EINVAL;          /* XXX deal with TIMER_ABSTIME */
	if (SCARG(uap, which) != LINUX_CLOCK_REALTIME)
		return EINVAL;

	error = copyin(SCARG_P32(uap, rqtp), &lrqts, sizeof lrqts);
	if (error != 0)
		return error;
	linux32_to_native_timespec(&rqts, &lrqts);

	error = nanosleep1(l, &rqts, SCARG_P32(uap, rmtp) ? &rmts : 0);
	if (SCARG_P32(uap, rmtp) == NULL || (error != 0 && error != EINTR))
		return error;

	native_to_linux32_timespec(&lrmts, &rmts);
	error1 = copyout(&lrmts, SCARG_P32(uap, rmtp), sizeof lrmts);
	return error1 ? error1 : error;
}
