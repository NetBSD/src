/*	$NetBSD: sched_m2.c,v 1.4 2007/10/13 12:05:32 yamt Exp $	*/

/*
 * Copyright (c) 2007, Mindaugas Rasiukevicius
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
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
 * TODO:
 *  - Implementation of fair share queue;
 *  - Support for NUMA;
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sched_m2.c,v 1.4 2007/10/13 12:05:32 yamt Exp $");

#include <sys/param.h>

#include <sys/cpu.h>
#include <sys/callout.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/sched.h>
#include <sys/syscallargs.h>
#include <sys/sysctl.h>
#include <sys/types.h>

#include <machine/cpu.h>

/*
 * XXX: Some defintions below will dissapear
 * XXX: with the merge of vmlocking branch.
 */
#define	PRI_MAX		MAXPRI
#define	PRI_COUNT	(PRI_MAX + 1)			/* 0 .. 127  -> 128 */
#define	PRI_RT_COUNT	(50)				/* 0 .. 49   -> 50  */
#define	PRI_TS_COUNT	(PRI_COUNT - PRI_RT_COUNT)	/* 50 .. 127 -> 78  */

#define	PRI_DEFAULT	70				/* 70 */
#define	PRI_REALTIME	50				/* 50 */
#define	PRI_HTS_RANGE	10				/* 50 .. 60  -> 10 */

/*
 * Bits per map.
 */
#define	BITMAP_SHIFT		5		/* 32 bits */
#define	BITMAP_SIZE		PRI_COUNT >> BITMAP_SHIFT

/*
 * Time-slices and priorities.
 */
static u_int	min_ts;			/* Minimal time-slice */
static u_int	max_ts;			/* Maximal time-slice */
static u_int	rt_ts;			/* Real-time time-slice */
static u_int	ts_map[PRI_COUNT];	/* Map of time-slices */
static pri_t	high_pri[PRI_COUNT];	/* Map for priority increase */

/*
 * Migration and balancing.
 */
#ifdef MULTIPROCESSOR
static u_int	cacheht_time;		/* Cache hotness time */
static u_int	min_catch;		/* Minimal LWP count for catching */

static u_int		balance_period;	/* Balance period */
static struct callout	balance_ch;	/* Callout of balancer */

static struct cpu_info * volatile worker_ci;

#define CACHE_HOT(sil)		(sil->sl_lrtime && \
    (hardclock_ticks - sil->sl_lrtime < cacheht_time))

#endif

/*
 * Structures, runqueue.
 */

typedef struct {
	TAILQ_HEAD(, lwp) q_head;
} queue_t;

typedef struct {
	/* Lock and bitmap */
	kmutex_t	r_rq_mutex;
	uint32_t	r_bitmap[BITMAP_SIZE];
	/* Counters */
	u_int		r_count;	/* Count of the threads */
	pri_t		r_highest_pri;	/* Highest priority */
	u_int		r_avgcount;	/* Average count of threads */
	u_int		r_mcount;	/* Count of migratable threads */
	/* Runqueues */
	queue_t		r_rt_queue[PRI_RT_COUNT];
	queue_t		r_ts_queue[PRI_TS_COUNT];
} runqueue_t;

typedef struct {
	u_int		sl_flags;
	u_int		sl_timeslice;	/* Time-slice of thread */
	u_int		sl_slept;	/* Saved sleep time for sleep sum */
	u_int		sl_slpsum;	/* Sum of sleep time */
	u_int		sl_rtime;	/* Saved start time of run */
	u_int		sl_rtsum;	/* Sum of the run time */
	u_int		sl_lrtime;	/* Last run time */
} sched_info_lwp_t;

/* Flags */
#define	SL_BATCH	0x01

/* Pool of the scheduler-specific structures for threads */
static struct pool	sil_pool;

/*
 * Prototypes.
 */

static inline void *	sched_getrq(runqueue_t *, const pri_t);
static inline void	sched_newts(struct lwp *);
static void		sched_precalcts(void);

#ifdef MULTIPROCESSOR
static struct lwp *	sched_catchlwp(void);
static void		sched_balance(void *);
#endif

/*
 * Initialization and setup.
 */

void
sched_rqinit(void)
{
	struct cpu_info *ci = curcpu();

	if (hz < 100) {
		panic("sched_rqinit: value of HZ is too low\n");
	}

	/* Default timing ranges */
	min_ts = mstohz(50);			/* ~50ms  */
	max_ts = mstohz(150);			/* ~150ms */
	rt_ts = mstohz(100);			/* ~100ms */
	sched_precalcts();

#ifdef MULTIPROCESSOR
	/* Balancing */
	worker_ci = ci;
	cacheht_time = mstohz(5);		/* ~5 ms  */
	balance_period = mstohz(300);		/* ~300ms */
	min_catch = ~0;
#endif

	/* Pool of the scheduler-specific structures */
	pool_init(&sil_pool, sizeof(sched_info_lwp_t), 0, 0, 0,
	    "lwpsd", &pool_allocator_nointr, IPL_NONE);

	/* Attach the primary CPU here */
	sched_cpuattach(ci);

	/* Initialize the scheduler structure of the primary LWP */
	lwp0.l_mutex = &ci->ci_schedstate.spc_lwplock;
	sched_lwp_fork(&lwp0);
	sched_newts(&lwp0);
}

void
sched_setup(void)
{

#ifdef MULTIPROCESSOR
	/* Minimal count of LWPs for catching: log2(count of CPUs) */
	min_catch = min(ffs(ncpu) - 1, 4);

	/* Initialize balancing callout and run it */
	callout_init(&balance_ch, CALLOUT_MPSAFE);
	callout_setfunc(&balance_ch, sched_balance, NULL);
	callout_schedule(&balance_ch, balance_period);
#endif
}

void
sched_cpuattach(struct cpu_info *ci)
{
	runqueue_t *ci_rq;
	void *rq_ptr;
	u_int i, size;

	/*
	 * Allocate the run queue.
	 * XXX: Estimate cache behaviour more..
	 */
	size = roundup(sizeof(runqueue_t), CACHE_LINE_SIZE) + CACHE_LINE_SIZE;
	rq_ptr = kmem_zalloc(size, KM_NOSLEEP);
	if (rq_ptr == NULL) {
		panic("scheduler: could not allocate the runqueue");
	}
	/* XXX: Save the original pointer for future.. */
	ci_rq = (void *)(roundup((intptr_t)(rq_ptr), CACHE_LINE_SIZE));

	/* Initialize run queues */
	mutex_init(&ci_rq->r_rq_mutex, MUTEX_SPIN, IPL_SCHED);
	for (i = 0; i < PRI_RT_COUNT; i++)
		TAILQ_INIT(&ci_rq->r_rt_queue[i].q_head);
	for (i = 0; i < PRI_TS_COUNT; i++)
		TAILQ_INIT(&ci_rq->r_ts_queue[i].q_head);
	ci_rq->r_highest_pri = PRI_MAX;

	ci->ci_schedstate.spc_sched_info = ci_rq;
	ci->ci_schedstate.spc_mutex = &ci_rq->r_rq_mutex;
}

/* Pre-calculate the time-slices for the priorities */
static void
sched_precalcts(void)
{
	pri_t p;
	u_int i;

	for (p = 0; p < PRI_REALTIME; p++) {
		ts_map[p] = rt_ts;
		high_pri[p] = p;
	}

	for (p = PRI_REALTIME, i = 0; p < PRI_COUNT; p++, i++) {
		ts_map[p] = min_ts +
		    (i * 100 / (PRI_TS_COUNT - 1) * (max_ts - min_ts) / 100);
		high_pri[p] = PRI_REALTIME + (i * PRI_HTS_RANGE /
		    (PRI_MAX - PRI_REALTIME));
	}
}

/*
 * Hooks.
 */

void
sched_proc_fork(struct proc *parent, struct proc *child)
{
	struct lwp *l;

	LIST_FOREACH(l, &child->p_lwps, l_sibling) {
		lwp_lock(l);
		sched_newts(l);
		lwp_unlock(l);
	}
}

void
sched_proc_exit(struct proc *child, struct proc *parent)
{

	/* Dummy */
}

void
sched_lwp_fork(struct lwp *l)
{

	KASSERT(l->l_sched_info == NULL);
	l->l_sched_info = pool_get(&sil_pool, PR_WAITOK);
	memset(l->l_sched_info, 0, sizeof(sched_info_lwp_t));
	if (l->l_usrpri >= PRI_REALTIME) /* XXX: For now only.. */
		l->l_usrpri = l->l_priority = PRI_DEFAULT;
}

void
sched_lwp_exit(struct lwp *l)
{

	KASSERT(l->l_sched_info != NULL);
	pool_put(&sil_pool, l->l_sched_info);
	l->l_sched_info = NULL;
}

void
sched_setrunnable(struct lwp *l)
{

	/* Dummy */
}

void
sched_schedclock(struct lwp *l)
{

	/* Dummy */
}

/*
 * Priorities and time-slice.
 */

void
sched_nice(struct proc *p, int prio)
{
	int nprio;
	struct lwp *l;

	KASSERT(mutex_owned(&p->p_stmutex));

	p->p_nice = prio;
	nprio = max(PRI_DEFAULT + p->p_nice, PRI_REALTIME);

	LIST_FOREACH(l, &p->p_lwps, l_sibling) {
		lwp_lock(l);
		lwp_changepri(l, nprio);
		lwp_unlock(l);
	}
}

/* Recalculate the time-slice */
static inline void
sched_newts(struct lwp *l)
{
	sched_info_lwp_t *sil = l->l_sched_info;

	sil->sl_timeslice = ts_map[lwp_eprio(l)];
}

/*
 * Control of the runqueue.
 */

static inline void *
sched_getrq(runqueue_t *ci_rq, const pri_t prio)
{

	KASSERT(prio < PRI_COUNT);
	return (prio < PRI_REALTIME) ?
	    &ci_rq->r_rt_queue[prio].q_head :
	    &ci_rq->r_ts_queue[prio - PRI_REALTIME].q_head;
}

void
sched_enqueue(struct lwp *l, bool swtch)
{
	runqueue_t *ci_rq;
	sched_info_lwp_t *sil = l->l_sched_info;
	TAILQ_HEAD(, lwp) *q_head;
	const pri_t eprio = lwp_eprio(l);

	ci_rq = l->l_cpu->ci_schedstate.spc_sched_info;
	KASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));

	/* Update the last run time on switch */
	if (swtch == true) {
		sil->sl_lrtime = hardclock_ticks;
		sil->sl_rtsum += (hardclock_ticks - sil->sl_rtime);
	} else
		sil->sl_lrtime = 0;

	/* Enqueue the thread */
	q_head = sched_getrq(ci_rq, eprio);
	if (TAILQ_EMPTY(q_head)) {
		u_int i;
		uint32_t q;

		/* Mark bit */
		i = eprio >> BITMAP_SHIFT;
		q = eprio - (i << BITMAP_SHIFT);
		KASSERT((ci_rq->r_bitmap[i] & (1 << q)) == 0);
		ci_rq->r_bitmap[i] |= 1 << q;
	}
	TAILQ_INSERT_TAIL(q_head, l, l_runq);
	ci_rq->r_count++;
	if ((l->l_flag & LW_BOUND) == 0)
		ci_rq->r_mcount++;

	/*
	 * Update the value of highest priority in the runqueue,
	 * if priority of this thread is higher.
	 */
	if (eprio < ci_rq->r_highest_pri)
		ci_rq->r_highest_pri = eprio;

	sched_newts(l);
}

void
sched_dequeue(struct lwp *l)
{
	runqueue_t *ci_rq;
	TAILQ_HEAD(, lwp) *q_head;
	const pri_t eprio = lwp_eprio(l);

	ci_rq = l->l_cpu->ci_schedstate.spc_sched_info;
	KASSERT(lwp_locked(l, l->l_cpu->ci_schedstate.spc_mutex));
	KASSERT(ci_rq->r_highest_pri <= eprio); 
	KASSERT(ci_rq->r_bitmap[eprio >> BITMAP_SHIFT] != 0);
	KASSERT(ci_rq->r_count > 0);

	ci_rq->r_count--;
	if ((l->l_flag & LW_BOUND) == 0)
		ci_rq->r_mcount--;

	q_head = sched_getrq(ci_rq, eprio);
	TAILQ_REMOVE(q_head, l, l_runq);
	if (TAILQ_EMPTY(q_head)) {
		u_int i;
		uint32_t q;

		/* Unmark bit */
		i = eprio >> BITMAP_SHIFT;
		q = eprio - (i << BITMAP_SHIFT);
		KASSERT((ci_rq->r_bitmap[i] & (1 << q)) != 0);
		ci_rq->r_bitmap[i] &= ~(1 << q);

		/*
		 * Update the value of highest priority in the runqueue, in a
		 * case it was a last thread in the queue of highest priority.
		 */
		if (eprio != ci_rq->r_highest_pri)
			return;

		do {
			q = ffs(ci_rq->r_bitmap[i]);
			if (q) {
				ci_rq->r_highest_pri =
				    (i << BITMAP_SHIFT) + q - 1;
				return;
			}
		} while (++i < BITMAP_SIZE);

		/* If not found - set the maximal value */
		ci_rq->r_highest_pri = PRI_MAX;
	}
}

void
sched_slept(struct lwp *l)
{
	sched_info_lwp_t *sil = l->l_sched_info;

	/* Save the time when thread has slept */
	sil->sl_slept = hardclock_ticks;

	/*
	 * If thread is not a real-time and batch flag is not marked,
	 * increase the the priority, and run with lower time-quantum.
	 */
	if (l->l_usrpri > PRI_REALTIME && (sil->sl_flags & SL_BATCH) == 0)
		l->l_usrpri--;
}

void
sched_wakeup(struct lwp *l)
{
	sched_info_lwp_t *sil = l->l_sched_info;

	/* Update sleep time delta */
	sil->sl_slpsum += (l->l_slptime == 0) ?
	    (hardclock_ticks - sil->sl_slept) : hz;

	/* If thread was sleeping a second or more - set a high priority */
	if (l->l_slptime > 1 || (hardclock_ticks - sil->sl_slept) >= hz)
		l->l_usrpri = l->l_priority = high_pri[l->l_usrpri];

	/* Also, consider looking for a better CPU to wake up */
	if ((l->l_flag & (LW_BOUND | LW_SYSTEM)) == 0)
		l->l_cpu = sched_takecpu(l);
}

void
sched_pstats_hook(struct lwp *l)
{
	sched_info_lwp_t *sil = l->l_sched_info;

	/*
	 * Set that thread is more CPU-bound, if sum of run time exceeds the
	 * sum of sleep time.  If it is CPU-bound not a first time - decrease
	 * the priority.
	 */
	if (sil->sl_rtsum > sil->sl_slpsum) {
		if ((sil->sl_flags & SL_BATCH) && (l->l_usrpri < PRI_MAX))
			l->l_usrpri++;
		sil->sl_flags |= SL_BATCH;
	} else {
		sil->sl_flags &= ~SL_BATCH;
	}
	sil->sl_slpsum = 0;
	sil->sl_rtsum = 0;

	/*
	 * Estimate only threads on time-sharing run queue, also,
	 * ignore the highest time-sharing priority.
	 */
	if (l->l_stat != LSRUN || l->l_usrpri <= PRI_REALTIME)
		return;

	/* If thread was not ran a second or more - set a high priority */
	if (sil->sl_lrtime && (hardclock_ticks - sil->sl_lrtime >= hz))
		lwp_changepri(l, high_pri[l->l_usrpri]);
}

/*
 * Migration and balancing.
 */

#ifdef MULTIPROCESSOR

/* Check if LWP can migrate to the chosen CPU */
static inline bool
sched_migratable(const struct lwp *l, const struct cpu_info *ci)
{

	if (ci->ci_schedstate.spc_flags & SPCF_OFFLINE)
		return false;

	if ((l->l_flag & LW_BOUND) == 0)
		return true;
#if 0
	return cpu_in_pset(ci, l->l_psid);
#else
	return false;
#endif
}

/*
 * Estimate the migration of LWP to the other CPU.
 * Take and return the CPU, if migration is needed.
 */
struct cpu_info *
sched_takecpu(struct lwp *l)
{
	struct cpu_info *ci, *tci = NULL;
	struct schedstate_percpu *spc;
	runqueue_t *ci_rq;
	sched_info_lwp_t *sil;
	CPU_INFO_ITERATOR cii;
	pri_t eprio, lpri;

	ci = l->l_cpu;
	spc = &ci->ci_schedstate;
	ci_rq = spc->spc_sched_info;

	/* CPU of this thread is idling - run there */
	if (ci_rq->r_count == 0)
		return ci;

	eprio = lwp_eprio(l);
	sil = l->l_sched_info;

	/* Stay if thread is cache-hot */
	if (l->l_stat == LSSLEEP && l->l_slptime <= 1 &&
	    CACHE_HOT(sil) && eprio <= spc->spc_curpriority)
		return ci;

	/* Run on current CPU if priority of thread is higher */
	ci = curcpu();
	spc = &ci->ci_schedstate;
	if (eprio < spc->spc_curpriority && sched_migratable(l, ci))
		return ci;

	/*
	 * Look for the CPU with the lowest priority thread.  In case of
	 * equal the priority - check the lower count of the threads.
	 */
	lpri = 0;
	ci_rq = NULL;
	tci = l->l_cpu;
	for (CPU_INFO_FOREACH(cii, ci)) {
		runqueue_t *ici_rq;
		pri_t pri;

		spc = &ci->ci_schedstate;
		ici_rq = spc->spc_sched_info;
		pri = min(spc->spc_curpriority, ici_rq->r_highest_pri);
		if (pri < lpri)
			continue;

		if (pri == lpri && ci_rq && ci_rq->r_count < ici_rq->r_count)
			continue;

		if (sched_migratable(l, ci) == false)
			continue;

		lpri = pri;
		tci = ci;
		ci_rq = ici_rq;
	}

	return tci;
}

/*
 * Tries to catch an LWP from the runqueue of other CPU.
 */
static struct lwp *
sched_catchlwp(void)
{
	struct cpu_info *curci = curcpu(), *ci = worker_ci;
	TAILQ_HEAD(, lwp) *q_head;
	runqueue_t *ci_rq;
	struct lwp *l;

	if (curci == ci)
		return NULL;

	/* Lockless check */
	ci_rq = ci->ci_schedstate.spc_sched_info;
	if (ci_rq->r_count < min_catch)
		return NULL;

	/*
	 * Double-lock the runqueues.
	 */
	if (curci < ci) {
		spc_lock(ci);
	} else if (!mutex_tryenter(ci->ci_schedstate.spc_mutex)) {
		const runqueue_t *cur_rq = curci->ci_schedstate.spc_sched_info;

		spc_unlock(curci);
		spc_lock(ci);
		spc_lock(curci);

		if (cur_rq->r_count) {
			spc_unlock(ci);
			return NULL;
		}
	}

	if (ci_rq->r_count < min_catch) {
		spc_unlock(ci);
		return NULL;
	}

	/* Take the highest priority thread */
	q_head = sched_getrq(ci_rq, ci_rq->r_highest_pri);
	l = TAILQ_FIRST(q_head);

	for (;;) {
		sched_info_lwp_t *sil;

		/* Check the first and next result from the queue */
		if (l == NULL)
			break;

		/* Look for threads, whose are allowed to migrate */
		sil = l->l_sched_info;
		if ((l->l_flag & LW_SYSTEM) || CACHE_HOT(sil) ||
		    sched_migratable(l, curci) == false) {
			l = TAILQ_NEXT(l, l_runq);
			continue;
		}
		/* Recheck if chosen thread is still on the runqueue */
		if (l->l_stat == LSRUN && (l->l_flag & LW_INMEM)) {
			sched_dequeue(l);
			l->l_cpu = curci;
			lwp_setlock(l, curci->ci_schedstate.spc_mutex);
			sched_enqueue(l, false);
			break;
		}
		l = TAILQ_NEXT(l, l_runq);
	}
	spc_unlock(ci);

	return l;
}

/*
 * Periodical calculations for balancing.
 */
static void
sched_balance(void *nocallout)
{
	struct cpu_info *ci, *hci;
	runqueue_t *ci_rq;
	CPU_INFO_ITERATOR cii;
	u_int highest;

	hci = curcpu();
	highest = 0;

	/* Make lockless countings */
	for (CPU_INFO_FOREACH(cii, ci)) {
		ci_rq = ci->ci_schedstate.spc_sched_info;

		/* Average count of the threads */
		ci_rq->r_avgcount = (ci_rq->r_avgcount + ci_rq->r_mcount) >> 1;

		/* Look for CPU with the highest average */
		if (ci_rq->r_avgcount > highest) {
			hci = ci;
			highest = ci_rq->r_avgcount;
		}
	}

	/* Update the worker */
	worker_ci = hci;

	if (nocallout == NULL)
		callout_schedule(&balance_ch, balance_period);
}

#else

struct cpu_info *
sched_takecpu(struct lwp *l)
{

	return l->l_cpu;
}

#endif	/* MULTIPROCESSOR */

/*
 * Scheduler mill.
 */
struct lwp *
sched_nextlwp(void)
{
	struct cpu_info *ci = curcpu();
	struct schedstate_percpu *spc;
	TAILQ_HEAD(, lwp) *q_head;
	sched_info_lwp_t *sil;
	runqueue_t *ci_rq;
	struct lwp *l;

	spc = &ci->ci_schedstate;
	ci_rq = ci->ci_schedstate.spc_sched_info;

#ifdef MULTIPROCESSOR
	/* If runqueue is empty, try to catch some thread from other CPU */
	if (spc->spc_flags & SPCF_OFFLINE) {
		if (ci_rq->r_mcount == 0)
			return NULL;
	} else if (ci_rq->r_count == 0) {
		/* Reset the counter, and call the balancer */
		ci_rq->r_avgcount = 0;
		sched_balance(ci);

		/* The re-locking will be done inside */
		return sched_catchlwp();
	}
#else
	if (ci_rq->r_count == 0)
		return NULL;
#endif

	/* Take the highest priority thread */
	KASSERT(ci_rq->r_bitmap[ci_rq->r_highest_pri >> BITMAP_SHIFT]);
	q_head = sched_getrq(ci_rq, ci_rq->r_highest_pri);
	l = TAILQ_FIRST(q_head);
	KASSERT(l != NULL);

	/* Update the counters */
	sil = l->l_sched_info;
	KASSERT(sil->sl_timeslice >= min_ts);
	KASSERT(sil->sl_timeslice <= max_ts);
	spc->spc_ticks = sil->sl_timeslice;
	sil->sl_rtime = hardclock_ticks;

	return l;
}

bool
sched_curcpu_runnable_p(void)
{
	const struct cpu_info *ci = curcpu();
	const runqueue_t *ci_rq = ci->ci_schedstate.spc_sched_info;

	if (ci->ci_schedstate.spc_flags & SPCF_OFFLINE)
		return ci_rq->r_mcount;

	return ci_rq->r_count;
}

/*
 * Time-driven events.
 */

/*
 * Called once per time-quantum.  This routine is CPU-local and runs at
 * IPL_SCHED, thus the locking is not needed.
 */
void
sched_tick(struct cpu_info *ci)
{
	const runqueue_t *ci_rq = ci->ci_schedstate.spc_sched_info;
	struct schedstate_percpu *spc = &ci->ci_schedstate;
	struct lwp *l = curlwp;
	sched_info_lwp_t *sil = l->l_sched_info;

	if (CURCPU_IDLE_P())
		return;

	switch (l->l_policy) {
	case SCHED_FIFO:
		/*
		 * Update the time-quantum, and continue running,
		 * if thread runs on FIFO real-time policy.
		 */
		spc->spc_ticks = sil->sl_timeslice;
		return;
	case SCHED_OTHER:
		/* Decrease the priority, and run with a higher time-quantum */
		if (l->l_usrpri < PRI_REALTIME)
			break;
		l->l_usrpri = min(l->l_usrpri + 1, PRI_MAX);
		l->l_priority = l->l_usrpri;
		break;
	}

	/*
	 * If there are higher priority threads or threads in the same queue,
	 * mark that thread should yield, otherwise, continue running.
	 */
	if (lwp_eprio(l) >= ci_rq->r_highest_pri) {
		spc->spc_flags |= SPCF_SHOULDYIELD;
		cpu_need_resched(ci, 0);
	} else
		spc->spc_ticks = sil->sl_timeslice;
}

/*
 * Sysctl nodes and initialization.
 */

static int
sysctl_sched_mints(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct cpu_info *ci;
	int error, newsize;
	CPU_INFO_ITERATOR cii;

	node = *rnode;
	node.sysctl_data = &newsize;

	newsize = hztoms(min_ts);
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (newsize < 1 || newsize > hz || newsize >= max_ts)
		return EINVAL;

	/* It is safe to do this in such order */
	for (CPU_INFO_FOREACH(cii, ci))
		spc_lock(ci);

	min_ts = mstohz(newsize);
	sched_precalcts();

	for (CPU_INFO_FOREACH(cii, ci))
		spc_unlock(ci);

	return 0;
}

static int
sysctl_sched_maxts(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct cpu_info *ci;
	int error, newsize;
	CPU_INFO_ITERATOR cii;

	node = *rnode;
	node.sysctl_data = &newsize;

	newsize = hztoms(max_ts);
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (newsize < 10 || newsize > hz || newsize <= min_ts)
		return EINVAL;

	/* It is safe to do this in such order */
	for (CPU_INFO_FOREACH(cii, ci))
		spc_lock(ci);

	max_ts = mstohz(newsize);
	sched_precalcts();

	for (CPU_INFO_FOREACH(cii, ci))
		spc_unlock(ci);

	return 0;
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

	if (node == NULL)
		return;

	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT,
		CTLTYPE_STRING, "name", NULL,
		NULL, 0, __UNCONST("M2"), 0,
		CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "maxts",
		SYSCTL_DESCR("Maximal time quantum (in microseconds)"),
		sysctl_sched_maxts, 0, &max_ts, 0,
		CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "mints",
		SYSCTL_DESCR("Minimal time quantum (in microseconds)"),
		sysctl_sched_mints, 0, &min_ts, 0,
		CTL_CREATE, CTL_EOL);

#ifdef MULTIPROCESSOR
	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "cacheht_time",
		SYSCTL_DESCR("Cache hotness time"),
		NULL, 0, &cacheht_time, 0,
		CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "balance_period",
		SYSCTL_DESCR("Balance period"),
		NULL, 0, &balance_period, 0,
		CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_INT, "min_catch",
		SYSCTL_DESCR("Minimal count of threads for catching"),
		NULL, 0, &min_catch, 0,
		CTL_CREATE, CTL_EOL);
#endif
}

/*
 * Debugging.
 */

#ifdef DDB

void
sched_print_runqueue(void (*pr)(const char *, ...))
{
	runqueue_t *ci_rq;
	sched_info_lwp_t *sil;
	struct lwp *l;
	struct proc *p;
	int i;

	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	for (CPU_INFO_FOREACH(cii, ci)) {
		ci_rq = ci->ci_schedstate.spc_sched_info;

		(*pr)("Run-queue (CPU = %d):\n", ci->ci_cpuid);
		(*pr)(" pid.lid = %d.%d, threads count = %u, "
		    "avgcount = %u, highest pri = %d\n",
		    ci->ci_curlwp->l_proc->p_pid, ci->ci_curlwp->l_lid,
		    ci_rq->r_count, ci_rq->r_avgcount, ci_rq->r_highest_pri);
		i = 0;
		do {
			int b;
			b = ci_rq->r_bitmap[i];
			(*pr)(" bitmap[%d] => [ %d (0x%x) ]\n", i, ffs(b), b);
		} while (++i < BITMAP_SIZE);
	}

	(*pr)("   %5s %4s %4s %10s %3s %4s %11s %3s %s\n",
	    "LID", "PRI", "UPRI", "FL", "ST", "TS", "LWP", "CPU", "LRTIME");

	PROCLIST_FOREACH(p, &allproc) {
		(*pr)(" /- %d (%s)\n", (int)p->p_pid, p->p_comm);
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			sil = l->l_sched_info;
			ci = l->l_cpu;
			(*pr)(" | %5d %4u %4u 0x%8.8x %3s %4u %11p %3d "
			    "%u ST=%d RT=%d %d\n",
			    (int)l->l_lid, l->l_priority, l->l_usrpri,
			    l->l_flag, l->l_stat == LSRUN ? "RQ" :
			    (l->l_stat == LSSLEEP ? "SQ" : "-"),
			    sil->sl_timeslice, l, ci->ci_cpuid,
			    (u_int)(hardclock_ticks - sil->sl_lrtime),
			    sil->sl_slpsum, sil->sl_rtsum, sil->sl_flags);
		}
	}
}

#endif /* defined(DDB) */
