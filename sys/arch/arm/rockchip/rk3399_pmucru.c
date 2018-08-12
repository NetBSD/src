/* $NetBSD: rk3399_pmucru.c,v 1.1 2018/08/12 16:48:05 jmcneill Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: rk3399_pmucru.c,v 1.1 2018/08/12 16:48:05 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/rockchip/rk_cru.h>
#include <arm/rockchip/rk3399_pmucru.h>

#define	PLL_CON(n)	(0x0000 + (n) * 4)
#define	CLKSEL_CON(n)	(0x0080 + (n) * 4)
#define	CLKGATE_CON(n)	(0x0100 + (n) * 4)
#define	SOFTRST_CON(n)	(0x0110 + (n) * 4)

static int rk3399_pmucru_match(device_t, cfdata_t, void *);
static void rk3399_pmucru_attach(device_t, device_t, void *);

static const char * const compatible[] = {
	"rockchip,rk3399-pmucru",
	NULL
};

CFATTACH_DECL_NEW(rk3399_pmucru, sizeof(struct rk_cru_softc),
	rk3399_pmucru_match, rk3399_pmucru_attach, NULL, NULL);

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
rk3399_pmucru_pll_get_rate(struct rk_cru_softc *sc,
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
rk3399_pmucru_pll_set_rate(struct rk_cru_softc *sc,
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
                .get_rate = rk3399_pmucru_pll_get_rate,            \
                .set_rate = rk3399_pmucru_pll_set_rate,            \
                .get_parent = rk_cru_pll_get_parent,            \
        }

static const char * pll_parents[] = { "xin24m", "xin32k" };

static struct rk_cru_clk rk3399_pmucru_clks[] = {
	RK3399_PLL(RK3399_PLL_PPLL, "ppll", pll_parents,
		   PLL_CON(0),		/* con_base */
		   PLL_CON(3),		/* mode_reg */
		   __BIT(8),		/* mode_mask */
		   __BIT(31),		/* lock_mask */
		   pll_rates),

	RK_DIV(RK3399_PCLK_SRC_PMU, "pclk_pmu_src", "ppll", CLKSEL_CON(0), __BITS(4,0), 0),

	RK_GATE(RK3399_PCLK_PMU, "pclk_pmu", "pclk_pmu_src", CLKGATE_CON(1), 0),
	RK_GATE(RK3399_PCLK_GPIO0_PMU, "pclk_gpio0_pmu", "pclk_pmu_src", CLKGATE_CON(1), 3),
	RK_GATE(RK3399_PCLK_GPIO1_PMU, "pclk_gpio1_pmu", "pclk_pmu_src", CLKGATE_CON(1), 4),
};

static int
rk3399_pmucru_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
rk3399_pmucru_attach(device_t parent, device_t self, void *aux)
{
	struct rk_cru_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_clks = rk3399_pmucru_clks;
	sc->sc_nclks = __arraycount(rk3399_pmucru_clks);

	sc->sc_softrst_base = SOFTRST_CON(0);

	if (rk_cru_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": RK3399 PMU CRU\n");

	rk_cru_print(sc);
}
