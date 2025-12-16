/* $NetBSD: ti_gate_clock.c,v 1.1 2025/12/16 12:20:22 skrll Exp $ */

/*-
 * Copyright (c) 2025 Rui-Xiang Guo
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
__KERNEL_RCSID(0, "$NetBSD: ti_gate_clock.c,v 1.1 2025/12/16 12:20:22 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

static int ti_gate_clock_match(device_t, cfdata_t, void *);
static void ti_gate_clock_attach(device_t, device_t, void *);

static struct clk *ti_gate_clock_decode(device_t, int, const void *, size_t);

static const struct fdtbus_clock_controller_func ti_gate_clock_fdt_funcs = {
	.decode = ti_gate_clock_decode
};

static struct clk *ti_gate_clock_get(void *, const char *);
static void ti_gate_clock_put(void *, struct clk *);
static int ti_gate_clock_enable(void *, struct clk *);
static int ti_gate_clock_disable(void *, struct clk *);
static struct clk *ti_gate_clock_get_parent(void *, struct clk *);

static const struct clk_funcs ti_gate_clock_clk_funcs = {
	.get = ti_gate_clock_get,
	.put = ti_gate_clock_put,
	.enable = ti_gate_clock_enable,
	.disable = ti_gate_clock_disable,
	.get_parent = ti_gate_clock_get_parent,
};

struct ti_gate_clock_softc {
	device_t		sc_dev;
	int			sc_phandle;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct clk_domain	sc_clkdom;
	struct clk		sc_clk;

	uint32_t		sc_shift;
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(ti_gate_clock, sizeof(struct ti_gate_clock_softc),
    ti_gate_clock_match, ti_gate_clock_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,composite-no-wait-gate-clock" },
	{ .compat = "ti,gate-clock" },
	DEVICE_COMPAT_EOL
};

static int
ti_gate_clock_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
ti_gate_clock_attach(device_t parent, device_t self, void *aux)
{
	struct ti_gate_clock_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr, base_addr;
	u_int shift;

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

	if (of_getprop_uint32(phandle, "ti,bit-shift", &shift) != 0)
		shift = 0;

	sc->sc_shift = shift;

	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &ti_gate_clock_clk_funcs;
	sc->sc_clkdom.priv = sc;

	sc->sc_clk.domain = &sc->sc_clkdom;
	sc->sc_clk.name = kmem_asprintf("%s", faa->faa_name);
	clk_attach(&sc->sc_clk);

	aprint_naive("\n");
	aprint_normal(": TI gate clock (%s)\n", sc->sc_clk.name);

	fdtbus_register_clock_controller(self, phandle, &ti_gate_clock_fdt_funcs);
}

static struct clk *
ti_gate_clock_decode(device_t dev, int cc_phandle, const void *data,
		     size_t len)
{
	struct ti_gate_clock_softc * const sc = device_private(dev);

	return &sc->sc_clk;
}

static struct clk *
ti_gate_clock_get(void *priv, const char *name)
{
	struct ti_gate_clock_softc * const sc = priv;

	return &sc->sc_clk;
}

static void
ti_gate_clock_put(void *priv, struct clk *clk)
{
}

static int
ti_gate_clock_enable(void *priv, struct clk *clk)
{
	struct ti_gate_clock_softc * const sc = priv;
	struct clk *clk_parent = clk_get_parent(clk);
	uint32_t val;

	val = RD4(sc, 0);
	val |= __BIT(sc->sc_shift);
	WR4(sc, 0, val);

	return clk_enable(clk_parent);
}

static int
ti_gate_clock_disable(void *priv, struct clk *clk)
{
	struct ti_gate_clock_softc * const sc = priv;
	struct clk *clk_parent = clk_get_parent(clk);
	uint32_t val;

	val = RD4(sc, 0);
	val &= ~__BIT(sc->sc_shift);
	WR4(sc, 0, val);

	return clk_disable(clk_parent);
}

static struct clk *
ti_gate_clock_get_parent(void *priv, struct clk *clk)
{
	struct ti_gate_clock_softc * const sc = priv;

	return fdtbus_clock_get_index(sc->sc_phandle, 0);
}
