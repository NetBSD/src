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
 * Drivers for the async clock gates in the syscfg block of the TI AM1808.
 */

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/device.h>

#include <dev/clk/clk_backend.h>
#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#include <arm/fdt/arm_fdtvar.h>

struct am18xx_aclk_config {
	struct clk *clk; /* double wrap clk because the config must be const */
	uint32_t mask;
};

struct am18xx_aclk_softc {
	struct clk_domain sc_clkdom;
	const struct am18xx_aclk_config *sc_config;
	struct clk *sc_parent_clk;
};

static int	am18xx_aclk_match(device_t, cfdata_t, void *);
static void	am18xx_aclk_attach(device_t, device_t, void *);
static struct clk * am18xx_aclk_decode(device_t, int, const void *, size_t);
static struct clk * am18xx_aclk_clk_get(void *, const char *);
static u_int am18xx_aclk_clk_get_rate(void *, struct clk *);
static struct clk * am18xx_aclk_clk_get_parent(void *, struct clk *);

CFATTACH_DECL_NEW(am18xxaclk, sizeof(struct am18xx_aclk_softc),
		  am18xx_aclk_match, am18xx_aclk_attach, NULL, NULL);

#define AM18XX_ACLK_CFGCHIP3 0xC
#define AM18XX_ACLK_CFGCHIP3_ASYNC3_CLKSRC __BIT(4)
#define AM18XX_ACLK_CFGCHIP3_ASYNC1_CLKSRC __BIT(2)

static struct clk am18xx_aclk_async1_clk = {
	.name = "async1",
	.flags = 0
};
static struct clk am18xx_aclk_async3_clk = {
	.name = "async3",
	.flags = 0
};

static const struct am18xx_aclk_config am18xx_aclk_async1_config = {
	.clk = &am18xx_aclk_async1_clk,
	.mask = AM18XX_ACLK_CFGCHIP3_ASYNC1_CLKSRC,
};
static const struct am18xx_aclk_config am18xx_aclk_async3_config = {
	.clk = &am18xx_aclk_async3_clk,
	.mask = AM18XX_ACLK_CFGCHIP3_ASYNC3_CLKSRC,
};

static const struct fdtbus_clock_controller_func am18xx_aclk_clk_fdt_funcs = {
	.decode = am18xx_aclk_decode,
};

static const struct clk_funcs am18xx_aclk_clk_funcs = {
	.get = am18xx_aclk_clk_get,
	.get_rate = am18xx_aclk_clk_get_rate,
	.get_parent = am18xx_aclk_clk_get_parent,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,da850-async1-clksrc",
	  .data = &am18xx_aclk_async1_config },
	{ .compat = "ti,da850-async3-clksrc",
	  .data = &am18xx_aclk_async3_config },
	DEVICE_COMPAT_EOL
};

static struct clk *
am18xx_aclk_clk_get(void *priv, const char *name)
{
	struct am18xx_aclk_softc * const sc = priv;

	if (strcmp(sc->sc_config->clk->name, name) == 0) {
		return sc->sc_config->clk;
	}

	return NULL;
}

static u_int
am18xx_aclk_clk_get_rate(void *priv, struct clk *clkp)
{
	struct am18xx_aclk_softc * const sc = priv;

	return clk_get_rate(sc->sc_parent_clk);
}

static struct clk *
am18xx_aclk_clk_get_parent(void *priv, struct clk *clkp)
{
	struct am18xx_aclk_softc * const sc = priv;

	return sc->sc_parent_clk;
}

static struct clk *
am18xx_aclk_decode(device_t dev, int cc_phandle, const void *data, size_t len)
{
	struct am18xx_aclk_softc * const sc = device_private(dev);

	return sc->sc_config->clk;
}

int
am18xx_aclk_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
am18xx_aclk_attach(device_t parent, device_t self, void *aux)
{
	struct am18xx_aclk_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_config = of_compatible_lookup(phandle, compat_data)->data;

	/* ensure we have a clock parent */
	sc->sc_parent_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_parent_clk == NULL) {
		aprint_error(": couldn't get parent clock");
		return;
	}

	/* ensure clock gate bits are as expected */
	struct syscon *syscon = fdtbus_syscon_lookup(OF_parent(phandle));
	if (syscon == NULL) {
		aprint_error(": couldn't get syscon registers\n");
		return;
	}
	syscon_lock(syscon);
	uint32_t cfgchip_reg3 = syscon_read_4(syscon, AM18XX_ACLK_CFGCHIP3);
	syscon_unlock(syscon);
	if ((cfgchip_reg3 & sc->sc_config->mask) != 0) {
		aprint_error(": unexpected clock gate bits\n");
		return;
	}

	/* attach a clock controller */
	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &am18xx_aclk_clk_funcs;
	sc->sc_clkdom.priv = sc;

	sc->sc_config->clk->domain = &sc->sc_clkdom;
	clk_attach(sc->sc_config->clk);

	fdtbus_register_clock_controller(self, phandle, &am18xx_aclk_clk_fdt_funcs);

	aprint_normal("\n");
}
