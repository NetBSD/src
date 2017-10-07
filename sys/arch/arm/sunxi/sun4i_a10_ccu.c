/* $NetBSD: sun4i_a10_ccu.c,v 1.2 2017/10/07 12:22:29 jmcneill Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: sun4i_a10_ccu.c,v 1.2 2017/10/07 12:22:29 jmcneill Exp $");

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
#define	PLL6_CFG_REG		0x028
#define	OSC24M_CFG_REG		0x050
#define	CPU_AHB_APB0_CFG_REG	0x054
#define	APB1_CLK_DIV_REG	0x058
#define	AHB_GATING_REG0		0x060
#define	AHB_GATING_REG1		0x064
#define	APB0_GATING_REG		0x068
#define	APB1_GATING_REG		0x06c
#define	SD0_SCLK_CFG_REG        0x088
#define	SD1_SCLK_CFG_REG        0x08c
#define	SD2_SCLK_CFG_REG        0x090
#define	SD3_SCLK_CFG_REG	0x094
#define	USBPHY_CFG_REG		0x0cc
#define	BE_CFG_REG		0x104
#define	FE_CFG_REG		0x10c
#define	CSI_CFG_REG		0x134
#define	VE_CFG_REG		0x13c
#define	AUDIO_CODEC_SCLK_CFG_REG 0x140
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
};

static const char *cpu_parents[] = { "losc", "osc24m", "pll_core", "pll_periph" };
static const char *axi_parents[] = { "cpu" };
static const char *ahb_parents[] = { "axi", "pll_periph", "pll_periph_base" };
static const char *apb0_parents[] = { "ahb" };
static const char *apb1_parents[] = { "osc24m", "pll_periph", "losc" };
static const char *mod_parents[] = { "osc24m", "pll_periph", "pll_ddr" };

static const struct sunxi_ccu_nkmp_tbl sun4i_a10_ac_dig_table[] = {
	{ 24576000, 86, 0, 21, 3 },
	{ 0 }
};

static struct sunxi_ccu_clk sun4i_a10_ccu_clks[] = {
	SUNXI_CCU_GATE(A10_CLK_HOSC, "osc24m", "hosc",
	    OSC24M_CFG_REG, 0),

	SUNXI_CCU_NKMP(A10_CLK_PLL_CORE, "pll_core", "osc24m",
	    PLL1_CFG_REG,		/* reg */
	    __BITS(12,8),		/* n */
	    __BITS(5,4), 		/* k */
	    __BITS(1,0),		/* m */
	    __BITS(17,16),		/* p */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NKMP_FACTOR_P_POW2 | SUNXI_CCU_NKMP_FACTOR_N_EXACT |
	    SUNXI_CCU_NKMP_FACTOR_N_ZERO_IS_ONE),

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

	SUNXI_CCU_DIV(A10_CLK_CPU, "cpu", cpu_parents,
	    CPU_AHB_APB0_CFG_REG,	/* reg */
	    0,				/* div */
	    __BITS(17,16),		/* sel */
	    0),

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

static int
sun4i_a10_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sun4i_a10_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	enum sun4i_a10_ccu_type type;

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

	sunxi_ccu_print(sc);
}
