/* $NetBSD: sgmapvar.h,v 1.1.2.3 1997/06/04 05:46:37 thorpej Exp $ */

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

#include <sys/extent.h>
#include <machine/bus.h>

/*
 * Bits n:13 of the DMA address are the index of the PTE into
 * the SGMAP page table.
 */
#define	SGMAP_ADDR_PTEIDX_SHIFT	13

/*
 * An Alpha SGMAP's state information.
 */
struct alpha_sgmap {
	struct extent *aps_ex;		/* extent map to manage sgva space */
	void	*aps_pt;		/* page table */
	bus_addr_t aps_ptpa;		/* page table physical address */
	bus_addr_t aps_sgvabase;	/* base of the sgva space */
	bus_size_t aps_sgvasize;	/* size of the sgva space */
	bus_addr_t aps_wbase;		/* base of the dma window */
};

/*
 * Cookie used to store SGMAP state for a DMA mapping.  We
 * could compute some of this at unload time, but storing it
 * is convenient.
 */
struct alpha_sgmap_cookie {
	int	apdc_pteidx;		/* PTE index */
	int	apdc_ptecnt;		/* PTE count */
	u_long	apdc_sgva;		/* allocated sgva */
	bus_size_t apdc_len;		/* sgva size */
	int	apdc_flags;
};

/* apdc_flags */
#define	APDC_HAS_SGMAP		0x01	/* sgva/len are valid */

void	alpha_sgmap_init __P((bus_dma_tag_t, struct alpha_sgmap *,
	    const char *, bus_addr_t, bus_addr_t, bus_size_t, size_t, void *));

int	alpha_sgmap_alloc __P((bus_dmamap_t, bus_size_t,
	    struct alpha_sgmap *, int));
void	alpha_sgmap_free __P((struct alpha_sgmap *,
	    struct alpha_sgmap_cookie *));
