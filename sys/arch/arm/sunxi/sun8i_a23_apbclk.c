/* $NetBSD: sun8i_a23_apbclk.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sun8i_a23_apbclk.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

#define	APB0_DIV	__BITS(1,0)

static int	sun8i_a23_apbclk_match(device_t, cfdata_t, void *);
static void	sun8i_a23_apbclk_attach(device_t, device_t, void *);

static struct clk *sun8i_a23_apbclk_decode(device_t, int, const void *, size_t);

static const struct fdtbus_clock_controller_func sun8i_a23_apbclk_fdt_funcs = {
	.decode = sun8i_a23_apbclk_decode
};

static struct clk *sun8i_a23_apbclk_get(void *, const char *);
static void	sun8i_a23_apbclk_put(void *, struct clk *);
static int	sun8i_a23_apbclk_set_rate(void *, struct clk *, u_int);
static u_int	sun8i_a23_apbclk_get_rate(void *, struct clk *);
static struct clk *sun8i_a23_apbclk_get_parent(void *, struct clk *);

static const struct clk_funcs sun8i_a23_apbclk_clk_funcs = {
	.get = sun8i_a23_apbclk_get,
	.put = sun8i_a23_apbclk_put,
	.set_rate = sun8i_a23_apbclk_set_rate,
	.get_rate = sun8i_a23_apbclk_get_rate,
	.get_parent = sun8i_a23_apbclk_get_parent,
};

struct sun8i_a23_apbclk_softc {
	device_t		sc_dev;
	int			sc_phandle;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct clk_domain	sc_clkdom;
	struct clk		sc_clk;
	struct clk		*sc_parent;
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(sunxi_a23_apbclk, sizeof(struct sun8i_a23_apbclk_softc),
    sun8i_a23_apbclk_match, sun8i_a23_apbclk_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun8i-a23-apb0-clk" },
	DEVICE_COMPAT_EOL
};

static int
sun8i_a23_apbclk_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sun8i_a23_apbclk_attach(device_t parent, device_t self, void *aux)
{
	struct sun8i_a23_apbclk_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_parent = fdtbus_clock_get_index(phandle, 0);

	sc->sc_clkdom.funcs = &sun8i_a23_apbclk_clk_funcs;
	sc->sc_clkdom.priv = sc;

	sc->sc_clk.domain = &sc->sc_clkdom;
	sc->sc_clk.name = kmem_asprintf("%s", faa->faa_name);

	aprint_naive("\n");
	aprint_normal(": A23 APB0 clock\n");

	fdtbus_register_clock_controller(self, phandle, &sun8i_a23_apbclk_fdt_funcs);
}

static struct clk *
sun8i_a23_apbclk_decode(device_t dev, int cc_phandle, const void *data,
		     size_t len)
{
	struct sun8i_a23_apbclk_softc * const sc = device_private(dev);

	if (len != 0)
		return NULL;

	return &sc->sc_clk;
}

static struct clk *
sun8i_a23_apbclk_get(void *priv, const char *name)
{
	struct sun8i_a23_apbclk_softc * const sc = priv;

	if (strcmp(name, sc->sc_clk.name) != 0)
		return NULL;

	return &sc->sc_clk;
}

static void
sun8i_a23_apbclk_put(void *priv, struct clk *clk)
{
}

static int
sun8i_a23_apbclk_set_rate(void *priv, struct clk *clk, u_int rate)
{
	return ENXIO;
}

static u_int
sun8i_a23_apbclk_get_rate(void *priv, struct clk *clk)
{
	struct sun8i_a23_apbclk_softc * const sc = priv;
	struct clk *clk_parent = clk_get_parent(clk);

	const uint32_t val = RD4(sc, 0);
	const u_int div = __SHIFTOUT(val, APB0_DIV);

	return clk_get_rate(clk_parent) / (div + 1);
}

static struct clk *
sun8i_a23_apbclk_get_parent(void *priv, struct clk *clk)
{
	struct sun8i_a23_apbclk_softc * const sc = priv;

	return sc->sc_parent;
}
