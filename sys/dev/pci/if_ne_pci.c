/*	$NetBSD: if_ne_pci.c,v 1.28 2006/10/12 01:31:30 christos Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ne_pci.c,v 1.28 2006/10/12 01:31:30 christos Exp $");

#include "opt_ipkdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <machine/intr.h>

#ifdef IPKDB_NE_PCI
#include <ipkdb/ipkdb.h>
#endif

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>

#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>

#include <dev/ic/rtl80x9reg.h>
#include <dev/ic/rtl80x9var.h>

struct ne_pci_softc {
	struct ne2000_softc sc_ne2000;		/* real "ne2000" softc */

	/* PCI-specific goo */
	void *sc_ih;				/* interrupt handle */
};

static int	ne_pci_match(struct device *, struct cfdata *, void *);
static void	ne_pci_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ne_pci, sizeof(struct ne_pci_softc),
    ne_pci_match, ne_pci_attach, NULL, NULL);

#ifdef IPKDB_NE_PCI
static struct ne_pci_softc ipkdb_softc;
static pci_chipset_tag_t ipkdb_pc;
static pcitag_t ipkdb_tag;
static struct ipkdb_if *ne_kip;

int ne_pci_ipkdb_attach(struct ipkdb_if *, bus_space_tag_t,       /* XXX */
			pci_chipset_tag_t, int, int);

static int ne_pci_isipkdb(pci_chipset_tag_t, pcitag_t);
#endif

static const struct ne_pci_product {
	pci_vendor_id_t npp_vendor;
	pci_product_id_t npp_product;
	int (*npp_mediachange)(struct dp8390_softc *);
	void (*npp_mediastatus)(struct dp8390_softc *, struct ifmediareq *);
	void (*npp_init_card)(struct dp8390_softc *);
	void (*npp_media_init)(struct dp8390_softc *);
	const char *npp_name;
} ne_pci_products[] = {
	{ PCI_VENDOR_REALTEK,		PCI_PRODUCT_REALTEK_RT8029,
	  rtl80x9_mediachange,		rtl80x9_mediastatus,
	  rtl80x9_init_card,		rtl80x9_media_init,
	  "Realtek 8029" },

	{ PCI_VENDOR_WINBOND,		PCI_PRODUCT_WINBOND_W89C940F,
	  NULL,				NULL,
	  NULL,				NULL,
	  "Winbond 89C940F" },

	{ PCI_VENDOR_WINBOND,		PCI_PRODUCT_WINBOND_W89C940F_1,
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

static const struct ne_pci_product *
ne_pci_lookup(const struct pci_attach_args *pa)
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

static int
ne_pci_match(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	struct pci_attach_args *pa = aux;

	if (ne_pci_lookup(pa) != NULL)
		return (1);

	return (0);
}

static void
ne_pci_attach(struct device *parent __unused, struct device *self, void *aux)
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

	npp = ne_pci_lookup(pa);
	if (npp == NULL) {
		printf("\n");
		panic("ne_pci_attach: impossible");
	}

	printf(": %s Ethernet\n", npp->npp_name);

#ifdef IPKDB_NE_PCI
	if (ne_pci_isipkdb(pc, pa->pa_tag)) {
		nict = ipkdb_softc.sc_ne2000.sc_dp8390.sc_regt;
		nich = ipkdb_softc.sc_ne2000.sc_dp8390.sc_regh;
		ne_kip->port = nsc;
	} else
#endif
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

	dsc->sc_mediachange = npp->npp_mediachange;
	dsc->sc_mediastatus = npp->npp_mediastatus;
	dsc->sc_media_init = npp->npp_media_init;
	dsc->init_card = npp->npp_init_card;

	/*
	 * Do generic NE2000 attach.  This will read the station address
	 * from the EEPROM.
	 */
	ne2000_attach(nsc, NULL);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
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

#ifdef IPKDB_NE_PCI
static int
ne_pci_isipkdb(pci_chipset_tag_t pc, pcitag_t tag)
{
	return !memcmp(&pc, &ipkdb_pc, sizeof pc)
		&& !memcmp(&tag, &ipkdb_tag, sizeof tag);
}

int
ne_pci_ipkdb_attach(struct ipkdb_if *kip, bus_space_tag_t iot,
    pci_chipset_tag_t pc, int bus, int dev)
{
	struct pci_attach_args pa;
	bus_space_tag_t nict, asict;
	bus_space_handle_t nich, asich;
	u_int32_t csr;

	pa.pa_iot = iot;
	pa.pa_pc = pc;
	pa.pa_device = dev;
	pa.pa_function = 0;
	pa.pa_flags = PCI_FLAGS_IO_ENABLED;
	pa.pa_tag = pci_make_tag(pc, bus, dev, /*func*/0);
	pa.pa_id = pci_conf_read(pc, pa.pa_tag, PCI_ID_REG);
	pa.pa_class = pci_conf_read(pc, pa.pa_tag, PCI_CLASS_REG);
	if (ne_pci_lookup(&pa) == NULL)
		return -1;

	if (pci_mapreg_map(&pa, PCI_CBIO, PCI_MAPREG_TYPE_IO, 0,
			&nict, &nich, NULL, NULL))
		return -1;

	asict = nict;
	if (bus_space_subregion(nict, nich, NE2000_ASIC_OFFSET,
				NE2000_ASIC_NPORTS, &asich)) {
		bus_space_unmap(nict, nich, NE2000_NPORTS);
		return -1;
	}

	/* Enable card */
	csr = pci_conf_read(pc, pa.pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pc, pa.pa_tag, PCI_COMMAND_STATUS_REG,
			csr | PCI_COMMAND_MASTER_ENABLE);

	ipkdb_softc.sc_ne2000.sc_dp8390.sc_regt = nict;
	ipkdb_softc.sc_ne2000.sc_dp8390.sc_regh = nich;
	ipkdb_softc.sc_ne2000.sc_asict = asict;
	ipkdb_softc.sc_ne2000.sc_asich = asich;

	kip->port = &ipkdb_softc;
	ipkdb_pc = pc;
	ipkdb_tag = pa.pa_tag;
	ne_kip = kip;

	if (ne2000_ipkdb_attach(kip) < 0) {
		bus_space_unmap(nict, nich, NE2000_NPORTS);
		return -1;
	}

	return 0;
}
#endif /* IPKDB_NE_PCI */
