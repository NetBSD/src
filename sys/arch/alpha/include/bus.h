/* $NetBSD: bus.h,v 1.29.8.1 1999/12/27 18:31:25 wrstuden Exp $ */

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _ALPHA_BUS_H_
#define	_ALPHA_BUS_H_

/*
 * Turn on BUS_SPACE_DEBUG if the global DEBUG option is enabled.
 */
#if defined(DEBUG) && !defined(BUS_SPACE_DEBUG)
#define	BUS_SPACE_DEBUG
#endif

#ifdef BUS_SPACE_DEBUG
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

/*
 * Addresses (in bus space).
 */
typedef u_long bus_addr_t;
typedef u_long bus_size_t;

/*
 * Access methods for bus space.
 */
typedef struct alpha_bus_space *bus_space_tag_t;
typedef u_long bus_space_handle_t;

struct alpha_bus_space {
	/* cookie */
	void		*abs_cookie;

	/* mapping/unmapping */
	int		(*abs_map) __P((void *, bus_addr_t, bus_size_t,
			    int, bus_space_handle_t *, int));
	void		(*abs_unmap) __P((void *, bus_space_handle_t,
			    bus_size_t, int));
	int		(*abs_subregion) __P((void *, bus_space_handle_t,
			    bus_size_t, bus_size_t, bus_space_handle_t *));

	/* allocation/deallocation */
	int		(*abs_alloc) __P((void *, bus_addr_t, bus_addr_t,
			    bus_size_t, bus_size_t, bus_size_t, int,
			    bus_addr_t *, bus_space_handle_t *));
	void		(*abs_free) __P((void *, bus_space_handle_t,
			    bus_size_t));

	/* barrier */
	void		(*abs_barrier) __P((void *, bus_space_handle_t,
			    bus_size_t, bus_size_t, int));

	/* read (single) */
	u_int8_t	(*abs_r_1) __P((void *, bus_space_handle_t,
			    bus_size_t));
	u_int16_t	(*abs_r_2) __P((void *, bus_space_handle_t,
			    bus_size_t));
	u_int32_t	(*abs_r_4) __P((void *, bus_space_handle_t,
			    bus_size_t));
	u_int64_t	(*abs_r_8) __P((void *, bus_space_handle_t,
			    bus_size_t));

	/* read multiple */
	void		(*abs_rm_1) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int8_t *, bus_size_t));
	void		(*abs_rm_2) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int16_t *, bus_size_t));
	void		(*abs_rm_4) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int32_t *, bus_size_t));
	void		(*abs_rm_8) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int64_t *, bus_size_t));
					
	/* read region */
	void		(*abs_rr_1) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int8_t *, bus_size_t));
	void		(*abs_rr_2) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int16_t *, bus_size_t));
	void		(*abs_rr_4) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int32_t *, bus_size_t));
	void		(*abs_rr_8) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int64_t *, bus_size_t));
					
	/* write (single) */
	void		(*abs_w_1) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int8_t));
	void		(*abs_w_2) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int16_t));
	void		(*abs_w_4) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int32_t));
	void		(*abs_w_8) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int64_t));

	/* write multiple */
	void		(*abs_wm_1) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int8_t *, bus_size_t));
	void		(*abs_wm_2) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int16_t *, bus_size_t));
	void		(*abs_wm_4) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int32_t *, bus_size_t));
	void		(*abs_wm_8) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int64_t *, bus_size_t));
					
	/* write region */
	void		(*abs_wr_1) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int8_t *, bus_size_t));
	void		(*abs_wr_2) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int16_t *, bus_size_t));
	void		(*abs_wr_4) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int32_t *, bus_size_t));
	void		(*abs_wr_8) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int64_t *, bus_size_t));

	/* set multiple */
	void		(*abs_sm_1) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int8_t, bus_size_t));
	void		(*abs_sm_2) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int16_t, bus_size_t));
	void		(*abs_sm_4) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int32_t, bus_size_t));
	void		(*abs_sm_8) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int64_t, bus_size_t));

	/* set region */
	void		(*abs_sr_1) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int8_t, bus_size_t));
	void		(*abs_sr_2) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int16_t, bus_size_t));
	void		(*abs_sr_4) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int32_t, bus_size_t));
	void		(*abs_sr_8) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int64_t, bus_size_t));

	/* copy */
	void		(*abs_c_1) __P((void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t));
	void		(*abs_c_2) __P((void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t));
	void		(*abs_c_4) __P((void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t));
	void		(*abs_c_8) __P((void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t));
};


/*
 * Utility macros; INTERNAL USE ONLY.
 */
#define	__abs_c(a,b)		__CONCAT(a,b)
#define	__abs_opname(op,size)	__abs_c(__abs_c(__abs_c(abs_,op),_),size)

#define	__abs_rs(sz, tn, t, h, o)					\
	(__BUS_SPACE_ADDRESS_SANITY((h) + (o), tn, "bus addr"),		\
	 (*(t)->__abs_opname(r,sz))((t)->abs_cookie, h, o))

#define	__abs_ws(sz, tn, t, h, o, v)					\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), tn, "bus addr");		\
	(*(t)->__abs_opname(w,sz))((t)->abs_cookie, h, o, v);		\
} while (0)

#define	__abs_nonsingle(type, sz, tn, t, h, o, a, c)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((a), tn, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), tn, "bus addr");		\
	(*(t)->__abs_opname(type,sz))((t)->abs_cookie, h, o, a, c);	\
} while (0)

#define	__abs_set(type, sz, tn, t, h, o, v, c)				\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), tn, "bus addr");		\
	(*(t)->__abs_opname(type,sz))((t)->abs_cookie, h, o, v, c);	\
} while (0)

#define	__abs_copy(sz, tn, t, h1, o1, h2, o2, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h1) + (o1), tn, "bus addr 1");	\
	__BUS_SPACE_ADDRESS_SANITY((h2) + (o2), tn, "bus addr 2");	\
	(*(t)->__abs_opname(c,sz))((t)->abs_cookie, h1, o1, h2, o2, cnt); \
} while (0)


/*
 * Mapping and unmapping operations.
 */
#define	bus_space_map(t, a, s, f, hp)					\
	(*(t)->abs_map)((t)->abs_cookie, (a), (s), (f), (hp), 1)
#define	alpha_bus_space_map_noacct(t, a, s, f, hp)			\
	(*(t)->abs_map)((t)->abs_cookie, (a), (s), (f), (hp), 0)
#define	bus_space_unmap(t, h, s)					\
	(*(t)->abs_unmap)((t)->abs_cookie, (h), (s), 1)
#define	alpha_bus_space_unmap_noacct(t, h, s)				\
	(*(t)->abs_unmap)((t)->abs_cookie, (h), (s), 0)
#define	bus_space_subregion(t, h, o, s, hp)				\
	(*(t)->abs_subregion)((t)->abs_cookie, (h), (o), (s), (hp))

#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02

/*
 * Allocation and deallocation operations.
 */
#define	bus_space_alloc(t, rs, re, s, a, b, f, ap, hp)			\
	(*(t)->abs_alloc)((t)->abs_cookie, (rs), (re), (s), (a), (b),	\
	    (f), (ap), (hp))
#define	bus_space_free(t, h, s)						\
	(*(t)->abs_free)((t)->abs_cookie, (h), (s))


/*
 * Bus barrier operations.
 */
#define	bus_space_barrier(t, h, o, l, f)				\
	(*(t)->abs_barrier)((t)->abs_cookie, (h), (o), (l), (f))

#define	BUS_SPACE_BARRIER_READ	0x01
#define	BUS_SPACE_BARRIER_WRITE	0x02


/*
 * Bus read (single) operations.
 */
#define	bus_space_read_1(t, h, o)	__abs_rs(1,u_int8_t,(t),(h),(o))
#define	bus_space_read_2(t, h, o)	__abs_rs(2,u_int16_t,(t),(h),(o))
#define	bus_space_read_4(t, h, o)	__abs_rs(4,u_int32_t,(t),(h),(o))
#define	bus_space_read_8(t, h, o)	__abs_rs(8,u_int64_t,(t),(h),(o))


/*
 * Bus read multiple operations.
 */
#define	bus_space_read_multi_1(t, h, o, a, c)				\
	__abs_nonsingle(rm,1,u_int8_t,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_2(t, h, o, a, c)				\
	__abs_nonsingle(rm,2,u_int16_t,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_4(t, h, o, a, c)				\
	__abs_nonsingle(rm,4,u_int32_t,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_8(t, h, o, a, c)				\
	__abs_nonsingle(rm,8,u_int64_t,(t),(h),(o),(a),(c))


/*
 * Bus read region operations.
 */
#define	bus_space_read_region_1(t, h, o, a, c)				\
	__abs_nonsingle(rr,1,u_int8_t,(t),(h),(o),(a),(c))
#define	bus_space_read_region_2(t, h, o, a, c)				\
	__abs_nonsingle(rr,2,u_int16_t,(t),(h),(o),(a),(c))
#define	bus_space_read_region_4(t, h, o, a, c)				\
	__abs_nonsingle(rr,4,u_int32_t,(t),(h),(o),(a),(c))
#define	bus_space_read_region_8(t, h, o, a, c)				\
	__abs_nonsingle(rr,8,u_int64_t,(t),(h),(o),(a),(c))


/*
 * Bus write (single) operations.
 */
#define	bus_space_write_1(t, h, o, v)	__abs_ws(1,u_int8_t,(t),(h),(o),(v))
#define	bus_space_write_2(t, h, o, v)	__abs_ws(2,u_int16_t,(t),(h),(o),(v))
#define	bus_space_write_4(t, h, o, v)	__abs_ws(4,u_int32_t,(t),(h),(o),(v))
#define	bus_space_write_8(t, h, o, v)	__abs_ws(8,u_int64_t,(t),(h),(o),(v))


/*
 * Bus write multiple operations.
 */
#define	bus_space_write_multi_1(t, h, o, a, c)				\
	__abs_nonsingle(wm,1,u_int8_t,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_2(t, h, o, a, c)				\
	__abs_nonsingle(wm,2,u_int16_t,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_4(t, h, o, a, c)				\
	__abs_nonsingle(wm,4,u_int32_t,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_8(t, h, o, a, c)				\
	__abs_nonsingle(wm,8,u_int64_t,(t),(h),(o),(a),(c))


/*
 * Bus write region operations.
 */
#define	bus_space_write_region_1(t, h, o, a, c)				\
	__abs_nonsingle(wr,1,u_int8_t,(t),(h),(o),(a),(c))
#define	bus_space_write_region_2(t, h, o, a, c)				\
	__abs_nonsingle(wr,2,u_int16_t,(t),(h),(o),(a),(c))
#define	bus_space_write_region_4(t, h, o, a, c)				\
	__abs_nonsingle(wr,4,u_int32_t,(t),(h),(o),(a),(c))
#define	bus_space_write_region_8(t, h, o, a, c)				\
	__abs_nonsingle(wr,8,u_int64_t,(t),(h),(o),(a),(c))


/*
 * Set multiple operations.
 */
#define	bus_space_set_multi_1(t, h, o, v, c)				\
	__abs_set(sm,1,u_int8_t,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_2(t, h, o, v, c)				\
	__abs_set(sm,2,u_int16_t,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_4(t, h, o, v, c)				\
	__abs_set(sm,4,u_int32_t,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_8(t, h, o, v, c)				\
	__abs_set(sm,8,u_int64_t,(t),(h),(o),(v),(c))


/*
 * Set region operations.
 */
#define	bus_space_set_region_1(t, h, o, v, c)				\
	__abs_set(sr,1,u_int8_t,(t),(h),(o),(v),(c))
#define	bus_space_set_region_2(t, h, o, v, c)				\
	__abs_set(sr,2,u_int16_t,(t),(h),(o),(v),(c))
#define	bus_space_set_region_4(t, h, o, v, c)				\
	__abs_set(sr,4,u_int32_t,(t),(h),(o),(v),(c))
#define	bus_space_set_region_8(t, h, o, v, c)				\
	__abs_set(sr,8,u_int64_t,(t),(h),(o),(v),(c))


/*
 * Copy region operations.
 */
#define	bus_space_copy_region_1(t, h1, o1, h2, o2, c)			\
	__abs_copy(1, u_int8_t, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_2(t, h1, o1, h2, o2, c)			\
	__abs_copy(2, u_int16_t, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_4(t, h1, o1, h2, o2, c)			\
	__abs_copy(4, u_int32_t, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_8(t, h1, o1, h2, o2, c)			\
	__abs_copy(8, u_int64_t, (t), (h1), (o1), (h2), (o2), (c))


/*
 * Bus DMA methods.
 */

/*
 * Flags used in various bus DMA methods.
 */
#define	BUS_DMA_WAITOK		0x00	/* safe to sleep (pseudo-flag) */
#define	BUS_DMA_NOWAIT		0x01	/* not safe to sleep */
#define	BUS_DMA_ALLOCNOW	0x02	/* perform resource allocation now */
#define	BUS_DMA_COHERENT	0x04	/* hint: map memory DMA coherent */
#define	BUS_DMA_BUS1		0x10	/* placeholders for bus functions... */
#define	BUS_DMA_BUS2		0x20
#define	BUS_DMA_BUS3		0x40
#define	BUS_DMA_BUS4		0x80

/*
 * Private flags stored in the DMA map.
 */
#define	DMAMAP_HAS_SGMAP	0x80000000	/* sgva/len are valid */

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
	 * be specified.  If the device has a more restrictive contraint,
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
	 * Internal-use only utility methods.  NOT TO BE USED BY
	 * MACHINE-INDEPENDENT CODE!
	 */
	bus_dma_tag_t (*_get_tag) __P((bus_dma_tag_t, alpha_bus_t));

	/*
	 * DMA mapping methods.
	 */
	int	(*_dmamap_create) __P((bus_dma_tag_t, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *));
	void	(*_dmamap_destroy) __P((bus_dma_tag_t, bus_dmamap_t));
	int	(*_dmamap_load) __P((bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int));
	int	(*_dmamap_load_mbuf) __P((bus_dma_tag_t, bus_dmamap_t,
		    struct mbuf *, int));
	int	(*_dmamap_load_uio) __P((bus_dma_tag_t, bus_dmamap_t,
		    struct uio *, int));
	int	(*_dmamap_load_raw) __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int));
	void	(*_dmamap_unload) __P((bus_dma_tag_t, bus_dmamap_t));
	void	(*_dmamap_sync) __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int));

	/*
	 * DMA memory utility functions.
	 */
	int	(*_dmamem_alloc) __P((bus_dma_tag_t, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int));
	void	(*_dmamem_free) __P((bus_dma_tag_t,
		    bus_dma_segment_t *, int));
	int	(*_dmamem_map) __P((bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, caddr_t *, int));
	void	(*_dmamem_unmap) __P((bus_dma_tag_t, caddr_t, size_t));
	int	(*_dmamem_mmap) __P((bus_dma_tag_t, bus_dma_segment_t *,
		    int, int, int, int));
};

#define	alphabus_dma_get_tag(t, b)				\
	(*(t)->_get_tag)(t, b)

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
	bus_size_t	_dm_maxsegsz;	/* largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	int		_dm_flags;	/* misc. flags */

	/*
	 * This is used only for SGMAP-mapped DMA, but we keep it
	 * here to avoid pointless indirection.
	 */
	int		_dm_pteidx;	/* PTE index */
	int		_dm_ptecnt;	/* PTE count */
	u_long		_dm_sgva;	/* allocated sgva */
	bus_size_t	_dm_sgvalen;	/* svga length */

	/*
	 * Private cookie to be used by the DMA back-end.
	 */
	void		*_dm_cookie;

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

#ifdef _ALPHA_BUS_DMA_PRIVATE
int	_bus_dmamap_create __P((bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *));
void	_bus_dmamap_destroy __P((bus_dma_tag_t, bus_dmamap_t));

int	_bus_dmamap_load_direct __P((bus_dma_tag_t, bus_dmamap_t,
	    void *, bus_size_t, struct proc *, int));
int	_bus_dmamap_load_mbuf_direct __P((bus_dma_tag_t,
	    bus_dmamap_t, struct mbuf *, int));
int	_bus_dmamap_load_uio_direct __P((bus_dma_tag_t,
	    bus_dmamap_t, struct uio *, int));
int	_bus_dmamap_load_raw_direct __P((bus_dma_tag_t,
	    bus_dmamap_t, bus_dma_segment_t *, int, bus_size_t, int));

void	_bus_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
void	_bus_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int));

int	_bus_dmamem_alloc __P((bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags));
int	_bus_dmamem_alloc_range __P((bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    paddr_t low, paddr_t high));
void	_bus_dmamem_free __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs));
int	_bus_dmamem_map __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, size_t size, caddr_t *kvap, int flags));
void	_bus_dmamem_unmap __P((bus_dma_tag_t tag, caddr_t kva,
	    size_t size));
int	_bus_dmamem_mmap __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, int off, int prot, int flags));
#endif /* _ALPHA_BUS_DMA_PRIVATE */

#endif /* _ALPHA_BUS_H_ */
