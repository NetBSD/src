/*	$NetBSD: bus.c,v 1.16 2006/09/30 13:54:53 tsutsui Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: Utah Hdr: machdep.c 1.74 92/12/20
 *	from: @(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

/*
 * Copyright (c) 2001 Matthew Fredette.
 * Copyright (c) 1994, 1995 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: Utah Hdr: machdep.c 1.74 92/12/20
 *	from: @(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)machdep.c	8.6 (Berkeley) 1/14/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus.c,v 1.16 2006/09/30 13:54:53 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <uvm/uvm.h> /* XXX: not _extern ... need vm_map_create */ 
 
#include <machine/pte.h>
#define _SUN68K_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pmap.h>

#include <sun68k/sun68k/control.h>

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int 
_bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct sun68k_bus_dmamap *map;
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
	mapsize = sizeof(struct sun68k_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	memset(mapstore, 0, mapsize);
	map = (struct sun68k_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxmaxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_align = PAGE_SIZE;
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

	/*
	 * If the handle contains a valid mapping, unload it.
	 */
	if (map->dm_mapsize != 0)
		bus_dmamap_unload(t, map);

	free(map, M_DMAMAP);
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
	vaddr_t low, high;
	struct pglist *mlist;
	int error;
extern	paddr_t avail_start;
extern	paddr_t avail_end;

	/* Always round the size. */
	size = m68k_round_page(size);
	low = avail_start;
	high = avail_end;

	if ((mlist = malloc(sizeof(*mlist), M_DEVBUF,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	/*
	 * Allocate physical pages from the VM system.
	 */
	error = uvm_pglistalloc(size, low, high, 0, 0,
				mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
	if (error)
		return (error);

	/*
	 * Simply keep a pointer around to the linked list, so
	 * bus_dmamap_free() can return it.
	 *
	 * NOBODY SHOULD TOUCH THE pageq FIELDS WHILE THESE PAGES
	 * ARE IN OUR CUSTODY.
	 */
	segs[0]._ds_mlist = mlist;

	/*
	 * We now have physical pages, but no DVMA addresses yet. These
	 * will be allocated in bus_dmamap_load*() routines. Hence we
	 * save any alignment and boundary requirements in this DMA
	 * segment.
	 */
	segs[0].ds_addr = 0;
	segs[0].ds_len = 0;
	segs[0]._ds_va = 0;
	*rsegs = 1;
	return (0);
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void 
_bus_dmamem_free(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs)
{

	if (nsegs != 1)
		panic("bus_dmamem_free: nsegs = %d", nsegs);

	/*
	 * Return the list of physical pages back to the VM system.
	 */
	uvm_pglistfree(segs[0]._ds_mlist);
	free(segs[0]._ds_mlist, M_DEVBUF);
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int 
_bus_dmamem_map(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
    size_t size, caddr_t *kvap, int flags)
{
	struct vm_page *m;
	vaddr_t va;
	struct pglist *mlist;
	const uvm_flag_t kmflags =
	    (flags & BUS_DMA_NOWAIT) != 0 ? UVM_KMF_NOWAIT : 0;

	if (nsegs != 1)
		panic("_bus_dmamem_map: nsegs = %d", nsegs);

	size = m68k_round_page(size);

	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY | kmflags);
	if (va == 0)
		return (ENOMEM);

	segs[0]._ds_va = va;
	*kvap = (caddr_t)va;

	mlist = segs[0]._ds_mlist;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {
		paddr_t pa;

		if (size == 0)
			panic("_bus_dmamem_map: size botch");

		pa = VM_PAGE_TO_PHYS(m);
		pmap_enter(pmap_kernel(), va, pa | PMAP_NC,
			   VM_PROT_READ | VM_PROT_WRITE,
			   VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);

		va += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	pmap_update(pmap_kernel());

	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void 
_bus_dmamem_unmap(bus_dma_tag_t t, caddr_t kva, size_t size)
{

#ifdef DIAGNOSTIC
	if ((u_long)kva & PAGE_MASK)
		panic("_bus_dmamem_unmap");
#endif

	size = m68k_round_page(size);
	uvm_unmap(kernel_map, (vaddr_t)kva, (vaddr_t)kva + size);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t 
_bus_dmamem_mmap(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs, off_t off,
    int prot, int flags)
{

	panic("_bus_dmamem_mmap: not implemented");
}

/*
 * Utility to allocate an aligned kernel virtual address range
 */
vaddr_t 
_bus_dma_valloc_skewed(size_t size, u_long boundary, u_long align, u_long skew)
{
	vaddr_t va;

	/*
	 * Find a region of kernel virtual addresses that is aligned
	 * to the given address modulo the requested alignment, i.e.
	 *
	 *	(va - skew) == 0 mod align
	 *
	 * The following conditions apply to the arguments:
	 *
	 *	- `size' must be a multiple of the VM page size
	 *	- `align' must be a power of two
	 *	   and greater than or equal to the VM page size
	 *	- `skew' must be smaller than `align'
	 *	- `size' must be smaller than `boundary'
	 */

#ifdef DIAGNOSTIC
	if ((size & PAGE_MASK) != 0)
		panic("_bus_dma_valloc_skewed: invalid size %lx", (unsigned long) size);
	if ((align & PAGE_MASK) != 0)
		panic("_bus_dma_valloc_skewed: invalid alignment %lx", align);
	if (align < skew)
		panic("_bus_dma_valloc_skewed: align %lx < skew %lx",
			align, skew);
#endif

	/* XXX - Implement this! */
	if (boundary || skew)
		panic("_bus_dma_valloc_skewed: not implemented");

	/*
	 * First, find a region large enough to contain any aligned chunk
	 */
	va = uvm_km_alloc(kernel_map, size, align, UVM_KMF_VAONLY);
	if (va == 0)
		return (ENOMEM);

	return (va);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int 
_bus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map, struct mbuf *m,
    int flags)
{

	panic("_bus_dmamap_load_mbuf: not implemented");
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int 
_bus_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio,
    int flags)
{

	panic("_bus_dmamap_load_uio: not implemented");
}

/*
 * Common function for DMA map synchronization.  May be called
 * by bus-specific DMA map synchronization functions.
 */
void 
_bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
}

struct sun68k_bus_dma_tag mainbus_dma_tag = {
	NULL,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,

	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};


/*
 * Base bus space handlers.
 */
int		sun68k_find_prom_map(bus_addr_t, bus_type_t, int,
		    bus_space_handle_t *);
static int	sun68k_bus_map(bus_space_tag_t, bus_type_t, bus_addr_t,
		    bus_size_t, int, vaddr_t, bus_space_handle_t *);
static int	sun68k_bus_unmap(bus_space_tag_t, bus_space_handle_t,
		    bus_size_t);
static int	sun68k_bus_subregion(bus_space_tag_t, bus_space_handle_t,
		    bus_size_t, bus_size_t, bus_space_handle_t *);
static paddr_t	sun68k_bus_mmap(bus_space_tag_t, bus_type_t, bus_addr_t, off_t,
		    int, int);
static void	*sun68k_mainbus_intr_establish(bus_space_tag_t, int, int, int,
		    int (*)(void *), void *);
static void	sun68k_bus_barrier(bus_space_tag_t, bus_space_handle_t,
		    bus_size_t, bus_size_t, int);
static int	sun68k_bus_peek(bus_space_tag_t, bus_space_handle_t,
		    bus_size_t, size_t, void *);
static int	sun68k_bus_poke(bus_space_tag_t, bus_space_handle_t,
		    bus_size_t, size_t, uint32_t);

/*
 * If we can find a mapping that was established by the PROM, use it.
 */
int
sun68k_find_prom_map(bus_addr_t pa, bus_type_t iospace, int len,
    bus_space_handle_t *hp)
{
	u_long	pf;
	int	pgtype;
	u_long	va, eva;
	int	sme;
	u_long	pte;
	int	saved_ctx;

	/*
	 * The mapping must fit entirely within one page.
	 */
	if ((((u_long)pa & PGOFSET) + len) > PAGE_SIZE)
		return (EINVAL);

	pf = PA_PGNUM(pa);
	pgtype = iospace << PG_MOD_SHIFT;
	saved_ctx = kernel_context();

	/*
	 * Walk the PROM address space, looking for a page with the
	 * mapping we want.
	 */
	for (va = SUN_MONSTART; va < SUN_MONEND; ) {

		/*
		 * Make sure this segment is mapped.
		 */
		sme = get_segmap(va);
		if (sme == SEGINV) {
			va += NBSG;
			continue;			/* next segment */
		}

		/*
		 * Walk the pages of this segment.
		 */
		for(eva = va + NBSG; va < eva; va += PAGE_SIZE) {
			pte = get_pte(va);

			if ((pte & (PG_VALID | PG_TYPE)) ==
				(PG_VALID | pgtype) &&
			    PG_PFNUM(pte) == pf)
			{
				/* 
				 * Found the PROM mapping.
				 * note: preserve page offset
				 */
				*hp = (bus_space_handle_t)(va | ((u_long)pa & PGOFSET));
				restore_context(saved_ctx);
				return (0);
			}
		}
	}
	restore_context(saved_ctx);
	return (ENOENT);
}

int
sun68k_bus_map(bus_space_tag_t t, bus_type_t iospace, bus_addr_t addr,
    bus_size_t size, int flags, vaddr_t vaddr, bus_space_handle_t *hp)
{
	bus_size_t	offset;
	vaddr_t v;

	/*
	 * If we suspect there might be one, try to find
	 * and use a PROM mapping.
	 */
	if ((flags & _SUN68K_BUS_MAP_USE_PROM) != 0 &&
	     sun68k_find_prom_map(addr, iospace, size, hp) == 0)
		return (0);

	/*
	 * Adjust the user's request to be page-aligned.
	 */
	offset = addr & PGOFSET;
	addr -= offset;
	size += offset;
	size = m68k_round_page(size);
	if (size == 0) {
		printf("sun68k_bus_map: zero size\n");
		return (EINVAL);
	}

	/* Get some kernel virtual address space. */
	if (vaddr)
		v = vaddr;
	else
		v = uvm_km_alloc(kernel_map, size, 0,
		    UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	if (v == 0)
		panic("sun68k_bus_map: no memory");

	/* note: preserve page offset */
	*hp = (bus_space_handle_t)(v | offset);

	/*
	 * Map the device.  
	 */
	addr |= iospace | PMAP_NC;
	pmap_map(v, addr, addr + size, VM_PROT_ALL);

	return (0);
}

int 
sun68k_bus_unmap(bus_space_tag_t t, bus_space_handle_t bh, bus_size_t size)
{
	bus_size_t	offset;
	vaddr_t va = (vaddr_t)bh;

	/*
	 * Adjust the user's request to be page-aligned.
	 */
	offset = va & PGOFSET;
	va -= offset;
	size += offset;
	size = m68k_round_page(size);
	if (size == 0) {
		printf("sun68k_bus_unmap: zero size\n");
		return (EINVAL);
	}

	/*
	 * If any part of the request is in the PROM's address space,
	 * don't unmap it.
	 */
#ifdef	DIAGNOSTIC
	if ((va >= SUN_MONSTART && va < SUN_MONEND) !=
	    ((va + size) >= SUN_MONSTART && (va + size) < SUN_MONEND))
		panic("sun_bus_unmap: bad PROM mapping");
#endif
	if (va >= SUN_MONSTART && va < SUN_MONEND)
		return (0);

	pmap_remove(pmap_kernel(), va, va + size);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, va, size, UVM_KMF_VAONLY);
	return (0);
}

int 
sun68k_bus_subregion(bus_space_tag_t tag, bus_space_handle_t handle,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nhandlep)
{

	*nhandlep = handle + offset;
	return (0);
}

paddr_t
sun68k_bus_mmap(bus_space_tag_t t, bus_type_t iospace, bus_addr_t paddr,
    off_t offset, int prot, int flags)
{
	paddr_t npaddr;

	npaddr = m68k_trunc_page(paddr + offset);
	return (npaddr | (paddr_t)iospace | PMAP_NC);
}

/*
 * These assist in device probes.
 */

extern label_t *nofault;

int 
sun68k_bus_peek(bus_space_tag_t tag, bus_space_handle_t handle,
    bus_size_t offset, size_t size, void *vp)
{
	int result;
	label_t	faultbuf;
	uint32_t junk;
	
	if (vp == NULL)
		vp = &junk;

	nofault = &faultbuf; 
	if (setjmp(&faultbuf))
		result = -1;
	else {
		switch(size) {
		case 1:
			*((uint8_t *)vp) =
			    bus_space_read_1(tag, handle, offset);
			break;
		case 2:
			*((uint16_t *)vp) =
			    bus_space_read_2(tag, handle, offset);
			break;
		case 4:
			*((uint32_t *)vp) =
			    bus_space_read_4(tag, handle, offset);
			break;
		default:
			panic("_bus_space_peek: bad size");
		}
		result = 0;
	}

	nofault = NULL;
	return (result);
}

int 
sun68k_bus_poke(bus_space_tag_t tag, bus_space_handle_t handle,
    bus_size_t offset, size_t size, uint32_t v)
{
	int result;
	label_t	faultbuf;
	
	nofault = &faultbuf; 
	if (setjmp(&faultbuf))
		result = -1;
	else {
		switch(size) {
		case 1:
			bus_space_write_1(tag, handle, offset, (uint8_t)v);
			break;
		case 2:
			bus_space_write_2(tag, handle, offset, (uint16_t)v);
			break;
		case 4:
			bus_space_write_4(tag, handle, offset, (uint32_t)v);
			break;
		default:
			panic("_bus_space_poke: bad size");
		}
		result = 0;
	}

	nofault = NULL;
	return (result);
}

void *
sun68k_mainbus_intr_establish(bus_space_tag_t t, int pil, int level, int flags,
    int (*handler)(void *), void *arg)
{

	isr_add_autovect(handler, arg, pil);
	return (NULL);
}

void
sun68k_bus_barrier(bus_space_tag_t t, bus_space_handle_t h, bus_size_t offset,
    bus_size_t size, int flags)
{

	/* No default barrier action defined */
	return;
}

struct sun68k_bus_space_tag mainbus_space_tag = {
	NULL,				/* cookie */
	NULL,				/* parent bus tag */
	sun68k_bus_map,			/* bus_space_map */
	sun68k_bus_unmap,		/* bus_space_unmap */
	sun68k_bus_subregion,		/* bus_space_subregion */
	sun68k_bus_barrier,		/* bus_space_barrier */
	sun68k_bus_mmap,		/* bus_space_mmap */
	sun68k_mainbus_intr_establish,	/* bus_intr_establish */
	sun68k_bus_peek,		/* bus_space_peek_N */
	sun68k_bus_poke			/* bus_space_poke_N */
};
