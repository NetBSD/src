/*	$NetBSD: imx6_intr.h,v 1.1.2.1 2017/12/03 11:35:53 jdolecek Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_ARM_IMX6_INTR_H_
#define _ARM_ARM_IMX6_INTR_H_

#define	PIC_MAXSOURCES			256
#define	PIC_MAXMAXSOURCES		(256 + 6 * 32)
#define	__HAVE_PIC_PENDING_INTRS	/* for imxgpio */

/*
 * The BCM53xx uses a generic interrupt controller so pull that stuff.
 */
#include <arm/cortex/gic_intr.h>
#include <arm/cortex/a9tmr_intr.h>	/* A9 Timer PPIs */

#define	IRQ_IOMUXC		32
#define	IRQ_DAP			33
#define	IRQ_SDMA		34
#define	IRQ_VPU_JPEG		35
#define	IRQ_TSC			35	/* iMX6UL */
#define	IRQ_SNVS_PMIC		36
#define	IRQ_IPU1_ERROR		37
#define	IRQ_LCDIF_SYNC		37	/* iMX6UL */
#define	IRQ_IPU1_SYNC		38
#define	IRQ_BEE			38	/* iMX6UL */
#define	IRQ_IPU2_ERROR		39
#define	IRQ_CSI			39	/* iMX6UL */
#define	IRQ_IPU2_SYNC		40
#define	IRQ_PXP			40	/* iMX6UL */
#define	IRQ_GPU3D		41
#define	IRQ_GPU2D_IDLE		42
#define	IRQ_SCTR_A		41	/* iMX6UL */
#define	IRQ_SCTR_B		42	/* iMX6UL */
#define	IRQ_GPU2D		43
#define	IRQ_WDOG3		43	/* iMX6UL */
#define	IRQ_VPU			44
#define	IRQ_APBH		45
#define	IRQ_EIM			46
#define	IRQ_BCH			47
#define	IRQ_GPMI		48
#define	IRQ_DTCP		49
#define	IRQ_UART6		49	/* iMX6UL */
#define	IRQ_VDOA		50
#define	IRQ_SNVS		51
#define	IRQ_SNVS_SEC		52
#define	IRQ_CSU			53
#define	IRQ_USDHC1		54
#define	IRQ_USDHC2		55
#define	IRQ_USDHC3		56
#define	IRQ_USDHC4		57
#define	IRQ_SAI3_A		56	/* iMX6UL */
#define	IRQ_SAI3_B		57	/* iMX6UL */
#define	IRQ_UART1		58
#define	IRQ_UART2		59
#define	IRQ_UART3		60
#define	IRQ_UART4		61
#define	IRQ_UART5		62
#define	IRQ_ECSPI1		63
#define	IRQ_ECSPI2		64
#define	IRQ_ECSPI3		65
#define	IRQ_ECSPI4		66
#define	IRQ_ECSPI5		67
#define	IRQ_I2C4		67	/* iMX6UL */
#define	IRQ_I2C1		68
#define	IRQ_I2C2		69
#define	IRQ_I2C3		70
#define	IRQ_SATA		71
#define	IRQ_UART7		71	/* iMX6UL */
#define	IRQ_USB1		72
#define	IRQ_UART8		72	/* iMX6UL */
#define	IRQ_USB2		73
#define	IRQ__RSVD73		73	/* iMX6UL */
#define	IRQ_USB3		74
#define	IRQ_USBOTG2		74	/* iMX6UL */
#define	IRQ_USBOTG		75
#define	IRQ_USBOTG1		75	/* iMX6UL */
#define	IRQ_USBPHY0		76
#define	IRQ_USBPHY1		77
#define	IRQ_SSI1		78
#define	IRQ_CAAM_JQ2		78	/* iMX6UL */
#define	IRQ_SSI2		79
#define	IRQ_CAAM_ERR		79	/* iMX6UL */
#define	IRQ_SSI3		80
#define	IRQ_CAAM_RTIC		80	/* iMX6UL */
#define	IRQ_TEMP		81
#define	IRQ_ASRC		82
#define	IRQ_ESAI		83
#define	IRQ_SPDIF		84
#define	IRQ_MLB			85
#define	IRQ_PMU			86
#define	IRQ_GPT1		87
#define	IRQ_EPIT1		88
#define	IRQ_EPIT2		89
#define	IRQ_GPIO7		90
#define	IRQ_GPIO6		91
#define	IRQ_GPIO5		92
#define	IRQ_GPIO4		93
#define	IRQ_GPIO3		94
#define	IRQ_GPIO2		95
#define	IRQ_GPIO1		96
#define	IRQ_GPIO0		97
#define	IRQ_GPIO1L		98
#define	IRQ_GPIO1H		99
#define	IRQ_GPIO2L		100
#define	IRQ_GPIO2H		101
#define	IRQ_GPIO3L		102
#define	IRQ_GPIO3H		103
#define	IRQ_GPIO4L		104
#define	IRQ_GPIO4H		105
#define	IRQ_GPIO5L		106
#define	IRQ_GPIO5H		107
#define	IRQ_GPIO6L		108
#define	IRQ_GPIO6H		109
#define	IRQ_GPIO7L		110
#define	IRQ_GPIO7H		111
#define	IRQ_WDOG1		112
#define	IRQ_WDOG2		113
#define	IRQ_KPP			114
#define	IRQ_PWM1		115
#define	IRQ_PWM2		116
#define	IRQ_PWM3		117
#define	IRQ_PWM4		118
#define	IRQ_CCM1		119
#define	IRQ_CCM2		120
#define	IRQ_GPC			121
#define	IRQ__RSVD122		122
#define	IRQ_SRC			123
#define	IRQ_CPU_L2		124
#define	IRQ_CPU_PARITY		125
#define	IRQ_CPU_PMU		126
#define	IRQ_CPU_CTI		127
#define	IRQ_CPU_WDOG		128
#define	IRQ__RSVD129		129
#define	IRQ_SAI1		129	/* iMX6UL */
#define	IRQ__RSVD130		130
#define	IRQ_SAI2		130	/* iMX6UL */
#define	IRQ__RSVD131		131
#define	IRQ_MIPI_CSI1		132
#define	IRQ_ADC1		132	/* iMX6UL */
#define	IRQ_MIPI_CSI2		133
#define	IRQ_ADC2		133	/* iMX6UL */
#define	IRQ_MIPI_DSI		134
#define	IRQ__RSVD134		134	/* iMX6UL */
#define	IRQ_MIPI_HSI		135
#define	IRQ__RSVD135		135	/* iMX6UL */
#define	IRQ_SJC			136
#define	IRQ_CAAM0		137
#define	IRQ_CAAM1		138
#define	IRQ__RSVD139		139
#define	IRQ_QSPI		139	/* iMX6UL */
#define	IRQ_ASC1		140
#define	IRQ_ASC2		141
#define	IRQ_GPT2		141	/* iMX6UL */
#define	IRQ_FLEXCAN1		142
#define	IRQ_FLEXCAN2		143
#define	IRQ_SIM1		144	/* iMX6UL */
#define	IRQ_SIM2		145	/* iMX6UL */
#define	IRQ_PWM5		146	/* iMX6UL */
#define	IRQ_HDMI_MASTER		147
#define	IRQ_PWM6		147	/* iMX6UL */
#define	IRQ_HDMI_CEC		148
#define	IRQ_PWM7		148	/* iMX6UL */
#define	IRQ_MLB150L		149
#define	IRQ_PWM8		149	/* iMX6UL */
#define	IRQ_ENET1		150
#define	IRQ_ENET1_1588		151
#define	IRQ_ENET2		152	/* iMX6UL */
#define	IRQ_ENET2_1588		153	/* iMX6UL */
#define	IRQ_PCIE1		152
#define	IRQ_PCIE2		153
#define	IRQ_PCIE3		154
#define	IRQ_PCIE4		155
#define	IRQ_DCIC1		156
#define	IRQ_DCIC2		157
#define	IRQ_MLB150H		158
#define	IRQ_PMU_D		159

#endif /* _ARM_ARM_IMX6INTR_H_ */
