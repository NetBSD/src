/*	$NetBSD: bus.c,v 1.15.6.1 1999/12/27 18:31:46 wrstuden Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Chris G. Demetriou.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/cpu.h>
#include <m68k/cacheops.h>
#include <machine/bus.h>

static int  _bus_dmamap_load_buffer __P((bus_dma_tag_t tag, bus_dmamap_t,
		void *, bus_size_t, struct proc *, int, paddr_t *,
		int *, int));
static int  _bus_dmamem_alloc_range __P((bus_dma_tag_t tag, bus_size_t size,
		bus_size_t alignment, bus_size_t boundary,
		bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
		paddr_t low, paddr_t high));
static int  bus_mem_add_mapping __P((bus_space_tag_t t, bus_addr_t bpa,
		bus_size_t size, int flags, bus_space_handle_t *bsph));

extern struct extent *iomem_ex;
extern int iomem_malloc_safe;

extern paddr_t avail_end;

/*
 * We need these for the early memory allocator. The idea is this:
 * Allocate VA-space through ptextra (atari_init.c:startc()). When
 * The VA & size of this space are known, call bootm_init().
 * Until the VM-system is up, bus_mem_add_mapping() allocates it's virtual
 * addresses from this extent-map.
 *
 * This allows for the console code to use the bus_space interface at a
 * very early stage of the system configuration.
 */
static pt_entry_t	*bootm_ptep;
static long		bootm_ex_storage[EXTENT_FIXED_STORAGE_SIZE(32) /
								sizeof(long)];
static struct extent	*bootm_ex;

void bootm_init(vaddr_t, pt_entry_t *, u_long);
static vaddr_t	bootm_alloc(paddr_t pa, u_long size, int flags);
static int	bootm_free(vaddr_t va, u_long size);

void
bootm_init(va, ptep, size)
vaddr_t		va;
pt_entry_t	*ptep;
u_long		size;
{
	bootm_ex = extent_create("bootmem", va, va + size, M_DEVBUF,
	    (caddr_t)bootm_ex_storage, sizeof(bootm_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);
	bootm_ptep = ptep;
}

vaddr_t
bootm_alloc(pa, size, flags)
paddr_t	pa;
u_long	size;
int	flags;
{
	pt_entry_t	*pg, *epg;
	pt_entry_t	pg_proto;
	vaddr_t		va, rva;

	if (extent_alloc(bootm_ex, size, NBPG, 0, EX_NOWAIT, &rva)) {
		printf("bootm_alloc fails! Not enough fixed extents?\n");
		printf("Requested extent: pa=%lx, size=%lx\n",
						(u_long)pa, size);
		return 0;
	}
	
	pg  = &bootm_ptep[btoc(rva - bootm_ex->ex_start)];
	epg = &pg[btoc(size)];
	va  = rva;
	pg_proto = pa | PG_RW | PG_V;
	if (!(flags & BUS_SPACE_MAP_CACHEABLE))
		pg_proto |= PG_CI;
	while(pg < epg) {
		*pg++     = pg_proto;
		pg_proto += NBPG;
#if defined(M68040) || defined(M68060)
		if (mmutype == MMU_68040) {
			DCFP(pa);
			pa += NBPG;
		}
#endif
		TBIS(va);
		va += NBPG;
	}
	return rva;
}

int
bootm_free(va, size)
vaddr_t	va;
u_long	size;
{
	if ((va < bootm_ex->ex_start) || ((va + size) > bootm_ex->ex_end))
		return 0; /* Not for us! */
	extent_free(bootm_ex, va, size, EX_NOWAIT);
	return 1;
}

int
bus_space_map(t, bpa, size, flags, mhp)
bus_space_tag_t		t;
bus_addr_t		bpa;
bus_size_t		size;
int			flags;
bus_space_handle_t	*mhp;
{
	paddr_t	pa, endpa;
	int	error;

	/*
	 * Before we go any further, let's make sure that this
	 * region is available.
	 */
	error = extent_alloc_region(iomem_ex, bpa + t->base, size,
			EX_NOWAIT | (iomem_malloc_safe ? EX_MALLOCOK : 0));

	if (error)
		return (error);

	pa    = m68k_trunc_page(bpa + t->base);
	endpa = m68k_round_page((bpa + t->base + size) - 1);

	error = bus_mem_add_mapping(t, bpa, size, flags, mhp);
	if (error) {
		if (extent_free(iomem_ex, bpa + t->base, size, EX_NOWAIT |
				(iomem_malloc_safe ? EX_MALLOCOK : 0))) {
		    printf("bus_space_map: pa 0x%lx, size 0x%lx\n", bpa, size);
		    printf("bus_space_map: can't free region\n");
		}
	}
	return (error);
}

int
bus_space_alloc(t, rstart, rend, size, alignment, boundary, flags, bpap, bshp)
	bus_space_tag_t t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int flags;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	u_long bpa;
	int error;

#ifdef DIAGNOSTIC
	/*
	 * Sanity check the allocation against the extent's boundaries.
	 * XXX: Since we manage the whole of memory in a single map,
	 *      this is nonsense for now! Brace it DIAGNOSTIC....
	 */
	if ((rstart + t->base) < iomem_ex->ex_start
				|| (rend + t->base) > iomem_ex->ex_end)
		panic("bus_space_alloc: bad region start/end");
#endif /* DIAGNOSTIC */

	/*
	 * Do the requested allocation.
	 */
	error = extent_alloc_subregion(iomem_ex, rstart + t->base,
	    rend + t->base, size, alignment, boundary,
	    EX_FAST | EX_NOWAIT | (iomem_malloc_safe ?  EX_MALLOCOK : 0),
	    &bpa);

	if (error)
		return (error);

	/*
	 * Map the bus physical address to a kernel virtual address.
	 */
	error = bus_mem_add_mapping(t, bpa, size, flags, bshp);
	if (error) {
		if (extent_free(iomem_ex, bpa, size, EX_NOWAIT |
		    (iomem_malloc_safe ? EX_MALLOCOK : 0))) {
			printf("bus_space_alloc: pa 0x%lx, size 0x%lx\n",
			    bpa, size);
			printf("bus_space_alloc: can't free region\n");
		}
	}

	*bpap = bpa;

	return (error);
}

static int
bus_mem_add_mapping(t, bpa, size, flags, bshp)
bus_space_tag_t		t;
bus_addr_t		bpa;
bus_size_t		size;
int			flags;
bus_space_handle_t	*bshp;
{
	vaddr_t	va;
	paddr_t	pa, endpa;

	pa    = m68k_trunc_page(bpa + t->base);
	endpa = m68k_round_page((bpa + t->base + size) - 1);

#ifdef DIAGNOSTIC
	if (endpa <= pa)
		panic("bus_mem_add_mapping: overflow");
#endif

	if (kernel_map == NULL) {
		/*
		 * The VM-system is not yet operational, allocate from
		 * a special pool.
		 */
		va = bootm_alloc(pa, endpa - pa, flags);
		if (va == 0)
			return (ENOMEM);
		*bshp = (caddr_t)(va + (bpa & PGOFSET));
		return (0);
	}

	va = uvm_km_valloc(kernel_map, endpa - pa);
	if (va == 0)
		return (ENOMEM);

	*bshp = (caddr_t)(va + (bpa & PGOFSET));

	for(; pa < endpa; pa += NBPG, va += NBPG) {
		pmap_enter(pmap_kernel(), (vaddr_t)va, pa,
				VM_PROT_READ|VM_PROT_WRITE, PMAP_WIRED);
		if (!(flags & BUS_SPACE_MAP_CACHEABLE))
			pmap_changebit(pa, PG_CI, TRUE);
	}
	return (0);
}

void
bus_space_unmap(t, bsh, size)
bus_space_tag_t		t;
bus_space_handle_t	bsh;
bus_size_t		size;
{
	vaddr_t	va, endva;
	paddr_t bpa;

	va = m68k_trunc_page(bsh);
	endva = m68k_round_page((bsh + size) - 1);
#ifdef DIAGNOSTIC
	if (endva < va)
		panic("unmap_iospace: overflow");
#endif

	(void) pmap_extract(pmap_kernel(), va, &bpa);
	bpa += ((u_long)bsh & PGOFSET);

	/*
	 * Free the kernel virtual mapping.
	 */
	if (!bootm_free(va, endva - va))
		uvm_km_free(kernel_map, va, endva - va);

	/*
	 * Mark as free in the extent map.
	 */
	if (extent_free(iomem_ex, bpa, size,
			EX_NOWAIT | (iomem_malloc_safe ? EX_MALLOCOK : 0))) {
		printf("bus_space_unmap: pa 0x%lx, size 0x%lx\n", bpa, size);
		printf("bus_space_unmap: can't free region\n");
	}
}

/*
 * Get a new handle for a subregion of an already-mapped area of bus space.
 */
int bus_space_subregion(t, memh, off, sz, mhp)
bus_space_tag_t		t;
bus_space_handle_t	memh;
bus_size_t		off, sz;
bus_space_handle_t	*mhp;
{
	*mhp = memh + off;
	return 0;
}

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
bus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	struct atari_bus_dmamap *map;
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
	mapsize = sizeof(struct atari_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	bzero(mapstore, mapsize);
	map = (struct atari_bus_dmamap *)mapstore;
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
bus_dmamap_destroy(t, map)
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
bus_dmamap_load(t, map, buf, buflen, p, flags)
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
bus_dmamap_load_mbuf(t, map, m0, flags)
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
bus_dmamap_load_uio(t, map, uio, flags)
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
bus_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{

	panic("bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * bus-specific DMA map unload functions.
 */
void
bus_dmamap_unload(t, map)
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
bus_dmamap_sync(t, map, off, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t off;
	bus_size_t len;
	int ops;
{
#if defined(M68040) || defined(M68060)
	int	i, pa_off, inc, seglen;
	u_long	pa, end_pa;

	if (t == BUS_PCI_DMA_TAG)
		pa_off = 0x80000000; /* XXX */
	else pa_off = 0;

	/* Flush granularity */
	inc = (len > 1024) ? NBPG : 16;

	for (i = 0; i < map->dm_nsegs && len > 0; i++) {
		if (map->dm_segs[i].ds_len <= off) {
			/* Segment irrelevant - before requested offset */
			off -= map->dm_segs[i].ds_len;
			continue;
		}
		seglen = map->dm_segs[i].ds_len - off;
		if (seglen > len)
			seglen = len;
		len -= seglen;
		pa = map->dm_segs[i].ds_addr + off - pa_off;
		end_pa = pa + seglen;

		if (inc == 16) {
			pa &= ~15;
			while (pa < end_pa) {
				DCFL(pa);
				pa += 16;
			}
		} else {
			pa &= ~PGOFSET;
			while (pa < end_pa) {
				DCFP(pa);
				pa += NBPG;
			}
		}
	}
#endif
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{

	return (_bus_dmamem_alloc_range(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags, 0, trunc_page(avail_end)));
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
bus_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	vm_page_t m;
	bus_addr_t addr, offset;
	struct pglist mlist;
	int curseg;

	if (t == BUS_PCI_DMA_TAG)
		offset = 0x80000000; /* XXX */
	else offset = 0;

	/*
	 * Build a list of pages to free back to the VM system.
	 */
	TAILQ_INIT(&mlist);
	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += PAGE_SIZE) {
			m = PHYS_TO_VM_PAGE(addr - offset);
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
bus_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	vaddr_t va;
	bus_addr_t addr, offset;
	int curseg;

	if (t == BUS_PCI_DMA_TAG)
		offset = 0x80000000; /* XXX */
	else offset = 0;

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
			pmap_enter(pmap_kernel(), va, addr - offset,
			    VM_PROT_READ | VM_PROT_WRITE,
			    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);
		}
	}

	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
bus_dmamem_unmap(t, kva, size)
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
int
bus_dmamem_mmap(t, segs, nsegs, off, prot, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs, off, prot, flags;
{
	int i, offset;

	if (t == BUS_PCI_DMA_TAG)
		offset = 0x80000000; /* XXX */
	else offset = 0;

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

		return (m68k_btop((caddr_t)segs[i].ds_addr - offset + off));
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
static int
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
	bus_addr_t curaddr, lastaddr, offset, baddr, bmask;
	vaddr_t vaddr = (vaddr_t)buf;
	int seg;
	pmap_t pmap;

	if (t == BUS_PCI_DMA_TAG)
		offset = 0x80000000; /* XXX */
	else
		offset = 0;

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	lastaddr = *lastaddrp;
	bmask = ~(map->_dm_boundary - 1);

	for (seg = *segp; buflen > 0 ; ) {
		/*
		 * Get the physical address for this segment.
		 */
		(void) pmap_extract(pmap, vaddr, &curaddr);

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
			map->dm_segs[seg].ds_addr = curaddr + offset;
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
				map->dm_segs[seg].ds_addr = curaddr + offset;
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
static int
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
	bus_addr_t offset;
	vm_page_t m;
	struct pglist mlist;
	int curseg, error;

	if (t == BUS_PCI_DMA_TAG)
		offset = 0x80000000; /* XXX */
	else offset = 0;

	/* Always round the size. */
	size = round_page(size);

	/*
	 * Allocate pages from the VM system.
	 */
	TAILQ_INIT(&mlist);
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
	segs[curseg].ds_addr = lastaddr + offset;
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
			segs[curseg].ds_addr = curaddr + offset;
			segs[curseg].ds_len = PAGE_SIZE;
		}
		lastaddr = curaddr;
	}

	*rsegs = curseg + 1;

	return (0);
}
