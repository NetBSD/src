/*	$NetBSD: mc68851.h,v 1.5 1997/05/14 01:37:23 jeremy Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeremy Cooper.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * This file contains the machine-independent definitions
 * related to the Motorola MC68851 Memory Management Unit (MMU).
 * Things that depend on the contents of the Translation Control
 * (TC) register are in <machine/pte.h>.
 */

#ifndef _SUN3X_MC68851_H
#define _SUN3X_MC68851_H

/**************************** MMU STRUCTURES ****************************
 * MMU structures define the format of data used by the MC68851.        *
 ************************************************************************
 ** MC68851 Root Pointer
 * All address translations begin with the examination of the value
 * in the MC68851 Root Pointer register.  It describes the base address
 * (in physical memory) of the root table to be used as well as any limits
 * to the address range it supports.  Its structure is identical to a Long
 * Format Table Descriptor (described below.)
 */
struct mmu_rootptr {
	u_long	rp_attr;	/* Lower/Upper Limit and access flags */
	u_long	rp_addr;	/* Physical Base Address */
};
typedef struct mmu_rootptr mmu_rootptr_t;


/** MC68851 Long Format Table Descriptor
 * The root table for a sun3x pmap is a 128 element array of 'long format
 * table descriptors'.  The structure of a long format table descriptor is:
 *
 *  63                                                             48
 *  +---+---.---.---.---.---.---.---.---.---.---.---.---.---.---.---+
 *  |L/U|                 LIMIT                                     |
 *  +---+---.---+---.---.---+---+---+---+---+---+---+---+---+---.---+
 *  |    RAL    |    WAL    |SG | S | 0 | 0 | 0 | 0 | U |WP |DT (10)|
 *  +---.---.---+---.---.---+---+---+---+---+---+---+---+---+---.---+
 *  |              TABLE PHYSICAL ADDRESS (BITS 31-16)              |
 *  +---.---.---.---.---.---.---.---.---.---.---.---+---.---.---.---+
 *  |       TABLE PHYSICAL ADDRESS (15-4)           |     UNUSED    |
 *  +---.---.---.---.---.---.---.---.---.---.---.---+---.---.---.---+
 *  15                                                              0
 *
 * Note: keep the unused bits set to zero so that no masking of the
 *       base address is needed.
 */
struct mmu_long_dte_struct { /* 'dte' stands for 'descriptor table entry' */
	union {
		struct {
			char	lu_flag:1;	/* Lower/Upper Limit flag */
			int	limit:15;	/* Table Size limit */
			char	ral:3;		/* Read Access Level */
			char	wal:3;		/* Write Access Level */
			char	sg:1;		/* Shared Globally flag */
			char	supv:1;		/* Supervisor Only flag */
			char	rsvd:4;		/* Reserved (All zeros) */
			char	u:1;		/* Used flag */
			char	wp:1;		/* Write Protect flag */
			char	dt:2;		/* Descriptor Type */
			/* Bit masks for fields above */
#define			MMU_LONG_DTE_LU    0x80000000
#define			MMU_LONG_DTE_LIMIT 0x7fff0000 
#define			MMU_LONG_DTE_RAL   0x0000e000
#define			MMU_LONG_DTE_WAL   0x00001c00
#define			MMU_LONG_DTE_SG    0x00000200
#define			MMU_LONG_DTE_SUPV  0x00000100
#define			MMU_LONG_DTE_USED  0x00000008
#define			MMU_LONG_DTE_WP    0x00000004
#define			MMU_LONG_DTE_DT    0x00000003
		} attr_struct;
		u_long raw; /* struct above, addressable as a long */
	} attr;
	union	{
		struct {
			int	base_addr:28;	/* Physical base address
			char	unused:4;	 * of the table pointed to
						 * by this entry.
						 */
			/* Bit masks for fields above */
#define			MMU_LONG_DTE_BASEADDR   0xfffffff0
		} addr_struct;
		u_long	raw; /* struct above, addressable as a long */
	} addr;
};
typedef struct mmu_long_dte_struct mmu_long_dte_t;
typedef struct mmu_long_dte_struct *mmu_long_dtbl_t;

/** MC68851 Long Format Page Descriptor
 * Although not likely to be used in this implementation, a level
 * 'A' table may contain long format PAGE descriptors.  A long format
 * page descriptor is the same size as a long format table descriptor.
 * Its discriminating feature to the MMU is its descriptor field: 01.
 *  63                                                             48
 *  +---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---+
 *  |                          UNUSED                               |
 *  +---.---.---+---.---.---+---+---+---+---+---+---+---+---+---.---+
 *  |    RAL    |    WAL    |SG | S | G |CI | L | M | U |WP |DT (01)|
 *  +---.---.---+---.---.---+---+---+---+---+---+---+---+---+---.---+  
 *  |              TABLE PHYSICAL ADDRESS (BITS 31-16)              |
 *  +---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---+
 *  |TABLE PHYS. ADDRESS (15-8)     |            UNUSED             |
 *  +---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---+
 *  15                                                              0
 */
struct mmu_long_pte_struct { /* 'pte' stands for 'page table entry' */
	union {
		struct {
			int	unused:16;	/* Unused */
			char	ral:3;		/* Read Access Level */
			char	wal:3;		/* Write Access Level */
			char	sg:1;		/* Shared Globally flag */
			char	supv:1;		/* Supervisor Only flag */
			char	g:1;		/* Gate allowed */
			char	ci:1;		/* Cache inhibit */
			char	l:1;		/* Lock entry */
			char	m:1;		/* Modified flag */
			char	u:1;		/* Used flag */
			char	wp:1;		/* Write Protect flag */
			char	dt:2;		/* Descriptor Type */
			/* Bit masks for fields above */
#define			MMU_LONG_PTE_RAL   0x0000e000
#define			MMU_LONG_PTE_WAL   0x00001c00
#define			MMU_LONG_PTE_SG    0x00000200
#define			MMU_LONG_PTE_SUPV  0x00000100
#define			MMU_LONG_PTE_GATE  0x00000080
#define			MMU_LONG_PTE_CI    0x00000040
#define			MMU_LONG_PTE_LOCK  0x00000020
#define			MMU_LONG_PTE_M     0x00000010
#define			MMU_LONG_PTE_USED  0x00000008
#define			MMU_LONG_PTE_WP    0x00000004
#define			MMU_LONG_PTE_DT    0x00000003
		} attr_struct;
		u_long	raw; /* struct above, addressable as a long */
	} attr;
	union	{
		struct {
			long	base_addr:24;	/* Physical base address
			char	unused:8;	 * of page this entry
						 * points to.
						 */
			/* Bit masks for fields above */
#define			MMU_LONG_PTE_BASEADDR   0xffffff00
		} addr_struct;
		u_long	raw; /* struct above, addressable as a long */
	} addr;
};
typedef struct mmu_long_pte_struct mmu_long_pte_t;
typedef struct mmu_long_pte_struct *mmu_long_ptbl_t;

/* Every entry in the level A table (except for the page entries  
 * described above) points to a level B table.  Level B tables are 
 * arrays of 'short format' table descriptors.  Their structure 
 * is smaller than an A table descriptor and is as follows:
 *
 * 31                                                             16
 * +---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---+
 * |           TABLE PHYSICAL BASE ADDRESS (BITS 31-16)            |
 * +---.---.---.---.---.---.---.---.---.---.---.---+---+---+---.---+
 * | TABLE PHYSICAL BASE ADDRESS (15-4)            | U |WP |DT (10)|
 * +---.---.---.---.---.---.---.---.---.---.---.---+---+---+---.---+
 * 15                                                              0
 */
struct mmu_short_dte_struct { /* 'dte' stands for 'descriptor table entry' */
	union {
		struct {
			long	base_addr:28;
			char	u:1;
			char	wp:1;
			char	dt:2;
#define			MMU_SHORT_DTE_BASEADDR	0xfffffff0
#define			MMU_SHORT_DTE_USED	0x00000008
#define			MMU_SHORT_DTE_WP	0x00000004
#define			MMU_SHORT_DTE_DT	0x00000003
		} attr_struct;
		u_long	raw;
	} attr;
};
typedef struct mmu_short_dte_struct mmu_short_dte_t;
typedef struct mmu_short_dte_struct *mmu_short_dtbl_t;

/* Every entry in a level B table points to a level C table.  Level C tables
 * contain arrays of short format page 'entry' descriptors.  A short format
 * page 'entry' is the same size as a short format page 'table' 
 * descriptor (a B table entry).  Thus B and C tables can be allocated
 * interchangeably from the same pool.  However, we will keep them separate.
 *
 * The descriptor type (DT) field of a Page Table Entry (PTE) is '01'. This 
 * indicates to the MMU that the address contained in the PTE's 'base 
 * address' field is the base address for a physical page in memory to which 
 * the VA should be mapped, and not a base address for a yet another 
 * descriptor table, thus ending the table walk.
 *
 * 31                                                             16
 * +---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---+
 * |           TABLE PHYSICAL BASE ADDRESS (BITS 31-16)            |
 * +---.---.---.---.---.---.---.---+---+---+---+---+---+---+---.---+
 * |TABLE PHYS. BASE ADDRESS (15-8)| G |CI | L | M | U |WP |DT (10)|
 * +---.---.---.---.---.---.---.---+---+---+---+---+---+---+---.---+
 * 15                                                              0
 */
struct mmu_short_pte_struct { /* 'pte' stands for 'page table entry' */
	union {
		struct {
			long	base_addr:24;
			char	g:1;
			char	ci:1;
			char	l:1;
			char	m:1;
			char	u:1;
			char	wp:1;
			char	dt:2;
#define			MMU_SHORT_PTE_BASEADDR 0xffffff00
#define			MMU_SHORT_PTE_UN2      0x00000080
#define			MMU_SHORT_PTE_CI       0x00000040
#define			MMU_SHORT_PTE_UN1      0x00000020
#define			MMU_SHORT_PTE_M        0x00000010
#define			MMU_SHORT_PTE_USED     0x00000008
#define			MMU_SHORT_PTE_WP       0x00000004
#define			MMU_SHORT_PTE_DT       0x00000003
		} attr_struct;
		u_long raw;
	} attr;
};
typedef struct mmu_short_pte_struct mmu_short_pte_t;
typedef struct mmu_short_pte_struct *mmu_short_ptbl_t;

/* These are bit masks and other values that are common to all types of 
 * descriptors.
 */
/* Page table descriptors have a 'Descriptor Type' field describing the
 * format of the tables they point to.  It is two bits wide and is one of:
 */
#define MMU_DT_INVALID	0x0 /* Invalid descriptor entry            */
#define MMU_DT_PAGE	0x1 /* Descriptor describes a page entry   */
#define MMU_DT_SHORT	0x2 /*   describes a short format table    */
#define MMU_DT_LONG	0x3 /*   describes a long format table     */
#define MMU_DT_MASK	0x00000003 /* Bit location of the DT field */

/* Various macros for manipulating and setting MMU descriptor
 * characteristics.
 */
/* returns true if a descriptor is valid. */
#define MMU_VALID_DT(dte)	((dte).attr.raw & MMU_DT_MASK)
/* returns true if a descriptor is invalid */
#define	MMU_INVALID_DT(dte)	(!((dte).attr.raw & MMU_DT_MASK))
/* returns true if a descriptor has been referenced */
#define MMU_PTE_USED(pte)	((pte).attr.raw & MMU_SHORT_PTE_USED)
/* returns true if a descriptor has been modified */
#define MMU_PTE_MODIFIED(pte)	((pte).attr.raw & MMU_SHORT_PTE_M)
/* extracts the physical address from a pte */
#define MMU_PTE_PA(pte)		((pte).attr.raw & MMU_SHORT_PTE_BASEADDR)
/* extracts the physical address from a dte */
#define MMU_DTE_PA(dte)		((dte).attr.raw & MMU_SHORT_DTE_BASEADDR)

#endif /* _SUN3X_MC68851_H */
