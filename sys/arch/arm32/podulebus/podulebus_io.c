/*	$NetBSD: podulebus_io.c,v 1.3 1997/07/30 23:49:54 mark Exp $	*/

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
 * bus_space I/O functions for podulebus
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <machine/bus.h>

/* Proto types for all the bus_space structure functions */

bs_protos(podulebus);

/* Declare the podulebus bus space tag */

struct bus_space podulebus_bs_tag = {
	/* cookie */
	(void *) 2,			/* shift to apply to registers */

	/* mapping/unmapping */
	podulebus_map,
	podulebus_unmap,
	podulebus_subregion,

	/* allocation/deallocation */
	podulebus_alloc,
	podulebus_free,

	/* barrier */
	podulebus_barrier,

	/* read (single) */
	podulebus_r_1,
	podulebus_r_2,
	podulebus_r_4,
	podulebus_r_8,

	/* read multiple */
	podulebus_rm_1,
   	podulebus_rm_2,
	podulebus_rm_4,
	podulebus_rm_8,

	/* read region */
	podulebus_rr_1,
	podulebus_rr_2,
	podulebus_rr_4,
	podulebus_rr_8,

	/* write (single) */
	podulebus_w_1,
	podulebus_w_2,
	podulebus_w_4,
	podulebus_w_8,

	/* write multiple */
	podulebus_wm_1,
	podulebus_wm_2,
	podulebus_wm_4,
	podulebus_wm_8,

	/* write region */
	podulebus_wr_1,
	podulebus_wr_2,
	podulebus_wr_4,
	podulebus_wr_8,

	/* set multiple */
	podulebus_sm_1,
	podulebus_sm_2,
	podulebus_sm_4,
	podulebus_sm_8,

	/* set region */
	podulebus_sr_1,
	podulebus_sr_2,
	podulebus_sr_4,
	podulebus_sr_8,

	/* copy */
	podulebus_c_1,
	podulebus_c_2,
	podulebus_c_4,
	podulebus_c_8,
};

/* bus space functions */

int
podulebus_map(t, bpa, size, cacheable, bshp)
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
podulebus_alloc(t, rstart, rend, size, alignment, boundary, cacheable,
    bpap, bshp)
	void *t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int cacheable;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	panic("podulebus_alloc(): Help!\n");
}


void
podulebus_unmap(t, bsh, size)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	/*
	 * Temporary implementation
	 */
}

void    
podulebus_free(t, bsh, size)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t size;
{

	panic("podulebus_free(): Help!\n");
	/* podulebus_unmap() does all that we need to do. */
/*	podulebus_unmap(t, bsh, size);*/
}

int
podulebus_subregion(t, bsh, offset, size, nbshp)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t offset, size;
	bus_space_handle_t *nbshp;
{

	*nbshp = bsh + (offset << ((int)t));
	return (0);
}

void
podulebus_barrier(t, bsh, offset, len, flags)
	void *t;
	bus_space_handle_t bsh;
	bus_size_t offset, len;
	int flags;
{
}	
