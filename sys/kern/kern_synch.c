/*	$NetBSD: kern_synch.c,v 1.160.6.1 2006/06/01 22:38:08 kardel Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
__KERNEL_RCSID(0, "$NetBSD: kern_synch.c,v 1.160.6.1 2006/06/01 22:38:08 kardel Exp $");

#include "opt_ddb.h"
#include "opt_ktrace.h"
#include "opt_kstack.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_perfctrs.h"

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

#include <uvm/uvm_extern.h>

#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <machine/cpu.h>

int	lbolt;			/* once a second sleep address */
int	rrticks;		/* number of hardclock ticks per roundrobin() */

/*
 * Sleep queues.
 *
 * We're only looking at 7 bits of the address; everything is
 * aligned to 4, lots of things are aligned to greater powers
 * of 2.  Shift right by 8, i.e. drop the bottom 256 worth.
 */
#define	SLPQUE_TABLESIZE	128
#define	SLPQUE_LOOKUP(x)	(((u_long)(x) >> 8) & (SLPQUE_TABLESIZE - 1))

#define	SLPQUE(ident)	(&sched_slpque[SLPQUE_LOOKUP(ident)])

/*
 * The global scheduler state.
 */
struct prochd sched_qs[RUNQUE_NQS];	/* run queues */
volatile uint32_t sched_whichqs;	/* bitmap of non-empty queues */
struct slpque sched_slpque[SLPQUE_TABLESIZE]; /* sleep queues */

struct simplelock sched_lock = SIMPLELOCK_INITIALIZER;

void schedcpu(void *);
void updatepri(struct lwp *);
void endtsleep(void *);

inline void sa_awaken(struct lwp *);
inline void awaken(struct lwp *);

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
	need_resched(curcpu());
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
	struct lwp *l;
	struct proc *p;
	int s, minslp;
	int clkhz;

	schedcpu_ticks++;

	proclist_lock_read();
	PROCLIST_FOREACH(p, &allproc) {
		/*
		 * Increment time in/out of memory and sleep time
		 * (if sleeping).  We ignore overflow; with 16-bit int's
		 * (remember them?) overflow takes 45 days.
		 */
		minslp = 2;
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			l->l_swtime++;
			if (l->l_stat == LSSLEEP || l->l_stat == LSSTOP ||
			    l->l_stat == LSSUSPENDED) {
				l->l_slptime++;
				minslp = min(minslp, l->l_slptime);
			} else
				minslp = 0;
		}
		p->p_pctcpu = (p->p_pctcpu * ccpu) >> FSHIFT;
		/*
		 * If the process has slept the entire second,
		 * stop recalculating its priority until it wakes up.
		 */
		if (minslp > 1)
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
		p->p_pctcpu += ((FSCALE - ccpu) *
			(p->p_cpticks * FSCALE / clkhz)) >> FSHIFT;
#endif
		p->p_cpticks = 0;
		p->p_estcpu = decay_cpu(loadfac, p->p_estcpu);
		splx(s);	/* Done with the process CPU ticks update */
		SCHED_LOCK(s);
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			if (l->l_slptime > 1)
				continue;
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
		}
		SCHED_UNLOCK(s);
	}
	proclist_unlock_read();
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

	SCHED_ASSERT_LOCKED();
	KASSERT(l->l_slptime > 1);

	loadfac = loadfactor(averunnable.ldavg[0]);

	l->l_slptime--; /* the first time was done in schedcpu */
	/* XXX NJWLWP */
	p->p_estcpu = decay_cpu_batch(loadfac, p->p_estcpu, l->l_slptime);
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
ltsleep(volatile const void *ident, int priority, const char *wmesg, int timo,
    volatile struct simplelock *interlock)
{
	struct lwp *l = curlwp;
	struct proc *p = l ? l->l_proc : NULL;
	struct slpque *qp;
	struct sadata_upcall *sau;
	int sig, s;
	int catch = priority & PCATCH;
	int relock = (priority & PNORELOCK) == 0;
	int exiterr = (priority & PNOEXITERR) == 0;

	/*
	 * XXXSMP
	 * This is probably bogus.  Figure out what the right
	 * thing to do here really is.
	 * Note that not sleeping if ltsleep is called with curlwp == NULL
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
	LOCK_ASSERT(interlock == NULL || simple_lock_held(interlock));

#ifdef KTRACE
	if (KTRPOINT(p, KTR_CSW))
		ktrcsw(l, 1, 0);
#endif

	/*
	 * XXX We need to allocate the sadata_upcall structure here,
	 * XXX since we can't sleep while waiting for memory inside
	 * XXX sa_upcall().  It would be nice if we could safely
	 * XXX allocate the sadata_upcall structure on the stack, here.
	 */
	if (l->l_flag & L_SA) {
		sau = sadata_upcall_alloc(0);
	} else {
		sau = NULL;
	}

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
		if (((sig = CURSIG(l)) != 0) ||
		    ((p->p_flag & P_WEXIT) && p->p_nlwps > 1)) {
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
		sa_switch(l, sau, SA_UPCALL_BLOCKED);
	else
		mi_switch(l, NULL);

#if	defined(DDB) && !defined(GPROF)
	/* handy breakpoint location after process "wakes" */
	__asm(".globl bpendtsleep\nbpendtsleep:");
#endif
	/*
	 * p->p_nrlwps is incremented by whoever made us runnable again,
	 * either setrunnable() or awaken().
	 */

	SCHED_ASSERT_UNLOCKED();
	splx(s);

 resume:
	KDASSERT(l->l_cpu != NULL);
	KDASSERT(l->l_cpu == curcpu());
	l->l_cpu->ci_schedstate.spc_curpriority = l->l_usrpri;

	l->l_flag &= ~L_SINTR;
	if (l->l_flag & L_TIMEOUT) {
		l->l_flag &= ~(L_TIMEOUT|L_CANCELLED);
		if (sig == 0) {
#ifdef KTRACE
			if (KTRPOINT(p, KTR_CSW))
				ktrcsw(l, 0, 0);
#endif
			if (relock && interlock != NULL)
				simple_lock(interlock);
			return (EWOULDBLOCK);
		}
	} else if (timo)
		callout_stop(&l->l_tsleep_ch);

	if (catch) {
		const int cancelled = l->l_flag & L_CANCELLED;
		l->l_flag &= ~L_CANCELLED;
		if (sig != 0 || (sig = CURSIG(l)) != 0 || cancelled) {
#ifdef KTRACE
			if (KTRPOINT(p, KTR_CSW))
				ktrcsw(l, 0, 0);
#endif
			if (relock && interlock != NULL)
				simple_lock(interlock);
			/*
			 * If this sleep was canceled, don't let the syscall
			 * restart.
			 */
			if (cancelled ||
			    (SIGACTION(p, sig).sa_flags & SA_RESTART) == 0)
				return (EINTR);
			return (ERESTART);
		}
	}

#ifdef KTRACE
	if (KTRPOINT(p, KTR_CSW))
		ktrcsw(l, 0, 0);
#endif
	if (relock && interlock != NULL)
		simple_lock(interlock);

	/* XXXNJW this is very much a kluge.
	 * revisit. a better way of preventing looping/hanging syscalls like
	 * wait4() and _lwp_wait() from wedging an exiting process
	 * would be preferred.
	 */
	if (catch && ((p->p_flag & P_WEXIT) && p->p_nlwps > 1 && exiterr))
		return (EINTR);
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

inline void
sa_awaken(struct lwp *l)
{

	SCHED_ASSERT_LOCKED();

	if (l == l->l_savp->savp_lwp && l->l_flag & L_SA_YIELD)
		l->l_flag &= ~L_SA_IDLE;
}

/*
 * Optimized-for-wakeup() version of setrunnable().
 */
inline void
awaken(struct lwp *l)
{

	SCHED_ASSERT_LOCKED();

	if (l->l_proc->p_sa)
		sa_awaken(l);

	if (l->l_slptime > 1)
		updatepri(l);
	l->l_slptime = 0;
	l->l_stat = LSRUN;
	l->l_proc->p_nrlwps++;
	/*
	 * Since curpriority is a user priority, p->p_priority
	 * is always better than curpriority on the last CPU on
	 * which it ran.
	 *
	 * XXXSMP See affinity comment in resched_proc().
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
wakeup(volatile const void *ident)
{
	int s;

	SCHED_ASSERT_UNLOCKED();

	SCHED_LOCK(s);
	sched_wakeup(ident);
	SCHED_UNLOCK(s);
}

void
sched_wakeup(volatile const void *ident)
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
wakeup_one(volatile const void *ident)
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
 * performs a voluntary context switch.  Should only be called when the
 * current process explicitly requests it (eg sched_yield(2) in compat code).
 */
void
yield(void)
{
	struct lwp *l = curlwp;
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
	int r, s;

	SCHED_LOCK(s);
	l->l_priority = l->l_usrpri;
	l->l_stat = LSRUN;
	setrunqueue(l);
	l->l_proc->p_stats->p_ru.ru_nivcsw++;
	r = mi_switch(l, NULL);
	SCHED_ASSERT_UNLOCKED();
	splx(s);
	if ((l->l_flag & L_SA) != 0 && r != 0 && more == 0)
		sa_preempt(l);
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
mi_switch(struct lwp *l, struct lwp *newl)
{
	struct schedstate_percpu *spc;
	struct rlimit *rlim;
	long s, u;
	struct timeval tv;
	int hold_count;
	struct proc *p = l->l_proc;
	int retval;

	SCHED_ASSERT_LOCKED();

	/*
	 * Release the kernel_lock, as we are about to yield the CPU.
	 * The scheduler lock is still held until cpu_switch()
	 * selects a new process and removes it from the run queue.
	 */
	hold_count = KERNEL_LOCK_RELEASE_ALL();

	KDASSERT(l->l_cpu != NULL);
	KDASSERT(l->l_cpu == curcpu());

	spc = &l->l_cpu->ci_schedstate;

#ifdef LOCKDEBUG
	spinlock_switchcheck();
	simple_lock_switchcheck();
#endif

	/*
	 * Compute the amount of time during which the current
	 * process was running.
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
	 * Check if the process exceeds its CPU resource allocation.
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
	if (autonicetime && s > autonicetime &&
	    kauth_cred_geteuid(p->p_cred) && p->p_nice == NZERO) {
		p->p_nice = autoniceval + NZERO;
		resetpriority(l);
	}

	/*
	 * Process is about to yield the CPU; clear the appropriate
	 * scheduling flags.
	 */
	spc->spc_flags &= ~SPCF_SWITCHCLEAR;

#ifdef KSTACK_CHECK_MAGIC
	kstack_check_magic(l);
#endif

	/*
	 * If we are using h/w performance counters, save context.
	 */
#if PERFCTRS
	if (PMC_ENABLED(p))
		pmc_save_context(p);
#endif

	/*
	 * Switch to the new current process.  When we
	 * run again, we'll return back here.
	 */
	uvmexp.swtch++;
	if (newl == NULL) {
		retval = cpu_switch(l, NULL);
	} else {
		remrunqueue(newl);
		cpu_switchto(l, newl);
		retval = 0;
	}

	/*
	 * If we are using h/w performance counters, restore context.
	 */
#if PERFCTRS
	if (PMC_ENABLED(p))
		pmc_restore_context(p);
#endif

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

	/*
	 * Reacquire the kernel_lock now.  We do this after we've
	 * released the scheduler lock to avoid deadlock, and before
	 * we reacquire the interlock.
	 */
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
}

static inline void
resched_proc(struct lwp *l, u_char pri)
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
	 *
	 * XXXSMP
	 * There is also the issue of locking the other CPU's
	 * sched state, which we currently do not do.
	 */
	ci = (l->l_cpu != NULL) ? l->l_cpu : curcpu();
	if (pri < ci->ci_schedstate.spc_curpriority)
		need_resched(ci);
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
		panic("setrunnable: lwp %p state was %d", l, l->l_stat);
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

	if (l->l_proc->p_sa)
		sa_awaken(l);

	l->l_stat = LSRUN;
	p->p_nrlwps++;

	if (l->l_flag & L_INMEM)
		setrunqueue(l);

	if (l->l_slptime > 1)
		updatepri(l);
	l->l_slptime = 0;
	if ((l->l_flag & L_INMEM) == 0)
		sched_wakeup((caddr_t)&proc0);
	else
		resched_proc(l, l->l_priority);
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

	newpriority = PUSER + (p->p_estcpu >> ESTCPU_SHIFT) +
			NICE_WEIGHT * (p->p_nice - NZERO);
	newpriority = min(newpriority, MAXPRI);
	l->l_usrpri = newpriority;
	resched_proc(l, l->l_usrpri);
}

/*
 * Recompute priority for all LWPs in a process.
 */
void
resetprocpriority(struct proc *p)
{
	struct lwp *l;

	LIST_FOREACH(l, &p->p_lwps, l_sibling)
	    resetpriority(l);
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
	int s;

	p->p_estcpu = ESTCPULIM(p->p_estcpu + (1 << ESTCPU_SHIFT));
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
	LIST_FOREACH(l, &alllwp, l_list) {
		if ((l->l_proc->p_flag & P_SYSTEM) != 0)
			continue;

		switch (l->l_stat) {
		case LSRUN:
			l->l_proc->p_nrlwps--;
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

/*
 * scheduler_fork_hook:
 *
 *	Inherit the parent's scheduler history.
 */
void
scheduler_fork_hook(struct proc *parent, struct proc *child)
{

	child->p_estcpu = child->p_estcpu_inherited = parent->p_estcpu;
	child->p_forktime = schedcpu_ticks;
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

	estcpu = decay_cpu_batch(loadfac, child->p_estcpu_inherited,
	    schedcpu_ticks - child->p_forktime);
	if (child->p_estcpu > estcpu) {
		parent->p_estcpu =
		    ESTCPULIM(parent->p_estcpu + child->p_estcpu - estcpu);
	}
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
	for (l2 = rq->ph_link; l2 != (void*) rq; l2 = l2->l_forw) {
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

#ifdef RQDEBUG
	checkrunqueue(whichq, NULL);
#endif
#ifdef DIAGNOSTIC
	if (l->l_back != NULL || l->l_wchan != NULL || l->l_stat != LSRUN)
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
#ifdef RQDEBUG
	checkrunqueue(whichq, l);
#endif
#ifdef DIAGNOSTIC
	if (((sched_whichqs & RQMASK(whichq)) == 0))
		panic("remrunqueue: bit %d not set", whichq);
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
