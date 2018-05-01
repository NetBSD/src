/* $NetBSD: sunxi_usb3phy.c,v 1.1 2018/05/01 23:59:42 jmcneill Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: sunxi_usb3phy.c,v 1.1 2018/05/01 23:59:42 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/fdt/fdtvar.h>

#define	SUNXI_APP			0x00
#define	 APP_FORCE_VBUS			__BITS(13,12)

#define	SUNXI_PIPE_CLOCK_CONTROL	0x14
#define	 PCC_PIPE_CLK_OPEN		__BIT(6)

#define	SUNXI_PHY_TUNE_LOW		0x18
#define	 PTL_MAGIC			0x0047fc87

#define	SUNXI_PHY_TUNE_HIGH		0x1c
#define	 PTH_TX_DEEMPH_3P5DB		__BITS(24,19)
#define	 PTH_TX_DEEMPH_6DB		__BITS(18,13)
#define	 PTH_TX_SWING_FULL		__BITS(12,6)
#define	 PTH_LOS_BIAS			__BITS(5,3)
#define	 PTH_TX_BOOST_LVL		__BITS(2,0)

#define	SUNXI_PHY_EXTERNAL_CONTROL	0x20
#define	 PEC_REF_SSP_EN			__BIT(26)
#define	 PEC_SSC_EN			__BIT(24)
#define	 PEC_EXTERN_VBUS		__BITS(2,1)

static int sunxi_usb3phy_match(device_t, cfdata_t, void *);
static void sunxi_usb3phy_attach(device_t, device_t, void *);

enum sunxi_usb3phy_type {
	USB3PHY_H6 = 1,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun50i-h6-usb3-phy",	USB3PHY_H6 },
	{ NULL }
};

struct sunxi_usb3phy {
	bus_space_tag_t		phy_bst;
	bus_space_handle_t	phy_bsh;
	struct fdtbus_regulator *phy_reg;
};

struct sunxi_usb3phy_softc {
	device_t		sc_dev;
	enum sunxi_usb3phy_type	sc_type;

	struct sunxi_usb3phy	sc_phy;

	struct fdtbus_gpio_pin	*sc_gpio_id_det;
	struct fdtbus_gpio_pin	*sc_gpio_vbus_det;
};

#define	PHY_READ(phy, reg)				\
	bus_space_read_4((phy)->phy_bst, (phy)->phy_bsh, (reg))
#define	PHY_WRITE(phy, reg, val)				\
	bus_space_write_4((phy)->phy_bst, (phy)->phy_bsh, (reg), (val))

CFATTACH_DECL_NEW(sunxi_usb3phy, sizeof(struct sunxi_usb3phy_softc),
	sunxi_usb3phy_match, sunxi_usb3phy_attach, NULL, NULL);

static void *
sunxi_usb3phy_acquire(device_t dev, const void *data, size_t len)
{
	struct sunxi_usb3phy_softc * const sc = device_private(dev);

	return &sc->sc_phy;
}

static void
sunxi_usb3phy_release(device_t dev, void *priv)
{
}

static int
sunxi_usb3phy_enable(device_t dev, void *priv, bool enable)
{
	struct sunxi_usb3phy * const phy = priv;
	uint32_t val;

	if (enable) {
		val = PHY_READ(phy, SUNXI_PHY_EXTERNAL_CONTROL);
		val |= PEC_EXTERN_VBUS;
		val |= PEC_SSC_EN;
		val |= PEC_REF_SSP_EN;
		PHY_WRITE(phy, SUNXI_PHY_EXTERNAL_CONTROL, val);

		val = PHY_READ(phy, SUNXI_PIPE_CLOCK_CONTROL);
		val |= PCC_PIPE_CLK_OPEN;
		PHY_WRITE(phy, SUNXI_PIPE_CLOCK_CONTROL, val);

		val = PHY_READ(phy, SUNXI_APP);
		val |= APP_FORCE_VBUS;
		PHY_WRITE(phy, SUNXI_APP, val);

		PHY_WRITE(phy, SUNXI_PHY_TUNE_LOW, PTL_MAGIC);

		val = PHY_READ(phy, SUNXI_PHY_TUNE_HIGH);
		val |= PTH_TX_BOOST_LVL;
		val |= PTH_LOS_BIAS;
		val &= ~PTH_TX_SWING_FULL;
		val |= __SHIFTIN(0x55, PTH_TX_SWING_FULL);
		val &= ~PTH_TX_DEEMPH_6DB;
		val |= __SHIFTIN(0x20, PTH_TX_DEEMPH_6DB);
		val &= ~PTH_TX_DEEMPH_3P5DB;
		val |= __SHIFTIN(0x15, PTH_TX_DEEMPH_3P5DB);
		PHY_WRITE(phy, SUNXI_PHY_TUNE_HIGH, val);

		return phy->phy_reg ? fdtbus_regulator_enable(phy->phy_reg) : 0;
	} else {
		return phy->phy_reg ? fdtbus_regulator_disable(phy->phy_reg) : 0;
	}
}

const struct fdtbus_phy_controller_func sunxi_usb3phy_funcs = {
	.acquire = sunxi_usb3phy_acquire,
	.release = sunxi_usb3phy_release,
	.enable = sunxi_usb3phy_enable,
};

static int
sunxi_usb3phy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_usb3phy_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_usb3phy_softc * const sc = device_private(self);
	struct sunxi_usb3phy *phy = &sc->sc_phy;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	u_int n;

	sc->sc_dev = self;
	sc->sc_type = of_search_compatible(phandle, compat_data)->data;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get phy registers\n");
		return;
	}

	phy->phy_bst = faa->faa_bst;
	if (bus_space_map(phy->phy_bst, addr, size, 0, &phy->phy_bsh) != 0) {
		aprint_error(": couldn't map phy registers\n");
		return;
	}

	/* Get optional regulator */
	phy->phy_reg = fdtbus_regulator_acquire(phandle, "phy-supply");

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

	aprint_naive("\n");
	aprint_normal(": USB3 PHY\n");

	fdtbus_register_phy_controller(self, phandle, &sunxi_usb3phy_funcs);
}
