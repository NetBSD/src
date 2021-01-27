/* $NetBSD: meson_usbphy.c,v 1.6 2021/01/27 03:10:18 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: meson_usbphy.c,v 1.6 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/fdt/fdtvar.h>

#define	CBUS_REG(x)			((x) << 2)
#define	PREI_USB_PHY_CFG_REG		CBUS_REG(0x00)
#define	 PREI_USB_PHY_CFG_CLK_32K_ALT_SEL	__BIT(15)
#define	PREI_USB_PHY_CTRL_REG		CBUS_REG(0x01)
#define	 PREI_USB_PHY_CTRL_FSEL		__BITS(24,22)
#define	 PREI_USB_PHY_CTRL_FSEL_24M	5
#define	 PREI_USB_PHY_CTRL_FSEL_12M	2
#define	 PREI_USB_PHY_CTRL_POR		__BIT(15)
#define	 PREI_USB_PHY_CTRL_CLK_DET	__BIT(8)
#define	PREI_USB_PHY_ADP_BC_REG		CBUS_REG(0x03)
#define	 PREI_USB_PHY_ADP_BC_ACA_FLOATING __BIT(26)
#define	 PREI_USB_PHY_ADP_BC_ACA_ENABLE	__BIT(16)

static int meson_usbphy_match(device_t, cfdata_t, void *);
static void meson_usbphy_attach(device_t, device_t, void *);

enum meson_usbphy_type {
	USBPHY_MESON8B,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson8b-usb2-phy",
	  .value = USBPHY_MESON8B },
	{ .compat = "amlogic,meson-gxbb-usb2-phy",
	  .value = USBPHY_MESON8B },

	DEVICE_COMPAT_EOL
};

struct meson_usbphy_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
	const char		*sc_dr_mode;
	enum meson_usbphy_type	sc_type;
};

#define	PHY_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PHY_WRITE(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(meson_usbphy, sizeof(struct meson_usbphy_softc),
	meson_usbphy_match, meson_usbphy_attach, NULL, NULL);

static const char *
meson_usbphy_dr_mode(struct meson_usbphy_softc *sc)
{
	int index, phandle;

	index = 0;
	while ((phandle = fdt_find_with_property("phys", &index)) != -1) {
		const int phy_phandle = fdtbus_get_phandle(phandle, "phys");
		if (phy_phandle != sc->sc_phandle)
			continue;
		return fdtbus_get_string(phandle, "dr_mode");
	}

	return NULL;
}

static void *
meson_usbphy_acquire(device_t dev, const void *data, size_t len)
{
	if (len != 0)
		return NULL;

	return (void *)(uintptr_t)1;
}

static void
meson_usbphy_release(device_t dev, void *priv)
{
}

static int
meson_usbphy_enable(device_t dev, void *priv, bool enable)
{
	struct meson_usbphy_softc * const sc = device_private(dev);
	struct fdtbus_regulator *reg;
	uint32_t val;
	int error;

	if (enable) {
		if (of_hasprop(sc->sc_phandle, "phy-supply")) {
			reg = fdtbus_regulator_acquire(sc->sc_phandle, "phy-supply");
			if (reg != NULL) {
				error = fdtbus_regulator_enable(reg);
				if (error != 0)
					device_printf(dev, "WARNING: couldn't enable phy-supply: %d\n", error);
			} else {
				device_printf(dev, "WARNING: couldn't acquire phy-supply\n");
			}
		}

		delay(1000);

		val = PHY_READ(sc, PREI_USB_PHY_CFG_REG);
		val |= PREI_USB_PHY_CFG_CLK_32K_ALT_SEL;
		PHY_WRITE(sc, PREI_USB_PHY_CFG_REG, val);

		val = PHY_READ(sc, PREI_USB_PHY_CTRL_REG);
		val &= ~PREI_USB_PHY_CTRL_FSEL;
		val |= __SHIFTIN(PREI_USB_PHY_CTRL_FSEL_24M,
				 PREI_USB_PHY_CTRL_FSEL);
		val |= PREI_USB_PHY_CTRL_POR;
		PHY_WRITE(sc, PREI_USB_PHY_CTRL_REG, val);

		delay(1000);

		val = PHY_READ(sc, PREI_USB_PHY_CTRL_REG);
		val &= ~PREI_USB_PHY_CTRL_POR;
		PHY_WRITE(sc, PREI_USB_PHY_CTRL_REG, val);

		delay(50000);

		val = PHY_READ(sc, PREI_USB_PHY_CTRL_REG);
		if ((val & PREI_USB_PHY_CTRL_CLK_DET) == 0)
			aprint_error_dev(dev, "WARNING: USB PHY clock not detected\n");

		if (sc->sc_dr_mode && strcmp(sc->sc_dr_mode, "host") == 0) {
			val = PHY_READ(sc, PREI_USB_PHY_ADP_BC_REG);
			val |= PREI_USB_PHY_ADP_BC_ACA_ENABLE;
			PHY_WRITE(sc, PREI_USB_PHY_ADP_BC_REG, val);

			delay(1000);

			val = PHY_READ(sc, PREI_USB_PHY_ADP_BC_REG);
			if ((val & PREI_USB_PHY_ADP_BC_ACA_FLOATING) != 0)
				aprint_error_dev(dev, "WARNING: USB PHY failed to enable ACA detection\n");
		}
	}

	return 0;
}

const struct fdtbus_phy_controller_func meson_usbphy_funcs = {
	.acquire = meson_usbphy_acquire,
	.release = meson_usbphy_release,
	.enable = meson_usbphy_enable,
};

static int
meson_usbphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
meson_usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct meson_usbphy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	u_int n;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = phandle;
	sc->sc_type = of_compatible_lookup(phandle, compat_data)->value;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
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

	sc->sc_dr_mode = meson_usbphy_dr_mode(sc);

	aprint_naive("\n");
	aprint_normal(": USB2 PHY (%s)\n", sc->sc_dr_mode ? sc->sc_dr_mode : "unknown mode");

	fdtbus_register_phy_controller(self, phandle, &meson_usbphy_funcs);
}
