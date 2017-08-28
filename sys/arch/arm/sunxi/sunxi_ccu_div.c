/* $NetBSD: sunxi_ccu_div.c,v 1.2.2.2 2017/08/28 17:51:32 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_ccu_div.c,v 1.2.2.2 2017/08/28 17:51:32 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/sunxi/sunxi_ccu.h>

u_int
sunxi_ccu_div_get_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_div *div = &clk->u.div;
	struct clk *clkp, *clkp_parent;
	u_int rate, ratio;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_DIV);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	rate = clk_get_rate(clkp_parent);
	if (rate == 0)
		return 0;

	val = CCU_READ(sc, div->reg);
	if (div->div)
		ratio = __SHIFTOUT(val, div->div);
	else
		ratio = 0;

	if ((div->flags & SUNXI_CCU_DIV_ZERO_IS_ONE) != 0 && ratio == 0)
		ratio = 1;
	if (div->flags & SUNXI_CCU_DIV_POWER_OF_TWO)
		ratio = 1 << ratio;
	else
		ratio++;

	return rate / ratio;
}

int
sunxi_ccu_div_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, u_int new_rate)
{
	return EINVAL;
}

int
sunxi_ccu_div_set_parent(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, const char *name)
{
	struct sunxi_ccu_div *div = &clk->u.div;
	uint32_t val;
	u_int index;

	KASSERT(clk->type == SUNXI_CCU_DIV);

	if (div->sel == 0)
		return ENODEV;

	for (index = 0; index < div->nparents; index++) {
		if (div->parents[index] != NULL &&
		    strcmp(div->parents[index], name) == 0)
			break;
	}
	if (index == div->nparents)
		return EINVAL;

	val = CCU_READ(sc, div->reg);
	val &= ~div->sel;
	val |= __SHIFTIN(index, div->sel);
	CCU_WRITE(sc, div->reg, val);

	return 0;
}

const char *
sunxi_ccu_div_get_parent(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_div *div = &clk->u.div;
	u_int index;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_DIV);

	if (div->sel == 0)
		return div->parents[0];

	val = CCU_READ(sc, div->reg);
	index = __SHIFTOUT(val, div->sel);

	return div->parents[index];
}
