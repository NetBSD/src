/*	$NetBSD: int_bus_dma.c,v 1.8 2002/06/02 14:44:45 drochner Exp $	*/

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
 * The integrator board has memory steering hardware that means that
 * the normal physical addresses used by the processor cannot be used
 * for DMA.  Instead we have to use the "core module alias mapping
 * addresses".  We don't use these for normal processor accesses since
 * they are much slower than the direct addresses when accessing
 * memory on the local board.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/vnode.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#define _ARM32_BUS_DMA_PRIVATE
#include <evbarm/integrator/int_bus_dma.h>

#include <machine/cpu.h>
#include <arm/cpufunc.h>

static int	integrator_bus_dmamap_load_buffer __P((bus_dma_tag_t,
		    bus_dmamap_t, void *, bus_size_t, struct proc *, int,
		    vm_offset_t *, int *, int));
static int	integrator_bus_dma_inrange __P((bus_dma_segment_t *, int,
		    bus_addr_t));

/*
 * Common function for loading a DMA map with a linear buffer.  May
 * be called by bus-specific DMA map load functions.
 */
int
integrator_bus_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	vm_offset_t lastaddr;
	int seg, error;

#ifdef DEBUG_DMA
	printf("dmamap_load: t=%p map=%p buf=%p len=%lx p=%p f=%d\n",
	    t, map, buf, buflen, p, flags);
#endif	/* DEBUG_DMA */

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
		return (EINVAL);

	seg = 0;
	error = integrator_bus_dmamap_load_buffer(t, map, buf, buflen, p, flags,
	    &lastaddr, &seg, 1);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;
		map->_dm_proc = p;
	}
#ifdef DEBUG_DMA
	printf("dmamap_load: error=%d\n", error);
#endif	/* DEBUG_DMA */
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
integrator_bus_dmamap_load_mbuf(t, map, m0, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m0;
	int flags;
{
	vm_offset_t lastaddr;
	int seg, error, first;
	struct mbuf *m;

#ifdef DEBUG_DMA
	printf("dmamap_load_mbuf: t=%p map=%p m0=%p f=%d\n",
	    t, map, m0, flags);
#endif	/* DEBUG_DMA */

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("integrator_bus_dmamap_load_mbuf: no packet header");
#endif	/* DIAGNOSTIC */

	if (m0->m_pkthdr.len > map->_dm_size)
		return (EINVAL);

	first = 1;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		error = integrator_bus_dmamap_load_buffer(t, map, m->m_data,
		    m->m_len, NULL, flags, &lastaddr, &seg, first);
		first = 0;
	}
	if (error == 0) {
		map->dm_mapsize = m0->m_pkthdr.len;
		map->dm_nsegs = seg + 1;
		map->_dm_proc = NULL;	/* always kernel */
	}
#ifdef DEBUG_DMA
	printf("dmamap_load_mbuf: error=%d\n", error);
#endif	/* DEBUG_DMA */
	return (error);
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
integrator_bus_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{
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
			panic("integrator_bus_dmamap_load_uio: USERSPACE but no proc");
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

		error = integrator_bus_dmamap_load_buffer(t, map, addr, minlen,
		    p, flags, &lastaddr, &seg, first);
		first = 0;

		resid -= minlen;
	}
	if (error == 0) {
		map->dm_mapsize = uio->uio_resid;
		map->dm_nsegs = seg + 1;
		map->_dm_proc = p;
	}
	return (error);
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */

extern vm_offset_t physical_start;
extern vm_offset_t physical_freestart;
extern vm_offset_t physical_freeend;
extern vm_offset_t physical_end;

int
integrator_bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	int error;
#ifdef DEBUG_DMA
	printf("dmamem_alloc t=%p size=%lx align=%lx boundary=%lx segs=%p nsegs=%x rsegs=%p flags=%x\n",
	    t, size, alignment, boundary, segs, nsegs, rsegs, flags);
#endif	/* DEBUG_DMA */
	error =  (integrator_bus_dmamem_alloc_range(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags, trunc_page(physical_start), trunc_page(physical_end)));
#ifdef DEBUG_DMA
	printf("dmamem_alloc: =%d\n", error);
#endif	/* DEBUG_DMA */
	return(error);
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
integrator_bus_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	struct vm_page *m;
	bus_addr_t addr;
	struct pglist mlist;
	int curseg;

#ifdef DEBUG_DMA
	printf("dmamem_free: t=%p segs=%p nsegs=%x\n", t, segs, nsegs);
#endif	/* DEBUG_DMA */

	/*
	 * Build a list of pages to free back to the VM system.
	 */
	TAILQ_INIT(&mlist);
	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += PAGE_SIZE) {
			m = PHYS_TO_VM_PAGE(CM_ALIAS_TO_LOCAL(addr));
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
integrator_bus_dmamem_map(t, segs, nsegs, size, kvap, flags)
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
	pt_entry_t *ptep/*, pte*/;

#ifdef DEBUG_DMA
	printf("dmamem_map: t=%p segs=%p nsegs=%x size=%lx flags=%x\n", t,
	    segs, nsegs, (unsigned long)size, flags);
#endif	/* DEBUG_DMA */

	size = round_page(size);
	va = uvm_km_valloc(kernel_map, size);

	if (va == 0)
		return (ENOMEM);

	*kvap = (caddr_t)va;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += NBPG, va += NBPG, size -= NBPG) {
#ifdef DEBUG_DMA
			printf("wiring p%lx to v%lx", CM_ALIAS_TO_LOCAL(addr),
			    va);
#endif	/* DEBUG_DMA */
			if (size == 0)
				panic("integrator_bus_dmamem_map: size botch");
			pmap_enter(pmap_kernel(), va, CM_ALIAS_TO_LOCAL(addr),
			    VM_PROT_READ | VM_PROT_WRITE,
			    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);
			/*
			 * If the memory must remain coherent with the
			 * cache then we must make the memory uncacheable
			 * in order to maintain virtual cache coherency.
			 * We must also guarentee the cache does not already
			 * contain the virtal addresses we are making
			 * uncacheable.
			 */
			if (flags & BUS_DMA_COHERENT) {
				cpu_dcache_wbinv_range(va, NBPG);
				cpu_drain_writebuf();
				ptep = vtopte(va);
				*ptep &= ~(L2_C | L2_B);
				tlb_flush();
			}
#ifdef DEBUG_DMA
			ptep = vtopte(va);
			printf(" pte=v%p *pte=%x\n", ptep, *ptep);
#endif	/* DEBUG_DMA */
		}
	}
	pmap_update(pmap_kernel());
#ifdef DEBUG_DMA
	printf("dmamem_map: =%p\n", *kvap);
#endif	/* DEBUG_DMA */
	return (0);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
integrator_bus_dmamem_mmap(t, segs, nsegs, off, prot, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	off_t off;
	int prot, flags;
{
	int i;

	for (i = 0; i < nsegs; i++) {
#ifdef DIAGNOSTIC
		if (off & PGOFSET)
			panic("integrator_bus_dmamem_mmap: offset unaligned");
		if (segs[i].ds_addr & PGOFSET)
			panic("integrator_bus_dmamem_mmap: segment unaligned");
		if (segs[i].ds_len & PGOFSET)
			panic("integrator_bus_dmamem_mmap: segment size not multiple"
			    " of page size");
#endif	/* DIAGNOSTIC */
		if (off >= segs[i].ds_len) {
			off -= segs[i].ds_len;
			continue;
		}

		return arm_btop((u_long)CM_ALIAS_TO_LOCAL(segs[i].ds_addr) + off);
	}

	/* Page not found. */
	return -1;
}

/**********************************************************************
 * DMA utility functions
 **********************************************************************/

/*
 * Utility function to load a linear buffer.  lastaddrp holds state
 * between invocations (for multiple-buffer loads).  segp contains
 * the starting segment on entrace, and the ending segment on exit.
 * first indicates if this is the first invocation of this function.
 */
static int
integrator_bus_dmamap_load_buffer(t, map, buf, buflen, p, flags, lastaddrp, 
    segp, first)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
	vm_offset_t *lastaddrp;
	int *segp;
	int first;
{
	bus_size_t sgsize;
	bus_addr_t curaddr, lastaddr, baddr, bmask;
	vm_offset_t vaddr = (vm_offset_t)buf;
	int seg;
	pmap_t pmap;

#ifdef DEBUG_DMA
	printf("integrator_bus_dmamem_load_buffer(buf=%p, len=%lx, flags=%d, 1st=%d)\n",
	    buf, buflen, flags, first);
#endif	/* DEBUG_DMA */

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	lastaddr = *lastaddrp;
	bmask  = ~(map->_dm_boundary - 1);

	for (seg = *segp; buflen > 0; ) {
		/*
		 * Get the physical address for this segment.
		 */
		(void) pmap_extract(pmap, (vaddr_t)vaddr, &curaddr);

		/*
		 * Make sure we're in an allowed DMA range.
		 */
		if (t->_ranges != NULL &&
		    integrator_bus_dma_inrange(t->_ranges, t->_nranges, curaddr) == 0)
			return (EINVAL);

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
		 * previous segment if possible.
		 */
		if (first) {
			map->dm_segs[seg].ds_addr = LOCAL_TO_CM_ALIAS(curaddr);
			map->dm_segs[seg].ds_len = sgsize;
			map->dm_segs[seg]._ds_vaddr = vaddr;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			     map->_dm_maxsegsz &&
			    (map->_dm_boundary == 0 ||
			     (map->dm_segs[seg].ds_addr & bmask) ==
			     (LOCAL_TO_CM_ALIAS(curaddr) & bmask)))
				map->dm_segs[seg].ds_len += sgsize;
			else {
				if (++seg >= map->_dm_segcnt)
					break;
				map->dm_segs[seg].ds_addr = LOCAL_TO_CM_ALIAS(curaddr);
				map->dm_segs[seg].ds_len = sgsize;
				map->dm_segs[seg]._ds_vaddr = vaddr;
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
 * Check to see if the specified page is in an allowed DMA range.
 */
static int
integrator_bus_dma_inrange(ranges, nranges, curaddr)
	bus_dma_segment_t *ranges;
	int nranges;
	bus_addr_t curaddr;
{
	bus_dma_segment_t *ds;
	int i;

	for (i = 0, ds = ranges; i < nranges; i++, ds++) {
		if (curaddr >= CM_ALIAS_TO_LOCAL(ds->ds_addr) &&
		    round_page(curaddr) <= (CM_ALIAS_TO_LOCAL(ds->ds_addr) + ds->ds_len))
			return (1);
	}

	return (0);
}

/*
 * Allocate physical memory from the given physical address range.
 * Called by DMA-safe memory allocation methods.
 */
int
integrator_bus_dmamem_alloc_range(t, size, alignment, boundary, segs, nsegs, rsegs,
    flags, low, high)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
	vm_offset_t low;
	vm_offset_t high;
{
	vm_offset_t curaddr, lastaddr;
	struct vm_page *m;
	struct pglist mlist;
	int curseg, error;

#ifdef DEBUG_DMA
	printf("alloc_range: t=%p size=%lx align=%lx boundary=%lx segs=%p nsegs=%x rsegs=%p flags=%x lo=%lx hi=%lx\n",
	    t, size, alignment, boundary, segs, nsegs, rsegs, flags, low, high);
#endif	/* DEBUG_DMA */

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
	segs[curseg].ds_addr = LOCAL_TO_CM_ALIAS(lastaddr);
	segs[curseg].ds_len = PAGE_SIZE;
#ifdef DEBUG_DMA
		printf("alloc: page %lx\n", lastaddr);
#endif	/* DEBUG_DMA */
	m = m->pageq.tqe_next;

	for (; m != NULL; m = m->pageq.tqe_next) {
		curaddr = VM_PAGE_TO_PHYS(m);
#ifdef DIAGNOSTIC
		if (curaddr < low || curaddr >= high) {
			printf("uvm_pglistalloc returned non-sensical"
			    " address 0x%lx\n", curaddr);
			panic("integrator_bus_dmamem_alloc_range");
		}
#endif	/* DIAGNOSTIC */
#ifdef DEBUG_DMA
		printf("alloc: page %lx\n", curaddr);
#endif	/* DEBUG_DMA */
		if (curaddr == (lastaddr + PAGE_SIZE))
			segs[curseg].ds_len += PAGE_SIZE;
		else {
			curseg++;
			segs[curseg].ds_addr = LOCAL_TO_CM_ALIAS(curaddr);
			segs[curseg].ds_len = PAGE_SIZE;
		}
		lastaddr = curaddr;
	}

	*rsegs = curseg + 1;

	return (0);
}
