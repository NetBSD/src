/*      $NetBSD: scheduler.c,v 1.19 2010/09/01 19:37:59 pooka Exp $	*/

/*
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: scheduler.c,v 1.19 2010/09/01 19:37:59 pooka Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/namei.h>
#include <sys/queue.h>
#include <sys/select.h>
#include <sys/systm.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

static struct cpu_info rump_cpus[MAXCPUS];
static struct rumpcpu {
	/* needed in fastpath */
	struct cpu_info *rcpu_ci;
	void *rcpu_prevlwp;

	/* needed in slowpath */
	struct rumpuser_mtx *rcpu_mtx;
	struct rumpuser_cv *rcpu_cv;
	int rcpu_wanted;

	/* offset 20 (P=4) or 36 (P=8) here */

	/*
	 * Some stats.  Not really that necessary, but we should
	 * have room.  Note that these overflow quite fast, so need
	 * to be collected often.
	 */
	unsigned int rcpu_fastpath;
	unsigned int rcpu_slowpath;
	unsigned int rcpu_migrated;

	/* offset 32 (P=4) or 50 (P=8) */

	int rcpu_align[0] __aligned(CACHE_LINE_SIZE);
} rcpu_storage[MAXCPUS];
struct cpu_info *rump_cpu = &rump_cpus[0];
int ncpu;

#define RCPULWP_BUSY	((void *)-1)
#define RCPULWP_WANTED	((void *)-2)

static struct rumpuser_mtx *lwp0mtx;
static struct rumpuser_cv *lwp0cv;
static unsigned nextcpu;

static bool lwp0isbusy = false;

/*
 * Keep some stats.
 *
 * Keeping track of there is not really critical for speed, unless
 * stats happen to be on a different cache line (CACHE_LINE_SIZE is
 * really just a coarse estimate), so default for the performant case
 * (i.e. no stats).
 */
#ifdef RUMPSCHED_STATS
#define SCHED_FASTPATH(rcpu) rcpu->rcpu_fastpath++;
#define SCHED_SLOWPATH(rcpu) rcpu->rcpu_slowpath++;
#define SCHED_MIGRATED(rcpu) rcpu->rcpu_migrated++;
#else
#define SCHED_FASTPATH(rcpu)
#define SCHED_SLOWPATH(rcpu)
#define SCHED_MIGRATED(rcpu)
#endif

struct cpu_info *
cpu_lookup(u_int index)
{

	return &rump_cpus[index];
}

static inline struct rumpcpu *
getnextcpu(void)
{
	unsigned newcpu;

	newcpu = atomic_inc_uint_nv(&nextcpu);
	if (__predict_false(ncpu > UINT_MAX/2))
		atomic_and_uint(&nextcpu, 0);
	newcpu = newcpu % ncpu;

	return &rcpu_storage[newcpu];
}

/* this could/should be mi_attach_cpu? */
void
rump_cpus_bootstrap(int num)
{
	struct rumpcpu *rcpu;
	struct cpu_info *ci;
	int i;

	if (num > MAXCPUS) {
		aprint_verbose("CPU limit: %d wanted, %d (MAXCPUS) available\n",
		    num, MAXCPUS);
		num = MAXCPUS;
	}

	for (i = 0; i < num; i++) {
		rcpu = &rcpu_storage[i];
		ci = &rump_cpus[i];
		ci->ci_index = i;
		rump_cpu_attach(ci);
		ncpu++;
	}
}

void
rump_scheduler_init()
{
	struct rumpcpu *rcpu;
	struct cpu_info *ci;
	int i;

	rumpuser_mutex_init(&lwp0mtx);
	rumpuser_cv_init(&lwp0cv);
	for (i = 0; i < ncpu; i++) {
		rcpu = &rcpu_storage[i];
		ci = &rump_cpus[i];
		rcpu->rcpu_ci = ci;
		ci->ci_schedstate.spc_mutex =
		    mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
		ci->ci_schedstate.spc_flags = SPCF_RUNNING;
		rcpu->rcpu_wanted = 0;
		rumpuser_cv_init(&rcpu->rcpu_cv);
		rumpuser_mutex_init(&rcpu->rcpu_mtx);
	}
}

/*
 * condvar ops using scheduler lock as the rumpuser interlock.
 */
void
rump_schedlock_cv_wait(struct rumpuser_cv *cv)
{
	struct lwp *l = curlwp;
	struct rumpcpu *rcpu = &rcpu_storage[l->l_cpu-&rump_cpus[0]];

	/* mutex will be taken and released in cpu schedule/unschedule */
	rumpuser_cv_wait(cv, rcpu->rcpu_mtx);
}

int
rump_schedlock_cv_timedwait(struct rumpuser_cv *cv, const struct timespec *ts)
{
	struct lwp *l = curlwp;
	struct rumpcpu *rcpu = &rcpu_storage[l->l_cpu-&rump_cpus[0]];

	/* mutex will be taken and released in cpu schedule/unschedule */
	return rumpuser_cv_timedwait(cv, rcpu->rcpu_mtx,
	    ts->tv_sec, ts->tv_nsec);
}

static void
lwp0busy(void)
{

	/* busy lwp0 */
	KASSERT(curlwp == NULL || curlwp->l_cpu == NULL);
	rumpuser_mutex_enter_nowrap(lwp0mtx);
	while (lwp0isbusy)
		rumpuser_cv_wait_nowrap(lwp0cv, lwp0mtx);
	lwp0isbusy = true;
	rumpuser_mutex_exit(lwp0mtx);
}

static void
lwp0rele(void)
{

	rumpuser_mutex_enter_nowrap(lwp0mtx);
	KASSERT(lwp0isbusy == true);
	lwp0isbusy = false;
	rumpuser_cv_signal(lwp0cv);
	rumpuser_mutex_exit(lwp0mtx);
}

void
rump_schedule()
{
	struct lwp *l;

	/*
	 * If there is no dedicated lwp, allocate a temp one and
	 * set it to be free'd upon unschedule().  Use lwp0 context
	 * for reserving the necessary resources.  Don't optimize
	 * for this case -- anyone who cares about performance will
	 * start a real thread.
	 */
	if (__predict_true((l = rumpuser_get_curlwp()) != NULL)) {
		rump_schedule_cpu(l);
		LWP_CACHE_CREDS(l, l->l_proc);
	} else {
		lwp0busy();

		/* schedule cpu and use lwp0 */
		rump_schedule_cpu(&lwp0);
		rumpuser_set_curlwp(&lwp0);

		/* allocate thread, switch to it, and release lwp0 */
		l = rump__lwproc_allockernlwp();
		rump_lwproc_switch(l);
		lwp0rele();

		/*
		 * mark new thread dead-on-unschedule.  this
		 * means that we'll be running with l_refcnt == 0.
		 * relax, it's fine.
		 */
		rump_lwproc_releaselwp();
	}
}

void
rump_schedule_cpu(struct lwp *l)
{

	rump_schedule_cpu_interlock(l, NULL);
}

/*
 * Schedule a CPU.  This optimizes for the case where we schedule
 * the same thread often, and we have nCPU >= nFrequently-Running-Thread
 * (where CPU is virtual rump cpu, not host CPU).
 */
void
rump_schedule_cpu_interlock(struct lwp *l, void *interlock)
{
	struct rumpcpu *rcpu;
	void *old;
	bool domigrate;
	bool bound = l->l_pflag & LP_BOUND;

	/*
	 * First, try fastpath: if we were the previous user of the
	 * CPU, everything is in order cachewise and we can just
	 * proceed to use it.
	 *
	 * If we are a different thread (i.e. CAS fails), we must go
	 * through a memory barrier to ensure we get a truthful
	 * view of the world.
	 */

	KASSERT(l->l_target_cpu != NULL);
	rcpu = &rcpu_storage[l->l_target_cpu-&rump_cpus[0]];
	if (atomic_cas_ptr(&rcpu->rcpu_prevlwp, l, RCPULWP_BUSY) == l) {
		if (__predict_true(interlock == rcpu->rcpu_mtx))
			rumpuser_mutex_exit(rcpu->rcpu_mtx);
		SCHED_FASTPATH(rcpu);
		/* jones, you're the man */
		goto fastlane;
	}

	/*
	 * Else, it's the slowpath for us.  First, determine if we
	 * can migrate.
	 */
	if (ncpu == 1)
		domigrate = false;
	else
		domigrate = true;

	/* Take lock.  This acts as a load barrier too. */
	if (__predict_true(interlock != rcpu->rcpu_mtx))
		rumpuser_mutex_enter_nowrap(rcpu->rcpu_mtx);

	for (;;) {
		SCHED_SLOWPATH(rcpu);
		old = atomic_swap_ptr(&rcpu->rcpu_prevlwp, RCPULWP_WANTED);

		/* CPU is free? */
		if (old != RCPULWP_BUSY && old != RCPULWP_WANTED) {
			if (atomic_cas_ptr(&rcpu->rcpu_prevlwp,
			    RCPULWP_WANTED, RCPULWP_BUSY) == RCPULWP_WANTED) {
				break;
			}
		}

		/*
		 * Do we want to migrate once?
		 * This may need a slightly better algorithm, or we
		 * might cache pingpong eternally for non-frequent
		 * threads.
		 */
		if (domigrate && !bound) {
			domigrate = false;
			SCHED_MIGRATED(rcpu);
			rumpuser_mutex_exit(rcpu->rcpu_mtx);
			rcpu = getnextcpu();
			rumpuser_mutex_enter_nowrap(rcpu->rcpu_mtx);
			continue;
		}

		/* Want CPU, wait until it's released an retry */
		rcpu->rcpu_wanted++;
		rumpuser_cv_wait_nowrap(rcpu->rcpu_cv, rcpu->rcpu_mtx);
		rcpu->rcpu_wanted--;
	}
	rumpuser_mutex_exit(rcpu->rcpu_mtx);

 fastlane:
	l->l_cpu = l->l_target_cpu = rcpu->rcpu_ci;
	l->l_mutex = rcpu->rcpu_ci->ci_schedstate.spc_mutex;
	l->l_ncsw++;
}

void
rump_unschedule()
{
	struct lwp *l;

	l = rumpuser_get_curlwp();
	KASSERT(l->l_mutex == l->l_cpu->ci_schedstate.spc_mutex);
	rump_unschedule_cpu(l);
	l->l_mutex = NULL;

	/*
	 * Check special conditions:
	 *  1) do we need to free the lwp which just unscheduled?
	 *     (locking order: lwp0, cpu)
	 *  2) do we want to clear curlwp for the current host thread
	 */
	if (__predict_false(l->l_flag & LW_WEXIT)) {
		lwp0busy();

		/* Now that we have lwp0, we can schedule a CPU again */
		rump_schedule_cpu(l);

		/* switch to lwp0.  this frees the old thread */
		KASSERT(l->l_flag & LW_WEXIT);
		rump_lwproc_switch(&lwp0);

		/* release lwp0 */
		rump_unschedule_cpu(&lwp0);
		lwp0.l_mutex = NULL;
		lwp0.l_pflag &= ~LP_RUNNING;
		lwp0rele();
		rumpuser_set_curlwp(NULL);

	} else if (__predict_false(l->l_flag & LW_RUMP_CLEAR)) {
		rumpuser_set_curlwp(NULL);
		l->l_flag &= ~LW_RUMP_CLEAR;
	}
}

void
rump_unschedule_cpu(struct lwp *l)
{

	rump_unschedule_cpu_interlock(l, NULL);
}

void
rump_unschedule_cpu_interlock(struct lwp *l, void *interlock)
{

	if ((l->l_pflag & LP_INTR) == 0)
		rump_softint_run(l->l_cpu);
	rump_unschedule_cpu1(l, interlock);
}

void
rump_unschedule_cpu1(struct lwp *l, void *interlock)
{
	struct rumpcpu *rcpu;
	struct cpu_info *ci;
	void *old;

	ci = l->l_cpu;
	l->l_cpu = NULL;
	rcpu = &rcpu_storage[ci-&rump_cpus[0]];

	KASSERT(rcpu->rcpu_ci == ci);

	/*
	 * Make sure all stores are seen before the CPU release.  This
	 * is relevant only in the non-fastpath scheduling case, but
	 * we don't know here if that's going to happen, so need to
	 * expect the worst.
	 */
	membar_exit();

	/* Release the CPU. */
	old = atomic_swap_ptr(&rcpu->rcpu_prevlwp, l);

	/* No waiters?  No problems.  We're outta here. */
	if (old == RCPULWP_BUSY) {
		/* Was the scheduler interlock requested? */
		if (__predict_false(interlock == rcpu->rcpu_mtx))
			rumpuser_mutex_enter_nowrap(rcpu->rcpu_mtx);
		return;
	}

	KASSERT(old == RCPULWP_WANTED);

	/*
	 * Ok, things weren't so snappy.
	 *
	 * Snailpath: take lock and signal anyone waiting for this CPU.
	 */

	rumpuser_mutex_enter_nowrap(rcpu->rcpu_mtx);
	if (rcpu->rcpu_wanted)
		rumpuser_cv_broadcast(rcpu->rcpu_cv);

	if (__predict_true(interlock != rcpu->rcpu_mtx))
		rumpuser_mutex_exit(rcpu->rcpu_mtx);
}

/* Give up and retake CPU (perhaps a different one) */
void
yield()
{
	struct lwp *l = curlwp;
	int nlocks;

	KERNEL_UNLOCK_ALL(l, &nlocks);
	rump_unschedule_cpu(l);
	rump_schedule_cpu(l);
	KERNEL_LOCK(nlocks, l);
}

void
preempt()
{

	yield();
}

bool
kpreempt(uintptr_t where)
{

	return false;
}

/*
 * There is no kernel thread preemption in rump currently.  But call
 * the implementing macros anyway in case they grow some side-effects
 * down the road.
 */
void
kpreempt_disable(void)
{

	KPREEMPT_DISABLE(curlwp);
}

void
kpreempt_enable(void)
{

	KPREEMPT_ENABLE(curlwp);
}

void
suspendsched(void)
{

	/*
	 * Could wait until everyone is out and block further entries,
	 * but skip that for now.
	 */
}

void
sched_nice(struct proc *p, int level)
{

	/* nothing to do for now */
}
