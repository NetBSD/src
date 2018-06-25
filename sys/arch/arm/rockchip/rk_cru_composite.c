/* $NetBSD: rk_cru_composite.c,v 1.3.2.2 2018/06/25 07:25:39 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: rk_cru_composite.c,v 1.3.2.2 2018/06/25 07:25:39 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/rockchip/rk_cru.h>

#include <dev/fdt/fdtvar.h>

int
rk_cru_composite_enable(struct rk_cru_softc *sc, struct rk_cru_clk *clk,
    int enable)
{
	struct rk_cru_composite *composite = &clk->u.composite;

	KASSERT(clk->type == RK_CRU_COMPOSITE);

	if (composite->gate_mask == 0)
		return enable ? 0 : ENXIO;

	const uint32_t write_mask = composite->gate_mask << 16;
	const uint32_t write_val = enable ? 0 : composite->gate_mask;

	CRU_WRITE(sc, composite->gate_reg, write_mask | write_val);

	return 0;
}

u_int
rk_cru_composite_get_rate(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk)
{
	struct rk_cru_composite *composite = &clk->u.composite;
	struct clk *clkp, *clkp_parent;

	KASSERT(clk->type == RK_CRU_COMPOSITE);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	const u_int prate = clk_get_rate(clkp_parent);
	if (prate == 0)
		return 0;

	const uint32_t val = CRU_READ(sc, composite->muxdiv_reg);
	const u_int div = __SHIFTOUT(val, composite->div_mask) + 1;

	return prate / div;
}

int
rk_cru_composite_set_rate(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk, u_int rate)
{
	struct rk_cru_composite *composite = &clk->u.composite;
	u_int best_div, best_mux, best_diff;
	struct rk_cru_clk *rclk_parent;
	struct clk *clk_parent;

	KASSERT(clk->type == RK_CRU_COMPOSITE);

	best_div = 0;
	best_mux = 0;
	best_diff = INT_MAX;
	for (u_int mux = 0; mux < composite->nparents; mux++) {
		rclk_parent = rk_cru_clock_find(sc, composite->parents[mux]);
		if (rclk_parent != NULL)
			clk_parent = &rclk_parent->base;
		else
			clk_parent = fdtbus_clock_byname(composite->parents[mux]);
		if (clk_parent == NULL)
			continue;

		const u_int prate = clk_get_rate(clk_parent);
		if (prate == 0)
			continue;

		for (u_int div = 1; div <= __SHIFTOUT_MASK(composite->div_mask) + 1; div++) {
			const u_int cur_rate = prate / div;
			const int diff = (int)rate - (int)cur_rate;
			if (composite->flags & RK_COMPOSITE_ROUND_DOWN) {
				if (diff >= 0 && diff < best_diff) {
					best_diff = diff;
					best_mux = mux;
					best_div = div;
				}
			} else {
				if (abs(diff) < best_diff) {
					best_diff = abs(diff);
					best_mux = mux;
					best_div = div;
				}
			}
		}
	}
	if (best_diff == INT_MAX)
		return ERANGE;

	uint32_t write_mask = composite->div_mask << 16;
	uint32_t write_val = __SHIFTIN(best_div - 1, composite->div_mask);
	if (composite->mux_mask) {
		write_mask |= composite->mux_mask << 16;
		write_val |= __SHIFTIN(best_mux, composite->mux_mask);
	}

	CRU_WRITE(sc, composite->muxdiv_reg, write_mask | write_val);

	return 0;
}

const char *
rk_cru_composite_get_parent(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk)
{
	struct rk_cru_composite *composite = &clk->u.composite;
	uint32_t val;
	u_int mux;

	KASSERT(clk->type == RK_CRU_COMPOSITE);

	if (composite->mux_mask) {
		val = CRU_READ(sc, composite->muxdiv_reg);
		mux = __SHIFTOUT(val, composite->mux_mask);
	} else {
		mux = 0;
	}

	return composite->parents[mux];
}

int
rk_cru_composite_set_parent(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk, const char *parent)
{
	struct rk_cru_composite *composite = &clk->u.composite;

	KASSERT(clk->type == RK_CRU_COMPOSITE);

	if (!composite->mux_mask)
		return EINVAL;

	for (u_int mux = 0; mux < composite->nparents; mux++) {
		if (strcmp(composite->parents[mux], parent) == 0) {
			const uint32_t write_mask = composite->mux_mask << 16;
			const uint32_t write_val = __SHIFTIN(mux, composite->mux_mask);

			CRU_WRITE(sc, composite->muxdiv_reg, write_mask | write_val);
			return 0;
		}
	}

	return EINVAL;
}
