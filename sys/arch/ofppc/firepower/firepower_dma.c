/*	$NetBSD: firepower_dma.c,v 1.2 2003/07/15 02:46:31 lukem Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * DMA support for Firepower systems.  Nothing special, here.  We just
 * use the generic PowerPC DMA routines for PCI DMA, and supply a set
 * of bounce-capable DMA routines for ISA DMA.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: firepower_dma.c,v 1.2 2003/07/15 02:46:31 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#define	_POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h> 

#include <ofppc/firepower/firepowervar.h>

void
firepower_dma_init(struct firepower_config *cp)
{
	bus_dma_tag_t t;

	/*
	 * Initialize the DMA tag used for PCI DMA.
	 */
	t = &cp->c_dmat_pci;
	t->_cookie = NULL;	/* not used */
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

#if 0
	/*
	 * Initialize the DMA tag used for ISA DMA.
	 */
	t = &cp->c_dmat_isa;
	t->_cookie = NULL;	/* not used */
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
#endif
}
