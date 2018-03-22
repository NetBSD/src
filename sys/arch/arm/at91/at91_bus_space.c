/*	$NetBSD: at91_bus_space.c,v 1.4.52.1 2018/03/22 01:44:42 pgoyette Exp $ */

/*
 * Based on ep93xx_space.c
 * Copyright (c) 2007 Embedtronics Oy
 * All rights reserved.
 *
 * Copyright (c) 2004 Jesse Off
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
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: at91_bus_space.c,v 1.4.52.1 2018/03/22 01:44:42 pgoyette Exp $");

/*
 * bus_space I/O functions for ep93xx
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/queue.h>

#include <uvm/uvm.h>

#include <sys/bus.h>

#include <arm/at91/at91var.h>
//#include <arm/ep93xx/ep93xxreg.h>
//#include <arm/ep93xx/ep93xxvar.h>

/* Proto types for all the bus_space structure functions */
bs_protos(at91);
bs_protos(generic);
bs_protos(generic_armv4);
bs_protos(bs_notimpl);

struct bus_space at91_bs_tag = {
	/* cookie */
	.bs_cookie = (void *) 0,

	/* mapping/unmapping */
	.bs_map = at91_bs_map,
	.bs_unmap = at91_bs_unmap,
	.bs_subregion = at91_bs_subregion,

	/* allocation/deallocation */
	.bs_alloc = at91_bs_alloc,
	.bs_free = at91_bs_free,

	/* get kernel virtual address */
	.bs_vaddr = at91_bs_vaddr,

	/* mmap bus space for userland */
	.bs_mmap = at91_bs_mmap,

	/* barrier */
	.bs_barrier = at91_bs_barrier,

	/* read (single) */
	.bs_r_1 = generic_bs_r_1,
	.bs_r_2 = generic_armv4_bs_r_2,
	.bs_r_4 = generic_bs_r_4,
	.bs_r_8 = bs_notimpl_bs_r_8,

	/* read multiple */
	.bs_rm_1 = generic_bs_rm_1,
	.bs_rm_2 = generic_armv4_bs_rm_2,
	.bs_rm_4 = generic_bs_rm_4,
	.bs_rm_8 = bs_notimpl_bs_rm_8,

	/* read region */
	.bs_rr_1 = generic_bs_rr_1,
	.bs_rr_2 = generic_armv4_bs_rr_2,
	.bs_rr_4 = generic_bs_rr_4,
	.bs_rr_8 = bs_notimpl_bs_rr_8,

	/* write (single) */
	.bs_w_1 = generic_bs_w_1,
	.bs_w_2 = generic_armv4_bs_w_2,
	.bs_w_4 = generic_bs_w_4,
	.bs_w_8 = bs_notimpl_bs_w_8,

	/* write multiple */
	.bs_wm_1 = generic_bs_wm_1,
	.bs_wm_2 = generic_armv4_bs_wm_2,
	.bs_wm_4 = generic_bs_wm_4,
	.bs_wm_8 = bs_notimpl_bs_wm_8,

	/* write region */
	.bs_wr_1 = generic_bs_wr_1,
	.bs_wr_2 = generic_armv4_bs_wr_2,
	.bs_wr_4 = generic_bs_wr_4,
	.bs_wr_8 = bs_notimpl_bs_wr_8,

	/* set multiple */
	.bs_sm_1 = bs_notimpl_bs_sm_1,
	.bs_sm_2 = bs_notimpl_bs_sm_2,
	.bs_sm_4 = bs_notimpl_bs_sm_4,
	.bs_sm_8 = bs_notimpl_bs_sm_8,

	/* set region */
	.bs_sr_1 = bs_notimpl_bs_sr_1,
	.bs_sr_2 = generic_armv4_bs_sr_2,
	.bs_sr_4 = generic_bs_sr_4,
	.bs_sr_8 = bs_notimpl_bs_sr_8,

	/* copy */
	.bs_c_1 = bs_notimpl_bs_c_1,
	.bs_c_2 = generic_armv4_bs_c_2,
	.bs_c_4 = bs_notimpl_bs_c_4,
	.bs_c_8 = bs_notimpl_bs_c_8,
};

int
at91_bs_map(void *t, bus_addr_t bpa, bus_size_t size,
	      int cacheable, bus_space_handle_t *bshp)
{
	const struct pmap_devmap	*pd;

	paddr_t		startpa;
        paddr_t		endpa;
        paddr_t		pa;
        paddr_t		offset;
        vaddr_t		va;

	if ((pd = pmap_devmap_find_pa(bpa, size)) != NULL) {
		/* Device was statically mapped. */
		*bshp = pd->pd_va + (bpa - pd->pd_pa);
		return 0;
	}

	endpa = round_page(bpa + size);
	offset = bpa & PAGE_MASK;
	startpa = trunc_page(bpa);

	/* Get some VM.  */
	va = uvm_km_alloc(kernel_map, endpa - startpa, 0,
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (va == 0)
		return ENOMEM;

	/* Store the bus space handle */
	*bshp = va + offset;

	/* Now map the pages */
	for (pa = startpa; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_enter(pmap_kernel(), va, pa,
		    VM_PROT_READ | VM_PROT_WRITE,
		    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);
	}
	pmap_update(pmap_kernel());

	return(0);
}

void
at91_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	vaddr_t	va;
	vaddr_t	endva;

	if (pmap_devmap_find_va(bsh, size) != NULL) {
		/* Device was statically mapped; nothing to do. */
		return;
	}

	endva = round_page(bsh + size);
	va = trunc_page(bsh);

	pmap_remove(pmap_kernel(), va, endva);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, va, endva - va, UVM_KMF_VAONLY);
}

int
at91_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
	bus_size_t size, bus_size_t alignment, bus_size_t boundary, int cacheable,
	bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	panic("at91_bs_alloc(): not implemented\n");
}

void    
at91_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	panic("at91_bs_free(): not implemented\n");
}

int
at91_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset,
	bus_size_t size, bus_space_handle_t *nbshp)
{
	*nbshp = bsh + offset;
	return (0);
}

void *
at91_bs_vaddr(void *t, bus_space_handle_t bsh)
{
	return ((void *)bsh);
}

paddr_t
at91_bs_mmap(void *t, bus_addr_t addr, off_t off, int prot, int flags)
{
	/* Not supported. */
	return (-1);
}

void
at91_bs_barrier(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t len, int flags)
{
/* NULL */
}	
/* End of at91_io.c */
