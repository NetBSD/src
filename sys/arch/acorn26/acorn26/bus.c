/* $NetBSD: bus.c,v 1.6 2006/09/30 16:30:10 bjh21 Exp $ */
/*-
 * Copyright (c) 1999, 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * bus.c - bus space functions for Archimedes I/O bus
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus.c,v 1.6 2006/09/30 16:30:10 bjh21 Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <machine/bus.h>
#include <machine/memcreg.h>

#include <arm/blockio.h>

bs_protos(iobus);
bs_protos(bs_notimpl);

struct bus_space iobus_bs_tag = {
	.bs_cookie = (void *)2,
	.bs_map = iobus_bs_map, .bs_unmap = iobus_bs_unmap,
	.bs_subregion = iobus_bs_subregion,
	.bs_alloc = iobus_bs_alloc, .bs_free = iobus_bs_free,
	.bs_mmap = iobus_bs_mmap,
	.bs_barrier = iobus_bs_barrier,
	.bs_r_1  = iobus_bs_r_1,  .bs_r_2  = iobus_bs_r_2,
	.bs_rm_1 = iobus_bs_rm_1, .bs_rm_2 = iobus_bs_rm_2,
	.bs_rr_1 = iobus_bs_rr_1, .bs_rr_2 = iobus_bs_rr_2,
	.bs_w_1  = iobus_bs_w_1,  .bs_w_2  = iobus_bs_w_2,
	.bs_wm_1 = iobus_bs_wm_1, .bs_wm_2 = iobus_bs_wm_2,
	.bs_wr_1 = iobus_bs_wr_1, .bs_wr_2 = iobus_bs_wr_2,
	.bs_sm_1 = iobus_bs_sm_1, .bs_sm_2 = iobus_bs_sm_2,
	.bs_sr_1 = iobus_bs_sr_1, .bs_sr_2 = iobus_bs_sr_2,
};	

int
iobus_bs_map(void *t, bus_addr_t addr, bus_size_t size,
	      int flags, bus_space_handle_t *bshp)
{

	if (flags & BUS_SPACE_MAP_LINEAR)
		return -1;
	*bshp = (u_long)addr;
	return 0;
}

void
iobus_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{

}

int
iobus_bs_alloc(void *t,	bus_addr_t rstart, bus_addr_t rend, bus_size_t size,
    bus_size_t alignment, bus_size_t boundary, int flags, bus_addr_t *bpap,
    bus_space_handle_t *bshp)
{

	return -1;
}

void    
iobus_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{

}

int
iobus_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + (offset << (int)t);
	return 0;
}

paddr_t
iobus_bs_mmap(void *t, bus_addr_t addr, off_t off, int prot, int flags)
{

	return -1;
}

void
iobus_bs_barrier(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t len, int flags)
{

}

int
bus_space_shift(bus_space_tag_t bst, int shift, bus_space_tag_t *nbstp)
{

	*nbstp = malloc(sizeof(**nbstp), M_DEVBUF, M_WAITOK);
	**nbstp = *bst;
	(*nbstp)->bs_cookie = (void *)shift;
	return 0;
}
