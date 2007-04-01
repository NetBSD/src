/*	$NetBSD: kern_exit.c,v 1.162.2.1 2007/04/01 16:16:20 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kern_exit.c,v 1.162.2.1 2007/04/01 16:16:20 bouyer Exp $");

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
#include <sys/kauth.h>

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
 * Fill in the appropriate signal information, and signal the parent.
 */
static void
exit_psignal(struct proc *p, struct proc *pp, ksiginfo_t *ksi)
{

	(void)memset(ksi, 0, sizeof(ksiginfo_t));
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
	 * we fill those in, even for non-SIGCHLD.
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
	ksiginfo_t	ksi;
	int		do_psignal = 0;

	p = l->l_proc;

	if (__predict_false(p == initproc))
		panic("init died (signal %d, exit %d)",
		    WTERMSIG(rv), WEXITSTATUS(rv));

	p->p_flag |= P_WEXIT;
	if (p->p_flag & P_STOPEXIT) {
		sigminusset(&contsigmask, &p->p_sigctx.ps_siglist);
		SCHED_LOCK(s);
		p->p_stat = SSTOP;
		l->l_stat = LSSTOP;
		p->p_nrlwps--;
		mi_switch(l, NULL);
		SCHED_ASSERT_UNLOCKED();
		splx(s);
	}

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
		wakeup(p->p_pptr);
	}
	sigfillset(&p->p_sigctx.ps_sigignore);
	sigemptyset(&p->p_sigctx.ps_siglist);
	p->p_sigctx.ps_sigcheck = 0;
	p->p_stat = SDYING;
	timers_free(p, TIMERS_ALL);

	if (sa || (p->p_nlwps > 1)) {
		exit_lwps(l);

		/*
		 * Collect thread u-areas.
		 */
		uvm_uarea_drain(FALSE);
	}

#if defined(__HAVE_RAS)
	ras_purgeall(p);
#endif

	/*
	 * Close open files and release open-file table.
	 * This may block!
	 */
	fdfree(l);
	cwdfree(p->p_cwdi);
	p->p_cwdi = 0;

	doexithooks(p);

	if (SESS_LEADER(p)) {
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
				if (tp->t_pgrp)
					pgsignal(tp->t_pgrp, SIGHUP, 1);
				/* we can't guarantee the revoke will do this */
				tp->t_pgrp = NULL;
				tp->t_session = NULL;
				TTY_UNLOCK(tp);
				splx(s);
				SESSRELE(sp);
				(void) ttywait(tp);
				/*
				 * The tty could have been revoked
				 * if we blocked.
				 */
				if (sp->s_ttyvp)
					VOP_REVOKE(sp->s_ttyvp, REVOKEALL);
			} else {
				TTY_UNLOCK(tp);
				splx(s);
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

	/*
	 * Collect accounting flags from the last remaining LWP (this one),
	 * and write out accounting data.
	 */
	p->p_acflag |= l->l_acflag;
	(void)acct_process(l);

#ifdef KTRACE
	/*
	 * Release trace file.
	 */
	ktrderef(p);
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

	/*
	 * Free the VM resources we're still holding on to.
	 * We must do this from a valid thread because doing
	 * so may block. This frees vmspace, which we don't
	 * need anymore. The only remaining lwp is the one
	 * we run at this moment, nothing runs in userland
	 * anymore.
	 */
	uvm_proc_exit(p);

	/* Release substructures.  These might need to sleep. */
	kauth_cred_free(l->l_cred);
	sigactsfree(p->p_sigacts);
	p->p_sigacts = NULL;

	/*
	 * Finalize the last LWP's specificdata, as well as the
	 * specificdata for the proc itself.
	 */
	lwp_finispecific(l);
	proc_finispecific(p);

	/*
	 * Give machine-dependent code a chance to free any
	 * MD LWP resources while we can still block. This must be done
	 * before uvm_lwp_exit(), in case these resources are in the
	 * PCB.
	 * THIS IS LAST BLOCKING OPERATION.
	 */
#ifndef __NO_CPU_LWP_FREE
	cpu_lwp_free(l, 1);
#endif

	pmap_deactivate(l);

	/*
	 * NOTE: WE ARE NO LONGER ALLOWED TO SLEEP!
	 */

	/*
	 * Save final rusage info, adding in child rusage info and self times.
	 * In order to pick up the time for the current execution, we must
	 * do this before unlinking the lwp from l_list.
	 */
	*p->p_ru = p->p_stats->p_ru;
	calcru(p, &p->p_ru->ru_utime, &p->p_ru->ru_stime, NULL);
	ruadd(p->p_ru, &p->p_stats->p_cru);

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

	s = proclist_lock_write();
	/*
	 * Reset p_opptr pointer of all former children which got
	 * traced by another process and were reparented. We reset
	 * it to NULL here; the trace detach code then reparents
	 * the child to initproc. We only check allproc list, since
	 * eventual former children on zombproc list won't reference
	 * p_opptr anymore.
	 */
	if (p->p_flag & P_CHTRACED) {
		PROCLIST_FOREACH(q, &allproc) {
			if (q->p_opptr == p)
				q->p_opptr = NULL;
		}
	}

	/*
	 * Give orphaned children to init(8).
	 */
	q = LIST_FIRST(&p->p_children);
	if (q)		/* only need this if any child is SZOMB */
		wakeup(initproc);
	for (; q != NULL; q = nq) {
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
			q->p_flag &= ~(P_TRACED|P_WAITED|P_FSTRACE|P_SYSCALL);
			killproc(q, "orphaned traced process");
		} else {
			proc_reparent(q, initproc);
		}
	}

	/*
	 * Move proc from allproc to zombproc, it's now ready
	 * to be collected by parent. Remaining lwp resources
	 * will be freed in lwp_exit2() once we've switch to idle
	 * context.
	 * Changing the state to SZOMB stops it being found by pfind().
	 */
	LIST_REMOVE(p, p_list);
	LIST_INSERT_HEAD(&zombproc, p, p_list);
	p->p_stat = SZOMB;

	LIST_REMOVE(l, l_list);
	LIST_REMOVE(l, l_sibling);
	l->l_flag |= L_DETACHED;	/* detached from proc too */
	l->l_stat = LSDEAD;

	KASSERT(p->p_nrlwps == 1);
	KASSERT(p->p_nlwps == 1);
	p->p_nrlwps--;
	p->p_nlwps--;

	/* Put in front of parent's sibling list for parent to collect it */
	q = p->p_pptr;
	q->p_nstopchild++;
	if (LIST_FIRST(&q->p_children) != p) {
		/* Put child where it can be found quickly */
		LIST_REMOVE(p, p_sibling);
		LIST_INSERT_HEAD(&q->p_children, p, p_sibling);
	}

	/*
	 * Notify parent that we're gone.  If parent has the P_NOCLDWAIT
	 * flag set, notify init instead (and hope it will handle
	 * this situation).
	 */
	if (q->p_flag & (P_NOCLDWAIT|P_CLDSIGIGN)) {
		proc_reparent(p, initproc);

		/*
		 * If this was the last child of our parent, notify
		 * parent, so in case he was wait(2)ing, he will
		 * continue.
		 */
		if (LIST_FIRST(&q->p_children) == NULL)
			wakeup(q);
	}

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

	/* Reload parent pointer, since p may have been reparented above */
	q = p->p_pptr;

	if ((p->p_flag & P_FSTRACE) == 0 && p->p_exitsig != 0) {
		exit_psignal(p, q, &ksi);
		do_psignal = 1;
	}

	/*
	 * Once we release the proclist lock, we shouldn't touch the
	 * process structure anymore, since it's now on the zombie
	 * list and available for collection by the parent.
	 */
	proclist_unlock_write(s);

	if (do_psignal)
		kpsignal(q, &ksi, NULL);

	/* Wake up the parent so it can get exit status. */
	wakeup(q);

#ifdef DEBUG
	/* Nothing should use the process link anymore */
	l->l_proc = NULL;
#endif

	/* This process no longer needs to hold the kernel lock. */
	KERNEL_PROC_UNLOCK(l);

	/*
	 * Finally, call machine-dependent code to switch to a new
	 * context (possibly the idle context).  Once we are no longer
	 * using the dead lwp's stack, lwp_exit2() will be called
	 * to arrange for the resources to be released.
	 *
	 * Note that cpu_exit() will end with a call equivalent to
	 * cpu_switch(), finishing our execution (pun intended).
	 */

	uvmexp.swtch++;
	cpu_exit(l);
}

void
exit_lwps(struct lwp *l)
{
	struct proc *p;
	struct lwp *l2;
	struct sadata_vp *vp;
	int s, error;
	lwpid_t		waited;

	p = l->l_proc;

	/* XXX SMP
	 * This would be the right place to IPI any LWPs running on
	 * other processors so that they can notice the userret exit hook.
	 */
	p->p_userret = lwp_exit_hook;
	p->p_userret_arg = NULL;

	if (p->p_sa) {
		SLIST_FOREACH(vp, &p->p_sa->sa_vps, savp_next) {
			/*
			 * Make SA-cached LWPs normal process runnable
			 * LWPs so that they'll also self-destruct.
			 */
			DPRINTF(("exit_lwps: Making cached LWPs of %d on VP %d runnable: ",
				    p->p_pid, vp->savp_id));
			SCHED_LOCK(s);
			while ((l2 = sa_getcachelwp(vp)) != 0) {
				l2->l_priority = l2->l_usrpri;
				setrunnable(l2);
				DPRINTF(("%d ", l2->l_lid));
			}
			SCHED_UNLOCK(s);
			DPRINTF(("\n"));

			/*
			 * Clear wokenq, the LWPs on the queue will
			 * run below.
			 */
			vp->savp_wokenq_head = NULL;
		}
	}

retry:
	/*
	 * Interrupt LWPs in interruptable sleep, unsuspend suspended
	 * LWPs, make detached LWPs undetached (so we can wait for
	 * them) and then wait for everyone else to finish.
	 */
	LIST_FOREACH(l2, &p->p_lwps, l_sibling) {
		l2->l_flag &= ~(L_DETACHED|L_SA);

		SCHED_LOCK(s);
		if ((l2->l_stat == LSSLEEP && (l2->l_flag & L_SINTR)) ||
		    l2->l_stat == LSSUSPENDED || l2->l_stat == LSSTOP) {
			setrunnable(l2);
			DPRINTF(("exit_lwps: Made %d.%d runnable\n",
			    p->p_pid, l2->l_lid));
		}
		SCHED_UNLOCK(s);
	}


	while (p->p_nlwps > 1) {
		DPRINTF(("exit_lwps: waiting for %d LWPs (%d runnable, %d zombies)\n",
		    p->p_nlwps, p->p_nrlwps, p->p_nzlwps));
		error = lwp_wait1(l, 0, &waited, LWPWAIT_EXITCONTROL);
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

	p->p_userret = NULL;
}

/* Wrapper function for use in p_userret */
static void
lwp_exit_hook(struct lwp *l, void *arg)
{

	KERNEL_PROC_LOCK(l);
	lwp_exit(l);
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

	/*
	 * Collect child u-areas.
	 */
	uvm_uarea_drain(FALSE);

	retval[0] = child->p_pid;

	if (P_ZOMBIE(child)) {
		if (SCARG(uap, status)) {
			status = child->p_xstat;	/* convert to int */
			error = copyout(&status, SCARG(uap, status),
					sizeof(status));
			if (error)
				return (error);
		}
		if (SCARG(uap, rusage)) {
			error = copyout(child->p_ru, SCARG(uap, rusage),
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
		return copyout(&status, SCARG(uap, status), sizeof(status));
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
	int error;

	for (;;) {
		proclist_lock_read();
		error = ECHILD;
		LIST_FOREACH(child, &parent->p_children, p_sibling) {
			if (pid >= 0) {
				if (child->p_pid != pid) {
					child = p_find(pid, PFIND_ZOMBIE |
								PFIND_LOCKED);
					if (child == NULL
					    || child->p_pptr != parent) {
						child = NULL;
						break;
					}
				}
			} else
				if (pid != WAIT_ANY && child->p_pgid != -pid)
					/* child not in correct pgrp */
					continue;
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
			if (child->p_stat == SZOMB && !(options & WNOZOMBIE))
				break;

			if (child->p_stat == SSTOP &&
			    (child->p_flag & P_WAITED) == 0 &&
			    (child->p_flag & P_TRACED || options & WUNTRACED)) {
				if ((options & WNOWAIT) == 0) {
					child->p_flag |= P_WAITED;
					parent->p_nstopchild--;
				}
				break;
			}
			if (parent->p_nstopchild == 0 || child->p_pid == pid) {
				child = NULL;
				break;
			}
		}
		proclist_unlock_read();
		if (child != NULL || error != 0 || options & WNOHANG) {
			*child_p = child;
			return error;
		}
		error = tsleep(parent, PWAIT | PCATCH, "wait", 0);
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
	ksiginfo_t ksi;
	int s;

	KASSERT(p->p_nlwps == 0);
	KASSERT(p->p_nzlwps == 0);
	KASSERT(p->p_nrlwps == 0);
	KASSERT(LIST_EMPTY(&p->p_lwps));

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
		p->p_flag &= ~(P_TRACED|P_WAITED|P_FSTRACE|P_SYSCALL);
		if (p->p_exitsig != 0) {
			exit_psignal(p, parent, &ksi);
			kpsignal(parent, &ksi, NULL);
		}
		wakeup(parent);
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
	p->p_pptr->p_nstopchild--;
	LIST_REMOVE(p, p_sibling);
	proclist_unlock_write(s);

	/*
	 * Decrement the count of procs running with this uid.
	 */
	(void)chgproccnt(kauth_cred_getuid(p->p_cred), -1);

	/*
	 * Free up credentials.
	 */
	kauth_cred_free(p->p_cred);

	/*
	 * Release reference to text vnode
	 */
	if (p->p_textvp)
		vrele(p->p_textvp);

	/* Release any SA state. */
	if (p->p_sa)
		sa_release(p);

	/* Release remaining substructures. */
	limfree(p->p_limit);
	pstatsfree(p->p_stats);

	/* Free proc structure and let pid be reallocated */
	proc_free_mem(p);
}

/*
 * make process 'parent' the new parent of process 'child'.
 *
 * Must be called with proclist_lock_write() held.
 */
void
proc_reparent(struct proc *child, struct proc *parent)
{

	if (child->p_pptr == parent)
		return;

	if (child->p_stat == SZOMB
	    || (child->p_stat == SSTOP && !(child->p_flag & P_WAITED))) {
		child->p_pptr->p_nstopchild--;
		parent->p_nstopchild++;
	}
	if (parent == initproc)
		child->p_exitsig = SIGCHLD;

	LIST_REMOVE(child, p_sibling);
	LIST_INSERT_HEAD(&parent->p_children, child, p_sibling);
	child->p_pptr = parent;
}
