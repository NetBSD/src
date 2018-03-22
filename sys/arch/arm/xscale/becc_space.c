/*	$NetBSD: becc_space.c,v 1.5.52.1 2018/03/22 01:44:44 pgoyette Exp $	*/

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
 * bus_space functions for the ADI Engineering Big Endian Companion Chip.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: becc_space.c,v 1.5.52.1 2018/03/22 01:44:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>

#include <arm/xscale/beccreg.h>
#include <arm/xscale/beccvar.h>

/* Prototypes for all the bus_space structure functions */
bs_protos(becc);
bs_protos(becc_io);
bs_protos(becc_mem);
bs_protos(becc_pci);
bs_protos(bs_notimpl);

/*
 * Template bus_space -- copied, and the bits that are NULL are
 * filled in.
 */
const struct bus_space becc_bs_tag_template = {
	/* cookie */
	.bs_cookie = (void *) 0,

	/* mapping/unmapping */
	.bs_map = NULL,
	.bs_unmap = NULL,
	.bs_subregion = becc_bs_subregion,

	/* allocation/deallocation */
	.bs_alloc = NULL,
	.bs_free = NULL,

	/* get kernel virtual address */
	.bs_vaddr = becc_bs_vaddr,

	/* mmap */
	.bs_mmap = becc_bs_mmap,

	/* barrier */
	.bs_barrier = becc_bs_barrier,

	/* read (single) */
	.bs_r_1 = becc_pci_bs_r_1,
	.bs_r_2 = becc_pci_bs_r_2,
	.bs_r_4 = becc_pci_bs_r_4,
	.bs_r_8 = bs_notimpl_bs_r_8,

	/* read multiple */
	.bs_rm_1 = bs_notimpl_bs_rm_1,
	.bs_rm_2 = bs_notimpl_bs_rm_2,
	.bs_rm_4 = bs_notimpl_bs_rm_4,
	.bs_rm_8 = bs_notimpl_bs_rm_8,

	/* read region */
	.bs_rr_1 = bs_notimpl_bs_rr_1,
	.bs_rr_2 = bs_notimpl_bs_rr_2,
	.bs_rr_4 = bs_notimpl_bs_rr_4,
	.bs_rr_8 = bs_notimpl_bs_rr_8,

	/* write (single) */
	.bs_w_1 = becc_pci_bs_w_1,
	.bs_w_2 = becc_pci_bs_w_2,
	.bs_w_4 = becc_pci_bs_w_4,
	.bs_w_8 = bs_notimpl_bs_w_8,

	/* write multiple */
	.bs_wm_1 = bs_notimpl_bs_wm_1,
	.bs_wm_2 = bs_notimpl_bs_wm_2,
	.bs_wm_4 = bs_notimpl_bs_wm_4,
	.bs_wm_8 = bs_notimpl_bs_wm_8,

	/* write region */
	.bs_wr_1 = bs_notimpl_bs_wr_1,
	.bs_wr_2 = bs_notimpl_bs_wr_2,
	.bs_wr_4 = bs_notimpl_bs_wr_4,
	.bs_wr_8 = bs_notimpl_bs_wr_8,

	/* set multiple */
	.bs_sm_1 = bs_notimpl_bs_sm_1,
	.bs_sm_2 = bs_notimpl_bs_sm_2,
	.bs_sm_4 = bs_notimpl_bs_sm_4,
	.bs_sm_8 = bs_notimpl_bs_sm_8,

	/* set region */
	.bs_sr_1 = bs_notimpl_bs_sr_1,
	.bs_sr_2 = bs_notimpl_bs_sr_2,
	.bs_sr_4 = bs_notimpl_bs_sr_4,
	.bs_sr_8 = bs_notimpl_bs_sr_8,

	/* copy */
	.bs_c_1 = bs_notimpl_bs_c_1,
	.bs_c_2 = bs_notimpl_bs_c_2,
	.bs_c_4 = bs_notimpl_bs_c_4,
	.bs_c_8 = bs_notimpl_bs_c_8,
};

void
becc_io_bs_init(bus_space_tag_t bs, void *cookie)
{

	*bs = becc_bs_tag_template;
	bs->bs_cookie = cookie;

	bs->bs_map = becc_io_bs_map;
	bs->bs_unmap = becc_io_bs_unmap;
	bs->bs_alloc = becc_io_bs_alloc;
	bs->bs_free = becc_io_bs_free;

	bs->bs_vaddr = becc_io_bs_vaddr;
}

void
becc_mem_bs_init(bus_space_tag_t bs, void *cookie)
{

	*bs = becc_bs_tag_template;
	bs->bs_cookie = cookie;

	bs->bs_map = becc_mem_bs_map;
	bs->bs_unmap = becc_mem_bs_unmap;
	bs->bs_alloc = becc_mem_bs_alloc;
	bs->bs_free = becc_mem_bs_free;

	bs->bs_mmap = becc_mem_bs_mmap;
}

/* *** Routines shared by becc, PCI IO, and PCI MEM. *** */

int
becc_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return (0);
}

void
becc_bs_barrier(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t len, int flags)
{

	/* Nothing to do. */
}

void *
becc_bs_vaddr(void *t, bus_space_handle_t bsh)
{

	return ((void *)bsh);
}

paddr_t
becc_bs_mmap(void *t, bus_addr_t addr, off_t off, int prot, int flags)
{

	/* Not supported. */
	return (-1);
}

/* *** Routines for PCI IO. *** */

int
becc_io_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp)
{
	struct becc_softc *sc = t;
	vaddr_t winvaddr;
	uint32_t busbase;

	if (bpa >= sc->sc_ioout_xlate &&
	    bpa < (sc->sc_ioout_xlate + (64 * 1024))) {
		busbase = sc->sc_ioout_xlate;
		winvaddr = sc->sc_pci_io_base;
	} else
		return (EINVAL);

	if ((bpa + size) >= (busbase + (64 * 1024)))
		return (EINVAL);

	/*
	 * Found the window -- PCI I/O space is mapped at a fixed
	 * virtual address by board-specific code.  Translate the
	 * bus address to the virtual address.
	 */
	*bshp = winvaddr + (bpa - busbase);

	return (0);
}

void
becc_io_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{

	/* Nothing to do. */
}

int
becc_io_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{

	panic("becc_io_bs_alloc(): not implemented\n");
}

void    
becc_io_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{

	panic("becc_io_bs_free(): not implemented\n");
}

void *
becc_io_bs_vaddr(void *t, bus_space_handle_t bsh)
{

	/* Not supported. */
	return (NULL);
}

/* *** Routines for PCI MEM. *** */

int
becc_mem_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp)
{

	struct becc_softc *sc = t;
	vaddr_t winvaddr;
	uint32_t busbase;

	/*
	 * The two memory windows have been configured to the same
	 * PCI base by board-specific code, so we only need to look
	 * at the first one.
	 */

	if (bpa >= sc->sc_owin_xlate[0] &&
	    bpa < (sc->sc_owin_xlate[0] + BECC_PCI_MEM1_SIZE)) {
		busbase = sc->sc_owin_xlate[0];
		winvaddr = sc->sc_pci_mem_base[0];
	} else
		return (EINVAL);

	if ((bpa + size) >= (busbase + BECC_PCI_MEM1_SIZE))
		return (EINVAL);

	/*
	 * Found the window -- PCI MEM space is mapped at a fixed
	 * virtual address by board-specific code.  Translate the
	 * bus address to the virtual address.
	 */
	*bshp = winvaddr + (bpa - busbase);

	return (0);
}

void
becc_mem_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{

	/* Nothing to do. */
}

int
becc_mem_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{

	panic("becc_mem_bs_alloc(): not implemented\n");
}

void    
becc_mem_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{

	panic("becc_mem_bs_free(): not implemented\n");
}

paddr_t
becc_mem_bs_mmap(void *t, bus_addr_t addr, off_t off, int prot, int flags)
{

	/* XXX */
	return (-1);
}
