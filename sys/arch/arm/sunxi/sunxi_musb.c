/* $NetBSD: sunxi_musb.c,v 1.1 2017/09/09 12:01:04 jmcneill Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: sunxi_musb.c,v 1.1 2017/09/09 12:01:04 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/pool.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/motgvar.h>

#include <dev/fdt/fdtvar.h>

#define	MUSB2_REG_AWIN_VEND0	0x43
#define	MUSB2_REG_INTTX		0x44
#define	MUSB2_REG_INTRX		0x46
#define	MUSB2_REG_INTUSB	0x4c

static int	sunxi_musb_match(device_t, cfdata_t, void *);
static void	sunxi_musb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sunxi_musb, sizeof(struct motg_softc),
	sunxi_musb_match, sunxi_musb_attach, NULL, NULL);

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun4i-a10-musb",		5 },
	{ "allwinner,sun6i-a13-musb",		5 },
	{ "allwinner,sun8i-h3-musb",		4 },
	{ "allwinner,sun8i-a33-musb",		5 },
	{ NULL }
};

static int
sunxi_musb_intr(void *priv)
{
	struct motg_softc * const sc = priv;
	uint16_t inttx, intrx;
	uint8_t intusb;

	mutex_enter(&sc->sc_intr_lock);

	intusb = bus_space_read_1(sc->sc_iot, sc->sc_ioh, MUSB2_REG_INTUSB);
	inttx = bus_space_read_2(sc->sc_iot, sc->sc_ioh, MUSB2_REG_INTTX);
	intrx = bus_space_read_2(sc->sc_iot, sc->sc_ioh, MUSB2_REG_INTRX);
	if (!intusb && !inttx && !intrx) {
		mutex_exit(&sc->sc_intr_lock);
		return 0;
	}

	if (intusb)
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, MUSB2_REG_INTUSB, intusb);
	if (inttx)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, MUSB2_REG_INTTX, inttx);
	if (intrx)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, MUSB2_REG_INTRX, intrx);

	motg_intr(sc, intrx, inttx, intusb);

	mutex_exit(&sc->sc_intr_lock);

	return 1;
}

static void
sunxi_musb_poll(void *priv)
{
	sunxi_musb_intr(priv);
}

static int
sunxi_musb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_musb_attach(device_t parent, device_t self, void *aux)
{
	struct motg_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;
	struct fdtbus_phy *phy;
	struct clk *clk;
	char intrstr[128];
	const char *dr_mode;
	bus_addr_t addr;
	bus_size_t size;
	void *ih;
	u_int n;

	/* Only "host" mode is supported */
	dr_mode = fdtbus_get_string(phandle, "dr_mode");
	if (dr_mode == NULL || strcmp(dr_mode, "host") != 0) {
		aprint_normal(": '%s' mode not supported\n", dr_mode);
		return;
	}

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
	strlcpy(sc->sc_vendor, "Allwinner", sizeof(sc->sc_vendor));
	sc->sc_size = size;
	sc->sc_iot = faa->faa_bst;
	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_intr_poll = sunxi_musb_poll;
	sc->sc_intr_poll_arg = sc;
	sc->sc_mode = MOTG_MODE_HOST;
	sc->sc_ep_max = of_search_compatible(phandle, compat_data)->data;
	sc->sc_ep_fifosize = 512;

	aprint_naive("\n");
	aprint_normal(": USB OTG\n");

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	ih = fdtbus_intr_establish(phandle, 0, IPL_USB, FDT_INTR_MPSAFE,
	    sunxi_musb_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, MUSB2_REG_AWIN_VEND0, 0);

	motg_init(sc);
}
