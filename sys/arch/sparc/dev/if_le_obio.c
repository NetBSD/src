/*	$NetBSD: if_le_obio.c,v 1.18 2003/04/02 04:35:27 thorpej Exp $	*/

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

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/socket.h>

#include <uvm/uvm_extern.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/autoconf.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

/*
 * LANCE registers.
 */
#define LEREG1_RDP	0	/* Register Data port */
#define LEREG1_RAP	2	/* Register Address port */

struct	le_softc {
	struct	am7990_softc	sc_am7990;	/* glue to MI code */
	bus_space_tag_t		sc_bustag;
	bus_dma_tag_t		sc_dmatag;
	bus_dmamap_t		sc_dmamap;
	bus_space_handle_t	sc_reg;		/* LANCE registers */
};

#define MEMSIZE 0x4000		/* LANCE memory size */

int	lematch_obio __P((struct device *, struct cfdata *, void *));
void	leattach_obio __P((struct device *, struct device *, void *));

/*
 * Media types supported.
 */
static int lemedia[] = {
	IFM_ETHER|IFM_10_T,
};
#define NLEMEDIA	(sizeof(lemedia) / sizeof(lemedia[0]))

CFATTACH_DECL(le_obio, sizeof(struct le_softc),
    lematch_obio, leattach_obio, NULL, NULL);

extern struct cfdriver le_cd;

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#endif

#ifdef DDB
#define	integrate
#define hide
#else
#define	integrate	static __inline
#define hide		static
#endif

static void lewrcsr __P((struct lance_softc *, u_int16_t, u_int16_t));
static u_int16_t lerdcsr __P((struct lance_softc *, u_int16_t));

static void
lewrcsr(sc, port, val)
	struct lance_softc *sc;
	u_int16_t port, val;
{
	struct le_softc *lesc = (struct le_softc *)sc;

	bus_space_write_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RAP, port);
	bus_space_write_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RDP, val);
}

static u_int16_t
lerdcsr(sc, port)
	struct lance_softc *sc;
	u_int16_t port;
{
	struct le_softc *lesc = (struct le_softc *)sc;

	bus_space_write_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RAP, port);
	return (bus_space_read_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RDP));
}

int
lematch_obio(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (0);

	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
				2,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

void
leattach_obio(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;
	struct le_softc *lesc = (struct le_softc *)self;
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	bus_dma_segment_t seg;
	bus_dma_tag_t dmatag;
	int rseg;
	int error;
	/* XXX the following declarations should be elsewhere */
	extern void myetheraddr __P((u_char *));

	lesc->sc_bustag = oba->oba_bustag;
	lesc->sc_dmatag = dmatag = oba->oba_dmatag;

	if (bus_space_map(oba->oba_bustag, oba->oba_paddr,
			  2 * sizeof(u_int16_t),
			  0, &lesc->sc_reg) != 0) {
		printf("%s @ obio: cannot map registers\n", self->dv_xname);
		return;
	}

	/* Get a DMA handle */
	if ((error = bus_dmamap_create(dmatag, MEMSIZE, 1, MEMSIZE, 0,
					BUS_DMA_NOWAIT|BUS_DMA_24BIT,
					&lesc->sc_dmamap)) != 0) {
		printf("%s: DMA map create error %d\n",
			self->dv_xname, error);
		return;
	}

	/* Allocate DMA buffer */
	if ((error = bus_dmamem_alloc(dmatag, MEMSIZE, PAGE_SIZE, 0,
			     &seg, 1, &rseg,
			     BUS_DMA_NOWAIT | BUS_DMA_24BIT)) != 0) {
		printf("%s: DMA memory allocation error %d\n",
			self->dv_xname, error);
		return;
	}
	/* Map DMA buffer into kernel space */
	if ((error = bus_dmamem_map(dmatag, &seg, rseg, MEMSIZE,
			   (caddr_t *)&sc->sc_mem,
			   BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s: DMA memory map error %d\n", self->dv_xname, error);
		bus_dmamem_free(lesc->sc_dmatag, &seg, rseg);
		return;
	}
	/* Load DMA buffer */
	if ((error = bus_dmamap_load(dmatag, lesc->sc_dmamap,
				     sc->sc_mem, MEMSIZE, NULL,
				     BUS_DMA_NOWAIT)) != 0) {
		printf("%s: DMA buffer map load error %d\n",
			self->dv_xname, error);
		bus_dmamem_unmap(dmatag, (caddr_t)sc->sc_mem, MEMSIZE);
		bus_dmamem_free(dmatag, &seg, rseg);
		return;
	}

	sc->sc_addr = lesc->sc_dmamap->dm_segs[0].ds_addr & 0xffffff;
	sc->sc_memsize = MEMSIZE;
	sc->sc_conf3 = LE_C3_BSWP | LE_C3_ACON | LE_C3_BCON;

	sc->sc_supmedia = lemedia;
	sc->sc_nsupmedia = NLEMEDIA;
	sc->sc_defaultmedia = lemedia[0];

	myetheraddr(sc->sc_enaddr);

	sc->sc_copytodesc = lance_copytobuf_contig;
	sc->sc_copyfromdesc = lance_copyfrombuf_contig;
	sc->sc_copytobuf = lance_copytobuf_contig;
	sc->sc_copyfrombuf = lance_copyfrombuf_contig;
	sc->sc_zerobuf = lance_zerobuf_contig;

	sc->sc_rdcsr = lerdcsr;
	sc->sc_wrcsr = lewrcsr;

	am7990_config(&lesc->sc_am7990);

	/* Install interrupt */
	(void)bus_intr_establish(lesc->sc_bustag, oba->oba_pri, IPL_NET,
				 am7990_intr, sc);
}
