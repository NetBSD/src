/* $Id: imx23_icollreg.h,v 1.2.6.2 2013/02/25 00:28:27 tls Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#ifndef _ARM_IMX_IMX23_ICOLLREG_H_
#define _ARM_IMX_IMX23_ICOLLREG_H_

#include <sys/cdefs.h>

#define HW_ICOLL_BASE 0x80000000
#define HW_ICOLL_SIZE 0x2000

/*
 * IRQ numbers known to i.MX23.
 */
#define IRQ_DEBUG_UART		0 /* Non DMA on the debug UART */
#define IRQ_COMMS_RX		1 /* JTAG debug communications port */
#define IRQ_COMMS_TX		1
#define IRQ_SSP2_ERROR		2 /* SSP2 device-level error and status */
#define IRQ_VDD5V		3 /* IRQ on 5V connect or disconnect. Shared
				   * with DCDC status, Linear Regulator status,
				   * PSWITCH, and Host 4.2V */
#define IRQ_HEADPHONE_SHORT	4 /* HEADPHONE_SHORT */
#define IRQ_DAC_DMA		5 /* DAC DMA channel */
#define IRQ_DAC_ERROR		6 /* DAC FIFO buffer underflow */
#define IRQ_ADC_DMA		7 /* ADC DMA channel */
#define IRQ_ADC_ERROR		8 /* ADC FIFO buffer overflow */
#define IRQ_SPDIF_DMA		9 /* SPDIF DMA channel, SAIF2 DMA channel */
#define IRQ_SAIF2_DMA		9
#define IRQ_SPDIF_ERROR		10 /* SPDIF, SAIF1, SAIF2 FIFO
				    * underflow/overflow */
#define IRQ_SAIF1_IRQ		10
#define IRQ_SAIF2_IRQ		10
#define IRQ_USB_CTRL		11 /* USB controller */
#define IRQ_USB_WAKEUP		12 /* USB wakeup. Also ARC core to remain
				    * suspended. */
#define IRQ_GPMI_DMA		13 /* From DMA channel for GPMI */
#define IRQ_SSP1_DMA		14 /* From DMA channel for SSP1 */
#define IRQ_SSP_ERROR		15 /* SSP1 device-level error and status */
#define IRQ_GPIO0		16 /* GPIO bank 0 interrupt */
#define IRQ_GPIO1		17 /* GPIO bank 1 interrupt */
#define IRQ_GPIO2		18 /* GPIO bank 2 interrupt */
#define IRQ_SAIF1_DMA		19 /* SAIF1 DMA channel */
#define IRQ_SSP2_DMA		20 /* From DMA channel for SSP2 */
#define IRQ_ECC8_IRQ		21 /* ECC8 completion interrupt */
#define IRQ_RTC_ALARM		22 /* RTC alarm event */
#define IRQ_UARTAPP_TX_DMA	23 /* Application UART1 transmitter DMA */
#define IRQ_UARTAPP_INTERNAL	24 /* Application UART1 internal error */
#define IRQ_UARTAPP_RX_DMA	25 /* Application UART1 receiver DMA */
#define IRQ_I2C_DMA		26 /* From DMA channel for I2C */
#define IRQ_I2C_ERROR		27 /* From I2C device detected errors and line
				    * conditions. */
#define IRQ_TIMER0		28 /* TIMROT Timer0, recommend to set as FIQ. */
#define IRQ_TIMER1		29 /* TIMROT Timer1, recommend to set as FIQ. */
#define IRQ_TIMER2		30 /* TIMROT Timer2, recommend to set as FIQ. */
#define IRQ_TIMER3		31 /* TIMROT Timer3, recommend to set as FIQ. */
#define IRQ_BATT_BRNOUT		32 /* Power module battery brownout detect,
				    * recommend to set as FIQ. */
#define IRQ_VDDD_BRNOUT		33 /* Power module VDDD brownout detect,
				    * recommend to set as FIQ. */
#define IRQ_VDDIO_BRNOUT	34 /* Power module VDDIO brownout detect,
				    * recommend to set as FIQ. */
#define IRQ_VDD18_BRNOUT	35 /* Power module VDD18 brownout detect,
				    * recommend to set as FIQ. */
#define IRQ_TOUCH_DETECT	36 /* Touch detection. */
#define IRQ_LRADC_CH0		37 /* Channel 0 complete. */
#define IRQ_LRADC_CH1		38 /* Channel 1 complete. */
#define IRQ_LRADC_CH2		39 /* Channel 2 complete. */
#define IRQ_LRADC_CH3		40 /* Channel 3 complete. */
#define IRQ_LRADC_CH4		41 /* Channel 4 complete. */
#define IRQ_LRADC_CH5		42 /* Channel 5 complete. */
#define IRQ_LRADC_CH6		43 /* Channel 6 complete. */
#define IRQ_LRADC_CH7		44 /* Channel 7 complete. */
#define IRQ_LCDIF_DMA		45 /* From DMA channel for LCDIF. */
#define IRQ_LCDIF_ERROR		46 /* LCDIF error. */
#define IRQ_DIGCTL_DEBUG_TRAP	47 /* AHB arbiter debug trap. */
#define IRQ_RTC_1MSEC		48 /* RTC 1 ms tick interrupt. */
#define IRQ_RSVD49		49 /* Reserved */
#define IRQ_RSVD50		50 /* Reserved */
#define IRQ_GPMI		51 /* From GPMI internal error and status IRQ.*/
#define IRQ_RSVD52		52 /* Reserved */
#define IRQ_DCP_VMI		53 /* DCP Channel 0 virtual memory page copy. */
#define IRQ_DCP			54 /* DCP */
#define IRQ_RSVD55		55 /* Reserved. */
#define IRQ_BCH			56 /* BCH consolidated Interrupt */
#define IRQ_PXP			57 /* Pixel Pipeline consolidated Interrupt */
#define IRQ_UARTAPP2_TX_DMA	58 /* Application UART2 transmitter DMA */
#define IRQ_UARTAPP2_INTERNAL	59 /* Application UART2 internal error */
#define IRQ_UARTAPP2_RX_DMA	60 /* Application UART2 receiver DMA */
#define IRQ_VDAC_DETECT		61 /* Video dac, jack presence auto-detect */
#define IRQ_RSVD62		62 /* Reserved. */
#define IRQ_RSVD63		63 /* Reserved. */
#define IRQ_VDD5V_DROOP		64 /* 5V Droop, recommend to be set as FIQ. */
#define IRQ_DCDC4P2_BO		65 /* 4.2V regulated supply brown-out,
				    * recommend to be set as FIQ. */

#define IRQ_LAST		IRQ_DCDC4P2_BO

		/* IRQ's 66-127 are reserved. */

#define IRQ_MAX			127 /* Number or IRQ registers on i.MX23. */


/*
 * Interrupt Collector Interrupt Vector Address Register.
 */
#define HW_ICOLL_VECTOR		0x000
#define HW_ICOLL_VECTOR_SET	0x004
#define HW_ICOLL_VECTOR_CLR	0x008
#define HW_ICOLL_VECTOR_TOG	0x00C

#define HW_ICOLL_VECTOR_IRQVECTOR	__BITS(32, 2)
#define HW_ICOLL_VECTOR_RSRVD1		__BITS(1, 0)

/*
 * Interrupt Collector Level Acknowledge Register.
 */
#define HW_ICOLL_LEVELACK	0x010

#define HW_ICOLL_LEVELACK_RSRVD1	__BITS(31, 4)
#define HW_ICOLL_LEVELACK_IRQLEVELACK	__BIT(3, 0)

/*
 * Interrupt Collector Control Register.
 */
#define HW_ICOLL_CTRL		0x020
#define HW_ICOLL_CTRL_SET	0x024
#define HW_ICOLL_CTRL_CLR	0x028
#define HW_ICOLL_CTRL_TOG	0x02C

#define HW_ICOLL_CTRL_SFTRST		__BIT(31)
#define HW_ICOLL_CTRL_CLKGATE		__BIT(30)
#define HW_ICOLL_CTRL_RSRVD3		__BITS(29, 24)
#define HW_ICOLL_CTRL_VECTOR_PITCH	__BITS(23, 21)
#define HW_ICOLL_CTRL_BYPASS_FSM	__BIT(20)
#define HW_ICOLL_CTRL_NO_NESTING	__BIT(19)
#define HW_ICOLL_CTRL_ARM_RSE_MODE	__BIT(18)
#define HW_ICOLL_CTRL_FIQ_FINAL_ENABLE	__BIT(17)
#define HW_ICOLL_CTRL_IRQ_FINAL_ENABLE	__BIT(16)
#define HW_ICOLL_CTRL_RSRVD1		__BITS(15, 0)

/*
 * Interrupt Collector Interrupt Vector Base Address Register.
 */
#define HW_ICOLL_VBASE		0x040
#define HW_ICOLL_VBASE_SET	0x044
#define HW_ICOLL_VBASE_CLR	0x048
#define HW_ICOLL_VBASE_TOG	0x04C

#define HW_ICOLL_VBASE_TABLE_ADDRESS	__BITS(32, 2)
#define HW_ICOLL_VBASE_RSRVD1		__BITS(1, 0)

/*
 * Interrupt Collector Status Register.
 */
#define HW_ICOLL_STAT	0x070

#define HW_ICOLL_STAT_RSRVD1		__BITS(31, 7)
#define HW_ICOLL_STAT_VECTOR_NUMBER	__BITS(6, 0)

/*
 * Interrupt Collector Raw Interrupt Input Register.
 */
#define HW_ICOLL_RAW0		0x0A0
#define HW_ICOLL_RAW0_SET	0x0A4
#define HW_ICOLL_RAW0_CLR	0x0A8
#define HW_ICOLL_RAW0_TOG	0x0AC

#define HW_ICOLL_RAW0_RAW_IRQS	__BITS(31, 0)

/*
 * Interrupt Collector Raw Interrupt Input Register 1.
 */
#define HW_ICOLL_RAW1		0x0B0
#define HW_ICOLL_RAW1_SET	0x0B4
#define HW_ICOLL_RAW1_CLR	0x0B8
#define HW_ICOLL_RAW1_TOG	0x0BC

#define HW_ICOLL_RAW1_RAW_IRQS	__BITS(31, 0)

/*
 * Interrupt Collector Raw Interrupt Input Register 2.
 */
#define HW_ICOLL_RAW2		0x0C0
#define HW_ICOLL_RAW2_SET	0x0C4
#define HW_ICOLL_RAW2_CLR	0x0C8
#define HW_ICOLL_RAW2_TOG	0x0CC

#define HW_ICOLL_RAW2_RAW_IRQS	__BITS(31, 0)

/*
 * Interrupt Collector Raw Interrupt Input Register 3.
 */
#define HW_ICOLL_RAW3		0x0D0
#define HW_ICOLL_RAW3_SET	0x0D4
#define HW_ICOLL_RAW3_CLR	0x0D8
#define HW_ICOLL_RAW3_TOG	0x0DC

#define HW_ICOLL_RAW3_RAW_IRQS	__BITS(31, 0)

/*
 * Interrupt Collector Interrupt common registers.
 */
#define HW_ICOLL_INTERRUPT_RSRVD1	__BITS(31, 5)
#define HW_ICOLL_INTERRUPT_ENFIQ	__BIT(4)
#define HW_ICOLL_INTERRUPT_SOFTIRQ	__BIT(3)
#define HW_ICOLL_INTERRUPT_ENABLE	__BIT(2)
#define HW_ICOLL_INTERRUPT_PRIORITY	__BITS(1, 0)

/*
 * Interrupt Collector Interrupt Register 0.
 */
#define HW_ICOLL_INTERRUPT0	0x120
#define HW_ICOLL_INTERRUPT0_SET	0x124
#define HW_ICOLL_INTERRUPT0_CLR	0x128
#define HW_ICOLL_INTERRUPT0_TOG	0x12C

#define HW_ICOLL_INTERRUPT0_RSRVD1	__BITS(31, 5)
#define HW_ICOLL_INTERRUPT0_ENFIQ	__BIT(4)
#define HW_ICOLL_INTERRUPT0_SOFTIRQ	__BIT(3)
#define HW_ICOLL_INTERRUPT0_ENABLE	__BIT(2)
#define HW_ICOLL_INTERRUPT0_PRIORITY	__BITS(1, 0)

/*
 * Interrupt Collector Interrupt Register 127.
 */
#define HW_ICOLL_INTERRUPT127		0x910
#define HW_ICOLL_INTERRUPT127_SET	0x914
#define HW_ICOLL_INTERRUPT127_CLR	0x918
#define HW_ICOLL_INTERRUPT127_TOG	0x91C

#define HW_ICOLL_INTERRUPT127_RSRVD1	__BITS(31, 5)
#define HW_ICOLL_INTERRUPT127_ENFIQ	__BIT(4)
#define HW_ICOLL_INTERRUPT127_SOFTIRQ	__BIT(3)
#define HW_ICOLL_INTERRUPT127_ENABLE	__BIT(2)
#define HW_ICOLL_INTERRUPT127_PRIORITY	__BITS(1, 0)

/*
 * Interrupt Collector Debug Register 0.
 */
#define HW_ICOLL_DEBUG		0x1120
#define HW_ICOLL_DEBUG_SET	0x1124
#define HW_ICOLL_DEBUG_CLR	0x1128
#define HW_ICOLL_DEBUG_TOG	0x112C

#define HW_ICOLL_DEBUG_INSERVICE		__BITS(31, 28)
#define HW_ICOLL_DEBUG_LEVEL_REQUESTS		__BITS(27, 24)
#define HW_ICOLL_DEBUG_REQUESTS_BY_LEVEL	__BITS(23, 20)
#define HW_ICOLL_DEBUG_RSRVD2			__BITS(19, 18)
#define HW_ICOLL_DEBUG_FIQ			__BIT(17)
#define HW_ICOLL_DEBUG_IRQ			__BIT(16)
#define HW_ICOLL_DEBUG_RSRVD1			__BITS(15, 10)
#define HW_ICOLL_DEBUG_VECTOR_FSM		__BITS(9, 0)

/*
 * Interrupt Collector Debug Read Register 0.
 */
#define HW_ICOLL_DBGREAD0	0x1130
#define HW_ICOLL_DBGREAD0_SET	0x1134
#define HW_ICOLL_DBGREAD0_CLR	0x1138
#define HW_ICOLL_DBGREAD0_TOG	0x113C

#define HW_ICOLL_DBGREAD0_VALUE	__BITS(31, 0)

/*
 * Interrupt Collector Debug Read Register 1.
 */
#define HW_ICOLL_DBGREAD1	0x1140
#define HW_ICOLL_DBGREAD1_SET	0x1144
#define HW_ICOLL_DBGREAD1_CLR	0x1148
#define HW_ICOLL_DBGREAD1_TOG	0x114C

#define HW_ICOLL_DBGREAD1_VALUE	__BITS(31, 0)

/*
 * Interrupt Collector Debug Flag Register.
 */
#define HW_ICOLL_DBGFLAG	0x1150
#define HW_ICOLL_DBGFLAG_SET	0x1154
#define HW_ICOLL_DBGFLAG_CLR	0x1158
#define HW_ICOLL_DBGFLAG_TOG	0x115C

#define HW_ICOLL_DBGFLAG_RSRVD1	__BITS(31, 16)
#define HW_ICOLL_DBGFLAG_FLAG	__BITS(15, 0)

/*
 * Interrupt Collector Debug Read Request Register 0.
 */
#define HW_ICOLL_DBGREQUEST0		0x1160
#define HW_ICOLL_DBGREQUEST0_SET	0x1164
#define HW_ICOLL_DBGREQUEST0_CLR	0x1168
#define HW_ICOLL_DBGREQUEST0_TOG	0x116C

#define HW_ICOLL_DBGREQUEST0_BITS	__BITS(31, 0)

/*
 * Interrupt Collector Debug Read Request Register 1.
 */
#define HW_ICOLL_DBGREQUEST1		0x1170
#define HW_ICOLL_DBGREQUEST1_SET	0x1174
#define HW_ICOLL_DBGREQUEST1_CLR	0x1178
#define HW_ICOLL_DBGREQUEST1_TOG	0x117C

#define HW_ICOLL_DBGREQUEST1_BITS	__BITS(31, 0)

/*
 * Interrupt Collector Debug Read Request Register 2.
 */
#define HW_ICOLL_DBGREQUEST2		0x1180
#define HW_ICOLL_DBGREQUEST2_SET	0x1184
#define HW_ICOLL_DBGREQUEST2_CLR	0x1188
#define HW_ICOLL_DBGREQUEST2_TOG	0x118C

#define HW_ICOLL_DBGREQUEST2_BITS	__BITS(31, 0)

/*
 * Interrupt Collector Debug Read Request Register 3.
 */
#define HW_ICOLL_DBGREQUEST3		0x1190
#define HW_ICOLL_DBGREQUEST3_SET	0x1194
#define HW_ICOLL_DBGREQUEST3_CLR	0x1198
#define HW_ICOLL_DBGREQUEST3_TOG	0x119C

#define HW_ICOLL_DBGREQUEST3_BITS	__BITS(31, 0)

/*
 * Interrupt Collector Version Register.
 */
#define HW_ICOLL_VERSION	0x11E0

#define HW_ICOLL_VERSION_MAJOR	__BITS(31, 24)
#define HW_ICOLL_VERSION_MINOR	__BITS(23, 16)
#define HW_ICOLL_VERSION_STEP	__BITS(15, 0)

#endif /* !_ARM_IMX_IMX23_ICOLLREG_H_ */
