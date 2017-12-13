/* $NetBSD: mtk_gpio.c,v 1.1.2.1 2017/12/13 01:06:02 matt Exp $ */

/*-
 * Copyright (c) 2017 Biao Huang <biao.huang@mediatek.com>
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
__KERNEL_RCSID(0, "$NetBSD: mtk_gpio.c,v 1.1.2.1 2017/12/13 01:06:02 matt Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/fdt/fdtvar.h>
#include <dev/gpio/gpiovar.h>

#include <arm/mediatek/mtk_gpio.h>
#include <arm/mediatek/mercury_reg.h>

#define MAX_GPIO_MODE_PER_REG		5
#define GPIO_MODE_BITS				3
#define	MERCURY_GPIO_PORT(pin)		((pin >> 4) << 4)
#define	MERCURY_GPIO_DIR(pin)		(MERCURY_GPIO_PORT(pin) + 0x0)
#define	MERCURY_GPIO_DOUT(pin)		(MERCURY_GPIO_PORT(pin) + 0x100)
#define	MERCURY_GPIO_DATA(pin)		(MERCURY_GPIO_PORT(pin) + 0x200)
#define	MERCURY_GPIO_PINMUX(pin)	(((pin / 5) << 4) + 0x300)
#define	MERCURY_GPIO_PUEN(pin)		(MERCURY_GPIO_PORT(pin) + 0x500)
#define	MERCURY_GPIO_PUSEL(pin)		(MERCURY_GPIO_PORT(pin) + 0x600)

#define _PIN(b, gp, m0, m1, m2, m3, m4, m5, m6, m7, em, en) \
	{	.ballname = b, \
		.pin = gp, \
		.functions[0] = m0, \
		.functions[1] = m1, \
		.functions[2] = m2, \
		.functions[3] = m3, \
		.functions[4] = m4, \
		.functions[5] = m5, \
		.functions[6] = m6, \
		.functions[7] = m7, \
		.eint_func = em, \
		.eint_num = en, \
	}

const struct mercury_gpio_pins mercury_pins[] = {
	_PIN("EINT0", 0, "GPIO", "PWM_B", "DPI_CK", "I2S2_BCK", "EXT_TXD0", NULL, "SQICS", "DBG_MON_A[6]", 0, 0),
	_PIN("EINT1", 1, "GPIO", "PWM_C", "DPI_D12", "I2S2_DI", "EXT_TXD1", "CONN_MCU_TDO", "SQISO", "DBG_MON_A[7]", 0, 1),
	_PIN("EINT2", 2, "GPIO", "CLKM0", "DPI_D13", "I2S2_LRCK", "EXT_TXD2", "CONN_MCU_DBGACK_N", "SQISI", "DBG_MON_A[8]", 0, 2),
	_PIN("EINT3", 3, "GPIO", "CLKM1", "DPI_D14", "SPI_MI", "EXT_TXD3", "CONN_MCU_DBGI_N", "SQIWP", "DBG_MON_A[9]", 0, 3),
	_PIN("EINT4", 4, "GPIO", "CLKM2", "DPI_D15", "SPI_MO", "EXT_TXC", "CONN_MCU_TCK", "CONN_MCU_AICE_JCKC", "DBG_MON_A[10]", 0, 4),
	_PIN("EINT5", 5, "GPIO", "UCTS2", "DPI_D16", "SPI_CSB", "EXT_RXER", "CONN_MCU_TDI", "CONN_TEST_CK", "DBG_MON_A[11]", 0, 5),
	_PIN("EINT6", 6, "GPIO", "URTS2", "DPI_D17", "SPI_CLK", "EXT_RXC", "CONN_MCU_TRST_B", "MM_TEST_CK", "DBG_MON_A[12]", 0, 6),
	_PIN("EINT7", 7, "GPIO", "SQIRST", "DPI_D6", "SDA1_0", "EXT_RXDV", "CONN_MCU_TMS", "CONN_MCU_AICE_JMSC", "DBG_MON_A[13]", 0, 7),
	_PIN("EINT8", 8, "GPIO", "SQICK", "CLKM3", "SCL1_0", "EXT_RXD0", "ANT_SEL0", "DPI_D7", "DBG_MON_A[14]", 0, 8),
	_PIN("EINT9", 9, "GPIO", "CLKM4", "SDA2_0", "EXT_FRAME_SYNC", "EXT_RXD1", "ANT_SEL1", "DPI_D8", "DBG_MON_A[15]", 0, 9),
	_PIN("EINT10", 10, "GPIO", "CLKM5", "SCL2_0", "EXT_FRAME_SYNC", "EXT_RXD2", "ANT_SEL2", "DPI_D9", "DBG_MON_A[16]", 0, 10),
	_PIN("EINT11", 11, "GPIO", "CLKM4", "PWM_C", "CONN_TEST_CK", "ANT_SEL3", "DPI_D10", "EXT_RXD3", "DBG_MON_A[17]", 0, 11),
	_PIN("EINT12", 12, "GPIO", "CLKM5", "PWM_A", "SPDIF_OUT", "ANT_SEL4", "DPI_D11", "EXT_TXEN", "DBG_MON_A[18]", 0, 12),
	_PIN("EINT13", 13, "GPIO", NULL, NULL, "TSF_IN", "ANT_SEL5", "DPI_D0", "SPDIF_IN", "DBG_MON_A[19]", 0, 13),
	_PIN("EINT14", 14, "GPIO", NULL, "I2S_8CH_DO1", "TDM_RX_MCK", "ANT_SEL1", "CONN_MCU_DBGACK_N", "NCLE", "DBG_MON_B[8]", 0, 14),
	_PIN("EINT15", 15, "GPIO", NULL, "I2S_8CH_LRCK", "TDM_RX_BCK", "ANT_SEL2", "CONN_MCU_DBGI_N", "NCEB1", "DBG_MON_B[9]", 0, 15),
	_PIN("EINT16", 16, "GPIO", NULL, "I2S_8CH_BCK", "TDM_RX_LRCK", "ANT_SEL3", "CONN_MCU_TRST_B", "NCEB0", "DBG_MON_B[10]", 0, 16),
	_PIN("EINT17", 17, "GPIO", NULL, "I2S_8CH_MCK", "TDM_RX_DI", "IDDIG", "ANT_SEL4", "NREB", "DBG_MON_B[11]", 0, 17),
	_PIN("EINT18", 18, "GPIO", NULL, "USB_DRVVBUS", "I2S3_LRCK", "CLKM1", "ANT_SEL3", "I2S2_BCK", "DBG_MON_A[20]", 0, 18),
	_PIN("EINT19", 19, "GPIO", "UCTS1", "IDDIG", "I2S3_BCK", "CLKM2", "ANT_SEL4", "I2S2_DI", "DBG_MON_A[21]", 0, 19),
	_PIN("EINT20", 20, "GPIO", "URTS1", NULL, "I2S3_DO", "CLKM3", "ANT_SEL5", "I2S2_LRCK", "DBG_MON_A[22]", 0, 20),
	_PIN("EINT21", 21, "GPIO", "NRNB", "ANT_SEL0", "I2S_8CH_DO4", NULL, NULL, NULL, "DBG_MON_B[31]", 0, 21),
	_PIN("EINT22", 22, "GPIO", NULL, "I2S_8CH_DO2", "TSF_IN", "USB_DRVVBUS", "SPDIF_OUT", "NRE_C", "DBG_MON_B[12]", 0, 22),
	_PIN("EINT23", 23, "GPIO", NULL, "I2S_8CH_DO3", "CLKM0", "IR", "SPDIF_IN", "B*NDQS_C", "DBG_MON_B[13]", 0, 23),
	_PIN("EINT24", 24, "GPIO", "DPI_D20", "DPI_DE", "ANT_SEL1", "UCTS2", "PWM_A", "I2S0_MCK", "DBG_MON_A[0]", 0, 24),
	_PIN("EINT25", 25, "GPIO", "DPI_D19", "DPI_VSYNC", "ANT_SEL0", "URTS2", "PWM_B", "I2S_8CH_MCK", "DBG_MON_A[1]", 0, 25),
	_PIN("PWRAP_SPI0_MI", 26, "GPIO", "PWRAP_SPI0_MO", "PWRAP_SPI0_MI", NULL, NULL, NULL, NULL, NULL, 0, 26),
	_PIN("PWRAP_SPI0_MO", 27, "GPIO", "PWRAP_SPI0_MI", "PWRAP_SPI0_MO", NULL, NULL, NULL, NULL, NULL, 0, 27),
	_PIN("PWRAP_INT", 28, "GPIO", "I2S0_MCK", NULL, NULL, "I2S_8CH_MCK", "I2S2_MCK", "I2S3_MCK", NULL, 0, 28),
	_PIN("PWRAP_SPI0_CK", 29, "GPIO", "PWRAP_SPI0_CK", NULL, NULL, NULL, NULL, NULL, NULL, 0, 29),
	_PIN("PWRAP_SPI0_CSN", 30, "GPIO", "PWRAP_SPI0_CSN", NULL, NULL, NULL, NULL, NULL, NULL, 0, 30),
	_PIN("RTC32K_CK", 31, "GPIO", "RTC32K_CK", NULL, NULL, NULL, NULL, NULL, NULL, 0, 31),
	_PIN("WATCHDOG", 32, "GPIO", "WATCHDOG", NULL, NULL, NULL, NULL, NULL, NULL, 0, 32),
	_PIN("SRCLKENA", 33, "GPIO", "SRCLKENA0", NULL, NULL, NULL, NULL, NULL, NULL, 0, 33),
	_PIN("URXD2", 34, "GPIO", "URXD2", "DPI_D5", "UTXD2", "DBG_SCL", NULL, "I2S2_MCK", "DBG_MON_B[0]", 0, 34),
	_PIN("UTXD2", 35, "GPIO", "UTXD2", "DPI_HSYNC", "URXD2", "DBG_SDA", "DPI_D18", "I2S3_MCK", "DBG_MON_B[1]", 0, 35),
	_PIN("MRG_CLK", 36, "GPIO", "MRG_CLK", "DPI_D4", "I2S0_BCK", "I2S3_BCK", "PCM0_CLK", "IR", "DBG_MON_A[2]", 0, 36),
	_PIN("MRG_SYNC", 37, "GPIO", "MRG_SYNC", "DPI_D3", "I2S0_LRCK", "I2S3_LRCK", "PCM0_SYNC", "EXT_COL", "DBG_MON_A[3]", 0, 37),
	_PIN("MRG_DI", 38, "GPIO", "MRG_DI", "DPI_D1", "I2S0_DI", "I2S3_DO", "PCM0_DI", "EXT_MDIO", "DBG_MON_A[4]", 0, 38),
	_PIN("MRG_DO", 39, "GPIO", "MRG_DO", "DPI_D2", "I2S0_MCK", "I2S3_MCK", "PCM0_DO", "EXT_MDC", "DBG_MON_A[5]", 0, 39),
	_PIN("KPROW0", 40, "GPIO", "KPROW0", NULL, NULL, "IMG_TEST_CK", NULL, NULL, "DBG_MON_B[4]", 0, 40),
	_PIN("KPROW1", 41, "GPIO", "KPROW1", "IDDIG", "EXT_FRAME_SYNC", "MFG_TEST_CK", NULL, NULL, "DBG_MON_B[5]", 0, 41),
	_PIN("KPCOL0", 42, "GPIO", "KPCOL0", NULL, NULL, NULL, NULL, NULL, "DBG_MON_B[6]", 0, 42),
	_PIN("KPCOL1", 43, "GPIO", "KPCOL1", "USB_DRVVBUS", "EXT_FRAME_SYNC", "TSF_IN", "DFD_NTRST_XI", "UDI_NTRST_XI", "DBG_MON_B[7]", 0, 43),
	_PIN("JTMS", 44, "GPIO", "JTMS", "CONN_MCU_TMS", "CONN_MCU_AICE_JMSC", "GPUDFD_TMS_XI", "DFD_TMS_XI", "UDI_TMS_XI", NULL, 0, 44),
	_PIN("JTCK", 45, "GPIO", "JTCK", "CONN_MCU_TCK", "CONN_MCU_AICE_JCKC", "GPUDFD_TCK_XI", "DFD_TCK_XI", "UDI_TCK_XI", NULL, 0, 45),
	_PIN("JTDI", 46, "GPIO", "JTDI", "CONN_MCU_TDI", NULL, "GPUDFD_TDI_XI", "DFD_TDI_XI", "UDI_TDI_XI", NULL, 0, 46),
	_PIN("JTDO", 47, "GPIO", "JTDO", "CONN_MCU_TDO", NULL, "GPUDFD_TDO", "DFD_TDO", "UDI_TDO", NULL, 0, 47),
	_PIN("SPI_CS", 48, "GPIO", "SPI_CSB", NULL, "I2S0_DI", "I2S2_BCK", NULL, NULL, "DBG_MON_A[23]", 0, 48),
	_PIN("SPI_CK", 49, "GPIO", "SPI_CLK", NULL, "I2S0_LRCK", "I2S2_DI", NULL, NULL, "DBG_MON_A[24]", 0, 49),
	_PIN("SPI_MI", 50, "GPIO", "SPI_MI", "SPI_MO", "I2S0_BCK", "I2S2_LRCK", NULL, NULL, "DBG_MON_A[25]", 0, 50),
	_PIN("SPI_MO", 51, "GPIO", "SPI_MO", "SPI_MI", "I2S0_MCK", "I2S2_MCK", NULL, NULL, "DBG_MON_A[26]", 0, 51),
	_PIN("SDA1", 52, "GPIO", "SDA1_0", NULL, NULL, NULL, NULL, NULL, NULL, 0, 52),
	_PIN("SCL1", 53, "GPIO", "SCL1_0", NULL, NULL, NULL, NULL, NULL, NULL, 0, 53),
	_PIN("DISP_PWM", 54, "GPIO", "DISP_PWM", "PWM_B", NULL, NULL, NULL, NULL, "DBG_MON_B[2]", 0, 54),
	_PIN("I2S_DATA_IN", 55, "GPIO", "I2S0_DI", "UCTS0", "I2S3_DO", "I2S_8CH_DO1", "PWM_A", "I2S2_BCK", "DBG_MON_A[28]", 0, 55),
	_PIN("I2S_LRCK", 56, "GPIO", "I2S0_LRCK", NULL, "I2S3_LRCK", "I2S_8CH_LRCK", "PWM_B", "I2S2_DI", "DBG_MON_A[29]", 0, 56),
	_PIN("I2S_BCK", 57, "GPIO", "I2S0_BCK", "URTS0", "I2S3_BCK", "I2S_8CH_BCK", "PWM_C", "I2S2_LRCK", "DBG_MON_A[30]", 0, 57),
	_PIN("SDA0", 58, "GPIO", "SDA0_0", NULL, NULL, NULL, NULL, NULL, NULL, 0, 58),
	_PIN("SCL0", 59, "GPIO", "SCL0_0", NULL, NULL, NULL, NULL, NULL, NULL, 0, 59),
	_PIN("SDA2", 60, "GPIO", "SDA2_0", "PWM_B", NULL, NULL, NULL, NULL, NULL, 0, 60),
	_PIN("SCL2", 61, "GPIO", "SCL2_0", "PWM_C", NULL, NULL, NULL, NULL, NULL, 0, 61),
	_PIN("URXD0", 62, "GPIO", "URXD0", "UTXD0", NULL, NULL, NULL, NULL, NULL, 0, 62),
	_PIN("UTXD0", 63, "GPIO", "UTXD0", "URXD0", NULL, NULL, NULL, NULL, NULL, 0, 63),
	_PIN("URXD1", 64, "GPIO", "URXD1", "UTXD1", NULL, NULL, NULL, NULL, "DBG_MON_A[27]", 0, 64),
	_PIN("UTXD1", 65, "GPIO", "UTXD1", "URXD1", NULL, NULL, NULL, NULL, "DBG_MON_A[31]", 0, 65),
	_PIN("LCM_RST", 66, "GPIO", "LCM_RST", NULL, "I2S0_MCK", NULL, NULL, NULL, "DBG_MON_B[3]", 0, 66),
	_PIN("DSI_TE", 67, "GPIO", "DSI_TE", NULL, "I2S_8CH_MCK", NULL, NULL, NULL, "DBG_MON_B[14]", 0, 67),
	_PIN("MSDC2_CMD", 68, "GPIO", "MSDC2_CMD", "I2S_8CH_DO4", "SDA1_0", NULL, "USB_SDA", "I2S3_BCK", "DBG_MON_B[15]", 0, 68),
	_PIN("MSDC2_CLK", 69, "GPIO", "MSDC2_CLK", "I2S_8CH_DO3", "SCL1_0", "DPI_D21", "USB_SCL", "I2S3_LRCK", "DBG_MON_B[16]", 0, 69),
	_PIN("MSDC2_DAT0", 70, "GPIO", "MSDC2_DAT0", "I2S_8CH_DO2", NULL, "DPI_D22", "UTXD0", "I2S3_DO", "DBG_MON_B[17]", 0, 70),
	_PIN("MSDC2_DAT1", 71, "GPIO", "MSDC2_DAT1", "I2S_8CH_DO1", "PWM_A", "I2S3_MCK", "URXD0", "PWM_B", "DBG_MON_B[18]", 0, 71),
	_PIN("MSDC2_DAT2", 72, "GPIO", "MSDC2_DAT2", "I2S_8CH_LRCK", "SDA2_0", "DPI_D23", "UTXD1", "PWM_C", "DBG_MON_B[19]", 0, 72),
	_PIN("MSDC2_DAT3", 73, "GPIO", "MSDC2_DAT3", "I2S_8CH_BCK", "SCL2_0", "EXT_FRAME_SYNC", "URXD1", "PWM_A", "DBG_MON_B[20]", 0, 73),
	_PIN("TDN3", 74, "GPI", "TDN3", NULL, NULL, NULL, NULL, NULL, NULL, 0, 74),
	_PIN("TDP3", 75, "GPI", "TDP3", NULL, NULL, NULL, NULL, NULL, NULL, 0, 75),
	_PIN("TDN2", 76, "GPI", "TDN2", NULL, NULL, NULL, NULL, NULL, NULL, 0, 76),
	_PIN("TDP2", 77, "GPI", "TDP2", NULL, NULL, NULL, NULL, NULL, NULL, 0, 77),
	_PIN("TCN", 78, "GPI", "TCN", NULL, NULL, NULL, NULL, NULL, NULL, 0, 78),
	_PIN("TCP", 79, "GPI", "TCP", NULL, NULL, NULL, NULL, NULL, NULL, 0, 79),
	_PIN("TDN1", 80, "GPI", "TDN1", NULL, NULL, NULL, NULL, NULL, NULL, 0, 80),
	_PIN("TDP1", 81, "GPI", "TDP1", NULL, NULL, NULL, NULL, NULL, NULL, 0, 81),
	_PIN("TDN0", 82, "GPI", "TDN0", NULL, NULL, NULL, NULL, NULL, NULL, 0, 82),
	_PIN("TDP0", 83, "GPI", "TDP0", NULL, NULL, NULL, NULL, NULL, NULL, 0, 83),
	_PIN("RDN0", 84, "GPI", "RDN0", NULL, NULL, NULL, NULL, NULL, NULL, 0, 84),
	_PIN("RDP0", 85, "GPI", "RDP0", NULL, NULL, NULL, NULL, NULL, NULL, 0, 85),
	_PIN("RDN1", 86, "GPI", "RDN1", NULL, NULL, NULL, NULL, NULL, NULL, 0, 86),
	_PIN("RDP1", 87, "GPI", "RDP1", NULL, NULL, NULL, NULL, NULL, NULL, 0, 87),
	_PIN("RCN", 88, "GPI", "RCN", NULL, NULL, NULL, NULL, NULL, NULL, 0, 88),
	_PIN("RCP", 89, "GPI", "RCP", NULL, NULL, NULL, NULL, NULL, NULL, 0, 89),
	_PIN("RDN2", 90, "GPI", "RDN2", "CMDAT8", NULL, NULL, NULL, NULL, NULL, 0, 90),
	_PIN("RDP2", 91, "GPI", "RDP2", "CMDAT9", NULL, NULL, NULL, NULL, NULL, 0, 91),
	_PIN("RDN3", 92, "GPI", "RDN3", "CMDAT4", NULL, NULL, NULL, NULL, NULL, 0, 92),
	_PIN("RDP3", 93, "GPI", "RDP3", "CMDAT5", NULL, NULL, NULL, NULL, NULL, 0, 93),
	_PIN("RCN_A", 94, "GPI", "RCN_A", "CMDAT6", NULL, NULL, NULL, NULL, NULL, 0, 94),
	_PIN("RCP_A", 95, "GPI", "RCP_A", "CMDAT7", NULL, NULL, NULL, NULL, NULL, 0, 95),
	_PIN("RDN1_A", 96, "GPI", "RDN1_A", "CMDAT2", "CMCSD2", NULL, NULL, NULL, NULL, 0, 96),
	_PIN("RDP1_A", 97, "GPI", "RDP1_A", "CMDAT3", "CMCSD3", NULL, NULL, NULL, NULL, 0, 97),
	_PIN("RDN0_A", 98, "GPI", "RDN0_A", "CMHSYNC", NULL, NULL, NULL, NULL, NULL, 0, 98),
	_PIN("RDP0_A", 99, "GPI", "RDP0_A", "CMVSYNC", NULL, NULL, NULL, NULL, NULL, 0, 99),
	_PIN("CMDAT0", 100, "GPIO", "CMDAT0", "CMCSD0", "ANT_SEL2", NULL, "TDM_RX_MCK", NULL, "DBG_MON_B[21]", 0, 100),
	_PIN("CMDAT1", 101, "GPIO", "CMDAT1", "CMCSD1", "ANT_SEL3", "CMFLASH", "TDM_RX_BCK", NULL, "DBG_MON_B[22]", 0, 101),
	_PIN("CMMCLK", 102, "GPIO", "CMMCLK", NULL, "ANT_SEL4", NULL, "TDM_RX_LRCK", NULL, "DBG_MON_B[23]", 0, 102),
	_PIN("CMPCLK", 103, "GPIO", "CMPCLK", "CMCSK", "ANT_SEL5", NULL, "TDM_RX_DI", NULL, "DBG_MON_B[24]", 0, 103),
	_PIN("MSDC1_CMD", 104, "GPIO", "MSDC1_CMD", NULL, NULL, "SQICS", NULL, NULL, "DBG_MON_B[25]", 0, 104),
	_PIN("MSDC1_CLK", 105, "GPIO", "MSDC1_CLK", "UDI_NTRST_XI", "DFD_NTRST_XI", "SQISO", "GPUEJ_NTRST_XI", NULL, "DBG_MON_B[26]", 0, 105),
	_PIN("MSDC1_DAT0", 106, "GPIO", "MSDC1_DAT0", "UDI_TMS_XI", "DFD_TMS_XI", "SQISI", "GPUEJ_TMS_XI", NULL, "DBG_MON_B[27]", 0, 106),
	_PIN("MSDC1_DAT1", 107, "GPIO", "MSDC1_DAT1", "UDI_TCK_XI", "DFD_TCK_XI", "SQIWP", "GPUEJ_TCK_XI", NULL, "DBG_MON_B[28]", 0, 107),
	_PIN("MSDC1_DAT2", 108, "GPIO", "MSDC1_DAT2", "UDI_TDI_XI", "DFD_TDI_XI", "SQIRST", "GPUEJ_TDI_XI", NULL, "DBG_MON_B[29]", 0, 108),
	_PIN("MSDC1_DAT3", 109, "GPIO", "MSDC1_DAT3", "UDI_TDO", "DFD_TDO", "SQICK", "GPUEJ_TDO", NULL, "DBG_MON_B[30]", 0, 109),
	_PIN("MSDC0_DAT7", 110, "GPIO", "MSDC0_DAT7", NULL, NULL, "NLD7", NULL, NULL, NULL, 0, 110),
	_PIN("MSDC0_DAT6", 111, "GPIO", "MSDC0_DAT6", NULL, NULL, "NLD6", NULL, NULL, NULL, 0, 111),
	_PIN("MSDC0_DAT5", 112, "GPIO", "MSDC0_DAT5", NULL, NULL, "NLD4", NULL, NULL, NULL, 0, 112),
	_PIN("MSDC0_DAT4", 113, "GPIO", "MSDC0_DAT4", NULL, NULL, "NLD3", NULL, NULL, NULL, 0, 113),
	_PIN("MSDC0_RSTB", 114, "GPIO", "MSDC0_RSTB", NULL, NULL, "NLD0", NULL, NULL, NULL, 0, 114),
	_PIN("MSDC0_CMD", 115, "GPIO", "MSDC0_CMD", NULL, NULL, "NALE", NULL, NULL, NULL, 0, 115),
	_PIN("MSDC0_CLK", 116, "GPIO", "MSDC0_CLK", NULL, NULL, "NWEB", NULL, NULL, NULL, 0, 116),
	_PIN("MSDC0_DAT3", 117, "GPIO", "MSDC0_DAT3", NULL, NULL, "NLD1", NULL, NULL, NULL, 0, 117),
	_PIN("MSDC0_DAT2", 118, "GPIO", "MSDC0_DAT2", NULL, NULL, "NLD5", NULL, NULL, NULL, 0, 118),
	_PIN("MSDC0_DAT1", 119, "GPIO", "MSDC0_DAT1", NULL, NULL, "NLD8", NULL, NULL, NULL, 0, 119),
	_PIN("MSDC0_DAT0", 120, "GPIO", "MSDC0_DAT0", NULL, NULL, "WATCHDOG", "NLD2", NULL, NULL, 0, 120),
	_PIN("CEC", 121, "GPIO", "CEC", NULL, NULL, NULL, NULL, NULL, NULL, 0, 121),
	_PIN("HTPLG", 122, "GPIO", "HTPLG", NULL, NULL, NULL, NULL, NULL, NULL, 0, 122),
	_PIN("HDMISCK", 123, "GPIO", "HDMISCK", NULL, NULL, NULL, NULL, NULL, NULL, 0, 123),
	_PIN("HDMISD", 124, "GPIO", "HDMISD", NULL, NULL, NULL, NULL, NULL, NULL, 0, 124),
};

const struct mercury_gpio_padconf mercury_padconf = {
	.npins = __arraycount(mercury_pins),
	.pins = mercury_pins,
};

#define MTK_PIN_PUPD_SPEC_SR(_pin, _offset, _pupd, _r1, _r0)	\
	{	\
		.pin = _pin,	\
		.offset = _offset,	\
		.pupd_bit = _pupd,	\
		.r1_bit = _r1,		\
		.r0_bit = _r0,		\
	}

const struct mtk_spec_pupd mercury_spec_pupd[] = {
	MTK_PIN_PUPD_SPEC_SR(14, 0xe50, 14, 13, 12),
	MTK_PIN_PUPD_SPEC_SR(15, 0xe60, 2, 1, 0),
	MTK_PIN_PUPD_SPEC_SR(16, 0xe60, 6, 5, 4),
	MTK_PIN_PUPD_SPEC_SR(17, 0xe60, 10, 9, 8),
	MTK_PIN_PUPD_SPEC_SR(21, 0xe60, 14, 13, 12),
	MTK_PIN_PUPD_SPEC_SR(22, 0xe70, 2, 1, 0),
	MTK_PIN_PUPD_SPEC_SR(23, 0xe70, 6, 5, 4),
	MTK_PIN_PUPD_SPEC_SR(40, 0xe80, 2, 1, 0),
	MTK_PIN_PUPD_SPEC_SR(41, 0xe80, 6, 5, 4),
	MTK_PIN_PUPD_SPEC_SR(42, 0xe90, 2, 1, 0),
	MTK_PIN_PUPD_SPEC_SR(43, 0xe90, 6, 5, 4),
	MTK_PIN_PUPD_SPEC_SR(68, 0xe50, 10, 9, 8),
	MTK_PIN_PUPD_SPEC_SR(69, 0xe50, 6, 5, 4),
	MTK_PIN_PUPD_SPEC_SR(70, 0xe40, 6, 5, 4),
	MTK_PIN_PUPD_SPEC_SR(71, 0xe40, 10, 9, 8),
	MTK_PIN_PUPD_SPEC_SR(72, 0xe40, 14, 13, 12),
	MTK_PIN_PUPD_SPEC_SR(73, 0xe50, 2, 1, 0),
	MTK_PIN_PUPD_SPEC_SR(104, 0xe40, 2, 1, 0),
	MTK_PIN_PUPD_SPEC_SR(105, 0xe30, 14, 13, 12),
	MTK_PIN_PUPD_SPEC_SR(106, 0xe20, 14, 13, 12),
	MTK_PIN_PUPD_SPEC_SR(107, 0xe30, 2, 1, 0),
	MTK_PIN_PUPD_SPEC_SR(108, 0xe30, 6, 5, 4),
	MTK_PIN_PUPD_SPEC_SR(109, 0xe30, 10, 9, 8),
	MTK_PIN_PUPD_SPEC_SR(110, 0xe10, 14, 13, 12),
	MTK_PIN_PUPD_SPEC_SR(111, 0xe10, 10, 9, 8),
	MTK_PIN_PUPD_SPEC_SR(112, 0xe10, 6, 5, 4),
	MTK_PIN_PUPD_SPEC_SR(113, 0xe10, 2, 1, 0),
	MTK_PIN_PUPD_SPEC_SR(114, 0xe20, 10, 9, 8),
	MTK_PIN_PUPD_SPEC_SR(115, 0xe20, 2, 1, 0),
	MTK_PIN_PUPD_SPEC_SR(116, 0xe20, 6, 5, 4),
	MTK_PIN_PUPD_SPEC_SR(117, 0xe00, 14, 13, 12),
	MTK_PIN_PUPD_SPEC_SR(118, 0xe00, 10, 9, 8),
	MTK_PIN_PUPD_SPEC_SR(119, 0xe00, 6, 5, 4),
	MTK_PIN_PUPD_SPEC_SR(120, 0xe00, 2, 1, 0),
};

const struct mercury_spec_pupd mercury_padpupd = {
	.size = __arraycount(mercury_spec_pupd),
	.padpupd = mercury_spec_pupd,
};

#define MTK_DRV_GRP(_min, _max, _low, _high, _step)	\
	{	\
		.min_drv = _min,	\
		.max_drv = _max,	\
		.low_bit = _low,	\
		.high_bit = _high,	\
		.step = _step,		\
	}

#define MTK_PIN_DRV_GRP(_pin, _offset, _bit, _grp)	\
	{	\
		.pin = _pin,	\
		.offset = _offset,	\
		.bit = _bit,	\
		.grp = _grp,	\
	}

static const struct mtk_drv_group_desc mercury_drv_grp[] = {
	/* 0E4E8SR 4/8/12/16 */
	MTK_DRV_GRP(4, 16, 1, 2, 4),
	/* 0E2E4SR  2/4/6/8 */
	MTK_DRV_GRP(2, 8, 1, 2, 2),
	/* E8E4E2  2/4/6/8/10/12/14/16 */
	MTK_DRV_GRP(2, 16, 0, 2, 2)
};
static const struct mtk_pin_drv_grp mercury_pin_drv[] = {
	MTK_PIN_DRV_GRP(0, 0xd00, 0, 0),
	MTK_PIN_DRV_GRP(1, 0xd00, 0, 0),
	MTK_PIN_DRV_GRP(2, 0xd00, 0, 0),
	MTK_PIN_DRV_GRP(3, 0xd00, 0, 0),
	MTK_PIN_DRV_GRP(4, 0xd00, 0, 0),
	MTK_PIN_DRV_GRP(5, 0xd00, 4, 0),
	MTK_PIN_DRV_GRP(6, 0xd00, 4, 0),
	MTK_PIN_DRV_GRP(7, 0xd00, 4, 0),
	MTK_PIN_DRV_GRP(8, 0xd00, 4, 0),
	MTK_PIN_DRV_GRP(9, 0xd00, 4, 0),
	MTK_PIN_DRV_GRP(10, 0xd00, 4, 0),
	MTK_PIN_DRV_GRP(11, 0xd00, 8, 0),
	MTK_PIN_DRV_GRP(12, 0xd00, 8, 0),
	MTK_PIN_DRV_GRP(13, 0xd00, 8, 0),
	MTK_PIN_DRV_GRP(14, 0xd00, 12, 2),
	MTK_PIN_DRV_GRP(15, 0xd00, 12, 2),
	MTK_PIN_DRV_GRP(16, 0xd00, 12, 2),
	MTK_PIN_DRV_GRP(17, 0xd00, 12, 2),
	MTK_PIN_DRV_GRP(18, 0xd10, 0, 0),
	MTK_PIN_DRV_GRP(19, 0xd10, 0, 0),
	MTK_PIN_DRV_GRP(20, 0xd10, 0, 0),
	MTK_PIN_DRV_GRP(21, 0xd00, 12, 2),
	MTK_PIN_DRV_GRP(22, 0xd00, 12, 2),
	MTK_PIN_DRV_GRP(23, 0xd00, 12, 2),
	MTK_PIN_DRV_GRP(24, 0xd00, 8, 0),
	MTK_PIN_DRV_GRP(25, 0xd00, 8, 0),
	MTK_PIN_DRV_GRP(26, 0xd10, 4, 1),
	MTK_PIN_DRV_GRP(27, 0xd10, 4, 1),
	MTK_PIN_DRV_GRP(28, 0xd10, 4, 1),
	MTK_PIN_DRV_GRP(29, 0xd10, 4, 1),
	MTK_PIN_DRV_GRP(30, 0xd10, 4, 1),
	MTK_PIN_DRV_GRP(31, 0xd10, 8, 1),
	MTK_PIN_DRV_GRP(32, 0xd10, 8, 1),
	MTK_PIN_DRV_GRP(33, 0xd10, 8, 1),
	MTK_PIN_DRV_GRP(34, 0xd10, 12, 0),
	MTK_PIN_DRV_GRP(35, 0xd10, 12, 0),
	MTK_PIN_DRV_GRP(36, 0xd20, 0, 0),
	MTK_PIN_DRV_GRP(37, 0xd20, 0, 0),
	MTK_PIN_DRV_GRP(38, 0xd20, 0, 0),
	MTK_PIN_DRV_GRP(39, 0xd20, 0, 0),
	MTK_PIN_DRV_GRP(40, 0xd20, 4, 1),
	MTK_PIN_DRV_GRP(41, 0xd20, 8, 1),
	MTK_PIN_DRV_GRP(42, 0xd20, 8, 1),
	MTK_PIN_DRV_GRP(43, 0xd20, 8, 1),
	MTK_PIN_DRV_GRP(44, 0xd20, 12, 1),
	MTK_PIN_DRV_GRP(45, 0xd20, 12, 1),
	MTK_PIN_DRV_GRP(46, 0xd20, 12, 1),
	MTK_PIN_DRV_GRP(47, 0xd20, 12, 1),
	MTK_PIN_DRV_GRP(48, 0xd30, 0, 1),
	MTK_PIN_DRV_GRP(49, 0xd30, 0, 1),
	MTK_PIN_DRV_GRP(50, 0xd30, 0, 1),
	MTK_PIN_DRV_GRP(51, 0xd30, 0, 1),
	MTK_PIN_DRV_GRP(54, 0xd30, 8, 1),
	MTK_PIN_DRV_GRP(55, 0xd30, 12, 1),
	MTK_PIN_DRV_GRP(56, 0xd30, 12, 1),
	MTK_PIN_DRV_GRP(57, 0xd30, 12, 1),
	MTK_PIN_DRV_GRP(62, 0xd40, 8, 1),
	MTK_PIN_DRV_GRP(63, 0xd40, 8, 1),
	MTK_PIN_DRV_GRP(64, 0xd40, 8, 1),
	MTK_PIN_DRV_GRP(65, 0xd40, 8, 1),
	MTK_PIN_DRV_GRP(66, 0xd40, 8, 1),
	MTK_PIN_DRV_GRP(67, 0xd40, 8, 1),
	MTK_PIN_DRV_GRP(68, 0xd40, 12, 2),
	MTK_PIN_DRV_GRP(69, 0xd50, 0, 2),
	MTK_PIN_DRV_GRP(70, 0xd50, 4, 2),
	MTK_PIN_DRV_GRP(71, 0xd50, 4, 2),
	MTK_PIN_DRV_GRP(72, 0xd50, 4, 2),
	MTK_PIN_DRV_GRP(73, 0xd50, 4, 2),
	MTK_PIN_DRV_GRP(100, 0xd50, 8, 1),
	MTK_PIN_DRV_GRP(101, 0xd50, 8, 1),
	MTK_PIN_DRV_GRP(102, 0xd50, 8, 1),
	MTK_PIN_DRV_GRP(103, 0xd50, 8, 1),
	MTK_PIN_DRV_GRP(104, 0xd50, 12, 2),
	MTK_PIN_DRV_GRP(105, 0xd60, 0, 2),
	MTK_PIN_DRV_GRP(106, 0xd60, 4, 2),
	MTK_PIN_DRV_GRP(107, 0xd60, 4, 2),
	MTK_PIN_DRV_GRP(108, 0xd60, 4, 2),
	MTK_PIN_DRV_GRP(109, 0xd60, 4, 2),
	MTK_PIN_DRV_GRP(110, 0xd70, 0, 2),
	MTK_PIN_DRV_GRP(111, 0xd70, 0, 2),
	MTK_PIN_DRV_GRP(112, 0xd70, 0, 2),
	MTK_PIN_DRV_GRP(113, 0xd70, 0, 2),
	MTK_PIN_DRV_GRP(114, 0xd70, 4, 2),
	MTK_PIN_DRV_GRP(115, 0xd60, 12, 2),
	MTK_PIN_DRV_GRP(116, 0xd60, 8, 2),
	MTK_PIN_DRV_GRP(117, 0xd70, 0, 2),
	MTK_PIN_DRV_GRP(118, 0xd70, 0, 2),
	MTK_PIN_DRV_GRP(119, 0xd70, 0, 2),
	MTK_PIN_DRV_GRP(120, 0xd70, 0, 2),
};

struct mercury_gpio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	const struct mercury_gpio_padconf *sc_padconf;
	const struct mercury_spec_pupd *sc_padpupd;
	kmutex_t sc_lock;

	struct gpio_chipset_tag sc_gp;
	gpio_pin_t *sc_pins;
	device_t sc_gpiodev;
};

struct mercury_gpio_pin {
	struct mercury_gpio_softc *pin_sc;
	const struct mercury_gpio_pins *pin_def;
	int pin_flags;
	bool pin_actlo;
};

#define GPIO_READ(sc, reg) 		\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define GPIO_WRITE(sc, reg, val) 	\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static struct mercury_gpio_softc * gpio_sc;

static int	mercury_gpio_match(device_t, cfdata_t, void *);
static void	mercury_gpio_attach(device_t, device_t, void *);

static const char * const compatible[] = {
	"mediatek,mercury-pinctrl",
	NULL
};

CFATTACH_DECL_NEW(mtk_gpio, sizeof(struct mercury_gpio_softc),
	mercury_gpio_match, mercury_gpio_attach, NULL, NULL);


static int
mercury_gpio_setfunc(struct mercury_gpio_softc *sc,
    const struct mercury_gpio_pins *pin_def, const char *func)
{
	uint32_t cfg;
	u_int n;

	KASSERT(mutex_owned(&sc->sc_lock));

	const bus_size_t cfg_reg = MERCURY_GPIO_PINMUX(pin_def->pin);
	uint32_t cfg_mask =  7U << ((pin_def->pin % 5) * 3);
	for (n = 0; n < MERCURY_GPIO_MAXFUNC; n++) {
		if (pin_def->functions[n] == NULL)
			continue;
		if (strcmp(pin_def->functions[n], func) == 0) {
			cfg = GPIO_READ(sc, cfg_reg);
			cfg &= ~cfg_mask;
			cfg |= __SHIFTIN(n, cfg_mask);
#ifdef MERCURY_GPIO_DEBUG
			device_printf(sc->sc_dev, "GPIO%03d cfg %08x -> %08x\n",
			    pin_def->pin, GPIO_READ(sc, cfg_reg), cfg);
#endif
			GPIO_WRITE(sc, cfg_reg, cfg);
			return 0;
		}
	}

	/* Function not found */
	device_printf(sc->sc_dev, "function '%s' not supported on GPIO%03d\n",
	    func, pin_def->pin);

	return ENXIO;
}

static int
mercury_gpio_setdir(struct mercury_gpio_softc *sc,
    const struct mercury_gpio_pins *pin_def, int flags)
{
	KASSERT(mutex_owned(&sc->sc_lock));

	bus_size_t dir_reg = MERCURY_GPIO_DIR(pin_def->pin);
	const uint32_t dir_mask = __BIT(pin_def->pin % 16);

	if (flags & GPIO_PIN_INPUT)
			dir_reg += 0x8;
	if (flags & GPIO_PIN_OUTPUT)
			dir_reg += 0x4;

#ifdef MERCURY_GPIO_DEBUG
	device_printf(sc->sc_dev, "GPIO%02d dir_reg:%x dir_mask:%x\n",
	    pin_def->pin, dir_reg, dir_mask);
#endif
	GPIO_WRITE(sc, dir_reg, dir_mask);

	return 0;
}

static int
mercury_set_spec_pupd(struct mercury_gpio_softc *sc,
    const struct mercury_gpio_pins *pin_def, int flags, u_int r1r0)
{
	const struct mtk_spec_pupd *spec_pupd_pin;
	bus_size_t reg_pupd, reg_set, reg_rst;
	uint32_t bit_pupd, bit_r0, bit_r1;
	bool spec_flag = false;
	u_int n;

	for (n = 0; n < sc->sc_padpupd->size; n++) {
			if (pin_def->pin == sc->sc_padpupd->padpupd[n].pin) {
				spec_flag = true;
				break;
			}
	}

	if (!spec_flag)
		return EINVAL;

	spec_pupd_pin = sc->sc_padpupd->padpupd + n;
	reg_set = spec_pupd_pin->offset + 0x4;
	reg_rst = spec_pupd_pin->offset + 0x8;

	if (flags & GPIO_PIN_PULLUP)
		reg_pupd = reg_rst;
	else if (flags & GPIO_PIN_PULLDOWN)
		reg_pupd = reg_set;
	else {
		reg_pupd = reg_set;
		r1r0 = MTK_PUPD_SET_R1R0_00;
	}

	bit_pupd = __BIT(spec_pupd_pin->pupd_bit);

	GPIO_WRITE(sc, reg_pupd, bit_pupd);

	bit_r0 = __BIT(spec_pupd_pin->r0_bit);
	bit_r1 = __BIT(spec_pupd_pin->r1_bit);

	switch (r1r0) {
	case MTK_PUPD_SET_R1R0_00:
		GPIO_WRITE(sc, reg_rst, bit_r0);
		GPIO_WRITE(sc, reg_rst, bit_r1);
		break;
	case MTK_PUPD_SET_R1R0_01:
		GPIO_WRITE(sc, reg_set, bit_r0);
		GPIO_WRITE(sc, reg_rst, bit_r1);
		break;
	case MTK_PUPD_SET_R1R0_10:
		GPIO_WRITE(sc, reg_rst, bit_r0);
		GPIO_WRITE(sc, reg_set, bit_r1);
		break;
	case MTK_PUPD_SET_R1R0_11:
		GPIO_WRITE(sc, reg_set, bit_r0);
		GPIO_WRITE(sc, reg_set, bit_r1);
		break;
	default:
		break;
	}

	return 0;
}


static int
mercury_gpio_setpull(struct mercury_gpio_softc *sc,
    const struct mercury_gpio_pins *pin_def, int flags)
{
	int error;

	KASSERT(mutex_owned(&sc->sc_lock));

	bus_size_t reg_pullen = MERCURY_GPIO_PUEN(pin_def->pin);
	bus_size_t reg_pullsel = MERCURY_GPIO_PUSEL(pin_def->pin);
	const uint32_t pull_mask = __BIT(pin_def->pin % 16);

	error = mercury_set_spec_pupd(sc, pin_def, flags, 0);
	if (error == 0)
		return 0;

	if (flags & GPIO_PIN_PULLUP) {
		reg_pullen += 0x4;
		reg_pullsel += 0x4;
	} else if (flags & GPIO_PIN_PULLDOWN) {
		reg_pullen += 0x4;
		reg_pullsel += 0x8;
	} else {
		reg_pullen += 0x8;
		reg_pullsel += 0x8;
	}
	GPIO_WRITE(sc, reg_pullen, pull_mask);
	GPIO_WRITE(sc, reg_pullsel, pull_mask);

	return 0;
}

static int
mercury_gpio_ctl(struct mercury_gpio_softc *sc, const struct mercury_gpio_pins *pin_def,
    int flags)
{
	KASSERT(mutex_owned(&sc->sc_lock));

	mercury_gpio_setfunc(sc, pin_def, "GPIO");
	if ((flags & GPIO_PIN_INPUT)||(flags & GPIO_PIN_OUTPUT))
		return mercury_gpio_setdir(sc, pin_def, flags);

	return EINVAL;
}

static int
mercury_gpio_pin_read(void *priv, int pin)
{
	struct mercury_gpio_softc * const sc = priv;
	const struct mercury_gpio_pins *pin_def = &sc->sc_padconf->pins[pin];
	uint32_t data;
	int val;

	KASSERT(pin < sc->sc_padconf->npins);

	const bus_size_t data_reg = MERCURY_GPIO_DATA(pin_def->pin);
	const uint32_t data_mask = __BIT(pin_def->pin % 16);

	/* No lock required for reads */
	data = GPIO_READ(sc, data_reg);
	val = __SHIFTOUT(data, data_mask);

	return val;
}

static void
mercury_gpio_pin_write(void *priv, int pin, int val)
{
	struct mercury_gpio_softc * const sc = priv;
	const struct mercury_gpio_pins *pin_def = &sc->sc_padconf->pins[pin];

	KASSERT(pin < sc->sc_padconf->npins);

	bus_size_t data_reg = MERCURY_GPIO_DOUT(pin_def->pin);
	const uint32_t data_mask = __BIT(pin_def->pin % 16);

	mutex_enter(&sc->sc_lock);
	if (val)
		data_reg += 0x4;
	else
		data_reg += 0x8;
	GPIO_WRITE(sc, data_reg, data_mask);
	mutex_exit(&sc->sc_lock);
}

static void
mercury_gpio_pin_ctl(void *priv, int pin, int flags)
{
	struct mercury_gpio_softc * const sc = priv;
	const struct mercury_gpio_pins *pin_def = &sc->sc_padconf->pins[pin];

	KASSERT(pin < sc->sc_padconf->npins);

	mutex_enter(&sc->sc_lock);
	mercury_gpio_ctl(sc, pin_def, flags);
	mercury_gpio_setpull(sc, pin_def, flags);
	mutex_exit(&sc->sc_lock);
}

static void
mercury_gpio_attach_ports(struct mercury_gpio_softc *sc)
{
	const struct mercury_gpio_pins *pin_def;
	struct gpio_chipset_tag *gp = &sc->sc_gp;
	struct gpiobus_attach_args gba;
	u_int pin;

	gp->gp_cookie = sc;
	gp->gp_pin_read = mercury_gpio_pin_read;
	gp->gp_pin_write = mercury_gpio_pin_write;
	gp->gp_pin_ctl = mercury_gpio_pin_ctl;

	const u_int npins = sc->sc_padconf->npins;
	sc->sc_pins = kmem_zalloc(sizeof(*sc->sc_pins) * npins, KM_SLEEP);

	for (pin = 0; pin < sc->sc_padconf->npins; pin++) {
		pin_def = &sc->sc_padconf->pins[pin];
		sc->sc_pins[pin].pin_num = pin;
		sc->sc_pins[pin].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
		    GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN;
		sc->sc_pins[pin].pin_state = mercury_gpio_pin_read(sc, pin);
		strlcpy(sc->sc_pins[pin].pin_defname, pin_def->ballname,
		    sizeof(sc->sc_pins[pin].pin_defname));
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = gp;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = npins;
	sc->sc_gpiodev = config_found_ia(sc->sc_dev, "gpiobus", &gba, NULL);
}

static int
mercury_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
mercury_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct mercury_gpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;

	gpio_sc = sc;
	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	sc->sc_padconf = &mercury_padconf;
	sc->sc_padpupd = &mercury_padpupd;

	mercury_gpio_attach_ports(sc);
}

int
mt_set_gpio_mode(int pin, int val)
{
	uint32_t cfg;

	KASSERT(val < MERCURY_GPIO_MAXFUNC);
	KASSERT(pin < gpio_sc->sc_padconf->npins);

	const bus_size_t cfg_reg = MERCURY_GPIO_PINMUX(pin);
	uint32_t cfg_mask =  7U << ((pin % 5) * 3);

	cfg = GPIO_READ(gpio_sc, cfg_reg);
	cfg &= ~cfg_mask;
	cfg |= __SHIFTIN(val, cfg_mask);

	GPIO_WRITE(gpio_sc, cfg_reg, cfg);

	return 0;
}

int
mt_set_gpio_dir(int pin, int val)
{
	KASSERT(pin < gpio_sc->sc_padconf->npins);

	bus_size_t dir_reg = MERCURY_GPIO_DIR(pin);
	const uint32_t dir_mask = __BIT(pin % 16);

	if (val)
			dir_reg += 0x4;
	else
			dir_reg += 0x8;

	GPIO_WRITE(gpio_sc, dir_reg, dir_mask);
	return 0;
}

int
mt_get_gpio_din(int pin)
{
	uint32_t data;
	uint val;

	KASSERT(pin < gpio_sc->sc_padconf->npins);

	const bus_size_t data_reg = MERCURY_GPIO_DATA(pin);
	const uint32_t data_mask = __BIT(pin % 16);

	/* No lock required for reads */
	data = GPIO_READ(gpio_sc, data_reg);
	val = __SHIFTOUT(data, data_mask);

	return val;
}

int
mt_set_gpio_dout(int pin, int val)
{
	KASSERT(pin < gpio_sc->sc_padconf->npins);

	bus_size_t data_reg = MERCURY_GPIO_DOUT(pin);
	const uint32_t data_mask = __BIT(pin % 16);

	mutex_enter(&gpio_sc->sc_lock);
	if (val)
		data_reg += 0x4;
	else
		data_reg += 0x8;

	GPIO_WRITE(gpio_sc, data_reg, data_mask);
	mutex_exit(&gpio_sc->sc_lock);

	return 0;
}

int
mtk_set_spec_pupd(int pin, int flags, u_int r1r0)
{
	const struct mtk_spec_pupd *spec_pupd_pin;
	bus_size_t reg_pupd, reg_set, reg_rst;
	uint32_t bit_pupd, bit_r0, bit_r1;
	bool spec_flag = false;
	u_int n;

	KASSERT(pin < gpio_sc->sc_padconf->npins);

	for (n = 0; n < gpio_sc->sc_padpupd->size; n++) {
			if (pin == gpio_sc->sc_padpupd->padpupd[n].pin) {
				spec_flag = true;
				break;
			}
	}

	if (!spec_flag)
		return EINVAL;

	spec_pupd_pin = gpio_sc->sc_padpupd->padpupd + n;
	reg_set = spec_pupd_pin->offset + 0x4;
	reg_rst = spec_pupd_pin->offset + 0x8;

	if (flags == MTK_GPIO_PULLUP)
		reg_pupd = reg_rst;
	else if (flags == MTK_GPIO_PULLDOWN)
		reg_pupd = reg_set;
	else {
		reg_pupd = reg_set;
		r1r0 = MTK_PUPD_SET_R1R0_00;
	}

	bit_pupd = __BIT(spec_pupd_pin->pupd_bit);

	GPIO_WRITE(gpio_sc, reg_pupd, bit_pupd);

	bit_r0 = __BIT(spec_pupd_pin->r0_bit);
	bit_r1 = __BIT(spec_pupd_pin->r1_bit);

	switch (r1r0) {
	case MTK_PUPD_SET_R1R0_00:
		GPIO_WRITE(gpio_sc, reg_rst, bit_r0);
		GPIO_WRITE(gpio_sc, reg_rst, bit_r1);
		break;
	case MTK_PUPD_SET_R1R0_01:
		GPIO_WRITE(gpio_sc, reg_set, bit_r0);
		GPIO_WRITE(gpio_sc, reg_rst, bit_r1);
		break;
	case MTK_PUPD_SET_R1R0_10:
		GPIO_WRITE(gpio_sc, reg_rst, bit_r0);
		GPIO_WRITE(gpio_sc, reg_set, bit_r1);
		break;
	case MTK_PUPD_SET_R1R0_11:
		GPIO_WRITE(gpio_sc, reg_set, bit_r0);
		GPIO_WRITE(gpio_sc, reg_set, bit_r1);
		break;
	default:
		break;
	}

	return 0;
}

int
mt_set_gpio_pull(int pin, int flags)
{
	int error;

	KASSERT(pin < gpio_sc->sc_padconf->npins);

	bus_size_t reg_pullen = MERCURY_GPIO_PUEN(pin);
	bus_size_t reg_pullsel = MERCURY_GPIO_PUSEL(pin);
	const uint32_t pull_mask = __BIT(pin % 16);

	error = mtk_set_spec_pupd(pin, flags, 0);
	if (error == 0)
		return 0;

	switch (flags) {
	case MTK_GPIO_PULLDISABLE:
		reg_pullen += 0x8;
		reg_pullsel += 0x8;
		break;
	case MTK_GPIO_PULLUP:
		reg_pullen += 0x4;
		reg_pullsel += 0x4;
		break;
	case MTK_GPIO_PULLDOWN:
		reg_pullen += 0x4;
		reg_pullsel += 0x8;
		break;
	default:
		break;
	}

	GPIO_WRITE(gpio_sc, reg_pullen, pull_mask);
	GPIO_WRITE(gpio_sc, reg_pullsel, pull_mask);

	return 0;
}

static const struct mtk_pin_drv_grp *
mtk_find_pin_drv_grp_by_pin(int pin) {
	int i;
	for (i = 0; i < __arraycount(mercury_pin_drv); i++) {
		const struct mtk_pin_drv_grp *pin_drv =
				mercury_pin_drv + i;
		if (pin == pin_drv->pin)
			return pin_drv;
	}
	return NULL;
}

int
mt_set_gpio_driving(int pin, unsigned char driving)
{
	const struct mtk_pin_drv_grp *pin_drv;
	unsigned int val;
	unsigned int bits, mask, shift, regval;
	const struct mtk_drv_group_desc *drv_grp;
	bus_size_t reg_drv;

	KASSERT(pin < gpio_sc->sc_padconf->npins);

	pin_drv = mtk_find_pin_drv_grp_by_pin(pin);
	if (!pin_drv || pin_drv->grp > __arraycount(mercury_drv_grp))
		return -EINVAL;

	drv_grp = mercury_drv_grp + pin_drv->grp;
	reg_drv = pin_drv->offset;
	if (driving >= drv_grp->min_drv && driving <= drv_grp->max_drv
		&& !(driving % drv_grp->step)) {
		regval = GPIO_READ(gpio_sc, reg_drv);

		val = driving / drv_grp->step - 1;
		bits = drv_grp->high_bit - drv_grp->low_bit + 1;
		mask = __BIT(bits) - 1;
		shift = pin_drv->bit + drv_grp->low_bit;
		mask <<= shift;
		regval &= ~mask;
		regval |= __SHIFTIN(val, mask);

		GPIO_WRITE(gpio_sc, reg_drv, regval);
		return 0;
	}
	aprint_normal("driving:%dmA is not support for GPIO%d\n", driving, pin);
	return EINVAL;
}
