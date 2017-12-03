/*	$NetBSD: bus_defs.h,v 1.2.12.1 2017/12/03 11:36:45 jdolecek Exp $	*/

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
 * Copyright (c) 1997-1999, 2001 Eduardo E. Horvath. All rights reserved.
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

#ifndef _SPARC64_BUS_DEFS_H_
#define _SPARC64_BUS_DEFS_H_

#include <machine/types.h>
#include <machine/ctlreg.h>

/*
 * Debug hooks
 */

#define	BSDB_ACCESS	0x01
#define BSDB_MAP	0x02

/*
 * UPA and SBUS spaces are non-cached and big endian
 * (except for RAM and PROM)
 *
 * PCI spaces are non-cached and little endian
 */

enum bus_type { 
	UPA_BUS_SPACE,
	SBUS_BUS_SPACE,
	PCI_CONFIG_BUS_SPACE,
	PCI_IO_BUS_SPACE,
	PCI_MEMORY_BUS_SPACE,
	LAST_BUS_SPACE
};
/* For backwards compatibility */
#define SPARC_BUS_SPACE	UPA_BUS_SPACE

#define __BUS_SPACE_HAS_STREAM_METHODS	1

/*
 * Bus address and size types
 */
typedef	struct _bus_space_handle {
	uint64_t _ptr;
	uint8_t	 _asi;
	uint8_t	 _sasi;
} bus_space_handle_t;
typedef enum bus_type	bus_type_t;
typedef uint64_t	bus_addr_t;
typedef uint64_t	bus_size_t;

/*
 * XXXX -- convert prom virtual address to bus_space_handle_t
 */
#define	sparc_promaddr_to_handle(tag, promaddr, hp)	\
	do {						\
		(hp)->_ptr = (uint64_t)(promaddr);	\
		(hp)->_asi = ASI_PRIMARY;		\
		(hp)->_sasi = ASI_PRIMARY;		\
	} while (0)

/* For buses which have an iospace. */
#define	BUS_ADDR_IOSPACE(x)	((x)>>32)
#define	BUS_ADDR_PADDR(x)	((x)&0xffffffff)
#define	BUS_ADDR(io, pa)	((((bus_addr_t)io)<<32)|(pa))

/*
 * Access methods for bus resources and address space.
 */
typedef struct sparc_bus_space_tag	*bus_space_tag_t;

struct sparc_bus_space_tag {
	void		*cookie;
	bus_space_tag_t	parent;
	/*
	 * Windows onto the parent bus that this tag maps.  If ranges
	 * is non-NULL, the address will be translated, and recursively
	 * mapped via the parent tag.
	 */
	struct openprom_range *ranges;
	int nranges;

	int		type;

	int	(*sparc_bus_alloc)(bus_space_tag_t, bus_addr_t, 
		bus_addr_t, bus_size_t, bus_size_t, bus_size_t, int, 
		bus_addr_t *, bus_space_handle_t *);

	void	(*sparc_bus_free)(bus_space_tag_t, bus_space_handle_t, 
		bus_size_t);

	int	(*sparc_bus_map)(bus_space_tag_t, bus_addr_t, bus_size_t,
		int, vaddr_t, bus_space_handle_t *);

	int	(*sparc_bus_unmap)(bus_space_tag_t, bus_space_handle_t, 
		bus_size_t);

	int	(*sparc_bus_subregion)(bus_space_tag_t,
		bus_space_handle_t, bus_size_t,	bus_size_t, 
		bus_space_handle_t *);

	paddr_t	(*sparc_bus_mmap)(bus_space_tag_t, bus_addr_t, off_t, 
		int, int);

	void	*(*sparc_intr_establish)(bus_space_tag_t, int, int,
		int (*)(void *), void *, void (*)(void));

};

/* flags for bus space map functions */
#define	BUS_SPACE_MAP_READONLY		0x0008
#define	BUS_SPACE_MAP_BUS1		0x0100	
#define	BUS_SPACE_MAP_BUS2		0x0200
#define	BUS_SPACE_MAP_BUS3		0x0400
#define	BUS_SPACE_MAP_BUS4		0x0800
#define BUS_SPACE_MAP_LITTLE		0x1000
/* sparc uses this, it's not supposed to do anything on sparc64 */
#define BUS_SPACE_MAP_LARGE		0

/* flags for bus_space_barrier() */
#define	BUS_SPACE_BARRIER_READ	0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE	0x02		/* force write barrier */

#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

/*
 * Flags used in various bus DMA methods.
 */
#define	BUS_DMA_NOWRITE		BUS_DMA_BUS1
#define	BUS_DMA_DVMA		BUS_DMA_BUS2	/* Don't bother with alignment */

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

typedef struct sparc_bus_dma_tag	*bus_dma_tag_t;
typedef struct sparc_bus_dmamap		*bus_dmamap_t;

#define BUS_DMA_TAG_VALID(t)	((t) != (bus_dma_tag_t)0)

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct sparc_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DVMA address */
	bus_size_t	ds_len;		/* length of transfer */
	bus_size_t	_ds_boundary;	/* don't cross this */
	bus_size_t	_ds_align;	/* align to this */
	void		*_ds_mlist;	/* XXX - dmamap_alloc'ed pages */
};
typedef struct sparc_bus_dma_segment	bus_dma_segment_t;


/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */
struct sparc_bus_dma_tag {
	void	*_cookie;		/* cookie used in the guts */
	struct sparc_bus_dma_tag* _parent;

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
	void	(*_dmamem_free)(bus_dma_tag_t, bus_dma_segment_t *, int);
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
struct sparc_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use by machine-independent code.
	 */
	bus_addr_t	_dm_dvmastart;	/* start and size of allocated */
	bus_size_t	_dm_dvmasize;	/* DVMA segment for this map. */

	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	bus_size_t	_dm_maxmaxsegsz; /* fixed largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	int		_dm_segcnt;	/* number of segs this map can map */
	int		_dm_flags;	/* misc. flags */
#define _DM_TYPE_LOAD	0
#define _DM_TYPE_SEGS	1
#define _DM_TYPE_UIO	2
#define _DM_TYPE_MBUF	3
	int		_dm_type;	/* type of mapping: raw, uio, mbuf, etc */
	void		*_dm_source;	/* source mbuf, uio, etc. needed for unload */

	void		*_dm_cookie;	/* cookie for bus-specific functions */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_maxsegsz;	/* largest possible segment */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

#endif /* _SPARC64_BUS_DEFS_H_ */
