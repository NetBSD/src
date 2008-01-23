/*	$NetBSD: bus_private.h,v 1.9 2008/01/23 19:46:45 bouyer Exp $	*/

/*-
 * Copyright (c)2005 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <uvm/uvm_extern.h>
#include "opt_xen.h"

#define	_BUS_PHYS_TO_BUS(pa)	((bus_addr_t)xpmap_ptom(pa))
#define	_BUS_BUS_TO_PHYS(ba)	((paddr_t)xpmap_mtop(ba))
#define	_BUS_VIRT_TO_BUS(pm, va) _bus_virt_to_bus((pm), (va))
#define _BUS_BUS_TO_VM_PAGE(ba) (PHYS_TO_VM_PAGE(xpmap_mtop(ba)))
#define _BUS_PMAP_ENTER(pmap, va, ba, prot, flags) \
    pmap_enter(pmap, va, xpmap_mtop(ba), prot, flags)

static __inline bus_addr_t _bus_virt_to_bus(struct pmap *, vaddr_t);

static __inline bus_addr_t
_bus_virt_to_bus(struct pmap *pm, vaddr_t va)
{
	bus_addr_t ba;

	if (!pmap_extract_ma(pm, va, &ba)) {
		panic("_bus_virt_to_bus");
	}

	return ba;
}

/* we need our own bus_dmamem_alloc_range */
#define _BUS_DMAMEM_ALLOC_RANGE _xen_bus_dmamem_alloc_range
int _xen_bus_dmamem_alloc_range(bus_dma_tag_t, bus_size_t, bus_size_t,
	    bus_size_t, bus_dma_segment_t *, int, int *, int,
	    bus_addr_t, bus_addr_t);

/*
 * The higher machine address of our allocated range isn't know and can change
 * over time. Just assume it's the largest possible value.
 */
#if defined(_LP64) || defined(PAE)
#define _BUS_AVAIL_END ((bus_addr_t)0xffffffffffffffff)
#else
#define _BUS_AVAIL_END ((bus_addr_t)0xffffffff)
#endif

#include <x86/bus_private.h>
