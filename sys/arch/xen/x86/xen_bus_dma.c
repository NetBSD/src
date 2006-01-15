/*	$NetBSD: xen_bus_dma.c,v 1.6 2006/01/15 22:09:52 bouyer Exp $	*/
/*	NetBSD bus_dma.c,v 1.21 2005/04/16 07:53:35 yamt Exp */

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
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
__KERNEL_RCSID(0, "$NetBSD: xen_bus_dma.c,v 1.6 2006/01/15 22:09:52 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/proc.h>

#include <machine/bus.h>
#include <machine/bus_private.h>

#include <uvm/uvm_extern.h>

extern paddr_t avail_end;

/* Pure 2^n version of get_order */
static inline int get_order(unsigned long size)
{
	int order = -1;
	size = (size - 1) >> (PAGE_SHIFT - 1);
	do {
		size >>= 1;
		order++;
	} while (size);
	return order;
}

static int
_xen_alloc_contig(bus_size_t size, bus_size_t alignment, bus_size_t boundary,
    struct pglist *mlistp, int flags)
{
	int order, i;
	unsigned long npagesreq, npages, mfn;
	bus_addr_t pa;
	struct vm_page *pg, *pgnext;
	struct pglist freelist;
	int s, error;
#ifdef XEN3
	struct xen_memory_reservation res;
#endif

	TAILQ_INIT(&freelist);

	/*
	 * When requesting a contigous memory region, the hypervisor will
	 * return a memory range aligned on size. This will automagically
	 * handle "boundary", but the only way to enforce alignment
	 * is to request a memory region of size max(alignment, size).
	 */
	order = max(get_order(size), get_order(alignment));
	npages = (1 << order);
	npagesreq = (size >> PAGE_SHIFT);
	KASSERT(npages >= npagesreq);

	/* get npages from UWM, and give them back to the hypervisor */
	error = uvm_pglistalloc(npages << PAGE_SHIFT, 0, avail_end, 0, 0,
	    mlistp, npages, (flags & BUS_DMA_NOWAIT) == 0);
	if (error)
		return (error);

	for (pg = mlistp->tqh_first; pg != NULL; pg = pg->pageq.tqe_next) {
		pa = VM_PAGE_TO_PHYS(pg);
		mfn = xpmap_ptom(pa) >> PAGE_SHIFT;
		xpmap_phys_to_machine_mapping[
		    (pa - XPMAP_OFFSET) >> PAGE_SHIFT] = INVALID_P2M_ENTRY;
#ifdef XEN3
		res.extent_start = &mfn;
		res.nr_extents = 1;
		res.extent_order = 0;
		res.domid = DOMID_SELF;
		if (HYPERVISOR_memory_op(XENMEM_decrease_reservation, &res)
		    < 0) {
			printf("xen_alloc_contig: XENMEM_decrease_reservation "
			    "failed!\n");
			return ENOMEM;
		}
#else
		if (HYPERVISOR_dom_mem_op(MEMOP_decrease_reservation,
		    &mfn, 1, 0) != 1) {
			printf("xen_alloc_contig: MEMOP_decrease_reservation "
			    "failed!\n");
			return ENOMEM;
		}
#endif
	}
	/* Get the new contiguous memory extent */
#ifdef XEN3
	res.extent_start = &mfn;
	res.nr_extents = 1;
	res.extent_order = order;
	res.address_bits = 31;
	res.domid = DOMID_SELF;
	if (HYPERVISOR_memory_op(XENMEM_increase_reservation, &res) < 0) {
		printf("xen_alloc_contig: XENMEM_increase_reservation "
		    "failed!\n");
		return ENOMEM;
	}
#else
	if (HYPERVISOR_dom_mem_op(MEMOP_increase_reservation,
	    &mfn, 1, order) != 1) {
		printf("xen_alloc_contig: MEMOP_increase_reservation "
		    "failed!\n");
		return ENOMEM;
	}
#endif
	s = splvm();
	/* Map the new extent in place of the old pages */
	for (pg = mlistp->tqh_first, i = 0; pg != NULL; pg = pgnext, i++) {
		pgnext = pg->pageq.tqe_next;
		pa = VM_PAGE_TO_PHYS(pg);
		xpmap_phys_to_machine_mapping[
		    (pa - XPMAP_OFFSET) >> PAGE_SHIFT] = mfn+i;
		xpq_queue_machphys_update((mfn+i) << PAGE_SHIFT, pa);
		/* while here, give extra pages back to UVM */
		if (i >= npagesreq) {
			TAILQ_REMOVE(mlistp, pg, pageq);
			uvm_pagefree(pg);
		}

	}
	/* Flush updates through and flush the TLB */
	xpq_queue_tlb_flush();
	xpq_flush_queue();
	splx(s);
	return 0;
}


/*
 * Allocate physical memory from the given physical address range.
 * Called by DMA-safe memory allocation methods.
 * We need our own version to deal with physical vs machine addresses.
 */
int
_xen_bus_dmamem_alloc_range(bus_dma_tag_t t, bus_size_t size,
    bus_size_t alignment, bus_size_t boundary, bus_dma_segment_t *segs,
    int nsegs, int *rsegs, int flags, bus_addr_t low, bus_addr_t high)
{
	paddr_t curaddr, lastaddr;
	bus_addr_t bus_curaddr, bus_lastaddr;
	struct vm_page *m;
	struct pglist mlist;
	int curseg, error;
	int doingrealloc = 0;

	/* Always round the size. */
	size = round_page(size);

	KASSERT((alignment & (alignment - 1)) == 0);
	KASSERT((boundary & (boundary - 1)) == 0);
	if (alignment < PAGE_SIZE)
		alignment = PAGE_SIZE;
	if (boundary != 0 && boundary < size)
		return (EINVAL);

	/*
	 * Allocate pages from the VM system.
	 */
	error = uvm_pglistalloc(size, 0, avail_end, alignment, boundary,
	    &mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
	if (error)
		return (error);
again:

	/*
	 * Compute the location, size, and number of segments actually
	 * returned by the VM code.
	 */
	m = mlist.tqh_first;
	curseg = 0;
	lastaddr = segs[curseg].ds_addr = VM_PAGE_TO_PHYS(m);
	segs[curseg].ds_len = PAGE_SIZE;
	m = m->pageq.tqe_next;
	if ((_BUS_PHYS_TO_BUS(segs[curseg].ds_addr) & (alignment - 1)) != 0)
		goto dorealloc;

	for (; m != NULL; m = m->pageq.tqe_next) {
		curaddr = VM_PAGE_TO_PHYS(m);
		bus_curaddr = _BUS_PHYS_TO_BUS(curaddr);
		bus_lastaddr = _BUS_PHYS_TO_BUS(lastaddr);
		if ((bus_lastaddr < low || bus_lastaddr >= high) ||
		    (bus_curaddr < low || bus_curaddr >= high)) {
			/*
			 * If machine addresses are outside the allowed
			 * range we have to bail. Xen2 doesn't offer an
			 * interface to get memory in a specific address
			 * range.
			 */
			printf("_xen_bus_dmamem_alloc_range: no way to "
			    "enforce address range\n");
			uvm_pglistfree(&mlist);
			return EINVAL;
		}
		if (bus_curaddr == (bus_lastaddr + PAGE_SIZE)) {
			segs[curseg].ds_len += PAGE_SIZE;
			if ((bus_lastaddr & boundary) !=
			    (bus_curaddr & boundary))
				goto dorealloc;
		}
		else {
			curseg++;
			if (curseg >= nsegs ||
			    (bus_curaddr & (alignment - 1)) != 0) {
dorealloc:
				if (doingrealloc == 1)
					panic("_xen_bus_dmamem_alloc_range: "
					   "xen_alloc_contig returned "
					   "too much segments");
				doingrealloc = 1;
				/*
				 * Too much segments. Free this memory and
				 * get a contigous segment from the hypervisor.
				 */
				uvm_pglistfree(&mlist);
				for (curseg = 0; curseg < nsegs; curseg++) {
					segs[curseg].ds_addr = 0;
					segs[curseg].ds_len = 0;
				}
				error = _xen_alloc_contig(size, alignment,
				    boundary, &mlist, flags);
				if (error)
					return error;
				goto again;
			}
			segs[curseg].ds_addr = curaddr;
			segs[curseg].ds_len = PAGE_SIZE;
		}
		lastaddr = curaddr;
	}

	*rsegs = curseg + 1;

	return (0);
}
