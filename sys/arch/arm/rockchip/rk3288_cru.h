/* $NetBSD: rk3288_cru.h,v 1.1 2021/11/12 22:02:08 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _RK3328_CRU_H
#define _RK3328_CRU_H

#define  RK3288_PLL_APLL                            1
#define  RK3288_PLL_DPLL                            2
#define  RK3288_PLL_CPLL                            3
#define  RK3288_PLL_GPLL                            4
#define  RK3288_PLL_NPLL                            5
#define  RK3288_ARMCLK                              6
#define  RK3288_SCLK_GPU                            64
#define  RK3288_SCLK_SPI0                           65
#define  RK3288_SCLK_SPI1                           66
#define  RK3288_SCLK_SPI2                           67
#define  RK3288_SCLK_SDMMC                          68
#define  RK3288_SCLK_SDIO0                          69
#define  RK3288_SCLK_SDIO1                          70
#define  RK3288_SCLK_EMMC                           71
#define  RK3288_SCLK_TSADC                          72
#define  RK3288_SCLK_SARADC                         73
#define  RK3288_SCLK_PS2C                           74
#define  RK3288_SCLK_NANDC0                         75
#define  RK3288_SCLK_NANDC1                         76
#define  RK3288_SCLK_UART0                          77
#define  RK3288_SCLK_UART1                          78
#define  RK3288_SCLK_UART2                          79
#define  RK3288_SCLK_UART3                          80
#define  RK3288_SCLK_UART4                          81
#define  RK3288_SCLK_I2S0                           82
#define  RK3288_SCLK_SPDIF                          83
#define  RK3288_SCLK_SPDIF8CH                       84
#define  RK3288_SCLK_TIMER0                         85
#define  RK3288_SCLK_TIMER1                         86
#define  RK3288_SCLK_TIMER2                         87
#define  RK3288_SCLK_TIMER3                         88
#define  RK3288_SCLK_TIMER4                         89
#define  RK3288_SCLK_TIMER5                         90
#define  RK3288_SCLK_TIMER6                         91
#define  RK3288_SCLK_HSADC                          92
#define  RK3288_SCLK_OTGPHY0                        93
#define  RK3288_SCLK_OTGPHY1                        94
#define  RK3288_SCLK_OTGPHY2                        95
#define  RK3288_SCLK_OTG_ADP                        96
#define  RK3288_SCLK_HSICPHY480M                    97
#define  RK3288_SCLK_HSICPHY12M                     98
#define  RK3288_SCLK_MACREF                         99
#define  RK3288_SCLK_LCDC_PWM0                      100
#define  RK3288_SCLK_LCDC_PWM1                      101
#define  RK3288_SCLK_MAC_RX                         102
#define  RK3288_SCLK_MAC_TX                         103
#define  RK3288_SCLK_EDP_24M                        104
#define  RK3288_SCLK_EDP                            105
#define  RK3288_SCLK_RGA                            106
#define  RK3288_SCLK_ISP                            107
#define  RK3288_SCLK_ISP_JPE                        108
#define  RK3288_SCLK_HDMI_HDCP                      109
#define  RK3288_SCLK_HDMI_CEC                       110
#define  RK3288_SCLK_HEVC_CABAC                     111
#define  RK3288_SCLK_HEVC_CORE                      112
#define  RK3288_SCLK_I2S0_OUT                       113
#define  RK3288_SCLK_SDMMC_DRV                      114
#define  RK3288_SCLK_SDIO0_DRV                      115
#define  RK3288_SCLK_SDIO1_DRV                      116
#define  RK3288_SCLK_EMMC_DRV                       117
#define  RK3288_SCLK_SDMMC_SAMPLE                   118
#define  RK3288_SCLK_SDIO0_SAMPLE                   119
#define  RK3288_SCLK_SDIO1_SAMPLE                   120
#define  RK3288_SCLK_EMMC_SAMPLE                    121
#define  RK3288_SCLK_USBPHY480M_SRC                 122
#define  RK3288_SCLK_PVTM_CORE                      123
#define  RK3288_SCLK_PVTM_GPU                       124
#define  RK3288_SCLK_CRYPTO                         125
#define  RK3288_SCLK_MIPIDSI_24M                    126
#define  RK3288_SCLK_VIP_OUT                        127
#define  RK3288_SCLK_MAC                            151
#define  RK3288_SCLK_MACREF_OUT                     152
#define  RK3288_DCLK_VOP0                           190
#define  RK3288_DCLK_VOP1                           191
#define  RK3288_ACLK_GPU                            192
#define  RK3288_ACLK_DMAC1                          193
#define  RK3288_ACLK_DMAC2                          194
#define  RK3288_ACLK_MMU                            195
#define  RK3288_ACLK_GMAC                           196
#define  RK3288_ACLK_VOP0                           197
#define  RK3288_ACLK_VOP1                           198
#define  RK3288_ACLK_CRYPTO                         199
#define  RK3288_ACLK_RGA                            200
#define  RK3288_ACLK_RGA_NIU                        201
#define  RK3288_ACLK_IEP                            202
#define  RK3288_ACLK_VIO0_NIU                       203
#define  RK3288_ACLK_VIP                            204
#define  RK3288_ACLK_ISP                            205
#define  RK3288_ACLK_VIO1_NIU                       206
#define  RK3288_ACLK_HEVC                           207
#define  RK3288_ACLK_VCODEC                         208
#define  RK3288_ACLK_CPU                            209
#define  RK3288_ACLK_PERI                           210
#define  RK3288_PCLK_GPIO0                          320
#define  RK3288_PCLK_GPIO1                          321
#define  RK3288_PCLK_GPIO2                          322
#define  RK3288_PCLK_GPIO3                          323
#define  RK3288_PCLK_GPIO4                          324
#define  RK3288_PCLK_GPIO5                          325
#define  RK3288_PCLK_GPIO6                          326
#define  RK3288_PCLK_GPIO7                          327
#define  RK3288_PCLK_GPIO8                          328
#define  RK3288_PCLK_GRF                            329
#define  RK3288_PCLK_SGRF                           330
#define  RK3288_PCLK_PMU                            331
#define  RK3288_PCLK_I2C0                           332
#define  RK3288_PCLK_I2C1                           333
#define  RK3288_PCLK_I2C2                           334
#define  RK3288_PCLK_I2C3                           335
#define  RK3288_PCLK_I2C4                           336
#define  RK3288_PCLK_I2C5                           337
#define  RK3288_PCLK_SPI0                           338
#define  RK3288_PCLK_SPI1                           339
#define  RK3288_PCLK_SPI2                           340
#define  RK3288_PCLK_UART0                          341
#define  RK3288_PCLK_UART1                          342
#define  RK3288_PCLK_UART2                          343
#define  RK3288_PCLK_UART3                          344
#define  RK3288_PCLK_UART4                          345
#define  RK3288_PCLK_TSADC                          346
#define  RK3288_PCLK_SARADC                         347
#define  RK3288_PCLK_SIM                            348
#define  RK3288_PCLK_GMAC                           349
#define  RK3288_PCLK_PWM                            350
#define  RK3288_PCLK_RKPWM                          351
#define  RK3288_PCLK_PS2C                           352
#define  RK3288_PCLK_TIMER                          353
#define  RK3288_PCLK_TZPC                           354
#define  RK3288_PCLK_EDP_CTRL                       355
#define  RK3288_PCLK_MIPI_DSI0                      356
#define  RK3288_PCLK_MIPI_DSI1                      357
#define  RK3288_PCLK_MIPI_CSI                       358
#define  RK3288_PCLK_LVDS_PHY                       359
#define  RK3288_PCLK_HDMI_CTRL                      360
#define  RK3288_PCLK_VIO2_H2P                       361
#define  RK3288_PCLK_CPU                            362
#define  RK3288_PCLK_PERI                           363
#define  RK3288_PCLK_DDRUPCTL0                      364
#define  RK3288_PCLK_PUBL0                          365
#define  RK3288_PCLK_DDRUPCTL1                      366
#define  RK3288_PCLK_PUBL1                          367
#define  RK3288_PCLK_WDT                            368
#define  RK3288_PCLK_EFUSE256                       369
#define  RK3288_PCLK_EFUSE1024                      370
#define  RK3288_PCLK_ISP_IN                         371
#define  RK3288_HCLK_GPS                            448
#define  RK3288_HCLK_OTG0                           449
#define  RK3288_HCLK_USBHOST0                       450
#define  RK3288_HCLK_USBHOST1                       451
#define  RK3288_HCLK_HSIC                           452
#define  RK3288_HCLK_NANDC0                         453
#define  RK3288_HCLK_NANDC1                         454
#define  RK3288_HCLK_TSP                            455
#define  RK3288_HCLK_SDMMC                          456
#define  RK3288_HCLK_SDIO0                          457
#define  RK3288_HCLK_SDIO1                          458
#define  RK3288_HCLK_EMMC                           459
#define  RK3288_HCLK_HSADC                          460
#define  RK3288_HCLK_CRYPTO                         461
#define  RK3288_HCLK_I2S0                           462
#define  RK3288_HCLK_SPDIF                          463
#define  RK3288_HCLK_SPDIF8CH                       464
#define  RK3288_HCLK_VOP0                           465
#define  RK3288_HCLK_VOP1                           466
#define  RK3288_HCLK_ROM                            467
#define  RK3288_HCLK_IEP                            468
#define  RK3288_HCLK_ISP                            469
#define  RK3288_HCLK_RGA                            470
#define  RK3288_HCLK_VIO_AHB_ARBI                   471
#define  RK3288_HCLK_VIO_NIU                        472
#define  RK3288_HCLK_VIP                            473
#define  RK3288_HCLK_VIO2_H2P                       474
#define  RK3288_HCLK_HEVC                           475
#define  RK3288_HCLK_VCODEC                         476
#define  RK3288_HCLK_CPU                            477
#define  RK3288_HCLK_PERI                           478

#endif /* !_RK3328_CRU_H */
