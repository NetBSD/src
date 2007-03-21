/*	$NetBSD: kern_rwlock.c,v 1.6.2.1 2007/03/21 20:10:21 ad Exp $	*/

/*-
 * Copyright (c) 2002, 2006, 2007 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 */

#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_rwlock.c,v 1.6.2.1 2007/03/21 20:10:21 ad Exp $");

#define	__RWLOCK_PRIVATE

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/rwlock.h>
#include <sys/sched.h>
#include <sys/sleepq.h>
#include <sys/systm.h>
#include <sys/lockdebug.h>

#include <dev/lockstat.h>

#define RW_ABORT(rw, msg)						\
    LOCKDEBUG_ABORT(RW_GETID(rw), rw, &rwlock_lockops, __FUNCTION__, msg)

/*
 * LOCKDEBUG
 */

#if defined(LOCKDEBUG)

#define	RW_WANTLOCK(rw, op)						\
	LOCKDEBUG_WANTLOCK(RW_GETID(rw),				\
	    (uintptr_t)__builtin_return_address(0), op == RW_READER);
#define	RW_LOCKED(rw, op)						\
	LOCKDEBUG_LOCKED(RW_GETID(rw),					\
	    (uintptr_t)__builtin_return_address(0), op == RW_READER);
#define	RW_UNLOCKED(rw, op)						\
	LOCKDEBUG_UNLOCKED(RW_GETID(rw),				\
	    (uintptr_t)__builtin_return_address(0), op == RW_READER);
#define	RW_DASSERT(rw, cond)						\
do {									\
	if (!(cond))							\
		RW_ABORT(rw, "assertion failed: " #cond);		\
} while (/* CONSTCOND */ 0);

#else	/* LOCKDEBUG */

#define	RW_WANTLOCK(rw, op)	/* nothing */
#define	RW_LOCKED(rw, op)	/* nothing */
#define	RW_UNLOCKED(rw, op)	/* nothing */
#define	RW_DASSERT(rw, cond)	/* nothing */

#endif	/* LOCKDEBUG */

/*
 * DIAGNOSTIC
 */

#if defined(DIAGNOSTIC)

#define	RW_ASSERT(rw, cond)						\
do {									\
	if (!(cond))							\
		RW_ABORT(rw, "assertion failed: " #cond);		\
} while (/* CONSTCOND */ 0)

#else

#define	RW_ASSERT(rw, cond)	/* nothing */

#endif	/* DIAGNOSTIC */

/*
 * For platforms that use 'simple' RW locks.
 */
#ifdef __HAVE_SIMPLE_RW_LOCKS
#define	RW_ACQUIRE(rw, old, new)	RW_CAS(&(rw)->rw_owner, old, new)
#define	RW_RELEASE(rw, old, new)	RW_CAS(&(rw)->rw_owner, old, new)
#define	RW_SETID(rw, id)		((rw)->rw_id = id)
#define	RW_GETID(rw)			((rw)->rw_id)

static inline int
RW_SET_WAITERS(krwlock_t *rw, uintptr_t need, uintptr_t set)
{
	uintptr_t old;

	if (((old = rw->rw_owner) & need) == 0)
		return 0;
	return RW_CAS(&rw->rw_owner, old, old | set);
}
#endif	/* __HAVE_SIMPLE_RW_LOCKS */

/*
 * For platforms that do not provide stubs, or for the LOCKDEBUG case.
 */
#ifdef LOCKDEBUG
#undef	__HAVE_RW_STUBS
#endif

#ifndef __HAVE_RW_STUBS
__strong_alias(rw_enter,rw_vector_enter);
__strong_alias(rw_exit,rw_vector_exit);
#endif

void	rw_dump(volatile void *);
static struct lwp *rw_owner(wchan_t);

lockops_t rwlock_lockops = {
	"Reader / writer lock",
	1,
	rw_dump
};

syncobj_t rw_syncobj = {
	SOBJ_SLEEPQ_SORTED,
	turnstile_unsleep,
	turnstile_changepri,
	sleepq_lendpri,
	rw_owner,
};

/*
 * rw_dump:
 *
 *	Dump the contents of a rwlock structure.
 */
void
rw_dump(volatile void *cookie)
{
	volatile krwlock_t *rw = cookie;

	printf_nolog("owner/count  : %#018lx flags    : %#018x\n",
	    (long)RW_OWNER(rw), (int)RW_FLAGS(rw));
}

/*
 * rw_init:
 *
 *	Initialize a rwlock for use.
 */
void
rw_init(krwlock_t *rw)
{
	u_int id;

	memset(rw, 0, sizeof(*rw));

	id = LOCKDEBUG_ALLOC(rw, &rwlock_lockops);
	RW_SETID(rw, id);
}

/*
 * rw_destroy:
 *
 *	Tear down a rwlock.
 */
void
rw_destroy(krwlock_t *rw)
{

	LOCKDEBUG_FREE(rw, RW_GETID(rw));
	RW_ASSERT(rw, rw->rw_owner == 0);
}

/*
 * rw_vector_enter:
 *
 *	Acquire a rwlock.
 */
void
rw_vector_enter(krwlock_t *rw, const krw_t op)
{
	uintptr_t owner, incr, need_wait, set_wait, curthread;
	turnstile_t *ts;
	int queue;
	struct lwp *l;
	LOCKSTAT_TIMER(slptime);
	LOCKSTAT_FLAG(lsflag);

	l = curlwp;
	curthread = (uintptr_t)l;

	RW_ASSERT(rw, curthread != 0);
	RW_WANTLOCK(rw, op);

#ifdef LOCKDEBUG
	if (panicstr == NULL) {
#ifdef MULTIPROCESSOR
		LOCKDEBUG_BARRIER(&kernel_lock, 1);
#else
		LOCKDEBUG_BARRIER(NULL, 1);
#endif
	}
#endif

	/*
	 * We play a slight trick here.  If we're a reader, we want
	 * increment the read count.  If we're a writer, we want to
	 * set the owner field and whe WRITE_LOCKED bit.
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
		RW_DASSERT(rw, op == RW_WRITER);
		incr = curthread | RW_WRITE_LOCKED;
		set_wait = RW_HAS_WAITERS | RW_WRITE_WANTED;
		need_wait = RW_WRITE_LOCKED | RW_THREAD;
		queue = TS_WRITER_Q;
	}

	LOCKSTAT_ENTER(lsflag);

	for (;;) {
		/*
		 * Read the lock owner field.  If the need-to-wait
		 * indicator is clear, then try to acquire the lock.
		 */
		owner = rw->rw_owner;
		if ((owner & need_wait) == 0) {
			if (RW_ACQUIRE(rw, owner, owner + incr)) {
				/* Got it! */
				break;
			}

			/*
			 * Didn't get it -- spin around again (we'll
			 * probably sleep on the next iteration).
			 */
			continue;
		}

		if (panicstr != NULL)
			return;
		if (RW_OWNER(rw) == curthread)
			RW_ABORT(rw, "locking against myself");

		/*
		 * Grab the turnstile chain lock.  Once we have that, we
		 * can adjust the waiter bits and sleep queue.
		 */
		ts = turnstile_lookup(rw);

		/*
		 * XXXSMP if this is a high priority LWP (interrupt handler
		 * or realtime) and acquiring a read hold, then we shouldn't
		 * wait for RW_WRITE_WANTED if our priority is >= that of
		 * the highest priority writer that is waiting.
		 */

		/*
		 * Mark the rwlock as having waiters.  If the set fails,
		 * then we may not need to sleep and should spin again.
		 */
		if (!RW_SET_WAITERS(rw, need_wait, set_wait)) {
			turnstile_exit(rw);
			continue;
		}

		LOCKSTAT_START_TIMER(lsflag, slptime);

		turnstile_block(ts, queue, rw, &rw_syncobj);

		/* If we wake up and arrive here, we've been handed the lock. */
		RW_RECEIVE(rw);

		LOCKSTAT_STOP_TIMER(lsflag, slptime);
		LOCKSTAT_EVENT(lsflag, rw,
		    LB_RWLOCK | (op == RW_WRITER ? LB_SLEEP1 : LB_SLEEP2),
		    1, slptime);

		turnstile_unblock();
		break;
	}

	LOCKSTAT_EXIT(lsflag);

	RW_DASSERT(rw, (op != RW_READER && RW_OWNER(rw) == curthread) ||
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
	uintptr_t curthread, owner, decr, new;
	turnstile_t *ts;
	int rcnt, wcnt;
	struct lwp *l;

	curthread = (uintptr_t)curlwp;
	RW_ASSERT(rw, curthread != 0);

	if (panicstr != NULL) {
		/*
		 * XXX What's the correct thing to do here?  We should at
		 * least release the lock.
		 */
		return;
	}

	/*
	 * Again, we use a trick.  Since we used an add operation to
	 * set the required lock bits, we can use a subtract to clear
	 * them, which makes the read-release and write-release path
	 * the same.
	 */
	owner = rw->rw_owner;
	if (__predict_false((owner & RW_WRITE_LOCKED) != 0)) {
		RW_UNLOCKED(rw, RW_WRITER);
		RW_DASSERT(rw, (rw->rw_owner & RW_WRITE_LOCKED) != 0);
		RW_ASSERT(rw, RW_OWNER(rw) == curthread);
		decr = curthread | RW_WRITE_LOCKED;
	} else {
		RW_UNLOCKED(rw, RW_READER);
		RW_ASSERT(rw, (rw->rw_owner & RW_WRITE_LOCKED) == 0);
		RW_ASSERT(rw, RW_COUNT(rw) != 0);
		decr = RW_READ_INCR;
	}

	/*
	 * Compute what we expect the new value of the lock to be. Only
	 * proceed to do direct handoff if there are waiters, and if the
	 * lock would become unowned.
	 */
	for (;; owner = rw->rw_owner) {
		new = (owner - decr);
		if ((new & (RW_THREAD | RW_HAS_WAITERS)) == RW_HAS_WAITERS)
			break;
		if (RW_RELEASE(rw, owner, new))
			return;
	}

	for (;;) {
		/*
		 * Grab the turnstile chain lock.  This gets the interlock
		 * on the sleep queue.  Once we have that, we can adjust the
		 * waiter bits.
		 */
		ts = turnstile_lookup(rw);
		RW_DASSERT(rw, ts != NULL);
		RW_DASSERT(rw, (rw->rw_owner & RW_HAS_WAITERS) != 0);

		owner = rw->rw_owner;
		wcnt = TS_WAITERS(ts, TS_WRITER_Q);
		rcnt = TS_WAITERS(ts, TS_READER_Q);

		/*
		 * Give the lock away.
		 *
		 * If we are releasing a write lock, then wake all
		 * outstanding readers.  If we are releasing a read
		 * lock, then wake one writer.
		 */
		if (rcnt == 0 || (decr == RW_READ_INCR && wcnt != 0)) {
			RW_DASSERT(rw, wcnt != 0);
			RW_DASSERT(rw, (rw->rw_owner & RW_WRITE_WANTED) != 0);

			/*
			 * Give the lock to the longest waiting
			 * writer.
			 */
			l = TS_FIRST(ts, TS_WRITER_Q);
			new = (uintptr_t)l | RW_WRITE_LOCKED;

			if (wcnt > 1)
				new |= RW_HAS_WAITERS | RW_WRITE_WANTED;
			else if (rcnt != 0)
				new |= RW_HAS_WAITERS;

			RW_GIVE(rw);
			if (!RW_RELEASE(rw, owner, new)) {
				/* Oops, try again. */
				turnstile_exit(rw);
				continue;
			}

			/* Wake the writer. */
			turnstile_wakeup(ts, TS_WRITER_Q, wcnt, l);
		} else {
			RW_DASSERT(rw, rcnt != 0);

			/*
			 * Give the lock to all blocked readers.  If there
			 * is a writer waiting, new readers that arrive
			 * after the release will be blocked out.
			 */
			new = rcnt << RW_READ_COUNT_SHIFT;
			if (wcnt != 0)
				new |= RW_HAS_WAITERS | RW_WRITE_WANTED;
				
			RW_GIVE(rw);
			if (!RW_RELEASE(rw, owner, new)) {
				/* Oops, try again. */
				turnstile_exit(rw);
				continue;
			}

			/* Wake up all sleeping readers. */
			turnstile_wakeup(ts, TS_READER_Q, rcnt, NULL);
		}

		break;
	}
}

/*
 * rw_tryenter:
 *
 *	Try to acquire a rwlock.
 */
int
rw_tryenter(krwlock_t *rw, const krw_t op)
{
	uintptr_t curthread, owner, incr, need_wait;

	curthread = (uintptr_t)curlwp;

	RW_ASSERT(rw, curthread != 0);
	RW_WANTLOCK(rw, op);

	if (op == RW_READER) {
		incr = RW_READ_INCR;
		need_wait = RW_WRITE_LOCKED | RW_WRITE_WANTED;
	} else {
		RW_DASSERT(rw, op == RW_WRITER);
		incr = curthread | RW_WRITE_LOCKED;
		need_wait = RW_WRITE_LOCKED | RW_THREAD;
	}

	for (;;) {
		owner = rw->rw_owner;
		if ((owner & need_wait) == 0) {
			if (RW_ACQUIRE(rw, owner, owner + incr)) {
				/* Got it! */
				break;
			}
			continue;
		}
		return 0;
	}

	RW_LOCKED(rw, op);
	RW_DASSERT(rw, (op != RW_READER && RW_OWNER(rw) == curthread) ||
	    (op == RW_READER && RW_COUNT(rw) != 0));
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
	uintptr_t owner, curthread, new;
	turnstile_t *ts;
	int rcnt, wcnt;

	curthread = (uintptr_t)curlwp;
	RW_ASSERT(rw, curthread != 0);
	RW_DASSERT(rw, (rw->rw_owner & RW_WRITE_LOCKED) != 0);
	RW_ASSERT(rw, RW_OWNER(rw) == curthread);
	RW_UNLOCKED(rw, RW_WRITER);

	owner = rw->rw_owner;
	if ((owner & RW_HAS_WAITERS) == 0) {
		/*
		 * There are no waiters, so we can do this the easy way.
		 * Try swapping us down to one read hold.  If it fails, the
		 * lock condition has changed and we most likely now have
		 * waiters.
		 */
		if (RW_RELEASE(rw, owner, RW_READ_INCR)) {
			RW_LOCKED(rw, RW_READER);
			RW_DASSERT(rw, (rw->rw_owner & RW_WRITE_LOCKED) == 0);
			RW_DASSERT(rw, RW_COUNT(rw) != 0);
			return;
		}
	}

	/*
	 * Grab the turnstile chain lock.  This gets the interlock
	 * on the sleep queue.  Once we have that, we can adjust the
	 * waiter bits.
	 */
	for (;;) {
		ts = turnstile_lookup(rw);
		RW_DASSERT(rw, ts != NULL);

		owner = rw->rw_owner;
		rcnt = TS_WAITERS(ts, TS_READER_Q);
		wcnt = TS_WAITERS(ts, TS_WRITER_Q);

		/*
		 * If there are no readers, just preserve the waiters
		 * bits, swap us down to one read hold and return.
		 */
		if (rcnt == 0) {
			RW_DASSERT(rw, wcnt != 0);
			RW_DASSERT(rw, (rw->rw_owner & RW_WRITE_WANTED) != 0);
			RW_DASSERT(rw, (rw->rw_owner & RW_HAS_WAITERS) != 0);

			new = RW_READ_INCR | RW_HAS_WAITERS | RW_WRITE_WANTED;
			if (!RW_RELEASE(rw, owner, new)) {
				/* Oops, try again. */
				turnstile_exit(ts);
				continue;
			}
			break;
		}
				
		/*
		 * Give the lock to all blocked readers.  We may
		 * retain one read hold if downgrading.  If there
		 * is a writer waiting, new readers will be blocked
		 * out.
		 */
		new = (rcnt << RW_READ_COUNT_SHIFT) + RW_READ_INCR;
		if (wcnt != 0)
			new |= RW_HAS_WAITERS | RW_WRITE_WANTED;

		RW_GIVE(rw);
		if (!RW_RELEASE(rw, owner, new)) {
			/* Oops, try again. */
			turnstile_exit(rw);
			continue;
		}

		/* Wake up all sleeping readers. */
		turnstile_wakeup(ts, TS_READER_Q, rcnt, NULL);
		break;
	}

	RW_LOCKED(rw, RW_READER);
	RW_DASSERT(rw, (rw->rw_owner & RW_WRITE_LOCKED) == 0);
	RW_DASSERT(rw, RW_COUNT(rw) != 0);
}

/*
 * rw_tryupgrade:
 *
 *	Try to upgrade a read lock to a write lock.  We must be the
 *	only reader.
 */
int
rw_tryupgrade(krwlock_t *rw)
{
	uintptr_t owner, curthread, new;

	curthread = (uintptr_t)curlwp;
	RW_ASSERT(rw, curthread != 0);
	RW_WANTLOCK(rw, RW_WRITER);

	for (;;) {
		owner = rw->rw_owner;
		RW_ASSERT(rw, (owner & RW_WRITE_LOCKED) == 0);
		if ((owner & RW_THREAD) != RW_READ_INCR) {
			RW_ASSERT(rw, (owner & RW_THREAD) != 0);
			return 0;
		}
		new = curthread | RW_WRITE_LOCKED | (owner & ~RW_THREAD);
		if (RW_ACQUIRE(rw, owner, new))
			break;
	}

	RW_UNLOCKED(rw, RW_READER);
	RW_LOCKED(rw, RW_WRITER);
	RW_DASSERT(rw, rw->rw_owner & RW_WRITE_LOCKED);
	RW_DASSERT(rw, RW_OWNER(rw) == curthread);

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

	if (panicstr != NULL)
		return 1;

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

	if (panicstr != NULL)
		return 1;

	return (rw->rw_owner & RW_WRITE_LOCKED) != 0;
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

	if (panicstr != NULL)
		return 1;

	return (rw->rw_owner & RW_THREAD) != 0;
}

/*
 * rw_owner:
 *
 *	Return the current owner of an RW lock, but only if it is write
 *	held.  Used for priority inheritance.
 */
static struct lwp *
rw_owner(wchan_t obj)
{
	krwlock_t *rw = (void *)(uintptr_t)obj; /* discard qualifiers */
	uintptr_t owner = rw->rw_owner;

	if ((owner & RW_WRITE_LOCKED) == 0)
		return NULL;

	return (void *)(owner & RW_THREAD);
}
