/* $NetBSD: imx_ccm_div.c,v 1.5 2026/11/26 03:39:30 hkenken Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: imx_ccm_div.c,v 1.5 2026/11/26 03:39:30 hkenken Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/nxp/imx_ccm.h>

static int
imx_ccm_clk_wait(struct imx_ccm_softc *sc, struct imx_ccm_clk *clk,
    bus_size_t busy_reg, uint32_t busy_mask)
{
	int tout = 0;

	KASSERT(busy_mask != 0);

	while (CCM_READ(sc, clk->regidx, busy_reg) & busy_mask) {
		if (++tout >= 1000)
			return ETIMEDOUT;
		delay(10);
	}

	return 0;
}

u_int
imx_ccm_div_get_rate(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk)
{
	struct imx_ccm_div *div = &clk->u.div;
	struct clk *clkp, *clkp_parent;

	KASSERT(clk->type == IMX_CCM_DIV);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	const u_int prate = clk_get_rate(clkp_parent);
	if (prate == 0)
		return 0;

	const uint32_t val = CCM_READ(sc, clk->regidx, div->reg);
	const u_int n = __SHIFTOUT(val, div->mask);

	if ((div->flags & IMX_DIV_ONE_BASED) != 0) {
		if (n == 0)
			return 0;
		else
			return prate / n;
	}

	return prate / (n + 1);
}

int
imx_ccm_div_set_rate(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk, u_int rate)
{
	struct imx_ccm_div *div = &clk->u.div;
	struct clk *clkp, *clkp_parent;
	u_int best_diff = 0;
	u_int n, best_n;
	uint32_t val;
	int ret;

	KASSERT(clk->type == IMX_CCM_DIV);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return ENXIO;

	if ((div->flags & IMX_DIV_SET_RATE_PARENT) != 0)
		return clk_set_rate(clkp_parent, rate);

	const u_int prate = clk_get_rate(clkp_parent);
	if (prate == 0)
		return EIO;

	u_int base = (div->flags & IMX_DIV_ONE_BASED) ? 1 : 0;
	bool found = false;
	for (n = base; n <= __SHIFTOUT_MASK(div->mask); n++) {
		const u_int tmp_rate = prate / (n + 1 - base);
		if (div->flags & IMX_DIV_ROUND_DOWN)
			if (rate < tmp_rate)
				continue;

		const u_int diff = (rate > tmp_rate) ? rate - tmp_rate : tmp_rate - rate;
		if (!found || diff < best_diff) {
			best_n = n;
			best_diff = diff;
			found = true;
		}
	}
	if (!found)
		return EIO;

	if (div->busy_mask != 0) {
		ret = imx_ccm_clk_wait(sc, clk, div->busy_reg, div->busy_mask);
		if (ret)
			return EBUSY;
	}

	val = CCM_READ(sc, clk->regidx, div->reg);
	val &= ~div->mask;
	val |= __SHIFTIN(best_n, div->mask);
	CCM_WRITE(sc, clk->regidx, div->reg, val);

	if (div->busy_mask != 0) {
		ret = imx_ccm_clk_wait(sc, clk, div->busy_reg, div->busy_mask);
		if (ret)
			return ETIMEDOUT;
	}

	return 0;
}

const char *
imx_ccm_div_get_parent(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk)
{
	struct imx_ccm_div *div = &clk->u.div;

	KASSERT(clk->type == IMX_CCM_DIV);

	return div->parent;
}
