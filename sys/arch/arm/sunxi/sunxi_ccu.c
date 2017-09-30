/* $NetBSD: sunxi_ccu.c,v 1.7 2017/09/30 12:48:58 jmcneill Exp $ */

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

#include "opt_soc.h"
#include "opt_multiprocessor.h"
#include "opt_fdt_arm.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_ccu.c,v 1.7 2017/09/30 12:48:58 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <dev/clk/clk_backend.h>

#include <arm/sunxi/sunxi_ccu.h>

static void *
sunxi_ccu_reset_acquire(device_t dev, const void *data, size_t len)
{
	struct sunxi_ccu_softc * const sc = device_private(dev);
	struct sunxi_ccu_reset *reset;

	if (len != 4)
		return NULL;

	const u_int reset_id = be32dec(data);

	if (reset_id >= sc->sc_nresets)
		return NULL;

	reset = &sc->sc_resets[reset_id];
	if (reset->mask == 0)
		return NULL;

	return reset;
}

static void
sunxi_ccu_reset_release(device_t dev, void *priv)
{
}

static int
sunxi_ccu_reset_assert(device_t dev, void *priv)
{
	struct sunxi_ccu_softc * const sc = device_private(dev);
	struct sunxi_ccu_reset * const reset = priv;

	const uint32_t val = CCU_READ(sc, reset->reg);
	CCU_WRITE(sc, reset->reg, val & ~reset->mask);

	return 0;
}

static int
sunxi_ccu_reset_deassert(device_t dev, void *priv)
{
	struct sunxi_ccu_softc * const sc = device_private(dev);
	struct sunxi_ccu_reset * const reset = priv;

	const uint32_t val = CCU_READ(sc, reset->reg);
	CCU_WRITE(sc, reset->reg, val | reset->mask);

	return 0;
}

static const struct fdtbus_reset_controller_func sunxi_ccu_fdtreset_funcs = {
	.acquire = sunxi_ccu_reset_acquire,
	.release = sunxi_ccu_reset_release,
	.reset_assert = sunxi_ccu_reset_assert,
	.reset_deassert = sunxi_ccu_reset_deassert,
};

static struct clk *
sunxi_ccu_clock_decode(device_t dev, const void *data, size_t len)
{
	struct sunxi_ccu_softc * const sc = device_private(dev);
	struct sunxi_ccu_clk *clk;

	if (len != 4)
		return NULL;

	const u_int clock_id = be32dec(data);
	if (clock_id >= sc->sc_nclks)
		return NULL;

	clk = &sc->sc_clks[clock_id];
	if (clk->type == SUNXI_CCU_UNKNOWN)
		return NULL;

	return &clk->base;
}

static const struct fdtbus_clock_controller_func sunxi_ccu_fdtclock_funcs = {
	.decode = sunxi_ccu_clock_decode,
};

static struct clk *
sunxi_ccu_clock_get(void *priv, const char *name)
{
	struct sunxi_ccu_softc * const sc = priv;
	struct sunxi_ccu_clk *clk;

	clk = sunxi_ccu_clock_find(sc, name);
	if (clk == NULL)
		return NULL;

	return &clk->base;
}

static void
sunxi_ccu_clock_put(void *priv, struct clk *clk)
{
}

static u_int
sunxi_ccu_clock_get_rate(void *priv, struct clk *clkp)
{
	struct sunxi_ccu_softc * const sc = priv;
	struct sunxi_ccu_clk *clk = (struct sunxi_ccu_clk *)clkp;
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
sunxi_ccu_clock_set_rate(void *priv, struct clk *clkp, u_int rate)
{
	struct sunxi_ccu_softc * const sc = priv;
	struct sunxi_ccu_clk *clk = (struct sunxi_ccu_clk *)clkp;
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

static int
sunxi_ccu_clock_enable(void *priv, struct clk *clkp)
{
	struct sunxi_ccu_softc * const sc = priv;
	struct sunxi_ccu_clk *clk = (struct sunxi_ccu_clk *)clkp;
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
sunxi_ccu_clock_disable(void *priv, struct clk *clkp)
{
	struct sunxi_ccu_softc * const sc = priv;
	struct sunxi_ccu_clk *clk = (struct sunxi_ccu_clk *)clkp;
	int error = EINVAL;

	if (clk->enable)
		error = clk->enable(sc, clk, 0);

	return error;
}

static int
sunxi_ccu_clock_set_parent(void *priv, struct clk *clkp,
    struct clk *clkp_parent)
{
	struct sunxi_ccu_softc * const sc = priv;
	struct sunxi_ccu_clk *clk = (struct sunxi_ccu_clk *)clkp;

	if (clk->set_parent == NULL)
		return EINVAL;

	return clk->set_parent(sc, clk, clkp_parent->name);
}

static struct clk *
sunxi_ccu_clock_get_parent(void *priv, struct clk *clkp)
{
	struct sunxi_ccu_softc * const sc = priv;
	struct sunxi_ccu_clk *clk = (struct sunxi_ccu_clk *)clkp;
	struct sunxi_ccu_clk *clk_parent;
	const char *parent;

	if (clk->get_parent == NULL)
		return NULL;

	parent = clk->get_parent(sc, clk);
	if (parent == NULL)
		return NULL;

	clk_parent = sunxi_ccu_clock_find(sc, parent);
	if (clk_parent != NULL)
		return &clk_parent->base;

	/* No parent in this domain, try FDT */
	return fdtbus_clock_get(sc->sc_phandle, parent);
}

static const struct clk_funcs sunxi_ccu_clock_funcs = {
	.get = sunxi_ccu_clock_get,
	.put = sunxi_ccu_clock_put,
	.get_rate = sunxi_ccu_clock_get_rate,
	.set_rate = sunxi_ccu_clock_set_rate,
	.enable = sunxi_ccu_clock_enable,
	.disable = sunxi_ccu_clock_disable,
	.set_parent = sunxi_ccu_clock_set_parent,
	.get_parent = sunxi_ccu_clock_get_parent,
};

struct sunxi_ccu_clk *
sunxi_ccu_clock_find(struct sunxi_ccu_softc *sc, const char *name)
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
sunxi_ccu_attach(struct sunxi_ccu_softc *sc)
{
	bus_addr_t addr;
	bus_size_t size;
	int i;

	if (fdtbus_get_reg(sc->sc_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return ENXIO;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return ENXIO;
	}

	sc->sc_clkdom.funcs = &sunxi_ccu_clock_funcs;
	sc->sc_clkdom.priv = sc;
	for (i = 0; i < sc->sc_nclks; i++)
		sc->sc_clks[i].base.domain = &sc->sc_clkdom;

	fdtbus_register_clock_controller(sc->sc_dev, sc->sc_phandle,
	    &sunxi_ccu_fdtclock_funcs);

	fdtbus_register_reset_controller(sc->sc_dev, sc->sc_phandle,
	    &sunxi_ccu_fdtreset_funcs);

	return 0;
}

void
sunxi_ccu_print(struct sunxi_ccu_softc *sc)
{
	struct sunxi_ccu_clk *clk;
	struct clk *clkp_parent;
	const char *type;
	int i;

	for (i = 0; i < sc->sc_nclks; i++) {
		clk = &sc->sc_clks[i];
		if (clk->type == SUNXI_CCU_UNKNOWN)
			continue;

		clkp_parent = clk_get_parent(&clk->base);

		switch (clk->type) {
		case SUNXI_CCU_GATE:		type = "gate"; break;
		case SUNXI_CCU_NM:		type = "nm"; break;
		case SUNXI_CCU_NKMP:		type = "nkmp"; break;
		case SUNXI_CCU_PREDIV:		type = "prediv"; break;
		case SUNXI_CCU_DIV:		type = "div"; break;
		case SUNXI_CCU_PHASE:		type = "phase"; break;
		case SUNXI_CCU_FIXED_FACTOR:	type = "fixed-factor"; break;
		default:			type = "???"; break;
		}

        	aprint_debug_dev(sc->sc_dev,
		    "%3d %-12s %2s %-12s %-7s ",
		    i,
        	    clk->base.name,
        	    clkp_parent ? "<-" : "",
        	    clkp_parent ? clkp_parent->name : "",
        	    type);
		aprint_debug("%10d Hz\n", clk_get_rate(&clk->base));
	}
}
