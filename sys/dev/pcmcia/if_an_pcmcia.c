/* $NetBSD: if_an_pcmcia.c,v 1.4 2000/12/12 05:34:02 onoe Exp $ */

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
static void an_pcmcia_powerhook __P((int, void *));

struct an_pcmcia_softc {
	struct an_softc sc_an;		/* real "an" softc */

	/* PCMCIA-specific goo */
	struct pcmcia_io_handle sc_pcioh;	/* PCMCIA i/o space info */
	int sc_io_window;			/* our i/o window */
	struct pcmcia_function *sc_pf;		/* our PCMCIA function */
	void *sc_powerhook;			/* power hook descriptor */
};

static int	an_pcmcia_find __P((struct an_pcmcia_softc *,
    struct pcmcia_attach_args *, struct pcmcia_config_entry *));

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
	{ 0,				0,
	  { NULL, NULL, NULL, NULL },	NULL }
};

static struct an_pcmcia_product *
	an_pcmcia_lookup __P((struct pcmcia_attach_args *));

static struct an_pcmcia_product *
an_pcmcia_lookup(pa)
	struct pcmcia_attach_args *pa;
{
	struct an_pcmcia_product *app;

	for (app = an_pcmcia_products; app->app_name != NULL; app++) {
		/* match by vendor/product id */
		if (pa->manufacturer != PCMCIA_VENDOR_INVALID &&
		    pa->manufacturer == app->app_vendor &&
		    pa->product != PCMCIA_PRODUCT_INVALID &&
		    pa->product == app->app_product)
			return (app);

		/* match by CIS information */
		if (pa->card->cis1_info[0] != NULL &&
		    app->app_cisinfo[0] != NULL &&
		    strcmp(pa->card->cis1_info[0], app->app_cisinfo[0]) == 0 &&
		    pa->card->cis1_info[1] != NULL &&
		    app->app_cisinfo[1] != NULL &&
		    strcmp(pa->card->cis1_info[1], app->app_cisinfo[1]) == 0)
			return (app);
	}

	return (NULL);
}

static int
an_pcmcia_enable(sc)
	struct an_softc *sc;
{
	struct an_pcmcia_softc *psc = (struct an_pcmcia_softc *)sc;
	struct pcmcia_function *pf = psc->sc_pf;

	/* establish the interrupt. */
	sc->irq_handle = pcmcia_intr_establish(pf, IPL_NET, an_intr, sc);
	if (sc->irq_handle == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->an_dev.dv_xname);
		return (1);
	}

	if (pcmcia_function_enable(pf)) {
		pcmcia_intr_disestablish(pf, sc->irq_handle);
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
	pcmcia_intr_disestablish(pf, sc->irq_handle);
}

static int
an_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (an_pcmcia_lookup(pa) != NULL)
		return (1);

	return (0);
}

static int
an_pcmcia_find(psc, pa, cfe)
	struct an_pcmcia_softc *psc;
	struct pcmcia_attach_args *pa;
	struct pcmcia_config_entry *cfe;
{
	struct an_softc *sc = &psc->sc_an;
	int fail = 0;

	/*
	 * see if we can read the firmware version sanely
	 * through the i/o ports.
	 * if not, try a different CIS string..
	 */
	if (pcmcia_io_alloc(psc->sc_pf, cfe->iospace[0].start,
	    cfe->iospace[0].length, AN_IOSIZ,
	    &psc->sc_pcioh) != 0)
		goto fail;

	if (pcmcia_io_map(psc->sc_pf, PCMCIA_WIDTH_AUTO, 0, psc->sc_pcioh.size,
	    &psc->sc_pcioh, &psc->sc_io_window))
		goto fail_io_free;

	/* Enable the card. */
	pcmcia_function_init(psc->sc_pf, cfe);
	if (pcmcia_function_enable(psc->sc_pf))
		goto fail_io_unmap;

	sc->an_btag = psc->sc_pcioh.iot;
	sc->an_bhandle = psc->sc_pcioh.ioh;

	if (an_probe(sc))
		return 0;

	fail++;
	pcmcia_function_disable(psc->sc_pf);

 fail_io_unmap:
	fail++;
	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);

 fail_io_free:
	fail++;
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);
 fail:
	fail++;
	return (fail);
}

static void
an_pcmcia_attach(parent, self, aux)
	struct device  *parent, *self;
	void           *aux;
{
	struct an_pcmcia_softc *psc = (void *)self;
	struct an_softc *sc = &psc->sc_an;
	struct an_pcmcia_product *app;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;

	app = an_pcmcia_lookup(pa);
	if (app == NULL)
		panic("an_pcmcia_attach: impossible");

	psc->sc_pf = pa->pf;

	for (cfe = SIMPLEQ_FIRST(&pa->pf->cfe_head); cfe != NULL;
	     cfe = SIMPLEQ_NEXT(cfe, cfe_list)) {
		if (cfe->iftype != PCMCIA_IFTYPE_IO)
			continue;
		if (cfe->num_iospace < 1)
			continue;
		if (cfe->iospace[0].length < AN_IOSIZ)
			continue;

		if (an_pcmcia_find(psc, pa, cfe) == 0)
			break;
	}
	if (cfe == NULL) {
		printf(": no suitable CIS info found\n");
		goto no_config_entry;
	}

	sc->sc_enabled = 1;
	printf(": %s\n", app->app_name);

	sc->sc_enable = an_pcmcia_enable;
	sc->sc_disable = an_pcmcia_disable;

	/* establish the interrupt. */
	sc->irq_handle = pcmcia_intr_establish(psc->sc_pf, IPL_NET, an_intr, sc);
	if (sc->irq_handle == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->an_dev.dv_xname);
		goto no_interrupt;
	}

	if (an_attach(sc) != 0) {
		printf("%s: failed to attach controller\n",
		    sc->an_dev.dv_xname);
		goto attach_failed;
	}
	psc->sc_powerhook = powerhook_establish(an_pcmcia_powerhook, psc);
	sc->sc_enabled = 0;

	/* disable device and disestablish the interrupt */
	an_pcmcia_disable(sc);
	return;

 attach_failed:
	pcmcia_intr_disestablish(psc->sc_pf, sc->irq_handle);

 no_interrupt:
	/* Unmap our i/o window and space */
	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);

	/* Disable the function */
	pcmcia_function_disable(psc->sc_pf);

 no_config_entry:
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

static void
an_pcmcia_powerhook(why, arg)
	int why;
	void *arg;
{
#if 0
	struct an_pcmcia_softc *psc = arg;
	struct an_softc *sc = &psc->sc_an;

	an_power(sc, why);
#endif
}
