/* $NetBSD: sun20i_d1_ccu.c,v 1.1 2024/08/13 07:20:23 skrll Exp $ */

/*-
 * Copyright (c) 2024 Rui-Xiang Guo
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

__KERNEL_RCSID(1, "$NetBSD: sun20i_d1_ccu.c,v 1.1 2024/08/13 07:20:23 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <riscv/sunxi/sun20i_d1_ccu.h>

#define	PLL_CPU_CTRL_REG		0x000
#define	PLL_DDR_CTRL_REG		0x010
#define	PLL_PERI_CTRL_REG		0x020
#define	PLL_AUDIO1_CTRL_REG		0x080
#define	PSI_CLK_REG			0x510
#define	APB0_CLK_REG			0x520
#define	APB1_CLK_REG			0x524
#define	MBUS_CLK_REG			0x540
#define	DE_BGR_REG			0x60c
#define	DI_BGR_REG			0x62c
#define	G2D_BGR_REG			0x63c
#define	CE_BGR_REG			0x68c
#define	VE_BGR_REG			0x69c
#define	DMA_BGR_REG			0x70c
#define	MSGBOX_BGR_REG			0x71c
#define	SPINLOCK_BGR_REG		0x72c
#define	HSTIMER_BGR_REG			0x73c
#define	AVS_CLK_REG			0x740
#define	DBGSYS_BGR_REG			0x78c
#define	PWM_BGR_REG			0x7ac
#define	IOMMU_BGR_REG			0x7bc
#define	DRAM_CLK_REG			0x800
#define	MBUS_MAT_CLK_GATING_REG		0x804
#define	DRAM_BGR_REG			0x80c
#define	SMHC0_CLK_REG			0x830
#define	SMHC1_CLK_REG			0x834
#define	SMHC2_CLK_REG			0x838
#define	SMHC_BGR_REG			0x84c
#define	UART_BGR_REG			0x90c
#define	TWI_BGR_REG			0x91c
#define	SPI_BGR_REG			0x96c
#define	EMAC_BGR_REG			0x97c
#define	IRTX_BGR_REG			0x9cc
#define	GPADC_BGR_REG			0x9ec
#define	THS_BGR_REG			0x9fc
#define	I2S_BGR_REG			0xa20
#define	OWA_BGR_REG			0xa2c
#define	DMIC_BGR_REG			0xa4c
#define	AUDIO_CODEC_BGR_REG		0xa5c
#define	USB0_CLK_REG			0xa70
#define	USB1_CLK_REG			0xa74
#define	USB_BGR_REG			0xa8c
#define	LRADC_BGR_REG			0xa9c
#define	DPSS_TOP_BGR_REG		0xabc
#define	HDMI_BGR_REG			0xb1c
#define	DSI_BGR_REG			0xb4c
#define	TCONLCD_BGR_REG			0xb7c
#define	TCONTV_BGR_REG			0xb9c
#define	LVDS_BGR_REG			0xbac
#define	TVE_BGR_REG			0xbbc
#define	TVD_BGR_REG			0xbdc
#define	LEDC_BGR_REG			0xbfc
#define	CSI_BGR_REG			0xc1c
#define	TPADC_BGR_REG			0xc5c
#define	DSP_BGR_REG			0xc7c
#define	RISCV_CFG_BGR_REG		0xd0c

static int sun20i_d1_ccu_match(device_t, cfdata_t, void *);
static void sun20i_d1_ccu_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun20i-d1-ccu" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(sunxi_d1_ccu, sizeof(struct sunxi_ccu_softc),
	sun20i_d1_ccu_match, sun20i_d1_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun20i_d1_ccu_resets[] = {
	SUNXI_CCU_RESET(D1_RST_MBUS, MBUS_CLK_REG, 30),

	SUNXI_CCU_RESET(D1_RST_BUS_DE, DE_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_DI, DI_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_G2D, G2D_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_CE, CE_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_VE, VE_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_DMA, DMA_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_MSGBOX0, MSGBOX_BGR_REG, 16),
	SUNXI_CCU_RESET(D1_RST_BUS_MSGBOX1, MSGBOX_BGR_REG, 17),
	SUNXI_CCU_RESET(D1_RST_BUS_MSGBOX2, MSGBOX_BGR_REG, 18),

	SUNXI_CCU_RESET(D1_RST_BUS_SPINLOCK, SPINLOCK_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_HSTIMER, HSTIMER_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_DBGSYS, DBGSYS_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_PWM, PWM_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_DRAM, DRAM_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_MMC0, SMHC_BGR_REG, 16),
	SUNXI_CCU_RESET(D1_RST_BUS_MMC1, SMHC_BGR_REG, 17),
	SUNXI_CCU_RESET(D1_RST_BUS_MMC2, SMHC_BGR_REG, 18),

	SUNXI_CCU_RESET(D1_RST_BUS_UART0, UART_BGR_REG, 16),
	SUNXI_CCU_RESET(D1_RST_BUS_UART1, UART_BGR_REG, 17),
	SUNXI_CCU_RESET(D1_RST_BUS_UART2, UART_BGR_REG, 18),
	SUNXI_CCU_RESET(D1_RST_BUS_UART3, UART_BGR_REG, 19),
	SUNXI_CCU_RESET(D1_RST_BUS_UART4, UART_BGR_REG, 20),
	SUNXI_CCU_RESET(D1_RST_BUS_UART5, UART_BGR_REG, 21),

	SUNXI_CCU_RESET(D1_RST_BUS_I2C0, TWI_BGR_REG, 16),
	SUNXI_CCU_RESET(D1_RST_BUS_I2C1, TWI_BGR_REG, 17),
	SUNXI_CCU_RESET(D1_RST_BUS_I2C2, TWI_BGR_REG, 18),
	SUNXI_CCU_RESET(D1_RST_BUS_I2C3, TWI_BGR_REG, 19),

	SUNXI_CCU_RESET(D1_RST_BUS_SPI0, SPI_BGR_REG, 16),
	SUNXI_CCU_RESET(D1_RST_BUS_SPI1, SPI_BGR_REG, 17),

	SUNXI_CCU_RESET(D1_RST_BUS_EMAC, EMAC_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_IRTX, IRTX_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_GPADC, GPADC_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_THS, THS_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_I2S0, I2S_BGR_REG, 16),
	SUNXI_CCU_RESET(D1_RST_BUS_I2S1, I2S_BGR_REG, 17),
	SUNXI_CCU_RESET(D1_RST_BUS_I2S2, I2S_BGR_REG, 18),

	SUNXI_CCU_RESET(D1_RST_BUS_SPDIF, OWA_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_DMIC, DMIC_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_AUDIO, AUDIO_CODEC_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_USB_PHY0, USB0_CLK_REG, 30),

	SUNXI_CCU_RESET(D1_RST_USB_PHY1, USB1_CLK_REG, 30),

	SUNXI_CCU_RESET(D1_RST_BUS_OHCI0, USB_BGR_REG, 16),
	SUNXI_CCU_RESET(D1_RST_BUS_OHCI1, USB_BGR_REG, 17),
	SUNXI_CCU_RESET(D1_RST_BUS_EHCI0, USB_BGR_REG, 20),
	SUNXI_CCU_RESET(D1_RST_BUS_EHCI1, USB_BGR_REG, 21),
	SUNXI_CCU_RESET(D1_RST_BUS_OTG, USB_BGR_REG, 24),

	SUNXI_CCU_RESET(D1_RST_BUS_LRADC, LRADC_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_DPSS_TOP, DPSS_TOP_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_HDMI_SUB, HDMI_BGR_REG, 17),
	SUNXI_CCU_RESET(D1_RST_BUS_HDMI_MAIN, HDMI_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_DSI, DSI_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_TCONLCD, TCONLCD_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_TCONTV, TCONTV_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_LVDS, LVDS_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_TVE_TOP, TVE_BGR_REG, 16),
	SUNXI_CCU_RESET(D1_RST_BUS_TVE, TVE_BGR_REG, 17),

	SUNXI_CCU_RESET(D1_RST_BUS_TVD_TOP, TVD_BGR_REG, 16),
	SUNXI_CCU_RESET(D1_RST_BUS_TVD, TVD_BGR_REG, 17),

	SUNXI_CCU_RESET(D1_RST_BUS_LEDC, LEDC_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_CSI, CSI_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_TPADC, TPADC_BGR_REG, 16),

	SUNXI_CCU_RESET(D1_RST_BUS_DSP, DSP_BGR_REG, 16),
	SUNXI_CCU_RESET(D1_RST_BUS_DSP_CFG, DSP_BGR_REG, 17),
	SUNXI_CCU_RESET(D1_RST_BUS_DSP_DBG, DSP_BGR_REG, 18),

	SUNXI_CCU_RESET(D1_RST_BUS_RISCV_CFG, RISCV_CFG_BGR_REG, 16),
};

static const char *psi_parents[] = { "hosc", "losc", "iosc", "pll_periph" };
static const char *apb_parents[] = { "hosc", "losc", "psi", "pll_periph" };
static const char *dram_parents[] = { "pll_ddr", "pll_audio1_div2", "pll_periph_2x", "pll_periph_800m" };
static const char *mmc_parents[] = { "hosc", "pll_periph", "pll_periph_2x", "pll_audio1_div2" };
static const char *mmc2_parents[] = { "pll_periph", "pll_periph_2x", "pll_periph_800m", "pll_audio1_div2" };

static struct sunxi_ccu_clk sun20i_d1_ccu_clks[] = {
	SUNXI_CCU_NKMP_TABLE(D1_CLK_PLL_CPU, "pll_cpu", "hosc",
	    PLL_CPU_CTRL_REG,		/* reg */
	    __BITS(15,8),		/* n */
	    0,				/* k */
	    0,				/* m */
	    0,				/* p */
	    __BIT(31),			/* enable */
	    __BIT(28),			/* lock */
	    NULL,			/* table */
	    SUNXI_CCU_NKMP_SCALE_CLOCK),

	SUNXI_CCU_NKMP_TABLE(D1_CLK_PLL_DDR, "pll_ddr", "hosc",
	    PLL_DDR_CTRL_REG,		/* reg */
	    __BITS(15,8),		/* n */
	    0,				/* k */
	    __BIT(1),			/* m */
	    __BIT(0),			/* p */
	    __BIT(31),			/* enable */
	    __BIT(28),			/* lock */
	    NULL,			/* table */
	    SUNXI_CCU_NKMP_SCALE_CLOCK),

	SUNXI_CCU_NKMP_TABLE(D1_CLK_PLL_PERIPH_2X, "pll_periph_2x", "hosc",
	    PLL_PERI_CTRL_REG,		/* reg */
	    __BITS(15,8),		/* n */
	    0, 				/* k */
	    __BIT(1),			/* m */
	    __BITS(18,16),		/* p */
	    __BIT(31),			/* enable */
	    __BIT(28),			/* lock */
	    NULL,			/* table */
	    SUNXI_CCU_NKMP_SCALE_CLOCK),

	SUNXI_CCU_NKMP_TABLE(D1_CLK_PLL_PERIPH_800M, "pll_periph_800m", "hosc",
	    PLL_PERI_CTRL_REG,		/* reg */
	    __BITS(15,8),		/* n */
	    0, 				/* k */
	    __BIT(1),			/* m */
	    __BITS(22,20),		/* p */
	    __BIT(31),			/* enable */
	    __BIT(28),			/* lock */
	    NULL,			/* table */
	    SUNXI_CCU_NKMP_SCALE_CLOCK),

	SUNXI_CCU_FIXED_FACTOR(D1_CLK_PLL_PERIPH, "pll_periph", "pll_periph_2x", 2, 1),

	SUNXI_CCU_NKMP_TABLE(D1_CLK_PLL_AUDIO1, "pll_audio1_div2", "hosc",
	    PLL_AUDIO1_CTRL_REG,	/* reg */
	    __BITS(15,8),		/* n */
	    0, 				/* k */
	    __BIT(1),			/* m */
	    __BITS(18,16),		/* p */
	    __BIT(31),			/* enable */
	    __BIT(28),			/* lock */
	    NULL,			/* table */
	    SUNXI_CCU_NKMP_SCALE_CLOCK),

	SUNXI_CCU_NM(D1_CLK_PSI, "psi", psi_parents,
	    PSI_CLK_REG,	/* reg */
	    __BITS(9,8),	/* n */
	    __BITS(1,0),	/* m */
	    __BITS(25,24),	/* sel */
	    0,			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),

	SUNXI_CCU_NM(D1_CLK_APB0, "apb0", apb_parents,
	    APB0_CLK_REG,	/* reg */
	    __BITS(9,8),	/* n */
	    __BITS(4,0),	/* m */
	    __BITS(25,24),	/* sel */
	    0,			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),

	SUNXI_CCU_NM(D1_CLK_APB1, "apb1", apb_parents,
	    APB1_CLK_REG,	/* reg */
	    __BITS(9,8),	/* n */
	    __BITS(4,0),	/* m */
	    __BITS(25,24),	/* sel */
	    0,			/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),

	SUNXI_CCU_FIXED_FACTOR(D1_CLK_MBUS, "mbus", "dram", 4, 1),

	SUNXI_CCU_NM(D1_CLK_DRAM, "dram", dram_parents,
	    DRAM_CLK_REG,	/* reg */
	    __BITS(9,8),	/* n */
	    __BITS(1,0),	/* m */
	    __BITS(26,24),	/* sel */
	    __BIT(31),		/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),

	SUNXI_CCU_NM(D1_CLK_MMC0, "mmc0", mmc_parents,
	    SMHC0_CLK_REG,	/* reg */
	    __BITS(9,8),	/* n */
	    __BITS(3,0),	/* m */
	    __BITS(26,24),	/* sel */
	    __BIT(31),		/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_NM(D1_CLK_MMC1, "mmc1", mmc_parents,
	    SMHC1_CLK_REG,	/* reg */
	    __BITS(9,8),	/* n */
	    __BITS(3,0),	/* m */
	    __BITS(26,24),	/* sel */
	    __BIT(31),		/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),
	SUNXI_CCU_NM(D1_CLK_MMC2, "mmc2", mmc2_parents,
	    SMHC2_CLK_REG,	/* reg */
	    __BITS(9,8),	/* n */
	    __BITS(3,0),	/* m */
	    __BITS(26,24),	/* sel */
	    __BIT(31),		/* enable */
	    SUNXI_CCU_NM_POWER_OF_TWO|SUNXI_CCU_NM_ROUND_DOWN),

	SUNXI_CCU_GATE(D1_CLK_BUS_DE, "bus-de", "psi",
	    DE_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_DI, "bus-di", "psi",
	    DI_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_G2D, "bus-g2d", "psi",
	    G2D_BGR_REG ,0),

	SUNXI_CCU_GATE(D1_CLK_BUS_CE, "bus-ce", "psi",
	    CE_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_VE, "bus-ve", "psi",
	    VE_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_DMA, "bus-dma", "psi",
	    DMA_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_MSGBOX0, "bus-msgbox0", "psi",
	    MSGBOX_BGR_REG, 0),
	SUNXI_CCU_GATE(D1_CLK_BUS_MSGBOX1, "bus-msgbox1", "psi",
	    MSGBOX_BGR_REG, 1),
	SUNXI_CCU_GATE(D1_CLK_BUS_MSGBOX2, "bus-msgbox2", "psi",
	    MSGBOX_BGR_REG, 2),

	SUNXI_CCU_GATE(D1_CLK_BUS_SPINLOCK, "bus-spinlock", "psi",
	    SPINLOCK_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_HSTIMER, "bus-hstimer", "psi",
	    HSTIMER_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_AVS, "avs", "hosc",
	    AVS_CLK_REG, 31),

	SUNXI_CCU_GATE(D1_CLK_BUS_DBGSYS, "bus-dbgsys", "psi",
	    DBGSYS_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_PWM, "bus-pwm", "apb0",
	    PWM_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_IOMMU, "bus-iommu", "apb0",
	    IOMMU_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_MBUS_DMA, "mbus-dma", "mbus",
	    MBUS_MAT_CLK_GATING_REG, 0),
	SUNXI_CCU_GATE(D1_CLK_MBUS_VE, "mbus-ve", "mbus",
	    MBUS_MAT_CLK_GATING_REG, 1),
	SUNXI_CCU_GATE(D1_CLK_MBUS_CE, "mbus-ce", "mbus",
	    MBUS_MAT_CLK_GATING_REG, 2),
	SUNXI_CCU_GATE(D1_CLK_MBUS_TVIN, "mbus-tvin", "mbus",
	    MBUS_MAT_CLK_GATING_REG, 7),
	SUNXI_CCU_GATE(D1_CLK_MBUS_CSI, "mbus-csi", "mbus",
	    MBUS_MAT_CLK_GATING_REG, 8),
	SUNXI_CCU_GATE(D1_CLK_MBUS_G2D, "mbus-g2d", "mbus",
	    MBUS_MAT_CLK_GATING_REG, 10),
	SUNXI_CCU_GATE(D1_CLK_MBUS_RISCV, "mbus-riscv", "mbus",
	    MBUS_MAT_CLK_GATING_REG, 11),

	SUNXI_CCU_GATE(D1_CLK_BUS_DRAM, "bus-dram", "psi",
	    DRAM_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_MMC0, "bus-mmc0", "psi",
	    SMHC_BGR_REG, 0),
	SUNXI_CCU_GATE(D1_CLK_BUS_MMC1, "bus-mmc1", "psi",
	    SMHC_BGR_REG, 1),
	SUNXI_CCU_GATE(D1_CLK_BUS_MMC2, "bus-mmc2", "psi",
	    SMHC_BGR_REG, 2),

	SUNXI_CCU_GATE(D1_CLK_BUS_UART0, "bus-uart0", "apb1",
	    UART_BGR_REG, 0),
	SUNXI_CCU_GATE(D1_CLK_BUS_UART1, "bus-uart1", "apb1",
	    UART_BGR_REG, 1),
	SUNXI_CCU_GATE(D1_CLK_BUS_UART2, "bus-uart2", "apb1",
	    UART_BGR_REG, 2),
	SUNXI_CCU_GATE(D1_CLK_BUS_UART3, "bus-uart3", "apb1",
	    UART_BGR_REG, 3),
	SUNXI_CCU_GATE(D1_CLK_BUS_UART4, "bus-uart4", "apb1",
	    UART_BGR_REG, 4),
	SUNXI_CCU_GATE(D1_CLK_BUS_UART5, "bus-uart5", "apb1",
	    UART_BGR_REG, 5),

	SUNXI_CCU_GATE(D1_CLK_BUS_I2C0, "bus-i2c0", "apb1",
	    TWI_BGR_REG, 0),
	SUNXI_CCU_GATE(D1_CLK_BUS_I2C1, "bus-i2c1", "apb1",
	    TWI_BGR_REG, 1),
	SUNXI_CCU_GATE(D1_CLK_BUS_I2C2, "bus-i2c2", "apb1",
	    TWI_BGR_REG, 2),
	SUNXI_CCU_GATE(D1_CLK_BUS_I2C3, "bus-i2c3", "apb1",
	    TWI_BGR_REG, 3),

	SUNXI_CCU_GATE(D1_CLK_BUS_SPI0, "bus-spi0", "psi",
	    SPI_BGR_REG, 0),
	SUNXI_CCU_GATE(D1_CLK_BUS_SPI1, "bus-spi1", "psi",
	    SPI_BGR_REG, 1),

	SUNXI_CCU_GATE(D1_CLK_BUS_EMAC, "bus-emac", "psi",
	    EMAC_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_IRTX, "bus-irtx", "apb0",
	    IRTX_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_GPADC, "bus-gpadc", "apb0",
	    GPADC_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_THS, "bus-ths", "apb0",
	    THS_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_I2S0, "bus-i2s0", "apb0",
	    I2S_BGR_REG, 0),
	SUNXI_CCU_GATE(D1_CLK_BUS_I2S1, "bus-i2s1", "apb0",
	    I2S_BGR_REG, 1),
	SUNXI_CCU_GATE(D1_CLK_BUS_I2S2, "bus-i2s2", "apb0",
	    I2S_BGR_REG, 2),

	SUNXI_CCU_GATE(D1_CLK_BUS_SPDIF, "bus-spdif", "apb0",
	    OWA_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_DMIC, "bus-dmic", "apb0",
	    DMIC_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_AUDIO, "bus-audio", "apb0",
	    AUDIO_CODEC_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_OHCI0, "bus-ohci0", "psi",
	    USB_BGR_REG, 0),
	SUNXI_CCU_GATE(D1_CLK_BUS_OHCI1, "bus-ohci1", "psi",
	    USB_BGR_REG, 1),
	SUNXI_CCU_GATE(D1_CLK_BUS_EHCI0, "bus-ehci0", "psi",
	    USB_BGR_REG, 4),
	SUNXI_CCU_GATE(D1_CLK_BUS_EHCI1, "bus-ehci1", "psi",
	    USB_BGR_REG, 5),
	SUNXI_CCU_GATE(D1_CLK_BUS_OTG, "bus-otg", "psi",
	    USB_BGR_REG, 8),

	SUNXI_CCU_GATE(D1_CLK_BUS_LRADC, "bus-lradc", "apb0",
	    LRADC_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_DPSS_TOP, "bus-dpss-top", "psi",
	    DPSS_TOP_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_HDMI, "bus-hdmi", "psi",
	    HDMI_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_DSI, "bus-dsi", "psi",
	    DSI_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_TCONLCD, "bus-tconlcd", "psi",
	    TCONLCD_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_TCONTV, "bus-tcontv", "psi",
	    TCONTV_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_TVE_TOP, "bus-tve-top", "psi",
	    TVE_BGR_REG, 0),
	SUNXI_CCU_GATE(D1_CLK_BUS_TVE, "bus-tve", "psi",
	    TVE_BGR_REG, 1),

	SUNXI_CCU_GATE(D1_CLK_BUS_TVD_TOP, "bus-tvd-top", "psi",
	    TVD_BGR_REG, 0),
	SUNXI_CCU_GATE(D1_CLK_BUS_TVD, "bus-tvd", "psi",
	    TVD_BGR_REG, 1),

	SUNXI_CCU_GATE(D1_CLK_BUS_LEDC, "bus-ledc", "psi",
	    LEDC_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_CSI, "bus-csi", "psi",
	    CSI_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_TPADC, "bus-tpadc", "apb0",
	    TPADC_BGR_REG, 0),

	SUNXI_CCU_GATE(D1_CLK_BUS_DSP_CFG, "bus-dsp-cfg", "psi",
	    DSP_BGR_REG, 1),

	SUNXI_CCU_GATE(D1_CLK_BUS_RISCV_CFG, "bus-riscv-cfg", "psi",
	    RISCV_CFG_BGR_REG, 0),
};

static int
sun20i_d1_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sun20i_d1_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun20i_d1_ccu_resets;
	sc->sc_nresets = __arraycount(sun20i_d1_ccu_resets);

	sc->sc_clks = sun20i_d1_ccu_clks;
	sc->sc_nclks = __arraycount(sun20i_d1_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": D1 CCU\n");

	sunxi_ccu_print(sc);
}
