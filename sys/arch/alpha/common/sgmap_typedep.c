/* $NetBSD: sgmap_typedep.c,v 1.44 2021/07/19 16:25:54 thorpej Exp $ */

/*-
 * Copyright (c) 1997, 1998, 2001 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: sgmap_typedep.c,v 1.44 2021/07/19 16:25:54 thorpej Exp $");

#include "opt_ddb.h"

#include <sys/evcnt.h>
#include <uvm/uvm_extern.h>

#define	DMA_COUNT_DECL(cnt)	_DMA_COUNT_DECL(dma_sgmap, cnt)
#define	DMA_COUNT(cnt)		_DMA_COUNT(dma_sgmap, cnt)

#ifdef SGMAP_DEBUG
int			__C(SGMAP_TYPE,_debug) = 0;
#endif

SGMAP_PTE_TYPE		__C(SGMAP_TYPE,_prefetch_spill_page_pte);

static void		__C(SGMAP_TYPE,_do_unload)(bus_dma_tag_t, bus_dmamap_t,
			    struct alpha_sgmap *);

void
__C(SGMAP_TYPE,_init_spill_page_pte)(void)
{

	__C(SGMAP_TYPE,_prefetch_spill_page_pte) =
	    (alpha_sgmap_prefetch_spill_page_pa >>
	     SGPTE_PGADDR_SHIFT) | SGPTE_VALID;
}

DMA_COUNT_DECL(spill_page);
DMA_COUNT_DECL(extra_segment);
DMA_COUNT_DECL(extra_segment_and_spill);

static int
__C(SGMAP_TYPE,_load_buffer)(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    size_t buflen, struct vmspace *vm, int flags, int * const segp,
    struct alpha_sgmap *sgmap)
{
	vaddr_t endva, va = (vaddr_t)buf;
	paddr_t pa;
	bus_addr_t dmaoffset, sgva, extra_sgva;
	bus_size_t sgvalen, extra_sgvalen, boundary, alignment;
	SGMAP_PTE_TYPE *pte, *page_table = sgmap->aps_pt;
	int pteidx, error, spill, seg = *segp;
	bool address_is_valid __diagused;

	/* Initialize the spill page PTE if it hasn't been already. */
	if (__C(SGMAP_TYPE,_prefetch_spill_page_pte) == 0)
		__C(SGMAP_TYPE,_init_spill_page_pte)();

	if (seg == map->_dm_segcnt) {
		/* Ran of segments. */
		return EFBIG;
	}
	KASSERT(seg < map->_dm_segcnt);

	/*
	 * Remember the offset into the first page and the total
	 * transfer length.
	 */
	dmaoffset = ((u_long)buf) & PGOFSET;

#ifdef SGMAP_DEBUG
	if (__C(SGMAP_TYPE,_debug)) {
		printf("sgmap_load: ----- buf = %p -----\n", buf);
		printf("sgmap_load: dmaoffset = 0x%lx, buflen = 0x%lx\n",
		    dmaoffset, buflen);
	}
#endif

	/*
	 * Allocate the necessary virtual address space for the
	 * mapping.  Round the size, since we deal with whole pages.
	 */

	/*
	 * XXX Always allocate a spill page for now.  Note
	 * the spill page is not needed for an in-bound-only
	 * transfer.
	 */
	if ((flags & BUS_DMA_READ) == 0)
		spill = 1;
	else
		spill = 0;

	boundary = map->_dm_boundary;

	/*
	 * Caller's mistake if the requested length is larger than
	 * their own boundary constraint.
	 */
	if (__predict_false(boundary != 0 && buflen > boundary)) {
		return EINVAL;
	}

	endva = round_page(va + buflen);
	va = trunc_page(va);

	const vm_flag_t vmflags = VM_INSTANTFIT |
	    ((flags & BUS_DMA_NOWAIT) ? VM_NOSLEEP : VM_SLEEP);

	KASSERT(t->_sgmap_minalign != 0);
	alignment = t->_sgmap_minalign;
	sgvalen = (endva - va);

	SGMAP_PTE_TYPE spill_pte_v = __C(SGMAP_TYPE,_prefetch_spill_page_pte);

	/*
	 * If we have a boundary constraint, it's possible to end up in
	 * a situation where sgvalen > boundary if the caller's buffer
	 * is not page aligned.  In this case, we will have to allocate
	 * an extra SG segment and split the buffer.
	 */
	if (__predict_false(boundary != 0 && boundary < sgvalen)) {
#ifdef SGMAP_DEBUG
		if (__C(SGMAP_TYPE,_debug)) {
			printf("sgmap_load: extra segment needed\n");
		}
#endif
		DMA_COUNT(extra_segment);

		/* This should only ever happen for unaligned buffers. */
		KASSERT(dmaoffset != 0);

		extra_sgvalen = sgvalen - boundary;
		KASSERT(extra_sgvalen == PAGE_SIZE);

		/*
		 * Adjust the lengths of the first segment.  The length
		 * of the second segment will be dmaoffset.
		 */
		sgvalen -= extra_sgvalen;
		endva -= extra_sgvalen;
		buflen -= dmaoffset;

		if (spill) {
			DMA_COUNT(extra_segment_and_spill);
			extra_sgvalen += PAGE_SIZE;
		}

		error = vmem_xalloc(sgmap->aps_arena, extra_sgvalen,
				    alignment,		/* alignment */
				    0,			/* phase */
				    boundary,		/* nocross */
				    VMEM_ADDR_MIN,	/* minaddr */
				    VMEM_ADDR_MAX,	/* maxaddr */
				    vmflags,
				    &extra_sgva);
		if (error) {
			return error;
		}
	} else {
		extra_sgvalen = 0;
		extra_sgva = 0;
	}


	if (spill) {
		DMA_COUNT(spill_page);
		sgvalen += PAGE_SIZE;

		/*
		 * ARGH!  If the addition of the spill page bumped us
		 * over our boundary, we have to 2x the boundary limit.
		 * To compensate (and enforce the original boundary
		 * constraint), we force our alignment to be at least the
		 * previous boundary, thus ensuring that the only boundary
		 * violation is the pre-fetch that the SGMAP controller
		 * performs that necessitates the spill page in the first
		 * place.
		 */
		if (boundary && boundary < sgvalen) {
			if (alignment < boundary) {
				alignment = boundary;
			}
			do {
				boundary <<= 1;
			} while (boundary < sgvalen);
		}
	}

#ifdef SGMAP_DEBUG
	if (__C(SGMAP_TYPE,_debug)) {
		printf("sgmap_load: va:endva = 0x%lx:0x%lx\n", va, endva);
		printf("sgmap_load: sgvalen = 0x%lx, boundary = 0x%lx\n",
		       sgvalen, boundary);
	}
#endif

	error = vmem_xalloc(sgmap->aps_arena, sgvalen,
			    alignment,		/* alignment */
			    0,			/* phase */
			    boundary,		/* nocross */
			    VMEM_ADDR_MIN,	/* minaddr */
			    VMEM_ADDR_MAX,	/* maxaddr */
			    vmflags,
			    &sgva);
	if (error) {
		if (extra_sgvalen != 0) {
			vmem_xfree(sgmap->aps_arena, extra_sgva, extra_sgvalen);
		}
		return error;
	}

	pteidx = sgva >> SGMAP_ADDR_PTEIDX_SHIFT;
	pte = &page_table[pteidx * SGMAP_PTE_SPACING];

#ifdef SGMAP_DEBUG
	if (__C(SGMAP_TYPE,_debug))
		printf("sgmap_load: sgva = 0x%lx, pteidx = %d, "
		    "pte = %p (pt = %p)\n", sgva, pteidx, pte,
		    page_table);
#endif

	/* Generate the DMA address. */
	map->dm_segs[seg].ds_addr = sgmap->aps_wbase | sgva | dmaoffset;
	map->dm_segs[seg].ds_len = buflen;
	if (__predict_false(extra_sgvalen != 0)) {
		if (++seg == map->_dm_segcnt) {
			/* Boo! Ran out of segments! */
			vmem_xfree(sgmap->aps_arena, extra_sgva, extra_sgvalen);
			vmem_xfree(sgmap->aps_arena, sgva, sgvalen);
			return EFBIG;
		}
		map->dm_segs[seg].ds_addr = sgmap->aps_wbase | extra_sgva;
		map->dm_segs[seg].ds_len = dmaoffset;
		*segp = seg;
	}

#ifdef SGMAP_DEBUG
	if (__C(SGMAP_TYPE,_debug))
		printf("sgmap_load: wbase = 0x%lx, vpage = 0x%lx, "
		    "DMA addr = 0x%lx\n", sgmap->aps_wbase, (uint64_t)sgva,
		    map->dm_segs[seg].ds_addr);
#endif

	for (; va < endva; va += PAGE_SIZE, pteidx++,
	     pte = &page_table[pteidx * SGMAP_PTE_SPACING]) {
		/* Get the physical address for this segment. */
		address_is_valid = pmap_extract(vm->vm_map.pmap, va, &pa);
		KASSERT(address_is_valid);

		/* Load the current PTE with this page. */
		*pte = (pa >> SGPTE_PGADDR_SHIFT) | SGPTE_VALID;
#ifdef SGMAP_DEBUG
		if (__C(SGMAP_TYPE,_debug))
			printf("sgmap_load:     pa = 0x%lx, pte = %p, "
			    "*pte = 0x%lx\n", pa, pte, (u_long)(*pte));
#endif
	}

	if (__predict_false(extra_sgvalen != 0)) {
		int extra_pteidx = extra_sgva >> SGMAP_ADDR_PTEIDX_SHIFT;
		SGMAP_PTE_TYPE *extra_pte =
		    &page_table[extra_pteidx * SGMAP_PTE_SPACING];

		/* va == endva == address of extra page */
		KASSERT(va == endva);
		address_is_valid = pmap_extract(vm->vm_map.pmap, va, &pa);
		KASSERT(address_is_valid);

		/*
		 * If a spill page is needed, the previous segment will
		 * need to use this PTE value for it.
		 */
		spill_pte_v = (pa >> SGPTE_PGADDR_SHIFT) | SGPTE_VALID;
		*extra_pte = spill_pte_v;

		/* ...but the extra segment uses the real spill PTE. */
		if (spill) {
			extra_pteidx++;
			extra_pte =
			    &page_table[extra_pteidx * SGMAP_PTE_SPACING];
			*extra_pte = __C(SGMAP_TYPE,_prefetch_spill_page_pte);
		}
	}

	if (spill) {
		/* ...and the prefetch-spill page. */
		*pte = spill_pte_v;
#ifdef SGMAP_DEBUG
		if (__C(SGMAP_TYPE,_debug)) {
			printf("sgmap_load:     spill page, pte = %p, "
			    "*pte = 0x%lx\n", pte, (uint64_t)*pte);
		}
#endif
	}

	return (0);
}

DMA_COUNT_DECL(load);
DMA_COUNT_DECL(load_next_window);

int
__C(SGMAP_TYPE,_load)(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags, struct alpha_sgmap *sgmap)
{
	int seg, error;
	struct vmspace *vm;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
		return (EINVAL);

	KASSERT((map->_dm_flags & (BUS_DMA_READ|BUS_DMA_WRITE)) == 0);
	KASSERT((flags & (BUS_DMA_READ|BUS_DMA_WRITE)) !=
	    (BUS_DMA_READ|BUS_DMA_WRITE));

	map->_dm_flags |= flags & (BUS_DMA_READ|BUS_DMA_WRITE);

	if (p != NULL) {
		vm = p->p_vmspace;
	} else {
		vm = vmspace_kernel();
	}
	seg = 0;
	error = __C(SGMAP_TYPE,_load_buffer)(t, map, buf, buflen, vm,
	    flags, &seg, sgmap);

	alpha_mb();

#if defined(SGMAP_DEBUG) && defined(DDB)
	if (__C(SGMAP_TYPE,_debug) > 1)
		Debugger();
#endif

	if (error == 0) {
		DMA_COUNT(load);
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;
		map->_dm_window = t;
	} else {
		map->_dm_flags &= ~(BUS_DMA_READ|BUS_DMA_WRITE);
		if (t->_next_window != NULL) {
			/* Give the next window a chance. */
			DMA_COUNT(load_next_window);
			error = bus_dmamap_load(t->_next_window, map, buf,
			    buflen, p, flags);
		}
	}
	return (error);
}

DMA_COUNT_DECL(load_mbuf);
DMA_COUNT_DECL(load_mbuf_next_window);

int
__C(SGMAP_TYPE,_load_mbuf)(bus_dma_tag_t t, bus_dmamap_t map,
    struct mbuf *m0, int flags, struct alpha_sgmap *sgmap)
{
	struct mbuf *m;
	int seg, error;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic(__S(__C(SGMAP_TYPE,_load_mbuf)) ": no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return (EINVAL);

	KASSERT((map->_dm_flags & (BUS_DMA_READ|BUS_DMA_WRITE)) == 0);
	KASSERT((flags & (BUS_DMA_READ|BUS_DMA_WRITE)) !=
	    (BUS_DMA_READ|BUS_DMA_WRITE));

	map->_dm_flags |= flags & (BUS_DMA_READ|BUS_DMA_WRITE);

	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		error = __C(SGMAP_TYPE,_load_buffer)(t, map,
		    m->m_data, m->m_len, vmspace_kernel(), flags, &seg, sgmap);
		seg++;
	}

	alpha_mb();

#if defined(SGMAP_DEBUG) && defined(DDB)
	if (__C(SGMAP_TYPE,_debug) > 1)
		Debugger();
#endif

	if (error == 0) {
		DMA_COUNT(load_mbuf);
		map->dm_mapsize = m0->m_pkthdr.len;
		map->dm_nsegs = seg;
		map->_dm_window = t;
	} else {
		/* Need to back out what we've done so far. */
		map->dm_nsegs = seg - 1;
		__C(SGMAP_TYPE,_do_unload)(t, map, sgmap);
		map->_dm_flags &= ~(BUS_DMA_READ|BUS_DMA_WRITE);
		if (t->_next_window != NULL) {
			/* Give the next window a chance. */
			DMA_COUNT(load_mbuf_next_window);
			error = bus_dmamap_load_mbuf(t->_next_window, map,
			    m0, flags);
		}
	}

	return (error);
}

DMA_COUNT_DECL(load_uio);
DMA_COUNT_DECL(load_uio_next_window);

int
__C(SGMAP_TYPE,_load_uio)(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio,
    int flags, struct alpha_sgmap *sgmap)
{
	bus_size_t minlen, resid;
	struct vmspace *vm;
	struct iovec *iov;
	void *addr;
	int i, seg, error;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	KASSERT((map->_dm_flags & (BUS_DMA_READ|BUS_DMA_WRITE)) == 0);
	KASSERT((flags & (BUS_DMA_READ|BUS_DMA_WRITE)) !=
	    (BUS_DMA_READ|BUS_DMA_WRITE));

	map->_dm_flags |= flags & (BUS_DMA_READ|BUS_DMA_WRITE);

	resid = uio->uio_resid;
	iov = uio->uio_iov;

	vm = uio->uio_vmspace;

	seg = 0;
	error = 0;
	for (i = 0; i < uio->uio_iovcnt && resid != 0 && error == 0; i++) {
		/*
		 * Now at the first iovec to load.  Load each iovec
		 * until we have exhausted the residual count.
		 */
		minlen = resid < iov[i].iov_len ? resid : iov[i].iov_len;
		addr = (void *)iov[i].iov_base;

		error = __C(SGMAP_TYPE,_load_buffer)(t, map,
		    addr, minlen, vm, flags, &seg, sgmap);
		seg++;

		resid -= minlen;
	}

	alpha_mb();

#if defined(SGMAP_DEBUG) && defined(DDB)
	if (__C(SGMAP_TYPE,_debug) > 1)
		Debugger();
#endif

	if (error == 0) {
		DMA_COUNT(load_uio);
		map->dm_mapsize = uio->uio_resid;
		map->dm_nsegs = seg;
		map->_dm_window = t;
	} else {
		/* Need to back out what we've done so far. */
		map->dm_nsegs = seg - 1;
		__C(SGMAP_TYPE,_do_unload)(t, map, sgmap);
		map->_dm_flags &= ~(BUS_DMA_READ|BUS_DMA_WRITE);
		if (t->_next_window != NULL) {
			/* Give the next window a chance. */
			DMA_COUNT(load_uio_next_window);
			error = bus_dmamap_load_uio(t->_next_window, map,
			    uio, flags);
		}
	}

	return (error);
}

int
__C(SGMAP_TYPE,_load_raw)(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags,
    struct alpha_sgmap *sgmap)
{

	KASSERT((map->_dm_flags & (BUS_DMA_READ|BUS_DMA_WRITE)) == 0);
	KASSERT((flags & (BUS_DMA_READ|BUS_DMA_WRITE)) !=
	    (BUS_DMA_READ|BUS_DMA_WRITE));

	panic(__S(__C(SGMAP_TYPE,_load_raw)) ": not implemented");
}

static void
__C(SGMAP_TYPE,_do_unload)(bus_dma_tag_t t, bus_dmamap_t map,
    struct alpha_sgmap *sgmap)
{
	SGMAP_PTE_TYPE *pte, *page_table = sgmap->aps_pt;
	bus_addr_t osgva, sgva, esgva;
	int spill, seg, pteidx;

	for (seg = 0; seg < map->dm_nsegs; seg++) {
		/*
		 * XXX Always allocate a spill page for now.  Note
		 * the spill page is not needed for an in-bound-only
		 * transfer.
		 */
		if ((map->_dm_flags & BUS_DMA_READ) == 0)
			spill = 1;
		else
			spill = 0;

		sgva = map->dm_segs[seg].ds_addr & ~sgmap->aps_wbase;

		esgva = round_page(sgva + map->dm_segs[seg].ds_len);
		osgva = sgva = trunc_page(sgva);

		if (spill)
			esgva += PAGE_SIZE;

		/* Invalidate the PTEs for the mapping. */
		for (pteidx = sgva >> SGMAP_ADDR_PTEIDX_SHIFT;
		     sgva < esgva; sgva += PAGE_SIZE, pteidx++) {
			pte = &page_table[pteidx * SGMAP_PTE_SPACING];
#ifdef SGMAP_DEBUG
			if (__C(SGMAP_TYPE,_debug))
				printf("sgmap_unload:     pte = %p, "
				    "*pte = 0x%lx\n", pte, (u_long)(*pte));
#endif
			*pte = 0;
		}

		alpha_mb();

		/* Free the virtual address space used by the mapping. */
		vmem_xfree(sgmap->aps_arena, osgva, (esgva - osgva));
	}

	map->_dm_flags &= ~(BUS_DMA_READ|BUS_DMA_WRITE);

	/* Mark the mapping invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	map->_dm_window = NULL;
}

DMA_COUNT_DECL(unload);

void
__C(SGMAP_TYPE,_unload)(bus_dma_tag_t t, bus_dmamap_t map,
    struct alpha_sgmap *sgmap)
{
	KASSERT(map->_dm_window == t);
	DMA_COUNT(unload);
	__C(SGMAP_TYPE,_do_unload)(t, map, sgmap);
}
