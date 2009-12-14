/*	$NetBSD: bus_space.h,v 1.5 2009/12/14 00:46:04 matt Exp $	*/

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

#ifndef _MIPS_BUS_SPACE_H_
#define	_MIPS_BUS_SPACE_H_

#include <sys/types.h>

#ifdef _KERNEL

#define	__BUS_SPACE_HAS_STREAM_METHODS	1

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
 * space ops.  Strict alignment is required by the MIPS architecture,
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
		    d, (u_long)(p), (u_long)sizeof(t),			\
		    __FILE__, __LINE__);				\
	}								\
	(void) 0;							\
})

#define BUS_SPACE_ALIGNED_POINTER(p, t) __BUS_SPACE_ALIGNED_ADDRESS(p, t)
#else
#define	__BUS_SPACE_ADDRESS_SANITY(p, t, d)	(void) 0
#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)
#endif /* BUS_SPACE_DEBUG */
#endif /* _KERNEL */

struct mips_bus_space_translation;

/*
 * Addresses (in bus space).
 */
typedef paddr_t bus_addr_t;
typedef psize_t bus_size_t;
#define	PRIxBUSADDR	PRIxPADDR
#define	PRIxBUSSIZE	PRIxPSIZE

/*
 * Access methods for bus space.
 */
typedef struct mips_bus_space *bus_space_tag_t;
typedef intptr_t bus_space_handle_t;
#define	PRIxBSH		PRIxPTR

struct mips_bus_space {
	/* cookie */
	void		*bs_cookie;

	/* mapping/unmapping */
	int		(*bs_map)(void *, bus_addr_t, bus_size_t, int,
			    bus_space_handle_t *, int);
	void		(*bs_unmap)(void *, bus_space_handle_t, bus_size_t,
			    int);
	int		(*bs_subregion)(void *, bus_space_handle_t, bus_size_t,
			    bus_size_t, bus_space_handle_t *);

	/* MIPS SPECIFIC MAPPING METHOD */
	int		(*bs_translate)(void *, bus_addr_t, bus_size_t, int,
			    struct mips_bus_space_translation *);
	int		(*bs_get_window)(void *, int,
			    struct mips_bus_space_translation *);

	/* allocation/deallocation */
	int		(*bs_alloc)(void *, bus_addr_t, bus_addr_t,
			    bus_size_t, bus_size_t, bus_size_t, int,
			    bus_addr_t *, bus_space_handle_t *);
	void		(*bs_free)(void *, bus_space_handle_t, bus_size_t);

	/* get kernel virtual address */
	void *		(*bs_vaddr)(void *, bus_space_handle_t);

	/* mmap for user */
	paddr_t		(*bs_mmap)(void *, bus_addr_t, off_t, int, int);

	/* barrier */
	void		(*bs_barrier)(void *, bus_space_handle_t,
			    bus_size_t, bus_size_t, int);

	/* read (single) */
	uint8_t		(*bs_r_1)(void *, bus_space_handle_t, bus_size_t);
	uint16_t	(*bs_r_2)(void *, bus_space_handle_t, bus_size_t);
	uint32_t	(*bs_r_4)(void *, bus_space_handle_t, bus_size_t);
	uint64_t	(*bs_r_8)(void *, bus_space_handle_t, bus_size_t);

	/* read multiple */
	void		(*bs_rm_1)(void *, bus_space_handle_t, bus_size_t,
			    uint8_t *, bus_size_t);
	void		(*bs_rm_2)(void *, bus_space_handle_t, bus_size_t,
			    uint16_t *, bus_size_t);
	void		(*bs_rm_4)(void *, bus_space_handle_t, bus_size_t,
			    uint32_t *, bus_size_t);
	void		(*bs_rm_8)(void *, bus_space_handle_t, bus_size_t,
			    uint64_t *, bus_size_t);
					
	/* read region */
	void		(*bs_rr_1)(void *, bus_space_handle_t, bus_size_t,
			    uint8_t *, bus_size_t);
	void		(*bs_rr_2)(void *, bus_space_handle_t, bus_size_t,
			    uint16_t *, bus_size_t);
	void		(*bs_rr_4)(void *, bus_space_handle_t, bus_size_t,
			    uint32_t *, bus_size_t);
	void		(*bs_rr_8)(void *, bus_space_handle_t, bus_size_t,
			    uint64_t *, bus_size_t);
					
	/* write (single) */
	void		(*bs_w_1)(void *, bus_space_handle_t, bus_size_t,
			    uint8_t);
	void		(*bs_w_2)(void *, bus_space_handle_t, bus_size_t,
			    uint16_t);
	void		(*bs_w_4)(void *, bus_space_handle_t, bus_size_t,
			    uint32_t);
	void		(*bs_w_8)(void *, bus_space_handle_t, bus_size_t,
			    uint64_t);

	/* write multiple */
	void		(*bs_wm_1)(void *, bus_space_handle_t, bus_size_t,
			    const uint8_t *, bus_size_t);
	void		(*bs_wm_2)(void *, bus_space_handle_t, bus_size_t,
			    const uint16_t *, bus_size_t);
	void		(*bs_wm_4)(void *, bus_space_handle_t, bus_size_t,
			    const uint32_t *, bus_size_t);
	void		(*bs_wm_8)(void *, bus_space_handle_t, bus_size_t,
			    const uint64_t *, bus_size_t);
					
	/* write region */
	void		(*bs_wr_1)(void *, bus_space_handle_t, bus_size_t,
			    const uint8_t *, bus_size_t);
	void		(*bs_wr_2)(void *, bus_space_handle_t, bus_size_t,
			    const uint16_t *, bus_size_t);
	void		(*bs_wr_4)(void *, bus_space_handle_t, bus_size_t,
			    const uint32_t *, bus_size_t);
	void		(*bs_wr_8)(void *, bus_space_handle_t, bus_size_t,
			    const uint64_t *, bus_size_t);

	/* read (single) stream */
	uint8_t		(*bs_rs_1)(void *, bus_space_handle_t, bus_size_t);
	uint16_t	(*bs_rs_2)(void *, bus_space_handle_t, bus_size_t);
	uint32_t	(*bs_rs_4)(void *, bus_space_handle_t, bus_size_t);
	uint64_t	(*bs_rs_8)(void *, bus_space_handle_t, bus_size_t);

	/* read multiple stream */
	void		(*bs_rms_1)(void *, bus_space_handle_t, bus_size_t,
			    uint8_t *, bus_size_t);
	void		(*bs_rms_2)(void *, bus_space_handle_t, bus_size_t,
			    uint16_t *, bus_size_t);
	void		(*bs_rms_4)(void *, bus_space_handle_t, bus_size_t,
			    uint32_t *, bus_size_t);
	void		(*bs_rms_8)(void *, bus_space_handle_t, bus_size_t,
			    uint64_t *, bus_size_t);
					
	/* read region stream */
	void		(*bs_rrs_1)(void *, bus_space_handle_t, bus_size_t,
			    uint8_t *, bus_size_t);
	void		(*bs_rrs_2)(void *, bus_space_handle_t, bus_size_t,
			    uint16_t *, bus_size_t);
	void		(*bs_rrs_4)(void *, bus_space_handle_t, bus_size_t,
			    uint32_t *, bus_size_t);
	void		(*bs_rrs_8)(void *, bus_space_handle_t, bus_size_t,
			    uint64_t *, bus_size_t);
					
	/* write (single) stream */
	void		(*bs_ws_1)(void *, bus_space_handle_t, bus_size_t,
			    uint8_t);
	void		(*bs_ws_2)(void *, bus_space_handle_t, bus_size_t,
			    uint16_t);
	void		(*bs_ws_4)(void *, bus_space_handle_t, bus_size_t,
			    uint32_t);
	void		(*bs_ws_8)(void *, bus_space_handle_t, bus_size_t,
			    uint64_t);

	/* write multiple stream */
	void		(*bs_wms_1)(void *, bus_space_handle_t, bus_size_t,
			    const uint8_t *, bus_size_t);
	void		(*bs_wms_2)(void *, bus_space_handle_t, bus_size_t,
			    const uint16_t *, bus_size_t);
	void		(*bs_wms_4)(void *, bus_space_handle_t, bus_size_t,
			    const uint32_t *, bus_size_t);
	void		(*bs_wms_8)(void *, bus_space_handle_t, bus_size_t,
			    const uint64_t *, bus_size_t);
					
	/* write region stream */
	void		(*bs_wrs_1)(void *, bus_space_handle_t, bus_size_t,
			    const uint8_t *, bus_size_t);
	void		(*bs_wrs_2)(void *, bus_space_handle_t, bus_size_t,
			    const uint16_t *, bus_size_t);
	void		(*bs_wrs_4)(void *, bus_space_handle_t, bus_size_t,
			    const uint32_t *, bus_size_t);
	void		(*bs_wrs_8)(void *, bus_space_handle_t, bus_size_t,
			    const uint64_t *, bus_size_t);

	/* set multiple */
	void		(*bs_sm_1)(void *, bus_space_handle_t, bus_size_t,
			    uint8_t, bus_size_t);
	void		(*bs_sm_2)(void *, bus_space_handle_t, bus_size_t,
			    uint16_t, bus_size_t);
	void		(*bs_sm_4)(void *, bus_space_handle_t, bus_size_t,
			    uint32_t, bus_size_t);
	void		(*bs_sm_8)(void *, bus_space_handle_t, bus_size_t,
			    uint64_t, bus_size_t);

	/* set region */
	void		(*bs_sr_1)(void *, bus_space_handle_t, bus_size_t,
			    uint8_t, bus_size_t);
	void		(*bs_sr_2)(void *, bus_space_handle_t, bus_size_t,
			    uint16_t, bus_size_t);
	void		(*bs_sr_4)(void *, bus_space_handle_t, bus_size_t,
			    uint32_t, bus_size_t);
	void		(*bs_sr_8)(void *, bus_space_handle_t, bus_size_t,
			    uint64_t, bus_size_t);

	/* copy */
	void		(*bs_c_1)(void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t);
	void		(*bs_c_2)(void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t);
	void		(*bs_c_4)(void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t);
	void		(*bs_c_8)(void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t);
};

/*
 * Translation of an MIPS bus address; INTERNAL USE ONLY.
 */
struct mips_bus_space_translation {
	bus_addr_t	mbst_bus_start;	/* start of bus window */
	bus_addr_t	mbst_bus_end;	/* end of bus window */
	paddr_t		mbst_sys_start;	/* start of sysBus window */
	paddr_t		mbst_sys_end;	/* end of sysBus window */
	int		mbst_align_stride;/* alignment stride */
	int		mbst_flags;	/* flags; see below */
};

#ifdef _KERNEL
/*
 * Utility macros; INTERNAL USE ONLY.
 */
#define	__bs_c(a,b)		__CONCAT(a,b)
#define	__bs_opname(op,size)	__bs_c(__bs_c(__bs_c(bs_,op),_),size)

#define	__bs_r(type, sz, tn, t, h, o)					\
	(__BUS_SPACE_ADDRESS_SANITY((h) + (o), tn, "bus addr"),		\
	 (*(t)->__bs_opname(type,sz))((t)->bs_cookie, h, o))

#define	__bs_w(type, sz, tn, t, h, o, v)				\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), tn, "bus addr");		\
	(*(t)->__bs_opname(type,sz))((t)->bs_cookie, h, o, v);		\
} while (0)

#define	__bs_nonsingle(type, sz, tn, t, h, o, a, c)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((a), tn, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), tn, "bus addr");		\
	(*(t)->__bs_opname(type,sz))((t)->bs_cookie, h, o, a, c);	\
} while (0)

#define	__bs_set(type, sz, tn, t, h, o, v, c)				\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), tn, "bus addr");		\
	(*(t)->__bs_opname(type,sz))((t)->bs_cookie, h, o, v, c);	\
} while (0)

#define	__bs_copy(sz, tn, t, h1, o1, h2, o2, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h1) + (o1), tn, "bus addr 1");	\
	__BUS_SPACE_ADDRESS_SANITY((h2) + (o2), tn, "bus addr 2");	\
	(*(t)->__bs_opname(c,sz))((t)->bs_cookie, h1, o1, h2, o2, cnt); \
} while (0)


/*
 * Mapping and unmapping operations.
 */
#define	bus_space_map(t, a, s, f, hp)					\
	(*(t)->bs_map)((t)->bs_cookie, (a), (s), (f), (hp), 1)
#define	mips_bus_space_map_noacct(t, a, s, f, hp)			\
	(*(t)->bs_map)((t)->bs_cookie, (a), (s), (f), (hp), 0)
#define	bus_space_unmap(t, h, s)					\
	(*(t)->bs_unmap)((t)->bs_cookie, (h), (s), 1)
#define	mips_bus_space_unmap_noacct(t, h, s)				\
	(*(t)->bs_unmap)((t)->bs_cookie, (h), (s), 0)
#define	bus_space_subregion(t, h, o, s, hp)				\
	(*(t)->bs_subregion)((t)->bs_cookie, (h), (o), (s), (hp))

#define	mips_bus_space_translate(t, a, s, f, bst)			\
	(*(t)->bs_translate)((t)->bs_cookie, (a), (s), (f), (bst))
#define	mips_bus_space_get_window(t, w, bst)				\
	(*(t)->bs_get_window)((t)->bs_cookie, (w), (bst))
#endif /* _KERNEL */

#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02
#define	BUS_SPACE_MAP_PREFETCHABLE     	0x04

#ifdef _KERNEL
/*
 * Allocation and deallocation operations.
 */
#define	bus_space_alloc(t, rs, re, s, a, b, f, ap, hp)			\
	(*(t)->bs_alloc)((t)->bs_cookie, (rs), (re), (s), (a), (b),	\
	    (f), (ap), (hp))
#define	bus_space_free(t, h, s)						\
	(*(t)->bs_free)((t)->bs_cookie, (h), (s))

/*
 * Get kernel virtual address for ranges mapped BUS_SPACE_MAP_LINEAR.
 */
#define bus_space_vaddr(t, h) \
	(*(t)->bs_vaddr)((t)->bs_cookie, (h))

/*
 * Mmap bus space for a user application.
 */
#define	bus_space_mmap(t, a, o, p, f)					\
	(*(t)->bs_mmap)((t)->bs_cookie, (a), (o), (p), (f))

/*
 * Bus barrier operations.
 */
#define	bus_space_barrier(t, h, o, l, f)				\
	(*(t)->bs_barrier)((t)->bs_cookie, (h), (o), (l), (f))

#define	BUS_SPACE_BARRIER_READ	0x01
#define	BUS_SPACE_BARRIER_WRITE	0x02

/*
 * New style.
 */
#define	BUS_SPACE_BARRIER_SYNC		0x03
#define	BUS_SPACE_BARRIER_READ_BEFORE_READ	BUS_SPACE_BARRIER_READ
#define	BUS_SPACE_BARRIER_READ_BEFORE_WRITE	BUS_SPACE_BARRIER_READ
#define	BUS_SPACE_BARRIER_WRITE_BEFORE_READ	BUS_SPACE_BARRIER_WRITE
#define	BUS_SPACE_BARRIER_WRITE_BEFORE_WRITE	BUS_SPACE_BARRIER_WRITE

/*
 * Bus read (single) operations.
 */
#define	bus_space_read_1(t, h, o)					\
	__bs_r(r,1,uint8_t,(t),(h),(o))
#define	bus_space_read_2(t, h, o)					\
	__bs_r(r,2,uint16_t,(t),(h),(o))
#define	bus_space_read_4(t, h, o)					\
	__bs_r(r,4,uint32_t,(t),(h),(o))
#define	bus_space_read_8(t, h, o)					\
	__bs_r(r,8,uint64_t,(t),(h),(o))
#define	bus_space_read_stream_1(t, h, o)				\
	__bs_r(rs,1,uint8_t,(t),(h),(o))
#define	bus_space_read_stream_2(t, h, o)				\
	__bs_r(rs,2,uint16_t,(t),(h),(o))
#define	bus_space_read_stream_4(t, h, o)				\
	__bs_r(rs,4,uint32_t,(t),(h),(o))
#define	bus_space_read_stream_8(t, h, o)				\
	__bs_r(rs,8,uint64_t,(t),(h),(o))


/*
 * Bus read multiple operations.
 */
#define	bus_space_read_multi_1(t, h, o, a, c)				\
	__bs_nonsingle(rm,1,uint8_t,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_2(t, h, o, a, c)				\
	__bs_nonsingle(rm,2,uint16_t,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_4(t, h, o, a, c)				\
	__bs_nonsingle(rm,4,uint32_t,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_8(t, h, o, a, c)				\
	__bs_nonsingle(rm,8,uint64_t,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_stream_1(t, h, o, a, c)			\
	__bs_nonsingle(rms,1,uint8_t,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_stream_2(t, h, o, a, c)			\
	__bs_nonsingle(rms,2,uint16_t,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_stream_4(t, h, o, a, c)			\
	__bs_nonsingle(rms,4,uint32_t,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_stream_8(t, h, o, a, c)			\
	__bs_nonsingle(rms,8,uint64_t,(t),(h),(o),(a),(c))


/*
 * Bus read region operations.
 */
#define	bus_space_read_region_1(t, h, o, a, c)				\
	__bs_nonsingle(rr,1,uint8_t,(t),(h),(o),(a),(c))
#define	bus_space_read_region_2(t, h, o, a, c)				\
	__bs_nonsingle(rr,2,uint16_t,(t),(h),(o),(a),(c))
#define	bus_space_read_region_4(t, h, o, a, c)				\
	__bs_nonsingle(rr,4,uint32_t,(t),(h),(o),(a),(c))
#define	bus_space_read_region_8(t, h, o, a, c)				\
	__bs_nonsingle(rr,8,uint64_t,(t),(h),(o),(a),(c))
#define	bus_space_read_region_stream_1(t, h, o, a, c)			\
	__bs_nonsingle(rrs,1,uint8_t,(t),(h),(o),(a),(c))
#define	bus_space_read_region_stream_2(t, h, o, a, c)			\
	__bs_nonsingle(rrs,2,uint16_t,(t),(h),(o),(a),(c))
#define	bus_space_read_region_stream_4(t, h, o, a, c)			\
	__bs_nonsingle(rrs,4,uint32_t,(t),(h),(o),(a),(c))
#define	bus_space_read_region_stream_8(t, h, o, a, c)			\
	__bs_nonsingle(rrs,8,uint64_t,(t),(h),(o),(a),(c))


/*
 * Bus write (single) operations.
 */
#define	bus_space_write_1(t, h, o, v)					\
	__bs_w(w,1,uint8_t,(t),(h),(o),(v))
#define	bus_space_write_2(t, h, o, v)					\
	__bs_w(w,2,uint16_t,(t),(h),(o),(v))
#define	bus_space_write_4(t, h, o, v)					\
	__bs_w(w,4,uint32_t,(t),(h),(o),(v))
#define	bus_space_write_8(t, h, o, v)					\
	__bs_w(w,8,uint64_t,(t),(h),(o),(v))
#define	bus_space_write_stream_1(t, h, o, v)				\
	__bs_w(ws,1,uint8_t,(t),(h),(o),(v))
#define	bus_space_write_stream_2(t, h, o, v)				\
	__bs_w(ws,2,uint16_t,(t),(h),(o),(v))
#define	bus_space_write_stream_4(t, h, o, v)				\
	__bs_w(ws,4,uint32_t,(t),(h),(o),(v))
#define	bus_space_write_stream_8(t, h, o, v)				\
	__bs_w(ws,8,uint64_t,(t),(h),(o),(v))


/*
 * Bus write multiple operations.
 */
#define	bus_space_write_multi_1(t, h, o, a, c)				\
	__bs_nonsingle(wm,1,uint8_t,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_2(t, h, o, a, c)				\
	__bs_nonsingle(wm,2,uint16_t,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_4(t, h, o, a, c)				\
	__bs_nonsingle(wm,4,uint32_t,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_8(t, h, o, a, c)				\
	__bs_nonsingle(wm,8,uint64_t,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_stream_1(t, h, o, a, c)			\
	__bs_nonsingle(wms,1,uint8_t,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_stream_2(t, h, o, a, c)			\
	__bs_nonsingle(wms,2,uint16_t,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_stream_4(t, h, o, a, c)			\
	__bs_nonsingle(wms,4,uint32_t,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_stream_8(t, h, o, a, c)			\
	__bs_nonsingle(wms,8,uint64_t,(t),(h),(o),(a),(c))


/*
 * Bus write region operations.
 */
#define	bus_space_write_region_1(t, h, o, a, c)				\
	__bs_nonsingle(wr,1,uint8_t,(t),(h),(o),(a),(c))
#define	bus_space_write_region_2(t, h, o, a, c)				\
	__bs_nonsingle(wr,2,uint16_t,(t),(h),(o),(a),(c))
#define	bus_space_write_region_4(t, h, o, a, c)				\
	__bs_nonsingle(wr,4,uint32_t,(t),(h),(o),(a),(c))
#define	bus_space_write_region_8(t, h, o, a, c)				\
	__bs_nonsingle(wr,8,uint64_t,(t),(h),(o),(a),(c))
#define	bus_space_write_region_stream_1(t, h, o, a, c)			\
	__bs_nonsingle(wrs,1,uint8_t,(t),(h),(o),(a),(c))
#define	bus_space_write_region_stream_2(t, h, o, a, c)			\
	__bs_nonsingle(wrs,2,uint16_t,(t),(h),(o),(a),(c))
#define	bus_space_write_region_stream_4(t, h, o, a, c)			\
	__bs_nonsingle(wrs,4,uint32_t,(t),(h),(o),(a),(c))
#define	bus_space_write_region_stream_8(t, h, o, a, c)			\
	__bs_nonsingle(wrs,8,uint64_t,(t),(h),(o),(a),(c))


/*
 * Set multiple operations.
 */
#define	bus_space_set_multi_1(t, h, o, v, c)				\
	__bs_set(sm,1,uint8_t,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_2(t, h, o, v, c)				\
	__bs_set(sm,2,uint16_t,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_4(t, h, o, v, c)				\
	__bs_set(sm,4,uint32_t,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_8(t, h, o, v, c)				\
	__bs_set(sm,8,uint64_t,(t),(h),(o),(v),(c))


/*
 * Set region operations.
 */
#define	bus_space_set_region_1(t, h, o, v, c)				\
	__bs_set(sr,1,uint8_t,(t),(h),(o),(v),(c))
#define	bus_space_set_region_2(t, h, o, v, c)				\
	__bs_set(sr,2,uint16_t,(t),(h),(o),(v),(c))
#define	bus_space_set_region_4(t, h, o, v, c)				\
	__bs_set(sr,4,uint32_t,(t),(h),(o),(v),(c))
#define	bus_space_set_region_8(t, h, o, v, c)				\
	__bs_set(sr,8,uint64_t,(t),(h),(o),(v),(c))


/*
 * Copy region operations.
 */
#define	bus_space_copy_region_1(t, h1, o1, h2, o2, c)			\
	__bs_copy(1, uint8_t, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_2(t, h1, o1, h2, o2, c)			\
	__bs_copy(2, uint16_t, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_4(t, h1, o1, h2, o2, c)			\
	__bs_copy(4, uint32_t, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_8(t, h1, o1, h2, o2, c)			\
	__bs_copy(8, uint64_t, (t), (h1), (o1), (h2), (o2), (c))


#endif /* _KERNEL */

#endif /* _MIPS_BUS_SPACE_H_ */
