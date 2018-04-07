/* $NetBSD: bus_space.c,v 1.1.28.1 2018/04/07 04:12:10 pgoyette Exp $ */

/*
 * Copyright (c) 2017 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: bus_space.c,v 1.1.28.1 2018/04/07 04:12:10 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <aarch64/bus_funcs.h>
#include <aarch64/machdep.h>


/* Prototypes for all the bus_space structure functions */
bs_protos(generic)
bs_protos(generic_dsb)

struct bus_space arm_generic_bs_tag = {
	.bs_cookie = &arm_generic_bs_tag,

	.bs_stride = 0,
	.bs_flags = 0,

	.bs_map = generic_bs_map,
	.bs_unmap = generic_bs_unmap,
	.bs_subregion = generic_bs_subregion,
	.bs_alloc = generic_bs_alloc,
	.bs_free = generic_bs_free,
	.bs_vaddr = generic_bs_vaddr,
	.bs_mmap = generic_bs_mmap,
	.bs_barrier = generic_bs_barrier,

	/* read */
	.bs_r_1 = generic_bs_r_1,
	.bs_r_2 = generic_bs_r_2,
	.bs_r_4 = generic_bs_r_4,
	.bs_r_8 = generic_bs_r_8,

	/* write */
	.bs_w_1 = generic_bs_w_1,
	.bs_w_2 = generic_bs_w_2,
	.bs_w_4 = generic_bs_w_4,
	.bs_w_8 = generic_bs_w_8,

	/* read region */
	.bs_rr_1 = generic_bs_rr_1,
	.bs_rr_2 = generic_bs_rr_2,
	.bs_rr_4 = generic_bs_rr_4,
	.bs_rr_8 = generic_bs_rr_8,

	/* write region */
	.bs_wr_1 = generic_bs_wr_1,
	.bs_wr_2 = generic_bs_wr_2,
	.bs_wr_4 = generic_bs_wr_4,
	.bs_wr_8 = generic_bs_wr_8,

	/* copy region */
	.bs_c_1 = generic_bs_c_1,
	.bs_c_2 = generic_bs_c_2,
	.bs_c_4 = generic_bs_c_4,
	.bs_c_8 = generic_bs_c_8,

	/* set region */
	.bs_sr_1 = generic_bs_sr_1,
	.bs_sr_2 = generic_bs_sr_2,
	.bs_sr_4 = generic_bs_sr_4,
	.bs_sr_8 = generic_bs_sr_8,

	/* read multi */
	.bs_rm_1 = generic_bs_rm_1,
	.bs_rm_2 = generic_bs_rm_2,
	.bs_rm_4 = generic_bs_rm_4,
	.bs_rm_8 = generic_bs_rm_8,

	/* write multi */
	.bs_wm_1 = generic_bs_wm_1,
	.bs_wm_2 = generic_bs_wm_2,
	.bs_wm_4 = generic_bs_wm_4,
	.bs_wm_8 = generic_bs_wm_8,

	/* set multi */
	.bs_sm_1 = generic_bs_sm_1,
	.bs_sm_2 = generic_bs_sm_2,
	.bs_sm_4 = generic_bs_sm_4,
	.bs_sm_8 = generic_bs_sm_8,

#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	/* read stream */
	.bs_r_1_s = generic_bs_r_1,
	.bs_r_2_s = generic_bs_r_2,
	.bs_r_4_s = generic_bs_r_4,
	.bs_r_8_s = generic_bs_r_8,

	/* write stream */
	.bs_w_1_s = generic_bs_w_1,
	.bs_w_2_s = generic_bs_w_2,
	.bs_w_4_s = generic_bs_w_4,
	.bs_w_8_s = generic_bs_w_8,

	/* read region stream */
	.bs_rr_1_s = generic_bs_rr_1,
	.bs_rr_2_s = generic_bs_rr_2,
	.bs_rr_4_s = generic_bs_rr_4,
	.bs_rr_8_s = generic_bs_rr_8,

	/* write region stream */
	.bs_wr_1_s = generic_bs_wr_1,
	.bs_wr_2_s = generic_bs_wr_2,
	.bs_wr_4_s = generic_bs_wr_4,
	.bs_wr_8_s = generic_bs_wr_8,

	/* read multi stream */
	.bs_rm_1_s = generic_bs_rm_1,
	.bs_rm_2_s = generic_bs_rm_2,
	.bs_rm_4_s = generic_bs_rm_4,
	.bs_rm_8_s = generic_bs_rm_8,

	/* write multi stream */
	.bs_wm_1_s = generic_bs_wm_1,
	.bs_wm_2_s = generic_bs_wm_2,
	.bs_wm_4_s = generic_bs_wm_4,
	.bs_wm_8_s = generic_bs_wm_8,
#endif

#ifdef __BUS_SPACE_HAS_PROBING_METHODS
	/* peek */
	.bs_pe_1 = generic_bs_pe_1,
	.bs_pe_2 = generic_bs_pe_2,
	.bs_pe_4 = generic_bs_pe_4,
	.bs_pe_8 = generic_bs_pe_8,

	/* poke */
	.bs_po_1 = generic_bs_po_1,
	.bs_po_2 = generic_bs_po_2,
	.bs_po_4 = generic_bs_po_4,
	.bs_po_8 = generic_bs_po_8,
#endif
};

struct bus_space aarch64_generic_dsb_bs_tag = {
	.bs_stride = 0,
	.bs_flags = 0,

	.bs_map = generic_bs_map,
	.bs_unmap = generic_bs_unmap,
	.bs_subregion = generic_bs_subregion,
	.bs_alloc = generic_bs_alloc,
	.bs_free = generic_bs_free,
	.bs_vaddr = generic_bs_vaddr,
	.bs_mmap = generic_bs_mmap,
	.bs_barrier = generic_bs_barrier,

	/* read */
	.bs_r_1 = generic_dsb_bs_r_1,
	.bs_r_2 = generic_dsb_bs_r_2,
	.bs_r_4 = generic_dsb_bs_r_4,
	.bs_r_8 = generic_dsb_bs_r_8,

	/* write */
	.bs_w_1 = generic_dsb_bs_w_1,
	.bs_w_2 = generic_dsb_bs_w_2,
	.bs_w_4 = generic_dsb_bs_w_4,
	.bs_w_8 = generic_dsb_bs_w_8,

	/* read region */
	.bs_rr_1 = generic_dsb_bs_rr_1,
	.bs_rr_2 = generic_dsb_bs_rr_2,
	.bs_rr_4 = generic_dsb_bs_rr_4,
	.bs_rr_8 = generic_dsb_bs_rr_8,

	/* write region */
	.bs_wr_1 = generic_dsb_bs_wr_1,
	.bs_wr_2 = generic_dsb_bs_wr_2,
	.bs_wr_4 = generic_dsb_bs_wr_4,
	.bs_wr_8 = generic_dsb_bs_wr_8,

	/* copy region */
	.bs_c_1 = generic_dsb_bs_c_1,
	.bs_c_2 = generic_dsb_bs_c_2,
	.bs_c_4 = generic_dsb_bs_c_4,
	.bs_c_8 = generic_dsb_bs_c_8,

	/* set region */
	.bs_sr_1 = generic_dsb_bs_sr_1,
	.bs_sr_2 = generic_dsb_bs_sr_2,
	.bs_sr_4 = generic_dsb_bs_sr_4,
	.bs_sr_8 = generic_dsb_bs_sr_8,

	/* read multi */
	.bs_rm_1 = generic_dsb_bs_rm_1,
	.bs_rm_2 = generic_dsb_bs_rm_2,
	.bs_rm_4 = generic_dsb_bs_rm_4,
	.bs_rm_8 = generic_dsb_bs_rm_8,

	/* write multi */
	.bs_wm_1 = generic_dsb_bs_wm_1,
	.bs_wm_2 = generic_dsb_bs_wm_2,
	.bs_wm_4 = generic_dsb_bs_wm_4,
	.bs_wm_8 = generic_dsb_bs_wm_8,

	/* set multi */
	.bs_sm_1 = generic_dsb_bs_sm_1,
	.bs_sm_2 = generic_dsb_bs_sm_2,
	.bs_sm_4 = generic_dsb_bs_sm_4,
	.bs_sm_8 = generic_dsb_bs_sm_8,

#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	/* read stream */
	.bs_r_1_s = generic_dsb_bs_r_1,
	.bs_r_2_s = generic_dsb_bs_r_2,
	.bs_r_4_s = generic_dsb_bs_r_4,
	.bs_r_8_s = generic_dsb_bs_r_8,

	/* write stream */
	.bs_w_1_s = generic_dsb_bs_w_1,
	.bs_w_2_s = generic_dsb_bs_w_2,
	.bs_w_4_s = generic_dsb_bs_w_4,
	.bs_w_8_s = generic_dsb_bs_w_8,

	/* read region stream */
	.bs_rr_1_s = generic_dsb_bs_rr_1,
	.bs_rr_2_s = generic_dsb_bs_rr_2,
	.bs_rr_4_s = generic_dsb_bs_rr_4,
	.bs_rr_8_s = generic_dsb_bs_rr_8,

	/* write region stream */
	.bs_wr_1_s = generic_dsb_bs_wr_1,
	.bs_wr_2_s = generic_dsb_bs_wr_2,
	.bs_wr_4_s = generic_dsb_bs_wr_4,
	.bs_wr_8_s = generic_dsb_bs_wr_8,

	/* read multi stream */
	.bs_rm_1_s = generic_dsb_bs_rm_1,
	.bs_rm_2_s = generic_dsb_bs_rm_2,
	.bs_rm_4_s = generic_dsb_bs_rm_4,
	.bs_rm_8_s = generic_dsb_bs_rm_8,

	/* write multi stream */
	.bs_wm_1_s = generic_dsb_bs_wm_1,
	.bs_wm_2_s = generic_dsb_bs_wm_2,
	.bs_wm_4_s = generic_dsb_bs_wm_4,
	.bs_wm_8_s = generic_dsb_bs_wm_8,
#endif

#ifdef __BUS_SPACE_HAS_PROBING_METHODS
	/* peek */
	.bs_pe_1 = generic_bs_pe_1,
	.bs_pe_2 = generic_bs_pe_2,
	.bs_pe_4 = generic_bs_pe_4,
	.bs_pe_8 = generic_bs_pe_8,

	/* poke */
	.bs_po_1 = generic_bs_po_1,
	.bs_po_2 = generic_bs_po_2,
	.bs_po_4 = generic_bs_po_4,
	.bs_po_8 = generic_bs_po_8,
#endif
};

struct bus_space arm_generic_a4x_bs_tag = {
	.bs_cookie = &arm_generic_a4x_bs_tag,

	.bs_stride = 2,
	.bs_flags = 0,

	.bs_map = generic_bs_map,
	.bs_unmap = generic_bs_unmap,
	.bs_subregion = generic_bs_subregion,
	.bs_alloc = generic_bs_alloc,
	.bs_free = generic_bs_free,
	.bs_vaddr = generic_bs_vaddr,
	.bs_mmap = generic_bs_mmap,
	.bs_barrier = generic_bs_barrier,

	/* read */
	.bs_r_1 = generic_bs_r_1,
	.bs_r_2 = generic_bs_r_2,
	.bs_r_4 = generic_bs_r_4,
	.bs_r_8 = generic_bs_r_8,

	/* write */
	.bs_w_1 = generic_bs_w_1,
	.bs_w_2 = generic_bs_w_2,
	.bs_w_4 = generic_bs_w_4,
	.bs_w_8 = generic_bs_w_8,

	/* read region */
	.bs_rr_1 = generic_bs_rr_1,
	.bs_rr_2 = generic_bs_rr_2,
	.bs_rr_4 = generic_bs_rr_4,
	.bs_rr_8 = generic_bs_rr_8,

	/* write region */
	.bs_wr_1 = generic_bs_wr_1,
	.bs_wr_2 = generic_bs_wr_2,
	.bs_wr_4 = generic_bs_wr_4,
	.bs_wr_8 = generic_bs_wr_8,

	/* copy region */
	.bs_c_1 = generic_bs_c_1,
	.bs_c_2 = generic_bs_c_2,
	.bs_c_4 = generic_bs_c_4,
	.bs_c_8 = generic_bs_c_8,

	/* set region */
	.bs_sr_1 = generic_bs_sr_1,
	.bs_sr_2 = generic_bs_sr_2,
	.bs_sr_4 = generic_bs_sr_4,
	.bs_sr_8 = generic_bs_sr_8,

	/* read multi */
	.bs_rm_1 = generic_bs_rm_1,
	.bs_rm_2 = generic_bs_rm_2,
	.bs_rm_4 = generic_bs_rm_4,
	.bs_rm_8 = generic_bs_rm_8,

	/* write multi */
	.bs_wm_1 = generic_bs_wm_1,
	.bs_wm_2 = generic_bs_wm_2,
	.bs_wm_4 = generic_bs_wm_4,
	.bs_wm_8 = generic_bs_wm_8,

	/* set multi */
	.bs_sm_1 = generic_bs_sm_1,
	.bs_sm_2 = generic_bs_sm_2,
	.bs_sm_4 = generic_bs_sm_4,
	.bs_sm_8 = generic_bs_sm_8,

#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	/* read stream */
	.bs_r_1_s = generic_bs_r_1,
	.bs_r_2_s = generic_bs_r_2,
	.bs_r_4_s = generic_bs_r_4,
	.bs_r_8_s = generic_bs_r_8,

	/* write stream */
	.bs_w_1_s = generic_bs_w_1,
	.bs_w_2_s = generic_bs_w_2,
	.bs_w_4_s = generic_bs_w_4,
	.bs_w_8_s = generic_bs_w_8,

	/* read region stream */
	.bs_rr_1_s = generic_bs_rr_1,
	.bs_rr_2_s = generic_bs_rr_2,
	.bs_rr_4_s = generic_bs_rr_4,
	.bs_rr_8_s = generic_bs_rr_8,

	/* write region stream */
	.bs_wr_1_s = generic_bs_wr_1,
	.bs_wr_2_s = generic_bs_wr_2,
	.bs_wr_4_s = generic_bs_wr_4,
	.bs_wr_8_s = generic_bs_wr_8,

	/* read multi stream */
	.bs_rm_1_s = generic_bs_rm_1,
	.bs_rm_2_s = generic_bs_rm_2,
	.bs_rm_4_s = generic_bs_rm_4,
	.bs_rm_8_s = generic_bs_rm_8,

	/* write multi stream */
	.bs_wm_1_s = generic_bs_wm_1,
	.bs_wm_2_s = generic_bs_wm_2,
	.bs_wm_4_s = generic_bs_wm_4,
	.bs_wm_8_s = generic_bs_wm_8,
#endif

#ifdef __BUS_SPACE_HAS_PROBING_METHODS
	/* peek */
	.bs_pe_1 = generic_bs_pe_1,
	.bs_pe_2 = generic_bs_pe_2,
	.bs_pe_4 = generic_bs_pe_4,
	.bs_pe_8 = generic_bs_pe_8,

	/* poke */
	.bs_po_1 = generic_bs_po_1,
	.bs_po_2 = generic_bs_po_2,
	.bs_po_4 = generic_bs_po_4,
	.bs_po_8 = generic_bs_po_8,
#endif
};

struct bus_space aarch64_generic_a4x_dsb_bs_tag = {
	.bs_stride = 2,
	.bs_flags = 0,

	.bs_map = generic_bs_map,
	.bs_unmap = generic_bs_unmap,
	.bs_subregion = generic_bs_subregion,
	.bs_alloc = generic_bs_alloc,
	.bs_free = generic_bs_free,
	.bs_vaddr = generic_bs_vaddr,
	.bs_mmap = generic_bs_mmap,
	.bs_barrier = generic_bs_barrier,

	/* read */
	.bs_r_1 = generic_dsb_bs_r_1,
	.bs_r_2 = generic_dsb_bs_r_2,
	.bs_r_4 = generic_dsb_bs_r_4,
	.bs_r_8 = generic_dsb_bs_r_8,

	/* write */
	.bs_w_1 = generic_dsb_bs_w_1,
	.bs_w_2 = generic_dsb_bs_w_2,
	.bs_w_4 = generic_dsb_bs_w_4,
	.bs_w_8 = generic_dsb_bs_w_8,

	/* read region */
	.bs_rr_1 = generic_dsb_bs_rr_1,
	.bs_rr_2 = generic_dsb_bs_rr_2,
	.bs_rr_4 = generic_dsb_bs_rr_4,
	.bs_rr_8 = generic_dsb_bs_rr_8,

	/* write region */
	.bs_wr_1 = generic_dsb_bs_wr_1,
	.bs_wr_2 = generic_dsb_bs_wr_2,
	.bs_wr_4 = generic_dsb_bs_wr_4,
	.bs_wr_8 = generic_dsb_bs_wr_8,

	/* copy region */
	.bs_c_1 = generic_dsb_bs_c_1,
	.bs_c_2 = generic_dsb_bs_c_2,
	.bs_c_4 = generic_dsb_bs_c_4,
	.bs_c_8 = generic_dsb_bs_c_8,

	/* set region */
	.bs_sr_1 = generic_dsb_bs_sr_1,
	.bs_sr_2 = generic_dsb_bs_sr_2,
	.bs_sr_4 = generic_dsb_bs_sr_4,
	.bs_sr_8 = generic_dsb_bs_sr_8,

	/* read multi */
	.bs_rm_1 = generic_dsb_bs_rm_1,
	.bs_rm_2 = generic_dsb_bs_rm_2,
	.bs_rm_4 = generic_dsb_bs_rm_4,
	.bs_rm_8 = generic_dsb_bs_rm_8,

	/* write multi */
	.bs_wm_1 = generic_dsb_bs_wm_1,
	.bs_wm_2 = generic_dsb_bs_wm_2,
	.bs_wm_4 = generic_dsb_bs_wm_4,
	.bs_wm_8 = generic_dsb_bs_wm_8,

	/* set multi */
	.bs_sm_1 = generic_dsb_bs_sm_1,
	.bs_sm_2 = generic_dsb_bs_sm_2,
	.bs_sm_4 = generic_dsb_bs_sm_4,
	.bs_sm_8 = generic_dsb_bs_sm_8,

#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	/* read stream */
	.bs_r_1_s = generic_dsb_bs_r_1,
	.bs_r_2_s = generic_dsb_bs_r_2,
	.bs_r_4_s = generic_dsb_bs_r_4,
	.bs_r_8_s = generic_dsb_bs_r_8,

	/* write stream */
	.bs_w_1_s = generic_dsb_bs_w_1,
	.bs_w_2_s = generic_dsb_bs_w_2,
	.bs_w_4_s = generic_dsb_bs_w_4,
	.bs_w_8_s = generic_dsb_bs_w_8,

	/* read region stream */
	.bs_rr_1_s = generic_dsb_bs_rr_1,
	.bs_rr_2_s = generic_dsb_bs_rr_2,
	.bs_rr_4_s = generic_dsb_bs_rr_4,
	.bs_rr_8_s = generic_dsb_bs_rr_8,

	/* write region stream */
	.bs_wr_1_s = generic_dsb_bs_wr_1,
	.bs_wr_2_s = generic_dsb_bs_wr_2,
	.bs_wr_4_s = generic_dsb_bs_wr_4,
	.bs_wr_8_s = generic_dsb_bs_wr_8,

	/* read multi stream */
	.bs_rm_1_s = generic_dsb_bs_rm_1,
	.bs_rm_2_s = generic_dsb_bs_rm_2,
	.bs_rm_4_s = generic_dsb_bs_rm_4,
	.bs_rm_8_s = generic_dsb_bs_rm_8,

	/* write multi stream */
	.bs_wm_1_s = generic_dsb_bs_wm_1,
	.bs_wm_2_s = generic_dsb_bs_wm_2,
	.bs_wm_4_s = generic_dsb_bs_wm_4,
	.bs_wm_8_s = generic_dsb_bs_wm_8,
#endif

#ifdef __BUS_SPACE_HAS_PROBING_METHODS
	/* peek */
	.bs_pe_1 = generic_bs_pe_1,
	.bs_pe_2 = generic_bs_pe_2,
	.bs_pe_4 = generic_bs_pe_4,
	.bs_pe_8 = generic_bs_pe_8,

	/* poke */
	.bs_po_1 = generic_bs_po_1,
	.bs_po_2 = generic_bs_po_2,
	.bs_po_4 = generic_bs_po_4,
	.bs_po_8 = generic_bs_po_8,
#endif
};

int
generic_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int flag,
    bus_space_handle_t *bshp)
{
	const struct pmap_devmap *pd;
	paddr_t startpa, endpa, pa;
	vaddr_t va;
	int pmapflags;

	if ((pd = pmap_devmap_find_pa(bpa, size)) != NULL) {
		*bshp = pd->pd_va + (bpa - pd->pd_pa);
		return 0;
	}

	startpa = trunc_page(bpa);
	endpa = round_page(bpa + size);

	/* XXX use extent manager to check duplicate mapping */

	va = uvm_km_alloc(kernel_map, endpa - startpa, 0,
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (va == 0)
		return ENOMEM;

	*bshp = (bus_space_handle_t)(va + (bpa - startpa));

	if ((flag & BUS_SPACE_MAP_PREFETCHABLE) != 0)
		pmapflags = PMAP_WRITE_COMBINE;
	else if ((flag & BUS_SPACE_MAP_CACHEABLE) != 0)
		pmapflags = PMAP_WRITE_BACK;
	else
		pmapflags = PMAP_DEV;

	for (pa = startpa; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, pmapflags);
	}
	pmap_update(pmap_kernel());

	return 0;
}

void
generic_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	vaddr_t va;
	vsize_t sz;

	if (pmap_devmap_find_va(bsh, size) != NULL)
		return;

	va = trunc_page(bsh);
	sz = round_page(bsh + size) - va;

	pmap_kremove(va, sz);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, va, sz, UVM_KMF_VAONLY);
}


int
generic_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nbshp)
{
	*nbshp = bsh + (offset << ((struct bus_space *)t)->bs_stride);
	return 0;
}

void
generic_bs_barrier(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t len, int flags)
{
	flags &= BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE;

	if (flags != 0)
		__asm __volatile ("dmb sy" ::: "memory");
}

void *
generic_bs_vaddr(void *t, bus_space_handle_t bsh)
{
	return (void *)bsh;
}

paddr_t
generic_bs_mmap(void *t, bus_addr_t bpa, off_t offset, int prot, int flags)
{
	paddr_t bus_flags = 0;

	if ((flags & (BUS_SPACE_MAP_CACHEABLE|BUS_SPACE_MAP_PREFETCHABLE)) != 0)
		bus_flags |= AARCH64_MMAP_WRITEBACK;

	return (atop(bpa + (offset << ((struct bus_space *)t)->bs_stride)) |
	    bus_flags);
}

int
generic_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	panic("%s(): not implemented\n", __func__);
}

void
generic_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	panic("%s(): not implemented\n", __func__);
}

#ifdef __BUS_SPACE_HAS_PROBING_METHODS
int
generic_bs_pe_1(void *t, bus_space_handle_t bsh, bus_size_t offset,
    uint8_t *datap)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb)) == 0) {
		*datap = generic_dsb_bs_r_1(t, bsh, offset);
		cpu_unset_onfault();
	}
	return error;
}

int
generic_bs_pe_2(void *t, bus_space_handle_t bsh, bus_size_t offset,
    uint16_t *datap)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb)) == 0) {
		*datap = generic_dsb_bs_r_2(t, bsh, offset);
		cpu_unset_onfault();
	}
	return error;
}

int
generic_bs_pe_4(void *t, bus_space_handle_t bsh, bus_size_t offset,
    uint32_t *datap)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb)) == 0) {
		*datap = generic_dsb_bs_r_4(t, bsh, offset);
		cpu_unset_onfault();
	}
	return error;
}

int
generic_bs_pe_8(void *t, bus_space_handle_t bsh, bus_size_t offset,
    uint64_t *datap)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb)) == 0) {
		*datap = generic_dsb_bs_r_8(t, bsh, offset);
		cpu_unset_onfault();
	}
	return error;
}

int
generic_bs_po_1(void *t, bus_space_handle_t bsh, bus_size_t offset,
    uint8_t data)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb)) == 0) {
		generic_dsb_bs_w_1(t, bsh, offset, data);
		cpu_unset_onfault();
	}
	return error;
}

int
generic_bs_po_2(void *t, bus_space_handle_t bsh, bus_size_t offset,
    uint16_t data)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb)) == 0) {
		generic_dsb_bs_w_2(t, bsh, offset, data);
		cpu_unset_onfault();
	}
	return error;
}

int
generic_bs_po_4(void *t, bus_space_handle_t bsh, bus_size_t offset,
    uint32_t data)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb)) == 0) {
		generic_dsb_bs_w_4(t, bsh, offset, data);
		cpu_unset_onfault();
	}
	return error;
}

int
generic_bs_po_8(void *t, bus_space_handle_t bsh, bus_size_t offset,
    uint64_t data)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb)) == 0) {
		generic_dsb_bs_w_8(t, bsh, offset, data);
		cpu_unset_onfault();
	}
	return error;
}
#endif /* __BUS_SPACE_HAS_PROBING_METHODS */
