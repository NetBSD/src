/*	$NetBSD: bus_dma.c,v 1.26.4.1 2012/04/17 00:06:41 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: bus_dma.c,v 1.26.4.1 2012/04/17 00:06:41 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#define _MIPSCO_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <common/bus_dma/bus_dmamem_common.h>

#include <mips/cache.h>

paddr_t	kvtophys(vaddr_t);	/* XXX */

static int	_bus_dmamap_load_buffer(bus_dma_tag_t, bus_dmamap_t,
		    void *, bus_size_t, struct vmspace *, int, paddr_t *,
		    int *, int);

void
_bus_dma_tag_init(bus_dma_tag_t t)
{
	t->dma_offset = 0;

	t->_dmamap_create = _bus_dmamap_create;
	t->_dmamap_destroy = _bus_dmamap_destroy;
	t->_dmamap_load = _bus_dmamap_load;
	t->_dmamap_load_mbuf = _bus_dmamap_load_mbuf;
	t->_dmamap_load_uio = _bus_dmamap_load_uio;
	t->_dmamap_load_raw = _bus_dmamap_load_raw;
	t->_dmamap_unload = _bus_dmamap_unload;
	t->_dmamap_sync = _bus_dmamap_sync;
	t->_dmamem_alloc = _bus_dmamem_alloc;
	t->_dmamem_free = _bus_dmamem_free;
	t->_dmamem_map = _bus_dmamem_map;
	t->_dmamem_unmap = _bus_dmamem_unmap;
	t->_dmamem_mmap = _bus_dmamem_mmap;
}

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments, bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct mipsco_bus_dmamap *map;
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
	mapsize = sizeof(struct mipsco_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	memset(mapstore, 0, mapsize);
	map = (struct mipsco_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxmaxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
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
_bus_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
{

	free(map, M_DMAMAP);
}

/*
 * Utility function to load a linear buffer.  lastaddrp holds state
 * between invocations (for multiple-buffer loads).  segp contains
 * the starting segment on entrance, and the ending segment on exit.
 * first indicates if this is the first invocation of this function.
 */
static int
_bus_dmamap_load_buffer(bus_dma_tag_t t, bus_dmamap_t map, void *buf, bus_size_t buflen, struct vmspace *vm, int flags, paddr_t *lastaddrp, int *segp, int first)
{
	bus_size_t sgsize;
	bus_addr_t baddr, bmask;
	paddr_t curaddr, lastaddr;
	vaddr_t vaddr = (vaddr_t)buf;
	int seg;

	lastaddr = *lastaddrp;
	bmask = ~(map->_dm_boundary - 1);

	for (seg = *segp; buflen > 0 ; ) {
		/*
		 * Get the physical address for this segment.
		 */
		if (!VMSPACE_IS_KERNEL_P(vm))
			(void) pmap_extract(vm_map_pmap(&vm->vm_map),
			    vaddr, &curaddr);
		else
			curaddr = kvtophys(vaddr);

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = PAGE_SIZE - ((u_long)vaddr & PGOFSET);
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
			map->dm_segs[seg].ds_addr = curaddr + t->dma_offset;
			map->dm_segs[seg].ds_len = sgsize;
			map->dm_segs[seg]._ds_vaddr = vaddr;
			map->dm_segs[seg]._ds_paddr = curaddr;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			     map->dm_maxsegsz &&
			    (map->_dm_boundary == 0 ||
			     (map->dm_segs[seg]._ds_paddr & bmask) ==
			     (curaddr & bmask)))
				map->dm_segs[seg].ds_len += sgsize;
			else {
				if (++seg >= map->_dm_segcnt)
					break;
				map->dm_segs[seg].ds_addr =
				    curaddr + t->dma_offset;
				map->dm_segs[seg].ds_len = sgsize;
				map->dm_segs[seg]._ds_vaddr = vaddr;
				map->dm_segs[seg]._ds_paddr = curaddr;
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
 * Common function for loading a direct-mapped DMA map with a linear
 * buffer.
 */
int
_bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf, bus_size_t buflen, struct proc *p, int flags)
{
	paddr_t lastaddr;
	int seg, error;
	struct vmspace *vm;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->_dm_maxmaxsegsz);

	if (buflen > map->_dm_size)
		return (EINVAL);

	if (p != NULL) {
		vm = p->p_vmspace;
	} else {
		vm = vmspace_kernel();
	}

	seg = 0;
	error = _bus_dmamap_load_buffer(t, map, buf, buflen,
	    vm, flags, &lastaddr, &seg, 1);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;

		/*
		 * For linear buffers, we support marking the mapping
		 * as COHERENT.
		 *
		 * XXX Check TLB entries for cache-inhibit bits?
		 */
		if (buf >= (void *)MIPS_KSEG1_START &&
		    buf < (void *)MIPS_KSEG2_START)
			map->_dm_flags |= MIPSCO_DMAMAP_COHERENT;
	}
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
_bus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map, struct mbuf *m0, int flags)
{
	paddr_t lastaddr;
	int seg, error, first;
	struct mbuf *m;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->_dm_maxmaxsegsz);

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("_bus_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return (EINVAL);

	first = 1;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		error = _bus_dmamap_load_buffer(t, map, m->m_data, m->m_len,
		    vmspace_kernel(), flags, &lastaddr, &seg, first);
		first = 0;
	}
	if (error == 0) {
		map->dm_mapsize = m0->m_pkthdr.len;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
_bus_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio, int flags)
{
	paddr_t lastaddr;
	int seg, i, error, first;
	bus_size_t minlen, resid;
	struct iovec *iov;
	void *addr;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->_dm_maxmaxsegsz);

	resid = uio->uio_resid;
	iov = uio->uio_iov;

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

		error = _bus_dmamap_load_buffer(t, map, addr, minlen,
		    uio->uio_vmspace, flags, &lastaddr, &seg, first);
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
 * Like _bus_dmamap_load(), but for raw memory.
 */
int
_bus_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map, bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{
	panic("_bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * chipset-specific DMA map unload functions.
 */
void
_bus_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{

	/*
	 * No resources to free; just mark the mappings as
	 * invalid.
	 */
	map->dm_maxsegsz = map->_dm_maxmaxsegsz;
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	map->_dm_flags &= ~MIPSCO_DMAMAP_COHERENT;
}

/*
 * Common function for MIPS1 DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 *
 * This is the R3000 version.
 */
void
_bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset, bus_size_t len, int ops)
{
	bus_size_t minlen;
	bus_addr_t addr;
	int i;

#ifdef DIAGNOSTIC
	/*
	 * Mixing PRE and POST operations is not allowed.
	 */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	    (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		panic("_bus_dmamap_sync: mix PRE and POST");

	if (offset >= map->dm_mapsize)
		panic("_bus_dmamap_sync: bad offset %lu (map size is %lu)",
		      offset, map->dm_mapsize);
	if (len == 0 || (offset + len) > map->dm_mapsize)
		panic("_bus_dmamap_sync: bad length");
#endif

	/*
	 * The R3000 cache is write-through.  Therefore, we only need
	 * to drain the write buffer on PREWRITE.  The cache is not
	 * coherent, however, so we need to invalidate the data cache
	 * on POSTREAD.
	 *
	 * PREREAD and POSTWRITE are noops.
	 */

	if (ops & BUS_DMASYNC_PREWRITE) {
		/*
		 * Flush the write buffer.
		 */
		wbflush();
	}

	/*
	 * If we're not doing a POSTREAD, nothing more to do.
	 */
	if ((ops & BUS_DMASYNC_POSTREAD) == 0)
		return;

	/*
	 * If the mapping is of COHERENT DMA-safe memory, no cache
	 * flush is necessary.
	 */
	if (map->_dm_flags & MIPSCO_DMAMAP_COHERENT)
		return;

	/*
	 * If we are going to hit something as large or larger
	 * than the entire data cache, just nail the whole thing.
	 *
	 * NOTE: Even though this is `wbinv_all', since the cache is
	 * write-through, it just invalidates it.
	 */
	if (len >= mips_cache_info.mci_pdcache_size) {
		mips_dcache_wbinv_all();
		return;
	}

	for (i = 0; i < map->dm_nsegs && len != 0; i++) {
		/* Find the beginning segment. */
		if (offset >= map->dm_segs[i].ds_len) {
			offset -= map->dm_segs[i].ds_len;
			continue;
		}

		/*
		 * Now at the first segment to sync; nail
		 * each segment until we have exhausted the
		 * length.
		 */
		minlen = len < map->dm_segs[i].ds_len - offset ?
		    len : map->dm_segs[i].ds_len - offset;

		addr = map->dm_segs[i].ds_addr;

#ifdef BUS_DMA_DEBUG
		printf("bus_dmamap_sync: flushing segment %d "
		    "(0x%lx..0x%lx) ...", i, addr + offset,
		    addr + offset + minlen - 1);
#endif
		mips_dcache_inv_range(
		    MIPS_PHYS_TO_KSEG0(addr + offset), minlen);
#ifdef BUS_DMA_DEBUG
		printf("\n");
#endif
		offset = 0;
		len -= minlen;
	}
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_bus_dmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment, bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags)
{

	return (_bus_dmamem_alloc_range(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags, mips_avail_start, trunc_page(mips_avail_end)));
}

/*
 * Allocate physical memory from the given physical address range.
 * Called by DMA-safe memory allocation methods.
 */
int
_bus_dmamem_alloc_range(
	bus_dma_tag_t t,
	bus_size_t size,
	bus_size_t alignment,
	bus_size_t boundary,
	bus_dma_segment_t *segs,
	int nsegs,
	int *rsegs,
	int flags,
	paddr_t low,
	paddr_t high)
{

	return (_bus_dmamem_alloc_range_common(t, size, alignment, boundary,
					       segs, nsegs, rsegs, flags,
					       low, high));
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
_bus_dmamem_free(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs)
{

	_bus_dmamem_free_common(t, segs, nsegs);
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
_bus_dmamem_map(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs, size_t size, void **kvap, int flags)
{

	/*
	 * If we're only mapping 1 segment, use KSEG0 or KSEG1, to avoid
	 * TLB thrashing.
	 */
	if (nsegs == 1) {
		if (flags & BUS_DMA_COHERENT)
			*kvap = (void *)MIPS_PHYS_TO_KSEG1(segs[0]._ds_paddr);
		else
			*kvap = (void *)MIPS_PHYS_TO_KSEG0(segs[0]._ds_paddr);
		return (0);
	}

	/* XXX BUS_DMA_COHERENT */
	return (_bus_dmamem_map_common(t, segs, nsegs, size, kvap, flags, 0));
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_bus_dmamem_unmap(bus_dma_tag_t t, void *kva, size_t size)
{

	/*
	 * Nothing to do if we mapped it with KSEG0 or KSEG1 (i.e.
	 * not in KSEG2).
	 */
	if (kva >= (void *)MIPS_KSEG0_START &&
	    kva < (void *)MIPS_KSEG2_START)
		return;

	_bus_dmamem_unmap_common(t, kva, size);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
_bus_dmamem_mmap(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs, off_t off, int prot, int flags)
{
	bus_addr_t rv;

	rv = _bus_dmamem_mmap_common(t, segs, nsegs, off, prot, flags);
	if (rv == (bus_addr_t)-1)
		return (-1);
	
	return (mips_btop((char *)rv));
}
