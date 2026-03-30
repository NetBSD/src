/* $NetBSD $ */

/*-
* Copyright (c) 2025 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Yuri Honegger.
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

/*
 * Power&Sleep Controller for the TI AM18XX SOC.
 */

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/device.h>

#include <dev/clk/clk_backend.h>
#include <dev/fdt/fdtvar.h>

#include <arm/fdt/arm_fdtvar.h>

#define MAX_PARENT_CLOCKS 5

struct am18xx_psc_clk {
	struct clk clk_base;
	int clk_index;
	union {
		const char *clk_parent_name;
		struct clk *clk_parent;
	} u;
};

struct am18xx_psc_config {
	struct am18xx_psc_clk *clks;
	int clknum;
};

struct am18xx_psc_softc {
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct clk_domain sc_clkdom;
	struct clk parent_clocks[MAX_PARENT_CLOCKS];
	struct am18xx_psc_clk *sc_clks;
	int sc_clknum;
};

static int		am18xx_psc_match(device_t, cfdata_t, void *);
static void		am18xx_psc_attach(device_t, device_t, void *);
static struct clk *	am18xx_psc_decode(device_t, int, const void *, size_t);
static struct clk *	am18xx_psc_clk_get(void *, const char *);
static u_int		am18xx_psc_clk_get_rate(void *, struct clk *);
static int		am18xx_psc_clk_enable(void *, struct clk *);
static int		am18xx_psc_clk_disable(void *, struct clk *);
static struct clk *	am18xx_psc_clk_get_parent(void *, struct clk *);
static void		am18xx_psc_clk_transition(struct am18xx_psc_softc *,
						  struct am18xx_psc_clk *,
						  uint32_t);

CFATTACH_DECL_NEW(am18xxpsc, sizeof(struct am18xx_psc_softc),
		  am18xx_psc_match, am18xx_psc_attach, NULL, NULL);

#define AM18XX_PSC_PTCMD 0x120
#define AM18XX_PSC_PTSTAT 0x128
#define AM18XX_PSC_MDCTL0 0xA00

#define AM18XX_PSC_PTCMD_GO_ALL 0x3
#define AM18XX_PSC_PTSTAT_MASK 0x3
#define AM18XX_PSC_MDCTL_DISABLE 0x2
#define AM18XX_PSC_MDCTL_ENABLE 0x3

#define	PSC_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, reg)
#define	PSC_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, reg, val)

#define PSC_CLK(_i, _name, _p) 						\
	{								\
		.clk_index = (_i), 					\
		.u.clk_parent_name = (_p), 				\
		.clk_base.name = (_name),				\
		.clk_base.flags = 0,					\
	}

static struct am18xx_psc_clk am18xx_psc0_clks[] = {
	PSC_CLK(0, "edma0_cc0", "pll0_sysclk2"),
	PSC_CLK(1, "edma0_tc0", "pll0_sysclk2"),
	PSC_CLK(2, "edma0_tc1", "pll0_sysclk2"),
	PSC_CLK(3, "emifa", "async1"),
	PSC_CLK(4, "spi0", "pll0_sysclk2"),
	PSC_CLK(5, "sdmmc0", "pll0_sysclk2"),
	PSC_CLK(6, "aintc", "pll0_sysclk4"),
	PSC_CLK(7, "imem", "pll0_sysclk2"),
	PSC_CLK(8, NULL, NULL), /* unused */
	PSC_CLK(9, "uart0", "pll0_sysclk2"),
	PSC_CLK(10, NULL, NULL), /* scr0; purpose and parent clk unclear */
	PSC_CLK(11, NULL, NULL), /* scr1; purpose and parent clk unclear */
	PSC_CLK(12, NULL, NULL), /* scr2; purpose and parent clk unclear */
	PSC_CLK(13, "pru", "pll0_sysclk2"),
	PSC_CLK(14, "arm", "pll0_sysclk6"),
};

static struct am18xx_psc_clk am18xx_psc1_clks[] = {
	PSC_CLK(0, "edma1_cc0", "pll0_sysclk2"),
	PSC_CLK(1, "usb2_0", "pll0_sysclk2"),
	PSC_CLK(2, "usb1_1", "pll0_sysclk4"),
	PSC_CLK(3, "gpio", "pll0_sysclk4"),
	PSC_CLK(4, "hpi", "pll0_sysclk2"),
	PSC_CLK(5, "emac", "pll0_sysclk4"),
	PSC_CLK(6, "ddr", "pll0_sysclk2"),
	PSC_CLK(7, "mcasp0", "async3"),
	PSC_CLK(8, "sata", "pll0_sysclk2"),
	PSC_CLK(9, "vpif", "pll0_sysclk2"),
	PSC_CLK(10, "spi1", "async3"),
	PSC_CLK(11, "i2c1", "pll0_sysclk4"),
	PSC_CLK(12, "uart1", "async3"),
	PSC_CLK(13, "uart2", "async3"),
	PSC_CLK(14, "mcbsp0", "async3"),
	PSC_CLK(15, "mcbsp1", "async3"),
	PSC_CLK(16, "lcdc", "pll0_sysclk2"),
	PSC_CLK(17, "ehrpwm", "async3"),
	PSC_CLK(18, "sdmmc1", "pll0_sysclk2"),
	PSC_CLK(19, "upp", "pll0_sysclk2"),
	PSC_CLK(20, "ecap", "async3"),
	PSC_CLK(21, "edma1_tc0", "pll0_sysclk2"),
};

static const struct am18xx_psc_config am18xx_psc0_config = {
	.clks = am18xx_psc0_clks,
	.clknum = __arraycount(am18xx_psc0_clks)
};

static const struct am18xx_psc_config am18xx_psc1_config = {
	.clks = am18xx_psc1_clks,
	.clknum = __arraycount(am18xx_psc1_clks)
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,da850-psc0", .data = &am18xx_psc0_config },
	{ .compat = "ti,da850-psc1", .data = &am18xx_psc1_config },
	DEVICE_COMPAT_EOL
};

static const struct fdtbus_clock_controller_func am18xx_psc_clk_fdt_funcs = {
	.decode = am18xx_psc_decode,
};

static const struct clk_funcs am18xx_psc_clk_funcs = {
	.get = am18xx_psc_clk_get,
	.get_rate = am18xx_psc_clk_get_rate,
	.enable = am18xx_psc_clk_enable,
	.disable = am18xx_psc_clk_disable,
	.get_parent = am18xx_psc_clk_get_parent
};

static void
am18xx_psc_clk_transition(struct am18xx_psc_softc *sc,
			struct am18xx_psc_clk *clk, uint32_t state)
{
	/* update clock gate state */
	uint32_t mdctl_reg = AM18XX_PSC_MDCTL0 + 4*clk->clk_index;
	PSC_WRITE(sc, mdctl_reg, state);
	PSC_WRITE(sc, AM18XX_PSC_PTCMD, AM18XX_PSC_PTCMD_GO_ALL);

	/* wait for clock state transition to finish */
	while (PSC_READ(sc, AM18XX_PSC_PTSTAT) & AM18XX_PSC_PTSTAT_MASK)
		continue;
}

static struct clk *
am18xx_psc_clk_get(void *priv, const char *name)
{
	struct am18xx_psc_softc * const sc = priv;

	for (int i = 0; i < sc->sc_clknum; i++) {
		if (strcmp(sc->sc_clks[i].clk_base.name, name) == 0)
			return &sc->sc_clks[i].clk_base;
	}

	return NULL;
}

static u_int
am18xx_psc_clk_get_rate(void *priv, struct clk *clkp)
{
	struct am18xx_psc_clk *clk = (struct am18xx_psc_clk *)clkp;

	/* The PSC is only for clock gates, rates are passed through */
	return clk_get_rate(clk->u.clk_parent);
}

static int
am18xx_psc_clk_enable(void *priv, struct clk *clkp)
{
	struct am18xx_psc_softc * const sc = priv;
	struct am18xx_psc_clk *clk = (struct am18xx_psc_clk *)clkp;

	am18xx_psc_clk_transition(sc, clk, AM18XX_PSC_MDCTL_ENABLE);

	return 0;
}

static int
am18xx_psc_clk_disable(void *priv, struct clk *clkp)
{
	struct am18xx_psc_softc * const sc = priv;
	struct am18xx_psc_clk *clk = (struct am18xx_psc_clk *)clkp;

	am18xx_psc_clk_transition(sc, clk, AM18XX_PSC_MDCTL_DISABLE);

	return 0;
}

static struct clk *
am18xx_psc_clk_get_parent(void *priv, struct clk *clkp)
{
	struct am18xx_psc_clk *clk = (struct am18xx_psc_clk *)clkp;

	return clk->u.clk_parent;
}

static struct clk *
am18xx_psc_decode(device_t dev, int cc_phandle, const void *data, size_t len)
{
	struct am18xx_psc_softc * const sc = device_private(dev);
	const u_int *cells = data;

	if (len != 4)
		return NULL;
	const u_int clock_index = be32toh(cells[0]);

	if (clock_index >= sc->sc_clknum)
		return NULL;

	struct am18xx_psc_clk *clk = &sc->sc_clks[clock_index];

	return &clk->clk_base;
}

int
am18xx_psc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
am18xx_psc_attach(device_t parent, device_t self, void *aux)
{
	struct am18xx_psc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	const struct am18xx_psc_config *config;

	sc->sc_bst = faa->faa_bst;

	/* map PSC control registers */
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	/* initialize clock domain */
	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &am18xx_psc_clk_funcs;
	sc->sc_clkdom.priv = sc;

	/* wire up different clock tables depending on if it is psc0 or psc1 */
	config = of_compatible_lookup(phandle, compat_data)->data;
	sc->sc_clks = config->clks;
	sc->sc_clknum = config->clknum;

	/* get parent clock, then attach the clk gate  */
	for (int i = 0; i < sc->sc_clknum; i++) {
		/* skip unused clock gates */
		if (sc->sc_clks[i].clk_base.name == NULL)
			continue;

		struct clk *parent_clk =
		    fdtbus_clock_get(phandle, sc->sc_clks[i].u.clk_parent_name);
		if (parent_clk == NULL) {
			aprint_error(": couldn't get clock parent for %s\n",
				     sc->sc_clks[i].clk_base.name);
			return;
		}
		sc->sc_clks[i].u.clk_parent = parent_clk;
		sc->sc_clks[i].clk_base.domain = &sc->sc_clkdom;

		clk_attach(&sc->sc_clks[i].clk_base);
	}

	/* register it int he fdt subsystem */
	fdtbus_register_clock_controller(self, phandle,
					 &am18xx_psc_clk_fdt_funcs);

	aprint_normal("\n");
}

