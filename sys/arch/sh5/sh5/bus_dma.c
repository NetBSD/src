/*	$NetBSD: bus_dma.c,v 1.7 2003/03/13 13:44:18 scw Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: bus_dma.c,v 1.7 2003/03/13 13:44:18 scw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/kcore.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/bus.h>
#include <machine/cacheops.h>

static int _bus_dmamap_create(void *, bus_size_t, int, bus_size_t,
		bus_size_t, int, bus_dmamap_t *);
static void _bus_dmamap_destroy(void *, bus_dmamap_t);
static int _bus_dmamap_load_direct(void *, bus_dmamap_t,
		void *, bus_size_t, struct proc *, int);
static int _bus_dmamap_load_mbuf_direct(void *,
		bus_dmamap_t, struct mbuf *, int);
static int _bus_dmamap_load_uio_direct(void *, bus_dmamap_t, struct uio *, int);
static int _bus_dmamap_load_raw_direct(void *,
		bus_dmamap_t, bus_dma_segment_t *, int, bus_size_t, int);
static void _bus_dmamap_unload(void *, bus_dmamap_t);
static void _bus_dmamap_sync(void *, bus_dmamap_t, bus_addr_t, bus_size_t, int);
static int _bus_dmamem_alloc(void *, bus_size_t, bus_size_t, bus_size_t,
		bus_dma_segment_t *, int, int *, int);
static void _bus_dmamem_free(void *, bus_dma_segment_t *, int);
static int _bus_dmamem_map(void *, bus_dma_segment_t *, int, size_t,
		caddr_t *, int);
static void _bus_dmamem_unmap(void *, caddr_t, size_t);
static paddr_t _bus_dmamem_mmap(void *, bus_dma_segment_t *, int,
		off_t, int, int);

static int _bus_dmamap_load_buffer_direct_common(void *,
    bus_dmamap_t, void *, bus_size_t, struct proc *, int,
    paddr_t *, int *, int);

static void _bus_dmamap_sync_helper(vaddr_t, paddr_t, vsize_t, int);

const struct sh5_bus_dma_tag _sh5_bus_dma_tag = {
	NULL,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load_direct,
	_bus_dmamap_load_mbuf_direct,
	_bus_dmamap_load_uio_direct,
	_bus_dmamap_load_raw_direct,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};

extern	phys_ram_seg_t mem_clusters[];

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
/*ARGSUSED*/
static int
_bus_dmamap_create(void *cookie, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct sh5_bus_dmamap *map;
	void *mapstore;
	size_t mapsize;

	/*
	 * Allcoate and initialize the DMA map.  The end of the map
	 * is a variable-sized array of segments, so we allocate enough
	 * room for them in one shot.
	 *
	 * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
	 * of ALLOCNOW notifes others that we've reserved these resources,
	 * and they are not to be freed.
	 *
	 * The bus_dmamap_t includes one bus_dma_segment_t, hence
	 * the (nsegments - 1).
	 */
	mapsize = sizeof(struct sh5_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	memset(mapstore, 0, mapsize);
	map = (struct sh5_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->dm_mapsize = 0;		/* no valid mappings */
	map->dm_nsegs = 0;

	*dmamp = map;
	return (0);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
/*ARGSUSED*/
static void
_bus_dmamap_destroy(void *cookie, bus_dmamap_t map)
{

	free(map, M_DMAMAP);
}

/*
 * Utility function to load a linear buffer.  lastaddrp holds state
 * between invocations (for multiple-buffer loads).  segp contains
 * the starting segment on entrance, and the ending segment on exit.
 * first indicates if this is the first invocation of this function.
 */
/*ARGSUSED*/
static int
_bus_dmamap_load_buffer_direct_common(void *cookie, bus_dmamap_t map,
    void *buf, bus_size_t buflen, struct proc *p, int flags,
    paddr_t *lastaddrp, int *segp, int first)
{
	bus_size_t sgsize;
	bus_addr_t curaddr, lastaddr, baddr, bmask;
	vaddr_t vaddr = (vaddr_t)buf;
	paddr_t paddr;
	pmap_t pm;
	int seg, cacheable, coherent = 1;

	lastaddr = *lastaddrp;
	bmask = ~(map->_dm_boundary - 1);

	pm = p ? p->p_vmspace->vm_map.pmap : pmap_kernel();

	for (seg = *segp; buflen > 0 ; ) {
		/*
		 * Get the physical address for this segment.
		 */
		(void) pmap_extract(pm, vaddr, &paddr);
		cacheable = pmap_page_is_cacheable(pm, vaddr);

		curaddr = (bus_addr_t) paddr;

		if (cacheable) {
			coherent = 0;
			cacheable = 0;
		} else
			cacheable = BUS_DMA_COHERENT;

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = NBPG - ((u_long)vaddr & PGOFSET);
		if (buflen < sgsize)
			sgsize = buflen;

		/*
		 * Make sure we don't cross any boundaries.
		 */
		if (map->_dm_boundary > 0) {
			baddr = (curaddr + map->_dm_boundary) & bmask;
			if (sgsize > (baddr - curaddr))
				sgsize = (baddr - curaddr);
		}

		/*
		 * Insert chunk into a segment, coalescing with
		 * the previous segment if possible.
		 */
		if (first) {
			map->dm_segs[seg].ds_addr =
			    map->dm_segs[seg]._ds_cpuaddr = curaddr;
			map->dm_segs[seg].ds_len = sgsize;
			map->dm_segs[seg]._ds_vaddr = vaddr;
			map->dm_segs[seg]._ds_flags = cacheable;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    map->dm_segs[seg]._ds_flags == cacheable &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			     map->_dm_maxsegsz &&
			    (map->_dm_boundary == 0 ||
			     (map->dm_segs[seg].ds_addr & bmask) ==
			     (curaddr & bmask)))
				map->dm_segs[seg].ds_len += sgsize;
			else {
				if (++seg >= map->_dm_segcnt)
					break;
				map->dm_segs[seg].ds_addr =
				    map->dm_segs[seg]._ds_cpuaddr = curaddr;
				map->dm_segs[seg].ds_len = sgsize;
				map->dm_segs[seg]._ds_vaddr = vaddr;
				map->dm_segs[seg]._ds_flags = cacheable;
			}
		}

		lastaddr = curaddr + sgsize;
		vaddr += sgsize;
		buflen -= sgsize;
	}

	*segp = seg;
	*lastaddrp = lastaddr;

	/*
	 * If the map is marked as being coherent, but we just added
	 * at least one non-coherent segment, kill the coherent flag
	 * in the map.
	 */
	if (coherent == 0 && map->_dm_flags & BUS_DMA_COHERENT)
		map->_dm_flags &= ~BUS_DMA_COHERENT;

	/*
	 * Did we fit?
	 */
	if (buflen != 0) {
		/*
		 * If there is a chained window, we will automatically
		 * fall back to it.
		 */
		return (EFBIG);		/* XXX better return value here? */
	}

	return (0);
}

/*
 * Common function for loading a direct-mapped DMA map with a linear
 * buffer.  Called by bus-specific DMA map load functions with the
 * OR value appropriate for indicating "direct-mapped" for that
 * chipset.
 */
/*ARGSUSED*/
static int
_bus_dmamap_load_direct(void *cookie, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	paddr_t lastaddr;
	int seg, error;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
		return (EINVAL);

	if (buflen == 0)
		return (0);

	seg = 0;
	map->_dm_flags |= BUS_DMA_COHERENT;
	error = _bus_dmamap_load_buffer_direct_common(cookie, map, buf, buflen,
	    p, flags, &lastaddr, &seg, 1);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _bus_dmamap_load_direct_common(), but for mbufs.
 */
/*ARGSUSED*/
static int
_bus_dmamap_load_mbuf_direct(void *cookie, bus_dmamap_t map, struct mbuf *m0,
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

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("_bus_dmamap_load_mbuf_direct: no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return (EINVAL);

	first = 1;
	seg = 0;
	error = 0;
	map->_dm_flags |= BUS_DMA_COHERENT;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		error = _bus_dmamap_load_buffer_direct_common(cookie, map,
		    m->m_data, m->m_len, NULL, flags, &lastaddr, &seg, first);
		first = 0;
	}
	if (error == 0) {
		map->dm_mapsize = m0->m_pkthdr.len;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _bus_dmamap_load_direct_common(), but for uios.
 */
/*ARGSUSED*/
static int
_bus_dmamap_load_uio_direct(void *cookie, bus_dmamap_t map, struct uio *uio,
    int flags)
{
	paddr_t lastaddr;
	int seg, i, error, first;
	bus_size_t minlen, resid;
	struct proc *p = NULL;
	struct iovec *iov;
	caddr_t addr;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	resid = uio->uio_resid;
	iov = uio->uio_iov;

	if (uio->uio_segflg == UIO_USERSPACE) {
		p = uio->uio_procp;
#ifdef DIAGNOSTIC
		if (p == NULL)
			panic("_bus_dmamap_load_uio_direct: USERSPACE but no proc");
#endif
	}

	first = 1;
	seg = 0;
	error = 0;
	map->_dm_flags |= BUS_DMA_COHERENT;
	for (i = 0; i < uio->uio_iovcnt && resid != 0 && error == 0; i++) {
		/*
		 * Now at the first iovec to load.  Load each iovec
		 * until we have exhausted the residual count.
		 */
		minlen = resid < iov[i].iov_len ? resid : iov[i].iov_len;
		addr = (caddr_t)iov[i].iov_base;

		if (minlen == 0)
			continue;

		error = _bus_dmamap_load_buffer_direct_common(cookie, map,
		    addr, minlen, p, flags, &lastaddr, &seg, first);
		first = 0;

		resid -= minlen;
	}
	if (error == 0) {
		map->dm_mapsize = uio->uio_resid;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _bus_dmamap_load_direct_common(), but for raw memory.
 */
/*ARGSUSED*/
static int
_bus_dmamap_load_raw_direct(void *cookie, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{
	int i, j;

	/* @@@ This routine doesn't enforce map boundary requirement
	 * @@@ perhaps it should return an error instead of panicing
	 */

#ifdef DIAGNOSTIC
	if (map->_dm_size < size)
		panic("_bus_dmamap_load_raw_direct: size is too large for map");
	if (map->_dm_segcnt < nsegs)
		panic("_bus_dmamap_load_raw_direct: too many segments for map");
#endif

	for (i = j = 0; i < nsegs; i++) {
		if (segs[i].ds_len == 0)
			continue;
#ifdef DIAGNOSTIC
		if (map->_dm_maxsegsz < map->dm_segs[i].ds_len)
			panic("_bus_dmamap_load_raw_direct: segment too large for map");
#endif
		map->dm_segs[j++] = segs[i];
	}

	map->dm_nsegs   = j;
	map->dm_mapsize = size;

	return (0);
}

/*
 * Common function for unloading a DMA map.  May be called by
 * chipset-specific DMA map unload functions.
 */
/*ARGSUSED*/
static void
_bus_dmamap_unload(void *cookie, bus_dmamap_t map)
{

	/*
	 * No resources to free; just mark the mappings as
	 * invalid.
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	map->_dm_flags &= ~BUS_DMA_COHERENT;
}

/*
 * DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 */
/*ARGSUSED*/
static void
_bus_dmamap_sync(void *cookie, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	vsize_t seglen;
	vaddr_t va;
	paddr_t pa;
	int i, inv;

	/* If the whole DMA map is uncached, do nothing */
	if (map->_dm_flags & BUS_DMA_COHERENT)
		return;

	switch (ops & (BUS_DMASYNC_PREWRITE|BUS_DMASYNC_PREREAD)) {
	case BUS_DMASYNC_PREWRITE|BUS_DMASYNC_PREREAD:
		inv = 0;
		break;

	case BUS_DMASYNC_PREREAD:
		inv = 1;
		break;

	case BUS_DMASYNC_PREWRITE:
		inv = 0;
		break;

	default:
		return;
	}

	for (i = 0; i < map->dm_nsegs && len > 0; i++) {
		if (map->dm_segs[i].ds_len <= offset) {
			/* Segment irrelevant - before requested offset */
			offset -= map->dm_segs[i].ds_len;
			continue;
		}

		seglen = (vsize_t)(map->dm_segs[i].ds_len - offset);
		if (seglen > (vsize_t)len)
			seglen = (vsize_t)len;
		len -= (bus_size_t)seglen;

		/* Ignore cache-inhibited segments */
		if (map->dm_segs[i]._ds_flags & BUS_DMA_COHERENT)
			continue;

		pa = (paddr_t)(map->dm_segs[i]._ds_cpuaddr + offset);
		va = map->dm_segs[i]._ds_vaddr + offset;

		_bus_dmamap_sync_helper(va, pa, seglen, inv);
	}
}

static void
_bus_dmamap_sync_helper(vaddr_t va, paddr_t pa, vsize_t len, int inv)
{
	void (*op)(vaddr_t, paddr_t, vsize_t);
	vsize_t bytes;

	KDASSERT((int)(va & (NBPG - 1)) == (int)(pa & (NBPG - 1)));

	if (len == 0)
		return;

	op = inv ? cpu_cache_dinv : cpu_cache_dpurge;

	/*
	 * Align the region to a cache-line boundary by always purging
	 * the first partial cache-line.
	 */
	if ((va & (SH5_CACHELINE_SIZE - 1)) != 0) {
		bytes = (vsize_t)va & (SH5_CACHELINE_SIZE - 1);
		bytes = min(SH5_CACHELINE_SIZE - bytes, len);
		cpu_cache_dpurge(va & ~(SH5_CACHELINE_SIZE - 1),
		    pa & ~(SH5_CACHELINE_SIZE - 1), SH5_CACHELINE_SIZE);
		if ((len -= bytes) == 0)
			return;
		pa += bytes;
		va += bytes;
	}

	/*
	 * Align the length to a multiple of the cache-line size by always
	 * purging the last partial cache-line
	 */
	if ((len & (SH5_CACHELINE_SIZE - 1)) != 0) {
		bytes = min((vsize_t)len & (SH5_CACHELINE_SIZE - 1), len);
		cpu_cache_dpurge(va + (len - bytes), pa + (len - bytes),
		    SH5_CACHELINE_SIZE);
		if ((len -= bytes) == 0)
			return;
	}

	/*
	 * For KSEG0, we don't need to flush the cache on a page-by-page
	 * basis.
	 */
	if (va < SH5_KSEG1_BASE && va >= SH5_KSEG0_BASE) {
		(*op)(va, pa, len);
		return;
	}

	/*
	 * Align the region to a page boundary
	 */
	if ((va & (NBPG-1)) != 0) {
		bytes = min(NBPG - (vsize_t)(va & (NBPG - 1)), len);
		(*op)(va, pa, bytes);
		len -= bytes;
		pa += bytes;
		va += bytes;
	}

	/*
	 * Do things one page at a time
	 */
	while (len) {
		bytes = min(NBPG, len);
		(*op)(va, pa, bytes);
		len -= bytes;
		pa += bytes;
		va += bytes;
	}
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
/*ARGSUSED*/
static int
_bus_dmamem_alloc_common(void *cookie, bus_addr_t low, bus_addr_t high,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary,
    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags)
{
	paddr_t curaddr, lastaddr;
	struct vm_page *m;    
	struct pglist mlist;
	int curseg, error;

	/* Always round the size. */
	size = round_page(size);
	high -= PAGE_SIZE;

	/*
	 * Allocate pages from the VM system.
	 *
	 * uvm_pglistalloc() also currently ignores the 'nsegs' parameter,
	 * and always returns only one (contiguous) segment.
	 */
	error = uvm_pglistalloc(size, low, high, alignment, boundary,
	    &mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
	if (error)
		return (error);

	/*
	 * Compute the location, size, and number of segments actually
	 * returned by the VM code.
	 */
	m = mlist.tqh_first;
	curseg = 0;
	lastaddr = VM_PAGE_TO_PHYS(m);
	segs[curseg].ds_addr = segs[curseg]._ds_cpuaddr = lastaddr;
	segs[curseg].ds_len = PAGE_SIZE;
	segs[curseg]._ds_flags = 0;
	m = m->pageq.tqe_next;

	for (; m != NULL; m = m->pageq.tqe_next) {
		if (curseg > nsegs) {
#ifdef DIAGNOSTIC
			printf("_bus_dmamem_alloc_common: too many segments\n");
#ifdef DEBUG
			panic("_bus_dmamem_alloc_common");
#endif
#endif
			uvm_pglistfree(&mlist);
			return (-1);
		}

		curaddr = VM_PAGE_TO_PHYS(m);
#ifdef DIAGNOSTIC
		if (curaddr < low || curaddr > high) {
			printf("uvm_pglistalloc returned non-sensical"
			    " address 0x%lx\n", curaddr);
			panic("_bus_dmamem_alloc_common");
		}
#endif
		if (curaddr == (lastaddr + PAGE_SIZE))
			segs[curseg].ds_len += PAGE_SIZE;
		else {
			curseg++;
			segs[curseg].ds_addr =
			    segs[curseg]._ds_cpuaddr = curaddr;
			segs[curseg].ds_len = PAGE_SIZE;
			segs[curseg]._ds_flags = 0;
		}
		lastaddr = curaddr;
	}

	*rsegs = curseg + 1;

	return (0);
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
static int
_bus_dmamem_alloc(void *cookie, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs,
    int nsegs, int *rsegs, int flags)
{
	extern paddr_t avail_start;
	extern paddr_t avail_end;
	bus_addr_t high;

	/*
	 * Assume any memory will do for now.
	 */
	high = (bus_addr_t) avail_end;

	return _bus_dmamem_alloc_common(cookie, avail_start, high,
	    size, alignment, boundary, segs, nsegs, rsegs, flags);
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
/*ARGSUSED*/
static void
_bus_dmamem_free(void *cookie, bus_dma_segment_t *segs, int nsegs)
{
	struct vm_page *m;
	bus_addr_t addr;
	struct pglist mlist;
	int curseg;

	/*
	 * Build a list of pages to free back to the VM system.
	 */
	TAILQ_INIT(&mlist);
	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg]._ds_cpuaddr;
		    addr < (segs[curseg]._ds_cpuaddr + segs[curseg].ds_len);
		    addr += PAGE_SIZE) {
			m = PHYS_TO_VM_PAGE(addr);
			TAILQ_INSERT_TAIL(&mlist, m, pageq);
		}
	}

	uvm_pglistfree(&mlist);
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
/*ARGSUSED*/
static int
_bus_dmamem_map(void *cookie, bus_dma_segment_t *segs, int nsegs,
    size_t size, caddr_t *kvap, int flags)
{
	vaddr_t va;
	bus_addr_t addr;
	int curseg;

	size = round_page(size);

	va = uvm_km_valloc(kernel_map, size);

	if (va == 0)
		return (ENOMEM);

	*kvap = (caddr_t)va;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg]._ds_cpuaddr;
		    addr < (segs[curseg]._ds_cpuaddr + segs[curseg].ds_len);
		    addr += NBPG, va += NBPG, size -= NBPG) {
			if (size == 0)
				panic("_bus_dmamem_map: size botch");

			pmap_enter(pmap_kernel(), va, addr,
			    VM_PROT_READ | VM_PROT_WRITE,
			    VM_PROT_READ | VM_PROT_WRITE |
			    PMAP_WIRED   | PMAP_UNMANAGED |
			    ((flags & BUS_DMA_COHERENT) ? PMAP_NC : 0));

			segs[curseg]._ds_flags &= ~BUS_DMA_COHERENT;
			segs[curseg]._ds_flags |= (flags & BUS_DMA_COHERENT);
		}
	}
	pmap_update(pmap_kernel());

	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
/*ARGSUSED*/
static void
_bus_dmamem_unmap(void *cookie, caddr_t kva, size_t size)
{

#ifdef DIAGNOSTIC
	if ((u_long)kva & PGOFSET)
		panic("_bus_dmamem_unmap");
#endif

	size = round_page(size);

	uvm_km_free(kernel_map, (vaddr_t)kva, size);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
/*ARGSUSED*/
static paddr_t
_bus_dmamem_mmap(void *cookie, bus_dma_segment_t *segs, int nsegs,
    off_t off, int prot, int flags)
{
	int i;

	for (i = 0; i < nsegs; i++) {
#ifdef DIAGNOSTIC
		if (off & PGOFSET)
			panic("_bus_dmamem_mmap: offset unaligned");
		if (segs[i]._ds_cpuaddr & PGOFSET)
			panic("_bus_dmamem_mmap: segment unaligned");
		if (segs[i].ds_len & PGOFSET)
			panic("_bus_dmamem_mmap: segment size not multiple"
			    " of page size");
#endif
		if (off >= segs[i].ds_len) {
			off -= segs[i].ds_len;
			continue;
		}

		/*
		 * XXXSCW: What about BUS_DMA_COHERENT ??
		 */

		return (sh5_btop((caddr_t)(intptr_t)(segs[i]._ds_cpuaddr +
		    off)));
	}

	/* Page not found. */
	return (-1);
}
