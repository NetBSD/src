/* $NetBSD: sgmap_typedep.c,v 1.1.2.5 1997/06/05 02:33:02 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: sgmap_typedep.c,v 1.1.2.5 1997/06/05 02:33:02 thorpej Exp $");

int
__C(SGMAP_TYPE,_sgmap_load)(t, map, buf, buflen, p, flags, sgmap)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
	struct alpha_sgmap *sgmap;
{
	struct alpha_sgmap_cookie *a = map->_dm_sgcookie;
	vm_offset_t va = (vm_offset_t)buf;
	vm_offset_t pa, endva;
	bus_addr_t dmaoffset;
	bus_size_t dmalen;
	SGMAP_PTE_TYPE *pte, *page_table = sgmap->aps_pt;
	int pteidx, error;

	if (buflen > map->_dm_size)
		return (EINVAL);

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;

	/*
	 * Remember the offset into the first page and the total
	 * transfer length.
	 */
	dmaoffset = ((u_long)buf) & PGOFSET;
	dmalen = buflen;

#ifdef SGMAP_DEBUG
	printf("sgmap_load: ----- buf = %p -----\n", buf);
	printf("sgmap_load: dmaoffset = 0x%lx, dmalen = 0x%lx\n",
	    dmaoffset, dmalen);
#endif

	/*
	 * Allocate the necessary virtual address space for the
	 * mapping.  Round the size, since we deal with whole pages.
	 */
	endva = round_page(va + buflen);
	va = trunc_page(va);
	if ((a->apdc_flags & APDC_HAS_SGMAP) == 0) {
		error = alpha_sgmap_alloc(map, endva - va, sgmap, flags);
		if (error)
			return (error);
	}

	pteidx = a->apdc_sgva >> PGSHIFT;
	pte = &page_table[pteidx * SGMAP_PTE_SPACING];

#ifdef SGMAP_DEBUG
	printf("sgmap_load: sgva = 0x%lx, pteidx = %d, pte = %p (pt = %p)\n",
	    a->apdc_sgva, pteidx, pte, page_table);
#endif

	/*
	 * Generate the DMA address.
	 */
	map->dm_segs[0].ds_addr = sgmap->aps_wbase |
	    (pteidx << SGMAP_ADDR_PTEIDX_SHIFT) | dmaoffset;
	map->dm_segs[0].ds_len = dmalen;

#ifdef SGMAP_DEBUG
	printf("sgmap_load: wbase = 0x%lx, vpage = 0x%x, dma addr = 0x%lx\n",
	    sgmap->aps_wbase, (pteidx << SGMAP_ADDR_PTEIDX_SHIFT),
	    map->dm_segs[0].ds_addr);
#endif

	a->apdc_pteidx = pteidx;
	a->apdc_ptecnt = 0;

	for (; va < endva; va += NBPG, pte += SGMAP_PTE_SPACING,
	    a->apdc_ptecnt++) {
		/*
		 * Get the physical address for this segment.
		 */
		if (p != NULL)
			pa = pmap_extract(&p->p_vmspace->vm_pmap, va);
		else
			pa = vtophys(va);

		/*
		 * Load the current PTE with this page.
		 */
		*pte = (pa >> SGPTE_PGADDR_SHIFT) | SGPTE_VALID;
#ifdef SGMAP_DEBUG
		printf("sgmap_load:     pa = 0x%lx, pte = %p, *pte = 0x%lx\n",
		    pa, pte, (u_long)(*pte));
#endif
	}

	alpha_mb();

	map->dm_nsegs = 1;
	return (0);
}

int
__C(SGMAP_TYPE,_sgmap_load_mbuf)(t, map, m, flags, sgmap)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m;
	int flags;
	struct alpha_sgmap *sgmap;
{

	panic(__S(__C(SGMAP_TYPE,_sgmap_load_mbuf)) ": not implemented");
}

int
__C(SGMAP_TYPE,_sgmap_load_uio)(t, map, uio, flags, sgmap)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
	struct alpha_sgmap *sgmap;
{

	panic(__S(__C(SGMAP_TYPE,_sgmap_load_uio)) ": not implemented");
}

int
__C(SGMAP_TYPE,_sgmap_load_raw)(t, map, segs, nsegs, size, flags, sgmap)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
	struct alpha_sgmap *sgmap;
{

	panic(__S(__C(SGMAP_TYPE,_sgmap_load_raw)) ": not implemented");
}

void
__C(SGMAP_TYPE,_sgmap_unload)(t, map, sgmap)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct alpha_sgmap *sgmap;
{
	struct alpha_sgmap_cookie *a = map->_dm_sgcookie;
	SGMAP_PTE_TYPE *pte, *page_table = sgmap->aps_pt;
	int ptecnt;

	/*
	 * Invalidate the PTEs for the mapping.
	 */
	for (ptecnt = a->apdc_ptecnt,
	    pte = &page_table[a->apdc_pteidx * SGMAP_PTE_SPACING];
	    ptecnt != 0; ptecnt--, pte += SGMAP_PTE_SPACING) {
#ifdef SGMAP_DEBUG
		printf("sgmap_unload:     pte = %p, *pte = 0x%lx\n",
		    pte, (u_long)(*pte));
#endif
		*pte = 0;
	}

	/*
	 * Free the virtual address space used by the mapping
	 * if necessary.
	 */
	if ((map->_dm_flags & BUS_DMA_ALLOCNOW) == 0)
		alpha_sgmap_free(sgmap, a);
}
