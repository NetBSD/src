/*	$NetBSD: netbsd32_compat_50.c,v 1.20.10.4 2017/12/03 11:36:55 jdolecek Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_50.c,v 1.20.10.4 2017/12/03 11:36:55 jdolecek Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ktrace.h>
#include <sys/eventvar.h>
#include <sys/resourcevar.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/poll.h>
#include <sys/namei.h>
#include <sys/statvfs.h>
#include <sys/syscallargs.h>
#include <sys/proc.h>
#include <sys/dirent.h>
#include <sys/kauth.h>
#include <sys/vfs_syscalls.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/sys/mount.h>
#include <compat/sys/time.h>

#if defined(COMPAT_50)

/*
 * Common routine to set access and modification times given a vnode.
 */
static int
get_utimes32(const netbsd32_timeval50p_t *tptr, struct timeval *tv,
    struct timeval **tvp)
{
	int error;
	struct netbsd32_timeval50 tv32[2];

	if (tptr == NULL) {
		*tvp = NULL;
		return 0;
	}

	error = copyin(tptr, tv32, sizeof(tv32));
	if (error)
		return error;
	netbsd32_to_timeval50(&tv32[0], &tv[0]);
	netbsd32_to_timeval50(&tv32[1], &tv[1]);

	*tvp = tv;
	return 0;
}

int
compat_50_netbsd32_mknod(struct lwp *l,
    const struct compat_50_netbsd32_mknod_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_charp) path;
		syscallarg(mode_t) mode;
		syscallarg(uint32_t) dev;
	} */
	return do_sys_mknod(l, SCARG_P32(uap, path), SCARG(uap, mode),
	    SCARG(uap, dev), retval, UIO_USERSPACE);
}

int
compat_50_netbsd32_select(struct lwp *l,
    const struct compat_50_netbsd32_select_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) nd;
		syscallarg(netbsd32_fd_setp_t) in;
		syscallarg(netbsd32_fd_setp_t) ou;
		syscallarg(netbsd32_fd_setp_t) ex;
		syscallarg(netbsd32_timeval50p_t) tv;
	} */
	int error;
	struct netbsd32_timeval50 tv32;
	struct timespec ats, *ts = NULL;

	if (SCARG_P32(uap, tv)) {
		error = copyin(SCARG_P32(uap, tv), &tv32, sizeof(tv32));
		if (error != 0)
			return error;
		ats.tv_sec = tv32.tv_sec;
		ats.tv_nsec = tv32.tv_usec * 1000;
		ts = &ats;
	}

	return selcommon(retval, SCARG(uap, nd), SCARG_P32(uap, in),
	    SCARG_P32(uap, ou), SCARG_P32(uap, ex), ts, NULL);
}

int
compat_50_netbsd32_gettimeofday(struct lwp *l,
    const struct compat_50_netbsd32_gettimeofday_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_timeval50p_t) tp;
		syscallarg(netbsd32_timezonep_t) tzp;
	} */
	struct timeval atv;
	struct netbsd32_timeval50 tv32;
	int error = 0;
	struct netbsd32_timezone tzfake;

	if (SCARG_P32(uap, tp)) {
		microtime(&atv);
		netbsd32_from_timeval50(&atv, &tv32);
		error = copyout(&tv32, SCARG_P32(uap, tp), sizeof(tv32));
		if (error)
			return error;
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
	return error;
}

int
compat_50_netbsd32_settimeofday(struct lwp *l,
    const struct compat_50_netbsd32_settimeofday_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_timeval50p_t) tv;
		syscallarg(const netbsd32_timezonep_t) tzp;
	} */
	struct netbsd32_timeval50 atv32;
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

	netbsd32_to_timeval50(&atv32, &atv);
	TIMEVAL_TO_TIMESPEC(&atv, &ats);
	return settime(p, &ats);
}

int
compat_50_netbsd32_utimes(struct lwp *l,
    const struct compat_50_netbsd32_utimes_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_timeval50p_t) tptr;
	} */
	int error;
	struct timeval tv[2], *tvp;

	error = get_utimes32(SCARG_P32(uap, tptr), tv, &tvp);
	if (error != 0)
		return error;

	return do_sys_utimes(l, NULL, SCARG_P32(uap, path), FOLLOW,
	    tvp, UIO_SYSSPACE);
}

int
compat_50_netbsd32_adjtime(struct lwp *l,
    const struct compat_50_netbsd32_adjtime_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_timeval50p_t) delta;
		syscallarg(netbsd32_timeval50p_t) olddelta;
	} */
	struct netbsd32_timeval50 atv;
	int error;

	extern int time_adjusted;     /* in kern_ntptime.c */
	extern int64_t time_adjtime;  /* in kern_ntptime.c */

	if ((error = kauth_authorize_system(l->l_cred,
	    KAUTH_SYSTEM_TIME, KAUTH_REQ_SYSTEM_TIME_ADJTIME, NULL, NULL,
	    NULL)) != 0)
		return (error);

	if (SCARG_P32(uap, olddelta)) {
		mutex_spin_enter(&timecounter_lock);
		atv.tv_sec = time_adjtime / 1000000;
		atv.tv_usec = time_adjtime % 1000000;
		if (atv.tv_usec < 0) {
			atv.tv_usec += 1000000;
			atv.tv_sec--;
		}
		mutex_spin_exit(&timecounter_lock);

		error = copyout(&atv, SCARG_P32(uap, olddelta), sizeof(atv));
		if (error)
			return (error);
	}
	
	if (SCARG_P32(uap, delta)) {
		error = copyin(SCARG_P32(uap, delta), &atv, sizeof(atv));
		if (error)
			return (error);

		mutex_spin_enter(&timecounter_lock);
		time_adjtime = (int64_t)atv.tv_sec * 1000000 + atv.tv_usec;
		if (time_adjtime)
			/* We need to save the system time during shutdown */
			time_adjusted |= 1;
		mutex_spin_exit(&timecounter_lock);
	}

	return 0;
}

int
compat_50_netbsd32_futimes(struct lwp *l,
    const struct compat_50_netbsd32_futimes_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_timeval50p_t) tptr;
	} */
	int error;
	file_t *fp;
	struct timeval tv[2], *tvp;

	error = get_utimes32(SCARG_P32(uap, tptr), tv, &tvp);
	if (error != 0)
		return error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return error;

	error = do_sys_utimes(l, fp->f_vnode, NULL, 0, tvp, UIO_SYSSPACE);

	fd_putfile(SCARG(uap, fd));
	return error;
}

int
compat_50_netbsd32_clock_gettime(struct lwp *l,
    const struct compat_50_netbsd32_clock_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(netbsd32_timespec50p_t) tp;
	} */
	int error;
	struct timespec ats;
	struct netbsd32_timespec50 ts32;

	error = clock_gettime1(SCARG(uap, clock_id), &ats);
	if (error != 0)
		return error;

	netbsd32_from_timespec50(&ats, &ts32);
	return copyout(&ts32, SCARG_P32(uap, tp), sizeof(ts32));
}

int
compat_50_netbsd32_clock_settime(struct lwp *l,
    const struct compat_50_netbsd32_clock_settime_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(const netbsd32_timespec50p_t) tp;
	} */
	struct netbsd32_timespec50 ts32;
	struct timespec ats;
	int error;

	if ((error = copyin(SCARG_P32(uap, tp), &ts32, sizeof(ts32))) != 0)
		return (error);

	netbsd32_to_timespec50(&ts32, &ats);
	return clock_settime1(l->l_proc, SCARG(uap, clock_id), &ats, true);
}

int
compat_50_netbsd32_clock_getres(struct lwp *l,
    const struct compat_50_netbsd32_clock_getres_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(netbsd32_timespec50p_t) tp;
	} */
	struct netbsd32_timespec50 ts32;
	struct timespec ts;
	int error;

	error = clock_getres1(SCARG(uap, clock_id), &ts);
	if (error != 0)
		return error;

	if (SCARG_P32(uap, tp)) {
		netbsd32_from_timespec50(&ts, &ts32);
		error = copyout(&ts32, SCARG_P32(uap, tp), sizeof(ts32));
	}

	return error;
}

int
compat_50_netbsd32_timer_settime(struct lwp *l,
    const struct compat_50_netbsd32_timer_settime_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_timer_t) timerid;
		syscallarg(int) flags;
		syscallarg(const netbsd32_itimerspec50p_t) value;
		syscallarg(netbsd32_itimerspec50p_t) ovalue;
	} */
	int error;
	struct itimerspec value, ovalue, *ovp = NULL;
	struct netbsd32_itimerspec50 its32;

	if ((error = copyin(SCARG_P32(uap, value), &its32, sizeof(its32))) != 0)
		return (error);
	netbsd32_to_timespec50(&its32.it_interval, &value.it_interval);
	netbsd32_to_timespec50(&its32.it_value, &value.it_value);

	if (SCARG_P32(uap, ovalue))
		ovp = &ovalue;

	if ((error = dotimer_settime(SCARG(uap, timerid), &value, ovp,
	    SCARG(uap, flags), l->l_proc)) != 0)
		return error;

	if (ovp) {
		netbsd32_from_timespec50(&ovp->it_interval, &its32.it_interval);
		netbsd32_from_timespec50(&ovp->it_value, &its32.it_value);
		return copyout(&its32, SCARG_P32(uap, ovalue), sizeof(its32));
	}
	return 0;
}

int
compat_50_netbsd32_timer_gettime(struct lwp *l, const struct compat_50_netbsd32_timer_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_timer_t) timerid;
		syscallarg(netbsd32_itimerspec50p_t) value;
	} */
	int error;
	struct itimerspec its;
	struct netbsd32_itimerspec50 its32;

	if ((error = dotimer_gettime(SCARG(uap, timerid), l->l_proc,
	    &its)) != 0)
		return error;

	netbsd32_from_timespec50(&its.it_interval, &its32.it_interval);
	netbsd32_from_timespec50(&its.it_value, &its32.it_value);

	return copyout(&its32, SCARG_P32(uap, value), sizeof(its32));
}

int
compat_50_netbsd32_nanosleep(struct lwp *l,
    const struct compat_50_netbsd32_nanosleep_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_timespec50p_t) rqtp;
		syscallarg(netbsd32_timespecp_t) rmtp;
	} */
	struct netbsd32_timespec50 ts32;
	struct timespec rqt, rmt;
	int error, error1;

	error = copyin(SCARG_P32(uap, rqtp), &ts32, sizeof(ts32));
	if (error)
		return (error);
	netbsd32_to_timespec50(&ts32, &rqt);

	error = nanosleep1(l, CLOCK_MONOTONIC, 0, &rqt,
	    SCARG_P32(uap, rmtp) ? &rmt : NULL);
	if (SCARG_P32(uap, rmtp) == NULL || (error != 0 && error != EINTR))
		return error;

	netbsd32_from_timespec50(&rmt, &ts32);
	error1 = copyout(&ts32, SCARG_P32(uap,rmtp), sizeof(ts32));
	return error1 ? error1 : error;
}

static int
compat_50_netbsd32_sigtimedwait_put_info(const void *src, void *dst, size_t size)
{
	const siginfo_t *info = src;
	siginfo32_t info32;

	netbsd32_si_to_si32(&info32, info);

	return copyout(&info32, dst, sizeof(info32));
}

static int
compat_50_netbsd32_sigtimedwait_fetch_timeout(const void *src, void *dst, size_t size)
{
	struct timespec *ts = dst;
	struct netbsd32_timespec50 ts32;
	int error;

	error = copyin(src, &ts32, sizeof(ts32));
	if (error)
		return error;

	netbsd32_to_timespec50(&ts32, ts);
	return 0;
}

static int
compat_50_netbsd32_sigtimedwait_put_timeout(const void *src, void *dst, size_t size)
{
	const struct timespec *ts = src;
	struct netbsd32_timespec50 ts32;

	netbsd32_from_timespec50(ts, &ts32);

	return copyout(&ts32, dst, sizeof(ts32));
}

int
compat_50_netbsd32___sigtimedwait(struct lwp *l,
    const struct compat_50_netbsd32___sigtimedwait_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_sigsetp_t) set;
		syscallarg(netbsd32_siginfop_t) info;
		syscallarg(netbsd32_timespec50p_t) timeout;
	} */
	struct sys_____sigtimedwait50_args ua;
	int res;

	NETBSD32TOP_UAP(set, const sigset_t);
	NETBSD32TOP_UAP(info, siginfo_t);
	NETBSD32TOP_UAP(timeout, struct timespec);

	res = sigtimedwait1(l, &ua, retval,
	    copyin,
	    compat_50_netbsd32_sigtimedwait_put_info,
	    compat_50_netbsd32_sigtimedwait_fetch_timeout,
	    compat_50_netbsd32_sigtimedwait_put_timeout);
	if (!res)
		*retval = 0; /* XXX NetBSD<=5 was not POSIX compliant */
	return res;
}

int
compat_50_netbsd32_lutimes(struct lwp *l,
    const struct compat_50_netbsd32_lutimes_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_timeval50p_t) tptr;
	} */
	int error;
	struct timeval tv[2], *tvp;

	error = get_utimes32(SCARG_P32(uap, tptr), tv, &tvp);
	if (error != 0)
		return error;

	return do_sys_utimes(l, NULL, SCARG_P32(uap, path), NOFOLLOW,
	    tvp, UIO_SYSSPACE);
}

int
compat_50_netbsd32__lwp_park(struct lwp *l,
    const struct compat_50_netbsd32__lwp_park_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_timespec50p) ts;
		syscallarg(lwpid_t) unpark;
		syscallarg(netbsd32_voidp) hint;
		syscallarg(netbsd32_voidp) unparkhint;
	} */
	struct timespec ts, *tsp;
	struct netbsd32_timespec50 ts32;
	int error;

	if (SCARG_P32(uap, ts) == NULL)
		tsp = NULL;
	else {
		error = copyin(SCARG_P32(uap, ts), &ts32, sizeof ts32);
		if (error != 0)
			return error;
		netbsd32_to_timespec50(&ts32, &ts);
		tsp = &ts;
	}

	if (SCARG(uap, unpark) != 0) {
		error = lwp_unpark(SCARG(uap, unpark),
		    SCARG_P32(uap, unparkhint));
		if (error != 0)
			return error;
	}

	return lwp_park(CLOCK_REALTIME, TIMER_ABSTIME, tsp,
	    SCARG_P32(uap, hint));
}

static int
netbsd32_kevent_fetch_timeout(const void *src, void *dest, size_t length)
{
	struct netbsd32_timespec50 ts32;
	int error;

	KASSERT(length == sizeof(struct timespec50));

	error = copyin(src, &ts32, sizeof(ts32));
	if (error)
		return error;
	netbsd32_to_timespec50(&ts32, (struct timespec *)dest);
	return 0;
}

static int
netbsd32_kevent_fetch_changes(void *ctx, const struct kevent *changelist,
    struct kevent *changes, size_t index, int n)
{
	const struct netbsd32_kevent *src =
	    (const struct netbsd32_kevent *)changelist;
	struct netbsd32_kevent *kev32, *changes32 = ctx;
	int error, i;

	error = copyin(src + index, changes32, n * sizeof(*changes32));
	if (error)
		return error;
	for (i = 0, kev32 = changes32; i < n; i++, kev32++, changes++)
		netbsd32_to_kevent(kev32, changes);
	return 0;
}

static int
netbsd32_kevent_put_events(void *ctx, struct kevent *events,
    struct kevent *eventlist, size_t index, int n)
{
	struct netbsd32_kevent *kev32, *events32 = ctx;
	int i;

	for (i = 0, kev32 = events32; i < n; i++, kev32++, events++)
		netbsd32_from_kevent(events, kev32);
	kev32 = (struct netbsd32_kevent *)eventlist;
	return  copyout(events32, kev32, n * sizeof(*events32));
}

int
compat_50_netbsd32_kevent(struct lwp *l,
    const struct compat_50_netbsd32_kevent_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_keventp_t) changelist;
		syscallarg(netbsd32_size_t) nchanges;
		syscallarg(netbsd32_keventp_t) eventlist;
		syscallarg(netbsd32_size_t) nevents;
		syscallarg(netbsd32_timespec50p_t) timeout;
	} */
	int error;
	size_t maxalloc, nchanges, nevents;
	struct kevent_ops netbsd32_kevent_ops = {
		.keo_fetch_timeout = netbsd32_kevent_fetch_timeout,
		.keo_fetch_changes = netbsd32_kevent_fetch_changes,
		.keo_put_events = netbsd32_kevent_put_events,
	};

	nchanges = SCARG(uap, nchanges);
	nevents = SCARG(uap, nevents);
	maxalloc = KQ_NEVENTS;

	netbsd32_kevent_ops.keo_private =
	    kmem_alloc(maxalloc * sizeof(struct netbsd32_kevent), KM_SLEEP);

	error = kevent1(retval, SCARG(uap, fd),
	    NETBSD32PTR64(SCARG(uap, changelist)), nchanges,
	    NETBSD32PTR64(SCARG(uap, eventlist)), nevents,
	    NETBSD32PTR64(SCARG(uap, timeout)), &netbsd32_kevent_ops);

	kmem_free(netbsd32_kevent_ops.keo_private,
	    maxalloc * sizeof(struct netbsd32_kevent));
	return error;
}

int
compat_50_netbsd32_pselect(struct lwp *l,
    const struct compat_50_netbsd32_pselect_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) nd;
		syscallarg(netbsd32_fd_setp_t) in;
		syscallarg(netbsd32_fd_setp_t) ou;
		syscallarg(netbsd32_fd_setp_t) ex;
		syscallarg(const netbsd32_timespec50p_t) ts;
		syscallarg(const netbsd32_sigsetp_t) mask;
	} */
	int error;
	struct netbsd32_timespec50 ts32;
	struct timespec ats, *ts = NULL;
	sigset_t amask, *mask = NULL;

	if (SCARG_P32(uap, ts)) {
		error = copyin(SCARG_P32(uap, ts), &ts32, sizeof(ts32));
		if (error != 0)
			return error;
		netbsd32_to_timespec50(&ts32, &ats);
		ts = &ats;
	}
	if (SCARG_P32(uap, mask)) {
		error = copyin(SCARG_P32(uap, mask), &amask, sizeof(amask));
		if (error != 0)
			return error;
		mask = &amask;
	}

	return selcommon(retval, SCARG(uap, nd), SCARG_P32(uap, in),
	    SCARG_P32(uap, ou), SCARG_P32(uap, ex), ts, mask);
}

int
compat_50_netbsd32_pollts(struct lwp *l,
    const struct compat_50_netbsd32_pollts_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct netbsd32_pollfdp_t) fds;
		syscallarg(u_int) nfds;
		syscallarg(const netbsd32_timespec50p_t) ts;
		syscallarg(const netbsd32_sigsetp_t) mask;
	} */
	int error;
	struct netbsd32_timespec50 ts32;
	struct timespec ats, *ts = NULL;
	sigset_t amask, *mask = NULL;

	if (SCARG_P32(uap, ts)) {
		error = copyin(SCARG_P32(uap, ts), &ts32, sizeof(ts32));
		if (error != 0)
			return error;
		netbsd32_to_timespec50(&ts32, &ats);
		ts = &ats;
	}
	if (NETBSD32PTR64( SCARG(uap, mask))) {
		error = copyin(SCARG_P32(uap, mask), &amask, sizeof(amask));
		if (error != 0)
			return error;
		mask = &amask;
	}

	return pollcommon(retval, SCARG_P32(uap, fds),
	    SCARG(uap, nfds), ts, mask);
}

int
compat_50_netbsd32___stat30(struct lwp *l,
    const struct compat_50_netbsd32___stat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat50p_t) ub;
	} */
	struct netbsd32_stat50 sb32;
	struct stat sb;
	int error;
	const char *path;

	path = SCARG_P32(uap, path);

	error = do_sys_stat(path, FOLLOW, &sb);
	if (error)
		return error;
	netbsd32_from___stat50(&sb, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, ub), sizeof(sb32));
	return error;
}

int
compat_50_netbsd32___fstat30(struct lwp *l,
    const struct compat_50_netbsd32___fstat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_stat50p_t) sb;
	} */
	struct netbsd32_stat50 sb32;
	struct stat ub;
	int error;

	error = do_sys_fstat(SCARG(uap, fd), &ub);
	if (error == 0) {
		netbsd32_from___stat50(&ub, &sb32);
		error = copyout(&sb32, SCARG_P32(uap, sb), sizeof(sb32));
	}
	return error;
}

int
compat_50_netbsd32___lstat30(struct lwp *l,
    const struct compat_50_netbsd32___lstat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat50p_t) ub;
	} */
	struct netbsd32_stat50 sb32;
	struct stat sb;
	int error;
	const char *path;

	path = SCARG_P32(uap, path);

	error = do_sys_stat(path, NOFOLLOW, &sb);
	if (error)
		return error;
	netbsd32_from___stat50(&sb, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, ub), sizeof(sb32));
	return error;
}

int
compat_50_netbsd32___fhstat40(struct lwp *l, const struct compat_50_netbsd32___fhstat40_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_pointer_t) fhp;
		syscallarg(netbsd32_size_t) fh_size;
		syscallarg(netbsd32_stat50p_t) sb;
	} */
	struct stat sb;
	struct netbsd32_stat50 sb32;
	int error;

	error = do_fhstat(l, SCARG_P32(uap, fhp), SCARG(uap, fh_size), &sb);
	if (error != 0) {
		netbsd32_from___stat50(&sb, &sb32);
		error = copyout(&sb32, SCARG_P32(uap, sb), sizeof(sb));
	}
	return error;
}

int
compat_50_netbsd32_wait4(struct lwp *l, const struct compat_50_netbsd32_wait4_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(netbsd32_intp) status;
		syscallarg(int) options;
		syscallarg(netbsd32_rusage50p_t) rusage;
	} */
	int error, status, pid = SCARG(uap, pid);
	struct netbsd32_rusage50 ru32;
	struct rusage ru;

	error = do_sys_wait(&pid, &status, SCARG(uap, options),
	    SCARG_P32(uap, rusage) != NULL ? &ru : NULL);

	retval[0] = pid;
	if (pid == 0)
		return error;

	if (SCARG_P32(uap, rusage)) {
		netbsd32_from_rusage50(&ru, &ru32);
		error = copyout(&ru32, SCARG_P32(uap, rusage), sizeof(ru32));
	}

	if (error == 0 && SCARG_P32(uap, status))
		error = copyout(&status, SCARG_P32(uap, status), sizeof(status));

	return error;
}


int
compat_50_netbsd32_getrusage(struct lwp *l, const struct compat_50_netbsd32_getrusage_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) who;
		syscallarg(netbsd32_rusage50p_t) rusage;
	} */
	int error;
	struct proc *p = l->l_proc;
	struct rusage ru;
	struct netbsd32_rusage50 ru32;

	error = getrusage1(p, SCARG(uap, who), &ru);
	if (error != 0)
		return error;

	netbsd32_from_rusage50(&ru, &ru32);
	return copyout(&ru32, SCARG_P32(uap, rusage), sizeof(ru32));
}

int
compat_50_netbsd32_setitimer(struct lwp *l,
    const struct compat_50_netbsd32_setitimer_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(const netbsd32_itimerval50p_t) itv;
		syscallarg(netbsd32_itimerval50p_t) oitv;
	} */
	struct proc *p = l->l_proc;
	struct netbsd32_itimerval50 s32it, *itv32;
	int which = SCARG(uap, which);
	struct compat_50_netbsd32_getitimer_args getargs;
	struct itimerval aitv;
	int error;

	if ((u_int)which > ITIMER_PROF)
		return (EINVAL);
	itv32 = SCARG_P32(uap, itv);
	if (itv32) {
		if ((error = copyin(itv32, &s32it, sizeof(s32it))))
			return (error);
		netbsd32_to_itimerval50(&s32it, &aitv);
	}
	if (SCARG_P32(uap, oitv) != 0) {
		SCARG(&getargs, which) = which;
		SCARG(&getargs, itv) = SCARG(uap, oitv);
		if ((error = compat_50_netbsd32_getitimer(l, &getargs, retval)) != 0)
			return (error);
	}
	if (itv32 == 0)
		return 0;

	return dosetitimer(p, which, &aitv);
}

int
compat_50_netbsd32_getitimer(struct lwp *l, const struct compat_50_netbsd32_getitimer_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(netbsd32_itimerval50p_t) itv;
	} */
	struct proc *p = l->l_proc;
	struct netbsd32_itimerval50 s32it;
	struct itimerval aitv;
	int error;

	error = dogetitimer(p, SCARG(uap, which), &aitv);
	if (error)
		return error;

	netbsd32_from_itimerval50(&aitv, &s32it);
	return copyout(&s32it, SCARG_P32(uap, itv), sizeof(s32it));
}

int
compat_50_netbsd32_quotactl(struct lwp *l, const struct compat_50_netbsd32_quotactl_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) cmd;
		syscallarg(int) uid;
		syscallarg(netbsd32_voidp) arg;
	} */
	struct compat_50_sys_quotactl_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(cmd);
	NETBSD32TO64_UAP(uid);
	NETBSD32TOP_UAP(arg, void *);
	return (compat_50_sys_quotactl(l, &ua, retval));
}

#endif /* COMPAT_50 */
