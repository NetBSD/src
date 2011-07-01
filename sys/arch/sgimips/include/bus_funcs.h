/*	$NetBSD: bus_funcs.h,v 1.1 2011/07/01 17:10:01 dyoung Exp $	*/

/*
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

#ifndef _SGIMIPS_BUS_FUNCS_H_
#define _SGIMIPS_BUS_FUNCS_H_

/* Initialization for bus_dmamap_sync, which differs from MIPS1 to MIPS3 */

void	sgimips_bus_dma_init(void);

#define bus_space_read_stream_1		bus_space_read_1
#define bus_space_write_stream_1	bus_space_write_1
#define bus_space_read_region_stream_1	bus_space_read_reigion_1
#define bus_space_read_multi_stream_1	bus_space_read_multi_1
#define bus_space_write_multi_stream_1	bus_space_write_multi_1
#define bus_space_write_region_stream_1 bus_space_write_region_1

/*
 * Bus read/write barrier methods.
 *
 *	void bus_space_barrier(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags);
 *
 * On the MIPS, we just flush the write buffer.
 */
#define	bus_space_barrier(t, h, o, l, f)	\
	((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f),	\
	 wbflush()))

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
	(*(t)->_dmamap_sync)((t), (p), (o), (l), (ops))

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

#ifdef _SGIMIPS_BUS_DMA_PRIVATE

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
void	_bus_dmamap_sync_mips1(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int);
void	_bus_dmamap_sync_mips3(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int);

int	_bus_dmamem_alloc(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags);
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
	    vaddr_t low, vaddr_t high);

extern struct sgimips_bus_dma_tag sgimips_default_bus_dma_tag;
#endif /* _SGIMIPS_BUS_DMA_PRIVATE */

#endif /* _SGIMIPS_BUS_FUNCS_H_ */
