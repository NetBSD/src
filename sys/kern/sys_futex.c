/*	$NetBSD: sys_futex.c,v 1.18 2022/04/21 12:05:13 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sys_futex.c,v 1.18 2022/04/21 12:05:13 riastradh Exp $");

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
 *		membar_acquire();
 *
 *		...
 *
 *		// Release the lock.  Optimistically assume there are
 *		// no waiters first until demonstrated otherwise.
 *		membar_release();
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/condvar.h>
#include <sys/futex.h>
#include <sys/mutex.h>
#include <sys/rbtree.h>
#include <sys/queue.h>

#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>

#include <uvm/uvm_extern.h>

/*
 * Lock order:
 *
 *	futex_tab.lock
 *	futex::fx_qlock			ordered by kva of struct futex
 *	 -> futex_wait::fw_lock		only one at a time
 *	futex_wait::fw_lock		only one at a time
 *	 -> futex::fx_abortlock		only one at a time
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
 */
struct futex {
	union futex_key		fx_key;
	unsigned long		fx_refcnt;
	bool			fx_shared;
	bool			fx_on_tree;
	struct rb_node		fx_node;

	kmutex_t			fx_qlock;
	TAILQ_HEAD(, futex_wait)	fx_queue;

	kmutex_t			fx_abortlock;
	LIST_HEAD(, futex_wait)		fx_abortlist;
	kcondvar_t			fx_abortcv;
};

/*
 * struct futex_wait
 *
 *	State for a thread to wait on a futex.  Threads wait on fw_cv
 *	for fw_bitset to be set to zero.  The thread may transition to
 *	a different futex queue at any time under the futex's lock.
 */
struct futex_wait {
	kmutex_t		fw_lock;
	kcondvar_t		fw_cv;
	struct futex		*fw_futex;
	TAILQ_ENTRY(futex_wait)	fw_entry;	/* queue lock */
	LIST_ENTRY(futex_wait)	fw_abort;	/* queue abortlock */
	int			fw_bitset;
	bool			fw_aborting;	/* fw_lock */
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
		return +1;
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

static void	futex_wait_dequeue(struct futex_wait *, struct futex *);

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
 * futex_queue_init(f)
 *
 *	Initialize the futex queue.  Caller must call futex_queue_fini
 *	when done.
 *
 *	Never sleeps.
 */
static void
futex_queue_init(struct futex *f)
{

	mutex_init(&f->fx_qlock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&f->fx_abortlock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&f->fx_abortcv, "fqabort");
	LIST_INIT(&f->fx_abortlist);
	TAILQ_INIT(&f->fx_queue);
}

/*
 * futex_queue_drain(f)
 *
 *	Wait for any aborting waiters in f; then empty the queue of
 *	any stragglers and wake them.  Caller must guarantee no new
 *	references to f.
 *
 *	May sleep.
 */
static void
futex_queue_drain(struct futex *f)
{
	struct futex_wait *fw, *fw_next;

	mutex_enter(&f->fx_abortlock);
	while (!LIST_EMPTY(&f->fx_abortlist))
		cv_wait(&f->fx_abortcv, &f->fx_abortlock);
	mutex_exit(&f->fx_abortlock);

	mutex_enter(&f->fx_qlock);
	TAILQ_FOREACH_SAFE(fw, &f->fx_queue, fw_entry, fw_next) {
		mutex_enter(&fw->fw_lock);
		futex_wait_dequeue(fw, f);
		cv_broadcast(&fw->fw_cv);
		mutex_exit(&fw->fw_lock);
	}
	mutex_exit(&f->fx_qlock);
}

/*
 * futex_queue_fini(fq)
 *
 *	Finalize the futex queue initialized by futex_queue_init.  Queue
 *	must be empty.  Caller must not use f again until a subsequent
 *	futex_queue_init.
 */
static void
futex_queue_fini(struct futex *f)
{

	KASSERT(TAILQ_EMPTY(&f->fx_queue));
	KASSERT(LIST_EMPTY(&f->fx_abortlist));
	mutex_destroy(&f->fx_qlock);
	mutex_destroy(&f->fx_abortlock);
	cv_destroy(&f->fx_abortcv);
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
futex_create(union futex_key *fk, bool shared)
{
	struct futex *f;

	f = kmem_alloc(sizeof(*f), KM_NOSLEEP);
	if (f == NULL) {
		futex_key_fini(fk, shared);
		return NULL;
	}
	f->fx_key = *fk;
	f->fx_refcnt = 1;
	f->fx_shared = shared;
	f->fx_on_tree = false;
	futex_queue_init(f);

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

	/* Drain and destroy the private queue.  */
	futex_queue_drain(f);
	futex_queue_fini(f);

	futex_key_fini(&f->fx_key, f->fx_shared);

	kmem_free(f, sizeof(*f));
}

/*
 * futex_hold(f)
 *
 *	Attempt to acquire a reference to f.  Return 0 on success,
 *	ENFILE on too many references.
 *
 *	Never sleeps.
 */
static int
futex_hold(struct futex *f)
{
	unsigned long refcnt;

	do {
		refcnt = atomic_load_relaxed(&f->fx_refcnt);
		if (refcnt == ULONG_MAX)
			return ENFILE;
	} while (atomic_cas_ulong(&f->fx_refcnt, refcnt, refcnt + 1) != refcnt);

	return 0;
}

/*
 * futex_rele(f)
 *
 *	Release a reference to f acquired with futex_create or
 *	futex_hold.
 *
 *	May sleep to free f.
 */
static void
futex_rele(struct futex *f)
{
	unsigned long refcnt;

	ASSERT_SLEEPABLE();

	do {
		refcnt = atomic_load_relaxed(&f->fx_refcnt);
		if (refcnt == 1)
			goto trylast;
#ifndef __HAVE_ATOMIC_AS_MEMBAR
		membar_release();
#endif
	} while (atomic_cas_ulong(&f->fx_refcnt, refcnt, refcnt - 1) != refcnt);
	return;

trylast:
	mutex_enter(&futex_tab.lock);
	if (atomic_dec_ulong_nv(&f->fx_refcnt) == 0) {
#ifndef __HAVE_ATOMIC_AS_MEMBAR
		membar_acquire();
#endif
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

/*
 * futex_rele_not_last(f)
 *
 *	Release a reference to f acquired with futex_create or
 *	futex_hold.
 *
 *	This version asserts that we are not dropping the last
 *	reference to f.
 */
static void
futex_rele_not_last(struct futex *f)
{
	unsigned long refcnt;

	do {
		refcnt = atomic_load_relaxed(&f->fx_refcnt);
		KASSERT(refcnt > 1);
	} while (atomic_cas_ulong(&f->fx_refcnt, refcnt, refcnt - 1) != refcnt);
}

/*
 * futex_lookup_by_key(key, shared, &f)
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
futex_lookup_by_key(union futex_key *fk, bool shared, struct futex **fp)
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
		error = futex_hold(f);
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
 * futex_lookup(uaddr, shared, &f)
 *
 *	Find a futex at the userland pointer uaddr in the current
 *	process's VM space.  On success, return the futex in f and
 *	increment its reference count.
 *
 *	Caller must call futex_rele when done.
 */
static int
futex_lookup(int *uaddr, bool shared, struct futex **fp)
{
	union futex_key fk;
	struct vmspace *vm = curproc->p_vmspace;
	vaddr_t va = (vaddr_t)uaddr;
	int error;

	/*
	 * Reject unaligned user pointers so we don't cross page
	 * boundaries and so atomics will work.
	 */
	if ((va & 3) != 0)
		return EINVAL;

	/* Look it up. */
	error = futex_key_init(&fk, vm, va, shared);
	if (error)
		return error;

	error = futex_lookup_by_key(&fk, shared, fp);
	futex_key_fini(&fk, shared);
	if (error)
		return error;

	KASSERT(*fp == NULL || (*fp)->fx_shared == shared);
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
 * futex_lookup_create(uaddr, shared, &f)
 *
 *	Find or create a futex at the userland pointer uaddr in the
 *	current process's VM space.  On success, return the futex in f
 *	and increment its reference count.
 *
 *	Caller must call futex_rele when done.
 */
static int
futex_lookup_create(int *uaddr, bool shared, struct futex **fp)
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
	if ((va & 3) != 0)
		return EINVAL;

	error = futex_key_init(&fk, vm, va, shared);
	if (error)
		return error;

	/*
	 * Optimistically assume there already is one, and try to find
	 * it.
	 */
	error = futex_lookup_by_key(&fk, shared, fp);
	if (error || *fp != NULL) {
		/*
		 * We either found one, or there was an error.
		 * In either case, we are done with the key.
		 */
		futex_key_fini(&fk, shared);
		goto out;
	}

	/*
	 * Create a futex record.  This transfers ownership of the key
	 * in all cases.
	 */
	f = futex_create(&fk, shared);
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
 * futex_wait_init(fw, bitset)
 *
 *	Initialize a record for a thread to wait on a futex matching
 *	the specified bit set.  Should be passed to futex_wait_enqueue
 *	before futex_wait, and should be passed to futex_wait_fini when
 *	done.
 */
static void
futex_wait_init(struct futex_wait *fw, int bitset)
{

	KASSERT(bitset);

	mutex_init(&fw->fw_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&fw->fw_cv, "futex");
	fw->fw_futex = NULL;
	fw->fw_bitset = bitset;
	fw->fw_aborting = false;
}

/*
 * futex_wait_fini(fw)
 *
 *	Finalize a record for a futex waiter.  Must not be on any
 *	futex's queue.
 */
static void
futex_wait_fini(struct futex_wait *fw)
{

	KASSERT(fw->fw_futex == NULL);

	cv_destroy(&fw->fw_cv);
	mutex_destroy(&fw->fw_lock);
}

/*
 * futex_wait_enqueue(fw, f)
 *
 *	Put fw on the futex queue.  Must be done before futex_wait.
 *	Caller must hold fw's lock and f's lock, and fw must not be on
 *	any existing futex's waiter list.
 */
static void
futex_wait_enqueue(struct futex_wait *fw, struct futex *f)
{

	KASSERT(mutex_owned(&f->fx_qlock));
	KASSERT(mutex_owned(&fw->fw_lock));
	KASSERT(fw->fw_futex == NULL);
	KASSERT(!fw->fw_aborting);

	fw->fw_futex = f;
	TAILQ_INSERT_TAIL(&f->fx_queue, fw, fw_entry);
}

/*
 * futex_wait_dequeue(fw, f)
 *
 *	Remove fw from the futex queue.  Precludes subsequent
 *	futex_wait until a futex_wait_enqueue.  Caller must hold fw's
 *	lock and f's lock, and fw must be on f.
 */
static void
futex_wait_dequeue(struct futex_wait *fw, struct futex *f)
{

	KASSERT(mutex_owned(&f->fx_qlock));
	KASSERT(mutex_owned(&fw->fw_lock));
	KASSERT(fw->fw_futex == f);

	TAILQ_REMOVE(&f->fx_queue, fw, fw_entry);
	fw->fw_futex = NULL;
}

/*
 * futex_wait_abort(fw)
 *
 *	Caller is no longer waiting for fw.  Remove it from any queue
 *	if it was on one.  Caller must hold fw->fw_lock.
 */
static void
futex_wait_abort(struct futex_wait *fw)
{
	struct futex *f;

	KASSERT(mutex_owned(&fw->fw_lock));

	/*
	 * Grab the futex queue.  It can't go away as long as we hold
	 * fw_lock.  However, we can't take the queue lock because
	 * that's a lock order reversal.
	 */
	f = fw->fw_futex;

	/* Put us on the abort list so that fq won't go away.  */
	mutex_enter(&f->fx_abortlock);
	LIST_INSERT_HEAD(&f->fx_abortlist, fw, fw_abort);
	mutex_exit(&f->fx_abortlock);

	/*
	 * Mark fw as aborting so it won't lose wakeups and won't be
	 * transferred to any other queue.
	 */
	fw->fw_aborting = true;

	/* f is now stable, so we can release fw_lock.  */
	mutex_exit(&fw->fw_lock);

	/* Now we can remove fw under the queue lock.  */
	mutex_enter(&f->fx_qlock);
	mutex_enter(&fw->fw_lock);
	futex_wait_dequeue(fw, f);
	mutex_exit(&fw->fw_lock);
	mutex_exit(&f->fx_qlock);

	/*
	 * Finally, remove us from the abort list and notify anyone
	 * waiting for the abort to complete if we were the last to go.
	 */
	mutex_enter(&f->fx_abortlock);
	LIST_REMOVE(fw, fw_abort);
	if (LIST_EMPTY(&f->fx_abortlist))
		cv_broadcast(&f->fx_abortcv);
	mutex_exit(&f->fx_abortlock);

	/*
	 * Release our reference to the futex now that we are not
	 * waiting for it.
	 */
	futex_rele(f);

	/*
	 * Reacquire the fw lock as caller expects.  Verify that we're
	 * aborting and no longer associated with a futex.
	 */
	mutex_enter(&fw->fw_lock);
	KASSERT(fw->fw_aborting);
	KASSERT(fw->fw_futex == NULL);
}

/*
 * futex_wait(fw, deadline, clkid)
 *
 *	fw must be a waiter on a futex's queue.  Wait until deadline on
 *	the clock clkid, or forever if deadline is NULL, for a futex
 *	wakeup.  Return 0 on explicit wakeup or destruction of futex,
 *	ETIMEDOUT on timeout, EINTR/ERESTART on signal.  Either way, fw
 *	will no longer be on a futex queue on return.
 */
static int
futex_wait(struct futex_wait *fw, const struct timespec *deadline,
    clockid_t clkid)
{
	int error = 0;

	/* Test and wait under the wait lock.  */
	mutex_enter(&fw->fw_lock);

	for (;;) {
		/* If we're done yet, stop and report success.  */
		if (fw->fw_bitset == 0 || fw->fw_futex == NULL) {
			error = 0;
			break;
		}

		/* If anything went wrong in the last iteration, stop.  */
		if (error)
			break;

		/* Not done yet.  Wait.  */
		if (deadline) {
			struct timespec ts;

			/* Check our watch.  */
			error = clock_gettime1(clkid, &ts);
			if (error)
				break;

			/* If we're past the deadline, ETIMEDOUT.  */
			if (timespeccmp(deadline, &ts, <=)) {
				error = ETIMEDOUT;
				break;
			}

			/* Count how much time is left.  */
			timespecsub(deadline, &ts, &ts);

			/* Wait for that much time, allowing signals.  */
			error = cv_timedwait_sig(&fw->fw_cv, &fw->fw_lock,
			    tstohz(&ts));
		} else {
			/* Wait indefinitely, allowing signals. */
			error = cv_wait_sig(&fw->fw_cv, &fw->fw_lock);
		}
	}

	/*
	 * If we were woken up, the waker will have removed fw from the
	 * queue.  But if anything went wrong, we must remove fw from
	 * the queue ourselves.  While here, convert EWOULDBLOCK to
	 * ETIMEDOUT.
	 */
	if (error) {
		futex_wait_abort(fw);
		if (error == EWOULDBLOCK)
			error = ETIMEDOUT;
	}

	mutex_exit(&fw->fw_lock);

	return error;
}

/*
 * futex_wake(f, nwake, f2, nrequeue, bitset)
 *
 *	Wake up to nwake waiters on f matching bitset; then, if f2 is
 *	provided, move up to nrequeue remaining waiters on f matching
 *	bitset to f2.  Return the number of waiters actually woken.
 *	Caller must hold the locks of f and f2, if provided.
 */
static unsigned
futex_wake(struct futex *f, unsigned nwake, struct futex *f2,
    unsigned nrequeue, int bitset)
{
	struct futex_wait *fw, *fw_next;
	unsigned nwoken = 0;
	int hold_error __diagused;

	KASSERT(mutex_owned(&f->fx_qlock));
	KASSERT(f2 == NULL || mutex_owned(&f2->fx_qlock));

	/* Wake up to nwake waiters, and count the number woken.  */
	TAILQ_FOREACH_SAFE(fw, &f->fx_queue, fw_entry, fw_next) {
		if ((fw->fw_bitset & bitset) == 0)
			continue;
		if (nwake > 0) {
			mutex_enter(&fw->fw_lock);
			if (__predict_false(fw->fw_aborting)) {
				mutex_exit(&fw->fw_lock);
				continue;
			}
			futex_wait_dequeue(fw, f);
			fw->fw_bitset = 0;
			cv_broadcast(&fw->fw_cv);
			mutex_exit(&fw->fw_lock);
			nwake--;
			nwoken++;
			/*
			 * Drop the futex reference on behalf of the
			 * waiter.  We assert this is not the last
			 * reference on the futex (our caller should
			 * also have one).
			 */
			futex_rele_not_last(f);
		} else {
			break;
		}
	}

	if (f2) {
		/* Move up to nrequeue waiters from f's queue to f2's queue. */
		TAILQ_FOREACH_SAFE(fw, &f->fx_queue, fw_entry, fw_next) {
			if ((fw->fw_bitset & bitset) == 0)
				continue;
			if (nrequeue > 0) {
				mutex_enter(&fw->fw_lock);
				if (__predict_false(fw->fw_aborting)) {
					mutex_exit(&fw->fw_lock);
					continue;
				}
				futex_wait_dequeue(fw, f);
				futex_wait_enqueue(fw, f2);
				mutex_exit(&fw->fw_lock);
				nrequeue--;
				/*
				 * Transfer the reference from f to f2.
				 * As above, we assert that we are not
				 * dropping the last reference to f here.
				 *
				 * XXX futex_hold() could theoretically
				 * XXX fail here.
				 */
				futex_rele_not_last(f);
				hold_error = futex_hold(f2);
				KASSERT(hold_error == 0);
			} else {
				break;
			}
		}
	} else {
		KASSERT(nrequeue == 0);
	}

	/* Return the number of waiters woken.  */
	return nwoken;
}

/*
 * futex_queue_lock(f)
 *
 *	Acquire the queue lock of f.  Pair with futex_queue_unlock.  Do
 *	not use if caller needs to acquire two locks; use
 *	futex_queue_lock2 instead.
 */
static void
futex_queue_lock(struct futex *f)
{
	mutex_enter(&f->fx_qlock);
}

/*
 * futex_queue_unlock(f)
 *
 *	Release the queue lock of f.
 */
static void
futex_queue_unlock(struct futex *f)
{
	mutex_exit(&f->fx_qlock);
}

/*
 * futex_queue_lock2(f, f2)
 *
 *	Acquire the queue locks of both f and f2, which may be null, or
 *	which may have the same underlying queue.  If they are
 *	distinct, an arbitrary total order is chosen on the locks.
 *
 *	Callers should only ever acquire multiple queue locks
 *	simultaneously using futex_queue_lock2.
 */
static void
futex_queue_lock2(struct futex *f, struct futex *f2)
{

	/*
	 * If both are null, do nothing; if one is null and the other
	 * is not, lock the other and be done with it.
	 */
	if (f == NULL && f2 == NULL) {
		return;
	} else if (f == NULL) {
		mutex_enter(&f2->fx_qlock);
		return;
	} else if (f2 == NULL) {
		mutex_enter(&f->fx_qlock);
		return;
	}

	/* If both futexes are the same, acquire only one. */
	if (f == f2) {
		mutex_enter(&f->fx_qlock);
		return;
	}

	/* Otherwise, use the ordering on the kva of the futex pointer.  */
	if ((uintptr_t)f < (uintptr_t)f2) {
		mutex_enter(&f->fx_qlock);
		mutex_enter(&f2->fx_qlock);
	} else {
		mutex_enter(&f2->fx_qlock);
		mutex_enter(&f->fx_qlock);
	}
}

/*
 * futex_queue_unlock2(f, f2)
 *
 *	Release the queue locks of both f and f2, which may be null, or
 *	which may have the same underlying queue.
 */
static void
futex_queue_unlock2(struct futex *f, struct futex *f2)
{

	/*
	 * If both are null, do nothing; if one is null and the other
	 * is not, unlock the other and be done with it.
	 */
	if (f == NULL && f2 == NULL) {
		return;
	} else if (f == NULL) {
		mutex_exit(&f2->fx_qlock);
		return;
	} else if (f2 == NULL) {
		mutex_exit(&f->fx_qlock);
		return;
	}

	/* If both futexes are the same, release only one. */
	if (f == f2) {
		mutex_exit(&f->fx_qlock);
		return;
	}

	/* Otherwise, use the ordering on the kva of the futex pointer.  */
	if ((uintptr_t)f < (uintptr_t)f2) {
		mutex_exit(&f2->fx_qlock);
		mutex_exit(&f->fx_qlock);
	} else {
		mutex_exit(&f->fx_qlock);
		mutex_exit(&f2->fx_qlock);
	}
}

/*
 * futex_func_wait(uaddr, val, val3, timeout, clkid, clkflags, retval)
 *
 *	Implement futex(FUTEX_WAIT).
 */
static int
futex_func_wait(bool shared, int *uaddr, int val, int val3,
    const struct timespec *timeout, clockid_t clkid, int clkflags,
    register_t *retval)
{
	struct futex *f;
	struct futex_wait wait, *fw = &wait;
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
		error = clock_gettime1(clkid, &ts);
		if (error)
			return error;
		timespecadd(&ts, timeout, &ts);
		deadline = &ts;
	}

	/* Get the futex, creating it if necessary.  */
	error = futex_lookup_create(uaddr, shared, &f);
	if (error)
		return error;
	KASSERT(f);

	/* Get ready to wait.  */
	futex_wait_init(fw, val3);

	/*
	 * Under the queue lock, check the value again: if it has
	 * already changed, EAGAIN; otherwise enqueue the waiter.
	 * Since FUTEX_WAKE will use the same lock and be done after
	 * modifying the value, the order in which we check and enqueue
	 * is immaterial.
	 */
	futex_queue_lock(f);
	if (!futex_test(uaddr, val)) {
		futex_queue_unlock(f);
		error = EAGAIN;
		goto out;
	}
	mutex_enter(&fw->fw_lock);
	futex_wait_enqueue(fw, f);
	mutex_exit(&fw->fw_lock);
	futex_queue_unlock(f);

	/*
	 * We cannot drop our reference to the futex here, because
	 * we might be enqueued on a different one when we are awakened.
	 * The references will be managed on our behalf in the requeue
	 * and wake cases.
	 */
	f = NULL;

	/* Wait. */
	error = futex_wait(fw, deadline, clkid);
	if (error)
		goto out;

	/* Return 0 on success, error on failure. */
	*retval = 0;

out:	if (f != NULL)
		futex_rele(f);
	futex_wait_fini(fw);
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
	error = futex_lookup(uaddr, shared, &f);
	if (error)
		goto out;

	/* If there's no futex, there are no waiters to wake.  */
	if (f == NULL)
		goto out;

	/*
	 * Under f's queue lock, wake the waiters and remember the
	 * number woken.
	 */
	futex_queue_lock(f);
	nwoken = futex_wake(f, val, NULL, 0, val3);
	futex_queue_unlock(f);

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
	error = futex_lookup(uaddr, shared, &f);
	if (error)
		goto out;

	/* If there is none, nothing to do. */
	if (f == NULL)
		goto out;

	/*
	 * We may need to create the destination futex because it's
	 * entirely possible it does not currently have any waiters.
	 */
	error = futex_lookup_create(uaddr2, shared, &f2);
	if (error)
		goto out;

	/*
	 * Under the futexes' queue locks, check the value; if
	 * unchanged from val3, wake the waiters.
	 */
	futex_queue_lock2(f, f2);
	if (op == FUTEX_CMP_REQUEUE && !futex_test(uaddr, val3)) {
		error = EAGAIN;
	} else {
		error = 0;
		nwoken = futex_wake(f, val, f2, val2, FUTEX_BITSET_MATCH_ANY);
	}
	futex_queue_unlock2(f, f2);

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
	error = futex_lookup(uaddr, shared, &f);
	if (error)
		goto out;

	/* Look up the second futex, if any.  */
	error = futex_lookup(uaddr2, shared, &f2);
	if (error)
		goto out;

	/*
	 * Under the queue locks:
	 *
	 * 1. Read/modify/write: *uaddr2 op= oparg.
	 * 2. Unconditionally wake uaddr.
	 * 3. Conditionally wake uaddr2, if it previously matched val2.
	 */
	futex_queue_lock2(f, f2);
	do {
		error = futex_load(uaddr2, &oldval);
		if (error)
			goto out_unlock;
		newval = futex_compute_op(oldval, val3);
		error = ucas_int(uaddr2, oldval, newval, &actual);
		if (error)
			goto out_unlock;
	} while (actual != oldval);
	nwoken = (f ? futex_wake(f, val, NULL, 0, FUTEX_BITSET_MATCH_ANY) : 0);
	if (f2 && futex_compute_cmp(oldval, val3))
		nwoken += futex_wake(f2, val2, NULL, 0,
		    FUTEX_BITSET_MATCH_ANY);

	/* Success! */
	error = 0;
out_unlock:
	futex_queue_unlock2(f, f2);

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

	op &= FUTEX_CMD_MASK;

	switch (op) {
	case FUTEX_WAIT:
		return futex_func_wait(shared, uaddr, val,
		    FUTEX_BITSET_MATCH_ANY, timeout, clkid, TIMER_RELTIME,
		    retval);

	case FUTEX_WAKE:
		val3 = FUTEX_BITSET_MATCH_ANY;
		/* FALLTHROUGH */
	case FUTEX_WAKE_BITSET:
		return futex_func_wake(shared, uaddr, val, val3, retval);

	case FUTEX_REQUEUE:
	case FUTEX_CMP_REQUEUE:
		return futex_func_requeue(shared, op, uaddr, val, uaddr2,
		    val2, val3, retval);

	case FUTEX_WAIT_BITSET:
		return futex_func_wait(shared, uaddr, val, val3, timeout,
		    clkid, TIMER_ABSTIME, retval);

	case FUTEX_WAKE_OP:
		return futex_func_wake_op(shared, uaddr, val, uaddr2, val2,
		    val3, retval);

	case FUTEX_FD:
	default:
		return ENOSYS;
	}
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
		(void) futex_func_wake(/*shared*/true, uaddr, 1,
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
	 * Look for a shared futex since we have no positive indication
	 * it is private.  If we can't, tough.
	 */
	error = futex_lookup(uaddr, /*shared*/true, &f);
	if (error)
		return;

	/*
	 * If there's no kernel state for this futex, there's nothing to
	 * release.
	 */
	if (f == NULL)
		return;

	/* Work under the futex queue lock.  */
	futex_queue_lock(f);

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
	if (oldval & FUTEX_WAITERS)
		(void)futex_wake(f, 1, NULL, 0, FUTEX_BITSET_MATCH_ANY);

	/* Unlock the queue and release the futex.  */
out:	futex_queue_unlock(f);
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
futex_release_all_lwp(struct lwp * const l)
{
	u_long rhead[_FUTEX_ROBUST_HEAD_NWORDS];
	int limit = 1000000;
	int error;

	/* If there's no robust list there's nothing to do. */
	if (l->l_robust_head == 0)
		return;

	KASSERT((l->l_lid & FUTEX_TID_MASK) == l->l_lid);

	/* Read the final snapshot of the robust list head. */
	error = futex_fetch_robust_head(l->l_robust_head, rhead);
	if (error) {
		printf("WARNING: pid %jd (%s) lwp %jd:"
		    " unmapped robust futex list head\n",
		    (uintmax_t)l->l_proc->p_pid, l->l_proc->p_comm,
		    (uintmax_t)l->l_lid);
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
			release_futex(next + offset, l->l_lid, is_pi, false);
		error = futex_fetch_robust_entry(next, &next, &is_pi);
		if (error)
			break;
		preempt_point();
	}
	if (limit <= 0) {
		printf("WARNING: pid %jd (%s) lwp %jd:"
		    " exhausted robust futex limit\n",
		    (uintmax_t)l->l_proc->p_pid, l->l_proc->p_comm,
		    (uintmax_t)l->l_lid);
	}

	/* If there's a pending futex, it may need to be released too. */
	if (pending != 0) {
		release_futex(pending + offset, l->l_lid, pending_is_pi, true);
	}
}
