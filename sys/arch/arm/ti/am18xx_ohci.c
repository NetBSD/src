/* $NetBSD $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yuri Honegger.
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

/*
 * Driver for the USB1.1 OHCI port on the TI AM18XX family of SoCs.
 */

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/device.h>

#include <dev/clk/clk.h>
#include <dev/fdt/fdtvar.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

struct am18xx_ohci_softc {
	struct ohci_softc ohci_sc;
	struct clk *sc_clk;
	struct fdtbus_phy *sc_phy;
};

static int am18xx_ohci_match(device_t, cfdata_t, void *);
static void am18xx_ohci_attach(device_t, device_t, void *);

static int am18xx_ohci_enable_clocks(struct am18xx_ohci_softc *);

CFATTACH_DECL_NEW(am18xxohci, sizeof(struct am18xx_ohci_softc),
    am18xx_ohci_match, am18xx_ohci_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,da830-ohci" },
	DEVICE_COMPAT_EOL
};

static int
am18xx_ohci_enable_clocks(struct am18xx_ohci_softc *sc)
{
	int error;

	error = clk_enable(sc->sc_clk);
	if (error != 0) {
		return error;
	}

	error = fdtbus_phy_enable(sc->sc_phy, true);
	if (error != 0) {
		return error;
	}

	return 0;
}

int
am18xx_ohci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args *const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
am18xx_ohci_attach(device_t parent, device_t self, void *aux)
{
	struct am18xx_ohci_softc *const sc = device_private(self);
	struct fdt_attach_args *const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	char intrstr[128];
	int error;

	sc->ohci_sc.iot = faa->faa_bst;
	sc->ohci_sc.sc_dev = self;
	sc->ohci_sc.sc_bus.ub_hcpriv = &sc->ohci_sc;
	sc->ohci_sc.sc_bus.ub_dmatag = faa->faa_dmat;
	sc->ohci_sc.sc_flags = 0;

	/* get USB 1.1 clock */
	sc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get usb clock\n");
		return;
	}

	/* get phy */
	sc->sc_phy = fdtbus_phy_get(phandle, "usb-phy");
	if (sc->sc_phy == NULL) {
		aprint_error(": couldn't get usb phy\n");
		return;
	}

	/* enable all clocks */
	if (am18xx_ohci_enable_clocks(sc)) {
		aprint_error(": couldn't enable clocks\n");
		return;
	}

	/* map bus space */
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->ohci_sc.iot, addr, size, 0, &sc->ohci_sc.ioh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->ohci_sc.sc_size = size;

	/* disable interrupts on the ohci side */
	bus_space_write_4(sc->ohci_sc.iot, sc->ohci_sc.ioh,
	    OHCI_INTERRUPT_DISABLE, OHCI_ALL_INTRS);

	/* establish interrupt */
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}
	void *ih = fdtbus_intr_establish_xname(phandle, 0, IPL_USB,
	    FDT_INTR_MPSAFE, ohci_intr, &sc->ohci_sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, ": couldn't install interrupt %s\n",
		    intrstr);
		return;
	}

	aprint_normal(": ohci on %s\n", intrstr);

	/* initialize the controller */
	error = ohci_init(&sc->ohci_sc);
	if (error) {
		aprint_error_dev(self, ": init failed, error=%d\n", error);
		return;
	}

	sc->ohci_sc.sc_child = config_found(self, &sc->ohci_sc.sc_bus,
	    usbctlprint, CFARGS_NONE);
}
