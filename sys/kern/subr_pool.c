/*	$NetBSD: subr_pool.c,v 1.128.2.1 2007/03/13 16:51:56 ad Exp $	*/

/*-
 * Copyright (c) 1997, 1999, 2000, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg; by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_pool.c,v 1.128.2.1 2007/03/13 16:51:56 ad Exp $");

#include "opt_pool.h"
#include "opt_poollog.h"
#include "opt_lockdebug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <sys/pool.h>
#include <sys/syslog.h>
#include <sys/debug.h>

#include <uvm/uvm.h>

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

/* List of all pools */
LIST_HEAD(,pool) pool_head = LIST_HEAD_INITIALIZER(pool_head);

/* Private pool for page header structures */
#define	PHPOOL_MAX	8
static struct pool phpool[PHPOOL_MAX];
#define	PHPOOL_FREELIST_NELEM(idx)	(((idx) == 0) ? 0 : (1 << (idx)))

#ifdef POOL_SUBPAGE
/* Pool of subpages for use by normal pools. */
static struct pool psppool;
#endif

static SLIST_HEAD(, pool_allocator) pa_deferinitq =
    SLIST_HEAD_INITIALIZER(pa_deferinitq);

static void *pool_page_alloc_meta(struct pool *, int);
static void pool_page_free_meta(struct pool *, void *);

/* allocator for pool metadata */
static struct pool_allocator pool_allocator_meta = {
	pool_page_alloc_meta, pool_page_free_meta,
	.pa_backingmapptr = &kmem_map,
};

/* # of seconds to retain page after last use */
int pool_inactive_time = 10;

/* Next candidate for drainage (see pool_drain()) */
static struct pool	*drainpp;

/* This spin lock protects both pool_head and drainpp. */
struct simplelock pool_head_slock = SIMPLELOCK_INITIALIZER;

typedef uint8_t pool_item_freelist_t;

struct pool_item_header {
	/* Page headers */
	LIST_ENTRY(pool_item_header)
				ph_pagelist;	/* pool page list */
	SPLAY_ENTRY(pool_item_header)
				ph_node;	/* Off-page page headers */
	void *			ph_page;	/* this page's address */
	struct timeval		ph_time;	/* last referenced */
	union {
		/* !PR_NOTOUCH */
		struct {
			LIST_HEAD(, pool_item)
				phu_itemlist;	/* chunk list for this page */
		} phu_normal;
		/* PR_NOTOUCH */
		struct {
			uint16_t
				phu_off;	/* start offset in page */
			pool_item_freelist_t
				phu_firstfree;	/* first free item */
			/*
			 * XXX it might be better to use
			 * a simple bitmap and ffs(3)
			 */
		} phu_notouch;
	} ph_u;
	uint16_t		ph_nmissing;	/* # of chunks in use */
};
#define	ph_itemlist	ph_u.phu_normal.phu_itemlist
#define	ph_off		ph_u.phu_notouch.phu_off
#define	ph_firstfree	ph_u.phu_notouch.phu_firstfree

struct pool_item {
#ifdef DIAGNOSTIC
	u_int pi_magic;
#endif
#define	PI_MAGIC 0xdeadbeefU
	/* Other entries use only this list entry */
	LIST_ENTRY(pool_item)	pi_list;
};

#define	POOL_NEEDS_CATCHUP(pp)						\
	((pp)->pr_nitems < (pp)->pr_minitems)

/*
 * Pool cache management.
 *
 * Pool caches provide a way for constructed objects to be cached by the
 * pool subsystem.  This can lead to performance improvements by avoiding
 * needless object construction/destruction; it is deferred until absolutely
 * necessary.
 *
 * Caches are grouped into cache groups.  Each cache group references
 * up to 16 constructed objects.  When a cache allocates an object
 * from the pool, it calls the object's constructor and places it into
 * a cache group.  When a cache group frees an object back to the pool,
 * it first calls the object's destructor.  This allows the object to
 * persist in constructed form while freed to the cache.
 *
 * Multiple caches may exist for each pool.  This allows a single
 * object type to have multiple constructed forms.  The pool references
 * each cache, so that when a pool is drained by the pagedaemon, it can
 * drain each individual cache as well.  Each time a cache is drained,
 * the most idle cache group is freed to the pool in its entirety.
 *
 * Pool caches are layed on top of pools.  By layering them, we can avoid
 * the complexity of cache management for pools which would not benefit
 * from it.
 */

/* The cache group pool. */
static struct pool pcgpool;

static void	pool_cache_reclaim(struct pool_cache *, struct pool_pagelist *,
				   struct pool_cache_grouplist *);
static void	pcg_grouplist_free(struct pool_cache_grouplist *);

static int	pool_catchup(struct pool *);
static void	pool_prime_page(struct pool *, void *,
		    struct pool_item_header *);
static void	pool_update_curpage(struct pool *);

static int	pool_grow(struct pool *, int);
static void	*pool_allocator_alloc(struct pool *, int);
static void	pool_allocator_free(struct pool *, void *);

static void pool_print_pagelist(struct pool *, struct pool_pagelist *,
	void (*)(const char *, ...));
static void pool_print1(struct pool *, const char *,
	void (*)(const char *, ...));

static int pool_chk_page(struct pool *, const char *,
			 struct pool_item_header *);

/*
 * Pool log entry. An array of these is allocated in pool_init().
 */
struct pool_log {
	const char	*pl_file;
	long		pl_line;
	int		pl_action;
#define	PRLOG_GET	1
#define	PRLOG_PUT	2
	void		*pl_addr;
};

#ifdef POOL_DIAGNOSTIC
/* Number of entries in pool log buffers */
#ifndef POOL_LOGSIZE
#define	POOL_LOGSIZE	10
#endif

int pool_logsize = POOL_LOGSIZE;

static inline void
pr_log(struct pool *pp, void *v, int action, const char *file, long line)
{
	int n = pp->pr_curlogentry;
	struct pool_log *pl;

	if ((pp->pr_roflags & PR_LOGGING) == 0)
		return;

	/*
	 * Fill in the current entry. Wrap around and overwrite
	 * the oldest entry if necessary.
	 */
	pl = &pp->pr_log[n];
	pl->pl_file = file;
	pl->pl_line = line;
	pl->pl_action = action;
	pl->pl_addr = v;
	if (++n >= pp->pr_logsize)
		n = 0;
	pp->pr_curlogentry = n;
}

static void
pr_printlog(struct pool *pp, struct pool_item *pi,
    void (*pr)(const char *, ...))
{
	int i = pp->pr_logsize;
	int n = pp->pr_curlogentry;

	if ((pp->pr_roflags & PR_LOGGING) == 0)
		return;

	/*
	 * Print all entries in this pool's log.
	 */
	while (i-- > 0) {
		struct pool_log *pl = &pp->pr_log[n];
		if (pl->pl_action != 0) {
			if (pi == NULL || pi == pl->pl_addr) {
				(*pr)("\tlog entry %d:\n", i);
				(*pr)("\t\taction = %s, addr = %p\n",
				    pl->pl_action == PRLOG_GET ? "get" : "put",
				    pl->pl_addr);
				(*pr)("\t\tfile: %s at line %lu\n",
				    pl->pl_file, pl->pl_line);
			}
		}
		if (++n >= pp->pr_logsize)
			n = 0;
	}
}

static inline void
pr_enter(struct pool *pp, const char *file, long line)
{

	if (__predict_false(pp->pr_entered_file != NULL)) {
		printf("pool %s: reentrancy at file %s line %ld\n",
		    pp->pr_wchan, file, line);
		printf("         previous entry at file %s line %ld\n",
		    pp->pr_entered_file, pp->pr_entered_line);
		panic("pr_enter");
	}

	pp->pr_entered_file = file;
	pp->pr_entered_line = line;
}

static inline void
pr_leave(struct pool *pp)
{

	if (__predict_false(pp->pr_entered_file == NULL)) {
		printf("pool %s not entered?\n", pp->pr_wchan);
		panic("pr_leave");
	}

	pp->pr_entered_file = NULL;
	pp->pr_entered_line = 0;
}

static inline void
pr_enter_check(struct pool *pp, void (*pr)(const char *, ...))
{

	if (pp->pr_entered_file != NULL)
		(*pr)("\n\tcurrently entered from file %s line %ld\n",
		    pp->pr_entered_file, pp->pr_entered_line);
}
#else
#define	pr_log(pp, v, action, file, line)
#define	pr_printlog(pp, pi, pr)
#define	pr_enter(pp, file, line)
#define	pr_leave(pp)
#define	pr_enter_check(pp, pr)
#endif /* POOL_DIAGNOSTIC */

static inline int
pr_item_notouch_index(const struct pool *pp, const struct pool_item_header *ph,
    const void *v)
{
	const char *cp = v;
	int idx;

	KASSERT(pp->pr_roflags & PR_NOTOUCH);
	idx = (cp - (char *)ph->ph_page - ph->ph_off) / pp->pr_size;
	KASSERT(idx < pp->pr_itemsperpage);
	return idx;
}

#define	PR_FREELIST_ALIGN(p) \
	roundup((uintptr_t)(p), sizeof(pool_item_freelist_t))
#define	PR_FREELIST(ph)	((pool_item_freelist_t *)PR_FREELIST_ALIGN((ph) + 1))
#define	PR_INDEX_USED	((pool_item_freelist_t)-1)
#define	PR_INDEX_EOL	((pool_item_freelist_t)-2)

static inline void
pr_item_notouch_put(const struct pool *pp, struct pool_item_header *ph,
    void *obj)
{
	int idx = pr_item_notouch_index(pp, ph, obj);
	pool_item_freelist_t *freelist = PR_FREELIST(ph);

	KASSERT(freelist[idx] == PR_INDEX_USED);
	freelist[idx] = ph->ph_firstfree;
	ph->ph_firstfree = idx;
}

static inline void *
pr_item_notouch_get(const struct pool *pp, struct pool_item_header *ph)
{
	int idx = ph->ph_firstfree;
	pool_item_freelist_t *freelist = PR_FREELIST(ph);

	KASSERT(freelist[idx] != PR_INDEX_USED);
	ph->ph_firstfree = freelist[idx];
	freelist[idx] = PR_INDEX_USED;

	return (char *)ph->ph_page + ph->ph_off + idx * pp->pr_size;
}

static inline int
phtree_compare(struct pool_item_header *a, struct pool_item_header *b)
{

	/*
	 * we consider pool_item_header with smaller ph_page bigger.
	 * (this unnatural ordering is for the benefit of pr_find_pagehead.)
	 */

	if (a->ph_page < b->ph_page)
		return (1);
	else if (a->ph_page > b->ph_page)
		return (-1);
	else
		return (0);
}

SPLAY_PROTOTYPE(phtree, pool_item_header, ph_node, phtree_compare);
SPLAY_GENERATE(phtree, pool_item_header, ph_node, phtree_compare);

/*
 * Return the pool page header based on item address.
 */
static inline struct pool_item_header *
pr_find_pagehead(struct pool *pp, void *v)
{
	struct pool_item_header *ph, tmp;

	if ((pp->pr_roflags & PR_NOALIGN) != 0) {
		tmp.ph_page = (void *)(uintptr_t)v;
		ph = SPLAY_FIND(phtree, &pp->pr_phtree, &tmp);
		if (ph == NULL) {
			ph = SPLAY_ROOT(&pp->pr_phtree);
			if (ph != NULL && phtree_compare(&tmp, ph) >= 0) {
				ph = SPLAY_NEXT(phtree, &pp->pr_phtree, ph);
			}
			KASSERT(ph == NULL || phtree_compare(&tmp, ph) < 0);
		}
	} else {
		void *page =
		    (void *)((uintptr_t)v & pp->pr_alloc->pa_pagemask);

		if ((pp->pr_roflags & PR_PHINPAGE) != 0) {
			ph = (struct pool_item_header *)((char *)page + pp->pr_phoffset);
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
	int s;

	while ((ph = LIST_FIRST(pq)) != NULL) {
		LIST_REMOVE(ph, ph_pagelist);
		pool_allocator_free(pp, ph->ph_page);
		if ((pp->pr_roflags & PR_PHINPAGE) == 0) {
			s = splvm();
			pool_put(pp->pr_phpool, ph);
			splx(s);
		}
	}
}

/*
 * Remove a page from the pool.
 */
static inline void
pr_rmpage(struct pool *pp, struct pool_item_header *ph,
     struct pool_pagelist *pq)
{

	LOCK_ASSERT(simple_lock_held(&pp->pr_slock));

	/*
	 * If the page was idle, decrement the idle page count.
	 */
	if (ph->ph_nmissing == 0) {
#ifdef DIAGNOSTIC
		if (pp->pr_nidle == 0)
			panic("pr_rmpage: nidle inconsistent");
		if (pp->pr_nitems < pp->pr_itemsperpage)
			panic("pr_rmpage: nitems inconsistent");
#endif
		pp->pr_nidle--;
	}

	pp->pr_nitems -= pp->pr_itemsperpage;

	/*
	 * Unlink the page from the pool and queue it for release.
	 */
	LIST_REMOVE(ph, ph_pagelist);
	if ((pp->pr_roflags & PR_PHINPAGE) == 0)
		SPLAY_REMOVE(phtree, &pp->pr_phtree, ph);
	LIST_INSERT_HEAD(pq, ph, ph_pagelist);

	pp->pr_npages--;
	pp->pr_npagefree++;

	pool_update_curpage(pp);
}

static bool
pa_starved_p(struct pool_allocator *pa)
{

	if (pa->pa_backingmap != NULL) {
		return vm_map_starved_p(pa->pa_backingmap);
	}
	return false;
}

static int
pool_reclaim_callback(struct callback_entry *ce, void *obj, void *arg)
{
	struct pool *pp = obj;
	struct pool_allocator *pa = pp->pr_alloc;

	KASSERT(&pp->pr_reclaimerentry == ce);
	pool_reclaim(pp);
	if (!pa_starved_p(pa)) {
		return CALLBACK_CHAIN_ABORT;
	}
	return CALLBACK_CHAIN_CONTINUE;
}

static void
pool_reclaim_register(struct pool *pp)
{
	struct vm_map *map = pp->pr_alloc->pa_backingmap;
	int s;

	if (map == NULL) {
		return;
	}

	s = splvm(); /* not necessary for INTRSAFE maps, but don't care. */
	callback_register(&vm_map_to_kernel(map)->vmk_reclaim_callback,
	    &pp->pr_reclaimerentry, pp, pool_reclaim_callback);
	splx(s);
}

static void
pool_reclaim_unregister(struct pool *pp)
{
	struct vm_map *map = pp->pr_alloc->pa_backingmap;
	int s;

	if (map == NULL) {
		return;
	}

	s = splvm(); /* not necessary for INTRSAFE maps, but don't care. */
	callback_unregister(&vm_map_to_kernel(map)->vmk_reclaim_callback,
	    &pp->pr_reclaimerentry);
	splx(s);
}

static void
pa_reclaim_register(struct pool_allocator *pa)
{
	struct vm_map *map = *pa->pa_backingmapptr;
	struct pool *pp;

	KASSERT(pa->pa_backingmap == NULL);
	if (map == NULL) {
		SLIST_INSERT_HEAD(&pa_deferinitq, pa, pa_q);
		return;
	}
	pa->pa_backingmap = map;
	TAILQ_FOREACH(pp, &pa->pa_list, pr_alloc_list) {
		pool_reclaim_register(pp);
	}
}

/*
 * Initialize all the pools listed in the "pools" link set.
 */
void
pool_subsystem_init(void)
{
	struct pool_allocator *pa;
	__link_set_decl(pools, struct link_pool_init);
	struct link_pool_init * const *pi;

	__link_set_foreach(pi, pools)
		pool_init((*pi)->pp, (*pi)->size, (*pi)->align,
		    (*pi)->align_offset, (*pi)->flags, (*pi)->wchan,
		    (*pi)->palloc, (*pi)->ipl);

	while ((pa = SLIST_FIRST(&pa_deferinitq)) != NULL) {
		KASSERT(pa->pa_backingmapptr != NULL);
		KASSERT(*pa->pa_backingmapptr != NULL);
		SLIST_REMOVE_HEAD(&pa_deferinitq, pa_q);
		pa_reclaim_register(pa);
	}
}

/*
 * Initialize the given pool resource structure.
 *
 * We export this routine to allow other kernel parts to declare
 * static pools that must be initialized before malloc() is available.
 */
void
pool_init(struct pool *pp, size_t size, u_int align, u_int ioff, int flags,
    const char *wchan, struct pool_allocator *palloc, int ipl)
{
#ifdef DEBUG
	struct pool *pp1;
#endif
	size_t trysize, phsize;
	int off, slack, s;

	KASSERT((1UL << (CHAR_BIT * sizeof(pool_item_freelist_t))) - 2 >=
	    PHPOOL_FREELIST_NELEM(PHPOOL_MAX - 1));

#ifdef DEBUG
	/*
	 * Check that the pool hasn't already been initialised and
	 * added to the list of all pools.
	 */
	LIST_FOREACH(pp1, &pool_head, pr_poollist) {
		if (pp == pp1)
			panic("pool_init: pool %s already initialised",
			    wchan);
	}
#endif

#ifdef POOL_DIAGNOSTIC
	/*
	 * Always log if POOL_DIAGNOSTIC is defined.
	 */
	if (pool_logsize != 0)
		flags |= PR_LOGGING;
#endif

	if (palloc == NULL)
		palloc = &pool_allocator_kmem;
#ifdef POOL_SUBPAGE
	if (size > palloc->pa_pagesz) {
		if (palloc == &pool_allocator_kmem)
			palloc = &pool_allocator_kmem_fullpage;
		else if (palloc == &pool_allocator_nointr)
			palloc = &pool_allocator_nointr_fullpage;
	}		
#endif /* POOL_SUBPAGE */
	if ((palloc->pa_flags & PA_INITIALIZED) == 0) {
		if (palloc->pa_pagesz == 0)
			palloc->pa_pagesz = PAGE_SIZE;

		TAILQ_INIT(&palloc->pa_list);

		simple_lock_init(&palloc->pa_slock);
		palloc->pa_pagemask = ~(palloc->pa_pagesz - 1);
		palloc->pa_pageshift = ffs(palloc->pa_pagesz) - 1;

		if (palloc->pa_backingmapptr != NULL) {
			pa_reclaim_register(palloc);
		}
		palloc->pa_flags |= PA_INITIALIZED;
	}

	if (align == 0)
		align = ALIGN(1);

	if ((flags & PR_NOTOUCH) == 0 && size < sizeof(struct pool_item))
		size = sizeof(struct pool_item);

	size = roundup(size, align);
#ifdef DIAGNOSTIC
	if (size > palloc->pa_pagesz)
		panic("pool_init: pool item size (%zu) too large", size);
#endif

	/*
	 * Initialize the pool structure.
	 */
	LIST_INIT(&pp->pr_emptypages);
	LIST_INIT(&pp->pr_fullpages);
	LIST_INIT(&pp->pr_partpages);
	LIST_INIT(&pp->pr_cachelist);
	pp->pr_curpage = NULL;
	pp->pr_npages = 0;
	pp->pr_minitems = 0;
	pp->pr_minpages = 0;
	pp->pr_maxpages = UINT_MAX;
	pp->pr_roflags = flags;
	pp->pr_flags = 0;
	pp->pr_size = size;
	pp->pr_align = align;
	pp->pr_wchan = wchan;
	pp->pr_alloc = palloc;
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

	/*
	 * Decide whether to put the page header off page to avoid
	 * wasting too large a part of the page or too big item.
	 * Off-page page headers go on a hash table, so we can match
	 * a returned item with its header based on the page address.
	 * We use 1/16 of the page size and about 8 times of the item
	 * size as the threshold (XXX: tune)
	 *
	 * However, we'll put the header into the page if we can put
	 * it without wasting any items.
	 *
	 * Silently enforce `0 <= ioff < align'.
	 */
	pp->pr_itemoffset = ioff %= align;
	/* See the comment below about reserved bytes. */
	trysize = palloc->pa_pagesz - ((align - ioff) % align);
	phsize = ALIGN(sizeof(struct pool_item_header));
	if ((pp->pr_roflags & (PR_NOTOUCH | PR_NOALIGN)) == 0 &&
	    (pp->pr_size < MIN(palloc->pa_pagesz / 16, phsize << 3) ||
	    trysize / pp->pr_size == (trysize - phsize) / pp->pr_size)) {
		/* Use the end of the page for the page header */
		pp->pr_roflags |= PR_PHINPAGE;
		pp->pr_phoffset = off = palloc->pa_pagesz - phsize;
	} else {
		/* The page header will be taken from our page header pool */
		pp->pr_phoffset = 0;
		off = palloc->pa_pagesz;
		SPLAY_INIT(&pp->pr_phtree);
	}

	/*
	 * Alignment is to take place at `ioff' within the item. This means
	 * we must reserve up to `align - 1' bytes on the page to allow
	 * appropriate positioning of each item.
	 */
	pp->pr_itemsperpage = (off - ((align - ioff) % align)) / pp->pr_size;
	KASSERT(pp->pr_itemsperpage != 0);
	if ((pp->pr_roflags & PR_NOTOUCH)) {
		int idx;

		for (idx = 0; pp->pr_itemsperpage > PHPOOL_FREELIST_NELEM(idx);
		    idx++) {
			/* nothing */
		}
		if (idx >= PHPOOL_MAX) {
			/*
			 * if you see this panic, consider to tweak
			 * PHPOOL_MAX and PHPOOL_FREELIST_NELEM.
			 */
			panic("%s: too large itemsperpage(%d) for PR_NOTOUCH",
			    pp->pr_wchan, pp->pr_itemsperpage);
		}
		pp->pr_phpool = &phpool[idx];
	} else if ((pp->pr_roflags & PR_PHINPAGE) == 0) {
		pp->pr_phpool = &phpool[0];
	}
#if defined(DIAGNOSTIC)
	else {
		pp->pr_phpool = NULL;
	}
#endif

	/*
	 * Use the slack between the chunks and the page header
	 * for "cache coloring".
	 */
	slack = off - pp->pr_itemsperpage * pp->pr_size;
	pp->pr_maxcolor = (slack / align) * align;
	pp->pr_curcolor = 0;

	pp->pr_nget = 0;
	pp->pr_nfail = 0;
	pp->pr_nput = 0;
	pp->pr_npagealloc = 0;
	pp->pr_npagefree = 0;
	pp->pr_hiwat = 0;
	pp->pr_nidle = 0;

#ifdef POOL_DIAGNOSTIC
	if (flags & PR_LOGGING) {
		if (kmem_map == NULL ||
		    (pp->pr_log = malloc(pool_logsize * sizeof(struct pool_log),
		     M_TEMP, M_NOWAIT)) == NULL)
			pp->pr_roflags &= ~PR_LOGGING;
		pp->pr_curlogentry = 0;
		pp->pr_logsize = pool_logsize;
	}
#endif

	pp->pr_entered_file = NULL;
	pp->pr_entered_line = 0;

	simple_lock_init(&pp->pr_slock);

	/*
	 * Initialize private page header pool and cache magazine pool if we
	 * haven't done so yet.
	 * XXX LOCKING.
	 */
	if (phpool[0].pr_size == 0) {
		int idx;
		for (idx = 0; idx < PHPOOL_MAX; idx++) {
			static char phpool_names[PHPOOL_MAX][6+1+6+1];
			int nelem;
			size_t sz;

			nelem = PHPOOL_FREELIST_NELEM(idx);
			snprintf(phpool_names[idx], sizeof(phpool_names[idx]),
			    "phpool-%d", nelem);
			sz = sizeof(struct pool_item_header);
			if (nelem) {
				sz = PR_FREELIST_ALIGN(sz)
				    + nelem * sizeof(pool_item_freelist_t);
			}
			pool_init(&phpool[idx], sz, 0, 0, 0,
			    phpool_names[idx], &pool_allocator_meta, IPL_VM);
		}
#ifdef POOL_SUBPAGE
		pool_init(&psppool, POOL_SUBPAGE, POOL_SUBPAGE, 0,
		    PR_RECURSIVE, "psppool", &pool_allocator_meta, IPL_VM);
#endif
		pool_init(&pcgpool, sizeof(struct pool_cache_group), 0, 0,
		    0, "pcgpool", &pool_allocator_meta, IPL_VM);
	}

	/* Insert into the list of all pools. */
	simple_lock(&pool_head_slock);
	LIST_INSERT_HEAD(&pool_head, pp, pr_poollist);
	simple_unlock(&pool_head_slock);

	/* Insert this into the list of pools using this allocator. */
	s = splvm();
	simple_lock(&palloc->pa_slock);
	TAILQ_INSERT_TAIL(&palloc->pa_list, pp, pr_alloc_list);
	simple_unlock(&palloc->pa_slock);
	splx(s);
	pool_reclaim_register(pp);
}

/*
 * De-commision a pool resource.
 */
void
pool_destroy(struct pool *pp)
{
	struct pool_pagelist pq;
	struct pool_item_header *ph;
	int s;

	/* Remove from global pool list */
	simple_lock(&pool_head_slock);
	LIST_REMOVE(pp, pr_poollist);
	if (drainpp == pp)
		drainpp = NULL;
	simple_unlock(&pool_head_slock);

	/* Remove this pool from its allocator's list of pools. */
	pool_reclaim_unregister(pp);
	s = splvm();
	simple_lock(&pp->pr_alloc->pa_slock);
	TAILQ_REMOVE(&pp->pr_alloc->pa_list, pp, pr_alloc_list);
	simple_unlock(&pp->pr_alloc->pa_slock);
	splx(s);

	s = splvm();
	simple_lock(&pp->pr_slock);

	KASSERT(LIST_EMPTY(&pp->pr_cachelist));

#ifdef DIAGNOSTIC
	if (pp->pr_nout != 0) {
		pr_printlog(pp, NULL, printf);
		panic("pool_destroy: pool busy: still out: %u",
		    pp->pr_nout);
	}
#endif

	KASSERT(LIST_EMPTY(&pp->pr_fullpages));
	KASSERT(LIST_EMPTY(&pp->pr_partpages));

	/* Remove all pages */
	LIST_INIT(&pq);
	while ((ph = LIST_FIRST(&pp->pr_emptypages)) != NULL)
		pr_rmpage(pp, ph, &pq);

	simple_unlock(&pp->pr_slock);
	splx(s);

	pr_pagelist_free(pp, &pq);

#ifdef POOL_DIAGNOSTIC
	if ((pp->pr_roflags & PR_LOGGING) != 0)
		free(pp->pr_log, M_TEMP);
#endif
}

void
pool_set_drain_hook(struct pool *pp, void (*fn)(void *, int), void *arg)
{

	/* XXX no locking -- must be used just after pool_init() */
#ifdef DIAGNOSTIC
	if (pp->pr_drain_hook != NULL)
		panic("pool_set_drain_hook(%s): already set", pp->pr_wchan);
#endif
	pp->pr_drain_hook = fn;
	pp->pr_drain_hook_arg = arg;
}

static struct pool_item_header *
pool_alloc_item_header(struct pool *pp, void *storage, int flags)
{
	struct pool_item_header *ph;
	int s;

	LOCK_ASSERT(simple_lock_held(&pp->pr_slock) == 0);

	if ((pp->pr_roflags & PR_PHINPAGE) != 0)
		ph = (struct pool_item_header *) ((char *)storage + pp->pr_phoffset);
	else {
		s = splvm();
		ph = pool_get(pp->pr_phpool, flags);
		splx(s);
	}

	return (ph);
}

/*
 * Grab an item from the pool; must be called at appropriate spl level
 */
void *
#ifdef POOL_DIAGNOSTIC
_pool_get(struct pool *pp, int flags, const char *file, long line)
#else
pool_get(struct pool *pp, int flags)
#endif
{
	struct pool_item *pi;
	struct pool_item_header *ph;
	void *v;

#ifdef DIAGNOSTIC
	if (__predict_false(pp->pr_itemsperpage == 0))
		panic("pool_get: pool %p: pr_itemsperpage is zero, "
		    "pool not initialized?", pp);
	if (__predict_false(curlwp == NULL && doing_shutdown == 0 &&
			    (flags & PR_WAITOK) != 0))
		panic("pool_get: %s: must have NOWAIT", pp->pr_wchan);

#endif /* DIAGNOSTIC */
#ifdef LOCKDEBUG
	if (flags & PR_WAITOK)
		ASSERT_SLEEPABLE(NULL, "pool_get(PR_WAITOK)");
#endif

	simple_lock(&pp->pr_slock);
	pr_enter(pp, file, line);

 startover:
	/*
	 * Check to see if we've reached the hard limit.  If we have,
	 * and we can wait, then wait until an item has been returned to
	 * the pool.
	 */
#ifdef DIAGNOSTIC
	if (__predict_false(pp->pr_nout > pp->pr_hardlimit)) {
		pr_leave(pp);
		simple_unlock(&pp->pr_slock);
		panic("pool_get: %s: crossed hard limit", pp->pr_wchan);
	}
#endif
	if (__predict_false(pp->pr_nout == pp->pr_hardlimit)) {
		if (pp->pr_drain_hook != NULL) {
			/*
			 * Since the drain hook is going to free things
			 * back to the pool, unlock, call the hook, re-lock,
			 * and check the hardlimit condition again.
			 */
			pr_leave(pp);
			simple_unlock(&pp->pr_slock);
			(*pp->pr_drain_hook)(pp->pr_drain_hook_arg, flags);
			simple_lock(&pp->pr_slock);
			pr_enter(pp, file, line);
			if (pp->pr_nout < pp->pr_hardlimit)
				goto startover;
		}

		if ((flags & PR_WAITOK) && !(flags & PR_LIMITFAIL)) {
			/*
			 * XXX: A warning isn't logged in this case.  Should
			 * it be?
			 */
			pp->pr_flags |= PR_WANTED;
			pr_leave(pp);
			ltsleep(pp, PSWP, pp->pr_wchan, 0, &pp->pr_slock);
			pr_enter(pp, file, line);
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

		pr_leave(pp);
		simple_unlock(&pp->pr_slock);
		return (NULL);
	}

	/*
	 * The convention we use is that if `curpage' is not NULL, then
	 * it points at a non-empty bucket. In particular, `curpage'
	 * never points at a page header which has PR_PHINPAGE set and
	 * has no items in its bucket.
	 */
	if ((ph = pp->pr_curpage) == NULL) {
		int error;

#ifdef DIAGNOSTIC
		if (pp->pr_nitems != 0) {
			simple_unlock(&pp->pr_slock);
			printf("pool_get: %s: curpage NULL, nitems %u\n",
			    pp->pr_wchan, pp->pr_nitems);
			panic("pool_get: nitems inconsistent");
		}
#endif

		/*
		 * Call the back-end page allocator for more memory.
		 * Release the pool lock, as the back-end page allocator
		 * may block.
		 */
		pr_leave(pp);
		error = pool_grow(pp, flags);
		pr_enter(pp, file, line);
		if (error != 0) {
			/*
			 * We were unable to allocate a page or item
			 * header, but we released the lock during
			 * allocation, so perhaps items were freed
			 * back to the pool.  Check for this case.
			 */
			if (pp->pr_curpage != NULL)
				goto startover;

			pp->pr_nfail++;
			pr_leave(pp);
			simple_unlock(&pp->pr_slock);
			return (NULL);
		}

		/* Start the allocation process over. */
		goto startover;
	}
	if (pp->pr_roflags & PR_NOTOUCH) {
#ifdef DIAGNOSTIC
		if (__predict_false(ph->ph_nmissing == pp->pr_itemsperpage)) {
			pr_leave(pp);
			simple_unlock(&pp->pr_slock);
			panic("pool_get: %s: page empty", pp->pr_wchan);
		}
#endif
		v = pr_item_notouch_get(pp, ph);
#ifdef POOL_DIAGNOSTIC
		pr_log(pp, v, PRLOG_GET, file, line);
#endif
	} else {
		v = pi = LIST_FIRST(&ph->ph_itemlist);
		if (__predict_false(v == NULL)) {
			pr_leave(pp);
			simple_unlock(&pp->pr_slock);
			panic("pool_get: %s: page empty", pp->pr_wchan);
		}
#ifdef DIAGNOSTIC
		if (__predict_false(pp->pr_nitems == 0)) {
			pr_leave(pp);
			simple_unlock(&pp->pr_slock);
			printf("pool_get: %s: items on itemlist, nitems %u\n",
			    pp->pr_wchan, pp->pr_nitems);
			panic("pool_get: nitems inconsistent");
		}
#endif

#ifdef POOL_DIAGNOSTIC
		pr_log(pp, v, PRLOG_GET, file, line);
#endif

#ifdef DIAGNOSTIC
		if (__predict_false(pi->pi_magic != PI_MAGIC)) {
			pr_printlog(pp, pi, printf);
			panic("pool_get(%s): free list modified: "
			    "magic=%x; page %p; item addr %p\n",
			    pp->pr_wchan, pi->pi_magic, ph->ph_page, pi);
		}
#endif

		/*
		 * Remove from item list.
		 */
		LIST_REMOVE(pi, pi_list);
	}
	pp->pr_nitems--;
	pp->pr_nout++;
	if (ph->ph_nmissing == 0) {
#ifdef DIAGNOSTIC
		if (__predict_false(pp->pr_nidle == 0))
			panic("pool_get: nidle inconsistent");
#endif
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
#ifdef DIAGNOSTIC
		if (__predict_false((pp->pr_roflags & PR_NOTOUCH) == 0 &&
		    !LIST_EMPTY(&ph->ph_itemlist))) {
			pr_leave(pp);
			simple_unlock(&pp->pr_slock);
			panic("pool_get: %s: nmissing inconsistent",
			    pp->pr_wchan);
		}
#endif
		/*
		 * This page is now full.  Move it to the full list
		 * and select a new current page.
		 */
		LIST_REMOVE(ph, ph_pagelist);
		LIST_INSERT_HEAD(&pp->pr_fullpages, ph, ph_pagelist);
		pool_update_curpage(pp);
	}

	pp->pr_nget++;
	pr_leave(pp);

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

	simple_unlock(&pp->pr_slock);
	KASSERT((((vaddr_t)v + pp->pr_itemoffset) & (pp->pr_align - 1)) == 0);
	FREECHECK_OUT(&pp->pr_freecheck, v);
	return (v);
}

/*
 * Internal version of pool_put().  Pool is already locked/entered.
 */
static void
pool_do_put(struct pool *pp, void *v, struct pool_pagelist *pq)
{
	struct pool_item *pi = v;
	struct pool_item_header *ph;

	LOCK_ASSERT(simple_lock_held(&pp->pr_slock));
	FREECHECK_IN(&pp->pr_freecheck, v);

#ifdef DIAGNOSTIC
	if (__predict_false(pp->pr_nout == 0)) {
		printf("pool %s: putting with none out\n",
		    pp->pr_wchan);
		panic("pool_put");
	}
#endif

	if (__predict_false((ph = pr_find_pagehead(pp, v)) == NULL)) {
		pr_printlog(pp, NULL, printf);
		panic("pool_put: %s: page header missing", pp->pr_wchan);
	}

#ifdef LOCKDEBUG
	/*
	 * Check if we're freeing a locked simple lock.
	 */
	simple_lock_freecheck(pi, (char *)pi + pp->pr_size);
#endif

	/*
	 * Return to item list.
	 */
	if (pp->pr_roflags & PR_NOTOUCH) {
		pr_item_notouch_put(pp, ph, v);
	} else {
#ifdef DIAGNOSTIC
		pi->pi_magic = PI_MAGIC;
#endif
#ifdef DEBUG
		{
			int i, *ip = v;

			for (i = 0; i < pp->pr_size / sizeof(int); i++) {
				*ip++ = PI_MAGIC;
			}
		}
#endif

		LIST_INSERT_HEAD(&ph->ph_itemlist, pi, pi_list);
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
		if (ph->ph_nmissing == 0)
			pp->pr_nidle++;
		wakeup((void *)pp);
		return;
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
		if (pp->pr_npages > pp->pr_minpages &&
		    (pp->pr_npages > pp->pr_maxpages ||
		     pa_starved_p(pp->pr_alloc))) {
			pr_rmpage(pp, ph, pq);
		} else {
			LIST_REMOVE(ph, ph_pagelist);
			LIST_INSERT_HEAD(&pp->pr_emptypages, ph, ph_pagelist);

			/*
			 * Update the timestamp on the page.  A page must
			 * be idle for some period of time before it can
			 * be reclaimed by the pagedaemon.  This minimizes
			 * ping-pong'ing for memory.
			 */
			getmicrotime(&ph->ph_time);
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

/*
 * Return resource to the pool; must be called at appropriate spl level
 */
#ifdef POOL_DIAGNOSTIC
void
_pool_put(struct pool *pp, void *v, const char *file, long line)
{
	struct pool_pagelist pq;

	LIST_INIT(&pq);

	simple_lock(&pp->pr_slock);
	pr_enter(pp, file, line);

	pr_log(pp, v, PRLOG_PUT, file, line);

	pool_do_put(pp, v, &pq);

	pr_leave(pp);
	simple_unlock(&pp->pr_slock);

	pr_pagelist_free(pp, &pq);
}
#undef pool_put
#endif /* POOL_DIAGNOSTIC */

void
pool_put(struct pool *pp, void *v)
{
	struct pool_pagelist pq;

	LIST_INIT(&pq);

	simple_lock(&pp->pr_slock);
	pool_do_put(pp, v, &pq);
	simple_unlock(&pp->pr_slock);

	pr_pagelist_free(pp, &pq);
}

#ifdef POOL_DIAGNOSTIC
#define		pool_put(h, v)	_pool_put((h), (v), __FILE__, __LINE__)
#endif

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
	struct pool_item_header *ph = NULL;
	char *cp;

	simple_unlock(&pp->pr_slock);
	cp = pool_allocator_alloc(pp, flags);
	if (__predict_true(cp != NULL)) {
		ph = pool_alloc_item_header(pp, cp, flags);
	}
	if (__predict_false(cp == NULL || ph == NULL)) {
		if (cp != NULL) {
			pool_allocator_free(pp, cp);
		}
		simple_lock(&pp->pr_slock);
		return ENOMEM;
	}

	simple_lock(&pp->pr_slock);
	pool_prime_page(pp, cp, ph);
	pp->pr_npagealloc++;
	return 0;
}

/*
 * Add N items to the pool.
 */
int
pool_prime(struct pool *pp, int n)
{
	int newpages;
	int error = 0;

	simple_lock(&pp->pr_slock);

	newpages = roundup(n, pp->pr_itemsperpage) / pp->pr_itemsperpage;

	while (newpages-- > 0) {
		error = pool_grow(pp, PR_NOWAIT);
		if (error) {
			break;
		}
		pp->pr_minpages++;
	}

	if (pp->pr_minpages >= pp->pr_maxpages)
		pp->pr_maxpages = pp->pr_minpages + 1;	/* XXX */

	simple_unlock(&pp->pr_slock);
	return error;
}

/*
 * Add a page worth of items to the pool.
 *
 * Note, we must be called with the pool descriptor LOCKED.
 */
static void
pool_prime_page(struct pool *pp, void *storage, struct pool_item_header *ph)
{
	struct pool_item *pi;
	void *cp = storage;
	const unsigned int align = pp->pr_align;
	const unsigned int ioff = pp->pr_itemoffset;
	int n;

	LOCK_ASSERT(simple_lock_held(&pp->pr_slock));

#ifdef DIAGNOSTIC
	if ((pp->pr_roflags & PR_NOALIGN) == 0 &&
	    ((uintptr_t)cp & (pp->pr_alloc->pa_pagesz - 1)) != 0)
		panic("pool_prime_page: %s: unaligned page", pp->pr_wchan);
#endif

	/*
	 * Insert page header.
	 */
	LIST_INSERT_HEAD(&pp->pr_emptypages, ph, ph_pagelist);
	LIST_INIT(&ph->ph_itemlist);
	ph->ph_page = storage;
	ph->ph_nmissing = 0;
	getmicrotime(&ph->ph_time);
	if ((pp->pr_roflags & PR_PHINPAGE) == 0)
		SPLAY_INSERT(phtree, &pp->pr_phtree, ph);

	pp->pr_nidle++;

	/*
	 * Color this page.
	 */
	cp = (char *)cp + pp->pr_curcolor;
	if ((pp->pr_curcolor += align) > pp->pr_maxcolor)
		pp->pr_curcolor = 0;

	/*
	 * Adjust storage to apply aligment to `pr_itemoffset' in each item.
	 */
	if (ioff != 0)
		cp = (char *)cp + align - ioff;

	KASSERT((((vaddr_t)cp + ioff) & (align - 1)) == 0);

	/*
	 * Insert remaining chunks on the bucket list.
	 */
	n = pp->pr_itemsperpage;
	pp->pr_nitems += n;

	if (pp->pr_roflags & PR_NOTOUCH) {
		pool_item_freelist_t *freelist = PR_FREELIST(ph);
		int i;

		ph->ph_off = (char *)cp - (char *)storage;
		ph->ph_firstfree = 0;
		for (i = 0; i < n - 1; i++)
			freelist[i] = i + 1;
		freelist[n - 1] = PR_INDEX_EOL;
	} else {
		while (n--) {
			pi = (struct pool_item *)cp;

			KASSERT(((((vaddr_t)pi) + ioff) & (align - 1)) == 0);

			/* Insert on page list */
			LIST_INSERT_HEAD(&ph->ph_itemlist, pi, pi_list);
#ifdef DIAGNOSTIC
			pi->pi_magic = PI_MAGIC;
#endif
			cp = (char *)cp + pp->pr_size;

			KASSERT((((vaddr_t)cp + ioff) & (align - 1)) == 0);
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
}

void
pool_setlowat(struct pool *pp, int n)
{

	simple_lock(&pp->pr_slock);

	pp->pr_minitems = n;
	pp->pr_minpages = (n == 0)
		? 0
		: roundup(n, pp->pr_itemsperpage) / pp->pr_itemsperpage;

	/* Make sure we're caught up with the newly-set low water mark. */
	if (POOL_NEEDS_CATCHUP(pp) && pool_catchup(pp) != 0) {
		/*
		 * XXX: Should we log a warning?  Should we set up a timeout
		 * to try again in a second or so?  The latter could break
		 * a caller's assumptions about interrupt protection, etc.
		 */
	}

	simple_unlock(&pp->pr_slock);
}

void
pool_sethiwat(struct pool *pp, int n)
{

	simple_lock(&pp->pr_slock);

	pp->pr_maxpages = (n == 0)
		? 0
		: roundup(n, pp->pr_itemsperpage) / pp->pr_itemsperpage;

	simple_unlock(&pp->pr_slock);
}

void
pool_sethardlimit(struct pool *pp, int n, const char *warnmess, int ratecap)
{

	simple_lock(&pp->pr_slock);

	pp->pr_hardlimit = n;
	pp->pr_hardlimit_warning = warnmess;
	pp->pr_hardlimit_ratecap.tv_sec = ratecap;
	pp->pr_hardlimit_warning_last.tv_sec = 0;
	pp->pr_hardlimit_warning_last.tv_usec = 0;

	/*
	 * In-line version of pool_sethiwat(), because we don't want to
	 * release the lock.
	 */
	pp->pr_maxpages = (n == 0)
		? 0
		: roundup(n, pp->pr_itemsperpage) / pp->pr_itemsperpage;

	simple_unlock(&pp->pr_slock);
}

/*
 * Release all complete pages that have not been used recently.
 */
int
#ifdef POOL_DIAGNOSTIC
_pool_reclaim(struct pool *pp, const char *file, long line)
#else
pool_reclaim(struct pool *pp)
#endif
{
	struct pool_item_header *ph, *phnext;
	struct pool_cache *pc;
	struct pool_pagelist pq;
	struct pool_cache_grouplist pcgl;
	struct timeval curtime, diff;

	if (pp->pr_drain_hook != NULL) {
		/*
		 * The drain hook must be called with the pool unlocked.
		 */
		(*pp->pr_drain_hook)(pp->pr_drain_hook_arg, PR_NOWAIT);
	}

	if (simple_lock_try(&pp->pr_slock) == 0)
		return (0);
	pr_enter(pp, file, line);

	LIST_INIT(&pq);
	LIST_INIT(&pcgl);

	/*
	 * Reclaim items from the pool's caches.
	 */
	LIST_FOREACH(pc, &pp->pr_cachelist, pc_poollist)
		pool_cache_reclaim(pc, &pq, &pcgl);

	getmicrotime(&curtime);

	for (ph = LIST_FIRST(&pp->pr_emptypages); ph != NULL; ph = phnext) {
		phnext = LIST_NEXT(ph, ph_pagelist);

		/* Check our minimum page claim */
		if (pp->pr_npages <= pp->pr_minpages)
			break;

		KASSERT(ph->ph_nmissing == 0);
		timersub(&curtime, &ph->ph_time, &diff);
		if (diff.tv_sec < pool_inactive_time
		    && !pa_starved_p(pp->pr_alloc))
			continue;

		/*
		 * If freeing this page would put us below
		 * the low water mark, stop now.
		 */
		if ((pp->pr_nitems - pp->pr_itemsperpage) <
		    pp->pr_minitems)
			break;

		pr_rmpage(pp, ph, &pq);
	}

	pr_leave(pp);
	simple_unlock(&pp->pr_slock);
	if (LIST_EMPTY(&pq) && LIST_EMPTY(&pcgl))
		return 0;

	pr_pagelist_free(pp, &pq);
	pcg_grouplist_free(&pcgl);
	return (1);
}

/*
 * Drain pools, one at a time.
 *
 * Note, we must never be called from an interrupt context.
 */
void
pool_drain(void *arg)
{
	struct pool *pp;
	int s;

	pp = NULL;
	s = splvm();
	simple_lock(&pool_head_slock);
	if (drainpp == NULL) {
		drainpp = LIST_FIRST(&pool_head);
	}
	if (drainpp) {
		pp = drainpp;
		drainpp = LIST_NEXT(pp, pr_poollist);
	}
	simple_unlock(&pool_head_slock);
	if (pp)
		pool_reclaim(pp);
	splx(s);
}

/*
 * Diagnostic helpers.
 */
void
pool_print(struct pool *pp, const char *modif)
{
	int s;

	s = splvm();
	if (simple_lock_try(&pp->pr_slock) == 0) {
		printf("pool %s is locked; try again later\n",
		    pp->pr_wchan);
		splx(s);
		return;
	}
	pool_print1(pp, modif, printf);
	simple_unlock(&pp->pr_slock);
	splx(s);
}

void
pool_printall(const char *modif, void (*pr)(const char *, ...))
{
	struct pool *pp;

	if (simple_lock_try(&pool_head_slock) == 0) {
		(*pr)("WARNING: pool_head_slock is locked\n");
	} else {
		simple_unlock(&pool_head_slock);
	}

	LIST_FOREACH(pp, &pool_head, pr_poollist) {
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

	/*
	 * Called from DDB; interrupts should be blocked, and all
	 * other processors should be paused.  We can skip locking
	 * the pool in this case.
	 *
	 * We do a simple_lock_try() just to print the lock
	 * status, however.
	 */

	if (simple_lock_try(&pp->pr_slock) == 0)
		(*pr)("WARNING: pool %s is locked\n", pp->pr_wchan);
	else
		simple_unlock(&pp->pr_slock);

	pool_print1(pp, modif, pr);
}

static void
pool_print_pagelist(struct pool *pp, struct pool_pagelist *pl,
    void (*pr)(const char *, ...))
{
	struct pool_item_header *ph;
#ifdef DIAGNOSTIC
	struct pool_item *pi;
#endif

	LIST_FOREACH(ph, pl, ph_pagelist) {
		(*pr)("\t\tpage %p, nmissing %d, time %lu,%lu\n",
		    ph->ph_page, ph->ph_nmissing,
		    (u_long)ph->ph_time.tv_sec,
		    (u_long)ph->ph_time.tv_usec);
#ifdef DIAGNOSTIC
		if (!(pp->pr_roflags & PR_NOTOUCH)) {
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
	struct pool_cache *pc;
	struct pool_cache_group *pcg;
	int i, print_log = 0, print_pagelist = 0, print_cache = 0;
	char c;

	while ((c = *modif++) != '\0') {
		if (c == 'l')
			print_log = 1;
		if (c == 'p')
			print_pagelist = 1;
		if (c == 'c')
			print_cache = 1;
	}

	(*pr)("POOL %s: size %u, align %u, ioff %u, roflags 0x%08x\n",
	    pp->pr_wchan, pp->pr_size, pp->pr_align, pp->pr_itemoffset,
	    pp->pr_roflags);
	(*pr)("\talloc %p\n", pp->pr_alloc);
	(*pr)("\tminitems %u, minpages %u, maxpages %u, npages %u\n",
	    pp->pr_minitems, pp->pr_minpages, pp->pr_maxpages, pp->pr_npages);
	(*pr)("\titemsperpage %u, nitems %u, nout %u, hardlimit %u\n",
	    pp->pr_itemsperpage, pp->pr_nitems, pp->pr_nout, pp->pr_hardlimit);

	(*pr)("\n\tnget %lu, nfail %lu, nput %lu\n",
	    pp->pr_nget, pp->pr_nfail, pp->pr_nput);
	(*pr)("\tnpagealloc %lu, npagefree %lu, hiwat %u, nidle %lu\n",
	    pp->pr_npagealloc, pp->pr_npagefree, pp->pr_hiwat, pp->pr_nidle);

	if (print_pagelist == 0)
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
	if (print_log == 0)
		goto skip_log;

	(*pr)("\n");
	if ((pp->pr_roflags & PR_LOGGING) == 0)
		(*pr)("\tno log\n");
	else {
		pr_printlog(pp, NULL, pr);
	}

 skip_log:
	if (print_cache == 0)
		goto skip_cache;

#define PR_GROUPLIST(pcg)						\
	(*pr)("\t\tgroup %p: avail %d\n", pcg, pcg->pcg_avail);		\
	for (i = 0; i < PCG_NOBJECTS; i++) {				\
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

	LIST_FOREACH(pc, &pp->pr_cachelist, pc_poollist) {
		(*pr)("\tcache %p\n", pc);
		(*pr)("\t    hits %lu misses %lu ngroups %lu nitems %lu\n",
		    pc->pc_hits, pc->pc_misses, pc->pc_ngroups, pc->pc_nitems);
		(*pr)("\t    full groups:\n");
		LIST_FOREACH(pcg, &pc->pc_fullgroups, pcg_list) {
			PR_GROUPLIST(pcg);
		}
		(*pr)("\t    partial groups:\n");
		LIST_FOREACH(pcg, &pc->pc_partgroups, pcg_list) {
			PR_GROUPLIST(pcg);
		}
		(*pr)("\t    empty groups:\n");
		LIST_FOREACH(pcg, &pc->pc_emptygroups, pcg_list) {
			PR_GROUPLIST(pcg);
		}
	}
#undef PR_GROUPLIST

 skip_cache:
	pr_enter_check(pp, pr);
}

static int
pool_chk_page(struct pool *pp, const char *label, struct pool_item_header *ph)
{
	struct pool_item *pi;
	void *page;
	int n;

	if ((pp->pr_roflags & PR_NOALIGN) == 0) {
		page = (void *)((uintptr_t)ph & pp->pr_alloc->pa_pagemask);
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

	if ((pp->pr_roflags & PR_NOTOUCH) != 0)
		return 0;

	for (pi = LIST_FIRST(&ph->ph_itemlist), n = 0;
	     pi != NULL;
	     pi = LIST_NEXT(pi,pi_list), n++) {

#ifdef DIAGNOSTIC
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
		page = (void *)((uintptr_t)pi & pp->pr_alloc->pa_pagemask);
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

	simple_lock(&pp->pr_slock);
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
	simple_unlock(&pp->pr_slock);
	return (r);
}

/*
 * pool_cache_init:
 *
 *	Initialize a pool cache.
 *
 *	NOTE: If the pool must be protected from interrupts, we expect
 *	to be called at the appropriate interrupt priority level.
 */
void
pool_cache_init(struct pool_cache *pc, struct pool *pp,
    int (*ctor)(void *, void *, int),
    void (*dtor)(void *, void *),
    void *arg)
{

	LIST_INIT(&pc->pc_emptygroups);
	LIST_INIT(&pc->pc_fullgroups);
	LIST_INIT(&pc->pc_partgroups);
	simple_lock_init(&pc->pc_slock);

	pc->pc_pool = pp;

	pc->pc_ctor = ctor;
	pc->pc_dtor = dtor;
	pc->pc_arg  = arg;

	pc->pc_hits   = 0;
	pc->pc_misses = 0;

	pc->pc_ngroups = 0;

	pc->pc_nitems = 0;

	simple_lock(&pp->pr_slock);
	LIST_INSERT_HEAD(&pp->pr_cachelist, pc, pc_poollist);
	simple_unlock(&pp->pr_slock);
}

/*
 * pool_cache_destroy:
 *
 *	Destroy a pool cache.
 */
void
pool_cache_destroy(struct pool_cache *pc)
{
	struct pool *pp = pc->pc_pool;

	/* First, invalidate the entire cache. */
	pool_cache_invalidate(pc);

	/* ...and remove it from the pool's cache list. */
	simple_lock(&pp->pr_slock);
	LIST_REMOVE(pc, pc_poollist);
	simple_unlock(&pp->pr_slock);
}

static inline void *
pcg_get(struct pool_cache_group *pcg, paddr_t *pap)
{
	void *object;
	u_int idx;

	KASSERT(pcg->pcg_avail <= PCG_NOBJECTS);
	KASSERT(pcg->pcg_avail != 0);
	idx = --pcg->pcg_avail;

	KASSERT(pcg->pcg_objects[idx].pcgo_va != NULL);
	object = pcg->pcg_objects[idx].pcgo_va;
	if (pap != NULL)
		*pap = pcg->pcg_objects[idx].pcgo_pa;
	pcg->pcg_objects[idx].pcgo_va = NULL;

	return (object);
}

static inline void
pcg_put(struct pool_cache_group *pcg, void *object, paddr_t pa)
{
	u_int idx;

	KASSERT(pcg->pcg_avail < PCG_NOBJECTS);
	idx = pcg->pcg_avail++;

	KASSERT(pcg->pcg_objects[idx].pcgo_va == NULL);
	pcg->pcg_objects[idx].pcgo_va = object;
	pcg->pcg_objects[idx].pcgo_pa = pa;
}

static void
pcg_grouplist_free(struct pool_cache_grouplist *pcgl)
{
	struct pool_cache_group *pcg;
	int s;

	s = splvm();
	while ((pcg = LIST_FIRST(pcgl)) != NULL) {
		LIST_REMOVE(pcg, pcg_list);
		pool_put(&pcgpool, pcg);
	}
	splx(s);
}

/*
 * pool_cache_get{,_paddr}:
 *
 *	Get an object from a pool cache (optionally returning
 *	the physical address of the object).
 */
void *
pool_cache_get_paddr(struct pool_cache *pc, int flags, paddr_t *pap)
{
	struct pool_cache_group *pcg;
	void *object;

#ifdef LOCKDEBUG
	if (flags & PR_WAITOK)
		ASSERT_SLEEPABLE(NULL, "pool_cache_get(PR_WAITOK)");
#endif

	simple_lock(&pc->pc_slock);

	pcg = LIST_FIRST(&pc->pc_partgroups);
	if (pcg == NULL) {
		pcg = LIST_FIRST(&pc->pc_fullgroups);
		if (pcg != NULL) {
			LIST_REMOVE(pcg, pcg_list);
			LIST_INSERT_HEAD(&pc->pc_partgroups, pcg, pcg_list);
		}
	}
	if (pcg == NULL) {

		/*
		 * No groups with any available objects.  Allocate
		 * a new object, construct it, and return it to
		 * the caller.  We will allocate a group, if necessary,
		 * when the object is freed back to the cache.
		 */
		pc->pc_misses++;
		simple_unlock(&pc->pc_slock);
		object = pool_get(pc->pc_pool, flags);
		if (object != NULL && pc->pc_ctor != NULL) {
			if ((*pc->pc_ctor)(pc->pc_arg, object, flags) != 0) {
				pool_put(pc->pc_pool, object);
				return (NULL);
			}
		}
		KASSERT((((vaddr_t)object + pc->pc_pool->pr_itemoffset) &
		    (pc->pc_pool->pr_align - 1)) == 0);
		if (object != NULL && pap != NULL) {
#ifdef POOL_VTOPHYS
			*pap = POOL_VTOPHYS(object);
#else
			*pap = POOL_PADDR_INVALID;
#endif
		}

		FREECHECK_OUT(&pc->pc_freecheck, object);
		return (object);
	}

	pc->pc_hits++;
	pc->pc_nitems--;
	object = pcg_get(pcg, pap);

	if (pcg->pcg_avail == 0) {
		LIST_REMOVE(pcg, pcg_list);
		LIST_INSERT_HEAD(&pc->pc_emptygroups, pcg, pcg_list);
	}
	simple_unlock(&pc->pc_slock);

	KASSERT((((vaddr_t)object + pc->pc_pool->pr_itemoffset) &
	    (pc->pc_pool->pr_align - 1)) == 0);
	FREECHECK_OUT(&pc->pc_freecheck, object);
	return (object);
}

/*
 * pool_cache_put{,_paddr}:
 *
 *	Put an object back to the pool cache (optionally caching the
 *	physical address of the object).
 */
void
pool_cache_put_paddr(struct pool_cache *pc, void *object, paddr_t pa)
{
	struct pool_cache_group *pcg;
	int s;

	FREECHECK_IN(&pc->pc_freecheck, object);

	if (__predict_false((pc->pc_pool->pr_flags & PR_WANTED) != 0)) {
		goto destruct;
	}

	simple_lock(&pc->pc_slock);

	pcg = LIST_FIRST(&pc->pc_partgroups);
	if (pcg == NULL) {
		pcg = LIST_FIRST(&pc->pc_emptygroups);
		if (pcg != NULL) {
			LIST_REMOVE(pcg, pcg_list);
			LIST_INSERT_HEAD(&pc->pc_partgroups, pcg, pcg_list);
		}
	}
	if (pcg == NULL) {

		/*
		 * No empty groups to free the object to.  Attempt to
		 * allocate one.
		 */
		simple_unlock(&pc->pc_slock);
		s = splvm();
		pcg = pool_get(&pcgpool, PR_NOWAIT);
		splx(s);
		if (pcg == NULL) {
destruct:

			/*
			 * Unable to allocate a cache group; destruct the object
			 * and free it back to the pool.
			 */
			pool_cache_destruct_object(pc, object);
			return;
		}
		memset(pcg, 0, sizeof(*pcg));
		simple_lock(&pc->pc_slock);
		pc->pc_ngroups++;
		LIST_INSERT_HEAD(&pc->pc_partgroups, pcg, pcg_list);
	}

	pc->pc_nitems++;
	pcg_put(pcg, object, pa);

	if (pcg->pcg_avail == PCG_NOBJECTS) {
		LIST_REMOVE(pcg, pcg_list);
		LIST_INSERT_HEAD(&pc->pc_fullgroups, pcg, pcg_list);
	}
	simple_unlock(&pc->pc_slock);
}

/*
 * pool_cache_destruct_object:
 *
 *	Force destruction of an object and its release back into
 *	the pool.
 */
void
pool_cache_destruct_object(struct pool_cache *pc, void *object)
{

	if (pc->pc_dtor != NULL)
		(*pc->pc_dtor)(pc->pc_arg, object);
	pool_put(pc->pc_pool, object);
}

static void
pool_do_cache_invalidate_grouplist(struct pool_cache_grouplist *pcgsl,
    struct pool_cache *pc, struct pool_pagelist *pq,
    struct pool_cache_grouplist *pcgdl)
{
	struct pool_cache_group *pcg, *npcg;
	void *object;

	for (pcg = LIST_FIRST(pcgsl); pcg != NULL; pcg = npcg) {
		npcg = LIST_NEXT(pcg, pcg_list);
		while (pcg->pcg_avail != 0) {
			pc->pc_nitems--;
			object = pcg_get(pcg, NULL);
			if (pc->pc_dtor != NULL)
				(*pc->pc_dtor)(pc->pc_arg, object);
			pool_do_put(pc->pc_pool, object, pq);
		}
		pc->pc_ngroups--;
		LIST_REMOVE(pcg, pcg_list);
		LIST_INSERT_HEAD(pcgdl, pcg, pcg_list);
	}
}

static void
pool_do_cache_invalidate(struct pool_cache *pc, struct pool_pagelist *pq,
    struct pool_cache_grouplist *pcgl)
{

	LOCK_ASSERT(simple_lock_held(&pc->pc_slock));
	LOCK_ASSERT(simple_lock_held(&pc->pc_pool->pr_slock));

	pool_do_cache_invalidate_grouplist(&pc->pc_fullgroups, pc, pq, pcgl);
	pool_do_cache_invalidate_grouplist(&pc->pc_partgroups, pc, pq, pcgl);

	KASSERT(LIST_EMPTY(&pc->pc_partgroups));
	KASSERT(LIST_EMPTY(&pc->pc_fullgroups));
	KASSERT(pc->pc_nitems == 0);
}

/*
 * pool_cache_invalidate:
 *
 *	Invalidate a pool cache (destruct and release all of the
 *	cached objects).
 */
void
pool_cache_invalidate(struct pool_cache *pc)
{
	struct pool_pagelist pq;
	struct pool_cache_grouplist pcgl;

	LIST_INIT(&pq);
	LIST_INIT(&pcgl);

	simple_lock(&pc->pc_slock);
	simple_lock(&pc->pc_pool->pr_slock);

	pool_do_cache_invalidate(pc, &pq, &pcgl);

	simple_unlock(&pc->pc_pool->pr_slock);
	simple_unlock(&pc->pc_slock);

	pr_pagelist_free(pc->pc_pool, &pq);
	pcg_grouplist_free(&pcgl);
}

/*
 * pool_cache_reclaim:
 *
 *	Reclaim a pool cache for pool_reclaim().
 */
static void
pool_cache_reclaim(struct pool_cache *pc, struct pool_pagelist *pq,
    struct pool_cache_grouplist *pcgl)
{

	/*
	 * We're locking in the wrong order (normally pool_cache -> pool,
	 * but the pool is already locked when we get here), so we have
	 * to use trylock.  If we can't lock the pool_cache, it's not really
	 * a big deal here.
	 */
	if (simple_lock_try(&pc->pc_slock) == 0)
		return;

	pool_do_cache_invalidate(pc, pq, pcgl);

	simple_unlock(&pc->pc_slock);
}

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
void	*pool_page_alloc(struct pool *, int);
void	pool_page_free(struct pool *, void *);

#ifdef POOL_SUBPAGE
struct pool_allocator pool_allocator_kmem_fullpage = {
	pool_page_alloc, pool_page_free, 0,
	.pa_backingmapptr = &kmem_map,
};
#else
struct pool_allocator pool_allocator_kmem = {
	pool_page_alloc, pool_page_free, 0,
	.pa_backingmapptr = &kmem_map,
};
#endif

void	*pool_page_alloc_nointr(struct pool *, int);
void	pool_page_free_nointr(struct pool *, void *);

#ifdef POOL_SUBPAGE
struct pool_allocator pool_allocator_nointr_fullpage = {
	pool_page_alloc_nointr, pool_page_free_nointr, 0,
	.pa_backingmapptr = &kernel_map,
};
#else
struct pool_allocator pool_allocator_nointr = {
	pool_page_alloc_nointr, pool_page_free_nointr, 0,
	.pa_backingmapptr = &kernel_map,
};
#endif

#ifdef POOL_SUBPAGE
void	*pool_subpage_alloc(struct pool *, int);
void	pool_subpage_free(struct pool *, void *);

struct pool_allocator pool_allocator_kmem = {
	pool_subpage_alloc, pool_subpage_free, POOL_SUBPAGE,
	.pa_backingmapptr = &kmem_map,
};

void	*pool_subpage_alloc_nointr(struct pool *, int);
void	pool_subpage_free_nointr(struct pool *, void *);

struct pool_allocator pool_allocator_nointr = {
	pool_subpage_alloc, pool_subpage_free, POOL_SUBPAGE,
	.pa_backingmapptr = &kmem_map,
};
#endif /* POOL_SUBPAGE */

static void *
pool_allocator_alloc(struct pool *pp, int flags)
{
	struct pool_allocator *pa = pp->pr_alloc;
	void *res;

	LOCK_ASSERT(!simple_lock_held(&pp->pr_slock));

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

	LOCK_ASSERT(!simple_lock_held(&pp->pr_slock));

	(*pa->pa_free)(pp, v);
}

void *
pool_page_alloc(struct pool *pp, int flags)
{
	bool waitok = (flags & PR_WAITOK) ? true : false;

	return ((void *) uvm_km_alloc_poolpage_cache(kmem_map, waitok));
}

void
pool_page_free(struct pool *pp, void *v)
{

	uvm_km_free_poolpage_cache(kmem_map, (vaddr_t) v);
}

static void *
pool_page_alloc_meta(struct pool *pp, int flags)
{
	bool waitok = (flags & PR_WAITOK) ? true : false;

	return ((void *) uvm_km_alloc_poolpage(kmem_map, waitok));
}

static void
pool_page_free_meta(struct pool *pp, void *v)
{

	uvm_km_free_poolpage(kmem_map, (vaddr_t) v);
}

#ifdef POOL_SUBPAGE
/* Sub-page allocator, for machines with large hardware pages. */
void *
pool_subpage_alloc(struct pool *pp, int flags)
{
	void *v;
	int s;
	s = splvm();
	v = pool_get(&psppool, flags);
	splx(s);
	return v;
}

void
pool_subpage_free(struct pool *pp, void *v)
{
	int s;
	s = splvm();
	pool_put(&psppool, v);
	splx(s);
}

/* We don't provide a real nointr allocator.  Maybe later. */
void *
pool_subpage_alloc_nointr(struct pool *pp, int flags)
{

	return (pool_subpage_alloc(pp, flags));
}

void
pool_subpage_free_nointr(struct pool *pp, void *v)
{

	pool_subpage_free(pp, v);
}
#endif /* POOL_SUBPAGE */
void *
pool_page_alloc_nointr(struct pool *pp, int flags)
{
	bool waitok = (flags & PR_WAITOK) ? true : false;

	return ((void *) uvm_km_alloc_poolpage_cache(kernel_map, waitok));
}

void
pool_page_free_nointr(struct pool *pp, void *v)
{

	uvm_km_free_poolpage_cache(kernel_map, (vaddr_t) v);
}
