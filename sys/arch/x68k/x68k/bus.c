/*	$NetBSD: bus.c,v 1.37 2022/05/24 19:55:09 andvar Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
 * bus_space(9) and bus_dma(9) implementation for NetBSD/x68k.
 * These are default implementations; some buses may use their own.
 */

#include "opt_m68k_arch.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus.c,v 1.37 2022/05/24 19:55:09 andvar Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <m68k/cacheops.h>
#include <machine/bus.h>

#include <dev/bus_dma/bus_dmamem_common.h>

#if defined(M68040) || defined(M68060)
static inline void dmasync_flush(bus_addr_t, bus_size_t);
static inline void dmasync_inval(bus_addr_t, bus_size_t);
#endif

int
x68k_bus_space_alloc(bus_space_tag_t t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	return (EINVAL);
}

void
x68k_bus_space_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	panic("bus_space_free: shouldn't be here");
}


extern paddr_t avail_end;

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
x68k_bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct x68k_bus_dmamap *map;
	void *mapstore;
	size_t mapsize;

	/*
	 * Allocate and initialize the DMA map.  The end of the map
	 * is a variable-sized array of segments, so we allocate enough
	 * room for them in one shot.
	 *
	 * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
	 * of ALLOCNOW notifies others that we've reserved these resources,
	 * and they are not to be freed.
	 *
	 * The bus_dmamap_t includes one bus_dma_segment_t, hence
	 * the (nsegments - 1).
	 */
	mapsize = sizeof(struct x68k_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	memset(mapstore, 0, mapsize);
	map = (struct x68k_bus_dmamap *)mapstore;
	map->x68k_dm_size = size;
	map->x68k_dm_segcnt = nsegments;
	map->x68k_dm_maxmaxsegsz = maxsegsz;
	map->x68k_dm_boundary = boundary;
	map->x68k_dm_bounce_thresh = t->_bounce_thresh;
	map->x68k_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->dm_maxsegsz = maxsegsz;
	map->dm_mapsize = 0;		/* no valid mappings */
	map->dm_nsegs = 0;

	*dmamp = map;
	return (0);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
x68k_bus_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
{

	free(map, M_DMAMAP);
}

/*
 * Common function for loading a DMA map with a linear buffer.  May
 * be called by bus-specific DMA map load functions.
 */
int
x68k_bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	paddr_t lastaddr;
	int seg, error;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->x68k_dm_maxmaxsegsz);

	if (buflen > map->x68k_dm_size)
		return (EINVAL);

	seg = 0;
	error = x68k_bus_dmamap_load_buffer(map, buf, buflen, p, flags,
	    &lastaddr, &seg, 1);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like x68k_bus_dmamap_load(), but for mbufs.
 */
int
x68k_bus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map, struct mbuf *m0,
    int flags)
{
	paddr_t lastaddr;
	int seg, error, first;
	struct mbuf *m;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->x68k_dm_maxmaxsegsz);

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("x68k_bus_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->x68k_dm_size)
		return (EINVAL);

	first = 1;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		error = x68k_bus_dmamap_load_buffer(map, m->m_data, m->m_len,
		    NULL, flags, &lastaddr, &seg, first);
		first = 0;
	}
	if (error == 0) {
		map->dm_mapsize = m0->m_pkthdr.len;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like x68k_bus_dmamap_load(), but for uios.
 */
int
x68k_bus_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio,
    int flags)
{
#if 0
	paddr_t lastaddr;
	int seg, i, error, first;
	bus_size_t minlen, resid;
	struct proc *p = NULL;
	struct iovec *iov;
	void *addr;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->x68k_dm_maxmaxsegsz);

	resid = uio->uio_resid;
	iov = uio->uio_iov;

	if (uio->uio_segflg == UIO_USERSPACE) {
		p = uio->uio_lwp ? uio->uio_lwp->l_proc : NULL;
#ifdef DIAGNOSTIC
		if (p == NULL)
			panic("_bus_dmamap_load_uio: USERSPACE but no proc");
#endif
	}

	first = 1;
	seg = 0;
	error = 0;
	for (i = 0; i < uio->uio_iovcnt && resid != 0 && error == 0; i++) {
		/*
		 * Now at the first iovec to load.  Load each iovec
		 * until we have exhausted the residual count.
		 */
		minlen = resid < iov[i].iov_len ? resid : iov[i].iov_len;
		addr = (void *)iov[i].iov_base;

		error = x68k_bus_dmamap_load_buffer(map, addr, minlen,
		    p, flags, &lastaddr, &seg, first);
		first = 0;

		resid -= minlen;
	}
	if (error == 0) {
		map->dm_mapsize = uio->uio_resid;
		map->dm_nsegs = seg + 1;
	}
	return (error);
#else
	panic ("x68k_bus_dmamap_load_uio: not implemented");
#endif
}

/*
 * Like x68k_bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
x68k_bus_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{

	panic("x68k_bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * bus-specific DMA map unload functions.
 */
void
x68k_bus_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{

	/*
	 * No resources to free; just mark the mappings as
	 * invalid.
	 */
	map->dm_maxsegsz = map->x68k_dm_maxmaxsegsz;
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
}

#if defined(M68040) || defined(M68060)
static inline void
dmasync_flush(bus_addr_t addr, bus_size_t len)
{
	bus_addr_t end = addr+len;

	if (len <= 1024) {
		addr = addr & ~0xF;

		do {
			DCFL(addr);
			addr += 16;
		} while (addr < end);
	} else {
		addr = m68k_trunc_page(addr);

		do {
			DCFP(addr);
			addr += PAGE_SIZE;
		} while (addr < end);
	}
}

static inline void
dmasync_inval(bus_addr_t addr, bus_size_t len)
{
	bus_addr_t end = addr+len;

	if (len <= 1024) {
		addr = addr & ~0xF;

		do {
			DCFL(addr);
			ICPL(addr);
			addr += 16;
		} while (addr < end);
	} else {
		addr = m68k_trunc_page(addr);

		do {
			DCPL(addr);
			ICPP(addr);
			addr += PAGE_SIZE;
		} while (addr < end);
	}
}
#endif

/*
 * Common function for DMA map synchronization.  May be called
 * by bus-specific DMA map synchronization functions.
 */
void
x68k_bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
#if defined(M68040) || defined(M68060)
	bus_dma_segment_t *ds = map->dm_segs;
	bus_addr_t seg;
	int i;

	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_POSTWRITE)) == 0)
		return;
#if defined(M68020) || defined(M68030)
	if (mmutype != MMU_68040) {
		if ((ops & BUS_DMASYNC_POSTWRITE) == 0)
			return;	/* no copyback cache */
		ICIA();		/* no per-page/per-line control */
		DCIA();
		return;
	}		
#endif
	if (offset >= map->dm_mapsize)
		return;	/* driver bug; warn it? */
	if (offset+len > map->dm_mapsize)
		len = map->dm_mapsize; /* driver bug; warn it? */

	i = 0;
	while (ds[i].ds_len <= offset) {
		offset -= ds[i++].ds_len;
		continue;
	}
	while (len > 0) {
		seg = ds[i].ds_len - offset;
		if (seg > len)
			seg = len;
		if (mmutype == MMU_68040 && (ops & BUS_DMASYNC_PREWRITE))
			dmasync_flush(ds[i].ds_addr+offset, seg);
		if (ops & BUS_DMASYNC_POSTREAD)
			dmasync_inval(ds[i].ds_addr+offset, seg);
		offset = 0;
		len -= seg;
		i++;
	}
#else  /* no 040/060 */
	if ((ops & BUS_DMASYNC_POSTWRITE)) {
		ICIA();		/* no per-page/per-line control */
		DCIA();
	}
#endif
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
x68k_bus_dmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags)
{

	return (x68k_bus_dmamem_alloc_range(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags, 0, trunc_page(avail_end)));
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
x68k_bus_dmamem_free(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs)
{

	_bus_dmamem_free_common(t, segs, nsegs);
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
x68k_bus_dmamem_map(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
    size_t size, void **kvap, int flags)
{

	/* XXX BUS_DMA_COHERENT */
	return (_bus_dmamem_map_common(t, segs, nsegs, size, kvap, flags, 0));
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
x68k_bus_dmamem_unmap(bus_dma_tag_t t, void *kva, size_t size)
{

	_bus_dmamem_unmap_common(t, kva, size);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
x68k_bus_dmamem_mmap(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
    off_t off, int prot, int flags)
{
	bus_addr_t rv;

	rv = _bus_dmamem_mmap_common(t, segs, nsegs, off, prot, flags);
	if (rv == (bus_addr_t)-1)
		return (-1);
	
	return (m68k_btop((char *)rv));
}


/**********************************************************************
 * DMA utility functions
 **********************************************************************/

/*
 * Utility function to load a linear buffer.  lastaddrp holds state
 * between invocations (for multiple-buffer loads).  segp contains
 * the starting segment on entrance, and the ending segment on exit.
 * first indicates if this is the first invocation of this function.
 */
int
x68k_bus_dmamap_load_buffer(bus_dmamap_t map, void *buf, bus_size_t buflen,
     struct proc *p, int flags, paddr_t *lastaddrp, int *segp, int first)
{
	bus_size_t sgsize;
	bus_addr_t curaddr, lastaddr, baddr, bmask;
	vaddr_t vaddr = (vaddr_t)buf;
	int seg;
	pmap_t pmap;

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	lastaddr = *lastaddrp;
	bmask  = ~(map->x68k_dm_boundary - 1);

	for (seg = *segp; buflen > 0 ; ) {
		/*
		 * Get the physical address for this segment.
		 */
		(void) pmap_extract(pmap, vaddr, &curaddr);

		/*
		 * If we're beyond the bounce threshold, notify
		 * the caller.
		 */
		if (map->x68k_dm_bounce_thresh != 0 &&
		    curaddr >= map->x68k_dm_bounce_thresh)
			return (EINVAL);

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = PAGE_SIZE - m68k_page_offset(vaddr);
		if (buflen < sgsize)
			sgsize = buflen;

		/*
		 * Make sure we don't cross any boundaries.
		 */
		if (map->x68k_dm_boundary > 0) {
			baddr = (curaddr + map->x68k_dm_boundary) & bmask;
			if (sgsize > (baddr - curaddr))
				sgsize = (baddr - curaddr);
		}

		/*
		 * Insert chunk into a segment, coalescing with
		 * previous segment if possible.
		 */
		if (first) {
			map->dm_segs[seg].ds_addr = curaddr;
			map->dm_segs[seg].ds_len = sgsize;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			     map->dm_maxsegsz &&
			    (map->x68k_dm_boundary == 0 ||
			     (map->dm_segs[seg].ds_addr & bmask) ==
			     (curaddr & bmask)))
				map->dm_segs[seg].ds_len += sgsize;
			else {
				if (++seg >= map->x68k_dm_segcnt)
					break;
				map->dm_segs[seg].ds_addr = curaddr;
				map->dm_segs[seg].ds_len = sgsize;
			}
		}

		lastaddr = curaddr + sgsize;
		vaddr += sgsize;
		buflen -= sgsize;
	}

	*segp = seg;
	*lastaddrp = lastaddr;

	/*
	 * Did we fit?
	 */
	if (buflen != 0)
		return (EFBIG);		/* XXX better return value here? */
	return (0);
}

/*
 * Allocate physical memory from the given physical address range.
 * Called by DMA-safe memory allocation methods.
 */
int
x68k_bus_dmamem_alloc_range(bus_dma_tag_t t, bus_size_t size,
    bus_size_t alignment, bus_size_t boundary, bus_dma_segment_t *segs,
    int nsegs, int *rsegs, int flags, paddr_t low, paddr_t high)
{

	return (_bus_dmamem_alloc_range_common(t, size, alignment, boundary,
					       segs, nsegs, rsegs, flags,
					       low, high));
}
