/*	$NetBSD: footbridge_com_io.c,v 1.9 2018/03/16 17:56:31 ryo Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This file provides the bus space tag for the footbridge serial console
 */

/*
 * bus_space I/O functions for mainbus
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: footbridge_com_io.c,v 1.9 2018/03/16 17:56:31 ryo Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>

/* Proto types for all the bus_space structure functions */

bs_protos(fcomcons);
bs_protos(generic);
bs_protos(bs_notimpl);

/* Declare the fcomcons bus space tag */

struct bus_space fcomcons_bs_tag = {
	/* cookie */
	.bs_cookie = NULL,

	/* mapping/unmapping */
	.bs_map = fcomcons_bs_map,
	.bs_unmap = fcomcons_bs_unmap,
	.bs_subregion = fcomcons_bs_subregion,

	/* allocation/deallocation */
	.bs_alloc = fcomcons_bs_alloc,
	.bs_free = fcomcons_bs_free,

	/* get kernel virtual address */
	.bs_vaddr = 0, /* never used */

	/* Mmap bus space for user */
	.bs_mmap = bs_notimpl_bs_mmap,

	/* barrier */
	.bs_barrier = fcomcons_bs_barrier,

	/* read (single) */
	.bs_r_1 = bs_notimpl_bs_r_1,
	.bs_r_2 = bs_notimpl_bs_r_2,
	.bs_r_4 = generic_bs_r_4,
	.bs_r_8 = bs_notimpl_bs_r_8,

	/* read multiple */
	.bs_rm_1 = bs_notimpl_bs_rm_1,
	.bs_rm_2 = bs_notimpl_bs_rm_2,
	.bs_rm_4 = bs_notimpl_bs_rm_4,
	.bs_rm_8 = bs_notimpl_bs_rm_8,

	/* read region */
	.bs_rr_1 = bs_notimpl_bs_rr_1,
	.bs_rr_2 = bs_notimpl_bs_rr_2,
	.bs_rr_4 = bs_notimpl_bs_rr_4,
	.bs_rr_8 = bs_notimpl_bs_rr_8,

	/* write (single) */
	.bs_w_1 = bs_notimpl_bs_w_1,
	.bs_w_2 = bs_notimpl_bs_w_2,
	.bs_w_4 = generic_bs_w_4,
	.bs_w_8 = bs_notimpl_bs_w_8,

	/* write multiple */
	.bs_wm_1 = bs_notimpl_bs_wm_1,
	.bs_wm_2 = bs_notimpl_bs_wm_2,
	.bs_wm_4 = bs_notimpl_bs_wm_4,
	.bs_wm_8 = bs_notimpl_bs_wm_8,

	/* write region */
	.bs_wr_1 = bs_notimpl_bs_wr_1,
	.bs_wr_2 = bs_notimpl_bs_wr_2,
	.bs_wr_4 = bs_notimpl_bs_wr_4,
	.bs_wr_8 = bs_notimpl_bs_wr_8,

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
};

/* bus space functions */

int
fcomcons_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int cacheable, bus_space_handle_t *bshp)
{
	/*
	 * Temporary implementation as all I/O is already mapped etc.
	 *
	 * Eventually this function will do the mapping check for multiple maps
	 */
	*bshp = bpa;
	return(0);
	}

int
fcomcons_bs_alloc(
	void *t,
	bus_addr_t rstart,
	bus_addr_t rend,
	bus_size_t size,
	bus_size_t alignment,
	bus_size_t boundary,
	int cacheable,
	bus_addr_t *bpap,
	bus_space_handle_t *bshp)
{
	panic("fcomcons_alloc(): Help!");
}


void
fcomcons_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	/*
	 * Temporary implementation
	 */
}

void    
fcomcons_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{

	panic("fcomcons_free(): Help!");
	/* fcomcons_unmap() does all that we need to do. */
/*	fcomcons_unmap(t, bsh, size);*/
}

int
fcomcons_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return (0);
}

void
fcomcons_bs_barrier(void *t, bus_space_handle_t bsh, bus_size_t offset, bus_size_t len, int flags)
{
}	

/* End of footbridge_com_io.c */
