/*	$NetBSD: kern_synch.c,v 1.216 2008/01/15 03:37:11 rmind Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2004, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, by Charles M. Hannum, Andrew Doran and
 * Daniel Sieger.
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

/*-
 * Copyright (c) 1982, 1986, 1990, 1991, 1993
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
 *	@(#)kern_synch.c	8.9 (Berkeley) 5/19/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_synch.c,v 1.216 2008/01/15 03:37:11 rmind Exp $");

#include "opt_kstack.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_perfctrs.h"

#define	__MUTEX_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#if defined(PERFCTRS)
#include <sys/pmc.h>
#endif
#include <sys/cpu.h>
#include <sys/resourcevar.h>
#include <sys/sched.h>
#include <sys/syscall_stats.h>
#include <sys/sleepq.h>
#include <sys/lockdebug.h>
#include <sys/evcnt.h>
#include <sys/intr.h>
#include <sys/lwpctl.h>
#include <sys/atomic.h>
#include <sys/simplelock.h>

#include <uvm/uvm_extern.h>

callout_t sched_pstats_ch;
unsigned int sched_pstats_ticks;

kcondvar_t	lbolt;			/* once a second sleep address */

static void	sched_unsleep(struct lwp *);
static void	sched_changepri(struct lwp *, pri_t);
static void	sched_lendpri(struct lwp *, pri_t);

syncobj_t sleep_syncobj = {
	SOBJ_SLEEPQ_SORTED,
	sleepq_unsleep,
	sleepq_changepri,
	sleepq_lendpri,
	syncobj_noowner,
};

syncobj_t sched_syncobj = {
	SOBJ_SLEEPQ_SORTED,
	sched_unsleep,
	sched_changepri,
	sched_lendpri,
	syncobj_noowner,
};

/*
 * During autoconfiguration or after a panic, a sleep will simply lower the
 * priority briefly to allow interrupts, then return.  The priority to be
 * used (safepri) is machine-dependent, thus this value is initialized and
 * maintained in the machine-dependent layers.  This priority will typically
 * be 0, or the lowest priority that is safe for use on the interrupt stack;
 * it can be made higher to block network software interrupts after panics.
 */
int	safepri;

/*
 * OBSOLETE INTERFACE
 *
 * General sleep call.  Suspends the current process until a wakeup is
 * performed on the specified identifier.  The process will then be made
 * runnable with the specified priority.  Sleeps at most timo/hz seconds (0
 * means no timeout).  If pri includes PCATCH flag, signals are checked
 * before and after sleeping, else signals are not checked.  Returns 0 if
 * awakened, EWOULDBLOCK if the timeout expires.  If PCATCH is set and a
 * signal needs to be delivered, ERESTART is returned if the current system
 * call should be restarted if possible, and EINTR is returned if the system
 * call should be interrupted by the signal (return EINTR).
 *
 * The interlock is held until we are on a sleep queue. The interlock will
 * be locked before returning back to the caller unless the PNORELOCK flag
 * is specified, in which case the interlock will always be unlocked upon
 * return.
 */
int
ltsleep(wchan_t ident, pri_t priority, const char *wmesg, int timo,
	volatile struct simplelock *interlock)
{
	struct lwp *l = curlwp;
	sleepq_t *sq;
	int error;

	KASSERT((l->l_pflag & LP_INTR) == 0);

	if (sleepq_dontsleep(l)) {
		(void)sleepq_abort(NULL, 0);
		if ((priority & PNORELOCK) != 0)
			simple_unlock(interlock);
		return 0;
	}

	l->l_kpriority = true;
	sq = sleeptab_lookup(&sleeptab, ident);
	sleepq_enter(sq, l);
	sleepq_enqueue(sq, ident, wmesg, &sleep_syncobj);

	if (interlock != NULL) {
		KASSERT(simple_lock_held(interlock));
		simple_unlock(interlock);
	}

	error = sleepq_block(timo, priority & PCATCH);

	if (interlock != NULL && (priority & PNORELOCK) == 0)
		simple_lock(interlock);
 
	return error;
}

int
mtsleep(wchan_t ident, pri_t priority, const char *wmesg, int timo,
	kmutex_t *mtx)
{
	struct lwp *l = curlwp;
	sleepq_t *sq;
	int error;

	KASSERT((l->l_pflag & LP_INTR) == 0);

	if (sleepq_dontsleep(l)) {
		(void)sleepq_abort(mtx, (priority & PNORELOCK) != 0);
		return 0;
	}

	l->l_kpriority = true;
	sq = sleeptab_lookup(&sleeptab, ident);
	sleepq_enter(sq, l);
	sleepq_enqueue(sq, ident, wmesg, &sleep_syncobj);
	mutex_exit(mtx);
	error = sleepq_block(timo, priority & PCATCH);

	if ((priority & PNORELOCK) == 0)
		mutex_enter(mtx);
 
	return error;
}

/*
 * General sleep call for situations where a wake-up is not expected.
 */
int
kpause(const char *wmesg, bool intr, int timo, kmutex_t *mtx)
{
	struct lwp *l = curlwp;
	sleepq_t *sq;
	int error;

	if (sleepq_dontsleep(l))
		return sleepq_abort(NULL, 0);

	if (mtx != NULL)
		mutex_exit(mtx);
	l->l_kpriority = true;
	sq = sleeptab_lookup(&sleeptab, l);
	sleepq_enter(sq, l);
	sleepq_enqueue(sq, l, wmesg, &sleep_syncobj);
	error = sleepq_block(timo, intr);
	if (mtx != NULL)
		mutex_enter(mtx);

	return error;
}

/*
 * OBSOLETE INTERFACE
 *
 * Make all processes sleeping on the specified identifier runnable.
 */
void
wakeup(wchan_t ident)
{
	sleepq_t *sq;

	if (cold)
		return;

	sq = sleeptab_lookup(&sleeptab, ident);
	sleepq_wake(sq, ident, (u_int)-1);
}

/*
 * OBSOLETE INTERFACE
 *
 * Make the highest priority process first in line on the specified
 * identifier runnable.
 */
void 
wakeup_one(wchan_t ident)
{
	sleepq_t *sq;

	if (cold)
		return;

	sq = sleeptab_lookup(&sleeptab, ident);
	sleepq_wake(sq, ident, 1);
}


/*
 * General yield call.  Puts the current process back on its run queue and
 * performs a voluntary context switch.  Should only be called when the
 * current process explicitly requests it (eg sched_yield(2)).
 */
void
yield(void)
{
	struct lwp *l = curlwp;

	KERNEL_UNLOCK_ALL(l, &l->l_biglocks);
	lwp_lock(l);
	KASSERT(lwp_locked(l, &l->l_cpu->ci_schedstate.spc_lwplock));
	KASSERT(l->l_stat == LSONPROC);
	l->l_kpriority = false;
	if (l->l_class == SCHED_OTHER) {
		/*
		 * Only for timeshared threads.  It will be reset
		 * by the scheduler in due course.
		 */
		l->l_priority = 0;
	}
	(void)mi_switch(l);
	KERNEL_LOCK(l->l_biglocks, l);
}

/*
 * General preemption call.  Puts the current process back on its run queue
 * and performs an involuntary context switch.
 */
void
preempt(void)
{
	struct lwp *l = curlwp;

	KERNEL_UNLOCK_ALL(l, &l->l_biglocks);
	lwp_lock(l);
	KASSERT(lwp_locked(l, &l->l_cpu->ci_schedstate.spc_lwplock));
	KASSERT(l->l_stat == LSONPROC);
	l->l_kpriority = false;
	l->l_nivcsw++;
	(void)mi_switch(l);
	KERNEL_LOCK(l->l_biglocks, l);
}

/*
 * Compute the amount of time during which the current lwp was running.
 *
 * - update l_rtime unless it's an idle lwp.
 */

void
updatertime(lwp_t *l, const struct bintime *now)
{

	if ((l->l_flag & LW_IDLE) != 0)
		return;

	/* rtime += now - stime */
	bintime_add(&l->l_rtime, now);
	bintime_sub(&l->l_rtime, &l->l_stime);
}

/*
 * The machine independent parts of context switch.
 *
 * Returns 1 if another LWP was actually run.
 */
int
mi_switch(lwp_t *l)
{
	struct cpu_info *ci, *tci = NULL;
	struct schedstate_percpu *spc;
	struct lwp *newl;
	int retval, oldspl;
	struct bintime bt;
	bool returning;

	KASSERT(lwp_locked(l, NULL));
	LOCKDEBUG_BARRIER(l->l_mutex, 1);

#ifdef KSTACK_CHECK_MAGIC
	kstack_check_magic(l);
#endif

	binuptime(&bt);

	KDASSERT(l->l_cpu == curcpu());
	ci = l->l_cpu;
	spc = &ci->ci_schedstate;
	returning = false;
	newl = NULL;

	/*
	 * If we have been asked to switch to a specific LWP, then there
	 * is no need to inspect the run queues.  If a soft interrupt is
	 * blocking, then return to the interrupted thread without adjusting
	 * VM context or its start time: neither have been changed in order
	 * to take the interrupt.
	 */
	if (l->l_switchto != NULL) {
		if ((l->l_pflag & LP_INTR) != 0) {
			returning = true;
			softint_block(l);
			if ((l->l_flag & LW_TIMEINTR) != 0)
				updatertime(l, &bt);
		}
		newl = l->l_switchto;
		l->l_switchto = NULL;
	}
#ifndef __HAVE_FAST_SOFTINTS
	else if (ci->ci_data.cpu_softints != 0) {
		/* There are pending soft interrupts, so pick one. */
		newl = softint_picklwp();
		newl->l_stat = LSONPROC;
		newl->l_flag |= LW_RUNNING;
	}
#endif	/* !__HAVE_FAST_SOFTINTS */

	/* Count time spent in current system call */
	if (!returning) {
		SYSCALL_TIME_SLEEP(l);

		/*
		 * XXXSMP If we are using h/w performance counters,
		 * save context.
		 */
#if PERFCTRS
		if (PMC_ENABLED(l->l_proc)) {
			pmc_save_context(l->l_proc);
		}
#endif
		updatertime(l, &bt);
	}

	/*
	 * If on the CPU and we have gotten this far, then we must yield.
	 */
	KASSERT(l->l_stat != LSRUN);
	if (l->l_stat == LSONPROC && (l->l_target_cpu || l != newl)) {
		KASSERT(lwp_locked(l, &spc->spc_lwplock));

		tci = l->l_target_cpu;
		if (__predict_false(tci != NULL)) {
			/* Double-lock the runqueues */
			spc_dlock(ci, tci);
		} else {
			/* Lock the runqueue */
			spc_lock(ci);
		}

		if ((l->l_flag & LW_IDLE) == 0) {
			l->l_stat = LSRUN;
			if (__predict_false(tci != NULL)) {
				/* 
				 * Set the new CPU, lock and unset the
				 * l_target_cpu - thread will be enqueued
				 * to the runqueue of target CPU.
				 */
				l->l_cpu = tci;
				lwp_setlock(l, tci->ci_schedstate.spc_mutex);
				l->l_target_cpu = NULL;
			} else {
				lwp_setlock(l, spc->spc_mutex);
			}
			sched_enqueue(l, true);
		} else {
			KASSERT(tci == NULL);
			l->l_stat = LSIDL;
		}
	} else {
		/* Lock the runqueue */
		spc_lock(ci);
	}

	/*
	 * Let sched_nextlwp() select the LWP to run the CPU next.
	 * If no LWP is runnable, select the idle LWP.
	 * 
	 * Note that spc_lwplock might not necessary be held, and
	 * new thread would be unlocked after setting the LWP-lock.
	 */
	if (newl == NULL) {
		newl = sched_nextlwp();
		if (newl != NULL) {
			sched_dequeue(newl);
			KASSERT(lwp_locked(newl, spc->spc_mutex));
			newl->l_stat = LSONPROC;
			newl->l_cpu = ci;
			newl->l_flag |= LW_RUNNING;
			lwp_setlock(newl, &spc->spc_lwplock);
		} else {
			newl = ci->ci_data.cpu_idlelwp;
			newl->l_stat = LSONPROC;
			newl->l_flag |= LW_RUNNING;
		}
		/*
		 * Only clear want_resched if there are no
		 * pending (slow) software interrupts.
		 */
		ci->ci_want_resched = ci->ci_data.cpu_softints;
		spc->spc_flags &= ~SPCF_SWITCHCLEAR;
		spc->spc_curpriority = lwp_eprio(newl);
	}

	/* Items that must be updated with the CPU locked. */
	if (!returning) {
		/* Update the new LWP's start time. */
		newl->l_stime = bt;

		/*
		 * ci_curlwp changes when a fast soft interrupt occurs.
		 * We use cpu_onproc to keep track of which kernel or
		 * user thread is running 'underneath' the software
		 * interrupt.  This is important for time accounting,
		 * itimers and forcing user threads to preempt (aston).
		 */
		ci->ci_data.cpu_onproc = newl;
	}

	if (l != newl) {
		struct lwp *prevlwp;

		/* Release all locks, but leave the current LWP locked */
		if (l->l_mutex == l->l_cpu->ci_schedstate.spc_mutex) {
			/*
			 * In case of migration, drop the local runqueue
			 * lock, thread is on other runqueue now.
			 */
			if (__predict_false(tci != NULL))
				spc_unlock(ci);
			/*
			 * Drop spc_lwplock, if the current LWP has been moved
			 * to the run queue (it is now locked by spc_mutex).
			 */
			mutex_spin_exit(&spc->spc_lwplock);
		} else {
			/*
			 * Otherwise, drop the spc_mutex, we are done with the
			 * run queues.
			 */
			mutex_spin_exit(spc->spc_mutex);
			KASSERT(tci == NULL);
		}

		/*
		 * Mark that context switch is going to be perfomed
		 * for this LWP, to protect it from being switched
		 * to on another CPU.
		 */
		KASSERT(l->l_ctxswtch == 0);
		l->l_ctxswtch = 1;
		l->l_ncsw++;
		l->l_flag &= ~LW_RUNNING;

		/*
		 * Increase the count of spin-mutexes before the release
		 * of the last lock - we must remain at IPL_SCHED during
		 * the context switch.
		 */
		oldspl = MUTEX_SPIN_OLDSPL(ci);
		ci->ci_mtx_count--;
		lwp_unlock(l);

		/* Unlocked, but for statistics only. */
		uvmexp.swtch++;

		/* Update status for lwpctl, if present. */
		if (l->l_lwpctl != NULL)
			l->l_lwpctl->lc_curcpu = LWPCTL_CPU_NONE;

		/*
		 * Save old VM context, unless a soft interrupt
		 * handler is blocking.
		 */
		if (!returning)
			pmap_deactivate(l);

		/*
		 * We may need to spin-wait for if 'newl' is still
		 * context switching on another CPU.
		 */
		if (newl->l_ctxswtch != 0) {
			u_int count;
			count = SPINLOCK_BACKOFF_MIN;
			while (newl->l_ctxswtch)
				SPINLOCK_BACKOFF(count);
		}

		/* Switch to the new LWP.. */
		prevlwp = cpu_switchto(l, newl, returning);
		ci = curcpu();

		/*
		 * Switched away - we have new curlwp.
		 * Restore VM context and IPL.
		 */
		pmap_activate(l);
		if (prevlwp != NULL) {
			/* Normalize the count of the spin-mutexes */
			ci->ci_mtx_count++;
			/* Unmark the state of context switch */
			membar_exit();
			prevlwp->l_ctxswtch = 0;
		}
		splx(oldspl);

		/* Update status for lwpctl, if present. */
		if (l->l_lwpctl != NULL)
			l->l_lwpctl->lc_curcpu = (int)cpu_index(ci);

		retval = 1;
	} else {
		/* Nothing to do - just unlock and return. */
		KASSERT(tci == NULL);
		spc_unlock(ci);
		lwp_unlock(l);
		retval = 0;
	}

	KASSERT(l == curlwp);
	KASSERT(l->l_stat == LSONPROC);
	KASSERT(l->l_cpu == ci);

	/*
	 * XXXSMP If we are using h/w performance counters, restore context.
	 */
#if PERFCTRS
	if (PMC_ENABLED(l->l_proc)) {
		pmc_restore_context(l->l_proc);
	}
#endif
	SYSCALL_TIME_WAKEUP(l);
	LOCKDEBUG_BARRIER(NULL, 1);

	return retval;
}

/*
 * Change process state to be runnable, placing it on the run queue if it is
 * in memory, and awakening the swapper if it isn't in memory.
 *
 * Call with the process and LWP locked.  Will return with the LWP unlocked.
 */
void
setrunnable(struct lwp *l)
{
	struct proc *p = l->l_proc;
	struct cpu_info *ci;
	sigset_t *ss;

	KASSERT((l->l_flag & LW_IDLE) == 0);
	KASSERT(mutex_owned(&p->p_smutex));
	KASSERT(lwp_locked(l, NULL));
	KASSERT(l->l_mutex != l->l_cpu->ci_schedstate.spc_mutex);

	switch (l->l_stat) {
	case LSSTOP:
		/*
		 * If we're being traced (possibly because someone attached us
		 * while we were stopped), check for a signal from the debugger.
		 */
		if ((p->p_slflag & PSL_TRACED) != 0 && p->p_xstat != 0) {
			if ((sigprop[p->p_xstat] & SA_TOLWP) != 0)
				ss = &l->l_sigpend.sp_set;
			else
				ss = &p->p_sigpend.sp_set;
			sigaddset(ss, p->p_xstat);
			signotify(l);
		}
		p->p_nrlwps++;
		break;
	case LSSUSPENDED:
		l->l_flag &= ~LW_WSUSPEND;
		p->p_nrlwps++;
		cv_broadcast(&p->p_lwpcv);
		break;
	case LSSLEEP:
		KASSERT(l->l_wchan != NULL);
		break;
	default:
		panic("setrunnable: lwp %p state was %d", l, l->l_stat);
	}

	/*
	 * If the LWP was sleeping interruptably, then it's OK to start it
	 * again.  If not, mark it as still sleeping.
	 */
	if (l->l_wchan != NULL) {
		l->l_stat = LSSLEEP;
		/* lwp_unsleep() will release the lock. */
		lwp_unsleep(l);
		return;
	}

	/*
	 * If the LWP is still on the CPU, mark it as LSONPROC.  It may be
	 * about to call mi_switch(), in which case it will yield.
	 */
	if ((l->l_flag & LW_RUNNING) != 0) {
		l->l_stat = LSONPROC;
		l->l_slptime = 0;
		lwp_unlock(l);
		return;
	}

	/*
	 * Look for a CPU to run.
	 * Set the LWP runnable.
	 */
	ci = sched_takecpu(l);
	l->l_cpu = ci;
	if (l->l_mutex != l->l_cpu->ci_schedstate.spc_mutex) {
		lwp_unlock_to(l, ci->ci_schedstate.spc_mutex);
		lwp_lock(l);
	}
	sched_setrunnable(l);
	l->l_stat = LSRUN;
	l->l_slptime = 0;

	/*
	 * If thread is swapped out - wake the swapper to bring it back in.
	 * Otherwise, enter it into a run queue.
	 */
	if (l->l_flag & LW_INMEM) {
		sched_enqueue(l, false);
		resched_cpu(l);
		lwp_unlock(l);
	} else {
		lwp_unlock(l);
		uvm_kick_scheduler();
	}
}

/*
 * suspendsched:
 *
 *	Convert all non-L_SYSTEM LSSLEEP or LSRUN LWPs to LSSUSPENDED. 
 */
void
suspendsched(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	struct lwp *l;
	struct proc *p;

	/*
	 * We do this by process in order not to violate the locking rules.
	 */
	mutex_enter(&proclist_lock);
	PROCLIST_FOREACH(p, &allproc) {
		mutex_enter(&p->p_smutex);

		if ((p->p_flag & PK_SYSTEM) != 0) {
			mutex_exit(&p->p_smutex);
			continue;
		}

		p->p_stat = SSTOP;

		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			if (l == curlwp)
				continue;

			lwp_lock(l);

			/*
			 * Set L_WREBOOT so that the LWP will suspend itself
			 * when it tries to return to user mode.  We want to
			 * try and get to get as many LWPs as possible to
			 * the user / kernel boundary, so that they will
			 * release any locks that they hold.
			 */
			l->l_flag |= (LW_WREBOOT | LW_WSUSPEND);

			if (l->l_stat == LSSLEEP &&
			    (l->l_flag & LW_SINTR) != 0) {
				/* setrunnable() will release the lock. */
				setrunnable(l);
				continue;
			}

			lwp_unlock(l);
		}

		mutex_exit(&p->p_smutex);
	}
	mutex_exit(&proclist_lock);

	/*
	 * Kick all CPUs to make them preempt any LWPs running in user mode. 
	 * They'll trap into the kernel and suspend themselves in userret().
	 */
	for (CPU_INFO_FOREACH(cii, ci)) {
		spc_lock(ci);
		cpu_need_resched(ci, RESCHED_IMMED);
		spc_unlock(ci);
	}
}

/*
 * sched_unsleep:
 *
 *	The is called when the LWP has not been awoken normally but instead
 *	interrupted: for example, if the sleep timed out.  Because of this,
 *	it's not a valid action for running or idle LWPs.
 */
static void
sched_unsleep(struct lwp *l)
{

	lwp_unlock(l);
	panic("sched_unsleep");
}

void
resched_cpu(struct lwp *l)
{
	struct cpu_info *ci;

	/*
	 * XXXSMP
	 * Since l->l_cpu persists across a context switch,
	 * this gives us *very weak* processor affinity, in
	 * that we notify the CPU on which the process last
	 * ran that it should try to switch.
	 *
	 * This does not guarantee that the process will run on
	 * that processor next, because another processor might
	 * grab it the next time it performs a context switch.
	 *
	 * This also does not handle the case where its last
	 * CPU is running a higher-priority process, but every
	 * other CPU is running a lower-priority process.  There
	 * are ways to handle this situation, but they're not
	 * currently very pretty, and we also need to weigh the
	 * cost of moving a process from one CPU to another.
	 */
	ci = l->l_cpu;
	if (lwp_eprio(l) > ci->ci_schedstate.spc_curpriority)
		cpu_need_resched(ci, 0);
}

static void
sched_changepri(struct lwp *l, pri_t pri)
{

	KASSERT(lwp_locked(l, NULL));

	if (l->l_stat == LSRUN && (l->l_flag & LW_INMEM) != 0) {
		KASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));
		sched_dequeue(l);
		l->l_priority = pri;
		sched_enqueue(l, false);
	} else {
		l->l_priority = pri;
	}
	resched_cpu(l);
}

static void
sched_lendpri(struct lwp *l, pri_t pri)
{

	KASSERT(lwp_locked(l, NULL));

	if (l->l_stat == LSRUN && (l->l_flag & LW_INMEM) != 0) {
		KASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));
		sched_dequeue(l);
		l->l_inheritedprio = pri;
		sched_enqueue(l, false);
	} else {
		l->l_inheritedprio = pri;
	}
	resched_cpu(l);
}

struct lwp *
syncobj_noowner(wchan_t wchan)
{

	return NULL;
}


/* decay 95% of `p_pctcpu' in 60 seconds; see CCPU_SHIFT before changing */
fixpt_t	ccpu = 0.95122942450071400909 * FSCALE;		/* exp(-1/20) */

/*
 * If `ccpu' is not equal to `exp(-1/20)' and you still want to use the
 * faster/more-accurate formula, you'll have to estimate CCPU_SHIFT below
 * and possibly adjust FSHIFT in "param.h" so that (FSHIFT >= CCPU_SHIFT).
 *
 * To estimate CCPU_SHIFT for exp(-1/20), the following formula was used:
 *	1 - exp(-1/20) ~= 0.0487 ~= 0.0488 == 1 (fixed pt, *11* bits).
 *
 * If you dont want to bother with the faster/more-accurate formula, you
 * can set CCPU_SHIFT to (FSHIFT + 1) which will use a slower/less-accurate
 * (more general) method of calculating the %age of CPU used by a process.
 */
#define	CCPU_SHIFT	(FSHIFT + 1)

/*
 * sched_pstats:
 *
 * Update process statistics and check CPU resource allocation.
 * Call scheduler-specific hook to eventually adjust process/LWP
 * priorities.
 */
/* ARGSUSED */
void
sched_pstats(void *arg)
{
	struct rlimit *rlim;
	struct lwp *l;
	struct proc *p;
	int sig, clkhz;
	long runtm;

	sched_pstats_ticks++;

	mutex_enter(&proclist_lock);
	PROCLIST_FOREACH(p, &allproc) {
		/*
		 * Increment time in/out of memory and sleep time (if
		 * sleeping).  We ignore overflow; with 16-bit int's
		 * (remember them?) overflow takes 45 days.
		 */
		mutex_enter(&p->p_smutex);
		mutex_spin_enter(&p->p_stmutex);
		runtm = p->p_rtime.sec;
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			if ((l->l_flag & LW_IDLE) != 0)
				continue;
			lwp_lock(l);
			runtm += l->l_rtime.sec;
			l->l_swtime++;
			sched_pstats_hook(l);
			lwp_unlock(l);

			/*
			 * p_pctcpu is only for ps.
			 */
			l->l_pctcpu = (l->l_pctcpu * ccpu) >> FSHIFT;
			if (l->l_slptime < 1) {
				clkhz = stathz != 0 ? stathz : hz;
#if	(FSHIFT >= CCPU_SHIFT)
				l->l_pctcpu += (clkhz == 100) ?
				    ((fixpt_t)l->l_cpticks) <<
				        (FSHIFT - CCPU_SHIFT) :
				    100 * (((fixpt_t) p->p_cpticks)
				        << (FSHIFT - CCPU_SHIFT)) / clkhz;
#else
				l->l_pctcpu += ((FSCALE - ccpu) *
				    (l->l_cpticks * FSCALE / clkhz)) >> FSHIFT;
#endif
				l->l_cpticks = 0;
			}
		}
		p->p_pctcpu = (p->p_pctcpu * ccpu) >> FSHIFT;
		mutex_spin_exit(&p->p_stmutex);

		/*
		 * Check if the process exceeds its CPU resource allocation.
		 * If over max, kill it.
		 */
		rlim = &p->p_rlimit[RLIMIT_CPU];
		sig = 0;
		if (runtm >= rlim->rlim_cur) {
			if (runtm >= rlim->rlim_max)
				sig = SIGKILL;
			else {
				sig = SIGXCPU;
				if (rlim->rlim_cur < rlim->rlim_max)
					rlim->rlim_cur += 5;
			}
		}
		mutex_exit(&p->p_smutex);
		if (sig) {
			mutex_enter(&proclist_mutex);
			psignal(p, sig);
			mutex_exit(&proclist_mutex);
		}
	}
	mutex_exit(&proclist_lock);
	uvm_meter();
	cv_wakeup(&lbolt);
	callout_schedule(&sched_pstats_ch, hz);
}

void
sched_init(void)
{

	cv_init(&lbolt, "lbolt");
	callout_init(&sched_pstats_ch, CALLOUT_MPSAFE);
	callout_setfunc(&sched_pstats_ch, sched_pstats, NULL);
	sched_setup();
	sched_pstats(NULL);
}
