/* $NetBSD: mesongxl_usb3phy.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: mesongxl_usb3phy.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/fdt/fdtvar.h>

#define	USB3PHY_REG0			0x00
#define	 REG0_U2D_ACT			__BIT(31)

#define	USB3PHY_REG1			0x04
#define	 REG1_U3H_FLADJ_30MHZ_REG	__BITS(24,19)

#define	USB3PHY_REG4			0x10
#define	 REG4_P21_SLEEP_M0		__BIT(1)

#define	USB3PHY_REG5			0x14
#define	 REG5_ID_DIG_TH			__BITS(15,8)
#define	 REG5_ID_DIG_EN_1		__BIT(5)
#define	 REG5_ID_DIG_EN_0		__BIT(4)

static int mesongxl_usb3phy_match(device_t, cfdata_t, void *);
static void mesongxl_usb3phy_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson-gxl-usb3-phy" },
	DEVICE_COMPAT_EOL
};

struct mesongxl_usb3phy_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
	struct clk		*sc_clk_phy;
	struct clk		*sc_clk_peripheral;
};

#define	PHY_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PHY_WRITE(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(mesongxl_usb3phy, sizeof(struct mesongxl_usb3phy_softc),
	mesongxl_usb3phy_match, mesongxl_usb3phy_attach, NULL, NULL);

static void *
mesongxl_usb3phy_acquire(device_t dev, const void *data, size_t len)
{
	if (len != 0)
		return NULL;

	return (void *)(uintptr_t)1;
}

static void
mesongxl_usb3phy_release(device_t dev, void *priv)
{
}

static int
mesongxl_usb3phy_enable(device_t dev, void *priv, bool enable)
{
	struct mesongxl_usb3phy_softc * const sc = device_private(dev);
	uint32_t val;

	if (enable) {
		/* Power on PHY */
		val = PHY_READ(sc, USB3PHY_REG5);
		val |= REG5_ID_DIG_EN_0;
		val |= REG5_ID_DIG_EN_1;
		val &= ~REG5_ID_DIG_TH;
		val |= __SHIFTIN(0xff, REG5_ID_DIG_TH);
		PHY_WRITE(sc, USB3PHY_REG5, val);

		/* Set host mode */
		val = PHY_READ(sc, USB3PHY_REG0);
		val &= ~REG0_U2D_ACT;
		PHY_WRITE(sc, USB3PHY_REG0, val);

		val = PHY_READ(sc, USB3PHY_REG4);
		val &= ~REG4_P21_SLEEP_M0;
		PHY_WRITE(sc, USB3PHY_REG4, val);
	} else {
		/* Power off PHY */
		val = PHY_READ(sc, USB3PHY_REG5);
		val &= ~REG5_ID_DIG_EN_0;
		val &= ~REG5_ID_DIG_EN_1;
		PHY_WRITE(sc, USB3PHY_REG5, val);
	}

	return 0;
}

const struct fdtbus_phy_controller_func mesongxl_usb3phy_funcs = {
	.acquire = mesongxl_usb3phy_acquire,
	.release = mesongxl_usb3phy_release,
	.enable = mesongxl_usb3phy_enable,
};

static void
mesongxl_usb3phy_init(struct mesongxl_usb3phy_softc *sc)
{
	uint32_t val;

	val = PHY_READ(sc, USB3PHY_REG1);
	val &= ~REG1_U3H_FLADJ_30MHZ_REG;
	val |= __SHIFTIN(0x20, REG1_U3H_FLADJ_30MHZ_REG);
	PHY_WRITE(sc, USB3PHY_REG1, val);
}

static int
mesongxl_usb3phy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
mesongxl_usb3phy_attach(device_t parent, device_t self, void *aux)
{
	struct mesongxl_usb3phy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_regulator *supply;
	struct fdtbus_reset *rst;
	bus_addr_t addr;
	bus_size_t size;
	u_int n;

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

	sc->sc_clk_phy = fdtbus_clock_get(phandle, "phy");
	if (sc->sc_clk_phy == NULL) {
		aprint_error(": couldn't get phy clock\n");
		return;
	}
#if notyet
	sc->sc_clk_peripheral = fdtbus_clock_get(phandle, "peripheral");
	if (sc->sc_clk_peripheral == NULL) {
		aprint_error(": couldn't get peripheral clock\n");
		return;
	}
#endif

	for (n = 0; (rst = fdtbus_reset_get_index(phandle, n)) != NULL; n++) {
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset #%d\n", n);
			return;
		}
	}
	if (clk_enable(sc->sc_clk_phy) != 0) {
		aprint_error(": couldn't enable phy clock\n");
		return;
	}
#if notyet
	if (clk_enable(sc->sc_clk_peripheral) != 0) {
		aprint_error(": couldn't enable peripheral clock\n");
		return;
	}
#endif

	supply = fdtbus_regulator_acquire(phandle, "phy-supply");
	if (supply != NULL) {
		if (fdtbus_regulator_enable(supply) != 0) {
			aprint_error(": couldn't enable supply\n");
			return;
		}
	}

	aprint_naive("\n");
	aprint_normal(": USB3 PHY\n");

	mesongxl_usb3phy_init(sc);

	fdtbus_register_phy_controller(self, phandle, &mesongxl_usb3phy_funcs);
}
