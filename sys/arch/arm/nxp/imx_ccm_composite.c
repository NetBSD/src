/* $NetBSD: imx_ccm_composite.c,v 1.1 2020/12/23 14:42:38 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: imx_ccm_composite.c,v 1.1 2020/12/23 14:42:38 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/nxp/imx_ccm.h>

#include <dev/fdt/fdtvar.h>

#define	CCM_TARGET_ROOT		0x00
#define	 TARGET_ROOT_ENABLE	__BIT(28)
#define	 TARGET_ROOT_MUX	__BITS(26,24)
#define	 TARGET_ROOT_PRE_PODF	__BITS(18,16)
#define	 TARGET_ROOT_POST_PODF	__BITS(5,0)

int
imx_ccm_composite_enable(struct imx_ccm_softc *sc, struct imx_ccm_clk *clk,
    int enable)
{
	struct imx_ccm_composite *composite = &clk->u.composite;
	uint32_t val;

	KASSERT(clk->type == IMX_CCM_COMPOSITE);

	val = CCM_READ(sc, clk->regidx, composite->reg + CCM_TARGET_ROOT);
	if (enable)
		val |= TARGET_ROOT_ENABLE;
	else
		val &= ~TARGET_ROOT_ENABLE;
	CCM_WRITE(sc, clk->regidx, composite->reg + CCM_TARGET_ROOT, val);

	return 0;
}

u_int
imx_ccm_composite_get_rate(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk)
{
	struct imx_ccm_composite *composite = &clk->u.composite;
	struct clk *clkp, *clkp_parent;

	KASSERT(clk->type == IMX_CCM_COMPOSITE);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	const u_int prate = clk_get_rate(clkp_parent);
	if (prate == 0)
		return 0;

	const uint32_t val = CCM_READ(sc, clk->regidx, composite->reg + CCM_TARGET_ROOT);
	const u_int pre_div = __SHIFTOUT(val, TARGET_ROOT_PRE_PODF) + 1;
	const u_int post_div = __SHIFTOUT(val, TARGET_ROOT_POST_PODF) + 1;

	return prate / pre_div / post_div;
}

int
imx_ccm_composite_set_rate(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk, u_int rate)
{
	struct imx_ccm_composite *composite = &clk->u.composite;
	u_int best_prediv, best_postdiv, best_diff;
	struct imx_ccm_clk *rclk_parent;
	struct clk *clk_parent;
	uint32_t val;

	KASSERT(clk->type == IMX_CCM_COMPOSITE);

	if (composite->flags & IMX_COMPOSITE_SET_RATE_PARENT) {
		clk_parent = clk_get_parent(&clk->base);
		if (clk_parent == NULL)
			return ENXIO;
		return clk_set_rate(clk_parent, rate);
	}

	best_prediv = 0;
	best_postdiv = 0;
	best_diff = INT_MAX;

	val = CCM_READ(sc, clk->regidx, composite->reg + CCM_TARGET_ROOT);
	const u_int mux = __SHIFTOUT(val, TARGET_ROOT_MUX);

	if (mux >= composite->nparents)
		return EIO;

	rclk_parent = imx_ccm_clock_find(sc, composite->parents[mux]);
	if (rclk_parent != NULL)
		clk_parent = &rclk_parent->base;
	else
		clk_parent = fdtbus_clock_byname(composite->parents[mux]);
	if (clk_parent == NULL)
		return EIO;

	const u_int prate = clk_get_rate(clk_parent);
	if (prate == 0)
		return ERANGE;

	for (u_int prediv = 1; prediv <= __SHIFTOUT_MASK(TARGET_ROOT_PRE_PODF) + 1; prediv++) {
		for (u_int postdiv = 1; postdiv <= __SHIFTOUT_MASK(TARGET_ROOT_POST_PODF) + 1; postdiv++) {
			const u_int cur_rate = prate / prediv / postdiv;
			const int diff = (int)rate - (int)cur_rate;
			if (composite->flags & IMX_COMPOSITE_ROUND_DOWN) {
				if (diff >= 0 && diff < best_diff) {
					best_diff = diff;
					best_prediv = prediv;
					best_postdiv = postdiv;
				}
			} else {
				if (abs(diff) < best_diff) {
					best_diff = abs(diff);
					best_prediv = prediv;
					best_postdiv = postdiv;
				}
			}
		}
	}
	if (best_diff == INT_MAX)
		return ERANGE;

	val = CCM_READ(sc, clk->regidx, composite->reg + CCM_TARGET_ROOT);
	val &= ~TARGET_ROOT_PRE_PODF;
	val |= __SHIFTIN(best_prediv - 1, TARGET_ROOT_PRE_PODF);
	val &= ~TARGET_ROOT_POST_PODF;
	val |= __SHIFTIN(best_postdiv - 1, TARGET_ROOT_POST_PODF);
	CCM_WRITE(sc, clk->regidx, composite->reg + CCM_TARGET_ROOT, val);

	return 0;
}

const char *
imx_ccm_composite_get_parent(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk)
{
	struct imx_ccm_composite *composite = &clk->u.composite;

	KASSERT(clk->type == IMX_CCM_COMPOSITE);

	const uint32_t val = CCM_READ(sc, clk->regidx, composite->reg + CCM_TARGET_ROOT);
	const u_int mux = __SHIFTOUT(val, TARGET_ROOT_MUX);

	if (mux >= composite->nparents)
		return NULL;

	return composite->parents[mux];
}

int
imx_ccm_composite_set_parent(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk, const char *parent)
{
	struct imx_ccm_composite *composite = &clk->u.composite;
	uint32_t val;

	KASSERT(clk->type == IMX_CCM_COMPOSITE);

	for (u_int mux = 0; mux < composite->nparents; mux++) {
		if (strcmp(composite->parents[mux], parent) == 0) {
			val = CCM_READ(sc, clk->regidx, composite->reg + CCM_TARGET_ROOT);
			val &= ~TARGET_ROOT_MUX;
			val |= __SHIFTIN(mux, TARGET_ROOT_MUX);
			CCM_WRITE(sc, clk->regidx, composite->reg + CCM_TARGET_ROOT, val);
			return 0;
		}
	}

	return EINVAL;
}
