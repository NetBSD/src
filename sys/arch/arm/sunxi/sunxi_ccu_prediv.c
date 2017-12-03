/* $NetBSD: sunxi_ccu_prediv.c,v 1.3.4.2 2017/12/03 11:35:56 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_ccu_prediv.c,v 1.3.4.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/sunxi/sunxi_ccu.h>

u_int
sunxi_ccu_prediv_get_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_prediv *prediv = &clk->u.prediv;
	struct clk *clkp, *clkp_parent;
	u_int rate, pre, div, sel;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_PREDIV);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	rate = clk_get_rate(clkp_parent);
	if (rate == 0)
		return 0;

	val = CCU_READ(sc, prediv->reg);
	if (prediv->prediv)
		pre = __SHIFTOUT(val, prediv->prediv);
	else
		pre = 0;
	if (prediv->div)
		div = __SHIFTOUT(val, prediv->div);
	else
		div = 0;
	sel = __SHIFTOUT(val, prediv->sel);

	if (prediv->flags & SUNXI_CCU_PREDIV_POWER_OF_TWO)
		div = 1 << div;
	else
		div++;

	if (prediv->prediv_fixed)
		pre = prediv->prediv_fixed;
	else
		pre++;

	if (prediv->flags & SUNXI_CCU_PREDIV_DIVIDE_BY_TWO)
		pre *= 2;

	if (prediv->prediv_sel & __BIT(sel))
		return rate / pre / div;
	else
		return rate / div;
}

int
sunxi_ccu_prediv_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, u_int new_rate)
{
	return EINVAL;
}

int
sunxi_ccu_prediv_set_parent(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, const char *name)
{
	struct sunxi_ccu_prediv *prediv = &clk->u.prediv;
	uint32_t val;
	u_int index;

	KASSERT(clk->type == SUNXI_CCU_PREDIV);

	if (prediv->sel == 0)
		return ENODEV;

	for (index = 0; index < prediv->nparents; index++) {
		if (prediv->parents[index] != NULL &&
		    strcmp(prediv->parents[index], name) == 0)
			break;
	}
	if (index == prediv->nparents)
		return EINVAL;

	val = CCU_READ(sc, prediv->reg);
	val &= ~prediv->sel;
	val |= __SHIFTIN(index, prediv->sel);
	CCU_WRITE(sc, prediv->reg, val);

	return 0;
}

const char *
sunxi_ccu_prediv_get_parent(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_prediv *prediv = &clk->u.prediv;
	u_int index;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_PREDIV);

	val = CCU_READ(sc, prediv->reg);
	index = __SHIFTOUT(val, prediv->sel);

	return prediv->parents[index];
}
