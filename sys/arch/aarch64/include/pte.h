/* $NetBSD: pte.h,v 1.1.4.2 2014/08/20 00:02:39 tls Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _AARCH64_PTE_H_
#define _AARCH64_PTE_H_

#ifdef __aarch64__

typedef unsigned long long pt_entry_t;

#define LX_VALID		__BIT(0)
#define LX_TYPE			__BIT(1)
#define LX_TYPE_BLK		__SHIFTOUT(0, LX_TYPE)
#define LX_TYPE_TBL		__SHIFTOUT(1, LX_TYPE)
#define L3_TYPE_PAG		__SHIFTOUT(1, LX_TYPE)

#define	L1_BLK_OS		__BITS(58, 55)
#define	L1_BLK_UXN		__BIT(54)
#define	L1_BLK_PXN		__BIT(53)
#define	L1_BLK_CONTIG		__BIT(52)
#define	L1_BLK_OA		__BITS(47, 30)	/* 1GB */
#define L1_BLK_NG		__BIT(11)	// Not Global
#define L1_BLK_AF		__BIT(10)	// Access Flag
#define L1_BLK_SH		__BITS(9,8)	// Shareability
#define L1_BLK_AP		__BITS(7,6)
#define L1_BLK_NS		__BIT(5)
#define L1_BLK_ATTR_INDX	__BITS(4,2)

#define LX_TBL_NLTA_4K		__BITS(47, 12)
#define LX_TBL_NLTA_16K		__BITS(47, 14)
#define LX_TBL_NLTA_64K		__BITS(47, 16)
#define	L1_TBL_NS_TABLE		__BIT(63)
#define	L1_TBL_AP_TABLE		__BITS(62,61)
#define	L1_TBL_XN_TABLE		__BIT(60)
#define	L1_TBL_PXN_TABLE	__BIT(59)

#define	L2_BLKPAG_OS		__BITS(58, 55)
#define	L2_BLKPAG_UXN		__BIT(54)
#define	L2_BLKPAG_CONTIG	__BIT(52)
#define	L2_BLK_OA_4K		__BITS(47, 21)	// 2MB
#define	L2_BLK_OA_16K		__BITS(47, 25)	// 32MB
#define	L2_BLK_OA_64K		__BITS(47, 29)	// 512MB
#define L2_BLKPAG_AF		__BIT(10)	// Access Flag
#define L2_BLKPAG_SH		__BITS(9,8)	// Shareability
#define L2_BLKPAG_S2AP		__BITS(7,6)
#define L2_BLKPAG_MEM_ATTR	__BITS(5,2)

#define L3_PAG_OA_4K		__BITS(47, 12)
#define L3_PAG_OA_16K		__BITS(47, 14)
#define L3_PAG_OA_64K		__BITS(47, 16)

#define TCR_TBI1		__BIT(38)	// ignore Top Byte for TTBR1_EL1
#define TCR_TBI0		__BIT(37)	// ignore Top Byte for TTBR0_EL1
#define TCR_AS64K		__BIT(36)	// Use 64K ASIDs
#define TCR_IPS			__BITS(34,32)	// Intermediate Phys Addr Size
#define	TCR_IPS_256TB		5		// 48 bits (256 TB)
#define	TCR_IPS_64TB		4		// 44 bits  (16 TB)
#define	TCR_IPS_4TB		3		// 42 bits  ( 4 TB)
#define	TCR_IPS_1TB		2		// 40 bits  ( 1 TB)
#define	TCR_IPS_64GB		1		// 36 bits  (64 GB)
#define	TCR_IPS_4GB		0		// 32 bits   (4 GB)
#define TCR_TG1			__BITS(31,30)	// Page Granule Size
#define TCR_PAGE_SIZE1(tcr)	(1L << (__SHIFTOUT(tcr, TCR_TG1) * 2 + 10))
#define	TCR_TG_4KB		1		// 4KB page size
#define	TCR_TG_16KB		2		// 16KB page size
#define	TCR_TG_64KB		3		// 64KB page size
#define TCR_SH1			__BITS(29,28)
#define TCR_SH_NONE		0
#define TCR_SH_OUTER		1
#define TCR_SH_INNER		2
#define TCR_ORGN1		__BITS(27,26)
#define	TCR_XRGN_NC		0		// Non Cacheable
#define	TCR_XRGN_WB_WA		1		// WriteBack WriteAllocate
#define	TCR_XRGN_WT		0		// WriteThrough
#define	TCR_XRGN_WB		0		// WriteBack
#define TCR_IRGN1		__BITS(25,24)
#define TCR_EPD1		__BIT(23)	// Walk Disable for TTBR1_EL1
#define TCR_A1			__BIT(22)	// ASID is in TTBR1_EL1
#define TCR_T1SZ		__BITS(21,16)	// Size offset for TTBR1_EL1
#define TCR_TG0			__BITS(15,14)
#define TCR_SH0			__BITS(13,12)
#define TCR_ORGN1		__BITS(11,10)
#define TCR_IRGN1		__BITS(9,8)
#define TCR_EPD0		__BIT(7)	// Walk Disable for TTBR0
#define TCR_T0SZ		__BITS(5,0)	// Size offset for TTBR0_EL1

#define	TTBR_ASID		__BITS(63, 48)
#define	TTBR_BADDR		__BITS(47, 0)

#elif defined(__arm__)

#include <arm/pte.h>

#endif /* __aarch64__/__arm__ */

#endif /* _AARCH64_PTE_H_ */
