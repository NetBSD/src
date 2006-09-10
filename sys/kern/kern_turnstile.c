/*	$NetBSD: kern_turnstile.c,v 1.1.36.1 2006/09/10 23:42:42 ad Exp $	*/

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
 * Turnsiles are specialized sleep queues for use by locks.  Turnstiles
 * are described in detail in:
 *
 *	Solaris Internals: Core Kernel Architecture, Jim Mauro and
 *	    Richard McDougall.
 *
 * Turnstiles are kept in a hash table.  There are likely to be many more
 * lock objects than there are threads.  Since a thread can block on only
 * one lock at a time, we only need one turnstile per thread, and so they
 * are allocated at thread creation time.
 *
 * When a thread decides it needs to block on a lock, it looks up the
 * active turnstile for that lock.  If no active turnstile exists, then
 * the process lends its turnstile to the lock.  If there is already
 * an active turnstile for the lock, the thread places its turnstile on
 * a list of free turnstiles, and references the active one instead.
 *
 * The act of looking up the turnstile acquires an interlock on the sleep
 * queue.  If a thread decides it doesn't need to block after all, then
 * this interlock must be released by explicitly aborting the turnstile
 * operation.
 *
 * When a thread is awakened, it needs to get its turnstile back.  If
 * there are still other threads waiting in the active turnstile, the
 * the thread grabs a free turnstile off the free list.  Otherwise, it
 * can take back the active turnstile from the lock (thus deactivating
 * the turnstile).
 *
 * Turnstiles are the place to do priority inheritence.  However, we do
 * not currently implement that.
 *
 * XXX We currently have to interlock with the sched_lock.  The locking
 * order is:
 *
 *	turnstile chain -> sched_lock
 */

#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_turnstile.c,v 1.1.36.1 2006/09/10 23:42:42 ad Exp $");

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/pool.h>
#include <sys/proc.h> 
#include <sys/resourcevar.h>
#include <sys/sched.h>
#include <sys/turnstile.h>
#include <sys/systm.h>
#include <sys/sa.h>
#include <sys/savar.h>

/* XXX */
#ifdef MULTIPROCESSOR
#define	_SCHED_LOCK	simple_lock(&sched_lock);
#define	_SCHED_UNLOCK	simple_unlock(&sched_lock);
#else
#define	_SCHED_LOCK	/* nothing */
#define	_SCHED_UNLOCK	/* nothing */
#endif

/*
 * Turnstile hash -- shift the lock object to eliminate the zero bits
 * of the address, and mask it off with the turnstile table's size.
 */
#if LONG_BIT == 64
#define	TURNSTILE_HASH_SHIFT	3
#elif LONG_BIT == 32
#define	TURNSTILE_HASH_SHIFT	2
#else
#error "Don't know how big your pointers are."
#endif

#define	TURNSTILE_HASH_SIZE	64
#define	TURNSTILE_HASH_MASK	(TURNSTILE_HASH_SIZE - 1)

#define	TURNSTILE_HASH(obj)						\
	((((u_long)(obj)) >> TURNSTILE_HASH_SHIFT) & TURNSTILE_HASH_MASK)

struct turnstile_chain {
	struct simplelock tc_lock;	/* lock on hash chain */
	int		tc_oldspl;	/* saved spl of lock holder
					   (only valid while tc_lock held) */
	LIST_HEAD(, turnstile) tc_chain;/* turnstile chain */
} turnstile_table[TURNSTILE_HASH_SIZE];

#define	TURNSTILE_CHAIN(obj)						\
	&turnstile_table[TURNSTILE_HASH(obj)]

#define	TURNSTILE_CHAIN_LOCK(tc)					\
do {									\
	int _s_ = spllock();						\
	simple_lock(&(tc)->tc_lock);					\
	(tc)->tc_oldspl = _s_;						\
} while (/*CONSTCOND*/0)

#define	TURNSTILE_CHAIN_UNLOCK(tc)					\
do {									\
	int _s_ = (tc)->tc_oldspl;					\
	simple_unlock(&(tc)->tc_lock);					\
	splx(_s_);							\
} while (/*CONSTCOND*/0)

struct pool turnstile_pool;
struct pool_cache turnstile_cache;

int	turnstile_ctor(void *, void *, int);

/*
 * turnstile_init:
 *
 *	Initialize the turnstile mechanism.
 */
void
turnstile_init(void)
{
	struct turnstile_chain *tc;
	int i;

	for (i = 0; i < TURNSTILE_HASH_SIZE; i++) {
		tc = &turnstile_table[i];
		simple_lock_init(&tc->tc_lock);
		LIST_INIT(&tc->tc_chain);
	}

	pool_init(&turnstile_pool, sizeof(struct turnstile), 0, 0, 0,
	    "tspool", &pool_allocator_nointr);
	pool_cache_init(&turnstile_cache, &turnstile_pool,
	    turnstile_ctor, NULL, NULL);
}

/*
 * turnstile_ctor:
 *
 *	Constructor for turnstiles.
 */
int
turnstile_ctor(void *arg, void *obj, int flags)
{
	struct turnstile *ts = obj;

	memset(ts, 0, sizeof(*ts));
	return (0);
}

static void
turnstile_remque(struct turnstile *ts, struct lwp *l,
		 struct turnstile_sleepq *tsq)
{
	struct lwp **q = &tsq->tsq_q.sq_head;
	struct turnstile *nts;

	KASSERT(l->l_ts == ts);

	/*
	 * This process is no longer using the active turnstile.
	 * Find an inactive one on the free list to give to it.
	 */
	if ((nts = ts->ts_free) != NULL) {
		KASSERT(TS_ALL_WAITERS(ts) > 1);
		l->l_ts = nts;
		ts->ts_free = nts->ts_free;
		nts->ts_free = NULL;
	} else {
		/*
		 * If the free list is empty, this is the last
		 * waiter.
		 */
		KASSERT(TS_ALL_WAITERS(ts) == 1);
		LIST_REMOVE(ts, ts_chain);
	}

	tsq->tsq_waiters--;

	*q = l->l_forw;
	if (tsq->tsq_q.sq_tailp == &l->l_forw)
		tsq->tsq_q.sq_tailp = q;

	KASSERT(ts->ts_sleepq[TS_READER_Q].tsq_waiters != 0 ||
		ts->ts_sleepq[TS_READER_Q].tsq_q.sq_head == NULL);
	KASSERT(ts->ts_sleepq[TS_WRITER_Q].tsq_waiters != 0 ||
		ts->ts_sleepq[TS_WRITER_Q].tsq_q.sq_head == NULL);

	KASSERT(ts->ts_sleepq[TS_READER_Q].tsq_waiters == 0 ||
		ts->ts_sleepq[TS_READER_Q].tsq_q.sq_head != NULL);
	KASSERT(ts->ts_sleepq[TS_WRITER_Q].tsq_waiters == 0 ||
		ts->ts_sleepq[TS_WRITER_Q].tsq_q.sq_head != NULL);
}

/*
 * turnstile_lookup:
 *
 *	Look up the turnstile for the specified lock object.  This
 *	acquires and holds the turnstile chain lock (sleep queue
 *	interlock).
 */
struct turnstile *
turnstile_lookup(void *lp)
{
	struct turnstile_chain *tc = TURNSTILE_CHAIN(lp);
	struct turnstile *ts;

	TURNSTILE_CHAIN_LOCK(tc);

	LIST_FOREACH(ts, &tc->tc_chain, ts_chain)
		if (ts->ts_obj == lp)
			return (ts);

	/*
	 * No turnstile yet for this lock.  No problem, turnstile_block()
	 * handles this by fetching the turnstile from the blocking thread.
	 */
	return (NULL);
}

/*
 * turnstile_exit:
 *
 *	Abort a turnstile operation.
 */
void
turnstile_exit(void *lp)
{
	struct turnstile_chain *tc = TURNSTILE_CHAIN(lp);

	TURNSTILE_CHAIN_UNLOCK(tc);
}

/*
 * turnstile_block:
 *
 *	Block a thread on a lock object.
 */
int
turnstile_block(struct turnstile *ts, int rw, int pri, void *lp,
		const char *wmesg)
{
	struct turnstile_chain *tc = TURNSTILE_CHAIN(lp);
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct turnstile *ots;
	struct turnstile_sleepq *tsq;
	struct sadata_upcall *sau;
	struct slpque *qp;
	int s, hold_count;

	KASSERT(l->l_ts != NULL);
	KASSERT(rw == TS_READER_Q || rw == TS_WRITER_Q);

	if (ts == NULL) {
		/*
		 * We are the first thread to wait for this lock;
		 * lend our turnstile to it.
		 */
		ts = l->l_ts;
		KASSERT(TS_ALL_WAITERS(ts) == 0);
		KASSERT(ts->ts_sleepq[TS_READER_Q].tsq_q.sq_head == NULL &&
			ts->ts_sleepq[TS_WRITER_Q].tsq_q.sq_head == NULL);
		ts->ts_obj = lp;
		LIST_INSERT_HEAD(&tc->tc_chain, ts, ts_chain);
	} else {
		/*
		 * Lock already has a turnstile.  Put our turnstile
		 * onto the free list, and reference the existing
		 * turnstile instead.
		 */
		ots = l->l_ts;
		ots->ts_free = ts->ts_free;
		ts->ts_free = ots;
		l->l_ts = ts;
	}

#ifdef DIAGNOSTIC
	if (l->l_stat != LSONPROC)
		panic("turnstile_block: l_stat %d != LSONPROC", l->l_stat);
	if (l->l_back != NULL)
		panic("turnstile_block: l_back != NULL");
#endif

#ifdef KTRACE
	if (KTRPOINT(p, KTR_CSW))
		ktrcsw(l, 1, 0);
#endif

	/*
	 * XXX We need to allocate the sadata_upcall structure here,
	 * XXX since we can't sleep while waiting for memory inside
	 * XXX sa_upcall().  It would be nice if we could safely
	 * XXX allocate the sadata_upcall structure on the stack, here.
	 */
	if ((l->l_flag & L_SA) != 0)
		sau = sadata_upcall_alloc(0);
	else
		sau = NULL;

	l->l_wchan = lp;
	l->l_wmesg = wmesg;
	l->l_slptime = 0;
	l->l_priority = pri & PRIMASK;

	tsq = &ts->ts_sleepq[rw];
	qp = &tsq->tsq_q;
	tsq->tsq_waiters++;
	if (qp->sq_head == NULL)
		qp->sq_head = l;
	else
		*qp->sq_tailp = l;
	*(qp->sq_tailp = &l->l_forw) = NULL;

	/*
	 * XXX We currently need to interlock with sched_lock.
	 * Note we're already at spllock(), which will block
	 * scheduler interrupts.
	 */
	_SCHED_LOCK;

	/*
	 * Release the kernel_lock, as we are about to yield the CPU.
	 * The scheduler lock is still held until cpu_switch()
	 * selects a new process and removes it from the run queue.
	 */
	hold_count = KERNEL_LOCK_RELEASE_ALL();

	l->l_stat = LSSLEEP;
	p->p_nrlwps--;
	p->p_stats->p_ru.ru_nvcsw++;

	/*
	 * We can now release the turnstile chain interlock; the
	 * scheduler lock is held, so a thread can't get in to
	 * do a turnstile_wakeup() before we do the switch.
	 *
	 * Note: we need to remember our old spl which is currently
	 * stored in the turnstile chain, because we have to stay
	 * st spllock while the sched_lock is held.
	 */
	s = tc->tc_oldspl;
	simple_unlock(&tc->tc_lock);

	if ((l->l_flag & L_SA) != 0)
		sa_switch(l, sau, SA_UPCALL_BLOCKED);
	else
		mi_switch(l, NULL);

	SCHED_ASSERT_UNLOCKED();
	splx(s);

	/*
	 * We are now back to the base spl level we were at when the
	 * caller called turnstile_lookup().
	 */
	KDASSERT(l->l_cpu != NULL);
	KDASSERT(l->l_cpu == curcpu());
	l->l_cpu->ci_schedstate.spc_curpriority = l->l_usrpri;

	/*
	 * Reacquire the kernel_lock now.  We do this after we've
	 * released the scheduler lock to avoid deadlock, and before
	 * we reacquire other locks.
	 */
	KERNEL_LOCK_ACQUIRE_COUNT(hold_count);

	KDASSERT((p->p_flag & (L_SINTR|L_TIMEOUT)) == 0);

#ifdef KTRACE
	if (KTRPOINT(p, KTR_CSW))
		ktrcsw(l, 0, 0);
#endif

	return (0);
}

/*
 * turnstile_wakeup:
 *
 *	Wake up the specified number of threads that are blocked
 *	in a turnstile.
 */
void
turnstile_wakeup(struct turnstile *ts, int rw, int count,
		 struct lwp *nextlwp)
{
	struct turnstile_chain *tc = TURNSTILE_CHAIN(ts->ts_obj);
	struct turnstile_sleepq *tsq;
	struct lwp *l;

	KASSERT(rw == TS_READER_Q || rw == TS_WRITER_Q);
	KASSERT(count > 0);

	tsq = &ts->ts_sleepq[rw];

	/* XXX We currently interlock with sched_lock. */
	_SCHED_LOCK;

	if (nextlwp != NULL) {
#if defined(DEBUG) || defined(LOCKDEBUG)
		for (l = tsq->tsq_q.sq_head; l != NULL;
		     l = l->l_forw) {
			if (l == nextlwp)
				break;
		}
		if (l == NULL)
			panic("turnstile_wakeup: nextlwp not on sleepq");
#endif
		turnstile_remque(ts, nextlwp, tsq);
		nextlwp->l_wchan = NULL;
		if (nextlwp->l_stat == LSSLEEP)
			awaken(nextlwp);
	} else {
		while (count-- > 0) {
			l = tsq->tsq_q.sq_head;
			KASSERT(l != NULL);
			turnstile_remque(ts, l, tsq);
			l->l_wchan = NULL;
			if (l->l_stat == LSSLEEP)
				awaken(l);
		}
	}

	_SCHED_UNLOCK;

	TURNSTILE_CHAIN_UNLOCK(tc);
}

