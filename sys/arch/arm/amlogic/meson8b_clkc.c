/* $NetBSD: meson8b_clkc.c,v 1.3 2019/02/25 19:30:17 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(1, "$NetBSD: meson8b_clkc.c,v 1.3 2019/02/25 19:30:17 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/amlogic/meson_clk.h>
#include <arm/amlogic/meson8b_clkc.h>

/*
 * The DT for amlogic,meson8b-clkc defines two reg resources. The first
 * is not used by this driver.
 */
#define	MESON8B_CLKC_REG_INDEX	1

#define	CBUS_REG(x)	((x) << 2)

#define	HHI_GCLK_MPEG0		CBUS_REG(0x50)
#define	HHI_GCLK_MPEG1		CBUS_REG(0x51)
#define	HHI_GCLK_MPEG2		CBUS_REG(0x52)
#define	HHI_SYS_CPU_CLK_CNTL1	CBUS_REG(0x57)
#define	HHI_MPEG_CLK_CNTL	CBUS_REG(0x5d)
#define	HHI_SYS_CPU_CLK_CNTL0	CBUS_REG(0x67)
#define	 HHI_SYS_CPU_CLK_CNTL0_CLKSEL	__BIT(7)
#define	 HHI_SYS_CPU_CLK_CNTL0_SOUTSEL	__BITS(3,2)
#define	 HHI_SYS_CPU_CLK_CNTL0_PLLSEL	__BITS(1,0)
#define	HHI_MPLL_CNTL		CBUS_REG(0xa0)
#define	HHI_MPLL_CNTL2		CBUS_REG(0xa1)
#define	HHI_MPLL_CNTL5		CBUS_REG(0xa4)
#define	HHI_MPLL_CNTL6		CBUS_REG(0xa5)
#define	HHI_MPLL_CNTL7		CBUS_REG(0xa6)
#define	HHI_MPLL_CNTL8		CBUS_REG(0xa7)
#define	HHI_MPLL_CNTL9		CBUS_REG(0xa8)
#define	HHI_SYS_PLL_CNTL	CBUS_REG(0xc0)
#define	 HHI_SYS_PLL_CNTL_LOCK	__BIT(31)
#define	 HHI_SYS_PLL_CNTL_OD	__BITS(17,16)
#define	 HHI_SYS_PLL_CNTL_DIV	__BITS(14,9)
#define	 HHI_SYS_PLL_CNTL_MUL	__BITS(8,0)

static int meson8b_clkc_match(device_t, cfdata_t, void *);
static void meson8b_clkc_attach(device_t, device_t, void *);

static const char * const compatible[] = {
	"amlogic,meson8b-clkc",
	NULL
};

CFATTACH_DECL_NEW(meson8b_clkc, sizeof(struct meson_clk_softc),
	meson8b_clkc_match, meson8b_clkc_attach, NULL, NULL);

static struct meson_clk_reset meson8b_clkc_resets[] = {
	MESON_CLK_RESET(MESON8B_RESET_CPU0_SOFT_RESET, HHI_SYS_CPU_CLK_CNTL0, 24),
	MESON_CLK_RESET(MESON8B_RESET_CPU1_SOFT_RESET, HHI_SYS_CPU_CLK_CNTL0, 25),
	MESON_CLK_RESET(MESON8B_RESET_CPU2_SOFT_RESET, HHI_SYS_CPU_CLK_CNTL0, 26),
	MESON_CLK_RESET(MESON8B_RESET_CPU3_SOFT_RESET, HHI_SYS_CPU_CLK_CNTL0, 27),
};

static const char *mpeg_sel_parents[] = { "xtal", NULL, "fclk_div7", "mpll_clkout1", "mpll_clkout2", "fclk_div4", "fclk_div3", "fclk_div5" };
static const char *cpu_in_sel_parents[] = { "xtal", "sys_pll" };
static const char *cpu_scale_out_sel_parents[] = { "cpu_in_sel", "cpu_in_div2", "cpu_in_div3", "cpu_scale_div" };
static const char *cpu_clk_parents[] = { "xtal", "cpu_scale_out_sel" };
static const char *periph_clk_sel_parents[] = { "cpu_clk_div2", "cpu_clk_div3", "cpu_clk_div4", "cpu_clk_div5", "cpu_clk_div6", "cpu_clk_div7", "cpu_clk_div8" };

static int
meson8b_clkc_pll_sys_set_rate(struct meson_clk_softc *sc,
    struct meson_clk_clk *clk, u_int rate)
{
	struct clk *clkp, *clkp_parent;
	int error;

	KASSERT(clk->type == MESON_CLK_PLL);

	clkp = &clk->base;
	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return ENXIO;

	const u_int old_rate = clk_get_rate(clkp);
	if (old_rate == rate)
		return 0;

	const u_int parent_rate = clk_get_rate(clkp_parent);
	if (parent_rate == 0)
		return EIO;

	CLK_LOCK(sc);

	uint32_t cntl0 = CLK_READ(sc, HHI_SYS_CPU_CLK_CNTL0);
	uint32_t cntl = CLK_READ(sc, HHI_SYS_PLL_CNTL);

	u_int new_mul = rate / parent_rate;
	u_int new_div = 1;
	u_int new_od = 0;

	if (rate < 600 * 1000000) {
		new_od = 2;
		new_mul *= 4;
	} else if (rate < 1200 * 1000000) {
		new_od = 1;
		new_mul *= 2;
	}

	if ((cntl0 & HHI_SYS_CPU_CLK_CNTL0_CLKSEL) == 0) {
		error = EIO;
		goto done;
	}
	if (__SHIFTOUT(cntl0, HHI_SYS_CPU_CLK_CNTL0_PLLSEL) != 1) {
		error = EIO;
		goto done;
	}
	if (__SHIFTOUT(cntl0, HHI_SYS_CPU_CLK_CNTL0_SOUTSEL) != 0) {
		error = EIO;
		goto done;
	}

	cntl &= ~HHI_SYS_PLL_CNTL_MUL;
	cntl |= __SHIFTIN(new_mul, HHI_SYS_PLL_CNTL_MUL);
	cntl &= ~HHI_SYS_PLL_CNTL_DIV;
	cntl |= __SHIFTIN(new_div, HHI_SYS_PLL_CNTL_DIV);
	cntl &= ~HHI_SYS_PLL_CNTL_OD;
	cntl |= __SHIFTIN(new_od, HHI_SYS_PLL_CNTL_OD);

	/* Switch CPU to XTAL clock */
	cntl0 &= ~HHI_SYS_CPU_CLK_CNTL0_CLKSEL;
	CLK_WRITE(sc, HHI_SYS_CPU_CLK_CNTL0, cntl0);

	delay((100 * old_rate) / parent_rate);

	/* Update multiplier */
	do {
		CLK_WRITE(sc, HHI_SYS_PLL_CNTL, cntl);

		/* Switch CPU to sys pll */
		cntl0 |= HHI_SYS_CPU_CLK_CNTL0_CLKSEL;
		CLK_WRITE(sc, HHI_SYS_CPU_CLK_CNTL0, cntl0);
	} while ((CLK_READ(sc, HHI_SYS_PLL_CNTL) & HHI_SYS_PLL_CNTL_LOCK) == 0);

	error = 0;

done:
	CLK_UNLOCK(sc);

	return error;
}

static struct meson_clk_clk meson8b_clkc_clks[] = {

	MESON_CLK_FIXED(MESON8B_CLOCK_XTAL, "xtal", 24000000),

	MESON_CLK_PLL_RATE(MESON8B_CLOCK_PLL_SYS_DCO, "pll_sys_dco", "xtal",
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL, __BIT(30)),	/* enable */
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL, __BITS(8,0)),	/* m */
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL, __BITS(13,9)),	/* n */
	    MESON_CLK_PLL_REG_INVALID,				/* frac */
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL, __BIT(31)),	/* l */
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL, __BIT(29)),	/* reset */
	    meson8b_clkc_pll_sys_set_rate,
	    0),

	MESON_CLK_DIV(MESON8B_CLOCK_PLL_SYS, "sys_pll", "pll_sys_dco",
	    HHI_SYS_PLL_CNTL,		/* reg */
	    __BITS(17,16),		/* div */
	    MESON_CLK_DIV_POWER_OF_TWO | MESON_CLK_DIV_SET_RATE_PARENT),

	MESON_CLK_MUX(MESON8B_CLOCK_CPU_IN_SEL, "cpu_in_sel", cpu_in_sel_parents,
	    HHI_SYS_CPU_CLK_CNTL0,	/* reg */
	    __BIT(0),			/* sel */
	    0),

	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_CPU_IN_DIV2, "cpu_in_div2", "cpu_in_sel", 2, 1),
	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_CPU_IN_DIV3, "cpu_in_div3", "cpu_in_sel", 3, 1),

	MESON_CLK_DIV(MESON8B_CLOCK_CPU_SCALE_DIV, "cpu_scale_div", "cpu_in_sel",
	    HHI_SYS_CPU_CLK_CNTL1,	/* reg */
	    __BITS(29,20),		/* div */
	    MESON_CLK_DIV_CPU_SCALE_TABLE | MESON_CLK_DIV_SET_RATE_PARENT),

	MESON_CLK_MUX(MESON8B_CLOCK_CPU_SCALE_OUT_SEL, "cpu_scale_out_sel", cpu_scale_out_sel_parents,
	    HHI_SYS_CPU_CLK_CNTL0,	/* reg */
	    __BITS(3,2),		/* sel */
	    0),

	MESON_CLK_MUX(MESON8B_CLOCK_CPUCLK, "cpu_clk", cpu_clk_parents,
	    HHI_SYS_CPU_CLK_CNTL0,	/* reg */
	    __BIT(7),			/* sel */
	    0),

	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_CPU_CLK_DIV2, "cpu_clk_div2", "cpu_clk", 2, 1),
	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_CPU_CLK_DIV3, "cpu_clk_div3", "cpu_clk", 3, 1),
	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_CPU_CLK_DIV4, "cpu_clk_div4", "cpu_clk", 4, 1),
	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_CPU_CLK_DIV5, "cpu_clk_div5", "cpu_clk", 5, 1),
	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_CPU_CLK_DIV6, "cpu_clk_div6", "cpu_clk", 6, 1),
	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_CPU_CLK_DIV7, "cpu_clk_div7", "cpu_clk", 7, 1),
	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_CPU_CLK_DIV8, "cpu_clk_div8", "cpu_clk", 8, 1),

	MESON_CLK_MUX(MESON8B_CLOCK_PERIPH_SEL, "periph_clk_sel", periph_clk_sel_parents,
	    HHI_SYS_CPU_CLK_CNTL1,	/* reg */
	    __BITS(8,6),		/* sel */
	    0),
	MESON_CLK_GATE_FLAGS(MESON8B_CLOCK_PERIPH, "periph_clk_dis", "periph_clk_sel",
	    HHI_SYS_CPU_CLK_CNTL1,	/* reg */
	    17,				/* bit */
	    MESON_CLK_GATE_SET_TO_DISABLE),

	MESON_CLK_PLL(MESON8B_CLOCK_PLL_FIXED_DCO, "pll_fixed_dco", "xtal",
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL, __BIT(30)),	/* enable */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL, __BITS(8,0)),	/* m */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL, __BITS(13,9)),	/* n */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL2, __BITS(11,0)),	/* frac */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL, __BIT(31)),	/* l */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL, __BIT(29)),	/* reset */
	    0),

	MESON_CLK_DIV(MESON8B_CLOCK_PLL_FIXED, "pll_fixed", "pll_fixed_dco",
	    HHI_MPLL_CNTL,	/* reg */
	    __BITS(17,16),	/* div */
	    MESON_CLK_DIV_POWER_OF_TWO),

	MESON_CLK_DIV(MESON8B_CLOCK_MPLL_PREDIV, "mpll_prediv", "pll_fixed",
	    HHI_MPLL_CNTL5,	/* reg */
	    __BIT(12),		/* div */
	    0),

	MESON_CLK_MPLL(MESON8B_CLOCK_MPLL0_DIV, "mpll0_div", "mpll_prediv",
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL7, __BITS(13,0)),	/* sdm */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL7, __BIT(15)),	/* sdm_enable */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL7, __BITS(24,16)),	/* n2 */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL, __BIT(25)),	/* ssen */
	    0),
	MESON_CLK_MPLL(MESON8B_CLOCK_MPLL1_DIV, "mpll1_div", "mpll_prediv",
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL8, __BITS(13,0)),	/* sdm */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL8, __BIT(15)),	/* sdm_enable */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL8, __BITS(24,16)),	/* n2 */
	    MESON_CLK_PLL_REG_INVALID,				/* ssen */
	    0),
	MESON_CLK_MPLL(MESON8B_CLOCK_MPLL2_DIV, "mpll2_div", "mpll_prediv",
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL8, __BITS(13,0)),	/* sdm */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL8, __BIT(15)),	/* sdm_enable */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL8, __BITS(24,16)),	/* n2 */
	    MESON_CLK_PLL_REG_INVALID,				/* ssen */
	    0),

	MESON_CLK_GATE(MESON8B_CLOCK_MPLL0, "mpll0", "mpll0_div", HHI_MPLL_CNTL7, 14),
	MESON_CLK_GATE(MESON8B_CLOCK_MPLL1, "mpll1", "mpll1_div", HHI_MPLL_CNTL8, 14),
	MESON_CLK_GATE(MESON8B_CLOCK_MPLL2, "mpll2", "mpll2_div", HHI_MPLL_CNTL9, 14),

	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_FCLK_DIV2_DIV, "fclk_div2_div", "pll_fixed", 2, 1),
	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_FCLK_DIV3_DIV, "fclk_div3_div", "pll_fixed", 3, 1),
	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_FCLK_DIV4_DIV, "fclk_div4_div", "pll_fixed", 4, 1),
	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_FCLK_DIV5_DIV, "fclk_div5_div", "pll_fixed", 5, 1),
	MESON_CLK_FIXED_FACTOR(MESON8B_CLOCK_FCLK_DIV7_DIV, "fclk_div7_div", "pll_fixed", 7, 1),

	MESON_CLK_GATE(MESON8B_CLOCK_FCLK_DIV2, "fclk_div2", "fclk_div2_div", HHI_MPLL_CNTL6, 27),
	MESON_CLK_GATE(MESON8B_CLOCK_FCLK_DIV3, "fclk_div3", "fclk_div3_div", HHI_MPLL_CNTL6, 28),
	MESON_CLK_GATE(MESON8B_CLOCK_FCLK_DIV4, "fclk_div4", "fclk_div4_div", HHI_MPLL_CNTL6, 29),
	MESON_CLK_GATE(MESON8B_CLOCK_FCLK_DIV5, "fclk_div5", "fclk_div5_div", HHI_MPLL_CNTL6, 30),
	MESON_CLK_GATE(MESON8B_CLOCK_FCLK_DIV7, "fclk_div7", "fclk_div7_div", HHI_MPLL_CNTL6, 31),

	MESON_CLK_MUX(MESON8B_CLOCK_MPEG_SEL, "mpeg_sel", mpeg_sel_parents,
	    HHI_MPEG_CLK_CNTL,	/* reg */
	    __BITS(14,12),	/* sel */
	    0),

	MESON_CLK_DIV(MESON8B_CLOCK_MPEG_DIV, "mpeg_div", "mpeg_sel",
	    HHI_MPEG_CLK_CNTL,	/* reg */
	    __BITS(6,0),	/* div */
	    0),

	MESON_CLK_GATE(MESON8B_CLOCK_CLK81, "clk81", "mpeg_div", HHI_MPEG_CLK_CNTL, 7),

	MESON_CLK_GATE(MESON8B_CLOCK_I2C, "i2c", "clk81", HHI_GCLK_MPEG0, 9),
	MESON_CLK_GATE(MESON8B_CLOCK_SAR_ADC, "sar_adc", "clk81", HHI_GCLK_MPEG0, 10),
	MESON_CLK_GATE(MESON8B_CLOCK_RNG0, "rng0", "clk81", HHI_GCLK_MPEG0, 12),
	MESON_CLK_GATE(MESON8B_CLOCK_UART0, "uart0", "clk81", HHI_GCLK_MPEG0, 13),
	MESON_CLK_GATE(MESON8B_CLOCK_SDHC, "sdhc", "clk81", HHI_GCLK_MPEG0, 14),
	MESON_CLK_GATE(MESON8B_CLOCK_SDIO, "sdio", "clk81", HHI_GCLK_MPEG0, 17),

	MESON_CLK_GATE(MESON8B_CLOCK_ETH, "eth", "clk81", HHI_GCLK_MPEG1, 3),
	MESON_CLK_GATE(MESON8B_CLOCK_UART1, "uart1", "clk81", HHI_GCLK_MPEG1, 16),
	MESON_CLK_GATE(MESON8B_CLOCK_USB0, "usb0", "clk81", HHI_GCLK_MPEG1, 21),
	MESON_CLK_GATE(MESON8B_CLOCK_USB1, "usb1", "clk81", HHI_GCLK_MPEG1, 22),
	MESON_CLK_GATE(MESON8B_CLOCK_USB, "usb", "clk81", HHI_GCLK_MPEG1, 26),
	MESON_CLK_GATE(MESON8B_CLOCK_EFUSE, "efuse", "clk81", HHI_GCLK_MPEG1, 30),

	MESON_CLK_GATE(MESON8B_CLOCK_USB1_DDR_BRIDGE, "usb1_ddr_bridge", "clk81", HHI_GCLK_MPEG2, 8),
	MESON_CLK_GATE(MESON8B_CLOCK_USB0_DDR_BRIDGE, "usb0_ddr_bridge", "clk81", HHI_GCLK_MPEG2, 9),
	MESON_CLK_GATE(MESON8B_CLOCK_UART2, "uart2", "clk81", HHI_GCLK_MPEG2, 15),
};

static int
meson8b_clkc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
meson8b_clkc_attach(device_t parent, device_t self, void *aux)
{
	struct meson_clk_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;
	if (fdtbus_get_reg(sc->sc_phandle, MESON8B_CLKC_REG_INDEX, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_resets = meson8b_clkc_resets;
	sc->sc_nresets = __arraycount(meson8b_clkc_resets);

	sc->sc_clks = meson8b_clkc_clks;
	sc->sc_nclks = __arraycount(meson8b_clkc_clks);

	meson_clk_attach(sc);

	aprint_naive("\n");
	aprint_normal(": Meson8b clock controller\n");

	meson_clk_print(sc);
}
