/* $NetBSD: if_an_pcmcia.c,v 1.9 2001/04/06 09:28:39 onoe Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"

#ifdef INET
#define	ANCACHE		/* XXX: should be defined elsewhere */
#endif

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
#include <net/if_ieee80211.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

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

struct cfattach an_pcmcia_ca = {
	sizeof(struct an_pcmcia_softc), an_pcmcia_match, an_pcmcia_attach,
	an_pcmcia_detach, an_activate
};

static struct an_pcmcia_product {
	u_int32_t	app_vendor;	/* vendor ID */
	u_int32_t	app_product;	/* product ID */
	const char	*app_cisinfo[4]; /* CIS information */
	const char	*app_name;	/* product name */
} an_pcmcia_products[] = {
	{ PCMCIA_VENDOR_AIRONET,	PCMCIA_PRODUCT_AIRONET_PC4800,
	  PCMCIA_CIS_AIRONET_PC4800,	PCMCIA_STR_AIRONET_PC4800 },
	{ PCMCIA_VENDOR_AIRONET,	PCMCIA_PRODUCT_AIRONET_PC4500,
	  PCMCIA_CIS_AIRONET_PC4500,	PCMCIA_STR_AIRONET_PC4500 },
	{ PCMCIA_VENDOR_AIRONET,	PCMCIA_PRODUCT_AIRONET_350,
	  PCMCIA_CIS_AIRONET_350,	PCMCIA_STR_AIRONET_350 },
	{ 0,				0,
	  { NULL, NULL, NULL, NULL },	NULL }
};

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
		    sc->an_dev.dv_xname);
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
	struct an_pcmcia_product *app;

	for (app = an_pcmcia_products; app->app_name != NULL; app++) {
		/* match by vendor/product id */
		if (pa->manufacturer != PCMCIA_VENDOR_INVALID &&
		    pa->manufacturer == app->app_vendor &&
		    pa->product != PCMCIA_PRODUCT_INVALID &&
		    pa->product == app->app_product)
			return 1;

		/* match by CIS information */
		if (pa->card->cis1_info[0] != NULL &&
		    app->app_cisinfo[0] != NULL &&
		    strcmp(pa->card->cis1_info[0], app->app_cisinfo[0]) == 0 &&
		    pa->card->cis1_info[1] != NULL &&
		    app->app_cisinfo[1] != NULL &&
		    strcmp(pa->card->cis1_info[1], app->app_cisinfo[1]) == 0)
			return 1;
	}
	return 0;
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
	char devinfo[256];

	/* Print out what we are. */
	pcmcia_devinfo(&pa->pf->sc->card, 0, devinfo, sizeof(devinfo));
	printf(": %s\n", devinfo);

	psc->sc_pf = pa->pf;
	if ((cfe = SIMPLEQ_FIRST(&pa->pf->cfe_head)) == NULL) {
		printf("%s: no suitable CIS info found\n", sc->an_dev.dv_xname);
		goto fail1;
	}

	if (pcmcia_io_alloc(psc->sc_pf, cfe->iospace[0].start,
	    cfe->iospace[0].length, AN_IOSIZ, &psc->sc_pcioh) != 0) {
		printf("%s: failed to allocate io space\n",
		    sc->an_dev.dv_xname);
		goto fail1;
	}

	if (pcmcia_io_map(psc->sc_pf, PCMCIA_WIDTH_AUTO, 0, psc->sc_pcioh.size,
	    &psc->sc_pcioh, &psc->sc_io_window) != 0) {
		printf("%s: failed to map io space\n", sc->an_dev.dv_xname);
		goto fail2;
	}

	pcmcia_function_init(psc->sc_pf, cfe);

	if (pcmcia_function_enable(psc->sc_pf)) {
		printf("%s: failed to enable pcmcia\n", sc->an_dev.dv_xname);
		goto fail3;
	}

	if ((psc->sc_ih = pcmcia_intr_establish(psc->sc_pf, IPL_NET, an_intr,
	    sc)) == NULL) {
		printf("%s: unable to establish interrupt\n",
		    sc->an_dev.dv_xname);
		goto fail4;
	}

	sc->an_btag = psc->sc_pcioh.iot;
	sc->an_bhandle = psc->sc_pcioh.ioh;
	sc->sc_enabled = 1;
	sc->sc_enable = an_pcmcia_enable;
	sc->sc_disable = an_pcmcia_disable;

	if (an_attach(sc) != 0) {
		printf("%s: failed to attach controller\n",
		    sc->an_dev.dv_xname);
		goto fail5;;
	}
	psc->sc_powerhook = powerhook_establish(an_power, sc);
	sc->sc_enabled = 0;

	/* disable device and disestablish the interrupt */
	an_pcmcia_disable(sc);
	return;

  fail5:
	pcmcia_intr_disestablish(psc->sc_pf, psc->sc_ih);
  fail4:
	pcmcia_function_disable(psc->sc_pf);
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
