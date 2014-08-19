/*	$NetBSD: timerreg.h,v 1.8.6.2 2014/08/20 00:03:18 tls Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

/*
 * 16bit timer 0:3
 *	source: BUSCLK, H-BLNK
 */

#define EE_TIMER_MIN		0
#define EE_TIMER_MAX		3
#define LEGAL_TIMER(x)							\
	(((x) >= EE_TIMER_MIN) && ((x) <= EE_TIMER_MAX))

/* Register address. all registers are 32bit wide */
#define TIMER_REGBASE		0x10000000
#define TIMER_REGSIZE		0x2000
#define TIMER_OFS		0x800

#define T_COUNT_REG(x)	MIPS_PHYS_TO_KSEG1((TIMER_REGBASE + TIMER_OFS * (x)))
#define T_MODE_REG(x)	MIPS_PHYS_TO_KSEG1((TIMER_REGBASE +		\
	TIMER_OFS * (x) + 0x10))
#define T_COMP_REG(x)	MIPS_PHYS_TO_KSEG1((TIMER_REGBASE +		\
	TIMER_OFS * (x) + 0x20))
/* 
 * timer0, timer1 have `hold register'. 
 * (save T_COUNT when SBUS interrupt occurred)
 */
#define T_HOLD_REG(x)	(TIMER_REGBASE + TIMER_OFS * (x) + 0x30)

#define T0_COUNT_REG		MIPS_PHYS_TO_KSEG1(0x10000000)
#define T0_MODE_REG		MIPS_PHYS_TO_KSEG1(0x10000010)
#define T0_COMP_REG		MIPS_PHYS_TO_KSEG1(0x10000020)
#define T0_HOLD_REG		MIPS_PHYS_TO_KSEG1(0x10000030)
#define T1_COUNT_REG		MIPS_PHYS_TO_KSEG1(0x10000800)
#define T1_MODE_REG		MIPS_PHYS_TO_KSEG1(0x10000810)
#define T1_COMP_REG		MIPS_PHYS_TO_KSEG1(0x10000820)
#define T1_HOLD_REG		MIPS_PHYS_TO_KSEG1(0x10000830)
#define T2_COUNT_REG		MIPS_PHYS_TO_KSEG1(0x10001000)
#define T2_MODE_REG		MIPS_PHYS_TO_KSEG1(0x10001010)
#define T2_COMP_REG		MIPS_PHYS_TO_KSEG1(0x10001020)
#define T3_COUNT_REG		MIPS_PHYS_TO_KSEG1(0x10001800)
#define T3_MODE_REG		MIPS_PHYS_TO_KSEG1(0x10001810)
#define T3_COMP_REG		MIPS_PHYS_TO_KSEG1(0x10001820)

/*
 * Tn_MODE: mode, status register.
 */
#define T_MODE_CLKS_MASK		0x3
#define T_MODE_CLKS(x)		((x) & T_MODE_CLKS_MASK)
#define T_MODE_CLKS_CLR(x)	((x) & ~T_MODE_CLKS_MASK)

#define T_MODE_CLKS_BUSCLK1		0	/* 150 MHz */
#define T_MODE_CLKS_BUSCLK16		1	/* 150 / 16 */
#define T_MODE_CLKS_BUSCLK256		2	/* 150 / 256 */
#define T_MODE_CLKS_HBLNK		3	/* H-Blank */

/* Gate Function Enabled */
#define T_MODE_GATE			0x00000004
/* Gate Selection */
#define T_MODE_GATS_VBLNK		0x00000008
/* Gate Mode */
#define T_MODE_GATM_MASK		0x3
#define T_MODE_GATM_SHIFT		4
#define T_MODE_GATM(x)		(((x) >> T_MODE_GATM_SHIFT) & T_MODE_GATM_MASK)
#define T_MODE_GATM_CLR(x)						\
	((x) & ~(T_MODE_GATM_MASK << T_MODE_GATM_SHIFT))
#define T_MODE_GATM_SET(x, val)						\
	((x) | (((val) << T_MODE_GATM_SHIFT) &				\
	(T_MODE_GATM_MASK << T_MODE_GATM_SHIFT)))
#define T_MODE_GATM_LOW			0x0
#define T_MODE_GATM_POSEDGE		0x1
#define T_MODE_GATM_NEGEDGE		0x2
#define T_MODE_GATM_EDGE		0x3

/* Zero Return */
#define T_MODE_ZRET			0x00000040
/* Count Up Enable */
#define T_MODE_CUE			0x00000080
/* Compare-Interrupt Enable */
#define T_MODE_CMPE			0x00000100
/* Overflow-Interrupt Enable */
#define T_MODE_OVFE			0x00000200
/* Equal Flag (write clear) */
#define T_MODE_EQUF			0x00000400
/* Overflow Flag (write clear) */
#define T_MODE_OVFF			0x00000800

/*
 * Tn_COUNT: counter register
 */
#define T_COUNT_MASK			0x0000ffff
#define T_COUNT(x)			((x) & T_COUNT_MASK)

/*
 * Tn_COMP: compare register
 */
#define T_COMP_MASK			0x0000ffff
#define T_COMP(x)			((x) & T_COMP_MASK)

/*
 * Tn_HOLD: hold register
 */
#define T_HOLD_MASK			0x0000ffff
#define T_HOLD(x)			((x) & T_HOLD_MASK)
