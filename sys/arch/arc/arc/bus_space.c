/*	$NetBSD: bus_space.c,v 1.11.2.1 2012/04/17 00:06:03 yamt Exp $	*/
/*	NetBSD: bus_machdep.c,v 1.1 2000/01/26 18:48:00 drochner Exp 	*/

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
__KERNEL_RCSID(0, "$NetBSD: bus_space.c,v 1.11.2.1 2012/04/17 00:06:03 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>

/*
 *	uintN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

#define bus_space_read(BYTES,BITS)					\
__CONCAT3(uint,BITS,_t)					\
__CONCAT(bus_space_read_,BYTES)(bus_space_tag_t bst,			\
    bus_space_handle_t bsh, bus_size_t offset)				\
{									\
	return (*(volatile __CONCAT3(uint,BITS,_t) *)			\
	    (bsh + (offset << __CONCAT(bst->bs_stride_,BYTES))));	\
}

bus_space_read(1,8)
bus_space_read(2,16)
bus_space_read(4,32)
bus_space_read(8,64)

/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

#define bus_space_read_multi(BYTES,BITS)				\
void							\
__CONCAT(bus_space_read_multi_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t bsh, bus_size_t offset,				\
    __CONCAT3(uint,BITS,_t) *datap, bus_size_t count)			\
{									\
	volatile __CONCAT3(uint,BITS,_t) *p =				\
	    (volatile __CONCAT3(uint,BITS,_t) *)			\
	    (bsh + (offset << __CONCAT(bst->bs_stride_,BYTES)));	\
									\
	for (; count > 0; --count)					\
		*datap++ = *p;						\
}

bus_space_read_multi(1,8)
bus_space_read_multi(2,16)
bus_space_read_multi(4,32)
bus_space_read_multi(8,64)

/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */

#define bus_space_read_region(BYTES,BITS)				\
void							\
__CONCAT(bus_space_read_region_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t bsh, bus_size_t offset,				\
    __CONCAT3(uint,BITS,_t) *datap, bus_size_t count)			\
{									\
	int stride = 1 << __CONCAT(bst->bs_stride_,BYTES);		\
	volatile __CONCAT3(uint,BITS,_t) *p =				\
	    (volatile __CONCAT3(uint,BITS,_t) *)			\
	    (bsh + (offset << __CONCAT(bst->bs_stride_,BYTES)));	\
									\
	for (; count > 0; --count) {					\
		*datap++ = *p;						\
		p += stride;						\
	}								\
}

bus_space_read_region(1,8)
bus_space_read_region(2,16)
bus_space_read_region(4,32)
bus_space_read_region(8,64)

/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

#define bus_space_write(BYTES,BITS)					\
void							\
__CONCAT(bus_space_write_,BYTES)(bus_space_tag_t bst,			\
    bus_space_handle_t bsh,						\
    bus_size_t offset, __CONCAT3(uint,BITS,_t) data)			\
{									\
	*(volatile __CONCAT3(uint,BITS,_t) *)				\
	    (bsh + (offset << __CONCAT(bst->bs_stride_,BYTES))) = data; \
}

bus_space_write(1,8)
bus_space_write(2,16)
bus_space_write(4,32)
bus_space_write(8,64)

/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const uintN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

#define bus_space_write_multi(BYTES,BITS)				\
void							\
__CONCAT(bus_space_write_multi_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t bsh, bus_size_t offset,				\
    const __CONCAT3(uint,BITS,_t) *datap, bus_size_t count)		\
{									\
	volatile __CONCAT3(uint,BITS,_t) *p =				\
	    (volatile __CONCAT3(uint,BITS,_t) *)			\
	    (bsh + (offset << __CONCAT(bst->bs_stride_,BYTES)));	\
									\
	for (; count > 0; --count)					\
		*p = *datap++;						\
}

bus_space_write_multi(1,8)
bus_space_write_multi(2,16)
bus_space_write_multi(4,32)
bus_space_write_multi(8,64)

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const uintN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */

#define bus_space_write_region(BYTES,BITS)				\
void							\
__CONCAT(bus_space_write_region_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t bsh, bus_size_t offset,				\
    const __CONCAT3(uint,BITS,_t) *datap, bus_size_t count)		\
{									\
	int stride = 1 << __CONCAT(bst->bs_stride_,BYTES);		\
	volatile __CONCAT3(uint,BITS,_t) *p =				\
	    (volatile __CONCAT3(uint,BITS,_t) *)			\
	    (bsh + (offset << __CONCAT(bst->bs_stride_,BYTES)));	\
									\
	for (; count > 0; --count) {					\
		*p = *datap++;						\
		p += stride;						\
	}								\
}

bus_space_write_region(1,8)
bus_space_write_region(2,16)
bus_space_write_region(4,32)
bus_space_write_region(8,64)

/*
 *	void bus_space_set_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
 *	    size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */

#define bus_space_set_multi(BYTES,BITS)					\
void							\
__CONCAT(bus_space_set_multi_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t bsh, bus_size_t offset,				\
    const __CONCAT3(uint,BITS,_t) data, bus_size_t count)		\
{									\
	volatile __CONCAT3(uint,BITS,_t) *p =				\
	    (volatile __CONCAT3(uint,BITS,_t) *)			\
	    (bsh + (offset << __CONCAT(bst->bs_stride_,BYTES)));	\
									\
	for (; count > 0; --count)					\
		*p = data;						\
}

bus_space_set_multi(1,8)
bus_space_set_multi(2,16)
bus_space_set_multi(4,32)
bus_space_set_multi(8,64)

/*
 *	void bus_space_set_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
 *	    size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */

#define bus_space_set_region(BYTES,BITS)				\
void							\
__CONCAT(bus_space_set_region_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t bsh, bus_size_t offset,				\
    __CONCAT3(uint,BITS,_t) data, bus_size_t count)			\
{									\
	int stride = 1 << __CONCAT(bst->bs_stride_,BYTES);		\
	volatile __CONCAT3(uint,BITS,_t) *p =				\
	    (volatile __CONCAT3(uint,BITS,_t) *)			\
	    (bsh + (offset << __CONCAT(bst->bs_stride_,BYTES)));	\
									\
	for (; count > 0; --count) {					\
		*p = data;						\
		p += stride;						\
	}								\
}

bus_space_set_region(1,8)
bus_space_set_region(2,16)
bus_space_set_region(4,32)
bus_space_set_region(8,64)

/*
 *	void bus_space_copy_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */

#define bus_space_copy_region(BYTES,BITS)				\
void							\
__CONCAT(bus_space_copy_region_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t srcbsh, bus_size_t srcoffset,			\
    bus_space_handle_t dstbsh, bus_size_t dstoffset, bus_size_t count)	\
{									\
	int stride = 1 << __CONCAT(bst->bs_stride_,BYTES);		\
	volatile __CONCAT3(uint,BITS,_t) *srcp =			\
	    (volatile __CONCAT3(uint,BITS,_t) *)			\
	    (srcbsh + (srcoffset << __CONCAT(bst->bs_stride_,BYTES)));	\
	volatile __CONCAT3(uint,BITS,_t) *dstp =			\
	    (volatile __CONCAT3(uint,BITS,_t) *)			\
	    (dstbsh + (dstoffset << __CONCAT(bst->bs_stride_,BYTES)));	\
	bus_size_t offset;						\
									\
	if (srcp >= dstp) {						\
		/* src after dest: copy forward */			\
		for (offset = 0; count > 0; --count, offset += stride)	\
			dstp[offset] = srcp[offset];			\
	} else {							\
		/* dest after src: copy backward */			\
		offset = (count << __CONCAT(bst->bs_stride_,BYTES))	\
		    - stride;						\
		for (; count > 0; --count, offset -= stride)		\
			dstp[offset] = srcp[offset];			\
	}								\
}

bus_space_copy_region(1,8)
bus_space_copy_region(2,16)
bus_space_copy_region(4,32)
bus_space_copy_region(8,64)

void
arc_bus_space_init(bus_space_tag_t bst, const char *name, paddr_t paddr,
    vaddr_t vaddr, bus_addr_t start, bus_size_t size)
{

	bst->bs_name = name;
	bst->bs_extent = NULL;
	bst->bs_start = start;
	bst->bs_size = size;
	bst->bs_pbase = paddr;
	bst->bs_vbase = vaddr;
	bst->bs_compose_handle = arc_bus_space_compose_handle;
	bst->bs_dispose_handle = arc_bus_space_dispose_handle;
	bst->bs_paddr = arc_bus_space_paddr;
	bst->bs_map = arc_bus_space_map;
	bst->bs_unmap = arc_bus_space_unmap;
	bst->bs_subregion = arc_bus_space_subregion;
	bst->bs_mmap = arc_bus_space_mmap;
	bst->bs_alloc = arc_bus_space_alloc;
	bst->bs_free = arc_bus_space_free;
	bst->bs_aux = NULL;
	bst->bs_stride_1 = 0;
	bst->bs_stride_2 = 0;
	bst->bs_stride_4 = 0;
	bst->bs_stride_8 = 0;
}

void
arc_bus_space_init_extent(bus_space_tag_t bst, void *storage,
    size_t storagesize)
{

	bst->bs_extent = extent_create(bst->bs_name,
	    bst->bs_start, bst->bs_start + bst->bs_size,
	    storage, storagesize, EX_NOWAIT);
	if (bst->bs_extent == NULL)
		panic("arc_bus_space_init_extent: cannot create extent map %s",
		    bst->bs_name);
}

void
arc_bus_space_set_aligned_stride(bus_space_tag_t bst,
    unsigned int alignment_shift)
{

	bst->bs_stride_1 = alignment_shift;
	if (alignment_shift > 0)
		--alignment_shift;
	bst->bs_stride_2 = alignment_shift;
	if (alignment_shift > 0)
		--alignment_shift;
	bst->bs_stride_4 = alignment_shift;
	if (alignment_shift > 0)
		--alignment_shift;
	bst->bs_stride_8 = alignment_shift;
}

static int malloc_safe = 0;

void
arc_bus_space_malloc_set_safe(void)
{

	malloc_safe = EX_MALLOCOK;
}

int
arc_bus_space_extent_malloc_flag(void)
{

	return malloc_safe;
}

int
arc_bus_space_compose_handle(bus_space_tag_t bst, bus_addr_t addr,
    bus_size_t size, int flags, bus_space_handle_t *bshp)
{
	bus_space_handle_t bsh = bst->bs_vbase + (addr - bst->bs_start);

	/*
	 * Since all buses can be linearly mappable, we don't have to check
	 * BUS_SPACE_MAP_LINEAR and BUS_SPACE_MAP_PREFETCHABLE.
	 */
	if ((flags & BUS_SPACE_MAP_CACHEABLE) == 0) {
		*bshp = bsh;
		return 0;
	}
	if (bsh < MIPS_KSEG1_START) /* KUSEG or KSEG0 */
		panic("arc_bus_space_compose_handle: bad address 0x%x", bsh);
	if (bsh < MIPS_KSEG2_START) { /* KSEG1 */
		*bshp = MIPS_PHYS_TO_KSEG0(MIPS_KSEG1_TO_PHYS(bsh));
		return 0;
	}
	/*
	 * KSEG2:
	 * Do not make the page cacheable in this case, since:
	 * - the page which this bus_space belongs might include
	 *   other bus_spaces.
	 * or
	 * - this bus might be mapped by wired TLB, in that case,
	 *   we cannot manupulate cacheable attribute with page granularity.
	 */
#ifdef DIAGNOSTIC
	printf("arc_bus_space_compose_handle: ignore cacheable 0x%x\n", bsh);
#endif
	*bshp = bsh;
	return 0;
}

int
arc_bus_space_dispose_handle(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t size)
{

	return 0;
}

int
arc_bus_space_paddr(bus_space_tag_t bst, bus_space_handle_t bsh, paddr_t *pap)
{

	if (bsh < MIPS_KSEG0_START) /* KUSEG */
		panic("arc_bus_space_paddr(0x%qx): bad address",
		    (unsigned long long) bsh);
	else if (bsh < MIPS_KSEG1_START) /* KSEG0 */
		*pap = MIPS_KSEG0_TO_PHYS(bsh);
	else if (bsh < MIPS_KSEG2_START) /* KSEG1 */
		*pap = MIPS_KSEG1_TO_PHYS(bsh);
	else { /* KSEG2 */
		/*
		 * Since this region may be mapped by wired TLB,
		 * kvtophys() is not always available.
		 */
		*pap = bst->bs_pbase + (bsh - bst->bs_vbase);
	}
	return 0;
}

int
arc_bus_space_map(bus_space_tag_t bst, bus_addr_t addr, bus_size_t size,
    int flags, bus_space_handle_t *bshp)
{
	int err;

	if (addr < bst->bs_start || addr + size > bst->bs_start + bst->bs_size)
		return EINVAL;

	if (bst->bs_extent != NULL) {
		err = extent_alloc_region(bst->bs_extent, addr, size,
		    EX_NOWAIT | malloc_safe);
		if (err)
			return err;
	}

	return bus_space_compose_handle(bst, addr, size, flags, bshp);
}

void
arc_bus_space_unmap(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t size)
{

	if (bst->bs_extent != NULL) {
		paddr_t pa;
		bus_addr_t addr;
		int err;

		/* bus_space_paddr() becomes unavailable after unmapping */
		err = bus_space_paddr(bst, bsh, &pa);
		if (err)
			panic("arc_bus_space_unmap: %s va 0x%qx: error %d",
			    bst->bs_name, (unsigned long long) bsh, err);
		addr = (bus_size_t)(pa - bst->bs_pbase) + bst->bs_start;
		extent_free(bst->bs_extent, addr, size,
		    EX_NOWAIT | malloc_safe);
	}
	bus_space_dispose_handle(bst, bsh, size);
}

int
arc_bus_space_subregion(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return 0;
}

paddr_t
arc_bus_space_mmap(bus_space_tag_t bst, bus_addr_t addr, off_t off, int prot,
    int flags)
{

	/*
	 * XXX We do not disallow mmap'ing of EISA/PCI I/O space here,
	 * XXX which we should be doing.
	 */

	if (addr < bst->bs_start ||
	    (addr + off) >= (bst->bs_start + bst->bs_size))
		return -1;

	return mips_btop(bst->bs_pbase + (addr - bst->bs_start) + off);
}

int
arc_bus_space_alloc(bus_space_tag_t bst, bus_addr_t start, bus_addr_t end,
    bus_size_t size, bus_size_t align, bus_size_t boundary, int flags,
    bus_addr_t *addrp, bus_space_handle_t *bshp)
{
	u_long addr;
	int err;

	if (bst->bs_extent == NULL)
		panic("arc_bus_space_alloc: extent map %s not available",
		    bst->bs_name);

	if (start < bst->bs_start ||
	    start + size > bst->bs_start + bst->bs_size)
		return EINVAL;

	err = extent_alloc_subregion(bst->bs_extent, start, end, size,
	    align, boundary, EX_FAST | EX_NOWAIT | malloc_safe, &addr);
	if (err)
		return err;

	*addrp = addr;
	return bus_space_compose_handle(bst, addr, size, flags, bshp);
}
