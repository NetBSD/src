/* $NetBSD: sun9i_a80_ccu.c,v 1.1.2.2 2017/12/03 11:35:56 jdolecek Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: sun9i_a80_ccu.c,v 1.1.2.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sun9i_a80_ccu.h>

/* CCU */
#define	PLL_PERIPH0_CTRL_REG	0x00c
#define	PLL_PERIPH1_CTRL_REG	0x02c
#define	GTBUS_CLK_CFG_REG	0x05c
#define	AHB0_CLK_CFG_REG	0x060
#define	AHB1_CLK_CFG_REG	0x064
#define	AHB2_CLK_CFG_REG	0x068
#define	APB0_CLK_CFG_REG	0x070
#define	APB1_CLK_CFG_REG	0x074

/* CCU_SCLK */
#define	SDMMC0_CLK_REG		0x410
#define	SDMMC1_CLK_REG		0x414
#define	SDMMC2_CLK_REG		0x418
#define	BUS_CLK_GATING_REG0	0x580
#define	BUS_CLK_GATING_REG1	0x584
#define	BUS_CLK_GATING_REG2	0x588
#define	BUS_CLK_GATING_REG3	0x590
#define	BUS_CLK_GATING_REG4	0x594
#define	BUS_SOFT_RST_REG0	0x5a0
#define	BUS_SOFT_RST_REG1	0x5a4
#define	BUS_SOFT_RST_REG2	0x5a8
#define	BUS_SOFT_RST_REG3	0x5b0
#define	BUS_SOFT_RST_REG4	0x5b4

static int sun9i_a80_ccu_match(device_t, cfdata_t, void *);
static void sun9i_a80_ccu_attach(device_t, device_t, void *);

static const char * compatible[] = {
	"allwinner,sun9i-a80-ccu",
	NULL
};

CFATTACH_DECL_NEW(sunxi_a80_ccu, sizeof(struct sunxi_ccu_softc),
	sun9i_a80_ccu_match, sun9i_a80_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun9i_a80_ccu_resets[] = {
	SUNXI_CCU_RESET(A80_RST_BUS_FD, BUS_SOFT_RST_REG0, 0),
	SUNXI_CCU_RESET(A80_RST_BUS_GPU_CTRL, BUS_SOFT_RST_REG0, 3),
	SUNXI_CCU_RESET(A80_RST_BUS_SS, BUS_SOFT_RST_REG0, 5),
	SUNXI_CCU_RESET(A80_RST_BUS_MMC, BUS_SOFT_RST_REG0, 8),
	SUNXI_CCU_RESET(A80_RST_BUS_NAND1, BUS_SOFT_RST_REG0, 12),
	SUNXI_CCU_RESET(A80_RST_BUS_NAND0, BUS_SOFT_RST_REG0, 13),
	SUNXI_CCU_RESET(A80_RST_BUS_TS, BUS_SOFT_RST_REG0, 18),
	SUNXI_CCU_RESET(A80_RST_BUS_SPI0, BUS_SOFT_RST_REG0, 20),
	SUNXI_CCU_RESET(A80_RST_BUS_SPI1, BUS_SOFT_RST_REG0, 21),
	SUNXI_CCU_RESET(A80_RST_BUS_SPI2, BUS_SOFT_RST_REG0, 22),
	SUNXI_CCU_RESET(A80_RST_BUS_SPI3, BUS_SOFT_RST_REG0, 23),

	SUNXI_CCU_RESET(A80_RST_BUS_OTG_PHY, BUS_SOFT_RST_REG1, 1),
	SUNXI_CCU_RESET(A80_RST_BUS_MSGBOX, BUS_SOFT_RST_REG1, 21),
	SUNXI_CCU_RESET(A80_RST_BUS_SPINLOCK, BUS_SOFT_RST_REG1, 22),
	SUNXI_CCU_RESET(A80_RST_BUS_HSTIMER, BUS_SOFT_RST_REG1, 23),
	SUNXI_CCU_RESET(A80_RST_BUS_DMA, BUS_SOFT_RST_REG1, 24),

	SUNXI_CCU_RESET(A80_RST_BUS_LCD0, BUS_SOFT_RST_REG2, 0),
	SUNXI_CCU_RESET(A80_RST_BUS_LCD1, BUS_SOFT_RST_REG2, 1),
	SUNXI_CCU_RESET(A80_RST_BUS_CSI, BUS_SOFT_RST_REG2, 4),
	SUNXI_CCU_RESET(A80_RST_BUS_DE, BUS_SOFT_RST_REG2, 7),
	SUNXI_CCU_RESET(A80_RST_BUS_MP, BUS_SOFT_RST_REG2, 8),
	SUNXI_CCU_RESET(A80_RST_BUS_GPU, BUS_SOFT_RST_REG2, 9),

	SUNXI_CCU_RESET(A80_RST_BUS_LRADC, BUS_SOFT_RST_REG3, 15),
	SUNXI_CCU_RESET(A80_RST_BUS_GPADC, BUS_SOFT_RST_REG3, 17),

	SUNXI_CCU_RESET(A80_RST_BUS_I2C0, BUS_SOFT_RST_REG4, 0),
	SUNXI_CCU_RESET(A80_RST_BUS_I2C1, BUS_SOFT_RST_REG4, 1),
	SUNXI_CCU_RESET(A80_RST_BUS_I2C2, BUS_SOFT_RST_REG4, 2),
	SUNXI_CCU_RESET(A80_RST_BUS_I2C3, BUS_SOFT_RST_REG4, 3),
	SUNXI_CCU_RESET(A80_RST_BUS_I2C4, BUS_SOFT_RST_REG4, 4),
	SUNXI_CCU_RESET(A80_RST_BUS_UART0, BUS_SOFT_RST_REG4, 16),
	SUNXI_CCU_RESET(A80_RST_BUS_UART1, BUS_SOFT_RST_REG4, 17),
	SUNXI_CCU_RESET(A80_RST_BUS_UART2, BUS_SOFT_RST_REG4, 18),
	SUNXI_CCU_RESET(A80_RST_BUS_UART3, BUS_SOFT_RST_REG4, 19),
	SUNXI_CCU_RESET(A80_RST_BUS_UART4, BUS_SOFT_RST_REG4, 20),
	SUNXI_CCU_RESET(A80_RST_BUS_UART5, BUS_SOFT_RST_REG4, 21),
};

static const char *gtbus_parents[] = { "hosc", "pll_periph0", "pll_periph1" };
static const char *ahb0_parents[] = { "gtbus", "pll_periph0", "pll_periph1" };
static const char *ahb1_parents[] = { "gtbus", "pll_periph0", "pll_periph1" };
static const char *ahb2_parents[] = { "hosc", "pll_periph0", "pll_periph1" };
static const char *apb_parents[] = { "hosc", "pll_periph0" };
static const char *mmc_parents[] = { "hosc", "pll_periph0" };

static struct sunxi_ccu_clk sun9i_a80_ccu_clks[] = {
	SUNXI_CCU_NKMP(A80_CLK_PLL_PERIPH0, "pll_periph0", "hosc",
	    PLL_PERIPH0_CTRL_REG,	/* reg */
	    __BITS(15,8),		/* n */
	    __BIT(16), 			/* k */
	    0,				/* m */
	    __BIT(18),			/* p */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NKMP_FACTOR_N_EXACT),
	SUNXI_CCU_NKMP(A80_CLK_PLL_PERIPH1, "pll_periph1", "hosc",
	    PLL_PERIPH1_CTRL_REG,	/* reg */
	    __BITS(15,8),		/* n */
	    __BIT(16), 			/* k */
	    0,				/* m */
	    __BIT(18),			/* p */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NKMP_FACTOR_N_EXACT),

	SUNXI_CCU_DIV(A80_CLK_GTBUS, "gtbus", gtbus_parents,
	    GTBUS_CLK_CFG_REG,		/* reg */
	    __BITS(1,0),		/* div */
	    __BITS(25,24),		/* sel */
	    0),

	SUNXI_CCU_DIV(A80_CLK_AHB0, "ahb0", ahb0_parents,
	    AHB0_CLK_CFG_REG,		/* reg */
	    __BITS(1,0),		/* div */
	    __BITS(25,24),		/* sel */
	    SUNXI_CCU_DIV_POWER_OF_TWO),

	SUNXI_CCU_DIV(A80_CLK_AHB1, "ahb1", ahb1_parents,
	    AHB1_CLK_CFG_REG,		/* reg */
	    __BITS(1,0),		/* div */
	    __BITS(25,24),		/* sel */
	    SUNXI_CCU_DIV_POWER_OF_TWO),

	SUNXI_CCU_DIV(A80_CLK_AHB2, "ahb2", ahb2_parents,
	    AHB2_CLK_CFG_REG,		/* reg */
	    __BITS(1,0),		/* div */
	    __BITS(25,24),		/* sel */
	    SUNXI_CCU_DIV_POWER_OF_TWO),

	SUNXI_CCU_DIV(A80_CLK_APB0, "apb0", apb_parents,
	    APB0_CLK_CFG_REG,		/* reg */
	    __BITS(1,0),		/* div */
	    __BIT(24),			/* sel */
	    SUNXI_CCU_DIV_POWER_OF_TWO),

	SUNXI_CCU_NM(A80_CLK_APB1, "apb1", apb_parents,
	    APB1_CLK_CFG_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(4,0),		/* m */
	    __BIT(24),			/* sel */
	    0,				/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(A80_CLK_MMC0, "mmc0", mmc_parents,
	    SDMMC0_CLK_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(27,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),
	SUNXI_CCU_PHASE(A80_CLK_MMC0_SAMPLE, "mmc0_sample", "mmc0",
	    SDMMC0_CLK_REG, __BITS(22,20)),
	SUNXI_CCU_PHASE(A80_CLK_MMC0_OUTPUT, "mmc0_output", "mmc0",
	    SDMMC0_CLK_REG, __BITS(10,8)),
	SUNXI_CCU_NM(A80_CLK_MMC1, "mmc1", mmc_parents,
	    SDMMC1_CLK_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(27,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),
	SUNXI_CCU_PHASE(A80_CLK_MMC1_SAMPLE, "mmc1_sample", "mmc1",
	    SDMMC1_CLK_REG, __BITS(22,20)),
	SUNXI_CCU_PHASE(A80_CLK_MMC1_OUTPUT, "mmc1_output", "mmc1",
	    SDMMC1_CLK_REG, __BITS(10,8)),
	SUNXI_CCU_NM(A80_CLK_MMC2, "mmc2", mmc_parents,
	    SDMMC2_CLK_REG,		/* reg */
	    __BITS(17,16),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(27,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),
	SUNXI_CCU_PHASE(A80_CLK_MMC2_SAMPLE, "mmc2_sample", "mmc2",
	    SDMMC2_CLK_REG, __BITS(22,20)),
	SUNXI_CCU_PHASE(A80_CLK_MMC2_OUTPUT, "mmc2_output", "mmc2",
	    SDMMC2_CLK_REG, __BITS(10,8)),

	SUNXI_CCU_GATE(A80_CLK_BUS_FD, "ahb0-fd", "ahb0",
	    BUS_CLK_GATING_REG0, 0),
	SUNXI_CCU_GATE(A80_CLK_BUS_GPU_CTRL, "ahb0-gpu-ctrl", "ahb0",
	    BUS_CLK_GATING_REG0, 3),
	SUNXI_CCU_GATE(A80_CLK_BUS_SS, "ahb0-ss", "ahb0",
	    BUS_CLK_GATING_REG0, 5),
	SUNXI_CCU_GATE(A80_CLK_BUS_MMC, "ahb0-mmc", "ahb0",
	    BUS_CLK_GATING_REG0, 8),
	SUNXI_CCU_GATE(A80_CLK_BUS_NAND1, "ahb0-nand1", "ahb0",
	    BUS_CLK_GATING_REG0, 12),
	SUNXI_CCU_GATE(A80_CLK_BUS_NAND0, "ahb0-nand0", "ahb0",
	    BUS_CLK_GATING_REG0, 13),
	SUNXI_CCU_GATE(A80_CLK_BUS_TS, "ahb0-ts", "ahb0",
	    BUS_CLK_GATING_REG0, 18),
	SUNXI_CCU_GATE(A80_CLK_BUS_SPI0, "ahb0-spi0", "ahb0",
	    BUS_CLK_GATING_REG0, 20),
	SUNXI_CCU_GATE(A80_CLK_BUS_SPI1, "ahb0-spi1", "ahb0",
	    BUS_CLK_GATING_REG0, 21),
	SUNXI_CCU_GATE(A80_CLK_BUS_SPI2, "ahb0-spi2", "ahb0",
	    BUS_CLK_GATING_REG0, 22),
	SUNXI_CCU_GATE(A80_CLK_BUS_SPI3, "ahb0-spi3", "ahb0",
	    BUS_CLK_GATING_REG0, 23),

	SUNXI_CCU_GATE(A80_CLK_BUS_USB, "ahb1-usb", "ahb1",
	    BUS_CLK_GATING_REG1, 1),
	SUNXI_CCU_GATE(A80_CLK_BUS_MSGBOX, "ahb1-msgbox", "ahb1",
	    BUS_CLK_GATING_REG1, 21),
	SUNXI_CCU_GATE(A80_CLK_BUS_SPINLOCK, "ahb1-spinlock", "ahb1",
	    BUS_CLK_GATING_REG1, 22),
	SUNXI_CCU_GATE(A80_CLK_BUS_HSTIMER, "ahb1-hstimer", "ahb1",
	    BUS_CLK_GATING_REG1, 23),
	SUNXI_CCU_GATE(A80_CLK_BUS_DMA, "ahb1-dma", "ahb1",
	    BUS_CLK_GATING_REG1, 24),

	SUNXI_CCU_GATE(A80_CLK_BUS_LCD0, "ahb2-lcd0", "ahb2",
	    BUS_CLK_GATING_REG2, 0),
	SUNXI_CCU_GATE(A80_CLK_BUS_LCD1, "ahb2-lcd1", "ahb2",
	    BUS_CLK_GATING_REG2, 1),
	SUNXI_CCU_GATE(A80_CLK_BUS_CSI, "ahb2-csi", "ahb2",
	    BUS_CLK_GATING_REG2, 4),
	SUNXI_CCU_GATE(A80_CLK_BUS_DE, "ahb2-de", "ahb2",
	    BUS_CLK_GATING_REG2, 7),
	SUNXI_CCU_GATE(A80_CLK_BUS_MP, "ahb2-mp", "ahb2",
	    BUS_CLK_GATING_REG2, 8),

	SUNXI_CCU_GATE(A80_CLK_BUS_PIO, "apb0-pio", "apb0",
	    BUS_CLK_GATING_REG3, 5),
	SUNXI_CCU_GATE(A80_CLK_BUS_LRADC, "apb0-lradc", "apb0",
	    BUS_CLK_GATING_REG3, 15),
	SUNXI_CCU_GATE(A80_CLK_BUS_GPADC, "apb0-gpadc", "apb0",
	    BUS_CLK_GATING_REG3, 17),

	SUNXI_CCU_GATE(A80_CLK_BUS_I2C0, "apb1-i2c0", "apb1",
	    BUS_CLK_GATING_REG4, 0),
	SUNXI_CCU_GATE(A80_CLK_BUS_I2C1, "apb1-i2c1", "apb1",
	    BUS_CLK_GATING_REG4, 1),
	SUNXI_CCU_GATE(A80_CLK_BUS_I2C2, "apb1-i2c2", "apb1",
	    BUS_CLK_GATING_REG4, 2),
	SUNXI_CCU_GATE(A80_CLK_BUS_I2C3, "apb1-i2c3", "apb1",
	    BUS_CLK_GATING_REG4, 3),
	SUNXI_CCU_GATE(A80_CLK_BUS_I2C4, "apb1-i2c4", "apb1",
	    BUS_CLK_GATING_REG4, 4),
	SUNXI_CCU_GATE(A80_CLK_BUS_UART0, "apb1-uart0", "apb1",
	    BUS_CLK_GATING_REG4, 16),
	SUNXI_CCU_GATE(A80_CLK_BUS_UART1, "apb1-uart1", "apb1",
	    BUS_CLK_GATING_REG4, 17),
	SUNXI_CCU_GATE(A80_CLK_BUS_UART2, "apb1-uart2", "apb1",
	    BUS_CLK_GATING_REG4, 18),
	SUNXI_CCU_GATE(A80_CLK_BUS_UART3, "apb1-uart3", "apb1",
	    BUS_CLK_GATING_REG4, 19),
	SUNXI_CCU_GATE(A80_CLK_BUS_UART4, "apb1-uart4", "apb1",
	    BUS_CLK_GATING_REG4, 20),
	SUNXI_CCU_GATE(A80_CLK_BUS_UART5, "apb1-uart5", "apb1",
	    BUS_CLK_GATING_REG4, 21),
};

static int
sun9i_a80_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sun9i_a80_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun9i_a80_ccu_resets;
	sc->sc_nresets = __arraycount(sun9i_a80_ccu_resets);

	sc->sc_clks = sun9i_a80_ccu_clks;
	sc->sc_nclks = __arraycount(sun9i_a80_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": A80 CCU\n");

	sunxi_ccu_print(sc);
}
