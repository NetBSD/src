/*	$NetBSD: pte.h,v 1.5 2003/04/02 07:36:03 thorpej Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * SH5 Page Table Entry
 */

#ifndef _SH5_PTE_H
#define	_SH5_PTE_H

/*
 * The size of the in-core copy of the PTE registers depends on the
 * number of valid effective address bits.
 * This is purely a memory-saving exercise.
 */
#if SH5_NEFF_BITS > 32
typedef	u_int64_t	pteh_t;
typedef	u_int64_t	ptel_t;
#else
typedef	u_int32_t	pteh_t;
typedef	u_int32_t	ptel_t;
#endif

typedef u_int16_t	vsid_t;
typedef u_int16_t	tlbcookie_t;

/*
 * The in-core PTE for user mappings
 *
 * XXX: Don't change the ordering of this structure. XXX
 */
typedef struct pte {
	volatile ptel_t	ptel;
	volatile tlbcookie_t tlbcookie;
	vsid_t	vsid;
	pteh_t	pteh;
} pte_t;

/*
 * The in-core PTE for kernel (KSEG1) mappings.
 *
 * Note: This structure must match the first two items of the userland
 * PTE structure.
 *
 * Note#2: Use pmap_kernel_ipt_{set,get}_{ptel,tlbcookie}() to access
 * members of the following structure. The compiler generates lousy
 * code otherwise.
 */
typedef struct kpte {
	volatile ptel_t ptel;
	volatile tlbcookie_t tlbcookie;
} __attribute__ ((__packed__)) kpte_t;

/*
 * Hardware fields in a pteh_t
 *
 * This is what a PTEH looks like when it is in the TLB.
 * Note that the pmap module uses the ASID field to store metadata
 * for in-core copies of PTEHs.
 */
#define	SH5_PTEH_V		(1 << 0)   /* Valid */
#define	SH5_PTEH_SH		(1 << 1)   /* Shared mapping */
#define	SH5_PTEH_ASID_SHIFT	2
#define	SH5_PTEH_ASID_SIZE	(1 << SH5_ASID_BITS)
#define	SH5_PTEH_ASID_MASK	(SH5_PTEH_ASID_SIZE - 1)
#define	SH5_PTEH_EPN_SHIFT	12
#if SH5_NEFF_BITS == 32
#define	SH5_PTEH_EPN_MASK	0xfffff000UL
#else
#define	SH5_PTEH_EPN_MASK	0xfffffffffffff000UL
#endif

/*
 * Hardware fields in a ptel_t
 *
 * This is what a PTEH looks like when it is in the TLB.
 */
#define	SH5_PTEL_CB_MASK	0x3
#define	SH5_PTEL_CB_NOCACHE	0x0
#define	SH5_PTEL_CB_DEVICE	0x1
#define	SH5_PTEL_CB_WRITEBACK	0x2
#define	SH5_PTEL_CB_WRITETHRU	0x3
#define	SH5_PTEL_CACHEABLE(p)	(((p)&SH5_PTEL_CB_MASK)>=SH5_PTEL_CB_WRITEBACK)

#define	SH5_PTEL_SZ_MASK	0x18
#define	SH5_PTEL_SZ_4KB		0x00
#define	SH5_PTEL_SZ_64KB	0x08
#define	SH5_PTEL_SZ_1MB		0x10
#define	SH5_PTEL_SZ_512MB	0x18

#define	SH5_PTEL_PR_MASK	0x1e0
#define	SH5_PTEL_PR_R		0x040
#define	SH5_PTEL_PR_R_SHIFT	6
#define	SH5_PTEL_PR_X		0x080
#define	SH5_PTEL_PR_X_SHIFT	7
#define	SH5_PTEL_PR_W		0x100
#define	SH5_PTEL_PR_W_SHIFT	8
#define	SH5_PTEL_PR_U		0x200
#define	SH5_PTEL_PR_U_SHIFT	9

#if SH5_NEFF_BITS == 32
#define	SH5_PTEL_PPN_MASK	0xfffff000UL
#else
#define	SH5_PTEL_PPN_MASK	0xfffffffffffff000UL
#endif

#define	SH5_PTE_PN_MASK_MOVI	-4096	/* movi only accepts signed 16-bit #s */


/*
 * Software bits stored in some low-order bits of an in-core PTEL
 */
#define	SH5_PTEL_R		0x400	/* Set when the mapping is referenced */
#define	SH5_PTEL_M		0x800	/* Set when the mapping is modified */
#define	SH5_PTEL_RM_MASK	0xc00


/*
 * The pmap_pteg_table consists of an array of Hash Buckets, called PTE Groups,
 * where each group is 8 PTEs in size. The number of groups is calculated
 * at boot time such that there is one group for every two PAGE_SIZE-sized pages
 * of physical RAM.
 */
#define	SH5_PTEG_SIZE		8
typedef struct {
	volatile pte_t	pte[SH5_PTEG_SIZE];
} pteg_t;

#endif /* _SH5_PTE_H */
