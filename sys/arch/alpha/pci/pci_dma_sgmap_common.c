/* $NetBSD: pci_dma_sgmap_common.c,v 1.1.2.1 1997/05/23 21:46:46 thorpej Exp $ */

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

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pci_dma_sgmap_common.c,v 1.1.2.1 1997/05/23 21:46:46 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <vm/vm.h>

#include <machine/bus.h>

#include <alpha/pci/pci_dma_sgmap.h>

void
pci_dma_sgmap_init(t, sgmap, name, base, size)
	bus_dma_tag_t t;
	struct alpha_pci_sgmap *sgmap;
	const char *name;
	bus_addr_t base;
	bus_size_t size;
{
	bus_dma_segment_t seg;
	size_t ptsize;
	int rseg;

	if (size & PGOFSET)
		panic("pci_dma_sgmap_create: size botch");

	/*
	 * Compute the page table size and allocate it.  It must
	 * be aligned to its size.
	 */
	ptsize = (size / NBPG) * sizeof(u_int64_t);
	if (bus_dmamem_alloc(t, ptsize, ptsize, 0, &seg, 1, &rseg,
	    BUS_DMA_NOWAIT))
		panic("pci_dma_sgmap_create: can't allocate page table");
	if (bus_dmamem_map(t, &seg, rseg, ptsize,
	    (caddr_t *)&sgmap->aps_pt, BUS_DMA_NOWAIT))
		panic("pci_dma_sgmap_create: can't map page table");

	/*
	 * Create the extent map used to manage the virtual address
	 * space.
	 */
	sgmap->aps_ex = extent_create((char *)name, 0, size - 1,
	    M_DEVBUF, NULL, 0, EX_NOWAIT|EX_NOCOALESCE);
	if (sgmap->aps_ex == NULL)
		panic("pci_dma_sgmap_create: can't create extent");

	sgmap->aps_ptpa = seg.ds_addr;
	sgmap->aps_base = base;
	sgmap->aps_size = size;
}

int
pci_dma_sgmap_alloc(map, len, sgmap, a, flags)
	bus_dmamap_t map;
	bus_size_t len;
	struct alpha_pci_sgmap *sgmap;
	struct alpha_pci_dma_cookie *a;
	int flags;
{
	int error;

#ifdef DIAGNOSTIC
	if (a->apdc_flags & APDC_HAS_SGMAP)
		panic("pci_dma_sgmap_alloc: already have sgva space");
#endif

	a->apdc_len = round_page(len);
	error = extent_alloc(sgmap->aps_ex, a->apdc_len, NBPG,
	    map->_dm_boundary, (flags & BUS_DMA_NOWAIT) ? EX_NOWAIT :
	    EX_WAITOK, &a->apdc_sgva);

	if (error == 0)
		a->apdc_flags |= APDC_HAS_SGMAP;
	else
		a->apdc_flags &= ~APDC_HAS_SGMAP;
	
	return (error);
}

void
pci_dma_sgmap_free(sgmap, a)
	struct alpha_pci_sgmap *sgmap;
	struct alpha_pci_dma_cookie *a;
{

#ifdef DIAGNOSTIC
	if ((a->apdc_flags & APDC_HAS_SGMAP) == 0)
		panic("pci_dma_sgmap_free: no sgva space to free");
#endif

	if (extent_free(sgmap->aps_ex, a->apdc_sgva,
	    a->apdc_len, EX_NOWAIT))
		panic("pci_dma_sgmap_free");

	a->apdc_flags &= ~APDC_HAS_SGMAP;
}

int
pci_dma_sgmap_load(t, map, buf, buflen, p, flags, sgmap, a)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
	struct alpha_pci_sgmap *sgmap;
	struct alpha_pci_dma_cookie *a;
{
	vm_offset_t va = (vm_offset_t)buf;
	vm_offset_t pa, endva;
	bus_addr_t dmaoffset;
	bus_size_t dmalen;
	u_int64_t *pte;
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

	/*
	 * Allocate the necessary virtual address space for the
	 * mapping.  Round the size, since we deal with whole pages.
	 */
	endva = round_page(va + buflen);
	if ((a->apdc_flags & APDC_HAS_SGMAP) == 0) {
		error = pci_dma_sgmap_alloc(map, endva - trunc_page(va),
		    sgmap, a, flags);
		if (error)
			return (error);
	}

	pteidx = a->apdc_sgva >> PGSHIFT;
	pte = &sgmap->aps_pt[pteidx];

	/*
	 * Generate the PCI DMA address.
	 */
	map->dm_segs[0].ds_addr = sgmap->aps_base |
	    (pteidx << PCIADDR_PTEIDX_SHIFT) | dmaoffset;
	map->dm_segs[0].ds_len = dmalen;

	a->apdc_pteidx = pteidx;
	a->apdc_ptecnt = 0;

	for (; va < endva; va += NBPG, pte++, a->apdc_ptecnt++) {
		/*
		 * Get the physical address for this segment.
		 */
		pa = pmap_extract(p != NULL ? &p->p_vmspace->vm_pmap :
		    pmap_kernel(), va);
		pa = trunc_page(pa);

		/*
		 * Load the current PTE with this page.
		 */
		*pte = ((pa >> PSGPTE_PGADDR_SHIFT) & PSGPTE_PGADDR_MASK) |
		    PSGPTE_VALID;
	}

	alpha_mb();		/* XXX paranoia? */

	map->dm_nsegs = 1;
	a->apdc_flags |= APDC_USING_SGMAP;
	return (0);
}

int
pci_dma_sgmap_load_mbuf(t, map, m, flags, sgmap, a)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m;
	int flags;
	struct alpha_pci_sgmap *sgmap;
	struct alpha_pci_dma_cookie *a;
{

	panic("pci_dma_sgmap_load_mbuf: not implemented");
}

int
pci_dma_sgmap_load_uio(t, map, uio, flags, sgmap, a)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
	struct alpha_pci_sgmap *sgmap;
	struct alpha_pci_dma_cookie *a;
{

	panic("pci_dma_sgmap_load_uio: not implemented");
}

int
pci_dma_sgmap_load_raw(t, map, segs, nsegs, size, flags, sgmap, a)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
	struct alpha_pci_sgmap *sgmap;
	struct alpha_pci_dma_cookie *a;
{

	panic("pci_dma_sgmap_load_raw: not implemented");
}

void
pci_dma_sgmap_unload(t, map, sgmap, a)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct alpha_pci_sgmap *sgmap;
	struct alpha_pci_dma_cookie *a;
{
	u_int64_t *pte;
	int ptecnt;

	/*
	 * Invalidate the PTEs for the mapping.
	 */
	for (ptecnt = a->apdc_ptecnt, pte = &sgmap->aps_pt[a->apdc_pteidx];
	    ptecnt != 0; ptecnt--, pte++)
		*pte = 0UL;

	/*
	 * Free the virtual address space used by the mapping
	 * if necessary.
	 */
	if ((map->_dm_flags & BUS_DMA_ALLOCNOW) == 0)
		pci_dma_sgmap_free(sgmap, a);

	a->apdc_flags &= ~APDC_USING_SGMAP;
}
