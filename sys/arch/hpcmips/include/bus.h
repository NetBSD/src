/*	$NetBSD: bus.h,v 1.16 2003/01/28 01:07:58 kent Exp $	*/

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

/*
 * derived from arch/arm/include/bus.h Rev. 1.3
 */

#ifndef _SYS_BUS_H_
#define _SYS_BUS_H_

#include <machine/bus_types.h>

#ifndef BUS_SPACE_MD_CALLS

#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02
#define	BUS_SPACE_MAP_PREFETCHABLE     	0x04

#define	BUS_SPACE_BARRIER_READ	0x01
#define	BUS_SPACE_BARRIER_WRITE	0x02

#ifndef BUS_SPACE_MD_TYPES
typedef struct bus_space_tag *bus_space_tag_t;
#endif

/*
 * bus space operaion table
 */
struct bus_space_ops {
	/* mapping/unmapping */
	int	  (*bs_map)(bus_space_tag_t, bus_addr_t, bus_size_t,
		      int, bus_space_handle_t *);
	void	  (*bs_unmap)(bus_space_tag_t, bus_space_handle_t, bus_size_t);
	int	  (*bs_subregion)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, bus_size_t, bus_space_handle_t *);

	/* allocation/deallocation */
	int	  (*bs_alloc)(bus_space_tag_t, bus_addr_t,
		      bus_addr_t, bus_size_t, bus_size_t, bus_size_t,
		      int, bus_addr_t *, bus_space_handle_t *);
	void	  (*bs_free)(bus_space_tag_t, bus_space_handle_t, bus_size_t);

	/* get kernel virtual address */
	void *	  (*bs_vaddr)(bus_space_tag_t, bus_space_handle_t);

	/* mmap bus space for user */
	paddr_t	  (*bs_mmap)(bus_space_tag_t, bus_addr_t, off_t, int, int);

	/* barrier */
	void	  (*bs_barrier)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, bus_size_t, int);

	/* probe */
	int	  (*bs_peek)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, size_t, void *);
	int	  (*bs_poke)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, size_t, u_int32_t);

	/* read (single) */
	u_int8_t  (*bs_r_1)(bus_space_tag_t, bus_space_handle_t, bus_size_t);
	u_int16_t (*bs_r_2)(bus_space_tag_t, bus_space_handle_t, bus_size_t);
	u_int32_t (*bs_r_4)(bus_space_tag_t, bus_space_handle_t, bus_size_t);
	u_int64_t (*bs_r_8)(bus_space_tag_t, bus_space_handle_t, bus_size_t);

	/* read multiple */
	void	  (*bs_rm_1)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int8_t *, bus_size_t);
	void	  (*bs_rm_2)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int16_t *, bus_size_t);
	void	  (*bs_rm_4)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int32_t *, bus_size_t);
	void	  (*bs_rm_8)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int64_t *, bus_size_t);
					
	/* read region */
	void	  (*bs_rr_1)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int8_t *, bus_size_t);
	void	  (*bs_rr_2)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int16_t *, bus_size_t);
	void	  (*bs_rr_4)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int32_t *, bus_size_t);
	void	  (*bs_rr_8)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int64_t *, bus_size_t);
					
	/* write (single) */
	void	  (*bs_w_1)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int8_t);
	void	  (*bs_w_2)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int16_t);
	void	  (*bs_w_4)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int32_t);
	void	  (*bs_w_8)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int64_t);

	/* write multiple */
	void	  (*bs_wm_1)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int8_t *, bus_size_t);
	void	  (*bs_wm_2)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int16_t *, bus_size_t);
	void	  (*bs_wm_4)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int32_t *, bus_size_t);
	void	  (*bs_wm_8)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int64_t *, bus_size_t);
					
	/* write region */
	void	  (*bs_wr_1)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int8_t *, bus_size_t);
	void	  (*bs_wr_2)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int16_t *, bus_size_t);
	void	  (*bs_wr_4)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int32_t *, bus_size_t);
	void	  (*bs_wr_8)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int64_t *, bus_size_t);

#ifdef BUS_SPACE_HAS_REAL_STREAM_METHODS
	/* read (single) stream */
	u_int8_t  (*bs_rs_1)(bus_space_tag_t, bus_space_handle_t, bus_size_t);
	u_int16_t (*bs_rs_2)(bus_space_tag_t, bus_space_handle_t, bus_size_t);
	u_int32_t (*bs_rs_4)(bus_space_tag_t, bus_space_handle_t, bus_size_t);
	u_int64_t (*bs_rs_8)(bus_space_tag_t, bus_space_handle_t, bus_size_t);

	/* read multiple stream */
	void	  (*bs_rms_1)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int8_t *, bus_size_t);
	void	  (*bs_rms_2)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int16_t *, bus_size_t);
	void	  (*bs_rms_4)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int32_t *, bus_size_t);
	void	  (*bs_rms_8)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int64_t *, bus_size_t);
					
	/* read region stream */
	void	  (*bs_rrs_1)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int8_t *, bus_size_t);
	void	  (*bs_rrs_2)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int16_t *, bus_size_t);
	void	  (*bs_rrs_4)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int32_t *, bus_size_t);
	void	  (*bs_rrs_8)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int64_t *, bus_size_t);
					
	/* write (single) stream */
	void	  (*bs_ws_1)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int8_t);
	void	  (*bs_ws_2)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int16_t);
	void	  (*bs_ws_4)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int32_t);
	void	  (*bs_ws_8)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int64_t);

	/* write multiple stream */
	void	  (*bs_wms_1)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int8_t *, bus_size_t);
	void	  (*bs_wms_2)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int16_t *, bus_size_t);
	void	  (*bs_wms_4)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int32_t *, bus_size_t);
	void	  (*bs_wms_8)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int64_t *, bus_size_t);
					
	/* write region stream */
	void	  (*bs_wrs_1)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int8_t *, bus_size_t);
	void	  (*bs_wrs_2)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int16_t *, bus_size_t);
	void	  (*bs_wrs_4)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int32_t *, bus_size_t);
	void	  (*bs_wrs_8)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, const u_int64_t *, bus_size_t);
#endif /* BUS_SPACE_HAS_REAL_STREAM_METHODS */

	/* set multiple */
	void	  (*bs_sm_1)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int8_t, bus_size_t);
	void	  (*bs_sm_2)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int16_t, bus_size_t);
	void	  (*bs_sm_4)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int32_t, bus_size_t);
	void	  (*bs_sm_8)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int64_t, bus_size_t);

	/* set region */
	void	  (*bs_sr_1)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int8_t, bus_size_t);
	void	  (*bs_sr_2)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int16_t, bus_size_t);
	void	  (*bs_sr_4)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int32_t, bus_size_t);
	void	  (*bs_sr_8)(bus_space_tag_t, bus_space_handle_t,
		      bus_size_t, u_int64_t, bus_size_t);

	/* copy */
	void	  (*bs_c_1)(bus_space_tag_t, bus_space_handle_t, bus_size_t,
		      bus_space_handle_t, bus_size_t, bus_size_t);
	void	  (*bs_c_2)(bus_space_tag_t, bus_space_handle_t, bus_size_t,
		      bus_space_handle_t, bus_size_t, bus_size_t);
	void	  (*bs_c_4)(bus_space_tag_t, bus_space_handle_t, bus_size_t,
		      bus_space_handle_t, bus_size_t, bus_size_t);
	void	  (*bs_c_8)(bus_space_tag_t, bus_space_handle_t, bus_size_t,
		      bus_space_handle_t, bus_size_t, bus_size_t);
};


/*
 * Utility macros; INTERNAL USE ONLY.
 */
#define	__bs_c(a,b)		__CONCAT(a,b)
#define	__bs_opname(op,s)	__bs_c(__bs_c(__bs_c(bs_,op),_),s)
#define	__bs_popname(pfx,op,s)	__bs_c(pfx,__bs_c(_,__bs_opname(op,s)))
#define	__bs_ops(t)		(((bus_space_tag_t)(t))->bs_ops)

#define	__bs_rs(sz, t, h, o)						\
	(*__bs_ops(t).__bs_opname(r,sz))(t, h, o)
#define	__bs_ws(sz, t, h, o, v)						\
	(*__bs_ops(t).__bs_opname(w,sz))(t, h, o, v)
#ifdef BUS_SPACE_HAS_REAL_STREAM_METHODS
#define	__bs_rss(sz, t, h, o)						\
	(*__bs_ops(t).__bs_opname(rs,sz))(t, h, o)
#define	__bs_wss(sz, t, h, o, v)					\
	(*__bs_ops(t).__bs_opname(ws,sz))(t, h, o, v)
#endif /* BUS_SPACE_HAS_REAL_STREAM_METHODS */
#define	__bs_nonsingle(type, sz, t, h, o, a, c)				\
	(*__bs_ops(t).__bs_opname(type,sz))(t, h, o, a, c)
#define	__bs_set(type, sz, t, h, o, v, c)				\
	(*__bs_ops(t).__bs_opname(type,sz))(t, h, o, v, c)
#define	__bs_copy(sz, t, h1, o1, h2, o2, cnt)				\
	(*__bs_ops(t).__bs_opname(c,sz))(t, h1, o1, h2, o2, cnt)


/*
 * Mapping and unmapping operations.
 */
#define	bus_space_map(t, a, s, c, hp)					\
	(*__bs_ops(t).bs_map)(t, (a), (s), (c), (hp))
#define	bus_space_unmap(t, h, s)					\
	(*__bs_ops(t).bs_unmap)(t, (h), (s))
#define	bus_space_subregion(t, h, o, s, hp)				\
	(*__bs_ops(t).bs_subregion)(t, (h), (o), (s), (hp))


/*
 * Allocation and deallocation operations.
 */
#define	bus_space_alloc(t, rs, re, s, a, b, c, ap, hp)			\
	(*__bs_ops(t).bs_alloc)(t, (rs), (re), (s), (a), (b), (c), (ap), (hp))
#define	bus_space_free(t, h, s)						\
	(*__bs_ops(t).bs_free)(t, (h), (s))


/*
 * Get kernel virtual address for ranges mapped BUS_SPACE_MAP_LINEAR.
 */
#define	bus_space_vaddr(t, h)						\
	(*__bs_ops(t).bs_vaddr)(t, (h))


/*
 * MMap bus space for a user application.
 */
#define bus_space_mmap(t, a, o, p, f)					\
	(*__bs_ops(t).bs_mmap)(t, (a), (o), (p), (f))


/*
 * Bus barrier operations.
 */
#define	bus_space_barrier(t, h, o, l, f)				\
	(*__bs_ops(t).bs_barrier)(t, (h), (o), (l), (f))


/*
 * Bus probe operations.
 */
#define	bus_space_peek(t, h, o, s, p)					\
	(*__bs_ops(t).bs_peek)(t, (h), (o), (s), (p))
#define	bus_space_poke(t, h, o, s, v)					\
	(*__bs_ops(t).bs_poke)(t, (h), (o), (s), (v))


/*
 * Bus read (single) operations.
 */
#define	bus_space_read_1(t, h, o)	__bs_rs(1,(t),(h),(o))
#define	bus_space_read_2(t, h, o)	__bs_rs(2,(t),(h),(o))
#define	bus_space_read_4(t, h, o)	__bs_rs(4,(t),(h),(o))
#define	bus_space_read_8(t, h, o)	__bs_rs(8,(t),(h),(o))


/*
 * Bus read multiple operations.
 */
#define	bus_space_read_multi_1(t, h, o, a, c)				\
	__bs_nonsingle(rm,1,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_2(t, h, o, a, c)				\
	__bs_nonsingle(rm,2,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_4(t, h, o, a, c)				\
	__bs_nonsingle(rm,4,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_8(t, h, o, a, c)				\
	__bs_nonsingle(rm,8,(t),(h),(o),(a),(c))


/*
 * Bus read region operations.
 */
#define	bus_space_read_region_1(t, h, o, a, c)				\
	__bs_nonsingle(rr,1,(t),(h),(o),(a),(c))
#define	bus_space_read_region_2(t, h, o, a, c)				\
	__bs_nonsingle(rr,2,(t),(h),(o),(a),(c))
#define	bus_space_read_region_4(t, h, o, a, c)				\
	__bs_nonsingle(rr,4,(t),(h),(o),(a),(c))
#define	bus_space_read_region_8(t, h, o, a, c)				\
	__bs_nonsingle(rr,8,(t),(h),(o),(a),(c))


/*
 * Bus write (single) operations.
 */
#define	bus_space_write_1(t, h, o, v)	__bs_ws(1,(t),(h),(o),(v))
#define	bus_space_write_2(t, h, o, v)	__bs_ws(2,(t),(h),(o),(v))
#define	bus_space_write_4(t, h, o, v)	__bs_ws(4,(t),(h),(o),(v))
#define	bus_space_write_8(t, h, o, v)	__bs_ws(8,(t),(h),(o),(v))


/*
 * Bus write multiple operations.
 */
#define	bus_space_write_multi_1(t, h, o, a, c)				\
	__bs_nonsingle(wm,1,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_2(t, h, o, a, c)				\
	__bs_nonsingle(wm,2,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_4(t, h, o, a, c)				\
	__bs_nonsingle(wm,4,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_8(t, h, o, a, c)				\
	__bs_nonsingle(wm,8,(t),(h),(o),(a),(c))


/*
 * Bus write region operations.
 */
#define	bus_space_write_region_1(t, h, o, a, c)				\
	__bs_nonsingle(wr,1,(t),(h),(o),(a),(c))
#define	bus_space_write_region_2(t, h, o, a, c)				\
	__bs_nonsingle(wr,2,(t),(h),(o),(a),(c))
#define	bus_space_write_region_4(t, h, o, a, c)				\
	__bs_nonsingle(wr,4,(t),(h),(o),(a),(c))
#define	bus_space_write_region_8(t, h, o, a, c)				\
	__bs_nonsingle(wr,8,(t),(h),(o),(a),(c))


#ifdef BUS_SPACE_HAS_REAL_STREAM_METHODS
/*
 * Bus read (single) stream operations.
 */
#define	bus_space_read_stream_1(t, h, o)	__bs_rss(1,(t),(h),(o))
#define	bus_space_read_stream_2(t, h, o)	__bs_rss(2,(t),(h),(o))
#define	bus_space_read_stream_4(t, h, o)	__bs_rss(4,(t),(h),(o))
#define	bus_space_read_stream_8(t, h, o)	__bs_rss(8,(t),(h),(o))


/*
 * Bus read multiple operations.
 */
#define	bus_space_read_multi_stream_1(t, h, o, a, c)			\
	__bs_nonsingle(rms,1,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_stream_2(t, h, o, a, c)			\
	__bs_nonsingle(rms,2,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_stream_4(t, h, o, a, c)			\
	__bs_nonsingle(rms,4,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_stream_8(t, h, o, a, c)			\
	__bs_nonsingle(rms,8,(t),(h),(o),(a),(c))


/*
 * Bus read region operations.
 */
#define	bus_space_read_region_stream_1(t, h, o, a, c)			\
	__bs_nonsingle(rrs,1,(t),(h),(o),(a),(c))
#define	bus_space_read_region_stream_2(t, h, o, a, c)			\
	__bs_nonsingle(rrs,2,(t),(h),(o),(a),(c))
#define	bus_space_read_region_stream_4(t, h, o, a, c)			\
	__bs_nonsingle(rrs,4,(t),(h),(o),(a),(c))
#define	bus_space_read_region_stream_8(t, h, o, a, c)			\
	__bs_nonsingle(rrs,8,(t),(h),(o),(a),(c))


/*
 * Bus write (single) operations.
 */
#define	bus_space_write_stream_1(t, h, o, v)	__bs_wss(1,(t),(h),(o),(v))
#define	bus_space_write_stream_2(t, h, o, v)	__bs_wss(2,(t),(h),(o),(v))
#define	bus_space_write_stream_4(t, h, o, v)	__bs_wss(4,(t),(h),(o),(v))
#define	bus_space_write_stream_8(t, h, o, v)	__bs_wss(8,(t),(h),(o),(v))


/*
 * Bus write multiple operations.
 */
#define	bus_space_write_multi_stream_1(t, h, o, a, c)			\
	__bs_nonsingle(wms,1,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_stream_2(t, h, o, a, c)			\
	__bs_nonsingle(wms,2,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_stream_4(t, h, o, a, c)			\
	__bs_nonsingle(wms,4,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_stream_8(t, h, o, a, c)			\
	__bs_nonsingle(wms,8,(t),(h),(o),(a),(c))


/*
 * Bus write region operations.
 */
#define	bus_space_write_region_stream_1(t, h, o, a, c)			\
	__bs_nonsingle(wrs,1,(t),(h),(o),(a),(c))
#define	bus_space_write_region_stream_2(t, h, o, a, c)			\
	__bs_nonsingle(wrs,2,(t),(h),(o),(a),(c))
#define	bus_space_write_region_stream_4(t, h, o, a, c)			\
	__bs_nonsingle(wrs,4,(t),(h),(o),(a),(c))
#define	bus_space_write_region_stream_8(t, h, o, a, c)			\
	__bs_nonsingle(wrs,8,(t),(h),(o),(a),(c))
#else
#ifdef __BUS_SPACE_HAS_STREAM_METHODS
#define bus_space_read_stream_1 bus_space_read_1
#define bus_space_read_stream_2 bus_space_read_2
#define bus_space_read_stream_4 bus_space_read_4
#define	bus_space_read_stream_8 bus_space_read_8
#define bus_space_read_multi_stream_1 bus_space_read_multi_1
#define bus_space_read_multi_stream_2 bus_space_read_multi_2
#define bus_space_read_multi_stream_4 bus_space_read_multi_4
#define	bus_space_read_multi_stream_8 bus_space_read_multi_8
#define bus_space_read_region_stream_1 bus_space_read_region_1
#define bus_space_read_region_stream_2 bus_space_read_region_2
#define bus_space_read_region_stream_4 bus_space_read_region_4
#define	bus_space_read_region_stream_8 bus_space_read_region_8
#define bus_space_write_stream_1 bus_space_write_1
#define bus_space_write_stream_2 bus_space_write_2
#define bus_space_write_stream_4 bus_space_write_4
#define	bus_space_write_stream_8 bus_space_write_8
#define bus_space_write_multi_stream_1 bus_space_write_multi_1
#define bus_space_write_multi_stream_2 bus_space_write_multi_2
#define bus_space_write_multi_stream_4 bus_space_write_multi_4
#define	bus_space_write_multi_stream_8 bus_space_write_multi_8
#define bus_space_write_region_stream_1 bus_space_write_region_1
#define bus_space_write_region_stream_2 bus_space_write_region_2
#define bus_space_write_region_stream_4 bus_space_write_region_4
#define	bus_space_write_region_stream_8	bus_space_write_region_8
#endif /* __BUS_SPACE_HAS_STREAM_METHODS */
#endif /* BUS_SPACE_HAS_REAL_STREAM_METHODS */


/*
 * Set multiple operations.
 */
#define	bus_space_set_multi_1(t, h, o, v, c)				\
	__bs_set(sm,1,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_2(t, h, o, v, c)				\
	__bs_set(sm,2,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_4(t, h, o, v, c)				\
	__bs_set(sm,4,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_8(t, h, o, v, c)				\
	__bs_set(sm,8,(t),(h),(o),(v),(c))


/*
 * Set region operations.
 */
#define	bus_space_set_region_1(t, h, o, v, c)				\
	__bs_set(sr,1,(t),(h),(o),(v),(c))
#define	bus_space_set_region_2(t, h, o, v, c)				\
	__bs_set(sr,2,(t),(h),(o),(v),(c))
#define	bus_space_set_region_4(t, h, o, v, c)				\
	__bs_set(sr,4,(t),(h),(o),(v),(c))
#define	bus_space_set_region_8(t, h, o, v, c)				\
	__bs_set(sr,8,(t),(h),(o),(v),(c))


/*
 * Copy operations.
 */
#define	bus_space_copy_region_1(t, h1, o1, h2, o2, c)			\
	__bs_copy(1, t, h1, o1, h2, o2, c)
#define	bus_space_copy_region_2(t, h1, o1, h2, o2, c)			\
	__bs_copy(2, t, h1, o1, h2, o2, c)
#define	bus_space_copy_region_4(t, h1, o1, h2, o2, c)			\
	__bs_copy(4, t, h1, o1, h2, o2, c)
#define	bus_space_copy_region_8(t, h1, o1, h2, o2, c)			\
	__bs_copy(8, t, h1, o1, h2, o2, c)


/*
 * Macros to provide prototypes for all the functions used in the
 * bus_space structure
 */
#define bs_map_proto(f)							\
int	__bs_c(f,_bs_map)(bus_space_tag_t t, bus_addr_t addr,		\
	    bus_size_t size, int cacheable, bus_space_handle_t *bshp)

#define bs_unmap_proto(f)						\
void	__bs_c(f,_bs_unmap)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t size)

#define bs_subregion_proto(f)						\
int	__bs_c(f,_bs_subregion)(bus_space_tag_t t,			\
	    bus_space_handle_t bsh, bus_size_t offset,			\
	    bus_size_t size, bus_space_handle_t *nbshp)

#define bs_alloc_proto(f)						\
int	__bs_c(f,_bs_alloc)(bus_space_tag_t t, bus_addr_t rstart,	\
	    bus_addr_t rend, bus_size_t size, bus_size_t align,		\
	    bus_size_t boundary, int cacheable, bus_addr_t *addrp,	\
	    bus_space_handle_t *bshp)

#define bs_free_proto(f)						\
void	__bs_c(f,_bs_free)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t size)

#define bs_vaddr_proto(f)						\
void *	__bs_c(f,_bs_vaddr)(bus_space_tag_t t, bus_space_handle_t bsh)

#define bs_mmap_proto(f)						\
paddr_t	__bs_c(f,_bs_mmap)(bus_space_tag_t t, bus_addr_t addr,		\
	    off_t offset, int prot, int flags)

#define bs_barrier_proto(f)						\
void	__bs_c(f,_bs_barrier)(bus_space_tag_t t, bus_space_handle_t bsh,\
	    bus_size_t offset, bus_size_t len, int flags)

#define bs_peek_proto(f)						\
int	__bs_c(f,_bs_peek)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, size_t len, void *ptr)
#define bs_poke_proto(f)						\
int	__bs_c(f,_bs_poke)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, size_t len, u_int32_t val)

#define	bs_r_1_proto(f)							\
u_int8_t	__bs_c(f,_bs_r_1)(bus_space_tag_t t,			\
		    bus_space_handle_t bsh, bus_size_t offset)
#define	bs_r_2_proto(f)							\
u_int16_t	__bs_c(f,_bs_r_2)(bus_space_tag_t t,			\
		    bus_space_handle_t bsh, bus_size_t offset)
#define	bs_r_4_proto(f)							\
u_int32_t	__bs_c(f,_bs_r_4)(bus_space_tag_t t,			\
		    bus_space_handle_t bsh, bus_size_t offset)
#define	bs_r_8_proto(f)							\
u_int64_t	__bs_c(f,_bs_r_8)(bus_space_tag_t t,			\
		    bus_space_handle_t bsh, bus_size_t offset)

#define	bs_rm_1_proto(f)						\
void	__bs_c(f,_bs_rm_1)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int8_t *addr,	bus_size_t count)
#define	bs_rm_2_proto(f)						\
void	__bs_c(f,_bs_rm_2)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int16_t *addr,	bus_size_t count)
#define	bs_rm_4_proto(f)						\
void	__bs_c(f,_bs_rm_4)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int32_t *addr,	bus_size_t count)
#define	bs_rm_8_proto(f)						\
void	__bs_c(f,_bs_rm_8)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int64_t *addr,	bus_size_t count)

#define	bs_rr_1_proto(f)						\
void	__bs_c(f,_bs_rr_1)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, u_int8_t *addr, bus_size_t count)
#define	bs_rr_2_proto(f)						\
void	__bs_c(f,_bs_rr_2)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, u_int16_t *addr, bus_size_t count)
#define	bs_rr_4_proto(f)						\
void	__bs_c(f,_bs_rr_4)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, u_int32_t *addr, bus_size_t count)
#define	bs_rr_8_proto(f)						\
void	__bs_c(f,_bs_rr_8)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, u_int64_t *addr, bus_size_t count)

#define	bs_w_1_proto(f)							\
void	__bs_c(f,_bs_w_1)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int8_t value)
#define	bs_w_2_proto(f)							\
void	__bs_c(f,_bs_w_2)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int16_t value)
#define	bs_w_4_proto(f)							\
void	__bs_c(f,_bs_w_4)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int32_t value)
#define	bs_w_8_proto(f)							\
void	__bs_c(f,_bs_w_8)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int64_t value)

#define	bs_wm_1_proto(f)						\
void	__bs_c(f,_bs_wm_1)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, const u_int8_t *addr, bus_size_t count)
#define	bs_wm_2_proto(f)						\
void	__bs_c(f,_bs_wm_2)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, const u_int16_t *addr, bus_size_t count)
#define	bs_wm_4_proto(f)						\
void	__bs_c(f,_bs_wm_4)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, const u_int32_t *addr, bus_size_t count)
#define	bs_wm_8_proto(f)						\
void	__bs_c(f,_bs_wm_8)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, const u_int64_t *addr, bus_size_t count)

#define	bs_wr_1_proto(f)						\
void	__bs_c(f,_bs_wr_1)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, const u_int8_t *addr, bus_size_t count)
#define	bs_wr_2_proto(f)						\
void	__bs_c(f,_bs_wr_2)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, const u_int16_t *addr, bus_size_t count)
#define	bs_wr_4_proto(f)						\
void	__bs_c(f,_bs_wr_4)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, const u_int32_t *addr, bus_size_t count)
#define	bs_wr_8_proto(f)						\
void	__bs_c(f,_bs_wr_8)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, const u_int64_t *addr, bus_size_t count)

#ifdef BUS_SPACE_HAS_REAL_STREAM_METHODS
#define	bs_rs_1_proto(f)						\
u_int8_t	__bs_c(f,_bs_rs_1)(bus_space_tag_t t,			\
		    bus_space_handle_t bsh, bus_size_t offset)
#define	bs_rs_2_proto(f)						\
u_int16_t	__bs_c(f,_bs_rs_2)(bus_space_tag_t t,			\
		    bus_space_handle_t bsh, bus_size_t offset)
#define	bs_rs_4_proto(f)						\
u_int32_t	__bs_c(f,_bs_rs_4)(bus_space_tag_t t,			\
		    bus_space_handle_t bsh, bus_size_t offset)
#define	bs_rs_8_proto(f)						\
u_int64_t	__bs_c(f,_bs_rs_8)(bus_space_tag_t t,			\
		    bus_space_handle_t bsh, bus_size_t offset)

#define	bs_rms_1_proto(f)						\
void	__bs_c(f,_bs_rms_1)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int8_t *addr,	bus_size_t count)
#define	bs_rms_2_proto(f)						\
void	__bs_c(f,_bs_rms_1)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int16_t *addr,	bus_size_t count)
#define	bs_rms_4_proto(f)						\
void	__bs_c(f,_bs_rms_4)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int32_t *addr,	bus_size_t count)
#define	bs_rms_8_proto(f)						\
void	__bs_c(f,_bs_rms_8)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int64_t *addr,	bus_size_t count)

#define	bs_rrs_1_proto(f)						\
void	__bs_c(f,_bs_rrs_1)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, u_int8_t *addr, bus_size_t count)
#define	bs_rrs_2_proto(f)						\
void	__bs_c(f,_bs_rrs_2)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, u_int16_t *addr, bus_size_t count)
#define	bs_rrs_4_proto(f)						\
void	__bs_c(f,_bs_rrs_4)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, u_int32_t *addr, bus_size_t count)
#define	bs_rrs_8_proto(f)						\
void	__bs_c(f,_bs_rrs_8)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, u_int64_t *addr, bus_size_t count)

#define	bs_ws_1_proto(f)						\
void	__bs_c(f,_bs_ws_1)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int8_t value)
#define	bs_ws_2_proto(f)						\
void	__bs_c(f,_bs_ws_2)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int16_t value)
#define	bs_ws_4_proto(f)						\
void	__bs_c(f,_bs_ws_4)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int32_t value)
#define	bs_ws_8_proto(f)						\
void	__bs_c(f,_bs_ws_8)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int64_t value)

#define	bs_wms_1_proto(f)						\
void	__bs_c(f,_bs_wms_1)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, const u_int8_t *addr, bus_size_t count)
#define	bs_wms_2_proto(f)						\
void	__bs_c(f,_bs_wms_2)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, const u_int16_t *addr, bus_size_t count)
#define	bs_wms_4_proto(f)						\
void	__bs_c(f,_bs_wms_4)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, const u_int32_t *addr, bus_size_t count)
#define	bs_wms_8_proto(f)						\
void	__bs_c(f,_bs_wms_8)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	   bus_size_t offset, const u_int64_t *addr, bus_size_t count)

#define	bs_wrs_1_proto(f)						\
void	__bs_c(f,_bs_wrs_1)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, const u_int8_t *addr, bus_size_t count)
#define	bs_wrs_2_proto(f)						\
void	__bs_c(f,_bs_wrs_2)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, const u_int16_t *addr, bus_size_t count)
#define	bs_wrs_4_proto(f)						\
void	__bs_c(f,_bs_wrs_4)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, const u_int32_t *addr, bus_size_t count)
#define	bs_wrs_8_proto(f)						\
void	__bs_c(f,_bs_wrs_8)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, const u_int64_t *addr, bus_size_t count)
#else /* BUS_SPACE_HAS_REAL_STREAM_METHODS */
#define	bs_rs_1_proto(f)
#define	bs_rs_2_proto(f)
#define	bs_rs_4_proto(f)
#define	bs_rs_8_proto(f)
#define	bs_rms_1_proto(f)
#define	bs_rms_2_proto(f)
#define	bs_rms_4_proto(f)
#define	bs_rms_8_proto(f)
#define	bs_rrs_1_proto(f)
#define	bs_rrs_2_proto(f)
#define	bs_rrs_4_proto(f)
#define	bs_rrs_8_proto(f)
#define	bs_ws_1_proto(f)
#define	bs_ws_2_proto(f)
#define	bs_ws_4_proto(f)
#define	bs_ws_8_proto(f)
#define	bs_wms_1_proto(f)
#define	bs_wms_2_proto(f)
#define	bs_wms_4_proto(f)
#define	bs_wms_8_proto(f)
#define	bs_wrs_1_proto(f)
#define	bs_wrs_2_proto(f)
#define	bs_wrs_4_proto(f)
#define	bs_wrs_8_proto(f)
#endif /* ! BUS_SPACE_HAS_REAL_STREAM_METHODS */

#define	bs_sm_1_proto(f)						\
void	__bs_c(f,_bs_sm_1)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int8_t value, bus_size_t count)
#define	bs_sm_2_proto(f)						\
void	__bs_c(f,_bs_sm_2)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int16_t value, bus_size_t count)
#define	bs_sm_4_proto(f)						\
void	__bs_c(f,_bs_sm_4)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int32_t value, bus_size_t count)
#define	bs_sm_8_proto(f)						\
void	__bs_c(f,_bs_sm_8)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int64_t value, bus_size_t count)

#define	bs_sr_1_proto(f)						\
void	__bs_c(f,_bs_sr_1)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int8_t value, bus_size_t count)
#define	bs_sr_2_proto(f)						\
void	__bs_c(f,_bs_sr_2)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int16_t value, bus_size_t count)
#define	bs_sr_4_proto(f)						\
void	__bs_c(f,_bs_sr_4)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int32_t value, bus_size_t count)
#define	bs_sr_8_proto(f)						\
void	__bs_c(f,_bs_sr_8)(bus_space_tag_t t, bus_space_handle_t bsh,	\
	    bus_size_t offset, u_int64_t value, bus_size_t count)

#define	bs_c_1_proto(f)							\
void	__bs_c(f,_bs_c_1)(bus_space_tag_t t, bus_space_handle_t bsh1,	\
	    bus_size_t offset1, bus_space_handle_t bsh2,		\
	    bus_size_t offset2, bus_size_t count)
#define	bs_c_2_proto(f)							\
void	__bs_c(f,_bs_c_2)(bus_space_tag_t t, bus_space_handle_t bsh1,	\
	    bus_size_t offset1, bus_space_handle_t bsh2,		\
	    bus_size_t offset2, bus_size_t count)
#define	bs_c_4_proto(f)							\
void	__bs_c(f,_bs_c_4)(bus_space_tag_t t, bus_space_handle_t bsh1,	\
	    bus_size_t offset1, bus_space_handle_t bsh2,		\
	    bus_size_t offset2, bus_size_t count)
#define	bs_c_8_proto(f)							\
void	__bs_c(f,_bs_c_8)(bus_space_tag_t t, bus_space_handle_t bsh1,	\
	    bus_size_t offset1, bus_space_handle_t bsh2,		\
	    bus_size_t offset2, bus_size_t count)


#define bus_space_protos(f)	\
bs_map_proto(f);		\
bs_unmap_proto(f);		\
bs_subregion_proto(f);		\
bs_alloc_proto(f);		\
bs_free_proto(f);		\
bs_vaddr_proto(f);		\
bs_mmap_proto(f);		\
bs_barrier_proto(f);		\
bs_peek_proto(f);		\
bs_poke_proto(f);		\
bs_r_1_proto(f);		\
bs_r_2_proto(f);		\
bs_r_4_proto(f);		\
bs_r_8_proto(f);		\
bs_rm_1_proto(f);		\
bs_rm_2_proto(f);		\
bs_rm_4_proto(f);		\
bs_rm_8_proto(f);		\
bs_rr_1_proto(f);		\
bs_rr_2_proto(f);		\
bs_rr_4_proto(f);		\
bs_rr_8_proto(f);		\
bs_w_1_proto(f);		\
bs_w_2_proto(f);		\
bs_w_4_proto(f);		\
bs_w_8_proto(f);		\
bs_wm_1_proto(f);		\
bs_wm_2_proto(f);		\
bs_wm_4_proto(f);		\
bs_wm_8_proto(f);		\
bs_wr_1_proto(f);		\
bs_wr_2_proto(f);		\
bs_wr_4_proto(f);		\
bs_wr_8_proto(f);		\
bs_rs_1_proto(f);		\
bs_rs_2_proto(f);		\
bs_rs_4_proto(f);		\
bs_rs_8_proto(f);		\
bs_rms_1_proto(f);		\
bs_rms_2_proto(f);		\
bs_rms_4_proto(f);		\
bs_rms_8_proto(f);		\
bs_rrs_1_proto(f);		\
bs_rrs_2_proto(f);		\
bs_rrs_4_proto(f);		\
bs_rrs_8_proto(f);		\
bs_ws_1_proto(f);		\
bs_ws_2_proto(f);		\
bs_ws_4_proto(f);		\
bs_ws_8_proto(f);		\
bs_wms_1_proto(f);		\
bs_wms_2_proto(f);		\
bs_wms_4_proto(f);		\
bs_wms_8_proto(f);		\
bs_wrs_1_proto(f);		\
bs_wrs_2_proto(f);		\
bs_wrs_4_proto(f);		\
bs_wrs_8_proto(f);		\
bs_sm_1_proto(f);		\
bs_sm_2_proto(f);		\
bs_sm_4_proto(f);		\
bs_sm_8_proto(f);		\
bs_sr_1_proto(f);		\
bs_sr_2_proto(f);		\
bs_sr_4_proto(f);		\
bs_sr_8_proto(f);		\
bs_c_1_proto(f);		\
bs_c_2_proto(f);		\
bs_c_4_proto(f);		\
bs_c_8_proto(f);

#endif /* ! BUS_SPACE_MD_CALLS */

#ifndef BUS_SPACE_MD_TYPES
#ifdef BUS_SPACE_MD_CALLS
typedef struct bus_space *bus_space_tag_t;
#endif /* BUS_SPACE_MD_CALLS */

/*
 *	bus_space_tag_t
 *
 *	bus space tag structure
 */
struct bus_space_tag {
	bus_space_tag_t bs_base;
	struct bus_space_ops bs_ops;
};
#endif /* ! BUS_SPACE_MD_TYPES */

#ifndef BUS_DMA_MD_CALLS

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
 * Operations performed by bus_dmamap_sync().
 */
#define	BUS_DMASYNC_PREREAD	0x01	/* pre-read synchronization */
#define	BUS_DMASYNC_POSTREAD	0x02	/* post-read synchronization */
#define	BUS_DMASYNC_PREWRITE	0x04	/* pre-write synchronization */
#define	BUS_DMASYNC_POSTWRITE	0x08	/* post-write synchronization */

/* Forwards needed by prototypes below. */
struct mbuf;
struct uio;

#ifndef BUS_DMA_MD_TYPES
typedef struct bus_dma_tag	*bus_dma_tag_t;
typedef struct bus_dma_segment	bus_dma_segment_t;
typedef struct bus_dmamap	*bus_dmamap_t;
#endif /* ! BUS_DMA_MD_TYPES */

/*
 * bus DMA operaion table
 */
struct bus_dma_ops {
	/*
	 * DMA mapping methods.
	 */
	int	(*bd_map_create)(bus_dma_tag_t, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *);
	void	(*bd_map_destroy)(bus_dma_tag_t, bus_dmamap_t);
	int	(*bd_map_load)(bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int);
	int	(*bd_map_load_mbuf)(bus_dma_tag_t, bus_dmamap_t,
		    struct mbuf *, int);
	int	(*bd_map_load_uio)(bus_dma_tag_t, bus_dmamap_t,
		    struct uio *, int);
	int	(*bd_map_load_raw)(bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int);
	void	(*bd_map_unload)(bus_dma_tag_t, bus_dmamap_t);
	void	(*bd_map_sync)(bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int);

	/*
	 * DMA memory utility functions.
	 */
	int	(*bd_mem_alloc)(bus_dma_tag_t, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int);
	void	(*bd_mem_free)(bus_dma_tag_t,
		    bus_dma_segment_t *, int);
	int	(*bd_mem_map)(bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, caddr_t *, int);
	void	(*bd_mem_unmap)(bus_dma_tag_t, caddr_t, size_t);
	paddr_t	(*bd_mem_mmap)(bus_dma_tag_t, bus_dma_segment_t *,
		    int, off_t, int, int);
};


/*
 * bus DMA methods
 */
#define	__bd_ops(t)	(((bus_dma_tag_t)(t))->bd_ops)
#define	bus_dmamap_create(t, s, n, m, b, f, p)				\
	(*__bd_ops(t).bd_map_create)((t), (s), (n), (m), (b), (f), (p))
#define	bus_dmamap_destroy(t, p)					\
	(*__bd_ops(t).bd_map_destroy)((t), (p))
#define	bus_dmamap_load(t, m, b, s, p, f)				\
	(*__bd_ops(t).bd_map_load)((t), (m), (b), (s), (p), (f))
#define	bus_dmamap_load_mbuf(t, m, b, f)				\
	(*__bd_ops(t).bd_map_load_mbuf)((t), (m), (b), (f))
#define	bus_dmamap_load_uio(t, m, u, f)					\
	(*__bd_ops(t).bd_map_load_uio)((t), (m), (u), (f))
#define	bus_dmamap_load_raw(t, m, sg, n, s, f)				\
	(*__bd_ops(t).bd_map_load_raw)((t), (m), (sg), (n), (s), (f))
#define	bus_dmamap_unload(t, p)						\
	(*__bd_ops(t).bd_map_unload)((t), (p))
#define	bus_dmamap_sync(t, p, o, l, ops)				\
	(void)(__bd_ops(t).bd_map_sync ?				\
	    (*__bd_ops(t).bd_map_sync)((t), (p), (o), (l), (ops)) : (void)0)

#define	bus_dmamem_alloc(t, s, a, b, sg, n, r, f)			\
	(*__bd_ops(t).bd_mem_alloc)((t), (s), (a), (b), (sg), (n), (r), (f))
#define	bus_dmamem_free(t, sg, n)					\
	(*__bd_ops(t).bd_mem_free)((t), (sg), (n))
#define	bus_dmamem_map(t, sg, n, s, k, f)				\
	(*__bd_ops(t).bd_mem_map)((t), (sg), (n), (s), (k), (f))
#define	bus_dmamem_unmap(t, k, s)					\
	(*__bd_ops(t).bd_mem_unmap)((t), (k), (s))
#define	bus_dmamem_mmap(t, sg, n, o, p, f)				\
	(*__bd_ops(t).bd_mem_mmap)((t), (sg), (n), (o), (p), (f))


/*
 * Macros to provide prototypes for all the functions used in the
 * bus_dma structure
 */
#define bus_dma_protos(f)						\
int	__bs_c(f,_bd_map_create)(bus_dma_tag_t, bus_size_t, int,	\
	    bus_size_t, bus_size_t, int, bus_dmamap_t *);		\
void	__bs_c(f,_bd_map_destroy)(bus_dma_tag_t, bus_dmamap_t);		\
int	__bs_c(f,_bd_map_load)(bus_dma_tag_t, bus_dmamap_t, void *,	\
	    bus_size_t, struct proc *, int);				\
int	__bs_c(f,_bd_map_load_mbuf)(bus_dma_tag_t, bus_dmamap_t,	\
	    struct mbuf *, int);					\
int	__bs_c(f,_bd_map_load_uio)(bus_dma_tag_t, bus_dmamap_t,		\
	    struct uio *, int);						\
int	__bs_c(f,_bd_map_load_raw)(bus_dma_tag_t, bus_dmamap_t,		\
	    bus_dma_segment_t *, int, bus_size_t, int);			\
void	__bs_c(f,_bd_map_unload)(bus_dma_tag_t, bus_dmamap_t);		\
void	__bs_c(f,_bd_map_sync)(bus_dma_tag_t, bus_dmamap_t,		\
	    bus_addr_t, bus_size_t, int);				\
int	__bs_c(f,_bd_mem_alloc)(bus_dma_tag_t, bus_size_t, bus_size_t,	\
	    bus_size_t, bus_dma_segment_t *, int, int *, int);		\
void	__bs_c(f,_bd_mem_free)(bus_dma_tag_t, bus_dma_segment_t *, int);\
int	__bs_c(f,_bd_mem_map)(bus_dma_tag_t, bus_dma_segment_t *,	\
	    int, size_t, caddr_t *, int);				\
void	__bs_c(f,_bd_mem_unmap)(bus_dma_tag_t, caddr_t, size_t);	\
paddr_t	__bs_c(f,_bd_mem_mmap)(bus_dma_tag_t, bus_dma_segment_t *,	\
	    int, off_t, int, int);

#endif /* ! BUS_DMA_MD_CALLS */

#ifndef BUS_DMA_MD_TYPES
#ifdef BUS_DMA_MD_CALLS
typedef struct bus_dma_tag	*bus_dma_tag_t;
typedef struct bus_dma_segment	bus_dma_segment_t;
typedef struct bus_dmamap	*bus_dmamap_t;
#endif /* ! BUS_DMA_MD_CALLS */

/*
 *	bus_dma_tag
 *
 *	Describes a implementation of DMA for a given bus.
 */
struct bus_dma_tag {
	bus_dma_tag_t bd_base;
	struct bus_dma_ops bd_ops;
};

/*
 *	bus_dma_segment
 *
 *	Describes a single contiguous DMA transaction.
 */
struct bus_dma_segment {
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */
};

/*
 *	bus_dmamap
 *
 *	Describes a DMA mapping.
 */
struct bus_dmamap {
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t *dm_segs;	/* segments; variable length */
};

#endif /* ! BUS_DMA_MD_TYPES */

#include <machine/bus_machdep.h>

#endif /* _SYS_BUS_H_ */
