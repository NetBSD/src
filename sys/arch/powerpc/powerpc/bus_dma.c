/*	$NetBSD: bus_dma.c,v 1.35 2009/11/07 07:27:46 cegger Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: bus_dma.c,v 1.35 2009/11/07 07:27:46 cegger Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#include <uvm/uvm_extern.h>

#define _POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/intr.h>

int	_bus_dmamap_load_buffer (bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct vmspace *, int, paddr_t *, int *, int);

static inline void
dcbst(paddr_t pa, long len, int dcache_line_size)
{
	paddr_t epa;
	for (epa = pa + len; pa < epa; pa += dcache_line_size)
		__asm volatile("dcbst 0,%0" :: "r"(pa));
}

static inline void
dcbi(paddr_t pa, long len, int dcache_line_size)
{
	paddr_t epa;
	for (epa = pa + len; pa < epa; pa += dcache_line_size)
		__asm volatile("dcbi 0,%0" :: "r"(pa));
}

static inline void
dcbf(paddr_t pa, long len, int dcache_line_size)
{
	paddr_t epa;
	for (epa = pa + len; pa < epa; pa += dcache_line_size)
		__asm volatile("dcbf 0,%0" :: "r"(pa));
}

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments, bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct powerpc_bus_dmamap *map;
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
	mapsize = sizeof(struct powerpc_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	memset(mapstore, 0, mapsize);
	map = (struct powerpc_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxmaxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_bounce_thresh = t->_bounce_thresh;
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
int
_bus_dmamap_load_buffer(bus_dma_tag_t t, bus_dmamap_t map, void *buf, bus_size_t buflen, struct vmspace *vm, int flags, paddr_t *lastaddrp, int *segp, int first)
{
	bus_size_t sgsize;
	bus_addr_t curaddr, lastaddr, baddr, bmask;
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
			    vaddr, (void *)&curaddr);
		else
			curaddr = vtophys(vaddr);

		/*
		 * If we're beyond the bounce threshold, notify
		 * the caller.
		 */
		if (map->_dm_bounce_thresh != 0 &&
		    curaddr >= map->_dm_bounce_thresh)
			return (EINVAL);

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = PAGE_SIZE - ((u_long)vaddr & PGOFSET);
		if (buflen < sgsize)
			sgsize = buflen;
		sgsize = min(sgsize, map->dm_maxsegsz);

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
			map->dm_segs[seg].ds_addr = PHYS_TO_BUS_MEM(t, curaddr);
			map->dm_segs[seg].ds_len = sgsize;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			     map->dm_maxsegsz &&
			    (map->_dm_boundary == 0 ||
			     (map->dm_segs[seg].ds_addr & bmask) ==
			     (PHYS_TO_BUS_MEM(t, curaddr) & bmask)))
				map->dm_segs[seg].ds_len += sgsize;
			else {
				if (++seg >= map->_dm_segcnt)
					break;
				map->dm_segs[seg].ds_addr =
					PHYS_TO_BUS_MEM(t, curaddr);
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
 * Common function for loading a DMA map with a linear buffer.  May
 * be called by bus-specific DMA map load functions.
 */
int
_bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf, bus_size_t buflen, struct proc *p, int flags)
{
	paddr_t lastaddr = 0;
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
	error = _bus_dmamap_load_buffer(t, map, buf, buflen, vm, flags,
		&lastaddr, &seg, 1);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
_bus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map, struct mbuf *m0, int flags)
{
	paddr_t lastaddr = 0;
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
	for (m = m0; m != NULL && error == 0; m = m->m_next, first = 0) {
		if (m->m_len == 0)
			continue;
#ifdef POOL_VTOPHYS
		/* XXX Could be better about coalescing. */
		/* XXX Doesn't check boundaries. */
		switch (m->m_flags & (M_EXT|M_CLUSTER)) {
		case M_EXT|M_CLUSTER:
			/* XXX KDASSERT */
			KASSERT(m->m_ext.ext_paddr != M_PADDR_INVALID);
			lastaddr = m->m_ext.ext_paddr +
			    (m->m_data - m->m_ext.ext_buf);
 have_addr:
			if (first == 0 && ++seg >= map->_dm_segcnt) {
				error = EFBIG;
				continue;
			}
			map->dm_segs[seg].ds_addr =
			    PHYS_TO_BUS_MEM(t, lastaddr);
			map->dm_segs[seg].ds_len = m->m_len;
			lastaddr += m->m_len;
			continue;

		case 0:
			lastaddr = m->m_paddr + M_BUFOFFSET(m) +
			    (m->m_data - M_BUFADDR(m));
			goto have_addr;

		default:
			break;
		}
#endif
		error = _bus_dmamap_load_buffer(t, map, m->m_data,
		    m->m_len, vmspace_kernel(), flags, &lastaddr, &seg, first);
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
	paddr_t lastaddr = 0;
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
 * Like _bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
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
}

/*
 * Common function for DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 */
void
_bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset, bus_size_t len, int ops)
{
	const int dcache_line_size = curcpu()->ci_ci.dcache_line_size;
	const bus_dma_segment_t *ds = map->dm_segs;

	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	    (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		panic("_bus_dmamap_sync: invalid ops %#x", ops);

#ifdef DIAGNOSTIC
	if (offset + len > map->dm_mapsize)
		panic("_bus_dmamap_sync: bad offset and/or length");
#endif

	/*
	 * Skip leading amount
	 */
	while (offset >= ds->ds_len) {
		offset -= ds->ds_len;
		ds++;
	}
	__asm volatile("eieio");
	for (; len > 0; ds++, offset = 0) {
		bus_size_t seglen = ds->ds_len - offset;
		bus_addr_t addr = BUS_MEM_TO_PHYS(t, ds->ds_addr) + offset;
		if (seglen > len)
			seglen = len;
		len -= seglen;
		KASSERT(ds < &map->dm_segs[map->dm_nsegs]);
		/*
		 * Readjust things to start on cacheline boundarys
		 */
		offset = (addr & (dcache_line_size-1));
		seglen += offset;
		addr -= offset;
		/*
		 * Now do the appropriate thing.
		 */
		switch (ops) {
		case BUS_DMASYNC_PREWRITE:
			/*
			 * Make sure cache contents are in memory for the DMA.
			 */
			dcbst(addr, seglen, dcache_line_size);
			break;
		case BUS_DMASYNC_PREREAD:
			/*
			 * If the region to be invalidated doesn't fall on
			 * cacheline boundary, flush that cacheline so we
			 * preserve the leading content.
			 */
			if (offset) {
				dcbf(addr, 1, 1);
				/*
				 * If we are doing <= one cache line, stop now.
				 */
				if (seglen <= dcache_line_size)
					break;
				/*
				 * Advance one cache line since we've flushed
				 * this one.
				 */
				addr += dcache_line_size;
				seglen -= dcache_line_size;
			}
			/*
			 * If the byte after the region to be invalidated
			 * doesn't fall on cacheline boundary, flush that
			 * cacheline so we preserve the trailing content.
			 */
			if (seglen & (dcache_line_size-1)) {
				dcbf(addr + seglen, 1, 1);
				if (seglen <= dcache_line_size)
					break;
				/*
				 * Truncate the length to a multiple of a
				 * dcache line size.  No reason to flush
				 * the last entry again.
				 */
				seglen &= ~(dcache_line_size - 1);
			}
			__asm volatile("sync; eieio"); /* is this needed? */
			/* FALLTHROUGH */
		case BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE:
		case BUS_DMASYNC_POSTREAD:
			/*
			 * The contents will have changed, make sure to remove
			 * them from the cache.  Note: some implementation
			 * implement dcbi identically to dcbf.  Thus if the
			 * cacheline has data, it will be written to memory.
			 * If the DMA is updating the same cacheline at the
			 * time, bad things can happen.
			 */
			dcbi(addr, seglen, dcache_line_size);
			break;
		case BUS_DMASYNC_POSTWRITE:
			/*
			 * Do nothing.
			 */
			break;
		case BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE:
			/*
			 * Force it to memory and remove from cache.
			 */
			dcbf(addr, seglen, dcache_line_size);
			break;
		}
	}
	__asm volatile("sync");
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_bus_dmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment, bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags)
{
	paddr_t avail_start = 0xffffffff, avail_end = 0;
	int bank;

	for (bank = 0; bank < vm_nphysseg; bank++) {
		if (avail_start > vm_physmem[bank].avail_start << PGSHIFT)
			avail_start = vm_physmem[bank].avail_start << PGSHIFT;
		if (avail_end < vm_physmem[bank].avail_end << PGSHIFT)
			avail_end = vm_physmem[bank].avail_end << PGSHIFT;
	}

	return _bus_dmamem_alloc_range(t, size, alignment, boundary, segs,
	    nsegs, rsegs, flags, avail_start, avail_end - PAGE_SIZE);
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
_bus_dmamem_free(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs)
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
		for (addr = BUS_MEM_TO_PHYS(t, segs[curseg].ds_addr);
		    addr < (BUS_MEM_TO_PHYS(t, segs[curseg].ds_addr)
			+ segs[curseg].ds_len);
		    addr += PAGE_SIZE) {
			m = PHYS_TO_VM_PAGE(addr);
			TAILQ_INSERT_TAIL(&mlist, m, pageq.queue);
		}
	}

	uvm_pglistfree(&mlist);
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
_bus_dmamem_map(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs, size_t size, void **kvap, int flags)
{
	vaddr_t va;
	bus_addr_t addr;
	int curseg;
	const uvm_flag_t kmflags =
	    (flags & BUS_DMA_NOWAIT) != 0 ? UVM_KMF_NOWAIT : 0;

	size = round_page(size);

	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY | kmflags);

	if (va == 0)
		return (ENOMEM);

	*kvap = (void *)va;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = BUS_MEM_TO_PHYS(t, segs[curseg].ds_addr);
		    addr < (BUS_MEM_TO_PHYS(t, segs[curseg].ds_addr)
			+ segs[curseg].ds_len);
		    addr += PAGE_SIZE, va += PAGE_SIZE, size -= PAGE_SIZE) {
			if (size == 0)
				panic("_bus_dmamem_map: size botch");
			/*
			 * If we are mapping nocache, flush the page from
			 * cache before we map it.
			 */
			if (flags & BUS_DMA_NOCACHE)
				dcbf(addr, PAGE_SIZE,
				    curcpu()->ci_ci.dcache_line_size);
			pmap_kenter_pa(va, addr,
			    VM_PROT_READ | VM_PROT_WRITE |
			    PMAP_WIRED |
			    ((flags & BUS_DMA_NOCACHE) ? PMAP_NC : 0), 0);
		}
	}

	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_bus_dmamem_unmap(bus_dma_tag_t t, void *kva, size_t size)
{

#ifdef DIAGNOSTIC
	if ((u_long)kva & PGOFSET)
		panic("_bus_dmamem_unmap");
#endif

	size = round_page(size);

	pmap_kremove((vaddr_t)kva, size);
	uvm_km_free(kernel_map, (vaddr_t)kva, size, UVM_KMF_VAONLY);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
_bus_dmamem_mmap(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs, off_t off, int prot, int flags)
{
	int i;

	for (i = 0; i < nsegs; i++) {
#ifdef DIAGNOSTIC
		if (off & PGOFSET)
			panic("_bus_dmamem_mmap: offset unaligned");
		if (BUS_MEM_TO_PHYS(t, segs[i].ds_addr) & PGOFSET)
			panic("_bus_dmamem_mmap: segment unaligned");
		if (segs[i].ds_len & PGOFSET)
			panic("_bus_dmamem_mmap: segment size not multiple"
			    " of page size");
#endif
		if (off >= segs[i].ds_len) {
			off -= segs[i].ds_len;
			continue;
		}

		return (BUS_MEM_TO_PHYS(t, segs[i].ds_addr) + off);
	}

	/* Page not found. */
	return (-1);
}

/*
 * Allocate physical memory from the given physical address range.
 * Called by DMA-safe memory allocation methods.
 */
int
_bus_dmamem_alloc_range(t, size, alignment, boundary, segs, nsegs, rsegs,
    flags, low, high)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
	paddr_t low;
	paddr_t high;
{
	paddr_t curaddr, lastaddr;
	struct vm_page *m;
	struct pglist mlist;
	int curseg, error;

	/* Always round the size. */
	size = round_page(size);

	/*
	 * Allocate pages from the VM system.
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
	segs[curseg].ds_addr = PHYS_TO_BUS_MEM(t, lastaddr);
	segs[curseg].ds_len = PAGE_SIZE;
	m = m->pageq.queue.tqe_next;

	for (; m != NULL; m = m->pageq.queue.tqe_next) {
		curaddr = VM_PAGE_TO_PHYS(m);
#ifdef DIAGNOSTIC
		if (curaddr < low || curaddr >= high) {
			printf("vm_page_alloc_memory returned non-sensical"
			    " address 0x%lx\n", curaddr);
			panic("_bus_dmamem_alloc_range");
		}
#endif
		if (curaddr == (lastaddr + PAGE_SIZE))
			segs[curseg].ds_len += PAGE_SIZE;
		else {
			curseg++;
			segs[curseg].ds_addr = PHYS_TO_BUS_MEM(t, curaddr);
			segs[curseg].ds_len = PAGE_SIZE;
		}
		lastaddr = curaddr;
	}

	*rsegs = curseg + 1;

	return (0);
}

/*
 * Generic form of PHYS_TO_BUS_MEM().
 */
bus_addr_t
_bus_dma_phys_to_bus_mem_generic(bus_dma_tag_t t, bus_addr_t addr)
{

	return (addr);
}

/*
 * Generic form of BUS_MEM_TO_PHYS().
 */
bus_addr_t
_bus_dma_bus_mem_to_phys_generic(bus_dma_tag_t t, bus_addr_t addr)
{

	return (addr);
}
