/*	$NetBSD: bus_space_large.c,v 1.3 2003/07/15 00:04:40 lukem Exp $	*/

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

/*
 * For large sparse bus space
 * (e.g. NEC RISCstation 2250 PCI memory space: 432MB)
 *
 * This bus_space uses either wired TLB or normal KSEG2 mapping
 * for each bus space region.
 *
 * Wired TLB will be used on the following cases:
 * - If requested region is already mapped by wired TLBs.
 *	This is needed for some critical resources which are used
 *	on kernel early stage (e.g. console device resources)
 * - If requested region size >= ARC_THRESHOLD_TO_USE_WIRED_TLB,
 *   and enough wired TLBs are still free.
 *	In this case, the size of wired TLBs becomes always
 *	ARC_WIRED_PAGE_SIZE (i.e. 16MB). (See wired_map.c for detail.)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_space_large.c,v 1.3 2003/07/15 00:04:40 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <arc/arc/wired_map.h>

static int arc_large_bus_space_compose_handle __P((bus_space_tag_t,
	    bus_addr_t, bus_size_t, int, bus_space_handle_t *));
static int arc_large_bus_space_dispose_handle __P((bus_space_tag_t,
	    bus_space_handle_t, bus_size_t));
static int arc_large_bus_space_paddr __P((bus_space_tag_t,
	    bus_space_handle_t, paddr_t *));

void
arc_large_bus_space_init(bst, name, paddr, start, size)
	bus_space_tag_t bst;
	const char *name;
	paddr_t paddr;
	bus_addr_t start;
	bus_size_t size;
{
	arc_sparse_bus_space_init(bst, name, paddr, start, size);
	bst->bs_compose_handle = arc_large_bus_space_compose_handle;
	bst->bs_dispose_handle = arc_large_bus_space_dispose_handle;
	bst->bs_paddr = arc_large_bus_space_paddr;
}

static int
arc_large_bus_space_compose_handle(bst, addr, size, flags, bshp)
	bus_space_tag_t bst;
	bus_addr_t addr;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	paddr_t pa;
	vaddr_t va;

	pa = bst->bs_pbase + (addr - bst->bs_start);
	/*
	 * Check whether the physical address is already wired mapped
	 * or not.  If not mapped but requested region is large enough,
	 * try to use wired TLB.
	 */
	if ((va = arc_contiguously_wired_mapped(pa, size)) != 0 ||
	    (/* requested region is large enough? */
	     size >= ARC_THRESHOLD_TO_USE_WIRED_TLB &&
	     /* enough wired TLBs are still available? */
	     (va = arc_map_wired(pa, size)) != 0)) {
#ifdef DIAGNOSTIC
		/*
		 * If wired TLB is used,
		 * we will not make this bus_space region cacheable,
		 * since other bus_space might share this wired TLB.
		 */
		if (flags & BUS_SPACE_MAP_CACHEABLE)
			printf("arc_large_bus_space_compose_handle: "
			    "ignore cacheable 0x%llx..0x%llx\n",
			    pa, pa + size - 1);
#endif
		*bshp = va;
		return (0);
	}
	return (arc_sparse_bus_space_compose_handle(bst, addr, size, flags,
	    bshp));
}

static int
arc_large_bus_space_dispose_handle(bst, bsh, size)
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	paddr_t pa;

	/*
	 * We never free already wired TLB entries,
	 * since the TLBs might be used for other bus_space region.
	 */
	if (arc_wired_map_extract(bsh, &pa))
		return (0);
	return (arc_sparse_bus_space_dispose_handle(bst, bsh, size));
}

static int
arc_large_bus_space_paddr(bst, bsh, pap)
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	paddr_t *pap;
{
	if (arc_wired_map_extract(bsh, pap))
		return (0);
	return (arc_sparse_bus_space_paddr(bst, bsh, pap));
}
