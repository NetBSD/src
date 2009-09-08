/*	$NetBSD: if_le.c,v 1.37 2009/09/08 18:15:17 tsutsui Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center; Paul Kranenburg.
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
__KERNEL_RCSID(0, "$NetBSD: if_le.c,v 1.37 2009/09/08 18:15:17 tsutsui Exp $");

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <sys/bus.h>
#include <sys/intr.h>
#include <machine/autoconf.h>

#include <dev/sbus/sbusvar.h>
#include <dev/sbus/lebuffervar.h>	/*XXX*/

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#include "ioconf.h"

/*
 * LANCE registers.
 */
#define LEREG1_RDP	0	/* Register Data port */
#define LEREG1_RAP	2	/* Register Address port */

struct	le_softc {
	struct	am7990_softc	sc_am7990;	/* glue to MI code */
	struct	sbusdev		sc_sd;		/* sbus device */
	bus_space_tag_t		sc_bustag;
	bus_dma_tag_t		sc_dmatag;
	bus_dmamap_t		sc_dmamap;
	bus_space_handle_t	sc_reg;
};

#define MEMSIZE 0x4000		/* LANCE memory size */

int	lematch_sbus(device_t, cfdata_t, void *);
void	leattach_sbus(device_t, device_t, void *);

static void le_sbus_reset(device_t);

/*
 * Media types supported.
 */
static int lemedia[] = {
	IFM_ETHER|IFM_10_5,
};
#define NLEMEDIA	__arraycount(lemedia)

CFATTACH_DECL_NEW(le_sbus, sizeof(struct le_softc),
    lematch_sbus, leattach_sbus, NULL, NULL);

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#endif

static void lewrcsr(struct lance_softc *, uint16_t, uint16_t);
static uint16_t lerdcsr(struct lance_softc *, uint16_t);

static void
lewrcsr(struct lance_softc *sc, uint16_t port, uint16_t val)
{
	struct le_softc *lesc = (struct le_softc *)sc;
	bus_space_tag_t t = lesc->sc_bustag;
	bus_space_handle_t h = lesc->sc_reg;

	bus_space_write_2(t, h, LEREG1_RAP, port);
	bus_space_write_2(t, h, LEREG1_RDP, val);

#if defined(SUN4M)
	/*
	 * We need to flush the Sbus->Mbus write buffers. This can most
	 * easily be accomplished by reading back the register that we
	 * just wrote (thanks to Chris Torek for this solution).
	 */
	(void)bus_space_read_2(t, h, LEREG1_RDP);
#endif
}

static uint16_t
lerdcsr(struct lance_softc *sc, uint16_t port)
{
	struct le_softc *lesc = (struct le_softc *)sc;
	bus_space_tag_t t = lesc->sc_bustag;
	bus_space_handle_t h = lesc->sc_reg;

	bus_space_write_2(t, h, LEREG1_RAP, port);
	return (bus_space_read_2(t, h, LEREG1_RDP));
}


int
lematch_sbus(device_t parent, cfdata_t cf, void *aux)
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(cf->cf_name, sa->sa_name) == 0);
}

void
leattach_sbus(device_t parent, device_t self, void *aux)
{
	struct le_softc *lesc = device_private(self);
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	struct sbus_softc *sbsc = device_private(parent);
	struct sbus_attach_args *sa = aux;
	bus_dma_tag_t dmatag;
	struct sbusdev *sd;

	sc->sc_dev = self;
	lesc->sc_bustag = sa->sa_bustag;
	lesc->sc_dmatag = dmatag = sa->sa_dmatag;

	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot,
			 sa->sa_offset,
			 sa->sa_size,
			 0, &lesc->sc_reg) != 0) {
		aprint_error(": cannot map registers\n");
		return;
	}

	/*
	 * Look for an "unallocated" lebuffer and pair it with
	 * this `le' device on the assumption that we're on
	 * a pre-historic ROM that doesn't establish le<=>lebuffer
	 * parent-child relationships.
	 */
	for (sd = sbsc->sc_sbdev; sd != NULL; sd = sd->sd_bchain) {

		struct lebuf_softc *lebuf = device_private(sd->sd_dev);

		if (strncmp("lebuffer", device_xname(sd->sd_dev), 8) != 0)
			continue;

		if (lebuf->attached != 0)
			continue;

		sc->sc_mem = lebuf->sc_buffer;
		sc->sc_memsize = lebuf->sc_bufsiz;
		sc->sc_addr = 0; /* Lance view is offset by buffer location */
		lebuf->attached = 1;

		/* That old black magic... */
		sc->sc_conf3 = prom_getpropint(sa->sa_node,
					  "busmaster-regval",
					  LE_C3_BSWP | LE_C3_ACON | LE_C3_BCON);
		break;
	}

	lesc->sc_sd.sd_reset = le_sbus_reset;
	sbus_establish(&lesc->sc_sd, self);

	if (sc->sc_mem == 0) {
		bus_dma_segment_t seg;
		int rseg, error;

#ifndef BUS_DMA_24BIT
/* XXX - This flag is not defined on all archs */
#define BUS_DMA_24BIT	0
#endif
		/* Get a DMA handle */
		if ((error = bus_dmamap_create(dmatag, MEMSIZE, 1, MEMSIZE, 0,
						BUS_DMA_NOWAIT|BUS_DMA_24BIT,
						&lesc->sc_dmamap)) != 0) {
			aprint_error(": DMA map create error %d\n", error);
			return;
		}

		/* Allocate DMA buffer */
		if ((error = bus_dmamem_alloc(dmatag, MEMSIZE, 0, 0,
					 &seg, 1, &rseg,
					 BUS_DMA_NOWAIT|BUS_DMA_24BIT)) != 0){
			aprint_error(": DMA buffer allocation error %d\n",
			    error);
			return;
		}

		/* Map DMA buffer into kernel space */
		if ((error = bus_dmamem_map(dmatag, &seg, rseg, MEMSIZE,
				       (void **)&sc->sc_mem,
				       BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
			aprint_error(": DMA buffer map error %d\n", error);
			bus_dmamem_free(lesc->sc_dmatag, &seg, rseg);
			return;
		}

		/* Load DMA buffer */
		if ((error = bus_dmamap_load(dmatag, lesc->sc_dmamap,
		    sc->sc_mem, MEMSIZE, NULL, BUS_DMA_NOWAIT)) != 0) {
			aprint_error(": DMA buffer map load error %d\n", error);
			bus_dmamem_free(dmatag, &seg, rseg);
			bus_dmamem_unmap(dmatag, sc->sc_mem, MEMSIZE);
			return;
		}

		sc->sc_addr = lesc->sc_dmamap->dm_segs[0].ds_addr & 0xffffff;
		sc->sc_memsize = MEMSIZE;
		sc->sc_conf3 = LE_C3_BSWP | LE_C3_ACON | LE_C3_BCON;
	}

	prom_getether(sa->sa_node, sc->sc_enaddr);

	sc->sc_supmedia = lemedia;
	sc->sc_nsupmedia = NLEMEDIA;
	sc->sc_defaultmedia = lemedia[0];

	sc->sc_copytodesc = lance_copytobuf_contig;
	sc->sc_copyfromdesc = lance_copyfrombuf_contig;
	sc->sc_copytobuf = lance_copytobuf_contig;
	sc->sc_copyfrombuf = lance_copyfrombuf_contig;
	sc->sc_zerobuf = lance_zerobuf_contig;

	sc->sc_rdcsr = lerdcsr;
	sc->sc_wrcsr = lewrcsr;

	am7990_config(&lesc->sc_am7990);

	/* Establish interrupt handler */
	if (sa->sa_nintr != 0)
		(void)bus_intr_establish(lesc->sc_bustag, sa->sa_pri,
					 IPL_NET, am7990_intr, sc);
}

void
le_sbus_reset(device_t self)
{
	struct le_softc *lesc = device_private(self);
	struct lance_softc *sc = &lesc->sc_am7990.lsc;

	lance_reset(sc);
}
