/*	$NetBSD: octeon_asxreg.h,v 1.1.2.2 2015/06/06 14:40:01 skrll Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * ASX Registers
 */

#ifndef _OCTEON_ASXREG_H_
#define _OCTEON_ASXREG_H_

#define	ASX0_RX_PRT_EN				0x00011800b0000000ULL
#define	ASX0_TX_PRT_EN				0x00011800b0000008ULL
#define	ASX0_INT_REG				0x00011800b0000010ULL
#define	ASX0_INT_EN				0x00011800b0000018ULL
#define	ASX0_RX_CLK_SET0			0x00011800b0000020ULL
#define	ASX0_RX_CLK_SET1			0x00011800b0000028ULL
#define	ASX0_RX_CLK_SET2			0x00011800b0000030ULL
#define	ASX0_PRT_LOOP				0x00011800b0000040ULL
#define	ASX0_TX_CLK_SET0			0x00011800b0000048ULL
#define	ASX0_TX_CLK_SET1			0x00011800b0000050ULL
#define	ASX0_TX_CLK_SET2			0x00011800b0000058ULL
#define	ASX0_COMP_BYP				0x00011800b0000068ULL
#define	ASX0_TX_HI_WATER000			0x00011800b0000080ULL
#define	ASX0_TX_HI_WATER001			0x00011800b0000088ULL
#define	ASX0_TX_HI_WATER002			0x00011800b0000090ULL
#define	ASX0_GMII_RX_CLK_SET			0x00011800b0000180ULL
#define	ASX0_GMII_RX_DAT_SET			0x00011800b0000188ULL
#define	ASX0_MII_RX_DAT_SET			0x00011800b0000190ULL

#define ASX0_BASE				0x00011800b0000000ULL
#define ASX0_SIZE				0x0198ULL

#define	ASX0_RX_PRT_EN_OFFSET			0x0000
#define	ASX0_TX_PRT_EN_OFFSET			0x0008
#define	ASX0_INT_REG_OFFSET			0x0010
#define	ASX0_INT_EN_OFFSET			0x0018
#define	ASX0_RX_CLK_SET0_OFFSET			0x0020
#define	ASX0_RX_CLK_SET1_OFFSET			0x0028
#define	ASX0_RX_CLK_SET2_OFFSET			0x0030
#define	ASX0_PRT_LOOP_OFFSET			0x0040
#define	ASX0_TX_CLK_SET0_OFFSET			0x0048
#define	ASX0_TX_CLK_SET1_OFFSET			0x0050
#define	ASX0_TX_CLK_SET2_OFFSET			0x0058
#define	ASX0_COMP_BYP_OFFSET			0x0068
#define	ASX0_TX_HI_WATER000_OFFSET		0x0080
#define	ASX0_TX_HI_WATER001_OFFSET		0x0088
#define	ASX0_TX_HI_WATER002_OFFSET		0x0090
#define	ASX0_GMII_RX_CLK_SET_OFFSET		0x0180
#define	ASX0_GMII_RX_DAT_SET_OFFSET		0x0188
#define	ASX0_MII_RX_DAT_SET_OFFSET		0x0190

/* XXX */


/*
 * ASX_RX_PRT_EN
 */
#define ASX0_RX_PRT_EN_63_3			0xfffffff8
#define ASX0_RX_PRT_EN_PRT_EN			0x00000007

/*
 * ASX0_TX_PRT_EN
 */
#define ASX0_TX_PRT_EN_63_3			0xfffffff8
#define ASX0_TX_PRT_EN_PRT_EN			0x00000007

/*
 * ASX0_INT_REG
 */
#define ASX0_INT_REG_63_11			0xfffff800
#define ASX0_INT_REG_TXPSH			0x00000700
#define ASX0_INT_REG_7				UINT32_C(0x00000080)
#define ASX0_INT_REG_TXPOP			0x00000070
#define ASX0_INT_REG_3				UINT32_C(0x00000008)
#define ASX0_INT_REG_OVRFLW			0x00000007

/*
 * ASX0_INT_EN
 */
#define ASX0_INT_EN_63_11			0xfffff800
#define ASX0_INT_EN_TXPSH			0x00000700
#define ASX0_INT_EN_7				UINT32_C(0x00000080)
#define ASX0_INT_EN_TXPOP			0x00000070
#define ASX0_INT_EN_3				UINT32_C(0x00000008)
#define ASX0_INT_EN_OVRFLW			0x00000007

/*
 * ASX0_RX_CLK_SET
 */
#define ASX0_RX_CLK_SET_63_5			0xffffffe0
#define ASX0_RX_CLK_SET_SETTING			0x0000001f

/*
 * ASX0_RRT_LOOP
 */
#define ASX0_PRT_LOOP_63_7			0xffffff80
#define ASX0_PRT_LOOP_EXT_LOOP			0x00000070
#define ASX0_PRT_LOOP_3				UINT32_C(0x00000008)
#define ASX0_PRT_LOOP_PRT_LOOP			0x00000007

/*
 * ASX0_TX_CLK_SET
 */
#define ASX0_TX_CLK_SET_63_5			0xffffffe0
#define ASX0_TX_CLK_SET_SETTING			0x0000001f

/*
 * ASX0_TX_COMP_BYP
 */
#define ASX0_TX_COMP_BYP_63_9			0xfffffe00
#define ASX0_TX_COMP_BYP_BYPASS			UINT32_C(0x00000100)
#define ASX0_TX_COMP_BYP_PCTL			0x000000f0
#define ASX0_TX_COMP_BYP_NCTL			0x0000000f

/*
 * ASX0_TX_HI_WATER
 */
#define ASX0_TX_HI_WATER_63_3			0xfffffff8
#define ASX0_TX_HI_WATER_MARK			0x00000007

/*
 * ASX0_GMXII_RX_CLK_SET
 */
#define ASX0_GMII_RX_CLK_SET_63_5		0xffffffe0
#define ASX0_GMII_RX_CLK_SET_SETTING		0x0000001f

/*
 * ASX0_GMXII_RX_DAT_SET
 */
#define ASX0_GMII_RX_DAT_SET_63_5		0xffffffe0
#define ASX0_GMII_RX_DAT_SET_SETTING		0x0000001f

/* ---- */

#define	ASX0_RX_PRT_EN_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x03\x3d"	"63_3\0" \
	"f\x00\x03"	"PRT_EN\0"
#define	ASX0_TX_PRT_EN_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x03\x3d"	"63_3\0" \
	"f\x00\x03"	"PRT_EN\0"
#define	ASX0_INT_REG_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x0b\x35"	"63_11\0" \
	"f\x08\x03"	"TXPSH\0" \
	"b\x07"		"7\0" \
	"f\x04\x03"	"TXPOP\0" \
	"b\x03"		"3\0" \
	"f\x00\x03"	"OVRFLW\0"
#define	ASX0_INT_EN_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x0b\x35"	"63_11\0" \
	"f\x08\x03"	"TXPSH\0" \
	"b\x07"		"7\0" \
	"f\x04\x03"	"TXPOP\0" \
	"b\x03"		"3\0" \
	"f\x00\x03"	"OVRFLW\0"
#define	ASX0_RX_CLK_SET0_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \

#define	ASX0_RX_CLK_SET1_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \

#define	ASX0_RX_CLK_SET2_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \

#define	ASX0_PRT_LOOP_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x07\x39"	"63_7\0" \
	"f\x04\x03"	"EXT_LOOP\0" \
	"b\x03"		"3\0" \
	"f\x00\x03"	"PRT_LOOP\0"
#define	ASX0_TX_CLK_SET0_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \

#define	ASX0_TX_CLK_SET1_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \

#define	ASX0_TX_CLK_SET2_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \

#define	ASX0_COMP_BYP_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \

#define	ASX0_TX_HI_WATER000_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \

#define	ASX0_TX_HI_WATER001_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \

#define	ASX0_TX_HI_WATER002_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \

#define	ASX0_GMII_RX_CLK_SET_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x05\x3b"	"63_5\0" \
	"f\x00\x05"	"SETTING\0"
#define	ASX0_GMII_RX_DAT_SET_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x05\x3b"	"63_5\0" \
	"f\x00\x05"	"SETTING\0"
#define	ASX0_MII_RX_DAT_SET_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \


#endif /* _OCTEON_ASXREG_H_ */
