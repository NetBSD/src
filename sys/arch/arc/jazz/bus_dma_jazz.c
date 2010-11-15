/*	$NetBSD: bus_dma_jazz.c,v 1.16 2010/11/15 06:13:16 uebayasi Exp $	*/

/*-
 * Copyright (c) 2003 Izumi Tsutsui.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_dma_jazz.c,v 1.16 2010/11/15 06:13:16 uebayasi Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#define _ARC_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <arc/jazz/jazzdmatlbreg.h>
#include <arc/jazz/jazzdmatlbvar.h>

typedef struct jazz_tlbmap {
	struct jazz_dma_pte *ptebase;
	bus_addr_t vaddr;
} *jazz_tlbmap_t;

static int	jazz_bus_dmamap_alloc_sgmap(bus_dma_tag_t,
		    bus_dma_segment_t *, int, bus_size_t, int);
static void	jazz_bus_dmamap_free_sgmap(bus_dma_tag_t,
		    bus_dma_segment_t *, int);

int	jazz_bus_dmamap_create(bus_dma_tag_t, bus_size_t, int,
	    bus_size_t, bus_size_t, int, bus_dmamap_t *);
void	jazz_bus_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
int	jazz_bus_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int);
int	jazz_bus_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int);
int	jazz_bus_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int);
int	jazz_bus_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int);
void	jazz_bus_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	jazz_bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t,
	    bus_addr_t, bus_size_t, int);

void
jazz_bus_dma_tag_init(bus_dma_tag_t t)
{

	_bus_dma_tag_init(t);

	t->_dmamap_create = jazz_bus_dmamap_create;
	t->_dmamap_destroy = jazz_bus_dmamap_destroy;
	t->_dmamap_load = jazz_bus_dmamap_load;
	t->_dmamap_load_mbuf = jazz_bus_dmamap_load_mbuf;
	t->_dmamap_load_uio = jazz_bus_dmamap_load_uio;
	t->_dmamap_load_raw = jazz_bus_dmamap_load_raw;
	t->_dmamap_unload = jazz_bus_dmamap_unload;
	t->_dmamap_sync = jazz_bus_dmamap_sync;
	t->_dmamem_alloc = _bus_dmamem_alloc;
	t->_dmamem_free = _bus_dmamem_free;
}

static int
jazz_bus_dmamap_alloc_sgmap(bus_dma_tag_t t, bus_dma_segment_t *segs,
    int nsegs, bus_size_t boundary, int flags)
{
	jazz_dma_pte_t *dmapte;
	bus_addr_t addr;
	bus_size_t off;
	int i, npte;

	for (i = 0; i < nsegs; i++) {
		off = jazz_dma_page_offs(segs[i]._ds_paddr);
		npte = jazz_dma_page_round(segs[i].ds_len + off) /
		    JAZZ_DMA_PAGE_SIZE;
		dmapte = jazz_dmatlb_alloc(npte, boundary, flags, &addr);
		if (dmapte == NULL)
			return ENOMEM;
		segs[i].ds_addr = addr + off;

		jazz_dmatlb_map_pa(segs[i]._ds_paddr, segs[i].ds_len, dmapte);
	}
	return 0;
}

static void
jazz_bus_dmamap_free_sgmap(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs)
{
	int i, npte;
	bus_addr_t addr;

	for (i = 0; i < nsegs; i++) {
		addr = (segs[i].ds_addr - t->dma_offset) & JAZZ_DMA_PAGE_NUM;
		npte =  jazz_dma_page_round(segs[i].ds_len +
		    jazz_dma_page_offs(segs[i].ds_addr)) / JAZZ_DMA_PAGE_SIZE;
		jazz_dmatlb_free(addr, npte);
	}
}


/*
 * function to create a DMA map. If BUS_DMA_ALLOCNOW is specified and
 * nsegments is 1, allocate jazzdmatlb here, too.
 */
int
jazz_bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct arc_bus_dmamap *map;
	jazz_tlbmap_t tlbmap;
	int error, npte;

	if (nsegments > 1)
		/*
		 * BUS_DMA_ALLOCNOW is allowed only with one segment for now.
		 * XXX needs re-think.
		 */
		flags &= ~BUS_DMA_ALLOCNOW;

	if ((flags & BUS_DMA_ALLOCNOW) == 0)
		return _bus_dmamap_create(t, size, nsegments, maxsegsz,
		    boundary, flags, dmamp);

	tlbmap = malloc(sizeof(struct jazz_tlbmap), M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK);
	if (tlbmap == NULL)
		return ENOMEM;

	npte = jazz_dma_page_round(maxsegsz) / JAZZ_DMA_PAGE_SIZE + 1;
	tlbmap->ptebase =
	    jazz_dmatlb_alloc(npte, boundary, flags, &tlbmap->vaddr);
	if (tlbmap->ptebase == NULL) {
		free(tlbmap, M_DMAMAP);
		return ENOMEM;
	}

	error = _bus_dmamap_create(t, size, 1, maxsegsz, boundary,
	    flags, dmamp);
	if (error != 0) {
		jazz_dmatlb_free(tlbmap->vaddr, npte);
		free(tlbmap, M_DMAMAP);
		return error;
	}
	map = *dmamp;
	map->_dm_cookie = (void *)tlbmap;

	return 0;
}

/*
 * function to destroy a DMA map. If BUS_DMA_ALLOCNOW is specified,
 * free jazzdmatlb, too.
 */
void
jazz_bus_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
{

	if ((map->_dm_flags & BUS_DMA_ALLOCNOW) != 0) {
		jazz_tlbmap_t tlbmap;
		int npte;

		tlbmap = (jazz_tlbmap_t)map->_dm_cookie;
		npte = jazz_dma_page_round(map->dm_maxsegsz) /
		    JAZZ_DMA_PAGE_SIZE + 1;
		jazz_dmatlb_free(tlbmap->vaddr, npte);
		free(tlbmap, M_DMAMAP);
	}

	_bus_dmamap_destroy(t, map);
}

/*
 * function for loading a direct-mapped DMA map with a linear buffer.
 */
int
jazz_bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	int error;

	if ((map->_dm_flags & BUS_DMA_ALLOCNOW) != 0) {
		/* just use pre-allocated DMA TLB for the buffer */
		jazz_tlbmap_t tlbmap;
		bus_size_t off;
		struct vmspace *vm;

		if (p != NULL) {
			vm = p->p_vmspace;
		} else {
			vm = vmspace_kernel();
		}

		tlbmap = (jazz_tlbmap_t)map->_dm_cookie;
		off = jazz_dma_page_offs(buf);
		jazz_dmatlb_map_va(vm, (vaddr_t)buf, buflen, tlbmap->ptebase);

		map->dm_segs[0].ds_addr = tlbmap->vaddr + off;
		map->dm_segs[0].ds_len = buflen;
		map->dm_segs[0]._ds_vaddr = (vaddr_t)buf;
		map->dm_mapsize = buflen;
		map->dm_nsegs = 1;
		map->_dm_vmspace = vm;

		if (buf >= (void *)MIPS_KSEG1_START &&
		    buf < (void *)MIPS_KSEG2_START)
			map->_dm_flags |= ARC_DMAMAP_COHERENT;

		return 0;
	}

	error = _bus_dmamap_load(t, map, buf, buflen, p, flags);
	if (error == 0) {
		/* allocate DMA TLB for each dmamap segment */
		error = jazz_bus_dmamap_alloc_sgmap(t, map->dm_segs,
		    map->dm_nsegs, map->_dm_boundary, flags);
	}
	return error;
}

/*
 * Like jazz_bus_dmamap_load(), but for mbufs.
 */
int
jazz_bus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map, struct mbuf *m0,
    int flags)
{
	int error;

	if ((map->_dm_flags & BUS_DMA_ALLOCNOW) != 0)
		/* BUS_DMA_ALLOCNOW is valid only for linear buffer. */
		return ENODEV; /* XXX which errno is better? */

	error = _bus_dmamap_load_mbuf(t, map, m0, flags);
	if (error == 0) {
		error = jazz_bus_dmamap_alloc_sgmap(t, map->dm_segs,
		    map->dm_nsegs, map->_dm_boundary, flags);
	}
	return error;
}

/*
 * Like jazz_bus_dmamap_load(), but for uios.
 */
int
jazz_bus_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio,
    int flags)
{
	int error;

	if ((map->_dm_flags & BUS_DMA_ALLOCNOW) != 0)
		/* BUS_DMA_ALLOCNOW is valid only for linear buffer. */
		return ENODEV; /* XXX which errno is better? */

	error = jazz_bus_dmamap_load_uio(t, map, uio, flags);
	if (error == 0) {
		error = jazz_bus_dmamap_alloc_sgmap(t, map->dm_segs,
		    map->dm_nsegs, map->_dm_boundary, flags);
	}
	return error;
}

/*
 * Like _bus_dmamap_load(), but for raw memory.
 */
int
jazz_bus_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{
	int error;

	if ((map->_dm_flags & BUS_DMA_ALLOCNOW) != 0)
		/* BUS_DMA_ALLOCNOW is valid only for linear buffer. */
		return ENODEV; /* XXX which errno is better? */

	error = _bus_dmamap_load_raw(t, map, segs, nsegs, size, flags);
	if (error == 0) {
		error = jazz_bus_dmamap_alloc_sgmap(t, map->dm_segs,
		    map->dm_nsegs, map->_dm_boundary, flags);
	}
	return error;
}

/*
 * unload a DMA map.
 */
void
jazz_bus_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{

	if ((map->_dm_flags & BUS_DMA_ALLOCNOW) != 0) {
		/* DMA TLB should be preserved */
		map->dm_mapsize = 0;
		map->dm_nsegs = 0;
		return;
	}

	jazz_bus_dmamap_free_sgmap(t, map->dm_segs, map->dm_nsegs);
	_bus_dmamap_unload(t, map);
}

/*
 * Function for MIPS3 DMA map synchronization.
 */
void
jazz_bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{

	/* Flush DMA TLB */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0)
		jazz_dmatlb_flush();

	return _bus_dmamap_sync(t, map, offset, len, ops);
}
