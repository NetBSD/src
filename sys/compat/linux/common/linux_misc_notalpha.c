/*	$NetBSD: linux_misc_notalpha.c,v 1.86.2.2 2007/05/27 14:35:07 ad Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz; by Jason R. Thorpe
 * of the Numerical Aerospace Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: linux_misc_notalpha.c,v 1.86.2.2 2007/05/27 14:35:07 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/time.h>
#include <sys/vfs_syscalls.h>
#include <sys/wait.h>
#include <sys/kauth.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_mmap.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>

#include <compat/linux/linux_syscallargs.h>

/*
 * This file contains routines which are used
 * on every linux architechture except the Alpha.
 */

/* Used on: arm, i386, m68k, mips, ppc, sparc, sparc64 */
/* Not used on: alpha */

#ifdef DEBUG_LINUX
#define DPRINTF(a)	uprintf a
#else
#define DPRINTF(a)
#endif

#ifndef COMPAT_LINUX32
#if !defined(__m68k__) && !defined(__amd64__)
static void bsd_to_linux_statfs64(const struct statvfs *,
	struct linux_statfs64  *);
#endif

/*
 * Alarm. This is a libc call which uses setitimer(2) in NetBSD.
 * Fiddle with the timers to make it work.
 */
int
linux_sys_alarm(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_alarm_args /* {
		syscallarg(unsigned int) secs;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct timeval now;
	struct itimerval *itp, it;
	struct ptimer *ptp;
	int s;

	if (p->p_timers && p->p_timers->pts_timers[ITIMER_REAL])
		itp = &p->p_timers->pts_timers[ITIMER_REAL]->pt_time;
	else
		itp = NULL;
	s = splclock();
	/*
	 * Clear any pending timer alarms.
	 */
	if (itp) {
		callout_stop(&p->p_timers->pts_timers[ITIMER_REAL]->pt_ch);
		timerclear(&itp->it_interval);
		getmicrotime(&now);
		if (timerisset(&itp->it_value) &&
		    timercmp(&itp->it_value, &now, >))
			timersub(&itp->it_value, &now, &itp->it_value);
		/*
		 * Return how many seconds were left (rounded up)
		 */
		retval[0] = itp->it_value.tv_sec;
		if (itp->it_value.tv_usec)
			retval[0]++;
	} else {
		retval[0] = 0;
	}

	/*
	 * alarm(0) just resets the timer.
	 */
	if (SCARG(uap, secs) == 0) {
		if (itp)
			timerclear(&itp->it_value);
		splx(s);
		return 0;
	}

	/*
	 * Check the new alarm time for sanity, and set it.
	 */
	timerclear(&it.it_interval);
	it.it_value.tv_sec = SCARG(uap, secs);
	it.it_value.tv_usec = 0;
	if (itimerfix(&it.it_value) || itimerfix(&it.it_interval)) {
		splx(s);
		return (EINVAL);
	}

	if (p->p_timers == NULL)
		timers_alloc(p);
	ptp = p->p_timers->pts_timers[ITIMER_REAL];
	if (ptp == NULL) {
		ptp = pool_get(&ptimer_pool, PR_WAITOK);
		ptp->pt_ev.sigev_notify = SIGEV_SIGNAL;
		ptp->pt_ev.sigev_signo = SIGALRM;
		ptp->pt_overruns = 0;
		ptp->pt_proc = p;
		ptp->pt_type = CLOCK_REALTIME;
		ptp->pt_entry = CLOCK_REALTIME;
		callout_init(&ptp->pt_ch);
		p->p_timers->pts_timers[ITIMER_REAL] = ptp;
	}

	if (timerisset(&it.it_value)) {
		/*
		 * Don't need to check hzto() return value, here.
		 * callout_reset() does it for us.
		 */
		getmicrotime(&now);
		timeradd(&it.it_value, &now, &it.it_value);
		callout_reset(&ptp->pt_ch, hzto(&it.it_value),
		    realtimerexpire, ptp);
	}
	ptp->pt_time = it;
	splx(s);

	return 0;
}
#endif /* !COMPAT_LINUX32 */

#if !defined(__amd64__) || defined(COMPAT_LINUX32)
int
linux_sys_nice(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_nice_args /* {
		syscallarg(int) incr;
	} */ *uap = v;
        struct sys_setpriority_args bsa;

        SCARG(&bsa, which) = PRIO_PROCESS;
        SCARG(&bsa, who) = 0;
	SCARG(&bsa, prio) = SCARG(uap, incr);
        return sys_setpriority(l, &bsa, retval);
}
#endif /* !__amd64__ || COMPAT_LINUX32 */

#ifndef COMPAT_LINUX32
#ifndef __amd64__
/*
 * The old Linux readdir was only able to read one entry at a time,
 * even though it had a 'count' argument. In fact, the emulation
 * of the old call was better than the original, because it did handle
 * the count arg properly. Don't bother with it anymore now, and use
 * it to distinguish between old and new. The difference is that the
 * newer one actually does multiple entries, and the reclen field
 * really is the reclen, not the namelength.
 */
int
linux_sys_readdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_readdir_args /* {
		syscallarg(int) fd;
		syscallarg(struct linux_dirent *) dent;
		syscallarg(unsigned int) count;
	} */ *uap = v;

	SCARG(uap, count) = 1;
	return linux_sys_getdents(l, uap, retval);
}
#endif /* !amd64 */

/*
 * I wonder why Linux has gettimeofday() _and_ time().. Still, we
 * need to deal with it.
 */
int
linux_sys_time(struct lwp *l, void *v, register_t *retval)
{
	struct linux_sys_time_args /* {
		linux_time_t *t;
	} */ *uap = v;
	struct timeval atv;
	linux_time_t tt;
	int error;

	microtime(&atv);

	tt = atv.tv_sec;
	if (SCARG(uap, t) && (error = copyout(&tt, SCARG(uap, t), sizeof tt)))
		return error;

	retval[0] = tt;
	return 0;
}

/*
 * utime(). Do conversion to things that utimes() understands,
 * and pass it on.
 */
int
linux_sys_utime(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_utime_args /* {
		syscallarg(const char *) path;
		syscallarg(struct linux_utimbuf *)times;
	} */ *uap = v;
	int error;
	struct timeval tv[2], *tvp;
	struct linux_utimbuf lut;

	if (SCARG(uap, times) != NULL) {
		if ((error = copyin(SCARG(uap, times), &lut, sizeof lut)))
			return error;
		tv[0].tv_usec = tv[1].tv_usec = 0;
		tv[0].tv_sec = lut.l_actime;
		tv[1].tv_sec = lut.l_modtime;
		tvp = tv;
	} else
		tvp = NULL;

	return do_sys_utimes(l, NULL, SCARG(uap, path), FOLLOW,
			   tvp,  UIO_SYSSPACE);
}

#ifndef __amd64__
/*
 * waitpid(2).  Just forward on to linux_sys_wait4 with a NULL rusage.
 */
int
linux_sys_waitpid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_waitpid_args /* {
		syscallarg(int) pid;
		syscallarg(int *) status;
		syscallarg(int) options;
	} */ *uap = v;
	struct linux_sys_wait4_args linux_w4a;

	SCARG(&linux_w4a, pid) = SCARG(uap, pid);
	SCARG(&linux_w4a, status) = SCARG(uap, status);
	SCARG(&linux_w4a, options) = SCARG(uap, options);
	SCARG(&linux_w4a, rusage) = NULL;

	return linux_sys_wait4(l, &linux_w4a, retval);
}
#endif /* !amd64 */

int
linux_sys_setresgid(struct lwp *l, void *v, register_t *retval)
{
	struct linux_sys_setresgid_args /* {
		syscallarg(gid_t) rgid;
		syscallarg(gid_t) egid;
		syscallarg(gid_t) sgid;
	} */ *uap = v;

	/*
	 * Note: These checks are a little different than the NetBSD
	 * setregid(2) call performs.  This precisely follows the
	 * behavior of the Linux kernel.
	 */
	return do_setresgid(l, SCARG(uap,rgid), SCARG(uap, egid),
			    SCARG(uap, sgid),
			    ID_R_EQ_R | ID_R_EQ_E | ID_R_EQ_S |
			    ID_E_EQ_R | ID_E_EQ_E | ID_E_EQ_S |
			    ID_S_EQ_R | ID_S_EQ_E | ID_S_EQ_S );
}

int
linux_sys_getresgid(struct lwp *l, void *v, register_t *retval)
{
	struct linux_sys_getresgid_args /* {
		syscallarg(gid_t *) rgid;
		syscallarg(gid_t *) egid;
		syscallarg(gid_t *) sgid;
	} */ *uap = v;
	kauth_cred_t pc = l->l_cred;
	int error;
	gid_t gid;

	/*
	 * Linux copies these values out to userspace like so:
	 *
	 *	1. Copy out rgid.
	 *	2. If that succeeds, copy out egid.
	 *	3. If both of those succeed, copy out sgid.
	 */
	gid = kauth_cred_getgid(pc);
	if ((error = copyout(&gid, SCARG(uap, rgid), sizeof(gid_t))) != 0)
		return (error);

	gid = kauth_cred_getegid(pc);
	if ((error = copyout(&gid, SCARG(uap, egid), sizeof(gid_t))) != 0)
		return (error);

	gid = kauth_cred_getsvgid(pc);

	return (copyout(&gid, SCARG(uap, sgid), sizeof(gid_t)));
}

#ifndef __amd64__
/*
 * I wonder why Linux has settimeofday() _and_ stime().. Still, we
 * need to deal with it.
 */
int
linux_sys_stime(struct lwp *l, void *v, register_t *retval)
{
	struct linux_sys_time_args /* {
		linux_time_t *t;
	} */ *uap = v;
	struct timespec ats;
	linux_time_t tt;
	int error;

	if ((error = kauth_authorize_system(l->l_cred,
	    KAUTH_SYSTEM_TIME, KAUTH_REQ_SYSTEM_TIME_SYSTEM, NULL, NULL,
	    NULL)) != 0)
		return (error);

	if ((error = copyin(&tt, SCARG(uap, t), sizeof tt)) != 0)
		return error;

	ats.tv_sec = tt;
	ats.tv_nsec = 0;

	if ((error = settime(l->l_proc, &ats)))
		return (error);

	return 0;
}
#endif /* !amd64 */

#if !defined(__m68k__) && !defined(__amd64__)
/*
 * Convert NetBSD statvfs structure to Linux statfs64 structure.
 * See comments in bsd_to_linux_statfs() for further background.
 * We can safely pass correct bsize and frsize here, since Linux glibc
 * statvfs() doesn't use statfs64().
 */
static void
bsd_to_linux_statfs64(bsp, lsp)
	const struct statvfs *bsp;
	struct linux_statfs64 *lsp;
{
	int i, div;

	for (i = 0; i < linux_fstypes_cnt; i++) {
		if (strcmp(bsp->f_fstypename, linux_fstypes[i].bsd) == 0) {
			lsp->l_ftype = linux_fstypes[i].linux;
			break;
		}
	}

	if (i == linux_fstypes_cnt) {
		DPRINTF(("unhandled fstype in linux emulation: %s\n",
		    bsp->f_fstypename));
		lsp->l_ftype = LINUX_DEFAULT_SUPER_MAGIC;
	}

	div = bsp->f_frsize ? (bsp->f_bsize / bsp->f_frsize) : 1;
	if (div == 0)
		div = 1;
	lsp->l_fbsize = bsp->f_bsize;
	lsp->l_ffrsize = bsp->f_frsize;
	lsp->l_fblocks = bsp->f_blocks / div;
	lsp->l_fbfree = bsp->f_bfree / div;
	lsp->l_fbavail = bsp->f_bavail / div;
	lsp->l_ffiles = bsp->f_files;
	lsp->l_fffree = bsp->f_ffree / div;
	/* Linux sets the fsid to 0..., we don't */
	lsp->l_ffsid.val[0] = bsp->f_fsidx.__fsid_val[0];
	lsp->l_ffsid.val[1] = bsp->f_fsidx.__fsid_val[1];
	lsp->l_fnamelen = bsp->f_namemax;
	(void)memset(lsp->l_fspare, 0, sizeof(lsp->l_fspare));
}

/*
 * Implement the fs stat functions. Straightforward.
 */
int
linux_sys_statfs64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_statfs64_args /* {
		syscallarg(const char *) path;
		syscallarg(size_t) sz;
		syscallarg(struct linux_statfs64 *) sp;
	} */ *uap = v;
	struct statvfs *sb;
	struct linux_statfs64 ltmp;
	int error;

	if (SCARG(uap, sz) != sizeof ltmp)
		return (EINVAL);

	sb = STATVFSBUF_GET();
	error = do_sys_pstatvfs(l, SCARG(uap, path), ST_WAIT, sb);
	if (error == 0) {
		bsd_to_linux_statfs64(sb, &ltmp);
		error = copyout(&ltmp, SCARG(uap, sp), sizeof ltmp);
	}
	STATVFSBUF_PUT(sb);
	return error;
}

int
linux_sys_fstatfs64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_fstatfs64_args /* {
		syscallarg(int) fd;
		syscallarg(size_t) sz;
		syscallarg(struct linux_statfs64 *) sp;
	} */ *uap = v;
	struct statvfs *sb;
	struct linux_statfs64 ltmp;
	int error;

	if (SCARG(uap, sz) != sizeof ltmp)
		return (EINVAL);

	sb = STATVFSBUF_GET();
	error = do_sys_fstatvfs(l, SCARG(uap, fd), ST_WAIT, sb);
	if (error == 0) {
		bsd_to_linux_statfs64(sb, &ltmp);
		error = copyout(&ltmp, SCARG(uap, sp), sizeof ltmp);
	}
	STATVFSBUF_PUT(sb);
	return error;
}
#endif /* !__m68k__ && !__amd64__ */
#endif /* !COMPAT_LINUX32 */
