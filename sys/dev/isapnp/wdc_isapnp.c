/*	$NetBSD: wdc_isapnp.c,v 1.1 1998/01/23 20:40:59 mycroft Exp $	*/

/*
 * Copyright (c) 1997 Charles M. Hannum.  All rights reserved.
 *
 * DMA and multi-sector PIO handling are derived from code contributed by
 * Onno van der Linden.
 *
 * ISA attachment created by Christopher G. Demetriou.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>

#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>

struct wdc_isapnp_softc {
	struct	wdc_softc sc_wdcdev;
	struct	wdc_attachment_data sc_ad;
	void	*sc_ih;
	int	sc_drq;
};

#ifdef __BROKEN_INDIRECT_CONFIG
int	wdc_isapnp_probe 	__P((struct device *, void *, void *));
#else
int	wdc_isapnp_probe 	__P((struct device *, struct cfdata *, void *));
#endif
void	wdc_isapnp_attach 	__P((struct device *, struct device *, void *));

struct cfattach wdc_isapnp_ca = {
	sizeof(struct wdc_isapnp_softc), wdc_isapnp_probe, wdc_isapnp_attach
};

#ifdef notyet
static void	wdc_isapnp_dma_setup __P((void *));
static void	wdc_isapnp_dma_start __P((void *, void *, size_t, int));
static void	wdc_isapnp_dma_finish __P((void *));
#endif

int
wdc_isapnp_probe(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct isapnp_attach_args *ipa = aux;

	if (strcmp(ipa->ipa_devcompat, "PNP0600"))
		return (0);

	return (1);
}

void
wdc_isapnp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wdc_isapnp_softc *sc = (void *)self;
	struct isapnp_attach_args *ipa = aux;

	printf("\n");

	if (ipa->ipa_nio != 2 ||
	    ipa->ipa_nmem != 0 ||
	    ipa->ipa_nmem32 != 0 ||
	    ipa->ipa_nirq != 1 ||
	    ipa->ipa_ndrq > 1) {
		printf("%s: unexpected configuration\n",
		    sc->sc_wdcdev.sc_dev.dv_xname);
		return;
	}

	if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
		printf("%s: couldn't map registers\n",
		    sc->sc_wdcdev.sc_dev.dv_xname);
		return;
	}

	printf("%s: %s %s\n", sc->sc_wdcdev.sc_dev.dv_xname, ipa->ipa_devident,
	    ipa->ipa_devclass);

	sc->sc_ad.iot = ipa->ipa_iot;
	sc->sc_ad.ioh = ipa->ipa_io[0].h;
	sc->sc_ad.auxiot = ipa->ipa_iot;
	sc->sc_ad.auxioh = ipa->ipa_io[1].h;

	sc->sc_ih = isa_intr_establish(ipa->ipa_ic, ipa->ipa_irq[0].num,
	    IST_EDGE, IPL_BIO, wdcintr, sc);

#ifdef notyet
	if (ipa->ipa_ndrq > 0) {
		sc->sc_drq = ipa->ipa_drq[0].num;

		sc->sc_ad.cap |= WDC_CAPABILITY_DMA;
		sc->sc_ad.dma_setup = &wdc_isapnp_dma_setup;
		sc->sc_ad.dma_start = &wdc_isapnp_dma_start;
		sc->sc_ad.dma_finish = &wdc_isapnp_dma_finish;
	}
#endif

	wdcattach(&sc->sc_wdcdev, &sc->sc_ad);
}

#ifdef notyet
static void
wdc_isapnp_dma_setup(scv)
	void *scv;
{
	struct wdc_isapnp_softc *sc = scv;

	if (isa_dmamap_create(sc->sc_wdcdev.sc_dev.dv_parent, sc->sc_drq,
	    MAXPHYS, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW)) {
		printf("%s: can't create map for drq %d\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, sc->sc_drq);
		sc->sc_ad.cap &= ~WDC_CAPABILITY_DMA;
	}
}

static void
wdc_isapnp_dma_start(scv, buf, size, read)
	void *scv, *buf;
	size_t size;
	int read;
{
	struct wdc_isapnp_softc *sc = scv;

	isa_dmastart(sc->sc_wdcdev.sc_dev.dv_parent, sc->sc_drq, buf,
	    size, NULL, read ? DMAMODE_READ : DMAMODE_WRITE,
	    BUS_DMA_NOWAIT);
}

static void
wdc_isapnp_dma_finish(scv)
	void *scv;
{
	struct wdc_isapnp_softc *sc = scv;

	isa_dmadone(sc->sc_wdcdev.sc_dev.dv_parent, sc->sc_drq);
}
#endif
