/* $NetBSD: jensenio_dma.c,v 1.1 2000/07/12 20:36:09 thorpej Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * DMA support for the Jensen system.
 *
 * The Jensen uses direct-mapped DMA with a window base of 0, i.e.
 * a page of phyical memory has the same PCI address as CPU address.
 * There is no support for SGMAPs on the Jensen.  This means that for
 * ISA DMA, we must employ bounce buffers for DMA transactions over
 * 16MB ISA limit (due to ISA's 24-bit address space limitation).
 *
 * Furthermore, the H_BUS will not respond to DMA transactions between
 * 512KB and 1MB.  This is to allow for the ISA `hole'.  However, the
 * ISA `hole' typically exists only between 640KB and 1MB.  We must
 * therefore bounce transactions that fall within this region, as well.
 * XXX Should we prevent having managed memory in this region, instead?
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: jensenio_dma.c,v 1.1 2000/07/12 20:36:09 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#define _ALPHA_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <dev/eisa/eisavar.h>

#include <dev/isa/isavar.h>

#include <alpha/jensenio/jenseniovar.h>

bus_dma_tag_t jensenio_dma_get_tag(bus_dma_tag_t, alpha_bus_t);

void
jensenio_dma_init(struct jensenio_config *jcp)
{
	bus_dma_tag_t t;

	/*
	 * Initialize the DMA tag used for EISA DMA.
	 */
	t = &jcp->jc_dmat_eisa;
	t->_cookie = jcp;
	t->_wbase = 0;
	t->_wsize = 0x100000000UL;
	t->_next_window = NULL;
	t->_boundary = 0;
	t->_sgmap = NULL;
	t->_get_tag = jensenio_dma_get_tag;
	t->_dmamap_create = _bus_dmamap_create;
	t->_dmamap_destroy = _bus_dmamap_destroy;
	t->_dmamap_load = _bus_dmamap_load_direct;
	t->_dmamap_load_mbuf = _bus_dmamap_load_mbuf_direct;
	t->_dmamap_load_uio = _bus_dmamap_load_uio_direct;
	t->_dmamap_load_raw = _bus_dmamap_load_raw_direct;
	t->_dmamap_unload = _bus_dmamap_unload;
	t->_dmamap_sync = _bus_dmamap_sync;

	t->_dmamem_alloc = _bus_dmamem_alloc;
	t->_dmamem_free = _bus_dmamem_free;
	t->_dmamem_map = _bus_dmamem_map;
	t->_dmamem_unmap = _bus_dmamem_unmap;
	t->_dmamem_mmap = _bus_dmamem_mmap;

	/*
	 * Initialize the DMA tag used for ISA DMA.
	 */
	t = &jcp->jc_dmat_isa;
	t->_cookie = jcp;
	t->_wbase = 0;
	t->_wsize = 0x1000000;
	t->_next_window = NULL;
	t->_boundary = 0;
	t->_sgmap = NULL;
	t->_get_tag = jensenio_dma_get_tag; 
	t->_dmamap_create = isadma_bounce_dmamap_create;
	t->_dmamap_destroy = isadma_bounce_dmamap_destroy;
	t->_dmamap_load = isadma_bounce_dmamap_load;
	t->_dmamap_load_mbuf = isadma_bounce_dmamap_load_mbuf;
	t->_dmamap_load_uio = isadma_bounce_dmamap_load_uio;
	t->_dmamap_load_raw = isadma_bounce_dmamap_load_raw;
	t->_dmamap_unload = isadma_bounce_dmamap_unload;
	t->_dmamap_sync = isadma_bounce_dmamap_sync;

	t->_dmamem_alloc = isadma_bounce_dmamem_alloc;
	t->_dmamem_free = _bus_dmamem_free;
	t->_dmamem_map = _bus_dmamem_map;
	t->_dmamem_unmap = _bus_dmamem_unmap;
	t->_dmamem_mmap = _bus_dmamem_mmap;

	/* XXX XXX BEGIN XXX XXX */
	{							/* XXX */
		extern paddr_t alpha_XXX_dmamap_or;		/* XXX */
		alpha_XXX_dmamap_or = 0;			/* XXX */
	}
}

/*
 * Return the bus dma tag to be used fo rthe specified bus type.
 * INTERNAL USE ONLY!
 */
bus_dma_tag_t
jensenio_dma_get_tag(bus_dma_tag_t t, alpha_bus_t bustype)
{
	struct jensenio_config *jcp = t->_cookie;

	switch (bustype) {
	case ALPHA_BUS_EISA:
		/*
		 * Busses capable of 32-bit DMA get the EISA DMA tag.
		 */
		return (&jcp->jc_dmat_eisa);

	case ALPHA_BUS_ISA:
		/*
		 * Lame 24-bit busses have to bounce.
		 */
		return (&jcp->jc_dmat_isa);

	default:
		panic("jensenio_dma_get_tag: shouldn't be here, really...");
	}
}
