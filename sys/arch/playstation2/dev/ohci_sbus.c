/*	$NetBSD: ohci_sbus.c,v 1.4 2003/07/15 02:54:36 lukem Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ohci_sbus.c,v 1.4 2003/07/15 02:54:36 lukem Exp $");

#include <sys/param.h>

/* bus_dma */
#include <sys/mbuf.h>
#include <uvm/uvm_extern.h>

#define _PLAYSTATION2_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

#include <playstation2/ee/sifvar.h>	/* DMA staff */
#include <playstation2/ee/dmacvar.h>
#include <playstation2/dev/sbusvar.h>

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

#define	SBUS_OHCI_REGBASE	MIPS_PHYS_TO_KSEG1(0x1f801600)
#define SBUS_OHCI_REGSIZE	0x1000

STATIC int ohci_sbus_match(struct device *, struct cfdata *, void *);
STATIC void ohci_sbus_attach(struct device *, struct device *, void *);

STATIC void _ohci_sbus_map_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
    bus_size_t, int);
STATIC int _ohci_sbus_mem_alloc(bus_dma_tag_t, bus_size_t, bus_size_t,
    bus_size_t, bus_dma_segment_t *, int, int *, int);
STATIC void _ohci_sbus_mem_free(bus_dma_tag_t, bus_dma_segment_t *, int);
STATIC int _ohci_sbus_mem_map(bus_dma_tag_t, bus_dma_segment_t *, int, size_t,
    caddr_t *, int);
STATIC void _ohci_sbus_mem_unmap(bus_dma_tag_t, caddr_t, size_t);

struct playstation2_bus_dma_tag ohci_bus_dma_tag = {
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_ohci_sbus_map_sync,
	_ohci_sbus_mem_alloc,
	_ohci_sbus_mem_free,
	_ohci_sbus_mem_map,
	_ohci_sbus_mem_unmap,
	_bus_dmamem_mmap,
};

struct ohci_dma_segment {
	struct iopdma_segment ds_iopdma_seg;

	LIST_ENTRY(ohci_dma_segment) ds_link;
};

struct ohci_sbus_softc {
	struct ohci_softc sc;

	LIST_HEAD(, ohci_dma_segment) sc_dmaseg_head;
};

CFATTACH_DECL(ohci_sbus, sizeof(struct ohci_sbus_softc),
    ohci_sbus_match, ohci_sbus_attach, NULL, NULL);

int
ohci_sbus_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

void
ohci_sbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct ohci_sbus_softc *sc = (void *)self;
	usbd_status result;

	printf("\n");

	sc->sc.iot = bus_space_create(0, "OHCI I/O space", SBUS_OHCI_REGBASE,
	    SBUS_OHCI_REGSIZE);
	sc->sc.ioh = SBUS_OHCI_REGBASE;

	ohci_bus_dma_tag._dmachip_cookie = sc;
	sc->sc.sc_bus.dmatag = &ohci_bus_dma_tag;

	/* Disable interrupts, so we don't can any spurious ones. */
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OHCI_INTERRUPT_DISABLE,
	    OHCI_ALL_INTRS);

	sbus_intr_establish(SBUS_IRQ_USB, ohci_intr, sc);

	/* IOP/EE DMA relay segment list */
	LIST_INIT(&sc->sc_dmaseg_head);

	result = ohci_init(&sc->sc);
	
	if (result != USBD_NORMAL_COMPLETION) {
		printf(": init failed. error=%d\n", result);
		return;
	}

	/* Attach usb device. */
	sc->sc.sc_child = config_found((void *)sc, &sc->sc.sc_bus,
	    usbctlprint);
}

void
_ohci_sbus_map_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{

	dmac_sync_buffer(); /* XXX over flush */
}

int
_ohci_sbus_mem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags)
{
	struct ohci_sbus_softc *sc = t->_dmachip_cookie;
	struct ohci_dma_segment *ds;
	struct iopdma_segment *iopdma_seg;
	int error;

	KDASSERT(sc);
	ds = malloc(sizeof(struct ohci_dma_segment), M_DEVBUF, M_NOWAIT);
	if (ds == NULL)
		return (1);
	/*
	 * Allocate DMA Area (IOP DMA Area <-> SIF DMA <-> EE DMA Area)
	 */
	iopdma_seg = &ds->ds_iopdma_seg;
	error = iopdma_allocate_buffer(iopdma_seg, size);

	if (error) {
		free(ds, M_DEVBUF);
		return (1);
	}

	segs[0].ds_len	  = iopdma_seg->size;
	segs[0].ds_addr	  = iopdma_seg->iop_paddr;
	segs[0]._ds_vaddr = iopdma_seg->ee_vaddr;

	LIST_INSERT_HEAD(&sc->sc_dmaseg_head, ds, ds_link);

	*rsegs = 1;

	return (0);
}

void
_ohci_sbus_mem_free(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs)
{
	struct ohci_sbus_softc *sc = t->_dmachip_cookie;
	struct ohci_dma_segment *ds;
	paddr_t addr = segs[0].ds_addr;

	for (ds = LIST_FIRST(&sc->sc_dmaseg_head); ds != NULL;
	    ds = LIST_NEXT(ds, ds_link)) {
		if (ds->ds_iopdma_seg.iop_paddr == addr) {
			iopdma_free_buffer(&ds->ds_iopdma_seg);

			LIST_REMOVE(ds, ds_link);
			free(ds, M_DEVBUF);
			return;
		}
	}

	panic("_dmamem_free: can't find corresponding handle.");
	/* NOTREACHED */
}

int
_ohci_sbus_mem_map(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs, size_t size,
    caddr_t *kvap, int flags)
{
	struct ohci_sbus_softc *sc = t->_dmachip_cookie;
	struct ohci_dma_segment *ds;
	paddr_t addr = segs[0].ds_addr;

	for (ds = LIST_FIRST(&sc->sc_dmaseg_head); ds != NULL;
	    ds = LIST_NEXT(ds, ds_link)) {
		if (ds->ds_iopdma_seg.iop_paddr == addr) {

			*kvap = (caddr_t)ds->ds_iopdma_seg.ee_vaddr;

			return (0);
		}
	}

	return (1);
}

void
_ohci_sbus_mem_unmap(bus_dma_tag_t t, caddr_t kva, size_t size)
{
	/* nothing to do */
}
