/*	$NetBSD: sched_4bsd.c,v 1.1.6.4 2007/07/07 11:56:11 ad Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2004, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, by Charles M. Hannum, Andrew Doran, and
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
__KERNEL_RCSID(0, "$NetBSD: sched_4bsd.c,v 1.1.6.4 2007/07/07 11:56:11 ad Exp $");

#include "opt_ddb.h"
#include "opt_lockdebug.h"
#include "opt_perfctrs.h"

#define	__MUTEX_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <sys/kauth.h>
#include <sys/lockdebug.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

/*
 * Run queues.
 *
 * We maintain a bitmask of non-empty queues in order speed up finding
 * the first runnable process.
 */

#define	PPQ		4			/* priorities per queue */
#define	RUNQUE_NQS	(PRI_COUNT / PPQ)	/* number of runqueues */

typedef struct subqueue {
	TAILQ_HEAD(, lwp) sq_queue;
} subqueue_t;

typedef struct runqueue {
	subqueue_t	rq_subqueues[RUNQUE_NQS];	/* run queues */
	uint64_t	rq_bitmap;	/* bitmap of non-empty queues */
} runqueue_t;

static runqueue_t global_queue; 

static void updatepri(struct lwp *);
static void resetpriority(struct lwp *);
static void resetprocpriority(struct proc *);

extern unsigned int sched_pstats_ticks; /* defined in kern_synch.c */

/* The global scheduler state */
kmutex_t sched_mutex;

/* Number of hardclock ticks per sched_tick() */
int rrticks;

const int schedppq = PPQ;

/*
 * Force switch among equal priority processes every 100ms.
 * Called from hardclock every hz/10 == rrticks hardclock ticks.
 */
/* ARGSUSED */
void
sched_tick(struct cpu_info *ci)
{
	struct schedstate_percpu *spc = &ci->ci_schedstate;

	spc->spc_ticks = rrticks;

	if (!CURCPU_IDLE_P()) {
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
	cpu_need_resched(curcpu(), 0);
}

#define	NICE_WEIGHT 	1			/* priorities per nice level */

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

/*
 * sched_pstats_hook:
 *
 * Periodically called from sched_pstats(); used to recalculate priorities.
 */
void
sched_pstats_hook(struct proc *p, int minslp)
{
	struct lwp *l;
	fixpt_t loadfac = loadfactor(averunnable.ldavg[0]);

	/*
	 * If the process has slept the entire second,
	 * stop recalculating its priority until it wakes up.
	 */
	if (minslp <= 1) {
		p->p_estcpu = decay_cpu(loadfac, p->p_estcpu);
		
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			if ((l->l_flag & LW_IDLE) != 0)
				continue;
			lwp_lock(l);
			if (l->l_slptime <= 1 && l->l_priority < PRI_KERNEL)
				resetpriority(l);
			lwp_unlock(l);
		}
	}
}

/*
 * Recalculate the priority of a process after it has slept for a while.
 */
static void
updatepri(struct lwp *l)
{
	struct proc *p = l->l_proc;
	fixpt_t loadfac;

	KASSERT(lwp_locked(l, NULL));
	KASSERT(l->l_slptime > 1);

	loadfac = loadfactor(averunnable.ldavg[0]);

	l->l_slptime--; /* the first time was done in sched_pstats */
	/* XXX NJWLWP */
	/* XXXSMP occasionally unlocked, should be per-LWP */
	p->p_estcpu = decay_cpu_batch(loadfac, p->p_estcpu, l->l_slptime);
	resetpriority(l);
}

/*
 * On some architectures, it's faster to use a MSB ordering for the priorites
 * than the traditional LSB ordering.
 */
#define	RQMASK(n)	(1ULL << (n))
#define WHICHQ(p)	(RUNQUE_NQS - 1 - ((p) / PPQ))

/*
 * The primitives that manipulate the run queues.  whichqs tells which of
 * the queues have processes in them.  sched_enqueue() puts processes into
 * queues, sched_dequeue() removes them from queues.
 */
#ifdef RQDEBUG
static void
runqueue_check(const runqueue_t *rq, int whichq, struct lwp *l)
{
	const subqueue_t * const sq = &rq->rq_subqueues[whichq];
	const uint32_t bitmap = rq->rq_bitmap;
	struct lwp *l2;
	int found = 0;
	int die = 0;
	int empty = 1;

	TAILQ_FOREACH(l2, &sq->sq_queue, l_runq) {
		if (l2->l_stat != LSRUN) {
			printf("runqueue_check[%d]: lwp %p state (%d) "
			    " != LSRUN\n", whichq, l2, l2->l_stat);
		}
		if (l2 == l)
			found = 1;
		empty = 0;
	}
	if (empty && (bitmap & RQMASK(whichq)) != 0) {
		printf("runqueue_check[%d]: bit set for empty run-queue %p\n",
		    whichq, rq);
		die = 1;
	} else if (!empty && (bitmap & RQMASK(whichq)) == 0) {
		printf("runqueue_check[%d]: bit clear for non-empty "
		    "run-queue %p\n", whichq, rq);
		die = 1;
	}
	if (l != NULL && (bitmap & RQMASK(whichq)) == 0) {
		printf("runqueue_check[%d]: bit clear for active lwp %p\n",
		    whichq, l);
		die = 1;
	}
	if (l != NULL && empty) {
		printf("runqueue_check[%d]: empty run-queue %p with "
		    "active lwp %p\n", whichq, rq, l);
		die = 1;
	}
	if (l != NULL && !found) {
		printf("runqueue_check[%d]: lwp %p not in runqueue %p!",
		    whichq, l, rq);
		die = 1;
	}
	if (die)
		panic("runqueue_check: inconsistency found");
}
#else /* RQDEBUG */
#define	runqueue_check(a, b, c)	/* nothing */
#endif /* RQDEBUG */

static void
runqueue_init(runqueue_t *rq)
{
	int i;

	for (i = 0; i < RUNQUE_NQS; i++)
		TAILQ_INIT(&rq->rq_subqueues[i].sq_queue);
}

static void
runqueue_enqueue(runqueue_t *rq, struct lwp *l)
{
	subqueue_t *sq;
	const int whichq = WHICHQ(lwp_eprio(l));
	const uint64_t rqmask = RQMASK(whichq);

	KASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));

	runqueue_check(rq, whichq, NULL);
	rq->rq_bitmap |= rqmask;
	sq = &rq->rq_subqueues[whichq];
	TAILQ_INSERT_TAIL(&sq->sq_queue, l, l_runq);
	runqueue_check(rq, whichq, l);
}

static void
runqueue_dequeue(runqueue_t *rq, struct lwp *l)
{
	subqueue_t *sq;
	const int whichq = WHICHQ(lwp_eprio(l));
	const uint64_t rqmask = RQMASK(whichq);

	KASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));

	runqueue_check(rq, whichq, l);
	KASSERT((rq->rq_bitmap & rqmask) != 0);
	sq = &rq->rq_subqueues[whichq];
	TAILQ_REMOVE(&sq->sq_queue, l, l_runq);
	if (TAILQ_EMPTY(&sq->sq_queue))
		rq->rq_bitmap &= ~rqmask;
	runqueue_check(rq, whichq, NULL);
}

static struct lwp *
runqueue_nextlwp(runqueue_t *rq)
{
	const uint64_t bitmap = rq->rq_bitmap;
	int whichq;

	if (bitmap == 0) {
		return NULL;
	}
	whichq = ffs((uint32_t)bitmap) - 1;
	if (whichq != -1)
		return TAILQ_FIRST(&rq->rq_subqueues[whichq].sq_queue);
	whichq = ffs((uint32_t)(bitmap >> 32)) - 1;
	return TAILQ_FIRST(&rq->rq_subqueues[whichq + 32].sq_queue);
}

#if defined(DDB)
static void
runqueue_print(const runqueue_t *rq, void (*pr)(const char *, ...))
{
	const uint64_t bitmap = rq->rq_bitmap;
	struct lwp *l;
	int i, first;

	for (i = 0; i < RUNQUE_NQS; i++) {
		const subqueue_t *sq;
		first = 1;
		sq = &rq->rq_subqueues[i];
		TAILQ_FOREACH(l, &sq->sq_queue, l_runq) {
			if (first) {
				(*pr)("%c%d",
				    (bitmap & RQMASK(i)) ? ' ' : '!', i);
				first = 0;
			}
			(*pr)("\t%d.%d (%s) pri=%d usrpri=%d\n",
			    l->l_proc->p_pid,
			    l->l_lid, l->l_proc->p_comm,
			    (int)l->l_priority, (int)l->l_usrpri);
		}
	}
}
#endif /* defined(DDB) */

/*
 * Initialize the (doubly-linked) run queues
 * to be empty.
 */
void
sched_rqinit()
{

	runqueue_init(&global_queue);
	mutex_init(&sched_mutex, MUTEX_SPIN, IPL_SCHED);
	/* Initialize the lock pointer for lwp0 */
	lwp0.l_mutex = &curcpu()->ci_schedstate.spc_lwplock;
}

void
sched_cpuattach(struct cpu_info *ci)
{
	runqueue_t *rq;

	ci->ci_schedstate.spc_mutex = &sched_mutex;
	rq = kmem_zalloc(sizeof(*rq), KM_NOSLEEP);
	runqueue_init(rq);
	ci->ci_schedstate.spc_sched_info = rq;
}

void
sched_setup()
{

	rrticks = hz / 10;
}

void
sched_setrunnable(struct lwp *l)
{

 	if (l->l_slptime > 1)
 		updatepri(l);
}

bool
sched_curcpu_runnable_p(void)
{
	runqueue_t *rq = curcpu()->ci_schedstate.spc_sched_info;

	return (global_queue.rq_bitmap | rq->rq_bitmap) != 0;
}

void
sched_nice(struct proc *chgp, int n)
{

	chgp->p_nice = n;
	(void)resetprocpriority(chgp);
}

/*
 * Compute the priority of a process when running in user mode.
 * Arrange to reschedule if the resulting priority is better
 * than that of the current process.
 */
static void
resetpriority(struct lwp *l)
{
	unsigned int newpriority;
	struct proc *p = l->l_proc;

	/* XXXSMP KASSERT(mutex_owned(&p->p_stmutex)); */
	KASSERT(lwp_locked(l, NULL));

	if ((l->l_flag & LW_SYSTEM) != 0)
		return;

	newpriority = PRI_KERNEL - 1 - (p->p_estcpu >> ESTCPU_SHIFT) -
	    NICE_WEIGHT * (p->p_nice - NZERO);
	newpriority = max(newpriority, 0);
	lwp_changepri(l, newpriority);
}

/*
 * Recompute priority for all LWPs in a process.
 */
static void
resetprocpriority(struct proc *p)
{
	struct lwp *l;

	KASSERT(mutex_owned(&p->p_stmutex));

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
sched_schedclock(struct lwp *l)
{
	struct proc *p = l->l_proc;

	KASSERT(!CURCPU_IDLE_P());
	mutex_spin_enter(&p->p_stmutex);
	p->p_estcpu = ESTCPULIM(p->p_estcpu + (1 << ESTCPU_SHIFT));
	lwp_lock(l);
	resetpriority(l);
	mutex_spin_exit(&p->p_stmutex);
	if ((l->l_flag & LW_SYSTEM) == 0 && l->l_priority < PRI_KERNEL)
		l->l_priority = l->l_usrpri;
	lwp_unlock(l);
}

/*
 * sched_proc_fork:
 *
 *	Inherit the parent's scheduler history.
 */
void
sched_proc_fork(struct proc *parent, struct proc *child)
{

	KASSERT(mutex_owned(&parent->p_smutex));

	child->p_estcpu = child->p_estcpu_inherited = parent->p_estcpu;
	child->p_forktime = sched_pstats_ticks;
}

/*
 * sched_proc_exit:
 *
 *	Chargeback parents for the sins of their children.
 */
void
sched_proc_exit(struct proc *parent, struct proc *child)
{
	fixpt_t loadfac = loadfactor(averunnable.ldavg[0]);
	fixpt_t estcpu;

	/* XXX Only if parent != init?? */

	mutex_spin_enter(&parent->p_stmutex);
	estcpu = decay_cpu_batch(loadfac, child->p_estcpu_inherited,
	    sched_pstats_ticks - child->p_forktime);
	if (child->p_estcpu > estcpu)
		parent->p_estcpu =
		    ESTCPULIM(parent->p_estcpu + child->p_estcpu - estcpu);
	mutex_spin_exit(&parent->p_stmutex);
}

void
sched_enqueue(struct lwp *l, bool ctxswitch)
{

	if ((l->l_flag & LW_BOUND) != 0)
		runqueue_enqueue(l->l_cpu->ci_schedstate.spc_sched_info, l);
	else
		runqueue_enqueue(&global_queue, l);
}

/*
 * XXXSMP When LWP dispatch (cpu_switch()) is changed to use sched_dequeue(),
 * drop of the effective priority level from kernel to user needs to be
 * moved here from userret().  The assignment in userret() is currently
 * done unlocked.
 */
void
sched_dequeue(struct lwp *l)
{

	if ((l->l_flag & LW_BOUND) != 0)
		runqueue_dequeue(l->l_cpu->ci_schedstate.spc_sched_info, l);
	else
		runqueue_dequeue(&global_queue, l);
}

struct lwp *
sched_nextlwp(void)
{
	lwp_t *l1, *l2;

	/* For now, just pick the highest priority LWP. */
	l1 = runqueue_nextlwp(curcpu()->ci_schedstate.spc_sched_info);
	l2 = runqueue_nextlwp(&global_queue);

	if (l1 == NULL)
		return l2;
	if (l2 == NULL)
		return l1;
	if (lwp_eprio(l2) > lwp_eprio(l1))
		return l2;
	else
		return l1;
}

/* Dummy */
void
sched_lwp_fork(struct lwp *l)
{

}

void
sched_lwp_exit(struct lwp *l)
{

}

/* SysCtl */

SYSCTL_SETUP(sysctl_sched_setup, "sysctl kern.sched subtree setup")
{
	const struct sysctlnode *node = NULL;

	sysctl_createv(clog, 0, NULL, NULL,
		CTLFLAG_PERMANENT,
		CTLTYPE_NODE, "kern", NULL,
		NULL, 0, NULL, 0,
		CTL_KERN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, &node,
		CTLFLAG_PERMANENT,
		CTLTYPE_NODE, "sched",
		SYSCTL_DESCR("Scheduler options"),
		NULL, 0, NULL, 0,
		CTL_KERN, CTL_CREATE, CTL_EOL);

	if (node != NULL) {
		sysctl_createv(clog, 0, &node, NULL,
			CTLFLAG_PERMANENT,
			CTLTYPE_STRING, "name", NULL,
			NULL, 0, __UNCONST("4.4BSD"), 0,
			CTL_CREATE, CTL_EOL);
	}
}

#if defined(DDB)
void
sched_print_runqueue(void (*pr)(const char *, ...))
{

	runqueue_print(&global_queue, pr);
}
#endif /* defined(DDB) */
