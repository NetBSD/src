/*	$NetBSD: uvm_pglist.c,v 1.38.28.2 2007/07/21 19:21:56 ad Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
 * uvm_pglist.c: pglist functions
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_pglist.c,v 1.38.28.2 2007/07/21 19:21:56 ad Exp $");

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
	int free_list, color, pgflidx;
#ifdef DEBUG
	struct vm_page *tp;
#endif

#if PGFL_NQUEUES != 2
#error uvm_pglistalloc needs to be updated
#endif

	free_list = uvm_page_lookup_freelist(pg);
	color = VM_PGCOLOR_BUCKET(pg);
	pgflidx = (pg->flags & PG_ZERO) ? PGFL_ZEROS : PGFL_UNKNOWN;
#ifdef DEBUG
	for (tp = TAILQ_FIRST(&uvm.page_free[
		free_list].pgfl_buckets[color].pgfl_queues[pgflidx]);
	     tp != NULL;
	     tp = TAILQ_NEXT(tp, pageq)) {
		if (tp == pg)
			break;
	}
	if (tp == NULL)
		panic("uvm_pglistalloc: page not on freelist");
#endif
	TAILQ_REMOVE(&uvm.page_free[free_list].pgfl_buckets[
			color].pgfl_queues[pgflidx], pg, pageq);
	uvmexp.free--;
	if (pg->flags & PG_ZERO)
		uvmexp.zeropages--;
	pg->flags = PG_CLEAN;
	pg->pqflags = 0;
	pg->uobject = NULL;
	pg->uanon = NULL;
	TAILQ_INSERT_TAIL(rlist, pg, pageq);
	STAT_INCR(uvm_pglistalloc_npages);
}

static int
uvm_pglistalloc_c_ps(struct vm_physseg *ps, int num, paddr_t low, paddr_t high,
    paddr_t alignment, paddr_t boundary, struct pglist *rlist)
{
	int try, limit, tryidx, end, idx;
	struct vm_page *pgs;
	int pagemask;
#ifdef DEBUG
	paddr_t idxpa, lastidxpa;
	int cidx = 0;	/* XXX: GCC */
#endif
#ifdef PGALLOC_VERBOSE
	printf("pgalloc: contig %d pgs from psi %ld\n", num,
	(long)(ps - vm_physmem));
#endif

	try = roundup(max(atop(low), ps->avail_start), atop(alignment));
	limit = min(atop(high), ps->avail_end);
	pagemask = ~((boundary >> PAGE_SHIFT) - 1);

	for (;;) {
		if (try + num > limit) {
			/*
			 * We've run past the allowable range.
			 */
			return (0); /* FAIL */
		}
		if (boundary != 0 &&
		    ((try ^ (try + num - 1)) & pagemask) != 0) {
			/*
			 * Region crosses boundary. Jump to the boundary
			 * just crossed and ensure alignment.
			 */
			try = (try + num - 1) & pagemask;
			try = roundup(try, atop(alignment));
			continue;
		}
#ifdef DEBUG
		/*
		 * Make sure this is a managed physical page.
		 */

		if (vm_physseg_find(try, &cidx) != ps - vm_physmem)
			panic("pgalloc contig: botch1");
		if (cidx != try - ps->start)
			panic("pgalloc contig: botch2");
		if (vm_physseg_find(try + num - 1, &cidx) != ps - vm_physmem)
			panic("pgalloc contig: botch3");
		if (cidx != try - ps->start + num - 1)
			panic("pgalloc contig: botch4");
#endif
		tryidx = try - ps->start;
		end = tryidx + num;
		pgs = ps->pgs;

		/*
		 * Found a suitable starting page.  See if the range is free.
		 */
		for (idx = tryidx; idx < end; idx++) {
			if (VM_PAGE_IS_FREE(&pgs[idx]) == 0)
				break;

#ifdef DEBUG
			idxpa = VM_PAGE_TO_PHYS(&pgs[idx]);
			if (idx > tryidx) {
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
		if (idx == end)
			break;

		try += atop(alignment);
	}

	/*
	 * we have a chunk of memory that conforms to the requested constraints.
	 */
	idx = tryidx;
	while (idx < end)
		uvm_pglist_add(&pgs[idx++], rlist);

#ifdef PGALLOC_VERBOSE
	printf("got %d pgs\n", num);
#endif
	return (num); /* number of pages allocated */
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
	int todo, limit, try;
	struct vm_page *pg;
#ifdef DEBUG
	int cidx = 0;	/* XXX: GCC */
#endif
#ifdef PGALLOC_VERBOSE
	printf("pgalloc: simple %d pgs from psi %ld\n", num,
	    (long)(ps - vm_physmem));
#endif

	todo = num;
	limit = min(atop(high), ps->avail_end);

	for (try = max(atop(low), ps->avail_start);
	     try < limit; try ++) {
#ifdef DEBUG
		if (vm_physseg_find(try, &cidx) != ps - vm_physmem)
			panic("pgalloc simple: botch1");
		if (cidx != (try - ps->start))
			panic("pgalloc simple: botch2");
#endif
		pg = &ps->pgs[try - ps->start];
		if (VM_PAGE_IS_FREE(pg) == 0)
			continue;

		uvm_pglist_add(pg, rlist);
		if (--todo == 0)
			break;
	}

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
	low = roundup(low, alignment);

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
	while ((pg = TAILQ_FIRST(list)) != NULL) {
		bool iszero;

		KASSERT(!uvmpdpol_pageisqueued_p(pg));
		TAILQ_REMOVE(list, pg, pageq);
		iszero = (pg->flags & PG_ZERO);
		pg->pqflags = PQ_FREE;
#ifdef DEBUG
		pg->uobject = (void *)0xdeadbeef;
		pg->offset = 0xdeadbeef;
		pg->uanon = (void *)0xdeadbeef;
#endif /* DEBUG */
#ifdef DEBUG
		if (iszero)
			uvm_pagezerocheck(pg);
#endif /* DEBUG */
		TAILQ_INSERT_HEAD(&uvm.page_free[uvm_page_lookup_freelist(pg)].
		    pgfl_buckets[VM_PGCOLOR_BUCKET(pg)].
		    pgfl_queues[iszero ? PGFL_ZEROS : PGFL_UNKNOWN], pg, pageq);
		uvmexp.free++;
		if (iszero)
			uvmexp.zeropages++;
		if (uvmexp.zeropages < UVM_PAGEZERO_TARGET)
			uvm.page_idle_zero = vm_page_zero_enable;
		STAT_DECR(uvm_pglistalloc_npages);
	}
	mutex_spin_exit(&uvm_fpageqlock);
}
