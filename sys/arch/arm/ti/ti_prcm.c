/* $NetBSD: ti_prcm.c,v 1.1.2.2 2017/12/03 11:35:56 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ti_prcm.c,v 1.1.2.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <dev/clk/clk_backend.h>

#define	TI_PRCM_PRIVATE
#include <arm/ti/ti_prcm.h>

static struct ti_prcm_softc *prcm_softc = NULL;

static struct clk *
ti_prcm_clock_get(void *priv, const char *name)
{
	struct ti_prcm_softc * const sc = priv;
	struct ti_prcm_clk *clk;

	clk = ti_prcm_clock_find(sc, name);
	if (clk == NULL)
		return NULL;

	return &clk->base;
}

static void
ti_prcm_clock_put(void *priv, struct clk *clk)
{
}

static u_int
ti_prcm_clock_get_rate(void *priv, struct clk *clkp)
{
	struct ti_prcm_softc * const sc = priv;
	struct ti_prcm_clk *clk = (struct ti_prcm_clk *)clkp;
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
ti_prcm_clock_set_rate(void *priv, struct clk *clkp, u_int rate)
{
	struct ti_prcm_softc * const sc = priv;
	struct ti_prcm_clk *clk = (struct ti_prcm_clk *)clkp;
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
ti_prcm_clock_enable(void *priv, struct clk *clkp)
{
	struct ti_prcm_softc * const sc = priv;
	struct ti_prcm_clk *clk = (struct ti_prcm_clk *)clkp;
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
ti_prcm_clock_disable(void *priv, struct clk *clkp)
{
	struct ti_prcm_softc * const sc = priv;
	struct ti_prcm_clk *clk = (struct ti_prcm_clk *)clkp;
	int error = EINVAL;

	if (clk->enable)
		error = clk->enable(sc, clk, 0);

	return error;
}

static int
ti_prcm_clock_set_parent(void *priv, struct clk *clkp,
    struct clk *clkp_parent)
{
	struct ti_prcm_softc * const sc = priv;
	struct ti_prcm_clk *clk = (struct ti_prcm_clk *)clkp;

	if (clk->set_parent == NULL)
		return EINVAL;

	return clk->set_parent(sc, clk, clkp_parent->name);
}

static struct clk *
ti_prcm_clock_get_parent(void *priv, struct clk *clkp)
{
	struct ti_prcm_softc * const sc = priv;
	struct ti_prcm_clk *clk = (struct ti_prcm_clk *)clkp;
	struct ti_prcm_clk *clk_parent;
	const char *parent;

	if (clk->get_parent == NULL)
		return NULL;

	parent = clk->get_parent(sc, clk);
	if (parent == NULL)
		return NULL;

	clk_parent = ti_prcm_clock_find(sc, parent);
	if (clk_parent != NULL)
		return &clk_parent->base;

	/* No parent in this domain, try FDT */
	return fdtbus_clock_get(sc->sc_phandle, parent);
}

static const struct clk_funcs ti_prcm_clock_funcs = {
	.get = ti_prcm_clock_get,
	.put = ti_prcm_clock_put,
	.get_rate = ti_prcm_clock_get_rate,
	.set_rate = ti_prcm_clock_set_rate,
	.enable = ti_prcm_clock_enable,
	.disable = ti_prcm_clock_disable,
	.set_parent = ti_prcm_clock_set_parent,
	.get_parent = ti_prcm_clock_get_parent,
};

struct ti_prcm_clk *
ti_prcm_clock_find(struct ti_prcm_softc *sc, const char *name)
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
ti_prcm_attach(struct ti_prcm_softc *sc)
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

	sc->sc_clkdom.funcs = &ti_prcm_clock_funcs;
	sc->sc_clkdom.priv = sc;
	for (i = 0; i < sc->sc_nclks; i++)
		sc->sc_clks[i].base.domain = &sc->sc_clkdom;

	KASSERT(prcm_softc == NULL);
	prcm_softc = sc;

	return 0;
}

struct clk *
ti_prcm_get_hwmod(const int phandle, u_int index)
{
	struct ti_prcm_clk *tc;
	const char *hwmods, *p;
	int len, resid;
	u_int n;

	KASSERTMSG(prcm_softc != NULL, "prcm driver not attached");

	hwmods = fdtbus_get_prop(phandle, "ti,hwmods", &len);
	if (len <= 0)
		return NULL;

	p = hwmods;
	for (n = 0, resid = len; resid > 0; n++) {
		if (n == index) {
			tc = ti_prcm_clock_find(prcm_softc, p);
			if (tc == NULL) {
				aprint_error_dev(prcm_softc->sc_dev,
				    "no hwmod with name '%s'\n", p);
				return NULL;
			}
			KASSERT(tc->type == TI_PRCM_HWMOD);
			return &tc->base;
		}
		resid -= strlen(p);
		p += strlen(p) + 1;
	}

	return NULL;
}
