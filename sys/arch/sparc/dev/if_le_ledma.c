/*	$NetBSD: if_le_ledma.c,v 1.2 1998/07/29 16:10:57 pk Exp $	*/

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

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <dev/sbus/sbusvar.h>
#include <sparc/dev/dmareg.h>
#include <sparc/dev/dmavar.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

/*
 * LANCE registers.
 */
struct lereg1 {
	volatile u_int16_t	ler1_rdp;	/* data port */
	volatile u_int16_t	ler1_rap;	/* register select port */
};

struct	le_softc {
	struct	am7990_softc sc_am7990;	/* glue to MI code */
	struct	sbusdev sc_sd;		/* sbus device */
	bus_space_tag_t	sc_bustag;
	bus_dma_tag_t	sc_dmatag;
	struct	lereg1 *sc_r1;		/* LANCE registers */
	struct	dma_softc *sc_dma;	/* pointer to my dma */
	u_long	sc_laddr;		/* LANCE DMA address */
};

#define MEMSIZE 0x4000		/* LANCE memory size */


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
	struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;

	ler1->ler1_rap = port;
	ler1->ler1_rdp = val;

#if defined(SUN4M)
	/*
	 * We need to flush the Sbus->Mbus write buffers. This can most
	 * easily be accomplished by reading back the register that we
	 * just wrote (thanks to Chris Torek for this solution).
	 */
	if (CPU_ISSUN4M) {
		volatile u_int16_t discard;
		discard = ler1->ler1_rdp;
	}
#endif
}

static u_int16_t
lerdcsr(sc, port)
	struct lance_softc *sc;
	u_int16_t port;
{
	struct lereg1 *ler1 = ((struct le_softc *)sc)->sc_r1;
	u_int16_t val;

	ler1->ler1_rap = port;
	val = ler1->ler1_rdp;
	return (val);
}

void
lesetutp(sc)
	struct lance_softc *sc;
{
	struct le_softc *lesc = (struct le_softc *)sc;

	lesc->sc_dma->sc_regs->csr |= DE_AUI_TP;
	delay(20000);	/* must not touch le for 20ms */
}

void
lesetaui(sc)
	struct lance_softc *sc;
{
	struct le_softc *lesc = (struct le_softc *)sc;

	lesc->sc_dma->sc_regs->csr &= ~DE_AUI_TP;
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
	struct le_softc *lesc = (struct le_softc *)sc;

	/*
	 * Notify the world which media we're currently using.
	 */
	if (lesc->sc_dma->sc_regs->csr & DE_AUI_TP)
		ifmr->ifm_active = IFM_ETHER|IFM_10_T;
	else
		ifmr->ifm_active = IFM_ETHER|IFM_10_5;
}

hide void
lehwreset(sc)
	struct lance_softc *sc;
{
	struct le_softc *lesc = (struct le_softc *)sc;
	struct dma_softc *dma = lesc->sc_dma;
	u_long aui_bit;

	/*
	 * Reset DMA channel.
	 */
	aui_bit = dma->sc_regs->csr & DE_AUI_TP;
	DMA_RESET(dma);
	dma->sc_regs->en_bar = lesc->sc_laddr & 0xff000000;
	DMA_ENINTR(dma);
#define D_DSBL_WRINVAL D_DSBL_SCSI_DRN	/* XXX: fix dmareg.h */
	/*
	 * Disable E-cache invalidates on chip writes.
	 * Retain previous cable selection bit.
	 */
	dma->sc_regs->csr |= (D_DSBL_WRINVAL | aui_bit);
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


#define SAME_LANCE(bp, sa) \
	((bp->val[0] == sa->sa_slot && bp->val[1] == sa->sa_offset) || \
	 (bp->val[0] == -1 && bp->val[1] == sc->sc_dev.dv_unit))

void
leattach_ledma(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sbus_attach_args *sa = aux;
	struct le_softc *lesc = (struct le_softc *)self;
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	bus_space_handle_t bh;
	bus_dma_segment_t seg;
	int rseg, error;
	/* XXX the following declarations should be elsewhere */
	extern void myetheraddr __P((u_char *));

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
	lesc->sc_sd.sd_reset = (void *)lance_reset;
	sbus_establish(&lesc->sc_sd, parent);

	if (sa->sa_bp != NULL && strcmp(sa->sa_bp->name, le_cd.cd_name) == 0 &&
	    SAME_LANCE(sa->sa_bp, sa))
		sa->sa_bp->dev = &sc->sc_dev;

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

	am7990_config(&lesc->sc_am7990);

	(void)bus_intr_establish(lesc->sc_bustag, sa->sa_pri, 0,
				 am7990_intr, sc);

	/* now initialize DMA */
	lehwreset(sc);
}
