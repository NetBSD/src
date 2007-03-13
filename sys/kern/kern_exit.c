/*	$NetBSD: kern_exit.c,v 1.169.2.1 2007/03/13 16:51:53 ad Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Andrew Doran.
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
__KERNEL_RCSID(0, "$NetBSD: kern_exit.c,v 1.169.2.1 2007/03/13 16:51:53 ad Exp $");

#include "opt_ktrace.h"
#include "opt_perfctrs.h"
#include "opt_systrace.h"
#include "opt_sysv.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/kernel.h>
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
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/systrace.h>
#include <sys/kauth.h>
#include <sys/sleepq.h>
#include <sys/lockdebug.h>
#include <sys/ktrace.h>

#include <machine/cpu.h>

#include <uvm/uvm_extern.h>

#define DEBUG_EXIT

#ifdef DEBUG_EXIT
int debug_exit = 0;
#define DPRINTF(x) if (debug_exit) printf x
#else
#define DPRINTF(x)
#endif

/*
 * Fill in the appropriate signal information, and signal the parent.
 */
static void
exit_psignal(struct proc *p, struct proc *pp, ksiginfo_t *ksi)
{

	KSI_INIT(ksi);
	if ((ksi->ksi_signo = P_EXITSIG(p)) == SIGCHLD) {
		if (WIFSIGNALED(p->p_xstat)) {
			if (WCOREDUMP(p->p_xstat))
				ksi->ksi_code = CLD_DUMPED;
			else
				ksi->ksi_code = CLD_KILLED;
		} else {
			ksi->ksi_code = CLD_EXITED;
		}
	}
	/*
	 * We fill those in, even for non-SIGCHLD.
	 * It's safe to access p->p_cred unlocked here.
	 */
	ksi->ksi_pid = p->p_pid;
	ksi->ksi_uid = kauth_cred_geteuid(p->p_cred);
	ksi->ksi_status = p->p_xstat;
	/* XXX: is this still valid? */
	ksi->ksi_utime = p->p_ru->ru_utime.tv_sec;
	ksi->ksi_stime = p->p_ru->ru_stime.tv_sec;
}

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
	struct proc *p = l->l_proc;

	/* Don't call exit1() multiple times in the same process. */
	mutex_enter(&p->p_smutex);
	if (p->p_sflag & PS_WEXIT) {
		mutex_exit(&p->p_smutex);
		lwp_exit(l);
	}

	/* exit1() will release the mutex. */
	exit1(l, W_EXITCODE(SCARG(uap, rval), 0));
	/* NOTREACHED */
	return (0);
}

/*
 * Exit: deallocate address space and other resources, change proc state
 * to zombie, and unlink proc from allproc and parent's lists.  Save exit
 * status and rusage for wait().  Check for child processes and orphan them.
 *
 * Must be called with p->p_smutex held.  Does not return.
 */
void
exit1(struct lwp *l, int rv)
{
	struct proc	*p, *q, *nq;
	int		s;
	ksiginfo_t	ksi;
	ksiginfoq_t	kq;
	int		wakeinit;

	p = l->l_proc;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	if (__predict_false(p == initproc))
		panic("init died (signal %d, exit %d)",
		    WTERMSIG(rv), WEXITSTATUS(rv));

	p->p_sflag |= PS_WEXIT;

	/*
	 * Force all other LWPs to exit before we do.  Only then can we
	 * begin to tear down the rest of the process state.
	 */
	if (p->p_nlwps > 1)
		exit_lwps(l);

	ksiginfo_queue_init(&kq);

	/*
	 * If we have been asked to stop on exit, do so now.
	 */
	if (p->p_sflag & PS_STOPEXIT) {
		KERNEL_UNLOCK_ALL(l, &l->l_biglocks);
		sigclearall(p, &contsigmask, &kq);
		p->p_waited = 0;
		mb_write();
		p->p_stat = SSTOP;
		lwp_lock(l);
		p->p_nrlwps--;
		l->l_stat = LSSTOP;
		mutex_exit(&p->p_smutex);
		mi_switch(l, NULL);
		KERNEL_LOCK(l->l_biglocks, l);
	} else
		mutex_exit(&p->p_smutex);

	/*
	 * Drain all remaining references that procfs, ptrace and others may
	 * have on the process.
	 */
	mutex_enter(&p->p_mutex);
	proc_drainrefs(p);
	mutex_exit(&p->p_mutex);

	/*
	 * Bin any remaining signals and mark the process as dying so it will
	 * not be found for, e.g. signals. 
	 */
	mutex_enter(&p->p_smutex);
	sigfillset(&p->p_sigctx.ps_sigignore);
	sigclearall(p, NULL, &kq);
	p->p_stat = SDYING;
	mutex_exit(&p->p_smutex);
	ksiginfo_queue_drain(&kq);

	DPRINTF(("exit1: %d.%d exiting.\n", p->p_pid, l->l_lid));

#ifdef PGINPROF
	vmsizmon();
#endif
	p->p_ru = pool_get(&rusage_pool, PR_WAITOK);
	timers_free(p, TIMERS_ALL);
#if defined(__HAVE_RAS)
	ras_purgeall(p);
#endif

	/*
	 * Close open files, release open-file table and free signal
	 * actions.  This may block!
	 */
	fdfree(l);
	cwdfree(p->p_cwdi);
	p->p_cwdi = NULL;
	doexithooks(p);
	sigactsfree(p->p_sigacts);

	/*
	 * Write out accounting data.
	 */
	(void)acct_process(l);

#ifdef KTRACE
	/*
	 * Release trace file.
	 */
	if (p->p_tracep != NULL) {
		mutex_enter(&ktrace_mutex);
		ktrderef(p);
		mutex_exit(&ktrace_mutex);
	}
#endif
#ifdef SYSTRACE
	systrace_sys_exit(p);
#endif

	/*
	 * If emulation has process exit hook, call it now.
	 * Set the exit status now so that the exit hook has
	 * an opportunity to tweak it (COMPAT_LINUX requires
	 * this for thread group emulation)
	 */
	p->p_xstat = rv;
	if (p->p_emul->e_proc_exit)
		(*p->p_emul->e_proc_exit)(p);

	/* Collect child u-areas. */
	uvm_uarea_drain(false);

	/*
	 * Free the VM resources we're still holding on to.
	 * We must do this from a valid thread because doing
	 * so may block. This frees vmspace, which we don't
	 * need anymore. The only remaining lwp is the one
	 * we run at this moment, nothing runs in userland
	 * anymore.
	 */
	uvm_proc_exit(p);

	/*
	 * Stop profiling.
	 */
	if ((p->p_stflag & PST_PROFIL) != 0) {
		mutex_spin_enter(&p->p_stmutex);
		stopprofclock(p);
		mutex_spin_exit(&p->p_stmutex);
	}

	/*
	 * If parent is waiting for us to exit or exec, P_PPWAIT is set; we
	 * wake up the parent early to avoid deadlock.  We can do this once
	 * the VM resources are released.
	 */
	mutex_enter(&proclist_lock);

	mutex_enter(&p->p_smutex);
	if (p->p_sflag & PS_PPWAIT) {
		p->p_sflag &= ~PS_PPWAIT;
		cv_wakeup(&p->p_pptr->p_waitcv); /* XXXSMP */
	}
	mutex_exit(&p->p_smutex);

	if (SESS_LEADER(p)) {
		struct vnode *vprele = NULL, *vprevoke = NULL;
		struct session *sp = p->p_session;
		struct tty *tp;

		if (sp->s_ttyvp) {
			/*
			 * Controlling process.
			 * Signal foreground pgrp,
			 * drain controlling terminal
			 * and revoke access to controlling terminal.
			 */
			tp = sp->s_ttyp;
			s = spltty();
			TTY_LOCK(tp);
			if (tp->t_session == sp) {
				if (tp->t_pgrp) {
					mutex_enter(&proclist_mutex);
					pgsignal(tp->t_pgrp, SIGHUP, 1);
					mutex_exit(&proclist_mutex);
				}
				/* we can't guarantee the revoke will do this */
				tp->t_pgrp = NULL;
				tp->t_session = NULL;
				TTY_UNLOCK(tp);
				splx(s);
				SESSRELE(sp);
				mutex_exit(&proclist_lock);
				(void) ttywait(tp);
				mutex_enter(&proclist_lock);

				/*
				 * The tty could have been revoked
				 * if we blocked.
				 */
				vprevoke = sp->s_ttyvp;
			} else {
				TTY_UNLOCK(tp);
				splx(s);
			}
			vprele = sp->s_ttyvp;
			sp->s_ttyvp = NULL;
			/*
			 * s_ttyp is not zero'd; we use this to indicate
			 * that the session once had a controlling terminal.
			 * (for logging and informational purposes)
			 */
		}
		sp->s_leader = NULL;

		if (vprevoke != NULL || vprele != NULL) {
			mutex_exit(&proclist_lock);
			if (vprevoke != NULL)
				VOP_REVOKE(vprevoke, REVOKEALL);
			if (vprele != NULL)
				vrele(vprele);
			mutex_enter(&proclist_lock);
		}
	}
	mutex_enter(&proclist_mutex);
	fixjobc(p, p->p_pgrp, 0);
	mutex_exit(&proclist_mutex);

	/*
	 * Finalize the last LWP's specificdata, as well as the
	 * specificdata for the proc itself.
	 */
	lwp_finispecific(l);
	proc_finispecific(p);

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
	 * Reset p_opptr pointer of all former children which got
	 * traced by another process and were reparented. We reset
	 * it to NULL here; the trace detach code then reparents
	 * the child to initproc. We only check allproc list, since
	 * eventual former children on zombproc list won't reference
	 * p_opptr anymore.
	 */
	if (p->p_slflag & PSL_CHTRACED) {
		PROCLIST_FOREACH(q, &allproc) {
			if (q->p_opptr == p)
				q->p_opptr = NULL;
		}
	}

	/*
	 * Give orphaned children to init(8).
	 */
	q = LIST_FIRST(&p->p_children);
	wakeinit = (q != NULL);
	for (; q != NULL; q = nq) {
		nq = LIST_NEXT(q, p_sibling);

		/*
		 * Traced processes are killed since their existence
		 * means someone is screwing up. Since we reset the
		 * trace flags, the logic in sys_wait4() would not be
		 * triggered to reparent the process to its
		 * original parent, so we must do this here.
		 */
		if (q->p_slflag & PSL_TRACED) {
			mutex_enter(&p->p_smutex);
			q->p_slflag &= ~(PSL_TRACED|PSL_FSTRACE|PSL_SYSCALL);
			mutex_exit(&p->p_smutex);
			if (q->p_opptr != q->p_pptr) {
				struct proc *t = q->p_opptr;
				proc_reparent(q, t ? t : initproc);
				q->p_opptr = NULL;
			} else
				proc_reparent(q, initproc);
			killproc(q, "orphaned traced process");
		} else
			proc_reparent(q, initproc);
	}

	/*
	 * Move proc from allproc to zombproc, it's now nearly ready to be
	 * collected by parent.
	 */
	mutex_enter(&proclist_mutex);
	LIST_REMOVE(l, l_list);
	LIST_REMOVE(p, p_list);
	LIST_INSERT_HEAD(&zombproc, p, p_list);

	/*
	 * Mark the process as dead.  We must do this before we signal
	 * the parent.
	 */
	p->p_stat = SDEAD;

	/* Put in front of parent's sibling list for parent to collect it */
	q = p->p_pptr;
	q->p_nstopchild++;
	if (LIST_FIRST(&q->p_children) != p) {
		/* Put child where it can be found quickly */
		LIST_REMOVE(p, p_sibling);
		LIST_INSERT_HEAD(&q->p_children, p, p_sibling);
	}
	mutex_exit(&proclist_mutex);

	/*
	 * Notify parent that we're gone.  If parent has the P_NOCLDWAIT
	 * flag set, notify init instead (and hope it will handle
	 * this situation).
	 */
	mutex_enter(&q->p_mutex);
	if (q->p_flag & (PK_NOCLDWAIT|PK_CLDSIGIGN)) {
		proc_reparent(p, initproc);
		wakeinit = 1;

		/*
		 * If this was the last child of our parent, notify
		 * parent, so in case he was wait(2)ing, he will
		 * continue.
		 */
		if (LIST_FIRST(&q->p_children) == NULL)
			cv_wakeup(&q->p_waitcv);	/* XXXSMP */
	}
	mutex_exit(&q->p_mutex);

	/* Reload parent pointer, since p may have been reparented above */
	q = p->p_pptr;

	if ((p->p_slflag & PSL_FSTRACE) == 0 && p->p_exitsig != 0) {
		exit_psignal(p, q, &ksi);
		mutex_enter(&proclist_mutex);
		kpsignal(q, &ksi, NULL);
		mutex_exit(&proclist_mutex);
	}

	/*
	 * Save final rusage info, adding in child rusage info and self
	 * times.  It's OK to call caclru() unlocked here.
	 */
	*p->p_ru = p->p_stats->p_ru;
	calcru(p, &p->p_ru->ru_utime, &p->p_ru->ru_stime, NULL, NULL);
	ruadd(p->p_ru, &p->p_stats->p_cru);

	if (wakeinit)
		cv_wakeup(&initproc->p_waitcv);	/* XXXSMP */

	/*
	 * Remaining lwp resources will be freed in lwp_exit2() once we've
	 * switch to idle context; at that point, we will be marked as a
	 * full blown zombie.
	 *
	 * XXXSMP disable preemption.
	 */
	mutex_enter(&p->p_smutex);
	lwp_drainrefs(l);
	lwp_lock(l);
	l->l_prflag &= ~LPR_DETACHED;
	l->l_stat = LSZOMB;
	lwp_unlock(l);
	KASSERT(curlwp == l);
	KASSERT(p->p_nrlwps == 1);
	KASSERT(p->p_nlwps == 1);
	p->p_stat = SZOMB;
	p->p_nrlwps--;
	p->p_nzlwps++;
	p->p_ndlwps = 0;
	mutex_exit(&p->p_smutex);

	/*
	 * Signal the parent to collect us, and drop the proclist lock.
	 */
	mutex_exit(&proclist_lock);

	/* Verify that we hold no locks other than the kernel lock. */
#ifdef MULTIPROCESSOR
	LOCKDEBUG_BARRIER(&kernel_lock, 0);
#else
	LOCKDEBUG_BARRIER(NULL, 0);
#endif

	/*
	 * NOTE: WE ARE NO LONGER ALLOWED TO SLEEP!
	 */

	/*
	 * Give machine-dependent code a chance to free any MD LWP
	 * resources.  This must be done before uvm_lwp_exit(), in
	 * case these resources are in the PCB.
	 */
#ifndef __NO_CPU_LWP_FREE
	cpu_lwp_free(l, 1);
#endif
	pmap_deactivate(l);

	/* This process no longer needs to hold the kernel lock. */
#ifdef notyet
	/* XXXSMP hold in lwp_userret() */
	KERNEL_UNLOCK_LAST(l);
#else
	KERNEL_UNLOCK_ALL(l, NULL);
#endif

	/*
	 * Finally, call machine-dependent code to switch to a new
	 * context (possibly the idle context).  Once we are no longer
	 * using the dead lwp's stack, lwp_exit2() will be called.
	 *
	 * Note that cpu_exit() will end with a call equivalent to
	 * cpu_switch(), finishing our execution (pun intended).
	 */
	uvmexp.swtch++;	/* XXXSMP unlocked */
	cv_wakeup(&p->p_pptr->p_waitcv);	/* XXXSMP */
	cpu_exit(l);
}

void
exit_lwps(struct lwp *l)
{
	struct proc *p;
	struct lwp *l2;
	int error;
	lwpid_t waited;
#if defined(MULTIPROCESSOR)
	int nlocks;
#endif

	KERNEL_UNLOCK_ALL(l, &nlocks);

	p = l->l_proc;

 retry:
	/*
	 * Interrupt LWPs in interruptable sleep, unsuspend suspended
	 * LWPs and then wait for everyone else to finish.
	 */
	LIST_FOREACH(l2, &p->p_lwps, l_sibling) {
		if (l2 == l)
			continue;
		lwp_lock(l2);
		l2->l_flag |= LW_WEXIT;
		if ((l2->l_stat == LSSLEEP && (l2->l_flag & LW_SINTR)) ||
		    l2->l_stat == LSSUSPENDED || l2->l_stat == LSSTOP) {
		    	/* setrunnable() will release the lock. */
			setrunnable(l2);
			DPRINTF(("exit_lwps: Made %d.%d runnable\n",
			    p->p_pid, l2->l_lid));
			continue;
		}
		lwp_unlock(l2);
	}
	while (p->p_nlwps > 1) {
		DPRINTF(("exit_lwps: waiting for %d LWPs (%d zombies)\n",
		    p->p_nlwps, p->p_nzlwps));
		error = lwp_wait1(l, 0, &waited, LWPWAIT_EXITCONTROL);
		if (p->p_nlwps == 1)
			break;
		if (error == EDEADLK) {
			/*
			 * LWPs can get suspended/slept behind us.
			 * (eg. sa_setwoken)
			 * kick them again and retry.
			 */
			goto retry;
		}
		if (error)
			panic("exit_lwps: lwp_wait1 failed with error %d",
			    error);
		DPRINTF(("exit_lwps: Got LWP %d from lwp_wait1()\n", waited));
	}

	KERNEL_LOCK(nlocks, l);
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
	struct rusage	ru;

	parent = l->l_proc;

	if (SCARG(uap, pid) == 0)
		SCARG(uap, pid) = -parent->p_pgid;
	if (SCARG(uap, options) & ~(WUNTRACED|WNOHANG|WALTSIG|WALLSIG))
		return (EINVAL);

	mutex_enter(&proclist_lock);

	error = find_stopped_child(parent, SCARG(uap,pid), SCARG(uap,options),
	    &child, &status);
	if (error != 0) {
		mutex_exit(&proclist_lock);
		return error;
	}
	if (child == NULL) {
		mutex_exit(&proclist_lock);
		*retval = 0;
		return 0;
	}

	retval[0] = child->p_pid;

	if (P_ZOMBIE(child)) {
		KERNEL_LOCK(1, l);		/* XXXSMP */
		/* proc_free() will release the proclist_lock. */
		proc_free(child, (SCARG(uap, rusage) == NULL ? NULL : &ru));
		KERNEL_UNLOCK_ONE(l);		/* XXXSMP */

		if (SCARG(uap, rusage))
			error = copyout(&ru, SCARG(uap, rusage), sizeof(ru));
		if (error == 0 && SCARG(uap, status))
			error = copyout(&status, SCARG(uap, status),
			    sizeof(status));

		return error;
	}

	mutex_exit(&proclist_lock);

	/* Child state must have been SSTOP. */
	if (SCARG(uap, status)) {
		status = W_STOPCODE(status);
		return copyout(&status, SCARG(uap, status), sizeof(status));
	}

	return 0;
}

/*
 * Scan list of child processes for a child process that has stopped or
 * exited.  Used by sys_wait4 and 'compat' equivalents.
 *
 * Must be called with the proclist_lock held, and may release
 * while waiting.
 */
int
find_stopped_child(struct proc *parent, pid_t pid, int options,
		   struct proc **child_p, int *status_p)
{
	struct proc *child, *dead;
	int error;

	KASSERT(mutex_owned(&proclist_lock));

	for (;;) {
		error = ECHILD;
		dead = NULL;

		mutex_enter(&proclist_mutex);
		LIST_FOREACH(child, &parent->p_children, p_sibling) {
			if (pid >= 0) {
				if (child->p_pid != pid) {
					child = p_find(pid, PFIND_ZOMBIE |
					    PFIND_LOCKED);
					if (child == NULL ||
					    child->p_pptr != parent) {
						child = NULL;
						break;
					}
				}
			} else if (pid != WAIT_ANY && child->p_pgid != -pid) {
				/* Child not in correct pgrp */
				continue;
			}

			/*
			 * Wait for processes with p_exitsig != SIGCHLD
			 * processes only if WALTSIG is set; wait for
			 * processes with p_exitsig == SIGCHLD only
			 * if WALTSIG is clear.
			 */
			if (((options & WALLSIG) == 0) &&
			    (options & WALTSIG ? child->p_exitsig == SIGCHLD
						: P_EXITSIG(child) != SIGCHLD)){
				if (child->p_pid == pid) {
					child = NULL;
					break;
				}
				continue;
			}

			error = 0;
			if ((options & WNOZOMBIE) == 0) {
				if (child->p_stat == SZOMB)
					break;
				if (child->p_stat == SDEAD) {
					/*
					 * We may occasionally arrive here
					 * after receiving a signal, but
					 * immediatley before the child
					 * process is zombified.  The wait
					 * will be short, so avoid returning
					 * to userspace.
					 */
					dead = child;
				}
			}

			if (child->p_stat == SSTOP &&
			    child->p_waited == 0 &&
			    (child->p_slflag & PSL_TRACED ||
			    options & WUNTRACED)) {
				if ((options & WNOWAIT) == 0) {
					child->p_waited = 1;
					parent->p_nstopchild--;
				}
				break;
			}
			if (parent->p_nstopchild == 0 || child->p_pid == pid) {
				child = NULL;
				break;
			}
		}

		if (child != NULL || error != 0 ||
		    ((options & WNOHANG) != 0 && dead == NULL)) {
		    	if (child != NULL)
			    	*status_p = child->p_xstat;
			mutex_exit(&proclist_mutex);
			*child_p = child;
			return error;
		}

		/*
		 * Wait for another child process to stop.
		 */
		mutex_exit(&proclist_lock);
		error = cv_wait_sig(&parent->p_waitcv, &proclist_mutex);
		mutex_exit(&proclist_mutex);
		mutex_enter(&proclist_lock);

		if (error != 0)
			return error;
	}
}

/*
 * Free a process after parent has taken all the state info.  Must be called
 * with the proclist lock held, and will release before returning.
 *
 * *ru is returned to the caller, and must be freed by the caller.
 */
void
proc_free(struct proc *p, struct rusage *caller_ru)
{
	struct plimit *plim;
	struct pstats *pstats;
	struct rusage *ru;
	struct proc *parent;
	struct lwp *l;
	ksiginfo_t ksi;
	kauth_cred_t cred;
	struct vnode *vp;
	uid_t uid;

	KASSERT(mutex_owned(&proclist_lock));
	KASSERT(p->p_nlwps == 1);
	KASSERT(p->p_nzlwps == 1);
	KASSERT(p->p_nrlwps == 0);
	KASSERT(p->p_stat == SZOMB);

	if (caller_ru != NULL)
		memcpy(caller_ru, p->p_ru, sizeof(*caller_ru));

	/*
	 * If we got the child via ptrace(2) or procfs, and
	 * the parent is different (meaning the process was
	 * attached, rather than run as a child), then we need
	 * to give it back to the old parent, and send the
	 * parent the exit signal.  The rest of the cleanup
	 * will be done when the old parent waits on the child.
	 */
	if ((p->p_slflag & PSL_TRACED) != 0) {
		parent = p->p_pptr;
		if (p->p_opptr != parent){
			mutex_enter(&p->p_smutex);
			p->p_slflag &= ~(PSL_TRACED|PSL_FSTRACE|PSL_SYSCALL);
			mutex_exit(&p->p_smutex);
			parent = p->p_opptr;
			if (parent == NULL)
				parent = initproc;
			proc_reparent(p, parent);
			p->p_opptr = NULL;
			if (p->p_exitsig != 0) {
				exit_psignal(p, parent, &ksi);
				mutex_enter(&proclist_mutex);
				kpsignal(parent, &ksi, NULL);
				mutex_exit(&proclist_mutex);
			}
			cv_wakeup(&parent->p_waitcv);	/* XXXSMP */
			mutex_exit(&proclist_lock);
			return;
		}
	}

	/*
	 * Finally finished with old proc entry.  Unlink it from its process
	 * group.
	 */
	leavepgrp(p);

	parent = p->p_pptr;
	scheduler_wait_hook(parent, p);
	ruadd(&parent->p_stats->p_cru, p->p_ru);
	p->p_xstat = 0;

	/*
	 * At this point we are going to start freeing the final resources. 
	 * If anyone tries to access the proc structure after here they will
	 * get a shock - bits are missing.  Attempt to make it hard!  We
	 * don't bother with any further locking past this point.
	 */
	mutex_enter(&proclist_mutex);
	p->p_stat = SIDL;		/* not even a zombie any more */
	LIST_REMOVE(p, p_list);	/* off zombproc */
	parent = p->p_pptr;
	p->p_pptr->p_nstopchild--;
	mutex_exit(&proclist_mutex);
	LIST_REMOVE(p, p_sibling);

	uid = kauth_cred_getuid(p->p_cred);
	vp = p->p_textvp;
	cred = p->p_cred;
	ru = p->p_ru;

	l = LIST_FIRST(&p->p_lwps);

#ifdef MULTIPROCESSOR
	/*
	 * If the last remaining LWP is still on the CPU (unlikely), then
	 * spin until it has switched away.  We need to release all locks
	 * to avoid deadlock against interrupt handlers on the target CPU.
	 */
	if (l->l_cpu->ci_curlwp == l) {
		int count;
		mutex_exit(&proclist_lock);
		KERNEL_UNLOCK_ALL(l, &count);
		while (l->l_cpu->ci_curlwp == l)
			SPINLOCK_BACKOFF_HOOK;
		KERNEL_LOCK(count, l);
		mutex_enter(&proclist_lock);
	}
#endif

	mutex_destroy(&p->p_rasmutex);
	mutex_destroy(&p->p_mutex);
	mutex_destroy(&p->p_stmutex);
	mutex_destroy(&p->p_smutex);
	cv_destroy(&p->p_waitcv);
	cv_destroy(&p->p_lwpcv);
	cv_destroy(&p->p_refcv);

	/*
	 * Delay release until after dropping the proclist lock.
	 */
	plim = p->p_limit;
	pstats = p->p_stats;

	/*
	 * Free the proc structure and let pid be reallocated.  This will
	 * release the proclist_lock.
	 */
	proc_free_mem(p);

	/*
	 * Decrement the count of procs running with this uid.
	 */
	(void)chgproccnt(uid, -1);

	/*
	 * Release substructures.
	 */
	limfree(plim);
	pstatsfree(pstats);
	kauth_cred_free(cred);
	kauth_cred_free(l->l_cred);

	/*
	 * Release reference to text vnode
	 */
	if (vp)
		vrele(vp);

	/*
	 * Free the last LWP's resources.
	 */
	lwp_free(l, 0, 1);

	/*
	 * Collect child u-areas.
	 */
	uvm_uarea_drain(false);
	pool_put(&rusage_pool, ru);
}

/*
 * make process 'parent' the new parent of process 'child'.
 *
 * Must be called with proclist_lock lock held.
 */
void
proc_reparent(struct proc *child, struct proc *parent)
{

	KASSERT(mutex_owned(&proclist_lock));

	if (child->p_pptr == parent)
		return;

	mutex_enter(&proclist_mutex);
	if (child->p_stat == SZOMB ||
	    (child->p_stat == SSTOP && !child->p_waited)) {
		child->p_pptr->p_nstopchild--;
		parent->p_nstopchild++;
	}
	mutex_exit(&proclist_mutex);
	if (parent == initproc)
		child->p_exitsig = SIGCHLD;

	LIST_REMOVE(child, p_sibling);
	LIST_INSERT_HEAD(&parent->p_children, child, p_sibling);
	child->p_pptr = parent;
}
