/* $NetBSD: if_eqos_pci.c,v 1.3.2.2 2023/11/03 08:56:36 martin Exp $ */

/*-
 * Copyright (c) 2023 Masanobu SAITOH <msaitoh@netbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * TODO:
 *	Use multi vector MSI to support multiqueue.
 *
 */

#include "opt_net_mpsafe.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_eqos_pci.c,v 1.3.2.2 2023/11/03 08:56:36 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/rndsource.h>

#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/mii/miivar.h>
#include <dev/ic/dwc_eqos_var.h>

#define EQOS_PCI_MAX_INTR	1

static int eqos_pci_match(device_t, cfdata_t, void *);
static void eqos_pci_attach(device_t, device_t, void *);

struct eqos_pci_softc {
	struct eqos_softc	sc_eqos;
	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_tag;
	void			*sc_ihs[EQOS_PCI_MAX_INTR];
	pci_intr_handle_t	*sc_intrs;
	uint16_t		sc_pcidevid;
};

static const struct device_compatible_entry compat_data[] = {
	{ .id = PCI_ID_CODE(PCI_VENDOR_INTEL,
		    PCI_PRODUCT_INTEL_EHL_ETH) },
	{ .id = PCI_ID_CODE(PCI_VENDOR_INTEL,
		    PCI_PRODUCT_INTEL_EHL_PSE_ETH_0_RGMII) },
	{ .id = PCI_ID_CODE(PCI_VENDOR_INTEL,
		    PCI_PRODUCT_INTEL_EHL_PSE_ETH_1_RGMII) },
	{ .id = PCI_ID_CODE(PCI_VENDOR_INTEL,
		    PCI_PRODUCT_INTEL_EHL_PSE_ETH_0_SGMII_1G) },
	{ .id = PCI_ID_CODE(PCI_VENDOR_INTEL,
		    PCI_PRODUCT_INTEL_EHL_PSE_ETH_1_SGMII_1G) },
	{ .id = PCI_ID_CODE(PCI_VENDOR_INTEL,
		    PCI_PRODUCT_INTEL_EHL_PSE_ETH_0_SGMII_2_5G) },
	{ .id = PCI_ID_CODE(PCI_VENDOR_INTEL,
		    PCI_PRODUCT_INTEL_EHL_PSE_ETH_1_SGMII_2_5G) },

	PCI_COMPAT_EOL
};

CFATTACH_DECL3_NEW(eqos_pci, sizeof(struct eqos_pci_softc),
    eqos_pci_match, eqos_pci_attach, NULL, NULL, NULL, NULL,
    0);

static int
eqos_pci_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa =aux;

	return pci_compatible_match(pa, compat_data);
}

static void
eqos_pci_attach(device_t parent, device_t self, void *aux)
{
	struct eqos_pci_softc * const psc = device_private(self);
	struct eqos_softc * const sc = &psc->sc_eqos;
	struct pci_attach_args *pa =aux;
	const pci_chipset_tag_t pc = pa->pa_pc;
	const pcitag_t tag = pa->pa_tag;
	prop_dictionary_t prop;
	bus_space_tag_t memt;
	bus_space_handle_t memh;
	int counts[PCI_INTR_TYPE_SIZE];
	char intrbuf[PCI_INTRSTR_LEN];
	bus_size_t memsize;
	pcireg_t memtype;
	const char *intrstr;
	uint32_t dma_pbl = 0;

	psc->sc_pc = pc;
	psc->sc_tag = tag;
	psc->sc_pcidevid = PCI_PRODUCT(pa->pa_id);

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, PCI_BAR0);
	if (pci_mapreg_map(pa, PCI_BAR0, memtype, 0, &memt, &memh, NULL,
		&memsize) != 0) {
		aprint_error(": can't map mem space\n");
		return;
	}
	sc->sc_dev = self;
	sc->sc_bst = memt;
	sc->sc_bsh = memh;
	prop = device_properties(sc->sc_dev);

	if (pci_dma64_available(pa))
		sc->sc_dmat = pa->pa_dmat64;
	else
		sc->sc_dmat = pa->pa_dmat;

	sc->sc_phy_id = MII_PHY_ANY;
	switch (psc->sc_pcidevid) {
	case PCI_PRODUCT_INTEL_EHL_ETH:
		sc->sc_csr_clock = 204800000;
		dma_pbl = 32;
		break;
	case PCI_PRODUCT_INTEL_EHL_PSE_ETH_0_RGMII:
	case PCI_PRODUCT_INTEL_EHL_PSE_ETH_1_RGMII:
	case PCI_PRODUCT_INTEL_EHL_PSE_ETH_0_SGMII_1G:
	case PCI_PRODUCT_INTEL_EHL_PSE_ETH_1_SGMII_1G:
	case PCI_PRODUCT_INTEL_EHL_PSE_ETH_0_SGMII_2_5G:
	case PCI_PRODUCT_INTEL_EHL_PSE_ETH_1_SGMII_2_5G:
		sc->sc_dmat = pa->pa_dmat; /* 32bit DMA only */
		sc->sc_csr_clock = 200000000;
		dma_pbl = 32;
		break;
#if 0
	case PCI_PRODUCT_INTEL_QUARTK_ETH:
		dma_pbl = 16;
#endif
	default:
		sc->sc_csr_clock = 200000000; /* XXX */
	}

	if (sc->sc_dmat == pa->pa_dmat64)
		aprint_verbose(", 64-bit DMA");
	else
		aprint_verbose(", 32-bit DMA");

	/* Defaults */
	if (dma_pbl != 0) {
		prop = device_properties(sc->sc_dev);
		prop_dictionary_set_uint32(prop, "snps,pbl", dma_pbl);
	}

	if (eqos_attach(sc) != 0) {
		aprint_error_dev(sc->sc_dev, "failed in eqos_attach()\n");
		return;
	}

	/* Allocation settings */
	counts[PCI_INTR_TYPE_MSI] = 1;
	counts[PCI_INTR_TYPE_INTX] = 1;
	if (pci_intr_alloc(pa, &psc->sc_intrs, counts, PCI_INTR_TYPE_MSI) != 0)
	{
		aprint_error_dev(sc->sc_dev, "failed to allocate interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pc, psc->sc_intrs[0], intrbuf,
	    sizeof(intrbuf));
	pci_intr_setattr(pc, &psc->sc_intrs[0], PCI_INTR_MPSAFE, true);
	psc->sc_ihs[0] = pci_intr_establish_xname(pc, psc->sc_intrs[0],
	    IPL_NET, eqos_intr, sc, device_xname(self));

	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	if (pmf_device_register(self, NULL, NULL))
		pmf_class_network_register(self, &sc->sc_ec.ec_if);
	else
		aprint_error_dev(self, "couldn't establish power handler\n");
}
