/*	$NetBSD: if_le.c,v 1.53 1998/03/21 20:14:13 pk Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*-
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	This product includes software developed by Aaron Brown and
 *	Harvard University.
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if_le.c	8.2 (Berkeley) 11/16/93
 */

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

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <sparc/dev/sbusvar.h>
#include <sparc/dev/dmareg.h>
#include <sparc/dev/dmavar.h>
#include <sparc/dev/lebuffervar.h>

#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#include <sparc/dev/if_lereg.h>
#include <sparc/dev/if_levar.h>

int	lematch_byname __P((struct device *, struct cfdata *, void *));
int	lematch_obio __P((struct device *, struct cfdata *, void *));
void	leattach_sbus __P((struct device *, struct device *, void *));
void	leattach_ledma __P((struct device *, struct device *, void *));
void	leattach_lebuffer __P((struct device *, struct device *, void *));
void	leattach_obio __P((struct device *, struct device *, void *));

void	leattach __P((struct le_softc *, int));

#if 0
#if defined(SUN4M)	/* XXX */
int	myleintr __P((void *));
int	ledmaintr __P((struct dma_softc *));

int
myleintr(arg)
	void	*arg;
{
	register struct le_softc *lesc = arg;
static int dodrain=0;

	if (lesc->sc_dma->sc_regs->csr & D_ERR_PEND) {
		dodrain = 1;
		return ledmaintr(lesc->sc_dma);
	}

	if (dodrain) {	/* XXX - is this necessary with D_DSBL_WRINVAL on? */
#define E_DRAIN 0x400 /* XXX: fix dmareg.h */
		int i = 10;
		while (i-- > 0 && (lesc->sc_dma->sc_regs->csr & D_DRAINING))
			delay(1);
	}

	return (am7990_intr(arg));
}
#endif
#endif

#if defined(SUN4M)
/*
 * Media types supported by the Sun4m.
 */
int lemediasun4m[] = {
	IFM_ETHER|IFM_10_T,
	IFM_ETHER|IFM_10_5,
	IFM_ETHER|IFM_AUTO,
};
#define NLEMEDIASUN4M	(sizeof(lemediasun4m) / sizeof(lemediasun4m[0]))

void	lesetutp __P((struct am7990_softc *));
void	lesetaui __P((struct am7990_softc *));

int	lemediachange __P((struct am7990_softc *));
void	lemediastatus __P((struct am7990_softc *, struct ifmediareq *));
#endif /* SUN4M */

/* Four attachments for this device */
struct cfattach le_sbus_ca = {
	sizeof(struct le_softc), lematch_byname, leattach_sbus
};

struct cfattach le_ledma_ca = {
	sizeof(struct le_softc), lematch_byname, leattach_ledma
};

struct cfattach le_lebuffer_ca = {
	sizeof(struct le_softc), lematch_byname, leattach_lebuffer
};

struct cfattach le_obio_ca = {
	sizeof(struct le_softc), lematch_obio, leattach_obio
};

extern struct cfdriver le_cd;

hide void lewrcsr __P((struct am7990_softc *, u_int16_t, u_int16_t));
hide u_int16_t lerdcsr __P((struct am7990_softc *, u_int16_t));
hide void lehwreset __P((struct am7990_softc *));
hide void lehwinit __P((struct am7990_softc *));
hide void lenocarrier __P((struct am7990_softc *));

hide void
lewrcsr(sc, port, val)
	struct am7990_softc *sc;
	u_int16_t port, val;
{
	register struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;
#if defined(SUN4M)
	volatile u_int16_t discard;
#endif

	ler1->ler1_rap = port;
	ler1->ler1_rdp = val;
#if defined(SUN4M)
	/* 
	 * We need to flush the Sbus->Mbus write buffers. This can most
	 * easily be accomplished by reading back the register that we
	 * just wrote (thanks to Chris Torek for this solution).
	 */	   
	if (CPU_ISSUN4M)
		discard = ler1->ler1_rdp;
#endif
}

hide u_int16_t
lerdcsr(sc, port)
	struct am7990_softc *sc;
	u_int16_t port;
{
	register struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;
	u_int16_t val;

	ler1->ler1_rap = port;
	val = ler1->ler1_rdp;
	return (val);
}

#if defined(SUN4M)
void
lesetutp(sc)
	struct am7990_softc *sc;
{
	struct le_softc *lesc = (struct le_softc *)sc;

	lesc->sc_dma->sc_regs->csr |= DE_AUI_TP;
	delay(20000);	/* must not touch le for 20ms */
}

void
lesetaui(sc)
	struct am7990_softc *sc;
{
	struct le_softc *lesc = (struct le_softc *)sc;

	lesc->sc_dma->sc_regs->csr &= ~DE_AUI_TP;
	delay(20000);	/* must not touch le for 20ms */
}

int
lemediachange(sc)
	struct am7990_softc *sc;
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
	struct am7990_softc *sc;
	struct ifmediareq *ifmr;
{
	struct le_softc *lesc = (struct le_softc *)sc;

	if (lesc->sc_dma == NULL)
		return;

	/*
	 * Notify the world which media we're currently using.
	 */
	if (lesc->sc_dma->sc_regs->csr & DE_AUI_TP)
		ifmr->ifm_active = IFM_ETHER|IFM_10_T;
	else
		ifmr->ifm_active = IFM_ETHER|IFM_10_5;
}
#endif /* SUN4M */

hide void
lehwreset(sc)
	struct am7990_softc *sc;
{
#if defined(SUN4M) 
	struct le_softc *lesc = (struct le_softc *)sc;

	/*
	 * Reset DMA channel.
	 */
	if (CPU_ISSUN4M && lesc->sc_dma) {
		DMA_RESET(lesc->sc_dma);
		lesc->sc_dma->sc_regs->en_bar = lesc->sc_laddr & 0xff000000;
		DMA_ENINTR(lesc->sc_dma);
#define D_DSBL_WRINVAL D_DSBL_SCSI_DRN	/* XXX: fix dmareg.h */
		/* Disable E-cache invalidates on chip writes */
		lesc->sc_dma->sc_regs->csr |= D_DSBL_WRINVAL;
	}
#endif
}

hide void
lehwinit(sc)
	struct am7990_softc *sc;
{
#if defined(SUN4M) 
	struct le_softc *lesc = (struct le_softc *)sc;

	/*
	 * Make sure we're using the currently-enabled media type.
	 * XXX Actually, this is probably unnecessary, now.
	 */
	if (CPU_ISSUN4M && lesc->sc_dma) {
		switch (IFM_SUBTYPE(sc->sc_media.ifm_cur->ifm_media)) {
		case IFM_10_T:
			lesetutp(sc);
			break;

		case IFM_10_5:
			lesetaui(sc);
			break;
		}
	}
#endif
}

hide void
lenocarrier(sc)
	struct am7990_softc *sc;
{
#if defined(SUN4M)
	struct le_softc *lesc = (struct le_softc *)sc;

	if (CPU_ISSUN4M && lesc->sc_dma) {
		/* 
		 * Check if the user has requested a certain cable type, and
		 * if so, honor that request.
		 */
		printf("%s: lost carrier on ", sc->sc_dev.dv_xname);
		if (lesc->sc_dma->sc_regs->csr & DE_AUI_TP) {
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
	} else
#endif
		printf("%s: lost carrier\n", sc->sc_dev.dv_xname);
}

int
lematch_byname(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(cf->cf_driver->cd_name, sa->sa_name) == 0);
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
	return (obio_bus_probe(oba->oba_bustag, oba->oba_paddr,
			       0, 2, NULL, NULL));
}


#define SAME_LANCE(bp, sa) \
	((bp->val[0] == sa->sa_slot && bp->val[1] == sa->sa_offset) || \
	 (bp->val[0] == -1 && bp->val[1] == sc->sc_dev.dv_unit))

void
leattach_sbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sbus_attach_args *sa = aux;
	struct le_softc *lesc = (struct le_softc *)self;
	struct am7990_softc *sc = &lesc->sc_am7990;
	struct sbusdev *sd;
	bus_space_handle_t bh;

	lesc->sc_bustag = sa->sa_bustag;
	lesc->sc_dmatag = sa->sa_dmatag;

	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot,
			 sa->sa_offset,
			 sizeof(struct lereg1),
			 BUS_SPACE_MAP_LINEAR,
			 0, &bh) != 0) {
		printf("%s @ sbus: cannot map registers\n", self->dv_xname);
		return;
	}
	lesc->sc_r1 = (struct lereg1 *)bh;

	/*
	 * Look for an "unallocated" lebuffer and pair it with
	 * this `le' device on the assumption that we're on
	 * a pre-historic ROM that doesn't establish le<=>lebuffer
	 * parent-child relationships.
	 */
	for (sd = ((struct sbus_softc *)parent)->sc_sbdev; sd != NULL;
	     sd = sd->sd_bchain) {

		struct lebuf_softc *lebuf = (struct lebuf_softc *)sd->sd_dev;

		if (strncmp("lebuffer", sd->sd_dev->dv_xname, 8) != 0)
			continue;

		if (lebuf->attached != 0)
			continue;

		sc->sc_mem = lebuf->sc_buffer;
		sc->sc_memsize = lebuf->sc_bufsiz;
		sc->sc_addr = 0; /* Lance view is offset by buffer location */
		lebuf->attached = 1;

		/* That old black magic... */
		sc->sc_conf3 = getpropint(sa->sa_node,
					  "busmaster-regval",
					  LE_C3_BSWP | LE_C3_ACON | LE_C3_BCON);
		break;
	}

	lesc->sc_sd.sd_reset = (void *)am7990_reset;
	sbus_establish(&lesc->sc_sd, &sc->sc_dev);

	if (sa->sa_bp != NULL && strcmp(sa->sa_bp->name, le_cd.cd_name) == 0 &&
	    SAME_LANCE(sa->sa_bp, sa))
		sa->sa_bp->dev = &sc->sc_dev;

	leattach(lesc, sa->sa_pri);
}

void
leattach_ledma(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
#if defined(SUN4M)
	struct sbus_attach_args *sa = aux;
	struct le_softc *lesc = (struct le_softc *)self;
	struct am7990_softc *sc = &lesc->sc_am7990;
	bus_space_handle_t bh;
	bus_dma_segment_t seg;
	int rseg, error;

	/* Establish link to `ledma' device */
	lesc->sc_dma = (struct dma_softc *)parent;
	lesc->sc_dma->sc_le = lesc;

	lesc->sc_bustag = sa->sa_bustag;
	lesc->sc_dmatag = sa->sa_dmatag;

	/* Map device registers */
	if (bus_space_map2(sa->sa_bustag,
			   sa->sa_slot,
			   sa->sa_offset,
			   sizeof(struct lereg1),
			   BUS_SPACE_MAP_LINEAR,
			   0, &bh) != 0) {
		printf("%s @ ledma: cannot map registers\n", self->dv_xname);
		return;
	}
	lesc->sc_r1 = (struct lereg1 *)bh;

	/* Allocate buffer memory */
	sc->sc_memsize = MEMSIZE;
	error = bus_dmamem_alloc(lesc->sc_dmatag, MEMSIZE, NBPG, 0,
				 &seg, 1, &rseg, BUS_DMA_NOWAIT);
	if (error) {
		printf("leattach_ledma: DMA buffer alloc error %d\n", error);
		return;
	}
	error = bus_dmamem_map(lesc->sc_dmatag, &seg, rseg, MEMSIZE,
			       (caddr_t *)&sc->sc_mem,
			       BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error) {
		printf("%s @ ledma: DMA buffer map error %d\n",
			self->dv_xname, error);
		return;
	}

#if defined (SUN4M)
	if ((seg.ds_addr & 0xffffff) >=
	    (seg.ds_addr & 0xffffff) + MEMSIZE)
		panic("leattach_ledma: Lance buffer crosses 16MB boundary");
#endif
	sc->sc_addr = seg.ds_addr & 0xffffff;
	sc->sc_conf3 = LE_C3_BSWP | LE_C3_ACON | LE_C3_BCON;

	lesc->sc_laddr = seg.ds_addr;

	/* Assume SBus is grandparent */
	lesc->sc_sd.sd_reset = (void *)am7990_reset;
	sbus_establish(&lesc->sc_sd, parent);

	if (sa->sa_bp != NULL && strcmp(sa->sa_bp->name, le_cd.cd_name) == 0 &&
	    SAME_LANCE(sa->sa_bp, sa))
		sa->sa_bp->dev = &sc->sc_dev;

	sc->sc_mediachange = lemediachange;
	sc->sc_mediastatus = lemediastatus;
	sc->sc_supmedia = lemediasun4m;
	sc->sc_nsupmedia = NLEMEDIASUN4M;
	sc->sc_defaultmedia = IFM_ETHER|IFM_AUTO;

	leattach(lesc, sa->sa_pri);
#endif
}

void
leattach_lebuffer(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sbus_attach_args *sa = aux;
	struct le_softc *lesc = (struct le_softc *)self;
	struct am7990_softc *sc = &lesc->sc_am7990;
	struct lebuf_softc *lebuf = (struct lebuf_softc *)parent;
	bus_space_handle_t bh;

	lesc->sc_bustag = sa->sa_bustag;
	lesc->sc_dmatag = sa->sa_dmatag;

	if (bus_space_map2(sa->sa_bustag,
			   sa->sa_slot,
			   sa->sa_offset,
			   sizeof(struct lereg1),
			   BUS_SPACE_MAP_LINEAR,
			   0, &bh)) {
		printf("%s @ lebuffer: cannot map registers\n", self->dv_xname);
		return;
	}
	lesc->sc_r1 = (struct lereg1 *)bh;

	sc->sc_mem = lebuf->sc_buffer;
	sc->sc_memsize = lebuf->sc_bufsiz;
	sc->sc_addr = 0; /* Lance view is offset by buffer location */
	lebuf->attached = 1;

	/* That old black magic... */
	sc->sc_conf3 = getpropint(sa->sa_node, "busmaster-regval",
				  LE_C3_BSWP | LE_C3_ACON | LE_C3_BCON);

	/* Assume SBus is grandparent */
	lesc->sc_sd.sd_reset = (void *)am7990_reset;
	sbus_establish(&lesc->sc_sd, parent);

	if (sa->sa_bp != NULL && strcmp(sa->sa_bp->name, le_cd.cd_name) == 0 &&
	    SAME_LANCE(sa->sa_bp, sa))
		sa->sa_bp->dev = &sc->sc_dev;

	leattach(lesc, sa->sa_pri);
}

void
leattach_obio(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;
	struct le_softc *lesc = (struct le_softc *)self;
	struct am7990_softc *sc = &lesc->sc_am7990;
	bus_space_handle_t bh;

	lesc->sc_bustag = oba->oba_bustag;
	lesc->sc_dmatag = oba->oba_dmatag;

	if (obio_bus_map(oba->oba_bustag, oba->oba_paddr,
			 0, sizeof(struct lereg1),
			 0, 0,
			 &bh) != 0) {
		printf("leattach_obio: cannot map registers\n");
		return;
	}
	lesc->sc_r1 = (struct lereg1 *)bh;

	if (oba->oba_bp != NULL &&
	    strcmp(oba->oba_bp->name, le_cd.cd_name) == 0 &&
	    sc->sc_dev.dv_unit == oba->oba_bp->val[1])
		oba->oba_bp->dev = &sc->sc_dev;

	/* Install interrupt */
	leattach(lesc, oba->oba_pri);
}

void
leattach(lesc, pri)
	struct le_softc *lesc;
	int pri;
{
	struct am7990_softc *sc = &lesc->sc_am7990;

	/* XXX the following declarations should be elsewhere */
	extern void myetheraddr __P((u_char *));

	if (sc->sc_mem == 0) {
#if 0
		bus_dma_segment_t seg;
		int rseg, error;

		error = bus_dmamem_alloc(lesc->sc_dmat, MEMSIZE, NBPG, 0,
					 &seg, 1, &rseg, BUS_DMA_NOWAIT);
		if (error) {
			printf("if_le: DMA buffer alloc error %d\n", error);
			return;
		}
		error = bus_dmamem_map(lesc->sc_dmat, &seg, rseg, MEMSIZE,
				       (caddr_t *)&sc->sc_mem,
				       BUS_DMA_NOWAIT|BUS_DMAMEM_NOSYNC);
		if (error) {
			printf("if_le: DMA buffer map error %d\n", error);
			return;
		}

		sc->sc_addr = seg.ds_addr & 0xffffff;

#else
		u_long laddr;
		laddr = (u_long)dvma_malloc(MEMSIZE, &sc->sc_mem, M_NOWAIT);
		sc->sc_addr = laddr & 0xffffff;
#endif/*0*/
#if defined (SUN4M)
		if ((sc->sc_addr & 0xffffff) >=
		    (sc->sc_addr & 0xffffff) + MEMSIZE)
			panic("if_le: Lance buffer crosses 16MB boundary");
#endif
		sc->sc_memsize = MEMSIZE;
		sc->sc_conf3 = LE_C3_BSWP | LE_C3_ACON | LE_C3_BCON;
	}

	myetheraddr(sc->sc_enaddr);

	sc->sc_copytodesc = am7990_copytobuf_contig;
	sc->sc_copyfromdesc = am7990_copyfrombuf_contig;
	sc->sc_copytobuf = am7990_copytobuf_contig;
	sc->sc_copyfrombuf = am7990_copyfrombuf_contig;
	sc->sc_zerobuf = am7990_zerobuf_contig;

	sc->sc_rdcsr = lerdcsr;
	sc->sc_wrcsr = lewrcsr;
	sc->sc_hwinit = lehwinit;
	sc->sc_nocarrier = lenocarrier;
	sc->sc_hwreset = lehwreset;

	am7990_config(sc);

	(void)bus_intr_establish(lesc->sc_bustag, pri, 0, am7990_intr, sc);

	/* now initialize DMA */
	lehwreset(sc);
}
