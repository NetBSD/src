/*	$NetBSD: uvm_pglist.c,v 1.90 2021/12/21 08:27:49 skrll Exp $	*/

/*-
 * Copyright (c) 1997, 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Andrew Doran.
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
__KERNEL_RCSID(0, "$NetBSD: uvm_pglist.c,v 1.90 2021/12/21 08:27:49 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>
#include <uvm/uvm_pdpolicy.h>
#include <uvm/uvm_pgflcache.h>

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

kmutex_t uvm_pglistalloc_contig_lock;

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
	struct pgfreelist *pgfl;
	struct pgflbucket *pgb;

	pgfl = &uvm.page_free[uvm_page_get_freelist(pg)];
	pgb = pgfl->pgfl_buckets[uvm_page_get_bucket(pg)];

#ifdef UVMDEBUG
	struct vm_page *tp;
	LIST_FOREACH(tp, &pgb->pgb_colors[VM_PGCOLOR(pg)], pageq.list) {
		if (tp == pg)
			break;
	}
	if (tp == NULL)
		panic("uvm_pglistalloc: page not on freelist");
#endif
	LIST_REMOVE(pg, pageq.list);
	pgb->pgb_nfree--;
    	CPU_COUNT(CPU_COUNT_FREEPAGES, -1);
	pg->flags = PG_CLEAN;
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
	printf("pgalloc: contig %d pgs from psi %d\n", num, psi);
#endif

	low = atop(low);
	high = atop(high);

	/*
	 * Make sure that physseg falls within with range to be allocated from.
	 */
	if (high <= uvm_physseg_get_avail_start(psi) ||
	    low >= uvm_physseg_get_avail_end(psi))
		return -1;

	/*
	 * We start our search at the just after where the last allocation
	 * succeeded.
	 */
	alignment = atop(alignment);
	candidate = roundup2(uimax(low, uvm_physseg_get_avail_start(psi) +
		uvm_physseg_get_start_hint(psi)), alignment);
	limit = uimin(high, uvm_physseg_get_avail_end(psi));
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
			candidate = roundup2(uimax(low, uvm_physseg_get_avail_start(psi)), alignment);
			limit = uimin(limit, uvm_physseg_get_avail_start(psi) +
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
		printf("%s: psi=%d candidate=%#x end=%#x skip=%#x, align=%#"PRIxPADDR,
		    __func__, psi, candidateidx, end, skip, alignment);
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
		skip = uimax(num - cnt, 0);
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
uvm_pglistalloc_contig_aggressive(int num, paddr_t low, paddr_t high,
    paddr_t alignment, paddr_t boundary, struct pglist *rlist)
{
	struct vm_page *pg;
	struct pglist tmp;
	paddr_t pa, off, spa, amask, bmask, rlo, rhi;
	uvm_physseg_t upm;
	int error, i, run, acnt;

	/*
	 * Allocate pages the normal way and for each new page, check if
	 * the page completes a range satisfying the request.
	 * The pagedaemon will evict pages as we go and we are very likely
	 * to get compatible pages eventually.
	 */

	error = ENOMEM;
	TAILQ_INIT(&tmp);
	acnt = atop(alignment);
	amask = ~(alignment - 1);
	bmask = ~(boundary - 1);
	KASSERT(bmask <= amask);
	mutex_enter(&uvm_pglistalloc_contig_lock);
	while (uvm_reclaimable()) {
		pg = uvm_pagealloc(NULL, 0, NULL, 0);
		if (pg == NULL) {
			uvm_wait("pglac2");
			continue;
		}
		pg->flags |= PG_PGLCA;
		TAILQ_INSERT_HEAD(&tmp, pg, pageq.queue);

		pa = VM_PAGE_TO_PHYS(pg);
		if (pa < low || pa >= high) {
			continue;
		}

		upm = uvm_physseg_find(atop(pa), &off);
		KASSERT(uvm_physseg_valid_p(upm));

		spa = pa & amask;

		/*
		 * Look backward for at most num - 1 pages, back to
		 * the highest of:
		 *  - the first page in the physseg
		 *  - the specified low address
		 *  - num-1 pages before the one we just allocated
		 *  - the start of the boundary range containing pa
		 * all rounded up to alignment.
		 */

		rlo = roundup2(ptoa(uvm_physseg_get_avail_start(upm)), alignment);
		rlo = MAX(rlo, roundup2(low, alignment));
		rlo = MAX(rlo, roundup2(pa - ptoa(num - 1), alignment));
		if (boundary) {
			rlo = MAX(rlo, spa & bmask);
		}

		/*
		 * Look forward as far as the lowest of:
		 *  - the last page of the physseg
		 *  - the specified high address
		 *  - the boundary after pa
		 */

		rhi = ptoa(uvm_physseg_get_avail_end(upm));
		rhi = MIN(rhi, high);
		if (boundary) {
			rhi = MIN(rhi, rounddown2(pa, boundary) + boundary);
		}

		/*
		 * Make sure our range to consider is big enough.
		 */

		if (rhi - rlo < ptoa(num)) {
			continue;
		}

		run = 0;
		while (spa > rlo) {

			/*
			 * Examine pages before spa in groups of acnt.
			 * If all the pages in a group are marked then add
			 * these pages to the run.
			 */

			for (i = 0; i < acnt; i++) {
				pg = PHYS_TO_VM_PAGE(spa - alignment + ptoa(i));
				if ((pg->flags & PG_PGLCA) == 0) {
					break;
				}
			}
			if (i < acnt) {
				break;
			}
			spa -= alignment;
			run += acnt;
		}

		/*
		 * Look forward for any remaining pages.
		 */

		if (spa + ptoa(num) > rhi) {
			continue;
		}
		for (; run < num; run++) {
			pg = PHYS_TO_VM_PAGE(spa + ptoa(run));
			if ((pg->flags & PG_PGLCA) == 0) {
				break;
			}
		}
		if (run < num) {
			continue;
		}

		/*
		 * We found a match.  Move these pages from the tmp list to
		 * the caller's list.
		 */

		for (i = 0; i < num; i++) {
			pg = PHYS_TO_VM_PAGE(spa + ptoa(i));
			TAILQ_REMOVE(&tmp, pg, pageq.queue);
			pg->flags &= ~PG_PGLCA;
			TAILQ_INSERT_TAIL(rlist, pg, pageq.queue);
			STAT_INCR(uvm_pglistalloc_npages);
		}

		error = 0;
		break;
	}

	/*
	 * Free all the pages that we didn't need.
	 */

	while (!TAILQ_EMPTY(&tmp)) {
		pg = TAILQ_FIRST(&tmp);
		TAILQ_REMOVE(&tmp, pg, pageq.queue);
		pg->flags &= ~PG_PGLCA;
		uvm_pagefree(pg);
	}
	mutex_exit(&uvm_pglistalloc_contig_lock);
	return error;
}

static int
uvm_pglistalloc_contig(int num, paddr_t low, paddr_t high, paddr_t alignment,
    paddr_t boundary, struct pglist *rlist, int waitok)
{
	int fl;
	int error;
	uvm_physseg_t psi;

	/* Default to "lose". */
	error = ENOMEM;
	bool valid = false;

	/*
	 * Block all memory allocation and lock the free list.
	 */
	uvm_pgfl_lock();

	/* Are there even any free pages? */
	if (uvm_availmem(false) <=
	    (uvmexp.reserve_pagedaemon + uvmexp.reserve_kernel))
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

			int done = uvm_pglistalloc_c_ps(psi, num, low, high,
			    alignment, boundary, rlist);
			if (done >= 0) {
				valid = true;
				num -= done;
			}
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
	if (!valid) {
		uvm_pgfl_unlock();
		return EINVAL;
	}

out:
	uvm_pgfl_unlock();

	/*
	 * If that didn't work, try the more aggressive approach.
	 */

	if (error) {
		if (waitok) {
			error = uvm_pglistalloc_contig_aggressive(num, low, high,
			    alignment, boundary, rlist);
		} else {
			uvm_pglistfree(rlist);
			uvm_kick_pdaemon();
		}
	}
	return error;
}

static int
uvm_pglistalloc_s_ps(uvm_physseg_t psi, int num, paddr_t low, paddr_t high,
    struct pglist *rlist)
{
	int todo, limit, candidate;
	struct vm_page *pg;
	bool second_pass;
#ifdef PGALLOC_VERBOSE
	printf("pgalloc: simple %d pgs from psi %d\n", num, psi);
#endif

	KASSERT(uvm_physseg_get_start(psi) <= uvm_physseg_get_avail_start(psi));
	KASSERT(uvm_physseg_get_start(psi) <= uvm_physseg_get_avail_end(psi));
	KASSERT(uvm_physseg_get_avail_start(psi) <= uvm_physseg_get_end(psi));
	KASSERT(uvm_physseg_get_avail_end(psi) <= uvm_physseg_get_end(psi));

	low = atop(low);
	high = atop(high);

	/*
	 * Make sure that physseg falls within with range to be allocated from.
	 */
	if (high <= uvm_physseg_get_avail_start(psi) ||
	    low >= uvm_physseg_get_avail_end(psi))
		return -1;

	todo = num;
	candidate = uimax(low, uvm_physseg_get_avail_start(psi) +
	    uvm_physseg_get_start_hint(psi));
	limit = uimin(high, uvm_physseg_get_avail_end(psi));
	pg = uvm_physseg_get_pg(psi, candidate - uvm_physseg_get_start(psi));
	second_pass = false;

again:
	for (;; candidate++, pg++) {
		if (candidate >= limit) {
			if (uvm_physseg_get_start_hint(psi) == 0 || second_pass) {
				candidate = limit - 1;
				break;
			}
			second_pass = true;
			candidate = uimax(low, uvm_physseg_get_avail_start(psi));
			limit = uimin(limit, uvm_physseg_get_avail_start(psi) +
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
	int count = 0;

	/* Default to "lose". */
	error = ENOMEM;
	bool valid = false;

again:
	/*
	 * Block all memory allocation and lock the free list.
	 */
	uvm_pgfl_lock();
	count++;

	/* Are there even any free pages? */
	if (uvm_availmem(false) <=
	    (uvmexp.reserve_pagedaemon + uvmexp.reserve_kernel))
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

			int done = uvm_pglistalloc_s_ps(psi, num, low, high,
                            rlist);
			if (done >= 0) {
				valid = true;
				num -= done;
			}
			if (num == 0) {
				error = 0;
				goto out;
			}
		}

	}
	if (!valid) {
		uvm_pgfl_unlock();
		return EINVAL;
	}

out:
	/*
	 * check to see if we need to generate some free pages waking
	 * the pagedaemon.
	 */

	uvm_pgfl_unlock();
	uvm_kick_pdaemon();

	if (error) {
		if (waitok) {
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

	KASSERT(!cpu_intr_p());
	KASSERT(!cpu_softintr_p());
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

	/*
	 * Turn off the caching of free pages - we need everything to be on
	 * the global freelists.
	 */
	uvm_pgflcache_pause();

	if (nsegs < num || alignment != PAGE_SIZE || boundary != 0)
		res = uvm_pglistalloc_contig(num, low, high, alignment,
					     boundary, rlist, waitok);
	else
		res = uvm_pglistalloc_simple(num, low, high, rlist, waitok);

	uvm_pgflcache_resume();

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

	KASSERT(!cpu_intr_p());
	KASSERT(!cpu_softintr_p());

	while ((pg = TAILQ_FIRST(list)) != NULL) {
		TAILQ_REMOVE(list, pg, pageq.queue);
		uvm_pagefree(pg);
		STAT_DECR(uvm_pglistalloc_npages);
	}
}

void
uvm_pglistalloc_init(void)
{

	mutex_init(&uvm_pglistalloc_contig_lock, MUTEX_DEFAULT, IPL_NONE);
}
