/*	$NetBSD: bus_dma_hpcmips.h,v 1.2.2.4 2002/02/28 04:09:58 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 TAKEMRUA Shin. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _BUS_DMA_HPCMIPS_H_
#define _BUS_DMA_HPCMIPS_H_

#define	HPCMIPS_DMAMAP_COHERENT	0x100	/* no cache flush necessary on sync */

/*
 *	bus_dma_segment
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct bus_dma_segment_hpcmips {
	bus_addr_t	_ds_vaddr;	/* virtual address, 0 if invalid */
};

/*
 *	bus_dma_tag
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */
struct bus_dma_tag_hpcmips {
	struct bus_dma_tag	bdt;
	void	*_dmamap_chipset_v;
};

/*
 *	bus_dmamap
 *
 *	Describes a DMA mapping.
 */
struct bus_dmamap_hpcmips {
	struct bus_dmamap bdm;
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxsegsz;	/* largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	int		_dm_flags;	/* misc. flags */
	struct bus_dma_segment_hpcmips _dm_segs[1];
};

extern struct bus_dma_tag_hpcmips hpcmips_default_bus_dma_tag;
bus_dma_protos(_hpcmips)

int	_hpcmips_bd_mem_alloc_range __P((bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    vaddr_t low, vaddr_t high));

#endif /* _BUS_DMA_HPCMIPS_H_ */
