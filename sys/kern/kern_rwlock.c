/*	$NetBSD: kern_rwlock.c,v 1.66 2022/04/09 23:46:19 riastradh Exp $	*/

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
 * The NetBSD implementation differs from that described in the book, in
 * that the locks are partially adaptive.  Lock waiters spin wait while a
 * lock is write held and the holder is still running on a CPU.  The method
 * of choosing which threads to awaken when a lock is released also differs,
 * mainly to take account of the partially adaptive behaviour.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_rwlock.c,v 1.66 2022/04/09 23:46:19 riastradh Exp $");

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

#include <machine/rwlock.h>

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
#define	RW_MEMBAR_ACQUIRE()
#define	RW_MEMBAR_RELEASE()
#define	RW_MEMBAR_PRODUCER()
#else
#define	RW_MEMBAR_ACQUIRE()		membar_acquire()
#define	RW_MEMBAR_RELEASE()		membar_release()
#define	RW_MEMBAR_PRODUCER()		membar_producer()
#endif

/*
 * For platforms that do not provide stubs, or for the LOCKDEBUG case.
 */
#ifdef LOCKDEBUG
#undef	__HAVE_RW_STUBS
#endif

#ifndef __HAVE_RW_STUBS
__strong_alias(rw_enter,rw_vector_enter);
__strong_alias(rw_exit,rw_vector_exit);
__strong_alias(rw_tryenter,rw_vector_tryenter);
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

#ifdef LOCKDEBUG
	/* XXX only because the assembly stubs can't handle RW_NODEBUG */
	if (LOCKDEBUG_ALLOC(rw, &rwlock_lockops, return_address))
		rw->rw_owner = 0;
	else
		rw->rw_owner = RW_NODEBUG;
#else
	rw->rw_owner = 0;
#endif
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

	RW_ASSERT(rw, (rw->rw_owner & ~RW_NODEBUG) == 0);
	LOCKDEBUG_FREE((rw->rw_owner & RW_NODEBUG) == 0, rw);
}

/*
 * rw_oncpu:
 *
 *	Return true if an rwlock owner is running on a CPU in the system.
 *	If the target is waiting on the kernel big lock, then we must
 *	release it.  This is necessary to avoid deadlock.
 */
static bool
rw_oncpu(uintptr_t owner)
{
#ifdef MULTIPROCESSOR
	struct cpu_info *ci;
	lwp_t *l;

	KASSERT(kpreempt_disabled());

	if ((owner & (RW_WRITE_LOCKED|RW_HAS_WAITERS)) != RW_WRITE_LOCKED) {
		return false;
	}

	/*
	 * See lwp_dtor() why dereference of the LWP pointer is safe.
	 * We must have kernel preemption disabled for that.
	 */
	l = (lwp_t *)(owner & RW_THREAD);
	ci = l->l_cpu;

	if (ci && ci->ci_curlwp == l) {
		/* Target is running; do we need to block? */
		return (ci->ci_biglock_wanted != l);
	}
#endif
	/* Not running.  It may be safe to block now. */
	return false;
}

/*
 * rw_vector_enter:
 *
 *	Acquire a rwlock.
 */
void
rw_vector_enter(krwlock_t *rw, const krw_t op)
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

	KPREEMPT_DISABLE(curlwp);
	for (owner = rw->rw_owner;;) {
		/*
		 * Read the lock owner field.  If the need-to-wait
		 * indicator is clear, then try to acquire the lock.
		 */
		if ((owner & need_wait) == 0) {
			next = rw_cas(rw, owner, (owner + incr) &
			    ~RW_WRITE_WANTED);
			if (__predict_true(next == owner)) {
				/* Got it! */
				RW_MEMBAR_ACQUIRE();
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
		 * If the lock owner is running on another CPU, and
		 * there are no existing waiters, then spin.
		 */
		if (rw_oncpu(owner)) {
			LOCKSTAT_START_TIMER(lsflag, spintime);
			u_int count = SPINLOCK_BACKOFF_MIN;
			do {
				KPREEMPT_ENABLE(curlwp);
				SPINLOCK_BACKOFF(count);
				KPREEMPT_DISABLE(curlwp);
				owner = rw->rw_owner;
			} while (rw_oncpu(owner));
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
		 * Mark the rwlock as having waiters.  If the set fails,
		 * then we may not need to sleep and should spin again.
		 * Reload rw_owner because turnstile_lookup() may have
		 * spun on the turnstile chain lock.
		 */
		owner = rw->rw_owner;
		if ((owner & need_wait) == 0 || rw_oncpu(owner)) {
			turnstile_exit(rw);
			continue;
		}
		next = rw_cas(rw, owner, owner | set_wait);
		/* XXX membar? */
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
	    (l->l_rwcallsite != 0 ? l->l_rwcallsite :
	      (uintptr_t)__builtin_return_address(0)));
	LOCKSTAT_EVENT_RA(lsflag, rw, LB_RWLOCK | LB_SPIN, spincnt, spintime,
	    (l->l_rwcallsite != 0 ? l->l_rwcallsite :
	      (uintptr_t)__builtin_return_address(0)));
	LOCKSTAT_EXIT(lsflag);

	RW_ASSERT(rw, (op != RW_READER && RW_OWNER(rw) == curthread) ||
	    (op == RW_READER && RW_COUNT(rw) != 0));
	RW_LOCKED(rw, op);
}

/*
 * rw_vector_exit:
 *
 *	Release a rwlock.
 */
void
rw_vector_exit(krwlock_t *rw)
{
	uintptr_t curthread, owner, decr, newown, next;
	turnstile_t *ts;
	int rcnt, wcnt;
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

	/*
	 * Compute what we expect the new value of the lock to be. Only
	 * proceed to do direct handoff if there are waiters, and if the
	 * lock would become unowned.
	 */
	RW_MEMBAR_RELEASE();
	for (;;) {
		newown = (owner - decr);
		if ((newown & (RW_THREAD | RW_HAS_WAITERS)) == RW_HAS_WAITERS)
			break;
		next = rw_cas(rw, owner, newown);
		if (__predict_true(next == owner))
			return;
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
			turnstile_wakeup(ts, TS_WRITER_Q, 1, l);
		} else {
			/* Wake all writers and let them fight it out. */
			newown = owner & RW_NODEBUG;
			newown |= RW_WRITE_WANTED;
			rw_swap(rw, owner, newown);
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
		turnstile_wakeup(ts, TS_READER_Q, rcnt, NULL);
	}
}

/*
 * rw_vector_tryenter:
 *
 *	Try to acquire a rwlock.
 */
int
rw_vector_tryenter(krwlock_t *rw, const krw_t op)
{
	uintptr_t curthread, owner, incr, need_wait, next;
	lwp_t *l;

	l = curlwp;
	curthread = (uintptr_t)l;

	RW_ASSERT(rw, curthread != 0);

	if (op == RW_READER) {
		incr = RW_READ_INCR;
		need_wait = RW_WRITE_LOCKED | RW_WRITE_WANTED;
	} else {
		RW_ASSERT(rw, op == RW_WRITER);
		incr = curthread | RW_WRITE_LOCKED;
		need_wait = RW_WRITE_LOCKED | RW_THREAD;
	}

	for (owner = rw->rw_owner;; owner = next) {
		if (__predict_false((owner & need_wait) != 0))
			return 0;
		next = rw_cas(rw, owner, owner + incr);
		if (__predict_true(next == owner)) {
			/* Got it! */
			break;
		}
	}

	RW_WANTLOCK(rw, op);
	RW_LOCKED(rw, op);
	RW_ASSERT(rw, (op != RW_READER && RW_OWNER(rw) == curthread) ||
	    (op == RW_READER && RW_COUNT(rw) != 0));

	RW_MEMBAR_ACQUIRE();
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
			newown = (owner & RW_NODEBUG);
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
			 * return.
			 */
			RW_ASSERT(rw, wcnt != 0);
			RW_ASSERT(rw, (rw->rw_owner & RW_WRITE_WANTED) != 0);
			RW_ASSERT(rw, (rw->rw_owner & RW_HAS_WAITERS) != 0);

			newown = owner & RW_NODEBUG;
			newown |= RW_READ_INCR | RW_HAS_WAITERS |
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
			 * out.
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
 * rw_lock_op:
 *
 *	For a rwlock that is known to be held by the caller, return
 *	RW_READER or RW_WRITER to describe the hold type.
 */
krw_t
rw_lock_op(krwlock_t *rw)
{

	RW_ASSERT(rw, rw_lock_held(rw));

	return (rw->rw_owner & RW_WRITE_LOCKED) != 0 ? RW_WRITER : RW_READER;
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
 *	Return true if a RW lock is unheld, or write held and the owner is
 *	running on a CPU.  For the pagedaemon.
 */
bool
rw_owner_running(const krwlock_t *rw)
{
#ifdef MULTIPROCESSOR
	uintptr_t owner;
	bool rv;

	kpreempt_disable();
	owner = rw->rw_owner;
	rv = (owner & RW_THREAD) == 0 || rw_oncpu(owner);
	kpreempt_enable();
	return rv;
#else
	return rw_owner(rw) == curlwp;
#endif
}
