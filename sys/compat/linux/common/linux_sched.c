/*	$NetBSD: linux_sched.c,v 1.19.2.6 2008/02/04 09:23:06 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_sched.c,v 1.19.2.6 2008/02/04 09:23:06 yamt Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/malloc.h>
#include <sys/syscallargs.h>
#include <sys/wait.h>
#include <sys/kauth.h>
#include <sys/ptrace.h>

#include <sys/cpu.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_machdep.h> /* For LINUX_NPTL */
#include <compat/linux/common/linux_emuldata.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>

#include <compat/linux/common/linux_sched.h>

int
linux_sys_clone(struct lwp *l, const struct linux_sys_clone_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) flags;
		syscallarg(void *) stack;
#ifdef LINUX_NPTL
		syscallarg(void *) parent_tidptr;
		syscallarg(void *) child_tidptr;
#endif
	} */
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

	sig = SCARG(uap, flags) & LINUX_CLONE_CSIGNAL;
	if (sig < 0 || sig >= LINUX__NSIG)
		return (EINVAL);
	sig = linux_to_native_signo[sig];

#ifdef LINUX_NPTL
	led = (struct linux_emuldata *)l->l_proc->p_emuldata;

	led->parent_tidptr = SCARG(uap, parent_tidptr);
	led->child_tidptr = SCARG(uap, child_tidptr);
	led->clone_flags = SCARG(uap, flags);
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
linux_sys_sched_setparam(struct lwp *l, const struct linux_sys_sched_setparam_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_pid_t) pid;
		syscallarg(const struct linux_sched_param *) sp;
	} */
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
		if ((p = pfind(SCARG(uap, pid))) == NULL)
			return ESRCH;

		if (l->l_proc != p &&
		    kauth_authorize_process(l->l_cred, KAUTH_PROCESS_SCHEDULER,
		    p, KAUTH_ARG(KAUTH_REQ_PROCESS_SCHEDULER_SETPARAM), NULL,
		    &lp) != 0)
			return EPERM;
	}

	return 0;
}

int
linux_sys_sched_getparam(struct lwp *l, const struct linux_sys_sched_getparam_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_pid_t) pid;
		syscallarg(struct linux_sched_param *) sp;
	} */
	struct proc *p;
	struct linux_sched_param lp;

/*
 * We only check for valid parameters and return a dummy priority afterwards.
 */
	if (SCARG(uap, pid) < 0 || SCARG(uap, sp) == NULL)
		return EINVAL;

	if (SCARG(uap, pid) != 0) {
		if ((p = pfind(SCARG(uap, pid))) == NULL)
			return ESRCH;

		if (l->l_proc != p &&
		    kauth_authorize_process(l->l_cred, KAUTH_PROCESS_SCHEDULER,
		    p, KAUTH_ARG(KAUTH_REQ_PROCESS_SCHEDULER_GETPARAM), NULL,
		    NULL) != 0)
			return EPERM;
	}

	lp.sched_priority = 0;
	return copyout(&lp, SCARG(uap, sp), sizeof(lp));
}

int
linux_sys_sched_setscheduler(struct lwp *l, const struct linux_sys_sched_setscheduler_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_pid_t) pid;
		syscallarg(int) policy;
		syscallarg(cont struct linux_sched_scheduler *) sp;
	} */
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
		if ((p = pfind(SCARG(uap, pid))) == NULL)
			return ESRCH;

		if (l->l_proc != p &&
		    kauth_authorize_process(l->l_cred, KAUTH_PROCESS_SCHEDULER,
		    p, KAUTH_ARG(KAUTH_REQ_PROCESS_SCHEDULER_SET),
		    KAUTH_ARG(SCARG(uap, policy)), &lp) != 0)
			return EPERM;
	}

	return 0;
/*
 * We can't emulate anything put the default scheduling policy.
 */
	if (SCARG(uap, policy) != LINUX_SCHED_OTHER || lp.sched_priority != 0)
		return EINVAL;

	return 0;
}

int
linux_sys_sched_getscheduler(struct lwp *l, const struct linux_sys_sched_getscheduler_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_pid_t) pid;
	} */
	struct proc *p;

	*retval = -1;
/*
 * We only check for valid parameters and return afterwards.
 */

	if (SCARG(uap, pid) != 0) {
		if ((p = pfind(SCARG(uap, pid))) == NULL)
			return ESRCH;

		if (l->l_proc != p &&
		    kauth_authorize_process(l->l_cred, KAUTH_PROCESS_SCHEDULER,
		    p, KAUTH_ARG(KAUTH_REQ_PROCESS_SCHEDULER_GET), NULL,
		    NULL) != 0)
			return EPERM;
	}

/*
 * We can't emulate anything put the default scheduling policy.
 */
	*retval = LINUX_SCHED_OTHER;
	return 0;
}

int
linux_sys_sched_yield(struct lwp *l, const void *v, register_t *retval)
{

	yield();
	return 0;
}

int
linux_sys_sched_get_priority_max(struct lwp *l, const struct linux_sys_sched_get_priority_max_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) policy;
	} */

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
linux_sys_sched_get_priority_min(struct lwp *l, const struct linux_sys_sched_get_priority_min_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) policy;
	} */

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
linux_sys_exit_group(struct lwp *l, const struct linux_sys_exit_group_args *uap, register_t *retval)
{
#ifdef LINUX_NPTL
	/* {
		syscallarg(int) error_code;
	} */
	struct proc *p = l->l_proc;
	struct linux_emuldata *led = p->p_emuldata;
	struct linux_emuldata *e;

	if (led->s->flags & LINUX_LES_USE_NPTL) {

#ifdef DEBUG_LINUX
		printf("%s:%d, led->s->refs = %d\n", __func__, __LINE__,
		    led->s->refs);
#endif

		/*
		 * The calling thread is supposed to kill all threads
		 * in the same thread group (i.e. all threads created
		 * via clone(2) with CLONE_THREAD flag set).
		 *
		 * If there is only one thread, things are quite simple
		 */
		if (led->s->refs == 1)
			return sys_exit(l, (const void *)uap, retval);

#ifdef DEBUG_LINUX
		printf("%s:%d\n", __func__, __LINE__);
#endif

		led->s->flags |= LINUX_LES_INEXITGROUP;
		led->s->xstat = W_EXITCODE(SCARG(uap, error_code), 0);

		/*
		 * Kill all threads in the group. The emulation exit hook takes
		 * care of hiding the zombies and reporting the exit code
		 * properly.
		 */
		mutex_enter(&proclist_mutex);
      		LIST_FOREACH(e, &led->s->threads, threads) {
			if (e->proc == p)
				continue;

#ifdef DEBUG_LINUX
			printf("%s: kill PID %d\n", __func__, e->proc->p_pid);
#endif
			psignal(e->proc, SIGKILL);
		}

		/* Now, kill ourselves */
		psignal(p, SIGKILL);
		mutex_exit(&proclist_mutex);

		return 0;

	}
#endif /* LINUX_NPTL */

	return sys_exit(l, (const void *)uap, retval);
}
#endif /* !__m68k__ */

#ifdef LINUX_NPTL
int
linux_sys_set_tid_address(struct lwp *l, const struct linux_sys_set_tid_address_args *uap, register_t *retval)
{
	/* {
		syscallarg(int *) tidptr;
	} */
	struct linux_emuldata *led;

	led = (struct linux_emuldata *)l->l_proc->p_emuldata;
	led->clear_tid = SCARG(uap, tid);

	led->s->flags |= LINUX_LES_USE_NPTL;

	*retval = l->l_proc->p_pid;

	return 0;
}

/* ARGUSED1 */
int
linux_sys_gettid(struct lwp *l, const void *v, register_t *retval)
{
	/* The Linux kernel does it exactly that way */
	*retval = l->l_proc->p_pid;
	return 0;
}

#ifdef LINUX_NPTL
/* ARGUSED1 */
int
linux_sys_getpid(struct lwp *l, const void *v, register_t *retval)
{
	struct linux_emuldata *led = l->l_proc->p_emuldata;

	if (led->s->flags & LINUX_LES_USE_NPTL) {
		/* The Linux kernel does it exactly that way */
		*retval = led->s->group_pid;
	} else {
		*retval = l->l_proc->p_pid;
	}

	return 0;
}

/* ARGUSED1 */
int
linux_sys_getppid(struct lwp *l, const void *v, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct linux_emuldata *led = p->p_emuldata;
	struct proc *glp;
	struct proc *pp;

	if (led->s->flags & LINUX_LES_USE_NPTL) {

		/* Find the thread group leader's parent */
		if ((glp = pfind(led->s->group_pid)) == NULL) {
			/* Maybe panic... */
			printf("linux_sys_getppid: missing group leader PID"
			    " %d\n", led->s->group_pid); 
			return -1;
		}
		pp = glp->p_pptr;

		/* If this is a Linux process too, return thread group PID */
		if (pp->p_emul == p->p_emul) {
			struct linux_emuldata *pled;

			pled = pp->p_emuldata;
			*retval = pled->s->group_pid;
		} else {
			*retval = pp->p_pid;
		}

	} else {
		*retval = p->p_pptr->p_pid;
	}

	return 0;
}
#endif /* LINUX_NPTL */

int
linux_sys_sched_getaffinity(struct lwp *l, const struct linux_sys_sched_getaffinity_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(unsigned int) len;
		syscallarg(unsigned long *) mask;
	} */
	int error;
	int ret;
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
linux_sys_sched_setaffinity(struct lwp *l, const struct linux_sys_sched_setaffinity_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(unsigned int) len;
		syscallarg(unsigned long *) mask;
	} */

	if (pfind(SCARG(uap, pid)) == NULL)
		return ESRCH;

	/* Let's ignore it */
#ifdef DEBUG_LINUX
	printf("linux_sys_sched_setaffinity\n");
#endif
	return 0;
};
#endif /* LINUX_NPTL */
