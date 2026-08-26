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

#ifndef _SUN50I_H616_CCU_H
#define _SUN50I_H616_CCU_H

/*
 * reset ids - must match dt-bindings/reset/sun50i-h616-ccu.h
 */
#define H616_RST_MBUS              0
#define H616_RST_BUS_DE            1
#define H616_RST_BUS_DEINTERLACE   2
#define H616_RST_BUS_GPU           3
#define H616_RST_BUS_CE            4
#define H616_RST_BUS_VE            5
#define H616_RST_BUS_DMA           6
#define H616_RST_BUS_HSTIMER       7
#define H616_RST_BUS_DBG           8
#define H616_RST_BUS_PSI           9
#define H616_RST_BUS_PWM           10
#define H616_RST_BUS_IOMMU         11
#define H616_RST_BUS_DRAM          12
#define H616_RST_BUS_NAND          13
#define H616_RST_BUS_MMC0          14
#define H616_RST_BUS_MMC1          15
#define H616_RST_BUS_MMC2          16
#define H616_RST_BUS_UART0         17
#define H616_RST_BUS_UART1         18
#define H616_RST_BUS_UART2         19
#define H616_RST_BUS_UART3         20
#define H616_RST_BUS_UART4         21
#define H616_RST_BUS_UART5         22
#define H616_RST_BUS_I2C0          23
#define H616_RST_BUS_I2C1          24
#define H616_RST_BUS_I2C2          25
#define H616_RST_BUS_I2C3          26
#define H616_RST_BUS_I2C4          27
#define H616_RST_BUS_SPI0          28
#define H616_RST_BUS_SPI1          29
#define H616_RST_BUS_EMAC0         30
#define H616_RST_BUS_EMAC1         31
#define H616_RST_BUS_TS            32
#define H616_RST_BUS_THS           33
#define H616_RST_BUS_SPDIF         34
#define H616_RST_BUS_DMIC          35
#define H616_RST_BUS_AUDIO_CODEC   36
#define H616_RST_BUS_AUDIO_HUB     37
#define H616_RST_USB_PHY0          38
#define H616_RST_USB_PHY1          39
#define H616_RST_USB_PHY2          40
#define H616_RST_USB_PHY3          41
#define H616_RST_BUS_OHCI0         42
#define H616_RST_BUS_OHCI1         43
#define H616_RST_BUS_OHCI2         44
#define H616_RST_BUS_OHCI3         45
#define H616_RST_BUS_EHCI0         46
#define H616_RST_BUS_EHCI1         47
#define H616_RST_BUS_EHCI2         48
#define H616_RST_BUS_EHCI3         49
#define H616_RST_BUS_OTG           50
#define H616_RST_BUS_HDMI          51
#define H616_RST_BUS_HDMI_SUB      52
#define H616_RST_BUS_TCON_TOP      53
#define H616_RST_BUS_TCON_TV0      54
#define H616_RST_BUS_TCON_TV1      55
#define H616_RST_BUS_TVE_TOP       56
#define H616_RST_BUS_TVE0          57
#define H616_RST_BUS_HDCP          58
#define H616_RST_BUS_KEYADC        59
#define H616_RST_BUS_GPADC         60

/*
 * clock ids - internal pll clocks (not exported via dt-bindings)
 */
#define H616_CLK_OSC12M            0
#define H616_CLK_PLL_CPUX          1
#define H616_CLK_PLL_DDR0          2
#define H616_CLK_PLL_DDR1          3
#define H616_CLK_PLL_PERIPH0       4
#define H616_CLK_PLL_PERIPH0_2X    5
#define H616_CLK_PLL_PERIPH1       6
#define H616_CLK_PLL_PERIPH1_2X    7
#define H616_CLK_PLL_GPU           8
#define H616_CLK_PLL_VIDEO0        9
#define H616_CLK_PLL_VIDEO0_4X     10
#define H616_CLK_PLL_VIDEO1        11
#define H616_CLK_PLL_VIDEO1_4X     12
#define H616_CLK_PLL_VIDEO2        13
#define H616_CLK_PLL_VIDEO2_4X     14
#define H616_CLK_PLL_VE            15
#define H616_CLK_PLL_DE            16
#define H616_CLK_PLL_AUDIO_HS      17
#define H616_CLK_PLL_AUDIO_1X      18
#define H616_CLK_PLL_AUDIO_2X      19
#define H616_CLK_PLL_AUDIO_4X      20

/*
 * clock ids - bus clocks (exported via dt-bindings)
 */
#define H616_CLK_CPUX              21
#define H616_CLK_AXI               22
#define H616_CLK_CPUX_APB          23
#define H616_CLK_PSI_AHB1_AHB2    24
#define H616_CLK_AHB3             25
#define H616_CLK_APB1             26
#define H616_CLK_APB2             27
#define H616_CLK_MBUS             28

/*
 * clock ids - module and bus gate clocks (exported via dt-bindings)
 */
#define H616_CLK_DE               29
#define H616_CLK_BUS_DE           30
#define H616_CLK_DEINTERLACE      31
#define H616_CLK_BUS_DEINTERLACE  32
#define H616_CLK_G2D              33
#define H616_CLK_BUS_G2D          34
#define H616_CLK_GPU0             35
#define H616_CLK_BUS_GPU          36
#define H616_CLK_GPU1             37
#define H616_CLK_CE               38
#define H616_CLK_BUS_CE           39
#define H616_CLK_VE               40
#define H616_CLK_BUS_VE           41
#define H616_CLK_BUS_DMA          42
#define H616_CLK_BUS_HSTIMER      43
#define H616_CLK_AVS              44
#define H616_CLK_BUS_DBG          45
#define H616_CLK_BUS_PSI          46
#define H616_CLK_BUS_PWM          47
#define H616_CLK_BUS_IOMMU        48
#define H616_CLK_DRAM             49
#define H616_CLK_MBUS_DMA         50
#define H616_CLK_MBUS_VE          51
#define H616_CLK_MBUS_CE          52
#define H616_CLK_MBUS_TS          53
#define H616_CLK_MBUS_NAND        54
#define H616_CLK_MBUS_G2D         55
#define H616_CLK_BUS_DRAM         56
#define H616_CLK_NAND0            57
#define H616_CLK_NAND1            58
#define H616_CLK_BUS_NAND         59
#define H616_CLK_MMC0             60
#define H616_CLK_MMC1             61
#define H616_CLK_MMC2             62
#define H616_CLK_BUS_MMC0         63
#define H616_CLK_BUS_MMC1         64
#define H616_CLK_BUS_MMC2         65
#define H616_CLK_BUS_UART0        66
#define H616_CLK_BUS_UART1        67
#define H616_CLK_BUS_UART2        68
#define H616_CLK_BUS_UART3        69
#define H616_CLK_BUS_UART4        70
#define H616_CLK_BUS_UART5        71
#define H616_CLK_BUS_I2C0         72
#define H616_CLK_BUS_I2C1         73
#define H616_CLK_BUS_I2C2         74
#define H616_CLK_BUS_I2C3         75
#define H616_CLK_BUS_I2C4         76
#define H616_CLK_SPI0             77
#define H616_CLK_SPI1             78
#define H616_CLK_BUS_SPI0         79
#define H616_CLK_BUS_SPI1         80
#define H616_CLK_EMAC_25M         81
#define H616_CLK_BUS_EMAC0        82
#define H616_CLK_BUS_EMAC1        83
#define H616_CLK_TS               84
#define H616_CLK_BUS_TS           85
#define H616_CLK_BUS_THS          86
#define H616_CLK_SPDIF            87
#define H616_CLK_BUS_SPDIF        88
#define H616_CLK_DMIC             89
#define H616_CLK_BUS_DMIC         90
#define H616_CLK_AUDIO_CODEC_1X   91
#define H616_CLK_AUDIO_CODEC_4X   92
#define H616_CLK_BUS_AUDIO_CODEC  93
#define H616_CLK_AUDIO_HUB        94
#define H616_CLK_BUS_AUDIO_HUB    95
#define H616_CLK_USB_OHCI0        96
#define H616_CLK_USB_PHY0         97
#define H616_CLK_USB_OHCI1        98
#define H616_CLK_USB_PHY1         99
#define H616_CLK_USB_OHCI2        100
#define H616_CLK_USB_PHY2         101
#define H616_CLK_USB_OHCI3        102
#define H616_CLK_USB_PHY3         103
#define H616_CLK_BUS_OHCI0        104
#define H616_CLK_BUS_OHCI1        105
#define H616_CLK_BUS_OHCI2        106
#define H616_CLK_BUS_OHCI3        107
#define H616_CLK_BUS_EHCI0        108
#define H616_CLK_BUS_EHCI1        109
#define H616_CLK_BUS_EHCI2        110
#define H616_CLK_BUS_EHCI3        111
#define H616_CLK_BUS_OTG          112
#define H616_CLK_BUS_KEYADC       113
#define H616_CLK_HDMI             114
#define H616_CLK_HDMI_SLOW        115
#define H616_CLK_HDMI_CEC         116
#define H616_CLK_BUS_HDMI         117
#define H616_CLK_BUS_TCON_TOP     118
#define H616_CLK_TCON_TV0         119
#define H616_CLK_TCON_TV1         120
#define H616_CLK_BUS_TCON_TV0     121
#define H616_CLK_BUS_TCON_TV1     122
#define H616_CLK_TVE0             123
#define H616_CLK_BUS_TVE_TOP      124
#define H616_CLK_BUS_TVE0         125
#define H616_CLK_HDCP             126
#define H616_CLK_BUS_HDCP         127
#define H616_CLK_PLL_SYSTEM_32K   128
#define H616_CLK_BUS_GPADC        129

/* internal clocks not exported via dt-bindings */
#define H616_CLK_PLL_PERIPH0_4X  130
#define H616_CLK_PLL_PERIPH1_4X  131

#endif /* !_SUN50I_H616_CCU_H */
