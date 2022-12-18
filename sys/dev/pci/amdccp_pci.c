/* $NetBSD: amdccp_pci.c,v 1.4 2022/12/18 15:50:32 reinoud Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: amdccp_pci.c,v 1.4 2022/12/18 15:50:32 reinoud Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/amdccpvar.h>

static int	amdccp_pci_match(device_t, cfdata_t, void *);
static void	amdccp_pci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(amdccp_pci, sizeof(struct amdccp_softc),
    amdccp_pci_match, amdccp_pci_attach, NULL, NULL);

#define	AMDCCP_MEM_BAR		PCI_BAR2

static const struct amdccp_pci_product {
	pci_vendor_id_t		vendor;
	pci_product_id_t	product;
} amdccp_pci_products[] = {
	{ .vendor	=	PCI_VENDOR_AMD,
	  .product	=	PCI_PRODUCT_AMD_F16_CCP,
	},
	{ .vendor	=	PCI_VENDOR_AMD,
	  .product	=	PCI_PRODUCT_AMD_F17_CCP_1,
	},
	{ .vendor	=	PCI_VENDOR_AMD,
	  .product	=	PCI_PRODUCT_AMD_F17_CCP_2,
	},
	{ .vendor	=	PCI_VENDOR_AMD,
	  .product	=	PCI_PRODUCT_AMD_F17_1X_PSP,
	},
	{ .vendor	=	PCI_VENDOR_AMD,
	  .product	=	PCI_PRODUCT_AMD_F17_7X_CCP,
	},
	{ .vendor	=	PCI_VENDOR_AMD,
	  .product	=	PCI_PRODUCT_AMD_F17_9X_CCP,
	},
};

static const struct amdccp_pci_product *
amdccp_pci_lookup(const struct pci_attach_args * const pa)
{
	unsigned int i;

	for (i = 0; i < __arraycount(amdccp_pci_products); i++) {
		if (PCI_VENDOR(pa->pa_id)  == amdccp_pci_products[i].vendor &&
		    PCI_PRODUCT(pa->pa_id) == amdccp_pci_products[i].product) {
			return &amdccp_pci_products[i];
		}
	}
	return NULL;
}

static int
amdccp_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct pci_attach_args * const pa = aux;

	return amdccp_pci_lookup(pa) != NULL;
}

static void
amdccp_pci_attach(device_t parent, device_t self, void *aux)
{
	struct amdccp_softc * const sc = device_private(self);
	const struct pci_attach_args * const pa = aux;
	pcireg_t type;

	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal(": AMD Cryptographic Coprocessor\n");

	type = pci_mapreg_type(pa->pa_pc, pa->pa_tag, AMDCCP_MEM_BAR);
	if (PCI_MAPREG_TYPE(type) != PCI_MAPREG_TYPE_MEM) {
		aprint_error_dev(self, "expected MEM register, got IO\n");
		return;
	}

	if (pci_mapreg_map(pa, AMDCCP_MEM_BAR, type, 0,
			   &sc->sc_bst, &sc->sc_bsh, NULL, NULL) != 0) {
		aprint_error_dev(self, "unable to map device registers\n");
		return;
	}

	amdccp_common_attach(sc);
	pmf_device_register(self, NULL, NULL);
}
