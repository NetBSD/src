/* $NetBSD: meson_dwmac.c,v 1.14 2021/11/19 07:04:27 jdc Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: meson_dwmac.c,v 1.14 2021/11/19 07:04:27 jdc Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/gpio.h>
#include <sys/rndsource.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <dev/ic/dwc_gmac_var.h>
#include <dev/ic/dwc_gmac_reg.h>

#include <dev/fdt/fdtvar.h>

#define	PRG_ETHERNET_ADDR0		0x00
#define	 CLKGEN_ENABLE			__BIT(12)
#define	 RMII_CLK_I_INVERTED		__BIT(11)
#define	 PHY_CLK_ENABLE			__BIT(10)
#define	 MP2_CLK_OUT_DIV		__BITS(9,7)
#define	 TX_CLK_DELAY			__BITS(6,5)
#define	 PHY_INTERFACE_SEL		__BIT(0)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson8b-dwmac" },
	{ .compat = "amlogic,meson-gx-dwmac" },
	{ .compat = "amlogic,meson-gxbb-dwmac" },
	{ .compat = "amlogic,meson-axg-dwmac" },
	DEVICE_COMPAT_EOL
};

static int
meson_dwmac_reset_eth(const int phandle)
{
	struct fdtbus_gpio_pin *pin_reset;
	const u_int *reset_delay_us;
	bool reset_active_low;
	int len, val;

	pin_reset = fdtbus_gpio_acquire(phandle, "snps,reset-gpio",
	    GPIO_PIN_OUTPUT);
	if (pin_reset == NULL)
		return ENXIO;

	reset_delay_us = fdtbus_get_prop(phandle, "snps,reset-delays-us", &len);
	if (reset_delay_us == NULL || len != 12)
		return ENXIO;

	reset_active_low = of_hasprop(phandle, "snps,reset-active-low");

	val = reset_active_low ? 1 : 0;

	fdtbus_gpio_write(pin_reset, val);
	delay(be32toh(reset_delay_us[0]));
	fdtbus_gpio_write(pin_reset, !val);
	delay(be32toh(reset_delay_us[1]));
	fdtbus_gpio_write(pin_reset, val);
	delay(be32toh(reset_delay_us[2]));

	return 0;
}

static int
meson_dwmac_reset_phy(const int phandle)
{
	struct fdtbus_gpio_pin *pin_reset;
	const u_int *reset_assert_us, *reset_deassert_us, *reset_gpios;
	bool reset_active_low;
	int len, val;

	pin_reset = fdtbus_gpio_acquire(phandle, "reset-gpios",
	    GPIO_PIN_OUTPUT);
	if (pin_reset == NULL)
		return ENXIO;

	reset_assert_us = fdtbus_get_prop(phandle, "reset-assert-us", &len);
	if (reset_assert_us == NULL || len != 4)
		return ENXIO;
	reset_deassert_us = fdtbus_get_prop(phandle, "reset-deassert-us", &len);
	if (reset_deassert_us == NULL || len != 4)
		return ENXIO;
	reset_gpios = fdtbus_get_prop(phandle, "reset-gpios", &len);
	if (reset_gpios == NULL || len != 12)
		return ENXIO;

	reset_active_low = be32toh(reset_gpios[2]);

	val = reset_active_low ? 1 : 0;

	fdtbus_gpio_write(pin_reset, val);
	delay(be32toh(reset_assert_us[0]));
	fdtbus_gpio_write(pin_reset, !val);
	delay(be32toh(reset_deassert_us[0]));

	return 0;
}

static void
meson_dwmac_set_mode_rgmii(int phandle, bus_space_tag_t bst,
    bus_space_handle_t bsh, struct clk *clkin)
{
	u_int tx_delay;
	uint32_t val;

#define DIV_ROUND_OFF(x, y)	(((x) + (y) / 2) / (y))
	const u_int div = DIV_ROUND_OFF(clk_get_rate(clkin), 250000000);

	if (of_getprop_uint32(phandle, "amlogic,tx-delay-ns", &tx_delay) != 0)
		tx_delay = 2;

	val = bus_space_read_4(bst, bsh, PRG_ETHERNET_ADDR0);
	val |= PHY_INTERFACE_SEL;
	val &= ~TX_CLK_DELAY;
	val |= __SHIFTIN((tx_delay >> 1), TX_CLK_DELAY);
	val &= ~MP2_CLK_OUT_DIV;
	val |= __SHIFTIN(div, MP2_CLK_OUT_DIV);
	val |= PHY_CLK_ENABLE;
	val |= CLKGEN_ENABLE;
	bus_space_write_4(bst, bsh, PRG_ETHERNET_ADDR0, val);
}

static void
meson_dwmac_set_mode_rmii(int phandle, bus_space_tag_t bst,
    bus_space_handle_t bsh)
{
	uint32_t val;

	val = bus_space_read_4(bst, bsh, PRG_ETHERNET_ADDR0);
	val &= ~PHY_INTERFACE_SEL;
	val |= RMII_CLK_I_INVERTED;
	val &= ~TX_CLK_DELAY;
	val |= CLKGEN_ENABLE;
	bus_space_write_4(bst, bsh, PRG_ETHERNET_ADDR0, val);
}

static int
meson_dwmac_intr(void *arg)
{
	struct dwc_gmac_softc * const sc = arg;

	return dwc_gmac_intr(sc);
}

static int
meson_dwmac_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
meson_dwmac_attach(device_t parent, device_t self, void *aux)
{
	struct dwc_gmac_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	int miiclk, phandle_phy, phy = MII_PHY_ANY;
	u_int miiclk_rate;
	bus_space_handle_t prgeth_bsh;
	struct fdtbus_reset *rst_gmac;
	struct clk *clk_gmac, *clk_in[2];
	const char *phy_mode;
	char intrstr[128];
	bus_addr_t addr[2];
	bus_size_t size[2];

	if (fdtbus_get_reg(phandle, 0, &addr[0], &size[0]) != 0 ||
	    fdtbus_get_reg(phandle, 1, &addr[1], &size[1]) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr[0], size[0], 0, &sc->sc_bsh) != 0 ||
	    bus_space_map(sc->sc_bst, addr[1], size[1], 0, &prgeth_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_dmat = faa->faa_dmat;

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	clk_gmac = fdtbus_clock_get(phandle, "stmmaceth");
	clk_in[0] = fdtbus_clock_get(phandle, "clkin0");
	clk_in[1] = fdtbus_clock_get(phandle, "clkin1");
	if (clk_gmac == NULL || clk_in[0] == NULL || clk_in[1] == NULL) {
		aprint_error(": couldn't get clocks\n");
		return;
	}

	rst_gmac = fdtbus_reset_get(phandle, "stmmaceth");

	phy_mode = fdtbus_get_string(phandle, "phy-mode");
	if (phy_mode == NULL) {
		aprint_error(": missing 'phy-mode' property\n");
		return;
	}
	phandle_phy = fdtbus_get_phandle(phandle, "phy-handle");
	if (phandle_phy > 0) {
		of_getprop_uint32(phandle_phy, "reg", &phy);
	} else {
		phandle_phy = phandle;
	}

	if (strncmp(phy_mode, "rgmii", 5) == 0) {
		meson_dwmac_set_mode_rgmii(phandle, sc->sc_bst, prgeth_bsh, clk_in[0]);
	} else if (strcmp(phy_mode, "rmii") == 0) {
		meson_dwmac_set_mode_rmii(phandle, sc->sc_bst, prgeth_bsh);
	} else {
		aprint_error(": unsupported phy-mode '%s'\n", phy_mode);
		return;
	}

	if (clk_enable(clk_gmac) != 0) {
		aprint_error(": couldn't enable clock\n");
		return;
	}

	if (rst_gmac != NULL && fdtbus_reset_deassert(rst_gmac) != 0) {
		aprint_error(": couldn't de-assert reset\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");

	if (fdtbus_intr_establish_xname(phandle, 0, IPL_NET,
	    DWCGMAC_FDT_INTR_MPSAFE, meson_dwmac_intr, sc,
	    device_xname(sc->sc_dev)) == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n", intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	/*
	 * Depending on the DTS, we need to check either the "snps,...",
	 * properties on the ethernet node, or the "reset-..."
	 * properties on the phy node for the MAC reset information.
	 */

	if (of_hasprop(phandle, "snps,reset-gpio")) {
		if (meson_dwmac_reset_eth(phandle) != 0)
			aprint_error_dev(self, "PHY reset failed\n");
	} else {
		if (meson_dwmac_reset_phy(phandle_phy) != 0)
			aprint_error_dev(self, "PHY reset failed\n");
	}

	miiclk_rate = clk_get_rate(clk_gmac);
	if (miiclk_rate > 250 * 1000 * 1000)
		miiclk = GMAC_MII_CLK_250_300M_DIV124;
	else if (miiclk_rate > 150 * 1000 * 1000)
		miiclk = GMAC_MII_CLK_150_250M_DIV102;
	else if (miiclk_rate > 100 * 1000 * 1000)
		miiclk = GMAC_MII_CLK_100_150M_DIV62;
	else if (miiclk_rate > 60 * 1000 * 1000)
		miiclk = GMAC_MII_CLK_60_100M_DIV42;
	else if (miiclk_rate > 35 * 1000 * 1000)
		miiclk = GMAC_MII_CLK_35_60M_DIV26;
	else
		miiclk = GMAC_MII_CLK_25_35M_DIV16;

	dwc_gmac_attach(sc, phy, miiclk);
}

CFATTACH_DECL_NEW(meson_dwmac, sizeof(struct dwc_gmac_softc),
	meson_dwmac_match, meson_dwmac_attach, NULL, NULL);
