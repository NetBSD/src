/*	$NetBSD: int_bus_dma.c,v 1.12 2003/07/15 00:25:00 lukem Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
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
 * PCI DMA support for the ARM Integrator.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: int_bus_dma.c,v 1.12 2003/07/15 00:25:00 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <uvm/uvm_extern.h>

#define	_ARM32_BUS_DMA_PRIVATE
#include <evbarm/integrator/int_bus_dma.h>

static struct arm32_dma_range integrator_dma_ranges[1];

void
integrator_pci_dma_init(bus_dma_tag_t dmat)
{
	struct arm32_dma_range *dr = integrator_dma_ranges;
	extern paddr_t physical_start, physical_end;

	dr->dr_sysbase = physical_start;
	dr->dr_busbase = LOCAL_TO_CM_ALIAS(dr->dr_sysbase);
	dr->dr_len = physical_end - physical_start;

	dmat->_ranges = dr;
	dmat->_nranges = 1;

	dmat->_dmamap_create = _bus_dmamap_create;
	dmat->_dmamap_destroy = _bus_dmamap_destroy;
	dmat->_dmamap_load = _bus_dmamap_load;
	dmat->_dmamap_load_mbuf = _bus_dmamap_load_mbuf;
	dmat->_dmamap_load_uio = _bus_dmamap_load_uio;
	dmat->_dmamap_load_raw = _bus_dmamap_load_raw;
	dmat->_dmamap_unload = _bus_dmamap_unload;
	dmat->_dmamap_sync_pre = _bus_dmamap_sync;
	dmat->_dmamap_sync_post = NULL;

	dmat->_dmamem_alloc = _bus_dmamem_alloc;
	dmat->_dmamem_free = _bus_dmamem_free;
	dmat->_dmamem_map = _bus_dmamem_map;
	dmat->_dmamem_unmap = _bus_dmamem_unmap;
	dmat->_dmamem_mmap = _bus_dmamem_mmap;
}
