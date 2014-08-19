/*	$NetBSD: ixp12x0_io.c,v 1.15.12.1 2014/08/20 00:02:46 tls Exp $ */

/*
 * Copyright (c) 2002, 2003
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
__KERNEL_RCSID(0, "$NetBSD: ixp12x0_io.c,v 1.15.12.1 2014/08/20 00:02:46 tls Exp $");

/*
 * bus_space I/O functions for ixp12x0
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/queue.h>

#include <uvm/uvm.h>

#include <sys/bus.h>

#include <arm/ixp12x0/ixp12x0reg.h>
#include <arm/ixp12x0/ixp12x0var.h>

/* Proto types for all the bus_space structure functions */
bs_protos(ixp12x0);
bs_protos(generic);
bs_protos(generic_armv4);
bs_protos(bs_notimpl);

struct bus_space ixp12x0_bs_tag = {
	/* cookie */
	(void *) 0,

	/* mapping/unmapping */
	ixp12x0_bs_map,
	ixp12x0_bs_unmap,
	ixp12x0_bs_subregion,

	/* allocation/deallocation */
	ixp12x0_bs_alloc,
	ixp12x0_bs_free,

	/* get kernel virtual address */
	ixp12x0_bs_vaddr,

	/* mmap bus space for userland */
	bs_notimpl_bs_mmap,

	/* barrier */
	ixp12x0_bs_barrier,

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
	generic_bs_rr_1,
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
	generic_bs_wr_1,
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

/* Common routines */

int
ixp12x0_bs_map(void *t, bus_addr_t bpa, bus_size_t size,
	       int flags, bus_space_handle_t *bshp)
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
		
	va = uvm_km_alloc(kernel_map, endpa - startpa, 0,
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (va  == 0)
		return ENOMEM;

	*bshp = va + offset;

	const int pmapflags =
	    (flags & (BUS_SPACE_MAP_CACHEABLE|BUS_SPACE_MAP_PREFETCHABLE))
		? 0
		: PMAP_NOCACHE;
	for (pa = startpa; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, pmapflags);
	}
	pmap_update(pmap_kernel());

	return 0;
}

void
ixp12x0_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
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
	uvm_km_free(kernel_map, va, endva - va, UVM_KMF_VAONLY);
}

int
ixp12x0_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return (0);
}

void *
ixp12x0_bs_vaddr(void *t, bus_space_handle_t bsh)
{
	return ((void *)bsh);
}

int
ixp12x0_bs_alloc(void *t,
		 bus_addr_t rstart, bus_addr_t rend, bus_size_t size,
		 bus_size_t alignment, bus_size_t boundary,
		 int flags, bus_addr_t *bpap,
		 bus_space_handle_t *bshp)
{
	panic("ixp12x0_bs_alloc(): not implemented\n");
}

void    
ixp12x0_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	panic("ixp12x0_bs_free(): not implemented\n");
}

void
ixp12x0_bs_barrier(void *t, bus_space_handle_t bsh, bus_size_t offset, bus_size_t len, int flags)
{
/* NULL */
}	

/* End of ixp12x0_io.c */
