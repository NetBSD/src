/* $NetBSD: sun8i_v3s_ccu.c,v 1.1 2021/05/05 10:24:04 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 Rui-Xiang Guo
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

__KERNEL_RCSID(1, "$NetBSD: sun8i_v3s_ccu.c,v 1.1 2021/05/05 10:24:04 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sun8i_v3s_ccu.h>

#define	PLL_CPU_CTRL_REG	0x000
#define	PLL_AUDIO_CTRL_REG	0x008
#define	PLL_VIDEO_CTRL_REG	0x010
#define	PLL_PERIPH0_CTRL_REG	0x028
#define	AHB1_APB1_CFG_REG	0x054
#define	APB2_CFG_REG		0x058
#define	AHB2_CFG_REG		0x05c
#define	 AHB2_CLK_CFG		__BITS(1,0)
#define	 AHB2_CLK_CFG_PLL_PERIPH0_2	1
#define	BUS_CLK_GATING_REG0	0x060
#define	BUS_CLK_GATING_REG1	0x064
#define	BUS_CLK_GATING_REG2	0x068
#define	BUS_CLK_GATING_REG3	0x06c
#define	BUS_CLK_GATING_REG4	0x070
#define	SDMMC0_CLK_REG		0x088
#define	SDMMC1_CLK_REG		0x08c
#define	SDMMC2_CLK_REG		0x090
#define	SPI_CLK_REG		0x0a0
#define	USBPHY_CFG_REG		0x0cc
#define	MBUS_RST_REG		0x0fc
#define	DE_CLK_REG		0x104
#define	TCON_CLK_REG		0x118
#define	AC_DIG_CLK_REG		0x140
#define	BUS_SOFT_RST_REG0	0x2c0
#define	BUS_SOFT_RST_REG1	0x2c4
#define	BUS_SOFT_RST_REG2	0x2c8
#define	BUS_SOFT_RST_REG3	0x2d0
#define	BUS_SOFT_RST_REG4	0x2d8

static int sun8i_v3s_ccu_match(device_t, cfdata_t, void *);
static void sun8i_v3s_ccu_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun8i-v3s-ccu" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(sunxi_v3s_ccu, sizeof(struct sunxi_ccu_softc),
	sun8i_v3s_ccu_match, sun8i_v3s_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun8i_v3s_ccu_resets[] = {
	SUNXI_CCU_RESET(V3S_RST_USBPHY, USBPHY_CFG_REG, 0),

	SUNXI_CCU_RESET(V3S_RST_MBUS, MBUS_RST_REG, 31),

	SUNXI_CCU_RESET(V3S_RST_BUS_CE, BUS_SOFT_RST_REG0, 5),
	SUNXI_CCU_RESET(V3S_RST_BUS_DMA, BUS_SOFT_RST_REG0, 6),
	SUNXI_CCU_RESET(V3S_RST_BUS_MMC0, BUS_SOFT_RST_REG0, 8),
	SUNXI_CCU_RESET(V3S_RST_BUS_MMC1, BUS_SOFT_RST_REG0, 9),
	SUNXI_CCU_RESET(V3S_RST_BUS_MMC2, BUS_SOFT_RST_REG0, 10),
	SUNXI_CCU_RESET(V3S_RST_BUS_DRAM, BUS_SOFT_RST_REG0, 14),
	SUNXI_CCU_RESET(V3S_RST_BUS_EMAC, BUS_SOFT_RST_REG0, 17),
	SUNXI_CCU_RESET(V3S_RST_BUS_HSTIMER, BUS_SOFT_RST_REG0, 19),
	SUNXI_CCU_RESET(V3S_RST_BUS_SPI, BUS_SOFT_RST_REG0, 20),
	SUNXI_CCU_RESET(V3S_RST_BUS_OTG, BUS_SOFT_RST_REG0, 24),
	SUNXI_CCU_RESET(V3S_RST_BUS_EHCI, BUS_SOFT_RST_REG0, 26),
	SUNXI_CCU_RESET(V3S_RST_BUS_OHCI, BUS_SOFT_RST_REG0, 29),
        
	SUNXI_CCU_RESET(V3S_RST_BUS_VE, BUS_SOFT_RST_REG1, 0),
	SUNXI_CCU_RESET(V3S_RST_BUS_TCON, BUS_SOFT_RST_REG1, 4),
	SUNXI_CCU_RESET(V3S_RST_BUS_CSI, BUS_SOFT_RST_REG1, 8),
	SUNXI_CCU_RESET(V3S_RST_BUS_DE, BUS_SOFT_RST_REG1, 12),
	SUNXI_CCU_RESET(V3S_RST_BUS_DBG, BUS_SOFT_RST_REG1, 31),

	SUNXI_CCU_RESET(V3S_RST_BUS_EPHY, BUS_SOFT_RST_REG2, 2),

	SUNXI_CCU_RESET(V3S_RST_BUS_CODEC, BUS_SOFT_RST_REG3, 0),

	SUNXI_CCU_RESET(V3S_RST_BUS_I2C0, BUS_SOFT_RST_REG4, 0),
	SUNXI_CCU_RESET(V3S_RST_BUS_I2C1, BUS_SOFT_RST_REG4, 1),
	SUNXI_CCU_RESET(V3S_RST_BUS_UART0, BUS_SOFT_RST_REG4, 16),
	SUNXI_CCU_RESET(V3S_RST_BUS_UART1, BUS_SOFT_RST_REG4, 17),
	SUNXI_CCU_RESET(V3S_RST_BUS_UART2, BUS_SOFT_RST_REG4, 18),
};

static const char *ahb1_parents[] = { "losc", "hosc", "axi", "pll_periph0" };
static const char *ahb2_parents[] = { "ahb1", "pll_periph0" };
static const char *apb1_parents[] = { "ahb1" };
static const char *apb2_parents[] = { "losc", "hosc", "pll_periph0" };
static const char *mod_parents[] = { "hosc", "pll_periph0", "pll_periph1" };
static const char *tcon_parents[] = { "pll_video" };

static const struct sunxi_ccu_nkmp_tbl sun8i_v3s_cpu_table[] = {
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
	{ 1392000000, 28, 1, 0, 0 },
	{ 1440000000, 29, 1, 0, 0 },
	{ 1512000000, 20, 2, 0, 0 },
	{ 1536000000, 15, 3, 0, 0 },
	{ 1584000000, 21, 2, 0, 0 },
	{ 1632000000, 16, 3, 0, 0 },
	{ 1656000000, 22, 2, 0, 0 },
	{ 1728000000, 23, 2, 0, 0 },
	{ 1800000000, 24, 2, 0, 0 },
	{ 1824000000, 18, 3, 0, 0 },
	{ 1872000000, 25, 2, 0, 0 },
	{ 0 }
};

static const struct sunxi_ccu_nkmp_tbl sun8i_v3s_ac_dig_table[] = {
	{ 24576000, 13, 0, 0, 13 },
	{ 0 }
};

static struct sunxi_ccu_clk sun8i_v3s_ccu_clks[] = {
	SUNXI_CCU_NKMP_TABLE(V3S_CLK_CPU, "pll_cpu", "hosc",
	    PLL_CPU_CTRL_REG,		/* reg */
	    __BITS(12,8),		/* n */
	    __BITS(5,4),		/* k */
	    __BITS(1,0),		/* m */
	    __BITS(17,16),		/* p */
	    __BIT(31),			/* enable */
	    __BIT(28),			/* lock */
	    sun8i_v3s_cpu_table,	/* table */
	    SUNXI_CCU_NKMP_SCALE_CLOCK | SUNXI_CCU_NKMP_FACTOR_P_POW2),
	
	SUNXI_CCU_NKMP(V3S_CLK_PLL_PERIPH0, "pll_periph0", "hosc",
	    PLL_PERIPH0_CTRL_REG,	/* reg */
	    __BITS(12,8),		/* n */
	    __BITS(5,4), 		/* k */
	    0,				/* m */
	    __BITS(17,16),		/* p */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NKMP_DIVIDE_BY_TWO),

	SUNXI_CCU_FIXED_FACTOR(V3S_CLK_PLL_PERIPH0_2X, "pll_periph0_2x", "pll_periph0", 1, 2),

	SUNXI_CCU_FRACTIONAL(V3S_CLK_PLL_VIDEO, "pll_video", "hosc",
	    PLL_VIDEO_CTRL_REG,		/* reg */
	    __BITS(14,8),		/* m */
	    16,				/* m_min */
	    50,				/* m_max */
	    __BIT(24),			/* div_en */
	    __BIT(25),			/* frac_sel */
	    270000000, 297000000,	/* frac values */
	    __BITS(3,0),		/* prediv */
	    4,				/* prediv_val */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_FRACTIONAL_PLUSONE | SUNXI_CCU_FRACTIONAL_SET_ENABLE),

	SUNXI_CCU_NKMP_TABLE(V3S_CLK_PLL_AUDIO_BASE, "pll_audio", "hosc",
	    PLL_AUDIO_CTRL_REG,		/* reg */
	    __BITS(14,8),		/* n */
	    0,				/* k */
	    __BITS(4,0),		/* m */
	    __BITS(19,16),		/* p */
	    __BIT(31),			/* enable */
	    __BIT(28),			/* lock */
	    sun8i_v3s_ac_dig_table,	/* table */
	    0),

	SUNXI_CCU_PREDIV(V3S_CLK_AHB1, "ahb1", ahb1_parents,
	    AHB1_APB1_CFG_REG,	/* reg */
	    __BITS(7,6),	/* prediv */
	    __BIT(3),		/* prediv_sel */
	    __BITS(5,4),	/* div */
	    __BITS(13,12),	/* sel */
	    SUNXI_CCU_PREDIV_POWER_OF_TWO),

	SUNXI_CCU_PREDIV(V3S_CLK_AHB2, "ahb2", ahb2_parents,
	    AHB2_CFG_REG,	/* reg */
	    0,			/* prediv */
	    __BIT(1),		/* prediv_sel */
	    0,			/* div */
	    __BITS(1,0),	/* sel */
	    SUNXI_CCU_PREDIV_DIVIDE_BY_TWO),

	SUNXI_CCU_DIV(V3S_CLK_APB1, "apb1", apb1_parents,
	    AHB1_APB1_CFG_REG,	/* reg */
	    __BITS(9,8),	/* div */
	    0,			/* sel */
	    SUNXI_CCU_DIV_POWER_OF_TWO|SUNXI_CCU_DIV_ZERO_IS_ONE),

	SUNXI_CCU_NM(V3S_CLK_APB2, "apb2", apb2_parents,
	    APB2_CFG_REG,	/* reg */
	    __BITS(17,16),	/* n */
	    __BITS(4,0),	/* m */
	    __BITS(25,24),	/* sel */
	    0,			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(V3S_CLK_MMC0, "mmc0", mod_parents,
	    SDMMC0_CLK_REG, __BITS(17, 16), __BITS(3,0), __BITS(25, 24), __BIT(31),
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_PHASE(V3S_CLK_MMC0_SAMPLE, "mmc0_sample", "mmc0",
	    SDMMC0_CLK_REG, __BITS(22,20)),
	SUNXI_CCU_PHASE(V3S_CLK_MMC0_OUTPUT, "mmc0_output", "mmc0",
	    SDMMC0_CLK_REG, __BITS(10,8)),
	SUNXI_CCU_NM(V3S_CLK_MMC1, "mmc1", mod_parents,
	    SDMMC1_CLK_REG, __BITS(17, 16), __BITS(3,0), __BITS(25, 24), __BIT(31),
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_PHASE(V3S_CLK_MMC1_SAMPLE, "mmc1_sample", "mmc1",
	    SDMMC1_CLK_REG, __BITS(22,20)),
	SUNXI_CCU_PHASE(V3S_CLK_MMC1_OUTPUT, "mmc1_output", "mmc1",
	    SDMMC1_CLK_REG, __BITS(10,8)),
	SUNXI_CCU_NM(V3S_CLK_MMC2, "mmc2", mod_parents,
	    SDMMC2_CLK_REG, __BITS(17, 16), __BITS(3,0), __BITS(25, 24), __BIT(31),
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_PHASE(V3S_CLK_MMC2_SAMPLE, "mmc2_sample", "mmc2",
	    SDMMC2_CLK_REG, __BITS(22,20)),
	SUNXI_CCU_PHASE(V3S_CLK_MMC2_OUTPUT, "mmc2_output", "mmc2",
	    SDMMC2_CLK_REG, __BITS(10,8)),

	SUNXI_CCU_NM(V3S_CLK_SPI, "spi", mod_parents,
	    SPI_CLK_REG,	/* reg */
	    __BITS(17,16),	/* n */
	    __BITS(3,0),	/* m */
	    __BITS(25,24),	/* sel */
	    __BIT(31),		/* enable */
	    SUNXI_CCU_NM_ROUND_DOWN),

	SUNXI_CCU_GATE(V3S_CLK_AC_DIG, "ac_dig", "pll_audio",
	    AC_DIG_CLK_REG, 31),

	SUNXI_CCU_DIV_GATE(V3S_CLK_TCON, "tcon", tcon_parents,
	    TCON_CLK_REG,	/* reg */
	    __BITS(3,0),	/* div */
	    __BITS(26,24),	/* sel */
	    __BIT(31),		/* enable */
	    0),

	SUNXI_CCU_GATE(V3S_CLK_BUS_DMA, "bus-dma", "ahb1",
	    BUS_CLK_GATING_REG0, 6),
	SUNXI_CCU_GATE(V3S_CLK_BUS_MMC0, "bus-mmc0", "ahb1",
	    BUS_CLK_GATING_REG0, 8),
	SUNXI_CCU_GATE(V3S_CLK_BUS_MMC1, "bus-mmc1", "ahb1",
	    BUS_CLK_GATING_REG0, 9),
	SUNXI_CCU_GATE(V3S_CLK_BUS_MMC2, "bus-mmc2", "ahb1",
	    BUS_CLK_GATING_REG0, 10),
	SUNXI_CCU_GATE(V3S_CLK_BUS_EMAC, "bus-emac", "ahb2",
	    BUS_CLK_GATING_REG0, 17),
	SUNXI_CCU_GATE(V3S_CLK_BUS_SPI, "bus-spi", "ahb1",
	    BUS_CLK_GATING_REG0, 20),
	SUNXI_CCU_GATE(V3S_CLK_BUS_OTG, "bus-otg", "ahb1",
	    BUS_CLK_GATING_REG0, 24),
	SUNXI_CCU_GATE(V3S_CLK_BUS_EHCI, "bus-ehci", "ahb1",
	    BUS_CLK_GATING_REG0, 26),
	SUNXI_CCU_GATE(V3S_CLK_BUS_OHCI, "bus-ohci", "ahb1",
	    BUS_CLK_GATING_REG0, 29),

	SUNXI_CCU_GATE(V3S_CLK_BUS_TCON, "bus-tcon", "ahb1",
	    BUS_CLK_GATING_REG1, 4),
	SUNXI_CCU_GATE(V3S_CLK_BUS_DE, "bus-de", "ahb1",
	    BUS_CLK_GATING_REG1, 12),

	SUNXI_CCU_GATE(V3S_CLK_BUS_CODEC, "bus-codec", "apb1",
	    BUS_CLK_GATING_REG2, 0),
	SUNXI_CCU_GATE(V3S_CLK_BUS_PIO, "bus-pio", "apb1",
	    BUS_CLK_GATING_REG2, 5),

	SUNXI_CCU_GATE(V3S_CLK_BUS_I2C0, "bus-i2c0", "apb2",
	    BUS_CLK_GATING_REG3, 0),
	SUNXI_CCU_GATE(V3S_CLK_BUS_I2C1, "bus-i2c1", "apb2",
	    BUS_CLK_GATING_REG3, 1),
	SUNXI_CCU_GATE(V3S_CLK_BUS_UART0, "bus-uart0", "apb2",
	    BUS_CLK_GATING_REG3, 16),
	SUNXI_CCU_GATE(V3S_CLK_BUS_UART1, "bus-uart1", "apb2",
	    BUS_CLK_GATING_REG3, 17),
	SUNXI_CCU_GATE(V3S_CLK_BUS_UART2, "bus-uart2", "apb2",
	    BUS_CLK_GATING_REG3, 18),

	SUNXI_CCU_GATE(V3S_CLK_BUS_EPHY, "bus-ephy", "ahb1",
	    BUS_CLK_GATING_REG4, 0),

	SUNXI_CCU_GATE(V3S_CLK_USBPHY, "usb-phy", "hosc",
	    USBPHY_CFG_REG, 8),
	SUNXI_CCU_GATE(V3S_CLK_USBOHCI, "usb-ohci", "hosc",
	    USBPHY_CFG_REG, 16),
};

static void
sun8i_v3s_ccu_init(struct sunxi_ccu_softc *sc)
{
	uint32_t val;

	/* Set AHB2 source to PLL_PERIPH/2 */
	val = CCU_READ(sc, AHB2_CFG_REG);
	val &= ~AHB2_CLK_CFG;
	val |= __SHIFTIN(AHB2_CLK_CFG_PLL_PERIPH0_2, AHB2_CLK_CFG);
	CCU_WRITE(sc, AHB2_CFG_REG, val);
}

static int
sun8i_v3s_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sun8i_v3s_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun8i_v3s_ccu_resets;
	sc->sc_nresets = __arraycount(sun8i_v3s_ccu_resets);

	sc->sc_clks = sun8i_v3s_ccu_clks;
	sc->sc_nclks = __arraycount(sun8i_v3s_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": V3s CCU\n");

	sun8i_v3s_ccu_init(sc);

	sunxi_ccu_print(sc);
}
