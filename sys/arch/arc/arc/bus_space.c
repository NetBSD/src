/*	$NetBSD: bus_space.c,v 1.4 2001/09/04 06:57:26 thorpej Exp $	*/
/*	NetBSD: bus_machdep.c,v 1.1 2000/01/26 18:48:00 drochner Exp 	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

void
arc_bus_space_init(bst, name, paddr, vaddr, start, size)
	bus_space_tag_t bst;
	const char *name;
	paddr_t paddr;
	vaddr_t vaddr;
	bus_addr_t start;
	bus_size_t size;
{
	bst->bs_name = name;
	bst->bs_extent = NULL;
	bst->bs_start = start;
	bst->bs_size = size;
	bst->bs_pbase = paddr;
	bst->bs_vbase = vaddr;
	bst->bs_compose_handle = arc_bus_space_compose_handle;
	bst->bs_dispose_handle = arc_bus_space_dispose_handle;
	bst->bs_paddr = arc_bus_space_paddr;
	bst->bs_map = arc_bus_space_map;
	bst->bs_unmap = arc_bus_space_unmap;
	bst->bs_subregion = arc_bus_space_subregion;
	bst->bs_mmap = arc_bus_space_mmap;
	bst->bs_alloc = arc_bus_space_alloc;
	bst->bs_free = arc_bus_space_free;
	bst->bs_aux = NULL;
	bst->bs_stride_1 = 0;
	bst->bs_stride_2 = 0;
	bst->bs_stride_4 = 0;
	bst->bs_stride_8 = 0;
}

void
arc_bus_space_init_extent(bst, storage, storagesize)
	bus_space_tag_t bst;
	caddr_t storage;
	size_t storagesize;
{
	bst->bs_extent = extent_create(bst->bs_name,
	    bst->bs_start, bst->bs_start + bst->bs_size, M_DEVBUF,
	    storage, storagesize, EX_NOWAIT);
	if (bst->bs_extent == NULL)
		panic("arc_bus_space_init_extent: cannot create extent map %s",
		    bst->bs_name);
}

void
arc_bus_space_set_aligned_stride(bst, alignment_shift)
	bus_space_tag_t bst;
	unsigned int alignment_shift;		/* log2(alignment) */
{
	bst->bs_stride_1 = alignment_shift;
	if (alignment_shift > 0)
		--alignment_shift;
	bst->bs_stride_2 = alignment_shift;
	if (alignment_shift > 0)
		--alignment_shift;
	bst->bs_stride_4 = alignment_shift;
	if (alignment_shift > 0)
		--alignment_shift;
	bst->bs_stride_8 = alignment_shift;
}

static int malloc_safe = 0;

void
arc_bus_space_malloc_set_safe()
{
	malloc_safe = EX_MALLOCOK;
}

int
arc_bus_space_extent_malloc_flag()
{
	return (malloc_safe);
}

int
arc_bus_space_compose_handle(bst, addr, size, flags, bshp)
	bus_space_tag_t bst;
	bus_addr_t addr;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	bus_space_handle_t bsh = bst->bs_vbase + (addr - bst->bs_start);

	/*
	 * Since all buses can be linearly mappable, we don't have to check
	 * BUS_SPACE_MAP_LINEAR and BUS_SPACE_MAP_PREFETCHABLE.
	 */
	if ((flags & BUS_SPACE_MAP_CACHEABLE) == 0) {
		*bshp = bsh;
		return (0);
	}
	if (bsh < MIPS_KSEG1_START) /* KUSEG or KSEG0 */
		panic("arc_bus_space_compose_handle: bad address 0x%x", bsh);
	if (bsh < MIPS_KSEG2_START) { /* KSEG1 */
		*bshp = MIPS_PHYS_TO_KSEG0(MIPS_KSEG1_TO_PHYS(bsh));
		return (0);
	}
	/*
	 * KSEG2:
	 * Do not make the page cacheable in this case, since:
	 * - the page which this bus_space belongs might include
	 *   other bus_spaces.
	 * or
	 * - this bus might be mapped by wired TLB, in that case,
	 *   we cannot manupulate cacheable attribute with page granularity.
	 */
#ifdef DIAGNOSTIC
	printf("arc_bus_space_compose_handle: ignore cacheable 0x%x\n", bsh);
#endif
	*bshp = bsh;
	return (0);
}

int
arc_bus_space_dispose_handle(bst, bsh, size)
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	return (0);
}

int
arc_bus_space_paddr(bst, bsh, pap)
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	paddr_t *pap;
{
	if (bsh < MIPS_KSEG0_START) /* KUSEG */
		panic("arc_bus_space_paddr(0x%qx): bad address",
		    (unsigned long long) bsh);
	else if (bsh < MIPS_KSEG1_START) /* KSEG0 */
		*pap = MIPS_KSEG0_TO_PHYS(bsh);
	else if (bsh < MIPS_KSEG2_START) /* KSEG1 */
		*pap = MIPS_KSEG1_TO_PHYS(bsh);
	else { /* KSEG2 */
		/*
		 * Since this region may be mapped by wired TLB,
		 * kvtophys() is not always available.
		 */
		*pap = bst->bs_pbase + (bsh - bst->bs_vbase);
	}
	return (0);
}

int
arc_bus_space_map(bst, addr, size, flags, bshp)
	bus_space_tag_t bst;
	bus_addr_t addr;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	int err;

	if (addr < bst->bs_start || addr + size > bst->bs_start + bst->bs_size)
		return (EINVAL);

	if (bst->bs_extent != NULL) {
		err = extent_alloc_region(bst->bs_extent, addr, size,
		    EX_NOWAIT | malloc_safe);
		if (err)
			return (err);
	}

	return (bus_space_compose_handle(bst, addr, size, flags, bshp));
}

void
arc_bus_space_unmap(bst, bsh, size)
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	if (bst->bs_extent != NULL) {
		paddr_t pa;
		bus_addr_t addr;
		int err;

		/* bus_space_paddr() becomes unavailable after unmapping */
		err = bus_space_paddr(bst, bsh, &pa);
		if (err)
			panic("arc_bus_space_unmap: %s va 0x%qx: error %d\n",
			    bst->bs_name, (unsigned long long) bsh, err);
		addr = (bus_size_t)(pa - bst->bs_pbase) + bst->bs_start;
		extent_free(bst->bs_extent, addr, size,
		    EX_NOWAIT | malloc_safe);
	}
	bus_space_dispose_handle(bst, bsh, size);
}

int
arc_bus_space_subregion(bst, bsh, offset, size, nbshp)
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t offset;
	bus_size_t size;
	bus_space_handle_t *nbshp;
{
	*nbshp = bsh + offset;
	return (0);
}

paddr_t
arc_bus_space_mmap(bst, addr, off, prot, flags)
	bus_space_tag_t bst;
	bus_addr_t addr;
	off_t off;
	int prot;
	int flags;
{

	/*
	 * XXX We do not disallow mmap'ing of EISA/PCI I/O space here,
	 * XXX which we should be doing.
	 */

	if (addr < bst->bs_start ||
	    (addr + off) >= (bst->bs_start + bst->bs_size))
		return (-1);

	return (mips_btop(bst->bs_pbase + (addr - bst->bs_start) + off));
}

int
arc_bus_space_alloc(bst, start, end, size, align, boundary, flags, addrp, bshp)
	bus_space_tag_t bst;
	bus_addr_t start;
	bus_addr_t end;
	bus_size_t size;
	bus_size_t align;
	bus_size_t boundary;
	int flags;
	bus_addr_t *addrp;
	bus_space_handle_t *bshp;
{
	u_long addr;
	int err;

	if (bst->bs_extent == NULL)
		panic("arc_bus_space_alloc: extent map %s not available",
		    bst->bs_name);

	if (start < bst->bs_start ||
	    start + size > bst->bs_start + bst->bs_size)
		return (EINVAL);

	err = extent_alloc_subregion(bst->bs_extent, start, end, size,
	    align, boundary, EX_FAST | EX_NOWAIT | malloc_safe, &addr);
	if (err)
		return (err);

	*addrp = addr;
	return (bus_space_compose_handle(bst, addr, size, flags, bshp));
}
