/*	$NetBSD: bus_space.c,v 1.10 2001/09/15 11:13:20 uch Exp $	*/

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*	$NetBSD: bus_space.c,v 1.10 2001/09/15 11:13:20 uch Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#include <mips/cpuregs.h>
#include <mips/pte.h>
#include <machine/bus.h>

#ifdef BUS_SPACE_DEBUG
#define	DPRINTF(arg) printf arg
#else
#define	DPRINTF(arg)
#endif

#define MAX_BUSSPACE_TAG 10

static  struct hpcmips_bus_space sys_bus_space[MAX_BUSSPACE_TAG];
static int bus_space_index = 0;
bus_space_handle_t __hpcmips_cacheable(bus_space_tag_t, bus_addr_t,
    bus_size_t, int);

bus_space_tag_t
hpcmips_alloc_bus_space_tag()
{
	bus_space_tag_t	t;

	if (bus_space_index >= MAX_BUSSPACE_TAG) {
		panic("hpcmips_internal_alloc_bus_space_tag: tag full.");
	}
	t = &sys_bus_space[bus_space_index++];

	return (t);
}

void
hpcmips_init_bus_space_extent(bus_space_tag_t t)
{
	u_int32_t pa, endpa;
	vaddr_t va;
	
	/* 
	 * If request physical address is greater than 512MByte,
	 * mapping it to kseg2.
	 */
	if (t->t_base >= 0x20000000) {
		pa = mips_trunc_page(t->t_base);
		endpa = mips_round_page(t->t_base + t->t_size);

		if (!(va = uvm_km_valloc(kernel_map, endpa - pa))) {
			panic("hpcmips_init_bus_space_extent:"
			    "can't allocate kernel virtual");
		}
		DPRINTF(("pa:0x%08x -> kv:0x%08x+0x%08x",
		    (unsigned int)t->t_base, (unsigned int)va, t->t_size));
		t->t_base = va; /* kseg2 addr */
				
		for (; pa < endpa; pa += NBPG, va += NBPG) {
			pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
		}
		pmap_update(pmap_kernel());
	}

	t->t_extent = (void*)extent_create(t->t_name, t->t_base, 
	    t->t_base + t->t_size, M_DEVBUF,
	    0, 0, EX_NOWAIT);
	if (!t->t_extent) {
		panic("hpcmips_init_bus_space_extent:"
		    "unable to allocate %s map", t->t_name);
	}
}

bus_space_handle_t
__hpcmips_cacheable(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size,
    int cacheable)
{
	vaddr_t va, endva;
	pt_entry_t *pte;
	u_int32_t opte, npte;

	MachFlushCache();
	if (t->t_base >= MIPS_KSEG2_START) {
		va = mips_trunc_page(bpa);
		endva = mips_round_page(bpa + size);
		npte = CPUISMIPS3 ? MIPS3_PG_UNCACHED : MIPS1_PG_N;
		
		for (; va < endva; va += NBPG) {
			pte = kvtopte(va);
			opte = pte->pt_entry;
			if (cacheable) {
				opte &= ~npte;
			} else {
				opte |= npte;
			}
			pte->pt_entry = opte;
			/*
			 * Update the same virtual address entry.
			 */
			MachTLBUpdate(va, opte);
		}
		return (bpa);
	}

	return (cacheable ? MIPS_PHYS_TO_KSEG0(bpa) : MIPS_PHYS_TO_KSEG1(bpa));
}

/* ARGSUSED */
int
bus_space_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp)
{
	int err;
	int cacheable = flags & BUS_SPACE_MAP_CACHEABLE;

	if (!t->t_extent) { /* Before autoconfiguration, can't use extent */
		DPRINTF(("bus_space_map: map temporary region:"
		    "0x%08x-0x%08x\n", bpa, bpa+size));
		bpa += t->t_base;
	} else {
		bpa += t->t_base;
		if ((err = extent_alloc_region(t->t_extent, bpa, size, 
		    EX_NOWAIT|EX_MALLOCOK))) {
			return (err);
		}
	}
	*bshp = __hpcmips_cacheable(t, bpa, size, cacheable);

	DPRINTF(("\tbus_space_map:%#x(%#x)+%#x\n",
	    bpa, bpa - t->t_base, size));

	return (0);
}

/* ARGSUSED */
int
bus_space_alloc(bus_space_tag_t t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	int cacheable = flags & BUS_SPACE_MAP_CACHEABLE;
	u_long bpa;
	int err;

	if (!t->t_extent)
		panic("bus_space_alloc: no extent");

	rstart += t->t_base;
	rend += t->t_base;
	if ((err = extent_alloc_subregion(t->t_extent, rstart, rend, size,
	    alignment, boundary, EX_FAST|EX_NOWAIT|EX_MALLOCOK, &bpa))) {
		return (err);
	}

	*bshp = __hpcmips_cacheable(t, bpa, size, cacheable);

	if (bpap) {
		*bpap = bpa;
	}

	DPRINTF(("\tbus_space_alloc:%#x(%#x)+%#x\n", (unsigned)bpa,
	    (unsigned)(bpa - t->t_base), size));

	return (0);
}

/* ARGSUSED */
void
bus_space_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	/* bus_space_unmap() does all that we need to do. */
	bus_space_unmap(t, bsh, size);
}

void
bus_space_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	int err;
	u_int32_t addr;

	if (!t->t_extent) {
		return; /* Before autoconfiguration, can't use extent */
	}

	if (t->t_base < MIPS_KSEG2_START) {
		addr = MIPS_KSEG1_TO_PHYS(bsh);
	} else {
		addr = bsh;
	}

	if ((err = extent_free(t->t_extent, addr, size, EX_NOWAIT))) {
		DPRINTF(("warning: %#x-%#x of %s space lost\n",
		    bsh, bsh+size, t->t_name));
	}
}

/* ARGSUSED */
int
bus_space_subregion(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;

	return (0);
}
