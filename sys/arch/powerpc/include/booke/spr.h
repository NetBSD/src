/*	$NetBSD: spr.h,v 1.11.2.1 2017/12/03 11:36:37 jdolecek Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#ifndef _POWERPC_BOOKE_SPR_H_
#define	_POWERPC_BOOKE_SPR_H_

#define PVR_MPCe500		  0x8020
#define PVR_MPCe500v2		  0x8021
#define PVR_MPCe500mc		  0x8023
#define PVR_MPCe5500		  0x8024	/* 64-bit */

#define	SVR_MPC8548v1		  0x80310010
#define	SVR_MPC8548v1plus	  0x80310011
#define	SVR_MPC8548v2		  0x80310020
#define	SVR_MPC8547v2		  0x80310120
#define	SVR_MPC8545v2		  0x80310220
#define	SVR_MPC8543v1		  0x80320010
#define	SVR_MPC8543v1plus	  0x80320011
#define	SVR_MPC8543v2		  0x80320020

#define	SVR_MPC8544v1		  0x80340110
#define	SVR_MPC8544v1plus	  0x80340111
#define	SVR_MPC8533		  0x80340010

#define	SVR_MPC8536v1		  0x80370091

#define	SVR_MPC8555v1		  0x80710110
#define	SVR_MPC8541v1		  0x80720111

#define	SVR_MPC8567v1		  0x80750111
#define	SVR_MPC8568v1		  0x80750011

#define	SVR_MPC8572v1		  0x80e00011

#define	SVR_P2020v2		  0x80e20020
#define	SVR_P2010v2		  0x80e30020
#define	SVR_P1011v2		  0x80e50020
#define	SVR_P1012v2		  0x80e50120
#define	SVR_P1013v2		  0x80e70020
#define	SVR_P1015v1		  0x80e50210
#define	SVR_P1016v1		  0x80e50310
#define	SVR_P1017v1		  0x80f70011
#define	SVR_P1020v2		  0x80e40020
#define	SVR_P1021v2		  0x80e40120
#define	SVR_P1022v2		  0x80e60020
#define	SVR_P1023v1		  0x80f60011
#define	SVR_P1024v2		  0x80e40210
#define	SVR_P1025v1		  0x80e40310

#define	SVR_SECURITY_P(svr)	  (((svr) & 0x00080000) != 0)

#define	SVR_P2040v1		  0x82100010	/* e500mc */
#define	SVR_P2041v1		  0x82100110	/* e500mc */

#define	SVR_P3041v1		  0x82110310	/* e500mc */

#define	SVR_P4080v1		  0x82000010	/* e500mc */
#define	SVR_P4040v1		  0x82000110	/* e500mc */

#define	SVR_P5010v1		  0x82210010	/* e5500 */
#define	SVR_P5020v1		  0x82200010	/* e5500 */

/*
 * Special Purpose Register declarations.
 *
 * The first column in the comments indicates which PowerPC architectures the
 * SPR is valid on - E for BookE series, 4 for 4xx series,
 * 6 for 6xx/7xx series and 8 for 8xx and 8xxx (but not 85xx) series.
 */

#define	SPR_PID0		48	/* E4.. 440 Process ID */
#define	SPR_DECAR		54	/* E... Decrementer Auto-reload */
#define	SPR_CSRR0		58	/* E... Critical Save/Restore Reg. 0 */
#define	SPR_CSRR1		59	/* E... Critical Save/Restore Reg. 1 */
#define	SPR_DEAR		61	/* E... Data Exception Address Reg. */
#define	SPR_ESR			62	/* E... Exception Syndrome Register */
#define	  ESR_PIL		  0x08000000 /* 4: Program ILlegal */
#define	  ESR_PPR		  0x04000000 /* 5: Program PRivileged */
#define	  ESR_PTR		  0x02000000 /* 6: Program TRap */
#define	  ESR_ST		  0x00800000 /* 8: Store operation */
#define	  ESR_DLK		  0x00200000 /* 10: dcache exception */
#define	  ESR_ILK		  0x00100000 /* 11: icache exception */
#define	  ESR_AP		  0x00080000 /* 12: Auxiliary Processor operation exception */
#define	  ESR_PUO		  0x00040000 /* 13: Program Unimplemented Operation exception */
#define	  ESR_BO		  0x00020000 /* 14: Byte ordering exception */
#define	  ESR_PIE		  0x00020000 /* 14: Program Imprecise Exception */
#define	  ESR_SPV		  0x00000080 /* 24: SPE exception */
#define	  ESR_VLEMI		  0x00000020 /* 26: VLE exception */
#define	  ESR_MIF		  0x00000002 /* 30: VLE Misaligned Instruction Fetch */
#define	  ESR_XTE		  0x00000001 /* 31: eXternal Transaction Error */
#define	SPR_IVPR		63	/* E... Interrupt Vector Prefix Reg. */
#define	SPR_USPRG0		256	/* E4.. User SPR General 0 */
#define	SPR_USPRG3		259	/* E... User SPR General 3 */
#define	SPR_USPRG4		260	/* E... User SPR General 4 */
#define	SPR_USPRG5		261	/* E... User SPR General 5 */
#define	SPR_USPRG6		262	/* E... User SPR General 6 */
#define	SPR_USPRG7		263	/* E... User SPR General 7 */
#define	SPR_RTBL		268	/* E468 Time Base Lower (RO) */
#define	SPR_RTBU		269	/* E468 Time Base Upper (RO) */
#define	SPR_WTBL		284	/* E468 Time Base Lower (WO) */
#define	SPR_WTBU		285	/* E468 Time Base Upper (WO) */
#define	SPR_PIR			286	/* E... Processor ID Register (RO) */

#define	SPR_DBSR		304	/* E... Debug Status Register (W1C) */
#define   DBSR_IDE		  0x80000000 /* 0: Imprecise debug event */
#define	  DBSR_UDE		  0x40000000 /* 1: Unconditional debug event */
#define	  DBSR_MRR_HARD		  0x20000000 /* 2: Most Recent Reset (Hard) */
#define	  DBSR_MRR_SOFT		  0x10000000 /* 3: Most Recent Reset (Soft) */
#define	  DBSR_ICMP		  0x08000000 /* 4: Instruction completion debug event */
#define	  DBSR_BRT		  0x04000000 /* 5: Branch Taken debug event */
#define	  DBSR_IRPT		  0x02000000 /* 6: Interrupt Taken debug event */
#define	  DBSR_TRAP		  0x01000000 /* 7: Trap Instruction debug event */
#define   DBSR_IAC		  0x00f00000 /* 8-11: IAC debug event */
#define	  DBSR_IAC1		  0x00800000 /* 8: IAC1 debug event */
#define	  DBSR_IAC2		  0x00400000 /* 9: IAC2 debug event */
#define	  DBSR_IAC3		  0x00200000 /* 10: IAC3 debug event */
#define	  DBSR_IAC4		  0x00100000 /* 11: IAC4 debug event */
#define   DBSR_DAC		  0x000f0000 /* 12-15: DAC debug event */
#define	  DBSR_DAC1R		  0x00080000 /* 12: DAC1 Read debug event */
#define	  DBSR_DAC1W		  0x00040000 /* 13: DAC1 Write debug event */
#define	  DBSR_DAC2R		  0x00020000 /* 14: DAC2 Read debug event */
#define	  DBSR_DAC2W		  0x00010000 /* 15: DAC2 Write debug event */
#define	  DBSR_RET		  0x00008000 /* 16: Return debug event */
#define	SPR_DBCR0		308	/* E... Debug Control Register 0 */
#define	  DBCR0_EDM		  0x80000000 /* 0: External Debug Mode */
#define	  DBCR0_IDM		  0x40000000 /* 1: Internal Debug Mode */
#define	  DBCR0_RST_MASK	  0x30000000 /* 2..3: ReSeT */
#define	  DBCR0_RST_NONE	  0x00000000 /*   No action */
#define	  DBCR0_RST_CORE	  0x10000000 /*   Core reset */
#define	  DBCR0_RST_CHIP	  0x20000000 /*   Chip reset */
#define	  DBCR0_RST_SYSTEM	  0x30000000 /*   System reset */
#define	  DBCR0_ICMP		  0x08000000 /* 4: Instruction Completion debug event */
#define	  DBCR0_BRT		  0x04000000 /* 5: Branch Taken debug event */
#define	  DBCR0_IRPT		  0x02000000 /* 6: Interrupt Taken debug event */
#define	  DBCR0_TRAP		  0x01000000 /* 7: Trap Instruction Debug Event */
#define	  DBCR0_IAC1		  0x00800000 /* 8: IAC (Instruction Address Compare) 1 debug event */
#define	  DBCR0_IAC2		  0x00400000 /* 9: IAC 2 debug event */
#define	  DBCR0_IAC3		  0x00200000 /* 10: IAC 3 debug event */
#define	  DBCR0_IAC4		  0x00100000 /* 11: IAC 4 debug event */
#define	  DBCR0_DAC1_LOAD	  0x00080000 /* 12: DAC (Data Address Compare) 1 load event */
#define	  DBCR0_DAC1_STORE	  0x00040000 /* 13: DAC (Data Address Compare) 1 store event */
#define	  DBCR0_DAC2_LOAD	  0x00020000 /* 14: DAC 2 load event */
#define	  DBCR0_DAC2_STORE	  0x00010000 /* 15: DAC 2 store event */
#define	  DBCR0_RET		  0x00008000 /* 16: Return debug event */
#define	  DBCR0_FT		  0x00000001 /* 31: Freeze Timers on debug event */
#define	SPR_DBCR1		309	/* E... Debug Control Register 1 */
#define	  DBCR1_IAC1US		  0xc0000000 /*  0-1: Data Address Compare 1 user/supervisor mode */
#define	  DBCR1_IAC1US_ANY	  0x00000000 /*  MSR[PR] = don't care */
#define	  DBCR1_IAC1US_KERNEL	  0x80000000 /*  MSR[PR] = 0 */
#define	  DBCR1_IAC1US_USER	  0xc0000000 /*  MSR[PR] = 1 */
#define	  DBCR1_IAC1ER		  0x30000000 /*  2-3: Data Address Compare 1 effective/real mode */
#define	  DBCR1_IAC1ER_DSX	  0x00000000 /*  effective address */
#define	  DBCR1_IAC1ER_REAL	  0x10000000 /*  real address */
#define	  DBCR1_IAC1ER_DS0	  0x20000000 /*  effective address MSR[DS] = 0 */
#define	  DBCR1_IAC1ER_DS1	  0x30000000 /*  effective address MSR[DS] = 1 */
#define	  DBCR1_IAC2US		  0x0c000000 /*  4-5: Data Address Compare 1 user/supervisor mode */
#define	  DBCR1_IAC2US_ANY	  0x00000000 /*  MSR[PR] = don't care */
#define	  DBCR1_IAC2US_KERNEL	  0x08000000 /*  MSR[PR] = 0 */
#define	  DBCR1_IAC2US_USER	  0x0c000000 /*  MSR[PR] = 1 */
#define	  DBCR1_IAC2ER		  0x03000000 /*  6-7: Data Address Compare 1 effective/real mode */
#define	  DBCR1_IAC2ER_DSX	  0x00000000 /*  effective address */
#define	  DBCR1_IAC2ER_REAL	  0x01000000 /*  real address */
#define	  DBCR1_IAC2ER_DS0	  0x02000000 /*  effective address MSR[DS] = 0 */
#define	  DBCR1_IAC2ER_DS1	  0x03000000 /*  effective address MSR[DS] = 1 */
#define	  DBCR1_IAC12M		  0x00c00000 /*  8-9: Data Address Compare 1 effective/real mode */
#define	  DBCR1_IAC12M_EXACT	  0x00000000 /*  equal IAC1 or IAC2 */
#define	  DBCR1_IAC12M_MASK	  0x00400000 /*  (addr & IAC2) == (IAC1 & IAC2) */
#define	  DBCR1_IAC12M_INCLUSIVE  0x00800000 /*  IAC1 <= addr < IAC2 */
#define	  DBCR1_IAC12M_EXCLUSIVE  0x00c00000 /*  addr < IAC1 || IAC2 <= addr */
#define	  DBCR1_IAC3US		  0x0000c000 /*  16-17: Data Address Compare 3 user/supervisor mode */
#define	  DBCR1_IAC3US_ANY	  0x00000000 /*  MSR[PR] = don't care */
#define	  DBCR1_IAC3US_KERNEL	  0x00008000 /*  MSR[PR] = 0 */
#define	  DBCR1_IAC3US_USER	  0x0000c000 /*  MSR[PR] = 1 */
#define	  DBCR1_IAC3ER		  0x00003000 /*  18-19: Data Address Compare 3 effective/real mode */
#define	  DBCR1_IAC3ER_DSX	  0x00000000 /*  effective address */
#define	  DBCR1_IAC3ER_REAL	  0x00001000 /*  real address */
#define	  DBCR1_IAC3ER_DS0	  0x00002000 /*  effective address MSR[DS] = 0 */
#define	  DBCR1_IAC3ER_DS1	  0x00003000 /*  effective address MSR[DS] = 1 */
#define	  DBCR1_IAC4US		  0x00000c00 /*  20-21: Data Address Compare 3 user/supervisor mode */
#define	  DBCR1_IAC4US_ANY	  0x00000000 /*  MSR[PR] = don't care */
#define	  DBCR1_IAC4US_KERNEL	  0x00000800 /*  MSR[PR] = 0 */
#define	  DBCR1_IAC4US_USER	  0x00000c00 /*  MSR[PR] = 1 */
#define	  DBCR1_IAC4ER		  0x00000300 /*  22-23: Data Address Compare 4 effective/real mode */
#define	  DBCR1_IAC4ER_DSX	  0x00000000 /*  effective address */
#define	  DBCR1_IAC4ER_REAL	  0x00000100 /*  real address */
#define	  DBCR1_IAC4ER_DS0	  0x00000200 /*  effective address MSR[DS] = 0 */
#define	  DBCR1_IAC4ER_DS1	  0x00000300 /*  effective address MSR[DS] = 1 */
#define	  DBCR1_IAC34M		  0x000000c0 /*  24-25: Data Address Compare 4 effective/real mode */
#define	  DBCR1_IAC34M_EXACT	  0x00000000 /*  equal IAC3 or IAC4 */
#define	  DBCR1_IAC34M_MASK	  0x00000040 /*  (addr & IAC4) == (IAC3 & IAC4) */
#define	  DBCR1_IAC34M_INCLUSIVE  0x00000080 /*  IAC3 <= addr < IAC4 */
#define	  DBCR1_IAC34M_EXCLUSIVE  0x000000c0 /*  addr < IAC3 || IAC4 <= addr */
#define	SPR_DBCR2		310	/* E... Debug Control Register 2 */
#define	  DBCR2_DAC1US		  0xc0000000 /*  0-1: Data Address Compare 1 user/supervisor mode */
#define	  DBCR2_DAC1US_ANY	  0x00000000 /*  MSR[PR] = don't care */
#define	  DBCR2_DAC1US_KERNEL	  0x80000000 /*  MSR[PR] = 0 */
#define	  DBCR2_DAC1US_USER	  0xc0000000 /*  MSR[PR] = 1 */
#define	  DBCR2_DAC1ER		  0x30000000 /*  2-3: Data Address Compare 1 effective/real mode */
#define	  DBCR2_DAC1ER_DSX	  0x00000000 /*  effective address */
#define	  DBCR2_DAC1ER_REAL	  0x10000000 /*  real address */
#define	  DBCR2_DAC1ER_DS0	  0x20000000 /*  effective address MSR[DS] = 0 */
#define	  DBCR2_DAC1ER_DS1	  0x30000000 /*  effective address MSR[DS] = 1 */
#define	  DBCR2_DAC2US		  0x0c000000 /*  4-5: Data Address Compare 1 user/supervisor mode */
#define	  DBCR2_DAC2US_ANY	  0x00000000 /*  MSR[PR] = don't care */
#define	  DBCR2_DAC2US_KERNEL	  0x08000000 /*  MSR[PR] = 0 */
#define	  DBCR2_DAC2US_USER	  0x0c000000 /*  MSR[PR] = 1 */
#define	  DBCR2_DAC2ER		  0x03000000 /*  6-7: Data Address Compare 1 effective/real mode */
#define	  DBCR2_DAC2ER_DSX	  0x00000000 /*  effective address */
#define	  DBCR2_DAC2ER_REAL	  0x01000000 /*  real address */
#define	  DBCR2_DAC2ER_DS0	  0x02000000 /*  effective address MSR[DS] = 0 */
#define	  DBCR2_DAC2ER_DS1	  0x03000000 /*  effective address MSR[DS] = 1 */
#define	  DBCR2_DAC12M		  0x00c00000 /*  8-9: Data Address Compare 1 effective/real mode */
#define	  DBCR2_DAC12M_EXACT	  0x00000000 /*  equal DAC1 or DAC2 */
#define	  DBCR2_DAC12M_MASK	  0x00400000 /*  (addr & DAC2) == (DAC1 & DAC2) */
#define	  DBCR2_DAC12M_INCLUSIVE  0x00800000 /*  DAC1 <= addr < DAC2 */
#define	  DBCR2_DAC12M_EXCLUSIVE  0x00c00000 /*  addr < DAC1 || DAC2 <= addr */
#define	  DBCR2_DAC3US		  0x0000c000 /*  16-17: Data Address Compare 3 user/supervisor mode */
#define	  DBCR2_DAC3US_ANY	  0x00000000 /*  MSR[PR] = don't care */
#define	  DBCR2_DAC3US_KERNEL	  0x00008000 /*  MSR[PR] = 0 */
#define	  DBCR2_DAC3US_USER	  0x0000c000 /*  MSR[PR] = 1 */
#define	  DBCR2_DAC3ER		  0x00003000 /*  18-19: Data Address Compare 3 effective/real mode */
#define	  DBCR2_DAC3ER_DSX	  0x00000000 /*  effective address */
#define	  DBCR2_DAC3ER_REAL	  0x00001000 /*  real address */
#define	  DBCR2_DAC3ER_DS0	  0x00002000 /*  effective address MSR[DS] = 0 */
#define	  DBCR2_DAC3ER_DS1	  0x00003000 /*  effective address MSR[DS] = 1 */
#define	  DBCR2_DAC4US		  0x00000c00 /*  20-21: Data Address Compare 3 user/supervisor mode */
#define	  DBCR2_DAC4US_ANY	  0x00000000 /*  MSR[PR] = don't care */
#define	  DBCR2_DAC4US_KERNEL	  0x00000800 /*  MSR[PR] = 0 */
#define	  DBCR2_DAC4US_USER	  0x00000c00 /*  MSR[PR] = 1 */
#define	  DBCR2_DAC4ER		  0x00000300 /*  22-23: Data Address Compare 4 effective/real mode */
#define	  DBCR2_DAC4ER_DSX	  0x00000000 /*  effective address */
#define	  DBCR2_DAC4ER_REAL	  0x00000100 /*  real address */
#define	  DBCR2_DAC4ER_DS0	  0x00000200 /*  effective address MSR[DS] = 0 */
#define	  DBCR2_DAC4ER_DS1	  0x00000300 /*  effective address MSR[DS] = 1 */
#define	  DBCR2_DAC34M		  0x000000c0 /*  24-25: Data Address Compare 4 effective/real mode */
#define	  DBCR2_DAC34M_EXACT	  0x00000000 /*  equal DAC3 or DAC4 */
#define	  DBCR2_DAC34M_MASK	  0x00000040 /*  (addr & DAC4) == (DAC3 & DAC4) */
#define	  DBCR2_DAC34M_INCLUSIVE  0x00000080 /*  DAC3 <= addr < DAC4 */
#define	  DBCR2_DAC34M_EXCLUSIVE  0x000000c0 /*  addr < DAC3 || DAC4 <= addr */
#define	SPR_IAC1		312	/* E... Instruction Address Compare 1 */
#define	SPR_IAC2		313	/* E... Instruction Address Compare 2 */
#define	SPR_IAC3		314	/* E... Instruction Address Compare 3 */
#define	SPR_IAC4		315	/* E... Instruction Address Compare 4 */
#define	SPR_DAC1		316	/* E... Data Address Compare 1 */
#define	SPR_DAC2		317	/* E... Data Address Compare 2 */
#define	SPR_TSR			336	/* E... Timer Status Register */
#define   TSR_ENW		  0x80000000 /* Enable Next Watchdog (W1C) */
#define   TSR_WIS		  0x40000000 /* Watchdog Interrupt Status (W1C) */
#define   TSR_WRS		  0x30000000 /* Watchdog Reset Status (W1C) */
#define   TSR_DIS		  0x08000000 /* Decementer Interrupt Status (W1C) */
#define   TSR_FIS		  0x04000000 /* Fixed-interval Interrupt Status (W1C) */
#define	SPR_TCR			340	/* E... Timer Control Register */
#define   TCR_WP		  0xc0000000 /* Watchdog Period */
#define	  TCR_WP_2_N(n)		  (__SHIFTIN((n), TCR_WP) | __SHIFTIN((n) >> 2, TCR_WPEXT))
#define	  TCR_WP_2_64		  0x00000000
#define	  TCR_WP_2_1		  0xc01e0000
#define   TCR_WRC		  0x30000000 /* Watchdog Timer Reset Control */
#define   TCR_WRC_RESET		  0x20000000
#define   TCR_WIE		  0x08000000 /* Watchdog Time Interrupt Enable */
#define   TCR_DIE		  0x04000000 /* Decremnter Interrupt Enable */
#define   TCR_FP		  0x03000000 /* Fixed-interval Timer Period */
#define	  TCR_FP_2_N(n)		  ((((64 - (n)) & 0x30) << 20) | (((64 - (n)) & 0xf) << 13))
#define	  TCR_FP_2_64		  0x00000000
#define	  TCR_FP_2_1		  0x0301e000
#define   TCR_FIE		  0x00800000 /* Fixed-interval Interrupt Enable */
#define   TCR_ARE		  0x00400000 /* Auto-reload Enable */
#define   TCR_WPEXT		  0x001e0000 /* Watchdog Period Extension */
#define   TCR_FPEXT		  0x0001e000 /* Fixed-interval Period Extension */

#define	SPR_IVOR0		400	/* E... Critical input interrupt offset */
#define	SPR_IVOR1		401	/* E... Machine check interrupt offset */
#define	SPR_IVOR2		402	/* E... Data storage interrupt offset */
#define	SPR_IVOR3		403	/* E... Instruction storage interrupt offset */
#define	SPR_IVOR4		404	/* E... External input interrupt offset */
#define	SPR_IVOR5		405	/* E... Alignment interrupt offset */
#define	SPR_IVOR6		406	/* E... Program interrupt offset */
#define	SPR_IVOR8		408	/* E... Syscall call interrupt offset */
#define	SPR_IVOR10		410	/* E... Decrementer interrupt offset */
#define	SPR_IVOR11		411	/* E... Fixed-interval timer interrupt offset */
#define	SPR_IVOR12		412	/* E... Watchdog timer interrupt offset */
#define	SPR_IVOR13		413	/* E... Data TLB error interrupt offset */
#define	SPR_IVOR14		414	/* E... Instruction TLB error interrupt offset */
#define	SPR_IVOR15		415	/* E... Debug interrupt offset */
#define SPR_SPEFSCR		512	/* E... Signal processing and embedded floating-point status and control register */
#define  SPEFSCR_SOVH		  0x80000000 /* 0: Summary Integer Overflow High */
#define  SPEFSCR_OVH		  0x40000000 /* 1: Integer Overflow High */
#define  SPEFSCR_FGH		  0x20000000 /* 2: Embedded Floating-Point Guard Bit High */
#define  SPEFSCR_FXH		  0x10000000 /* 3: Embedded Floating-Point Sticky Bit High */
#define  SPEFSCR_FINVH		  0x08000000 /* 4: Embedded Floating-Point Invalid Operation High */
#define  SPEFSCR_FDBZH		  0x04000000 /* 5: Embedded Floating-Point Divide By Zero Error High */
#define  SPEFSCR_FUNFH		  0x02000000 /* 6: Embedded Floating-Point Underflow Error High */
#define  SPEFSCR_FOVFH		  0x01000000 /* 7: Embedded Floating-Point Overflow Error High */
#define  SPEFSCR_FINXS		  0x00200000 /* 10: Embedded Floating-Point Inexact Sticky Bit */
#define  SPEFSCR_FINVS		  0x00100000 /* 11: Embedded Floating-Point Invalid Operation Sticky Bit */
#define  SPEFSCR_FDBZS		  0x00080000 /* 12: Embedded Floating-Point Divide By Zero Sticky Bit */
#define  SPEFSCR_FUNFS		  0x00040000 /* 13: Embedded Floating-Point Underflow Sticky Bit */
#define  SPEFSCR_FOVFS		  0x00020000 /* 14: Embedded Floating-Point Overflow Sticky Bit */
#define  SPEFSCR_MODE		  0x00010000 /* 15: Embedded Floating-Point Mode */
#define  SPEFSCR_SOV		  0x80000000 /* 16: Summary Integer Overflow */
#define  SPEFSCR_OV		  0x00004000 /* 17: Integer Overflow */
#define  SPEFSCR_FG		  0x00002000 /* 18: Embedded Floating-Point Guard Bit */
#define  SPEFSCR_FX		  0x00001000 /* 19: Embedded Floating-Point Sticky Bit */
#define  SPEFSCR_FINV		  0x00000800 /* 20: Embedded Floating-Point Invalid Operation */
#define  SPEFSCR_FDBZ		  0x00000400 /* 21: Embedded Floating-Point Divide By Zero Error */
#define  SPEFSCR_FUNF		  0x00000200 /* 22: Embedded Floating-Point Underflow Error */
#define  SPEFSCR_FOVF		  0x00000100 /* 23: Embedded Floating-Point Overflow Error */
#define  SPEFSCR_FINXE		  0x00000040 /* 25: Embedded Floating-Point Inexact Execption Enable */
#define  SPEFSCR_FINVE		  0x00000020 /* 26: Embedded Floating-Point Invalid Operation/Input Error Execption Enable */
#define  SPEFSCR_FDBZE		  0x00000010 /* 27: Embedded Floating-Point Divide By Zero Exception Enable */
#define  SPEFSCR_FUNFE		  0x00000008 /* 28: Embedded Floating-Point Underflow Exception Enable */
#define  SPEFSCR_FOVFE		  0x00000004 /* 29: Embedded Floating-Point Overflow Exception Enable */
#define  SPEFSCR_FRMC_MASK	  0x00000003 /* 30..31: Embedded Floating-Point Rounding Mode Control */
#define  SPEFSCR_FRMC_DOWNWARD	  0x00000003 /* Round toward -infinity */
#define  SPEFSCR_FRMC_UPWARD	  0x00000002 /* Round toward +infinity */
#define  SPEFSCR_FRMC_TOWARDZERO  0x00000001 /* Round toward zero */
#define  SPEFSCR_FRMC_TONEAREST	  0x00000000 /* Round to nearest */
#define	SPR_BBEAR		513	/* E... Brach buffer entry addr register */
#define	SPR_BBTAR		514	/* E... Brach buffer target addr register */
#define	SPR_L1CFG0		515	/* E... L1 Cache Configuration Register 0 */
#define	SPR_L1CFG1		516	/* E... L1 Cache Configuration Register 1 */
#define   L1CFG_CARCH_GET(n)	  (((n) >> 30) & 3)
#define   L1CFG_CARCH_HARVARD	  0
#define   L1CFG_CARCH_UNIFIED	  1
#define   L1CFG_CBSIZE_GET(n)	  (((n) >> 23) & 3)
#define   L1CFG_CBSIZE_32B	  0
#define   L1CFG_CBSIZE_64B	  1
#define   L1CFG_CREPL_GET(n)	  (((n) >> 21) & 3)
#define   L1CFG_CREPL_TRUE_LRU	  0
#define   L1CFG_CREPL_PSEUDO_LRU  1
#define   L1CFG_CLA_P(n)	  (((n) >> 20) & 1)
#define   L1CFG_CPA_P(n)	  (((n) >> 19) & 1)
#define   L1CFG_CNWAY_GET(n)	  ((((n) >> 11) & 0xff) + 1)
#define   L1CFG_CSIZE_GET(n)	  ((((n) >>  0) & 0x7ff) << 10)
#define	SPR_ATBL		526	/* E... Alternate Time Base Lower */
#define	SPR_ATBU		527	/* E... Alternate Time Base Upper */
#define	SPR_IVOR32		528	/* E... SPE unavailable interrupt offset */
#define	SPR_IVOR33		529	/* E... Floating-point data exception interrupt offset */
#define	SPR_IVOR34		530	/* E... Floating-point round exception interrupt offset */
#define	SPR_IVOR35		531	/* E... Performance monitor interrupt offset */
#define SPR_MCARU		569	/* E... Machine check address register upper */
#define SPR_MCSRR0		570	/* E... Machine check save/restore register 0 */
#define SPR_MCSRR1		571	/* E... Machine check save/restore register 1 */
#define SPR_MCSR		572	/* E... Machine check syndrome register */
#define   MCSR_MCP		  0x80000000 /* 0: Machine Check Input Pin */
#define   MCSR_ICPERR		  0x40000000 /* 1: Instruction Cache Parity Error */
#define   MCSR_DCP_PERR		  0x20000000 /* 2: Data Cache Push Parity Error */
#define   MCSR_DCPERR		  0x10000000 /* 3: Data Cache Parity Error */
#define   MCSR_NMI		  0x00100000 /* 12: non maskable interrupt */
#define   MCSR_MAV		  0x00080000 /* 13: MCAR address valid */
#define   MCSR_MEA		  0x00040000 /* 14: MCAR [is an] effective address */
#define   MCSR_BUS_IAERR	  0x00000080 /* 24: Bus Instruction Address Error */
#define   MCSR_BUS_RAERR	  0x00000040 /* 25: Bus Read Address Error */
#define   MCSR_BUS_WAERR	  0x00000020 /* 26: Bus Write Address Error */
#define   MCSR_BUS_IBERR	  0x00000010 /* 27: Bus Instruction Data Bus Error */
#define   MCSR_BUS_RBERR	  0x00000008 /* 28: Bus Read Data Bus Error */
#define   MCSR_BUS_WBERR	  0x00000004 /* 29: Bus Write Data Bus Error */
#define   MCSR_BUS_IPERR	  0x00000002 /* 30: Bus Instruction Parity Error */
#define   MCSR_BUS_RPERR	  0x00000001 /* 31: Bus Read Parity Error */
#define SPR_MCAR		573	/* E... Machine check address register */
#define	SPR_MAS0		624	/* E... MAS Register 0 */
#define   MAS0_TLBSEL		  0x30000000 /* Select TLB<n> for access */
#define   MAS0_TLBSEL_TLB3	  0x30000000 /* Select TLB3 for access */
#define   MAS0_TLBSEL_TLB2	  0x20000000 /* Select TLB2 for access */
#define   MAS0_TLBSEL_TLB1	  0x10000000 /* Select TLB1 for access */
#define   MAS0_TLBSEL_TLB0	  0x00000000 /* Select TLB0 for access */
#define   MASX_TLBSEL_GET(n)	  (((n) >> 28) & 3)
#define   MASX_TLBSEL_MAKE(n)	  (((n) & 3) << 28)
#define   MAS0_ESEL		  0x0fff0000 /* entry (way) select for tlbwe */
#define   MAS0_ESEL_GET(n)	  (((n) >> 16) & 4095)
#define   MAS0_ESEL_MAKE(n)	  (((n) & 4095) << 16)
#define   MAS0_NV		  0x00000fff /* next victim fr TLB0[NV] */
#define	SPR_MAS1		625	/* E... MAS Register 1 */
#define	  MAS1_V		  0x80000000 /* TLB Valid Bit */
#define   MAS1_IPROT		  0x40000000 /* Invalidate Protect */
#define   MAS1_TID		  0x0fff0000 /* Translation Identity */
#define   MASX_TID_GET(n)	  (((n) >> 16) & 4095)
#define   MASX_TID_MAKE(n)	  (((n) & 4095) << 16)
#define   MAS1_TS		  0x00001000 /* Translation Space [IS/DS MSR] */
#define	  MAS1_TS_SHIFT		  12
#define   MAS1_TSIZE		  0x00000f00 /* Translation Size (4KB**tsize) */
#define   MASX_TSIZE_4KB	  0x00000100 /*   4KB TSIZE */
#define   MASX_TSIZE_16KB	  0x00000200 /*  16KB TSIZE */
#define   MASX_TSIZE_64KB	  0x00000300 /*  64KB TSIZE */
#define   MASX_TSIZE_256KB	  0x00000400 /* 256KB TSIZE */
#define   MASX_TSIZE_1MB	  0x00000500 /*   1MB TSIZE */
#define   MASX_TSIZE_4MB	  0x00000600 /*   4MB TSIZE */
#define   MASX_TSIZE_16MB	  0x00000700 /*  16MB TSIZE */
#define   MASX_TSIZE_64MB	  0x00000800 /*  64MB TSIZE */
#define   MASX_TSIZE_256MB	  0x00000900 /* 256MB TSIZE */
#define   MASX_TSIZE_1GB	  0x00000a00 /*   1GB TSIZE */
#define   MASX_TSIZE_4GB	  0x00000b00 /*   4GB TSIZE */
#define   MASX_TSIZE_GET(n)	  (((n) >> 8) & 15)
#define   MASX_TSIZE_MAKE(n)	  (((n) & 15) << 8)
#define	SPR_MAS2		626	/* E... MAS Register 2 */
#define   MAS2_EPN		  0xfffff000 /* Effective Page Number */
#define   MAS2_EPN_GET(n)	  (((n) >> 12) & 1048575)
#define   MAS2_EPN_MAKE(n)	  (((n) & 1048575) << 12)
#define	  MAS2_X0		  0x00000040 /* Impl. dependent page attr. */
#define	  MAS2_ACM		  0x000000c0 /* Alternate Coherency Mode. */
#define	  MAS2_X1		  0x00000020 /* Impl. dependent page attr. */
#define	  MAS2_VLE		  0x00000020 /* VLE mode. */
#define	  MAS2_WIMGE		  0x0000001f /* Mask of next 5 bits */
#define	  MAS2_W		  0x00000010 /* Write-through */
#define	  MAS2_I		  0x00000008 /* cache-Inhibited */
#define	  MAS2_M		  0x00000004 /* Memory coherency required */
#define	  MAS2_G		  0x00000002 /* Gaurded */
#define	  MAS2_E		  0x00000001 /* [little] Endianness */
#define	SPR_MAS3		627	/* E... MAS Register 3 */
#define   MAS3_RPN		  0xfffff000 /* Real Page Number */
#define   MAS3_RPN_GET(n)	  (((n) >> 12) & 1048575)
#define   MAS3_RPN_MAKE(n)	  (((n) & 1048575) << 12)
#define   MAS3_U0		  0x00000200 /* User attribute 0 */
#define   MAS3_U1		  0x00000100 /* User attribute 1 */
#define   MAS3_U2		  0x00000080 /* User attribute 2 */
#define   MAS3_U3		  0x00000040 /* User attribute 3 */
#define   MAS3_UX		  0x00000020 /* User execute permission */
#define   MAS3_SX		  0x00000010 /* System execute permission */
#define   MAS3_UW		  0x00000008 /* User write permission */
#define   MAS3_SW		  0x00000004 /* System write permission */
#define   MAS3_UR		  0x00000002 /* User read permission */
#define   MAS3_SR		  0x00000001 /* System read permission */
#define	SPR_MAS4		628	/* E... MAS Register 4 */
#define   MAS4_TLBSELD		  0x30000000 /* TLBSEL default value */
#define   MAS4_TLBSEL_TLB3	  0x30000000 /* Select TLB3 for access */
#define   MAS4_TLBSEL_TLB2	  0x20000000 /* Select TLB2 for access */
#define   MAS4_TLBSEL_TLB1	  0x10000000 /* Select TLB1 for access */
#define   MAS4_TLBSEL_TLB0	  0x00000000 /* Select TLB0 for access */
#define   MAS4_TIDSELD		  0x00030000 /* select TID default value */
#define   MAS4_TIDSELD_TIDZ	  0x00030000 /* fill in MAS1[TID] with 0 */
#define   MAS4_TIDSELD_PID2	  0x00020000 /* fill in MAS1[TAD] from ... */
#define   MAS4_TIDSELD_PID1	  0x00010000 /* fill in MAS1[TAD] from ... */
#define   MAS4_TIDSELD_PID0	  0x00000000 /* fill in MAS1[TAD] from ... */
#define   MAS4_TSIZED		  0x00000f00 /* TSIZE default value */
#define   MAS4_TSIZED_4KB	  0x00000100 /* 4KB TSIZE */
#define	  MAS4_ACMD		  0x000000c0 /* Alternate Coherency Mode. */
#define	  MAS4_X0D		  0x00000040 /* default Impl. dep. page attr. */
#define	  MAS4_VLED		  0x00000020 /* VLE mode. */
#define	  MAS4_X1D		  0x00000020 /* default Impl. dep. page attr. */
#define	  MAS4_WD		  0x00000010 /* default Write-through */
#define	  MAS4_ID		  0x00000008 /* default Cache-inhibited */
#define	  MAS4_MD		  0x00000004 /* default Memory coherency req. */
#define	  MAS4_GD		  0x00000002 /* default Gaurded */
#define	  MAS4_ED		  0x00000001 /* default [little] Endianness */
#define	SPR_MAS6		630	/* E... MAS Register 6 (TLB Seach CTX) */
#define   MAS6_SPID0		  0x0fff0000 /* PID used with tlbsx */
#define	  MAS6_SPID0_SHIFT	  16
#define   MAS6_SAS		  0x00000001 /* Address space (IS/DS MSR) ... */
#define   MAS6_SAS_USER		  0x00000001 /* Address space (IS/DS MSR) ... */
#define	SPR_PID1		633	/* E... PID Register 1 */
#define	SPR_PID2		634	/* E... PID Register 2 */
#define	SPR_TLB0CFG		688	/* E... TLB Configuration Register 0 */
#define	SPR_TLB1CFG		689	/* E... TLB Configuration Register 1 */
#define	  TLBCFG_ASSOC(n)	  (((n) >> 24) & 0xff) /* assoc of tlb */
#define	  TLBCFG_MINSIZE(n)	  (((n) >> 20) & 0x0f) /* minpagesize */
#define	  TLBCFG_MAXSIZE(n)	  (((n) >> 16) & 0x0f) /* maxpagesize */
#define	  TLBCFG_IPROT_P(n)	  (((n) >> 15) & 0x01)
#define	  TLBCFG_AVAIL_P(n)	  (((n) >> 14) & 0x01) /* variable page size */
#define	  TLBCFG_NENTRY(n)	  (((n) >>  0) & 0xfff) /* # entrys */
#define	SPR_MAS7		944	/* E... MAS Register 7 */
#define	 MAS7_RPNHI		  0x00000004 /* bits 32-35 of RPN */
#define	SPR_HID0		1008
#define   HID0_EMCP		  0x80000000 /* Enable Machine Check Pin */
#define   HID0_DOZE		  0x00800000 /* Core in doze mode */
#define   HID0_NAP		  0x00400000 /* Core in nap mode */
#define   HID0_SLEEP		  0x00200000 /* Core in sleep mode */
#define   HID0_TBEN		  0x00004000 /* Time Base ENable */
#define   HID0_SEL_TBCLK	  0x00002000 /* SELect Time Base Clock */
#define   HID0_EN_MAS7_UPDATE	  0x00000080 /* ENable MAS7 UPDATE */
#define   HID0_DCFA		  0x00000040 /* Data Cache Flush Assist */
#define   HID0_NOOPTI		  0x00000001 /* NO-OP Touch Instructions */
#define	SPR_HID1		1009
#define   HID1_ASTME		  0x00004000 /* Address Streaming Enable */
#define   HID1_ABE		  0x00001000 /* Address Broadcast Enable */
#define	SPR_L1CSR0		1010	/* E... L1 Cache Control and Status Register 0 (Data) */
#define	SPR_L1CSR1		1011	/* E... L1 Cache Control and Status Register 1 (Instruction) */
#define   L1CSR_CPE		  0x00010000 /* 15: Cache Parity Error */
#define   L1CSR_CPI		  0x00008000 /* 16: Cache Parity Injection Enable */
#define   L1CSR_CSLC		  0x00000800 /* 20: Cache Snoop Lock Clear */
#define   L1CSR_CUL		  0x00000400 /* 21: Cache Unable to Lock (W0C) */
#define   L1CSR_CLO		  0x00000200 /* 22: Cache Lock Overflow (W0C) */
#define   L1CSR_CLFR		  0x00000100 /* 23: Cache Lock Bits Flash Reset */
#define   L1CSR_CFI		  0x00000002 /* 30: Cache Flash Invalidate */
#define   L1CSR_CE		  0x00000001 /* 31: Cache Enable */
#define	SPR_MMUCSR0		1012	/* E... MMU Control and Status Register 0 */
#define	  MMUCSR0_TLB2_FI	  0x00000040 /* TLB2 Flash Invalidate */
#define	  MMUCSR0_TLB3_FI	  0x00000020 /* TLB3 Flash Invalidate */
#define	  MMUCSR0_TLB0_FI	  0x00000004 /* TLB0 Flash Invalidate */
#define	  MMUCSR0_TLB1_FI	  0x00000002 /* TLB1 Flash Invalidate */
#define SPR_BUCSR		1013	/* E... Branch Unit Control and Status Register */
#define	SPR_MMUCFG		1015	/* E... MMU Configuration Register */
#define   MMUCFG_RASIZE_GET(n)	  (((n) >> 17) & 127) /* Real Address Size */
#define	  MMUCFG_NPIDS_GET(n)	  (((n) >> 11) & 15) /* # of PID registers */
#define	  MMUCFG_PIDSIZE_GET(n)	  (((n) >> 6) & 31) /* PID is PIDSIZE+1 bits wide */
#define	  MMUCFG_NTLBS_GET(n)	  (((n) >> 2) & 3) /* NTLBS is max value of MAS0[TLBSEL] */
#define	  MMUCFG_MAVN		  0x00000003 /* MMU Architecture Version Number */
#define	  MMUCFG_MAVN_V1	  0
#define	  MMUCFG_MAVN_V2	  1
#define	SPR_SVR			1023	/* E... System Version Register */

#define	PMR_PMC0		16
#define	PMR_PMC1		17
#define	PMR_PMC2		18
#define	PMR_PMC3		19
#define	PMR_PMLCa0		144
#define	PMR_PMLCa1		145
#define	PMR_PMLCa2		146
#define	PMR_PMLCa3		147
#define   PMLCa_FC		  0x80000000 /* 0: Freeze Counter */
#define   PMLCa_FCS		  0x40000000 /* 1: Freeze Counter In Super */
#define   PMLCa_FCU		  0x20000000 /* 2: Freeze Counter In User */
#define   PMLCa_FCM1		  0x10000000 /* 3: Freeze Counter While Mark=1 */
#define   PMLCa_FCM0		  0x08000000 /* 4: Freeze Counter While Mark=0 */
#define   PMLCa_CE		  0x04000000 /* 5: Condition Enable */
#define   PMLCa_EVENT		  0x007f0000 /* 9..15: Event */
#define   PMLCa_EVENT_GET(n)	  (((n) >> 16) & 127)
#define   PMLCa_EVENT_MAKE(n)	  (((n) & 127) << 16)

#define	PMR_PMLCb0		272
#define	PMR_PMLCb1		273
#define	PMR_PMLCb2		274
#define	PMR_PMLCb3		275
#define   PMLCb_THRESHMUL	  0x00007f00 /* 21..23: multiply threshold by 2**<n> */
#define   PMLCb_THRESHMUL_GET(n)  (((n) >> 16) & 127)
#define   PMLCb_THRESHMUL_MAKE(n) (((n) & 127) << 16)
#define   PMLCb_THRESHOLD	  0x0000003f /* 26..31: threshold */
#define   PMLCb_THRESHOLD_GET(n)  (((n) >> 0) & 63)
#define   PMLCb_THRESHOLD_MAKE(n) (((n) & 63) << 0)
#define	PMR_PMGC0		400
#define   PMGC0_FAC		  0x80000000 /* 0: Freeze All Counters */
#define	  PMGC0_PMIE		  0x40000000 /* 1: Performance Monitor Interrupt Enable */
#define	  PMGC0_FCECE		  0x40000000 /* 1: Freeze count on enabled condition or event */
#define   PMGC0_TBSEL		  0x00001800 /* 19..20: Time base selector */
#define   PMGC0_TBEE		  0x00000100 /* 23: Time base transition event exception enable */

#define	PMR_UPMC0			(PMR_PMC0 - 16)
#define	PMR_UPMC1			(PMR_PMC1 - 16)
#define	PMR_UPMC2			(PMR_PMC2 - 16)
#define	PMR_UPMC3			(PMR_PMC3 - 16)
#define	PMR_UPMLCa0			(PMR_PMLCa0 - 16)
#define	PMR_UPMLCa1			(PMR_PMLCa1 - 16)
#define	PMR_UPMLCa2			(PMR_PMLCa2 - 16)
#define	PMR_UPMLCa3			(PMR_PMLCa3 - 16)
#define	PMR_UPMLCb0			(PMR_PMLCb0 - 16)
#define	PMR_UPMLCb1			(PMR_PMLCb1 - 16)
#define	PMR_UPMLCb2			(PMR_PMLCb2 - 16)
#define	PMR_UPMLCb3			(PMR_PMLCb3 - 16)
#define	PMR_UPMGC0			(PMR_PMGC0 - 16)

#endif /* !_POWERPC_BOOKE_SPR_H_ */
