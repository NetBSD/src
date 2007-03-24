/*	$NetBSD: kern_synch.c,v 1.177.2.20 2007/03/24 17:13:14 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kern_synch.c,v 1.177.2.20 2007/03/24 17:13:14 ad Exp $");

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

#include <uvm/uvm_extern.h>

int	lbolt;			/* once a second sleep address */

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
	int error, catch;

	if (sleepq_dontsleep(l)) {
		(void)sleepq_abort(NULL, 0);
		if ((priority & PNORELOCK) != 0)
			simple_unlock(interlock);
		return 0;
	}

	sq = sleeptab_lookup(&sleeptab, ident);
	sleepq_enter(sq, l);

	if (interlock != NULL) {
		LOCK_ASSERT(simple_lock_held(interlock));
		simple_unlock(interlock);
	}

	catch = priority & PCATCH;
	sleepq_block(sq, priority & PRIMASK, ident, wmesg, timo, catch,
	    &sleep_syncobj);
	error = sleepq_unblock(timo, catch);

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
	int error, catch;

	if (sleepq_dontsleep(l)) {
		(void)sleepq_abort(mtx, (priority & PNORELOCK) != 0);
		return 0;
	}

	sq = sleeptab_lookup(&sleeptab, ident);
	sleepq_enter(sq, l);
	mutex_exit(mtx);

	catch = priority & PCATCH;
	sleepq_block(sq, priority & PRIMASK, ident, wmesg, timo, catch,
	    &sleep_syncobj);
	error = sleepq_unblock(timo, catch);

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
	sq = sleeptab_lookup(&sleeptab, l);
	sleepq_enter(sq, l);
	sleepq_block(sq, sched_kpri(l), l, wmesg, timo, intr, &sleep_syncobj);
	error = sleepq_unblock(timo, intr);
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
 * current process explicitly requests it (eg sched_yield(2) in compat code).
 */
void
yield(void)
{
	struct lwp *l = curlwp;

	KERNEL_UNLOCK_ALL(l, &l->l_biglocks);
	lwp_lock(l);
	if (l->l_stat == LSONPROC) {
		KASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));
		l->l_priority = l->l_usrpri;
	}
	l->l_nvcsw++;
	mi_switch(l);
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
	if (l->l_stat == LSONPROC) {
		KASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));
		l->l_priority = l->l_usrpri;
	}
	l->l_nivcsw++;
	(void)mi_switch(l);
	KERNEL_LOCK(l->l_biglocks, l);
}

/*
 * Compute the amount of time during which the current lwp was running.
 *
 * - update l_rtime unless it's an idle lwp.
 * - update spc_runtime for the next lwp.
 */

static inline void
updatertime(struct lwp *l, struct schedstate_percpu *spc)
{
	struct timeval tv;
	long s, u;

	if ((l->l_flag & LW_IDLE) != 0) {
		microtime(&spc->spc_runtime);
		return;
	}

	microtime(&tv);
	u = l->l_rtime.tv_usec + (tv.tv_usec - spc->spc_runtime.tv_usec);
	s = l->l_rtime.tv_sec + (tv.tv_sec - spc->spc_runtime.tv_sec);
	if (u < 0) {
		u += 1000000;
		s--;
	} else if (u >= 1000000) {
		u -= 1000000;
		s++;
	}
	l->l_rtime.tv_usec = u;
	l->l_rtime.tv_sec = s;

	spc->spc_runtime = tv;
}

/*
 * The machine independent parts of context switch.
 *
 * Returns 1 if another process was actually run.
 */
int
mi_switch(struct lwp *l)
{
	struct schedstate_percpu *spc;
	struct lwp *newl;
	int retval, oldspl;

	LOCK_ASSERT(lwp_locked(l, NULL));

#ifdef LOCKDEBUG
	spinlock_switchcheck();
	simple_lock_switchcheck();
#endif
#ifdef KSTACK_CHECK_MAGIC
	kstack_check_magic(l);
#endif

	/*
	 * It's safe to read the per CPU schedstate unlocked here, as all we
	 * are after is the run time and that's guarenteed to have been last
	 * updated by this CPU.
	 */
	KDASSERT(l->l_cpu == curcpu());
	spc = &l->l_cpu->ci_schedstate;

	/* Count time spent in current system call */
	SYSCALL_TIME_SLEEP(l);

	/*
	 * XXXSMP If we are using h/w performance counters, save context.
	 */
#if PERFCTRS
	if (PMC_ENABLED(l->l_proc)) {
		pmc_save_context(l->l_proc);
	}
#endif

	/*
	 * If on the CPU and we have gotten this far, then we must yield.
	 */
	KASSERT(l->l_stat != LSRUN);
	if (l->l_stat == LSONPROC) {
		KASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));
		l->l_stat = LSRUN;
		if ((l->l_flag & LW_IDLE) == 0) {
			sched_enqueue(l, true);
		}
	}

	/*
	 * Process is about to yield the CPU; clear the appropriate
	 * scheduling flags.
	 */
	spc->spc_flags &= ~SPCF_SWITCHCLEAR;

	LOCKDEBUG_BARRIER(l->l_mutex, 1);

	/*
	 * Acquire the spc_mutex if necessary.
	 */
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	if (l->l_mutex != spc->spc_mutex) {
		mutex_enter(spc->spc_mutex);
	}
#endif
	/*
	 * Let sched_nextlwp() select the LWP to run the CPU next. 
	 * If no LWP is runnable, switch to the idle LWP.
	 */
	newl = sched_nextlwp();
	if (newl) {
		sched_dequeue(newl);
	} else {
		newl = l->l_cpu->ci_data.cpu_idlelwp;
		KASSERT(newl != NULL);
	}
	KASSERT(lwp_locked(newl, spc->spc_mutex));
	newl->l_stat = LSONPROC;
	newl->l_cpu = l->l_cpu;
	newl->l_flag |= LW_RUNNING;

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	if (l->l_mutex != spc->spc_mutex) {
		mutex_exit(spc->spc_mutex);
	}
#endif

	updatertime(l, spc);
	if (l != newl) {
		struct lwp *prevlwp;

		/* Unlocked, but for statistics only. */
		uvmexp.swtch++;

		/* Save old VM context. */
		pmap_deactivate(l);

		/* Switch to the new LWP.. */
		l->l_flag &= ~LW_RUNNING;
		oldspl = MUTEX_SPIN_OLDSPL(l->l_cpu);
		prevlwp = cpu_switchto(l, newl);

		/* .. we have switched. */
		curlwp = l;
		if (prevlwp != NULL) {
			curcpu()->ci_mtx_oldspl = oldspl;
			lwp_unlock(prevlwp);
		} else {
			splx(oldspl);
		}

		/* Restore VM context. */
		pmap_activate(l);
		retval = 1;
	} else {
		/* Nothing to do - just unlock and return. */
		lwp_unlock(l);
		retval = 0;
	}

	KASSERT(l == curlwp);
	KASSERT(l->l_stat == LSONPROC);

	/*
	 * XXXSMP If we are using h/w performance counters, restore context.
	 */
#if PERFCTRS
	if (PMC_ENABLED(l->l_proc)) {
		pmc_restore_context(l->l_proc);
	}
#endif

	/*
	 * We're running again; record our new start time.  We might
	 * be running on a new CPU now, so don't use the cached
	 * schedstate_percpu pointer.
	 */
	SYSCALL_TIME_WAKEUP(l);
	KDASSERT(l->l_cpu == curcpu());
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
	sigset_t *ss;

	KASSERT((l->l_flag & LW_IDLE) == 0);
	KASSERT(mutex_owned(&p->p_smutex));
	KASSERT(lwp_locked(l, NULL));

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

	LOCK_ASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));

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
	 * Set the LWP runnable.  If it's swapped out, we need to wake the swapper
	 * to bring it back in.  Otherwise, enter it into a run queue.
	 */
	sched_setrunnable(l);
	l->l_stat = LSRUN;
	l->l_slptime = 0;

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
#ifdef MULTIPROCESSOR
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
#endif
	struct lwp *l;
	struct proc *p;

	/*
	 * We do this by process in order not to violate the locking rules.
	 */
	mutex_enter(&proclist_mutex);
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
	mutex_exit(&proclist_mutex);

	/*
	 * Kick all CPUs to make them preempt any LWPs running in user mode. 
	 * They'll trap into the kernel and suspend themselves in userret().
	 */
#ifdef MULTIPROCESSOR
	for (CPU_INFO_FOREACH(cii, ci))
		cpu_need_resched(ci, 0);
#else
	cpu_need_resched(curcpu(), 0);
#endif
}

/*
 * sched_kpri:
 *
 *	Scale a priority level to a kernel priority level, usually
 *	for an LWP that is about to sleep.
 */
pri_t
sched_kpri(struct lwp *l)
{
	/*
	 * Scale user priorities (127 -> 50) up to kernel priorities
	 * in the range (49 -> 8).  Reserve the top 8 kernel priorities
	 * for high priority kthreads.  Kernel priorities passed in
	 * are left "as is".  XXX This is somewhat arbitrary.
	 */
	static const uint8_t kpri_tab[] = {
		 0,   1,   2,   3,   4,   5,   6,   7,
		 8,   9,  10,  11,  12,  13,  14,  15,
		16,  17,  18,  19,  20,  21,  22,  23,
		24,  25,  26,  27,  28,  29,  30,  31,
		32,  33,  34,  35,  36,  37,  38,  39,
		40,  41,  42,  43,  44,  45,  46,  47,
		48,  49,   8,   8,   9,   9,  10,  10,
		11,  11,  12,  12,  13,  14,  14,  15,
		15,  16,  16,  17,  17,  18,  18,  19,
		20,  20,  21,  21,  22,  22,  23,  23,
		24,  24,  25,  26,  26,  27,  27,  28,
		28,  29,  29,  30,  30,  31,  32,  32,
		33,  33,  34,  34,  35,  35,  36,  36,
		37,  38,  38,  39,  39,  40,  40,  41,
		41,  42,  42,  43,  44,  44,  45,  45,
		46,  46,  47,  47,  48,  48,  49,  49,
	};

	return (pri_t)kpri_tab[l->l_usrpri];
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

inline void
resched_cpu(struct lwp *l)
{
	struct cpu_info *ci;
	const pri_t pri = lwp_eprio(l);

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
	ci = (l->l_cpu != NULL) ? l->l_cpu : curcpu();
	if (pri < ci->ci_schedstate.spc_curpriority)
		cpu_need_resched(ci, 0);
}

static void
sched_changepri(struct lwp *l, pri_t pri)
{

	LOCK_ASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));

	l->l_usrpri = pri;
	if (l->l_priority < PUSER)
		return;

	if (l->l_stat != LSRUN || (l->l_flag & LW_INMEM) == 0) {
		l->l_priority = pri;
		return;
	}

	sched_dequeue(l);
	l->l_priority = pri;
	sched_enqueue(l, false);
	resched_cpu(l);
}

static void
sched_lendpri(struct lwp *l, pri_t pri)
{

	LOCK_ASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));

	if (l->l_stat != LSRUN || (l->l_flag & LW_INMEM) == 0) {
		l->l_inheritedprio = pri;
		return;
	}

	sched_dequeue(l);
	l->l_inheritedprio = pri;
	sched_enqueue(l, false);
	resched_cpu(l);
}

struct lwp *
syncobj_noowner(wchan_t wchan)
{

	return NULL;
}
