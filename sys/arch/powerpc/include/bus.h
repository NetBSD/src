/*	$NetBSD: bus.h,v 1.12 2003/07/25 10:12:44 scw Exp $	*/
/*	$OpenBSD: bus.h,v 1.1 1997/10/13 10:53:42 pefo Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1996 Jason R. Thorpe.  All rights reserved.
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

/*
 * Copyright (c) 1997 Per Fogelstrom.  All rights reserved.
 * Copyright (c) 1996 Niklas Hallqvist.  All rights reserved.
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

#ifndef _POWERPC_BUS_H_
#define _POWERPC_BUS_H_

/*
 * Bus access types.
 */
typedef u_int32_t bus_addr_t;
typedef u_int32_t bus_size_t;
typedef	u_int32_t bus_space_handle_t;
typedef	const struct powerpc_bus_space *bus_space_tag_t;

struct extent;

struct powerpc_bus_space_scalar {
	u_int8_t (*pbss_read_1)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t);
	u_int16_t (*pbss_read_2)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t);
	u_int32_t (*pbss_read_4)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t);
	u_int64_t (*pbss_read_8)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t);

	void (*pbss_write_1)(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    u_int8_t);
	void (*pbss_write_2)(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    u_int16_t);
	void (*pbss_write_4)(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    u_int32_t);
	void (*pbss_write_8)(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    u_int64_t);
};

struct powerpc_bus_space_group {
	void (*pbsg_read_1)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, u_int8_t *, size_t);
	void (*pbsg_read_2)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, u_int16_t *, size_t);
	void (*pbsg_read_4)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, u_int32_t *, size_t);
	void (*pbsg_read_8)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, u_int64_t *, size_t);

	void (*pbsg_write_1)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, const u_int8_t *, size_t);
	void (*pbsg_write_2)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, const u_int16_t *, size_t);
	void (*pbsg_write_4)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, const u_int32_t *, size_t);
	void (*pbsg_write_8)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, const u_int64_t *, size_t);
};

struct powerpc_bus_space_set {
	void (*pbss_set_1)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, u_int8_t, size_t);
	void (*pbss_set_2)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, u_int16_t, size_t);
	void (*pbss_set_4)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, u_int32_t, size_t);
	void (*pbss_set_8)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, u_int64_t, size_t);
};

struct powerpc_bus_space_copy {
	void (*pbsc_copy_1)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, bus_space_handle_t, bus_size_t, size_t);
	void (*pbsc_copy_2)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, bus_space_handle_t, bus_size_t, size_t);
	void (*pbsc_copy_4)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, bus_space_handle_t, bus_size_t, size_t);
	void (*pbsc_copy_8)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, bus_space_handle_t, bus_size_t, size_t);
};

struct powerpc_bus_space {
	int pbs_flags;
#define	_BUS_SPACE_BIG_ENDIAN		0x00000100
#define	_BUS_SPACE_LITTLE_ENDIAN	0x00000000
#define	_BUS_SPACE_IO_TYPE		0x00000200
#define	_BUS_SPACE_MEM_TYPE		0x00000000
#define _BUS_SPACE_STRIDE_MASK		0x0000001f
	bus_addr_t pbs_offset;		/* offset to real start */
	bus_addr_t pbs_base;		/* extent base */
	bus_addr_t pbs_limit;		/* extent limit */
	struct extent *pbs_extent;

	paddr_t (*pbs_mmap)(bus_space_tag_t, bus_addr_t, off_t, int, int);
	int (*pbs_map)(bus_space_tag_t, bus_addr_t, bus_size_t, int,
	    bus_space_handle_t *);
	void (*pbs_unmap)(bus_space_tag_t, bus_space_handle_t, bus_size_t);
	int (*pbs_alloc)(bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t,
	    bus_size_t align, bus_size_t, int, bus_addr_t *,
	    bus_space_handle_t *);
	void (*pbs_free)(bus_space_tag_t, bus_space_handle_t, bus_size_t);
	int (*pbs_subregion)(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    bus_size_t, bus_space_handle_t *);

	struct powerpc_bus_space_scalar pbs_scalar;
	struct powerpc_bus_space_scalar pbs_scalar_stream;
	const struct powerpc_bus_space_group *pbs_multi;
	const struct powerpc_bus_space_group *pbs_multi_stream;
	const struct powerpc_bus_space_group *pbs_region;
	const struct powerpc_bus_space_group *pbs_region_stream;
	const struct powerpc_bus_space_set *pbs_set;
	const struct powerpc_bus_space_set *pbs_set_stream;
	const struct powerpc_bus_space_copy *pbs_copy;
};

#define _BUS_SPACE_STRIDE(t, o) \
	((o) << ((t)->pbs_flags & _BUS_SPACE_STRIDE_MASK)) 
#define _BUS_SPACE_UNSTRIDE(t, o) \
	((o) >> ((t)->pbs_flags & _BUS_SPACE_STRIDE_MASK)) 

#define BUS_SPACE_MAP_CACHEABLE         0x01
#define BUS_SPACE_MAP_LINEAR            0x02
#define	BUS_SPACE_MAP_PREFETCHABLE	0x04

#ifdef __STDC__
#define CAT(a,b)	a##b
#define CAT3(a,b,c)	a##b##c
#else
#define CAT(a,b)	a/**/b
#define CAT3(a,b,c)	a/**/b/**/c
#endif

int bus_space_init(struct powerpc_bus_space *, const char *, caddr_t, size_t);
void bus_space_mallocok(void);

/*
 * Access methods for bus resources
 */

#define	__BUS_SPACE_HAS_STREAM_METHODS

/*
 *	void *bus_space_vaddr (bus_space_tag_t, bus_space_handle_t);
 *
 * Get the kernel virtual address for the mapped bus space.
 * Only allowed for regions mapped with BUS_SPACE_MAP_LINEAR.
 *  (XXX not enforced)
 */
#define bus_space_vaddr(t, h) ((void *)(h))

/*
 *	paddr_t bus_space_mmap  (bus_space_tag_t t, bus_addr_t addr,
 *	    off_t offset, int prot, int flags);
 *
 * Mmap a region of bus space.
 */

#define bus_space_mmap(t, b, o, p, f)					\
    ((*(t)->pbs_mmap)((t), (b), (o), (p), (f)))

/*
 *	int bus_space_map  (bus_space_tag_t t, bus_addr_t addr,
 *	    bus_size_t size, int flags, bus_space_handle_t *bshp);
 *
 * Map a region of bus space.
 */

#define bus_space_map(t, a, s, f, hp)					\
    ((*(t)->pbs_map)((t), (a), (s), (f), (hp)))

/*
 *	int bus_space_unmap (bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size);
 *
 * Unmap a region of bus space.
 */

#define bus_space_unmap(t, h, s)					\
    ((void)(*(t)->pbs_unmap)((t), (h), (s)))

/*
 *	int bus_space_subregion (bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t offset, bus_size_t size,
 *	    bus_space_handle_t *nbshp);
 *
 * Get a new handle for a subregion of an already-mapped area of bus space.
 */

#define bus_space_subregion(t, h, o, s, hp)				\
    ((*(t)->pbs_subregion)((t), (h), (o), (s), (hp)))

/*
 *	int bus_space_alloc (bus_space_tag_t t, bus_addr_t rstart,
 *	    bus_addr_t rend, bus_size_t size, bus_size_t align,
 *	    bus_size_t boundary, int flags, bus_addr_t *bpap,
 *	    bus_space_handle_t *bshp);
 *
 * Allocate a region of bus space.
 */

#define bus_space_alloc(t, rs, re, s, a, b, f, ap, hp)			\
    ((*(t)->pbs_alloc)((t), (rs), (re), (s), (a), (b), (f), (ap), (hp)))

/*
 *	int bus_space_free (bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size);
 *
 * Free a region of bus space.
 */

#define	bus_space_free(t, h, s)						\
    ((void)(*(t)->pbs_free)((t), (h), (s)))

/*
 *	u_intN_t bus_space_read_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

#define bus_space_read_1(t, h, o)					\
	((*(t)->pbs_scalar.pbss_read_1)((t), (h), (o)))
#define bus_space_read_2(t, h, o)					\
	((*(t)->pbs_scalar.pbss_read_2)((t), (h), (o)))
#define bus_space_read_4(t, h, o)					\
	((*(t)->pbs_scalar.pbss_read_4)((t), (h), (o)))
#define bus_space_read_8(t, h, o)					\
	((*(t)->pbs_scalar.pbss_read_8)((t), (h), (o)))

/*
 *	u_intN_t bus_space_read_stream_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset ignoring endianness.
 */

#define bus_space_read_stream_2(t, h, o)				\
	((*(t)->pbs_scalar_stream.pbss_read_2)((t), (h), (o)))
#define bus_space_read_stream_4(t, h, o)				\
	((*(t)->pbs_scalar_stream.pbss_read_4)((t), (h), (o)))
#define bus_space_read_stream_8(t, h, o)				\
	((*(t)->pbs_scalar_stream.pbss_read_8)((t), (h), (o)))

/*
 *	void bus_space_read_multi_N _P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

#define bus_space_read_multi_1(t, h, o, a, c)				\
	((*(t)->pbs_multi->pbsg_read_1)((t), (h), (o), (a), (c)))
#define bus_space_read_multi_2(t, h, o, a, c)				\
	((*(t)->pbs_multi->pbsg_read_2)((t), (h), (o), (a), (c)))
#define bus_space_read_multi_4(t, h, o, a, c)				\
	((*(t)->pbs_multi->pbsg_read_4)((t), (h), (o), (a), (c)))
#define bus_space_read_multi_8(t, h, o, a, c)				\
	((*(t)->pbs_multi->pbsg_read_8)((t), (h), (o), (a), (c)))

/*
 *	void bus_space_read_multi_stream_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 2, 4, or 8 byte stream quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

#define bus_space_read_multi_stream_2(t, h, o, a, c)			\
	((*(t)->pbs_multi_stream->pbsg_read_2)((t), (h), (o), (a), (c)))
#define bus_space_read_multi_stream_4(t, h, o, a, c)			\
	((*(t)->pbs_multi_stream->pbsg_read_4)((t), (h), (o), (a), (c)))
#define bus_space_read_multi_stream_8(t, h, o, a, c)			\
	((*(t)->pbs_multi_stream->pbsg_read_8)((t), (h), (o), (a), (c)))

/*
 *	void bus_space_write_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

#define bus_space_write_1(t, h, o, v)					\
	((*(t)->pbs_scalar.pbss_write_1)((t), (h), (o), (v)))
#define bus_space_write_2(t, h, o, v)					\
	((*(t)->pbs_scalar.pbss_write_2)((t), (h), (o), (v)))
#define bus_space_write_4(t, h, o, v)					\
	((*(t)->pbs_scalar.pbss_write_4)((t), (h), (o), (v)))
#define bus_space_write_8(t, h, o, v)					\
	((*(t)->pbs_scalar.pbss_write_8)((t), (h), (o), (v)))

/*
 *	void bus_space_write_stream_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value);
 *
 * Write the 2, 4, or 8 byte stream value `value' to bus space
 * described by tag/handle/offset.
 */

#define bus_space_write_stream_1(t, h, o, v)				\
	((*(t)->pbs_scalar_stream.pbss_write_1)((t), (h), (o), (v)))
#define bus_space_write_stream_2(t, h, o, v)				\
	((*(t)->pbs_scalar_stream.pbss_write_2)((t), (h), (o), (v)))
#define bus_space_write_stream_4(t, h, o, v)				\
	((*(t)->pbs_scalar_stream.pbss_write_4)((t), (h), (o), (v)))
#define bus_space_write_stream_8(t, h, o, v)				\
	((*(t)->pbs_scalar_stream.pbss_write_8)((t), (h), (o), (v)))

/*
 *	void bus_space_write_multi_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

#define bus_space_write_multi_1(t, h, o, a, c)				\
	((*(t)->pbs_multi->pbsg_write_1)((t), (h), (o), (a), (c)))
#define bus_space_write_multi_2(t, h, o, a, c)				\
	((*(t)->pbs_multi->pbsg_write_2)((t), (h), (o), (a), (c)))
#define bus_space_write_multi_4(t, h, o, a, c)				\
	((*(t)->pbs_multi->pbsg_write_4)((t), (h), (o), (a), (c)))
#define bus_space_write_multi_8(t, h, o, a, c)				\
	((*(t)->pbs_multi->pbsg_write_8)((t), (h), (o), (a), (c)))

/*
 *	void bus_space_write_multi_stream_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 2, 4, or 8 byte stream quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

#define bus_space_write_multi_stream_1(t, h, o, a, c)			\
	((*(t)->pbs_multi_stream->pbsg_write_1)((t), (h), (o), (a), (c)))
#define bus_space_write_multi_stream_2(t, h, o, a, c)			\
	((*(t)->pbs_multi_stream->pbsg_write_2)((t), (h), (o), (a), (c)))
#define bus_space_write_multi_stream_4(t, h, o, a, c)			\
	((*(t)->pbs_multi_stream->pbsg_write_4)((t), (h), (o), (a), (c)))
#define bus_space_write_multi_stream_8(t, h, o, a, c)			\
	((*(t)->pbs_multi_stream->pbsg_write_8)((t), (h), (o), (a), (c)))

/*
 *	void bus_space_read_region_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */
#define bus_space_read_region_1(t, h, o, a, c)				\
	((*(t)->pbs_region->pbsg_read_1)((t), (h), (o), (a), (c)))
#define bus_space_read_region_2(t, h, o, a, c)				\
	((*(t)->pbs_region->pbsg_read_2)((t), (h), (o), (a), (c)))
#define bus_space_read_region_4(t, h, o, a, c)				\
	((*(t)->pbs_region->pbsg_read_4)((t), (h), (o), (a), (c)))
#define bus_space_read_region_8(t, h, o, a, c)				\
	((*(t)->pbs_region->pbsg_read_8)((t), (h), (o), (a), (c)))

/*
 *	void bus_space_read_region_stream_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 2, 4, or 8 byte stream quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */
#define bus_space_read_region_stream_2(t, h, o, a, c)			\
	((*(t)->pbs_region_stream->pbsg_read_2)((t), (h), (o), (a), (c)))
#define bus_space_read_region_stream_4(t, h, o, a, c)			\
	((*(t)->pbs_region_stream->pbsg_read_4)((t), (h), (o), (a), (c)))
#define bus_space_read_region_stream_8(t, h, o, a, c)			\
	((*(t)->pbs_region_stream->pbsg_read_8)((t), (h), (o), (a), (c)))

/*
 *	void bus_space_write_region_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */
#define bus_space_write_region_1(t, h, o, a, c)				\
	((*(t)->pbs_region->pbsg_write_1)((t), (h), (o), (a), (c)))
#define bus_space_write_region_2(t, h, o, a, c)				\
	((*(t)->pbs_region->pbsg_write_2)((t), (h), (o), (a), (c)))
#define bus_space_write_region_4(t, h, o, a, c)				\
	((*(t)->pbs_region->pbsg_write_4)((t), (h), (o), (a), (c)))
#define bus_space_write_region_8(t, h, o, a, c)				\
	((*(t)->pbs_region->pbsg_write_8)((t), (h), (o), (a), (c)))

/*
 *	void bus_space_write_region_stream_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 2, 4, or 8 byte stream quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */
#define bus_space_write_region_stream_2(t, h, o, a, c)			\
	((*(t)->pbs_region_stream->pbsg_write_2)((t), (h), (o), (a), (c)))
#define bus_space_write_region_stream_4(t, h, o, a, c)			\
	((*(t)->pbs_region_stream->pbsg_write_4)((t), (h), (o), (a), (c)))
#define bus_space_write_region_stream_8(t, h, o, a, c)			\
	((*(t)->pbs_region_stream->pbsg_write_8)((t), (h), (o), (a), (c)))

#if 0
/*
 *	void bus_space_set_multi_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */
#define	bus_space_set_multi_1(t, h, o, v, c)
	((*(t)->pbs_set_multi_1)((t), (h), (o), (v), (c)))
#define	bus_space_set_multi_2(t, h, o, v, c)
	((*(t)->pbs_set_multi_2)((t), (h), (o), (v), (c)))
#define	bus_space_set_multi_4(t, h, o, v, c)
	((*(t)->pbs_set_multi_4)((t), (h), (o), (v), (c)))
#define	bus_space_set_multi_8(t, h, o, v, c)
	((*(t)->pbs_set_multi_8)((t), (h), (o), (v), (c)))

/*
 *	void bus_space_set_multi_stream_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count);
 *
 * Write the 2, 4, or 8 byte stream value `val' to bus space described
 * by tag/handle/offset `count' times.
 */
#define	bus_space_set_multi_stream_2(t, h, o, v, c)
	((*(t)->pbs_set_multi_stream_2)((t), (h), (o), (v), (c)))
#define	bus_space_set_multi_stream_4(t, h, o, v, c)
	((*(t)->pbs_set_multi_stream_4)((t), (h), (o), (v), (c)))
#define	bus_space_set_multi_stream_8(t, h, o, v, c)
	((*(t)->pbs_set_multi_stream_8)((t), (h), (o), (v), (c)))

#endif

/*
 *	void bus_space_set_region_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */
#define bus_space_set_region_1(t, h, o, v, c)				\
	((*(t)->pbs_set->pbss_set_1)((t), (h), (o), (v), (c)))
#define bus_space_set_region_2(t, h, o, v, c)				\
	((*(t)->pbs_set->pbss_set_2)((t), (h), (o), (v), (c)))
#define bus_space_set_region_4(t, h, o, v, c)				\
	((*(t)->pbs_set->pbss_set_4)((t), (h), (o), (v), (c)))
#define bus_space_set_region_8(t, h, o, v, c)				\
	((*(t)->pbs_set->pbss_set_8)((t), (h), (o), (v), (c)))

/*
 *	void bus_space_set_region_stream_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count);
 *
 * Write `count' 2, 4, or 8 byte stream value `val' to bus space described
 * by tag/handle starting at `offset'.
 */
#define bus_space_set_region_stream_2(t, h, o, v, c)			\
	((*(t)->pbs_set_stream->pbss_set_2)((t), (h), (o), (v), (c)))
#define bus_space_set_region_stream_4(t, h, o, v, c)			\
	((*(t)->pbs_set_stream->pbss_set_4)((t), (h), (o), (v), (c)))
#define bus_space_set_region_stream_8(t, h, o, v, c)			\
	((*(t)->pbs_set_stream->pbss_set_8)((t), (h), (o), (v), (c)))


/*
 *	void bus_space_copy_region_N (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */
#define bus_space_copy_region_1(t, h1, o1, h2, o2, c)			\
	((*(t)->pbs_copy->pbsc_copy_1)((t), (h1), (o1), (h2), (o2), (c)))
#define bus_space_copy_region_2(t, h1, o1, h2, o2, c)			\
	((*(t)->pbs_copy->pbsc_copy_2)((t), (h1), (o1), (h2), (o2), (c)))
#define bus_space_copy_region_4(t, h1, o1, h2, o2, c)			\
	((*(t)->pbs_copy->pbsc_copy_4)((t), (h1), (o1), (h2), (o2), (c)))
#define bus_space_copy_region_8(t, h1, o1, h2, o2, c)			\
	((*(t)->pbs_copy->pbsc_copy_8)((t), (h1), (o1), (h2), (o2), (c)))

/*
 * Bus read/write barrier methods.
 *
 *	void bus_space_barrier (bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags);
 *
 */
#define	bus_space_barrier(t, h, o, l, f)	\
	((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f)))
#define	BUS_SPACE_BARRIER_READ	0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE	0x02		/* force write barrier */

#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

/*
 * Bus DMA methods.
 */

/*
 * Flags used in various bus DMA methods.
 */
#define	BUS_DMA_WAITOK		0x000	/* safe to sleep (pseudo-flag) */
#define	BUS_DMA_NOWAIT		0x001	/* not safe to sleep */
#define	BUS_DMA_ALLOCNOW	0x002	/* perform resource allocation now */
/* Allow machine-dependent code to override BUS_DMA_COHERENT */
#ifndef BUS_DMA_COHERENT
#define	BUS_DMA_COHERENT	0x004	/* hint: map memory DMA coherent */
#endif
#define	BUS_DMA_STREAMING	0x008	/* hint: sequential, unidirectional */
#define	BUS_DMA_BUS1		0x010	/* placeholders for bus functions... */
#define	BUS_DMA_BUS2		0x020
#define	BUS_DMA_BUS3		0x040
#define	BUS_DMA_BUS4		0x080
#define	BUS_DMA_READ		0x100	/* mapping is device -> memory only */
#define	BUS_DMA_WRITE		0x200	/* mapping is memory -> device only */
#define	BUS_DMA_NOCACHE		0x400	/* hint: map non-cached memory */

/* Forwards needed by prototypes below. */
struct proc;
struct mbuf;
struct uio;

/*
 * Operations performed by bus_dmamap_sync().
 */
#define	BUS_DMASYNC_PREREAD	0x01	/* pre-read synchronization */
#define	BUS_DMASYNC_POSTREAD	0x02	/* post-read synchronization */
#define	BUS_DMASYNC_PREWRITE	0x04	/* pre-write synchronization */
#define	BUS_DMASYNC_POSTWRITE	0x08	/* post-write synchronization */

typedef struct powerpc_bus_dma_tag		*bus_dma_tag_t;
typedef struct powerpc_bus_dmamap		*bus_dmamap_t;

#define BUS_DMA_TAG_VALID(t)    ((t) != (bus_dma_tag_t)0)

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct powerpc_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */
};
typedef struct powerpc_bus_dma_segment	bus_dma_segment_t;

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */

struct powerpc_bus_dma_tag {
	/*
	 * The `bounce threshold' is checked while we are loading
	 * the DMA map.  If the physical address of the segment
	 * exceeds the threshold, an error will be returned.  The
	 * caller can then take whatever action is necessary to
	 * bounce the transfer.  If this value is 0, it will be
	 * ignored.
	 */
	bus_addr_t _bounce_thresh;

	/*
	 * DMA mapping methods.
	 */
	int	(*_dmamap_create) (bus_dma_tag_t, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *);
	void	(*_dmamap_destroy) (bus_dma_tag_t, bus_dmamap_t);
	int	(*_dmamap_load) (bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int);
	int	(*_dmamap_load_mbuf) (bus_dma_tag_t, bus_dmamap_t,
		    struct mbuf *, int);
	int	(*_dmamap_load_uio) (bus_dma_tag_t, bus_dmamap_t,
		    struct uio *, int);
	int	(*_dmamap_load_raw) (bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int);
	void	(*_dmamap_unload) (bus_dma_tag_t, bus_dmamap_t);
	void	(*_dmamap_sync) (bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int);

	/*
	 * DMA memory utility functions.
	 */
	int	(*_dmamem_alloc) (bus_dma_tag_t, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int);
	void	(*_dmamem_free) (bus_dma_tag_t,
		    bus_dma_segment_t *, int);
	int	(*_dmamem_map) (bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, caddr_t *, int);
	void	(*_dmamem_unmap) (bus_dma_tag_t, caddr_t, size_t);
	paddr_t	(*_dmamem_mmap) (bus_dma_tag_t, bus_dma_segment_t *,
		    int, off_t, int, int);

#ifndef PHYS_TO_BUS_MEM
	bus_addr_t (*_dma_phys_to_bus_mem)(bus_dma_tag_t, bus_addr_t);
#define	PHYS_TO_BUS_MEM(t, addr) (*(t)->_dma_phys_to_bus_mem)((t), (addr))
#endif
#ifndef BUS_MEM_TO_PHYS
	bus_addr_t (*_dma_bus_mem_to_phys)(bus_dma_tag_t, bus_addr_t);
#define	BUS_MEM_TO_PHYS(t, addr) (*(t)->_dma_bus_mem_to_phys)((t), (addr))
#endif
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
#define	bus_dmamap_sync(t, p, o, l, ops)			\
	(void)((t)->_dmamap_sync ?				\
	    (*(t)->_dmamap_sync)((t), (p), (o), (l), (ops)) : (void)0)

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
struct powerpc_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use my machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxsegsz;	/* largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	bus_addr_t	_dm_bounce_thresh; /* bounce threshold; see tag */
	int		_dm_flags;	/* misc. flags */

	void		*_dm_cookie;	/* cookie for bus-specific functions */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

#ifdef _POWERPC_BUS_DMA_PRIVATE
int	_bus_dmamap_create (bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *);
void	_bus_dmamap_destroy (bus_dma_tag_t, bus_dmamap_t);
int	_bus_dmamap_load (bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int);
int	_bus_dmamap_load_mbuf (bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int);
int	_bus_dmamap_load_uio (bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int);
int	_bus_dmamap_load_raw (bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int);
void	_bus_dmamap_unload (bus_dma_tag_t, bus_dmamap_t);
void	_bus_dmamap_sync (bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int);

int	_bus_dmamem_alloc (bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags);
void	_bus_dmamem_free (bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs);
int	_bus_dmamem_map (bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, size_t size, caddr_t *kvap, int flags);
void	_bus_dmamem_unmap (bus_dma_tag_t tag, caddr_t kva,
	    size_t size);
paddr_t	_bus_dmamem_mmap (bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, off_t off, int prot, int flags);

int	_bus_dmamem_alloc_range (bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    paddr_t low, paddr_t high);
bus_addr_t _bus_dma_phys_to_bus_mem_generic(bus_dma_tag_t, bus_addr_t);
bus_addr_t _bus_dma_bus_mem_to_phys_generic(bus_dma_tag_t, bus_addr_t);
#endif /* _POWERPC_BUS_DMA_PRIVATE */
#endif /* _POWERPC_BUS_H_ */
