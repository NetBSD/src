/* $NetBSD: a12c_dma.c,v 1.3 2000/06/29 08:58:45 mrg Exp $ */

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

/*
 * Stub version -- drivers used on the A12 (primarily ade) were adapted
 *		   for NetBSD prior to the availability of bus_dma.
 *		   They employ ad-hoc support for the required sram
 *		   bounce buffers and are not in the NetBSD source
 *		   tree.  When a bus_dma if_de is available, this
 *		   module will need to be completed!  I certainly
 *		   would not mind committing if_ade, but it doesn't
 *		   have any lasting value.
 */

#include "opt_avalon_a12.h"		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: a12c_dma.c,v 1.3 2000/06/29 08:58:45 mrg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#define _ALPHA_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <alpha/pci/a12creg.h>
#include <alpha/pci/a12cvar.h>

#define	A12C_DMA()	/* Generate ctags(1) key */

bus_dma_tag_t a12c_dma_get_tag __P((bus_dma_tag_t, alpha_bus_t));


int	a12c_bus_dmamap_load_direct __P((bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int));

int	a12c_bus_dmamap_load_mbuf_direct __P((bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int));

int	a12c_bus_dmamap_load_uio_direct __P((bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int));

int	a12c_bus_dmamap_load_raw_direct __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int));

void
a12c_dma_init(ccp)
	struct a12c_config *ccp;
{
}

/*
 * Return the bus dma tag to be used for the specified bus type.
 * INTERNAL USE ONLY!
 */
bus_dma_tag_t
a12c_dma_get_tag(t, bustype)
	bus_dma_tag_t t;
	alpha_bus_t bustype;
{
	DIE();
}
/*
 * Load a A12C direct-mapped DMA map with a linear buffer.
 */
int
a12c_bus_dmamap_load_direct(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	DIE();
#if 0

	return (_bus_dmamap_load_direct_common(t, map, buf, buflen, p,
	    flags, A12C_DIRECT_MAPPED_BASE));
#endif
}
/*
 * Load a A12C direct-mapped DMA map with an mbuf chain.
 */
int
a12c_bus_dmamap_load_mbuf_direct(t, map, m, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m;
	int flags;
{
	DIE();
#if 0

	return (_bus_dmamap_load_mbuf_direct_common(t, map, m,
	    flags, A12C_DIRECT_MAPPED_BASE));
#endif
}
/*
 * Load a A12C direct-mapped DMA map with a uio.
 */
int
a12c_bus_dmamap_load_uio_direct(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{
	DIE();
#if 0

	return (_bus_dmamap_load_uio_direct_common(t, map, uio,
	    flags, A12C_DIRECT_MAPPED_BASE));
#endif
}
/*
 * Load a A12C direct-mapped DMA map with raw memory.
 */
int
a12c_bus_dmamap_load_raw_direct(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{
	DIE();
#if 0

	return (_bus_dmamap_load_raw_direct_common(t, map, segs, nsegs,
	    size, flags, A12C_DIRECT_MAPPED_BASE));
#endif
}
