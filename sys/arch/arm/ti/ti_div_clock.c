/* $NetBSD: ti_div_clock.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ti_div_clock.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,divider-clock" },
	DEVICE_COMPAT_EOL
};

static int	ti_div_clock_match(device_t, cfdata_t, void *);
static void	ti_div_clock_attach(device_t, device_t, void *);

static struct clk *ti_div_clock_decode(device_t, int, const void *, size_t);

static const struct fdtbus_clock_controller_func ti_div_clock_fdt_funcs = {
	.decode = ti_div_clock_decode
};

static struct clk *ti_div_clock_get(void *, const char *);
static void	ti_div_clock_put(void *, struct clk *);
static u_int	ti_div_clock_get_rate(void *, struct clk *);
static struct clk *ti_div_clock_get_parent(void *, struct clk *);

static const struct clk_funcs ti_div_clock_clk_funcs = {
	.get = ti_div_clock_get,
	.put = ti_div_clock_put,
	.get_rate = ti_div_clock_get_rate,
	.get_parent = ti_div_clock_get_parent,
};

struct ti_div_clock_softc {
	device_t		sc_dev;
	int			sc_phandle;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct clk_domain	sc_clkdom;
	struct clk		sc_clk;
};

#define	RD4(sc)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, 0)
#define	WR4(sc, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, 0, (val))

CFATTACH_DECL_NEW(ti_div_clock, sizeof(struct ti_div_clock_softc),
    ti_div_clock_match, ti_div_clock_attach, NULL, NULL);

static int
ti_div_clock_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
ti_div_clock_attach(device_t parent, device_t self, void *aux)
{
	struct ti_div_clock_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr, base_addr;

	const int prcm_phandle = OF_parent(OF_parent(phandle));
	if (fdtbus_get_reg(phandle, 0, &addr, NULL) != 0 ||
	    fdtbus_get_reg(prcm_phandle, 0, &base_addr, NULL) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, base_addr + addr, 4, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &ti_div_clock_clk_funcs;
	sc->sc_clkdom.priv = sc;

	sc->sc_clk.domain = &sc->sc_clkdom;
	sc->sc_clk.name = kmem_asprintf("%s", faa->faa_name);
	clk_attach(&sc->sc_clk);

	aprint_naive("\n");
	aprint_normal(": TI divider clock (%s)\n", sc->sc_clk.name);

	fdtbus_register_clock_controller(self, phandle, &ti_div_clock_fdt_funcs);
}

static struct clk *
ti_div_clock_decode(device_t dev, int cc_phandle, const void *data,
		     size_t len)
{
	struct ti_div_clock_softc * const sc = device_private(dev);

	return &sc->sc_clk;
}

static struct clk *
ti_div_clock_get(void *priv, const char *name)
{
	struct ti_div_clock_softc * const sc = priv;

	return &sc->sc_clk;
}

static void
ti_div_clock_put(void *priv, struct clk *clk)
{
}

static u_int
ti_div_clock_get_rate(void *priv, struct clk *clk)
{
	struct ti_div_clock_softc * const sc = priv;
	struct clk *clk_parent = clk_get_parent(clk);
	uint64_t parent_rate;
	uint32_t val, max_val, mask;
	u_int div, max_div, bit_shift;

	if (clk_parent == NULL)
		return 0;

	if (of_hasprop(sc->sc_phandle, "ti,index-power-of-two"))
		return EINVAL;	/* not supported yet */

	const int start_index = of_hasprop(sc->sc_phandle, "ti,index-starts-at-one") ? 1 : 0;

	if (of_getprop_uint32(sc->sc_phandle, "ti,bit-shift", &bit_shift) != 0)
		bit_shift = 0;

	if (of_getprop_uint32(sc->sc_phandle, "ti,max-div", &max_div) == 0) {
		max_val = start_index + max_div;
		while (!powerof2(max_val))
			max_val++;
		mask = (max_val - 1) << bit_shift;
	} else {
		/*
		 * The bindings allow for this, but it is not yet supported
		 * by this driver.
		 */
		return EINVAL;
	}

	val = RD4(sc);
	div = __SHIFTOUT(val, mask) + (start_index ? 0 : 1);
	if (div == 0)
		return EINVAL;

	parent_rate = clk_get_rate(clk_parent);

	return (u_int)(parent_rate / div);
}

static struct clk *
ti_div_clock_get_parent(void *priv, struct clk *clk)
{
	struct ti_div_clock_softc * const sc = priv;

	return fdtbus_clock_get_index(sc->sc_phandle, 0);
}
