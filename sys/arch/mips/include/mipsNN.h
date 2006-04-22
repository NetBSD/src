/*	$NetBSD: mipsNN.h,v 1.3.34.1 2006/04/22 11:37:42 simonb Exp $	*/

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

/* "DSPP" (R): DSPP ASE extension present. */
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
