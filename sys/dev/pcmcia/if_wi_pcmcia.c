/* $NetBSD: if_wi_pcmcia.c,v 1.5 2001/07/01 16:35:37 thorpej Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA (ichiro@ichiro.org).
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

/*
 * PCMCIA attachment for Lucent & Intersil WaveLAN PCMCIA card
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/if_ieee80211.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/wi_ieee.h>
#include <dev/ic/wireg.h>
#include <dev/ic/wivar.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

static int	wi_pcmcia_match __P((struct device *, struct cfdata *, void *));
static void	wi_pcmcia_attach __P((struct device *, struct device *, void *));
static int	wi_pcmcia_detach __P((struct device *, int));
static int	wi_pcmcia_enable __P((struct wi_softc *));
static void	wi_pcmcia_disable __P((struct wi_softc *));
static void	wi_pcmcia_powerhook __P((int, void *));
static void	wi_pcmcia_shutdown __P((void *));

static const struct wi_pcmcia_product
		*wi_pcmcia_lookup __P((struct pcmcia_attach_args *pa));

struct wi_pcmcia_softc {
	struct wi_softc sc_wi;

	/* PCMCIA-specific */
	struct pcmcia_io_handle sc_pcioh;	/* PCMCIA i/o space info */
	int sc_io_window;			/* our i/o window */
	struct pcmcia_function *sc_pf;		/* PCMCIA function */
	void *sc_powerhook;			/* power hook descriptor */
	void *sc_sdhook;			/* shutdown hook */

};

static int wi_pcmcia_find __P((struct wi_pcmcia_softc *,
	struct pcmcia_attach_args *, struct pcmcia_config_entry *));

struct cfattach wi_pcmcia_ca = {
	sizeof(struct wi_pcmcia_softc), wi_pcmcia_match, wi_pcmcia_attach,
	wi_pcmcia_detach, wi_activate,
};

static const struct wi_pcmcia_product {
	u_int32_t	pp_vendor;	/* vendor ID */
	u_int32_t	pp_product;	/* product ID */
	const char	*pp_cisinfo[4];	/* CIS information */
	const char	*pp_name;	/* product name */
} wi_pcmcia_products[] = {
	{ PCMCIA_VENDOR_LUCENT,
	  PCMCIA_PRODUCT_LUCENT_WAVELAN_IEEE,
	  PCMCIA_CIS_LUCENT_WAVELAN_IEEE,
	  PCMCIA_STR_LUCENT_WAVELAN_IEEE },

	{ PCMCIA_VENDOR_3COM,
	  PCMCIA_PRODUCT_3COM_3CRWE737A,
	  PCMCIA_CIS_3COM_3CRWE737A,
	  PCMCIA_STR_3COM_3CRWE737A },

	{ PCMCIA_VENDOR_COREGA,
	  PCMCIA_PRODUCT_COREGA_WIRELESS_LAN_PCC_11,
	  PCMCIA_CIS_COREGA_WIRELESS_LAN_PCC_11,
	  PCMCIA_STR_COREGA_WIRELESS_LAN_PCC_11 },

	{ PCMCIA_VENDOR_COREGA,
	  PCMCIA_PRODUCT_COREGA_WIRELESS_LAN_PCCA_11,
	  PCMCIA_CIS_COREGA_WIRELESS_LAN_PCCA_11,
	  PCMCIA_STR_COREGA_WIRELESS_LAN_PCCA_11 },

	{ PCMCIA_VENDOR_INTERSIL,
	  PCMCIA_PRODUCT_INTERSIL_PRISM2,
	  PCMCIA_CIS_INTERSIL_PRISM2,
	  PCMCIA_STR_INTERSIL_PRISM2 },

	{ PCMCIA_VENDOR_SAMSUNG,
	  PCMCIA_PRODUCT_SAMSUNG_SWL_2000N,
	  PCMCIA_CIS_SAMSUNG_SWL_2000N,
	  PCMCIA_STR_SAMSUNG_SWL_2000N },

	{ PCMCIA_VENDOR_LUCENT,
	  PCMCIA_PRODUCT_LUCENT_WAVELAN_IEEE,
	  PCMCIA_CIS_SMC_2632W,
	  PCMCIA_STR_SMC_2632W },

	{ PCMCIA_VENDOR_LUCENT,
	  PCMCIA_PRODUCT_LUCENT_WAVELAN_IEEE,
	  PCMCIA_CIS_NANOSPEED_PRISM2,
	  PCMCIA_STR_NANOSPEED_PRISM2 },

	{ PCMCIA_VENDOR_ELSA,
	  PCMCIA_PRODUCT_ELSA_XI300_IEEE,
	  PCMCIA_CIS_ELSA_XI300_IEEE,
	  PCMCIA_STR_ELSA_XI300_IEEE },

	{ PCMCIA_VENDOR_COMPAQ,
	  PCMCIA_PRODUCT_COMPAQ_NC5004,
	  PCMCIA_CIS_COMPAQ_NC5004,
	  PCMCIA_STR_COMPAQ_NC5004 },

	{ PCMCIA_VENDOR_CONTEC,
	  PCMCIA_PRODUCT_CONTEC_FX_DS110_PCC,
	  PCMCIA_CIS_CONTEC_FX_DS110_PCC,
	  PCMCIA_STR_CONTEC_FX_DS110_PCC },

	{ PCMCIA_VENDOR_TDK,
	  PCMCIA_PRODUCT_TDK_LAK_CD011WL,
	  PCMCIA_CIS_TDK_LAK_CD011WL,
	  PCMCIA_STR_TDK_LAK_CD011WL },

	{ PCMCIA_VENDOR_LUCENT,
	  PCMCIA_PRODUCT_LUCENT_WAVELAN_IEEE,
	  PCMCIA_CIS_NEC_CMZ_RT_WP,
	  PCMCIA_STR_NEC_CMZ_RT_WP },

	{ PCMCIA_VENDOR_LUCENT,
	  PCMCIA_PRODUCT_LUCENT_WAVELAN_IEEE,
	  PCMCIA_CIS_NTT_ME_WLAN,
	  PCMCIA_STR_NTT_ME_WLAN },

	{ PCMCIA_VENDOR_IODATA2,
	  PCMCIA_PRODUCT_IODATA2_WNB11PCM,
	  PCMCIA_CIS_IODATA2_WNB11PCM,
	  PCMCIA_STR_IODATA2_WNB11PCM },

	{ PCMCIA_VENDOR_BUFFALO,
	  PCMCIA_PRODUCT_BUFFALO_WLI_PCM_S11,
	  PCMCIA_CIS_BUFFALO_WLI_PCM_S11,
	  PCMCIA_STR_BUFFALO_WLI_PCM_S11 },

	{ PCMCIA_VENDOR_EMTAC,
	  PCMCIA_PRODUCT_EMTAC_WLAN,
	  PCMCIA_CIS_EMTAC_WLAN,
	  PCMCIA_STR_EMTAC_WLAN },

	{ 0,
	  0,
	  { NULL, NULL, NULL, NULL },
	  NULL }
};

static const struct wi_pcmcia_product *
wi_pcmcia_lookup(pa)
	struct pcmcia_attach_args *pa;
{
	const struct wi_pcmcia_product *pp;

	/* match by CIS information */
	for (pp = wi_pcmcia_products; pp->pp_name != NULL; pp++) {
		if (pa->card->cis1_info[0] != NULL &&
		    pp->pp_cisinfo[0] != NULL &&
		    strcmp(pa->card->cis1_info[0], pp->pp_cisinfo[0]) == 0 &&
		    pa->card->cis1_info[1] != NULL &&
		    pp->pp_cisinfo[1] != NULL &&
		    strcmp(pa->card->cis1_info[1], pp->pp_cisinfo[1]) == 0)
			return pp;
	}

	/* match by vendor/product id */
	for (pp = wi_pcmcia_products; pp->pp_name != NULL; pp++) {
		if (pa->manufacturer != PCMCIA_VENDOR_INVALID &&
		    pa->manufacturer == pp->pp_vendor &&
		    pa->product != PCMCIA_PRODUCT_INVALID &&
		    pa->product == pp->pp_product)
			return pp;
	}

	return (NULL);
}

static int
wi_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (wi_pcmcia_lookup(pa) != NULL)
		return (1);
	return (0);
}

static int
wi_pcmcia_enable(sc)
	struct wi_softc *sc;
{
	struct wi_pcmcia_softc *psc = (struct wi_pcmcia_softc *)sc;
	struct pcmcia_function *pf = psc->sc_pf;

	/* establish the interrupt. */
	sc->sc_ih = pcmcia_intr_establish(pf, IPL_NET, wi_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return (EIO);
	}
	if (pcmcia_function_enable(pf) != 0) {
		printf("%s: couldn't enable card\n", sc->sc_dev.dv_xname);
		pcmcia_intr_disestablish(pf, sc->sc_ih);
		return (EIO);
	}
	DELAY(1000);
	return (0);
}

static void
wi_pcmcia_disable(sc)
	struct wi_softc *sc;
{
	struct wi_pcmcia_softc *psc = (struct wi_pcmcia_softc *)sc;

	pcmcia_intr_disestablish(psc->sc_pf, sc->sc_ih);
	pcmcia_function_disable(psc->sc_pf);
}

static int
wi_pcmcia_find(psc, pa, cfe)
	struct wi_pcmcia_softc *psc;
	struct pcmcia_attach_args *pa;
	struct pcmcia_config_entry *cfe;
{
	struct wi_softc *sc = &psc->sc_wi;

	/* Allocate/map I/O space. */
	if (pcmcia_io_alloc(psc->sc_pf, cfe->iospace[0].start,
	    cfe->iospace[0].length, WI_IOSIZE,
	    &psc->sc_pcioh) != 0) {
		printf("%s: can't allocate i/o space\n",
			sc->sc_dev.dv_xname);
		goto fail1;
	}
	printf("%s:", sc->sc_dev.dv_xname);
	if (pcmcia_io_map(psc->sc_pf, PCMCIA_WIDTH_AUTO, 0,
	    psc->sc_pcioh.size, &psc->sc_pcioh,
	    &psc->sc_io_window) != 0) {
		printf(" can't map i/o space\n");
		goto fail2;
	}
	/* Enable the card */
	pcmcia_function_init(psc->sc_pf, cfe);
	if (pcmcia_function_enable(psc->sc_pf)) {
		printf("%s: function enable failed\n", sc->sc_dev.dv_xname);
		goto fail3;
	}
	
	sc->sc_iot = psc->sc_pcioh.iot;
	sc->sc_ioh = psc->sc_pcioh.ioh;

	DELAY(1000);
	return(0);

fail3:
	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);
fail2:
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);
fail1:
	psc->sc_io_window = -1;
	return(1);
}

static void
wi_pcmcia_attach(parent, self, aux)
	struct device  *parent, *self;
	void           *aux;
{
	struct wi_pcmcia_softc *psc = (void *)self;
	struct wi_softc *sc = &psc->sc_wi;
	const struct wi_pcmcia_product *pp;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	char devinfo[256];

	/* Print out what we are. */
	pcmcia_devinfo(&pa->pf->sc->card, 0, devinfo, sizeof(devinfo));
	printf(": %s\n", devinfo);

	psc->sc_pf = pa->pf;

	for (cfe = SIMPLEQ_FIRST(&pa->pf->cfe_head); cfe != NULL;
	     cfe = SIMPLEQ_NEXT(cfe, cfe_list)) {
		if (cfe->iftype != PCMCIA_IFTYPE_IO)
			continue;
		if (cfe->iospace[0].length < WI_IOSIZE)
			continue;
		if (wi_pcmcia_find(psc, pa, cfe) == 0)
			break;
	}
	if (cfe == NULL) {
		printf(": no suitable CIS info found\n");
		goto no_config_entry;
	}

	pp = wi_pcmcia_lookup(pa);
	if (pp == NULL)
		panic("wi_pcmcia_attach: impossible");

	sc->sc_enabled = 1;
	sc->sc_enable = wi_pcmcia_enable;
	sc->sc_disable = wi_pcmcia_disable;

	/* establish the interrupt. */
	sc->sc_ih = pcmcia_intr_establish(psc->sc_pf, IPL_NET, wi_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		        sc->sc_dev.dv_xname);
		goto no_interrupt;
	}

	sc->sc_ifp = &sc->sc_ethercom.ec_if;
	if (wi_attach(sc) != 0) {
		printf("%s: failed to attach controller\n",
		    sc->sc_dev.dv_xname);
			goto attach_failed;
	}

	psc->sc_sdhook    = shutdownhook_establish(wi_pcmcia_shutdown, psc);
	psc->sc_powerhook = powerhook_establish(wi_pcmcia_powerhook, psc);

	/* disable the card */
	sc->sc_enabled = 0;
	wi_pcmcia_disable(sc);

	return;

attach_failed:
	pcmcia_intr_disestablish(psc->sc_pf, sc->sc_ih);
no_interrupt:
	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);
no_config_entry:
	psc->sc_io_window = -1;
}

static int
wi_pcmcia_detach(self, flags)
	struct device *self;
	int flags;
{
	struct wi_pcmcia_softc *psc = (struct wi_pcmcia_softc *)self;
	int error;

	if (psc->sc_io_window == -1)
		/* Nothing to detach. */
		return (0);

	if (psc->sc_powerhook != NULL)
		powerhook_disestablish(psc->sc_powerhook);
	if (psc->sc_sdhook != NULL)
		shutdownhook_disestablish(psc->sc_sdhook);

	error = wi_detach(&psc->sc_wi);
	if (error != 0)
		return (error);

	/* Unmap our i/o window. */
	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);

	/* Free our i/o space. */
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);
	return (0);
}

static void
wi_pcmcia_powerhook(why, arg)
	int why;
	void *arg;
{
	struct wi_pcmcia_softc *psc = arg;
	struct wi_softc *sc = &psc->sc_wi;

	wi_power(sc, why);
}

static void
wi_pcmcia_shutdown(arg)
	void *arg;
{
	struct wi_pcmcia_softc *psc = arg;
	struct wi_softc *sc = &psc->sc_wi;

	wi_shutdown(sc);  
}
