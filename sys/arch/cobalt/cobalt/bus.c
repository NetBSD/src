/*	$NetBSD: bus.c,v 1.44.10.1 2014/08/10 06:53:53 tls Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
__KERNEL_RCSID(0, "$NetBSD: bus.c,v 1.44.10.1 2014/08/10 06:53:53 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#include <dev/bus_dma/bus_dmamem_common.h>

#include <uvm/uvm_extern.h>

#include <mips/cache.h>

/*
 * Utility macros; do not use outside this file.
 */
#define	__PB_TYPENAME_PREFIX(BITS)	___CONCAT(uint,BITS)
#define	__PB_TYPENAME(BITS)		___CONCAT(__PB_TYPENAME_PREFIX(BITS),_t)

/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t *addr, bus_size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

#define __COBALT_bus_space_read_multi(BYTES,BITS)			\
void __CONCAT(bus_space_read_multi_,BYTES)				\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	__PB_TYPENAME(BITS) *, bus_size_t);				\
									\
void									\
__CONCAT(bus_space_read_multi_,BYTES)(					\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	__PB_TYPENAME(BITS) *a,						\
	bus_size_t c)							\
{									\
									\
	while (c--)							\
		*a++ = __CONCAT(bus_space_read_,BYTES)(t, h, o);	\
}

__COBALT_bus_space_read_multi(1,8)
__COBALT_bus_space_read_multi(2,16)
__COBALT_bus_space_read_multi(4,32)

#if 0	/* Cause a link error for bus_space_read_multi_8 */
#define	bus_space_read_multi_8	!!! bus_space_read_multi_8 unimplemented !!!
#endif

#undef __COBALT_bus_space_read_multi

/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t *addr, bus_size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */

#define __COBALT_bus_space_read_region(BYTES,BITS)			\
void __CONCAT(bus_space_read_region_,BYTES)				\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	__PB_TYPENAME(BITS) *, bus_size_t);				\
									\
void									\
__CONCAT(bus_space_read_region_,BYTES)(					\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	__PB_TYPENAME(BITS) *a,						\
	bus_size_t c)							\
{									\
									\
	while (c--) {							\
		*a++ = __CONCAT(bus_space_read_,BYTES)(t, h, o);	\
		o += BYTES;						\
	}								\
}

__COBALT_bus_space_read_region(1,8)
__COBALT_bus_space_read_region(2,16)
__COBALT_bus_space_read_region(4,32)

#if 0	/* Cause a link error for bus_space_read_region_8 */
#define	bus_space_read_region_8	!!! bus_space_read_region_8 unimplemented !!!
#endif

#undef __COBALT_bus_space_read_region

/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const uintN_t *addr, bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

#define __COBALT_bus_space_write_multi(BYTES,BITS)			\
void __CONCAT(bus_space_write_multi_,BYTES)				\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	const __PB_TYPENAME(BITS) *, bus_size_t);			\
									\
void									\
__CONCAT(bus_space_write_multi_,BYTES)(					\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	const __PB_TYPENAME(BITS) *a,					\
	bus_size_t c)							\
{									\
									\
	while (c--)							\
		__CONCAT(bus_space_write_,BYTES)(t, h, o, *a++);	\
}

__COBALT_bus_space_write_multi(1,8)
__COBALT_bus_space_write_multi(2,16)
__COBALT_bus_space_write_multi(4,32)

#if 0	/* Cause a link error for bus_space_write_8 */
#define	bus_space_write_multi_8(t, h, o, a, c)				\
			!!! bus_space_write_multi_8 unimplimented !!!
#endif

#undef __COBALT_bus_space_write_multi

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const uintN_t *addr, bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */

#define __COBALT_bus_space_write_region(BYTES,BITS)			\
void __CONCAT(bus_space_write_region_,BYTES)				\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	const __PB_TYPENAME(BITS) *, bus_size_t);			\
									\
void									\
__CONCAT(bus_space_write_region_,BYTES)(				\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	const __PB_TYPENAME(BITS) *a,					\
	bus_size_t c)							\
{									\
									\
	while (c--) {							\
		__CONCAT(bus_space_write_,BYTES)(t, h, o, *a++);	\
		o += BYTES;						\
	}								\
}

__COBALT_bus_space_write_region(1,8)
__COBALT_bus_space_write_region(2,16)
__COBALT_bus_space_write_region(4,32)

#if 0	/* Cause a link error for bus_space_write_region_8 */
#define	bus_space_write_region_8					\
			!!! bus_space_write_region_8 unimplemented !!!
#endif

#undef __COBALT_bus_space_write_region

/*
 *	void bus_space_set_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
 *	    bus_size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */

#define __COBALT_bus_space_set_multi(BYTES,BITS)			\
void __CONCAT(bus_space_set_multi_,BYTES)				\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	__PB_TYPENAME(BITS), bus_size_t);				\
									\
void									\
__CONCAT(bus_space_set_multi_,BYTES)(					\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	__PB_TYPENAME(BITS) v,						\
	bus_size_t c)							\
{									\
									\
	while (c--)							\
		__CONCAT(bus_space_write_,BYTES)(t, h, o, v);		\
}

__COBALT_bus_space_set_multi(1,8)
__COBALT_bus_space_set_multi(2,16)
__COBALT_bus_space_set_multi(4,32)

#if 0	/* Cause a link error for bus_space_set_multi_8 */
#define	bus_space_set_multi_8						\
			!!! bus_space_set_multi_8 unimplemented !!!
#endif

#undef __COBALT_bus_space_set_multi

/*
 *	void bus_space_set_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
 *	    bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */

#define __COBALT_bus_space_set_region(BYTES,BITS)			\
void __CONCAT(bus_space_set_region_,BYTES)				\
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,		\
	__PB_TYPENAME(BITS), bus_size_t);				\
									\
void									\
__CONCAT(bus_space_set_region_,BYTES)(					\
	bus_space_tag_t t,						\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	__PB_TYPENAME(BITS) v,						\
	bus_size_t c)							\
{									\
									\
	while (c--) {							\
		__CONCAT(bus_space_write_,BYTES)(t, h, o, v);		\
		o += BYTES;						\
	}								\
}

__COBALT_bus_space_set_region(1,8)
__COBALT_bus_space_set_region(2,16)
__COBALT_bus_space_set_region(4,32)

#if 0	/* Cause a link error for bus_space_set_region_8 */
#define	bus_space_set_region_8						\
			!!! bus_space_set_region_8 unimplemented !!!
#endif

#undef __COBALT_bus_space_set_region

/*
 *	void bus_space_copy_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    bus_size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */

#define	__COBALT_copy_region(BYTES)					\
void __CONCAT(bus_space_copy_region_,BYTES)				\
	(bus_space_tag_t,						\
	    bus_space_handle_t bsh1, bus_size_t off1,			\
	    bus_space_handle_t bsh2, bus_size_t off2,			\
	    bus_size_t count);						\
									\
void									\
__CONCAT(bus_space_copy_region_,BYTES)(					\
	bus_space_tag_t t,						\
	bus_space_handle_t h1,						\
	bus_size_t o1,							\
	bus_space_handle_t h2,						\
	bus_size_t o2,							\
	bus_size_t c)							\
{									\
	bus_size_t o;							\
									\
	if ((h1 + o1) >= (h2 + o2)) {					\
		/* src after dest: copy forward */			\
		for (o = 0; c != 0; c--, o += BYTES)			\
			__CONCAT(bus_space_write_,BYTES)(t, h2, o2 + o,	\
			    __CONCAT(bus_space_read_,BYTES)(t, h1, o1 + o)); \
	} else {							\
		/* dest after src: copy backwards */			\
		for (o = (c - 1) * BYTES; c != 0; c--, o -= BYTES)	\
			__CONCAT(bus_space_write_,BYTES)(t, h2, o2 + o,	\
			    __CONCAT(bus_space_read_,BYTES)(t, h1, o1 + o)); \
	}								\
}

__COBALT_copy_region(1)
__COBALT_copy_region(2)
__COBALT_copy_region(4)

#if 0	/* Cause a link error for bus_space_copy_region_8 */
#define	bus_space_copy_region_8						\
			!!! bus_space_copy_region_8 unimplemented !!!
#endif

#undef __COBALT_copy_region

int
bus_space_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp)
{
	int cacheable = flags & BUS_SPACE_MAP_CACHEABLE;

	if (cacheable)
		*bshp = MIPS_PHYS_TO_KSEG0(bpa);
	else
		*bshp = MIPS_PHYS_TO_KSEG1(bpa);

	/* XXX Evil! */
	if (bpa < 0x10000000)
		*bshp += 0x10000000;

	return 0;
}

int
bus_space_alloc(bus_space_tag_t t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{

	panic("bus_space_alloc: not implemented");
}

void
bus_space_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{

	panic("bus_space_free: not implemented");
}

void
bus_space_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{

	return;
}

int
bus_space_subregion(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return 0;
}

paddr_t
bus_space_mmap(bus_space_tag_t t, bus_addr_t addr, off_t off, int prot,
    int flags)
{

	/* XXX not implemented */
	return -1;
}
