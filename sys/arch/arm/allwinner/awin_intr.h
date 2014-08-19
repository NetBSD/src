/* $NetBSD: awin_intr.h,v 1.3.12.2 2014/08/20 00:02:44 tls Exp $ */
/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#ifndef _ARM_ALLWINNER_AWIN_INTR_H_
#define _ARM_ALLWINNER_AWIN_INTR_H_ 

#define	PIC_MAXSOURCES			160
#define	PIC_MAXMAXSOURCES		192

/*
 * The Allwinner can use a generic interrupt controller so pull in that stuff.
 */
#include <arm/cortex/gic_intr.h>
#include <arm/cortex/gtmr_intr.h>	/* A7/A9/A15 Timer PPIs */

/*
 * There are for the A20 but the A10 are the same but offset by 32 less.
 */
#define AWIN_IRQ_UART0		33
#define AWIN_IRQ_UART1		34
#define AWIN_IRQ_UART2		35
#define AWIN_IRQ_UART3		36
#define AWIN_IRQ_IR0		37
#define AWIN_IRQ_IR1		38
#define AWIN_IRQ_TWI0		39
#define AWIN_IRQ_TWI1		40
#define AWIN_IRQ_TWI2		41
#define AWIN_IRQ_SPI0		42
#define AWIN_IRQ_SPI1		43
#define AWIN_IRQ_SPI2		44
#define AWIN_IRQ_SPDIF		45
#define AWIN_IRQ_AC97		46
#define AWIN_IRQ_TS		47
#define AWIN_IRQ_IIS0		48
#define AWIN_IRQ_UART4		49
#define AWIN_IRQ_UART5		50
#define AWIN_IRQ_UART6		51
#define AWIN_IRQ_UART7		52
#define AWIN_IRQ_KEYPAD		53
#define AWIN_IRQ_TMR0		54
#define AWIN_IRQ_TMR1		55
#define AWIN_IRQ_TMR2		56	/* WatchDog */
#define AWIN_IRQ_TMR3		57
#define AWIN_IRQ_CAN		58
#define AWIN_IRQ_DMA		59
#define AWIN_IRQ_PIO		60
#define AWIN_IRQ_TP		61
#define AWIN_IRQ_ADDC		62
#define AWIN_IRQ_LRADC		63
#define AWIN_IRQ_SDMMC0		64
#define AWIN_IRQ_SDMMC1		65
#define AWIN_IRQ_SDMMC2		66
#define AWIN_IRQ_SDMMC3		67
#define AWIN_IRQ_MS		68
#define AWIN_IRQ_NAND		69
#define AWIN_IRQ_USB0		70
#define AWIN_IRQ_USB1		71	// EHCI0
#define AWIN_IRQ_USB2		72	// EHCI1
#define AWIN_IRQ_SCR		73
#define AWIN_IRQ_CSI0		74
#define AWIN_IRQ_CSI1		75
#define AWIN_IRQ_LCD0		76
#define AWIN_IRQ_LCD1		77
#define AWIN_IRQ_MP		78
#define AWIN_IRQ_DE_XE0		79
#define AWIN_IRQ_DE_XE1		80
#define AWIN_IRQ_PMU		81
#define AWIN_IRQ_SPI3		82
#define AWIN_IRQ_TZASC		83
#define AWIN_IRQ_PATA		84
#define AWIN_IRQ_VE		85
#define AWIN_IRQ_SS		86
#define AWIN_IRQ_EMAC		87
#define AWIN_IRQ_SATA		88
#define AWIN_IRQ__RSVD89	89
#define AWIN_IRQ_HDMI0		90
#define AWIN_IRQ_TVE		91
#define AWIN_IRQ_ACE		92
#define AWIN_IRQ_TVD		93
#define AWIN_IRQ_PS2_0		94
#define AWIN_IRQ_PS2_1		95
#define AWIN_IRQ_USB3		96	// OHCI0
#define AWIN_IRQ_USB4		97	// OHCI1
#define AWIN_IRQ_PERFM		98
#define AWIN_IRQ_TMR4		99
#define AWIN_IRQ_TMR5		100
#define AWIN_IRQ_GPU_GP		101
#define AWIN_IRQ_GPU_GPMMU	102
#define AWIN_IRQ_GPU_PP0	103
#define AWIN_IRQ_GPU_PPMMU0	104
#define AWIN_IRQ_GPU_PMU	105
#define AWIN_IRQ_GPU_PP1	106
#define AWIN_IRQ_GPU_PPMMU1	107
#define AWIN_IRQ_GPU_RSV0	108
#define AWIN_IRQ_GPU_RSV1	109
#define AWIN_IRQ_GPU_RSV2	110
#define AWIN_IRQ_GPU_RSV3	111
#define AWIN_IRQ_GPU_RSV4	112
#define AWIN_IRQ_HSTMR0		113
#define AWIN_IRQ_HSTMR1		114
#define AWIN_IRQ_HSTMR2		115
#define AWIN_IRQ_HSTMR3		116
#define AWIN_IRQ_GMAC		117
#define AWIN_IRQ_HDMI1		118
#define AWIN_IRQ_IIS1		119
#define AWIN_IRQ_TWI3		120
#define AWIN_IRQ_TWI4		121
#define AWIN_IRQ_IIS2		122

#endif /* _ARM_ALLWINNER_AWIN_INTR_H_ */
