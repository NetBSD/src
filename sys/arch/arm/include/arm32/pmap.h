/*	$NetBSD: pmap.h,v 1.16.2.4 2002/04/01 07:39:11 nathanw Exp $	*/

/*
 * Copyright (c) 1994,1995 Mark Brinicombe.
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
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
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

#ifndef	_ARM32_PMAP_H_
#define	_ARM32_PMAP_H_

#ifdef _KERNEL

#include <arm/cpufunc.h>
#include <arm/arm32/pte.h>
#include <uvm/uvm_object.h>

/*
 * a pmap describes a processes' 4GB virtual address space.  this
 * virtual address space can be broken up into 4096 1MB regions which
 * are described by L1 PTEs in the L1 table.
 *
 * There is a line drawn at KERNEL_BASE.  Everything below that line
 * changes when the VM context is switched.  Everything above that line
 * is the same no matter which VM context is running.  This is achieved
 * by making the L1 PTEs for those slots above KERNEL_BASE reference
 * kernel L2 tables.
 *
 * The L2 tables are mapped linearly starting at PTE_BASE.  PTE_BASE
 * is below KERNEL_BASE, which means that the current process's PTEs
 * are always available starting at PTE_BASE.  Another region of KVA
 * above KERNEL_BASE, APTE_BASE, is reserved for mapping in the PTEs
 * of another process, should we need to manipulate them.
 *
 * The basic layout of the virtual address space thus looks like this:
 *
 *	0xffffffff
 *	.
 *	.
 *	.
 *	KERNEL_BASE
 *	--------------------
 *	PTE_BASE
 *	.
 *	.
 *	.
 *	0x00000000
 */

/*
 * The pmap structure itself.
 */
struct pmap {
	struct uvm_object	pm_obj;		/* uvm_object */
#define	pm_lock	pm_obj.vmobjlock	
	LIST_ENTRY(pmap)	pm_list;	/* list (lck by pm_list lock) */
	pd_entry_t		*pm_pdir;	/* KVA of page directory */
	struct l1pt		*pm_l1pt;	/* L1 table metadata */
	paddr_t                 pm_pptpt;	/* PA of pt's page table */
	vaddr_t                 pm_vptpt;	/* VA of pt's page table */
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	struct vm_page		*pm_ptphint;	/* recently used PT */
};

typedef struct pmap *pmap_t;

/*
 * Physical / virtual address structure. In a number of places (particularly
 * during bootstrapping) we need to keep track of the physical and virtual
 * addresses of various pages
 */
typedef struct pv_addr {
	SLIST_ENTRY(pv_addr) pv_list;
	paddr_t pv_pa;
	vaddr_t pv_va;
} pv_addr_t;

/*
 * Determine various modes for PTEs (user vs. kernel, cacheable
 * vs. non-cacheable).
 */
#define	PTE_KERNEL	0
#define	PTE_USER	1
#define	PTE_NOCACHE	0
#define	PTE_CACHE	1

/*
 * Commonly referenced structures
 */
extern struct pmap	kernel_pmap_store;
extern int		pmap_debug_level; /* Only exists if PMAP_DEBUG */

/*
 * Macros that we need to export
 */
#define pmap_kernel()			(&kernel_pmap_store)
#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

#define	pmap_is_modified(pg)		(((pg)->mdpage.pvh_attrs & PT_M) != 0)
#define	pmap_is_referenced(pg)		(((pg)->mdpage.pvh_attrs & PT_H) != 0)

#define	pmap_copy(dp, sp, da, l, sa)	/* nothing */

#define pmap_phys_address(ppn)		(arm_ptob((ppn)))

/*
 * Functions that we need to export
 */
vaddr_t	pmap_map(vaddr_t, vaddr_t, vaddr_t, int);
void	pmap_procwr(struct proc *, vaddr_t, int);

#define	PMAP_NEED_PROCWR
#define PMAP_GROWKERNEL		/* turn on pmap_growkernel interface */

/* Functions we use internally. */
void	pmap_bootstrap(pd_entry_t *, pv_addr_t);
void	pmap_debug(int);
int	pmap_handled_emulation(struct pmap *, vaddr_t);
int	pmap_modified_emulation(struct pmap *, vaddr_t);
void	pmap_postinit(void);

/* Bootstrapping routines. */
void	pmap_map_section(vaddr_t, vaddr_t, paddr_t, int, int);
void	pmap_map_entry(vaddr_t, vaddr_t, paddr_t, int, int);
vsize_t	pmap_map_chunk(vaddr_t, vaddr_t, paddr_t, vsize_t, int, int);
void	pmap_link_l2pt(vaddr_t, vaddr_t, pv_addr_t *);

/*
 * Special page zero routine for use by the idle loop (no cache cleans). 
 */
boolean_t	pmap_pageidlezero __P((paddr_t));
#define PMAP_PAGEIDLEZERO(pa)	pmap_pageidlezero((pa))

/*
 * The current top of kernel VM
 */
extern vaddr_t	pmap_curmaxkvaddr;

/*
 * Useful macros and constants 
 */

/* Virtual address to page table entry */
#define vtopte(va) \
	(((pt_entry_t *)PTE_BASE) + arm_btop((vaddr_t) (va)))

/* Virtual address to physical address */
#define vtophys(va) \
	((*vtopte(va) & PG_FRAME) | ((vaddr_t) (va) & ~PG_FRAME))

#define	l1pte_valid(pde)	((pde) != 0)
#define	l1pte_section_p(pde)	(((pde) & L1_MASK) == L1_SECTION)
#define	l1pte_page_p(pde)	(((pde) & L1_MASK) == L1_PAGE)
#define	l1pte_fpage_p(pde)	(((pde) & L1_MASK) == L1_FPAGE)

#define	l2pte_valid(pte)	((pte) != 0)
#define	l2pte_pa(pte)		((pte) & PG_FRAME)

/* L1 and L2 page table macros */
#define pmap_pdei(v)		((v & PD_MASK) >> PDSHIFT)
#define pmap_pde(m, v)		(&((m)->pm_pdir[pmap_pdei(v)]))

#define pmap_pde_v(pde)		l1pte_valid(*(pde))
#define pmap_pde_section(pde)	l1pte_section_p(*(pde))
#define pmap_pde_page(pde)	l1pte_page_p(*(pde))
#define pmap_pde_fpage(pde)	l1pte_fpage_p(*(pde))

#define	pmap_pte_v(pte)		l2pte_valid(*(pte))
#define	pmap_pte_pa(pte)	l2pte_pa(*(pte))


/* Size of the kernel part of the L1 page table */
#define KERNEL_PD_SIZE	\
	(PD_SIZE - (KERNEL_BASE >> PDSHIFT) * sizeof(pd_entry_t))

/*
 * tell MI code that the cache is virtually-indexed *and* virtually-tagged.
 */

#define PMAP_CACHE_VIVT

#endif /* _KERNEL */

#endif	/* _ARM32_PMAP_H_ */
