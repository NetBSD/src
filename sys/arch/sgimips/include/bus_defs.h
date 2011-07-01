/*	$NetBSD: bus_defs.h,v 1.1 2011/07/01 17:10:01 dyoung Exp $	*/

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

#ifndef _SGIMIPS_BUS_DEFS_H_
#define _SGIMIPS_BUS_DEFS_H_

#include <mips/locore.h>

#define __BUS_SPACE_HAS_STREAM_METHODS

/*
 * Utility macros; do not use outside this file.
 */
#define	__PB_TYPENAME_PREFIX(BITS)	___CONCAT(u_int,BITS)
#define	__PB_TYPENAME(BITS)		___CONCAT(__PB_TYPENAME_PREFIX(BITS),_t)

/*
 * Bus address and size types
 */
typedef uint64_t bus_addr_t;
typedef uint64_t bus_size_t;

/*
 * Access methods for bus resources and address space.
 */
typedef int	bus_space_tag_t;
typedef u_long	bus_space_handle_t;

/*
 * Values for sgimips bus space tag, not to be used directly by MI code.
 */
#define	SGIMIPS_BUS_SPACE_NORMAL	0
#define	SGIMIPS_BUS_SPACE_IP6_DPCLOCK	1
#define	SGIMIPS_BUS_SPACE_HPC		2
#define	SGIMIPS_BUS_SPACE_MEM		3
#define	SGIMIPS_BUS_SPACE_MACE		4
#define	SGIMIPS_BUS_SPACE_IO		5
#define	SGIMIPS_BUS_SPACE_CRIME		6

/*
 *	int bus_space_map(bus_space_tag_t t, bus_addr_t addr,
 *	    bus_size_t size, int flags, bus_space_handle_t *bshp);
 *
 * Map a region of bus space.
 */

#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02
#define BUS_SPACE_MAP_PREFETCHABLE	0x04

#define	BUS_SPACE_BARRIER_READ	0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE	0x02		/* force write barrier */

#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

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
#define	BUS_DMA_NOCACHE		0x400	/* hint: map non-cached memory */

#define	SGIMIPS_DMAMAP_COHERENT	0x10000	/* no cache flush necessary on sync */

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

typedef struct sgimips_bus_dma_tag		*bus_dma_tag_t;
typedef struct sgimips_bus_dmamap		*bus_dmamap_t;

#define BUS_DMA_TAG_VALID(t)    ((t) != (bus_dma_tag_t)0)

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct sgimips_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */
	vaddr_t		_ds_vaddr;	/* virtual address, 0 if invalid */
};
typedef struct sgimips_bus_dma_segment	bus_dma_segment_t;

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */

struct sgimips_bus_dma_tag {
	/*
	 * XXX soren O2: 0x40000000
	 * ikke endnu (bus.c)
	 */
#if 0
	bus_addr_t _wbase;		/* DMA window base */
	bus_addr_t _wbase_swap;		/* DMA window base (byteswapped) */
#endif

	/*
	 * DMA mapping methods.
	 */
	int	(*_dmamap_create)(bus_dma_tag_t, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *);
	void	(*_dmamap_destroy)(bus_dma_tag_t, bus_dmamap_t);
	int	(*_dmamap_load)(bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int);
	int	(*_dmamap_load_mbuf)(bus_dma_tag_t, bus_dmamap_t,
		    struct mbuf *, int);
	int	(*_dmamap_load_uio)(bus_dma_tag_t, bus_dmamap_t,
		    struct uio *, int);
	int	(*_dmamap_load_raw)(bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int);
	void	(*_dmamap_unload)(bus_dma_tag_t, bus_dmamap_t);
	void	(*_dmamap_sync)(bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int);

	/*
	 * DMA memory utility functions.
	 */
	int	(*_dmamem_alloc)(bus_dma_tag_t, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int);
	void	(*_dmamem_free)(bus_dma_tag_t,
		    bus_dma_segment_t *, int);
	int	(*_dmamem_map)(bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, void **, int);
	void	(*_dmamem_unmap)(bus_dma_tag_t, void *, size_t);
	paddr_t	(*_dmamem_mmap)(bus_dma_tag_t, bus_dma_segment_t *,
		    int, off_t, int, int);
};

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct sgimips_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use my machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxmaxsegsz; /* fixed largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	int		_dm_flags;	/* misc. flags */
	struct vmspace	*_dm_vmspace;	/* vmspace that owns the mapping */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_maxsegsz;	/* largest possible segment */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

#endif /* _SGIMIPS_BUS_DEFS_H_ */
