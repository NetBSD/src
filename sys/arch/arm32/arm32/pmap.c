/*	$NetBSD: pmap.c,v 1.40 1999/01/26 09:03:31 thorpej Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe.
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
 *
 * RiscBSD kernel project
 *
 * pmap.c
 *
 * Machine dependant vm stuff
 *
 * Created      : 20/09/94
 */

/*
 * Performance improvements, UVM changes, overhauls and part-rewrites
 * were contributed by Neil A. Carson <neil@causality.com>.
 */

/*
 * The dram block info is currently referenced from the bootconfig.
 * This should be placed in a separate structure.
 */

/*
 * Special compilation symbols
 * PMAP_DEBUG		- Build in pmap_debug_level code
 */
    
/* Include header files */

#include "opt_pmap_debug.h"
#include "opt_uvm.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#if defined(UVM)
#include <uvm/uvm.h>
#endif

#include <machine/bootconfig.h>
#include <machine/bus.h>
#include <machine/pmap.h>
#include <machine/pcb.h>
#include <machine/param.h>
#include <machine/katelib.h>
       
#ifdef HYDRA
#include "hydrabus.h"
#endif	/* HYDRA */

#ifdef PMAP_DEBUG
#define	PDEBUG(_lev_,_stat_) \
	if (pmap_debug_level >= (_lev_)) \
        	((_stat_))
int pmap_debug_level = -2;
#else	/* PMAP_DEBUG */
#define	PDEBUG(_lev_,_stat_) /* Nothing */
#endif	/* PMAP_DEBUG */

struct pmap     kernel_pmap_store;
pmap_t          kernel_pmap;

pagehook_t page_hook0;
pagehook_t page_hook1;
char *memhook;
pt_entry_t msgbufpte;
extern caddr_t msgbufaddr;

boolean_t pmap_initialized = FALSE;	/* Has pmap_init completed? */

TAILQ_HEAD(pv_page_list, pv_page) pv_page_freelist;

int pv_nfree = 0;

vm_size_t npages;

extern vm_offset_t physical_start;
extern vm_offset_t physical_freestart;
extern vm_offset_t physical_end;
extern vm_offset_t physical_freeend;
extern int physical_memoryblock;
extern unsigned int free_pages;
extern int max_processes;

vm_offset_t virtual_start;
vm_offset_t virtual_end;

vm_offset_t avail_start;
vm_offset_t avail_end;

extern pv_addr_t systempage;

#if NHYDRABUS > 0
extern pv_addr_t hydrascratch;
#endif	/* NHYDRABUS */

#define ALLOC_PAGE_HOOK(x, s) \
	x.va = virtual_start; \
	x.pte = (pt_entry_t *)pmap_pte(kernel_pmap, virtual_start); \
	virtual_start += s; 

/* Variables used by the L1 page table queue code */
SIMPLEQ_HEAD(l1pt_queue, l1pt);
struct l1pt_queue l1pt_static_queue;	/* head of our static l1 queue */
int l1pt_static_queue_count;		/* items in the static l1 queue */
int l1pt_static_create_count;		/* static l1 items created */
struct l1pt_queue l1pt_queue;		/* head of our l1 queue */
int l1pt_queue_count;			/* items in the l1 queue */
int l1pt_create_count;			/* stat - L1's create count */
int l1pt_reuse_count;			/* stat - L1's reused count */

/* Local function prototypes (not used outside this file) */
pt_entry_t *pmap_pte __P((pmap_t pmap, vm_offset_t va));
int pmap_page_index __P((vm_offset_t pa)); 
void map_pagetable __P((vm_offset_t pagetable, vm_offset_t va,
    vm_offset_t pa, unsigned int flags));
void pmap_copy_on_write __P((vm_offset_t pa));
void pmap_pinit __P((pmap_t));
void pmap_release __P((pmap_t));

/* Other function prototypes */
extern void bzero_page __P((vm_offset_t));
extern void bcopy_page __P((vm_offset_t, vm_offset_t));

struct l1pt *pmap_alloc_l1pt __P((void));
static __inline void pmap_map_in_l1 __P((pmap_t pmap, vm_offset_t va,
     vm_offset_t l2pa));

/* Function to set the debug level of the pmap code */

#ifdef PMAP_DEBUG
void
pmap_debug(level)
	int level;
{
	pmap_debug_level = level;
	printf("pmap_debug: level=%d\n", pmap_debug_level);
}
#endif	/* PMAP_DEBUG */

#include "isadma.h"

#if NISADMA > 0
/*
 * Used to protect memory for ISA DMA bounce buffers.  If, when loading
 * pages into the system, memory intersects with any of these ranges,
 * the intersecting memory will be loaded into a lower-priority free list.
 */
bus_dma_segment_t *pmap_isa_dma_ranges;
int pmap_isa_dma_nranges;

boolean_t pmap_isa_dma_range_intersect __P((vm_offset_t, vm_size_t,
	    vm_offset_t *, vm_size_t *));

/*
 * Check if a memory range intersects with an ISA DMA range, and
 * return the page-rounded intersection if it does.  The intersection
 * will be placed on a lower-priority free list.
 */
boolean_t
pmap_isa_dma_range_intersect(pa, size, pap, sizep)
	vm_offset_t pa;
	vm_size_t size;
	vm_offset_t *pap;
	vm_size_t *sizep;
{
	bus_dma_segment_t *ds;
	int i;

	if (pmap_isa_dma_ranges == NULL)
		return (FALSE);

	for (i = 0, ds = pmap_isa_dma_ranges;
	     i < pmap_isa_dma_nranges; i++, ds++) {
		if (ds->ds_addr <= pa && pa < (ds->ds_addr + ds->ds_len)) {
			/*
			 * Beginning of region intersects with this range.
			 */
			*pap = trunc_page(pa);
			*sizep = round_page(min(pa + size,
			    ds->ds_addr + ds->ds_len) - pa);
			return (TRUE);
		}
		if (pa < ds->ds_addr && ds->ds_addr < (pa + size)) {
			/*
			 * End of region intersects with this range.
			 */
			*pap = trunc_page(ds->ds_addr);
			*sizep = round_page(min((pa + size) - ds->ds_addr,
			    ds->ds_len));
			return (TRUE);
		}
	}

	/*
	 * No intersection found.
	 */
	return (FALSE);
}
#endif /* NISADMA > 0 */

/*
 * Functions for manipluation pv_entry structures. These are used to keep a
 * record of the mappings of virtual addresses and the associated physical
 * pages.
 */

/*
 * Allocate a new pv_entry structure from the freelist. If the list is
 * empty allocate a new page and fill the freelist.
 */
struct pv_entry *
pmap_alloc_pv()
{
	struct pv_page *pvp;
	struct pv_entry *pv;
	int i;

	/*
	 * Do we have any free pv_entry structures left ?
	 * If not allocate a page of them
	 */

	if (pv_nfree == 0) {
#if defined(UVM)
		/* NOTE: can't lock kernel_map here */
		MALLOC(pvp, struct pv_page *, NBPG, M_VMPVENT, M_WAITOK);
#else
		pvp = (struct pv_page *)kmem_alloc(kernel_map, NBPG);
#endif
		if (pvp == 0)
			panic("pmap_alloc_pv: kmem_alloc() failed");
		pvp->pvp_pgi.pgi_freelist = pv = &pvp->pvp_pv[1];
		for (i = NPVPPG - 2; i; i--, pv++)
			pv->pv_next = pv + 1;
		pv->pv_next = 0;
		pv_nfree += pvp->pvp_pgi.pgi_nfree = NPVPPG - 1;
		TAILQ_INSERT_HEAD(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
		pv = &pvp->pvp_pv[0];
	} else {
		--pv_nfree;
		pvp = pv_page_freelist.tqh_first;
		if (--pvp->pvp_pgi.pgi_nfree == 0) {
			TAILQ_REMOVE(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
		}
		pv = pvp->pvp_pgi.pgi_freelist;
#ifdef DIAGNOSTIC
		if (pv == 0)
			panic("pmap_alloc_pv: pgi_nfree inconsistent");
#endif	/* DIAGNOSTIC */
		pvp->pvp_pgi.pgi_freelist = pv->pv_next;
	}
	return pv;
}

/*
 * Release a pv_entry structure putting it back on the freelist.
 */

void
pmap_free_pv(pv)
	struct pv_entry *pv;
{
	struct pv_page *pvp;

	pvp = (struct pv_page *) trunc_page(pv);
	switch (++pvp->pvp_pgi.pgi_nfree) {
	case 1:
		TAILQ_INSERT_TAIL(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
	default:
		pv->pv_next = pvp->pvp_pgi.pgi_freelist;
		pvp->pvp_pgi.pgi_freelist = pv;
		++pv_nfree;
		break;
	case NPVPPG:
		pv_nfree -= NPVPPG - 1;
		TAILQ_REMOVE(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
#if defined(UVM)
		FREE((vm_offset_t)pvp, M_VMPVENT);
#else
		kmem_free(kernel_map, (vm_offset_t)pvp, NBPG);
#endif
		break;
	}
}

#if 0
void
pmap_collect_pv()
{
	struct pv_page_list pv_page_collectlist;
	struct pv_page *pvp, *npvp;
	struct pv_entry *ph, *ppv, *pv, *npv;
	int s;

	TAILQ_INIT(&pv_page_collectlist);

	for (pvp = pv_page_freelist.tqh_first; pvp; pvp = npvp) {
		if (pv_nfree < NPVPPG)
			break;
		npvp = pvp->pvp_pgi.pgi_list.tqe_next;
		if (pvp->pvp_pgi.pgi_nfree > NPVPPG / 3) {
			TAILQ_REMOVE(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
			TAILQ_INSERT_TAIL(&pv_page_collectlist, pvp,
			    pvp_pgi.pgi_list);
			pv_nfree -= pvp->pvp_pgi.pgi_nfree;
			pvp->pvp_pgi.pgi_nfree = -1;
		}
	}

	if (pv_page_collectlist.tqh_first == 0)
		return;

	for (ph = &pv_table[npages - 1]; ph >= &pv_table[0]; ph--) {
		if (ph->pv_pmap == 0)
			continue;
		s = splimp();
		for (ppv = ph; (pv = ppv->pv_next) != 0; ) {
			pvp = (struct pv_page *) trunc_page(pv);
			if (pvp->pvp_pgi.pgi_nfree == -1) {
				pvp = pv_page_freelist.tqh_first;
				if (--pvp->pvp_pgi.pgi_nfree == 0) {
					TAILQ_REMOVE(&pv_page_freelist,
					    pvp, pvp_pgi.pgi_list);
				}
				npv = pvp->pvp_pgi.pgi_freelist;
#ifdef DIAGNOSTIC
				if (npv == 0)
					panic("pmap_collect_pv: pgi_nfree inconsistent");
#endif	/* DIAGNOSTIC */
				pvp->pvp_pgi.pgi_freelist = npv->pv_next;
				*npv = *pv;
				ppv->pv_next = npv;
				ppv = npv;
			} else
				ppv = pv;
		}
		(void)splx(s);
	}

	for (pvp = pv_page_collectlist.tqh_first; pvp; pvp = npvp) {
		npvp = pvp->pvp_pgi.pgi_list.tqe_next;
#if defined(UVM)
		FREE((vm_offset_t)pvp, M_VMPVENT);
#else
		kmem_free(kernel_map, (vm_offset_t)pvp, NBPG);
#endif
	}
}
#endif

/*
 * Enter a new physical-virtual mapping into the pv table
 */

/*__inline*/ int
pmap_enter_pv(pmap, va, pv, flags)
	pmap_t pmap;
	vm_offset_t va;
	struct pv_entry *pv;
	u_int flags;
{
	struct pv_entry *npv;
	u_int s;

	if (!pmap_initialized)
		return(1);

	s = splimp();

	PDEBUG(5, printf("pmap_enter_pv: pv %p: %08lx/%p/%p\n",
	    pv, pv->pv_va, pv->pv_pmap, pv->pv_next));

	if (pv->pv_pmap == NULL) {
		/*
		 * No entries yet, use header as the first entry
		 */
		pv->pv_va = va;
		pv->pv_pmap = pmap;
		pv->pv_next = NULL;
		pv->pv_flags = flags;
		(void)splx(s);
		return(1);
	} else {
		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 */
		for (npv = pv; npv; npv = npv->pv_next)
			if (pmap == npv->pv_pmap && va == npv->pv_va)
				panic("pmap_enter_pv: already in pv_tab pv %p: %08lx/%p/%p",
				    pv, pv->pv_va, pv->pv_pmap, pv->pv_next);
		npv = pmap_alloc_pv();
		npv->pv_va = va;
		npv->pv_pmap = pmap;
		npv->pv_flags = flags;
		npv->pv_next = pv->pv_next;
		pv->pv_next = npv;
	}
	(void)splx(s);
	return(0);
}


/*
 * Remove a physical-virtual mapping from the pv table
 */

/* __inline*/ u_int
pmap_remove_pv(pmap, va, pv)
	pmap_t pmap;
	vm_offset_t va;
	struct pv_entry *pv;
{
	struct pv_entry *npv;
	u_int s;
	u_int flags = 0;
    
	if (!pmap_initialized)
		return(0);

	/*
	 * Remove from the PV table (raise IPL since we
	 * may be called at interrupt time).
	 */

	s = splimp();

	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */

	if (pmap == pv->pv_pmap && va == pv->pv_va) {
		npv = pv->pv_next;
		if (npv) {
			*pv = *npv;

			flags = npv->pv_flags;
			pmap_free_pv(npv);
		} else
			pv->pv_pmap = NULL;
	} else {
		for (npv = pv->pv_next; npv; pv = npv, npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && va == npv->pv_va)
				break;
		}
		if (npv) {
			pv->pv_next = npv->pv_next;

			flags = npv->pv_flags;
			pmap_free_pv(npv);
		}
	}
	(void)splx(s);
	return(flags);
}

/*
 * Modify a physical-virtual mapping in the pv table
 */

/*__inline */ u_int
pmap_modify_pv(pmap, va, pv, bic_mask, eor_mask)
	pmap_t pmap;
	vm_offset_t va;
	struct pv_entry *pv;
	u_int bic_mask;
	u_int eor_mask;
{
	struct pv_entry *npv;
	u_int s;
	u_int flags;

	PDEBUG(5, printf("pmap_modify_pv(pmap=%p, va=%08lx, pv=%p, bic_mask=%08x, eor_mask=%08x)\n",
	    pmap, va, pv, bic_mask, eor_mask));

	if (!pmap_initialized)
		return(0);

	s = splimp();

	PDEBUG(5, printf("pmap_modify_pv: pv %p: %08lx/%p/%p/%08x ",
	    pv, pv->pv_va, pv->pv_pmap, pv->pv_next, pv->pv_flags));

	/*
	 * There is at least one VA mapping this page.
	 */

	for (npv = pv; npv; npv = npv->pv_next) {
		if (pmap == npv->pv_pmap && va == npv->pv_va) {
			flags = npv->pv_flags;
			npv->pv_flags = ((flags & ~bic_mask) ^ eor_mask);
			PDEBUG(0, printf("done flags=%08x\n", flags));
			(void)splx(s);
			return(flags);
		}
	}

	PDEBUG(0, printf("done.\n"));
	(void)splx(s);
	return(0);
}


/*
 * Map the specified level 2 pagetable into the level 1 page table for
 * the given pmap to cover a chunk of virtual address space starting from the
 * address specified.
 */
static /*__inline*/ void
pmap_map_in_l1(pmap, va, l2pa)
	pmap_t pmap;
	vm_offset_t va, l2pa;
{
	vm_offset_t ptva;

	/* Calculate the index into the L1 page table */
	ptva = (va >> PDSHIFT) & ~3;

	/*
	 * XXX, rather than clearing the botton two bits of ptva test
	 * for zero and panic if not as this should always be the case
	 */

	PDEBUG(0, printf("wiring %08lx in to pd%p pte0x%lx va0x%lx\n", l2pa,
	    pmap->pm_pdir, L1_PTE(l2pa), ptva));

	/* Map page table into the L1 */
	pmap->pm_pdir[ptva + 0] = L1_PTE(l2pa + 0x000);
	pmap->pm_pdir[ptva + 1] = L1_PTE(l2pa + 0x400);
	pmap->pm_pdir[ptva + 2] = L1_PTE(l2pa + 0x800);
	pmap->pm_pdir[ptva + 3] = L1_PTE(l2pa + 0xc00);

	PDEBUG(0, printf("pt self reference %lx in %lx\n",
	    L2_PTE_NC_NB(l2pa, AP_KRW), pmap->pm_vptpt));

	/* Map the page table into the page table area */
	*((pt_entry_t *)(pmap->pm_vptpt + ptva)) =
	    L2_PTE_NC_NB(l2pa, AP_KRW);

	/* XXX should be a purge */
/*	cpu_tlb_flushD();*/
}


/*
 *	Used to map a range of physical addresses into kernel
 *	virtual address space.
 *
 *	For now, VM is already on, we only need to map the
 *	specified memory.
 */
vm_offset_t
pmap_map(va, spa, epa, prot)
	vm_offset_t va, spa, epa;
	int prot;
{
	while (spa < epa) {
		pmap_enter(pmap_kernel(), va, spa, prot, FALSE);
		va += NBPG;
		spa += NBPG;
	}
	return(va);
}


/*
 * void pmap_bootstrap(pd_entry_t *kernel_l1pt)
 *
 * bootstrap the pmap system. This is called from initarm and allows
 * the pmap system to initailise any structures it requires.
 *
 * Currently this sets up the kernel_pmap that is statically allocated
 * and also allocated virtual addresses for certain page hooks.
 * Currently the only one page hook is allocated that is used
 * to zero physical pages of memory.
 * It also initialises the start and end address of the kernel data space.
 */
extern vm_offset_t physical_freestart;
extern vm_offset_t physical_freeend;

void
pmap_bootstrap(kernel_l1pt, kernel_ptpt)
	pd_entry_t *kernel_l1pt;
	pv_addr_t kernel_ptpt;
{
	int loop;
	vm_offset_t start, end;
#if defined(UVM) && NISADMA > 0
	vm_offset_t istart;
	vm_size_t isize;
#endif

	kernel_pmap = &kernel_pmap_store;

	kernel_pmap->pm_pdir = kernel_l1pt;
	kernel_pmap->pm_pptpt = kernel_ptpt.pv_pa;
	kernel_pmap->pm_vptpt = kernel_ptpt.pv_va;
	simple_lock_init(&kernel_pmap->pm_lock);
	kernel_pmap->pm_count = 1;

	/*
	 * Initialize PAGE_SIZE-dependent variables.
	 */
#if defined(UVM)
	uvm_setpagesize();
#else
	vm_set_page_size();
#endif

	loop = 0;
	while (loop < bootconfig.dramblocks) {
		start = (vm_offset_t)bootconfig.dram[loop].address;
		end = start + (bootconfig.dram[loop].pages * NBPG);
		if (start < physical_freestart)
			start = physical_freestart;
		if (end > physical_freeend)
			end = physical_freeend;
#if 0
		printf("%d: %lx -> %lx\n", loop, start, end - 1);
#endif
#if defined(UVM)
#if NISADMA > 0
		if (pmap_isa_dma_range_intersect(start, end - start,
		    &istart, &isize)) {
			/*
			 * Place the pages that intersect with the
			 * ISA DMA range onto the ISA DMA free list.
			 */
#if 0
			printf("    ISADMA 0x%lx -> 0x%lx\n", istart,
			    istart + isize - 1);
#endif
			uvm_page_physload(atop(istart),
			    atop(istart + isize), atop(istart),
			    atop(istart + isize), VM_FREELIST_ISADMA);
			
			/*
			 * Load the pieces that come before
			 * the intersection into the default
			 * free list.
			 */
			if (start < istart) {
#if 0
				printf("    BEFORE 0x%lx -> 0x%lx\n",
				    start, istart - 1);
#endif
				uvm_page_physload(atop(start),
				    atop(istart), atop(start),
				    atop(istart), VM_FREELIST_DEFAULT);
			}

			/*
			 * Load the pieces that come after
			 * the intersection into the default
			 * free list.
			 */
			if ((istart + isize) < end) {
#if 0
				printf("     AFTER 0x%lx -> 0x%lx\n",
				    (istart + isize), end - 1);
#endif
				uvm_page_physload(atop(istart + isize),
				    atop(end), atop(istart + isize),
				    atop(end), VM_FREELIST_DEFAULT);
			}
		} else {
			uvm_page_physload(atop(start), atop(end),
			    atop(start), atop(end), VM_FREELIST_DEFAULT);
		}
#else	/* NISADMA > 0 */
		uvm_page_physload(atop(start), atop(end),
		    atop(start), atop(end), VM_FREELIST_DEFAULT);
#endif /* NISADMA > 0 */
#else	/* UVM */
		vm_page_physload(atop(start), atop(end),
		    atop(start), atop(end));
#endif /* UVM */
		++loop;
	}

	virtual_start = KERNEL_VM_BASE;
	virtual_end = virtual_start + KERNEL_VM_SIZE - 1;

	ALLOC_PAGE_HOOK(page_hook0, NBPG);
	ALLOC_PAGE_HOOK(page_hook1, NBPG);

	/*
	 * The mem special device needs a virtual hook but we don't
	 * need a pte
	 */
	memhook = (char *)virtual_start;
	virtual_start += NBPG;

	msgbufaddr = (caddr_t)virtual_start;
	msgbufpte = (pt_entry_t)pmap_pte(kernel_pmap, virtual_start);
	virtual_start += round_page(MSGBUFSIZE);

#if NHYDRABUS > 0
	hydrascratch.virtual = virtual_start;
	virtual_start += NBPG;

	*((pt_entry_t *)pmap_pte(kernel_pmap, hydrascratch.virtual)) =
	    L2_PTE_NC_NB(hydrascratch.physical, AP_KRW);
	cpu_tlb_flushD_SE(hydrascratch.virtual);
#endif	/* NHYDRABUS */
	cpu_cache_cleanD();
}

/*
 * void pmap_init(void)
 *
 * Initialize the pmap module.
 * Called by vm_init() in vm/vm_init.c in order to initialise
 * any structures that the pmap system needs to map virtual memory.
 */

extern int physmem;

void
pmap_init()
{
	vm_size_t s;
	vm_offset_t addr;
	int lcv;
    
	npages = physmem;
	printf("Number of pages to handle = %ld\n", npages);

	/*
	 * Set the available memory vars - These do not map to real memory
	 * addresses and cannot as the physical memory is fragmented.
	 * They are used by ps for %mem calculations.
	 * One could argue whether this should be the entire memory or just
	 * the memory that is useable in a user process.
	 */
 
	avail_start = 0;
	avail_end = physmem * NBPG;

	npages = 0;
	for (lcv = 0 ; lcv < vm_nphysseg ; lcv++) 
		npages += (vm_physmem[lcv].end - vm_physmem[lcv].start);
	s = (vm_size_t) (sizeof(struct pv_entry) * npages + npages);
	s = round_page(s);
#if defined(UVM)
	addr = (vm_offset_t)uvm_km_zalloc(kernel_map, s);
#else
	addr = (vm_offset_t)kmem_alloc(kernel_map, s);
#endif
	if (addr == NULL)
		panic("pmap_init");

	/* allocate pv_entry stuff first */
	for (lcv = 0 ; lcv < vm_nphysseg ; lcv++) {
		vm_physmem[lcv].pmseg.pvent = (struct pv_entry *) addr;
		addr = (vm_offset_t)(vm_physmem[lcv].pmseg.pvent +
			(vm_physmem[lcv].end - vm_physmem[lcv].start));
	}
	/* allocate attrs next */
	for (lcv = 0 ; lcv < vm_nphysseg ; lcv++) {
		vm_physmem[lcv].pmseg.attrs = (char *) addr;
		addr = (vm_offset_t)(vm_physmem[lcv].pmseg.attrs +
			(vm_physmem[lcv].end - vm_physmem[lcv].start));
	}
	TAILQ_INIT(&pv_page_freelist);

/*#ifdef DEBUG*/
	printf("pmap_init: %lx bytes (%lx pgs)\n", s, npages);
/*#endif*/

	/*
	 * Now it is safe to enable pv_entry recording.
	 */
	pmap_initialized = TRUE;
    
	/* Initialise our L1 page table queues and counters */
	SIMPLEQ_INIT(&l1pt_static_queue);
	l1pt_static_queue_count = 0;
	l1pt_static_create_count = 0;
	SIMPLEQ_INIT(&l1pt_queue);
	l1pt_queue_count = 0;
	l1pt_create_count = 0;
	l1pt_reuse_count = 0;
}

/*
 * pmap_postinit()
 *
 * This routine is called after the vm and kmem subsystems have been
 * initialised. This allows the pmap code to perform any initialisation
 * that can only be done one the memory allocation is in place.
 */

void
pmap_postinit()
{
	int loop;
	struct l1pt *pt;

#ifdef PMAP_STATIC_L1S
	for (loop = 0; loop < PMAP_STATIC_L1S; ++loop) {
#else	/* PMAP_STATIC_L1S */
	for (loop = 0; loop < max_processes; ++loop) {
#endif	/* PMAP_STATIC_L1S */
		/* Allocate a L1 page table */
		pt = pmap_alloc_l1pt();
		if (!pt)
			panic("Cannot allocate static L1 page tables\n");

		/* Clean it */
		bzero((void *)pt->pt_va, PD_SIZE);
		pt->pt_flags |= (PTFLAG_STATIC | PTFLAG_CLEAN);
		/* Add the page table to the queue */
		SIMPLEQ_INSERT_TAIL(&l1pt_static_queue, pt, pt_queue);
		++l1pt_static_queue_count;
		++l1pt_static_create_count;
	}
}


/*
 * Create and return a physical map.
 *
 * If the size specified for the map is zero, the map is an actual physical
 * map, and may be referenced by the hardware.
 *
 * If the size specified is non-zero, the map will be used in software only,
 * and is bounded by that size.
 */

pmap_t
pmap_create(size)
	vm_size_t size;
{
	pmap_t pmap;

	/* Software use map does not need a pmap */
	if (size) return NULL;

	/* Allocate memory for pmap structure and zero it */
	pmap = (pmap_t) malloc(sizeof *pmap, M_VMPMAP, M_WAITOK);
	bzero(pmap, sizeof(*pmap));

	/* Now init the machine part of the pmap */
	pmap_pinit(pmap);
	return(pmap);
}

/*
 * pmap_alloc_l1pt()
 *
 * This routine allocates physical and virtual memory for a L1 page table
 * and wires it.
 * A l1pt structure is returned to describe the allocated page table.
 *
 * This routine is allowed to fail if the required memory cannot be allocated.
 * In this case NULL is returned.
 */

struct l1pt *
pmap_alloc_l1pt(void)
{
	vm_offset_t va, pa;
	struct l1pt *pt;
	int error;
	vm_page_t m;
	pt_entry_t *pte;

	/* Allocate virtual address space for the L1 page table */
#if defined(UVM)
	va = uvm_km_valloc(kernel_map, PD_SIZE);
#else
	va = kmem_alloc_pageable(kernel_map, PD_SIZE);
#endif
	if (va == 0) {
#ifdef DIAGNOSTIC
		printf("pmap: Cannot allocate pageable memory for L1\n");
#endif	/* DIAGNOSTIC */
		return(NULL);
	}

	/* Allocate memory for the l1pt structure */
	pt = (struct l1pt *)malloc(sizeof(struct l1pt), M_VMPMAP, M_WAITOK);

	/*
	 * Allocate pages from the VM system.
	 */
	TAILQ_INIT(&pt->pt_plist);
#if defined(UVM)
	error = uvm_pglistalloc(PD_SIZE, physical_start, physical_end,
	    PD_SIZE, 0, &pt->pt_plist, 1, M_WAITOK);
#else
	error = vm_page_alloc_memory(PD_SIZE, physical_start, physical_end,
	    PD_SIZE, 0, &pt->pt_plist, 1, M_WAITOK);
#endif
	if (error) {
#ifdef DIAGNOSTIC
		printf("pmap: Cannot allocate physical memory for L1 (%d)\n",
		    error);
#endif	/* DIAGNOSTIC */
		/* Release the resources we already have claimed */
		free(pt, M_VMPMAP);
#if defined(UVM)
		uvm_km_free(kernel_map, va, PD_SIZE);
#else
		kmem_free(kernel_map, va, PD_SIZE);
#endif
		return(NULL);
	}

	/* Map our physical pages into our virtual space */
	pt->pt_va = va;
	m = pt->pt_plist.tqh_first;
	while (m && va < (pt->pt_va + PD_SIZE)) {
		pa = VM_PAGE_TO_PHYS(m);

		pmap_enter(pmap_kernel(), va, pa,
		    VM_PROT_READ | VM_PROT_WRITE, TRUE);

		/* Revoke cacheability and bufferability */
		/* XXX should be done better than this */
		pte = pmap_pte(pmap_kernel(), va);
		*pte = (*pte) & ~(PT_C | PT_B);

		va += NBPG;
		m = m->pageq.tqe_next;
	}

#ifdef DIAGNOSTIC
	if (m)
		panic("pmap_alloc_l1pt: pglist not empty\n");
#endif	/* DIAGNOSTIC */

	pt->pt_flags = 0;
	return(pt);
}

/*
 * Free a L1 page table previously allocated with pmap_alloc_l1pt().
 */
void
pmap_free_l1pt(pt)
	struct l1pt *pt;
{
	/* Separate the physical memory for the virtual space */
	pmap_remove(kernel_pmap, pt->pt_va, pt->pt_va + PD_SIZE);

	/* Return the physical memory */
#if defined(UVM)
	uvm_pglistfree(&pt->pt_plist);
#else
	vm_page_free_memory(&pt->pt_plist);
#endif

	/* Free the virtual space */
#if defined(UVM)
	uvm_km_free(kernel_map, pt->pt_va, PD_SIZE);
#else
	kmem_free(kernel_map, pt->pt_va, PD_SIZE);
#endif

	/* Free the l1pt structure */
	free(pt, M_VMPMAP);
}

/*
 * Allocate a page directory.
 * This routine will either allocate a new page directory from the pool
 * of L1 page tables currently held by the kernel or it will allocate
 * a new one via pmap_alloc_l1pt().
 * It will then initialise the l1 page table for use.
 */
int
pmap_allocpagedir(pmap)
	struct pmap *pmap;
{
	vm_offset_t pa;
	struct l1pt *pt;
	pt_entry_t *pte;

	PDEBUG(0, printf("pmap_allocpagedir(%p)\n", pmap));

	/* Do we have any spare L1's lying around ? */
	if (l1pt_static_queue_count) {
		--l1pt_static_queue_count;
		pt = l1pt_static_queue.sqh_first;
		SIMPLEQ_REMOVE_HEAD(&l1pt_static_queue, pt, pt_queue);
	} else if (l1pt_queue_count) {
		--l1pt_queue_count;
		pt = l1pt_queue.sqh_first;
		SIMPLEQ_REMOVE_HEAD(&l1pt_queue, pt, pt_queue);
		++l1pt_reuse_count;
	} else {
		pt = pmap_alloc_l1pt();
		if (!pt)
			return(ENOMEM);
		++l1pt_create_count;
	}

	/* Store the pointer to the l1 descriptor in the pmap. */
	pmap->pm_l1pt = pt;

	/* Get the physical address of the start of the l1 */
	pa = VM_PAGE_TO_PHYS(pt->pt_plist.tqh_first);

	/* Store the virtual address of the l1 in the pmap. */
	pmap->pm_pdir = (pd_entry_t *)pt->pt_va;

	/* Clean the L1 if it is dirty */
	if (!(pt->pt_flags & PTFLAG_CLEAN))
		bzero((void *)pmap->pm_pdir, (PD_SIZE - KERNEL_PD_SIZE));

	/* Do we already have the kernel mappings ? */
	if (!(pt->pt_flags & PTFLAG_KPT)) {
		/* Duplicate the kernel mapping i.e. all mappings 0xf0000000+ */

		bcopy((char *)kernel_pmap->pm_pdir + (PD_SIZE - KERNEL_PD_SIZE),
		    (char *)pmap->pm_pdir + (PD_SIZE - KERNEL_PD_SIZE),
		    KERNEL_PD_SIZE);
		pt->pt_flags |= PTFLAG_KPT;
	}

	/* Allocate a page table to map all the page tables for this pmap */

#ifdef DIAGNOSTIC
	if (pmap->pm_vptpt) {
		/* XXX What if we have one already ? */
		panic("pmap_allocpagedir: have pt already\n");
	}
#endif	/* DIAGNOSTIC */
#if defined(UVM)
	pmap->pm_vptpt = uvm_km_zalloc(kernel_map, NBPG);
#else
	pmap->pm_vptpt = kmem_alloc(kernel_map, NBPG);
#endif
	pmap->pm_pptpt = pmap_extract(kernel_pmap, pmap->pm_vptpt) & PG_FRAME;
	/* Revoke cacheability and bufferability */
	/* XXX should be done better than this */
	pte = pmap_pte(kernel_pmap, pmap->pm_vptpt);
	*pte = (*pte) & ~(PT_C | PT_B);

	/* Wire in this page table */
	pmap_map_in_l1(pmap, PROCESS_PAGE_TBLS_BASE, pmap->pm_pptpt);

	pt->pt_flags &= ~PTFLAG_CLEAN;	/* L1 is dirty now */

	/*
	 * Map the kernel page tables for 0xf0000000 +
	 * into the page table used to map the
	 * pmap's page tables
	 */
	bcopy((char *)(PROCESS_PAGE_TBLS_BASE
	    + (PROCESS_PAGE_TBLS_BASE >> (PGSHIFT - 2))
	    + ((PD_SIZE - KERNEL_PD_SIZE) >> 2)),
	    (char *)pmap->pm_vptpt + ((PD_SIZE - KERNEL_PD_SIZE) >> 2),
	    (KERNEL_PD_SIZE >> 2));

	pmap->pm_count = 1;
	simple_lock_init(&pmap->pm_lock);

	return(0);
}


/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */

static int pmap_pagedir_ident;	/* tsleep() ident */

void
pmap_pinit(pmap)
	struct pmap *pmap;
{
	PDEBUG(0, printf("pmap_pinit(%p)\n", pmap));

	/* Keep looping until we succeed in allocating a page directory */
	while (pmap_allocpagedir(pmap) != 0) {
		/*
		 * Ok we failed to allocate a suitable block of memory for an
		 * L1 page table. This means that either:
		 * 1. 16KB of virtual address space could not be allocated
		 * 2. 16KB of physically contiguous memory on a 16KB boundary
		 *    could not be allocated.
		 *
		 * Since we cannot fail we will sleep for a while and try
		 * again. Although we will be wakened when another page table
		 * is freed other memory releasing and swapping may occur
		 * that will mean we can succeed so we will keep trying
		 * regularly just in case.
		 */

		if (tsleep((caddr_t)&pmap_pagedir_ident, PZERO,
		   "l1ptwait", 1000) == EWOULDBLOCK)
			printf("pmap: Cannot allocate L1 page table, sleeping ...\n");
	}

	/* Map zero page for the pmap. This will also map the L2 for it */
	pmap_enter(pmap, 0x00000000, systempage.pv_pa,
	    VM_PROT_READ, TRUE);
}


void
pmap_freepagedir(pmap)
	pmap_t pmap;
{
	/* Free the memory used for the page table mapping */
#if defined(UVM)
	uvm_km_free(kernel_map, (vm_offset_t)pmap->pm_vptpt, NBPG);
#else
	kmem_free(kernel_map, (vm_offset_t)pmap->pm_vptpt, NBPG);
#endif

	/* junk the L1 page table */
	if (pmap->pm_l1pt->pt_flags & PTFLAG_STATIC) {
		/* Add the page table to the queue */
		SIMPLEQ_INSERT_TAIL(&l1pt_static_queue, pmap->pm_l1pt, pt_queue);
		++l1pt_static_queue_count;
		/* Wake up any sleeping processes waiting for a l1 page table */
		wakeup((caddr_t)&pmap_pagedir_ident);
	} else if (l1pt_queue_count < 8) {
		/* Add the page table to the queue */
		SIMPLEQ_INSERT_TAIL(&l1pt_queue, pmap->pm_l1pt, pt_queue);
		++l1pt_queue_count;
		/* Wake up any sleeping processes waiting for a l1 page table */
		wakeup((caddr_t)&pmap_pagedir_ident);
	} else
		pmap_free_l1pt(pmap->pm_l1pt);
}


/*
 * Retire the given physical map from service.
 * Should only be called if the map contains no valid mappings.
 */

void
pmap_destroy(pmap)
	pmap_t pmap;
{
	int count;

	if (pmap == NULL)
		return;

	PDEBUG(0, printf("pmap_destroy(%p)\n", pmap));
	simple_lock(&pmap->pm_lock);
	count = --pmap->pm_count;
	simple_unlock(&pmap->pm_lock);
	if (count == 0) {
		pmap_release(pmap);
		free((caddr_t)pmap, M_VMPMAP);
	}
}


/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 * Should only be called if the map contains no valid mappings.
 */

void
pmap_release(pmap)
	pmap_t pmap;
{
	struct vm_page *page;
	pt_entry_t *pte;
	int loop;

	PDEBUG(0, printf("pmap_release(%p)\n", pmap));

#if 0
	if (pmap->pm_count != 1)		/* XXX: needs sorting */
		panic("pmap_release count %d", pmap->pm_count);
#endif

	/* Remove the zero page mapping */
	pmap_remove(pmap, 0x00000000, 0x00000000 + NBPG);

	/*
	 * Free any page tables still mapped
	 * This is only temporay until pmap_enter can count the number
	 * of mappings made in a page table. Then pmap_remove() can
	 * reduce the count and free the pagetable when the count
	 * reaches zero.
	 */
	for (loop = 0; loop < (((PD_SIZE - KERNEL_PD_SIZE) >> 4) - 1); ++loop) {
		pte = (pt_entry_t *)(pmap->pm_vptpt + loop * 4);
		if (*pte != 0) {
			PDEBUG(0, printf("%x: pte=%p:%08x\n", loop, pte, *pte));
			page = PHYS_TO_VM_PAGE(pmap_pte_pa(pte));
			if (page == NULL)
				panic("pmap_release: bad address for phys page");
#if defined(UVM)
			uvm_pagefree(page);
#else
			vm_page_free1(page);
#endif
		}
	}
	/* Free the page dir */
	pmap_freepagedir(pmap);
}


/*
 * void pmap_reference(pmap_t pmap)
 *
 * Add a reference to the specified pmap.
 */

void
pmap_reference(pmap)
	pmap_t pmap;
{
	if (pmap == NULL)
		return;
                                  
	simple_lock(&pmap->pm_lock);
	pmap->pm_count++;
	simple_unlock(&pmap->pm_lock);
}

/*
 * void pmap_virtual_space(vm_offset_t *start, vm_offset_t *end)
 *
 * Return the start and end addresses of the kernel's virtual space.
 * These values are setup in pmap_bootstrap and are updated as pages
 * are allocated.
 */

void
pmap_virtual_space(start, end)
	vm_offset_t *start;
	vm_offset_t *end;
{
	*start = virtual_start;
	*end = virtual_end;
}


/*
 * void pmap_pageable(pmap_t pmap, vm_offset_t sva, vm_offset_t eva,
 *   boolean_t pageable)
 *  
 * Make the specified pages (by pmap, offset) pageable (or not) as requested.
 *
 * A page which is not pageable may not take a fault; therefore, its
 * page table entry must remain valid for the duration.
 *
 * This routine is merely advisory; pmap_enter will specify that these
 * pages are to be wired down (or not) as appropriate.
 */
 
void
pmap_pageable(pmap, sva, eva, pageable)
	pmap_t pmap;
	vm_offset_t sva;
	vm_offset_t eva;
	boolean_t pageable;
{
	/*
	 * Ok we can only make the specified pages pageable under the
	 * following conditions.
	 * 1. pageable == TRUE
	 * 2. eva = sva + NBPG
	 * 3. the pmap is the kernel_pmap ??? - got this from
	 *    i386/pmap.c ??
	 *
	 * right this will get called when making pagetables pageable
	 */
 
	PDEBUG(5, printf("pmap_pageable: pmap=%p sva=%08lx eva=%08lx p=%d\n",
	    pmap, sva, eva, pageable));

	if (pmap == kernel_pmap && pageable && eva == (sva + NBPG)) {
		vm_offset_t pa;
		pt_entry_t *pte;

		pte = pmap_pte(pmap, sva);
		if (!pte)
			return;
		if (!pmap_pte_v(pte))
			return;
		pa = pmap_pte_pa(pte);

		/*
		 * Mark it unmodified to avoid pageout
		 */
		pmap_clear_modify(pa);
	}
}                                     


/*
 * Activate the address space for the specified process.  If the process
 * is the current process, load the new MMU context.
 */
void
pmap_activate(p)
	struct proc *p;
{
	pmap_t pmap = p->p_vmspace->vm_map.pmap;
	struct pcb *pcb = &p->p_addr->u_pcb;

	pcb->pcb_pagedir = (pd_entry_t *)pmap_extract(kernel_pmap,
	    (vm_offset_t)pmap->pm_pdir);

	PDEBUG(0, printf("pmap_activate: p=%p pmap=%p pcb=%p pdir=%p l1=%p\n",
	    p, pmap, pcb, pmap->pm_pdir, pcb->pcb_pagedir));

	if (p == curproc) {
		PDEBUG(0, printf("pmap_activate: setting TTB\n"));
		setttb((u_int)pcb->pcb_pagedir);
	}
#if 0
	pmap->pm_pdchanged = FALSE;
#endif
}


/*
 * Deactivate the address space of the specified process.
 */
void
pmap_deactivate(p)
	struct proc *p;
{
}


/*
 * pmap_clean_page()
 *
 * This is a local function used to work out the best strategy to clean
 * a single page referenced by its entry in the PV table. It's used by
 * pmap_copy_page, pmap_zero page and maybe some others later on.
 *
 * Its policy is effectively:
 *  o If there are no mappings, we don't bother doing anything with the cache.
 *  o If there is one mapping, we clean just that page.
 *  o If there are multiple mappings, we clean the entire cache.
 *
 * So that some functions can be further optimised, it returns 0 if it didn't
 * clean the entire cache, or 1 if it did.
 *
 * XXX One bug in this routine is that if the pv_entry has a single page
 * mapped at 0x00000000 a whole cache clean will be performed rather than
 * just the 1 page. Since this should not occur in everyday use and if it does
 * it will just result in not the most efficient clean for the page.
 */
static int
pmap_clean_page(pv)
	struct pv_entry *pv;
{
	int s;
	int cache_needs_cleaning = 0;
	vm_offset_t page_to_clean = 0;

	if (!pv)
		return 0;

	/* Go to splimp() so we get exclusive lock for a mo */
	s = splimp();
	if (pv->pv_pmap) {
		cache_needs_cleaning = 1;
		if (!pv->pv_next)
			page_to_clean = pv->pv_va;
	}
	splx(s);

	/* Do cache ops outside the splimp. */
	if (page_to_clean)
		cpu_cache_purgeID_rng(page_to_clean, NBPG);
	else {
		if (cache_needs_cleaning) {
			cpu_cache_purgeID();
			return 1;
		}
	}
	return 0;
}

/*
 * pmap_find_pv()
 *
 * This is a local function that finds a PV entry for a given physical page.
 * This is a common op, and this function removes loads of ifdefs in the code.
 */
static __inline struct pv_entry *
pmap_find_pv(phys)
	vm_offset_t phys;
{
	struct pv_entry *pv = 0;

	if (pmap_initialized) {
		int bank, off;

		if ((bank = vm_physseg_find(atop(phys), &off)) == -1)
			panic("pmap_find_pv: not a real page, phys=%lx\n",
			    phys);
		pv = &vm_physmem[bank].pmseg.pvent[off];
	}
	return pv;
}

/*
 * pmap_zero_page()
 * 
 * Zero a given physical page by mapping it at a page hook point.
 * In doing the zero page op, the page we zero is mapped cachable, as with
 * StrongARM accesses to non-cached pages are non-burst making writing
 * _any_ bulk data very slow.
 */
void
pmap_zero_page(phys)
	vm_offset_t phys;
{
	struct pv_entry *pv;

	/* Get an entry for this page, and clean it it. */
	pv = pmap_find_pv(phys);
	pmap_clean_page(pv);

	/*
	 * Hook in the page, zero it, and purge the cache for that
	 * zeroed page. Invalidate the TLB as needed.
	 */
	*page_hook0.pte = L2_PTE(phys & PG_FRAME, AP_KRW);
	cpu_tlb_flushD_SE(page_hook0.va);
	bzero_page(page_hook0.va);
	cpu_cache_purgeD_rng(page_hook0.va, NBPG);
}

/*
 * pmap_copy_page()
 *
 * Copy one physical page into another, by mapping the pages into
 * hook points. The same comment regarding cachability as in
 * pmap_zero_page also applies here.
 */
void
pmap_copy_page(src, dest)
	vm_offset_t src;
	vm_offset_t dest;
{
	struct pv_entry *src_pv, *dest_pv;
	
	/* Get PV entries for the pages, and clean them if needed. */
	src_pv = pmap_find_pv(src);
	dest_pv = pmap_find_pv(dest);
	if (!pmap_clean_page(src_pv))
		pmap_clean_page(dest_pv);

	/*
	 * Map the pages into the page hook points, copy them, and purge
	 * the cache for the appropriate page. Invalidate the TLB
	 * as required.
	 */
	*page_hook0.pte = L2_PTE(src & PG_FRAME, AP_KRW);
	*page_hook1.pte = L2_PTE(dest & PG_FRAME, AP_KRW);
	cpu_tlb_flushD_SE(page_hook0.va);
	cpu_tlb_flushD_SE(page_hook1.va);
	bcopy_page(page_hook0.va, page_hook1.va);
	cpu_cache_purgeD_rng(page_hook0.va, NBPG);
	cpu_cache_purgeD_rng(page_hook1.va, NBPG);
}

/*
 * int pmap_next_phys_page(vm_offset_t *addr)
 *
 * Allocate another physical page returning true or false depending
 * on whether a page could be allocated.
 */
 
vm_offset_t
pmap_next_phys_page(addr)
	vm_offset_t addr;
	
{
	int loop;

	if (addr < bootconfig.dram[0].address)
		return(bootconfig.dram[0].address);

	loop = 0;

	while (bootconfig.dram[loop].address != 0
	    && addr > (bootconfig.dram[loop].address + bootconfig.dram[loop].pages * NBPG))
		++loop;

	if (bootconfig.dram[loop].address == 0)
		return(0);

	addr += NBPG;
	
	if (addr >= (bootconfig.dram[loop].address + bootconfig.dram[loop].pages * NBPG)) {
		if (bootconfig.dram[loop + 1].address == 0)
			return(0);
		addr = bootconfig.dram[loop + 1].address;
	}

	return(addr);
}

/*
 * pmap_remove()
 *
 * pmap_remove is responsible for nuking a number of mappings for a range
 * of virtual address space in the current pmap. To do this efficiently
 * is interesting, because in a number of cases a wide virtual address
 * range may be supplied that contains few actual mappings. So, the
 * optimisations are:
 *  1. Try and skip over hunks of address space for which an L1 entry
 *     does not exist.
 *  2. Build up a list of pages we've hit, up to a maximum, so we can
 *     maybe do just a partial cache clean. This path of execution is
 *     complicated by the fact that the cache must be flushed _before_
 *     the PTE is nuked, being a VAC :-)
 *  3. Maybe later fast-case a single page, but I don't think this is
 *     going to make _that_ much difference overall.
 */

#define PMAP_REMOVE_CLEAN_LIST_SIZE	3

void
pmap_remove(pmap, sva, eva)
	pmap_t pmap;
	vm_offset_t sva;
	vm_offset_t eva;
{
	int cleanlist_idx = 0;
	struct pagelist {
		vm_offset_t va;
		pt_entry_t *pte;
	} cleanlist[PMAP_REMOVE_CLEAN_LIST_SIZE];
	pt_entry_t *pte = 0;
	vm_offset_t pa;
	int pmap_active;

	/* Exit quick if there is no pmap */
	if (!pmap)
		return;

	PDEBUG(0, printf("pmap_remove: pmap=%p sva=%08lx eva=%08lx\n", pmap, sva, eva));

	sva &= PG_FRAME;
	eva &= PG_FRAME;

	/* Get a page table pointer */
	while (sva < eva) {
		pte = pmap_pte(pmap, sva);
		if (pte)
			break;
		sva = (sva & PD_MASK) + NBPD;
	}

	/* Note if the pmap is active thus require cache and tlb cleans */
	if ((curproc && curproc->p_vmspace->vm_map.pmap == pmap)
	    || (pmap == kernel_pmap))
		pmap_active = 1;
	else
		pmap_active = 0;

	/* Now loop along */
	while (sva < eva) {
		/* Check if we can move to the next PDE (l1 chunk) */
		if (!(sva & PT_MASK))
			if (!pmap_pde_v(pmap_pde(pmap, sva))) {
				sva += NBPD;
				pte += arm_byte_to_page(NBPD);
				continue;
			}

		/* We've found a valid PTE, so this page of PTEs has to go. */
		if (pmap_pte_v(pte)) {
			int bank, off;
			/* Update statistics */
			pmap->pm_stats.resident_count--;

			/*
			 * Add this page to our cache remove list, if we can.
			 * If, however the cache remove list is totally full,
			 * then do a complete cache invalidation taking note
			 * to backtrack the PTE table beforehand, and ignore
			 * the lists in future because there's no longer any
			 * point in bothering with them (we've paid the
			 * penalty, so will carry on unhindered). Otherwise,
			 * when we fall out, we just clean the list.
			 */
			if (pte) {
				PDEBUG(10, printf("remove: inv pte at %p(%x) ", pte, *pte));
				pa = pmap_pte_pa(pte);

				if (cleanlist_idx < PMAP_REMOVE_CLEAN_LIST_SIZE) {
					/* Add to the clean list. */
					cleanlist[cleanlist_idx].pte = pte;
					cleanlist[cleanlist_idx].va = sva;
					cleanlist_idx++;
				} else if (cleanlist_idx == PMAP_REMOVE_CLEAN_LIST_SIZE) {
					int cnt;

					/* Nuke everything if needed. */
					if (pmap_active) {
						cpu_cache_purgeID();
						cpu_tlb_flushID();
					}

					/*
					 * Roll back the previous PTE list,
					 * and zero out the current PTE.
					 */
					for (cnt = 0; cnt < PMAP_REMOVE_CLEAN_LIST_SIZE; cnt++)
						*cleanlist[cnt].pte = 0;
					*pte = 0;
					cleanlist_idx++;
				} else {
					/*
					 * We've already nuked the cache and
					 * TLB, so just carry on regardless,
					 * and we won't need to do it again
					 */
					*pte = 0;
				}

				/*
				 * Update flags. In a number of circumstances,
				 * we could cluster a lot of these and do a
				 * number of sequential pages in one go.
				 */
				if ((bank = vm_physseg_find(atop(pa), &off))
				    != -1) {
					int flags;

					flags = pmap_remove_pv(pmap, sva,
				    	    &vm_physmem[bank].pmseg.pvent[off]);
					vm_physmem[bank].pmseg.attrs[off]
					    |= flags & (PT_M | PT_H);
					if (flags & PT_W)
						pmap->pm_stats.wired_count--;

				}
			}
		}
		sva += NBPG;
		pte++;
	}

	/*
	 * Now, if we've fallen through down to here, chances are that there
	 * are less than PMAP_REMOVE_CLEAN_LIST_SIZE mappings left.
	 */
	if (cleanlist_idx <= PMAP_REMOVE_CLEAN_LIST_SIZE) {
		u_int cnt;

		for (cnt = 0; cnt < cleanlist_idx; cnt++) {
			if (pmap_active) {
				cpu_cache_purgeID_rng(cleanlist[cnt].va, NBPG);
				*cleanlist[cnt].pte = 0;
				cpu_tlb_flushID_SE(cleanlist[cnt].va);
			} else
				*cleanlist[cnt].pte = 0;
		}
	}
}

/*
 * Routine:	pmap_remove_all
 * Function:
 *		Removes this physical page from
 *		all physical maps in which it resides.
 *		Reflects back modify bits to the pager.
 */

void
pmap_remove_all(pa)
	vm_offset_t pa;
{
	struct pv_entry *ph, *pv, *npv;
	pmap_t pmap;
	pt_entry_t *pte;
	int bank, off;
	int s;

	PDEBUG(0, printf("pmap_remove_all: pa=%lx ", pa));

	bank = vm_physseg_find(atop(pa), &off);
	if (bank == -1)
		return;
	pv = ph = &vm_physmem[bank].pmseg.pvent[off];

	pmap_clean_page(pv);

	s = splimp();

	if (ph->pv_pmap == NULL) {
		PDEBUG(0, printf("free page\n"));
		(void)splx(s);
		return;
	}

	while (pv) {
		pmap = pv->pv_pmap;
		pte = pmap_pte(pmap, pv->pv_va);

		PDEBUG(0, printf("[%p,%08x,%08lx,%08x] ", pmap, *pte,
		    pv->pv_va, pv->pv_flags));
#ifdef DEBUG
		if (!pte || !pmap_pte_v(pte) || pmap_pte_pa(pte) != pa)
			panic("pmap_remove_all: bad mapping");
#endif	/* DEBUG */

		/*
		 * Update statistics
		 */
		pmap->pm_stats.resident_count--;

		/* Wired bit */
		if (pv->pv_flags & PT_W)
			pmap->pm_stats.wired_count--;
          
		/*
		 * Invalidate the PTEs.
		 * XXX: should cluster them up and invalidate as many
		 * as possible at once.
		 */

#ifdef needednotdone
reduce wiring count on page table pages as references drop
#endif

		/*
		 * Update saved attributes for managed page
		 */

		vm_physmem[bank].pmseg.attrs[off] |= pv->pv_flags & (PT_M | PT_H);
		*pte = 0;

		npv = pv->pv_next;
		if (pv == ph)
			ph->pv_pmap = NULL;
		else
			pmap_free_pv(pv);
		pv = npv;
	}
	(void)splx(s);

	PDEBUG(0, printf("done\n"));
	cpu_tlb_flushID();
}


/*
 * Set the physical protection on the specified range of this map as requested.
 */

void
pmap_protect(pmap, sva, eva, prot)
	pmap_t pmap;
	vm_offset_t sva;
	vm_offset_t eva;
	vm_prot_t prot;
{
	pt_entry_t *pte = NULL;
	int armprot;
	int flush = 0;
	vm_offset_t pa;
	int bank, off;

	/*
	 * Make sure pmap is valid. -dct
	 */
	if (pmap == NULL)
		return;
	PDEBUG(0, printf("pmap_protect: pmap=%p %08lx->%08lx %x\n",
	    pmap, sva, eva, prot));

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}
	if (prot & VM_PROT_WRITE)
		return;

	sva &= PG_FRAME;
	eva &= PG_FRAME;

	/*
	 * We need to acquire a pointer to a page table page before entering
	 * the following loop.
	 */
	while (sva < eva) {
		pte = pmap_pte(pmap, sva);
		if (pte)
			break;
		sva = (sva & PD_MASK) + NBPD;
	}

	while (sva < eva) {
		/* only check once in a while */
		if ((sva & PT_MASK) == 0) {
			if (!pmap_pde_v(pmap_pde(pmap, sva))) {
				/* We can race ahead here, to the next pde. */
				sva += NBPD;
				pte += arm_byte_to_page(NBPD);
				continue;
			}
		}

		if (!pmap_pte_v(pte))
			goto next;

		flush = 1;

		armprot = 0;
		if (sva < VM_MAXUSER_ADDRESS)
			armprot |= PT_AP(AP_U);
		else if (sva < VM_MAX_ADDRESS)
			armprot |= PT_AP(AP_W);  /* XXX Ekk what is this ? */
		*pte = (*pte & 0xfffff00f) | armprot;

		pa = pmap_pte_pa(pte);

		/* Get the physical page index */

		/* Clear write flag */
		if ((bank = vm_physseg_find(atop(pa), &off)) != -1)
			pmap_modify_pv(pmap, sva,
			    &vm_physmem[bank].pmseg.pvent[off], PT_Wr, 0);
next:
		sva += NBPG;
		pte++;
	}

	if (flush)
		cpu_tlb_flushID();
}


int
pmap_nightmare(pmap, pv, va, prot)
	pmap_t pmap;
	struct pv_entry *pv;
	vm_offset_t va;
	vm_prot_t prot;
{
	struct pv_entry *npv;
	int entries = 0;
	int writeable = 0;
	int cacheable = 0;

	/* We may need to inhibit the cache on all the mappings */

	if (pv->pv_pmap == NULL)
		panic("pmap_enter: pv_entry has been lost\n");

	/*
	 * There is at least one other VA mapping this page.
	 */
	for (npv = pv; npv; npv = npv->pv_next) {
		/* Count mappings in the same pmap */
		if (pmap == npv->pv_pmap) {
			++entries;
			/* Writeable mappings */
			if (npv->pv_flags & PT_Wr)
				++writeable;
		}
	}
	if (entries > 1) {
/*		printf("pmap_nightmare: e=%d w=%d p=%d c=%d va=%lx [",
		    entries, writeable, (prot & VM_PROT_WRITE), cacheable, va);*/
		for (npv = pv; npv; npv = npv->pv_next) {
			/* Count mappings in the same pmap */
			if (pmap == npv->pv_pmap) {
/*				printf("va=%lx ", npv->pv_va);*/
			}
		}
/*		printf("]\n");*/
#if 0
		if (writeable || (prot & VM_PROT_WRITE))) {
			for (npv = pv; npv; npv = npv->pv_next) {
				/* Revoke cacheability */
				if (pmap == npv->pv_pmap) {
					pte = pmap_pte(pmap, npv->pv_va);
					*pte = (*pte) & ~(PT_C | PT_B);
				}
			}
		}
#endif
	} else {
		if ((prot & VM_PROT_WRITE) == 0)
			cacheable = PT_C;
/*		if (cacheable == 0)
			printf("pmap_nightmare: w=%d p=%d va=%lx c=%d\n",
			    writeable, (prot & VM_PROT_WRITE), va, cacheable);*/
	}
	return(cacheable);
}


int
pmap_nightmare1(pmap, pv, va, prot, cacheable)
	pmap_t pmap;
	struct pv_entry *pv;
	vm_offset_t va;
	vm_prot_t prot;
	int cacheable;
{
	struct pv_entry *npv;
	int entries = 0;
	int writeable = 0;

	/* We may need to inhibit the cache on all the mappings */

	if (pv->pv_pmap == NULL)
		panic("pmap_enter: pv_entry has been lost\n");

	/*
	 * There is at least one other VA mapping this page.
	 */
	for (npv = pv; npv; npv = npv->pv_next) {
		/* Count mappings in the same pmap */
		if (pmap == npv->pv_pmap) {
			++entries;
			/* Writeable mappings */
			if (npv->pv_flags & PT_Wr)
				++writeable;
		}
	}
	if (entries > 1) {
/*		printf("pmap_nightmare1: e=%d w=%d p=%d c=%d va=%lx [",
		    entries, writeable, (prot & VM_PROT_WRITE), cacheable, va);*/
		for (npv = pv; npv; npv = npv->pv_next) {
			/* Count mappings in the same pmap */
			if (pmap == npv->pv_pmap) {
/*				printf("va=%lx ", npv->pv_va);*/
			}
		}
/*		printf("]\n");*/
#if 0
		if (writeable || (prot & VM_PROT_WRITE))) {
			for (npv = pv; npv; npv = npv->pv_next) {
				/* Revoke cacheability */
				if (pmap == npv->pv_pmap) {
					pte = pmap_pte(pmap, npv->pv_va);
					*pte = (*pte) & ~(PT_C | PT_B);
				}
			}
		}
#endif
	} else {
/*		cacheable = PT_C;*/
/*		printf("pmap_nightmare1: w=%d p=%d va=%lx c=%d\n",
		    writeable, (prot & VM_PROT_WRITE), va, cacheable);*/
	}

	return(cacheable);
}


/*
 * void pmap_enter(pmap_t pmap, vm_offset_t va, vm_offset_t pa, vm_prot_t prot,
 * boolean_t wired)
 *  
 *      Insert the given physical page (p) at
 *      the specified virtual address (v) in the
 *      target physical map with the protection requested.
 *
 *      If specified, the page will be wired down, meaning
 *      that the related pte can not be reclaimed.
 *
 *      NB:  This is the only routine which MAY NOT lazy-evaluate
 *      or lose information.  That is, this routine must actually
 *      insert this page into the given map NOW.
 */

void
pmap_enter(pmap, va, pa, prot, wired)
	pmap_t pmap;
	vm_offset_t va;
	vm_offset_t pa;
	vm_prot_t prot;
	boolean_t wired;
{
	pt_entry_t *pte;
	u_int npte;
	int bank, off;
	struct pv_entry *pv = NULL;
	u_int cacheable = 0;
	vm_offset_t opa = -1;

	PDEBUG(5, printf("pmap_enter: V%08lx P%08lx in pmap %p prot=%08x, wired = %d\n",
	    va, pa, pmap, prot, wired));

	/* Valid pmap ? */
	if (pmap == NULL)
		return;

	/* Valid address ? */
	if (va >= (KERNEL_VM_BASE + KERNEL_VM_SIZE))
		panic("pmap_enter: too big");

	/*
	 * Get a pointer to the pte for this virtual address. If the
	 * pte pointer is NULL then we are missing the L2 page table
	 * so we need to create one.
	 */
	pte = pmap_pte(pmap, va);
	if (!pte) {
		struct vm_page *page;
		vm_offset_t l2pa;

		/* Allocate a page table */
		for (;;) {
#if defined(UVM)
			page = uvm_pagealloc(NULL, 0, NULL);
#else
			page = vm_page_alloc1();
#endif
			if (page != NULL)
				break;
			
			/*
			 * No page available.  If we're the kernel
			 * pmap, we die, since we might not have
			 * a valid thread context.  For user pmaps,
			 * we assume that we _do_ have a valid thread
			 * context, so we wait here for the pagedaemon
			 * to free up some pages.
			 *
			 * XXX THE VM CODE IS PROBABLY HOLDING LOCKS
			 * XXX RIGHT NOW, BUT ONLY ON OUR PARENT VM_MAP
			 * XXX SO THIS IS PROBABLY SAFE.  In any case,
			 * XXX other pmap modules claim it is safe to
			 * XXX sleep here if it's a user pmap.
			 */
			if (pmap == pmap_kernel())
				panic("pmap_enter: kernel pmap and no more free pages");
			else {
#if defined(UVM)
				uvm_wait("pmap_enter");
#else
				vm_wait("pmap_enter");
#endif
			}
		}

		/* Wire this page table into the L1 */
		l2pa = VM_PAGE_TO_PHYS(page);
		pmap_zero_page(l2pa);
		pmap_map_in_l1(pmap, va, l2pa);

		/* Now try and get the pte again */
		pte = pmap_pte(pmap, va);
		if (!pte)
			panic("Failure 04 in pmap_enter(pmap=%p, va=%08lx pa=%08lx)\n",
			    pmap, va, pa);                                               
	}

	/* More debugging info */
	PDEBUG(5, printf("pmap_enter: pte for V%08lx = V%p (%08x)\n", va, pte,
	    *pte));

	/* Is the pte valid ? If so then this page is already mapped */
	if (pmap_pte_v(pte)) {
		/* Get the physical address of the current page mapped */
		opa = pmap_pte_pa(pte);

		/* Are we mapping the same page ? */
		if (opa == pa) {
			int flags;

			/* All we must be doing is changing the protection */
			PDEBUG(0, printf("Case 02 in pmap_enter (V%08lx P%08lx)\n",
			    va, pa));

			if ((bank = vm_physseg_find(atop(pa), &off)) != -1)
				pv = &vm_physmem[bank].pmseg.pvent[off];
			cacheable = (*pte) & PT_C;

			/* Has the wiring changed ? */
			if (pv) {
				flags = pmap_modify_pv(pmap, va, pv, 0, 0) & PT_W;
				if (flags && !wired)
					--pmap->pm_stats.wired_count;
				else if (!flags && wired)
					++pmap->pm_stats.wired_count;

 				cacheable = pmap_nightmare1(pmap, pv, va, prot, cacheable);
 			}
		} else {
			/* We are replacing the page with a new one. */
			cpu_cache_purgeID_rng(va, NBPG);

			PDEBUG(0, printf("Case 03 in pmap_enter (V%08lx P%08lx P%08lx)\n",
			    va, pa, opa));

			/*
			 * If it is part of our managed memory then we
			 * must remove it from the PV list
			 */
			if ((bank = vm_physseg_find(atop(opa), &off)) != -1) {
				int flags;
				flags = pmap_remove_pv(pmap, va,
				    &vm_physmem[bank].pmseg.pvent[off]);
				if (flags & PT_W)
					--pmap->pm_stats.wired_count;
				vm_physmem[bank].pmseg.attrs[off] |= flags & (PT_M | PT_H);
			}
			/* Update the wiring stats for the new page */
			if (wired)
				++pmap->pm_stats.wired_count;

			/*
			 * Enter on the PV list if part of our managed memory
			 */
			if ((bank = vm_physseg_find(atop(pa), &off)) != -1)
				pv = &vm_physmem[bank].pmseg.pvent[off];
			if (pv) {
				if (pmap_enter_pv(pmap, va, pv, 0))
 					cacheable = PT_C;
 				else
 					cacheable = pmap_nightmare(pmap, pv, va, prot);
 			} else
					cacheable = 0;
		}
	} else {
		/* pte is not valid so we must be hooking in a new page */

		++pmap->pm_stats.resident_count;
		if (wired)
			++pmap->pm_stats.wired_count;

		/*
		 * Enter on the PV list if part of our managed memory
		 */
		if ((bank = vm_physseg_find(atop(pa), &off)) != -1)
			pv = &vm_physmem[bank].pmseg.pvent[off];
		if (pv) {
			if (pmap_enter_pv(pmap, va, pv, 0)) 
				cacheable = PT_C;
			else
				cacheable = pmap_nightmare(pmap, pv, va, prot);
		} else {
			 /*
			  * Assumption: if it is not part of our managed
			  * memory then it must be device memory which
			  * may be volatile.
			  */
			if (bank == -1) {
			cacheable = 0;
			PDEBUG(0, printf("pmap_enter: non-managed memory mapping va=%08lx pa=%08lx\n",
			    va, pa));
			} else
				cacheable = PT_C;
		}
	}

	/* Construct the pte, giving the correct access */
      
	npte = (pa & PG_FRAME) | cacheable;
	if (pv)
		npte |= PT_B;

#ifdef DIAGNOSTIC
	if (va == 0 && (prot & VM_PROT_WRITE))
		printf("va=0 prot=%d\n", prot);
#endif	/* DIAGNOSTIC */

/*	if (va >= VM_MIN_ADDRESS && va < VM_MAXUSER_ADDRESS && !wired)
 		npte |= L2_INVAL;
	else*/
		npte |= L2_SPAGE;

	if (prot & VM_PROT_WRITE)
		npte |= PT_AP(AP_W);

	if (va >= VM_MIN_ADDRESS) {
		if (va < VM_MAXUSER_ADDRESS)
			npte |= PT_AP(AP_U);
		else if (va < VM_MAX_ADDRESS) { /* This must be a page table */
			npte |= PT_AP(AP_W);
			npte &= ~(PT_C | PT_B);
		}
	}

 	if (va >= VM_MIN_ADDRESS && va < VM_MAXUSER_ADDRESS && pv) /* Inhibit write access for user pages */
 		*pte = (npte & ~PT_AP(AP_W));
	else
		*pte = npte;

	if (*pte == 0)
		panic("oopss: *pte = 0 in pmap_enter() npte=%08x\n", npte);

	if (pv) {
		int flags = 0;
         
		if (wired) flags |= PT_W;
			flags |= npte & (PT_Wr | PT_Us);
		pmap_modify_pv(pmap, va, pv, ~(PT_Wr | PT_Us | PT_W), flags);
	}

	/*
	 * If we are mapping in a page to where the page tables are store
	 * then we must be mapping a page table. In this case we should
	 * also map the page table into the page directory
	 */
	if (va >= PROCESS_PAGE_TBLS_BASE && va < VM_MIN_KERNEL_ADDRESS)
		panic("pmap_enter: Mapping into page table area\n");

	/* Better flush the TLB ... */

	cpu_tlb_flushID_SE(va);

	PDEBUG(5, printf("pmap_enter: pte = V%p %08x\n", pte, *pte));
}


/*
 * pmap_page_protect:
 *
 * Lower the permission for all mappings to a given page.
 */

void
pmap_page_protect(phys, prot)
	vm_offset_t phys;
	vm_prot_t prot;
{
	PDEBUG(0, printf("pmap_page_protect(pa=%lx, prot=%d)\n", phys, prot));

	switch(prot) {
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		pmap_copy_on_write(phys);
		break;

	case VM_PROT_ALL:
		break;

	default:
		pmap_remove_all(phys);
		break;
	}
}


/*
 * Routine:	pmap_change_wiring
 * Function:	Change the wiring attribute for a map/virtual-address
 *		pair.
 * In/out conditions:
 *		The mapping must already exist in the pmap.
 */

void
pmap_change_wiring(pmap, va, wired)
	pmap_t pmap;
	vm_offset_t va;
	boolean_t wired;
{
	pt_entry_t *pte;
	vm_offset_t pa;
	int bank, off;
	int current;
	struct pv_entry *pv;

	/*
	 * Make sure pmap is valid. -dct
	 */
	if (pmap == NULL)
		return;

	/* Get the pte */
	pte = pmap_pte(pmap, va);
	if (!pte)
		return;

	/* Extract the physical address of the page */
	pa = pmap_pte_pa(pte);

	if ((bank = vm_physseg_find(atop(pa), &off)) == -1)
		return;
	pv = &vm_physmem[bank].pmseg.pvent[off];
	/* Update the wired bit in the pv entry for this page. */
	current = pmap_modify_pv(pmap, va, pv, PT_W, wired ? PT_W : 0) & PT_W;

	/* Update the statistics */
	if (wired & !current)
		pmap->pm_stats.wired_count++;
	else if (!wired && current)
		pmap->pm_stats.wired_count--;
}


/*
 * pt_entry_t *pmap_pte(pmap_t pmap, vm_offset_t va)
 *
 * Return the pointer to a page table entry corresponding to the supplied
 * virtual address.
 *
 * The page directory is first checked to make sure that a page table
 * for the address in question exists and if it does a pointer to the
 * entry is returned.
 *
 * The way this works is that that the kernel page tables are mapped
 * into the memory map at ALT_PAGE_TBLS_BASE to ALT_PAGE_TBLS_BASE+4MB.
 * This allows page tables to be located quickly.
 */
pt_entry_t *
pmap_pte(pmap, va)
	pmap_t pmap;
	vm_offset_t va;
{
	pt_entry_t *ptp;
	pt_entry_t *result;

	/* The pmap must be valid */
	if (!pmap)
		return(NULL);

	/* Return the address of the pte */
	PDEBUG(10, printf("pmap_pte: pmap=%p va=V%08lx pde = V%p (%08X)\n",
	    pmap, va, pmap_pde(pmap, va), *(pmap_pde(pmap, va))));

	/* Do we have a valid pde ? If not we don't have a page table */
	if (!pmap_pde_v(pmap_pde(pmap, va))) {
		PDEBUG(0, printf("pmap_pte: failed - pde = %p\n",
		    pmap_pde(pmap, va)));
		return(NULL); 
	}

	PDEBUG(10, printf("pmap pagetable = P%08lx current = P%08x\n",
	    pmap->pm_pptpt, (*((pt_entry_t *)(PROCESS_PAGE_TBLS_BASE
	    + (PROCESS_PAGE_TBLS_BASE >> (PGSHIFT - 2)) +
	    (PROCESS_PAGE_TBLS_BASE >> PDSHIFT))) & PG_FRAME)));

	/*
	 * If the pmap is the kernel pmap or the pmap is the active one
	 * then we can just return a pointer to entry relative to
	 * PROCESS_PAGE_TBLS_BASE.
	 * Otherwise we need to map the page tables to an alternative
	 * address and reference them there.
	 */
	if (pmap == kernel_pmap || pmap->pm_pptpt
	    == (*((pt_entry_t *)(PROCESS_PAGE_TBLS_BASE
	    + ((PROCESS_PAGE_TBLS_BASE >> (PGSHIFT - 2)) &
	    ~3) + (PROCESS_PAGE_TBLS_BASE >> PDSHIFT))) & PG_FRAME)) {
		ptp = (pt_entry_t *)PROCESS_PAGE_TBLS_BASE;
	} else {
		struct proc *p = curproc;

		/* If we don't have a valid curproc use proc0 */
		/* Perhaps we should just use kernel_pmap instead */
		if (p == NULL)
			p = &proc0;
#ifdef DIAGNOSTIC
		/*
		 * The pmap should always be valid for the process so
		 * panic if it is not.
		 */
		if (!p->p_vmspace || !p->p_vmspace->vm_map.pmap)
			panic("pmap_pte: problem\n");
		/*
		 * The pmap for the current process should be mapped. If it
		 * is not then we have a problem.
		 */
		if (p->p_vmspace->vm_map.pmap->pm_pptpt !=
		    (*((pt_entry_t *)(PROCESS_PAGE_TBLS_BASE
		    + (PROCESS_PAGE_TBLS_BASE >> (PGSHIFT - 2)) +
		    (PROCESS_PAGE_TBLS_BASE >> PDSHIFT))) & PG_FRAME)) {
			printf("pmap pagetable = P%08lx current = P%08x ",
			    pmap->pm_pptpt, (*((pt_entry_t *)(PROCESS_PAGE_TBLS_BASE
			    + (PROCESS_PAGE_TBLS_BASE >> (PGSHIFT - 2)) +
			    (PROCESS_PAGE_TBLS_BASE >> PDSHIFT))) &
			    PG_FRAME));
			printf("pptpt=%lx\n", p->p_vmspace->vm_map.pmap->pm_pptpt);
			panic("pmap_pte: current and pmap mismatch\n");
		}
#endif

		ptp = (pt_entry_t *)ALT_PAGE_TBLS_BASE;
		pmap_map_in_l1(p->p_vmspace->vm_map.pmap, ALT_PAGE_TBLS_BASE,
		    pmap->pm_pptpt);
		cpu_tlb_flushD();
	}
	PDEBUG(10, printf("page tables base = %p offset=%lx\n", ptp,
	    ((va >> (PGSHIFT-2)) & ~3)));
	result = (pt_entry_t *)((char *)ptp + ((va >> (PGSHIFT-2)) & ~3));
	return(result);
}

/*
 * Routine:  pmap_extract
 * Function:
 *           Extract the physical page address associated
 *           with the given map/virtual_address pair.
 */
vm_offset_t
pmap_extract(pmap, va)
	pmap_t pmap;
	vm_offset_t va;
{
	pt_entry_t *pte;
	vm_offset_t pa;

	PDEBUG(5, printf("pmap_extract: pmap=%p, va=V%08lx\n", pmap, va));

	/*
	 * Get the pte for this virtual address. If there is no pte
	 * then there is no page table etc.
	 */
  
	pte = pmap_pte(pmap, va);
	if (!pte)
		return(0);

	/* Is the pte valid ? If not then no paged is actually mapped here */
	if (!pmap_pte_v(pte))
		return(0);

	/* Extract the physical address from the pte */
	pa = pmap_pte_pa(pte);

	PDEBUG(5, printf("pmap_extract: pa = P%08lx\n",
	    (pa | (va & ~PG_FRAME))));

	return(pa | (va & ~PG_FRAME));
}


/*
 * Copy the range specified by src_addr/len from the source map to the
 * range dst_addr/len in the destination map.
 *
 * This routine is only advisory and need not do anything.
 */

void
pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	pmap_t dst_pmap;
	pmap_t src_pmap;
	vm_offset_t dst_addr;
	vm_size_t len;
	vm_offset_t src_addr;
{
	PDEBUG(0, printf("pmap_copy(%p, %p, %lx, %lx, %lx)\n",
	    dst_pmap, src_pmap, dst_addr, len, src_addr));
}

#if defined(PMAP_DEBUG)
void
pmap_dump_pvlist(phys, m)
	vm_offset_t phys;
	char *m;
{
	struct pv_entry *pv;
	int bank, off;

	if (!pmap_initialized)
		return;

	if ((bank = vm_physseg_find(atop(phys), &off)) == -1) {
		printf("INVALID PA\n");
		return;
	}
	pv = &vm_physmem[bank].pmseg.pvent[off];
	printf("%s %08lx:", m, phys);
	if (pv->pv_pmap == NULL) {
		printf(" no mappings\n");
		return;
	}

	for (; pv; pv = pv->pv_next)
		printf(" pmap %p va %08lx flags %08x", pv->pv_pmap,
		    pv->pv_va, pv->pv_flags);

	printf("\n");
}

#endif	/* PMAP_DEBUG */

boolean_t
pmap_testbit(pa, setbits)
	vm_offset_t pa;
	int setbits;
{
	struct pv_entry *pv;
	int bank, off;
	int s;

	PDEBUG(1, printf("pmap_testbit: pa=%08lx set=%08x\n", pa, setbits));

	if ((bank = vm_physseg_find(atop(pa), &off)) == -1)
		return(FALSE);
	pv = &vm_physmem[bank].pmseg.pvent[off];
	s = splimp();

	/*
	 * Check saved info first
	 */
	if (vm_physmem[bank].pmseg.attrs[off] & setbits) {
		PDEBUG(0, printf("pmap_attributes = %02x\n",
		    vm_physmem[bank].pmseg.attrs[off]));
		(void)splx(s);
		return(TRUE);
	}

	/*
	 * Not found, check current mappings returning
	 * immediately if found.
	 */
	if (pv->pv_pmap != NULL) {
		for (; pv; pv = pv->pv_next) {
/*			pte = pmap_pte(pv->pv_pmap, pv->pv_va);*/

			/* The write bit is in the flags */
			if ((pv->pv_flags & setbits) /*|| (*pte & (setbits & PT_Wr))*/) {
				(void)splx(s);
				return(TRUE);
			}
			if ((setbits & PT_M) && pv->pv_va >= VM_MAXUSER_ADDRESS) {
				(void)splx(s);
				return(TRUE);
			}
			if ((setbits & PT_H) && pv->pv_va >= VM_MAXUSER_ADDRESS) {
				(void)splx(s);
				return(TRUE);
			}
		}
	}

	(void)splx(s);
	return(FALSE);
}


/*
 * Modify pte bits for all ptes corresponding to the given physical address.
 * We use `maskbits' rather than `clearbits' because we're always passing
 * constants and the latter would require an extra inversion at run-time.
 */

void
pmap_changebit(pa, setbits, maskbits)
	vm_offset_t pa;
	int setbits;
	int maskbits;
{
	struct pv_entry *pv;
	pt_entry_t *pte;
	vm_offset_t va;
	int bank, off;
	int s;

	PDEBUG(1, printf("pmap_changebit: pa=%08lx set=%08x mask=%08x\n",
	    pa, setbits, maskbits));
	if ((bank = vm_physseg_find(atop(pa), &off)) == -1)
		return;
	pv = &vm_physmem[bank].pmseg.pvent[off];
	s = splimp();

	/*
	 * Clear saved attributes (modify, reference)
	 */

	if (~maskbits)
		vm_physmem[bank].pmseg.attrs[off] &= maskbits;

	/*
	 * Loop over all current mappings setting/clearing as appropos
	 */
	if (pv->pv_pmap != NULL) {
		for (; pv; pv = pv->pv_next) {
			va = pv->pv_va;

			/*
			 * XXX don't write protect pager mappings
			 */
			if (maskbits == ~PT_Wr) {
#if defined(UVM)
                                if (va >= uvm.pager_sva && va < uvm.pager_eva)
                                	continue;
#else
				extern vm_offset_t pager_sva, pager_eva;

				if (va >= pager_sva && va < pager_eva)
					continue;
#endif
			}

			pv->pv_flags = (pv->pv_flags & maskbits) | setbits;
			pte = pmap_pte(pv->pv_pmap, va);
			if ((maskbits & PT_Wr) == 0)
				*pte = (*pte) & ~PT_AP(AP_W);
			if (setbits & PT_Wr)
				*pte = (*pte) | PT_AP(AP_W);
/*
			if ((maskbits & PT_H) == 0)
				*pte = ((*pte) & ~L2_MASK) | L2_INVAL;
*/
		}
		cpu_tlb_flushID();
	}
	(void)splx(s);
}


void
pmap_clear_modify(pa)
	vm_offset_t pa;
{
	PDEBUG(0, printf("pmap_clear_modify pa=%08lx\n", pa));
	pmap_changebit(pa, 0, ~PT_M);
}


void
pmap_clear_reference(pa)
	vm_offset_t pa;
{
	PDEBUG(0, printf("pmap_clear_reference pa=%08lx\n", pa));
	pmap_changebit(pa, 0, ~PT_H);
}


void
pmap_copy_on_write(pa)
	vm_offset_t pa;
{
	PDEBUG(0, printf("pmap_copy_on_write pa=%08lx\n", pa));
	pmap_changebit(pa, 0, ~PT_Wr);
}

boolean_t
pmap_is_modified(pa)
	vm_offset_t pa;
{
	boolean_t result;
    
	result = pmap_testbit(pa, PT_M);
	PDEBUG(0, printf("pmap_is_modified pa=%08lx %x\n", pa, result));
	return(result);
}


boolean_t
pmap_is_referenced(pa)
	vm_offset_t pa;
{
	boolean_t result;
	
	result = pmap_testbit(pa, PT_H);
	PDEBUG(0, printf("pmap_is_referenced pa=%08lx %x\n", pa, result));
	return(result);
}


int
pmap_modified_emulation(pmap, va)
	pmap_t pmap;
	vm_offset_t va;
{
	pt_entry_t *pte;
	vm_offset_t pa;
	int bank, off;
	struct pv_entry *pv;
	u_int flags;

	/* Get the pte */
	pte = pmap_pte(pmap, va);
	if (!pte)
		return(0);

	/* Extract the physical address of the page */
	pa = pmap_pte_pa(pte);
	if ((bank = vm_physseg_find(atop(pa), &off)) == -1)
		return(0);
	pv = &vm_physmem[bank].pmseg.pvent[off];

	/* Get the current flags for this page. */
	flags = pmap_modify_pv(pmap, va, pv, 0, 0);
	PDEBUG(2, printf("pmap_modified_emulation: flags = %08x\n", flags));

	/*
	 * Do the flags say this page is writable ? If not then it is a
	 * genuine write fault. If yes then the write fault is our fault
	 * as we did not reflect the write access in the PTE. Now we know
	 * a write has occurred we can correct this and also set the
	 * modified bit
	 */
	if (!(flags & PT_Wr))
		return(0);

	PDEBUG(0, printf("pmap_modified_emulation: Got a hit va=%08lx, pte = %p (%08x)\n",
	    va, pte, *pte));
	*pte = *pte | PT_AP(AP_W);
	PDEBUG(0, printf("->(%08x)\n", *pte));
	cpu_tlb_flushID_SE(va);
    
/*	pmap_modify_pv(pmap, va, pv, PT_M, PT_M);*/

	vm_physmem[bank].pmseg.attrs[off] |= PT_M;

	/* Return, indicating the problem has been dealt with */
	return(1);
}


int
pmap_handled_emulation(pmap, va)
	pmap_t pmap;
	vm_offset_t va;
{
	pt_entry_t *pte;
	vm_offset_t pa;
	int bank, off;

	PDEBUG(2, printf("pmap_handled_emulation\n"));

	/* Get the pte */
	pte = pmap_pte(pmap, va);
	if (!pte) {
		PDEBUG(2, printf("no pte\n"));
		return(0);
	}

	PDEBUG(1, printf("*pte=%08x\n", *pte));

	/* Check for a zero pte */
	if (*pte == 0)
		return(0);

	PDEBUG(1, printf("pmap_handled_emulation: non zero pte %08x\n", *pte));

	/* Have we marked a valid pte as invalid ? */
	if (((*pte) & L2_MASK) != L2_INVAL)
		return(0);

	PDEBUG(1, printf("Got an invalid pte\n"));

	/* Extract the physical address of the page */
	pa = pmap_pte_pa(pte);

	if ((bank = vm_physseg_find(atop(pa), &off)) == -1)
		return(0);

	/*
	 * Ok we just enable the pte and mark the attibs as handled
	 */
	PDEBUG(0, printf("pmap_handled_emulation: Got a hit va=%08lx pte = %p (%08x)\n",
	    va, pte, *pte));
	*pte = ((*pte) & ~L2_MASK) | L2_SPAGE;
	PDEBUG(0, printf("->(%08x)\n", *pte));

	cpu_tlb_flushID_SE(va);

	vm_physmem[bank].pmseg.attrs[off] |= PT_H;

	/* Return, indicating the problem has been dealt with */
	return(1);
}

/*
 * pmap_collect: free resources held by a pmap
 *
 * => optional function.
 * => called when a process is swapped out to free memory.
 */

void
pmap_collect(pmap)
	pmap_t pmap;
{
}

/* End of pmap.c */
