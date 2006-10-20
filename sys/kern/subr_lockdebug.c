/*	$NetBSD: subr_lockdebug.c,v 1.1.2.1 2006/10/20 19:34:29 ad Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * Basic lock debugging code shared among lock primatives.
 *
 * XXX malloc() may want to initialize new mutexes.  To be fixed.
 */

#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_lockdebug.c,v 1.1.2.1 2006/10/20 19:34:29 ad Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <sys/lockdebug.h>

#include <machine/cpu.h>

#ifdef LOCKDEBUG

#define	LD_BATCH_SHIFT	9
#define	LD_BATCH	(1 << LD_BATCH_SHIFT)
#define	LD_BATCH_MASK	(LD_BATCH - 1)
#define	LD_MAX_LOCKS	1048576
#define	LD_SLOP		16

#define	LD_LOCKED	0x01
#define	LD_SLEEPER	0x02

typedef struct lockdebuglk {
	__cpu_simple_lock_t	lk_lock;
	int			lk_oldspl;
} volatile lockdebuglk_t;

typedef struct lockdebug {
	_TAILQ_ENTRY(struct lockdebug, volatile) ld_chain;
	void		*ld_lock;
	lockops_t	*ld_lockops;
	struct lwp	*ld_lwp;
	uintptr_t	ld_locked;
	uintptr_t	ld_unlocked;
	u_int		ld_id;
	u_short		ld_cpu;
	u_short		ld_shares;
	u_char		ld_flags;
} volatile lockdebug_t;

typedef _TAILQ_HEAD(lockdebuglist, struct lockdebug, volatile) lockdebuglist_t;

lockdebuglk_t		ld_sleeper_lk;
lockdebuglk_t		ld_spinner_lk;
lockdebuglk_t		ld_free_lk;

lockdebuglist_t		ld_sleepers;
lockdebuglist_t		ld_spinners;
lockdebuglist_t		ld_free;
int			ld_nfree;
int			ld_freeptr;
int			ld_recurse;
lockdebug_t		*ld_table[LD_MAX_LOCKS / LD_BATCH];

lockdebug_t		ld_prime[LD_BATCH];

MALLOC_DEFINE(M_LOCKDEBUG, "lockdebug", "lockdebug structures");

void	lockdebug_abort1(lockdebug_t *, lockdebuglk_t *lk, const char *,
			 const char *);
void	lockdebug_more(void);

static inline void
lockdebug_lock(lockdebuglk_t *lk)
{
	int s;

	s = spllock();
	__cpu_simple_lock(&lk->lk_lock);
	lk->lk_oldspl = s;
}

static inline void
lockdebug_unlock(lockdebuglk_t *lk)
{
	int s;

	s = lk->lk_oldspl;
	__cpu_simple_unlock(&lk->lk_lock);
	splx(s);
}

/*
 * lockdebug_lookup:
 *
 *	Find a lockdebug structure by ID and return it locked.
 */
static inline lockdebug_t *
lockdebug_lookup(u_int id, lockdebuglk_t **lk)
{
	lockdebug_t *ld;

	ld = ld_table[id >> LD_BATCH_SHIFT] + (id & LD_BATCH_MASK);

	if (id == 0 || id >= LD_MAX_LOCKS || ld == NULL || ld->ld_lock == NULL)
		panic("lockdebug_lookup: uninitialized lock (id=%d)", id);

	if (ld->ld_id != id)
		panic("lockdebug_lookup: corrupt table");

	if ((ld->ld_flags & LD_SLEEPER) != 0)
		*lk = &ld_sleeper_lk;
	else
		*lk = &ld_spinner_lk;

	lockdebug_lock(*lk);
	return ld;
}

/*
 * lockdebug_init:
 *
 *	Initialize the lockdebug system.  Allocate an initial pool of
 *	lockdebug structures before the VM system is up and running.
 */
void
lockdebug_init(void)
{
	lockdebug_t *ld;
	int i;

	__cpu_simple_lock_init(&ld_sleeper_lk.lk_lock);
	__cpu_simple_lock_init(&ld_spinner_lk.lk_lock);
	__cpu_simple_lock_init(&ld_free_lk.lk_lock);

	TAILQ_INIT(&ld_free);
	TAILQ_INIT(&ld_sleepers);
	TAILQ_INIT(&ld_spinners);

	ld = ld_prime;
	ld_table[0] = ld;
	for (i = 1, ld++; i < LD_BATCH; i++, ld++) {
		ld->ld_id = i;
		TAILQ_INSERT_TAIL(&ld_free, ld, ld_chain);
	}
	ld_freeptr = 1;
	ld_nfree = LD_BATCH;
}

/*
 * lockdebug_alloc:
 *
 *	A lock is being initialized, so allocate an associated debug
 *	structure.
 */
u_int
lockdebug_alloc(void *lock, lockops_t *lo, int sleeplock)
{
	lockdebug_t *ld;

	if (panicstr != NULL)
		return 0;

	/* Pinch a new debug structure. */
	lockdebug_lock(&ld_free_lk);
	if (TAILQ_EMPTY(&ld_free))
		lockdebug_more();
	ld = TAILQ_FIRST(&ld_free);
	TAILQ_REMOVE(&ld_free, ld, ld_chain);
	ld_nfree--;
	lockdebug_unlock(&ld_free_lk);

	if (ld->ld_lock != NULL)
		panic("lockdebug_alloc: corrupt table");

	if (sleeplock)
		lockdebug_lock(&ld_sleeper_lk);
	else
		lockdebug_lock(&ld_spinner_lk);

	/* Initialise the structure. */
	ld->ld_lock = lock;
	ld->ld_lockops = lo;
	ld->ld_locked = 0;
	ld->ld_unlocked = 0;
	ld->ld_lwp = NULL;

	if (sleeplock) {
		ld->ld_flags = LD_SLEEPER;
		lockdebug_unlock(&ld_sleeper_lk);
	} else {
		ld->ld_flags = 0;
		lockdebug_unlock(&ld_spinner_lk);
	}

	return ld->ld_id;
}

/*
 * lockdebug_free:
 *
 *	A lock is being destroyed, so release debugging resources.
 */
void
lockdebug_free(void *lock, u_int id)
{
	lockdebug_t *ld;
	lockdebuglk_t *lk;

	if (panicstr != NULL)
		return;

	ld = lockdebug_lookup(id, &lk);

	if (ld->ld_lock != lock) {
		panic("lockdebug_free: destroying uninitialized lock %p"
		    "(ld_id=%d ld_lock=%p)", lock, id, ld->ld_lock);
		lockdebug_abort1(ld, lk, __FUNCTION__, "lock record follows");
	}
	if ((ld->ld_flags & LD_LOCKED) != 0)
		lockdebug_abort1(ld, lk, __FUNCTION__, "is locked");

	ld->ld_lock = NULL;

	lockdebug_unlock(lk);

	lockdebug_lock(&ld_free_lk);
	TAILQ_INSERT_TAIL(&ld_free, ld, ld_chain);
	ld_nfree++;
	lockdebug_unlock(&ld_free_lk);
}

/*
 * lockdebug_more:
 *
 *	Allocate a batch of debug structures and add to the free list.  Must
 *	be called with ld_free_lk held.
 */
void
lockdebug_more(void)
{
	lockdebug_t *ld;
	void *block;
	int i, base;

	while (TAILQ_EMPTY(&ld_free)) {
		lockdebug_unlock(&ld_free_lk);
		block = malloc(LD_BATCH * sizeof(lockdebug_t), M_LOCKDEBUG,
		    M_NOWAIT | M_ZERO); /* XXX M_NOWAIT */
		lockdebug_lock(&ld_free_lk);

		base = ld_freeptr;
		if (ld_table[base] != NULL) {
			/* Somebody beat us to it. */
			lockdebug_unlock(&ld_free_lk);
			free(block, M_LOCKDEBUG);
			lockdebug_lock(&ld_free_lk);
			continue;
		}
		ld_table[base] = block;
		ld_freeptr++;
		ld = block;
		base <<= LD_BATCH_SHIFT;

		for (i = 0; i < LD_BATCH; i++, ld++) {
			ld->ld_id = i + base;
			ld->ld_lock = NULL;
			TAILQ_INSERT_TAIL(&ld_free, ld, ld_chain);
		}

		ld_table[base] = ld;
		mb_write();
	}
}

/*
 * lockdebug_locked:
 *
 *	Process a lock acquire operation.
 */
void
lockdebug_locked(u_int id, uintptr_t where, int shared)
{
	struct lwp *l = curlwp;
	lockdebuglk_t *lk;
	lockdebug_t *ld;

	if (panicstr != NULL)
		return;

	ld = lockdebug_lookup(id, &lk);

	if ((ld->ld_flags & LD_LOCKED) != 0)
		lockdebug_abort1(ld, lk, __FUNCTION__, "already locked");

	if (shared) {
		if (l == NULL)
			lockdebug_abort1(ld, lk, __FUNCTION__, "releasing "
			    "shared lock from interrupt context");

		l->l_shlocks++;
		ld->ld_shares++;
	} else {
		ld->ld_flags |= LD_LOCKED;
		ld->ld_locked = where;
		ld->ld_cpu = (u_short)cpu_number();
		ld->ld_lwp = l;

		if ((ld->ld_flags & LD_SLEEPER) != 0) {
			l->l_exlocks++;
			TAILQ_INSERT_TAIL(&ld_sleepers, ld, ld_chain);
		} else {
			curcpu()->ci_spin_locks2++;
			TAILQ_INSERT_TAIL(&ld_spinners, ld, ld_chain);
		}
	}

	lockdebug_unlock(lk);
}

/*
 * lockdebug_unlocked:
 *
 *	Process a lock release operation.
 */
void
lockdebug_unlocked(u_int id, uintptr_t where, int shared)
{
	struct lwp *l = curlwp;
	lockdebuglk_t *lk;
	lockdebug_t *ld;

	if (panicstr != NULL)
		return;

	ld = lockdebug_lookup(id, &lk);

	if (shared) {
		if (l == NULL)
			lockdebug_abort1(ld, lk, __FUNCTION__, "acquiring "
			    "shared lock from interrupt context");
		if (l->l_shlocks == 0)
			lockdebug_abort1(ld, lk, __FUNCTION__, "no shared "
			    "locks held by LWP");
		if (ld->ld_shares == 0)
			lockdebug_abort1(ld, lk, __FUNCTION__, "no shared "
			    "holds on this lock");
		l->l_shlocks--;
		ld->ld_shares--;
	} else {
		if ((ld->ld_flags & LD_LOCKED) == 0)
			lockdebug_abort1(ld, lk, __FUNCTION__, "not locked");

		if ((ld->ld_flags & LD_SLEEPER) != 0) {
			if (ld->ld_lwp != curlwp)
				lockdebug_abort1(ld, lk, __FUNCTION__,
				    "not held by current LWP");
			ld->ld_flags &= ~LD_LOCKED;
			ld->ld_unlocked = where;
			ld->ld_lwp = NULL;
			curlwp->l_exlocks--;
			TAILQ_REMOVE(&ld_sleepers, ld, ld_chain);
		} else {
			if (ld->ld_cpu != (u_short)cpu_number())
				lockdebug_abort1(ld, lk, __FUNCTION__,
				    "not held by current CPU");
			ld->ld_flags &= ~LD_LOCKED;
			ld->ld_unlocked = where;		
			ld->ld_lwp = NULL;
			curcpu()->ci_spin_locks2--;
			TAILQ_REMOVE(&ld_spinners, ld, ld_chain);
		}
	}

	lockdebug_unlock(lk);
}

/*
 * lockdebug_barrier:
 *	
 *	Panic if we hold more than one specified spin lock, and optionally,
 *	if we hold sleep locks.
 */
void
lockdebug_barrier(void *spinlock, int slplocks)
{
	struct lwp *l = curlwp;
	lockdebug_t *ld;
	u_short cpuno;

	if (panicstr != NULL)
		return;

	if (curcpu()->ci_spin_locks2 != 0) {
		cpuno = (u_short)cpu_number();

		lockdebug_lock(&ld_spinner_lk);
		TAILQ_FOREACH(ld, &ld_spinners, ld_chain) {
			if (ld->ld_lock == spinlock) {
				if (ld->ld_cpu != cpuno)
					lockdebug_abort1(ld, &ld_spinner_lk,
					    __FUNCTION__,
					    "not held by current CPU");
				continue;
			}
			if (ld->ld_cpu == cpuno)
				lockdebug_abort1(ld, &ld_spinner_lk,
				    __FUNCTION__, "spin lock held");
		}
		lockdebug_unlock(&ld_spinner_lk);
	}

	if (!slplocks) {
		if (l->l_exlocks != 0) {
			lockdebug_lock(&ld_sleeper_lk);
			TAILQ_FOREACH(ld, &ld_sleepers, ld_chain) {
				if (ld->ld_lwp == l)
					lockdebug_abort1(ld, &ld_sleeper_lk,
					    __FUNCTION__, "sleep lock held");
			}
			lockdebug_unlock(&ld_sleeper_lk);
		}
		if (l->l_shlocks != 0)
			panic("lockdebug_barrier: holding %d shared locks",
			    l->l_shlocks);
	}
}

/*
 * lockdebug_abort:
 *
 *	An error has been trapped - dump lock info call panic().
 */
void
lockdebug_abort(int id, void *lock, lockops_t *ops, const char *func,
		const char *msg)
{
	lockdebug_t *ld;
	lockdebuglk_t *lk;

	(void)lock;
	(void)ops;

	ld = lockdebug_lookup(id, &lk);

	lockdebug_abort1(ld, lk, func, msg);
}

void
lockdebug_abort1(lockdebug_t *ld, lockdebuglk_t *lk, const char *func,
		 const char *msg)
{
	static char buf[1024];
	int p;

	/*
	 * The kernel is about to fall flat on its face, so assume that 1k
	 * will be enough to hold the dump and abuse the return value from
	 * snprintf.
	 */
	p = snprintf(buf, sizeof(buf), "%s error: %s: %s\n",
	    ld->ld_lockops->lo_name, func, msg);

	p += snprintf(buf + p, sizeof(buf) - p,
	    "lock address : %#018lx type     : %18s\n"
	    "shared holds : %18d exclusive: %12slocked\n"
	    "last locked  : %#018lx unlocked : %#018lx\n"
	    "current cpu  : %18d last held: %18d\n"
	    "current lwp  : %#018lx last held: %#018lx\n",
	    (long)ld->ld_lock,
	    ((ld->ld_flags & LD_SLEEPER) == 0 ? "spin" : "sleep"),
	    ld->ld_shares, ((ld->ld_flags & LD_LOCKED) == 0 ? "un" : " "),
	    (long)ld->ld_locked, (long)ld->ld_unlocked,
	    (int)cpu_number(), (int)ld->ld_cpu,
	    (long)curlwp, (long)ld->ld_lwp);

	(void)(*ld->ld_lockops->lo_dump)(ld->ld_lock, buf + p, sizeof(buf) - p);

	lockdebug_unlock(lk);

	printf("%s", buf);
	panic("LOCKDEBUG");
}

#else	/* LOCKDEBUG */

/*
 * lockdebug_abort:
 *
 *	An error has been trapped - dump lock info and call panic().
 *	Called in the non-LOCKDEBUG case to print basic information.
 */
void
lockdebug_abort(int id, void *lock, lockops_t *ops, const char *func,
		const char *msg)
{
	static char buf[1024];
	int p;

	/*
	 * The kernel is about to fall flat on its face, so assume that 1k
	 * will be enough to hold the dump and abuse the return value from
	 * snprintf.
	 */
	p = snprintf(buf, sizeof(buf), "Lock error: %s: %s\n",
	    ops->lo_name, func, msg);

	p += snprintf(buf + p, sizeof(buf) - p,
	    "lock address : %#018lx\n"
	    "current cpu  : %18d\n"
	    "current lwp  : %#018lx\n",
	    (long)lock, (int)cpu_number, (long)curlwp);

	(void)(*ld->ld_lockops->lo_dump)(ld->ld_lock, buf + p, sizeof(buf) - p);

	printf("%s", buf);
	panic("lock error");
}

#endif	/* LOCKDEBUG */
