/*	$NetBSD: linux_srcu.c,v 1.3 2021/12/19 11:20:33 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: linux_srcu.c,v 1.3 2021/12/19 11:20:33 riastradh Exp $");

/*
 * SRCU: Sleepable RCU
 *
 *	(This is not exactly SRCU as Linux implements it; it is my
 *	approximation of the semantics I think we need.)
 *
 *	For each srcu context, representing a related set of read
 *	sections, on each CPU we store two counts of numbers of
 *	readers in two epochs: active readers and draining readers.
 *
 *	All new srcu read sections get counted in the active epoch.
 *	When there's no synchronize_srcu in progress, the draining
 *	epoch has zero readers.  When a thread calls synchronize_srcu,
 *	which must be serialized by the caller, it it swaps the sense
 *	of the epochs, issues an xcall to collect a global count of the
 *	number of readers in the now-draining epoch, and waits for the
 *	remainder to complete.
 *
 *	This is basically NetBSD localcount(9), but without the
 *	restriction that the caller of localcount_drain must guarantee
 *	no new readers -- srcu uses two counts per CPU instead of one
 *	like localcount(9), and synchronize_srcu just waits for all
 *	existing readers to drain while new oness count toward a new
 *	epoch.
 */

#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/percpu.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/xcall.h>

#include <linux/srcu.h>

struct srcu_cpu {
	int64_t	src_count[2];
};

/*
 * srcu_init(srcu, name)
 *
 *	Initialize the srcu state with the specified name.  Caller must
 *	call srcu_fini when done.
 *
 *	name should be no longer than 8 characters; longer will be
 *	truncated.
 *
 *	May sleep.
 */
void
srcu_init(struct srcu_struct *srcu, const char *name)
{

	ASSERT_SLEEPABLE();

	srcu->srcu_percpu = percpu_alloc(sizeof(struct srcu_cpu));
	mutex_init(&srcu->srcu_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&srcu->srcu_cv, name);
	srcu->srcu_sync = NULL;
	srcu->srcu_total = 0;
	srcu->srcu_gen = 0;
}

/*
 * srcu_fini(srcu)
 *
 *	Finalize an srcu state, which must not be in use right now.  If
 *	any srcu read sections might be active, caller must wait for
 *	them to complete with synchronize_srcu.
 *
 *	May sleep.
 */
void
srcu_fini(struct srcu_struct *srcu)
{

	ASSERT_SLEEPABLE();

	KASSERTMSG((srcu->srcu_sync == NULL),
	    "srcu_fini in lwp %p while synchronize_srcu running in lwp %p",
	    curlwp, srcu->srcu_sync);
	cv_destroy(&srcu->srcu_cv);
	mutex_destroy(&srcu->srcu_lock);
	percpu_free(srcu->srcu_percpu, sizeof(struct srcu_cpu));
}

/*
 * srcu_adjust(srcu, gen, delta)
 *
 *	Internal subroutine: Add delta to the local CPU's count of
 *	readers in the generation gen.
 *
 *	Never sleeps.
 */
static void
srcu_adjust(struct srcu_struct *srcu, unsigned gen, int delta)
{
	struct srcu_cpu *cpu;
	unsigned epoch = gen & 1; /* active epoch */

	cpu = percpu_getref(srcu->srcu_percpu);
	cpu->src_count[epoch] += delta;
	percpu_putref(srcu->srcu_percpu);
}

/*
 * srcu_read_lock(srcu)
 *
 *	Enter an srcu read section and return a ticket for it.  Any
 *	subsequent synchronize_srcu will wait until this thread calls
 *	srcu_read_unlock(srcu, ticket).
 *
 *	Never sleeps.
 */
int
srcu_read_lock(struct srcu_struct *srcu)
{
	unsigned gen;

	/*
	 * Prevent xcall while we fetch the generation and adjust the
	 * count.
	 */
	kpreempt_disable();
	gen = srcu->srcu_gen;
	srcu_adjust(srcu, gen, +1);
	kpreempt_enable();

	/*
	 * No stronger, inter-CPU memory barrier is needed: if there is
	 * a concurrent synchronize_srcu, it will issue an xcall that
	 * functions as a stronger memory barrier.
	 */

	return gen;
}

/*
 * srcu_read_unlock(srcu, ticket)
 *
 *	Exit an srcu read section started with srcu_read_lock returning
 *	ticket.  If there is a pending synchronize_srcu and we might be
 *	the last reader, notify it.
 *
 *	Never sleeps.
 */
void
srcu_read_unlock(struct srcu_struct *srcu, int ticket)
{
	unsigned gen = ticket;

	/*
	 * All side effects have completed on this CPU before we
	 * disable kpreemption.
	 *
	 * No stronger, inter-CPU memory barrier is needed: if there is
	 * a concurrent synchronize_srcu, it will issue an xcall that
	 * functions as a stronger memory barrier.
	 */

	/*
	 * Prevent xcall while we determine whether we need to notify a
	 * sync and decrement the count in our generation.
	 */
	kpreempt_disable();
	if (__predict_true(gen == srcu->srcu_gen)) {
		/*
		 * Fast path: just decrement the local count.  If a
		 * sync has begun and incremented gen after we observed
		 * it, it will issue an xcall that will run after this
		 * kpreempt_disable section to collect our local count.
		 */
		srcu_adjust(srcu, gen, -1);
	} else {
		/*
		 * Slow path: decrement the total count, and if it goes
		 * to zero, notify the sync in progress.  The xcall may
		 * have already run, or it may have yet to run; since
		 * we can't tell which, we must contribute to the
		 * global count, not to our local count.
		 */
		mutex_spin_enter(&srcu->srcu_lock);
		KASSERT(srcu->srcu_sync != NULL);
		if (--srcu->srcu_total == 0)
			cv_broadcast(&srcu->srcu_cv);
		mutex_spin_exit(&srcu->srcu_lock);
	}
	kpreempt_enable();
}

/*
 * synchronize_srcu_xc(a, b)
 *
 *	Cross-call function for synchronize_srcu: a is the struct srcu_struct
 *	pointer; b is ignored.  Transfer the local count of srcu
 *	readers on this CPU in the inactive epoch to the global count
 *	under the srcu sync lock.
 */
static void
synchronize_srcu_xc(void *a, void *b)
{
	struct srcu_struct *srcu = a;
	struct srcu_cpu *cpu;
	unsigned gen, epoch;
	uint64_t local;

	/* Operate under the sync lock.  Blocks preemption as side effect.  */
	mutex_spin_enter(&srcu->srcu_lock);

	gen = srcu->srcu_gen;	/* active generation */
	epoch = 1 ^ (gen & 1);	/* draining epoch */

	/* Transfer the local count to the global count.  */
	cpu = percpu_getref(srcu->srcu_percpu);
	local = cpu->src_count[epoch];
	srcu->srcu_total += local;
	cpu->src_count[epoch] -= local; /* i.e., cpu->src_count[epoch] = 0 */
	KASSERT(cpu->src_count[epoch] == 0);
	percpu_putref(srcu->srcu_percpu);

	mutex_spin_exit(&srcu->srcu_lock);
}

/*
 * synchronize_srcu(srcu)
 *
 *	Wait for all srcu readers on all CPUs that may have begun
 *	before sychronize_srcu to complete.
 *
 *	May sleep.  (Practically guaranteed to sleep!)
 */
void
synchronize_srcu(struct srcu_struct *srcu)
{

	ASSERT_SLEEPABLE();

	/* Start a sync, and advance the active generation.  */
	mutex_spin_enter(&srcu->srcu_lock);
	while (srcu->srcu_sync != NULL)
		cv_wait(&srcu->srcu_cv, &srcu->srcu_lock);
	KASSERT(srcu->srcu_total == 0);
	srcu->srcu_sync = curlwp;
	srcu->srcu_gen++;
	mutex_spin_exit(&srcu->srcu_lock);

	/*
	 * Wait for all CPUs to witness the change to the active
	 * generation, and collect their local counts in the draining
	 * epoch into the global count.
	 */
	xc_wait(xc_broadcast(0, synchronize_srcu_xc, srcu, NULL));

	/*
	 * Wait for the global count of users in the draining epoch to
	 * drain to zero.
	 */
	mutex_spin_enter(&srcu->srcu_lock);
	while (srcu->srcu_total != 0)
		cv_wait(&srcu->srcu_cv, &srcu->srcu_lock);
	srcu->srcu_sync = NULL;
	cv_broadcast(&srcu->srcu_cv);
	mutex_spin_exit(&srcu->srcu_lock);
}
