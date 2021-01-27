/* $NetBSD: sun9i_a80_usbphy.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: sun9i_a80_usbphy.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/fdt/fdtvar.h>

/* PMU registers */
#define	PMU_CFG			0x00
#define	 EHCI_HS_FORCE		__BIT(20)
#define	 HSIC_CONNECT_DET	__BIT(17)
#define	 HSIC_CONNECT_INT	__BIT(16)
#define	 AHB_INCR16		__BIT(11)
#define	 AHB_INCR8		__BIT(10)
#define	 AHB_INCR4		__BIT(9)
#define	 AHB_INCRX_ALIGN	__BIT(8)
#define	 HSIC			__BIT(1)
#define	 ULPI_BYPASS		__BIT(0)

static int sun9i_usbphy_match(device_t, cfdata_t, void *);
static void sun9i_usbphy_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun9i-a80-usb-phy" },
	DEVICE_COMPAT_EOL
};

struct sun9i_usbphy_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct clk		*sc_clk_phy;
	struct clk		*sc_clk_hsic;
	struct fdtbus_reset	*sc_rst;

	struct fdtbus_regulator	*sc_supply;
};

#define	PHY_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PHY_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(sunxi_a80_usbphy, sizeof(struct sun9i_usbphy_softc),
	sun9i_usbphy_match, sun9i_usbphy_attach, NULL, NULL);

static void *
sun9i_usbphy_acquire(device_t dev, const void *data, size_t len)
{
	struct sun9i_usbphy_softc * const sc = device_private(dev);

	return sc;
}

static void
sun9i_usbphy_release(device_t dev, void *priv)
{
}

static int
sun9i_usbphy_enable(device_t dev, void *priv, bool enable)
{
	struct sun9i_usbphy_softc * const sc = device_private(dev);
	uint32_t passby_mask;
	uint32_t val;
	int error;

	passby_mask = ULPI_BYPASS|AHB_INCR16|AHB_INCR8|AHB_INCR4|AHB_INCRX_ALIGN;
	if (sc->sc_clk_hsic != NULL)
		passby_mask |= HSIC|EHCI_HS_FORCE|HSIC_CONNECT_DET|HSIC_CONNECT_INT;

	/* Enable/disable passby */
	if (enable) {
		error = clk_enable(sc->sc_clk_phy);
		if (error != 0)
			return error;

		if (sc->sc_clk_hsic != NULL) {
			error = clk_enable(sc->sc_clk_hsic);
			if (error != 0)
				return error;
		}

		error = fdtbus_reset_deassert(sc->sc_rst);
		if (error != 0)
			return error;

		val = PHY_READ(sc, PMU_CFG);
		val |= passby_mask;
		PHY_WRITE(sc, PMU_CFG, val);
	} else {
		val = PHY_READ(sc, PMU_CFG);
		val &= ~passby_mask;
		PHY_WRITE(sc, PMU_CFG, val);

		error = fdtbus_reset_assert(sc->sc_rst);
		if (error != 0)
			return error;

		if (sc->sc_clk_hsic != NULL) {
			error = clk_disable(sc->sc_clk_hsic);
			if (error != 0)
				return error;
		}

		error = clk_disable(sc->sc_clk_phy);
		if (error != 0)
			return error;
	}

	return 0;
}

const struct fdtbus_phy_controller_func sun9i_usbphy_funcs = {
	.acquire = sun9i_usbphy_acquire,
	.release = sun9i_usbphy_release,
	.enable = sun9i_usbphy_enable,
};

static int
sun9i_usbphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sun9i_usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct sun9i_usbphy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const char *phy_type;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	phy_type = fdtbus_get_string(phandle, "phy_type");
	if (phy_type && strcmp(phy_type, "hsic") == 0) {
		sc->sc_clk_phy = fdtbus_clock_get(phandle, "hsic_480M");
		sc->sc_clk_hsic = fdtbus_clock_get(phandle, "hsic_12M");
		sc->sc_rst = fdtbus_reset_get(phandle, "hsic");

		if (sc->sc_clk_phy == NULL || sc->sc_clk_hsic == NULL || sc->sc_rst == NULL) {
			aprint_error(": couldn't get hsic resources\n");
			return;
		}
	} else {
		sc->sc_clk_phy = fdtbus_clock_get(phandle, "phy");
		sc->sc_rst = fdtbus_reset_get(phandle, "phy");
		if (sc->sc_clk_phy == NULL || sc->sc_rst == NULL) {
			aprint_error(": couldn't get phy resources\n");
			return;
		}
	}

	aprint_naive("\n");
	aprint_normal(": USB PHY\n");

	sc->sc_supply = fdtbus_regulator_acquire(phandle, "phy-supply");
	if (sc->sc_supply != NULL) {
		if (fdtbus_regulator_enable(sc->sc_supply) != 0)
			aprint_error_dev(self, "WARNING: couldn't enable power supply\n");
	}

	fdtbus_register_phy_controller(self, phandle, &sun9i_usbphy_funcs);
}
