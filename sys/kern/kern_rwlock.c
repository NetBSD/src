/*	$NetBSD: kern_rwlock.c,v 1.1.36.4 2006/11/17 16:34:36 ad Exp $	*/

/*-
 * Copyright (c) 2002, 2006 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kern_rwlock.c,v 1.1.36.4 2006/11/17 16:34:36 ad Exp $");

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

#define	RW_LOCKED(rw, op)						\
do {									\
	LOCKDEBUG_LOCKED(RW_GETID(rw),					\
	    (uintptr_t)__builtin_return_address(0), op == RW_READER);	\
} while (/* CONSTCOND */ 0)

#define	RW_UNLOCKED(rw, op)						\
do {									\
	LOCKDEBUG_UNLOCKED(RW_GETID(rw),				\
	    (uintptr_t)__builtin_return_address(0), op == RW_READER);	\
} while (/* CONSTCOND */ 0)

#define	RW_DASSERT(rw, cond)						\
do {									\
	if (!(cond))							\
		RW_ABORT(rw, "assertion failed: " #cond);		\
} while (/* CONSTCOND */ 0);

#else	/* LOCKDEBUG */

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

int	rw_dump(volatile void *, char *, size_t);

lockops_t rwlock_lockops = {
	"Reader / writer lock",
	1,
	rw_dump
};

/*
 * rw_dump:
 *
 *	Dump the contents of a rwlock structure.
 */
int
rw_dump(volatile void *cookie, char *buf, size_t l)
{
	volatile krwlock_t *rw = cookie;

	return snprintf(buf, l, "owner/count  : 0x%16lx flags    : 0x%16x\n",
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
#ifdef __NEED_RW_CALLSITE
rw_vector_enter(krwlock_t *rw, krw_t op, uintptr_t callsite)
#else
rw_vector_enter(krwlock_t *rw, krw_t op)
#endif
{
	uintptr_t owner, incr, need_wait, set_wait, curthread;
	turnstile_t *ts;
	int queue;
	LOCKSTAT_TIMER(slptime);
	struct lwp *l;

	l = curlwp;
	curthread = (uintptr_t)l;
	RW_ASSERT(rw, curthread != 0);

#ifdef LOCKDEBUG
	if (panicstr == NULL) {
		simple_lock_only_held(NULL, "rw_enter");
		LOCKDEBUG_BARRIER(&kernel_lock, 1);
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
	if (op == RW_READER) {
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
		 * Mark the rwlock as having waiters.  If the set fails,
		 * then we may not need to sleep and should spin again.
		 */
		if (!RW_SET_WAITERS(rw, need_wait, set_wait)) {
			turnstile_exit(rw);
			continue;
		}

		LOCKSTAT_START_TIMER(slptime);

		turnstile_block(ts, queue, sched_kpri(l), rw);

		/* If we wake up and arrive here, we've been handed the lock. */
		RW_RECEIVE(rw);

		LOCKSTAT_STOP_TIMER(slptime);
#ifdef __NEED_RW_CALLSITE
		LOCKSTAT_EVENT_RA(rw, LB_RWLOCK | LB_SLEEP, 1, slptime, callsite);
#else
		LOCKSTAT_EVENT(rw, LB_RWLOCK | LB_SLEEP, 1, slptime);
#endif

		turnstile_unblock();
		break;
	}

	RW_DASSERT(rw, (op != RW_READER && RW_OWNER(rw) == curthread) ||
	    (op == RW_READER && RW_COUNT(rw) != 0));
}

/*
 * rw_vector_exit:
 *
 *	Release a rwlock.
 */
void
rw_vector_exit(krwlock_t *rw, krw_t op)
{
	uintptr_t curthread, owner, decr, new;
	turnstile_t *ts;
	int rcnt, wcnt, dcnt;
	struct lwp *l;

	curthread = (uintptr_t)curlwp;
	RW_ASSERT(rw, curthread != 0);

	if (panicstr != NULL) {
		/*
		 * XXX What's the correct thing to do here?  We should at least
		 * release the lock.
		 */
		return;
	}

	/*
	 * Again, we use a trick.  Since we used an add operation to
	 * set the required lock bits, we can use a subtract to clear
	 * them, which makes the read-release and write-release path
	 * the same.
	 */
	switch (op) {
	case RW_READER:
		RW_ASSERT(rw, (rw->rw_owner & RW_WRITE_LOCKED) == 0);
		RW_ASSERT(rw, RW_COUNT(rw) != 0);
		dcnt = 0;
		decr = RW_READ_INCR;
		break;
	case RW_WRITER:
		RW_DASSERT(rw, (rw->rw_owner & RW_WRITE_LOCKED) != 0);
		RW_ASSERT(rw, RW_OWNER(rw) == curthread);
		dcnt = 0;
		decr = curthread | RW_WRITE_LOCKED;
		break;
	case __RW_DOWNGRADE:
		RW_DASSERT(rw, (rw->rw_owner & RW_WRITE_LOCKED) != 0);
		RW_ASSERT(rw, RW_OWNER(rw) == curthread);
		dcnt = 1;
		decr = (curthread | RW_WRITE_LOCKED) - RW_READ_INCR;
		break;
	default:
		RW_DASSERT(rw, "blame gcc, I do");
		return;
	}

	for (;;) {
		/*
		 * We assume that the caller has already tried to release
		 * the lock and optimize for the 'has waiters' case, and so
		 * grab the turnstile chain lock.  This gets the interlock
		 * on the sleep queue.  Once we have that, we can adjust the
		 * waiter bits.
		 */
		ts = turnstile_lookup(rw);

		/*
		 * Compute what we expect the new value of the lock to be. 
		 * Only proceed to do direct handoff if there are waiters,
		 * and if the lock would become unowned.
		 */
		owner = rw->rw_owner;
		new = (owner - decr) & ~RW_WRITE_WANTED;
		if ((new & (RW_THREAD | RW_HAS_WAITERS)) != RW_HAS_WAITERS) {
			if (RW_RELEASE(rw, owner, new)) {
				turnstile_exit(rw);
				break;
			}
			turnstile_exit(rw);
			continue;
		}

		/*
		 * Adjust the waiter bits.  If we are releasing a write
		 * lock or downgrading a write lock to read, then wake all
		 * outstanding readers.  If we are releasing a read lock,
		 * then wake one writer.
		 */
		RW_DASSERT(rw, ts != NULL);

		wcnt = TS_WAITERS(ts, TS_WRITER_Q);
		rcnt = TS_WAITERS(ts, TS_READER_Q);

		/*
		 * Give the lock away.
		 */
		if (dcnt == 0 &&
		    (rcnt == 0 || (op == RW_READER && wcnt != 0))) {
			RW_DASSERT(rw, wcnt != 0);

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

			if (!RW_RELEASE(rw, owner, new)) {
				/* Oops, try again. */
				turnstile_exit(rw);
				continue;
			}

			/* Wake the writer. */
			turnstile_wakeup(ts, TS_WRITER_Q, wcnt, l);
		} else {
			dcnt += rcnt;
			RW_DASSERT(rw, dcnt != 0);

			/*
			 * Give the lock to all blocked readers.  We may
			 * retain one read hold if downgrading.  If there
			 * is a writer waiting, new readers will be blocked
			 * out.
			 */
			new = dcnt << RW_READ_COUNT_SHIFT;
			if (wcnt != 0)
				new |= RW_HAS_WAITERS | RW_WRITE_WANTED;
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
rw_tryenter(krwlock_t *rw, krw_t op)
{
	uintptr_t curthread, owner, incr, need_wait;

	curthread = (uintptr_t)curlwp;
	RW_ASSERT(rw, curthread != 0);

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
	uintptr_t owner, curthread;

	curthread = (uintptr_t)curlwp;
	RW_ASSERT(rw, curthread != 0);
	RW_DASSERT(rw, (rw->rw_owner & RW_WRITE_LOCKED) != 0);
	RW_ASSERT(rw, RW_OWNER(rw) == curthread);
	RW_UNLOCKED(rw, RW_WRITER);

	for (;;) {
		owner = rw->rw_owner;

		/* If there are waiters we need to do this the hard way. */
		if ((owner & RW_HAS_WAITERS) != 0) {
			rw_vector_exit(rw, __RW_DOWNGRADE);
			return;
		}

		/*
		 * Try swapping us down to one read hold.  If it fails, the
		 * lock condition has changed and we most likely now have
		 * waiters.
		 */
		if (RW_RELEASE(rw, owner, RW_READ_INCR))
			break;
	}

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
 * Slow stubs for platforms that do not implement fast-path ones.
 */
#ifndef __HAVE_RW_ENTER
void
rw_enter(krwlock_t *rw, krw_t op)
{
	rw_vector_enter(rw, op, (uintptr_t)__builtin_return_address(0));
	RW_LOCKED(rw, op);
}
#endif

#ifndef __HAVE_RW_EXIT
void
rw_exit(krwlock_t *rw)
{
	krw_t op;
	op = ((rw->rw_owner & RW_WRITE_LOCKED) ? RW_WRITER : RW_READER);
	RW_UNLOCKED(rw, op);
	rw_vector_exit(rw, op);
}
#endif
