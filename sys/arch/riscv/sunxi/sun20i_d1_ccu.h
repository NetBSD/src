/* $NetBSD: sun20i_d1_ccu.h,v 1.1 2024/08/13 07:20:23 skrll Exp $ */

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

#ifndef __CCU_D1_H__
#define __CCU_D1_H__

#define D1_CLK_PLL_CPU		0
#define D1_CLK_PLL_DDR		1
#define D1_CLK_PLL_PERIPH_4X	2
#define D1_CLK_PLL_PERIPH_2X	3
#define D1_CLK_PLL_PERIPH_800M	4
#define D1_CLK_PLL_PERIPH	5
#define D1_CLK_PLL_PERIPH_DIV3	6
#define D1_CLK_PLL_VIDEO0_4X	7
#define D1_CLK_PLL_VIDEO0_2X	8
#define D1_CLK_PLL_VIDEO0	9
#define D1_CLK_PLL_VIDEO1_4X	10
#define D1_CLK_PLL_VIDEO1_2X	11
#define D1_CLK_PLL_VIDEO1	12
#define D1_CLK_PLL_VE		13
#define D1_CLK_PLL_AUDIO0_4X	14
#define D1_CLK_PLL_AUDIO0_2X	15
#define D1_CLK_PLL_AUDIO0	16
#define D1_CLK_PLL_AUDIO1	17
#define D1_CLK_PLL_AUDIO1_DIV2	18
#define D1_CLK_PLL_AUDIO1_DIV5	19
#define D1_CLK_CPU		20
#define D1_CLK_CPU_AXI		21
#define D1_CLK_CPU_APB		22
#define D1_CLK_PSI		23
#define D1_CLK_APB0		24
#define D1_CLK_APB1		25
#define D1_CLK_MBUS		26
#define D1_CLK_DE		27
#define D1_CLK_BUS_DE		28
#define D1_CLK_DI		29
#define D1_CLK_BUS_DI		30
#define D1_CLK_G2D		31
#define D1_CLK_BUS_G2D		32
#define D1_CLK_CE		33
#define D1_CLK_BUS_CE		34
#define D1_CLK_VE		35
#define D1_CLK_BUS_VE		36
#define D1_CLK_BUS_DMA		37
#define D1_CLK_BUS_MSGBOX0	38
#define D1_CLK_BUS_MSGBOX1	39
#define D1_CLK_BUS_MSGBOX2	40
#define D1_CLK_BUS_SPINLOCK	41
#define D1_CLK_BUS_HSTIMER	42
#define D1_CLK_AVS		43
#define D1_CLK_BUS_DBGSYS	44
#define D1_CLK_BUS_PWM		45
#define D1_CLK_BUS_IOMMU	46
#define D1_CLK_DRAM		47
#define D1_CLK_MBUS_DMA		48
#define D1_CLK_MBUS_VE		49
#define D1_CLK_MBUS_CE		50
#define D1_CLK_MBUS_TVIN	51
#define D1_CLK_MBUS_CSI		52
#define D1_CLK_MBUS_G2D		53
#define D1_CLK_MBUS_RISCV	54
#define D1_CLK_BUS_DRAM		55
#define D1_CLK_MMC0		56
#define D1_CLK_MMC1		57
#define D1_CLK_MMC2		58
#define D1_CLK_BUS_MMC0		59
#define D1_CLK_BUS_MMC1		60
#define D1_CLK_BUS_MMC2		61
#define D1_CLK_BUS_UART0	62
#define D1_CLK_BUS_UART1	63
#define D1_CLK_BUS_UART2	64
#define D1_CLK_BUS_UART3	65
#define D1_CLK_BUS_UART4	66
#define D1_CLK_BUS_UART5	67
#define D1_CLK_BUS_I2C0		68
#define D1_CLK_BUS_I2C1		69
#define D1_CLK_BUS_I2C2		70
#define D1_CLK_BUS_I2C3		71
#define D1_CLK_SPI0		72
#define D1_CLK_SPI1		73
#define D1_CLK_BUS_SPI0		74
#define D1_CLK_BUS_SPI1		75
#define D1_CLK_EMAC_25M		76
#define D1_CLK_BUS_EMAC		77
#define D1_CLK_IRTX		78
#define D1_CLK_BUS_IRTX		79
#define D1_CLK_BUS_GPADC	80
#define D1_CLK_BUS_THS		81
#define D1_CLK_I2S0		82
#define D1_CLK_I2S1		83
#define D1_CLK_I2S2		84
#define D1_CLK_I2S2_ASRC	85
#define D1_CLK_BUS_I2S0		86
#define D1_CLK_BUS_I2S1		87
#define D1_CLK_BUS_I2S2		88
#define D1_CLK_SPDIF_TX		89
#define D1_CLK_SPDIF_RX		90
#define D1_CLK_BUS_SPDIF	91
#define D1_CLK_DMIC		92
#define D1_CLK_BUS_DMIC		93
#define D1_CLK_AUDIO_DAC	94
#define D1_CLK_AUDIO_ADC	95
#define D1_CLK_BUS_AUDIO	96
#define D1_CLK_USB_PHY0		97
#define D1_CLK_USB_PHY1		98
#define D1_CLK_BUS_OHCI0	99
#define D1_CLK_BUS_OHCI1	100
#define D1_CLK_BUS_EHCI0	101
#define D1_CLK_BUS_EHCI1	102
#define D1_CLK_BUS_OTG		103
#define D1_CLK_BUS_LRADC	104
#define D1_CLK_BUS_DPSS_TOP	105
#define D1_CLK_HDMI_24M		106
#define D1_CLK_HDMI_CEC_32K	107
#define D1_CLK_HDMI_CEC		108
#define D1_CLK_BUS_HDMI		109
#define D1_CLK_DSI		110
#define D1_CLK_BUS_DSI		111
#define D1_CLK_TCONLCD		112
#define D1_CLK_BUS_TCONLCD	113
#define D1_CLK_TCONTV		114
#define D1_CLK_BUS_TCONTV	115
#define D1_CLK_TVE		116
#define D1_CLK_BUS_TVE_TOP	117
#define D1_CLK_BUS_TVE		118
#define D1_CLK_TVD		119
#define D1_CLK_BUS_TVD_TOP	120
#define D1_CLK_BUS_TVD		121
#define D1_CLK_LEDC		122
#define D1_CLK_BUS_LEDC		123
#define D1_CLK_CSI_TOP		124
#define D1_CLK_CSI_MCLK		125
#define D1_CLK_BUS_CSI		126
#define D1_CLK_TPADC		127
#define D1_CLK_BUS_TPADC	128
#define D1_CLK_BUS_TZMA		129
#define D1_CLK_DSP		130
#define D1_CLK_BUS_DSP_CFG	131
#define D1_CLK_RISCV		132
#define D1_CLK_RISCV_AXI	133
#define D1_CLK_BUS_RISCV_CFG	134
#define D1_CLK_FANOUT_24M	135
#define D1_CLK_FANOUT_12M	136
#define D1_CLK_FANOUT_16M	137
#define D1_CLK_FANOUT_25M	138
#define D1_CLK_FANOUT_32K	139
#define D1_CLK_FANOUT_27M	140
#define D1_CLK_FANOUT_PCLK	141
#define D1_CLK_FANOUT0		142
#define D1_CLK_FANOUT1		143
#define D1_CLK_FANOUT2		144
#define D1_CLK_BUS_CAN0		145
#define D1_CLK_BUS_CAN1		146

#define D1_RST_MBUS		0
#define D1_RST_BUS_DE		1
#define D1_RST_BUS_DI		2
#define D1_RST_BUS_G2D		3
#define D1_RST_BUS_CE		4
#define D1_RST_BUS_VE		5
#define D1_RST_BUS_DMA		6
#define D1_RST_BUS_MSGBOX0	7
#define D1_RST_BUS_MSGBOX1	8
#define D1_RST_BUS_MSGBOX2	9
#define D1_RST_BUS_SPINLOCK	10
#define D1_RST_BUS_HSTIMER	11
#define D1_RST_BUS_DBGSYS	12
#define D1_RST_BUS_PWM		13
#define D1_RST_BUS_DRAM		14
#define D1_RST_BUS_MMC0		15
#define D1_RST_BUS_MMC1		16
#define D1_RST_BUS_MMC2		17
#define D1_RST_BUS_UART0	18
#define D1_RST_BUS_UART1	19
#define D1_RST_BUS_UART2	20
#define D1_RST_BUS_UART3	21
#define D1_RST_BUS_UART4	22
#define D1_RST_BUS_UART5	23
#define D1_RST_BUS_I2C0		24
#define D1_RST_BUS_I2C1		25
#define D1_RST_BUS_I2C2		26
#define D1_RST_BUS_I2C3		27
#define D1_RST_BUS_SPI0		28
#define D1_RST_BUS_SPI1		29
#define D1_RST_BUS_EMAC		30
#define D1_RST_BUS_IRTX		31
#define D1_RST_BUS_GPADC	32
#define D1_RST_BUS_THS		33
#define D1_RST_BUS_I2S0		34
#define D1_RST_BUS_I2S1		35
#define D1_RST_BUS_I2S2		36
#define D1_RST_BUS_SPDIF	37
#define D1_RST_BUS_DMIC		38
#define D1_RST_BUS_AUDIO	39
#define D1_RST_USB_PHY0		40
#define D1_RST_USB_PHY1		41
#define D1_RST_BUS_OHCI0	42
#define D1_RST_BUS_OHCI1	43
#define D1_RST_BUS_EHCI0	44
#define D1_RST_BUS_EHCI1	45
#define D1_RST_BUS_OTG		46
#define D1_RST_BUS_LRADC	47
#define D1_RST_BUS_DPSS_TOP	48
#define D1_RST_BUS_HDMI_SUB	49
#define D1_RST_BUS_HDMI_MAIN	50
#define D1_RST_BUS_DSI		51
#define D1_RST_BUS_TCONLCD	52
#define D1_RST_BUS_TCONTV	53
#define D1_RST_BUS_LVDS		54
#define D1_RST_BUS_TVE		55
#define D1_RST_BUS_TVE_TOP	56
#define D1_RST_BUS_TVD		57
#define D1_RST_BUS_TVD_TOP	58
#define D1_RST_BUS_LEDC		59
#define D1_RST_BUS_CSI		60
#define D1_RST_BUS_TPADC	61
#define D1_RST_BUS_DSP		62
#define D1_RST_BUS_DSP_CFG	63
#define D1_RST_BUS_DSP_DBG	64
#define D1_RST_BUS_RISCV_CFG	65
#define D1_RST_BUS_CAN0		66
#define D1_RST_BUS_CAN1		67

#endif /* __CCU_D1_H__ */
