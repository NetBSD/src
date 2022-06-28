/* $NetBSD: sunxi_de2_ccu.c,v 1.8 2022/06/28 05:19:03 skrll Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: sunxi_de2_ccu.c,v 1.8 2022/06/28 05:19:03 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sunxi_de2_ccu.h>

static int sunxi_de2_ccu_match(device_t, cfdata_t, void *);
static void sunxi_de2_ccu_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sunxi_de2ccu, sizeof(struct sunxi_ccu_softc),
	sunxi_de2_ccu_match, sunxi_de2_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun8i_h3_de2_ccu_resets[] = {
	SUNXI_CCU_RESET(DE2_RST_MIXER0, 0x08, 0),
	SUNXI_CCU_RESET(DE2_RST_WB, 0x08, 2),
};

static struct sunxi_ccu_reset sun50i_a64_de2_ccu_resets[] = {
	SUNXI_CCU_RESET(DE2_RST_MIXER0, 0x08, 0),
	SUNXI_CCU_RESET(DE2_RST_MIXER1, 0x08, 1),
	SUNXI_CCU_RESET(DE2_RST_WB, 0x08, 2),
};

static const char *mod_parents[] = { "mod" };

static struct sunxi_ccu_clk sun8i_h3_de2_ccu_clks[] = {
	SUNXI_CCU_GATE(DE2_CLK_BUS_MIXER0, "bus-mixer0", "bus", 0x04, 0),
	SUNXI_CCU_GATE(DE2_CLK_BUS_MIXER1, "bus-mixer1", "bus", 0x04, 1),
	SUNXI_CCU_GATE(DE2_CLK_BUS_WB, "bus-wb", "bus", 0x04, 2),

	SUNXI_CCU_DIV(DE2_CLK_MIXER0_DIV, "mixer0-div", mod_parents,
	    0x0c, __BITS(3,0), 0, SUNXI_CCU_DIV_SET_RATE_PARENT),
	SUNXI_CCU_DIV(DE2_CLK_MIXER1_DIV, "mixer1-div", mod_parents,
	    0x0c, __BITS(7,4), 0, SUNXI_CCU_DIV_SET_RATE_PARENT),
	SUNXI_CCU_DIV(DE2_CLK_WB_DIV, "wb-div", mod_parents,
	    0x0c, __BITS(11,8), 0, SUNXI_CCU_DIV_SET_RATE_PARENT),

	SUNXI_CCU_GATE(DE2_CLK_MIXER0, "mixer0", "mixer0-div", 0x00, 0),
	SUNXI_CCU_GATE(DE2_CLK_MIXER1, "mixer1", "mixer1-div", 0x00, 1),
	SUNXI_CCU_GATE(DE2_CLK_WB, "wb", "wb-div", 0x00, 2),
};

struct sunxi_de2_ccu_config {
	struct sunxi_ccu_reset	*resets;
	u_int			nresets;
	struct sunxi_ccu_clk	*clks;
	u_int			nclks;
};

static const struct sunxi_de2_ccu_config sun8i_h3_de2_config = {
	.resets = sun8i_h3_de2_ccu_resets,
	.nresets = __arraycount(sun8i_h3_de2_ccu_resets),
	.clks = sun8i_h3_de2_ccu_clks,
	.nclks = __arraycount(sun8i_h3_de2_ccu_clks),
};

static const struct sunxi_de2_ccu_config sun50i_a64_de2_config = {
	.resets = sun50i_a64_de2_ccu_resets,
	.nresets = __arraycount(sun50i_a64_de2_ccu_resets),
	.clks = sun8i_h3_de2_ccu_clks,
	.nclks = __arraycount(sun8i_h3_de2_ccu_clks),
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun8i-h3-de2-clk",
	  .data = &sun8i_h3_de2_config },
	{ .compat = "allwinner,sun8i-v3s-de2-clk",
	  .data = &sun8i_h3_de2_config },
	{ .compat = "allwinner,sun50i-a64-de2-clk",
	  .data = &sun50i_a64_de2_config },
	{ .compat = "allwinner,sun50i-h5-de2-clk",
	  .data = &sun50i_a64_de2_config },

	DEVICE_COMPAT_EOL
};

static int
sunxi_de2_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sunxi_de2_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const struct sunxi_de2_ccu_config *conf;
	struct clk *clk_bus, *clk_mod;
	struct fdtbus_reset *rst;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;

	conf = of_compatible_lookup(phandle, compat_data)->data;

	sc->sc_resets = conf->resets;
	sc->sc_nresets = conf->nresets;
	sc->sc_clks = conf->clks;
	sc->sc_nclks = conf->nclks;

	clk_bus = fdtbus_clock_get(phandle, "bus");
	if (clk_bus == NULL || clk_enable(clk_bus) != 0) {
		aprint_error(": couldn't enable bus clock\n");
		return;
	}
	clk_mod = fdtbus_clock_get(phandle, "mod");
	if (clk_mod == NULL || clk_enable(clk_mod) != 0) {
		aprint_error(": couldn't enable mod clock\n");
		return;
	}
	rst = fdtbus_reset_get_index(phandle, 0);
	if (rst == NULL || fdtbus_reset_deassert(rst) != 0) {
		aprint_error(": couldn't de-assert reset\n");
		return;
	}

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": DE2 CCU\n");

	sunxi_ccu_print(sc);
}
