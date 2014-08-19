/*	$NetBSD: bus_defs.h,v 1.1.10.2 2014/08/20 00:03:05 tls Exp $	*/

/*	$OpenBSD: bus.h,v 1.13 2001/07/30 14:15:59 art Exp $	*/

/*
 * Copyright (c) 1998-2004 Michael Shalayeff
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _MACHINE_BUS_DEFS_H_
#define _MACHINE_BUS_DEFS_H_

#include <machine/cpufunc.h>

/*
 * Bus address and size types.
 */
typedef u_long bus_addr_t;
typedef u_long bus_size_t;
typedef u_long bus_space_handle_t;

struct hppa_bus_space_tag {
	void *hbt_cookie;

	int  (*hbt_map)(void *v, bus_addr_t addr, bus_size_t size,
			     int flags, bus_space_handle_t *bshp);
	void (*hbt_unmap)(void *v, bus_space_handle_t bsh,
			       bus_size_t size);
	int  (*hbt_subregion)(void *v, bus_space_handle_t bsh,
				   bus_size_t offset, bus_size_t size,
				   bus_space_handle_t *nbshp);
	int  (*hbt_alloc)(void *v, bus_addr_t rstart, bus_addr_t rend,
			       bus_size_t size, bus_size_t align,
			       bus_size_t boundary, int flags,
			       bus_addr_t *addrp, bus_space_handle_t *bshp);
	void (*hbt_free)(void *, bus_space_handle_t, bus_size_t);
	void (*hbt_barrier)(void *v, bus_space_handle_t h,
				 bus_size_t o, bus_size_t l, int op);
	void *(*hbt_vaddr)(void *, bus_space_handle_t);
	paddr_t (*hbt_mmap)(void *, bus_addr_t, off_t, int, int);

	uint8_t  (*hbt_r1)(void *, bus_space_handle_t, bus_size_t);
	uint16_t (*hbt_r2)(void *, bus_space_handle_t, bus_size_t);
	uint32_t (*hbt_r4)(void *, bus_space_handle_t, bus_size_t);
	uint64_t (*hbt_r8)(void *, bus_space_handle_t, bus_size_t);

	void (*hbt_w1)(void *, bus_space_handle_t, bus_size_t, uint8_t);
	void (*hbt_w2)(void *, bus_space_handle_t, bus_size_t, uint16_t);
	void (*hbt_w4)(void *, bus_space_handle_t, bus_size_t, uint32_t);
	void (*hbt_w8)(void *, bus_space_handle_t, bus_size_t, uint64_t);

	void (*hbt_rm_1)(void *v, bus_space_handle_t h,
			      bus_size_t o, uint8_t *a, bus_size_t c);
	void (*hbt_rm_2)(void *v, bus_space_handle_t h,
			      bus_size_t o, uint16_t *a, bus_size_t c);
	void (*hbt_rm_4)(void *v, bus_space_handle_t h,
			      bus_size_t o, uint32_t *a, bus_size_t c);
	void (*hbt_rm_8)(void *v, bus_space_handle_t h,
			      bus_size_t o, uint64_t *a, bus_size_t c);

	void (*hbt_wm_1)(void *v, bus_space_handle_t h, bus_size_t o,
			      const uint8_t *a, bus_size_t c);
	void (*hbt_wm_2)(void *v, bus_space_handle_t h, bus_size_t o,
			      const uint16_t *a, bus_size_t c);
	void (*hbt_wm_4)(void *v, bus_space_handle_t h, bus_size_t o,
			      const uint32_t *a, bus_size_t c);
	void (*hbt_wm_8)(void *v, bus_space_handle_t h, bus_size_t o,
			      const uint64_t *a, bus_size_t c);

	void (*hbt_sm_1)(void *v, bus_space_handle_t h, bus_size_t o,
			      uint8_t  vv, bus_size_t c);
	void (*hbt_sm_2)(void *v, bus_space_handle_t h, bus_size_t o,
			      uint16_t vv, bus_size_t c);
	void (*hbt_sm_4)(void *v, bus_space_handle_t h, bus_size_t o,
			      uint32_t vv, bus_size_t c);
	void (*hbt_sm_8)(void *v, bus_space_handle_t h, bus_size_t o,
			      uint64_t vv, bus_size_t c);

	void (*hbt_rrm_2)(void *v, bus_space_handle_t h,
			       bus_size_t o, uint16_t *a, bus_size_t c);
	void (*hbt_rrm_4)(void *v, bus_space_handle_t h,
			       bus_size_t o, uint32_t *a, bus_size_t c);
	void (*hbt_rrm_8)(void *v, bus_space_handle_t h,
			       bus_size_t o, uint64_t *a, bus_size_t c);

	void (*hbt_wrm_2)(void *v, bus_space_handle_t h,
			       bus_size_t o, const uint16_t *a, bus_size_t c);
	void (*hbt_wrm_4)(void *v, bus_space_handle_t h,
			       bus_size_t o, const uint32_t *a, bus_size_t c);
	void (*hbt_wrm_8)(void *v, bus_space_handle_t h,
			       bus_size_t o, const uint64_t *a, bus_size_t c);

	void (*hbt_rr_1)(void *v, bus_space_handle_t h,
			      bus_size_t o, uint8_t *a, bus_size_t c);
	void (*hbt_rr_2)(void *v, bus_space_handle_t h,
			      bus_size_t o, uint16_t *a, bus_size_t c);
	void (*hbt_rr_4)(void *v, bus_space_handle_t h,
			      bus_size_t o, uint32_t *a, bus_size_t c);
	void (*hbt_rr_8)(void *v, bus_space_handle_t h,
			      bus_size_t o, uint64_t *a, bus_size_t c);

	void (*hbt_wr_1)(void *v, bus_space_handle_t h,
			      bus_size_t o, const uint8_t *a, bus_size_t c);
	void (*hbt_wr_2)(void *v, bus_space_handle_t h,
			      bus_size_t o, const uint16_t *a, bus_size_t c);
	void (*hbt_wr_4)(void *v, bus_space_handle_t h,
			      bus_size_t o, const uint32_t *a, bus_size_t c);
	void (*hbt_wr_8)(void *v, bus_space_handle_t h,
			      bus_size_t o, const uint64_t *a, bus_size_t c);

	void (*hbt_rrr_2)(void *v, bus_space_handle_t h,
			       bus_size_t o, uint16_t *a, bus_size_t c);
	void (*hbt_rrr_4)(void *v, bus_space_handle_t h,
			       bus_size_t o, uint32_t *a, bus_size_t c);
	void (*hbt_rrr_8)(void *v, bus_space_handle_t h,
			       bus_size_t o, uint64_t *a, bus_size_t c);

	void (*hbt_wrr_2)(void *v, bus_space_handle_t h,
			       bus_size_t o, const uint16_t *a, bus_size_t c);
	void (*hbt_wrr_4)(void *v, bus_space_handle_t h,
			       bus_size_t o, const uint32_t *a, bus_size_t c);
	void (*hbt_wrr_8)(void *v, bus_space_handle_t h,
			       bus_size_t o, const uint64_t *a, bus_size_t c);

	void (*hbt_sr_1)(void *v, bus_space_handle_t h,
			       bus_size_t o, uint8_t vv, bus_size_t c);
	void (*hbt_sr_2)(void *v, bus_space_handle_t h,
			       bus_size_t o, uint16_t vv, bus_size_t c);
	void (*hbt_sr_4)(void *v, bus_space_handle_t h,
			       bus_size_t o, uint32_t vv, bus_size_t c);
	void (*hbt_sr_8)(void *v, bus_space_handle_t h,
			       bus_size_t o, uint64_t vv, bus_size_t c);

	void (*hbt_cp_1)(void *v, bus_space_handle_t h1, bus_size_t o1,
			      bus_space_handle_t h2, bus_size_t o2, bus_size_t c);
	void (*hbt_cp_2)(void *v, bus_space_handle_t h1, bus_size_t o1,
			      bus_space_handle_t h2, bus_size_t o2, bus_size_t c);
	void (*hbt_cp_4)(void *v, bus_space_handle_t h1, bus_size_t o1,
			      bus_space_handle_t h2, bus_size_t o2, bus_size_t c);
	void (*hbt_cp_8)(void *v, bus_space_handle_t h1, bus_size_t o1,
			      bus_space_handle_t h2, bus_size_t o2, bus_size_t c);
};
typedef const struct hppa_bus_space_tag *bus_space_tag_t;

/* flags for bus space map functions */
#define BUS_SPACE_MAP_READONLY		0x0008
#define BUS_SPACE_MAP_NOEXTENT		0x8000  /* no extent ops */


/* bus access routines */
#define	BUS_SPACE_ALIGNED_POINTER(p, t)	ALIGNED_POINTER(p, t)

/* XXX fredette */
#define	__BUS_SPACE_HAS_STREAM_METHODS

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

/* For devices that have a 24-bit address space */
#define	BUS_DMA_24BIT		BUS_DMA_BUS1

/* Forwards needed by prototypes below. */
struct mbuf;
struct proc;
struct uio;

typedef const struct hppa_bus_dma_tag	*bus_dma_tag_t;
typedef struct hppa_bus_dmamap	*bus_dmamap_t;

#define BUS_DMA_TAG_VALID(t)    ((t) != (bus_dma_tag_t)0)

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct hppa_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */
	void		*_ds_mlist;	/* page list when dmamem_alloc'ed */
	vaddr_t		_ds_va;		/* VA when dmamem_map'ed */
};
typedef struct hppa_bus_dma_segment	bus_dma_segment_t;

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */

struct hppa_bus_dma_tag {
	void	*_cookie;		/* cookie used in the guts */

	/*
	 * DMA mapping methods.
	 */
	int	(*_dmamap_create)(void *, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *);
	void	(*_dmamap_destroy)(void *, bus_dmamap_t);
	int	(*_dmamap_load)(void *, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int);
	int	(*_dmamap_load_mbuf)(void *, bus_dmamap_t,
		    struct mbuf *, int);
	int	(*_dmamap_load_uio)(void *, bus_dmamap_t,
		    struct uio *, int);
	int	(*_dmamap_load_raw)(void *, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int);
	void	(*_dmamap_unload)(void *, bus_dmamap_t);
	void	(*_dmamap_sync)(void *, bus_dmamap_t, bus_addr_t, bus_size_t, int);

	/*
	 * DMA memory utility functions.
	 */
	int	(*_dmamem_alloc)(void *, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int);
	void	(*_dmamem_free)(void *, bus_dma_segment_t *, int);
	int	(*_dmamem_map)(void *, bus_dma_segment_t *,
		    int, size_t, void **, int);
	void	(*_dmamem_unmap)(void *, void *, size_t);
	paddr_t	(*_dmamem_mmap)(void *, bus_dma_segment_t *,
		    int, off_t, int, int);
};

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct hppa_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use by machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxsegsz;	/* fixed largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
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

#endif /* _MACHINE_BUS_DEFS_H_ */
