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

__KERNEL_RCSID(1, "$NetBSD: rmixl_srio_pci.c,v 1.1.2.1 2012/01/19 17:34:18 matt Exp $");

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
int xlsrio_debug = 0;
#define	DPRINTF(x, ...)	do { if (xlsrio_debug) printf(x, ## __VA_ARGS__); } while (0)
#else
#define	DPRINTF(x)
#endif

static	int xlsrio_pci_match(device_t, cfdata_t, void *);
static	void xlsrio_pci_attach(device_t, device_t, void *);
static	int xlsrio_intr(void *);

struct	xlsrio_softc {
	device_t sc_dev;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_tag;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
};

CFATTACH_DECL_NEW(xlsrio_pci, sizeof(struct xlsrio_softc),
    xlsrio_pci_match, xlsrio_pci_attach, NULL, NULL);

static int
xlsrio_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args * const pa = aux;

	if (pa->pa_id == PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_SRIO))
		return 1;

        return 0;
}

static void
xlsrio_pci_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct pci_attach_args * const pa = aux;
	struct xlsrio_softc * const sc = device_private(self);
	bus_addr_t base;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_bst = &rcp->rc_pci_ecfg_eb_memt;
	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;

	/*
	 * Map the BAR so we can get access to the registers.
	 */
	if (pci_mapreg_map(pa, PCI_BAR0, PCI_MAPREG_TYPE_MEM, 0,
		    &sc->sc_bst, &sc->sc_bsh, &base, &size) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	aprint_naive(": XLP SRIO Controller\n");
	aprint_normal(": XLP Serial Rapid I/O Engine\n");

	aprint_normal_dev(sc->sc_dev, "BAR[0] (%zuKB @ %#"
	    PRIxBUSADDR") mapped at %#"PRIxBSH"\n",
	    (size_t)size >> 10, base, sc->sc_bsh);

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

	if (pci_intr_establish(pa->pa_pc, pcih, IPL_VM, xlsrio_intr, sc) == NULL) {
		aprint_error_dev(self, "failed to establish interrupt\n");
	} else {
		const char * const intrstr = pci_intr_string(pa->pa_pc, pcih);
		aprint_normal_dev(self, "interrupting at %s\n", intrstr);
	}
}

static int
xlsrio_intr(void *v)
{
	struct xlsrio_softc * const sc = v;

	panic("%s(%p)", device_xname(sc->sc_dev), v);
}
