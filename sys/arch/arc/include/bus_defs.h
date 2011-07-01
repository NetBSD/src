/*	$NetBSD: bus_defs.h,v 1.1 2011/07/01 17:09:58 dyoung Exp $	*/
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

#ifndef _ARC_BUS_DEFS_H_
#define _ARC_BUS_DEFS_H_
#ifdef _KERNEL

/*
 * Bus address and size types
 */
typedef u_long bus_addr_t;
typedef u_long bus_size_t;

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
 * Access methods for bus resources and address space.
 */
typedef uint32_t bus_space_handle_t;
typedef struct arc_bus_space *bus_space_tag_t;

struct arc_bus_space {
	const char	*bs_name;
	struct extent	*bs_extent;
	bus_addr_t	bs_start;
	bus_size_t	bs_size;

	paddr_t		bs_pbase;
	vaddr_t		bs_vbase;

	/* sparse addressing shift count */
	uint8_t		bs_stride_1;
	uint8_t		bs_stride_2;
	uint8_t 	bs_stride_4;
	uint8_t		bs_stride_8;

	/* compose a bus_space handle from tag/handle/addr/size/flags (MD) */
	int	(*bs_compose_handle)(bus_space_tag_t, bus_addr_t,
				bus_size_t, int, bus_space_handle_t *);

	/* dispose a bus_space handle (MD) */
	int	(*bs_dispose_handle)(bus_space_tag_t, bus_space_handle_t,
				bus_size_t);

	/* convert bus_space tag/handle to physical address (MD) */
	int	(*bs_paddr)(bus_space_tag_t, bus_space_handle_t,
				paddr_t *);

	/* mapping/unmapping */
	int	(*bs_map)(bus_space_tag_t, bus_addr_t, bus_size_t, int,
				bus_space_handle_t *);
	void	(*bs_unmap)(bus_space_tag_t, bus_space_handle_t, bus_size_t);
	int	(*bs_subregion)(bus_space_tag_t, bus_space_handle_t,
				bus_size_t, bus_size_t,	bus_space_handle_t *);
	paddr_t	(*bs_mmap)(bus_space_tag_t, bus_addr_t, off_t, int, int);

	/* allocation/deallocation */
	int	(*bs_alloc)(bus_space_tag_t, bus_addr_t, bus_addr_t,
				bus_size_t, bus_size_t,	bus_size_t, int,
				bus_addr_t *, bus_space_handle_t *);
	void	(*bs_free)(bus_space_tag_t, bus_space_handle_t, bus_size_t);

	void	*bs_aux;
};

/* vaddr_t argument of arc_bus_space_init() */
#define ARC_BUS_SPACE_UNMAPPED	((vaddr_t)0)

#define BUS_SPACE_MAP_CACHEABLE		0x01
#define BUS_SPACE_MAP_LINEAR		0x02
#define BUS_SPACE_MAP_PREFETCHABLE	0x04

#define __BUS_SPACE_HAS_STREAM_METHODS

#define BUS_SPACE_BARRIER_READ	0x01
#define BUS_SPACE_BARRIER_WRITE	0x02

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

#define ARC_DMAMAP_COHERENT	0x10000	/* no cache flush necessary on sync */

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

#define BUS_DMA_TAG_VALID(t)    ((t) != (bus_dma_tag_t)0)

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
struct arc_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use by machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxmaxsegsz; /* fixed largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	int		_dm_flags;	/* misc. flags */
	struct vmspace	*_dm_vmspace;	/* vmspace that owns the mapping */

	/*
	 * Private cookie to be used by the DMA back-end.
	 */
	void		*_dm_cookie;

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_maxsegsz;	/* largest possible segment */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

#endif /* _KERNEL */
#endif /* _ARC_BUS_DEFS_H_ */
