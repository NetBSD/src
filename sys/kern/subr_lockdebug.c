/*	$NetBSD: subr_lockdebug.c,v 1.22.2.7 2007/12/13 13:57:51 yamt Exp $	*/

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
 * Basic lock debugging code shared among lock primitives.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_lockdebug.c,v 1.22.2.7 2007/12/13 13:57:51 yamt Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/lock.h>
#include <sys/lockdebug.h>
#include <sys/sleepq.h>
#include <sys/cpu.h>
#include <sys/atomic.h>

#include <lib/libkern/rb.h>

#ifdef LOCKDEBUG

#define	LD_BATCH_SHIFT	9
#define	LD_BATCH	(1 << LD_BATCH_SHIFT)
#define	LD_BATCH_MASK	(LD_BATCH - 1)
#define	LD_MAX_LOCKS	1048576
#define	LD_SLOP		16

#define	LD_LOCKED	0x01
#define	LD_SLEEPER	0x02

#define	LD_WRITE_LOCK	0x80000000

typedef union lockdebuglk {
	struct {
		u_int	lku_lock;
		int	lku_oldspl;
	} ul;
	uint8_t	lk_pad[CACHE_LINE_SIZE];
} volatile __aligned(CACHE_LINE_SIZE) lockdebuglk_t;

#define	lk_lock		ul.lku_lock
#define	lk_oldspl	ul.lku_oldspl

typedef struct lockdebug {
	struct rb_node	ld_rb_node;	/* must be the first member */
	_TAILQ_ENTRY(struct lockdebug, volatile) ld_chain;
	_TAILQ_ENTRY(struct lockdebug, volatile) ld_achain;
	volatile void	*ld_lock;
	lockops_t	*ld_lockops;
	struct lwp	*ld_lwp;
	uintptr_t	ld_locked;
	uintptr_t	ld_unlocked;
	uintptr_t	ld_initaddr;
	uint16_t	ld_shares;
	uint16_t	ld_cpu;
	uint8_t		ld_flags;
	uint8_t		ld_shwant;	/* advisory */
	uint8_t		ld_exwant;	/* advisory */
	uint8_t		ld_unused;
} volatile lockdebug_t;

typedef _TAILQ_HEAD(lockdebuglist, struct lockdebug, volatile) lockdebuglist_t;

lockdebuglk_t		ld_tree_lk;
lockdebuglk_t		ld_sleeper_lk;
lockdebuglk_t		ld_spinner_lk;
lockdebuglk_t		ld_free_lk;

lockdebuglist_t		ld_sleepers = TAILQ_HEAD_INITIALIZER(ld_sleepers);
lockdebuglist_t		ld_spinners = TAILQ_HEAD_INITIALIZER(ld_spinners);
lockdebuglist_t		ld_free = TAILQ_HEAD_INITIALIZER(ld_free);
lockdebuglist_t		ld_all = TAILQ_HEAD_INITIALIZER(ld_all);
int			ld_nfree;
int			ld_freeptr;
int			ld_recurse;
bool			ld_nomore;
lockdebug_t		*ld_table[LD_MAX_LOCKS / LD_BATCH];

lockdebug_t		ld_prime[LD_BATCH];

static void	lockdebug_abort1(lockdebug_t *, lockdebuglk_t *lk,
				 const char *, const char *, bool);
static void	lockdebug_more(void);
static void	lockdebug_init(void);

static signed int
ld_rb_compare_nodes(const struct rb_node *n1, const struct rb_node *n2)
{
	const lockdebug_t *ld1 = (const void *)n1;
	const lockdebug_t *ld2 = (const void *)n2;
	const uintptr_t a = (uintptr_t)ld1->ld_lock;
	const uintptr_t b = (uintptr_t)ld2->ld_lock;

	if (a < b)
		return 1;
	if (a > b)
		return -1;
	return 0;
}

static signed int
ld_rb_compare_key(const struct rb_node *n, const void *key)
{
	const lockdebug_t *ld = (const void *)n;
	const uintptr_t a = (uintptr_t)ld->ld_lock;
	const uintptr_t b = (uintptr_t)key;

	if (a < b)
		return 1;
	if (a > b)
		return -1;
	return 0;
}

static struct rb_tree ld_rb_tree;

static const struct rb_tree_ops ld_rb_tree_ops = {
	.rb_compare_nodes = ld_rb_compare_nodes,
	.rb_compare_key = ld_rb_compare_key,
};

static void
lockdebug_lock_init(lockdebuglk_t *lk)
{

	lk->lk_lock = 0;
}

static void
lockdebug_lock(lockdebuglk_t *lk)
{
	int s;
	
	s = splhigh();
	do {
		while (lk->lk_lock != 0) {
			SPINLOCK_SPIN_HOOK;
		}
	} while (atomic_cas_uint(&lk->lk_lock, 0, LD_WRITE_LOCK) != 0);
	lk->lk_oldspl = s;
	membar_enter();
}

static void
lockdebug_unlock(lockdebuglk_t *lk)
{
	int s;

	s = lk->lk_oldspl;
	membar_exit();
	lk->lk_lock = 0;
	splx(s);
}

static int
lockdebug_lock_rd(lockdebuglk_t *lk)
{
	u_int val;
	int s;
	
	s = splhigh();
	do {
		while ((val = lk->lk_lock) == LD_WRITE_LOCK){
			SPINLOCK_SPIN_HOOK;
		}
	} while (atomic_cas_uint(&lk->lk_lock, val, val + 1) != val);
	membar_enter();
	return s;
}

static void
lockdebug_unlock_rd(lockdebuglk_t *lk, int s)
{

	membar_exit();
	atomic_dec_uint(&lk->lk_lock);
	splx(s);
}

static inline lockdebug_t *
lockdebug_lookup1(volatile void *lock, lockdebuglk_t **lk)
{
	lockdebug_t *ld;
	int s;

	s = lockdebug_lock_rd(&ld_tree_lk);
	ld = (lockdebug_t *)rb_tree_find_node(&ld_rb_tree, __UNVOLATILE(lock));
	lockdebug_unlock_rd(&ld_tree_lk, s);
	if (ld == NULL)
		return NULL;

	if ((ld->ld_flags & LD_SLEEPER) != 0)
		*lk = &ld_sleeper_lk;
	else
		*lk = &ld_spinner_lk;

	lockdebug_lock(*lk);
	return ld;
}

/*
 * lockdebug_lookup:
 *
 *	Find a lockdebug structure by a pointer to a lock and return it locked.
 */
static inline lockdebug_t *
lockdebug_lookup(volatile void *lock, lockdebuglk_t **lk)
{
	lockdebug_t *ld;

	ld = lockdebug_lookup1(lock, lk);
	if (ld == NULL)
		panic("lockdebug_lookup: uninitialized lock (lock=%p)", lock);
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

	lockdebug_lock_init(&ld_tree_lk);
	lockdebug_lock_init(&ld_sleeper_lk);
	lockdebug_lock_init(&ld_spinner_lk);
	lockdebug_lock_init(&ld_free_lk);

	rb_tree_init(&ld_rb_tree, &ld_rb_tree_ops);

	ld = ld_prime;
	ld_table[0] = ld;
	for (i = 1, ld++; i < LD_BATCH; i++, ld++) {
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
bool
lockdebug_alloc(volatile void *lock, lockops_t *lo, uintptr_t initaddr)
{
	struct cpu_info *ci;
	lockdebug_t *ld;
	lockdebuglk_t *lk;

	if (lo == NULL || panicstr != NULL)
		return false;
	if (ld_freeptr == 0)
		lockdebug_init();

	if ((ld = lockdebug_lookup1(lock, &lk)) != NULL) {
		lockdebug_abort1(ld, lk, __func__, "already initialized", true);
		/* NOTREACHED */
	}

	/*
	 * Pinch a new debug structure.  We may recurse because we call
	 * kmem_alloc(), which may need to initialize new locks somewhere
	 * down the path.  If not recursing, we try to maintain at least
	 * LD_SLOP structures free, which should hopefully be enough to
	 * satisfy kmem_alloc().  If we can't provide a structure, not to
	 * worry: we'll just mark the lock as not having an ID.
	 */
	lockdebug_lock(&ld_free_lk);
	ci = curcpu();
	ci->ci_lkdebug_recurse++;

	if (TAILQ_EMPTY(&ld_free)) {
		if (ci->ci_lkdebug_recurse > 1 || ld_nomore) {
			ci->ci_lkdebug_recurse--;
			lockdebug_unlock(&ld_free_lk);
			return false;
		}
		lockdebug_more();
	} else if (ci->ci_lkdebug_recurse == 1 && ld_nfree < LD_SLOP)
		lockdebug_more();

	if ((ld = TAILQ_FIRST(&ld_free)) == NULL) {
		lockdebug_unlock(&ld_free_lk);
		return false;
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
	ld->ld_initaddr = initaddr;

	lockdebug_lock(&ld_tree_lk);
	rb_tree_insert_node(&ld_rb_tree, __UNVOLATILE(&ld->ld_rb_node));
	lockdebug_unlock(&ld_tree_lk);

	if (lo->lo_sleeplock) {
		ld->ld_flags = LD_SLEEPER;
		lockdebug_unlock(&ld_sleeper_lk);
	} else {
		ld->ld_flags = 0;
		lockdebug_unlock(&ld_spinner_lk);
	}

	return true;
}

/*
 * lockdebug_free:
 *
 *	A lock is being destroyed, so release debugging resources.
 */
void
lockdebug_free(volatile void *lock)
{
	lockdebug_t *ld;
	lockdebuglk_t *lk;

	if (panicstr != NULL)
		return;

	ld = lockdebug_lookup(lock, &lk);
	if (ld == NULL) {
		panic("lockdebug_free: destroying uninitialized lock %p"
		    "(ld_lock=%p)", lock, ld->ld_lock);
		lockdebug_abort1(ld, lk, __func__, "lock record follows",
		    true);
	}
	if ((ld->ld_flags & LD_LOCKED) != 0 || ld->ld_shares != 0)
		lockdebug_abort1(ld, lk, __func__, "is locked", true);
	lockdebug_lock(&ld_tree_lk);
	rb_tree_remove_node(&ld_rb_tree, __UNVOLATILE(&ld->ld_rb_node));
	lockdebug_unlock(&ld_tree_lk);
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
			TAILQ_INSERT_TAIL(&ld_free, ld, ld_chain);
			TAILQ_INSERT_TAIL(&ld_all, ld, ld_achain);
		}

		membar_producer();
		ld_table[ld_freeptr++] = block;
	}
}

/*
 * lockdebug_wantlock:
 *
 *	Process the preamble to a lock acquire.
 */
void
lockdebug_wantlock(volatile void *lock, uintptr_t where, int shared)
{
	struct lwp *l = curlwp;
	lockdebuglk_t *lk;
	lockdebug_t *ld;
	bool recurse;

	(void)shared;
	recurse = false;

	if (panicstr != NULL)
		return;

	if ((ld = lockdebug_lookup(lock, &lk)) == NULL)
		return;

	if ((ld->ld_flags & LD_LOCKED) != 0) {
		if ((ld->ld_flags & LD_SLEEPER) != 0) {
			if (ld->ld_lwp == l)
				recurse = true;
		} else if (ld->ld_cpu == (uint16_t)cpu_number())
			recurse = true;
	}

	if (cpu_intr_p()) {
		if ((ld->ld_flags & LD_SLEEPER) != 0)
			lockdebug_abort1(ld, lk, __func__,
			    "acquiring sleep lock from interrupt context",
			    true);
	}

	if (shared)
		ld->ld_shwant++;
	else
		ld->ld_exwant++;

	if (recurse)
		lockdebug_abort1(ld, lk, __func__, "locking against myself",
		    true);

	lockdebug_unlock(lk);
}

/*
 * lockdebug_locked:
 *
 *	Process a lock acquire operation.
 */
void
lockdebug_locked(volatile void *lock, uintptr_t where, int shared)
{
	struct lwp *l = curlwp;
	lockdebuglk_t *lk;
	lockdebug_t *ld;

	if (panicstr != NULL)
		return;

	if ((ld = lockdebug_lookup(lock, &lk)) == NULL)
		return;

	if (shared) {
		l->l_shlocks++;
		ld->ld_shares++;
		ld->ld_shwant--;
	} else {
		if ((ld->ld_flags & LD_LOCKED) != 0)
			lockdebug_abort1(ld, lk, __func__,
			    "already locked", true);

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
lockdebug_unlocked(volatile void *lock, uintptr_t where, int shared)
{
	struct lwp *l = curlwp;
	lockdebuglk_t *lk;
	lockdebug_t *ld;

	if (panicstr != NULL)
		return;

	if ((ld = lockdebug_lookup(lock, &lk)) == NULL)
		return;

	if (shared) {
		if (l->l_shlocks == 0)
			lockdebug_abort1(ld, lk, __func__,
			    "no shared locks held by LWP", true);
		if (ld->ld_shares == 0)
			lockdebug_abort1(ld, lk, __func__,
			    "no shared holds on this lock", true);
		l->l_shlocks--;
		ld->ld_shares--;
	} else {
		if ((ld->ld_flags & LD_LOCKED) == 0)
			lockdebug_abort1(ld, lk, __func__, "not locked",
			    true);

		if ((ld->ld_flags & LD_SLEEPER) != 0) {
			if (ld->ld_lwp != curlwp)
				lockdebug_abort1(ld, lk, __func__,
				    "not held by current LWP", true);
			ld->ld_flags &= ~LD_LOCKED;
			ld->ld_unlocked = where;
			ld->ld_lwp = NULL;
			curlwp->l_exlocks--;
			TAILQ_REMOVE(&ld_sleepers, ld, ld_chain);
		} else {
			if (ld->ld_cpu != (uint16_t)cpu_number())
				lockdebug_abort1(ld, lk, __func__,
				    "not held by current CPU", true);
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
	int s;

	if (panicstr != NULL)
		return;

	crit_enter();

	if (curcpu()->ci_spin_locks2 != 0) {
		cpuno = (uint16_t)cpu_number();

		s = lockdebug_lock_rd(&ld_spinner_lk);
		TAILQ_FOREACH(ld, &ld_spinners, ld_chain) {
			if (ld->ld_lock == spinlock) {
				if (ld->ld_cpu != cpuno)
					lockdebug_abort1(ld, &ld_spinner_lk,
					    __func__,
					    "not held by current CPU", true);
				continue;
			}
			if (ld->ld_cpu == cpuno && (l->l_pflag & LP_INTR) == 0)
				lockdebug_abort1(ld, &ld_spinner_lk,
				    __func__, "spin lock held", true);
		}
		lockdebug_unlock_rd(&ld_spinner_lk, s);
	}

	if (!slplocks) {
		if (l->l_exlocks != 0) {
			s = lockdebug_lock_rd(&ld_sleeper_lk);
			TAILQ_FOREACH(ld, &ld_sleepers, ld_chain) {
				if (ld->ld_lwp == l)
					lockdebug_abort1(ld, &ld_sleeper_lk,
					    __func__, "sleep lock held", true);
			}
			lockdebug_unlock_rd(&ld_sleeper_lk, s);
		}
		if (l->l_shlocks != 0)
			panic("lockdebug_barrier: holding %d shared locks",
			    l->l_shlocks);
	}

	crit_exit();
}

/*
 * lockdebug_mem_check:
 *
 *	Check for in-use locks within a memory region that is
 *	being freed.
 */
void
lockdebug_mem_check(const char *func, void *base, size_t sz)
{
	lockdebug_t *ld;
	lockdebuglk_t *lk;
	int s;

	if (panicstr != NULL)
		return;

	s = lockdebug_lock_rd(&ld_tree_lk);
	ld = (lockdebug_t *)rb_tree_find_node_geq(&ld_rb_tree, base);
	if (ld != NULL) {
		const uintptr_t lock = (uintptr_t)ld->ld_lock;

		if ((uintptr_t)base > lock)
			panic("%s: corrupt tree ld=%p, base=%p, sz=%zu",
			    __func__, ld, base, sz);
		if (lock >= (uintptr_t)base + sz)
			ld = NULL;
	}
	lockdebug_unlock_rd(&ld_tree_lk, s);
	if (ld == NULL)
		return;

	if ((ld->ld_flags & LD_SLEEPER) != 0)
		lk = &ld_sleeper_lk;
	else
		lk = &ld_spinner_lk;

	lockdebug_lock(lk);
	lockdebug_abort1(ld, lk, func,
	    "allocation contains active lock", !cold);
	return;
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
	    "last locked  : %#018lx unlocked : %#018lx\n"
	    "initialized  : %#018lx\n",
	    (long)ld->ld_lock, (sleeper ? "sleep/adaptive" : "spin"),
	    (unsigned)ld->ld_shares, ((ld->ld_flags & LD_LOCKED) != 0),
	    (unsigned)ld->ld_shwant, (unsigned)ld->ld_exwant,
	    (unsigned)cpu_number(), (unsigned)ld->ld_cpu,
	    (long)curlwp, (long)ld->ld_lwp,
	    (long)ld->ld_locked, (long)ld->ld_unlocked,
	    (long)ld->ld_initaddr);

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
		 const char *msg, bool dopanic)
{

	printf_nolog("%s error: %s: %s\n\n", ld->ld_lockops->lo_name,
	    func, msg);
	lockdebug_dump(ld, printf_nolog);
	lockdebug_unlock(lk);
	printf_nolog("\n");
	if (dopanic)
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
lockdebug_abort(volatile void *lock, lockops_t *ops, const char *func,
		const char *msg)
{
#ifdef LOCKDEBUG
	lockdebug_t *ld;
	lockdebuglk_t *lk;

	if ((ld = lockdebug_lookup(lock, &lk)) != NULL) {
		lockdebug_abort1(ld, lk, func, msg, true);
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
