/*	$NetBSD: algor_p6032_dma.c,v 1.5 2009/10/26 00:59:39 snj Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 * Platform-specific DMA support for the Algorithmics P-6032.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: algor_p6032_dma.c,v 1.5 2009/10/26 00:59:39 snj Exp $");

#include <sys/param.h>

#define	_ALGOR_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <algor/algor/algor_p6032reg.h>
#include <algor/algor/algor_p6032var.h>

#include <dev/isa/isavar.h>

void
algor_p6032_dma_init(struct p6032_config *acp)
{
	bus_dma_tag_t t;

	/*
	 * Initialize the DMA tag used for PCI DMA.
	 */
	t = &acp->ac_pci_dmat;
	t->_cookie = acp;
	t->_wbase = P6032_DMA_PCI_PCIBASE;
	t->_physbase = P6032_DMA_PCI_PHYSBASE;
	t->_wsize = P6032_DMA_PCI_SIZE;
	t->_dmamap_create = _bus_dmamap_create;
	t->_dmamap_destroy = _bus_dmamap_destroy;
	t->_dmamap_load = _bus_dmamap_load;
	t->_dmamap_load_mbuf = _bus_dmamap_load_mbuf;
	t->_dmamap_load_uio = _bus_dmamap_load_uio;
	t->_dmamap_load_raw = _bus_dmamap_load_raw;
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
	t = &acp->ac_isa_dmat;
	t->_cookie = acp;
	t->_wbase = P6032_DMA_ISA_PCIBASE;
	t->_physbase = P6032_DMA_ISA_PHYSBASE;
	t->_wsize = P6032_DMA_ISA_SIZE;

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
}
