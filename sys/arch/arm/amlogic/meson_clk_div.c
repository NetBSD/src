/* $NetBSD: meson_clk_div.c,v 1.3 2019/02/25 19:30:17 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: meson_clk_div.c,v 1.3 2019/02/25 19:30:17 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/amlogic/meson_clk.h>

u_int
meson_clk_div_get_rate(struct meson_clk_softc *sc,
    struct meson_clk_clk *clk)
{
	struct meson_clk_div *div = &clk->u.div;
	struct clk *clkp, *clkp_parent;
	u_int rate, ratio;
	uint32_t val;

	KASSERT(clk->type == MESON_CLK_DIV);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	rate = clk_get_rate(clkp_parent);
	if (rate == 0)
		return 0;

	CLK_LOCK(sc);
	val = CLK_READ(sc, div->reg);
	CLK_UNLOCK(sc);

	if (div->div)
		ratio = __SHIFTOUT(val, div->div);
	else
		ratio = 0;

	if (div->flags & MESON_CLK_DIV_POWER_OF_TWO) {
		return rate >> ratio;
	} else if (div->flags & MESON_CLK_DIV_CPU_SCALE_TABLE) {
		if (ratio < 1 || ratio > 8)
			return 0;
		return rate / ((ratio + 1) * 2);
	} else {
		return rate / (ratio + 1);
	}
}

int
meson_clk_div_set_rate(struct meson_clk_softc *sc,
    struct meson_clk_clk *clk, u_int new_rate)
{
	struct meson_clk_div *div = &clk->u.div;
	struct clk *clkp, *clkp_parent;
	int parent_rate;
	uint32_t val, raw_div;
	int ratio, error;

	KASSERT(clk->type == MESON_CLK_DIV);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return ENXIO;

	if ((div->flags & MESON_CLK_DIV_SET_RATE_PARENT) != 0)
		return clk_set_rate(clkp_parent, new_rate);

	if (div->div == 0)
		return ENXIO;

	CLK_LOCK(sc);

	val = CLK_READ(sc, div->reg);

	parent_rate = clk_get_rate(clkp_parent);
	if (parent_rate == 0) {
		error = (new_rate == 0) ? 0 : ERANGE;
		goto done;
	}

	ratio = howmany(parent_rate, new_rate);
	if ((div->flags & MESON_CLK_DIV_POWER_OF_TWO) != 0) {
		error = EINVAL;
		goto done;
	} else if ((div->flags & MESON_CLK_DIV_CPU_SCALE_TABLE) != 0) {
		error = EINVAL;
		goto done;
	} else {
		raw_div = (ratio > 0) ? ratio - 1 : 0;
	}
	if (raw_div > __SHIFTOUT_MASK(div->div)) {
		error = ERANGE;
		goto done;
	}

	val &= ~div->div;
	val |= __SHIFTIN(raw_div, div->div);
	CLK_WRITE(sc, div->reg, val);

	error = 0;

done:
	CLK_UNLOCK(sc);

	return error;
}

const char *
meson_clk_div_get_parent(struct meson_clk_softc *sc,
    struct meson_clk_clk *clk)
{
	struct meson_clk_div *div = &clk->u.div;

	KASSERT(clk->type == MESON_CLK_DIV);

	return div->parent;
}
