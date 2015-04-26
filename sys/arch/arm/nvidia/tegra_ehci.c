/* $NetBSD: tegra_ehci.c,v 1.2 2015/04/26 17:14:24 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_ehci.c,v 1.2 2015/04/26 17:14:24 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#include <arm/nvidia/tegra_var.h>

#define TEGRA_EHCI_REG_OFFSET	0x100

static int	tegra_ehci_match(device_t, cfdata_t, void *);
static void	tegra_ehci_attach(device_t, device_t, void *);

struct tegra_ehci_softc {
	struct ehci_softc	sc;
	void			*sc_ih;
};

CFATTACH_DECL2_NEW(tegra_ehci, sizeof(struct tegra_ehci_softc),
	tegra_ehci_match, tegra_ehci_attach, NULL,
	ehci_activate, NULL, ehci_childdet);

static int
tegra_ehci_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_ehci_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_ehci_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;
	int error;

	sc->sc.sc_dev = self;
	sc->sc.sc_bus.hci_private = &sc->sc;
	sc->sc.sc_bus.dmatag = tio->tio_dmat;
	sc->sc.sc_bus.usbrev = USBREV_2_0;
	sc->sc.sc_flags = EHCIF_ETTF;
	sc->sc.sc_id_vendor = 0x10de;
	strlcpy(sc->sc.sc_vendor, "Tegra", sizeof(sc->sc.sc_vendor));
	sc->sc.sc_size = loc->loc_size;
	sc->sc.iot = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset + TEGRA_EHCI_REG_OFFSET,
	    loc->loc_size - TEGRA_EHCI_REG_OFFSET, &sc->sc.ioh);

	aprint_naive("\n");
	aprint_normal(": USB%d\n", loc->loc_port + 1);

	sc->sc.sc_offs = EREAD1(&sc->sc, EHCI_CAPLENGTH);

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_USB, IST_LEVEL,
	    ehci_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	error = ehci_init(&sc->sc);
	if (error != USBD_NORMAL_COMPLETION) {
		aprint_error_dev(self, "init failed, error = %d\n", error);
		return;
	}

	sc->sc.sc_child = config_found(self, &sc->sc.sc_bus, usbctlprint);
}
