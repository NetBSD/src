/*	$NetBSD: imx7_intr.h,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2015 Internet Initiative Japan, Inc.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_IMX_IMX7_INTR_H_
#define _ARM_IMX_IMX7_INTR_H_

#define	PIC_MAXSOURCES			256
#define	PIC_MAXMAXSOURCES		(256 + 7 * 32)
#define	__HAVE_PIC_PENDING_INTRS	/* for imxgpio */

#include <arm/cortex/gic_intr.h>

#define	IRQ_GPR			32
#define	IRQ_DAP			33
#define	IRQ_SDMA		34
#define	IRQ_DBGMON		35
#define	IRQ_SNVS_WRAPPER	36
#define	IRQ_LCDIF		37
#define	IRQ_SIM2		38
#define	IRQ_CSI			39
#define	IRQ_PXP1		40
#define	IRQ_RESERVED9		41
#define	IRQ_WDOG3		42
#define	IRQ_HS1			43
#define	IRQ_APBHDMA		44
#define	IRQ_EIM			45
#define	IRQ_BCH			46
#define	IRQ_GPMI		47
#define	IRQ_UART6		48
#define	IRQ_FTM1		49
#define	IRQ_FTM2		50
#define	IRQ_SNVS_HP_WRAPPER1	51
#define	IRQ_SNVS_HP_WRAPPER2	52
#define	IRQ_CSU			53
#define	IRQ_USDHC1		54
#define	IRQ_USDHC2		55
#define	IRQ_USDHC3		56
#define	IRQ_MIPI_CSI MIPI	57
#define	IRQ_UART1		58
#define	IRQ_UART2		59
#define	IRQ_UART3		60
#define	IRQ_UART4		61
#define	IRQ_UART5		62
#define	IRQ_eCSPI1		63
#define	IRQ_eCSPI2		64
#define	IRQ_eCSPI3		65
#define	IRQ_eCSPI4		66
#define	IRQ_I2C1		67
#define	IRQ_I2C2		68
#define	IRQ_I2C3		69
#define	IRQ_I2C4		70
#define	IRQ_RDC			71
#define	IRQ_USB			72
#define	IRQ_MIPI_DSI		73
#define	IRQ_USB_OH3		74
#define	IRQ_USB_OH2		75
#define	IRQ_USB_OTG1		76
#define	IRQ_USB_OTG2		77
#define	IRQ_PXP2		78
#define	IRQ_SCTR1		79
#define	IRQ_SCTR2		80
#define	IRQ_ANALOG_TEMP		81
#define	IRQ_SAI3		82
#define	IRQ_ANALOG_BROWNOUT	83
#define	IRQ_GPT4		84
#define	IRQ_GPT3		85
#define	IRQ_GPT2		86
#define	IRQ_GPT1		87
#define	IRQ_GPIO1_INT7		88
#define	IRQ_GPIO1_INT6		89
#define	IRQ_GPIO1_INT5		90
#define	IRQ_GPIO1_INT4		91
#define	IRQ_GPIO1_INT3		92
#define	IRQ_GPIO1_INT2		93
#define	IRQ_GPIO1_INT1		94
#define	IRQ_GPIO1_INT0		95
#define	IRQ_GPIO1L		96
#define	IRQ_GPIO1H		97
#define	IRQ_GPIO2L		98
#define	IRQ_GPIO2H		99
#define	IRQ_GPIO3L		100
#define	IRQ_GPIO3H		101
#define	IRQ_GPIO4L		102
#define	IRQ_GPIO4H		103
#define	IRQ_GPIO5L		104
#define	IRQ_GPIO5H		105
#define	IRQ_GPIO6L		106
#define	IRQ_GPIO6H		107
#define	IRQ_GPIO7L		108
#define	IRQ_GPIO7H		109
#define	IRQ_WDOG1		110
#define	IRQ_WDOG2		111
#define	IRQ_KPP			112
#define	IRQ_PWM1		113
#define	IRQ_PWM2		114
#define	IRQ_PWM3		115
#define	IRQ_PWM4		116
#define	IRQ_CCM1		117
#define	IRQ_CCM2		118
#define	IRQ_GPC			119
#define	IRQ_MU_A7		120
#define	IRQ_SRC			121
#define	IRQ_SIM1		122
#define	IRQ_RTIC		123
#define	IRQ_CPU_PMUIRQ0		124
#define	IRQ_CPU_PMUIRQ1		124
#define	IRQ_CPU_NCTIIRQ0	125
#define	IRQ_CPU_NCTIIRQ1	125
#define	IRQ_CCM_SRC_GPC		126
#define	IRQ_SAI1		127
#define	IRQ_SAI2		128
#define	IRQ_MU_M4		129
#define	IRQ_ADC1		130
#define	IRQ_ADC2		131
#define	IRQ_ENET2_TXRX1		132
#define	IRQ_ENET2_TXRX2		133
#define	IRQ_ENET2_MAC		134
#define	IRQ_ENET2_1588		135
#define	IRQ_TPR			136
#define	IRQ_CAAM_WRAPPER1	137
#define	IRQ_CAAM_WRAPPER2	138
#define	IRQ_QSPI		139
#define	IRQ_TZASC1		140
#define	IRQ_WDOG4		141
#define	IRQ_FLEXCAN1		142
#define	IRQ_FLEXCAN2		143
#define	IRQ_PERFMON1		144
#define	IRQ_PERFMON2		145
#define	IRQ_CAAM_WRAPPER3	146
#define	IRQ_CAAM_WRAPPER_ERR	147
#define	IRQ_HS2			148
#define	IRQ_EPDC		149
#define	IRQ_ENET1_TXTX1		150
#define	IRQ_ENET1_TXTX2		151
#define	IRQ_ENET1_MAC		152
#define	IRQ_ENET1_1588		153
#define	IRQ_PCIE_CTRL1		154
#define	IRQ_PCIE_CTRL2		155
#define	IRQ_PCIE_CTRL3		156
#define	IRQ_PCIE_CTRL4		157
#define	IRQ_UART7		158
#define	IRQ_PCIE_CTRL_CH	159

#endif /* _ARM_IMX_IMX7INTR_H_ */
