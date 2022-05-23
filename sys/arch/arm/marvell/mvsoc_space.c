/*	$NetBSD: mvsoc_space.c,v 1.12 2022/05/23 21:46:11 andvar Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: mvsoc_space.c,v 1.12 2022/05/23 21:46:11 andvar Exp $");

#include "opt_mvsoc.h"
#include "mvpex.h"
#include "gtpci.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>

#ifdef __ARMEB__
#define	NSWAP(n)	n ## _swap
#else
#define	NSWAP(n)	n
#endif

/* Proto types for all the bus_space structure functions */
bs_protos(mvsoc);
bs_protos(generic);
bs_protos(generic_armv4);
bs_protos(bs_notimpl);

#define MVSOC_BUS_SPACE_NORMAL_FUNCS		\
	/* read (single) */			\
	.bs_r_1 = generic_bs_r_1,		\
	.bs_r_2 = NSWAP(generic_armv4_bs_r_2),	\
	.bs_r_4 = NSWAP(generic_bs_r_4),	\
	.bs_r_8 = bs_notimpl_bs_r_8,		\
						\
	/* read multiple */			\
	.bs_rm_1 = generic_bs_rm_1,		\
	.bs_rm_2 = NSWAP(generic_armv4_bs_rm_2),\
	.bs_rm_4 = NSWAP(generic_bs_rm_4),	\
	.bs_rm_8 = bs_notimpl_bs_rm_8,		\
						\
	/* read region */			\
	.bs_rr_1 = generic_bs_rr_1,		\
	.bs_rr_2 = NSWAP(generic_armv4_bs_rr_2),\
	.bs_rr_4 = NSWAP(generic_bs_rr_4),	\
	.bs_rr_8 = bs_notimpl_bs_rr_8,		\
						\
	/* write (single) */			\
	.bs_w_1 = generic_bs_w_1,		\
	.bs_w_2 = NSWAP(generic_armv4_bs_w_2),	\
	.bs_w_4 = NSWAP(generic_bs_w_4),	\
	.bs_w_8 = bs_notimpl_bs_w_8,		\
						\
	/* write multiple */			\
	.bs_wm_1 = generic_bs_wm_1,		\
	.bs_wm_2 = NSWAP(generic_armv4_bs_wm_2),\
	.bs_wm_4 = NSWAP(generic_bs_wm_4),	\
	.bs_wm_8 = bs_notimpl_bs_wm_8,		\
						\
	/* write region */			\
	.bs_wr_1 = generic_bs_wr_1,		\
	.bs_wr_2 = NSWAP(generic_armv4_bs_wr_2),\
	.bs_wr_4 = NSWAP(generic_bs_wr_4),	\
	.bs_wr_8 = bs_notimpl_bs_wr_8

#define MVSOC_BUS_SPACE_STREAM_FUNCS		\
	/* read stream (single) */		\
	.bs_r_1_s = generic_bs_r_1,		\
	.bs_r_2_s = NSWAP(generic_armv4_bs_r_2),\
	.bs_r_4_s = NSWAP(generic_bs_r_4),	\
	.bs_r_8_s = bs_notimpl_bs_r_8,		\
						\
	/* read multiple stream */		\
	.bs_rm_1_s = generic_bs_rm_1,		\
	.bs_rm_2_s = NSWAP(generic_armv4_bs_rm_2),\
	.bs_rm_4_s = NSWAP(generic_bs_rm_4),	\
	.bs_rm_8_s = bs_notimpl_bs_rm_8,	\
						\
	/* read region stream */		\
	.bs_rr_1_s = generic_bs_rr_1,		\
	.bs_rr_2_s = NSWAP(generic_armv4_bs_rr_2),\
	.bs_rr_4_s = NSWAP(generic_bs_rr_4),	\
	.bs_rr_8_s = bs_notimpl_bs_rr_8,	\
						\
	/* write stream (single) */		\
	.bs_w_1_s = generic_bs_w_1,		\
	.bs_w_2_s = NSWAP(generic_armv4_bs_w_2),\
	.bs_w_4_s = NSWAP(generic_bs_w_4,	\
	.bs_w_8_s = bs_notimpl_bs_w_8,		\
						\
	/* write multiple stream */		\
	.bs_wm_1_s = generic_bs_wm_1,		\
	.bs_wm_2_s = NSWAP(generic_armv4_bs_wm_2),\
	.bs_wm_4_s = NSWAP(generic_bs_wm_4),	\
	.bs_wm_8_s = bs_notimpl_bs_wm_8,	\
						\
	/* write region stream */		\
	.bs_wr_1_s = generic_bs_wr_1,		\
	.bs_wr_2_s = NSWAP(generic_armv4_bs_wr_2),\
	.bs_wr_4_s = NSWAP(generic_bs_wr_4),	\
	.bs_wr_8_s = bs_notimpl_bs_wr_8

#define MVSOC_BUS_SPACE_DEFAULT_FUNCS		\
	/* mapping/unmapping */			\
	.bs_map = mvsoc_bs_map,			\
	.bs_unmap = mvsoc_bs_unmap,		\
	.bs_subregion = mvsoc_bs_subregion,	\
						\
	/* allocation/deallocation */		\
	.bs_alloc = mvsoc_bs_alloc,		\
	.bs_free = mvsoc_bs_free,		\
						\
	/* get kernel virtual address */	\
	.bs_vaddr = mvsoc_bs_vaddr,		\
						\
	/* mmap bus space for userland */	\
	.bs_mmap = bs_notimpl_bs_mmap,		\
						\
	/* barrier */				\
	.bs_barrier = mvsoc_bs_barrier,		\
						\
	MVSOC_BUS_SPACE_NORMAL_FUNCS,		\
						\
	/* set multiple */			\
	.bs_sm_1 = bs_notimpl_bs_sm_1,		\
	.bs_sm_2 = bs_notimpl_bs_sm_2,		\
	.bs_sm_4 = bs_notimpl_bs_sm_4,		\
	.bs_sm_8 = bs_notimpl_bs_sm_8,		\
						\
	/* set region */			\
	.bs_sr_1 = bs_notimpl_bs_sr_1,		\
	.bs_sr_2 = NSWAP(generic_armv4_bs_sr_2),\
	.bs_sr_4 = NSWAP(generic_bs_sr_4),	\
	.bs_sr_8 = bs_notimpl_bs_sr_8,		\
						\
	/* copy */				\
	.bs_c_1 = bs_notimpl_bs_c_1,		\
	.bs_c_2 = generic_armv4_bs_c_2,		\
	.bs_c_4 = bs_notimpl_bs_c_4,		\
	.bs_c_8 = bs_notimpl_bs_c_8


struct bus_space mvsoc_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)0,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};

#if NMVPEX > 0
#if defined(ORION)
struct bus_space orion_pex0_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ORION_TAG_PEX0_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
struct bus_space orion_pex0_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ORION_TAG_PEX0_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
struct bus_space orion_pex1_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ORION_TAG_PEX1_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
struct bus_space orion_pex1_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ORION_TAG_PEX1_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
#endif

#if defined(KIRKWOOD)
struct bus_space kirkwood_pex_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)KIRKWOOD_TAG_PEX_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
struct bus_space kirkwood_pex_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)KIRKWOOD_TAG_PEX_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
struct bus_space kirkwood_pex1_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)KIRKWOOD_TAG_PEX1_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
struct bus_space kirkwood_pex1_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)KIRKWOOD_TAG_PEX1_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
#endif

#if defined(DOVE)
struct bus_space dove_pex0_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)DOVE_TAG_PEX0_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
struct bus_space dove_pex0_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)DOVE_TAG_PEX0_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
struct bus_space dove_pex1_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)DOVE_TAG_PEX1_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
struct bus_space dove_pex1_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)DOVE_TAG_PEX1_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
#endif

#if defined(ARMADAXP)
struct bus_space armadaxp_pex00_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX00_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space armadaxp_pex00_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX00_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space armadaxp_pex01_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX01_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space armadaxp_pex01_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX01_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space armadaxp_pex02_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX02_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space armadaxp_pex02_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX02_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space armadaxp_pex03_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX03_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space armadaxp_pex03_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX03_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space armadaxp_pex10_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX10_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space armadaxp_pex10_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX10_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space armadaxp_pex2_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX2_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space armadaxp_pex2_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX2_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space armadaxp_pex3_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX3_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
struct bus_space armadaxp_pex3_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ARMADAXP_TAG_PEX3_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS
};
#endif
#endif

#if NGTPCI > 0
#if defined(ORION)
struct bus_space orion_pci_mem_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ORION_TAG_PCI_MEM,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
struct bus_space orion_pci_io_bs_tag = {
	/* cookie */
	.bs_cookie = (void *)ORION_TAG_PCI_IO,

	MVSOC_BUS_SPACE_DEFAULT_FUNCS,
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	MVSOC_BUS_SPACE_NORMAL_FUNCS,
#endif
};
#endif
#endif


int
mvsoc_bs_map(void *space, bus_addr_t address, bus_size_t size, int flags,
	     bus_space_handle_t *handlep)
{
	const struct pmap_devmap *pd;
	paddr_t startpa, endpa, offset, pa;
	vaddr_t va;

/*
 * XXX: We are not configuring any decode windows for Armada XP
 * 	at the moment. We rely on those that have been set by u-boot.
 *	Hence we don't want to mess around with decode windows,
 *	till we get full control over them.
 */

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

	const int pmapflags =
	    (flags & (BUS_SPACE_MAP_CACHEABLE|BUS_SPACE_MAP_PREFETCHABLE))
		? 0
		: PMAP_NOCACHE;

	/* Now map the pages */
	for (pa = startpa; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, pmapflags);
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
