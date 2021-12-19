/*	$NetBSD: linux_io_mapping.c,v 1.1 2021/12/19 12:28:04 riastradh Exp $	*/

/*-
 * Copyright (c) 2013-2021 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: linux_io_mapping.c,v 1.1 2021/12/19 12:28:04 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/mman.h>

#include <uvm/uvm_extern.h>

#include <linux/io-mapping.h>

bool
bus_space_io_mapping_init_wc(bus_space_tag_t bst, struct io_mapping *mapping,
    bus_addr_t addr, bus_size_t size)
{
	bus_size_t offset;

	KASSERT(PAGE_SIZE <= size);
	KASSERT(0 == (size & (PAGE_SIZE - 1)));
	KASSERT(__type_fit(off_t, size));

	/*
	 * XXX For x86: Reserve the region (bus_space_reserve) and set
	 * an MTRR to make it write-combining.  Doesn't matter if we
	 * have PAT and we use pmap_kenter_pa, but matters if we don't
	 * have PAT or if we later make this use direct map.
	 */

	/* Make sure the region is mappable.  */
	for (offset = 0; offset < size; offset += PAGE_SIZE) {
		if (bus_space_mmap(bst, addr, offset, PROT_READ|PROT_WRITE,
			BUS_SPACE_MAP_LINEAR|BUS_SPACE_MAP_PREFETCHABLE)
		    == (paddr_t)-1)
			return false;
	}

	/* Initialize the mapping record.  */
	mapping->diom_bst = bst;
	mapping->base = addr;
	mapping->size = size;
	mapping->diom_atomic = false;

	/* Allocate kva for one page.  */
	mapping->diom_va = uvm_km_alloc(kernel_map, PAGE_SIZE, PAGE_SIZE,
	    UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	KASSERT(mapping->diom_va != 0);

	return true;
}

void
io_mapping_fini(struct io_mapping *mapping)
{

	KASSERT(!mapping->diom_atomic);

	uvm_km_free(kernel_map, mapping->diom_va, PAGE_SIZE, UVM_KMF_VAONLY);
	mapping->diom_va = 0;	/* paranoia */
}

struct io_mapping *
bus_space_io_mapping_create_wc(bus_space_tag_t bst, bus_addr_t addr,
    bus_size_t size)
{
	struct io_mapping *mapping;

	mapping = kmem_alloc(sizeof(*mapping), KM_SLEEP);
	if (!bus_space_io_mapping_init_wc(bst, mapping, addr, size)) {
		kmem_free(mapping, sizeof(*mapping));
		return NULL;
	}

	return mapping;
}

void
io_mapping_free(struct io_mapping *mapping)
{

	io_mapping_fini(mapping);
	kmem_free(mapping, sizeof(*mapping));
}

void *
io_mapping_map_wc(struct io_mapping *mapping, bus_addr_t offset,
    bus_size_t size)
{
	bus_size_t pg, npgs = size >> PAGE_SHIFT;
	vaddr_t va;
	paddr_t cookie;

	KASSERT(0 == (offset & (PAGE_SIZE - 1)));
	KASSERT(PAGE_SIZE <= mapping->size);
	KASSERT(offset <= (mapping->size - PAGE_SIZE));
	KASSERT(__type_fit(off_t, offset));

	va = uvm_km_alloc(kernel_map, size, PAGE_SIZE,
	    UVM_KMF_VAONLY|UVM_KMF_WAITVA);
	KASSERT(va != mapping->diom_va);
	for (pg = 0; pg < npgs; pg++) {
		cookie = bus_space_mmap(mapping->diom_bst, mapping->base,
		    offset + pg*PAGE_SIZE,
		    PROT_READ|PROT_WRITE,
		    BUS_SPACE_MAP_LINEAR|BUS_SPACE_MAP_PREFETCHABLE);
		KASSERT(cookie != (paddr_t)-1);

		pmap_kenter_pa(va + pg*PAGE_SIZE, pmap_phys_address(cookie),
		    PROT_READ|PROT_WRITE, pmap_mmap_flags(cookie));
	}
	pmap_update(pmap_kernel());

	return (void *)va;
}

void
io_mapping_unmap(struct io_mapping *mapping, void *ptr, bus_size_t size)
{
	vaddr_t va = (vaddr_t)ptr;

	KASSERT(mapping->diom_va != va);

	pmap_kremove(va, size);
	pmap_update(pmap_kernel());

	uvm_km_free(kernel_map, va, size, UVM_KMF_VAONLY);
}

void *
io_mapping_map_atomic_wc(struct io_mapping *mapping, bus_addr_t offset)
{
	paddr_t cookie;

	KASSERT(0 == (offset & (PAGE_SIZE - 1)));
	KASSERT(PAGE_SIZE <= mapping->size);
	KASSERT(offset <= (mapping->size - PAGE_SIZE));
	KASSERT(__type_fit(off_t, offset));
	KASSERT(!mapping->diom_atomic);

	cookie = bus_space_mmap(mapping->diom_bst, mapping->base, offset,
	    PROT_READ|PROT_WRITE,
	    BUS_SPACE_MAP_LINEAR|BUS_SPACE_MAP_PREFETCHABLE);
	KASSERT(cookie != (paddr_t)-1);

	pmap_kenter_pa(mapping->diom_va, pmap_phys_address(cookie),
	    PROT_READ|PROT_WRITE, pmap_mmap_flags(cookie));
	pmap_update(pmap_kernel());

	mapping->diom_atomic = true;
	return (void *)mapping->diom_va;
}

void
io_mapping_unmap_atomic(struct io_mapping *mapping, void *ptr __diagused)
{

	KASSERT(mapping->diom_atomic);
	KASSERT(mapping->diom_va == (vaddr_t)ptr);

	pmap_kremove(mapping->diom_va, PAGE_SIZE);
	pmap_update(pmap_kernel());

	mapping->diom_atomic = false;
}
