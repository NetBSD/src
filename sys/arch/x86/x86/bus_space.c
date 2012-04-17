/*	$NetBSD: bus_space.c,v 1.37.2.1 2012/04/17 00:07:06 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: bus_space.c,v 1.37.2.1 2012/04/17 00:07:06 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

#include <dev/isa/isareg.h>

#include <sys/bus.h>
#include <machine/pio.h>
#include <machine/isa_machdep.h>

#ifdef XEN
#include <xen/hypervisor.h>
#endif

/*
 * Macros for sanity-checking the aligned-ness of pointers passed to
 * bus space ops.  These are not strictly necessary on the x86, but
 * could lead to performance improvements, and help catch problems
 * with drivers that would creep up on other architectures.
 */
#ifdef BUS_SPACE_DEBUG 
#define	BUS_SPACE_ALIGNED_ADDRESS(p, t)				\
	((((u_long)(p)) & (sizeof(t)-1)) == 0)

#define	BUS_SPACE_ADDRESS_SANITY(p, t, d)				\
({									\
	if (BUS_SPACE_ALIGNED_ADDRESS((p), t) == 0) {			\
		printf("%s 0x%lx not aligned to %zu bytes %s:%d\n",	\
		    d, (u_long)(p), sizeof(t), __FILE__, __LINE__);	\
	}								\
	(void) 0;							\
})
#else
#define	BUS_SPACE_ADDRESS_SANITY(p,t,d)	(void) 0
#endif /* BUS_SPACE_DEBUG */

/*
 * Extent maps to manage I/O and memory space.  Allocate
 * storage for 8 regions in each, initially.  Later, ioport_malloc_safe
 * will indicate that it's safe to use malloc() to dynamically allocate
 * region descriptors.
 *
 * N.B. At least two regions are _always_ allocated from the iomem
 * extent map; (0 -> ISA hole) and (end of ISA hole -> end of RAM).
 *
 * The extent maps are not static!  Machine-dependent ISA and EISA
 * routines need access to them for bus address space allocation.
 */
static	long ioport_ex_storage[EXTENT_FIXED_STORAGE_SIZE(16) / sizeof(long)];
static	long iomem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(64) / sizeof(long)];
struct	extent *ioport_ex;
struct	extent *iomem_ex;
static	int ioport_malloc_safe;

static struct bus_space_tag x86_io = { .bst_type = X86_BUS_SPACE_IO };
static struct bus_space_tag x86_mem = { .bst_type = X86_BUS_SPACE_MEM };

bus_space_tag_t x86_bus_space_io = &x86_io;
bus_space_tag_t x86_bus_space_mem = &x86_mem;

int x86_mem_add_mapping(bus_addr_t, bus_size_t,
	    int, bus_space_handle_t *);

static inline bool
x86_bus_space_is_io(bus_space_tag_t t)
{
	return t->bst_type == X86_BUS_SPACE_IO;
}

static inline bool
x86_bus_space_is_mem(bus_space_tag_t t)
{
	return t->bst_type == X86_BUS_SPACE_MEM;
}

void
x86_bus_space_init(void)
{
	/*
	 * Initialize the I/O port and I/O mem extent maps.
	 * Note: we don't have to check the return value since
	 * creation of a fixed extent map will never fail (since
	 * descriptor storage has already been allocated).
	 *
	 * N.B. The iomem extent manages _all_ physical addresses
	 * on the machine.  When the amount of RAM is found, the two
	 * extents of RAM are allocated from the map (0 -> ISA hole
	 * and end of ISA hole -> end of RAM).
	 */
	ioport_ex = extent_create("ioport", 0x0, 0xffff,
	    (void *)ioport_ex_storage, sizeof(ioport_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);
	iomem_ex = extent_create("iomem", 0x0, 0xffffffff,
	    (void *)iomem_ex_storage, sizeof(iomem_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);

#ifdef XEN
	/* We are privileged guest os - should have IO privileges. */
	if (xendomain_is_privileged()) {
		struct physdev_op physop;
		physop.cmd = PHYSDEVOP_SET_IOPL;
		physop.u.set_iopl.iopl = 1;
		if (HYPERVISOR_physdev_op(&physop) != 0)
			panic("Unable to obtain IOPL, "
			    "despite being SIF_PRIVILEGED");
	}
#endif	/* XEN */
}

void
x86_bus_space_mallocok(void)
{

	ioport_malloc_safe = 1;
}

int
bus_space_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size,
		int flags, bus_space_handle_t *bshp)
{
	bus_space_reservation_t bsr;
	bus_space_tag_t it;
	int error;

	if ((t->bst_exists & BUS_SPACE_OVERRIDE_MAP) == 0)
		;	/* skip override */
	else for (it = t; it != NULL; it = it->bst_super) {
		if ((it->bst_present & BUS_SPACE_OVERRIDE_MAP) == 0)
			continue;
		return (*it->bst_ov->ov_space_map)(it->bst_ctx, t, bpa, size,
		    flags, bshp);
	}

	error = bus_space_reserve(t, bpa, size, flags, &bsr);
	if (error != 0)
		return error;

	error = bus_space_reservation_map(t, &bsr, flags, bshp);
	if (error != 0)
		bus_space_release(t, &bsr);

	return error;
}

int
bus_space_reservation_map(bus_space_tag_t t, bus_space_reservation_t *bsr,
    int flags, bus_space_handle_t *bshp)
{
	bus_addr_t bpa;
	bus_size_t size;
	bus_space_tag_t it;

	if ((t->bst_exists & BUS_SPACE_OVERRIDE_RESERVATION_MAP) == 0)
		;	/* skip override */
	else for (it = t; it != NULL; it = it->bst_super) {
		if ((it->bst_present & BUS_SPACE_OVERRIDE_RESERVATION_MAP) == 0)
			continue;
		return (*it->bst_ov->ov_space_reservation_map)(it->bst_ctx, t,
		    bsr, flags, bshp);
	}

	bpa = bus_space_reservation_addr(bsr);
	size = bus_space_reservation_size(bsr);

	/*
	 * For I/O space, that's all she wrote.
	 */
	if (x86_bus_space_is_io(t)) {
		*bshp = bpa;
		return 0;
	}

#ifndef XEN
	if (bpa >= IOM_BEGIN && (bpa + size) != 0 && (bpa + size) <= IOM_END) {
		*bshp = (bus_space_handle_t)ISA_HOLE_VADDR(bpa);
		return 0;
	}
#endif	/* !XEN */

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	return x86_mem_add_mapping(bpa, size, flags, bshp);
}

int
_x86_memio_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size,
		int flags, bus_space_handle_t *bshp)
{

	/*
	 * For I/O space, just fill in the handle.
	 */
	if (x86_bus_space_is_io(t)) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		*bshp = bpa;
		return (0);
	}

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	return x86_mem_add_mapping(bpa, size, flags, bshp);
}

int
bus_space_reserve(bus_space_tag_t t,
    bus_addr_t bpa,
    bus_size_t size,
    int flags, bus_space_reservation_t *bsrp)
{
	struct extent *ex;
	int error;
	bus_space_tag_t it;

	if ((t->bst_exists & BUS_SPACE_OVERRIDE_RESERVE) == 0)
		;	/* skip override */
	else for (it = t; it != NULL; it = it->bst_super) {
		if ((it->bst_present & BUS_SPACE_OVERRIDE_RESERVE) == 0)
			continue;
		return (*it->bst_ov->ov_space_reserve)(it->bst_ctx, t,
		    bpa, size, flags, bsrp);
	}

	/*
	 * Pick the appropriate extent map.
	 */
	if (x86_bus_space_is_io(t)) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		ex = ioport_ex;
	} else if (x86_bus_space_is_mem(t))
		ex = iomem_ex;
	else
		panic("x86_memio_alloc: bad bus space tag");

	/*
	 * Before we go any further, let's make sure that this
	 * region is available.
	 */
	error = extent_alloc_region(ex, bpa, size,
	    EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0));

	if (error != 0)
		return error;

	bus_space_reservation_init(bsrp, bpa, size);

	return 0;
}

int
bus_space_reserve_subregion(bus_space_tag_t t,
    bus_addr_t rstart, bus_addr_t rend,
    const bus_size_t size, const bus_size_t alignment,
    const bus_size_t boundary,
    const int flags, bus_space_reservation_t *bsrp)
{
	bus_space_reservation_t bsr;
	struct extent *ex;
	u_long bpa;
	int error;
	bus_space_tag_t it;

	if ((t->bst_exists & BUS_SPACE_OVERRIDE_RESERVE_SUBREGION) == 0)
		;	/* skip override */
	else for (it = t; it != NULL; it = it->bst_super) {
		if ((it->bst_present & BUS_SPACE_OVERRIDE_RESERVE_SUBREGION) ==
		    0)
			continue;
		return (*it->bst_ov->ov_space_reserve_subregion)(it->bst_ctx, t,
		    rstart, rend, size, alignment, boundary, flags, bsrp);
	}

	/*
	 * Pick the appropriate extent map.
	 */
	if (x86_bus_space_is_io(t)) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		ex = ioport_ex;
	} else if (x86_bus_space_is_mem(t))
		ex = iomem_ex;
	else
		panic("x86_memio_alloc: bad bus space tag");

	/*
	 * Sanity check the allocation against the extent's boundaries.
	 */
	rstart = MAX(rstart, ex->ex_start);
	rend = MIN(rend, ex->ex_end);
	if (rstart >= rend)
		panic("x86_memio_alloc: bad region start/end");

	/*
	 * Do the requested allocation.
	 */
	error = extent_alloc_subregion(ex, rstart, rend, size, alignment,
	    boundary,
	    EX_FAST | EX_NOWAIT | (ioport_malloc_safe ?  EX_MALLOCOK : 0),
	    &bpa);

	if (error)
		return (error);

	bus_space_reservation_init(&bsr, bpa, size);

	*bsrp = bsr;

	return 0;
}

void
bus_space_release(bus_space_tag_t t, bus_space_reservation_t *bsr)
{
	struct extent *ex;
	bus_space_tag_t it;

	if ((t->bst_exists & BUS_SPACE_OVERRIDE_RELEASE) == 0)
		;	/* skip override */
	else for (it = t; it != NULL; it = it->bst_super) {
		if ((it->bst_present & BUS_SPACE_OVERRIDE_RELEASE) == 0)
			continue;
		(*it->bst_ov->ov_space_release)(it->bst_ctx, t, bsr);
		return;
	}

	/*
	 * Pick the appropriate extent map.
	 */
	if (x86_bus_space_is_io(t)) {
		ex = ioport_ex;
	} else if (x86_bus_space_is_mem(t))
		ex = iomem_ex;
	else
		panic("x86_memio_alloc: bad bus space tag");

	if (extent_free(ex, bus_space_reservation_addr(bsr),
	    bus_space_reservation_size(bsr), EX_NOWAIT |
	    (ioport_malloc_safe ? EX_MALLOCOK : 0))) {
		printf("%s: pa 0x%jx, size 0x%jx\n", __func__,
		    (uintmax_t)bus_space_reservation_addr(bsr),
		    (uintmax_t)bus_space_reservation_size(bsr));
		printf("%s: can't free region\n", __func__);
	}
}

int
bus_space_alloc(bus_space_tag_t t, bus_addr_t rstart, bus_addr_t rend,
		bus_size_t size, bus_size_t alignment, bus_size_t boundary,
		int flags, bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	bus_space_reservation_t bsr;
	bus_space_tag_t it;
	int error;

	if ((t->bst_exists & BUS_SPACE_OVERRIDE_ALLOC) == 0)
		;	/* skip override */
	else for (it = t; it != NULL; it = it->bst_super) {
		if ((it->bst_present & BUS_SPACE_OVERRIDE_ALLOC) == 0)
			continue;
		return (*it->bst_ov->ov_space_alloc)(it->bst_ctx, t,
		    rstart, rend, size, alignment, boundary, flags, bpap, bshp);
	}

	/*
	 * Do the requested allocation.
	 */
	error = bus_space_reserve_subregion(t, rstart, rend, size, alignment,
	    boundary, flags, &bsr);

	if (error != 0)
		return error;

	error = bus_space_reservation_map(t, &bsr, flags, bshp);
	if (error != 0)
		bus_space_release(t, &bsr);

	*bpap = bus_space_reservation_addr(&bsr);

	return error;
}

int
x86_mem_add_mapping(bus_addr_t bpa, bus_size_t size,
		int flags, bus_space_handle_t *bshp)
{
	paddr_t pa, endpa;
	vaddr_t va, sva;
	u_int pmapflags;

	pa = x86_trunc_page(bpa);
	endpa = x86_round_page(bpa + size);

	pmapflags = PMAP_NOCACHE;
	if ((flags & BUS_SPACE_MAP_CACHEABLE) != 0)
		pmapflags = 0;
	else if (flags & BUS_SPACE_MAP_PREFETCHABLE)
		pmapflags = PMAP_WRITE_COMBINE;

#ifdef DIAGNOSTIC
	if (endpa != 0 && endpa <= pa)
		panic("x86_mem_add_mapping: overflow");
#endif

#ifdef XEN
	if (bpa >= IOM_BEGIN && (bpa + size) != 0 && (bpa + size) <= IOM_END) {
		sva = (vaddr_t)ISA_HOLE_VADDR(pa);
	} else
#endif	/* XEN */
	{
		sva = uvm_km_alloc(kernel_map, endpa - pa, 0,
		    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
		if (sva == 0)
			return (ENOMEM);
	}

	*bshp = (bus_space_handle_t)(sva + (bpa & PGOFSET));

	for (va = sva; pa != endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_ma(va, pa, VM_PROT_READ | VM_PROT_WRITE, pmapflags);
	}
	pmap_update(pmap_kernel());

	return 0;
}

bool
bus_space_is_equal(bus_space_tag_t t1, bus_space_tag_t t2)
{
	if (t1 == NULL || t2 == NULL)
		return false;
	return t1->bst_type == t2->bst_type;
}

/*
 * void _x86_memio_unmap(bus_space_tag bst, bus_space_handle bsh,
 *                        bus_size_t size, bus_addr_t *adrp)
 *
 *   This function unmaps memory- or io-space mapped by the function
 *   _x86_memio_map().  This function works nearly as same as
 *   x86_memio_unmap(), but this function does not ask kernel
 *   built-in extents and returns physical address of the bus space,
 *   for the convenience of the extra extent manager.
 */
void
_x86_memio_unmap(bus_space_tag_t t, bus_space_handle_t bsh,
		bus_size_t size, bus_addr_t *adrp)
{
	u_long va, endva;
	bus_addr_t bpa;

	/*
	 * Find the correct extent and bus physical address.
	 */
	if (x86_bus_space_is_io(t)) {
		bpa = bsh;
	} else if (x86_bus_space_is_mem(t)) {
		if (bsh >= atdevbase && (bsh + size) != 0 &&
		    (bsh + size) <= (atdevbase + IOM_SIZE)) {
			bpa = (bus_addr_t)ISA_PHYSADDR(bsh);
		} else {

			va = x86_trunc_page(bsh);
			endva = x86_round_page(bsh + size);

#ifdef DIAGNOSTIC
			if (endva <= va) {
				panic("_x86_memio_unmap: overflow");
			}
#endif

			if (pmap_extract_ma(pmap_kernel(), va, &bpa) == FALSE) {
				panic("_x86_memio_unmap:"
				    " wrong virtual address");
			}
			bpa += (bsh & PGOFSET);
			pmap_kremove(va, endva - va);
			pmap_update(pmap_kernel());

			/*
			 * Free the kernel virtual mapping.
			 */
			uvm_km_free(kernel_map, va, endva - va, UVM_KMF_VAONLY);
		}
	} else {
		panic("_x86_memio_unmap: bad bus space tag");
	}

	if (adrp != NULL) {
		*adrp = bpa;
	}
}

static void
bus_space_reservation_unmap1(bus_space_tag_t t, const bus_space_handle_t bsh,
    const bus_size_t size, bus_addr_t *bpap)
{
	u_long va, endva;
	bus_addr_t bpa;

	/*
	 * Find the correct extent and bus physical address.
	 */
	if (x86_bus_space_is_io(t)) {
		bpa = bsh;
	} else if (x86_bus_space_is_mem(t)) {
		if (bsh >= atdevbase && (bsh + size) != 0 &&
		    (bsh + size) <= (atdevbase + IOM_SIZE)) {
			bpa = (bus_addr_t)ISA_PHYSADDR(bsh);
			goto ok;
		}

		va = x86_trunc_page(bsh);
		endva = x86_round_page(bsh + size);

#ifdef DIAGNOSTIC
		if (endva <= va)
			panic("x86_memio_unmap: overflow");
#endif

		(void) pmap_extract_ma(pmap_kernel(), va, &bpa);
		bpa += (bsh & PGOFSET);

		pmap_kremove(va, endva - va);
		pmap_update(pmap_kernel());

		/*
		 * Free the kernel virtual mapping.
		 */
		uvm_km_free(kernel_map, va, endva - va, UVM_KMF_VAONLY);
	} else
		panic("x86_memio_unmap: bad bus space tag");
ok:
	if (bpap != NULL)
		*bpap = bpa;
}

void
bus_space_reservation_unmap(bus_space_tag_t t, const bus_space_handle_t bsh,
    const bus_size_t size)
{
	bus_space_tag_t it;

	if ((t->bst_exists & BUS_SPACE_OVERRIDE_RESERVATION_UNMAP) == 0)
		;	/* skip override */
	else for (it = t; it != NULL; it = it->bst_super) {
		if ((it->bst_present & BUS_SPACE_OVERRIDE_RESERVATION_UNMAP) ==
		    0)
			continue;
		(*it->bst_ov->ov_space_reservation_unmap)(it->bst_ctx,
		    t, bsh, size);
		return;
	}

	bus_space_reservation_unmap1(t, bsh, size, NULL);
}

void
bus_space_unmap(bus_space_tag_t t, const bus_space_handle_t bsh,
    const bus_size_t size)
{
	bus_addr_t addr;
	bus_space_reservation_t bsr;
	bus_space_tag_t it;

	if ((t->bst_exists & BUS_SPACE_OVERRIDE_UNMAP) == 0)
		;	/* skip override */
	else for (it = t; it != NULL; it = it->bst_super) {
		if ((it->bst_present & BUS_SPACE_OVERRIDE_UNMAP) == 0)
			continue;
		(*it->bst_ov->ov_space_unmap)(it->bst_ctx, t, bsh, size);
		return;
	}

	bus_space_reservation_unmap1(t, bsh, size, &addr);

	bus_space_reservation_init(&bsr, addr, size);
	bus_space_release(t, &bsr);
}

void
bus_space_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	bus_space_tag_t it;

	if ((t->bst_exists & BUS_SPACE_OVERRIDE_FREE) == 0)
		;	/* skip override */
	else for (it = t; it != NULL; it = it->bst_super) {
		if ((it->bst_present & BUS_SPACE_OVERRIDE_FREE) == 0)
			continue;
		(*it->bst_ov->ov_space_free)(it->bst_ctx, t, bsh, size);
		return;
	}
	/* bus_space_unmap() does all that we need to do. */
	bus_space_unmap(t, bsh, size);
}

int
bus_space_subregion(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return (0);
}

paddr_t
bus_space_mmap(bus_space_tag_t t, bus_addr_t addr, off_t off, int prot,
    int flags)
{
	paddr_t pflags = 0;

	/* Can't mmap I/O space. */
	if (x86_bus_space_is_io(t))
		return (-1);

	/*
	 * "addr" is the base address of the device we're mapping.
	 * "off" is the offset into that device.
	 *
	 * Note we are called for each "page" in the device that
	 * the upper layers want to map.
	 */
	if (flags & BUS_SPACE_MAP_PREFETCHABLE)
		pflags |= X86_MMAP_FLAG_PREFETCH;

	return x86_btop(addr + off) | (pflags << X86_MMAP_FLAG_SHIFT);
}

void
bus_space_set_multi_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
		      uint8_t v, size_t c)
{
	vaddr_t addr = h + o;

	if (x86_bus_space_is_io(t))
		while (c--)
			outb(addr, v);
	else
		while (c--)
			*(volatile uint8_t *)(addr) = v;
}

void
bus_space_set_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
		      uint16_t v, size_t c)
{
	vaddr_t addr = h + o;

	BUS_SPACE_ADDRESS_SANITY(addr, uint16_t, "bus addr");

	if (x86_bus_space_is_io(t))
		while (c--)
			outw(addr, v);
	else
		while (c--)
			*(volatile uint16_t *)(addr) = v;
}

void
bus_space_set_multi_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
		      uint32_t v, size_t c)
{
	vaddr_t addr = h + o;

	BUS_SPACE_ADDRESS_SANITY(addr, uint32_t, "bus addr");

	if (x86_bus_space_is_io(t))
		while (c--)
			outl(addr, v);
	else
		while (c--)
			*(volatile uint32_t *)(addr) = v;
}

void
bus_space_set_region_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
		      uint8_t v, size_t c)
{
	vaddr_t addr = h + o;

	if (x86_bus_space_is_io(t))
		for (; c != 0; c--, addr++)
			outb(addr, v);
	else
		for (; c != 0; c--, addr++)
			*(volatile uint8_t *)(addr) = v;
}

void
bus_space_set_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
		       uint16_t v, size_t c)
{
	vaddr_t addr = h + o;

	BUS_SPACE_ADDRESS_SANITY(addr, uint16_t, "bus addr");

	if (x86_bus_space_is_io(t))
		for (; c != 0; c--, addr += 2)
			outw(addr, v);
	else
		for (; c != 0; c--, addr += 2)
			*(volatile uint16_t *)(addr) = v;
}

void
bus_space_set_region_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
		       uint32_t v, size_t c)
{
	vaddr_t addr = h + o;

	BUS_SPACE_ADDRESS_SANITY(addr, uint32_t, "bus addr");

	if (x86_bus_space_is_io(t))
		for (; c != 0; c--, addr += 4)
			outl(addr, v);
	else
		for (; c != 0; c--, addr += 4)
			*(volatile uint32_t *)(addr) = v;
}

void
bus_space_copy_region_1(bus_space_tag_t t, bus_space_handle_t h1,
			bus_size_t o1, bus_space_handle_t h2,
			bus_size_t o2, size_t c)
{
	vaddr_t addr1 = h1 + o1;
	vaddr_t addr2 = h2 + o2;

	if (x86_bus_space_is_io(t)) {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1++, addr2++)
				outb(addr2, inb(addr1));
		} else {
			/* dest after src: copy backwards */
			for (addr1 += (c - 1), addr2 += (c - 1);
			    c != 0; c--, addr1--, addr2--)
				outb(addr2, inb(addr1));
		}
	} else {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1++, addr2++)
				*(volatile uint8_t *)(addr2) =
				    *(volatile uint8_t *)(addr1);
		} else {
			/* dest after src: copy backwards */
			for (addr1 += (c - 1), addr2 += (c - 1);
			    c != 0; c--, addr1--, addr2--)
				*(volatile uint8_t *)(addr2) =
				    *(volatile uint8_t *)(addr1);
		}
	}
}

void
bus_space_copy_region_2(bus_space_tag_t t, bus_space_handle_t h1,
			bus_size_t o1, bus_space_handle_t h2,
			bus_size_t o2, size_t c)
{
	vaddr_t addr1 = h1 + o1;
	vaddr_t addr2 = h2 + o2;

	BUS_SPACE_ADDRESS_SANITY(addr1, uint16_t, "bus addr 1");
	BUS_SPACE_ADDRESS_SANITY(addr2, uint16_t, "bus addr 2");

	if (x86_bus_space_is_io(t)) {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1 += 2, addr2 += 2)
				outw(addr2, inw(addr1));
		} else {
			/* dest after src: copy backwards */
			for (addr1 += 2 * (c - 1), addr2 += 2 * (c - 1);
			    c != 0; c--, addr1 -= 2, addr2 -= 2)
				outw(addr2, inw(addr1));
		}
	} else {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1 += 2, addr2 += 2)
				*(volatile uint16_t *)(addr2) =
				    *(volatile uint16_t *)(addr1);
		} else {
			/* dest after src: copy backwards */
			for (addr1 += 2 * (c - 1), addr2 += 2 * (c - 1);
			    c != 0; c--, addr1 -= 2, addr2 -= 2)
				*(volatile uint16_t *)(addr2) =
				    *(volatile uint16_t *)(addr1);
		}
	}
}

void
bus_space_copy_region_4(bus_space_tag_t t, bus_space_handle_t h1,
			bus_size_t o1, bus_space_handle_t h2,
			bus_size_t o2, size_t c)
{
	vaddr_t addr1 = h1 + o1;
	vaddr_t addr2 = h2 + o2;

	BUS_SPACE_ADDRESS_SANITY(addr1, uint32_t, "bus addr 1");
	BUS_SPACE_ADDRESS_SANITY(addr2, uint32_t, "bus addr 2");

	if (x86_bus_space_is_io(t)) {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1 += 4, addr2 += 4)
				outl(addr2, inl(addr1));
		} else {
			/* dest after src: copy backwards */
			for (addr1 += 4 * (c - 1), addr2 += 4 * (c - 1);
			    c != 0; c--, addr1 -= 4, addr2 -= 4)
				outl(addr2, inl(addr1));
		}
	} else {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1 += 4, addr2 += 4)
				*(volatile uint32_t *)(addr2) =
				    *(volatile uint32_t *)(addr1);
		} else {
			/* dest after src: copy backwards */
			for (addr1 += 4 * (c - 1), addr2 += 4 * (c - 1);
			    c != 0; c--, addr1 -= 4, addr2 -= 4)
				*(volatile uint32_t *)(addr2) =
				    *(volatile uint32_t *)(addr1);
		}
	}
}

void
bus_space_barrier(bus_space_tag_t tag, bus_space_handle_t bsh,
		  bus_size_t offset, bus_size_t len, int flags)
{

	/* Function call is enough to prevent reordering of loads. */
}

void *
bus_space_vaddr(bus_space_tag_t tag, bus_space_handle_t bsh)
{

	return x86_bus_space_is_mem(tag) ? (void *)bsh : NULL;
}

static const void *
bit_to_function_pointer(const struct bus_space_overrides *ov, uint64_t bit)
{
	switch (bit) {
	case BUS_SPACE_OVERRIDE_MAP:
		return ov->ov_space_map;
	case BUS_SPACE_OVERRIDE_UNMAP:
		return ov->ov_space_unmap;
	case BUS_SPACE_OVERRIDE_ALLOC:
		return ov->ov_space_alloc;
	case BUS_SPACE_OVERRIDE_FREE:
		return ov->ov_space_free;
	case BUS_SPACE_OVERRIDE_RESERVE:
		return ov->ov_space_reserve;
	case BUS_SPACE_OVERRIDE_RELEASE:
		return ov->ov_space_release;
	case BUS_SPACE_OVERRIDE_RESERVATION_MAP:
		return ov->ov_space_reservation_map;
	case BUS_SPACE_OVERRIDE_RESERVATION_UNMAP:
		return ov->ov_space_reservation_unmap;
	case BUS_SPACE_OVERRIDE_RESERVE_SUBREGION:
		return ov->ov_space_reserve_subregion;
	default:
		return NULL;
	}
}

void
bus_space_tag_destroy(bus_space_tag_t bst)
{
	kmem_free(bst, sizeof(struct bus_space_tag));
}

int
bus_space_tag_create(bus_space_tag_t obst, const uint64_t present,
    const uint64_t extpresent, const struct bus_space_overrides *ov, void *ctx,
    bus_space_tag_t *bstp)
{
	uint64_t bit, bits, nbits;
	bus_space_tag_t bst;
	const void *fp;

	if (ov == NULL || present == 0 || extpresent != 0)
		return EINVAL;

	bst = kmem_alloc(sizeof(struct bus_space_tag), KM_SLEEP);

	if (bst == NULL)
		return ENOMEM;

	bst->bst_super = obst;
	bst->bst_type = obst->bst_type;

	for (bits = present; bits != 0; bits = nbits) {
		nbits = bits & (bits - 1);
		bit = nbits ^ bits;
		if ((fp = bit_to_function_pointer(ov, bit)) == NULL) {
			printf("%s: missing bit %" PRIx64 "\n", __func__, bit);
			goto einval;
		}
	}

	bst->bst_ov = ov;
	bst->bst_exists = obst->bst_exists | present;
	bst->bst_present = present;
	bst->bst_ctx = ctx;

	*bstp = bst;

	return 0;
einval:
	kmem_free(bst, sizeof(struct bus_space_tag));
	return EINVAL;
}
