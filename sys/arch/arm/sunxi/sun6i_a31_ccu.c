/* $NetBSD: sun6i_a31_ccu.c,v 1.4 2017/10/07 21:52:53 jmcneill Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
 * Copyright (c) 2017 Emmanuel Vadot <manu@freebsd.org>
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

__KERNEL_RCSID(1, "$NetBSD: sun6i_a31_ccu.c,v 1.4 2017/10/07 21:52:53 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sun6i_a31_ccu.h>

#define	PLL2_CFG_REG		0x008
#define	PLL_PERIPH_CTRL_REG	0x028
#define	AHB1_APB1_CFG_REG	0x054
#define	APB2_CLK_DIV_REG	0x058
#define	AHB1_GATING_REG0	0x060
#define	AHB1_GATING_REG1	0x064
#define	APB1_GATING_REG		0x068
#define	APB2_GATING_REG		0x06c
#define	SD0_CLK_REG		0x088
#define	SD1_CLK_REG		0x08c
#define	SD2_CLK_REG		0x090
#define	SD3_CLK_REG		0x094
#define	USBPHY_CFG_REG		0x0cc
#define	AUDIO_CODEC_CLK_REG	0x140
#define	BUS_SOFT_RST_REG0	0x2c0
#define	BUS_SOFT_RST_REG1	0x2c4
#define	BUS_SOFT_RST_REG2	0x2c8
#define	BUS_SOFT_RST_REG3	0x2d0
#define	BUS_SOFT_RST_REG4	0x2d8

static int sun6i_a31_ccu_match(device_t, cfdata_t, void *);
static void sun6i_a31_ccu_attach(device_t, device_t, void *);

static const char * const compatible[] = {
	"allwinner,sun6i-a31-ccu",
	NULL
};

CFATTACH_DECL_NEW(sunxi_a31_ccu, sizeof(struct sunxi_ccu_softc),
	sun6i_a31_ccu_match, sun6i_a31_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun6i_a31_ccu_resets[] = {
	SUNXI_CCU_RESET(A31_RST_USB_PHY0, USBPHY_CFG_REG, 0),
	SUNXI_CCU_RESET(A31_RST_USB_PHY1, USBPHY_CFG_REG, 1),
	SUNXI_CCU_RESET(A31_RST_USB_PHY2, USBPHY_CFG_REG, 2),

	SUNXI_CCU_RESET(A31_RST_AHB1_MIPI_DSI, BUS_SOFT_RST_REG0, 1),
	SUNXI_CCU_RESET(A31_RST_AHB1_SS, BUS_SOFT_RST_REG0, 5),
	SUNXI_CCU_RESET(A31_RST_AHB1_DMA, BUS_SOFT_RST_REG0, 6),
	SUNXI_CCU_RESET(A31_RST_AHB1_MMC0, BUS_SOFT_RST_REG0, 8),
	SUNXI_CCU_RESET(A31_RST_AHB1_MMC1, BUS_SOFT_RST_REG0, 9),
	SUNXI_CCU_RESET(A31_RST_AHB1_MMC2, BUS_SOFT_RST_REG0, 10),
	SUNXI_CCU_RESET(A31_RST_AHB1_MMC3, BUS_SOFT_RST_REG0, 11),
	SUNXI_CCU_RESET(A31_RST_AHB1_NAND1, BUS_SOFT_RST_REG0, 12),
	SUNXI_CCU_RESET(A31_RST_AHB1_NAND0, BUS_SOFT_RST_REG0, 13),
	SUNXI_CCU_RESET(A31_RST_AHB1_SDRAM, BUS_SOFT_RST_REG0, 14),
	SUNXI_CCU_RESET(A31_RST_AHB1_EMAC, BUS_SOFT_RST_REG0, 17),
	SUNXI_CCU_RESET(A31_RST_AHB1_TS, BUS_SOFT_RST_REG0, 18),
	SUNXI_CCU_RESET(A31_RST_AHB1_HSTIMER, BUS_SOFT_RST_REG0, 19),
	SUNXI_CCU_RESET(A31_RST_AHB1_SPI0, BUS_SOFT_RST_REG0, 20),
	SUNXI_CCU_RESET(A31_RST_AHB1_SPI1, BUS_SOFT_RST_REG0, 21),
	SUNXI_CCU_RESET(A31_RST_AHB1_SPI2, BUS_SOFT_RST_REG0, 22),
	SUNXI_CCU_RESET(A31_RST_AHB1_SPI3, BUS_SOFT_RST_REG0, 23),
	SUNXI_CCU_RESET(A31_RST_AHB1_OTG, BUS_SOFT_RST_REG0, 24),
	SUNXI_CCU_RESET(A31_RST_AHB1_EHCI0, BUS_SOFT_RST_REG0, 26),
	SUNXI_CCU_RESET(A31_RST_AHB1_EHCI1, BUS_SOFT_RST_REG0, 27),
	SUNXI_CCU_RESET(A31_RST_AHB1_OHCI0, BUS_SOFT_RST_REG0, 29),
	SUNXI_CCU_RESET(A31_RST_AHB1_OHCI1, BUS_SOFT_RST_REG0, 30),
	SUNXI_CCU_RESET(A31_RST_AHB1_OHCI2, BUS_SOFT_RST_REG0, 31),
        
	SUNXI_CCU_RESET(A31_RST_AHB1_VE, BUS_SOFT_RST_REG1, 0),
	SUNXI_CCU_RESET(A31_RST_AHB1_LCD0, BUS_SOFT_RST_REG1, 4),
	SUNXI_CCU_RESET(A31_RST_AHB1_LCD1, BUS_SOFT_RST_REG1, 5),
	SUNXI_CCU_RESET(A31_RST_AHB1_CSI, BUS_SOFT_RST_REG1, 8),
	SUNXI_CCU_RESET(A31_RST_AHB1_HDMI, BUS_SOFT_RST_REG1, 11),
	SUNXI_CCU_RESET(A31_RST_AHB1_BE0, BUS_SOFT_RST_REG1, 12),
	SUNXI_CCU_RESET(A31_RST_AHB1_BE1, BUS_SOFT_RST_REG1, 13),
	SUNXI_CCU_RESET(A31_RST_AHB1_FE0, BUS_SOFT_RST_REG1, 14),
	SUNXI_CCU_RESET(A31_RST_AHB1_FE1, BUS_SOFT_RST_REG1, 15),
	SUNXI_CCU_RESET(A31_RST_AHB1_MP, BUS_SOFT_RST_REG1, 16),
	SUNXI_CCU_RESET(A31_RST_AHB1_GPU, BUS_SOFT_RST_REG1, 20),
	SUNXI_CCU_RESET(A31_RST_AHB1_DEU0, BUS_SOFT_RST_REG1, 23),
	SUNXI_CCU_RESET(A31_RST_AHB1_DEU1, BUS_SOFT_RST_REG1, 24),
	SUNXI_CCU_RESET(A31_RST_AHB1_DRC0, BUS_SOFT_RST_REG1, 25),
	SUNXI_CCU_RESET(A31_RST_AHB1_DRC1, BUS_SOFT_RST_REG1, 26),

	SUNXI_CCU_RESET(A31_RST_AHB1_LVDS, BUS_SOFT_RST_REG2, 0),

	SUNXI_CCU_RESET(A31_RST_APB1_CODEC, BUS_SOFT_RST_REG3, 0),
	SUNXI_CCU_RESET(A31_RST_APB1_SPDIF, BUS_SOFT_RST_REG3, 1),
	SUNXI_CCU_RESET(A31_RST_APB1_DIGITAL_MIC, BUS_SOFT_RST_REG3, 4),
	SUNXI_CCU_RESET(A31_RST_APB1_DAUDIO0, BUS_SOFT_RST_REG3, 12),
	SUNXI_CCU_RESET(A31_RST_APB1_DAUDIO1, BUS_SOFT_RST_REG3, 13),

	SUNXI_CCU_RESET(A31_RST_APB2_I2C0, BUS_SOFT_RST_REG4, 0),
	SUNXI_CCU_RESET(A31_RST_APB2_I2C1, BUS_SOFT_RST_REG4, 1),
	SUNXI_CCU_RESET(A31_RST_APB2_I2C2, BUS_SOFT_RST_REG4, 2),
	SUNXI_CCU_RESET(A31_RST_APB2_I2C3, BUS_SOFT_RST_REG4, 3),
	SUNXI_CCU_RESET(A31_RST_APB2_UART0, BUS_SOFT_RST_REG4, 16),
	SUNXI_CCU_RESET(A31_RST_APB2_UART1, BUS_SOFT_RST_REG4, 17),
	SUNXI_CCU_RESET(A31_RST_APB2_UART2, BUS_SOFT_RST_REG4, 18),
	SUNXI_CCU_RESET(A31_RST_APB2_UART3, BUS_SOFT_RST_REG4, 19),
	SUNXI_CCU_RESET(A31_RST_APB2_UART4, BUS_SOFT_RST_REG4, 20),
	SUNXI_CCU_RESET(A31_RST_APB2_UART5, BUS_SOFT_RST_REG4, 21),
};

static const char *ahb1_parents[] = { "losc", "hosc", "axi", "pll_periph" };
static const char *apb1_parents[] = { "ahb1" };
static const char *apb2_parents[] = { "losc", "hosc", "pll_periph", "pll_periph" };
static const char *mod_parents[] = { "hosc", "pll_periph" };

static const struct sunxi_ccu_nkmp_tbl sun6i_a31_pll_audio_table[] = {
	{ 24576000, 85, 0, 20, 3 },
	{ 0 }
};

static struct sunxi_ccu_clk sun6i_a31_ccu_clks[] = {
	SUNXI_CCU_NKMP_TABLE(A31_CLK_PLL_AUDIO_BASE, "pll_audio", "hosc",
	    PLL2_CFG_REG,		/* reg */
	    __BITS(14,8),		/* n */
	    0,				/* k */
	    __BITS(4,0),		/* m */
	    __BITS(19,16),		/* p */
	    __BIT(31),			/* enable */
	    __BIT(28),			/* lock */
	    sun6i_a31_pll_audio_table,	/* table */
	    0),

	SUNXI_CCU_NKMP(A31_CLK_PLL_PERIPH, "pll_periph", "hosc",
	    PLL_PERIPH_CTRL_REG,	/* reg */
	    __BITS(12,8),		/* n */
	    __BITS(5,4), 		/* k */
	    0,				/* m */
	    0,				/* p */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NKMP_DIVIDE_BY_TWO),

	SUNXI_CCU_DIV(A31_CLK_APB1, "apb1", apb1_parents,
	    AHB1_APB1_CFG_REG,	/* reg */
	    __BITS(9,8),	/* div */
	    0,			/* sel */
	    SUNXI_CCU_DIV_POWER_OF_TWO|SUNXI_CCU_DIV_ZERO_IS_ONE),

	SUNXI_CCU_PREDIV(A31_CLK_AHB1, "ahb1", ahb1_parents,
	    AHB1_APB1_CFG_REG,	/* reg */
	    __BITS(7,6),	/* prediv */
	    __BIT(3),		/* prediv_sel */
	    __BITS(5,4),	/* div */
	    __BITS(13,12),	/* sel */
	    SUNXI_CCU_PREDIV_POWER_OF_TWO),

	SUNXI_CCU_NM(A31_CLK_APB2, "apb2", apb2_parents,
	    APB2_CLK_DIV_REG,	/* reg */
	    __BITS(17,16),	/* n */
	    __BITS(4,0),	/* m */
	    __BITS(25,24),	/* sel */
	    0,			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(A31_CLK_MMC0, "mmc0", mod_parents,
	    SD0_CLK_REG, __BITS(17, 16), __BITS(3,0), __BITS(25, 24), __BIT(31),
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_NM(A31_CLK_MMC1, "mmc1", mod_parents,
	    SD1_CLK_REG, __BITS(17, 16), __BITS(3,0), __BITS(25, 24), __BIT(31),
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_NM(A31_CLK_MMC2, "mmc2", mod_parents,
	    SD2_CLK_REG, __BITS(17, 16), __BITS(3,0), __BITS(25, 24), __BIT(31),
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_NM(A31_CLK_MMC3, "mmc3", mod_parents,
	    SD3_CLK_REG, __BITS(17, 16), __BITS(3,0), __BITS(25, 24), __BIT(31),
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),

	SUNXI_CCU_GATE(A31_CLK_AHB1_DMA, "ahb1-dma", "ahb1",
	    AHB1_GATING_REG0, 6),
	SUNXI_CCU_GATE(A31_CLK_AHB1_MMC0, "ahb1-mmc0", "ahb1",
	    AHB1_GATING_REG0, 8),
	SUNXI_CCU_GATE(A31_CLK_AHB1_MMC1, "ahb1-mmc1", "ahb1",
	    AHB1_GATING_REG0, 9),
	SUNXI_CCU_GATE(A31_CLK_AHB1_MMC2, "ahb1-mmc2", "ahb1",
	    AHB1_GATING_REG0, 10),
	SUNXI_CCU_GATE(A31_CLK_AHB1_MMC3, "ahb1-mmc3", "ahb1",
	    AHB1_GATING_REG0, 11),
	SUNXI_CCU_GATE(A31_CLK_AHB1_EMAC, "ahb1-emac", "ahb1",
	    AHB1_GATING_REG0, 17),
	SUNXI_CCU_GATE(A31_CLK_AHB1_OTG, "ahb1-otg", "ahb1",
	    AHB1_GATING_REG0, 24),
	SUNXI_CCU_GATE(A31_CLK_AHB1_EHCI0, "ahb1-ehci0", "ahb1",
	    AHB1_GATING_REG0, 26),
	SUNXI_CCU_GATE(A31_CLK_AHB1_EHCI1, "ahb1-ehci1", "ahb1",
	    AHB1_GATING_REG0, 27),
	SUNXI_CCU_GATE(A31_CLK_AHB1_OHCI0, "ahb1-ohci0", "ahb1",
	    AHB1_GATING_REG0, 29),
	SUNXI_CCU_GATE(A31_CLK_AHB1_OHCI1, "ahb1-ohci1", "ahb1",
	    AHB1_GATING_REG0, 30),
	SUNXI_CCU_GATE(A31_CLK_AHB1_OHCI2, "ahb1-ohci2", "ahb1",
	    AHB1_GATING_REG0, 31),

	SUNXI_CCU_GATE(A31_CLK_APB1_CODEC, "apb1-codec", "apb1",
	    APB1_GATING_REG, 0),
	SUNXI_CCU_GATE(A31_CLK_APB1_PIO, "apb1-pio", "apb1",
	    APB1_GATING_REG, 5),

	SUNXI_CCU_GATE(A31_CLK_APB2_I2C0, "apb2-i2c0", "apb2",
	    APB2_GATING_REG, 0),
	SUNXI_CCU_GATE(A31_CLK_APB2_I2C1, "apb2-i2c1", "apb2",
	    APB2_GATING_REG, 1),
	SUNXI_CCU_GATE(A31_CLK_APB2_I2C2, "apb2-i2c2", "apb2",
	    APB2_GATING_REG, 2),
	SUNXI_CCU_GATE(A31_CLK_APB2_I2C3, "apb2-i2c3", "apb2",
	    APB2_GATING_REG, 3),
	SUNXI_CCU_GATE(A31_CLK_APB2_UART0, "apb2-uart0", "apb2",
	    APB2_GATING_REG, 16),
	SUNXI_CCU_GATE(A31_CLK_APB2_UART1, "apb2-uart1", "apb2",
	    APB2_GATING_REG, 17),
	SUNXI_CCU_GATE(A31_CLK_APB2_UART2, "apb2-uart2", "apb2",
	    APB2_GATING_REG, 18),
	SUNXI_CCU_GATE(A31_CLK_APB2_UART3, "apb2-uart3", "apb2",
	    APB2_GATING_REG, 19),
	SUNXI_CCU_GATE(A31_CLK_APB2_UART4, "apb2-uart4", "apb2",
	    APB2_GATING_REG, 20),
	SUNXI_CCU_GATE(A31_CLK_APB2_UART5, "apb2-uart5", "apb2",
	    APB2_GATING_REG, 21),

	SUNXI_CCU_GATE(A31_CLK_USB_PHY0, "usb-phy0", "hosc",
	    USBPHY_CFG_REG, 8),
	SUNXI_CCU_GATE(A31_CLK_USB_PHY1, "usb-phy1", "hosc",
	    USBPHY_CFG_REG, 9),
	SUNXI_CCU_GATE(A31_CLK_USB_PHY2, "usb-phy2", "hosc",
	    USBPHY_CFG_REG, 10),
	SUNXI_CCU_GATE(A31_CLK_USB_OHCI0, "usb-ohci0", "hosc",
	    USBPHY_CFG_REG, 16),
	SUNXI_CCU_GATE(A31_CLK_USB_OHCI1, "usb-ohci1", "hosc",
	    USBPHY_CFG_REG, 17),
	SUNXI_CCU_GATE(A31_CLK_USB_OHCI2, "usb-ohci2", "hosc",
	    USBPHY_CFG_REG, 18),

	SUNXI_CCU_GATE(A31_CLK_CODEC, "codec", "pll_audio",
	    AUDIO_CODEC_CLK_REG, 31),
};

static int
sun6i_a31_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sun6i_a31_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun6i_a31_ccu_resets;
	sc->sc_nresets = __arraycount(sun6i_a31_ccu_resets);

	sc->sc_clks = sun6i_a31_ccu_clks;
	sc->sc_nclks = __arraycount(sun6i_a31_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": A31 CCU\n");

	sunxi_ccu_print(sc);
}
