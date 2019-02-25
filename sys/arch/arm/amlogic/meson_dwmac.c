/* $NetBSD: meson_dwmac.c,v 1.3 2019/02/25 19:30:17 jmcneill Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: meson_dwmac.c,v 1.3 2019/02/25 19:30:17 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/gpio.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <dev/ic/dwc_gmac_var.h>
#include <dev/ic/dwc_gmac_reg.h>

#include <dev/fdt/fdtvar.h>

#define	GMAC_TX_RATE_MII		25000000
#define	GMAC_TX_RATE_RGMII		125000000

static const char * compatible[] = {
	"amlogic,meson8b-dwmac",
	"amlogic,meson-gx-dwmac",
	NULL
};

static int
meson_dwmac_reset(const int phandle)
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
meson_dwmac_intr(void *arg)
{
	struct dwc_gmac_softc * const sc = arg;

	return dwc_gmac_intr(sc);
}

static int
meson_dwmac_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
meson_dwmac_attach(device_t parent, device_t self, void *aux)
{
	struct dwc_gmac_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst_gmac;
	struct clk *clk_gmac;
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
	if (clk_gmac == NULL) {
		aprint_error(": couldn't get clocks\n");
		return;
	}

	rst_gmac = fdtbus_reset_get(phandle, "stmmaceth");

	phy_mode = fdtbus_get_string(phandle, "phy-mode");
	if (phy_mode == NULL) {
		aprint_error(": missing 'phy-mode' property\n");
		return;
	}
#if notyet
	if (strcmp(phy_mode, "mii") == 0) {
		if (clk_set_rate(clk_gmac_tx, GMAC_TX_RATE_MII) != 0) {
			aprint_error(": failed to set TX clock rate (MII)\n");
			return;
		}
	} else if (strcmp(phy_mode, "rgmii") == 0) {
		if (clk_set_rate(clk_gmac_tx, GMAC_TX_RATE_RGMII) != 0) {
			aprint_error(": failed to set TX clock rate (RGMII)\n");
			return;
		}
	} else {
		aprint_error(": unsupported phy-mode '%s'\n", phy_mode);
		return;
	}
#endif

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

	if (fdtbus_intr_establish(phandle, 0, IPL_NET, 0, meson_dwmac_intr, sc) == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n", intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	if (meson_dwmac_reset(phandle) != 0)
		aprint_error_dev(self, "PHY reset failed\n");

	dwc_gmac_attach(sc, MII_PHY_ANY, GMAC_MII_CLK_100_150M_DIV62);
}

CFATTACH_DECL_NEW(meson_dwmac, sizeof(struct dwc_gmac_softc),
	meson_dwmac_match, meson_dwmac_attach, NULL, NULL);
