/*	$NetBSD: wdc_pciide.c,v 1.1 1998/03/04 06:35:12 cgd Exp $	*/

/*
 * Copyright (c) 1998 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/*
 * Machine-independent ATA/ATAPI ('wdc') driver attachment to PCI IDE
 * controller driver.
 *
 * Author: Christopher G. Demetriou, March 2, 1998.
 *
 * XXX Does not yet support DMA.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/ic/wdcvar.h>
#include <dev/pci/pciidevar.h>

#include "locators.h"

struct wdc_pciide_softc {
	struct wdc_softc		sc_wdcdev;
	struct wdc_attachment_data	sc_ad;
};

#ifdef __BROKEN_INDIRECT_CONFIG
int	wdc_pciide_probe __P((struct device *, void *, void *));
#else
int	wdc_pciide_probe __P((struct device *, struct cfdata *, void *));
#endif
void	wdc_pciide_attach __P((struct device *, struct device *, void *));

struct cfattach wdc_pciide_ca = {
	sizeof(struct wdc_pciide_softc), wdc_pciide_probe, wdc_pciide_attach
};

int
#ifdef __BROKEN_INDIRECT_CONFIG
wdc_pciide_probe(parent, matchv, aux)
#else
wdc_pciide_probe(parent, match, aux)
#endif
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *matchv;
#else
	struct cfdata *match;
#endif
	void *aux;
{
#ifdef __BROKEN_INDIRECT_CONFIG
	struct cfdata *match = matchv;
#endif
	struct pciide_attach_args *aa = aux;

	/*
	 * Only 'wdc's attach to 'pciide's, so our going-in postion
	 * is to succeed.
	 */

	/* If the locators don't match, the match doesn't succeed. */
	if (match->cf_loc[PCIIDECF_CHANNEL] != aa->channel &&
	    match->cf_loc[PCIIDECF_CHANNEL] != PCIIDECF_CHANNEL_DEFAULT) {
		return (0);
	}

	return (1);
}

void
wdc_pciide_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wdc_pciide_softc *sc = (void *)self;
	struct pciide_attach_args *aa = aux;

	printf("\n");

	/* Set up data for wdc attachment. */
	sc->sc_ad.iot = aa->cmd_iot;
	sc->sc_ad.ioh = aa->cmd_ioh;
	sc->sc_ad.auxiot = aa->ctl_iot;
	sc->sc_ad.auxioh = aa->ctl_ioh;

	/* Plug in the interrupt handler. */
	*(aa->ihandp) = wdcintr;
	*(aa->ihandargp) = sc;

	/* XXX Does not yet support DMA. */

	wdcattach(&sc->sc_wdcdev, &sc->sc_ad);
}
