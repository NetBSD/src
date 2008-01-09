/*	$NetBSD: freebsd_sched.c,v 1.9.16.2 2008/01/09 01:50:42 matt Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center; by Matthias Scheler.
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

/*
 * FreeBSD compatibility module. Try to deal with scheduler related syscalls.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: freebsd_sched.c,v 1.9.16.2 2008/01/09 01:50:42 matt Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/syscallargs.h>
#include <sys/kauth.h>

#include <sys/cpu.h>

#include <compat/freebsd/freebsd_syscallargs.h>
#include <compat/freebsd/freebsd_sched.h>

int
freebsd_sys_yield(struct lwp *l, const void *v, register_t *retval)
{

	yield();
	return 0;
}

/*
 * Verify access to the target process.
 * If we did any work this would need to return a reference to the
 * proc and have the mutex still held.
 * But we don't do anything, so it is ok.
 */
static int
check_proc_access(struct lwp *l, pid_t pid)
{
	struct proc *p;
	kauth_cred_t pc;

	if (pid == 0)
		return 0;
	if (pid < 0)
		return EINVAL;

	mutex_enter(&proclist_lock);

	p = p_find(pid, PFIND_LOCKED | PFIND_UNLOCK_FAIL);
	if (p == NULL)
		return ESRCH;

	pc = l->l_cred;

	if (!(l->l_proc == p ||
	    kauth_cred_getuid(pc) == kauth_cred_getuid(p->p_cred) ||
	    kauth_cred_geteuid(pc) == kauth_cred_getuid(p->p_cred) ||
	    kauth_cred_getuid(pc) == kauth_cred_geteuid(p->p_cred) ||
	    kauth_cred_geteuid(pc) == kauth_cred_geteuid(p->p_cred))) {
		mutex_exit(&proclist_lock);
		if (kauth_authorize_generic(pc, KAUTH_GENERIC_ISSUSER, NULL) != 0)
		    return EPERM;
	} else
		mutex_exit(&proclist_lock);

	return 0;
}

int
freebsd_sys_sched_setparam(struct lwp *l, const struct freebsd_sys_sched_setparam_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(const struct freebsd_sched_param *) sp;
	} */
	int error;
	struct freebsd_sched_param lp;

	/*
	 * We only check for valid parameters and return afterwards.
	 */
	if (SCARG(uap, sp) == NULL)
		return EINVAL;

	error = copyin(SCARG(uap, sp), &lp, sizeof(lp));
	if (error)
		return error;

	error = check_proc_access(l, SCARG(uap, pid));
	if (error)
		return error;

	return 0;
}

int
freebsd_sys_sched_getparam(struct lwp *l, const struct freebsd_sys_sched_getparam_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(struct freebsd_sched_param *) sp;
	} */
	struct freebsd_sched_param lp;
	int error;

	/*
	 * We only check for valid parameters and return a dummy
	 * priority afterwards.
	 */
	if (SCARG(uap, sp) == NULL)
		return EINVAL;

	error = check_proc_access(l, SCARG(uap, pid));
	if (error)
		return error;

	lp.sched_priority = 0;
	return copyout(&lp, SCARG(uap, sp), sizeof(lp));
}

int
freebsd_sys_sched_setscheduler(struct lwp *l, const struct freebsd_sys_sched_setscheduler_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(int) policy;
		syscallarg(cont struct freebsd_sched_scheduler *) sp;
	} */
	int error;
	struct freebsd_sched_param lp;

	/*
	 * We only check for valid parameters and return afterwards.
	 */
	if (SCARG(uap, sp) == NULL)
		return EINVAL;

	error = copyin(SCARG(uap, sp), &lp, sizeof(lp));
	if (error)
		return error;

	error = check_proc_access(l, SCARG(uap, pid));
	if (error)
		return error;

	/*
	 * We can't emulate anything put the default scheduling policy.
	 */
	if (SCARG(uap, policy) != FREEBSD_SCHED_OTHER || lp.sched_priority != 0)
		return EINVAL;

	return 0;
}

int
freebsd_sys_sched_getscheduler(struct lwp *l, const struct freebsd_sys_sched_getscheduler_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
	} */
	int error;

	*retval = -1;

	/*
	 * We only check for valid parameters and return afterwards.
	 */

	error = check_proc_access(l, SCARG(uap, pid));
	if (error)
		return error;

	/*
	 * We can't emulate anything put the default scheduling policy.
	 */
	*retval = FREEBSD_SCHED_OTHER;
	return 0;
}

int
freebsd_sys_sched_yield(struct lwp *l, const void *v, register_t *retval)
{

	yield();
	return 0;
}

int
freebsd_sys_sched_get_priority_max(struct lwp *l, const struct freebsd_sys_sched_get_priority_max_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) policy;
	} */

	/*
	 * We can't emulate anything put the default scheduling policy.
	 */
	if (SCARG(uap, policy) != FREEBSD_SCHED_OTHER) {
		*retval = -1;
		return EINVAL;
	}

	*retval = 0;
	return 0;
}

int
freebsd_sys_sched_get_priority_min(struct lwp *l, const struct freebsd_sys_sched_get_priority_min_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) policy;
	} */

	/*
	 * We can't emulate anything put the default scheduling policy.
	 */
	if (SCARG(uap, policy) != FREEBSD_SCHED_OTHER) {
		*retval = -1;
		return EINVAL;
	}

	*retval = 0;
	return 0;
}
