/*	$NetBSD: pmap.h,v 1.6.2.2 2007/12/13 21:26:35 bouyer Exp $	*/

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * All rights reserved.
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
 *    must display the following acknowledgment:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 * pmap.h: see pmap.c for the history of this pmap module.
 */

#ifndef _X86_PMAP_H_
#define	_X86_PMAP_H_

#define ptei(VA)	(((VA_SIGN_POS(VA)) & L1_MASK) >> L1_SHIFT)

/*
 * pl*_pi: index in the ptp page for a pde mapping a VA.
 * (pl*_i below is the index in the virtual array of all pdes per level)
 */
#define pl1_pi(VA)	(((VA_SIGN_POS(VA)) & L1_MASK) >> L1_SHIFT)
#define pl2_pi(VA)	(((VA_SIGN_POS(VA)) & L2_MASK) >> L2_SHIFT)
#define pl3_pi(VA)	(((VA_SIGN_POS(VA)) & L3_MASK) >> L3_SHIFT)
#define pl4_pi(VA)	(((VA_SIGN_POS(VA)) & L4_MASK) >> L4_SHIFT)

/*
 * pl*_i: generate index into pde/pte arrays in virtual space
 */
#define pl1_i(VA)	(((VA_SIGN_POS(VA)) & L1_FRAME) >> L1_SHIFT)
#define pl2_i(VA)	(((VA_SIGN_POS(VA)) & L2_FRAME) >> L2_SHIFT)
#define pl3_i(VA)	(((VA_SIGN_POS(VA)) & L3_FRAME) >> L3_SHIFT)
#define pl4_i(VA)	(((VA_SIGN_POS(VA)) & L4_FRAME) >> L4_SHIFT)
#define pl_i(va, lvl) \
        (((VA_SIGN_POS(va)) & ptp_masks[(lvl)-1]) >> ptp_shifts[(lvl)-1])

#define	pl_i_roundup(va, lvl)	pl_i((va)+ ~ptp_masks[(lvl)-1], (lvl))

/*
 * PTP macros:
 *   a PTP's index is the PD index of the PDE that points to it
 *   a PTP's offset is the byte-offset in the PTE space that this PTP is at
 *   a PTP's VA is the first VA mapped by that PTP
 */

#define ptp_va2o(va, lvl)	(pl_i(va, (lvl)+1) * PAGE_SIZE)

#if defined(_KERNEL)
/*
 * pmap data structures: see pmap.c for details of locking.
 */

struct pmap;
typedef struct pmap *pmap_t;

/*
 * we maintain a list of all non-kernel pmaps
 */

LIST_HEAD(pmap_head, pmap); /* struct pmap_head: head of a pmap list */

/*
 * the pmap structure
 *
 * note that the pm_obj contains the simple_lock, the reference count,
 * page list, and number of PTPs within the pmap.
 *
 * pm_lock is the same as the spinlock for vm object 0. Changes to
 * the other objects may only be made if that lock has been taken
 * (the other object locks are only used when uvm_pagealloc is called)
 *
 * XXX If we ever support processor numbers higher than 31, we'll have
 * XXX to rethink the CPU mask.
 */

struct pmap {
	struct uvm_object pm_obj[PTP_LEVELS-1]; /* objects for lvl >= 1) */
#define	pm_lock	pm_obj[0].vmobjlock
	LIST_ENTRY(pmap) pm_list;	/* list (lck by pm_list lock) */
	pd_entry_t *pm_pdir;		/* VA of PD (lck by object lock) */
	paddr_t pm_pdirpa;		/* PA of PD (read-only after create) */
	struct vm_page *pm_ptphint[PTP_LEVELS-1];
					/* pointer to a PTP in our pmap */
	struct pmap_statistics pm_stats;  /* pmap stats (lck by object lock) */

#if !defined(__x86_64__)
	vaddr_t pm_hiexec;		/* highest executable mapping */
#endif /* !defined(__x86_64__) */
	int pm_flags;			/* see below */

	union descriptor *pm_ldt;	/* user-set LDT */
	int pm_ldt_len;			/* number of LDT entries */
	int pm_ldt_sel;			/* LDT selector */
	uint32_t pm_cpus;		/* mask of CPUs using pmap */
	uint32_t pm_kernel_cpus;	/* mask of CPUs using kernel part
					 of pmap */
};

/* pm_flags */
#define	PMF_USER_LDT	0x01	/* pmap has user-set LDT */
#define	PMF_USER_RELOAD	0x04	/* reload user pmap on PTE unmap (Xen) */


/*
 * for each managed physical page we maintain a list of <PMAP,VA>'s
 * which it is mapped at.  the list is headed by a pv_head structure.
 * there is one pv_head per managed phys page (allocated at boot time).
 * the pv_head structure points to a list of pv_entry structures (each
 * describes one mapping).
 */

struct pv_entry {			/* locked by its list's pvh_lock */
	SPLAY_ENTRY(pv_entry) pv_node;	/* splay-tree node */
	struct pmap *pv_pmap;		/* the pmap */
	vaddr_t pv_va;			/* the virtual address */
	struct vm_page *pv_ptp;		/* the vm_page of the PTP */
};

/*
 * pv_entrys are dynamically allocated in chunks from a single page.
 * we keep track of how many pv_entrys are in use for each page and
 * we can free pv_entry pages if needed.  there is one lock for the
 * entire allocation system.
 */

struct pv_page_info {
	TAILQ_ENTRY(pv_page) pvpi_list;
	struct pv_entry *pvpi_pvfree;
	int pvpi_nfree;
};

/*
 * number of pv_entry's in a pv_page
 * (note: won't work on systems where NPBG isn't a constant)
 */

#define PVE_PER_PVPAGE ((PAGE_SIZE - sizeof(struct pv_page_info)) / \
			sizeof(struct pv_entry))

/*
 * a pv_page: where pv_entrys are allocated from
 */

struct pv_page {
	struct pv_page_info pvinfo;
	struct pv_entry pvents[PVE_PER_PVPAGE];
};

/*
 * global kernel variables
 */

/* PDPpaddr: is the physical address of the kernel's PDP */
extern u_long PDPpaddr;

extern struct pmap kernel_pmap_store;	/* kernel pmap */
extern int pmap_pg_g;			/* do we support PG_G? */
extern long nkptp[PTP_LEVELS];

/*
 * macros
 */

#define	pmap_kernel()			(&kernel_pmap_store)
#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

#define pmap_clear_modify(pg)		pmap_clear_attrs(pg, PG_M)
#define pmap_clear_reference(pg)	pmap_clear_attrs(pg, PG_U)
#define pmap_copy(DP,SP,D,L,S)
#define pmap_is_modified(pg)		pmap_test_attrs(pg, PG_M)
#define pmap_is_referenced(pg)		pmap_test_attrs(pg, PG_U)
#define pmap_move(DP,SP,D,L,S)
#define pmap_phys_address(ppn)		x86_ptob(ppn)
#define pmap_valid_entry(E) 		((E) & PG_V) /* is PDE or PTE valid? */


/*
 * prototypes
 */

void		pmap_activate(struct lwp *);
void		pmap_bootstrap(vaddr_t);
bool		pmap_clear_attrs(struct vm_page *, unsigned);
void		pmap_deactivate(struct lwp *);
void		pmap_page_remove (struct vm_page *);
void		pmap_remove(struct pmap *, vaddr_t, vaddr_t);
bool		pmap_test_attrs(struct vm_page *, unsigned);
void		pmap_write_protect(struct pmap *, vaddr_t, vaddr_t, vm_prot_t);
void		pmap_load(void);
paddr_t		pmap_init_tmp_pgtbl(paddr_t);

vaddr_t reserve_dumppages(vaddr_t); /* XXX: not a pmap fn */

void	pmap_tlb_shootdown(pmap_t, vaddr_t, vaddr_t, pt_entry_t);
void	pmap_tlb_shootwait(void);

#define PMAP_GROWKERNEL		/* turn on pmap_growkernel interface */

/*
 * Do idle page zero'ing uncached to avoid polluting the cache.
 */
bool	pmap_pageidlezero(paddr_t);
#define	PMAP_PAGEIDLEZERO(pa)	pmap_pageidlezero((pa))

/*
 * inline functions
 */

/*ARGSUSED*/
static __inline void
pmap_remove_all(struct pmap *pmap)
{
	/* Nothing. */
}

/*
 * pmap_update_pg: flush one page from the TLB (or flush the whole thing
 *	if hardware doesn't support one-page flushing)
 */

__inline static void __attribute__((__unused__))
pmap_update_pg(vaddr_t va)
{
	invlpg(va);
}

/*
 * pmap_update_2pg: flush two pages from the TLB
 */

__inline static void __attribute__((__unused__))
pmap_update_2pg(vaddr_t va, vaddr_t vb)
{
	invlpg(va);
	invlpg(vb);
}

/*
 * pmap_page_protect: change the protection of all recorded mappings
 *	of a managed page
 *
 * => this function is a frontend for pmap_page_remove/pmap_clear_attrs
 * => we only have to worry about making the page more protected.
 *	unprotecting a page is done on-demand at fault time.
 */

__inline static void __attribute__((__unused__))
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	if ((prot & VM_PROT_WRITE) == 0) {
		if (prot & (VM_PROT_READ|VM_PROT_EXECUTE)) {
			(void) pmap_clear_attrs(pg, PG_RW);
		} else {
			pmap_page_remove(pg);
		}
	}
}

/*
 * pmap_protect: change the protection of pages in a pmap
 *
 * => this function is a frontend for pmap_remove/pmap_write_protect
 * => we only have to worry about making the page more protected.
 *	unprotecting a page is done on-demand at fault time.
 */

__inline static void __attribute__((__unused__))
pmap_protect(struct pmap *pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	if ((prot & VM_PROT_WRITE) == 0) {
		if (prot & (VM_PROT_READ|VM_PROT_EXECUTE)) {
			pmap_write_protect(pmap, sva, eva, prot);
		} else {
			pmap_remove(pmap, sva, eva);
		}
	}
}

/*
 * various address inlines
 *
 *  vtopte: return a pointer to the PTE mapping a VA, works only for
 *  user and PT addresses
 *
 *  kvtopte: return a pointer to the PTE mapping a kernel VA
 */

#include <lib/libkern/libkern.h>

static __inline pt_entry_t * __attribute__((__unused__))
vtopte(vaddr_t va)
{

	KASSERT(va < VM_MIN_KERNEL_ADDRESS);

	return (PTE_BASE + pl1_i(va));
}

static __inline pt_entry_t * __attribute__((__unused__))
kvtopte(vaddr_t va)
{
	pd_entry_t *pde;

	KASSERT(va >= VM_MIN_KERNEL_ADDRESS);

	pde = L2_BASE + pl2_i(va);
	if (*pde & PG_PS)
		return ((pt_entry_t *)pde);

	return (PTE_BASE + pl1_i(va));
}

paddr_t vtophys(vaddr_t);
vaddr_t	pmap_map(vaddr_t, paddr_t, paddr_t, vm_prot_t);
void	pmap_cpu_init_early(struct cpu_info *);
void	pmap_cpu_init_late(struct cpu_info *);
void	sse2_zero_page(void *);
void	sse2_copy_page(void *, void *);


#ifdef XEN

#define XPTE_MASK	L1_FRAME
/* XPTE_SHIFT = L1_SHIFT - log2(sizeof(pt_entry_t)) */
#ifdef __x86_64__
#define XPTE_SHIFT	9
#else
#define XPTE_SHIFT	10
#endif

/* PTE access inline fuctions */

/*
 * Get the machine address of the pointed pte
 * We use hardware MMU to get value so works only for levels 1-3
 */

static __inline paddr_t
xpmap_ptetomach(pt_entry_t *pte)
{
	pt_entry_t *up_pte;
	vaddr_t va = (vaddr_t) pte;

	va = ((va & XPTE_MASK) >> XPTE_SHIFT) | (vaddr_t) PTE_BASE;
	up_pte = (pt_entry_t *) va;

	return (paddr_t) (((*up_pte) & PG_FRAME) + (((vaddr_t) pte) & (~PG_FRAME & ~VA_SIGN_MASK)));
}

/*
 * xpmap_update()
 * Update an active pt entry with Xen
 * Equivalent to *pte = npte
 */

static __inline void
xpmap_update (pt_entry_t *pte, pt_entry_t npte)
{
        int s = splvm();

        xpq_queue_pte_update((pt_entry_t *) xpmap_ptetomach(pte), npte);
        xpq_flush_queue();
        splx(s);
}


/* Xen helpers to change bits of a pte */
#define XPMAP_UPDATE_DIRECT	1	/* Update direct map entry flags too */

/* pmap functions with machine addresses */
void	pmap_kenter_ma(vaddr_t, paddr_t, vm_prot_t);
int	pmap_enter_ma(struct pmap *, vaddr_t, paddr_t, paddr_t,
	    vm_prot_t, int, int);
bool	pmap_extract_ma(pmap_t, vaddr_t, paddr_t *);
paddr_t	vtomach(vaddr_t);

#endif	/* XEN */

/*
 * Hooks for the pool allocator.
 */
#define	POOL_VTOPHYS(va)	vtophys((vaddr_t) (va))

/*
 * TLB shootdown mailbox.
 */

struct pmap_mbox {
	volatile void		*mb_pointer;
	volatile uintptr_t	mb_addr1;
	volatile uintptr_t	mb_addr2;
	volatile uintptr_t	mb_head;
	volatile uintptr_t	mb_tail;
	volatile uintptr_t	mb_global;
};

#endif /* _KERNEL */

#endif /* _X86_PMAP_H_ */
