/*	$NetBSD: kern_fork.c,v 1.111 2003/09/16 12:06:07 christos Exp $	*/

/*-
 * Copyright (c) 1999, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_fork.c	8.8 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_fork.c,v 1.111 2003/09/16 12:06:07 christos Exp $");

#include "opt_ktrace.h"
#include "opt_systrace.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/ras.h>
#include <sys/resourcevar.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/acct.h>
#include <sys/ktrace.h>
#include <sys/vmmeter.h>
#include <sys/sched.h>
#include <sys/signalvar.h>
#include <sys/systrace.h>

#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>


int	nprocs = 1;		/* process 0 */

/*
 * Number of ticks to sleep if fork() would fail due to process hitting
 * limits. Exported in miliseconds to userland via sysctl.
 */
int	forkfsleep = 0;

/*ARGSUSED*/
int
sys_fork(struct lwp *l, void *v, register_t *retval)
{

	return (fork1(l, 0, SIGCHLD, NULL, 0, NULL, NULL, retval, NULL));
}

/*
 * vfork(2) system call compatible with 4.4BSD (i.e. BSD with Mach VM).
 * Address space is not shared, but parent is blocked until child exit.
 */
/*ARGSUSED*/
int
sys_vfork(struct lwp *l, void *v, register_t *retval)
{

	return (fork1(l, FORK_PPWAIT, SIGCHLD, NULL, 0, NULL, NULL,
	    retval, NULL));
}

/*
 * New vfork(2) system call for NetBSD, which implements original 3BSD vfork(2)
 * semantics.  Address space is shared, and parent is blocked until child exit.
 */
/*ARGSUSED*/
int
sys___vfork14(struct lwp *l, void *v, register_t *retval)
{

	return (fork1(l, FORK_PPWAIT|FORK_SHAREVM, SIGCHLD, NULL, 0,
	    NULL, NULL, retval, NULL));
}

/*
 * Linux-compatible __clone(2) system call.
 */
int
sys___clone(struct lwp *l, void *v, register_t *retval)
{
	struct sys___clone_args /* {
		syscallarg(int) flags;
		syscallarg(void *) stack;
	} */ *uap = v;
	int flags, sig;

	/*
	 * We don't support the CLONE_PID or CLONE_PTRACE flags.
	 */
	if (SCARG(uap, flags) & (CLONE_PID|CLONE_PTRACE))
		return (EINVAL);

	flags = 0;

	if (SCARG(uap, flags) & CLONE_VM)
		flags |= FORK_SHAREVM;
	if (SCARG(uap, flags) & CLONE_FS)
		flags |= FORK_SHARECWD;
	if (SCARG(uap, flags) & CLONE_FILES)
		flags |= FORK_SHAREFILES;
	if (SCARG(uap, flags) & CLONE_SIGHAND)
		flags |= FORK_SHARESIGS;
	if (SCARG(uap, flags) & CLONE_VFORK)
		flags |= FORK_PPWAIT;

	sig = SCARG(uap, flags) & CLONE_CSIGNAL;
	if (sig < 0 || sig >= _NSIG)
		return (EINVAL);

	/*
	 * Note that the Linux API does not provide a portable way of
	 * specifying the stack area; the caller must know if the stack
	 * grows up or down.  So, we pass a stack size of 0, so that the
	 * code that makes this adjustment is a noop.
	 */
	return (fork1(l, flags, sig, SCARG(uap, stack), 0,
	    NULL, NULL, retval, NULL));
}

/* print the 'table full' message once per 10 seconds */
struct timeval fork_tfmrate = { 10, 0 };

int
fork1(struct lwp *l1, int flags, int exitsig, void *stack, size_t stacksize,
    void (*func)(void *), void *arg, register_t *retval,
    struct proc **rnewprocp)
{
	struct proc	*p1, *p2;
	uid_t		uid;
	struct lwp	*l2;
	int		count, s;
	vaddr_t		uaddr;
	boolean_t	inmem;

	/*
	 * Although process entries are dynamically created, we still keep
	 * a global limit on the maximum number we will create.  Don't allow
	 * a nonprivileged user to use the last few processes; don't let root
	 * exceed the limit. The variable nprocs is the current number of
	 * processes, maxproc is the limit.
	 */
	p1 = l1->l_proc;
	uid = p1->p_cred->p_ruid;
	if (__predict_false((nprocs >= maxproc - 5 && uid != 0) ||
			    nprocs >= maxproc)) {
		static struct timeval lasttfm;

		if (ratecheck(&lasttfm, &fork_tfmrate))
			tablefull("proc", "increase kern.maxproc or NPROC");
		if (forkfsleep)
			(void)tsleep(&nprocs, PUSER, "forkmx", forkfsleep);
		return (EAGAIN);
	}
	nprocs++;

	/*
	 * Increment the count of procs running with this uid. Don't allow
	 * a nonprivileged user to exceed their current limit.
	 */
	count = chgproccnt(uid, 1);
	if (__predict_false(uid != 0 && count >
			    p1->p_rlimit[RLIMIT_NPROC].rlim_cur)) {
		(void)chgproccnt(uid, -1);
		nprocs--;
		if (forkfsleep)
			(void)tsleep(&nprocs, PUSER, "forkulim", forkfsleep);
		return (EAGAIN);
	}

	/*
	 * Allocate virtual address space for the U-area now, while it
	 * is still easy to abort the fork operation if we're out of
	 * kernel virtual address space.  The actual U-area pages will
	 * be allocated and wired in uvm_fork() if needed.
	 */

	inmem = uvm_uarea_alloc(&uaddr);
	if (__predict_false(uaddr == 0)) {
		(void)chgproccnt(uid, -1);
		nprocs--;
		return (ENOMEM);
	}

	/*
	 * We are now committed to the fork.  From here on, we may
	 * block on resources, but resource allocation may NOT fail.
	 */

	/* Allocate new proc. */
	p2 = proc_alloc();

	/*
	 * Make a proc table entry for the new process.
	 * Start by zeroing the section of proc that is zero-initialized,
	 * then copy the section that is copied directly from the parent.
	 */
	memset(&p2->p_startzero, 0,
	    (unsigned) ((caddr_t)&p2->p_endzero - (caddr_t)&p2->p_startzero));
	memcpy(&p2->p_startcopy, &p1->p_startcopy,
	    (unsigned) ((caddr_t)&p2->p_endcopy - (caddr_t)&p2->p_startcopy));

	simple_lock_init(&p2->p_sigctx.ps_silock);
	CIRCLEQ_INIT(&p2->p_sigctx.ps_siginfo);
	simple_lock_init(&p2->p_lwplock);
	LIST_INIT(&p2->p_lwps);

	/*
	 * Duplicate sub-structures as needed.
	 * Increase reference counts on shared objects.
	 * The p_stats and p_sigacts substructs are set in uvm_fork().
	 */
	p2->p_flag = (p1->p_flag & P_SUGID);
	p2->p_emul = p1->p_emul;
	p2->p_execsw = p1->p_execsw;

	if (p1->p_flag & P_PROFIL)
		startprofclock(p2);
	p2->p_cred = pool_get(&pcred_pool, PR_WAITOK);
	memcpy(p2->p_cred, p1->p_cred, sizeof(*p2->p_cred));
	p2->p_cred->p_refcnt = 1;
	crhold(p1->p_ucred);

	LIST_INIT(&p2->p_raslist);
	p2->p_nras = 0;
	simple_lock_init(&p2->p_raslock);
#if defined(__HAVE_RAS)
	ras_fork(p1, p2);
#endif

	/* bump references to the text vnode (for procfs) */
	p2->p_textvp = p1->p_textvp;
	if (p2->p_textvp)
		VREF(p2->p_textvp);

	if (flags & FORK_SHAREFILES)
		fdshare(p1, p2);
	else if (flags & FORK_CLEANFILES)
		p2->p_fd = fdinit(p1);
	else
		p2->p_fd = fdcopy(p1);

	if (flags & FORK_SHARECWD)
		cwdshare(p1, p2);
	else
		p2->p_cwdi = cwdinit(p1);

	/*
	 * If p_limit is still copy-on-write, bump refcnt,
	 * otherwise get a copy that won't be modified.
	 * (If PL_SHAREMOD is clear, the structure is shared
	 * copy-on-write.)
	 */
	if (p1->p_limit->p_lflags & PL_SHAREMOD)
		p2->p_limit = limcopy(p1->p_limit);
	else {
		p2->p_limit = p1->p_limit;
		p2->p_limit->p_refcnt++;
	}

	/* Inherit STOPFORK and STOPEXEC flags */
	p2->p_flag |= p1->p_flag & (P_STOPFORK | P_STOPEXEC);

	if (p1->p_session->s_ttyvp != NULL && p1->p_flag & P_CONTROLT)
		p2->p_flag |= P_CONTROLT;
	if (flags & FORK_PPWAIT)
		p2->p_flag |= P_PPWAIT;
	p2->p_pptr = (flags & FORK_NOWAIT) ? initproc : p1;
	LIST_INIT(&p2->p_children);

	s = proclist_lock_write();
	LIST_INSERT_AFTER(p1, p2, p_pglist);
	LIST_INSERT_HEAD(&p2->p_pptr->p_children, p2, p_sibling);
	proclist_unlock_write(s);

#ifdef KTRACE
	/*
	 * Copy traceflag and tracefile if enabled.
	 * If not inherited, these were zeroed above.
	 */
	if (p1->p_traceflag & KTRFAC_INHERIT) {
		p2->p_traceflag = p1->p_traceflag;
		if ((p2->p_tracep = p1->p_tracep) != NULL)
			ktradref(p2);
	}
#endif

	scheduler_fork_hook(p1, p2);

	/*
	 * Create signal actions for the child process.
	 */
	sigactsinit(p2, p1, flags & FORK_SHARESIGS);

	/*
	 * p_stats. 
	 * Copy parts of p_stats, and zero out the rest.
	 */
	p2->p_stats = pstatscopy(p1->p_stats);

	/*
	 * If emulation has process fork hook, call it now.
	 */
	if (p2->p_emul->e_proc_fork)
		(*p2->p_emul->e_proc_fork)(p2, p1);

	/*
	 * ...and finally, any other random fork hooks that subsystems
	 * might have registered.
	 */
	doforkhooks(p2, p1);

	/*
	 * This begins the section where we must prevent the parent
	 * from being swapped.
	 */
	PHOLD(l1);

	uvm_proc_fork(p1, p2, (flags & FORK_SHAREVM) ? TRUE : FALSE);

	/*
	 * Finish creating the child process.
	 * It will return through a different path later.
	 */
	newlwp(l1, p2, uaddr, inmem, 0, stack, stacksize, 
	    (func != NULL) ? func : child_return, 
	    arg, &l2);

	/* Now safe for scheduler to see child process */
	s = proclist_lock_write();
	p2->p_stat = SIDL;			/* protect against others */
	p2->p_exitsig = exitsig;		/* signal for parent on exit */
	LIST_INSERT_HEAD(&allproc, p2, p_list);
	proclist_unlock_write(s);

#ifdef SYSTRACE
	/* Tell systrace what's happening. */
	if (ISSET(p1->p_flag, P_SYSTRACE))
		systrace_sys_fork(p1, p2);
#endif

#ifdef __HAVE_SYSCALL_INTERN
	(*p2->p_emul->e_syscall_intern)(p2);
#endif

	/*
	 * Make child runnable, set start time, and add to run queue
	 * except if the parent requested the child to start in SSTOP state.
	 */
	SCHED_LOCK(s);
	p2->p_stats->p_start = time;
	p2->p_acflag = AFORK;
	p2->p_nrlwps = 1;
	if (p1->p_flag & P_STOPFORK) {
		p2->p_stat = SSTOP;
		l2->l_stat = LSSTOP;
	} else {
		p2->p_stat = SACTIVE;
		l2->l_stat = LSRUN;
		setrunqueue(l2);
	}
	SCHED_UNLOCK(s);

	/*
	 * Now can be swapped.
	 */
	PRELE(l1);

	/*
	 * Notify any interested parties about the new process.
	 */
	KNOTE(&p1->p_klist, NOTE_FORK | p2->p_pid);

	/*
	 * Update stats now that we know the fork was successful.
	 */
	uvmexp.forks++;
	if (flags & FORK_PPWAIT)
		uvmexp.forks_ppwait++;
	if (flags & FORK_SHAREVM)
		uvmexp.forks_sharevm++;

	/*
	 * Pass a pointer to the new process to the caller.
	 */
	if (rnewprocp != NULL)
		*rnewprocp = p2;

#ifdef KTRACE
	if (KTRPOINT(p2, KTR_EMUL))
		ktremul(p2);
#endif

	/*
	 * Preserve synchronization semantics of vfork.  If waiting for
	 * child to exec or exit, set P_PPWAIT on child, and sleep on our
	 * proc (in case of exit).
	 */
	if (flags & FORK_PPWAIT)
		while (p2->p_flag & P_PPWAIT)
			tsleep(p1, PWAIT, "ppwait", 0);

	/*
	 * Return child pid to parent process,
	 * marking us as parent via retval[1].
	 */
	if (retval != NULL) {
		retval[0] = p2->p_pid;
		retval[1] = 0;
	}

	return (0);
}

#if defined(MULTIPROCESSOR)
/*
 * XXX This is a slight hack to get newly-formed processes to
 * XXX acquire the kernel lock as soon as they run.
 */
void
proc_trampoline_mp(void)
{
	struct lwp *l;

	l = curlwp;

	SCHED_ASSERT_UNLOCKED();
	KERNEL_PROC_LOCK(l);
}
#endif
