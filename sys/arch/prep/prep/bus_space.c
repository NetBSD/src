/*	$NetBSD: bus_space.c,v 1.1 2000/03/27 16:45:42 nonaka Exp $	*/

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

#include <machine/bus.h>

static long ioport_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
static long iomem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];

struct extent *ioport_ex;
struct extent *iomem_ex;

static int ioport_malloc_safe;

void
prep_bus_space_init()
{

	ioport_ex = extent_create("ioport", 0, 0xffffffff, M_DEVBUF,
	    (caddr_t)ioport_ex_storage, sizeof(ioport_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);
	iomem_ex = extent_create("iomem", 0x0, 0xffffffff, M_DEVBUF,
	    (caddr_t)iomem_ex_storage, sizeof(iomem_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);
}

void
prep_bus_space_mallocok()
{

	ioport_malloc_safe = 1;
}

int
prep_memio_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	int error;
	struct extent *ex;

	/*
	 * Pick the appropriate extent map.
	 */
	if (t == PREP_BUS_SPACE_IO) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		ex = ioport_ex;
	} else if (t == PREP_BUS_SPACE_MEM)
		ex = iomem_ex;
	else
		panic("prep_memio_map: bad bus space tag");

	/*
	 * Before we go any further, let's make sure that this
	 * region is available.
	 */
	error = extent_alloc_region(ex, bpa, size,
	    EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0));
	if (error)
		return (error);

	*bshp = t + bpa;

	return (0);
}

void
prep_memio_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	struct extent *ex;
	u_long va, endva;
	bus_addr_t bpa;

	/*
	 * Find the correct extent and bus physical address.
	 */
	if (t == PREP_BUS_SPACE_IO)
		ex = ioport_ex;
	else if (t == PREP_BUS_SPACE_MEM)
		ex = iomem_ex;
	else
		panic("prep_memio_unmap: bad bus space tag");

	bpa = bsh - t;

	if (extent_free(ex, bpa, size,
	    EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0))) {
		printf("prep_memio_unmap: %s 0x%lx, size 0x%lx\n",
		    (t == PREP_BUS_SPACE_IO) ? "port" : "mem", bpa, size);
		printf("prep_memio_unmap: can't free region\n");
	}
}

int
prep_memio_alloc(t, rstart, rend, size, alignment, boundary, flags,
    bpap, bshp)
	bus_space_tag_t t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int flags;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	struct extent *ex;
	u_long bpa;
	int error;

	if (t == PREP_BUS_SPACE_IO) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		ex = ioport_ex;
	} else if (t == PREP_BUS_SPACE_MEM)
		ex = iomem_ex;
	else
		panic("prep_memio_alloc: bad bus space tag");

	if (rstart < ex->ex_start || rend > ex->ex_end)
		panic("prep_memio_alloc: bad region start/end");

	error = extent_alloc_subregion(ex, rstart, rend, size, alignment,
	    boundary,
	    EX_FAST | EX_NOWAIT | (ioport_malloc_safe ?  EX_MALLOCOK : 0),
	    &bpa);

	if (error)
		return (error);

	*bshp = *bpap = t + bpa;

	return (0);
}

void    
prep_memio_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{

	/* prep_memio_unmap() does all that we need to do. */
	prep_memio_unmap(t, bsh, size);
}
