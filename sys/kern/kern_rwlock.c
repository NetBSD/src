/*	$NetBSD: kern_rwlock.c,v 1.1.2.4 2002/03/17 20:18:56 thorpej Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Kernel reader/writer lock implementation, modeled after those found in
 * Solaris, a description of which can be found in:
 *
 *	Solaris Internals: Core Kernel Architecture, Jim Mauro and
 *	    Richard McDougall.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_rwlock.c,v 1.1.2.4 2002/03/17 20:18:56 thorpej Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/rwlock.h>
#include <sys/sched.h>
#include <sys/systm.h>

/*
 * rw_init:
 *
 *	Initialize a rwlock for use.
 */
void
rw_init(krwlock_t *rwl)
{

	RWLOCK_INIT(rwl);
}

/*
 * rw_destroy:
 *
 *	Tear down a rwlock.
 */
void
rw_destroy(krwlock_t *rwl)
{

	/* XXX IMPLEMENT ME XXX */
}

/*
 * rw_enter:
 *
 *	Acquire a rwlock.
 */
void
rw_enter(krwlock_t *rwl, krw_t rw)
{
	struct turnstile *ts;
	struct proc *p;
	unsigned long owner, incr, need_wait, set_wait;

	/*
	 * Ensure RW_WRITER == 0, so that machine-dependent code can
	 * make that assumption.
	 */
#if RW_WRITER != 0
#error "RW_WRITER != 0"
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
	switch (rw) {
	case RW_WRITER:
		incr = ((unsigned long) curproc) | RWLOCK_WRITE_LOCKED;
		need_wait = RWLOCK_WRITE_LOCKED;
		set_wait = RWLOCK_HAS_WAITERS | RWLOCK_WRITE_WANTED;
		break;

	case RW_READER:
		incr = RWLOCK_READ_INCR;
		need_wait = RWLOCK_WRITE_LOCKED | RWLOCK_WRITE_WANTED;
		set_wait = RWLOCK_HAS_WAITERS;
		break;
#ifdef DIAGNOSTIC
	default:
		panic("rw_enter: bad rw %d", rw);
#endif
	}

	for (;;) {
		/*
		 * Read the lock owner field.  If the need-to-wait
		 * indicator is clear, then try to acquire the lock.
		 */
		owner = rwl->rwl_owner;
		if ((owner & need_wait) == 0) {
			if (RWLOCK_ACQUIRE(rwl, owner, owner + incr)) {
				/* Got it! */
				break;
			}

			/*
			 * Didn't get it -- spin around again (we'll
			 * probably sleep on the next iteration).
			 */
			continue;
		}

		if (RWLOCK_OWNER(rwl) == curproc)
			panic("rw_enter: locking against myself");

		ts = turnstile_lookup(rwl);

		/*
		 * Mark the rwlock as having waiters.  After we do
		 * this, we need to check one more time if the lock
		 * is busy, and if not, spin around again.
		 *
		 * Note, we also need to spin again if we failed to
		 * set the has-waiters indicator (which means the
		 * lock condition changed, but more importantly, we
		 * need to try and set that indicator again).
		 */
		RWLOCK_SET_WAITERS(rwl, need_wait, set_wait);
		owner = rwl->rwl_owner;
		if ((owner & need_wait) == 0 || (owner & set_wait) == 0) {
			turnstile_exit(rwl);
			continue;
		}
		/* XXXJRT p->p_priority */
		/* XXXJRT Do not currently distinguish reader vs. writer. */
		(void) turnstile_block(ts, TS_WRITER_Q, p->p_priority, rwl);

		/*
		 * XXX Solaris Internals says that the Solaris 7
		 * rwlock implementation does a direct-handoff.  We
		 * don't implement that yet, but if we did, then a
		 * thread wakes back up, i.e. arrives here, it would
		 * hold the lock as requested.
		 */
	}

	KASSERT((rw == RW_WRITER && RWLOCK_OWNER(rwl) == curproc) ||
		(rw == RW_READER && RWLOCK_COUNT(rwl) != 0));
}

/*
 * rw_tryenter:
 *
 *	Try to acquire a rwlock.
 */
int
rw_tryenter(krwlock_t *rwl, krw_t rw)
{
	unsigned long owner, incr, need_wait;

	switch (rw) {
	case RW_WRITER:
		incr = ((unsigned long) curproc) | RWLOCK_WRITE_LOCKED;
		need_wait = RWLOCK_WRITE_LOCKED;
		break;

	case RW_READER:
		incr = RWLOCK_READ_INCR;
		need_wait = RWLOCK_WRITE_LOCKED | RWLOCK_WRITE_WANTED;
		break;
#ifdef DIAGNOSTIC
	default:
		panic("rw_tryenter: bad rw %d", rw);
#endif
	}

	for (;;) {
		owner = rwl->rwl_owner;
		if ((owner & need_wait) == 0) {
			if (RWLOCK_ACQUIRE(rwl, owner, owner + incr)) {
				/* Got it! */
				break;
			}
			continue;
		}
		return (0);
	}

	KASSERT((rw == RW_WRITER && RWLOCK_OWNER(rwl) == curproc) ||
		(rw == RW_READER && RWLOCK_COUNT(rwl) != 0));
	return (1);
}

/*
 * rw_exit:
 *
 *	Release a rwlock.
 */
void
rw_exit(krwlock_t *rwl)
{
	struct turnstile *ts;
	unsigned long owner, decr, new;

	/*
	 * Again, we use a trick.  Since we used an add operation to
	 * set the required lock bits, we can use a subtract to clear
	 * them, which makes the read-release and write-release path
	 * the same.
	 */
	if (rwl->rwl_owner & RWLOCK_WRITE_LOCKED) {
		if (RWLOCK_OWNER(rwl) == NULL)
			panic("rw_exit: not owned");
		else
			panic("rw_exit: not owner, owner = %p, "
			    "current = %p", RWLOCK_OWNER(rwl), curproc);
		decr = ((unsigned long) curproc) | RWLOCK_WRITE_LOCKED;
	} else {
		if (RWLOCK_COUNT(rwl) == 0)
			panic("rw_exit: not held\n");
		decr = RWLOCK_READ_INCR;
	}

	for (;;) {
		/*
		 * Get this lock's turnstile.  This gets the interlock on
		 * the sleep queue.  Once we have that, we can perform the
		 * lock release operation.
		 */
		ts = turnstile_lookup(rwl);

		/*
		 * Compute what we expect the new value of the lock
		 * to be.  Skip the wakeup step if there are no
		 * appropriate waiters.
		 */
		owner = rwl->rwl_owner;
		new = owner - decr;
		if ((new & (RWLOCK_THREAD |
			    RWLOCK_HAS_WAITERS)) != RWLOCK_HAS_WAITERS) {
			if (RWLOCK_RELEASE(rwl, owner, new)) {
				/* Ding! */
				turnstile_exit(rwl);
				break;
			}
			turnstile_exit(rwl);
			continue;
		}

		/* We're about to wake everybody up; clear waiter bits. */
		new &= ~(RWLOCK_HAS_WAITERS | RWLOCK_WRITE_WANTED);

		if (RWLOCK_RELEASE(rwl, owner, new) == 0) {
			/* Oops, try again. */
			turnstile_exit(rwl);
			continue;
		}

		/*
		 * Wake the thundering herd.
		 * XXX Should implement direct-handoff.
		 */
		KASSERT(ts != NULL);
		turnstile_wakeup(ts, TS_WRITER_Q,
		    ts->ts_sleepq[TS_WRITER_Q].tsq_waiters, NULL);
		break;
	}
}

/*
 * rw_downgrade:
 *
 *	Downgrade a write lock to a read lock.
 */
void
rw_downgrade(krwlock_t *rwl)
{
	struct turnstile *ts;
	unsigned long owner;

	if (RWLOCK_OWNER(rwl) != curproc) {
		if (RWLOCK_OWNER(rwl) == NULL)
			panic("rw_downgrade: not owned");
		else
			panic("rw_downgrade: not owner, owner = %p, "
			    "current = %p", RWLOCK_OWNER(rwl), curproc);
	}

	/* XXX This algorithm has to change if we do direct-handoff. */
	for (;;) {
		ts = turnstile_lookup(rwl);

		owner = rwl->rwl_owner;
		if (RWLOCK_RELEASE(rwl, owner, RWLOCK_READ_INCR) == 0) {
			/* Oops, try again. */
			turnstile_exit(rwl);
			continue;
		}
		if (owner & RWLOCK_HAS_WAITERS) {
			KASSERT(ts != NULL);
			turnstile_wakeup(ts, TS_WRITER_Q,
			    ts->ts_sleepq[TS_WRITER_Q].tsq_waiters, NULL);
		}
		break;
	}

	KASSERT((rwl->rwl_owner & RWLOCK_WRITE_LOCKED) == 0);
	KASSERT(RWLOCK_COUNT(rwl) != 0);
}

/*
 * rw_tryupgrade:
 *
 *	Try to upgrade an read lock to a write lock.
 */
int
rw_tryupgrade(krwlock_t *rwl)
{
	unsigned long owner;

	KASSERT((rwl->rwl_owner & RWLOCK_WRITE_LOCKED) == 0);
	KASSERT(RWLOCK_COUNT(rwl) != 0);

	for (;;) {
		/*
		 * Since we want to favor writers, we don't bother
		 * checking for waiting writers, we just scarf it.
		 *
		 * We must be the only reader.
		 */
		owner = rwl->rwl_owner;
		if ((owner & RWLOCK_THREAD) != RWLOCK_READ_INCR)
			return (0);
		if (RWLOCK_ACQUIRE(rwl, owner,
		    ((unsigned long) curproc) | RWLOCK_WRITE_LOCKED |
		    (owner & ~RWLOCK_THREAD))) {
			/* Ding! */
			break;
		}
	}

	KASSERT(rwl->rwl_owner & RWLOCK_WRITE_LOCKED);
	KASSERT(RWLOCK_OWNER(rwl) == curproc);
	return (1);
}

/*
 * rw_read_held:
 *
 *	Returns true if the rwlock is held for reading.
 */
int
rw_read_held(krwlock_t *rwl)
{
	unsigned long owner = rwl->rwl_owner;

	return ((owner & RWLOCK_WRITE_LOCKED) == 0 &&
	    (owner & RWLOCK_THREAD) != 0);
}

/*
 * rw_write_held:
 *
 *	Returns true if the rwlock is held for writing.
 */
int
rw_write_held(krwlock_t *rwl)
{
	unsigned long owner = rwl->rwl_owner;

	return ((owner & RWLOCK_WRITE_LOCKED) != 0);
}

/*
 * rw_read_locked:
 *
 *	Like rw_read_held(), but asserts it.
 */
int
rw_read_locked(krwlock_t *rwl)
{
	int rv = rw_read_held(rwl);

#ifdef DIAGNOSTIC
	if (rv == 0)
		panic("rw_read_locked: not held");
#endif

	return (rv);
}

/*
 * rw_write_locked:
 *
 *	Like rw_write_held(), but asserts that we hold it.
 */
int
rw_write_locked(krwlock_t *rwl)
{
	int rv = rw_write_held(rwl);

#ifdef DIAGNOSTIC
	if (rv == 0)
		panic("rw_write_locked: not held");
	else if (RWLOCK_OWNER(rwl) != curproc)
		panic("rw_write_locked: not owner, owner = %p, "
		    "current = %p", RWLOCK_OWNER(rwl), curproc);
#endif

	return (rv);
}

/*
 * rw_owner:
 *
 *	Return the owner of the rwlock.
 */
struct proc *
rw_owner(krwlock_t *rwl)
{
	unsigned long owner = rwl->rwl_owner;

	return ((owner & RWLOCK_WRITE_LOCKED) ?
	    ((struct proc *) (owner & RWLOCK_THREAD)) : NULL);
}
