/* $NetBSD: sun5i_a13_ccu.c,v 1.5.2.2 2017/12/03 11:35:56 jdolecek Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: sun5i_a13_ccu.c,v 1.5.2.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sun5i_a13_ccu.h>

#define	PLL1_CFG_REG		0x000
#define	PLL2_CFG_REG		0x008
#define	PLL6_CFG_REG		0x028
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
#define	USBPHY_CFG_REG		0x0cc
#define	BE_CFG_REG		0x104
#define	FE_CFG_REG		0x10c
#define	CSI_CFG_REG		0x134
#define	VE_CFG_REG		0x13c
#define	AUDIO_CODEC_SCLK_CFG_REG 0x140
#define	MALI_CLOCK_CFG_REG	0x154
#define	IEP_SCLK_CFG_REG	0x160

static int sun5i_a13_ccu_match(device_t, cfdata_t, void *);
static void sun5i_a13_ccu_attach(device_t, device_t, void *);

static const char * const compatible[] = {
	"allwinner,sun5i-a13-ccu",
	"nextthing,gr8-ccu",
	NULL
};

CFATTACH_DECL_NEW(sunxi_a13_ccu, sizeof(struct sunxi_ccu_softc),
	sun5i_a13_ccu_match, sun5i_a13_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun5i_a13_ccu_resets[] = {
	SUNXI_CCU_RESET(A13_RST_USB_PHY0, USBPHY_CFG_REG, 0),
	SUNXI_CCU_RESET(A13_RST_USB_PHY1, USBPHY_CFG_REG, 1),

	/* Missing: GPS */

	SUNXI_CCU_RESET(A13_RST_DE_BE, BE_CFG_REG, 30),

	SUNXI_CCU_RESET(A13_RST_DE_FE, FE_CFG_REG, 30),

	/* Missing: TVE */

	/* Missing: LCD */

	SUNXI_CCU_RESET(A13_RST_CSI, CSI_CFG_REG, 30),

	SUNXI_CCU_RESET(A13_RST_VE, VE_CFG_REG, 0),

	SUNXI_CCU_RESET(A13_RST_GPU, MALI_CLOCK_CFG_REG, 30),

	SUNXI_CCU_RESET(A13_RST_IEP, IEP_SCLK_CFG_REG, 30),
};

static const char *cpu_parents[] = { "losc", "osc24m", "pll_core", "pll_periph" };
static const char *axi_parents[] = { "cpu" };
static const char *ahb_parents[] = { "axi", "cpu", "pll_periph" };
static const char *apb0_parents[] = { "ahb" };
static const char *apb1_parents[] = { "osc24m", "pll_periph", "losc" };
static const char *mod_parents[] = { "osc24m", "pll_periph", "pll_ddr" };

static const struct sunxi_ccu_nkmp_tbl sun5i_a13_ac_dig_table[] = {
	{ 24576000, 86, 0, 21, 3 },
	{ 0 }
};

static struct sunxi_ccu_clk sun5i_a13_ccu_clks[] = {
	SUNXI_CCU_GATE(A13_CLK_HOSC, "osc24m", "hosc",
	    OSC24M_CFG_REG, 0),

	SUNXI_CCU_NKMP(A13_CLK_PLL_CORE, "pll_core", "osc24m",
	    PLL1_CFG_REG,		/* reg */
	    __BITS(12,8),		/* n */
	    __BITS(5,4), 		/* k */
	    __BITS(1,0),		/* m */
	    __BITS(17,16),		/* p */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NKMP_FACTOR_P_POW2 | SUNXI_CCU_NKMP_FACTOR_N_EXACT),

	SUNXI_CCU_NKMP_TABLE(A13_CLK_PLL_AUDIO_BASE, "pll_audio", "osc24m",
	    PLL2_CFG_REG,		/* reg */
	    __BITS(14,8),		/* n */
	    0,				/* k */
	    __BITS(4,0),		/* m */
	    __BITS(29,26),		/* p */
	    __BIT(31),			/* enable */
	    0,				/* lock */
	    sun5i_a13_ac_dig_table,	/* table */
	    0),

	SUNXI_CCU_NKMP(A13_CLK_PERIPH, "pll_periph", "osc24m",
	    PLL6_CFG_REG,		/* reg */
	    __BITS(12,8),		/* n */
	    __BITS(5,4), 		/* k */
	    __BITS(1,0),		/* m */
	    0,				/* p */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NKMP_DIVIDE_BY_TWO | SUNXI_CCU_NKMP_FACTOR_N_EXACT),

	SUNXI_CCU_PREDIV_FIXED(A13_CLK_CPU, "cpu", cpu_parents,
	    CPU_AHB_APB0_CFG_REG,	/* reg */
	    0,				/* prediv */
	    __BIT(3),			/* prediv_sel */
	    6,				/* prediv_fixed */
	    0,				/* div */
	    __BITS(17,16),		/* sel */
	    0),

	SUNXI_CCU_DIV(A13_CLK_AXI, "axi", axi_parents,
	    CPU_AHB_APB0_CFG_REG,	/* reg */
	    __BITS(1,0),		/* div */
	    0,				/* sel */
	    0),

	SUNXI_CCU_DIV(A13_CLK_AHB, "ahb", ahb_parents,
	    CPU_AHB_APB0_CFG_REG,	/* reg */
	    0,				/* div */
	    __BITS(5,4),		/* sel */
	    SUNXI_CCU_DIV_POWER_OF_TWO),

	SUNXI_CCU_DIV(A13_CLK_APB0, "apb0", apb0_parents,
	    CPU_AHB_APB0_CFG_REG,	/* reg */
	    __BITS(9,8),		/* div */
	    0,				/* sel */
	    SUNXI_CCU_DIV_ZERO_IS_ONE | SUNXI_CCU_DIV_POWER_OF_TWO),

	SUNXI_CCU_NM(A13_CLK_APB1, "apb1", apb1_parents,
	    APB1_CLK_DIV_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(4,0),		/* m */
	    __BITS(25,24),		/* sel */
	    0,				/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(A13_CLK_MMC0, "mmc0", mod_parents,
	    SD0_SCLK_CFG_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),
	SUNXI_CCU_NM(A13_CLK_MMC1, "mmc1", mod_parents,
	    SD1_SCLK_CFG_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),
	SUNXI_CCU_NM(A13_CLK_MMC2, "mmc2", mod_parents,
	    SD2_SCLK_CFG_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(A13_CLK_NAND, "nand", mod_parents,
	    NAND_SCLK_CFG_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	/* AHB_GATING_REG0. Missing: SS, EMAC, TS, GPS */
	SUNXI_CCU_GATE(A13_CLK_AHB_OTG, "ahb-otg", "ahb",
	    AHB_GATING_REG0, 0),
	SUNXI_CCU_GATE(A13_CLK_AHB_EHCI, "ahb-ehci", "ahb",
	    AHB_GATING_REG0, 1),
	SUNXI_CCU_GATE(A13_CLK_AHB_OHCI, "ahb-ohci", "ahb",
	    AHB_GATING_REG0, 2),
	SUNXI_CCU_GATE(A13_CLK_AHB_DMA, "ahb-dma", "ahb",
	    AHB_GATING_REG0, 6),
	SUNXI_CCU_GATE(A13_CLK_AHB_BIST, "ahb-bist", "ahb",
	    AHB_GATING_REG0, 7),
	SUNXI_CCU_GATE(A13_CLK_AHB_MMC0, "ahb-mmc0", "ahb",
	    AHB_GATING_REG0, 8),
	SUNXI_CCU_GATE(A13_CLK_AHB_MMC1, "ahb-mmc1", "ahb",
	    AHB_GATING_REG0, 9),
	SUNXI_CCU_GATE(A13_CLK_AHB_MMC2, "ahb-mmc2", "ahb",
	    AHB_GATING_REG0, 10),
	SUNXI_CCU_GATE(A13_CLK_AHB_NAND, "ahb-nand", "ahb",
	    AHB_GATING_REG0, 13),
	SUNXI_CCU_GATE(A13_CLK_AHB_SDRAM, "ahb-sdram", "ahb",
	    AHB_GATING_REG0, 14),
	SUNXI_CCU_GATE(A13_CLK_AHB_SPI0, "ahb-spi0", "ahb",
	    AHB_GATING_REG0, 20),
	SUNXI_CCU_GATE(A13_CLK_AHB_SPI1, "ahb-spi1", "ahb",
	    AHB_GATING_REG0, 21),
	SUNXI_CCU_GATE(A13_CLK_AHB_SPI2, "ahb-spi2", "ahb",
	    AHB_GATING_REG0, 22),
	SUNXI_CCU_GATE(A13_CLK_AHB_HSTIMER, "ahb-hstimer", "ahb",
	    AHB_GATING_REG0, 28),

	/* AHB_GATING_REG1. Missing: TVE, HDMI */
	SUNXI_CCU_GATE(A13_CLK_AHB_VE, "ahb-ve", "ahb",
	    AHB_GATING_REG1, 0),
	SUNXI_CCU_GATE(A13_CLK_AHB_LCD, "ahb-lcd", "ahb",
	    AHB_GATING_REG1, 4),
	SUNXI_CCU_GATE(A13_CLK_AHB_CSI, "ahb-csi", "ahb",
	    AHB_GATING_REG1, 8),
	SUNXI_CCU_GATE(A13_CLK_AHB_DE_BE, "ahb-de_be", "ahb",
	    AHB_GATING_REG1, 12),
	SUNXI_CCU_GATE(A13_CLK_AHB_DE_FE, "ahb-de_fe", "ahb",
	    AHB_GATING_REG1, 14),
	SUNXI_CCU_GATE(A13_CLK_AHB_IEP, "ahb-iep", "ahb",
	    AHB_GATING_REG1, 19),
	SUNXI_CCU_GATE(A13_CLK_AHB_GPU, "ahb-gpu", "ahb",
	    AHB_GATING_REG1, 20),

	/* APB0_GATING_REG. Missing: SPDIF, I2S, KEYPAD */
	SUNXI_CCU_GATE(A13_CLK_APB0_CODEC, "apb0-codec", "apb0",
	    APB0_GATING_REG, 0),
	SUNXI_CCU_GATE(A13_CLK_APB0_PIO, "apb0-pio", "apb0",
	    APB0_GATING_REG, 5),
	SUNXI_CCU_GATE(A13_CLK_APB0_IR, "apb0-ir", "apb0",
	    APB0_GATING_REG, 6),

	/* APB1_GATING_REG. Missing: UART0, UART2 */
	SUNXI_CCU_GATE(A13_CLK_APB1_I2C0, "apb1-i2c0", "apb1",
	    APB1_GATING_REG, 0),
	SUNXI_CCU_GATE(A13_CLK_APB1_I2C1, "apb1-i2c1", "apb1",
	    APB1_GATING_REG, 1),
	SUNXI_CCU_GATE(A13_CLK_APB1_I2C2, "apb1-i2c2", "apb1",
	    APB1_GATING_REG, 2),
	SUNXI_CCU_GATE(A13_CLK_APB1_UART1, "apb1-uart1", "apb1",
	    APB1_GATING_REG, 17),
	SUNXI_CCU_GATE(A13_CLK_APB1_UART3, "apb1-uart3", "apb1",
	    APB1_GATING_REG, 19),

	/* AUDIO_CODEC_SCLK_CFG_REG */
	SUNXI_CCU_GATE(A13_CLK_CODEC, "codec", "pll_audio",
	    AUDIO_CODEC_SCLK_CFG_REG, 31),

	/* USBPHY_CFG_REG */
	SUNXI_CCU_GATE(A13_CLK_USB_OHCI, "usb-ohci", "osc24m",
	    USBPHY_CFG_REG, 6),
	SUNXI_CCU_GATE(A13_CLK_USB_PHY0, "usb-phy0", "osc24m",
	    USBPHY_CFG_REG, 8),
	SUNXI_CCU_GATE(A13_CLK_USB_PHY1, "usb-phy1", "osc24m",
	    USBPHY_CFG_REG, 9),
};

static int
sun5i_a13_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sun5i_a13_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun5i_a13_ccu_resets;
	sc->sc_nresets = __arraycount(sun5i_a13_ccu_resets);

	sc->sc_clks = sun5i_a13_ccu_clks;
	sc->sc_nclks = __arraycount(sun5i_a13_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": A13 CCU\n");

	sunxi_ccu_print(sc);
}
