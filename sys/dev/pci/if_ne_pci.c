/*	$NetBSD: if_ne_pci.c,v 1.11 1998/10/28 00:15:54 thorpej Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>

#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>

#include <dev/pci/if_ne_pcireg.h>

struct ne_pci_softc {
	struct ne2000_softc sc_ne2000;		/* real "ne2000" softc */

	/* PCI-specific goo */
	void *sc_ih;				/* interrupt handle */
};

int ne_pci_match __P((struct device *, struct cfdata *, void *));
void ne_pci_attach __P((struct device *, struct device *, void *));

struct cfattach ne_pci_ca = {
	sizeof(struct ne_pci_softc), ne_pci_match, ne_pci_attach
};

/*
 * RealTek 8029 media support
 */
int	ne_pci_rtl8029_mediachange __P((struct dp8390_softc *));
void	ne_pci_rtl8029_mediastatus __P((struct dp8390_softc *,
	    struct ifmediareq *));
void	ne_pci_rtl8029_init_card __P((struct dp8390_softc *));
void	ne_pci_rtl8029_init_media __P((struct ne_pci_softc *, int **,
	    int *, int *));

const struct ne_pci_product {
	pci_vendor_id_t npp_vendor;
	pci_product_id_t npp_product;
	int (*npp_mediachange) __P((struct dp8390_softc *));
	void (*npp_mediastatus) __P((struct dp8390_softc *,
	    struct ifmediareq *));
	void (*npp_init_card) __P((struct dp8390_softc *));
	void (*npp_init_media) __P((struct ne_pci_softc *, int **,
	    int *, int *));
	const char *npp_name;
} ne_pci_products[] = {
	{ PCI_VENDOR_REALTEK,		PCI_PRODUCT_REALTEK_RT8029,
	  ne_pci_rtl8029_mediachange,	ne_pci_rtl8029_mediastatus,
	  ne_pci_rtl8029_init_card,	ne_pci_rtl8029_init_media,
	  "RealTek 8029" },

	{ PCI_VENDOR_WINBOND,		PCI_PRODUCT_WINBOND_W89C940F,
	  NULL,				NULL,
	  NULL,				NULL,
	  "Winbond 89C940F" },

	{ PCI_VENDOR_VIATECH,		PCI_PRODUCT_VIATECH_VT86C926,
	  NULL,				NULL,
	  NULL,				NULL,
	  "VIA Technologies VT86C926" },

	{ PCI_VENDOR_SURECOM,		PCI_PRODUCT_SURECOM_NE34,
	  NULL,				NULL,
	  NULL,				NULL,
	  "Surecom NE-34" },

	{ PCI_VENDOR_NETVIN,		PCI_PRODUCT_NETVIN_5000,
	  NULL,				NULL,
	  NULL,				NULL,
	  "NetVin 5000" },

	/* XXX The following entries need sanity checking in pcidevs */
	{ PCI_VENDOR_COMPEX,		PCI_PRODUCT_COMPEX_NE2KETHER,
	  NULL,				NULL,
	  NULL,				NULL,
	  "Compex" },

	{ PCI_VENDOR_PROLAN,		PCI_PRODUCT_PROLAN_NE2KETHER,
	  NULL,				NULL,
	  NULL,				NULL,
	  "ProLAN" },

	{ PCI_VENDOR_KTI,		PCI_PRODUCT_KTI_NE2KETHER,
	  NULL,				NULL,
	  NULL,				NULL,
	  "KTI" },

	{ 0,				0,
	  NULL,				NULL,
	  NULL,				NULL,
	  NULL },
};

const struct ne_pci_product *ne_pci_lookup __P((struct pci_attach_args *));

const struct ne_pci_product *
ne_pci_lookup(pa)
	struct pci_attach_args *pa;
{
	const struct ne_pci_product *npp;

	for (npp = ne_pci_products; npp->npp_name != NULL; npp++) {
		if (PCI_VENDOR(pa->pa_id) == npp->npp_vendor &&
		    PCI_PRODUCT(pa->pa_id) == npp->npp_product)
			return (npp);
	}
	return (NULL);
}

/*
 * PCI constants.
 * XXX These should be in a common file!
 */
#define PCI_CBIO	0x10		/* Configuration Base IO Address */

int
ne_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (ne_pci_lookup(pa) != NULL)
		return (1);

	return (0);
}

void
ne_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ne_pci_softc *psc = (struct ne_pci_softc *)self;
	struct ne2000_softc *nsc = &psc->sc_ne2000;
	struct dp8390_softc *dsc = &nsc->sc_dp8390;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	bus_space_tag_t nict;
	bus_space_handle_t nich;
	bus_space_tag_t asict;
	bus_space_handle_t asich;
	const char *intrstr;
	const struct ne_pci_product *npp;
	pci_intr_handle_t ih;
	pcireg_t csr;
	int *media, nmedia, defmedia;

	npp = ne_pci_lookup(pa);
	if (npp == NULL) {
		printf("\n");
		panic("ne_pci_attach: impossible");
	}

	printf(": %s Ethernet\n", npp->npp_name);

	if (pci_mapreg_map(pa, PCI_CBIO, PCI_MAPREG_TYPE_IO, 0,
	    &nict, &nich, NULL, NULL)) {
		printf("%s: can't map i/o space\n", dsc->sc_dev.dv_xname);
		return;
	}

	asict = nict;
	if (bus_space_subregion(nict, nich, NE2000_ASIC_OFFSET,
	    NE2000_ASIC_NPORTS, &asich)) {
		printf("%s: can't subregion i/o space\n", dsc->sc_dev.dv_xname);
		return;
	}

	dsc->sc_regt = nict;
	dsc->sc_regh = nich;

	nsc->sc_asict = asict;
	nsc->sc_asich = asich;

	/* Enable the card. */
	csr = pci_conf_read(pc, pa->pa_tag,
	    PCI_COMMAND_STATUS_REG);
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    csr | PCI_COMMAND_MASTER_ENABLE);

	/* This interface is always enabled. */
	dsc->sc_enabled = 1;

	if (npp->npp_init_media != NULL) {
		(*npp->npp_init_media)(psc, &media, &nmedia, &defmedia);
		dsc->sc_mediachange = npp->npp_mediachange;
		dsc->sc_mediastatus = npp->npp_mediastatus;
	} else {
		media = NULL;
		nmedia = 0;
		defmedia = 0;
	}

	/* Always fill in init_card; it might be used for non-media stuff. */
	dsc->init_card = npp->npp_init_card;

	/*
	 * Do generic NE2000 attach.  This will read the station address
	 * from the EEPROM.
	 */
	ne2000_attach(nsc, NULL, media, nmedia, defmedia);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", dsc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	psc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, dp8390_intr, dsc);
	if (psc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    dsc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", dsc->sc_dev.dv_xname, intrstr);
}

/*
 * RealTek 8029 media support
 */

int
ne_pci_rtl8029_mediachange(dsc)
	struct dp8390_softc *dsc;
{

	/*
	 * Current media is already set up.  Just reset the interface
	 * to let the new value take hold.  The new media will be
	 * set up in ne_pci_rtl8029_init_card() called via dp8390_init().
	 */
	dp8390_reset(dsc);
	return (0);
}

void
ne_pci_rtl8029_mediastatus(sc, ifmr)
	struct dp8390_softc *sc;
	struct ifmediareq *ifmr;
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	u_int8_t cr_proto = sc->cr_proto |
	    ((ifp->if_flags & IFF_RUNNING) ? ED_CR_STA : ED_CR_STP);

	/*
	 * Sigh, can detect which media is being used, but can't
	 * detect if we have link or not.
	 */

	/* Set NIC to page 3 registers. */
	NIC_PUT(sc->sc_regt, sc->sc_regh, ED_P0_CR, cr_proto | ED_CR_PAGE_3);

	if (NIC_GET(sc->sc_regt, sc->sc_regh, NEPCI_RTL3_CONFIG0) &
	    RTL3_CONFIG0_BNC)
		ifmr->ifm_active = IFM_ETHER|IFM_10_2;
	else {
		ifmr->ifm_active = IFM_ETHER|IFM_10_T;
		if (NIC_GET(sc->sc_regt, sc->sc_regh, NEPCI_RTL3_CONFIG3) &
		    RTL3_CONFIG3_FUDUP)
			ifmr->ifm_active |= IFM_FDX;
	}

	/* Set NIC to page 0 registers. */
	NIC_PUT(sc->sc_regt, sc->sc_regh, ED_P0_CR, cr_proto | ED_CR_PAGE_0);
}

void
ne_pci_rtl8029_init_card(sc)
	struct dp8390_softc *sc;
{
	struct ifmedia *ifm = &sc->sc_media;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	u_int8_t cr_proto = sc->cr_proto |
	    ((ifp->if_flags & IFF_RUNNING) ? ED_CR_STA : ED_CR_STP);
	u_int8_t reg;

	/* Set NIC to page 3 registers. */
	NIC_PUT(sc->sc_regt, sc->sc_regh, ED_P0_CR, cr_proto | ED_CR_PAGE_3);

	/* First, set basic media type. */
	reg = NIC_GET(sc->sc_regt, sc->sc_regh, NEPCI_RTL3_CONFIG2);
	reg &= ~(RTL3_CONFIG2_PL1|RTL3_CONFIG2_PL0);
	switch (IFM_SUBTYPE(ifm->ifm_cur->ifm_media)) {
	case IFM_AUTO:
		/* Nothing to do; both bits clear == auto-detect. */
		break;

	case IFM_10_T:
		reg |= RTL3_CONFIG2_PL0;
		break;

	case IFM_10_2:
		reg |= RTL3_CONFIG2_PL1|RTL3_CONFIG2_PL0;
		break;
	}
	NIC_PUT(sc->sc_regt, sc->sc_regh, NEPCI_RTL3_CONFIG2, reg);

	/* Now, set duplex mode. */
	reg = NIC_GET(sc->sc_regt, sc->sc_regh, NEPCI_RTL3_CONFIG3);
	if (ifm->ifm_cur->ifm_media & IFM_FDX)
		reg |= RTL3_CONFIG3_FUDUP;
	else
		reg &= ~RTL3_CONFIG3_FUDUP;
	NIC_PUT(sc->sc_regt, sc->sc_regh, NEPCI_RTL3_CONFIG3, reg);

	/* Set NIC to page 0 registers. */
	NIC_PUT(sc->sc_regt, sc->sc_regh, ED_P0_CR, cr_proto | ED_CR_PAGE_0);
}

void
ne_pci_rtl8029_init_media(psc, mediap, nmediap, defmediap)
	struct ne_pci_softc *psc;
	int **mediap, *nmediap, *defmediap;
{
	static int rtl8029_media[] = {
		IFM_ETHER|IFM_AUTO,
		IFM_ETHER|IFM_10_T,
		IFM_ETHER|IFM_10_T|IFM_FDX,
		IFM_ETHER|IFM_10_2,
	};

	printf("%s: 10base2, 10baseT, 10baseT-FDX, auto, default auto\n",
	    psc->sc_ne2000.sc_dp8390.sc_dev.dv_xname);

	*mediap = rtl8029_media;
	*nmediap = sizeof(rtl8029_media) / sizeof(rtl8029_media[0]);
	*defmediap = IFM_ETHER|IFM_AUTO;
}
