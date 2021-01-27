/* $NetBSD: mesongxbb_clkc.c,v 1.6 2021/01/27 03:10:18 thorpej Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: mesongxbb_clkc.c,v 1.6 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/amlogic/meson_clk.h>
#include <arm/amlogic/mesongxbb_clkc.h>

#define	CBUS_REG(x)	((x) << 2)

#define	HHI_GCLK_MPEG0		CBUS_REG(0x50)
#define	HHI_GCLK_MPEG1		CBUS_REG(0x51)
#define	HHI_GCLK_MPEG2		CBUS_REG(0x52)
#define	HHI_GCLK_OTHER		CBUS_REG(0x54)
#define	HHI_SYS_CPU_CLK_CNTL1	CBUS_REG(0x57)
#define	HHI_MPEG_CLK_CNTL	CBUS_REG(0x5d)
#define	HHI_NAND_CLK_CNTL	CBUS_REG(0x97)
#define	HHI_SD_EMMC_CLK_CNTL	CBUS_REG(0x99)
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

static int mesongxbb_clkc_match(device_t, cfdata_t, void *);
static void mesongxbb_clkc_attach(device_t, device_t, void *);

struct mesongxbb_clkc_config {
	const char *name;
};

static const struct mesongxbb_clkc_config gxbb_config = {
	.name = "Meson GXBB",
};

static const struct mesongxbb_clkc_config gxl_config = {
	.name = "Meson GXL",
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,gxbb-clkc",	.data = &gxbb_config },
	{ .compat = "amlogic,gxl-clkc",		.data = &gxl_config },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(mesongxbb_clkc, sizeof(struct meson_clk_softc),
	mesongxbb_clkc_match, mesongxbb_clkc_attach, NULL, NULL);

static const char *mpeg_sel_parents[] = { "xtal", NULL, "fclk_div7", "mpll1", "mpll2", "fclk_div4", "fclk_div3", "fclk_div5" };
static const char *sd_emmc_clk0_sel_parents[] = { "xtal", "fclk_div2", "fclk_div3", "fclk_div5", "fclk_div7" };

static struct meson_clk_clk mesongxbb_clkc_clks[] = {

	MESON_CLK_PLL(MESONGXBB_CLOCK_SYS_PLL_DCO, "pll_sys_dco", "xtal",
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL, __BIT(30)),	/* enable */
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL, __BITS(8,0)),	/* m */
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL, __BITS(13,9)),	/* n */
	    MESON_CLK_PLL_REG_INVALID,				/* frac */
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL, __BIT(31)),	/* l */
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL, __BIT(29)),	/* reset */
	    0),

	MESON_CLK_DIV(MESONGXBB_CLOCK_SYS_PLL, "sys_pll", "pll_sys_dco",
	    HHI_SYS_PLL_CNTL,		/* reg */
	    __BITS(17,16),		/* div */
	    MESON_CLK_DIV_POWER_OF_TWO | MESON_CLK_DIV_SET_RATE_PARENT),

	MESON_CLK_PLL(MESONGXBB_CLOCK_FIXED_PLL_DCO, "pll_fixed_dco", "xtal",
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL, __BIT(30)),	/* enable */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL, __BITS(8,0)),	/* m */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL, __BITS(13,9)),	/* n */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL2, __BITS(11,0)),	/* frac */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL, __BIT(31)),	/* l */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL, __BIT(29)),	/* reset */
	    0),

	MESON_CLK_DIV(MESONGXBB_CLOCK_FIXED_PLL, "pll_fixed", "pll_fixed_dco",
	    HHI_MPLL_CNTL,	/* reg */
	    __BITS(17,16),	/* div */
	    MESON_CLK_DIV_POWER_OF_TWO),

	MESON_CLK_DIV(MESONGXBB_CLOCK_MPLL_PREDIV, "mpll_prediv", "pll_fixed",
	    HHI_MPLL_CNTL5,	/* reg */
	    __BIT(12),		/* div */
	    0),

	MESON_CLK_MPLL(MESONGXBB_CLOCK_MPLL0_DIV, "mpll0_div", "mpll_prediv",
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL7, __BITS(13,0)),	/* sdm */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL7, __BIT(15)),	/* sdm_enable */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL7, __BITS(24,16)),	/* n2 */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL, __BIT(25)),	/* ssen */
	    0),
	MESON_CLK_MPLL(MESONGXBB_CLOCK_MPLL1_DIV, "mpll1_div", "mpll_prediv",
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL8, __BITS(13,0)),	/* sdm */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL8, __BIT(15)),	/* sdm_enable */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL8, __BITS(24,16)),	/* n2 */
	    MESON_CLK_PLL_REG_INVALID,				/* ssen */
	    0),
	MESON_CLK_MPLL(MESONGXBB_CLOCK_MPLL2_DIV, "mpll2_div", "mpll_prediv",
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL8, __BITS(13,0)),	/* sdm */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL8, __BIT(15)),	/* sdm_enable */
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL8, __BITS(24,16)),	/* n2 */
	    MESON_CLK_PLL_REG_INVALID,				/* ssen */
	    0),

	MESON_CLK_GATE(MESONGXBB_CLOCK_MPLL0, "mpll0", "mpll0_div", HHI_MPLL_CNTL7, 14),
	MESON_CLK_GATE(MESONGXBB_CLOCK_MPLL1, "mpll1", "mpll1_div", HHI_MPLL_CNTL8, 14),
	MESON_CLK_GATE(MESONGXBB_CLOCK_MPLL2, "mpll2", "mpll2_div", HHI_MPLL_CNTL9, 14),

	MESON_CLK_FIXED_FACTOR(MESONGXBB_CLOCK_FCLK_DIV2_DIV, "fclk_div2_div", "pll_fixed", 2, 1),
	MESON_CLK_FIXED_FACTOR(MESONGXBB_CLOCK_FCLK_DIV3_DIV, "fclk_div3_div", "pll_fixed", 3, 1),
	MESON_CLK_FIXED_FACTOR(MESONGXBB_CLOCK_FCLK_DIV4_DIV, "fclk_div4_div", "pll_fixed", 4, 1),
	MESON_CLK_FIXED_FACTOR(MESONGXBB_CLOCK_FCLK_DIV5_DIV, "fclk_div5_div", "pll_fixed", 5, 1),
	MESON_CLK_FIXED_FACTOR(MESONGXBB_CLOCK_FCLK_DIV7_DIV, "fclk_div7_div", "pll_fixed", 7, 1),

	MESON_CLK_GATE(MESONGXBB_CLOCK_FCLK_DIV2, "fclk_div2", "fclk_div2_div", HHI_MPLL_CNTL6, 27),
	MESON_CLK_GATE(MESONGXBB_CLOCK_FCLK_DIV3, "fclk_div3", "fclk_div3_div", HHI_MPLL_CNTL6, 28),
	MESON_CLK_GATE(MESONGXBB_CLOCK_FCLK_DIV4, "fclk_div4", "fclk_div4_div", HHI_MPLL_CNTL6, 29),
	MESON_CLK_GATE(MESONGXBB_CLOCK_FCLK_DIV5, "fclk_div5", "fclk_div5_div", HHI_MPLL_CNTL6, 30),
	MESON_CLK_GATE(MESONGXBB_CLOCK_FCLK_DIV7, "fclk_div7", "fclk_div7_div", HHI_MPLL_CNTL6, 31),
	MESON_CLK_MUX(MESONGXBB_CLOCK_MPEG_SEL, "mpeg_sel", mpeg_sel_parents,
	    HHI_MPEG_CLK_CNTL,	/* reg */
	    __BITS(14,12),	/* sel */
	    0),

	MESON_CLK_DIV(MESONGXBB_CLOCK_MPEG_DIV, "mpeg_div", "mpeg_sel",
	    HHI_MPEG_CLK_CNTL,	/* reg */
	    __BITS(6,0),	/* div */
	    0),

	MESON_CLK_MUX(MESONGXBB_CLOCK_SD_EMMC_A_CLK0_SEL, "sd_emmc_a_clk0_sel", sd_emmc_clk0_sel_parents,
	    HHI_SD_EMMC_CLK_CNTL,	/* reg */
	    __BITS(11,9),		/* sel */
	    0),
	MESON_CLK_MUX(MESONGXBB_CLOCK_SD_EMMC_B_CLK0_SEL, "sd_emmc_b_clk0_sel", sd_emmc_clk0_sel_parents,
	    HHI_SD_EMMC_CLK_CNTL,	/* reg */
	    __BITS(27,25),		/* sel */
	    0),
	MESON_CLK_MUX(MESONGXBB_CLOCK_SD_EMMC_C_CLK0_SEL, "sd_emmc_c_clk0_sel", sd_emmc_clk0_sel_parents,
	    HHI_NAND_CLK_CNTL,		/* reg */
	    __BITS(11,9),		/* sel */
	    0),

	MESON_CLK_DIV(MESONGXBB_CLOCK_SD_EMMC_A_CLK0_DIV, "sd_emmc_a_clk0_div", "sd_emmc_a_clk0_sel",
	    HHI_SD_EMMC_CLK_CNTL,	/* reg */
	    __BITS(6,0),		/* div */
	    0),
	MESON_CLK_DIV(MESONGXBB_CLOCK_SD_EMMC_B_CLK0_DIV, "sd_emmc_b_clk0_div", "sd_emmc_b_clk0_sel",
	    HHI_SD_EMMC_CLK_CNTL,	/* reg */
	    __BITS(22,16),		/* div */
	    0),
	MESON_CLK_DIV(MESONGXBB_CLOCK_SD_EMMC_C_CLK0_DIV, "sd_emmc_c_clk0_div", "sd_emmc_c_clk0_sel",
	    HHI_NAND_CLK_CNTL,		/* reg */
	    __BITS(6,0),		/* div */
	    0),

	MESON_CLK_GATE(MESONGXBB_CLOCK_SD_EMMC_A_CLK0, "sd_emmc_a_clk0", "sd_emmc_a_clk0_div", HHI_SD_EMMC_CLK_CNTL, 7),
	MESON_CLK_GATE(MESONGXBB_CLOCK_SD_EMMC_B_CLK0, "sd_emmc_b_clk0", "sd_emmc_b_clk0_div", HHI_SD_EMMC_CLK_CNTL, 23),
	MESON_CLK_GATE(MESONGXBB_CLOCK_SD_EMMC_C_CLK0, "sd_emmc_c_clk0", "sd_emmc_c_clk0_div", HHI_NAND_CLK_CNTL, 7),

	MESON_CLK_GATE(MESONGXBB_CLOCK_CLK81, "clk81", "mpeg_div", HHI_MPEG_CLK_CNTL, 7),

	MESON_CLK_GATE(MESONGXBB_CLOCK_I2C, "i2c", "clk81", HHI_GCLK_MPEG0, 9),
	MESON_CLK_GATE(MESONGXBB_CLOCK_SAR_ADC, "sar_adc", "clk81", HHI_GCLK_MPEG0, 10),
	MESON_CLK_GATE(MESONGXBB_CLOCK_RNG0, "rng0", "clk81", HHI_GCLK_MPEG0, 12),
	MESON_CLK_GATE(MESONGXBB_CLOCK_UART0, "uart0", "clk81", HHI_GCLK_MPEG0, 13),
	MESON_CLK_GATE(MESONGXBB_CLOCK_SDHC, "sdhc", "clk81", HHI_GCLK_MPEG0, 14),
	MESON_CLK_GATE(MESONGXBB_CLOCK_SDIO, "sdio", "clk81", HHI_GCLK_MPEG0, 17),
	MESON_CLK_GATE(MESONGXBB_CLOCK_SD_EMMC_A, "sd_emmc_a", "clk81", HHI_GCLK_MPEG0, 24),
	MESON_CLK_GATE(MESONGXBB_CLOCK_SD_EMMC_B, "sd_emmc_b", "clk81", HHI_GCLK_MPEG0, 25),
	MESON_CLK_GATE(MESONGXBB_CLOCK_SD_EMMC_C, "sd_emmc_c", "clk81", HHI_GCLK_MPEG0, 26),

	MESON_CLK_GATE(MESONGXBB_CLOCK_ETH, "eth", "clk81", HHI_GCLK_MPEG1, 3),
	MESON_CLK_GATE(MESONGXBB_CLOCK_UART1, "uart1", "clk81", HHI_GCLK_MPEG1, 16),
	MESON_CLK_GATE(MESONGXBB_CLOCK_USB0, "usb0", "clk81", HHI_GCLK_MPEG1, 21),
	MESON_CLK_GATE(MESONGXBB_CLOCK_USB1, "usb1", "clk81", HHI_GCLK_MPEG1, 22),
	MESON_CLK_GATE(MESONGXBB_CLOCK_USB, "usb", "clk81", HHI_GCLK_MPEG1, 26),
	MESON_CLK_GATE(MESONGXBB_CLOCK_EFUSE, "efuse", "clk81", HHI_GCLK_MPEG1, 30),

	MESON_CLK_GATE(MESONGXBB_CLOCK_USB1_DDR_BRIDGE, "usb1_ddr_bridge", "clk81", HHI_GCLK_MPEG2, 8),
	MESON_CLK_GATE(MESONGXBB_CLOCK_USB0_DDR_BRIDGE, "usb0_ddr_bridge", "clk81", HHI_GCLK_MPEG2, 9),
	MESON_CLK_GATE(MESONGXBB_CLOCK_UART2, "uart2", "clk81", HHI_GCLK_MPEG2, 15),
};

static int
mesongxbb_clkc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
mesongxbb_clkc_attach(device_t parent, device_t self, void *aux)
{
	struct meson_clk_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const struct mesongxbb_clkc_config *conf;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_syscon = fdtbus_syscon_lookup(OF_parent(sc->sc_phandle));
	if (sc->sc_syscon == NULL) {
		aprint_error(": couldn't get syscon registers\n");
		return;
	}

	sc->sc_clks = mesongxbb_clkc_clks;
	sc->sc_nclks = __arraycount(mesongxbb_clkc_clks);

	meson_clk_attach(sc);

	conf = of_compatible_lookup(phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": %s clock controller\n", conf->name);

	meson_clk_print(sc);
}
