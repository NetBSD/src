/* $NetBSD: if_an_pcmcia.c,v 1.24 2004/08/10 19:12:25 mycroft Exp $ */

/*-
 * Copyright (c) 2000, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Atsushi Onoe
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_an_pcmcia.c,v 1.24 2004/08/10 19:12:25 mycroft Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_compat.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/anreg.h>
#include <dev/ic/anvar.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

static int an_pcmcia_match __P((struct device *, struct cfdata *, void *));
static void an_pcmcia_attach __P((struct device *, struct device *, void *));
static int an_pcmcia_detach __P((struct device *, int));
static int an_pcmcia_enable __P((struct an_softc *));
static void an_pcmcia_disable __P((struct an_softc *));

struct an_pcmcia_softc {
	struct an_softc sc_an;			/* real "an" softc */

	/* PCMCIA-specific goo */
	struct pcmcia_io_handle sc_pcioh;	/* PCMCIA i/o space info */
	int sc_io_window;			/* our i/o window */
	struct pcmcia_function *sc_pf;		/* our PCMCIA function */
	void *sc_ih;				/* interrupt handle */
	void *sc_powerhook;			/* power hook descriptor */
};

CFATTACH_DECL(an_pcmcia, sizeof(struct an_pcmcia_softc),
    an_pcmcia_match, an_pcmcia_attach, an_pcmcia_detach, an_activate);

static const struct pcmcia_product an_pcmcia_products[] = {
	{ PCMCIA_VENDOR_AIRONET,	PCMCIA_PRODUCT_AIRONET_PC4800,
	  PCMCIA_CIS_AIRONET_PC4800 },
	{ PCMCIA_VENDOR_AIRONET,	PCMCIA_PRODUCT_AIRONET_PC4500,
	  PCMCIA_CIS_AIRONET_PC4500 },
	{ PCMCIA_VENDOR_AIRONET,	PCMCIA_PRODUCT_AIRONET_350,
	  PCMCIA_CIS_AIRONET_350 },
};
static const size_t an_pcmcia_nproducts =
    sizeof(an_pcmcia_products) / sizeof(an_pcmcia_products[0]);

static int
an_pcmcia_enable(sc)
	struct an_softc *sc;
{
	struct an_pcmcia_softc *psc = (struct an_pcmcia_softc *)sc;
	struct pcmcia_function *pf = psc->sc_pf;

	/* establish the interrupt. */
	psc->sc_ih = pcmcia_intr_establish(pf, IPL_NET, an_intr, sc);
	if (psc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}

	if (pcmcia_function_enable(pf)) {
		pcmcia_intr_disestablish(pf, psc->sc_ih);
		return (1);
	}
	DELAY(1000);

	return (0);
}

static void
an_pcmcia_disable(sc)
	struct an_softc *sc;
{
	struct an_pcmcia_softc *psc = (struct an_pcmcia_softc *)sc;
	struct pcmcia_function *pf = psc->sc_pf;

	pcmcia_function_disable(pf);
	pcmcia_intr_disestablish(pf, psc->sc_ih);
}

static int
an_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (pcmcia_product_lookup(pa, an_pcmcia_products, an_pcmcia_nproducts,
	    sizeof(an_pcmcia_products[0]), NULL))
		return (1);
	return (0);
}

static void
an_pcmcia_attach(parent, self, aux)
	struct device  *parent, *self;
	void           *aux;
{
	struct an_pcmcia_softc *psc = (void *)self;
	struct an_softc *sc = &psc->sc_an;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;

	psc->sc_pf = pa->pf;
	if ((cfe = SIMPLEQ_FIRST(&pa->pf->cfe_head)) == NULL) {
		printf("%s: no suitable CIS info found\n", sc->sc_dev.dv_xname);
		goto fail1;
	}

	if (pcmcia_io_alloc(psc->sc_pf, cfe->iospace[0].start,
	    cfe->iospace[0].length, AN_IOSIZ, &psc->sc_pcioh) != 0) {
		printf("%s: failed to allocate io space\n",
		    sc->sc_dev.dv_xname);
		goto fail1;
	}

	if (pcmcia_io_map(psc->sc_pf, PCMCIA_WIDTH_AUTO, &psc->sc_pcioh,
	    &psc->sc_io_window) != 0) {
		printf("%s: failed to map io space\n", sc->sc_dev.dv_xname);
		goto fail2;
	}

	sc->sc_iot = psc->sc_pcioh.iot;
	sc->sc_ioh = psc->sc_pcioh.ioh;

	pcmcia_function_init(psc->sc_pf, cfe);

	if (an_pcmcia_enable(sc)) {
		printf("%s: enable failed\n", sc->sc_dev.dv_xname);
		goto fail3;
	}

	sc->sc_enabled = 1;
	sc->sc_enable = an_pcmcia_enable;
	sc->sc_disable = an_pcmcia_disable;

	if (an_attach(sc) != 0) {
		printf("%s: failed to attach controller\n",
		    sc->sc_dev.dv_xname);
		goto fail4;
	}
	psc->sc_powerhook = powerhook_establish(an_power, sc);

	/* disable device and disestablish the interrupt */
	sc->sc_enabled = 0;
	an_pcmcia_disable(sc);
	return;

fail4:
	sc->sc_enabled = 0;
	an_pcmcia_disable(sc);
fail3:
	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);
fail2:
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);
fail1:
	psc->sc_io_window = -1;
}


static int
an_pcmcia_detach(self, flags)
	struct device *self;
	int flags;
{
	struct an_pcmcia_softc *psc = (struct an_pcmcia_softc *)self;
	int error;

	if (psc->sc_io_window == -1)
		/* Nothing to detach. */
		return (0);

	if (psc->sc_powerhook != NULL)
		powerhook_disestablish(psc->sc_powerhook);

	error = an_detach(&psc->sc_an);
	if (error != 0)
		return (error);

	/* Unmap our i/o window. */
	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);

	/* Free our i/o space. */
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);
	return (0);
}
