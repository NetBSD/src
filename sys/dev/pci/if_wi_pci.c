/*      $NetBSD: if_wi_pci.c,v 1.17 2003/03/27 04:34:16 dyoung Exp $  */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_wi_pci.c,v 1.17 2003/03/27 04:34:16 dyoung Exp $");

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

#define WI_PCI_CBMA		0x10	/* Configuration Base Memory Address */
#define WI_PCI_PLX_LOMEM	0x10	/* PLX chip membase */
#define WI_PCI_PLX_LOIO		0x14	/* PLX chip iobase */
#define WI_PCI_LOMEM		0x18	/* ISA membase */
#define WI_PCI_LOIO		0x1C	/* ISA iobase */

#define WI_PLX_COR_OFFSET       0x3E0
#define WI_PLX_COR_VALUE        0x41

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
static void	wi_pci_reset __P((struct wi_softc *));
static void	wi_pci_powerhook __P((int, void *));

static const struct wi_pci_product
	*wi_pci_lookup __P((struct pci_attach_args *));

CFATTACH_DECL(wi_pci, sizeof(struct wi_pci_softc),
    wi_pci_match, wi_pci_attach, NULL, NULL);

const struct wi_pci_product {
	pci_vendor_id_t		wpp_vendor;	/* vendor ID */
	pci_product_id_t	wpp_product;	/* product ID */
	const char		*wpp_name;	/* product name */
	int			wpp_plx;	/* uses PLX chip */
} wi_pci_products[] = {
	{ PCI_VENDOR_GLOBALSUN,		PCI_PRODUCT_GLOBALSUN_GL24110P,
	  NULL, 1 },
	{ PCI_VENDOR_GLOBALSUN,		PCI_PRODUCT_GLOBALSUN_GL24110P02,
	  NULL, 1 },
	{ PCI_VENDOR_EUMITCOM,		PCI_PRODUCT_EUMITCOM_WL11000P,
	  NULL, 1 },
	{ PCI_VENDOR_3COM,		PCI_PRODUCT_3COM_3CRWE777A,
	  NULL, 1 },
	{ PCI_VENDOR_NETGEAR,		PCI_PRODUCT_NETGEAR_MA301,
	  NULL, 1 },
	{ PCI_VENDOR_INTERSIL,		PCI_PRODUCT_INTERSIL_MINI_PCI_WLAN,
	  "Intersil Prism2.5", 0 },
	{ 0,				0,
	  NULL, 0},
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

static void
wi_pci_reset(sc)
	struct wi_softc		*sc;
{
	int i, secs, usecs;

	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
	    WI_PCI_COR, WI_COR_SOFT_RESET);
	DELAY(250*1000); /* 1/4 second */

	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
	    WI_PCI_COR, WI_COR_CLEAR);
	DELAY(500*1000); /* 1/2 second */

	/* wait 2 seconds for firmware to complete initialization. */

	for (i = 200000; i--; DELAY(10))
		if (!(CSR_READ_2(sc, WI_COMMAND) & WI_CMD_BUSY))
			break;
 
	if (i < 0) {
		printf("%s: PCI reset timed out\n", sc->sc_dev.dv_xname);
	} else if (sc->sc_if.if_flags & IFF_DEBUG) {
		usecs = (200000 - i) * 10;
		secs = usecs / 1000000;
		usecs %= 1000000;

		printf("%s: PCI reset in %d.%06d seconds\n",
                       sc->sc_dev.dv_xname, secs, usecs);
	}

	return;
}

static const struct wi_pci_product *
wi_pci_lookup(pa)
	struct pci_attach_args *pa;
{
	const struct wi_pci_product *wpp;

	for (wpp = wi_pci_products; wpp->wpp_vendor != 0; wpp++) {
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
	bus_space_tag_t memt, iot;
	bus_space_handle_t memh, ioh;

	psc->psc_pa = pa;

	wpp = wi_pci_lookup(pa);
#ifdef DIAGNOSTIC
	if (wpp == NULL) {
		printf("\n");
		panic("wi_pci_attach: impossible");
	}
#endif

	if (wpp->wpp_plx) {
		/* Map memory and I/O registers. */
		if (pci_mapreg_map(pa, WI_PCI_LOMEM, PCI_MAPREG_TYPE_MEM, 0,
		    &memt, &memh, NULL, NULL) != 0) {
			printf(": can't map mem space\n");
			return;
		}
		if (pci_mapreg_map(pa, WI_PCI_LOIO, PCI_MAPREG_TYPE_IO, 0,
		    &iot, &ioh, NULL, NULL) != 0) {
			printf(": can't map I/O space\n");
			return;
		}
	} else {
		if (pci_mapreg_map(pa, WI_PCI_CBMA,
		    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT,
		    0, &iot, &ioh, NULL, NULL) != 0) {
			printf(": can't map mem space\n");
			return;
		}

		memt = iot;
		memh = ioh;
		sc->sc_pci = 1;
	}

	if (wpp->wpp_name != NULL) {
		printf(": %s Wireless Lan\n", wpp->wpp_name);
	} else {
		char devinfo[256];

		pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
		printf(": %s (rev. 0x%02x)\n", devinfo,
		       PCI_REVISION(pa->pa_class));
	}

	sc->sc_enabled = 1;
	sc->sc_enable = wi_pci_enable;
	sc->sc_disable = wi_pci_disable;

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	/* Make sure interrupts are disabled. */
	CSR_WRITE_2(sc, WI_INT_EN, 0);
	CSR_WRITE_2(sc, WI_EVENT_ACK, 0xFFFF);

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

	if (wpp->wpp_plx) {
		/*
		 * Setup the PLX chip for level interrupts and config index 1
		 * XXX - should really reset the PLX chip too.
		 */
		bus_space_write_1(memt, memh,
		    WI_PLX_COR_OFFSET, WI_PLX_COR_VALUE);
	} else {
		/* reset HFA3842 MAC core */
		wi_pci_reset(sc);
	}

	printf("%s:", sc->sc_dev.dv_xname);
	if (wi_attach(sc) != 0) {
		printf("%s: failed to attach controller\n",
			sc->sc_dev.dv_xname);
		pci_intr_disestablish(pa->pa_pc, sc->sc_ih);
		return;
	}

	sc->sc_reset = wi_pci_reset;

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
