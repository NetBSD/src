/*	$NetBSD: bus_mem.c,v 1.2 1999/01/01 21:43:19 ragge Exp $ */
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed at Ludd, University of 
 *	Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/bus.h>
#include <machine/intr.h>

static int
vax_mem_add_mapping(
	bus_addr_t bpa,
	bus_size_t size,
	int cacheable,
	bus_space_handle_t *bshp)
{
	u_long pa, endpa;
	vm_offset_t va;

	pa = trunc_page(bpa);
	endpa = round_page(bpa + size);

#if defined(UVM)
	va = uvm_km_valloc(kernel_map, endpa - pa);
#else
	va = kmem_alloc_pageable(kernel_map, endpa - pa);
#endif
	if (va == 0)
		return (ENOMEM);

	*bshp = (bus_space_handle_t)(va + (bpa & VAX_PGOFSET));

	for (; pa < endpa; pa += VAX_NBPG, va += VAX_NBPG) {
		pmap_enter(pmap_kernel(), va, pa,
		    VM_PROT_READ | VM_PROT_WRITE, TRUE);
	}

	return 0;   
} 

static int
vax_mem_bus_space_map(
	void *t,
	bus_addr_t a,
	bus_size_t s,
	int f,
	bus_space_handle_t *hp,
	int f2)
{
	return vax_mem_add_mapping(a, s, (f & BUS_SPACE_MAP_CACHEABLE), hp);
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
#if defined(UVM)
	uvm_km_free(kernel_map, va, endva - va);
#else           
	kmem_free(kernel_map, va, endva - va);
#endif
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
	
static const struct vax_bus_space vax_mem_bus_space = {
	NULL,
	vax_mem_bus_space_map,
	vax_mem_bus_space_unmap,
	vax_mem_bus_space_subregion,
	vax_mem_bus_space_alloc,
	vax_mem_bus_space_free
};
