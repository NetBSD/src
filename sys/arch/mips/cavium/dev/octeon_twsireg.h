/*	$NetBSD: octeon_twsireg.h,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

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
 * TWSI Registers
 */

#ifndef _OCTEON_TWSIREG_H_
#define _OCTEON_TWSIREG_H_

/* ---- register addresses */

#define	MIO_TWS_SW_TWSI				0x0001180000001000ULL
#define	MIO_TWS_TWSI_SW				0x0001180000001008ULL
#define	MIO_TWS_INT				0x0001180000001010ULL
#define	MIO_TWS_SW_TWSI_EXT			0x0001180000001018ULL

/* ---- register bits */

#define	MIO_TWS_SW_TWSI_V			UINT64_C(0x8000000000000000)
#define	MIO_TWS_SW_TWSI_SLONLY			UINT64_C(0x4000000000000000)
#define	MIO_TWS_SW_TWSI_EIA			UINT64_C(0x2000000000000000)
#define	MIO_TWS_SW_TWSI_OP			UINT64_C(0x1e00000000000000)
#define	 MIO_TWS_SW_TWSI_OP_SHIFT		57
#define	  MIO_TWS_SW_TWSI_OP_ONE		(0x0 << MIO_TWS_SW_TWSI_OP_SHIFT)
#define	  MIO_TWS_SW_TWSI_OP_MCLK		(0x4 << MIO_TWS_SW_TWSI_OP_SHIFT)
#define	  MIO_TWS_SW_TWSI_OP_EXTEND		(0x6 << MIO_TWS_SW_TWSI_OP_SHIFT)
#define	  MIO_TWS_SW_TWSI_OP_FOUR		(0x8 << MIO_TWS_SW_TWSI_OP_SHIFT)
#define	  MIO_TWS_SW_TWSI_OP_COMBR		(0x1 << MIO_TWS_SW_TWSI_OP_SHIFT)
#define	  MIO_TWS_SW_TWSI_OP_10BIT		(0x2 << MIO_TWS_SW_TWSI_OP_SHIFT)
#define	MIO_TWS_SW_TWSI_R			UINT64_C(0x0100000000000000)
#define	MIO_TWS_SW_TWSI_SOVR			UINT64_C(0x0080000000000000)
#define	MIO_TWS_SW_TWSI_SIZE			UINT64_C(0x0070000000000000)
#define	MIO_TWS_SW_TWSI_SCR			UINT64_C(0x000c000000000000)
#define	MIO_TWS_SW_TWSI_A			UINT64_C(0x0003ff0000000000)
#define	 MIO_TWS_SW_TWSI_A_SHIFT		40
#define	MIO_TWS_SW_TWSI_IA			UINT64_C(0x000000f800000000)
#define	MIO_TWS_SW_TWSI_EOP_IA			UINT64_C(0x0000000700000000)
#define	 MIO_TWS_SW_TWSI_EOP_IA_SHIFT		32
#define	  MIO_TWS_SW_TWSI_EOP_IA_TWSI_SLAVE_ADD	(0x0 << MIO_TWS_SW_TWSI_EOP_IA_SHIFT)
#define	  MIO_TWS_SW_TWSI_EOP_IA_TWSI_DATA	(0x1 << MIO_TWS_SW_TWSI_EOP_IA_SHIFT)
#define	  MIO_TWS_SW_TWSI_EOP_IA_TWSI_CTL	(0x2 << MIO_TWS_SW_TWSI_EOP_IA_SHIFT)
#define	  MIO_TWS_SW_TWSI_EOP_IA_TWSI_CLKCTL	(0x3 << MIO_TWS_SW_TWSI_EOP_IA_SHIFT)
#define	  MIO_TWS_SW_TWSI_EOP_IA_TWSI_STAT	(0x3 << MIO_TWS_SW_TWSI_EOP_IA_SHIFT)
#define	  MIO_TWS_SW_TWSI_EOP_IA_TWSI_SLAVE_ADD_EXT \
						(0x4 << MIO_TWS_SW_TWSI_EOP_IA_SHIFT)
#define	  MIO_TWS_SW_TWSI_EOP_IA_TWSI_RST	(0x7 << MIO_TWS_SW_TWSI_EOP_IA_SHIFT)
#define	MIO_TWS_SW_TWSI_D			UINT64_C(0x00000000ffffffff)

#define	MIO_TWS_TWSI_SW_V			UINT64_C(0xc000000000000000)
#define	MIO_TWS_TWSI_SW_XXX_61_32		UINT64_C(0x3fffffff00000000)
#define	MIO_TWS_TWSI_SW_D			UINT64_C(0x00000000ffffffff)

#define	MIO_TWS_INT_XXX_63_12			UINT64_C(0xfffffffffffff000)
#define	MIO_TWS_INT_SCL				UINT64_C(0x0000000000000800)
#define	MIO_TWS_INT_SDA				UINT64_C(0x0000000000000400)
#define	MIO_TWS_INT_SCL_OVR			UINT64_C(0x0000000000000200)
#define	MIO_TWS_INT_SDA_OVR			UINT64_C(0x0000000000000100)
#define	MIO_TWS_INT_XXX_7			UINT64_C(0x0000000000000080)
#define	MIO_TWS_INT_CORE_EN			UINT64_C(0x0000000000000040)
#define	MIO_TWS_INT_TS_EN			UINT64_C(0x0000000000000020)
#define	MIO_TWS_INT_ST_EN			UINT64_C(0x0000000000000010)
#define	MIO_TWS_INT_XXX_3			UINT64_C(0x0000000000000008)
#define	MIO_TWS_INT_CORE_INT			UINT64_C(0x0000000000000004)
#define	MIO_TWS_INT_TS_INT			UINT64_C(0x0000000000000002)
#define	MIO_TWS_INT_ST_INT			UINT64_C(0x0000000000000001)

#define	MIO_TWS_SW_TWSI_EXT_XXX_63_40		UINT64_C(0xffffff0000000000)
#define	MIO_TWS_SW_TWSI_EXT_IA			UINT64_C(0x000000ff00000000)
#define	MIO_TWS_SW_TWSI_EXT_D			UINT64_C(0x00000000ffffffff)

/*
 * TWSI Control Registers
 */

/* TWSI Slave Address Registers */

#define	TWSI_SLAVE_ADD_ADDR			0xfe
#define	TWSI_SLAVE_ADD_GCE			UINT8_C(0x01)

/* TWSI Slave Extended-Address Registers */

/* TWSI Data Register */

/* TWSI Control Register */

#define	TWSI_CTL_CE				UINT8_C(0x80)
#define	TWSI_CTL_ENAB				UINT8_C(0x40)
#define	TWSI_CTL_STA				UINT8_C(0x20)
#define	TWSI_CTL_STP				UINT8_C(0x10)
#define	TWSI_CTL_IFLG				UINT8_C(0x08)
#define	TWSI_CTL_AAK				UINT8_C(0x04)
#define	TWSI_CTL_XXX_1_0			0x03

/* TWSI Status Register */

/* ---- snprintb */

#define	MIO_TWS_SW_TWSI_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x3f"		"V\0" \
	"b\x3e"		"SLONLY\0" \
	"b\x3d"		"EIA\0" \
	"f\x39\x04"	"OP\0" \
	"b\x38"		"R\0" \
	"b\x37"		"SOVR\0" \
	"f\x34\x03"	"SIZE\0" \
	"f\x32\x02"	"SCR\0" \
	"f\x28\x0a"	"A\0" \
	"f\x23\x05"	"IA\0" \
	"f\x20\x03"	"EOP_IA\0" \
	"f\x00\x20"	"D\0"

#define	MIO_TWS_TWSI_SW_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x3e\x02"	"V\0" \
	"f\x00\x20"	"D\0"

#define	MIO_TWS_INT_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x0b"		"SCL\0" \
	"b\x0a"		"SDA\0" \
	"b\x09"		"SCL_OVR\0" \
	"b\x08"		"SDA_OVR\0" \
	"b\x06"		"CORE_EN\0" \
	"b\x05"		"TS_EN\0" \
	"b\x04"		"ST_EN\0" \
	"b\x02"		"CORE_INT\0" \
	"b\x01"		"TS_INT\0" \
	"b\x00"		"ST_INT\0"

#define	MIO_TWS_SW_TWSI_EXT_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x20\x08"	"IA\0" \
	"f\x00\x20"	"D\0"

/* ---- bus_space */

#define	MIO_TWS_NUNITS				1
#define	MIO_TWS_BASE_0				0x0001180000001000ULL
#define	MIO_TWS_SIZE				0x0020

#define	MIO_TWS_SW_TWSI_OFFSET			0x0000
#define	MIO_TWS_TWSI_SW_OFFSET			0x0008
#define	MIO_TWS_INT_OFFSET			0x0010
#define	MIO_TWS_SW_TWSI_EXT_OFFSET		0x0018

#endif /* _OCTEON_TWSIREG_H_ */
