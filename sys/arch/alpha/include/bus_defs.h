/* $NetBSD: bus_defs.h,v 1.2.2.1 2012/04/17 00:05:55 yamt Exp $ */

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

#ifndef _ALPHA_BUS_DEFS_H_
#define	_ALPHA_BUS_DEFS_H_

#include <sys/types.h>
#include <machine/bus_user.h>

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <stdbool.h>
#endif
#include <sys/stdint.h>

#ifdef _KERNEL

/*
 * Turn on BUS_SPACE_DEBUG if the global DEBUG option is enabled.
 */
#if defined(DEBUG) && !defined(BUS_SPACE_DEBUG)
#define	BUS_SPACE_DEBUG
#endif

#ifdef BUS_SPACE_DEBUG
#include <sys/systm.h> /* for printf() prototype */
/*
 * Macros for checking the aligned-ness of pointers passed to bus
 * space ops.  Strict alignment is required by the Alpha architecture,
 * and a trap will occur if unaligned access is performed.  These
 * may aid in the debugging of a broken device driver by displaying
 * useful information about the problem.
 */
#define	__BUS_SPACE_ALIGNED_ADDRESS(p, t)				\
	((((u_long)(p)) & (sizeof(t)-1)) == 0)

#define	__BUS_SPACE_ADDRESS_SANITY(p, t, d)				\
({									\
	if (__BUS_SPACE_ALIGNED_ADDRESS((p), t) == 0) {			\
		printf("%s 0x%lx not aligned to %lu bytes %s:%d\n",	\
		    d, (u_long)(p), sizeof(t), __FILE__, __LINE__);	\
	}								\
	(void) 0;							\
})

#define BUS_SPACE_ALIGNED_POINTER(p, t) __BUS_SPACE_ALIGNED_ADDRESS(p, t)
#else
#define	__BUS_SPACE_ADDRESS_SANITY(p, t, d)	(void) 0
#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)
#endif /* BUS_SPACE_DEBUG */
#endif /* _KERNEL */

struct alpha_bus_space_translation;

/*
 * Access methods for bus space.
 */
typedef struct alpha_bus_space *bus_space_tag_t;
typedef u_long bus_space_handle_t;

struct alpha_bus_space {
	/* cookie */
	void		*abs_cookie;

	/* mapping/unmapping */
	int		(*abs_map)(void *, bus_addr_t, bus_size_t,
			    int, bus_space_handle_t *, int);
	void		(*abs_unmap)(void *, bus_space_handle_t,
			    bus_size_t, int);
	int		(*abs_subregion)(void *, bus_space_handle_t,
			    bus_size_t, bus_size_t, bus_space_handle_t *);

	/* ALPHA SPECIFIC MAPPING METHOD */
	int		(*abs_translate)(void *, bus_addr_t, bus_size_t,
			    int, struct alpha_bus_space_translation *);
	int		(*abs_get_window)(void *, int,
			    struct alpha_bus_space_translation *);

	/* allocation/deallocation */
	int		(*abs_alloc)(void *, bus_addr_t, bus_addr_t,
			    bus_size_t, bus_size_t, bus_size_t, int,
			    bus_addr_t *, bus_space_handle_t *);
	void		(*abs_free)(void *, bus_space_handle_t,
			    bus_size_t);

	/* get kernel virtual address */
	void *		(*abs_vaddr)(void *, bus_space_handle_t);

	/* mmap bus space for user */
	paddr_t		(*abs_mmap)(void *, bus_addr_t, off_t, int, int);

	/* barrier */
	void		(*abs_barrier)(void *, bus_space_handle_t,
			    bus_size_t, bus_size_t, int);

	/* read (single) */
	uint8_t	(*abs_r_1)(void *, bus_space_handle_t,
			    bus_size_t);
	uint16_t	(*abs_r_2)(void *, bus_space_handle_t,
			    bus_size_t);
	uint32_t	(*abs_r_4)(void *, bus_space_handle_t,
			    bus_size_t);
	uint64_t	(*abs_r_8)(void *, bus_space_handle_t,
			    bus_size_t);

	/* read multiple */
	void		(*abs_rm_1)(void *, bus_space_handle_t,
			    bus_size_t, uint8_t *, bus_size_t);
	void		(*abs_rm_2)(void *, bus_space_handle_t,
			    bus_size_t, uint16_t *, bus_size_t);
	void		(*abs_rm_4)(void *, bus_space_handle_t,
			    bus_size_t, uint32_t *, bus_size_t);
	void		(*abs_rm_8)(void *, bus_space_handle_t,
			    bus_size_t, uint64_t *, bus_size_t);
					
	/* read region */
	void		(*abs_rr_1)(void *, bus_space_handle_t,
			    bus_size_t, uint8_t *, bus_size_t);
	void		(*abs_rr_2)(void *, bus_space_handle_t,
			    bus_size_t, uint16_t *, bus_size_t);
	void		(*abs_rr_4)(void *, bus_space_handle_t,
			    bus_size_t, uint32_t *, bus_size_t);
	void		(*abs_rr_8)(void *, bus_space_handle_t,
			    bus_size_t, uint64_t *, bus_size_t);
					
	/* write (single) */
	void		(*abs_w_1)(void *, bus_space_handle_t,
			    bus_size_t, uint8_t);
	void		(*abs_w_2)(void *, bus_space_handle_t,
			    bus_size_t, uint16_t);
	void		(*abs_w_4)(void *, bus_space_handle_t,
			    bus_size_t, uint32_t);
	void		(*abs_w_8)(void *, bus_space_handle_t,
			    bus_size_t, uint64_t);

	/* write multiple */
	void		(*abs_wm_1)(void *, bus_space_handle_t,
			    bus_size_t, const uint8_t *, bus_size_t);
	void		(*abs_wm_2)(void *, bus_space_handle_t,
			    bus_size_t, const uint16_t *, bus_size_t);
	void		(*abs_wm_4)(void *, bus_space_handle_t,
			    bus_size_t, const uint32_t *, bus_size_t);
	void		(*abs_wm_8)(void *, bus_space_handle_t,
			    bus_size_t, const uint64_t *, bus_size_t);
					
	/* write region */
	void		(*abs_wr_1)(void *, bus_space_handle_t,
			    bus_size_t, const uint8_t *, bus_size_t);
	void		(*abs_wr_2)(void *, bus_space_handle_t,
			    bus_size_t, const uint16_t *, bus_size_t);
	void		(*abs_wr_4)(void *, bus_space_handle_t,
			    bus_size_t, const uint32_t *, bus_size_t);
	void		(*abs_wr_8)(void *, bus_space_handle_t,
			    bus_size_t, const uint64_t *, bus_size_t);

	/* set multiple */
	void		(*abs_sm_1)(void *, bus_space_handle_t,
			    bus_size_t, uint8_t, bus_size_t);
	void		(*abs_sm_2)(void *, bus_space_handle_t,
			    bus_size_t, uint16_t, bus_size_t);
	void		(*abs_sm_4)(void *, bus_space_handle_t,
			    bus_size_t, uint32_t, bus_size_t);
	void		(*abs_sm_8)(void *, bus_space_handle_t,
			    bus_size_t, uint64_t, bus_size_t);

	/* set region */
	void		(*abs_sr_1)(void *, bus_space_handle_t,
			    bus_size_t, uint8_t, bus_size_t);
	void		(*abs_sr_2)(void *, bus_space_handle_t,
			    bus_size_t, uint16_t, bus_size_t);
	void		(*abs_sr_4)(void *, bus_space_handle_t,
			    bus_size_t, uint32_t, bus_size_t);
	void		(*abs_sr_8)(void *, bus_space_handle_t,
			    bus_size_t, uint64_t, bus_size_t);

	/* copy */
	void		(*abs_c_1)(void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t);
	void		(*abs_c_2)(void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t);
	void		(*abs_c_4)(void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t);
	void		(*abs_c_8)(void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t);
};

#define	BUS_SPACE_MAP_CACHEABLE		0x01

#ifdef _KERNEL

#define	BUS_SPACE_BARRIER_READ	0x01
#define	BUS_SPACE_BARRIER_WRITE	0x02
/*
 * Bus stream operations--defined in terms of non-stream counterparts
 */
#define __BUS_SPACE_HAS_STREAM_METHODS 1

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
#define	DMAMAP_NO_COALESCE	0x40000000	/* don't coalesce adjacent
						   segments */

/* Forwards needed by prototypes below. */
struct mbuf;
struct uio;
struct alpha_sgmap;

/*
 * Operations performed by bus_dmamap_sync().
 */
#define	BUS_DMASYNC_PREREAD	0x01	/* pre-read synchronization */
#define	BUS_DMASYNC_POSTREAD	0x02	/* post-read synchronization */
#define	BUS_DMASYNC_PREWRITE	0x04	/* pre-write synchronization */
#define	BUS_DMASYNC_POSTWRITE	0x08	/* post-write synchronization */

/*
 *	alpha_bus_t
 *
 *	Busses supported by NetBSD/alpha, used by internal
 *	utility functions.  NOT TO BE USED BY MACHINE-INDEPENDENT
 *	CODE!
 */
typedef enum {
	ALPHA_BUS_TURBOCHANNEL,
	ALPHA_BUS_PCI,
	ALPHA_BUS_EISA,
	ALPHA_BUS_ISA,
	ALPHA_BUS_TLSB,
} alpha_bus_t;

typedef struct alpha_bus_dma_tag	*bus_dma_tag_t;
typedef struct alpha_bus_dmamap		*bus_dmamap_t;

#define BUS_DMA_TAG_VALID(t)    ((t) != (bus_dma_tag_t)0)

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct alpha_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */
};
typedef struct alpha_bus_dma_segment	bus_dma_segment_t;

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */
struct alpha_bus_dma_tag {
	void	*_cookie;		/* cookie used in the guts */
	bus_addr_t _wbase;		/* DMA window base */

	/*
	 * The following two members are used to chain DMA windows
	 * together.  If, during the course of a map load, the
	 * resulting physical memory address is too large to
	 * be addressed by the window, the next window will be
	 * attempted.  These would be chained together like so:
	 *
	 *	direct -> sgmap -> NULL
	 *  or
	 *	sgmap -> NULL
	 *  or
	 *	direct -> NULL
	 *
	 * If the window size is 0, it will not be checked (e.g.
	 * TurboChannel DMA).
	 */
	bus_size_t _wsize;
	struct alpha_bus_dma_tag *_next_window;

	/*
	 * Some chipsets have a built-in boundary constraint, independent
	 * of what the device requests.  This allows that boundary to
	 * be specified.  If the device has a more restrictive constraint,
	 * the map will use that, otherwise this boundary will be used.
	 * This value is ignored if 0.
	 */
	bus_size_t _boundary;

	/*
	 * A chipset may have more than one SGMAP window, so SGMAP
	 * windows also get a pointer to their SGMAP state.
	 */
	struct alpha_sgmap *_sgmap;

	/*
	 * The SGMAP MMU implements a prefetch FIFO to keep data
	 * moving down the pipe, when doing host->bus DMA writes.
	 * The threshold (distance until the next page) used to
	 * trigger the prefetch is differnet on different chipsets,
	 * and we need to know what it is in order to know whether
	 * or not to allocate a spill page.
	 */
	bus_size_t _pfthresh;

	/*
	 * Internal-use only utility methods.  NOT TO BE USED BY
	 * MACHINE-INDEPENDENT CODE!
	 */
	bus_dma_tag_t (*_get_tag)(bus_dma_tag_t, alpha_bus_t);

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

#define	alphabus_dma_get_tag(t, b)				\
	(*(t)->_get_tag)(t, b)

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct alpha_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use my machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxmaxsegsz; /* fixed largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	int		_dm_flags;	/* misc. flags */

	/*
	 * Private cookie to be used by the DMA back-end.
	 */
	void		*_dm_cookie;

	/*
	 * The DMA window that we ended up being mapped in.
	 */
	bus_dma_tag_t	_dm_window;

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_maxsegsz;	/* largest possible segment */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

#endif /* _KERNEL */

#endif /* _ALPHA_BUS_DEFS_H_ */
