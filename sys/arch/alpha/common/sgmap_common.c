/* $NetBSD: sgmap_common.c,v 1.2.2.2 1997/06/07 04:42:54 cgd Exp $ */

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: sgmap_common.c,v 1.2.2.2 1997/06/07 04:42:54 cgd Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <vm/vm.h>

#include <machine/bus.h>

#include <alpha/common/sgmapvar.h>

void
alpha_sgmap_init(t, sgmap, name, wbase, sgvabase, sgvasize, ptesize, ptva)
	bus_dma_tag_t t;
	struct alpha_sgmap *sgmap;
	const char *name;
	bus_addr_t wbase;
	bus_addr_t sgvabase;
	bus_size_t sgvasize;
	size_t ptesize;
	void *ptva;
{
	bus_dma_segment_t seg;
	size_t ptsize;
	int rseg;

	if (sgvasize & PGOFSET)
		panic("alpha_sgmap_init: size botch");

	sgmap->aps_wbase = wbase;
	sgmap->aps_sgvabase = sgvabase;
	sgmap->aps_sgvasize = sgvasize;

	if (ptva != NULL) {
		/*
		 * We already have a page table; this may be a system
		 * where the page table resides in bridge-resident SRAM.
		 */
		sgmap->aps_pt = ptva;
		sgmap->aps_ptpa = 0;
	} else {
		/*
		 * Compute the page table size and allocate it.  It must
		 * be aligned to its size.
		 */
		ptsize = (sgvasize / NBPG) * ptesize;
		if (bus_dmamem_alloc(t, ptsize, ptsize, 0, &seg, 1, &rseg,
		    BUS_DMA_NOWAIT))
			panic("alpha_sgmap_init: can't allocate page table");
		if (bus_dmamem_map(t, &seg, rseg, ptsize,
		    (caddr_t *)&sgmap->aps_pt, BUS_DMA_NOWAIT))
			panic("alpha_sgmap_init: can't map page table");
		sgmap->aps_ptpa = seg.ds_addr;
	}

	/*
	 * Create the extent map used to manage the virtual address
	 * space.
	 */
	sgmap->aps_ex = extent_create((char *)name, sgvabase, sgvasize - 1,
	    M_DEVBUF, NULL, 0, EX_NOWAIT|EX_NOCOALESCE);
	if (sgmap->aps_ex == NULL)
		panic("alpha_sgmap_init: can't create extent map");
}

int
alpha_sgmap_alloc(map, len, sgmap, flags)
	bus_dmamap_t map;
	bus_size_t len;
	struct alpha_sgmap *sgmap;
	int flags;
{
	struct alpha_sgmap_cookie *a = map->_dm_sgcookie;
	int error;

#ifdef DIAGNOSTIC
	if (a->apdc_flags & APDC_HAS_SGMAP)
		panic("alpha_sgmap_alloc: already have sgva space");
#endif

	a->apdc_len = round_page(len);
	error = extent_alloc(sgmap->aps_ex, a->apdc_len, NBPG,
	    map->_dm_boundary, (flags & BUS_DMA_NOWAIT) ? EX_NOWAIT :
	    EX_WAITOK, &a->apdc_sgva);

	if (error == 0)
		a->apdc_flags |= APDC_HAS_SGMAP;
	else
		a->apdc_flags &= ~APDC_HAS_SGMAP;
	
	return (error);
}

void
alpha_sgmap_free(sgmap, a)
	struct alpha_sgmap *sgmap;
	struct alpha_sgmap_cookie *a;
{

#ifdef DIAGNOSTIC
	if ((a->apdc_flags & APDC_HAS_SGMAP) == 0)
		panic("alpha_sgmap_free: no sgva space to free");
#endif

	if (extent_free(sgmap->aps_ex, a->apdc_sgva,
	    a->apdc_len, EX_NOWAIT))
		panic("alpha_sgmap_free");

	a->apdc_flags &= ~APDC_HAS_SGMAP;
}
