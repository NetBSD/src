/*	$NetBSD: gapspci_dma.c,v 1.7 2002/06/02 14:44:44 drochner Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Bus DMA implementation for the SEGA GAPS PCI bridge.
 *
 * NOTE: We only implement a small subset of what the bus_space(9)
 * API specifies.  Right now, the GAPS PCI bridge is only used for
 * the Dreamcast Broadband Adatper, so we only provide what the
 * pci(4) and rtk(4) drivers need.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/param.h>
#include <sys/systm.h> 
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <machine/cpu.h> 
#include <machine/bus.h>

#include <dev/pci/pcivar.h>

#include <dreamcast/dev/g2/gapspcivar.h>

#include <uvm/uvm_extern.h>

int	gaps_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *);
void	gaps_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
int	gaps_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *, bus_size_t,
	    struct proc *, int);
int	gaps_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t, struct mbuf *, int);
int	gaps_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t, struct uio *, int);
int	gaps_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t, bus_dma_segment_t *,
	    int, bus_size_t, int);
void	gaps_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	gaps_dmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int);

int	gaps_dmamem_alloc(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary, bus_dma_segment_t *segs,
	    int nsegs, int *rsegs, int flags);
void	gaps_dmamem_free(bus_dma_tag_t tag, bus_dma_segment_t *segs, int nsegs);
int	gaps_dmamem_map(bus_dma_tag_t tag, bus_dma_segment_t *segs, int nsegs,
	    size_t size, caddr_t *kvap, int flags);
void	gaps_dmamem_unmap(bus_dma_tag_t tag, caddr_t kva, size_t size);
paddr_t	gaps_dmamem_mmap(bus_dma_tag_t tag, bus_dma_segment_t *segs, int nsegs,
	    off_t off, int prot, int flags);

void
gaps_dma_init(struct gaps_softc *sc)
{
	bus_dma_tag_t t = &sc->sc_dmat;

	memset(t, 0, sizeof(*t));

	t->_cookie = sc;
	t->_dmamap_create = gaps_dmamap_create;
	t->_dmamap_destroy = gaps_dmamap_destroy;
	t->_dmamap_load = gaps_dmamap_load;
	t->_dmamap_load_mbuf = gaps_dmamap_load_mbuf;
	t->_dmamap_load_uio = gaps_dmamap_load_uio;
	t->_dmamap_load_raw = gaps_dmamap_load_raw;
	t->_dmamap_unload = gaps_dmamap_unload;
	t->_dmamap_sync = gaps_dmamap_sync;

	t->_dmamem_alloc = gaps_dmamem_alloc;
	t->_dmamem_free = gaps_dmamem_free;
	t->_dmamem_map = gaps_dmamem_map;
	t->_dmamem_unmap = gaps_dmamem_unmap;
	t->_dmamem_mmap = gaps_dmamem_mmap;

	/*
	 * The GAPS PCI bridge has 32k of DMA memory.  We manage it
	 * with an extent map.
	 */
	sc->sc_dma_ex = extent_create("gaps dma",
	    sc->sc_dmabase, sc->sc_dmabase + (sc->sc_dmasize - 1),
	    M_DEVBUF, NULL, 0, EX_WAITOK | EXF_NOCOALESCE);

	if (bus_space_map(sc->sc_memt, sc->sc_dmabase, sc->sc_dmasize,
	    0, &sc->sc_dma_memh) != 0)
		panic("gaps_dma_init: can't map SRAM buffer");
}

/*
 * A GAPS DMA map -- has the standard DMA map, plus some extra
 * housekeeping data.
 */
struct gaps_dmamap {
	struct dreamcast_bus_dmamap gd_dmamap;
	void *gd_origbuf;
	int gd_buftype;
};

#define	GAPS_DMA_BUFTYPE_INVALID	0
#define	GAPS_DMA_BUFTYPE_LINEAR		1
#define	GAPS_DMA_BUFTYPE_MBUF		2

int
gaps_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamap)
{
	struct gaps_softc *sc = t->_cookie;
	struct gaps_dmamap *gmap;
	bus_dmamap_t map;

	/*
	 * Allocate an initialize the DMA map.  The end of the map is
	 * a variable-sized array of segments, so we allocate enough
	 * room for them in one shot.  Since the DMA map always includes
	 * one segment, and we only support one segment, this is really
	 * easy.
	 *
	 * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
	 * of ALLOCNOW notifies others that we've reserved these resources
	 * and they are not to be freed.
	 */

	gmap = malloc(sizeof(*gmap), M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK);
	if (gmap == NULL)
		return (ENOMEM);

	memset(gmap, 0, sizeof(*gmap));

	gmap->gd_buftype = GAPS_DMA_BUFTYPE_INVALID;

	map = &gmap->gd_dmamap;

	map->_dm_size = size;
	map->_dm_segcnt = 1;
	map->_dm_maxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);

	if (flags & BUS_DMA_ALLOCNOW) {
		u_long res;
		int error;

		error = extent_alloc(sc->sc_dma_ex, size, 1024 /* XXX */,
		    map->_dm_boundary,
		    (flags & BUS_DMA_NOWAIT) ? EX_NOWAIT : EX_WAITOK, &res);
		if (error) {
			free(gmap, M_DEVBUF);
			return (error);
		}

		map->dm_segs[0].ds_addr = res;
		map->dm_segs[0].ds_len = size;
		
		map->dm_mapsize = size;
		map->dm_nsegs = 1;
	} else {
		map->dm_mapsize = 0;		/* no valid mappings */
		map->dm_nsegs = 0;
	}

	*dmamap = map;

	return (0);
}

void
gaps_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct gaps_softc *sc = t->_cookie;

	if (map->_dm_flags & BUS_DMA_ALLOCNOW) {
		(void) extent_free(sc->sc_dma_ex,
		    map->dm_segs[0].ds_addr,
		    map->dm_mapsize, EX_NOWAIT);
	}
	free(map, M_DMAMAP);
}

int
gaps_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *addr,
    bus_size_t size, struct proc *p, int flags)
{
	struct gaps_softc *sc = t->_cookie;
	struct gaps_dmamap *gmap = (void *) map;
	u_long res;
	int error;

	if ((map->_dm_flags & BUS_DMA_ALLOCNOW) == 0) {
		/*
		 * Make sure that on error condition we return
		 * "no valid mappings".
		 */
		map->dm_mapsize = 0;
		map->dm_nsegs = 0;
	}

	/* XXX Don't support DMA to process space right now. */
	if (p != NULL)
		return (EINVAL);

	if (size > map->_dm_size)
		return (EINVAL);

	error = extent_alloc(sc->sc_dma_ex, size, 1024 /* XXX */,
	    map->_dm_boundary,
	    (flags & BUS_DMA_NOWAIT) ? EX_NOWAIT : EX_WAITOK, &res);
	if (error)
		return (error);

	map->dm_segs[0].ds_addr = res;
	map->dm_segs[0].ds_len = size;

	gmap->gd_origbuf = addr;
	gmap->gd_buftype = GAPS_DMA_BUFTYPE_LINEAR;

	map->dm_mapsize = size;
	map->dm_nsegs = 1;

	return (0);
}

int
gaps_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map, struct mbuf *m0,
    int flags)
{
	struct gaps_softc *sc = t->_cookie;
	struct gaps_dmamap *gmap = (void *) map;
	u_long res;
	int error;

	if ((map->_dm_flags & BUS_DMA_ALLOCNOW) == 0) {
		/*
		 * Make sure that on error condition we return
		 * "no valid mappings".
		 */
		map->dm_mapsize = 0;
		map->dm_nsegs = 0;
	}

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("gaps_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return (EINVAL);

	error = extent_alloc(sc->sc_dma_ex, m0->m_pkthdr.len, 1024 /* XXX */,
	    map->_dm_boundary,
	    (flags & BUS_DMA_NOWAIT) ? EX_NOWAIT : EX_WAITOK, &res);
	if (error)
		return (error);

	map->dm_segs[0].ds_addr = res;
	map->dm_segs[0].ds_len = m0->m_pkthdr.len;

	gmap->gd_origbuf = m0;
	gmap->gd_buftype = GAPS_DMA_BUFTYPE_MBUF;

	map->dm_mapsize = m0->m_pkthdr.len;
	map->dm_nsegs = 1;

	return (0);
}

int
gaps_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio,
    int flags)
{

	printf("gaps_dmamap_load_uio: not implemented\n");
	return (EINVAL);
}

int
gaps_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{

	printf("gaps_dmamap_load_raw: not implemented\n");
	return (EINVAL);
}

void
gaps_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct gaps_softc *sc = t->_cookie;
	struct gaps_dmamap *gmap = (void *) map;

	if (gmap->gd_buftype == GAPS_DMA_BUFTYPE_INVALID) {
		printf("gaps_dmamap_unload: DMA map not loaded!\n");
		return;
	}

	if ((map->_dm_flags & BUS_DMA_ALLOCNOW) == 0) {
		(void) extent_free(sc->sc_dma_ex,
		    map->dm_segs[0].ds_addr,
		    map->dm_mapsize, EX_NOWAIT);

		map->dm_mapsize = 0;
		map->dm_nsegs = 0;
	}

	gmap->gd_buftype = GAPS_DMA_BUFTYPE_INVALID;
}

void
gaps_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	struct gaps_softc *sc = t->_cookie;
	struct gaps_dmamap *gmap = (void *) map;
	bus_addr_t dmaoff = map->dm_segs[0].ds_addr - sc->sc_dmabase;

	/*
	 * Mixing PRE and POST operations is not allowed.
	 */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	    (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		panic("gaps_dmamap_sync: mix PRE and POST");

#ifdef DIAGNOSTIC
	if ((ops & (BUS_DMASYNC_PREWRITE|BUS_DMASYNC_POSTREAD)) != 0) {
		if (offset >= map->dm_mapsize) {
			printf("offset 0x%lx mapsize 0x%lx\n",
			    offset, map->dm_mapsize);
			panic("gaps_dmamap_sync: bad offset");
		}
		if (len == 0 || (offset + len) > map->dm_mapsize) {
			printf("len 0x%lx offset 0x%lx mapsize 0x%lx\n",
			    len, offset, map->dm_mapsize);
			panic("gaps_dmamap_sync: bad length");
		}
	}
#endif

	switch (gmap->gd_buftype) {
	case GAPS_DMA_BUFTYPE_INVALID:
		printf("gaps_dmamap_sync: DMA map is not loaded!\n");
		return;

	case GAPS_DMA_BUFTYPE_LINEAR:
		/*
		 * Nothing to do for pre-read.
		 */

		if (ops & BUS_DMASYNC_PREWRITE) {
			/*
			 * Copy the caller's buffer to the SRAM buffer.
			 */
			bus_space_write_region_1(sc->sc_memt,
			    sc->sc_dma_memh,
			    dmaoff + offset,
			    (u_int8_t *)gmap->gd_origbuf + offset, len);
		}

		if (ops & BUS_DMASYNC_POSTREAD) {
			/*
			 * Copy the SRAM buffer to the caller's buffer.
			 */
			bus_space_read_region_1(sc->sc_memt,
			    sc->sc_dma_memh,
			    dmaoff + offset,
			    (u_int8_t *)gmap->gd_origbuf + offset, len);
		}

		/*
		 * Nothing to do for post-write.
		 */
		break;

	case GAPS_DMA_BUFTYPE_MBUF:
	    {
		struct mbuf *m, *m0 = gmap->gd_origbuf;
		bus_size_t minlen, moff;

		/*
		 * Nothing to do for pre-read.
		 */

		if (ops & BUS_DMASYNC_PREWRITE) {
			/*
			 * Copy the caller's buffer into the SRAM buffer.
			 */
			for (moff = offset, m = m0; m != NULL && len != 0;
			     m = m->m_next) {
				/* Find the beginning mbuf. */
				if (moff >= m->m_len) {
					moff -= m->m_len;
					continue;
				}

				/*
				 * Now at the first mbuf to sync; nail
				 * each one until we have exhausted the
				 * length.
				 */
				minlen = len < m->m_len - moff ?
				    len : m->m_len - moff;

				bus_space_write_region_1(sc->sc_memt,
				    sc->sc_dma_memh, dmaoff + offset,
				    mtod(m, u_int8_t *) + moff, minlen);

				moff = 0;
				len -= minlen;
				offset += minlen;
			}
		}

		if (ops & BUS_DMASYNC_POSTREAD) {
			/*
			 * Copy the SRAM buffer into the caller's buffer.
			 */
			for (moff = offset, m = m0; m != NULL && len != 0;
			     m = m->m_next) {
				/* Find the beginning mbuf. */
				if (moff >= m->m_len) {
					moff -= m->m_len;
					continue;
				}

				/*
				 * Now at the first mbuf to sync; nail
				 * each one until we have exhausted the
				 * length.
				 */
				minlen = len < m->m_len - moff ?
				    len : m->m_len - moff;

				bus_space_read_region_1(sc->sc_memt,
				    sc->sc_dma_memh, dmaoff + offset,
				    mtod(m, u_int8_t *) + moff, minlen);

				moff = 0;
				len -= minlen;
				offset += minlen;
			}
		}

		/*
		 * Nothing to do for post-write.
		 */
		break;
	    }

	default:
		printf("unknown buffer type %d\n", gmap->gd_buftype);
		panic("gaps_dmamap_sync");
	}
}

int
gaps_dmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags)
{
	extern paddr_t avail_start, avail_end;	/* from pmap.c */

	struct pglist mlist;
	paddr_t curaddr, lastaddr;
	struct vm_page *m;
	int curseg, error;

	/* Always round the size. */
	size = round_page(size);

	/*
	 * Allocate the pages from the VM system.
	 */
	error = uvm_pglistalloc(size, avail_start, avail_end - PAGE_SIZE,
	    alignment, boundary, &mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
	if (error)
		return (error);

	/*
	 * Compute the location, size, and number of segments actually
	 * returned by the VM code.
	 */
	m = mlist.tqh_first;
	curseg = 0;
	lastaddr = segs[curseg].ds_addr = VM_PAGE_TO_PHYS(m);
	segs[curseg].ds_len = PAGE_SIZE;
	m = TAILQ_NEXT(m, pageq);

	for (; m != NULL; m = TAILQ_NEXT(m, pageq)) {
		curaddr = VM_PAGE_TO_PHYS(m);
		if (curaddr == (lastaddr + PAGE_SIZE))
			segs[curseg].ds_len += PAGE_SIZE;
		else {
			curseg++;
			segs[curseg].ds_addr = curaddr;
			segs[curseg].ds_len = PAGE_SIZE;
		}
		lastaddr = curaddr;
	}

	*rsegs = curseg + 1;

	return (0);
}

void
gaps_dmamem_free(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs)
{
	struct pglist mlist;
	struct vm_page *m;
	bus_addr_t addr;
	int curseg;

	/*
	 * Build a list of pages to free back to the VM system.
	 */
	TAILQ_INIT(&mlist);
	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		     addr < segs[curseg].ds_addr + segs[curseg].ds_len;
		     addr += PAGE_SIZE) {
			m = PHYS_TO_VM_PAGE(addr);
			TAILQ_INSERT_TAIL(&mlist, m, pageq);
		}
	}

	uvm_pglistfree(&mlist);
}

int
gaps_dmamem_map(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
    size_t size, caddr_t *kvap, int flags)
{
	vaddr_t va;
	bus_addr_t addr;
	int curseg;

	/*
	 * If we're only mapping 1 segment, use P2SEG, to avoid
	 * TLB thrashing.
	 */
	if (nsegs == 1) {
		*kvap = (caddr_t) SH3_PHYS_TO_P2SEG(segs[0].ds_addr);
		return (0);
	}

	size = round_page(size);

	va = uvm_km_valloc(kernel_map, size);

	if (va == 0)
		return (ENOMEM);

	*kvap = (caddr_t) va;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		     addr < segs[curseg].ds_addr + segs[curseg].ds_len;
		     addr += PAGE_SIZE, va += PAGE_SIZE, size -= PAGE_SIZE) {
			if (size == 0)
				panic("gaps_dmamem_map: size botch");
			pmap_kenter_pa(va, addr,
			    VM_PROT_READ | VM_PROT_WRITE);
		}
	}
	pmap_update(pmap_kernel());

	return (0);
}

void
gaps_dmamem_unmap(bus_dma_tag_t t, caddr_t kva, size_t size)
{

#ifdef DIAGNOSTIC
	if ((u_long) kva & PAGE_MASK)
		panic("gaps_dmamem_unmap");
#endif
	
	/*
	 * Nothing to do if we mapped it with P2SEG.
	 */
	if (kva >= (caddr_t) SH3_P2SEG_BASE &&
	    kva <= (caddr_t) SH3_P2SEG_END)
		return;

	size = round_page(size);
	pmap_kremove((vaddr_t) kva, size);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, (vaddr_t) kva, size);
}

paddr_t
gaps_dmamem_mmap(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
    off_t off, int prot, int flags)
{

	/* Not implemented. */
	return (-1);
}
