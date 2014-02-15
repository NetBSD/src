/*	$NetBSD: uvm_pglist.c,v 1.42.16.14 2014/02/15 10:19:14 matt Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * uvm_pglist.c: pglist functions
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_pglist.c,v 1.42.16.14 2014/02/15 10:19:14 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <uvm/uvm.h>
#include <uvm/uvm_pdpolicy.h>

#ifdef VM_PAGE_ALLOC_MEMORY_STATS
#define	STAT_INCR(v)	(v)++
#define	STAT_DECR(v)	do { \
		if ((v) == 0) \
			printf("%s:%d -- Already 0!\n", __FILE__, __LINE__); \
		else \
			(v)--; \
	} while (/*CONSTCOND*/ 0)
u_long	uvm_pglistalloc_npages;
#else
#define	STAT_INCR(v)
#define	STAT_DECR(v)
#endif

/*
 * uvm_pglistalloc: allocate a list of pages
 *
 * => allocated pages are placed onto an rlist.  rlist is
 *    initialized by uvm_pglistalloc.
 * => returns 0 on success or errno on failure
 * => implementation allocates a single segment if any constraints are
 *	imposed by call arguments.
 * => doesn't take into account clean non-busy pages on inactive list
 *	that could be used(?)
 * => params:
 *	size		the size of the allocation, rounded to page size.
 *	low		the low address of the allowed allocation range.
 *	high		the high address of the allowed allocation range.
 *	alignment	memory must be aligned to this power-of-two boundary.
 *	boundary	no segment in the allocation may cross this
 *			power-of-two boundary (relative to zero).
 */

static void
uvm_pglist_add(struct vm_page *pg, struct pglist *rlist)
{
	struct uvm_cpu *ucpu;
	int free_list, color, queue;

	KASSERT(mutex_owned(&uvm_fpageqlock));
	KASSERT(pg->pqflags & PQ_FREE);

#if PGFL_NQUEUES != 2
#error uvm_pglistalloc needs to be updated
#endif

	ucpu = VM_FREE_PAGE_TO_CPU(pg);
#ifndef MULTIPROCESSOR
	KASSERT(ucpu == uvm.cpus);
#endif
	free_list = uvm_page_lookup_freelist(pg);
	color = VM_PGCOLOR_BUCKET(pg);
	queue = (pg->flags & PG_ZERO) ? PGFL_ZEROS : PGFL_UNKNOWN;
#if defined(DEBUG) && DEBUG > 1
	struct vm_page *tp;
	LIST_FOREACH(tp,
	    &uvm.page_free[color].pgfl_queues[free_list][queue],
	    pageq.list) {
		if (tp == pg)
			break;
	}
	if (tp == NULL)
		panic("uvm_pglistalloc: page not on freelist");
#endif
#ifndef MULTIPROCESSOR
	KASSERT(LIST_NEXT(pg, pageq.list) == LIST_NEXT(pg, listq.list));
	KASSERT(LIST_FIRST(&uvm.page_free[color].pgfl_queues[free_list][queue]) == LIST_FIRST(&ucpu->page_free[color].pgfl_queues[free_list][queue]));
#endif
	LIST_REMOVE(pg, pageq.list);	/* global */
	LIST_REMOVE(pg, listq.list);	/* cpu */
	uvm.page_free[color].pgfl_pages[queue]--;
	ucpu->page_free[color].pgfl_pages[queue]--;
	ucpu->pages[queue]--;
	uvm.page_free[color].pgfl_pggroups[free_list]->pgrp_free--;
	uvmexp.free--;
	if (pg->flags & PG_ZERO)
		uvmexp.zeropages--;
	pg->flags = PG_CLEAN;
	pg->pqflags = 0;
	pg->uobject = NULL;
	pg->uanon = NULL;
	TAILQ_INSERT_TAIL(rlist, pg, pageq.queue);
	STAT_INCR(uvm_pglistalloc_npages);
}

static int
uvm_pglistalloc_c_ps(struct vm_physseg *ps, int num, paddr_t low, paddr_t high,
    paddr_t alignment, paddr_t boundary, struct pglist *rlist)
{
	signed int try, limit, tryidx, end, idx, skip;
	struct vm_page *pgs;
	int pagemask;
	bool second_pass;
#ifdef DEBUG
	paddr_t idxpa, lastidxpa;
	int cidx = 0;	/* XXX: GCC */
#endif
#ifdef PGALLOC_VERBOSE
	printf("pgalloc: contig %d pgs from psi %ld\n", num,
	(long)(ps - vm_physmem));
#endif

	KASSERT(mutex_owned(&uvm_fpageqlock));

	low = atop(low);
	high = atop(high);
	alignment = atop(alignment);

	/*
	 * We start our search at the just after where the last allocation
	 * succeeded.
	 */
	try = roundup2(max(low, ps->avail_start + ps->start_hint), alignment);
	limit = min(high, ps->avail_end);
	pagemask = ~((boundary >> PAGE_SHIFT) - 1);
	skip = 0;
	second_pass = false;
	pgs = ps->pgs;

	for (;;) {
		bool ok = true;
		signed int cnt;

		if (try + num > limit) {
			if (ps->start_hint == 0 || second_pass) {
				/*
				 * We've run past the allowable range.
				 */
				return 0; /* FAIL = 0 pages*/
			}
			/*
			 * We've wrapped around the end of this segment
			 * so restart at the beginning but now our limit
			 * is were we started.
			 */
			second_pass = true;
			try = roundup2(max(low, ps->avail_start), alignment);
			limit = min(high, ps->avail_start + ps->start_hint);
			if (limit >= ps->avail_end)
				return 0;
			skip = 0;
			continue;
		}
		if (boundary != 0 &&
		    ((try ^ (try + num - 1)) & pagemask) != 0) {
			/*
			 * Region crosses boundary. Jump to the boundary
			 * just crossed and ensure alignment.
			 */
			try = (try + num - 1) & pagemask;
			try = roundup2(try, alignment);
			skip = 0;
			continue;
		}

		/*
		 * Make sure this is a managed physical page.
		 */
		KDASSERTMSG(vm_physseg_find(try, &cidx) == ps - vm_physmem,
		    "%s: %s(%#x, &cidx) (%d) != ps - vm_physmem (%zd)",
		     __func__, "vm_physseg_find", try,
		    vm_physseg_find(try, &cidx), ps - vm_physmem);

		KDASSERTMSG(cidx == try - ps->start,
		    "%s: cidx (%#x) != try (%#x) - ps->start (%#"PRIxPADDR")",
		    __func__, cidx, try, ps->start);

		KDASSERTMSG(vm_physseg_find(try + num - 1, &cidx) == ps - vm_physmem,
		    "%s: %s(%#x + %#x - 1, &cidx) (%d) != ps - vm_physmem (%zd)",
		    __func__, "vm_physseg_find", try, num,
		    vm_physseg_find(try, &cidx), ps - vm_physmem);

		KDASSERTMSG(cidx == try - ps->start + num - 1,
		    "%s: cidx (%#x) != try (%#x) - ps->start (%#"PRIxPADDR") + num (%#x) - 1",
		    __func__, cidx, try, ps->start, num);

		tryidx = try - ps->start;
		end = tryidx + num;

		/*
		 * Found a suitable starting page.  See if the range is free.
		 */
#ifdef PGALLOC_VERBOSE
		printf("%s: ps=%p try=%#x end=%#x skip=%#x, align=%#x",
		    __func__, ps, tryidx, end, skip, alignment);
#endif
		/*
		 * We start at the end and work backwards since if we find a
		 * non-free page, it makes no sense to continue.
		 *
		 * But on the plus size we have "vetted" some number of free
		 * pages.  If this iteration fails, we may be able to skip
		 * testing most of those pages again in the next pass.
		 */
		for (idx = end - 1; idx >= tryidx + skip; idx--) {
			if (VM_PAGE_IS_FREE(&pgs[idx]) == 0) {
				ok = false;
				break;
			}

#ifdef DEBUG
			if (idx > tryidx) {
				idxpa = VM_PAGE_TO_PHYS(&pgs[idx]);
				lastidxpa = VM_PAGE_TO_PHYS(&pgs[idx - 1]);
				if ((lastidxpa + PAGE_SIZE) != idxpa) {
					/*
					 * Region not contiguous.
					 */
					panic("pgalloc contig: botch5");
				}
				if (boundary != 0 &&
				    ((lastidxpa ^ idxpa) & ~(boundary - 1))
				    != 0) {
					/*
					 * Region crosses boundary.
					 */
					panic("pgalloc contig: botch6");
				}
			}
#endif
		}

		if (ok) {
			while (skip-- > 0) {
				KDASSERT(VM_PAGE_IS_FREE(&pgs[tryidx + skip]));
			}
#ifdef PGALLOC_VERBOSE
			printf(": ok\n");
#endif
			break;
		}

#ifdef PGALLOC_VERBOSE
		printf(": non-free at %#x\n", idx - tryidx);
#endif
		/*
		 * count the number of pages we can advance
		 * since we know they aren't all free.
		 */
		cnt = idx + 1 - tryidx;
		/*
		 * now round up that to the needed alignment.
		 */
		cnt = roundup2(cnt, alignment);
		/*
		 * The number of pages we can skip checking 
		 * (might be 0 if cnt > num).
		 */
		skip = max(num - cnt, 0);
		try += cnt;
	}

	/*
	 * we have a chunk of memory that conforms to the requested constraints.
	 */
	for (idx = tryidx, pgs += idx; idx < end; idx++, pgs++)
		uvm_pglist_add(pgs, rlist);

	/*
	 * the next time we need to search this segment, start after this
	 * chunk of pages we just allocated.
	 */
	ps->start_hint = tryidx + num;

#ifdef PGALLOC_VERBOSE
	printf("got %d pgs\n", num);
#endif
	return num; /* number of pages allocated */
}

static int
uvm_pglistalloc_contig(int num, paddr_t low, paddr_t high, paddr_t alignment,
    paddr_t boundary, struct pglist *rlist)
{
	int fl, psi;
	struct vm_physseg *ps;
	int error;

	/* Default to "lose". */
	error = ENOMEM;

	/*
	 * Block all memory allocation and lock the free list.
	 */
	mutex_spin_enter(&uvm_fpageqlock);

	/* Are there even any free pages? */
	if (uvmexp.free <= (uvmexp.reserve_pagedaemon + uvmexp.reserve_kernel))
		goto out;

	for (fl = 0; fl < VM_NFREELIST; fl++) {
#if (VM_PHYSSEG_STRAT == VM_PSTRAT_BIGFIRST)
		for (psi = vm_nphysseg - 1 ; psi >= 0 ; psi--)
#else
		for (psi = 0 ; psi < vm_nphysseg ; psi++)
#endif
		{
			ps = &vm_physmem[psi];

			if (ps->free_list != fl)
				continue;

			num -= uvm_pglistalloc_c_ps(ps, num, low, high,
						    alignment, boundary, rlist);
			if (num == 0) {
#ifdef PGALLOC_VERBOSE
				printf("pgalloc: %lx-%lx\n",
				       VM_PAGE_TO_PHYS(TAILQ_FIRST(rlist)),
				       VM_PAGE_TO_PHYS(TAILQ_LAST(rlist)));
#endif
				error = 0;
				goto out;
			}
		}
	}

out:
	/*
	 * check to see if we need to generate some free pages waking
	 * the pagedaemon.
	 */

	uvm_kick_pdaemon();
	mutex_spin_exit(&uvm_fpageqlock);
	return (error);
}

static int
uvm_pglistalloc_s_ps(struct vm_physseg *ps, int num, paddr_t low, paddr_t high,
    struct pglist *rlist)
{
	int todo, limit, try, color;
	struct vm_page *pg;
	bool second_pass, colorless;
	const int colormask = uvmexp.colormask;
#ifdef DEBUG
	int cidx = 0;	/* XXX: GCC */
#endif
#ifdef PGALLOC_VERBOSE
	printf("pgalloc: simple %d pgs from psi %ld\n", num,
	    (long)(ps - vm_physmem));
#endif

	KASSERT(mutex_owned(&uvm_fpageqlock));

	/*
	 * If the pageq (rlist) is empty, then no pages have been allocated
	 * and we can start with any color.  If it wasn't empty, then the
	 * starting color is the last page's next color.
	 */
	colorless = TAILQ_EMPTY(rlist);
	color = (ps->avail_start + ps->start_hint) & colormask;
#ifdef DIAGNOSTIC
	if (!colorless) {
		pg = TAILQ_LAST(rlist, pglist);
		KASSERT(color == ((VM_PGCOLOR_BUCKET(pg) + 1) & colormask));
	}
#endif

	low = atop(low);
	high = atop(high);
	todo = num;
	try = max(low, ps->avail_start + ps->start_hint);
	limit = min(high, ps->avail_end);
	pg = &ps->pgs[try - ps->start];
	second_pass = false;

	for (;; try++, pg++) {
		KDASSERTMSG(limit <= ps->avail_end,
		    "%s: limit (%#x) > ps->avail_end (%#"PRIxPADDR")",
		    __func__, limit, ps->avail_end);

		if (try >= limit) {
			if (ps->start_hint == 0 || second_pass)
				break;
			second_pass = true;
			try = max(low, ps->avail_start) - 1;
			limit = min(high, ps->avail_start + ps->start_hint);
			if (limit >= ps->avail_end)
				break;
			pg = &ps->pgs[try - ps->start];
			continue;
		}

		KDASSERTMSG(vm_physseg_find(try, &cidx) == ps - vm_physmem,
		    "%s: %s(%#x, &cidx) (%d) != ps - vm_physmem (%zd)",
		    __func__, "vm_physseg_find", try,
		    vm_physseg_find(try, &cidx), ps - vm_physmem);

		KDASSERTMSG(cidx == try - ps->start,
		    "%s: cidx (%#x) != try (%#x) - ps->start (%#"PRIxPADDR")",
		    __func__, cidx, try, ps->start);

		/*
		 * If this page isn't free, then we need to skip a colors worth
		 * of pages to get a matching color.  Note that colormask is 1
		 * less than what we need since the loop will also increment
		 * try and pgs.
		 */
		if (VM_PAGE_IS_FREE(pg) == 0) {
			try += colormask;
			pg += colormask;
			continue;
		}

		/*
		 * If this page doesn't have the right color, figure out how
		 * many pages we need to skip until we get to the right color.
		 * Note that skip is 1 less that what we need since the loop
		 * will also increment try and pgs.
		 */
		if (!colorless && (try & colormask) != color) {
			const int skip = (color - (try + 1)) & colormask;
			try += skip;
			pg += skip;
			continue;
		}

		uvm_pglist_add(pg, rlist);
		if (--todo == 0) {
			break;
		}

		/*
		 * Advance the color (use try instead of color in case we
		 * haven't set an initial color).
		 */
		color = (try + 1) & colormask;
		colorless = false;
	}

	/*
	 * The next time we need to search this segment,
	 * start just after the pages we just allocated.
	 */
	ps->start_hint = try + 1 - ps->start;

#ifdef PGALLOC_VERBOSE
	printf("got %d pgs\n", num - todo);
#endif
	return (num - todo); /* number of pages allocated */
}

static int
uvm_pglistalloc_simple(int num, paddr_t low, paddr_t high,
    struct pglist *rlist, int waitok)
{
	int fl, psi, error;
	struct vm_physseg *ps;

	/* Default to "lose". */
	error = ENOMEM;

again:
	/*
	 * Block all memory allocation and lock the free list.
	 */
	mutex_spin_enter(&uvm_fpageqlock);

	/* Are there even any free pages? */
	if (uvmexp.free <= (uvmexp.reserve_pagedaemon + uvmexp.reserve_kernel))
		goto out;

	for (fl = 0; fl < VM_NFREELIST; fl++) {
#if (VM_PHYSSEG_STRAT == VM_PSTRAT_BIGFIRST)
		for (psi = vm_nphysseg - 1 ; psi >= 0 ; psi--)
#else
		for (psi = 0 ; psi < vm_nphysseg ; psi++)
#endif
		{
			ps = &vm_physmem[psi];

			if (ps->free_list != fl)
				continue;

			num -= uvm_pglistalloc_s_ps(ps, num, low, high, rlist);
			if (num == 0) {
				error = 0;
				goto out;
			}
		}

	}

out:
	/*
	 * check to see if we need to generate some free pages waking
	 * the pagedaemon.
	 */

	uvm_kick_pdaemon();
	mutex_spin_exit(&uvm_fpageqlock);

	if (error) {
		if (waitok) {
			/* XXX perhaps some time limitation? */
#ifdef DEBUG
			printf("pglistalloc waiting\n");
#endif
			uvm_wait("pglalloc");
			goto again;
		} else
			uvm_pglistfree(rlist);
	}
#ifdef PGALLOC_VERBOSE
	if (!error)
		printf("pgalloc: %lx..%lx\n",
		       VM_PAGE_TO_PHYS(TAILQ_FIRST(rlist)),
		       VM_PAGE_TO_PHYS(TAILQ_LAST(rlist, pglist)));
#endif
	return (error);
}

int
uvm_pglistalloc(psize_t size, paddr_t low, paddr_t high, paddr_t alignment,
    paddr_t boundary, struct pglist *rlist, int nsegs, int waitok)
{
	int num, res;

	KASSERT((alignment & (alignment - 1)) == 0);
	KASSERT((boundary & (boundary - 1)) == 0);

	/*
	 * Our allocations are always page granularity, so our alignment
	 * must be, too.
	 */
	if (alignment < PAGE_SIZE)
		alignment = PAGE_SIZE;
	if (boundary != 0 && boundary < size)
		return (EINVAL);
	num = atop(round_page(size));
	low = roundup2(low, alignment);

	TAILQ_INIT(rlist);

	if ((nsegs < size >> PAGE_SHIFT) || (alignment != PAGE_SIZE) ||
	    (boundary != 0))
		res = uvm_pglistalloc_contig(num, low, high, alignment,
					     boundary, rlist);
	else
		res = uvm_pglistalloc_simple(num, low, high, rlist, waitok);

	return (res);
}

/*
 * uvm_pglistfree: free a list of pages
 *
 * => pages should already be unmapped
 */

void
uvm_pglistfree(struct pglist *list)
{
	struct vm_page *pg;

	/*
	 * Lock the free list and free each page.
	 */

	mutex_spin_enter(&uvm_fpageqlock);
	struct uvm_cpu * const ucpu = curcpu()->ci_data.cpu_uvm;
#ifndef MULTIPROCESSOR
	KASSERT(ucpu == uvm.cpus);
#endif
	while ((pg = TAILQ_FIRST(list)) != NULL) {
		KASSERT(!uvmpdpol_pageisqueued_p(pg));
		TAILQ_REMOVE(list, pg, pageq.queue);
		const bool iszero = (pg->flags & PG_ZERO);
		pg->pqflags = PQ_FREE;
#ifdef DEBUG
		pg->uobject = (void *)0xdeadbeef;
		pg->uanon = (void *)0xdeadbeef;
#endif /* DEBUG */
#ifdef DEBUG
		if (iszero)
			uvm_pagezerocheck(pg);
#endif /* DEBUG */
		const size_t free_list = uvm_page_lookup_freelist(pg);
		const size_t color = VM_PGCOLOR_BUCKET(pg);
		const size_t queue = iszero ? PGFL_ZEROS : PGFL_UNKNOWN;
#ifndef MULTIPROCESSOR
		KASSERT(ucpu == uvm.cpus);
		KASSERT(LIST_FIRST(&uvm.page_free[color].pgfl_queues[free_list][queue]) == LIST_FIRST(&ucpu->page_free[color].pgfl_queues[free_list][queue]));
#endif
		pg->offset = (uintptr_t)ucpu;
		LIST_INSERT_HEAD(&uvm.page_free[color].
		    pgfl_queues[free_list][queue], pg, pageq.list);
		LIST_INSERT_HEAD(&ucpu->page_free[color].
		    pgfl_queues[free_list][queue], pg, listq.list);
#ifndef MULTIPROCESSOR
		KASSERT(LIST_FIRST(&uvm.page_free[color].pgfl_queues[free_list][queue]) == pg);
		KASSERT(LIST_FIRST(&ucpu->page_free[color].pgfl_queues[free_list][queue]) == pg);
		KASSERT(LIST_NEXT(pg, pageq.list) == LIST_NEXT(pg, listq.list));
#endif
		uvm.page_free[color].pgfl_pages[queue]++;
		ucpu->page_free[color].pgfl_pages[queue]++;
		ucpu->pages[queue]++;
		uvm.page_free[color].pgfl_pggroups[free_list]->pgrp_free++;
		uvmexp.free++;
		if (iszero)
			uvmexp.zeropages++;
		STAT_DECR(uvm_pglistalloc_npages);
	}
	for (size_t color = 0; color < uvmexp.ncolors; color++) {
		const u_long * const pages =
		    ucpu->page_free[color].pgfl_pages;
		if (pages[PGFL_ZEROS] < pages[PGFL_UNKNOWN]) {
			ucpu->page_idle_zero = vm_page_zero_enable;
			break;
		}
	}
	mutex_spin_exit(&uvm_fpageqlock);
}
