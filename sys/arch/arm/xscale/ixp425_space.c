/*	$NetBSD: ixp425_space.c,v 1.1 2003/09/25 14:11:18 ichiro Exp $ */

/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: ixp425_space.c,v 1.1 2003/09/25 14:11:18 ichiro Exp $");

/*
 * bus_space I/O functions for ixp425
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/queue.h>

#include <uvm/uvm.h>

#include <machine/bus.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

/* Proto types for all the bus_space structure functions */
bs_protos(ixp425);
bs_protos(generic);
bs_protos(generic_armv4);
bs_protos(bs_notimpl);

struct bus_space ixp425_bs_tag = {
	/* cookie */
	(void *) 0,

	/* mapping/unmapping */
	ixp425_bs_map,
	ixp425_bs_unmap,
	ixp425_bs_subregion,

	/* allocation/deallocation */
	ixp425_bs_alloc,
	ixp425_bs_free,

	/* get kernel virtual address */
	ixp425_bs_vaddr,

	/* mmap bus space for userland */
	ixp425_bs_mmap,

	/* barrier */
	ixp425_bs_barrier,

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
        generic_bs_sr_4,
        bs_notimpl_bs_sr_8,

        /* copy */
        bs_notimpl_bs_c_1,
        generic_armv4_bs_c_2,
        bs_notimpl_bs_c_4,
        bs_notimpl_bs_c_8,
};

int
ixp425_bs_map(void *t, bus_addr_t bpa, bus_size_t size,
	      int cacheable, bus_space_handle_t *bshp)
{
	const struct pmap_devmap	*pd;

	paddr_t		startpa;
        paddr_t		endpa;
        paddr_t		pa;
        paddr_t		offset;
        vaddr_t		va;
        pt_entry_t	*pte;

	if ((pd = pmap_devmap_find_pa(bpa, size)) != NULL) {
		/* Device was statically mapped. */
		*bshp = pd->pd_va + (bpa - pd->pd_pa);
		return 0;
	}

	endpa = round_page(bpa + size);
	offset = bpa & PAGE_MASK;
	startpa = trunc_page(bpa);

	/* Get some VM.  */
	if ((va = uvm_km_valloc(kernel_map, endpa - startpa)) == 0)
		return ENOMEM;

	/* Store the bus space handle */
	*bshp = va + offset;

	/* Now map the pages */
	for (pa = startpa; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
		pte = vtopte(va);
		*pte &= ~L2_S_CACHE_MASK;
		PTE_SYNC(pte);
	}
	pmap_update(pmap_kernel());

	return(0);
}

void
ixp425_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	vaddr_t	va;
	vaddr_t	endva;

	if (pmap_devmap_find_va(bsh, size) != NULL) {
		/* Device was statically mapped; nothing to do. */
		return;
	}

	endva = round_page(bsh + size);
	va = trunc_page(bsh);

	pmap_kremove(va, endva - va);
	uvm_km_free(kernel_map, va, endva - va);
}

int
ixp425_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
	bus_size_t size, bus_size_t alignment, bus_size_t boundary, int cacheable,
	bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	panic("ixp425_bs_alloc(): not implemented\n");
}

void    
ixp425_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	panic("ixp425_bs_free(): not implemented\n");
}

int
ixp425_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset,
	bus_size_t size, bus_space_handle_t *nbshp)
{
	*nbshp = bsh + offset;
	return (0);
}

void *
ixp425_bs_vaddr(void *t, bus_space_handle_t bsh)
{
	return ((void *)bsh);
}

paddr_t
ixp425_bs_mmap(void *t, bus_addr_t addr, off_t off, int prot, int flags)
{
	/* Not supported. */
	return (-1);
}

void
ixp425_bs_barrier(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t len, int flags)
{
/* NULL */
}	
/* End of ixp425_space.c */
