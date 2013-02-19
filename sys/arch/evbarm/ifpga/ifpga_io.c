/*	$NetBSD: ifpga_io.c,v 1.12 2013/02/19 10:57:10 skrll Exp $ */

/*
 * Copyright (c) 1997 Causality Limited
 * Copyright (c) 1997 Mark Brinicombe.
 * All rights reserved.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * From arm/footbridge/footbridge_io.c
 */

/*
 * bus_space I/O functions for IFPGA
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ifpga_io.c,v 1.12 2013/02/19 10:57:10 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <uvm/uvm_extern.h>

#include <evbarm/ifpga/ifpgavar.h>
#include <evbarm/ifpga/ifpgamem.h>

/* Proto types for all the bus_space structure functions */

bs_protos(ifpga);
bs_protos(generic);
bs_protos(generic_armv4);
bs_protos(bs_notimpl);
bs_map_proto(ifpga_mem);
bs_unmap_proto(ifpga_mem);

/* Declare the ifpga bus space tag */

struct bus_space ifpga_bs_tag = {
	/* cookie */
	(void *) 0,			/* Physical base address */

	/* mapping/unmapping */
	ifpga_bs_map,
	ifpga_bs_unmap,
	ifpga_bs_subregion,

	/* allocation/deallocation */
	ifpga_bs_alloc,
	ifpga_bs_free,

	/* get kernel virtual address */
	ifpga_bs_vaddr,

	/* mmap */
	bs_notimpl_bs_mmap,

	/* barrier */
	ifpga_bs_barrier,

	/* read (single) */
	generic_bs_r_1,
	generic_armv4_bs_r_2,
	generic_bs_r_4,
	bs_notimpl_bs_r_8,

	/* read multiple */
	generic_bs_rm_1,
	generic_armv4_bs_rm_2,
	generic_bs_rm_4,
	bs_notimpl_bs_rm_8,

	/* read region */
	bs_notimpl_bs_rr_1,
	generic_armv4_bs_rr_2,
	generic_bs_rr_4,
	bs_notimpl_bs_rr_8,

	/* write (single) */
	generic_bs_w_1,
	generic_armv4_bs_w_2,
	generic_bs_w_4,
	bs_notimpl_bs_w_8,

	/* write multiple */
	generic_bs_wm_1,
	generic_armv4_bs_wm_2,
	generic_bs_wm_4,
	bs_notimpl_bs_wm_8,

	/* write region */
	bs_notimpl_bs_wr_1,
	generic_armv4_bs_wr_2,
	generic_bs_wr_4,
	bs_notimpl_bs_wr_8,

	/* set multiple */
	bs_notimpl_bs_sm_1,
	bs_notimpl_bs_sm_2,
	bs_notimpl_bs_sm_4,
	bs_notimpl_bs_sm_8,

	/* set region */
	bs_notimpl_bs_sr_1,
	generic_armv4_bs_sr_2,
	bs_notimpl_bs_sr_4,
	bs_notimpl_bs_sr_8,

	/* copy */
	bs_notimpl_bs_c_1,
	generic_armv4_bs_c_2,
	bs_notimpl_bs_c_4,
	bs_notimpl_bs_c_8,
};

/* This is a preinitialized version of ifpga_bs_tag */

struct bus_space ifpga_common_bs_tag = {
	/* cookie */
	(void *) IFPGA_IO_BASE,		/* Physical base address */

	/* mapping/unmapping */
	ifpga_mem_bs_map,
	ifpga_mem_bs_unmap,
	ifpga_bs_subregion,

	/* allocation/deallocation */
	ifpga_bs_alloc,
	ifpga_bs_free,

	/* get kernel virtual address */
	ifpga_bs_vaddr,

	/* mmap */
	bs_notimpl_bs_mmap,

	/* barrier */
	ifpga_bs_barrier,

	/* read (single) */
	generic_bs_r_1,
	generic_armv4_bs_r_2,
	generic_bs_r_4,
	bs_notimpl_bs_r_8,

	/* read multiple */
	generic_bs_rm_1,
	generic_armv4_bs_rm_2,
	generic_bs_rm_4,
	bs_notimpl_bs_rm_8,

	/* read region */
	bs_notimpl_bs_rr_1,
	generic_armv4_bs_rr_2,
	generic_bs_rr_4,
	bs_notimpl_bs_rr_8,

	/* write (single) */
	generic_bs_w_1,
	generic_armv4_bs_w_2,
	generic_bs_w_4,
	bs_notimpl_bs_w_8,

	/* write multiple */
	generic_bs_wm_1,
	generic_armv4_bs_wm_2,
	generic_bs_wm_4,
	bs_notimpl_bs_wm_8,

	/* write region */
	bs_notimpl_bs_wr_1,
	generic_armv4_bs_wr_2,
	generic_bs_wr_4,
	bs_notimpl_bs_wr_8,

	/* set multiple */
	bs_notimpl_bs_sm_1,
	bs_notimpl_bs_sm_2,
	bs_notimpl_bs_sm_4,
	bs_notimpl_bs_sm_8,

	/* set region */
	bs_notimpl_bs_sr_1,
	generic_armv4_bs_sr_2,
	bs_notimpl_bs_sr_4,
	bs_notimpl_bs_sr_8,

	/* copy */
	bs_notimpl_bs_c_1,
	generic_armv4_bs_c_2,
	bs_notimpl_bs_c_4,
	bs_notimpl_bs_c_8,
};

void
ifpga_create_io_bs_tag(struct bus_space *t, void *cookie)
{
	*t = ifpga_bs_tag;
	t->bs_cookie = cookie;
}

void
ifpga_create_mem_bs_tag(struct bus_space *t, void *cookie)
{
	*t = ifpga_bs_tag;
	t->bs_map = ifpga_mem_bs_map;
	t->bs_unmap = ifpga_mem_bs_unmap;
	t->bs_cookie = cookie;
}

/* bus space functions */

int
ifpga_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int cacheable, bus_space_handle_t *bshp)
{
	/* The cookie is the base address for the I/O area */
	*bshp = bpa + (bus_addr_t)t;
	return 0;
}

int
ifpga_mem_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int cacheable, bus_space_handle_t *bshp)
{
	bus_addr_t startpa, endpa;
	vaddr_t va;
	const struct pmap_devmap *pd;
	bus_addr_t pa = bpa + (bus_addr_t) t;

	if ((pd = pmap_devmap_find_pa(pa, size)) != NULL) {
		/* Device was statically mapped. */
		*bshp = pd->pd_va + (pa - pd->pd_pa);
		return 0;
	}

	/* Round the allocation to page boundries */
	startpa = trunc_page(bpa);
	endpa = round_page(bpa + size);

	/* Get some VM.  */
	va = uvm_km_alloc(kernel_map, endpa - startpa, 0,
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (va == 0)
		return ENOMEM;

	/* Store the bus space handle */
	*bshp = va + (bpa & PGOFSET);

	/* Now map the pages */
	/* The cookie is the physical base address for the I/O area */
	while (startpa < endpa) {
		/* XXX pmap_kenter_pa maps pages cacheable -- not what 
		   we want.  */
		pmap_enter(pmap_kernel(), va, (bus_addr_t)t + startpa,
			   VM_PROT_READ | VM_PROT_WRITE, 0);
		va += PAGE_SIZE;
		startpa += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());

	return 0;
}

int
ifpga_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend, bus_size_t size,
	bus_size_t alignment, bus_size_t boundary, int cacheable,
	bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	panic("ifpga_alloc(): Help!");
}


void
ifpga_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	/* Nothing to do for an io map.  */
}

void
ifpga_mem_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	vaddr_t startva, endva;

	if (pmap_devmap_find_va(bsh, size) != NULL) {
		/* Device was statically mapped; nothing to do. */
		return;
	}

	startva = trunc_page(bsh);
	endva = round_page(bsh + size);

	pmap_remove(pmap_kernel(), startva, endva);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, startva, endva - startva, UVM_KMF_VAONLY);
}

void    
ifpga_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{

	panic("ifpga_free(): Help!");
	/* ifpga_bs_unmap() does all that we need to do. */
/*	ifpga_bs_unmap(t, bsh, size);*/
}

int
ifpga_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + (offset << ((int)t));
	return (0);
}

void *
ifpga_bs_vaddr(void *t, bus_space_handle_t bsh)
{

	return ((void *)bsh);
}

void
ifpga_bs_barrier(void *t, bus_space_handle_t bsh, bus_size_t offset, bus_size_t len, int flags)
{
}	
