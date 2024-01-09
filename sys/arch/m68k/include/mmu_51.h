/*	$NetBSD: mmu_51.h,v 1.3 2024/01/09 07:28:26 thorpej Exp $	*/

/*-
 * Copyright (c) 1997, 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeremy Cooper and by Jason R. Thorpe.
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

#ifndef _M68K_MMU_51_H_
#define	_M68K_MMU_51_H_

/*
 * Translation table structures for the 68851 MMU.
 *
 * The 68851 MMU (as well as the 68030's built-in MMU) are pretty flexible and
 * can use a 1, 2, 3, or 4-level tree structure and a number of page sizes.
 *
 * The logical address format is defined as:
 *
 *  31                                                                0
 * |        |          |          |          |          |              |
 *  SSSSSSSS AAAAAAAAAA BBBBBBBBBB CCCCCCCCCC DDDDDDDDDD PPPPPPPPPPPPPP
 *  Initial   A Index    B Index    C Index    D Index    Page Offset
 *   Shift
 *
 * The Initial Shift, and number of A, B, C, and D index bits are defined
 * in the Translation Control register.  Once the MMU encounters a tree
 * level where the number of index bits is 0, tree traversal stops.  The
 * values of IS + TIA + TIB + TIC + TID + page offset must equal 32.  For
 * example, for a 2-level arrangment using 4KB pages where all 32-bits of
 * the address are significant:
 *
 *     IS  TIA  TIB  TIC  TID  page
 *	0 + 10 + 10 +  0 +  0 +  12 == 32
 */

/*
 * The 68851 has 3 descriptor formats:
 *
 *	Long Table Descriptors		(8 byte)
 *	Short Table Descriptors		(4 byte)
 *	Page Descriptors		(4 byte)
 *
 * These occupy the lower 2 bits of each descriptor and the root pointers.
 */
#define	DT51_INVALID	0
#define	DT51_PAGE	1	/* points to a page */
#define	DT51_SHORT	2	/* points to a short entry table */
#define	DT51_LONG	3	/* points to a long entry table */

/*
 * Long Format Table Descriptor
 *
 *  63                                                             48
 *  +---+---.---.---.---.---.---.---.---.---.---.---.---.---.---.---+
 *  |L/U|                 LIMIT                                     |
 *  +---+---.---+---.---.---+---+---+---+---+---+---+---+---+---.---+
 *  |    RAL    |    WAL    |SG | S | 0 | 0 | 0 | 0 | U |WP |  DT   |
 *  +---.---.---+---.---.---+---+---+---+---+---+---+---+---+---.---+
 *  |              TABLE PHYSICAL ADDRESS (BITS 31-16)              |
 *  +---.---.---.---.---.---.---.---.---.---.---.---+---.---.---.---+
 *  |       TABLE PHYSICAL ADDRESS (15-4)           |     UNUSED    |
 *  +---.---.---.---.---.---.---.---.---.---.---.---+---.---.---.---+
 *  15                                                              0
 *
 * DT is either 2 or 3, depending on what next table descriptor format is.
 */
struct mmu51_ldte {		/* 'dte' stands for 'descriptor table entry' */
	uint32_t	ldte_attr;
	uint32_t	ldte_addr;
};
#define	DTE51_ADDR	__BITS(4,31)	/* table address mask */
#define	DTE51_LOWER	__BIT(31)	/* L: Index limit is lower limit */
#define	DTE51_LIMIT	__BITS(16,30)	/* L: Index limit */
#define	DTE51_RAL	__BITS(13,15)	/* L: Read Access Level */
#define	DTE51_WAL	__BITS(10,12)	/* L: Write Access Level */
#define	DTE51_SG	__BIT(9)	/* L: Shared Globally */
#define	DTE51_S		__BIT(8)	/* L: Supervisor protected */
#define	DTE51_U		__BIT(3)	/* Used */
#define	DTE51_WP	__BIT(2)	/* Write Protected */

/*
 * Short Format Table Descriptor
 *
 * 31                                                             16
 * +---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---+
 * |           TABLE PHYSICAL BASE ADDRESS (BITS 31-16)            |
 * +---.---.---.---.---.---.---.---.---.---.---.---+---+---+---.---+
 * | TABLE PHYSICAL BASE ADDRESS (15-4)            | U |WP |  DT   |
 * +---.---.---.---.---.---.---.---.---.---.---.---+---+---+---.---+
 * 15                                                              0
 *
 * DT is either 2 or 3, depending on what next table descriptor format is.
 */

/*
 * Long Format Page Descriptor (Level A table only)
 *
 *  63                                                             48
 *  +---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---+
 *  |                          UNUSED                               |
 *  +---.---.---+---.---.---+---+---+---+---+---+---+---+---+---.---+
 *  |    RAL    |    WAL    |SG | S | G |CI | L | M | U |WP |DT (01)|
 *  +---.---.---+---.---.---+---+---+---+---+---+---+---+---+---.---+
 *  |               PAGE PHYSICAL ADDRESS (BITS 31-16)              |
 *  +---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---+
 *  | PAGE PHYS. ADDRESS (15-8)     |            UNUSED             |
 *  +---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---+
 *  15                                                              0
 *
 * N.B. Unused bits of the page address (if the page size is larger
 * than 256 bytes) can be used as software-defined PTE bits.
 */
struct mmu51_lpte {		/* 'pte' stands for 'page table entry' */
	uint32_t	lpte_attr;
	uint32_t	lpte_addr;
};
#define	PTE51_ADDR	__BITS(8,31)	/* page address mask */
#define	PTE51_RAL	__BITS(13,15)	/* L: Read Access Level */
#define	PTE51_WAL	__BITS(10,12)	/* L: Write Access Level */
#define	PTE51_SG	__BIT(9)	/* L: Shared Globally */
#define	PTE51_S		__BIT(8)	/* L: Supervisor protected */
#define	PTE51_G		__BIT(7)	/* Gate allowed */
#define	PTE51_CI	__BIT(6)	/* Cache inhibit */
#define	PTE51_L		__BIT(5)	/* Lock entry */
#define	PTE51_M		__BIT(4)	/* Modified */
#define	PTE51_U		__BIT(3)	/* Used */
#define	PTE51_WP	__BIT(2)	/* Write Protected */

/*
 * Short Format Page Descriptor
 *
 * 31                                                             16
 * +---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---+
 * |            PAGE PHYSICAL BASE ADDRESS (BITS 31-16)            |
 * +---.---.---.---.---.---.---.---+---+---+---+---+---+---+---.---+
 * | PAGE PHYS. BASE ADDRESS (15-8)| G |CI | L | M | U |WP |DT (01)|
 * +---.---.---.---.---.---.---.---+---+---+---+---+---+---+---.---+
 * 15                                                              0
 *
 * N.B. Unused bits of the page address (if the page size is larger
 * than 256 bytes) can be used as software-defined PTE bits.
 */

/*
 * MMU registers (and the sections in the 68851 manual that
 * describe them).
 */

/*
 * 5.1.4 -- Root Pointer
 * (and also 6.1.1)
 *
 * This is a 64-bit register.  The upper 32 bits contain configuration
 * information, and the lower 32 bits contain the A table address.
 * Bits 3-0 of the address must be 0.  The root pointer is essentially
 * a long format table descriptor with only the U/L, limit, and SG bits.
 *
 * The 68851 has 3 root pointers:
 *
 * CRP		CPU root pointer, for user accesses
 * SRP		Supervisor root pointer
 * DRP		DMA root pointer, for IOMMU functionality (not on '030)
 *
 * Selection of root pointer is as follows:
 *
 *	FC3	FC2	SRE		Root pointer used
 *	 0	 0	 0		      CRP
 *	 0	 0	 1		      CRP
 *	 0	 1	 0		      CRP
 *	 0	 1	 1		      SRP
 *	 1	 x	 x		      DRP
 */
struct mmu51_rootptr {
	unsigned long	rp_attr;	/* Lower/Upper Limit and access flags */
	unsigned long	rp_addr;	/* Physical Base Address */
};

/*
 * 6.1.2 -- PMMU Cache Status (PCSR) (16-bit)
 */
#define	PCSR51_F	__BIT(15)	/* Flush(ed) */
#define	PCSR51_LW	__BIT(14)	/* Lock Warning */
#define	PCSR51_TA	__BITS(0,2)	/* Task Alias (not '030) */

/*
 * 6.1.3 -- Translation Control (TCR) (32-bit)
 */
#define	TCR51_E		__BIT(31)	/* Enable translation */
#define	TCR51_SRE	__BIT(25)	/* Supervisor Root Enable */
#define	TCR51_FCL	__BIT(24)	/* Function Code Lookup */
#define	TCR51_PS	__BITS(20,23)	/* Page Size (see below) */
#define	TCR51_IS	__BITS(16,19)	/* Initial Shift */
#define	TCR51_TIA	__BITS(12,15)	/* Table A Index bits */
#define	TCR51_TIB	__BITS(8,11)	/* Table B Index bits */
#define	TCR51_TIC	__BITS(4,7)	/* Table C Index bits */
#define	TCR51_TID	__BITS(0,3)	/* Table D Index bits */

/*
 * Astute readers will note that the value in the PS field is
 * log2(PAGE_SIZE).
 */
#define	TCR51_PS_256	__SHIFTIN(0x8, TCR51_PS)
#define	TCR51_PS_512	__SHIFTIN(0x9, TCR51_PS)
#define	TCR51_PS_1K	__SHIFTIN(0xa, TCR51_PS)
#define	TCR51_PS_2K	__SHIFTIN(0xb, TCR51_PS)
#define	TCR51_PS_4K	__SHIFTIN(0xc, TCR51_PS)
#define	TCR51_PS_8K	__SHIFTIN(0xd, TCR51_PS)
#define	TCR51_PS_16K	__SHIFTIN(0xe, TCR51_PS)
#define	TCR51_PS_32K	__SHIFTIN(0xf, TCR51_PS)

/*
 * 6.1.4 -- Current Access Level (8-bit)
 * 6.1.5 -- Validate Access Level
 */
#define	CAL51_AL	__BITS(5,7)

/*
 * 6.1.6 -- Stack Change Control (8-bit)
 */

/*
 * 6.1.7 -- Access Control (16-bit)
 */
#define	AC51_MC		__BIT(7)	/* Module Control */
#define	AC51_ALC	__BITS(4,5)	/* Access Level Control */
#define	AC51_MDS	__BITS(0,1)	/* Module Descriptor Size */

/*
 * 6.1.8 -- PMMU Status Register (PSR) (16-bit)
 */
#define	PSR51_B		__BIT(15)	/* Bus Error */
#define	PSR51_L		__BIT(14)	/* Limit Violation */
#define	PSR51_S		__BIT(13)	/* Supervisor Violation */
#define	PSR51_A		__BIT(12)	/* Access Level Violation */
#define	PSR51_W		__BIT(11)	/* Write Protected */
#define	PSR51_I		__BIT(10)	/* Invalid */
#define	PSR51_M		__BIT(9)	/* Modified */
#define	PSR51_G		__BIT(8)	/* Gate */
#define	PSR51_C		__BIT(7)	/* Globally Sharable */
#define	PSR51_N		__BITS(0,2)	/* Number of levels */

#ifdef _KERNEL
extern unsigned int protorp[2];

void	mmu_load_urp51(paddr_t);
void	mmu_load_urp20hp(paddr_t);	/* for convenience */
#endif /* _KERNEL */

#endif /* _M68K_MMU_51_H_ */
