/*	$NetBSD: ixp12x0_io.c,v 1.6 2003/02/17 20:51:52 ichiro Exp $ */

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

/*
 * bus_space I/O functions for ixp12x0
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/queue.h>

#include <uvm/uvm.h>

#include <machine/bus.h>

#include <arm/ixp12x0/ixp12x0reg.h>
#include <arm/ixp12x0/ixp12x0var.h>

/* Proto types for all the bus_space structure functions */
bs_protos(ixp12x0);
bs_protos(ixp12x0_io);
bs_protos(ixp12x0_mem);
bs_protos(generic);
bs_protos(generic_armv4);
bs_protos(bs_notimpl);

struct bus_space ixp12x0_bs_tag = {
	/* cookie */
	(void *) 0,

	/* mapping/unmapping */
	NULL,
	NULL,
	ixp12x0_bs_subregion,

	/* allocation/deallocation */
	NULL,
	NULL,

	/* get kernel virtual address */
	ixp12x0_bs_vaddr,

	/* mmap bus space for userland */
	ixp12x0_bs_mmap,

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
ixp12x0_bs_init(bs, cookie)
	bus_space_tag_t bs;
	void *cookie;
{
	*bs = ixp12x0_bs_tag;
	bs->bs_cookie = cookie;
}

void
ixp12x0_io_bs_init(bs, cookie)
	bus_space_tag_t bs;
	void *cookie;
{
	*bs = ixp12x0_bs_tag;
	bs->bs_cookie = cookie;

	bs->bs_map = ixp12x0_io_bs_map;
	bs->bs_unmap = ixp12x0_io_bs_unmap;
	bs->bs_alloc = ixp12x0_io_bs_alloc;
	bs->bs_free = ixp12x0_io_bs_free;

	bs->bs_vaddr = ixp12x0_io_bs_vaddr;
}
void
ixp12x0_mem_bs_init(bs, cookie)
	bus_space_tag_t bs;
	void *cookie;
{
	*bs = ixp12x0_bs_tag;
	bs->bs_cookie = cookie;

	bs->bs_map = ixp12x0_mem_bs_map;
	bs->bs_unmap = ixp12x0_mem_bs_unmap;
	bs->bs_alloc = ixp12x0_mem_bs_alloc;
	bs->bs_free = ixp12x0_mem_bs_free;

	bs->bs_mmap = ixp12x0_mem_bs_mmap;
}

/* mem bus space functions */

int
ixp12x0_mem_bs_map(t, bpa, size, cacheable, bshp)
	void *t;
	bus_addr_t bpa;
	bus_size_t size;
	int cacheable;
	bus_space_handle_t *bshp;
{
	paddr_t pa, endpa;
	vaddr_t va;

	if ((bpa + size) >= IXP12X0_PCI_MEM_VBASE + IXP12X0_PCI_MEM_SIZE)
		return (EINVAL);
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
ixp12x0_mem_bs_unmap(t, bsh, size)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	vaddr_t startva, endva;

	startva = trunc_page(bsh);
	endva = round_page(bsh + size);

	uvm_km_free(kernel_map, startva, endva - startva);
}

int
ixp12x0_mem_bs_alloc(t, rstart, rend, size, alignment, boundary, cacheable,
    bpap, bshp)
	void *t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int cacheable;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	panic("ixp12x0_mem_bs_alloc(): Help!");
}

void
ixp12x0_mem_bs_free(t, bsh, size)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	panic("ixp12x0_mem_bs_free(): Help!");
}

paddr_t
ixp12x0_mem_bs_mmap(t, addr, off, prot, flags)
	void *t;
	bus_addr_t addr;
	off_t off;
	int prot;
	int flags;
{
	/* Not supported. */
	return (-1);
}

/* I/O bus space functions */

int
ixp12x0_io_bs_map(t, bpa, size, cacheable, bshp)
	void *t;
	bus_addr_t bpa;
	bus_size_t size;
	int cacheable;
	bus_space_handle_t *bshp;
{
	if ((bpa + size) >= IXP12X0_PCI_IO_SIZE)
		return (EINVAL);

	/*
	 * PCI I/O space is mapped at virtual address of each evaluation board.
	 * Translate the bus address(0x0) to the virtual address(0x54000000).
	 */
	*bshp = bpa + IXP12X0_PCI_IO_VBASE;

	return(0);
}

void
ixp12x0_io_bs_unmap(t, bsh, size)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	/*
	 * Temporary implementation
	 */
}

int
ixp12x0_io_bs_alloc(t, rstart, rend, size, alignment, boundary, cacheable,
    bpap, bshp)
	void *t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int cacheable;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	panic("ixp12x0_io_bs_alloc(): Help!");
}

void    
ixp12x0_io_bs_free(t, bsh, size)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	panic("ixp12x0_io_bs_free(): Help!");
}

void *
ixp12x0_io_bs_vaddr(t, bsh)
        void *t;
        bus_space_handle_t bsh;
{       
	/* Not supported. */
	return (NULL);
}


/* Common routines */

int
ixp12x0_bs_subregion(t, bsh, offset, size, nbshp)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t offset, size;
	bus_space_handle_t *nbshp;
{

	*nbshp = bsh + offset;
	return (0);
}

void *
ixp12x0_bs_vaddr(t, bsh)
	void *t;
	bus_space_handle_t bsh;
{
	return ((void *)bsh);
}

paddr_t
ixp12x0_bs_mmap(t, addr, off, prot, flags)
	void *t;
	bus_addr_t addr;
	off_t off;
	int prot;
	int flags;
{
	/* Not supported. */
	return (-1);
}

void
ixp12x0_bs_barrier(t, bsh, offset, len, flags)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t offset, len;
	int flags;
{
/* NULL */
}	



/* End of ixp12x0_io.c */
