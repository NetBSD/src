/*	$NetBSD: bus_space.c,v 1.1.14.2 2002/06/23 17:38:35 jdolecek Exp $	*/

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

#include <machine/bat.h>
#define _POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#undef _POWERPC_BUS_DMA_PRIVATE

paddr_t	mvmeppc_memio_mmap (bus_space_tag_t, bus_addr_t, off_t, int, int);
int	mvmeppc_memio_map (bus_space_tag_t, bus_addr_t, bus_size_t, int,
	    bus_space_handle_t *);
void	mvmeppc_memio_unmap (bus_space_tag_t, bus_space_handle_t, bus_size_t);
int	mvmeppc_memio_alloc (bus_space_tag_t, bus_addr_t, bus_addr_t,
	    bus_size_t, bus_size_t, bus_size_t, int, bus_addr_t *,
	    bus_space_handle_t *);
void	mvmeppc_memio_free (bus_space_tag_t, bus_space_handle_t, bus_size_t);


const struct powerpc_bus_space mvmeppc_isa_io_bs_tag = {
	MVMEPPC_BUS_SPACE_IO,
	MVMEPPC_PHYS_BASE_IO,	/* 60x-bus address of ISA I/O Space */
	0x00000000,		/* Corresponds to ISA-bus I/O address 0x0 */
	0x00010000,		/* End of ISA-bus I/O address space, +1 */
	mvmeppc_memio_mmap,
	mvmeppc_memio_map, mvmeppc_memio_unmap,
	mvmeppc_memio_alloc, mvmeppc_memio_free
};

const struct powerpc_bus_space mvmeppc_pci_io_bs_tag = {
	MVMEPPC_BUS_SPACE_IO,
	MVMEPPC_PHYS_BASE_IO,	/* 60x-bus address of PCI I/O Space */
	0x00000000,		/* Corresponds to PCI-bus I/O address 0x0 */
	MVMEPPC_PHYS_SIZE_IO,	/* End of PCI-bus I/O address space, +1 */
	mvmeppc_memio_mmap,
	mvmeppc_memio_map, mvmeppc_memio_unmap,
	mvmeppc_memio_alloc, mvmeppc_memio_free
};

const struct powerpc_bus_space mvmeppc_isa_mem_bs_tag = {
	MVMEPPC_BUS_SPACE_MEM,
	MVMEPPC_PHYS_BASE_MEM,	/* 60x-bus address of ISA Memory Space */
	0x00000000,		/* Corresponds to ISA-bus Memory addr 0x0 */
	0x01000000,		/* End of ISA-bus Memory addr space, +1 */
	mvmeppc_memio_mmap,
	mvmeppc_memio_map, mvmeppc_memio_unmap,
	mvmeppc_memio_alloc, mvmeppc_memio_free
};

const struct powerpc_bus_space mvmeppc_pci_mem_bs_tag = {
	MVMEPPC_BUS_SPACE_MEM,
	MVMEPPC_PHYS_BASE_MEM,	/* 60x-bus address of PCI Memory Space */
	0x00000000,		/* Corresponds to PCI-bus Memory addr 0x0 */
	MVMEPPC_PHYS_SIZE_MEM,	/* End of PCI-bus Memory addr space, +1 */
	mvmeppc_memio_mmap,
	mvmeppc_memio_map, mvmeppc_memio_unmap,
	mvmeppc_memio_alloc, mvmeppc_memio_free
};

/*
 * Evaluates true if `t' is not a valid tag type
 */
#define	BAD_TAG(t)	((t) < 0 || (t) >= MVMEPPC_BUS_SPACE_NUM_REGIONS)

/*
 * Bus Space region accounting
 */
static struct extent *bus_ex[MVMEPPC_BUS_SPACE_NUM_REGIONS];

/*
 * We maintain BAT mappings for subsets of I/O and Memory space...
 */
struct mvmeppc_bat_region {
	vaddr_t		br_kva;
	bus_addr_t	br_len;
};
static const struct mvmeppc_bat_region bat_regions[] = {
	{MVMEPPC_KVA_BASE_IO,  MVMEPPC_KVA_SIZE_IO},
	{MVMEPPC_KVA_BASE_MEM, MVMEPPC_KVA_SIZE_MEM}
};

#define page_offset(a)	((a) - trunc_page(a))


void
mvmeppc_bus_space_init(void)
{

	bus_ex[MVMEPPC_BUS_SPACE_IO] = extent_create("bus_io",
	    0, MVMEPPC_PHYS_SIZE_IO,
	    M_DEVBUF, NULL, 0, EX_NOCOALESCE | EX_NOWAIT | EX_MALLOCOK);

	if (extent_alloc_region(bus_ex[MVMEPPC_BUS_SPACE_IO],
	    MVMEPPC_PHYS_RESVD_START_IO, MVMEPPC_PHYS_RESVD_SIZE_IO,
	    EX_NOWAIT | EX_MALLOCOK) != 0)
		panic("mvmeppc_bus_space_init: reserving I/O hole");

	bus_ex[MVMEPPC_BUS_SPACE_MEM] = extent_create("bus_mem",
	    0, MVMEPPC_PHYS_SIZE_MEM,
	    M_DEVBUF, NULL, 0, EX_NOCOALESCE | EX_NOWAIT | EX_MALLOCOK);
}

paddr_t
mvmeppc_memio_mmap(t, bpa, offset, prot, flags)
	bus_space_tag_t t;
	bus_addr_t bpa;
	off_t offset;
	int prot, flags;
{

	return ((bpa + offset) >> PGSHIFT);
}

int
mvmeppc_memio_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	const struct mvmeppc_bat_region *br;
	struct extent *ex;
	bus_addr_t kpa;
	vaddr_t va;
	int error, i;

#ifdef DEBUG
	if (BAD_TAG(t->pbs_type))
		panic("mvmeppc_memio_map: bad tag: %d", t->pbs_type);
#endif

	if ((flags & BUS_SPACE_MAP_LINEAR) != 0 &&
	    t->pbs_type == MVMEPPC_BUS_SPACE_IO)
		return (EOPNOTSUPP);

	br = &bat_regions[t->pbs_type];
	ex = bus_ex[t->pbs_type];

	if ((bpa + size) > t->pbs_limit)
		return (EINVAL);

	/*
	 * Bail if trying to map a region which overlaps the end of
	 * a BAT mapping.
	 *
	 * XXX: Watch out, one day we'll trip over this...
	 */
	if (bpa < br->br_len && (bpa + size) > br->br_len)
		return (EINVAL);

	if (ex) {
		/*
		 * Before we go any further, let's make sure that this
		 * region is available.
		 */
		if ((error = extent_alloc_region(ex, bpa, size,
		    EX_NOWAIT | EX_MALLOCOK)) != 0)
			return (error);
	}

	/*
	 * Does the requested bus space region fall within the KVA mapping
	 * set up for this space by a BAT register?
	 */
	if ((bpa + size) < br->br_len) {
		/* We can satisfy this request directly from the BAT mapping */
		*bshp = (bus_space_handle_t)(bpa + (bus_addr_t)br->br_kva);
		return (0);
	}

	/*
	 * Otherwise, we have to manually fix up a mapping.
	 */
	va = uvm_km_valloc(kernel_map, round_page(size));
	if (va == NULL) {
		(void) extent_free(ex, bpa, size, EX_NOWAIT | EX_MALLOCOK);
		return (ENOMEM);
	}

	*bshp = (bus_space_handle_t)((caddr_t)va + page_offset(bpa));
	kpa = trunc_page(t->pbs_offset + bpa);

	for (i = round_page(size) / NBPG; i > 0; i--, kpa += NBPG, va += NBPG)
		pmap_enter(pmap_kernel(), va, kpa,
		    VM_PROT_READ | VM_PROT_WRITE,
		    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);

	return (0);
}

void
mvmeppc_memio_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	const struct mvmeppc_bat_region *br;
	struct extent *ex;
	bus_addr_t bpa;

#ifdef DEBUG
	if (BAD_TAG(t->pbs_type))
		panic("mvmeppc_memio_unmap: bad tag: %d", t->pbs_type);
#endif

	br = &bat_regions[t->pbs_type];
	ex = bus_ex[t->pbs_type];

	/*
	 * Was the mapping within the BAT region of this space?
	 */
	if (bsh < br->br_kva || bsh >= (br->br_kva + br->br_len)) {
		/*
		 * Nope. We have to figure out the physical address
		 * and unmap it for real.
		 */
		if (!pmap_extract(pmap_kernel(), (vaddr_t)bsh, (paddr_t *)&bpa))
			panic("mvmeppc_memio_unmap: bad bus handle: 0x%x", bsh);
		uvm_km_free(kernel_map, (vaddr_t)trunc_page(bsh),
		    round_page(size));
		bpa -= t->pbs_offset;
	} else
		bpa = (bus_addr_t) bsh - (bus_addr_t) br->br_kva;

	if (ex && extent_free(ex, bpa, size, EX_NOWAIT | EX_MALLOCOK)) {
		printf("mvmeppc_memio_unmap: %s 0x%lx, size 0x%lx\n",
		    (t->pbs_type == MVMEPPC_BUS_SPACE_IO) ? "port" : "mem",
		    (u_long)bpa, (u_long)size);
		printf("mvmeppc_memio_unmap: can't free region\n");
	}
}

int
mvmeppc_memio_alloc(t, rstart, rend, size, alignment, boundary, flags,
    bpap, bshp)
	bus_space_tag_t t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int flags;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	const struct mvmeppc_bat_region *br;
	struct extent *ex;
	bus_dma_segment_t seg;
	caddr_t va;
	u_long bpa;
	int error;

#ifdef DEBUG
	if (BAD_TAG(t->pbs_type))
		panic("mvmeppc_memio_alloc: bad tag: %d", t->pbs_type);
#endif

	if ((flags & BUS_SPACE_MAP_LINEAR) != 0 &&
	    t->pbs_type == MVMEPPC_BUS_SPACE_IO)
		return (EOPNOTSUPP);

	br = &bat_regions[t->pbs_type];
	ex = bus_ex[t->pbs_type];

	if ((rstart + size) > t->pbs_limit)
		return (EINVAL);

	if (rstart < ex->ex_start || rend > ex->ex_end)
		panic("mvmeppc_memio_alloc: bad region start/end");

	if (ex) {
		error = extent_alloc_subregion(ex, rstart, rend, size,
		    alignment, boundary, EX_FAST | EX_NOWAIT | EX_MALLOCOK,
		    &bpa);
		if (error)
			return (error);
	}

	/*
	 * Bail if trying to map a region which overlaps the end of
	 * a BAT mapping.
	 *
	 * XXX: Watch out, one day we'll trip over this...
	 */
	if (bpa < br->br_len && (bpa + size) > br->br_len) {
		if (ex) {
			(void) extent_free(ex, bpa, size,
			    EX_NOWAIT | EX_MALLOCOK);
		}
		return (EINVAL);
	}

	/*
	 * Does the requested bus space region fall within the KVA mapping
	 * set up for this space by a BAT register?
	 */
	if ((bpa + size) < br->br_len) {
		/* We can satisfy this request directly from the BAT mapping */
		*bpap = bpa;
		*bshp = (bus_space_handle_t)(bpa + (bus_addr_t)br->br_kva);
		return (0);
	}

	/*
	 * Otherwise, we have to manually fix up a mapping.
	 */
	seg.ds_addr = trunc_page(t->pbs_base + bpa);
	seg.ds_len = round_page(size);

	if (_bus_dmamem_map(NULL, &seg, 1, seg.ds_len, &va,
	    flags | BUS_DMA_COHERENT)) {
		if (ex) {
			(void) extent_free(ex, bpa, size,
			    EX_NOWAIT | EX_MALLOCOK);
		}
		return (ENOMEM);
	}

	*bpap = bpa;
	*bshp = (bus_space_handle_t)(va + page_offset(bpa));

	return (0);
}

void
mvmeppc_memio_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{

	/* mvmeppc_memio_unmap() does all that we need to do. */
	mvmeppc_memio_unmap(t, bsh, size);
}
