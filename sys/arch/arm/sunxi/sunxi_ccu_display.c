/* $NetBSD: sunxi_ccu_display.c,v 1.2 2018/04/02 20:55:49 bouyer Exp $ */

/*-
 * Copyright (c) 2018 Manuel Bouyer <bouyer@antioche.eu.org>
 * All rights reserved.
 *
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: sunxi_ccu_display.c,v 1.2 2018/04/02 20:55:49 bouyer Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/sunxi/sunxi_ccu.h>

int
sunxi_ccu_lcdxch0_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, struct sunxi_ccu_clk *pllclk,
    struct sunxi_ccu_clk *pllclk_x2, u_int new_rate)
{
	struct clk *clkp;
	int error;
	int diff, diff_x2;
	int rate, rate_x2;

	clkp = &pllclk->base;
	rate = clk_round_rate(clkp, new_rate);
	diff = abs(new_rate - rate);

	rate_x2 = (clk_round_rate(clkp, new_rate / 2) * 2);
	diff_x2 = abs(new_rate - rate_x2);

	if (rate == 0 && rate_x2 == 0)
		return ERANGE;

	if (diff_x2 < diff) {
		error = clk_set_rate(clkp, new_rate / 2);
		KASSERT(error == 0);
		error = clk_set_parent(&clk->base, &pllclk_x2->base);
		KASSERT(error == 0);
	} else {
		error = clk_set_rate(clkp, new_rate);
		KASSERT(error == 0);
		error = clk_set_parent(&clk->base, clkp);
		KASSERT(error == 0);
	}
	(void)error;
	return 0;
}

u_int
sunxi_ccu_lcdxch0_round_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk * clk, struct sunxi_ccu_clk *pllclk,
    struct sunxi_ccu_clk *pllclk_x2, u_int try_rate)
{
	struct clk *clkp;
	int diff, diff_x2;
	int rate, rate_x2;

	clkp = &pllclk->base;
	rate = clk_round_rate(clkp, try_rate);
	diff = abs(try_rate - rate);

	rate_x2 = (clk_round_rate(clkp, try_rate / 2) * 2);
	diff_x2 = abs(try_rate - rate_x2);

	if (diff_x2 < diff)
		return rate_x2;
	return rate;
}

int
sunxi_ccu_lcdxch1_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk * clk, struct sunxi_ccu_clk *pllclk,
    struct sunxi_ccu_clk *pllclk_x2, u_int new_rate)
{
	struct clk *clkp, *pllclkp;
	int best_diff;
	int parent_rate, best_parent_rate;
	uint32_t best_m, best_d;
	int error;

	pllclkp = clkp = &pllclk->base;
	best_diff = INT_MAX;
	best_m = best_d = 0;
	for (uint32_t d = 1; d <= 2 && best_diff != 0; d++) {
		for (uint32_t m = 1; m <= 16 && best_diff != 0; m++) {
			int rate, diff;
			parent_rate = clk_round_rate(pllclkp, 
			    new_rate * m / d);
			if (parent_rate == 0)
				continue;
			rate = parent_rate * d / m;
			diff = abs(rate - new_rate);
			if (diff < best_diff) {
				best_diff = diff;
				best_m = m;
				best_d = d;
				best_parent_rate = parent_rate;
			}
		}
	}
	if (best_m == 0)
		return ERANGE;

	if (best_d == 2)
		clkp = &pllclk_x2->base;

	error = clk_set_rate(pllclkp, best_parent_rate);
	KASSERT(error == 0);
	error = clk_set_parent(&clk->base, clkp);
	KASSERT(error == 0);
	error = clk_enable(clkp);
	KASSERT(error == 0);
	error = sunxi_ccu_div_set_rate(sc, clk, new_rate);
	KASSERT(error == 0);
	return error;
}
