/*	$NetBSD: bus_machdep.c,v 1.3 2002/06/02 14:44:45 drochner Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

#include "opt_largepages.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#define _X86_64_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <machine/isa_machdep.h>

/*
 * Extent maps to manage I/O and memory space.  Allocate
 * storage for 8 regions in each, initially.  Later, ioport_malloc_safe
 * will indicate that it's safe to use malloc() to dynamically allocate
 * region descriptors.
 *
 * N.B. At least two regions are _always_ allocated from the iomem
 * extent map; (0 -> ISA hole) and (end of ISA hole -> end of RAM).
 *
 * The extent maps are not static!  Machine-dependent ISA and EISA
 * routines need access to them for bus address space allocation.
 */
static	long ioport_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
static	long iomem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
struct	extent *ioport_ex;
struct	extent *iomem_ex;
static	int ioport_malloc_safe;

int	x86_64_mem_add_mapping __P((bus_addr_t, bus_size_t,
	    int, bus_space_handle_t *));

int	_bus_dmamap_load_buffer __P((bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int, paddr_t *, int *, int));

void
x86_64_bus_space_init()
{
	/*
	 * Initialize the I/O port and I/O mem extent maps.
	 * Note: we don't have to check the return value since
	 * creation of a fixed extent map will never fail (since
	 * descriptor storage has already been allocated).
	 *
	 * N.B. The iomem extent manages _all_ physical addresses
	 * on the machine.  When the amount of RAM is found, the two
	 * extents of RAM are allocated from the map (0 -> ISA hole
	 * and end of ISA hole -> end of RAM).
	 */
	ioport_ex = extent_create("ioport", 0x0, 0xffff, M_DEVBUF,
	    (caddr_t)ioport_ex_storage, sizeof(ioport_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);
	iomem_ex = extent_create("iomem", 0x0, 0xffffffff, M_DEVBUF,
	    (caddr_t)iomem_ex_storage, sizeof(iomem_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);
}

void
x86_64_bus_space_mallocok()
{

	ioport_malloc_safe = 1;
}

int
x86_64_memio_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	int error;
	struct extent *ex;

	/*
	 * Pick the appropriate extent map.
	 */
	if (t == X86_64_BUS_SPACE_IO) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		ex = ioport_ex;
	} else if (t == X86_64_BUS_SPACE_MEM)
		ex = iomem_ex;
	else
		panic("x86_64_memio_map: bad bus space tag");

	/*
	 * Before we go any further, let's make sure that this
	 * region is available.
	 */
	error = extent_alloc_region(ex, bpa, size,
	    EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0));
	if (error)
		return (error);

	/*
	 * For I/O space, that's all she wrote.
	 */
	if (t == X86_64_BUS_SPACE_IO) {
		*bshp = bpa;
		return (0);
	}

	if (bpa >= IOM_BEGIN && (bpa + size) <= IOM_END) {
		*bshp = (bus_space_handle_t)ISA_HOLE_VADDR(bpa);
		return(0);
	}

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	error = x86_64_mem_add_mapping(bpa, size,
		(flags & BUS_SPACE_MAP_CACHEABLE) != 0, bshp);
	if (error) {
		if (extent_free(ex, bpa, size, EX_NOWAIT |
		    (ioport_malloc_safe ? EX_MALLOCOK : 0))) {
			printf("x86_64_memio_map: pa 0x%lx, size 0x%lx\n",
			    bpa, size);
			printf("x86_64_memio_map: can't free region\n");
		}
	}

	return (error);
}

int
_x86_64_memio_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{

	/*
	 * For I/O space, just fill in the handle.
	 */
	if (t == X86_64_BUS_SPACE_IO) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		*bshp = bpa;
		return (0);
	}

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	return (x86_64_mem_add_mapping(bpa, size,
	    (flags & BUS_SPACE_MAP_CACHEABLE) != 0, bshp));
}

int
x86_64_memio_alloc(t, rstart, rend, size, alignment, boundary, flags,
    bpap, bshp)
	bus_space_tag_t t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int flags;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	struct extent *ex;
	u_long bpa;
	int error;

	/*
	 * Pick the appropriate extent map.
	 */
	if (t == X86_64_BUS_SPACE_IO) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		ex = ioport_ex;
	} else if (t == X86_64_BUS_SPACE_MEM)
		ex = iomem_ex;
	else
		panic("x86_64_memio_alloc: bad bus space tag");

	/*
	 * Sanity check the allocation against the extent's boundaries.
	 */
	if (rstart < ex->ex_start || rend > ex->ex_end)
		panic("x86_64_memio_alloc: bad region start/end");

	/*
	 * Do the requested allocation.
	 */
	error = extent_alloc_subregion(ex, rstart, rend, size, alignment,
	    boundary,
	    EX_FAST | EX_NOWAIT | (ioport_malloc_safe ?  EX_MALLOCOK : 0),
	    &bpa);

	if (error)
		return (error);

	/*
	 * For I/O space, that's all she wrote.
	 */
	if (t == X86_64_BUS_SPACE_IO) {
		*bshp = *bpap = bpa;
		return (0);
	}

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	error = x86_64_mem_add_mapping(bpa, size,
	    (flags & BUS_SPACE_MAP_CACHEABLE) != 0, bshp);
	if (error) {
		if (extent_free(iomem_ex, bpa, size, EX_NOWAIT |
		    (ioport_malloc_safe ? EX_MALLOCOK : 0))) {
			printf("x86_64_memio_alloc: pa 0x%lx, size 0x%lx\n",
			    bpa, size);
			printf("x86_64_memio_alloc: can't free region\n");
		}
	}

	*bpap = bpa;

	return (error);
}

int
x86_64_mem_add_mapping(bpa, size, cacheable, bshp)
	bus_addr_t bpa;
	bus_size_t size;
	int cacheable;
	bus_space_handle_t *bshp;
{
	u_long pa, endpa;
	vaddr_t va;
	pt_entry_t *pte;

	pa = x86_64_trunc_page(bpa);
	endpa = x86_64_round_page(bpa + size);

#ifdef DIAGNOSTIC
	if (endpa <= pa)
		panic("x86_64_mem_add_mapping: overflow");
#endif

	va = uvm_km_valloc(kernel_map, endpa - pa);
	if (va == 0)
		return (ENOMEM);

	*bshp = (bus_space_handle_t)(va + (bpa & PGOFSET));

	for (; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);

		pte = kvtopte(va);
		if (cacheable)
			*pte &= ~PG_N;
		else
			*pte |= PG_N;
#ifdef LARGEPAGES
		if (*pte & PG_PS)
			pmap_update_pg(va & PG_LGFRAME);
		else
#endif
			pmap_update_pg(va);
	}
 
	return 0;
}

void
x86_64_memio_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	struct extent *ex;
	u_long va, endva;
	bus_addr_t bpa;

	/*
	 * Find the correct extent and bus physical address.
	 */
	if (t == X86_64_BUS_SPACE_IO) {
		ex = ioport_ex;
		bpa = bsh;
	} else if (t == X86_64_BUS_SPACE_MEM) {
		ex = iomem_ex;

		if (bsh >= atdevbase &&
		    (bsh + size) <= (atdevbase + IOM_SIZE)) {
			bpa = (bus_addr_t)ISA_PHYSADDR(bsh);
			goto ok;
		}

		va = x86_64_trunc_page(bsh);
		endva = x86_64_round_page(bsh + size);

#ifdef DIAGNOSTIC
		if (endva <= va)
			panic("x86_64_memio_unmap: overflow");
#endif

		(void) pmap_extract(pmap_kernel(), va, &bpa);
		bpa += (bsh & PGOFSET);

		/*
		 * Free the kernel virtual mapping.
		 */
		uvm_km_free(kernel_map, va, endva - va);
	} else
		panic("x86_64_memio_unmap: bad bus space tag");

ok:
	if (extent_free(ex, bpa, size,
	    EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0))) {
		printf("x86_64_memio_unmap: %s 0x%lx, size 0x%lx\n",
		    (t == X86_64_BUS_SPACE_IO) ? "port" : "pa", bpa, size);
		printf("x86_64_memio_unmap: can't free region\n");
	}
}

void    
x86_64_memio_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{

	/* x86_64_memio_unmap() does all that we need to do. */
	x86_64_memio_unmap(t, bsh, size);
}

int
x86_64_memio_subregion(t, bsh, offset, size, nbshp)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset, size;
	bus_space_handle_t *nbshp;
{

	*nbshp = bsh + offset;
	return (0);
}

paddr_t
x86_64_memio_mmap(t, addr, off, prot, flags)
	bus_space_tag_t t;
	bus_addr_t addr;
	off_t off;
	int prot;
	int flags;
{

	/* Can't mmap I/O space. */
	if (t == X86_64_BUS_SPACE_IO)
		return (-1);

	/*
	 * "addr" is the base address of the device we're mapping.
	 * "off" is the offset into that device.
	 *
	 * Note we are called for each "page" in the device that
	 * the upper layers want to map.
	 */
	return (x86_64_btop(addr + off));
}

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
	struct x86_64_bus_dmamap *map;
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
	mapsize = sizeof(struct x86_64_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	memset(mapstore, 0, mapsize);
	map = (struct x86_64_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_bounce_thresh = t->_bounce_thresh;
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

	free(map, M_DMAMAP);
}

/*
 * Common function for loading a DMA map with a linear buffer.  May
 * be called by bus-specific DMA map load functions.
 */
int
_bus_dmamap_load(t, map, buf, buflen, p, flags)
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

	if (buflen > map->_dm_size)
		return (EINVAL);

	seg = 0;
	error = _bus_dmamap_load_buffer(t, map, buf, buflen, p, flags,
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
_bus_dmamap_load_mbuf(t, map, m0, flags)
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
		panic("_bus_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return (EINVAL);

	first = 1;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		error = _bus_dmamap_load_buffer(t, map, m->m_data, m->m_len,
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
 * Like _bus_dmamap_load(), but for uios.
 */
int
_bus_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
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

		error = _bus_dmamap_load_buffer(t, map, addr, minlen,
		    p, flags, &lastaddr, &seg, first);
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
_bus_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{

	panic("_bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * bus-specific DMA map unload functions.
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
 * by bus-specific DMA map synchronization functions.
 */
void
_bus_dmamap_sync(t, map, offset, len, ops)
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
_bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	extern paddr_t avail_end;
	paddr_t high, tend;

	/* XXXfvdl restrict all dmamem allocs to < 4G for now */
	tend = trunc_page(avail_end);
	high = tend > 0xffffffff ? 0xffffffff : tend;
	return (_bus_dmamem_alloc_range(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags, 0, high));
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
	struct vm_page *m;
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
	vaddr_t va;
	bus_addr_t addr;
	int curseg;

	size = round_page(size);

	va = uvm_km_valloc(kernel_map, size);

	if (va == 0)
		return (ENOMEM);

	*kvap = (caddr_t)va;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += PAGE_SIZE, va += PAGE_SIZE, size -= PAGE_SIZE) {
			if (size == 0)
				panic("_bus_dmamem_map: size botch");
			pmap_enter(pmap_kernel(), va, addr,
			    VM_PROT_READ | VM_PROT_WRITE,
			    PMAP_WIRED | VM_PROT_READ | VM_PROT_WRITE);
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
paddr_t
_bus_dmamem_mmap(t, segs, nsegs, off, prot, flags)
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

		return (x86_64_btop((caddr_t)segs[i].ds_addr + off));
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
_bus_dmamap_load_buffer(t, map, buf, buflen, p, flags, lastaddrp, segp, first)
	bus_dma_tag_t t;
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
	bmask  = ~(map->_dm_boundary - 1);

	for (seg = *segp; buflen > 0 ; ) {
		/*
		 * Get the physical address for this segment.
		 */
		(void) pmap_extract(pmap, vaddr, &curaddr);

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
			map->dm_segs[seg].ds_addr = curaddr;
			map->dm_segs[seg].ds_len = sgsize;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			     map->_dm_maxsegsz &&
			    (map->_dm_boundary == 0 ||
			     (map->dm_segs[seg].ds_addr & bmask) ==
			     (curaddr & bmask)))
				map->dm_segs[seg].ds_len += sgsize;
			else {
				if (++seg >= map->_dm_segcnt)
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
	lastaddr = segs[curseg].ds_addr = VM_PAGE_TO_PHYS(m);
	segs[curseg].ds_len = PAGE_SIZE;
	m = m->pageq.tqe_next;

	for (; m != NULL; m = m->pageq.tqe_next) {
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
			segs[curseg].ds_addr = curaddr;
			segs[curseg].ds_len = PAGE_SIZE;
		}
		lastaddr = curaddr;
	}

	*rsegs = curseg + 1;

	return (0);
}
