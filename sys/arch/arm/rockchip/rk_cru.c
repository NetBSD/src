/* $NetBSD: rk_cru.c,v 1.6 2018/08/12 19:48:36 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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

#include "opt_soc.h"
#include "opt_fdt_arm.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rk_cru.c,v 1.6 2018/08/12 19:48:36 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <dev/clk/clk_backend.h>

#include <arm/rockchip/rk_cru.h>

static void *
rk_cru_reset_acquire(device_t dev, const void *data, size_t len)
{
	if (len != 4)
		return NULL;

	return (void *)(uintptr_t)be32dec(data);
}

static void
rk_cru_reset_release(device_t dev, void *priv)
{
}

static int
rk_cru_reset_assert(device_t dev, void *priv)
{
	struct rk_cru_softc * const sc = device_private(dev);
	const uintptr_t reset_id = (uintptr_t)priv;
	const bus_size_t reg = sc->sc_softrst_base + (reset_id / 16) * 4;
	const u_int shift = reset_id % 16;

	CRU_WRITE(sc, reg, (1 << (shift + 16)) | (1 << shift));

	return 0;
}

static int
rk_cru_reset_deassert(device_t dev, void *priv)
{
	struct rk_cru_softc * const sc = device_private(dev);
	const uintptr_t reset_id = (uintptr_t)priv;
	const bus_size_t reg = sc->sc_softrst_base + (reset_id / 16) * 4;
	const u_int shift = reset_id % 16;

	CRU_WRITE(sc, reg, (1 << (shift + 16)) | (0 << shift));

	return 0;
}

static const struct fdtbus_reset_controller_func rk_cru_fdtreset_funcs = {
	.acquire = rk_cru_reset_acquire,
	.release = rk_cru_reset_release,
	.reset_assert = rk_cru_reset_assert,
	.reset_deassert = rk_cru_reset_deassert,
};

static struct clk *
rk_cru_clock_decode(device_t dev, const void *data, size_t len)
{
	struct rk_cru_softc * const sc = device_private(dev);
	struct rk_cru_clk *clk;

	if (len != 4)
		return NULL;

	const u_int clock_id = be32dec(data);

	for (int i = 0; i < sc->sc_nclks; i++) {
		clk = &sc->sc_clks[i];
		if (clk->id == clock_id)
			return &clk->base;
	}

	return NULL;
}

static const struct fdtbus_clock_controller_func rk_cru_fdtclock_funcs = {
	.decode = rk_cru_clock_decode,
};

static struct clk *
rk_cru_clock_get(void *priv, const char *name)
{
	struct rk_cru_softc * const sc = priv;
	struct rk_cru_clk *clk;

	clk = rk_cru_clock_find(sc, name);
	if (clk == NULL)
		return NULL;

	return &clk->base;
}

static void
rk_cru_clock_put(void *priv, struct clk *clk)
{
}

static u_int
rk_cru_clock_get_rate(void *priv, struct clk *clkp)
{
	struct rk_cru_softc * const sc = priv;
	struct rk_cru_clk *clk = (struct rk_cru_clk *)clkp;
	struct clk *clkp_parent;

	if (clk->get_rate)
		return clk->get_rate(sc, clk);

	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL) {
		aprint_error("%s: no parent for %s\n", __func__, clk->base.name);
		return 0;
	}

	return clk_get_rate(clkp_parent);
}

static int
rk_cru_clock_set_rate(void *priv, struct clk *clkp, u_int rate)
{
	struct rk_cru_softc * const sc = priv;
	struct rk_cru_clk *clk = (struct rk_cru_clk *)clkp;
	struct clk *clkp_parent;

	if (clkp->flags & CLK_SET_RATE_PARENT) {
		clkp_parent = clk_get_parent(clkp);
		if (clkp_parent == NULL) {
			aprint_error("%s: no parent for %s\n", __func__, clk->base.name);
			return ENXIO;
		}
		return clk_set_rate(clkp_parent, rate);
	}

	if (clk->set_rate)
		return clk->set_rate(sc, clk, rate);

	return ENXIO;
}

static u_int
rk_cru_clock_round_rate(void *priv, struct clk *clkp, u_int rate)
{
	struct rk_cru_softc * const sc = priv;
	struct rk_cru_clk *clk = (struct rk_cru_clk *)clkp;
	struct clk *clkp_parent;

	if (clkp->flags & CLK_SET_RATE_PARENT) {
		clkp_parent = clk_get_parent(clkp);
		if (clkp_parent == NULL) {
			aprint_error("%s: no parent for %s\n", __func__, clk->base.name);
			return 0;
		}
		return clk_round_rate(clkp_parent, rate);
	}

	if (clk->round_rate)
		return clk->round_rate(sc, clk, rate);

	return 0;
}

static int
rk_cru_clock_enable(void *priv, struct clk *clkp)
{
	struct rk_cru_softc * const sc = priv;
	struct rk_cru_clk *clk = (struct rk_cru_clk *)clkp;
	struct clk *clkp_parent;
	int error = 0;

	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent != NULL) {
		error = clk_enable(clkp_parent);
		if (error != 0)
			return error;
	}

	if (clk->enable)
		error = clk->enable(sc, clk, 1);

	return error;
}

static int
rk_cru_clock_disable(void *priv, struct clk *clkp)
{
	struct rk_cru_softc * const sc = priv;
	struct rk_cru_clk *clk = (struct rk_cru_clk *)clkp;
	int error = EINVAL;

	if (clk->enable)
		error = clk->enable(sc, clk, 0);

	return error;
}

static int
rk_cru_clock_set_parent(void *priv, struct clk *clkp,
    struct clk *clkp_parent)
{
	struct rk_cru_softc * const sc = priv;
	struct rk_cru_clk *clk = (struct rk_cru_clk *)clkp;

	if (clk->set_parent == NULL)
		return EINVAL;

	return clk->set_parent(sc, clk, clkp_parent->name);
}

static struct clk *
rk_cru_clock_get_parent(void *priv, struct clk *clkp)
{
	struct rk_cru_softc * const sc = priv;
	struct rk_cru_clk *clk = (struct rk_cru_clk *)clkp;
	struct rk_cru_clk *clk_parent;
	const char *parent;

	if (clk->get_parent == NULL)
		return NULL;

	parent = clk->get_parent(sc, clk);
	if (parent == NULL)
		return NULL;

	clk_parent = rk_cru_clock_find(sc, parent);
	if (clk_parent != NULL)
		return &clk_parent->base;

	/* No parent in this domain, try FDT */
	return fdtbus_clock_byname(parent);
}

static const struct clk_funcs rk_cru_clock_funcs = {
	.get = rk_cru_clock_get,
	.put = rk_cru_clock_put,
	.get_rate = rk_cru_clock_get_rate,
	.set_rate = rk_cru_clock_set_rate,
	.round_rate = rk_cru_clock_round_rate,
	.enable = rk_cru_clock_enable,
	.disable = rk_cru_clock_disable,
	.set_parent = rk_cru_clock_set_parent,
	.get_parent = rk_cru_clock_get_parent,
};

struct rk_cru_clk *
rk_cru_clock_find(struct rk_cru_softc *sc, const char *name)
{
	for (int i = 0; i < sc->sc_nclks; i++) {
		if (sc->sc_clks[i].base.name == NULL)
			continue;
		if (strcmp(sc->sc_clks[i].base.name, name) == 0)
			return &sc->sc_clks[i];
	}

	return NULL;
}

int
rk_cru_attach(struct rk_cru_softc *sc)
{
	bus_addr_t addr;
	bus_size_t size;
	int i;

	if (of_hasprop(sc->sc_phandle, "rockchip,grf")) {
		sc->sc_grf = fdtbus_syscon_acquire(sc->sc_phandle, "rockchip,grf");
		if (sc->sc_grf == NULL) {
			aprint_error(": couldn't get grf syscon\n");
			return ENXIO;
		}
	}

	if (fdtbus_get_reg(sc->sc_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return ENXIO;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return ENXIO;
	}

	sc->sc_clkdom.name = device_xname(sc->sc_dev);
	sc->sc_clkdom.funcs = &rk_cru_clock_funcs;
	sc->sc_clkdom.priv = sc;
	for (i = 0; i < sc->sc_nclks; i++) {
		sc->sc_clks[i].base.domain = &sc->sc_clkdom;
		clk_attach(&sc->sc_clks[i].base);
	}

	fdtbus_register_clock_controller(sc->sc_dev, sc->sc_phandle,
	    &rk_cru_fdtclock_funcs);

	fdtbus_register_reset_controller(sc->sc_dev, sc->sc_phandle,
	    &rk_cru_fdtreset_funcs);

	return 0;
}

void
rk_cru_print(struct rk_cru_softc *sc)
{
	struct rk_cru_clk *clk;
	struct clk *clkp_parent;
	const char *type;
	int i;

	for (i = 0; i < sc->sc_nclks; i++) {
		clk = &sc->sc_clks[i];
		if (clk->type == RK_CRU_UNKNOWN)
			continue;

		clkp_parent = clk_get_parent(&clk->base);

		switch (clk->type) {
		case RK_CRU_PLL:		type = "pll"; break;
		case RK_CRU_ARM:		type = "arm"; break;
		case RK_CRU_COMPOSITE:		type = "comp"; break;
		case RK_CRU_GATE:		type = "gate"; break;
		case RK_CRU_MUX:		type = "mux"; break;
		default:			type = "???"; break;
		}

        	aprint_debug_dev(sc->sc_dev,
		    "%3d %-14s %2s %-14s %-7s ",
		    clk->id,
        	    clk->base.name,
        	    clkp_parent ? "<-" : "",
        	    clkp_parent ? clkp_parent->name : "",
        	    type);
		aprint_debug("%10d Hz\n", clk_get_rate(&clk->base));
	}
}
