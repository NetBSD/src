/*	$NetBSD: mainbus_io.c,v 1.3 1997/10/14 21:25:03 mark Exp $	*/

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
 * bus_space I/O functions for mainbus
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <machine/bus.h>

/* Proto types for all the bus_space structure functions */

bs_protos(mainbus);
bs_protos(bs_notimpl);

/* Declare the mainbus bus space tag */

struct bus_space mainbus_bs_tag = {
	/* cookie */
	NULL,

	/* mapping/unmapping */
	mainbus_map,
	mainbus_unmap,
	mainbus_subregion,

	/* allocation/deallocation */
	mainbus_alloc,
	mainbus_free,

	/* barrier */
	mainbus_barrier,

	/* read (single) */
	mainbus_r_1,
	mainbus_r_2,
	mainbus_r_4,
	bs_notimpl_r_8,

	/* read multiple */
	bs_notimpl_rm_1,
	mainbus_rm_2,
	bs_notimpl_rm_4,
	bs_notimpl_rm_8,

	/* read region */
	bs_notimpl_rr_1,
	bs_notimpl_rr_2,
	bs_notimpl_rr_4,
	bs_notimpl_rr_8,

	/* write (single) */
	mainbus_w_1,
	mainbus_w_2,
	mainbus_w_4,
	bs_notimpl_w_8,

	/* write multiple */
	mainbus_wm_1,
	mainbus_wm_2,
	bs_notimpl_wm_4,
	bs_notimpl_wm_8,

	/* write region */
	bs_notimpl_wr_1,
	bs_notimpl_wr_2,
	bs_notimpl_wr_4,
	bs_notimpl_wr_8,

	bs_notimpl_sm_1,
	bs_notimpl_sm_2,
	bs_notimpl_sm_4,
	bs_notimpl_sm_8,

	/* set region */
	bs_notimpl_sr_1,
	bs_notimpl_sr_2,
	bs_notimpl_sr_4,
	bs_notimpl_sr_8,

	/* copy */
	bs_notimpl_c_1,
	bs_notimpl_c_2,
	bs_notimpl_c_4,
	bs_notimpl_c_8,
};

/* bus space functions */

int
mainbus_map(t, bpa, size, cacheable, bshp)
	void *t;
	bus_addr_t bpa;
	bus_size_t size;
	int cacheable;
	bus_space_handle_t *bshp;
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
mainbus_alloc(t, rstart, rend, size, alignment, boundary, cacheable,
    bpap, bshp)
	void *t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int cacheable;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	panic("mainbus_alloc(): Help!\n");
}


void
mainbus_unmap(t, bsh, size)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	/*
	 * Temporary implementation
	 */
}

void    
mainbus_free(t, bsh, size)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t size;
{

	panic("mainbus_free(): Help!\n");
	/* mainbus_unmap() does all that we need to do. */
/*	mainbus_unmap(t, bsh, size);*/
}

int
mainbus_subregion(t, bsh, offset, size, nbshp)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t offset, size;
	bus_space_handle_t *nbshp;
{

	*nbshp = bsh + offset;
	return (0);
}

void
mainbus_barrier(t, bsh, offset, len, flags)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t offset, len;
	int flags;
{
}	

/* End of mainbus_io.c */
