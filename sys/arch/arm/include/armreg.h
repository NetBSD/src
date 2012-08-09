/*	$NetBSD: armreg.h,v 1.48.8.1 2012/08/09 06:36:46 jdc Exp $	*/

/*
 * Copyright (c) 1998, 2001 Ben Harris
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_ARMREG_H
#define _ARM_ARMREG_H

/*
 * ARM Process Status Register
 *
 * The picture in the ARM manuals looks like this:
 *       3 3 2 2 2 2                            
 *       1 0 9 8 7 6                                   8 7 6 5 4       0
 *      +-+-+-+-+-+-------------------------------------+-+-+-+---------+
 *      |N|Z|C|V|Q|                reserved             |I|F|T|M M M M M|
 *      | | | | | |                                     | | | |4 3 2 1 0|
 *      +-+-+-+-+-+-------------------------------------+-+-+-+---------+
 */

#define	PSR_FLAGS 0xf0000000	/* flags */
#define PSR_N_bit (1 << 31)	/* negative */
#define PSR_Z_bit (1 << 30)	/* zero */
#define PSR_C_bit (1 << 29)	/* carry */
#define PSR_V_bit (1 << 28)	/* overflow */

#define PSR_Q_bit (1 << 27)	/* saturation */

#define I32_bit (1 << 7)	/* IRQ disable */
#define F32_bit (1 << 6)	/* FIQ disable */
#define	IF32_bits (3 << 6)	/* IRQ/FIQ disable */

#define PSR_T_bit (1 << 5)	/* Thumb state */
#define PSR_J_bit (1 << 24)	/* Java mode */

#define PSR_MODE	0x0000001f	/* mode mask */
#define PSR_USR26_MODE	0x00000000
#define PSR_FIQ26_MODE	0x00000001
#define PSR_IRQ26_MODE	0x00000002
#define PSR_SVC26_MODE	0x00000003
#define PSR_USR32_MODE	0x00000010
#define PSR_FIQ32_MODE	0x00000011
#define PSR_IRQ32_MODE	0x00000012
#define PSR_SVC32_MODE	0x00000013
#define PSR_ABT32_MODE	0x00000017
#define PSR_UND32_MODE	0x0000001b
#define PSR_SYS32_MODE	0x0000001f
#define PSR_32_MODE	0x00000010

#define PSR_IN_USR_MODE(psr)	(!((psr) & 3))		/* XXX */
#define PSR_IN_32_MODE(psr)	((psr) & PSR_32_MODE)

/* In 26-bit modes, the PSR is stuffed into R15 along with the PC. */

#define R15_MODE	0x00000003
#define R15_MODE_USR	0x00000000
#define R15_MODE_FIQ	0x00000001
#define R15_MODE_IRQ	0x00000002
#define R15_MODE_SVC	0x00000003

#define R15_PC		0x03fffffc

#define R15_FIQ_DISABLE	0x04000000
#define R15_IRQ_DISABLE	0x08000000

#define R15_FLAGS	0xf0000000
#define R15_FLAG_N	0x80000000
#define R15_FLAG_Z	0x40000000
#define R15_FLAG_C	0x20000000
#define R15_FLAG_V	0x10000000

/*
 * Co-processor 15:  The system control co-processor.
 */

#define ARM_CP15_CPU_ID		0

/*
 * The CPU ID register is theoretically structured, but the definitions of
 * the fields keep changing.
 */

/* The high-order byte is always the implementor */
#define CPU_ID_IMPLEMENTOR_MASK	0xff000000
#define CPU_ID_ARM_LTD		0x41000000 /* 'A' */
#define CPU_ID_DEC		0x44000000 /* 'D' */
#define CPU_ID_INTEL		0x69000000 /* 'i' */
#define	CPU_ID_TI		0x54000000 /* 'T' */
#define CPU_ID_MARVELL		0x56000000 /* 'V' */
#define	CPU_ID_FARADAY		0x66000000 /* 'f' */

/* How to decide what format the CPUID is in. */
#define CPU_ID_ISOLD(x)		(((x) & 0x0000f000) == 0x00000000)
#define CPU_ID_IS7(x)		(((x) & 0x0000f000) == 0x00007000)
#define CPU_ID_ISNEW(x)		(!CPU_ID_ISOLD(x) && !CPU_ID_IS7(x))

/* On ARM3 and ARM6, this byte holds the foundry ID. */
#define CPU_ID_FOUNDRY_MASK	0x00ff0000
#define CPU_ID_FOUNDRY_VLSI	0x00560000

/* On ARM7 it holds the architecture and variant (sub-model) */
#define CPU_ID_7ARCH_MASK	0x00800000
#define CPU_ID_7ARCH_V3		0x00000000
#define CPU_ID_7ARCH_V4T	0x00800000
#define CPU_ID_7VARIANT_MASK	0x007f0000

/* On more recent ARMs, it does the same, but in a different format */
#define CPU_ID_ARCH_MASK	0x000f0000
#define CPU_ID_ARCH_V3		0x00000000
#define CPU_ID_ARCH_V4		0x00010000
#define CPU_ID_ARCH_V4T		0x00020000
#define CPU_ID_ARCH_V5		0x00030000
#define CPU_ID_ARCH_V5T		0x00040000
#define CPU_ID_ARCH_V5TE	0x00050000
#define CPU_ID_ARCH_V5TEJ	0x00060000
#define CPU_ID_ARCH_V6		0x00070000
#define CPU_ID_VARIANT_MASK	0x00f00000

/* Next three nybbles are part number */
#define CPU_ID_PARTNO_MASK	0x0000fff0

/* Intel XScale has sub fields in part number */
#define CPU_ID_XSCALE_COREGEN_MASK	0x0000e000 /* core generation */
#define CPU_ID_XSCALE_COREREV_MASK	0x00001c00 /* core revision */
#define CPU_ID_XSCALE_PRODUCT_MASK	0x000003f0 /* product number */

/* And finally, the revision number. */
#define CPU_ID_REVISION_MASK	0x0000000f

/* Individual CPUs are probably best IDed by everything but the revision. */
#define CPU_ID_CPU_MASK		0xfffffff0

/* Fake CPU IDs for ARMs without CP15 */
#define CPU_ID_ARM2		0x41560200
#define CPU_ID_ARM250		0x41560250

/* Pre-ARM7 CPUs -- [15:12] == 0 */
#define CPU_ID_ARM3		0x41560300
#define CPU_ID_ARM600		0x41560600
#define CPU_ID_ARM610		0x41560610
#define CPU_ID_ARM620		0x41560620

/* ARM7 CPUs -- [15:12] == 7 */
#define CPU_ID_ARM700		0x41007000 /* XXX This is a guess. */
#define CPU_ID_ARM710		0x41007100
#define CPU_ID_ARM7500		0x41027100
#define CPU_ID_ARM710A		0x41047100 /* inc ARM7100 */
#define CPU_ID_ARM7500FE	0x41077100
#define CPU_ID_ARM710T		0x41807100
#define CPU_ID_ARM720T		0x41807200
#define CPU_ID_ARM740T8K	0x41807400 /* XXX no MMU, 8KB cache */
#define CPU_ID_ARM740T4K	0x41817400 /* XXX no MMU, 4KB cache */

/* Post-ARM7 CPUs */
#define CPU_ID_ARM810		0x41018100
#define CPU_ID_ARM920T		0x41129200
#define CPU_ID_ARM922T		0x41029220
#define CPU_ID_ARM926EJS	0x41069260
#define CPU_ID_ARM940T		0x41029400 /* XXX no MMU */
#define CPU_ID_ARM946ES		0x41049460 /* XXX no MMU */
#define	CPU_ID_ARM966ES		0x41049660 /* XXX no MMU */
#define	CPU_ID_ARM966ESR1	0x41059660 /* XXX no MMU */
#define CPU_ID_ARM1020E		0x4115a200 /* (AKA arm10 rev 1) */
#define CPU_ID_ARM1022ES	0x4105a220
#define CPU_ID_ARM1026EJS	0x4106a260
#define CPU_ID_ARM11MPCORE	0x410fb020
#define CPU_ID_ARM1136JS	0x4107b360
#define CPU_ID_ARM1136JSR1	0x4117b360
#define CPU_ID_ARM1176JZS	0x410fb760
#define CPU_ID_CORTEXA8R1	0x411fc080
#define CPU_ID_CORTEXA8R2	0x412fc080
#define CPU_ID_CORTEXA8R3	0x413fc080
#define CPU_ID_CORTEXA9R1	0x411fc090
#define CPU_ID_SA110		0x4401a100
#define CPU_ID_SA1100		0x4401a110
#define	CPU_ID_TI925T		0x54029250
#define CPU_ID_MV88FR571_VD	0x56155710
#define CPU_ID_MV88SV131	0x56251310
#define	CPU_ID_FA526		0x66015260
#define CPU_ID_SA1110		0x6901b110
#define CPU_ID_IXP1200		0x6901c120
#define CPU_ID_80200		0x69052000
#define CPU_ID_PXA250    	0x69052100 /* sans core revision */
#define CPU_ID_PXA210    	0x69052120
#define CPU_ID_PXA250A		0x69052100 /* 1st version Core */
#define CPU_ID_PXA210A		0x69052120 /* 1st version Core */
#define CPU_ID_PXA250B		0x69052900 /* 3rd version Core */
#define CPU_ID_PXA210B		0x69052920 /* 3rd version Core */
#define CPU_ID_PXA250C		0x69052d00 /* 4th version Core */
#define CPU_ID_PXA210C		0x69052d20 /* 4th version Core */
#define	CPU_ID_PXA27X		0x69054110
#define	CPU_ID_80321_400	0x69052420
#define	CPU_ID_80321_600	0x69052430
#define	CPU_ID_80321_400_B0	0x69052c20
#define	CPU_ID_80321_600_B0	0x69052c30
#define	CPU_ID_80219_400	0x69052e20
#define	CPU_ID_80219_600	0x69052e30
#define	CPU_ID_IXP425_533	0x690541c0
#define	CPU_ID_IXP425_400	0x690541d0
#define	CPU_ID_IXP425_266	0x690541f0

/* ARM3-specific coprocessor 15 registers */
#define ARM3_CP15_FLUSH		1
#define ARM3_CP15_CONTROL	2
#define ARM3_CP15_CACHEABLE	3
#define ARM3_CP15_UPDATEABLE	4
#define ARM3_CP15_DISRUPTIVE	5	

/* ARM3 Control register bits */
#define ARM3_CTL_CACHE_ON	0x00000001
#define ARM3_CTL_SHARED		0x00000002
#define ARM3_CTL_MONITOR	0x00000004

/*
 * Post-ARM3 CP15 registers:
 *
 *	1	Control register
 *
 *	2	Translation Table Base
 *
 *	3	Domain Access Control
 *
 *	4	Reserved
 *
 *	5	Fault Status
 *
 *	6	Fault Address
 *
 *	7	Cache/write-buffer Control
 *
 *	8	TLB Control
 *
 *	9	Cache Lockdown
 *
 *	10	TLB Lockdown
 *
 *	11	Reserved
 *
 *	12	Reserved
 *
 *	13	Process ID (for FCSE)
 *
 *	14	Reserved
 *
 *	15	Implementation Dependent
 */

/* Some of the definitions below need cleaning up for V3/V4 architectures */

/* CPU control register (CP15 register 1) */
#define CPU_CONTROL_MMU_ENABLE	0x00000001 /* M: MMU/Protection unit enable */
#define CPU_CONTROL_AFLT_ENABLE	0x00000002 /* A: Alignment fault enable */
#define CPU_CONTROL_DC_ENABLE	0x00000004 /* C: IDC/DC enable */
#define CPU_CONTROL_WBUF_ENABLE 0x00000008 /* W: Write buffer enable */
#define CPU_CONTROL_32BP_ENABLE 0x00000010 /* P: 32-bit exception handlers */
#define CPU_CONTROL_32BD_ENABLE 0x00000020 /* D: 32-bit addressing */
#define CPU_CONTROL_LABT_ENABLE 0x00000040 /* L: Late abort enable */
#define CPU_CONTROL_BEND_ENABLE 0x00000080 /* B: Big-endian mode */
#define CPU_CONTROL_SYST_ENABLE 0x00000100 /* S: System protection bit */
#define CPU_CONTROL_ROM_ENABLE	0x00000200 /* R: ROM protection bit */
#define CPU_CONTROL_CPCLK	0x00000400 /* F: Implementation defined */
#define CPU_CONTROL_BPRD_ENABLE 0x00000800 /* Z: Branch prediction enable */
#define CPU_CONTROL_IC_ENABLE   0x00001000 /* I: IC enable */
#define CPU_CONTROL_VECRELOC	0x00002000 /* V: Vector relocation */
#define CPU_CONTROL_ROUNDROBIN	0x00004000 /* RR: Predictable replacement */
#define CPU_CONTROL_V4COMPAT	0x00008000 /* L4: ARMv4 compat LDR R15 etc */
#define CPU_CONTROL_FI_ENABLE	0x00200000 /* FI: Low interrupt latency */
#define CPU_CONTROL_UNAL_ENABLE	0x00400000 /* U: unaligned data access */
#define CPU_CONTROL_XP_ENABLE	0x00800000 /* XP: extended page table */
#define	CPU_CONTROL_V_ENABLE	0x01000000 /* VE: Interrupt vectors enable */
#define	CPU_CONTROL_EX_BEND	0x02000000 /* EE: exception endianness */
#define	CPU_CONTROL_NMFI	0x08000000 /* NMFI: Non maskable FIQ */
#define	CPU_CONTROL_TR_ENABLE	0x10000000 /* TRE: */
#define	CPU_CONTROL_AF_ENABLE	0x20000000 /* AFE: Access flag enable */
#define	CPU_CONTROL_TE_ENABLE	0x40000000 /* TE: Thumb Exception enable */

#define CPU_CONTROL_IDC_ENABLE	CPU_CONTROL_DC_ENABLE

/* ARM11r0 Auxillary Control Register (CP15 register 1, opcode2 1) */
#define ARM11R0_AUXCTL_PFI	0x80000000 /* PFI: partial FI mode. */
					   /* This is an undocumented flag
					    * used to work around a cache bug
					    * in r0 steppings. See errata
					    * 364296.
					    */
/* ARM11x6 Auxiliary Control Register (CP15 register 1, opcode2 1) */
#define	ARM11X6_AUXCTL_RS	0x00000001 /* return stack */
#define	ARM11X6_AUXCTL_DB	0x00000002 /* dynamic branch prediction */
#define	ARM11X6_AUXCTL_SB	0x00000004 /* static branch prediction */
#define	ARM11X6_AUXCTL_TR	0x00000008 /* MicroTLB replacement strat. */
#define	ARM11X6_AUXCTL_EX	0x00000010 /* exclusive L1/L2 cache */
#define	ARM11X6_AUXCTL_RA	0x00000020 /* clean entire cache disable */
#define	ARM11X6_AUXCTL_RV	0x00000040 /* block transfer cache disable */
#define	ARM11X6_AUXCTL_CZ	0x00000080 /* restrict cache size */

/* ARM1176 Auxiliary Control Register (CP15 register 1, opcode2 1) */   
#define	ARM1176_AUXCTL_PHD	0x10000000 /* inst. prefetch halting disable */
#define	ARM1176_AUXCTL_BFD	0x20000000 /* branch folding disable */
#define	ARM1176_AUXCTL_FSD	0x40000000 /* force speculative ops disable */
#define	ARM1176_AUXCTL_FIO	0x80000000 /* low intr latency override */

/* XScale Auxillary Control Register (CP15 register 1, opcode2 1) */
#define	XSCALE_AUXCTL_K		0x00000001 /* dis. write buffer coalescing */
#define	XSCALE_AUXCTL_P		0x00000002 /* ECC protect page table access */
#define	XSCALE_AUXCTL_MD_WB_RA	0x00000000 /* mini-D$ wb, read-allocate */
#define	XSCALE_AUXCTL_MD_WB_RWA	0x00000010 /* mini-D$ wb, read/write-allocate */
#define	XSCALE_AUXCTL_MD_WT	0x00000020 /* mini-D$ wt, read-allocate */
#define	XSCALE_AUXCTL_MD_MASK	0x00000030

/* ARM11 MPCore Auxillary Control Register (CP15 register 1, opcode2 1) */
#define	MPCORE_AUXCTL_RS	0x00000001 /* return stack */
#define	MPCORE_AUXCTL_DB	0x00000002 /* dynamic branch prediction */
#define	MPCORE_AUXCTL_SB	0x00000004 /* static branch prediction */
#define	MPCORE_AUXCTL_F 	0x00000008 /* instruction folding enable */
#define	MPCORE_AUXCTL_EX	0x00000010 /* exclusive L1/L2 cache */
#define	MPCORE_AUXCTL_SA	0x00000020 /* SMP/AMP */

/* Marvell Feroceon Extra Features Register (CP15 register 1, opcode2 0) */
#define FC_DCACHE_REPL_LOCK	0x80000000 /* Replace DCache Lock */
#define FC_DCACHE_STREAM_EN	0x20000000 /* DCache Streaming Switch */
#define FC_WR_ALLOC_EN		0x10000000 /* Enable Write Allocate */
#define FC_L2_PREF_DIS		0x01000000 /* L2 Cache Prefetch Disable */
#define FC_L2_INV_EVICT_LINE	0x00800000 /* L2 Invalidates Uncorrectable Error Line Eviction */
#define FC_L2CACHE_EN		0x00400000 /* L2 enable */
#define FC_ICACHE_REPL_LOCK	0x00080000 /* Replace ICache Lock */
#define FC_GLOB_HIST_REG_EN	0x00040000 /* Branch Global History Register Enable */
#define FC_BRANCH_TARG_BUF_DIS	0x00020000 /* Branch Target Buffer Disable */
#define FC_L1_PAR_ERR_EN	0x00010000 /* L1 Parity Error Enable */

/* Cache type register definitions 0 */
#define	CPU_CT_FORMAT(x)	(((x) >> 29) & 0x7)	/* reg format */
#define	CPU_CT_ISIZE(x)		((x) & 0xfff)		/* I$ info */
#define	CPU_CT_DSIZE(x)		(((x) >> 12) & 0xfff)	/* D$ info */
#define	CPU_CT_S		(1U << 24)		/* split cache */
#define	CPU_CT_CTYPE(x)		(((x) >> 25) & 0xf)	/* cache type */

#define	CPU_CT_CTYPE_WT		0	/* write-through */
#define	CPU_CT_CTYPE_WB1	1	/* write-back, clean w/ read */
#define	CPU_CT_CTYPE_WB2	2	/* w/b, clean w/ cp15,7 */
#define	CPU_CT_CTYPE_WB6	6	/* w/b, cp15,7, lockdown fmt A */
#define	CPU_CT_CTYPE_WB7	7	/* w/b, cp15,7, lockdown fmt B */
#define	CPU_CT_CTYPE_WB14	14	/* w/b, cp15,7, lockdown fmt C */

#define	CPU_CT_xSIZE_LEN(x)	((x) & 0x3)		/* line size */
#define	CPU_CT_xSIZE_M		(1U << 2)		/* multiplier */
#define	CPU_CT_xSIZE_ASSOC(x)	(((x) >> 3) & 0x7)	/* associativity */
#define	CPU_CT_xSIZE_SIZE(x)	(((x) >> 6) & 0x7)	/* size */
#define	CPU_CT_xSIZE_P		(1U << 11)		/* need to page-color */

/* format 4 definitions */
#define	CPU_CT4_ILINE(x)	((x) & 0xf)		/* I$ line size */
#define	CPU_CT4_DLINE(x)	(((x) >> 16) & 0xf)	/* D$ line size */
#define	CPU_CT4_L1IPOLICY(x)	(((x) >> 14) & 0x3)	/* I$ policy */
#define	CPU_CT4_L1_VIPT		2			/* VIPT */

/* Cache size identifaction register definitions 1, Rd, c0, c0, 0 */
#define	CPU_CSID_CTYPE_WT	0x80000000	/* write-through avail */ 
#define	CPU_CSID_CTYPE_WB	0x40000000	/* write-back avail */ 
#define	CPU_CSID_CTYPE_RA	0x20000000	/* read-allocation avail */ 
#define	CPU_CSID_CTYPE_WA	0x10000000	/* write-allocation avail */ 
#define	CPU_CSID_NUMSETS(x)	(((x) >> 13) & 0x7fff)
#define	CPU_CSID_ASSOC(x)	(((x) >> 3) & 0x1ff)
#define	CPU_CSID_LEN(x)		((x) & 0x03)

/* Cache size selection register definitions 2, Rd, c0, c0, 0 */
#define	CPU_CSSR_L2		0x00000002
#define	CPU_CSSR_L1		0x00000000
#define	CPU_CSSR_InD		0x00000001

/* Fault status register definitions */

#define FAULT_TYPE_MASK 0x0f
#define FAULT_USER      0x10

#define FAULT_WRTBUF_0  0x00 /* Vector Exception */
#define FAULT_WRTBUF_1  0x02 /* Terminal Exception */
#define FAULT_BUSERR_0  0x04 /* External Abort on Linefetch -- Section */
#define FAULT_BUSERR_1  0x06 /* External Abort on Linefetch -- Page */
#define FAULT_BUSERR_2  0x08 /* External Abort on Non-linefetch -- Section */
#define FAULT_BUSERR_3  0x0a /* External Abort on Non-linefetch -- Page */
#define FAULT_BUSTRNL1  0x0c /* External abort on Translation -- Level 1 */
#define FAULT_BUSTRNL2  0x0e /* External abort on Translation -- Level 2 */
#define FAULT_ALIGN_0   0x01 /* Alignment */
#define FAULT_ALIGN_1   0x03 /* Alignment */
#define FAULT_TRANS_S   0x05 /* Translation -- Section */
#define FAULT_TRANS_P   0x07 /* Translation -- Page */
#define FAULT_DOMAIN_S  0x09 /* Domain -- Section */
#define FAULT_DOMAIN_P  0x0b /* Domain -- Page */
#define FAULT_PERM_S    0x0d /* Permission -- Section */
#define FAULT_PERM_P    0x0f /* Permission -- Page */

#define	FAULT_IMPRECISE	0x400	/* Imprecise exception (XSCALE) */

/*
 * Address of the vector page, low and high versions.
 */
#define	ARM_VECTORS_LOW		0x00000000U
#define	ARM_VECTORS_HIGH	0xffff0000U

/*
 * ARM Instructions
 *
 *       3 3 2 2 2                              
 *       1 0 9 8 7                                                     0
 *      +-------+-------------------------------------------------------+
 *      | cond  |              instruction dependent                    |
 *      |c c c c|                                                       |
 *      +-------+-------------------------------------------------------+
 */

#define INSN_SIZE		4		/* Always 4 bytes */
#define INSN_COND_MASK		0xf0000000	/* Condition mask */
#define INSN_COND_AL		0xe0000000	/* Always condition */

#define THUMB_INSN_SIZE		2		/* Some are 4 bytes.  */

/*
 * Defines and such for arm11 Performance Monitor Counters (p15, c15, c12, 0)
 */
#define ARM11_PMCCTL_E		__BIT(0)	/* enable all three counters */
#define ARM11_PMCCTL_P		__BIT(1)	/* reset both Count Registers to zero */
#define ARM11_PMCCTL_C		__BIT(2)	/* reset the Cycle Counter Register to zero */
#define ARM11_PMCCTL_D		__BIT(3)	/* cycle count divide by 64 */
#define ARM11_PMCCTL_EC0	__BIT(4)	/* Enable Counter Register 0 interrupt */
#define ARM11_PMCCTL_EC1	__BIT(5)	/* Enable Counter Register 1 interrupt */
#define ARM11_PMCCTL_ECC	__BIT(6)	/* Enable Cycle Counter interrupt */
#define ARM11_PMCCTL_SBZa	__BIT(7)	/* UNP/SBZ */
#define ARM11_PMCCTL_CR0	__BIT(8)	/* Count Register 0 overflow flag */
#define ARM11_PMCCTL_CR1	__BIT(9)	/* Count Register 1 overflow flag */
#define ARM11_PMCCTL_CCR	__BIT(10)	/* Cycle Count Register overflow flag */
#define ARM11_PMCCTL_X		__BIT(11)	/* Enable Export of the events to the event bus */
#define ARM11_PMCCTL_EVT1	__BITS(19,12)	/* source of events for Count Register 1 */
#define ARM11_PMCCTL_EVT0	__BITS(27,20)	/* source of events for Count Register 0 */
#define ARM11_PMCCTL_SBZb	__BITS(31,28)	/* UNP/SBZ */
#define ARM11_PMCCTL_SBZ	\
		(ARM11_PMCCTL_SBZa | ARM11_PMCCTL_SBZb)

#define	ARM11_PMCEVT_ICACHE_MISS	0	/* Instruction Cache Miss */
#define	ARM11_PMCEVT_ISTREAM_STALL	1	/* Instruction Stream Stall */
#define	ARM11_PMCEVT_IUTLB_MISS		2	/* Instruction uTLB Miss */
#define	ARM11_PMCEVT_DUTLB_MISS		3	/* Data uTLB Miss */
#define	ARM11_PMCEVT_BRANCH		4	/* Branch Inst. Executed */
#define	ARM11_PMCEVT_BRANCH_MISS	6	/* Branch mispredicted */
#define	ARM11_PMCEVT_INST_EXEC		7	/* Instruction Executed */
#define	ARM11_PMCEVT_DCACHE_ACCESS0	9	/* Data Cache Access */
#define	ARM11_PMCEVT_DCACHE_ACCESS1	10	/* Data Cache Access */
#define	ARM11_PMCEVT_DCACHE_MISS	11	/* Data Cache Miss */
#define	ARM11_PMCEVT_DCACHE_WRITEBACK	12	/* Data Cache Writeback */
#define	ARM11_PMCEVT_PC_CHANGE		13	/* Software PC change */
#define	ARM11_PMCEVT_TLB_MISS		15	/* Main TLB Miss */
#define	ARM11_PMCEVT_DATA_ACCESS	16	/* non-cached data access */
#define	ARM11_PMCEVT_LSU_STALL		17	/* Load/Store Unit stall */
#define	ARM11_PMCEVT_WBUF_DRAIN		18	/* Write buffer drained */
#define	ARM11_PMCEVT_ETMEXTOUT0		32	/* ETMEXTOUT[0] asserted */
#define	ARM11_PMCEVT_ETMEXTOUT1		33	/* ETMEXTOUT[1] asserted */
#define	ARM11_PMCEVT_ETMEXTOUT		34	/* ETMEXTOUT[0 & 1] */
#define	ARM11_PMCEVT_CALL_EXEC		35	/* Procedure call executed */
#define	ARM11_PMCEVT_RETURN_EXEC	36	/* Return executed */
#define	ARM11_PMCEVT_RETURN_HIT		37	/* return address predicted */
#define	ARM11_PMCEVT_RETURN_MISS	38	/* return addr. mispredicted */
#define	ARM11_PMCEVT_CYCLE		255	/* Increment each cycle */

/* Defines for ARM CORTEX performance counters */
#define CORTEX_CNTENS_C __BIT(31)	/* Enables the cycle counter */
#define CORTEX_CNTENC_C __BIT(31)	/* Disables the cycle counter */
#define CORTEX_CNTOFL_C __BIT(31)	/* Cycle counter overflow flag */

#endif	/* _ARM_ARMREG_H */
