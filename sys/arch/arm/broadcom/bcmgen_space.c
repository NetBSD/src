/*	$NetBSD: bcmgen_space.c,v 1.1.2.3 2017/12/03 11:35:52 jdolecek Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcmgen_space.c,v 1.1.2.3 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>
#include <sys/endian.h>

/* Prototypes for all the bus_space structure functions */
bs_protos(bcmgen);
bs_protos(generic);
bs_protos(generic_armv4);
bs_protos(bs_notimpl);

#if BYTE_ORDER == BIG_ENDIAN
#define NSWAP(n)	n ## _swap
#else
#define NSWAP(n)	n
#endif

struct bus_space bcmgen_bs_tag = {
	/* cookie */
	(void *) 0,

	/* mapping/unmapping */
	bcmgen_bs_map,
	bcmgen_bs_unmap,
	bcmgen_bs_subregion,

	/* allocation/deallocation */
	bcmgen_bs_alloc,	/* not implemented */
	bcmgen_bs_free,	/* not implemented */

	/* get kernel virtual address */
	bcmgen_bs_vaddr,

	/* mmap */
	bs_notimpl_bs_mmap,

	/* barrier */
	bcmgen_bs_barrier,

	/* read (single) */
	generic_bs_r_1,
	NSWAP(generic_armv4_bs_r_2),
	NSWAP(generic_bs_r_4),
	bs_notimpl_bs_r_8,

	/* read multiple */
	generic_bs_rm_1,
	NSWAP(generic_armv4_bs_rm_2),
	NSWAP(generic_bs_rm_4),
	bs_notimpl_bs_rm_8,

	/* read region */
	generic_bs_rr_1,
	NSWAP(generic_armv4_bs_rr_2),
	NSWAP(generic_bs_rr_4),
	bs_notimpl_bs_rr_8,

	/* write (single) */
	generic_bs_w_1,
	NSWAP(generic_armv4_bs_w_2),
	NSWAP(generic_bs_w_4),
	bs_notimpl_bs_w_8,

	/* write multiple */
	generic_bs_wm_1,
	NSWAP(generic_armv4_bs_wm_2),
	NSWAP(generic_bs_wm_4),
	bs_notimpl_bs_wm_8,

	/* write region */
	generic_bs_wr_1,
	NSWAP(generic_armv4_bs_wr_2),
	NSWAP(generic_bs_wr_4),
	bs_notimpl_bs_wr_8,

	/* set multiple */
	bs_notimpl_bs_sm_1,
	bs_notimpl_bs_sm_2,
	bs_notimpl_bs_sm_4,
	bs_notimpl_bs_sm_8,

	/* set region */
	generic_bs_sr_1,
	NSWAP(generic_armv4_bs_sr_2),
	bs_notimpl_bs_sr_4,
	bs_notimpl_bs_sr_8,

	/* copy */
	bs_notimpl_bs_c_1,
	generic_armv4_bs_c_2,
	bs_notimpl_bs_c_4,
	bs_notimpl_bs_c_8,

#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	/* read (single) */
	generic_bs_r_1,
	NSWAP(generic_armv4_bs_r_2),
	NSWAP(generic_bs_r_4),
	bs_notimpl_bs_r_8,

	/* read multiple */
	generic_bs_rm_1,
	NSWAP(generic_armv4_bs_rm_2),
	NSWAP(generic_bs_rm_4),
	bs_notimpl_bs_rm_8,

	/* read region */
	generic_bs_rr_1,
	NSWAP(generic_armv4_bs_rr_2),
	NSWAP(generic_bs_rr_4),
	bs_notimpl_bs_rr_8,

	/* write (single) */
	generic_bs_w_1,
	NSWAP(generic_armv4_bs_w_2),
	NSWAP(generic_bs_w_4),
	bs_notimpl_bs_w_8,

	/* write multiple */
	generic_bs_wm_1,
	NSWAP(generic_armv4_bs_wm_2),
	NSWAP(generic_bs_wm_4),
	bs_notimpl_bs_wm_8,

	/* write region */
	generic_bs_wr_1,
	NSWAP(generic_armv4_bs_wr_2),
	NSWAP(generic_bs_wr_4),
	bs_notimpl_bs_wr_8,
#endif
};

int
bcmgen_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int flag,
    bus_space_handle_t *bshp)
{
	u_long startpa, endpa, pa;
	const struct pmap_devmap *pd;
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
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT | UVM_KMF_COLORMATCH);
	if (!va)
		return ENOMEM;

	*bshp = (bus_space_handle_t)(va + (bpa - startpa));

	const int pmapflags =
	    (flag & (BUS_SPACE_MAP_CACHEABLE|BUS_SPACE_MAP_PREFETCHABLE))
		? 0
		: PMAP_NOCACHE;
	for (pa = startpa; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, pmapflags);
	}
	pmap_update(pmap_kernel());

	return 0;
}

void
bcmgen_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
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
bcmgen_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return (0);
}

void
bcmgen_bs_barrier(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t len, int flags)
{
	flags &= BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE;
	
	if (flags)
		arm_dsb();
}

void *
bcmgen_bs_vaddr(void *t, bus_space_handle_t bsh)
{

	return (void *)bsh;
}

paddr_t
bcmgen_bs_mmap(void *t, bus_addr_t bpa, off_t offset, int prot, int flags)
{
	paddr_t bus_flags = 0;

	if (flags & BUS_SPACE_MAP_PREFETCHABLE)
		bus_flags |= ARM32_MMAP_WRITECOMBINE;

	return (arm_btop(bpa + offset) | bus_flags);
}

int
bcmgen_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{

	panic("%s(): not implemented\n", __func__);
}

void
bcmgen_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{

	panic("%s(): not implemented\n", __func__);
}
