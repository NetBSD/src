/*	$NetBSD: bus_space_notimpl.S,v 1.1.2.2 2018/04/07 04:12:10 pgoyette Exp $	*/

/*
 * Copyright (c) 2017 Ryo Shimizu <ryo@nerv.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

#define NOT_IMPL(func, methodstr)					\
	.global _C_LABEL(func)					;	\
_C_LABEL(func):							;	\
	mov	x2, x0						;	\
	adr	x1, 1f						;	\
	adr	x0, .Lnotimpl_msg				;	\
	b	_C_LABEL(panic)					;	\
1:	.asciz	methodstr					;	\
	.align	2

.Lnotimpl_msg:
	.asciz	"bus_space_%s() is not implemented on this tag: %p";	\


/* misc */
NOT_IMPL(bs_notimpl_bs_map, "map")
NOT_IMPL(bs_notimpl_bs_unmap, "unmap")
NOT_IMPL(bs_notimpl_bs_subregion, "subregion")
NOT_IMPL(bs_notimpl_bs_alloc, "alloc")
NOT_IMPL(bs_notimpl_bs_free, "free")
NOT_IMPL(bs_notimpl_bs_vaddr, "vaddr")
NOT_IMPL(bs_notimpl_bs_mmap, "mmap")
NOT_IMPL(bs_notimpl_bs_barrier, "barrier")

/* read */
NOT_IMPL(bs_notimpl_bs_r_1, "read_1")
NOT_IMPL(bs_notimpl_bs_r_2, "read_2")
NOT_IMPL(bs_notimpl_bs_r_4, "read_4")
NOT_IMPL(bs_notimpl_bs_r_8, "read_8")

/* read_multi */
NOT_IMPL(bs_notimpl_bs_rm_1, "read_multi_1")
NOT_IMPL(bs_notimpl_bs_rm_2, "read_multi_2")
NOT_IMPL(bs_notimpl_bs_rm_4, "read_multi_4")
NOT_IMPL(bs_notimpl_bs_rm_8, "read_multi_8")

/* read_region */
NOT_IMPL(bs_notimpl_bs_rr_1, "read_region_1")
NOT_IMPL(bs_notimpl_bs_rr_2, "read_region_2")
NOT_IMPL(bs_notimpl_bs_rr_4, "read_region_4")
NOT_IMPL(bs_notimpl_bs_rr_8, "read_region_8")

/* set_multi */
NOT_IMPL(bs_notimpl_bs_sm_1, "set_multi_1")
NOT_IMPL(bs_notimpl_bs_sm_2, "set_multi_2")
NOT_IMPL(bs_notimpl_bs_sm_4, "set_multi_4")
NOT_IMPL(bs_notimpl_bs_sm_8, "set_multi_8")

/* set_region */
NOT_IMPL(bs_notimpl_bs_sr_1, "set_region_1")
NOT_IMPL(bs_notimpl_bs_sr_2, "set_region_2")
NOT_IMPL(bs_notimpl_bs_sr_4, "set_region_4")
NOT_IMPL(bs_notimpl_bs_sr_8, "set_region_8")

/* write */
NOT_IMPL(bs_notimpl_bs_w_1, "write_1")
NOT_IMPL(bs_notimpl_bs_w_2, "write_2")
NOT_IMPL(bs_notimpl_bs_w_4, "write_4")
NOT_IMPL(bs_notimpl_bs_w_8, "write_8")

/* write_multi */
NOT_IMPL(bs_notimpl_bs_wm_1, "write_multi_1")
NOT_IMPL(bs_notimpl_bs_wm_2, "write_multi_2")
NOT_IMPL(bs_notimpl_bs_wm_4, "write_multi_4")
NOT_IMPL(bs_notimpl_bs_wm_8, "write_multi_8")

/* write_region */
NOT_IMPL(bs_notimpl_bs_wr_1, "write_region_1")
NOT_IMPL(bs_notimpl_bs_wr_2, "write_region_2")
NOT_IMPL(bs_notimpl_bs_wr_4, "write_region_4")
NOT_IMPL(bs_notimpl_bs_wr_8, "write_region_8")

/* copy_region */
NOT_IMPL(bs_notimpl_bs_c_1, "copy_region_1")
NOT_IMPL(bs_notimpl_bs_c_2, "copy_region_2")
NOT_IMPL(bs_notimpl_bs_c_4, "copy_region_4")
NOT_IMPL(bs_notimpl_bs_c_8, "copy_region_8")

/* peek */
NOT_IMPL(bs_notimpl_bs_pe_1, "peek_1")
NOT_IMPL(bs_notimpl_bs_pe_2, "peek_2")
NOT_IMPL(bs_notimpl_bs_pe_4, "peek_4")
NOT_IMPL(bs_notimpl_bs_pe_8, "peek_8")

/* poke */
NOT_IMPL(bs_notimpl_bs_po_1, "poke_1")
NOT_IMPL(bs_notimpl_bs_po_2, "poke_2")
NOT_IMPL(bs_notimpl_bs_po_4, "poke_4")
NOT_IMPL(bs_notimpl_bs_po_8, "poke_8")

/* read_stream */
NOT_IMPL(bs_notimpl_bs_r_1_s, "read_stream_1")
NOT_IMPL(bs_notimpl_bs_r_2_s, "read_stream_2")
NOT_IMPL(bs_notimpl_bs_r_4_s, "read_stream_4")
NOT_IMPL(bs_notimpl_bs_r_8_s, "read_stream_8")

/* write_stream */
NOT_IMPL(bs_notimpl_bs_w_1_s, "write_stream_1")
NOT_IMPL(bs_notimpl_bs_w_2_s, "write_stream_2")
NOT_IMPL(bs_notimpl_bs_w_4_s, "write_stream_4")
NOT_IMPL(bs_notimpl_bs_w_8_s, "write_stream_8")

/* read_region_stream */
NOT_IMPL(bs_notimpl_bs_rr_1_s, "read_region_stream_1")
NOT_IMPL(bs_notimpl_bs_rr_2_s, "read_region_stream_2")
NOT_IMPL(bs_notimpl_bs_rr_4_s, "read_region_stream_4")
NOT_IMPL(bs_notimpl_bs_rr_8_s, "read_region_stream_8")

/* write_region_stream */
NOT_IMPL(bs_notimpl_bs_wr_1_s, "write_region_stream_1")
NOT_IMPL(bs_notimpl_bs_wr_2_s, "write_region_stream_2")
NOT_IMPL(bs_notimpl_bs_wr_4_s, "write_region_stream_4")
NOT_IMPL(bs_notimpl_bs_wr_8_s, "write_region_stream_8")

/* read_multi_stream */
NOT_IMPL(bs_notimpl_bs_rm_1_s, "read_multi_stream_1")
NOT_IMPL(bs_notimpl_bs_rm_2_s, "read_multi_stream_2")
NOT_IMPL(bs_notimpl_bs_rm_4_s, "read_multi_stream_4")
NOT_IMPL(bs_notimpl_bs_rm_8_s, "read_multi_stream_8")

/* write_multi_stream */
NOT_IMPL(bs_notimpl_bs_wm_1_s, "write_multi_stream_1")
NOT_IMPL(bs_notimpl_bs_wm_2_s, "write_multi_stream_2")
NOT_IMPL(bs_notimpl_bs_wm_4_s, "write_multi_stream_4")
NOT_IMPL(bs_notimpl_bs_wm_8_s, "write_multi_stream_8")
