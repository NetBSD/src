/* $NetBSD: sun50i_h6_ccu.c,v 1.1 2018/05/01 19:53:14 jmcneill Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: sun50i_h6_ccu.c,v 1.1 2018/05/01 19:53:14 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sun50i_h6_ccu.h>

#define	PLL_PERI0_CTRL_REG	0x020
#define	AHB3_CFG_REG		0x51c
#define	APB2_CFG_REG		0x524
#define	MBUS_CFG_REG		0x540
#define	DE_BGR_REG		0x60c
#define	DI_BGR_REG		0x62c
#define	GPU_BGR_REG		0x67c
#define	CE_BGR_REG		0x68c
#define	VE_BGR_REG		0x69c
#define	EMCE_BGR_REG		0x6bc
#define	VP9_BGR_REG		0x6cc
#define	DMA_BGR_REG		0x70c
#define	MSGBOX_BGR_REG		0x71c
#define	SPINLOCK_BGR_REG	0x72c
#define	HSTIMER_BGR_REG		0x73c
#define	DBGSYS_BGR_REG		0x78c
#define	PSI_BGR_REG		0x79c
#define	PWM_BGR_REG		0x7ac
#define	DRAM_CLK_REG		0x800
#define	NAND_BGR_REG		0x82c
#define	SMHC0_CLK_REG		0x830
#define	SMHC1_CLK_REG		0x834
#define	SMHC2_CLK_REG		0x838
#define	SMHC_BGR_REG		0x84c
#define	UART_BGR_REG		0x90c
#define	TWI_BGR_REG		0x91c
#define	SCR_BGR_REG		0x93c
#define	SPI_BGR_REG		0x96c
#define	EMAC_BGR_REG		0x97c
#define	TS_BGR_REG		0x9bc
#define	CIRTX_BGR_REG		0x9cc
#define	THS_BGR_REG		0x9fc
#define	I2S_PCM_BGR_REG		0xa1c
#define	OWA_BGR_REG		0xa2c
#define	DMIC_BGR_REG		0xa4c
#define	AUDIO_HUB_BGR_REG	0xa6c
#define	USB0_CLK_REG		0xa70
#define	USB1_CLK_REG		0xa74
#define	USB3_CLK_REG		0xa7c
#define	USB_BGR_REG		0xa8c
#define	PCIE_BGR_REG		0xabc
#define	HDMI_BGR_REG		0xb1c
#define	DISPLAY_IF_TOP_BGR_REG	0xb5c
#define	TCON_LCD_BGR_REG	0xb7c
#define	TCON_TV_BGR_REG		0xb9c
#define	CSI_BGR_REG		0xc2c
#define	HDMI_HDCP_BGR_REG	0xc4c

static int sun50i_h6_ccu_match(device_t, cfdata_t, void *);
static void sun50i_h6_ccu_attach(device_t, device_t, void *);

static const char * const compatible[] = {
	"allwinner,sun50i-h6-ccu",
	NULL
};

CFATTACH_DECL_NEW(sunxi_h6_ccu, sizeof(struct sunxi_ccu_softc),
	sun50i_h6_ccu_match, sun50i_h6_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun50i_h6_ccu_resets[] = {
	SUNXI_CCU_RESET(H6_RST_MBUS, MBUS_CFG_REG, 30),

	SUNXI_CCU_RESET(H6_RST_BUS_DE, DE_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_DEINTERLACE, DI_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_GPU, GPU_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_CE, CE_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_VE, VE_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_EMCE, EMCE_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_VP9, VP9_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_DMA, DMA_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_MSGBOX, MSGBOX_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_SPINLOCK, SPINLOCK_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_HSTIMER, HSTIMER_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_DBG, DBGSYS_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_PSI, PSI_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_PWM, PWM_BGR_REG, 16),

	/* H6_RST_BUS_IOMMU: No bit defined in user manual */

	SUNXI_CCU_RESET(H6_RST_BUS_DRAM, DRAM_CLK_REG, 30),

	SUNXI_CCU_RESET(H6_RST_BUS_NAND, NAND_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_MMC0, SMHC_BGR_REG, 16),
	SUNXI_CCU_RESET(H6_RST_BUS_MMC1, SMHC_BGR_REG, 17),
	SUNXI_CCU_RESET(H6_RST_BUS_MMC2, SMHC_BGR_REG, 18),

	SUNXI_CCU_RESET(H6_RST_BUS_UART0, UART_BGR_REG, 16),
	SUNXI_CCU_RESET(H6_RST_BUS_UART1, UART_BGR_REG, 17),
	SUNXI_CCU_RESET(H6_RST_BUS_UART2, UART_BGR_REG, 18),
	SUNXI_CCU_RESET(H6_RST_BUS_UART3, UART_BGR_REG, 19),

	SUNXI_CCU_RESET(H6_RST_BUS_I2C0, TWI_BGR_REG, 16),
	SUNXI_CCU_RESET(H6_RST_BUS_I2C1, TWI_BGR_REG, 17),
	SUNXI_CCU_RESET(H6_RST_BUS_I2C2, TWI_BGR_REG, 18),
	SUNXI_CCU_RESET(H6_RST_BUS_I2C3, TWI_BGR_REG, 19),

	SUNXI_CCU_RESET(H6_RST_BUS_SCR0, SCR_BGR_REG, 16),
	SUNXI_CCU_RESET(H6_RST_BUS_SCR1, SCR_BGR_REG, 17),

	SUNXI_CCU_RESET(H6_RST_BUS_SPI0, SPI_BGR_REG, 16),
	SUNXI_CCU_RESET(H6_RST_BUS_SPI1, SPI_BGR_REG, 17),

	SUNXI_CCU_RESET(H6_RST_BUS_EMAC, EMAC_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_TS, TS_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_IR_TX, CIRTX_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_THS, THS_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_I2S0, I2S_PCM_BGR_REG, 16),
	SUNXI_CCU_RESET(H6_RST_BUS_I2S1, I2S_PCM_BGR_REG, 17),
	SUNXI_CCU_RESET(H6_RST_BUS_I2S2, I2S_PCM_BGR_REG, 18),
	SUNXI_CCU_RESET(H6_RST_BUS_I2S3, I2S_PCM_BGR_REG, 19),

	SUNXI_CCU_RESET(H6_RST_BUS_SPDIF, OWA_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_DMIC, DMIC_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_AUDIO_HUB, AUDIO_HUB_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_USB_PHY0, USB0_CLK_REG, 30),

	SUNXI_CCU_RESET(H6_RST_USB_PHY1, USB1_CLK_REG, 30),

	SUNXI_CCU_RESET(H6_RST_USB_PHY3, USB3_CLK_REG, 30),
	SUNXI_CCU_RESET(H6_RST_USB_HSIC, USB3_CLK_REG, 28),

	SUNXI_CCU_RESET(H6_RST_BUS_OHCI0, USB_BGR_REG, 16),
	SUNXI_CCU_RESET(H6_RST_BUS_OHCI3, USB_BGR_REG, 19),
	SUNXI_CCU_RESET(H6_RST_BUS_EHCI0, USB_BGR_REG, 20),
	SUNXI_CCU_RESET(H6_RST_BUS_XHCI, USB_BGR_REG, 21),
	SUNXI_CCU_RESET(H6_RST_BUS_EHCI3, USB_BGR_REG, 23),
	SUNXI_CCU_RESET(H6_RST_BUS_OTG, USB_BGR_REG, 24),

	SUNXI_CCU_RESET(H6_RST_BUS_PCIE, PCIE_BGR_REG, 16),
	SUNXI_CCU_RESET(H6_RST_PCIE_POWERUP, PCIE_BGR_REG, 17),

	SUNXI_CCU_RESET(H6_RST_BUS_HDMI, HDMI_BGR_REG, 16),
	SUNXI_CCU_RESET(H6_RST_BUS_HDMI_SUB, HDMI_BGR_REG, 17),

	SUNXI_CCU_RESET(H6_RST_BUS_TCON_TOP, DISPLAY_IF_TOP_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_TCON_LCD0, TCON_LCD_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_TCON_TV0, TCON_TV_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_CSI, CSI_BGR_REG, 16),

	SUNXI_CCU_RESET(H6_RST_BUS_HDCP, HDMI_HDCP_BGR_REG, 16),
};

static const char *ahb3_parents[] = { "hosc", "losc", "psi", "pll_periph0" };
static const char *apb2_parents[] = { "hosc", "losc", "psi", "pll_periph0" };
static const char *mod_parents[] = { "hosc", "pll_periph0_2x", "pll_periph1_2x" };

static struct sunxi_ccu_clk sun50i_h6_ccu_clks[] = {
	SUNXI_CCU_FIXED_FACTOR(H6_CLK_OSC12M, "osc12m", "hosc", 2, 1),

	SUNXI_CCU_NKMP(H6_CLK_PLL_PERIPH0_4X, "pll_periph0_4x", "hosc",
	    PLL_PERI0_CTRL_REG,		/* reg */
	    __BITS(15,8),		/* n */
	    0,		 		/* k */
	    __BIT(1),			/* m */
	    __BIT(0),			/* p */
	    __BIT(31),			/* enable */
	    0),
	SUNXI_CCU_FIXED_FACTOR(H6_CLK_PLL_PERIPH0_2X, "pll_periph0_2x", "pll_periph0_4x", 2, 1),
	SUNXI_CCU_FIXED_FACTOR(H6_CLK_PLL_PERIPH0, "pll_periph0", "pll_periph0_4x", 4, 1),

	SUNXI_CCU_NM(H6_CLK_AHB3, "ahb3", ahb3_parents,
	    AHB3_CFG_REG,	/* reg */
	    __BITS(9,8),	/* n */
	    __BITS(1,0),	/* m */
	    __BITS(25,24),	/* sel */
	    0,			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(H6_CLK_APB2, "apb2", apb2_parents,
	    APB2_CFG_REG,	/* reg */
	    __BITS(9,8),	/* n */
	    __BITS(1,0),	/* m */
	    __BITS(25,24),	/* sel */
	    0,			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(H6_CLK_MMC0, "mmc0", mod_parents,
	    SMHC0_CLK_REG,	/* reg */
	    __BITS(9,8),	/* n */
	    __BITS(3,0),	/* m */
	    __BITS(25,24),	/* sel */
	    __BIT(31),		/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_NM(H6_CLK_MMC1, "mmc1", mod_parents,
	    SMHC1_CLK_REG,	/* reg */
	    __BITS(9,8),	/* n */
	    __BITS(3,0),	/* m */
	    __BITS(25,24),	/* sel */
	    __BIT(31),		/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_NM(H6_CLK_MMC2, "mmc2", mod_parents,
	    SMHC2_CLK_REG,	/* reg */
	    __BITS(9,8),	/* n */
	    __BITS(3,0),	/* m */
	    __BITS(25,24),	/* sel */
	    __BIT(31),		/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_GATE(H6_CLK_BUS_MMC0, "bus-mmc0", "mmc0",
	    SMHC_BGR_REG, 0),
	SUNXI_CCU_GATE(H6_CLK_BUS_MMC1, "bus-mmc1", "mmc1",
	    SMHC_BGR_REG, 1),
	SUNXI_CCU_GATE(H6_CLK_BUS_MMC2, "bus-mmc2", "mmc2",
	    SMHC_BGR_REG, 2),

	SUNXI_CCU_GATE(H6_CLK_BUS_UART0, "bus-uart0", "apb2",
	    UART_BGR_REG, 0),
	SUNXI_CCU_GATE(H6_CLK_BUS_UART1, "bus-uart1", "apb2",
	    UART_BGR_REG, 1),
	SUNXI_CCU_GATE(H6_CLK_BUS_UART2, "bus-uart2", "apb2",
	    UART_BGR_REG, 2),
	SUNXI_CCU_GATE(H6_CLK_BUS_UART3, "bus-uart3", "apb2",
	    UART_BGR_REG, 3),

	SUNXI_CCU_GATE(H6_CLK_BUS_I2C0, "bus-i2c0", "apb2",
	    TWI_BGR_REG, 0),
	SUNXI_CCU_GATE(H6_CLK_BUS_I2C1, "bus-i2c1", "apb2",
	    TWI_BGR_REG, 1),
	SUNXI_CCU_GATE(H6_CLK_BUS_I2C2, "bus-i2c2", "apb2",
	    TWI_BGR_REG, 2),
	SUNXI_CCU_GATE(H6_CLK_BUS_I2C3, "bus-i2c3", "apb2",
	    TWI_BGR_REG, 3),

	SUNXI_CCU_GATE(H6_CLK_USB_OHCI0, "usb-ohci0", "ahb3",
	    USB0_CLK_REG, 31),
	SUNXI_CCU_GATE(H6_CLK_USB_PHY0, "usb-phy0", "ahb3",
	    USB0_CLK_REG, 29),

	SUNXI_CCU_GATE(H6_CLK_USB_PHY1, "usb-phy1", "ahb3",
	    USB1_CLK_REG, 29),

	SUNXI_CCU_GATE(H6_CLK_USB_OHCI3, "usb-ohci3", "ahb3",
	    USB3_CLK_REG, 31),
	SUNXI_CCU_GATE(H6_CLK_USB_PHY3, "usb-phy3", "ahb3",
	    USB3_CLK_REG, 29),
	SUNXI_CCU_GATE(H6_CLK_USB_HSIC_12M, "usb-hsic-12m", "osc12m",
	    USB3_CLK_REG, 27),
	SUNXI_CCU_GATE(H6_CLK_USB_HSIC, "usb-hsic", "ahb3",
	    USB3_CLK_REG, 26),

	SUNXI_CCU_GATE(H6_CLK_BUS_OHCI0, "bus-ohci0", "ahb3",
	    USB_BGR_REG, 0),
	SUNXI_CCU_GATE(H6_CLK_BUS_OHCI3, "bus-ohci3", "ahb3",
	    USB_BGR_REG, 3),
	SUNXI_CCU_GATE(H6_CLK_BUS_EHCI0, "bus-ehci0", "ahb3",
	    USB_BGR_REG, 4),
	SUNXI_CCU_GATE(H6_CLK_BUS_XHCI, "bus-xhci", "ahb3",
	    USB_BGR_REG, 5),
	SUNXI_CCU_GATE(H6_CLK_BUS_EHCI3, "bus-ehci3", "ahb3",
	    USB_BGR_REG, 7),
	SUNXI_CCU_GATE(H6_CLK_BUS_OTG, "bus-otg", "ahb3",
	    USB_BGR_REG, 8),

	SUNXI_CCU_GATE(H6_CLK_BUS_EMAC, "bus-emac", "ahb3",
	    EMAC_BGR_REG, 0),
};

static int
sun50i_h6_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sun50i_h6_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun50i_h6_ccu_resets;
	sc->sc_nresets = __arraycount(sun50i_h6_ccu_resets);

	sc->sc_clks = sun50i_h6_ccu_clks;
	sc->sc_nclks = __arraycount(sun50i_h6_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": H6 CCU\n");

	sunxi_ccu_print(sc);
}
