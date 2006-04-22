/*	$NetBSD: jazzdmatlb.c,v 1.13.6.1 2006/04/22 11:37:16 simonb Exp $	*/
/*	$OpenBSD: dma.c,v 1.5 1998/03/01 16:49:57 niklas Exp $	*/

/*-
 * Copyright (C) 2000 Shuichiro URATA.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Jazz derived system dma driver. Handles resource allocation and
 * logical (virtual) address remaping.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: jazzdmatlb.c,v 1.13.6.1 2006/04/22 11:37:16 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <arc/jazz/jazzdmatlbreg.h>
#include <arc/jazz/jazzdmatlbvar.h>

#include <mips/cache.h>

extern paddr_t	kvtophys(vaddr_t);	/* XXX */

/*
 * Currently, only NET and BIO devices use DMA, and splnet > splbio.
 */
#define spldma()	splnet()

#define NDMATLB		(JAZZ_DMATLB_SIZE / sizeof(jazz_dma_pte_t))

static bus_space_tag_t dmatlb_iot;
static bus_space_handle_t dmatlb_ioh;

static struct extent *dmatlbmap;
static jazz_dma_pte_t *dma_tlb;

/*
 *  Initialize the dma mapping register area and pool.
 */
void
jazz_dmatlb_init(bus_space_tag_t iot, bus_addr_t ioaddr)
{
	int err;

	dmatlb_iot = iot;
	err = bus_space_map(iot, ioaddr, JAZZ_DMATLB_REGSIZE, 0, &dmatlb_ioh);
	if (err != 0)
		panic("jazz_dmatlb_init: cannot map 0x%lx", ioaddr);

	dma_tlb = (jazz_dma_pte_t *)PICA_TL_BASE;

	mips_dcache_wbinv_all();/* Make sure no map entries are cached */
	memset((char *)dma_tlb, 0, JAZZ_DMATLB_SIZE);

	dmatlbmap = extent_create("dmatlb", 0, NDMATLB, M_DEVBUF, NULL, 0,
	    EX_NOWAIT);
	if (dmatlbmap == NULL)
		panic("jazz_dmatlb_init: cannot create extent map");

	bus_space_write_4(dmatlb_iot, dmatlb_ioh, JAZZ_DMATLBREG_MAP,
	    MIPS_KSEG1_TO_PHYS(dma_tlb));
	bus_space_write_4(dmatlb_iot, dmatlb_ioh, JAZZ_DMATLBREG_LIMIT,
	    JAZZ_DMATLB_SIZE);
	jazz_dmatlb_flush();
}

/*
 *  Allocate an array of 'size' DMA PTEs.
 *  Return address to first pte.
 */
jazz_dma_pte_t *
jazz_dmatlb_alloc(int npte, bus_size_t boundary, int flags, bus_addr_t *addr)
{
	u_long start;
	int err;
	int s;

	s = spldma();
	err = extent_alloc(dmatlbmap, npte, 1, boundary / JAZZ_DMA_PAGE_SIZE,
	    (flags & BUS_DMA_WAITOK) ? (EX_WAITSPACE | EX_WAITOK) : EX_NOWAIT,
	    &start);
	splx(s);

	if (err)
		return NULL;

	*addr = start * JAZZ_DMA_PAGE_SIZE;

	return dma_tlb + start;
}

/*
 *  Free an array of DMA PTEs.
 */
void
jazz_dmatlb_free(bus_addr_t addr, int npte)
{
	u_long start;
	int s;

	start = addr / JAZZ_DMA_PAGE_SIZE;
	s = spldma();
	extent_free(dmatlbmap, start, npte, EX_NOWAIT);
	splx(s);
}

/*
 *  Map up a virtual address space in dma space given by
 *  the dma control structure.
 */
void
jazz_dmatlb_map_va(struct vmspace *vm, vaddr_t va, vsize_t size,
    jazz_dma_pte_t *dma_pte)
{
	paddr_t pa;

	size = jazz_dma_page_round(size + jazz_dma_page_offs(va));
	va &= JAZZ_DMA_PAGE_NUM;
	while (size > 0) {
		if (!VMSPACE_IS_KERNEL_P(vm))
			(void)pmap_extract(vm_map_pmap(&vm->vm_map), va, &pa);
		else
			pa = kvtophys(va);

		pa &= JAZZ_DMA_PAGE_NUM;
		dma_pte->lo_addr = pa;
		dma_pte->hi_addr = 0;
		dma_pte++;
		va += JAZZ_DMA_PAGE_SIZE;
		size -= JAZZ_DMA_PAGE_SIZE;
	}
}

/*
 *  Map up a physical address space in dma space given by
 *  the dma control structure.
 */
void
jazz_dmatlb_map_pa(paddr_t pa, psize_t size, jazz_dma_pte_t *dma_pte)
{

	size = jazz_dma_page_round(size + jazz_dma_page_offs(pa));
	pa &= JAZZ_DMA_PAGE_NUM;
	while (size > 0) {
		dma_pte->lo_addr = pa;
		dma_pte->hi_addr = 0;
		dma_pte++;
		pa += JAZZ_DMA_PAGE_SIZE;
		size -= JAZZ_DMA_PAGE_SIZE;
	}
}

/*
 *  Prepare for new dma by flushing
 */
void
jazz_dmatlb_flush(void)
{

	bus_space_write_4(dmatlb_iot, dmatlb_ioh, JAZZ_DMATLBREG_IVALID, 0);
}
