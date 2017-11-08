/*	$NetBSD: pmap.h,v 1.61.8.1.6.1 2017/11/08 21:22:57 snj Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)pmap.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Copyright (c) 1987 Carnegie-Mellon University
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)pmap.h	8.1 (Berkeley) 6/10/93
 */

#ifndef	_MIPS_PMAP_H_
#define	_MIPS_PMAP_H_

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#endif

#include <mips/cpuregs.h>	/* for KSEG0 below */
//#include <mips/pte.h>

/*
 * The user address space is 2Gb (0x0 - 0x80000000).
 * User programs are laid out in memory as follows:
 *			address
 *	USRTEXT		0x00001000
 *	USRDATA		USRTEXT + text_size
 *	USRSTACK	0x7FFFFFFF
 *
 * The user address space is mapped using a two level structure where
 * virtual address bits 30..22 are used to index into a segment table which
 * points to a page worth of PTEs (4096 page can hold 1024 PTEs).
 * Bits 21..12 are then used to index a PTE which describes a page within
 * a segment.
 *
 * The wired entries in the TLB will contain the following:
 *	0-1	(UPAGES)	for curproc user struct and kernel stack.
 *
 * Note: The kernel doesn't use the same data structures as user programs.
 * All the PTE entries are stored in a single array in Sysmap which is
 * dynamically allocated at boot time.
 */

#define mips_trunc_seg(x)	((vaddr_t)(x) & ~SEGOFSET)
#define mips_round_seg(x)	(((vaddr_t)(x) + SEGOFSET) & ~SEGOFSET)

#ifdef _LP64
#define PMAP_SEGTABSIZE		NSEGPG
#else
#define PMAP_SEGTABSIZE		(1 << (31 - SEGSHIFT))
#endif

union pt_entry;

union segtab {
#ifdef _LP64
	union segtab	*seg_seg[PMAP_SEGTABSIZE];
#endif
	union pt_entry	*seg_tab[PMAP_SEGTABSIZE];
};

/*
 * Structure defining an tlb entry data set.
 */
struct tlb {
	vaddr_t	tlb_hi;		/* should be 64 bits */
	uint32_t tlb_lo0;	/* XXX maybe 64 bits (only 32 really used) */
	uint32_t tlb_lo1;	/* XXX maybe 64 bits (only 32 really used) */
};

struct tlbmask {
	vaddr_t	tlb_hi;		/* should be 64 bits */
	uint32_t tlb_lo0;	/* XXX maybe 64 bits (only 32 really used) */
	uint32_t tlb_lo1;	/* XXX maybe 64 bits (only 32 really used) */
	uint32_t tlb_mask;
};

#ifdef _KERNEL
struct pmap;
typedef bool (*pte_callback_t)(struct pmap *, vaddr_t, vaddr_t,
	union pt_entry *, uintptr_t);
union pt_entry *pmap_pte_lookup(struct pmap *, vaddr_t);
union pt_entry *pmap_pte_reserve(struct pmap *, vaddr_t, int);
void pmap_pte_process(struct pmap *, vaddr_t, vaddr_t, pte_callback_t,
	uintptr_t);
void pmap_segtab_activate(struct pmap *, struct lwp *);
void pmap_segtab_init(struct pmap *);
void pmap_segtab_destroy(struct pmap *);
extern kmutex_t pmap_segtab_lock;
#endif /* _KERNEL */

/*
 * Per TLB (normally same as CPU) asid info
 */
struct pmap_asid_info {
	LIST_ENTRY(pmap_asid_info) pai_link;
	uint32_t	pai_asid;	/* TLB address space tag */
};

#define	TLBINFO_LOCK(ti)		mutex_spin_enter((ti)->ti_lock)
#define	TLBINFO_UNLOCK(ti)		mutex_spin_exit((ti)->ti_lock)
#define	PMAP_PAI_ASIDVALID_P(pai, ti)	((pai)->pai_asid != 0)
#define	PMAP_PAI(pmap, ti)		(&(pmap)->pm_pai[tlbinfo_index(ti)])
#define	PAI_PMAP(pai, ti)	\
	((pmap_t)((intptr_t)(pai) \
	    - offsetof(struct pmap, pm_pai[tlbinfo_index(ti)])))

/*
 * Machine dependent pmap structure.
 */
struct pmap {
#ifdef MULTIPROCESSOR
	volatile uint32_t	pm_active;	/* pmap was active on ... */
	volatile uint32_t	pm_onproc;	/* pmap is active on ... */
	volatile u_int		pm_shootdown_pending;
#endif
	union segtab		*pm_segtab;	/* pointers to pages of PTEs */
	u_int			pm_count;	/* pmap reference count */
	u_int			pm_flags;
#define	PMAP_DEFERRED_ACTIVATE	0x0001
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	struct pmap_asid_info	pm_pai[1];
};

enum tlb_invalidate_op {
	TLBINV_NOBODY=0,
	TLBINV_ONE=1,
	TLBINV_ALLUSER=2,
	TLBINV_ALLKERNEL=3,
	TLBINV_ALL=4
};

struct pmap_tlb_info {
	char ti_name[8];
	uint32_t ti_asid_hint;		/* probable next ASID to use */
	uint32_t ti_asids_free;		/* # of ASIDs free */
#define	tlbinfo_noasids_p(ti)	((ti)->ti_asids_free == 0)
	kmutex_t *ti_lock;
	u_int ti_wired;			/* # of wired TLB entries */
	uint32_t ti_asid_mask;
	uint32_t ti_asid_max;
	LIST_HEAD(, pmap_asid_info) ti_pais; /* list of active ASIDs */
#ifdef MULTIPROCESSOR
	pmap_t ti_victim;
	uint32_t ti_synci_page_bitmap;	/* page indices needing a syncicache */
	uint32_t ti_cpu_mask;		/* bitmask of CPUs sharing this TLB */
	enum tlb_invalidate_op ti_tlbinvop;
	u_int ti_index;
#define tlbinfo_index(ti)	((ti)->ti_index)
	struct evcnt ti_evcnt_synci_asts;
	struct evcnt ti_evcnt_synci_all;
	struct evcnt ti_evcnt_synci_pages;
	struct evcnt ti_evcnt_synci_deferred;
	struct evcnt ti_evcnt_synci_desired;
	struct evcnt ti_evcnt_synci_duplicate;
#else
#define tlbinfo_index(ti)	(0)
#endif
	struct evcnt ti_evcnt_asid_reinits;
	u_long ti_asid_bitmap[256 / (sizeof(u_long) * 8)];
};

#ifdef	_KERNEL

struct pmap_kernel {
	struct pmap kernel_pmap;
#ifdef MULTIPROCESSOR
	struct pmap_asid_info kernel_pai[MAXCPUS-1];
#endif
};

extern struct pmap_kernel kernel_pmap_store;
extern struct pmap_tlb_info pmap_tlb0_info;
#ifdef MULTIPROCESSOR
extern struct pmap_tlb_info *pmap_tlbs[MAXCPUS];
extern u_int pmap_ntlbs;
#endif
extern paddr_t mips_avail_start;
extern paddr_t mips_avail_end;
extern vaddr_t mips_virtual_end;

#define	pmap_wired_count(pmap) 	((pmap)->pm_stats.wired_count)
#define pmap_resident_count(pmap) ((pmap)->pm_stats.resident_count)

#define pmap_phys_address(x)	mips_ptob(x)

/*
 *	Bootstrap the system enough to run with virtual memory.
 */
void	pmap_bootstrap(void);

void	pmap_remove_all(pmap_t);
void	pmap_set_modified(paddr_t);
void	pmap_procwr(struct proc *, vaddr_t, size_t);
#define	PMAP_NEED_PROCWR

#ifdef MULTIPROCESSOR
void	pmap_tlb_shootdown_process(void);
bool	pmap_tlb_shootdown_bystanders(pmap_t pmap);
void	pmap_tlb_info_attach(struct pmap_tlb_info *, struct cpu_info *);
void	pmap_tlb_syncicache_ast(struct cpu_info *);
void	pmap_tlb_syncicache_wanted(struct cpu_info *);
void	pmap_tlb_syncicache(vaddr_t, uint32_t);
#endif
void	pmap_tlb_info_init(struct pmap_tlb_info *);
void	pmap_tlb_info_evcnt_attach(struct pmap_tlb_info *);
void	pmap_tlb_asid_acquire(pmap_t pmap, struct lwp *l);
void	pmap_tlb_asid_deactivate(pmap_t pmap);
void	pmap_tlb_asid_check(void);
void	pmap_tlb_asid_release_all(pmap_t pmap);
int	pmap_tlb_update_addr(pmap_t pmap, vaddr_t, uint32_t, bool);
void	pmap_tlb_invalidate_addr(pmap_t pmap, vaddr_t);

/*
 * pmap_prefer() helps reduce virtual-coherency exceptions in
 * the virtually-indexed cache on mips3 CPUs.
 */
#ifdef MIPS3_PLUS
#define PMAP_PREFER(pa, va, sz, td)	pmap_prefer((pa), (va), (sz), (td))
void	pmap_prefer(vaddr_t, vaddr_t *, vsize_t, int);
#endif /* MIPS3_PLUS */

#define	PMAP_STEAL_MEMORY	/* enable pmap_steal_memory() */
#define	PMAP_ENABLE_PMAP_KMPAGE	/* enable the PMAP_KMPAGE flag */

/*
 * Alternate mapping hooks for pool pages.  Avoids thrashing the TLB.
 */
vaddr_t mips_pmap_map_poolpage(paddr_t);
paddr_t mips_pmap_unmap_poolpage(vaddr_t);
struct vm_page *mips_pmap_alloc_poolpage(int);
#define	PMAP_ALLOC_POOLPAGE(flags)	mips_pmap_alloc_poolpage(flags)
#define	PMAP_MAP_POOLPAGE(pa)		mips_pmap_map_poolpage(pa)
#define	PMAP_UNMAP_POOLPAGE(va)		mips_pmap_unmap_poolpage(va)

/*
 * Other hooks for the pool allocator.
 */
#ifdef _LP64
#define	POOL_VTOPHYS(va)	(MIPS_KSEG0_P(va) \
				    ? MIPS_KSEG0_TO_PHYS(va) \
				    : MIPS_XKPHYS_TO_PHYS(va))
#else
#define	POOL_VTOPHYS(va)	MIPS_KSEG0_TO_PHYS((vaddr_t)(va))
#endif

/*
 * Select CCA to use for unmanaged pages.
 */
#define	PMAP_CCA_FOR_PA(pa)	CCA_UNCACHED		/* uncached */

#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
#define PGC_NOCACHE	0x4000000000000000ULL
#define PGC_PREFETCH	0x2000000000000000ULL
#endif

#define	__HAVE_VM_PAGE_MD

/*
 * pmap-specific data stored in the vm_page structure.
 */
/*
 * For each struct vm_page, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 * XXX really should do this as a part of the higher level code.
 */
typedef struct pv_entry {
	struct pv_entry	*pv_next;	/* next pv_entry */
	struct pmap	*pv_pmap;	/* pmap where mapping lies */
	vaddr_t		pv_va;		/* virtual address for mapping */
#define	PV_KENTER	0x001
} *pv_entry_t;

#define	PG_MD_UNCACHED		0x0001	/* page is mapped uncached */
#define	PG_MD_MODIFIED		0x0002	/* page has been modified */
#define	PG_MD_REFERENCED	0x0004	/* page has been recently referenced */
#define	PG_MD_POOLPAGE		0x0008	/* page is used as a poolpage */
#define	PG_MD_EXECPAGE		0x0010	/* page is exec mapped */

#define	PG_MD_CACHED_P(md)	(((md)->pvh_attrs & PG_MD_UNCACHED) == 0)
#define	PG_MD_UNCACHED_P(md)	(((md)->pvh_attrs & PG_MD_UNCACHED) != 0)
#define	PG_MD_MODIFIED_P(md)	(((md)->pvh_attrs & PG_MD_MODIFIED) != 0)
#define	PG_MD_REFERENCED_P(md)	(((md)->pvh_attrs & PG_MD_REFERENCED) != 0)
#define	PG_MD_POOLPAGE_P(md)	(((md)->pvh_attrs & PG_MD_POOLPAGE) != 0)
#define	PG_MD_EXECPAGE_P(md)	(((md)->pvh_attrs & PG_MD_EXECPAGE) != 0)

struct vm_page_md {
	struct pv_entry pvh_first;	/* pv_entry first */
#ifdef MULTIPROCESSOR
	volatile u_int pvh_attrs;	/* page attributes */
	kmutex_t *pvh_lock;		/* pv list lock */
#define	PG_MD_PVLIST_LOCK_INIT(md) 	((md)->pvh_lock = NULL)
#define	PG_MD_PVLIST_LOCKED_P(md)	(mutex_owner((md)->pvh_lock) != 0)
#define	PG_MD_PVLIST_LOCK(md, lc)	pmap_pvlist_lock((md), (lc))
#define	PG_MD_PVLIST_UNLOCK(md)		mutex_spin_exit((md)->pvh_lock)
#define	PG_MD_PVLIST_GEN(md)		((uint16_t)((md)->pvh_attrs >> 16))
#else
	u_int pvh_attrs;		/* page attributes */
#define	PG_MD_PVLIST_LOCK_INIT(md)	do { } while (/*CONSTCOND*/ 0)
#define	PG_MD_PVLIST_LOCKED_P(md)	true
#define	PG_MD_PVLIST_LOCK(md, lc)	(mutex_spin_enter(&pmap_pvlist_mutex), 0)
#define	PG_MD_PVLIST_UNLOCK(md)		mutex_spin_exit(&pmap_pvlist_mutex)
#define	PG_MD_PVLIST_GEN(md)		(0)
#endif
};

#define VM_MDPAGE_INIT(pg)					\
do {								\
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);	\
	(md)->pvh_first.pv_next = NULL;				\
	(md)->pvh_first.pv_pmap = NULL;				\
	(md)->pvh_first.pv_va = VM_PAGE_TO_PHYS(pg);		\
	PG_MD_PVLIST_LOCK_INIT(md);				\
	(md)->pvh_attrs = 0;					\
} while (/* CONSTCOND */ 0)

uint16_t pmap_pvlist_lock(struct vm_page_md *, bool);

#endif	/* _KERNEL */
#endif	/* _MIPS_PMAP_H_ */
