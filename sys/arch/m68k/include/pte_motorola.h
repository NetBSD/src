/*	$NetBSD: pte_motorola.h,v 1.10 2024/01/01 22:47:58 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 * from: Utah $Hdr: pte.h 1.13 92/01/20$
 *
 *	@(#)pte.h	8.1 (Berkeley) 6/10/93
 */

#ifndef	_MACHINE_PTE_H_
#define	_MACHINE_PTE_H_

#include <m68k/mmu_51.h>
#include <m68k/mmu_40.h>

/*
 * m68k motorola MMU segment/page table entries
 */

typedef u_int	st_entry_t;	/* segment table entry */
typedef u_int	pt_entry_t;	/* page table entry */

#define	PT_ENTRY_NULL	NULL
#define	ST_ENTRY_NULL	NULL

#define PG_SHIFT	PGSHIFT

/*
 * "Segment" Table Entry bits, defined in terms of the 68851 bits
 * (compatible 68040 bits noted in comments).
 */
#define	SG_V		DT51_SHORT	/* == UTE40_RESIDENT */
#define	SG_NV		DT51_INVALID	/* == UTE40_INVALID */
#define	SG_RO		DTE51_WP	/* == UTE40_W */
#define	SG_RW		0x00000000
#define	SG_PROT		DTE51_WP
#define	SG_U		DTE51_U		/* == UTE40_U */
#define	SG_FRAME	((~0U) << PG_SHIFT)
#define	SG_ISHIFT	((PG_SHIFT << 1) - 2)	/* 24 or 22 */
#define	SG_IMASK	((~0U) << SG_ISHIFT)
#define	SG_PSHIFT	PG_SHIFT
#define	SG_PMASK	(((~0U) << SG_PSHIFT) & ~SG_IMASK)

/* 68040 additions */
#define	SG4_MASK1	0xfe000000U
#define	SG4_SHIFT1	25
#define	SG4_MASK2	0x01fc0000U
#define	SG4_SHIFT2	18
#define	SG4_MASK3	(((~0U) << PG_SHIFT) & ~(SG4_MASK1 | SG4_MASK2))
#define	SG4_SHIFT3	PG_SHIFT
#define	SG4_ADDR1	0xfffffe00
#define	SG4_ADDR2	((~0U) << (20 - PG_SHIFT))
#define	SG4_LEV1SIZE	128
#define	SG4_LEV2SIZE	128
#define	SG4_LEV3SIZE	(1U << (SG4_SHIFT2 - PG_SHIFT))	/* 64 or 32 */

/*
 * Page Table Entry bits, defined in terms of the 68851 bits
 * (compatible 68040 bits noted in comments).
 */
#define	PG_V		DT51_PAGE	/* == PTE40_RESIDENT */
#define	PG_NV		DT51_INVALID	/* == PTE40_INVALID */
#define	PG_RO		PTE51_WP	/* == PTE40_W */
#define	PG_RW		0x00000000
#define	PG_PROT		PG_RO
#define	PG_U		PTE51_U		/* == PTE40_U */
#define	PG_M		PTE51_M		/* == PTE40_M */
#define	PG_CI		PTE51_CI
#define	PG_W		__BIT(8)	/* 851 unused bit XXX040 PTE40_U0 */
#define	PG_FRAME	((~0U) << PG_SHIFT)
#define	PG_PFNUM(x)	(((uintptr_t)(x) & PG_FRAME) >> PG_SHIFT)

/* 68040 additions */
#define	PG_CMASK	PTE40_CM	/* cache mode mask */
#define	PG_CWT		PTE40_CM_WT	/* writethrough caching */
#define	PG_CCB		PTE40_CM_CB	/* copyback caching */
#define	PG_CIS		PTE40_CM_NC_SER	/* cache inhibited serialized */
#define	PG_CIN		PTE40_CM_NC	/* cache inhibited nonserialized */
#define	PG_SO		PTE40_S		/* supervisor only */

#define M68K_STSIZE	(MAXUL2SIZE * SG4_LEV2SIZE * sizeof(st_entry_t))
					/* user process segment table size */
#define M68K_MAX_PTSIZE	 (1U << (32 - PG_SHIFT + 2))	/* max size of UPT */
#define M68K_MAX_KPTSIZE (M68K_MAX_PTSIZE >> 2)	/* max memory to allocate to KPT */
#define M68K_PTBASE	0x10000000	/* UPT map base address */
#define M68K_PTMAXSIZE	0x70000000	/* UPT map maximum size */

/*
 * Kernel virtual address to page table entry and to physical address.
 */

#ifdef cesfic
#define	kvtopte(va) \
	(&Sysmap[((unsigned)(va)) >> PGSHIFT])
#else
#define	kvtopte(va) \
	(&Sysmap[((unsigned)(va) - VM_MIN_KERNEL_ADDRESS) >> PGSHIFT])
#endif

#endif /* !_MACHINE_PTE_H_ */
