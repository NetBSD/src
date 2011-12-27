/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__KERNEL_RCSID(1, "$NetBSD: rmixl_nand_pci.c,v 1.1.2.1 2011/12/27 19:58:19 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include "locators.h"

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/nand/nand.h>
#include <dev/nand/onfi.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#include "locators.h"

static	int xlnand_pci_match(device_t, cfdata_t, void *);
static	void xlnand_pci_attach(device_t, device_t, void *);

struct	xlnand_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
};

CFATTACH_DECL_NEW(xlnand_pci, sizeof(struct xlnand_softc),
    xlnand_pci_match, xlnand_pci_attach, NULL, NULL);

static int
xlnand_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args * const pa = aux;

	if (pa->pa_id == PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_NAND))
		return 1;

        return 0;
}

static void
xlnand_pci_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct pci_attach_args * const pa = aux;
	struct xlnand_softc * const sc = device_private(self);
	// struct norbus_attach_args iba;

	sc->sc_dev = self;
	sc->sc_bst = &rcp->rc_pci_ecfg_eb_memt;

	/*
	 * Why isn't this accessible via a BAR?
	 */
	if (bus_space_subregion(sc->sc_bst, rcp->rc_pci_ecfg_eb_memh,
		    pa->pa_tag | 0x100, 0, &sc->sc_bsh)) {
		aprint_error(": can't map registers\n");
		return;
	}

	aprint_normal(": XLP NAND Controller\n");
}
