/* $NetBSD: sunxi_ccu_nm.c,v 1.4 2017/10/28 13:13:45 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_ccu_nm.c,v 1.4 2017/10/28 13:13:45 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/sunxi/sunxi_ccu.h>

int
sunxi_ccu_nm_enable(struct sunxi_ccu_softc *sc, struct sunxi_ccu_clk *clk,
    int enable)
{
	struct sunxi_ccu_nm *nm = &clk->u.nm;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_NM);

	if (!nm->enable)
		return enable ? 0 : EINVAL;

	val = CCU_READ(sc, nm->reg);
	if (enable)
		val |= nm->enable;
	else
		val &= ~nm->enable;
	CCU_WRITE(sc, nm->reg, val);

	return 0;
}

u_int
sunxi_ccu_nm_get_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_nm *nm = &clk->u.nm;
	struct clk *clkp, *clkp_parent;
	u_int rate, n, m;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_NM);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	rate = clk_get_rate(clkp_parent);
	if (rate == 0)
		return 0;

	val = CCU_READ(sc, nm->reg);
	n = __SHIFTOUT(val, nm->n);
	m = __SHIFTOUT(val, nm->m);

	if (nm->enable && !(val & nm->enable))
		return 0;

	if (nm->flags & SUNXI_CCU_NM_POWER_OF_TWO)
		n = 1 << n;
	else
		n++;

	m++;

	if (nm->flags & SUNXI_CCU_NM_DIVIDE_BY_TWO)
		m *= 2;

	return rate / n / m;
}

int
sunxi_ccu_nm_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, u_int new_rate)
{
	struct sunxi_ccu_nm *nm = &clk->u.nm;
	struct clk *clkp, *clkp_parent;
	u_int parent_rate, best_rate, best_n, best_m, best_parent;
	u_int n, m, pindex, rate;
	int best_diff;
	uint32_t val;

	const u_int n_max = __SHIFTOUT(nm->n, nm->n);
	const u_int m_max = __SHIFTOUT(nm->m, nm->m);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	rate = clk_get_rate(clkp_parent);
	if (rate == 0)
		return 0;

	best_rate = 0;
	best_diff = INT_MAX;
	for (pindex = 0; pindex < nm->nparents; pindex++) {
		/* XXX
		 * Shouldn't have to set parent to get potential parent clock rate
		 */
		val = CCU_READ(sc, nm->reg);
		val &= ~nm->sel;
		val |= __SHIFTIN(pindex, nm->sel);
		CCU_WRITE(sc, nm->reg, val);

		clkp_parent = clk_get_parent(clkp);
		if (clkp_parent == NULL)
			continue;
		parent_rate = clk_get_rate(clkp_parent);
		if (parent_rate == 0)
			continue;

		for (n = 0; n <= n_max; n++) {
			for (m = 0; m <= m_max; m++) {
				if (nm->flags & SUNXI_CCU_NM_POWER_OF_TWO)
					rate = parent_rate / (1 << n) / (m + 1);
				else
					rate = parent_rate / (n + 1) / (m + 1);
				if (nm->flags & SUNXI_CCU_NM_DIVIDE_BY_TWO)
					rate /= 2;

				if (nm->flags & SUNXI_CCU_NM_ROUND_DOWN) {
					const int diff = new_rate - rate;
					if (diff >= 0 && rate > best_rate) {
						best_diff = diff;
						best_rate = rate;
						best_n = n;
						best_m = m;
						best_parent = pindex;
					}
				} else {
					const int diff = abs(new_rate - rate);
					if (diff < best_diff) {
						best_diff = diff;
						best_rate = rate;
						best_n = n;
						best_m = m;
						best_parent = pindex;
					}
				}
			}
		}
	}

	if (best_rate == 0)
		return ERANGE;

	val = CCU_READ(sc, nm->reg);
	val &= ~nm->sel;
	val |= __SHIFTIN(best_parent, nm->sel);
	val &= ~nm->n;
	val |= __SHIFTIN(best_n, nm->n);
	val &= ~nm->m;
	val |= __SHIFTIN(best_m, nm->m);
	CCU_WRITE(sc, nm->reg, val);

	return 0;
}

int
sunxi_ccu_nm_set_parent(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, const char *name)
{
	struct sunxi_ccu_nm *nm = &clk->u.nm;
	uint32_t val;
	u_int index;

	KASSERT(clk->type == SUNXI_CCU_NM);

	if (nm->sel == 0)
		return ENODEV;

	for (index = 0; index < nm->nparents; index++) {
		if (nm->parents[index] != NULL &&
		    strcmp(nm->parents[index], name) == 0)
			break;
	}
	if (index == nm->nparents)
		return EINVAL;

	val = CCU_READ(sc, nm->reg);
	val &= ~nm->sel;
	val |= __SHIFTIN(index, nm->sel);
	CCU_WRITE(sc, nm->reg, val);

	return 0;
}

const char *
sunxi_ccu_nm_get_parent(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_nm *nm = &clk->u.nm;
	u_int index;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_NM);

	val = CCU_READ(sc, nm->reg);
	index = __SHIFTOUT(val, nm->sel);

	return nm->parents[index];
}
