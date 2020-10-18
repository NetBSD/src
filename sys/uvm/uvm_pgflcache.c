/*	$NetBSD: uvm_pgflcache.c,v 1.6 2020/10/18 18:31:31 chs Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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
 * uvm_pgflcache.c: page freelist cache.
 *
 * This implements a tiny per-CPU cache of pages that sits between the main
 * page allocator and the freelists.  By allocating and freeing pages in
 * batch, it reduces freelist contention by an order of magnitude.
 *
 * The cache can be paused & resumed at runtime so that UVM_HOTPLUG,
 * uvm_pglistalloc() and uvm_page_redim() can have a consistent view of the
 * world.  On system with one CPU per physical package (e.g. a uniprocessor)
 * the cache is not enabled.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_pgflcache.c,v 1.6 2020/10/18 18:31:31 chs Exp $");

#include "opt_uvm.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sched.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/xcall.h>

#include <uvm/uvm.h>
#include <uvm/uvm_pglist.h>
#include <uvm/uvm_pgflcache.h>

/* There is no point doing any of this on a uniprocessor. */
#ifdef MULTIPROCESSOR

/*
 * MAXPGS - maximum pages per color, per bucket.
 * FILLPGS - number of pages to allocate at once, per color, per bucket.
 *
 * Why the chosen values:
 *
 * (1) In 2019, an average Intel system has 4kB pages and 8x L2 cache
 * colors.  We make the assumption that most of the time allocation activity
 * will be centered around one UVM freelist, so most of the time there will
 * be no more than 224kB worth of cached pages per-CPU.  That's tiny, but
 * enough to hugely reduce contention on the freelist locks, and give us a
 * small pool of pages which if we're very lucky may have some L1/L2 cache
 * locality, and do so without subtracting too much from the L2/L3 cache
 * benefits of having per-package free lists in the page allocator.
 *
 * (2) With the chosen values on _LP64, the data structure for each color
 * takes up a single cache line (64 bytes) giving this very low overhead
 * even in the "miss" case.
 *
 * (3) We don't want to cause too much pressure by hiding away memory that
 * could otherwise be put to good use.
 */
#define	MAXPGS		7
#define	FILLPGS		6

/* Variable size, according to # colors. */
struct pgflcache {
	struct pccolor {
		intptr_t	count;
		struct vm_page	*pages[MAXPGS];
	} color[1];
};

static kmutex_t		uvm_pgflcache_lock;
static int		uvm_pgflcache_sem;

/*
 * uvm_pgflcache_fill: fill specified freelist/color from global list
 *
 * => must be called at IPL_VM
 * => must be called with given bucket lock held
 * => must only fill from the correct bucket for this CPU
 */

void
uvm_pgflcache_fill(struct uvm_cpu *ucpu, int fl, int b, int c)
{
	struct pgflbucket *pgb;
	struct pgflcache *pc;
	struct pccolor *pcc;
	struct pgflist *head;
	struct vm_page *pg;
	int count;

	KASSERT(mutex_owned(&uvm_freelist_locks[b].lock));
	KASSERT(ucpu->pgflbucket == b);

	/* If caching is off, then bail out. */
	if (__predict_false((pc = ucpu->pgflcache[fl]) == NULL)) {
		return;
	}

	/* Fill only to the limit. */
	pcc = &pc->color[c];
	pgb = uvm.page_free[fl].pgfl_buckets[b];
	head = &pgb->pgb_colors[c];
	if (pcc->count >= FILLPGS) {
		return;
	}

	/* Pull pages from the bucket until it's empty, or we are full. */
	count = pcc->count;
	pg = LIST_FIRST(head);
	while (__predict_true(pg != NULL && count < FILLPGS)) {
		KASSERT(pg->flags & PG_FREE);
		KASSERT(uvm_page_get_bucket(pg) == b);
		pcc->pages[count++] = pg;
		pg = LIST_NEXT(pg, pageq.list);
	}

	/* Violate LIST abstraction to remove all pages at once. */
	head->lh_first = pg;
	if (__predict_true(pg != NULL)) {
		pg->pageq.list.le_prev = &head->lh_first;
	}
	pgb->pgb_nfree -= (count - pcc->count);
	CPU_COUNT(CPU_COUNT_FREEPAGES, -(count - pcc->count));
	pcc->count = count;
}

/*
 * uvm_pgflcache_spill: spill specified freelist/color to global list
 *
 * => must be called at IPL_VM
 * => mark __noinline so we don't pull it into uvm_pgflcache_free()
 */

static void __noinline
uvm_pgflcache_spill(struct uvm_cpu *ucpu, int fl, int c)
{
	struct pgflbucket *pgb;
	struct pgfreelist *pgfl;
	struct pgflcache *pc;
	struct pccolor *pcc;
	struct pgflist *head;
	kmutex_t *lock;
	int b, adj;

	pc = ucpu->pgflcache[fl];
	pcc = &pc->color[c];
	pgfl = &uvm.page_free[fl];
	b = ucpu->pgflbucket;
	pgb = pgfl->pgfl_buckets[b];
	head = &pgb->pgb_colors[c];
	lock = &uvm_freelist_locks[b].lock;

	mutex_spin_enter(lock);
	for (adj = pcc->count; pcc->count != 0;) {
		pcc->count--;
		KASSERT(pcc->pages[pcc->count] != NULL);
		KASSERT(pcc->pages[pcc->count]->flags & PG_FREE);
		LIST_INSERT_HEAD(head, pcc->pages[pcc->count], pageq.list);
	}
	pgb->pgb_nfree += adj;
	CPU_COUNT(CPU_COUNT_FREEPAGES, adj);
	mutex_spin_exit(lock);
}

/*
 * uvm_pgflcache_alloc: try to allocate a cached page.
 *
 * => must be called at IPL_VM
 * => allocate only from the given freelist and given page color
 */

struct vm_page *
uvm_pgflcache_alloc(struct uvm_cpu *ucpu, int fl, int c)
{
	struct pgflcache *pc;
	struct pccolor *pcc;
	struct vm_page *pg;

	/* If caching is off, then bail out. */
	if (__predict_false((pc = ucpu->pgflcache[fl]) == NULL)) {
		return NULL;
	}

	/* Very simple: if we have a page then return it. */
	pcc = &pc->color[c];
	if (__predict_false(pcc->count == 0)) {
		return NULL;
	}
	pg = pcc->pages[--(pcc->count)];
	KASSERT(pg != NULL);
	KASSERT(pg->flags == PG_FREE);
	KASSERT(uvm_page_get_freelist(pg) == fl);
	KASSERT(uvm_page_get_bucket(pg) == ucpu->pgflbucket);
	pg->flags = PG_BUSY | PG_CLEAN | PG_FAKE;
	return pg;
}

/*
 * uvm_pgflcache_free: cache a page, if possible.
 *
 * => must be called at IPL_VM
 * => must only send pages for the correct bucket for this CPU
 */

bool
uvm_pgflcache_free(struct uvm_cpu *ucpu, struct vm_page *pg)
{
	struct pgflcache *pc;
	struct pccolor *pcc;
	int fl, c;

	KASSERT(uvm_page_get_bucket(pg) == ucpu->pgflbucket);

	/* If caching is off, then bail out. */
 	fl = uvm_page_get_freelist(pg);
	if (__predict_false((pc = ucpu->pgflcache[fl]) == NULL)) {
		return false;
	}

	/* If the array is full spill it first, then add page to array. */
	c = VM_PGCOLOR(pg);
	pcc = &pc->color[c];
	KASSERT((pg->flags & PG_FREE) == 0);
	if (__predict_false(pcc->count == MAXPGS)) {
		uvm_pgflcache_spill(ucpu, fl, c);
	}
	pg->flags = PG_FREE;
	pcc->pages[pcc->count] = pg;
	pcc->count++;
	return true;
}

/*
 * uvm_pgflcache_init: allocate and initialize per-CPU data structures for
 * the free page cache.  Don't set anything in motion - that's taken care
 * of by uvm_pgflcache_resume().
 */

static void
uvm_pgflcache_init_cpu(struct cpu_info *ci)
{
	struct uvm_cpu *ucpu;
	size_t sz;

	ucpu = ci->ci_data.cpu_uvm;
	KASSERT(ucpu->pgflcachemem == NULL);
	KASSERT(ucpu->pgflcache[0] == NULL);

	sz = offsetof(struct pgflcache, color[uvmexp.ncolors]);
	ucpu->pgflcachememsz =
	    (roundup2(sz * VM_NFREELIST, coherency_unit) + coherency_unit - 1);
	ucpu->pgflcachemem = kmem_zalloc(ucpu->pgflcachememsz, KM_SLEEP);
}

/*
 * uvm_pgflcache_fini_cpu: dump all cached pages back to global free list
 * and shut down caching on the CPU.  Called on each CPU in the system via
 * xcall.
 */

static void
uvm_pgflcache_fini_cpu(void *arg1 __unused, void *arg2 __unused)
{
	struct uvm_cpu *ucpu;
	int fl, color, s;

	ucpu = curcpu()->ci_data.cpu_uvm;
	for (fl = 0; fl < VM_NFREELIST; fl++) {
		s = splvm();
		for (color = 0; color < uvmexp.ncolors; color++) {
			uvm_pgflcache_spill(ucpu, fl, color);
		}
		ucpu->pgflcache[fl] = NULL;
		splx(s);
	}
}

/*
 * uvm_pgflcache_pause: pause operation of the caches
 */

void
uvm_pgflcache_pause(void)
{
	uint64_t where;

	/* First one in starts draining.  Everyone else waits. */
	mutex_enter(&uvm_pgflcache_lock);
	if (uvm_pgflcache_sem++ == 0) {
		where = xc_broadcast(XC_HIGHPRI, uvm_pgflcache_fini_cpu,
		    (void *)1, NULL);
		xc_wait(where);
	}
	mutex_exit(&uvm_pgflcache_lock);
}

/*
 * uvm_pgflcache_resume: resume operation of the caches
 */

void
uvm_pgflcache_resume(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	struct uvm_cpu *ucpu;
	uintptr_t addr;
	size_t sz;
	int fl;

	/* Last guy out takes care of business. */
	mutex_enter(&uvm_pgflcache_lock);
	KASSERT(uvm_pgflcache_sem > 0);
	if (uvm_pgflcache_sem-- > 1) {
		mutex_exit(&uvm_pgflcache_lock);
		return;
	}

	/*
	 * Make sure dependant data structure updates are remotely visible.
	 * Essentially this functions as a global memory barrier.
	 */
	xc_barrier(XC_HIGHPRI);

	/*
	 * Then set all of the pointers in place on each CPU.  As soon as
	 * each pointer is set, caching is operational in that dimension.
	 */
	sz = offsetof(struct pgflcache, color[uvmexp.ncolors]);
	for (CPU_INFO_FOREACH(cii, ci)) {
		ucpu = ci->ci_data.cpu_uvm;
		addr = roundup2((uintptr_t)ucpu->pgflcachemem, coherency_unit);
		for (fl = 0; fl < VM_NFREELIST; fl++) {
			ucpu->pgflcache[fl] = (struct pgflcache *)addr;
			addr += sz;
		}
	}
	mutex_exit(&uvm_pgflcache_lock);
}

/*
 * uvm_pgflcache_start: start operation of the cache.
 *
 * => called once only, when init(8) is about to be started
 */

void
uvm_pgflcache_start(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	KASSERT(uvm_pgflcache_sem > 0);

	/*
	 * There's not much point doing this if every CPU has its own
	 * bucket (and that includes the uniprocessor case).
	 */
	if (ncpu == uvm.bucketcount) {
		return;
	}

	/* Create data structures for each CPU. */
	for (CPU_INFO_FOREACH(cii, ci)) {
		uvm_pgflcache_init_cpu(ci);
	}

	/* Kick it into action. */
	uvm_pgflcache_resume();
}

/*
 * uvm_pgflcache_init: set up data structures for the free page cache.
 */

void
uvm_pgflcache_init(void)
{

	uvm_pgflcache_sem = 1;
	mutex_init(&uvm_pgflcache_lock, MUTEX_DEFAULT, IPL_NONE);
}

#else	/* MULTIPROCESSOR */

struct vm_page *
uvm_pgflcache_alloc(struct uvm_cpu *ucpu, int fl, int c)
{

	return NULL;
}

bool
uvm_pgflcache_free(struct uvm_cpu *ucpu, struct vm_page *pg)
{

	return false;
}

void
uvm_pgflcache_fill(struct uvm_cpu *ucpu, int fl, int b, int c)
{

}

void
uvm_pgflcache_pause(void)
{

}

void
uvm_pgflcache_resume(void)
{

}

void
uvm_pgflcache_start(void)
{

}

void
uvm_pgflcache_init(void)
{

}

#endif	/* MULTIPROCESSOR */
