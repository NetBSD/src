/* $NetBSD: ti_dpll_clock.c,v 1.6 2021/01/27 03:10:20 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ti_dpll_clock.c,v 1.6 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

#define	DPLL_MULT		__BITS(18,8)
#define	DPLL_DIV		__BITS(6,0)

#define	AM3_ST_MN_BYPASS	__BIT(8)
#define	AM3_ST_DPLL_CLK		__BIT(0)

#define	AM3_DPLL_EN		__BITS(2,0)
#define	 AM3_DPLL_EN_NM_BYPASS	4
#define	 AM3_DPLL_EN_LOCK	7

#define	OMAP3_ST_MPU_CLK	__BIT(0)

#define	OMAP3_EN_MPU_DPLL	__BITS(2,0)
#define	 OMAP3_EN_MPU_DPLL_BYPASS	5
#define	 OMAP3_EN_MPU_DPLL_LOCK		7

#define	OMAP3_CORE_DPLL_CLKOUT_DIV __BITS(31,27)
#define	OMAP3_CORE_DPLL_MULT	__BITS(26,16)
#define	OMAP3_CORE_DPLL_DIV	__BITS(14,8)

static int	ti_dpll_clock_match(device_t, cfdata_t, void *);
static void	ti_dpll_clock_attach(device_t, device_t, void *);

static struct clk *ti_dpll_clock_decode(device_t, int, const void *, size_t);

static const struct fdtbus_clock_controller_func ti_dpll_clock_fdt_funcs = {
	.decode = ti_dpll_clock_decode
};

static struct clk *ti_dpll_clock_get(void *, const char *);
static void	ti_dpll_clock_put(void *, struct clk *);
static u_int	ti_dpll_clock_get_rate(void *, struct clk *);
static struct clk *ti_dpll_clock_get_parent(void *, struct clk *);

static int	am3_dpll_clock_set_rate(void *, struct clk *, u_int);

static const struct clk_funcs am3_dpll_clock_clk_funcs = {
	.get = ti_dpll_clock_get,
	.put = ti_dpll_clock_put,
	.set_rate = am3_dpll_clock_set_rate,
	.get_rate = ti_dpll_clock_get_rate,
	.get_parent = ti_dpll_clock_get_parent,
};

static int	omap3_dpll_clock_set_rate(void *, struct clk *, u_int);

static const struct clk_funcs omap3_dpll_clock_clk_funcs = {
	.get = ti_dpll_clock_get,
	.put = ti_dpll_clock_put,
	.set_rate = omap3_dpll_clock_set_rate,
	.get_rate = ti_dpll_clock_get_rate,
	.get_parent = ti_dpll_clock_get_parent,
};

static u_int	omap3_dpll_core_clock_get_rate(void *, struct clk *);

static const struct clk_funcs omap3_dpll_core_clock_clk_funcs = {
	.get = ti_dpll_clock_get,
	.put = ti_dpll_clock_put,
	.get_rate = omap3_dpll_core_clock_get_rate,
	.get_parent = ti_dpll_clock_get_parent,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,am3-dpll-clock",
	  .data = &am3_dpll_clock_clk_funcs },
	{ .compat = "ti,omap3-dpll-clock",
	  .data = &omap3_dpll_clock_clk_funcs },
	{ .compat = "ti,omap3-dpll-core-clock",
	  .data = &omap3_dpll_core_clock_clk_funcs },

	DEVICE_COMPAT_EOL
};

enum {
	REG_CONTROL,
	REG_IDLEST,
	REG_MULT_DIV1,
	NREG
};

struct ti_dpll_clock_softc {
	device_t		sc_dev;
	int			sc_phandle;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh[NREG];

	struct clk_domain	sc_clkdom;
	struct clk		sc_clk;
};

#define	RD4(sc, space)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh[(space)], 0)
#define	WR4(sc, space, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh[(space)], 0, (val))

CFATTACH_DECL_NEW(ti_dpll_clock, sizeof(struct ti_dpll_clock_softc),
    ti_dpll_clock_match, ti_dpll_clock_attach, NULL, NULL);

static int
ti_dpll_clock_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
ti_dpll_clock_attach(device_t parent, device_t self, void *aux)
{
	struct ti_dpll_clock_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	const struct clk_funcs *clkfuncs;
	bus_addr_t addr[NREG], base_addr;
	u_int n;

	const int prcm_phandle = OF_parent(OF_parent(phandle));
	if (fdtbus_get_reg(prcm_phandle, 0, &base_addr, NULL) != 0) {
		aprint_error(": couldn't get prcm registers\n");
		return;
	}

	for (n = 0; n < NREG; n++) {
		if (fdtbus_get_reg(phandle, n, &addr[n], NULL) != 0) {
			aprint_error(": couldn't get registers\n");
			return;
		}
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	for (n = 0; n < NREG; n++) {
		if (bus_space_map(sc->sc_bst, base_addr + addr[n], 4, 0, &sc->sc_bsh[n]) != 0) {
			aprint_error(": couldn't map registers\n");
			return;
		}
	}

	clkfuncs = of_compatible_lookup(phandle, compat_data)->data;

	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = clkfuncs;
	sc->sc_clkdom.priv = sc;

	sc->sc_clk.domain = &sc->sc_clkdom;
	sc->sc_clk.name = kmem_asprintf("%s", faa->faa_name);
	clk_attach(&sc->sc_clk);

	aprint_naive("\n");
	aprint_normal(": TI DPLL clock (%s)\n", sc->sc_clk.name);

	fdtbus_register_clock_controller(self, phandle, &ti_dpll_clock_fdt_funcs);
}

static struct clk *
ti_dpll_clock_decode(device_t dev, int cc_phandle, const void *data,
		     size_t len)
{
	struct ti_dpll_clock_softc * const sc = device_private(dev);

	return &sc->sc_clk;
}

static struct clk *
ti_dpll_clock_get(void *priv, const char *name)
{
	struct ti_dpll_clock_softc * const sc = priv;

	return &sc->sc_clk;
}

static void
ti_dpll_clock_put(void *priv, struct clk *clk)
{
}

static u_int
ti_dpll_clock_get_rate(void *priv, struct clk *clk)
{
	struct ti_dpll_clock_softc * const sc = priv;
	struct clk *clk_parent = clk_get_parent(clk);
	uint32_t val;
	uint64_t parent_rate;

	if (clk_parent == NULL)
		return 0;

	val = RD4(sc, REG_MULT_DIV1);
	const u_int mult = __SHIFTOUT(val, DPLL_MULT);
	const u_int div = __SHIFTOUT(val, DPLL_DIV) + 1;

	parent_rate = clk_get_rate(clk_parent);

	return (u_int)((mult * parent_rate) / div);
}

static struct clk *
ti_dpll_clock_get_parent(void *priv, struct clk *clk)
{
	struct ti_dpll_clock_softc * const sc = priv;

	/* XXX assume ref clk */
	return fdtbus_clock_get_index(sc->sc_phandle, 0);
}

static int
am3_dpll_clock_set_rate(void *priv, struct clk *clk, u_int rate)
{
	struct ti_dpll_clock_softc * const sc = priv;
	struct clk *clk_parent = clk_get_parent(clk);
	uint64_t parent_rate;
	uint32_t control, mult_div1;

	if (clk_parent == NULL)
		return ENXIO;

	parent_rate = clk_get_rate(clk_parent);
	if (parent_rate == 0)
		return EIO;

	const u_int div = (parent_rate / 1000000) - 1;
	const u_int mult = rate / (parent_rate / (div + 1));
	if (mult < 2 || mult > 2047)
		return EINVAL;

	control = RD4(sc, REG_CONTROL);
	control &= ~AM3_DPLL_EN;
	control |= __SHIFTIN(AM3_DPLL_EN_NM_BYPASS, AM3_DPLL_EN);
	WR4(sc, REG_CONTROL, control);

	while (RD4(sc, REG_IDLEST) != AM3_ST_MN_BYPASS)
		;

	mult_div1 = __SHIFTIN(mult, DPLL_MULT);
	mult_div1 |= __SHIFTIN(div, DPLL_DIV);
	WR4(sc, REG_MULT_DIV1, mult_div1);

	control &= ~AM3_DPLL_EN;
	control |= __SHIFTIN(AM3_DPLL_EN_LOCK, AM3_DPLL_EN);
	WR4(sc, REG_CONTROL, control);

	while (RD4(sc, REG_IDLEST) != AM3_ST_DPLL_CLK)
		;    

	return 0;
}

static int
omap3_dpll_clock_set_rate(void *priv, struct clk *clk, u_int rate)
{
	struct ti_dpll_clock_softc * const sc = priv;
	struct clk *clk_parent = clk_get_parent(clk);
	uint64_t parent_rate;
	uint32_t control, mult_div1;

	if (clk_parent == NULL)
		return ENXIO;

	parent_rate = clk_get_rate(clk_parent);

	const u_int div = (parent_rate / 1000000) - 1;
	const u_int mult = rate / (parent_rate / (div + 1));
	if (mult < 2 || mult > 2047)
		return EINVAL;

	control = RD4(sc, REG_CONTROL);
	control &= ~OMAP3_EN_MPU_DPLL;
	control |= __SHIFTIN(OMAP3_EN_MPU_DPLL_BYPASS, OMAP3_EN_MPU_DPLL);
	WR4(sc, REG_CONTROL, control);

	delay(10);

	mult_div1 = __SHIFTIN(mult, DPLL_MULT);
	mult_div1 |= __SHIFTIN(div, DPLL_DIV);
	WR4(sc, REG_MULT_DIV1, mult_div1);

	control &= ~OMAP3_EN_MPU_DPLL;
	control |= __SHIFTIN(OMAP3_EN_MPU_DPLL_LOCK, OMAP3_EN_MPU_DPLL);
	WR4(sc, REG_CONTROL, control);

	while ((RD4(sc, REG_IDLEST) & OMAP3_ST_MPU_CLK) != 0)
		;    

	return 0;
}

static u_int
omap3_dpll_core_clock_get_rate(void *priv, struct clk *clk)
{
	struct ti_dpll_clock_softc * const sc = priv;
	struct clk *clk_parent = clk_get_parent(clk);
	uint32_t val;
	uint64_t parent_rate;

	if (clk_parent == NULL)
		return 0;

	val = RD4(sc, REG_MULT_DIV1);
	const u_int mult = __SHIFTOUT(val, OMAP3_CORE_DPLL_MULT);
	const u_int div = __SHIFTOUT(val, OMAP3_CORE_DPLL_DIV) + 1;
	const u_int postdiv = __SHIFTOUT(val, OMAP3_CORE_DPLL_CLKOUT_DIV);

	parent_rate = clk_get_rate(clk_parent);

	return (u_int)((mult * parent_rate) / div) / postdiv;
}
