/* $NetBSD: bus_defs.h,v 1.1.18.2 2017/12/03 11:36:34 jdolecek Exp $ */
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

#ifndef _OR1K_BUS_DEFS_H_
#define _OR1K_BUS_DEFS_H_

/*
 * Bus access types.
 */
typedef uintptr_t bus_addr_t;
typedef uintptr_t bus_size_t;

typedef	uintptr_t bus_space_handle_t;
typedef	const struct or1k_bus_space *bus_space_tag_t;

struct extent;

struct or1k_bus_space_scalar {
	uint8_t (*pbss_read_1)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t);
	uint16_t (*pbss_read_2)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t);
	uint32_t (*pbss_read_4)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t);
	uint64_t (*pbss_read_8)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t);

	void (*pbss_write_1)(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    uint8_t);
	void (*pbss_write_2)(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    uint16_t);
	void (*pbss_write_4)(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    uint32_t);
	void (*pbss_write_8)(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    uint64_t);
};

struct or1k_bus_space_group {
	void (*pbsg_read_1)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, uint8_t *, size_t);
	void (*pbsg_read_2)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, uint16_t *, size_t);
	void (*pbsg_read_4)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, uint32_t *, size_t);
	void (*pbsg_read_8)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, uint64_t *, size_t);

	void (*pbsg_write_1)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, const uint8_t *, size_t);
	void (*pbsg_write_2)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, const uint16_t *, size_t);
	void (*pbsg_write_4)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, const uint32_t *, size_t);
	void (*pbsg_write_8)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, const uint64_t *, size_t);
};

struct or1k_bus_space_set {
	void (*pbss_set_1)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, uint8_t, size_t);
	void (*pbss_set_2)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, uint16_t, size_t);
	void (*pbss_set_4)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, uint32_t, size_t);
	void (*pbss_set_8)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, uint64_t, size_t);
};

struct or1k_bus_space_copy {
	void (*pbsc_copy_1)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, bus_space_handle_t, bus_size_t, size_t);
	void (*pbsc_copy_2)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, bus_space_handle_t, bus_size_t, size_t);
	void (*pbsc_copy_4)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, bus_space_handle_t, bus_size_t, size_t);
	void (*pbsc_copy_8)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, bus_space_handle_t, bus_size_t, size_t);
};

struct or1k_bus_space {
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

	struct or1k_bus_space_scalar pbs_scalar;
	struct or1k_bus_space_scalar pbs_scalar_stream;
	const struct or1k_bus_space_group *pbs_multi;
	const struct or1k_bus_space_group *pbs_multi_stream;
	const struct or1k_bus_space_group *pbs_region;
	const struct or1k_bus_space_group *pbs_region_stream;
	const struct or1k_bus_space_set *pbs_set;
	const struct or1k_bus_space_set *pbs_set_stream;
	const struct or1k_bus_space_copy *pbs_copy;
};

#define _BUS_SPACE_STRIDE(t, o) \
	((o) << ((t)->pbs_flags & _BUS_SPACE_STRIDE_MASK)) 
#define _BUS_SPACE_UNSTRIDE(t, o) \
	((o) >> ((t)->pbs_flags & _BUS_SPACE_STRIDE_MASK)) 

#define BUS_SPACE_MAP_CACHEABLE         0x01
#define BUS_SPACE_MAP_LINEAR            0x02
#define	BUS_SPACE_MAP_PREFETCHABLE	0x04

int bus_space_init(struct or1k_bus_space *, const char *, void *, size_t);
void bus_space_mallocok(void);

/*
 * Access methods for bus resources
 */

#define	__BUS_SPACE_HAS_STREAM_METHODS

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
#define	BUS_DMA_COHERENT	0x004	/* hint: map memory DMA coherent */
#define	BUS_DMA_STREAMING	0x008	/* hint: sequential, unidirectional */
#define	BUS_DMA_BUS1		0x010	/* placeholders for bus functions... */
#define	BUS_DMA_BUS2		0x020
#define	BUS_DMA_BUS3		0x040
#define	BUS_DMA_BUS4		0x080
#define	BUS_DMA_READ		0x100	/* mapping is device -> memory only */
#define	BUS_DMA_WRITE		0x200	/* mapping is memory -> device only */
#define	BUS_DMA_NOCACHE		0x400	/* hint: map non-cached memory */

#ifndef BUS_DMA_DONTCACHE
#define	BUS_DMA_DONTCACHE	BUS_DMA_NOCACHE
#endif

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

typedef struct or1k_bus_dma_tag		*bus_dma_tag_t;
typedef struct or1k_bus_dmamap		*bus_dmamap_t;

#define BUS_DMA_TAG_VALID(t)    ((t) != (bus_dma_tag_t)0)

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct or1k_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */
};
typedef struct or1k_bus_dma_segment	bus_dma_segment_t;

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */

struct or1k_bus_dma_tag {
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
		    int, size_t, void **, int);
	void	(*_dmamem_unmap) (bus_dma_tag_t, void *, size_t);
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

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct or1k_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use my machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxmaxsegsz; /* fixed largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	bus_addr_t	_dm_bounce_thresh; /* bounce threshold; see tag */
	int		_dm_flags;	/* misc. flags */

	void		*_dm_cookie;	/* cookie for bus-specific functions */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_maxsegsz;	/* largest possible segment */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

#endif /* _OR1K_BUS_DEFS_H_ */
