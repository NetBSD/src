/* $NetBSD: rk_cru_arm.c,v 1.1.2.2 2018/06/25 07:25:39 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: rk_cru_arm.c,v 1.1.2.2 2018/06/25 07:25:39 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/rockchip/rk_cru.h>

u_int
rk_cru_arm_get_rate(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk)
{
	struct rk_cru_arm *arm = &clk->u.arm;
	struct clk *clkp, *clkp_parent;

	KASSERT(clk->type == RK_CRU_ARM);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	const u_int fref = clk_get_rate(clkp_parent);
	if (fref == 0)
		return 0;

	const uint32_t val = CRU_READ(sc, arm->reg);
	const u_int div = __SHIFTOUT(val, arm->div_mask) + 1;

	return fref / div;
}

int
rk_cru_arm_set_rate(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk, u_int rate)
{
	struct rk_cru_arm *arm = &clk->u.arm;
	const struct rk_cru_arm_rate *arm_rate = NULL;
	struct clk *clkp_parent;
	int error;

	KASSERT(clk->type == RK_CRU_ARM);

	if (arm->rates == NULL || rate == 0)
		return EIO;

	for (int i = 0; i < arm->nrates; i++)
		if (arm->rates[i].rate == rate) {
			arm_rate = &arm->rates[i];
			break;
		}
	if (arm_rate == NULL)
		return EINVAL;

	error = rk_cru_arm_set_parent(sc, clk, arm->parents[arm->mux_main]);
	if (error != 0)
		return error;

	clkp_parent = clk_get_parent(&clk->base);
	if (clkp_parent == NULL)
		return ENXIO;

	const u_int parent_rate = arm_rate->rate / arm_rate->div;

	error = clk_set_rate(clkp_parent, parent_rate);
	if (error != 0)
		return error;

	const uint32_t write_mask = arm->div_mask << 16;
	const uint32_t write_val = __SHIFTIN(arm_rate->div - 1, arm->div_mask);

	CRU_WRITE(sc, arm->reg, write_mask | write_val);

	return 0;
}

const char *
rk_cru_arm_get_parent(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk)
{
	struct rk_cru_arm *arm = &clk->u.arm;

	KASSERT(clk->type == RK_CRU_ARM);

	const uint32_t val = CRU_READ(sc, arm->reg);
	const u_int mux = __SHIFTOUT(val, arm->mux_mask);

	return arm->parents[mux];
}

int
rk_cru_arm_set_parent(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk, const char *parent)
{
	struct rk_cru_arm *arm = &clk->u.arm;

	KASSERT(clk->type == RK_CRU_ARM);

	for (u_int mux = 0; mux < arm->nparents; mux++)
		if (strcmp(arm->parents[mux], parent) == 0) {
			const uint32_t write_mask = arm->mux_mask << 16;
			const uint32_t write_val = __SHIFTIN(mux, arm->mux_mask);

			CRU_WRITE(sc, arm->reg, write_mask | write_val);
			return 0;
		}

	return EINVAL;
}
