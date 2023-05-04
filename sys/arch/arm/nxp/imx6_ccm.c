/*	$NetBSD: imx6_ccm.c,v 1.4 2023/05/04 13:25:07 bouyer Exp $	*/

/*
 * Copyright (c) 2010-2012, 2014  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Clock Controller Module (CCM) for i.MX6
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx6_ccm.c,v 1.4 2023/05/04 13:25:07 bouyer Exp $");

#include "opt_imx.h"
#include "opt_cputypes.h"

#include "locators.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/cpufreq.h>
#include <sys/param.h>

#include <machine/cpu.h>

#include <arm/nxp/imx6_ccmvar.h>
#include <arm/nxp/imx6_ccmreg.h>

#include <dev/clk/clk_backend.h>

static void imxccm_init_clocks(struct imx6ccm_softc *,
			       struct imxccm_init_parent *);
static struct clk *imxccm_clk_get(void *, const char *);
static void imxccm_clk_put(void *, struct clk *);
static u_int imxccm_clk_get_rate(void *, struct clk *);
static int imxccm_clk_set_rate(void *, struct clk *, u_int);
static int imxccm_clk_enable(void *, struct clk *);
static int imxccm_clk_disable(void *, struct clk *);
static int imxccm_clk_set_parent(void *, struct clk *, struct clk *);
static struct clk *imxccm_clk_get_parent(void *, struct clk *);

static const struct clk_funcs imxccm_clk_funcs = {
	.get = imxccm_clk_get,
	.put = imxccm_clk_put,
	.get_rate = imxccm_clk_get_rate,
	.set_rate = imxccm_clk_set_rate,
	.enable = imxccm_clk_enable,
	.disable = imxccm_clk_disable,
	.set_parent = imxccm_clk_set_parent,
	.get_parent = imxccm_clk_get_parent,
};

void
imx6ccm_attach_common(device_t self, struct imx6_clk *imx6_clks, int size, 
    struct imxccm_init_parent *imxccm_init_parents)
{
	struct imx6ccm_softc * const sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_imx6_clks = imx6_clks;
	sc->sc_imx6_clksize = size;

	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &imxccm_clk_funcs;
	sc->sc_clkdom.priv = sc;
	for (u_int n = 0; n < size; n++) {
		imx6_clks[n].base.domain = &sc->sc_clkdom;
		clk_attach(&imx6_clks[n].base);
	}

	imxccm_init_clocks(sc, imxccm_init_parents);

	for (int n = 0; n < size; n++) {
		struct clk *clk = &imx6_clks[n].base;
		struct clk *clk_parent = clk_get_parent(clk);
		const char *parent_str = clk_parent ? clk_parent->name : "none";

		aprint_verbose_dev(self, "%s (%s) : %u Hz\n", clk->name,
		    parent_str, clk_get_rate(clk));
	}
}

struct clk *
imx6_get_clock(struct imx6ccm_softc *sc, const char *name)
{
	struct imx6_clk *iclk;
	iclk = imx6_clk_find(sc, name);

	if (iclk == NULL)
		return NULL;

	return &iclk->base;
}

struct imx6_clk *
imx6_clk_find(struct imx6ccm_softc *sc, const char *name)
{
	if (name == NULL)
		return NULL;

	for (int n = 0; n < sc->sc_imx6_clksize; n++) {
		if (strcmp(sc->sc_imx6_clks[n].base.name, name) == 0)
			return &sc->sc_imx6_clks[n];
	}

	return NULL;
}

static void
imxccm_init_clocks(struct imx6ccm_softc *sc,
    struct imxccm_init_parent *imxccm_init_parents)
{
	struct clk *clk;
	struct clk *clk_parent;

	for (u_int n = 0; imxccm_init_parents[n].clock != NULL; n++) {
		clk = clk_get(&sc->sc_clkdom, imxccm_init_parents[n].clock);
		KASSERT(clk != NULL);
		clk_parent = clk_get(&sc->sc_clkdom, imxccm_init_parents[n].parent);
		KASSERT(clk_parent != NULL);

		int error = clk_set_parent(clk, clk_parent);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't set '%s' parent to '%s': %d\n",
			    clk->name, clk_parent->name, error);
		}
		clk_put(clk_parent);
		clk_put(clk);
	}
}

static u_int
imxccm_clk_get_rate_pll_generic(struct imx6ccm_softc *sc, struct imx6_clk *iclk,
    const u_int rate_parent)
{
	struct imx6_clk_pll *pll = &iclk->clk.pll;
	uint64_t freq = rate_parent;

	KASSERT((pll->type == IMX6_CLK_PLL_GENERIC) ||
	    (pll->type == IMX6_CLK_PLL_USB));

	uint32_t v = bus_space_read_4(sc->sc_iot, sc->sc_ioh_analog, pll->reg);
	uint32_t div = __SHIFTOUT(v, pll->mask);

	return freq * ((div == 1) ? 22 : 20);
}

static u_int
imxccm_clk_get_rate_pll_sys(struct imx6ccm_softc *sc, struct imx6_clk *iclk,
    const u_int rate_parent)
{
	struct imx6_clk_pll *pll = &iclk->clk.pll;
	uint64_t freq = rate_parent;

	KASSERT(pll->type == IMX6_CLK_PLL_SYS);

	uint32_t v = bus_space_read_4(sc->sc_iot, sc->sc_ioh_analog, pll->reg);
	uint32_t div = __SHIFTOUT(v, pll->mask);

	return freq * div / 2;
}

#define PLL_AUDIO_VIDEO_NUM_OFFSET	0x10
#define PLL_AUDIO_VIDEO_DENOM_OFFSET	0x20

static u_int
imxccm_clk_get_rate_pll_audio_video(struct imx6ccm_softc *sc,
    struct imx6_clk *iclk, const u_int rate_parent)
{
	struct imx6_clk_pll *pll = &iclk->clk.pll;
	uint64_t freq = rate_parent;

	KASSERT(pll->type == IMX6_CLK_PLL_AUDIO_VIDEO);

	uint32_t v = bus_space_read_4(sc->sc_iot, sc->sc_ioh_analog, pll->reg);
	uint32_t div = __SHIFTOUT(v, pll->mask);
	uint32_t num = bus_space_read_4(sc->sc_iot, sc->sc_ioh_analog,
	    pll->reg + PLL_AUDIO_VIDEO_NUM_OFFSET);
	uint32_t denom = bus_space_read_4(sc->sc_iot, sc->sc_ioh_analog,
	    pll->reg + PLL_AUDIO_VIDEO_DENOM_OFFSET);

	uint64_t tmp = freq * num / denom;

	return freq * div + tmp;
}

static u_int
imxccm_clk_get_rate_pll_enet(struct imx6ccm_softc *sc,
    struct imx6_clk *iclk, const u_int rate_parent)
{
	struct imx6_clk_pll *pll = &iclk->clk.pll;

	KASSERT(pll->type == IMX6_CLK_PLL_ENET);

	return pll->ref;
}

static u_int
imxccm_clk_get_rate_fixed_factor(struct imx6ccm_softc *sc, struct imx6_clk *iclk)
{
	struct imx6_clk_fixed_factor *fixed_factor = &iclk->clk.fixed_factor;
	struct imx6_clk *parent;

	KASSERT(iclk->type == IMX6_CLK_FIXED_FACTOR);

	parent = imx6_clk_find(sc, iclk->parent);
	KASSERT(parent != NULL);

	uint64_t rate_parent = imxccm_clk_get_rate(sc, &parent->base);

	return rate_parent * fixed_factor->mult / fixed_factor->div;
}

static u_int
imxccm_clk_get_rate_pll(struct imx6ccm_softc *sc, struct imx6_clk *iclk)
{
	struct imx6_clk_pll *pll = &iclk->clk.pll;
	struct imx6_clk *parent;

	KASSERT(iclk->type == IMX6_CLK_PLL);

	parent = imx6_clk_find(sc, iclk->parent);
	KASSERT(parent != NULL);

	uint64_t rate_parent = imxccm_clk_get_rate(sc, &parent->base);

	switch(pll->type) {
	case IMX6_CLK_PLL_GENERIC:
		return imxccm_clk_get_rate_pll_generic(sc, iclk, rate_parent);
	case IMX6_CLK_PLL_SYS:
		return imxccm_clk_get_rate_pll_sys(sc, iclk, rate_parent);
	case IMX6_CLK_PLL_USB:
		return imxccm_clk_get_rate_pll_generic(sc, iclk, rate_parent);
	case IMX6_CLK_PLL_AUDIO_VIDEO:
		return imxccm_clk_get_rate_pll_audio_video(sc, iclk, rate_parent);
	case IMX6_CLK_PLL_ENET:
		return imxccm_clk_get_rate_pll_enet(sc, iclk, rate_parent);
	default:
		panic("imx6: unknown pll type %d", iclk->type);
	}
}

static u_int
imxccm_clk_get_rate_div(struct imx6ccm_softc *sc, struct imx6_clk *iclk)
{
	struct imx6_clk_div *div = &iclk->clk.div;
	struct imx6_clk *parent;

	KASSERT(iclk->type == IMX6_CLK_DIV);

	parent = imx6_clk_find(sc, iclk->parent);
	KASSERT(parent != NULL);

	u_int rate = imxccm_clk_get_rate(sc, &parent->base);

	bus_space_handle_t ioh;
	if (div->base == IMX6_CLK_REG_CCM_ANALOG)
		ioh = sc->sc_ioh_analog;
	else
		ioh = sc->sc_ioh;

	uint32_t v = bus_space_read_4(sc->sc_iot, ioh, div->reg);
	uint32_t n = __SHIFTOUT(v, div->mask);

	if (div->type == IMX6_CLK_DIV_TABLE) {
		KASSERT(div->tbl != NULL);

		for (int i = 0; div->tbl[i] != 0; i++)
			if (i == n)
				rate /= div->tbl[i];
	} else {
		rate /= n + 1;
	}

	return rate;
}

static u_int
imxccm_clk_get_rate_pfd(struct imx6ccm_softc *sc, struct imx6_clk *iclk)
{
	struct imx6_clk_pfd *pfd = &iclk->clk.pfd;
	struct imx6_clk *parent;

	KASSERT(iclk->type == IMX6_CLK_PFD);

	parent = imx6_clk_find(sc, iclk->parent);
	KASSERT(parent != NULL);

	uint64_t rate_parent = imxccm_clk_get_rate(sc, &parent->base);

	uint32_t v = bus_space_read_4(sc->sc_iot, sc->sc_ioh_analog, pfd->reg);
	uint32_t n = __SHIFTOUT(v, __BITS(5, 0) << (pfd->index * 8));

	KASSERT(n != 0);

	return (rate_parent * 18) / n;
}

static int
imxccm_clk_mux_wait(struct imx6ccm_softc *sc, struct imx6_clk_mux *mux)
{
	KASSERT(mux->busy_reg == 0);
	KASSERT(mux->busy_mask == 0);

	bus_space_handle_t ioh;
	if (mux->base == IMX6_CLK_REG_CCM_ANALOG)
		ioh = sc->sc_ioh_analog;
	else
		ioh = sc->sc_ioh;

	while (bus_space_read_4(sc->sc_iot, ioh, mux->busy_reg) & mux->busy_mask)
		delay(10);

	return 0;
}

static int
imxccm_clk_set_parent_mux(struct imx6ccm_softc *sc,
    struct imx6_clk *iclk, struct clk *parent)
{
	struct imx6_clk_mux *mux = &iclk->clk.mux;
	const char *pname = parent->name;
	u_int sel;

	KASSERT(iclk->type == IMX6_CLK_MUX);

	for (sel = 0; sel < mux->nparents; sel++)
		if (strcmp(pname, mux->parents[sel]) == 0)
			break;

	if (sel == mux->nparents)
		return EINVAL;

	bus_space_handle_t ioh;
	if (mux->base == IMX6_CLK_REG_CCM_ANALOG)
		ioh = sc->sc_ioh_analog;
	else
		ioh = sc->sc_ioh;

	uint32_t v = bus_space_read_4(sc->sc_iot, ioh, mux->reg);
	v &= ~mux->mask;
	v |= __SHIFTIN(sel, mux->mask);

	bus_space_write_4(sc->sc_iot, ioh, mux->reg, v);

	iclk->parent = pname;

	if (mux->type == IMX6_CLK_MUX_BUSY)
		imxccm_clk_mux_wait(sc, mux);

	return 0;
}

static struct imx6_clk *
imxccm_clk_get_parent_mux(struct imx6ccm_softc *sc, struct imx6_clk *iclk)
{
	struct imx6_clk_mux *mux = &iclk->clk.mux;

	KASSERT(iclk->type == IMX6_CLK_MUX);

	bus_space_handle_t ioh;
	if (mux->base == IMX6_CLK_REG_CCM_ANALOG)
		ioh = sc->sc_ioh_analog;
	else
		ioh = sc->sc_ioh;

	uint32_t v = bus_space_read_4(sc->sc_iot, ioh, mux->reg);
	u_int sel = __SHIFTOUT(v, mux->mask);

	KASSERTMSG(sel < mux->nparents, "mux %s sel %d nparents %d",
	    iclk->base.name, sel, mux->nparents);

	iclk->parent = mux->parents[sel];

	return imx6_clk_find(sc, iclk->parent);
}

static int
imxccm_clk_set_rate_pll(struct imx6ccm_softc *sc,
    struct imx6_clk *iclk, u_int rate)
{
	/* ToDo */

	return EOPNOTSUPP;
}

static int
imxccm_clk_set_rate_div(struct imx6ccm_softc *sc,
    struct imx6_clk *iclk, u_int rate)
{
	struct imx6_clk_div *div = &iclk->clk.div;
	struct imx6_clk *parent;

	KASSERT(iclk->type == IMX6_CLK_DIV);

	parent = imx6_clk_find(sc, iclk->parent);
	KASSERT(parent != NULL);

	u_int rate_parent = imxccm_clk_get_rate(sc, &parent->base);
	u_int divider = uimax(1, rate_parent / rate);

	bus_space_handle_t ioh;
	if (div->base == IMX6_CLK_REG_CCM_ANALOG)
		ioh = sc->sc_ioh_analog;
	else
		ioh = sc->sc_ioh;

	uint32_t v = bus_space_read_4(sc->sc_iot, ioh, div->reg);
	v &= ~div->mask;
	if (div->type == IMX6_CLK_DIV_TABLE) {
		int n = -1;

		KASSERT(div->tbl != NULL);
		for (int i = 0; div->tbl[i] != 0; i++)
			if (div->tbl[i] == divider)
				n = i;

		if (n >= 0)
			v |= __SHIFTIN(n, div->mask);
		else
			return EINVAL;
	} else {
		v |= __SHIFTIN(divider - 1, div->mask);
	}
	bus_space_write_4(sc->sc_iot, ioh, div->reg, v);

	return 0;
}

/*
 * CLK Driver APIs
 */
static struct clk *
imxccm_clk_get(void *priv, const char *name)
{
	struct imx6_clk *iclk;
	struct imx6ccm_softc *sc = priv;

	iclk = imx6_clk_find(sc, name);
	if (iclk == NULL)
		return NULL;

	atomic_inc_uint(&iclk->refcnt);

	return &iclk->base;
}

static void
imxccm_clk_put(void *priv, struct clk *clk)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;

	KASSERT(iclk->refcnt > 0);

	atomic_dec_uint(&iclk->refcnt);
}

static u_int
imxccm_clk_get_rate(void *priv, struct clk *clk)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;
	struct clk *parent;
	struct imx6ccm_softc *sc = priv;

	switch (iclk->type) {
	case IMX6_CLK_FIXED:
		return iclk->clk.fixed.rate;
	case IMX6_CLK_FIXED_FACTOR:
		return imxccm_clk_get_rate_fixed_factor(sc, iclk);
	case IMX6_CLK_PLL:
		return imxccm_clk_get_rate_pll(sc, iclk);
	case IMX6_CLK_MUX:
	case IMX6_CLK_GATE:
		parent = imxccm_clk_get_parent(sc, clk);
		return imxccm_clk_get_rate(sc, parent);
	case IMX6_CLK_DIV:
		return imxccm_clk_get_rate_div(sc, iclk);
	case IMX6_CLK_PFD:
		return imxccm_clk_get_rate_pfd(sc, iclk);
	default:
		panic("imx6: unknown clk type %d", iclk->type);
	}
}

static int
imxccm_clk_set_rate(void *priv, struct clk *clk, u_int rate)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;
	struct imx6ccm_softc *sc = priv;

	switch (iclk->type) {
	case IMX6_CLK_FIXED:
	case IMX6_CLK_FIXED_FACTOR:
		return ENXIO;
	case IMX6_CLK_PLL:
		return imxccm_clk_set_rate_pll(sc, iclk, rate);
	case IMX6_CLK_MUX:
		return ENXIO;
	case IMX6_CLK_GATE:
		return ENXIO;
	case IMX6_CLK_DIV:
		return imxccm_clk_set_rate_div(sc, iclk, rate);
	case IMX6_CLK_PFD:
		return EINVAL;
	default:
		panic("imx6: unknown clk type %d", iclk->type);
	}
}

static int
imxccm_clk_enable_pll(struct imx6ccm_softc *sc, struct imx6_clk *iclk, bool enable)
{
	struct imx6_clk_pll *pll = &iclk->clk.pll;

	KASSERT(iclk->type == IMX6_CLK_PLL);

	/* Power up bit */
	if (pll->type == IMX6_CLK_PLL_USB)
		enable = !enable;

	bus_space_handle_t ioh = sc->sc_ioh_analog;
	uint32_t  v = bus_space_read_4(sc->sc_iot, ioh, pll->reg);
	if (__SHIFTOUT(v, pll->powerdown) != enable)
		return 0;
	if (enable)
		v &= ~pll->powerdown;
	else
		v |= pll->powerdown;
	bus_space_write_4(sc->sc_iot, ioh, pll->reg, v);

	/* wait look */
	while (!(bus_space_read_4(sc->sc_iot, ioh, pll->reg) & CCM_ANALOG_PLL_LOCK))
		delay(10);

	return 0;
}

static int
imxccm_clk_enable_gate(struct imx6ccm_softc *sc, struct imx6_clk *iclk, bool enable)
{
	struct imx6_clk_gate *gate = &iclk->clk.gate;

	KASSERT(iclk->type == IMX6_CLK_GATE);

	bus_space_handle_t ioh;
	if (gate->base == IMX6_CLK_REG_CCM_ANALOG)
		ioh = sc->sc_ioh_analog;
	else
		ioh = sc->sc_ioh;

	uint32_t v = bus_space_read_4(sc->sc_iot, ioh, gate->reg);
	if (enable) {
		if (gate->exclusive_mask)
			v &= ~gate->exclusive_mask;
		v |= gate->mask;
	} else {
		if (gate->exclusive_mask)
			v |= gate->exclusive_mask;
		v &= ~gate->mask;
	}
	bus_space_write_4(sc->sc_iot, ioh, gate->reg, v);

	return 0;
}

static int
imxccm_clk_enable(void *priv, struct clk *clk)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;
	struct imx6_clk *parent = NULL;
	struct imx6ccm_softc *sc = priv;

	if ((parent = imx6_clk_find(sc, iclk->parent)) != NULL)
		imxccm_clk_enable(sc, &parent->base);

	switch (iclk->type) {
	case IMX6_CLK_FIXED:
	case IMX6_CLK_FIXED_FACTOR:
		return 0;	/* always on */
	case IMX6_CLK_PLL:
		return imxccm_clk_enable_pll(sc, iclk, true);
	case IMX6_CLK_MUX:
	case IMX6_CLK_DIV:
	case IMX6_CLK_PFD:
		return 0;
	case IMX6_CLK_GATE:
		return imxccm_clk_enable_gate(sc, iclk, true);
	default:
		panic("imx6: unknown clk type %d", iclk->type);
	}
}

static int
imxccm_clk_disable(void *priv, struct clk *clk)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;
	struct imx6ccm_softc *sc = priv;

	switch (iclk->type) {
	case IMX6_CLK_FIXED:
	case IMX6_CLK_FIXED_FACTOR:
		return EINVAL;	/* always on */
	case IMX6_CLK_PLL:
		return imxccm_clk_enable_pll(sc, iclk, false);
	case IMX6_CLK_MUX:
	case IMX6_CLK_DIV:
	case IMX6_CLK_PFD:
		return EINVAL;
	case IMX6_CLK_GATE:
		return imxccm_clk_enable_gate(sc, iclk, false);
	default:
		panic("imx6: unknown clk type %d", iclk->type);
	}
}

static int
imxccm_clk_set_parent(void *priv, struct clk *clk, struct clk *parent)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;
	struct imx6ccm_softc *sc = priv;

	switch (iclk->type) {
	case IMX6_CLK_FIXED:
	case IMX6_CLK_FIXED_FACTOR:
	case IMX6_CLK_PLL:
	case IMX6_CLK_GATE:
	case IMX6_CLK_DIV:
	case IMX6_CLK_PFD:
		return EINVAL;
	case IMX6_CLK_MUX:
		return imxccm_clk_set_parent_mux(sc, iclk, parent);
	default:
		panic("imx6: unknown clk type %d", iclk->type);
	}
}

static struct clk *
imxccm_clk_get_parent(void *priv, struct clk *clk)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;
	struct imx6_clk *parent = NULL;
	struct imx6ccm_softc *sc = priv;

	switch (iclk->type) {
	case IMX6_CLK_FIXED:
	case IMX6_CLK_FIXED_FACTOR:
	case IMX6_CLK_PLL:
	case IMX6_CLK_GATE:
	case IMX6_CLK_DIV:
	case IMX6_CLK_PFD:
		if (iclk->parent != NULL)
			parent = imx6_clk_find(sc, iclk->parent);
		break;
	case IMX6_CLK_MUX:
		parent = imxccm_clk_get_parent_mux(sc, iclk);
		break;
	default:
		panic("imx6: unknown clk type %d", iclk->type);
	}

	return (struct clk *)parent;
}
