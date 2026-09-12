/* $NetBSD$ */

/*-
 * Copyright (c) 2026
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

__KERNEL_RCSID(1, "$NetBSD$");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sun50i_h616_ccu.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_opp.h>
#include <dev/fdt/syscon.h>

/* pll registers */
#define PLL_CPUX_CTRL_REG       0x000
#define PLL_DDR0_CTRL_REG       0x010
#define PLL_DDR1_CTRL_REG       0x018
#define PLL_PERI0_CTRL_REG      0x020
#define PLL_PERI1_CTRL_REG      0x028
#define PLL_GPU_CTRL_REG        0x030
#define PLL_VIDEO0_CTRL_REG     0x040
#define PLL_VIDEO1_CTRL_REG     0x048
#define PLL_VIDEO2_CTRL_REG     0x050
#define PLL_VE_CTRL_REG         0x058
#define PLL_DE_CTRL_REG         0x060
#define PLL_AUDIO_CTRL_REG      0x078

/* bus config registers */
#define CPUX_AXI_CFG_REG        0x500
#define PSI_AHB1_AHB2_CFG_REG  0x510
#define AHB3_CFG_REG            0x51c
#define APB1_CFG_REG            0x520
#define APB2_CFG_REG            0x524
#define MBUS_CFG_REG            0x540

/* module clock registers */
#define DE_CLK_REG              0x600
#define DE_BGR_REG              0x60c
#define DI_CLK_REG              0x620
#define DI_BGR_REG              0x62c
#define G2D_CLK_REG             0x630
#define G2D_BGR_REG             0x63c
#define GPU_CLK_REG             0x670
#define GPU_BGR_REG             0x67c
#define CE_CLK_REG              0x680
#define CE_BGR_REG              0x68c
#define VE_CLK_REG              0x690
#define VE_BGR_REG              0x69c
#define DMA_BGR_REG             0x70c
#define HSTIMER_BGR_REG         0x73c
#define AVS_CLK_REG             0x740
#define DBGSYS_BGR_REG          0x78c
#define PSI_BGR_REG             0x79c
#define PWM_BGR_REG             0x7ac
#define IOMMU_BGR_REG           0x7bc
#define DRAM_CLK_REG            0x800
#define DRAM_BGR_REG            0x80c
#define NAND0_CLK_REG           0x810
#define NAND1_CLK_REG           0x814
#define NAND_BGR_REG            0x82c
#define SMHC0_CLK_REG           0x830
#define SMHC1_CLK_REG           0x834
#define SMHC2_CLK_REG           0x838
#define SMHC_BGR_REG            0x84c
#define UART_BGR_REG            0x90c
#define TWI_BGR_REG             0x91c
#define SPI0_CLK_REG            0x940
#define SPI1_CLK_REG            0x944
#define SPI_BGR_REG             0x96c
#define EMAC_25M_CLK_REG        0x970
#define EMAC_BGR_REG            0x97c
#define TS_CLK_REG              0x9b0
#define TS_BGR_REG              0x9bc
#define GPADC_BGR_REG           0x9ec
#define THS_BGR_REG             0x9fc
#define SPDIF_CLK_REG           0xa20
#define SPDIF_BGR_REG           0xa2c
#define DMIC_CLK_REG            0xa40
#define DMIC_BGR_REG            0xa4c
#define AUDIO_CODEC_DAC_CLK_REG 0xa50
#define AUDIO_CODEC_ADC_CLK_REG 0xa54
#define AUDIO_CODEC_BGR_REG     0xa5c
#define AUDIO_HUB_CLK_REG       0xa60
#define AUDIO_HUB_BGR_REG       0xa6c
#define USB0_CLK_REG            0xa70
#define USB1_CLK_REG            0xa74
#define USB2_CLK_REG            0xa78
#define USB3_CLK_REG            0xa7c
#define USB_BGR_REG             0xa8c
#define HDMI_CLK_REG            0xb00
#define HDMI_SLOW_CLK_REG       0xb04
#define HDMI_CEC_CLK_REG        0xb10
#define HDMI_BGR_REG            0xb1c
#define DISPLAY_IF_TOP_BGR_REG  0xb5c
#define TCON_TV0_CLK_REG        0xb80
#define TCON_TV1_CLK_REG        0xb84
#define TCON_TV_BGR_REG         0xb9c
#define TVE0_CLK_REG            0xbb0
#define TVE_BGR_REG             0xbbc
#define HDCP_CLK_REG            0xc40
#define HDCP_BGR_REG            0xc4c
#define KEYADC_BGR_REG          0x9ec

static int sun50i_h616_ccu_match(device_t, cfdata_t, void *);
static void sun50i_h616_ccu_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun50i-h616-ccu" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(sunxi_h616_ccu, sizeof(struct sunxi_ccu_softc),
	sun50i_h616_ccu_match, sun50i_h616_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun50i_h616_ccu_resets[] = {
	SUNXI_CCU_RESET(H616_RST_MBUS, MBUS_CFG_REG, 30),

	SUNXI_CCU_RESET(H616_RST_BUS_DE, DE_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_DEINTERLACE, DI_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_GPU, GPU_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_CE, CE_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_VE, VE_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_DMA, DMA_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_HSTIMER, HSTIMER_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_DBG, DBGSYS_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_PSI, PSI_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_PWM, PWM_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_IOMMU, IOMMU_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_DRAM, DRAM_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_NAND, NAND_BGR_REG, 16),

	SUNXI_CCU_RESET(H616_RST_BUS_MMC0, SMHC_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_MMC1, SMHC_BGR_REG, 17),
	SUNXI_CCU_RESET(H616_RST_BUS_MMC2, SMHC_BGR_REG, 18),

	SUNXI_CCU_RESET(H616_RST_BUS_UART0, UART_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_UART1, UART_BGR_REG, 17),
	SUNXI_CCU_RESET(H616_RST_BUS_UART2, UART_BGR_REG, 18),
	SUNXI_CCU_RESET(H616_RST_BUS_UART3, UART_BGR_REG, 19),
	SUNXI_CCU_RESET(H616_RST_BUS_UART4, UART_BGR_REG, 20),
	SUNXI_CCU_RESET(H616_RST_BUS_UART5, UART_BGR_REG, 21),

	SUNXI_CCU_RESET(H616_RST_BUS_I2C0, TWI_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_I2C1, TWI_BGR_REG, 17),
	SUNXI_CCU_RESET(H616_RST_BUS_I2C2, TWI_BGR_REG, 18),
	SUNXI_CCU_RESET(H616_RST_BUS_I2C3, TWI_BGR_REG, 19),
	SUNXI_CCU_RESET(H616_RST_BUS_I2C4, TWI_BGR_REG, 20),

	SUNXI_CCU_RESET(H616_RST_BUS_SPI0, SPI_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_SPI1, SPI_BGR_REG, 17),

	SUNXI_CCU_RESET(H616_RST_BUS_EMAC0, EMAC_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_EMAC1, EMAC_BGR_REG, 17),

	SUNXI_CCU_RESET(H616_RST_BUS_TS, TS_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_THS, THS_BGR_REG, 16),

	SUNXI_CCU_RESET(H616_RST_BUS_SPDIF, SPDIF_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_DMIC, DMIC_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_AUDIO_CODEC, AUDIO_CODEC_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_AUDIO_HUB, AUDIO_HUB_BGR_REG, 16),

	SUNXI_CCU_RESET(H616_RST_USB_PHY0, USB0_CLK_REG, 30),
	SUNXI_CCU_RESET(H616_RST_USB_PHY1, USB1_CLK_REG, 30),
	SUNXI_CCU_RESET(H616_RST_USB_PHY2, USB2_CLK_REG, 30),
	SUNXI_CCU_RESET(H616_RST_USB_PHY3, USB3_CLK_REG, 30),

	SUNXI_CCU_RESET(H616_RST_BUS_OHCI0, USB_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_OHCI1, USB_BGR_REG, 17),
	SUNXI_CCU_RESET(H616_RST_BUS_OHCI2, USB_BGR_REG, 18),
	SUNXI_CCU_RESET(H616_RST_BUS_OHCI3, USB_BGR_REG, 19),
	SUNXI_CCU_RESET(H616_RST_BUS_EHCI0, USB_BGR_REG, 20),
	SUNXI_CCU_RESET(H616_RST_BUS_EHCI1, USB_BGR_REG, 21),
	SUNXI_CCU_RESET(H616_RST_BUS_EHCI2, USB_BGR_REG, 22),
	SUNXI_CCU_RESET(H616_RST_BUS_EHCI3, USB_BGR_REG, 23),
	SUNXI_CCU_RESET(H616_RST_BUS_OTG, USB_BGR_REG, 24),

	SUNXI_CCU_RESET(H616_RST_BUS_HDMI, HDMI_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_HDMI_SUB, HDMI_BGR_REG, 17),
	SUNXI_CCU_RESET(H616_RST_BUS_TCON_TOP, DISPLAY_IF_TOP_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_TCON_TV0, TCON_TV_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_TCON_TV1, TCON_TV_BGR_REG, 17),
	SUNXI_CCU_RESET(H616_RST_BUS_TVE_TOP, TVE_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_TVE0, TVE_BGR_REG, 17),
	SUNXI_CCU_RESET(H616_RST_BUS_HDCP, HDCP_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_KEYADC, KEYADC_BGR_REG, 16),
	SUNXI_CCU_RESET(H616_RST_BUS_GPADC, GPADC_BGR_REG, 16),
};

static const char *ahb3_parents[] = { "hosc", "losc", "psi", "pll_periph0" };
static const char *apb1_parents[] = { "hosc", "losc", "psi", "pll_periph0" };
static const char *apb2_parents[] = { "hosc", "losc", "psi", "pll_periph0" };
static const char *mod_parents[] = { "hosc", "pll_periph0_2x", "pll_periph1_2x" };
static const char *psi_ahb1_ahb2_parents[] = { "hosc", "losc", "iosc", "pll_periph0" };

static struct sunxi_ccu_clk sun50i_h616_ccu_clks[] = {
	SUNXI_CCU_FIXED_FACTOR(H616_CLK_OSC12M, "osc12m", "hosc", 2, 1),

	/*
	 * pll_cpux: multiplier clock.
	 * register 0x000, n field bits[15:8], m field bits[1:0],
	 * p field bits[17:16], enable bit 31, lock bit 28.
	 */
	SUNXI_CCU_NKMP_TABLE(H616_CLK_PLL_CPUX, "pll_cpux", "hosc",
	    PLL_CPUX_CTRL_REG,
	    __BITS(15,8),		/* n */
	    0,				/* k */
	    __BITS(1,0),		/* m */
	    __BITS(17,16),		/* p */
	    __BIT(31),			/* enable */
	    __BIT(28),			/* lock */
	    NULL,			/* table */
	    SUNXI_CCU_NKMP_SCALE_CLOCK | SUNXI_CCU_NKMP_FACTOR_P_POW2),

	/*
	 * pll_periph0: base for most peripheral clocks.
	 * model as 4x and derive 2x and 1x from it.
	 */
	SUNXI_CCU_NKMP(H616_CLK_PLL_PERIPH0_4X, "pll_periph0_4x", "hosc",
	    PLL_PERI0_CTRL_REG,
	    __BITS(15,8),		/* n */
	    0,				/* k */
	    __BIT(1),			/* m */
	    0,				/* p - not used on h616 */
	    __BIT(31),			/* enable */
	    0),
	SUNXI_CCU_FIXED_FACTOR(H616_CLK_PLL_PERIPH0_2X, "pll_periph0_2x",
	    "pll_periph0_4x", 2, 1),
	SUNXI_CCU_FIXED_FACTOR(H616_CLK_PLL_PERIPH0, "pll_periph0",
	    "pll_periph0_4x", 4, 1),
	SUNXI_CCU_FIXED_FACTOR(H616_CLK_PLL_SYSTEM_32K, "pll_system_32k",
	    "pll_periph0_2x", 36621, 1),

	SUNXI_CCU_NKMP(H616_CLK_PLL_PERIPH1_4X, "pll_periph1_4x", "hosc",
	    PLL_PERI1_CTRL_REG,
	    __BITS(15,8),		/* n */
	    0,				/* k */
	    __BIT(1),			/* m */
	    0,				/* p - not used on h616 */
	    __BIT(31),			/* enable */
	    0),
	SUNXI_CCU_FIXED_FACTOR(H616_CLK_PLL_PERIPH1_2X, "pll_periph1_2x",
	    "pll_periph1_4x", 2, 1),
	SUNXI_CCU_FIXED_FACTOR(H616_CLK_PLL_PERIPH1, "pll_periph1",
	    "pll_periph1_4x", 4, 1),
	
	/* cpux: passthrough to pll_cpux (mux is set by u-boot) */
	SUNXI_CCU_FIXED_FACTOR(H616_CLK_CPUX, "cpux", "pll_cpux", 1, 1),

	/* bus clocks */
	SUNXI_CCU_NM(H616_CLK_PSI_AHB1_AHB2, "psi",
	    psi_ahb1_ahb2_parents,
	    PSI_AHB1_AHB2_CFG_REG,
	    __BITS(9,8),		/* n */
	    __BITS(1,0),		/* m */
	    __BITS(25,24),		/* sel */
	    0,				/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(H616_CLK_AHB3, "ahb3", ahb3_parents,
	    AHB3_CFG_REG,
	    __BITS(9,8),		/* n */
	    __BITS(1,0),		/* m */
	    __BITS(25,24),		/* sel */
	    0,				/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(H616_CLK_APB1, "apb1", apb1_parents,
	    APB1_CFG_REG,
	    __BITS(9,8),		/* n */
	    __BITS(1,0),		/* m */
	    __BITS(25,24),		/* sel */
	    0,				/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	SUNXI_CCU_NM(H616_CLK_APB2, "apb2", apb2_parents,
	    APB2_CFG_REG,
	    __BITS(9,8),		/* n */
	    __BITS(1,0),		/* m */
	    __BITS(25,24),		/* sel */
	    0,				/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO),

	/* mmc clocks */
	SUNXI_CCU_NM(H616_CLK_MMC0, "mmc0", mod_parents,
	    SMHC0_CLK_REG,
	    __BITS(9,8),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_NM(H616_CLK_MMC1, "mmc1", mod_parents,
	    SMHC1_CLK_REG,
	    __BITS(9,8),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_NM(H616_CLK_MMC2, "mmc2", mod_parents,
	    SMHC2_CLK_REG,
	    __BITS(9,8),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),

	/* mmc bus gates */
	SUNXI_CCU_GATE(H616_CLK_BUS_MMC0, "bus-mmc0", "ahb3",
	    SMHC_BGR_REG, 0),
	SUNXI_CCU_GATE(H616_CLK_BUS_MMC1, "bus-mmc1", "ahb3",
	    SMHC_BGR_REG, 1),
	SUNXI_CCU_GATE(H616_CLK_BUS_MMC2, "bus-mmc2", "ahb3",
	    SMHC_BGR_REG, 2),

	/* uart bus gates */
	SUNXI_CCU_GATE(H616_CLK_BUS_UART0, "bus-uart0", "apb2",
	    UART_BGR_REG, 0),
	SUNXI_CCU_GATE(H616_CLK_BUS_UART1, "bus-uart1", "apb2",
	    UART_BGR_REG, 1),
	SUNXI_CCU_GATE(H616_CLK_BUS_UART2, "bus-uart2", "apb2",
	    UART_BGR_REG, 2),
	SUNXI_CCU_GATE(H616_CLK_BUS_UART3, "bus-uart3", "apb2",
	    UART_BGR_REG, 3),
	SUNXI_CCU_GATE(H616_CLK_BUS_UART4, "bus-uart4", "apb2",
	    UART_BGR_REG, 4),
	SUNXI_CCU_GATE(H616_CLK_BUS_UART5, "bus-uart5", "apb2",
	    UART_BGR_REG, 5),

	/* i2c bus gates */
	SUNXI_CCU_GATE(H616_CLK_BUS_I2C0, "bus-i2c0", "apb2",
	    TWI_BGR_REG, 0),
	SUNXI_CCU_GATE(H616_CLK_BUS_I2C1, "bus-i2c1", "apb2",
	    TWI_BGR_REG, 1),
	SUNXI_CCU_GATE(H616_CLK_BUS_I2C2, "bus-i2c2", "apb2",
	    TWI_BGR_REG, 2),
	SUNXI_CCU_GATE(H616_CLK_BUS_I2C3, "bus-i2c3", "apb2",
	    TWI_BGR_REG, 3),
	SUNXI_CCU_GATE(H616_CLK_BUS_I2C4, "bus-i2c4", "apb2",
	    TWI_BGR_REG, 4),

	/* spi clocks */
	SUNXI_CCU_NM(H616_CLK_SPI0, "spi0", mod_parents,
	    SPI0_CLK_REG,
	    __BITS(9,8),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_NM(H616_CLK_SPI1, "spi1", mod_parents,
	    SPI1_CLK_REG,
	    __BITS(9,8),		/* n */
	    __BITS(3,0),		/* m */
	    __BITS(25,24),		/* sel */
	    __BIT(31),			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_GATE(H616_CLK_BUS_SPI0, "bus-spi0", "ahb3",
	    SPI_BGR_REG, 0),
	SUNXI_CCU_GATE(H616_CLK_BUS_SPI1, "bus-spi1", "ahb3",
	    SPI_BGR_REG, 1),

	/* emac */
	SUNXI_CCU_GATE(H616_CLK_EMAC_25M, "emac-25m", "ahb3",
	    EMAC_25M_CLK_REG, 31),
	SUNXI_CCU_GATE(H616_CLK_BUS_EMAC0, "bus-emac0", "ahb3",
	    EMAC_BGR_REG, 0),
	SUNXI_CCU_GATE(H616_CLK_BUS_EMAC1, "bus-emac1", "ahb3",
	    EMAC_BGR_REG, 1),

	/* misc bus gates */
	SUNXI_CCU_GATE(H616_CLK_BUS_DMA, "bus-dma", "psi",
	    DMA_BGR_REG, 0),
	SUNXI_CCU_GATE(H616_CLK_BUS_HSTIMER, "bus-hstimer", "psi",
	    HSTIMER_BGR_REG, 0),
	SUNXI_CCU_GATE(H616_CLK_BUS_DBG, "bus-dbg", "psi",
	    DBGSYS_BGR_REG, 0),
	SUNXI_CCU_GATE(H616_CLK_BUS_PSI, "bus-psi", "psi",
	    PSI_BGR_REG, 0),
	SUNXI_CCU_GATE(H616_CLK_BUS_PWM, "bus-pwm", "apb1",
	    PWM_BGR_REG, 0),
	SUNXI_CCU_GATE(H616_CLK_BUS_IOMMU, "bus-iommu", "apb1",
	    IOMMU_BGR_REG, 0),

	/* dram */
	SUNXI_CCU_GATE(H616_CLK_BUS_DRAM, "bus-dram", "psi",
	    DRAM_BGR_REG, 0),

	/* ths */
	SUNXI_CCU_GATE(H616_CLK_BUS_THS, "bus-ths", "apb1",
	    THS_BGR_REG, 0),

	/* gpadc */
	SUNXI_CCU_GATE(H616_CLK_BUS_GPADC, "bus-gpadc", "apb1",
	    GPADC_BGR_REG, 0),

	/* usb */
	SUNXI_CCU_GATE(H616_CLK_USB_OHCI0, "usb-ohci0", "osc12m",
	    USB0_CLK_REG, 31),
	SUNXI_CCU_GATE(H616_CLK_USB_PHY0, "usb-phy0", "hosc",
	    USB0_CLK_REG, 29),
	SUNXI_CCU_GATE(H616_CLK_USB_OHCI1, "usb-ohci1", "osc12m",
	    USB1_CLK_REG, 31),
	SUNXI_CCU_GATE(H616_CLK_USB_PHY1, "usb-phy1", "hosc",
	    USB1_CLK_REG, 29),
	SUNXI_CCU_GATE(H616_CLK_USB_OHCI2, "usb-ohci2", "osc12m",
	    USB2_CLK_REG, 31),
	SUNXI_CCU_GATE(H616_CLK_USB_PHY2, "usb-phy2", "hosc",
	    USB2_CLK_REG, 29),
	SUNXI_CCU_GATE(H616_CLK_USB_OHCI3, "usb-ohci3", "osc12m",
	    USB3_CLK_REG, 31),
	SUNXI_CCU_GATE(H616_CLK_USB_PHY3, "usb-phy3", "hosc",
	    USB3_CLK_REG, 29),
	SUNXI_CCU_GATE(H616_CLK_BUS_OHCI0, "bus-ohci0", "ahb3",
	    USB_BGR_REG, 0),
	SUNXI_CCU_GATE(H616_CLK_BUS_OHCI1, "bus-ohci1", "ahb3",
	    USB_BGR_REG, 1),
	SUNXI_CCU_GATE(H616_CLK_BUS_OHCI2, "bus-ohci2", "ahb3",
	    USB_BGR_REG, 2),
	SUNXI_CCU_GATE(H616_CLK_BUS_OHCI3, "bus-ohci3", "ahb3",
	    USB_BGR_REG, 3),
	SUNXI_CCU_GATE(H616_CLK_BUS_EHCI0, "bus-ehci0", "ahb3",
	    USB_BGR_REG, 4),
	SUNXI_CCU_GATE(H616_CLK_BUS_EHCI1, "bus-ehci1", "ahb3",
	    USB_BGR_REG, 5),
	SUNXI_CCU_GATE(H616_CLK_BUS_EHCI2, "bus-ehci2", "ahb3",
	    USB_BGR_REG, 6),
	SUNXI_CCU_GATE(H616_CLK_BUS_EHCI3, "bus-ehci3", "ahb3",
	    USB_BGR_REG, 7),
	SUNXI_CCU_GATE(H616_CLK_BUS_OTG, "bus-otg", "ahb3",
	    USB_BGR_REG, 8),
};

static int
sun50i_h616_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sun50i_h616_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun50i_h616_ccu_resets;
	sc->sc_nresets = __arraycount(sun50i_h616_ccu_resets);

	sc->sc_clks = sun50i_h616_ccu_clks;
	sc->sc_nclks = __arraycount(sun50i_h616_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": H616 CCU\n");

	sunxi_ccu_print(sc);
}

/*
 * h616 opp support: accept opp nodes that have plain
 * "opp-microvolt" property. nodes with speed-specific
 * variants (opp-microvolt-speedN) are skipped because
 * the parser only looks for "opp-microvolt".
 */
static bool
sun50i_h616_opp_supported(const int opp_table, const int opp_node)
{
	return of_hasprop(opp_node, "opp-microvolt");
}

FDT_OPP(sun50i_h616_opp, "allwinner,sun50i-h616-operating-points", sun50i_h616_opp_supported);