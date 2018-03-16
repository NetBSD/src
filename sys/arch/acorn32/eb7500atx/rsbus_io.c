/*	$NetBSD: rsbus_io.c,v 1.5 2018/03/16 17:56:31 ryo Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
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
 * bus_space I/O functions for rsbus
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rsbus_io.c,v 1.5 2018/03/16 17:56:31 ryo Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>

/* Proto types for all the bus_space structure functions */

bs_protos(rsbus);
bs_protos(bs_notimpl);
bs_protos(mainbus);

/* Declare the rsbus bus space tag */
struct bus_space rsbus_bs_tag = {
	/* cookie */
	.bs_cookie = (void *) 2,	/* Shift to apply to registers */

	/* mapping/unmapping */
	.bs_map = mainbus_bs_map,
	.bs_unmap = mainbus_bs_unmap,
	.bs_subregion = mainbus_bs_subregion,

	/* allocation/deallocation */
	.bs_alloc = mainbus_bs_alloc,
	.bs_free = mainbus_bs_free,

	/* get kernel virtual address */
	.bs_vaddr = 0, /* there is no linear mapping */

	/* mmap bus space for userland */
	.bs_mmap = mainbus_bs_mmap,

	/* barrier */
	.bs_barrier = mainbus_bs_barrier,

	/* read (single) */
	.bs_r_1 = rsbus_bs_r_1,
	.bs_r_2 = rsbus_bs_r_2,
	.bs_r_4 = rsbus_bs_r_4,
	.bs_r_8 = bs_notimpl_bs_r_8,

	/* read multiple */
	.bs_rm_1 = rsbus_bs_rm_1,
	.bs_rm_2 = rsbus_bs_rm_2,
	.bs_rm_4 = bs_notimpl_bs_rm_4,
	.bs_rm_8 = bs_notimpl_bs_rm_8,

	/* read region */
	.bs_rr_1 = rsbus_bs_rr_1,
	.bs_rr_2 = rsbus_bs_rr_2,
	.bs_rr_4 = bs_notimpl_bs_rr_4,
	.bs_rr_8 = bs_notimpl_bs_rr_8,

	/* write (single) */
	.bs_w_1 = rsbus_bs_w_1,
	.bs_w_2 = rsbus_bs_w_2,
	.bs_w_4 = rsbus_bs_w_4,
	.bs_w_8 = bs_notimpl_bs_w_8,

	/* write multiple */
	.bs_wm_1 = rsbus_bs_wm_1,
	.bs_wm_2 = rsbus_bs_wm_2,
	.bs_wm_4 = bs_notimpl_bs_wm_4,
	.bs_wm_8 = bs_notimpl_bs_wm_8,

	/* write region */
	.bs_wr_1 = rsbus_bs_wr_1,
	.bs_wr_2 = rsbus_bs_wr_2,
	.bs_wr_4 = bs_notimpl_bs_wr_4,
	.bs_wr_8 = bs_notimpl_bs_wr_8,

	/* set multiple */
	.bs_sm_1 = bs_notimpl_bs_sm_1,
	.bs_sm_2 = bs_notimpl_bs_sm_2,
	.bs_sm_4 = bs_notimpl_bs_sm_4,
	.bs_sm_8 = bs_notimpl_bs_sm_8,

	/* set region */
	.bs_sr_1 = rsbus_bs_sr_1,
	.bs_sr_2 = rsbus_bs_sr_2,
	.bs_sr_4 = bs_notimpl_bs_sr_4,
	.bs_sr_8 = bs_notimpl_bs_sr_8,

	/* copy */
	.bs_c_1 = bs_notimpl_bs_c_1,
	.bs_c_2 = bs_notimpl_bs_c_2,
	.bs_c_4 = bs_notimpl_bs_c_4,
	.bs_c_8 = bs_notimpl_bs_c_8,
};

/* bus space functions */

/* Rough-and-ready implementations from arm26 */
void
rsbus_bs_rr_1(void *cookie, bus_space_handle_t bsh,
			bus_size_t offset, uint8_t *datap, bus_size_t count)
{
	int i;

	for (i = 0; i < count; i++)
		datap[i] = rsbus_bs_r_1(cookie, bsh, offset + i);
}

void
rsbus_bs_rr_2(void *cookie, bus_space_handle_t bsh,
			bus_size_t offset, uint16_t *datap, bus_size_t count)
{
	int i;

	for (i = 0; i < count; i++)
		datap[i] = rsbus_bs_r_2(cookie, bsh, offset + i);
}

void
rsbus_bs_wr_1(void *cookie, bus_space_handle_t bsh,
			 bus_size_t offset, uint8_t const *datap,
			 bus_size_t count)
{
	int i;

	for (i = 0; i < count; i++)
		rsbus_bs_w_1(cookie, bsh, offset + i, datap[i]);
}

void
rsbus_bs_wr_2(void *cookie, bus_space_handle_t bsh,
			 bus_size_t offset, uint16_t const *datap,
			 bus_size_t count)
{
	int i;

	for (i = 0; i < count; i++)
		rsbus_bs_w_2(cookie, bsh, offset + i, datap[i]);
}

void
rsbus_bs_sr_1(void *cookie, bus_space_handle_t bsh,
		       bus_size_t offset, uint8_t value, bus_size_t count)
{
	int i;

	for (i = 0; i < count; i++)
		rsbus_bs_w_1(cookie, bsh, offset + i, value);
}

void
rsbus_bs_sr_2(void *cookie, bus_space_handle_t bsh,
		       bus_size_t offset, uint16_t value, bus_size_t count)
{
	int i;

	for (i = 0; i < count; i++)
		rsbus_bs_w_2(cookie, bsh, offset + i, value);
}
