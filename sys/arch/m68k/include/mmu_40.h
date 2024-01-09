/*	$NetBSD: mmu_40.h,v 1.2 2024/01/09 04:16:25 thorpej Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _M68K_MMU_40_H_
#define	_M68K_MMU_40_H_

/*
 * Translation table structures for the 68040 MMU.
 *
 * The 68040 MMU uses a 3-level tree structure.  The root (L1) and
 * and pointer (L2) tables contain the base addresses of the tables
 * at the lext level, and the page (L3) tables contain the addresses
 * of the page descriptors, which may either contain the address of
 * a physical page (4K or 8K) directly, or point to an indirect
 * decriptor which points to the physical page.
 *
 * The L1 and L2 tables contain 128 4-byte descriptors, and are thus 512
 * bytes in size.  Each of the 128 L1 descriptors corresponds to a 32MB
 * region of address space.  Each of the 128 L2 descriptors corresponds
 * to a 256KB region of address space.
 *
 * For 8K pages, the L3 tables contain 32 4-byte descriptors, and are
 * thus 128 bytes in size.
 *
 *  31          25 24          18 17      13 12                       0
 * |              |              |          |                          |
 *  11111111111111 22222222222222 3333333333 ..........................
 *       Root         Pointer        Page               Page
 *       Index         Index         Index             Offset
 *
 * For 4K pages, the L3 tables contain 64 4-byte descriptors, and are
 * thus 256 bytes in size.
 *
 *  31          25 24          18 17        12 11                     0
 * |              |              |            |                        |
 *  11111111111111 22222222222222 333333333333 ........................
 *       Root         Pointer         Page              Page
 *       Index         Index          Index            Offset
 *
 *                       Logical Address Format
 */

#define	LA40_L1_NBITS	7U
#define	LA40_L1_SHIFT	25
#define	LA40_L2_NBITS	7U
#define	LA40_L2_SHIFT	18
#define	LA40_L3_NBITS	(32U - LA40_L1_NBITS - LA40_L2_NBITS - PGSHIFT)
#define	LA40_L3_SHIFT	PGSHIFT

#define	LA40_L1_COUNT	__BIT(LA40_L1_NBITS)
#define	LA40_L2_COUNT	__BIT(LA40_L2_NBITS)
#define	LA40_L3_COUNT	__BIT(LA40_L3_NBITS)

#define	LA40_L1_MASK	(__BITS(0,(LA40_L1_NBITS - 1)) << LA40_L1_SHIFT)
#define	LA40_L2_MASK	(__BITS(0,(LA40_L2_NBITS - 1)) << LA40_L2_SHIFT)
#define	LA40_L3_MASK	(__BITS(0,(LA40_L3_NBITS - 1)) << LA40_L3_SHIFT)

/* N.B. all tables must be aligned to their size */
#define	TBL40_L1_SIZE	(LA40_L1_COUNT * sizeof(uint32_t))
#define	TBL40_L2_SIZE	(LA40_L2_COUNT * sizeof(uint32_t))
#define	TBL40_L3_SIZE	(LA40_L3_COUNT * sizeof(uint32_t))

#define	LA40_RI(va)	__SHIFTOUT((va), LA40_L1_MASK)	/* root index */
#define	LA40_PI(va)	__SHIFTOUT((va), LA40_L2_MASK)	/* pointer index */
#define	LA40_PGI(va)	__SHIFTOUT((va), LA40_L3_MASK)	/* page index */

#define	LA40_TRUNC_L1(va) (((vaddr_t)(va)) & LA40_L1_MASK)
#define	LA40_TRUNC_L2(va) (((vaddr_t)(va)) & (LA40_L1_MASK | LA40_L2_MASK))

/*
 * The PTE format for L1 and L2 tables (Upper Tables).
 */
#define	UTE40_PTA	__BITS(9,31)	/* Pointer Table Address (L1 PTE) */
					/* Page Table Address (L2 PTE) */
#define	UTE40_PGTA	__BITS(8 - (13 - PGSHIFT),31)
#define	UTE40_U		__BIT(3)	/* Used (referenced) */
#define	UTE40_W		__BIT(2)	/* Write Protected */
#define	UTE40_UDT	__BITS(0,1)	/* Upper Descriptor Type */
					/* 00 or 01 -- Invalid */
					/* 10 or 11 -- Resident */

#define	UTE40_INVALID	__SHIFTIN(0, UTE_UDT)
#define	UTE40_RESIDENT	__SHIFTIN(2, UTE_UDT)

/*
 * The PTE format for L3 tables.
 *
 * Some notes:
 *
 * - PFLUSH variants that specify non-global entries do not invalidate
 *   global entries.  If these PFLUSH variants are not used, then the G
 *   bit can be used as a software-defined bit.
 *
 * - The UR bits are "reserved for use by the user", so can be
 *   used as software-defined bits.
 *
 * - The U0 and U1 "User Page Attribute" bits should *not* be used
 *   as software-defined bits; they are reflected on the UPA0 and UPA1
 *   CPU signals if an external bus transfer results from the access,
 *   meaning that they may have system-specific side-effects.
 */
#define	PTE40_PGA	__BITS(PGSHIFT,31) /* Page Physical Address */
#define	PTE40_UR_x	__BIT(12)	/* User Reserved (extra avail if 8K) */
#define	PTE40_UR	__BIT(11)	/* User Reserved */
#define	PTE40_G		__BIT(10)	/* Global */
#define	PTE40_U1	__BIT(9)	/* User Page Attribute 1 */
#define	PTE40_U0	__BIT(8)	/* User Page Attribute 0 */
#define	PTE40_S		__BIT(7)	/* Supervisor Protected */
#define	PTE40_CM	__BITS(5,6)	/* Cache Mode */
					/* 00 -- write-through */
					/* 01 -- copy-back */
					/* 10 -- non-cacheable, serialized */
					/* 11 -- non-cacheable */
#define	PTE40_M		__BIT(4)	/* Modified */
#define	PTE40_U		__BIT(3)	/* Used (referenced) */
#define	PTE40_W		__BIT(2)	/* Write Protected */
#define	PTE40_PDT	__BITS(0,1)	/* Page Descriptor Type */
					/* 00       -- Invalid */
					/* 01 or 11 -- Resident */
					/* 10       -- Indirect */

#define	PTE40_CM_WT	__SHIFTIN(0, PTE40_CM)
#define	PTE40_CM_CB	__SHIFTIN(1, PTE40_CM)
#define	PTE40_CM_NC_SER	__SHIFTIN(2, PTE40_CM)
#define	PTE40_CM_NC	__SHIFTIN(3, PTE40_CM)

#define	PTE40_INVALID	__SHIFTIN(0, PTE40_PDT)
#define	PTE40_RESIDENT	__SHIFTIN(1, PTE40_PDT)
#define	PTE40_INDIRECT	__SHIFTIN(2, PTE40_PDT)

/*
 * MMU registers (and the sections in the 68040 manual that
 * describe them).
 */

/*
 * 3.1.1 -- User and Supervisor Root Pointer Registers (32-bit)
 *
 * URP and SRP contain the physical address of the L1 table for
 * user and supervisor space, respectively.  Bits 8-0 of the address
 * must be 0.
 */

/*
 * 3.1.2 -- Translation Control Register (16-bit)
 */
#define	TCR40_E		__BIT(15)	/* enable translation */
#define	TCR40_P		__BIT(14)	/* page size: 0=4K 1=8K */

/*
 * 3.1.3 -- Transparent Translation Registers (32-bit)
 *
 * There are 2 data translation registers (DTTR0, DTTR1) and 2
 * instruction translation registers (ITTR0, ITTR1).
 */
#define	TTR40_LAB	__BITS(24,31)	/* logical address base */
#define	TTR40_LAM	__BITS(16,23)	/* logical address mask */
#define	TTR40_E		__BIT(15)	/* enable TTR */
#define	TTR40_SFIELD	__BITS(13,14)	/* Supervisor Mode field (see below) */
#define	TTR40_U1	PTE40_U1
#define	TTR40_U0	PTE40_U0
#define	TTR40_CM	PTE40_CM
#define	TTR40_W		PTE40_W

#define	TTR40_USER	__SHIFTIN(0, TTR40_SFIELD)
#define	TTR40_SUPER	__SHIFTIN(1, TTR40_SFIELD)
#define	TTR40_BOTH	__SHIFTIN(2, TTR40_SFIELD)

/*
 * 3.1.4 -- MMU Status Register
 *
 * N.B. If 8K pages are in use, bit 12 of the PA field is **undefined**.
 */
#define	MMUSR40_PA	PTE40_PGA
#define	MMUSR40_B	__BIT(11)	/* bus error */
#define	MMUSR40_G	PTE40_G
#define	MMUSR40_U1	PTE40_U1
#define	MMUSR40_U0	PTE40_U0
#define	MMUSR40_S	PTE40_S
#define	MMUSR40_CM	PTE40_CM
#define	MMUSR40_M	PTE40_M
#define	MMUSR40_W	PTE40_W
#define	MMUSR40_T	__BIT(1)	/* Transparent Translation hit */
#define	MMUSR40_R	PTE40_RESIDENT

#ifdef _KERNEL
void	mmu_load_urp40(paddr_t);
void	mmu_load_urp60(paddr_t);
#endif /* _KERNEL */

#endif /* _M68K_MMU_40_H_ */
