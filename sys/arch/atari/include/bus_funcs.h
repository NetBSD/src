/*	$NetBSD: bus_funcs.h,v 1.1 2011/07/01 17:09:58 dyoung Exp $	*/

/*-
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

/*
 * Copyright (c) 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef _ATARI_BUS_FUNCS_H_
#define _ATARI_BUS_FUNCS_H_

int	bus_space_alloc(bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t,
	    bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
int	bus_space_map(bus_space_tag_t, bus_addr_t, bus_size_t, int,
	    bus_space_handle_t *);
void	bus_space_unmap(bus_space_tag_t, bus_space_handle_t, bus_size_t);
int	bus_space_subregion(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    bus_size_t, bus_space_handle_t *);

/*
 * Tag allocation
 */
bus_space_tag_t		beb_alloc_bus_space_tag(bus_space_tag_t);
bus_space_tag_t		leb_alloc_bus_space_tag(bus_space_tag_t);

/*
 * XXX
 */
bus_space_tag_t	mb_alloc_bus_space_tag(void);
void		mb_free_bus_space_tag(bus_space_tag_t);

/*
 * Utility macros; INTERNAL USE ONLY.
 */
#define	__abs_c(a,b)		__CONCAT(a,b)
#define	__abs_opname(op,size)	__abs_c(__abs_c(__abs_c(abs_,op),_),size)

#define	__abs_p(sz, t, h, o)						\
	(*(t)->__abs_opname(p,sz))(t, h, o)
#define	__abs_rs(sz, t, h, o)						\
	(*(t)->__abs_opname(r,sz))(t, h, o)
#define	__abs_rss(sz, t, h, o)						\
	(*(t)->__abs_opname(rs,sz))(t, h, o)
#define	__abs_ws(sz, t, h, o, v)					\
	(*(t)->__abs_opname(w,sz))(t, h, o, v)
#define	__abs_wss(sz, t, h, o, v)					\
	(*(t)->__abs_opname(ws,sz))(t, h, o, v)
#define	__abs_nonsingle(type, sz, t, h, o, a, c)			\
	(*(t)->__abs_opname(type,sz))(t, h, o, a, c)
#define	__abs_set(type, sz, t, h, o, v, c)				\
	(*(t)->__abs_opname(type,sz))(t, h, o, v, c)

    /*
     * No swaps needed and no other trickery, so it should be possible
     * to shortcut these to memcpy() directly [ leo 19990107 ]
     */
#if 0
    #define	__abs_copy(sz, t, h1, o1, h2, o2, cnt)			\
	(*(t)->__abs_opname(c,sz))(t, h1, o1, h2, o2, cnt)
#else
    #define	__abs_copy(sz, t, h1, o1, h2, o2, cnt) do {		\
		    memcpy((void*)(h2 + o2), (void *)(h1 + o1), sz * cnt);  \
		    (void)t;						    \
		} while (0)
#endif


/*
 * Check accesibility of the location for various sized bus accesses
 */
#define bus_space_peek_1(t, h, o)	__abs_p(1,(t),(h),(o))
#define bus_space_peek_2(t, h, o)	__abs_p(2,(t),(h),(o))
#define bus_space_peek_4(t, h, o)	__abs_p(4,(t),(h),(o))
#define bus_space_peek_8(t, h, o)	__abs_p(8,(t),(h),(o))

/*
 * Bus read (single) operations.
 */
#define	bus_space_read_1(t, h, o)	__abs_rs(1,(t),(h),(o))
#define	bus_space_read_2(t, h, o)	__abs_rs(2,(t),(h),(o))
#define	bus_space_read_4(t, h, o)	__abs_rs(4,(t),(h),(o))
#define	bus_space_read_8(t, h, o)	__abs_rs(8,(t),(h),(o))

/*
 * Bus read (single) stream operations.
 */
#define	bus_space_read_stream_1(t, h, o)	__abs_rss(1,(t),(h),(o))
#define	bus_space_read_stream_2(t, h, o)	__abs_rss(2,(t),(h),(o))
#define	bus_space_read_stream_4(t, h, o)	__abs_rss(4,(t),(h),(o))
#define	bus_space_read_stream_8(t, h, o)	__abs_rss(8,(t),(h),(o))

/*
 * Bus read multiple operations.
 */
#define	bus_space_read_multi_1(t, h, o, a, c)				\
	__abs_nonsingle(rm,1,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_2(t, h, o, a, c)				\
	__abs_nonsingle(rm,2,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_4(t, h, o, a, c)				\
	__abs_nonsingle(rm,4,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_8(t, h, o, a, c)				\
	__abs_nonsingle(rm,8,(t),(h),(o),(a),(c))

/*
 * Bus read multiple stream operations.
 */
#define	bus_space_read_multi_stream_1(t, h, o, a, c)			\
	__abs_nonsingle(rms,1,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_stream_2(t, h, o, a, c)			\
	__abs_nonsingle(rms,2,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_stream_4(t, h, o, a, c)			\
	__abs_nonsingle(rms,4,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_stream_8(t, h, o, a, c)			\
	__abs_nonsingle(rms,8,(t),(h),(o),(a),(c))


/*
 * Bus read region operations.
 */
#define	bus_space_read_region_1(t, h, o, a, c)				\
	__abs_nonsingle(rr,1,(t),(h),(o),(a),(c))
#define	bus_space_read_region_2(t, h, o, a, c)				\
	__abs_nonsingle(rr,2,(t),(h),(o),(a),(c))
#define	bus_space_read_region_4(t, h, o, a, c)				\
	__abs_nonsingle(rr,4,(t),(h),(o),(a),(c))
#define	bus_space_read_region_8(t, h, o, a, c)				\
	__abs_nonsingle(rr,8,(t),(h),(o),(a),(c))

/*
 * Bus read region stream operations.
 */
#define	bus_space_read_region_stream_1(t, h, o, a, c)			\
	__abs_nonsingle(rrs,1,(t),(h),(o),(a),(c))
#define	bus_space_read_region_stream_2(t, h, o, a, c)			\
	__abs_nonsingle(rrs,2,(t),(h),(o),(a),(c))
#define	bus_space_read_region_stream_4(t, h, o, a, c)			\
	__abs_nonsingle(rrs,4,(t),(h),(o),(a),(c))
#define	bus_space_read_region_stream_8(t, h, o, a, c)			\
	__abs_nonsingle(rrs,8,(t),(h),(o),(a),(c))

/*
 * Bus write (single) operations.
 */
#define	bus_space_write_1(t, h, o, v)	__abs_ws(1,(t),(h),(o),(v))
#define	bus_space_write_2(t, h, o, v)	__abs_ws(2,(t),(h),(o),(v))
#define	bus_space_write_4(t, h, o, v)	__abs_ws(4,(t),(h),(o),(v))
#define	bus_space_write_8(t, h, o, v)	__abs_ws(8,(t),(h),(o),(v))

/*
 * Bus write (single) stream operations.
 */
#define	bus_space_write_stream_1(t, h, o, v)	__abs_wss(1,(t),(h),(o),(v))
#define	bus_space_write_stream_2(t, h, o, v)	__abs_wss(2,(t),(h),(o),(v))
#define	bus_space_write_stream_4(t, h, o, v)	__abs_wss(4,(t),(h),(o),(v))
#define	bus_space_write_stream_8(t, h, o, v)	__abs_wss(8,(t),(h),(o),(v))


/*
 * Bus write multiple operations.
 */
#define	bus_space_write_multi_1(t, h, o, a, c)				\
	__abs_nonsingle(wm,1,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_2(t, h, o, a, c)				\
	__abs_nonsingle(wm,2,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_4(t, h, o, a, c)				\
	__abs_nonsingle(wm,4,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_8(t, h, o, a, c)				\
	__abs_nonsingle(wm,8,(t),(h),(o),(a),(c))

/*
 * Bus write multiple stream operations.
 */
#define	bus_space_write_multi_stream_1(t, h, o, a, c)			\
	__abs_nonsingle(wms,1,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_stream_2(t, h, o, a, c)			\
	__abs_nonsingle(wms,2,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_stream_4(t, h, o, a, c)			\
	__abs_nonsingle(wms,4,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_stream_8(t, h, o, a, c)			\
	__abs_nonsingle(wms,8,(t),(h),(o),(a),(c))


/*
 * Bus write region operations.
 */
#define	bus_space_write_region_1(t, h, o, a, c)				\
	__abs_nonsingle(wr,1,(t),(h),(o),(a),(c))
#define	bus_space_write_region_2(t, h, o, a, c)				\
	__abs_nonsingle(wr,2,(t),(h),(o),(a),(c))
#define	bus_space_write_region_4(t, h, o, a, c)				\
	__abs_nonsingle(wr,4,(t),(h),(o),(a),(c))
#define	bus_space_write_region_8(t, h, o, a, c)				\
	__abs_nonsingle(wr,8,(t),(h),(o),(a),(c))

/*
 * Bus write region stream operations.
 */
#define	bus_space_write_region_stream_1(t, h, o, a, c)			\
	__abs_nonsingle(wrs,1,(t),(h),(o),(a),(c))
#define	bus_space_write_region_stream_2(t, h, o, a, c)			\
	__abs_nonsingle(wrs,2,(t),(h),(o),(a),(c))
#define	bus_space_write_region_stream_4(t, h, o, a, c)			\
	__abs_nonsingle(wrs,4,(t),(h),(o),(a),(c))
#define	bus_space_write_region_stream_8(t, h, o, a, c)			\
	__abs_nonsingle(wrs,8,(t),(h),(o),(a),(c))


/*
 * Set multiple operations.
 */
#define	bus_space_set_multi_1(t, h, o, v, c)				\
	__abs_set(sm,1,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_2(t, h, o, v, c)				\
	__abs_set(sm,2,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_4(t, h, o, v, c)				\
	__abs_set(sm,4,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_8(t, h, o, v, c)				\
	__abs_set(sm,8,(t),(h),(o),(v),(c))


/*
 * Set region operations.
 */
#define	bus_space_set_region_1(t, h, o, v, c)				\
	__abs_set(sr,1,(t),(h),(o),(v),(c))
#define	bus_space_set_region_2(t, h, o, v, c)				\
	__abs_set(sr,2,(t),(h),(o),(v),(c))
#define	bus_space_set_region_4(t, h, o, v, c)				\
	__abs_set(sr,4,(t),(h),(o),(v),(c))
#define	bus_space_set_region_8(t, h, o, v, c)				\
	__abs_set(sr,8,(t),(h),(o),(v),(c))


/*
 * Copy region operations.
 */
#define	bus_space_copy_region_1(t, h1, o1, h2, o2, c)			\
	__abs_copy(1, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_2(t, h1, o1, h2, o2, c)			\
	__abs_copy(2, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_4(t, h1, o1, h2, o2, c)			\
	__abs_copy(4, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_8(t, h1, o1, h2, o2, c)			\
	__abs_copy(8, (t), (h1), (o1), (h2), (o2), (c))

/*
 *	void *bus_space_vaddr(bus_space_tag_t, bus_space_handle_t);
 *
 * Get the kernel virtual address for the mapped bus space.
 * Only allowed for regions mapped with BUS_SPACE_MAP_LINEAR.
 *  (XXX not enforced)
 */
#define bus_space_vaddr(t, h)	((void)(t), (void *)(h))

/*
 *	paddr_t bus_space_mmap(bus_space_tag_t t, bus_addr_t base,
 *	    off_t offset, int prot, int flags);
 *
 * Mmap an area of bus space.
 */

paddr_t bus_space_mmap(bus_space_tag_t, bus_addr_t, off_t,
	    int, int);

#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

/*
 * Bus read/write barrier methods.
 *
 *	void bus_space_barrier(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags);
 *
 * Note: the Atari does not currently require barriers, but we must
 * provide the flags to MI code.
 */
#define	bus_space_barrier(t, h, o, l, f)	\
	((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f)))

/* Forwards needed by prototypes below. */
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

#define bus_dmatag_subregion(t, mna, mxa, nt, f) EOPNOTSUPP
#define bus_dmatag_destroy(t)

#ifdef _ATARI_BUS_DMA_PRIVATE
int	_bus_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *);
void	_bus_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
int	_bus_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int);
int	_bus_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t, struct mbuf *, int);
int	_bus_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t, struct uio *, int);
int	_bus_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int);
void	_bus_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	_bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int);
#endif /* _ATARI_BUS_DMA_PRIVATE */

int	bus_dmamem_alloc(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags);
int	bus_dmamem_alloc_range(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    paddr_t low, paddr_t high);
void	bus_dmamem_free(bus_dma_tag_t tag, bus_dma_segment_t *segs, int nsegs);
int	bus_dmamem_map(bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, size_t size, void **kvap, int flags);
void	bus_dmamem_unmap(bus_dma_tag_t tag, void *kva, size_t size);
paddr_t	bus_dmamem_mmap(bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, off_t off, int prot, int flags);

#endif /* _ATARI_BUS_FUNCS_H_ */
