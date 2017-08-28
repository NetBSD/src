/*	$NetBSD: bus_mem.c,v 1.13.36.1 2017/08/28 17:51:55 skrll Exp $ */
/*
 * Copyright (c) 1998 Matt Thomas
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by Bertram Barth.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_mem.c,v 1.13.36.1 2017/08/28 17:51:55 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/cpu.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>


static int
vax_mem_bus_space_map(
	void *t,
	bus_addr_t pa,
	bus_size_t size,
	int cacheable,
	bus_space_handle_t *bshp,
	int f2)
{
	vaddr_t va;

	size += (pa & VAX_PGOFSET);	/* have to include the byte offset */
	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (va == 0)
		return (ENOMEM);

	*bshp = (bus_space_handle_t)(va + (pa & VAX_PGOFSET));

	ioaccess(va, pa, (size + VAX_NBPG - 1) >> VAX_PGSHIFT);

	return 0;   
} 

static int
vax_mem_bus_space_subregion(
	void *t,
	bus_space_handle_t h,
	bus_size_t o,
	bus_size_t s,
	bus_space_handle_t *hp)
{
	*hp = h + o;
	return (0);             
}

static void
vax_mem_bus_space_unmap(
	void *t,
	bus_space_handle_t h,
	bus_size_t size,
	int f)
{
	u_long va = trunc_page(h);
	u_long endva = round_page(h + size);

        /* 
         * Free the kernel virtual mapping.
         */
	iounaccess(va, size >> VAX_PGSHIFT);
	uvm_km_free(kernel_map, va, endva - va, UVM_KMF_VAONLY);
}

static int
vax_mem_bus_space_alloc(
	void *t,
	bus_addr_t rs,
	bus_addr_t re,
	bus_size_t s,
	bus_size_t a,
	bus_size_t b,
	int f,
	bus_addr_t *ap,
	bus_space_handle_t *hp)
{
	panic("vax_mem_bus_alloc not implemented");
}

static void
vax_mem_bus_space_free(
	void *t,
	bus_space_handle_t h,
	bus_size_t s)
{    
	panic("vax_mem_bus_free not implemented");
}

static paddr_t
vax_mem_bus_space_mmap(void *v, bus_addr_t addr, off_t off, int prot, int flags)
{
	bus_addr_t rv;

	rv = addr + off;
	return btop(rv);
}

struct vax_bus_space vax_mem_bus_space = {
	NULL,
	vax_mem_bus_space_map,
	vax_mem_bus_space_unmap,
	vax_mem_bus_space_subregion,
	vax_mem_bus_space_alloc,
	vax_mem_bus_space_free,
	vax_mem_bus_space_mmap,
};
