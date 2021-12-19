/*	$NetBSD: linux_rcu.c,v 1.7 2021/12/19 12:40:11 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_rcu.c,v 1.7 2021/12/19 12:40:11 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/condvar.h>
#include <sys/cpu.h>
#include <sys/kthread.h>
#include <sys/lockdebug.h>
#include <sys/mutex.h>
#include <sys/sdt.h>
#include <sys/xcall.h>

#include <linux/rcupdate.h>
#include <linux/slab.h>

SDT_PROBE_DEFINE0(sdt, linux, rcu, synchronize__start);
SDT_PROBE_DEFINE1(sdt, linux, rcu, synchronize__cpu, "unsigned"/*cpu*/);
SDT_PROBE_DEFINE0(sdt, linux, rcu, synchronize__done);
SDT_PROBE_DEFINE0(sdt, linux, rcu, barrier__start);
SDT_PROBE_DEFINE0(sdt, linux, rcu, barrier__done);
SDT_PROBE_DEFINE2(sdt, linux, rcu, call__queue,
    "struct rcu_head *"/*head*/, "void (*)(struct rcu_head *)"/*callback*/);
SDT_PROBE_DEFINE2(sdt, linux, rcu, call__run,
    "struct rcu_head *"/*head*/, "void (*)(struct rcu_head *)"/*callback*/);
SDT_PROBE_DEFINE2(sdt, linux, rcu, call__done,
    "struct rcu_head *"/*head*/, "void (*)(struct rcu_head *)"/*callback*/);
SDT_PROBE_DEFINE2(sdt, linux, rcu, kfree__queue,
    "struct rcu_head *"/*head*/, "void *"/*obj*/);
SDT_PROBE_DEFINE2(sdt, linux, rcu, kfree__free,
    "struct rcu_head *"/*head*/, "void *"/*obj*/);
SDT_PROBE_DEFINE2(sdt, linux, rcu, kfree__done,
    "struct rcu_head *"/*head*/, "void *"/*obj*/);

static struct {
	kmutex_t	lock;
	kcondvar_t	cv;
	struct rcu_head	*first_callback;
	struct rcu_head	*first_kfree;
	struct lwp	*lwp;
	uint64_t	gen;
	bool		running;
	bool		dying;
} gc __cacheline_aligned;

static void
synchronize_rcu_xc(void *a, void *b)
{

	SDT_PROBE1(sdt, linux, rcu, synchronize__cpu,  cpu_index(curcpu()));
}

/*
 * synchronize_rcu()
 *
 *	Wait for any pending RCU read section on every CPU to complete
 *	by triggering on every CPU activity that is blocked by an RCU
 *	read section.
 *
 *	May sleep.  (Practically guaranteed to sleep!)
 */
void
synchronize_rcu(void)
{

	SDT_PROBE0(sdt, linux, rcu, synchronize__start);
	xc_wait(xc_broadcast(0, &synchronize_rcu_xc, NULL, NULL));
	SDT_PROBE0(sdt, linux, rcu, synchronize__done);
}

/*
 * synchronize_rcu_expedited()
 *
 *	Wait for any pending RCU read section on every CPU to complete
 *	by triggering on every CPU activity that is blocked by an RCU
 *	read section.  Try to get an answer faster than
 *	synchronize_rcu, at the cost of more activity triggered on
 *	other CPUs.
 *
 *	May sleep.  (Practically guaranteed to sleep!)
 */
void
synchronize_rcu_expedited(void)
{

	synchronize_rcu();
}

/*
 * cookie = get_state_synchronize_rcu(), cond_synchronize_rcu(cookie)
 *
 *	Optimization for synchronize_rcu -- skip if it has already
 *	happened between get_state_synchronize_rcu and
 *	cond_synchronize_rcu.  get_state_synchronize_rcu implies a full
 *	SMP memory barrier (membar_sync).
 */
unsigned long
get_state_synchronize_rcu(void)
{

	membar_sync();
	return 0;
}

void
cond_synchronize_rcu(unsigned long cookie)
{

	synchronize_rcu();
}

/*
 * rcu_barrier()
 *
 *	Wait for all pending RCU callbacks to complete.
 *
 *	Does not imply, and is not implied by, synchronize_rcu.
 */
void
rcu_barrier(void)
{
	uint64_t gen;

	/*
	 * If the GC isn't running anything yet, then all callbacks of
	 * interest are queued, and it suffices to wait for the GC to
	 * advance one generation number.
	 *
	 * If the GC is already running, however, and there are any
	 * callbacks of interest queued but not in the GC's current
	 * batch of work, then when the advances the generation number
	 * it will not have completed the queued callbacks.  So we have
	 * to wait for one more generation -- or until the GC has
	 * stopped running because there's no work left.
	 */

	SDT_PROBE0(sdt, linux, rcu, barrier__start);
	mutex_enter(&gc.lock);
	gen = gc.gen;
	if (gc.running)
		gen++;
	while (gc.running || gc.first_callback || gc.first_kfree) {
		cv_wait(&gc.cv, &gc.lock);
		if (gc.gen > gen)
			break;
	}
	mutex_exit(&gc.lock);
	SDT_PROBE0(sdt, linux, rcu, barrier__done);
}

/*
 * call_rcu(head, callback)
 *
 *	Arrange to call callback(head) after any pending RCU read
 *	sections on every CPU is complete.  Return immediately.
 */
void
call_rcu(struct rcu_head *head, void (*callback)(struct rcu_head *))
{

	head->rcuh_u.callback = callback;

	mutex_enter(&gc.lock);
	head->rcuh_next = gc.first_callback;
	gc.first_callback = head;
	cv_broadcast(&gc.cv);
	SDT_PROBE2(sdt, linux, rcu, call__queue,  head, callback);
	mutex_exit(&gc.lock);
}

/*
 * _kfree_rcu(head, obj)
 *
 *	kfree_rcu helper: schedule kfree(obj) using head for storage.
 */
void
_kfree_rcu(struct rcu_head *head, void *obj)
{

	LOCKDEBUG_MEM_CHECK(obj, ((struct linux_malloc *)obj - 1)->lm_size);

	head->rcuh_u.obj = obj;

	mutex_enter(&gc.lock);
	head->rcuh_next = gc.first_kfree;
	gc.first_kfree = head;
	cv_broadcast(&gc.cv);
	SDT_PROBE2(sdt, linux, rcu, kfree__queue,  head, obj);
	mutex_exit(&gc.lock);
}

static void
gc_thread(void *cookie)
{
	struct rcu_head *head_callback, *head_kfree, *head, *next;

	mutex_enter(&gc.lock);
	for (;;) {
		/* Start with no work.  */
		bool work = false;

		/* Grab the list of callbacks.  */
		if ((head_callback = gc.first_callback) != NULL) {
			gc.first_callback = NULL;
			work = true;
		}

		/* Grab the list of objects to kfree.  */
		if ((head_kfree = gc.first_kfree) != NULL) {
			gc.first_kfree = NULL;
			work = true;
		}

		/*
		 * If no work, then either stop, if we're dying, or
		 * wait for work, if not.
		 */
		if (!work) {
			if (gc.dying)
				break;
			cv_wait(&gc.cv, &gc.lock);
			continue;
		}

		/*
		 * We have work to do.  Drop the lock to do it, and
		 * notify rcu_barrier that we're still doing it.
		 */
		gc.running = true;
		mutex_exit(&gc.lock);

		/* Wait for activity on all CPUs.  */
		synchronize_rcu();

		/* Call the callbacks.  */
		for (head = head_callback; head != NULL; head = next) {
			void (*callback)(struct rcu_head *) =
			    head->rcuh_u.callback;
			next = head->rcuh_next;
			SDT_PROBE2(sdt, linux, rcu, call__run,
			    head, callback);
			(*callback)(head);
			/*
			 * Can't dereference head or invoke
			 * callback after this point.
			 */
			SDT_PROBE2(sdt, linux, rcu, call__done,
			    head, callback);
		}

		/* Free the objects to kfree.  */
		for (head = head_kfree; head != NULL; head = next) {
			void *obj = head->rcuh_u.obj;
			next = head->rcuh_next;
			SDT_PROBE2(sdt, linux, rcu, kfree__free,  head, obj);
			kfree(obj);
			/* Can't dereference head or obj after this point.  */
			SDT_PROBE2(sdt, linux, rcu, kfree__done,  head, obj);
		}

		/* Return to the lock.  */
		mutex_enter(&gc.lock);

		/* Finished a batch of work.  Notify rcu_barrier.  */
		gc.gen++;
		gc.running = false;
		cv_broadcast(&gc.cv);

		/*
		 * Limit ourselves to one batch per tick, in an attempt
		 * to make the batches larger.
		 *
		 * XXX We should maybe also limit the size of each
		 * batch.
		 */
		(void)kpause("lxrcubat", /*intr*/false, /*timo*/1, &gc.lock);
	}
	KASSERT(gc.first_callback == NULL);
	KASSERT(gc.first_kfree == NULL);
	mutex_exit(&gc.lock);

	kthread_exit(0);
}

void
init_rcu_head(struct rcu_head *head)
{
}

void
destroy_rcu_head(struct rcu_head *head)
{
}

int
linux_rcu_gc_init(void)
{
	int error;

	mutex_init(&gc.lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&gc.cv, "lnxrcugc");
	gc.first_callback = NULL;
	gc.first_kfree = NULL;
	gc.gen = 0;
	gc.dying = false;

	error = kthread_create(PRI_NONE,
	    KTHREAD_MPSAFE|KTHREAD_TS|KTHREAD_MUSTJOIN, NULL, &gc_thread, NULL,
	    &gc.lwp, "lnxrcugc");
	if (error)
		goto fail;

	/* Success!  */
	return 0;

fail:	cv_destroy(&gc.cv);
	mutex_destroy(&gc.lock);
	return error;
}

void
linux_rcu_gc_fini(void)
{

	mutex_enter(&gc.lock);
	gc.dying = true;
	cv_broadcast(&gc.cv);
	mutex_exit(&gc.lock);

	kthread_join(gc.lwp);
	gc.lwp = NULL;
	KASSERT(gc.first_callback == NULL);
	KASSERT(gc.first_kfree == NULL);
	cv_destroy(&gc.cv);
	mutex_destroy(&gc.lock);
}
