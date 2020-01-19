/*	$NetBSD: kern_rwlock.c,v 1.59.2.3 2020/01/19 21:08:29 ad Exp $	*/

/*-
 * Copyright (c) 2002, 2006, 2007, 2008, 2009, 2019, 2020
 *     The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Andrew Doran.
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
 * Kernel reader/writer lock implementation, modeled after those
 * found in Solaris, a description of which can be found in:
 *
 *	Solaris Internals: Core Kernel Architecture, Jim Mauro and
 *	    Richard McDougall.
 *
 * The NetBSD implementation is different from that described in the book,
 * in that the locks are adaptive.  Lock waiters spin wait while the lock
 * holders are on CPU (if the holds can be tracked: up to N per-thread). 
 *
 * While spin waiting, threads compete for the lock without the assistance
 * of turnstiles.  If a lock holder sleeps for any reason, the lock waiters
 * will also sleep in response and at that point turnstiles, priority
 * inheritance and strong efforts at ensuring fairness come into play.
 *
 * The adaptive behaviour is controlled by the RW_SPIN flag bit, which is
 * cleared by a lock owner that is going off the CPU, and set again by the
 * lock owner that releases the last hold on the lock.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_rwlock.c,v 1.59.2.3 2020/01/19 21:08:29 ad Exp $");

#include "opt_lockdebug.h"

#define	__RWLOCK_PRIVATE

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/rwlock.h>
#include <sys/sched.h>
#include <sys/sleepq.h>
#include <sys/systm.h>
#include <sys/lockdebug.h>
#include <sys/cpu.h>
#include <sys/atomic.h>
#include <sys/lock.h>
#include <sys/pserialize.h>

#include <dev/lockstat.h>

/*
 * LOCKDEBUG
 */

#define	RW_DEBUG_P(rw)		(((rw)->rw_owner & RW_NODEBUG) == 0)

#define	RW_WANTLOCK(rw, op) \
    LOCKDEBUG_WANTLOCK(RW_DEBUG_P(rw), (rw), \
        (uintptr_t)__builtin_return_address(0), op == RW_READER);
#define	RW_LOCKED(rw, op) \
    LOCKDEBUG_LOCKED(RW_DEBUG_P(rw), (rw), NULL, \
        (uintptr_t)__builtin_return_address(0), op == RW_READER);
#define	RW_UNLOCKED(rw, op) \
    LOCKDEBUG_UNLOCKED(RW_DEBUG_P(rw), (rw), \
        (uintptr_t)__builtin_return_address(0), op == RW_READER);

/*
 * DIAGNOSTIC
 */

#if defined(DIAGNOSTIC)
#define	RW_ASSERT(rw, cond) \
do { \
	if (__predict_false(!(cond))) \
		rw_abort(__func__, __LINE__, rw, "assertion failed: " #cond);\
} while (/* CONSTCOND */ 0)
#else
#define	RW_ASSERT(rw, cond)	/* nothing */
#endif	/* DIAGNOSTIC */

/*
 * Memory barriers.
 */
#ifdef __HAVE_ATOMIC_AS_MEMBAR
#define	RW_MEMBAR_ENTER()
#define	RW_MEMBAR_EXIT()
#define	RW_MEMBAR_PRODUCER()
#else
#define	RW_MEMBAR_ENTER()		membar_enter()
#define	RW_MEMBAR_EXIT()		membar_exit()
#define	RW_MEMBAR_PRODUCER()		membar_producer()
#endif

static void	rw_abort(const char *, size_t, krwlock_t *, const char *);
static void	rw_dump(const volatile void *, lockop_printer_t);
static lwp_t	*rw_owner(wchan_t);

lockops_t rwlock_lockops = {
	.lo_name = "Reader / writer lock",
	.lo_type = LOCKOPS_SLEEP,
	.lo_dump = rw_dump,
};

syncobj_t rw_syncobj = {
	.sobj_flag	= SOBJ_SLEEPQ_SORTED,
	.sobj_unsleep	= turnstile_unsleep,
	.sobj_changepri	= turnstile_changepri,
	.sobj_lendpri	= sleepq_lendpri,
	.sobj_owner	= rw_owner,
};

/*
 * rw_cas:
 *
 *	Do an atomic compare-and-swap on the lock word.
 */
static inline uintptr_t
rw_cas(krwlock_t *rw, uintptr_t o, uintptr_t n)
{

	return (uintptr_t)atomic_cas_ptr((volatile void *)&rw->rw_owner,
	    (void *)o, (void *)n);
}

/*
 * rw_and:
 *
 *	Do an atomic AND on the lock word.
 */
static inline void
rw_and(krwlock_t *rw, uintptr_t m)
{

#ifdef _LP64
	atomic_and_64(&rw->rw_owner, m);
#else
	atomic_and_32(&rw->rw_owner, m);
#endif
}

/*
 * rw_swap:
 *
 *	Do an atomic swap of the lock word.  This is used only when it's
 *	known that the lock word is set up such that it can't be changed
 *	behind us (assert this), so there's no point considering the result.
 */
static inline void
rw_swap(krwlock_t *rw, uintptr_t o, uintptr_t n)
{

	n = (uintptr_t)atomic_swap_ptr((volatile void *)&rw->rw_owner,
	    (void *)n);

	RW_ASSERT(rw, n == o);
	RW_ASSERT(rw, (o & RW_HAS_WAITERS) != 0);
}

/*
 * rw_hold_remember:
 *
 *	Helper - when acquring a lock, record the new hold.
 */
static inline uintptr_t
rw_hold_remember(krwlock_t *rw, lwp_t *l)
{
	int i;

	KASSERT(kpreempt_disabled());

	for (i = 0; i < __arraycount(l->l_rwlocks); i++) {
		if (__predict_true(l->l_rwlocks[i] == NULL)) {
			l->l_rwlocks[i] = rw;
			/*
			 * Clear the write wanted flag on every acquire to
			 * give readers a chance once again.
			 */
			return ~RW_WRITE_WANTED;
		}
	}

	/*
	 * Nowhere to track the hold so we lose: temporarily disable
	 * spinning on the lock.
	 */
	return ~(RW_WRITE_WANTED | RW_SPIN);
}

/*
 * rw_hold_forget:
 *
 *	Helper - when releasing a lock, stop tracking the hold.
 */
static inline void
rw_hold_forget(krwlock_t *rw, lwp_t *l)
{
	int i;

	KASSERT(kpreempt_disabled());

	for (i = 0; i < __arraycount(l->l_rwlocks); i++) {
		if (__predict_true(l->l_rwlocks[i] == rw)) {
			l->l_rwlocks[i] = NULL;
			return;
		}
	}
}

/*
 * rw_switch:
 *
 *	Called by mi_switch() to indicate that an LWP is going off the CPU.
 */
void
rw_switch(void)
{
	lwp_t *l = curlwp;
	int i;

	for (i = 0; i < __arraycount(l->l_rwlocks); i++) {
		if (l->l_rwlocks[i] != NULL) {
			rw_and(l->l_rwlocks[i], ~RW_SPIN);
			/* Leave in place for exit to clear. */
		}
	}
}

/*
 * rw_dump:
 *
 *	Dump the contents of a rwlock structure.
 */
static void
rw_dump(const volatile void *cookie, lockop_printer_t pr)
{
	const volatile krwlock_t *rw = cookie;

	pr("owner/count  : %#018lx flags    : %#018x\n",
	    (long)RW_OWNER(rw), (int)RW_FLAGS(rw));
}

/*
 * rw_abort:
 *
 *	Dump information about an error and panic the system.  This
 *	generates a lot of machine code in the DIAGNOSTIC case, so
 *	we ask the compiler to not inline it.
 */
static void __noinline
rw_abort(const char *func, size_t line, krwlock_t *rw, const char *msg)
{

	if (panicstr != NULL)
		return;

	LOCKDEBUG_ABORT(func, line, rw, &rwlock_lockops, msg);
}

/*
 * rw_init:
 *
 *	Initialize a rwlock for use.
 */
void
_rw_init(krwlock_t *rw, uintptr_t return_address)
{

	if (LOCKDEBUG_ALLOC(rw, &rwlock_lockops, return_address))
		rw->rw_owner = RW_SPIN;
	else
		rw->rw_owner = RW_SPIN | RW_NODEBUG;
}

void
rw_init(krwlock_t *rw)
{

	_rw_init(rw, (uintptr_t)__builtin_return_address(0));
}

/*
 * rw_destroy:
 *
 *	Tear down a rwlock.
 */
void
rw_destroy(krwlock_t *rw)
{

	RW_ASSERT(rw, (rw->rw_owner & ~(RW_NODEBUG | RW_SPIN)) == 0);
	LOCKDEBUG_FREE((rw->rw_owner & RW_NODEBUG) == 0, rw);
}

/*
 * rw_vector_enter:
 *
 *	The slow path for acquiring a rwlock, that considers all conditions.
 *	Marked __noinline to prevent the compiler pulling it into rw_enter().
 */
static void __noinline
rw_vector_enter(krwlock_t *rw, const krw_t op, uintptr_t mask, uintptr_t ra)
{
	uintptr_t owner, incr, need_wait, set_wait, curthread, next;
	turnstile_t *ts;
	int queue;
	lwp_t *l;
	LOCKSTAT_TIMER(slptime);
	LOCKSTAT_TIMER(slpcnt);
	LOCKSTAT_TIMER(spintime);
	LOCKSTAT_COUNTER(spincnt);
	LOCKSTAT_FLAG(lsflag);

	l = curlwp;
	curthread = (uintptr_t)l;

	RW_ASSERT(rw, !cpu_intr_p());
	RW_ASSERT(rw, curthread != 0);
	RW_ASSERT(rw, kpreempt_disabled());
	RW_WANTLOCK(rw, op);

	if (panicstr == NULL) {
		KDASSERT(pserialize_not_in_read_section());
		LOCKDEBUG_BARRIER(&kernel_lock, 1);
	}

	/*
	 * We play a slight trick here.  If we're a reader, we want
	 * increment the read count.  If we're a writer, we want to
	 * set the owner field and the WRITE_LOCKED bit.
	 *
	 * In the latter case, we expect those bits to be zero,
	 * therefore we can use an add operation to set them, which
	 * means an add operation for both cases.
	 */
	if (__predict_true(op == RW_READER)) {
		incr = RW_READ_INCR;
		set_wait = RW_HAS_WAITERS;
		need_wait = RW_WRITE_LOCKED | RW_WRITE_WANTED;
		queue = TS_READER_Q;
	} else {
		RW_ASSERT(rw, op == RW_WRITER);
		incr = curthread | RW_WRITE_LOCKED;
		set_wait = RW_HAS_WAITERS | RW_WRITE_WANTED;
		need_wait = RW_WRITE_LOCKED | RW_THREAD;
		queue = TS_WRITER_Q;
	}

	LOCKSTAT_ENTER(lsflag);

	for (owner = rw->rw_owner;;) {
		/*
		 * Read the lock owner field.  If the need-to-wait
		 * indicator is clear, then try to acquire the lock.
		 */
		if ((owner & need_wait) == 0) {
			next = rw_cas(rw, owner, (owner + incr) & mask);
			if (__predict_true(next == owner)) {
				/* Got it! */
				RW_MEMBAR_ENTER();
				break;
			}

			/*
			 * Didn't get it -- spin around again (we'll
			 * probably sleep on the next iteration).
			 */
			owner = next;
			continue;
		}
		if (__predict_false(RW_OWNER(rw) == curthread)) {
			rw_abort(__func__, __LINE__, rw,
			    "locking against myself");
		}

		/*
		 * If the lock owner is running on another CPU, and there
		 * are no existing waiters, then spin.  Notes:
		 *
		 * 1) If an LWP on this CPU (possibly curlwp, or an LWP that
		 * curlwp has interupted) holds kernel_lock, we can't spin
		 * without a deadlock.  The CPU that holds the rwlock may be
		 * blocked trying to acquire kernel_lock, or there may be an
		 * unseen chain of dependant locks.  To defeat the potential
		 * deadlock, this LWP needs to sleep (and thereby directly
		 * drop the kernel_lock, or permit the interrupted LWP that
		 * holds kernel_lock to complete its work).
		 *
		 * 2) If trying to acquire a write lock, and the lock is
		 * currently read held, after a brief wait set the write
		 * wanted bit to block out new readers and try to avoid
		 * starvation.  When the hold is acquired, we'll clear the
		 * WRITE_WANTED flag to give readers a chance again.  With
		 * luck this should nudge things in the direction of
		 * interleaving readers and writers when there is high
		 * contention.
		 *
		 * 3) The spin wait can't be done in soft interrupt context,
		 * because a lock holder could be pinned down underneath the
		 * soft interrupt LWP (i.e. curlwp) on the same CPU.  For
		 * the lock holder to make progress and release the lock,
		 * the soft interrupt needs to sleep.
		 */
		if ((owner & RW_SPIN) != 0 && !cpu_softintr_p()) {
			LOCKSTAT_START_TIMER(lsflag, spintime);
			u_int count = SPINLOCK_BACKOFF_MIN;
			do {
				KPREEMPT_ENABLE(curlwp);
				SPINLOCK_BACKOFF(count);
				KPREEMPT_DISABLE(curlwp);
				owner = rw->rw_owner;
				if ((owner & need_wait) == 0)
					break;
				if (count != SPINLOCK_BACKOFF_MAX)
					continue;
				if (curcpu()->ci_biglock_count != 0)
					break;
				if (op == RW_WRITER &&
				    (owner & RW_WRITE_LOCKED) == 0 &&
				    (owner & RW_WRITE_WANTED) == 0) {
					(void)rw_cas(rw, owner,
					    owner | RW_WRITE_WANTED);
				}
			} while ((owner & RW_SPIN) != 0);
			LOCKSTAT_STOP_TIMER(lsflag, spintime);
			LOCKSTAT_COUNT(spincnt, 1);
			if ((owner & need_wait) == 0)
				continue;
		}

		/*
		 * Grab the turnstile chain lock.  Once we have that, we
		 * can adjust the waiter bits and sleep queue.
		 */
		ts = turnstile_lookup(rw);

		/*
		 * Mark the rwlock as having waiters, and disable spinning. 
		 * If the set fails, then we may not need to sleep and
		 * should spin again.  Reload rw_owner now that we own
		 * the turnstile chain lock.
		 */
		owner = rw->rw_owner;
		if ((owner & need_wait) == 0 ||
		    ((owner & RW_SPIN) != 0 && !cpu_softintr_p())) {
			turnstile_exit(rw);
			continue;
		}
		next = rw_cas(rw, owner, (owner | set_wait) & ~RW_SPIN);
		if (__predict_false(next != owner)) {
			turnstile_exit(rw);
			owner = next;
			continue;
		}

		LOCKSTAT_START_TIMER(lsflag, slptime);
		turnstile_block(ts, queue, rw, &rw_syncobj);
		LOCKSTAT_STOP_TIMER(lsflag, slptime);
		LOCKSTAT_COUNT(slpcnt, 1);

		/*
		 * No need for a memory barrier because of context switch.
		 * If not handed the lock, then spin again.
		 */
		if (op == RW_READER || (rw->rw_owner & RW_THREAD) == curthread)
			break;

		owner = rw->rw_owner;
	}
	KPREEMPT_ENABLE(curlwp);

	LOCKSTAT_EVENT_RA(lsflag, rw, LB_RWLOCK |
	    (op == RW_WRITER ? LB_SLEEP1 : LB_SLEEP2), slpcnt, slptime,
	    (l->l_rwcallsite != 0 ? l->l_rwcallsite : ra));
	LOCKSTAT_EVENT_RA(lsflag, rw, LB_RWLOCK | LB_SPIN, spincnt, spintime,
	    (l->l_rwcallsite != 0 ? l->l_rwcallsite : ra));
	LOCKSTAT_EXIT(lsflag);

	RW_ASSERT(rw, (op != RW_READER && RW_OWNER(rw) == curthread) ||
	    (op == RW_READER && RW_COUNT(rw) != 0));
	RW_LOCKED(rw, op);
}

/*
 * rw_enter:
 *
 *	The fast path for acquiring a lock that considers only the
 *	uncontended case.  Falls back to rw_vector_enter().
 */
void
rw_enter(krwlock_t *rw, const krw_t op)
{
	uintptr_t owner, incr, need_wait, curthread, next, mask;
	lwp_t *l;

	l = curlwp;
	curthread = (uintptr_t)l;

	RW_ASSERT(rw, !cpu_intr_p());
	RW_ASSERT(rw, curthread != 0);
	RW_WANTLOCK(rw, op);

	KPREEMPT_DISABLE(l);
	mask = rw_hold_remember(rw, l);

	/*
	 * We play a slight trick here.  If we're a reader, we want
	 * increment the read count.  If we're a writer, we want to
	 * set the owner field and the WRITE_LOCKED bit.
	 *
	 * In the latter case, we expect those bits to be zero,
	 * therefore we can use an add operation to set them, which
	 * means an add operation for both cases.
	 */
	if (__predict_true(op == RW_READER)) {
		incr = RW_READ_INCR;
		need_wait = RW_WRITE_LOCKED | RW_WRITE_WANTED;
	} else {
		RW_ASSERT(rw, op == RW_WRITER);
		incr = curthread | RW_WRITE_LOCKED;
		need_wait = RW_WRITE_LOCKED | RW_THREAD;
	}

	/*
	 * Read the lock owner field.  If the need-to-wait
	 * indicator is clear, then try to acquire the lock.
	 */
	owner = rw->rw_owner;
	if ((owner & need_wait) == 0) {
		next = rw_cas(rw, owner, (owner + incr) & mask);
		if (__predict_true(next == owner)) {
			/* Got it! */
			KPREEMPT_ENABLE(l);
			RW_MEMBAR_ENTER();
			return;
		}
	}

	rw_vector_enter(rw, op, mask, (uintptr_t)__builtin_return_address(0));
}

/*
 * rw_vector_exit:
 *
 *	The slow path for releasing a rwlock, that considers all conditions.
 *	Marked __noinline to prevent the compiler pulling it into rw_enter().
 */
static void __noinline
rw_vector_exit(krwlock_t *rw)
{
	uintptr_t curthread, owner, decr, newown, next;
	turnstile_t *ts;
	int rcnt, wcnt;
	lwp_t *l;

	l = curlwp;
	curthread = (uintptr_t)l;
	RW_ASSERT(rw, curthread != 0);
	RW_ASSERT(rw, kpreempt_disabled());

	/*
	 * Again, we use a trick.  Since we used an add operation to
	 * set the required lock bits, we can use a subtract to clear
	 * them, which makes the read-release and write-release path
	 * the same.
	 */
	owner = rw->rw_owner;
	if (__predict_false((owner & RW_WRITE_LOCKED) != 0)) {
		RW_UNLOCKED(rw, RW_WRITER);
		RW_ASSERT(rw, RW_OWNER(rw) == curthread);
		decr = curthread | RW_WRITE_LOCKED;
	} else {
		RW_UNLOCKED(rw, RW_READER);
		RW_ASSERT(rw, RW_COUNT(rw) != 0);
		decr = RW_READ_INCR;
	}

	/*
	 * Compute what we expect the new value of the lock to be. Only
	 * proceed to do direct handoff if there are waiters, and if the
	 * lock would become unowned.
	 */
	RW_MEMBAR_EXIT();
	for (;;) {
		newown = (owner - decr);
		if ((newown & (RW_THREAD | RW_HAS_WAITERS)) == RW_HAS_WAITERS)
			break;
		/* Want spinning enabled if lock is becoming free. */
		if ((newown & RW_THREAD) == 0)
			newown |= RW_SPIN;
		next = rw_cas(rw, owner, newown);
		if (__predict_true(next == owner)) {
			rw_hold_forget(rw, l);
			kpreempt_enable();
			return;
		}
		owner = next;
	}

	/*
	 * Grab the turnstile chain lock.  This gets the interlock
	 * on the sleep queue.  Once we have that, we can adjust the
	 * waiter bits.
	 */
	ts = turnstile_lookup(rw);
	owner = rw->rw_owner;
	RW_ASSERT(rw, ts != NULL);
	RW_ASSERT(rw, (owner & RW_HAS_WAITERS) != 0);

	wcnt = TS_WAITERS(ts, TS_WRITER_Q);
	rcnt = TS_WAITERS(ts, TS_READER_Q);

	/*
	 * Give the lock away.
	 *
	 * If we are releasing a write lock, then prefer to wake all
	 * outstanding readers.  Otherwise, wake one writer if there
	 * are outstanding readers, or all writers if there are no
	 * pending readers.  If waking one specific writer, the writer
	 * is handed the lock here.  If waking multiple writers, we
	 * set WRITE_WANTED to block out new readers, and let them
	 * do the work of acquiring the lock in rw_vector_enter().
	 */
	if (rcnt == 0 || decr == RW_READ_INCR) {
		RW_ASSERT(rw, wcnt != 0);
		RW_ASSERT(rw, (owner & RW_WRITE_WANTED) != 0);

		if (rcnt != 0) {
			/* Give the lock to the longest waiting writer. */
			l = TS_FIRST(ts, TS_WRITER_Q);
			newown = (uintptr_t)l | (owner & RW_NODEBUG);
			newown |= RW_WRITE_LOCKED | RW_HAS_WAITERS;
			if (wcnt > 1)
				newown |= RW_WRITE_WANTED;
			rw_swap(rw, owner, newown);
			rw_hold_forget(rw, l);
			turnstile_wakeup(ts, TS_WRITER_Q, 1, l);
		} else {
			/* Wake all writers and let them fight it out. */
			newown = owner & RW_NODEBUG;
			newown |= RW_WRITE_WANTED;
			rw_swap(rw, owner, newown);
			rw_hold_forget(rw, l);
			turnstile_wakeup(ts, TS_WRITER_Q, wcnt, NULL);
		}
	} else {
		RW_ASSERT(rw, rcnt != 0);

		/*
		 * Give the lock to all blocked readers.  If there
		 * is a writer waiting, new readers that arrive
		 * after the release will be blocked out.
		 */
		newown = owner & RW_NODEBUG;
		newown += rcnt << RW_READ_COUNT_SHIFT;
		if (wcnt != 0)
			newown |= RW_HAS_WAITERS | RW_WRITE_WANTED;
			
		/* Wake up all sleeping readers. */
		rw_swap(rw, owner, newown);
		rw_hold_forget(rw, l);
		turnstile_wakeup(ts, TS_READER_Q, rcnt, NULL);
	}
	kpreempt_enable();
}

/*
 * rw_exit:
 *
 *	The fast path for releasing a lock that considers only the
 *	uncontended case.  Falls back to rw_vector_exit().
 */
void
rw_exit(krwlock_t *rw)
{
	uintptr_t curthread, owner, decr, newown, next;
	lwp_t *l;

	l = curlwp;
	curthread = (uintptr_t)l;
	RW_ASSERT(rw, curthread != 0);

	/*
	 * Again, we use a trick.  Since we used an add operation to
	 * set the required lock bits, we can use a subtract to clear
	 * them, which makes the read-release and write-release path
	 * the same.
	 */
	owner = rw->rw_owner;
	if (__predict_false((owner & RW_WRITE_LOCKED) != 0)) {
		RW_UNLOCKED(rw, RW_WRITER);
		RW_ASSERT(rw, RW_OWNER(rw) == curthread);
		decr = curthread | RW_WRITE_LOCKED;
	} else {
		RW_UNLOCKED(rw, RW_READER);
		RW_ASSERT(rw, RW_COUNT(rw) != 0);
		decr = RW_READ_INCR;
	}

	/* Now try to release it. */
	RW_MEMBAR_EXIT();
	KPREEMPT_DISABLE(l);
	newown = (owner - decr);
	if (__predict_true((newown & (RW_THREAD | RW_HAS_WAITERS)) !=
	    RW_HAS_WAITERS)) {
		/* Want spinning (re-)enabled if lock is becoming free. */
		if ((newown & RW_THREAD) == 0)
			newown |= RW_SPIN;
		next = rw_cas(rw, owner, newown);
		if (__predict_true(next == owner)) {
			rw_hold_forget(rw, l);
			KPREEMPT_ENABLE(l);
			return;
		}
	}
	rw_vector_exit(rw);
}

/*
 * rw_tryenter:
 *
 *	Try to acquire a rwlock.
 */
int
rw_tryenter(krwlock_t *rw, const krw_t op)
{
	uintptr_t curthread, owner, incr, need_wait, next, mask;
	lwp_t *l;

	l = curlwp;
	curthread = (uintptr_t)l;

	RW_ASSERT(rw, curthread != 0);

	KPREEMPT_DISABLE(l);
	mask = rw_hold_remember(rw, l);

	if (op == RW_READER) {
		incr = RW_READ_INCR;
		need_wait = RW_WRITE_LOCKED | RW_WRITE_WANTED;
	} else {
		RW_ASSERT(rw, op == RW_WRITER);
		incr = curthread | RW_WRITE_LOCKED;
		need_wait = RW_WRITE_LOCKED | RW_THREAD;
	}

	for (owner = rw->rw_owner;; owner = next) {
		if (__predict_false((owner & need_wait) != 0)) {
			rw_hold_forget(rw, l);
			KPREEMPT_ENABLE(l);
			return 0;
		}
		next = rw_cas(rw, owner, (owner + incr) & mask);
		if (__predict_true(next == owner)) {
			/* Got it! */
			break;
		}
	}

	RW_WANTLOCK(rw, op);
	RW_LOCKED(rw, op);
	RW_ASSERT(rw, (op != RW_READER && RW_OWNER(rw) == curthread) ||
	    (op == RW_READER && RW_COUNT(rw) != 0));

	KPREEMPT_ENABLE(l);
	RW_MEMBAR_ENTER();
	return 1;
}

/*
 * rw_downgrade:
 *
 *	Downgrade a write lock to a read lock.
 */
void
rw_downgrade(krwlock_t *rw)
{
	uintptr_t owner, curthread, newown, next;
	turnstile_t *ts;
	int rcnt, wcnt;
	lwp_t *l;

	l = curlwp;
	curthread = (uintptr_t)l;
	RW_ASSERT(rw, curthread != 0);
	RW_ASSERT(rw, (rw->rw_owner & RW_WRITE_LOCKED) != 0);
	RW_ASSERT(rw, RW_OWNER(rw) == curthread);
	RW_UNLOCKED(rw, RW_WRITER);
#if !defined(DIAGNOSTIC)
	__USE(curthread);
#endif

	RW_MEMBAR_PRODUCER();

	for (owner = rw->rw_owner;; owner = next) {
		/*
		 * If there are no waiters we can do this the easy way.  Try
		 * swapping us down to one read hold.  If it fails, the lock
		 * condition has changed and we most likely now have
		 * waiters.
		 */
		if ((owner & RW_HAS_WAITERS) == 0) {
			newown = (owner & RW_NODEBUG) | RW_SPIN;
			next = rw_cas(rw, owner, newown + RW_READ_INCR);
			if (__predict_true(next == owner)) {
				RW_LOCKED(rw, RW_READER);
				RW_ASSERT(rw,
				    (rw->rw_owner & RW_WRITE_LOCKED) == 0);
				RW_ASSERT(rw, RW_COUNT(rw) != 0);
				return;
			}
			continue;
		}

		/*
		 * Grab the turnstile chain lock.  This gets the interlock
		 * on the sleep queue.  Once we have that, we can adjust the
		 * waiter bits.
		 */
		ts = turnstile_lookup(rw);
		RW_ASSERT(rw, ts != NULL);

		rcnt = TS_WAITERS(ts, TS_READER_Q);
		wcnt = TS_WAITERS(ts, TS_WRITER_Q);

		if (rcnt == 0) {
			/*
			 * If there are no readers, just preserve the
			 * waiters bits, swap us down to one read hold and
			 * return.  Don't set the spin bit as nobody's
			 * running yet.
			 */
			RW_ASSERT(rw, wcnt != 0);
			RW_ASSERT(rw, (rw->rw_owner & RW_WRITE_WANTED) != 0);
			RW_ASSERT(rw, (rw->rw_owner & RW_HAS_WAITERS) != 0);

			newown = owner & RW_NODEBUG;
			newown = RW_READ_INCR | RW_HAS_WAITERS |
			    RW_WRITE_WANTED;
			next = rw_cas(rw, owner, newown);
			turnstile_exit(rw);
			if (__predict_true(next == owner))
				break;
		} else {
			/*
			 * Give the lock to all blocked readers.  We may
			 * retain one read hold if downgrading.  If there is
			 * a writer waiting, new readers will be blocked
			 * out.  Don't set the spin bit as nobody's running
			 * yet.
			 */
			newown = owner & RW_NODEBUG;
			newown += (rcnt << RW_READ_COUNT_SHIFT) + RW_READ_INCR;
			if (wcnt != 0)
				newown |= RW_HAS_WAITERS | RW_WRITE_WANTED;

			next = rw_cas(rw, owner, newown);
			if (__predict_true(next == owner)) {
				/* Wake up all sleeping readers. */
				turnstile_wakeup(ts, TS_READER_Q, rcnt, NULL);
				break;
			}
			turnstile_exit(rw);
		}
	}

	RW_WANTLOCK(rw, RW_READER);
	RW_LOCKED(rw, RW_READER);
	RW_ASSERT(rw, (rw->rw_owner & RW_WRITE_LOCKED) == 0);
	RW_ASSERT(rw, RW_COUNT(rw) != 0);
}

/*
 * rw_tryupgrade:
 *
 *	Try to upgrade a read lock to a write lock.  We must be the only
 *	reader.
 */
int
rw_tryupgrade(krwlock_t *rw)
{
	uintptr_t owner, curthread, newown, next;
	struct lwp *l;

	l = curlwp;
	curthread = (uintptr_t)l;
	RW_ASSERT(rw, curthread != 0);
	RW_ASSERT(rw, rw_read_held(rw));

	for (owner = RW_READ_INCR;; owner = next) {
		newown = curthread | RW_WRITE_LOCKED | (owner & ~RW_THREAD);
		next = rw_cas(rw, owner, newown);
		if (__predict_true(next == owner)) {
			RW_MEMBAR_PRODUCER();
			break;
		}
		RW_ASSERT(rw, (next & RW_WRITE_LOCKED) == 0);
		if (__predict_false((next & RW_THREAD) != RW_READ_INCR)) {
			RW_ASSERT(rw, (next & RW_THREAD) != 0);
			return 0;
		}
	}

	RW_UNLOCKED(rw, RW_READER);
	RW_WANTLOCK(rw, RW_WRITER);
	RW_LOCKED(rw, RW_WRITER);
	RW_ASSERT(rw, rw->rw_owner & RW_WRITE_LOCKED);
	RW_ASSERT(rw, RW_OWNER(rw) == curthread);

	return 1;
}

/*
 * rw_read_held:
 *
 *	Returns true if the rwlock is held for reading.  Must only be
 *	used for diagnostic assertions, and never be used to make
 * 	decisions about how to use a rwlock.
 */
int
rw_read_held(krwlock_t *rw)
{
	uintptr_t owner;

	if (rw == NULL)
		return 0;
	owner = rw->rw_owner;
	return (owner & RW_WRITE_LOCKED) == 0 && (owner & RW_THREAD) != 0;
}

/*
 * rw_write_held:
 *
 *	Returns true if the rwlock is held for writing.  Must only be
 *	used for diagnostic assertions, and never be used to make
 *	decisions about how to use a rwlock.
 */
int
rw_write_held(krwlock_t *rw)
{

	if (rw == NULL)
		return 0;
	return (rw->rw_owner & (RW_WRITE_LOCKED | RW_THREAD)) ==
	    (RW_WRITE_LOCKED | (uintptr_t)curlwp);
}

/*
 * rw_lock_held:
 *
 *	Returns true if the rwlock is held for reading or writing.  Must
 *	only be used for diagnostic assertions, and never be used to make
 *	decisions about how to use a rwlock.
 */
int
rw_lock_held(krwlock_t *rw)
{

	if (rw == NULL)
		return 0;
	return (rw->rw_owner & RW_THREAD) != 0;
}

/*
 * rw_owner:
 *
 *	Return the current owner of an RW lock, but only if it is write
 *	held.  Used for priority inheritance.
 */
static lwp_t *
rw_owner(wchan_t obj)
{
	krwlock_t *rw = (void *)(uintptr_t)obj; /* discard qualifiers */
	uintptr_t owner = rw->rw_owner;

	if ((owner & RW_WRITE_LOCKED) == 0)
		return NULL;

	return (void *)(owner & RW_THREAD);
}

/*
 * rw_owner_running:
 *
 *	Return true if a RW lock is unheld, or held and the owner is running
 *	on a CPU.  For the pagedaemon only - do not document or use in other
 *	code.
 */
bool
rw_owner_running(const krwlock_t *rw)
{
	uintptr_t owner = rw->rw_owner;

	return (owner & RW_THREAD) == 0 || (owner & RW_SPIN) != 0;
}
