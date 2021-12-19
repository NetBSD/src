/*	$NetBSD: linux_rcu.c,v 1.7 2021/12/19 01:19:52 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_rcu.c,v 1.7 2021/12/19 01:19:52 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/cpu.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/sdt.h>
#include <sys/xcall.h>

#include <linux/rcupdate.h>

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

static struct {
	kmutex_t	lock;
	kcondvar_t	cv;
	struct rcu_head	*first;
	struct lwp	*lwp;
	uint64_t	gen;
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
 */
void
synchronize_rcu(void)
{

	SDT_PROBE0(sdt, linux, rcu, synchronize__start);
	xc_wait(xc_broadcast(0, &synchronize_rcu_xc, NULL, NULL));
	SDT_PROBE0(sdt, linux, rcu, synchronize__done);
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

	SDT_PROBE0(sdt, linux, rcu, barrier__start);
	mutex_enter(&gc.lock);
	if (gc.first != NULL) {
		gen = gc.gen;
		do {
			cv_wait(&gc.cv, &gc.lock);
		} while (gc.gen == gen);
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

	head->rcuh_callback = callback;

	mutex_enter(&gc.lock);
	head->rcuh_next = gc.first;
	gc.first = head;
	cv_broadcast(&gc.cv);
	SDT_PROBE2(sdt, linux, rcu, call__queue,  head, callback);
	mutex_exit(&gc.lock);
}

static void
gc_thread(void *cookie)
{
	struct rcu_head *head, *next;
	void (*callback)(struct rcu_head *);

	mutex_enter(&gc.lock);
	for (;;) {
		/* Wait for a task or death notice.  */
		while ((head = gc.first) == NULL && !gc.dying)
			cv_wait(&gc.cv, &gc.lock);

		/* If we got a list of callbacks, run them.  */
		if (head != NULL) {
			gc.first = NULL;	/* mine */
			mutex_exit(&gc.lock);

			/* Wait for activity on all CPUs.  */
			synchronize_rcu();

			/* It is now safe to call the callbacks.  */
			for (; head != NULL; head = next) {
				next = head->rcuh_next;
				callback = head->rcuh_callback;
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

			mutex_enter(&gc.lock);
			gc.gen++;		/* done running */
			cv_broadcast(&gc.cv);	/* notify rcu_barrier */

			/*
			 * Go back to the top and get more work before
			 * deciding whether to stop so that we
			 * guarantee to run all callbacks.
			 */
			continue;
		}

		/* If we're asked to close shop, do so.  */
		if (gc.dying)
			break;
	}
	KASSERT(gc.first == NULL);
	mutex_exit(&gc.lock);

	kthread_exit(0);
}

int
linux_rcu_gc_init(void)
{
	int error;

	mutex_init(&gc.lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&gc.cv, "lnxrcugc");
	gc.first = NULL;
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
	KASSERT(gc.first == NULL);
	cv_destroy(&gc.cv);
	mutex_destroy(&gc.lock);
}
