/*	$NetBSD: bus.h,v 1.3.4.1 2000/06/30 16:27:37 simonb Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
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

#ifndef _SH3_BUS_H_
#define _SH3_BUS_H_

#include <machine/bswap.h>

#ifndef __BUS_SPACE_COMPAT_OLDDEFS
#define	__BUS_SPACE_COMPAT_OLDDEFS
#endif

#define __BUS_SPACE_HAS_STREAM_METHODS

#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

/*
 * Values for the sh3 bus space tag, not to be used directly by MI code.
 */
#define	SH3_BUS_SPACE_IO	0	/* space is i/o space */
#define SH3_BUS_SPACE_MEM	1	/* space is mem space */
#ifdef SH4_PCMCIA
#define SH3_BUS_SPACE_PCMCIA_IO 2	/* PCMCIA IO space */
#define SH3_BUS_SPACE_PCMCIA_MEM 3	/* PCMCIA Mem space */
#define SH3_BUS_SPACE_PCMCIA_ATT 4	/* PCMCIA Attr space */
#endif

/*
 * Bus address and size types
 */
typedef u_long bus_addr_t;
typedef u_long bus_size_t;

/*
 * Access methods for bus resources and address space.
 */
typedef	int bus_space_tag_t;
typedef	u_long bus_space_handle_t;

/*
 *	int bus_space_map  __P((bus_space_tag_t t, bus_addr_t addr,
 *	    bus_size_t size, int flags, bus_space_handle_t *bshp));
 *
 * Map a region of bus space.
 */
#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02

int bus_space_map __P((bus_space_tag_t, bus_addr_t, bus_size_t size,
	int, bus_space_handle_t *));

#ifdef SH4_PCMCIA
int shpcmcia_memio_map __P((bus_space_tag_t, bus_addr_t, bus_size_t size,
	int, bus_space_handle_t *));

int shpcmcia_mem_add_mapping __P((bus_addr_t, bus_size_t, int, 
				  bus_space_handle_t *));
void shpcmcia_memio_unmap __P((bus_space_tag_t, bus_space_handle_t,
			       bus_size_t));
void shpcmcia_memio_free __P((bus_space_tag_t, bus_space_handle_t,
			      bus_size_t));
int shpcmcia_memio_subregion __P((bus_space_tag_t, bus_space_handle_t,
				  bus_size_t, bus_size_t,
				  bus_space_handle_t *));
#endif

/*
 *	u_intN_t bus_space_read_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset));
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */
static __inline u_int8_t bus_space_read_1
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t));
static __inline u_int16_t bus_space_read_2
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t));
static __inline u_int32_t bus_space_read_4
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t));

u_int8_t
bus_space_read_1(tag, bsh, offset)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	return *(volatile u_int8_t *)(bsh + offset);
}

u_int16_t
bus_space_read_2(tag, bsh, offset)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	return bswap16(*(volatile u_int16_t *)(bsh + offset));
}

u_int32_t
bus_space_read_4(tag, bsh, offset)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	return bswap32(*(volatile u_int32_t *)(bsh + offset));
}

static __inline u_int16_t bus_space_read_stream_2
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t));

static __inline u_int32_t bus_space_read_stream_4
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t));

u_int16_t
bus_space_read_stream_2(tag, bsh, offset)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	return *(volatile u_int16_t *)(bsh + offset);
}

u_int32_t
bus_space_read_stream_4(tag, bsh, offset)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	return *(volatile u_int32_t *)(bsh + offset);
}

/*
 *	void bus_space_read_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count));
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */
static __inline void bus_space_read_multi_1 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int8_t *, bus_size_t));
static __inline void bus_space_read_multi_2 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t));
static __inline void bus_space_read_multi_4 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t));

void
bus_space_read_multi_1(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t *addr;
	bus_size_t count;
{
	while (count--)
		*addr++ = bus_space_read_1(tag, bsh, offset);
}

void
bus_space_read_multi_2(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t *addr;
	bus_size_t count;
{
	while (count--)
		*addr++ = bus_space_read_2(tag, bsh, offset);
}

void
bus_space_read_multi_4(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t *addr;
	bus_size_t count;
{
	while (count--)
		*addr++ = bus_space_read_4(tag, bsh, offset);
}

static __inline void bus_space_read_multi_stream_2 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t));
static __inline void bus_space_read_multi_stream_4 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t));

void
bus_space_read_multi_stream_2(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t *addr;
	bus_size_t count;
{
	while (count--)
		*addr++ = *(volatile u_int16_t *)(bsh + offset);
}

void
bus_space_read_multi_stream_4(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t *addr;
	bus_size_t count;
{
	while (count--)
		*addr++ = *(volatile u_int32_t *)(bsh + offset);
}

/*
 *	int bus_space_alloc __P((bus_space_tag_t t, bus_addr_t rstart,
 *	    bus_addr_t rend, bus_size_t size, bus_size_t align,
 *	    bus_size_t boundary, int flags, bus_addr_t *addrp,
 *	    bus_space_handle_t *bshp));
 *
 * Allocate a region of bus space.
 */
int sh_memio_alloc __P((bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t,
			bus_size_t, bus_size_t, int,
			bus_addr_t *, bus_space_handle_t *));

#define bus_space_alloc(t, rs, re, s, a, b, f, ap, hp)			\
	sh_memio_alloc((t), (rs), (re), (s), (a), (b), (f), (ap), (hp))

/*
 *	int bus_space_free __P((bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size));
 *
 * Free a region of bus space.
 */
void sh_memio_free __P((bus_space_tag_t, bus_space_handle_t, bus_size_t));

#define bus_space_free(t, h, s)						\
	sh_memio_free((t), (h), (s))

/*
 *	int bus_space_unmap __P((bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size));
 *
 * Unmap a region of bus space.
 */
void sh_memio_unmap __P((bus_space_tag_t, bus_space_handle_t, bus_size_t));

#define bus_space_unmap(t, h, s)					\
	sh_memio_unmap((t), (h), (s))

/*
 *	int bus_space_subregion __P((bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t offset, bus_size_t size,
 *	    bus_space_handle_t *nbshp));
 *
 * Get a new handle for a subregion of an already-mapped area of bus space.
 */
int sh_memio_subregion __P((bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, bus_size_t, bus_space_handle_t *));

#define bus_space_subregion(t, h, o, s, nhp)				\
	sh_memio_subregion((t), (h), (o), (s), (nhp))

/*
 *	void bus_space_read_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count));
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */
static __inline void bus_space_read_region_1 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int8_t *, bus_size_t));
static __inline void bus_space_read_region_2 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t));
static __inline void bus_space_read_region_4 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t));

void
bus_space_read_region_1(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t *addr;
	bus_size_t count;
{
	u_int8_t *p = (u_int8_t *)(bsh + offset);

	while (count--)
		*addr++ = *p++;
}

void
bus_space_read_region_2(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t *addr;
	bus_size_t count;
{
	u_int16_t *p = (u_int16_t *)(bsh + offset);

	while (count--)
		*addr++ = bswap16(*p++);
}

void
bus_space_read_region_4(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t *addr;
	bus_size_t count;
{
	u_int32_t *p = (u_int32_t *)(bsh + offset);

	while (count--)
		*addr++ = bswap32(*p++);
}

/*
 *	void bus_space_write_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */
static __inline void bus_space_write_region_1 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, const u_int8_t *, bus_size_t));
static __inline void bus_space_write_region_2 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, const u_int16_t *, bus_size_t));
static __inline void bus_space_write_region_4 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, const u_int32_t *, bus_size_t));

void
bus_space_write_region_1(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	const u_int8_t *addr;
	bus_size_t count;
{
	u_int8_t *p = (u_int8_t *)(bsh + offset);

	while (count--)
		*p++ = *addr++;
}

void
bus_space_write_region_2(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	const u_int16_t *addr;
	bus_size_t count;
{
	u_int16_t *p = (u_int16_t *)(bsh + offset);

	while (count--)
		*p++ = bswap16(*addr++);
}

void
bus_space_write_region_4(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	const u_int32_t *addr;
	bus_size_t count;
{
	u_int32_t *p = (u_int32_t *)(bsh + offset);

	while (count--)
		*p++ = bswap32(*addr++);
}

/*
 *	void bus_space_write_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value));
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */
static __inline void bus_space_write_1 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int8_t));
static __inline void bus_space_write_2 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int16_t));
static __inline void bus_space_write_4 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int32_t));

void
bus_space_write_1(tag, bsh, offset, value)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t value;
{
	*(volatile u_int8_t *)(bsh + offset) = value;
}

void
bus_space_write_2(tag, bsh, offset, value)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t value;
{
	*(volatile u_int16_t *)(bsh + offset) = bswap16(value);
}

void
bus_space_write_4(tag, bsh, offset, value)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t value;
{
	*(volatile u_int32_t *)(bsh + offset) = bswap32(value);
}

static __inline void bus_space_write_stream_2 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int16_t));
static __inline void bus_space_write_stream_4 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int32_t));

void
bus_space_write_stream_2(tag, bsh, offset, value)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t value;
{
	*(volatile u_int16_t *)(bsh + offset) = value;
}

void
bus_space_write_stream_4(tag, bsh, offset, value)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t value;
{
	*(volatile u_int32_t *)(bsh + offset) = value;
}

/*
 *	void bus_space_write_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */
static __inline void bus_space_write_multi_1 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int8_t *, bus_size_t));
static __inline void bus_space_write_multi_2 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t));
static __inline void bus_space_write_multi_4 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t));

void
bus_space_write_multi_1(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t *addr;
	bus_size_t count;
{
	while (count--)
		bus_space_write_1(tag, bsh, offset, *addr++);
}

void
bus_space_write_multi_2(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t *addr;
	bus_size_t count;
{
	while (count--)
		bus_space_write_2(tag, bsh, offset, *addr++);
}

void
bus_space_write_multi_4(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t *addr;
	bus_size_t count;
{
	while (count--)
		bus_space_write_4(tag, bsh, offset, *addr++);
}

static __inline void bus_space_write_multi_stream_2 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t));
static __inline void bus_space_write_multi_stream_4 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t));

void
bus_space_write_multi_stream_2(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t *addr;
	bus_size_t count;
{
	while (count--)
		bus_space_write_stream_2(tag, bsh, offset, *addr++);
}

void
bus_space_write_multi_stream_4(tag, bsh, offset, addr, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t *addr;
	bus_size_t count;
{
	while (count--)
		bus_space_write_stream_4(tag, bsh, offset, *addr++);
}

/*
 *	void bus_space_set_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count));
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */
static __inline void bus_space_set_multi_1 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int8_t, bus_size_t));
static __inline void bus_space_set_multi_2 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int16_t, bus_size_t));
static __inline void bus_space_set_multi_4 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int32_t, bus_size_t));

void
bus_space_set_multi_1(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t val;
	bus_size_t count;
{
	while (count--)
		bus_space_write_1(tag, bsh, offset, val);
}

void
bus_space_set_multi_2(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t val;
	bus_size_t count;
{
	while (count--)
		bus_space_write_2(tag, bsh, offset, val);
}

void
bus_space_set_multi_4(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t val;
	bus_size_t count;
{
	while (count--)
		bus_space_write_4(tag, bsh, offset, val);
}

/*
 *	void bus_space_set_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */
static __inline void bus_space_set_region_1 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int8_t, bus_size_t));
static __inline void bus_space_set_region_2 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int16_t, bus_size_t));
static __inline void bus_space_set_region_4 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, u_int32_t, bus_size_t));

void
bus_space_set_region_1(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t val;
	bus_size_t count;
{
	volatile u_int8_t *addr = (void *)(bsh + offset);

	while (count--)
		*addr++ = val;
}

void
bus_space_set_region_2(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t val;
	bus_size_t count;
{
	volatile u_int16_t *addr = (void *)(bsh + offset);

	val = bswap16(val);
	while (count--)
		*addr++ = val;
}

void
bus_space_set_region_4(tag, bsh, offset, val, count)
	bus_space_tag_t tag;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t val;
	bus_size_t count;
{
	volatile u_int32_t *addr = (void *)(bsh + offset);

	val = bswap32(val);
	while (count--)
		*addr++ = val;
}

/*
 *	void bus_space_copy_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    size_t count));
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */
static __inline void bus_space_copy_region_1 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t,
	bus_size_t));
static __inline void bus_space_copy_region_2 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t,
	bus_size_t));
static __inline void bus_space_copy_region_4 __P((bus_space_tag_t,
	bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t,
	bus_size_t));

void
bus_space_copy_region_1(t, h1, o1, h2, o2, c)
	bus_space_tag_t t;
	bus_space_handle_t h1;
	bus_size_t o1;
	bus_space_handle_t h2;
	bus_size_t o2;
	bus_size_t c;
{
	volatile u_int8_t *addr1 = (void *)(h1 + o1);
	volatile u_int8_t *addr2 = (void *)(h2 + o2);

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (c--)
			*addr2++ = *addr1++;
	} else {		/* dest after src: copy backwards */
		addr1 += c - 1;
		addr2 += c - 1;
		while (c--)
			*addr2-- = *addr1--;
	}
}

void
bus_space_copy_region_2(t, h1, o1, h2, o2, c)
	bus_space_tag_t t;
	bus_space_handle_t h1;
	bus_size_t o1;
	bus_space_handle_t h2;
	bus_size_t o2;
	bus_size_t c;
{
	volatile u_int16_t *addr1 = (void *)(h1 + o1);
	volatile u_int16_t *addr2 = (void *)(h2 + o2);

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (c--)
			*addr2++ = *addr1++;
	} else {		/* dest after src: copy backwards */
		addr1 += c - 1;
		addr2 += c - 1;
		while (c--)
			*addr2-- = *addr1--;
	}
}

void
bus_space_copy_region_4(t, h1, o1, h2, o2, c)
	bus_space_tag_t t;
	bus_space_handle_t h1;
	bus_size_t o1;
	bus_space_handle_t h2;
	bus_size_t o2;
	bus_size_t c;
{
	volatile u_int32_t *addr1 = (void *)(h1 + o1);
	volatile u_int32_t *addr2 = (void *)(h2 + o2);

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (c--)
			*addr2++ = *addr1++;
	} else {		/* dest after src: copy backwards */
		addr1 += c - 1;
		addr2 += c - 1;
		while (c--)
			*addr2-- = *addr1--;
	}
}

#ifdef __BUS_SPACE_COMPAT_OLDDEFS
/* compatibility definitions; deprecated */
#define	bus_space_copy_1(t, h1, o1, h2, o2, c)				\
	bus_space_copy_region_1((t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_2(t, h1, o1, h2, o2, c)				\
	bus_space_copy_region_2((t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_4(t, h1, o1, h2, o2, c)				\
	bus_space_copy_region_4((t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_8(t, h1, o1, h2, o2, c)				\
	bus_space_copy_region_8((t), (h1), (o1), (h2), (o2), (c))
#endif

/*
 * Bus read/write barrier methods.
 *
 *	void bus_space_barrier __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags));
 *
 * Note: the sh3 does not currently require barriers, but we must
 * provide the flags to MI code.
 */
#define	bus_space_barrier(t, h, o, l, f)	\
	((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f)))
#define	BUS_SPACE_BARRIER_READ	0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE	0x02		/* force write barrier */

#ifdef __BUS_SPACE_COMPAT_OLDDEFS
/* compatibility definitions; deprecated */
#define	BUS_BARRIER_READ	BUS_SPACE_BARRIER_READ
#define	BUS_BARRIER_WRITE	BUS_SPACE_BARRIER_WRITE
#endif

/*
 * Flags used in various bus DMA methods.
 */
#define	BUS_DMA_WAITOK		0x00	/* safe to sleep (pseudo-flag) */
#define	BUS_DMA_NOWAIT		0x01	/* not safe to sleep */
#define	BUS_DMA_ALLOCNOW	0x02	/* perform resource allocation now */
#define	BUS_DMAMEM_NOSYNC	0x04	/* map memory to not require sync */
#define	BUS_DMA_BUS1		0x10	/* placeholders for bus functions... */
#define	BUS_DMA_BUS2		0x20
#define	BUS_DMA_BUS3		0x40
#define	BUS_DMA_BUS4		0x80

/* Forwards needed by prototypes below. */
struct mbuf;
struct uio;

#if 0
/*
 *	bus_dmasync_op_t
 *
 *	Operations performed by bus_dmamap_sync().
 */
typedef enum {
	BUS_DMASYNC_PREREAD,
	BUS_DMASYNC_POSTREAD,
	BUS_DMASYNC_PREWRITE,
	BUS_DMASYNC_POSTWRITE,
} bus_dmasync_op_t;

typedef struct sh3_bus_dma_tag		*bus_dma_tag_t;
typedef struct sh3_bus_dmamap		*bus_dmamap_t;

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct sh3_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */
};
typedef struct sh3_bus_dma_segment	bus_dma_segment_t;

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */

struct sh3_bus_dma_tag {
	void	*_cookie;		/* cookie used in the guts */

	/*
	 * DMA mapping methods.
	 */
	int	(*_dmamap_create) __P((bus_dma_tag_t, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *));
	void	(*_dmamap_destroy) __P((bus_dma_tag_t, bus_dmamap_t));
	int	(*_dmamap_load) __P((bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int));
	int	(*_dmamap_load_mbuf) __P((bus_dma_tag_t, bus_dmamap_t,
		    struct mbuf *, int));
	int	(*_dmamap_load_uio) __P((bus_dma_tag_t, bus_dmamap_t,
		    struct uio *, int));
	int	(*_dmamap_load_raw) __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int));
	void	(*_dmamap_unload) __P((bus_dma_tag_t, bus_dmamap_t));
	void	(*_dmamap_sync) __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_dmasync_op_t));

	/*
	 * DMA memory utility functions.
	 */
	int	(*_dmamem_alloc) __P((bus_dma_tag_t, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int));
	void	(*_dmamem_free) __P((bus_dma_tag_t,
		    bus_dma_segment_t *, int));
	int	(*_dmamem_map) __P((bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, caddr_t *, int));
	void	(*_dmamem_unmap) __P((bus_dma_tag_t, caddr_t, size_t));
	paddr_t	(*_dmamem_mmap) __P((bus_dma_tag_t, bus_dma_segment_t *,
		    int, off_t, int, int));
};

#define	bus_dmamap_create(t, s, n, m, b, f, p)			\
	(*(t)->_dmamap_create)((t), (s), (n), (m), (b), (f), (p))
#define	bus_dmamap_destroy(t, p)				\
	(*(t)->_dmamap_destroy)((t), (p))
#define	bus_dmamap_load(t, m, b, s, p, f)			\
	(*(t)->_dmamap_load)((t), (m), (b), (s), (p), (f))
#define	bus_dmamap_load_mbuf(t, m, b, f)			\
	(*(t)->_dmamap_load_mbuf)((t), (m), (b), (f))
#define	bus_dmamap_load_uio(t, m, u, f)				\
	(*(t)->_dmamap_load_uio)((t), (m), (u), (f))
#define	bus_dmamap_load_raw(t, m, sg, n, s, f)			\
	(*(t)->_dmamap_load_raw)((t), (m), (sg), (n), (s), (f))
#define	bus_dmamap_unload(t, p)					\
	(*(t)->_dmamap_unload)((t), (p))
#define	bus_dmamap_sync(t, p, o)				\
	(void)((t)->_dmamap_sync ?				\
	    (*(t)->_dmamap_sync)((t), (p), (o)) : (void)0)

#define	bus_dmamem_alloc(t, s, a, b, sg, n, r, f)		\
	(*(t)->_dmamem_alloc)((t), (s), (a), (b), (sg), (n), (r), (f))
#define	bus_dmamem_free(t, sg, n)				\
	(*(t)->_dmamem_free)((t), (sg), (n))
#define	bus_dmamem_map(t, sg, n, s, k, f)			\
	(*(t)->_dmamem_map)((t), (sg), (n), (s), (k), (f))
#define	bus_dmamem_unmap(t, k, s)				\
	(*(t)->_dmamem_unmap)((t), (k), (s))
#define	bus_dmamem_mmap(t, sg, n, o, p, f)			\
	(*(t)->_dmamem_mmap)((t), (sg), (n), (o), (p), (f))

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct sh3_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use my machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxsegsz;	/* largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	int		_dm_flags;	/* misc. flags */

	void		*_dm_cookie;	/* cookie for bus-specific functions */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

#ifdef _SH3_BUS_DMA_PRIVATE
int	_bus_dmamap_create __P((bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *));
void	_bus_dmamap_destroy __P((bus_dma_tag_t, bus_dmamap_t));
int	_bus_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int));
int	_bus_dmamap_load_mbuf __P((bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int));
int	_bus_dmamap_load_uio __P((bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int));
int	_bus_dmamap_load_raw __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int));
void	_bus_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
void	_bus_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_dmasync_op_t));

int	_bus_dmamem_alloc __P((bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags));
void	_bus_dmamem_free __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs));
int	_bus_dmamem_map __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, size_t size, caddr_t *kvap, int flags));
void	_bus_dmamem_unmap __P((bus_dma_tag_t tag, caddr_t kva,
	    size_t size));
paddr_t	_bus_dmamem_mmap __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, off_t off, int prot, int flags));

int	_bus_dmamem_alloc_range __P((bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    vm_offset_t low, vm_offset_t high));
#endif /* _SH3_BUS_DMA_PRIVATE */

#endif /* 0 */

#endif /* _SH3_BUS_H_ */
