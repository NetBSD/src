/* $NetBSD: sun50i_a64_ccu.c,v 1.3.4.1 2018/05/21 04:35:59 pgoyette Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: sun50i_a64_ccu.c,v 1.3.4.1 2018/05/21 04:35:59 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sun50i_a64_ccu.h>

#define	PLL_CPUX_CTRL_REG	0x000
#define	PLL_AUDIO_CTRL_REG	0x008
#define	PLL_PERIPH0_CTRL_REG	0x028
#define	PLL_PERIPH1_CTRL_REG	0x02c
#define	AHB1_APB1_CFG_REG	0x054
#define	APB2_CFG_REG		0x058
#define	AHB2_CFG_REG		0x05c
#define	BUS_CLK_GATING_REG0	0x060
#define	BUS_CLK_GATING_REG1	0x064
#define	BUS_CLK_GATING_REG2	0x068
#define	BUS_CLK_GATING_REG3	0x06c
#define	BUS_CLK_GATING_REG4	0x070
#define	THS_CLK_REG		0x074
#define	SDMMC0_CLK_REG		0x088
#define	SDMMC1_CLK_REG		0x08c
#define	SDMMC2_CLK_REG		0x090
#define	USBPHY_CFG_REG		0x0cc
#define	DRAM_CFG_REG		0x0f4
#define	MBUS_RST_REG		0x0fc
#define	AC_DIG_CLK_REG		0x140
#define	BUS_SOFT_RST_REG0	0x2c0
#define	BUS_SOFT_RST_REG1	0x2c4
#define	BUS_SOFT_RST_REG2	0x2c8
#define	BUS_SOFT_RST_REG3	0x2d0
#define	BUS_SOFT_RST_REG4	0x2d8

static int sun50i_a64_ccu_match(device_t, cfdata_t, void *);
static void sun50i_a64_ccu_attach(device_t, device_t, void *);

static const char * const compatible[] = {
	"allwinner,sun50i-a64-ccu",
	NULL
};

CFATTACH_DECL_NEW(sunxi_a64_ccu, sizeof(struct sunxi_ccu_softc),
	sun50i_a64_ccu_match, sun50i_a64_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun50i_a64_ccu_resets[] = {
	SUNXI_CCU_RESET(A64_RST_USB_PHY0, USBPHY_CFG_REG, 0),
	SUNXI_CCU_RESET(A64_RST_USB_PHY1, USBPHY_CFG_REG, 1),
	SUNXI_CCU_RESET(A64_RST_USB_HSIC, USBPHY_CFG_REG, 2),

	SUNXI_CCU_RESET(A64_RST_DRAM, DRAM_CFG_REG, 31),

	SUNXI_CCU_RESET(A64_RST_MBUS, MBUS_RST_REG, 31),

	SUNXI_CCU_RESET(A64_RST_BUS_MIPI_DSI, BUS_SOFT_RST_REG0, 1),
	SUNXI_CCU_RESET(A64_RST_BUS_CE, BUS_SOFT_RST_REG0, 5),
	SUNXI_CCU_RESET(A64_RST_BUS_DMA, BUS_SOFT_RST_REG0, 6),
	SUNXI_CCU_RESET(A64_RST_BUS_MMC0, BUS_SOFT_RST_REG0, 8),
	SUNXI_CCU_RESET(A64_RST_BUS_MMC1, BUS_SOFT_RST_REG0, 9),
	SUNXI_CCU_RESET(A64_RST_BUS_MMC2, BUS_SOFT_RST_REG0, 10),
	SUNXI_CCU_RESET(A64_RST_BUS_NAND, BUS_SOFT_RST_REG0, 13),
	SUNXI_CCU_RESET(A64_RST_BUS_DRAM, BUS_SOFT_RST_REG0, 14),
	SUNXI_CCU_RESET(A64_RST_BUS_EMAC, BUS_SOFT_RST_REG0, 17),
	SUNXI_CCU_RESET(A64_RST_BUS_TS, BUS_SOFT_RST_REG0, 18),
	SUNXI_CCU_RESET(A64_RST_BUS_HSTIMER, BUS_SOFT_RST_REG0, 19),
	SUNXI_CCU_RESET(A64_RST_BUS_SPI0, BUS_SOFT_RST_REG0, 20),
	SUNXI_CCU_RESET(A64_RST_BUS_SPI1, BUS_SOFT_RST_REG0, 21),
	SUNXI_CCU_RESET(A64_RST_BUS_OTG, BUS_SOFT_RST_REG0, 23),
	SUNXI_CCU_RESET(A64_RST_BUS_EHCI0, BUS_SOFT_RST_REG0, 24),
	SUNXI_CCU_RESET(A64_RST_BUS_EHCI1, BUS_SOFT_RST_REG0, 25),
	SUNXI_CCU_RESET(A64_RST_BUS_OHCI0, BUS_SOFT_RST_REG0, 28),
	SUNXI_CCU_RESET(A64_RST_BUS_OHCI1, BUS_SOFT_RST_REG0, 29),
        
	SUNXI_CCU_RESET(A64_RST_BUS_VE, BUS_SOFT_RST_REG1, 0),
	SUNXI_CCU_RESET(A64_RST_BUS_TCON0, BUS_SOFT_RST_REG1, 3),
	SUNXI_CCU_RESET(A64_RST_BUS_TCON1, BUS_SOFT_RST_REG1, 4),
	SUNXI_CCU_RESET(A64_RST_BUS_DEINTERLACE, BUS_SOFT_RST_REG1, 5),
	SUNXI_CCU_RESET(A64_RST_BUS_CSI, BUS_SOFT_RST_REG1, 8),
	SUNXI_CCU_RESET(A64_RST_BUS_HDMI0, BUS_SOFT_RST_REG1, 10),
	SUNXI_CCU_RESET(A64_RST_BUS_HDMI1, BUS_SOFT_RST_REG1, 11),
	SUNXI_CCU_RESET(A64_RST_BUS_DE, BUS_SOFT_RST_REG1, 12),
	SUNXI_CCU_RESET(A64_RST_BUS_GPU, BUS_SOFT_RST_REG1, 20),
	SUNXI_CCU_RESET(A64_RST_BUS_MSGBOX, BUS_SOFT_RST_REG1, 21),
	SUNXI_CCU_RESET(A64_RST_BUS_SPINLOCK, BUS_SOFT_RST_REG1, 22),
	SUNXI_CCU_RESET(A64_RST_BUS_DBG, BUS_SOFT_RST_REG1, 31),

	SUNXI_CCU_RESET(A64_RST_BUS_LVDS, BUS_SOFT_RST_REG2, 0),

	SUNXI_CCU_RESET(A64_RST_BUS_CODEC, BUS_SOFT_RST_REG3, 0),
	SUNXI_CCU_RESET(A64_RST_BUS_SPDIF, BUS_SOFT_RST_REG3, 1),
	SUNXI_CCU_RESET(A64_RST_BUS_THS, BUS_SOFT_RST_REG3, 8),
	SUNXI_CCU_RESET(A64_RST_BUS_I2S0, BUS_SOFT_RST_REG3, 12),
	SUNXI_CCU_RESET(A64_RST_BUS_I2S1, BUS_SOFT_RST_REG3, 13),
	SUNXI_CCU_RESET(A64_RST_BUS_I2S2, BUS_SOFT_RST_REG3, 14),

	SUNXI_CCU_RESET(A64_RST_BUS_I2C0, BUS_SOFT_RST_REG4, 0),
	SUNXI_CCU_RESET(A64_RST_BUS_I2C1, BUS_SOFT_RST_REG4, 1),
	SUNXI_CCU_RESET(A64_RST_BUS_I2C2, BUS_SOFT_RST_REG4, 2),
	SUNXI_CCU_RESET(A64_RST_BUS_SCR, BUS_SOFT_RST_REG4, 5),
	SUNXI_CCU_RESET(A64_RST_BUS_UART0, BUS_SOFT_RST_REG4, 16),
	SUNXI_CCU_RESET(A64_RST_BUS_UART1, BUS_SOFT_RST_REG4, 17),
	SUNXI_CCU_RESET(A64_RST_BUS_UART2, BUS_SOFT_RST_REG4, 18),
	SUNXI_CCU_RESET(A64_RST_BUS_UART3, BUS_SOFT_RST_REG4, 19),
};

static const char *ahb1_parents[] = { "losc", "hosc", "axi", "pll_periph0" };
static const char *ahb2_parents[] = { "ahb1", "pll_periph0" };
static const char *apb1_parents[] = { "ahb1" };
static const char *apb2_parents[] = { "losc", "hosc", "pll_periph0" };
static const char *mmc_parents[] = { "hosc", "pll_periph0_2x", "pll_periph1_2x" };
static const char *ths_parents[] = { "hosc", NULL, NULL, NULL };

static const struct sunxi_ccu_nkmp_tbl sun50i_a64_cpux_table[] = {
	{ 60000000, 9, 0, 0, 2 },
	{ 66000000, 10, 0, 0, 2 },
	{ 72000000, 11, 0, 0, 2 },
	{ 78000000, 12, 0, 0, 2 },
	{ 84000000, 13, 0, 0, 2 },
	{ 90000000, 14, 0, 0, 2 },
	{ 96000000, 15, 0, 0, 2 },
	{ 102000000, 16, 0, 0, 2 },
	{ 108000000, 17, 0, 0, 2 },
	{ 114000000, 18, 0, 0, 2 },
	{ 120000000, 9, 0, 0, 1 },
	{ 132000000, 10, 0, 0, 1 },
	{ 144000000, 11, 0, 0, 1 },
	{ 156000000, 12, 0, 0, 1 },
	{ 168000000, 13, 0, 0, 1 },
	{ 180000000, 14, 0, 0, 1 },
	{ 192000000, 15, 0, 0, 1 },
	{ 204000000, 16, 0, 0, 1 },
	{ 216000000, 17, 0, 0, 1 },
	{ 228000000, 18, 0, 0, 1 },
	{ 240000000, 9, 0, 0, 0 },
	{ 264000000, 10, 0, 0, 0 },
	{ 288000000, 11, 0, 0, 0 },
	{ 312000000, 12, 0, 0, 0 },
	{ 336000000, 13, 0, 0, 0 },
	{ 360000000, 14, 0, 0, 0 },
	{ 384000000, 15, 0, 0, 0 },
	{ 408000000, 16, 0, 0, 0 },
	{ 432000000, 17, 0, 0, 0 },
	{ 456000000, 18, 0, 0, 0 },
	{ 480000000, 19, 0, 0, 0 },
	{ 504000000, 20, 0, 0, 0 },
	{ 528000000, 21, 0, 0, 0 },
	{ 552000000, 22, 0, 0, 0 },
	{ 576000000, 23, 0, 0, 0 },
	{ 600000000, 24, 0, 0, 0 },
	{ 624000000, 25, 0, 0, 0 },
	{ 648000000, 26, 0, 0, 0 },
	{ 672000000, 27, 0, 0, 0 },
	{ 696000000, 28, 0, 0, 0 },
	{ 720000000, 29, 0, 0, 0 },
	{ 768000000, 15, 1, 0, 0 },
	{ 792000000, 10, 2, 0, 0 },
	{ 816000000, 16, 1, 0, 0 },
	{ 864000000, 17, 1, 0, 0 },
	{ 912000000, 18, 1, 0, 0 },
	{ 936000000, 12, 2, 0, 0 },
	{ 960000000, 19, 1, 0, 0 },
	{ 1008000000, 20, 1, 0, 0 },
	{ 1056000000, 21, 1, 0, 0 },
	{ 1080000000, 14, 2, 0, 0 },
	{ 1104000000, 22, 1, 0, 0 },
	{ 1152000000, 23, 1, 0, 0 },
	{ 1200000000, 24, 1, 0, 0 },
	{ 1224000000, 16, 2, 0, 0 },
	{ 1248000000, 25, 1, 0, 0 },
	{ 1296000000, 26, 1, 0, 0 },
	{ 1344000000, 27, 1, 0, 0 },
	{ 1368000000, 18, 2, 0, 0 },
	{ 1440000000, 19, 2, 0, 0 },
	{ 1512000000, 20, 2, 0, 0 },
	{ 1536000000, 15, 3, 0, 0 },
	{ 1584000000, 21, 2, 0, 0 },
	{ 1632000000, 16, 3, 0, 0 },
	{ 1656000000, 22, 2, 0, 0 },
	{ 1728000000, 23, 2, 0, 0 },
	{ 1800000000, 24, 2, 0, 0 },
	{ 1872000000, 25, 2, 0, 0 },
	{ 0 }
};

static const struct sunxi_ccu_nkmp_tbl sun50i_a64_ac_dig_table[] = {
	{ 24576000, 0x55, 0, 0x14, 0x3 },
	{ 0 }
};

static struct sunxi_ccu_clk sun50i_a64_ccu_clks[] = {
	SUNXI_CCU_NKMP_TABLE(A64_CLK_PLL_CPUX, "pll_cpux", "hosc",
	    PLL_CPUX_CTRL_REG,		/* reg */
	    __BITS(12,8),		/* n */
	    __BITS(5,4),		/* k */
	    __BITS(1,0),		/* m */
	    __BITS(17,16),		/* p */
	    __BIT(31),			/* enable */
	    __BIT(28),			/* lock */
	    sun50i_a64_cpux_table,	/* table */
	    SUNXI_CCU_NKMP_SCALE_CLOCK | SUNXI_CCU_NKMP_FACTOR_P_POW2),

	SUNXI_CCU_NKMP(A64_CLK_PLL_PERIPH0, "pll_periph0", "hosc",
	    PLL_PERIPH0_CTRL_REG,	/* reg */
	    __BITS(12,8),		/* n */
	    __BITS(5,4), 		/* k */
	    0,				/* m */
	    __BITS(17,16),		/* p */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NKMP_DIVIDE_BY_TWO),
	SUNXI_CCU_FIXED_FACTOR(A64_CLK_PLL_PERIPH0_2X, "pll_periph0_2x", "pll_periph0", 1, 2),

	SUNXI_CCU_NKMP_TABLE(A64_CLK_PLL_AUDIO_BASE, "pll_audio_base", "hosc",
	    PLL_AUDIO_CTRL_REG,		/* reg */
	    __BITS(14,8),		/* n */
	    0,				/* k */
	    __BITS(4,0),		/* m */
	    __BITS(19,16),		/* p */
	    __BIT(31),			/* enable */
	    __BIT(28),			/* lock */
	    sun50i_a64_ac_dig_table,	/* table */
	    0),

	SUNXI_CCU_FIXED_FACTOR(A64_CLK_PLL_AUDIO, "pll_audio", "pll_audio_base", 1, 1),
	SUNXI_CCU_FIXED_FACTOR(A64_CLK_PLL_AUDIO_2X, "pll_audio_2x", "pll_audio_base", 1, 2),
	SUNXI_CCU_FIXED_FACTOR(A64_CLK_PLL_AUDIO_4X, "pll_audio_4x", "pll_audio_base", 1, 4),
	SUNXI_CCU_FIXED_FACTOR(A64_CLK_PLL_AUDIO_8X, "pll_audio_8x", "pll_audio_base", 1, 8),

	SUNXI_CCU_PREDIV(A64_CLK_AHB1, "ahb1", ahb1_parents,
	    AHB1_APB1_CFG_REG,	/* reg */
	    __BITS(7,6),	/* prediv */
	    __BIT(3),		/* prediv_sel */
	    __BITS(5,4),	/* div */
	    __BITS(13,12),	/* sel */
	    SUNXI_CCU_PREDIV_POWER_OF_TWO),

	SUNXI_CCU_PREDIV(A64_CLK_AHB2, "ahb2", ahb2_parents,
	    AHB2_CFG_REG,	/* reg */
	    0,			/* prediv */
	    __BIT(1),		/* prediv_sel */
	    0,			/* div */
	    __BITS(1,0),	/* sel */
	    SUNXI_CCU_PREDIV_DIVIDE_BY_TWO),

	SUNXI_CCU_DIV(A64_CLK_APB1, "apb1", apb1_parents,
	    AHB1_APB1_CFG_REG,	/* reg */
	    __BITS(9,8),	/* div */
	    0,			/* sel */
	    SUNXI_CCU_DIV_POWER_OF_TWO|SUNXI_CCU_DIV_ZERO_IS_ONE),

	SUNXI_CCU_NM(A64_CLK_APB2, "apb2", apb2_parents,
	    APB2_CFG_REG,	/* reg */
	    __BITS(17,16),	/* n */
	    __BITS(4,0),	/* m */
	    __BITS(25,24),	/* sel */
	    0,			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(A64_CLK_MMC0, "mmc0", mmc_parents,
	    SDMMC0_CLK_REG,	/* reg */
	    __BITS(17,16),	/* n */
	    __BITS(3,0),	/* m */
	    __BITS(25,24),	/* sel */
	    __BIT(31),		/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN|SUNXI_CCU_NM_DIVIDE_BY_TWO),
	SUNXI_CCU_NM(A64_CLK_MMC1, "mmc1", mmc_parents,
	    SDMMC1_CLK_REG,	/* reg */
	    __BITS(17,16),	/* n */
	    __BITS(3,0),	/* m */
	    __BITS(25,24),	/* sel */
	    __BIT(31),		/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN|SUNXI_CCU_NM_DIVIDE_BY_TWO),
	SUNXI_CCU_NM(A64_CLK_MMC2, "mmc2", mmc_parents,
	    SDMMC2_CLK_REG,	/* reg */
	    __BITS(17,16),	/* n */
	    __BITS(3,0),	/* m */
	    __BITS(25,24),	/* sel */
	    __BIT(31),		/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN|SUNXI_CCU_NM_DIVIDE_BY_TWO),

	SUNXI_CCU_DIV_GATE(A64_CLK_THS, "ths", ths_parents,
	    THS_CLK_REG,	/* reg */
	    __BITS(1,0),	/* div */
	    __BITS(25,24),	/* sel */
	    __BIT(31),		/* enable */
	    SUNXI_CCU_DIV_TIMES_TWO),

	SUNXI_CCU_GATE(A64_CLK_AC_DIG, "ac-dig", "pll_audio",
	    AC_DIG_CLK_REG, 31),
	SUNXI_CCU_GATE(A64_CLK_AC_DIG_4X, "ac-dig-4x", "pll_audio_4x",
	    AC_DIG_CLK_REG, 30),

	SUNXI_CCU_GATE(A64_CLK_BUS_MIPI_DSI, "bus-mipi-dsi", "ahb1",
	    BUS_CLK_GATING_REG0, 1),
	SUNXI_CCU_GATE(A64_CLK_BUS_CE, "bus-ce", "ahb1",
	    BUS_CLK_GATING_REG0, 5),
	SUNXI_CCU_GATE(A64_CLK_BUS_DMA, "bus-dma", "ahb1",
	    BUS_CLK_GATING_REG0, 6),
	SUNXI_CCU_GATE(A64_CLK_BUS_MMC0, "bus-mmc0", "ahb1",
	    BUS_CLK_GATING_REG0, 8),
	SUNXI_CCU_GATE(A64_CLK_BUS_MMC1, "bus-mmc1", "ahb1",
	    BUS_CLK_GATING_REG0, 9),
	SUNXI_CCU_GATE(A64_CLK_BUS_MMC2, "bus-mmc2", "ahb1",
	    BUS_CLK_GATING_REG0, 10),
	SUNXI_CCU_GATE(A64_CLK_BUS_NAND, "bus-nand", "ahb1",
	    BUS_CLK_GATING_REG0, 13),
	SUNXI_CCU_GATE(A64_CLK_BUS_DRAM, "bus-dram", "ahb1",
	    BUS_CLK_GATING_REG0, 14),
	SUNXI_CCU_GATE(A64_CLK_BUS_EMAC, "bus-emac", "ahb2",
	    BUS_CLK_GATING_REG0, 17),
	SUNXI_CCU_GATE(A64_CLK_BUS_TS, "bus-ts", "ahb1",
	    BUS_CLK_GATING_REG0, 18),
	SUNXI_CCU_GATE(A64_CLK_BUS_HSTIMER, "bus-hstimer", "ahb1",
	    BUS_CLK_GATING_REG0, 19),
	SUNXI_CCU_GATE(A64_CLK_BUS_SPI0, "bus-spi0", "ahb1",
	    BUS_CLK_GATING_REG0, 20),
	SUNXI_CCU_GATE(A64_CLK_BUS_SPI1, "bus-spi1", "ahb1",
	    BUS_CLK_GATING_REG0, 21),
	SUNXI_CCU_GATE(A64_CLK_BUS_OTG, "bus-otg", "ahb1",
	    BUS_CLK_GATING_REG0, 23),
	SUNXI_CCU_GATE(A64_CLK_BUS_EHCI0, "bus-ehci0", "ahb1",
	    BUS_CLK_GATING_REG0, 24),
	SUNXI_CCU_GATE(A64_CLK_BUS_EHCI1, "bus-ehci1", "ahb2",
	    BUS_CLK_GATING_REG0, 25),
	SUNXI_CCU_GATE(A64_CLK_BUS_OHCI0, "bus-ohci0", "ahb1",
	    BUS_CLK_GATING_REG0, 28),
	SUNXI_CCU_GATE(A64_CLK_BUS_OHCI1, "bus-ohci1", "ahb2",
	    BUS_CLK_GATING_REG0, 29),

	SUNXI_CCU_GATE(A64_CLK_BUS_VE, "bus-ve", "ahb1",
	    BUS_CLK_GATING_REG1, 0),
	SUNXI_CCU_GATE(A64_CLK_BUS_TCON0, "bus-tcon0", "ahb1",
	    BUS_CLK_GATING_REG1, 3),
	SUNXI_CCU_GATE(A64_CLK_BUS_TCON1, "bus-tcon1", "ahb1",
	    BUS_CLK_GATING_REG1, 4),
	SUNXI_CCU_GATE(A64_CLK_BUS_DEINTERLACE, "bus-deinterlace", "ahb1",
	    BUS_CLK_GATING_REG1, 5),
	SUNXI_CCU_GATE(A64_CLK_BUS_CSI, "bus-csi", "ahb1",
	    BUS_CLK_GATING_REG1, 8),
	SUNXI_CCU_GATE(A64_CLK_BUS_HDMI, "bus-hdmi", "ahb1",
	    BUS_CLK_GATING_REG1, 10),
	SUNXI_CCU_GATE(A64_CLK_BUS_DE, "bus-de", "ahb1",
	    BUS_CLK_GATING_REG1, 12),
	SUNXI_CCU_GATE(A64_CLK_BUS_GPU, "bus-gpu", "ahb1",
	    BUS_CLK_GATING_REG1, 20),
	SUNXI_CCU_GATE(A64_CLK_BUS_MSGBOX, "bus-msgbox", "ahb1",
	    BUS_CLK_GATING_REG1, 21),
	SUNXI_CCU_GATE(A64_CLK_BUS_SPINLOCK, "bus-spinlock", "ahb1",
	    BUS_CLK_GATING_REG1, 22),

	SUNXI_CCU_GATE(A64_CLK_BUS_CODEC, "bus-codec", "apb1",
	    BUS_CLK_GATING_REG2, 0),
	SUNXI_CCU_GATE(A64_CLK_BUS_SPDIF, "bus-spdif", "apb1",
	    BUS_CLK_GATING_REG2, 1),
	SUNXI_CCU_GATE(A64_CLK_BUS_PIO, "bus-pio", "apb1",
	    BUS_CLK_GATING_REG2, 5),
	SUNXI_CCU_GATE(A64_CLK_BUS_THS, "bus-ths", "apb1",
	    BUS_CLK_GATING_REG2, 8),
	SUNXI_CCU_GATE(A64_CLK_BUS_I2S0, "bus-i2s0", "apb1",
	    BUS_CLK_GATING_REG2, 12),
	SUNXI_CCU_GATE(A64_CLK_BUS_I2S1, "bus-i2s1", "apb1",
	    BUS_CLK_GATING_REG2, 13),
	SUNXI_CCU_GATE(A64_CLK_BUS_I2S2, "bus-i2s2", "apb1",
	    BUS_CLK_GATING_REG2, 14),

	SUNXI_CCU_GATE(A64_CLK_BUS_I2C0, "bus-i2c0", "apb2",
	    BUS_CLK_GATING_REG3, 0),
	SUNXI_CCU_GATE(A64_CLK_BUS_I2C1, "bus-i2c1", "apb2",
	    BUS_CLK_GATING_REG3, 1),
	SUNXI_CCU_GATE(A64_CLK_BUS_I2C2, "bus-i2c2", "apb2",
	    BUS_CLK_GATING_REG3, 2),
	SUNXI_CCU_GATE(A64_CLK_BUS_SCR, "bus-scr", "apb2",
	    BUS_CLK_GATING_REG3, 5),
	SUNXI_CCU_GATE(A64_CLK_BUS_UART0, "bus-uart0", "apb2",
	    BUS_CLK_GATING_REG3, 16),
	SUNXI_CCU_GATE(A64_CLK_BUS_UART1, "bus-uart1", "apb2",
	    BUS_CLK_GATING_REG3, 17),
	SUNXI_CCU_GATE(A64_CLK_BUS_UART2, "bus-uart2", "apb2",
	    BUS_CLK_GATING_REG3, 18),
	SUNXI_CCU_GATE(A64_CLK_BUS_UART3, "bus-uart3", "apb2",
	    BUS_CLK_GATING_REG3, 19),
	SUNXI_CCU_GATE(A64_CLK_BUS_UART4, "bus-uart4", "apb2",
	    BUS_CLK_GATING_REG3, 20),

	SUNXI_CCU_GATE(A64_CLK_USB_PHY0, "usb-phy0", "hosc",
	    USBPHY_CFG_REG, 8),
	SUNXI_CCU_GATE(A64_CLK_USB_PHY1, "usb-phy1", "hosc",
	    USBPHY_CFG_REG, 9),
	SUNXI_CCU_GATE(A64_CLK_USB_HSIC, "usb-hsic", "hosc",
	    USBPHY_CFG_REG, 10),
	SUNXI_CCU_GATE(A64_CLK_USB_HSIC_12M, "usb-hsic-12m", "hosc",
	    USBPHY_CFG_REG, 11),
	SUNXI_CCU_GATE(A64_CLK_USB_OHCI0, "usb-ohci0", "hosc",
	    USBPHY_CFG_REG, 16),
	SUNXI_CCU_GATE(A64_CLK_USB_OHCI1, "usb-ohci1", "usb-ohci0",
	    USBPHY_CFG_REG, 17),
};

static int
sun50i_a64_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sun50i_a64_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun50i_a64_ccu_resets;
	sc->sc_nresets = __arraycount(sun50i_a64_ccu_resets);

	sc->sc_clks = sun50i_a64_ccu_clks;
	sc->sc_nclks = __arraycount(sun50i_a64_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": A64 CCU\n");

	sunxi_ccu_print(sc);
}
