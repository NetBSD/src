/*	$NetBSD: omap_a2x_space.c,v 1.3.38.1 2018/03/22 01:44:43 pgoyette Exp $ */

/*
 * Based on arch/arm/xscale/pxa2x0_a4x_space.c
 *
 * Copyright (c) 2002  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 *	This product includes software developed for the NetBSD Project by
 *	Genetec Corporation.
 * 4. The name of Genetec Corporation may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Bus space tag for 8/16-bit devices on 32-bit bus.
 * all registers are located at the address of multiple of 2.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap_a2x_space.c,v 1.3.38.1 2018/03/22 01:44:43 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>

/* Prototypes for all the bus_space structure functions */
bs_protos(omap);
bs_protos(a2x);
bs_protos(bs_notimpl);

struct bus_space omap_a2x_bs_tag = {
	/* cookie */
	.bs_cookie = (void *) 0,

	/* mapping/unmapping */
	.bs_map = omap_bs_map,
	.bs_unmap = omap_bs_unmap,
	.bs_subregion = omap_bs_subregion,

	/* allocation/deallocation */
	.bs_alloc = omap_bs_alloc,	/* not implemented */
	.bs_free = omap_bs_free,	/* not implemented */

	/* get kernel virtual address */
	.bs_vaddr = omap_bs_vaddr,

	/* mmap */
	.bs_mmap = bs_notimpl_bs_mmap,

	/* barrier */
	.bs_barrier = omap_bs_barrier,

	/* read (single) */
	.bs_r_1 = a2x_bs_r_1,
	.bs_r_2 = a2x_bs_r_2,
	.bs_r_4 = a2x_bs_r_4,
	.bs_r_8 = bs_notimpl_bs_r_8,

	/* read multiple */
	.bs_rm_1 = a2x_bs_rm_1,
	.bs_rm_2 = a2x_bs_rm_2,
	.bs_rm_4 = bs_notimpl_bs_rm_4,
	.bs_rm_8 = bs_notimpl_bs_rm_8,

	/* read region */
	.bs_rr_1 = bs_notimpl_bs_rr_1,
	.bs_rr_2 = bs_notimpl_bs_rr_2,
	.bs_rr_4 = bs_notimpl_bs_rr_4,
	.bs_rr_8 = bs_notimpl_bs_rr_8,

	/* write (single) */
	.bs_w_1 = a2x_bs_w_1,
	.bs_w_2 = a2x_bs_w_2,
	.bs_w_4 = a2x_bs_w_4,
	.bs_w_8 = bs_notimpl_bs_w_8,

	/* write multiple */
	.bs_wm_1 = a2x_bs_wm_1,
	.bs_wm_2 = a2x_bs_wm_2,
	.bs_wm_4 = bs_notimpl_bs_wm_4,
	.bs_wm_8 = bs_notimpl_bs_wm_8,

	/* write region */
	.bs_wr_1 = bs_notimpl_bs_wr_1,
	.bs_wr_2 = bs_notimpl_bs_wr_2,
	.bs_wr_4 = bs_notimpl_bs_wr_4,
	.bs_wr_8 = bs_notimpl_bs_wr_8,

	/* set multiple */
	.bs_sm_1 = bs_notimpl_bs_sm_1,
	.bs_sm_2 = bs_notimpl_bs_sm_2,
	.bs_sm_4 = bs_notimpl_bs_sm_4,
	.bs_sm_8 = bs_notimpl_bs_sm_8,

	/* set region */
	.bs_sr_1 = bs_notimpl_bs_sr_1,
	.bs_sr_2 = bs_notimpl_bs_sr_2,
	.bs_sr_4 = bs_notimpl_bs_sr_4,
	.bs_sr_8 = bs_notimpl_bs_sr_8,

	/* copy */
	.bs_c_1 = bs_notimpl_bs_c_1,
	.bs_c_2 = bs_notimpl_bs_c_2,
	.bs_c_4 = bs_notimpl_bs_c_4,
	.bs_c_8 = bs_notimpl_bs_c_8,

#ifdef __BUS_SPACE_HAS_STREAM_METHODS
	/* stream methods */
	/* read (single) */
	.bs_r_1_s = a2x_bs_r_1,
	.bs_r_2_s = a2x_bs_r_2,
	.bs_r_4_s = a2x_bs_r_4,
	.bs_r_8_s = bs_notimpl_bs_r_8,

	/* read multiple */
	.bs_rm_1_s = a2x_bs_rm_1,
	.bs_rm_2_s = a2x_bs_rm_2,
	.bs_rm_4_s = bs_notimpl_bs_rm_4,
	.bs_rm_8_s = bs_notimpl_bs_rm_8,

	/* read region */
	.bs_rr_1_s = bs_notimpl_bs_rr_1,
	.bs_rr_2_s = bs_notimpl_bs_rr_2,
	.bs_rr_4_s = bs_notimpl_bs_rr_4,
	.bs_rr_8_s = bs_notimpl_bs_rr_8,

	/* write (single) */
	.bs_w_1_s = a2x_bs_w_1,
	.bs_w_2_s = a2x_bs_w_2,
	.bs_w_4_s = a2x_bs_w_4,
	.bs_w_8_s = bs_notimpl_bs_w_8,

	/* write multiple */
	.bs_wm_1_s = a2x_bs_wm_1,
	.bs_wm_2_s = a2x_bs_wm_2,
	.bs_wm_4_s = bs_notimpl_bs_wm_4,
	.bs_wm_8_s = bs_notimpl_bs_wm_8,

	/* write region */
	.bs_wr_1_s = bs_notimpl_bs_wr_1,
	.bs_wr_2_s = bs_notimpl_bs_wr_2,
	.bs_wr_4_s = bs_notimpl_bs_wr_4,
	.bs_wr_8_s = bs_notimpl_bs_wr_8,
#endif
};



