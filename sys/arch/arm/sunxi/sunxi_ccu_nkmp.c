/* $NetBSD: sunxi_ccu_nkmp.c,v 1.2 2017/06/29 10:53:59 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_ccu_nkmp.c,v 1.2 2017/06/29 10:53:59 jmcneill Exp $");

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

	KASSERT(clk->type == SUNXI_CCU_NKMP);

	if (!nkmp->enable)
		return enable ? 0 : EINVAL;

	val = CCU_READ(sc, nkmp->reg);
	if (enable)
		val |= nkmp->enable;
	else
		val &= ~nkmp->enable;
	CCU_WRITE(sc, nkmp->reg, val);

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
	n = __SHIFTOUT(val, nkmp->n);
	k = __SHIFTOUT(val, nkmp->k);
	if (nkmp->m)
		m = __SHIFTOUT(val, nkmp->m);
	else
		m = 0;
	p = __SHIFTOUT(val, nkmp->p);

	if (nkmp->enable && !(val & nkmp->enable))
		return 0;

	n++;
	k++;
	m++;
	p++;

	if (nkmp->flags & SUNXI_CCU_NKMP_DIVIDE_BY_TWO)
		m *= 2;

	return (u_int)((uint64_t)rate * n * k) / (m * p);
}

int
sunxi_ccu_nkmp_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, u_int rate)
{
	return EIO;
}

const char *
sunxi_ccu_nkmp_get_parent(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_nkmp *nkmp = &clk->u.nkmp;

	KASSERT(clk->type == SUNXI_CCU_NKMP);

	return nkmp->parent;
}
