/*	$NetBSD: subr_localcount.c,v 1.7 2017/11/17 09:26:36 ozaki-r Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
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

/*
 * CPU-local reference counts
 *
 *	localcount(9) is a reference-counting scheme that involves no
 *	interprocessor synchronization most of the time, at the cost of
 *	eight bytes of memory per CPU per object and at the cost of
 *	expensive interprocessor synchronization to drain references.
 *
 *	localcount(9) references may be held across sleeps, may be
 *	transferred from CPU to CPU or thread to thread: they behave
 *	semantically like typical reference counts, with different
 *	pragmatic performance characteristics.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_localcount.c,v 1.7 2017/11/17 09:26:36 ozaki-r Exp $");

#include <sys/param.h>
#include <sys/localcount.h>
#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/percpu.h>
#include <sys/xcall.h>
#if defined(DEBUG) && defined(LOCKDEBUG)
#include <sys/atomic.h>
#endif

static void localcount_xc(void *, void *);

/*
 * localcount_init(lc)
 *
 *	Initialize a localcount object.  Returns 0 on success, error
 *	code on failure.  May fail to allocate memory for percpu(9).
 *
 *	The caller must call localcount_drain and then localcount_fini
 *	when done with lc.
 */
void
localcount_init(struct localcount *lc)
{

	lc->lc_totalp = NULL;
	lc->lc_percpu = percpu_alloc(sizeof(int64_t));
}

/*
 * localcount_drain(lc, cv, interlock)
 *
 *	Wait for all acquired references to lc to drain.  Caller must
 *	hold interlock; localcount_drain releases it during cross-calls
 *	and waits on cv.  The cv and interlock passed here must be the
 *	same as are passed to localcount_release for this lc.
 *
 *	Caller must guarantee that no new references can be acquired
 *	with localcount_acquire before calling localcount_drain.  For
 *	example, any object that may be found in a list and acquired
 *	must be removed from the list before localcount_drain.
 *
 *	The localcount object lc may be used only with localcount_fini
 *	after this, unless reinitialized after localcount_fini with
 *	localcount_init.
 */
void
localcount_drain(struct localcount *lc, kcondvar_t *cv, kmutex_t *interlock)
{
	int64_t total = 0;

	KASSERT(mutex_owned(interlock));
	KASSERT(lc->lc_totalp == NULL);

	/* Mark it draining.  */
	lc->lc_totalp = &total;

	/*
	 * Count up all references on all CPUs.
	 *
	 * This serves as a global memory barrier: after xc_wait, all
	 * CPUs will have witnessed the nonnull value of lc->lc_totalp,
	 * so that it is safe to wait on the cv for them.
	 */
	mutex_exit(interlock);
	xc_wait(xc_broadcast(0, &localcount_xc, lc, interlock));
	mutex_enter(interlock);

	/* Wait for remaining references to drain.  */
	while (total != 0) {
		/*
		 * At this point, now that we have added up all
		 * references on all CPUs, the total had better be
		 * nonnegative.
		 */
		KASSERTMSG((0 < total),
		    "negatively referenced localcount: %p, %"PRId64,
		    lc, total);
		cv_wait(cv, interlock);
	}

	/* Paranoia: Cause any further use of lc->lc_totalp to crash.  */
	lc->lc_totalp = (void *)(uintptr_t)1;
}

/*
 * localcount_fini(lc)
 *
 *	Finalize a localcount object, releasing any memory allocated
 *	for it.  The localcount object must already have been drained.
 */
void
localcount_fini(struct localcount *lc)
{

	KASSERT(lc->lc_totalp == (void *)(uintptr_t)1);
	percpu_free(lc->lc_percpu, sizeof(uint64_t));
}

/*
 * localcount_xc(cookie0, cookie1)
 *
 *	Accumulate and transfer the per-CPU reference counts to a
 *	global total, resetting the per-CPU counter to zero.  Once
 *	localcount_drain() has started, we only maintain the total
 *	count in localcount_release().
 */
static void
localcount_xc(void *cookie0, void *cookie1)
{
	struct localcount *lc = cookie0;
	kmutex_t *interlock = cookie1;
	int64_t *localp;

	mutex_enter(interlock);
	localp = percpu_getref(lc->lc_percpu);
	*lc->lc_totalp += *localp;
	*localp -= *localp;		/* ie, *localp = 0; */
	percpu_putref(lc->lc_percpu);
	mutex_exit(interlock);
}

/*
 * localcount_adjust(lc, delta)
 *
 *	Add delta -- positive or negative -- to the local CPU's count
 *	for lc.
 */
static void
localcount_adjust(struct localcount *lc, int delta)
{
	int64_t *localp;

	localp = percpu_getref(lc->lc_percpu);
	*localp += delta;
	percpu_putref(lc->lc_percpu);
}

/*
 * localcount_acquire(lc)
 *
 *	Acquire a reference to lc.
 *
 *	The reference may be held across sleeps and may be migrated
 *	from CPU to CPU, or even thread to thread -- it is only
 *	counted, not associated with a particular concrete owner.
 *
 *	Involves no interprocessor synchronization.  May be used in any
 *	context: while a lock is held, within a pserialize(9) read
 *	section, in hard interrupt context (provided other users block
 *	hard interrupts), in soft interrupt context, in thread context,
 *	&c.
 *
 *	Caller must guarantee that there is no concurrent
 *	localcount_drain.  For example, any object that may be found in
 *	a list and acquired must be removed from the list before
 *	localcount_drain.
 */
void
localcount_acquire(struct localcount *lc)
{

	KASSERT(lc->lc_totalp == NULL);
	localcount_adjust(lc, +1);
#if defined(DEBUG) && defined(LOCKDEBUG)
	if (atomic_inc_32_nv(&lc->lc_refcnt) == 0)
		panic("counter overflow");
#endif
}

/*
 * localcount_release(lc, cv, interlock)
 *
 *	Release a reference to lc.  If there is a concurrent
 *	localcount_drain and this may be the last reference, notify
 *	localcount_drain by acquiring interlock, waking cv, and
 *	releasing interlock.  The cv and interlock passed here must be
 *	the same as are passed to localcount_drain for this lc.
 *
 *	Involves no interprocessor synchronization unless there is a
 *	concurrent localcount_drain in progress.
 */
void
localcount_release(struct localcount *lc, kcondvar_t *cv, kmutex_t *interlock)
{

	/*
	 * Block xcall so that if someone begins draining after we see
	 * lc->lc_totalp as null, then they won't start cv_wait until
	 * after they have counted this CPU's contributions.
	 *
	 * Otherwise, localcount_drain may notice an extant reference
	 * from this CPU and cv_wait for it, but having seen
	 * lc->lc_totalp as null, this CPU will not wake
	 * localcount_drain.
	 */
	kpreempt_disable();

	KDASSERT(mutex_ownable(interlock));
	if (__predict_false(lc->lc_totalp != NULL)) {
		/*
		 * Slow path -- wake localcount_drain in case this is
		 * the last reference.
		 */
		mutex_enter(interlock);
		if (--*lc->lc_totalp == 0)
			cv_broadcast(cv);
		mutex_exit(interlock);
		goto out;
	}

	localcount_adjust(lc, -1);
#if defined(DEBUG) && defined(LOCKDEBUG)
	if (atomic_dec_32_nv(&lc->lc_refcnt) == UINT_MAX)
		panic("counter underflow");
#endif
 out:	kpreempt_enable();
}

/*
 * localcount_debug_refcnt(lc)
 *
 *	Return a total reference count of lc.  It returns a correct value
 *	only if DEBUG and LOCKDEBUG enabled.  Otherwise always return 0.
 */
uint32_t
localcount_debug_refcnt(const struct localcount *lc)
{

#if defined(DEBUG) && defined(LOCKDEBUG)
	return lc->lc_refcnt;
#else
	return 0;
#endif
}
