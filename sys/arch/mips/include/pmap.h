/*	$NetBSD: pmap.h,v 1.54.26.13 2010/05/04 17:15:53 matt Exp $	*/

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

#define PMAP_SEGTABSIZE		(1 << (31 - SEGSHIFT))

union pt_entry;

struct segtab {
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
void pmap_segtab_alloc(struct pmap *);
void pmap_segtab_free(struct pmap *);
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
typedef struct pmap {
#ifdef MULTIPROCESSOR
	volatile uint32_t	pm_active;	/* pmap was active on ... */
	volatile uint32_t	pm_onproc;	/* pmap is active on ... */
	volatile u_int		pm_shootdown_pending;
#endif
	struct segtab		*pm_segtab;	/* pointers to pages of PTEs */
	u_int			pm_count;	/* pmap reference count */
	u_int			pm_flags;
#define	PMAP_DEFERRED_ACTIVATE	0x0001
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	struct pmap_asid_info	pm_pai[1];
} *pmap_t;

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

#define pmap_kernel()		(&kernel_pmap_store.kernel_pmap)
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
void	pmap_tlb_asid_acquire(pmap_t pmap, struct lwp *l);
void	pmap_tlb_asid_deactivate(pmap_t pmap);
void	pmap_tlb_asid_release_all(pmap_t pmap);
int	pmap_tlb_update_addr(pmap_t pmap, vaddr_t, uint32_t, bool);
void	pmap_tlb_invalidate_addr(pmap_t pmap, vaddr_t);

uint16_t pmap_pvlist_lock(struct vm_page *, bool);

/*
 * pmap_prefer() helps reduce virtual-coherency exceptions in
 * the virtually-indexed cache on mips3 CPUs.
 */
#ifdef MIPS3_PLUS
#define PMAP_PREFER(pa, va, sz, td)	pmap_prefer((pa), (va), (sz), (td))
void	pmap_prefer(vaddr_t, vaddr_t *, vsize_t, int);
#endif /* MIPS3_PLUS */

#define	PMAP_STEAL_MEMORY	/* enable pmap_steal_memory() */

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
#define PMAP_NOCACHE	0x4000000000000000ULL
#endif

#endif	/* _KERNEL */
#endif	/* _MIPS_PMAP_H_ */
