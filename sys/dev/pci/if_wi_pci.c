/*      $NetBSD: if_wi_pci.c,v 1.1 2001/10/13 15:01:07 ichiro Exp $  */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Hideaki Imaizumi <hiddy@sfc.wide.ad.jp>
 * and Ichiro FUKUHARA (ichiro@ichiro.org).
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
 * PCI bus front-end for the Intersil PCI WaveLan.
 * Works with Prism2.5 Mini-PCI wavelan.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/callout.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/if_ieee80211.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/wi_ieee.h>
#include <dev/ic/wireg.h>
#include <dev/ic/wivar.h>

struct wi_pci_softc {
	struct wi_softc psc_wi;		/* real "wi" softc */

	/* PCI-specific goo */
	pci_intr_handle_t psc_ih;	
	struct pci_attach_args *psc_pa;  

	void *sc_powerhook;		/* power hook descriptor */
};

static int	wi_pci_match __P((struct device *, struct cfdata *, void *));
static void	wi_pci_attach __P((struct device *, struct device *, void *));
static int	wi_pci_enable __P((struct wi_softc *));
static void	wi_pci_disable __P((struct wi_softc *));
static void	wi_pci_powerhook __P((int, void *));

static const struct wi_pci_product
	*wi_pci_lookup __P((struct pci_attach_args *));

struct cfattach wi_pci_ca = {
	sizeof(struct wi_pci_softc), wi_pci_match, wi_pci_attach
};

const struct wi_pci_product {
	pci_vendor_id_t		wpp_vendor;	/* vendor ID */
	pci_product_id_t	wpp_product;	/* product ID */
	const char		*wpp_name;	/* product name */
} wi_pci_products[] = {
	{ PCI_VENDOR_INTERSIL,		PCI_PRODUCT_INTERSIL_MINI_PCI_WLAN,
	  "Intersil Prism2.5" },
	{ 0,				0,
	  NULL},
};

static int
wi_pci_enable(sc)
	struct wi_softc *sc;
{
	struct wi_pci_softc *psc = (struct wi_pci_softc *)sc;

	/* establish the interrupt. */
	sc->sc_ih = pci_intr_establish(psc->psc_pa->pa_pc, 
					psc->psc_ih, IPL_NET, wi_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return (EIO);
	}
	sc->sc_pci = 1;

	/* reset HFA3842 MAC core */
	wi_pci_reset(sc);

	return (0);
}

static void
wi_pci_disable(sc)
	struct wi_softc *sc;
{
	struct wi_pci_softc *psc = (struct wi_pci_softc *)sc;

	pci_intr_disestablish(psc->psc_pa->pa_pc, sc->sc_ih);
}

static const struct wi_pci_product *
wi_pci_lookup(pa)
	struct pci_attach_args *pa;
{
	const struct wi_pci_product *wpp;

	for (wpp = wi_pci_products; wpp->wpp_name != NULL; wpp++) {
		if (PCI_VENDOR(pa->pa_id) == wpp->wpp_vendor &&
		    PCI_PRODUCT(pa->pa_id) == wpp->wpp_product)
			return (wpp);
	}
	return (NULL);
}

static int
wi_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (wi_pci_lookup(pa) != NULL)
		return (1);
	return (0);
}

static void
wi_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wi_pci_softc *psc = (struct wi_pci_softc *)self;
	struct wi_softc *sc = &psc->psc_wi;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	const char *intrstr;
	const struct wi_pci_product *wpp;
	pci_intr_handle_t ih;
	bus_space_tag_t memt;
	bus_space_handle_t memh;
	int memh_valid;
	bus_addr_t busbase; 
	bus_size_t bussize;

	psc->psc_pa = pa;

	memh_valid = !pci_mapreg_map(pa, WI_PCI_CBMA, 
		PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    	    		   &memt, &memh, &busbase, &bussize);

	if(memh_valid){
		sc->sc_iot = memt;
		sc->sc_ioh = memh;
	}
	else {
		printf(": unable to map device registers\n");
		return;
	}

	wpp = wi_pci_lookup(pa);
	if (wpp == NULL) {
		printf("\n");
		panic("wi_pci_attach: impossible");
	}

	printf(": %s Wireless Lan\n", wpp->wpp_name);

	sc->sc_pci = 1;
	sc->sc_enabled = 1;
	sc->sc_enable = wi_pci_enable;
	sc->sc_disable = wi_pci_disable;

	/* Enable the card. */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
		pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
		PCI_COMMAND_MASTER_ENABLE);


	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);

	psc->psc_ih = ih;
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, wi_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}

	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/* reset HFA3842 MAC core */
	wi_pci_reset(sc);

	sc->sc_ifp = &sc->sc_ethercom.ec_if;
	if (wi_attach(sc) != 0) {
		printf("%s: failed to attach controller\n",
		    sc->sc_dev.dv_xname);
		pci_intr_disestablish(pa->pa_pc, sc->sc_ih);
		return;
	}

	/* Add a suspend hook to restore PCI config state */
	psc->sc_powerhook = powerhook_establish(wi_pci_powerhook, psc);
	if (psc->sc_powerhook == NULL)
		printf ("%s: WARNING: unable to establish pci power hook\n",
		        sc->sc_dev.dv_xname);
}

static void
wi_pci_powerhook(why, arg)
	int why;
	void *arg;
{
	struct wi_pci_softc *psc = arg;
	struct wi_softc *sc = &psc->psc_wi;

	wi_power(sc, why);
}
