/*	$NetBSD: if_le_ledma.c,v 1.11 2000/07/09 20:57:43 pk Exp $	*/

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
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/sbus/sbusvar.h>

#include <dev/ic/lsi64854reg.h>
#include <dev/ic/lsi64854var.h>

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
	struct	sbusdev		sc_sd;		/* sbus device */
	bus_space_tag_t		sc_bustag;
	bus_dmamap_t		sc_dmamap;
	bus_space_handle_t	sc_reg;		/* LANCE registers */
	struct	lsi64854_softc	*sc_dma;	/* pointer to my dma */
	u_int			sc_laddr;	/* LANCE DMA address */
};

#define MEMSIZE		(16*1024)	/* LANCE memory size */
#define LEDMA_BOUNDARY	(16*1024*1024)	/* must not cross 16MB boundary */

int	lematch_ledma __P((struct device *, struct cfdata *, void *));
void	leattach_ledma __P((struct device *, struct device *, void *));

/*
 * Media types supported by the Sun4m.
 */
static int lemedia[] = {
	IFM_ETHER|IFM_10_T,
	IFM_ETHER|IFM_10_5,
	IFM_ETHER|IFM_AUTO,
};
#define NLEMEDIA	(sizeof(lemedia) / sizeof(lemedia[0]))

void	lesetutp __P((struct lance_softc *));
void	lesetaui __P((struct lance_softc *));

int	lemediachange __P((struct lance_softc *));
void	lemediastatus __P((struct lance_softc *, struct ifmediareq *));

struct cfattach le_ledma_ca = {
	sizeof(struct le_softc), lematch_ledma, leattach_ledma
};

extern struct cfdriver le_cd;

#if defined(_KERNEL) && !defined(_LKM)
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
hide void lehwreset __P((struct lance_softc *));
hide void lehwinit __P((struct lance_softc *));
hide void lenocarrier __P((struct lance_softc *));

static void
lewrcsr(sc, port, val)
	struct lance_softc *sc;
	u_int16_t port, val;
{
	struct le_softc *lesc = (struct le_softc *)sc;

	bus_space_write_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RAP, port);
	bus_space_write_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RDP, val);

#if defined(SUN4M)
	/*
	 * We need to flush the Sbus->Mbus write buffers. This can most
	 * easily be accomplished by reading back the register that we
	 * just wrote (thanks to Chris Torek for this solution).
	 */
	if (CPU_ISSUN4M) {
		volatile u_int16_t discard;
		discard = bus_space_read_2(lesc->sc_bustag, lesc->sc_reg,
					   LEREG1_RDP);
	}
#endif
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

void
lesetutp(sc)
	struct lance_softc *sc;
{
	struct lsi64854_softc *dma = ((struct le_softc *)sc)->sc_dma;
	u_int32_t csr;

	csr = L64854_GCSR(dma);
	csr |= E_TP_AUI;
	L64854_SCSR(dma, csr);
	delay(20000);	/* must not touch le for 20ms */
}

void
lesetaui(sc)
	struct lance_softc *sc;
{
	struct lsi64854_softc *dma = ((struct le_softc *)sc)->sc_dma;
	u_int32_t csr;

	csr = L64854_GCSR(dma);
	csr &= ~E_TP_AUI;
	L64854_SCSR(dma, csr);
	delay(20000);	/* must not touch le for 20ms */
}

int
lemediachange(sc)
	struct lance_softc *sc;
{
	struct ifmedia *ifm = &sc->sc_media;

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return (EINVAL);

	/*
	 * Switch to the selected media.  If autoselect is
	 * set, we don't really have to do anything.  We'll
	 * switch to the other media when we detect loss of
	 * carrier.
	 */
	switch (IFM_SUBTYPE(ifm->ifm_media)) {
	case IFM_10_T:
		lesetutp(sc);
		break;

	case IFM_10_5:
		lesetaui(sc);
		break;

	case IFM_AUTO:
		break;

	default:
		return (EINVAL);
	}

	return (0);
}

void
lemediastatus(sc, ifmr)
	struct lance_softc *sc;
	struct ifmediareq *ifmr;
{
	struct lsi64854_softc *dma = ((struct le_softc *)sc)->sc_dma;

	/*
	 * Notify the world which media we're currently using.
	 */
	if (L64854_GCSR(dma) & E_TP_AUI)
		ifmr->ifm_active = IFM_ETHER|IFM_10_T;
	else
		ifmr->ifm_active = IFM_ETHER|IFM_10_5;
}

hide void
lehwreset(sc)
	struct lance_softc *sc;
{
	struct le_softc *lesc = (struct le_softc *)sc;
	struct lsi64854_softc *dma = lesc->sc_dma;
	u_int32_t csr;
	u_int aui_bit;

	/*
	 * Reset DMA channel.
	 */
	csr = L64854_GCSR(dma);
	aui_bit = csr & E_TP_AUI;
	DMA_RESET(dma);

	/* Write bits 24-31 of Lance address */
	bus_space_write_4(dma->sc_bustag, dma->sc_regs, L64854_REG_ENBAR,
			  lesc->sc_laddr & 0xff000000);

	DMA_ENINTR(dma);

	/*
	 * Disable E-cache invalidates on chip writes.
	 * Retain previous cable selection bit.
	 */
	csr = L64854_GCSR(dma);
	csr |= (E_DSBL_WR_INVAL | aui_bit);
	L64854_SCSR(dma, csr);
	delay(20000);	/* must not touch le for 20ms */
}

hide void
lehwinit(sc)
	struct lance_softc *sc;
{

	/*
	 * Make sure we're using the currently-enabled media type.
	 * XXX Actually, this is probably unnecessary, now.
	 */
	switch (IFM_SUBTYPE(sc->sc_media.ifm_cur->ifm_media)) {
	case IFM_10_T:
		lesetutp(sc);
		break;

	case IFM_10_5:
		lesetaui(sc);
		break;
	}
}

hide void
lenocarrier(sc)
	struct lance_softc *sc;
{
	struct le_softc *lesc = (struct le_softc *)sc;

	/*
	 * Check if the user has requested a certain cable type, and
	 * if so, honor that request.
	 */
	printf("%s: lost carrier on ", sc->sc_dev.dv_xname);

	if (L64854_GCSR(lesc->sc_dma) & E_TP_AUI) {
		printf("UTP port");
		switch (IFM_SUBTYPE(sc->sc_media.ifm_media)) {
		case IFM_10_5:
		case IFM_AUTO:
			printf(", switching to AUI port");
			lesetaui(sc);
		}
	} else {
		printf("AUI port");
		switch (IFM_SUBTYPE(sc->sc_media.ifm_media)) {
		case IFM_10_T:
		case IFM_AUTO:
			printf(", switching to UTP port");
			lesetutp(sc);
		}
	}
	printf("\n");
}

int
lematch_ledma(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(cf->cf_driver->cd_name, sa->sa_name) == 0);
}


void
leattach_ledma(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sbus_attach_args *sa = aux;
	struct le_softc *lesc = (struct le_softc *)self;
	struct lsi64854_softc *lsi = (struct lsi64854_softc *)parent;
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	bus_dma_tag_t dmatag = sa->sa_dmatag;
	bus_dma_segment_t seg;
	int rseg, error;
	/* XXX the following declarations should be elsewhere */
	extern void myetheraddr __P((u_char *));

	lesc->sc_bustag = sa->sa_bustag;

	/* Establish link to `ledma' device */
	lesc->sc_dma = lsi;
	lesc->sc_dma->sc_client = lesc;

	/* Map device registers */
	if (bus_space_map2(sa->sa_bustag,
			   sa->sa_slot,
			   sa->sa_offset,
			   sa->sa_size,
			   BUS_SPACE_MAP_LINEAR,
			   0, &lesc->sc_reg) != 0) {
		printf("%s @ ledma: cannot map registers\n", self->dv_xname);
		return;
	}

	/* Allocate buffer memory */
	sc->sc_memsize = MEMSIZE;

	/* Get a DMA handle */
	if ((error = bus_dmamap_create(dmatag, MEMSIZE, 1, MEMSIZE,
					LEDMA_BOUNDARY, BUS_DMA_NOWAIT,
					&lesc->sc_dmamap)) != 0) {
		printf("%s: DMA map create error %d\n", self->dv_xname, error);
		return;
	}

	/* Allocate DMA buffer */
	if ((error = bus_dmamem_alloc(dmatag, MEMSIZE, 0, LEDMA_BOUNDARY,
				 &seg, 1, &rseg, BUS_DMA_NOWAIT)) != 0) {
		printf("%s @ ledma: DMA buffer alloc error %d\n",
			self->dv_xname, error);
		return;
	}

	/* Load DMA buffer */
	if ((error = bus_dmamap_load_raw(dmatag, lesc->sc_dmamap,
				&seg, rseg, MEMSIZE, BUS_DMA_NOWAIT)) != 0) {
		printf("%s: DMA buffer map load error %d\n",
			self->dv_xname, error);
		bus_dmamem_free(dmatag, &seg, rseg);
		return;
	}

	/* Map DMA buffer into kernel space */
	if ((error = bus_dmamem_map(dmatag, &seg, rseg, MEMSIZE,
			       (caddr_t *)&sc->sc_mem,
			       BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s @ ledma: DMA buffer map error %d\n",
			self->dv_xname, error);
		bus_dmamap_unload(dmatag, lesc->sc_dmamap);
		bus_dmamem_free(dmatag, &seg, rseg);
		return;
	}

	lesc->sc_laddr = lesc->sc_dmamap->dm_segs[0].ds_addr;
	sc->sc_addr = lesc->sc_laddr & 0xffffff;
	sc->sc_conf3 = LE_C3_BSWP | LE_C3_ACON | LE_C3_BCON;


	/* Assume SBus is grandparent */
	lesc->sc_sd.sd_reset = (void *)lance_reset;
	sbus_establish(&lesc->sc_sd, parent);

	sc->sc_mediachange = lemediachange;
	sc->sc_mediastatus = lemediastatus;
	sc->sc_supmedia = lemedia;
	sc->sc_nsupmedia = NLEMEDIA;
	sc->sc_defaultmedia = IFM_ETHER|IFM_AUTO;

	myetheraddr(sc->sc_enaddr);

	sc->sc_copytodesc = lance_copytobuf_contig;
	sc->sc_copyfromdesc = lance_copyfrombuf_contig;
	sc->sc_copytobuf = lance_copytobuf_contig;
	sc->sc_copyfrombuf = lance_copyfrombuf_contig;
	sc->sc_zerobuf = lance_zerobuf_contig;

	sc->sc_rdcsr = lerdcsr;
	sc->sc_wrcsr = lewrcsr;
	sc->sc_hwinit = lehwinit;
	sc->sc_nocarrier = lenocarrier;
	sc->sc_hwreset = lehwreset;

	/* Establish interrupt handler */
	if (sa->sa_nintr != 0)
		(void)bus_intr_establish(sa->sa_bustag, sa->sa_pri, IPL_NET, 0,
					 am7990_intr, sc);

	am7990_config(&lesc->sc_am7990);

	/* now initialize DMA */
	lehwreset(sc);
}
