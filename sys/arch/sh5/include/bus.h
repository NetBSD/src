/*	$NetBSD: bus.h,v 1.1 2002/07/05 13:31:56 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

#ifndef _SH5_BUS_H_
#define	_SH5_BUS_H_

#include <machine/bswap.h>

#ifndef __BUS_SPACE_COMPAT_OLDDEFS
#define	__BUS_SPACE_COMPAT_OLDDEFS
#endif

#define	__BUS_SPACE_HAS_STREAM_METHODS

#define	BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

/*
 * Bus address and size types
 *
 * XXX: Ideally, these should be sized according to NEFF
 * Actually, defining them as u_long may cause problems for LP64 ...
 */
typedef u_long bus_addr_t;
typedef u_long bus_size_t;

/*
 * Access methods for bus resources and address space.
 */
struct sh5_bus_space_tag;
typedef	const struct sh5_bus_space_tag *bus_space_tag_t;
typedef	u_long bus_space_handle_t;

struct sh5_bus_space_tag {
	void	*bs_cookie;
	int	(*bs_map)(void *, bus_addr_t, bus_size_t,
		    int, bus_space_handle_t *);
	void	(*bs_unmap)(void *, bus_space_handle_t, bus_size_t);
	int	(*bs_alloc)(void *, bus_addr_t, bus_addr_t, bus_size_t,
		    bus_size_t, bus_size_t, int, bus_addr_t *,
		    bus_space_handle_t *);
	void	(*bs_free)(void *, bus_space_handle_t, bus_size_t);
	int	(*bs_subregion)(void *, bus_space_handle_t, bus_size_t,
		    bus_size_t, bus_space_handle_t *);
	paddr_t	(*bs_mmap)(void *, bus_addr_t, off_t, int, int);
	u_int8_t (*bs_read_1)(void *, bus_space_handle_t,
		    bus_size_t);
	u_int16_t (*bs_read_2)(void *, bus_space_handle_t,
		    bus_size_t);
	u_int32_t (*bs_read_4)(void *, bus_space_handle_t,
		    bus_size_t);
	u_int64_t (*bs_read_8)(void *, bus_space_handle_t,
		    bus_size_t);
	void (*bs_write_1)(void *, bus_space_handle_t,
		    bus_size_t, u_int8_t);
	void (*bs_write_2)(void *, bus_space_handle_t,
		    bus_size_t, u_int16_t);
	void (*bs_write_4)(void *, bus_space_handle_t,
		    bus_size_t, u_int32_t);
	void (*bs_write_8)(void *, bus_space_handle_t,
		    bus_size_t, u_int64_t);
	u_int16_t (*bs_read_stream_2)(void *, bus_space_handle_t,
		    bus_size_t);
	u_int32_t (*bs_read_stream_4)(void *, bus_space_handle_t,
		    bus_size_t);
	u_int64_t (*bs_read_stream_8)(void *, bus_space_handle_t,
		    bus_size_t);
	void (*bs_write_stream_2)(void *, bus_space_handle_t,
		    bus_size_t, u_int16_t);
	void (*bs_write_stream_4)(void *, bus_space_handle_t,
		    bus_size_t, u_int32_t);
	void (*bs_write_stream_8)(void *, bus_space_handle_t,
		    bus_size_t, u_int64_t);
	int	(*bs_peek_1)(void *, bus_space_handle_t,
		    bus_size_t, u_int8_t *);
	int	(*bs_peek_2)(void *, bus_space_handle_t,
		    bus_size_t, u_int16_t *);
	int	(*bs_peek_4)(void *, bus_space_handle_t,
		    bus_size_t, u_int32_t *);
	int	(*bs_peek_8)(void *, bus_space_handle_t,
		    bus_size_t, u_int64_t *);
	int	(*bs_poke_1)(void *, bus_space_handle_t,
		    bus_size_t, u_int8_t);
	int	(*bs_poke_2)(void *, bus_space_handle_t,
		    bus_size_t, u_int16_t);
	int	(*bs_poke_4)(void *, bus_space_handle_t,
		    bus_size_t, u_int32_t);
	int	(*bs_poke_8)(void *, bus_space_handle_t,
		    bus_size_t, u_int64_t);
};


/*
 *	int bus_space_map(bus_space_tag_t tag, bus_addr_t address,
 *		bus_size_t size, int flags, bus_space_handle_t *handlep)
 */
#define	bus_space_map(t,a,s,f,hp) \
	    (*(t)->bs_map)((t)->bs_cookie, (a), (s), (f), (hp))

/*
 * Flags to bus_space_map()
 */
#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02

/*
 *	void bus_space_unmap(bus_space_tag_t tag, bus_space_handle_t handle,
 *		bus_size_t size)
 */
#define	bus_space_unmap(t,h,s) \
	    (*(t)->bs_unmap)((t)->bs_cookie, (h), (s))

/*
 *	int bus_space_alloc(bus_space_tag_t t, bus_addr_t rstart,
 *		bus_addr_t rend, bus_size_t size, bus_size_t align,
 *		bus_size_t boundary, int flags, bus_addr_t *addrp,
 *		bus_space_handle_t *bshp);
 *
 * Allocate a region of bus space.
 */
#define	bus_space_alloc(t,rs,re,s,a,b,f,ap,hp)			\
	(*(t)->bs_alloc)((t)->bs_cookie,(rs),(re),(s),(a),(b),(f),(ap),(hp))

/*
 *	int bus_space_free(bus_space_tag_t t,
 *		bus_space_handle_t bsh, bus_size_t size);
 *
 * Free a region of bus space.
 */
#define	bus_space_free(t, h, s)						\
	(*(t)->bs_free)((t)->bs_cookie, (h), (s))

/*
 *	int bus_space_subregion(bus_space_tag_t t,
 *		bus_space_handle_t bsh, bus_size_t offset, bus_size_t size,
 *		bus_space_handle_t *nbshp);
 *
 * Get a new handle for a subregion of an already-mapped area of bus space.
 */
#define	bus_space_subregion(t, h, o, s, nhp)				\
	(*(t)->bs_subregion)((t)->bs_cookie, (h), (o), (s), (nhp))

/*
 *	vaddr_t bus_space_vaddr(bus_space_tag tag, bus_space_handle handle)
 *
 * Get the KVA associated with the mapped bus space IFF is was mapped
 * with BUS_SPACE_MAP_LINEAR.
 */
#define	bus_space_vaddr(t, h)	((void *)(h))

/*
 *	paddr_t bus_space_mmap(bus_space_tag_t tag, bus_addr_t,
 *		off_t, int, int)
 */
#define	bus_space_mmap(t,a,o,p,f) \
	    (*(t)->bs_mmap)((t)->bs_cookie, (a), (o), (p), (f))

/*
 *	u_intN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */
#define bus_space_read_1(t,h,o) \
	    (*(t)->bs_read_1)((t)->bs_cookie, (h), (o))
#define bus_space_read_2(t,h,o) \
	    (*(t)->bs_read_2)((t)->bs_cookie, (h), (o))
#define bus_space_read_4(t,h,o) \
	    (*(t)->bs_read_4)((t)->bs_cookie, (h), (o))
#define bus_space_read_8(t,h,o) \
	    (*(t)->bs_read_8)((t)->bs_cookie, (h), (o))


/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */
#define bus_space_write_1(t,h,o,v) \
	    (*(t)->bs_write_1)((t)->bs_cookie, (h), (o), (v))
#define bus_space_write_2(t,h,o,v) \
	    (*(t)->bs_write_2)((t)->bs_cookie, (h), (o), (v))
#define bus_space_write_4(t,h,o,v) \
	    (*(t)->bs_write_4)((t)->bs_cookie, (h), (o), (v))
#define bus_space_write_8(t,h,o,v) \
	    (*(t)->bs_write_8)((t)->bs_cookie, (h), (o), (v))


/*
 *	u_intN_t bus_space_read_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset)
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset, with no endian conversion.
 */
#define	bus_space_read_stream_2(t,h,o) \
	    (*(t)->bs_read_stream_2)((t)->bs_cookie, (h), (o))
#define	bus_space_read_stream_4(t,h,o) \
	    (*(t)->bs_read_stream_4)((t)->bs_cookie, (h), (o))
#define	bus_space_read_stream_8(t,h,o) \
	    (*(t)->bs_read_stream_8)((t)->bs_cookie, (h), (o))


/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */
static __inline void bus_space_read_multi_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int8_t *, bus_size_t);
static __inline void bus_space_read_multi_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t);
static __inline void bus_space_read_multi_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t);
static __inline void bus_space_read_multi_8(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int64_t *, bus_size_t);

static __inline void
bus_space_read_multi_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int8_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = bus_space_read_1(tag, bsh, offset);
}

static __inline void
bus_space_read_multi_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int16_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = bus_space_read_2(tag, bsh, offset);
}

static __inline void
bus_space_read_multi_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int32_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = bus_space_read_4(tag, bsh, offset);
}

static __inline void
bus_space_read_multi_8(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int64_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = bus_space_read_8(tag, bsh, offset);
}

static __inline void bus_space_read_multi_stream_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t);
static __inline void bus_space_read_multi_stream_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t);
static __inline void bus_space_read_multi_stream_8(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int64_t *, bus_size_t);

static __inline void
bus_space_read_multi_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int16_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = bus_space_read_stream_2(tag, bsh, offset);
}

static __inline void
bus_space_read_multi_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int32_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = bus_space_read_stream_4(tag, bsh, offset);
}

static __inline void
bus_space_read_multi_stream_8(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int64_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = bus_space_read_stream_8(tag, bsh, offset);
}

/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */
static __inline void bus_space_read_region_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int8_t *, bus_size_t);
static __inline void bus_space_read_region_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t);
static __inline void bus_space_read_region_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t);
static __inline void bus_space_read_region_8(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int64_t *, bus_size_t);

static __inline void
bus_space_read_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int8_t *addr, bus_size_t count)
{
	while (count--)
		*addr++ = bus_space_read_1(tag, bsh, offset);
}

static __inline void
bus_space_read_region_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int16_t *addr, bus_size_t count)
{
	while (count--)
		*addr++ = bus_space_read_2(tag, bsh, offset);
}

static __inline void
bus_space_read_region_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int32_t *addr, bus_size_t count)
{
	while (count--)
		*addr++ = bus_space_read_4(tag, bsh, offset);
}

static __inline void
bus_space_read_region_8(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int64_t *addr, bus_size_t count)
{
	while (count--)
		*addr++ = bus_space_read_8(tag, bsh, offset);
}

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */
static __inline void bus_space_write_region_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const u_int8_t *, bus_size_t);
static __inline void bus_space_write_region_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const u_int16_t *, bus_size_t);
static __inline void bus_space_write_region_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const u_int32_t *, bus_size_t);
static __inline void bus_space_write_region_8(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const u_int64_t *, bus_size_t);

static __inline void
bus_space_write_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const u_int8_t *addr, bus_size_t count)
{
	while (count--)
		bus_space_write_1(tag, bsh, offset, *addr++);
}

static __inline void
bus_space_write_region_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const u_int16_t *addr, bus_size_t count)
{
	while (count--)
		bus_space_write_2(tag, bsh, offset, *addr++);
}

static __inline void
bus_space_write_region_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const u_int32_t *addr, bus_size_t count)
{
	while (count--)
		bus_space_write_4(tag, bsh, offset, *addr++);
}

static __inline void
bus_space_write_region_8(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const u_int64_t *addr, bus_size_t count)
{
	while (count--)
		bus_space_write_8(tag, bsh, offset, *addr++);
}


/*
 *	void bus_space_write_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t value)
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset, with no endian conversion.
 */
#define	bus_space_write_stream_2(t,h,o,v) \
	    (*(t)->bs_write_stream_2)((t)->bs_cookie, (h), (o), (v))
#define	bus_space_write_stream_4(t,h,o,v) \
	    (*(t)->bs_write_stream_4)((t)->bs_cookie, (h), (o), (v))
#define	bus_space_write_stream_8(t,h,o,v) \
	    (*(t)->bs_write_stream_8)((t)->bs_cookie, (h), (o), (v))


/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */
static __inline void bus_space_write_multi_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int8_t *, bus_size_t);
static __inline void bus_space_write_multi_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t);
static __inline void bus_space_write_multi_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t);
static __inline void bus_space_write_multi_8(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int64_t *, bus_size_t);

static __inline void
bus_space_write_multi_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int8_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_1(tag, bsh, offset, *addr++);
}

static __inline void
bus_space_write_multi_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int16_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_2(tag, bsh, offset, *addr++);
}

static __inline void
bus_space_write_multi_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int32_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_4(tag, bsh, offset, *addr++);
}

static __inline void
bus_space_write_multi_8(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int64_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_8(tag, bsh, offset, *addr++);
}

static __inline void bus_space_write_multi_stream_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t);
static __inline void bus_space_write_multi_stream_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t);
static __inline void bus_space_write_multi_stream_8(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int64_t *, bus_size_t);

static __inline void
bus_space_write_multi_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int16_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_stream_2(tag, bsh, offset, *addr++);
}

static __inline void
bus_space_write_multi_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int32_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_stream_4(tag, bsh, offset, *addr++);
}

static __inline void
bus_space_write_multi_stream_8(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int64_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_stream_8(tag, bsh, offset, *addr++);
}

/*
 *	void bus_space_set_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */
static __inline void bus_space_set_multi_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int8_t, bus_size_t);
static __inline void bus_space_set_multi_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int16_t, bus_size_t);
static __inline void bus_space_set_multi_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int32_t, bus_size_t);
static __inline void bus_space_set_multi_8(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int64_t, bus_size_t);

static __inline void
bus_space_set_multi_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int8_t val, bus_size_t count)
{

	while (count--)
		bus_space_write_1(tag, bsh, offset, val);
}

static __inline void
bus_space_set_multi_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int16_t val, bus_size_t count)
{

	while (count--)
		bus_space_write_2(tag, bsh, offset, val);
}

static __inline void
bus_space_set_multi_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int32_t val, bus_size_t count)
{

	while (count--)
		bus_space_write_4(tag, bsh, offset, val);
}

static __inline void
bus_space_set_multi_8(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int64_t val, bus_size_t count)
{

	while (count--)
		bus_space_write_8(tag, bsh, offset, val);
}

/*
 *	void bus_space_set_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */
static __inline void bus_space_set_region_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int8_t, bus_size_t);
static __inline void bus_space_set_region_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int16_t, bus_size_t);
static __inline void bus_space_set_region_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int32_t, bus_size_t);
static __inline void bus_space_set_region_8(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, u_int64_t, bus_size_t);

static __inline void
bus_space_set_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int8_t val, bus_size_t count)
{
	while (count--)
		bus_space_write_1(tag, bsh, offset, val);
}

static __inline void
bus_space_set_region_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int16_t val, bus_size_t count)
{
	while (count--)
		bus_space_write_2(tag, bsh, offset, val);
}

static __inline void
bus_space_set_region_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int32_t val, bus_size_t count)
{
	while (count--)
		bus_space_write_4(tag, bsh, offset, val);
}

static __inline void
bus_space_set_region_8(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, u_int64_t val, bus_size_t count)
{
	while (count--)
		bus_space_write_8(tag, bsh, offset, val);
}

/*
 *	void bus_space_copy_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */
static __inline void bus_space_copy_region_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t,
    bus_size_t);
static __inline void bus_space_copy_region_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t,
    bus_size_t);
static __inline void bus_space_copy_region_4(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t,
    bus_size_t);
static __inline void bus_space_copy_region_8(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t,
    bus_size_t);

static __inline void
bus_space_copy_region_1(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{

	if ((h1 + o1) >= (h2 + o2)) {	/* src after dest: copy forward */
		while (c--)
			bus_space_write_1(t, h2, o2++,
			    bus_space_read_1(t, h1, o1++));
	} else {		/* dest after src: copy backwards */
		o1 += c - 1;
		o2 += c - 1;
		while (c--)
			bus_space_write_1(t, h2, o2--,
			    bus_space_read_1(t, h1, o1--));
	}
}

static __inline void
bus_space_copy_region_2(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{

	if ((h1 + o1) >= (h2 + o2)) {	/* src after dest: copy forward */
		while (c--) {
			bus_space_write_2(t, h2, o2,
			    bus_space_read_2(t, h1, o1));
			o1 += 2;
			o2 += 2;
		}
	} else {		/* dest after src: copy backwards */
		o1 += c - 1;
		o2 += c - 1;
		while (c--) {
			bus_space_write_2(t, h2, o2,
			    bus_space_read_2(t, h1, o1));
			o1 -= 2;
			o2 -= 2;
		}
	}
}

static __inline void
bus_space_copy_region_4(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{

	if ((h1 + o1) >= (h2 + o2)) {	/* src after dest: copy forward */
		while (c--) {
			bus_space_write_4(t, h2, o2,
			    bus_space_read_4(t, h1, o1));
			o1 += 4;
			o2 += 4;
		}
	} else {		/* dest after src: copy backwards */
		o1 += c - 1;
		o2 += c - 1;
		while (c--) {
			bus_space_write_4(t, h2, o2,
			    bus_space_read_4(t, h1, o1));
			o1 -= 4;
			o2 -= 4;
		}
	}
}

static __inline void
bus_space_copy_region_8(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{

	if ((h1 + o1) >= (h2 + o2)) {	/* src after dest: copy forward */
		while (c--) {
			bus_space_write_8(t, h2, o2,
			    bus_space_read_8(t, h1, o1));
			o1 += 8;
			o2 += 8;
		}
	} else {		/* dest after src: copy backwards */
		o1 += c - 1;
		o2 += c - 1;
		while (c--) {
			bus_space_write_8(t, h2, o2,
			    bus_space_read_8(t, h1, o1));
			o1 -= 8;
			o2 -= 8;
		}
	}
}

/*
 * Bus read/write barrier methods.
 *
 *	void bus_space_barrier(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags);
 */
static __inline void bus_space_barrier(bus_space_tag_t, bus_space_handle_t,
		bus_size_t, bus_size_t, int);
/*ARGSUSED*/
static __inline void
bus_space_barrier(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    bus_size_t l, int f)
{

	__asm __volatile("synco");
}
#define	BUS_SPACE_BARRIER_READ	0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE	0x02		/* force write barrier */

/*
 *	int bus_space_peek_N(bus_space_tag_t tag, bus_space_handle_t handle,
 *		bus_size_t offset, u_intN_t *datap)
 */
#define	bus_space_peek_1(t,h,o,vp) \
	    (*(t)->bs_peek_1)((t)->bs_cookie, (h), (o), (vp))
#define	bus_space_peek_2(t,h,o,vp) \
	    (*(t)->bs_peek_2)((t)->bs_cookie, (h), (o), (vp))
#define	bus_space_peek_4(t,h,o,vp) \
	    (*(t)->bs_peek_4)((t)->bs_cookie, (h), (o), (vp))
#define	bus_space_peek_8(t,h,o,vp) \
	    (*(t)->bs_peek_8)((t)->bs_cookie, (h), (o), (vp))

/*
 *	int bus_space_poke_N(bus_space_tag_t tag, bus_space_handle_t handle,
 *		bus_size_t offset, u_intN_t data)
 */
#define	bus_space_poke_1(t,h,o,v) \
	    (*(t)->bs_poke_1)((t)->bs_cookie, (h), (o), (v))
#define	bus_space_poke_2(t,h,o,v) \
	    (*(t)->bs_poke_2)((t)->bs_cookie, (h), (o), (v))
#define	bus_space_poke_4(t,h,o,v) \
	    (*(t)->bs_poke_4)((t)->bs_cookie, (h), (o), (v))
#define	bus_space_poke_8(t,h,o,v) \
	    (*(t)->bs_poke_8)((t)->bs_cookie, (h), (o), (v))


/*
 * Bus DMA Methods
 */

/*
 * Flags used in various bus DMA methods.
 */
#define	BUS_DMA_WAITOK		0x000	/* safe to sleep (pseudo-flag) */
#define	BUS_DMA_NOWAIT		0x001	/* not safe to sleep */
#define	BUS_DMA_ALLOCNOW	0x002	/* perform resource allocation now */
#define	BUS_DMA_COHERENT	0x004	/* hint: map memory DMA coherent */
#define	BUS_DMA_STREAMING	0x008	/* hint: sequential, unidirectional */
#define	BUS_DMA_BUS1		0x010	/* placeholders for bus functions... */
#define	BUS_DMA_BUS2		0x020
#define	BUS_DMA_BUS3		0x040
#define	BUS_DMA_BUS4		0x080
#define	BUS_DMA_READ		0x100	/* mapping is device -> memory only */
#define	BUS_DMA_WRITE		0x200	/* mapping is memory -> device only */

/* Forwards needed by prototypes below. */
struct mbuf;
struct uio;

/*
 * Operations performed by bus_dmamap_sync().
 */
#define	BUS_DMASYNC_PREREAD	0x01	/* pre-read synchronization */
#define	BUS_DMASYNC_POSTREAD	0x02	/* post-read synchronization */
#define	BUS_DMASYNC_PREWRITE	0x04	/* pre-write synchronization */
#define	BUS_DMASYNC_POSTWRITE	0x08	/* post-write synchronization */

struct sh5_bus_dma_tag;
struct sh5_bus_dmamap;
typedef const struct sh5_bus_dma_tag *bus_dma_tag_t;
typedef struct sh5_bus_dmamap *bus_dmamap_t;

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct sh5_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */

	/* Internal Use only */
	bus_addr_t	_ds_cpuaddr;	/* CPU address */
	int		_ds_flags;
};
typedef struct sh5_bus_dma_segment	bus_dma_segment_t;

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */
struct sh5_bus_dma_tag {
	void	*bd_cookie;		/* cookie used in the guts */

	/*
	 * DMA mapping methods.
	 */
	int	(*bd_dmamap_create)(void *, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *);
	void	(*bd_dmamap_destroy)(void *, bus_dmamap_t);
	int	(*bd_dmamap_load)(void *, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int);
	int	(*bd_dmamap_load_mbuf)(void *, bus_dmamap_t,
		    struct mbuf *, int);
	int	(*bd_dmamap_load_uio)(void *, bus_dmamap_t,
		    struct uio *, int);
	int	(*bd_dmamap_load_raw)(void *, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int);
	void	(*bd_dmamap_unload)(void *, bus_dmamap_t);
	void	(*bd_dmamap_sync)(void *, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int);

	/*
	 * DMA memory utility functions.
	 */
	int	(*bd_dmamem_alloc)(void *, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int);
	void	(*bd_dmamem_free)(void *, bus_dma_segment_t *, int);
	int	(*bd_dmamem_map)(void *, bus_dma_segment_t *,
		    int, size_t, caddr_t *, int);
	void	(*bd_dmamem_unmap)(void *, caddr_t, size_t);
	paddr_t	(*bd_dmamem_mmap)(void *, bus_dma_segment_t *,
		    int, off_t, int, int);
};


#define	bus_dmamap_create(t, s, n, m, b, f, p)			\
	(*(t)->bd_dmamap_create)((t)->bd_cookie, (s), (n), (m), (b), (f), (p))
#define	bus_dmamap_destroy(t, p)				\
	(*(t)->bd_dmamap_destroy)((t)->bd_cookie, (p))
#define	bus_dmamap_load(t, m, b, s, p, f)			\
	(*(t)->bd_dmamap_load)((t)->bd_cookie, (m), (b), (s), (p), (f))
#define	bus_dmamap_load_mbuf(t, m, b, f)			\
	(*(t)->bd_dmamap_load_mbuf)((t)->bd_cookie, (m), (b), (f))
#define	bus_dmamap_load_uio(t, m, u, f)				\
	(*(t)->bd_dmamap_load_uio)((t)->bd_cookie, (m), (u), (f))
#define	bus_dmamap_load_raw(t, m, sg, n, s, f)			\
	(*(t)->bd_dmamap_load_raw)((t)->bd_cookie, (m), (sg), (n), (s), (f))
#define	bus_dmamap_unload(t, p)					\
	(*(t)->bd_dmamap_unload)((t)->bd_cookie, (p))
#define	bus_dmamap_sync(t, p, o, l, ops)			\
	(*(t)->bd_dmamap_sync)((t)->bd_cookie, (p), (o), (l), (ops))
#define	bus_dmamem_alloc(t, s, a, b, sg, n, r, f)		\
	(*(t)->bd_dmamem_alloc)((t)->bd_cookie, (s), (a), (b),(sg),(n),(r),(f))
#define	bus_dmamem_free(t, sg, n)				\
	(*(t)->bd_dmamem_free)((t)->bd_cookie, (sg), (n))
#define	bus_dmamem_map(t, sg, n, s, k, f)			\
	(*(t)->bd_dmamem_map)((t)->bd_cookie, (sg), (n), (s), (k), (f))
#define	bus_dmamem_unmap(t, k, s)				\
	(*(t)->bd_dmamem_unmap)((t)->bd_cookie, (k), (s))
#define	bus_dmamem_mmap(t, sg, n, o, p, f)			\
	(*(t)->bd_dmamem_mmap)((t)->bd_cookie, (sg), (n), (o), (p), (f))

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct sh5_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use by machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxsegsz;	/* largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	int		_dm_flags;	/* misc. flags */
	void		*_dm_cookie;	/* Bus-specific cookie */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

/* Internal use only */
extern const struct sh5_bus_space_tag _sh5_bus_space_tag;
extern const struct sh5_bus_dma_tag _sh5_bus_dma_tag;

#endif /* _SH5_BUS_H_ */
