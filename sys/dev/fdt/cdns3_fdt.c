/* $NetBSD: cdns3_fdt.c,v 1.1 2024/01/18 07:48:57 skrll Exp $ */

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: cdns3_fdt.c,v 1.1 2024/01/18 07:48:57 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>

#include <dev/usb/xhcireg.h>
#include <dev/usb/xhcivar.h>

#include <dev/fdt/fdtvar.h>

struct cdns3_fdt_softc {
	struct xhci_softc	 sc_xhci;
	bus_space_tag_t		 sc_otg_bst;
	bus_space_handle_t	 sc_otg_bsh;
	void			*sc_ih;
};

#define	OTGRD4(sc, reg)							\
	bus_space_read_4((sc)->sc_otg_bst, (sc)->sc_otg_bsh, (reg))
#define	OTGWR4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_otg_bst, (sc)->sc_otg_bsh, (reg), (val))


/*
 * Cadence USB3 controller.
 */

#define	OTG_DID                 0x0000
#define	 OTG_DID_V1             0x4024e
#define	OTG_CMD                 0x10
#define	 OTG_CMD_HOST_BUS_REQ   __BIT(1)
#define	 OTG_CMD_OTG_DIS        __BIT(3)
#define	OTG_STS                 0x14
#define	 OTG_STS_XHCI_READY     __BIT(26)


static const struct device_compatible_entry compat_data[] = {
	{ .compat = "cdns,usb3" },
	DEVICE_COMPAT_EOL
};

static int
cdns3_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
cdns3_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct cdns3_fdt_softc * const cfsc = device_private(self);
	struct xhci_softc * const sc = &cfsc->sc_xhci;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	struct fdtbus_phy *phy;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	int error;

	/*
	 * Only host mode is supported, but this includes otg devices
	 * that have 'usb-role-switch' and 'role-switch-default-mode' of
	 * 'host'
	 */
	const char *dr_mode = fdtbus_get_string(phandle, "dr_mode");
	if (dr_mode == NULL || strcmp(dr_mode, "otg") == 0) {
		bool ok = false;
		if (of_hasprop(phandle, "usb-role-switch")) {
			const char *rsdm = fdtbus_get_string(phandle,
			    "role-switch-default-mode");
			if (rsdm != NULL && strcmp(rsdm, "host") == 0)
				ok = true;

			if (!ok) {
				aprint_error(": host is not default mode\n");
				return;
			}
		}
		if (!ok) {
			aprint_error(": cannot switch 'otg' mode to host\n");
			return;
		}
	} else if (strcmp(dr_mode, "host") != 0) {
		aprint_error(": '%s' not supported\n", dr_mode);
		return;
	}

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_dmatag = faa->faa_dmat;
	sc->sc_iot = faa->faa_bst;

	bus_space_handle_t bsh;
	if (fdtbus_get_reg_byname(phandle, "otg", &addr, &size) != 0 ||
	    bus_space_map(faa->faa_bst, addr, size, 0, &bsh) != 0) {
		aprint_error(": couldn't map otg registers\n");
		return;
	}
	cfsc->sc_otg_bst = faa->faa_bst;
	cfsc->sc_otg_bsh = bsh;

	if (fdtbus_get_reg_byname(phandle, "xhci", &addr, &size) != 0 ||
	    bus_space_map(faa->faa_bst, addr, size, 0, &bsh) != 0) {
		aprint_error(": couldn't map xhci registers\n");
		return;
	}

	sc->sc_ios = size;
	sc->sc_ioh = bsh;

	uint32_t did = OTGRD4(cfsc, OTG_DID);
	if (did != OTG_DID_V1) {
		aprint_error(": unsupported IP (%#x)\n", did);
		return;
	}
	OTGWR4(cfsc, OTG_CMD, OTG_CMD_HOST_BUS_REQ | OTG_CMD_OTG_DIS);
	int tries;
	uint32_t sts;
	for (tries = 100; tries > 0; tries--) {
		sts = OTGRD4(cfsc, OTG_STS);
		if (sts & OTG_STS_XHCI_READY)
			break;
		delay(1000);
	}
	if (tries == 0) {
		aprint_error(": not ready (%#x)\n", sts);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Cadence USB3 XHCI\n");

	/* Enable PHY devices */
	for (u_int i = 0; ; i++) {
		phy = fdtbus_phy_get_index(phandle, i);
		if (phy == NULL)
			break;
		if (fdtbus_phy_enable(phy, true) != 0)
			aprint_error_dev(self, "couldn't enable phy #%d\n", i);
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	void *ih = fdtbus_intr_establish_xname(phandle, 0, IPL_USB,
	    FDT_INTR_MPSAFE, xhci_intr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_bus.ub_revision = USBREV_3_0;
	error = xhci_init(sc);
	if (error) {
		aprint_error_dev(self, "init failed, error = %d\n", error);
		return;
	}

	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint, CFARGS_NONE);
	sc->sc_child2 = config_found(self, &sc->sc_bus2, usbctlprint,
	    CFARGS_NONE);
}


CFATTACH_DECL2_NEW(cdns3_fdt, sizeof(struct cdns3_fdt_softc),
	cdns3_fdt_match, cdns3_fdt_attach, NULL,
	xhci_activate, NULL, xhci_childdet);
