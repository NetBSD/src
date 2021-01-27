/* $NetBSD: mesongxl_usb2phy.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(0, "$NetBSD: mesongxl_usb2phy.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/fdt/fdtvar.h>

#define	USB2PHY_REG0			0x00
#define	 REG0_POWER_ON_RESET		__BIT(22)
#define	 REG0_ID_PULLUP			__BIT(13)
#define	 REG0_DP_PULLDOWN		__BIT(6)
#define	 REG0_DM_PULLDOWN		__BIT(5)

static int mesongxl_usb2phy_match(device_t, cfdata_t, void *);
static void mesongxl_usb2phy_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson-gxl-usb2-phy" },
	DEVICE_COMPAT_EOL
};

struct mesongxl_usb2phy_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
	struct clk		*sc_clk;
	struct fdtbus_reset	*sc_rst;
	struct fdtbus_regulator	*sc_supply;
};

#define	PHY_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PHY_WRITE(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(mesongxl_usb2phy, sizeof(struct mesongxl_usb2phy_softc),
	mesongxl_usb2phy_match, mesongxl_usb2phy_attach, NULL, NULL);

static void *
mesongxl_usb2phy_acquire(device_t dev, const void *data, size_t len)
{
	if (len != 0)
		return NULL;

	return (void *)(uintptr_t)1;
}

static void
mesongxl_usb2phy_release(device_t dev, void *priv)
{
}

static int
mesongxl_usb2phy_enable(device_t dev, void *priv, bool enable)
{
	struct mesongxl_usb2phy_softc * const sc = device_private(dev);
	uint32_t val;

	if (enable) {
		/* Power on PHY */
		val = PHY_READ(sc, USB2PHY_REG0);
		val &= ~REG0_POWER_ON_RESET;
		PHY_WRITE(sc, USB2PHY_REG0, val);

		/* Configure PHY for host mode */
		val = PHY_READ(sc, USB2PHY_REG0);
		val |= REG0_DM_PULLDOWN;
		val |= REG0_DP_PULLDOWN;
		val &= ~REG0_ID_PULLUP;
		PHY_WRITE(sc, USB2PHY_REG0, val);

		/* Reset the PHY */
		val = PHY_READ(sc, USB2PHY_REG0);
		val |= REG0_POWER_ON_RESET;
		PHY_WRITE(sc, USB2PHY_REG0, val);
		delay(500);
		val = PHY_READ(sc, USB2PHY_REG0);
		val &= ~REG0_POWER_ON_RESET;
		PHY_WRITE(sc, USB2PHY_REG0, val);
		delay(500);

		if (sc->sc_supply != NULL) {
			if (fdtbus_regulator_enable(sc->sc_supply) != 0)
				aprint_error(": couldn't enable supply\n");
		}
	} else {
		/* Power off PHY */
		val = PHY_READ(sc, USB2PHY_REG0);
		val |= REG0_POWER_ON_RESET;
		PHY_WRITE(sc, USB2PHY_REG0, val);
	}

	return 0;
}

const struct fdtbus_phy_controller_func mesongxl_usb2phy_funcs = {
	.acquire = mesongxl_usb2phy_acquire,
	.release = mesongxl_usb2phy_release,
	.enable = mesongxl_usb2phy_enable,
};

static int
mesongxl_usb2phy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
mesongxl_usb2phy_attach(device_t parent, device_t self, void *aux)
{
	struct mesongxl_usb2phy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = phandle;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}
	sc->sc_rst = fdtbus_reset_get_index(phandle, 0);

	if (sc->sc_rst != NULL) {
		if (fdtbus_reset_deassert(sc->sc_rst) != 0) {
			aprint_error(": couldn't de-assert reset\n");
			return;
		}
	}
	if (clk_enable(sc->sc_clk) != 0) {
		aprint_error(": couldn't enable clock\n");
		return;
	}

	sc->sc_supply = fdtbus_regulator_acquire(phandle, "phy-supply");

	aprint_naive("\n");
	aprint_normal(": USB2 PHY\n");

	fdtbus_register_phy_controller(self, phandle, &mesongxl_usb2phy_funcs);
}
