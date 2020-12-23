/* $NetBSD: imx_ccm_div.c,v 1.1 2020/12/23 14:42:38 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: imx_ccm_div.c,v 1.1 2020/12/23 14:42:38 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/nxp/imx_ccm.h>

#include <dev/fdt/fdtvar.h>

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

	return prate / (n + 1);
}

int
imx_ccm_div_set_rate(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk, u_int rate)
{
	struct imx_ccm_div *div = &clk->u.div;
	struct clk *clkp, *clkp_parent;
	int best_diff = INT_MAX;
	u_int n, best_n;
	uint32_t val;

	KASSERT(clk->type == IMX_CCM_DIV);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return ENXIO;

	if ((div->flags & IMX_DIV_SET_RATE_PARENT) != 0)
		return clk_set_rate(clkp, rate);

	const u_int prate = clk_get_rate(clkp_parent);
	if (prate == 0)
		return EIO;

	for (n = 0; n < __SHIFTOUT_MASK(div->mask); n++) {
		const u_int tmp_rate = prate / (n + 1);
		const int diff = (int)rate - (int)tmp_rate;
		if ((div->flags & IMX_DIV_ROUND_DOWN) != 0) {
			if (diff >= 0 && diff < best_diff) {
				best_n = n;
				best_diff = diff;
			}
		} else {
			if (abs(diff) < abs(best_diff)) {
				best_n = n;
				best_diff = abs(diff);
			}
		}
	}
	if (best_diff == INT_MAX)
		return EIO;

	val = CCM_READ(sc, clk->regidx, div->reg);
	val &= ~div->mask;
	val |= __SHIFTIN(best_n, div->mask);
	CCM_WRITE(sc, clk->regidx, div->reg, val);

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
