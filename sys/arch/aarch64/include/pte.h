/* $NetBSD: pte.h,v 1.2.12.2 2018/07/28 04:37:26 pgoyette Exp $ */

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

#ifndef _LOCORE
typedef uint64_t pd_entry_t;	/* L0(512G) / L1(1G) / L2(2M) table entry */
typedef uint64_t pt_entry_t;	/* L3(4k) table entry */
#endif /* _LOCORE */

/*
 * translation table, block, and page descriptors
 */
#define LX_TBL_NSTABLE		__BIT(63)	/* inherited next level */
#define LX_TBL_APTABLE		__BITS(62,61)	/* inherited next level */
#define LX_TBL_APTABLE_NOEFFECT		__SHIFTIN(0,LX_TBL_APTABLE)
#define LX_TBL_APTABLE_EL0_NOACCESS	__SHIFTIN(1,LX_TBL_APTABLE)
#define LX_TBL_APTABLE_RO		__SHIFTIN(2,LX_TBL_APTABLE)
#define LX_TBL_APTABLE_RO_EL0_NOREAD	__SHIFTIN(3,LX_TBL_APTABLE)
#define LX_TBL_UXNTABLE		__BIT(60)	/* inherited next level */
#define LX_TBL_PXNTABLE		__BIT(59)	/* inherited next level */
#define LX_BLKPAG_OS		__BITS(58, 55)
# define LX_BLKPAG_OS_0		__SHIFTIN(1,LX_BLKPAG_OS)
# define LX_BLKPAG_OS_1		__SHIFTIN(2,LX_BLKPAG_OS)
# define LX_BLKPAG_OS_2		__SHIFTIN(4,LX_BLKPAG_OS)
# define LX_BLKPAG_OS_3		__SHIFTIN(8,LX_BLKPAG_OS)
#define LX_BLKPAG_UXN		__BIT(54)	/* Unprivileged Execute Never */
#define LX_BLKPAG_PXN		__BIT(53)	/* Privileged Execute Never */
#define LX_BLKPAG_CONTIG	__BIT(52)	/* Hint of TLB cache */
#define LX_TBL_PA		__BITS(47, 12)
#define LX_BLKPAG_OA		__BITS(47, 12)
#define LX_BLKPAG_NG		__BIT(11)	/* Not Global */
#define LX_BLKPAG_AF		__BIT(10)	/* Access Flag */
#define LX_BLKPAG_SH		__BITS(9,8)	/* Shareability */
#define LX_BLKPAG_SH_NS		__SHIFTIN(0,LX_BLKPAG_SH) /* Non Shareable */
#define LX_BLKPAG_SH_OS		__SHIFTIN(2,LX_BLKPAG_SH) /* Outer Shareable */
#define LX_BLKPAG_SH_IS		__SHIFTIN(3,LX_BLKPAG_SH) /* Inner Shareable */
#define LX_BLKPAG_AP		__BIT(7)
#define LX_BLKPAG_AP_RW		__SHIFTIN(0,LX_BLKPAG_AP) /* RW */
#define LX_BLKPAG_AP_RO		__SHIFTIN(1,LX_BLKPAG_AP) /* RO */
#define LX_BLKPAG_APUSER	__BIT(6)
#define LX_BLKPAG_NS		__BIT(5)
#define LX_BLKPAG_ATTR_INDX	__BITS(4,2)	/* refer MAIR_EL1 attr<n> */
#define  LX_BLKPAG_ATTR_INDX_0	__SHIFTIN(0,LX_BLKPAG_ATTR_INDX)
#define  LX_BLKPAG_ATTR_INDX_1	__SHIFTIN(1,LX_BLKPAG_ATTR_INDX)
#define  LX_BLKPAG_ATTR_INDX_2	__SHIFTIN(2,LX_BLKPAG_ATTR_INDX)
#define  LX_BLKPAG_ATTR_INDX_3	__SHIFTIN(3,LX_BLKPAG_ATTR_INDX)
#define LX_TYPE			__BIT(1)
#define LX_TYPE_BLK		__SHIFTIN(0, LX_TYPE)
#define LX_TYPE_TBL		__SHIFTIN(1, LX_TYPE)
#define L3_TYPE_PAG		__SHIFTIN(1, LX_TYPE)
#define LX_VALID		__BIT(0)

#define L1_BLK_OA		__BITS(47, 30)	/* 1GB */
#define L2_BLK_OA		__BITS(47, 21)	/* 2MB */
#define L3_PAG_OA		__BITS(47, 12)	/* 4KB */


/* L0 table, 512GB/entry * 512 */
#define L0_SHIFT		39
#define L0_ADDR_BITS		__BITS(47,39)
#define L0_SIZE			(1UL << L0_SHIFT)
#define L0_OFFSET		(L0_SIZE - 1UL)
#define L0_FRAME		(~L0_OFFSET)
/*      L0_BLOCK		Level 0 doesn't support block translation */
#define L0_TABLE		(LX_TYPE_TBL | LX_VALID)

/* L1 table, 1GB/entry * 512 */
#define L1_SHIFT		30
#define L1_ADDR_BITS		__BITS(38,30)
#define L1_SIZE			(1UL << L1_SHIFT)
#define L1_OFFSET		(L1_SIZE - 1UL)
#define L1_FRAME		(~L1_OFFSET)
#define L1_BLOCK		(LX_BLKPAG_NG | LX_TYPE_BLK | LX_VALID)
#define L1_TABLE		(LX_TYPE_TBL | LX_VALID)

/* L2 table, 2MB/entry * 512 */
#define L2_SHIFT		21
#define L2_ADDR_BITS		__BITS(29,21)
#define L2_SIZE			(1UL << L2_SHIFT)
#define L2_OFFSET		(L2_SIZE - 1UL)
#define L2_FRAME		(~L2_OFFSET)
#define L2_BLOCK		(LX_BLKPAG_NG | LX_TYPE_BLK | LX_VALID)
#define L2_TABLE		(LX_TYPE_TBL | LX_VALID)
#define L2_BLOCK_MASK		__BITS(47,21)

/* L3 table, 4KB/entry * 512 */
#define L3_SHIFT		12
#define L3_ADDR_BITS		__BITS(20,12)
#define L3_SIZE			(1UL << L3_SHIFT)
#define L3_OFFSET		(L3_SIZE - 1UL)
#define L3_FRAME		(~L3_OFFSET)
#define L3_PAGE			(LX_BLKPAG_NG | L3_TYPE_PAG | LX_VALID)

#define Ln_ENTRIES_SHIFT	9
#define Ln_ENTRIES		(1 << Ln_ENTRIES_SHIFT)
#define Ln_TABLE_SIZE		(8 << Ln_ENTRIES_SHIFT)


/* TCR_EL1 - Translation Control Register */
#define TCR_TBI1	__BIT(38)		/* ignore Top Byte TTBR1_EL1 */
#define TCR_TBI0	__BIT(37)		/* ignore Top Byte TTBR0_EL1 */
#define TCR_AS64K	__BIT(36)		/* Use 64K ASIDs */
#define TCR_IPS		__BITS(34,32)		/* Intermediate PhysAdr Size */
#define TCR_IPS_256TB	__SHIFTIN(5,TCR_IPS)	/* 48 bits (256 TB) */
#define TCR_IPS_64TB	__SHIFTIN(4,TCR_IPS)	/* 44 bits  (16 TB) */
#define TCR_IPS_4TB	__SHIFTIN(3,TCR_IPS)	/* 42 bits  ( 4 TB) */
#define TCR_IPS_1TB	__SHIFTIN(2,TCR_IPS)	/* 40 bits  ( 1 TB) */
#define TCR_IPS_64GB	__SHIFTIN(1,TCR_IPS)	/* 36 bits  (64 GB) */
#define TCR_IPS_4GB	__SHIFTIN(0,TCR_IPS)	/* 32 bits   (4 GB) */
#define TCR_TG1		__BITS(31,30)		/* TTBR1 Page Granule Size */
#define TCR_TG1_16KB	__SHIFTIN(1,TCR_TG1)	/* 16KB page size */
#define TCR_TG1_4KB	__SHIFTIN(2,TCR_TG1)	/* 4KB page size */
#define TCR_TG1_64KB	__SHIFTIN(3,TCR_TG1)	/* 64KB page size */
#define TCR_SH1		__BITS(29,28)
#define TCR_SH1_NONE	__SHIFTIN(0,TCR_SH1)
#define TCR_SH1_OUTER	__SHIFTIN(2,TCR_SH1)
#define TCR_SH1_INNER	__SHIFTIN(3,TCR_SH1)
#define TCR_ORGN1	__BITS(27,26)		/* TTBR1 Outer cacheability */
#define TCR_ORGN1_NC	__SHIFTIN(0,TCR_ORGN1)	/* Non Cacheable */
#define TCR_ORGN1_WB_WA	__SHIFTIN(1,TCR_ORGN1)	/* WriteBack WriteAllocate */
#define TCR_ORGN1_WT	__SHIFTIN(2,TCR_ORGN1)	/* WriteThrough */
#define TCR_ORGN1_WB	__SHIFTIN(3,TCR_ORGN1)	/* WriteBack */
#define TCR_IRGN1	__BITS(25,24)		/* TTBR1 Inner cacheability */
#define TCR_IRGN1_NC	__SHIFTIN(0,TCR_IRGN1)	/* Non Cacheable */
#define TCR_IRGN1_WB_WA	__SHIFTIN(1,TCR_IRGN1)	/* WriteBack WriteAllocate */
#define TCR_IRGN1_WT	__SHIFTIN(2,TCR_IRGN1)	/* WriteThrough */
#define TCR_IRGN1_WB	__SHIFTIN(3,TCR_IRGN1)	/* WriteBack */
#define TCR_EPD1	__BIT(23)		/* Walk Disable for TTBR1_EL1 */
#define TCR_A1		__BIT(22)		/* ASID is in TTBR1_EL1 */
#define TCR_T1SZ	__BITS(21,16)		/* Size offset for TTBR1_EL1 */
#define TCR_TG0		__BITS(15,14)		/* TTBR0 Page Granule Size */
#define TCR_TG0_16KB	__SHIFTIN(1,TCR_TG1)	/* 16KB page size */
#define TCR_TG0_4KB	__SHIFTIN(2,TCR_TG1)	/* 4KB page size */
#define TCR_TG0_64KB	__SHIFTIN(3,TCR_TG1)	/* 64KB page size */
#define TCR_SH0		__BITS(13,12)
#define TCR_SH0_NONE	__SHIFTIN(0,TCR_SH0)
#define TCR_SH0_OUTER	__SHIFTIN(2,TCR_SH0)
#define TCR_SH0_INNER	__SHIFTIN(3,TCR_SH0)
#define TCR_ORGN0	__BITS(11,10)		/* TTBR0 Outer cacheability */
#define TCR_ORGN0_NC	__SHIFTIN(0,TCR_ORGN0)	/* Non Cacheable */
#define TCR_ORGN0_WB_WA	__SHIFTIN(1,TCR_ORGN0)	/* WriteBack WriteAllocate */
#define TCR_ORGN0_WT	__SHIFTIN(2,TCR_ORGN0)	/* WriteThrough */
#define TCR_ORGN0_WB	__SHIFTIN(3,TCR_ORGN0)	/* WriteBack */
#define TCR_IRGN0	__BITS(9,8)		/* TTBR0 Inner cacheability */
#define TCR_IRGN0_NC	__SHIFTIN(0,TCR_IRGN0)	/* Non Cacheable */
#define TCR_IRGN0_WB_WA	__SHIFTIN(1,TCR_IRGN0)	/* WriteBack WriteAllocate */
#define TCR_IRGN0_WT	__SHIFTIN(2,TCR_IRGN0)	/* WriteThrough */
#define TCR_IRGN0_WB	__SHIFTIN(3,TCR_IRGN0)	/* WriteBack */
#define TCR_EPD0	__BIT(7)		/* Walk Disable for TTBR0 */
#define TCR_T0SZ	__BITS(5,0)		/* Size offset for TTBR0_EL1 */


/* TTBR0_EL1, TTBR1_EL1 - Translation Table Base Register */
#define TTBR_ASID		__BITS(63,48)
#define TTBR_BADDR		__BITS(47,0)


#elif defined(__arm__)

#include <arm/pte.h>

#endif /* __aarch64__/__arm__ */

#endif /* _AARCH64_PTE_H_ */
