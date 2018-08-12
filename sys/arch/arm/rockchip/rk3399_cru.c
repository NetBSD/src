/* $NetBSD: rk3399_cru.c,v 1.1 2018/08/12 16:48:05 jmcneill Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: rk3399_cru.c,v 1.1 2018/08/12 16:48:05 jmcneill Exp $");

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

static const char * const compatible[] = {
	"rockchip,rk3399-cru",
	NULL
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
	int retry;

	KASSERT(clk->type == RK_CRU_PLL);

	if (pll->rates == NULL || rate == 0)
		return EIO;

	for (int i = 0; i < pll->nrates; i++)
		if (pll->rates[i].rate == rate) {
			pll_rate = &pll->rates[i];
			break;
		}
	if (pll_rate == NULL)
		return EINVAL;

	val = __SHIFTIN(PLL_WORK_MODE_SLOW, PLL_WORK_MODE) | (PLL_WORK_MODE << 16);
	CRU_WRITE(sc, pll->con_base + PLL_CON3, val);

	CRU_WRITE(sc, pll->con_base + PLL_CON0,
	    __SHIFTIN(pll_rate->fbdiv, PLL_FBDIV) |
	    PLL_WRITE_MASK);

	CRU_WRITE(sc, pll->con_base + PLL_CON1,
	    __SHIFTIN(pll_rate->postdiv2, PLL_POSTDIV2) |
	    __SHIFTIN(pll_rate->postdiv1, PLL_POSTDIV1) |
	    __SHIFTIN(pll_rate->refdiv, PLL_REFDIV) |
	    PLL_WRITE_MASK);

	val = CRU_READ(sc, pll->con_base + PLL_CON2);
	val &= ~PLL_FRACDIV;
	val |= __SHIFTIN(pll_rate->fracdiv, PLL_FRACDIV);
	CRU_WRITE(sc, pll->con_base + PLL_CON2, val);

	val = __SHIFTIN(pll_rate->dsmpd, PLL_DSMPD) | (PLL_DSMPD << 16);
	CRU_WRITE(sc, pll->con_base + PLL_CON3, val);

	/* Set PLL work mode to normal */
	const uint32_t write_mask = pll->mode_mask << 16;
	const uint32_t write_val = pll->mode_mask;
	CRU_WRITE(sc, pll->mode_reg, write_mask | write_val);

	for (retry = 1000; retry > 0; retry--) {
		if (CRU_READ(sc, pll->con_base + PLL_CON2) & pll->lock_mask)
			break;
		delay(1);
	}

	if (retry == 0)
		device_printf(sc->sc_dev, "WARNING: %s failed to lock\n",
		    clk->base.name);

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
static const char * mux_pll_src_cpll_gpll_parents[] = { "cpll", "gpll" };
static const char * mux_pll_src_cpll_gpll_npll_parents[] = { "cpll", "gpll", "npll" };
static const char * mux_pll_src_cpll_gpll_upll_parents[] = { "cpll", "gpll", "upll" };
static const char * mux_pll_src_cpll_gpll_npll_ppll_upll_24m_parents[] = { "cpll", "gpll", "npll", "ppll", "upll", "xin24m" };
static const char * mux_aclk_perilp0_parents[] = { "cpll_aclk_perilp0_src", "gpll_aclk_perilp0_src" };
static const char * mux_hclk_perilp1_parents[] = { "cpll_hclk_perilp1_src", "gpll_hclk_perilp1_src" };
static const char * mux_aclk_perihp_parents[] = { "cpll_aclk_perihp_src", "gpll_aclk_perihp_src" };
static const char * mux_aclk_cci_parents[] = { "cpll_aclk_cci_src", "gpll_aclk_cci_src", "npll_aclk_cci_src", "vpll_aclk_cci_src" };
static const char * mux_uart0_parents[] = { "clk_uart0_div", "clk_uart0_frac", "xin24m" };
static const char * mux_uart1_parents[] = { "clk_uart1_div", "clk_uart1_frac", "xin24m" };
static const char * mux_uart2_parents[] = { "clk_uart2_div", "clk_uart2_frac", "xin24m" };
static const char * mux_uart3_parents[] = { "clk_uart3_div", "clk_uart3_frac", "xin24m" };
static const char * mux_rmii_parents[] = { "clk_gmac", "clkin_gmac" };
static const char * mux_aclk_gmac_parents[] = { "cpll_aclk_gmac_src", "gpll_aclk_gmac_src" };

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
		   PLL_CON(43),		/* con_base */
		   PLL_CON(51),		/* mode_reg */
		   __BIT(8),		/* mode_mask */
		   __BIT(31),		/* lock_mask */
		   pll_rates),

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
		     0),
	RK_COMPOSITE(RK3399_SCLK_SDIO, "clk_sdio", mux_pll_src_cpll_gpll_npll_ppll_upll_24m_parents,
		     CLKSEL_CON(15),	/* muxdiv_reg */
		     __BITS(10,8),	/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(6),	/* gate_reg */
		     __BIT(0),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3399_SCLK_SDMMC, "clk_sdmmc", mux_pll_src_cpll_gpll_npll_ppll_upll_24m_parents,
		     CLKSEL_CON(16),	/* muxdiv_reg */
		     __BITS(10,8),	/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(6),	/* gate_reg */
		     __BIT(1),		/* gate_mask */
		     0),
	RK_GATE(RK3399_HCLK_SDMMC, "hclk_sdmmc", "hclk_sd", CLKGATE_CON(33), 8),
	RK_GATE(RK3399_HCLK_SDIO, "hclk_sdio", "pclk_perilp1", CLKGATE_CON(34), 4),

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
};

static int
rk3399_cru_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
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

	sc->sc_softrst_base = SOFTRST_CON(0);

	if (rk_cru_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": RK3399 CRU\n");

	rk_cru_print(sc);
}
