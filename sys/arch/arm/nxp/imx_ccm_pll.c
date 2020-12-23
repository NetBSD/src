/* $NetBSD: imx_ccm_pll.c,v 1.1 2020/12/23 14:42:38 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: imx_ccm_pll.c,v 1.1 2020/12/23 14:42:38 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/nxp/imx_ccm.h>

#include <dev/fdt/fdtvar.h>

#define	PLL_POWERDOWN		__BIT(12)
#define	PLL_POWERDOWN_ENET	__BIT(5)

int
imx_ccm_pll_enable(struct imx_ccm_softc *sc, struct imx_ccm_clk *clk,
    int enable)
{
	struct imx_ccm_pll *pll = &clk->u.pll;
	uint32_t val, mask;

	KASSERT(clk->type == IMX_CCM_PLL);

	if ((pll->flags & IMX_PLL_ENET) != 0)
		mask = PLL_POWERDOWN_ENET;
	else
		mask = PLL_POWERDOWN;

	val = CCM_READ(sc, clk->regidx, pll->reg);
	if (enable)
		val &= ~mask;
	else
		val |= mask;
	CCM_WRITE(sc, clk->regidx, pll->reg, val);

	return 0;
}

u_int
imx_ccm_pll_get_rate(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk)
{
	struct imx_ccm_pll *pll= &clk->u.pll;
	struct clk *clkp, *clkp_parent;

	KASSERT(clk->type == IMX_CCM_PLL);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	const u_int prate = clk_get_rate(clkp_parent);
	if (prate == 0)
		return 0;

	if ((pll->flags & IMX_PLL_ENET) != 0) {
		/* For ENET PLL, div_mask contains the fixed output rate */
		return pll->div_mask;
	}

	const uint32_t val = CCM_READ(sc, clk->regidx, pll->reg);
	const u_int div = __SHIFTOUT(val, pll->div_mask);

	if ((pll->flags & IMX_PLL_ARM) != 0) {
		return prate * div / 2;
	}

	if ((pll->flags & IMX_PLL_480M_528M) != 0) {
		return div == 1 ? 528000000 : 480000000;
	}

	return 0;
}

const char *
imx_ccm_pll_get_parent(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk)
{
	struct imx_ccm_pll *pll = &clk->u.pll;

	KASSERT(clk->type == IMX_CCM_PLL);

	return pll->parent;
}
