/* $NetBSD: meson_clk_pll.c,v 1.2 2019/02/25 19:30:17 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: meson_clk_pll.c,v 1.2 2019/02/25 19:30:17 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/amlogic/meson_clk.h>

u_int
meson_clk_pll_get_rate(struct meson_clk_softc *sc,
    struct meson_clk_clk *clk)
{
	struct meson_clk_pll *pll = &clk->u.pll;
	struct clk *clkp, *clkp_parent;
	u_int n, m, frac;
	uint64_t parent_rate, rate;
	uint32_t val;

	KASSERT(clk->type == MESON_CLK_PLL);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	parent_rate = clk_get_rate(clkp_parent);
	if (parent_rate == 0)
		return 0;

	CLK_LOCK(sc);

	val = CLK_READ(sc, pll->n.reg);
	n = __SHIFTOUT(val, pll->n.mask);

	val = CLK_READ(sc, pll->m.reg);
	m = __SHIFTOUT(val, pll->m.mask);

	if (pll->frac.mask) {
		val = CLK_READ(sc, pll->frac.reg);
		frac = __SHIFTOUT(val, pll->frac.mask);
	} else {
		frac = 0;
	}

	CLK_UNLOCK(sc);

	rate = parent_rate * m;
	if (frac) {
		uint64_t frac_rate = parent_rate * frac;
		rate += howmany(frac_rate, __SHIFTOUT_MASK(pll->frac.mask) + 1);
	}

	return (u_int)howmany(rate, n);
}

const char *
meson_clk_pll_get_parent(struct meson_clk_softc *sc,
    struct meson_clk_clk *clk)
{
	struct meson_clk_pll *pll = &clk->u.pll;

	KASSERT(clk->type == MESON_CLK_PLL);

	return pll->parent;
}
