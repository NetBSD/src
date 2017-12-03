/* $NetBSD: sunxi_ccu_nkmp.c,v 1.7.2.2 2017/12/03 11:35:56 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_ccu_nkmp.c,v 1.7.2.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/sunxi/sunxi_ccu.h>

int
sunxi_ccu_nkmp_enable(struct sunxi_ccu_softc *sc, struct sunxi_ccu_clk *clk,
    int enable)
{
	struct sunxi_ccu_nkmp *nkmp = &clk->u.nkmp;
	uint32_t val;
	int retry;

	KASSERT(clk->type == SUNXI_CCU_NKMP);

	if (!nkmp->enable)
		return enable ? 0 : EINVAL;

	val = CCU_READ(sc, nkmp->reg);
	if (enable)
		val |= nkmp->enable;
	else
		val &= ~nkmp->enable;
	CCU_WRITE(sc, nkmp->reg, val);

	if (enable && nkmp->lock) {
		for (retry = 1000; retry > 0; retry--) {
			val = CCU_READ(sc, nkmp->reg);
			if (val & nkmp->lock)
				break;
			delay(100);
		}
		if (retry == 0)
			return ETIMEDOUT;
	}

	return 0;
}

u_int
sunxi_ccu_nkmp_get_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_nkmp *nkmp = &clk->u.nkmp;
	struct clk *clkp, *clkp_parent;
	u_int rate, n, k, m, p;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_NKMP);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	rate = clk_get_rate(clkp_parent);
	if (rate == 0)
		return 0;

	val = CCU_READ(sc, nkmp->reg);
	if (nkmp->n)
		n = __SHIFTOUT(val, nkmp->n);
	else
		n = 0;
	if (nkmp->k)
		k = __SHIFTOUT(val, nkmp->k);
	else
		k = 0;
	if (nkmp->m)
		m = __SHIFTOUT(val, nkmp->m);
	else
		m = 0;
	if (nkmp->p)
		p = __SHIFTOUT(val, nkmp->p);
	else
		p = 0;

	if (nkmp->enable && !(val & nkmp->enable))
		return 0;

	if ((nkmp->flags & SUNXI_CCU_NKMP_FACTOR_N_EXACT) == 0)
		n++;

	if ((nkmp->flags & SUNXI_CCU_NKMP_FACTOR_N_ZERO_IS_ONE) != 0 && n == 0)
		n++;

	k++;

	if ((nkmp->flags & SUNXI_CCU_NKMP_FACTOR_P_POW2) != 0)
		p = 1 << p;
	else
		p++;

	m++;
	if (nkmp->flags & SUNXI_CCU_NKMP_DIVIDE_BY_TWO)
		m *= 2;

	return (u_int)((uint64_t)rate * n * k) / (m * p);
}

int
sunxi_ccu_nkmp_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, u_int rate)
{
	struct sunxi_ccu_nkmp *nkmp = &clk->u.nkmp;
	const struct sunxi_ccu_nkmp_tbl *tab;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_NKMP);

	if (nkmp->table == NULL || rate == 0)
		return EIO;

	for (tab = nkmp->table; tab->rate > 0; tab++)
		if (tab->rate == rate)
			break;
	if (tab->rate == 0)
		return EINVAL;

	val = CCU_READ(sc, nkmp->reg);

	if (nkmp->flags & SUNXI_CCU_NKMP_SCALE_CLOCK) {
		if (nkmp->p && __SHIFTOUT(val, nkmp->p) < tab->p) {
			val &= ~nkmp->p;
			val |= __SHIFTIN(tab->p, nkmp->p);
			CCU_WRITE(sc, nkmp->reg, val);
			delay(2000);
		}
		if (nkmp->m && __SHIFTOUT(val, nkmp->m) < tab->m) {
			val &= ~nkmp->m;
			val |= __SHIFTIN(tab->m, nkmp->m);
			CCU_WRITE(sc, nkmp->reg, val);
			delay(2000);
		}
		if (nkmp->n) {
			val &= ~nkmp->n;
			val |= __SHIFTIN(tab->n, nkmp->n);
		}
		if (nkmp->k) {
			val &= ~nkmp->k;
			val |= __SHIFTIN(tab->k, nkmp->k);
		}
		CCU_WRITE(sc, nkmp->reg, val);
		delay(2000);
		if (nkmp->m && __SHIFTOUT(val, nkmp->m) > tab->m) {
			val &= ~nkmp->m;
			val |= __SHIFTIN(tab->m, nkmp->m);
			CCU_WRITE(sc, nkmp->reg, val);
			delay(2000);
		}
		if (nkmp->p && __SHIFTOUT(val, nkmp->p) > tab->p) {
			val &= ~nkmp->p;
			val |= __SHIFTIN(tab->p, nkmp->p);
			CCU_WRITE(sc, nkmp->reg, val);
			delay(2000);
		}
	} else {
		if (nkmp->n) {
			val &= ~nkmp->n;
			val |= __SHIFTIN(tab->n, nkmp->n);
		}
		if (nkmp->k) {
			val &= ~nkmp->k;
			val |= __SHIFTIN(tab->k, nkmp->k);
		}
		if (nkmp->m) {
			val &= ~nkmp->m;
			val |= __SHIFTIN(tab->m, nkmp->m);
		}
		if (nkmp->p) {
			val &= ~nkmp->p;
			val |= __SHIFTIN(tab->p, nkmp->p);
		}
		CCU_WRITE(sc, nkmp->reg, val);
	}

	return 0;
}

const char *
sunxi_ccu_nkmp_get_parent(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_nkmp *nkmp = &clk->u.nkmp;

	KASSERT(clk->type == SUNXI_CCU_NKMP);

	return nkmp->parent;
}
