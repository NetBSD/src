/* $NetBSD: ehci_fdt.c,v 1.2.8.2 2017/12/03 11:37:01 jdolecek Exp $ */

/*-
 * Copyright (c) 2015-2017 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ehci_fdt.c,v 1.2.8.2 2017/12/03 11:37:01 jdolecek Exp $");

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

#include <dev/fdt/fdtvar.h>

static int	ehci_fdt_match(device_t, cfdata_t, void *);
static void	ehci_fdt_attach(device_t, device_t, void *);

CFATTACH_DECL2_NEW(ehci_fdt, sizeof(struct ehci_softc),
	ehci_fdt_match, ehci_fdt_attach, NULL,
	ehci_activate, NULL, ehci_childdet);

static int
ehci_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"generic-ehci",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
ehci_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct ehci_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;
	struct fdtbus_phy *phy;
	struct clk *clk;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	int error;
	void *ih;
	u_int n;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	/* Enable clocks */
	for (n = 0; (clk = fdtbus_clock_get_index(phandle, n)) != NULL; n++)
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable clock #%d\n", n);
			return;
		}
	/* De-assert resets */
	for (n = 0; (rst = fdtbus_reset_get_index(phandle, n)) != NULL; n++)
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset #%d\n", n);
			return;
		}

	/* Enable optional phy */
	phy = fdtbus_phy_get(phandle, "usb");
	if (phy && fdtbus_phy_enable(phy, true) != 0) {
		aprint_error(": couldn't enable phy\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_dmatag = faa->faa_dmat;
	sc->sc_bus.ub_revision = USBREV_2_0;
	if (of_hasprop(phandle, "has-transaction-translator"))
		sc->sc_flags |= EHCIF_ETTF;
	else
		sc->sc_ncomp = 1;
	sc->sc_id_vendor = 0;
	strlcpy(sc->sc_vendor, "Generic", sizeof(sc->sc_vendor));
	sc->sc_size = size;
	sc->iot = faa->faa_bst;
	if (bus_space_map(sc->iot, addr, size, 0, &sc->ioh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": EHCI\n");

	/* Disable interrupts */
	sc->sc_offs = EREAD1(sc, EHCI_CAPLENGTH);
	EOWRITE4(sc, EHCI_USBINTR, 0);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	ih = fdtbus_intr_establish(phandle, 0, IPL_USB, FDT_INTR_MPSAFE,
	    ehci_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	error = ehci_init(sc);
	if (error) {
		aprint_error_dev(self, "init failed, error = %d\n", error);
		return;
	}

	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
}
