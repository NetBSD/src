/*	$NetBSD: sxreg.h,v 1.11.12.2 2017/02/05 13:40:20 skrll Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Lorenz.
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

/* register definitions for Sun's SX / SPAM rendering engine */

#ifndef SXREG_H
#define SXREG_H

/* SX control registers */
#define SX_CONTROL_STATUS	0x00000000
#define SX_ERROR		0x00000004
#define SX_PAGE_BOUND_LOWER	0x00000008
#define SX_PAGE_BOUND_UPPER	0x0000000c
#define SX_PLANEMASK		0x00000010
#define SX_ROP_CONTROL		0x00000014	/* 8 bit ROP */
#define SX_IQ_OVERFLOW_COUNTER	0x00000018
#define SX_DIAGNOSTICS		0x0000001c
#define SX_INSTRUCTIONS		0x00000020
#define SX_ID			0x00000028
#define SX_R0_INIT		0x0000002c
#define SX_SOFTRESET		0x00000030
/* write registers directly, only when processor is stopped */
#define SX_DIRECT_R0		0x00000100
#define SX_DIRECT_R1		0x00000104	/* and so on until R127 */
/* write registers via pseudo instructions */
#define SX_QUEUED_R0		0x00000300
#define SX_QUEUED_R1		0x00000304	/* and so on until R127 */
#define SX_QUEUED(r)		(0x300 + ((r) << 2))

/* special purpose registers */
#define R_ZERO	0
#define R_SCAM	1
#define R_MASK	2	/* bitmask for SX_STORE_SELECT */

/*
 * registers are repeated at 0x1000 with certain parts read only
 * ( like the PAGE_BOUND registers ) which userland has no business writing to
 */

/* SX_CONTROL_STATUS */
#define SX_EE1		0x00000001	/* illegal instruction */
#define SX_EE2		0x00000002	/* page bound error */
#define SX_EE3		0x00000004	/* illegal memory access */
#define SX_EE4		0x00000008	/* illegal register access */
#define SX_EE5		0x00000010	/* alignment violation */
#define SX_EE6		0x00000020	/* illegal instruction queue write */
#define SX_EI		0x00000080	/* interrupt on error */
#define SX_PB		0x00001000	/* enable page bound checking */
#define SX_WO		0x00002000	/* write occured ( by SX ) */
#define SX_GO		0x00004000	/* start/stop the processor */
#define SX_MT		0x00008000	/* instruction queue is empty */

/* SX_ERROR */
#define SX_SE1		0x00000001	/* illegal instruction */
#define SX_SE2		0x00000002	/* page bound error */
#define SX_SE3		0x00000004	/* illegal memory access */
#define SX_SE4		0x00000008	/* illegal register access */
#define SX_SE5		0x00000010	/* alignment violation */
#define SX_SE6		0x00000020	/* illegal instruction queue write */
#define SX_SI		0x00000080	/* interrupt on error */

/* SX_ID */
#define SX_ARCHITECTURE_MASK	0x000000ff
#define SX_CHIP_REVISION	0x0000ff00

/* SX_DIAGNOSTICS */
#define SX_IQ_FIFO_ACCESS	0x00000001	/* allow memory instructions
						 * in SX_INSTRUCTIONS */

/*
 * memory referencing instructions are written to 0x800000000 + PA
 * so we have to go through ASI 0x28 ( ASI_BYPASS + 8 )
 */
#define ASI_SX	0x28

/* load / store instructions */
#define SX_STORE_COND	(0x4 << 19)	/* conditional write with mask */
#define SX_STORE_CLAMP	(0x2 << 19)
#define SX_STORE_MASK	(0x1 << 19)	/* apply plane mask */
#define SX_STORE_SELECT	(0x8 << 19)	/* expand with plane reg dest[0]/dest[1] */
#define SX_LOAD		(0xa << 19)
#define SX_STORE	(0x0 << 19)

/* data type */
#define SX_UBYTE_0	(0x00 << 14)
#define SX_UBYTE_8	(0x01 << 14)
#define SX_UBYTE_16	(0x02 << 14)
#define SX_UBYTE_24	(0x03 << 14)
#define SX_SBYTE_0	(0x04 << 14)
#define SX_SBYTE_8	(0x05 << 14)
#define SX_SBYTE_16	(0x06 << 14)
#define SX_SBYTE_24	(0x07 << 14)
#define SX_UQUAD_0	(0x08 << 14)
#define SX_UQUAD_8	(0x09 << 14)
#define SX_UQUAD_16	(0x0a << 14)
#define SX_UQUAD_24	(0x0b << 14)
#define SX_SQUAD_0	(0x0c << 14)
#define SX_SQUAD_8	(0x0d << 14)
#define SX_SQUAD_16	(0x0e << 14)
#define SX_SQUAD_24	(0x0f << 14)
#define SX_UCHAN_0	(0x10 << 14)
#define SX_UCHAN_8	(0x11 << 14)
#define SX_UCHAN_16	(0x12 << 14)
#define SX_UCHAN_24	(0x13 << 14)
#define SX_SCHAN_0	(0x14 << 14)
#define SX_SCHAN_8	(0x15 << 14)
#define SX_SCHAN_16	(0x16 << 14)
#define SX_SCHAN_24	(0x17 << 14)
#define SX_USHORT_0	(0x18 << 14)
#define SX_USHORT_8	(0x19 << 14)
#define SX_USHORT_16	(0x1a << 14)
#define SX_SSHORT_0	(0x1c << 14)
#define SX_SSHORT_8	(0x1d << 14)
#define SX_SSHORT_16	(0x1e << 14)
#define SX_LONG		(0x1b << 14)
#define SX_PACKED	(0x1f << 14)


#define SX_LD(dreg, cnt, o)  (0x80000000 | ((cnt) << 23) | SX_LOAD | \
				SX_LONG | (dreg << 7) | (o))
#define SX_LDB(dreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_LOAD | \
				SX_UBYTE_0 | (dreg << 7) | (o))
#define SX_LDP(dreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_LOAD | \
				SX_PACKED | (dreg << 7) | (o))
#define SX_LDUQ0(dreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_LOAD | \
				SX_UQUAD_0 | (dreg << 7) | (o))
#define SX_LDUQ8(dreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_LOAD | \
				SX_UQUAD_8 | (dreg << 7) | (o))
#define SX_LDUQ16(dreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_LOAD | \
				SX_UQUAD_16 | (dreg << 7) | (o))
#define SX_LDUQ24(dreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_LOAD | \
				SX_UQUAD_24 | (dreg << 7) | (o))
#define SX_ST(sreg, cnt, o)  (0x80000000 | ((cnt) << 23) | SX_STORE | \
				SX_LONG | (sreg << 7) | (o))
#define SX_STM(sreg, cnt, o)  (0x80000000 | ((cnt) << 23) | SX_STORE_MASK | \
				SX_LONG | (sreg << 7) | (o))
#define SX_STB(sreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_STORE | \
				SX_UBYTE_0 | (sreg << 7) | (o))
#define SX_STBM(sreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_STORE_MASK | \
				SX_UBYTE_0 | (sreg << 7) | (o))
#define SX_STBC(sreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_STORE_CLAMP | \
				SX_UBYTE_0 | (sreg << 7) | (o))
#define SX_STP(sreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_STORE | \
				SX_PACKED | (sreg << 7) | (o))
#define SX_STS(sreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_STORE_SELECT \
				| SX_LONG | (sreg << 7) | (o))
#define SX_STBS(reg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_STORE_SELECT \
				| SX_UBYTE_0 | (reg << 7) | (o))
#define SX_STUQ0(sreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_STORE | \
				SX_UQUAD_0 | (sreg << 7) | (o))
#define SX_STUQ0C(sreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_STORE_CLAMP | \
				SX_UQUAD_0 | (sreg << 7) | (o))
#define SX_STUQ8(sreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_STORE | \
				SX_UQUAD_8 | (sreg << 7) | (o))
#define SX_STUQ16(sreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_STORE | \
				SX_UQUAD_16 | (sreg << 7) | (o))
#define SX_STUQ24(sreg, cnt, o) (0x80000000 | ((cnt) << 23) | SX_STORE | \
				SX_UQUAD_24 | (sreg << 7) | (o))

/* ROP and SELECT instructions */
#define SX_ROPB	(0x0 << 21)	/* mask bits apply to bytes */
#define SX_ROPM	(0x1 << 21)	/* mask bits apply to each bit */
#define SX_ROPL	(0x2 << 21)	/* mask bits apply per register */
#define SX_SELB	(0x4 << 21)	/* byte select scalar */
#define SX_SELV (0x6 << 21)	/* register select vector */
#define SX_SELS (0x7 << 21)	/* register select scalar */

#define SX_ROP(sa, sb, d, cnt) (0x90000000 | ((cnt) << 24) | SX_ROPL | \
		((sa) << 14) | (sb) | ((d) << 7))
#define SX_SELECT_S(sa, sb, d, cnt) (0x90000000 | ((cnt) << 24) | SX_SELS | \
		((sa) << 14) | (sb) | ((d) << 7))

/* multiply group */
#define SX_M16X16SR0	(0x0 << 28)	/* 16bit multiply, no shift */
#define SX_M16X16SR8	(0x1 << 28)	/* 16bit multiply, shift right 8 */
#define SX_M16X16SR16	(0x2 << 28)	/* 16bit multiply, shift right 16 */
#define SX_M32X16SR0	(0x4 << 28)	/* 32x16bit multiply, no shift */
#define SX_M32X16SR8	(0x5 << 28)	/* 32x16bit multiply, shift right 8 */
#define SX_M32X16SR16	(0x6 << 28)	/* 32x16bit multiply, shift right 16 */

#define SX_MULTIPLY	(0x0 << 21)	/* normal multiplication */
#define SX_DOT		(0x1 << 21)	/* dot product of A and B */
#define SX_SAXP		(0x2 << 21)	/* A * SCAM + B */

#define SX_ROUND	(0x1 << 23)	/* round results */

#define SX_MUL16X16(sa, sb, d, cnt) (SX_M16X16SR0 | ((cnt) << 24) | \
		SX_MULTIPLY | ((sa) << 14) | ((d) << 7) | (sb))	
#define SX_MUL16X16R(sa, sb, d, cnt) (SX_M16X16SR0 | ((cnt) << 24) | \
		SX_MULTIPLY | ((sa) << 14) | ((d) << 7) | (sb) | SX_ROUND)	
#define SX_MUL16X16SR8(sa, sb, d, cnt) (SX_M16X16SR8 | ((cnt) << 24) | \
		SX_MULTIPLY | ((sa) << 14) | ((d) << 7) | (sb))	
#define SX_MUL16X16SR8R(sa, sb, d, cnt) (SX_M16X16SR8 | ((cnt) << 24) | \
		SX_MULTIPLY | ((sa) << 14) | ((d) << 7) | (sb) | SX_ROUND)	

#define SX_SAXP16X16(sa, sb, d, cnt) (SX_M16X16SR0 | ((cnt) << 24) | \
		SX_SAXP | ((sa) << 14) | ((d) << 7) | (sb))	
#define SX_SAXP16X16R(sa, sb, d, cnt) (SX_M16X16SR0 | ((cnt) << 24) | \
		SX_SAXP | ((sa) << 14) | ((d) << 7) | (sb) | SX_ROUND)	
#define SX_SAXP16X16SR8(sa, sb, d, cnt) (SX_M16X16SR8 | ((cnt) << 24) | \
		SX_SAXP | ((sa) << 14) | ((d) << 7) | (sb))	
#define SX_SAXP16X16SR8R(sa, sb, d, cnt) (SX_M16X16SR8 | ((cnt) << 24) | \
		SX_SAXP | ((sa) << 14) | ((d) << 7) | (sb) | SX_ROUND)	

/* logic group */
#define SX_AND_V	(0x0 << 21)	/* vector AND vector */
#define SX_AND_S	(0x1 << 21)	/* vector AND scalar */
#define SX_AND_I	(0x2 << 21)	/* vector AND immediate */
#define SX_XOR_V	(0x3 << 21)	/* vector XOR vector */
#define SX_XOR_S	(0x4 << 21)	/* vector XOR scalar */
#define SX_XOR_I	(0x5 << 21)	/* vector XOR immediate */
#define SX_OR_V		(0x6 << 21)	/* vector OR vector */
#define SX_OR_S		(0x7 << 21)	/* vector OR scalar */
/* immediates are 7bit sign extended to 32bit */

#define SX_ANDV(sa, sb, d, cnt) (0xb0000000 | ((cnt) << 24) | SX_AND_V | \
		((sa) << 14) | ((d) << 7) | (sb))
#define SX_ANDS(sa, sb, d, cnt) (0xb0000000 | ((cnt) << 24) | SX_AND_S | \
		((sa) << 14) | ((d) << 7) | (sb))
#define SX_ANDI(sa, sb, d, cnt) (0xb0000000 | ((cnt) << 24) | SX_AND_I | \
		((sa) << 14) | ((d) << 7) | (sb))
#define SX_XORV(sa, sb, d, cnt) (0xb0000000 | ((cnt) << 24) | SX_XOR_V | \
		((sa) << 14) | ((d) << 7) | (sb))
#define SX_XORS(sa, sb, d, cnt) (0xb0000000 | ((cnt) << 24) | SX_XOR_S | \
		((sa) << 14) | ((d) << 7) | (sb))
#define SX_XORI(sa, sb, d, cnt) (0xb0000000 | ((cnt) << 24) | SX_XOR_I | \
		((sa) << 14) | ((d) << 7) | (sb))
#define SX_ORV(sa, sb, d, cnt) (0xb0000000 | ((cnt) << 24) | SX_OR_V | \
		((sa) << 14) | ((d) << 7) | (sb))
#define SX_ORS(sa, sb, d, cnt) (0xb0000000 | ((cnt) << 24) | SX_OR_S | \
		((sa) << 14) | ((d) << 7) | (sb))

/* arithmetic group */
#define SX_ADD_V	(0x00 << 21)	/* vector + vector */
#define SX_ADD_S	(0x01 << 21)	/* vector + scalar */
#define SX_ADD_I	(0x02 << 21)	/* vector + immediate */
#define SX_SUM		(0x03 << 21)	/* sum of vector and scalar */
#define SX_SUB_V	(0x04 << 21)	/* vector - vector */
#define SX_SUB_S	(0x05 << 21)	/* vector - scalar */
#define SX_SUB_I	(0x06 << 21)	/* vector - immediate */
#define SX_ABS		(0x07 << 21)	/* abs(sb) with sa=R0 */
/* hardware does sa - sb for sb < 0 and sa + sb if sb > 0 */

#define SX_ADDV(sa, sb, d, cnt) (0xa0000000 | ((cnt) << 24) | SX_ADD_V | \
		((sa) << 14) | ((d) << 7) | (sb))

#endif /* SXREG_H */
