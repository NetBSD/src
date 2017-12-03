/*	$NetBSD: uvm_pglist.c,v 1.62.12.2 2017/12/03 11:39:22 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: uvm_pglist.c,v 1.62.12.2 2017/12/03 11:39:22 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>

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
	int free_list __unused, color __unused, pgflidx;

	KASSERT(mutex_owned(&uvm_fpageqlock));

#if PGFL_NQUEUES != 2
#error uvm_pglistalloc needs to be updated
#endif

	free_list = uvm_page_lookup_freelist(pg);
	color = VM_PGCOLOR_BUCKET(pg);
	pgflidx = (pg->flags & PG_ZERO) ? PGFL_ZEROS : PGFL_UNKNOWN;
#ifdef UVMDEBUG
	struct vm_page *tp;
	LIST_FOREACH(tp,
	    &uvm.page_free[free_list].pgfl_buckets[color].pgfl_queues[pgflidx],
	    pageq.list) {
		if (tp == pg)
			break;
	}
	if (tp == NULL)
		panic("uvm_pglistalloc: page not on freelist");
#endif
	LIST_REMOVE(pg, pageq.list);	/* global */
	LIST_REMOVE(pg, listq.list);	/* cpu */
	uvmexp.free--;
	if (pg->flags & PG_ZERO)
		uvmexp.zeropages--;
	VM_FREE_PAGE_TO_CPU(pg)->pages[pgflidx]--;
	pg->flags = PG_CLEAN;
	pg->pqflags = 0;
	pg->uobject = NULL;
	pg->uanon = NULL;
	TAILQ_INSERT_TAIL(rlist, pg, pageq.queue);
	STAT_INCR(uvm_pglistalloc_npages);
}

static int
uvm_pglistalloc_c_ps(uvm_physseg_t psi, int num, paddr_t low, paddr_t high,
    paddr_t alignment, paddr_t boundary, struct pglist *rlist)
{
	signed int candidate, limit, candidateidx, end, idx, skip;
	int pagemask;
	bool second_pass;
#ifdef DEBUG
	paddr_t idxpa, lastidxpa;
	paddr_t cidx = 0;	/* XXX: GCC */
#endif
#ifdef PGALLOC_VERBOSE
	printf("pgalloc: contig %d pgs from psi %zd\n", num, ps - vm_physmem);
#endif

	KASSERT(mutex_owned(&uvm_fpageqlock));

	low = atop(low);
	high = atop(high);
	alignment = atop(alignment);

	/*
	 * Make sure that physseg falls within with range to be allocated from.
	 */
	if (high <= uvm_physseg_get_avail_start(psi) || low >= uvm_physseg_get_avail_end(psi))
		return 0;

	/*
	 * We start our search at the just after where the last allocation
	 * succeeded.
	 */
	candidate = roundup2(max(low, uvm_physseg_get_avail_start(psi) +
		uvm_physseg_get_start_hint(psi)), alignment);
	limit = min(high, uvm_physseg_get_avail_end(psi));
	pagemask = ~((boundary >> PAGE_SHIFT) - 1);
	skip = 0;
	second_pass = false;

	for (;;) {
		bool ok = true;
		signed int cnt;

		if (candidate + num > limit) {
			if (uvm_physseg_get_start_hint(psi) == 0 || second_pass) {
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
			candidate = roundup2(max(low, uvm_physseg_get_avail_start(psi)), alignment);
			limit = min(limit, uvm_physseg_get_avail_start(psi) +
			    uvm_physseg_get_start_hint(psi));
			skip = 0;
			continue;
		}
		if (boundary != 0 &&
		    ((candidate ^ (candidate + num - 1)) & pagemask) != 0) {
			/*
			 * Region crosses boundary. Jump to the boundary
			 * just crossed and ensure alignment.
			 */
			candidate = (candidate + num - 1) & pagemask;
			candidate = roundup2(candidate, alignment);
			skip = 0;
			continue;
		}
#ifdef DEBUG
		/*
		 * Make sure this is a managed physical page.
		 */

		if (uvm_physseg_find(candidate, &cidx) != psi)
			panic("pgalloc contig: botch1");
		if (cidx != candidate - uvm_physseg_get_start(psi))
			panic("pgalloc contig: botch2");
		if (uvm_physseg_find(candidate + num - 1, &cidx) != psi)
			panic("pgalloc contig: botch3");
		if (cidx != candidate - uvm_physseg_get_start(psi) + num - 1)
			panic("pgalloc contig: botch4");
#endif
		candidateidx = candidate - uvm_physseg_get_start(psi);
		end = candidateidx + num;

		/*
		 * Found a suitable starting page.  See if the range is free.
		 */
#ifdef PGALLOC_VERBOSE
		printf("%s: ps=%p candidate=%#x end=%#x skip=%#x, align=%#"PRIxPADDR,
		    __func__, ps, candidateidx, end, skip, alignment);
#endif
		/*
		 * We start at the end and work backwards since if we find a
		 * non-free page, it makes no sense to continue.
		 *
		 * But on the plus size we have "vetted" some number of free
		 * pages.  If this iteration fails, we may be able to skip
		 * testing most of those pages again in the next pass.
		 */
		for (idx = end - 1; idx >= candidateidx + skip; idx--) {
			if (VM_PAGE_IS_FREE(uvm_physseg_get_pg(psi, idx)) == 0) {
				ok = false;
				break;
			}

#ifdef DEBUG
			if (idx > candidateidx) {
				idxpa = VM_PAGE_TO_PHYS(uvm_physseg_get_pg(psi, idx));
				lastidxpa = VM_PAGE_TO_PHYS(uvm_physseg_get_pg(psi, idx - 1));
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
				KDASSERT(VM_PAGE_IS_FREE(uvm_physseg_get_pg(psi, candidateidx + skip)));
			}
#ifdef PGALLOC_VERBOSE
			printf(": ok\n");
#endif
			break;
		}

#ifdef PGALLOC_VERBOSE
		printf(": non-free at %#x\n", idx - candidateidx);
#endif
		/*
		 * count the number of pages we can advance
		 * since we know they aren't all free.
		 */
		cnt = idx + 1 - candidateidx;
		/*
		 * now round up that to the needed alignment.
		 */
		cnt = roundup2(cnt, alignment);
		/*
		 * The number of pages we can skip checking 
		 * (might be 0 if cnt > num).
		 */
		skip = max(num - cnt, 0);
		candidate += cnt;
	}

	/*
	 * we have a chunk of memory that conforms to the requested constraints.
	 */
	for (idx = candidateidx; idx < end; idx++)
		uvm_pglist_add(uvm_physseg_get_pg(psi, idx), rlist);

	/*
	 * the next time we need to search this segment, start after this
	 * chunk of pages we just allocated.
	 */
	uvm_physseg_set_start_hint(psi, candidate + num -
	    uvm_physseg_get_avail_start(psi));
	KASSERTMSG(uvm_physseg_get_start_hint(psi) <=
	    uvm_physseg_get_avail_end(psi) - uvm_physseg_get_avail_start(psi),
	    "%x %u (%#x) <= %#"PRIxPADDR" - %#"PRIxPADDR" (%#"PRIxPADDR")",
	    candidate + num,
	    uvm_physseg_get_start_hint(psi), uvm_physseg_get_start_hint(psi),
	    uvm_physseg_get_avail_end(psi), uvm_physseg_get_avail_start(psi),
	    uvm_physseg_get_avail_end(psi) - uvm_physseg_get_avail_start(psi));

#ifdef PGALLOC_VERBOSE
	printf("got %d pgs\n", num);
#endif
	return num; /* number of pages allocated */
}

static int
uvm_pglistalloc_contig(int num, paddr_t low, paddr_t high, paddr_t alignment,
    paddr_t boundary, struct pglist *rlist)
{
	int fl;
	int error;

	uvm_physseg_t psi;
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
		for (psi = uvm_physseg_get_last(); uvm_physseg_valid_p(psi); psi = uvm_physseg_get_prev(psi))
#else
		for (psi = uvm_physseg_get_first(); uvm_physseg_valid_p(psi); psi = uvm_physseg_get_next(psi))
#endif
		{
			if (uvm_physseg_get_free_list(psi) != fl)
				continue;

			num -= uvm_pglistalloc_c_ps(psi, num, low, high,
						    alignment, boundary, rlist);
			if (num == 0) {
#ifdef PGALLOC_VERBOSE
				printf("pgalloc: %"PRIxMAX"-%"PRIxMAX"\n",
				       (uintmax_t) VM_PAGE_TO_PHYS(TAILQ_FIRST(rlist)),
				       (uintmax_t) VM_PAGE_TO_PHYS(TAILQ_LAST(rlist, pglist)));
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
uvm_pglistalloc_s_ps(uvm_physseg_t psi, int num, paddr_t low, paddr_t high,
    struct pglist *rlist)
{
	int todo, limit, candidate;
	struct vm_page *pg;
	bool second_pass;
#ifdef PGALLOC_VERBOSE
	printf("pgalloc: simple %d pgs from psi %zd\n", num, psi);
#endif

	KASSERT(mutex_owned(&uvm_fpageqlock));
	KASSERT(uvm_physseg_get_start(psi) <= uvm_physseg_get_avail_start(psi));
	KASSERT(uvm_physseg_get_start(psi) <= uvm_physseg_get_avail_end(psi));
	KASSERT(uvm_physseg_get_avail_start(psi) <= uvm_physseg_get_end(psi));
	KASSERT(uvm_physseg_get_avail_end(psi) <= uvm_physseg_get_end(psi));

	low = atop(low);
	high = atop(high);
	todo = num;
	candidate = max(low, uvm_physseg_get_avail_start(psi) +
	    uvm_physseg_get_start_hint(psi));
	limit = min(high, uvm_physseg_get_avail_end(psi));
	pg = uvm_physseg_get_pg(psi, candidate - uvm_physseg_get_start(psi));
	second_pass = false;

	/*
	 * Make sure that physseg falls within with range to be allocated from.
	 */
	if (high <= uvm_physseg_get_avail_start(psi) ||
	    low >= uvm_physseg_get_avail_end(psi))
		return 0;

again:
	for (;; candidate++, pg++) {
		if (candidate >= limit) {
			if (uvm_physseg_get_start_hint(psi) == 0 || second_pass) {
				candidate = limit - 1;
				break;
			}
			second_pass = true;
			candidate = max(low, uvm_physseg_get_avail_start(psi));
			limit = min(limit, uvm_physseg_get_avail_start(psi) +
			    uvm_physseg_get_start_hint(psi));
			pg = uvm_physseg_get_pg(psi, candidate - uvm_physseg_get_start(psi));
			goto again;
		}
#if defined(DEBUG)
		{
			paddr_t cidx = 0;
			const uvm_physseg_t bank = uvm_physseg_find(candidate, &cidx);
			KDASSERTMSG(bank == psi,
			    "uvm_physseg_find(%#x) (%"PRIxPHYSSEG ") != psi %"PRIxPHYSSEG,
			     candidate, bank, psi);
			KDASSERTMSG(cidx == candidate - uvm_physseg_get_start(psi),
			    "uvm_physseg_find(%#x): %#"PRIxPADDR" != off %"PRIxPADDR,
			     candidate, cidx, candidate - uvm_physseg_get_start(psi));
		}
#endif
		if (VM_PAGE_IS_FREE(pg) == 0)
			continue;

		uvm_pglist_add(pg, rlist);
		if (--todo == 0) {
			break;
		}
	}

	/*
	 * The next time we need to search this segment,
	 * start just after the pages we just allocated.
	 */
	uvm_physseg_set_start_hint(psi, candidate + 1 - uvm_physseg_get_avail_start(psi));
	KASSERTMSG(uvm_physseg_get_start_hint(psi) <= uvm_physseg_get_avail_end(psi) -
	    uvm_physseg_get_avail_start(psi),
	    "%#x %u (%#x) <= %#"PRIxPADDR" - %#"PRIxPADDR" (%#"PRIxPADDR")",
	    candidate + 1,
	    uvm_physseg_get_start_hint(psi),
	    uvm_physseg_get_start_hint(psi),
	    uvm_physseg_get_avail_end(psi),
	    uvm_physseg_get_avail_start(psi),
	    uvm_physseg_get_avail_end(psi) - uvm_physseg_get_avail_start(psi));

#ifdef PGALLOC_VERBOSE
	printf("got %d pgs\n", num - todo);
#endif
	return (num - todo); /* number of pages allocated */
}

static int
uvm_pglistalloc_simple(int num, paddr_t low, paddr_t high,
    struct pglist *rlist, int waitok)
{
	int fl, error;
	uvm_physseg_t psi;

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
		for (psi = uvm_physseg_get_last(); uvm_physseg_valid_p(psi); psi = uvm_physseg_get_prev(psi))
#else
		for (psi = uvm_physseg_get_first(); uvm_physseg_valid_p(psi); psi = uvm_physseg_get_next(psi))
#endif
		{
			if (uvm_physseg_get_free_list(psi) != fl)
				continue;

			num -= uvm_pglistalloc_s_ps(psi, num, low, high, rlist);
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
		printf("pgalloc: %"PRIxMAX"..%"PRIxMAX"\n",
		       (uintmax_t) VM_PAGE_TO_PHYS(TAILQ_FIRST(rlist)),
		       (uintmax_t) VM_PAGE_TO_PHYS(TAILQ_LAST(rlist, pglist)));
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
	struct uvm_cpu *ucpu;
	struct vm_page *pg;
	int index, color, queue;
	bool iszero;

	/*
	 * Lock the free list and free each page.
	 */

	mutex_spin_enter(&uvm_fpageqlock);
	ucpu = curcpu()->ci_data.cpu_uvm;
	while ((pg = TAILQ_FIRST(list)) != NULL) {
		KASSERT(!uvmpdpol_pageisqueued_p(pg));
		TAILQ_REMOVE(list, pg, pageq.queue);
		iszero = (pg->flags & PG_ZERO);
		pg->pqflags = PQ_FREE;
#ifdef DEBUG
		pg->uobject = (void *)0xdeadbeef;
		pg->uanon = (void *)0xdeadbeef;
#endif /* DEBUG */
#ifdef DEBUG
		if (iszero)
			uvm_pagezerocheck(pg);
#endif /* DEBUG */
		index = uvm_page_lookup_freelist(pg);
		color = VM_PGCOLOR_BUCKET(pg);
		queue = iszero ? PGFL_ZEROS : PGFL_UNKNOWN;
		pg->offset = (uintptr_t)ucpu;
		LIST_INSERT_HEAD(&uvm.page_free[index].pgfl_buckets[color].
		    pgfl_queues[queue], pg, pageq.list);
		LIST_INSERT_HEAD(&ucpu->page_free[index].pgfl_buckets[color].
		    pgfl_queues[queue], pg, listq.list);
		uvmexp.free++;
		if (iszero)
			uvmexp.zeropages++;
		ucpu->pages[queue]++;
		STAT_DECR(uvm_pglistalloc_npages);
	}
	if (ucpu->pages[PGFL_ZEROS] < ucpu->pages[PGFL_UNKNOWN])
		ucpu->page_idle_zero = vm_page_zero_enable;
	mutex_spin_exit(&uvm_fpageqlock);
}
