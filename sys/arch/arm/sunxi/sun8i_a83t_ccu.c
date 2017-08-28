/* $NetBSD: sun8i_a83t_ccu.c,v 1.1.6.2 2017/08/28 17:51:32 skrll Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: sun8i_a83t_ccu.c,v 1.1.6.2 2017/08/28 17:51:32 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sun8i_a83t_ccu.h>

#define	PLL_PERIPH0_CTRL_REG	0x028
#define	AHB1_APB1_CFG_REG	0x054
#define	APB2_CFG_REG		0x058
#define	BUS_CLK_GATING_REG0	0x060
#define	BUS_CLK_GATING_REG2	0x068
#define	BUS_CLK_GATING_REG3	0x06c
#define	SDMMC0_CLK_REG		0x088
#define	SDMMC1_CLK_REG		0x08c
#define	SDMMC2_CLK_REG		0x090
#define	USBPHY_CFG_REG		0x0cc
#define	MBUS_RST_REG		0x0fc
#define	BUS_SOFT_RST_REG0	0x2c0
#define	BUS_SOFT_RST_REG1	0x2c4
#define	BUS_SOFT_RST_REG2	0x2c8
#define	BUS_SOFT_RST_REG3	0x2d0
#define	BUS_SOFT_RST_REG4	0x2d8

static int sun8i_a83t_ccu_match(device_t, cfdata_t, void *);
static void sun8i_a83t_ccu_attach(device_t, device_t, void *);

static const char * const compatible[] = {
	"allwinner,sun8i-a83t-ccu",
	NULL
};

CFATTACH_DECL_NEW(sunxi_a83t_ccu, sizeof(struct sunxi_ccu_softc),
	sun8i_a83t_ccu_match, sun8i_a83t_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun8i_a83t_ccu_resets[] = {
	SUNXI_CCU_RESET(H3_RST_USB_PHY0, USBPHY_CFG_REG, 0),
	SUNXI_CCU_RESET(H3_RST_USB_PHY1, USBPHY_CFG_REG, 1),
	SUNXI_CCU_RESET(H3_RST_USB_PHY2, USBPHY_CFG_REG, 2),
	SUNXI_CCU_RESET(H3_RST_USB_PHY3, USBPHY_CFG_REG, 3),

	SUNXI_CCU_RESET(H3_RST_MBUS, MBUS_RST_REG, 31),

	SUNXI_CCU_RESET(H3_RST_BUS_CE, BUS_SOFT_RST_REG0, 5),
	SUNXI_CCU_RESET(H3_RST_BUS_DMA, BUS_SOFT_RST_REG0, 6),
	SUNXI_CCU_RESET(H3_RST_BUS_MMC0, BUS_SOFT_RST_REG0, 8),
	SUNXI_CCU_RESET(H3_RST_BUS_MMC1, BUS_SOFT_RST_REG0, 9),
	SUNXI_CCU_RESET(H3_RST_BUS_MMC2, BUS_SOFT_RST_REG0, 10),
	SUNXI_CCU_RESET(H3_RST_BUS_NAND, BUS_SOFT_RST_REG0, 13),
	SUNXI_CCU_RESET(H3_RST_BUS_DRAM, BUS_SOFT_RST_REG0, 14),
	SUNXI_CCU_RESET(H3_RST_BUS_EMAC, BUS_SOFT_RST_REG0, 17),
	SUNXI_CCU_RESET(H3_RST_BUS_TS, BUS_SOFT_RST_REG0, 18),
	SUNXI_CCU_RESET(H3_RST_BUS_HSTIMER, BUS_SOFT_RST_REG0, 19),
	SUNXI_CCU_RESET(H3_RST_BUS_SPI0, BUS_SOFT_RST_REG0, 20),
	SUNXI_CCU_RESET(H3_RST_BUS_SPI1, BUS_SOFT_RST_REG0, 21),
	SUNXI_CCU_RESET(H3_RST_BUS_OTG, BUS_SOFT_RST_REG0, 23),
	SUNXI_CCU_RESET(H3_RST_BUS_EHCI0, BUS_SOFT_RST_REG0, 24),
	SUNXI_CCU_RESET(H3_RST_BUS_EHCI1, BUS_SOFT_RST_REG0, 25),
	SUNXI_CCU_RESET(H3_RST_BUS_EHCI2, BUS_SOFT_RST_REG0, 26),
	SUNXI_CCU_RESET(H3_RST_BUS_EHCI3, BUS_SOFT_RST_REG0, 27),
	SUNXI_CCU_RESET(H3_RST_BUS_OHCI0, BUS_SOFT_RST_REG0, 28),
	SUNXI_CCU_RESET(H3_RST_BUS_OHCI1, BUS_SOFT_RST_REG0, 29),
	SUNXI_CCU_RESET(H3_RST_BUS_OHCI2, BUS_SOFT_RST_REG0, 30),
	SUNXI_CCU_RESET(H3_RST_BUS_OHCI3, BUS_SOFT_RST_REG0, 31),
        
	SUNXI_CCU_RESET(H3_RST_BUS_VE, BUS_SOFT_RST_REG1, 0),
	SUNXI_CCU_RESET(H3_RST_BUS_TCON0, BUS_SOFT_RST_REG1, 3),
	SUNXI_CCU_RESET(H3_RST_BUS_TCON1, BUS_SOFT_RST_REG1, 4),
	SUNXI_CCU_RESET(H3_RST_BUS_DEINTERLACE, BUS_SOFT_RST_REG1, 5),
	SUNXI_CCU_RESET(H3_RST_BUS_CSI, BUS_SOFT_RST_REG1, 8),
	SUNXI_CCU_RESET(H3_RST_BUS_TVE, BUS_SOFT_RST_REG1, 9),
	SUNXI_CCU_RESET(H3_RST_BUS_HDMI0, BUS_SOFT_RST_REG1, 10),
	SUNXI_CCU_RESET(H3_RST_BUS_HDMI1, BUS_SOFT_RST_REG1, 11),
	SUNXI_CCU_RESET(H3_RST_BUS_DE, BUS_SOFT_RST_REG1, 12),
	SUNXI_CCU_RESET(H3_RST_BUS_GPU, BUS_SOFT_RST_REG1, 20),
	SUNXI_CCU_RESET(H3_RST_BUS_MSGBOX, BUS_SOFT_RST_REG1, 21),
	SUNXI_CCU_RESET(H3_RST_BUS_SPINLOCK, BUS_SOFT_RST_REG1, 22),
	SUNXI_CCU_RESET(H3_RST_BUS_DBG, BUS_SOFT_RST_REG1, 31),

	SUNXI_CCU_RESET(H3_RST_BUS_EPHY, BUS_SOFT_RST_REG2, 2),

	SUNXI_CCU_RESET(H3_RST_BUS_CODEC, BUS_SOFT_RST_REG3, 0),
	SUNXI_CCU_RESET(H3_RST_BUS_SPDIF, BUS_SOFT_RST_REG3, 1),
	SUNXI_CCU_RESET(H3_RST_BUS_THS, BUS_SOFT_RST_REG3, 8),
	SUNXI_CCU_RESET(H3_RST_BUS_I2S0, BUS_SOFT_RST_REG3, 12),
	SUNXI_CCU_RESET(H3_RST_BUS_I2S1, BUS_SOFT_RST_REG3, 13),
	SUNXI_CCU_RESET(H3_RST_BUS_I2S2, BUS_SOFT_RST_REG3, 14),

	SUNXI_CCU_RESET(H3_RST_BUS_I2C0, BUS_SOFT_RST_REG4, 0),
	SUNXI_CCU_RESET(H3_RST_BUS_I2C1, BUS_SOFT_RST_REG4, 1),
	SUNXI_CCU_RESET(H3_RST_BUS_I2C2, BUS_SOFT_RST_REG4, 2),
	SUNXI_CCU_RESET(H3_RST_BUS_UART0, BUS_SOFT_RST_REG4, 16),
	SUNXI_CCU_RESET(H3_RST_BUS_UART1, BUS_SOFT_RST_REG4, 17),
	SUNXI_CCU_RESET(H3_RST_BUS_UART2, BUS_SOFT_RST_REG4, 18),
	SUNXI_CCU_RESET(H3_RST_BUS_UART3, BUS_SOFT_RST_REG4, 19),
	SUNXI_CCU_RESET(H3_RST_BUS_SCR, BUS_SOFT_RST_REG4, 20),
};

static const char *ahb1_parents[] = { "losc", "hosc", "pll_periph" };
static const char *ahb2_parents[] = { "ahb1", "pll_periph" };
static const char *apb1_parents[] = { "ahb1" };
static const char *apb2_parents[] = { "losc", "hosc", "pll_periph" };
static const char *mod_parents[] = { "hosc", "pll_periph" };

static struct sunxi_ccu_clk sun8i_a83t_ccu_clks[] = {
	SUNXI_CCU_NKMP(H3_CLK_PLL_PERIPH0, "pll_periph", "hosc",
	    PLL_PERIPH0_CTRL_REG,	/* reg */
	    __BITS(15,8),		/* n */
	    0,		 		/* k */
	    __BIT(18),			/* m */
	    __BIT(16),			/* p */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NKMP_FACTOR_N_EXACT),

	SUNXI_CCU_PREDIV(H3_CLK_AHB1, "ahb1", ahb1_parents,
	    AHB1_APB1_CFG_REG,	/* reg */
	    __BITS(7,6),	/* prediv */
	    __BIT(3),		/* prediv_sel */
	    __BITS(5,4),	/* div */
	    __BITS(13,12),	/* sel */
	    SUNXI_CCU_PREDIV_POWER_OF_TWO),

	SUNXI_CCU_PREDIV(H3_CLK_AHB2, "ahb2", ahb2_parents,
	    APB2_CFG_REG,	/* reg */
	    0,			/* prediv */
	    __BIT(1),		/* prediv_sel */
	    0,			/* div */
	    __BITS(1,0),	/* sel */
	    SUNXI_CCU_PREDIV_DIVIDE_BY_TWO),

	SUNXI_CCU_DIV(H3_CLK_APB1, "apb1", apb1_parents,
	    AHB1_APB1_CFG_REG,	/* reg */
	    __BITS(9,8),	/* div */
	    0,			/* sel */
	    SUNXI_CCU_DIV_POWER_OF_TWO|SUNXI_CCU_DIV_ZERO_IS_ONE),

	SUNXI_CCU_NM(H3_CLK_APB2, "apb2", apb2_parents,
	    APB2_CFG_REG,	/* reg */
	    __BITS(17,16),	/* n */
	    __BITS(4,0),	/* m */
	    __BITS(25,24),	/* sel */
	    0,			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(H3_CLK_MMC0, "mmc0", mod_parents,
	    SDMMC0_CLK_REG, __BITS(17, 16), __BITS(3,0), __BITS(25, 24), __BIT(31),
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_NM(H3_CLK_MMC1, "mmc1", mod_parents,
	    SDMMC1_CLK_REG, __BITS(17, 16), __BITS(3,0), __BITS(25, 24), __BIT(31),
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_NM(H3_CLK_MMC2, "mmc2", mod_parents,
	    SDMMC2_CLK_REG, __BITS(17, 16), __BITS(3,0), __BITS(25, 24), __BIT(31),
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),

	SUNXI_CCU_GATE(H3_CLK_BUS_MMC0, "bus-mmc0", "ahb1",
	    BUS_CLK_GATING_REG0, 8),
	SUNXI_CCU_GATE(H3_CLK_BUS_MMC1, "bus-mmc1", "ahb1",
	    BUS_CLK_GATING_REG0, 9),
	SUNXI_CCU_GATE(H3_CLK_BUS_MMC2, "bus-mmc2", "ahb1",
	    BUS_CLK_GATING_REG0, 10),
	SUNXI_CCU_GATE(H3_CLK_BUS_EMAC, "bus-emac", "ahb2",
	    BUS_CLK_GATING_REG0, 17),
	SUNXI_CCU_GATE(H3_CLK_BUS_OTG, "bus-otg", "ahb1",
	    BUS_CLK_GATING_REG0, 23),
	SUNXI_CCU_GATE(H3_CLK_BUS_EHCI0, "bus-ehci0", "ahb1",
	    BUS_CLK_GATING_REG0, 24),
	SUNXI_CCU_GATE(H3_CLK_BUS_EHCI1, "bus-ehci1", "ahb2",
	    BUS_CLK_GATING_REG0, 25),
	SUNXI_CCU_GATE(H3_CLK_BUS_EHCI2, "bus-ehci2", "ahb2",
	    BUS_CLK_GATING_REG0, 26),
	SUNXI_CCU_GATE(H3_CLK_BUS_EHCI3, "bus-ehci3", "ahb2",
	    BUS_CLK_GATING_REG0, 27),
	SUNXI_CCU_GATE(H3_CLK_BUS_OHCI0, "bus-ohci0", "ahb1",
	    BUS_CLK_GATING_REG0, 28),
	SUNXI_CCU_GATE(H3_CLK_BUS_OHCI1, "bus-ohci1", "ahb2",
	    BUS_CLK_GATING_REG0, 29),
	SUNXI_CCU_GATE(H3_CLK_BUS_OHCI2, "bus-ohci2", "ahb2",
	    BUS_CLK_GATING_REG0, 30),
	SUNXI_CCU_GATE(H3_CLK_BUS_OHCI3, "bus-ohci3", "ahb2",
	    BUS_CLK_GATING_REG0, 31),

	SUNXI_CCU_GATE(H3_CLK_BUS_PIO, "bus-pio", "apb1",
	    BUS_CLK_GATING_REG2, 5),

	SUNXI_CCU_GATE(H3_CLK_BUS_I2C0, "bus-i2c0", "apb2",
	    BUS_CLK_GATING_REG3, 0),
	SUNXI_CCU_GATE(H3_CLK_BUS_I2C1, "bus-i2c1", "apb2",
	    BUS_CLK_GATING_REG3, 1),
	SUNXI_CCU_GATE(H3_CLK_BUS_I2C2, "bus-i2c2", "apb2",
	    BUS_CLK_GATING_REG3, 2),
	SUNXI_CCU_GATE(H3_CLK_BUS_UART0, "bus-uart0", "apb2",
	    BUS_CLK_GATING_REG3, 16),
	SUNXI_CCU_GATE(H3_CLK_BUS_UART1, "bus-uart1", "apb2",
	    BUS_CLK_GATING_REG3, 17),
	SUNXI_CCU_GATE(H3_CLK_BUS_UART2, "bus-uart2", "apb2",
	    BUS_CLK_GATING_REG3, 18),
	SUNXI_CCU_GATE(H3_CLK_BUS_UART3, "bus-uart3", "apb2",
	    BUS_CLK_GATING_REG3, 19),

	SUNXI_CCU_GATE(H3_CLK_USBPHY0, "usb-phy0", "hosc",
	    USBPHY_CFG_REG, 8),
	SUNXI_CCU_GATE(H3_CLK_USBPHY1, "usb-phy1", "hosc",
	    USBPHY_CFG_REG, 9),
	SUNXI_CCU_GATE(H3_CLK_USBPHY2, "usb-phy2", "hosc",
	    USBPHY_CFG_REG, 10),
	SUNXI_CCU_GATE(H3_CLK_USBPHY3, "usb-phy3", "hosc",
	    USBPHY_CFG_REG, 11),
	SUNXI_CCU_GATE(H3_CLK_USBOHCI0, "usb-ohci0", "hosc",
	    USBPHY_CFG_REG, 16),
	SUNXI_CCU_GATE(H3_CLK_USBOHCI1, "usb-ohci1", "hosc",
	    USBPHY_CFG_REG, 17),
	SUNXI_CCU_GATE(H3_CLK_USBOHCI2, "usb-ohci2", "hosc",
	    USBPHY_CFG_REG, 18),
	SUNXI_CCU_GATE(H3_CLK_USBOHCI3, "usb-ohci3", "hosc",
	    USBPHY_CFG_REG, 19),
};

static int
sun8i_a83t_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sun8i_a83t_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun8i_a83t_ccu_resets;
	sc->sc_nresets = __arraycount(sun8i_a83t_ccu_resets);

	sc->sc_clks = sun8i_a83t_ccu_clks;
	sc->sc_nclks = __arraycount(sun8i_a83t_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": A83T CCU\n");

	sunxi_ccu_print(sc);
}
