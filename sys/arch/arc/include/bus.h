/*	$NetBSD: bus.h,v 1.10.2.1 2001/08/03 04:10:56 lukem Exp $	*/
/*	NetBSD: bus.h,v 1.27 2000/03/15 16:44:50 drochner Exp 	*/
/*	$OpenBSD: bus.h,v 1.15 1999/08/11 23:15:21 niklas Exp $	*/

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

#ifndef _ARC_BUS_H_
#define _ARC_BUS_H_
#ifdef _KERNEL

#include <mips/locore.h>

#ifdef BUS_SPACE_DEBUG
#include <sys/systm.h> /* for printf() prototype */
/*
 * Macros for checking the aligned-ness of pointers passed to bus
 * space ops.  Strict alignment is required by the MIPS architecture,
 * and a trap will occur if unaligned access is performed.  These
 * may aid in the debugging of a broken device driver by displaying
 * useful information about the problem.
 */
#define __BUS_SPACE_ALIGNED_ADDRESS(p, t)				\
	((((u_long)(p)) & (sizeof(t)-1)) == 0)

#define __BUS_SPACE_ADDRESS_SANITY(p, t, d)				\
({									\
	if (__BUS_SPACE_ALIGNED_ADDRESS((p), t) == 0) {			\
		printf("%s 0x%lx not aligned to %d bytes %s:%d\n",	\
		    d, (u_long)(p), sizeof(t), __FILE__, __LINE__);	\
	}								\
	(void) 0;							\
})

#define BUS_SPACE_ALIGNED_POINTER(p, t) __BUS_SPACE_ALIGNED_ADDRESS(p, t)
#else
#define __BUS_SPACE_ADDRESS_SANITY(p,t,d)	(void) 0
#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)
#endif /* BUS_SPACE_DEBUG */

/*
 * Utility macro; do not use outside this file.
 */
#ifdef __STDC__
#define __CONCAT3(a,b,c)	a##b##c
#else
#define __CONCAT3(a,b,c)	a/**/b/**/c
#endif

/*
 * Bus address and size types
 */
typedef u_long bus_addr_t;
typedef u_long bus_size_t;

/*
 * Access methods for bus resources and address space.
 */
typedef u_int32_t bus_space_handle_t;
typedef struct arc_bus_space *bus_space_tag_t;

struct arc_bus_space {
	const char	*bs_name;
	struct extent	*bs_extent;
	bus_addr_t	bs_start;
	bus_size_t	bs_size;

	paddr_t		bs_pbase;
	vaddr_t		bs_vbase;

	/* sparse addressing shift count */
	u_int8_t	bs_stride_1;
	u_int8_t	bs_stride_2;
	u_int8_t	bs_stride_4;
	u_int8_t	bs_stride_8;

	/* compose a bus_space handle from tag/handle/addr/size/flags (MD) */
	int	(*bs_compose_handle) __P((bus_space_tag_t, bus_addr_t,
				bus_size_t, int, bus_space_handle_t *));

	/* dispose a bus_space handle (MD) */
	int	(*bs_dispose_handle) __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t));

	/* convert bus_space tag/handle to physical address (MD) */
	int	(*bs_paddr) __P((bus_space_tag_t, bus_space_handle_t,
				paddr_t *));

	/* mapping/unmapping */
	int	(*bs_map) __P((bus_space_tag_t, bus_addr_t, bus_size_t, int,
				bus_space_handle_t *));
	void	(*bs_unmap) __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t));
	int	(*bs_subregion) __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, bus_size_t,	bus_space_handle_t *));

	/* allocation/deallocation */
	int	(*bs_alloc) __P((bus_space_tag_t, bus_addr_t, bus_addr_t,
				bus_size_t, bus_size_t,	bus_size_t, int,
				bus_addr_t *, bus_space_handle_t *));
	void	(*bs_free) __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t));

	void	*bs_aux;
};

/* vaddr_t argument of arc_bus_space_init() */
#define ARC_BUS_SPACE_UNMAPPED	((vaddr_t)0)

/* machine dependent utility function for bus_space users */
void	arc_bus_space_malloc_set_safe __P((void));
void	arc_bus_space_init __P((bus_space_tag_t, const char *,
	    paddr_t, vaddr_t, bus_addr_t, bus_size_t));
void	arc_bus_space_init_extent __P((bus_space_tag_t, caddr_t, size_t));
void	arc_bus_space_set_aligned_stride __P((bus_space_tag_t, unsigned int));
void	arc_sparse_bus_space_init __P((bus_space_tag_t, const char *,
	    paddr_t, bus_addr_t, bus_size_t));
void	arc_large_bus_space_init __P((bus_space_tag_t, const char *,
	    paddr_t, bus_addr_t, bus_size_t));

/* machine dependent utility function for bus_space implementations */
int	arc_bus_space_extent_malloc_flag __P((void));

/* these are provided for subclasses which override base bus_space. */

int	arc_bus_space_compose_handle __P((bus_space_tag_t,
	    bus_addr_t, bus_size_t, int, bus_space_handle_t *));
int	arc_bus_space_dispose_handle __P((bus_space_tag_t,
	    bus_space_handle_t, bus_size_t));
int	arc_bus_space_paddr __P((bus_space_tag_t,
	    bus_space_handle_t, paddr_t *));

int	arc_sparse_bus_space_compose_handle __P((bus_space_tag_t,
	    bus_addr_t, bus_size_t, int, bus_space_handle_t *));
int	arc_sparse_bus_space_dispose_handle __P((bus_space_tag_t,
	    bus_space_handle_t, bus_size_t));
int	arc_sparse_bus_space_paddr __P((bus_space_tag_t,
	    bus_space_handle_t, paddr_t *));

int	arc_bus_space_map __P((bus_space_tag_t, bus_addr_t, bus_size_t, int,
	    bus_space_handle_t *));
void	arc_bus_space_unmap __P((bus_space_tag_t, bus_space_handle_t,
	    bus_size_t));
int	arc_bus_space_subregion __P((bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, bus_size_t, bus_space_handle_t *));
int	arc_bus_space_alloc __P((bus_space_tag_t, bus_addr_t, bus_addr_t,
	    bus_size_t, bus_size_t, bus_size_t, int, bus_addr_t *,
	    bus_space_handle_t *));
#define arc_bus_space_free	arc_bus_space_unmap

/*
 *	int bus_space_compose_handle __P((bus_space_tag_t t, bus_addr_t addr,
 *	    bus_size_t size, int flags, bus_space_handle_t *bshp));
 *
 * MACHINE DEPENDENT, NOT PORTABLE INTERFACE:
 * Compose a bus_space handle from tag/handle/addr/size/flags.
 * A helper function for bus_space_map()/bus_space_alloc() implementation.
 */
#define bus_space_compose_handle(bst, addr, size, flags, bshp)		\
	(*(bst)->bs_compose_handle)(bst, addr, size, flags, bshp)

/*
 *	int bus_space_dispose_handle __P((bus_space_tag_t t, bus_addr_t addr,
 *	    bus_space_handle_t bsh, bus_size_t size));
 *
 * MACHINE DEPENDENT, NOT PORTABLE INTERFACE:
 * Dispose a bus_space handle.
 * A helper function for bus_space_unmap()/bus_space_free() implementation.
 */
#define bus_space_dispose_handle(bst, bsh, size)			\
	(*(bst)->bs_dispose_handle)(bst, bsh, size)

/*
 *	int bus_space_paddr __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, paddr_t *pap));
 *
 * MACHINE DEPENDENT, NOT PORTABLE INTERFACE:
 * (cannot be implemented on e.g. I/O space on i386, non-linear space on alpha)
 * Return physical address of a region.
 * A helper function for device mmap entry.
 */
#define bus_space_paddr(bst, bsh, pap)					\
	(*(bst)->bs_paddr)(bst, bsh, pap)

/*
 *	void *bus_space_vaddr __P((bus_space_tag_t, bus_space_handle_t));
 *
 * Get the kernel virtual address for the mapped bus space.
 * Only allowed for regions mapped with BUS_SPACE_MAP_LINEAR.
 *  (XXX not enforced)
 */
#define bus_space_vaddr(bst, bsh)					\
	((void *)(bsh))

/*
 *	int bus_space_map __P((bus_space_tag_t t, bus_addr_t addr,
 *	    bus_size_t size, int flags, bus_space_handle_t *bshp));
 *
 * Map a region of bus space.
 */

#define BUS_SPACE_MAP_CACHEABLE		0x01
#define BUS_SPACE_MAP_LINEAR		0x02
#define BUS_SPACE_MAP_PREFETCHABLE	0x04

#define bus_space_map(t, a, s, f, hp)					\
	(*(t)->bs_map)((t), (a), (s), (f), (hp))

/*
 *	void bus_space_unmap __P((bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size));
 *
 * Unmap a region of bus space.
 */

#define bus_space_unmap(t, h, s)					\
	(*(t)->bs_unmap)((t), (h), (s))

/*
 *	int bus_space_subregion __P((bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t offset, bus_size_t size,
 *	    bus_space_handle_t *nbshp));
 *
 * Get a new handle for a subregion of an already-mapped area of bus space.
 */

#define bus_space_subregion(t, h, o, s, hp)				\
	(*(t)->bs_subregion)((t), (h), (o), (s), (hp))

/*
 *	int bus_space_alloc __P((bus_space_tag_t t, bus_addr_t, rstart,
 *	    bus_addr_t rend, bus_size_t size, bus_size_t align,
 *	    bus_size_t boundary, int flags, bus_addr_t *addrp,
 *	    bus_space_handle_t *bshp));
 *
 * Allocate a region of bus space.
 */

#define bus_space_alloc(t, rs, re, s, a, b, f, ap, hp)			\
	(*(t)->bs_alloc)((t), (rs), (re), (s), (a), (b), (f), (ap), (hp))

/*
 *	int bus_space_free __P((bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size));
 *
 * Free a region of bus space.
 */

#define bus_space_free(t, h, s)						\
	(*(t)->bs_free)((t), (h), (s))

/*
 *	u_intN_t bus_space_read_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset));
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

#define bus_space_read(BYTES,BITS)					\
static __inline __CONCAT3(u_int,BITS,_t)				\
__CONCAT(bus_space_read_,BYTES)(bus_space_tag_t bst,			\
    bus_space_handle_t bsh, bus_size_t offset)				\
{									\
	return (*(volatile __CONCAT3(u_int,BITS,_t) *)			\
	    (bsh + (offset << __CONCAT(bst->bs_stride_,BYTES))));	\
}

bus_space_read(1,8)
bus_space_read(2,16)
bus_space_read(4,32)
bus_space_read(8,64)

/*
 *	void bus_space_read_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count));
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

#define bus_space_read_multi(BYTES,BITS)				\
static __inline void							\
__CONCAT(bus_space_read_multi_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t bsh, bus_size_t offset,				\
    __CONCAT3(u_int,BITS,_t) *datap, bus_size_t count)			\
{									\
	volatile __CONCAT3(u_int,BITS,_t) *p =				\
	    (volatile __CONCAT3(u_int,BITS,_t) *)			\
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
 *	void bus_space_read_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count));
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */

#define bus_space_read_region(BYTES,BITS)				\
static __inline void							\
__CONCAT(bus_space_read_region_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t bsh, bus_size_t offset,				\
    __CONCAT3(u_int,BITS,_t) *datap, bus_size_t count)			\
{									\
	int stride = 1 << __CONCAT(bst->bs_stride_,BYTES);		\
	volatile __CONCAT3(u_int,BITS,_t) *p =				\
	    (volatile __CONCAT3(u_int,BITS,_t) *)			\
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
 *	void bus_space_write_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value));
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

#define bus_space_write(BYTES,BITS)					\
static __inline void							\
__CONCAT(bus_space_write_,BYTES)(bus_space_tag_t bst,			\
    bus_space_handle_t bsh,						\
    bus_size_t offset, __CONCAT3(u_int,BITS,_t) data)			\
{									\
	*(volatile __CONCAT3(u_int,BITS,_t) *)				\
	    (bsh + (offset << __CONCAT(bst->bs_stride_,BYTES))) = data; \
}

bus_space_write(1,8)
bus_space_write(2,16)
bus_space_write(4,32)
bus_space_write(8,64)

/*
 *	void bus_space_write_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

#define bus_space_write_multi(BYTES,BITS)				\
static __inline void							\
__CONCAT(bus_space_write_multi_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t bsh, bus_size_t offset,				\
    const __CONCAT3(u_int,BITS,_t) *datap, bus_size_t count)		\
{									\
	volatile __CONCAT3(u_int,BITS,_t) *p =				\
	    (volatile __CONCAT3(u_int,BITS,_t) *)			\
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
 *	void bus_space_write_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */

#define bus_space_write_region(BYTES,BITS)				\
static __inline void							\
__CONCAT(bus_space_write_region_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t bsh, bus_size_t offset,				\
    const __CONCAT3(u_int,BITS,_t) *datap, bus_size_t count)		\
{									\
	int stride = 1 << __CONCAT(bst->bs_stride_,BYTES);		\
	volatile __CONCAT3(u_int,BITS,_t) *p =				\
	    (volatile __CONCAT3(u_int,BITS,_t) *)			\
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
 *	void bus_space_set_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count));
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */

#define bus_space_set_multi(BYTES,BITS)					\
static __inline void							\
__CONCAT(bus_space_set_multi_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t bsh, bus_size_t offset,				\
    const __CONCAT3(u_int,BITS,_t) data, bus_size_t count)		\
{									\
	volatile __CONCAT3(u_int,BITS,_t) *p =				\
	    (volatile __CONCAT3(u_int,BITS,_t) *)			\
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
 *	void bus_space_set_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */

#define bus_space_set_region(BYTES,BITS)				\
static __inline void							\
__CONCAT(bus_space_set_region_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t bsh, bus_size_t offset,				\
    __CONCAT3(u_int,BITS,_t) data, bus_size_t count)			\
{									\
	int stride = 1 << __CONCAT(bst->bs_stride_,BYTES);		\
	volatile __CONCAT3(u_int,BITS,_t) *p =				\
	    (volatile __CONCAT3(u_int,BITS,_t) *)			\
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
 *	void bus_space_copy_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    size_t count));
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */

#define bus_space_copy_region(BYTES,BITS)				\
static __inline void							\
__CONCAT(bus_space_copy_region_,BYTES)(bus_space_tag_t bst,		\
    bus_space_handle_t srcbsh, bus_size_t srcoffset,			\
    bus_space_handle_t dstbsh, bus_size_t dstoffset, bus_size_t count)	\
{									\
	int stride = 1 << __CONCAT(bst->bs_stride_,BYTES);		\
	volatile __CONCAT3(u_int,BITS,_t) *srcp =			\
	    (volatile __CONCAT3(u_int,BITS,_t) *)			\
	    (srcbsh + (srcoffset << __CONCAT(bst->bs_stride_,BYTES)));	\
	volatile __CONCAT3(u_int,BITS,_t) *dstp =			\
	    (volatile __CONCAT3(u_int,BITS,_t) *)			\
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

/*
 * Operations which handle byte stream data on word access.
 *
 * These functions are defined to resolve endian mismatch, by either
 * - When normal (i.e. stream-less) operations perform byte swap
 *   to resolve endian mismatch, these functions bypass the byte swap.
 * or
 * - When bus bridge performs automatic byte swap, these functions
 *   perform byte swap once more, to cancel the bridge's behavior.
 *
 * Currently these are just same as normal operations, since all
 * supported buses are same endian with CPU (i.e. little-endian).
 *
 */
#define __BUS_SPACE_HAS_STREAM_METHODS
#define bus_space_read_stream_2(tag, bsh, offset)			\
	bus_space_read_2(tag, bsh, offset)
#define bus_space_read_stream_4(tag, bsh, offset)			\
	bus_space_read_4(tag, bsh, offset)
#define bus_space_read_stream_8(tag, bsh, offset)			\
	bus_space_read_8(tag, bsh, offset)
#define bus_space_read_multi_stream_2(tag, bsh, offset, datap, count)	\
	bus_space_read_multi_2(tag, bsh, offset, datap, count)
#define bus_space_read_multi_stream_4(tag, bsh, offset, datap, count)	\
	bus_space_read_multi_4(tag, bsh, offset, datap, count)
#define bus_space_read_multi_stream_8(tag, bsh, offset, datap, count)	\
	bus_space_read_multi_8(tag, bsh, offset, datap, count)
#define bus_space_read_region_stream_2(tag, bsh, offset, datap, count)	\
	bus_space_read_region_2(tag, bsh, offset, datap, count)
#define bus_space_read_region_stream_4(tag, bsh, offset, datap, count)	\
	bus_space_read_region_4(tag, bsh, offset, datap, count)
#define bus_space_read_region_stream_8(tag, bsh, offset, datap, count)	\
	bus_space_read_region_8(tag, bsh, offset, datap, count)
#define bus_space_write_stream_2(tag, bsh, offset, data)		\
	bus_space_write_2(tag, bsh, offset, data)
#define bus_space_write_stream_4(tag, bsh, offset, data)		\
	bus_space_write_4(tag, bsh, offset, data)
#define bus_space_write_stream_8(tag, bsh, offset, data)		\
	bus_space_write_8(tag, bsh, offset, data)
#define bus_space_write_multi_stream_2(tag, bsh, offset, datap, count)	\
	bus_space_write_multi_2(tag, bsh, offset, datap, count)
#define bus_space_write_multi_stream_4(tag, bsh, offset, datap, count)	\
	bus_space_write_multi_4(tag, bsh, offset, datap, count)
#define bus_space_write_multi_stream_8(tag, bsh, offset, datap, count)	\
	bus_space_write_multi_8(tag, bsh, offset, datap, count)
#define bus_space_write_region_stream_2(tag, bsh, offset, datap, count)	\
	bus_space_write_region_2(tag, bsh, offset, datap, count)
#define bus_space_write_region_stream_4(tag, bsh, offset, datap, count)	\
	bus_space_write_region_4(tag, bsh, offset, datap, count)
#define bus_space_write_region_stream_8(tag, bsh, offset, datap, count)	\
	bus_space_write_region_8(tag, bsh, offset, datap, count)
#define bus_space_write_region_stream_2(tag, bsh, offset, datap, count)	\
	bus_space_write_region_2(tag, bsh, offset, datap, count)
#define bus_space_write_region_stream_4(tag, bsh, offset, datap, count)	\
	bus_space_write_region_4(tag, bsh, offset, datap, count)
#define bus_space_write_region_stream_8(tag, bsh, offset, datap, count)	\
	bus_space_write_region_8(tag, bsh, offset, datap, count)
#define bus_space_set_multi_stream_2(tag, bsh, offset, data, count)	\
	bus_space_set_multi_2(tag, bsh, offset, data, count)
#define bus_space_set_multi_stream_4(tag, bsh, offset, data, count)	\
	bus_space_set_multi_4(tag, bsh, offset, data, count)
#define bus_space_set_multi_stream_8(tag, bsh, offset, data, count)	\
	bus_space_set_multi_8(tag, bsh, offset, data, count)
#define bus_space_set_region_stream_2(tag, bsh, offset, data, count)	\
	bus_space_set_region_2(tag, bsh, offset, data, count)
#define bus_space_set_region_stream_4(tag, bsh, offset, data, count)	\
	bus_space_set_region_4(tag, bsh, offset, data, count)
#define bus_space_set_region_stream_8(tag, bsh, offset, data, count)	\
	bus_space_set_region_8(tag, bsh, offset, data, count)

/*
 * Bus read/write barrier methods.
 *
 *	void bus_space_barrier __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags));
 *
 * On the MIPS, we just flush the write buffer.
 */
#define bus_space_barrier(t, h, o, l, f)				\
	((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f)),	\
	 wbflush())

#define BUS_SPACE_BARRIER_READ	0x01
#define BUS_SPACE_BARRIER_WRITE	0x02

/*
 * Flags used in various bus DMA methods.
 */
#define BUS_DMA_WAITOK		0x000	/* safe to sleep (pseudo-flag) */
#define BUS_DMA_NOWAIT		0x001	/* not safe to sleep */
#define BUS_DMA_ALLOCNOW	0x002	/* perform resource allocation now */
#define BUS_DMA_COHERENT	0x004	/* hint: map memory DMA coherent */
#define	BUS_DMA_STREAMING	0x008	/* hint: sequential, unidirectional */
#define BUS_DMA_BUS1		0x010	/* placeholders for bus functions... */
#define BUS_DMA_BUS2		0x020
#define BUS_DMA_BUS3		0x040
#define BUS_DMA_BUS4		0x080
#define	BUS_DMA_READ		0x100	/* mapping is device -> memory only */
#define	BUS_DMA_WRITE		0x200	/* mapping is memory -> device only */

#define ARC_DMAMAP_COHERENT	0x100	/* no cache flush necessary on sync */

/* Forwards needed by prototypes below. */
struct mbuf;
struct uio;

/*
 * Operations performed by bus_dmamap_sync().
 */
#define BUS_DMASYNC_PREREAD	0x01	/* pre-read synchronization */
#define BUS_DMASYNC_POSTREAD	0x02	/* post-read synchronization */
#define BUS_DMASYNC_PREWRITE	0x04	/* pre-write synchronization */
#define BUS_DMASYNC_POSTWRITE	0x08	/* post-write synchronization */

typedef struct arc_bus_dma_tag		*bus_dma_tag_t;
typedef struct arc_bus_dmamap		*bus_dmamap_t;

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct arc_bus_dma_segment {
	/*
	 * PUBLIC MEMBERS: these are used by device drivers.
	 */
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */
	/*
	 * PRIVATE MEMBERS for the DMA back-end.: not for use by drivers.
	 */
	vaddr_t		_ds_paddr;	/* CPU physical address */
	vaddr_t		_ds_vaddr;	/* virtual address, 0 if invalid */
};
typedef struct arc_bus_dma_segment	bus_dma_segment_t;

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */

struct arc_bus_dma_tag {
	bus_addr_t	dma_offset;

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
		    bus_addr_t, bus_size_t, int));

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

#define bus_dmamap_create(t, s, n, m, b, f, p)			\
	(*(t)->_dmamap_create)((t), (s), (n), (m), (b), (f), (p))
#define bus_dmamap_destroy(t, p)				\
	(*(t)->_dmamap_destroy)((t), (p))
#define bus_dmamap_load(t, m, b, s, p, f)			\
	(*(t)->_dmamap_load)((t), (m), (b), (s), (p), (f))
#define bus_dmamap_load_mbuf(t, m, b, f)			\
	(*(t)->_dmamap_load_mbuf)((t), (m), (b), (f))
#define bus_dmamap_load_uio(t, m, u, f)				\
	(*(t)->_dmamap_load_uio)((t), (m), (u), (f))
#define bus_dmamap_load_raw(t, m, sg, n, s, f)			\
	(*(t)->_dmamap_load_raw)((t), (m), (sg), (n), (s), (f))
#define bus_dmamap_unload(t, p)					\
	(*(t)->_dmamap_unload)((t), (p))
#define bus_dmamap_sync(t, p, o, l, ops)			\
	(*(t)->_dmamap_sync)((t), (p), (o), (l), (ops))
#define bus_dmamem_alloc(t, s, a, b, sg, n, r, f)		\
	(*(t)->_dmamem_alloc)((t), (s), (a), (b), (sg), (n), (r), (f))
#define bus_dmamem_free(t, sg, n)				\
	(*(t)->_dmamem_free)((t), (sg), (n))
#define bus_dmamem_map(t, sg, n, s, k, f)			\
	(*(t)->_dmamem_map)((t), (sg), (n), (s), (k), (f))
#define bus_dmamem_unmap(t, k, s)				\
	(*(t)->_dmamem_unmap)((t), (k), (s))
#define bus_dmamem_mmap(t, sg, n, o, p, f)			\
	(*(t)->_dmamem_mmap)((t), (sg), (n), (o), (p), (f))

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct arc_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use by machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxsegsz;	/* largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	int		_dm_flags;	/* misc. flags */

	/*
	 * Private cookie to be used by the DMA back-end.
	 */
	void		*_dm_cookie;

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

#ifdef _ARC_BUS_DMA_PRIVATE
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
void	_mips1_bus_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int));
void	_mips3_bus_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int));

int	_bus_dmamem_alloc __P((bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags));
int	_bus_dmamem_alloc_range(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    paddr_t low, paddr_t high);
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
	    paddr_t low, paddr_t high));
#endif /* _ARC_BUS_DMA_PRIVATE */

void	_bus_dma_tag_init __P((bus_dma_tag_t tag));
void	jazz_bus_dma_tag_init __P((bus_dma_tag_t tag));
void	isadma_bounce_tag_init __P((bus_dma_tag_t tag));

#endif /* _KERNEL */
#endif /* _ARC_BUS_H_ */
