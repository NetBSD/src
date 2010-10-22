/*	$NetBSD: mvsoc_space.c,v 1.1.4.2 2010/10/22 09:23:12 uebayasi Exp $	*/
/*
 * Copyright (c) 2007 KIYOHARA Takashi
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
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mvsoc_space.c,v 1.1.4.2 2010/10/22 09:23:12 uebayasi Exp $");

#include "opt_mvsoc.h"
#include "mvpex.h"
#include "gtpci.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>


/* Proto types for all the bus_space structure functions */
bs_protos(mvsoc);
bs_protos(generic);
bs_protos(generic_armv4);
bs_protos(bs_notimpl);

#define MVSOC_BUS_SPACE_DEFAULT_FUNCS		\
	/* mapping/unmapping */			\
	mvsoc_bs_map,				\
	mvsoc_bs_unmap,				\
	mvsoc_bs_subregion,			\
						\
	/* allocation/deallocation */		\
	mvsoc_bs_alloc,				\
	mvsoc_bs_free,				\
						\
	/* get kernel virtual address */	\
	mvsoc_bs_vaddr,				\
						\
	/* mmap bus space for userland */	\
	bs_notimpl_bs_mmap,			\
						\
	/* barrier */				\
	mvsoc_bs_barrier,			\
						\
	/* read (single) */			\
	generic_bs_r_1,				\
	generic_armv4_bs_r_2,			\
	generic_bs_r_4,				\
	bs_notimpl_bs_r_8,			\
						\
	/* read multiple */			\
	generic_bs_rm_1,			\
	generic_armv4_bs_rm_2,			\
	generic_bs_rm_4,			\
	bs_notimpl_bs_rm_8,			\
						\
	/* read region */			\
	generic_bs_rr_1,			\
	generic_armv4_bs_rr_2,			\
	generic_bs_rr_4,			\
	bs_notimpl_bs_rr_8,			\
						\
	/* write (single) */			\
	generic_bs_w_1,				\
	generic_armv4_bs_w_2,			\
	generic_bs_w_4,				\
	bs_notimpl_bs_w_8,			\
						\
	/* write multiple */			\
	generic_bs_wm_1,			\
	generic_armv4_bs_wm_2,			\
	generic_bs_wm_4,			\
	bs_notimpl_bs_wm_8,			\
						\
	/* write region */			\
	generic_bs_wr_1,			\
	generic_armv4_bs_wr_2,			\
	generic_bs_wr_4,			\
	bs_notimpl_bs_wr_8,			\
						\
	/* set multiple */			\
	bs_notimpl_bs_sm_1,			\
	bs_notimpl_bs_sm_2,			\
	bs_notimpl_bs_sm_4,			\
	bs_notimpl_bs_sm_8,			\
						\
	/* set region */			\
	bs_notimpl_bs_sr_1,			\
	generic_armv4_bs_sr_2,			\
	generic_bs_sr_4,			\
	bs_notimpl_bs_sr_8,			\
						\
	/* copy */				\
	bs_notimpl_bs_c_1,			\
	generic_armv4_bs_c_2,			\
	bs_notimpl_bs_c_4,			\
	bs_notimpl_bs_c_8,


struct bus_space mvsoc_bs_tag = {
	/* cookie */
	(void *)0,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};

#if NMVPEX > 0
#if defined(ORION)
struct bus_space orion_pex0_mem_bs_tag = {
	/* cookie */
	(void *)ORION_TAG_PEX0_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space orion_pex0_io_bs_tag = {
	/* cookie */
	(void *)ORION_TAG_PEX0_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space orion_pex1_mem_bs_tag = {
	/* cookie */
	(void *)ORION_TAG_PEX1_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space orion_pex1_io_bs_tag = {
	/* cookie */
	(void *)ORION_TAG_PEX1_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
#endif

#if defined(KIRKWOOD)
struct bus_space kirkwood_pex_mem_bs_tag = {
	/* cookie */
	(void *)KIRKWOOD_TAG_PEX_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space kirkwood_pex_io_bs_tag = {
	/* cookie */
	(void *)KIRKWOOD_TAG_PEX_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
#endif
#endif

#if NGTPCI > 0
#if defined(ORION)
struct bus_space orion_pci_mem_bs_tag = {
	/* cookie */
	(void *)ORION_TAG_PCI_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space orion_pci_io_bs_tag = {
	/* cookie */
	(void *)ORION_TAG_PCI_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
#endif
#endif


int
mvsoc_bs_map(void *space, bus_addr_t address, bus_size_t size, int flags,
	     bus_space_handle_t *handlep)
{
	const struct pmap_devmap *pd;
	paddr_t startpa, endpa, offset, pa;
	pt_entry_t *pte;
	vaddr_t va;
	int tag = (int)space;

	if (tag != 0) {
		bus_addr_t remap;
		uint32_t base;
		int window;

		window = mvsoc_target(tag, NULL, NULL, &base, NULL);
		if (window == -1)
			return ENOMEM;
		if (window < nremap) {
			remap = read_mlmbreg(MVSOC_MLMB_WRLR(window)) &
			    MVSOC_MLMB_WRLR_REMAP_MASK;
			remap |=
			    (read_mlmbreg(MVSOC_MLMB_WRHR(window)) << 16) << 16;
			address = address - remap + base;
		}
	}

	if ((pd = pmap_devmap_find_pa(address, size)) != NULL) {
		/* Device was statically mapped. */
		*handlep = pd->pd_va + (address - pd->pd_pa);
		return 0;
	}

	startpa = trunc_page(address);
	endpa = round_page(address + size);
	offset = address & PAGE_MASK;

	/* XXX use extent manager to check duplicate mapping */

	va = uvm_km_alloc(kernel_map, endpa - startpa, 0,
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (va == 0x00000000)
		return ENOMEM;

	*handlep = va + offset;

	/* Now map the pages */
	for (pa = startpa; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
		if ((flags & BUS_SPACE_MAP_CACHEABLE) == 0) {
			pte = vtopte(va);
			*pte &= ~L2_S_CACHE_MASK;
			PTE_SYNC(pte);
			/*
			 * XXX: pmap_kenter_pa() also does PTE_SYNC(). a bit of
			 *      waste.
			 */
		}
	}
	pmap_update(pmap_kernel());

	return 0;
}

void
mvsoc_bs_unmap(void *space, bus_space_handle_t handle, bus_size_t size)
{
	vaddr_t va, sz;

	if (pmap_devmap_find_va(handle, size) != NULL)
		/* Device was statically mapped; nothing to do. */
		return;

	va = trunc_page(handle);
        sz = round_page(handle + size) - va;

	pmap_kremove(va, sz);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, va, sz, UVM_KMF_VAONLY);
}

/* ARGSUSED */
int
mvsoc_bs_subregion(void *space, bus_space_handle_t handle,
		       bus_size_t offset, bus_size_t size,
		       bus_space_handle_t *nhandlep)
{

	*nhandlep = handle + offset;
	return 0;
}

/* ARGSUSED */
int
mvsoc_bs_alloc(void *space, bus_addr_t reg_start, bus_addr_t reg_end,
	       bus_size_t size, bus_size_t alignment, bus_size_t boundary,
	       int flags, bus_addr_t *addrp, bus_space_handle_t *handlep)
{

	panic("%s(): not implemented\n", __func__);
}

/* ARGSUSED */
void
mvsoc_bs_free(void *space, bus_space_handle_t handle, bus_size_t size)
{

	panic("%s(): not implemented\n", __func__);
}

/* ARGSUSED */
void
mvsoc_bs_barrier(void *space, bus_space_handle_t handle, bus_size_t offset,
		 bus_size_t length, int flags)
{

	/* Nothing to do. */
}

/* ARGSUSED */
void *
mvsoc_bs_vaddr(void *space, bus_space_handle_t handle)
{

	return (void *)handle;
}
