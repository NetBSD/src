/*	$NetBSD: bus_space_simple.c,v 1.4 2026/04/26 17:43:41 thorpej Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford and Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: bus_space_simple.c,v 1.4 2026/04/26 17:43:41 thorpej Exp $");

#define _M68K_BUS_SPACE_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/bus.h>

paddr_t	_bus_space_mmap_default(void *, bus_addr_t, off_t, int, int);

struct m68k_simple_bus_space_tag m68k_simple_bus_space = {
	NULL,
	_bus_space_map,
	_bus_space_unmap,
	_bus_space_mmap,
	_bus_space_peek_1,
	_bus_space_peek_2,
	_bus_space_peek_4,
	_bus_space_poke_1,
	_bus_space_poke_2,
	_bus_space_poke_4,
};

/* ARGSUSED */
int
_bus_space_map(void *cookie, bus_addr_t addr, bus_size_t size, int flags,
    bus_space_handle_t *bushp)
{
	vsize_t offset;
	vaddr_t va;
	int map_flags;

	if (pmap_pa_has_static_mapping(addr, size,
				       UVM_PROT_READ|UVM_PROT_WRITE,
				       &va, &map_flags)) {
		*bushp = (bus_space_handle_t)va;
		return 0;
	}

	/*
	 * Allocate virtual space from the kernel_map.
	 *
	 * XXX Provide a hook for the platform to supply a
	 * XXX vmem arena for the space.  That would be needed
	 * XXX in situations where non-static mappings are
	 * XXX needed before UVM is initialized (e.g. frame
	 * XXX buffer console devices).
	 */
	offset = m68k_page_offset(addr);
	size = m68k_round_page(offset + size);
	addr = m68k_trunc_page(addr);

	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY);
	if (va == 0) {
		return ENOMEM;
	}

	*bushp = (bus_space_handle_t)(va + offset);

	while (size != 0) {
		pmap_kenter_pa(va, addr, VM_PROT_READ | VM_PROT_WRITE,
		    PMAP_NOCACHE);
		va += PAGE_SIZE;
		addr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	return 0;
}

/* ARGSUSED */
void
_bus_space_unmap(void *cookie, bus_space_handle_t bush, bus_size_t size)
{
	vaddr_t va;
	vsize_t offset;

	if (pmap_va_is_static_mapping((vaddr_t)bush, size)) {
		return;
	}

	va = m68k_trunc_page(bush);
	offset = m68k_page_offset(bush);
	size = m68k_round_page(offset + size);

	pmap_kremove(va, size);

	/* XXX vmem arena hook */

	uvm_km_free(kernel_map, va, size, UVM_KMF_VAONLY);
}

paddr_t
_bus_space_mmap_default(void *cookie, bus_addr_t addr, off_t offset,
    int prot, int flags)
{
	/*
	 * The default is to fail this operation.  If a platform wants
	 * specific behavior, they simply need to provide a _bus_space_mmap()
	 * function.
	 */
	return (paddr_t)-1;
}
__weak_alias(_bus_space_mmap,_bus_space_mmap_default);

/* ARGSUSED */
int
_bus_space_peek_1(void *cookie, bus_space_handle_t bush, bus_size_t offset,
    uint8_t *valuep)
{
	return badaddr_read((void *)(bush + offset), 1, valuep);
}

/* ARGSUSED */
int
_bus_space_peek_2(void *cookie, bus_space_handle_t bush, bus_size_t offset,
    uint16_t *valuep)
{
	return badaddr_read((void *)(bush + offset), 2, valuep);
}

/* ARGSUSED */
int
_bus_space_peek_4(void *cookie, bus_space_handle_t bush, bus_size_t offset,
    uint32_t *valuep)
{
	return badaddr_read((void *)(bush + offset), 4, valuep);
}

/* ARGSUSED */
int
_bus_space_poke_1(void *cookie, bus_space_handle_t bush, bus_size_t offset,
    uint8_t value)
{
	return badaddr_write((void *)(bush + offset), 1, value);
}

/* ARGSUSED */
int
_bus_space_poke_2(void *cookie, bus_space_handle_t bush, bus_size_t offset,
    uint16_t value)
{
	return badaddr_write((void *)(bush + offset), 2, value);
}

/* ARGSUSED */
int
_bus_space_poke_4(void *cookie, bus_space_handle_t bush, bus_size_t offset,
    uint32_t value)
{
	return badaddr_write((void *)(bush + offset), 4, value);
}
