/* $NetBSD: sun8i_v3s_ccu.h,v 1.1 2021/05/05 10:24:04 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 Rui-Xiang Guo
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

#ifndef __CCU_V3S_H__
#define __CCU_V3S_H__

#define	V3S_CLK_PLL_CPU		0
#define	V3S_CLK_PLL_AUDIO_BASE	1
#define	V3S_CLK_PLL_AUDIO	2
#define	V3S_CLK_PLL_AUDIO_2X	3
#define	V3S_CLK_PLL_AUDIO_4X	4
#define	V3S_CLK_PLL_AUDIO_8X	5
#define	V3S_CLK_PLL_VIDEO	6
#define	V3S_CLK_PLL_VE		7
#define	V3S_CLK_PLL_DDR		8
#define	V3S_CLK_PLL_PERIPH0	9
#define	V3S_CLK_PLL_PERIPH0_2X	10
#define	V3S_CLK_PLL_ISP		11
#define	V3S_CLK_PLL_PERIPH1	12
#define	V3S_CLK_CPU		14
#define	V3S_CLK_AXI		15
#define	V3S_CLK_AHB1		16
#define	V3S_CLK_APB1		17
#define	V3S_CLK_APB2		18
#define	V3S_CLK_AHB2		19
#define	V3S_CLK_BUS_CE		20
#define	V3S_CLK_BUS_DMA		21
#define	V3S_CLK_BUS_MMC0	22
#define	V3S_CLK_BUS_MMC1	23
#define	V3S_CLK_BUS_MMC2	24
#define	V3S_CLK_BUS_DRAM	25
#define	V3S_CLK_BUS_EMAC	26
#define	V3S_CLK_BUS_HSTIMER	27
#define	V3S_CLK_BUS_SPI		28
#define	V3S_CLK_BUS_OTG		29
#define	V3S_CLK_BUS_EHCI	30
#define	V3S_CLK_BUS_OHCI	31
#define	V3S_CLK_BUS_VE		32
#define	V3S_CLK_BUS_TCON	33
#define	V3S_CLK_BUS_CSI		34
#define	V3S_CLK_BUS_DE		35
#define	V3S_CLK_BUS_CODEC	36
#define	V3S_CLK_BUS_PIO		37
#define	V3S_CLK_BUS_I2C0	38
#define	V3S_CLK_BUS_I2C1	39
#define	V3S_CLK_BUS_UART0	40
#define	V3S_CLK_BUS_UART1	41
#define	V3S_CLK_BUS_UART2	42
#define	V3S_CLK_BUS_EPHY	43
#define	V3S_CLK_BUS_DBG		44
#define	V3S_CLK_MMC0		45
#define	V3S_CLK_MMC0_SAMPLE	46
#define	V3S_CLK_MMC0_OUTPUT	47
#define	V3S_CLK_MMC1		48
#define	V3S_CLK_MMC1_SAMPLE	49
#define	V3S_CLK_MMC1_OUTPUT	50
#define	V3S_CLK_MMC2		51
#define	V3S_CLK_MMC2_SAMPLE	52
#define	V3S_CLK_MMC2_OUTPUT	53
#define	V3S_CLK_CE		54
#define	V3S_CLK_SPI		55
#define	V3S_CLK_USBPHY		56
#define	V3S_CLK_USBOHCI		57
#define	V3S_CLK_DRAM		58
#define	V3S_CLK_DRAM_VE		59
#define	V3S_CLK_DRAM_CSI	60
#define	V3S_CLK_DRAM_EHCI	61
#define	V3S_CLK_DRAM_OHCI	62
#define	V3S_CLK_DE		63
#define	V3S_CLK_TCON		64
#define	V3S_CLK_CSI_MISC	65
#define	V3S_CLK_CSI0_MCLK	66
#define	V3S_CLK_CSI1_SCLK	67
#define	V3S_CLK_CSI1_MCLK	68
#define	V3S_CLK_VE		69
#define	V3S_CLK_AC_DIG		70
#define	V3S_CLK_AVS		71
#define	V3S_CLK_MBUS		72
#define	V3S_CLK_MIPI_CSI	73

#define	V3S_RST_USBPHY		0
#define	V3S_RST_MBUS		1
#define	V3S_RST_BUS_CE		5
#define	V3S_RST_BUS_DMA		6
#define	V3S_RST_BUS_MMC0	7
#define	V3S_RST_BUS_MMC1	8
#define	V3S_RST_BUS_MMC2	9
#define	V3S_RST_BUS_DRAM	11
#define	V3S_RST_BUS_EMAC	12
#define	V3S_RST_BUS_HSTIMER	14
#define	V3S_RST_BUS_SPI		15
#define	V3S_RST_BUS_OTG		17
#define	V3S_RST_BUS_EHCI	18
#define	V3S_RST_BUS_OHCI	22
#define	V3S_RST_BUS_VE		26
#define	V3S_RST_BUS_TCON	27
#define	V3S_RST_BUS_CSI		30
#define	V3S_RST_BUS_DE		34
#define	V3S_RST_BUS_DBG		38
#define	V3S_RST_BUS_EPHY	39
#define	V3S_RST_BUS_CODEC	40
#define	V3S_RST_BUS_I2C0	46
#define	V3S_RST_BUS_I2C1	47
#define	V3S_RST_BUS_UART0	49
#define	V3S_RST_BUS_UART1	50
#define	V3S_RST_BUS_UART2	51

#endif /* __CCU_V3S_H__ */
