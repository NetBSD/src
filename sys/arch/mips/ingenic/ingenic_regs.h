/*	$NetBSD: ingenic_regs.h,v 1.1 2014/11/22 15:17:01 macallan Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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

#include <mips/locore.h>

#ifndef INGENIC_REGS_H
#define INGENIC_REGS_H

/* UARTs, mostly 16550 compatible with 32bit spaced registers */
#define JZ_UART0 0x10030000
#define JZ_UART1 0x10031000
#define JZ_UART2 0x10032000
#define JZ_UART3 0x10033000
#define JZ_UART4 0x10034000

/* watchdog */
#define JZ_WDOG_TDR	0x10002000	/* compare */
#define JZ_WDOG_TCER	0x10002004
	#define TCER_ENABLE	0x01	/* enable counter */
#define JZ_WDOG_TCNT	0x10002008	/* 16bit up count */
#define JZ_WDOG_TCSR	0x1000200c
	#define TCSR_PCK_EN	0x01	/* PCLK */
	#define TCSR_RTC_EN	0x02	/* RTCCLK - 32.768kHz */
	#define TCSR_EXT_EN	0x04	/* EXTCLK - 12MHz? */
	#define TCSR_PRESCALE_M	0x38
	#define TCSR_DIV_1	0x00
	#define TCSR_DIV_4	0x08
	#define TCSR_DIV_16	0x10
	#define TCSR_DIV_64	0x18
	#define TCSR_DIV_256	0x20
	#define TCSR_DIV_1024	0x28

/* timers and PWMs */
#define JZ_TC_TER	0x10002010	/* TC enable reg, ro */
#define JZ_TC_TESR	0x10002014	/* TC enable set reg. */
	#define TESR_TCST0	0x0001	/* enable counter 0 */ 
	#define TESR_TCST1	0x0002	/* enable counter 1 */ 
	#define TESR_TCST2	0x0004	/* enable counter 2 */ 
	#define TESR_TCST3	0x0008	/* enable counter 3 */ 
	#define TESR_TCST4	0x0010	/* enable counter 4 */ 
	#define TESR_TCST5	0x0014	/* enable counter 5 */ 
	#define TESR_TCST6	0x0018	/* enable counter 6 */ 
	#define TESR_TCST7	0x001c	/* enable counter 7 */ 
	#define TESR_OST	0x8000	/* enable OST */ 
#define JZ_TC_TECR	0x10002018	/* TC enable clear reg. */

/* operating system timer */
#define JZ_OST_DATA	0x100020e0	/* compare */
#define JZ_OST_CNT_LO	0x100020e4
#define JZ_OST_CNT_HI	0x100020e8
#define JZ_OST_CTRL	0x100020ec
	#define OSTC_PCK_EN	0x0001	/* use PCLK */
	#define OSTC_RTC_EN	0x0002	/* use RTCCLK */
	#define OSTC_EXT_EN	0x0004	/* use EXTCLK */
	#define OSTC_PRESCALE_M	0x0038
	#define OSTC_DIV_1	0x0000
	#define OSTC_DIV_4	0x0008
	#define OSTC_DIV_16	0x0010
	#define OSTC_DIV_64	0x0018
	#define OSTC_DIV_256	0x0020
	#define OSTC_DIV_1024	0x0028
	#define OSTC_SHUTDOWN	0x0200
	#define OSTC_MODE	0x8000	/* 0 - reset to 0 when = OST_DATA */
#define JZ_OST_CNT_U32	0x100020fc	/* copy of CNT_HI when reading CNT_LO */

static inline void
writereg(uint32_t reg, uint32_t val)
{
	*(int32_t *)MIPS_PHYS_TO_KSEG1(reg) = val;
	wbflush();
}

static inline uint32_t
readreg(uint32_t reg)
{
	wbflush();
	return *(int32_t *)MIPS_PHYS_TO_KSEG1(reg);
}

#endif /* INGENIC_REGS_H */