/*	$NetBSD: bus_space.c,v 1.1.2.2 2002/06/23 17:39:34 jdolecek Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <dev/ic/cpc700reg.h>
#include <machine/pmppc.h>

static paddr_t pmppc_memio_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);
static int pmppc_memio_map (bus_space_tag_t, bus_addr_t, bus_size_t, int,
	bus_space_handle_t *);
static void pmppc_memio_unmap(bus_space_tag_t, bus_space_handle_t, bus_size_t);
static int pmppc_memio_alloc(bus_space_tag_t, bus_addr_t, bus_addr_t,
	bus_size_t, bus_size_t, bus_size_t, int, bus_addr_t *,
	bus_space_handle_t *);
static void pmppc_memio_free(bus_space_tag_t, bus_space_handle_t, bus_size_t);

const struct powerpc_bus_space pmppc_mem_tag = {
	0, 0, 0x0000000, 0xffffffff,
	pmppc_memio_mmap,
	pmppc_memio_map, pmppc_memio_unmap, pmppc_memio_alloc,
	pmppc_memio_free
};
const struct powerpc_bus_space pmppc_pci_io_tag = {
	0, 0, CPC_PCI_IO_BASE, 0xffffffff,
	pmppc_memio_mmap,
	pmppc_memio_map, pmppc_memio_unmap, pmppc_memio_alloc,
	pmppc_memio_free
};

static long ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];

static struct extent *allmem_ex;

static int ioport_malloc_safe;

void
pmppc_bus_space_init(void)
{
	allmem_ex = extent_create("iomem", PMPPC_IO_START, 0xffffffff,
	        M_DEVBUF, (caddr_t)ex_storage, sizeof(ex_storage),
		EX_NOCOALESCE | EX_NOWAIT);
}

void
pmppc_bus_space_mallocok(void)
{

	ioport_malloc_safe = 1;
}

static paddr_t
pmppc_memio_mmap(t, bpa, offset, prot, flags)
	bus_space_tag_t t;
	bus_addr_t bpa;
	off_t offset;
	int prot, flags;
{
	return ((bpa + offset) >> PGSHIFT);
}

static int
pmppc_memio_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	int error;

	bpa += t->pbs_base;

	/*
	 * Before we go any further, let's make sure that this
	 * region is available.
	 */
	error = extent_alloc_region(allmem_ex, bpa, size,
	    EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0));
	if (error)
		return (error);

	*bshp = bpa;

	return (0);
}

static void
pmppc_memio_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	bus_addr_t bpa;

	bpa = bsh;

	if (extent_free(allmem_ex, bpa, size,
	    EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0))) {
		printf("pmppc_memio_unmap: type=%d 0x%lx, size 0x%lx\n",
		       t->pbs_type, (unsigned long)bpa, (unsigned long)size);
		printf("pmppc_memio_unmap: can't free region\n");
	}
}

static int
pmppc_memio_alloc(t, rstart, rend, size, alignment, boundary, flags,
    bpap, bshp)
	bus_space_tag_t t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int flags;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	u_long bpa;
	int error;

	if (rstart < allmem_ex->ex_start || rend > allmem_ex->ex_end)
		panic("pmppc_memio_alloc: bad region start/end");

	error = extent_alloc_subregion(allmem_ex, rstart, rend, size,
	    alignment, boundary,
	    EX_FAST | EX_NOWAIT | (ioport_malloc_safe ?  EX_MALLOCOK : 0),
	    &bpa);

	if (error)
		return (error);

	*bpap = bpa - t->pbs_base;
	*bshp = bpa;

	return (0);
}

static void
pmppc_memio_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	/* pmppc_memio_unmap() does all that we need to do. */
	pmppc_memio_unmap(t, bsh, size);
}
