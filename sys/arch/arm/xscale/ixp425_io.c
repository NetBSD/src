/*	$NetBSD: ixp425_io.c,v 1.2 2003/06/01 21:42:26 ichiro Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ixp425_io.c,v 1.2 2003/06/01 21:42:26 ichiro Exp $");

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
bs_protos(ixp425_io);
bs_protos(ixp425_mem);
bs_protos(generic);
bs_protos(generic_armv4);
bs_protos(bs_notimpl);

struct bus_space ixp425_bs_tag = {
	/* cookie */
	(void *) 0,

	/* mapping/unmapping */
	NULL,
	NULL,
	ixp425_bs_subregion,

	/* allocation/deallocation */
	NULL,
	NULL,

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

void
ixp425_bs_init(bus_space_tag_t bs, void *cookie)
{
	*bs = ixp425_bs_tag;
	bs->bs_cookie = cookie;
}

void
ixp425_io_bs_init(bus_space_tag_t bs, void *cookie)
{
	*bs = ixp425_bs_tag;
	bs->bs_cookie = cookie;

	bs->bs_map = ixp425_io_bs_map;
	bs->bs_unmap = ixp425_io_bs_unmap;
	bs->bs_alloc = ixp425_io_bs_alloc;
	bs->bs_free = ixp425_io_bs_free;

	bs->bs_vaddr = ixp425_io_bs_vaddr;
}
void
ixp425_mem_bs_init(bus_space_tag_t bs, void *cookie)
{
	*bs = ixp425_bs_tag;
	bs->bs_cookie = cookie;

	bs->bs_map = ixp425_mem_bs_map;
	bs->bs_unmap = ixp425_mem_bs_unmap;
	bs->bs_alloc = ixp425_mem_bs_alloc;
	bs->bs_free = ixp425_mem_bs_free;

	bs->bs_mmap = ixp425_mem_bs_mmap;
}

/* mem bus space functions */

int
ixp425_mem_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int cacheable,
    bus_space_handle_t *bshp)
{
	paddr_t pa, endpa;
	vaddr_t va;

#if 0
	if ((bpa + size) >= IXP425_PCI_MEM_VBASE + IXP425_PCI_MEM_SIZE)
		return (EINVAL);
#endif
	/*
	 * PCI MEM space is mapped same address as real memory
	 *  see. PCI_ADDR_EXT
	 */
	pa = trunc_page(bpa);
	endpa = round_page(bpa + size);

	/* Get some VM.  */
	va = uvm_km_valloc(kernel_map, endpa - pa);
	if (va == 0)
		return(ENOMEM);

	/* Store the bus space handle */
	*bshp = va + (bpa & PAGE_MASK);

	/* Now map the pages */
	for(; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_enter(pmap_kernel(), va, pa,
		    VM_PROT_READ | VM_PROT_WRITE,
		    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);
	}
	pmap_update(pmap_kernel());

	return(0);
}

void
ixp425_mem_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	vaddr_t startva, endva;

	startva = trunc_page(bsh);
	endva = round_page(bsh + size);

	uvm_km_free(kernel_map, startva, endva - startva);
}

int
ixp425_mem_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int cacheable,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	panic("ixp425_mem_bs_alloc(): Help!");
}

void
ixp425_mem_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	panic("ixp425_mem_bs_free(): Help!");
}

paddr_t
ixp425_mem_bs_mmap(void *t, bus_addr_t addr, off_t off, int prot, int flags)
{
	/* Not supported. */
	return (-1);
}

/* I/O bus space functions */

int
ixp425_io_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int cacheable,
    bus_space_handle_t *bshp)
{
	if ((bpa + size) >= IXP425_IO_VBASE)
		return (EINVAL);

	/*
	 * PCI I/O space is mapped at virtual address of each evaluation board.
	 * Translate the bus address(0x0) to the virtual address(0x54000000).
	 */
	*bshp = bpa + IXP425_IO_VBASE;

	return(0);
}

void
ixp425_io_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	/*
	 * Temporary implementation
	 */
}

int
ixp425_io_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int cacheable,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	panic("ixp425_io_bs_alloc(): Help!");
}

void    
ixp425_io_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	panic("ixp425_io_bs_free(): Help!");
}

void *
ixp425_io_bs_vaddr(void *t, bus_space_handle_t bsh)
{       
	/* Not supported. */
	return (NULL);
}


/* Common routines */

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

/* End of ixp425_io.c */
