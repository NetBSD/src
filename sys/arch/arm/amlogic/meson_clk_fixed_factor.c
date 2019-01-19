/* $NetBSD: meson_clk_fixed_factor.c,v 1.1 2019/01/19 20:56:03 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: meson_clk_fixed_factor.c,v 1.1 2019/01/19 20:56:03 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/amlogic/meson_clk.h>

static u_int
meson_clk_fixed_factor_get_parent_rate(struct clk *clkp)
{
	struct clk *clkp_parent;

	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	return clk_get_rate(clkp_parent);
}

u_int
meson_clk_fixed_factor_get_rate(struct meson_clk_softc *sc,
    struct meson_clk_clk *clk)
{
	struct meson_clk_fixed_factor *fixed_factor = &clk->u.fixed_factor;
	struct clk *clkp = &clk->base;

	KASSERT(clk->type == MESON_CLK_FIXED_FACTOR);

	const u_int p_rate = meson_clk_fixed_factor_get_parent_rate(clkp);
	if (p_rate == 0)
		return 0;

	return (u_int)(((uint64_t)p_rate * fixed_factor->mult) / fixed_factor->div);
}

static int
meson_clk_fixed_factor_set_parent_rate(struct clk *clkp, u_int rate)
{
	struct clk *clkp_parent;

	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return ENXIO;

	return clk_set_rate(clkp_parent, rate);
}

int
meson_clk_fixed_factor_set_rate(struct meson_clk_softc *sc,
    struct meson_clk_clk *clk, u_int rate)
{
	struct meson_clk_fixed_factor *fixed_factor = &clk->u.fixed_factor;
	struct clk *clkp = &clk->base;

	KASSERT(clk->type == MESON_CLK_FIXED_FACTOR);

	rate *= fixed_factor->div;
	rate /= fixed_factor->mult;

	return meson_clk_fixed_factor_set_parent_rate(clkp, rate);
}

const char *
meson_clk_fixed_factor_get_parent(struct meson_clk_softc *sc,
    struct meson_clk_clk *clk)
{
	struct meson_clk_fixed_factor *fixed_factor = &clk->u.fixed_factor;

	KASSERT(clk->type == MESON_CLK_FIXED_FACTOR);

	return fixed_factor->parent;
}
