/*	$NetBSD: mipsNN.h,v 1.1 2002/03/05 16:07:10 simonb Exp $	*/

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
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
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
//	reserved		0x00000078

/* "K0" (RW): Kseg0 coherency algorithm.  (values are TLB_ATTRs) */
#define	MIPSNN_CFG_K0_MASK	0x00000007
#define	MIPSNN_CFG_K0_SHIFT	0


/*
 * Values in Configuration Register 1 (CP0 Register 16, Select 1)
 */

/* Reserved for Configuration Register 2 present.  Write as 0, reads as 0. */
//	reserved		0x80000000

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

/* Reserved.  Write as 0, reads as 0. */
//	reserved		0x00000060

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
