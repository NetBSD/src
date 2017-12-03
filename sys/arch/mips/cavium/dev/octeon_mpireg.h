/*	$NetBSD: octeon_mpireg.h,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

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
 * MPI/SPI Registers
 */

#ifndef _OCTEON_MPIREG_H_
#define _OCTEON_MPIREG_H_

#define	MPI_CFG					0x0001070000001000ULL
#define	MPI_STS					0x0001070000001008ULL
#define	MPI_TX					0x0001070000001010ULL
#define	MPI_DAT0				0x0001070000001080ULL
#define	MPI_DAT1				0x0001070000001088ULL
#define	MPI_DAT2				0x0001070000001090ULL
#define	MPI_DAT3				0x0001070000001098ULL
#define	MPI_DAT4				0x00010700000010a0ULL
#define	MPI_DAT5				0x00010700000010a8ULL
#define	MPI_DAT6				0x00010700000010b0ULL
#define	MPI_DAT7				0x00010700000010b8ULL
#define	MPI_DAT8				0x00010700000010c0ULL

#define MPI_CFG_XXX_63_29			UINT64_C(0xffffffffe0000000)
#define MPI_CFG_CLKDIV				UINT64_C(0x000000001fff0000)
#define  MPI_CFG_CLKDIV_SHIFT			16
#define MPI_CFG_XXX_15_12			UINT64_C(0x000000000000f000)
#define MPI_CFG_CSLATE				UINT64_C(0x0000000000000800)
#define MPI_CFG_TRITX				UINT64_C(0x0000000000000400)
#define MPI_CFG_IDLECLKS			UINT64_C(0x0000000000000300)
#define MPI_CFG_CSHI				UINT64_C(0x0000000000000080)
#define MPI_CFG_CSENA				UINT64_C(0x0000000000000040)
#define MPI_CFG_INT_ENA				UINT64_C(0x0000000000000020)
#define MPI_CFG_LSBFIRST			UINT64_C(0x0000000000000010)
#define MPI_CFG_WIREOR				UINT64_C(0x0000000000000008)
#define MPI_CFG_CLK_CONT			UINT64_C(0x0000000000000004)
#define MPI_CFG_IDLELO				UINT64_C(0x0000000000000002)
#define MPI_CFG_ENABLE				UINT64_C(0x0000000000000001)

#define MPI_STS_XXX_63_13			UINT64_C(0xffffffffffffe000)
#define MPI_STS_RXNUM				UINT64_C(0x0000000000001f00)
#define MPI_STS_XXX_7_1				UINT64_C(0x00000000000000fe)
#define MPI_STS_BUSY				UINT64_C(0x0000000000000001)

#define MPI_TX_XXX_63_17			UINT64_C(0xfffffffffffe0000)
#define MPI_TX_LEAVECS				UINT64_C(0x0000000000010000)
#define MPI_TX_XXX_15_13			UINT64_C(0x000000000000e000)
#define MPI_TX_TXNUM				UINT64_C(0x0000000000001f00)
#define  MPI_TX_TXNUM_SHIFT			8
#define MPI_TX_XXX_7_5				UINT64_C(0x00000000000000e0)
#define MPI_TX_TOTNUM				UINT64_C(0x000000000000001f)
#define  MPI_TX_TOTNUM_SHIFT			0

#define MPI_DATX_XXX_63_8			UINT64_C(0xffffffffffffff00)
#define MPI_DATX_DATA				UINT64_C(0x00000000000000ff)

/* ---- snprintb */

#define	MPI_CFG_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x10\x0d"	"CLKDIV\0" \
	"b\x0b"		"CSLATE\0" \
	"b\x0a"		"TRITX\0" \
	"f\x08\x02"	"IDLECLKS\0" \
	"b\x07"		"CSHI\0" \
	"b\x06"		"CSENA\0" \
	"b\x05"		"INT_ENA\0" \
	"b\x04"		"LSBFIRST\0" \
	"b\x03"		"WIREOR\0" \
	"b\x02"		"CLK_CONT\0" \
	"b\x01"		"IDLELO\0" \
	"b\x00"		"ENABLE\0"

#define	MPI_STS_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x08\x05"	"RXNUM\0" \
	"b\x00"		"BUSY\0"

#define	MPI_TX_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x10"		"LEAVECS\0" \
	"f\x08\x05"	"TXNUM\0" \
	"f\x00\x05"	"TOTNUM\0"

/* ---- bus_space */

#define	MPI_BASE				0x0001070000001000ULL
#define	MPI_SIZE				0x0100
#define MPI_NUNITS                              1

#define	MPI_CFG_OFFSET				0x0000
#define	MPI_STS_OFFSET				0x0008
#define	MPI_TX_OFFSET				0x0010
#define	MPI_DAT0_OFFSET				0x0080
#define	MPI_DAT1_OFFSET				0x0088
#define	MPI_DAT2_OFFSET				0x0090
#define	MPI_DAT3_OFFSET				0x0098
#define	MPI_DAT4_OFFSET				0x00a0
#define	MPI_DAT5_OFFSET				0x00a8
#define	MPI_DAT6_OFFSET				0x00b0
#define	MPI_DAT7_OFFSET				0x00b8
#define	MPI_DAT8_OFFSET				0x00c0

#endif /* _OCTEON_MPIREG_H_ */
