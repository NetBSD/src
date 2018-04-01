/* $NetBSD: sunxi_ccu_fractional.c,v 1.2 2018/04/01 21:19:17 bouyer Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_ccu_fractional.c,v 1.2 2018/04/01 21:19:17 bouyer Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/sunxi/sunxi_ccu.h>

int
sunxi_ccu_fractional_enable(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, int enable)
{
	struct sunxi_ccu_fractional *fractional = &clk->u.fractional;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_FRACTIONAL);

	if (!fractional->enable)
		return enable ? 0 : EINVAL;

	val = CCU_READ(sc, fractional->reg);
	if (enable)
		val |= fractional->enable;
	else
		val &= ~fractional->enable;
	CCU_WRITE(sc, fractional->reg, val);

	return 0;
}

u_int
sunxi_ccu_fractional_get_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_fractional *fractional = &clk->u.fractional;
	struct clk *clkp, *clkp_parent;
	u_int rate, m;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_FRACTIONAL);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	rate = clk_get_rate(clkp_parent);
	if (rate == 0)
		return 0;

	if (fractional->prediv > 0)
		rate = rate / fractional->prediv;

	val = CCU_READ(sc, fractional->reg);

	if (fractional->enable && !(val & fractional->enable))
		return 0;

	if ((val & fractional->div_en) == 0) {
		int sel = __SHIFTOUT(val, fractional->frac_sel);
		return fractional->frac[sel];
	}
	m = __SHIFTOUT(val, fractional->m);

	return rate * m;
}

int
sunxi_ccu_fractional_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, u_int new_rate)
{
	struct sunxi_ccu_fractional *fractional = &clk->u.fractional;
	struct clk *clkp, *clkp_parent;
	u_int parent_rate, best_rate, best_m;
	u_int m, rate;
	int best_diff;
	uint32_t val;
	int i;

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return ENXIO;

	parent_rate = clk_get_rate(clkp_parent);
	if (parent_rate == 0)
		return (new_rate == 0) ? 0 : ERANGE;

	if (fractional->prediv > 0)
		parent_rate = parent_rate / fractional->prediv;

	val = CCU_READ(sc, fractional->reg);
	for (i = 0; i < __arraycount(fractional->frac); i++) {
		if (fractional->frac[i] == new_rate) {
			val &= ~fractional->div_en;
			val &= ~fractional->frac_sel;
			val |= __SHIFTIN(i, fractional->frac_sel);
			CCU_WRITE(sc, fractional->reg, val);
			return 0;
		}
	}
	val |= fractional->div_en;

	best_rate = 0;
	best_diff = INT_MAX;

	for (m = fractional->m_min; m <= fractional->m_max; m++) {
		rate = parent_rate * m;
		const int diff = abs(new_rate - rate);
		if (diff < best_diff) {
			best_diff = diff;
			best_rate = rate;
			best_m = m;
			if (diff == 0)
				break;
		}
	}

	if (best_rate == 0)
		return ERANGE;

	val &= ~fractional->m;
	val |= __SHIFTIN(best_m, fractional->m);
	CCU_WRITE(sc, fractional->reg, val);

	return 0;
}

u_int
sunxi_ccu_fractional_round_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, u_int try_rate)
{
	struct sunxi_ccu_fractional *fractional = &clk->u.fractional;
	struct clk *clkp, *clkp_parent;
	u_int parent_rate, best_rate;
	u_int m, rate;
	int best_diff;
	int i;

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	parent_rate = clk_get_rate(clkp_parent);
	if (parent_rate == 0)
		return 0;

	if (fractional->prediv > 0)
		parent_rate = parent_rate / fractional->prediv;

	for (i = 0; i < __arraycount(fractional->frac); i++) {
		if (fractional->frac[i] == try_rate) {
			return try_rate;
		}
	}

	best_rate = 0;
	best_diff = INT_MAX;

	for (m = fractional->m_min; m <= fractional->m_max; m++) {
		rate = parent_rate * m;
		const int diff = abs(try_rate - rate);
		if (diff < best_diff) {
			best_diff = diff;
			best_rate = rate;
			if (diff == 0)
				break;
		}
	}

	return best_rate;
}

const char *
sunxi_ccu_fractional_get_parent(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_fractional *fractional = &clk->u.fractional;

	KASSERT(clk->type == SUNXI_CCU_FRACTIONAL);

	return fractional->parent;
}
