/*	$NetBSD: smdk2800_pci.c,v 1.3 2003/07/30 18:17:16 bsh Exp $ */

/*
 * Copyright (c) 2002 Fujitsu Component Limited
 * Copyright (c) 2002 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Platform specific part for S3C2800 PCI support on SMDK2800
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: smdk2800_pci.c,v 1.3 2003/07/30 18:17:16 bsh Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/extent.h>

#define _ARM32_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <arm/s3c2xx0/s3c2800var.h>

static struct arm32_dma_range smdk2800_dma_ranges[1];

static struct arm32_bus_dma_tag smdk2800_bus_dma = {
	NULL,			/* _ranges: set by init routine */
	0,			/* _nranges */

	NULL,			/* _cookie */

	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	NULL,			/* sync_post */

	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

bus_dma_tag_t
s3c2800_pci_dma_init(void)
{
	extern paddr_t physical_start, physical_end;

	smdk2800_dma_ranges[0].dr_sysbase = physical_start;
	smdk2800_dma_ranges[0].dr_busbase =
	    0x80000000 + physical_start - S3C2800_DBANK0_START;
	smdk2800_dma_ranges[0].dr_len = physical_end - physical_start;

	smdk2800_bus_dma._ranges = smdk2800_dma_ranges;
	smdk2800_bus_dma._nranges = 1;

	return &smdk2800_bus_dma;
}
