/* $Id: imx_space.c,v 1.5.38.1 2018/03/22 01:44:42 pgoyette Exp $ */

/* derived from: */
/*	$NetBSD: imx_space.c,v 1.5.38.1 2018/03/22 01:44:42 pgoyette Exp $ */

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA.
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
 *	This product includes software developed by Mark Brinicombe.
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
 */

/*
 * bus_space(9) support for Freescale iMX31 processor
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <uvm/uvm_extern.h>
#include <sys/bus.h>

bs_protos(imx);
bs_protos(generic);
bs_protos(generic_armv4);
bs_protos(bs_notimpl);

struct bus_space imx_bs_tag = {
	/* cookie */
	.bs_cookie = (void *) 0,

	/* mapping/unmapping */
	.bs_map = imx_bs_map,
	.bs_unmap = imx_bs_unmap,
	.bs_subregion = imx_bs_subregion,

	/* allocation/deallocation */
	.bs_alloc = imx_bs_alloc,	/* not implemented */
	.bs_free = imx_bs_free,		/* not implemented */

	/* get kernel virtual address */
	.bs_vaddr = imx_bs_vaddr,

	/* mmap */
	.bs_mmap = bs_notimpl_bs_mmap,

	/* barrier */
	.bs_barrier = imx_bs_barrier,

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
	.bs_sr_1 = generic_bs_sr_1,
	.bs_sr_2 = generic_armv4_bs_sr_2,
	.bs_sr_4 = bs_notimpl_bs_sr_4,
	.bs_sr_8 = bs_notimpl_bs_sr_8,

	/* copy */
	.bs_c_1 = bs_notimpl_bs_c_1,
	.bs_c_2 = generic_armv4_bs_c_2,
	.bs_c_4 = bs_notimpl_bs_c_4,
	.bs_c_8 = bs_notimpl_bs_c_8,

#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	/* read (single) */
	.bs_r_1_s = generic_bs_r_1,
	.bs_r_2_s = generic_armv4_bs_r_2,
	.bs_r_4_s = generic_bs_r_4,
	.bs_r_8_s = bs_notimpl_bs_r_8,

	/* read multiple */
	.bs_rm_1_s = generic_bs_rm_1,
	.bs_rm_2_s = generic_armv4_bs_rm_2,
	.bs_rm_4_s = generic_bs_rm_4,
	.bs_rm_8_s = bs_notimpl_bs_rm_8,

	/* read region */
	.bs_rr_1_s = generic_bs_rr_1,
	.bs_rr_2_s = generic_armv4_bs_rr_2,
	.bs_rr_4_s = generic_bs_rr_4,
	.bs_rr_8_s = bs_notimpl_bs_rr_8,

	/* write (single) */
	.bs_w_1_s = generic_bs_w_1,
	.bs_w_2_s = generic_armv4_bs_w_2,
	.bs_w_4_s = generic_bs_w_4,
	.bs_w_8_s = bs_notimpl_bs_w_8,

	/* write multiple */
	.bs_wm_1_s = generic_bs_wm_1,
	.bs_wm_2_s = generic_armv4_bs_wm_2,
	.bs_wm_4_s = generic_bs_wm_4,
	.bs_wm_8_s = bs_notimpl_bs_wm_8,

	/* write region */
	.bs_wr_1_s = generic_bs_wr_1,
	.bs_wr_2_s = generic_armv4_bs_wr_2,
	.bs_wr_4_s = generic_bs_wr_4,
	.bs_wr_8_s = bs_notimpl_bs_wr_8,
#endif
};

int
imx_bs_map(void *t, bus_addr_t bpa, bus_size_t size,
	      int flag, bus_space_handle_t *bshp)
{
	const struct pmap_devmap	*pd;
	paddr_t startpa, endpa, pa;
	vaddr_t va;

	if ((pd = pmap_devmap_find_pa(bpa, size)) != NULL) {
		/* Device was statically mapped. */
		*bshp = pd->pd_va + (bpa - pd->pd_pa);
		return 0;
	}

	startpa = trunc_page(bpa);
	endpa = round_page(bpa + size);

	/* XXX use extent manager to check duplicate mapping */

	va = uvm_km_alloc(kernel_map, endpa - startpa, 0,
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (! va)
		return(ENOMEM);

	*bshp = (bus_space_handle_t)(va + (bpa - startpa));

	for (pa = startpa; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE,
		    (flag & BUS_SPACE_MAP_CACHEABLE) ? 0 : PMAP_NOCACHE);
	}
	pmap_update(pmap_kernel());

	return(0);
}

void
imx_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	vaddr_t	va;
	vsize_t	sz;

	if (pmap_devmap_find_va(bsh, size) != NULL) {
		/* Device was statically mapped; nothing to do. */
		return;
	}

	va = trunc_page(bsh);
	sz = round_page(bsh + size) - va;

	pmap_kremove(va, sz);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, va, sz, UVM_KMF_VAONLY);
}


int
imx_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return (0);
}

void
imx_bs_barrier(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t len, int flags)
{

	/* Nothing to do. */
}

void *
imx_bs_vaddr(void *t, bus_space_handle_t bsh)
{

	return ((void *)bsh);
}


int
imx_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{

	panic("imx_io_bs_alloc(): not implemented\n");
}

void    
imx_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{

	panic("imx_io_bs_free(): not implemented\n");
}


