/*	$NetBSD: kern_synch.c,v 1.101.2.4 2001/08/24 04:20:08 nathanw Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#include "opt_ddb.h"
#include "opt_ktrace.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/sched.h>
#include <sys/sa.h>
#include <sys/savar.h>

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
struct prochd sched_qs[RUNQUE_NQS];	/* run queues */
__volatile u_int32_t sched_whichqs;	/* bitmap of non-empty queues */
struct slpque sched_slpque[SLPQUE_TABLESIZE]; /* sleep queues */

struct simplelock sched_lock = SIMPLELOCK_INITIALIZER;
#if defined(MULTIPROCESSOR)
struct lock kernel_lock;
#endif

void schedcpu(void *);
void updatepri(struct lwp *);
void endtsleep(void *);

__inline void awaken(struct lwp *);

struct callout schedcpu_ch = CALLOUT_INITIALIZER;



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
	
	if (curproc != NULL) {
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
	need_resched(curcpu());
}

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
#define	decay_cpu(loadfac, cpu)	(((loadfac) * (cpu)) / ((loadfac) + FSCALE))

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
	struct lwp *l;
	struct proc *p;
	int s, s1;
	unsigned int newcpu;
	int clkhz;

	proclist_lock_read();
	for (l = LIST_FIRST(&alllwp); l != NULL; l = LIST_NEXT(l,l_list)) {
		/*
		 * Increment time in/out of memory and sleep time
		 * (if sleeping).  We ignore overflow; with 16-bit int's
		 * (remember them?) overflow takes 45 days.
		 */
		p = l->l_proc;
		l->l_swtime++;
		if (l->l_stat == LSSLEEP || l->l_stat == LSSTOP || 
		    l->l_stat == LSSUSPENDED)
			l->l_slptime++;
		p->p_pctcpu = (p->p_pctcpu * ccpu) >> FSHIFT;
		/*
		 * If the process has slept the entire second,
		 * stop recalculating its priority until it wakes up.
		 */
		if (l->l_slptime > 1)
			continue;
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
		l->l_pctcpu += ((FSCALE - ccpu) *
			(p->p_cpticks * FSCALE / clkhz)) >> FSHIFT;
#endif
		p->p_cpticks = 0;
		newcpu = (u_int)decay_cpu(loadfac, p->p_estcpu);
		p->p_estcpu = newcpu;
		SCHED_LOCK(s1);
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
		SCHED_UNLOCK(s1);
		splx(s);
	}
	proclist_unlock_read();
	uvm_meter();
	wakeup((caddr_t)&lbolt);
	callout_reset(&schedcpu_ch, hz, schedcpu, NULL);
}

/*
 * Recalculate the priority of a process after it has slept for a while.
 * For all load averages >= 1 and max p_estcpu of 255, sleeping for at
 * least six times the loadfactor will decay p_estcpu to zero.
 */
void
updatepri(struct lwp *l)
{
	struct proc *p = l->l_proc;
	unsigned int newcpu;
	fixpt_t loadfac;

	SCHED_ASSERT_LOCKED();

	newcpu = p->p_estcpu;
	loadfac = loadfactor(averunnable.ldavg[0]);

	if (l->l_slptime > 5 * loadfac)
		p->p_estcpu = 0; /* XXX NJWLWP */
	else {
		l->l_slptime--;	/* the first time was done in schedcpu */
		while (newcpu && --l->l_slptime)
			newcpu = (int) decay_cpu(loadfac, newcpu);
		p->p_estcpu = newcpu;
	}
	resetpriority(l);
}

/*
 * During autoconfiguration or after a panic, a sleep will simply
 * lower the priority briefly to allow interrupts, then return.
 * The priority to be used (safepri) is machine-dependent, thus this
 * value is initialized and maintained in the machine-dependent layers.
 * This priority will typically be 0, or the lowest priority
 * that is safe for use on the interrupt stack; it can be made
 * higher to block network software interrupts after panics.
 */
int safepri;

/*
 * General sleep call.  Suspends the current process until a wakeup is
 * performed on the specified identifier.  The process will then be made
 * runnable with the specified priority.  Sleeps at most timo/hz seconds
 * (0 means no timeout).  If pri includes PCATCH flag, signals are checked
 * before and after sleeping, else signals are not checked.  Returns 0 if
 * awakened, EWOULDBLOCK if the timeout expires.  If PCATCH is set and a
 * signal needs to be delivered, ERESTART is returned if the current system
 * call should be restarted if possible, and EINTR is returned if the system
 * call should be interrupted by the signal (return EINTR).
 *
 * The interlock is held until the scheduler_slock is acquired.  The
 * interlock will be locked before returning back to the caller
 * unless the PNORELOCK flag is specified, in which case the
 * interlock will always be unlocked upon return.
 */
int
ltsleep(void *ident, int priority, const char *wmesg, int timo,
    __volatile struct simplelock *interlock)
{
	struct lwp *l = curproc;
	struct proc *p = l->l_proc;
	struct slpque *qp;
	int sig, s;
	int catch = priority & PCATCH;
	int relock = (priority & PNORELOCK) == 0;
	int exiterr = (priority & PNOEXITERR) == 0;

	/*
	 * XXXSMP
	 * This is probably bogus.  Figure out what the right
	 * thing to do here really is.
	 * Note that not sleeping if ltsleep is called with curproc == NULL 
	 * in the shutdown case is disgusting but partly necessary given
	 * how shutdown (barely) works.
	 */
	if (cold || (doing_shutdown && (panicstr || (l == NULL)))) {
		/*
		 * After a panic, or during autoconfiguration,
		 * just give interrupts a chance, then just return;
		 * don't run any other procs or panic below,
		 * in case this is the idle process and already asleep.
		 */
		s = splhigh();
		splx(safepri);
		splx(s);
		if (interlock != NULL && relock == 0)
			simple_unlock(interlock);
		return (0);
	}

	KASSERT(p != NULL);

#ifdef KTRACE
	if (KTRPOINT(p, KTR_CSW))
		ktrcsw(p, 1, 0);
#endif

	SCHED_LOCK(s);

#ifdef DIAGNOSTIC
	if (ident == NULL)
		panic("ltsleep: ident == NULL");
	if (l->l_stat != LSONPROC)
		panic("ltsleep: l_stat %d != LSONPROC", l->l_stat);
	if (l->l_back != NULL)
		panic("ltsleep: p_back != NULL");
#endif

	l->l_wchan = ident;
	l->l_wmesg = wmesg;
	l->l_slptime = 0;
	l->l_priority = priority & PRIMASK;

	qp = SLPQUE(ident);
	if (qp->sq_head == 0)
		qp->sq_head = l;
	else {
		*qp->sq_tailp = l;
	}
	*(qp->sq_tailp = &l->l_forw) = 0;

	if (timo)
		callout_reset(&l->l_tsleep_ch, timo, endtsleep, l);

	/*
	 * We can now release the interlock; the scheduler_slock
	 * is held, so a thread can't get in to do wakeup() before
	 * we do the switch.
	 *
	 * XXX We leave the code block here, after inserting ourselves
	 * on the sleep queue, because we might want a more clever
	 * data structure for the sleep queues at some point.
	 */
	if (interlock != NULL)
		simple_unlock(interlock);

	/*
	 * We put ourselves on the sleep queue and start our timeout
	 * before calling CURSIG, as we could stop there, and a wakeup
	 * or a SIGCONT (or both) could occur while we were stopped.
	 * A SIGCONT would cause us to be marked as SSLEEP
	 * without resuming us, thus we must be ready for sleep
	 * when CURSIG is called.  If the wakeup happens while we're
	 * stopped, p->p_wchan will be 0 upon return from CURSIG.
	 */
	if (catch) {
		l->l_flag |= L_SINTR;
		if ((sig = CURSIG(l)) != 0) {
			if (l->l_wchan != NULL)
				unsleep(l);
			l->l_stat = LSONPROC;
			SCHED_UNLOCK(s);
			goto resume;
		}
		if (l->l_wchan == NULL) {
			catch = 0;
			SCHED_UNLOCK(s);
			goto resume;
		}
	} else
		sig = 0;
	l->l_stat = LSSLEEP;
	p->p_nrlwps--;
	p->p_stats->p_ru.ru_nvcsw++;
	SCHED_ASSERT_LOCKED();
	if (l->l_flag & L_SA)
		sa_switch(l, SA_UPCALL_BLOCKED);
	else
		mi_switch(l, NULL);

#if	defined(DDB) && !defined(GPROF)
	/* handy breakpoint location after process "wakes" */
	asm(".globl bpendtsleep ; bpendtsleep:");
#endif

	SCHED_ASSERT_UNLOCKED();
	splx(s);

 resume:
	KDASSERT(l->l_cpu != NULL);
	KDASSERT(l->l_cpu == curcpu());
	l->l_cpu->ci_schedstate.spc_curpriority = l->l_usrpri;
	p->p_nrlwps++;

	l->l_flag &= ~L_SINTR;
	if (l->l_flag & L_TIMEOUT) {
		l->l_flag &= ~L_TIMEOUT;
		if (sig == 0) {
#ifdef KTRACE
			if (KTRPOINT(p, KTR_CSW))
				ktrcsw(p, 0, 0);
#endif
			if (relock && interlock != NULL)
				simple_lock(interlock);
			return (EWOULDBLOCK);
		}
	} else if (timo)
		callout_stop(&l->l_tsleep_ch);
	if (catch && (sig != 0 || (sig = CURSIG(l)) != 0)) {
#ifdef KTRACE
		if (KTRPOINT(p, KTR_CSW))
			ktrcsw(p, 0, 0);
#endif
		if (relock && interlock != NULL)
			simple_lock(interlock);
		if ((SIGACTION(p, sig).sa_flags & SA_RESTART) == 0)
			return (EINTR);
		return (ERESTART);
	}
	/* XXXNJW this is very much a kluge.
	 * revisit. a better way of preventing looping/hanging syscalls like 
	 * wait4() and _lwp_wait() from wedging an exiting process
	 * would be preferred.
	 */
	if (catch && ((p->p_flag & P_WEXIT) && exiterr))
		return (EINTR);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_CSW))
		ktrcsw(p, 0, 0);
#endif
	if (relock && interlock != NULL)
		simple_lock(interlock);
	return (0);
}

/*
 * Implement timeout for tsleep.
 * If process hasn't been awakened (wchan non-zero),
 * set timeout flag and undo the sleep.  If proc
 * is stopped, just unsleep so it will remain stopped.
 */
void
endtsleep(void *arg)
{
	struct lwp *l;
	int s;

	l = (struct lwp *)arg;
	SCHED_LOCK(s);
	if (l->l_wchan) {
		if (l->l_stat == LSSLEEP)
			setrunnable(l);
		else
			unsleep(l);
		l->l_flag |= L_TIMEOUT;
	}
	SCHED_UNLOCK(s);
}

/*
 * Remove a process from its wait queue
 */
void
unsleep(struct lwp *l)
{
	struct slpque *qp;
	struct lwp **hp;

	SCHED_ASSERT_LOCKED();

	if (l->l_wchan) {
		hp = &(qp = SLPQUE(l->l_wchan))->sq_head;
		while (*hp != l)
			hp = &(*hp)->l_forw;
		*hp = l->l_forw;
		if (qp->sq_tailp == &l->l_forw)
			qp->sq_tailp = hp;
		l->l_wchan = 0;
	}
}

/*
 * Optimized-for-wakeup() version of setrunnable().
 */
__inline void
awaken(struct lwp *l)
{

	SCHED_ASSERT_LOCKED();
	    
	if (l->l_slptime > 1)
		updatepri(l);
	l->l_slptime = 0;
	l->l_stat = LSRUN;

	/*
	 * Since curpriority is a user priority, p->p_priority
	 * is always better than curpriority.
	 */
	if (l->l_flag & L_INMEM) {
		setrunqueue(l);
		KASSERT(l->l_cpu != NULL);
		need_resched(l->l_cpu);
	} else
		sched_wakeup(&proc0);
}

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
void
sched_unlock_idle(void)
{

	simple_unlock(&sched_lock);
}

void
sched_lock_idle(void)
{

	simple_lock(&sched_lock);
}
#endif /* MULTIPROCESSOR || LOCKDEBUG */

/*
 * Make all processes sleeping on the specified identifier runnable.
 */

void
wakeup(void *ident)
{
	int s;

	SCHED_ASSERT_UNLOCKED();

	SCHED_LOCK(s);
	sched_wakeup(ident);
	SCHED_UNLOCK(s);
}

void
sched_wakeup(void *ident)
{
	struct slpque *qp;
	struct lwp *l, **q;

	SCHED_ASSERT_LOCKED();

	qp = SLPQUE(ident);
 restart:
	for (q = &qp->sq_head; (l = *q) != NULL; ) {
#ifdef DIAGNOSTIC
		if (l->l_back || (l->l_stat != LSSLEEP && 
		    l->l_stat != LSSTOP && l->l_stat != LSSUSPENDED))
			panic("wakeup");
#endif
		if (l->l_wchan == ident) {
			l->l_wchan = 0;
			*q = l->l_forw;
			if (qp->sq_tailp == &l->l_forw)
				qp->sq_tailp = q;
			if (l->l_stat == LSSLEEP) {
				awaken(l);
				goto restart;
			}
		} else
			q = &l->l_forw;
	}
}

/*
 * Make the highest priority process first in line on the specified
 * identifier runnable.
 */
void
wakeup_one(void *ident)
{
	struct slpque *qp;
	struct lwp *l, **q;
	struct lwp *best_sleepp, **best_sleepq;
	struct lwp *best_stopp, **best_stopq;
	int s;

	best_sleepp = best_stopp = NULL;
	best_sleepq = best_stopq = NULL;

	SCHED_LOCK(s);

	qp = SLPQUE(ident);

	for (q = &qp->sq_head; (l = *q) != NULL; q = &l->l_forw) {
#ifdef DIAGNOSTIC
		if (l->l_back || (l->l_stat != LSSLEEP && 
		    l->l_stat != LSSTOP && l->l_stat != LSSUSPENDED))
			panic("wakeup_one");
#endif
		if (l->l_wchan == ident) {
			if (l->l_stat == LSSLEEP) {
				if (best_sleepp == NULL ||
				    l->l_priority < best_sleepp->l_priority) {
					best_sleepp = l;
					best_sleepq = q;
				}
			} else {
				if (best_stopp == NULL ||
				    l->l_priority < best_stopp->l_priority) {
				    	best_stopp = l;
					best_stopq = q;
				}
			}
		}
	}

	/*
	 * Consider any SSLEEP process higher than the highest priority SSTOP
	 * process.
	 */
	if (best_sleepp != NULL) {
		l = best_sleepp;
		q = best_sleepq;
	} else {
		l = best_stopp;
		q = best_stopq;
	}

	if (l != NULL) {
		l->l_wchan = NULL;
		*q = l->l_forw;
		if (qp->sq_tailp == &l->l_forw)
			qp->sq_tailp = q;
		if (l->l_stat == LSSLEEP)
			awaken(l);
	}
	SCHED_UNLOCK(s);
}

/*
 * General yield call.  Puts the current process back on its run queue and
 * performs a voluntary context switch.
 */
void
yield(void)
{
	struct lwp *l = curproc;
	int s;

	SCHED_LOCK(s);
	l->l_priority = l->l_usrpri;
	l->l_stat = LSRUN;
	setrunqueue(l);
	l->l_proc->p_stats->p_ru.ru_nvcsw++;
	mi_switch(l, NULL);
	SCHED_ASSERT_UNLOCKED();
	splx(s);
}

/*
 * General preemption call.  Puts the current process back on its run queue
 * and performs an involuntary context switch.  If a process is supplied,
 * we switch to that process.  Otherwise, we use the normal process selection
 * criteria.
 */

void
preempt(struct lwp *newl)
{
	struct lwp *l = curproc;
	int r, s;

	SCHED_LOCK(s);
	l->l_priority = l->l_usrpri;
	l->l_stat = LSRUN;
	setrunqueue(l);
	l->l_proc->p_stats->p_ru.ru_nivcsw++;
	r = mi_switch(l, newl);
	if (r && (l->l_flag & L_SA))
		sa_upcall(l, SA_UPCALL_PREEMPTED, l, NULL, 0, 0, NULL);
	SCHED_ASSERT_UNLOCKED();
	splx(s);
}

/*
 * The machine independent parts of context switch.
 * Must be called at splsched() (no higher!) and with
 * the sched_lock held.
 * Switch to "new" if non-NULL, otherwise let cpu_switch choose
 * the next lwp.
 * 
 * Returns 1 if another process was actually run.
 */
int
mi_switch(struct lwp *l, struct lwp *new)
{
	struct schedstate_percpu *spc;
	struct rlimit *rlim;
	long s, u;
	struct timeval tv;
#if defined(MULTIPROCESSOR)
	int hold_count;
#endif
	struct proc *p = l->l_proc;
	int retval;

	SCHED_ASSERT_LOCKED();

#if defined(MULTIPROCESSOR)
	/*
	 * Release the kernel_lock, as we are about to yield the CPU.
	 * The scheduler lock is still held until cpu_switch()
	 * selects a new process and removes it from the run queue.
	 */
	if (p->p_flag & P_BIGLOCK)
		hold_count = spinlock_release_all(&kernel_lock);
#endif

	KDASSERT(l->l_cpu != NULL);
	KDASSERT(l->l_cpu == curcpu());

	spc = &l->l_cpu->ci_schedstate;

#if defined(LOCKDEBUG) || defined(DIAGNOSTIC)
	spinlock_switchcheck();
#endif
#ifdef LOCKDEBUG
	simple_lock_switchcheck();
#endif

	/*
	 * Compute the amount of time during which the current
	 * process was running, and add that to its total so far.
	 */
	microtime(&tv);
	u = p->p_rtime.tv_usec + 
	    (tv.tv_usec - spc->spc_runtime.tv_usec);
	s = p->p_rtime.tv_sec + (tv.tv_sec - spc->spc_runtime.tv_sec);
	if (u < 0) {
		u += 1000000;
		s--;
	} else if (u >= 1000000) {
		u -= 1000000;
		s++;
	}
	p->p_rtime.tv_usec = u;
	p->p_rtime.tv_sec = s;

	/*
	 * Check if the process exceeds its cpu resource allocation.
	 * If over max, kill it.  In any case, if it has run for more
	 * than 10 minutes, reduce priority to give others a chance.
	 */
	rlim = &p->p_rlimit[RLIMIT_CPU];
	if (s >= rlim->rlim_cur) {
		/*
		 * XXXSMP: we're inside the scheduler lock perimeter;
		 * use sched_psignal.
		 */
		if (s >= rlim->rlim_max)
			sched_psignal(p, SIGKILL);
		else {
			sched_psignal(p, SIGXCPU);
			if (rlim->rlim_cur < rlim->rlim_max)
				rlim->rlim_cur += 5;
		}
	}
	if (autonicetime && s > autonicetime && p->p_ucred->cr_uid &&
	    p->p_nice == NZERO) {
		p->p_nice = autoniceval + NZERO;
		resetpriority(l);
	}

	/*
	 * Process is about to yield the CPU; clear the appropriate
	 * scheduling flags.
	 */
	spc->spc_flags &= ~SPCF_SWITCHCLEAR;

	/*
	 * Pick a new current process and switch to it.  When we
	 * run again, we'll return back here.
	 */
	uvmexp.swtch++;
	if (new == NULL) {
		retval = cpu_switch(l);
	} else {
		cpu_preempt(l, new);
		retval = 0;
	}

	/*
	 * Make sure that MD code released the scheduler lock before
	 * resuming us.
	 */
	SCHED_ASSERT_UNLOCKED();

	/*
	 * We're running again; record our new start time.  We might
	 * be running on a new CPU now, so don't use the cache'd
	 * schedstate_percpu pointer.
	 */
	KDASSERT(l->l_cpu != NULL);
	KDASSERT(l->l_cpu == curcpu());
	microtime(&l->l_cpu->ci_schedstate.spc_runtime);

#if defined(MULTIPROCESSOR)
	/*
	 * Reacquire the kernel_lock now.  We do this after we've
	 * released the scheduler lock to avoid deadlock, and before
	 * we reacquire the interlock.
	 */
	if (p->p_flag & P_BIGLOCK)
		spinlock_acquire_count(&kernel_lock, hold_count);
#endif

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
}

/*
 * Change process state to be runnable,
 * placing it on the run queue if it is in memory,
 * and awakening the swapper if it isn't in memory.
 */
void
setrunnable(struct lwp *l)
{
	struct proc *p = l->l_proc;

	SCHED_ASSERT_LOCKED();

	switch (l->l_stat) {
	case 0:
	case LSRUN:
	case LSONPROC:
	case LSZOMB:
	case LSDEAD:
	default:
		panic("setrunnable");
	case LSSTOP:
		/*
		 * If we're being traced (possibly because someone attached us
		 * while we were stopped), check for a signal from the debugger.
		 */
		if ((p->p_flag & P_TRACED) != 0 && p->p_xstat != 0) {
			sigaddset(&p->p_sigctx.ps_siglist, p->p_xstat);
			CHECKSIGS(p);
		}
	case LSSLEEP:
		unsleep(l);		/* e.g. when sending signals */
		break;

	case LSIDL:
		break;
	case LSSUSPENDED:
		break;
	}
	l->l_stat = LSRUN;
	if (l->l_flag & L_INMEM)
		setrunqueue(l);

	if (l->l_slptime > 1)
		updatepri(l);
	l->l_slptime = 0;
	if ((l->l_flag & L_INMEM) == 0)
		wakeup((caddr_t)&proc0);
	else if (l->l_priority < curcpu()->ci_schedstate.spc_curpriority) {
		/*
		 * XXXSMP
		 * This is not exactly right.  Since p->p_cpu persists
		 * across a context switch, this gives us some sort
		 * of processor affinity.  But we need to figure out
		 * at what point it's better to reschedule on a different
		 * CPU than the last one.
		 */
		need_resched((p->p_cpu != NULL) ? p->p_cpu : curcpu());
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

	SCHED_ASSERT_LOCKED();

	newpriority = PUSER + p->p_estcpu + 
			NICE_WEIGHT * (p->p_nice - NZERO);
	newpriority = min(newpriority, MAXPRI);
	l->l_usrpri = newpriority;
	if (newpriority < curcpu()->ci_schedstate.spc_curpriority) {
		/*
		 * XXXSMP
		 * Same applies as in setrunnable() above.
		 */
		need_resched((p->p_cpu != NULL) ? p->p_cpu : curcpu());
	}
}

/* 
 * Recompute priority for all LWPs in a process.
 */
void
resetprocpriority(struct proc *p)
{
	struct lwp *l;

	LIST_FOREACH(l, &p->p_lwps, l_list)
	    resetpriority(l);
}

/*
 * We adjust the priority of the current process.  The priority of a process
 * gets worse as it accumulates CPU time.  The cpu usage estimator (p_estcpu)
 * is increased here.  The formula for computing priorities (in kern_synch.c)
 * will compute a different value each time p_estcpu increases. This can
 * cause a switch, but unless the priority crosses a PPQ boundary the actual
 * queue will not change.  The cpu usage estimator ramps up quite quickly
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
	int s;

	p->p_estcpu = ESTCPULIM(p->p_estcpu + 1);
	SCHED_LOCK(s);
	resetpriority(l);
	SCHED_UNLOCK(s);
	    
	if (l->l_priority >= PUSER)
		l->l_priority = l->l_usrpri;
}

void
suspendsched()
{
	struct lwp *l;
	int s;

	/*
	 * Convert all non-P_SYSTEM LSSLEEP or LSRUN processes to 
	 * LSSUSPENDED.
	 */
	proclist_lock_read();
	SCHED_LOCK(s);
	for (l = LIST_FIRST(&alllwp); l != NULL; l = LIST_NEXT(l, l_list)) {
		if ((l->l_proc->p_flag & P_SYSTEM) != 0)
			continue;

		switch (l->l_stat) {
		case LSRUN:
			if ((l->l_flag & L_INMEM) != 0)
				remrunqueue(l);
			/* FALLTHROUGH */
		case LSSLEEP:
			l->l_stat = LSSUSPENDED;
			break;
		case LSONPROC:
			/*
			 * XXX SMP: we need to deal with processes on
			 * others CPU !
			 */
			break;
		default:
			break;
		}
	}
	SCHED_UNLOCK(s);
	proclist_unlock_read();
}


