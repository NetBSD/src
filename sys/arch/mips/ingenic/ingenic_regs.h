/*	$NetBSD: ingenic_regs.h,v 1.2 2014/12/06 14:33:34 macallan Exp $ */

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
	#define TCSR_EXT_EN	0x04	/* EXTCLK - 48MHz */
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
	#define TESR_TCST5	0x0020	/* enable counter 5 */ 
	#define TESR_TCST6	0x0040	/* enable counter 6 */ 
	#define TESR_TCST7	0x0080	/* enable counter 7 */ 
	#define TESR_OST	0x8000	/* enable OST */ 
#define JZ_TC_TECR	0x10002018	/* TC enable clear reg. */
#define JZ_TC_TFR	0x10002020
	#define TFR_FFLAG0	0x00000001	/* channel 0 */
	#define TFR_FFLAG1	0x00000002	/* channel 1 */
	#define TFR_FFLAG2	0x00000004	/* channel 2 */
	#define TFR_FFLAG3	0x00000008	/* channel 3 */
	#define TFR_FFLAG4	0x00000010	/* channel 4 */
	#define TFR_FFLAG5	0x00000020	/* channel 5 */
	#define TFR_FFLAG6	0x00000040	/* channel 6 */
	#define TFR_FFLAG7	0x00000080	/* channel 7 */
	#define TFR_OSTFLAG	0x00008000	/* OS timer */
#define JZ_TC_TFSR	0x10002024	/* timer flag set */
#define JZ_TC_TFCR	0x10002028	/* timer flag clear */
#define JZ_TC_TMR	0x10002030	/* timer flag mask */
#define JZ_TC_TMSR	0x10002034	/* timer flag mask set */
#define JZ_TC_TMCR	0x10002038	/* timer flag mask clear*/

#define JZ_TC_TDFR(n)	(0x10002040 + (n * 0x10))	/* FULL compare */
#define JZ_TC_TDHR(n)	(0x10002044 + (n * 0x10))	/* HALF compare */
#define JZ_TC_TCNT(n)	(0x10002048 + (n * 0x10))	/* count */

#define JZ_TC_TCSR(n)	(0x1000204c + (n * 0x10))
/* same bits as in JZ_WDOG_TCSR	*/

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

/* extra CP0 registers */
static inline uint32_t
MFC0(uint32_t r, uint32_t s)
{
	uint32_t ret = 0x12345678;

	__asm volatile("mfc0 %0, $%1, %2; nop;" : "=r"(ret) : "i"(r), "i"(s));
	return ret;
}

#define MTC0(v, r, s) __asm volatile("mtc0 %0, $%1, %2; nop;" :: "r"(v), "i"(r), "i"(s))

#define CP0_CORE_CTRL	12	/* select 2 */
	#define CC_SW_RST0	1	/* reset core 0 */
	#define CC_SW_RST1	2	/* reset core 1 */
	#define CC_RPC0		0x100	/* dedicater reset entry core 0 */
	#define CC_RPC1		0x200	/* -- || -- core 1 */
	#define CC_SLEEP0M	0x10000	/* mask sleep core 0 */
	#define CC_SLEEP1M	0x20000	/* mask sleep core 1 */

/* cores status, 12 select 3 */
#define CS_MIRQ0_P	0x00001	/* mailbox IRQ for 0 pending */
#define CS_MIRQ1_P	0x00002	/* || core 1 */
#define CS_IRQ0_P	0x00100	/* peripheral IRQ for core 0 */
#define CS_IRQ1_P	0x00200	/* || core 1 */
#define CS_SLEEP0	0x10000	/* core 0 sleeping */
#define CS_SLEEP1	0x20000	/* core 1 sleeping */

/* cores reset entry & IRQ masks - 12 select 4 */
#define REIM_MIRQ0_M	0x00001	/* allow mailbox IRQ for core 0 */
#define REIM_MIRQ1_M	0x00002	/* allow mailbox IRQ for core 1 */
#define REIM_IRQ0_M	0x00100	/* allow peripheral IRQ for core 0 */
#define REIM_IRQ1_M	0x00200	/* allow peripheral IRQ for core 1 */
#define REIM_ENTRY_M	0xffff0000	/* reset exception entry if RPCn=1 */

#define CP0_CORE_MBOX	20	/* select 0 for core 0, 1 for 1 */

/* interrupt controller */
#define JZ_ICSR0	0x10001000	/* raw IRQ line status */
#define JZ_ICMR0	0x10001004	/* IRQ mask, 1 masks IRQ */
#define JZ_ICMSR0	0x10001008	/* sets bits in mask register */
#define JZ_ICMCR0	0x1000100c	/* clears bits in maks register */
#define JZ_ICPR0	0x10001010	/* line status after masking */

#define JZ_ICSR1	0x10001020	/* raw IRQ line status */
#define JZ_ICMR1	0x10001024	/* IRQ mask, 1 masks IRQ */
#define JZ_ICMSR1	0x10001028	/* sets bits in mask register */
#define JZ_ICMCR1	0x1000102c	/* clears bits in maks register */
#define JZ_ICPR1	0x10001030	/* line status after masking */

#define JZ_DSR0		0x10001034	/* source for PDMA */
#define JZ_DMR0		0x10001038	/* mask for PDMA */
#define JZ_DPR0		0x1000103c	/* pending for PDMA */

#define JZ_DSR1		0x10001040	/* source for PDMA */
#define JZ_DMR1		0x10001044	/* mask for PDMA */
#define JZ_DPR1		0x10001048	/* pending for PDMA */

#endif /* INGENIC_REGS_H */
