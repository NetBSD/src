/* $NetBSD $ */

/*-
* Copyright (c) 2026 The NetBSD Foundation, Inc.
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
 * PLL Controller for the TI AM18XX SOC.
 */

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/device.h>

#include <dev/clk/clk_backend.h>
#include <dev/fdt/fdtvar.h>

#include <arm/fdt/arm_fdtvar.h>

struct am18xx_pllc_softc;

struct am18xx_pllc_clk {
	struct clk clk_base; /* must be first */
	u_int (*get_rate)(struct am18xx_pllc_softc *, struct am18xx_pllc_clk *);
	u_int clk_index;
};

struct am18xx_pllc_config {
	struct am18xx_pllc_clk *sysclks;
	struct am18xx_pllc_clk *auxclk;
	int num_sysclk;
};

struct am18xx_pllc_softc {
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int sc_sysclk_phandle;
	int sc_auxclk_phandle;
	const struct am18xx_pllc_config *sc_config;
	struct clk_domain sc_clkdom;
	struct clk *sc_ref_clk;
};

static int	am18xx_pllc_match(device_t, cfdata_t, void *);
static void	am18xx_pllc_attach(device_t, device_t, void *);
static u_int	am18xx_pllc_get_sysclk_rate(struct am18xx_pllc_softc *,
					    struct am18xx_pllc_clk *);
static u_int	am18xx_pllc_get_auxclk_rate(struct am18xx_pllc_softc *,
					    struct am18xx_pllc_clk *);
static struct clk * am18xx_pllc_decode(device_t, int, const void *, size_t);
static struct clk *	am18xx_pllc_clk_get(void *, const char *);
static u_int		am18xx_pllc_clk_get_rate(void *, struct clk *);
static struct clk *	am18xx_pllc_clk_get_parent(void *, struct clk *);

CFATTACH_DECL_NEW(am18xxpllc, sizeof(struct am18xx_pllc_softc),
		  am18xx_pllc_match, am18xx_pllc_attach, NULL, NULL);

#define AM18XX_PLLC_PLLCTL 0x100
#define AM18XX_PLLC_PLLM 0x110
#define AM18XX_PLLC_PREDIV 0x114
#define AM18XX_PLLC_PLLDIV1 0x118
#define AM18XX_PLLC_POSTDIV 0x128
#define AM18XX_PLLC_PLLDIV4 0x160

#define AM18XX_PLLC_PLLCTL_PLLEN __BIT(0)
#define AM18XX_PLLC_PLLCTL_EXTCLKSRC __BIT(9)
#define AM18XX_PLLC_PLLM_MULTIPLIER __BITS(4,0)
#define AM18XX_PLLC_PREDIV_RATIO __BITS(4,0)
#define AM18XX_PLLC_POSTDIV_RATIO __BITS(4,0)
#define AM18XX_PLLC_PLLDIV_RATIO __BITS(4,0)

#define	PLLC_READ(sc, reg)					\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, reg)
#define	PLLC_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, reg, val)

#define PLLC_CLK(_i, _name, _rate) 					\
	{								\
		.clk_base.name = (_name),				\
		.clk_base.flags = 0,					\
		.get_rate = (_rate),					\
		.clk_index = (_i),					\
	}

static struct am18xx_pllc_clk am18xx_pllc_pll0_auxclk =
	PLLC_CLK(0, "pll0_auxclk", &am18xx_pllc_get_auxclk_rate);

static struct am18xx_pllc_clk am18xx_pllc_pll0_sysclks[] = {
	PLLC_CLK(0, "pll0_sysclk1", &am18xx_pllc_get_sysclk_rate),
	PLLC_CLK(1, "pll0_sysclk2", &am18xx_pllc_get_sysclk_rate),
	PLLC_CLK(2, "pll0_sysclk3", &am18xx_pllc_get_sysclk_rate),
	PLLC_CLK(3, "pll0_sysclk4", &am18xx_pllc_get_sysclk_rate),
	PLLC_CLK(4, "pll0_sysclk5", &am18xx_pllc_get_sysclk_rate),
	PLLC_CLK(5, "pll0_sysclk6", &am18xx_pllc_get_sysclk_rate),
	PLLC_CLK(6, "pll0_sysclk7", &am18xx_pllc_get_sysclk_rate),
};

static const struct am18xx_pllc_config am18xx_pllc_pll0_config = {
	.auxclk = &am18xx_pllc_pll0_auxclk,
	.sysclks = am18xx_pllc_pll0_sysclks,
	.num_sysclk = __arraycount(am18xx_pllc_pll0_sysclks),
};

static const struct fdtbus_clock_controller_func am18xx_pllc_clk_fdt_funcs = {
	.decode = am18xx_pllc_decode,
};

static const struct clk_funcs am18xx_pllc_clk_funcs = {
	.get = am18xx_pllc_clk_get,
	.get_rate = am18xx_pllc_clk_get_rate,
	.get_parent = am18xx_pllc_clk_get_parent,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,da850-pll0", .data = &am18xx_pllc_pll0_config },
	DEVICE_COMPAT_EOL
};

static struct clk *
am18xx_pllc_clk_get(void *priv, const char *name)
{
	struct am18xx_pllc_softc * const sc = priv;

	/* check if it is the auxclk */
	if (strcmp(sc->sc_config->auxclk->clk_base.name, name) == 0) {
		return &sc->sc_config->auxclk->clk_base;
	}

	/* check if it is a sysclk */
	for (int i = 0; i < sc->sc_config->num_sysclk; i++) {
		if (strcmp(sc->sc_config->sysclks[i].clk_base.name, name) == 0)
			return &sc->sc_config->sysclks[i].clk_base;
	}

	return NULL;
}

static u_int
am18xx_pllc_clk_get_rate(void *priv, struct clk *clkp)
{
	struct am18xx_pllc_softc * const sc = priv;
	struct am18xx_pllc_clk *clk = (struct am18xx_pllc_clk *)clkp;

	return clk->get_rate(sc, clk);
}

static struct clk *
am18xx_pllc_clk_get_parent(void *priv, struct clk *clkp)
{
	struct am18xx_pllc_softc * const sc = priv;

	return sc->sc_ref_clk;
}

static u_int
am18xx_pllc_get_sysclk_rate(struct am18xx_pllc_softc *sc,
			    struct am18xx_pllc_clk *clk)
{
	uint32_t pllctl_reg = PLLC_READ(sc, AM18XX_PLLC_PLLCTL);

	uint32_t prediv_reg = PLLC_READ(sc, AM18XX_PLLC_PREDIV);
	uint32_t prediv_ratio = (prediv_reg & AM18XX_PLLC_PREDIV_RATIO) + 1;

	uint32_t pllm_reg = PLLC_READ(sc, AM18XX_PLLC_PLLM);
	uint32_t pllm_multiplier = (pllm_reg & AM18XX_PLLC_PLLM_MULTIPLIER) + 1;

	uint32_t postdiv_reg = PLLC_READ(sc, AM18XX_PLLC_POSTDIV);
	uint32_t postdiv_ratio = (postdiv_reg & AM18XX_PLLC_POSTDIV_RATIO) + 1;

	uint32_t plldiv_regaddr;
	if (clk->clk_index <= 2) {
		plldiv_regaddr = AM18XX_PLLC_PLLDIV1 + 4 * clk->clk_index;
	} else {
		plldiv_regaddr = AM18XX_PLLC_PLLDIV4 + 4 * (clk->clk_index - 3);
	}
	uint32_t plldiv_reg = PLLC_READ(sc, plldiv_regaddr);
	uint32_t plldiv_ratio = (plldiv_reg & AM18XX_PLLC_PLLDIV_RATIO) + 1;

	u_int ref_clk_rate = clk_get_rate(sc->sc_ref_clk);

	if (pllctl_reg & AM18XX_PLLC_PLLCTL_PLLEN) {
		/* PLL enabled */
		ref_clk_rate /= prediv_ratio;
		ref_clk_rate *= pllm_multiplier;
		ref_clk_rate /= postdiv_ratio;
	} else {
		/* bypass mode (ensure we aren't using the other PLL)*/
		KASSERT((pllctl_reg & AM18XX_PLLC_PLLCTL_EXTCLKSRC) == 0);
	}

	ref_clk_rate /= plldiv_ratio;

	return ref_clk_rate;
}

static u_int
am18xx_pllc_get_auxclk_rate(struct am18xx_pllc_softc *sc,
			    struct am18xx_pllc_clk *clk)
{
	return clk_get_rate(sc->sc_ref_clk);
}

static struct clk *
am18xx_pllc_decode(device_t dev, int cc_phandle, const void *data, size_t len)
{
	struct am18xx_pllc_softc * const sc = device_private(dev);
	const u_int *cells = data;

	if (cc_phandle == sc->sc_sysclk_phandle) {
		if (len != 4)
			return NULL;
		const u_int clock_index = be32toh(cells[0]) - 1;
		if (clock_index >= sc->sc_config->num_sysclk) {
			return NULL;
		}

		return &sc->sc_config->sysclks[clock_index].clk_base;
	} else if (cc_phandle == sc->sc_auxclk_phandle) {
		return &sc->sc_config->auxclk->clk_base;
	}

	return NULL;
}

int
am18xx_pllc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
am18xx_pllc_attach(device_t parent, device_t self, void *aux)
{
	struct am18xx_pllc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_bst = faa->faa_bst;
	sc->sc_config = of_compatible_lookup(phandle, compat_data)->data;

	/* map PSC control registers */
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	/* get parent clock */
	sc->sc_ref_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_ref_clk == NULL) {
		aprint_error(": couldn't get reference clock\n");
		return;
	}

	/* initialize clock domain */
	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &am18xx_pllc_clk_funcs;
	sc->sc_clkdom.priv = sc;

	/* initialize the clocks */
	sc->sc_config->auxclk->clk_base.domain = &sc->sc_clkdom;
	clk_attach(&sc->sc_config->auxclk->clk_base);
	for (int i = 0; i < sc->sc_config->num_sysclk; i++) {
		sc->sc_config->sysclks[i].clk_base.domain = &sc->sc_clkdom;
		clk_attach(&sc->sc_config->sysclks[i].clk_base);
	}

	/* register auxclk fdt controller*/
	sc->sc_auxclk_phandle = of_find_firstchild_byname(phandle, "auxclk");
	if (sc->sc_auxclk_phandle < 0) {
		aprint_error(": couldn't get pll0_auxclk child\n");
		return;
	}
	fdtbus_register_clock_controller(self, sc->sc_auxclk_phandle,
					 &am18xx_pllc_clk_fdt_funcs);

	/* register sysclk fdt controller*/
	sc->sc_sysclk_phandle = of_find_firstchild_byname(phandle, "sysclk");
	if (sc->sc_sysclk_phandle < 0) {
		aprint_error(": couldn't get pll0_sysclk child\n");
		return;
	}
	fdtbus_register_clock_controller(self, sc->sc_sysclk_phandle,
					 &am18xx_pllc_clk_fdt_funcs);

	aprint_normal("\n");
}

