/*	$NetBSD: bus.h,v 1.15 1999/01/08 09:25:33 leo Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

#ifndef _ATARI_BUS_H_
#define _ATARI_BUS_H_

/*
 * I/O addresses (in bus space)
 */
typedef u_long bus_io_addr_t;
typedef u_long bus_io_size_t;

/*
 * Memory addresses (in bus space)
 */
typedef u_long	bus_addr_t;
typedef u_long	bus_size_t;

#define __BUS_SPACE_HAS_STREAM_METHODS

/*
 * Access methods for bus resources and address space.
 */
typedef struct atari_bus_space	*bus_space_tag_t;
typedef caddr_t			bus_space_handle_t;

#define	BUS_SPACE_MAP_CACHEABLE	0x01
#define	BUS_SPACE_MAP_LINEAR	0x02

int	bus_space_map __P((bus_space_tag_t, bus_addr_t, bus_size_t,
				int, bus_space_handle_t *));
void	bus_space_unmap __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t));
int	bus_space_subregion __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, bus_size_t, bus_space_handle_t *));

/*
 * XXX
 */
bus_space_tag_t	mb_alloc_bus_space_tag __P((void));
void		mb_free_bus_space_tag  __P((bus_space_tag_t));

/*
 * Structure containing functions and other feature-data that might differ
 * between the various bus spaces on the atari. Currently 'known' bus
 * spaces are: ISA, PCI, VME and 'mainbus'.
 */
struct atari_bus_space {
	u_long	base;

	/* XXX Next 2 lines can be turned into an opaque cookie */
	int	stride;
	int	wo_1, wo_2, wo_4, wo_8;

	/* Autoconf detection stuff */
	int		(*abs_p_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t));
	int		(*abs_p_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t));
	int		(*abs_p_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t));
	int		(*abs_p_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t));

	/* read (single) */
	u_int8_t	(*abs_r_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t));
	u_int16_t	(*abs_r_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t));
	u_int32_t	(*abs_r_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t));
	u_int64_t	(*abs_r_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t));

	/* read (single) stream */
	u_int8_t	(*abs_rs_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t));
	u_int16_t	(*abs_rs_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t));
	u_int32_t	(*abs_rs_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t));
	u_int64_t	(*abs_rs_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t));

	/* read multiple */
	void		(*abs_rm_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int8_t *, bus_size_t));
	void		(*abs_rm_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int16_t *, bus_size_t));
	void		(*abs_rm_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int32_t *, bus_size_t));
	void		(*abs_rm_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int64_t *, bus_size_t));

	/* read multiple stream */
	void		(*abs_rms_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int8_t *, bus_size_t));
	void		(*abs_rms_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int16_t *, bus_size_t));
	void		(*abs_rms_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int32_t *, bus_size_t));
	void		(*abs_rms_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int64_t *, bus_size_t));

	/* read region */
	void		(*abs_rr_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int8_t *, bus_size_t));
	void		(*abs_rr_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int16_t *, bus_size_t));
	void		(*abs_rr_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int32_t *, bus_size_t));
	void		(*abs_rr_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int64_t *, bus_size_t));

	/* read region stream */
	void		(*abs_rrs_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int8_t *, bus_size_t));
	void		(*abs_rrs_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int16_t *, bus_size_t));
	void		(*abs_rrs_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int32_t *, bus_size_t));
	void		(*abs_rrs_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int64_t *, bus_size_t));

	/* write (single) */
	void		(*abs_w_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int8_t));
	void		(*abs_w_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int16_t));
	void		(*abs_w_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int32_t));
	void		(*abs_w_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int64_t));

	/* write (single) stream */
	void		(*abs_ws_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int8_t));
	void		(*abs_ws_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int16_t));
	void		(*abs_ws_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int32_t));
	void		(*abs_ws_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int64_t));
	
	/* write multiple */
	void		(*abs_wm_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int8_t *, bus_size_t));
	void		(*abs_wm_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int16_t *, bus_size_t));
	void		(*abs_wm_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int32_t *, bus_size_t));
	void		(*abs_wm_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int64_t *, bus_size_t));
	
	/* write multiple stream */
	void		(*abs_wms_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int8_t *, bus_size_t));
	void		(*abs_wms_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int16_t *, bus_size_t));
	void		(*abs_wms_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int32_t *, bus_size_t));
	void		(*abs_wms_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int64_t *, bus_size_t));

	/* write region */
	void		(*abs_wr_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int8_t *, bus_size_t));
	void		(*abs_wr_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int16_t *, bus_size_t));
	void		(*abs_wr_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int32_t *, bus_size_t));
	void		(*abs_wr_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int64_t *, bus_size_t));

	/* write region stream */
	void		(*abs_wrs_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int8_t *, bus_size_t));
	void		(*abs_wrs_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int16_t *, bus_size_t));
	void		(*abs_wrs_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int32_t *, bus_size_t));
	void		(*abs_wrs_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, const u_int64_t *, bus_size_t));

	/* set multiple */
	void		(*abs_sm_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int8_t, bus_size_t));
	void		(*abs_sm_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int16_t, bus_size_t));
	void		(*abs_sm_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int32_t, bus_size_t));
	void		(*abs_sm_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int64_t, bus_size_t));
	
	/* set region */
	void		(*abs_sr_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int8_t, bus_size_t));
	void		(*abs_sr_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int16_t, bus_size_t));
	void		(*abs_sr_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int32_t, bus_size_t));
	void		(*abs_sr_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, u_int64_t, bus_size_t));
	
#if 0 /* See comment on __abs_copy below */
	/* copy */
	void		(*abs_c_1) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, bus_space_handle_t, bus_size_t,
			    bus_size_t));
	void		(*abs_c_2) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, bus_space_handle_t, bus_size_t,
			    bus_size_t));
	void		(*abs_c_4) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, bus_space_handle_t, bus_size_t,
			    bus_size_t));
	void		(*abs_c_8) __P((bus_space_tag_t, bus_space_handle_t,
			    bus_size_t, bus_space_handle_t, bus_size_t,
			    bus_size_t));
#endif
};

/*
 * Utility macros; INTERNAL USE ONLY.
 */
#define	__abs_c(a,b)		__CONCAT(a,b)
#define	__abs_opname(op,size)	__abs_c(__abs_c(__abs_c(abs_,op),_),size)

#define	__abs_p(sz, t, h, o)						\
	(*(t)->__abs_opname(p,sz))(t, h, o)
#define	__abs_rs(sz, t, h, o)						\
	(*(t)->__abs_opname(r,sz))(t, h, o)
#define	__abs_rss(sz, t, h, o)						\
	(*(t)->__abs_opname(rs,sz))(t, h, o)
#define	__abs_ws(sz, t, h, o, v)					\
	(*(t)->__abs_opname(w,sz))(t, h, o, v)
#define	__abs_wss(sz, t, h, o, v)					\
	(*(t)->__abs_opname(ws,sz))(t, h, o, v)
#define	__abs_nonsingle(type, sz, t, h, o, a, c)			\
	(*(t)->__abs_opname(type,sz))(t, h, o, a, c)
#define	__abs_set(type, sz, t, h, o, v, c)				\
	(*(t)->__abs_opname(type,sz))(t, h, o, v, c)

    /*
     * No swaps needed and no other trickery, so it should be possible
     * to shortcut these to memcpy() directly [ leo 19990107 ]
     */
#if 0
    #define	__abs_copy(sz, t, h1, o1, h2, o2, cnt)			\
	(*(t)->__abs_opname(c,sz))(t, h1, o1, h2, o2, cnt)
#else
    #define	__abs_copy(sz, t, h1, o1, h2, o2, cnt)			\
	memcpy((void*)(h1 + o1), (void *)(h2 + o2), sz * cnt)
#endif


/*
 * Check accesibility of the location for various sized bus accesses
 */
#define bus_space_peek_1(t, h, o)	__abs_p(1,(t),(h),(o))
#define bus_space_peek_2(t, h, o)	__abs_p(2,(t),(h),(o))
#define bus_space_peek_4(t, h, o)	__abs_p(4,(t),(h),(o))
#define bus_space_peek_8(t, h, o)	__abs_p(8,(t),(h),(o))

/*
 * Bus read (single) operations.
 */
#define	bus_space_read_1(t, h, o)	__abs_rs(1,(t),(h),(o))
#define	bus_space_read_2(t, h, o)	__abs_rs(2,(t),(h),(o))
#define	bus_space_read_4(t, h, o)	__abs_rs(4,(t),(h),(o))
#define	bus_space_read_8(t, h, o)	__abs_rs(8,(t),(h),(o))

/*
 * Bus read (single) stream operations.
 */
#define	bus_space_read_stream_1(t, h, o)	__abs_rss(1,(t),(h),(o))
#define	bus_space_read_stream_2(t, h, o)	__abs_rss(2,(t),(h),(o))
#define	bus_space_read_stream_4(t, h, o)	__abs_rss(4,(t),(h),(o))
#define	bus_space_read_stream_8(t, h, o)	__abs_rss(8,(t),(h),(o))

/*
 * Bus read multiple operations.
 */
#define	bus_space_read_multi_1(t, h, o, a, c)				\
	__abs_nonsingle(rm,1,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_2(t, h, o, a, c)				\
	__abs_nonsingle(rm,2,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_4(t, h, o, a, c)				\
	__abs_nonsingle(rm,4,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_8(t, h, o, a, c)				\
	__abs_nonsingle(rm,8,(t),(h),(o),(a),(c))

/*
 * Bus read multiple stream operations.
 */
#define	bus_space_read_multi_stream_1(t, h, o, a, c)			\
	__abs_nonsingle(rms,1,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_stream_2(t, h, o, a, c)			\
	__abs_nonsingle(rms,2,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_stream_4(t, h, o, a, c)			\
	__abs_nonsingle(rms,4,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_stream_8(t, h, o, a, c)			\
	__abs_nonsingle(rms,8,(t),(h),(o),(a),(c))


/*
 * Bus read region operations.
 */
#define	bus_space_read_region_1(t, h, o, a, c)				\
	__abs_nonsingle(rr,1,(t),(h),(o),(a),(c))
#define	bus_space_read_region_2(t, h, o, a, c)				\
	__abs_nonsingle(rr,2,(t),(h),(o),(a),(c))
#define	bus_space_read_region_4(t, h, o, a, c)				\
	__abs_nonsingle(rr,4,(t),(h),(o),(a),(c))
#define	bus_space_read_region_8(t, h, o, a, c)				\
	__abs_nonsingle(rr,8,(t),(h),(o),(a),(c))

/*
 * Bus read region stream operations.
 */
#define	bus_space_read_region_stream_1(t, h, o, a, c)			\
	__abs_nonsingle(rrs,1,(t),(h),(o),(a),(c))
#define	bus_space_read_region_stream_2(t, h, o, a, c)			\
	__abs_nonsingle(rrs,2,(t),(h),(o),(a),(c))
#define	bus_space_read_region_stream_4(t, h, o, a, c)			\
	__abs_nonsingle(rrs,4,(t),(h),(o),(a),(c))
#define	bus_space_read_region_stream_8(t, h, o, a, c)			\
	__abs_nonsingle(rrs,8,(t),(h),(o),(a),(c))

/*
 * Bus write (single) operations.
 */
#define	bus_space_write_1(t, h, o, v)	__abs_ws(1,(t),(h),(o),(v))
#define	bus_space_write_2(t, h, o, v)	__abs_ws(2,(t),(h),(o),(v))
#define	bus_space_write_4(t, h, o, v)	__abs_ws(4,(t),(h),(o),(v))
#define	bus_space_write_8(t, h, o, v)	__abs_ws(8,(t),(h),(o),(v))

/*
 * Bus write (single) stream operations.
 */
#define	bus_space_write_stream_1(t, h, o, v)	__abs_wss(1,(t),(h),(o),(v))
#define	bus_space_write_stream_2(t, h, o, v)	__abs_wss(2,(t),(h),(o),(v))
#define	bus_space_write_stream_4(t, h, o, v)	__abs_wss(4,(t),(h),(o),(v))
#define	bus_space_write_stream_8(t, h, o, v)	__abs_wss(8,(t),(h),(o),(v))


/*
 * Bus write multiple operations.
 */
#define	bus_space_write_multi_1(t, h, o, a, c)				\
	__abs_nonsingle(wm,1,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_2(t, h, o, a, c)				\
	__abs_nonsingle(wm,2,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_4(t, h, o, a, c)				\
	__abs_nonsingle(wm,4,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_8(t, h, o, a, c)				\
	__abs_nonsingle(wm,8,(t),(h),(o),(a),(c))

/*
 * Bus write multiple stream operations.
 */
#define	bus_space_write_multi_stream_1(t, h, o, a, c)			\
	__abs_nonsingle(wms,1,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_stream_2(t, h, o, a, c)			\
	__abs_nonsingle(wms,2,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_stream_4(t, h, o, a, c)			\
	__abs_nonsingle(wms,4,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_stream_8(t, h, o, a, c)			\
	__abs_nonsingle(wms,8,(t),(h),(o),(a),(c))


/*
 * Bus write region operations.
 */
#define	bus_space_write_region_1(t, h, o, a, c)				\
	__abs_nonsingle(wr,1,(t),(h),(o),(a),(c))
#define	bus_space_write_region_2(t, h, o, a, c)				\
	__abs_nonsingle(wr,2,(t),(h),(o),(a),(c))
#define	bus_space_write_region_4(t, h, o, a, c)				\
	__abs_nonsingle(wr,4,(t),(h),(o),(a),(c))
#define	bus_space_write_region_8(t, h, o, a, c)				\
	__abs_nonsingle(wr,8,(t),(h),(o),(a),(c))

/*
 * Bus write region stream operations.
 */
#define	bus_space_write_region_stream_1(t, h, o, a, c)			\
	__abs_nonsingle(wrs,1,(t),(h),(o),(a),(c))
#define	bus_space_write_region_stream_2(t, h, o, a, c)			\
	__abs_nonsingle(wrs,2,(t),(h),(o),(a),(c))
#define	bus_space_write_region_stream_4(t, h, o, a, c)			\
	__abs_nonsingle(wrs,4,(t),(h),(o),(a),(c))
#define	bus_space_write_region_stream_8(t, h, o, a, c)			\
	__abs_nonsingle(wrs,8,(t),(h),(o),(a),(c))


/*
 * Set multiple operations.
 */
#define	bus_space_set_multi_1(t, h, o, v, c)				\
	__abs_set(sm,1,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_2(t, h, o, v, c)				\
	__abs_set(sm,2,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_4(t, h, o, v, c)				\
	__abs_set(sm,4,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_8(t, h, o, v, c)				\
	__abs_set(sm,8,(t),(h),(o),(v),(c))


/*
 * Set region operations.
 */
#define	bus_space_set_region_1(t, h, o, v, c)				\
	__abs_set(sr,1,(t),(h),(o),(v),(c))
#define	bus_space_set_region_2(t, h, o, v, c)				\
	__abs_set(sr,2,(t),(h),(o),(v),(c))
#define	bus_space_set_region_4(t, h, o, v, c)				\
	__abs_set(sr,4,(t),(h),(o),(v),(c))
#define	bus_space_set_region_8(t, h, o, v, c)				\
	__abs_set(sr,8,(t),(h),(o),(v),(c))


/*
 * Copy region operations.
 */
#define	bus_space_copy_region_1(t, h1, o1, h2, o2, c)			\
	__abs_copy(1, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_2(t, h1, o1, h2, o2, c)			\
	__abs_copy(2, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_4(t, h1, o1, h2, o2, c)			\
	__abs_copy(4, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_8(t, h1, o1, h2, o2, c)			\
	__abs_copy(8, (t), (h1), (o1), (h2), (o2), (c))

/*
 * Flags used in various bus DMA methods.
 */
#define	BUS_DMA_WAITOK		0x00	/* safe to sleep (pseudo-flag)       */
#define	BUS_DMA_NOWAIT		0x01	/* not safe to sleep		     */
#define	BUS_DMA_ALLOCNOW	0x02	/* perform resource allocation now   */
#define	BUS_DMA_COHERENT	0x04	/* hint: map memory DMA coherent     */
#define	BUS_DMA_BUS1		0x10	/* placeholders for bus functions... */
#define	BUS_DMA_BUS2		0x20
#define	BUS_DMA_BUS3		0x40
#define	BUS_DMA_BUS4		0x80

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

/*
 * Tag values:
 */
#define	BUS_ISA_DMA_TAG		0x00
#define	BUS_PCI_DMA_TAG		0x01

typedef int				bus_dma_tag_t;
typedef struct atari_bus_dmamap		*bus_dmamap_t;

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct atari_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */
};
typedef struct atari_bus_dma_segment	bus_dma_segment_t;

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct atari_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use my machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxsegsz;	/* largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	int		_dm_flags;	/* misc. flags */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

int	bus_dmamap_create __P((bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *));
void	bus_dmamap_destroy __P((bus_dma_tag_t, bus_dmamap_t));
int	bus_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int));
int	bus_dmamap_load_mbuf __P((bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int));
int	bus_dmamap_load_uio __P((bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int));
int	bus_dmamap_load_raw __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int));
void	bus_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
void	bus_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int));

int	bus_dmamem_alloc __P((bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags));
void	bus_dmamem_free __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs));
int	bus_dmamem_map __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, size_t size, caddr_t *kvap, int flags));
void	bus_dmamem_unmap __P((bus_dma_tag_t tag, caddr_t kva,
	    size_t size));
int	bus_dmamem_mmap __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, int off, int prot, int flags));

#endif /* _ATARI_BUS_H_ */
