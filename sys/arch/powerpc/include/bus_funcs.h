/*	$NetBSD: bus_funcs.h,v 1.2 2022/03/10 00:14:16 riastradh Exp $	*/
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

#ifndef _POWERPC_BUS_FUNCS_H_
#define _POWERPC_BUS_FUNCS_H_

#ifndef __HAVE_LOCAL_BUS_SPACE

#ifdef __STDC__
#define CAT(a,b)	a##b
#define CAT3(a,b,c)	a##b##c
#else
#define CAT(a,b)	a/**/b
#define CAT3(a,b,c)	a/**/b/**/c
#endif

int bus_space_init(struct powerpc_bus_space *, const char *, void *, size_t);
void bus_space_mallocok(void);

/*
 * Access methods for bus resources
 */

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
 *	uintN_t bus_space_read_N (bus_space_tag_t tag,
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
 *	uintN_t bus_space_read_stream_N (bus_space_tag_t tag,
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
 *	    uintN_t *addr, size_t count);
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
 *	    uintN_t *addr, size_t count);
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
 *	    uintN_t value);
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
 *	    uintN_t value);
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
 *	    const uintN_t *addr, size_t count);
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
 *	    const uintN_t *addr, size_t count);
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
 *	    uintN_t *addr, size_t count);
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
 *	    uintN_t *addr, size_t count);
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
 *	    const uintN_t *addr, size_t count);
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
 *	    const uintN_t *addr, size_t count);
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
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
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
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
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
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
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
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
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
	((*(t)->pbs_barrier)((t), (h), (o), (l), (f)))

#endif	/* !__HAVE_LOCAL_BUS_SPACE */

/*
 * Bus DMA methods.
 */

/* Forwards needed by prototypes below. */
struct proc;
struct mbuf;
struct uio;

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

#define bus_dmatag_subregion(t, mna, mxa, nt, f) EOPNOTSUPP
#define bus_dmatag_destroy(t)

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
	    int nsegs, size_t size, void **kvap, int flags);
void	_bus_dmamem_unmap (bus_dma_tag_t tag, void *kva,
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
#endif /* _POWERPC_BUS_FUNCS_H_ */
