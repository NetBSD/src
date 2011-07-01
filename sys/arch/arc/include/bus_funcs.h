/*	$NetBSD: bus_funcs.h,v 1.1 2011/07/01 17:09:58 dyoung Exp $	*/
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

#ifndef _ARC_BUS_FUNCS_H_
#define _ARC_BUS_FUNCS_H_
#ifdef _KERNEL

/*
 * Utility macro; do not use outside this file.
 */
#ifdef __STDC__
#define __CONCAT3(a,b,c)	a##b##c
#else
#define __CONCAT3(a,b,c)	a/**/b/**/c
#endif

/* machine dependent utility function for bus_space users */
void	arc_bus_space_malloc_set_safe(void);
void	arc_bus_space_init(bus_space_tag_t, const char *,
	    paddr_t, vaddr_t, bus_addr_t, bus_size_t);
void	arc_bus_space_init_extent(bus_space_tag_t, void *, size_t);
void	arc_bus_space_set_aligned_stride(bus_space_tag_t, unsigned int);
void	arc_sparse_bus_space_init(bus_space_tag_t, const char *,
	    paddr_t, bus_addr_t, bus_size_t);
void	arc_large_bus_space_init(bus_space_tag_t, const char *,
	    paddr_t, bus_addr_t, bus_size_t);

/* machine dependent utility function for bus_space implementations */
int	arc_bus_space_extent_malloc_flag(void);

/* these are provided for subclasses which override base bus_space. */

int	arc_bus_space_compose_handle(bus_space_tag_t,
	    bus_addr_t, bus_size_t, int, bus_space_handle_t *);
int	arc_bus_space_dispose_handle(bus_space_tag_t,
	    bus_space_handle_t, bus_size_t);
int	arc_bus_space_paddr(bus_space_tag_t,
	    bus_space_handle_t, paddr_t *);

int	arc_sparse_bus_space_compose_handle(bus_space_tag_t,
	    bus_addr_t, bus_size_t, int, bus_space_handle_t *);
int	arc_sparse_bus_space_dispose_handle(bus_space_tag_t,
	    bus_space_handle_t, bus_size_t);
int	arc_sparse_bus_space_paddr(bus_space_tag_t,
	    bus_space_handle_t, paddr_t *);

int	arc_bus_space_map(bus_space_tag_t, bus_addr_t, bus_size_t, int,
	    bus_space_handle_t *);
void	arc_bus_space_unmap(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t);
int	arc_bus_space_subregion(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, bus_size_t, bus_space_handle_t *);
paddr_t	arc_bus_space_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);
int	arc_bus_space_alloc(bus_space_tag_t, bus_addr_t, bus_addr_t,
	    bus_size_t, bus_size_t, bus_size_t, int, bus_addr_t *,
	    bus_space_handle_t *);
#define arc_bus_space_free	arc_bus_space_unmap

/*
 *	int bus_space_compose_handle(bus_space_tag_t t, bus_addr_t addr,
 *	    bus_size_t size, int flags, bus_space_handle_t *bshp);
 *
 * MACHINE DEPENDENT, NOT PORTABLE INTERFACE:
 * Compose a bus_space handle from tag/handle/addr/size/flags.
 * A helper function for bus_space_map()/bus_space_alloc() implementation.
 */
#define bus_space_compose_handle(bst, addr, size, flags, bshp)		\
	(*(bst)->bs_compose_handle)(bst, addr, size, flags, bshp)

/*
 *	int bus_space_dispose_handle(bus_space_tag_t t, bus_addr_t addr,
 *	    bus_space_handle_t bsh, bus_size_t size);
 *
 * MACHINE DEPENDENT, NOT PORTABLE INTERFACE:
 * Dispose a bus_space handle.
 * A helper function for bus_space_unmap()/bus_space_free() implementation.
 */
#define bus_space_dispose_handle(bst, bsh, size)			\
	(*(bst)->bs_dispose_handle)(bst, bsh, size)

/*
 *	int bus_space_paddr(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, paddr_t *pap);
 *
 * MACHINE DEPENDENT, NOT PORTABLE INTERFACE:
 * (cannot be implemented on e.g. I/O space on i386, non-linear space on alpha)
 * Return physical address of a region.
 * A helper function for machine-dependent device mmap entry.
 */
#define bus_space_paddr(bst, bsh, pap)					\
	(*(bst)->bs_paddr)(bst, bsh, pap)

/*
 *	void *bus_space_vaddr(bus_space_tag_t, bus_space_handle_t);
 *
 * Get the kernel virtual address for the mapped bus space.
 * Only allowed for regions mapped with BUS_SPACE_MAP_LINEAR.
 *  (XXX not enforced)
 */
#define bus_space_vaddr(bst, bsh)					\
	((void *)(bsh))

/*
 *	paddr_t bus_space_mmap(bus_space_tag_t, bus_addr_t, off_t,
 *	    int, int);
 *
 * Mmap bus space on behalf of the user.
 */
#define	bus_space_mmap(bst, addr, off, prot, flags)			\
	(*(bst)->bs_mmap)((bst), (addr), (off), (prot), (flags))

/*
 *	int bus_space_map(bus_space_tag_t t, bus_addr_t addr,
 *	    bus_size_t size, int flags, bus_space_handle_t *bshp);
 *
 * Map a region of bus space.
 */

#define bus_space_map(t, a, s, f, hp)					\
	(*(t)->bs_map)((t), (a), (s), (f), (hp))

/*
 *	void bus_space_unmap(bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size);
 *
 * Unmap a region of bus space.
 */

#define bus_space_unmap(t, h, s)					\
	(*(t)->bs_unmap)((t), (h), (s))

/*
 *	int bus_space_subregion(bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t offset, bus_size_t size,
 *	    bus_space_handle_t *nbshp);
 *
 * Get a new handle for a subregion of an already-mapped area of bus space.
 */

#define bus_space_subregion(t, h, o, s, hp)				\
	(*(t)->bs_subregion)((t), (h), (o), (s), (hp))

/*
 *	int bus_space_alloc(bus_space_tag_t t, bus_addr_t, rstart,
 *	    bus_addr_t rend, bus_size_t size, bus_size_t align,
 *	    bus_size_t boundary, int flags, bus_addr_t *addrp,
 *	    bus_space_handle_t *bshp);
 *
 * Allocate a region of bus space.
 */

#define bus_space_alloc(t, rs, re, s, a, b, f, ap, hp)			\
	(*(t)->bs_alloc)((t), (rs), (re), (s), (a), (b), (f), (ap), (hp))

/*
 *	int bus_space_free(bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size);
 *
 * Free a region of bus space.
 */

#define bus_space_free(t, h, s)						\
	(*(t)->bs_free)((t), (h), (s))

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
 *	void bus_space_barrier(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags);
 *
 * On the MIPS, we just flush the write buffer.
 */
#define bus_space_barrier(t, h, o, l, f)				\
	((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f),	\
	 wbflush()))

/* Forwards needed by prototypes below. */
struct mbuf;
struct uio;

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

#define bus_dmatag_subregion(t, mna, mxa, nt, f) EOPNOTSUPP
#define bus_dmatag_destroy(t)

#ifdef _ARC_BUS_DMA_PRIVATE
int	_bus_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *);
void	_bus_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
int	_bus_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int);
int	_bus_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int);
int	_bus_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int);
int	_bus_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int);
void	_bus_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	_bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int);

int	_bus_dmamem_alloc(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags);
int	_bus_dmamem_alloc_range(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    paddr_t low, paddr_t high);
void	_bus_dmamem_free(bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs);
int	_bus_dmamem_map(bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, size_t size, void **kvap, int flags);
void	_bus_dmamem_unmap(bus_dma_tag_t tag, void *kva,
	    size_t size);
paddr_t	_bus_dmamem_mmap(bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, off_t off, int prot, int flags);

int	_bus_dmamem_alloc_range(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    paddr_t low, paddr_t high);
#endif /* _ARC_BUS_DMA_PRIVATE */

void	_bus_dma_tag_init(bus_dma_tag_t tag);
void	jazz_bus_dma_tag_init(bus_dma_tag_t tag);
void	isadma_bounce_tag_init(bus_dma_tag_t tag);

#endif /* _KERNEL */
#endif /* _ARC_BUS_FUNCS_H_ */
