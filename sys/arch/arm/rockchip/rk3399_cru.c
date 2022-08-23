/* $NetBSD: rk3399_cru.c,v 1.25 2022/08/23 05:33:39 ryo Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: rk3399_cru.c,v 1.25 2022/08/23 05:33:39 ryo Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/rockchip/rk_cru.h>
#include <arm/rockchip/rk3399_cru.h>

#define	PLL_CON(n)	(0x0000 + (n) * 4)
#define	CLKSEL_CON(n)	(0x0100 + (n) * 4)
#define	CLKGATE_CON(n)	(0x0300 + (n) * 4)
#define	SOFTRST_CON(n)	(0x0400 + (n) * 4)

static int rk3399_cru_match(device_t, cfdata_t, void *);
static void rk3399_cru_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3399-cru" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(rk3399_cru, sizeof(struct rk_cru_softc),
	rk3399_cru_match, rk3399_cru_attach, NULL, NULL);

static const struct rk_cru_pll_rate pll_rates[] = {
	RK_PLL_RATE(2208000000,  1,  92, 1, 1, 1, 0),
	RK_PLL_RATE(2184000000,  1,  91, 1, 1, 1, 0),
	RK_PLL_RATE(2160000000,  1,  90, 1, 1, 1, 0),
	RK_PLL_RATE(2136000000,  1,  89, 1, 1, 1, 0),
	RK_PLL_RATE(2112000000,  1,  88, 1, 1, 1, 0),
	RK_PLL_RATE(2088000000,  1,  87, 1, 1, 1, 0),
	RK_PLL_RATE(2064000000,  1,  86, 1, 1, 1, 0),
	RK_PLL_RATE(2040000000,  1,  85, 1, 1, 1, 0),
	RK_PLL_RATE(2016000000,  1,  84, 1, 1, 1, 0),
	RK_PLL_RATE(1992000000,  1,  83, 1, 1, 1, 0),
	RK_PLL_RATE(1968000000,  1,  82, 1, 1, 1, 0),
	RK_PLL_RATE(1944000000,  1,  81, 1, 1, 1, 0),
	RK_PLL_RATE(1920000000,  1,  80, 1, 1, 1, 0),
	RK_PLL_RATE(1896000000,  1,  79, 1, 1, 1, 0),
	RK_PLL_RATE(1872000000,  1,  78, 1, 1, 1, 0),
	RK_PLL_RATE(1848000000,  1,  77, 1, 1, 1, 0),
	RK_PLL_RATE(1824000000,  1,  76, 1, 1, 1, 0),
	RK_PLL_RATE(1800000000,  1,  75, 1, 1, 1, 0),
	RK_PLL_RATE(1776000000,  1,  74, 1, 1, 1, 0),
	RK_PLL_RATE(1752000000,  1,  73, 1, 1, 1, 0),
	RK_PLL_RATE(1728000000,  1,  72, 1, 1, 1, 0),
	RK_PLL_RATE(1704000000,  1,  71, 1, 1, 1, 0),
	RK_PLL_RATE(1680000000,  1,  70, 1, 1, 1, 0),
	RK_PLL_RATE(1656000000,  1,  69, 1, 1, 1, 0),
	RK_PLL_RATE(1632000000,  1,  68, 1, 1, 1, 0),
	RK_PLL_RATE(1608000000,  1,  67, 1, 1, 1, 0),
	RK_PLL_RATE(1600000000,  3, 200, 1, 1, 1, 0),
	RK_PLL_RATE(1584000000,  1,  66, 1, 1, 1, 0),
	RK_PLL_RATE(1560000000,  1,  65, 1, 1, 1, 0),
	RK_PLL_RATE(1536000000,  1,  64, 1, 1, 1, 0),
	RK_PLL_RATE(1512000000,  1,  63, 1, 1, 1, 0),
	RK_PLL_RATE(1488000000,  1,  62, 1, 1, 1, 0),
	RK_PLL_RATE(1464000000,  1,  61, 1, 1, 1, 0),
	RK_PLL_RATE(1440000000,  1,  60, 1, 1, 1, 0),
	RK_PLL_RATE(1416000000,  1,  59, 1, 1, 1, 0),
	RK_PLL_RATE(1392000000,  1,  58, 1, 1, 1, 0),
	RK_PLL_RATE(1368000000,  1,  57, 1, 1, 1, 0),
	RK_PLL_RATE(1344000000,  1,  56, 1, 1, 1, 0),
	RK_PLL_RATE(1320000000,  1,  55, 1, 1, 1, 0),
	RK_PLL_RATE(1296000000,  1,  54, 1, 1, 1, 0),
	RK_PLL_RATE(1272000000,  1,  53, 1, 1, 1, 0),
	RK_PLL_RATE(1248000000,  1,  52, 1, 1, 1, 0),
	RK_PLL_RATE(1200000000,  1,  50, 1, 1, 1, 0),
	RK_PLL_RATE(1188000000,  2,  99, 1, 1, 1, 0),
	RK_PLL_RATE(1104000000,  1,  46, 1, 1, 1, 0),
	RK_PLL_RATE(1100000000, 12, 550, 1, 1, 1, 0),
	RK_PLL_RATE(1008000000,  1,  84, 2, 1, 1, 0),
	RK_PLL_RATE(1000000000,  1, 125, 3, 1, 1, 0),
	RK_PLL_RATE( 984000000,  1,  82, 2, 1, 1, 0),
	RK_PLL_RATE( 960000000,  1,  80, 2, 1, 1, 0),
	RK_PLL_RATE( 936000000,  1,  78, 2, 1, 1, 0),
	RK_PLL_RATE( 912000000,  1,  76, 2, 1, 1, 0),
	RK_PLL_RATE( 900000000,  4, 300, 2, 1, 1, 0),
	RK_PLL_RATE( 888000000,  1,  74, 2, 1, 1, 0),
	RK_PLL_RATE( 864000000,  1,  72, 2, 1, 1, 0),
	RK_PLL_RATE( 840000000,  1,  70, 2, 1, 1, 0),
	RK_PLL_RATE( 816000000,  1,  68, 2, 1, 1, 0),
	RK_PLL_RATE( 800000000,  1, 100, 3, 1, 1, 0),
	RK_PLL_RATE( 700000000,  6, 350, 2, 1, 1, 0),
	RK_PLL_RATE( 696000000,  1,  58, 2, 1, 1, 0),
	RK_PLL_RATE( 676000000,  3, 169, 2, 1, 1, 0),
	RK_PLL_RATE( 600000000,  1,  75, 3, 1, 1, 0),
	RK_PLL_RATE( 594000000,  1,  99, 4, 1, 1, 0),
	RK_PLL_RATE( 533250000,  8, 711, 4, 1, 1, 0),
	RK_PLL_RATE( 504000000,  1,  63, 3, 1, 1, 0),
	RK_PLL_RATE( 500000000,  6, 250, 2, 1, 1, 0),
	RK_PLL_RATE( 408000000,  1,  68, 2, 2, 1, 0),
	RK_PLL_RATE( 312000000,  1,  52, 2, 2, 1, 0),
	RK_PLL_RATE( 297000000,  1,  99, 4, 2, 1, 0),
	RK_PLL_RATE( 216000000,  1,  72, 4, 2, 1, 0),
	RK_PLL_RATE( 148500000,  1,  99, 4, 4, 1, 0),
	RK_PLL_RATE( 106500000,  1,  71, 4, 4, 1, 0),
	RK_PLL_RATE(  96000000,  1,  64, 4, 4, 1, 0),
	RK_PLL_RATE(  74250000,  2,  99, 4, 4, 1, 0),
	RK_PLL_RATE(  65000000,  1,  65, 6, 4, 1, 0),
	RK_PLL_RATE(  54000000,  1,  54, 6, 4, 1, 0),
	RK_PLL_RATE(  27000000,  1,  27, 6, 4, 1, 0),
};

static const struct rk_cru_pll_rate pll_norates[] = {
};

#define	RK3399_ACLKM_MASK	__BITS(12,8)
#define	RK3399_ATCLK_MASK	__BITS(4,0)
#define	RK3399_PDBG_MASK	__BITS(12,8)

#define RK3399_CPU_RATE(_rate, _reg0, _reg0_mask, _reg0_val, _reg1, _reg1_mask, _reg1_val)\
	{										\
		.rate = (_rate),							\
		.divs[0] = { .reg = (_reg0), .mask = (_reg0_mask), .val = (_reg0_val) },\
		.divs[1] = { .reg = (_reg1), .mask = (_reg1_mask), .val = (_reg1_val) },\
	}

#define	RK3399_CPUL_RATE(_rate, _aclkm, _atclk, _pdbg)			\
	RK3399_CPU_RATE(_rate,						\
		    CLKSEL_CON(0), RK3399_ACLKM_MASK,			\
		    __SHIFTIN((_aclkm), RK3399_ACLKM_MASK),		\
		    CLKSEL_CON(1), RK3399_ATCLK_MASK|RK3399_PDBG_MASK,	\
		    __SHIFTIN((_atclk), RK3399_ATCLK_MASK)|__SHIFTIN((_pdbg), RK3399_PDBG_MASK))

#define	RK3399_CPUB_RATE(_rate, _aclkm, _atclk, _pdbg)			\
	RK3399_CPU_RATE(_rate,						\
		    CLKSEL_CON(2), RK3399_ACLKM_MASK,			\
		    __SHIFTIN((_aclkm), RK3399_ACLKM_MASK),		\
		    CLKSEL_CON(3), RK3399_ATCLK_MASK|RK3399_PDBG_MASK,	\
		    __SHIFTIN((_atclk), RK3399_ATCLK_MASK)|__SHIFTIN((_pdbg), RK3399_PDBG_MASK))

static const struct rk_cru_cpu_rate armclkl_rates[] = {
        RK3399_CPUL_RATE(1800000000, 1, 8, 8),
        RK3399_CPUL_RATE(1704000000, 1, 8, 8),
        RK3399_CPUL_RATE(1608000000, 1, 7, 7),
        RK3399_CPUL_RATE(1512000000, 1, 7, 7),
        RK3399_CPUL_RATE(1488000000, 1, 6, 6),
        RK3399_CPUL_RATE(1416000000, 1, 6, 6),
        RK3399_CPUL_RATE(1200000000, 1, 5, 5),
        RK3399_CPUL_RATE(1008000000, 1, 5, 5),
        RK3399_CPUL_RATE( 816000000, 1, 4, 4),
        RK3399_CPUL_RATE( 696000000, 1, 3, 3),
        RK3399_CPUL_RATE( 600000000, 1, 3, 3),
        RK3399_CPUL_RATE( 408000000, 1, 2, 2),
        RK3399_CPUL_RATE( 312000000, 1, 1, 1),
        RK3399_CPUL_RATE( 216000000, 1, 1, 1),
        RK3399_CPUL_RATE(  96000000, 1, 1, 1),
};

static const struct rk_cru_cpu_rate armclkb_rates[] = {
        RK3399_CPUB_RATE(2208000000, 1, 11, 11),
        RK3399_CPUB_RATE(2184000000, 1, 11, 11),
        RK3399_CPUB_RATE(2088000000, 1, 10, 10),
        RK3399_CPUB_RATE(2040000000, 1, 10, 10),
        RK3399_CPUB_RATE(2016000000, 1, 9, 9),
        RK3399_CPUB_RATE(2000000000, 1, 9, 9),
        RK3399_CPUB_RATE(1992000000, 1, 9, 9),
        RK3399_CPUB_RATE(1896000000, 1, 9, 9),
        RK3399_CPUB_RATE(1800000000, 1, 8, 8),
        RK3399_CPUB_RATE(1704000000, 1, 8, 8),
        RK3399_CPUB_RATE(1608000000, 1, 7, 7),
        RK3399_CPUB_RATE(1512000000, 1, 7, 7),
        RK3399_CPUB_RATE(1488000000, 1, 6, 6),
        RK3399_CPUB_RATE(1416000000, 1, 6, 6),
        RK3399_CPUB_RATE(1200000000, 1, 5, 5),
        RK3399_CPUB_RATE(1008000000, 1, 5, 5),
        RK3399_CPUB_RATE( 816000000, 1, 4, 4),
        RK3399_CPUB_RATE( 696000000, 1, 3, 3),
        RK3399_CPUB_RATE( 600000000, 1, 3, 3),
        RK3399_CPUB_RATE( 408000000, 1, 2, 2),
        RK3399_CPUB_RATE( 312000000, 1, 1, 1),
        RK3399_CPUB_RATE( 216000000, 1, 1, 1),
        RK3399_CPUB_RATE(  96000000, 1, 1, 1),
};

#define	PLL_CON0	0x00
#define	 PLL_FBDIV	__BITS(11,0)

#define	PLL_CON1	0x04
#define	 PLL_POSTDIV2	__BITS(14,12)
#define	 PLL_POSTDIV1	__BITS(10,8)
#define	 PLL_REFDIV	__BITS(5,0)

#define	PLL_CON2	0x08
#define	 PLL_LOCK	__BIT(31)
#define	 PLL_FRACDIV	__BITS(23,0)

#define	PLL_CON3	0x0c
#define	 PLL_WORK_MODE	__BITS(9,8)
#define	  PLL_WORK_MODE_SLOW		0
#define	  PLL_WORK_MODE_NORMAL		1
#define	  PLL_WORK_MODE_DEEP_SLOW	2
#define	 PLL_DSMPD	__BIT(3)

#define	PLL_WRITE_MASK	0xffff0000

static u_int
rk3399_cru_pll_get_rate(struct rk_cru_softc *sc,
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

	const u_int fbdiv = __SHIFTOUT(con0, PLL_FBDIV);
	const u_int postdiv2 = __SHIFTOUT(con1, PLL_POSTDIV2);
	const u_int postdiv1 = __SHIFTOUT(con1, PLL_POSTDIV1);
	const u_int refdiv = __SHIFTOUT(con1, PLL_REFDIV);
	const u_int fracdiv = __SHIFTOUT(con2, PLL_FRACDIV);
	const u_int dsmpd = __SHIFTOUT(con3, PLL_DSMPD);

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

static int
rk3399_cru_pll_set_rate(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk, u_int rate)
{
	struct rk_cru_pll *pll = &clk->u.pll;
	const struct rk_cru_pll_rate *pll_rate = NULL;
	uint32_t val;
	int retry, best_diff;

	KASSERT(clk->type == RK_CRU_PLL);

	if (pll->rates == NULL || rate == 0)
		return EIO;

	best_diff = INT_MAX;
	for (int i = 0; i < pll->nrates; i++) {
		int diff;

		if (rate > pll->rates[i].rate)
			diff = rate - pll->rates[i].rate;
		else
			diff = pll->rates[i].rate - rate;
		if (diff < best_diff) {
			pll_rate = &pll->rates[i];
			best_diff = diff;
		}
	}

	val = __SHIFTIN(PLL_WORK_MODE_SLOW, PLL_WORK_MODE) | (PLL_WORK_MODE << 16);
	CRU_WRITE(sc, pll->con_base + PLL_CON3, val);

	CRU_WRITE(sc, pll->con_base + PLL_CON0,
	    __SHIFTIN(pll_rate->fbdiv, PLL_FBDIV) | (PLL_FBDIV << 16));

	CRU_WRITE(sc, pll->con_base + PLL_CON1,
	    __SHIFTIN(pll_rate->postdiv2, PLL_POSTDIV2) |
	    __SHIFTIN(pll_rate->postdiv1, PLL_POSTDIV1) |
	    __SHIFTIN(pll_rate->refdiv, PLL_REFDIV) |
	    ((PLL_POSTDIV2 | PLL_POSTDIV1 | PLL_REFDIV) << 16));

	val = CRU_READ(sc, pll->con_base + PLL_CON2);
	val &= ~PLL_FRACDIV;
	val |= __SHIFTIN(pll_rate->fracdiv, PLL_FRACDIV);
	CRU_WRITE(sc, pll->con_base + PLL_CON2, val);

	val = __SHIFTIN(pll_rate->dsmpd, PLL_DSMPD) | (PLL_DSMPD << 16);
	CRU_WRITE(sc, pll->con_base + PLL_CON3, val);

	for (retry = 1000; retry > 0; retry--) {
		if (CRU_READ(sc, pll->con_base + PLL_CON2) & pll->lock_mask)
			break;
		delay(1);
	}

	if (retry == 0)
		device_printf(sc->sc_dev, "WARNING: %s failed to lock\n",
		    clk->base.name);

	/* Set PLL work mode to normal */
	val = __SHIFTIN(PLL_WORK_MODE_NORMAL, PLL_WORK_MODE) | (PLL_WORK_MODE << 16);
	CRU_WRITE(sc, pll->con_base + PLL_CON3, val);

	return 0;
}

#define RK3399_PLL(_id, _name, _parents, _con_base, _mode_reg, _mode_mask, _lock_mask, _rates) \
        {                                                       \
                .id = (_id),                                    \
                .type = RK_CRU_PLL,                             \
                .base.name = (_name),                           \
                .base.flags = 0,                                \
                .u.pll.parents = (_parents),                    \
                .u.pll.nparents = __arraycount(_parents),       \
                .u.pll.con_base = (_con_base),                  \
                .u.pll.mode_reg = (_mode_reg),                  \
                .u.pll.mode_mask = (_mode_mask),                \
                .u.pll.lock_mask = (_lock_mask),                \
                .u.pll.rates = (_rates),                        \
                .u.pll.nrates = __arraycount(_rates),           \
                .get_rate = rk3399_cru_pll_get_rate,            \
                .set_rate = rk3399_cru_pll_set_rate,            \
                .get_parent = rk_cru_pll_get_parent,            \
        }

static const char * pll_parents[] = { "xin24m", "xin32k" };
static const char * armclkl_parents[] = { "clk_core_l_lpll_src", "clk_core_l_bpll_src", "clk_core_l_dpll_src", "clk_core_l_gpll_src" };
static const char * armclkb_parents[] = { "clk_core_b_lpll_src", "clk_core_b_bpll_src", "clk_core_b_dpll_src", "clk_core_b_gpll_src" };
static const char * mux_clk_tsadc_parents[] = { "xin24m", "xin32k" };
static const char * mux_pll_src_cpll_gpll_parents[] = { "cpll", "gpll" };
static const char * mux_pll_src_cpll_gpll_npll_parents[] = { "cpll", "gpll", "npll" };
static const char * mux_pll_src_cpll_gpll_ppll_parents[] = { "cpll", "gpll", "ppll" };
static const char * mux_pll_src_cpll_gpll_upll_parents[] = { "cpll", "gpll", "upll" };
static const char * mux_pll_src_cpll_gpll_npll_24m_parents[] = { "cpll", "gpll", "npll", "xin24m" };
static const char * mux_pll_src_cpll_gpll_npll_ppll_upll_24m_parents[] = { "cpll", "gpll", "npll", "ppll", "upll", "xin24m" };
static const char * mux_pll_src_npll_cpll_gpll_parents[] = { "npll", "cpll", "gpll" };
static const char * mux_pll_src_vpll_cpll_gpll_parents[] = { "vpll", "cpll", "gpll" };
static const char * mux_pll_src_vpll_cpll_gpll_npll_parents[] = { "vpll", "cpll", "gpll", "npll" };
static const char * mux_aclk_perilp0_parents[] = { "cpll_aclk_perilp0_src", "gpll_aclk_perilp0_src" };
static const char * mux_hclk_perilp1_parents[] = { "cpll_hclk_perilp1_src", "gpll_hclk_perilp1_src" };
static const char * mux_aclk_perihp_parents[] = { "cpll_aclk_perihp_src", "gpll_aclk_perihp_src" };
static const char * mux_aclk_cci_parents[] = { "cpll_aclk_cci_src", "gpll_aclk_cci_src", "npll_aclk_cci_src", "vpll_aclk_cci_src" };
static const char * mux_dclk_vop0_parents[] = { "dclk_vop0_div", "dclk_vop0_frac" };
static const char * mux_dclk_vop1_parents[] = { "dclk_vop1_div", "dclk_vop1_frac" };
static const char * mux_i2s0_parents[] = { "clk_i2s0_div", "clk_i2s0_frac", "clkin_i2s", "xin12m" };
static const char * mux_i2s1_parents[] = { "clk_i2s1_div", "clk_i2s1_frac", "clkin_i2s", "xin12m" };
static const char * mux_i2s2_parents[] = { "clk_i2s2_div", "clk_i2s2_frac", "clkin_i2s", "xin12m" };
static const char * mux_i2sch_parents[] = { "clk_i2s0", "clk_i2s1", "clk_i2s2" };
static const char * mux_i2sout_parents[] = { "clk_i2sout_src", "xin12m" };
static const char * mux_uart0_parents[] = { "clk_uart0_div", "clk_uart0_frac", "xin24m" };
static const char * mux_uart1_parents[] = { "clk_uart1_div", "clk_uart1_frac", "xin24m" };
static const char * mux_uart2_parents[] = { "clk_uart2_div", "clk_uart2_frac", "xin24m" };
static const char * mux_uart3_parents[] = { "clk_uart3_div", "clk_uart3_frac", "xin24m" };
static const char * mux_rmii_parents[] = { "clk_gmac", "clkin_gmac" };
static const char * mux_aclk_gmac_parents[] = { "cpll_aclk_gmac_src", "gpll_aclk_gmac_src" };
static const char * mux_aclk_emmc_parents[] = { "cpll_aclk_emmc_src", "gpll_aclk_emmc_src" };
static const char * mux_pll_src_24m_pciephy_parents[] = { "xin24m", "clk_pciephy_ref100m" };
static const char * mux_pciecore_cru_phy_parents[] = { "clk_pcie_core_cru", "clk_pcie_core_phy" };

static struct rk_cru_clk rk3399_cru_clks[] = {
	RK3399_PLL(RK3399_PLL_APLLL, "lpll", pll_parents,
		   PLL_CON(0),		/* con_base */
		   PLL_CON(3),		/* mode_reg */
		   __BIT(8),		/* mode_mask */
		   __BIT(31),		/* lock_mask */
		   pll_rates),
	RK3399_PLL(RK3399_PLL_APLLB, "bpll", pll_parents,
		   PLL_CON(8),		/* con_base */
		   PLL_CON(11),		/* mode_reg */
		   __BIT(8),		/* mode_mask */
		   __BIT(31),		/* lock_mask */
		   pll_rates),
	RK3399_PLL(RK3399_PLL_DPLL, "dpll", pll_parents,
		   PLL_CON(16),		/* con_base */
		   PLL_CON(19),		/* mode_reg */
		   __BIT(8),		/* mode_mask */
		   __BIT(31),		/* lock_mask */
		   pll_norates),
	RK3399_PLL(RK3399_PLL_CPLL, "cpll", pll_parents,
		   PLL_CON(24),		/* con_base */
		   PLL_CON(27),		/* mode_reg */
		   __BIT(8),		/* mode_mask */
		   __BIT(31),		/* lock_mask */
		   pll_rates),
	RK3399_PLL(RK3399_PLL_GPLL, "gpll", pll_parents,
		   PLL_CON(32),		/* con_base */
		   PLL_CON(35),		/* mode_reg */
		   __BIT(8),		/* mode_mask */
		   __BIT(31),		/* lock_mask */
		   pll_rates),
	RK3399_PLL(RK3399_PLL_NPLL, "npll", pll_parents,
		   PLL_CON(40),		/* con_base */
		   PLL_CON(43),		/* mode_reg */
		   __BIT(8),		/* mode_mask */
		   __BIT(31),		/* lock_mask */
		   pll_rates),
	RK3399_PLL(RK3399_PLL_VPLL, "vpll", pll_parents,
		   PLL_CON(48),		/* con_base */
		   PLL_CON(51),		/* mode_reg */
		   __BIT(8),		/* mode_mask */
		   __BIT(31),		/* lock_mask */
		   pll_rates),

	RK_GATE(0, "clk_core_l_lpll_src", "lpll", CLKGATE_CON(0), 0),
	RK_GATE(0, "clk_core_l_bpll_src", "bpll", CLKGATE_CON(0), 1),
	RK_GATE(0, "clk_core_l_dpll_src", "dpll", CLKGATE_CON(0), 2),
	RK_GATE(0, "clk_core_l_gpll_src", "gpll", CLKGATE_CON(0), 3),

	RK_CPU(RK3399_ARMCLKL, "armclkl", armclkl_parents,
	       CLKSEL_CON(0),		/* mux_reg */
	       __BITS(7,6), 0, 3,	/* mux_mask, mux_main, mux_alt */
	       CLKSEL_CON(0),		/* div_reg */
	       __BITS(4,0),		/* div_mask */
	       armclkl_rates),

	RK_GATE(0, "clk_core_b_lpll_src", "lpll", CLKGATE_CON(1), 0),
	RK_GATE(0, "clk_core_b_bpll_src", "bpll", CLKGATE_CON(1), 1),
	RK_GATE(0, "clk_core_b_dpll_src", "dpll", CLKGATE_CON(1), 2),
	RK_GATE(0, "clk_core_b_gpll_src", "gpll", CLKGATE_CON(1), 3),

	RK_CPU(RK3399_ARMCLKB, "armclkb", armclkb_parents,
	       CLKSEL_CON(2),		/* mux_reg */
	       __BITS(7,6), 1, 3,	/* mux_mask, mux_main, mux_alt */
	       CLKSEL_CON(2),		/* div_reg */
	       __BITS(4,0),		/* div_mask */
	       armclkb_rates),

	/*
	 * perilp0
	 */
	RK_GATE(0, "gpll_aclk_perilp0_src", "gpll", CLKGATE_CON(7), 0),
	RK_GATE(0, "cpll_aclk_perilp0_src", "cpll", CLKGATE_CON(7), 1),
	RK_COMPOSITE(RK3399_ACLK_PERILP0, "aclk_perilp0", mux_aclk_perilp0_parents,
		     CLKSEL_CON(23),	/* muxdiv_reg */
		     __BIT(7),		/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     CLKGATE_CON(7),	/* gate_reg */
		     __BIT(2),		/* gate_mask */
		     0),
	RK_COMPOSITE_NOMUX(RK3399_HCLK_PERILP0, "hclk_perilp0", "aclk_perilp0",
			   CLKSEL_CON(23),	/* div_reg */
			   __BITS(10,8),	/* div_mask */
			   CLKGATE_CON(7),	/* gate_reg */
			   __BIT(3),		/* gate_mask */
			   0),
	RK_COMPOSITE_NOMUX(RK3399_PCLK_PERILP0, "pclk_perilp0", "aclk_perilp0",
			   CLKSEL_CON(23),	/* div_reg */
			   __BITS(14,12),	/* div_mask */
			   CLKGATE_CON(7),	/* gate_reg */
			   __BIT(4),		/* gate_mask */
			   0),

	/*
	 * perilp1
	 */
	RK_GATE(0, "gpll_hclk_perilp1_src", "gpll", CLKGATE_CON(8), 0),
	RK_GATE(0, "cpll_hclk_perilp1_src", "cpll", CLKGATE_CON(8), 1),
	RK_COMPOSITE_NOGATE(RK3399_HCLK_PERILP1, "hclk_perilp1", mux_hclk_perilp1_parents,
			    CLKSEL_CON(25),	/* muxdiv_reg */
			    __BITS(10,8),	/* mux_mask */
			    __BITS(4,0),	/* div_mask */
			    0),
	RK_COMPOSITE_NOMUX(RK3399_PCLK_PERILP1, "pclk_perilp1", "hclk_perilp1",
			   CLKSEL_CON(25),	/* div_reg */
			   __BITS(10,8),	/* div_mask */
			   CLKGATE_CON(8),	/* gate_reg */
			   __BIT(2),		/* gate_mask */
			   0),

	/*
	 * perihp
	 */
	RK_GATE(0, "gpll_aclk_perihp_src", "gpll", CLKGATE_CON(5), 0),
	RK_GATE(0, "cpll_aclk_perihp_src", "cpll", CLKGATE_CON(5), 1),
	RK_COMPOSITE(RK3399_ACLK_PERIHP, "aclk_perihp", mux_aclk_perihp_parents,
		     CLKSEL_CON(14),	/* muxdiv_reg */
		     __BIT(7),		/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     CLKGATE_CON(5),	/* gate_reg */
		     __BIT(2),		/* gate_mask */
		     0),
	RK_COMPOSITE_NOMUX(RK3399_HCLK_PERIHP, "hclk_perihp", "aclk_perihp",
			   CLKSEL_CON(14),	/* div_reg */
			   __BITS(10,8),	/* div_mask */
			   CLKGATE_CON(5),	/* gate_reg */
			   __BIT(3),		/* gate_mask */
			   0),
	RK_COMPOSITE_NOMUX(RK3399_PCLK_PERIHP, "pclk_perihp", "aclk_perihp",
			   CLKSEL_CON(14),	/* div_reg */
			   __BITS(14,12),	/* div_mask */
			   CLKGATE_CON(5),	/* gate_reg */
			   __BIT(4),		/* gate_mask */
			   0),

	/*
	 * CCI
	 */
	RK_GATE(0, "cpll_aclk_cci_src", "cpll", CLKGATE_CON(2), 0),
	RK_GATE(0, "gpll_aclk_cci_src", "gpll", CLKGATE_CON(2), 1),
	RK_GATE(0, "npll_aclk_cci_src", "npll", CLKGATE_CON(2), 2),
	RK_GATE(0, "vpll_aclk_cci_src", "vpll", CLKGATE_CON(2), 3),
	RK_COMPOSITE(0, "aclk_cci_pre", mux_aclk_cci_parents,
		     CLKSEL_CON(5),	/* muxdiv_reg */
		     __BITS(7,6),	/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     CLKGATE_CON(2),	/* gate_reg */
		     __BIT(4),		/* gate_mask */
		     0),
	RK_GATE(RK3399_ACLK_CCI, "aclk_cci", "aclk_cci_pre", CLKGATE_CON(15), 2),

	/*
	 * GIC
	 */
	RK_COMPOSITE(RK3399_ACLK_GIC_PRE, "aclk_gic_pre", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(56),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(12,8),	/* div_mask */
		     CLKGATE_CON(12),	/* gate_reg */
		     __BIT(12),		/* gate_mask */
		     0),

	/*
	 * DDR
	 */
	RK_COMPOSITE(RK3399_PCLK_DDR, "pclk_ddr", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(6),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(12,8),	/* div_mask */
		     CLKGATE_CON(3),	/* gate_reg */
		     __BIT(4),		/* gate_mask */
		     0),

	/*
	 * alive
	 */
	RK_DIV(RK3399_PCLK_ALIVE, "pclk_alive", "gpll", CLKSEL_CON(57), __BITS(4,0), 0),

	/*
	 * GPIO
	 */
	RK_GATE(RK3399_PCLK_GPIO2, "pclk_gpio2", "pclk_alive", CLKGATE_CON(31), 3),
	RK_GATE(RK3399_PCLK_GPIO3, "pclk_gpio3", "pclk_alive", CLKGATE_CON(31), 4),
	RK_GATE(RK3399_PCLK_GPIO4, "pclk_gpio4", "pclk_alive", CLKGATE_CON(31), 5),

	/*
	 * UART
	 */
	RK_MUX(0, "clk_uart0_src", mux_pll_src_cpll_gpll_upll_parents, CLKSEL_CON(33), __BITS(13,12)),
	RK_MUX(0, "clk_uart_src", mux_pll_src_cpll_gpll_parents, CLKSEL_CON(33), __BIT(15)),
	RK_COMPOSITE_NOMUX(0, "clk_uart0_div", "clk_uart0_src",
			   CLKSEL_CON(33),	/* div_reg */
			   __BITS(6,0),		/* div_mask */
			   CLKGATE_CON(9),	/* gate_reg */
			   __BIT(0),		/* gate_mask */
			   0),
	RK_COMPOSITE_NOMUX(0, "clk_uart1_div", "clk_uart_src",
			   CLKSEL_CON(34),	/* div_reg */
			   __BITS(6,0),		/* div_mask */
			   CLKGATE_CON(9),	/* gate_reg */
			   __BIT(2),		/* gate_mask */
			   0),
	RK_COMPOSITE_NOMUX(0, "clk_uart2_div", "clk_uart_src",
			   CLKSEL_CON(35),	/* div_reg */
			   __BITS(6,0),		/* div_mask */
			   CLKGATE_CON(9),	/* gate_reg */
			   __BIT(4),		/* gate_mask */
			   0),
	RK_COMPOSITE_NOMUX(0, "clk_uart3_div", "clk_uart_src",
			   CLKSEL_CON(36),	/* div_reg */
			   __BITS(6,0),		/* div_mask */
			   CLKGATE_CON(9),	/* gate_reg */
			   __BIT(6),		/* gate_mask */
			   0),
	RK_MUX(RK3399_SCLK_UART0, "clk_uart0", mux_uart0_parents, CLKSEL_CON(33), __BITS(9,8)),
	RK_MUX(RK3399_SCLK_UART1, "clk_uart1", mux_uart1_parents, CLKSEL_CON(34), __BITS(9,8)),
	RK_MUX(RK3399_SCLK_UART2, "clk_uart2", mux_uart2_parents, CLKSEL_CON(35), __BITS(9,8)),
	RK_MUX(RK3399_SCLK_UART3, "clk_uart3", mux_uart3_parents, CLKSEL_CON(36), __BITS(9,8)),
	RK_GATE(RK3399_PCLK_UART0, "pclk_uart0", "pclk_perilp1", CLKGATE_CON(22), 0),
	RK_GATE(RK3399_PCLK_UART1, "pclk_uart1", "pclk_perilp1", CLKGATE_CON(22), 1),
	RK_GATE(RK3399_PCLK_UART2, "pclk_uart2", "pclk_perilp1", CLKGATE_CON(22), 2),
	RK_GATE(RK3399_PCLK_UART3, "pclk_uart3", "pclk_perilp1", CLKGATE_CON(22), 3),

	/*
	 * SDMMC/SDIO
	 */
	RK_COMPOSITE(RK3399_HCLK_SD, "hclk_sd", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(13),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(12,8),	/* div_mask */
		     CLKGATE_CON(12),	/* gate_reg */
		     __BIT(13),		/* gate_mask */
		     RK_COMPOSITE_ROUND_DOWN),
	RK_COMPOSITE(RK3399_SCLK_SDIO, "clk_sdio", mux_pll_src_cpll_gpll_npll_ppll_upll_24m_parents,
		     CLKSEL_CON(15),	/* muxdiv_reg */
		     __BITS(10,8),	/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(6),	/* gate_reg */
		     __BIT(0),		/* gate_mask */
		     RK_COMPOSITE_ROUND_DOWN),
	RK_COMPOSITE(RK3399_SCLK_SDMMC, "clk_sdmmc", mux_pll_src_cpll_gpll_npll_ppll_upll_24m_parents,
		     CLKSEL_CON(16),	/* muxdiv_reg */
		     __BITS(10,8),	/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(6),	/* gate_reg */
		     __BIT(1),		/* gate_mask */
		     RK_COMPOSITE_ROUND_DOWN),
	RK_GATE(RK3399_HCLK_SDMMC, "hclk_sdmmc", "hclk_sd", CLKGATE_CON(33), 8),
	RK_GATE(RK3399_HCLK_SDIO, "hclk_sdio", "pclk_perilp1", CLKGATE_CON(34), 4),

	/*
	 * eMMC
	 */
	RK_COMPOSITE(RK3399_SCLK_EMMC, "clk_emmc", mux_pll_src_cpll_gpll_npll_ppll_upll_24m_parents,
		     CLKSEL_CON(22),	/* muxdiv_reg */
		     __BITS(10,8),	/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(6),	/* gate_reg */
		     __BIT(14),		/* gate_mask */
		     RK_COMPOSITE_ROUND_DOWN),
	RK_GATE(0, "cpll_aclk_emmc_src", "cpll", CLKGATE_CON(6), 13),
	RK_GATE(0, "gpll_aclk_emmc_src", "gpll", CLKGATE_CON(6), 12),
	RK_COMPOSITE_NOGATE(RK3399_ACLK_EMMC, "aclk_emmc", mux_aclk_emmc_parents,
			    CLKSEL_CON(21),	/* muxdiv_reg */
			    __BIT(7),		/* mux_mask */
			    __BITS(4,0),	/* div_mask */
			    0),
	RK_GATE(RK3399_ACLK_EMMC_CORE, "aclk_emmccore", "aclk_emmc", CLKGATE_CON(32), 8),
	RK_GATE(RK3399_ACLK_EMMC_NOC, "aclk_emmc_noc", "aclk_emmc", CLKGATE_CON(32), 9),
	RK_GATE(RK3399_ACLK_EMMC_GRF, "aclk_emmcgrf", "aclk_emmc", CLKGATE_CON(32), 10),

	/*
	 * GMAC
	 */
	RK_COMPOSITE(RK3399_SCLK_MAC, "clk_gmac", mux_pll_src_cpll_gpll_npll_parents,
		     CLKSEL_CON(20),	/* muxdiv_reg */
		     __BITS(15,14),	/* mux_mask */
		     __BITS(12,8),	/* div_mask */
		     CLKGATE_CON(5),	/* gate_reg */
		     __BIT(5),		/* gate_mask */
		     0),
	RK_MUX(RK3399_SCLK_RMII_SRC, "clk_rmii_src", mux_rmii_parents, CLKSEL_CON(19), __BIT(4)),
	RK_GATE(RK3399_SCLK_MACREF_OUT, "clk_mac_refout", "clk_rmii_src", CLKGATE_CON(5), 6),
	RK_GATE(RK3399_SCLK_MACREF, "clk_mac_ref", "clk_rmii_src", CLKGATE_CON(5), 7),
	RK_GATE(RK3399_SCLK_MAC_RX, "clk_rmii_rx", "clk_rmii_src", CLKGATE_CON(5), 8),
	RK_GATE(RK3399_SCLK_MAC_TX, "clk_rmii_tx", "clk_rmii_src", CLKGATE_CON(5), 9),
	RK_GATE(0, "gpll_aclk_gmac_src", "gpll", CLKGATE_CON(6), 8),
	RK_GATE(0, "cpll_aclk_gmac_src", "cpll", CLKGATE_CON(6), 9),
	RK_COMPOSITE(0, "aclk_gmac_pre", mux_aclk_gmac_parents,
		     CLKSEL_CON(20),	/* muxdiv_reg */
		     __BIT(17),		/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     CLKGATE_CON(6),	/* gate_reg */
		     __BIT(10),		/* gate_mask */
		     0),
	RK_GATE(RK3399_ACLK_GMAC, "aclk_gmac", "aclk_gmac_pre", CLKGATE_CON(32), 0),
	RK_COMPOSITE_NOMUX(0, "pclk_gmac_pre", "aclk_gmac_pre",
			   CLKSEL_CON(19),	/* div_reg */
			   __BITS(10,8),	/* div_mask */
			   CLKGATE_CON(6),	/* gate_reg */
			   __BIT(11),		/* gate_mask */
			   0),
	RK_GATE(RK3399_PCLK_GMAC, "pclk_gmac", "pclk_gmac_pre", CLKGATE_CON(32), 2),

	/*
	 * USB2
	 */
	RK_GATE(RK3399_HCLK_HOST0, "hclk_host0", "hclk_perihp", CLKGATE_CON(20), 5),
	RK_GATE(RK3399_HCLK_HOST0_ARB, "hclk_host0_arb", "hclk_perihp", CLKGATE_CON(20), 6),
	RK_GATE(RK3399_HCLK_HOST1, "hclk_host1", "hclk_perihp", CLKGATE_CON(20), 7),
	RK_GATE(RK3399_HCLK_HOST1_ARB, "hclk_host1_arb", "hclk_perihp", CLKGATE_CON(20), 8),
	RK_GATE(RK3399_SCLK_USB2PHY0_REF, "clk_usb2phy0_ref", "xin24m", CLKGATE_CON(6), 5),
	RK_GATE(RK3399_SCLK_USB2PHY1_REF, "clk_usb2phy1_ref", "xin24m", CLKGATE_CON(6), 6),

	/*
	 * USB3
	 */
	RK_GATE(RK3399_SCLK_USB3OTG0_REF, "clk_usb3otg0_ref", "xin24m", CLKGATE_CON(12), 1),
	RK_GATE(RK3399_SCLK_USB3OTG1_REF, "clk_usb3otg1_ref", "xin24m", CLKGATE_CON(12), 2),
	RK_COMPOSITE(RK3399_SCLK_USB3OTG0_SUSPEND, "clk_usb3otg0_suspend", pll_parents,
		     CLKSEL_CON(40),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(9,0),	/* div_mask */
		     CLKGATE_CON(12),	/* gate_reg */
		     __BIT(3),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3399_SCLK_USB3OTG1_SUSPEND, "clk_usb3otg1_suspend", pll_parents,
		     CLKSEL_CON(41),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(9,0),	/* div_mask */
		     CLKGATE_CON(12),	/* gate_reg */
		     __BIT(4),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3399_ACLK_USB3, "aclk_usb3", mux_pll_src_cpll_gpll_npll_parents,
		     CLKSEL_CON(39),	/* muxdiv_reg */
		     __BITS(7,6),	/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     CLKGATE_CON(12),	/* gate_reg */
		     __BIT(0),		/* gate_mask */
		     0),
	RK_GATE(RK3399_ACLK_USB3OTG0, "aclk_usb3otg0", "aclk_usb3", CLKGATE_CON(30), 1),
	RK_GATE(RK3399_ACLK_USB3OTG1, "aclk_usb3otg1", "aclk_usb3", CLKGATE_CON(30), 2),
	RK_GATE(RK3399_ACLK_USB3_RKSOC_AXI_PERF, "aclk_usb3_rksoc_axi_perf", "aclk_usb3", CLKGATE_CON(30), 3),
	RK_GATE(RK3399_ACLK_USB3_GRF, "aclk_usb3_grf", "aclk_usb3", CLKGATE_CON(30), 4),

	/*
	 * I2C
	 */
	RK_COMPOSITE(RK3399_SCLK_I2C1, "clk_i2c1", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(61),	/* muxdiv_reg */
		     __BIT(7),		/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(10),	/* gate_reg */
		     __BIT(0),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3399_SCLK_I2C2, "clk_i2c2", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(62),	/* muxdiv_reg */
		     __BIT(7),		/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(10),	/* gate_reg */
		     __BIT(2),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3399_SCLK_I2C3, "clk_i2c3", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(63),	/* muxdiv_reg */
		     __BIT(7),		/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(10),	/* gate_reg */
		     __BIT(4),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3399_SCLK_I2C5, "clk_i2c5", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(61),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(14,8),	/* div_mask */
		     CLKGATE_CON(10),	/* gate_reg */
		     __BIT(1),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3399_SCLK_I2C6, "clk_i2c6", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(62),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(14,8),	/* div_mask */
		     CLKGATE_CON(10),	/* gate_reg */
		     __BIT(3),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3399_SCLK_I2C7, "clk_i2c7", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(63),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(14,8),	/* div_mask */
		     CLKGATE_CON(10),	/* gate_reg */
		     __BIT(5),		/* gate_mask */
		     0),
	RK_GATE(RK3399_PCLK_I2C7, "pclk_rki2c7", "pclk_perilp1", CLKGATE_CON(22), 5),
	RK_GATE(RK3399_PCLK_I2C1, "pclk_rki2c1", "pclk_perilp1", CLKGATE_CON(22), 6),
	RK_GATE(RK3399_PCLK_I2C5, "pclk_rki2c5", "pclk_perilp1", CLKGATE_CON(22), 7),
	RK_GATE(RK3399_PCLK_I2C6, "pclk_rki2c6", "pclk_perilp1", CLKGATE_CON(22), 8),
	RK_GATE(RK3399_PCLK_I2C2, "pclk_rki2c2", "pclk_perilp1", CLKGATE_CON(22), 9),
	RK_GATE(RK3399_PCLK_I2C3, "pclk_rki2c3", "pclk_perilp1", CLKGATE_CON(22), 10),

	/*
	 * SPI
	 */
	RK_COMPOSITE(RK3399_SCLK_SPI0, "clk_spi0", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(59),	/* muxdiv_reg */
		     __BIT(7),		/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(9),	/* gate_reg */
		     __BIT(12),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3399_SCLK_SPI1, "clk_spi1", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(59),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(14,8),	/* div_mask */
		     CLKGATE_CON(9),	/* gate_reg */
		     __BIT(13),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3399_SCLK_SPI2, "clk_spi2", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(60),	/* muxdiv_reg */
		     __BIT(7),		/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(9),	/* gate_reg */
		     __BIT(14),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3399_SCLK_SPI4, "clk_spi4", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(60),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(14,8),	/* div_mask */
		     CLKGATE_CON(9),	/* gate_reg */
		     __BIT(15),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3399_SCLK_SPI5, "clk_spi5", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(58),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(14,8),	/* div_mask */
		     CLKGATE_CON(13),	/* gate_reg */
		     __BIT(13),		/* gate_mask */
		     0),
	RK_GATE(RK3399_PCLK_SPI0, "pclk_rkspi0", "pclk_perilp1", CLKGATE_CON(23), 10),
	RK_GATE(RK3399_PCLK_SPI1, "pclk_rkspi1", "pclk_perilp1", CLKGATE_CON(23), 11),
	RK_GATE(RK3399_PCLK_SPI2, "pclk_rkspi2", "pclk_perilp1", CLKGATE_CON(23), 12),
	RK_GATE(RK3399_PCLK_SPI4, "pclk_rkspi4", "pclk_perilp1", CLKGATE_CON(23), 13),
	RK_GATE(RK3399_PCLK_SPI5, "pclk_rkspi5", "hclk_perilp1", CLKGATE_CON(34), 5),

	/* Watchdog */
	RK_SECURE_GATE(RK3399_PCLK_WDT, "pclk_wdt", "pclk_alive" /*, SECURE_CLKGATE_CON(3), 8 */),

	/* PCIe */
	RK_GATE(RK3399_ACLK_PERF_PCIE, "aclk_perf_pcie", "aclk_perihp", CLKGATE_CON(20), 2),
	RK_GATE(RK3399_ACLK_PCIE, "aclk_pcie", "aclk_perihp", CLKGATE_CON(20), 10),
	RK_GATE(RK3399_PCLK_PCIE, "pclk_pcie", "pclk_perihp", CLKGATE_CON(20), 11),
	RK_COMPOSITE(RK3399_SCLK_PCIE_PM, "clk_pcie_pm", mux_pll_src_cpll_gpll_npll_24m_parents,
		     CLKSEL_CON(17),	/* muxdiv_reg */
		     __BITS(10,8),	/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(6),	/* gate_reg */
		     __BIT(2),		/* gate_mask */
		     0),
	RK_COMPOSITE_NOMUX(RK3399_SCLK_PCIEPHY_REF100M, "clk_pciephy_ref100m", "npll",
			   CLKSEL_CON(18),	/* div_reg */
			   __BITS(15,11),	/* div_mask */
			   CLKGATE_CON(12),	/* gate_reg */
			   __BIT(6),		/* gate_mask */
			   0),
	RK_MUX(RK3399_SCLK_PCIEPHY_REF, "clk_pciephy_ref", mux_pll_src_24m_pciephy_parents, CLKSEL_CON(18), __BIT(10)),
	RK_COMPOSITE(0, "clk_pcie_core_cru", mux_pll_src_cpll_gpll_npll_parents,
		     CLKSEL_CON(18),	/* muxdiv_reg */
		     __BITS(9,8),	/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(6),	/* gate_reg */
		     __BIT(3),		/* gate_mask */
		     0),
	RK_MUX(RK3399_SCLK_PCIE_CORE, "clk_pcie_core", mux_pciecore_cru_phy_parents, CLKSEL_CON(18), __BIT(7)),

	/* Crypto */
	RK_COMPOSITE(RK3399_SCLK_CRYPTO0, "clk_crypto0", mux_pll_src_cpll_gpll_ppll_parents,
		     CLKSEL_CON(24),	/* muxdiv_reg */
		     __BITS(7,6),	/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     CLKGATE_CON(7),	/* gate_reg */
		     __BIT(7),		/* gate_mask */
		     RK_COMPOSITE_ROUND_DOWN /*???*/),
	RK_COMPOSITE(RK3399_SCLK_CRYPTO1, "clk_crypto1", mux_pll_src_cpll_gpll_ppll_parents,
		     CLKSEL_CON(26),	/* muxdiv_reg */
		     __BITS(7,6),	/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     CLKGATE_CON(8),	/* gate_reg */
		     __BIT(7),		/* gate_mask */
		     RK_COMPOSITE_ROUND_DOWN /*???*/),
	RK_GATE(RK3399_HCLK_M_CRYPTO0, "hclk_m_crypto0", "pclk_perilp0", CLKGATE_CON(24), 5),
	RK_GATE(RK3399_HCLK_S_CRYPTO0, "hclk_s_crypto0", "pclk_perilp0", CLKGATE_CON(24), 6),
	RK_GATE(RK3399_HCLK_M_CRYPTO1, "hclk_m_crypto1", "pclk_perilp0", CLKGATE_CON(24), 14),
	RK_GATE(RK3399_HCLK_S_CRYPTO1, "hclk_s_crypto1", "pclk_perilp0", CLKGATE_CON(24), 15),
	RK_GATE(RK3399_ACLK_DMAC1_PERILP, "aclk_dmac1_perilp", "pclk_perilp", CLKGATE_CON(25), 6),

	/* TSADC */
	RK_COMPOSITE(RK3399_SCLK_TSADC, "clk_tsadc", mux_clk_tsadc_parents,
		     CLKSEL_CON(27),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(9,0),	/* div_mask */
		     CLKGATE_CON(9),	/* gate_reg */
		     __BIT(1),		/* gate_mask */
		     RK_COMPOSITE_ROUND_DOWN),
	RK_GATE(RK3399_PCLK_TSADC, "pclk_tsadc", "pclk_perilp1", CLKGATE_CON(22), 13),

	/* VOP0 */
	RK_COMPOSITE(RK3399_ACLK_VOP0_PRE, "aclk_vop0_pre", mux_pll_src_vpll_cpll_gpll_npll_parents,
		     CLKSEL_CON(47),	/* muxdiv_reg */
		     __BITS(7,6),	/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     CLKGATE_CON(10),	/* gate_reg */
		     __BIT(8),		/* gate_mask */
		     0),
	RK_COMPOSITE_NOMUX(0, "hclk_vop0_pre", "aclk_vop0_pre",
			   CLKSEL_CON(47),	/* div_reg */
			   __BITS(12,8),	/* div_mask */
			   CLKGATE_CON(10),	/* gate_reg */
			   __BIT(9),		/* gate_mask */
			   0),
	RK_COMPOSITE(RK3399_DCLK_VOP0_DIV, "dclk_vop0_div", mux_pll_src_vpll_cpll_gpll_parents,
		     CLKSEL_CON(49),	/* muxdiv_reg */
		     __BITS(9,8),	/* mux_mask */
		     __BITS(7,0),	/* div_mask */
		     CLKGATE_CON(10),	/* gate_reg */
		     __BIT(12),		/* gate_mask */
		     RK_COMPOSITE_SET_RATE_PARENT),
	RK_GATE(RK3399_ACLK_VOP0, "aclk_vop0", "aclk_vop0_pre", CLKGATE_CON(28), 3),
	RK_GATE(RK3399_HCLK_VOP0, "hclk_vop0", "hclk_vop0_pre", CLKGATE_CON(28), 2),
	RK_COMPOSITE_FRAC(RK3399_DCLK_VOP0_FRAC, "dclk_vop0_frac", "dclk_vop0_div",
			  CLKSEL_CON(106),	/* frac_reg */
			  0),
	RK_MUX(RK3399_DCLK_VOP0, "dclk_vop0", mux_dclk_vop0_parents, CLKSEL_CON(49), __BIT(11)),

	/* VOP1 */
	RK_COMPOSITE(RK3399_ACLK_VOP1_PRE, "aclk_vop1_pre", mux_pll_src_vpll_cpll_gpll_npll_parents,
		     CLKSEL_CON(48),	/* muxdiv_reg */
		     __BITS(7,6),	/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     CLKGATE_CON(10),	/* gate_reg */
		     __BIT(10),		/* gate_mask */
		     0),
	RK_COMPOSITE_NOMUX(0, "hclk_vop1_pre", "aclk_vop1_pre",
			   CLKSEL_CON(48),	/* div_reg */
			   __BITS(12,8),	/* div_mask */
			   CLKGATE_CON(10),	/* gate_reg */
			   __BIT(11),		/* gate_mask */
			   0),
	RK_COMPOSITE(RK3399_DCLK_VOP1_DIV, "dclk_vop1_div", mux_pll_src_vpll_cpll_gpll_parents,
		     CLKSEL_CON(50),	/* muxdiv_reg */
		     __BITS(9,8),	/* mux_mask */
		     __BITS(7,0),	/* div_mask */
		     CLKGATE_CON(10),	/* gate_reg */
		     __BIT(13),		/* gate_mask */
		     RK_COMPOSITE_SET_RATE_PARENT),
	RK_GATE(RK3399_ACLK_VOP1, "aclk_vop1", "aclk_vop1_pre", CLKGATE_CON(28), 7),
	RK_GATE(RK3399_HCLK_VOP1, "hclk_vop1", "hclk_vop1_pre", CLKGATE_CON(28), 6),
	RK_COMPOSITE_FRAC(RK3399_DCLK_VOP1_FRAC, "dclk_vop1_frac", "dclk_vop1_div",
			  CLKSEL_CON(107),	/* frac_reg */
			  0),
	RK_MUX(RK3399_DCLK_VOP1, "dclk_vop1", mux_dclk_vop1_parents, CLKSEL_CON(50), __BIT(11)),

	/* VIO */
	RK_COMPOSITE(RK3399_ACLK_VIO, "aclk_vio", mux_pll_src_cpll_gpll_ppll_parents,
		     CLKSEL_CON(42),	/* muxdiv_reg */
		     __BITS(7,6),	/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     CLKGATE_CON(11),	/* gate_reg */
		     __BIT(0),		/* gate_mask */
		     0),
	RK_COMPOSITE_NOMUX(RK3399_PCLK_VIO, "pclk_vio", "aclk_vio",
			   CLKSEL_CON(43),	/* div_reg */
			   __BITS(4,0),		/* div_mask */
			   CLKGATE_CON(11),	/* gate_reg */
			   __BIT(1),		/* gate_mask */
			   0),
	RK_GATE(RK3399_PCLK_VIO_GRF, "pclk_vio_grf", "pclk_vio", CLKGATE_CON(29), 12),

	/* HDMI */
	RK_COMPOSITE(RK3399_ACLK_HDCP, "aclk_hdcp", mux_pll_src_cpll_gpll_ppll_parents,
		     CLKSEL_CON(42),	/* muxdiv_reg */
		     __BITS(15,14),	/* mux_mask */
		     __BITS(12,8),	/* div_mask */
		     CLKGATE_CON(11),	/* gate_reg */
		     __BIT(12),		/* gate_mask */
		     0),
	RK_COMPOSITE_NOMUX(RK3399_PCLK_HDCP, "pclk_hdcp", "aclk_hdcp",
			   CLKSEL_CON(43),	/* div_reg */
			   __BITS(14,10),	/* div_mask */
			   CLKGATE_CON(11),	/* gate_reg */
			   __BIT(10),		/* gate_mask */
			   0),
	RK_COMPOSITE(RK3399_SCLK_HDMI_CEC, "clk_hdmi_cec", pll_parents,
		     CLKSEL_CON(45),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(9,0),	/* div_mask */
		     CLKGATE_CON(11),	/* gate_reg */
		     __BIT(7),		/* gate_mask */
		     0),
	RK_GATE(RK3399_PCLK_HDMI_CTRL, "pclk_hdmi_ctrl", "pclk_hdcp", CLKGATE_CON(29), 6),
	RK_GATE(RK3399_SCLK_HDMI_SFR, "clk_hdmi_sfr", "xin24m", CLKGATE_CON(11), 6),

	/* I2S2 */
	RK_COMPOSITE(0, "clk_i2s0_div", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(28),	/* muxdiv_reg */
		     __BIT(7),		/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(8),	/* gate_reg */
		     __BIT(3),		/* gate_mask */
		     0),
	RK_COMPOSITE(0, "clk_i2s1_div", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(29),	/* muxdiv_reg */
		     __BIT(7),		/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(8),	/* gate_reg */
		     __BIT(6),		/* gate_mask */
		     0),
	RK_COMPOSITE(0, "clk_i2s2_div", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(30),	/* muxdiv_reg */
		     __BIT(7),		/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(8),	/* gate_reg */
		     __BIT(9),		/* gate_mask */
		     0),
	RK_COMPOSITE_FRAC(0, "clk_i2s0_frac", "clk_i2s0_div",
			  CLKSEL_CON(96),	/* frac_reg */
			  0),
	RK_COMPOSITE_FRAC(0, "clk_i2s1_frac", "clk_i2s1_div",
			  CLKSEL_CON(97),	/* frac_reg */
			  0),
	RK_COMPOSITE_FRAC(0, "clk_i2s2_frac", "clk_i2s2_div",
			  CLKSEL_CON(98),	/* frac_reg */
			  0),
	RK_MUX(0, "clk_i2s0_mux", mux_i2s0_parents, CLKSEL_CON(28), __BITS(9,8)),
	RK_MUX(0, "clk_i2s1_mux", mux_i2s1_parents, CLKSEL_CON(29), __BITS(9,8)),
	RK_MUX(0, "clk_i2s2_mux", mux_i2s2_parents, CLKSEL_CON(30), __BITS(9,8)),
	RK_GATE(RK3399_SCLK_I2S0_8CH, "clk_i2s0", "clk_i2s0_mux", CLKGATE_CON(8), 5),
	RK_GATE(RK3399_SCLK_I2S1_8CH, "clk_i2s1", "clk_i2s1_mux", CLKGATE_CON(8), 8),
	RK_GATE(RK3399_SCLK_I2S2_8CH, "clk_i2s2", "clk_i2s2_mux", CLKGATE_CON(8), 11),
	RK_GATE(RK3399_HCLK_I2S0_8CH, "hclk_i2s0", "hclk_perilp1", CLKGATE_CON(34), 0),
	RK_GATE(RK3399_HCLK_I2S1_8CH, "hclk_i2s1", "hclk_perilp1", CLKGATE_CON(34), 1),
	RK_GATE(RK3399_HCLK_I2S2_8CH, "hclk_i2s2", "hclk_perilp1", CLKGATE_CON(34), 2),
	RK_MUX(0, "clk_i2sout_src", mux_i2sch_parents, CLKSEL_CON(31), __BITS(1,0)),
	RK_COMPOSITE(RK3399_SCLK_I2S_8CH_OUT, "clk_i2sout", mux_i2sout_parents,
		     CLKSEL_CON(31),	/* muxdiv_reg */
		     __BIT(2),		/* mux_mask */
		     0,			/* div_mask */
		     CLKGATE_CON(8),	/* gate_reg */
		     __BIT(12),		/* gate_mask */
		     RK_COMPOSITE_SET_RATE_PARENT),

	/* eDP */
	RK_COMPOSITE(RK3399_PCLK_EDP, "pclk_edp", mux_pll_src_cpll_gpll_parents,
		     CLKSEL_CON(44),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(13,8),	/* div_mask */
		     CLKGATE_CON(11),	/* gate_reg */
		     __BIT(11),		/* gate_mask */
		     0),
	RK_GATE(RK3399_PCLK_EDP_NOC, "pclk_edp_noc", "pclk_edp", CLKGATE_CON(32), 12),
	RK_GATE(RK3399_PCLK_EDP_CTRL, "pclk_edp_ctrl", "pclk_edp", CLKGATE_CON(32), 13),

	RK_COMPOSITE(RK3399_SCLK_DP_CORE, "clk_dp_core", mux_pll_src_npll_cpll_gpll_parents,
		     CLKSEL_CON(46),	/* muxdiv_reg */
		     __BITS(7,6),	/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     CLKGATE_CON(11),	/* gate_reg */
		     __BIT(8),		/* gate_mask */
		     0),
	RK_GATE(RK3399_PCLK_DP_CTRL, "pclk_dp_ctrl", "pclk_hdcp", CLKGATE_CON(29), 7),

};

static const struct rk3399_init_param {
	const char *clk;
	const char *parent;
} rk3399_init_params[] = {
	{ .clk = "clk_i2s0_mux",	.parent = "clk_i2s0_frac" },
	{ .clk = "clk_i2s1_mux",	.parent = "clk_i2s1_frac" },
	{ .clk = "clk_i2s2_mux",	.parent = "clk_i2s2_frac" },
	{ .clk = "dclk_vop0_div",	.parent = "gpll" },
	{ .clk = "dclk_vop1_div",	.parent = "gpll" },
	{ .clk = "dclk_vop0",		.parent = "dclk_vop0_frac" },
	{ .clk = "dclk_vop1",		.parent = "dclk_vop1_frac" },
};

static void
rk3399_cru_init(struct rk_cru_softc *sc)
{
	struct rk_cru_clk *clk, *pclk;
	uint32_t write_mask, write_val;
	int error;
	u_int n;

	/*
	 * Force an update of BPLL to bring it out of slow mode.
	 */
	clk = rk_cru_clock_find(sc, "armclkb");
	clk_set_rate(&clk->base, clk_get_rate(&clk->base));

	/*
	 * Set DCLK_VOP0 and DCLK_VOP1 dividers to 1.
	 */
	write_mask = __BITS(7,0) << 16;
	write_val = 0;
	CRU_WRITE(sc, CLKSEL_CON(49), write_mask | write_val);
	CRU_WRITE(sc, CLKSEL_CON(50), write_mask | write_val);

	/*
	 * Set defaults
	 */
	for (n = 0; n < __arraycount(rk3399_init_params); n++) {
		const struct rk3399_init_param *param = &rk3399_init_params[n];
		clk = rk_cru_clock_find(sc, param->clk);
		KASSERTMSG(clk != NULL, "couldn't find clock %s", param->clk);
		if (param->parent != NULL) {
			pclk = rk_cru_clock_find(sc, param->parent);
			KASSERTMSG(pclk != NULL, "couldn't find clock %s", param->parent);
			error = clk_set_parent(&clk->base, &pclk->base);
			if (error != 0) {
				aprint_error_dev(sc->sc_dev, "couldn't set %s parent to %s: %d\n",
				    param->clk, param->parent, error);
				continue;
			}
		}
	}
}

static int
rk3399_cru_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk3399_cru_attach(device_t parent, device_t self, void *aux)
{
	struct rk_cru_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_clks = rk3399_cru_clks;
	sc->sc_nclks = __arraycount(rk3399_cru_clks);

	sc->sc_grf_soc_status = 0x0480;
	sc->sc_softrst_base = SOFTRST_CON(0);

	if (rk_cru_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": RK3399 CRU\n");

	rk3399_cru_init(sc);

	rk_cru_print(sc);
}
