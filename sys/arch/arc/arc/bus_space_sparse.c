/*	$NetBSD: bus_space_sparse.c,v 1.2 2000/06/26 14:20:32 mrg Exp $	*/
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

/*
 * For sparse bus space
 *
 * This bus_space uses KSEG2 mapping, if the physical address is not
 * accessible via KSEG0/KSEG1.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <vm/vm.h>

#include <uvm/uvm_extern.h>

#include <mips/cpuregs.h>
#include <mips/pte.h>

#include <machine/bus.h>

extern paddr_t kvtophys __P((vaddr_t));	/* XXX */

void	arc_kseg2_make_cacheable __P((vaddr_t vaddr, vsize_t size));

void
arc_kseg2_make_cacheable(vaddr, size)
	vaddr_t vaddr;
	vsize_t size;
{
	vaddr_t start, end;
	pt_entry_t *pte;
	u_int32_t entry, mask;

	start = mips_trunc_page(vaddr);
	end = mips_round_page(vaddr + size);
	mask = ~(CPUISMIPS3 ? MIPS3_PG_UNCACHED : MIPS1_PG_N);
	for (; start < end; start += NBPG) {
		pte = kvtopte(start);
		entry = pte->pt_entry & mask;
		pte->pt_entry &= entry;
		MachTLBUpdate(start, entry);
	}
}

void
arc_sparse_bus_space_init(bst, name, paddr, start, size)
	bus_space_tag_t bst;
	const char *name;
	paddr_t paddr;
	bus_addr_t start;
	bus_size_t size;
{
	arc_bus_space_init(bst, name, paddr, ARC_BUS_SPACE_UNMAPPED,
	    start, size);
	bst->bs_compose_handle = arc_sparse_bus_space_compose_handle;
	bst->bs_dispose_handle = arc_sparse_bus_space_dispose_handle;
	bst->bs_paddr = arc_sparse_bus_space_paddr;
}

int
arc_sparse_bus_space_compose_handle(bst, addr, size, flags, bshp)
	bus_space_tag_t bst;
	bus_addr_t addr;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	bus_size_t offset = addr - bst->bs_start;
	/*
	 * Since all buses can be linearly mappable, we don't have to check
	 * BUS_SPACE_MAP_LINEAR and BUS_SPACE_MAP_PREFETCHABLE.
	 */
	int cacheable = (flags & BUS_SPACE_MAP_CACHEABLE);

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
		vaddr_t va,
		    vaddr = uvm_km_valloc(kernel_map, (vsize_t)(end - start));

		if (vaddr == NULL)
			panic("arc_bus_space_compose_handle: "
			      "cannot allocate KVA 0x%llx..0x%llx",
			      start, end);
		for (va = vaddr; start < end; start += NBPG, va += NBPG)
			pmap_kenter_pa(va, start, VM_PROT_READ|VM_PROT_WRITE);
		vaddr += (offset & PGOFSET);
		if (cacheable)
			arc_kseg2_make_cacheable(vaddr, size);
		*bshp = vaddr;
	}
	return (0);
}

int
arc_sparse_bus_space_dispose_handle(bst, bsh, size)
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	vaddr_t start = mips_trunc_page(bsh);
	vaddr_t end = mips_round_page(bsh + size);

	if (start < MIPS_KSEG2_START) /* KSEG0/KSEG1 */
		return (0);

	uvm_km_free(kernel_map, start, end - start);
	return (0);
}

int
arc_sparse_bus_space_paddr(bst, bsh, pap)
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	paddr_t *pap;
{
	*pap = kvtophys(bsh);
	return (0);
}
