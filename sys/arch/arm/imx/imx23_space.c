/* $Id: imx23_space.c,v 1.1.6.2 2013/02/25 00:28:28 tls Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

/*
 * bus_space(9) support for Freescale i.MX23 processor.
 */
#include <sys/bus.h>
#include <sys/mutex.h>
#include <uvm/uvm_prot.h>
#include <machine/pcb.h>
#include <uvm/uvm_pmap.h>
#include <machine/bus_defs.h>
#include <machine/pmap.h>

int	imx23_bs_map(void *, bus_addr_t, bus_size_t, int, bus_space_handle_t *);
void	imx23_bs_unmap(void *, bus_space_handle_t, bus_size_t);
int	imx23_bs_subregion(void *, bus_space_handle_t, bus_size_t, bus_size_t,
		bus_space_handle_t *);
int	imx23_bs_alloc(void *, bus_addr_t, bus_addr_t, bus_size_t, bus_size_t,
		bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
void	imx23_bs_free(void *, bus_space_handle_t, bus_size_t);
void *	imx23_bs_vaddr(void *, bus_space_handle_t);
paddr_t	imx23_bs_mmap(void *, bus_addr_t, off_t, int, int);
void	imx23_bs_barrier(void *, bus_space_handle_t, bus_size_t,
		bus_size_t, int);

bs_protos(bs_notimpl);
bs_protos(generic);
bs_protos(generic_armv4);

/* Describes bus space on i.MX23 */
struct bus_space imx23_bus_space = {
	/* cookie */
	(void *) 0,

	/* mapping/unmapping */
	imx23_bs_map,
	imx23_bs_unmap,
	imx23_bs_subregion,

	/* allocation/deallocation */
	imx23_bs_alloc,
	imx23_bs_free,

	/* get kernel virtual address */
	imx23_bs_vaddr,

	/* mmap bus space for user */
	imx23_bs_mmap,

	/* barrier */
	imx23_bs_barrier,

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
	generic_bs_sr_1,
	generic_armv4_bs_sr_2,
	generic_bs_sr_4,
	bs_notimpl_bs_sr_8,

	/* copy */
	bs_notimpl_bs_c_1,
	generic_armv4_bs_c_2,
	bs_notimpl_bs_c_4,
	bs_notimpl_bs_c_8
};

int
imx23_bs_map(void *space, bus_addr_t address, bus_size_t size,
	int flags, bus_space_handle_t *handlep)
{
	const struct pmap_devmap *pd;

	if ((pd = pmap_devmap_find_pa(address, size)) != NULL) {
		/* Device was statically mapped. */
		*handlep = pd->pd_va + (address - pd->pd_pa);
		return 0;
	}

	return 0;
}

void
imx23_bs_unmap(void *space, bus_space_handle_t handle, bus_size_t size)
{
	if (pmap_devmap_find_va(handle, size) != NULL) {
		/* Device was statically mapped. */
		return;
	}

	return;
}

int
imx23_bs_subregion(void *space, bus_space_handle_t handle,
	bus_size_t offset, bus_size_t size, bus_space_handle_t *nhandlep)
{
	*nhandlep = handle + offset;
	return 0;
}

int
imx23_bs_alloc(void *space, bus_addr_t reg_start, bus_addr_t reg_end,
	bus_size_t size, bus_size_t alignment,
	bus_size_t boundary, int flags, bus_addr_t *addrp,
	bus_space_handle_t *handlep)
{
	return 0;
}

void
imx23_bs_free(void *space, bus_space_handle_t handle, bus_size_t size)
{
	return;
}

void *
imx23_bs_vaddr(void *space, bus_space_handle_t handle)
{
	return (void *)0;
}

paddr_t
imx23_bs_mmap(void *space, bus_addr_t addr, off_t off, int prot,
	int flags)
{
	return (paddr_t)0;
}

void
imx23_bs_barrier(void *space, bus_space_handle_t handle,
	bus_size_t offset,
	bus_size_t length, int flags)
{
	return;
}
