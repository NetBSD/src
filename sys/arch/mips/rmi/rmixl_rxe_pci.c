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

__KERNEL_RCSID(1, "$NetBSD: rmixl_rxe_pci.c,v 1.1.2.1 2012/01/19 17:34:18 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include "locators.h"

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#include "locators.h"

#ifdef DEBUG
int xlrxe_debug = 0;
#define	DPRINTF(x, ...)	do { if (xlrxe_debug) printf(x, ## __VA_ARGS__); } while (0)
#else
#define	DPRINTF(x)
#endif

static	int xlrxe_pci_match(device_t, cfdata_t, void *);
static	void xlrxe_pci_attach(device_t, device_t, void *);
static	int xlrxe_intr(void *);

struct	xlrxe_softc {
	device_t sc_dev;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_tag;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
};

CFATTACH_DECL_NEW(xlrxe_pci, sizeof(struct xlrxe_softc),
    xlrxe_pci_match, xlrxe_pci_attach, NULL, NULL);

static int
xlrxe_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args * const pa = aux;

	if (pa->pa_id == PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_RXE))
		return 1;

        return 0;
}

static void
xlrxe_pci_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct pci_attach_args * const pa = aux;
	struct xlrxe_softc * const sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_bst = &rcp->rc_pci_ecfg_eb_memt;
	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;

	/*
	 * Why isn't this accessible via a BAR?
	 */
	if (bus_space_subregion(sc->sc_bst, rcp->rc_pci_ecfg_eb_memh,
		    pa->pa_tag, 0, &sc->sc_bsh)) {
		aprint_error(": can't map registers\n");
		return;
	}

	aprint_naive(": XLP RXE Controller\n");
	aprint_normal(": XLP Regular Expression Engine\n");

	const pcireg_t statinfo = pci_conf_read(sc->sc_pc, sc->sc_tag,
	    PCI_RMIXLP_STATID);

	const size_t stid_start = PCI_RMIXLP_STATID_BASE(statinfo);
	const size_t stid_count = PCI_RMIXLP_STATID_COUNT(statinfo);
	if (stid_count) {
		aprint_normal_dev(sc->sc_dev, "%zu station%s starting at %zu\n",
		    stid_count, (stid_count == 1 ? "" : "s"), stid_start);
	}

	pci_intr_handle_t pcih;
	pci_intr_map(pa, &pcih);

	if (pci_intr_establish(pa->pa_pc, pcih, IPL_VM, xlrxe_intr, sc) == NULL) {
		aprint_error_dev(self, "failed to establish interrupt\n");
	} else {
		const char * const intrstr = pci_intr_string(pa->pa_pc, pcih);
		aprint_normal_dev(self, "interrupting at %s\n", intrstr);
	}
}

static int
xlrxe_intr(void *v)
{
	struct xlrxe_softc * const sc = v;

	panic("%s(%p)", device_xname(sc->sc_dev), v);
}
