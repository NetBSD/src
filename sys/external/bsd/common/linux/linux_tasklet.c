/*	$NetBSD: linux_tasklet.c,v 1.11 2022/04/09 23:43:31 riastradh Exp $	*/

/*-
 * Copyright (c) 2018, 2020, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_tasklet.c,v 1.11 2022/04/09 23:43:31 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/errno.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/lock.h>
#include <sys/percpu.h>
#include <sys/queue.h>

#include <lib/libkern/libkern.h>

#include <machine/limits.h>

#include <linux/tasklet.h>

#define	TASKLET_SCHEDULED	((unsigned)__BIT(0))
#define	TASKLET_RUNNING		((unsigned)__BIT(1))

struct tasklet_queue {
	struct percpu	*tq_percpu;	/* struct tasklet_cpu * */
	void		*tq_sih;
};

SIMPLEQ_HEAD(tasklet_head, tasklet_struct);

struct tasklet_cpu {
	struct tasklet_head	tc_head;
};

static struct tasklet_queue	tasklet_queue __read_mostly;
static struct tasklet_queue	tasklet_hi_queue __read_mostly;

static void	tasklet_softintr(void *);
static int	tasklet_queue_init(struct tasklet_queue *, unsigned);
static void	tasklet_queue_fini(struct tasklet_queue *);
static void	tasklet_queue_schedule(struct tasklet_queue *,
		    struct tasklet_struct *);
static void	tasklet_queue_enqueue(struct tasklet_queue *,
		    struct tasklet_struct *);

/*
 * linux_tasklets_init()
 *
 *	Initialize the Linux tasklets subsystem.  Return 0 on success,
 *	error code on failure.
 */
int
linux_tasklets_init(void)
{
	int error;

	error = tasklet_queue_init(&tasklet_queue, SOFTINT_CLOCK);
	if (error)
		goto fail0;
	error = tasklet_queue_init(&tasklet_hi_queue, SOFTINT_SERIAL);
	if (error)
		goto fail1;

	/* Success!  */
	return 0;

fail2: __unused
	tasklet_queue_fini(&tasklet_hi_queue);
fail1:	tasklet_queue_fini(&tasklet_queue);
fail0:	KASSERT(error);
	return error;
}

/*
 * linux_tasklets_fini()
 *
 *	Finalize the Linux tasklets subsystem.  All use of tasklets
 *	must be done.
 */
void
linux_tasklets_fini(void)
{

	tasklet_queue_fini(&tasklet_hi_queue);
	tasklet_queue_fini(&tasklet_queue);
}

static void
tasklet_cpu_init(void *ptr, void *cookie, struct cpu_info *ci)
{
	struct tasklet_cpu **tcp = ptr, *tc;

	*tcp = tc = kmem_zalloc(sizeof(*tc), KM_SLEEP);
	SIMPLEQ_INIT(&tc->tc_head);
}

static void
tasklet_cpu_fini(void *ptr, void *cookie, struct cpu_info *ci)
{
	struct tasklet_cpu **tcp = ptr, *tc = *tcp;

	KASSERT(SIMPLEQ_EMPTY(&tc->tc_head));
	kmem_free(tc, sizeof(*tc));
	*tcp = NULL;		/* paranoia */
}

/*
 * tasklet_queue_init(tq, prio)
 *
 *	Initialize the tasklet queue tq for running tasklets at softint
 *	priority prio (SOFTINT_*).
 */
static int
tasklet_queue_init(struct tasklet_queue *tq, unsigned prio)
{
	int error;

	/* Allocate per-CPU memory.  percpu_alloc cannot fail.  */
	tq->tq_percpu = percpu_create(sizeof(struct tasklet_cpu),
	    tasklet_cpu_init, tasklet_cpu_fini, NULL);
	KASSERT(tq->tq_percpu != NULL);

	/* Try to establish a softint.  softint_establish may fail.  */
	tq->tq_sih = softint_establish(prio|SOFTINT_MPSAFE, &tasklet_softintr,
	    tq);
	if (tq->tq_sih == NULL) {
		error = ENOMEM;
		goto fail1;
	}

	/* Success!  */
	return 0;

fail2: __unused
	softint_disestablish(tq->tq_sih);
	tq->tq_sih = NULL;
fail1:	percpu_free(tq->tq_percpu, sizeof(struct tasklet_cpu));
	tq->tq_percpu = NULL;
fail0: __unused
	KASSERT(error);
	return error;
}

/*
 * tasklet_queue_fini(tq)
 *
 *	Finalize the tasklet queue tq: free all resources associated
 *	with it.
 */
static void
tasklet_queue_fini(struct tasklet_queue *tq)
{

	softint_disestablish(tq->tq_sih);
	tq->tq_sih = NULL;
	percpu_free(tq->tq_percpu, sizeof(struct tasklet_cpu));
	tq->tq_percpu = NULL;
}

/*
 * tasklet_softintr(cookie)
 *
 *	Soft interrupt handler: Process queued tasklets on the tasklet
 *	queue passed in as cookie.
 */
static void
tasklet_softintr(void *cookie)
{
	struct tasklet_queue *const tq = cookie;
	struct tasklet_head th = SIMPLEQ_HEAD_INITIALIZER(th);
	struct tasklet_cpu **tcp, *tc;
	int s;

	/*
	 * With all interrupts deferred, transfer the current CPU's
	 * queue of tasklets to a local variable in one swell foop.
	 *
	 * No memory barriers: CPU-local state only.
	 */
	tcp = percpu_getref(tq->tq_percpu);
	tc = *tcp;
	s = splhigh();
	SIMPLEQ_CONCAT(&th, &tc->tc_head);
	splx(s);
	percpu_putref(tq->tq_percpu);

	/* Go through the queue of tasklets we grabbed.  */
	while (!SIMPLEQ_EMPTY(&th)) {
		struct tasklet_struct *tasklet;

		/* Remove the first tasklet from the queue.  */
		tasklet = SIMPLEQ_FIRST(&th);
		SIMPLEQ_REMOVE_HEAD(&th, tl_entry);

		KASSERT(atomic_load_relaxed(&tasklet->tl_state) &
		    TASKLET_SCHEDULED);

		/*
		 * Test and set RUNNING, in case it is already running
		 * on another CPU and got scheduled again on this one
		 * before it completed.
		 */
		if (!tasklet_trylock(tasklet)) {
			/*
			 * Put it back on the queue to run it again in
			 * a sort of busy-wait, and move on to the next
			 * one.
			 */
			tasklet_queue_enqueue(tq, tasklet);
			continue;
		}

		/*
		 * Check whether it's currently disabled.
		 *
		 * Pairs with membar_release in __tasklet_enable.
		 */
		if (atomic_load_acquire(&tasklet->tl_disablecount)) {
			/*
			 * Disabled: clear the RUNNING bit and, requeue
			 * it, but keep it SCHEDULED.
			 */
			tasklet_unlock(tasklet);
			tasklet_queue_enqueue(tq, tasklet);
			continue;
		}

		/* Not disabled.  Clear SCHEDULED and call func.  */
		KASSERT(atomic_load_relaxed(&tasklet->tl_state) &
		    TASKLET_SCHEDULED);
		atomic_and_uint(&tasklet->tl_state, ~TASKLET_SCHEDULED);

		(*tasklet->func)(tasklet->data);

		/* Clear RUNNING to notify tasklet_disable.  */
		tasklet_unlock(tasklet);
	}
}

/*
 * tasklet_queue_schedule(tq, tasklet)
 *
 *	Schedule tasklet to run on tq.  If it was already scheduled and
 *	has not yet run, no effect.
 */
static void
tasklet_queue_schedule(struct tasklet_queue *tq,
    struct tasklet_struct *tasklet)
{
	unsigned ostate, nstate;

	/* Test and set the SCHEDULED bit.  If already set, we're done.  */
	do {
		ostate = atomic_load_relaxed(&tasklet->tl_state);
		if (ostate & TASKLET_SCHEDULED)
			return;
		nstate = ostate | TASKLET_SCHEDULED;
	} while (atomic_cas_uint(&tasklet->tl_state, ostate, nstate)
	    != ostate);

	/*
	 * Not already set and we have set it now.  Put it on the queue
	 * and kick off a softint.
	 */
	tasklet_queue_enqueue(tq, tasklet);
}

/*
 * tasklet_queue_enqueue(tq, tasklet)
 *
 *	Put tasklet on the queue tq and ensure it will run.  tasklet
 *	must be marked SCHEDULED.
 */
static void
tasklet_queue_enqueue(struct tasklet_queue *tq, struct tasklet_struct *tasklet)
{
	struct tasklet_cpu **tcp, *tc;
	int s;

	KASSERT(atomic_load_relaxed(&tasklet->tl_state) & TASKLET_SCHEDULED);

	/*
	 * Insert on the current CPU's queue while all interrupts are
	 * blocked, and schedule a soft interrupt to process it.  No
	 * memory barriers: CPU-local state only.
	 */
	tcp = percpu_getref(tq->tq_percpu);
	tc = *tcp;
	s = splhigh();
	SIMPLEQ_INSERT_TAIL(&tc->tc_head, tasklet, tl_entry);
	splx(s);
	softint_schedule(tq->tq_sih);
	percpu_putref(tq->tq_percpu);
}

/*
 * tasklet_init(tasklet, func, data)
 *
 *	Initialize tasklet to call func(data) when scheduled.
 *
 *	Caller is responsible for issuing the appropriate memory
 *	barriers or store releases to publish the tasklet to other CPUs
 *	before use.
 */
void
tasklet_init(struct tasklet_struct *tasklet, void (*func)(unsigned long),
    unsigned long data)
{

	atomic_store_relaxed(&tasklet->tl_state, 0);
	atomic_store_relaxed(&tasklet->tl_disablecount, 0);
	tasklet->func = func;
	tasklet->data = data;
}

/*
 * tasklet_schedule(tasklet)
 *
 *	Schedule tasklet to run at regular priority.  If it was already
 *	scheduled and has not yet run, no effect.
 */
void
tasklet_schedule(struct tasklet_struct *tasklet)
{

	tasklet_queue_schedule(&tasklet_queue, tasklet);
}

/*
 * tasklet_hi_schedule(tasklet)
 *
 *	Schedule tasklet to run at high priority.  If it was already
 *	scheduled and has not yet run, no effect.
 */
void
tasklet_hi_schedule(struct tasklet_struct *tasklet)
{

	tasklet_queue_schedule(&tasklet_hi_queue, tasklet);
}

/*
 * tasklet_disable_nosync(tasklet)
 *
 *	Increment the disable count of tasklet, but don't wait for it
 *	to complete -- it may remain running after this returns.
 *
 *	As long as the disable count is nonzero, the tasklet's function
 *	will not run, but if already scheduled, the tasklet will remain
 *	so and the softint will repeatedly trigger itself in a sort of
 *	busy-wait, so this should be used only for short durations.
 *
 *	Load-acquire semantics.
 */
void
tasklet_disable_nosync(struct tasklet_struct *tasklet)
{
	unsigned int disablecount __diagused;

	/* Increment the disable count.  */
	disablecount = atomic_inc_uint_nv(&tasklet->tl_disablecount);
	KASSERT(disablecount < UINT_MAX);
	KASSERT(disablecount != 0);

	/* Pairs with membar_release in __tasklet_enable.  */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_acquire();
#endif
}

/*
 * tasklet_disable(tasklet)
 *
 *	Increment the disable count of tasklet, and if it was already
 *	running, busy-wait for it to complete.
 *
 *	As long as the disable count is nonzero, the tasklet's function
 *	will not run, but if already scheduled, the tasklet will remain
 *	so and the softint will repeatedly trigger itself in a sort of
 *	busy-wait, so this should be used only for short durations.
 *
 *	If tasklet is guaranteed not to be scheduled, e.g. if you have
 *	just invoked tasklet_kill, then tasklet_disable serves to wait
 *	for it to complete in case it might already be running.
 *
 *	Load-acquire semantics.
 */
void
tasklet_disable(struct tasklet_struct *tasklet)
{

	/* Increment the disable count.  */
	tasklet_disable_nosync(tasklet);

	/* Wait for it to finish running, if it was running.  */
	tasklet_unlock_wait(tasklet);
}

/*
 * tasklet_enable(tasklet)
 *
 *	Decrement tasklet's disable count.  If it was previously
 *	scheduled to run, it may now run.
 *
 *	Store-release semantics.
 */
void
tasklet_enable(struct tasklet_struct *tasklet)
{

	(void)__tasklet_enable(tasklet);
}

/*
 * tasklet_kill(tasklet)
 *
 *	Busy-wait for tasklet to run, if it is currently scheduled.
 *	Caller must guarantee it does not get scheduled again for this
 *	to be useful.
 */
void
tasklet_kill(struct tasklet_struct *tasklet)
{

	KASSERTMSG(!cpu_intr_p(),
	    "deadlock: soft interrupts are blocked in interrupt context");

	/* Wait for it to be removed from the queue.  */
	while (atomic_load_relaxed(&tasklet->tl_state) & TASKLET_SCHEDULED)
		SPINLOCK_BACKOFF_HOOK;

	/*
	 * No need for a memory barrier here because writes to the
	 * single state word are globally ordered, and RUNNING is set
	 * before SCHEDULED is cleared, so as long as the caller
	 * guarantees no scheduling, the only possible transitions we
	 * can witness are:
	 *
	 *	0                 -> 0
	 *	SCHEDULED         -> 0
	 *	SCHEDULED         -> RUNNING
	 *	RUNNING           -> 0
	 *	RUNNING           -> RUNNING
	 *	SCHEDULED|RUNNING -> 0
	 *	SCHEDULED|RUNNING -> RUNNING
	 */

	/* Wait for it to finish running.  */
	tasklet_unlock_wait(tasklet);
}

/*
 * tasklet_is_locked(tasklet)
 *
 *	True if tasklet is currently locked.  Caller must use it only
 *	for positive assertions.
 */
bool
tasklet_is_locked(const struct tasklet_struct *tasklet)
{

	return atomic_load_relaxed(&tasklet->tl_state) & TASKLET_RUNNING;
}

/*
 * tasklet_trylock(tasklet)
 *
 *	Try to lock tasklet, i.e., set TASKLET_RUNNING.  Return true if
 *	we locked it, false if already locked.
 *
 *	Load-acquire semantics.
 */
bool
tasklet_trylock(struct tasklet_struct *tasklet)
{
	unsigned state;

	do {
		state = atomic_load_relaxed(&tasklet->tl_state);
		if (state & TASKLET_RUNNING)
			return false;
	} while (atomic_cas_uint(&tasklet->tl_state, state,
		state | TASKLET_RUNNING) != state);

	/* Pairs with membar_release in tasklet_unlock.  */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_acquire();
#endif

	return true;
}

/*
 * tasklet_unlock(tasklet)
 *
 *	Unlock tasklet, i.e., clear TASKLET_RUNNING.
 *
 *	Store-release semantics.
 */
void
tasklet_unlock(struct tasklet_struct *tasklet)
{

	KASSERT(atomic_load_relaxed(&tasklet->tl_state) & TASKLET_RUNNING);

	/*
	 * Pairs with membar_acquire in tasklet_trylock and with
	 * atomic_load_acquire in tasklet_unlock_wait.
	 */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_release();
#endif
	atomic_and_uint(&tasklet->tl_state, ~TASKLET_RUNNING);
}

/*
 * tasklet_unlock_wait(tasklet)
 *
 *	Busy-wait until tasklet is not running.
 *
 *	Load-acquire semantics.
 */
void
tasklet_unlock_wait(const struct tasklet_struct *tasklet)
{

	/* Pairs with membar_release in tasklet_unlock.  */
	while (atomic_load_acquire(&tasklet->tl_state) & TASKLET_RUNNING)
		SPINLOCK_BACKOFF_HOOK;
}

/*
 * BEGIN I915 HACKS
 *
 * The i915 driver abuses the tasklet abstraction like a cop abuses his
 * wife.
 */

/*
 * __tasklet_disable_sync_once(tasklet)
 *
 *	Increment the disable count of tasklet, and if this is the
 *	first time it was disabled and it was already running,
 *	busy-wait for it to complete.
 *
 *	Caller must not care about whether the tasklet is running, or
 *	about waiting for any side effects of the tasklet to complete,
 *	if this was not the first time it was disabled.
 */
void
__tasklet_disable_sync_once(struct tasklet_struct *tasklet)
{
	unsigned int disablecount;

	/* Increment the disable count.  */
	disablecount = atomic_inc_uint_nv(&tasklet->tl_disablecount);
	KASSERT(disablecount < UINT_MAX);
	KASSERT(disablecount != 0);

	/* Pairs with membar_release in __tasklet_enable_sync_once.  */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_acquire();
#endif

	/*
	 * If it was zero, wait for it to finish running.  If it was
	 * not zero, caller must not care whether it was running.
	 */
	if (disablecount == 1)
		tasklet_unlock_wait(tasklet);
}

/*
 * __tasklet_enable_sync_once(tasklet)
 *
 *	Decrement the disable count of tasklet, and if it goes to zero,
 *	kill tasklet.
 */
void
__tasklet_enable_sync_once(struct tasklet_struct *tasklet)
{
	unsigned int disablecount;

	/* Pairs with membar_acquire in __tasklet_disable_sync_once.  */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_release();
#endif

	/* Decrement the disable count.  */
	disablecount = atomic_dec_uint_nv(&tasklet->tl_disablecount);
	KASSERT(disablecount < UINT_MAX);

	/*
	 * If it became zero, kill the tasklet.  If it was not zero,
	 * caller must not care whether it was running.
	 */
	if (disablecount == 0)
		tasklet_kill(tasklet);
}

/*
 * __tasklet_is_enabled(tasklet)
 *
 *	True if tasklet is not currently disabled.  Answer may be stale
 *	as soon as it is returned -- caller must use it only as a hint,
 *	or must arrange synchronization externally.
 */
bool
__tasklet_is_enabled(const struct tasklet_struct *tasklet)
{
	unsigned int disablecount;

	disablecount = atomic_load_relaxed(&tasklet->tl_disablecount);

	return (disablecount == 0);
}

/*
 * __tasklet_is_scheduled(tasklet)
 *
 *	True if tasklet is currently scheduled.  Answer may be stale as
 *	soon as it is returned -- caller must use it only as a hint, or
 *	must arrange synchronization externally.
 */
bool
__tasklet_is_scheduled(const struct tasklet_struct *tasklet)
{

	return atomic_load_relaxed(&tasklet->tl_state) & TASKLET_SCHEDULED;
}

/*
 * __tasklet_enable(tasklet)
 *
 *	Decrement tasklet's disable count.  If it was previously
 *	scheduled to run, it may now run.  Return true if the disable
 *	count went down to zero; otherwise return false.
 *
 *	Store-release semantics.
 */
bool
__tasklet_enable(struct tasklet_struct *tasklet)
{
	unsigned int disablecount;

	/*
	 * Guarantee all caller-relevant reads or writes have completed
	 * before potentially allowing tasklet to run again by
	 * decrementing the disable count.
	 *
	 * Pairs with atomic_load_acquire in tasklet_softintr and with
	 * membar_acquire in tasklet_disable.
	 */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_release();
#endif

	/* Decrement the disable count.  */
	disablecount = atomic_dec_uint_nv(&tasklet->tl_disablecount);
	KASSERT(disablecount != UINT_MAX);

	return (disablecount == 0);
}
