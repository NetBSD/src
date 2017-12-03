/*	$NetBSD: mipsNN.h,v 1.5.12.1 2017/12/03 11:36:27 jdolecek Exp $	*/

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Values related to the MIPS32/MIPS64 Privileged Resource Architecture.
 */

#define	_MIPSNN_SHIFT(reg)	__MIPSNN_SHIFT(reg)
#define	__MIPSNN_SHIFT(reg)	MIPSNN_ ## reg ## _SHIFT
#define	_MIPSNN_MASK(reg)	__MIPSNN_MASK(reg)
#define	__MIPSNN_MASK(reg)	MIPSNN_ ## reg ## _MASK

#define	MIPSNN_GET(reg, x)						\
    ((unsigned)((x) & _MIPSNN_MASK(reg)) >> _MIPSNN_SHIFT(reg))
#define	MIPSNN_PUT(reg, val)						\
    (((x) << _MIPSNN_SHIFT(reg)) & _MIPSNN_MASK(reg))

/*
 * Values in Configuration Register (CP0 Register 16, Select 0)
 */

/* "M" (R): Configuration Register 1 present if set.  Defined as always set. */
#define	MIPSNN_CFG_M		0x80000000

/* Reserved for CPU implementations. */
//	reserved		0x7fff0000

/* "BE" (R): Big endian if set, little endian if clear. */
#define	MIPSNN_CFG_BE		0x00008000

/* "AT" (R): architecture type implemented by processor */
#define	MIPSNN_CFG_AT_MASK	0x00006000
#define	MIPSNN_CFG_AT_SHIFT	13

#define	MIPSNN_CFG_AT_MIPS32	0		/* MIPS32 */
#define	MIPSNN_CFG_AT_MIPS64S	1		/* MIPS64S */
#define	MIPSNN_CFG_AT_MIPS64	2		/* MIPS64 */
//	reserved		3

/* "AR" (R): Architecture revision level implemented by proc. */
#define	MIPSNN_CFG_AR_MASK	0x00001c00
#define	MIPSNN_CFG_AR_SHIFT	10

#define	MIPSNN_CFG_AR_REV1	0		/* Revision 1 */
#define	MIPSNN_CFG_AR_REV2	1		/* Revision 2 */
//	reserved		other values

/* "MT" (R): MMU type implemented by processor */
#define	MIPSNN_CFG_MT_MASK	0x00000380
#define	MIPSNN_CFG_MT_SHIFT	7

#define	MIPSNN_CFG_MT_NONE	0		/* No MMU */
#define	MIPSNN_CFG_MT_TLB	1		/* Std TLB */
#define	MIPSNN_CFG_MT_BAT	2		/* Std BAT */
#define	MIPSNN_CFG_MT_FIXED	3		/* Std Fixed mapping */
//	reserved		other values

/* Reserved.  Write as 0, reads as 0. */
//	reserved		0x00000070

/* "M" (R): Virtual instruction cache if set. */
#define	MIPSNN_CFG_VI		0x00000008

/* "K0" (RW): Kseg0 coherency algorithm.  (values are TLB_ATTRs) */
#define	MIPSNN_CFG_K0_MASK	0x00000007
#define	MIPSNN_CFG_K0_SHIFT	0


/*
 * Values in Configuration Register 1 (CP0 Register 16, Select 1)
 */

/* M (R): Configuration Register 2 present. */
#define	MIPSNN_CFG1_M		0x80000000

/* MS (R): Number of TLB entries - 1. */
#define	MIPSNN_CFG1_MS_MASK	0x7e000000
#define	MIPSNN_CFG1_MS_SHIFT	25

#define	MIPSNN_CFG1_MS(x)	(MIPSNN_GET(CFG1_MS, (x)) + 1)

/* "IS" (R): (Primary) I-cache sets per way. */
#define	MIPSNN_CFG1_IS_MASK	0x01c00000
#define	MIPSNN_CFG1_IS_SHIFT	22

#define	MIPSNN_CFG1_IS_RSVD	7		/* rsvd value, otherwise: */
#define	MIPSNN_CFG1_IS(x)	(64 << MIPSNN_GET(CFG1_IS, (x)))

/* "IL" (R): (Primary) I-cache line size. */
#define	MIPSNN_CFG1_IL_MASK	0x00380000
#define	MIPSNN_CFG1_IL_SHIFT	19

#define	MIPSNN_CFG1_IL_NONE	0		/* No I-cache, */
#define	MIPSNN_CFG1_IL_RSVD	7		/* rsvd value, otherwise: */
#define	MIPSNN_CFG1_IL(x)	(2 << MIPSNN_GET(CFG1_IL, (x)))

/* "IA" (R): (Primary) I-cache associativity (ways - 1). */
#define	MIPSNN_CFG1_IA_MASK	0x00070000
#define	MIPSNN_CFG1_IA_SHIFT	16

#define	MIPSNN_CFG1_IA(x)	MIPSNN_GET(CFG1_IA, (x))

/* "DS" (R): (Primary) D-cache sets per way. */
#define	MIPSNN_CFG1_DS_MASK	0x0000e000
#define	MIPSNN_CFG1_DS_SHIFT	13

#define	MIPSNN_CFG1_DS_RSVD	7		/* rsvd value, otherwise: */
#define	MIPSNN_CFG1_DS(x)	(64 << MIPSNN_GET(CFG1_DS, (x)))

/* "DL" (R): (Primary) D-cache line size. */
#define	MIPSNN_CFG1_DL_MASK	0x00001c00
#define	MIPSNN_CFG1_DL_SHIFT	10

#define	MIPSNN_CFG1_DL_NONE	0		/* No D-cache, */
#define	MIPSNN_CFG1_DL_RSVD	7		/* rsvd value, otherwise: */
#define	MIPSNN_CFG1_DL(x)	(2 << MIPSNN_GET(CFG1_DL, (x)))

/* "DA" (R): (Primary) D-cache associativity (ways - 1). */
#define	MIPSNN_CFG1_DA_MASK	0x00000380
#define	MIPSNN_CFG1_DA_SHIFT	7

#define	MIPSNN_CFG1_DA(x)	MIPSNN_GET(CFG1_DA, (x))

/* "C2" (R): Coprocessor 2 implemented if set. */
#define MIPSNN_CFG1_C2		0x00000040

/* "MD" (R): MDMX ASE implemented if set. */
#define MIPSNN_CFG1_MD		0x00000020

/* "PC" (R): Performance Counters implemented if set. */
#define	MIPSNN_CFG1_PC		0x00000010

/* "WR" (R): Watch registers implemented if set. */
#define	MIPSNN_CFG1_WR		0x00000008

/* "CA" (R): Code compressiong (MIPS16) implemented if set. */
#define	MIPSNN_CFG1_CA		0x00000004

/* "EP" (R): EJTAG implemented if set. */
#define	MIPSNN_CFG1_EP		0x00000002

/* "FP" (R): FPU implemented if set. */
#define	MIPSNN_CFG1_FP		0x00000001

/*
 * Values in Configuration Register 2 (CP0 Register 16, Select 2)
 */

/* "M" (R): Configuration Register 3 present. */
#define	MIPSNN_CFG2_M		0x80000000

/* "TU" (RW): Implementation specific tertiary cache status and control. */
#define	MIPSNN_CFG2_TU_MASK	0x70000000
#define	MIPSNN_CFG2_TU_SHIFT	28

/* "TS" (R): Tertiary cache sets per way. */
#define	MIPSNN_CFG2_TS_MASK	0x07000000
#define	MIPSNN_CFG2_TS_SHIFT	24

#define	MIPSNN_CFG2_TS(x)	(64 << MIPSNN_GET(CFG2_TS, (x)))

/* "TL" (R): Tertiary cache line size. */
#define	MIPSNN_CFG2_TL_MASK	0x00700000
#define	MIPSNN_CFG2_TL_SHIFT	20

#define	MIPSNN_CFG2_TL_NONE	0		/* No Tertiary cache */
#define	MIPSNN_CFG2_TL(x)	(2 << MIPSNN_GET(CFG2_TL, (x)))

/* "TA" (R): Tertiary cache associativity (ways - 1). */
#define	MIPSNN_CFG2_TA_MASK	0x00070000
#define	MIPSNN_CFG2_TA_SHIFT	16

#define	MIPSNN_CFG2_TA(x)	MIPSNN_GET(CFG2_TA, (x))

/* "SU" (RW): Implementation specific secondary cache status and control. */
#define	MIPSNN_CFG2_SU_MASK	0x0000f000
#define	MIPSNN_CFG2_SU_SHIFT	12

/* "SS" (R): Secondary cache sets per way. */
#define	MIPSNN_CFG2_SS_MASK	0x00000700
#define	MIPSNN_CFG2_SS_SHIFT	8

#define	MIPSNN_CFG2_SS(x)	(64 << MIPSNN_GET(CFG2_SS, (x)))

/* "SL" (R): Secdonary cache line size. */
#define	MIPSNN_CFG2_SL_MASK	0x00000070
#define	MIPSNN_CFG2_SL_SHIFT	4

#define	MIPSNN_CFG2_SL_NONE	0		/* No Secondary cache */
#define	MIPSNN_CFG2_SL(x)	(2 << MIPSNN_GET(CFG2_SL, (x)))

/* "SA" (R): Secondary cache associativity (ways - 1). */
#define	MIPSNN_CFG2_SA_MASK	0x00000007
#define	MIPSNN_CFG2_SA_SHIFT	0

#define	MIPSNN_CFG2_SA(x)	MIPSNN_GET(CFG2_SA, (x))

/*
 * Values in Configuration Register 3 (CP0 Register 16, Select 3)
 */

/* "M" (R): Configuration Register 4 present. */
#define	MIPSNN_CFG3_M		0x80000000

/* "BPG" (R): Big Pages feature is implemented (PageMask is 64-bits wide). */
#define	MIPSNN_CFG3_BPG		0x40000000

/* "CMGCR" (R): Coherency Manager memory-mapped Global Configuration Register Space is implemented. */
#define	MIPSNN_CFG3_CMGCR	0x20000000

/* "ULRP" (R): UserLocal register is implemented. */
#define	MIPSNN_CFG3_ULRP	0x00002000

/* "IPLW" (R): Width of Status[IPL] and Cause[RIPL] fields. */
#define	MIPSNN_CFG3_IPLW_MASK	0x00600000
#define	MIPSNN_CFG3_IPLW_SHIFT	21

#define	MIPSNN_CFG3_IPLW_6BITS	0	/* IPL and RIPL fields are 6-bits in width. */
#define	MIPSNN_CFG3_IPLW_8BITS	1	/* IPL and RIPL fields are 8-bits in width. */
//	reserved		other values

#define	MIPSNN_CFG3_MMAR_MASK	0x001c0000
#define	MIPSNN_CFG3_MMAR_SHIFT	18

#define	MIPSNN_CFG3_MMAR_REV1	0		/* Revision 1 */
//	reserved		other values

/* "MCU" (R): MCU ASE extension present. */
#define	MIPSNN_CFG3_MCU		0x00020000

/* "ISAOnExc" (R): ISA used on exception. */
#define	MIPSNN_CFG3_ISAOnExc	0x00010000	/* microMIPS used on entrance to exception vector */

/* "ISA" (R): Instruction Set Availability. */
#define	MIPSNN_CFG3_ISA_MASK	0x0000c000
#define	MIPSNN_CFG3_ISA_SHIFT	14

#define	MIPSNN_CFG3_ISA_MIPS64		0	/* only MIPS64 */
#define	MIPSNN_CFG3_ISA_microMIPS64	1	/* only microMIPS64 */
#define	MIPSNN_CFG3_ISA_MIPS64_OOR	2	/* both, MIPS64 out of reset */
#define	MIPSNN_CFG3_ISA_microMIPS64_OOR	3	/* both, microMIPS64 OOR */

/* "DSP2P" (R): DSP v2 ASE extension present. */
#define	MIPSNN_CFG3_DSP2P	0x00000800

/* "DSPP" (R): DSP ASE extension present. */
#define	MIPSNN_CFG3_DSPP	0x00000400

/* "LPA" (R): Large physical addresses implemented. (MIPS64 rev 2 only). */
#define	MIPSNN_CFG3_LPA		0x00000080

/* "VEIC" (R): External interrupt controller present. (rev 2 only). */
#define	MIPSNN_CFG3_VEIC	0x00000040

/* "VINT" (R): Vectored interrupts implemented. (rev 2 only). */
#define	MIPSNN_CFG3_VINT	0x00000020

/* "SP" (R): Small (1K) page support implemented. (rev 2 only). */
#define	MIPSNN_CFG3_SP		0x00000010

/* "MT" (R): MT ASE extension implemented. */
#define	MIPSNN_CFG3_MT		0x00000004

/* "SM" (R): SmartMIPS ASE extension implemented. */
#define	MIPSNN_CFG3_SM		0x00000002

/* "TL" (R): Trace Logic implemented. */
#define	MIPSNN_CFG3_TL		0x00000001

/*
 * Values in Configuration Register 6 (CP0 Register 16, Select 6)
 * for RMI XLP processors
 */

/* "CTLB_SIZE" (R): Number of Combined TLB entries - 1. */
#define MIPSNN_RMIXLP_CFG6_CTLB_SIZE_MASK	0xffff0000
#define MIPSNN_RMIXLP_CFG6_CTLB_SIZE_SHIFT	16

/* "VTLB_SIZE" (R): Number of Variable TLB entries - 1. */
#define MIPSNN_RMIXLP_CFG6_VTLB_SIZE_MASK	0x0000ffc0
#define MIPSNN_RMIXLP_CFG6_VTLB_SIZE_SHIFT	6

/* "ELVT" (RW): Enable Large Variable TLB. */
#define MIPSNN_RMIXLP_CFG6_ELVT			0x00000020

/* "EPW" (RW): Enable PageWalker. */
#define MIPSNN_RMIXLP_CFG6_EPW			0x00000008

/* "EFT" (RW): Enable Fixed TLB. */
#define MIPSNN_RMIXLP_CFG6_EFT			0x00000004

/* "PWI" (R): PageWalker implemented. */
#define MIPSNN_RMIXLP_CFG6_PWI			0x00000001

/* "FTI" (R): Fixed TLB implemented. */
#define MIPSNN_RMIXLP_CFG6_FTI			0x00000001

/*
 * Values in Configuration Register 7 (CP0 Register 16, Select 7)
 * for RMI XLP processors
 */

/* "LG" (RW): Small or Large Page. */
#define MIPSNN_RMIXLP_CFG7_LG_MASK	__BIT(61)

/* "MASKLG" (RW): large page size supported in CAM only. */
#define MIPSNN_RMIXLP_CFG7_MASKLG_MASK	0x0000ff00
#define MIPSNN_RMIXLP_CFG7_MASKLG_SHIFT	8

#define	MIPSNN_RMIXLP_CFG7_MASKLG_4KB	(0xff >> 8)
#define	MIPSNN_RMIXLP_CFG7_MASKLG_16KB	(0xff >> 7)
#define	MIPSNN_RMIXLP_CFG7_MASKLG_64KB	(0xff >> 6)
#define	MIPSNN_RMIXLP_CFG7_MASKLG_256KB	(0xff >> 5)
#define	MIPSNN_RMIXLP_CFG7_MASKLG_1MB	(0xff >> 4)
#define	MIPSNN_RMIXLP_CFG7_MASKLG_4MB	(0xff >> 3)
#define	MIPSNN_RMIXLP_CFG7_MASKLG_16MB	(0xff >> 2)
#define	MIPSNN_RMIXLP_CFG7_MASKLG_64MB	(0xff >> 1)
#define	MIPSNN_RMIXLP_CFG7_MASKLG_256MB	(0xff >> 0)

/* "MASKSM" (RW): small page size supported in CAM/RAM. */
#define MIPSNN_RMIXLP_CFG7_MASKSM_MASK	0x000000ff
#define MIPSNN_RMIXLP_CFG7_MASKSM_SHIFT	0

#define	MIPSNN_RMIXLP_CFG7_MASKSM_4KB	(0xff >> 8)
#define	MIPSNN_RMIXLP_CFG7_MASKSM_16KB	(0xff >> 7)
#define	MIPSNN_RMIXLP_CFG7_MASKSM_64KB	(0xff >> 6)
#define	MIPSNN_RMIXLP_CFG7_MASKSM_256KB	(0xff >> 5)
#define	MIPSNN_RMIXLP_CFG7_MASKSM_1MB	(0xff >> 4)
#define	MIPSNN_RMIXLP_CFG7_MASKSM_4MB	(0xff >> 3)
#define	MIPSNN_RMIXLP_CFG7_MASKSM_16MB	(0xff >> 2)
#define	MIPSNN_RMIXLP_CFG7_MASKSM_64MB	(0xff >> 1)
#define	MIPSNN_RMIXLP_CFG7_MASKSM_256MB	(0xff >> 0)

/*
 * Values in Configuration Register 6 (CP0 Register 16, Select 6)
 * for the MTI 74K and 1074K cores.
 */
/* "SPCD" (R/W): Sleep state Perforance Counter Disable. */
#define MIPSNN_MTI_CFG6_SPCD		__BIT(14)

/* "SYND" (R/W): SYNonym tag update Disable. */
#define MIPSNN_MTI_CFG6_SYND		__BIT(13)

/* "IFUPerfCtl" (R/W): IFU Performance Control. */
#define MIPSNN_MTI_CFG6_IFU_PERF_CTL_MASK			__BIT(12:10)
#define MIPSNN_MTI_CFG6_IFU_PERF_CTL_STALL			0
#define MIPSNN_MTI_CFG6_IFU_PERF_CTL_JUMP			1
#define MIPSNN_MTI_CFG6_IFU_PERF_CTL_STALLED_INSN		2
#define MIPSNN_MTI_CFG6_IFU_PERF_CTL_CACHE_MISPREDICTION	3
#define MIPSNN_MTI_CFG6_IFU_PERF_CTL_CACHE_PREDICTION		4
#define MIPSNN_MTI_CFG6_IFU_PERF_CTL_BAD_JR_CACHE_ENTRY		5
#define MIPSNN_MTI_CFG6_IFU_PERF_CTL_UNIMPL			6
#define MIPSNN_MTI_CFG6_IFU_PERF_CTL_CBRACH_TAKEN		7

/* "NMRUP" (R): Most Recently Used JTLB Replacement scheme Present. */
#define MIPSNN_MTI_CFG6_NMRUP		__BIT(9)	/* 1: implemented */

/* "NMRUD" (R/W): NMRU Disable. */
#define MIPSNN_MTI_CFG6_NMRUD		__BIT(8)	/* 1: TLBWR is random */

/* "JRCP" (R): JR Cache Present. */
#define MIPSNN_MTI_CFG6_JRCP		__BIT(1)	/* 1: implemented */

/* "JRCD" (R/W): JR Cache prediction Disable. */
#define MIPSNN_MTI_CFG6_JRCD		__BIT(0)	/* 1: disabled */


/*
 * Values in Configuration Register 7 (CP0 Register 16, Select 7)
 * for the MTI 24K, 34K, 74K, 1004K, and 1074K cores
 */

/* "WII" (R): Wait IE Ignore. */
#define MIPSNN_MTI_CFG7_WII		__BIT(31)

/* "FPFS" (R/W): Fast Prepare For Store (74K, 1074K) */
#define MIPSNN_MTI_CFG7_FPFS		__BIT(30)

/* "IHB" (R/W): Implicit HB (74K, 1074K) */
#define MIPSNN_MTI_CFG7_IHB		__BIT(29)

/* "FPR1" (R): Float Point Ratio 1 (74K, 1074K). */
#define MIPSNN_MTI_CFG7_FPR1		__BIT(28)	/* 1: 3:2 */

/* "SEHB" (R/W): slow EHB (74K, 1074K) */
#define MIPSNN_MTI_CFG7_SEHB		__BIT(27)

/* "CP2IO" (R/W): Force COP2 data to be in-order (74K, 1074K) */
#define MIPSNN_MTI_CFG7_CP2IO		__BIT(26)

/* "IAGN" (R/W): Issue LSU-side instructions in program order (74K, 1074K) */
#define MIPSNN_MTI_CFG7_IAGN		__BIT(25)

/* "IAGN" (R/W): Issue LSU-side instructions in program order (74K, 1074K) */
#define MIPSNN_MTI_CFG7_IAGN		__BIT(25)

/* "IALU" (R/W): Issue ALU-side instructions in program order (74K, 1074K) */
#define MIPSNN_MTI_CFG7_IALU		__BIT(24)

/* "DGHR" (R/W): disable global history in branch prediction (74K, 1074K). */
#define MIPSNN_MTI_CFG7_DGHR		__BIT(23)	/* 1: disable */

/* "SG" (R/W): Single Graduation per cycle (74K, 1074K). */
#define MIPSNN_MTI_CFG7_SG		__BIT(22)	/* 1: no superscalar */

/* "SUI" (R/W): Strict Uncached Instruction (SUI) policy control (74K, 1074K). */
#define MIPSNN_MTI_CFG7_SUI		__BIT(21)

/* "NCWB" (R/W): Non-Choerent WriteBack (1004K). */
#define MIPSNN_MTI_CFG7_NCWB		__BIT(20)

/* "PCT" (R): Performance Counters per TC (34K, 1004K). */
#define MIPSNN_MTI_CFG7_PCT		__BIT(19)

/* "HCI" (R): Hardware Cache Initialization. */
#define MIPSNN_MTI_CFG7_HCI		__BIT(18)

/* "FPR" (R): Float Point Ratio. */
#define MIPSNN_MTI_CFG7_FPR0		__BIT(17)	/* 1: half speed */

#define	MIPSNN_MTI_CFG7_FPR_MASK	(MIPSNN_MTI_CFG7_FPR1|MIPSNN_MTI_CFG7_FPR0)
#define	MIPSNN_MTI_CFG7_FPR_SHIFT	0
#define	MIPSNN_MTI_CFG7_FPR_1to1	0
#define	MIPSNN_MTI_CFG7_FPR_2to1	MIPSNN_MTI_CFG7_FPR0
#define	MIPSNN_MTI_CFG7_FPR_3to2	MIPSNN_MTI_CFG7_FPR1
#define	MIPSNN_MTI_CFG7_FPR_RESERVED	MIPSNN_MTI_CFG7_FPR_MASK

/* "AR" (R): Alias Removal. */
#define MIPSNN_MTI_CFG7_AR		__BIT(16)	/* 1: no virt aliases */

/* "PREF" (R/W): Instruction Prefetching (74K, 1074K). */
#define MIPSNN_MTI_CFG7_PREF_MASK	__BITS(12:11)
#define	MIPSNN_MTI_CFG7_PREF_SHIFT	11
#define	MIPSNN_MTI_CFG7_PREF_DISABLE	0
#define	MIPSNN_MTI_CFG7_PREF_ONELINE	1
#define	MIPSNN_MTI_CFG7_PREF_RESERVED	2
#define	MIPSNN_MTI_CFG7_PREF_TWOLINES	3

/* "IAR" (R): Instruction Alias Removal. */
#define MIPSNN_MTI_CFG7_IAR		__BIT(10)	/* 1: no virt aliases */

/* "IVA" (R or RW): Instruction Virtual Alias fix disable. */
#define MIPSNN_MTI_CFG7_IVA		__BIT(9)	/* 1: fix disable */

/* "ES" (RW): External Sync. */
#define MIPSNN_MTI_CFG7_ES		__BIT(8)

/* "BTLM" (RW): Block TC on Load Miss. */
#define MIPSNN_MTI_CFG7_BTLM		__BIT(7)

/* "CPOOO" (RW): Out-Of-Order on Coprocessor interfaces (COP0/COP1). */
#define MIPSNN_MTI_CFG7_CPOOO		__BIT(6)	/* 1: disable OOO */

/* "NBLSU" (RW): Non-Blocking LSU. (24K, 34K) */
#define MIPSNN_MTI_CFG7_NBLSU		__BIT(5)	/* 1: stalls pipeline */

/* "UBL" (RW): Uncached Loads Blocking. */
#define MIPSNN_MTI_CFG7_UBL		__BIT(4)	/* 1: blocking loads */

/* "BP" (RW): Branch Prediction. */
#define MIPSNN_MTI_CFG7_BP		__BIT(3)	/* 1: disabled */

/* "RPS" (RW): Return Prediction Stack. */
#define MIPSNN_MTI_CFG7_RPS		__BIT(2)	/* 1: disabled */

/* "BHT" (RW): Branch History Table. */
#define MIPSNN_MTI_CFG7_BHT		__BIT(1)	/* 1: disabled */

/* "SL" (RW): Scheduled Loads. */
#define MIPSNN_MTI_CFG7_SL		__BIT(0)	/* 1: load misses block */
