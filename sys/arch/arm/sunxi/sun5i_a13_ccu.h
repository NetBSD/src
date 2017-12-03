/* $NetBSD: sun5i_a13_ccu.h,v 1.1.4.2 2017/12/03 11:35:56 jdolecek Exp $ */

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

#ifndef _SUN5I_A13_CCU_H
#define _SUN5I_A13_CCU_H

#define	A13_RST_USB_PHY0	0
#define	A13_RST_USB_PHY1	1
#define	A13_RST_GPS		2
#define	A13_RST_DE_BE		3
#define	A13_RST_DE_FE		4
#define	A13_RST_TVE		5
#define	A13_RST_LCD		6
#define	A13_RST_CSI		7
#define	A13_RST_VE		8
#define	A13_RST_GPU		9
#define	A13_RST_IEP		10

#define	A13_CLK_HOSC		1
#define	A13_CLK_PLL_CORE	2
#define	A13_CLK_PLL_AUDIO_BASE	3
#define	A13_CLK_PLL_AUDIO	4
#define	A13_CLK_PLL_AUDIO_2X	5
#define	A13_CLK_PLL_AUDIO_4X	6
#define	A13_CLK_PLL_AUDIO_8X	7
#define	A13_CLK_PLL_VIDEO0	8
#define	A13_CLK_PLL_VIDEO0_2X	9
#define	A13_CLK_PLL_VE		10
#define	A13_CLK_PLL_DDR_BASE	11
#define	A13_CLK_PLL_DDR		12
#define	A13_CLK_PLL_DDR_OTHER	13
#define	A13_CLK_PERIPH		14
#define	A13_CLK_VIDEO1		15
#define	A13_CLK_VIDEO1_2X	16
#define	A13_CLK_CPU		17
#define	A13_CLK_AXI		18
#define	A13_CLK_AHB		19
#define	A13_CLK_APB0		20
#define	A13_CLK_APB1		21
#define	A13_CLK_DRAM_AXI	22
#define	A13_CLK_AHB_OTG		23
#define	A13_CLK_AHB_EHCI	24
#define	A13_CLK_AHB_OHCI	25
#define	A13_CLK_AHB_SS		26
#define	A13_CLK_AHB_DMA		27
#define	A13_CLK_AHB_BIST	28
#define	A13_CLK_AHB_MMC0	29
#define	A13_CLK_AHB_MMC1	30
#define	A13_CLK_AHB_MMC2	31
#define	A13_CLK_AHB_NAND	32
#define	A13_CLK_AHB_SDRAM	33
#define	A13_CLK_AHB_EMAC	34
#define	A13_CLK_AHB_TS		35
#define	A13_CLK_AHB_SPI0	36
#define	A13_CLK_AHB_SPI1	37
#define	A13_CLK_AHB_SPI2	38
#define	A13_CLK_AHB_GPS		39
#define	A13_CLK_AHB_HSTIMER	40
#define	A13_CLK_AHB_VE		41
#define	A13_CLK_AHB_TVE		42
#define	A13_CLK_AHB_LCD		43
#define	A13_CLK_AHB_CSI		44
#define	A13_CLK_AHB_HDMI	45
#define	A13_CLK_AHB_DE_BE	46
#define	A13_CLK_AHB_DE_FE	47
#define	A13_CLK_AHB_IEP		48
#define	A13_CLK_AHB_GPU		49
#define	A13_CLK_APB0_CODEC	50
#define	A13_CLK_APB0_SPDIF	51
#define	A13_CLK_APB0_I2S	52
#define	A13_CLK_APB0_PIO	53
#define	A13_CLK_APB0_IR		54
#define	A13_CLK_APB0_KEYPAD	55
#define	A13_CLK_APB1_I2C0	56
#define	A13_CLK_APB1_I2C1	57
#define	A13_CLK_APB1_I2C2	58
#define	A13_CLK_APB1_UART0	59
#define	A13_CLK_APB1_UART1	60
#define	A13_CLK_APB1_UART2	61
#define	A13_CLK_APB1_UART3	62
#define	A13_CLK_NAND		63
#define	A13_CLK_MMC0		64
#define	A13_CLK_MMC1		65
#define	A13_CLK_MMC2		66
#define	A13_CLK_TS		67
#define	A13_CLK_SS		68
#define	A13_CLK_SPI0		69
#define	A13_CLK_SPI1		70
#define	A13_CLK_SPI2		71
#define	A13_CLK_IR		72
#define	A13_CLK_I2S		73
#define	A13_CLK_SPDIF		74
#define	A13_CLK_KEYPAD		75
#define	A13_CLK_USB_OHCI	76
#define	A13_CLK_USB_PHY0	77
#define	A13_CLK_USB_PHY1	78
#define	A13_CLK_GPS		79
#define	A13_CLK_DRAM_VE		80
#define	A13_CLK_DRAM_CSI	81
#define	A13_CLK_DRAM_TS		82
#define	A13_CLK_DRAM_TVE	83
#define	A13_CLK_DRAM_DE_FE	84
#define	A13_CLK_DRAM_DE_BE	85
#define	A13_CLK_DRAM_ACE	86
#define	A13_CLK_DRAM_IEP	87
#define	A13_CLK_DE_BE		88
#define	A13_CLK_DE_FE		89
#define	A13_CLK_TCON_CH0	90
#define	A13_CLK_TCON_CH1_SCLK	91
#define	A13_CLK_TCON_CH1	92
#define	A13_CLK_CSI		93
#define	A13_CLK_VE		94
#define	A13_CLK_CODEC		95
#define	A13_CLK_AVS		96
#define	A13_CLK_HDMI		97
#define	A13_CLK_GPU		98
#define	A13_CLK_MBUS		99
#define	A13_CLK_IEP		100

#endif /* !_SUN5I_A13_CCU_H */
