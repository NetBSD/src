/*	$NetBSD: bus_space_funcs.h,v 1.2 2020/07/26 08:08:41 simonb Exp $	*/

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

#ifndef _MIPS_BUS_SPACE_FUNCS_H_
#define	_MIPS_BUS_SPACE_FUNCS_H_

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
#define	bus_space_vaddr(t, h) \
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

#endif /* _MIPS_BUS_SPACE_FUNCS_H_ */
