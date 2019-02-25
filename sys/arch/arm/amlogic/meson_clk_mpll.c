/* $NetBSD: meson_clk_mpll.c,v 1.2 2019/02/25 19:30:17 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: meson_clk_mpll.c,v 1.2 2019/02/25 19:30:17 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/amlogic/meson_clk.h>

#define	SDM_DEN		0x4000

u_int
meson_clk_mpll_get_rate(struct meson_clk_softc *sc,
    struct meson_clk_clk *clk)
{
	struct meson_clk_mpll *mpll = &clk->u.mpll;
	struct clk *clkp, *clkp_parent;
	uint64_t parent_rate, sdm, n2;
	uint32_t val;

	KASSERT(clk->type == MESON_CLK_MPLL);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	parent_rate = clk_get_rate(clkp_parent);
	if (parent_rate == 0)
		return 0;

	CLK_LOCK(sc);

	val = CLK_READ(sc, mpll->sdm.reg);
	sdm = __SHIFTOUT(val, mpll->sdm.mask);

	val = CLK_READ(sc, mpll->n2.reg);
	n2 = __SHIFTOUT(val, mpll->n2.mask);

	CLK_UNLOCK(sc);

	const uint64_t div = (SDM_DEN * n2) + sdm;
	if (div == 0)
		return 0;

	return (u_int)howmany(parent_rate * SDM_DEN, div);
}

const char *
meson_clk_mpll_get_parent(struct meson_clk_softc *sc,
    struct meson_clk_clk *clk)
{
	struct meson_clk_mpll *mpll = &clk->u.mpll;

	KASSERT(clk->type == MESON_CLK_MPLL);

	return mpll->parent;
}
