/* $NetBSD: sunxi_gmac.c,v 1.10 2021/11/07 19:21:33 jmcneill Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: sunxi_gmac.c,v 1.10 2021/11/07 19:21:33 jmcneill Exp $");

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

#define	GMAC_TX_RATE_MII		25000000
#define	GMAC_TX_RATE_RGMII		125000000

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun7i-a20-gmac" },
	DEVICE_COMPAT_EOL
};

static int
sunxi_gmac_reset(const int phandle)
{
	struct fdtbus_gpio_pin *pin_reset;
	const u_int *reset_delay_us;
	bool reset_active_low;
	int len, val;

	pin_reset = fdtbus_gpio_acquire(phandle, "snps,reset-gpio", GPIO_PIN_OUTPUT);
	if (pin_reset == NULL)
		return 0;

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
sunxi_gmac_intr(void *arg)
{
	return dwc_gmac_intr(arg);
}

static int
sunxi_gmac_get_phyid(int phandle)
{
	bus_addr_t addr;
	int phy_phandle;

	phy_phandle = fdtbus_get_phandle(phandle, "phy");
	if (phy_phandle == -1)
		phy_phandle = fdtbus_get_phandle(phandle, "phy-handle");
	if (phy_phandle == -1)
		return MII_PHY_ANY;

	if (fdtbus_get_reg(phy_phandle, 0, &addr, NULL) != 0)
		return MII_PHY_ANY;

	return (int)addr;
}

static int
sunxi_gmac_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sunxi_gmac_attach(device_t parent, device_t self, void *aux)
{
	struct dwc_gmac_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct clk *clk_gmac, *clk_gmac_tx;
	struct fdtbus_reset *rst_gmac;
	struct fdtbus_regulator *reg_phy;
	const char *phy_mode;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_dmat = faa->faa_dmat;

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	clk_gmac = fdtbus_clock_get(phandle, "stmmaceth");
	clk_gmac_tx = fdtbus_clock_get(phandle, "allwinner_gmac_tx");
	if (clk_gmac == NULL || clk_gmac_tx == NULL) {
		aprint_error(": couldn't get clocks\n");
		return;
	}

	rst_gmac = fdtbus_reset_get(phandle, "stmmaceth");

	phy_mode = fdtbus_get_string(phandle, "phy-mode");
	if (phy_mode == NULL) {
		aprint_error(": missing 'phy-mode' property\n");
		return;
	}

	reg_phy = fdtbus_regulator_acquire(phandle, "phy-supply");
	if (reg_phy != NULL && fdtbus_regulator_enable(reg_phy) != 0) {
		aprint_error(": couldn't enable PHY regualtor\n");
		return;
	}

	if (strcmp(phy_mode, "mii") == 0) {
		if (clk_set_rate(clk_gmac_tx, GMAC_TX_RATE_MII) != 0) {
			aprint_error(": failed to set TX clock rate (MII)\n");
			return;
		}
	} else if (strncmp(phy_mode, "rgmii", 5) == 0) {
		if (clk_set_rate(clk_gmac_tx, GMAC_TX_RATE_RGMII) != 0) {
			aprint_error(": failed to set TX clock rate (RGMII)\n");
			return;
		}
	} else {
		aprint_error(": unsupported phy-mode '%s'\n", phy_mode);
		return;
	}

	if (clk_enable(clk_gmac_tx) != 0 || clk_enable(clk_gmac) != 0) {
		aprint_error(": couldn't enable clocks\n");
		return;
	}

	if (rst_gmac != NULL && fdtbus_reset_deassert(rst_gmac) != 0) {
		aprint_error(": couldn't de-assert reset\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": GMAC\n");

	if (fdtbus_intr_establish_xname(phandle, 0, IPL_NET,
	    DWCGMAC_FDT_INTR_MPSAFE, sunxi_gmac_intr, sc,
	    device_xname(self)) == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n", intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	if (sunxi_gmac_reset(phandle) != 0)
		aprint_error_dev(self, "PHY reset failed\n");

	dwc_gmac_attach(sc, sunxi_gmac_get_phyid(phandle),
	    GMAC_MII_CLK_150_250M_DIV102);
}

CFATTACH_DECL_NEW(sunxi_gmac, sizeof(struct dwc_gmac_softc),
	sunxi_gmac_match, sunxi_gmac_attach, NULL, NULL);
