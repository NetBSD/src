/* $NetBSD: dwc3_fdt.c,v 1.1.2.2 2018/05/02 07:20:06 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: dwc3_fdt.c,v 1.1.2.2 2018/05/02 07:20:06 pgoyette Exp $");

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
#include <dev/usb/xhcireg.h>
#include <dev/usb/xhcivar.h>

#include <dev/fdt/fdtvar.h>

#define	DWC3_GCTL			0xc110
#define	 GCTL_PRTCAP			__BITS(13,12)
#define	  GCTL_PRTCAP_HOST		1
#define	  GCTL_PRTCAP_DEVICE		2
#define	  GCTL_PRTCAP_OTG		3
#define	 GCTL_CORESOFTRESET		__BIT(11)

#define	DWC3_GUSB2PHYCFG(n)		(0xc200 + ((n) * 4))
#define	 GUSB2PHYCFG_PHYSOFTRST		__BIT(31)

#define	DWC3_GUSB3PIPECTL(n)		(0xc2c0 + ((n) * 4))
#define	 GUSB3PIPECTL_PHYSOFTRST	__BIT(31)

static int	dwc3_fdt_match(device_t, cfdata_t, void *);
static void	dwc3_fdt_attach(device_t, device_t, void *);

CFATTACH_DECL2_NEW(dwc3_fdt, sizeof(struct xhci_softc),
	dwc3_fdt_match, dwc3_fdt_attach, NULL,
	xhci_activate, NULL, xhci_childdet);

#define	RD4(sc, reg)				\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define	WR4(sc, reg, val)			\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define	SET4(sc, reg, mask)			\
	WR4((sc), (reg), RD4((sc), (reg)) | (mask))
#define	CLR4(sc, reg, mask)			\
	WR4((sc), (reg), RD4((sc), (reg)) & ~(mask))

static void
dwc3_fdt_soft_reset(struct xhci_softc *sc)
{
	/* Put core in reset */
	SET4(sc, DWC3_GCTL, GCTL_CORESOFTRESET);

	/* Assert USB3 PHY reset */
	SET4(sc, DWC3_GUSB3PIPECTL(0), GUSB3PIPECTL_PHYSOFTRST);

	/* Assert USB2 PHY reset */
	SET4(sc, DWC3_GUSB2PHYCFG(0), GUSB2PHYCFG_PHYSOFTRST);

	delay(100000);

	/* Clear USB3 PHY reset */
	CLR4(sc, DWC3_GUSB3PIPECTL(0), GUSB3PIPECTL_PHYSOFTRST);

	/* Clear USB2 PHY reset */
	CLR4(sc, DWC3_GUSB2PHYCFG(0), GUSB2PHYCFG_PHYSOFTRST);

	delay(100000);

	/* Take core out of reset */
	CLR4(sc, DWC3_GCTL, GCTL_CORESOFTRESET);
}

static void
dwc3_fdt_set_mode(struct xhci_softc *sc, u_int mode)
{
	uint32_t val;

	val = RD4(sc, DWC3_GCTL);
	val &= ~GCTL_PRTCAP;
	val |= __SHIFTIN(mode, GCTL_PRTCAP);
	WR4(sc, DWC3_GCTL, val);
}

static int
dwc3_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"allwinner,sun50i-h6-dwc3",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
dwc3_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct xhci_softc * const sc = device_private(self);
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

	/* Find dwc3 sub-node */
	const int dwc3_phandle = of_find_firstchild_byname(phandle, "dwc3");
	if (dwc3_phandle <= 0) {
		aprint_error(": couldn't find dwc3 child node\n");
		return;
	}

	/* Only host mode is supported */
	const char *dr_mode = fdtbus_get_string(dwc3_phandle, "dr_mode");
	if (dr_mode == NULL || strcmp(dr_mode, "host") != 0) {
		aprint_error(": '%s' not supported\n", dr_mode);
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

	/* Enable phy */
	phy = fdtbus_phy_get(dwc3_phandle, "usb3-phy");
	if (!phy || fdtbus_phy_enable(phy, true) != 0) {
		aprint_error(": couldn't enable phy\n");
		return;
	}

	/* Get resources */
	if (fdtbus_get_reg(dwc3_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_dmatag = faa->faa_dmat;
	sc->sc_iot = faa->faa_bst;
	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": DesignWare USB3 XHCI\n");

	dwc3_fdt_soft_reset(sc);
	dwc3_fdt_set_mode(sc, GCTL_PRTCAP_HOST);

	if (!fdtbus_intr_str(dwc3_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	ih = fdtbus_intr_establish(dwc3_phandle, 0, IPL_USB, FDT_INTR_MPSAFE,
	    xhci_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	error = xhci_init(sc);
	if (error) {
		aprint_error_dev(self, "init failed, error = %d\n", error);
		return;
	}

	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
	sc->sc_child2 = config_found(self, &sc->sc_bus2, usbctlprint);
}
