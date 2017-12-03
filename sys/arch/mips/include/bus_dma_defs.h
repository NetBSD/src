/* $NetBSD: bus_dma_defs.h,v 1.1.12.1 2017/12/03 11:36:27 jdolecek Exp $ */

/*-
 * Copyright (c) 1997, 1998, 2000, 2001 The NetBSD Foundation, Inc.
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

#ifndef _MIPS_BUS_DMA_DEFS_H_
#define	_MIPS_BUS_DMA_DEFS_H_

#include <sys/types.h>

#ifdef _KERNEL
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

/*
 * Private flags stored in the DMA map.
 */
#define	_BUS_DMAMAP_COHERENT	0x10000	/* no cache flush necessary on sync */

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

typedef struct mips_bus_dma_tag	*bus_dma_tag_t;
typedef struct mips_bus_dmamap	*bus_dmamap_t;

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct mips_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */
	register_t	_ds_vaddr;	/* virtual address, 0 if invalid */
};
typedef struct mips_bus_dma_segment	bus_dma_segment_t;

/*
 * DMA mapping methods.
 */
struct mips_bus_dmamap_ops {
	int	(*dmamap_create)(bus_dma_tag_t, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *);
	void	(*dmamap_destroy)(bus_dma_tag_t, bus_dmamap_t);
	int	(*dmamap_load)(bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int);
	int	(*dmamap_load_mbuf)(bus_dma_tag_t, bus_dmamap_t,
		    struct mbuf *, int);
	int	(*dmamap_load_uio)(bus_dma_tag_t, bus_dmamap_t,
		    struct uio *, int);
	int	(*dmamap_load_raw)(bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int);
	void	(*dmamap_unload)(bus_dma_tag_t, bus_dmamap_t);
	void	(*dmamap_sync)(bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int);
};

/*
 * DMA memory utility functions.
 */
struct mips_bus_dmamem_ops {
	int	(*dmamem_alloc)(bus_dma_tag_t, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int);
	void	(*dmamem_free)(bus_dma_tag_t,
		    bus_dma_segment_t *, int);
	int	(*dmamem_map)(bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, void **, int);
	void	(*dmamem_unmap)(bus_dma_tag_t, void *, size_t);
	paddr_t	(*dmamem_mmap)(bus_dma_tag_t, bus_dma_segment_t *,
		    int, off_t, int, int);
};

/*
 * DMA tag utility functions.
 */
struct mips_bus_dmatag_ops {
	int	(*dmatag_subregion)(bus_dma_tag_t, bus_addr_t, bus_addr_t,
		    bus_dma_tag_t *, int);
	void	(*dmatag_destroy)(bus_dma_tag_t);
};

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */
struct mips_bus_dma_tag {
	void	*_cookie;		/* cookie used in the guts */

	bus_addr_t _wbase;		/* DMA window base */
	int _tag_needs_free;		/* number of references (maybe 0) */
	bus_addr_t _bounce_thresh;
	bus_addr_t _bounce_alloc_lo;	/* physical base of the window */
	bus_addr_t _bounce_alloc_hi;	/* physical limit of the windows */
	int	(*_may_bounce)(bus_dma_tag_t, bus_dmamap_t, int, int *);

	struct mips_bus_dmamap_ops _dmamap_ops;
	struct mips_bus_dmamem_ops _dmamem_ops;
	struct mips_bus_dmatag_ops _dmatag_ops;
};

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct mips_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use my machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxmaxsegsz; /* fixed largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	bus_addr_t	_dm_bounce_thresh; /* bounce threshold; see tag */
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

#ifdef _MIPS_BUS_DMA_PRIVATE
#define	_BUS_AVAIL_END	pmap_limits.avail_end
/*
 * Cookie used for bounce buffers. A pointer to one of these it stashed in
 * the DMA map.
 */
struct mips_bus_dma_cookie {
	int	id_flags;		/* flags; see below */

	/*
	 * Information about the original buffer used during
	 * DMA map syncs.  Note that origibuflen is only used
	 * for ID_BUFTYPE_LINEAR.
	 */
	union {
		void	*un_origbuf;		/* pointer to orig buffer if
						   bouncing */
		char	*un_linearbuf;
		struct mbuf	*un_mbuf;
		struct uio	*un_uio;
	} id_origbuf_un;
#define	id_origbuf		id_origbuf_un.un_origbuf
#define	id_origlinearbuf	id_origbuf_un.un_linearbuf
#define	id_origmbuf		id_origbuf_un.un_mbuf
#define	id_origuio		id_origbuf_un.un_uio
	bus_size_t id_origbuflen;	/* ...and size */
	int	id_buftype;		/* type of buffer */

	void	*id_bouncebuf;		/* pointer to the bounce buffer */
	bus_size_t id_bouncebuflen;	/* ...and size */
	int	id_nbouncesegs;		/* number of valid bounce segs */
	bus_dma_segment_t id_bouncesegs[0]; /* array of bounce buffer
					       physical memory segments */
};

/* id_flags */
#endif /* _MIPS_BUS_DMA_PRIVATE */
#define	_BUS_DMA_MIGHT_NEED_BOUNCE	0x01	/* may need bounce buffers */
#ifdef _MIPS_BUS_DMA_PRIVATE
#define	_BUS_DMA_HAS_BOUNCE		0x02	/* has bounce buffers */
#define	_BUS_DMA_IS_BOUNCING		0x04	/* is bouncing current xfer */

/* id_buftype */
#define	_BUS_DMA_BUFTYPE_INVALID	0
#define	_BUS_DMA_BUFTYPE_LINEAR		1
#define	_BUS_DMA_BUFTYPE_MBUF		2
#define	_BUS_DMA_BUFTYPE_UIO		3
#define	_BUS_DMA_BUFTYPE_RAW		4

extern const struct mips_bus_dmamap_ops mips_bus_dmamap_ops;
extern const struct mips_bus_dmamem_ops mips_bus_dmamem_ops;
extern const struct mips_bus_dmatag_ops mips_bus_dmatag_ops;

#define	_BUS_DMAMAP_OPS_INITIALIZER {					\
		.dmamap_create		= _bus_dmamap_create,		\
		.dmamap_destroy		= _bus_dmamap_destroy,		\
		.dmamap_load		= _bus_dmamap_load,		\
		.dmamap_load_mbuf	= _bus_dmamap_load_mbuf,	\
		.dmamap_load_uio	= _bus_dmamap_load_uio,		\
		.dmamap_load_raw	= _bus_dmamap_load_raw,		\
		.dmamap_unload		= _bus_dmamap_unload,		\
		.dmamap_sync		= _bus_dmamap_sync,		\
	}

#define _BUS_DMAMEM_OPS_INITIALIZER {					\
		.dmamem_alloc = 	_bus_dmamem_alloc,		\
		.dmamem_free =		_bus_dmamem_free,		\
		.dmamem_map =		_bus_dmamem_map,		\
		.dmamem_unmap =		_bus_dmamem_unmap,		\
		.dmamem_mmap =		_bus_dmamem_mmap,		\
	}

#define _BUS_DMATAG_OPS_INITIALIZER {					\
		.dmatag_subregion =	_bus_dmatag_subregion,		\
		.dmatag_destroy =	_bus_dmatag_destroy,		\
	}
#endif /* _MIPS_BUS_DMA_PRIVATE */

#endif /* _KERNEL */

#endif /* _MIPS_BUS_DMA_DEFS_H_ */
