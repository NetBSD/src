/* -*-C++-*-	$NetBSD: sh4.h,v 1.1.2.2 2002/02/28 04:09:47 nathanw Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#ifndef _HPCBOOT_SH_CPU_SH4_H_
#define _HPCBOOT_SH_CPU_SH4_H_
#include <sh3/cpu/sh.h>

/*
 * SH4 designed for Windows CE (SH7750) common defines.
 */

#define SH4_TRA			0xff000020
#define SH4_EXPEVT		0xff000024
#define SH4_INTEVT		0xff000028

#define SH4_ICR			0xffd00000
#define SH4_IPRA		0xffd00004
#define SH4_IPRB		0xffd00008
#define SH4_IPRC		0xffd0000c
#define SH4_IPRD		0xffd00010

/* Windows CE uses 1Kbyte page for SH3, 4Kbyte for SH4 */
#define SH4_PAGE_SIZE		0x1000
#define SH4_PAGE_MASK		(~(SH4_PAGE_SIZE - 1))

/*
 * Cache
 */
#define SH4_ICACHE_SIZE		8192
#define SH4_DCACHE_SIZE		16384
#define SH4_CACHE_LINESZ	32

#define SH4_CCR			0xff00001c
#define   SH4_CCR_IIX		  0x00008000
#define   SH4_CCR_ICI		  0x00000800
#define   SH4_CCR_ICE		  0x00000100
#define   SH4_CCR_OIX		  0x00000080
#define   SH4_CCR_ORA		  0x00000020
#define   SH4_CCR_OCI		  0x00000008
#define   SH4_CCR_CB		  0x00000004
#define   SH4_CCR_WT		  0x00000002
#define   SH4_CCR_OCE		  0x00000001

#define SH4_QACR0		0xff000038
#define SH4_QACR1		0xff00003c
#define   SH4_QACR_AREA_SHIFT	  2
#define   SH4_QACR_AREA_MASK	  0x0000001c

/* I-cache address/data array  */
#define SH4REG_CCIA		0xf0000000
/* address specification */
#define   CCIA_A		  0x00000008	/* associate bit */
#define   CCIA_ENTRY_SHIFT	  5		/* line size 32B */
#define   CCIA_ENTRY_MASK	  0x00001fe0	/* [12:5] 256-entries */
/* data specification */
#define   CCIA_V		  0x00000001
#define   CCIA_TAGADDR_MASK	  0xfffffc00	/* [31:10] */

#define SH4REG_CCID		0xf1000000
/* address specification */
#define   CCID_L_SHIFT		  2
#define   CCID_L_MASK		  0x1c		/* line-size is 32B */
#define   CCID_ENTRY_MASK	  0x00001fe0	/* [12:5] 128-entries */

/* D-cache address/data array  */
#define SH4REG_CCDA		0xf4000000
/* address specification */
#define   CCDA_A		  0x00000008	/* associate bit */
#define   CCDA_ENTRY_SHIFT	  5		/* line size 32B */
#define   CCDA_ENTRY_MASK	  0x00003fe0	/* [13:5] 512-entries */
/* data specification */
#define   CCDA_V		  0x00000001
#define   CCDA_U		  0x00000002
#define   CCDA_TAGADDR_MASK	  0xfffffc00	/* [31:10] */

#define SH4REG_CCDD		0xf5000000

/*
 * MMU
 */
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

#define SH4_MMU_DISABLE()	_reg_write_4(SH4_MMUCR, SH4_MMUCR_TI)

/* 
 * Product dependent headers
 */
#include <sh3/cpu/7750.h>

#endif /* _HPCBOOT_SH_CPU_SH4_H_ */
