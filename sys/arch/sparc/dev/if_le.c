/*	$NetBSD: if_le.c,v 1.44 1997/03/15 18:10:38 is Exp $	*/

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

int	lematch __P((struct device *, struct cfdata *, void *));
void	leattach __P((struct device *, struct device *, void *));

#if defined(SUN4M)	/* XXX */
int	myleintr __P((void *));
int	ledmaintr __P((struct dma_softc *));

int
myleintr(arg)
	void	*arg;
{
	register struct le_softc *lesc = arg;

	if (lesc->sc_dma->sc_regs->csr & D_ERR_PEND)
		return ledmaintr(lesc->sc_dma);

	return (am7990_intr(arg));
}
#endif

struct cfattach le_ca = {
	sizeof(struct le_softc), lematch, leattach
};

hide void lewrcsr __P((struct am7990_softc *, u_int16_t, u_int16_t));
hide u_int16_t lerdcsr __P((struct am7990_softc *, u_int16_t));
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

hide void
lehwinit(sc)
	struct am7990_softc *sc;
{
#if defined(SUN4M) 
	struct le_softc *lesc = (struct le_softc *)sc;

	if (CPU_ISSUN4M && lesc->sc_dma) {
		struct ifnet *ifp = &sc->sc_ethercom.ec_if;

		if (ifp->if_flags & IFF_LINK0)
			lesc->sc_dma->sc_regs->csr |= DE_AUI_TP;
		else if (ifp->if_flags & IFF_LINK1)
			lesc->sc_dma->sc_regs->csr &= ~DE_AUI_TP;

		delay(20000);	/* must not touch le for 20ms */
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
		struct ifnet *ifp = &sc->sc_ethercom.ec_if;

		/* 
		 * Check if the user has requested a certain cable type, and
		 * if so, honor that request.
		 */
		if (ifp->if_flags & IFF_LINK0)
			printf("%s: lost carrier on UTP port\n",
			       sc->sc_dev.dv_xname);
		else if (ifp->if_flags & IFF_LINK1)
			printf("%s: lost carrier on AUI port\n",
			       sc->sc_dev.dv_xname);
		else {
			/*
			 * Switch cable type and inform the user that
			 * we have done so
			 */
			if (lesc->sc_dma->sc_regs->csr & DE_AUI_TP) {
				printf("%s: no carrier on UTP port, "
				       "switching to AUI port\n",
				       sc->sc_dev.dv_xname);
				lesc->sc_dma->sc_regs->csr &= ~DE_AUI_TP;
			} else {
				printf("%s: no carrier on AUI port, "
				       "switching to UTP port\n",
				       sc->sc_dev.dv_xname);
				lesc->sc_dma->sc_regs->csr |= DE_AUI_TP;
			}
			delay(20000); /* make cable selection stick */
		}
	} else
#endif
		printf("%s: lost carrier\n",sc->sc_dev.dv_xname);
}

int
lematch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;
	register struct romaux *ra = &ca->ca_ra;

	if (strcmp(cf->cf_driver->cd_name, ra->ra_name))
		return (0);
	if (ca->ca_bustype == BUS_SBUS)
		return (1);

	return (probeget(ra->ra_vaddr, 2) != -1);
}

void
leattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct le_softc *lesc = (struct le_softc *)self;
	struct am7990_softc *sc = &lesc->sc_am7990;
	struct confargs *ca = aux;
	int pri;
	struct bootpath *bp;
#if defined(SUN4C) || defined(SUN4M)
	int sbuschild = strncmp(parent->dv_xname, "sbus", 4) == 0;
	int lebufchild = strncmp(parent->dv_xname, "lebuffer", 8) == 0;
	int dmachild = strncmp(parent->dv_xname, "ledma", 5) == 0;
	struct lebuf_softc *lebuf;
#endif

	/* XXX the following declarations should be elsewhere */
	extern void myetheraddr __P((u_char *));

	if (ca->ca_ra.ra_nintr != 1) {
		printf(": expected 1 interrupt, got %d\n", ca->ca_ra.ra_nintr);
		return;
	}
	pri = ca->ca_ra.ra_intr[0].int_pri;
	printf(" pri %d", pri);

	lesc->sc_r1 = (struct lereg1 *)mapiodev(ca->ca_ra.ra_reg, 0,
					      sizeof(struct lereg1),
					      ca->ca_bustype);
#if defined(SUN4C) || defined(SUN4M)
	lebuf = NULL;
	if (lebufchild) {
		lebuf = (struct lebuf_softc *)parent;
	} else if (sbuschild) {
		struct sbus_softc *sbus = (struct sbus_softc *)parent;
		struct sbusdev *sd;

		/*
		 * Find last "unallocated" lebuffer and pair it with
		 * this `le' device on the assumption that we're on
		 * a pre-historic ROM that doesn't establish le<=>lebuffer
		 * parent-child relationships.
		 */
		for (sd = sbus->sc_sbdev; sd != NULL; sd = sd->sd_bchain) {
			if (strncmp("lebuffer", sd->sd_dev->dv_xname, 8) != 0)
				continue;
			if (((struct lebuf_softc *)sd->sd_dev)->attached == 0) {
				lebuf = (struct lebuf_softc *)sd->sd_dev;
				break;
			}
		}
	}
	if (lebuf != NULL) {
		sc->sc_mem = lebuf->sc_buffer;
		sc->sc_memsize = lebuf->sc_bufsiz;
		sc->sc_addr = 0; /* Lance view is offset by buffer location */
		lebuf->attached = 1;

		/* That old black magic... */
		sc->sc_conf3 = getpropint(ca->ca_ra.ra_node,
			 	"busmaster-regval",
				LE_C3_BSWP | LE_C3_ACON | LE_C3_BCON);
	} else
#endif
	{
		u_long laddr;
		laddr = (u_long)dvma_malloc(MEMSIZE, &sc->sc_mem, M_NOWAIT);
#if defined (SUN4M)
		if ((laddr & 0xffffff) >= (laddr & 0xffffff) + MEMSIZE)
			panic("if_le: Lance buffer crosses 16MB boundary");
#endif
		sc->sc_addr = laddr & 0xffffff;
		sc->sc_memsize = MEMSIZE;
		sc->sc_conf3 = LE_C3_BSWP | LE_C3_ACON | LE_C3_BCON;
#if defined(SUN4C) || defined(SUN4M)
		if (dmachild) {
			lesc->sc_dma = (struct dma_softc *)parent;
			lesc->sc_dma->sc_le = lesc;
			lesc->sc_dma->sc_regs->en_bar = laddr & 0xff000000;
		}
#endif
	}

	bp = ca->ca_ra.ra_bp;
	switch (ca->ca_bustype) {
#if defined(SUN4C) || defined(SUN4M)
#define SAME_LANCE(bp, ca) \
	((bp->val[0] == ca->ca_slot && bp->val[1] == ca->ca_offset) || \
	 (bp->val[0] == -1 && bp->val[1] == sc->sc_dev.dv_unit))

	case BUS_SBUS:
		lesc->sc_sd.sd_reset = (void *)am7990_reset;
		if (sbuschild) {
			sbus_establish(&lesc->sc_sd, &sc->sc_dev);
		} else {
			/* Assume SBus is grandparent */
			sbus_establish(&lesc->sc_sd, parent);
		}

		if (bp != NULL && strcmp(bp->name, le_cd.cd_name) == 0 &&
		    SAME_LANCE(bp, ca))
			bp->dev = &sc->sc_dev;
		break;
#endif /* SUN4C || SUN4M */

	default:
		if (bp != NULL && strcmp(bp->name, le_cd.cd_name) == 0 &&
		    sc->sc_dev.dv_unit == bp->val[1])
			bp->dev = &sc->sc_dev;
		break;
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

	am7990_config(sc);

	lesc->sc_ih.ih_fun = am7990_intr;
#if defined(SUN4M) /*XXX*/
	if (CPU_ISSUN4M && lesc->sc_dma)
		lesc->sc_ih.ih_fun = myleintr;
#endif
	lesc->sc_ih.ih_arg = sc;
	intr_establish(pri, &lesc->sc_ih);

	/* now initialize DMA */
	if (lesc->sc_dma) {
		DMA_ENINTR(lesc->sc_dma);
	}
}
