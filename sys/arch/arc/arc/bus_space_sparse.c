/*	$NetBSD: bus_space_sparse.c,v 1.18.12.1 2017/12/03 11:35:49 jdolecek Exp $	*/
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
 * For sparse bus space
 *
 * This bus_space uses KSEG2 mapping, if the physical address is not
 * accessible via KSEG0/KSEG1.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_space_sparse.c,v 1.18.12.1 2017/12/03 11:35:49 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <mips/locore.h>
#include <mips/pte.h>

extern paddr_t kvtophys(vaddr_t);	/* XXX */

void
arc_sparse_bus_space_init(bus_space_tag_t bst, const char *name, paddr_t paddr,
    bus_addr_t start, bus_size_t size)
{

	arc_bus_space_init(bst, name, paddr, ARC_BUS_SPACE_UNMAPPED,
	    start, size);
	bst->bs_compose_handle = arc_sparse_bus_space_compose_handle;
	bst->bs_dispose_handle = arc_sparse_bus_space_dispose_handle;
	bst->bs_paddr = arc_sparse_bus_space_paddr;
}

int
arc_sparse_bus_space_compose_handle(bus_space_tag_t bst, bus_addr_t addr,
    bus_size_t size, int flags, bus_space_handle_t *bshp)
{
	bus_size_t offset = addr - bst->bs_start;
	/*
	 * Since all buses can be linearly mappable, we don't have to check
	 * BUS_SPACE_MAP_LINEAR and BUS_SPACE_MAP_PREFETCHABLE.
	 */
	const bool cacheable = (flags & BUS_SPACE_MAP_CACHEABLE) != 0;
	const u_int pmap_flags = cacheable ? PMAP_WRITE_BACK : 0;

	/*
	 * XXX - `bst->bs_pbase' must be page aligned,
	 * mips_trunc/round_page() cannot treat paddr_t due to overflow.
	 */
	paddr_t start = bst->bs_pbase + mips_trunc_page(offset);
	paddr_t end = bst->bs_pbase + mips_round_page(offset + size);

	if (end <= MIPS_KSEG1_START - MIPS_KSEG0_START) {
		/* mappable on KSEG0 or KSEG1 */
		*bshp = (cacheable ?
		    MIPS_PHYS_TO_KSEG0(start) :
		    MIPS_PHYS_TO_KSEG1(start));
	} else {
		vaddr_t vaddr = uvm_km_alloc(kernel_map, (vsize_t)(end - start),
		    0, UVM_KMF_VAONLY | UVM_KMF_NOWAIT);

		if (vaddr == 0)
			panic("arc_sparse_bus_space_compose_handle: "
			      "cannot allocate KVA 0x%llx..0x%llx",
			      start, end);
		for (vaddr_t va = vaddr; start < end;
		     start += PAGE_SIZE, va += PAGE_SIZE) {
			pmap_kenter_pa(va, start, VM_PROT_READ|VM_PROT_WRITE,
			    pmap_flags);
		}
		pmap_update(pmap_kernel());
		vaddr += (offset & PGOFSET);
		*bshp = vaddr;
	}
	return 0;
}

int
arc_sparse_bus_space_dispose_handle(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t size)
{
	vaddr_t start = mips_trunc_page(bsh);
	vaddr_t end = mips_round_page(bsh + size);

	if (start < MIPS_KSEG2_START) /* KSEG0/KSEG1 */
		return 0;

	pmap_kremove(start, end - start);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, start, end - start, UVM_KMF_VAONLY);
	return 0;
}

int
arc_sparse_bus_space_paddr(bus_space_tag_t bst, bus_space_handle_t bsh,
    paddr_t *pap)
{

	*pap = kvtophys(bsh);
	return 0;
}
