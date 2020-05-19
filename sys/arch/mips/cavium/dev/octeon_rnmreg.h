/*	$NetBSD: octeon_rnmreg.h,v 1.1.26.1 2020/05/19 17:35:50 martin Exp $	*/

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
 * RNM Registers
 */

#ifndef _OCTEON_RNMREG_H_
#define _OCTEON_RNMREG_H_

/* ---- register addresses */

#define	RNM_CTL_STATUS				0x0001180040000000ULL
#define	RNM_BIST_STATUS				0x0001180040000008ULL

/* ---- register bits */

#define RNM_CTL_STATUS_XXX_63_5			UINT64_C(0xfffffffffffffe00)
#define RNM_CTL_STATUS_ENT_SEL_MASK		UINT64_C(0x00000000000001e0)
#define RNM_CTL_STATUS_EXP_ENT			UINT64_C(0x0000000000000010)
#define RNM_CTL_STATUS_RNG_RST			UINT64_C(0x0000000000000008)
#define RNM_CTL_STATUS_RNM_RST			UINT64_C(0x0000000000000004)
#define RNM_CTL_STATUS_RNG_EN			UINT64_C(0x0000000000000002)
#define RNM_CTL_STATUS_ENT_EN			UINT64_C(0x0000000000000001)

#define RNM_BIST_STATUS_XXX_63_2		UINT64_C(0xfffffffffffffffc)
#define RNM_BIST_STATUS_RRC			UINT64_C(0x0000000000000002)
#define RNM_BIST_STATUS_MEM			UINT64_C(0x0000000000000001)

/* ---- operations */
#define RNM_OPERATION_BASE_IO_BIT		UINT64_C(0x0001000000000000)
#define RNM_OPERATION_BASE_MAJOR_DID		UINT64_C(0x0000f80000000000)
#define RNM_OPERATION_BASE_SUB_DID		UINT64_C(0x0000070000000000)
#define	RNM_OPERATION_BASE_MAJOR_DID_SHIFT	43
#define	RNM_OPERATION_BASE_SUB_DID_SHIFT	40
#define	RNM_OPERATION_BASE_IO_BIT_SHIFT	48

/* ---- IOBDMA */

/* 4.7 IOBDMA Operations (XXX move me elsewhere) */
#define	IOBDMA_SCRADDR		__BITS(63,56)
#define	IOBDMA_LEN		__BITS(55,48)
#define	IOBDMA_MAJORDID		__BITS(47,43)
#define	IOBDMA_SUBDID		__BITS(42,40)
/* reserved 39:36 */
#define	IOBDMA_OFFSET		__BITS(35,0)

/* 19.1.12 IOBDMA Operations, p. 661 */
#define	RNM_IOBDMA_MAJORDID	8
#define	RNM_IOBDMA_SUBDID	0

/* ---- snprintb */

#define	RNM_CTL_STATUS_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x05\x04"	"ENT_SEL\0" \
	"b\x04"		"EXP_ENT\0" \
	"b\x03"		"RNG_RST\0" \
	"b\x02"		"RNM_RST\0" \
	"b\x01"		"RNG_EN\0" \
	"b\x00"		"ENT_EN\0"

#define	RNM_BIST_STATUS_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x01"		"RRC\0" \
	"b\x00"		"MEM\0"

/* ---- bus_space */

#define	RNM_BASE				0x0001180040000000ULL
#define	RNM_SIZE				0x0010
#define	RNM_NUNITS				1

#define	RNM_CTL_STATUS_OFFSET			0x0000
#define	RNM_BIST_STATUS_OFFSET			0x0008

#endif /* _OCTEON_RNMREG_H_ */
