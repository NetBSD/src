/* -*-C++-*-	$NetBSD: sh3.h,v 1.1 2002/02/11 17:08:58 uch Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#ifndef _HPCBOOT_SH_CPU_SH3_H_
#define _HPCBOOT_SH_CPU_SH3_H_
#include <sh3/cpu/sh.h>

/*
 * SH3 designed for Windows CE (SH7709, SH7709A) common defines.
 */
#define SH3_TRA			0xffffffd0
#define SH3_EXPEVT		0xffffffd4
#define SH3_INTEVT		0xffffffd8
#define SH3_INTEVT2		0xa4000000

/* Windows CE uses 1Kbyte page for SH3, 4Kbyte for SH4 */
#define SH3_PAGE_SIZE		0x400
#define SH3_PAGE_MASK		(~(SH3_PAGE_SIZE - 1))

/* 
 * Cache (Windows CE uses normal-mode.)
 */
#define SH3_CCR			0xffffffec
#define SH3_CCR_CE		  0x00000001
#define SH3_CCR_WT		  0x00000002
#define SH3_CCR_CB		  0x00000004
#define SH3_CCR_CF		  0x00000008
#define SH3_CCR_RA		  0x00000020
#define SH3_CCA			0xf0000000
#define SH3_CCD			0xf1000000

/* 
 * 4-way set-associative 32-entry (total 128 entries) 
 */
#define SH3_MMU_WAY			4
#define SH3_MMU_ENTRY			32

#define SH3_PTEH			0xfffffff0
#define   SH3_PTEH_ASID_MASK		  0x0000000f
#define   SH3_PTEH_VPN_MASK		  0xfffffc00
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


#define SH3_MMU_DISABLE()	_reg_write_4(SH3_MMUCR, SH3_MMUCR_TF)

/* 
 * Product dependent headers
 */
#include <sh3/cpu/7707.h>
#include <sh3/cpu/7709.h>
#include <sh3/cpu/7709a.h>

#endif // _HPCBOOT_SH_CPU_SH3_H_
