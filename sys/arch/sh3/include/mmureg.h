/*	$NetBSD: mmureg.h,v 1.6 2002/02/11 18:03:48 uch Exp $	*/

/*-
 * Copyright (C) 2002 UCHIYAMA Yasushi.  All rights reserved.
 * Copyright (C) 1999 SAITOH Masanobu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH3_MMUREG_H__
#define _SH3_MMUREG_H__

#ifdef _KERNEL
/*
 * MMU
 */

#if !defined(SH4)

/* SH3 definitions */

#define SHREG_PTEH	(*(volatile unsigned long *)	0xFFFFFFF0)
#define SHREG_PTEL	(*(volatile unsigned long *)	0xFFFFFFF4)
#define SHREG_TTB	(*(volatile unsigned long *)	0xFFFFFFF8)
#define SHREG_TEA	(*(volatile unsigned long *)	0xFFFFFFFC)
#define SHREG_MMUCR	(*(volatile unsigned long *)	0xFFFFFFE0)

#else /* !SH4 */

/* SH4 definitions */

#define SHREG_PTEH	(*(volatile unsigned long *)	0xff000000)
#define SHREG_PTEL	(*(volatile unsigned long *)	0xff000004)
#define SHREG_PTEA	(*(volatile unsigned long *)	0xff000034)
#define SHREG_TTB	(*(volatile unsigned long *)	0xff000008)
#define SHREG_TEA	(*(volatile unsigned long *)	0xff00000c)
#define SHREG_MMUCR	(*(volatile unsigned long *)	0xff000010)

#endif /* !SH4 */

#if !defined(SH4)

/* SH3 definitions */
#define MMUCR_AT	0x0001	/* address traslation enable */
#define MMUCR_IX	0x0002	/* index mode */
#define MMUCR_TF	0x0004	/* TLB flush */
#define MMUCR_SV	0x0100	/* single virtual space mode */

#else /* !SH4 */

/* SH4 definitions */
#define MMUCR_AT	0x0001	/* address traslation enable */
#define MMUCR_TI	0x0004	/* TLB Invaliate */
#define MMUCR_SV	0x0100	/* single virtual space mode */
#define MMUCR_SQMD	0x0200  /* Store Queue mode */

#define MMUCR_VALIDBITS 0xfcfcff05	/* XXX */

/* alias */
#define MMUCR_TF	MMUCR_TI

#endif /* !SH4 */

extern int PageDirReg;

/*
 * name-space free version.
 */
/*
 *	SH3
 */
/* 128-entry 4-way set-associative */
#define SH3_MMU_WAY			4
#define SH3_MMU_ENTRY			32

#define SH3_PTEH			0xfffffff0
#define   SH3_PTEH_ASID_MASK		  0x0000000f
#define   SH3_PTEH_VPN_MASK		  0xfffffc00
#define SH3_PTEL			0xfffffff4
#define   SH3_PTEL_HWBITS		  0x1ffff17e /* [28:12][8][6:1] */
#define SH3_TTB				0xfffffff8
#define SH3_TEA				0xfffffffc
#define SH3_MMUCR			0xffffffe0
#define   SH3_MMUCR_AT			  0x00000001
#define   SH3_MMUCR_IX			  0x00000002
#define   SH3_MMUCR_TF			  0x00000004
#define   SH3_MMUCR_RC			  0x00000030
#define   SH3_MMUCR_SV			  0x00000100

/* 
 * memory-mapped TLB 
 */
/* Address array */
#define SH3_MMUAA			0xf2000000
/* address specification */
#define   SH3_MMU_VPN_SHIFT		  12
#define   SH3_MMU_VPN_MASK		  0x0001f000	/* [16:12] */
#define   SH3_MMU_WAY_SHIFT		  8
#define   SH3_MMU_WAY_MASK		  0x00000300
/* data specification */
#define   SH3_MMU_D_VALID		  0x00000100
#define   SH3_MMUAA_D_VPN_MASK		  0xfffe0c00	/* [31:17][11:10] */
#define   SH3_MMUAA_D_ASID_MASK		  0x0000000f

/* Data array */
#define SH3_MMUDA			0xf3000000
#define   SH3_MMUDA_D_PPN_MASK		  0xfffffc00
#define   SH3_MMUDA_D_V			  0x00000100
#define   SH3_MMUDA_D_PR_SHIFT		  5
#define   SH3_MMUDA_D_PR_MASK		  0x00000060	/* [6:5] */
#define   SH3_MMUDA_D_SZ		  0x00000010
#define   SH3_MMUDA_D_C			  0x00000008
#define   SH3_MMUDA_D_D			  0x00000004
#define   SH3_MMUDA_D_SH		  0x00000002

/*
 *	SH4
 */
/* ITLB 4-entry full-associative UTLB 64-entry full-associative */
#define SH4_PTEH			0xff000000
#define   SH4_PTEH_ASID_MASK		  0x0000000f
#define SH4_PTEL			0xff000004
#define   SH4_PTEL_WT			  0x00000001
#define   SH4_PTEL_SH			  0x00000002
#define   SH4_PTEL_D			  0x00000004
#define   SH4_PTEL_C			  0x00000008
#define   SH4_PTEL_PR_SHIFT		  5
#define   SH4_PTEL_PR_MASK		  0x00000060	/* [5:6] */
#define   SH4_PTEL_SZ_MASK		  0x00000090	/* [4][7] */
#define     SH4_PTEL_SZ_1K		  0x00000000
#define     SH4_PTEL_SZ_4K		  0x00000010
#define     SH4_PTEL_SZ_64K		  0x00000080
#define     SH4_PTEL_SZ_1M		  0x00000090
#define   SH4_PTEL_V			  0x00000100
#define   SH4_PTEL_HWBITS		  0x1ffff1ff /* [28:12]PFN [8:0]attr. */

#define SH4_PTEA			0xff000034
#define   SH4_PTEA_SA_MASK		  0x00000007
#define   SH4_PTEA_SA_TC		  0x00000008
#define SH4_TTB				0xff000008
#define SH4_TTA				0xff00000c
#define SH4_MMUCR			0xff000010
#define   SH4_MMUCR_AT			  0x00000001
#define   SH4_MMUCR_TI			  0x00000004
#define   SH4_MMUCR_SV			  0x00000100
#define   SH4_MMUCR_SQMD		  0x00000200
#define   SH4_MMUCR_URC_SHIFT		  10
#define   SH4_MMUCR_URC_MASK		  0x0000fc00	/* [10:15] */
#define   SH4_MMUCR_URB_SHIFT		  18
#define   SH4_MMUCR_URB_MASK		  0x00fc0000	/* [18:23] */
#define   SH4_MMUCR_LRUI_SHIFT		  26
#define   SH4_MMUCR_LRUT_MASK		  0xfc000000	/* [26:31] */
/* 
 * memory-mapped TLB 
 *	must be access from P2-area program.
 *	branch to the other area must be maed at least 8 instruction
 *	after access.
 */
/* ITLB */
#define SH4_ITLB_AA			0xf2000000
/* address specification (common for address and data array(0,1)) */
#define   SH4_ITLB_E_SHIFT		  8
#define   SH4_ITLB_E_MASK		  0x00000300	/* [9:8] */
/* data specification */
/* address-array */
#define   SH4_ITLB_AA_ASID_MASK		  0x000000ff	/* [7:0] */
#define   SH4_ITLB_AA_V			  0x00000100
#define   SH4_ITLB_AA_VPN_SHIFT		  10
#define   SH4_ITLB_AA_VPN_MASK		  0xfffffc00	/* [31:10] */
/* data-array 1 */
#define SH4_ITLB_DA1			0xf3000000
#define   SH4_ITLB_DA1_SH		  0x00000002
#define   SH4_ITLB_DA1_C		  0x00000008
#define   SH4_ITLB_DA1_SZ_MASK		  0x00000090	/* [7][4] */
#define     SH4_ITLB_DA1_SZ_1K		  0x00000000
#define     SH4_ITLB_DA1_SZ_4K		  0x00000010
#define     SH4_ITLB_DA1_SZ_64K		  0x00000080
#define     SH4_ITLB_DA1_SZ_1M		  0x00000090
#define   SH4_ITLB_DA1_PR		  0x00000040
#define   SH4_ITLB_DA1_V		  0x00000100
#define   SH4_ITLB_DA1_PPN_SHIFT	  11
#define   SH4_ITLB_DA1_PPN_MASK		  0x1ffffc00	/* [28:10] */
/* data-array 2 */
#define SH4_ITLB_DA2			0xf3800000
#define   SH4_ITLB_DA2_SA_MASK		  0x00000003
#define   SH4_ITLB_DA2_TC		  0x00000004

/* UTLB */
#define SH4_UTLB_AA			0xf6000000
/* address specification (common for address and data array(0,1)) */
#define   SH4_UTLB_E_SHIFT		  8
#define   SH4_UTLB_E_MASK		  0x00003f00
/* data specification */
/* address-array */
#define   SH4_UTLB_AA_VPN_MASK		  0xfffffc00	/* [31:10] */
#define   SH4_UTLB_AA_D			  0x00000200
#define   SH4_UTLB_AA_V			  0x00000100
#define   SH4_UTLB_AA_ASID_MASK		  0x000000ff	/* [7:0] */
/* data-array 1 */
#define SH4_UTLB_DA1			0xf7000000
#define   SH4_UTLB_DA1_WT		  0x00000001
#define   SH4_UTLB_DA1_SH		  0x00000002
#define   SH4_UTLB_DA1_D		  0x00000004
#define   SH4_UTLB_DA1_C		  0x00000008
#define   SH4_UTLB_DA1_SZ_MASK		  0x00000090	/* [7][4] */
#define     SH4_UTLB_DA1_SZ_1K		  0x00000000
#define     SH4_UTLB_DA1_SZ_4K		  0x00000010
#define     SH4_UTLB_DA1_SZ_64K		  0x00000080
#define     SH4_UTLB_DA1_SZ_1M		  0x00000090
#define   SH4_UTLB_DA1_PR_SHIFT		  5
#define   SH4_UTLB_DA1_PR_MASK		  0x00000060
#define   SH4_UTLB_DA1_V		  0x00000100
#define   SH4_UTLB_DA1_PPN_SHIFT	  11
#define   SH4_UTLB_DA1_PPN_MASK		  0x1ffffc00	/* [28:10] */
/* data-array 2 */
#define SH4_UTLB_DA2			0xf7800000
#define   SH4_UTLB_DA2_SA_MASK		  0x00000003
#define   SH4_UTLB_DA2_TC		  0x00000004

#endif /* _KERNEL */
#endif /* !_SH3_MMUREG_H__ */
