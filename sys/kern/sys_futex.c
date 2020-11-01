/*	$NetBSD: sys_futex.c,v 1.11.2.1 2020/11/01 15:16:43 thorpej Exp $	*/

/*-
 * Copyright (c) 2018, 2019, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell and Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_futex.c,v 1.11.2.1 2020/11/01 15:16:43 thorpej Exp $");

/*
 * Futexes
 *
 *	The futex system call coordinates notifying threads waiting for
 *	changes on a 32-bit word of memory.  The word can be managed by
 *	CPU atomic operations in userland, without system calls, as long
 *	as there is no contention.
 *
 *	The simplest use case demonstrating the utility is:
 *
 *		// 32-bit word of memory shared among threads or
 *		// processes in userland.  lock & 1 means owned;
 *		// lock & 2 means there are waiters waiting.
 *		volatile int lock = 0;
 *
 *		int v;
 *
 *		// Acquire a lock.
 *		do {
 *			v = lock;
 *			if (v & 1) {
 *				// Lock is held.  Set a bit to say that
 *				// there are waiters, and wait for lock
 *				// to change to anything other than v;
 *				// then retry.
 *				if (atomic_cas_uint(&lock, v, v | 2) != v)
 *					continue;
 *				futex(FUTEX_WAIT, &lock, v | 2, NULL, NULL, 0);
 *				continue;
 *			}
 *		} while (atomic_cas_uint(&lock, v, v & ~1) != v);
 *		membar_enter();
 *
 *		...
 *
 *		// Release the lock.  Optimistically assume there are
 *		// no waiters first until demonstrated otherwise.
 *		membar_exit();
 *		if (atomic_cas_uint(&lock, 1, 0) != 1) {
 *			// There may be waiters.
 *			v = atomic_swap_uint(&lock, 0);
 *			// If there are still waiters, wake one.
 *			if (v & 2)
 *				futex(FUTEX_WAKE, &lock, 1, NULL, NULL, 0);
 *		}
 *
 *	The goal is to avoid the futex system call unless there is
 *	contention; then if there is contention, to guarantee no missed
 *	wakeups.
 *
 *	For a simple implementation, futex(FUTEX_WAIT) could queue
 *	itself to be woken, double-check the lock word, and then sleep;
 *	spurious wakeups are generally a fact of life, so any
 *	FUTEX_WAKE could just wake every FUTEX_WAIT in the system.
 *
 *	If this were all there is to it, we could then increase
 *	parallelism by refining the approximation: partition the
 *	waiters into buckets by hashing the lock addresses to reduce
 *	the incidence of spurious wakeups.  But this is not all.
 *
 *	The futex(FUTEX_CMP_REQUEUE, &lock, n, &lock2, m, val)
 *	operation not only wakes n waiters on lock if lock == val, but
 *	also _transfers_ m additional waiters to lock2.  Unless wakeups
 *	on lock2 also trigger wakeups on lock, we cannot move waiters
 *	to lock2 if they merely share the same hash as waiters on lock.
 *	Thus, we can't approximately distribute waiters into queues by
 *	a hash function; we must distinguish futex queues exactly by
 *	lock address.
 *
 *	For now, we use a global red/black tree to index futexes.  This
 *	should be replaced by a lockless radix tree with a thread to
 *	free entries no longer in use once all lookups on all CPUs have
 *	completed.
 *
 *	Specifically, we maintain two maps:
 *
 *	futex_tab.va[vmspace, va] for private futexes
 *	futex_tab.oa[uvm_voaddr] for shared futexes
 *
 *	This implementation does not support priority inheritance.
 */

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/condvar.h>
#include <sys/futex.h>
#include <sys/mutex.h>
#include <sys/rbtree.h>
#include <sys/sleepq.h>
#include <sys/queue.h>
#include <sys/sdt.h>

#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>

#include <uvm/uvm_extern.h>

/*
 * DTrace probes.
 */
SDT_PROVIDER_DEFINE(futex);

/* entry: uaddr, val, bitset, timeout, clkflags, fflags */
/* exit: error */
SDT_PROBE_DEFINE6(futex, func, wait, entry, "int *", "int", "int",
		  "struct timespec *", "int", "int");
SDT_PROBE_DEFINE1(futex, func, wait, exit, "int");

/* entry: uaddr, nwake, bitset, fflags */
/* exit: error, nwoken */
SDT_PROBE_DEFINE4(futex, func, wake, entry, "int *", "int", "int", "int");
SDT_PROBE_DEFINE2(futex, func, wake, exit, "int", "int");

/* entry: uaddr, nwake, uaddr2, nrequeue, fflags */
/* exit: error, nwoken */
SDT_PROBE_DEFINE5(futex, func, requeue, entry, "int *", "int", "int *", "int",
		  "int");
SDT_PROBE_DEFINE2(futex, func, requeue, exit, "int", "int");

/* entry: uaddr, nwake, uaddr2, nrequeue, val3, fflags */
/* exit: error, nwoken */
SDT_PROBE_DEFINE6(futex, func, cmp_requeue, entry, "int *", "int", "int *",
		  "int", "int", "int");
SDT_PROBE_DEFINE2(futex, func, cmp_requeue, exit, "int", "int");

/* entry: uaddr, nwake, uaddr2, nwake2, wakeop, fflags */
/* exit: error, nwoken */
SDT_PROBE_DEFINE6(futex, func, wake_op, entry, "int *", "int", "int *", "int",
		  "int", "int");
SDT_PROBE_DEFINE2(futex, func, wake_op, exit, "int", "int");

/* entry: uaddr, val, r/w, abstime, fflags */
/* exit: error */
SDT_PROBE_DEFINE5(futex, func, rw_wait, entry, "int *", "int", "int",
		  "struct timespec *", "int");
SDT_PROBE_DEFINE1(futex, func, rw_wait, exit, "int");

/* entry: uaddr, val, fflags */
/* exit: error, nwoken */
SDT_PROBE_DEFINE3(futex, func, rw_handoff, entry, "int *", "int", "int");
SDT_PROBE_DEFINE2(futex, func, rw_handoff, exit, "int", "int");

SDT_PROBE_DEFINE0(futex, wait, finish, normally);
SDT_PROBE_DEFINE0(futex, wait, finish, wakerace);
SDT_PROBE_DEFINE0(futex, wait, finish, aborted);

/* entry: timo */
/* exit: error */
SDT_PROBE_DEFINE1(futex, wait, sleepq_block, entry, "int");
SDT_PROBE_DEFINE1(futex, wait, sleepq_block, exit, "int");

/*
 * Lock order:
 *
 *	futex_tab.lock
 *	futex::fx_op_lock		ordered by kva of struct futex
 *	 -> futex::fx_sq_lock		ordered by kva of sleepq lock
 *
 * N.B. multiple futexes can share a single sleepq lock.
 */

/*
 * union futex_key
 *
 *	A futex is addressed either by a vmspace+va (private) or by
 *	a uvm_voaddr (shared).
 */
union futex_key {
	struct {
		struct vmspace			*vmspace;
		vaddr_t				va;
	}			fk_private;
	struct uvm_voaddr	fk_shared;
};

/*
 * struct futex
 *
 *	Kernel state for a futex located at a particular address in a
 *	particular virtual address space.
 *
 *	N.B. fx_refcnt is an unsigned long because we need to be able
 *	to operate on it atomically on all systems while at the same
 *	time rendering practically impossible the chance of it reaching
 *	its max value.  In practice, we're limited by the number of LWPs
 *	that can be present on the system at any given time, and the
 *	assumption is that limit will be good enough on a 32-bit platform.
 *	See futex_wake() for why overflow needs to be avoided.
 *
 *	XXX Since futex addresses must be 4-byte aligned, we could
 *	XXX squirrel away fx_shared and fx_on_tree bits in the "va"
 *	XXX field of the key.  Worth it?
 */
struct futex {
	union futex_key		fx_key;
	struct rb_node		fx_node;
	unsigned long		fx_refcnt;
	bool			fx_shared;
	bool			fx_on_tree;
	uint8_t			fx_class;

	kmutex_t		fx_op_lock;	/* adaptive */
	kmutex_t *		fx_sq_lock;	/* &sleepq_locks[...] */
	sleepq_t		fx_sqs[2];	/* 0=reader, 1=writer */
	unsigned int		fx_nwaiters[2];
};

/*
 * futex classes: Some futex operations can only be carried out on
 * futexes that are known to be abiding by a certain protocol.  These
 * classes are assigned to a futex when created due to a wait event,
 * and when subsequent wake or requeue operations are issued, the
 * class is checked at futex lookup time.  If the class does not match,
 * EINVAL is the result.
 */
#define	FUTEX_CLASS_ANY		0		/* match any class in lookup */
#define	FUTEX_CLASS_NORMAL	1		/* normal futex */
#define	FUTEX_CLASS_RWLOCK	2		/* rwlock futex */
#define	FUTEX_CLASS_PI		3		/* for FUTEX_*_PI ops */

/* sleepq definitions */
#define	FUTEX_READERQ		0
#define	FUTEX_WRITERQ		1

CTASSERT(FUTEX_READERQ == FUTEX_RW_READER);
CTASSERT(FUTEX_WRITERQ == FUTEX_RW_WRITER);

static const char futex_wmesg[] = "futex";

static void	futex_unsleep(lwp_t *, bool);

static syncobj_t futex_syncobj = {
	.sobj_flag	= SOBJ_SLEEPQ_SORTED,
	.sobj_unsleep	= futex_unsleep,
	.sobj_changepri	= sleepq_changepri,
	.sobj_lendpri	= sleepq_lendpri,
	.sobj_owner	= syncobj_noowner,
};

/*
 * futex_tab
 *
 *	Global trees of futexes by vmspace/va and VM object address.
 *
 *	XXX This obviously doesn't scale in parallel.  We could use a
 *	pserialize-safe data structure, but there may be a high cost to
 *	frequent deletion since we don't cache futexes after we're done
 *	with them.  We could use hashed locks.  But for now, just make
 *	sure userland can't DoS the serial performance, by using a
 *	balanced binary tree for lookup.
 *
 *	XXX We could use a per-process tree for the table indexed by
 *	virtual address to reduce contention between processes.
 */
static struct {
	kmutex_t	lock;
	struct rb_tree	va;
	struct rb_tree	oa;
} futex_tab __cacheline_aligned;

static int
compare_futex_key(void *cookie, const void *n, const void *k)
{
	const struct futex *fa = n;
	const union futex_key *fka = &fa->fx_key;
	const union futex_key *fkb = k;

	if ((uintptr_t)fka->fk_private.vmspace <
	    (uintptr_t)fkb->fk_private.vmspace)
		return -1;
	if ((uintptr_t)fka->fk_private.vmspace >
	    (uintptr_t)fkb->fk_private.vmspace)
		return +1;
	if (fka->fk_private.va < fkb->fk_private.va)
		return -1;
	if (fka->fk_private.va > fkb->fk_private.va)
		return -1;
	return 0;
}

static int
compare_futex(void *cookie, const void *na, const void *nb)
{
	const struct futex *fa = na;
	const struct futex *fb = nb;

	return compare_futex_key(cookie, fa, &fb->fx_key);
}

static const rb_tree_ops_t futex_rb_ops = {
	.rbto_compare_nodes = compare_futex,
	.rbto_compare_key = compare_futex_key,
	.rbto_node_offset = offsetof(struct futex, fx_node),
};

static int
compare_futex_shared_key(void *cookie, const void *n, const void *k)
{
	const struct futex *fa = n;
	const union futex_key *fka = &fa->fx_key;
	const union futex_key *fkb = k;

	return uvm_voaddr_compare(&fka->fk_shared, &fkb->fk_shared);
}

static int
compare_futex_shared(void *cookie, const void *na, const void *nb)
{
	const struct futex *fa = na;
	const struct futex *fb = nb;

	return compare_futex_shared_key(cookie, fa, &fb->fx_key);
}

static const rb_tree_ops_t futex_shared_rb_ops = {
	.rbto_compare_nodes = compare_futex_shared,
	.rbto_compare_key = compare_futex_shared_key,
	.rbto_node_offset = offsetof(struct futex, fx_node),
};

/*
 * futex_sq_lock2(f, f2)
 *
 *	Acquire the sleepq locks for f and f2, which may be null, or
 *	which may be the same.  If they are distinct, an arbitrary total
 *	order is chosen on the locks.
 *
 *	Callers should only ever acquire multiple sleepq locks
 *	simultaneously using futex_sq_lock2.
 */
static void
futex_sq_lock2(struct futex * const f, struct futex * const f2)
{

	/*
	 * If both are null, do nothing; if one is null and the other
	 * is not, lock the other and be done with it.
	 */
	if (f == NULL && f2 == NULL) {
		return;
	} else if (f == NULL) {
		mutex_spin_enter(f2->fx_sq_lock);
		return;
	} else if (f2 == NULL) {
		mutex_spin_enter(f->fx_sq_lock);
		return;
	}

	kmutex_t * const m = f->fx_sq_lock;
	kmutex_t * const m2 = f2->fx_sq_lock;

	/* If both locks are the same, acquire only one. */
	if (m == m2) {
		mutex_spin_enter(m);
		return;
	}

	/* Otherwise, use the ordering on the kva of the lock pointer.  */
	if ((uintptr_t)m < (uintptr_t)m2) {
		mutex_spin_enter(m);
		mutex_spin_enter(m2);
	} else {
		mutex_spin_enter(m2);
		mutex_spin_enter(m);
	}
}

/*
 * futex_sq_unlock2(f, f2)
 *
 *	Release the sleep queue locks for f and f2, which may be null, or
 *	which may have the same underlying lock.
 */
static void
futex_sq_unlock2(struct futex * const f, struct futex * const f2)
{

	/*
	 * If both are null, do nothing; if one is null and the other
	 * is not, unlock the other and be done with it.
	 */
	if (f == NULL && f2 == NULL) {
		return;
	} else if (f == NULL) {
		mutex_spin_exit(f2->fx_sq_lock);
		return;
	} else if (f2 == NULL) {
		mutex_spin_exit(f->fx_sq_lock);
		return;
	}

	kmutex_t * const m = f->fx_sq_lock;
	kmutex_t * const m2 = f2->fx_sq_lock;

	/* If both locks are the same, release only one. */
	if (m == m2) {
		mutex_spin_exit(m);
		return;
	}

	/* Otherwise, use the ordering on the kva of the lock pointer.  */
	if ((uintptr_t)m < (uintptr_t)m2) {
		mutex_spin_exit(m2);
		mutex_spin_exit(m);
	} else {
		mutex_spin_exit(m);
		mutex_spin_exit(m2);
	}
}

/*
 * futex_load(uaddr, kaddr)
 *
 *	Perform a single atomic load to read *uaddr, and return the
 *	result in *kaddr.  Return 0 on success, EFAULT if uaddr is not
 *	mapped.
 */
static inline int
futex_load(int *uaddr, int *kaddr)
{
	return ufetch_int((u_int *)uaddr, (u_int *)kaddr);
}

/*
 * futex_test(uaddr, expected)
 *
 *	True if *uaddr == expected.  False if *uaddr != expected, or if
 *	uaddr is not mapped.
 */
static bool
futex_test(int *uaddr, int expected)
{
	int val;
	int error;

	error = futex_load(uaddr, &val);
	if (error)
		return false;
	return val == expected;
}

static pool_cache_t	futex_cache		__read_mostly;

static int	futex_ctor(void *, void *, int);
static void	futex_dtor(void *, void *);

/*
 * futex_sys_init()
 *
 *	Initialize the futex subsystem.
 */
void
futex_sys_init(void)
{

	mutex_init(&futex_tab.lock, MUTEX_DEFAULT, IPL_NONE);
	rb_tree_init(&futex_tab.va, &futex_rb_ops);
	rb_tree_init(&futex_tab.oa, &futex_shared_rb_ops);

	futex_cache = pool_cache_init(sizeof(struct futex),
	    coherency_unit, 0, 0, "futex", NULL, IPL_NONE, futex_ctor,
	    futex_dtor, NULL);
}

/*
 * futex_sys_fini()
 *
 *	Finalize the futex subsystem.
 */
void
futex_sys_fini(void)
{

	KASSERT(RB_TREE_MIN(&futex_tab.oa) == NULL);
	KASSERT(RB_TREE_MIN(&futex_tab.va) == NULL);
	mutex_destroy(&futex_tab.lock);
}

/*
 * futex_ctor()
 *
 *	Futex object constructor.
 */
static int
futex_ctor(void *arg __unused, void *obj, int flags __unused)
{
	extern sleepqlock_t sleepq_locks[SLEEPTAB_HASH_SIZE];
	struct futex * const f = obj;

	mutex_init(&f->fx_op_lock, MUTEX_DEFAULT, IPL_NONE);
	f->fx_sq_lock = &sleepq_locks[SLEEPTAB_HASH(f)].lock;

	sleepq_init(&f->fx_sqs[FUTEX_READERQ]);
	sleepq_init(&f->fx_sqs[FUTEX_WRITERQ]);
	f->fx_nwaiters[FUTEX_READERQ] = f->fx_nwaiters[FUTEX_WRITERQ] = 0;

	return 0;
}

/*
 * futex_dtor()
 *
 *	Futex object destructor.
 */
static void
futex_dtor(void *arg __unused, void *obj)
{
	struct futex * const f = obj;

	mutex_destroy(&f->fx_op_lock);
	f->fx_sq_lock = NULL;
}

/*
 * futex_key_init(key, vm, va, shared)
 *
 *	Initialize a futex key for lookup, etc.
 */
static int
futex_key_init(union futex_key *fk, struct vmspace *vm, vaddr_t va, bool shared)
{
	int error = 0;

	if (__predict_false(shared)) {
		if (!uvm_voaddr_acquire(&vm->vm_map, va, &fk->fk_shared))
			error = EFAULT;
	} else {
		fk->fk_private.vmspace = vm;
		fk->fk_private.va = va;
	}

	return error;
}

/*
 * futex_key_fini(key, shared)
 *
 *	Release a futex key.
 */
static void
futex_key_fini(union futex_key *fk, bool shared)
{
	if (__predict_false(shared))
		uvm_voaddr_release(&fk->fk_shared);
	memset(fk, 0, sizeof(*fk));
}

/*
 * futex_create(fk, shared)
 *
 *	Create a futex.  Initial reference count is 1, representing the
 *	caller.  Returns NULL on failure.  Always takes ownership of the
 *	key, either transferring it to the newly-created futex, or releasing
 *	the key if creation fails.
 *
 *	Never sleeps for memory, but may sleep to acquire a lock.
 */
static struct futex *
futex_create(union futex_key *fk, bool shared, uint8_t class)
{
	struct futex *f;

	f = pool_cache_get(futex_cache, PR_NOWAIT);
	if (f == NULL) {
		futex_key_fini(fk, shared);
		return NULL;
	}
	f->fx_key = *fk;
	f->fx_refcnt = 1;
	f->fx_shared = shared;
	f->fx_on_tree = false;
	f->fx_class = class;

	return f;
}

/*
 * futex_destroy(f)
 *
 *	Destroy a futex created with futex_create.  Reference count
 *	must be zero.
 *
 *	May sleep.
 */
static void
futex_destroy(struct futex *f)
{

	ASSERT_SLEEPABLE();

	KASSERT(atomic_load_relaxed(&f->fx_refcnt) == 0);
	KASSERT(!f->fx_on_tree);
	KASSERT(LIST_EMPTY(&f->fx_sqs[FUTEX_READERQ]));
	KASSERT(LIST_EMPTY(&f->fx_sqs[FUTEX_WRITERQ]));
	KASSERT(f->fx_nwaiters[FUTEX_READERQ] == 0);
	KASSERT(f->fx_nwaiters[FUTEX_WRITERQ] == 0);

	futex_key_fini(&f->fx_key, f->fx_shared);

	pool_cache_put(futex_cache, f);
}

/*
 * futex_hold_count(f, n)
 *
 *	Attempt to acquire a reference to f.  Return 0 on success,
 *	ENFILE on too many references.
 *
 *	Never sleeps.
 */
static int
futex_hold_count(struct futex *f, unsigned long const count)
{
	unsigned long refcnt;

	do {
		refcnt = atomic_load_relaxed(&f->fx_refcnt);
		if (ULONG_MAX - refcnt < count)
			return ENFILE;
	} while (atomic_cas_ulong(&f->fx_refcnt, refcnt,
				  refcnt + count) != refcnt);

	return 0;
}
#define	futex_hold(f)	futex_hold_count(f, 1)

/*
 * futex_rele_count(f, n)
 *
 *	Release a reference to f acquired with futex_create or
 *	futex_hold.
 *
 *	May sleep to free f.
 */
static void
futex_rele_count(struct futex *f, unsigned long const count)
{
	unsigned long refcnt;

	ASSERT_SLEEPABLE();

	do {
		refcnt = atomic_load_relaxed(&f->fx_refcnt);
		KASSERT(refcnt >= count);
		if (refcnt - count == 0)
			goto trylast;
	} while (atomic_cas_ulong(&f->fx_refcnt, refcnt,
				  refcnt - count) != refcnt);
	return;

trylast:
	KASSERT(count <= LONG_MAX);
	mutex_enter(&futex_tab.lock);
	if (atomic_add_long_nv(&f->fx_refcnt, -(long)count) == 0) {
		if (f->fx_on_tree) {
			if (__predict_false(f->fx_shared))
				rb_tree_remove_node(&futex_tab.oa, f);
			else
				rb_tree_remove_node(&futex_tab.va, f);
			f->fx_on_tree = false;
		}
	} else {
		/* References remain -- don't destroy it.  */
		f = NULL;
	}
	mutex_exit(&futex_tab.lock);
	if (f != NULL)
		futex_destroy(f);
}
#define	futex_rele(f)	futex_rele_count(f, 1)

/*
 * futex_rele_count_not_last(f, n)
 *
 *	Release a reference to f acquired with futex_create or
 *	futex_hold.
 *
 *	This version asserts that we are not dropping the last
 *	reference to f.
 */
static void
futex_rele_count_not_last(struct futex *f, unsigned long const count)
{
	unsigned long refcnt;

	do {
		refcnt = atomic_load_relaxed(&f->fx_refcnt);
		KASSERT(refcnt >= count);
	} while (atomic_cas_ulong(&f->fx_refcnt, refcnt,
				  refcnt - count) != refcnt);
}

/*
 * futex_lookup_by_key(key, shared, class, &f)
 *
 *	Try to find an existing futex va reference in the specified key
 *	On success, return 0, set f to found futex or to NULL if not found,
 *	and increment f's reference count if found.
 *
 *	Return ENFILE if reference count too high.
 *
 *	Internal lookup routine shared by futex_lookup() and
 *	futex_lookup_create().
 */
static int
futex_lookup_by_key(union futex_key *fk, bool shared, uint8_t class,
    struct futex **fp)
{
	struct futex *f;
	int error = 0;

	mutex_enter(&futex_tab.lock);
	if (__predict_false(shared)) {
		f = rb_tree_find_node(&futex_tab.oa, fk);
	} else {
		f = rb_tree_find_node(&futex_tab.va, fk);
	}
	if (f) {
		if (__predict_true(f->fx_class == class ||
				   class == FUTEX_CLASS_ANY))
			error = futex_hold(f);
		else
			error = EINVAL;
		if (error)
			f = NULL;
	}
 	*fp = f;
	mutex_exit(&futex_tab.lock);

	return error;
}

/*
 * futex_insert(f, fp)
 *
 *	Try to insert the futex f into the tree by va.  If there
 *	already is a futex for its va, acquire a reference to it, and
 *	store it in *fp; otherwise store f in *fp.
 *
 *	Return 0 on success, ENFILE if there already is a futex but its
 *	reference count is too high.
 */
static int
futex_insert(struct futex *f, struct futex **fp)
{
	struct futex *f0;
	int error;

	KASSERT(atomic_load_relaxed(&f->fx_refcnt) != 0);
	KASSERT(!f->fx_on_tree);

	mutex_enter(&futex_tab.lock);
	if (__predict_false(f->fx_shared))
		f0 = rb_tree_insert_node(&futex_tab.oa, f);
	else
		f0 = rb_tree_insert_node(&futex_tab.va, f);
	if (f0 == f) {
		f->fx_on_tree = true;
		error = 0;
	} else {
		KASSERT(atomic_load_relaxed(&f0->fx_refcnt) != 0);
		KASSERT(f0->fx_on_tree);
		error = futex_hold(f0);
		if (error)
			goto out;
	}
	*fp = f0;
out:	mutex_exit(&futex_tab.lock);

	return error;
}

/*
 * futex_lookup(uaddr, shared, class, &f)
 *
 *	Find a futex at the userland pointer uaddr in the current
 *	process's VM space.  On success, return the futex in f and
 *	increment its reference count.
 *
 *	Caller must call futex_rele when done.
 */
static int
futex_lookup(int *uaddr, bool shared, uint8_t class, struct futex **fp)
{
	union futex_key fk;
	struct vmspace *vm = curproc->p_vmspace;
	vaddr_t va = (vaddr_t)uaddr;
	int error;

	/*
	 * Reject unaligned user pointers so we don't cross page
	 * boundaries and so atomics will work.
	 */
	if (__predict_false((va & 3) != 0))
		return EINVAL;

	/* Look it up. */
	error = futex_key_init(&fk, vm, va, shared);
	if (error)
		return error;

	error = futex_lookup_by_key(&fk, shared, class, fp);
	futex_key_fini(&fk, shared);
	if (error)
		return error;

	KASSERT(*fp == NULL || (*fp)->fx_shared == shared);
	KASSERT(*fp == NULL || (*fp)->fx_class == class);
	KASSERT(*fp == NULL || atomic_load_relaxed(&(*fp)->fx_refcnt) != 0);

	/*
	 * Success!  (Caller must still check whether we found
	 * anything, but nothing went _wrong_ like trying to use
	 * unmapped memory.)
	 */
	KASSERT(error == 0);

	return error;
}

/*
 * futex_lookup_create(uaddr, shared, class, &f)
 *
 *	Find or create a futex at the userland pointer uaddr in the
 *	current process's VM space.  On success, return the futex in f
 *	and increment its reference count.
 *
 *	Caller must call futex_rele when done.
 */
static int
futex_lookup_create(int *uaddr, bool shared, uint8_t class, struct futex **fp)
{
	union futex_key fk;
	struct vmspace *vm = curproc->p_vmspace;
	struct futex *f = NULL;
	vaddr_t va = (vaddr_t)uaddr;
	int error;

	/*
	 * Reject unaligned user pointers so we don't cross page
	 * boundaries and so atomics will work.
	 */
	if (__predict_false((va & 3) != 0))
		return EINVAL;

	if (__predict_false(class == FUTEX_CLASS_ANY))
		return EINVAL;

	error = futex_key_init(&fk, vm, va, shared);
	if (error)
		return error;

	/*
	 * Optimistically assume there already is one, and try to find
	 * it.
	 */
	error = futex_lookup_by_key(&fk, shared, class, fp);
	if (error || *fp != NULL) {
		/*
		 * We either found one, or there was an error.
		 * In either case, we are done with the key.
		 */
		futex_key_fini(&fk, shared);
		goto out;
	}

	/*
	 * Create a futex recoard.  This tranfers ownership of the key
	 * in all cases.
	 */
	f = futex_create(&fk, shared, class);
	if (f == NULL) {
		error = ENOMEM;
		goto out;
	}

	/*
	 * Insert our new futex, or use existing if someone else beat
	 * us to it.
	 */
	error = futex_insert(f, fp);
	if (error)
		goto out;
	if (*fp == f)
		f = NULL;	/* don't release on exit */

	/* Success!  */
	KASSERT(error == 0);

out:	if (f != NULL)
		futex_rele(f);
	KASSERT(error || *fp != NULL);
	KASSERT(error || atomic_load_relaxed(&(*fp)->fx_refcnt) != 0);
	return error;
}

/*
 * futex_unsleep:
 *
 *	Remove an LWP from the futex and sleep queue.  This is called when
 *	the LWP has not been awoken normally but instead interrupted: for
 *	example, when a signal is received.  Must be called with the LWP
 *	locked.  Will unlock if "unlock" is true.
 */
static void
futex_unsleep(lwp_t *l, bool unlock)
{
	struct futex *f __diagused;

	f = (struct futex *)(uintptr_t)l->l_wchan;

	KASSERT(mutex_owned(f->fx_sq_lock));

	/* WRITERQ is more likely */
	if (__predict_true(l->l_sleepq == &f->fx_sqs[FUTEX_WRITERQ])) {
		KASSERT(f->fx_nwaiters[FUTEX_WRITERQ] > 0);
		f->fx_nwaiters[FUTEX_WRITERQ]--;
	} else {
		KASSERT(l->l_sleepq == &f->fx_sqs[FUTEX_READERQ]);
		KASSERT(f->fx_nwaiters[FUTEX_READERQ] > 0);
		f->fx_nwaiters[FUTEX_READERQ]--;
	}

	sleepq_unsleep(l, unlock);
}

/*
 * futex_wait(f, timeout, clkid, bitset)
 *
 *	Wait until deadline on the clock clkid, or forever if deadline
 *	is NULL, for a futex wakeup.  Return 0 on explicit wakeup,
 *	ETIMEDOUT on timeout, EINTR on signal.
 */
static int
futex_wait(struct futex *f, int q, const struct timespec *deadline,
    clockid_t clkid, int bitset)
{

	/*
	 * Some things to note about how this function works:
	 *
	 * ==> When we enter futex_wait(), the futex's op lock is
	 * held, preventing us from missing wakes.  Once we are on
	 * the futex's sleepq, it is safe to release the op lock.
	 *
	 * ==> We have a single early escape to avoid calling
	 * sleepq_block(): our deadline already having passed.
	 * If this is a no-timeout wait, or if the deadline has
	 * not already passed, we are guaranteed to call sleepq_block(). 
	 *
	 * ==> sleepq_block() contains all of the necessary logic
	 * for handling signals; we do not need to check for them
	 * ourselves.
	 *
	 * ==> Once we have blocked, other futex operations will
	 * be able to wake us or requeue us.  Until we have blocked,
	 * those other futex operations will not be able to acquire
	 * the sleepq locks in order to perform those operations,
	 * even if we have dropped the op lock.  Thus, we will not
	 * miss wakes.  This fundamental invariant is relied upon by
	 * every other synchronization object in the kernel.
	 *
	 * ==> sleepq_block() returns for one of three reasons:
	 *
	 *	-- We were awakened.
	 *	-- We were signaled.
	 *	-- We timed out.
	 *
	 * ==> Once sleepq_block() returns, we will not call
	 * sleepq_block() again.
	 *
	 * ==> If sleepq_block() returns due to being signaled,
	 * we must check to see if we were also awakened.  This
	 * is to handle the following sequence:
	 *
	 *	-- Futex wake takes us off sleepq, places us on runq.
	 *	-- We are signaled while sitting on the runq.
	 *	-- We run, and sleepq_block() notices the signal and
	 *	   returns  EINTR/ERESTART.
	 *
	 * In this situation, we want to indicate a successful wake
	 * to the caller, because that wake has been reported to the
	 * thread that issued it.
	 *
	 * Note that it is NOT possible for a wake to be issued after
	 * a signal; the LWP will already have been taken off the sleepq
	 * by the signal, so the wake will never happen.  Note that for
	 * this reason, we must *never* return ERESTART, because it could
	 * result in us waiting again with no one to awaken us.
	 */

	struct lwp * const l = curlwp;
	int timo;
	int error;

	/*
	 * Compute our timeout before taking any locks.
	 */
	if (deadline) {
		struct timespec ts;

		/* Check our watch.  */
		error = clock_gettime1(clkid, &ts);
		if (error) {
			mutex_exit(&f->fx_op_lock);
			return error;
		}

		/*
		 * If we're past the deadline, ETIMEDOUT.
		 */
		if (timespeccmp(deadline, &ts, <=)) {
			mutex_exit(&f->fx_op_lock);
			return ETIMEDOUT;
		} else {
			/* Count how much time is left.  */
			timespecsub(deadline, &ts, &ts);
			timo = tstohz(&ts);
			if (timo == 0) {
				/*
				 * tstohz() already rounds up, and
				 * a return value of 0 ticks means
				 * "expired now or in the past".
				 */
				mutex_exit(&f->fx_op_lock);
				return ETIMEDOUT;
			}
		}
	} else {
		timo = 0;
	}

	/*
	 * Acquire in sleepq -> lwp order.  While we're at it, ensure
	 * that there's room for us to block.
	 */
	mutex_spin_enter(f->fx_sq_lock);
	if (__predict_false(q == FUTEX_READERQ &&
			    f->fx_nwaiters[q] == FUTEX_TID_MASK)) {
		mutex_spin_exit(f->fx_sq_lock);
		mutex_exit(&f->fx_op_lock);
		return ENFILE;
	}
	lwp_lock(l);

	/*
	 * No need for locks here; until we're on a futex's sleepq, these
	 * values are private to the LWP, and the locks needed to get onto
	 * the sleepq provide the memory barriers needed to ensure visibility.
	 */
	l->l_futex = f;
	l->l_futex_wakesel = bitset;

	/*
	 * This is an inlined bit of sleepq_enter();
	 * we already hold the lwp lock.
	 */
	lwp_unlock_to(l, f->fx_sq_lock);
	KERNEL_UNLOCK_ALL(NULL, &l->l_biglocks);
	KASSERT(lwp_locked(l, f->fx_sq_lock));

	sleepq_enqueue(&f->fx_sqs[q], f, futex_wmesg, &futex_syncobj, true);
	f->fx_nwaiters[q]++;

	/* We can now safely release the op lock. */
	mutex_exit(&f->fx_op_lock);

	SDT_PROBE1(futex, wait, sleepq_block, entry, timo);
	error = sleepq_block(timo, true);
	SDT_PROBE1(futex, wait, sleepq_block, exit, error);

	/*
	 * f is no longer valid; we may have been moved to another
	 * futex sleepq while we slept.
	 */

	/*
	 * If something went wrong, we may still have a futex reference.
	 * Check for that here and drop it if so.  We can do this without
	 * taking any locks because l->l_futex is private to the LWP when
	 * not on any futex's sleepq, and we are not on any sleepq because
	 * we are running.
	 *
	 * Convert EWOULDBLOCK to ETIMEDOUT in case sleepq_block() returned
	 * EWOULDBLOCK, and ensure the only other error we return is EINTR.
	 */
	if (error) {
		f = l->l_futex;
		if (f != NULL) {
			SDT_PROBE0(futex, wait, finish, aborted);
			l->l_futex = NULL;
			futex_rele(f);
		} else {
			/* Raced with wakeup; report success. */
			SDT_PROBE0(futex, wait, finish, wakerace);
			error = 0;
		}
		if (error == EWOULDBLOCK)
			error = ETIMEDOUT;
		else if (error != ETIMEDOUT)
			error = EINTR;
	} else {
		KASSERT(l->l_futex == NULL);
		SDT_PROBE0(futex, wait, finish, normally);
	}

	return error;
}

/*
 * futex_wake(f, q, nwake, f2, q2, nrequeue, bitset)
 *
 *	Wake up to nwake waiters on f matching bitset; then, if f2 is
 *	provided, move up to nrequeue remaining waiters on f matching
 *	bitset to f2.  Return the number of waiters actually woken.
 *	Caller must hold the locks of f and f2, if provided.
 */
static unsigned
futex_wake(struct futex *f, int q, unsigned int const nwake,
    struct futex *f2, int q2, unsigned int const nrequeue,
    int bitset)
{
	struct lwp *l, *l_next;
	unsigned nwoken = 0;
	unsigned nrequeued = 0;

	KASSERT(mutex_owned(&f->fx_op_lock));
	KASSERT(f2 == NULL || mutex_owned(&f2->fx_op_lock));

	futex_sq_lock2(f, f2);

	/* Wake up to nwake waiters, and count the number woken.  */
	LIST_FOREACH_SAFE(l, &f->fx_sqs[q], l_sleepchain, l_next) {
		if (nwoken == nwake)
			break;

		KASSERT(l->l_syncobj == &futex_syncobj);
		KASSERT(l->l_wchan == (wchan_t)f);
		KASSERT(l->l_futex == f);
		KASSERT(l->l_sleepq == &f->fx_sqs[q]);
		KASSERT(l->l_mutex == f->fx_sq_lock);

		if ((l->l_futex_wakesel & bitset) == 0)
			continue;

		l->l_futex_wakesel = 0;
		l->l_futex = NULL;
		sleepq_remove(&f->fx_sqs[q], l);
		f->fx_nwaiters[q]--;
		nwoken++;
		/* hold counts adjusted in bulk below */
	}

	if (nrequeue) {
		KASSERT(f2 != NULL);
		/* Move up to nrequeue waiters from f's queue to f2's queue. */
		LIST_FOREACH_SAFE(l, &f->fx_sqs[q], l_sleepchain, l_next) {
			if (nrequeued == nrequeue)
				break;

			KASSERT(l->l_syncobj == &futex_syncobj);
			KASSERT(l->l_wchan == (wchan_t)f);
			KASSERT(l->l_futex == f);
			KASSERT(l->l_sleepq == &f->fx_sqs[q]);
			KASSERT(l->l_mutex == f->fx_sq_lock);

			if ((l->l_futex_wakesel & bitset) == 0)
				continue;

			l->l_futex = f2;
			sleepq_transfer(l, &f->fx_sqs[q],
			    &f2->fx_sqs[q2], f2, futex_wmesg,
			    &futex_syncobj, f2->fx_sq_lock, true);
			f->fx_nwaiters[q]--;
			f2->fx_nwaiters[q2]++;
			nrequeued++;
			/* hold counts adjusted in bulk below */
		}
		if (nrequeued) {
			/* XXX futex_hold() could theoretically fail here. */
			int hold_error __diagused =
			    futex_hold_count(f2, nrequeued);
			KASSERT(hold_error == 0);
		}
	}

	/*
	 * Adjust the futex reference counts for the wakes and
	 * requeues.
	 */
	KASSERT(nwoken + nrequeued <= LONG_MAX);
	futex_rele_count_not_last(f, nwoken + nrequeued);

	futex_sq_unlock2(f, f2);

	/* Return the number of waiters woken and requeued.  */
	return nwoken + nrequeued;
}

/*
 * futex_op_lock(f)
 *
 *	Acquire the op lock of f.  Pair with futex_op_unlock.  Do
 *	not use if caller needs to acquire two locks; use
 *	futex_op_lock2 instead.
 */
static void
futex_op_lock(struct futex *f)
{
	mutex_enter(&f->fx_op_lock);
}

/*
 * futex_op_unlock(f)
 *
 *	Release the queue lock of f.
 */
static void
futex_op_unlock(struct futex *f)
{
	mutex_exit(&f->fx_op_lock);
}

/*
 * futex_op_lock2(f, f2)
 *
 *	Acquire the op locks of both f and f2, which may be null, or
 *	which may be the same.  If they are distinct, an arbitrary total
 *	order is chosen on the locks.
 *
 *	Callers should only ever acquire multiple op locks
 *	simultaneously using futex_op_lock2.
 */
static void
futex_op_lock2(struct futex *f, struct futex *f2)
{

	/*
	 * If both are null, do nothing; if one is null and the other
	 * is not, lock the other and be done with it.
	 */
	if (f == NULL && f2 == NULL) {
		return;
	} else if (f == NULL) {
		mutex_enter(&f2->fx_op_lock);
		return;
	} else if (f2 == NULL) {
		mutex_enter(&f->fx_op_lock);
		return;
	}

	/* If both futexes are the same, acquire only one. */
	if (f == f2) {
		mutex_enter(&f->fx_op_lock);
		return;
	}

	/* Otherwise, use the ordering on the kva of the futex pointer.  */
	if ((uintptr_t)f < (uintptr_t)f2) {
		mutex_enter(&f->fx_op_lock);
		mutex_enter(&f2->fx_op_lock);
	} else {
		mutex_enter(&f2->fx_op_lock);
		mutex_enter(&f->fx_op_lock);
	}
}

/*
 * futex_op_unlock2(f, f2)
 *
 *	Release the queue locks of both f and f2, which may be null, or
 *	which may have the same underlying queue.
 */
static void
futex_op_unlock2(struct futex *f, struct futex *f2)
{

	/*
	 * If both are null, do nothing; if one is null and the other
	 * is not, unlock the other and be done with it.
	 */
	if (f == NULL && f2 == NULL) {
		return;
	} else if (f == NULL) {
		mutex_exit(&f2->fx_op_lock);
		return;
	} else if (f2 == NULL) {
		mutex_exit(&f->fx_op_lock);
		return;
	}

	/* If both futexes are the same, release only one. */
	if (f == f2) {
		mutex_exit(&f->fx_op_lock);
		return;
	}

	/* Otherwise, use the ordering on the kva of the futex pointer.  */
	if ((uintptr_t)f < (uintptr_t)f2) {
		mutex_exit(&f2->fx_op_lock);
		mutex_exit(&f->fx_op_lock);
	} else {
		mutex_exit(&f->fx_op_lock);
		mutex_exit(&f2->fx_op_lock);
	}
}

/*
 * futex_func_wait(uaddr, val, val3, timeout, clkid, clkflags, retval)
 *
 *	Implement futex(FUTEX_WAIT) and futex(FUTEX_WAIT_BITSET).
 */
static int
futex_func_wait(bool shared, int *uaddr, int val, int val3,
    const struct timespec *timeout, clockid_t clkid, int clkflags,
    register_t *retval)
{
	struct futex *f;
	struct timespec ts;
	const struct timespec *deadline;
	int error;

	/*
	 * If there's nothing to wait for, and nobody will ever wake
	 * us, then don't set anything up to wait -- just stop here.
	 */
	if (val3 == 0)
		return EINVAL;

	/* Optimistically test before anything else.  */
	if (!futex_test(uaddr, val))
		return EAGAIN;

	/* Determine a deadline on the specified clock.  */
	if (timeout == NULL || (clkflags & TIMER_ABSTIME) == TIMER_ABSTIME) {
		deadline = timeout;
	} else {
		struct timespec interval = *timeout;

		error = itimespecfix(&interval);
		if (error)
			return error;
		error = clock_gettime1(clkid, &ts);
		if (error)
			return error;
		timespecadd(&ts, &interval, &ts);
		deadline = &ts;
	}

	/* Get the futex, creating it if necessary.  */
	error = futex_lookup_create(uaddr, shared, FUTEX_CLASS_NORMAL, &f);
	if (error)
		return error;
	KASSERT(f);

	/*
	 * Under the op lock, check the value again: if it has
	 * already changed, EAGAIN; otherwise enqueue the waiter.
	 * Since FUTEX_WAKE will use the same lock and be done after
	 * modifying the value, the order in which we check and enqueue
	 * is immaterial.
	 */
	futex_op_lock(f);
	if (!futex_test(uaddr, val)) {
		futex_op_unlock(f);
		error = EAGAIN;
		goto out;
	}

	/*
	 * Now wait.  futex_wait() will drop our op lock once we
	 * are entered into the sleep queue, thus ensuring atomicity
	 * of wakes with respect to waits.
	 */
	error = futex_wait(f, FUTEX_WRITERQ, deadline, clkid, val3);

	/*
	 * We cannot drop our reference to the futex here, because
	 * we might be enqueued on a different one when we are awakened.
	 * The references will be managed on our behalf in the requeue,
	 * wake, and error cases.
	 */
	f = NULL;

	if (__predict_true(error == 0)) {
		/* Return 0 on success, error on failure. */
		*retval = 0;
	}

out:	if (f != NULL)
		futex_rele(f);
	return error;
}

/*
 * futex_func_wake(uaddr, val, val3, retval)
 *
 *	Implement futex(FUTEX_WAKE) and futex(FUTEX_WAKE_BITSET).
 */
static int
futex_func_wake(bool shared, int *uaddr, int val, int val3, register_t *retval)
{
	struct futex *f;
	unsigned int nwoken = 0;
	int error = 0;

	/* Reject negative number of wakeups.  */
	if (val < 0) {
		error = EINVAL;
		goto out;
	}

	/* Look up the futex, if any.  */
	error = futex_lookup(uaddr, shared, FUTEX_CLASS_NORMAL, &f);
	if (error)
		goto out;

	/* If there's no futex, there are no waiters to wake.  */
	if (f == NULL)
		goto out;

	/*
	 * Under f's op lock, wake the waiters and remember the
	 * number woken.
	 */
	futex_op_lock(f);
	nwoken = futex_wake(f, FUTEX_WRITERQ, val,
			    NULL, FUTEX_WRITERQ, 0, val3);
	futex_op_unlock(f);

	/* Release the futex.  */
	futex_rele(f);

out:
	/* Return the number of waiters woken.  */
	*retval = nwoken;

	/* Success!  */
	return error;
}

/*
 * futex_func_requeue(op, uaddr, val, uaddr2, val2, val3, retval)
 *
 *	Implement futex(FUTEX_REQUEUE) and futex(FUTEX_CMP_REQUEUE).
 */
static int
futex_func_requeue(bool shared, int op, int *uaddr, int val, int *uaddr2,
    int val2, int val3, register_t *retval)
{
	struct futex *f = NULL, *f2 = NULL;
	unsigned nwoken = 0;	/* default to zero woken on early return */
	int error;

	/* Reject negative number of wakeups or requeues. */
	if (val < 0 || val2 < 0) {
		error = EINVAL;
		goto out;
	}

	/* Look up the source futex, if any. */
	error = futex_lookup(uaddr, shared, FUTEX_CLASS_NORMAL, &f);
	if (error)
		goto out;

	/* If there is none, nothing to do. */
	if (f == NULL)
		goto out;

	/*
	 * We may need to create the destination futex because it's
	 * entirely possible it does not currently have any waiters.
	 */
	error = futex_lookup_create(uaddr2, shared, FUTEX_CLASS_NORMAL, &f2);
	if (error)
		goto out;

	/*
	 * Under the futexes' op locks, check the value; if
	 * unchanged from val3, wake the waiters.
	 */
	futex_op_lock2(f, f2);
	if (op == FUTEX_CMP_REQUEUE && !futex_test(uaddr, val3)) {
		error = EAGAIN;
	} else {
		error = 0;
		nwoken = futex_wake(f, FUTEX_WRITERQ, val,
				    f2, FUTEX_WRITERQ, val2,
				    FUTEX_BITSET_MATCH_ANY);
	}
	futex_op_unlock2(f, f2);

out:
	/* Return the number of waiters woken.  */
	*retval = nwoken;

	/* Release the futexes if we got them.  */
	if (f2)
		futex_rele(f2);
	if (f)
		futex_rele(f);
	return error;
}

/*
 * futex_validate_op_cmp(val3)
 *
 *	Validate an op/cmp argument for FUTEX_WAKE_OP.
 */
static int
futex_validate_op_cmp(int val3)
{
	int op = __SHIFTOUT(val3, FUTEX_OP_OP_MASK);
	int cmp = __SHIFTOUT(val3, FUTEX_OP_CMP_MASK);

	if (op & FUTEX_OP_OPARG_SHIFT) {
		int oparg = __SHIFTOUT(val3, FUTEX_OP_OPARG_MASK);
		if (oparg < 0)
			return EINVAL;
		if (oparg >= 32)
			return EINVAL;
		op &= ~FUTEX_OP_OPARG_SHIFT;
	}

	switch (op) {
	case FUTEX_OP_SET:
	case FUTEX_OP_ADD:
	case FUTEX_OP_OR:
	case FUTEX_OP_ANDN:
	case FUTEX_OP_XOR:
		break;
	default:
		return EINVAL;
	}

	switch (cmp) {
	case FUTEX_OP_CMP_EQ:
	case FUTEX_OP_CMP_NE:
	case FUTEX_OP_CMP_LT:
	case FUTEX_OP_CMP_LE:
	case FUTEX_OP_CMP_GT:
	case FUTEX_OP_CMP_GE:
		break;
	default:
		return EINVAL;
	}

	return 0;
}

/*
 * futex_compute_op(oldval, val3)
 *
 *	Apply a FUTEX_WAIT_OP operation to oldval.
 */
static int
futex_compute_op(int oldval, int val3)
{
	int op = __SHIFTOUT(val3, FUTEX_OP_OP_MASK);
	int oparg = __SHIFTOUT(val3, FUTEX_OP_OPARG_MASK);

	if (op & FUTEX_OP_OPARG_SHIFT) {
		KASSERT(oparg >= 0);
		KASSERT(oparg < 32);
		oparg = 1u << oparg;
		op &= ~FUTEX_OP_OPARG_SHIFT;
	}

	switch (op) {
	case FUTEX_OP_SET:
		return oparg;

	case FUTEX_OP_ADD:
		/*
		 * Avoid signed arithmetic overflow by doing
		 * arithmetic unsigned and converting back to signed
		 * at the end.
		 */
		return (int)((unsigned)oldval + (unsigned)oparg);

	case FUTEX_OP_OR:
		return oldval | oparg;

	case FUTEX_OP_ANDN:
		return oldval & ~oparg;

	case FUTEX_OP_XOR:
		return oldval ^ oparg;

	default:
		panic("invalid futex op");
	}
}

/*
 * futex_compute_cmp(oldval, val3)
 *
 *	Apply a FUTEX_WAIT_OP comparison to oldval.
 */
static bool
futex_compute_cmp(int oldval, int val3)
{
	int cmp = __SHIFTOUT(val3, FUTEX_OP_CMP_MASK);
	int cmparg = __SHIFTOUT(val3, FUTEX_OP_CMPARG_MASK);

	switch (cmp) {
	case FUTEX_OP_CMP_EQ:
		return (oldval == cmparg);

	case FUTEX_OP_CMP_NE:
		return (oldval != cmparg);

	case FUTEX_OP_CMP_LT:
		return (oldval < cmparg);

	case FUTEX_OP_CMP_LE:
		return (oldval <= cmparg);

	case FUTEX_OP_CMP_GT:
		return (oldval > cmparg);

	case FUTEX_OP_CMP_GE:
		return (oldval >= cmparg);

	default:
		panic("invalid futex cmp operation");
	}
}

/*
 * futex_func_wake_op(uaddr, val, uaddr2, val2, val3, retval)
 *
 *	Implement futex(FUTEX_WAKE_OP).
 */
static int
futex_func_wake_op(bool shared, int *uaddr, int val, int *uaddr2, int val2,
    int val3, register_t *retval)
{
	struct futex *f = NULL, *f2 = NULL;
	int oldval, newval, actual;
	unsigned nwoken = 0;
	int error;

	/* Reject negative number of wakeups.  */
	if (val < 0 || val2 < 0) {
		error = EINVAL;
		goto out;
	}

	/* Reject invalid operations before we start doing things.  */
	if ((error = futex_validate_op_cmp(val3)) != 0)
		goto out;

	/* Look up the first futex, if any.  */
	error = futex_lookup(uaddr, shared, FUTEX_CLASS_NORMAL, &f);
	if (error)
		goto out;

	/* Look up the second futex, if any.  */
	error = futex_lookup(uaddr2, shared, FUTEX_CLASS_NORMAL, &f2);
	if (error)
		goto out;

	/*
	 * Under the op locks:
	 *
	 * 1. Read/modify/write: *uaddr2 op= oparg.
	 * 2. Unconditionally wake uaddr.
	 * 3. Conditionally wake uaddr2, if it previously matched val3.
	 */
	futex_op_lock2(f, f2);
	do {
		error = futex_load(uaddr2, &oldval);
		if (error)
			goto out_unlock;
		newval = futex_compute_op(oldval, val3);
		error = ucas_int(uaddr2, oldval, newval, &actual);
		if (error)
			goto out_unlock;
	} while (actual != oldval);
	nwoken = (f ? futex_wake(f, FUTEX_WRITERQ, val,
				 NULL, FUTEX_WRITERQ, 0,
				 FUTEX_BITSET_MATCH_ANY) : 0);
	if (f2 && futex_compute_cmp(oldval, val3))
		nwoken += futex_wake(f2, FUTEX_WRITERQ, val2,
				     NULL, FUTEX_WRITERQ, 0,
				     FUTEX_BITSET_MATCH_ANY);

	/* Success! */
	error = 0;
out_unlock:
	futex_op_unlock2(f, f2);

out:
	/* Return the number of waiters woken. */
	*retval = nwoken;

	/* Release the futexes, if we got them. */
	if (f2)
		futex_rele(f2);
	if (f)
		futex_rele(f);
	return error;
}

/*
 * futex_read_waiter_prio(l)
 *
 *	Returns the read-waiter priority for purposes of comparing
 *	against a write-waiter's priority.  Read-waiters are only
 *	prioritized if they are real-time threads.
 */
static inline pri_t
futex_read_waiter_prio(struct lwp * const l)
{
	const pri_t pri = lwp_eprio(l);
	if (__predict_false(pri >= PRI_USER_RT))
		return pri;
	return PRI_NONE;
}

#if 0 /* XXX */
/*
 * futex_rw_handle_rt_reader(f, uaddr, val, pri, errorp)
 *
 *	Attempt to resolve the case of a real-time thread attempting
 *	to acquire a read lock.  Turns true if the attempt is resolved
 *	and the wait should be elided.
 */
static int __noinline
futex_rw_handle_rt_reader(struct futex *f, int *uaddr, int val,
    pri_t pri_reader)
{
	struct lwp *l_writer = NULL;
	int newval, next;
	int error;

	KASSERT(mutex_owned(&f->fx_op_lock));

	/*
	 * If the lock is write-locked, there isn't anything we
	 * can do but wait.
	 */
	if (val & FUTEX_RW_WRITE_LOCKED) {
		return 0;
	}

	/*
	 * If we're maxed out on readers already, nothing we can do.
	 */
	if ((val & FUTEX_TID_MASK) == FUTEX_TID_MASK) {
		return ENFILE;		/* disctinct from EAGAIN */
	}

	/*
	 * The assumption then is that we arrived here with WRITE_WANTED
	 * set.  We're not going to bother testing that, however.  We
	 * will preserve it if we see it.
	 *
	 * Because WRITE_WANTED is set, This will cause everyone to enter
	 * the futex to rw_wait.  And we are holding the op lock so no
	 * additional waiters will apear on the sleepq.  We are going
	 * to do the same tricky dance as rw_handoff to let higher-priority
	 * real-time waiters slip past the gate.
	 */

	/*
	 * All we want to do here is check if there is a writer waiting.
	 * If there is, and it is equal or greater priority, this reader
	 * loses, otherwise we will just make note if it to ensure that
	 * the WRITE_WANTED bit is set when we update the futex word.
	 *
	 * Since we are not going to actually wake someone from the
	 * queue here, we're not really interested if the write-waiter
	 * happens to leave based on a timeout or signal; we know that
	 * any write-waiter *after* the first one is of equal or lower
	 * priority, so we would still best it.
	 */
	mutex_spin_enter(f->fx_sq_lock);
	l_writer = LIST_FIRST(&f->fx_sqs[FUTEX_WRITERQ]);
	if (__predict_true(l_writer != NULL)) {
		if (pri_reader <= lwp_eprio(l_writer)) {
			return 0;
		}
	}
	mutex_spin_exit(f->fx_sq_lock);

	for (;; val = next) {
		if (__predict_true(l_writer != NULL)) {
			newval = (val + 1) | FUTEX_RW_WRITE_WANTED;
		} else {
			/*
			 * No writer waiting; increment the reader
			 * count, preserve any WRITE_WANTED bit.
			 */
			newval = val + 1;
			KASSERT(((newval ^ val) & FUTEX_RW_WRITE_WANTED) == 0);
		}

		error = ucas_int(uaddr, val, newval, &next);
		if (__predict_false(error != 0)) {
			return error;
		}
		if (next == val) {
			/* Successfully acquired the read lock. */
			return EJUSTRETURN;
		}
		/*
		 * Repeat the terminal checks from above on the new
		 * value.
		 */
		if (next & FUTEX_RW_WRITE_LOCKED) {
			return 0;
		}
		if ((next & FUTEX_TID_MASK) == FUTEX_TID_MASK) {
			return ENFILE;		/* disctinct from EAGAIN */
		}
	}
}
#endif /* XXX */

/*
 * futex_func_rw_wait(uaddr, val, val3, abstime, clkid, retval)
 *
 *	Implement futex(FUTEX_NETBSD_RW_WAIT).
 */
static int
futex_func_rw_wait(bool shared, int *uaddr, int val, int val3,
    const struct timespec *abstime, clockid_t clkid, register_t *retval)
{
#if 1
	/* XXX NETBSD_RW ops are currently broken XXX */
	return ENOSYS;
#else
	struct futex *f;
	int error;

	/* Must specify READER or WRITER. */
	if (__predict_false(val3 != FUTEX_RW_READER &&
			    val3 != FUTEX_RW_WRITER))
		return EINVAL;

	/* Optimistically test before anything else.  */
	if (!futex_test(uaddr, val))
		return EAGAIN;
	
	/* Get the futex, creating it if necessary.  */
	error = futex_lookup_create(uaddr, shared, FUTEX_CLASS_RWLOCK, &f);
	if (error)
		return error;
	KASSERT(f);

	/*
	 * Under the op lock, check the value again: if it has
	 * already changed, EAGAIN; otherwise, enqueue the waiter
	 * on the specified queue.
	 */
	futex_op_lock(f);
	if (!futex_test(uaddr, val)) {
		futex_op_unlock(f);
		error = EAGAIN;
		goto out;
	}

	/*
	 * POSIX dictates that a real-time reader will be prioritized
	 * over writers of lesser priority, when normally we would
	 * prefer the writers.
	 */
	if (__predict_false(val3 == FUTEX_RW_READER)) {
		struct lwp * const l = curlwp;
		const pri_t pri_reader = futex_read_waiter_prio(l);
		if (__predict_false(pri_reader > PRI_NONE)) {
			error = futex_rw_handle_rt_reader(f, uaddr, val,
			    pri_reader);
			if (error) {
				if (__predict_true(error == EJUSTRETURN)) {
					/* RT reader acquired the lock. */
					error = 0;
				}
				futex_op_unlock(f);
				goto out;
			}
		}
	}

	/*
	 * Now wait.  futex_wait() will dop our op lock once we
	 * are entered into the sleep queue, thus ensuring atomicity
	 * of wakes with respect to waits.
	 *
	 * Use a wake selector of 0 so that this waiter won't match on any
	 * of the other operations in case someone makes that error; only
	 * rw_handoff is allowed!  This is critical because a waiter that
	 * awakens from an rw_wait assumes it has been given ownership of
	 * the lock.
	 */
	error = futex_wait(f, val3, abstime, clkid, 0);

	/*
	 * Don't drop our reference here.  We won't be requeued, but
	 * it's best to main symmetry with other operations.
	 */
	f = NULL;

out:	if (__predict_true(error == 0)) {
		/* Return 0 on success, error on failure. */
		*retval = 0;
	}

	if (f != NULL)
		futex_rele(f);
	return error;
#endif
}

/*
 * futex_func_rw_handoff(uaddr, val, retval)
 *
 *	Implement futex(FUTEX_NETBSD_RW_HANDOFF).
 *
 *	This implements direct hand-off to either a single writer
 *	or all readers.
 */
static int
futex_func_rw_handoff(bool shared, int *uaddr, int val, register_t *retval)
{
#if 1
	/* XXX NETBSD_RW ops are currently broken XXX */
	return ENOSYS;
#else
	struct lwp *l, *l_next, *l_writer, *l_reader;
	struct futex *f;
	sleepq_t wsq, *sq;
	int error = 0;
	int newval, next, nwake, nwoken = 0;
	int allreaders __diagused = 0;
	unsigned int *nwaitersp;
	pri_t pri_writer;

	/* Look up the futex, if any.  */
	error = futex_lookup(uaddr, shared, FUTEX_CLASS_RWLOCK, &f);
	if (error)
		goto out;

	/*
	 * There are a couple of diffrent scenarios where a thread
	 * releasing a RW lock would call rw_handoff, yet we find no
	 * waiters:
	 *
	 * ==> There were waiters on the queue, but they left the queue
	 *     due to a signal.
	 * ==> The waiting thread set the waiter bit(s), but decided to
	 *     try spinning before calling rw_wait.
	 *
	 * In both of these cases, we will ensure that the lock word
	 * gets cleared.
	 */

	/* If there's no futex, there are no waiters to wake. */
	if (__predict_false(f == NULL)) {
		/*
		 * If there are no waiters, ensure that the lock
		 * word is cleared.
		 */
		error = ucas_int(uaddr, val, 0, &next);
		if (__predict_true(error == 0)) {
			if (__predict_false(val != next))
				error = EAGAIN;
		}
		goto out;
	}

	/*
	 * Compute the new value and store it in the futex word.
	 *
	 * This is a little tricky because the ucas could cause
	 * a page fault, and we can't let that happen while holding
	 * the sleepq locks.  But we have to make sure that choices
	 * we make regarding what threads to wake is accurately
	 * reflected in the futex word and that the futex word is
	 * updated before the LWPs can run.
	 *
	 * This is easy enough ... we can transfer the LWPs to a private
	 * sleepq to prevent changes in the original sleepq while we have
	 * it unlocked from affecting the results, but we must also
	 * consider that LWPs might be using timed-wait, so we have to
	 * make sure that won't wake the LWP up out from under us if the
	 * timer fires.  To do this, we have to set the "wchan" to NULL
	 * and use a dummy syncobj that takes no action on "unsleep".
	 *
	 * We thus perform the hand-off in three steps, all with the op
	 * lock held:
	 *
	 * ==> Wake selection: sleepq locked, select LWPs to wake,
	 *     compute new futex word.  LWPs are tranferred from the
	 *     futex sleepq to the private sleepq and further sedated.
	 *
	 * ==> Futex word update: sleepq unlocked, use a loop around ucas
	 *     to update the futex word.  There are no scenarios where
	 *     user space code can update the futex in a way that would
	 *     impact the work we do here; because we've been asked to do
	 *     the hand-off, any new LWPs attempting to acquire the lock
	 *     would be entering rw_wait by definition (either because
	 *     they're read-lockers and the lock is write-wanted, or they're
	 *     write-lockers and the lock is read-held).  Those new rw_wait
	 *     LWPs will serialize against the op lock.  We DO, however,
	 *     need to preserve any newly-set WANTED bits, hence the ucas
	 *     loop.  Those newly-set WANTED bits, however, will not impact
	 *     our decisions.  Those LWPs have simply lost the race to
	 *     acquire the lock, and we don't consult those bits anyway;
	 *     we instead use the contents of the sleepqs as the truth
	 *     about who is waiting, and now new waiters will appear on
	 *     the sleepqs while we hold the op lock.
	 *
	 * ==> Wake waiters: sleepq locked, run down our private sleepq
	 *     and actually awaken all of the LWPs we selected earlier.
	 *
	 * If for some reason, the ucas fails because it page backing it
	 * was unmapped, all bets are off.  We still awaken the waiters.
	 * This is either a malicious attempt to screw things up, or a
	 * programming error, and we don't care about either one.
	 */
	sleepq_init(&wsq);

	futex_op_lock(f);
	mutex_spin_enter(f->fx_sq_lock);

	/*
	 * STEP 1
	 */

	/*
	 * POSIX dictates that any real-time waiters will acquire the
	 * lock in priority order.  This implies that a real-time
	 * read-waiter has priority over a non-real-time write-waiter,
	 * where we would otherwise prioritize waking the write-waiter.
	 */
	l_writer = LIST_FIRST(&f->fx_sqs[FUTEX_WRITERQ]);
	l_reader = LIST_FIRST(&f->fx_sqs[FUTEX_READERQ]);
	if (__predict_true(l_writer == NULL)) {
		/* We will wake all the readers. */
		sq = &f->fx_sqs[FUTEX_READERQ];
		nwaitersp = &f->fx_nwaiters[FUTEX_READERQ];
		nwake = allreaders = f->fx_nwaiters[FUTEX_READERQ];
		KASSERT(nwake >= 0 && nwake <= FUTEX_TID_MASK);
		if (__predict_false(nwake == 0)) {
			KASSERT(l_reader == NULL);
			newval = 0;
		} else {
			KASSERT(l_reader != NULL);
			newval = nwake;
		}
		l = l_reader;
	} else if (__predict_false(l_reader != NULL &&
				   futex_read_waiter_prio(l_reader) >
					(pri_writer = lwp_eprio(l_writer)))) {
		/*
		 * Count the number of real-time read-waiters that
		 * exceed the write-waiter's priority.  We will
		 * wake that many (we alreay know it's at least one).
		 */
		sq = &f->fx_sqs[FUTEX_READERQ];
		nwaitersp = &f->fx_nwaiters[FUTEX_READERQ];
		for (nwake = 1, l = LIST_NEXT(l_reader, l_sleepchain);
		     l != NULL && futex_read_waiter_prio(l) > pri_writer;
		     l = LIST_NEXT(l, l_sleepchain)) {
			nwake++;
		}
		KASSERT(nwake >= 0 && nwake <= FUTEX_TID_MASK);
		/* We know there is at least one write-waiter. */
		newval = nwake | FUTEX_WAITERS | FUTEX_RW_WRITE_WANTED;
		l = l_reader;
	} else {
		/* We will wake one writer. */
		sq = &f->fx_sqs[FUTEX_WRITERQ];
		nwaitersp = &f->fx_nwaiters[FUTEX_WRITERQ];
		nwake = 1;
		newval = FUTEX_RW_WRITE_LOCKED | l_writer->l_lid;
		if (__predict_false(f->fx_nwaiters[FUTEX_WRITERQ] > 1)) {
			KASSERT(LIST_NEXT(l_writer, l_sleepchain) != NULL);
			newval |= FUTEX_WAITERS | FUTEX_RW_WRITE_WANTED;
		} else if (__predict_true(f->fx_nwaiters[FUTEX_READERQ] != 0)) {
			KASSERT(!LIST_EMPTY(&f->fx_sqs[FUTEX_READERQ]));
			newval |= FUTEX_WAITERS;
		} else {
			KASSERT(LIST_EMPTY(&f->fx_sqs[FUTEX_READERQ]));
		}
		l = l_writer;
	}

	KASSERT(sq == &f->fx_sqs[FUTEX_READERQ] ||
		sq == &f->fx_sqs[FUTEX_WRITERQ]);
	while (nwake != 0 && l != NULL) {
		l_next = LIST_NEXT(l, l_sleepchain);
		KASSERT(l->l_syncobj == &futex_syncobj);
		KASSERT(l->l_wchan == (wchan_t)f);
		KASSERT(l->l_futex == f);
		KASSERT(l->l_sleepq == sq);
		KASSERT(l->l_mutex == f->fx_sq_lock);

		/*
		 * NULL wchan --> timeout will not wake LWP.
		 * NULL lock --> keep existing lock.
		 */
		sleepq_transfer(l, sq, &wsq, NULL/*wchan*/, futex_wmesg,
		    &sched_syncobj, NULL/*lock*/, false);

		KASSERT(*nwaitersp > 0);
		(*nwaitersp)--;
		nwoken++;
		nwake--;
		/* hold count adjusted below */
		l = l_next;
	}

	mutex_spin_exit(f->fx_sq_lock);

	/*
	 * STEP 2
	 */
	for (;; val = next) {
		error = ucas_int(uaddr, val, newval, &next);
		if (__predict_false(error != 0)) {
			/*
			 * The futex word has become unmapped.  All bets
			 * are off.  Break out of the loop and awaken the
			 * waiters; this is easier than trying to stuff
			 * them back into their previous sleepq, and the
			 * application is screwed anyway.
			 */
			break;
		}
		if (__predict_true(next == val)) {
			/*
			 * Successfully updated the futex word!
			 */
			break;
		}
		/*
		 * The only thing that could have possibly happened
		 * (barring some bug in the thread library) is that
		 * additional waiter bits arrived.  Those new waiters
		 * have lost the race to acquire the lock, but we
		 * need to preserve those bits.
		 */
		newval |= next & (FUTEX_WAITERS | FUTEX_RW_WRITE_WANTED);
	}

	mutex_spin_enter(f->fx_sq_lock);

	/*
	 * STEP 3
	 */

	LIST_FOREACH_SAFE(l, &wsq, l_sleepchain, l_next) {
		KASSERT(l->l_syncobj == &sched_syncobj);
		KASSERT(l->l_wchan == NULL);
		KASSERT(l->l_futex == f);
		KASSERT(l->l_sleepq == &wsq);
		KASSERT(l->l_mutex == f->fx_sq_lock);

		l->l_futex_wakesel = 0;
		l->l_futex = NULL;
		sleepq_remove(&wsq, l);
	}
	/* If we woke all-readers, ensure we will wake them all. */
	KASSERT(allreaders == 0 || allreaders == nwoken);
	KASSERT(allreaders == 0 || LIST_EMPTY(&f->fx_sqs[FUTEX_READERQ]));
	KASSERT(allreaders == 0 || f->fx_nwaiters[FUTEX_READERQ] == 0);

	mutex_spin_exit(f->fx_sq_lock);

	/* Adjust hold count. */
	futex_rele_count_not_last(f, nwoken);

	futex_op_unlock(f);

	/* Release the futex.  */
	futex_rele(f);

out:	if (__predict_true(error == 0)) {
		/* Return the number of waiters woken.  */
		*retval = nwoken;
	}

	/* Success!  */
	return error;
#endif
}

/*
 * do_futex(uaddr, op, val, timeout, uaddr2, val2, val3)
 *
 *	Implement the futex system call with all the parameters
 *	parsed out.
 */
int
do_futex(int *uaddr, int op, int val, const struct timespec *timeout,
    int *uaddr2, int val2, int val3, register_t *retval)
{
	const bool shared = (op & FUTEX_PRIVATE_FLAG) ? false : true;
	const clockid_t clkid = (op & FUTEX_CLOCK_REALTIME) ? CLOCK_REALTIME
							    : CLOCK_MONOTONIC;
	int error;

	op &= FUTEX_CMD_MASK;

	switch (op) {
	case FUTEX_WAIT:
		SDT_PROBE6(futex, func, wait, entry,
		    uaddr, val, FUTEX_BITSET_MATCH_ANY, timeout,
		    TIMER_RELTIME, op & ~FUTEX_CMD_MASK);
		error = futex_func_wait(shared, uaddr, val,
		    FUTEX_BITSET_MATCH_ANY, timeout, clkid, TIMER_RELTIME,
		    retval);
		SDT_PROBE1(futex, func, wait, exit, error);
		break;

	case FUTEX_WAIT_BITSET:
		SDT_PROBE6(futex, func, wait, entry,
		    uaddr, val, val3, timeout,
		    TIMER_ABSTIME, op & ~FUTEX_CMD_MASK);
		error = futex_func_wait(shared, uaddr, val, val3, timeout,
		    clkid, TIMER_ABSTIME, retval);
		SDT_PROBE1(futex, func, wait, exit, error);
		break;

	case FUTEX_WAKE:
		SDT_PROBE4(futex, func, wake, entry,
		    uaddr, val, FUTEX_BITSET_MATCH_ANY, op & ~FUTEX_CMD_MASK);
		error = futex_func_wake(shared, uaddr, val,
		    FUTEX_BITSET_MATCH_ANY, retval);
		SDT_PROBE2(futex, func, wake, exit, error, *retval);
		break;

	case FUTEX_WAKE_BITSET:
		SDT_PROBE4(futex, func, wake, entry,
		    uaddr, val, val3, op & ~FUTEX_CMD_MASK);
		error = futex_func_wake(shared, uaddr, val, val3, retval);
		SDT_PROBE2(futex, func, wake, exit, error, *retval);
		break;

	case FUTEX_REQUEUE:
		SDT_PROBE5(futex, func, requeue, entry,
		    uaddr, val, uaddr2, val2, op & ~FUTEX_CMD_MASK);
		error = futex_func_requeue(shared, op, uaddr, val, uaddr2,
		    val2, 0, retval);
		SDT_PROBE2(futex, func, requeue, exit, error, *retval);
		break;

	case FUTEX_CMP_REQUEUE:
		SDT_PROBE6(futex, func, cmp_requeue, entry,
		    uaddr, val, uaddr2, val2, val3, op & ~FUTEX_CMD_MASK);
		error = futex_func_requeue(shared, op, uaddr, val, uaddr2,
		    val2, val3, retval);
		SDT_PROBE2(futex, func, cmp_requeue, exit, error, *retval);
		break;

	case FUTEX_WAKE_OP:
		SDT_PROBE6(futex, func, wake_op, entry,
		    uaddr, val, uaddr2, val2, val3, op & ~FUTEX_CMD_MASK);
		error = futex_func_wake_op(shared, uaddr, val, uaddr2, val2,
		    val3, retval);
		SDT_PROBE2(futex, func, wake_op, exit, error, *retval);
		break;

	case FUTEX_NETBSD_RW_WAIT:
		SDT_PROBE5(futex, func, rw_wait, entry,
		    uaddr, val, val3, timeout, op & ~FUTEX_CMD_MASK);
		error = futex_func_rw_wait(shared, uaddr, val, val3,
		    timeout, clkid, retval);
		SDT_PROBE1(futex, func, rw_wait, exit, error);
		break;

	case FUTEX_NETBSD_RW_HANDOFF:
		SDT_PROBE3(futex, func, rw_handoff, entry,
		    uaddr, val, op & ~FUTEX_CMD_MASK);
		error = futex_func_rw_handoff(shared, uaddr, val, retval);
		SDT_PROBE2(futex, func, rw_handoff, exit, error, *retval);
		break;

	case FUTEX_FD:
	default:
		error = ENOSYS;
		break;
	}

	return error;
}

/*
 * sys___futex(l, uap, retval)
 *
 *	__futex(2) system call: generic futex operations.
 */
int
sys___futex(struct lwp *l, const struct sys___futex_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int *) uaddr;
		syscallarg(int) op;
		syscallarg(int) val;
		syscallarg(const struct timespec *) timeout;
		syscallarg(int *) uaddr2;
		syscallarg(int) val2;
		syscallarg(int) val3;
	} */
	struct timespec ts, *tsp;
	int error;

	/*
	 * Copy in the timeout argument, if specified.
	 */
	if (SCARG(uap, timeout)) {
		error = copyin(SCARG(uap, timeout), &ts, sizeof(ts));
		if (error)
			return error;
		tsp = &ts;
	} else {
		tsp = NULL;
	}

	return do_futex(SCARG(uap, uaddr), SCARG(uap, op), SCARG(uap, val),
	    tsp, SCARG(uap, uaddr2), SCARG(uap, val2), SCARG(uap, val3),
	    retval);
}

/*
 * sys___futex_set_robust_list(l, uap, retval)
 *
 *	__futex_set_robust_list(2) system call for robust futexes.
 */
int
sys___futex_set_robust_list(struct lwp *l,
    const struct sys___futex_set_robust_list_args *uap, register_t *retval)
{
	/* {
		syscallarg(void *) head;
		syscallarg(size_t) len;
	} */
	void *head = SCARG(uap, head);

	if (SCARG(uap, len) != _FUTEX_ROBUST_HEAD_SIZE)
		return EINVAL;
	if ((uintptr_t)head % sizeof(u_long))
		return EINVAL;

	l->l_robust_head = (uintptr_t)head;

	return 0;
}

/*
 * sys___futex_get_robust_list(l, uap, retval)
 *
 *	__futex_get_robust_list(2) system call for robust futexes.
 */
int
sys___futex_get_robust_list(struct lwp *l,
    const struct sys___futex_get_robust_list_args *uap, register_t *retval)
{
	/* {
		syscallarg(lwpid_t) lwpid;
		syscallarg(void **) headp;
		syscallarg(size_t *) lenp;
	} */
	void *head;
	const size_t len = _FUTEX_ROBUST_HEAD_SIZE;
	int error;

	error = futex_robust_head_lookup(l, SCARG(uap, lwpid), &head);
	if (error)
		return error;

	/* Copy out the head pointer and the head structure length. */
	error = copyout(&head, SCARG(uap, headp), sizeof(head));
	if (__predict_true(error == 0)) {
		error = copyout(&len, SCARG(uap, lenp), sizeof(len));
	}

	return error;
}

/*
 * release_futex(uva, tid)
 *
 *	Try to release the robust futex at uva in the current process
 *	on lwp exit.  If anything goes wrong, silently fail.  It is the
 *	userland program's obligation to arrange correct behaviour.
 */
static void
release_futex(uintptr_t const uptr, lwpid_t const tid, bool const is_pi,
    bool const is_pending)
{
	int *uaddr;
	struct futex *f;
	int oldval, newval, actual;
	int error;
	bool shared;

	/* If it's misaligned, tough.  */
	if (__predict_false(uptr & 3))
		return;
	uaddr = (int *)uptr;

	error = futex_load(uaddr, &oldval);
	if (__predict_false(error))
		return;

	/*
	 * There are two race conditions we need to handle here:
	 *
	 * 1. User space cleared the futex word but died before
	 *    being able to issue the wakeup.  No wakeups will
	 *    ever be issued, oops!
	 *
	 * 2. Awakened waiter died before being able to acquire
	 *    the futex in user space.  Any other waiters are
	 *    now stuck, oops!
	 *
	 * In both of these cases, the futex word will be 0 (because
	 * it's updated before the wake is issued).  The best we can
	 * do is detect this situation if it's the pending futex and
	 * issue a wake without modifying the futex word.
	 *
	 * XXX eventual PI handling?
	 */
	if (__predict_false(is_pending && (oldval & ~FUTEX_WAITERS) == 0)) {
		register_t retval;
		error = futex_func_wake(/*shared*/true, uaddr, 1,
		    FUTEX_BITSET_MATCH_ANY, &retval);
		if (error != 0 || retval == 0)
			(void) futex_func_wake(/*shared*/false, uaddr, 1,
			    FUTEX_BITSET_MATCH_ANY, &retval);
		return;
	}

	/* Optimistically test whether we need to do anything at all.  */
	if ((oldval & FUTEX_TID_MASK) != tid)
		return;

	/*
	 * We need to handle the case where this thread owned the futex,
	 * but it was uncontended.  In this case, there won't be any
	 * kernel state to look up.  All we can do is mark the futex
	 * as a zombie to be mopped up the next time another thread
	 * attempts to acquire it.
	 *
	 * N.B. It's important to ensure to set FUTEX_OWNER_DIED in
	 * this loop, even if waiters appear while we're are doing
	 * so.  This is beause FUTEX_WAITERS is set by user space
	 * before calling __futex() to wait, and the futex needs
	 * to be marked as a zombie when the new waiter gets into
	 * the kernel.
	 */
	if ((oldval & FUTEX_WAITERS) == 0) {
		do {
			error = futex_load(uaddr, &oldval);
			if (error)
				return;
			if ((oldval & FUTEX_TID_MASK) != tid)
				return;
			newval = oldval | FUTEX_OWNER_DIED;
			error = ucas_int(uaddr, oldval, newval, &actual);
			if (error)
				return;
		} while (actual != oldval);

		/*
		 * If where is still no indication of waiters, then there is
		 * no more work for us to do.
		 */
		if ((oldval & FUTEX_WAITERS) == 0)
			return;
	}

	/*
	 * Look for a futex.  Try shared first, then private.  If we
	 * can't fine one, tough.
	 *
	 * Note: the assumption here is that anyone placing a futex on
	 * the robust list is adhering to the rules, regardless of the
	 * futex class.
	 */
	for (f = NULL, shared = true; f == NULL; shared = false) {
		error = futex_lookup(uaddr, shared, FUTEX_CLASS_ANY, &f);
		if (error)
			return;
		if (f == NULL && shared == false)
			return;
	}

	/* Work under the futex op lock.  */
	futex_op_lock(f);

	/*
	 * Fetch the word: if the tid doesn't match ours, skip;
	 * otherwise, set the owner-died bit, atomically.
	 */
	do {
		error = futex_load(uaddr, &oldval);
		if (error)
			goto out;
		if ((oldval & FUTEX_TID_MASK) != tid)
			goto out;
		newval = oldval | FUTEX_OWNER_DIED;
		error = ucas_int(uaddr, oldval, newval, &actual);
		if (error)
			goto out;
	} while (actual != oldval);

	/*
	 * If there may be waiters, try to wake one.  If anything goes
	 * wrong, tough.
	 *
	 * XXX eventual PI handling?
	 */
	if (oldval & FUTEX_WAITERS) {
		(void)futex_wake(f, FUTEX_WRITERQ, 1,
				 NULL, FUTEX_WRITERQ, 0,
				 FUTEX_BITSET_MATCH_ANY);
	}

	/* Unlock the queue and release the futex.  */
out:	futex_op_unlock(f);
	futex_rele(f);
}

/*
 * futex_robust_head_lookup(l, lwpid)
 *
 *	Helper function to look up a robust head by LWP ID.
 */
int
futex_robust_head_lookup(struct lwp *l, lwpid_t lwpid, void **headp)
{
	struct proc *p = l->l_proc;

	/* Find the other lwp, if requested; otherwise use our robust head.  */
	if (lwpid) {
		mutex_enter(p->p_lock);
		l = lwp_find(p, lwpid);
		if (l == NULL) {
			mutex_exit(p->p_lock);
			return ESRCH;
		}
		*headp = (void *)l->l_robust_head;
		mutex_exit(p->p_lock);
	} else {
		*headp = (void *)l->l_robust_head;
	}
	return 0;
}

/*
 * futex_fetch_robust_head(uaddr)
 *
 *	Helper routine to fetch the futex robust list head that
 *	handles 32-bit binaries running on 64-bit kernels.
 */
static int
futex_fetch_robust_head(uintptr_t uaddr, u_long *rhead)
{
#ifdef _LP64
	if (curproc->p_flag & PK_32) {
		uint32_t rhead32[_FUTEX_ROBUST_HEAD_NWORDS];
		int error;

		error = copyin((void *)uaddr, rhead32, sizeof(rhead32));
		if (__predict_true(error == 0)) {
			for (int i = 0; i < _FUTEX_ROBUST_HEAD_NWORDS; i++) {
				if (i == _FUTEX_ROBUST_HEAD_OFFSET) {
					/*
					 * Make sure the offset is sign-
					 * extended.
					 */
					rhead[i] = (int32_t)rhead32[i];
				} else {
					rhead[i] = rhead32[i];
				}
			}
		}
		return error;
	}
#endif /* _L64 */

	return copyin((void *)uaddr, rhead,
	    sizeof(*rhead) * _FUTEX_ROBUST_HEAD_NWORDS);
}

/*
 * futex_decode_robust_word(word)
 *
 *	Decode a robust futex list word into the entry and entry
 *	properties.
 */
static inline void
futex_decode_robust_word(uintptr_t const word, uintptr_t * const entry,
    bool * const is_pi)
{
	*is_pi = (word & _FUTEX_ROBUST_ENTRY_PI) ? true : false;
	*entry = word & ~_FUTEX_ROBUST_ENTRY_PI;
}

/*
 * futex_fetch_robust_entry(uaddr)
 *
 *	Helper routine to fetch and decode a robust futex entry
 *	that handles 32-bit binaries running on 64-bit kernels.
 */
static int
futex_fetch_robust_entry(uintptr_t const uaddr, uintptr_t * const valp,
    bool * const is_pi)
{
	uintptr_t val = 0;
	int error = 0;

#ifdef _LP64
	if (curproc->p_flag & PK_32) {
		uint32_t val32;

		error = ufetch_32((uint32_t *)uaddr, &val32);
		if (__predict_true(error == 0))
			val = val32;
	} else
#endif /* _LP64 */
		error = ufetch_long((u_long *)uaddr, (u_long *)&val);
	if (__predict_false(error))
		return error;

	futex_decode_robust_word(val, valp, is_pi);
	return 0;
}

/*
 * futex_release_all_lwp(l, tid)
 *
 *	Release all l's robust futexes.  If anything looks funny in
 *	the process, give up -- it's userland's responsibility to dot
 *	the i's and cross the t's.
 */
void
futex_release_all_lwp(struct lwp * const l, lwpid_t const tid)
{
	u_long rhead[_FUTEX_ROBUST_HEAD_NWORDS];
	int limit = 1000000;
	int error;

	/* If there's no robust list there's nothing to do. */
	if (l->l_robust_head == 0)
		return;

	/* Read the final snapshot of the robust list head. */
	error = futex_fetch_robust_head(l->l_robust_head, rhead);
	if (error) {
		printf("WARNING: pid %jd (%s) lwp %jd tid %jd:"
		    " unmapped robust futex list head\n",
		    (uintmax_t)l->l_proc->p_pid, l->l_proc->p_comm,
		    (uintmax_t)l->l_lid, (uintmax_t)tid);
		return;
	}

	const long offset = (long)rhead[_FUTEX_ROBUST_HEAD_OFFSET];

	uintptr_t next, pending;
	bool is_pi, pending_is_pi;

	futex_decode_robust_word(rhead[_FUTEX_ROBUST_HEAD_LIST],
	    &next, &is_pi);
	futex_decode_robust_word(rhead[_FUTEX_ROBUST_HEAD_PENDING],
	    &pending, &pending_is_pi);

	/*
	 * Walk down the list of locked futexes and release them, up
	 * to one million of them before we give up.
	 */

	while (next != l->l_robust_head && limit-- > 0) {
		/* pending handled below. */
		if (next != pending)
			release_futex(next + offset, tid, is_pi, false);
		error = futex_fetch_robust_entry(next, &next, &is_pi);
		if (error)
			break;
		preempt_point();
	}
	if (limit <= 0) {
		printf("WARNING: pid %jd (%s) lwp %jd tid %jd:"
		    " exhausted robust futex limit\n",
		    (uintmax_t)l->l_proc->p_pid, l->l_proc->p_comm,
		    (uintmax_t)l->l_lid, (uintmax_t)tid);
	}

	/* If there's a pending futex, it may need to be released too. */
	if (pending != 0) {
		release_futex(pending + offset, tid, pending_is_pi, true);
	}
}
