/*	$NetBSD: bus.h,v 1.24.10.1 2007/06/05 20:25:43 matt Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2001 The NetBSD Foundation, Inc.
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

/*
 * Copyright (c) 1996 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MACPPC_BUS_H_
#define _MACPPC_BUS_H_

#include <machine/pio.h>

/*
 * Values for the macppc bus space tag, not to be used directly by MI code.
 */

#define __BUS_SPACE_HAS_STREAM_METHODS
#define __HAVE_LOCAL_BUS_SPACE

#define MACPPC_BUS_ADDR_MASK	0xfffff000
#define MACPPC_BUS_STRIDE_MASK	0x0000000f

#define	PHYS_TO_BUS_MEM(t, addr)	(addr)
#define	BUS_MEM_TO_PHYS(t, addr)	(addr)

#include <powerpc/bus.h>

#define macppc_make_bus_space_tag(addr, stride) \
	(((addr) & MACPPC_BUS_ADDR_MASK) | (stride))
#define __BA(t, h, o) ((void *)((h) + ((o) << ((t) & MACPPC_BUS_STRIDE_MASK))))

/*
 * Access methods for bus resources and address space.
 */
typedef uint32_t bus_space_tag_t;
typedef uint32_t bus_space_handle_t;

/*
 *	int bus_space_map(bus_space_tag_t t, bus_addr_t addr,
 *	    bus_size_t size, int flags, bus_space_handle_t *bshp);
 *
 * Map a region of bus space.
 */

#define BUS_SPACE_MAP_CACHEABLE		0x01
#define BUS_SPACE_MAP_LINEAR		0x02
#define BUS_SPACE_MAP_PREFETCHABLE	0x04

static __inline int bus_space_map(bus_space_tag_t, bus_addr_t,
    bus_size_t, int, bus_space_handle_t *);
static __inline void bus_space_read_region_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t *, size_t);
static __inline void bus_space_read_region_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint16_t *, size_t);
static __inline void bus_space_read_region_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint32_t *, size_t);
static __inline void bus_space_read_region_stream_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint16_t *, size_t);
static __inline void bus_space_read_region_stream_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint32_t *, size_t);
static __inline void bus_space_write_region_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const uint8_t *, size_t);
static __inline void bus_space_write_region_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const uint16_t *, size_t);
static __inline void bus_space_write_region_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const uint32_t *, size_t);
static __inline void bus_space_write_region_stream_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const uint16_t *, size_t);
static __inline void bus_space_write_region_stream_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const uint32_t *, size_t);
static __inline void bus_space_set_multi_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t, size_t);
static __inline void bus_space_set_multi_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint16_t, size_t);
static __inline void bus_space_set_multi_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint32_t, size_t);
static __inline void bus_space_set_multi_stream_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint16_t, size_t);
static __inline void bus_space_set_multi_stream_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint32_t, size_t);
static __inline void bus_space_set_region_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t, size_t);
static __inline void bus_space_set_region_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint16_t, size_t);
static __inline void bus_space_set_region_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint32_t, size_t);
static __inline void bus_space_set_region_stream_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint16_t, size_t);
static __inline void bus_space_set_region_stream_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint32_t, size_t);

static __inline int
bus_space_map(t, addr, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t addr;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	paddr_t base = t & MACPPC_BUS_ADDR_MASK;
	int stride = t & MACPPC_BUS_STRIDE_MASK;

	*bshp = (bus_space_handle_t)
		mapiodev(base + (addr << stride), size << stride);
	return 0;
}

/*
 *	int bus_space_unmap(bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size);
 *
 * Unmap a region of bus space.
 */

#define bus_space_unmap(t, bsh, size)

/*
 *	int bus_space_subregion(bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t offset, bus_size_t size,
 *	    bus_space_handle_t *nbshp);
 *
 * Get a new handle for a subregion of an already-mapped area of bus space.
 */

#define	bus_space_subregion(t, bsh, offset, size, bshp)			\
	((*(bshp) = (bus_space_handle_t)__BA(t, bsh, offset)), 0)

/*
 *	int bus_space_alloc(bus_space_tag_t t, bus_addr_t rstart,
 *	    bus_addr_t rend, bus_size_t size, bus_size_t align,
 *	    bus_size_t boundary, int flags, bus_addr_t *addrp,
 *	    bus_space_handle_t *bshp);
 *
 * Allocate a region of bus space.
 */

#if 0
#define bus_space_alloc(t, rs, re, s, a, b, f, ap, hp)	!!! unimplemented !!!
#endif

/*
 *	int bus_space_free(bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size);
 *
 * Free a region of bus space.
 */
#if 0
#define bus_space_free(t, h, s)		!!! unimplemented !!!
#endif

/*
 *	u_intN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

#define bus_space_read_1(t, h, o)	(in8(__BA(t, h, o)))
#define bus_space_read_2(t, h, o)	(in16rb(__BA(t, h, o)))
#define bus_space_read_4(t, h, o)	(in32rb(__BA(t, h, o)))
#if 0	/* Cause a link error for bus_space_read_8 */
#define bus_space_read_8(t, h, o)	!!! unimplemented !!!
#endif

#define bus_space_read_stream_1(t, h, o)	(in8(__BA(t, h, o)))
#define bus_space_read_stream_2(t, h, o)	(in16(__BA(t, h, o)))
#define bus_space_read_stream_4(t, h, o)	(in32(__BA(t, h, o)))
#if 0	/* Cause a link error for bus_space_read_stream_8 */
#define bus_space_read_8(t, h, o)	!!! unimplemented !!!
#endif

/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

#define bus_space_read_multi_1(t, h, o, a, c) do {			\
		ins8(__BA(t, h, o), (a), (c));				\
	} while (0)

#define bus_space_read_multi_2(t, h, o, a, c) do {			\
		ins16rb(__BA(t, h, o), (a), (c));			\
	} while (0)

#define bus_space_read_multi_4(t, h, o, a, c) do {			\
		ins32rb(__BA(t, h, o), (a), (c));			\
	} while (0)

#if 0	/* Cause a link error for bus_space_read_multi_8 */
#define bus_space_read_multi_8		!!! unimplemented !!!
#endif

#define bus_space_read_multi_stream_1(t, h, o, a, c) do {		\
		ins8(__BA(t, h, o), (a), (c));				\
	} while (0)

#define bus_space_read_multi_stream_2(t, h, o, a, c) do {		\
		ins16(__BA(t, h, o), (a), (c));				\
	} while (0)

#define bus_space_read_multi_stream_4(t, h, o, a, c) do {		\
		ins32(__BA(t, h, o), (a), (c));				\
	} while (0)

#if 0	/* Cause a link error for bus_space_read_multi_stream_8 */
#define bus_space_read_multi_stream_8	!!! unimplemented !!!
#endif

/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */

static __inline void
bus_space_read_region_1(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint8_t *addr;
	size_t count;
{
	volatile u_int8_t *s = __BA(tag, bsh, offset);

	while (count--)
		*addr++ = *s++;
	__asm volatile("eieio; sync");
}

static __inline void
bus_space_read_region_2(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint16_t *addr;
	size_t count;
{
	volatile uint16_t *s = __BA(tag, bsh, offset);

	while (count--)
		__asm volatile("lhbrx %0, 0, %1" :
			"=r"(*addr++) : "r"(s++));
	__asm volatile("eieio; sync");
}

static __inline void
bus_space_read_region_4(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint32_t *addr;
	size_t count;
{
	volatile uint32_t *s = __BA(tag, bsh, offset);

	while (count--)
		__asm volatile("lwbrx %0, 0, %1" :
			"=r"(*addr++) : "r"(s++));
	__asm volatile("eieio; sync");
}

#if 0	/* Cause a link error for bus_space_read_region_8 */
#define	bus_space_read_region_8		!!! unimplemented !!!
#endif

static __inline void
bus_space_read_region_stream_2(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint16_t *addr;
	size_t count;
{
	volatile uint16_t *s = __BA(tag, bsh, offset);

	while (count--)
		*addr++ = *s++;
	__asm volatile("eieio; sync");
}

static __inline void
bus_space_read_region_stream_4(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint32_t *addr;
	size_t count;
{
	volatile uint32_t *s = __BA(tag, bsh, offset);

	while (count--)
		*addr++ = *s++;
	__asm volatile("eieio; sync");
}

#if 0	/* Cause a link error */
#define	bus_space_read_region_stream_8		!!! unimplemented !!!
#endif

/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

#define bus_space_write_1(t, h, o, v)	out8(__BA(t, h, o), (v))
#define bus_space_write_2(t, h, o, v)	out16rb(__BA(t, h, o), (v))
#define bus_space_write_4(t, h, o, v)	out32rb(__BA(t, h, o), (v))

#define bus_space_write_stream_1(t, h, o, v)	out8(__BA(t, h, o), (v))
#define bus_space_write_stream_2(t, h, o, v)	out16(__BA(t, h, o), (v))
#define bus_space_write_stream_4(t, h, o, v)	out32(__BA(t, h, o), (v))

#define bus_space_mmap(tag, addr, off, prot, flags) \
    (paddr_t)((tag & MACPPC_BUS_ADDR_MASK) + addr + off)
#define bus_space_vaddr(tag, handle)	__BA(tag, handle, 0)

#if 0	/* Cause a link error for bus_space_write_8 */
#define bus_space_write_8		!!! unimplemented !!!
#endif

/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

#define bus_space_write_multi_1(t, h, o, a, c) do {			\
		outs8(__BA(t, h, o), (a), (c));				\
	} while (0)

#define bus_space_write_multi_2(t, h, o, a, c) do {			\
		outs16rb(__BA(t, h, o), (a), (c));				\
	} while (0)

#define bus_space_write_multi_4(t, h, o, a, c) do {			\
		outs32rb(__BA(t, h, o), (a), (c));				\
	} while (0)

#if 0
#define bus_space_write_multi_8		!!! unimplemented !!!
#endif

#define bus_space_write_multi_stream_1(t, h, o, a, c) do {		\
		outs8(__BA(t, h, o), (a), (c));				\
	} while (0)

#define bus_space_write_multi_stream_2(t, h, o, a, c) do {		\
		outs16(__BA(t, h, o), (a), (c));				\
	} while (0)

#define bus_space_write_multi_stream_4(t, h, o, a, c) do {		\
		outs32(__BA(t, h, o), (a), (c));				\
	} while (0)

#if 0
#define bus_space_write_multi_stream_8	!!! unimplemented !!!
#endif

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */

static __inline void
bus_space_write_region_1(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	const uint8_t *addr;
	size_t count;
{
	volatile uint8_t *d = __BA(tag, bsh, offset);

	while (count--)
		*d++ = *addr++;
	__asm volatile("eieio; sync");
}

static __inline void
bus_space_write_region_2(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	const uint16_t *addr;
	size_t count;
{
	volatile uint16_t *d = __BA(tag, bsh, offset);

	while (count--)
		__asm volatile("sthbrx %0, 0, %1" ::
			"r"(*addr++), "r"(d++));
	__asm volatile("eieio; sync");
}

static __inline void
bus_space_write_region_4(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	const uint32_t *addr;
	size_t count;
{
	volatile uint32_t *d = __BA(tag, bsh, offset);

	while (count--)
		__asm volatile("stwbrx %0, 0, %1" ::
			"r"(*addr++), "r"(d++));
	__asm volatile("eieio; sync");
}

#if 0
#define	bus_space_write_region_8 !!! bus_space_write_region_8 unimplemented !!!
#endif

static __inline void
bus_space_write_region_stream_2(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	const uint16_t *addr;
	size_t count;
{
	volatile uint16_t *d = __BA(tag, bsh, offset);

	while (count--)
		*d++ = *addr++;
	__asm volatile("eieio; sync");
}

static __inline void
bus_space_write_region_stream_4(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	const uint32_t *addr;
	size_t count;
{
	volatile uint32_t *d = __BA(tag, bsh, offset);

	while (count--)
		*d++ = *addr++;
	__asm volatile("eieio; sync");
}

#if 0
#define	bus_space_write_region_stream_8	!!! unimplemented !!!
#endif

/*
 *	void bus_space_set_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */

static __inline void
bus_space_set_multi_1(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint8_t val;
	size_t count;
{
	volatile uint8_t *d = __BA(tag, bsh, offset);

	while (count--)
		*d = val;
	__asm volatile("eieio; sync");
}

static __inline void
bus_space_set_multi_2(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint16_t val;
	size_t count;
{
	volatile uint16_t *d = __BA(tag, bsh, offset);

	while (count--)
		__asm volatile("sthbrx %0, 0, %1" ::
			"r"(val), "r"(d));
	__asm volatile("eieio; sync");
}

static __inline void
bus_space_set_multi_4(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint32_t val;
	size_t count;
{
	volatile uint32_t *d = __BA(tag, bsh, offset);

	while (count--)
		__asm volatile("stwbrx %0, 0, %1" ::
			"r"(val), "r"(d));
	__asm volatile("eieio; sync");
}

#if 0
#define	bus_space_set_multi_8 !!! bus_space_set_multi_8 unimplemented !!!
#endif

static __inline void
bus_space_set_multi_stream_2(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint16_t val;
	size_t count;
{
	volatile uint16_t *d = __BA(tag, bsh, offset);

	while (count--)
		*d = val;
	__asm volatile("eieio; sync");
}

static __inline void
bus_space_set_multi_stream_4(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint32_t val;
	size_t count;
{
	volatile uint32_t *d = __BA(tag, bsh, offset);

	while (count--)
		*d = val;
	__asm volatile("eieio; sync");
}

#if 0
#define	bus_space_set_multi_stream_8	!!! unimplemented !!!
#endif

/*
 *	void bus_space_set_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */

static __inline void
bus_space_set_region_1(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint8_t val;
	size_t count;
{
	volatile uint8_t *d = __BA(tag, bsh, offset);

	while (count--)
		*d++ = val;
	__asm volatile("eieio; sync");
}

static __inline void
bus_space_set_region_2(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint16_t val;
	size_t count;
{
	volatile uint16_t *d = __BA(tag, bsh, offset);

	while (count--)
		__asm volatile("sthbrx %0, 0, %1" ::
			"r"(val), "r"(d++));
	__asm volatile("eieio; sync");
}

static __inline void
bus_space_set_region_4(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint32_t val;
	size_t count;
{
	volatile uint32_t *d = __BA(tag, bsh, offset);

	while (count--)
		__asm volatile("stwbrx %0, 0, %1" ::
			"r"(val), "r"(d++));
	__asm volatile("eieio; sync");
}

#if 0
#define	bus_space_set_region_8 !!! bus_space_set_region_8 unimplemented !!!
#endif

static __inline void
bus_space_set_region_stream_2(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint16_t val;
	size_t count;
{
	volatile uint16_t *d = __BA(tag, bsh, offset);

	while (count--)
		*d++ = val;
	__asm volatile("eieio; sync");
}

static __inline void
bus_space_set_region_stream_4(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	uint32_t val;
	size_t count;
{
	volatile uint32_t *d = __BA(tag, bsh, offset);

	while (count--)
		*d++ = val;
	__asm volatile("eieio; sync");
}

#if 0
#define	bus_space_set_region_stream_8	!!! unimplemented !!!
#endif

/*
 *	void bus_space_copy_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */

	/* XXX IMPLEMENT bus_space_copy_N() XXX */

/*
 * Bus read/write barrier methods.
 *
 *	void bus_space_barrier(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags);
 *
 * Note: the macppc does not currently require barriers, but we must
 * provide the flags to MI code.
 */

#define bus_space_barrier(t, h, o, l, f)	\
	((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f)))
#define BUS_SPACE_BARRIER_READ	0x01		/* force read barrier */
#define BUS_SPACE_BARRIER_WRITE	0x02		/* force write barrier */

#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

#endif /* _MACPPC_BUS_H_ */
