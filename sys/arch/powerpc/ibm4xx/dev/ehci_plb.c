/*	$NetBSD: ehci_plb.c,v 1.1 2026/06/14 00:02:35 rkujawa Exp $	*/

/*
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * EHCI behind the PLB-AHB bridge.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ehci_plb.c,v 1.1 2026/06/14 00:02:35 rkujawa Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/extent.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dev/plbvar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#define	EHCI_PLB_SIZE	0x400

#include "locators.h"

static int	ehci_plb_match(device_t, cfdata_t, void *);
static void	ehci_plb_attach(device_t, device_t, void *);
static void	ehci_plb_deferred(device_t);

CFATTACH_DECL_NEW(ehci_plb, sizeof(struct ehci_softc),
    ehci_plb_match, ehci_plb_attach, NULL, NULL);

static struct powerpc_bus_space ehci_plb_tag = {
	_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE,
	0x00000000,
};
static char ehci_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));

static int
ehci_plb_match(device_t parent, cfdata_t match, void *aux)
{
	struct plb_attach_args *paa = aux;

	if (strcmp(paa->plb_name, match->cf_name) != 0)
		return 0;

	if (match->cf_loc[PLBCF_ADDR] == PLBCF_ADDR_DEFAULT)
		panic("ehci_plb_match: wildcard addr not allowed");
	if (match->cf_loc[PLBCF_IRQ] == PLBCF_IRQ_DEFAULT)
		panic("ehci_plb_match: wildcard IRQ not allowed");

	paa->plb_addr = match->cf_loc[PLBCF_ADDR];
	paa->plb_irq = match->cf_loc[PLBCF_IRQ];
	return 1;
}

static void
ehci_plb_attach(device_t parent, device_t self, void *aux)
{
	struct ehci_softc *sc = device_private(self);
	struct plb_attach_args *paa = aux;

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_dmatag = paa->plb_dmat;
	sc->sc_bus.ub_revision = USBREV_2_0;

	ehci_plb_tag.pbs_base = paa->plb_addr;
	ehci_plb_tag.pbs_limit = paa->plb_addr + EHCI_PLB_SIZE;
	sc->iot = &ehci_plb_tag;

	if (bus_space_init(&ehci_plb_tag, "ehciplb", ehci_ex_storage,
	      sizeof(ehci_ex_storage)) ||
	    bus_space_map(sc->iot, paa->plb_addr, EHCI_PLB_SIZE, 0,
	      &sc->ioh)) {
		aprint_error(": can't map registers\n");
		return;
	}
	sc->sc_size = EHCI_PLB_SIZE;

	aprint_normal(": EHCI USB controller\n");

	sc->sc_offs = bus_space_read_1(sc->iot, sc->ioh, EHCI_CAPLENGTH);

	/* Disable interrupts, so we don't get any spurious ones. */
	bus_space_write_4(sc->iot, sc->ioh, sc->sc_offs + EHCI_USBINTR, 0);

	intr_establish_xname(paa->plb_irq, IST_LEVEL, IPL_USB, ehci_intr,
	    sc, device_xname(self));

	/* Wait until the companion OHCI has attached. */
	config_defer(self, ehci_plb_deferred);
}

static void
ehci_plb_deferred(device_t self)
{
	struct ehci_softc *sc = device_private(self);
	device_t comp;
	int error;

	comp = device_find_by_xname("ohci0");
	if (comp != NULL) {
		sc->sc_ncomp = 1;
		sc->sc_comps[0] = comp;
	}

	error = ehci_init(sc);
	if (error) {
		aprint_error_dev(self, "init failed, error=%d\n", error);
		return;
	}

	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint,
	    CFARGS_NONE);
}
