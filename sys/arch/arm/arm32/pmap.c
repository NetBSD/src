/*	$NetBSD: pmap.c,v 1.14 2001/07/08 19:44:43 chs Exp $	*/

/*
 * Copyright (c) 2001 Richard Earnshaw
 * Copyright (c) 2001 Christopher Gilbert
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
#include "opt_ddb.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/pool.h>

#include <uvm/uvm.h>

#include <machine/bootconfig.h>
#include <machine/bus.h>
#include <machine/pmap.h>
#include <machine/pcb.h>
#include <machine/param.h>
#include <machine/katelib.h>
       
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

/*
 * pool that pmap structures are allocated from
 */

struct pool pmap_pmap_pool;

pagehook_t page_hook0;
pagehook_t page_hook1;
char *memhook;
pt_entry_t msgbufpte;
extern caddr_t msgbufaddr;

#ifdef DIAGNOSTIC
boolean_t pmap_initialized = FALSE;	/* Has pmap_init completed? */
#endif

TAILQ_HEAD(pv_page_list, pv_page) pv_page_freelist;

int pv_nfree = 0;

vsize_t npages;

extern paddr_t physical_start;
extern paddr_t physical_freestart;
extern paddr_t physical_end;
extern paddr_t physical_freeend;
extern unsigned int free_pages;
extern int max_processes;

vaddr_t virtual_start;
vaddr_t virtual_end;

vaddr_t avail_start;
vaddr_t avail_end;

extern pv_addr_t systempage;

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
pt_entry_t *pmap_pte __P((pmap_t pmap, vaddr_t va));
void map_pagetable __P((vaddr_t pagetable, vaddr_t va,
    paddr_t pa, unsigned int flags));
void pmap_copy_on_write __P((paddr_t pa));
void pmap_pinit __P((pmap_t));
void pmap_freepagedir __P((pmap_t));
void pmap_release __P((pmap_t));

/* Other function prototypes */
extern void bzero_page __P((vaddr_t));
extern void bcopy_page __P((vaddr_t, vaddr_t));

struct l1pt *pmap_alloc_l1pt __P((void));
static __inline void pmap_map_in_l1 __P((pmap_t pmap, vaddr_t va,
     vaddr_t l2pa));

static pt_entry_t *pmap_map_ptes __P((struct pmap *));
/* eventually this will be a function */
#define pmap_unmap_ptes(a)

void pmap_vac_me_harder __P((struct pmap *, struct pv_entry *,
	    pt_entry_t *, boolean_t));

#ifdef MYCROFT_HACK
int mycroft_hack = 0;
#endif

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

boolean_t pmap_isa_dma_range_intersect __P((paddr_t, psize_t,
	    paddr_t *, psize_t *));

/*
 * Check if a memory range intersects with an ISA DMA range, and
 * return the page-rounded intersection if it does.  The intersection
 * will be placed on a lower-priority free list.
 */
boolean_t
pmap_isa_dma_range_intersect(pa, size, pap, sizep)
	paddr_t pa;
	psize_t size;
	paddr_t *pap;
	psize_t *sizep;
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
		/* NOTE: can't lock kernel_map here */
		MALLOC(pvp, struct pv_page *, NBPG, M_VMPVENT, M_WAITOK);
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

	pvp = (struct pv_page *) trunc_page((vaddr_t)pv);
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
		FREE((vaddr_t)pvp, M_VMPVENT);
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
			pv_nfree -= NPVPPG;
			pvp->pvp_pgi.pgi_nfree = -1;
		}
	}

	if (pv_page_collectlist.tqh_first == 0)
		return;

	for (ph = &pv_table[npages - 1]; ph >= &pv_table[0]; ph--) {
		if (ph->pv_pmap == 0)
			continue;
		s = splvm();
		for (ppv = ph; (pv = ppv->pv_next) != 0; ) {
			pvp = (struct pv_page *) trunc_page((vaddr_t)pv);
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
		splx(s);
	}

	for (pvp = pv_page_collectlist.tqh_first; pvp; pvp = npvp) {
		npvp = pvp->pvp_pgi.pgi_list.tqe_next;
		FREE((vaddr_t)pvp, M_VMPVENT);
	}
}
#endif

/*
 * Enter a new physical-virtual mapping into the pv table
 */

/*__inline*/ void
pmap_enter_pv(pmap, va, pv, flags)
	pmap_t pmap;
	vaddr_t va;
	struct pv_entry *pv;
	u_int flags;
{
	struct pv_entry *npv;
	u_int s;

#ifdef DIAGNOSTIC
	if (!pmap_initialized)
		panic("pmap_enter_pv: !pmap_initialized");
#endif

	s = splvm();

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
	} else {
		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 */
#ifdef PMAP_DEBUG
		for (npv = pv; npv; npv = npv->pv_next)
			if (pmap == npv->pv_pmap && va == npv->pv_va)
				panic("pmap_enter_pv: already in pv_tab pv %p: %08lx/%p/%p",
				    pv, pv->pv_va, pv->pv_pmap, pv->pv_next);
#endif
		npv = pmap_alloc_pv();
		/* Must make sure that the new entry is before any others
		 * for the same pmap.  Otherwise the vac handling code
		 * will get confused.
		 * XXX this would be better if we used lists like i386 (infact
		 * this would be a lot simpler)
		 */
		*npv = *pv; 
 		pv->pv_va = va;
 		pv->pv_pmap = pmap;
 		pv->pv_flags = flags;
		pv->pv_next = npv;
	}

	if (flags & PT_W)
		++pmap->pm_stats.wired_count;

	splx(s);
}


/*
 * Remove a physical-virtual mapping from the pv table
 */

/*__inline*/ void
pmap_remove_pv(pmap, va, pv)
	pmap_t pmap;
	vaddr_t va;
	struct pv_entry *pv;
{
	struct pv_entry *npv;
	u_int s;
	u_int flags = 0;
    
#ifdef DIAGNOSTIC
	if (!pmap_initialized)
		panic("pmap_remove_pv: !pmap_initialized");
#endif

	s = splvm();

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
		} else {
			flags = pv->pv_flags;
			pv->pv_pmap = NULL;
		}
	} else {
		for (npv = pv->pv_next; npv; pv = npv, npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && va == npv->pv_va)
				break;
		}
		if (npv) {
			pv->pv_next = npv->pv_next;
			flags = npv->pv_flags;
			pmap_free_pv(npv);
		} else
			panic("pmap_remove_pv: lost entry");
	}

	if (flags & PT_W)
		--pmap->pm_stats.wired_count;

	splx(s);
}

/*
 * Modify a physical-virtual mapping in the pv table
 */

/*__inline */ u_int
pmap_modify_pv(pmap, va, pv, bic_mask, eor_mask)
	pmap_t pmap;
	vaddr_t va;
	struct pv_entry *pv;
	u_int bic_mask;
	u_int eor_mask;
{
	struct pv_entry *npv;
	u_int s;
	u_int flags, oflags;

	PDEBUG(5, printf("pmap_modify_pv(pmap=%p, va=%08lx, pv=%p, bic_mask=%08x, eor_mask=%08x)\n",
	    pmap, va, pv, bic_mask, eor_mask));

#ifdef DIAGNOSTIC
	if (!pmap_initialized)
		panic("pmap_modify_pv: !pmap_initialized");
#endif

	s = splvm();

	PDEBUG(5, printf("pmap_modify_pv: pv %p: %08lx/%p/%p/%08x ",
	    pv, pv->pv_va, pv->pv_pmap, pv->pv_next, pv->pv_flags));

	/*
	 * There is at least one VA mapping this page.
	 */

	for (npv = pv; npv; npv = npv->pv_next) {
		if (pmap == npv->pv_pmap && va == npv->pv_va) {
			oflags = npv->pv_flags;
			npv->pv_flags = flags =
			    ((oflags & ~bic_mask) ^ eor_mask);
			if ((flags ^ oflags) & PT_W) {
				if (flags & PT_W)
					++pmap->pm_stats.wired_count;
				else
					--pmap->pm_stats.wired_count;
			}
			PDEBUG(0, printf("done flags=%08x\n", flags));
			splx(s);
			return (oflags);
		}
	}

	PDEBUG(0, printf("done.\n"));
	splx(s);
	return (0);
}


/*
 * Map the specified level 2 pagetable into the level 1 page table for
 * the given pmap to cover a chunk of virtual address space starting from the
 * address specified.
 */
static /*__inline*/ void
pmap_map_in_l1(pmap, va, l2pa)
	pmap_t pmap;
	vaddr_t va, l2pa;
{
	vaddr_t ptva;

	/* Calculate the index into the L1 page table. */
	ptva = (va >> PDSHIFT) & ~3;

	PDEBUG(0, printf("wiring %08lx in to pd%p pte0x%lx va0x%lx\n", l2pa,
	    pmap->pm_pdir, L1_PTE(l2pa), ptva));

	/* Map page table into the L1. */
	pmap->pm_pdir[ptva + 0] = L1_PTE(l2pa + 0x000);
	pmap->pm_pdir[ptva + 1] = L1_PTE(l2pa + 0x400);
	pmap->pm_pdir[ptva + 2] = L1_PTE(l2pa + 0x800);
	pmap->pm_pdir[ptva + 3] = L1_PTE(l2pa + 0xc00);

	PDEBUG(0, printf("pt self reference %lx in %lx\n",
	    L2_PTE_NC_NB(l2pa, AP_KRW), pmap->pm_vptpt));

	/* Map the page table into the page table area. */
	*((pt_entry_t *)(pmap->pm_vptpt + ptva)) = L2_PTE_NC_NB(l2pa, AP_KRW);

	/* XXX should be a purge */
/*	cpu_tlb_flushD();*/
}

#if 0
static /*__inline*/ void
pmap_unmap_in_l1(pmap, va)
	pmap_t pmap;
	vaddr_t va;
{
	vaddr_t ptva;

	/* Calculate the index into the L1 page table. */
	ptva = (va >> PDSHIFT) & ~3;

	/* Unmap page table from the L1. */
	pmap->pm_pdir[ptva + 0] = 0;
	pmap->pm_pdir[ptva + 1] = 0;
	pmap->pm_pdir[ptva + 2] = 0;
	pmap->pm_pdir[ptva + 3] = 0;

	/* Unmap the page table from the page table area. */
	*((pt_entry_t *)(pmap->pm_vptpt + ptva)) = 0;

	/* XXX should be a purge */
/*	cpu_tlb_flushD();*/
}
#endif


/*
 *	Used to map a range of physical addresses into kernel
 *	virtual address space.
 *
 *	For now, VM is already on, we only need to map the
 *	specified memory.
 */
vaddr_t
pmap_map(va, spa, epa, prot)
	vaddr_t va, spa, epa;
	int prot;
{
	while (spa < epa) {
		pmap_enter(pmap_kernel(), va, spa, prot, 0);
		va += NBPG;
		spa += NBPG;
	}
	pmap_update();
	return(va);
}


/*
 * void pmap_bootstrap(pd_entry_t *kernel_l1pt, pv_addr_t kernel_ptpt)
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
extern paddr_t physical_freestart;
extern paddr_t physical_freeend;

struct pv_entry *boot_pvent;
char *boot_attrs;

void
pmap_bootstrap(kernel_l1pt, kernel_ptpt)
	pd_entry_t *kernel_l1pt;
	pv_addr_t kernel_ptpt;
{
	int loop;
	paddr_t start, end;
#if NISADMA > 0
	paddr_t istart;
	psize_t isize;
#endif
	vsize_t size;

	kernel_pmap = &kernel_pmap_store;

	kernel_pmap->pm_pdir = kernel_l1pt;
	kernel_pmap->pm_pptpt = kernel_ptpt.pv_pa;
	kernel_pmap->pm_vptpt = kernel_ptpt.pv_va;
	simple_lock_init(&kernel_pmap->pm_lock);
	kernel_pmap->pm_count = 1;

	/*
	 * Initialize PAGE_SIZE-dependent variables.
	 */
	uvm_setpagesize();

	npages = 0;
	loop = 0;
	while (loop < bootconfig.dramblocks) {
		start = (paddr_t)bootconfig.dram[loop].address;
		end = start + (bootconfig.dram[loop].pages * NBPG);
		if (start < physical_freestart)
			start = physical_freestart;
		if (end > physical_freeend)
			end = physical_freeend;
#if 0
		printf("%d: %lx -> %lx\n", loop, start, end - 1);
#endif
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
			npages += atop(istart + isize) - atop(istart);
			
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
				npages += atop(istart) - atop(start);
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
				npages += atop(end) - atop(istart + isize);
			}
		} else {
			uvm_page_physload(atop(start), atop(end),
			    atop(start), atop(end), VM_FREELIST_DEFAULT);
			npages += atop(end) - atop(start);
		}
#else	/* NISADMA > 0 */
		uvm_page_physload(atop(start), atop(end),
		    atop(start), atop(end), VM_FREELIST_DEFAULT);
		npages += atop(end) - atop(start);
#endif /* NISADMA > 0 */
		++loop;
	}

#ifdef MYCROFT_HACK
	printf("npages = %ld\n", npages);
#endif

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

	size = npages * sizeof(struct pv_entry);
	boot_pvent = (struct pv_entry *)uvm_pageboot_alloc(size);
	bzero(boot_pvent, size);
	size = npages * sizeof(char);
	boot_attrs = (char *)uvm_pageboot_alloc(size);
	bzero(boot_attrs, size);

	/*
	 * initialize the pmap pool.
	 */

	pool_init(&pmap_pmap_pool, sizeof(struct pmap), 0, 0, 0, "pmappl",
		  0, pool_page_alloc_nointr, pool_page_free_nointr, M_VMPMAP);
	
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
	int lcv;
    
#ifdef MYCROFT_HACK
	printf("physmem = %d\n", physmem);
#endif

	/*
	 * Set the available memory vars - These do not map to real memory
	 * addresses and cannot as the physical memory is fragmented.
	 * They are used by ps for %mem calculations.
	 * One could argue whether this should be the entire memory or just
	 * the memory that is useable in a user process.
	 */
	avail_start = 0;
	avail_end = physmem * NBPG;

	/* Set up pmap info for physsegs. */
	for (lcv = 0; lcv < vm_nphysseg; lcv++) {
		vm_physmem[lcv].pmseg.pvent = boot_pvent;
		boot_pvent += vm_physmem[lcv].end - vm_physmem[lcv].start;
		vm_physmem[lcv].pmseg.attrs = boot_attrs;
		boot_attrs += vm_physmem[lcv].end - vm_physmem[lcv].start;
	}
#ifdef MYCROFT_HACK
	for (lcv = 0 ; lcv < vm_nphysseg ; lcv++) {
		printf("physseg[%d] pvent=%p attrs=%p start=%ld end=%ld\n",
		    lcv,
		    vm_physmem[lcv].pmseg.pvent, vm_physmem[lcv].pmseg.attrs,
		    vm_physmem[lcv].start, vm_physmem[lcv].end);
	}
#endif
	TAILQ_INIT(&pv_page_freelist);

#ifdef DIAGNOSTIC
	/* Now it is safe to enable pv_entry recording. */
	pmap_initialized = TRUE;
#endif
    
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
pmap_create()
{
	pmap_t pmap;

	/*
	 * Fetch pmap entry from the pool
	 */
	
	pmap = pool_get(&pmap_pmap_pool, PR_WAITOK);
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
	paddr_t pa;
	vaddr_t va;
	struct l1pt *pt;
	int error;
	struct vm_page *m;
	pt_entry_t *ptes;

	/* Allocate virtual address space for the L1 page table */
	va = uvm_km_valloc(kernel_map, PD_SIZE);
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
	error = uvm_pglistalloc(PD_SIZE, physical_start, physical_end,
	    PD_SIZE, 0, &pt->pt_plist, 1, M_WAITOK);
	if (error) {
#ifdef DIAGNOSTIC
		printf("pmap: Cannot allocate physical memory for L1 (%d)\n",
		    error);
#endif	/* DIAGNOSTIC */
		/* Release the resources we already have claimed */
		free(pt, M_VMPMAP);
		uvm_km_free(kernel_map, va, PD_SIZE);
		return(NULL);
	}

	/* Map our physical pages into our virtual space */
	pt->pt_va = va;
	m = pt->pt_plist.tqh_first;
	ptes = pmap_map_ptes(pmap_kernel());
	while (m && va < (pt->pt_va + PD_SIZE)) {
		pa = VM_PAGE_TO_PHYS(m);

		pmap_enter(pmap_kernel(), va, pa,
		    VM_PROT_READ | VM_PROT_WRITE, PMAP_WIRED);

		/* Revoke cacheability and bufferability */
		/* XXX should be done better than this */
		ptes[arm_byte_to_page(va)] &= ~(PT_C | PT_B);

		va += NBPG;
		m = m->pageq.tqe_next;
	}
	pmap_unmap_ptes(pmap_kernel());
	pmap_update();

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
	pmap_update();

	/* Return the physical memory */
	uvm_pglistfree(&pt->pt_plist);

	/* Free the virtual space */
	uvm_km_free(kernel_map, pt->pt_va, PD_SIZE);

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
	paddr_t pa;
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
	pmap->pm_vptpt = uvm_km_zalloc(kernel_map, NBPG);
	if (pmap->pm_vptpt == 0) {
		pmap_freepagedir(pmap);
		return(ENOMEM);
	}

	(void) pmap_extract(kernel_pmap, pmap->pm_vptpt, &pmap->pm_pptpt);
	pmap->pm_pptpt &= PG_FRAME;
	/* Revoke cacheability and bufferability */
	/* XXX should be done better than this */
	pte = pmap_pte(kernel_pmap, pmap->pm_vptpt);
	*pte = *pte & ~(PT_C | PT_B);

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
	    VM_PROT_READ, VM_PROT_READ | PMAP_WIRED);
	pmap_update();
}


void
pmap_freepagedir(pmap)
	pmap_t pmap;
{
	/* Free the memory used for the page table mapping */
	if (pmap->pm_vptpt != 0)
		uvm_km_free(kernel_map, (vaddr_t)pmap->pm_vptpt, NBPG);

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
		pool_put(&pmap_pmap_pool, pmap);
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
	pmap_update();

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
			uvm_pagefree(page);
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
 * void pmap_virtual_space(vaddr_t *start, vaddr_t *end)
 *
 * Return the start and end addresses of the kernel's virtual space.
 * These values are setup in pmap_bootstrap and are updated as pages
 * are allocated.
 */

void
pmap_virtual_space(start, end)
	vaddr_t *start;
	vaddr_t *end;
{
	*start = virtual_start;
	*end = virtual_end;
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

	(void) pmap_extract(kernel_pmap, (vaddr_t)pmap->pm_pdir,
	    (paddr_t *)&pcb->pcb_pagedir);

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
	vaddr_t page_to_clean = 0;

	/* Go to splvm() so we get exclusive lock for a mo */
	s = splvm();
	if (pv->pv_pmap) {
		cache_needs_cleaning = 1;
		if (!pv->pv_next)
			page_to_clean = pv->pv_va;
	}
	splx(s);

	/* Do cache ops outside the splvm. */
	if (page_to_clean)
		cpu_cache_purgeID_rng(page_to_clean, NBPG);
	else if (cache_needs_cleaning) {
		cpu_cache_purgeID();
		return (1);
	}
	return (0);
}

/*
 * pmap_find_pv()
 *
 * This is a local function that finds a PV entry for a given physical page.
 * This is a common op, and this function removes loads of ifdefs in the code.
 */
static __inline struct pv_entry *
pmap_find_pv(phys)
	paddr_t phys;
{
	int bank, off;
	struct pv_entry *pv;

#ifdef DIAGNOSTIC
	if (!pmap_initialized)
		panic("pmap_find_pv: !pmap_initialized");
#endif

	if ((bank = vm_physseg_find(atop(phys), &off)) == -1)
		panic("pmap_find_pv: not a real page, phys=%lx\n", phys);
	pv = &vm_physmem[bank].pmseg.pvent[off];
	return (pv);
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
	paddr_t phys;
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
	paddr_t src;
	paddr_t dest;
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
 * int pmap_next_phys_page(paddr_t *addr)
 *
 * Allocate another physical page returning true or false depending
 * on whether a page could be allocated.
 */
 
paddr_t
pmap_next_phys_page(addr)
	paddr_t addr;
	
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

#if 0
void
pmap_pte_addref(pmap, va)
	pmap_t pmap;
	vaddr_t va;
{
	pd_entry_t *pde;
	paddr_t pa;
	struct vm_page *m;

	if (pmap == pmap_kernel())
		return;

	pde = pmap_pde(pmap, va & ~(3 << PDSHIFT));
	pa = pmap_pte_pa(pde);
	m = PHYS_TO_VM_PAGE(pa);
	++m->wire_count;
#ifdef MYCROFT_HACK
	printf("addref pmap=%p va=%08lx pde=%p pa=%08lx m=%p wire=%d\n",
	    pmap, va, pde, pa, m, m->wire_count);
#endif
}

void
pmap_pte_delref(pmap, va)
	pmap_t pmap;
	vaddr_t va;
{
	pd_entry_t *pde;
	paddr_t pa;
	struct vm_page *m;

	if (pmap == pmap_kernel())
		return;

	pde = pmap_pde(pmap, va & ~(3 << PDSHIFT));
	pa = pmap_pte_pa(pde);
	m = PHYS_TO_VM_PAGE(pa);
	--m->wire_count;
#ifdef MYCROFT_HACK
	printf("delref pmap=%p va=%08lx pde=%p pa=%08lx m=%p wire=%d\n",
	    pmap, va, pde, pa, m, m->wire_count);
#endif
	if (m->wire_count == 0) {
#ifdef MYCROFT_HACK
		printf("delref pmap=%p va=%08lx pde=%p pa=%08lx m=%p\n",
		    pmap, va, pde, pa, m);
#endif
		pmap_unmap_in_l1(pmap, va);
		uvm_pagefree(m);
		--pmap->pm_stats.resident_count;
	}
}
#else
#define	pmap_pte_addref(pmap, va)
#define	pmap_pte_delref(pmap, va)
#endif

/*
 * Since we have a virtually indexed cache, we may need to inhibit caching if
 * there is more than one mapping and at least one of them is writable.
 * Since we purge the cache on every context switch, we only need to check for
 * other mappings within the same pmap, or kernel_pmap.
 * This function is also called when a page is unmapped, to possibly reenable
 * caching on any remaining mappings.
 *
 * Note that the pmap must have it's ptes mapped in, and passed with ptes.
 */
void
pmap_vac_me_harder(struct pmap *pmap, struct pv_entry *pv, pt_entry_t *ptes,
	boolean_t clear_cache)
{
	struct pv_entry *npv;
	pt_entry_t *pte;
	int entries = 0;
	int writeable = 0;
	int cacheable_entries = 0;

	if (pv->pv_pmap == NULL)
		return;
	KASSERT(ptes != NULL);

	/*
	 * Count mappings and writable mappings in this pmap.
	 * Keep a pointer to the first one.
	 */
	for (npv = pv; npv; npv = npv->pv_next) {
		/* Count mappings in the same pmap */
		if (pmap == npv->pv_pmap) {
			if (entries++ == 0)
				pv = npv;
			/* Cacheable mappings */
			if ((npv->pv_flags & PT_NC) == 0)
				cacheable_entries++;
			/* Writeable mappings */
			if (npv->pv_flags & PT_Wr)
				++writeable;
		}
	}

	PDEBUG(3,printf("pmap_vac_me_harder: pmap %p Entries %d, "
		"writeable %d cacheable %d %s\n", pmap, entries, writeable,
	    	cacheable_entries, clear_cache ? "clean" : "no clean"));
	
	/*
	 * Enable or disable caching as necessary.
	 * We do a quick check of the first PTE to avoid walking the list if
	 * we're already in the right state.
	 */
	if (entries > 1 && writeable) {
		if (cacheable_entries == 0)
		    return;
		if (pv->pv_flags & PT_NC) {
#ifdef DIAGNOSTIC
    			/* We have cacheable entries, but the first one
 			isn't among them. Something is wrong.  */
    			if (cacheable_entries)
				panic("pmap_vac_me_harder: "
	    				"cacheable inconsistent");
#endif
			return;
		}
		pte =  &ptes[arm_byte_to_page(pv->pv_va)];
		*pte &= ~(PT_C | PT_B);
		pv->pv_flags |= PT_NC;
		if (clear_cache && cacheable_entries < 4) {
			cpu_cache_purgeID_rng(pv->pv_va, NBPG);
			cpu_tlb_flushID_SE(pv->pv_va);
		}
		for (npv = pv->pv_next; npv; npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && 
			    (npv->pv_flags & PT_NC) == 0) {
				ptes[arm_byte_to_page(npv->pv_va)] &= 
				    ~(PT_C | PT_B);
 				npv->pv_flags |= PT_NC;
				if (clear_cache && cacheable_entries < 4) {
					cpu_cache_purgeID_rng(npv->pv_va,
					    NBPG);
					cpu_tlb_flushID_SE(npv->pv_va);
				}
			}
		}
		if (clear_cache && cacheable_entries >= 4) {
			cpu_cache_purgeID();
			cpu_tlb_flushID();
		}
	} else if (entries > 0) {
		if ((pv->pv_flags & PT_NC) == 0)
			return;
		pte = &ptes[arm_byte_to_page(pv->pv_va)];
		*pte |= (PT_C | PT_B);
		pv->pv_flags &= ~PT_NC;
		for (npv = pv->pv_next; npv; npv = npv->pv_next) {
			if (pmap == npv->pv_pmap &&
				(npv->pv_flags & PT_NC)) {
				ptes[arm_byte_to_page(npv->pv_va)] |=
				    (PT_C | PT_B);
				npv->pv_flags &= ~PT_NC;
			}
		}
	}
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
	vaddr_t sva;
	vaddr_t eva;
{
	int cleanlist_idx = 0;
	struct pagelist {
		vaddr_t va;
		pt_entry_t *pte;
	} cleanlist[PMAP_REMOVE_CLEAN_LIST_SIZE];
	pt_entry_t *pte = 0, *ptes;
	paddr_t pa;
	int pmap_active;
	struct pv_entry *pv;

	/* Exit quick if there is no pmap */
	if (!pmap)
		return;

	PDEBUG(0, printf("pmap_remove: pmap=%p sva=%08lx eva=%08lx\n", pmap, sva, eva));

	sva &= PG_FRAME;
	eva &= PG_FRAME;

	ptes = pmap_map_ptes(pmap);
	/* Get a page table pointer */
	while (sva < eva) {
		if (pmap_pde_v(pmap_pde(pmap, sva)))
			break;
		sva = (sva & PD_MASK) + NBPD;
	}
	
	pte = &ptes[arm_byte_to_page(sva)];
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
			--pmap->pm_stats.resident_count;

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
				for (cnt = 0; cnt < PMAP_REMOVE_CLEAN_LIST_SIZE; cnt++) {
					*cleanlist[cnt].pte = 0;
					pmap_pte_delref(pmap, cleanlist[cnt].va);
				}
				*pte = 0;
				pmap_pte_delref(pmap, sva);
				cleanlist_idx++;
			} else {
				/*
				 * We've already nuked the cache and
				 * TLB, so just carry on regardless,
				 * and we won't need to do it again
				 */
				*pte = 0;
				pmap_pte_delref(pmap, sva);
			}

			/*
			 * Update flags. In a number of circumstances,
			 * we could cluster a lot of these and do a
			 * number of sequential pages in one go.
			 */
			if ((bank = vm_physseg_find(atop(pa), &off)) != -1) {
				pv = &vm_physmem[bank].pmseg.pvent[off];
				pmap_remove_pv(pmap, sva, pv);
				pmap_vac_me_harder(pmap, pv, ptes, FALSE);
			}
		}
		sva += NBPG;
		pte++;
	}

	pmap_unmap_ptes(pmap);
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
			pmap_pte_delref(pmap, cleanlist[cnt].va);
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
	paddr_t pa;
{
	struct pv_entry *ph, *pv, *npv;
	pmap_t pmap;
	pt_entry_t *pte, *ptes;
	int s;

	PDEBUG(0, printf("pmap_remove_all: pa=%lx ", pa));

	pv = ph = pmap_find_pv(pa);
	pmap_clean_page(pv);

	s = splvm();

	if (ph->pv_pmap == NULL) {
		PDEBUG(0, printf("free page\n"));
		splx(s);
		return;
	}

	
	
	while (pv) {
		pmap = pv->pv_pmap;
		ptes = pmap_map_ptes(pmap);
		pte = &ptes[arm_byte_to_page(pv->pv_va)];

		PDEBUG(0, printf("[%p,%08x,%08lx,%08x] ", pmap, *pte,
		    pv->pv_va, pv->pv_flags));
#ifdef DEBUG
		if (!pmap_pde_v(pmap_pde(pmap, va)) || !pmap_pte_v(pte)
			    || pmap_pte_pa(pte) != pa)
			panic("pmap_remove_all: bad mapping");
#endif	/* DEBUG */

		/*
		 * Update statistics
		 */
		--pmap->pm_stats.resident_count;

		/* Wired bit */
		if (pv->pv_flags & PT_W)
			--pmap->pm_stats.wired_count;

		/*
		 * Invalidate the PTEs.
		 * XXX: should cluster them up and invalidate as many
		 * as possible at once.
		 */

#ifdef needednotdone
reduce wiring count on page table pages as references drop
#endif

		*pte = 0;
		pmap_pte_delref(pmap, pv->pv_va);

		npv = pv->pv_next;
		if (pv == ph)
			ph->pv_pmap = NULL;
		else
			pmap_free_pv(pv);
		pv = npv;
		pmap_unmap_ptes(pmap);
	}
	
	splx(s);

	PDEBUG(0, printf("done\n"));
	cpu_tlb_flushID();
}


/*
 * Set the physical protection on the specified range of this map as requested.
 */

void
pmap_protect(pmap, sva, eva, prot)
	pmap_t pmap;
	vaddr_t sva;
	vaddr_t eva;
	vm_prot_t prot;
{
	pt_entry_t *pte = NULL, *ptes;
	int armprot;
	int flush = 0;
	paddr_t pa;
	int bank, off;
	struct pv_entry *pv;

	/*
	 * Make sure pmap is valid. -dct
	 */
	if (pmap == NULL)
		return;
	PDEBUG(0, printf("pmap_protect: pmap=%p %08lx->%08lx %x\n",
	    pmap, sva, eva, prot));

	if (~prot & VM_PROT_READ) {
		/* Just remove the mappings. */
		pmap_remove(pmap, sva, eva);
		return;
	}
	if (prot & VM_PROT_WRITE) {
		/*
		 * If this is a read->write transition, just ignore it and let
		 * uvm_fault() take care of it later.
		 */
		return;
	}

	sva &= PG_FRAME;
	eva &= PG_FRAME;

	ptes = pmap_map_ptes(pmap);
	/*
	 * We need to acquire a pointer to a page table page before entering
	 * the following loop.
	 */
	while (sva < eva) {
		if (pmap_pde_v(pmap_pde(pmap, sva)))
			break;
		sva = (sva & PD_MASK) + NBPD;
	}
	
	pte = &ptes[arm_byte_to_page(sva)];

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
		if ((bank = vm_physseg_find(atop(pa), &off)) != -1) {
			pv = &vm_physmem[bank].pmseg.pvent[off];
			(void) pmap_modify_pv(pmap, sva, pv, PT_Wr, 0);
			pmap_vac_me_harder(pmap, pv, ptes, FALSE);
		}

next:
		sva += NBPG;
		pte++;
	}
	pmap_unmap_ptes(pmap);
	if (flush)
		cpu_tlb_flushID();
}

/*
 * void pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot,
 * int flags)
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

int
pmap_enter(pmap, va, pa, prot, flags)
	pmap_t pmap;
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
	int flags;
{
	pt_entry_t *pte, *ptes;
	u_int npte;
	int bank, off;
	struct pv_entry *pv = NULL;
	paddr_t opa;
	int nflags;
	boolean_t wired = (flags & PMAP_WIRED) != 0;

	PDEBUG(5, printf("pmap_enter: V%08lx P%08lx in pmap %p prot=%08x, wired = %d\n",
	    va, pa, pmap, prot, wired));

#ifdef DIAGNOSTIC
	/* Valid address ? */
	if (va >= (KERNEL_VM_BASE + KERNEL_VM_SIZE))
		panic("pmap_enter: too big");
	if (pmap != pmap_kernel() && va != 0) {
		if (va < VM_MIN_ADDRESS || va >= VM_MAXUSER_ADDRESS)
			panic("pmap_enter: kernel page in user map");
	} else {
		if (va >= VM_MIN_ADDRESS && va < VM_MAXUSER_ADDRESS)
			panic("pmap_enter: user page in kernel map");
		if (va >= VM_MAXUSER_ADDRESS && va < VM_MAX_ADDRESS)
			panic("pmap_enter: entering PT page");
	}
#endif

	/*
	 * Get a pointer to the pte for this virtual address. If the
	 * pte pointer is NULL then we are missing the L2 page table
	 * so we need to create one.
	 */
	pte = pmap_pte(pmap, va);
	if (!pte) {
		paddr_t l2pa;
		struct vm_page *m;
	
		/* Allocate a page table */
		for (;;) {
			m = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE);
			if (m != NULL)
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
				panic("pmap_enter: no free pages");
			else
				uvm_wait("pmap_enter");
		}

		/* Wire this page table into the L1. */
		l2pa = VM_PAGE_TO_PHYS(m);
		pmap_zero_page(l2pa);
		pmap_map_in_l1(pmap, va, l2pa);
		++pmap->pm_stats.resident_count;

		pte = pmap_pte(pmap, va);
#ifdef DIAGNOSTIC
		if (!pte)
			panic("pmap_enter: no pte");
#endif
	}

	nflags = 0;
	if (prot & VM_PROT_WRITE)
		nflags |= PT_Wr;
	if (wired)
		nflags |= PT_W;

	/* More debugging info */
	PDEBUG(5, printf("pmap_enter: pte for V%08lx = V%p (%08x)\n", va, pte,
	    *pte));

	/* Is the pte valid ? If so then this page is already mapped */
	if (pmap_pte_v(pte)) {
		/* Get the physical address of the current page mapped */
		opa = pmap_pte_pa(pte);

#ifdef MYCROFT_HACK
		printf("pmap_enter: pmap=%p va=%lx pa=%lx opa=%lx\n", pmap, va, pa, opa);
#endif

		/* Are we mapping the same page ? */
		if (opa == pa) {
			/* All we must be doing is changing the protection */
			PDEBUG(0, printf("Case 02 in pmap_enter (V%08lx P%08lx)\n",
			    va, pa));

			/* Has the wiring changed ? */
			if ((bank = vm_physseg_find(atop(pa), &off)) != -1) {
				pv = &vm_physmem[bank].pmseg.pvent[off];
				(void) pmap_modify_pv(pmap, va, pv,
				    PT_Wr | PT_W, nflags);
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
				pv = &vm_physmem[bank].pmseg.pvent[off];
				pmap_remove_pv(pmap, va, pv);
			}

			goto enter;
		}
	} else {
		opa = 0;
		pmap_pte_addref(pmap, va);

		/* pte is not valid so we must be hooking in a new page */
		++pmap->pm_stats.resident_count;

	enter:
		/*
		 * Enter on the PV list if part of our managed memory
		 */
		if ((bank = vm_physseg_find(atop(pa), &off)) != -1) {
			pv = &vm_physmem[bank].pmseg.pvent[off];
			pmap_enter_pv(pmap, va, pv, nflags);
		}
	}

#ifdef MYCROFT_HACK
	if (mycroft_hack)
		printf("pmap_enter: pmap=%p va=%lx pa=%lx opa=%lx bank=%d off=%d pv=%p\n", pmap, va, pa, opa, bank, off, pv);
#endif

	/* Construct the pte, giving the correct access. */
	npte = (pa & PG_FRAME);

	/* VA 0 is magic. */
	if (pmap != pmap_kernel() && va != 0)
		npte |= PT_AP(AP_U);

	if (bank != -1) {
#ifdef DIAGNOSTIC
		if ((flags & VM_PROT_ALL) & ~prot)
			panic("pmap_enter: access_type exceeds prot");
#endif
		npte |= PT_C | PT_B;
		if (flags & VM_PROT_WRITE) {
			npte |= L2_SPAGE | PT_AP(AP_W);
			vm_physmem[bank].pmseg.attrs[off] |= PT_H | PT_M;
		} else if (flags & VM_PROT_ALL) {
			npte |= L2_SPAGE;
			vm_physmem[bank].pmseg.attrs[off] |= PT_H;
		} else
			npte |= L2_INVAL;
	} else {
		if (prot & VM_PROT_WRITE)
			npte |= L2_SPAGE | PT_AP(AP_W);
		else if (prot & VM_PROT_ALL)
			npte |= L2_SPAGE;
		else
			npte |= L2_INVAL;
	}

#ifdef MYCROFT_HACK
	if (mycroft_hack)
		printf("pmap_enter: pmap=%p va=%lx pa=%lx prot=%x wired=%d access_type=%x npte=%08x\n", pmap, va, pa, prot, wired, flags & VM_PROT_ALL, npte);
#endif

	*pte = npte;

	if (bank != -1)
	{
		boolean_t pmap_active = FALSE;
		/* XXX this will change once the whole of pmap_enter uses
		 * map_ptes
		 */
		ptes = pmap_map_ptes(pmap);
		if ((curproc && curproc->p_vmspace->vm_map.pmap == pmap)
		    || (pmap == kernel_pmap))
			pmap_active = TRUE;
 		pmap_vac_me_harder(pmap, pv, ptes, pmap_active);
		pmap_unmap_ptes(pmap);
	}

	/* Better flush the TLB ... */
	cpu_tlb_flushID_SE(va);

	PDEBUG(5, printf("pmap_enter: pte = V%p %08x\n", pte, *pte));

	return 0;
}

void
pmap_kenter_pa(va, pa, prot)
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
{
	struct pmap *pmap = pmap_kernel();
	pt_entry_t *pte;
	struct vm_page *pg;
 
	if (!pmap_pde_v(pmap_pde(pmap, va))) {

		/* 
		 * For the kernel pmaps it would be better to ensure
		 * that they are always present, and to grow the
		 * kernel as required.
		 */

		/* Allocate a page table */
		pg = uvm_pagealloc(NULL, 0, NULL,
		    UVM_PGA_USERESERVE | UVM_PGA_ZERO);
		if (pg == NULL) {
			panic("pmap_kenter_pa: no free pages");
		}

		/* Wire this page table into the L1. */
		pmap_map_in_l1(pmap, va, VM_PAGE_TO_PHYS(pg));
	}
	pte = vtopte(va);
	KASSERT(!pmap_pte_v(pte));
	*pte = L2_PTE(pa, AP_KRW);
}

void
pmap_kremove(va, len)
	vaddr_t va;
	vsize_t len;
{
	pt_entry_t *pte;

	for (len >>= PAGE_SHIFT; len > 0; len--, va += PAGE_SIZE) {

		/*
		 * We assume that we will only be called with small
		 * regions of memory.
		 */

		KASSERT(pmap_pde_v(pmap_pde(pmap_kernel(), va)));
		pte = vtopte(va);
		cpu_cache_purgeID_rng(va, PAGE_SIZE);
		*pte = 0;
		cpu_tlb_flushID_SE(va);
	}
}

/*
 * pmap_page_protect:
 *
 * Lower the permission for all mappings to a given page.
 */

void
pmap_page_protect(pg, prot)
	struct vm_page *pg;
	vm_prot_t prot;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);

	PDEBUG(0, printf("pmap_page_protect(pa=%lx, prot=%d)\n", pa, prot));

	switch(prot) {
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		pmap_copy_on_write(pa);
		break;

	case VM_PROT_ALL:
		break;

	default:
		pmap_remove_all(pa);
		break;
	}
}


/*
 * Routine:	pmap_unwire
 * Function:	Clear the wired attribute for a map/virtual-address
 *		pair.
 * In/out conditions:
 *		The mapping must already exist in the pmap.
 */

void
pmap_unwire(pmap, va)
	pmap_t pmap;
	vaddr_t va;
{
	pt_entry_t *pte;
	paddr_t pa;
	int bank, off;
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
	(void) pmap_modify_pv(pmap, va, pv, PT_W, 0);
}

/*
 * pt_entry_t *pmap_pte(pmap_t pmap, vaddr_t va)
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
	vaddr_t va;
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
		if (!p->p_vmspace || !p->p_vmspace->vm_map.pmap) {
			printf("pmap_pte: va=%08lx p=%p vm=%p\n",
			    va, p, p->p_vmspace);
			console_debugger();
		}
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
boolean_t
pmap_extract(pmap, va, pap)
	pmap_t pmap;
	vaddr_t va;
	paddr_t *pap;
{
	pt_entry_t *pte, *ptes;
	paddr_t pa;

	PDEBUG(5, printf("pmap_extract: pmap=%p, va=V%08lx\n", pmap, va));

	/*
	 * Get the pte for this virtual address.
	 */
	ptes = pmap_map_ptes(pmap);
	pte = &ptes[arm_byte_to_page(va)]; 

	/*
	 * If there is no pte then there is no page table etc.
	 * Is the pte valid ? If not then no paged is actually mapped here
	 */
	if (!pmap_pde_v(pmap_pde(pmap, va)) || !pmap_pte_v(pte)){
	    pmap_unmap_ptes(pmap);
    	    return (FALSE);
	}

	/* Return the physical address depending on the PTE type */
	/* XXX What about L1 section mappings ? */
	if ((*(pte) & L2_MASK) == L2_LPAGE) {
		/* Extract the physical address from the pte */
		pa = (*(pte)) & ~(L2_LPAGE_SIZE - 1);

		PDEBUG(5, printf("pmap_extract: LPAGE pa = P%08lx\n",
		    (pa | (va & (L2_LPAGE_SIZE - 1)))));

		if (pap != NULL)
			*pap = pa | (va & (L2_LPAGE_SIZE - 1));
	} else {
		/* Extract the physical address from the pte */
		pa = pmap_pte_pa(pte);

		PDEBUG(5, printf("pmap_extract: SPAGE pa = P%08lx\n",
		    (pa | (va & ~PG_FRAME))));

		if (pap != NULL)
			*pap = pa | (va & ~PG_FRAME);
	}
	pmap_unmap_ptes(pmap);
	return (TRUE);
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
	vaddr_t dst_addr;
	vsize_t len;
	vaddr_t src_addr;
{
	PDEBUG(0, printf("pmap_copy(%p, %p, %lx, %lx, %lx)\n",
	    dst_pmap, src_pmap, dst_addr, len, src_addr));
}

#if defined(PMAP_DEBUG)
void
pmap_dump_pvlist(phys, m)
	vaddr_t phys;
	char *m;
{
	struct pv_entry *pv;
	int bank, off;

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
	paddr_t pa;
	int setbits;
{
	int bank, off;

	PDEBUG(1, printf("pmap_testbit: pa=%08lx set=%08x\n", pa, setbits));

	if ((bank = vm_physseg_find(atop(pa), &off)) == -1)
		return(FALSE);

	/*
	 * Check saved info only
	 */
	if (vm_physmem[bank].pmseg.attrs[off] & setbits) {
		PDEBUG(0, printf("pmap_attributes = %02x\n",
		    vm_physmem[bank].pmseg.attrs[off]));
		return(TRUE);
	}

	return(FALSE);
}

static pt_entry_t *
pmap_map_ptes(struct pmap *pmap)
{
    struct proc *p;
   
    /* the kernel's pmap is always accessible */
    if (pmap == pmap_kernel()) {
	return (pt_entry_t *)PROCESS_PAGE_TBLS_BASE ;
    }
    
    if (curproc &&
	    curproc->p_vmspace->vm_map.pmap == pmap)
	return (pt_entry_t *)PROCESS_PAGE_TBLS_BASE;
    
    p = curproc;
    
    if (p == NULL)
	p = &proc0;
    
    pmap_map_in_l1(p->p_vmspace->vm_map.pmap, ALT_PAGE_TBLS_BASE,
	    pmap->pm_pptpt);
    cpu_tlb_flushD();
    return (pt_entry_t *)ALT_PAGE_TBLS_BASE;
}

/*
 * Modify pte bits for all ptes corresponding to the given physical address.
 * We use `maskbits' rather than `clearbits' because we're always passing
 * constants and the latter would require an extra inversion at run-time.
 */

void
pmap_clearbit(pa, maskbits)
	paddr_t pa;
	int maskbits;
{
	struct pv_entry *pv;
	pt_entry_t *pte;
	vaddr_t va;
	int bank, off;
	int s;

	PDEBUG(1, printf("pmap_clearbit: pa=%08lx mask=%08x\n",
	    pa, maskbits));
	if ((bank = vm_physseg_find(atop(pa), &off)) == -1)
		return;
	pv = &vm_physmem[bank].pmseg.pvent[off];
	s = splvm();

	/*
	 * Clear saved attributes (modify, reference)
	 */
	vm_physmem[bank].pmseg.attrs[off] &= ~maskbits;

	if (pv->pv_pmap == NULL) {
		splx(s);
		return;
	}

	/*
	 * Loop over all current mappings setting/clearing as appropos
	 */
	for (; pv; pv = pv->pv_next) {
		va = pv->pv_va;

		/*
		 * XXX don't write protect pager mappings
		 */
		if (va >= uvm.pager_sva && va < uvm.pager_eva) {
			printf("pmap_clearbit: found page VA on pv_list\n");
			continue;
		}

		pv->pv_flags &= ~maskbits;
		pte = pmap_pte(pv->pv_pmap, va);
		if (maskbits & (PT_Wr|PT_M))
			*pte = *pte & ~PT_AP(AP_W);
		if (maskbits & PT_H)
			*pte = (*pte & ~L2_MASK) | L2_INVAL;
	}
	cpu_tlb_flushID();

	splx(s);
}


boolean_t
pmap_clear_modify(pg)
	struct vm_page *pg;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	boolean_t rv;

	PDEBUG(0, printf("pmap_clear_modify pa=%08lx\n", pa));
	rv = pmap_testbit(pa, PT_M);
	pmap_clearbit(pa, PT_M);
	return rv;
}


boolean_t
pmap_clear_reference(pg)
	struct vm_page *pg;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	boolean_t rv;

	PDEBUG(0, printf("pmap_clear_reference pa=%08lx\n", pa));
	rv = pmap_testbit(pa, PT_H);
	pmap_clearbit(pa, PT_H);
	return rv;
}


void
pmap_copy_on_write(pa)
	paddr_t pa;
{
	PDEBUG(0, printf("pmap_copy_on_write pa=%08lx\n", pa));
	pmap_clearbit(pa, PT_Wr);
}


boolean_t
pmap_is_modified(pg)
	struct vm_page *pg;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	boolean_t result;
    
	result = pmap_testbit(pa, PT_M);
	PDEBUG(0, printf("pmap_is_modified pa=%08lx %x\n", pa, result));
	return (result);
}


boolean_t
pmap_is_referenced(pg)
	struct vm_page *pg;
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	boolean_t result;
	
	result = pmap_testbit(pa, PT_H);
	PDEBUG(0, printf("pmap_is_referenced pa=%08lx %x\n", pa, result));
	return (result);
}


int
pmap_modified_emulation(pmap, va)
	pmap_t pmap;
	vaddr_t va;
{
	pt_entry_t *pte;
	paddr_t pa;
	int bank, off;
	struct pv_entry *pv;
	u_int flags;

	PDEBUG(2, printf("pmap_modified_emulation\n"));

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

	/* This can happen if user code tries to access kernel memory. */
	if ((*pte & PT_AP(AP_W)) != 0)
		return (0);

	/* Extract the physical address of the page */
	pa = pmap_pte_pa(pte);
	if ((bank = vm_physseg_find(atop(pa), &off)) == -1)
		return(0);

	/* Get the current flags for this page. */
	pv = &vm_physmem[bank].pmseg.pvent[off];
	flags = pmap_modify_pv(pmap, va, pv, 0, 0);
	PDEBUG(2, printf("pmap_modified_emulation: flags = %08x\n", flags));

	/*
	 * Do the flags say this page is writable ? If not then it is a
	 * genuine write fault. If yes then the write fault is our fault
	 * as we did not reflect the write access in the PTE. Now we know
	 * a write has occurred we can correct this and also set the
	 * modified bit
	 */
	if (~flags & PT_Wr)
		return(0);

	PDEBUG(0, printf("pmap_modified_emulation: Got a hit va=%08lx, pte = %p (%08x)\n",
	    va, pte, *pte));
	vm_physmem[bank].pmseg.attrs[off] |= PT_H | PT_M;
	*pte = (*pte & ~L2_MASK) | L2_SPAGE | PT_AP(AP_W);
	PDEBUG(0, printf("->(%08x)\n", *pte));

	/* Return, indicating the problem has been dealt with */
	cpu_tlb_flushID_SE(va);
	return(1);
}


int
pmap_handled_emulation(pmap, va)
	pmap_t pmap;
	vaddr_t va;
{
	pt_entry_t *pte;
	paddr_t pa;
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

	/* This can happen if user code tries to access kernel memory. */
	if ((*pte & L2_MASK) != L2_INVAL)
		return (0);

	/* Extract the physical address of the page */
	pa = pmap_pte_pa(pte);
	if ((bank = vm_physseg_find(atop(pa), &off)) == -1)
		return(0);

	/*
	 * Ok we just enable the pte and mark the attibs as handled
	 */
	PDEBUG(0, printf("pmap_handled_emulation: Got a hit va=%08lx pte = %p (%08x)\n",
	    va, pte, *pte));
	vm_physmem[bank].pmseg.attrs[off] |= PT_H;
	*pte = (*pte & ~L2_MASK) | L2_SPAGE;
	PDEBUG(0, printf("->(%08x)\n", *pte));

	/* Return, indicating the problem has been dealt with */
	cpu_tlb_flushID_SE(va);
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

/*
 * Routine:	pmap_procwr
 *
 * Function:
 *	Synchronize caches corresponding to [addr, addr+len) in p.
 *
 */
void
pmap_procwr(p, va, len)
	struct proc	*p;
	vaddr_t		va;
	int		len;
{
	/* We only need to do anything if it is the current process. */
	if (p == curproc)
		cpu_cache_syncI_rng(va, len);
}

/* End of pmap.c */
