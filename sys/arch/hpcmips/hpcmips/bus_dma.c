/*	$NetBSD: bus_dma.c,v 1.30.44.3 2009/09/16 13:37:38 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: bus_dma.c,v 1.30.44.3 2009/09/16 13:37:38 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>
#include <mips/cache.h>

#include <machine/bus.h>
#include <machine/bus_dma_hpcmips.h>

#include <common/bus_dma/bus_dmamem_common.h>

static int _hpcmips_bd_map_load_buffer(bus_dmamap_t, void *, bus_size_t,
    struct vmspace *, int, vaddr_t *, int *, int);

paddr_t	kvtophys(vaddr_t);	/* XXX */

/*
 * The default DMA tag for all busses on the hpcmips
 */
struct bus_dma_tag_hpcmips hpcmips_default_bus_dma_tag = {
	{
		NULL,
		{
			_hpcmips_bd_map_create,
			_hpcmips_bd_map_destroy,
			_hpcmips_bd_map_load,
			_hpcmips_bd_map_load_mbuf,
			_hpcmips_bd_map_load_uio,
			_hpcmips_bd_map_load_raw,
			_hpcmips_bd_map_unload,
			_hpcmips_bd_map_sync,
			_hpcmips_bd_mem_alloc,
			_hpcmips_bd_mem_free,
			_hpcmips_bd_mem_map,
			_hpcmips_bd_mem_unmap,
			_hpcmips_bd_mem_mmap,
		},
	},
	NULL,
};

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_hpcmips_bd_map_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct bus_dmamap_hpcmips *map;
	void *mapstore;
	size_t mapsize;

	/*
	 * Allocate and initialize the DMA map.  The end of the map
	 * has two variable-sized array of segments, so we allocate enough
	 * room for them in one shot.
	 *
	 * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
	 * of ALLOCNOW notifies others that we've reserved these resources,
	 * and they are not to be freed.
	 */
	mapsize = sizeof(struct bus_dmamap_hpcmips) +
	    sizeof(struct bus_dma_segment_hpcmips) * (nsegments - 1) +
	    sizeof(bus_dma_segment_t) * nsegments;
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	memset(mapstore, 0, mapsize);
	map = (struct bus_dmamap_hpcmips *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxmaxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->bdm.dm_maxsegsz = maxsegsz;
	map->bdm.dm_mapsize = 0;	/* no valid mappings */
	map->bdm.dm_nsegs = 0;
	map->bdm.dm_segs = (bus_dma_segment_t *)((char *)mapstore +
	    sizeof(struct bus_dmamap_hpcmips) +
	    sizeof(struct bus_dma_segment_hpcmips) * (nsegments - 1));

	*dmamp = &map->bdm;
	return (0);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
_hpcmips_bd_map_destroy(bus_dma_tag_t t, bus_dmamap_t map)
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
_hpcmips_bd_map_load_buffer(bus_dmamap_t mapx, void *buf, bus_size_t buflen,
    struct vmspace *vm, int flags, vaddr_t *lastaddrp, int *segp, int first)
{
	struct bus_dmamap_hpcmips *map = (struct bus_dmamap_hpcmips *)mapx;
	bus_size_t sgsize;
	bus_addr_t curaddr, lastaddr, baddr, bmask;
	vaddr_t vaddr = (vaddr_t)buf;
	int seg;

	lastaddr = *lastaddrp;
	bmask  = ~(map->_dm_boundary - 1);

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
			map->bdm.dm_segs[seg].ds_addr = curaddr;
			map->bdm.dm_segs[seg].ds_len = sgsize;
			map->_dm_segs[seg]._ds_vaddr = vaddr;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    (map->bdm.dm_segs[seg].ds_len + sgsize) <=
			    map->bdm.dm_maxsegsz &&
			    (map->_dm_boundary == 0 ||
				(map->bdm.dm_segs[seg].ds_addr & bmask) ==
				(curaddr & bmask)))
				map->bdm.dm_segs[seg].ds_len += sgsize;
			else {
				if (++seg >= map->_dm_segcnt)
					break;
				map->bdm.dm_segs[seg].ds_addr = curaddr;
				map->bdm.dm_segs[seg].ds_len = sgsize;
				map->_dm_segs[seg]._ds_vaddr = vaddr;
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
_hpcmips_bd_map_load(bus_dma_tag_t t, bus_dmamap_t mapx, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	struct bus_dmamap_hpcmips *map = (struct bus_dmamap_hpcmips *)mapx;
	vaddr_t lastaddr;
	int seg, error;
	struct vmspace *vm;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->bdm.dm_mapsize = 0;
	map->bdm.dm_nsegs = 0;
	KASSERT(map->bdm.dm_maxsegsz <= map->_dm_maxmaxsegsz);

	if (buflen > map->_dm_size)
		return (EINVAL);

	if (p != NULL) {
		vm = p->p_vmspace;
	} else {
		vm = vmspace_kernel();
	}

	seg = 0;
	error = _hpcmips_bd_map_load_buffer(mapx, buf, buflen,
	    vm, flags, &lastaddr, &seg, 1);
	if (error == 0) {
		map->bdm.dm_mapsize = buflen;
		map->bdm.dm_nsegs = seg + 1;

		/*
		 * For linear buffers, we support marking the mapping
		 * as COHERENT.
		 *
		 * XXX Check TLB entries for cache-inhibit bits?
		 */
		if (buf >= (void *)MIPS_KSEG1_START &&
		    buf < (void *)MIPS_KSEG2_START)
			map->_dm_flags |= HPCMIPS_DMAMAP_COHERENT;
	}
	return (error);
}

/*
 * Like _hpcmips_bd_map_load(), but for mbufs.
 */
int
_hpcmips_bd_map_load_mbuf(bus_dma_tag_t t, bus_dmamap_t mapx, struct mbuf *m0,
    int flags)
{
	struct bus_dmamap_hpcmips *map = (struct bus_dmamap_hpcmips *)mapx;
	vaddr_t lastaddr;
	int seg, error, first;
	struct mbuf *m;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->bdm.dm_mapsize = 0;
	map->bdm.dm_nsegs = 0;
	KASSERT(map->bdm.dm_maxsegsz <= map->_dm_maxmaxsegsz);

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("_hpcmips_bd_map_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return (EINVAL);

	first = 1;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		error = _hpcmips_bd_map_load_buffer(mapx, m->m_data, m->m_len,
		    vmspace_kernel(), flags, &lastaddr, &seg, first);
		first = 0;
	}
	if (error == 0) {
		map->bdm.dm_mapsize = m0->m_pkthdr.len;
		map->bdm.dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _hpcmips_bd_map_load(), but for uios.
 */
int
_hpcmips_bd_map_load_uio(bus_dma_tag_t t, bus_dmamap_t mapx, struct uio *uio,
    int flags)
{
	struct bus_dmamap_hpcmips *map = (struct bus_dmamap_hpcmips *)mapx;
	vaddr_t lastaddr;
	int seg, i, error, first;
	bus_size_t minlen, resid;
	struct iovec *iov;
	void *addr;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->bdm.dm_mapsize = 0;
	map->bdm.dm_nsegs = 0;
	KASSERT(map->bdm.dm_maxsegsz <= map->_dm_maxmaxsegsz);

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

		error = _hpcmips_bd_map_load_buffer(mapx, addr, minlen,
		    uio->uio_vmspace, flags, &lastaddr, &seg, first);
		first = 0;

		resid -= minlen;
	}
	if (error == 0) {
		map->bdm.dm_mapsize = uio->uio_resid;
		map->bdm.dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _hpcmips_bd_map_load(), but for raw memory.
 */
int
_hpcmips_bd_map_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{

	panic("_hpcmips_bd_map_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * chipset-specific DMA map unload functions.
 */
void
_hpcmips_bd_map_unload(bus_dma_tag_t t, bus_dmamap_t mapx)
{
	struct bus_dmamap_hpcmips *map = (struct bus_dmamap_hpcmips *)mapx;

	/*
	 * No resources to free; just mark the mappings as
	 * invalid.
	 */
	map->bdm.dm_maxsegsz = map->_dm_maxmaxsegsz;
	map->bdm.dm_mapsize = 0;
	map->bdm.dm_nsegs = 0;
	map->_dm_flags &= ~HPCMIPS_DMAMAP_COHERENT;
}

/*
 * Common function for DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 */
void
_hpcmips_bd_map_sync(bus_dma_tag_t t, bus_dmamap_t mapx, bus_addr_t offset,
    bus_size_t len, int ops)
{
	struct bus_dmamap_hpcmips *map = (struct bus_dmamap_hpcmips *)mapx;
	bus_size_t minlen;
	bus_addr_t addr;
	int i;

	/*
	 * Mixing PRE and POST operations is not allowed.
	 */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	    (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		panic("_hpcmips_bd_map_sync: mix PRE and POST");

#ifdef DIAGNOSTIC
	if (offset >= map->bdm.dm_mapsize)
		panic("_hpcmips_bd_map_sync: bad offset %lu (map size is %lu)",
		    offset, map->bdm.dm_mapsize);
	if (len == 0 || (offset + len) > map->bdm.dm_mapsize)
		panic("_hpcmips_bd_map_sync: bad length");
#endif

	/*
	 * Flush the write buffer.
	 */
	wbflush();

	/*
	 * If the mapping is of COHERENT DMA-safe memory, no cache
	 * flush is necessary.
	 */
	if (map->_dm_flags & HPCMIPS_DMAMAP_COHERENT)
		return;

	/*
	 * No cache flushes are necessary if we're only doing
	 * POSTREAD or POSTWRITE (i.e. not doing PREREAD or PREWRITE).
	 */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) == 0)
		return;

	/*
	 * Flush data cache for PREREAD.  This has the side-effect
	 * of invalidating the cache.  Done at PREREAD since it
	 * causes the cache line(s) to be written back to memory.
	 *
	 * Flush data cache for PREWRITE, so that the contents of
	 * the data buffer in memory reflect reality.
	 *
	 * Given the test above, we know we're doing one of these
	 * two operations, so no additional tests are necessary.
	 */

	/*
	 * The R2000 and R3000 have a physically indexed
	 * cache.  Loop through the DMA segments, looking
	 * for the appropriate offset, and flush the D-cache
	 * at that physical address.
	 *
	 * The R4000 has a virtually indexed primary data cache.  We
	 * do the same loop, instead using the virtual address stashed
	 * away in the segments when the map was loaded.
	 */
	for (i = 0; i < map->bdm.dm_nsegs && len != 0; i++) {
		/* Find the beginning segment. */
		if (offset >= map->bdm.dm_segs[i].ds_len) {
			offset -= map->bdm.dm_segs[i].ds_len;
			continue;
		}

		/*
		 * Now at the first segment to sync; nail
		 * each segment until we have exhausted the
		 * length.
		 */
		minlen = len < map->bdm.dm_segs[i].ds_len - offset ?
		    len : map->bdm.dm_segs[i].ds_len - offset;

		if (CPUISMIPS3)
			addr = map->_dm_segs[i]._ds_vaddr;
		else
			addr = map->bdm.dm_segs[i].ds_addr;

#ifdef BUS_DMA_DEBUG
		printf("_hpcmips_bd_map_sync: flushing segment %d "
		    "(0x%lx..0x%lx) ...", i, addr + offset,
		    addr + offset + minlen - 1);
#endif
		if (CPUISMIPS3)
			mips_dcache_wbinv_range(addr + offset, minlen);
		else {
			/*
			 * We can't have a TLB miss; use KSEG0.
			 */
			mips_dcache_wbinv_range(
				MIPS_PHYS_TO_KSEG0(map->bdm.dm_segs[i].ds_addr
				    + offset),
				minlen);
		}
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
_hpcmips_bd_mem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags)
{
	extern paddr_t avail_start, avail_end;		/* XXX */
	psize_t high;

	high = avail_end - PAGE_SIZE;

	return (_hpcmips_bd_mem_alloc_range(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags, avail_start, high));
}

/*
 * Allocate physical memory from the given physical address range.
 * Called by DMA-safe memory allocation methods.
 */
int
_hpcmips_bd_mem_alloc_range(bus_dma_tag_t t, bus_size_t size,
    bus_size_t alignment, bus_size_t boundary,
    bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags, paddr_t low, paddr_t high)
{
#ifdef DIAGNOSTIC
	extern paddr_t avail_start, avail_end;		/* XXX */

	high = high<(avail_end - PAGE_SIZE)? high: (avail_end - PAGE_SIZE);
	low = low>avail_start? low: avail_start;
#endif

	return (_bus_dmamem_alloc_range_common(t, size, alignment, boundary,
					       segs, nsegs, rsegs, flags,
					       low, high));
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
_hpcmips_bd_mem_free(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs)
{

	_bus_dmamem_free_common(t, segs, nsegs);
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
_hpcmips_bd_mem_map(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
    size_t size, void **kvap, int flags)
{

	/*
	 * If we're only mapping 1 segment, use KSEG0 or KSEG1, to avoid
	 * TLB thrashing.
	 */
	if (nsegs == 1) {
		if (flags & BUS_DMA_COHERENT)
			*kvap = (void *)MIPS_PHYS_TO_KSEG1(segs[0].ds_addr);
		else
			*kvap = (void *)MIPS_PHYS_TO_KSEG0(segs[0].ds_addr);
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
_hpcmips_bd_mem_unmap(bus_dma_tag_t t, void *kva, size_t size)
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
_hpcmips_bd_mem_mmap(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
    off_t off, int prot, int flags)
{
	bus_addr_t rv;

	rv = _bus_dmamem_mmap_common(t, segs, nsegs, off, prot, flags);
	if (rv == (bus_addr_t)-1)
		return (-1);
	
	return (mips_btop((char *)rv));
}
