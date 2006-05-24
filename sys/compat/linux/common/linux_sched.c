/*	$NetBSD: linux_sched.c,v 1.29.12.1 2006/05/24 15:48:27 tron Exp $	*/

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
 * Linux compatibility module. Try to deal with scheduler related syscalls.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_sched.c,v 1.29.12.1 2006/05/24 15:48:27 tron Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/malloc.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/wait.h>
#include <sys/kauth.h>

#include <machine/cpu.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_machdep.h> /* For LINUX_NPTL */
#include <compat/linux/common/linux_emuldata.h>

#include <compat/linux/linux_syscallargs.h>

#include <compat/linux/common/linux_sched.h>

int
linux_sys_clone(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_clone_args /* {
		syscallarg(int) flags;
		syscallarg(void *) stack;
#ifdef LINUX_NPTL
		syscallarg(void *) parent_tidptr;
		syscallarg(void *) child_tidptr;
#endif
	} */ *uap = v;
	int flags, sig;
	int error;
#ifdef LINUX_NPTL
	struct linux_emuldata *led;
#endif

	/*
	 * We don't support the Linux CLONE_PID or CLONE_PTRACE flags.
	 */
	if (SCARG(uap, flags) & (LINUX_CLONE_PID|LINUX_CLONE_PTRACE))
		return (EINVAL);

	/*
	 * Thread group implies shared signals. Shared signals
	 * imply shared VM. This matches what Linux kernel does.
	 */
	if (SCARG(uap, flags) & LINUX_CLONE_THREAD
	    && (SCARG(uap, flags) & LINUX_CLONE_SIGHAND) == 0)
		return (EINVAL);
	if (SCARG(uap, flags) & LINUX_CLONE_SIGHAND
	    && (SCARG(uap, flags) & LINUX_CLONE_VM) == 0)
		return (EINVAL);

	flags = 0;

	if (SCARG(uap, flags) & LINUX_CLONE_VM)
		flags |= FORK_SHAREVM;
	if (SCARG(uap, flags) & LINUX_CLONE_FS)
		flags |= FORK_SHARECWD;
	if (SCARG(uap, flags) & LINUX_CLONE_FILES)
		flags |= FORK_SHAREFILES;
	if (SCARG(uap, flags) & LINUX_CLONE_SIGHAND)
		flags |= FORK_SHARESIGS;
	if (SCARG(uap, flags) & LINUX_CLONE_VFORK)
		flags |= FORK_PPWAIT;

	/* Thread should not issue a SIGCHLD on termination */
	if (SCARG(uap, flags) & LINUX_CLONE_THREAD) {
		sig = 0;
	} else {
		sig = SCARG(uap, flags) & LINUX_CLONE_CSIGNAL;
		if (sig < 0 || sig >= LINUX__NSIG)
			return (EINVAL);
		sig = linux_to_native_signo[sig];
	}

#ifdef LINUX_NPTL
	led = (struct linux_emuldata *)l->l_proc->p_emuldata;

	if (SCARG(uap, flags) & LINUX_CLONE_PARENT_SETTID) {
		if (SCARG(uap, parent_tidptr) == NULL) {
			printf("linux_sys_clone: NULL parent_tidptr\n");
			return EINVAL;
		}

		if ((error = copyout(&l->l_proc->p_pid,
		    SCARG(uap, parent_tidptr), 
		    sizeof(l->l_proc->p_pid))) != 0)
			return error;
	}

	/* CLONE_CHILD_CLEARTID: TID clear in the child on exit() */
	if (SCARG(uap, flags) & LINUX_CLONE_CHILD_CLEARTID)
		led->child_clear_tid = SCARG(uap, child_tidptr);
	else	
		led->child_clear_tid = NULL;

	/* CLONE_CHILD_SETTID: TID set in the child on clone() */
	if (SCARG(uap, flags) & LINUX_CLONE_CHILD_SETTID)
		led->child_set_tid = SCARG(uap, child_tidptr);
	else
		led->child_set_tid = NULL;

	/* CLONE_SETTLS: new Thread Local Storage in the child */
	if (SCARG(uap, flags) & LINUX_CLONE_SETTLS)
		led->set_tls = linux_get_newtls(l);
	else
		led->set_tls = 0;
#endif /* LINUX_NPTL */
	/*
	 * Note that Linux does not provide a portable way of specifying
	 * the stack area; the caller must know if the stack grows up
	 * or down.  So, we pass a stack size of 0, so that the code
	 * that makes this adjustment is a noop.
	 */
	if ((error = fork1(l, flags, sig, SCARG(uap, stack), 0,
	    NULL, NULL, retval, NULL)) != 0)
		return error;

	return 0;
}

int
linux_sys_sched_setparam(cl, v, retval)
	struct lwp *cl;
	void *v;
	register_t *retval;
{
	struct linux_sys_sched_setparam_args /* {
		syscallarg(linux_pid_t) pid;
		syscallarg(const struct linux_sched_param *) sp;
	} */ *uap = v;
	struct proc *cp = cl->l_proc;
	int error;
	struct linux_sched_param lp;
	struct proc *p;

/*
 * We only check for valid parameters and return afterwards.
 */

	if (SCARG(uap, pid) < 0 || SCARG(uap, sp) == NULL)
		return EINVAL;

	error = copyin(SCARG(uap, sp), &lp, sizeof(lp));
	if (error)
		return error;

	if (SCARG(uap, pid) != 0) {
		kauth_cred_t pc = cp->p_cred;

		if ((p = pfind(SCARG(uap, pid))) == NULL)
			return ESRCH;
		if (!(cp == p ||
		      kauth_cred_geteuid(pc) == 0 ||
		      kauth_cred_getuid(pc) == kauth_cred_getuid(p->p_cred) ||
		      kauth_cred_geteuid(pc) == kauth_cred_getuid(p->p_cred) ||
		      kauth_cred_getuid(pc) == kauth_cred_geteuid(p->p_cred) ||
		      kauth_cred_geteuid(pc) == kauth_cred_geteuid(p->p_cred)))
			return EPERM;
	}

	return 0;
}

int
linux_sys_sched_getparam(cl, v, retval)
	struct lwp *cl;
	void *v;
	register_t *retval;
{
	struct linux_sys_sched_getparam_args /* {
		syscallarg(linux_pid_t) pid;
		syscallarg(struct linux_sched_param *) sp;
	} */ *uap = v;
	struct proc *cp = cl->l_proc;
	struct proc *p;
	struct linux_sched_param lp;

/*
 * We only check for valid parameters and return a dummy priority afterwards.
 */
	if (SCARG(uap, pid) < 0 || SCARG(uap, sp) == NULL)
		return EINVAL;

	if (SCARG(uap, pid) != 0) {
		kauth_cred_t pc = cp->p_cred;

		if ((p = pfind(SCARG(uap, pid))) == NULL)
			return ESRCH;
		if (!(cp == p ||
		      kauth_cred_geteuid(pc) == 0 ||
		      kauth_cred_getuid(pc) == kauth_cred_getuid(p->p_cred) ||
		      kauth_cred_geteuid(pc) == kauth_cred_getuid(p->p_cred) ||
		      kauth_cred_getuid(pc) == kauth_cred_geteuid(p->p_cred) ||
		      kauth_cred_geteuid(pc) == kauth_cred_geteuid(p->p_cred)))
			return EPERM;
	}

	lp.sched_priority = 0;
	return copyout(&lp, SCARG(uap, sp), sizeof(lp));
}

int
linux_sys_sched_setscheduler(cl, v, retval)
	struct lwp *cl;
	void *v;
	register_t *retval;
{
	struct linux_sys_sched_setscheduler_args /* {
		syscallarg(linux_pid_t) pid;
		syscallarg(int) policy;
		syscallarg(cont struct linux_sched_scheduler *) sp;
	} */ *uap = v;
	struct proc *cp = cl->l_proc;
	int error;
	struct linux_sched_param lp;
	struct proc *p;

/*
 * We only check for valid parameters and return afterwards.
 */

	if (SCARG(uap, pid) < 0 || SCARG(uap, sp) == NULL)
		return EINVAL;

	error = copyin(SCARG(uap, sp), &lp, sizeof(lp));
	if (error)
		return error;

	if (SCARG(uap, pid) != 0) {
		kauth_cred_t pc = cp->p_cred;

		if ((p = pfind(SCARG(uap, pid))) == NULL)
			return ESRCH;
		if (!(cp == p ||
		      kauth_cred_geteuid(pc) == 0 ||
		      kauth_cred_getuid(pc) == kauth_cred_getuid(p->p_cred) ||
		      kauth_cred_geteuid(pc) == kauth_cred_getuid(p->p_cred) ||
		      kauth_cred_getuid(pc) == kauth_cred_geteuid(p->p_cred) ||
		      kauth_cred_geteuid(pc) == kauth_cred_geteuid(p->p_cred)))
			return EPERM;
	}

/*
 * We can't emulate anything put the default scheduling policy.
 */
	if (SCARG(uap, policy) != LINUX_SCHED_OTHER || lp.sched_priority != 0)
		return EINVAL;

	return 0;
}

int
linux_sys_sched_getscheduler(cl, v, retval)
	struct lwp *cl;
	void *v;
	register_t *retval;
{
	struct linux_sys_sched_getscheduler_args /* {
		syscallarg(linux_pid_t) pid;
	} */ *uap = v;
	struct proc *cp = cl->l_proc;
	struct proc *p;

	*retval = -1;
/*
 * We only check for valid parameters and return afterwards.
 */

	if (SCARG(uap, pid) != 0) {
		kauth_cred_t pc = cp->p_cred;

		if ((p = pfind(SCARG(uap, pid))) == NULL)
			return ESRCH;
		if (!(cp == p ||
		      kauth_cred_geteuid(pc) == 0 ||
		      kauth_cred_getuid(pc) == kauth_cred_getuid(p->p_cred) ||
		      kauth_cred_geteuid(pc) == kauth_cred_getuid(p->p_cred) ||
		      kauth_cred_getuid(pc) == kauth_cred_geteuid(p->p_cred) ||
		      kauth_cred_geteuid(pc) == kauth_cred_geteuid(p->p_cred)))
			return EPERM;
	}

/*
 * We can't emulate anything put the default scheduling policy.
 */
	*retval = LINUX_SCHED_OTHER;
	return 0;
}

int
linux_sys_sched_yield(cl, v, retval)
	struct lwp *cl;
	void *v;
	register_t *retval;
{

	yield();
	return 0;
}

int
linux_sys_sched_get_priority_max(cl, v, retval)
	struct lwp *cl;
	void *v;
	register_t *retval;
{
	struct linux_sys_sched_get_priority_max_args /* {
		syscallarg(int) policy;
	} */ *uap = v;

/*
 * We can't emulate anything put the default scheduling policy.
 */
	if (SCARG(uap, policy) != LINUX_SCHED_OTHER) {
		*retval = -1;
		return EINVAL;
	}

	*retval = 0;
	return 0;
}

int
linux_sys_sched_get_priority_min(cl, v, retval)
	struct lwp *cl;
	void *v;
	register_t *retval;
{
	struct linux_sys_sched_get_priority_min_args /* {
		syscallarg(int) policy;
	} */ *uap = v;

/*
 * We can't emulate anything put the default scheduling policy.
 */
	if (SCARG(uap, policy) != LINUX_SCHED_OTHER) {
		*retval = -1;
		return EINVAL;
	}

	*retval = 0;
	return 0;
}

#ifndef __m68k__
/* Present on everything but m68k */
int
linux_sys_exit_group(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_exit_group_args /* {
		syscallarg(int) error_code;
	} */ *uap = v;

	/*
	 * XXX The calling thread is supposed to kill all threads
	 * in the same thread group (i.e. all threads created
	 * via clone(2) with CLONE_THREAD flag set). This appears
	 * to not be used yet, so the thread group handling
	 * is currently not implemented.
	 */

	exit1(l, W_EXITCODE(SCARG(uap, error_code), 0));
	/* NOTREACHED */
	return 0;
}
#endif /* !__m68k__ */

#ifdef LINUX_NPTL
int
linux_sys_set_tid_address(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_set_tid_address_args /* {
		syscallarg(int *) tidptr;
	} */ *uap = v;
	struct linux_emuldata *led;

	led = (struct linux_emuldata *)l->l_proc->p_emuldata;
	led->clear_tid = SCARG(uap, tid);

	*retval = l->l_proc->p_pid;

	return 0;
}

/* ARGUSED1 */
int
linux_sys_gettid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	*retval = l->l_proc->p_pid;
	return 0;
}

int
linux_sys_sched_getaffinity(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_sched_getaffinity_args /* {
		syscallarg(pid_t) pid;
		syscallarg(unsigned int) len;
		syscallarg(unsigned long *) mask;
	} */ *uap = v;
	int error;
	int ret;
	int ncpu;
	int name[2];
	size_t sz;
	char *data;
	int *retp;

	if (SCARG(uap, mask) == NULL)
		return EINVAL;

	if (SCARG(uap, len) < sizeof(int))
		return EINVAL;

	if (pfind(SCARG(uap, pid)) == NULL)
		return ESRCH;

	/* 
	 * return the actual number of CPU, tag all of them as available 
	 * The result is a mask, the first CPU being in the least significant
	 * bit.
	 */
	name[0] = CTL_HW;
	name[1] = HW_NCPU;
	sz = sizeof(ncpu);

	if ((error = old_sysctl(&name[0], 2, &ncpu, &sz, NULL, 0, NULL)) != 0)
		return error;

	ret = (1 << ncpu) - 1;

	data = malloc(SCARG(uap, len), M_TEMP, M_WAITOK|M_ZERO);
	retp = (int *)&data[SCARG(uap, len) - sizeof(ret)];
	*retp = ret;

	if ((error = copyout(data, SCARG(uap, mask), SCARG(uap, len))) != 0)
		return error;

	free(data, M_TEMP);

	return 0;

}

int
linux_sys_sched_setaffinity(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_sched_setaffinity_args /* {
		syscallarg(pid_t) pid;
		syscallarg(unsigned int) len;
		syscallarg(unsigned long *) mask;
	} */ *uap = v;

	if (pfind(SCARG(uap, pid)) == NULL)
		return ESRCH;

	/* Let's ignore it */
#ifdef DEBUG_LINUX
	printf("linux_sys_sched_setaffinity\n");
#endif
	return 0;
};
#endif /* LINUX_NPTL */
