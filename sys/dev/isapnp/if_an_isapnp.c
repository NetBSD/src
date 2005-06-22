/*	$NetBSD: if_an_isapnp.c,v 1.10 2005/06/22 06:16:02 dyoung Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * ISAPnP bus front-end for the Aironet ISA4500/ISA4800 Wireless LAN Adapter.
 * Unlike WaveLAN, this adapter is attached as an ISA device using an address
 * decoder hack.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_an_isapnp.c,v 1.10 2005/06/22 06:16:02 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/callout.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net80211/ieee80211_netbsd.h>
#include <net80211/ieee80211_var.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/anreg.h>
#include <dev/ic/anvar.h>

#include <dev/isa/isavar.h>

#include <dev/isapnp/isapnpreg.h>
#include <dev/isapnp/isapnpvar.h>
#include <dev/isapnp/isapnpdevs.h>

int	an_isapnp_match(struct device *, struct cfdata *, void *);
void	an_isapnp_attach(struct device *, struct device *, void *);

struct an_isapnp_softc {
	struct an_softc sc_an;			/* real "an" softc */

	/* ISA-specific goo. */
	void	*sc_ih;				/* interrupt cookie */
};

CFATTACH_DECL(an_isapnp, sizeof(struct an_isapnp_softc),
    an_isapnp_match, an_isapnp_attach, NULL, NULL);

int
an_isapnp_match(struct device *parent, struct cfdata *match, void *aux)
{
	int pri, variant;

	pri = isapnp_devmatch(aux, &isapnp_an_devinfo, &variant);
	if (pri && variant > 0)
		pri = 0;
	return (pri);
}

void
an_isapnp_attach(struct device *parent, struct device *self, void *aux)
{
	struct an_isapnp_softc *isc = (void *) self;
	struct an_softc *sc = &isc->sc_an;
	struct isapnp_attach_args *ipa = aux;

	printf("\n");

	if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
		printf("%s: can't configure isapnp resources\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_iot = ipa->ipa_iot;
	sc->sc_ioh = ipa->ipa_io[0].h;

	printf("%s: %s %s\n", sc->sc_dev.dv_xname, ipa->ipa_devident,
	    ipa->ipa_devclass);

	/* This interface is always enabled. */
	sc->sc_enabled = 1;

	/* Establish the interrupt handler. */
	isc->sc_ih = isa_intr_establish(ipa->ipa_ic, ipa->ipa_irq[0].num,
	    ipa->ipa_irq[0].type, IPL_NET, an_intr, sc);
	if (isc->sc_ih == NULL)
		printf("%s: couldn't establish interrupt handler\n",
		    sc->sc_dev.dv_xname);

	if (an_attach(sc) != 0) {
		printf("%s: failed to attach controller\n",
		    sc->sc_dev.dv_xname);
		isa_intr_disestablish(ipa->ipa_ic, isc->sc_ih);
		isc->sc_ih = NULL;
	}
}
