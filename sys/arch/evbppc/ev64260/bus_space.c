/*	$NetBSD: bus_space.c,v 1.4 2003/03/07 18:24:00 matt Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/extent.h>
#include <sys/mbuf.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

static paddr_t memio_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);
static int memio_map(bus_space_tag_t, bus_addr_t, bus_size_t, int,
	bus_space_handle_t *);
static void memio_unmap(bus_space_tag_t, bus_space_handle_t, bus_size_t);
static int memio_alloc(bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t,
	bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
static void memio_free(bus_space_tag_t, bus_space_handle_t, bus_size_t);

struct powerpc_bus_space gt_pci0_mem_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0x00000000, 0x81000000, 0x89000000,
	NULL,
	memio_mmap, memio_map, memio_unmap, memio_alloc, memio_free
};
struct powerpc_bus_space gt_pci0_io_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	0x80000000, 0x00000000, 0x00800000,
	NULL,
	memio_mmap, memio_map, memio_unmap, memio_alloc, memio_free
};
struct powerpc_bus_space gt_pci1_mem_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0x00000000, 0x89000000, 0x90000000,
	NULL,
	memio_mmap, memio_map, memio_unmap, memio_alloc, memio_free
};
struct powerpc_bus_space gt_pci1_io_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	0x88000000, 0x00000000, 0x00800000,
	NULL,
	memio_mmap, memio_map, memio_unmap, memio_alloc, memio_free
};
struct powerpc_bus_space gt_mem_bs_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0x00000000, 0x00000000, 0x00010000,
	NULL,
	memio_mmap, memio_map, memio_unmap, memio_alloc, memio_free
};

static char ex_storage[5][EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));

static int extent_flags;

void
bus_space_init(void)
{
	int error;

	gt_pci0_mem_bs_tag.pbs_extent = extent_create("pci0-mem",
	    gt_pci0_mem_bs_tag.pbs_base,
	    gt_pci0_mem_bs_tag.pbs_limit-1,
	    M_DEVBUF,
	    ex_storage[0], sizeof(ex_storage[0]),
	    EX_NOCOALESCE|EX_NOWAIT);

	gt_pci0_io_bs_tag.pbs_extent = extent_create("pci0-ioport",
	    gt_pci0_io_bs_tag.pbs_base,
	    gt_pci0_io_bs_tag.pbs_limit-1,
	    M_DEVBUF,
	    ex_storage[1], sizeof(ex_storage[1]),
	    EX_NOCOALESCE|EX_NOWAIT);
	error = extent_alloc_region(gt_pci0_io_bs_tag.pbs_extent,
	    0x10000, 0x7F0000, EX_NOWAIT);
	if (error)
		panic("gt_bus_space_init: can't block out reserved "
		    "I/O space 0x10000-0x7fffff: error=%d\n", error);

	gt_pci1_mem_bs_tag.pbs_extent = extent_create("pci1-iomem",
	    gt_pci1_mem_bs_tag.pbs_base,
	    gt_pci1_mem_bs_tag.pbs_limit-1,
	    M_DEVBUF,
	    ex_storage[2], sizeof(ex_storage[2]),
	    EX_NOCOALESCE|EX_NOWAIT);

	gt_pci1_io_bs_tag.pbs_extent = extent_create("pci1-ioport",
	    gt_pci1_io_bs_tag.pbs_base,
	    gt_pci1_io_bs_tag.pbs_limit-1,
	    M_DEVBUF,
	    ex_storage[3], sizeof(ex_storage[3]),
	    EX_NOCOALESCE|EX_NOWAIT);
	error = extent_alloc_region(gt_pci1_io_bs_tag.pbs_extent,
	     0x10000, 0x7F0000, EX_NOWAIT);
	if (error)
		panic("gt_bus_space_init: can't block out reserved "
		    "I/O space 0x10000-0x7fffff: error=%d\n", error);

	gt_mem_bs_tag.pbs_extent = extent_create("gtmem",
	    gt_mem_bs_tag.pbs_base,
	    gt_mem_bs_tag.pbs_limit-1,
	    M_DEVBUF,
	    ex_storage[4], sizeof(ex_storage[4]),
	    EX_NOCOALESCE|EX_NOWAIT);
}

void
bus_space_mallocok(void)
{
	extent_flags = EX_MALLOCOK;
}

paddr_t
memio_mmap(bus_space_tag_t t, bus_addr_t bpa, off_t offset, int prot, int flags)
{
	return (trunc_page(bpa + offset));
}

int
memio_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size, int flags,
	bus_space_handle_t *bshp)
{
	int error;

	if (bpa + size > t->pbs_limit)
		return (EINVAL);

	/*
	 * Can't map I/O space as linear.
	 */
	if ((flags & BUS_SPACE_MAP_LINEAR) &&
	    (t->pbs_flags & _BUS_SPACE_IO_TYPE))
		return (EOPNOTSUPP);

	/*
	 * Before we go any further, let's make sure that this
	 * region is available.
	 */
	error = extent_alloc_region(t->pbs_extent, bpa, size,
	    EX_NOWAIT | extent_flags);
	if (error)
		return (error);

	*bshp = t->pbs_offset + bpa;

	return (0);
}

void
memio_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	bus_addr_t bpa = bsh - t->pbs_offset;

	if (extent_free(t->pbs_extent, bpa, size, EX_NOWAIT | extent_flags)) {
		printf("memio_unmap: %s 0x%lx, size 0x%lx\n",
		    (t->pbs_flags & _BUS_SPACE_IO_TYPE) ? "port" : "mem",
		    (unsigned long)bpa, (unsigned long)size);
		printf("memio_unmap: can't free region\n");
	}
}

int
memio_alloc(bus_space_tag_t t, bus_addr_t rstart, bus_addr_t rend,
	bus_size_t size, bus_size_t alignment, bus_size_t boundary,
	int flags, bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	u_long bpa;
	int error;

	if (rstart + size > t->pbs_limit)
		return (EINVAL);

	/*
	 * Can't map I/O space as linear.
	 */
	if ((flags & BUS_SPACE_MAP_LINEAR) &&
	    (t->pbs_flags & _BUS_SPACE_IO_TYPE))
		return (EOPNOTSUPP);

	if (rstart < t->pbs_extent->ex_start || rend > t->pbs_extent->ex_end)
		panic("memio_alloc: bad region start/end");

	error = extent_alloc_subregion(t->pbs_extent, rstart, rend, size,
	    alignment, boundary, EX_FAST | EX_NOWAIT | extent_flags, &bpa);

	if (error)
		return (error);

	*bpap = bpa;
	*bshp = t->pbs_offset + bpa;

	return (0);
}

void
memio_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	/* memio_unmap() does all that we need to do. */
	memio_unmap(t, bsh, size);
}
