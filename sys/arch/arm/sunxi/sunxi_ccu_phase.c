/* $NetBSD: sunxi_ccu_phase.c,v 1.1.8.2 2017/12/03 11:35:56 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_ccu_phase.c,v 1.1.8.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/sunxi/sunxi_ccu.h>

static u_int
sunxi_ccu_phase_get_parent_rate(struct clk *clkp)
{
	struct clk *clkp_parent;

	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	return clk_get_rate(clkp_parent);
}

static u_int
sunxi_ccu_phase_div(u_int n, u_int d)
{
	return (n + (d/2)) / d;
}

u_int
sunxi_ccu_phase_get_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_phase *phase = &clk->u.phase;
	struct clk *clkp = &clk->base;
	u_int p_rate, gp_rate, p_div, delay;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_PHASE);

	p_rate = sunxi_ccu_phase_get_parent_rate(clkp);
	if (p_rate == 0)
		return 0;
	gp_rate = sunxi_ccu_phase_get_parent_rate(clk_get_parent(clkp));
	if (gp_rate == 0)
		return 0;

	p_div = gp_rate / p_rate;

	val = CCU_READ(sc, phase->reg);
	delay = __SHIFTOUT(val, phase->mask);

	return delay * sunxi_ccu_phase_div(360, p_div);
}

int
sunxi_ccu_phase_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, u_int new_rate)
{
	struct sunxi_ccu_phase *phase = &clk->u.phase;
	struct clk *clkp = &clk->base;
	u_int p_rate, gp_rate, p_div, delay;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_PHASE);

	clkp = &clk->base;

	p_rate = sunxi_ccu_phase_get_parent_rate(clkp);
	if (p_rate == 0)
		return 0;
	gp_rate = sunxi_ccu_phase_get_parent_rate(clk_get_parent(clkp));
	if (gp_rate == 0)
		return 0;

	p_div = gp_rate / p_rate;

	delay = new_rate == 180 ? 0 :
	    sunxi_ccu_phase_div(new_rate,
				sunxi_ccu_phase_div(360, p_div));

	val = CCU_READ(sc, phase->reg);
	val &= ~phase->mask;
	val |= __SHIFTIN(delay, phase->mask);
	CCU_WRITE(sc, phase->reg, val);

	return 0;
}

const char *
sunxi_ccu_phase_get_parent(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_phase *phase = &clk->u.phase;

	KASSERT(clk->type == SUNXI_CCU_PHASE);

	return phase->parent;
}
