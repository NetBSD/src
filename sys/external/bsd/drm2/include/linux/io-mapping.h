/*	$NetBSD: io-mapping.h,v 1.1.2.2 2013/07/24 03:17:48 riastradh Exp $	*/

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

#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/systm.h>

/*
 * XXX This uses NetBSD bus space reservations, which are currently
 * implemented only for x86.
 */

struct io_mapping {
	bus_space_tag_t		diom_bst;
	bus_addr_t		diom_addr;
	bus_addr_t		diom_size;
	int			diom_flags;
	bus_space_reservation_t	diom_bsr;
	bool			diom_mapped;
	bus_space_handle_t	diom_bsh;
};

static inline struct io_mapping *
bus_space_io_mapping_create_wc(bus_space_tag_t bst, bus_addr_t addr,
    bus_size_t size)
{
	struct io_mapping *const mapping = kmem_alloc(sizeof(*mapping),
	    KM_SLEEP);
	mapping->diom_bst = bst;
	mapping->diom_addr = addr;
	mapping->diom_size = size;
	mapping->diom_flags = 0;
	mapping->diom_flags |= BUS_SPACE_MAP_LINEAR;
	mapping->diom_flags |= BUS_SPACE_MAP_PREFETCHABLE;
	mapping->diom_mapped = false;

	if (bus_space_reserve(mapping->diom_bst, addr, size,
		mapping->diom_flags, &mapping->diom_bsr)) {
		kmem_free(mapping, sizeof(*mapping));
		return NULL;
	}

	return mapping;
}

static inline void
io_mapping_free(struct io_mapping *mapping)
{

	KASSERT(!mapping->diom_mapped);
	bus_space_release(mapping->diom_bst, &mapping->diom_bsr);
	kmem_free(mapping, sizeof(*mapping));
}

static inline void *
io_mapping_map_wc(struct io_mapping *mapping, unsigned long offset)
{

	KASSERT(!mapping->diom_mapped);
	KASSERT(ISSET(mapping->diom_flags, BUS_SPACE_MAP_LINEAR));
	if (bus_space_reservation_map(mapping->diom_bst, &mapping->diom_bsr,
		mapping->diom_flags, &mapping->diom_bsh))
		panic("Unable to make I/O mapping!"); /* XXX */
	mapping->diom_mapped = true;

	return (char *)bus_space_vaddr(mapping->diom_bst, mapping->diom_bsh)
	    + offset;		/* XXX arithmetic overflow */
}

static inline void
io_mapping_unmap(struct io_mapping *mapping, void *vaddr __unused)
{

	KASSERT(mapping->diom_mapped);
	bus_space_reservation_unmap(mapping->diom_bst, mapping->diom_bsh,
	    mapping->diom_size);
	mapping->diom_mapped = false;
}

static inline void *
io_mapping_map_atomic_wc(struct io_mapping *mapping, unsigned long offset)
{
	return io_mapping_map_wc(mapping, offset);
}

static inline void
io_mapping_unmap_atomic(struct io_mapping *mapping, void *vaddr __unused)
{
	return io_mapping_unmap(mapping, vaddr);
}

#endif  /* _LINUX_IO_MAPPING_H_ */
