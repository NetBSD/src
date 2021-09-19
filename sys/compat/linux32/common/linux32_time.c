/*	$NetBSD: linux32_time.c,v 1.40 2021/09/19 23:51:37 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: linux32_time.c,v 1.40 2021/09/19 23:51:37 thorpej Exp $");

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
#include <sys/timerfd.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/ucred.h>
#include <sys/swap.h>
#include <sys/vfs_syscalls.h>

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
#include <compat/linux32/common/linux32_sched.h>
#include <compat/linux32/linux32_syscallargs.h>

CTASSERT(LINUX_TIMER_ABSTIME == TIMER_ABSTIME);

extern struct timezone linux_sys_tz;

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

		memset(&ltms32, 0, sizeof(ltms32));

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

void
native_to_linux32_timespec(struct linux32_timespec *ltp,
    const struct timespec *ntp)
{

	memset(ltp, 0, sizeof(*ltp));
	ltp->tv_sec = ntp->tv_sec;
	ltp->tv_nsec = ntp->tv_nsec;
}

void
linux32_to_native_timespec(struct timespec *ntp,
    const struct linux32_timespec *ltp)
{

	memset(ntp, 0, sizeof(*ntp));
	ntp->tv_sec = ltp->tv_sec;
	ntp->tv_nsec = ltp->tv_nsec;
}

void
native_to_linux32_itimerspec(struct linux32_itimerspec *litp,
    const struct itimerspec *nitp)
{
	memset(litp, 0, sizeof(*litp));
	native_to_linux32_timespec(&litp->it_interval, &nitp->it_interval);
	native_to_linux32_timespec(&litp->it_value, &nitp->it_value);
}

void
linux32_to_native_itimerspec(struct itimerspec *nitp,
    const struct linux32_itimerspec *litp)
{
	memset(nitp, 0, sizeof(*nitp));
	linux32_to_native_timespec(&nitp->it_interval, &litp->it_interval);
	linux32_to_native_timespec(&nitp->it_value, &litp->it_value);
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

	error = nanosleep1(l, CLOCK_MONOTONIC, 0, &rqts,
	    SCARG_P32(uap, rmtp) ? &rmts : NULL);
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
	clockid_t id;

	error = linux_to_native_clockid(&id, SCARG(uap, which));
	if (error != 0)
		return error;

	if ((error = copyin(SCARG_P32(uap, tp), &lts, sizeof lts)))
		return error;

	linux32_to_native_timespec(&ts, &lts);
	return clock_settime1(l->l_proc, id, &ts, true);
}

int
linux32_sys_clock_gettime(struct lwp *l,
    const struct linux32_sys_clock_gettime_args *uap, register_t *retval)
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
	if (error != 0)
		return error;

	error = clock_gettime1(id, &ts);
	if (error != 0)
		return error;

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

	error = clock_getres1(id, &ts);
	if (error != 0)
		return error;

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
	int error, error1, flags;
	clockid_t id;

	flags = SCARG(uap, flags) != 0 ? TIMER_ABSTIME : 0;

	error = linux_to_native_clockid(&id, SCARG(uap, which));
	if (error != 0)
		return error;

	error = copyin(SCARG_P32(uap, rqtp), &lrqts, sizeof lrqts);
	if (error != 0)
		return error;
	linux32_to_native_timespec(&rqts, &lrqts);

	error = nanosleep1(l, id, flags, &rqts,
	    SCARG_P32(uap, rmtp) ? &rmts : NULL);
	if (SCARG_P32(uap, rmtp) == NULL || (error != 0 && error != EINTR))
		return error;

	native_to_linux32_timespec(&lrmts, &rmts);
	error1 = copyout(&lrmts, SCARG_P32(uap, rmtp), sizeof lrmts);
	return error1 ? error1 : error;
}

int
linux32_sys_timer_create(struct lwp *l,
    const struct linux32_sys_timer_create_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) clockid;
		syscallarg(struct linux32_sigevent *) evp;
		syscallarg(timer_t *) timerid;
	} */
	clockid_t id;
	int error;

	error = linux_to_native_timer_create_clockid(&id, SCARG(uap, clockid));
	if (error == 0) {
		error = timer_create1(SCARG(uap, timerid), id,
		    (void *)SCARG(uap, evp), linux32_sigevent_copyin, l);
	}

	return error;
}

int
linux32_sys_timer_settime(struct lwp *l,
    const struct linux32_sys_timer_settime_args *uap, register_t *retval)
{
	/* {
		syscallarg(timer_t) timerid;
		syscallarg(int) flags;
		syscallarg(const struct linux32_itimerspec *) tim;
		syscallarg(struct linux32_itimerspec *) otim;
	} */
	struct itimerspec value, ovalue, *ovp = NULL;
	struct linux32_itimerspec tim, otim;
	int error;

	error = copyin(SCARG(uap, tim), &tim, sizeof(tim));
	if (error) {
		return error;
	}
	linux32_to_native_itimerspec(&value, &tim);

	if (SCARG(uap, otim)) {
		ovp = &ovalue;
	}

	if (SCARG(uap, flags) & ~TIMER_ABSTIME) {
		return EINVAL;
	}

	error = dotimer_settime(SCARG(uap, timerid), &value, ovp,
	    SCARG(uap, flags), l->l_proc);
	if (error) {
		return error;
	}

	if (ovp) {
		native_to_linux32_itimerspec(&otim, ovp);
		error = copyout(&otim, SCARG(uap, otim), sizeof(otim));
	}

	return error;
}

int
linux32_sys_timer_gettime(struct lwp *l,
    const struct linux32_sys_timer_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(timer_t) timerid;
		syscallarg(struct linux32_itimerspec *) tim;
	} */
	struct itimerspec its;
	struct linux32_itimerspec lits;
	int error;

	error = dotimer_gettime(SCARG(uap, timerid), l->l_proc, &its);
	if (error == 0) {
		native_to_linux32_itimerspec(&lits, &its);
		error = copyout(&lits, SCARG(uap, tim), sizeof(lits));
	}

	return error;
}

/*
 * timer_gettoverrun(2) and timer_delete(2) are handled directly
 * by the native calls.
 */

/*
 * timerfd_create() is handled by the standard COMPAT_LINUX call.
 */

int
linux32_sys_timerfd_gettime(struct lwp *l,
    const struct linux32_sys_timerfd_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct linux32_itimerspec *) tim;
	} */
	struct itimerspec its;
	struct linux32_itimerspec lits;
	int error;

	error = do_timerfd_gettime(l, SCARG(uap, fd), &its, retval);
	if (error == 0) {
		native_to_linux32_itimerspec(&lits, &its);
		error = copyout(&lits, SCARG(uap, tim), sizeof(lits));
	}

	return error;
}

int
linux32_sys_timerfd_settime(struct lwp *l,
    const struct linux32_sys_timerfd_settime_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) flags;
		syscallarg(const struct linux32_itimerspec *) tim;
		syscallarg(struct linux32_itimerspec *) otim;
	} */
	struct itimerspec nits, oits, *oitsp = NULL;
	struct linux32_itimerspec lits;
	int nflags;
	int error;

	error = copyin(SCARG(uap, tim), &lits, sizeof(lits));
	if (error) {
		return error;
	}
	linux32_to_native_itimerspec(&nits, &lits);

	error = linux_to_native_timerfd_settime_flags(&nflags,
	    SCARG(uap, flags));
	if (error) {
		return error;
	}

	if (SCARG(uap, otim)) {
		oitsp = &oits;
	}

	error = do_timerfd_settime(l, SCARG(uap, fd), nflags,
	    &nits, oitsp, retval);
	if (error == 0 && oitsp != NULL) {
		native_to_linux32_itimerspec(&lits, oitsp);
		error = copyout(&lits, SCARG(uap, otim), sizeof(lits));
	}

	return error;
}
