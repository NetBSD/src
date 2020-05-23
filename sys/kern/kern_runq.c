/*	$NetBSD: kern_runq.c,v 1.69 2020/05/23 21:24:41 ad Exp $	*/

/*-
 * Copyright (c) 2019, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * Copyright (c) 2007, 2008 Mindaugas Rasiukevicius <rmind at NetBSD org>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_runq.c,v 1.69 2020/05/23 21:24:41 ad Exp $");

#include "opt_dtrace.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/bitops.h>
#include <sys/cpu.h>
#include <sys/idle.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/pset.h>
#include <sys/sched.h>
#include <sys/syscallargs.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>

/*
 * Bits per map.
 */
#define	BITMAP_BITS	(32)
#define	BITMAP_SHIFT	(5)
#define	BITMAP_MSB	(0x80000000U)
#define	BITMAP_MASK	(BITMAP_BITS - 1)

const int	schedppq = 1;

static void	*sched_getrq(struct schedstate_percpu *, const pri_t);
#ifdef MULTIPROCESSOR
static lwp_t *	sched_catchlwp(struct cpu_info *);
#endif

/*
 * Preemption control.
 */
#ifdef __HAVE_PREEMPTION
# ifdef DEBUG
int		sched_kpreempt_pri = 0;
# else
int		sched_kpreempt_pri = PRI_USER_RT;
# endif
#else
int		sched_kpreempt_pri = 1000;
#endif

/*
 * Migration and balancing.
 */
static u_int	cacheht_time;	/* Cache hotness time */
static u_int	min_catch;	/* Minimal LWP count for catching */
static u_int	skim_interval;	/* Rate limit for stealing LWPs */

#ifdef KDTRACE_HOOKS
struct lwp *curthread;
#endif

void
runq_init(void)
{

	/* Pulling from remote packages, LWP must not have run for 10ms. */
	cacheht_time = 10;

	/* Minimal count of LWPs for catching */
	min_catch = 1;

	/* Steal from other CPUs at most every 10ms. */
	skim_interval = 10;
}

void
sched_cpuattach(struct cpu_info *ci)
{
	struct schedstate_percpu *spc;
	size_t size;
	void *p;
	u_int i;

	spc = &ci->ci_schedstate;
	spc->spc_nextpkg = ci;

	if (spc->spc_lwplock == NULL) {
		spc->spc_lwplock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SCHED);
	}
	if (ci == lwp0.l_cpu) {
		/* Initialize the scheduler structure of the primary LWP */
		lwp0.l_mutex = spc->spc_lwplock;
	}
	if (spc->spc_mutex != NULL) {
		/* Already initialized. */
		return;
	}

	/* Allocate the run queue */
	size = roundup2(sizeof(spc->spc_queue[0]) * PRI_COUNT, coherency_unit) +
	    coherency_unit;
	p = kmem_alloc(size, KM_SLEEP);
	spc->spc_queue = (void *)roundup2((uintptr_t)p, coherency_unit);

	/* Initialize run queues */
	spc->spc_mutex = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SCHED);
	for (i = 0; i < PRI_COUNT; i++)
		TAILQ_INIT(&spc->spc_queue[i]);
}

/*
 * Control of the runqueue.
 */
static inline void *
sched_getrq(struct schedstate_percpu *spc, const pri_t prio)
{

	KASSERT(prio < PRI_COUNT);
	return &spc->spc_queue[prio];
}

/*
 * Put an LWP onto a run queue.  The LWP must be locked by spc_mutex for
 * l_cpu.
 */
void
sched_enqueue(struct lwp *l)
{
	struct schedstate_percpu *spc;
	TAILQ_HEAD(, lwp) *q_head;
	const pri_t eprio = lwp_eprio(l);
	struct cpu_info *ci;

	ci = l->l_cpu;
	spc = &ci->ci_schedstate;
	KASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));

	/* Enqueue the thread */
	q_head = sched_getrq(spc, eprio);
	if (TAILQ_EMPTY(q_head)) {
		u_int i;
		uint32_t q;

		/* Mark bit */
		i = eprio >> BITMAP_SHIFT;
		q = BITMAP_MSB >> (eprio & BITMAP_MASK);
		KASSERT((spc->spc_bitmap[i] & q) == 0);
		spc->spc_bitmap[i] |= q;
	}

	/*
	 * Determine run queue position according to POSIX.  XXX Explicitly
	 * lowering a thread's priority with pthread_setschedparam() is not
	 * handled.
	 */
	if ((l->l_pflag & LP_PREEMPTING) != 0) {
		switch (l->l_class) {
		case SCHED_OTHER:
			TAILQ_INSERT_TAIL(q_head, l, l_runq);
			break;
		case SCHED_FIFO:
			TAILQ_INSERT_HEAD(q_head, l, l_runq);
			break;
		case SCHED_RR:
			if (getticks() - l->l_rticks >= sched_rrticks) {
				TAILQ_INSERT_TAIL(q_head, l, l_runq);
			} else {
				TAILQ_INSERT_HEAD(q_head, l, l_runq);
			}
			break;
		default: /* SCHED_OTHER */
			panic("sched_enqueue: LWP %p has class %d\n",
			    l, l->l_class);
		}
	} else {
		TAILQ_INSERT_TAIL(q_head, l, l_runq);
	}
	spc->spc_flags &= ~SPCF_IDLE;
	spc->spc_count++;
	if ((l->l_pflag & LP_BOUND) == 0) {
		atomic_store_relaxed(&spc->spc_mcount,
		    atomic_load_relaxed(&spc->spc_mcount) + 1);
	}

	/*
	 * Update the value of highest priority in the runqueue,
	 * if priority of this thread is higher.
	 */
	if (eprio > spc->spc_maxpriority)
		spc->spc_maxpriority = eprio;

	sched_newts(l);
}

/*
 * Remove and LWP from the run queue it's on.  The LWP must be in state
 * LSRUN.
 */
void
sched_dequeue(struct lwp *l)
{
	TAILQ_HEAD(, lwp) *q_head;
	struct schedstate_percpu *spc;
	const pri_t eprio = lwp_eprio(l);

	spc = &l->l_cpu->ci_schedstate;

	KASSERT(lwp_locked(l, spc->spc_mutex));
	KASSERT(eprio <= spc->spc_maxpriority);
	KASSERT(spc->spc_bitmap[eprio >> BITMAP_SHIFT] != 0);
	KASSERT(spc->spc_count > 0);

	if (spc->spc_migrating == l)
		spc->spc_migrating = NULL;

	spc->spc_count--;
	if ((l->l_pflag & LP_BOUND) == 0) {
		atomic_store_relaxed(&spc->spc_mcount,
		    atomic_load_relaxed(&spc->spc_mcount) - 1);
	}

	q_head = sched_getrq(spc, eprio);
	TAILQ_REMOVE(q_head, l, l_runq);
	if (TAILQ_EMPTY(q_head)) {
		u_int i;
		uint32_t q;

		/* Unmark bit */
		i = eprio >> BITMAP_SHIFT;
		q = BITMAP_MSB >> (eprio & BITMAP_MASK);
		KASSERT((spc->spc_bitmap[i] & q) != 0);
		spc->spc_bitmap[i] &= ~q;

		/*
		 * Update the value of highest priority in the runqueue, in a
		 * case it was a last thread in the queue of highest priority.
		 */
		if (eprio != spc->spc_maxpriority)
			return;

		do {
			if (spc->spc_bitmap[i] != 0) {
				q = ffs(spc->spc_bitmap[i]);
				spc->spc_maxpriority =
				    (i << BITMAP_SHIFT) + (BITMAP_BITS - q);
				return;
			}
		} while (i--);

		/* If not found - set the lowest value */
		spc->spc_maxpriority = 0;
	}
}

/*
 * Cause a preemption on the given CPU, if the priority "pri" is higher
 * priority than the running LWP.  If "unlock" is specified, and ideally it
 * will be for concurrency reasons, spc_mutex will be dropped before return.
 */
void
sched_resched_cpu(struct cpu_info *ci, pri_t pri, bool unlock)
{
	struct schedstate_percpu *spc;
	u_int o, n, f;
	lwp_t *l;

	spc = &ci->ci_schedstate;

	KASSERT(mutex_owned(spc->spc_mutex));

	/*
	 * If the priority level we're evaluating wouldn't cause a new LWP
	 * to be run on the CPU, then we have nothing to do.
	 */
	if (pri <= spc->spc_curpriority || !mp_online) {
		if (__predict_true(unlock)) {
			spc_unlock(ci);
		}
		return;
	}

	/*
	 * Figure out what kind of preemption we should do.
	 */	
	l = ci->ci_onproc;
	if ((l->l_flag & LW_IDLE) != 0) {
		f = RESCHED_IDLE | RESCHED_UPREEMPT;
	} else if (pri >= sched_kpreempt_pri && (l->l_pflag & LP_INTR) == 0) {
		/* We can't currently preempt softints - should be able to. */
#ifdef __HAVE_PREEMPTION
		f = RESCHED_KPREEMPT;
#else
		/* Leave door open for test: set kpreempt_pri with sysctl. */
		f = RESCHED_UPREEMPT;
#endif
		/*
		 * l_dopreempt must be set with the CPU locked to sync with
		 * mi_switch().  It must also be set with an atomic to sync
		 * with kpreempt().
		 */
		atomic_or_uint(&l->l_dopreempt, DOPREEMPT_ACTIVE);
	} else {
		f = RESCHED_UPREEMPT;
	}
	if (ci != curcpu()) {
		f |= RESCHED_REMOTE;
	}

	/*
	 * Things can start as soon as ci_want_resched is touched: x86 has
	 * an instruction that monitors the memory cell it's in.  Drop the
	 * schedstate lock in advance, otherwise the remote CPU can awaken
	 * and immediately block on the lock.
	 */
	if (__predict_true(unlock)) {
		spc_unlock(ci);
	}

	/*
	 * The caller almost always has a second scheduler lock held: either
	 * the running LWP lock (spc_lwplock), or a sleep queue lock.  That
	 * keeps preemption disabled, which among other things ensures all
	 * LWPs involved won't be freed while we're here (see lwp_dtor()).
	 */
 	KASSERT(kpreempt_disabled());

	for (o = 0;; o = n) {
		n = atomic_cas_uint(&ci->ci_want_resched, o, o | f);
		if (__predict_true(o == n)) {
			/*
			 * We're the first to set a resched on the CPU.  Try
			 * to avoid causing a needless trip through trap()
			 * to handle an AST fault, if it's known the LWP
			 * will either block or go through userret() soon.
			 */
			if (l != curlwp || cpu_intr_p()) {
				cpu_need_resched(ci, l, f);
			}
			break;
		}
		if (__predict_true(
		    (n & (RESCHED_KPREEMPT|RESCHED_UPREEMPT)) >=
		    (f & (RESCHED_KPREEMPT|RESCHED_UPREEMPT)))) {
			/* Already in progress, nothing to do. */
			break;
		}
	}
}

/*
 * Cause a preemption on the given CPU, if the priority of LWP "l" in state
 * LSRUN, is higher priority than the running LWP.  If "unlock" is
 * specified, and ideally it will be for concurrency reasons, spc_mutex will
 * be dropped before return.
 */
void
sched_resched_lwp(struct lwp *l, bool unlock)
{
	struct cpu_info *ci = l->l_cpu;

	KASSERT(lwp_locked(l, ci->ci_schedstate.spc_mutex));
	KASSERT(l->l_stat == LSRUN);

	sched_resched_cpu(ci, lwp_eprio(l), unlock);
}

/*
 * Migration and balancing.
 */

#ifdef MULTIPROCESSOR

/*
 * Estimate if LWP is cache-hot.
 */
static inline bool
lwp_cache_hot(const struct lwp *l)
{

	/* Leave new LWPs in peace, determination has already been made. */
	if (l->l_stat == LSIDL)
		return true;

	if (__predict_false(l->l_slptime != 0 || l->l_rticks == 0))
		return false;

	return (getticks() - l->l_rticks < mstohz(cacheht_time));
}

/*
 * Check if LWP can migrate to the chosen CPU.
 */
static inline bool
sched_migratable(const struct lwp *l, struct cpu_info *ci)
{
	const struct schedstate_percpu *spc = &ci->ci_schedstate;
	KASSERT(lwp_locked(__UNCONST(l), NULL));

	/* Is CPU offline? */
	if (__predict_false(spc->spc_flags & SPCF_OFFLINE))
		return false;

	/* Is affinity set? */
	if (__predict_false(l->l_affinity))
		return kcpuset_isset(l->l_affinity, cpu_index(ci));

	/* Is there a processor-set? */
	return (spc->spc_psid == l->l_psid);
}

/*
 * A small helper to do round robin through CPU packages.
 */
static struct cpu_info *
sched_nextpkg(void)
{
	struct schedstate_percpu *spc = &curcpu()->ci_schedstate;

	spc->spc_nextpkg = 
	    spc->spc_nextpkg->ci_sibling[CPUREL_PACKAGE1ST];

	return spc->spc_nextpkg;
}

/*
 * Find a CPU to run LWP "l".  Look for the CPU with the lowest priority
 * thread.  In case of equal priority, prefer first class CPUs, and amongst
 * the remainder choose the CPU with the fewest runqueue entries.
 *
 * Begin the search in the CPU package which "pivot" is a member of.
 */
static struct cpu_info * __noinline 
sched_bestcpu(struct lwp *l, struct cpu_info *pivot)
{
	struct cpu_info *bestci, *curci, *outer;
	struct schedstate_percpu *bestspc, *curspc;
	pri_t bestpri, curpri;

	/*
	 * If this fails (it shouldn't), run on the given CPU.  This also
	 * gives us a weak preference for "pivot" to begin with.
	 */
	bestci = pivot;
	bestspc = &bestci->ci_schedstate;
	if (sched_migratable(l, bestci)) {
		bestpri = MAX(bestspc->spc_curpriority,
		    bestspc->spc_maxpriority);
	} else {
		/* Invalidate the priority. */
		bestpri = PRI_COUNT;
	}

	/* In the outer loop scroll through all CPU packages. */
	pivot = pivot->ci_package1st;
	outer = pivot;
	do {
		/* In the inner loop scroll through all CPUs in package. */
		curci = outer;
		do {
			if (!sched_migratable(l, curci)) {
				continue;
			}

			curspc = &curci->ci_schedstate;

			/* If this CPU is idle and 1st class, we're done. */
			if ((curspc->spc_flags & (SPCF_IDLE | SPCF_1STCLASS)) ==
			    (SPCF_IDLE | SPCF_1STCLASS)) {
				return curci;
			}

			curpri = MAX(curspc->spc_curpriority,
			    curspc->spc_maxpriority);

			if (curpri > bestpri) {
				continue;
			}
			if (curpri == bestpri) {
				/* Prefer first class CPUs over others. */
				if ((curspc->spc_flags & SPCF_1STCLASS) == 0 &&
				    (bestspc->spc_flags & SPCF_1STCLASS) != 0) {
				    	continue;
				}
				/*
				 * Pick the least busy CPU.  Make sure this is not
				 * <=, otherwise it defeats the above preference.
				 */
				if (bestspc->spc_count < curspc->spc_count) {
					continue;
				}
			}

			bestpri = curpri;
			bestci = curci;
			bestspc = curspc;

		} while (curci = curci->ci_sibling[CPUREL_PACKAGE],
		    curci != outer);
	} while (outer = outer->ci_sibling[CPUREL_PACKAGE1ST],
	    outer != pivot);

	return bestci;
}

/*
 * Estimate the migration of LWP to the other CPU.
 * Take and return the CPU, if migration is needed.
 */
struct cpu_info *
sched_takecpu(struct lwp *l)
{
	struct schedstate_percpu *spc, *tspc;
	struct cpu_info *ci, *curci, *tci;
	pri_t eprio;
	int flags;

	KASSERT(lwp_locked(l, NULL));

	/* If thread is strictly bound, do not estimate other CPUs */
	ci = l->l_cpu;
	if (l->l_pflag & LP_BOUND)
		return ci;

	spc = &ci->ci_schedstate;
	eprio = lwp_eprio(l);

	/*
	 * Handle new LWPs.  For vfork() with a timeshared child, make it
	 * run on the same CPU as the parent if no other LWPs in queue. 
	 * Otherwise scatter far and wide - try for an even distribution
	 * across all CPU packages and CPUs.
	 */
	if (l->l_stat == LSIDL) {
		if (curlwp->l_vforkwaiting && l->l_class == SCHED_OTHER) {
			if (sched_migratable(l, curlwp->l_cpu) && eprio >
			    curlwp->l_cpu->ci_schedstate.spc_maxpriority) {
				return curlwp->l_cpu;
			}
		} else {
			return sched_bestcpu(l, sched_nextpkg());
		}
		flags = SPCF_IDLE;
	} else {
		flags = SPCF_IDLE | SPCF_1STCLASS;
	}

	/*
	 * Try to send the LWP back to the first CPU in the same core if
	 * idle.  This keeps LWPs clustered in the run queues of 1st class
	 * CPUs.  This implies stickiness.  If we didn't find a home for
	 * a vfork() child above, try to use any SMT sibling to help out.
	 */
	tci = ci;
	do {
		tspc = &tci->ci_schedstate;
		if ((tspc->spc_flags & flags) == flags &&
		    sched_migratable(l, tci)) {
			return tci;
		}
		tci = tci->ci_sibling[CPUREL_CORE];
	} while (tci != ci);

	/*
	 * Otherwise the LWP is "sticky", i.e.  generally preferring to stay
	 * on the same CPU.
	 */
	if (sched_migratable(l, ci) && (eprio > spc->spc_curpriority ||
	    (lwp_cache_hot(l) && l->l_class == SCHED_OTHER))) {
		return ci;
	}

	/*
	 * If the current CPU core is idle, run there and avoid the
	 * expensive scan of CPUs below.
	 */
	curci = curcpu();
	tci = curci;
	do {
		tspc = &tci->ci_schedstate;
		if ((tspc->spc_flags & flags) == flags &&
		    sched_migratable(l, tci)) {
			return tci;
		}
		tci = tci->ci_sibling[CPUREL_CORE];
	} while (tci != curci);

	/*
	 * Didn't find a new home above - happens infrequently.  Start the
	 * search in last CPU package that the LWP ran in, but expand to
	 * include the whole system if needed.
	 */
	return sched_bestcpu(l, l->l_cpu);
}

/*
 * Tries to catch an LWP from the runqueue of other CPU.
 */
static struct lwp *
sched_catchlwp(struct cpu_info *ci)
{
	struct cpu_info *curci = curcpu();
	struct schedstate_percpu *spc, *curspc;
	TAILQ_HEAD(, lwp) *q_head;
	struct lwp *l;
	bool gentle;

	curspc = &curci->ci_schedstate;
	spc = &ci->ci_schedstate;

	/*
	 * Be more aggressive if this CPU is first class, and the other
	 * is not.
	 */
	gentle = ((curspc->spc_flags & SPCF_1STCLASS) == 0 ||
	    (spc->spc_flags & SPCF_1STCLASS) != 0);

	if (atomic_load_relaxed(&spc->spc_mcount) < (gentle ? min_catch : 1) ||
	    curspc->spc_psid != spc->spc_psid) {
		spc_unlock(ci);
		return NULL;
	}

	/* Take the highest priority thread */
	q_head = sched_getrq(spc, spc->spc_maxpriority);
	l = TAILQ_FIRST(q_head);

	for (;;) {
		/* Check the first and next result from the queue */
		if (l == NULL) {
			break;
		}
		KASSERTMSG(l->l_stat == LSRUN, "%s l %p (%s) l_stat %d",
		    ci->ci_data.cpu_name,
		    l, (l->l_name ? l->l_name : l->l_proc->p_comm), l->l_stat);

		/* Look for threads, whose are allowed to migrate */
		if ((l->l_pflag & LP_BOUND) ||
		    (gentle && lwp_cache_hot(l)) ||
		    !sched_migratable(l, curci)) {
			l = TAILQ_NEXT(l, l_runq);
			/* XXX Gap: could walk down priority list. */
			continue;
		}

		/* Grab the thread, and move to the local run queue */
		sched_dequeue(l);
		l->l_cpu = curci;
		lwp_unlock_to(l, curspc->spc_mutex);
		sched_enqueue(l);
		return l;
	}
	spc_unlock(ci);

	return l;
}

/*
 * Called from sched_idle() to handle migration.  Return the CPU that we
 * pushed the LWP to (may be NULL).
 */
static struct cpu_info *
sched_idle_migrate(void)
{
	struct cpu_info *ci = curcpu(), *tci = NULL;
	struct schedstate_percpu *spc, *tspc;
	bool dlock = false;

	spc = &ci->ci_schedstate;
	spc_lock(ci);
	for (;;) {
		struct lwp *l;

		l = spc->spc_migrating;
		if (l == NULL)
			break;

		/*
		 * If second attempt, and target CPU has changed,
		 * drop the old lock.
		 */
		if (dlock == true && tci != l->l_target_cpu) {
			KASSERT(tci != NULL);
			spc_unlock(tci);
			dlock = false;
		}

		/*
		 * Nothing to do if destination has changed to the
		 * local CPU, or migration was done by other CPU.
		 */
		tci = l->l_target_cpu;
		if (tci == NULL || tci == ci) {
			spc->spc_migrating = NULL;
			l->l_target_cpu = NULL;
			break;
		}
		tspc = &tci->ci_schedstate;

		/*
		 * Double-lock the runqueues.
		 * We do that only once.
		 */
		if (dlock == false) {
			dlock = true;
			if (ci < tci) {
				spc_lock(tci);
			} else if (!mutex_tryenter(tspc->spc_mutex)) {
				spc_unlock(ci);
				spc_lock(tci);
				spc_lock(ci);
				/* Check the situation again.. */
				continue;
			}
		}

		/* Migrate the thread */
		KASSERT(l->l_stat == LSRUN);
		spc->spc_migrating = NULL;
		l->l_target_cpu = NULL;
		sched_dequeue(l);
		l->l_cpu = tci;
		lwp_setlock(l, tspc->spc_mutex);
		sched_enqueue(l);
		sched_resched_lwp(l, true);
		/* tci now unlocked */
		spc_unlock(ci);
		return tci;
	}
	if (dlock == true) {
		KASSERT(tci != NULL);
		spc_unlock(tci);
	}
	spc_unlock(ci);
	return NULL;
}

/*
 * Try to steal an LWP from "tci".
 */
static bool
sched_steal(struct cpu_info *ci, struct cpu_info *tci)
{
	struct schedstate_percpu *spc, *tspc;
	lwp_t *l;

	spc = &ci->ci_schedstate;
	tspc = &tci->ci_schedstate;
	if (atomic_load_relaxed(&tspc->spc_mcount) != 0 &&
	    spc->spc_psid == tspc->spc_psid) {
		spc_dlock(ci, tci);
		l = sched_catchlwp(tci);
		spc_unlock(ci);
		if (l != NULL) {
			return true;
		}
	}
	return false;
}

/*
 * Called from each CPU's idle loop.
 */
void
sched_idle(void)
{
	struct cpu_info *ci, *inner, *outer, *first, *tci, *mci;
	struct schedstate_percpu *spc, *tspc;
	struct lwp *l;

	ci = curcpu();
	spc = &ci->ci_schedstate;
	tci = NULL;
	mci = NULL;

	/*
	 * Handle LWP migrations off this CPU to another.  If there a is
	 * migration to do then remember the CPU the LWP was sent to, and
	 * don't steal the LWP back from that CPU below.
	 */
	if (spc->spc_migrating != NULL) {
		mci = sched_idle_migrate();
	}

	/* If this CPU is offline, or we have an LWP to run, we're done. */
	if ((spc->spc_flags & SPCF_OFFLINE) != 0 || spc->spc_count != 0) {
		return;
	}

	/* Deal with SMT. */
	if (ci->ci_nsibling[CPUREL_CORE] > 1) {
		/* Try to help our siblings out. */
		tci = ci->ci_sibling[CPUREL_CORE];
		while (tci != ci) {
			if (tci != mci && sched_steal(ci, tci)) {
				return;
			}
			tci = tci->ci_sibling[CPUREL_CORE];
		}
		/*
		 * If not the first SMT in the core, and in the default
		 * processor set, the search ends here.
		 */
		if ((spc->spc_flags & SPCF_1STCLASS) == 0 &&
		    spc->spc_psid == PS_NONE) {
			return;
		}
	}

	/*
	 * Find something to run, unless this CPU exceeded the rate limit. 
	 * Start looking on the current package to maximise L2/L3 cache
	 * locality.  Then expand to looking at the rest of the system.
	 *
	 * XXX Should probably look at 2nd class CPUs first, but they will
	 * shed jobs via preempt() anyway.
	 */
	if (spc->spc_nextskim > getticks()) {
		return;
	}
	spc->spc_nextskim = getticks() + mstohz(skim_interval);

	/* In the outer loop scroll through all CPU packages, starting here. */
	first = ci->ci_package1st;
	outer = first;
	do {
		/* In the inner loop scroll through all CPUs in package. */
		inner = outer;
		do {
			/* Don't hit the locks unless needed. */
			tspc = &inner->ci_schedstate;
			if (ci == inner || ci == mci ||
			    spc->spc_psid != tspc->spc_psid ||
			    atomic_load_relaxed(&tspc->spc_mcount) < min_catch) {
				continue;
			}
			spc_dlock(ci, inner);
			l = sched_catchlwp(inner);
			spc_unlock(ci);
			if (l != NULL) {
				/* Got it! */
				return;
			}
		} while (inner = inner->ci_sibling[CPUREL_PACKAGE],
		    inner != outer);
	} while (outer = outer->ci_sibling[CPUREL_PACKAGE1ST],
	    outer != first);
}

/*
 * Called from mi_switch() when an LWP has been preempted / has yielded. 
 * The LWP is presently in the CPU's run queue.  Here we look for a better
 * CPU to teleport the LWP to; there may not be one.
 */
void
sched_preempted(struct lwp *l)
{
	const int flags = SPCF_IDLE | SPCF_1STCLASS;
	struct schedstate_percpu *tspc;
	struct cpu_info *ci, *tci;

	ci = l->l_cpu;
	tspc = &ci->ci_schedstate;

	KASSERT(tspc->spc_count >= 1);

	/*
	 * Try to select another CPU if:
	 *
	 * - there is no migration pending already
	 * - and this LWP is running on a 2nd class CPU
	 * - or this LWP is a child of vfork() that has just done execve()
	 */
	if (l->l_target_cpu != NULL ||
	    ((tspc->spc_flags & SPCF_1STCLASS) != 0 &&
	    (l->l_pflag & LP_TELEPORT) == 0)) {
		return;
	}

	/*
	 * Fast path: if the first SMT in the core is idle, send it back
	 * there, because the cache is shared (cheap) and we want all LWPs
	 * to be clustered on 1st class CPUs (either running there or on
	 * their runqueues).
	 */
	tci = ci->ci_sibling[CPUREL_CORE];
	while (tci != ci) {
		tspc = &tci->ci_schedstate;
		if ((tspc->spc_flags & flags) == flags &&
		    sched_migratable(l, tci)) {
		    	l->l_target_cpu = tci;
			l->l_pflag &= ~LP_TELEPORT;
		    	return;
		}
		tci = tci->ci_sibling[CPUREL_CORE];
	}

	if ((l->l_pflag & LP_TELEPORT) != 0) {
		/*
		 * A child of vfork(): now that the parent is released,
		 * scatter far and wide, to match the LSIDL distribution
		 * done in sched_takecpu().
		 */
		l->l_pflag &= ~LP_TELEPORT;
		tci = sched_bestcpu(l, sched_nextpkg());
		if (tci != ci) {
			l->l_target_cpu = tci;
		}
	} else {
		/*
		 * Try to find a better CPU to take it, but don't move to
		 * another 2nd class CPU, and don't move to a non-idle CPU,
		 * because that would prevent SMT being used to maximise
		 * throughput.
		 *
		 * Search in the current CPU package in order to try and
		 * keep L2/L3 cache locality, but expand to include the
		 * whole system if needed.
		 */
		tci = sched_bestcpu(l, l->l_cpu);
		if (tci != ci &&
		    (tci->ci_schedstate.spc_flags & flags) == flags) {
			l->l_target_cpu = tci;
		}
	}
}

/*
 * Called during execve() by a child of vfork().  Does two things:
 *
 * - If the parent has been awoken and put back on curcpu then give the
 *   CPU back to the parent.
 *
 * - If curlwp is not on a 1st class CPU then find somewhere else to run,
 *   since it dodged the distribution in sched_takecpu() when first set
 *   runnable.
 */
void
sched_vforkexec(struct lwp *l, bool samecpu)
{

	KASSERT(l == curlwp);
	if ((samecpu && ncpu > 1) ||
	    (l->l_cpu->ci_schedstate.spc_flags & SPCF_1STCLASS) == 0) {
		l->l_pflag |= LP_TELEPORT;
		preempt();
	}
}

#else

/*
 * stubs for !MULTIPROCESSOR
 */

struct cpu_info *
sched_takecpu(struct lwp *l)
{

	return l->l_cpu;
}

void
sched_idle(void)
{

}

void
sched_preempted(struct lwp *l)
{

}

void
sched_vforkexec(struct lwp *l, bool samecpu)
{

	KASSERT(l == curlwp);
}

#endif	/* MULTIPROCESSOR */

/*
 * Scheduling statistics and balancing.
 */
void
sched_lwp_stats(struct lwp *l)
{
	int batch;

	KASSERT(lwp_locked(l, NULL));

	/* Update sleep time */
	if (l->l_stat == LSSLEEP || l->l_stat == LSSTOP ||
	    l->l_stat == LSSUSPENDED)
		l->l_slptime++;

	/*
	 * Set that thread is more CPU-bound, if sum of run time exceeds the
	 * sum of sleep time.  Check if thread is CPU-bound a first time.
	 */
	batch = (l->l_rticksum > l->l_slpticksum);
	if (batch != 0) {
		if ((l->l_flag & LW_BATCH) == 0)
			batch = 0;
		l->l_flag |= LW_BATCH;
	} else
		l->l_flag &= ~LW_BATCH;

	/* Reset the time sums */
	l->l_slpticksum = 0;
	l->l_rticksum = 0;

	/* Scheduler-specific hook */
	sched_pstats_hook(l, batch);
#ifdef KDTRACE_HOOKS
	curthread = l;
#endif
}

/*
 * Scheduler mill.
 */
struct lwp *
sched_nextlwp(void)
{
	struct cpu_info *ci = curcpu();
	struct schedstate_percpu *spc;
	TAILQ_HEAD(, lwp) *q_head;
	struct lwp *l;

	/* Update the last run time on switch */
	l = curlwp;
	l->l_rticksum += (getticks() - l->l_rticks);

	/* Return to idle LWP if there is a migrating thread */
	spc = &ci->ci_schedstate;
	if (__predict_false(spc->spc_migrating != NULL))
		return NULL;

	/* Return to idle LWP if there is no runnable job */
	if (__predict_false(spc->spc_count == 0))
		return NULL;

	/* Take the highest priority thread */
	KASSERT(spc->spc_bitmap[spc->spc_maxpriority >> BITMAP_SHIFT]);
	q_head = sched_getrq(spc, spc->spc_maxpriority);
	l = TAILQ_FIRST(q_head);
	KASSERT(l != NULL);

	sched_oncpu(l);
	l->l_rticks = getticks();

	return l;
}

/*
 * sched_curcpu_runnable_p: return if curcpu() should exit the idle loop.
 */

bool
sched_curcpu_runnable_p(void)
{
	const struct cpu_info *ci;
	const struct schedstate_percpu *spc;
	bool rv;

	kpreempt_disable();
	ci = curcpu();
	spc = &ci->ci_schedstate;
	rv = (spc->spc_count != 0);
#ifndef __HAVE_FAST_SOFTINTS
	rv |= (ci->ci_data.cpu_softints != 0);
#endif
	kpreempt_enable();

	return rv;
}

/*
 * Sysctl nodes and initialization.
 */

SYSCTL_SETUP(sysctl_sched_setup, "sysctl sched setup")
{
	const struct sysctlnode *node = NULL;

	sysctl_createv(clog, 0, NULL, &node,
		CTLFLAG_PERMANENT,
		CTLTYPE_NODE, "sched",
		SYSCTL_DESCR("Scheduler options"),
		NULL, 0, NULL, 0,
		CTL_KERN, CTL_CREATE, CTL_EOL);

	if (node == NULL)
		return;

	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "cacheht_time",
		SYSCTL_DESCR("Cache hotness time (in ms)"),
		NULL, 0, &cacheht_time, 0,
		CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "skim_interval",
		SYSCTL_DESCR("Rate limit for stealing from other CPUs (in ms)"),
		NULL, 0, &skim_interval, 0,
		CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "min_catch",
		SYSCTL_DESCR("Minimal count of threads for catching"),
		NULL, 0, &min_catch, 0,
		CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "timesoftints",
		SYSCTL_DESCR("Track CPU time for soft interrupts"),
		NULL, 0, &softint_timing, 0,
		CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "kpreempt_pri",
		SYSCTL_DESCR("Minimum priority to trigger kernel preemption"),
		NULL, 0, &sched_kpreempt_pri, 0,
		CTL_CREATE, CTL_EOL);
}

/*
 * Debugging.
 */

#ifdef DDB

void
sched_print_runqueue(void (*pr)(const char *, ...))
{
	struct cpu_info *ci, *tci;
	struct schedstate_percpu *spc;
	struct lwp *l;
	struct proc *p;
	CPU_INFO_ITERATOR cii;

	for (CPU_INFO_FOREACH(cii, ci)) {
		int i;

		spc = &ci->ci_schedstate;

		(*pr)("Run-queue (CPU = %u):\n", ci->ci_index);
		(*pr)(" pid.lid = %d.%d, r_count = %u, "
		    "maxpri = %d, mlwp = %p\n",
#ifdef MULTIPROCESSOR
		    ci->ci_curlwp->l_proc->p_pid, ci->ci_curlwp->l_lid,
#else
		    curlwp->l_proc->p_pid, curlwp->l_lid,
#endif
		    spc->spc_count, spc->spc_maxpriority,
		    spc->spc_migrating);
		i = (PRI_COUNT >> BITMAP_SHIFT) - 1;
		do {
			uint32_t q;
			q = spc->spc_bitmap[i];
			(*pr)(" bitmap[%d] => [ %d (0x%x) ]\n", i, ffs(q), q);
		} while (i--);
	}

	(*pr)("   %5s %4s %4s %10s %3s %18s %4s %4s %s\n",
	    "LID", "PRI", "EPRI", "FL", "ST", "LWP", "CPU", "TCI", "LRTICKS");

	PROCLIST_FOREACH(p, &allproc) {
		(*pr)(" /- %d (%s)\n", (int)p->p_pid, p->p_comm);
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			ci = l->l_cpu;
			tci = l->l_target_cpu;
			(*pr)(" | %5d %4u %4u 0x%8.8x %3s %18p %4u %4d %u\n",
			    (int)l->l_lid, l->l_priority, lwp_eprio(l),
			    l->l_flag, l->l_stat == LSRUN ? "RQ" :
			    (l->l_stat == LSSLEEP ? "SQ" : "-"),
			    l, ci->ci_index, (tci ? tci->ci_index : -1),
			    (u_int)(getticks() - l->l_rticks));
		}
	}
}

#endif
