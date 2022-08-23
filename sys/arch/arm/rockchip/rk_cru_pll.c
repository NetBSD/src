/* $NetBSD: rk_cru_pll.c,v 1.6 2022/08/23 05:39:06 ryo Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: rk_cru_pll.c,v 1.6 2022/08/23 05:39:06 ryo Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/rockchip/rk_cru.h>

#define	PLL_CON0		0x00
#define	 PLL_BYPASS		__BIT(15)
#define	 PLL_POSTDIV1		__BITS(14,12)
#define	 PLL_FBDIV		__BITS(11,0)

#define	PLL_CON1		0x04
#define	 PLL_PDSEL		__BIT(15)
#define	 PLL_PD1		__BIT(14)
#define	 PLL_PD0		__BIT(13)
#define	 PLL_DSMPD		__BIT(12)
#define	 PLL_LOCK		__BIT(10)
#define	 PLL_POSTDIV2		__BITS(8,6)
#define	 PLL_REFDIV		__BITS(5,0)

#define	PLL_CON2		0x08
#define	 PLL_FOUT4PHASEPD	__BIT(27)
#define	 PLL_FOUTVCOPD		__BIT(26)
#define	 PLL_FOUTPOSTDIVPD	__BIT(25)
#define	 PLL_DACPD		__BIT(24)
#define	 PLL_FRACDIV		__BITS(23,0)

#define	PLL_CON3		0x0c
#define	PLL_CON6		0x18

#define	PLL_WRITE_MASK		0xffff0000	/* for CON0 and CON1 */

/* RK3288 CON0 */
#define	RK3288_CLKR		__BITS(13,8)
#define	RK3288_CLKOD		__BITS(3,0)
/* RK3288 CON1 */
#define	RK3288_LOCK		__BIT(31)
#define	RK3288_CLKF		__BITS(12,0)
/* RK3288 CON2 */
#define	RK3288_BWADJ		__BITS(11,0)
/* RK3288 CON3 */
#define	RK3288_BYPASS		__BIT(0)

#define RK3588_PLLCON0_M	__BITS(9,0)
#define RK3588_PLLCON1_P	__BITS(5,0)
#define RK3588_PLLCON1_S	__BITS(8,6)
#define RK3588_PLLCON2_K	__BITS(15,0)
#define RK3588_PLLCON1_PWRDOWN	__BIT(13)
#define RK3588_PLLCON6_LOCK	__BIT(15)

#define PLL_MODE_SLOW		0x0
#define PLL_MODE_NORM		0x1

u_int
rk_cru_pll_get_rate(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk)
{
	struct rk_cru_pll *pll = &clk->u.pll;
	struct clk *clkp, *clkp_parent;
	u_int foutvco, foutpostdiv;

	KASSERT(clk->type == RK_CRU_PLL);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	const u_int fref = clk_get_rate(clkp_parent);
	if (fref == 0)
		return 0;

	const uint32_t con0 = CRU_READ(sc, pll->con_base + PLL_CON0);
	const uint32_t con1 = CRU_READ(sc, pll->con_base + PLL_CON1);
	const uint32_t con2 = CRU_READ(sc, pll->con_base + PLL_CON2);
	const uint32_t con3 = CRU_READ(sc, pll->con_base + PLL_CON3);

	if ((pll->flags & RK_PLL_RK3288) != 0) {
		if ((con3 & RK3288_BYPASS) != 0) {
			return fref;
		}

		const u_int nr = __SHIFTOUT(con0, RK3288_CLKR) + 1;
		const u_int no = __SHIFTOUT(con0, RK3288_CLKOD) + 1;
		const u_int nf = __SHIFTOUT(con1, RK3288_CLKF) + 1;

		const uint64_t tmp = (uint64_t)fref * nf / nr / no;

		return (u_int)tmp;
	} else if ((pll->flags & RK_PLL_RK3588) != 0) {
		const uint64_t m = __SHIFTOUT(con0, RK3588_PLLCON0_M);
		const uint64_t p = __SHIFTOUT(con1, RK3588_PLLCON1_P);
		const uint64_t s = __SHIFTOUT(con1, RK3588_PLLCON1_S);
		const uint64_t k = __SHIFTOUT(con2, RK3588_PLLCON2_K);

		uint64_t tmp = (uint64_t)fref * m;
		if (p != 0)
			tmp /= p;
		if (k != 0 && p != 0)
			tmp += ((uint64_t)fref * k) / (p * 65535);
		tmp >>= s;
		return (u_int)tmp;
	} else {
		const u_int postdiv1 = __SHIFTOUT(con0, PLL_POSTDIV1);
		const u_int fbdiv = __SHIFTOUT(con0, PLL_FBDIV);
		const u_int dsmpd = __SHIFTOUT(con1, PLL_DSMPD);
		const u_int refdiv = __SHIFTOUT(con1, PLL_REFDIV);
		const u_int postdiv2 = __SHIFTOUT(con1, PLL_POSTDIV2);
		const u_int fracdiv = __SHIFTOUT(con2, PLL_FRACDIV);

		if (dsmpd == 1) {
			/* integer mode */
			foutvco = fref / refdiv * fbdiv;
		} else {
			/* fractional mode */
			foutvco = fref / refdiv * fbdiv + ((fref * fracdiv) >> 24);
		}
		foutpostdiv = foutvco / postdiv1 / postdiv2;

		return foutpostdiv;
	}
}

int
rk_cru_pll_set_rate(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk, u_int rate)
{
	struct rk_cru_pll *pll = &clk->u.pll;
	const struct rk_cru_pll_rate *pll_rate = NULL;
	uint32_t val;
	int retry;

	KASSERT(clk->type == RK_CRU_PLL);

	if (pll->rates == NULL || rate == 0 || !HAS_GRF(sc))
		return EIO;

	for (int i = 0; i < pll->nrates; i++)
		if (pll->rates[i].rate == rate) {
			pll_rate = &pll->rates[i];
			break;
		}
	if (pll_rate == NULL)
		return EINVAL;

	if ((pll->flags & RK_PLL_RK3288) != 0) {
		/* XXX TODO */
		KASSERT(false);
	} else if ((pll->flags & RK_PLL_RK3588) != 0) {
		bool muxed = false;

		/* into SLOW mode */
		if (__SHIFTOUT(CRU_READ(sc, pll->mode_reg), pll->mode_mask) ==
		    PLL_MODE_NORM) {
			CRU_WRITE(sc, pll->mode_reg,
			    pll->mode_mask << 16 |
			    __SHIFTIN(PLL_MODE_SLOW, pll->mode_mask));
			muxed = true;
		}

		/* power down */
		CRU_WRITE(sc, pll->con_base + PLL_CON1,
		    RK3588_PLLCON1_PWRDOWN << 16 |
		    __SHIFTIN(1, RK3588_PLLCON1_PWRDOWN));

		/* update m,p,s,k */
		CRU_WRITE(sc, pll->con_base + PLL_CON0,
		    RK3588_PLLCON0_M << 16 |
		    __SHIFTIN(pll_rate->m, RK3588_PLLCON0_M));
		CRU_WRITE(sc, pll->con_base + PLL_CON1,
		    RK3588_PLLCON1_P << 16 |
		    RK3588_PLLCON1_S << 16 |
		    __SHIFTIN(pll_rate->p, RK3588_PLLCON1_P) |
		    __SHIFTIN(pll_rate->s, RK3588_PLLCON1_S));
		CRU_WRITE(sc, pll->con_base + PLL_CON2,
		    RK3588_PLLCON2_K << 16 |
		    __SHIFTIN(pll_rate->k, RK3588_PLLCON2_K));

		/* power up */
		CRU_WRITE(sc, pll->con_base + PLL_CON1,
		    RK3588_PLLCON1_PWRDOWN << 16 |
		    __SHIFTIN(0, RK3588_PLLCON1_PWRDOWN));

		/* wait */
		for (retry = 1000; retry > 0; retry--) {
			if (CRU_READ(sc, pll->con_base + PLL_CON6) &
			    pll->lock_mask) {
				break;
			}
			delay(1);
		}
		if (retry == 0)
			device_printf(sc->sc_dev,
			    "WARNING: %s failed to lock\n", clk->base.name);

		/* into NORM mode */
		if (muxed) {
			CRU_WRITE(sc, pll->mode_reg,
			    pll->mode_mask << 16 |
			    __SHIFTIN(PLL_MODE_NORM, pll->mode_mask));
		}
	} else {
		CRU_WRITE(sc, pll->con_base + PLL_CON0,
		    __SHIFTIN(pll_rate->postdiv1, PLL_POSTDIV1) |
		    __SHIFTIN(pll_rate->fbdiv, PLL_FBDIV) |
		    PLL_WRITE_MASK);

		CRU_WRITE(sc, pll->con_base + PLL_CON1,
		    __SHIFTIN(pll_rate->dsmpd, PLL_DSMPD) |
		    __SHIFTIN(pll_rate->postdiv2, PLL_POSTDIV2) |
		    __SHIFTIN(pll_rate->refdiv, PLL_REFDIV) |
		    PLL_WRITE_MASK);

		val = CRU_READ(sc, pll->con_base + PLL_CON2);
		val &= ~PLL_FRACDIV;
		val |= __SHIFTIN(pll_rate->fracdiv, PLL_FRACDIV);
		CRU_WRITE(sc, pll->con_base + PLL_CON2, val);

		/* Set PLL work mode to normal */
		const uint32_t write_mask = pll->mode_mask << 16;
		const uint32_t write_val = pll->mode_mask;
		CRU_WRITE(sc, pll->mode_reg, write_mask | write_val);

		syscon_lock(sc->sc_grf);
		for (retry = 1000; retry > 0; retry--) {
			if (syscon_read_4(sc->sc_grf,
			    sc->sc_grf_soc_status) & pll->lock_mask)
				break;
			delay(1);
		}
		syscon_unlock(sc->sc_grf);

		if (retry == 0)
			device_printf(sc->sc_dev,
			    "WARNING: %s failed to lock\n", clk->base.name);
	}

	return 0;
}

const char *
rk_cru_pll_get_parent(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk)
{
	struct rk_cru_pll *pll = &clk->u.pll;

	KASSERT(clk->type == RK_CRU_PLL);

	return pll->parents[0];
}
