/*	$NetBSD: kern_exit.c,v 1.121 2003/08/20 14:04:00 yamt Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
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
 *	@(#)kern_exit.c	8.10 (Berkeley) 2/23/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_exit.c,v 1.121 2003/08/20 14:04:00 yamt Exp $");

#include "opt_ktrace.h"
#include "opt_perfctrs.h"
#include "opt_systrace.h"
#include "opt_sysv.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/kernel.h>
#include <sys/ktrace.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/resourcevar.h>
#if defined(PERFCTRS)
#include <sys/pmc.h>
#endif
#include <sys/ptrace.h>
#include <sys/acct.h>
#include <sys/filedesc.h>
#include <sys/ras.h>
#include <sys/signalvar.h>
#include <sys/sched.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/systrace.h>

#include <machine/cpu.h>

#include <uvm/uvm_extern.h>

#define DEBUG_EXIT

#ifdef DEBUG_EXIT
int debug_exit = 0;
#define DPRINTF(x) if (debug_exit) printf x
#else
#define DPRINTF(x)
#endif

static void lwp_exit_hook(struct lwp *, void *);

/*
 * exit --
 *	Death of process.
 */
int
sys_exit(struct lwp *l, void *v, register_t *retval)
{
	struct sys_exit_args /* {
		syscallarg(int)	rval;
	} */ *uap = v;

	/* Don't call exit1() multiple times in the same process.*/
	if (l->l_proc->p_flag & P_WEXIT)
		lwp_exit(l);

	exit1(l, W_EXITCODE(SCARG(uap, rval), 0));
	/* NOTREACHED */
	return (0);
}

/*
 * Exit: deallocate address space and other resources, change proc state
 * to zombie, and unlink proc from allproc and parent's lists.  Save exit
 * status and rusage for wait().  Check for child processes and orphan them.
 */
void
exit1(struct lwp *l, int rv)
{
	struct proc	*p, *q, *nq;
	int		s, sa;

	p = l->l_proc;

	if (__predict_false(p == initproc))
		panic("init died (signal %d, exit %d)",
		    WTERMSIG(rv), WEXITSTATUS(rv));

	p->p_flag |= P_WEXIT;

	DPRINTF(("exit1: %d.%d exiting.\n", p->p_pid, l->l_lid));
	/*
	 * Disable scheduler activation upcalls. 
	 * We're trying to get out of here. 
	 */
	sa = 0;
	if (p->p_sa != NULL) {

		l->l_flag &= ~L_SA;
#if 0
		p->p_flag &= ~P_SA;
#endif
		sa = 1;
	}

#ifdef PGINPROF
	vmsizmon();
#endif
	if (p->p_flag & P_PROFIL)
		stopprofclock(p);
	p->p_ru = pool_get(&rusage_pool, PR_WAITOK);
	/*
	 * If parent is waiting for us to exit or exec, P_PPWAIT is set; we
	 * wake up the parent early to avoid deadlock.
	 */
	if (p->p_flag & P_PPWAIT) {
		p->p_flag &= ~P_PPWAIT;
		wakeup((caddr_t)p->p_pptr);
	}
	sigfillset(&p->p_sigctx.ps_sigignore);
	sigemptyset(&p->p_sigctx.ps_siglist);
	p->p_sigctx.ps_sigcheck = 0;
	timers_free(p, TIMERS_ALL);

	if (sa || (p->p_nlwps > 1))
		exit_lwps(l);

#if defined(__HAVE_RAS)
	ras_purgeall(p);
#endif

	/*
	 * Close open files and release open-file table.
	 * This may block!
	 */
	fdfree(p);
	cwdfree(p);

	doexithooks(p);

	if (SESS_LEADER(p)) {
		struct session *sp = p->p_session;

		if (sp->s_ttyvp) {
			/*
			 * Controlling process.
			 * Signal foreground pgrp,
			 * drain controlling terminal
			 * and revoke access to controlling terminal.
			 */
			if (sp->s_ttyp->t_session == sp) {
				if (sp->s_ttyp->t_pgrp)
					pgsignal(sp->s_ttyp->t_pgrp, SIGHUP, 1);
				(void) ttywait(sp->s_ttyp);
				/*
				 * The tty could have been revoked
				 * if we blocked.
				 */
				if (sp->s_ttyvp)
					VOP_REVOKE(sp->s_ttyvp, REVOKEALL);
			}
			if (sp->s_ttyvp)
				vrele(sp->s_ttyvp);
			sp->s_ttyvp = NULL;
			/*
			 * s_ttyp is not zero'd; we use this to indicate
			 * that the session once had a controlling terminal.
			 * (for logging and informational purposes)
			 */
		}
		sp->s_leader = NULL;
	}
	fixjobc(p, p->p_pgrp, 0);
	(void)acct_process(p);
#ifdef KTRACE
	/* 
	 * release trace file
	 */
	ktrderef(p);
#endif
#ifdef SYSTRACE
	systrace_sys_exit(p);
#endif
	/*
	 * If emulation has process exit hook, call it now.
	 */
	if (p->p_emul->e_proc_exit)
		(*p->p_emul->e_proc_exit)(p);

	/*
	 * Give orphaned children to init(8).
	 */
	q = LIST_FIRST(&p->p_children);
	if (q)		/* only need this if any child is S_ZOMB */
		wakeup((caddr_t)initproc);
	for (; q != 0; q = nq) {
		nq = LIST_NEXT(q, p_sibling);

		/*
		 * Traced processes are killed since their existence
		 * means someone is screwing up. Since we reset the
		 * trace flags, the logic in sys_wait4() would not be
		 * triggered to reparent the process to its
		 * original parent, so we must do this here.
		 */
		if (q->p_flag & P_TRACED) {
			if (q->p_opptr != q->p_pptr) {
				struct proc *t = q->p_opptr;
				proc_reparent(q, t ? t : initproc);
				q->p_opptr = NULL;
			} else
				proc_reparent(q, initproc);
			q->p_flag &= ~(P_TRACED|P_WAITED|P_FSTRACE);
			psignal(q, SIGKILL);
		} else {
			proc_reparent(q, initproc);
		}
	}

	/*
	 * Reset p_opptr pointer of all former children which got
	 * traced by another process and were reparented. We reset
	 * it to NULL here; the trace detach code then reparents
	 * the child to initproc. We only check allproc list, since
	 * eventual former children on zombproc list won't reference
	 * p_opptr anymore.
	 */
	if (p->p_flag & P_CHTRACED) {
		struct proc *t;

		proclist_lock_read();

		LIST_FOREACH(t, &allproc, p_list) {
			if (t->p_opptr == p)
				t->p_opptr = NULL;
		}

		proclist_unlock_read();
	}

	/*
	 * Save exit status and final rusage info, adding in child rusage
	 * info and self times.
	 * In order to pick up the time for the current execution, we must
	 * do this before unlinking the lwp from l_list.
	 */
	p->p_xstat = rv;
	*p->p_ru = p->p_stats->p_ru;
	calcru(p, &p->p_ru->ru_utime, &p->p_ru->ru_stime, NULL);
	ruadd(p->p_ru, &p->p_stats->p_cru);

	/*
	 * NOTE: WE ARE NO LONGER ALLOWED TO SLEEP!
	 */

	/*
	 * Move proc from allproc to zombproc, but do not yet
	 * wake up the reaper.  We will put the proc on the
	 * deadproc list later (using the p_dead member), and
	 * wake up the reaper when we do.
	 * Changing the state to SDEAD stops it being found by pfind().
	 */
	s = proclist_lock_write();
	p->p_stat = SDEAD;
	p->p_nrlwps--;
	l->l_stat = LSDEAD;
	LIST_REMOVE(p, p_list);
	LIST_INSERT_HEAD(&zombproc, p, p_list);
	LIST_REMOVE(l, l_list);
	l->l_flag |= L_DETACHED;
	proclist_unlock_write(s);

	/*
	 * Notify interested parties of our demise.
	 */
	KNOTE(&p->p_klist, NOTE_EXIT);

#if PERFCTRS
	/*
	 * Save final PMC information in parent process & clean up.
	 */
	if (PMC_ENABLED(p)) {
		pmc_save_context(p);
		pmc_accumulate(p->p_pptr, p);
		pmc_process_exit(p);
	}
#endif

	/*
	 * Notify parent that we're gone.  If parent has the P_NOCLDWAIT
	 * flag set, notify init instead (and hope it will handle
	 * this situation).
	 */
	if (p->p_pptr->p_flag & P_NOCLDWAIT) {
		struct proc *pp = p->p_pptr;
		proc_reparent(p, initproc);
		/*
		 * If this was the last child of our parent, notify
		 * parent, so in case he was wait(2)ing, he will
		 * continue.
		 */
		if (LIST_FIRST(&pp->p_children) == NULL)
			wakeup((caddr_t)pp);
	}

	/*
	 * Release the process's signal state.
	 */
	sigactsfree(p);

	/*
	 * Clear curlwp after we've done all operations
	 * that could block, and before tearing down the rest
	 * of the process state that might be used from clock, etc.
	 * Also, can't clear curlwp while we're still runnable,
	 * as we're not on a run queue (we are current, just not
	 * a proper proc any longer!).
	 *
	 * Other substructures are freed from wait().
	 */
	curlwp = NULL;
	limfree(p->p_limit);
	pstatsfree(p->p_stats);
	p->p_limit = NULL;

	/* This process no longer needs to hold the kernel lock. */
	KERNEL_PROC_UNLOCK(l);

	/*
	 * Finally, call machine-dependent code to switch to a new
	 * context (possibly the idle context).  Once we are no longer
	 * using the dead process's vmspace and stack, exit2() will be
	 * called to schedule those resources to be released by the
	 * reaper thread.
	 *
	 * Note that cpu_exit() will end with a call equivalent to
	 * cpu_switch(), finishing our execution (pun intended).
	 */

	cpu_exit(l, 1);
}

void
exit_lwps(struct lwp *l)
{
	struct proc *p;
	struct lwp *l2;
	int s, error;
	lwpid_t		waited;

	p = l->l_proc;

	/* XXX SMP 
	 * This would be the right place to IPI any LWPs running on 
	 * other processors so that they can notice the userret exit hook.
	 */
	p->p_userret = lwp_exit_hook;
	p->p_userret_arg = NULL;

	/*
	 * Make SA-cached LWPs normal process runnable LWPs so that
	 * they'll also self-destruct.
	 */
	if (p->p_sa && p->p_sa->sa_ncached > 0) {
		DPRINTF(("exit_lwps: Making cached LWPs of %d runnable: ",
		    p->p_pid));
		SCHED_LOCK(s);
		while ((l2 = sa_getcachelwp(p)) != 0) {
			l2->l_priority = l2->l_usrpri;
			setrunnable(l2);
			DPRINTF(("%d ", l2->l_lid));
		}
		DPRINTF(("\n"));
		SCHED_UNLOCK(s);
	}
	
	/*
	 * Interrupt LWPs in interruptable sleep, unsuspend suspended
	 * LWPs, make detached LWPs undetached (so we can wait for
	 * them) and then wait for everyone else to finish.  
	 */
	LIST_FOREACH(l2, &p->p_lwps, l_sibling) {
#if 0
		l2->l_flag &= ~(L_DETACHED|L_SA);
#endif	
		l2->l_flag &= ~(L_DETACHED);
	
		if(l2->l_flag & L_SA_WANTS_VP)
		{
			wakeup(l2);
		}

		if ((l2->l_stat == LSSLEEP && (l2->l_flag & L_SINTR)) ||
		    l2->l_stat == LSSUSPENDED || l2->l_stat == LSSTOP) {
			SCHED_LOCK(s);
			setrunnable(l2);
			SCHED_UNLOCK(s);
			DPRINTF(("exit_lwps: Made %d.%d runnable\n",
			    p->p_pid, l2->l_lid));
		}
	}

	
	while (p->p_nlwps > 1) {
		DPRINTF(("exit_lwps: waiting for %d LWPs (%d runnable, %d zombies)\n", 
		    p->p_nlwps, p->p_nrlwps, p->p_nzlwps));
		error = lwp_wait1(l, 0, &waited, LWPWAIT_EXITCONTROL);
		if (error)
			panic("exit_lwps: lwp_wait1 failed with error %d\n",
			    error);
		DPRINTF(("exit_lwps: Got LWP %d from lwp_wait1()\n", waited));
	}	

	p->p_userret = NULL;
}

/* Wrapper function for use in p_userret */
static void
lwp_exit_hook(struct lwp *l, void *arg)
{
	KERNEL_PROC_LOCK(l);
	lwp_exit(l);
}

/*
 * We are called from cpu_exit() once it is safe to schedule the
 * dead process's resources to be freed (i.e., once we've switched to
 * the idle PCB for the current CPU).
 *
 * NOTE: One must be careful with locking in this routine.  It's
 * called from a critical section in machine-dependent code, so
 * we should refrain from changing any interrupt state.
 *
 * We lock the deadproc list (a spin lock), place the proc on that
 * list (using the p_dead member), and wake up the reaper.
 */
void
exit2(struct lwp *l)
{
	struct proc *p = l->l_proc;

	simple_lock(&deadproc_slock);
	SLIST_INSERT_HEAD(&deadprocs, p, p_dead);
	simple_unlock(&deadproc_slock);

	/* lwp_exit2() will wake up deadproc for us. */
	lwp_exit2(l);
}

/*
 * Process reaper.  This is run by a kernel thread to free the resources
 * of a dead process.  Once the resources are free, the process becomes
 * a zombie, and the parent is allowed to read the undead's status.
 */
void
reaper(void *arg)
{
	struct proc *p;
	struct lwp *l;

	KERNEL_PROC_UNLOCK(curlwp);

	for (;;) {
		simple_lock(&deadproc_slock);
		p = SLIST_FIRST(&deadprocs);
		l = LIST_FIRST(&deadlwp);
		if (p == NULL && l == NULL) {
			/* No work for us; go to sleep until someone exits. */
			(void) ltsleep(&deadprocs, PVM|PNORELOCK,
			    "reaper", 0, &deadproc_slock);
			continue;
		}

		if (l != NULL ) {
			p = l->l_proc;

			/* Remove lwp from the deadlwp list. */
			LIST_REMOVE(l, l_list);
			simple_unlock(&deadproc_slock);
			KERNEL_PROC_LOCK(curlwp);
			
			/*
			 * Give machine-dependent code a chance to free any
			 * resources it couldn't free while still running on
			 * that process's context.  This must be done before
			 * uvm_lwp_exit(), in case these resources are in the 
			 * PCB.
			 */
			cpu_wait(l);

			/*
			 * Free the VM resources we're still holding on to.
			 */
			uvm_lwp_exit(l);

			l->l_stat = LSZOMB;
			if (l->l_flag & L_DETACHED) {
				/* Nobody waits for detached LWPs. */
				LIST_REMOVE(l, l_sibling);
				p->p_nlwps--;
				pool_put(&lwp_pool, l);
			} else {
				p->p_nzlwps++;
				wakeup((caddr_t)&p->p_nlwps);
			}
			/* XXXNJW where should this be with respect to 
			 * the wakeup() above? */
			KERNEL_PROC_UNLOCK(curlwp);
		} else {
			/* Remove proc from the deadproc list. */
			SLIST_REMOVE_HEAD(&deadprocs, p_dead);
			simple_unlock(&deadproc_slock);
			KERNEL_PROC_LOCK(curlwp);

			/*
			 * Free the VM resources we're still holding on to.
			 * We must do this from a valid thread because doing
			 * so may block.
			 */
			uvm_proc_exit(p);
			
			/* Process is now a true zombie. */
			p->p_stat = SZOMB;
			
			/* Wake up the parent so it can get exit status. */
			if ((p->p_flag & P_FSTRACE) == 0 && p->p_exitsig != 0)
				psignal(p->p_pptr, P_EXITSIG(p));
			KERNEL_PROC_UNLOCK(curlwp);
			wakeup((caddr_t)p->p_pptr);
		}
	}
}

int
sys_wait4(struct lwp *l, void *v, register_t *retval)
{
	struct sys_wait4_args /* {
		syscallarg(int)			pid;
		syscallarg(int *)		status;
		syscallarg(int)			options;
		syscallarg(struct rusage *)	rusage;
	} */ *uap = v;
	struct proc	*child, *parent;
	int		status, error;

	parent = l->l_proc;

	if (SCARG(uap, pid) == 0)
		SCARG(uap, pid) = -parent->p_pgid;
	if (SCARG(uap, options) & ~(WUNTRACED|WNOHANG|WALTSIG|WALLSIG))
		return (EINVAL);

	error = find_stopped_child(parent, SCARG(uap,pid), SCARG(uap,options),
				&child);
	if (error != 0)
		return error;
	if (child == NULL) {
		*retval = 0;
		return 0;
	}

	retval[0] = child->p_pid;

	if (child->p_stat == SZOMB) {
		if (SCARG(uap, status)) {
			status = child->p_xstat;	/* convert to int */
			error = copyout((caddr_t)&status,
					(caddr_t)SCARG(uap, status),
					sizeof(status));
			if (error)
				return (error);
		}
		if (SCARG(uap, rusage)) {
			error = copyout((caddr_t)child->p_ru,
				    (caddr_t)SCARG(uap, rusage),
				    sizeof(struct rusage));
			if (error)
				return (error);
		}

		proc_free(child);
		return 0;
	}

	/* child state must be SSTOP */
	if (SCARG(uap, status)) {
		status = W_STOPCODE(child->p_xstat);
		return copyout((caddr_t)&status,
				(caddr_t)SCARG(uap, status),
				sizeof(status));
	}
	return 0;
}

/*
 * Scan list of child processes for a child process that has stopped or
 * exited.  Used by sys_wait4 and 'compat' equivalents.
 */
int
find_stopped_child(struct proc *parent, pid_t pid, int options,
	struct proc **child_p)
{
	struct proc *child;
	int c_found, error;

	for (;;) {
		c_found = 0;
		LIST_FOREACH(child, &parent->p_children, p_sibling) {
			if (pid != WAIT_ANY &&
			    child->p_pid != pid &&
			    child->p_pgid != -pid)
				continue;
			/*
			 * Wait for processes with p_exitsig != SIGCHLD
			 * processes only if WALTSIG is set; wait for
			 * processes with p_exitsig == SIGCHLD only
			 * if WALTSIG is clear.
			 */
			if (((options & WALLSIG) == 0) &&
			    (options & WALTSIG ? child->p_exitsig == SIGCHLD
						: P_EXITSIG(child) != SIGCHLD))
				continue;

			c_found = 1;
			if (child->p_stat == SZOMB &&
			    (options & WNOZOMBIE) == 0) {
				*child_p = child;
				return 0;
			}

			if (child->p_stat == SSTOP &&
			    (child->p_flag & P_WAITED) == 0 &&
			    (child->p_flag & P_TRACED || options & WUNTRACED)) {
				if ((options & WNOWAIT) == 0)
					child->p_flag |= P_WAITED;
				*child_p = child;
				return 0;
			}
		}
		if (c_found == 0)
			return ECHILD;
		if (options & WNOHANG) {
			*child_p = NULL;
			return 0;
		}
		error = tsleep((caddr_t)parent, PWAIT | PCATCH, "wait", 0);
		if (error != 0)
			return error;
	}
}

/*
 * Free a process after parent has taken all the state info.
 */
void
proc_free(struct proc *p)
{
	struct proc *parent = p->p_pptr;
	int s;

	/*
	 * If we got the child via ptrace(2) or procfs, and
	 * the parent is different (meaning the process was
	 * attached, rather than run as a child), then we need
	 * to give it back to the old parent, and send the
	 * parent the exit signal.  The rest of the cleanup
	 * will be done when the old parent waits on the child.
	 */
	if ((p->p_flag & P_TRACED) && p->p_opptr != parent){
		parent = p->p_opptr;
		if (parent == NULL)
			parent = initproc;
		proc_reparent(p, parent);
		p->p_opptr = NULL;
		p->p_flag &= ~(P_TRACED|P_WAITED|P_FSTRACE);
		if (p->p_exitsig != 0)
			psignal(parent, P_EXITSIG(p));
		wakeup((caddr_t)parent);
		return;
	}

	scheduler_wait_hook(parent, p);
	p->p_xstat = 0;

	ruadd(&parent->p_stats->p_cru, p->p_ru);

	/*
	 * At this point we are going to start freeing the final resources.
	 * If anyone tries to access the proc structure after here they
	 * will get a shock - bits are missing.
	 * Attempt to make it hard!
	 */

	p->p_stat = SIDL;		/* not even a zombie any more */

	pool_put(&rusage_pool, p->p_ru);

	/*
	 * Finally finished with old proc entry.
	 * Unlink it from its process group and free it.
	 */
	leavepgrp(p);

	s = proclist_lock_write();
	LIST_REMOVE(p, p_list);	/* off zombproc */
	LIST_REMOVE(p, p_sibling);
	proclist_unlock_write(s);

	/*
	 * Decrement the count of procs running with this uid.
	 */
	(void)chgproccnt(p->p_cred->p_ruid, -1);

	/*
	 * Free up credentials.
	 */
	if (--p->p_cred->p_refcnt == 0) {
		crfree(p->p_cred->pc_ucred);
		pool_put(&pcred_pool, p->p_cred);
	}

	/*
	 * Release reference to text vnode
	 */
	if (p->p_textvp)
		vrele(p->p_textvp);

	/*
	 * Release any SA state
	 */
	if (p->p_sa) {
		free(p->p_sa->sa_stacks, M_SA);
		pool_put(&sadata_pool, p->p_sa);
	}

	/* Free proc structure and let pid be reallocated */
	proc_free_mem(p);
}

/*
 * make process 'parent' the new parent of process 'child'.
 */
void
proc_reparent(struct proc *child, struct proc *parent)
{

	if (child->p_pptr == parent)
		return;

	if (parent == initproc)
		child->p_exitsig = SIGCHLD;

	LIST_REMOVE(child, p_sibling);
	LIST_INSERT_HEAD(&parent->p_children, child, p_sibling);
	child->p_pptr = parent;
}
