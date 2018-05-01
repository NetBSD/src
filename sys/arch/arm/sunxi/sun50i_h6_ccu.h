/* $NetBSD: sun50i_h6_ccu.h,v 1.1 2018/05/01 19:53:14 jmcneill Exp $ */

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

#ifndef _SUN50I_H6_CCU_H
#define _SUN50I_H6_CCU_H

#define	H6_RST_MBUS		0
#define	H6_RST_BUS_DE		1
#define	H6_RST_BUS_DEINTERLACE	2
#define	H6_RST_BUS_GPU		3
#define	H6_RST_BUS_CE		4
#define	H6_RST_BUS_VE		5
#define	H6_RST_BUS_EMCE		6
#define	H6_RST_BUS_VP9		7
#define	H6_RST_BUS_DMA		8
#define	H6_RST_BUS_MSGBOX	9
#define	H6_RST_BUS_SPINLOCK	10
#define	H6_RST_BUS_HSTIMER	11
#define	H6_RST_BUS_DBG		12
#define	H6_RST_BUS_PSI		13
#define	H6_RST_BUS_PWM		14
#define	H6_RST_BUS_IOMMU	15
#define	H6_RST_BUS_DRAM		16
#define	H6_RST_BUS_NAND		17
#define	H6_RST_BUS_MMC0		18
#define	H6_RST_BUS_MMC1		19
#define	H6_RST_BUS_MMC2		20
#define	H6_RST_BUS_UART0	21
#define	H6_RST_BUS_UART1	22
#define	H6_RST_BUS_UART2	23
#define	H6_RST_BUS_UART3	24
#define	H6_RST_BUS_I2C0		25
#define	H6_RST_BUS_I2C1		26
#define	H6_RST_BUS_I2C2		27
#define	H6_RST_BUS_I2C3		28
#define	H6_RST_BUS_SCR0		29
#define	H6_RST_BUS_SCR1		30
#define	H6_RST_BUS_SPI0		31
#define	H6_RST_BUS_SPI1		32
#define	H6_RST_BUS_EMAC		33
#define	H6_RST_BUS_TS		34
#define	H6_RST_BUS_IR_TX	35
#define	H6_RST_BUS_THS		36
#define	H6_RST_BUS_I2S0		37
#define	H6_RST_BUS_I2S1		38
#define	H6_RST_BUS_I2S2		39
#define	H6_RST_BUS_I2S3		40
#define	H6_RST_BUS_SPDIF	41
#define	H6_RST_BUS_DMIC		42
#define	H6_RST_BUS_AUDIO_HUB	43
#define	H6_RST_USB_PHY0		44
#define	H6_RST_USB_PHY1		45
#define	H6_RST_USB_PHY3		46
#define	H6_RST_USB_HSIC		47
#define	H6_RST_BUS_OHCI0	48
#define	H6_RST_BUS_OHCI3	49
#define	H6_RST_BUS_EHCI0	50
#define	H6_RST_BUS_XHCI		51
#define	H6_RST_BUS_EHCI3	52
#define	H6_RST_BUS_OTG		53
#define	H6_RST_BUS_PCIE		54
#define	H6_RST_PCIE_POWERUP	55
#define	H6_RST_BUS_HDMI		56
#define	H6_RST_BUS_HDMI_SUB	57
#define	H6_RST_BUS_TCON_TOP	58
#define	H6_RST_BUS_TCON_LCD0	59
#define	H6_RST_BUS_TCON_TV0	60
#define	H6_RST_BUS_CSI		61
#define	H6_RST_BUS_HDCP		62

#define	H6_CLK_OSC12M		0
#define	H6_CLK_PLL_CPUX		1
#define	H6_CLK_PLL_DDR0		2
#define	H6_CLK_PLL_PERIPH0	3
#define	H6_CLK_PLL_PERIPH0_2X	4
#define	H6_CLK_PLL_PERIPH0_4X	5
#define	H6_CLK_PLL_PERIPH1	6
#define	H6_CLK_PLL_PERIPH1_2X	7
#define	H6_CLK_PLL_PERIPH1_4X	8
#define	H6_CLK_PLL_GPU		9
#define	H6_CLK_PLL_VIDEO0	10
#define	H6_CLK_PLL_VIDEO0_4X	11
#define	H6_CLK_PLL_VIDEO1	12
#define	H6_CLK_PLL_VIDEO1_4X	13
#define	H6_CLK_PLL_VE		14
#define	H6_CLK_PLL_DE		15
#define	H6_CLK_PLL_HSIC		16
#define	H6_CLK_PLL_AUDIO_BASE	17
#define	H6_CLK_PLL_AUDIO	18
#define	H6_CLK_PLL_AUDIO_2X	19
#define	H6_CLK_PLL_AUDIO_4X	20
#define	H6_CLK_CPUX		21
#define	H6_CLK_AXI		22
#define	H6_CLK_CPUX_APB		23
#define	H6_CLK_PSI_AHB1_AHB2	24
#define	H6_CLK_AHB3		25
#define	H6_CLK_APB1		26
#define	H6_CLK_APB2		27
#define	H6_CLK_MBUS		28
#define	H6_CLK_DE		29
#define	H6_CLK_BUS_DE		30
#define	H6_CLK_DEINTERLACE	31
#define	H6_CLK_BUS_DEINTERLACE	32
#define	H6_CLK_GPU		33
#define	H6_CLK_BUS_GPU		34
#define	H6_CLK_CE		35
#define	H6_CLK_BUS_CE		36
#define	H6_CLK_VE		37
#define	H6_CLK_BUS_VE		38
#define	H6_CLK_EMCE		39
#define	H6_CLK_BUS_EMCE		40
#define	H6_CLK_VP9		41
#define	H6_CLK_BUS_VP9		42
#define	H6_CLK_BUS_DMA		43
#define	H6_CLK_BUS_MSGBOX	44
#define	H6_CLK_BUS_SPINLOCK	45
#define	H6_CLK_BUS_HSTIMER	46
#define	H6_CLK_AVS		47
#define	H6_CLK_BUS_DBG		48
#define	H6_CLK_BUS_PSI		49
#define	H6_CLK_BUS_PWM		50
#define	H6_CLK_BUS_IOMMU	51
#define	H6_CLK_DRAM		52
#define	H6_CLK_MBUS_DMA		53
#define	H6_CLK_MBUS_VE		54
#define	H6_CLK_MBUS_CE		55
#define	H6_CLK_MBUS_TS		56
#define	H6_CLK_MBUS_NAND	57
#define	H6_CLK_MBUS_CSI		58
#define	H6_CLK_MBUS_DEINTERLACE	59
#define	H6_CLK_BUS_DRAM		60
#define	H6_CLK_NAND0		61
#define	H6_CLK_NAND1		62
#define	H6_CLK_BUS_NAND		63
#define	H6_CLK_MMC0		64
#define	H6_CLK_MMC1		65
#define	H6_CLK_MMC2		66
#define	H6_CLK_BUS_MMC0		67
#define	H6_CLK_BUS_MMC1		68
#define	H6_CLK_BUS_MMC2		69
#define	H6_CLK_BUS_UART0	70
#define	H6_CLK_BUS_UART1	71
#define	H6_CLK_BUS_UART2	72
#define	H6_CLK_BUS_UART3	73
#define	H6_CLK_BUS_I2C0		74
#define	H6_CLK_BUS_I2C1		75
#define	H6_CLK_BUS_I2C2		76
#define	H6_CLK_BUS_I2C3		77
#define	H6_CLK_BUS_SCR0		78
#define	H6_CLK_BUS_SCR1		79
#define	H6_CLK_SPI0		80
#define	H6_CLK_SPI1		81
#define	H6_CLK_BUS_SPI0		82
#define	H6_CLK_BUS_SPI1		83
#define	H6_CLK_BUS_EMAC		84
#define	H6_CLK_TS		85
#define	H6_CLK_BUS_TS		86
#define	H6_CLK_IR_TX		87
#define	H6_CLK_BUS_IR_TX	88
#define	H6_CLK_BUS_THS		89
#define	H6_CLK_I2S3		90
#define	H6_CLK_I2S0		91
#define	H6_CLK_I2S1		92
#define	H6_CLK_I2S2		93
#define	H6_CLK_BUS_I2S0		94
#define	H6_CLK_BUS_I2S1		95
#define	H6_CLK_BUS_I2S2		96
#define	H6_CLK_BUS_I2S3		97
#define	H6_CLK_SPDIF		98
#define	H6_CLK_BUS_SPDIF	99
#define	H6_CLK_DMIC		100
#define	H6_CLK_BUS_DMIC		101
#define	H6_CLK_AUDIO_HUB	102
#define	H6_CLK_BUS_AUDIO_HUB	103
#define	H6_CLK_USB_OHCI0	104
#define	H6_CLK_USB_PHY0		105
#define	H6_CLK_USB_PHY1		106
#define	H6_CLK_USB_OHCI3	107
#define	H6_CLK_USB_PHY3		108
#define	H6_CLK_USB_HSIC_12M	109
#define	H6_CLK_USB_HSIC		110
#define	H6_CLK_BUS_OHCI0	111
#define	H6_CLK_BUS_OHCI3	112
#define	H6_CLK_BUS_EHCI0	113
#define	H6_CLK_BUS_XHCI		114
#define	H6_CLK_BUS_EHCI3	115
#define	H6_CLK_BUS_OTG		116
#define	H6_CLK_PCIE_REF_100M	117
#define	H6_CLK_PCIE_REF		118
#define	H6_CLK_PCIE_REF_OUT	119
#define	H6_CLK_PCIE_MAXI	120
#define	H6_CLK_PCIE_AUX		121
#define	H6_CLK_BUS_PCIE		122
#define	H6_CLK_HDMI		123
#define	H6_CLK_HDMI_SLOW	124
#define	H6_CLK_HDMI_CEC		125
#define	H6_CLK_BUS_HDMI		126
#define	H6_CLK_BUS_TCON_TOP	127
#define	H6_CLK_TCON_LCD0	128
#define	H6_CLK_BUS_TCON_LCD0	129
#define	H6_CLK_TCON_TV0		130
#define	H6_CLK_BUS_TCON_TV0	131
#define	H6_CLK_CSI_CCI		132
#define	H6_CLK_CSI_TOP		133
#define	H6_CLK_CSI_MCLK		134
#define	H6_CLK_BUS_CSI		135
#define	H6_CLK_HDCP		136
#define	H6_CLK_BUS_HDCP		137

#endif /* !_SUN50I_H6_CCU_H */
