/* $NetBSD: bus_dma.c,v 1.8 1999/05/25 23:14:06 thorpej Exp $ */

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

#if 0
__KERNEL_RCSID(0, "$NetBSD: bus_dma.c,v 1.8 1999/05/25 23:14:06 thorpej Exp $");
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <uvm/uvm_extern.h>

#define _GENERIC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/intr.h>


#if !defined(DEBUG) || 1
#define DPRINTF(x)
#else

#define DPRINTF(x) printf x;

#define XCHR(x) "0123456789abcdef"[(x) & 0xf]
void
hex_dump(unsigned char *pkt, size_t len)
{
	size_t i, j;

	printf("0000: ");
	for(i=0; i<len; i++) {
		printf("%c%c ", XCHR(pkt[i]>>4), XCHR(pkt[i]));
		if ((i+1) % 16 == 0) {
			printf("  %c", '"');
			for(j=0; j<16; j++)
				printf("%c", pkt[i-15+j]>=32 && pkt[i-15+j]<127?pkt[i-15+j]:'.');
			printf("%c\n%c%c%c%c: ", '"', XCHR((i+1)>>12),
				XCHR((i+1)>>8), XCHR((i+1)>>4), XCHR(i+1));
		}
	}
	printf("\n");
}
#endif


int	_bus_dmamap_load_buffer_direct_common __P((bus_dmamap_t,
	    void *, bus_size_t, struct proc *, int, bus_addr_t,
	    vm_offset_t *, int *, int));

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_bus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	struct generic_bus_dmamap *map;
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
	mapsize = sizeof(struct generic_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DEVBUF,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	bzero(mapstore, mapsize);
	map = (struct generic_bus_dmamap *)mapstore;
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
void
_bus_dmamap_destroy(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{

	free(map, M_DEVBUF);
}

/*
 * Utility function to load a linear buffer.  lastaddrp holds state
 * between invocations (for multiple-buffer loads).  segp contains
 * the starting segment on entrance, and the ending segment on exit.
 * first indicates if this is the first invocation of this function.
 */
int
_bus_dmamap_load_buffer_direct_common(map, buf, buflen, p, flags, wbase,
    lastaddrp, segp, first)
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
	bus_addr_t wbase;
	vm_offset_t *lastaddrp;
	int *segp;
	int first;
{
	bus_size_t sgsize;
	vm_offset_t curaddr, lastaddr;
	vm_offset_t vaddr = (vm_offset_t)buf;
	int seg;

	lastaddr = *lastaddrp;

	for (seg = *segp; buflen > 0 && seg < map->_dm_segcnt; ) {

		/*
		 * Get the physical address for this segment.
		 */
		if (p != NULL)
			curaddr = pmap_extract(p->p_vmspace->vm_map.pmap,
			    vaddr);
		else
			curaddr = pmap_extract(pmap_kernel(),vaddr);

		curaddr |= wbase;

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = NBPG - ((u_long)vaddr & PGOFSET);
		if (buflen < sgsize)
			sgsize = buflen;

#if 0 && defined(DEBUG)
		DPRINTF(("Dumping PA 0x%08x-0x%08x, length %d\n",
				curaddr,curaddr+sgsize,sgsize));
		hex_dump(vaddr, sgsize);
#endif

		/*
		 * Insert chunk into a segment, coalescing with
		 * the previous segment if possible.
		 */
		if (first) {
			map->dm_segs[seg].ds_addr = curaddr;
			map->dm_segs[seg].ds_len = sgsize;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			    map->_dm_maxsegsz)
				map->dm_segs[seg].ds_len += sgsize;
			else {
				seg++;
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
	if (buflen != 0) {
		/*
		 * XXX Should fall back on SGMAPs.
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
int
_bus_dmamap_load_direct(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	vm_offset_t lastaddr;
	int seg, error;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
		return (EINVAL);

	seg = 0;
	error = _bus_dmamap_load_buffer_direct_common(map, buf, buflen,
	    p, flags, 0, &lastaddr, &seg, 1);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _bus_dmamap_load_direct_common(), but for mbufs.
 */
int
_bus_dmamap_load_mbuf_direct(t, map, m0, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m0;
	int flags;
{
	vm_offset_t lastaddr;
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
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		error = _bus_dmamap_load_buffer_direct_common(map,
		    m->m_data, m->m_len, NULL, flags, 0, &lastaddr,
		    &seg, first);
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
int
_bus_dmamap_load_uio_direct(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{
#if 0
	vm_offset_t lastaddr;
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
		addr = (caddr_t)iov[i].iov_base;

		error = _bus_dmamap_load_buffer_direct_common(map, addr,
		    minlen, p, flags, 
				&lastaddr, &seg, first);
		first = 0;

		resid -= minlen;
	}
	if (error == 0) {
		map->dm_mapsize = uio->uio_resid;
		map->dm_nsegs = seg + 1;
	}
	return (error);
#else
	/* @@@ This needs to be re-synced with the
	 * arch/alpha/common/bus_dma.c driver. (and probably
	 * that should be moved to MI code.  Unfortunately,
	 * the above addition won't compile until this is updated.
	 * Hopefully, I will get to this shortly.
   * Darrin B. Jewell <dbj@netbsd.org>  Sun Jul 19 17:28:16 1998
	 */
	panic("_bus_dmamap_load_uio_direct: not implemented");
#endif
}

/*
 * Like _bus_dmamap_load_direct_common(), but for raw memory.
 */
int
_bus_dmamap_load_raw_direct(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{

	panic("_bus_dmamap_load_raw_direct: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * chipset-specific DMA map unload functions.
 */
void
_bus_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{

	/*
	 * No resources to free; just mark the mappings as
	 * invalid.
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
}

/*
 * Common function for DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 */
void
_bus_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{

	/*
	 * Flush the store buffer.
	 */

  /* Should we do a bus_space barrier call here?
   * Darrin B Jewell <jewell@mit.edu>  Mon Feb  9 16:56:34 1998
   */
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t; 
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags; 
{
	extern vm_offset_t avail_start, avail_end;
	vm_offset_t curaddr, lastaddr, high;
	vm_page_t m;    
	struct pglist mlist;
	int curseg, error;

	/* Always round the size. */
	size = round_page(size);

	high = avail_end - PAGE_SIZE;

	/*
	 * Allocate pages from the VM system.
	 */
	TAILQ_INIT(&mlist);
	error = uvm_pglistalloc(size, avail_start, high, alignment, boundary,
	    &mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
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
	m = m->pageq.tqe_next;

	for (; m != NULL; m = m->pageq.tqe_next) {
		curaddr = VM_PAGE_TO_PHYS(m);
#ifdef DIAGNOSTIC
		if (curaddr < avail_start || curaddr >= high) {
			printf("vm_page_alloc_memory returned non-sensical"
			    " address 0x%lx\n", curaddr);
			panic("_bus_dmamem_alloc");
		}
#endif
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

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
_bus_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	vm_page_t m;
	bus_addr_t addr;
	struct pglist mlist;
	int curseg;

	/*
	 * Build a list of pages to free back to the VM system.
	 */
	TAILQ_INIT(&mlist);
	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
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
_bus_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;  
	int flags;
{
	vm_offset_t va;
	bus_addr_t addr;
	int curseg;

#if 0
	/*
	 * If we're only mapping 1 segment, use K0SEG, to avoid
	 * TLB thrashing.
	 */
	if (nsegs == 1) {
		*kvap = (caddr_t)GENERIC_PHYS_TO_K0SEG(segs[0].ds_addr);
		return (0);
	}
#endif

	size = round_page(size);

	va = uvm_km_valloc(kernel_map, size);

	if (va == 0)
		return (ENOMEM);

	*kvap = (caddr_t)va;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += NBPG, va += NBPG, size -= NBPG) {
			if (size == 0)
				panic("_bus_dmamem_map: size botch");
			pmap_enter(pmap_kernel(), va, addr,
			    VM_PROT_READ | VM_PROT_WRITE, TRUE,
			    VM_PROT_READ | VM_PROT_WRITE);
		}
	}

	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_bus_dmamem_unmap(t, kva, size)
	bus_dma_tag_t t;
	caddr_t kva;
	size_t size;
{
	int s;

#ifdef DIAGNOSTIC
	if ((u_long)kva & PGOFSET)
		panic("_bus_dmamem_unmap");
#endif

#if 0
	/*
	 * Nothing to do if we mapped it with K0SEG.
	 */
	if (kva >= (caddr_t)GENERIC_K0SEG_BASE &&
	    kva <= (caddr_t)GENERIC_K0SEG_END)
		return;
#endif

	size = round_page(size);
	uvm_km_free(kernel_map, (vaddr_t)kva, size);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
int
_bus_dmamem_mmap(t, segs, nsegs, off, prot, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs, off, prot, flags;
{
	int i;

	for (i = 0; i < nsegs; i++) {
#ifdef DIAGNOSTIC
		if (off & PGOFSET)
			panic("_bus_dmamem_mmap: offset unaligned");
		if (segs[i].ds_addr & PGOFSET)
			panic("_bus_dmamem_mmap: segment unaligned");
		if (segs[i].ds_len & PGOFSET)
			panic("_bus_dmamem_mmap: segment size not multiple"
			    " of page size");
#endif
		if (off >= segs[i].ds_len) {
			off -= segs[i].ds_len;
			continue;
		}

		return (generic_btop((caddr_t)segs[i].ds_addr + off));
	}

	/* Page not found. */
	return (-1);
}

