/* $NetBSD: bus_dma.c,v 1.28.2.3 2008/01/21 09:37:42 yamt Exp $	*/

/*
 * This file was taken from from next68k/dev/bus_dma.c, which was originally
 * taken from alpha/common/bus_dma.c.
 * It should probably be re-synced when needed.
 * original cvs id: NetBSD: bus_dma.c,v 1.13 1999/11/13 00:30:40 thorpej Exp
 */

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

__KERNEL_RCSID(0, "$NetBSD: bus_dma.c,v 1.28.2.3 2008/01/21 09:37:42 yamt Exp $");

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
#define _MVME68K_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <m68k/cacheops.h>

extern	phys_ram_seg_t mem_clusters[];

int	_bus_dmamap_load_buffer_direct_common(bus_dma_tag_t,
	    bus_dmamap_t, void *, bus_size_t, struct vmspace *, int,
	    paddr_t *, int *, int);

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct mvme68k_bus_dmamap *map;
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
	mapsize = sizeof(struct mvme68k_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return ENOMEM;

	memset(mapstore, 0, mapsize);
	map = (struct mvme68k_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxmaxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->dm_maxsegsz = maxsegsz;
	map->dm_mapsize = 0;		/* no valid mappings */
	map->dm_nsegs = 0;

	*dmamp = map;
	return 0;
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
_bus_dmamap_load_buffer_direct_common(bus_dma_tag_t t, bus_dmamap_t map,
    void *buf, bus_size_t buflen, struct vmspace *vm, int flags,
    paddr_t *lastaddrp, int *segp, int first)
{
	bus_size_t sgsize;
	bus_addr_t curaddr, lastaddr, baddr, bmask;
	vaddr_t vaddr = (vaddr_t)buf;
	int seg, cacheable, coherent = BUS_DMA_COHERENT;

	lastaddr = *lastaddrp;
	bmask = ~(map->_dm_boundary - 1);

	for (seg = *segp; buflen > 0 ; ) {
		/*
		 * Get the physical address for this segment.
		 */
		(void) pmap_extract(vm_map_pmap(&vm->vm_map), vaddr, &curaddr);
		cacheable = _pmap_page_is_cacheable(vm_map_pmap(&vm->vm_map),
		    vaddr);

		if (cacheable)
			coherent = 0;

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
			map->dm_segs[seg].ds_addr =
			    map->dm_segs[seg]._ds_cpuaddr = curaddr;
			map->dm_segs[seg].ds_len = sgsize;
			map->dm_segs[seg]._ds_flags =
			    cacheable ? 0 : BUS_DMA_COHERENT;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			     map->dm_maxsegsz &&
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
				map->dm_segs[seg]._ds_flags =
				    cacheable ? 0 : BUS_DMA_COHERENT;
			}
		}

		lastaddr = curaddr + sgsize;
		vaddr += sgsize;
		buflen -= sgsize;
	}

	*segp = seg;
	*lastaddrp = lastaddr;
	map->_dm_flags &= ~BUS_DMA_COHERENT;
	map->_dm_flags |= coherent;

	/*
	 * Did we fit?
	 */
	if (buflen != 0) {
		/*
		 * If there is a chained window, we will automatically
		 * fall back to it.
		 */
		return EFBIG;		/* XXX better return value here? */
	}

	return 0;
}

/*
 * Common function for loading a direct-mapped DMA map with a linear
 * buffer.  Called by bus-specific DMA map load functions with the
 * OR value appropriate for indicating "direct-mapped" for that
 * chipset.
 */
int
_bus_dmamap_load_direct(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
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
		return EINVAL;

	if (p != NULL) {
		vm = p->p_vmspace;
	} else {
		vm = vmspace_kernel();
	}

	seg = 0;
	error = _bus_dmamap_load_buffer_direct_common(t, map, buf, buflen,
	    vm, flags, &lastaddr, &seg, 1);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;
	}
	return error;
}

/*
 * Like _bus_dmamap_load_direct_common(), but for mbufs.
 */
int
_bus_dmamap_load_mbuf_direct(bus_dma_tag_t t, bus_dmamap_t map,
    struct mbuf *m0, int flags)
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
		panic("_bus_dmamap_load_mbuf_direct_common: no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return EINVAL;

	first = 1;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		error = _bus_dmamap_load_buffer_direct_common(t, map,
		    m->m_data, m->m_len, vmspace_kernel(), flags, &lastaddr,
		    &seg, first);
		first = 0;
	}
	if (error == 0) {
		map->dm_mapsize = m0->m_pkthdr.len;
		map->dm_nsegs = seg + 1;
	}
	return error;
}

/*
 * Like _bus_dmamap_load_direct_common(), but for uios.
 */
int
_bus_dmamap_load_uio_direct(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio,
    int flags)
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

		error = _bus_dmamap_load_buffer_direct_common(t, map,
		    addr, minlen, uio->uio_vmspace, flags, &lastaddr, &seg,
		    first);
		first = 0;

		resid -= minlen;
	}
	if (error == 0) {
		map->dm_mapsize = uio->uio_resid;
		map->dm_nsegs = seg + 1;
	}
	return error;
}

/*
 * Like _bus_dmamap_load_direct_common(), but for raw memory.
 */
int
_bus_dmamap_load_raw_direct(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{
	/*
	 * @@@ This routine doesn't enforce map boundary requirement
	 * @@@ perhaps it should return an error instead of panicking
	 */

#ifdef DIAGNOSTIC
	if (map->_dm_size < size) {
		panic("_bus_dmamap_load_raw_direct: size is too large for map");
	}
	if (map->_dm_segcnt < nsegs) {
		panic("_bus_dmamap_load_raw_direct: too many segments for map");
	}
#endif

	{
		int i;
		for (i = 0; i < nsegs; i++) {
#ifdef DIAGNOSTIC
			if (map->dm_maxsegsz < map->dm_segs[i].ds_len) {
				panic("%s: segment too large for map",
				    __func__);
			}
#endif
			map->dm_segs[i] = segs[i];
		}
	}

	map->dm_nsegs   = nsegs;
	map->dm_mapsize = size;

	return 0;
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
	map->_dm_flags &= ~BUS_DMA_COHERENT;
}

/*
 * 68030 DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 */
void
_bus_dmamap_sync_030(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{

	/* Nothing yet */
}

/*
 * 68040/68060 DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 */
void
_bus_dmamap_sync_0460(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	bus_addr_t p, e, ps, pe;
	bus_size_t seglen;
	int i;

	/* If the whole DMA map is uncached, do nothing.  */
	if (map->_dm_flags & BUS_DMA_COHERENT)
		return;

	/* Short-circuit for unsupported `ops' */
	if ((ops & (BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE)) == 0)
		return;

	for (i = 0; i < map->dm_nsegs && len > 0; i++) {
		if (map->dm_segs[i].ds_len <= offset) {
			/* Segment irrelevant - before requested offset */
			offset -= map->dm_segs[i].ds_len;
			continue;
		}

		seglen = map->dm_segs[i].ds_len - offset;
		if (seglen > len)
			seglen = len;
		len -= seglen;

		/* Ignore cache-inhibited segments */
		if (map->dm_segs[i]._ds_flags & BUS_DMA_COHERENT)
			continue;

		ps = map->dm_segs[i]._ds_cpuaddr + offset;
		pe = ps + seglen;

		if (ops & BUS_DMASYNC_PREWRITE) {
			p = ps & ~0xf;
			e = (pe + 15) & ~0xf;

			/* flush cache line (060 too) */
			while((p < e) && (p % PAGE_SIZE)) {
				DCFL_40(p);
				p += 16;
			}

			/* flush page (060 too) */
			while((p + PAGE_SIZE) <= e) {
				DCFP_40(p);
				p += PAGE_SIZE;
			}

			/* flush cache line (060 too) */
			while(p < e) {
				DCFL_40(p);
				p += 16;
			}
		}

		/*
		 * Normally, the `PREREAD' flag instructs us to purge the
		 * cache for the specified offset and length. However, if
		 * the offset/length is not aligned to a cacheline boundary,
		 * we may end up purging some legitimate data from the
		 * start/end of the cache. In such a case, *flush* the
		 * cachelines at the start and end of the required region.
		 */
		if (ops & BUS_DMASYNC_PREREAD) {
			if (ps & 0xf) {
				DCFL_40(ps & ~0xf);
				ICPL_40(ps & ~0xf);
			}
			if (pe & 0xf) {
				DCFL_40(pe & ~0xf);
				ICPL_40(pe & ~0xf);
			}

			p = (ps + 15) & ~0xf;
			e = pe & ~0xf;

			/* purge cache line */
			while((p < e) && (p % PAGE_SIZE)) {
				DCPL_40(p);
				ICPL_40(p);
				p += 16;
			}

			/* purge page */
			while((p + PAGE_SIZE) <= e) {
				DCPP_40(p);
				ICPP_40(p);
				p += PAGE_SIZE;
			}

			/* purge cache line */
			while(p < e) {
				DCPL_40(p);
				ICPL_40(p);
				p += 16;
			}
		}
	}
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_bus_dmamem_alloc_common(bus_dma_tag_t t, bus_addr_t low, bus_addr_t high,
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
	 * XXXSCW: This will be sub-optimal if the base-address of offboard
	 * RAM is significantly higher than the end-address of onboard RAM.
	 * (Due to how uvm_pglistalloc() is implemented.)
	 *
	 * uvm_pglistalloc() also currently ignores the 'nsegs' parameter,
	 * and always returns only one (contiguous) segment.
	 */
	error = uvm_pglistalloc(size, low, high, alignment, boundary,
	    &mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
	if (error)
		return error;

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
			printf("%s: too many segments\n", __func__);
#ifdef DEBUG
			panic("%s", __func__);
#endif
#endif
			uvm_pglistfree(&mlist);
			return -1;
		}

		curaddr = VM_PAGE_TO_PHYS(m);
#ifdef DIAGNOSTIC
		if (curaddr < low || curaddr > high) {
			printf("uvm_pglistalloc returned non-sensical"
			    " address 0x%lx\n", curaddr);
			panic("%s", __func__);
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

	return 0;
}
/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_bus_dmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags) 
{
	extern paddr_t avail_start, avail_end;
	bus_addr_t high;

	/*
	 * Assume any memory will do (this includes off-board RAM)
	 */
	high = avail_end;

	if ((flags & BUS_DMA_ONBOARD_RAM) != 0) {
		/*
		 * Constrain the memory to 'onboard' RAM only
		 */
		high = mem_clusters[0].size;
	}

	if ((flags & BUS_DMA_24BIT) != 0 && (high & 0xff000000u) != 0) {
		/*
		 * We need to constrain the memory to a 24-bit address
		 */
		high = 0x01000000u;
	}

	return _bus_dmamem_alloc_common(t, avail_start, high,
	    size, alignment, boundary, segs, nsegs, rsegs, flags);
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
int
_bus_dmamem_map(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
    size_t size, void **kvap, int flags)
{
	vaddr_t va;
	bus_addr_t addr;
	int curseg;
	const uvm_flag_t kmflags =
	    (flags & BUS_DMA_NOWAIT) != 0 ? UVM_KMF_NOWAIT : 0;

	size = round_page(size);

	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY | kmflags);

	if (va == 0)
		return ENOMEM;

	*kvap = (void *)va;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg]._ds_cpuaddr;
		    addr < (segs[curseg]._ds_cpuaddr + segs[curseg].ds_len);
		    addr += PAGE_SIZE, va += PAGE_SIZE, size -= PAGE_SIZE) {
			if (size == 0)
				panic("%s: size botch", __func__);

			pmap_enter(pmap_kernel(), va, addr,
			    VM_PROT_READ | VM_PROT_WRITE,
			    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);

			/* Cache-inhibit the page if necessary */
			if ((flags & BUS_DMA_COHERENT) != 0)
				_pmap_set_page_cacheinhibit(pmap_kernel(), va);

			segs[curseg]._ds_flags &= ~BUS_DMA_COHERENT;
			segs[curseg]._ds_flags |= (flags & BUS_DMA_COHERENT);
		}
	}
	pmap_update(pmap_kernel());

	if ((flags & BUS_DMA_COHERENT) != 0)
		TBIAS();

	return 0;
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_bus_dmamem_unmap(bus_dma_tag_t t, void *kva, size_t size)
{
	vaddr_t va;
	size_t s;

#ifdef DIAGNOSTIC
	if ((u_long)kva & PGOFSET)
		panic("%s", __func__);
#endif

	size = round_page(size);

	/*
	 * Re-enable cacheing on the range
	 * XXXSCW: There should be some way to indicate that the pages
	 * were mapped DMA_MAP_COHERENT in the first place...
	 */
	for (s = 0, va = (vaddr_t)kva; s < size;
	     s += PAGE_SIZE, va += PAGE_SIZE)
		_pmap_set_page_cacheable(pmap_kernel(), va);

	pmap_remove(pmap_kernel(), (vaddr_t)kva, (vaddr_t)kva + size);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, (vaddr_t)kva, size, UVM_KMF_VAONLY);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
_bus_dmamem_mmap(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
    off_t off, int prot, int flags)
{
	int i;

	for (i = 0; i < nsegs; i++) {
#ifdef DIAGNOSTIC
		if (off & PGOFSET)
			panic("%s: offset unaligned", __func__);
		if (segs[i]._ds_cpuaddr & PGOFSET)
			panic("%s: segment unaligned", __func__);
		if (segs[i].ds_len & PGOFSET)
			panic("%s: segment size not multiple of page size",
			    __func__);
#endif
		if (off >= segs[i].ds_len) {
			off -= segs[i].ds_len;
			continue;
		}

		/*
		 * XXXSCW: What about BUS_DMA_COHERENT ??
		 */

		return m68k_btop((char *)segs[i]._ds_cpuaddr + off);
	}

	/* Page not found. */
	return -1;
}
