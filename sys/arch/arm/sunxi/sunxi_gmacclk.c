/* $NetBSD: sunxi_gmacclk.c,v 1.1 2017/10/07 13:28:59 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_gmacclk.c,v 1.1 2017/10/07 13:28:59 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

#define	GMAC_CLK_PIT		__BIT(2)
#define	 GMAC_CLK_PIT_MII	0
#define	 GMAC_CLK_PIT_RGMII	1
#define	GMAC_CLK_SRC		__BITS(1,0)
#define	 GMAC_CLK_SRC_MII	0
#define	 GMAC_CLK_SRC_EXT_RGMII	1
#define	 GMAC_CLK_SRC_RGMII	2

static int	sunxi_gmacclk_match(device_t, cfdata_t, void *);
static void	sunxi_gmacclk_attach(device_t, device_t, void *);

static struct clk *sunxi_gmacclk_decode(device_t, const void *, size_t);

static const struct fdtbus_clock_controller_func sunxi_gmacclk_fdt_funcs = {
	.decode = sunxi_gmacclk_decode
};

static struct clk *sunxi_gmacclk_get(void *, const char *);
static void	sunxi_gmacclk_put(void *, struct clk *);
static int	sunxi_gmacclk_set_rate(void *, struct clk *, u_int);
static u_int	sunxi_gmacclk_get_rate(void *, struct clk *);
static struct clk *sunxi_gmacclk_get_parent(void *, struct clk *);

static const struct clk_funcs sunxi_gmacclk_clk_funcs = {
	.get = sunxi_gmacclk_get,
	.put = sunxi_gmacclk_put,
	.set_rate = sunxi_gmacclk_set_rate,
	.get_rate = sunxi_gmacclk_get_rate,
	.get_parent = sunxi_gmacclk_get_parent,
};

struct sunxi_gmacclk_softc {
	device_t		sc_dev;
	int			sc_phandle;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct clk_domain	sc_clkdom;
	struct clk		sc_clk;
	struct clk		*sc_parent[2];
};

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(sunxi_gmacclk, sizeof(struct sunxi_gmacclk_softc),
    sunxi_gmacclk_match, sunxi_gmacclk_attach, NULL, NULL);

static int
sunxi_gmacclk_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "allwinner,sun7i-a20-gmac-clk", NULL };
	const struct fdt_attach_args *faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sunxi_gmacclk_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_gmacclk_softc * const sc = device_private(self);
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
	sc->sc_parent[0] = fdtbus_clock_get_index(phandle, 0);
	sc->sc_parent[1] = fdtbus_clock_get_index(phandle, 1);
	if (sc->sc_parent[0] == NULL || sc->sc_parent[1] == NULL) {
		aprint_error(": couldn't get parent clocks\n");
		return;
	}

	sc->sc_clkdom.funcs = &sunxi_gmacclk_clk_funcs;
	sc->sc_clkdom.priv = sc;

	sc->sc_clk.domain = &sc->sc_clkdom;
	sc->sc_clk.name = kmem_asprintf("%s", faa->faa_name);

	aprint_naive("\n");
	aprint_normal(": GMAC MII/RGMII clock mux\n");

	fdtbus_register_clock_controller(self, phandle, &sunxi_gmacclk_fdt_funcs);
}

static struct clk *
sunxi_gmacclk_decode(device_t dev, const void *data, size_t len)
{
	struct sunxi_gmacclk_softc * const sc = device_private(dev);

	if (len != 0)
		return NULL;

	return &sc->sc_clk;
}

static struct clk *
sunxi_gmacclk_get(void *priv, const char *name)
{
	struct sunxi_gmacclk_softc * const sc = priv;

	if (strcmp(name, sc->sc_clk.name) != 0)
		return NULL;

	return &sc->sc_clk;
}

static void
sunxi_gmacclk_put(void *priv, struct clk *clk)
{
}

static int
sunxi_gmacclk_set_rate(void *priv, struct clk *clk, u_int rate)
{
	struct sunxi_gmacclk_softc * const sc = priv;
	uint32_t val;

	val = RD4(sc, 0);
	val &= ~(GMAC_CLK_PIT|GMAC_CLK_SRC);
	if (rate == clk_get_rate(sc->sc_parent[GMAC_CLK_PIT_MII])) {
		/* MII clock */
		val |= __SHIFTIN(GMAC_CLK_PIT_MII, GMAC_CLK_PIT);
		val |= __SHIFTIN(GMAC_CLK_SRC_MII, GMAC_CLK_SRC);
	} else if (rate == clk_get_rate(sc->sc_parent[GMAC_CLK_PIT_RGMII])) {
		/* RGMII clock */
		val |= __SHIFTIN(GMAC_CLK_PIT_RGMII, GMAC_CLK_PIT);
		val |= __SHIFTIN(GMAC_CLK_SRC_RGMII, GMAC_CLK_SRC);
	} else {
		return ENXIO;
	}
	WR4(sc, 0, val);

	return 0;
}

static u_int
sunxi_gmacclk_get_rate(void *priv, struct clk *clk)
{
	struct clk *clk_parent = clk_get_parent(clk);

	return clk_get_rate(clk_parent);
}

static struct clk *
sunxi_gmacclk_get_parent(void *priv, struct clk *clk)
{
	struct sunxi_gmacclk_softc * const sc = priv;
	uint32_t val;
	u_int sel;

	val = RD4(sc, 0);
	sel = __SHIFTOUT(val, GMAC_CLK_PIT);

	return sc->sc_parent[sel];
}
