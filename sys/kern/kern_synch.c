/*	$NetBSD: kern_synch.c,v 1.166.2.3 2006/10/24 21:10:21 ad Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2004, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, by Charles M. Hannum, and by Andrew Doran.
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
__KERNEL_RCSID(0, "$NetBSD: kern_synch.c,v 1.166.2.3 2006/10/24 21:10:21 ad Exp $");

#include "opt_ddb.h"
#include "opt_ktrace.h"
#include "opt_kstack.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_perfctrs.h"

#define	__MUTEX_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#if defined(PERFCTRS)
#include <sys/pmc.h>
#endif
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/sched.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/kauth.h>
#include <sys/sleepq.h>
#include <sys/lockdebug.h>

#include <uvm/uvm_extern.h>

#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <machine/cpu.h>

int	lbolt;			/* once a second sleep address */
int	rrticks;		/* number of hardclock ticks per roundrobin() */

/*
 * The global scheduler state.
 */
kmutex_t	sched_mutex;		/* global run queue mutex */
struct prochd	sched_qs[RUNQUE_NQS];	/* run queues */
volatile uint32_t sched_whichqs;	/* bitmap of non-empty queues */

void	schedcpu(void *);
void	updatepri(struct lwp *);
void	sa_awaken(struct lwp *);

struct callout schedcpu_ch = CALLOUT_INITIALIZER_SETFUNC(schedcpu, NULL);
static unsigned int schedcpu_ticks;

/*
 * Force switch among equal priority processes every 100ms.
 * Called from hardclock every hz/10 == rrticks hardclock ticks.
 */
/* ARGSUSED */
void
roundrobin(struct cpu_info *ci)
{
	struct schedstate_percpu *spc = &ci->ci_schedstate;

	spc->spc_rrticks = rrticks;

	if (curlwp != NULL) {
		if (spc->spc_flags & SPCF_SEENRR) {
			/*
			 * The process has already been through a roundrobin
			 * without switching and may be hogging the CPU.
			 * Indicate that the process should yield.
			 */
			spc->spc_flags |= SPCF_SHOULDYIELD;
		} else
			spc->spc_flags |= SPCF_SEENRR;
	}
	cpu_need_resched(curcpu());
}

#define	PPQ	(128 / RUNQUE_NQS)	/* priorities per queue */
#define	NICE_WEIGHT 2			/* priorities per nice level */

#define	ESTCPU_SHIFT	11
#define	ESTCPU_MAX	((NICE_WEIGHT * PRIO_MAX - PPQ) << ESTCPU_SHIFT)
#define	ESTCPULIM(e)	min((e), ESTCPU_MAX)

/*
 * Constants for digital decay and forget:
 *	90% of (p_estcpu) usage in 5 * loadav time
 *	95% of (p_pctcpu) usage in 60 seconds (load insensitive)
 *          Note that, as ps(1) mentions, this can let percentages
 *          total over 100% (I've seen 137.9% for 3 processes).
 *
 * Note that hardclock updates p_estcpu and p_cpticks independently.
 *
 * We wish to decay away 90% of p_estcpu in (5 * loadavg) seconds.
 * That is, the system wants to compute a value of decay such
 * that the following for loop:
 * 	for (i = 0; i < (5 * loadavg); i++)
 * 		p_estcpu *= decay;
 * will compute
 * 	p_estcpu *= 0.1;
 * for all values of loadavg:
 *
 * Mathematically this loop can be expressed by saying:
 * 	decay ** (5 * loadavg) ~= .1
 *
 * The system computes decay as:
 * 	decay = (2 * loadavg) / (2 * loadavg + 1)
 *
 * We wish to prove that the system's computation of decay
 * will always fulfill the equation:
 * 	decay ** (5 * loadavg) ~= .1
 *
 * If we compute b as:
 * 	b = 2 * loadavg
 * then
 * 	decay = b / (b + 1)
 *
 * We now need to prove two things:
 *	1) Given factor ** (5 * loadavg) ~= .1, prove factor == b/(b+1)
 *	2) Given b/(b+1) ** power ~= .1, prove power == (5 * loadavg)
 *
 * Facts:
 *         For x close to zero, exp(x) =~ 1 + x, since
 *              exp(x) = 0! + x**1/1! + x**2/2! + ... .
 *              therefore exp(-1/b) =~ 1 - (1/b) = (b-1)/b.
 *         For x close to zero, ln(1+x) =~ x, since
 *              ln(1+x) = x - x**2/2 + x**3/3 - ...     -1 < x < 1
 *              therefore ln(b/(b+1)) = ln(1 - 1/(b+1)) =~ -1/(b+1).
 *         ln(.1) =~ -2.30
 *
 * Proof of (1):
 *    Solve (factor)**(power) =~ .1 given power (5*loadav):
 *	solving for factor,
 *      ln(factor) =~ (-2.30/5*loadav), or
 *      factor =~ exp(-1/((5/2.30)*loadav)) =~ exp(-1/(2*loadav)) =
 *          exp(-1/b) =~ (b-1)/b =~ b/(b+1).                    QED
 *
 * Proof of (2):
 *    Solve (factor)**(power) =~ .1 given factor == (b/(b+1)):
 *	solving for power,
 *      power*ln(b/(b+1)) =~ -2.30, or
 *      power =~ 2.3 * (b + 1) = 4.6*loadav + 2.3 =~ 5*loadav.  QED
 *
 * Actual power values for the implemented algorithm are as follows:
 *      loadav: 1       2       3       4
 *      power:  5.68    10.32   14.94   19.55
 */

/* calculations for digital decay to forget 90% of usage in 5*loadav sec */
#define	loadfactor(loadav)	(2 * (loadav))

static fixpt_t
decay_cpu(fixpt_t loadfac, fixpt_t estcpu)
{

	if (estcpu == 0) {
		return 0;
	}

#if !defined(_LP64)
	/* avoid 64bit arithmetics. */
#define	FIXPT_MAX ((fixpt_t)((UINTMAX_C(1) << sizeof(fixpt_t) * CHAR_BIT) - 1))
	if (__predict_true(loadfac <= FIXPT_MAX / ESTCPU_MAX)) {
		return estcpu * loadfac / (loadfac + FSCALE);
	}
#endif /* !defined(_LP64) */

	return (uint64_t)estcpu * loadfac / (loadfac + FSCALE);
}

/*
 * For all load averages >= 1 and max p_estcpu of (255 << ESTCPU_SHIFT),
 * sleeping for at least seven times the loadfactor will decay p_estcpu to
 * less than (1 << ESTCPU_SHIFT).
 *
 * note that our ESTCPU_MAX is actually much smaller than (255 << ESTCPU_SHIFT).
 */
static fixpt_t
decay_cpu_batch(fixpt_t loadfac, fixpt_t estcpu, unsigned int n)
{

	if ((n << FSHIFT) >= 7 * loadfac) {
		return 0;
	}

	while (estcpu != 0 && n > 1) {
		estcpu = decay_cpu(loadfac, estcpu);
		n--;
	}

	return estcpu;
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
#define	CCPU_SHIFT	11

/*
 * Recompute process priorities, every hz ticks.
 */
/* ARGSUSED */
void
schedcpu(void *arg)
{
	fixpt_t loadfac = loadfactor(averunnable.ldavg[0]);
	struct rlimit *rlim;
	struct lwp *l;
	struct proc *p;
	int s, minslp;
	int clkhz;
	long runtm;

	schedcpu_ticks++;

	mutex_enter(&proclist_mutex);
	PROCLIST_FOREACH(p, &allproc) {
		/*
		 * Increment time in/out of memory and sleep time (if
		 * sleeping).  We ignore overflow; with 16-bit int's
		 * (remember them?) overflow takes 45 days.
		 *
		 * XXXSMP Should create an activeproc list so that we
		 * don't touch every proc+LWP in the system on a regular
		 * basis. l->l_swtime/l->l_slptime can become deltas.
		 */
		minslp = 2;
		runtm = 0;
		mutex_enter(&p->p_smutex);
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			lwp_lock(l);
			runtm += l->l_rtime.tv_sec;
			l->l_swtime++;
			if (l->l_stat == LSSLEEP || l->l_stat == LSSTOP ||
			    l->l_stat == LSSUSPENDED) {
				l->l_slptime++;
				minslp = min(minslp, l->l_slptime);
			} else
				minslp = 0;
			lwp_unlock(l);
		}
		p->p_pctcpu = (p->p_pctcpu * ccpu) >> FSHIFT;

		/*
		 * Check if the process exceeds its CPU resource allocation.
		 * If over max, kill it.  In any case, if it has run for more
		 * than autonicetime, reduce priority to give others a chance.
		 */
		rlim = &p->p_rlimit[RLIMIT_CPU];
		if (runtm >= rlim->rlim_cur) {
			if (runtm >= rlim->rlim_max)
				psignal(p, SIGKILL);
			else {
				psignal(p, SIGXCPU);
				if (rlim->rlim_cur < rlim->rlim_max)
					rlim->rlim_cur += 5;
			}
		}
		if (autonicetime && runtm > autonicetime && p->p_nice == NZERO
		    && kauth_cred_geteuid(p->p_cred)) {
			p->p_nice = autoniceval + NZERO;
			resetprocpriority(p);
		}

		/*
		 * If the process has slept the entire second,
		 * stop recalculating its priority until it wakes up.
		 */
		if (minslp > 1) {
			mutex_exit(&p->p_smutex);
			continue;
		}
		/* XXXAD lock */
		s = splstatclock();	/* prevent state changes */
		/*
		 * p_pctcpu is only for ps.
		 */
		clkhz = stathz != 0 ? stathz : hz;
#if	(FSHIFT >= CCPU_SHIFT)
		p->p_pctcpu += (clkhz == 100)?
			((fixpt_t) p->p_cpticks) << (FSHIFT - CCPU_SHIFT):
                	100 * (((fixpt_t) p->p_cpticks)
				<< (FSHIFT - CCPU_SHIFT)) / clkhz;
#else
		p->p_pctcpu += ((FSCALE - ccpu) *
			(p->p_cpticks * FSCALE / clkhz)) >> FSHIFT;
#endif
		p->p_cpticks = 0;
		p->p_estcpu = decay_cpu(loadfac, p->p_estcpu);
		splx(s);	/* Done with the process CPU ticks update */
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			lwp_lock(l);
			if (l->l_slptime > 1) {
				lwp_unlock(l);
				continue;
			}
			resetpriority(l);
			if (l->l_priority >= PUSER) {
				if (l->l_stat == LSRUN &&
				    (l->l_flag & L_INMEM) &&
				    (l->l_priority / PPQ) != (l->l_usrpri / PPQ)) {
					remrunqueue(l);
					l->l_priority = l->l_usrpri;
					setrunqueue(l);
				} else
					l->l_priority = l->l_usrpri;
			}
			lwp_unlock(l);
		}
		mutex_exit(&p->p_smutex);
	}
	mutex_exit(&proclist_mutex);
	uvm_meter();
	wakeup((caddr_t)&lbolt);
	callout_schedule(&schedcpu_ch, hz);
}

/*
 * Recalculate the priority of a process after it has slept for a while.
 */
void
updatepri(struct lwp *l)
{
	struct proc *p = l->l_proc;
	fixpt_t loadfac;

	LOCK_ASSERT(lwp_locked(l, NULL));
	KASSERT(l->l_slptime > 1);

	loadfac = loadfactor(averunnable.ldavg[0]);

	l->l_slptime--; /* the first time was done in schedcpu */
	/* XXX NJWLWP */
	/* XXXSMP occasionaly unlocked. */
	p->p_estcpu = decay_cpu_batch(loadfac, p->p_estcpu, l->l_slptime);
	resetpriority(l);
}

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
 * ltsleep: see mtsleep() for comments.
 */
int
ltsleep(wchan_t ident, int priority, const char *wmesg, int timo,
	volatile struct simplelock *interlock)
{
	struct lwp *l = curlwp;
	sleepq_t *sq;
	int error;

	if (sleepq_dontsleep(l)) {
		(void)sleepq_abort(NULL, 0);
		if ((priority & PNORELOCK) != 0)
			simple_unlock(interlock);
		return 0;
	}

	sq = sleeptab_lookup(ident);
	sleepq_enter(sq, priority, ident, wmesg, timo, priority & PCATCH);

	if (interlock != NULL) {
		LOCK_ASSERT(simple_lock_held(interlock));
		simple_unlock(interlock);
	}

	error = sleepq_block(sq, timo);

	if (interlock != NULL && (priority & PNORELOCK) == 0)
		simple_lock(interlock);
 
	return error;
}

/*
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
mtsleep(wchan_t ident, int priority, const char *wmesg, int timo,
	kmutex_t *mtx)
{
	struct lwp *l = curlwp;
	sleepq_t *sq;
	int error;

	if (sleepq_dontsleep(l))
		return sleepq_abort(mtx, priority & PNORELOCK);

	sq = sleeptab_lookup(ident);
	sleepq_enter(sq, priority, ident, wmesg, timo, priority & PCATCH);

	if (mtx != NULL) {
		LOCK_ASSERT(mutex_owned(mtx));
		mutex_exit_linked(mtx, l->l_mutex);
	}

	error = sleepq_block(sq, timo);

	if (mtx != NULL && (priority & PNORELOCK) == 0)
		mutex_enter(mtx);
 
	return error;
}

void
sa_awaken(struct lwp *l)
{

	LOCK_ASSERT(lwp_locked(l, NULL));

	if (l == l->l_savp->savp_lwp && l->l_flag & L_SA_YIELD)
		l->l_flag &= ~L_SA_IDLE;
}

/*
 * Make all processes sleeping on the specified identifier runnable.
 */
void
wakeup(wchan_t ident)
{
	sleepq_t *sq;

	if (cold)
		return;

	sq = sleeptab_lookup(ident);
	sleepq_wakeall(sq, ident, (u_int)-1);
}

/*
 * Make the highest priority process first in line on the specified
 * identifier runnable.
 */
void 
wakeup_one(wchan_t ident)
{
	sleepq_t *sq;

	if (cold)
		return;
	
	sq = sleeptab_lookup(ident);
	sleepq_wakeone(sq, ident);
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

	lwp_lock(l);
	if (l->l_stat == LSONPROC) {
		KASSERT(lwp_locked(l, &sched_mutex));
		l->l_priority = l->l_usrpri;
	}
	l->l_nvcsw++;
	mi_switch(l, NULL);
}

/*
 * General preemption call.  Puts the current process back on its run queue
 * and performs an involuntary context switch.
 * The 'more' ("more work to do") argument is boolean. Returning to userspace
 * preempt() calls pass 0. "Voluntary" preemptions in e.g. uiomove() pass 1.
 * This will be used to indicate to the SA subsystem that the LWP is
 * not yet finished in the kernel.
 */
void
preempt(int more)
{
	struct lwp *l = curlwp;
	int r;

	lwp_lock(l);
	if (l->l_stat == LSONPROC) {
		KASSERT(lwp_locked(l, &sched_mutex));
		l->l_priority = l->l_usrpri;
	}
	l->l_nivcsw++;
	r = mi_switch(l, NULL);
	if ((l->l_flag & L_SA) != 0 && r != 0 && more == 0)	/* XXXAD */
		sa_preempt(l);
}

/*
 * The machine independent parts of context switch.  Switch to "new"
 * if non-NULL, otherwise let cpu_switch choose the next lwp.
 *
 * Returns 1 if another process was actually run.
 */
int
mi_switch(struct lwp *l, struct lwp *newl)
{
	struct schedstate_percpu *spc;
	struct timeval tv;
	int hold_count;
	int retval, oldspl;
	long s, u;
#if PERFCTRS
	struct proc *p = l->l_proc;
#endif

	LOCK_ASSERT(lwp_locked(l, NULL));

	/*
	 * Release the kernel_lock, as we are about to yield the CPU.
	 */
	hold_count = KERNEL_LOCK_RELEASE_ALL();

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
	KDASSERT(l->l_cpu != NULL);
	KDASSERT(l->l_cpu == curcpu());
	spc = &l->l_cpu->ci_schedstate;

	/*
	 * Compute the amount of time during which the current
	 * process was running.
	 */
	microtime(&tv);
	u = l->l_rtime.tv_usec +
	    (tv.tv_usec - spc->spc_runtime.tv_usec);
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

	/*
	 * XXXSMP If we are using h/w performance counters, save context.
	 */
#if PERFCTRS
	if (PMC_ENABLED(p)) {
		pmc_save_context(p);
	}
#endif

	/*
	 * Acquire the sched_mutex if necessary.  It will be released by
	 * cpu_switch once it has decided to idle, or picked another LWP
	 * to run.
	 */
	oldspl = mutex_getspl(l->l_mutex);
#ifdef MULTIPROCESSOR
	if (l->l_mutex != &sched_mutex || l->l_omutex != NULL) {
		lwp_unlock(l);
		mutex_enter(&sched_mutex);
	}
#endif

	/*
	 * If on the CPU and we have gotten this far, then we must yield.
	 */
	KASSERT(l->l_stat != LSRUN);
	if (l->l_stat == LSONPROC) {
		l->l_stat = LSRUN;
		setrunqueue(l);
	}
	uvmexp.swtch++;

	/*
	 * Process is about to yield the CPU; clear the appropriate
	 * scheduling flags.
	 */
	spc->spc_flags &= ~SPCF_SWITCHCLEAR;

	LOCKDEBUG_BARRIER(&sched_mutex, 1);

	/*
	 * Switch to the new current LWP.  When we run again, we'll
	 * return back here.
	 */
	if (newl == NULL)
		retval = cpu_switch(l, NULL);
	else {
		/* XXXAD ? */
		remrunqueue(newl);
		cpu_switchto(l, newl);
		retval = 0;
	}

	/*
	 * XXXSMP If we are using h/w performance counters, restore context.
	 */
#if PERFCTRS
	if (PMC_ENABLED(p)) {
		pmc_restore_context(p);
	}
#endif

	/*
	 * We're running again; record our new start time.  We might
	 * be running on a new CPU now, so don't use the cached
	 * schedstate_percpu pointer.
	 */
	KDASSERT(l->l_cpu != NULL);
	KDASSERT(l->l_cpu == curcpu());
	microtime(&l->l_cpu->ci_schedstate.spc_runtime);

	/*
	 * Reacquire the kernel_lock, and restore the old SPL.
	 * XXX We should do spl0() before acquiring the kernel
	 * lock but splx() may not raise the IPL.
	 */
	splx(oldspl);
	KERNEL_LOCK_ACQUIRE_COUNT(hold_count);

	return retval;
}

/*
 * Initialize the (doubly-linked) run queues
 * to be empty.
 */
void
rqinit()
{
	int i;

	for (i = 0; i < RUNQUE_NQS; i++)
		sched_qs[i].ph_link = sched_qs[i].ph_rlink =
		    (struct lwp *)&sched_qs[i];

	mutex_init(&sched_mutex, MUTEX_SPIN, IPL_SCHED);
}

static inline void
resched_lwp(struct lwp *l, u_char pri)
{
	struct cpu_info *ci;

	LOCK_ASSERT(lwp_locked(l, NULL));

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
		cpu_need_resched(ci);
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

	LOCK_ASSERT(mutex_owned(&p->p_smutex));
	LOCK_ASSERT(lwp_locked(l, NULL));

	switch (l->l_stat) {
	case LSSTOP:
		/*
		 * If we're being traced (possibly because someone attached us
		 * while we were stopped), check for a signal from the debugger.
		 */
		if ((p->p_flag & P_TRACED) != 0 && p->p_xstat != 0) {
			sigaddset(&l->l_sigpend->sp_set, p->p_xstat);
			signotify(l);
		}
		p->p_nrlwps++;
		break;
	case LSSUSPENDED:
		p->p_nrlwps++;
		break;
	case LSSLEEP:
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
		if ((l->l_flag & L_SINTR) != 0)
			sleepq_unsleep(l);
		return;
	}

	if (l->l_proc->p_sa)
		sa_awaken(l);

	/*
	 * Set in sched_mutex as it the LWP's current mutex.
	 */
	lwp_relock(l, &sched_mutex);

	/*
	 * If the LWP is still on the CPU, mark it as LSONPROC.  It may be
	 * about to call mi_switch(), in which case it will yield.
	 */
	if ((ci = l->l_cpu) != NULL && ci->ci_curlwp == l) {
		l->l_stat = LSONPROC;
		l->l_slptime = 0;
		lwp_unlock(l);
		return;
	}

	/*
	 * Set the LWP runnable.  If it's swapped out, we need to wake the swapper
	 * to bring it back in.  Otherwise, enter it into a run queue.
	 */
	l->l_stat = LSRUN;
	if (l->l_slptime > 1)
		updatepri(l);
	l->l_slptime = 0;

	if (l->l_flag & L_INMEM) {
		setrunqueue(l);
		resched_lwp(l, l->l_priority);
		lwp_unlock(l);
	} else {
		lwp_unlock(l);
		wakeup(&proc0);
	}
}

/*
 * Compute the priority of a process when running in user mode.
 * Arrange to reschedule if the resulting priority is better
 * than that of the current process.
 */
void
resetpriority(struct lwp *l)
{
	unsigned int newpriority;
	struct proc *p = l->l_proc;

	LOCK_ASSERT(lwp_locked(l, NULL));

	/* XXXSMP proc values will be accessed unlocked */
	newpriority = PUSER + (p->p_estcpu >> ESTCPU_SHIFT) +
			NICE_WEIGHT * (p->p_nice - NZERO);
	newpriority = min(newpriority, MAXPRI);
	l->l_usrpri = newpriority;
	resched_lwp(l, l->l_usrpri);
}

/*
 * Recompute priority for all LWPs in a process.
 */
void
resetprocpriority(struct proc *p)
{
	struct lwp *l;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	LIST_FOREACH(l, &p->p_lwps, l_sibling) {
		lwp_lock(l);
		resetpriority(l);
		lwp_unlock(l);
	}
}

/*
 * We adjust the priority of the current process.  The priority of a process
 * gets worse as it accumulates CPU time.  The CPU usage estimator (p_estcpu)
 * is increased here.  The formula for computing priorities (in kern_synch.c)
 * will compute a different value each time p_estcpu increases. This can
 * cause a switch, but unless the priority crosses a PPQ boundary the actual
 * queue will not change.  The CPU usage estimator ramps up quite quickly
 * when the process is running (linearly), and decays away exponentially, at
 * a rate which is proportionally slower when the system is busy.  The basic
 * principle is that the system will 90% forget that the process used a lot
 * of CPU time in 5 * loadav seconds.  This causes the system to favor
 * processes which haven't run much recently, and to round-robin among other
 * processes.
 */

void
schedclock(struct lwp *l)
{
	struct proc *p = l->l_proc;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	p->p_estcpu = ESTCPULIM(p->p_estcpu + (1 << ESTCPU_SHIFT));

	lwp_lock(l);
	resetpriority(l);
	if (l->l_priority >= PUSER)
		l->l_priority = l->l_usrpri;
	lwp_unlock(l);
}

/*
 * suspendsched:
 *
 *	Convert all non-L_SYSTEM LSSLEEP or LSRUN LWPs to LSSUSPENDED.
 *	This violates locking conventions (p->p_smutex is not held),
 *	however since it is only used during panic or shutdown that is
 *	not a problem.
 *
 *	XXXAD Do by process?
 */
void
suspendsched()
{
	struct lwp *l;
	struct proc *p;

	mutex_enter(&alllwp_mutex);
	LIST_FOREACH(l, &alllwp, l_list) {
		if ((l->l_flag & L_SYSTEM) != 0)
			continue;

		p = l->l_proc;

		lwp_lock(l);
		switch (l->l_stat) {
		case LSRUN:
			p->p_nrlwps--;
			l->l_stat = LSSUSPENDED;
			remrunqueue(l);
			break;
		case LSONPROC:
			p->p_nrlwps--;
			l->l_stat = LSSUSPENDED;
			break;
		case LSSLEEP:
			p->p_nrlwps--;
			l->l_stat = LSSUSPENDED;
			break;
		default:
			break;
		}
		lwp_setlock_unlock(l, &lwp_mutex);
	}
	mutex_exit(&alllwp_mutex);
}

/*
 * scheduler_fork_hook:
 *
 *	Inherit the parent's scheduler history.
 */
void
scheduler_fork_hook(struct proc *parent, struct proc *child)
{

	mutex_enter(&parent->p_smutex);
	child->p_estcpu = child->p_estcpu_inherited = parent->p_estcpu;
	child->p_forktime = schedcpu_ticks;
	mutex_exit(&parent->p_smutex);
}

/*
 * scheduler_wait_hook:
 *
 *	Chargeback parents for the sins of their children.
 */
void
scheduler_wait_hook(struct proc *parent, struct proc *child)
{
	fixpt_t loadfac = loadfactor(averunnable.ldavg[0]);
	fixpt_t estcpu;

	/* XXX Only if parent != init?? */

	mutex_enter(&parent->p_smutex);
	estcpu = decay_cpu_batch(loadfac, child->p_estcpu_inherited,
	    schedcpu_ticks - child->p_forktime);
	if (child->p_estcpu > estcpu)
		parent->p_estcpu =
		    ESTCPULIM(parent->p_estcpu + child->p_estcpu - estcpu);
	mutex_exit(&parent->p_smutex);
}

/*
 * XXXAD Scale an LWP priority (possibly a user priority) to a kernel one. 
 * This is a hack; think of something better.  Do unlocked for now.
 */
int
sched_kpri(struct lwp *l)
{
	const int obase = PUSER;
	const int ospan = MAXPRI - obase;
	const int nbase = PRIBIO;
	const int nspan = PUSER - nbase;

	if (l->l_priority < obase)
		return (l->l_priority);

	return (l->l_priority - obase) * nspan / ospan + nbase;
}

void
sched_lock(void)
{

	mutex_enter(&sched_mutex);
}

void
sched_unlock(void)
{

	mutex_exit(&sched_mutex);
}

/*
 * Low-level routines to access the run queue.  Optimised assembler
 * routines can override these.
 */

#ifndef __HAVE_MD_RUNQUEUE

/*
 * On some architectures, it's faster to use a MSB ordering for the priorites
 * than the traditional LSB ordering.
 */
#ifdef __HAVE_BIGENDIAN_BITOPS
#define	RQMASK(n) (0x80000000 >> (n))
#else
#define	RQMASK(n) (0x00000001 << (n))
#endif

/*
 * The primitives that manipulate the run queues.  whichqs tells which
 * of the 32 queues qs have processes in them.  Setrunqueue puts processes
 * into queues, remrunqueue removes them from queues.  The running process is
 * on no queue, other processes are on a queue related to p->p_priority,
 * divided by 4 actually to shrink the 0-127 range of priorities into the 32
 * available queues.
 */
#ifdef RQDEBUG
static void
checkrunqueue(int whichq, struct lwp *l)
{
	const struct prochd * const rq = &sched_qs[whichq];
	struct lwp *l2;
	int found = 0;
	int die = 0;
	int empty = 1;
	for (l2 = rq->ph_link; l2 != (const void*) rq; l2 = l2->l_forw) {
		if (l2->l_stat != LSRUN) {
			printf("checkrunqueue[%d]: lwp %p state (%d) "
			    " != LSRUN\n", whichq, l2, l2->l_stat);
		}
		if (l2->l_back->l_forw != l2) {
			printf("checkrunqueue[%d]: lwp %p back-qptr (%p) "
			    "corrupt %p\n", whichq, l2, l2->l_back,
			    l2->l_back->l_forw);
			die = 1;
		}
		if (l2->l_forw->l_back != l2) {
			printf("checkrunqueue[%d]: lwp %p forw-qptr (%p) "
			    "corrupt %p\n", whichq, l2, l2->l_forw,
			    l2->l_forw->l_back);
			die = 1;
		}
		if (l2 == l)
			found = 1;
		empty = 0;
	}
	if (empty && (sched_whichqs & RQMASK(whichq)) != 0) {
		printf("checkrunqueue[%d]: bit set for empty run-queue %p\n",
		    whichq, rq);
		die = 1;
	} else if (!empty && (sched_whichqs & RQMASK(whichq)) == 0) {
		printf("checkrunqueue[%d]: bit clear for non-empty "
		    "run-queue %p\n", whichq, rq);
		die = 1;
	}
	if (l != NULL && (sched_whichqs & RQMASK(whichq)) == 0) {
		printf("checkrunqueue[%d]: bit clear for active lwp %p\n",
		    whichq, l);
		die = 1;
	}
	if (l != NULL && empty) {
		printf("checkrunqueue[%d]: empty run-queue %p with "
		    "active lwp %p\n", whichq, rq, l);
		die = 1;
	}
	if (l != NULL && !found) {
		printf("checkrunqueue[%d]: lwp %p not in runqueue %p!",
		    whichq, l, rq);
		die = 1;
	}
	if (die)
		panic("checkrunqueue: inconsistency found");
}
#endif /* RQDEBUG */

void
setrunqueue(struct lwp *l)
{
	struct prochd *rq;
	struct lwp *prev;
	const int whichq = l->l_priority / PPQ;

	LOCK_ASSERT(lwp_locked(l, &sched_mutex));

#ifdef RQDEBUG
	checkrunqueue(whichq, NULL);
#endif
#ifdef DIAGNOSTIC
	if (l->l_back != NULL || l->l_stat != LSRUN)
		panic("setrunqueue");
#endif
	sched_whichqs |= RQMASK(whichq);
	rq = &sched_qs[whichq];
	prev = rq->ph_rlink;
	l->l_forw = (struct lwp *)rq;
	rq->ph_rlink = l;
	prev->l_forw = l;
	l->l_back = prev;
#ifdef RQDEBUG
	checkrunqueue(whichq, l);
#endif
}

void
remrunqueue(struct lwp *l)
{
	struct lwp *prev, *next;
	const int whichq = l->l_priority / PPQ;

	LOCK_ASSERT(lwp_locked(l, &sched_mutex));

#ifdef RQDEBUG
	checkrunqueue(whichq, l);
#endif

#if defined(DIAGNOSTIC)
	if (((sched_whichqs & RQMASK(whichq)) == 0) || l->l_back == NULL) {
		/* Shouldn't happen - interrupts disabled. */
		panic("remrunqueue: bit %d not set", whichq);
	}
#endif
	prev = l->l_back;
	l->l_back = NULL;
	next = l->l_forw;
	prev->l_forw = next;
	next->l_back = prev;
	if (prev == next)
		sched_whichqs &= ~RQMASK(whichq);
#ifdef RQDEBUG
	checkrunqueue(whichq, NULL);
#endif
}

#undef RQMASK
#endif /* !defined(__HAVE_MD_RUNQUEUE) */
