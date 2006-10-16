/*	$NetBSD: subr_vmem.c,v 1.11 2006/10/16 16:05:34 dogcow Exp $	*/

/*-
 * Copyright (c)2006 YAMAMOTO Takashi,
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * reference:
 * -	Magazines and Vmem: Extending the Slab Allocator
 *	to Many CPUs and Arbitrary Resources
 *	http://www.usenix.org/event/usenix01/bonwick.html
 *
 * TODO:
 * -	implement vmem_xalloc/vmem_xfree
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_vmem.c,v 1.11 2006/10/16 16:05:34 dogcow Exp $");

#define	VMEM_DEBUG
#if defined(_KERNEL)
#define	QCACHE
#endif /* defined(_KERNEL) */

#include <sys/param.h>
#include <sys/hash.h>
#include <sys/queue.h>

#if defined(_KERNEL)
#include <sys/systm.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/once.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/vmem.h>
#else /* defined(_KERNEL) */
#include "../sys/vmem.h"
#endif /* defined(_KERNEL) */

#if defined(_KERNEL)
#define	SIMPLELOCK_DECL(name)	struct simplelock name
#else /* defined(_KERNEL) */
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#define	KASSERT(a)		assert(a)
#define	SIMPLELOCK_DECL(name)	/* nothing */
#define	LOCK_ASSERT(a)		/* nothing */
#define	simple_lock_init(a)	/* nothing */
#define	simple_lock(a)		/* nothing */
#define	simple_unlock(a)	/* nothing */
#define	ASSERT_SLEEPABLE(lk, msg) /* nothing */
#endif /* defined(_KERNEL) */

struct vmem;
struct vmem_btag;

#if defined(VMEM_DEBUG)
void vmem_dump(const vmem_t *);
#endif /* defined(VMEM_DEBUG) */

#define	VMEM_MAXORDER		(sizeof(vmem_size_t) * CHAR_BIT)
#define	VMEM_HASHSIZE_INIT	4096	/* XXX */

#define	VM_FITMASK	(VM_BESTFIT | VM_INSTANTFIT)

CIRCLEQ_HEAD(vmem_seglist, vmem_btag);
LIST_HEAD(vmem_freelist, vmem_btag);
LIST_HEAD(vmem_hashlist, vmem_btag);

#if defined(QCACHE)
#define	VMEM_QCACHE_IDX_MAX	32

#define	QC_NAME_MAX	16

struct qcache {
	struct pool qc_pool;
	struct pool_cache qc_cache;
	vmem_t *qc_vmem;
	char qc_name[QC_NAME_MAX];
};
typedef struct qcache qcache_t;
#define	QC_POOL_TO_QCACHE(pool)	((qcache_t *)(pool))
#endif /* defined(QCACHE) */

/* vmem arena */
struct vmem {
	SIMPLELOCK_DECL(vm_lock);
	vmem_addr_t (*vm_allocfn)(vmem_t *, vmem_size_t, vmem_size_t *,
	    vm_flag_t);
	void (*vm_freefn)(vmem_t *, vmem_addr_t, vmem_size_t);
	vmem_t *vm_source;
	struct vmem_seglist vm_seglist;
	struct vmem_freelist vm_freelist[VMEM_MAXORDER];
	size_t vm_hashsize;
	size_t vm_nbusytag;
	struct vmem_hashlist *vm_hashlist;
	size_t vm_quantum_mask;
	int vm_quantum_shift;
	const char *vm_name;

#if defined(QCACHE)
	/* quantum cache */
	size_t vm_qcache_max;
	struct pool_allocator vm_qcache_allocator;
	qcache_t vm_qcache[VMEM_QCACHE_IDX_MAX];
#endif /* defined(QCACHE) */
};

#define	VMEM_LOCK(vm)	simple_lock(&vm->vm_lock)
#define	VMEM_UNLOCK(vm)	simple_unlock(&vm->vm_lock)
#define	VMEM_LOCK_INIT(vm)	simple_lock_init(&vm->vm_lock);
#define	VMEM_ASSERT_LOCKED(vm) \
	LOCK_ASSERT(simple_lock_held(&vm->vm_lock))
#define	VMEM_ASSERT_UNLOCKED(vm) \
	LOCK_ASSERT(!simple_lock_held(&vm->vm_lock))

/* boundary tag */
struct vmem_btag {
	CIRCLEQ_ENTRY(vmem_btag) bt_seglist;
	union {
		LIST_ENTRY(vmem_btag) u_freelist; /* BT_TYPE_FREE */
		LIST_ENTRY(vmem_btag) u_hashlist; /* BT_TYPE_BUSY */
	} bt_u;
#define	bt_hashlist	bt_u.u_hashlist
#define	bt_freelist	bt_u.u_freelist
	vmem_addr_t bt_start;
	vmem_size_t bt_size;
	int bt_type;
};

#define	BT_TYPE_SPAN		1
#define	BT_TYPE_SPAN_STATIC	2
#define	BT_TYPE_FREE		3
#define	BT_TYPE_BUSY		4
#define	BT_ISSPAN_P(bt)	((bt)->bt_type <= BT_TYPE_SPAN_STATIC)

#define	BT_END(bt)	((bt)->bt_start + (bt)->bt_size)

typedef struct vmem_btag bt_t;

/* ---- misc */

#define	ORDER2SIZE(order)	((vmem_size_t)1 << (order))

static int
calc_order(vmem_size_t size)
{
	vmem_size_t target;
	int i;

	KASSERT(size != 0);

	i = 0;
	target = size >> 1;
	while (ORDER2SIZE(i) <= target) {
		i++;
	}

	KASSERT(ORDER2SIZE(i) <= size);
	KASSERT(size < ORDER2SIZE(i + 1) || ORDER2SIZE(i + 1) < ORDER2SIZE(i));

	return i;
}

#if defined(_KERNEL)
static MALLOC_DEFINE(M_VMEM, "vmem", "vmem");
#endif /* defined(_KERNEL) */

static void *
xmalloc(size_t sz, vm_flag_t flags)
{

#if defined(_KERNEL)
	return malloc(sz, M_VMEM,
	    M_CANFAIL | ((flags & VM_SLEEP) ? M_WAITOK : M_NOWAIT));
#else /* defined(_KERNEL) */
	return malloc(sz);
#endif /* defined(_KERNEL) */
}

static void
xfree(void *p)
{

#if defined(_KERNEL)
	return free(p, M_VMEM);
#else /* defined(_KERNEL) */
	return free(p);
#endif /* defined(_KERNEL) */
}

/* ---- boundary tag */

#if defined(_KERNEL)
static struct pool_cache bt_poolcache;
static POOL_INIT(bt_pool, sizeof(bt_t), 0, 0, 0, "vmembtpl", NULL);
#endif /* defined(_KERNEL) */

static bt_t *
bt_alloc(vmem_t *vm __unused, vm_flag_t flags)
{
	bt_t *bt;

#if defined(_KERNEL)
	/* XXX bootstrap */
	bt = pool_cache_get(&bt_poolcache,
	    (flags & VM_SLEEP) != 0 ? PR_WAITOK : PR_NOWAIT);
#else /* defined(_KERNEL) */
	bt = malloc(sizeof *bt);
#endif /* defined(_KERNEL) */

	return bt;
}

static void
bt_free(vmem_t *vm __unused, bt_t *bt)
{

#if defined(_KERNEL)
	/* XXX bootstrap */
	pool_cache_put(&bt_poolcache, bt);
#else /* defined(_KERNEL) */
	free(bt);
#endif /* defined(_KERNEL) */
}

/*
 * freelist[0] ... [1, 1] 
 * freelist[1] ... [2, 3]
 * freelist[2] ... [4, 7]
 * freelist[3] ... [8, 15]
 *  :
 * freelist[n] ... [(1 << n), (1 << (n + 1)) - 1]
 *  :
 */

static struct vmem_freelist *
bt_freehead_tofree(vmem_t *vm, vmem_size_t size)
{
	const vmem_size_t qsize = size >> vm->vm_quantum_shift;
	int idx;

	KASSERT((size & vm->vm_quantum_mask) == 0);
	KASSERT(size != 0);

	idx = calc_order(qsize);
	KASSERT(idx >= 0);
	KASSERT(idx < VMEM_MAXORDER);

	return &vm->vm_freelist[idx];
}

static struct vmem_freelist *
bt_freehead_toalloc(vmem_t *vm, vmem_size_t size, vm_flag_t strat)
{
	const vmem_size_t qsize = size >> vm->vm_quantum_shift;
	int idx;

	KASSERT((size & vm->vm_quantum_mask) == 0);
	KASSERT(size != 0);

	idx = calc_order(qsize);
	if (strat == VM_INSTANTFIT && ORDER2SIZE(idx) != qsize) {
		idx++;
		/* check too large request? */
	}
	KASSERT(idx >= 0);
	KASSERT(idx < VMEM_MAXORDER);

	return &vm->vm_freelist[idx];
}

/* ---- boundary tag hash */

static struct vmem_hashlist *
bt_hashhead(vmem_t *vm, vmem_addr_t addr)
{
	struct vmem_hashlist *list;
	unsigned int hash;

	hash = hash32_buf(&addr, sizeof(addr), HASH32_BUF_INIT);
	list = &vm->vm_hashlist[hash % vm->vm_hashsize];

	return list;
}

static bt_t *
bt_lookupbusy(vmem_t *vm, vmem_addr_t addr)
{
	struct vmem_hashlist *list;
	bt_t *bt;

	list = bt_hashhead(vm, addr); 
	LIST_FOREACH(bt, list, bt_hashlist) {
		if (bt->bt_start == addr) {
			break;
		}
	}

	return bt;
}

static void
bt_rembusy(vmem_t *vm, bt_t *bt)
{

	KASSERT(vm->vm_nbusytag > 0);
	vm->vm_nbusytag--;
	LIST_REMOVE(bt, bt_hashlist);
}

static void
bt_insbusy(vmem_t *vm, bt_t *bt)
{
	struct vmem_hashlist *list;

	KASSERT(bt->bt_type == BT_TYPE_BUSY);

	list = bt_hashhead(vm, bt->bt_start);
	LIST_INSERT_HEAD(list, bt, bt_hashlist);
	vm->vm_nbusytag++;
}

/* ---- boundary tag list */

static void
bt_remseg(vmem_t *vm, bt_t *bt)
{

	CIRCLEQ_REMOVE(&vm->vm_seglist, bt, bt_seglist);
}

static void
bt_insseg(vmem_t *vm, bt_t *bt, bt_t *prev)
{

	CIRCLEQ_INSERT_AFTER(&vm->vm_seglist, prev, bt, bt_seglist);
}

static void
bt_insseg_tail(vmem_t *vm, bt_t *bt)
{

	CIRCLEQ_INSERT_TAIL(&vm->vm_seglist, bt, bt_seglist);
}

static void
bt_remfree(vmem_t *vm __unused, bt_t *bt)
{

	KASSERT(bt->bt_type == BT_TYPE_FREE);

	LIST_REMOVE(bt, bt_freelist);
}

static void
bt_insfree(vmem_t *vm, bt_t *bt)
{
	struct vmem_freelist *list;

	list = bt_freehead_tofree(vm, bt->bt_size);
	LIST_INSERT_HEAD(list, bt, bt_freelist);
}

/* ---- vmem internal functions */

#if defined(QCACHE)
static inline vm_flag_t
prf_to_vmf(int prflags)
{
	vm_flag_t vmflags;

	KASSERT((prflags & ~(PR_LIMITFAIL | PR_WAITOK | PR_NOWAIT)) == 0);
	if ((prflags & PR_WAITOK) != 0) {
		vmflags = VM_SLEEP;
	} else {
		vmflags = VM_NOSLEEP;
	}
	return vmflags;
}

static inline int
vmf_to_prf(vm_flag_t vmflags)
{
	int prflags;

	if ((vmflags & VM_SLEEP) != 0) {
		prflags = PR_WAITOK;
	} else {
		prflags = PR_NOWAIT;
	}
	return prflags;
}

static size_t
qc_poolpage_size(size_t qcache_max)
{
	int i;

	for (i = 0; ORDER2SIZE(i) <= qcache_max * 3; i++) {
		/* nothing */
	}
	return ORDER2SIZE(i);
}

static void *
qc_poolpage_alloc(struct pool *pool, int prflags)
{
	qcache_t *qc = QC_POOL_TO_QCACHE(pool);
	vmem_t *vm = qc->qc_vmem;

	return (void *)vmem_alloc(vm, pool->pr_alloc->pa_pagesz,
	    prf_to_vmf(prflags) | VM_INSTANTFIT);
}

static void
qc_poolpage_free(struct pool *pool, void *addr)
{
	qcache_t *qc = QC_POOL_TO_QCACHE(pool);
	vmem_t *vm = qc->qc_vmem;

	vmem_free(vm, (vmem_addr_t)addr, pool->pr_alloc->pa_pagesz);
}

static void
qc_init(vmem_t *vm, size_t qcache_max)
{
	struct pool_allocator *pa;
	int qcache_idx_max;
	int i;

	KASSERT((qcache_max & vm->vm_quantum_mask) == 0);
	if (qcache_max > (VMEM_QCACHE_IDX_MAX << vm->vm_quantum_shift)) {
		qcache_max = VMEM_QCACHE_IDX_MAX << vm->vm_quantum_shift;
	}
	vm->vm_qcache_max = qcache_max;
	pa = &vm->vm_qcache_allocator;
	memset(pa, 0, sizeof(*pa));
	pa->pa_alloc = qc_poolpage_alloc;
	pa->pa_free = qc_poolpage_free;
	pa->pa_pagesz = qc_poolpage_size(qcache_max);

	qcache_idx_max = qcache_max >> vm->vm_quantum_shift;
	for (i = 1; i <= qcache_idx_max; i++) {
		qcache_t *qc = &vm->vm_qcache[i - 1];
		size_t size = i << vm->vm_quantum_shift;

		qc->qc_vmem = vm;
		snprintf(qc->qc_name, sizeof(qc->qc_name), "%s-%zu",
		    vm->vm_name, size);
		pool_init(&qc->qc_pool, size, 0, 0,
		    PR_NOALIGN | PR_NOTOUCH /* XXX */, qc->qc_name, pa);
		pool_cache_init(&qc->qc_cache, &qc->qc_pool, NULL, NULL, NULL);
	}
}

static boolean_t
qc_reap(vmem_t *vm)
{
	int i;
	int qcache_idx_max;
	boolean_t didsomething = FALSE;

	qcache_idx_max = vm->vm_qcache_max >> vm->vm_quantum_shift;
	for (i = 1; i <= qcache_idx_max; i++) {
		qcache_t *qc = &vm->vm_qcache[i - 1];

		if (pool_reclaim(&qc->qc_pool) != 0) {
			didsomething = TRUE;
		}
	}

	return didsomething;
}
#endif /* defined(QCACHE) */

#if defined(_KERNEL)
static int
vmem_init(void)
{

	pool_cache_init(&bt_poolcache, &bt_pool, NULL, NULL, NULL);
	return 0;
}
#endif /* defined(_KERNEL) */

static vmem_addr_t
vmem_add1(vmem_t *vm, vmem_addr_t addr, vmem_size_t size, vm_flag_t flags,
    int spanbttype)
{
	bt_t *btspan;
	bt_t *btfree;

	KASSERT((flags & (VM_SLEEP|VM_NOSLEEP)) != 0);
	KASSERT((~flags & (VM_SLEEP|VM_NOSLEEP)) != 0);
	VMEM_ASSERT_UNLOCKED(vm);

	btspan = bt_alloc(vm, flags);
	if (btspan == NULL) {
		return VMEM_ADDR_NULL;
	}
	btfree = bt_alloc(vm, flags);
	if (btfree == NULL) {
		bt_free(vm, btspan);
		return VMEM_ADDR_NULL;
	}

	btspan->bt_type = spanbttype;
	btspan->bt_start = addr;
	btspan->bt_size = size;

	btfree->bt_type = BT_TYPE_FREE;
	btfree->bt_start = addr;
	btfree->bt_size = size;

	VMEM_LOCK(vm);
	bt_insseg_tail(vm, btspan);
	bt_insseg(vm, btfree, btspan);
	bt_insfree(vm, btfree);
	VMEM_UNLOCK(vm);

	return addr;
}

static int
vmem_import(vmem_t *vm, vmem_size_t size, vm_flag_t flags)
{
	vmem_addr_t addr;

	VMEM_ASSERT_UNLOCKED(vm);

	if (vm->vm_allocfn == NULL) {
		return EINVAL;
	}

	addr = (*vm->vm_allocfn)(vm->vm_source, size, &size, flags);
	if (addr == VMEM_ADDR_NULL) {
		return ENOMEM;
	}

	if (vmem_add1(vm, addr, size, flags, BT_TYPE_SPAN) == VMEM_ADDR_NULL) {
		(*vm->vm_freefn)(vm->vm_source, addr, size);
		return ENOMEM;
	}

	return 0;
}

static int
vmem_rehash(vmem_t *vm, size_t newhashsize, vm_flag_t flags)
{
	bt_t *bt;
	int i;
	struct vmem_hashlist *newhashlist;
	struct vmem_hashlist *oldhashlist;
	size_t oldhashsize;

	KASSERT(newhashsize > 0);
	VMEM_ASSERT_UNLOCKED(vm);

	newhashlist =
	    xmalloc(sizeof(struct vmem_hashlist *) * newhashsize, flags);
	if (newhashlist == NULL) {
		return ENOMEM;
	}
	for (i = 0; i < newhashsize; i++) {
		LIST_INIT(&newhashlist[i]);
	}

	VMEM_LOCK(vm);
	oldhashlist = vm->vm_hashlist;
	oldhashsize = vm->vm_hashsize;
	vm->vm_hashlist = newhashlist;
	vm->vm_hashsize = newhashsize;
	if (oldhashlist == NULL) {
		VMEM_UNLOCK(vm);
		return 0;
	}
	for (i = 0; i < oldhashsize; i++) {
		while ((bt = LIST_FIRST(&oldhashlist[i])) != NULL) {
			bt_rembusy(vm, bt); /* XXX */
			bt_insbusy(vm, bt);
		}
	}
	VMEM_UNLOCK(vm);

	xfree(oldhashlist);

	return 0;
}

/*
 * vmem_fit: check if a bt can satisfy the given restrictions.
 */

static vmem_addr_t
vmem_fit(const bt_t *bt, vmem_size_t size, vmem_size_t align, vmem_size_t phase,
    vmem_size_t nocross, vmem_addr_t minaddr, vmem_addr_t maxaddr)
{
	vmem_addr_t start;
	vmem_addr_t end;

	KASSERT(bt->bt_size >= size);

	/*
	 * XXX assumption: vmem_addr_t and vmem_size_t are
	 * unsigned integer of the same size.
	 */

	start = bt->bt_start;
	if (start < minaddr) {
		start = minaddr;
	}
	end = BT_END(bt);
	if (end > maxaddr - 1) {
		end = maxaddr - 1;
	}
	if (start >= end) {
		return VMEM_ADDR_NULL;
	}
	start = -(-(start - phase) & -align) + phase;
	if (start < bt->bt_start) {
		start += align;
	}
	if (((start ^ (start + size - 1)) & -nocross) != 0) {
		KASSERT(align < nocross);
		start = -(-(start - phase) & -nocross) + phase;
	}
	if (start < end && end - start >= size) {
		KASSERT((start & (align - 1)) == phase);
		KASSERT(((start ^ (start + size - 1)) & -nocross) == 0);
		KASSERT(minaddr <= start);
		KASSERT(maxaddr == 0 || start + size <= maxaddr);
		KASSERT(bt->bt_start <= start);
		KASSERT(start + size <= BT_END(bt));
		return start;
	}
	return VMEM_ADDR_NULL;
}

/* ---- vmem API */

/*
 * vmem_create: create an arena.
 *
 * => must not be called from interrupt context.
 */

vmem_t *
vmem_create(const char *name, vmem_addr_t base, vmem_size_t size,
    vmem_size_t quantum,
    vmem_addr_t (*allocfn)(vmem_t *, vmem_size_t, vmem_size_t *, vm_flag_t),
    void (*freefn)(vmem_t *, vmem_addr_t, vmem_size_t),
    vmem_t *source, vmem_size_t qcache_max, vm_flag_t flags)
{
	vmem_t *vm;
	int i;
#if defined(_KERNEL)
	static ONCE_DECL(control);
#endif /* defined(_KERNEL) */

	KASSERT((flags & (VM_SLEEP|VM_NOSLEEP)) != 0);
	KASSERT((~flags & (VM_SLEEP|VM_NOSLEEP)) != 0);

#if defined(_KERNEL)
	if (RUN_ONCE(&control, vmem_init)) {
		return NULL;
	}
#endif /* defined(_KERNEL) */
	vm = xmalloc(sizeof(*vm), flags);
	if (vm == NULL) {
		return NULL;
	}

	VMEM_LOCK_INIT(vm);
	vm->vm_name = name;
	vm->vm_quantum_mask = quantum - 1;
	vm->vm_quantum_shift = calc_order(quantum);
	KASSERT(ORDER2SIZE(vm->vm_quantum_shift) == quantum);
	vm->vm_allocfn = allocfn;
	vm->vm_freefn = freefn;
	vm->vm_source = source;
	vm->vm_nbusytag = 0;
#if defined(QCACHE)
	qc_init(vm, qcache_max);
#endif /* defined(QCACHE) */

	CIRCLEQ_INIT(&vm->vm_seglist);
	for (i = 0; i < VMEM_MAXORDER; i++) {
		LIST_INIT(&vm->vm_freelist[i]);
	}
	vm->vm_hashlist = NULL;
	if (vmem_rehash(vm, VMEM_HASHSIZE_INIT, flags)) {
		vmem_destroy(vm);
		return NULL;
	}

	if (size != 0) {
		if (vmem_add(vm, base, size, flags) == 0) {
			vmem_destroy(vm);
			return NULL;
		}
	}

	return vm;
}

void
vmem_destroy(vmem_t *vm)
{

	VMEM_ASSERT_UNLOCKED(vm);

	if (vm->vm_hashlist != NULL) {
		int i;

		for (i = 0; i < vm->vm_hashsize; i++) {
			bt_t *bt;

			while ((bt = LIST_FIRST(&vm->vm_hashlist[i])) != NULL) {
				KASSERT(bt->bt_type == BT_TYPE_SPAN_STATIC);
				bt_free(vm, bt);
			}
		}
		xfree(vm->vm_hashlist);
	}
	xfree(vm);
}

vmem_size_t
vmem_roundup_size(vmem_t *vm, vmem_size_t size)
{

	return (size + vm->vm_quantum_mask) & ~vm->vm_quantum_mask;
}

/*
 * vmem_alloc:
 *
 * => caller must ensure appropriate spl,
 *    if the arena can be accessed from interrupt context.
 */

vmem_addr_t
vmem_alloc(vmem_t *vm, vmem_size_t size0, vm_flag_t flags)
{
	const vmem_size_t size = vmem_roundup_size(vm, size0);
	const vm_flag_t strat = flags & VM_FITMASK;

	do { if (&strat) {} } while (/* CONSTCOND */ 0);
	KASSERT((flags & (VM_SLEEP|VM_NOSLEEP)) != 0);
	KASSERT((~flags & (VM_SLEEP|VM_NOSLEEP)) != 0);
	VMEM_ASSERT_UNLOCKED(vm);

	KASSERT(size0 > 0);
	KASSERT(size > 0);
	KASSERT(strat == VM_BESTFIT || strat == VM_INSTANTFIT);
	if ((flags & VM_SLEEP) != 0) {
		ASSERT_SLEEPABLE(NULL, "vmem_alloc");
	}

#if defined(QCACHE)
	if (size <= vm->vm_qcache_max) {
		int qidx = size >> vm->vm_quantum_shift;
		qcache_t *qc = &vm->vm_qcache[qidx - 1];

		return (vmem_addr_t)pool_cache_get(&qc->qc_cache,
		    vmf_to_prf(flags));
	}
#endif /* defined(QCACHE) */

	return vmem_xalloc(vm, size0, 0, 0, 0, 0, 0, flags);
}

vmem_addr_t
vmem_xalloc(vmem_t *vm, vmem_size_t size0, vmem_size_t align, vmem_size_t phase,
    vmem_size_t nocross, vmem_addr_t minaddr, vmem_addr_t maxaddr,
    vm_flag_t flags)
{
	struct vmem_freelist *list;
	struct vmem_freelist *first;
	struct vmem_freelist *end;
	bt_t *bt;
	bt_t *btnew;
	bt_t *btnew2;
	const vmem_size_t size = vmem_roundup_size(vm, size0);
	vm_flag_t strat = flags & VM_FITMASK;
	vmem_addr_t start;

	KASSERT(size0 > 0);
	KASSERT(size > 0);
	KASSERT(strat == VM_BESTFIT || strat == VM_INSTANTFIT);
	if ((flags & VM_SLEEP) != 0) {
		ASSERT_SLEEPABLE(NULL, "vmem_alloc");
	}
	KASSERT((align & vm->vm_quantum_mask) == 0);
	KASSERT((align & (align - 1)) == 0);
	KASSERT((phase & vm->vm_quantum_mask) == 0);
	KASSERT((nocross & vm->vm_quantum_mask) == 0);
	KASSERT((nocross & (nocross - 1)) == 0);
	KASSERT((align == 0 && phase == 0) || phase < align);
	KASSERT(nocross == 0 || nocross >= size);
	KASSERT(maxaddr == 0 || minaddr < maxaddr);
	KASSERT(((phase ^ (phase + size - 1)) & -nocross) == 0);

	if (align == 0) {
		align = vm->vm_quantum_mask + 1;
	}
	btnew = bt_alloc(vm, flags);
	if (btnew == NULL) {
		return VMEM_ADDR_NULL;
	}
	btnew2 = bt_alloc(vm, flags); /* XXX not necessary if no restrictions */
	if (btnew2 == NULL) {
		bt_free(vm, btnew);
		return VMEM_ADDR_NULL;
	}

retry_strat:
	first = bt_freehead_toalloc(vm, size, strat);
	end = &vm->vm_freelist[VMEM_MAXORDER];
retry:
	bt = NULL;
	VMEM_LOCK(vm);
	if (strat == VM_INSTANTFIT) {
		for (list = first; list < end; list++) {
			bt = LIST_FIRST(list);
			if (bt != NULL) {
				start = vmem_fit(bt, size, align, phase,
				    nocross, minaddr, maxaddr);
				if (start != VMEM_ADDR_NULL) {
					goto gotit;
				}
			}
		}
	} else { /* VM_BESTFIT */
		for (list = first; list < end; list++) {
			LIST_FOREACH(bt, list, bt_freelist) {
				if (bt->bt_size >= size) {
					start = vmem_fit(bt, size, align, phase,
					    nocross, minaddr, maxaddr);
					if (start != VMEM_ADDR_NULL) {
						goto gotit;
					}
				}
			}
		}
	}
	VMEM_UNLOCK(vm);
#if 1
	if (strat == VM_INSTANTFIT) {
		strat = VM_BESTFIT;
		goto retry_strat;
	}
#endif
	if (align != vm->vm_quantum_mask + 1 || phase != 0 ||
	    nocross != 0 || minaddr != 0 || maxaddr != 0) {

		/*
		 * XXX should try to import a region large enough to
		 * satisfy restrictions?
		 */

		return VMEM_ADDR_NULL;
	}
	if (vmem_import(vm, size, flags) == 0) {
		goto retry;
	}
	/* XXX */
	return VMEM_ADDR_NULL;

gotit:
	KASSERT(bt->bt_type == BT_TYPE_FREE);
	KASSERT(bt->bt_size >= size);
	bt_remfree(vm, bt);
	if (bt->bt_start != start) {
		btnew2->bt_type = BT_TYPE_FREE;
		btnew2->bt_start = bt->bt_start;
		btnew2->bt_size = start - bt->bt_start;
		bt->bt_start = start;
		bt->bt_size -= btnew2->bt_size;
		bt_insfree(vm, btnew2);
		bt_insseg(vm, btnew2, CIRCLEQ_PREV(bt, bt_seglist));
		btnew2 = NULL;
	}
	KASSERT(bt->bt_start == start);
	if (bt->bt_size != size && bt->bt_size - size > vm->vm_quantum_mask) {
		/* split */
		btnew->bt_type = BT_TYPE_BUSY;
		btnew->bt_start = bt->bt_start;
		btnew->bt_size = size;
		bt->bt_start = bt->bt_start + size;
		bt->bt_size -= size;
		bt_insfree(vm, bt);
		bt_insseg(vm, btnew, CIRCLEQ_PREV(bt, bt_seglist));
		bt_insbusy(vm, btnew);
		VMEM_UNLOCK(vm);
	} else {
		bt->bt_type = BT_TYPE_BUSY;
		bt_insbusy(vm, bt);
		VMEM_UNLOCK(vm);
		bt_free(vm, btnew);
		btnew = bt;
	}
	if (btnew2 != NULL) {
		bt_free(vm, btnew2);
	}
	KASSERT(btnew->bt_size >= size);
	btnew->bt_type = BT_TYPE_BUSY;

	return btnew->bt_start;
}

/*
 * vmem_free:
 *
 * => caller must ensure appropriate spl,
 *    if the arena can be accessed from interrupt context.
 */

void
vmem_free(vmem_t *vm, vmem_addr_t addr, vmem_size_t size)
{

	VMEM_ASSERT_UNLOCKED(vm);
	KASSERT(addr != VMEM_ADDR_NULL);
	KASSERT(size > 0);

#if defined(QCACHE)
	if (size <= vm->vm_qcache_max) {
		int qidx = (size + vm->vm_quantum_mask) >> vm->vm_quantum_shift;
		qcache_t *qc = &vm->vm_qcache[qidx - 1];

		return pool_cache_put(&qc->qc_cache, (void *)addr);
	}
#endif /* defined(QCACHE) */

	vmem_xfree(vm, addr, size);
}

void
vmem_xfree(vmem_t *vm, vmem_addr_t addr, vmem_size_t size __unused)
{
	bt_t *bt;
	bt_t *t;

	VMEM_ASSERT_UNLOCKED(vm);
	KASSERT(addr != VMEM_ADDR_NULL);
	KASSERT(size > 0);

	VMEM_LOCK(vm);

	bt = bt_lookupbusy(vm, addr);
	KASSERT(bt != NULL);
	KASSERT(bt->bt_start == addr);
	KASSERT(bt->bt_size == vmem_roundup_size(vm, size) ||
	    bt->bt_size - vmem_roundup_size(vm, size) <= vm->vm_quantum_mask);
	KASSERT(bt->bt_type == BT_TYPE_BUSY);
	bt_rembusy(vm, bt);
	bt->bt_type = BT_TYPE_FREE;

	/* coalesce */
	t = CIRCLEQ_NEXT(bt, bt_seglist);
	if (t != NULL && t->bt_type == BT_TYPE_FREE) {
		KASSERT(BT_END(bt) == t->bt_start);
		bt_remfree(vm, t);
		bt_remseg(vm, t);
		bt->bt_size += t->bt_size;
		bt_free(vm, t);
	}
	t = CIRCLEQ_PREV(bt, bt_seglist);
	if (t != NULL && t->bt_type == BT_TYPE_FREE) {
		KASSERT(BT_END(t) == bt->bt_start);
		bt_remfree(vm, t);
		bt_remseg(vm, t);
		bt->bt_size += t->bt_size;
		bt->bt_start = t->bt_start;
		bt_free(vm, t);
	}

	t = CIRCLEQ_PREV(bt, bt_seglist);
	KASSERT(t != NULL);
	KASSERT(BT_ISSPAN_P(t) || t->bt_type == BT_TYPE_BUSY);
	if (vm->vm_freefn != NULL && t->bt_type == BT_TYPE_SPAN &&
	    t->bt_size == bt->bt_size) {
		vmem_addr_t spanaddr;
		vmem_size_t spansize;

		KASSERT(t->bt_start == bt->bt_start);
		spanaddr = bt->bt_start;
		spansize = bt->bt_size;
		bt_remseg(vm, bt);
		bt_free(vm, bt);
		bt_remseg(vm, t);
		bt_free(vm, t);
		VMEM_UNLOCK(vm);
		(*vm->vm_freefn)(vm->vm_source, spanaddr, spansize);
	} else {
		bt_insfree(vm, bt);
		VMEM_UNLOCK(vm);
	}
}

/*
 * vmem_add:
 *
 * => caller must ensure appropriate spl,
 *    if the arena can be accessed from interrupt context.
 */

vmem_addr_t
vmem_add(vmem_t *vm, vmem_addr_t addr, vmem_size_t size, vm_flag_t flags)
{

	return vmem_add1(vm, addr, size, flags, BT_TYPE_SPAN_STATIC);
}

/*
 * vmem_reap: reap unused resources.
 *
 * => return TRUE if we successfully reaped something.
 */

boolean_t
vmem_reap(vmem_t *vm)
{
	boolean_t didsomething = FALSE;

	VMEM_ASSERT_UNLOCKED(vm);

#if defined(QCACHE)
	didsomething = qc_reap(vm);
#endif /* defined(QCACHE) */
	return didsomething;
}

/* ---- debug */

#if defined(VMEM_DEBUG)

#if !defined(_KERNEL)
#include <stdio.h>
#endif /* !defined(_KERNEL) */

void bt_dump(const bt_t *);

void
bt_dump(const bt_t *bt)
{

	printf("\t%p: %" PRIu64 ", %" PRIu64 ", %d\n",
	    bt, (uint64_t)bt->bt_start, (uint64_t)bt->bt_size,
	    bt->bt_type);
}

void
vmem_dump(const vmem_t *vm)
{
	const bt_t *bt;
	int i;

	printf("vmem %p '%s'\n", vm, vm->vm_name);
	CIRCLEQ_FOREACH(bt, &vm->vm_seglist, bt_seglist) {
		bt_dump(bt);
	}

	for (i = 0; i < VMEM_MAXORDER; i++) {
		const struct vmem_freelist *fl = &vm->vm_freelist[i];

		if (LIST_EMPTY(fl)) {
			continue;
		}

		printf("freelist[%d]\n", i);
		LIST_FOREACH(bt, fl, bt_freelist) {
			bt_dump(bt);
			if (bt->bt_size) {
			}
		}
	}
}

#if !defined(_KERNEL)

#include <stdlib.h>

int
main()
{
	vmem_t *vm;
	vmem_addr_t p;
	struct reg {
		vmem_addr_t p;
		vmem_size_t sz;
		boolean_t x;
	} *reg = NULL;
	int nreg = 0;
	int nalloc = 0;
	int nfree = 0;
	vmem_size_t total = 0;
#if 1
	vm_flag_t strat = VM_INSTANTFIT;
#else
	vm_flag_t strat = VM_BESTFIT;
#endif

	vm = vmem_create("test", VMEM_ADDR_NULL, 0, 1,
	    NULL, NULL, NULL, 0, VM_NOSLEEP);
	if (vm == NULL) {
		printf("vmem_create\n");
		exit(EXIT_FAILURE);
	}
	vmem_dump(vm);

	p = vmem_add(vm, 100, 200, VM_SLEEP);
	p = vmem_add(vm, 2000, 1, VM_SLEEP);
	p = vmem_add(vm, 40000, 0x10000000>>12, VM_SLEEP);
	p = vmem_add(vm, 10000, 10000, VM_SLEEP);
	p = vmem_add(vm, 500, 1000, VM_SLEEP);
	vmem_dump(vm);
	for (;;) {
		struct reg *r;
		int t = rand() % 100;

		if (t > 45) {
			/* alloc */
			vmem_size_t sz = rand() % 500 + 1;
			boolean_t x;
			vmem_size_t align, phase, nocross;
			vmem_addr_t minaddr, maxaddr;

			if (t > 70) {
				x = TRUE;
				/* XXX */
				align = 1 << (rand() % 15);
				phase = rand() % 65536;
				nocross = 1 << (rand() % 15);
				if (align <= phase) {
					phase = 0;
				}
				if (((phase ^ (phase + sz)) & -nocross) != 0) {
					nocross = 0;
				}
				minaddr = rand() % 50000;
				maxaddr = rand() % 70000;
				if (minaddr > maxaddr) {
					minaddr = 0;
					maxaddr = 0;
				}
				printf("=== xalloc %" PRIu64
				    " align=%" PRIu64 ", phase=%" PRIu64
				    ", nocross=%" PRIu64 ", min=%" PRIu64
				    ", max=%" PRIu64 "\n",
				    (uint64_t)sz,
				    (uint64_t)align,
				    (uint64_t)phase,
				    (uint64_t)nocross,
				    (uint64_t)minaddr,
				    (uint64_t)maxaddr);
				p = vmem_xalloc(vm, sz, align, phase, nocross,
				    minaddr, maxaddr, strat|VM_SLEEP);
			} else {
				x = FALSE;
				printf("=== alloc %" PRIu64 "\n", (uint64_t)sz);
				p = vmem_alloc(vm, sz, strat|VM_SLEEP);
			}
			printf("-> %" PRIu64 "\n", (uint64_t)p);
			vmem_dump(vm);
			if (p == VMEM_ADDR_NULL) {
				if (x) {
					continue;
				}
				break;
			}
			nreg++;
			reg = realloc(reg, sizeof(*reg) * nreg);
			r = &reg[nreg - 1];
			r->p = p;
			r->sz = sz;
			r->x = x;
			total += sz;
			nalloc++;
		} else if (nreg != 0) {
			/* free */
			r = &reg[rand() % nreg];
			printf("=== free %" PRIu64 ", %" PRIu64 "\n",
			    (uint64_t)r->p, (uint64_t)r->sz);
			if (r->x) {
				vmem_xfree(vm, r->p, r->sz);
			} else {
				vmem_free(vm, r->p, r->sz);
			}
			total -= r->sz;
			vmem_dump(vm);
			*r = reg[nreg - 1];
			nreg--;
			nfree++;
		}
		printf("total=%" PRIu64 "\n", (uint64_t)total);
	}
	fprintf(stderr, "total=%" PRIu64 ", nalloc=%d, nfree=%d\n",
	    (uint64_t)total, nalloc, nfree);
	exit(EXIT_SUCCESS);
}
#endif /* !defined(_KERNEL) */
#endif /* defined(VMEM_DEBUG) */
