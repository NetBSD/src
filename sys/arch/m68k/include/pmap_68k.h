/*	$NetBSD: pmap_68k.h,v 1.5 2025/11/12 03:34:58 thorpej Exp $	*/

/*-     
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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

/* 
 * Copyright (c) 1987 Carnegie-Mellon University
 * Copyright (c) 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
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
 *      @(#)pmap.h      8.1 (Berkeley) 6/10/93
 */   

#ifndef _M68K_PMAP_68K_H_
#define	_M68K_PMAP_68K_H_

#include <sys/rbtree.h>
#include <sys/queue.h>

#include <m68k/mmu_51.h>
#include <m68k/mmu_40.h>

#include <sys/kcore.h>
#include <m68k/kcore.h>

typedef unsigned int	pt_entry_t;

TAILQ_HEAD(pmap_ptpage_list, pmap_ptpage);
LIST_HEAD(pmap_pv_list, pv_entry);

struct pmap {
	struct pmap_table *pm_lev1map;	/* level 1 table */
	paddr_t            pm_lev1pa;	/* PA of level 1 table */
	unsigned int       pm_refcnt;	/* reference count */

	struct pmap_table *pm_pt_cache;	/* most recently used leaf table */

	/* Red-Black tree that contains the active tables. */
	struct rb_tree     pm_tables;	/* lev1map not in here */

	/* Page table pages for segment and leaf tables. */
	struct pmap_ptpage_list pm_ptpages[2];

	struct pmap_pv_list pm_pvlist;	/* all associated P->V entries */

	struct pmap_statistics pm_stats;/* statistics */
};

/*
 * One entry per P->V mapping of a managed page.
 *
 * N.B. We want to keep this structure's size to be a multiple of
 * 8; we want to align them to 8 bytes in order to be able to use
 * the lower 3 bits of the pv_entry list head for page attributes.
 */
struct pv_entry {
/* 0*/	struct pv_entry     *pv_next;	/* link on page list */
/* 4*/	LIST_ENTRY(pv_entry) pv_pmlist;	/* link on pmap list */
/*12*/	pmap_t               pv_pmap;	/* pmap that contains mapping */
/*16*/	vaddr_t              pv_vf;	/* virtual address + flags */
/*20*/	struct pmap_table   *pv_pt;	/* table that contains the PTE */
/*24*/
};

/* Upper bits of pv_vf contain the virtual addess */
#define	PV_VA(pv)	((pv)->pv_vf & ~PAGE_MASK)

/* Lower bits of pv_vf contain flags */
#define	PV_F_CI_VAC	__BIT(0)	/* mapping CI due to VAC alias */
#define	PV_F_CI_USR	__BIT(1)	/* mapping CI due to user request */

/*
 * This describes an individual table used by the MMU.  Depending on
 * the MMU configuration, there may be more than one table per physical
 * page.
 *
 * For leaf (page) and inner segment tables, pt_st points to the
 * segment table one level up in the tree that maps it, and pt_stidx
 * is the index into that segment table.  pt_st also serves as a
 * proxy for whether or not the table has been inserted into the
 * table lookup tree.  For the level-1 table, pt_st is NULL and
 * that table is not inserted into the lookup tree.
 */
struct pmap_table {
	struct pmap_ptpage *pt_ptpage;
	pt_entry_t         *pt_entries;
	struct pmap_table  *pt_st;
	unsigned short      pt_holdcnt;
	unsigned short      pt_stidx;
	unsigned int        pt_key;
	union {
		LIST_ENTRY(pmap_table) pt_freelist;
		struct rb_node         pt_node;
	};
};

/*
 * This describes a page table page, which contains one or more MMU tables.
 * It's variable length, and the table descriptors are allocated along with.
 */
struct pmap_ptpage {
	TAILQ_ENTRY(pmap_ptpage)  ptp_list;
	LIST_HEAD(, pmap_table)   ptp_freelist;
	struct vm_page           *ptp_pg;
	unsigned int              ptp_vpagenum : 23,
	                          ptp_freecnt : 8,
	                          ptp_segtab : 1;
	struct pmap_table         ptp_tables[];
};

/*
 * Abstract definitions for PTE bits / fields.  C code will compile-time-
 * assert the equivalencies that we assume.
 *
 * N.B. assumes exclusive use of short descriptors on 68851.
 */
#define	PTE_VALID	PTE40_RESIDENT	/* == DT51_PAGE */
#define	PTE_WP		PTE40_W		/* == PTE51_WP */
#define	PTE_M		PTE40_M		/* == PTE51_M */
#define	PTE_U		PTE40_U		/* == PTE51_U */
#define	PTE_PVLIST	PTE40_G		/* unused on '51, don't use PFLUSHxN */
#define	PTE_WIRED	PTE40_UR	/* unused on '51 */

/*
 * PTE40_CM overlaps with PTE51_CI and PTE51_L (which we don't use).
 */
#define	PTE_CMASK	PTE40_CM

/*
 * Critical bits that, when changed (see pmap_changebit()), require
 * invalidation of the ATC.
 */
#define	PTE_CRIT_BITS	(PTE_WP | PTE_CMASK)

/*
 * Root Pointer attributes for Supervisor and User modes.
 *
 * Supervisor:
 * - No index limit (Lower limit == 0)
 * - Points to Short format descriptor table.
 * - Shared Globally
 *
 * User:
 * - No index limit (Lower limit == 0)
 * - Points to Short format descriptor table.
 */
#define	MMU51_SRP_BITS	(DTE51_LOWER | DTE51_SG | DT51_SHORT)
#define	MMU51_CRP_BITS	(DTE51_LOWER |            DT51_SHORT)

/*
 * Our abstract definition of a "segment" is "that which points to the
 * leaf tables".  On the 2-level configuration, that's the level 1 table,
 * and on the 3-level configuraiton, that's the level 2 table.
 *
 * This is the logical address layout:
 *
 * 2-level 4KB/page: l1,l2,page    == 10,10,12	(HP MMU compatible)
 * 2-level 8KB/page: l1,l2,page    ==  8,11,13
 * 3-level 4KB/page: l1,l2,l3,page == 7,7,6,12
 * 3-level 8KB/page: l1,l2,l3,page == 7,7,5,13
 *
 * The 2-level l2 size is chosen per the number of page table entries
 * per page, to use one whole page for PTEs per one segment table entry.
 *
 * The 3-level layout is defined by the 68040/68060 hardware, and is not
 * configurable (other than chosen page size).  If '851 / '030 chooses
 * to use the 3-level layout, it is specifically configured to be compatible
 * with the 68040.
 */
							/*  8KB /  4KB  */
#define	LA2L_L2_NBITS	(PGSHIFT - 2)			/*   11 /   10  */
#define	LA2L_L2_COUNT	__BIT(LA2L_L2_NBITS)		/* 2048 / 1024  */
#define	LA2L_L2_SHIFT	PGSHIFT				/*   13 /   12  */
#define	LA2L_L1_NBITS	(32 - LA2L_L2_NBITS - PGSHIFT)	/*    8 /   10  */
#define	LA2L_L1_COUNT	__BIT(LA2L_L1_NBITS)		/*  256 / 1024  */
#define	LA2L_L1_SHIFT	(LA2L_L2_NBITS + PGSHIFT)	/*   24 /   22  */

#define	LA2L_L1_MASK	(__BITS(0,(LA2L_L1_NBITS - 1)) << LA2L_L1_SHIFT)
#define	LA2L_L2_MASK	(__BITS(0,(LA2L_L2_NBITS - 1)) << LA2L_L2_SHIFT)

#define	LA2L_RI(va)	__SHIFTOUT((va), LA2L_L1_MASK)	/* root index */
#define	LA2L_PGI(va)	__SHIFTOUT((va), LA2L_L2_MASK)	/* page index */

#define	MMU51_TCR_BITS	(TCR51_E | TCR51_SRE |				\
			 __SHIFTIN(PGSHIFT, TCR51_PS) |			\
			 __SHIFTIN(LA2L_L1_NBITS, TCR51_TIA) |		\
			 __SHIFTIN(LA2L_L2_NBITS, TCR51_TIB))

#define	MMU51_3L_TCR_BITS (TCR51_E | TCR51_SRE |			\
			__SHIFTIN(PGSHIFT, TCR51_PS) |			\
			__SHIFTIN(LA40_L1_NBITS, TCR51_TIA) |		\
			__SHIFTIN(LA40_L2_NBITS, TCR51_TIB) |		\
			__SHIFTIN(LA40_L3_NBITS, TCR51_TIC))

#define	MMU40_TCR_BITS	(TCR40_E |					\
			 __SHIFTIN(PGSHIFT - 12, TCR40_P))

/* SEG1SHIFT3L is for the "upper" segment on the 3-level configuration */
#define	SEGSHIFT2L	(LA2L_L1_SHIFT)			/*   24 /   22  */
#define	SEGSHIFT3L	(LA40_L2_SHIFT)			/*   18 /   18  */
#define	SEG1SHIFT3L	(LA40_L1_SHIFT)			/*   25 /   25  */

/* NBSEG13L is for the "upper" segment on the 3-level configuration */
#define	NBSEG2L		__BIT(SEGSHIFT2L)
#define	NBSEG3L		__BIT(SEGSHIFT3L)
#define	NBSEG13L	__BIT(SEG1SHIFT3L)

#define	SEGOFSET2L	(NBSEG2L - 1)
#define	SEGOFSET3L	(NBSEG3L - 1)
#define	SEG1OFSET3L	(NBSEG13L - 1)

#define	pmap_trunc_seg_2L(va)	(((vaddr_t)(va)) & ~SEGOFSET2L)
#define	pmap_round_seg_2L(va)	(pmap_trunc_seg_2L((vaddr_t)(va) + SEGOFSET2L))
#define	pmap_seg_offset_2L(va)	(((vaddr_t)(va)) & SEGOFSET2L)

#define	pmap_trunc_seg_3L(va)	(((vaddr_t)(va)) & ~SEGOFSET3L)
#define	pmap_round_seg_3L(va)	(pmap_trunc_seg_3L((vaddr_t)(va) + SEGOFSET3L))
#define	pmap_seg_offset_3L(va)	(((vaddr_t)(va)) & SEGOFSET3L)

#define	pmap_trunc_seg1_3L(va)	(((vaddr_t)(va)) & ~SEG1OFSET3L)
#define	pmap_round_seg1_3L(va)	(pmap_trunc_seg1_3L((vaddr_t)(va)+ SEG1OFSET3L))
#define	pmap_seg1_offset_3L(va)	(((vaddr_t)(va)) & SEG1OFSET3L)

/*
 * pmap-specific data store in the vm_page structure.
 *
 * We keep the U/M attrs in the lower 2 bits of the list head
 * pointer.  This is possible because both the U and M bits are
 * adjacent; we just need to shift them down 3 bit positions.
 *
 * Assumes that PV entries will be 4-byte aligned, but the allocator
 * guarantees this for us.
 */
#define	__HAVE_VM_PAGE_MD
struct vm_page_md {
	uintptr_t pvh_listx;		/* pv_entry list + attrs */
};

#define	PVH_UM_SHIFT	3
#define	PVH_UM_MASK	__BITS(0,1)
#define	PVH_CI		__BIT(2)
#define	PVH_ATTR_MASK	(PVH_UM_MASK | PVH_CI)
#define	PVH_PV_MASK	(~PVH_ATTR_MASK)

#define VM_MDPAGE_INIT(pg)					\
do {								\
	(pg)->mdpage.pvh_listx = 0;				\
} while (/*CONSTCOND*/0)

#define	VM_MDPAGE_PVS(pg)					\
	((struct pv_entry *)((pg)->mdpage.pvh_listx & (uintptr_t)PVH_PV_MASK))

#define	VM_MDPAGE_HEAD_PVP(pg)					\
	((struct pv_entry **)&(pg)->mdpage.pvh_listx)

#define	VM_MDPAGE_SETPVP(pvp, pv)				\
do {								\
	/*							\
	 * The page attributes are in the lower two bits of	\
	 * the first PV pointer.  Rather than comparing the	\
	 * address and branching, we just always preserve what	\
	 * might be there (either the attribute bits or zero	\
	 * bits).						\
	 */							\
	*(pvp) = (struct pv_entry *)				\
	    ((uintptr_t)(pv) |					\
	     (((uintptr_t)(*(pvp))) & (uintptr_t)PVH_ATTR_MASK));\
} while (/*CONSTCOND*/0)

#define	VM_MDPAGE_UM(pg)					\
	(((pg)->mdpage.pvh_listx & PVH_UM_MASK) << PVH_UM_SHIFT)

#define	VM_MDPAGE_ADD_UM(pg, a)					\
do {								\
	(pg)->mdpage.pvh_listx |=				\
	    ((a) >> PVH_UM_SHIFT) & PVH_UM_MASK;		\
} while (/*CONSTCOND*/0)

#define	VM_MDPAGE_SET_UM(pg, v)					\
do {								\
	(pg)->mdpage.pvh_listx =				\
	    ((pg)->mdpage.pvh_listx & ~PVH_UM_MASK) |		\
	    (((v) >> PVH_UM_SHIFT) & PVH_UM_MASK);		\
} while (/*CONSTCOND*/0)

#define	VM_MDPAGE_SET_CI(pg)					\
do {								\
	(pg)->mdpage.pvh_listx |= PVH_CI;			\
} while (/*CONSTCOND*/0)

#define VM_MDPAGE_CLR_CI(pg)					\
do {								\
	(pg)->mdpage.pvh_listx &= ~PVH_CI;			\
} while (/*CONSTCOND*/0)

#define	VM_MDPAGE_CI_P(pg)					\
	((pg)->mdpage.pvh_listx & PVH_CI)

bool	pmap_testbit(struct vm_page *, pt_entry_t);
#define	pmap_is_referenced(pg)					\
	((VM_MDPAGE_UM(pg) & PTE_U) || pmap_testbit((pg), PTE_U))
#define	pmap_is_modified(pg)					\
	((VM_MDPAGE_UM(pg) & PTE_M) || pmap_testbit((pg), PTE_M))

bool	pmap_changebit(struct vm_page *, pt_entry_t, pt_entry_t);
#define	pmap_clear_reference(pg)				\
	pmap_changebit((pg), 0, (pt_entry_t)~PTE_U)
#define	pmap_clear_modify(pg)					\
	pmap_changebit((pg), 0, (pt_entry_t)~PTE_M)

#define	pmap_update(pmap)		__nothing
#define	pmap_copy(dp, sp, da, l, sa)	__nothing

#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

#define	PMAP_GROWKERNEL			/* enable pmap_growkernel() */

void	pmap_procwr(struct proc *, vaddr_t, size_t);
#define	PMAP_NEED_PROCWR

/*
 * pmap_bootstrap1() is called before the MMU is turned on.
 * pmap_bootstrap2() is called after.
 */
paddr_t	pmap_bootstrap1(paddr_t/*nextpa*/, paddr_t/*reloff*/);
void *	pmap_bootstrap2(void);

/*
 * Variant of pmap_extract() that returns additional information about
 * the mapping.  Used by bus_dma(9).
 */
bool	pmap_extract_info(pmap_t, vaddr_t, paddr_t *, int *);

/*
 * Functions exported for compatibility with the Hibler pmap, where
 * these are needed by other shared m68k code.
 *
 * XXX Clean this up eventually.
 */
pt_entry_t *		pmap_kernel_pte(vaddr_t);
#define	kvtopte(va)	pmap_kernel_pte(va)

paddr_t		vtophys(vaddr_t);

extern char *	vmmap;
extern void *	msgbufaddr;

/* Support functions for HP MMU. */
void	pmap_init_vac(size_t);
void	pmap_prefer(vaddr_t, vaddr_t *, int);
/* PMAP_PREFER() defined in <machine/pmap.h> on machines were it's needed. */

/* Kernel crash dump support. */
phys_ram_seg_t *	pmap_init_kcore_hdr(cpu_kcore_hdr_t *);

#endif /* _M68K_PMAP_68K_H_ */
