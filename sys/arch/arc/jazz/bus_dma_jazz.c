/*	$NetBSD: bus_dma_jazz.c,v 1.3 2000/06/29 08:34:12 mrg Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#define _ARC_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <arc/jazz/jazzdmatlbreg.h>
#include <arc/jazz/jazzdmatlbvar.h>

static int	jazz_bus_dmamap_alloc_sgmap __P((bus_dma_tag_t,
		    bus_dma_segment_t *, int, bus_size_t, struct proc *, int));
static void	jazz_bus_dmamap_free_sgmap __P((bus_dma_tag_t,
		    bus_dma_segment_t *, int));

int	jazz_bus_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int));
int	jazz_bus_dmamap_load_mbuf __P((bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int));
int	jazz_bus_dmamap_load_uio __P((bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int));
int	jazz_bus_dmamap_load_raw __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int));
void	jazz_bus_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
void	jazz_mips1_bus_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_addr_t, bus_size_t, int));
void	jazz_mips3_bus_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_addr_t, bus_size_t, int));

void
jazz_bus_dma_tag_init(t)
	bus_dma_tag_t t;
{
	_bus_dma_tag_init(t);

	t->_dmamap_load = jazz_bus_dmamap_load;
	t->_dmamap_load_mbuf = jazz_bus_dmamap_load_mbuf;
	t->_dmamap_load_uio = jazz_bus_dmamap_load_uio;
	t->_dmamap_load_raw = jazz_bus_dmamap_load_raw;
	t->_dmamap_unload = jazz_bus_dmamap_unload;
#if defined(MIPS1) && defined(MIPS3)
	t->_dmamap_sync = (CPUISMIPS3) ?
	    jazz_mips3_bus_dmamap_sync : jazz_mips1_bus_dmamap_sync;
#elif defined(MIPS1)
	t->_dmamap_sync = jazz_mips1_bus_dmamap_sync;
#elif defined(MIPS3)
	t->_dmamap_sync = jazz_mips3_bus_dmamap_sync;
#else
#error neither MIPS1 nor MIPS3 is defined
#endif
	t->_dmamem_alloc = _bus_dmamem_alloc;
	t->_dmamem_free = _bus_dmamem_free;
}

static int
jazz_bus_dmamap_alloc_sgmap(t, segs, nsegs, boundary, p, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t boundary;
	struct proc *p;
	int flags;
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
			return (ENOMEM);
		segs[i].ds_addr = addr + off;

		jazz_dmatlb_map_pa(segs[i]._ds_paddr, segs[i].ds_len, dmapte);
	}
	return (0);
}

static void
jazz_bus_dmamap_free_sgmap(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
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
 * function for loading a direct-mapped DMA map with a linear buffer.
 */
int
jazz_bus_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	int error = _bus_dmamap_load(t, map, buf, buflen, p, flags);

	if (error == 0) {
		error = jazz_bus_dmamap_alloc_sgmap(t, map->dm_segs,
		    map->dm_nsegs, map->_dm_boundary, p, flags);
	}
	return (error);
}

/*
 * Like jazz_bus_dmamap_load(), but for mbufs.
 */
int
jazz_bus_dmamap_load_mbuf(t, map, m0, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m0;
	int flags;
{
	int error = _bus_dmamap_load_mbuf(t, map, m0, flags);

	if (error == 0) {
		error = jazz_bus_dmamap_alloc_sgmap(t, map->dm_segs,
		    map->dm_nsegs, map->_dm_boundary, NULL, flags);
	}
	return (error);
}

/*
 * Like jazz_bus_dmamap_load(), but for uios.
 */
int
jazz_bus_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{
	int error = jazz_bus_dmamap_load_uio(t, map, uio, flags);

	if (error == 0) {
		error = jazz_bus_dmamap_alloc_sgmap(t, map->dm_segs,
		    map->dm_nsegs, map->_dm_boundary,
		    uio->uio_segflg == UIO_USERSPACE ? uio->uio_procp : NULL,
		    flags);
	}
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for raw memory.
 */
int
jazz_bus_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{
	int error = _bus_dmamap_load_raw(t, map, segs, nsegs, size, flags);

	if (error == 0) {
		error = jazz_bus_dmamap_alloc_sgmap(t, map->dm_segs,
		    map->dm_nsegs, map->_dm_boundary, NULL, flags);
	}
	return (error);
}

/*
 * unload a DMA map.
 */
void
jazz_bus_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	jazz_bus_dmamap_free_sgmap(t, map->dm_segs, map->dm_nsegs);
	_bus_dmamap_unload(t, map);
}

#ifdef MIPS1
/*
 * Function for MIPS1 DMA map synchronization.
 */
void
jazz_mips1_bus_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{
	/* Flush DMA TLB */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0)
		jazz_dmatlb_flush();

	return (_mips1_bus_dmamap_sync(t, map, offset, len, ops));
}
#endif /* MIPS1 */

#ifdef MIPS3
/*
 * Function for MIPS3 DMA map synchronization.
 */
void
jazz_mips3_bus_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{
	/* Flush DMA TLB */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0)
		jazz_dmatlb_flush();

	return (_mips3_bus_dmamap_sync(t, map, offset, len, ops));
}
#endif /* MIPS3 */
