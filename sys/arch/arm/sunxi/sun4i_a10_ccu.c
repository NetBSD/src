/* $NetBSD: sun4i_a10_ccu.c,v 1.6.2.2 2018/04/07 04:12:12 pgoyette Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: sun4i_a10_ccu.c,v 1.6.2.2 2018/04/07 04:12:12 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sun4i_a10_ccu.h>
#include <arm/sunxi/sun7i_a20_ccu.h>

#define	PLL1_CFG_REG		0x000
#define	PLL2_CFG_REG		0x008
#define	PLL3_CFG_REG		0x010
#define	PLL5_CFG_REG		0x020
#define	PLL6_CFG_REG		0x028
#define	PLL7_CFG_REG		0x030
#define	OSC24M_CFG_REG		0x050
#define	CPU_AHB_APB0_CFG_REG	0x054
#define	APB1_CLK_DIV_REG	0x058
#define	AHB_GATING_REG0		0x060
#define	AHB_GATING_REG1		0x064
#define	APB0_GATING_REG		0x068
#define	APB1_GATING_REG		0x06c
#define	NAND_SCLK_CFG_REG	0x080
#define	SD0_SCLK_CFG_REG        0x088
#define	SD1_SCLK_CFG_REG        0x08c
#define	SD2_SCLK_CFG_REG        0x090
#define	SD3_SCLK_CFG_REG	0x094
#define	SATA_CFG_REG		0x0c8
#define	USBPHY_CFG_REG		0x0cc
#define	DRAM_GATING_REG		0x100
#define	BE0_CFG_REG		0x104
#define	BE1_CFG_REG		0x108
#define	FE0_CFG_REG		0x10c
#define	FE1_CFG_REG		0x110
#define	MP_CFG_REG		0x114
#define	LCD0CH0_CFG_REG		0x118
#define	LCD1CH0_CFG_REG		0x11c
#define LCD0CH1_CFG_REG		0x12c
#define LCD1CH1_CFG_REG		0x130
#define	CSI_CFG_REG		0x134
#define	VE_CFG_REG		0x13c
#define	AUDIO_CODEC_SCLK_CFG_REG 0x140
#define	LVDS_CFG_REG 		0x14c
#define	HDMI_CLOCK_CFG_REG	0x150
#define	MALI_CLOCK_CFG_REG	0x154
#define	IEP_SCLK_CFG_REG	0x160

static int sun4i_a10_ccu_match(device_t, cfdata_t, void *);
static void sun4i_a10_ccu_attach(device_t, device_t, void *);

enum sun4i_a10_ccu_type {
	CCU_A10 = 1,
	CCU_A20,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun4i-a10-ccu",	CCU_A10 },
	{ "allwinner,sun7i-a20-ccu",	CCU_A20 },
	{ NULL }
};

CFATTACH_DECL_NEW(sunxi_a10_ccu, sizeof(struct sunxi_ccu_softc),
	sun4i_a10_ccu_match, sun4i_a10_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun4i_a10_ccu_resets[] = {
	SUNXI_CCU_RESET(A10_RST_USB_PHY0, USBPHY_CFG_REG, 0),
	SUNXI_CCU_RESET(A10_RST_USB_PHY1, USBPHY_CFG_REG, 1),
	SUNXI_CCU_RESET(A10_RST_USB_PHY2, USBPHY_CFG_REG, 2),
	SUNXI_CCU_RESET(A10_RST_DE_BE0, BE0_CFG_REG, 30),
	SUNXI_CCU_RESET(A10_RST_DE_BE1, BE1_CFG_REG, 30),
	SUNXI_CCU_RESET(A10_RST_DE_FE0, FE0_CFG_REG, 30),
	SUNXI_CCU_RESET(A10_RST_DE_FE1, FE1_CFG_REG, 30),
	SUNXI_CCU_RESET(A10_RST_DE_MP, MP_CFG_REG, 30),
	SUNXI_CCU_RESET(A10_RST_TCON0, LCD0CH0_CFG_REG, 30),
	SUNXI_CCU_RESET(A10_RST_TCON1, LCD1CH0_CFG_REG, 30),
	SUNXI_CCU_RESET(A10_RST_LVDS, LVDS_CFG_REG, 0),
};

static const char *cpu_parents[] = { "losc", "osc24m", "pll_core", "pll_periph" };
static const char *axi_parents[] = { "cpu" };
static const char *ahb_parents[] = { "axi", "pll_periph", "pll_periph_base" };
static const char *apb0_parents[] = { "ahb" };
static const char *apb1_parents[] = { "osc24m", "pll_periph", "losc" };
static const char *mod_parents[] = { "osc24m", "pll_periph", "pll_ddr_other" };
static const char *sata_parents[] = { "pll6_periph_sata", "external" };
static const char *de_parents[] = { "pll_video0", "pll_video1", "pll_ddr_other" };
static const char *lcd_parents[] = { "pll_video0", "pll_video1", "pll_video0x2", "pll_video1x2" };

static const struct sunxi_ccu_nkmp_tbl sun4i_a10_pll1_table[] = {
	{ 1008000000, 21, 1, 0, 0 },
	{  960000000, 20, 1, 0, 0 },
	{  912000000, 19, 1, 0, 0 },
	{  864000000, 18, 1, 0, 0 },
	{  720000000, 30, 0, 0, 0 },
	{  624000000, 26, 0, 0, 0 },
	{  528000000, 22, 0, 0, 0 },
	{  312000000, 13, 0, 0, 0 },
	{  144000000, 12, 0, 0, 1 },
	{          0 }
};

static const struct sunxi_ccu_nkmp_tbl sun4i_a10_ac_dig_table[] = {
	{ 24576000, 86, 0, 21, 4 },
	{ 0 }
};

/*
 * some special cases
 * hardcode lcd0 (tcon0) to pll3 and lcd1 (tcon1) to pll7.
 * compute pll rate based on desired pixel clock
 */

static int sun4i_a10_ccu_lcd0ch0_set_rate(struct sunxi_ccu_softc *,
    struct sunxi_ccu_clk *, u_int);
static int sun4i_a10_ccu_lcd1ch0_set_rate(struct sunxi_ccu_softc *,
    struct sunxi_ccu_clk *, u_int);
static u_int sun4i_a10_ccu_lcd0ch0_round_rate(struct sunxi_ccu_softc *,
    struct sunxi_ccu_clk *, u_int);
static u_int sun4i_a10_ccu_lcd1ch0_round_rate(struct sunxi_ccu_softc *,
    struct sunxi_ccu_clk *, u_int);
static int sun4i_a10_ccu_lcd0ch1_set_rate(struct sunxi_ccu_softc *,
    struct sunxi_ccu_clk *, u_int);
static int sun4i_a10_ccu_lcd1ch1_set_rate(struct sunxi_ccu_softc *,
    struct sunxi_ccu_clk *, u_int);

static struct sunxi_ccu_clk sun4i_a10_ccu_clks[] = {
	SUNXI_CCU_GATE(A10_CLK_HOSC, "osc24m", "hosc",
	    OSC24M_CFG_REG, 0),

	SUNXI_CCU_NKMP_TABLE(A10_CLK_PLL_CORE, "pll_core", "osc24m",
	    PLL1_CFG_REG,		/* reg */
	    __BITS(12,8),		/* n */
	    __BITS(5,4), 		/* k */
	    __BITS(1,0),		/* m */
	    __BITS(17,16),		/* p */
	    __BIT(31),			/* enable */
	    0,				/* lock */
	    sun4i_a10_pll1_table,	/* table */
	    SUNXI_CCU_NKMP_FACTOR_P_POW2 | SUNXI_CCU_NKMP_FACTOR_N_EXACT |
	    SUNXI_CCU_NKMP_FACTOR_N_ZERO_IS_ONE | SUNXI_CCU_NKMP_SCALE_CLOCK),

	SUNXI_CCU_NKMP_TABLE(A10_CLK_PLL_AUDIO_BASE, "pll_audio", "osc24m",
	    PLL2_CFG_REG,		/* reg */
	    __BITS(14,8),		/* n */
	    0,				/* k */
	    __BITS(4,0),		/* m */
	    __BITS(29,26),		/* p */
	    __BIT(31),			/* enable */
	    0,				/* lock */
	    sun4i_a10_ac_dig_table,	/* table */
	    0),

	SUNXI_CCU_NKMP(A10_CLK_PLL_PERIPH_BASE, "pll_periph_base", "osc24m",
	    PLL6_CFG_REG,		/* reg */
	    __BITS(12,8),		/* n */
	    __BITS(5,4), 		/* k */
	    0,				/* m */
	    0,				/* p */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NKMP_FACTOR_N_EXACT),

	SUNXI_CCU_FIXED_FACTOR(A10_CLK_PLL_PERIPH, "pll_periph", "pll_periph_base",
	    2, 1),

	SUNXI_CCU_NKMP(A10_CLK_PLL_PERIPH_SATA, "pll_periph_sata", "pll_periph_base",
	    PLL6_CFG_REG,		/* reg */
	    0,				/* n */
	    0,				/* k */
	    __BITS(1,0),		/* m */
	    0,				/* p */
	    __BIT(14),			/* enable */
	    0),

	SUNXI_CCU_DIV_GATE(A10_CLK_SATA, "sata", sata_parents,
	    SATA_CFG_REG,		/* reg */
	    0,				/* div */
	    __BIT(24),			/* sel */
	    __BIT(31),			/* enable */
	    0),

	SUNXI_CCU_NKMP(A10_CLK_PLL_DDR_BASE, "pll_ddr_other", "osc24m",
	    PLL5_CFG_REG,		/* reg */
	    __BITS(12, 8),		/* n */
	    __BITS(5,4),		/* k */
	    0,				/* m */
	    __BITS(17,16),		/* p */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NKMP_FACTOR_N_EXACT | SUNXI_CCU_NKMP_FACTOR_P_POW2),

	SUNXI_CCU_NKMP(A10_CLK_PLL_DDR, "pll_ddr", "osc24m",
	    PLL5_CFG_REG,		/* reg */
	    __BITS(12, 8),		/* n */
	    __BITS(5,4),		/* k */
	    __BITS(1,0),		/* m */
	    0,				/* p */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NKMP_FACTOR_N_EXACT),

	SUNXI_CCU_DIV(A10_CLK_CPU, "cpu", cpu_parents,
	    CPU_AHB_APB0_CFG_REG,	/* reg */
	    0,				/* div */
	    __BITS(17,16),		/* sel */
	    SUNXI_CCU_DIV_SET_RATE_PARENT),

	SUNXI_CCU_DIV(A10_CLK_AXI, "axi", axi_parents,
	    CPU_AHB_APB0_CFG_REG,	/* reg */
	    __BITS(1,0),		/* div */
	    0,				/* sel */
	    0),

	SUNXI_CCU_DIV(A10_CLK_AHB, "ahb", ahb_parents,
	    CPU_AHB_APB0_CFG_REG,	/* reg */
	    __BITS(5,4),		/* div */
	    __BITS(7,6),		/* sel */
	    SUNXI_CCU_DIV_POWER_OF_TWO),

	SUNXI_CCU_DIV(A10_CLK_APB0, "apb0", apb0_parents,
	    CPU_AHB_APB0_CFG_REG,	/* reg */
	    __BITS(9,8),		/* div */
	    0,				/* sel */
	    SUNXI_CCU_DIV_ZERO_IS_ONE | SUNXI_CCU_DIV_POWER_OF_TWO),

	SUNXI_CCU_NM(A10_CLK_APB1, "apb1", apb1_parents,
	    APB1_CLK_DIV_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(4,0),		/* m */
	    __BITS(25,24),		/* sel */
	    0,				/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(A10_CLK_NAND, "nand", mod_parents,
	    NAND_SCLK_CFG_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(A10_CLK_MMC0, "mmc0", mod_parents,
	    SD0_SCLK_CFG_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),
	SUNXI_CCU_PHASE(A10_CLK_MMC0_SAMPLE, "mmc0_sample", "mmc0",
	    SD0_SCLK_CFG_REG, __BITS(22,20)),
	SUNXI_CCU_PHASE(A10_CLK_MMC0_OUTPUT, "mmc0_output", "mmc0",
	    SD0_SCLK_CFG_REG, __BITS(10,8)),
	SUNXI_CCU_NM(A10_CLK_MMC1, "mmc1", mod_parents,
	    SD1_SCLK_CFG_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),
	SUNXI_CCU_PHASE(A10_CLK_MMC1_SAMPLE, "mmc1_sample", "mmc1",
	    SD1_SCLK_CFG_REG, __BITS(22,20)),
	SUNXI_CCU_PHASE(A10_CLK_MMC1_OUTPUT, "mmc1_output", "mmc1",
	    SD1_SCLK_CFG_REG, __BITS(10,8)),
	SUNXI_CCU_NM(A10_CLK_MMC2, "mmc2", mod_parents,
	    SD2_SCLK_CFG_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),
	SUNXI_CCU_PHASE(A10_CLK_MMC2_SAMPLE, "mmc2_sample", "mmc2",
	    SD2_SCLK_CFG_REG, __BITS(22,20)),
	SUNXI_CCU_PHASE(A10_CLK_MMC2_OUTPUT, "mmc2_output", "mmc2",
	    SD2_SCLK_CFG_REG, __BITS(10,8)),
	SUNXI_CCU_NM(A10_CLK_MMC3, "mmc3", mod_parents,
	    SD3_SCLK_CFG_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),
	SUNXI_CCU_PHASE(A10_CLK_MMC3_SAMPLE, "mmc3_sample", "mmc3",
	    SD3_SCLK_CFG_REG, __BITS(22,20)),
	SUNXI_CCU_PHASE(A10_CLK_MMC3_OUTPUT, "mmc3_output", "mmc3",
	    SD3_SCLK_CFG_REG, __BITS(10,8)),

	SUNXI_CCU_FRACTIONAL(A10_CLK_PLL_VIDEO0, "pll_video0", "osc24m",
	    PLL3_CFG_REG,		/* reg */
	    __BITS(7,0),		/* m */
	    9,				/* m_min */
	    127,			/* m_max */
	    __BIT(15),			/* div_en */
	    __BIT(14),			/* frac_sel */
	    270000000, 297000000,	/* frac values */
	    8,				/* prediv */
	    __BIT(31)			/* enable */
	    ),
	SUNXI_CCU_FRACTIONAL(A10_CLK_PLL_VIDEO1, "pll_video1", "osc24m",
	    PLL7_CFG_REG,		/* reg */
	    __BITS(7,0),		/* m */
	    9,				/* m_min */
	    127,			/* m_max */
	    __BIT(15),			/* div_en */
	    __BIT(14),			/* frac_sel */
	    270000000, 297000000,	/* frac values */
	    8,				/* prediv */
	    __BIT(31)			/* enable */
	    ),
	SUNXI_CCU_FIXED_FACTOR(A10_CLK_PLL_VIDEO0_2X,
	    "pll_video0x2", "pll_video0",
	    1, 2),
	SUNXI_CCU_FIXED_FACTOR(A10_CLK_PLL_VIDEO1_2X,
	    "pll_video1x2", "pll_video1",
	    1, 2),

	SUNXI_CCU_DIV_GATE(A10_CLK_DE_BE0, "debe0-mod", de_parents,
	    BE0_CFG_REG,		/* reg */
	    __BITS(3,0),		/* div */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    0				/* flags */
	    ),
	SUNXI_CCU_DIV_GATE(A10_CLK_DE_BE1, "debe1-mod", de_parents,
	    BE1_CFG_REG,		/* reg */
	    __BITS(3,0),		/* div */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    0				/* flags */
	    ),
	SUNXI_CCU_DIV_GATE(A10_CLK_DE_FE0, "defe0-mod", de_parents,
	    FE0_CFG_REG,		/* reg */
	    __BITS(3,0),		/* div */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    0				/* flags */
	    ),
	SUNXI_CCU_DIV_GATE(A10_CLK_DE_FE1, "defe1-mod", de_parents,
	    FE1_CFG_REG,		/* reg */
	    __BITS(3,0),		/* div */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    0				/* flags */
	    ),
	[A10_CLK_TCON0_CH0] = {
	    .type = SUNXI_CCU_DIV,
	    .base.name = "tcon0-ch0",
	    .u.div.reg = LCD0CH0_CFG_REG,
	    .u.div.parents = lcd_parents,
	    .u.div.nparents = __arraycount(lcd_parents),
	    .u.div.div = 0,
	    .u.div.sel = __BITS(25,24),
	    .u.div.enable = __BIT(31),
	    .u.div.flags = 0,
	    .enable = sunxi_ccu_div_enable,
	    .get_rate = sunxi_ccu_div_get_rate,
	    .set_rate = sun4i_a10_ccu_lcd0ch0_set_rate,
	    .round_rate = sun4i_a10_ccu_lcd0ch0_round_rate,
	    .set_parent = sunxi_ccu_div_set_parent,
	    .get_parent = sunxi_ccu_div_get_parent,
	    },
	[A10_CLK_TCON1_CH0] = {
	    .type = SUNXI_CCU_DIV,
	    .base.name = "tcon1-ch0",
	    .u.div.reg = LCD1CH0_CFG_REG,
	    .u.div.parents = lcd_parents,
	    .u.div.nparents = __arraycount(lcd_parents),
	    .u.div.div = 0,
	    .u.div.sel = __BITS(25,24),
	    .u.div.enable = __BIT(31),
	    .u.div.flags = 0,
	    .enable = sunxi_ccu_div_enable,
	    .get_rate = sunxi_ccu_div_get_rate,
	    .set_rate = sun4i_a10_ccu_lcd1ch0_set_rate,
	    .round_rate = sun4i_a10_ccu_lcd1ch0_round_rate,
	    .set_parent = sunxi_ccu_div_set_parent,
	    .get_parent = sunxi_ccu_div_get_parent,
	    },
	[A10_CLK_TCON0_CH1] = {
	    .type = SUNXI_CCU_DIV,
	    .base.name = "tcon0-ch1",
	    .u.div.reg = LCD0CH1_CFG_REG,
	    .u.div.parents = lcd_parents,
	    .u.div.nparents = __arraycount(lcd_parents),
	    .u.div.div = __BITS(3,0),
	    .u.div.sel = __BITS(25,24),
	    .u.div.enable = __BIT(15) | __BIT(31),
	    .u.div.flags = 0,
	    .enable = sunxi_ccu_div_enable,
	    .get_rate = sunxi_ccu_div_get_rate,
	    .set_rate = sun4i_a10_ccu_lcd0ch1_set_rate,
	    .set_parent = sunxi_ccu_div_set_parent,
	    .get_parent = sunxi_ccu_div_get_parent,
	    },
	[A10_CLK_TCON1_CH1] = {
	    .type = SUNXI_CCU_DIV,
	    .base.name = "tcon1-ch1",
	    .u.div.reg = LCD1CH1_CFG_REG,
	    .u.div.parents = lcd_parents,
	    .u.div.nparents = __arraycount(lcd_parents),
	    .u.div.div = __BITS(3,0),
	    .u.div.sel = __BITS(25,24),
	    .u.div.enable = __BIT(15) | __BIT(31),
	    .u.div.flags = 0,
	    .enable = sunxi_ccu_div_enable,
	    .get_rate = sunxi_ccu_div_get_rate,
	    .set_rate = sun4i_a10_ccu_lcd1ch1_set_rate,
	    .set_parent = sunxi_ccu_div_set_parent,
	    .get_parent = sunxi_ccu_div_get_parent,
	    },
	SUNXI_CCU_DIV_GATE(A10_CLK_HDMI, "hdmi-mod", lcd_parents,
	    HDMI_CLOCK_CFG_REG,		/* reg */
	    __BITS(3,0),		/* div */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    0				/* flags */
	    ),

	/* AHB_GATING_REG0 */
	SUNXI_CCU_GATE(A10_CLK_AHB_OTG, "ahb-otg", "ahb",
	    AHB_GATING_REG0, 0),
	SUNXI_CCU_GATE(A10_CLK_AHB_EHCI0, "ahb-ehci0", "ahb",
	    AHB_GATING_REG0, 1),
	SUNXI_CCU_GATE(A10_CLK_AHB_OHCI0, "ahb-ohci0", "ahb",
	    AHB_GATING_REG0, 2),
	SUNXI_CCU_GATE(A10_CLK_AHB_EHCI1, "ahb-ehci1", "ahb",
	    AHB_GATING_REG0, 3),
	SUNXI_CCU_GATE(A10_CLK_AHB_OHCI1, "ahb-ohci1", "ahb",
	    AHB_GATING_REG0, 4),
	SUNXI_CCU_GATE(A10_CLK_AHB_SS, "ahb-ss", "ahb",
	    AHB_GATING_REG0, 5),
	SUNXI_CCU_GATE(A10_CLK_AHB_DMA, "ahb-dma", "ahb",
	    AHB_GATING_REG0, 6),
	SUNXI_CCU_GATE(A10_CLK_AHB_BIST, "ahb-bist", "ahb",
	    AHB_GATING_REG0, 7),
	SUNXI_CCU_GATE(A10_CLK_AHB_MMC0, "ahb-mmc0", "ahb",
	    AHB_GATING_REG0, 8),
	SUNXI_CCU_GATE(A10_CLK_AHB_MMC1, "ahb-mmc1", "ahb",
	    AHB_GATING_REG0, 9),
	SUNXI_CCU_GATE(A10_CLK_AHB_MMC2, "ahb-mmc2", "ahb",
	    AHB_GATING_REG0, 10),
	SUNXI_CCU_GATE(A10_CLK_AHB_MMC3, "ahb-mmc3", "ahb",
	    AHB_GATING_REG0, 11),
	SUNXI_CCU_GATE(A10_CLK_AHB_MS, "ahb-ms", "ahb",
	    AHB_GATING_REG0, 12),
	SUNXI_CCU_GATE(A10_CLK_AHB_NAND, "ahb-nand", "ahb",
	    AHB_GATING_REG0, 13),
	SUNXI_CCU_GATE(A10_CLK_AHB_SDRAM, "ahb-sdram", "ahb",
	    AHB_GATING_REG0, 14),
	SUNXI_CCU_GATE(A10_CLK_AHB_ACE, "ahb-ace", "ahb",
	    AHB_GATING_REG0, 16),
	SUNXI_CCU_GATE(A10_CLK_AHB_EMAC, "ahb-emac", "ahb",
	    AHB_GATING_REG0, 17),
	SUNXI_CCU_GATE(A10_CLK_AHB_TS, "ahb-ts", "ahb",
	    AHB_GATING_REG0, 18),
	SUNXI_CCU_GATE(A10_CLK_AHB_SPI0, "ahb-spi0", "ahb",
	    AHB_GATING_REG0, 20),
	SUNXI_CCU_GATE(A10_CLK_AHB_SPI1, "ahb-spi1", "ahb",
	    AHB_GATING_REG0, 21),
	SUNXI_CCU_GATE(A10_CLK_AHB_SPI2, "ahb-spi2", "ahb",
	    AHB_GATING_REG0, 22),
	SUNXI_CCU_GATE(A10_CLK_AHB_SPI3, "ahb-spi3", "ahb",
	    AHB_GATING_REG0, 23),
	SUNXI_CCU_GATE(A10_CLK_AHB_SATA, "ahb-sata", "ahb",
	    AHB_GATING_REG0, 25),
	SUNXI_CCU_GATE(A10_CLK_AHB_HSTIMER, "ahb-hstimer", "ahb",
	    AHB_GATING_REG0, 28),

	/* AHB_GATING_REG1. Missing: TVE, HDMI */
	SUNXI_CCU_GATE(A10_CLK_AHB_VE, "ahb-ve", "ahb",
	    AHB_GATING_REG1, 0),
	SUNXI_CCU_GATE(A10_CLK_AHB_TVD, "ahb-tvd", "ahb",
	    AHB_GATING_REG1, 1),
	SUNXI_CCU_GATE(A10_CLK_AHB_TVE0, "ahb-tve0", "ahb",
	    AHB_GATING_REG1, 2),
	SUNXI_CCU_GATE(A10_CLK_AHB_TVE1, "ahb-tve1", "ahb",
	    AHB_GATING_REG1, 3),
	SUNXI_CCU_GATE(A10_CLK_AHB_LCD0, "ahb-lcd0", "ahb",
	    AHB_GATING_REG1, 4),
	SUNXI_CCU_GATE(A10_CLK_AHB_LCD1, "ahb-lcd1", "ahb",
	    AHB_GATING_REG1, 5),
	SUNXI_CCU_GATE(A10_CLK_AHB_CSI0, "ahb-csi0", "ahb",
	    AHB_GATING_REG1, 8),
	SUNXI_CCU_GATE(A10_CLK_AHB_CSI1, "ahb-csi1", "ahb",
	    AHB_GATING_REG1, 9),
	SUNXI_CCU_GATE(A10_CLK_AHB_HDMI1, "ahb-hdmi1", "ahb",
	    AHB_GATING_REG1, 10),
	SUNXI_CCU_GATE(A10_CLK_AHB_HDMI0, "ahb-hdmi0", "ahb",
	    AHB_GATING_REG1, 11),
	SUNXI_CCU_GATE(A10_CLK_AHB_DE_BE0, "ahb-de_be0", "ahb",
	    AHB_GATING_REG1, 12),
	SUNXI_CCU_GATE(A10_CLK_AHB_DE_BE1, "ahb-de_be1", "ahb",
	    AHB_GATING_REG1, 13),
	SUNXI_CCU_GATE(A10_CLK_AHB_DE_FE0, "ahb-de_fe0", "ahb",
	    AHB_GATING_REG1, 14),
	SUNXI_CCU_GATE(A10_CLK_AHB_DE_FE1, "ahb-de_fe1", "ahb",
	    AHB_GATING_REG1, 15),
	SUNXI_CCU_GATE(A10_CLK_AHB_GMAC, "ahb-gmac", "ahb",
	    AHB_GATING_REG1, 17),
	SUNXI_CCU_GATE(A10_CLK_AHB_MP, "ahb-mp", "ahb",
	    AHB_GATING_REG1, 18),
	SUNXI_CCU_GATE(A10_CLK_AHB_GPU, "ahb-gpu", "ahb",
	    AHB_GATING_REG1, 20),

	/* APB0_GATING_REG */
	SUNXI_CCU_GATE(A10_CLK_APB0_CODEC, "apb0-codec", "apb0",
	    APB0_GATING_REG, 0),
	SUNXI_CCU_GATE(A10_CLK_APB0_SPDIF, "apb0-spdif", "apb0",
	    APB0_GATING_REG, 1),
	SUNXI_CCU_GATE(A10_CLK_APB0_AC97, "apb0-ac97", "apb0",
	    APB0_GATING_REG, 2),
	SUNXI_CCU_GATE(A10_CLK_APB0_I2S0, "apb0-i2s0", "apb0",
	    APB0_GATING_REG, 3),
	SUNXI_CCU_GATE(A10_CLK_APB0_I2S1, "apb0-i2s1", "apb0",
	    APB0_GATING_REG, 4),
	SUNXI_CCU_GATE(A10_CLK_APB0_PIO, "apb0-pio", "apb0",
	    APB0_GATING_REG, 5),
	SUNXI_CCU_GATE(A10_CLK_APB0_IR0, "apb0-ir0", "apb0",
	    APB0_GATING_REG, 6),
	SUNXI_CCU_GATE(A10_CLK_APB0_IR1, "apb0-ir1", "apb0",
	    APB0_GATING_REG, 7),
	SUNXI_CCU_GATE(A10_CLK_APB0_I2S2, "apb0-i2s2", "apb0",
	    APB0_GATING_REG, 8),
	SUNXI_CCU_GATE(A10_CLK_APB0_KEYPAD, "apb0-keypad", "apb0",
	    APB0_GATING_REG, 10),

	/* APB1_GATING_REG */
	SUNXI_CCU_GATE(A10_CLK_APB1_I2C0, "apb1-i2c0", "apb1",
	    APB1_GATING_REG, 0),
	SUNXI_CCU_GATE(A10_CLK_APB1_I2C1, "apb1-i2c1", "apb1",
	    APB1_GATING_REG, 1),
	SUNXI_CCU_GATE(A10_CLK_APB1_I2C2, "apb1-i2c2", "apb1",
	    APB1_GATING_REG, 2),
	SUNXI_CCU_GATE(A10_CLK_APB1_I2C3, "apb1-i2c3", "apb1",
	    APB1_GATING_REG, 3),
	SUNXI_CCU_GATE(A10_CLK_APB1_CAN, "apb1-can", "apb1",
	    APB1_GATING_REG, 4),
	SUNXI_CCU_GATE(A10_CLK_APB1_SCR, "apb1-scr", "apb1",
	    APB1_GATING_REG, 5),
	SUNXI_CCU_GATE(A10_CLK_APB1_PS20, "apb1-ps20", "apb1",
	    APB1_GATING_REG, 6),
	SUNXI_CCU_GATE(A10_CLK_APB1_PS21, "apb1-ps21", "apb1",
	    APB1_GATING_REG, 7),
	SUNXI_CCU_GATE(A10_CLK_APB1_I2C4, "apb1-i2c4", "apb1",
	    APB1_GATING_REG, 15),
	SUNXI_CCU_GATE(A10_CLK_APB1_UART0, "apb1-uart0", "apb1",
	    APB1_GATING_REG, 16),
	SUNXI_CCU_GATE(A10_CLK_APB1_UART1, "apb1-uart1", "apb1",
	    APB1_GATING_REG, 17),
	SUNXI_CCU_GATE(A10_CLK_APB1_UART2, "apb1-uart2", "apb1",
	    APB1_GATING_REG, 18),
	SUNXI_CCU_GATE(A10_CLK_APB1_UART3, "apb1-uart3", "apb1",
	    APB1_GATING_REG, 19),
	SUNXI_CCU_GATE(A10_CLK_APB1_UART4, "apb1-uart4", "apb1",
	    APB1_GATING_REG, 20),
	SUNXI_CCU_GATE(A10_CLK_APB1_UART5, "apb1-uart5", "apb1",
	    APB1_GATING_REG, 21),
	SUNXI_CCU_GATE(A10_CLK_APB1_UART6, "apb1-uart6", "apb1",
	    APB1_GATING_REG, 22),
	SUNXI_CCU_GATE(A10_CLK_APB1_UART7, "apb1-uart7", "apb1",
	    APB1_GATING_REG, 23),

	/* DRAM GATING */
	SUNXI_CCU_GATE(A10_CLK_DRAM_DE_BE0, "dram-de-be0", "pll_ddr_other",
	    DRAM_GATING_REG, 26),
	SUNXI_CCU_GATE(A10_CLK_DRAM_DE_BE1, "dram-de-be1", "pll_ddr_other",
	    DRAM_GATING_REG, 27),
	SUNXI_CCU_GATE(A10_CLK_DRAM_DE_FE0, "dram-de-fe0", "pll_ddr_other",
	    DRAM_GATING_REG, 25),
	SUNXI_CCU_GATE(A10_CLK_DRAM_DE_FE1, "dram-de-fe1", "pll_ddr_other",
	    DRAM_GATING_REG, 24),

	/* AUDIO_CODEC_SCLK_CFG_REG */
	SUNXI_CCU_GATE(A10_CLK_CODEC, "codec", "pll_audio",
	    AUDIO_CODEC_SCLK_CFG_REG, 31),

	/* USBPHY_CFG_REG */
	SUNXI_CCU_GATE(A10_CLK_USB_OHCI0, "usb-ohci0", "osc24m",
	    USBPHY_CFG_REG, 6),
	SUNXI_CCU_GATE(A10_CLK_USB_OHCI1, "usb-ohci1", "osc24m",
	    USBPHY_CFG_REG, 7),
	SUNXI_CCU_GATE(A10_CLK_USB_PHY, "usb-phy", "osc24m",
	    USBPHY_CFG_REG, 8),
};

/*
 * some special cases
 * hardcode lcd0 (tcon0) to pll3 and lcd1 (tcon1) to pll7.
 * compute pll rate based on desired pixel clock
 */

static int
sun4i_a10_ccu_lcd0ch0_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk * clk, u_int rate)
{
	int error;
	error = sunxi_ccu_lcdxch0_set_rate(sc, clk,
	    &sun4i_a10_ccu_clks[A10_CLK_PLL_VIDEO0],
	    &sun4i_a10_ccu_clks[A10_CLK_PLL_VIDEO0_2X],
	    rate);
	return error;
}

static int
sun4i_a10_ccu_lcd1ch0_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk * clk, u_int rate)
{
	return sunxi_ccu_lcdxch0_set_rate(sc, clk,
	    &sun4i_a10_ccu_clks[A10_CLK_PLL_VIDEO1],
	    &sun4i_a10_ccu_clks[A10_CLK_PLL_VIDEO1_2X],
	    rate);
}

static u_int
sun4i_a10_ccu_lcd0ch0_round_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk * clk, u_int rate)
{
	return sunxi_ccu_lcdxch0_round_rate(sc, clk,
	    &sun4i_a10_ccu_clks[A10_CLK_PLL_VIDEO0],
	    &sun4i_a10_ccu_clks[A10_CLK_PLL_VIDEO0_2X],
	    rate);
}

static u_int
sun4i_a10_ccu_lcd1ch0_round_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk * clk, u_int rate)
{
	return sunxi_ccu_lcdxch0_round_rate(sc, clk,
	    &sun4i_a10_ccu_clks[A10_CLK_PLL_VIDEO1],
	    &sun4i_a10_ccu_clks[A10_CLK_PLL_VIDEO1_2X],
	    rate);
}

static int
sun4i_a10_ccu_lcd0ch1_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk * clk, u_int rate)
{
	return sunxi_ccu_lcdxch1_set_rate(sc, clk,
	    &sun4i_a10_ccu_clks[A10_CLK_PLL_VIDEO0],
	    &sun4i_a10_ccu_clks[A10_CLK_PLL_VIDEO0_2X],
	    rate);
}

static int
sun4i_a10_ccu_lcd1ch1_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk * clk, u_int rate)
{
	return sunxi_ccu_lcdxch1_set_rate(sc, clk,
	    &sun4i_a10_ccu_clks[A10_CLK_PLL_VIDEO1],
	    &sun4i_a10_ccu_clks[A10_CLK_PLL_VIDEO1_2X],
	    rate);
}

#if 0
static int
sun4i_a10_ccu_lcdxch0_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk * clk, u_int rate, int unit)
{
	int parent_index;
	struct clk *clkp;
	int error;

	parent_index = (unit == 0) ? A10_CLK_PLL_VIDEO0 : A10_CLK_PLL_VIDEO1;
	clkp = &sun4i_a10_ccu_clks[parent_index].base;
	error = clk_set_rate(clkp, rate);
	if (error) {
		error = clk_set_rate(clkp, rate / 2);
		if (error != 0)
			return error;
		parent_index =
		    (unit == 0) ? A10_CLK_PLL_VIDEO0_2X : A10_CLK_PLL_VIDEO1_2X;
		clkp = &sun4i_a10_ccu_clks[parent_index].base;
	}
	error = clk_set_parent(&clk->base, clkp);
	KASSERT(error == 0);
	return error;
}

static u_int
sun4i_a10_ccu_lcdxch0_round_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk * clk, u_int try_rate, int unit)
{
	int parent_index;
	struct clk *clkp;
	int diff, diff_x2;
	int rate, rate_x2;

	parent_index = (unit == 0) ? A10_CLK_PLL_VIDEO0 : A10_CLK_PLL_VIDEO1;
	clkp = &sun4i_a10_ccu_clks[parent_index].base;
	rate = clk_round_rate(clkp, try_rate);
	diff = abs(try_rate - rate);

	rate_x2 = (clk_round_rate(clkp, try_rate / 2) * 2);
	diff_x2 = abs(try_rate - rate_x2);
	
	if (diff_x2 < diff)
		return rate_x2;
	return rate;
}

static void
sun4i_a10_tcon_calc_pll(int f_ref, int f_out, int *pm, int *pn, int *pd)
{
	int best = INT_MAX;
	for (int d = 1; d <= 2 && best != 0; d++) {
		for (int m = 1; m <= 16 && best != 0; m++) {
			for (int n = 9; n <= 127 && best != 0; n++) {
				int f_cur = (n * f_ref * d) / m;
				int diff = abs(f_out - f_cur);
				if (diff < best) {
					best = diff;
					*pm = m;
					*pn = n;
					*pd = d;
					if (diff == 0)
						return;
				}
			}
		}
	}
}

static int
sun4i_a10_ccu_lcdxch1_set_rate(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, u_int rate, int unit)
{
	int parent_index;
	struct clk *clkp, *pllclk;
	int error;
        int n = 0, m = 0, d = 0;

	parent_index = (unit == 0) ? A10_CLK_PLL_VIDEO0 : A10_CLK_PLL_VIDEO1;
	clkp = &sun4i_a10_ccu_clks[parent_index].base;
	pllclk = clkp;

        sun4i_a10_tcon_calc_pll(3000000, rate, &m, &n, &d);

        if (n == 0 || m == 0 || d == 0)
		return ERANGE;

        if (d == 2) {
		parent_index =
		    (unit == 0) ? A10_CLK_PLL_VIDEO0_2X : A10_CLK_PLL_VIDEO1_2X;
		clkp = &sun4i_a10_ccu_clks[parent_index].base;
        }

	error = clk_set_rate(pllclk, 3000000 * n);
	KASSERT(error == 0);
	error = clk_set_parent(&clk->base, clkp);
	KASSERT(error == 0);
	error = sunxi_ccu_div_set_rate(sc, clk, rate);
	KASSERT(error == 0);
	return error;
}
#endif

static int
sun4i_a10_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static struct sunxi_ccu_softc *sc0;
static void
sun4i_a10_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	enum sun4i_a10_ccu_type type;
	struct clk *clk, *clkp;
	int error;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun4i_a10_ccu_resets;
	sc->sc_nresets = __arraycount(sun4i_a10_ccu_resets);

	sc->sc_clks = sun4i_a10_ccu_clks;
	sc->sc_nclks = __arraycount(sun4i_a10_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");

	type = of_search_compatible(faa->faa_phandle, compat_data)->data;

	switch (type) {
	case CCU_A10:
		aprint_normal(": A10 CCU\n");
		break;
	case CCU_A20:
		aprint_normal(": A20 CCU\n");
		break;
	}
	/* hardcode debe clocks parent to PLL5 */
	clkp = &sun4i_a10_ccu_clks[A10_CLK_PLL_DDR_BASE].base;
	clk =  &sun4i_a10_ccu_clks[A10_CLK_DE_BE0].base;
	error = clk_set_parent(clk, clkp);
	KASSERT(error == 0);
	clk =  &sun4i_a10_ccu_clks[A10_CLK_DE_BE1].base;
	error = clk_set_parent(clk, clkp);
	KASSERT(error == 0);

	(void)error;
	sunxi_ccu_print(sc);
	sc0 = sc;
}

void sun4i_ccu_print(void);
void
sun4i_ccu_print(void)
{
	sunxi_ccu_print(sc0);
}
