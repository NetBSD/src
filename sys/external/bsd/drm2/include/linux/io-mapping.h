/*	$NetBSD: io-mapping.h,v 1.2.10.3 2017/12/03 11:37:59 jdolecek Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_IO_MAPPING_H_
#define _LINUX_IO_MAPPING_H_

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/mman.h>

#include <uvm/uvm_extern.h>

struct io_mapping {
	bus_space_tag_t		diom_bst;
	bus_addr_t		diom_addr;
	bus_size_t		diom_size;
	vaddr_t			diom_va;
	bool			diom_mapped;
};

static inline struct io_mapping *
bus_space_io_mapping_create_wc(bus_space_tag_t bst, bus_addr_t addr,
    bus_size_t size)
{
	struct io_mapping *mapping;
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
			return NULL;
	}

	/* Create a mapping record.  */
	mapping = kmem_alloc(sizeof(*mapping), KM_SLEEP);
	mapping->diom_bst = bst;
	mapping->diom_addr = addr;
	mapping->diom_size = size;
	mapping->diom_mapped = false;

	/* Allocate kva for one page.  */
	mapping->diom_va = uvm_km_alloc(kernel_map, PAGE_SIZE, PAGE_SIZE,
	    UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	KASSERT(mapping->diom_va != 0);

	return mapping;
}

static inline void
io_mapping_free(struct io_mapping *mapping)
{

	KASSERT(!mapping->diom_mapped);

	uvm_km_free(kernel_map, mapping->diom_va, PAGE_SIZE, UVM_KMF_VAONLY);
	kmem_free(mapping, sizeof(*mapping));
}

static inline void *
io_mapping_map_wc(struct io_mapping *mapping, unsigned long offset)
{
	paddr_t cookie;

	KASSERT(0 == (offset & (PAGE_SIZE - 1)));
	KASSERT(PAGE_SIZE <= mapping->diom_size);
	KASSERT(offset <= (mapping->diom_size - PAGE_SIZE));
	KASSERT(__type_fit(off_t, offset));
	KASSERT(!mapping->diom_mapped);

	cookie = bus_space_mmap(mapping->diom_bst, mapping->diom_addr, offset,
	    PROT_READ|PROT_WRITE,
	    BUS_SPACE_MAP_LINEAR|BUS_SPACE_MAP_PREFETCHABLE);
	KASSERT(cookie != (paddr_t)-1);

	pmap_kenter_pa(mapping->diom_va, pmap_phys_address(cookie),
	    PROT_READ|PROT_WRITE, pmap_mmap_flags(cookie));
	pmap_update(pmap_kernel());

	mapping->diom_mapped = true;
	return (void *)mapping->diom_va;
}

static inline void
io_mapping_unmap(struct io_mapping *mapping, void *ptr __diagused)
{

	KASSERT(mapping->diom_mapped);
	KASSERT(mapping->diom_va == (vaddr_t)ptr);

	pmap_kremove(mapping->diom_va, PAGE_SIZE);
	pmap_update(pmap_kernel());

	mapping->diom_mapped = false;
}

static inline void *
io_mapping_map_atomic_wc(struct io_mapping *mapping, unsigned long offset)
{

	return io_mapping_map_wc(mapping, offset);
}

static inline void
io_mapping_unmap_atomic(struct io_mapping *mapping, void *ptr)
{

	io_mapping_unmap(mapping, ptr);
}

#endif  /* _LINUX_IO_MAPPING_H_ */
