/*	$NetBSD: ixp12x0_pci_dma.c,v 1.6 2003/03/25 06:12:47 igy Exp $ */
/*
 * Copyright (c) 2002, 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixp12x0_pci_dma.c,v 1.6 2003/03/25 06:12:47 igy Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <uvm/uvm_extern.h>

#define _ARM32_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <arm/ixp12x0/ixp12x0_pcireg.h>
#include <arm/ixp12x0/ixp12x0var.h>

void
ixp12x0_pci_dma_init(struct ixp12x0_softc *sc)
{
	extern paddr_t physical_start, physical_end;

	bus_dma_tag_t dmat = &sc->ia_pci_dmat;
	struct arm32_dma_range *dr = &sc->ia_pci_dma_range;

	dmat->_ranges = dr;
	dmat->_nranges = 1;

	dr->dr_sysbase = physical_start;
	dr->dr_busbase = PCI_MAPREG_MEM_ADDR(IXP1200_PCI_MEM_BAR +
			 physical_start);
	dr->dr_len = physical_end - physical_start;

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
