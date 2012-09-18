/*	$NetBSD: int_bus_dma.c,v 1.18 2012/09/18 14:42:19 matt Exp $	*/

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

#define	_ARM32_BUS_DMA_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: int_bus_dma.c,v 1.18 2012/09/18 14:42:19 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>

#include <evbarm/integrator/int_bus_dma.h>

#undef	DEBUG
#define DEBUG(x)

static struct arm32_dma_range integrator_dma_ranges[DRAM_BLOCKS];

extern BootConfig bootconfig;

void
integrator_pci_dma_init(bus_dma_tag_t dmat)
{
	struct arm32_dma_range *dr = integrator_dma_ranges;
	int i;
	int nranges = 0;
	
	for (i = 0; i < bootconfig.dramblocks; i++) {
		if (bootconfig.dram[i].flags & BOOT_DRAM_CAN_DMA) {
			dr[nranges].dr_sysbase = bootconfig.dram[i].address;
			dr[nranges].dr_busbase = 
			    LOCAL_TO_CM_ALIAS(dr[nranges].dr_sysbase);
			dr[nranges].dr_len = bootconfig.dram[i].pages * NBPG;
			nranges++;
		}
	}

	if (nranges == 0)
		panic ("integrator_pci_dma_init: No DMA capable memory"); 

	dmat->_ranges = dr;
	dmat->_nranges = nranges;

	dmat->_dmamap_create = _bus_dmamap_create;
	dmat->_dmamap_destroy = _bus_dmamap_destroy;
	dmat->_dmamap_load = _bus_dmamap_load;
	dmat->_dmamap_load_mbuf = _bus_dmamap_load_mbuf;
	dmat->_dmamap_load_uio = _bus_dmamap_load_uio;
	dmat->_dmamap_load_raw = _bus_dmamap_load_raw;
	dmat->_dmamap_unload = _bus_dmamap_unload;
	dmat->_dmamap_sync_pre = _bus_dmamap_sync;
	dmat->_dmamap_sync_post = _bus_dmamap_sync;

	dmat->_dmamem_alloc = _bus_dmamem_alloc;
	dmat->_dmamem_free = _bus_dmamem_free;
	dmat->_dmamem_map = _bus_dmamem_map;
	dmat->_dmamem_unmap = _bus_dmamem_unmap;
	dmat->_dmamem_mmap = _bus_dmamem_mmap;

	dmat->_dmatag_subregion = _bus_dmatag_subregion;
	dmat->_dmatag_destroy = _bus_dmatag_destroy;
}
