/*	$NetBSD: linux_misc_notalpha.c,v 1.57 2000/08/07 02:51:04 itohy Exp $	*/

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
#include <sys/wait.h>

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

/*
 * Alarm. This is a libc call which uses setitimer(2) in NetBSD.
 * Fiddle with the timers to make it work.
 */
int
linux_sys_alarm(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_alarm_args /* {
		syscallarg(unsigned int) secs;
	} */ *uap = v;
	int s;
	struct itimerval *itp, it;

	itp = &p->p_realtimer;
	s = splclock();
	/*
	 * Clear any pending timer alarms.
	 */
	callout_stop(&p->p_realit_ch);
	timerclear(&itp->it_interval);
	if (timerisset(&itp->it_value) &&
	    timercmp(&itp->it_value, &time, >))
		timersub(&itp->it_value, &time, &itp->it_value);
	/*
	 * Return how many seconds were left (rounded up)
	 */
	retval[0] = itp->it_value.tv_sec;
	if (itp->it_value.tv_usec)
		retval[0]++;

	/*
	 * alarm(0) just resets the timer.
	 */
	if (SCARG(uap, secs) == 0) {
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

	if (timerisset(&it.it_value)) {
		/*
		 * Don't need to check hzto() return value, here.
		 * callout_reset() does it for us.
		 */
		timeradd(&it.it_value, &time, &it.it_value);
		callout_reset(&p->p_realit_ch, hzto(&it.it_value),
		    realitexpire, p);
	}
	p->p_realtimer = it;
	splx(s);

	return 0;
}

int
linux_sys_nice(p, v, retval)
	struct proc *p;
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
        return sys_setpriority(p, &bsa, retval);
}

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
linux_sys_readdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_readdir_args /* {
		syscallarg(int) fd;
		syscallarg(struct linux_dirent *) dent;
		syscallarg(unsigned int) count;
	} */ *uap = v;

	SCARG(uap, count) = 1;
	return linux_sys_getdents(p, uap, retval);
}

/*
 * I wonder why Linux has gettimeofday() _and_ time().. Still, we
 * need to deal with it.
 */
int
linux_sys_time(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
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
linux_sys_utime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_utime_args /* {
		syscallarg(const char *) path;
		syscallarg(struct linux_utimbuf *)times;
	} */ *uap = v;
	caddr_t sg;
	int error;
	struct sys_utimes_args ua;
	struct timeval tv[2], *tvp;
	struct linux_utimbuf lut;

	sg = stackgap_init(p->p_emul);
	LINUX_CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&ua, path) = SCARG(uap, path);

	if (SCARG(uap, times) != NULL) {
		if ((error = copyin(SCARG(uap, times), &lut, sizeof lut)))
			return error;
		tv[0].tv_usec = tv[1].tv_usec = 0;
		tv[0].tv_sec = lut.l_actime;
		tv[1].tv_sec = lut.l_modtime;
		tvp = (struct timeval *) stackgap_alloc(&sg, sizeof(tv));
		if ((error = copyout(tv, tvp, sizeof tv)))
			return error;
		SCARG(&ua, tptr) = tvp;
	}
	else
		SCARG(&ua, tptr) = NULL;

	return sys_utimes(p, &ua, retval);
}

/*
 * waitpid(2). Passed on to the NetBSD call, surrounded by code to
 * reserve some space for a NetBSD-style wait status, and converting
 * it to what Linux wants.
 */
int
linux_sys_waitpid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_waitpid_args /* {
		syscallarg(int) pid;
		syscallarg(int *) status;
		syscallarg(int) options;
	} */ *uap = v;
	struct sys_wait4_args w4a;
	int error, *status, tstat;
	caddr_t sg;

	if (SCARG(uap, status) != NULL) {
		sg = stackgap_init(p->p_emul);
		status = (int *) stackgap_alloc(&sg, sizeof status);
	} else
		status = NULL;

	SCARG(&w4a, pid) = SCARG(uap, pid);
	SCARG(&w4a, status) = status;
	SCARG(&w4a, options) = SCARG(uap, options);
	SCARG(&w4a, rusage) = NULL;

	if ((error = sys_wait4(p, &w4a, retval)))
		return error;

	sigdelset(&p->p_siglist, SIGCHLD);

	if (status != NULL) {
		if ((error = copyin(status, &tstat, sizeof tstat)))
			return error;

		bsd_to_linux_wstat(&tstat);
		return copyout(&tstat, SCARG(uap, status), sizeof tstat);
	}

	return 0;
}

int
linux_sys_setresgid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_setresgid_args /* {
		syscallarg(gid_t) rgid;
		syscallarg(gid_t) egid;
		syscallarg(gid_t) sgid;
	} */ *uap = v;
	struct pcred *pc = p->p_cred;
	gid_t rgid, egid, sgid;
	int error;

	rgid = SCARG(uap, rgid);
	egid = SCARG(uap, egid);
	sgid = SCARG(uap, sgid);

	/*
	 * Note: These checks are a little different than the NetBSD
	 * setregid(2) call performs.  This precisely follows the
	 * behavior of the Linux kernel.
	 */
	if (rgid != (gid_t)-1 &&
	    rgid != pc->p_rgid &&
	    rgid != pc->pc_ucred->cr_gid &&
	    rgid != pc->p_svgid &&
	    (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);

	if (egid != (gid_t)-1 &&
	    egid != pc->p_rgid &&
	    egid != pc->pc_ucred->cr_gid &&
	    egid != pc->p_svgid &&
	    (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);

	if (sgid != (gid_t)-1 &&
	    sgid != pc->p_rgid &&
	    sgid != pc->pc_ucred->cr_gid &&
	    sgid != pc->p_svgid &&
	    (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);

	/*
	 * Now assign the real, effective, and saved GIDs.
	 * Note that Linux, unlike NetBSD in setregid(2), does not
	 * set the saved UID in this call unless the user specifies
	 * it.
	 */
	if (rgid != (gid_t)-1)
		pc->p_rgid = rgid;

	if (egid != (gid_t)-1) {
		pc->pc_ucred = crcopy(pc->pc_ucred);
		pc->pc_ucred->cr_gid = egid;
	}

	if (sgid != (gid_t)-1)
		pc->p_svgid = sgid;

	if (rgid != (gid_t)-1 && egid != (gid_t)-1 && sgid != (gid_t)-1)
		p->p_flag |= P_SUGID;
	return (0);
}

int
linux_sys_getresgid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_getresgid_args /* {
		syscallarg(gid_t *) rgid;
		syscallarg(gid_t *) egid;
		syscallarg(gid_t *) sgid;
	} */ *uap = v;
	struct pcred *pc = p->p_cred;
	int error;

	/*
	 * Linux copies these values out to userspace like so:
	 *
	 *	1. Copy out rgid.
	 *	2. If that succeeds, copy out egid.
	 *	3. If both of those succeed, copy out sgid.
	 */
	if ((error = copyout(&pc->p_rgid, SCARG(uap, rgid),
			     sizeof(gid_t))) != 0)
		return (error);

	if ((error = copyout(&pc->pc_ucred->cr_uid, SCARG(uap, egid),
			     sizeof(gid_t))) != 0)
		return (error);

	return (copyout(&pc->p_svgid, SCARG(uap, sgid), sizeof(gid_t)));
}

/*
 * I wonder why Linux has settimeofday() _and_ stime().. Still, we
 * need to deal with it.
 */
int
linux_sys_stime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_time_args /* {
		linux_time_t *t;
	} */ *uap = v;
	struct timeval atv;
	linux_time_t tt;
	int error;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);

	if ((error = copyin(&tt, SCARG(uap, t), sizeof tt)) != 0)
		return error;

	atv.tv_sec = tt;
	atv.tv_usec = 0;

	if ((error = settime(&atv)))
		return (error);

	return 0;
}
