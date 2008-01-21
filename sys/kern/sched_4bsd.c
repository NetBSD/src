/*	$NetBSD: sched_4bsd.c,v 1.4.4.6 2008/01/21 09:46:16 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sched_4bsd.c,v 1.4.4.6 2008/01/21 09:46:16 yamt Exp $");

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
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

/*
 * Run queues.
 *
 * We maintain bitmasks of non-empty queues in order speed up finding
 * the first runnable process.  Since there can be (by definition) few
 * real time LWPs in the the system, we maintain them on a linked list,
 * sorted by priority.
 */

#define	PPB_SHIFT	5
#define	PPB_MASK	31

#define	NUM_Q		(NPRI_KERNEL + NPRI_USER)
#define	NUM_PPB		(1 << PPB_SHIFT)
#define	NUM_B		(NUM_Q / NUM_PPB)

typedef struct runqueue {
	TAILQ_HEAD(, lwp) rq_fixedpri;		/* realtime, kthread */
	u_int		rq_count;		/* total # jobs */
	uint32_t	rq_bitmap[NUM_B];	/* bitmap of queues */
	TAILQ_HEAD(, lwp) rq_queue[NUM_Q];	/* user+kernel */
} runqueue_t;

static runqueue_t global_queue; 

static void updatepri(struct lwp *);
static void resetpriority(struct lwp *);

fixpt_t decay_cpu(fixpt_t, fixpt_t);

extern unsigned int sched_pstats_ticks; /* defined in kern_synch.c */

/* The global scheduler state */
kmutex_t runqueue_lock;

/* Number of hardclock ticks per sched_tick() */
static int rrticks;

const int schedppq = 1;

/*
 * Force switch among equal priority processes every 100ms.
 * Called from hardclock every hz/10 == rrticks hardclock ticks.
 *
 * There's no need to lock anywhere in this routine, as it's
 * CPU-local and runs at IPL_SCHED (called from clock interrupt).
 */
/* ARGSUSED */
void
sched_tick(struct cpu_info *ci)
{
	struct schedstate_percpu *spc = &ci->ci_schedstate;

	spc->spc_ticks = rrticks;

	if (CURCPU_IDLE_P())
		return;

	if (spc->spc_flags & SPCF_SEENRR) {
		/*
		 * The process has already been through a roundrobin
		 * without switching and may be hogging the CPU.
		 * Indicate that the process should yield.
		 */
		spc->spc_flags |= SPCF_SHOULDYIELD;
	} else
		spc->spc_flags |= SPCF_SEENRR;

	cpu_need_resched(ci, 0);
}

/*
 * Why PRIO_MAX - 2? From setpriority(2):
 *
 *	prio is a value in the range -20 to 20.  The default priority is
 *	0; lower priorities cause more favorable scheduling.  A value of
 *	19 or 20 will schedule a process only when nothing at priority <=
 *	0 is runnable.
 *
 * This gives estcpu influence over 18 priority levels, and leaves nice
 * with 40 levels.  One way to think about it is that nice has 20 levels
 * either side of estcpu's 18.
 */
#define	ESTCPU_SHIFT	11
#define	ESTCPU_MAX	((PRIO_MAX - 2) << ESTCPU_SHIFT)
#define	ESTCPU_ACCUM	(1 << (ESTCPU_SHIFT - 1))
#define	ESTCPULIM(e)	min((e), ESTCPU_MAX)

/*
 * Constants for digital decay and forget:
 *	90% of (l_estcpu) usage in 5 * loadav time
 *	95% of (l_pctcpu) usage in 60 seconds (load insensitive)
 *          Note that, as ps(1) mentions, this can let percentages
 *          total over 100% (I've seen 137.9% for 3 processes).
 *
 * Note that hardclock updates l_estcpu and l_cpticks independently.
 *
 * We wish to decay away 90% of l_estcpu in (5 * loadavg) seconds.
 * That is, the system wants to compute a value of decay such
 * that the following for loop:
 * 	for (i = 0; i < (5 * loadavg); i++)
 * 		l_estcpu *= decay;
 * will compute
 * 	l_estcpu *= 0.1;
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

fixpt_t
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
 * For all load averages >= 1 and max l_estcpu of (255 << ESTCPU_SHIFT),
 * sleeping for at least seven times the loadfactor will decay l_estcpu to
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
sched_pstats_hook(struct lwp *l)
{
	fixpt_t loadfac;
	int sleeptm;

	/*
	 * If the LWP has slept an entire second, stop recalculating
	 * its priority until it wakes up.
	 */
	if (l->l_stat == LSSLEEP || l->l_stat == LSSTOP ||
	    l->l_stat == LSSUSPENDED) {
		l->l_slptime++;
		sleeptm = 1;
	} else {
		sleeptm = 0x7fffffff;
	}

	if (l->l_slptime <= sleeptm) {
		loadfac = 2 * (averunnable.ldavg[0]);
		l->l_estcpu = decay_cpu(loadfac, l->l_estcpu);
		resetpriority(l);
	}
}

/*
 * Recalculate the priority of a process after it has slept for a while.
 */
static void
updatepri(struct lwp *l)
{
	fixpt_t loadfac;

	KASSERT(lwp_locked(l, NULL));
	KASSERT(l->l_slptime > 1);

	loadfac = loadfactor(averunnable.ldavg[0]);

	l->l_slptime--; /* the first time was done in sched_pstats */
	l->l_estcpu = decay_cpu_batch(loadfac, l->l_estcpu, l->l_slptime);
	resetpriority(l);
}

static void
runqueue_init(runqueue_t *rq)
{
	int i;

	for (i = 0; i < NUM_Q; i++)
		TAILQ_INIT(&rq->rq_queue[i]);
	for (i = 0; i < NUM_B; i++)
		rq->rq_bitmap[i] = 0;
	TAILQ_INIT(&rq->rq_fixedpri);
	rq->rq_count = 0;
}

static void
runqueue_enqueue(runqueue_t *rq, struct lwp *l)
{
	pri_t pri;
	lwp_t *l2;

	KASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));

	pri = lwp_eprio(l);
	rq->rq_count++;

	if (pri >= PRI_KTHREAD) {
		TAILQ_FOREACH(l2, &rq->rq_fixedpri, l_runq) {
			if (lwp_eprio(l2) < pri) {
				TAILQ_INSERT_BEFORE(l2, l, l_runq);
				return;
			}
		}
		TAILQ_INSERT_TAIL(&rq->rq_fixedpri, l, l_runq);
		return;
	}

	rq->rq_bitmap[pri >> PPB_SHIFT] |=
	    (0x80000000U >> (pri & PPB_MASK));
	TAILQ_INSERT_TAIL(&rq->rq_queue[pri], l, l_runq);
}

static void
runqueue_dequeue(runqueue_t *rq, struct lwp *l)
{
	pri_t pri;

	KASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));

	pri = lwp_eprio(l);
	rq->rq_count--;

	if (pri >= PRI_KTHREAD) {
		TAILQ_REMOVE(&rq->rq_fixedpri, l, l_runq);
		return;
	}

	TAILQ_REMOVE(&rq->rq_queue[pri], l, l_runq);
	if (TAILQ_EMPTY(&rq->rq_queue[pri]))
		rq->rq_bitmap[pri >> PPB_SHIFT] ^=
		    (0x80000000U >> (pri & PPB_MASK));
}

#if (NUM_B != 3) || (NUM_Q != 96)
#error adjust runqueue_nextlwp
#endif

static struct lwp *
runqueue_nextlwp(runqueue_t *rq)
{
	pri_t pri;

	KASSERT(rq->rq_count != 0);

	if (!TAILQ_EMPTY(&rq->rq_fixedpri))
		return TAILQ_FIRST(&rq->rq_fixedpri);

	if (rq->rq_bitmap[2] != 0)
		pri = 96 - ffs(rq->rq_bitmap[2]);
	else if (rq->rq_bitmap[1] != 0)
		pri = 64 - ffs(rq->rq_bitmap[1]);
	else
		pri = 32 - ffs(rq->rq_bitmap[0]);
	return TAILQ_FIRST(&rq->rq_queue[pri]);
}

#if defined(DDB)
static void
runqueue_print(const runqueue_t *rq, void (*pr)(const char *, ...))
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	lwp_t *l;
	int i;

	printf("PID\tLID\tPRI\tIPRI\tEPRI\tLWP\t\t NAME\n");

	TAILQ_FOREACH(l, &rq->rq_fixedpri, l_runq) {
		(*pr)("%d\t%d\%d\t%d\t%d\t%016lx %s\n",
		    l->l_proc->p_pid, l->l_lid, (int)l->l_priority,
		    (int)l->l_inheritedprio, lwp_eprio(l),
		    (long)l, l->l_proc->p_comm);
	}

	for (i = NUM_Q - 1; i >= 0; i--) {
		TAILQ_FOREACH(l, &rq->rq_queue[i], l_runq) {
			(*pr)("%d\t%d\t%d\t%d\t%d\t%016lx %s\n",
			    l->l_proc->p_pid, l->l_lid, (int)l->l_priority,
			    (int)l->l_inheritedprio, lwp_eprio(l),
			    (long)l, l->l_proc->p_comm);
		}
	}

	printf("CPUIDX\tRESCHED\tCURPRI\tFLAGS\n");
	for (CPU_INFO_FOREACH(cii, ci)) {
		printf("%d\t%d\t%d\t%04x\n", (int)ci->ci_index,
		    (int)ci->ci_want_resched,
		    (int)ci->ci_schedstate.spc_curpriority,
		    (int)ci->ci_schedstate.spc_flags);
	}

	printf("NEXTLWP\n%016lx\n", (long)sched_nextlwp());
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
	mutex_init(&runqueue_lock, MUTEX_DEFAULT, IPL_SCHED);
	/* Initialize the lock pointer for lwp0 */
	lwp0.l_mutex = &curcpu()->ci_schedstate.spc_lwplock;
}

void
sched_cpuattach(struct cpu_info *ci)
{
	runqueue_t *rq;

	ci->ci_schedstate.spc_mutex = &runqueue_lock;
	rq = kmem_zalloc(sizeof(*rq), KM_SLEEP);
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
	struct schedstate_percpu *spc;
	struct cpu_info *ci;
	int bits;

	ci = curcpu();
	spc = &ci->ci_schedstate;
#ifndef __HAVE_FAST_SOFTINTS
	bits = ci->ci_data.cpu_softints;
	bits |= ((runqueue_t *)spc->spc_sched_info)->rq_count;
#else
	bits = ((runqueue_t *)spc->spc_sched_info)->rq_count;
#endif
	if (__predict_true((spc->spc_flags & SPCF_OFFLINE) == 0))
		bits |= global_queue.rq_count;
	return bits != 0;
}

void
sched_nice(struct proc *p, int n)
{
	struct lwp *l;

	KASSERT(mutex_owned(&p->p_smutex));

	p->p_nice = n;
	LIST_FOREACH(l, &p->p_lwps, l_sibling) {
		lwp_lock(l);
		resetpriority(l);
		lwp_unlock(l);
	}
}

/*
 * Recompute the priority of an LWP.  Arrange to reschedule if
 * the resulting priority is better than that of the current LWP.
 */
static void
resetpriority(struct lwp *l)
{
	pri_t pri;
	struct proc *p = l->l_proc;

	KASSERT(lwp_locked(l, NULL));

	if (l->l_class != SCHED_OTHER)
		return;

	/* See comments above ESTCPU_SHIFT definition. */
	pri = (PRI_KERNEL - 1) - (l->l_estcpu >> ESTCPU_SHIFT) - p->p_nice;
	pri = imax(pri, 0);
	if (pri != l->l_priority)
		lwp_changepri(l, pri);
}

/*
 * We adjust the priority of the current process.  The priority of a process
 * gets worse as it accumulates CPU time.  The CPU usage estimator (l_estcpu)
 * is increased here.  The formula for computing priorities (in kern_synch.c)
 * will compute a different value each time l_estcpu increases. This can
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

	if (l->l_class != SCHED_OTHER)
		return;

	KASSERT(!CURCPU_IDLE_P());
	l->l_estcpu = ESTCPULIM(l->l_estcpu + ESTCPU_ACCUM);
	lwp_lock(l);
	resetpriority(l);
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
	lwp_t *pl;

	KASSERT(mutex_owned(&parent->p_smutex));

	pl = LIST_FIRST(&parent->p_lwps);
	child->p_estcpu_inherited = pl->l_estcpu;
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
	lwp_t *pl, *cl;

	/* XXX Only if parent != init?? */

	mutex_enter(&parent->p_smutex);
	pl = LIST_FIRST(&parent->p_lwps);
	cl = LIST_FIRST(&child->p_lwps);
	estcpu = decay_cpu_batch(loadfac, child->p_estcpu_inherited,
	    sched_pstats_ticks - child->p_forktime);
	if (cl->l_estcpu > estcpu) {
		lwp_lock(pl);
		pl->l_estcpu = ESTCPULIM(pl->l_estcpu + cl->l_estcpu - estcpu);
		lwp_unlock(pl);
	}
	mutex_exit(&parent->p_smutex);
}

void
sched_enqueue(struct lwp *l, bool ctxswitch)
{

	if (__predict_false(l->l_target_cpu != NULL)) {
		/* Global mutex is used - just change the CPU */
		l->l_cpu = l->l_target_cpu;
		l->l_target_cpu = NULL;
	}

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
	struct schedstate_percpu *spc;
	runqueue_t *rq;
	lwp_t *l1, *l2;

	spc = &curcpu()->ci_schedstate;

	/* For now, just pick the highest priority LWP. */
	rq = spc->spc_sched_info;
	l1 = NULL;
	if (rq->rq_count != 0)
		l1 = runqueue_nextlwp(rq);

	rq = &global_queue;
	if (__predict_false((spc->spc_flags & SPCF_OFFLINE) != 0) ||
	    rq->rq_count == 0)
		return l1;
	l2 = runqueue_nextlwp(rq);

	if (l1 == NULL)
		return l2;
	if (l2 == NULL)
		return l1;
	if (lwp_eprio(l2) > lwp_eprio(l1))
		return l2;
	else
		return l1;
}

struct cpu_info *
sched_takecpu(struct lwp *l)
{

	return l->l_cpu;
}

void
sched_wakeup(struct lwp *l)
{

}

void
sched_slept(struct lwp *l)
{

}

void
sched_lwp_fork(struct lwp *l1, struct lwp *l2)
{

	l2->l_estcpu = l1->l_estcpu;
}

void
sched_lwp_exit(struct lwp *l)
{

}

void
sched_lwp_collect(struct lwp *t)
{
	lwp_t *l;

	/* Absorb estcpu value of collected LWP. */
	l = curlwp;
	lwp_lock(l);
	l->l_estcpu += t->l_estcpu;
	lwp_unlock(l);
}

/*
 * Sysctl nodes and initialization.
 */

static int
sysctl_sched_rtts(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int rttsms = hztoms(rrticks);

	node = *rnode;
	node.sysctl_data = &rttsms;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

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

	KASSERT(node != NULL);

	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT,
		CTLTYPE_STRING, "name", NULL,
		NULL, 0, __UNCONST("4.4BSD"), 0,
		CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT,
		CTLTYPE_INT, "rtts",
		SYSCTL_DESCR("Round-robin time quantum (in miliseconds)"),
		sysctl_sched_rtts, 0, NULL, 0,
		CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_READWRITE,
		CTLTYPE_INT, "timesoftints",
		SYSCTL_DESCR("Track CPU time for soft interrupts"),
		NULL, 0, &softint_timing, 0,
		CTL_CREATE, CTL_EOL);
}

#if defined(DDB)
void
sched_print_runqueue(void (*pr)(const char *, ...))
{

	runqueue_print(&global_queue, pr);
}
#endif /* defined(DDB) */
