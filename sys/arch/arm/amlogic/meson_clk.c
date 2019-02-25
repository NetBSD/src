/* $NetBSD: meson_clk.c,v 1.2 2019/02/25 19:30:17 jmcneill Exp $ */

/*-
 * Copyright (c) 2017-2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: meson_clk.c,v 1.2 2019/02/25 19:30:17 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#include <dev/clk/clk_backend.h>

#include <arm/amlogic/meson_clk.h>

static void *
meson_clk_reset_acquire(device_t dev, const void *data, size_t len)
{
	struct meson_clk_softc * const sc = device_private(dev);
	struct meson_clk_reset *reset;

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
meson_clk_reset_release(device_t dev, void *priv)
{
}

static int
meson_clk_reset_assert(device_t dev, void *priv)
{
	struct meson_clk_softc * const sc = device_private(dev);
	struct meson_clk_reset * const reset = priv;

	CLK_LOCK(sc);
	const uint32_t val = CLK_READ(sc, reset->reg);
	CLK_WRITE(sc, reset->reg, val | reset->mask);
	CLK_UNLOCK(sc);

	return 0;
}

static int
meson_clk_reset_deassert(device_t dev, void *priv)
{
	struct meson_clk_softc * const sc = device_private(dev);
	struct meson_clk_reset * const reset = priv;

	CLK_LOCK(sc);
	const uint32_t val = CLK_READ(sc, reset->reg);
	CLK_WRITE(sc, reset->reg, val & ~reset->mask);
	CLK_UNLOCK(sc);

	return 0;
}

static const struct fdtbus_reset_controller_func meson_clk_fdtreset_funcs = {
	.acquire = meson_clk_reset_acquire,
	.release = meson_clk_reset_release,
	.reset_assert = meson_clk_reset_assert,
	.reset_deassert = meson_clk_reset_deassert,
};

static struct clk *
meson_clk_clock_decode(device_t dev, int cc_phandle, const void *data,
		       size_t len)
{
	struct meson_clk_softc * const sc = device_private(dev);
	struct meson_clk_clk *clk;

	if (len != 4)
		return NULL;

	const u_int clock_id = be32dec(data);
	if (clock_id >= sc->sc_nclks)
		return NULL;

	clk = &sc->sc_clks[clock_id];
	if (clk->type == MESON_CLK_UNKNOWN)
		return NULL;

	return &clk->base;
}

static const struct fdtbus_clock_controller_func meson_clk_fdtclock_funcs = {
	.decode = meson_clk_clock_decode,
};

static struct clk *
meson_clk_clock_get(void *priv, const char *name)
{
	struct meson_clk_softc * const sc = priv;
	struct meson_clk_clk *clk;

	clk = meson_clk_clock_find(sc, name);
	if (clk == NULL)
		return NULL;

	return &clk->base;
}

static void
meson_clk_clock_put(void *priv, struct clk *clk)
{
}

static u_int
meson_clk_clock_get_rate(void *priv, struct clk *clkp)
{
	struct meson_clk_softc * const sc = priv;
	struct meson_clk_clk *clk = (struct meson_clk_clk *)clkp;
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
meson_clk_clock_set_rate(void *priv, struct clk *clkp, u_int rate)
{
	struct meson_clk_softc * const sc = priv;
	struct meson_clk_clk *clk = (struct meson_clk_clk *)clkp;
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
meson_clk_clock_round_rate(void *priv, struct clk *clkp, u_int rate)
{
	struct meson_clk_softc * const sc = priv;
	struct meson_clk_clk *clk = (struct meson_clk_clk *)clkp;
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
meson_clk_clock_enable(void *priv, struct clk *clkp)
{
	struct meson_clk_softc * const sc = priv;
	struct meson_clk_clk *clk = (struct meson_clk_clk *)clkp;
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
meson_clk_clock_disable(void *priv, struct clk *clkp)
{
	struct meson_clk_softc * const sc = priv;
	struct meson_clk_clk *clk = (struct meson_clk_clk *)clkp;
	int error = EINVAL;

	if (clk->enable)
		error = clk->enable(sc, clk, 0);

	return error;
}

static int
meson_clk_clock_set_parent(void *priv, struct clk *clkp,
    struct clk *clkp_parent)
{
	struct meson_clk_softc * const sc = priv;
	struct meson_clk_clk *clk = (struct meson_clk_clk *)clkp;

	if (clk->set_parent == NULL)
		return EINVAL;

	return clk->set_parent(sc, clk, clkp_parent->name);
}

static struct clk *
meson_clk_clock_get_parent(void *priv, struct clk *clkp)
{
	struct meson_clk_softc * const sc = priv;
	struct meson_clk_clk *clk = (struct meson_clk_clk *)clkp;
	struct meson_clk_clk *clk_parent;
	const char *parent;

	if (clk->get_parent == NULL)
		return NULL;

	parent = clk->get_parent(sc, clk);
	if (parent == NULL)
		return NULL;

	clk_parent = meson_clk_clock_find(sc, parent);
	if (clk_parent != NULL)
		return &clk_parent->base;

	/* No parent in this domain, try FDT */
	return fdtbus_clock_get(sc->sc_phandle, parent);
}

static const struct clk_funcs meson_clk_clock_funcs = {
	.get = meson_clk_clock_get,
	.put = meson_clk_clock_put,
	.get_rate = meson_clk_clock_get_rate,
	.set_rate = meson_clk_clock_set_rate,
	.round_rate = meson_clk_clock_round_rate,
	.enable = meson_clk_clock_enable,
	.disable = meson_clk_clock_disable,
	.set_parent = meson_clk_clock_set_parent,
	.get_parent = meson_clk_clock_get_parent,
};

struct meson_clk_clk *
meson_clk_clock_find(struct meson_clk_softc *sc, const char *name)
{
	for (int i = 0; i < sc->sc_nclks; i++) {
		if (sc->sc_clks[i].base.name == NULL)
			continue;
		if (strcmp(sc->sc_clks[i].base.name, name) == 0)
			return &sc->sc_clks[i];
	}

	return NULL;
}

void
meson_clk_attach(struct meson_clk_softc *sc)
{
	int i;

	sc->sc_clkdom.name = device_xname(sc->sc_dev);
	sc->sc_clkdom.funcs = &meson_clk_clock_funcs;
	sc->sc_clkdom.priv = sc;
	for (i = 0; i < sc->sc_nclks; i++) {
		sc->sc_clks[i].base.domain = &sc->sc_clkdom;
		clk_attach(&sc->sc_clks[i].base);
	}

	if (sc->sc_nclks > 0)
		fdtbus_register_clock_controller(sc->sc_dev, sc->sc_phandle,
		    &meson_clk_fdtclock_funcs);

	if (sc->sc_nresets > 0)
		fdtbus_register_reset_controller(sc->sc_dev, sc->sc_phandle,
		    &meson_clk_fdtreset_funcs);
}

void
meson_clk_print(struct meson_clk_softc *sc)
{
	struct meson_clk_clk *clk;
	struct clk *clkp_parent;
	const char *type;
	int i;

	for (i = 0; i < sc->sc_nclks; i++) {
		clk = &sc->sc_clks[i];
		if (clk->type == MESON_CLK_UNKNOWN)
			continue;

		clkp_parent = clk_get_parent(&clk->base);

		switch (clk->type) {
		case MESON_CLK_FIXED:		type = "fixed"; break;
		case MESON_CLK_GATE:		type = "gate"; break;
		case MESON_CLK_MPLL:		type = "mpll"; break;
		case MESON_CLK_PLL:		type = "pll"; break;
		case MESON_CLK_DIV:		type = "div"; break;
		case MESON_CLK_FIXED_FACTOR:	type = "fixed-factor"; break;
		case MESON_CLK_MUX:		type = "mux"; break;
		default:			type = "???"; break;
		}

        	aprint_debug_dev(sc->sc_dev,
		    "%3d %-12s %2s %-12s %-7s ",
		    i,
        	    clk->base.name,
        	    clkp_parent ? "<-" : "",
        	    clkp_parent ? clkp_parent->name : "",
        	    type);
		aprint_debug("%10u Hz\n", clk_get_rate(&clk->base));
	}
}

void
meson_clk_lock(struct meson_clk_softc *sc)
{
	if (sc->sc_syscon != NULL)
		syscon_lock(sc->sc_syscon);
}

void
meson_clk_unlock(struct meson_clk_softc *sc)
{
	if (sc->sc_syscon != NULL)
		syscon_unlock(sc->sc_syscon);
}

uint32_t
meson_clk_read(struct meson_clk_softc *sc, bus_size_t reg)
{
	if (sc->sc_syscon != NULL)
		return syscon_read_4(sc->sc_syscon, reg);
	else
		return bus_space_read_4(sc->sc_bst, sc->sc_bsh, reg);
}

void
meson_clk_write(struct meson_clk_softc *sc, bus_size_t reg, uint32_t val)
{
	if (sc->sc_syscon != NULL)
		syscon_write_4(sc->sc_syscon, reg, val);
	else
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, reg, val);
}
