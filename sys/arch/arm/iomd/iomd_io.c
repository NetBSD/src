/*	$NetBSD: iomd_io.c,v 1.6.52.1 2018/03/22 01:44:43 pgoyette Exp $	*/

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
 *	This product includes software developed by Mark Brinicombe.
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
 * bus_space I/O functions for iomd
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iomd_io.c,v 1.6.52.1 2018/03/22 01:44:43 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>

/* Proto types for all the bus_space structure functions */

bs_protos(iomd);
bs_protos(bs_notimpl);

/* Declare the iomd bus space tag */

struct bus_space iomd_bs_tag = {
	/* cookie */
	.bs_cookie = NULL,

	/* mapping/unmapping */
	.bs_map = iomd_bs_map,
	.bs_unmap = iomd_bs_unmap,
	.bs_subregion = iomd_bs_subregion,

	/* allocation/deallocation */
	.bs_alloc = iomd_bs_alloc,
	.bs_free = iomd_bs_free,

	/* get kernel virtual address */
	.bs_vaddr = 0, /* there is no linear mapping */

	/* mmap bus space for userland */
	.bs_mmap = bs_notimpl_bs_mmap,	/* XXX correct? XXX */

	/* barrier */
	.bs_barrier = iomd_bs_barrier,

	/* read (single) */
	.bs_r_1 = iomd_bs_r_1,
	.bs_r_2 = iomd_bs_r_2,
	.bs_r_4 = iomd_bs_r_4,
	.bs_r_8 = bs_notimpl_bs_r_8,

	/* read multiple */
	.bs_rm_1 = bs_notimpl_bs_rm_1,
	.bs_rm_2 = iomd_bs_rm_2,
	.bs_rm_4 = bs_notimpl_bs_rm_4,
	.bs_rm_8 = bs_notimpl_bs_rm_8,

	/* read region */
	.bs_rr_1 = bs_notimpl_bs_rr_1,
	.bs_rr_2 = bs_notimpl_bs_rr_2,
	.bs_rr_4 = bs_notimpl_bs_rr_4,
	.bs_rr_8 = bs_notimpl_bs_rr_8,

	/* write (single) */
	.bs_w_1 = iomd_bs_w_1,
	.bs_w_2 = iomd_bs_w_2,
	.bs_w_4 = iomd_bs_w_4,
	.bs_w_8 = bs_notimpl_bs_w_8,

	/* write multiple */
	.bs_wm_1 = bs_notimpl_bs_wm_1,
	.bs_wm_2 = iomd_bs_wm_2,
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
};

/* bus space functions */

int
iomd_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int cacheable,
    bus_space_handle_t *bshp)
{

	/*
	 * Temporary implementation as all I/O is already mapped etc.
	 *
	 * Eventually this function will do the mapping check for multiple maps
	 */
	*bshp = bpa;
	return 0;
}

int
iomd_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend, bus_size_t size,
    bus_size_t alignment, bus_size_t boundary, int cacheable, bus_addr_t *bpap,
    bus_space_handle_t *bshp)
{

	panic("iomd_alloc(): Help!");
}


void
iomd_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{

	/*
	 * Temporary implementation
	 */
}

void    
iomd_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{

	panic("iomd_free(): Help!");
	/* iomd_unmap() does all that we need to do. */
/*	iomd_unmap(t, bsh, size);*/
}

int
iomd_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + (offset << 2);
	return 0;
}

void
iomd_bs_barrier(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t len, int flags)
{

}	

/* End of iomd_io.c */
