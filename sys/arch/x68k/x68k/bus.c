/*	$NetBSD: bus.c,v 1.1.2.2 1999/01/30 15:07:41 minoura Exp $	*/

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
 * bus_space(9) and bus_dma(9) implementation for NetBSD/x68k.
 * These are default implementations; some buses may use their own.
 */

#include "opt_uvm.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <vm/vm.h>
#include <vm/vm_page.h>
#if defined(UVM)
#include <uvm/uvm_extern.h>
#endif

#include <machine/bus.h>

int
x68k_bus_space_alloc(t, rstart, rend, size, alignment, boundary, flags,
    bpap, bshp)
	bus_space_tag_t t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int flags;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	return (EINVAL);
}

void
x68k_bus_space_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	panic("bus_space_free: shouldn't be here");
}

u_int8_t
x68k_bus_space_read_1(t, bsh, offset)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	return (*((volatile u_int8_t *) (bsh + offset)));
}

u_int16_t
x68k_bus_space_read_2(t, bsh, offset)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	return (*((volatile u_int16_t *) (bsh + offset)));
}

u_int32_t
x68k_bus_space_read_4(t, bsh, offset)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	return (*((volatile u_int32_t *) (bsh + offset)));
}

void
x68k_bus_space_read_multi_1(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t *datap;
	bus_size_t count;
{
	while (count-- > 0) {
		*datap++ = *(volatile u_int8_t *) (bsh + offset);
	}
}

void
x68k_bus_space_read_multi_2(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t *datap;
	bus_size_t count;
{
	while (count-- > 0) {
		*datap++ = *(volatile u_int16_t *) (bsh + offset);
	}
}

void
x68k_bus_space_read_multi_4(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t *datap;
	bus_size_t count;
{
	while (count-- > 0) {
		*datap++ = *(volatile u_int32_t *) (bsh + offset);
	}
}

void
x68k_bus_space_read_region_1(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t *datap;
	bus_size_t count;
{
	volatile u_int8_t *addr = (void *) (bsh + offset);

	while (count-- > 0) {
		*datap++ = *addr++;
	}
}

void
x68k_bus_space_read_region_2(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t *datap;
	bus_size_t count;
{
	volatile u_int16_t *addr = (void *) (bsh + offset);

	while (count-- > 0) {
		*datap++ = *addr++;
	}
}

void
x68k_bus_space_read_region_4(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t *datap;
	bus_size_t count;
{
	volatile u_int32_t *addr = (void *) (bsh + offset);

	while (count-- > 0) {
		*datap++ = *addr++;
	}
}

void
x68k_bus_space_write_1(t, bsh, offset, value)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t value;
{
	*(volatile u_int8_t *) (bsh + offset) = value;
}

void
x68k_bus_space_write_2(t, bsh, offset, value)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t value;
{
	*(volatile u_int16_t *) (bsh + offset) = value;
}

void
x68k_bus_space_write_4(t, bsh, offset, value)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t value;
{
	*(volatile u_int32_t *) (bsh + offset) = value;
}

void
x68k_bus_space_write_multi_1(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t *datap;
	bus_size_t count;
{
	while (count-- > 0) {
		*(volatile u_int8_t *) (bsh + offset) = *datap++;
	}
}

void
x68k_bus_space_write_multi_2(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t *datap;
	bus_size_t count;
{
	while (count-- > 0) {
		*(volatile u_int16_t *) (bsh + offset) = *datap++;
	}
}

void
x68k_bus_space_write_multi_4(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t *datap;
	bus_size_t count;
{
	while (count-- > 0) {
		*(volatile u_int32_t *) (bsh + offset) = *datap++;
	}
}

void
x68k_bus_space_write_region_1(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t *datap;
	bus_size_t count;
{
	volatile u_int8_t *addr = (void *) (bsh + offset);

	while (count-- > 0) {
		*addr++ = *datap++;
	}
}

void
x68k_bus_space_write_region_2(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t *datap;
	bus_size_t count;
{
	volatile u_int16_t *addr = (void *) (bsh + offset);

	while (count-- > 0) {
		*addr++ = *datap++;
	}
}

void
x68k_bus_space_write_region_4(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t *datap;
	bus_size_t count;
{
	volatile u_int32_t *addr = (void *) (bsh + offset);

	while (count-- > 0) {
		*addr++ = *datap++;
	}
}

void
x68k_bus_space_set_region_1(t, bsh, offset, value, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t value;
	bus_size_t count;
{
	volatile u_int8_t *addr = (void *) (bsh + offset);

	while (count-- > 0) {
		*addr++ = value;
	}
}

void
x68k_bus_space_set_region_2(t, bsh, offset, value, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t value;
	bus_size_t count;
{
	volatile u_int16_t *addr = (void *) (bsh + offset);

	while (count-- > 0) {
		*addr++ = value;
	}
}

void
x68k_bus_space_set_region_4(t, bsh, offset, value, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t value;
	bus_size_t count;
{
	volatile u_int32_t *addr = (void *) (bsh + offset);

	while (count-- > 0) {
		*addr++ = value;
	}
}

void
x68k_bus_space_copy_region_1(t, sbsh, soffset, dbsh, doffset, count)
	bus_space_tag_t t;
	bus_space_handle_t sbsh;
	bus_size_t soffset;
	bus_space_handle_t dbsh;
	bus_size_t doffset;
	bus_size_t count;
{
	volatile u_int8_t *saddr = (void *) (sbsh + soffset);
	volatile u_int8_t *daddr = (void *) (dbsh + doffset);

	if ((u_int32_t) saddr >= (u_int32_t) daddr)
		while (count-- > 0)
			*daddr++ = *saddr++;
	else {
		saddr += count;
		daddr += count;
		while (count-- > 0)
			*--daddr = *--saddr;
	}
}

void
x68k_bus_space_copy_region_2(t, sbsh, soffset, dbsh, doffset, count)
	bus_space_tag_t t;
	bus_space_handle_t sbsh;
	bus_size_t soffset;
	bus_space_handle_t dbsh;
	bus_size_t doffset;
	bus_size_t count;
{
	volatile u_int16_t *saddr = (void *) (sbsh + soffset);
	volatile u_int16_t *daddr = (void *) (dbsh + doffset);

	if ((u_int32_t) saddr >= (u_int32_t) daddr)
		while (count-- > 0)
			*daddr++ = *saddr++;
	else {
		saddr += count;
		daddr += count;
		while (count-- > 0)
			*--daddr = *--saddr;
	}
}

void
x68k_bus_space_copy_region_4(t, sbsh, soffset, dbsh, doffset, count)
	bus_space_tag_t t;
	bus_space_handle_t sbsh;
	bus_size_t soffset;
	bus_space_handle_t dbsh;
	bus_size_t doffset;
	bus_size_t count;
{
	volatile u_int32_t *saddr = (void *) (sbsh + soffset);
	volatile u_int32_t *daddr = (void *) (dbsh + doffset);

	if ((u_int32_t) saddr >= (u_int32_t) daddr)
		while (count-- > 0)
			*daddr++ = *saddr++;
	else {
		saddr += count;
		daddr += count;
		while (count-- > 0)
			*--daddr = *--saddr;
	}
}


extern paddr_t avail_end;

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
x68k_bus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
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
	map->x68k_dm_maxsegsz = maxsegsz;
	map->x68k_dm_boundary = boundary;
	map->x68k_dm_bounce_thresh = t->_bounce_thresh;
	map->x68k_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
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
x68k_bus_dmamap_destroy(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{

	free(map, M_DMAMAP);
}

/*
 * Common function for loading a DMA map with a linear buffer.  May
 * be called by bus-specific DMA map load functions.
 */
int
x68k_bus_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	paddr_t lastaddr;
	int seg, error;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

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
x68k_bus_dmamap_load_mbuf(t, map, m0, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m0;
	int flags;
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
		panic("x68k_bus_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->x68k_dm_size)
		return (EINVAL);

	first = 1;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
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
x68k_bus_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{
#if 0
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
x68k_bus_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{

	panic("x68k_bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * bus-specific DMA map unload functions.
 */
void
x68k_bus_dmamap_unload(t, map)
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
 * by bus-specific DMA map synchronization functions.
 */
void
x68k_bus_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{

	/* Nothing to do here. */
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
x68k_bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{

	return (x68k_bus_dmamem_alloc_range(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags, 0, trunc_page(avail_end)));
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
x68k_bus_dmamem_free(t, segs, nsegs)
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

#if defined(UVM)
	uvm_pglistfree(&mlist);
#else
	vm_page_free_memory(&mlist);
#endif
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
x68k_bus_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	vaddr_t va;
	bus_addr_t addr;
	int curseg;
	extern vm_map_t kernel_map;

	size = round_page(size);

#if defined(UVM)
	va = uvm_km_valloc(kernel_map, size);
#else
	va = kmem_alloc_pageable(kernel_map, size);
#endif

	if (va == 0)
		return (ENOMEM);

	*kvap = (caddr_t)va;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += NBPG, va += NBPG, size -= NBPG) {
			if (size == 0)
				panic("x68k_bus_dmamem_map: size botch");
			pmap_enter(pmap_kernel(), va, addr,
			    VM_PROT_READ | VM_PROT_WRITE, TRUE);
		}
	}

	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
x68k_bus_dmamem_unmap(t, kva, size)
	bus_dma_tag_t t;
	caddr_t kva;
	size_t size;
{
	extern vm_map_t kernel_map;

#ifdef DIAGNOSTIC
	if ((u_long)kva & PGOFSET)
		panic("x68k_bus_dmamem_unmap");
#endif

	size = round_page(size);

#if defined(UVM)
	uvm_km_free(kernel_map, (vaddr_t)kva, size);
#else
	kmem_free(kernel_map, (vaddr_t)kva, size);
#endif
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
int
x68k_bus_dmamem_mmap(t, segs, nsegs, off, prot, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs, off, prot, flags;
{
	int i;

	for (i = 0; i < nsegs; i++) {
#ifdef DIAGNOSTIC
		if (off & PGOFSET)
			panic("x68k_bus_dmamem_mmap: offset unaligned");
		if (segs[i].ds_addr & PGOFSET)
			panic("x68k_bus_dmamem_mmap: segment unaligned");
		if (segs[i].ds_len & PGOFSET)
			panic("x68k_bus_dmamem_mmap: segment size not multiple"
			    " of page size");
#endif
		if (off >= segs[i].ds_len) {
			off -= segs[i].ds_len;
			continue;
		}

		return (m68k_btop((caddr_t)segs[i].ds_addr + off));
	}

	/* Page not found. */
	return (-1);
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
int
x68k_bus_dmamap_load_buffer(map, buf, buflen, p, flags,
    lastaddrp, segp, first)
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
	paddr_t *lastaddrp;
	int *segp;
	int first;
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
		curaddr = pmap_extract(pmap, vaddr);

		/*
		 * If we're beyond the bounce threshold, notify
		 * the caller.
		 */
		if (map->x68k_dm_bounce_thresh != 0 &&
		    curaddr >= map->x68k_dm_bounce_thresh)

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = NBPG - ((u_long)vaddr & PGOFSET);
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
			     map->x68k_dm_maxsegsz &&
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
x68k_bus_dmamem_alloc_range(t, size, alignment, boundary, segs, nsegs, rsegs,
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
	vm_page_t m;
	struct pglist mlist;
	int curseg, error;

	/* Always round the size. */
	size = round_page(size);

	/*
	 * Allocate pages from the VM system.
	 */
	TAILQ_INIT(&mlist);
#if defined(UVM)
	error = uvm_pglistalloc(size, low, high, alignment, boundary,
	    &mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
#else
	error = vm_page_alloc_memory(size, low, high,
	    alignment, boundary, &mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
#endif
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
		if (curaddr < low || curaddr >= high) {
			printf("vm_page_alloc_memory returned non-sensical"
			    " address 0x%lx\n", curaddr);
			panic("x68k_bus_dmamem_alloc_range");
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
