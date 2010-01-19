/*	$NetBSD: if_le_sbdio.c,v 1.6 2010/01/19 22:06:20 pooka Exp $	*/

/*-
 * Copyright (c) 1996, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_le_sbdio.c,v 1.6 2010/01/19 22:06:20 pooka Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/sbdiovar.h>
#include <machine/sbdvar.h>	/* for ether_addr() */

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#define	LEREG1_RDP	0	/* offset to lance data register */
#define	LEREG1_RAP	6	/* offset to lance address regsiter */
#define	LE_MEMSIZE	(64 * 1024)

struct le_sbdio_softc {
	struct am7990_softc sc_am7990;

	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	bus_dmamap_t sc_dmamap;
};

int le_sbdio_match(device_t, struct cfdata *, void *);
void le_sbdio_attach(device_t, struct device *, void *);
static void le_sbdio_wrcsr(struct lance_softc *, uint16_t, uint16_t);
static uint16_t le_sbdio_rdcsr(struct lance_softc *, uint16_t);

CFATTACH_DECL_NEW(le_sbdio, sizeof(struct le_sbdio_softc),
    le_sbdio_match, le_sbdio_attach, NULL, NULL);

int
le_sbdio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct sbdio_attach_args *sa = aux;

	return strcmp(sa->sa_name, "lance") ? 0 : 1;
}

void
le_sbdio_attach(device_t parent, device_t self, void *aux)
{
	struct le_sbdio_softc *lesc = device_private(self);
	struct sbdio_attach_args *sa = aux;
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	bus_dma_segment_t seg;
	int rseg;

	sc->sc_dev = self;
	lesc->sc_dmat = sa->sa_dmat;
	lesc->sc_bst  = sa->sa_bust;

	if (bus_space_map(lesc->sc_bst, sa->sa_addr1, 8 /* XXX */,
	    BUS_SPACE_MAP_LINEAR, &lesc->sc_bsh) != 0) {
		aprint_error(": cannot map registers\n");
		return;
	}

	/* Allocate DMA memory for the chip. */
	if (bus_dmamem_alloc(lesc->sc_dmat, LE_MEMSIZE, 0, 0, &seg, 1, &rseg,
	    BUS_DMA_NOWAIT) != 0) {
		aprint_error(": can't allocate DMA memory\n");
		return;
	}
	if (bus_dmamem_map(lesc->sc_dmat, &seg, rseg, LE_MEMSIZE,
	    (void **)&sc->sc_mem, BUS_DMA_NOWAIT|BUS_DMA_COHERENT) != 0) {
		aprint_error(": can't map DMA memory\n");
		return;
	}
	if (bus_dmamap_create(lesc->sc_dmat, LE_MEMSIZE, 1, LE_MEMSIZE,
	    0, BUS_DMA_NOWAIT, &lesc->sc_dmamap) != 0) {
		aprint_error(": can't create DMA map\n");
		return;
	}
	if (bus_dmamap_load(lesc->sc_dmat, lesc->sc_dmamap, sc->sc_mem,
	    LE_MEMSIZE, NULL, BUS_DMA_NOWAIT) != 0) {
		aprint_error(": can't load DMA map\n");
		return;
	}

	sc->sc_memsize = LE_MEMSIZE;
	sc->sc_addr = lesc->sc_dmamap->dm_segs[0].ds_addr;
	sc->sc_conf3 = LE_C3_BSWP | LE_C3_BCON;
	(*platform.ether_addr)(sc->sc_enaddr);

	sc->sc_copytodesc = lance_copytobuf_contig;
	sc->sc_copyfromdesc = lance_copyfrombuf_contig;
	sc->sc_copytobuf = lance_copytobuf_contig;
	sc->sc_copyfrombuf = lance_copyfrombuf_contig;
	sc->sc_zerobuf = lance_zerobuf_contig;
#ifdef LEDEBUG
	sc->sc_debug = 0xff;
#endif
	sc->sc_rdcsr = le_sbdio_rdcsr;
	sc->sc_wrcsr = le_sbdio_wrcsr;

	am7990_config(&lesc->sc_am7990);
	intr_establish(sa->sa_irq, am7990_intr, sc);
}

static void
le_sbdio_wrcsr(struct lance_softc *sc, uint16_t port, uint16_t val)
{
	struct le_sbdio_softc *lesc = (struct le_sbdio_softc *)sc;

	bus_space_write_2(lesc->sc_bst, lesc->sc_bsh, LEREG1_RAP, port);
	bus_space_write_2(lesc->sc_bst, lesc->sc_bsh, LEREG1_RDP, val);
}

static uint16_t
le_sbdio_rdcsr(struct lance_softc *sc, uint16_t port)
{
	struct le_sbdio_softc *lesc = (struct le_sbdio_softc *)sc;

	bus_space_write_2(lesc->sc_bst, lesc->sc_bsh, LEREG1_RAP, port);
	return bus_space_read_2(lesc->sc_bst, lesc->sc_bsh, LEREG1_RDP);
}
