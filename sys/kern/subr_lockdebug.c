/*	$NetBSD: subr_lockdebug.c,v 1.8 2007/06/15 20:17:08 ad Exp $	*/

/*-
 * Copyright (c) 2006, 2007 The NetBSD Foundation, Inc.
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
 */

#include "opt_multiprocessor.h"
#include "opt_ddb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_lockdebug.c,v 1.8 2007/06/15 20:17:08 ad Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/lock.h>
#include <sys/lockdebug.h>
#include <sys/sleepq.h>

#include <machine/cpu.h>

#ifdef LOCKDEBUG

#define	LD_BATCH_SHIFT	9
#define	LD_BATCH	(1 << LD_BATCH_SHIFT)
#define	LD_BATCH_MASK	(LD_BATCH - 1)
#define	LD_MAX_LOCKS	1048576
#define	LD_SLOP		16

#define	LD_LOCKED	0x01
#define	LD_SLEEPER	0x02

#define	LD_NOID		(LD_MAX_LOCKS + 1)

typedef union lockdebuglk {
	struct {
		__cpu_simple_lock_t	lku_lock;
		int			lku_oldspl;
	} ul;
	uint8_t	lk_pad[64];
} volatile __aligned(64) lockdebuglk_t;

#define	lk_lock		ul.lku_lock
#define	lk_oldspl	ul.lku_oldspl

typedef struct lockdebug {
	_TAILQ_ENTRY(struct lockdebug, volatile) ld_chain;
	_TAILQ_ENTRY(struct lockdebug, volatile) ld_achain;
	volatile void	*ld_lock;
	lockops_t	*ld_lockops;
	struct lwp	*ld_lwp;
	uintptr_t	ld_locked;
	uintptr_t	ld_unlocked;
	u_int		ld_id;
	uint16_t	ld_shares;
	uint16_t	ld_cpu;
	uint8_t		ld_flags;
	uint8_t		ld_shwant;	/* advisory */
	uint8_t		ld_exwant;	/* advisory */
	uint8_t		ld_unused;
} volatile lockdebug_t;

typedef _TAILQ_HEAD(lockdebuglist, struct lockdebug, volatile) lockdebuglist_t;

lockdebuglk_t		ld_sleeper_lk;
lockdebuglk_t		ld_spinner_lk;
lockdebuglk_t		ld_free_lk;

lockdebuglist_t		ld_sleepers;
lockdebuglist_t		ld_spinners;
lockdebuglist_t		ld_free;
lockdebuglist_t		ld_all;
int			ld_nfree;
int			ld_freeptr;
int			ld_recurse;
bool			ld_nomore;
lockdebug_t		*ld_table[LD_MAX_LOCKS / LD_BATCH];

lockdebug_t		ld_prime[LD_BATCH];

static void	lockdebug_abort1(lockdebug_t *, lockdebuglk_t *lk,
				 const char *, const char *);
static void	lockdebug_more(void);
static void	lockdebug_init(void);

static inline void
lockdebug_lock(lockdebuglk_t *lk)
{
	int s;
	
	s = splhigh();
	__cpu_simple_lock(&lk->lk_lock);
	lk->lk_oldspl = s;
}

static inline void
lockdebug_unlock(lockdebuglk_t *lk)
{
	int s;

	s = lk->lk_oldspl;
	__cpu_simple_unlock(&(lk->lk_lock));
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
	lockdebug_t *base, *ld;

	if (id == LD_NOID)
		return NULL;

	if (id == 0 || id >= LD_MAX_LOCKS)
		panic("lockdebug_lookup: uninitialized lock (1, id=%d)", id);

	base = ld_table[id >> LD_BATCH_SHIFT];
	ld = base + (id & LD_BATCH_MASK);

	if (base == NULL || ld->ld_lock == NULL || ld->ld_id != id)
		panic("lockdebug_lookup: uninitialized lock (2, id=%d)", id);

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
static void
lockdebug_init(void)
{
	lockdebug_t *ld;
	int i;

	__cpu_simple_lock_init(&ld_sleeper_lk.lk_lock);
	__cpu_simple_lock_init(&ld_spinner_lk.lk_lock);
	__cpu_simple_lock_init(&ld_free_lk.lk_lock);

	TAILQ_INIT(&ld_free);
	TAILQ_INIT(&ld_all);
	TAILQ_INIT(&ld_sleepers);
	TAILQ_INIT(&ld_spinners);

	ld = ld_prime;
	ld_table[0] = ld;
	for (i = 1, ld++; i < LD_BATCH; i++, ld++) {
		ld->ld_id = i;
		TAILQ_INSERT_TAIL(&ld_free, ld, ld_chain);
		TAILQ_INSERT_TAIL(&ld_all, ld, ld_achain);
	}
	ld_freeptr = 1;
	ld_nfree = LD_BATCH - 1;
}

/*
 * lockdebug_alloc:
 *
 *	A lock is being initialized, so allocate an associated debug
 *	structure.
 */
u_int
lockdebug_alloc(volatile void *lock, lockops_t *lo)
{
	struct cpu_info *ci;
	lockdebug_t *ld;

	if (lo == NULL || panicstr != NULL)
		return LD_NOID;
	if (ld_freeptr == 0)
		lockdebug_init();

	ci = curcpu();

	/*
	 * Pinch a new debug structure.  We may recurse because we call
	 * kmem_alloc(), which may need to initialize new locks somewhere
	 * down the path.  If not recursing, we try to maintain at least
	 * LD_SLOP structures free, which should hopefully be enough to
	 * satisfy kmem_alloc().  If we can't provide a structure, not to
	 * worry: we'll just mark the lock as not having an ID.
	 */
	lockdebug_lock(&ld_free_lk);
	ci->ci_lkdebug_recurse++;

	if (TAILQ_EMPTY(&ld_free)) {
		if (ci->ci_lkdebug_recurse > 1 || ld_nomore) {
			ci->ci_lkdebug_recurse--;
			lockdebug_unlock(&ld_free_lk);
			return LD_NOID;
		}
		lockdebug_more();
	} else if (ci->ci_lkdebug_recurse == 1 && ld_nfree < LD_SLOP)
		lockdebug_more();

	if ((ld = TAILQ_FIRST(&ld_free)) == NULL) {
		lockdebug_unlock(&ld_free_lk);
		return LD_NOID;
	}

	TAILQ_REMOVE(&ld_free, ld, ld_chain);
	ld_nfree--;

	ci->ci_lkdebug_recurse--;
	lockdebug_unlock(&ld_free_lk);

	if (ld->ld_lock != NULL)
		panic("lockdebug_alloc: corrupt table");

	if (lo->lo_sleeplock)
		lockdebug_lock(&ld_sleeper_lk);
	else
		lockdebug_lock(&ld_spinner_lk);

	/* Initialise the structure. */
	ld->ld_lock = lock;
	ld->ld_lockops = lo;
	ld->ld_locked = 0;
	ld->ld_unlocked = 0;
	ld->ld_lwp = NULL;

	if (lo->lo_sleeplock) {
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
lockdebug_free(volatile void *lock, u_int id)
{
	lockdebug_t *ld;
	lockdebuglk_t *lk;

	if (panicstr != NULL)
		return;

	if ((ld = lockdebug_lookup(id, &lk)) == NULL)
		return;

	if (ld->ld_lock != lock) {
		panic("lockdebug_free: destroying uninitialized lock %p"
		    "(ld_id=%d ld_lock=%p)", lock, id, ld->ld_lock);
		lockdebug_abort1(ld, lk, __func__, "lock record follows");
	}
	if ((ld->ld_flags & LD_LOCKED) != 0 || ld->ld_shares != 0)
		lockdebug_abort1(ld, lk, __func__, "is locked");

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
 *	Allocate a batch of debug structures and add to the free list.
 *	Must be called with ld_free_lk held.
 */
static void
lockdebug_more(void)
{
	lockdebug_t *ld;
	void *block;
	int i, base, m;

	while (ld_nfree < LD_SLOP) {
		lockdebug_unlock(&ld_free_lk);
		block = kmem_zalloc(LD_BATCH * sizeof(lockdebug_t), KM_SLEEP);
		lockdebug_lock(&ld_free_lk);

		if (block == NULL)
			return;

		if (ld_nfree > LD_SLOP) {
			/* Somebody beat us to it. */
			lockdebug_unlock(&ld_free_lk);
			kmem_free(block, LD_BATCH * sizeof(lockdebug_t));
			lockdebug_lock(&ld_free_lk);
			continue;
		}

		base = ld_freeptr;
		ld_nfree += LD_BATCH;
		ld = block;
		base <<= LD_BATCH_SHIFT;
		m = min(LD_MAX_LOCKS, base + LD_BATCH);

		if (m == LD_MAX_LOCKS)
			ld_nomore = true;

		for (i = base; i < m; i++, ld++) {
			ld->ld_id = i;
			TAILQ_INSERT_TAIL(&ld_free, ld, ld_chain);
			TAILQ_INSERT_TAIL(&ld_all, ld, ld_achain);
		}

		mb_write();
		ld_table[ld_freeptr++] = block;
	}
}

/*
 * lockdebug_wantlock:
 *
 *	Process the preamble to a lock acquire.
 */
void
lockdebug_wantlock(u_int id, uintptr_t where, int shared)
{
	struct lwp *l = curlwp;
	lockdebuglk_t *lk;
	lockdebug_t *ld;
	bool recurse;

	(void)shared;
	recurse = false;

	if (panicstr != NULL)
		return;

	if ((ld = lockdebug_lookup(id, &lk)) == NULL)
		return;

	if ((ld->ld_flags & LD_LOCKED) != 0) {
		if ((ld->ld_flags & LD_SLEEPER) != 0) {
			if (ld->ld_lwp == l)
				recurse = true;
		} else if (ld->ld_cpu == (uint16_t)cpu_number())
			recurse = true;
	}

	if (shared)
		ld->ld_shwant++;
	else
		ld->ld_exwant++;

	if (recurse)
		lockdebug_abort1(ld, lk, __func__, "locking against myself");

	lockdebug_unlock(lk);
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

	if ((ld = lockdebug_lookup(id, &lk)) == NULL)
		return;

	if (shared) {
		l->l_shlocks++;
		ld->ld_shares++;
		ld->ld_shwant--;
	} else {
		if ((ld->ld_flags & LD_LOCKED) != 0)
			lockdebug_abort1(ld, lk, __func__,
			    "already locked");

		ld->ld_flags |= LD_LOCKED;
		ld->ld_locked = where;
		ld->ld_cpu = (uint16_t)cpu_number();
		ld->ld_lwp = l;
		ld->ld_exwant--;

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

	if ((ld = lockdebug_lookup(id, &lk)) == NULL)
		return;

	if (shared) {
		if (l->l_shlocks == 0)
			lockdebug_abort1(ld, lk, __func__,
			    "no shared locks held by LWP");
		if (ld->ld_shares == 0)
			lockdebug_abort1(ld, lk, __func__,
			    "no shared holds on this lock");
		l->l_shlocks--;
		ld->ld_shares--;
	} else {
		if ((ld->ld_flags & LD_LOCKED) == 0)
			lockdebug_abort1(ld, lk, __func__, "not locked");

		if ((ld->ld_flags & LD_SLEEPER) != 0) {
			if (ld->ld_lwp != curlwp)
				lockdebug_abort1(ld, lk, __func__,
				    "not held by current LWP");
			ld->ld_flags &= ~LD_LOCKED;
			ld->ld_unlocked = where;
			ld->ld_lwp = NULL;
			curlwp->l_exlocks--;
			TAILQ_REMOVE(&ld_sleepers, ld, ld_chain);
		} else {
			if (ld->ld_cpu != (uint16_t)cpu_number())
				lockdebug_abort1(ld, lk, __func__,
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
lockdebug_barrier(volatile void *spinlock, int slplocks)
{
	struct lwp *l = curlwp;
	lockdebug_t *ld;
	uint16_t cpuno;

	if (panicstr != NULL)
		return;

	if (curcpu()->ci_spin_locks2 != 0) {
		cpuno = (uint16_t)cpu_number();

		lockdebug_lock(&ld_spinner_lk);
		TAILQ_FOREACH(ld, &ld_spinners, ld_chain) {
			if (ld->ld_lock == spinlock) {
				if (ld->ld_cpu != cpuno)
					lockdebug_abort1(ld, &ld_spinner_lk,
					    __func__,
					    "not held by current CPU");
				continue;
			}
			if (ld->ld_cpu == cpuno)
				lockdebug_abort1(ld, &ld_spinner_lk,
				    __func__, "spin lock held");
		}
		lockdebug_unlock(&ld_spinner_lk);
	}

	if (!slplocks) {
		if (l->l_exlocks != 0) {
			lockdebug_lock(&ld_sleeper_lk);
			TAILQ_FOREACH(ld, &ld_sleepers, ld_chain) {
				if (ld->ld_lwp == l)
					lockdebug_abort1(ld, &ld_sleeper_lk,
					    __func__, "sleep lock held");
			}
			lockdebug_unlock(&ld_sleeper_lk);
		}
		if (l->l_shlocks != 0)
			panic("lockdebug_barrier: holding %d shared locks",
			    l->l_shlocks);
	}
}

/*
 * lockdebug_dump:
 *
 *	Dump information about a lock on panic, or for DDB.
 */
static void
lockdebug_dump(lockdebug_t *ld, void (*pr)(const char *, ...))
{
	int sleeper = (ld->ld_flags & LD_SLEEPER);

	(*pr)(
	    "lock address : %#018lx type     : %18s\n"
	    "shared holds : %18u exclusive: %18u\n"
	    "shares wanted: %18u exclusive: %18u\n"
	    "current cpu  : %18u last held: %18u\n"
	    "current lwp  : %#018lx last held: %#018lx\n"
	    "last locked  : %#018lx unlocked : %#018lx\n",
	    (long)ld->ld_lock, (sleeper ? "sleep/adaptive" : "spin"),
	    (unsigned)ld->ld_shares, ((ld->ld_flags & LD_LOCKED) != 0),
	    (unsigned)ld->ld_shwant, (unsigned)ld->ld_exwant,
	    (unsigned)cpu_number(), (unsigned)ld->ld_cpu,
	    (long)curlwp, (long)ld->ld_lwp,
	    (long)ld->ld_locked, (long)ld->ld_unlocked);

	if (ld->ld_lockops->lo_dump != NULL)
		(*ld->ld_lockops->lo_dump)(ld->ld_lock);

	if (sleeper) {
		(*pr)("\n");
		turnstile_print(ld->ld_lock, pr);
	}
}

/*
 * lockdebug_dump:
 *
 *	Dump information about a known lock.
 */
static void
lockdebug_abort1(lockdebug_t *ld, lockdebuglk_t *lk, const char *func,
		 const char *msg)
{

	printf_nolog("%s error: %s: %s\n\n", ld->ld_lockops->lo_name,
	    func, msg);
	lockdebug_dump(ld, printf_nolog);
	lockdebug_unlock(lk);
	printf_nolog("\n");
	panic("LOCKDEBUG");
}

#endif	/* LOCKDEBUG */

/*
 * lockdebug_lock_print:
 *
 *	Handle the DDB 'show lock' command.
 */
#ifdef DDB
void
lockdebug_lock_print(void *addr, void (*pr)(const char *, ...))
{
#ifdef LOCKDEBUG
	lockdebug_t *ld;

	TAILQ_FOREACH(ld, &ld_all, ld_achain) {
		if (ld->ld_lock == addr) {
			lockdebug_dump(ld, pr);
			return;
		}
	}
	(*pr)("Sorry, no record of a lock with address %p found.\n", addr);
#else
	(*pr)("Sorry, kernel not built with the LOCKDEBUG option.\n");
#endif	/* LOCKDEBUG */
}
#endif	/* DDB */

/*
 * lockdebug_abort:
 *
 *	An error has been trapped - dump lock info and call panic().
 */
void
lockdebug_abort(u_int id, volatile void *lock, lockops_t *ops,
		const char *func, const char *msg)
{
#ifdef LOCKDEBUG
	lockdebug_t *ld;
	lockdebuglk_t *lk;

	if ((ld = lockdebug_lookup(id, &lk)) != NULL) {
		lockdebug_abort1(ld, lk, func, msg);
		/* NOTREACHED */
	}
#endif	/* LOCKDEBUG */

	printf_nolog("%s error: %s: %s\n\n"
	    "lock address : %#018lx\n"
	    "current cpu  : %18d\n"
	    "current lwp  : %#018lx\n",
	    ops->lo_name, func, msg, (long)lock, (int)cpu_number(),
	    (long)curlwp);

	(*ops->lo_dump)(lock);

	printf_nolog("\n");
	panic("lock error");
}
