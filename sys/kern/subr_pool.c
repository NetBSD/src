/*	$NetBSD: subr_pool.c,v 1.284 2022/05/29 10:47:40 andvar Exp $	*/

/*
 * Copyright (c) 1997, 1999, 2000, 2002, 2007, 2008, 2010, 2014, 2015, 2018,
 *     2020, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg; by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center; by Andrew Doran, and by
 * Maxime Villard.
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
__KERNEL_RCSID(0, "$NetBSD: subr_pool.c,v 1.284 2022/05/29 10:47:40 andvar Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_lockdebug.h"
#include "opt_pool.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/bitops.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/vmem.h>
#include <sys/pool.h>
#include <sys/syslog.h>
#include <sys/debug.h>
#include <sys/lock.h>
#include <sys/lockdebug.h>
#include <sys/xcall.h>
#include <sys/cpu.h>
#include <sys/atomic.h>
#include <sys/asan.h>
#include <sys/msan.h>
#include <sys/fault.h>

#include <uvm/uvm_extern.h>

/*
 * Pool resource management utility.
 *
 * Memory is allocated in pages which are split into pieces according to
 * the pool item size. Each page is kept on one of three lists in the
 * pool structure: `pr_emptypages', `pr_fullpages' and `pr_partpages',
 * for empty, full and partially-full pages respectively. The individual
 * pool items are on a linked list headed by `ph_itemlist' in each page
 * header. The memory for building the page list is either taken from
 * the allocated pages themselves (for small pool items) or taken from
 * an internal pool of page headers (`phpool').
 */

/* List of all pools. Non static as needed by 'vmstat -m' */
TAILQ_HEAD(, pool) pool_head = TAILQ_HEAD_INITIALIZER(pool_head);

/* Private pool for page header structures */
#define	PHPOOL_MAX	8
static struct pool phpool[PHPOOL_MAX];
#define	PHPOOL_FREELIST_NELEM(idx) \
	(((idx) == 0) ? BITMAP_MIN_SIZE : BITMAP_SIZE * (1 << (idx)))

#if !defined(KMSAN) && (defined(DIAGNOSTIC) || defined(KASAN))
#define POOL_REDZONE
#endif

#if defined(POOL_QUARANTINE)
#define POOL_NOCACHE
#endif

#ifdef POOL_REDZONE
# ifdef KASAN
#  define POOL_REDZONE_SIZE 8
# else
#  define POOL_REDZONE_SIZE 2
# endif
static void pool_redzone_init(struct pool *, size_t);
static void pool_redzone_fill(struct pool *, void *);
static void pool_redzone_check(struct pool *, void *);
static void pool_cache_redzone_check(pool_cache_t, void *);
#else
# define pool_redzone_init(pp, sz)		__nothing
# define pool_redzone_fill(pp, ptr)		__nothing
# define pool_redzone_check(pp, ptr)		__nothing
# define pool_cache_redzone_check(pc, ptr)	__nothing
#endif

#ifdef KMSAN
static inline void pool_get_kmsan(struct pool *, void *);
static inline void pool_put_kmsan(struct pool *, void *);
static inline void pool_cache_get_kmsan(pool_cache_t, void *);
static inline void pool_cache_put_kmsan(pool_cache_t, void *);
#else
#define pool_get_kmsan(pp, ptr)		__nothing
#define pool_put_kmsan(pp, ptr)		__nothing
#define pool_cache_get_kmsan(pc, ptr)	__nothing
#define pool_cache_put_kmsan(pc, ptr)	__nothing
#endif

#ifdef POOL_QUARANTINE
static void pool_quarantine_init(struct pool *);
static void pool_quarantine_flush(struct pool *);
static bool pool_put_quarantine(struct pool *, void *,
    struct pool_pagelist *);
#else
#define pool_quarantine_init(a)			__nothing
#define pool_quarantine_flush(a)		__nothing
#define pool_put_quarantine(a, b, c)		false
#endif

#ifdef POOL_NOCACHE
static bool pool_cache_put_nocache(pool_cache_t, void *);
#else
#define pool_cache_put_nocache(a, b)		false
#endif

#define NO_CTOR	__FPTRCAST(int (*)(void *, void *, int), nullop)
#define NO_DTOR	__FPTRCAST(void (*)(void *, void *), nullop)

#define pc_has_pser(pc) (((pc)->pc_roflags & PR_PSERIALIZE) != 0)
#define pc_has_ctor(pc) ((pc)->pc_ctor != NO_CTOR)
#define pc_has_dtor(pc) ((pc)->pc_dtor != NO_DTOR)

#define pp_has_pser(pp) (((pp)->pr_roflags & PR_PSERIALIZE) != 0)

#define pool_barrier()	xc_barrier(0)

/*
 * Pool backend allocators.
 *
 * Each pool has a backend allocator that handles allocation, deallocation,
 * and any additional draining that might be needed.
 *
 * We provide two standard allocators:
 *
 *	pool_allocator_kmem - the default when no allocator is specified
 *
 *	pool_allocator_nointr - used for pools that will not be accessed
 *	in interrupt context.
 */
void *pool_page_alloc(struct pool *, int);
void pool_page_free(struct pool *, void *);

static void *pool_page_alloc_meta(struct pool *, int);
static void pool_page_free_meta(struct pool *, void *);

struct pool_allocator pool_allocator_kmem = {
	.pa_alloc = pool_page_alloc,
	.pa_free = pool_page_free,
	.pa_pagesz = 0
};

struct pool_allocator pool_allocator_nointr = {
	.pa_alloc = pool_page_alloc,
	.pa_free = pool_page_free,
	.pa_pagesz = 0
};

struct pool_allocator pool_allocator_meta = {
	.pa_alloc = pool_page_alloc_meta,
	.pa_free = pool_page_free_meta,
	.pa_pagesz = 0
};

#define POOL_ALLOCATOR_BIG_BASE 13
static struct pool_allocator pool_allocator_big[] = {
	{
		.pa_alloc = pool_page_alloc,
		.pa_free = pool_page_free,
		.pa_pagesz = 1 << (POOL_ALLOCATOR_BIG_BASE + 0),
	},
	{
		.pa_alloc = pool_page_alloc,
		.pa_free = pool_page_free,
		.pa_pagesz = 1 << (POOL_ALLOCATOR_BIG_BASE + 1),
	},
	{
		.pa_alloc = pool_page_alloc,
		.pa_free = pool_page_free,
		.pa_pagesz = 1 << (POOL_ALLOCATOR_BIG_BASE + 2),
	},
	{
		.pa_alloc = pool_page_alloc,
		.pa_free = pool_page_free,
		.pa_pagesz = 1 << (POOL_ALLOCATOR_BIG_BASE + 3),
	},
	{
		.pa_alloc = pool_page_alloc,
		.pa_free = pool_page_free,
		.pa_pagesz = 1 << (POOL_ALLOCATOR_BIG_BASE + 4),
	},
	{
		.pa_alloc = pool_page_alloc,
		.pa_free = pool_page_free,
		.pa_pagesz = 1 << (POOL_ALLOCATOR_BIG_BASE + 5),
	},
	{
		.pa_alloc = pool_page_alloc,
		.pa_free = pool_page_free,
		.pa_pagesz = 1 << (POOL_ALLOCATOR_BIG_BASE + 6),
	},
	{
		.pa_alloc = pool_page_alloc,
		.pa_free = pool_page_free,
		.pa_pagesz = 1 << (POOL_ALLOCATOR_BIG_BASE + 7),
	},
	{
		.pa_alloc = pool_page_alloc,
		.pa_free = pool_page_free,
		.pa_pagesz = 1 << (POOL_ALLOCATOR_BIG_BASE + 8),
	},
	{
		.pa_alloc = pool_page_alloc,
		.pa_free = pool_page_free,
		.pa_pagesz = 1 << (POOL_ALLOCATOR_BIG_BASE + 9),
	},
	{
		.pa_alloc = pool_page_alloc,
		.pa_free = pool_page_free,
		.pa_pagesz = 1 << (POOL_ALLOCATOR_BIG_BASE + 10),
	},
	{
		.pa_alloc = pool_page_alloc,
		.pa_free = pool_page_free,
		.pa_pagesz = 1 << (POOL_ALLOCATOR_BIG_BASE + 11),
	}
};

static int pool_bigidx(size_t);

/* # of seconds to retain page after last use */
int pool_inactive_time = 10;

/* Next candidate for drainage (see pool_drain()) */
static struct pool *drainpp;

/* This lock protects both pool_head and drainpp. */
static kmutex_t pool_head_lock;
static kcondvar_t pool_busy;

/* This lock protects initialization of a potentially shared pool allocator */
static kmutex_t pool_allocator_lock;

static unsigned int poolid_counter = 0;

typedef uint32_t pool_item_bitmap_t;
#define	BITMAP_SIZE	(CHAR_BIT * sizeof(pool_item_bitmap_t))
#define	BITMAP_MASK	(BITMAP_SIZE - 1)
#define	BITMAP_MIN_SIZE	(CHAR_BIT * sizeof(((struct pool_item_header *)NULL)->ph_u2))

struct pool_item_header {
	/* Page headers */
	LIST_ENTRY(pool_item_header)
				ph_pagelist;	/* pool page list */
	union {
		/* !PR_PHINPAGE */
		struct {
			SPLAY_ENTRY(pool_item_header)
				phu_node;	/* off-page page headers */
		} phu_offpage;
		/* PR_PHINPAGE */
		struct {
			unsigned int phu_poolid;
		} phu_onpage;
	} ph_u1;
	void *			ph_page;	/* this page's address */
	uint32_t		ph_time;	/* last referenced */
	uint16_t		ph_nmissing;	/* # of chunks in use */
	uint16_t		ph_off;		/* start offset in page */
	union {
		/* !PR_USEBMAP */
		struct {
			LIST_HEAD(, pool_item)
				phu_itemlist;	/* chunk list for this page */
		} phu_normal;
		/* PR_USEBMAP */
		struct {
			pool_item_bitmap_t phu_bitmap[1];
		} phu_notouch;
	} ph_u2;
};
#define ph_node		ph_u1.phu_offpage.phu_node
#define ph_poolid	ph_u1.phu_onpage.phu_poolid
#define ph_itemlist	ph_u2.phu_normal.phu_itemlist
#define ph_bitmap	ph_u2.phu_notouch.phu_bitmap

#define PHSIZE	ALIGN(sizeof(struct pool_item_header))

CTASSERT(offsetof(struct pool_item_header, ph_u2) +
    BITMAP_MIN_SIZE / CHAR_BIT == sizeof(struct pool_item_header));

#if defined(DIAGNOSTIC) && !defined(KASAN)
#define POOL_CHECK_MAGIC
#endif

struct pool_item {
#ifdef POOL_CHECK_MAGIC
	u_int pi_magic;
#endif
#define	PI_MAGIC 0xdeaddeadU
	/* Other entries use only this list entry */
	LIST_ENTRY(pool_item)	pi_list;
};

#define	POOL_NEEDS_CATCHUP(pp)						\
	((pp)->pr_nitems < (pp)->pr_minitems ||				\
	 (pp)->pr_npages < (pp)->pr_minpages)
#define	POOL_OBJ_TO_PAGE(pp, v)						\
	(void *)((uintptr_t)v & pp->pr_alloc->pa_pagemask)

/*
 * Pool cache management.
 *
 * Pool caches provide a way for constructed objects to be cached by the
 * pool subsystem.  This can lead to performance improvements by avoiding
 * needless object construction/destruction; it is deferred until absolutely
 * necessary.
 *
 * Caches are grouped into cache groups.  Each cache group references up
 * to PCG_NUMOBJECTS constructed objects.  When a cache allocates an
 * object from the pool, it calls the object's constructor and places it
 * into a cache group.  When a cache group frees an object back to the
 * pool, it first calls the object's destructor.  This allows the object
 * to persist in constructed form while freed to the cache.
 *
 * The pool references each cache, so that when a pool is drained by the
 * pagedaemon, it can drain each individual cache as well.  Each time a
 * cache is drained, the most idle cache group is freed to the pool in
 * its entirety.
 *
 * Pool caches are laid on top of pools.  By layering them, we can avoid
 * the complexity of cache management for pools which would not benefit
 * from it.
 */

static struct pool pcg_normal_pool;
static struct pool pcg_large_pool;
static struct pool cache_pool;
static struct pool cache_cpu_pool;

static pcg_t *volatile pcg_large_cache __cacheline_aligned;
static pcg_t *volatile pcg_normal_cache __cacheline_aligned;

/* List of all caches. */
TAILQ_HEAD(,pool_cache) pool_cache_head =
    TAILQ_HEAD_INITIALIZER(pool_cache_head);

int pool_cache_disable;		/* global disable for caching */
static const pcg_t pcg_dummy;	/* zero sized: always empty, yet always full */

static bool	pool_cache_put_slow(pool_cache_t, pool_cache_cpu_t *, int,
				    void *);
static bool	pool_cache_get_slow(pool_cache_t, pool_cache_cpu_t *, int,
				    void **, paddr_t *, int);
static void	pool_cache_cpu_init1(struct cpu_info *, pool_cache_t);
static int	pool_cache_invalidate_groups(pool_cache_t, pcg_t *);
static void	pool_cache_invalidate_cpu(pool_cache_t, u_int);
static void	pool_cache_transfer(pool_cache_t);
static int	pool_pcg_get(pcg_t *volatile *, pcg_t **);
static int	pool_pcg_put(pcg_t *volatile *, pcg_t *);
static pcg_t *	pool_pcg_trunc(pcg_t *volatile *);

static int	pool_catchup(struct pool *);
static void	pool_prime_page(struct pool *, void *,
		    struct pool_item_header *);
static void	pool_update_curpage(struct pool *);

static int	pool_grow(struct pool *, int);
static void	*pool_allocator_alloc(struct pool *, int);
static void	pool_allocator_free(struct pool *, void *);

static void pool_print_pagelist(struct pool *, struct pool_pagelist *,
	void (*)(const char *, ...) __printflike(1, 2));
static void pool_print1(struct pool *, const char *,
	void (*)(const char *, ...) __printflike(1, 2));

static int pool_chk_page(struct pool *, const char *,
			 struct pool_item_header *);

/* -------------------------------------------------------------------------- */

static inline unsigned int
pr_item_bitmap_index(const struct pool *pp, const struct pool_item_header *ph,
    const void *v)
{
	const char *cp = v;
	unsigned int idx;

	KASSERT(pp->pr_roflags & PR_USEBMAP);
	idx = (cp - (char *)ph->ph_page - ph->ph_off) / pp->pr_size;

	if (__predict_false(idx >= pp->pr_itemsperpage)) {
		panic("%s: [%s] %u >= %u", __func__, pp->pr_wchan, idx,
		    pp->pr_itemsperpage);
	}

	return idx;
}

static inline void
pr_item_bitmap_put(const struct pool *pp, struct pool_item_header *ph,
    void *obj)
{
	unsigned int idx = pr_item_bitmap_index(pp, ph, obj);
	pool_item_bitmap_t *bitmap = ph->ph_bitmap + (idx / BITMAP_SIZE);
	pool_item_bitmap_t mask = 1U << (idx & BITMAP_MASK);

	if (__predict_false((*bitmap & mask) != 0)) {
		panic("%s: [%s] %p already freed", __func__, pp->pr_wchan, obj);
	}

	*bitmap |= mask;
}

static inline void *
pr_item_bitmap_get(const struct pool *pp, struct pool_item_header *ph)
{
	pool_item_bitmap_t *bitmap = ph->ph_bitmap;
	unsigned int idx;
	int i;

	for (i = 0; ; i++) {
		int bit;

		KASSERT((i * BITMAP_SIZE) < pp->pr_itemsperpage);
		bit = ffs32(bitmap[i]);
		if (bit) {
			pool_item_bitmap_t mask;

			bit--;
			idx = (i * BITMAP_SIZE) + bit;
			mask = 1U << bit;
			KASSERT((bitmap[i] & mask) != 0);
			bitmap[i] &= ~mask;
			break;
		}
	}
	KASSERT(idx < pp->pr_itemsperpage);
	return (char *)ph->ph_page + ph->ph_off + idx * pp->pr_size;
}

static inline void
pr_item_bitmap_init(const struct pool *pp, struct pool_item_header *ph)
{
	pool_item_bitmap_t *bitmap = ph->ph_bitmap;
	const int n = howmany(pp->pr_itemsperpage, BITMAP_SIZE);
	int i;

	for (i = 0; i < n; i++) {
		bitmap[i] = (pool_item_bitmap_t)-1;
	}
}

/* -------------------------------------------------------------------------- */

static inline void
pr_item_linkedlist_put(const struct pool *pp, struct pool_item_header *ph,
    void *obj)
{
	struct pool_item *pi = obj;

	KASSERT(!pp_has_pser(pp));

#ifdef POOL_CHECK_MAGIC
	pi->pi_magic = PI_MAGIC;
#endif

	if (pp->pr_redzone) {
		/*
		 * Mark the pool_item as valid. The rest is already
		 * invalid.
		 */
		kasan_mark(pi, sizeof(*pi), sizeof(*pi), 0);
	}

	LIST_INSERT_HEAD(&ph->ph_itemlist, pi, pi_list);
}

static inline void *
pr_item_linkedlist_get(struct pool *pp, struct pool_item_header *ph)
{
	struct pool_item *pi;
	void *v;

	v = pi = LIST_FIRST(&ph->ph_itemlist);
	if (__predict_false(v == NULL)) {
		mutex_exit(&pp->pr_lock);
		panic("%s: [%s] page empty", __func__, pp->pr_wchan);
	}
	KASSERTMSG((pp->pr_nitems > 0),
	    "%s: [%s] nitems %u inconsistent on itemlist",
	    __func__, pp->pr_wchan, pp->pr_nitems);
#ifdef POOL_CHECK_MAGIC
	KASSERTMSG((pi->pi_magic == PI_MAGIC),
	    "%s: [%s] free list modified: "
	    "magic=%x; page %p; item addr %p", __func__,
	    pp->pr_wchan, pi->pi_magic, ph->ph_page, pi);
#endif

	/*
	 * Remove from item list.
	 */
	LIST_REMOVE(pi, pi_list);

	return v;
}

/* -------------------------------------------------------------------------- */

static inline void
pr_phinpage_check(struct pool *pp, struct pool_item_header *ph, void *page,
    void *object)
{
	if (__predict_false((void *)ph->ph_page != page)) {
		panic("%s: [%s] item %p not part of pool", __func__,
		    pp->pr_wchan, object);
	}
	if (__predict_false((char *)object < (char *)page + ph->ph_off)) {
		panic("%s: [%s] item %p below item space", __func__,
		    pp->pr_wchan, object);
	}
	if (__predict_false(ph->ph_poolid != pp->pr_poolid)) {
		panic("%s: [%s] item %p poolid %u != %u", __func__,
		    pp->pr_wchan, object, ph->ph_poolid, pp->pr_poolid);
	}
}

static inline void
pc_phinpage_check(pool_cache_t pc, void *object)
{
	struct pool_item_header *ph;
	struct pool *pp;
	void *page;

	pp = &pc->pc_pool;
	page = POOL_OBJ_TO_PAGE(pp, object);
	ph = (struct pool_item_header *)page;

	pr_phinpage_check(pp, ph, page, object);
}

/* -------------------------------------------------------------------------- */

static inline int
phtree_compare(struct pool_item_header *a, struct pool_item_header *b)
{

	/*
	 * We consider pool_item_header with smaller ph_page bigger. This
	 * unnatural ordering is for the benefit of pr_find_pagehead.
	 */
	if (a->ph_page < b->ph_page)
		return 1;
	else if (a->ph_page > b->ph_page)
		return -1;
	else
		return 0;
}

SPLAY_PROTOTYPE(phtree, pool_item_header, ph_node, phtree_compare);
SPLAY_GENERATE(phtree, pool_item_header, ph_node, phtree_compare);

static inline struct pool_item_header *
pr_find_pagehead_noalign(struct pool *pp, void *v)
{
	struct pool_item_header *ph, tmp;

	tmp.ph_page = (void *)(uintptr_t)v;
	ph = SPLAY_FIND(phtree, &pp->pr_phtree, &tmp);
	if (ph == NULL) {
		ph = SPLAY_ROOT(&pp->pr_phtree);
		if (ph != NULL && phtree_compare(&tmp, ph) >= 0) {
			ph = SPLAY_NEXT(phtree, &pp->pr_phtree, ph);
		}
		KASSERT(ph == NULL || phtree_compare(&tmp, ph) < 0);
	}

	return ph;
}

/*
 * Return the pool page header based on item address.
 */
static inline struct pool_item_header *
pr_find_pagehead(struct pool *pp, void *v)
{
	struct pool_item_header *ph, tmp;

	if ((pp->pr_roflags & PR_NOALIGN) != 0) {
		ph = pr_find_pagehead_noalign(pp, v);
	} else {
		void *page = POOL_OBJ_TO_PAGE(pp, v);
		if ((pp->pr_roflags & PR_PHINPAGE) != 0) {
			ph = (struct pool_item_header *)page;
			pr_phinpage_check(pp, ph, page, v);
		} else {
			tmp.ph_page = page;
			ph = SPLAY_FIND(phtree, &pp->pr_phtree, &tmp);
		}
	}

	KASSERT(ph == NULL || ((pp->pr_roflags & PR_PHINPAGE) != 0) ||
	    ((char *)ph->ph_page <= (char *)v &&
	    (char *)v < (char *)ph->ph_page + pp->pr_alloc->pa_pagesz));
	return ph;
}

static void
pr_pagelist_free(struct pool *pp, struct pool_pagelist *pq)
{
	struct pool_item_header *ph;

	while ((ph = LIST_FIRST(pq)) != NULL) {
		LIST_REMOVE(ph, ph_pagelist);
		pool_allocator_free(pp, ph->ph_page);
		if ((pp->pr_roflags & PR_PHINPAGE) == 0)
			pool_put(pp->pr_phpool, ph);
	}
}

/*
 * Remove a page from the pool.
 */
static inline void
pr_rmpage(struct pool *pp, struct pool_item_header *ph,
     struct pool_pagelist *pq)
{

	KASSERT(mutex_owned(&pp->pr_lock));

	/*
	 * If the page was idle, decrement the idle page count.
	 */
	if (ph->ph_nmissing == 0) {
		KASSERT(pp->pr_nidle != 0);
		KASSERTMSG((pp->pr_nitems >= pp->pr_itemsperpage),
		    "%s: [%s] nitems=%u < itemsperpage=%u", __func__,
		    pp->pr_wchan, pp->pr_nitems, pp->pr_itemsperpage);
		pp->pr_nidle--;
	}

	pp->pr_nitems -= pp->pr_itemsperpage;

	/*
	 * Unlink the page from the pool and queue it for release.
	 */
	LIST_REMOVE(ph, ph_pagelist);
	if (pp->pr_roflags & PR_PHINPAGE) {
		if (__predict_false(ph->ph_poolid != pp->pr_poolid)) {
			panic("%s: [%s] ph %p poolid %u != %u",
			    __func__, pp->pr_wchan, ph, ph->ph_poolid,
			    pp->pr_poolid);
		}
	} else {
		SPLAY_REMOVE(phtree, &pp->pr_phtree, ph);
	}
	LIST_INSERT_HEAD(pq, ph, ph_pagelist);

	pp->pr_npages--;
	pp->pr_npagefree++;

	pool_update_curpage(pp);
}

/*
 * Initialize all the pools listed in the "pools" link set.
 */
void
pool_subsystem_init(void)
{
	size_t size;
	int idx;

	mutex_init(&pool_head_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&pool_allocator_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&pool_busy, "poolbusy");

	/*
	 * Initialize private page header pool and cache magazine pool if we
	 * haven't done so yet.
	 */
	for (idx = 0; idx < PHPOOL_MAX; idx++) {
		static char phpool_names[PHPOOL_MAX][6+1+6+1];
		int nelem;
		size_t sz;

		nelem = PHPOOL_FREELIST_NELEM(idx);
		KASSERT(nelem != 0);
		snprintf(phpool_names[idx], sizeof(phpool_names[idx]),
		    "phpool-%d", nelem);
		sz = offsetof(struct pool_item_header,
		    ph_bitmap[howmany(nelem, BITMAP_SIZE)]);
		pool_init(&phpool[idx], sz, 0, 0, 0,
		    phpool_names[idx], &pool_allocator_meta, IPL_VM);
	}

	size = sizeof(pcg_t) +
	    (PCG_NOBJECTS_NORMAL - 1) * sizeof(pcgpair_t);
	pool_init(&pcg_normal_pool, size, coherency_unit, 0, 0,
	    "pcgnormal", &pool_allocator_meta, IPL_VM);

	size = sizeof(pcg_t) +
	    (PCG_NOBJECTS_LARGE - 1) * sizeof(pcgpair_t);
	pool_init(&pcg_large_pool, size, coherency_unit, 0, 0,
	    "pcglarge", &pool_allocator_meta, IPL_VM);

	pool_init(&cache_pool, sizeof(struct pool_cache), coherency_unit,
	    0, 0, "pcache", &pool_allocator_meta, IPL_NONE);

	pool_init(&cache_cpu_pool, sizeof(pool_cache_cpu_t), coherency_unit,
	    0, 0, "pcachecpu", &pool_allocator_meta, IPL_NONE);
}

static inline bool
pool_init_is_phinpage(const struct pool *pp)
{
	size_t pagesize;

	if (pp->pr_roflags & PR_PHINPAGE) {
		return true;
	}
	if (pp->pr_roflags & (PR_NOTOUCH | PR_NOALIGN)) {
		return false;
	}

	pagesize = pp->pr_alloc->pa_pagesz;

	/*
	 * Threshold: the item size is below 1/16 of a page size, and below
	 * 8 times the page header size. The latter ensures we go off-page
	 * if the page header would make us waste a rather big item.
	 */
	if (pp->pr_size < MIN(pagesize / 16, PHSIZE * 8)) {
		return true;
	}

	/* Put the header into the page if it doesn't waste any items. */
	if (pagesize / pp->pr_size == (pagesize - PHSIZE) / pp->pr_size) {
		return true;
	}

	return false;
}

static inline bool
pool_init_is_usebmap(const struct pool *pp)
{
	size_t bmapsize;

	if (pp->pr_roflags & PR_NOTOUCH) {
		return true;
	}

	/*
	 * If we're off-page, go with a bitmap.
	 */
	if (!(pp->pr_roflags & PR_PHINPAGE)) {
		return true;
	}

	/*
	 * If we're on-page, and the page header can already contain a bitmap
	 * big enough to cover all the items of the page, go with a bitmap.
	 */
	bmapsize = roundup(PHSIZE, pp->pr_align) -
	    offsetof(struct pool_item_header, ph_bitmap[0]);
	KASSERT(bmapsize % sizeof(pool_item_bitmap_t) == 0);
	if (pp->pr_itemsperpage <= bmapsize * CHAR_BIT) {
		return true;
	}

	return false;
}

/*
 * Initialize the given pool resource structure.
 *
 * We export this routine to allow other kernel parts to declare
 * static pools that must be initialized before kmem(9) is available.
 */
void
pool_init(struct pool *pp, size_t size, u_int align, u_int ioff, int flags,
    const char *wchan, struct pool_allocator *palloc, int ipl)
{
	struct pool *pp1;
	size_t prsize;
	int itemspace, slack;

	/* XXX ioff will be removed. */
	KASSERT(ioff == 0);

#ifdef DEBUG
	if (__predict_true(!cold))
		mutex_enter(&pool_head_lock);
	/*
	 * Check that the pool hasn't already been initialised and
	 * added to the list of all pools.
	 */
	TAILQ_FOREACH(pp1, &pool_head, pr_poollist) {
		if (pp == pp1)
			panic("%s: [%s] already initialised", __func__,
			    wchan);
	}
	if (__predict_true(!cold))
		mutex_exit(&pool_head_lock);
#endif

	if (palloc == NULL)
		palloc = &pool_allocator_kmem;

	if (!cold)
		mutex_enter(&pool_allocator_lock);
	if (palloc->pa_refcnt++ == 0) {
		if (palloc->pa_pagesz == 0)
			palloc->pa_pagesz = PAGE_SIZE;

		TAILQ_INIT(&palloc->pa_list);

		mutex_init(&palloc->pa_lock, MUTEX_DEFAULT, IPL_VM);
		palloc->pa_pagemask = ~(palloc->pa_pagesz - 1);
		palloc->pa_pageshift = ffs(palloc->pa_pagesz) - 1;
	}
	if (!cold)
		mutex_exit(&pool_allocator_lock);

	/*
	 * PR_PSERIALIZE implies PR_NOTOUCH; freed objects must remain
	 * valid until the the backing page is returned to the system.
	 */
	if (flags & PR_PSERIALIZE) {
		flags |= PR_NOTOUCH;
	}

	if (align == 0)
		align = ALIGN(1);

	prsize = size;
	if ((flags & PR_NOTOUCH) == 0 && prsize < sizeof(struct pool_item))
		prsize = sizeof(struct pool_item);

	prsize = roundup(prsize, align);
	KASSERTMSG((prsize <= palloc->pa_pagesz),
	    "%s: [%s] pool item size (%zu) larger than page size (%u)",
	    __func__, wchan, prsize, palloc->pa_pagesz);

	/*
	 * Initialize the pool structure.
	 */
	LIST_INIT(&pp->pr_emptypages);
	LIST_INIT(&pp->pr_fullpages);
	LIST_INIT(&pp->pr_partpages);
	pp->pr_cache = NULL;
	pp->pr_curpage = NULL;
	pp->pr_npages = 0;
	pp->pr_minitems = 0;
	pp->pr_minpages = 0;
	pp->pr_maxpages = UINT_MAX;
	pp->pr_roflags = flags;
	pp->pr_flags = 0;
	pp->pr_size = prsize;
	pp->pr_reqsize = size;
	pp->pr_align = align;
	pp->pr_wchan = wchan;
	pp->pr_alloc = palloc;
	pp->pr_poolid = atomic_inc_uint_nv(&poolid_counter);
	pp->pr_nitems = 0;
	pp->pr_nout = 0;
	pp->pr_hardlimit = UINT_MAX;
	pp->pr_hardlimit_warning = NULL;
	pp->pr_hardlimit_ratecap.tv_sec = 0;
	pp->pr_hardlimit_ratecap.tv_usec = 0;
	pp->pr_hardlimit_warning_last.tv_sec = 0;
	pp->pr_hardlimit_warning_last.tv_usec = 0;
	pp->pr_drain_hook = NULL;
	pp->pr_drain_hook_arg = NULL;
	pp->pr_freecheck = NULL;
	pp->pr_redzone = false;
	pool_redzone_init(pp, size);
	pool_quarantine_init(pp);

	/*
	 * Decide whether to put the page header off-page to avoid wasting too
	 * large a part of the page or too big an item. Off-page page headers
	 * go on a hash table, so we can match a returned item with its header
	 * based on the page address.
	 */
	if (pool_init_is_phinpage(pp)) {
		/* Use the beginning of the page for the page header */
		itemspace = palloc->pa_pagesz - roundup(PHSIZE, align);
		pp->pr_itemoffset = roundup(PHSIZE, align);
		pp->pr_roflags |= PR_PHINPAGE;
	} else {
		/* The page header will be taken from our page header pool */
		itemspace = palloc->pa_pagesz;
		pp->pr_itemoffset = 0;
		SPLAY_INIT(&pp->pr_phtree);
	}

	pp->pr_itemsperpage = itemspace / pp->pr_size;
	KASSERT(pp->pr_itemsperpage != 0);

	/*
	 * Decide whether to use a bitmap or a linked list to manage freed
	 * items.
	 */
	if (pool_init_is_usebmap(pp)) {
		pp->pr_roflags |= PR_USEBMAP;
	}

	/*
	 * If we're off-page, then we're using a bitmap; choose the appropriate
	 * pool to allocate page headers, whose size varies depending on the
	 * bitmap. If we're on-page, nothing to do.
	 */
	if (!(pp->pr_roflags & PR_PHINPAGE)) {
		int idx;

		KASSERT(pp->pr_roflags & PR_USEBMAP);

		for (idx = 0; pp->pr_itemsperpage > PHPOOL_FREELIST_NELEM(idx);
		    idx++) {
			/* nothing */
		}
		if (idx >= PHPOOL_MAX) {
			/*
			 * if you see this panic, consider to tweak
			 * PHPOOL_MAX and PHPOOL_FREELIST_NELEM.
			 */
			panic("%s: [%s] too large itemsperpage(%d) for "
			    "PR_USEBMAP", __func__,
			    pp->pr_wchan, pp->pr_itemsperpage);
		}
		pp->pr_phpool = &phpool[idx];
	} else {
		pp->pr_phpool = NULL;
	}

	/*
	 * Use the slack between the chunks and the page header
	 * for "cache coloring".
	 */
	slack = itemspace - pp->pr_itemsperpage * pp->pr_size;
	pp->pr_maxcolor = rounddown(slack, align);
	pp->pr_curcolor = 0;

	pp->pr_nget = 0;
	pp->pr_nfail = 0;
	pp->pr_nput = 0;
	pp->pr_npagealloc = 0;
	pp->pr_npagefree = 0;
	pp->pr_hiwat = 0;
	pp->pr_nidle = 0;
	pp->pr_refcnt = 0;

	mutex_init(&pp->pr_lock, MUTEX_DEFAULT, ipl);
	cv_init(&pp->pr_cv, wchan);
	pp->pr_ipl = ipl;

	/* Insert into the list of all pools. */
	if (!cold)
		mutex_enter(&pool_head_lock);
	TAILQ_FOREACH(pp1, &pool_head, pr_poollist) {
		if (strcmp(pp1->pr_wchan, pp->pr_wchan) > 0)
			break;
	}
	if (pp1 == NULL)
		TAILQ_INSERT_TAIL(&pool_head, pp, pr_poollist);
	else
		TAILQ_INSERT_BEFORE(pp1, pp, pr_poollist);
	if (!cold)
		mutex_exit(&pool_head_lock);

	/* Insert this into the list of pools using this allocator. */
	if (!cold)
		mutex_enter(&palloc->pa_lock);
	TAILQ_INSERT_TAIL(&palloc->pa_list, pp, pr_alloc_list);
	if (!cold)
		mutex_exit(&palloc->pa_lock);
}

/*
 * De-commission a pool resource.
 */
void
pool_destroy(struct pool *pp)
{
	struct pool_pagelist pq;
	struct pool_item_header *ph;

	pool_quarantine_flush(pp);

	/* Remove from global pool list */
	mutex_enter(&pool_head_lock);
	while (pp->pr_refcnt != 0)
		cv_wait(&pool_busy, &pool_head_lock);
	TAILQ_REMOVE(&pool_head, pp, pr_poollist);
	if (drainpp == pp)
		drainpp = NULL;
	mutex_exit(&pool_head_lock);

	/* Remove this pool from its allocator's list of pools. */
	mutex_enter(&pp->pr_alloc->pa_lock);
	TAILQ_REMOVE(&pp->pr_alloc->pa_list, pp, pr_alloc_list);
	mutex_exit(&pp->pr_alloc->pa_lock);

	mutex_enter(&pool_allocator_lock);
	if (--pp->pr_alloc->pa_refcnt == 0)
		mutex_destroy(&pp->pr_alloc->pa_lock);
	mutex_exit(&pool_allocator_lock);

	mutex_enter(&pp->pr_lock);

	KASSERT(pp->pr_cache == NULL);
	KASSERTMSG((pp->pr_nout == 0),
	    "%s: [%s] pool busy: still out: %u", __func__, pp->pr_wchan,
	    pp->pr_nout);
	KASSERT(LIST_EMPTY(&pp->pr_fullpages));
	KASSERT(LIST_EMPTY(&pp->pr_partpages));

	/* Remove all pages */
	LIST_INIT(&pq);
	while ((ph = LIST_FIRST(&pp->pr_emptypages)) != NULL)
		pr_rmpage(pp, ph, &pq);

	mutex_exit(&pp->pr_lock);

	pr_pagelist_free(pp, &pq);
	cv_destroy(&pp->pr_cv);
	mutex_destroy(&pp->pr_lock);
}

void
pool_set_drain_hook(struct pool *pp, void (*fn)(void *, int), void *arg)
{

	/* XXX no locking -- must be used just after pool_init() */
	KASSERTMSG((pp->pr_drain_hook == NULL),
	    "%s: [%s] already set", __func__, pp->pr_wchan);
	pp->pr_drain_hook = fn;
	pp->pr_drain_hook_arg = arg;
}

static struct pool_item_header *
pool_alloc_item_header(struct pool *pp, void *storage, int flags)
{
	struct pool_item_header *ph;

	if ((pp->pr_roflags & PR_PHINPAGE) != 0)
		ph = storage;
	else
		ph = pool_get(pp->pr_phpool, flags);

	return ph;
}

/*
 * Grab an item from the pool.
 */
void *
pool_get(struct pool *pp, int flags)
{
	struct pool_item_header *ph;
	void *v;

	KASSERT(!(flags & PR_NOWAIT) != !(flags & PR_WAITOK));
	KASSERTMSG((pp->pr_itemsperpage != 0),
	    "%s: [%s] pr_itemsperpage is zero, "
	    "pool not initialized?", __func__, pp->pr_wchan);
	KASSERTMSG((!(cpu_intr_p() || cpu_softintr_p())
		|| pp->pr_ipl != IPL_NONE || cold || panicstr != NULL),
	    "%s: [%s] is IPL_NONE, but called from interrupt context",
	    __func__, pp->pr_wchan);
	if (flags & PR_WAITOK) {
		ASSERT_SLEEPABLE();
	}

	if (flags & PR_NOWAIT) {
		if (fault_inject())
			return NULL;
	}

	mutex_enter(&pp->pr_lock);
 startover:
	/*
	 * Check to see if we've reached the hard limit.  If we have,
	 * and we can wait, then wait until an item has been returned to
	 * the pool.
	 */
	KASSERTMSG((pp->pr_nout <= pp->pr_hardlimit),
	    "%s: %s: crossed hard limit", __func__, pp->pr_wchan);
	if (__predict_false(pp->pr_nout == pp->pr_hardlimit)) {
		if (pp->pr_drain_hook != NULL) {
			/*
			 * Since the drain hook is going to free things
			 * back to the pool, unlock, call the hook, re-lock,
			 * and check the hardlimit condition again.
			 */
			mutex_exit(&pp->pr_lock);
			(*pp->pr_drain_hook)(pp->pr_drain_hook_arg, flags);
			mutex_enter(&pp->pr_lock);
			if (pp->pr_nout < pp->pr_hardlimit)
				goto startover;
		}

		if ((flags & PR_WAITOK) && !(flags & PR_LIMITFAIL)) {
			/*
			 * XXX: A warning isn't logged in this case.  Should
			 * it be?
			 */
			pp->pr_flags |= PR_WANTED;
			do {
				cv_wait(&pp->pr_cv, &pp->pr_lock);
			} while (pp->pr_flags & PR_WANTED);
			goto startover;
		}

		/*
		 * Log a message that the hard limit has been hit.
		 */
		if (pp->pr_hardlimit_warning != NULL &&
		    ratecheck(&pp->pr_hardlimit_warning_last,
			      &pp->pr_hardlimit_ratecap))
			log(LOG_ERR, "%s\n", pp->pr_hardlimit_warning);

		pp->pr_nfail++;

		mutex_exit(&pp->pr_lock);
		KASSERT((flags & (PR_NOWAIT|PR_LIMITFAIL)) != 0);
		return NULL;
	}

	/*
	 * The convention we use is that if `curpage' is not NULL, then
	 * it points at a non-empty bucket. In particular, `curpage'
	 * never points at a page header which has PR_PHINPAGE set and
	 * has no items in its bucket.
	 */
	if ((ph = pp->pr_curpage) == NULL) {
		int error;

		KASSERTMSG((pp->pr_nitems == 0),
		    "%s: [%s] curpage NULL, inconsistent nitems %u",
		    __func__, pp->pr_wchan, pp->pr_nitems);

		/*
		 * Call the back-end page allocator for more memory.
		 * Release the pool lock, as the back-end page allocator
		 * may block.
		 */
		error = pool_grow(pp, flags);
		if (error != 0) {
			/*
			 * pool_grow aborts when another thread
			 * is allocating a new page. Retry if it
			 * waited for it.
			 */
			if (error == ERESTART)
				goto startover;

			/*
			 * We were unable to allocate a page or item
			 * header, but we released the lock during
			 * allocation, so perhaps items were freed
			 * back to the pool.  Check for this case.
			 */
			if (pp->pr_curpage != NULL)
				goto startover;

			pp->pr_nfail++;
			mutex_exit(&pp->pr_lock);
			KASSERT((flags & (PR_NOWAIT|PR_LIMITFAIL)) != 0);
			return NULL;
		}

		/* Start the allocation process over. */
		goto startover;
	}
	if (pp->pr_roflags & PR_USEBMAP) {
		KASSERTMSG((ph->ph_nmissing < pp->pr_itemsperpage),
		    "%s: [%s] pool page empty", __func__, pp->pr_wchan);
		v = pr_item_bitmap_get(pp, ph);
	} else {
		v = pr_item_linkedlist_get(pp, ph);
	}
	pp->pr_nitems--;
	pp->pr_nout++;
	if (ph->ph_nmissing == 0) {
		KASSERT(pp->pr_nidle > 0);
		pp->pr_nidle--;

		/*
		 * This page was previously empty.  Move it to the list of
		 * partially-full pages.  This page is already curpage.
		 */
		LIST_REMOVE(ph, ph_pagelist);
		LIST_INSERT_HEAD(&pp->pr_partpages, ph, ph_pagelist);
	}
	ph->ph_nmissing++;
	if (ph->ph_nmissing == pp->pr_itemsperpage) {
		KASSERTMSG(((pp->pr_roflags & PR_USEBMAP) ||
			LIST_EMPTY(&ph->ph_itemlist)),
		    "%s: [%s] nmissing (%u) inconsistent", __func__,
			pp->pr_wchan, ph->ph_nmissing);
		/*
		 * This page is now full.  Move it to the full list
		 * and select a new current page.
		 */
		LIST_REMOVE(ph, ph_pagelist);
		LIST_INSERT_HEAD(&pp->pr_fullpages, ph, ph_pagelist);
		pool_update_curpage(pp);
	}

	pp->pr_nget++;

	/*
	 * If we have a low water mark and we are now below that low
	 * water mark, add more items to the pool.
	 */
	if (POOL_NEEDS_CATCHUP(pp) && pool_catchup(pp) != 0) {
		/*
		 * XXX: Should we log a warning?  Should we set up a timeout
		 * to try again in a second or so?  The latter could break
		 * a caller's assumptions about interrupt protection, etc.
		 */
	}

	mutex_exit(&pp->pr_lock);
	KASSERT((((vaddr_t)v) & (pp->pr_align - 1)) == 0);
	FREECHECK_OUT(&pp->pr_freecheck, v);
	pool_redzone_fill(pp, v);
	pool_get_kmsan(pp, v);
	if (flags & PR_ZERO)
		memset(v, 0, pp->pr_reqsize);
	return v;
}

/*
 * Internal version of pool_put().  Pool is already locked/entered.
 */
static void
pool_do_put(struct pool *pp, void *v, struct pool_pagelist *pq)
{
	struct pool_item_header *ph;

	KASSERT(mutex_owned(&pp->pr_lock));
	pool_redzone_check(pp, v);
	pool_put_kmsan(pp, v);
	FREECHECK_IN(&pp->pr_freecheck, v);
	LOCKDEBUG_MEM_CHECK(v, pp->pr_size);

	KASSERTMSG((pp->pr_nout > 0),
	    "%s: [%s] putting with none out", __func__, pp->pr_wchan);

	if (__predict_false((ph = pr_find_pagehead(pp, v)) == NULL)) {
		panic("%s: [%s] page header missing", __func__,  pp->pr_wchan);
	}

	/*
	 * Return to item list.
	 */
	if (pp->pr_roflags & PR_USEBMAP) {
		pr_item_bitmap_put(pp, ph, v);
	} else {
		pr_item_linkedlist_put(pp, ph, v);
	}
	KDASSERT(ph->ph_nmissing != 0);
	ph->ph_nmissing--;
	pp->pr_nput++;
	pp->pr_nitems++;
	pp->pr_nout--;

	/* Cancel "pool empty" condition if it exists */
	if (pp->pr_curpage == NULL)
		pp->pr_curpage = ph;

	if (pp->pr_flags & PR_WANTED) {
		pp->pr_flags &= ~PR_WANTED;
		cv_broadcast(&pp->pr_cv);
	}

	/*
	 * If this page is now empty, do one of two things:
	 *
	 *	(1) If we have more pages than the page high water mark,
	 *	    free the page back to the system.  ONLY CONSIDER
	 *	    FREEING BACK A PAGE IF WE HAVE MORE THAN OUR MINIMUM PAGE
	 *	    CLAIM.
	 *
	 *	(2) Otherwise, move the page to the empty page list.
	 *
	 * Either way, select a new current page (so we use a partially-full
	 * page if one is available).
	 */
	if (ph->ph_nmissing == 0) {
		pp->pr_nidle++;
		if (pp->pr_nitems - pp->pr_itemsperpage >= pp->pr_minitems &&
		    pp->pr_npages > pp->pr_minpages &&
		    pp->pr_npages > pp->pr_maxpages) {
			pr_rmpage(pp, ph, pq);
		} else {
			LIST_REMOVE(ph, ph_pagelist);
			LIST_INSERT_HEAD(&pp->pr_emptypages, ph, ph_pagelist);

			/*
			 * Update the timestamp on the page.  A page must
			 * be idle for some period of time before it can
			 * be reclaimed by the pagedaemon.  This minimizes
			 * ping-pong'ing for memory.
			 *
			 * note for 64-bit time_t: truncating to 32-bit is not
			 * a problem for our usage.
			 */
			ph->ph_time = time_uptime;
		}
		pool_update_curpage(pp);
	}

	/*
	 * If the page was previously completely full, move it to the
	 * partially-full list and make it the current page.  The next
	 * allocation will get the item from this page, instead of
	 * further fragmenting the pool.
	 */
	else if (ph->ph_nmissing == (pp->pr_itemsperpage - 1)) {
		LIST_REMOVE(ph, ph_pagelist);
		LIST_INSERT_HEAD(&pp->pr_partpages, ph, ph_pagelist);
		pp->pr_curpage = ph;
	}
}

void
pool_put(struct pool *pp, void *v)
{
	struct pool_pagelist pq;

	LIST_INIT(&pq);

	mutex_enter(&pp->pr_lock);
	if (!pool_put_quarantine(pp, v, &pq)) {
		pool_do_put(pp, v, &pq);
	}
	mutex_exit(&pp->pr_lock);

	pr_pagelist_free(pp, &pq);
}

/*
 * pool_grow: grow a pool by a page.
 *
 * => called with pool locked.
 * => unlock and relock the pool.
 * => return with pool locked.
 */

static int
pool_grow(struct pool *pp, int flags)
{
	struct pool_item_header *ph;
	char *storage;

	/*
	 * If there's a pool_grow in progress, wait for it to complete
	 * and try again from the top.
	 */
	if (pp->pr_flags & PR_GROWING) {
		if (flags & PR_WAITOK) {
			do {
				cv_wait(&pp->pr_cv, &pp->pr_lock);
			} while (pp->pr_flags & PR_GROWING);
			return ERESTART;
		} else {
			if (pp->pr_flags & PR_GROWINGNOWAIT) {
				/*
				 * This needs an unlock/relock dance so
				 * that the other caller has a chance to
				 * run and actually do the thing.  Note
				 * that this is effectively a busy-wait.
				 */
				mutex_exit(&pp->pr_lock);
				mutex_enter(&pp->pr_lock);
				return ERESTART;
			}
			return EWOULDBLOCK;
		}
	}
	pp->pr_flags |= PR_GROWING;
	if (flags & PR_WAITOK)
		mutex_exit(&pp->pr_lock);
	else
		pp->pr_flags |= PR_GROWINGNOWAIT;

	storage = pool_allocator_alloc(pp, flags);
	if (__predict_false(storage == NULL))
		goto out;

	ph = pool_alloc_item_header(pp, storage, flags);
	if (__predict_false(ph == NULL)) {
		pool_allocator_free(pp, storage);
		goto out;
	}

	if (flags & PR_WAITOK)
		mutex_enter(&pp->pr_lock);
	pool_prime_page(pp, storage, ph);
	pp->pr_npagealloc++;
	KASSERT(pp->pr_flags & PR_GROWING);
	pp->pr_flags &= ~(PR_GROWING|PR_GROWINGNOWAIT);
	/*
	 * If anyone was waiting for pool_grow, notify them that we
	 * may have just done it.
	 */
	cv_broadcast(&pp->pr_cv);
	return 0;
out:
	if (flags & PR_WAITOK)
		mutex_enter(&pp->pr_lock);
	KASSERT(pp->pr_flags & PR_GROWING);
	pp->pr_flags &= ~(PR_GROWING|PR_GROWINGNOWAIT);
	return ENOMEM;
}

void
pool_prime(struct pool *pp, int n)
{

	mutex_enter(&pp->pr_lock);
	pp->pr_minpages = roundup(n, pp->pr_itemsperpage) / pp->pr_itemsperpage;
	if (pp->pr_maxpages <= pp->pr_minpages)
		pp->pr_maxpages = pp->pr_minpages + 1;	/* XXX */
	while (pp->pr_npages < pp->pr_minpages)
		(void) pool_grow(pp, PR_WAITOK);
	mutex_exit(&pp->pr_lock);
}

/*
 * Add a page worth of items to the pool.
 *
 * Note, we must be called with the pool descriptor LOCKED.
 */
static void
pool_prime_page(struct pool *pp, void *storage, struct pool_item_header *ph)
{
	const unsigned int align = pp->pr_align;
	struct pool_item *pi;
	void *cp = storage;
	int n;

	KASSERT(mutex_owned(&pp->pr_lock));
	KASSERTMSG(((pp->pr_roflags & PR_NOALIGN) ||
		(((uintptr_t)cp & (pp->pr_alloc->pa_pagesz - 1)) == 0)),
	    "%s: [%s] unaligned page: %p", __func__, pp->pr_wchan, cp);

	/*
	 * Insert page header.
	 */
	LIST_INSERT_HEAD(&pp->pr_emptypages, ph, ph_pagelist);
	LIST_INIT(&ph->ph_itemlist);
	ph->ph_page = storage;
	ph->ph_nmissing = 0;
	ph->ph_time = time_uptime;
	if (pp->pr_roflags & PR_PHINPAGE)
		ph->ph_poolid = pp->pr_poolid;
	else
		SPLAY_INSERT(phtree, &pp->pr_phtree, ph);

	pp->pr_nidle++;

	/*
	 * The item space starts after the on-page header, if any.
	 */
	ph->ph_off = pp->pr_itemoffset;

	/*
	 * Color this page.
	 */
	ph->ph_off += pp->pr_curcolor;
	cp = (char *)cp + ph->ph_off;
	if ((pp->pr_curcolor += align) > pp->pr_maxcolor)
		pp->pr_curcolor = 0;

	KASSERT((((vaddr_t)cp) & (align - 1)) == 0);

	/*
	 * Insert remaining chunks on the bucket list.
	 */
	n = pp->pr_itemsperpage;
	pp->pr_nitems += n;

	if (pp->pr_roflags & PR_USEBMAP) {
		pr_item_bitmap_init(pp, ph);
	} else {
		while (n--) {
			pi = (struct pool_item *)cp;

			KASSERT((((vaddr_t)pi) & (align - 1)) == 0);

			/* Insert on page list */
			LIST_INSERT_HEAD(&ph->ph_itemlist, pi, pi_list);
#ifdef POOL_CHECK_MAGIC
			pi->pi_magic = PI_MAGIC;
#endif
			cp = (char *)cp + pp->pr_size;

			KASSERT((((vaddr_t)cp) & (align - 1)) == 0);
		}
	}

	/*
	 * If the pool was depleted, point at the new page.
	 */
	if (pp->pr_curpage == NULL)
		pp->pr_curpage = ph;

	if (++pp->pr_npages > pp->pr_hiwat)
		pp->pr_hiwat = pp->pr_npages;
}

/*
 * Used by pool_get() when nitems drops below the low water mark.  This
 * is used to catch up pr_nitems with the low water mark.
 *
 * Note 1, we never wait for memory here, we let the caller decide what to do.
 *
 * Note 2, we must be called with the pool already locked, and we return
 * with it locked.
 */
static int
pool_catchup(struct pool *pp)
{
	int error = 0;

	while (POOL_NEEDS_CATCHUP(pp)) {
		error = pool_grow(pp, PR_NOWAIT);
		if (error) {
			if (error == ERESTART)
				continue;
			break;
		}
	}
	return error;
}

static void
pool_update_curpage(struct pool *pp)
{

	pp->pr_curpage = LIST_FIRST(&pp->pr_partpages);
	if (pp->pr_curpage == NULL) {
		pp->pr_curpage = LIST_FIRST(&pp->pr_emptypages);
	}
	KASSERT((pp->pr_curpage == NULL && pp->pr_nitems == 0) ||
	    (pp->pr_curpage != NULL && pp->pr_nitems > 0));
}

void
pool_setlowat(struct pool *pp, int n)
{

	mutex_enter(&pp->pr_lock);
	pp->pr_minitems = n;

	/* Make sure we're caught up with the newly-set low water mark. */
	if (POOL_NEEDS_CATCHUP(pp) && pool_catchup(pp) != 0) {
		/*
		 * XXX: Should we log a warning?  Should we set up a timeout
		 * to try again in a second or so?  The latter could break
		 * a caller's assumptions about interrupt protection, etc.
		 */
	}

	mutex_exit(&pp->pr_lock);
}

void
pool_sethiwat(struct pool *pp, int n)
{

	mutex_enter(&pp->pr_lock);

	pp->pr_maxitems = n;

	mutex_exit(&pp->pr_lock);
}

void
pool_sethardlimit(struct pool *pp, int n, const char *warnmess, int ratecap)
{

	mutex_enter(&pp->pr_lock);

	pp->pr_hardlimit = n;
	pp->pr_hardlimit_warning = warnmess;
	pp->pr_hardlimit_ratecap.tv_sec = ratecap;
	pp->pr_hardlimit_warning_last.tv_sec = 0;
	pp->pr_hardlimit_warning_last.tv_usec = 0;

	pp->pr_maxpages = roundup(n, pp->pr_itemsperpage) / pp->pr_itemsperpage;

	mutex_exit(&pp->pr_lock);
}

unsigned int
pool_nget(struct pool *pp)
{

	return pp->pr_nget;
}

unsigned int
pool_nput(struct pool *pp)
{

	return pp->pr_nput;
}

/*
 * Release all complete pages that have not been used recently.
 *
 * Must not be called from interrupt context.
 */
int
pool_reclaim(struct pool *pp)
{
	struct pool_item_header *ph, *phnext;
	struct pool_pagelist pq;
	struct pool_cache *pc;
	uint32_t curtime;
	bool klock;
	int rv;

	KASSERT(!cpu_intr_p() && !cpu_softintr_p());

	if (pp->pr_drain_hook != NULL) {
		/*
		 * The drain hook must be called with the pool unlocked.
		 */
		(*pp->pr_drain_hook)(pp->pr_drain_hook_arg, PR_NOWAIT);
	}

	/*
	 * XXXSMP Because we do not want to cause non-MPSAFE code
	 * to block.
	 */
	if (pp->pr_ipl == IPL_SOFTNET || pp->pr_ipl == IPL_SOFTCLOCK ||
	    pp->pr_ipl == IPL_SOFTSERIAL) {
		KERNEL_LOCK(1, NULL);
		klock = true;
	} else
		klock = false;

	/* Reclaim items from the pool's cache (if any). */
	if ((pc = atomic_load_consume(&pp->pr_cache)) != NULL)
		pool_cache_invalidate(pc);

	if (mutex_tryenter(&pp->pr_lock) == 0) {
		if (klock) {
			KERNEL_UNLOCK_ONE(NULL);
		}
		return 0;
	}

	LIST_INIT(&pq);

	curtime = time_uptime;

	for (ph = LIST_FIRST(&pp->pr_emptypages); ph != NULL; ph = phnext) {
		phnext = LIST_NEXT(ph, ph_pagelist);

		/* Check our minimum page claim */
		if (pp->pr_npages <= pp->pr_minpages)
			break;

		KASSERT(ph->ph_nmissing == 0);
		if (curtime - ph->ph_time < pool_inactive_time)
			continue;

		/*
		 * If freeing this page would put us below the minimum free items
		 * or the minimum pages, stop now.
		 */
		if (pp->pr_nitems - pp->pr_itemsperpage < pp->pr_minitems ||
		    pp->pr_npages - 1 < pp->pr_minpages)
			break;

		pr_rmpage(pp, ph, &pq);
	}

	mutex_exit(&pp->pr_lock);

	if (LIST_EMPTY(&pq))
		rv = 0;
	else {
		pr_pagelist_free(pp, &pq);
		rv = 1;
	}

	if (klock) {
		KERNEL_UNLOCK_ONE(NULL);
	}

	return rv;
}

/*
 * Drain pools, one at a time. The drained pool is returned within ppp.
 *
 * Note, must never be called from interrupt context.
 */
bool
pool_drain(struct pool **ppp)
{
	bool reclaimed;
	struct pool *pp;

	KASSERT(!TAILQ_EMPTY(&pool_head));

	pp = NULL;

	/* Find next pool to drain, and add a reference. */
	mutex_enter(&pool_head_lock);
	do {
		if (drainpp == NULL) {
			drainpp = TAILQ_FIRST(&pool_head);
		}
		if (drainpp != NULL) {
			pp = drainpp;
			drainpp = TAILQ_NEXT(pp, pr_poollist);
		}
		/*
		 * Skip completely idle pools.  We depend on at least
		 * one pool in the system being active.
		 */
	} while (pp == NULL || pp->pr_npages == 0);
	pp->pr_refcnt++;
	mutex_exit(&pool_head_lock);

	/* Drain the cache (if any) and pool.. */
	reclaimed = pool_reclaim(pp);

	/* Finally, unlock the pool. */
	mutex_enter(&pool_head_lock);
	pp->pr_refcnt--;
	cv_broadcast(&pool_busy);
	mutex_exit(&pool_head_lock);

	if (ppp != NULL)
		*ppp = pp;

	return reclaimed;
}

/*
 * Calculate the total number of pages consumed by pools.
 */
int
pool_totalpages(void)
{

	mutex_enter(&pool_head_lock);
	int pages = pool_totalpages_locked();
	mutex_exit(&pool_head_lock);

	return pages;
}

int
pool_totalpages_locked(void)
{
	struct pool *pp;
	uint64_t total = 0;

	TAILQ_FOREACH(pp, &pool_head, pr_poollist) {
		uint64_t bytes = pp->pr_npages * pp->pr_alloc->pa_pagesz;

		if ((pp->pr_roflags & PR_RECURSIVE) != 0)
			bytes -= (pp->pr_nout * pp->pr_size);
		total += bytes;
	}

	return atop(total);
}

/*
 * Diagnostic helpers.
 */

void
pool_printall(const char *modif, void (*pr)(const char *, ...))
{
	struct pool *pp;

	TAILQ_FOREACH(pp, &pool_head, pr_poollist) {
		pool_printit(pp, modif, pr);
	}
}

void
pool_printit(struct pool *pp, const char *modif, void (*pr)(const char *, ...))
{

	if (pp == NULL) {
		(*pr)("Must specify a pool to print.\n");
		return;
	}

	pool_print1(pp, modif, pr);
}

static void
pool_print_pagelist(struct pool *pp, struct pool_pagelist *pl,
    void (*pr)(const char *, ...))
{
	struct pool_item_header *ph;

	LIST_FOREACH(ph, pl, ph_pagelist) {
		(*pr)("\t\tpage %p, nmissing %d, time %" PRIu32 "\n",
		    ph->ph_page, ph->ph_nmissing, ph->ph_time);
#ifdef POOL_CHECK_MAGIC
		struct pool_item *pi;
		if (!(pp->pr_roflags & PR_USEBMAP)) {
			LIST_FOREACH(pi, &ph->ph_itemlist, pi_list) {
				if (pi->pi_magic != PI_MAGIC) {
					(*pr)("\t\t\titem %p, magic 0x%x\n",
					    pi, pi->pi_magic);
				}
			}
		}
#endif
	}
}

static void
pool_print1(struct pool *pp, const char *modif, void (*pr)(const char *, ...))
{
	struct pool_item_header *ph;
	pool_cache_t pc;
	pcg_t *pcg;
	pool_cache_cpu_t *cc;
	uint64_t cpuhit, cpumiss, pchit, pcmiss;
	uint32_t nfull;
	int i;
	bool print_log = false, print_pagelist = false, print_cache = false;
	bool print_short = false, skip_empty = false;
	char c;

	while ((c = *modif++) != '\0') {
		if (c == 'l')
			print_log = true;
		if (c == 'p')
			print_pagelist = true;
		if (c == 'c')
			print_cache = true;
		if (c == 's')
			print_short = true;
		if (c == 'S')
			skip_empty = true;
	}

	if (skip_empty && pp->pr_nget == 0)
		return;

	if ((pc = atomic_load_consume(&pp->pr_cache)) != NULL) {
		(*pr)("POOLCACHE");
	} else {
		(*pr)("POOL");
	}

	/* Single line output. */
	if (print_short) {
		(*pr)(" %s:%p:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u\n",
		    pp->pr_wchan, pp, pp->pr_size, pp->pr_align, pp->pr_npages,
		    pp->pr_nitems, pp->pr_nout, pp->pr_nget, pp->pr_nput,
		    pp->pr_npagealloc, pp->pr_npagefree, pp->pr_nidle);
		return;
	}

	(*pr)(" %s: size %u, align %u, ioff %u, roflags 0x%08x\n",
	    pp->pr_wchan, pp->pr_size, pp->pr_align, pp->pr_itemoffset,
	    pp->pr_roflags);
	(*pr)("\tpool %p, alloc %p\n", pp, pp->pr_alloc);
	(*pr)("\tminitems %u, minpages %u, maxpages %u, npages %u\n",
	    pp->pr_minitems, pp->pr_minpages, pp->pr_maxpages, pp->pr_npages);
	(*pr)("\titemsperpage %u, nitems %u, nout %u, hardlimit %u\n",
	    pp->pr_itemsperpage, pp->pr_nitems, pp->pr_nout, pp->pr_hardlimit);

	(*pr)("\tnget %lu, nfail %lu, nput %lu\n",
	    pp->pr_nget, pp->pr_nfail, pp->pr_nput);
	(*pr)("\tnpagealloc %lu, npagefree %lu, hiwat %u, nidle %lu\n",
	    pp->pr_npagealloc, pp->pr_npagefree, pp->pr_hiwat, pp->pr_nidle);

	if (!print_pagelist)
		goto skip_pagelist;

	if ((ph = LIST_FIRST(&pp->pr_emptypages)) != NULL)
		(*pr)("\n\tempty page list:\n");
	pool_print_pagelist(pp, &pp->pr_emptypages, pr);
	if ((ph = LIST_FIRST(&pp->pr_fullpages)) != NULL)
		(*pr)("\n\tfull page list:\n");
	pool_print_pagelist(pp, &pp->pr_fullpages, pr);
	if ((ph = LIST_FIRST(&pp->pr_partpages)) != NULL)
		(*pr)("\n\tpartial-page list:\n");
	pool_print_pagelist(pp, &pp->pr_partpages, pr);

	if (pp->pr_curpage == NULL)
		(*pr)("\tno current page\n");
	else
		(*pr)("\tcurpage %p\n", pp->pr_curpage->ph_page);

 skip_pagelist:
	if (print_log)
		goto skip_log;

	(*pr)("\n");

 skip_log:

#define PR_GROUPLIST(pcg)						\
	(*pr)("\t\tgroup %p: avail %d\n", pcg, pcg->pcg_avail);		\
	for (i = 0; i < pcg->pcg_size; i++) {				\
		if (pcg->pcg_objects[i].pcgo_pa !=			\
		    POOL_PADDR_INVALID) {				\
			(*pr)("\t\t\t%p, 0x%llx\n",			\
			    pcg->pcg_objects[i].pcgo_va,		\
			    (unsigned long long)			\
			    pcg->pcg_objects[i].pcgo_pa);		\
		} else {						\
			(*pr)("\t\t\t%p\n",				\
			    pcg->pcg_objects[i].pcgo_va);		\
		}							\
	}

	if (pc != NULL) {
		cpuhit = 0;
		cpumiss = 0;
		pcmiss = 0;
		nfull = 0;
		for (i = 0; i < __arraycount(pc->pc_cpus); i++) {
			if ((cc = pc->pc_cpus[i]) == NULL)
				continue;
			cpuhit += cc->cc_hits;
			cpumiss += cc->cc_misses;
			pcmiss += cc->cc_pcmisses;
			nfull += cc->cc_nfull;
		}
		pchit = cpumiss - pcmiss;
		(*pr)("\tcpu layer hits %llu misses %llu\n", cpuhit, cpumiss);
		(*pr)("\tcache layer hits %llu misses %llu\n", pchit, pcmiss);
		(*pr)("\tcache layer full groups %u\n", nfull);
		if (print_cache) {
			(*pr)("\tfull cache groups:\n");
			for (pcg = pc->pc_fullgroups; pcg != NULL;
			    pcg = pcg->pcg_next) {
				PR_GROUPLIST(pcg);
			}
		}
	}
#undef PR_GROUPLIST
}

static int
pool_chk_page(struct pool *pp, const char *label, struct pool_item_header *ph)
{
	struct pool_item *pi;
	void *page;
	int n;

	if ((pp->pr_roflags & PR_NOALIGN) == 0) {
		page = POOL_OBJ_TO_PAGE(pp, ph);
		if (page != ph->ph_page &&
		    (pp->pr_roflags & PR_PHINPAGE) != 0) {
			if (label != NULL)
				printf("%s: ", label);
			printf("pool(%p:%s): page inconsistency: page %p;"
			       " at page head addr %p (p %p)\n", pp,
				pp->pr_wchan, ph->ph_page,
				ph, page);
			return 1;
		}
	}

	if ((pp->pr_roflags & PR_USEBMAP) != 0)
		return 0;

	for (pi = LIST_FIRST(&ph->ph_itemlist), n = 0;
	     pi != NULL;
	     pi = LIST_NEXT(pi,pi_list), n++) {

#ifdef POOL_CHECK_MAGIC
		if (pi->pi_magic != PI_MAGIC) {
			if (label != NULL)
				printf("%s: ", label);
			printf("pool(%s): free list modified: magic=%x;"
			       " page %p; item ordinal %d; addr %p\n",
				pp->pr_wchan, pi->pi_magic, ph->ph_page,
				n, pi);
			panic("pool");
		}
#endif
		if ((pp->pr_roflags & PR_NOALIGN) != 0) {
			continue;
		}
		page = POOL_OBJ_TO_PAGE(pp, pi);
		if (page == ph->ph_page)
			continue;

		if (label != NULL)
			printf("%s: ", label);
		printf("pool(%p:%s): page inconsistency: page %p;"
		       " item ordinal %d; addr %p (p %p)\n", pp,
			pp->pr_wchan, ph->ph_page,
			n, pi, page);
		return 1;
	}
	return 0;
}


int
pool_chk(struct pool *pp, const char *label)
{
	struct pool_item_header *ph;
	int r = 0;

	mutex_enter(&pp->pr_lock);
	LIST_FOREACH(ph, &pp->pr_emptypages, ph_pagelist) {
		r = pool_chk_page(pp, label, ph);
		if (r) {
			goto out;
		}
	}
	LIST_FOREACH(ph, &pp->pr_fullpages, ph_pagelist) {
		r = pool_chk_page(pp, label, ph);
		if (r) {
			goto out;
		}
	}
	LIST_FOREACH(ph, &pp->pr_partpages, ph_pagelist) {
		r = pool_chk_page(pp, label, ph);
		if (r) {
			goto out;
		}
	}

out:
	mutex_exit(&pp->pr_lock);
	return r;
}

/*
 * pool_cache_init:
 *
 *	Initialize a pool cache.
 */
pool_cache_t
pool_cache_init(size_t size, u_int align, u_int align_offset, u_int flags,
    const char *wchan, struct pool_allocator *palloc, int ipl,
    int (*ctor)(void *, void *, int), void (*dtor)(void *, void *), void *arg)
{
	pool_cache_t pc;

	pc = pool_get(&cache_pool, PR_WAITOK);
	if (pc == NULL)
		return NULL;

	pool_cache_bootstrap(pc, size, align, align_offset, flags, wchan,
	   palloc, ipl, ctor, dtor, arg);

	return pc;
}

/*
 * pool_cache_bootstrap:
 *
 *	Kernel-private version of pool_cache_init().  The caller
 *	provides initial storage.
 */
void
pool_cache_bootstrap(pool_cache_t pc, size_t size, u_int align,
    u_int align_offset, u_int flags, const char *wchan,
    struct pool_allocator *palloc, int ipl,
    int (*ctor)(void *, void *, int), void (*dtor)(void *, void *),
    void *arg)
{
	CPU_INFO_ITERATOR cii;
	pool_cache_t pc1;
	struct cpu_info *ci;
	struct pool *pp;
	unsigned int ppflags;

	pp = &pc->pc_pool;
	if (palloc == NULL && ipl == IPL_NONE) {
		if (size > PAGE_SIZE) {
			int bigidx = pool_bigidx(size);

			palloc = &pool_allocator_big[bigidx];
			flags |= PR_NOALIGN;
		} else
			palloc = &pool_allocator_nointr;
	}

	ppflags = flags;
	if (ctor == NULL) {
		ctor = NO_CTOR;
	}
	if (dtor == NULL) {
		dtor = NO_DTOR;
	} else {
		/*
		 * If we have a destructor, then the pool layer does not
		 * need to worry about PR_PSERIALIZE.
		 */
		ppflags &= ~PR_PSERIALIZE;
	}

	pool_init(pp, size, align, align_offset, ppflags, wchan, palloc, ipl);

	pc->pc_fullgroups = NULL;
	pc->pc_partgroups = NULL;
	pc->pc_ctor = ctor;
	pc->pc_dtor = dtor;
	pc->pc_arg  = arg;
	pc->pc_refcnt = 0;
	pc->pc_roflags = flags;
	pc->pc_freecheck = NULL;

	if ((flags & PR_LARGECACHE) != 0) {
		pc->pc_pcgsize = PCG_NOBJECTS_LARGE;
		pc->pc_pcgpool = &pcg_large_pool;
		pc->pc_pcgcache = &pcg_large_cache;
	} else {
		pc->pc_pcgsize = PCG_NOBJECTS_NORMAL;
		pc->pc_pcgpool = &pcg_normal_pool;
		pc->pc_pcgcache = &pcg_normal_cache;
	}

	/* Allocate per-CPU caches. */
	memset(pc->pc_cpus, 0, sizeof(pc->pc_cpus));
	pc->pc_ncpu = 0;
	if (ncpu < 2) {
		/* XXX For sparc: boot CPU is not attached yet. */
		pool_cache_cpu_init1(curcpu(), pc);
	} else {
		for (CPU_INFO_FOREACH(cii, ci)) {
			pool_cache_cpu_init1(ci, pc);
		}
	}

	/* Add to list of all pools. */
	if (__predict_true(!cold))
		mutex_enter(&pool_head_lock);
	TAILQ_FOREACH(pc1, &pool_cache_head, pc_cachelist) {
		if (strcmp(pc1->pc_pool.pr_wchan, pc->pc_pool.pr_wchan) > 0)
			break;
	}
	if (pc1 == NULL)
		TAILQ_INSERT_TAIL(&pool_cache_head, pc, pc_cachelist);
	else
		TAILQ_INSERT_BEFORE(pc1, pc, pc_cachelist);
	if (__predict_true(!cold))
		mutex_exit(&pool_head_lock);

	atomic_store_release(&pp->pr_cache, pc);
}

/*
 * pool_cache_destroy:
 *
 *	Destroy a pool cache.
 */
void
pool_cache_destroy(pool_cache_t pc)
{

	pool_cache_bootstrap_destroy(pc);
	pool_put(&cache_pool, pc);
}

/*
 * pool_cache_bootstrap_destroy:
 *
 *	Destroy a pool cache.
 */
void
pool_cache_bootstrap_destroy(pool_cache_t pc)
{
	struct pool *pp = &pc->pc_pool;
	u_int i;

	/* Remove it from the global list. */
	mutex_enter(&pool_head_lock);
	while (pc->pc_refcnt != 0)
		cv_wait(&pool_busy, &pool_head_lock);
	TAILQ_REMOVE(&pool_cache_head, pc, pc_cachelist);
	mutex_exit(&pool_head_lock);

	/* First, invalidate the entire cache. */
	pool_cache_invalidate(pc);

	/* Disassociate it from the pool. */
	mutex_enter(&pp->pr_lock);
	atomic_store_relaxed(&pp->pr_cache, NULL);
	mutex_exit(&pp->pr_lock);

	/* Destroy per-CPU data */
	for (i = 0; i < __arraycount(pc->pc_cpus); i++)
		pool_cache_invalidate_cpu(pc, i);

	/* Finally, destroy it. */
	pool_destroy(pp);
}

/*
 * pool_cache_cpu_init1:
 *
 *	Called for each pool_cache whenever a new CPU is attached.
 */
static void
pool_cache_cpu_init1(struct cpu_info *ci, pool_cache_t pc)
{
	pool_cache_cpu_t *cc;
	int index;

	index = ci->ci_index;

	KASSERT(index < __arraycount(pc->pc_cpus));

	if ((cc = pc->pc_cpus[index]) != NULL) {
		return;
	}

	/*
	 * The first CPU is 'free'.  This needs to be the case for
	 * bootstrap - we may not be able to allocate yet.
	 */
	if (pc->pc_ncpu == 0) {
		cc = &pc->pc_cpu0;
		pc->pc_ncpu = 1;
	} else {
		pc->pc_ncpu++;
		cc = pool_get(&cache_cpu_pool, PR_WAITOK);
	}

	cc->cc_current = __UNCONST(&pcg_dummy);
	cc->cc_previous = __UNCONST(&pcg_dummy);
	cc->cc_pcgcache = pc->pc_pcgcache;
	cc->cc_hits = 0;
	cc->cc_misses = 0;
	cc->cc_pcmisses = 0;
	cc->cc_contended = 0;
	cc->cc_nfull = 0;
	cc->cc_npart = 0;

	pc->pc_cpus[index] = cc;
}

/*
 * pool_cache_cpu_init:
 *
 *	Called whenever a new CPU is attached.
 */
void
pool_cache_cpu_init(struct cpu_info *ci)
{
	pool_cache_t pc;

	mutex_enter(&pool_head_lock);
	TAILQ_FOREACH(pc, &pool_cache_head, pc_cachelist) {
		pc->pc_refcnt++;
		mutex_exit(&pool_head_lock);

		pool_cache_cpu_init1(ci, pc);

		mutex_enter(&pool_head_lock);
		pc->pc_refcnt--;
		cv_broadcast(&pool_busy);
	}
	mutex_exit(&pool_head_lock);
}

/*
 * pool_cache_reclaim:
 *
 *	Reclaim memory from a pool cache.
 */
bool
pool_cache_reclaim(pool_cache_t pc)
{

	return pool_reclaim(&pc->pc_pool);
}

static inline void
pool_cache_pre_destruct(pool_cache_t pc)
{
	/*
	 * Perform a passive serialization barrier before destructing
	 * a batch of one or more objects.
	 */
	if (__predict_false(pc_has_pser(pc))) {
		pool_barrier();
	}
}

static void
pool_cache_destruct_object1(pool_cache_t pc, void *object)
{
	(*pc->pc_dtor)(pc->pc_arg, object);
	pool_put(&pc->pc_pool, object);
}

/*
 * pool_cache_destruct_object:
 *
 *	Force destruction of an object and its release back into
 *	the pool.
 */
void
pool_cache_destruct_object(pool_cache_t pc, void *object)
{

	FREECHECK_IN(&pc->pc_freecheck, object);

	pool_cache_pre_destruct(pc);
	pool_cache_destruct_object1(pc, object);
}

/*
 * pool_cache_invalidate_groups:
 *
 *	Invalidate a chain of groups and destruct all objects.  Return the
 *	number of groups that were invalidated.
 */
static int
pool_cache_invalidate_groups(pool_cache_t pc, pcg_t *pcg)
{
	void *object;
	pcg_t *next;
	int i, n;

	if (pcg == NULL) {
		return 0;
	}

	pool_cache_pre_destruct(pc);

	for (n = 0; pcg != NULL; pcg = next, n++) {
		next = pcg->pcg_next;

		for (i = 0; i < pcg->pcg_avail; i++) {
			object = pcg->pcg_objects[i].pcgo_va;
			pool_cache_destruct_object1(pc, object);
		}

		if (pcg->pcg_size == PCG_NOBJECTS_LARGE) {
			pool_put(&pcg_large_pool, pcg);
		} else {
			KASSERT(pcg->pcg_size == PCG_NOBJECTS_NORMAL);
			pool_put(&pcg_normal_pool, pcg);
		}
	}
	return n;
}

/*
 * pool_cache_invalidate:
 *
 *	Invalidate a pool cache (destruct and release all of the
 *	cached objects).  Does not reclaim objects from the pool.
 *
 *	Note: For pool caches that provide constructed objects, there
 *	is an assumption that another level of synchronization is occurring
 *	between the input to the constructor and the cache invalidation.
 *
 *	Invalidation is a costly process and should not be called from
 *	interrupt context.
 */
void
pool_cache_invalidate(pool_cache_t pc)
{
	uint64_t where;
	pcg_t *pcg;
	int n, s;

	KASSERT(!cpu_intr_p() && !cpu_softintr_p());

	if (ncpu < 2 || !mp_online) {
		/*
		 * We might be called early enough in the boot process
		 * for the CPU data structures to not be fully initialized.
		 * In this case, transfer the content of the local CPU's
		 * cache back into global cache as only this CPU is currently
		 * running.
		 */
		pool_cache_transfer(pc);
	} else {
		/*
		 * Signal all CPUs that they must transfer their local
		 * cache back to the global pool then wait for the xcall to
		 * complete.
		 */
		where = xc_broadcast(0,
		    __FPTRCAST(xcfunc_t, pool_cache_transfer), pc, NULL);
		xc_wait(where);
	}

	/* Now dequeue and invalidate everything. */
	pcg = pool_pcg_trunc(&pcg_normal_cache);
	(void)pool_cache_invalidate_groups(pc, pcg);

	pcg = pool_pcg_trunc(&pcg_large_cache);
	(void)pool_cache_invalidate_groups(pc, pcg);

	pcg = pool_pcg_trunc(&pc->pc_fullgroups);
	n = pool_cache_invalidate_groups(pc, pcg);
	s = splvm();
	((pool_cache_cpu_t *)pc->pc_cpus[curcpu()->ci_index])->cc_nfull -= n;
	splx(s);

	pcg = pool_pcg_trunc(&pc->pc_partgroups);
	n = pool_cache_invalidate_groups(pc, pcg);
	s = splvm();
	((pool_cache_cpu_t *)pc->pc_cpus[curcpu()->ci_index])->cc_npart -= n;
	splx(s);
}

/*
 * pool_cache_invalidate_cpu:
 *
 *	Invalidate all CPU-bound cached objects in pool cache, the CPU being
 *	identified by its associated index.
 *	It is caller's responsibility to ensure that no operation is
 *	taking place on this pool cache while doing this invalidation.
 *	WARNING: as no inter-CPU locking is enforced, trying to invalidate
 *	pool cached objects from a CPU different from the one currently running
 *	may result in an undefined behaviour.
 */
static void
pool_cache_invalidate_cpu(pool_cache_t pc, u_int index)
{
	pool_cache_cpu_t *cc;
	pcg_t *pcg;

	if ((cc = pc->pc_cpus[index]) == NULL)
		return;

	if ((pcg = cc->cc_current) != &pcg_dummy) {
		pcg->pcg_next = NULL;
		pool_cache_invalidate_groups(pc, pcg);
	}
	if ((pcg = cc->cc_previous) != &pcg_dummy) {
		pcg->pcg_next = NULL;
		pool_cache_invalidate_groups(pc, pcg);
	}
	if (cc != &pc->pc_cpu0)
		pool_put(&cache_cpu_pool, cc);

}

void
pool_cache_set_drain_hook(pool_cache_t pc, void (*fn)(void *, int), void *arg)
{

	pool_set_drain_hook(&pc->pc_pool, fn, arg);
}

void
pool_cache_setlowat(pool_cache_t pc, int n)
{

	pool_setlowat(&pc->pc_pool, n);
}

void
pool_cache_sethiwat(pool_cache_t pc, int n)
{

	pool_sethiwat(&pc->pc_pool, n);
}

void
pool_cache_sethardlimit(pool_cache_t pc, int n, const char *warnmess, int ratecap)
{

	pool_sethardlimit(&pc->pc_pool, n, warnmess, ratecap);
}

void
pool_cache_prime(pool_cache_t pc, int n)
{

	pool_prime(&pc->pc_pool, n);
}

unsigned int
pool_cache_nget(pool_cache_t pc)
{

	return pool_nget(&pc->pc_pool);
}

unsigned int
pool_cache_nput(pool_cache_t pc)
{

	return pool_nput(&pc->pc_pool);
}

/*
 * pool_pcg_get:
 *
 *	Get a cache group from the specified list.  Return true if
 *	contention was encountered.  Must be called at IPL_VM because
 *	of spin wait vs. kernel_lock.
 */
static int
pool_pcg_get(pcg_t *volatile *head, pcg_t **pcgp)
{
	int count = SPINLOCK_BACKOFF_MIN;
	pcg_t *o, *n;

	for (o = atomic_load_relaxed(head);; o = n) {
		if (__predict_false(o == &pcg_dummy)) {
			/* Wait for concurrent get to complete. */
			SPINLOCK_BACKOFF(count);
			n = atomic_load_relaxed(head);
			continue;
		}
		if (__predict_false(o == NULL)) {
			break;
		}
		/* Lock out concurrent get/put. */
		n = atomic_cas_ptr(head, o, __UNCONST(&pcg_dummy));
		if (o == n) {
			/* Fetch pointer to next item and then unlock. */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
			membar_datadep_consumer(); /* alpha */
#endif
			n = atomic_load_relaxed(&o->pcg_next);
			atomic_store_release(head, n);
			break;
		}
	}
	*pcgp = o;
	return count != SPINLOCK_BACKOFF_MIN;
}

/*
 * pool_pcg_trunc:
 *
 *	Chop out entire list of pool cache groups.
 */
static pcg_t *
pool_pcg_trunc(pcg_t *volatile *head)
{
	int count = SPINLOCK_BACKOFF_MIN, s;
	pcg_t *o, *n;

	s = splvm();
	for (o = atomic_load_relaxed(head);; o = n) {
		if (__predict_false(o == &pcg_dummy)) {
			/* Wait for concurrent get to complete. */
			SPINLOCK_BACKOFF(count);
			n = atomic_load_relaxed(head);
			continue;
		}
		n = atomic_cas_ptr(head, o, NULL);
		if (o == n) {
			splx(s);
#ifndef __HAVE_ATOMIC_AS_MEMBAR
			membar_datadep_consumer(); /* alpha */
#endif
			return o;
		}
	}
}

/*
 * pool_pcg_put:
 *
 *	Put a pool cache group to the specified list.  Return true if
 *	contention was encountered.  Must be called at IPL_VM because of
 *	spin wait vs. kernel_lock.
 */
static int
pool_pcg_put(pcg_t *volatile *head, pcg_t *pcg)
{
	int count = SPINLOCK_BACKOFF_MIN;
	pcg_t *o, *n;

	for (o = atomic_load_relaxed(head);; o = n) {
		if (__predict_false(o == &pcg_dummy)) {
			/* Wait for concurrent get to complete. */
			SPINLOCK_BACKOFF(count);
			n = atomic_load_relaxed(head);
			continue;
		}
		pcg->pcg_next = o;
#ifndef __HAVE_ATOMIC_AS_MEMBAR
		membar_release();
#endif
		n = atomic_cas_ptr(head, o, pcg);
		if (o == n) {
			return count != SPINLOCK_BACKOFF_MIN;
		}
	}
}

static bool __noinline
pool_cache_get_slow(pool_cache_t pc, pool_cache_cpu_t *cc, int s,
    void **objectp, paddr_t *pap, int flags)
{
	pcg_t *pcg, *cur;
	void *object;

	KASSERT(cc->cc_current->pcg_avail == 0);
	KASSERT(cc->cc_previous->pcg_avail == 0);

	cc->cc_misses++;

	/*
	 * If there's a full group, release our empty group back to the
	 * cache.  Install the full group as cc_current and return.
	 */
	cc->cc_contended += pool_pcg_get(&pc->pc_fullgroups, &pcg);
	if (__predict_true(pcg != NULL)) {
		KASSERT(pcg->pcg_avail == pcg->pcg_size);
		if (__predict_true((cur = cc->cc_current) != &pcg_dummy)) {
			KASSERT(cur->pcg_avail == 0);
			(void)pool_pcg_put(cc->cc_pcgcache, cur);
		}
		cc->cc_nfull--;
		cc->cc_current = pcg;
		return true;
	}

	/*
	 * Nothing available locally or in cache.  Take the slow
	 * path: fetch a new object from the pool and construct
	 * it.
	 */
	cc->cc_pcmisses++;
	splx(s);

	object = pool_get(&pc->pc_pool, flags);
	*objectp = object;
	if (__predict_false(object == NULL)) {
		KASSERT((flags & (PR_NOWAIT|PR_LIMITFAIL)) != 0);
		return false;
	}

	if (__predict_false((*pc->pc_ctor)(pc->pc_arg, object, flags) != 0)) {
		pool_put(&pc->pc_pool, object);
		*objectp = NULL;
		return false;
	}

	KASSERT((((vaddr_t)object) & (pc->pc_pool.pr_align - 1)) == 0);

	if (pap != NULL) {
#ifdef POOL_VTOPHYS
		*pap = POOL_VTOPHYS(object);
#else
		*pap = POOL_PADDR_INVALID;
#endif
	}

	FREECHECK_OUT(&pc->pc_freecheck, object);
	return false;
}

/*
 * pool_cache_get{,_paddr}:
 *
 *	Get an object from a pool cache (optionally returning
 *	the physical address of the object).
 */
void *
pool_cache_get_paddr(pool_cache_t pc, int flags, paddr_t *pap)
{
	pool_cache_cpu_t *cc;
	pcg_t *pcg;
	void *object;
	int s;

	KASSERT(!(flags & PR_NOWAIT) != !(flags & PR_WAITOK));
	KASSERTMSG((!cpu_intr_p() && !cpu_softintr_p()) ||
	    (pc->pc_pool.pr_ipl != IPL_NONE || cold || panicstr != NULL),
	    "%s: [%s] is IPL_NONE, but called from interrupt context",
	    __func__, pc->pc_pool.pr_wchan);

	if (flags & PR_WAITOK) {
		ASSERT_SLEEPABLE();
	}

	if (flags & PR_NOWAIT) {
		if (fault_inject())
			return NULL;
	}

	/* Lock out interrupts and disable preemption. */
	s = splvm();
	while (/* CONSTCOND */ true) {
		/* Try and allocate an object from the current group. */
		cc = pc->pc_cpus[curcpu()->ci_index];
	 	pcg = cc->cc_current;
		if (__predict_true(pcg->pcg_avail > 0)) {
			object = pcg->pcg_objects[--pcg->pcg_avail].pcgo_va;
			if (__predict_false(pap != NULL))
				*pap = pcg->pcg_objects[pcg->pcg_avail].pcgo_pa;
#if defined(DIAGNOSTIC)
			pcg->pcg_objects[pcg->pcg_avail].pcgo_va = NULL;
			KASSERT(pcg->pcg_avail < pcg->pcg_size);
			KASSERT(object != NULL);
#endif
			cc->cc_hits++;
			splx(s);
			FREECHECK_OUT(&pc->pc_freecheck, object);
			pool_redzone_fill(&pc->pc_pool, object);
			pool_cache_get_kmsan(pc, object);
			return object;
		}

		/*
		 * That failed.  If the previous group isn't empty, swap
		 * it with the current group and allocate from there.
		 */
		pcg = cc->cc_previous;
		if (__predict_true(pcg->pcg_avail > 0)) {
			cc->cc_previous = cc->cc_current;
			cc->cc_current = pcg;
			continue;
		}

		/*
		 * Can't allocate from either group: try the slow path.
		 * If get_slow() allocated an object for us, or if
		 * no more objects are available, it will return false.
		 * Otherwise, we need to retry.
		 */
		if (!pool_cache_get_slow(pc, cc, s, &object, pap, flags)) {
			if (object != NULL) {
				kmsan_orig(object, pc->pc_pool.pr_size,
				    KMSAN_TYPE_POOL, __RET_ADDR);
			}
			break;
		}
	}

	/*
	 * We would like to KASSERT(object || (flags & PR_NOWAIT)), but
	 * pool_cache_get can fail even in the PR_WAITOK case, if the
	 * constructor fails.
	 */
	return object;
}

static bool __noinline
pool_cache_put_slow(pool_cache_t pc, pool_cache_cpu_t *cc, int s, void *object)
{
	pcg_t *pcg, *cur;

	KASSERT(cc->cc_current->pcg_avail == cc->cc_current->pcg_size);
	KASSERT(cc->cc_previous->pcg_avail == cc->cc_previous->pcg_size);

	cc->cc_misses++;

	/*
	 * Try to get an empty group from the cache.  If there are no empty
	 * groups in the cache then allocate one.
	 */
	(void)pool_pcg_get(cc->cc_pcgcache, &pcg);
	if (__predict_false(pcg == NULL)) {
		if (__predict_true(!pool_cache_disable)) {
			pcg = pool_get(pc->pc_pcgpool, PR_NOWAIT);
		}
		if (__predict_true(pcg != NULL)) {
			pcg->pcg_avail = 0;
			pcg->pcg_size = pc->pc_pcgsize;
		}
	}

	/*
	 * If there's a empty group, release our full group back to the
	 * cache.  Install the empty group to the local CPU and return.
	 */
	if (pcg != NULL) {
		KASSERT(pcg->pcg_avail == 0);
		if (__predict_false(cc->cc_previous == &pcg_dummy)) {
			cc->cc_previous = pcg;
		} else {
			cur = cc->cc_current;
			if (__predict_true(cur != &pcg_dummy)) {
				KASSERT(cur->pcg_avail == cur->pcg_size);
				cc->cc_contended +=
				    pool_pcg_put(&pc->pc_fullgroups, cur);
				cc->cc_nfull++;
			}
			cc->cc_current = pcg;
		}
		return true;
	}

	/*
	 * Nothing available locally or in cache, and we didn't
	 * allocate an empty group.  Take the slow path and destroy
	 * the object here and now.
	 */
	cc->cc_pcmisses++;
	splx(s);
	pool_cache_destruct_object(pc, object);

	return false;
}

/*
 * pool_cache_put{,_paddr}:
 *
 *	Put an object back to the pool cache (optionally caching the
 *	physical address of the object).
 */
void
pool_cache_put_paddr(pool_cache_t pc, void *object, paddr_t pa)
{
	pool_cache_cpu_t *cc;
	pcg_t *pcg;
	int s;

	KASSERT(object != NULL);
	pool_cache_put_kmsan(pc, object);
	pool_cache_redzone_check(pc, object);
	FREECHECK_IN(&pc->pc_freecheck, object);

	if (pc->pc_pool.pr_roflags & PR_PHINPAGE) {
		pc_phinpage_check(pc, object);
	}

	if (pool_cache_put_nocache(pc, object)) {
		return;
	}

	/* Lock out interrupts and disable preemption. */
	s = splvm();
	while (/* CONSTCOND */ true) {
		/* If the current group isn't full, release it there. */
		cc = pc->pc_cpus[curcpu()->ci_index];
	 	pcg = cc->cc_current;
		if (__predict_true(pcg->pcg_avail < pcg->pcg_size)) {
			pcg->pcg_objects[pcg->pcg_avail].pcgo_va = object;
			pcg->pcg_objects[pcg->pcg_avail].pcgo_pa = pa;
			pcg->pcg_avail++;
			cc->cc_hits++;
			splx(s);
			return;
		}

		/*
		 * That failed.  If the previous group isn't full, swap
		 * it with the current group and try again.
		 */
		pcg = cc->cc_previous;
		if (__predict_true(pcg->pcg_avail < pcg->pcg_size)) {
			cc->cc_previous = cc->cc_current;
			cc->cc_current = pcg;
			continue;
		}

		/*
		 * Can't free to either group: try the slow path.
		 * If put_slow() releases the object for us, it
		 * will return false.  Otherwise we need to retry.
		 */
		if (!pool_cache_put_slow(pc, cc, s, object))
			break;
	}
}

/*
 * pool_cache_transfer:
 *
 *	Transfer objects from the per-CPU cache to the global cache.
 *	Run within a cross-call thread.
 */
static void
pool_cache_transfer(pool_cache_t pc)
{
	pool_cache_cpu_t *cc;
	pcg_t *prev, *cur;
	int s;

	s = splvm();
	cc = pc->pc_cpus[curcpu()->ci_index];
	cur = cc->cc_current;
	cc->cc_current = __UNCONST(&pcg_dummy);
	prev = cc->cc_previous;
	cc->cc_previous = __UNCONST(&pcg_dummy);
	if (cur != &pcg_dummy) {
		if (cur->pcg_avail == cur->pcg_size) {
			(void)pool_pcg_put(&pc->pc_fullgroups, cur);
			cc->cc_nfull++;
		} else if (cur->pcg_avail == 0) {
			(void)pool_pcg_put(pc->pc_pcgcache, cur);
		} else {
			(void)pool_pcg_put(&pc->pc_partgroups, cur);
			cc->cc_npart++;
		}
	}
	if (prev != &pcg_dummy) {
		if (prev->pcg_avail == prev->pcg_size) {
			(void)pool_pcg_put(&pc->pc_fullgroups, prev);
			cc->cc_nfull++;
		} else if (prev->pcg_avail == 0) {
			(void)pool_pcg_put(pc->pc_pcgcache, prev);
		} else {
			(void)pool_pcg_put(&pc->pc_partgroups, prev);
			cc->cc_npart++;
		}
	}
	splx(s);
}

static int
pool_bigidx(size_t size)
{
	int i;

	for (i = 0; i < __arraycount(pool_allocator_big); i++) {
		if (1 << (i + POOL_ALLOCATOR_BIG_BASE) >= size)
			return i;
	}
	panic("pool item size %zu too large, use a custom allocator", size);
}

static void *
pool_allocator_alloc(struct pool *pp, int flags)
{
	struct pool_allocator *pa = pp->pr_alloc;
	void *res;

	res = (*pa->pa_alloc)(pp, flags);
	if (res == NULL && (flags & PR_WAITOK) == 0) {
		/*
		 * We only run the drain hook here if PR_NOWAIT.
		 * In other cases, the hook will be run in
		 * pool_reclaim().
		 */
		if (pp->pr_drain_hook != NULL) {
			(*pp->pr_drain_hook)(pp->pr_drain_hook_arg, flags);
			res = (*pa->pa_alloc)(pp, flags);
		}
	}
	return res;
}

static void
pool_allocator_free(struct pool *pp, void *v)
{
	struct pool_allocator *pa = pp->pr_alloc;

	if (pp->pr_redzone) {
		KASSERT(!pp_has_pser(pp));
		kasan_mark(v, pa->pa_pagesz, pa->pa_pagesz, 0);
	} else if (__predict_false(pp_has_pser(pp))) {
		/*
		 * Perform a passive serialization barrier before freeing
		 * the pool page back to the system.
		 */
		pool_barrier();
	}
	(*pa->pa_free)(pp, v);
}

void *
pool_page_alloc(struct pool *pp, int flags)
{
	const vm_flag_t vflags = (flags & PR_WAITOK) ? VM_SLEEP: VM_NOSLEEP;
	vmem_addr_t va;
	int ret;

	ret = uvm_km_kmem_alloc(kmem_va_arena, pp->pr_alloc->pa_pagesz,
	    vflags | VM_INSTANTFIT, &va);

	return ret ? NULL : (void *)va;
}

void
pool_page_free(struct pool *pp, void *v)
{

	uvm_km_kmem_free(kmem_va_arena, (vaddr_t)v, pp->pr_alloc->pa_pagesz);
}

static void *
pool_page_alloc_meta(struct pool *pp, int flags)
{
	const vm_flag_t vflags = (flags & PR_WAITOK) ? VM_SLEEP: VM_NOSLEEP;
	vmem_addr_t va;
	int ret;

	ret = vmem_alloc(kmem_meta_arena, pp->pr_alloc->pa_pagesz,
	    vflags | VM_INSTANTFIT, &va);

	return ret ? NULL : (void *)va;
}

static void
pool_page_free_meta(struct pool *pp, void *v)
{

	vmem_free(kmem_meta_arena, (vmem_addr_t)v, pp->pr_alloc->pa_pagesz);
}

#ifdef KMSAN
static inline void
pool_get_kmsan(struct pool *pp, void *p)
{
	kmsan_orig(p, pp->pr_size, KMSAN_TYPE_POOL, __RET_ADDR);
	kmsan_mark(p, pp->pr_size, KMSAN_STATE_UNINIT);
}

static inline void
pool_put_kmsan(struct pool *pp, void *p)
{
	kmsan_mark(p, pp->pr_size, KMSAN_STATE_INITED);
}

static inline void
pool_cache_get_kmsan(pool_cache_t pc, void *p)
{
	if (__predict_false(pc_has_ctor(pc))) {
		return;
	}
	pool_get_kmsan(&pc->pc_pool, p);
}

static inline void
pool_cache_put_kmsan(pool_cache_t pc, void *p)
{
	pool_put_kmsan(&pc->pc_pool, p);
}
#endif

#ifdef POOL_QUARANTINE
static void
pool_quarantine_init(struct pool *pp)
{
	pp->pr_quar.rotor = 0;
	memset(&pp->pr_quar, 0, sizeof(pp->pr_quar));
}

static void
pool_quarantine_flush(struct pool *pp)
{
	pool_quar_t *quar = &pp->pr_quar;
	struct pool_pagelist pq;
	size_t i;

	LIST_INIT(&pq);

	mutex_enter(&pp->pr_lock);
	for (i = 0; i < POOL_QUARANTINE_DEPTH; i++) {
		if (quar->list[i] == 0)
			continue;
		pool_do_put(pp, (void *)quar->list[i], &pq);
	}
	mutex_exit(&pp->pr_lock);

	pr_pagelist_free(pp, &pq);
}

static bool
pool_put_quarantine(struct pool *pp, void *v, struct pool_pagelist *pq)
{
	pool_quar_t *quar = &pp->pr_quar;
	uintptr_t old;

	if (pp->pr_roflags & PR_NOTOUCH) {
		return false;
	}

	pool_redzone_check(pp, v);

	old = quar->list[quar->rotor];
	quar->list[quar->rotor] = (uintptr_t)v;
	quar->rotor = (quar->rotor + 1) % POOL_QUARANTINE_DEPTH;
	if (old != 0) {
		pool_do_put(pp, (void *)old, pq);
	}

	return true;
}
#endif

#ifdef POOL_NOCACHE
static bool
pool_cache_put_nocache(pool_cache_t pc, void *p)
{
	pool_cache_destruct_object(pc, p);
	return true;
}
#endif

#ifdef POOL_REDZONE
#if defined(_LP64)
# define PRIME 0x9e37fffffffc0000UL
#else /* defined(_LP64) */
# define PRIME 0x9e3779b1
#endif /* defined(_LP64) */
#define STATIC_BYTE	0xFE
CTASSERT(POOL_REDZONE_SIZE > 1);

#ifndef KASAN
static inline uint8_t
pool_pattern_generate(const void *p)
{
	return (uint8_t)(((uintptr_t)p) * PRIME
	   >> ((sizeof(uintptr_t) - sizeof(uint8_t))) * CHAR_BIT);
}
#endif

static void
pool_redzone_init(struct pool *pp, size_t requested_size)
{
	size_t redzsz;
	size_t nsz;

#ifdef KASAN
	redzsz = requested_size;
	kasan_add_redzone(&redzsz);
	redzsz -= requested_size;
#else
	redzsz = POOL_REDZONE_SIZE;
#endif

	if (pp->pr_roflags & PR_NOTOUCH) {
		pp->pr_redzone = false;
		return;
	}

	/*
	 * We may have extended the requested size earlier; check if
	 * there's naturally space in the padding for a red zone.
	 */
	if (pp->pr_size - requested_size >= redzsz) {
		pp->pr_reqsize_with_redzone = requested_size + redzsz;
		pp->pr_redzone = true;
		return;
	}

	/*
	 * No space in the natural padding; check if we can extend a
	 * bit the size of the pool.
	 *
	 * Avoid using redzone for allocations half of a page or larger.
	 * For pagesize items, we'd waste a whole new page (could be
	 * unmapped?), and for half pagesize items, approximately half
	 * the space is lost (eg, 4K pages, you get one 2K allocation.)
	 */
	nsz = roundup(pp->pr_size + redzsz, pp->pr_align);
	if (nsz <= (pp->pr_alloc->pa_pagesz / 2)) {
		/* Ok, we can */
		pp->pr_size = nsz;
		pp->pr_reqsize_with_redzone = requested_size + redzsz;
		pp->pr_redzone = true;
	} else {
		/* No space for a red zone... snif :'( */
		pp->pr_redzone = false;
		aprint_debug("pool redzone disabled for '%s'\n", pp->pr_wchan);
	}
}

static void
pool_redzone_fill(struct pool *pp, void *p)
{
	if (!pp->pr_redzone)
		return;
	KASSERT(!pp_has_pser(pp));
#ifdef KASAN
	kasan_mark(p, pp->pr_reqsize, pp->pr_reqsize_with_redzone,
	    KASAN_POOL_REDZONE);
#else
	uint8_t *cp, pat;
	const uint8_t *ep;

	cp = (uint8_t *)p + pp->pr_reqsize;
	ep = cp + POOL_REDZONE_SIZE;

	/*
	 * We really don't want the first byte of the red zone to be '\0';
	 * an off-by-one in a string may not be properly detected.
	 */
	pat = pool_pattern_generate(cp);
	*cp = (pat == '\0') ? STATIC_BYTE: pat;
	cp++;

	while (cp < ep) {
		*cp = pool_pattern_generate(cp);
		cp++;
	}
#endif
}

static void
pool_redzone_check(struct pool *pp, void *p)
{
	if (!pp->pr_redzone)
		return;
	KASSERT(!pp_has_pser(pp));
#ifdef KASAN
	kasan_mark(p, 0, pp->pr_reqsize_with_redzone, KASAN_POOL_FREED);
#else
	uint8_t *cp, pat, expected;
	const uint8_t *ep;

	cp = (uint8_t *)p + pp->pr_reqsize;
	ep = cp + POOL_REDZONE_SIZE;

	pat = pool_pattern_generate(cp);
	expected = (pat == '\0') ? STATIC_BYTE: pat;
	if (__predict_false(*cp != expected)) {
		panic("%s: [%s] 0x%02x != 0x%02x", __func__,
		    pp->pr_wchan, *cp, expected);
	}
	cp++;

	while (cp < ep) {
		expected = pool_pattern_generate(cp);
		if (__predict_false(*cp != expected)) {
			panic("%s: [%s] 0x%02x != 0x%02x", __func__,
			    pp->pr_wchan, *cp, expected);
		}
		cp++;
	}
#endif
}

static void
pool_cache_redzone_check(pool_cache_t pc, void *p)
{
#ifdef KASAN
	/*
	 * If there is a ctor/dtor, or if the cache objects use
	 * passive serialization, leave the data as valid.
	 */
	if (__predict_false(pc_has_ctor(pc) || pc_has_dtor(pc) ||
	    pc_has_pser(pc))) {
		return;
	}
#endif
	pool_redzone_check(&pc->pc_pool, p);
}

#endif /* POOL_REDZONE */

#if defined(DDB)
static bool
pool_in_page(struct pool *pp, struct pool_item_header *ph, uintptr_t addr)
{

	return (uintptr_t)ph->ph_page <= addr &&
	    addr < (uintptr_t)ph->ph_page + pp->pr_alloc->pa_pagesz;
}

static bool
pool_in_item(struct pool *pp, void *item, uintptr_t addr)
{

	return (uintptr_t)item <= addr && addr < (uintptr_t)item + pp->pr_size;
}

static bool
pool_in_cg(struct pool *pp, struct pool_cache_group *pcg, uintptr_t addr)
{
	int i;

	if (pcg == NULL) {
		return false;
	}
	for (i = 0; i < pcg->pcg_avail; i++) {
		if (pool_in_item(pp, pcg->pcg_objects[i].pcgo_va, addr)) {
			return true;
		}
	}
	return false;
}

static bool
pool_allocated(struct pool *pp, struct pool_item_header *ph, uintptr_t addr)
{

	if ((pp->pr_roflags & PR_USEBMAP) != 0) {
		unsigned int idx = pr_item_bitmap_index(pp, ph, (void *)addr);
		pool_item_bitmap_t *bitmap =
		    ph->ph_bitmap + (idx / BITMAP_SIZE);
		pool_item_bitmap_t mask = 1 << (idx & BITMAP_MASK);

		return (*bitmap & mask) == 0;
	} else {
		struct pool_item *pi;

		LIST_FOREACH(pi, &ph->ph_itemlist, pi_list) {
			if (pool_in_item(pp, pi, addr)) {
				return false;
			}
		}
		return true;
	}
}

void
pool_whatis(uintptr_t addr, void (*pr)(const char *, ...))
{
	struct pool *pp;

	TAILQ_FOREACH(pp, &pool_head, pr_poollist) {
		struct pool_item_header *ph;
		struct pool_cache *pc;
		uintptr_t item;
		bool allocated = true;
		bool incache = false;
		bool incpucache = false;
		char cpucachestr[32];

		if ((pp->pr_roflags & PR_PHINPAGE) != 0) {
			LIST_FOREACH(ph, &pp->pr_fullpages, ph_pagelist) {
				if (pool_in_page(pp, ph, addr)) {
					goto found;
				}
			}
			LIST_FOREACH(ph, &pp->pr_partpages, ph_pagelist) {
				if (pool_in_page(pp, ph, addr)) {
					allocated =
					    pool_allocated(pp, ph, addr);
					goto found;
				}
			}
			LIST_FOREACH(ph, &pp->pr_emptypages, ph_pagelist) {
				if (pool_in_page(pp, ph, addr)) {
					allocated = false;
					goto found;
				}
			}
			continue;
		} else {
			ph = pr_find_pagehead_noalign(pp, (void *)addr);
			if (ph == NULL || !pool_in_page(pp, ph, addr)) {
				continue;
			}
			allocated = pool_allocated(pp, ph, addr);
		}
found:
		if (allocated &&
		    (pc = atomic_load_consume(&pp->pr_cache)) != NULL) {
			struct pool_cache_group *pcg;
			int i;

			for (pcg = pc->pc_fullgroups; pcg != NULL;
			    pcg = pcg->pcg_next) {
				if (pool_in_cg(pp, pcg, addr)) {
					incache = true;
					goto print;
				}
			}
			for (i = 0; i < __arraycount(pc->pc_cpus); i++) {
				pool_cache_cpu_t *cc;

				if ((cc = pc->pc_cpus[i]) == NULL) {
					continue;
				}
				if (pool_in_cg(pp, cc->cc_current, addr) ||
				    pool_in_cg(pp, cc->cc_previous, addr)) {
					struct cpu_info *ci =
					    cpu_lookup(i);

					incpucache = true;
					snprintf(cpucachestr,
					    sizeof(cpucachestr),
					    "cached by CPU %u",
					    ci->ci_index);
					goto print;
				}
			}
		}
print:
		item = (uintptr_t)ph->ph_page + ph->ph_off;
		item = item + rounddown(addr - item, pp->pr_size);
		(*pr)("%p is %p+%zu in POOL '%s' (%s)\n",
		    (void *)addr, item, (size_t)(addr - item),
		    pp->pr_wchan,
		    incpucache ? cpucachestr :
		    incache ? "cached" : allocated ? "allocated" : "free");
	}
}
#endif /* defined(DDB) */

static int
pool_sysctl(SYSCTLFN_ARGS)
{
	struct pool_sysctl data;
	struct pool *pp;
	struct pool_cache *pc;
	pool_cache_cpu_t *cc;
	int error;
	size_t i, written;

	if (oldp == NULL) {
		*oldlenp = 0;
		TAILQ_FOREACH(pp, &pool_head, pr_poollist)
			*oldlenp += sizeof(data);
		return 0;
	}

	memset(&data, 0, sizeof(data));
	error = 0;
	written = 0;
	mutex_enter(&pool_head_lock);
	TAILQ_FOREACH(pp, &pool_head, pr_poollist) {
		if (written + sizeof(data) > *oldlenp)
			break;
		pp->pr_refcnt++;
		strlcpy(data.pr_wchan, pp->pr_wchan, sizeof(data.pr_wchan));
		data.pr_pagesize = pp->pr_alloc->pa_pagesz;
		data.pr_flags = pp->pr_roflags | pp->pr_flags;
#define COPY(field) data.field = pp->field
		COPY(pr_size);

		COPY(pr_itemsperpage);
		COPY(pr_nitems);
		COPY(pr_nout);
		COPY(pr_hardlimit);
		COPY(pr_npages);
		COPY(pr_minpages);
		COPY(pr_maxpages);

		COPY(pr_nget);
		COPY(pr_nfail);
		COPY(pr_nput);
		COPY(pr_npagealloc);
		COPY(pr_npagefree);
		COPY(pr_hiwat);
		COPY(pr_nidle);
#undef COPY

		data.pr_cache_nmiss_pcpu = 0;
		data.pr_cache_nhit_pcpu = 0;
		data.pr_cache_nmiss_global = 0;
		data.pr_cache_nempty = 0;
		data.pr_cache_ncontended = 0;
		data.pr_cache_npartial = 0;
		if ((pc = atomic_load_consume(&pp->pr_cache)) != NULL) {
			uint32_t nfull = 0;
			data.pr_cache_meta_size = pc->pc_pcgsize;
			for (i = 0; i < pc->pc_ncpu; ++i) {
				cc = pc->pc_cpus[i];
				if (cc == NULL)
					continue;
				data.pr_cache_ncontended += cc->cc_contended;
				data.pr_cache_nmiss_pcpu += cc->cc_misses;
				data.pr_cache_nhit_pcpu += cc->cc_hits;
				data.pr_cache_nmiss_global += cc->cc_pcmisses;
				nfull += cc->cc_nfull; /* 32-bit rollover! */
				data.pr_cache_npartial += cc->cc_npart;
			}
			data.pr_cache_nfull = nfull;
		} else {
			data.pr_cache_meta_size = 0;
			data.pr_cache_nfull = 0;
		}
		data.pr_cache_nhit_global = data.pr_cache_nmiss_pcpu -
		    data.pr_cache_nmiss_global;

		if (pp->pr_refcnt == UINT_MAX) /* XXX possible? */
			continue;
		mutex_exit(&pool_head_lock);
		error = sysctl_copyout(l, &data, oldp, sizeof(data));
		mutex_enter(&pool_head_lock);
		if (--pp->pr_refcnt == 0)
			cv_broadcast(&pool_busy);
		if (error)
			break;
		written += sizeof(data);
		oldp = (char *)oldp + sizeof(data);
	}
	mutex_exit(&pool_head_lock);

	*oldlenp = written;
	return error;
}

SYSCTL_SETUP(sysctl_pool_setup, "sysctl kern.pool setup")
{
	const struct sysctlnode *rnode = NULL;

	sysctl_createv(clog, 0, NULL, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "pool",
		       SYSCTL_DESCR("Get pool statistics"),
		       pool_sysctl, 0, NULL, 0,
		       CTL_KERN, CTL_CREATE, CTL_EOL);
}
