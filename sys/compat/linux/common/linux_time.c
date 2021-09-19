/*	$NetBSD: linux_time.c,v 1.41 2021/09/19 23:01:50 thorpej Exp $ */

/*-
 * Copyright (c) 2001, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus, and by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: linux_time.c,v 1.41 2021/09/19 23:01:50 thorpej Exp $");

#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/kauth.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/stdint.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/sched.h>
#include <sys/syscallargs.h>
#include <sys/lwp.h>
#include <sys/proc.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_sigevent.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_sched.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>

#include <compat/common/compat_util.h>

CTASSERT(LINUX_TIMER_ABSTIME == TIMER_ABSTIME);

/*
 * Linux keeps track of a system timezone in the kernel. It is readen
 * by gettimeofday and set by settimeofday. This emulates this behavior
 * See linux/kernel/time.c
 */
struct timezone linux_sys_tz;

int
linux_sys_gettimeofday(struct lwp *l, const struct linux_sys_gettimeofday_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct timeval50 *) tz;
		syscallarg(struct timezone *) tzp;
	} */
	int error = 0;

	if (SCARG(uap, tp)) {
		error = compat_50_sys_gettimeofday(l, (const void *)uap, retval);
		if (error)
			return (error);
	}

	if (SCARG(uap, tzp)) {
		error = copyout(&linux_sys_tz, SCARG(uap, tzp), sizeof(linux_sys_tz));
		if (error)
			return (error);
   }

	return (0);
}

int
linux_sys_settimeofday(struct lwp *l, const struct linux_sys_settimeofday_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct timeval50 *) tp;
		syscallarg(struct timezone *) tzp;
	} */
	int error = 0;

	if (SCARG(uap, tp)) {
		error = compat_50_sys_settimeofday(l, (const void *)uap, retval);
		if (error)
			return (error);
	}

	if (SCARG(uap, tzp)) {
		if (kauth_authorize_generic(kauth_cred_get(),
			KAUTH_GENERIC_ISSUSER, NULL) != 0)
			return (EPERM);
		error = copyin(SCARG(uap, tzp), &linux_sys_tz, sizeof(linux_sys_tz));
		if (error)
			return (error);
	}

	return (0);
}

void
native_to_linux_timespec(struct linux_timespec *ltp, const struct timespec *ntp)
{
	memset(ltp, 0, sizeof(*ltp));
	ltp->tv_sec = ntp->tv_sec;
	ltp->tv_nsec = ntp->tv_nsec;
}

void
linux_to_native_timespec(struct timespec *ntp, const struct linux_timespec *ltp)
{
	memset(ntp, 0, sizeof(*ntp));
	ntp->tv_sec = ltp->tv_sec;
	ntp->tv_nsec = ltp->tv_nsec;
}

void
native_to_linux_itimerspec(struct linux_itimerspec *litp,
    const struct itimerspec *nitp)
{
	memset(litp, 0, sizeof(*litp));
	native_to_linux_timespec(&litp->it_interval, &nitp->it_interval);
	native_to_linux_timespec(&litp->it_value, &nitp->it_value);
}

void
linux_to_native_itimerspec(struct itimerspec *nitp,
    const struct linux_itimerspec *litp)
{
	memset(nitp, 0, sizeof(*nitp));
	linux_to_native_timespec(&nitp->it_interval, &litp->it_interval);
	linux_to_native_timespec(&nitp->it_value, &litp->it_value);
}

int
linux_sys_nanosleep(struct lwp *l, const struct linux_sys_nanosleep_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(struct linux_timespec *) rqtp;
		syscallarg(struct linux_timespec *) rmtp;
	} */
	struct timespec rqts, rmts;
	struct linux_timespec lrqts, lrmts;
	int error, error1;

	error = copyin(SCARG(uap, rqtp), &lrqts, sizeof(lrqts));
	if (error != 0)
		return error;
	linux_to_native_timespec(&rqts, &lrqts);

	error = nanosleep1(l, CLOCK_MONOTONIC, 0, &rqts,
	    SCARG(uap, rmtp) ? &rmts : NULL);
	if (SCARG(uap, rmtp) == NULL || (error != 0 && error != EINTR))
		return error;

	native_to_linux_timespec(&lrmts, &rmts);
	error1 = copyout(&lrmts, SCARG(uap, rmtp), sizeof(lrmts));
	return error1 ? error1 : error;
}

int
linux_to_native_clockid(clockid_t *n, clockid_t l)
{
	switch (l) {
	case LINUX_CLOCK_REALTIME:
		*n = CLOCK_REALTIME;
		break;
	case LINUX_CLOCK_MONOTONIC:
		*n = CLOCK_MONOTONIC;
		break;
	case LINUX_CLOCK_PROCESS_CPUTIME_ID:
		*n = CLOCK_PROCESS_CPUTIME_ID /* self */;
		break;
	case LINUX_CLOCK_THREAD_CPUTIME_ID:
		*n = CLOCK_THREAD_CPUTIME_ID /* self */;
		break;

	case LINUX_CLOCK_MONOTONIC_RAW:
	case LINUX_CLOCK_REALTIME_COARSE:
	case LINUX_CLOCK_MONOTONIC_COARSE:
	case LINUX_CLOCK_BOOTTIME:
	case LINUX_CLOCK_BOOTTIME_ALARM:
	case LINUX_CLOCK_REALTIME_ALARM:
	default:
		return ENOTSUP;
	}

	return 0;
}

int
linux_sys_clock_gettime(struct lwp *l, const struct linux_sys_clock_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) which;
		syscallarg(struct linux_timespec *)tp;
	} */
	int error;
	clockid_t id;
	struct timespec ts;
	struct linux_timespec lts;

	error = linux_to_native_clockid(&id, SCARG(uap, which));
	if (error != 0)
		return error;

	error = clock_gettime1(id, &ts);
	if (error != 0)
		return error;

	native_to_linux_timespec(&lts, &ts);
	return copyout(&lts, SCARG(uap, tp), sizeof lts);
}

int
linux_sys_clock_settime(struct lwp *l, const struct linux_sys_clock_settime_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) which;
		syscallarg(struct linux_timespec *)tp;
	} */
	struct timespec ts;
	struct linux_timespec lts;
	clockid_t id;
	int error;

	error = linux_to_native_clockid(&id, SCARG(uap, which));
	if (error != 0)
		return error;

	error = copyin(SCARG(uap, tp), &lts, sizeof lts);
	if (error != 0)
		return error;

	linux_to_native_timespec(&ts, &lts);

	return clock_settime1(l->l_proc, id, &ts, true);
}

int
linux_sys_clock_getres(struct lwp *l, const struct linux_sys_clock_getres_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) which;
		syscallarg(struct linux_timespec *)tp;
	} */
	struct timespec ts;
	struct linux_timespec lts;
	int error;
	clockid_t nwhich = 0;	/* XXX: GCC */

	error = linux_to_native_clockid(&nwhich, SCARG(uap, which));
	if (error != 0 || SCARG(uap, tp) == NULL)
		return error;

	error = clock_getres1(nwhich, &ts);
	if (error != 0)
		return error;

	native_to_linux_timespec(&lts, &ts);
	return copyout(&lts, SCARG(uap, tp), sizeof lts);
}

int
linux_sys_clock_nanosleep(struct lwp *l, const struct linux_sys_clock_nanosleep_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) which;
		syscallarg(int) flags;
		syscallarg(struct linux_timespec) *rqtp;
		syscallarg(struct linux_timespec) *rmtp;
	} */
	struct linux_timespec lrqts, lrmts;
	struct timespec rqts, rmts;
	int error, error1, flags;
	clockid_t nwhich;

	flags = SCARG(uap, flags);
	if (flags & ~TIMER_ABSTIME) {
		return EINVAL;
	}

	error = linux_to_native_clockid(&nwhich, SCARG(uap, which));
	if (error != 0)
		return error;

	error = copyin(SCARG(uap, rqtp), &lrqts, sizeof lrqts);
	if (error != 0)
		return error;

	linux_to_native_timespec(&rqts, &lrqts);

	error = nanosleep1(l, nwhich, flags, &rqts,
	    SCARG(uap, rmtp) ? &rmts : NULL);
	if (SCARG(uap, rmtp) == NULL || (error != 0 && error != EINTR))
		return error;

	native_to_linux_timespec(&lrmts, &rmts);
	error1 = copyout(&lrmts, SCARG(uap, rmtp), sizeof lrmts);
	return error1 ? error1 : error;
}

int
linux_to_native_timer_create_clockid(clockid_t *nid, clockid_t lid)
{
	clockid_t id;
	int error;

	error = linux_to_native_clockid(&id, lid);
	if (error == 0) {
		/*
		 * We can't create a timer with every sort of clock ID
		 * that the system understands, so filter them out.
		 *
		 * Map CLOCK_PROCESS_CPUTIME_ID to CLOCK_VIRTUAL.
		 * We can't handle CLOCK_THREAD_CPUTIME_ID.
		 */
		switch (id) {
		case CLOCK_REALTIME:
		case CLOCK_MONOTONIC:
			break;

		case CLOCK_PROCESS_CPUTIME_ID:
			id = CLOCK_VIRTUAL;
			break;

		default:
			return ENOTSUP;
		}
		*nid = id;
	}

	return error;
}

int
linux_sys_timer_create(struct lwp *l,
    const struct linux_sys_timer_create_args *uap, register_t *retval)
{
	/* {
		syscallarg(clockid_t) clockid;
		syscallarg(struct linux_sigevent *) evp;
		syscallarg(timer_t *) timerid;
	} */
	clockid_t id;
	int error;

	error = linux_to_native_timer_create_clockid(&id, SCARG(uap, clockid));
	if (error == 0) {
		error = timer_create1(SCARG(uap, timerid), id,
		    (void *)SCARG(uap, evp), linux_sigevent_copyin, l);
	}

	return error;
}

int
linux_sys_timer_settime(struct lwp *l,
    const struct linux_sys_timer_settime_args *uap, register_t *retval)
{
	/* {
		syscallarg(timer_t) timerid;
		syscallarg(int) flags;
		syscallarg(const struct linux_itimerspec *) tim;
		syscallarg(struct linux_itimerspec *) otim;
	} */
	struct itimerspec value, ovalue, *ovp = NULL;
	struct linux_itimerspec tim, otim;
	int error;

	error = copyin(SCARG(uap, tim), &tim, sizeof(tim));
	if (error) {
		return error;
	}
	linux_to_native_itimerspec(&value, &tim);

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
		native_to_linux_itimerspec(&otim, ovp);
		error = copyout(&otim, SCARG(uap, otim), sizeof(otim));
	}

	return error;
}

int
linux_sys_timer_gettime(struct lwp *l,
    const struct linux_sys_timer_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(timer_t) timerid;
		syscallarg(struct linux_itimerspec *) tim;
	} */
	struct itimerspec its;
	struct linux_itimerspec lits;
	int error;

	error = dotimer_gettime(SCARG(uap, timerid), l->l_proc, &its);
	if (error == 0) {
		native_to_linux_itimerspec(&lits, &its);
		error = copyout(&lits, SCARG(uap, tim), sizeof(lits));
	}

	return error;
}

/*
 * timer_gettoverrun(2) and timer_delete(2) are handled directly
 * by the native calls.
 */
