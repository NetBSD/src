/*	$NetBSD: pmap.c,v 1.18 2001/08/11 14:47:56 chris Exp $	*/

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
#include <sys/cdefs.h>
 
#include <uvm/uvm.h>

#include <machine/bootconfig.h>
#include <machine/bus.h>
#include <machine/pmap.h>
#include <machine/pcb.h>
#include <machine/param.h>
#include <machine/katelib.h>

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.18 2001/08/11 14:47:56 chris Exp $");        
#ifdef PMAP_DEBUG
#define	PDEBUG(_lev_,_stat_) \
	if (pmap_debug_level >= (_lev_)) \
        	((_stat_))
int pmap_debug_level = -2;

/*
 * for switching to potentially finer grained debugging
 */
#define	PDB_FOLLOW	0x0001
#define	PDB_INIT	0x0002
#define	PDB_ENTER	0x0004
#define	PDB_REMOVE	0x0008
#define	PDB_CREATE	0x0010
#define	PDB_PTPAGE	0x0020
#define	PDB_ASN		0x0040
#define	PDB_BITS	0x0080
#define	PDB_COLLECT	0x0100
#define	PDB_PROTECT	0x0200
#define	PDB_BOOTSTRAP	0x1000
#define	PDB_PARANOIA	0x2000
#define	PDB_WIRING	0x4000
#define	PDB_PVDUMP	0x8000

int debugmap = 0;
int pmapdebug = PDB_PARANOIA | PDB_FOLLOW;
#define	NPDEBUG(_lev_,_stat_) \
	if (pmapdebug & (_lev_)) \
        	((_stat_))
    
#else	/* PMAP_DEBUG */
#define	PDEBUG(_lev_,_stat_) /* Nothing */
#define PDEBUG(_lev_,_stat_) /* Nothing */
#endif	/* PMAP_DEBUG */

struct pmap     kernel_pmap_store;

/*
 * pool that pmap structures are allocated from
 */

struct pool pmap_pmap_pool;

pagehook_t page_hook0;
pagehook_t page_hook1;
char *memhook;
pt_entry_t msgbufpte;
extern caddr_t msgbufaddr;

boolean_t pmap_initialized = FALSE;	/* Has pmap_init completed? */
/*
 * locking data structures
 */

static struct lock pmap_main_lock;
static struct simplelock pvalloc_lock;
#ifdef LOCKDEBUG
#define PMAP_MAP_TO_HEAD_LOCK() \
     (void) spinlockmgr(&pmap_main_lock, LK_SHARED, NULL)
#define PMAP_MAP_TO_HEAD_UNLOCK() \
     (void) spinlockmgr(&pmap_main_lock, LK_RELEASE, NULL)

#define PMAP_HEAD_TO_MAP_LOCK() \
     (void) spinlockmgr(&pmap_main_lock, LK_EXCLUSIVE, NULL)
#define PMAP_HEAD_TO_MAP_UNLOCK() \
     (void) spinlockmgr(&pmap_main_lock, LK_RELEASE, NULL)
#else
#define	PMAP_MAP_TO_HEAD_LOCK()		/* nothing */
#define	PMAP_MAP_TO_HEAD_UNLOCK()	/* nothing */
#define	PMAP_HEAD_TO_MAP_LOCK()		/* nothing */
#define	PMAP_HEAD_TO_MAP_UNLOCK()	/* nothing */
#endif /* LOCKDEBUG */
   
/*
 * pv_page management structures: locked by pvalloc_lock
 */

TAILQ_HEAD(pv_pagelist, pv_page);
static struct pv_pagelist pv_freepages;	/* list of pv_pages with free entrys */
static struct pv_pagelist pv_unusedpgs; /* list of unused pv_pages */
static int pv_nfpvents;			/* # of free pv entries */
static struct pv_page *pv_initpage;	/* bootstrap page from kernel_map */
static vaddr_t pv_cachedva;		/* cached VA for later use */

#define PVE_LOWAT (PVE_PER_PVPAGE / 2)	/* free pv_entry low water mark */
#define PVE_HIWAT (PVE_LOWAT + (PVE_PER_PVPAGE * 2))
					/* high water mark */

/*
 * local prototypes
 */

static struct pv_entry	*pmap_add_pvpage __P((struct pv_page *, boolean_t));
static struct pv_entry	*pmap_alloc_pv __P((struct pmap *, int)); /* see codes below */
#define ALLOCPV_NEED	0	/* need PV now */
#define ALLOCPV_TRY	1	/* just try to allocate, don't steal */
#define ALLOCPV_NONEED	2	/* don't need PV, just growing cache */
static struct pv_entry	*pmap_alloc_pvpage __P((struct pmap *, int));
static void		 pmap_enter_pv __P((struct pv_head *,
					    struct pv_entry *, struct pmap *,
					    vaddr_t, struct vm_page *, int));
static void		 pmap_free_pv __P((struct pmap *, struct pv_entry *));
static void		 pmap_free_pvs __P((struct pmap *, struct pv_entry *));
static void		 pmap_free_pv_doit __P((struct pv_entry *));
static void		 pmap_free_pvpage __P((void));
static boolean_t	 pmap_is_curpmap __P((struct pmap *));
static struct pv_entry	*pmap_remove_pv __P((struct pv_head *, struct pmap *, 
			vaddr_t));
#define PMAP_REMOVE_ALL		0	/* remove all mappings */
#define PMAP_REMOVE_SKIPWIRED	1	/* skip wired mappings */

vsize_t npages;

static struct vm_page	*pmap_alloc_ptp __P((struct pmap *, vaddr_t, boolean_t));
static struct vm_page	*pmap_get_ptp __P((struct pmap *, vaddr_t, boolean_t));

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
	x.pte = (pt_entry_t *)pmap_pte(pmap_kernel(), virtual_start); \
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
pt_entry_t *pmap_pte __P((struct pmap *pmap, vaddr_t va));
void map_pagetable __P((vaddr_t pagetable, vaddr_t va,
    paddr_t pa, unsigned int flags));
void pmap_copy_on_write __P((paddr_t pa));
void pmap_pinit __P((struct pmap *));
void pmap_freepagedir __P((struct pmap *));

/* Other function prototypes */
extern void bzero_page __P((vaddr_t));
extern void bcopy_page __P((vaddr_t, vaddr_t));

struct l1pt *pmap_alloc_l1pt __P((void));
static __inline void pmap_map_in_l1 __P((struct pmap *pmap, vaddr_t va,
     vaddr_t l2pa, boolean_t));

static pt_entry_t *pmap_map_ptes __P((struct pmap *));
static void pmap_unmap_ptes __P((struct pmap *));

void pmap_vac_me_harder __P((struct pmap *, struct pv_head *,
	    pt_entry_t *, boolean_t));

/*
 * real definition of pv_entry.
 */

struct pv_entry {
	struct pv_entry *pv_next;       /* next pv_entry */
	struct pmap     *pv_pmap;        /* pmap where mapping lies */
	vaddr_t         pv_va;          /* virtual address for mapping */
	int             pv_flags;       /* flags */
	struct vm_page	*pv_ptp;	/* vm_page for the ptp */
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

#define PVE_PER_PVPAGE ((NBPG - sizeof(struct pv_page_info)) / \
			sizeof(struct pv_entry))

/*
 * a pv_page: where pv_entrys are allocated from
 */

struct pv_page {
	struct pv_page_info pvinfo;
	struct pv_entry pvents[PVE_PER_PVPAGE];
};

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

__inline boolean_t
pmap_is_curpmap(struct pmap *pmap)
{
    if ((curproc && curproc->p_vmspace->vm_map.pmap == pmap)
	    || (pmap == pmap_kernel()))
	return (TRUE);
    return (FALSE);
}
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
 * p v _ e n t r y   f u n c t i o n s
 */

/*
 * pv_entry allocation functions:
 *   the main pv_entry allocation functions are:
 *     pmap_alloc_pv: allocate a pv_entry structure
 *     pmap_free_pv: free one pv_entry
 *     pmap_free_pvs: free a list of pv_entrys
 *
 * the rest are helper functions
 */

/*
 * pmap_alloc_pv: inline function to allocate a pv_entry structure
 * => we lock pvalloc_lock
 * => if we fail, we call out to pmap_alloc_pvpage
 * => 3 modes:
 *    ALLOCPV_NEED   = we really need a pv_entry, even if we have to steal it
 *    ALLOCPV_TRY    = we want a pv_entry, but not enough to steal
 *    ALLOCPV_NONEED = we are trying to grow our free list, don't really need
 *			one now
 *
 * "try" is for optional functions like pmap_copy().
 */

__inline static struct pv_entry *
pmap_alloc_pv(pmap, mode)
	struct pmap *pmap;
	int mode;
{
	struct pv_page *pvpage;
	struct pv_entry *pv;

	simple_lock(&pvalloc_lock);

	if (pv_freepages.tqh_first != NULL) {
		pvpage = pv_freepages.tqh_first;
		pvpage->pvinfo.pvpi_nfree--;
		if (pvpage->pvinfo.pvpi_nfree == 0) {
			/* nothing left in this one? */
			TAILQ_REMOVE(&pv_freepages, pvpage, pvinfo.pvpi_list);
		}
		pv = pvpage->pvinfo.pvpi_pvfree;
#ifdef DIAGNOSTIC
		if (pv == NULL)
			panic("pmap_alloc_pv: pvpi_nfree off");
#endif
		pvpage->pvinfo.pvpi_pvfree = pv->pv_next;
		pv_nfpvents--;  /* took one from pool */
	} else {
		pv = NULL;		/* need more of them */
	}

	/*
	 * if below low water mark or we didn't get a pv_entry we try and
	 * create more pv_entrys ...
	 */

	if (pv_nfpvents < PVE_LOWAT || pv == NULL) {
		if (pv == NULL)
			pv = pmap_alloc_pvpage(pmap, (mode == ALLOCPV_TRY) ?
					       mode : ALLOCPV_NEED);
		else
			(void) pmap_alloc_pvpage(pmap, ALLOCPV_NONEED);
	}

	simple_unlock(&pvalloc_lock);
	return(pv);
}

/*
 * pmap_alloc_pvpage: maybe allocate a new pvpage
 *
 * if need_entry is false: try and allocate a new pv_page
 * if need_entry is true: try and allocate a new pv_page and return a
 *	new pv_entry from it.   if we are unable to allocate a pv_page
 *	we make a last ditch effort to steal a pv_page from some other
 *	mapping.    if that fails, we panic...
 *
 * => we assume that the caller holds pvalloc_lock
 */

static struct pv_entry *
pmap_alloc_pvpage(pmap, mode)
	struct pmap *pmap;
	int mode;
{
	struct vm_page *pg;
	struct pv_page *pvpage;
	struct pv_entry *pv;
	int s;

	/*
	 * if we need_entry and we've got unused pv_pages, allocate from there
	 */

	if (mode != ALLOCPV_NONEED && pv_unusedpgs.tqh_first != NULL) {

		/* move it to pv_freepages list */
		pvpage = pv_unusedpgs.tqh_first;
		TAILQ_REMOVE(&pv_unusedpgs, pvpage, pvinfo.pvpi_list);
		TAILQ_INSERT_HEAD(&pv_freepages, pvpage, pvinfo.pvpi_list);

		/* allocate a pv_entry */
		pvpage->pvinfo.pvpi_nfree--;	/* can't go to zero */
		pv = pvpage->pvinfo.pvpi_pvfree;
#ifdef DIAGNOSTIC
		if (pv == NULL)
			panic("pmap_alloc_pvpage: pvpi_nfree off");
#endif
		pvpage->pvinfo.pvpi_pvfree = pv->pv_next;

		pv_nfpvents--;  /* took one from pool */
		return(pv);
	}

	/*
	 *  see if we've got a cached unmapped VA that we can map a page in.
	 * if not, try to allocate one.
	 */

	s = splvm();   /* must protect kmem_map/kmem_object with splvm! */
	if (pv_cachedva == 0) {
		pv_cachedva = uvm_km_kmemalloc(kmem_map, uvmexp.kmem_object,
		    PAGE_SIZE, UVM_KMF_TRYLOCK|UVM_KMF_VALLOC);
		if (pv_cachedva == 0) {
			splx(s);
			return (NULL);
		}
	}

	/*
	 * we have a VA, now let's try and allocate a page in the object
	 * note: we are still holding splvm to protect kmem_object
	 */

	if (!simple_lock_try(&uvmexp.kmem_object->vmobjlock)) {
		splx(s);
		return (NULL);
	}

	pg = uvm_pagealloc(uvmexp.kmem_object, pv_cachedva -
			   vm_map_min(kernel_map),
			   NULL, UVM_PGA_USERESERVE);
	if (pg)
		pg->flags &= ~PG_BUSY;	/* never busy */

	simple_unlock(&uvmexp.kmem_object->vmobjlock);
	splx(s);
	/* splvm now dropped */

	if (pg == NULL)
		return (NULL);

	/*
	 * add a mapping for our new pv_page and free its entrys (save one!)
	 *
	 * NOTE: If we are allocating a PV page for the kernel pmap, the
	 * pmap is already locked!  (...but entering the mapping is safe...)
	 */

	pmap_kenter_pa(pv_cachedva, VM_PAGE_TO_PHYS(pg), VM_PROT_ALL);
	pmap_update();
	pvpage = (struct pv_page *) pv_cachedva;
	pv_cachedva = 0;
	return (pmap_add_pvpage(pvpage, mode != ALLOCPV_NONEED));
}

/*
 * pmap_add_pvpage: add a pv_page's pv_entrys to the free list
 *
 * => caller must hold pvalloc_lock
 * => if need_entry is true, we allocate and return one pv_entry
 */

static struct pv_entry *
pmap_add_pvpage(pvp, need_entry)
	struct pv_page *pvp;
	boolean_t need_entry;
{
	int tofree, lcv;

	/* do we need to return one? */
	tofree = (need_entry) ? PVE_PER_PVPAGE - 1 : PVE_PER_PVPAGE;

	pvp->pvinfo.pvpi_pvfree = NULL;
	pvp->pvinfo.pvpi_nfree = tofree;
	for (lcv = 0 ; lcv < tofree ; lcv++) {
		pvp->pvents[lcv].pv_next = pvp->pvinfo.pvpi_pvfree;
		pvp->pvinfo.pvpi_pvfree = &pvp->pvents[lcv];
	}
	if (need_entry)
		TAILQ_INSERT_TAIL(&pv_freepages, pvp, pvinfo.pvpi_list);
	else
		TAILQ_INSERT_TAIL(&pv_unusedpgs, pvp, pvinfo.pvpi_list);
	pv_nfpvents += tofree;
	return((need_entry) ? &pvp->pvents[lcv] : NULL);
}

/*
 * pmap_free_pv_doit: actually free a pv_entry
 *
 * => do not call this directly!  instead use either
 *    1. pmap_free_pv ==> free a single pv_entry
 *    2. pmap_free_pvs => free a list of pv_entrys
 * => we must be holding pvalloc_lock
 */

__inline static void
pmap_free_pv_doit(pv)
	struct pv_entry *pv;
{
	struct pv_page *pvp;

	pvp = (struct pv_page *) arm_trunc_page((vaddr_t)pv);
	pv_nfpvents++;
	pvp->pvinfo.pvpi_nfree++;

	/* nfree == 1 => fully allocated page just became partly allocated */
	if (pvp->pvinfo.pvpi_nfree == 1) {
		TAILQ_INSERT_HEAD(&pv_freepages, pvp, pvinfo.pvpi_list);
	}

	/* free it */
	pv->pv_next = pvp->pvinfo.pvpi_pvfree;
	pvp->pvinfo.pvpi_pvfree = pv;

	/*
	 * are all pv_page's pv_entry's free?  move it to unused queue.
	 */

	if (pvp->pvinfo.pvpi_nfree == PVE_PER_PVPAGE) {
		TAILQ_REMOVE(&pv_freepages, pvp, pvinfo.pvpi_list);
		TAILQ_INSERT_HEAD(&pv_unusedpgs, pvp, pvinfo.pvpi_list);
	}
}

/*
 * pmap_free_pv: free a single pv_entry
 *
 * => we gain the pvalloc_lock
 */

__inline static void
pmap_free_pv(pmap, pv)
	struct pmap *pmap;
	struct pv_entry *pv;
{
	simple_lock(&pvalloc_lock);
	pmap_free_pv_doit(pv);

	/*
	 * Can't free the PV page if the PV entries were associated with
	 * the kernel pmap; the pmap is already locked.
	 */
	if (pv_nfpvents > PVE_HIWAT && pv_unusedpgs.tqh_first != NULL &&
	    pmap != pmap_kernel())
		pmap_free_pvpage();

	simple_unlock(&pvalloc_lock);
}

/*
 * pmap_free_pvs: free a list of pv_entrys
 *
 * => we gain the pvalloc_lock
 */

__inline static void
pmap_free_pvs(pmap, pvs)
	struct pmap *pmap;
	struct pv_entry *pvs;
{
	struct pv_entry *nextpv;

	simple_lock(&pvalloc_lock);

	for ( /* null */ ; pvs != NULL ; pvs = nextpv) {
		nextpv = pvs->pv_next;
		pmap_free_pv_doit(pvs);
	}

	/*
	 * Can't free the PV page if the PV entries were associated with
	 * the kernel pmap; the pmap is already locked.
	 */
	if (pv_nfpvents > PVE_HIWAT && pv_unusedpgs.tqh_first != NULL &&
	    pmap != pmap_kernel())
		pmap_free_pvpage();

	simple_unlock(&pvalloc_lock);
}


/*
 * pmap_free_pvpage: try and free an unused pv_page structure
 *
 * => assume caller is holding the pvalloc_lock and that
 *	there is a page on the pv_unusedpgs list
 * => if we can't get a lock on the kmem_map we try again later
 * => note: analysis of MI kmem_map usage [i.e. malloc/free] shows
 *	that if we can lock the kmem_map then we are not already
 *	holding kmem_object's lock.
 */

static void
pmap_free_pvpage()
{
	int s;
	struct vm_map *map;
	struct vm_map_entry *dead_entries;
	struct pv_page *pvp;

	s = splvm(); /* protect kmem_map */

	pvp = pv_unusedpgs.tqh_first;

	/*
	 * note: watch out for pv_initpage which is allocated out of
	 * kernel_map rather than kmem_map.
	 */
	if (pvp == pv_initpage)
		map = kernel_map;
	else
		map = kmem_map;

	if (vm_map_lock_try(map)) {

		/* remove pvp from pv_unusedpgs */
		TAILQ_REMOVE(&pv_unusedpgs, pvp, pvinfo.pvpi_list);

		/* unmap the page */
		dead_entries = NULL;
		uvm_unmap_remove(map, (vaddr_t)pvp, ((vaddr_t)pvp) + PAGE_SIZE,
		    &dead_entries);
		vm_map_unlock(map);

		if (dead_entries != NULL)
			uvm_unmap_detach(dead_entries, 0);

		pv_nfpvents -= PVE_PER_PVPAGE;  /* update free count */
	}

	if (pvp == pv_initpage)
		/* no more initpage, we've freed it */
		pv_initpage = NULL;

	splx(s);
}

/*
 * main pv_entry manipulation functions:
 *   pmap_enter_pv: enter a mapping onto a pv_head list
 *   pmap_remove_pv: remove a mappiing from a pv_head list
 *
 * NOTE: pmap_enter_pv expects to lock the pvh itself
 *       pmap_remove_pv expects te caller to lock the pvh before calling
 */

/*
 * pmap_enter_pv: enter a mapping onto a pv_head lst
 *
 * => caller should hold the proper lock on pmap_main_lock
 * => caller should have pmap locked
 * => we will gain the lock on the pv_head and allocate the new pv_entry
 * => caller should adjust ptp's wire_count before calling
 * => caller should not adjust pmap's wire_count
 */

__inline static void
pmap_enter_pv(pvh, pve, pmap, va, ptp, flags)
	struct pv_head *pvh;
	struct pv_entry *pve;	/* preallocated pve for us to use */
	struct pmap *pmap;
	vaddr_t va;
	struct vm_page *ptp;	/* PTP in pmap that maps this VA */
	int flags;
{
	pve->pv_pmap = pmap;
	pve->pv_va = va;
	pve->pv_ptp = ptp;			/* NULL for kernel pmap */
	pve->pv_flags = flags;
	simple_lock(&pvh->pvh_lock);		/* lock pv_head */
	pve->pv_next = pvh->pvh_list;		/* add to ... */
	pvh->pvh_list = pve;			/* ... locked list */
	simple_unlock(&pvh->pvh_lock);		/* unlock, done! */
	if (pve->pv_flags & PT_W)
		++pmap->pm_stats.wired_count;
}

/*
 * pmap_remove_pv: try to remove a mapping from a pv_list
 *
 * => caller should hold proper lock on pmap_main_lock
 * => pmap should be locked
 * => caller should hold lock on pv_head [so that attrs can be adjusted]
 * => caller should adjust ptp's wire_count and free PTP if needed
 * => caller should NOT adjust pmap's wire_count
 * => we return the removed pve
 */

__inline static struct pv_entry *
pmap_remove_pv(pvh, pmap, va)
	struct pv_head *pvh;
	struct pmap *pmap;
	vaddr_t va;
{
	struct pv_entry *pve, **prevptr;

	prevptr = &pvh->pvh_list;		/* previous pv_entry pointer */
	pve = *prevptr;
	while (pve) {
		if (pve->pv_pmap == pmap && pve->pv_va == va) {	/* match? */
			*prevptr = pve->pv_next;		/* remove it! */
			if (pve->pv_flags & PT_W)
			    --pmap->pm_stats.wired_count;
			break;
		}
		prevptr = &pve->pv_next;		/* previous pointer */
		pve = pve->pv_next;			/* advance */
	}
	return(pve);				/* return removed pve */
}

/*
 *
 * pmap_modify_pv: Update pv flags
 *
 * => caller should hold lock on pv_head [so that attrs can be adjusted]
 * => caller should NOT adjust pmap's wire_count
 * => we return the old flags
 * 
 * Modify a physical-virtual mapping in the pv table
 */

/*__inline */ u_int
pmap_modify_pv(pmap, va, pvh, bic_mask, eor_mask)
	struct pmap *pmap;
	vaddr_t va;
	struct pv_head *pvh;
	u_int bic_mask;
	u_int eor_mask;
{
	struct pv_entry *npv;
	u_int flags, oflags;

	/*
	 * There is at least one VA mapping this page.
	 */

	for (npv = pvh->pvh_list; npv; npv = npv->pv_next) {
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
			return (oflags);
		}
	}
	return (0);
}


/*
 * Map the specified level 2 pagetable into the level 1 page table for
 * the given pmap to cover a chunk of virtual address space starting from the
 * address specified.
 */
static /*__inline*/ void
pmap_map_in_l1(pmap, va, l2pa, selfref)
	struct pmap *pmap;
	vaddr_t va, l2pa;
	boolean_t selfref;
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
	if (selfref) {
		*((pt_entry_t *)(pmap->pm_vptpt + ptva)) =
			L2_PTE_NC_NB(l2pa, AP_KRW);
	}
	/* XXX should be a purge */
/*	cpu_tlb_flushD();*/
}

#if 0
static /*__inline*/ void
pmap_unmap_in_l1(pmap, va)
	struct pmap *pmap;
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

char *boot_head;

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

	pmap_kernel()->pm_pdir = kernel_l1pt;
	pmap_kernel()->pm_pptpt = kernel_ptpt.pv_pa;
	pmap_kernel()->pm_vptpt = kernel_ptpt.pv_va;
	simple_lock_init(&pmap_kernel()->pm_lock);
	pmap_kernel()->pm_obj.pgops = NULL;
	TAILQ_INIT(&(pmap_kernel()->pm_obj.memq));
	pmap_kernel()->pm_obj.uo_npages = 0;
	pmap_kernel()->pm_obj.uo_refs = 1;
	
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
	msgbufpte = (pt_entry_t)pmap_pte(pmap_kernel(), virtual_start);
	virtual_start += round_page(MSGBUFSIZE);

	/*
	 * init the static-global locks and global lists.
	 */
	spinlockinit(&pmap_main_lock, "pmaplk", 0);
	simple_lock_init(&pvalloc_lock);
	TAILQ_INIT(&pv_freepages);
	TAILQ_INIT(&pv_unusedpgs);

	/*
	 * compute the number of pages we have and then allocate RAM
	 * for each pages' pv_head and saved attributes.
	 */
	{
	       	int npages, lcv;
		vsize_t s;

		npages = 0;
		for (lcv = 0 ; lcv < vm_nphysseg ; lcv++)
			npages += (vm_physmem[lcv].end - vm_physmem[lcv].start);
		s = (vsize_t) (sizeof(struct pv_head) * npages +
				sizeof(char) * npages);
		s = round_page(s); /* round up */
		boot_head = (char *)uvm_pageboot_alloc(s);
		bzero((char *)boot_head, s);
		if (boot_head == 0)
			panic("pmap_init: unable to allocate pv_heads");
	}
	
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
	int lcv, i;
    
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

	/* allocate pv_head stuff first */
	for (lcv = 0 ; lcv < vm_nphysseg ; lcv++) {
		vm_physmem[lcv].pmseg.pvhead = (struct pv_head *)boot_head;
		boot_head = (char *)(vaddr_t)(vm_physmem[lcv].pmseg.pvhead +
				 (vm_physmem[lcv].end - vm_physmem[lcv].start));
		for (i = 0;
		     i < (vm_physmem[lcv].end - vm_physmem[lcv].start); i++) {
			simple_lock_init(
			    &vm_physmem[lcv].pmseg.pvhead[i].pvh_lock);
		}
	}

	/* now allocate attrs */
	for (lcv = 0 ; lcv < vm_nphysseg ; lcv++) {
		vm_physmem[lcv].pmseg.attrs = (char *) boot_head;
		boot_head = (char *)(vaddr_t)(vm_physmem[lcv].pmseg.attrs +
				 (vm_physmem[lcv].end - vm_physmem[lcv].start));
	}

	/*
	 * now we need to free enough pv_entry structures to allow us to get
	 * the kmem_map/kmem_object allocated and inited (done after this
	 * function is finished).  to do this we allocate one bootstrap page out
	 * of kernel_map and use it to provide an initial pool of pv_entry
	 * structures.   we never free this page.
	 */

	pv_initpage = (struct pv_page *) uvm_km_alloc(kernel_map, PAGE_SIZE);
	if (pv_initpage == NULL)
		panic("pmap_init: pv_initpage");
	pv_cachedva = 0;   /* a VA we have allocated but not used yet */
	pv_nfpvents = 0;
	(void) pmap_add_pvpage(pv_initpage, FALSE);

#ifdef MYCROFT_HACK
	for (lcv = 0 ; lcv < vm_nphysseg ; lcv++) {
		printf("physseg[%d] pvent=%p attrs=%p start=%ld end=%ld\n",
		    lcv,
		    vm_physmem[lcv].pmseg.pvent, vm_physmem[lcv].pmseg.attrs,
		    vm_physmem[lcv].start, vm_physmem[lcv].end);
	}
#endif
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
pmap_create()
{
	struct pmap *pmap;

	/*
	 * Fetch pmap entry from the pool
	 */
	
	pmap = pool_get(&pmap_pmap_pool, PR_WAITOK);
	/* XXX is this really needed! */
	memset(pmap, 0, sizeof(*pmap));

	simple_lock_init(&pmap->pm_obj.vmobjlock);
	pmap->pm_obj.pgops = NULL;	/* currently not a mappable object */
	TAILQ_INIT(&pmap->pm_obj.memq);
	pmap->pm_obj.uo_npages = 0;
	pmap->pm_obj.uo_refs = 1;
	pmap->pm_stats.wired_count = 0;
	pmap->pm_stats.resident_count = 1;

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
	pmap_remove(pmap_kernel(), pt->pt_va, pt->pt_va + PD_SIZE);
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

		bcopy((char *)pmap_kernel()->pm_pdir + (PD_SIZE - KERNEL_PD_SIZE),
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

	(void) pmap_extract(pmap_kernel(), pmap->pm_vptpt, &pmap->pm_pptpt);
	pmap->pm_pptpt &= PG_FRAME;
	/* Revoke cacheability and bufferability */
	/* XXX should be done better than this */
	pte = pmap_pte(pmap_kernel(), pmap->pm_vptpt);
	*pte = *pte & ~(PT_C | PT_B);

	/* Wire in this page table */
	pmap_map_in_l1(pmap, PROCESS_PAGE_TBLS_BASE, pmap->pm_pptpt, TRUE);

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

	return(0);
}


/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */

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
		 * again.
		 */
		(void) ltsleep(&lbolt, PVM, "l1ptwait", hz >> 3, NULL);
	}

	/* Map zero page for the pmap. This will also map the L2 for it */
	pmap_enter(pmap, 0x00000000, systempage.pv_pa,
	    VM_PROT_READ, VM_PROT_READ | PMAP_WIRED);
	pmap_update();
}


void
pmap_freepagedir(pmap)
	struct pmap *pmap;
{
	/* Free the memory used for the page table mapping */
	if (pmap->pm_vptpt != 0)
		uvm_km_free(kernel_map, (vaddr_t)pmap->pm_vptpt, NBPG);

	/* junk the L1 page table */
	if (pmap->pm_l1pt->pt_flags & PTFLAG_STATIC) {
		/* Add the page table to the queue */
		SIMPLEQ_INSERT_TAIL(&l1pt_static_queue, pmap->pm_l1pt, pt_queue);
		++l1pt_static_queue_count;
	} else if (l1pt_queue_count < 8) {
		/* Add the page table to the queue */
		SIMPLEQ_INSERT_TAIL(&l1pt_queue, pmap->pm_l1pt, pt_queue);
		++l1pt_queue_count;
	} else
		pmap_free_l1pt(pmap->pm_l1pt);
}


/*
 * Retire the given physical map from service.
 * Should only be called if the map contains no valid mappings.
 */

void
pmap_destroy(pmap)
	struct pmap *pmap;
{
	struct vm_page *page;
	int count;

	if (pmap == NULL)
		return;

	PDEBUG(0, printf("pmap_destroy(%p)\n", pmap));

	/*
	 * Drop reference count
	 */
	simple_lock(&pmap->pm_obj.vmobjlock);
	count = --pmap->pm_obj.uo_refs;
	simple_unlock(&pmap->pm_obj.vmobjlock);
	if (count > 0) {
		return;
	}

	/*
	 * reference count is zero, free pmap resources and then free pmap.
	 */
	
	/* Remove the zero page mapping */
	pmap_remove(pmap, 0x00000000, 0x00000000 + NBPG);
	pmap_update();

	/*
	 * Free any page tables still mapped
	 * This is only temporay until pmap_enter can count the number
	 * of mappings made in a page table. Then pmap_remove() can
	 * reduce the count and free the pagetable when the count
	 * reaches zero.  Note that entries in this list should match the
	 * contents of the ptpt, however this is faster than walking a 1024
	 * entries looking for pt's
	 * taken from i386 pmap.c
	 */
	while (pmap->pm_obj.memq.tqh_first != NULL) {
		page = pmap->pm_obj.memq.tqh_first;
#ifdef DIAGNOSTIC
		if (page->flags & PG_BUSY)
			panic("pmap_release: busy page table page");
#endif
		/* pmap_page_protect?  currently no need for it. */

		page->wire_count = 0;
		uvm_pagefree(page);
	}
	
	/* Free the page dir */
	pmap_freepagedir(pmap);
	
	/* return the pmap to the pool */
	pool_put(&pmap_pmap_pool, pmap);
}


/*
 * void pmap_reference(struct pmap *pmap)
 *
 * Add a reference to the specified pmap.
 */

void
pmap_reference(pmap)
	struct pmap *pmap;
{
	if (pmap == NULL)
		return;

	simple_lock(&pmap->pm_lock);
	pmap->pm_obj.uo_refs++;
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
	struct pmap *pmap = p->p_vmspace->vm_map.pmap;
	struct pcb *pcb = &p->p_addr->u_pcb;

	(void) pmap_extract(pmap_kernel(), (vaddr_t)pmap->pm_pdir,
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
pmap_clean_page(pv, is_src)
	struct pv_entry *pv;
	boolean_t is_src;
{
	struct pmap *pmap;
	struct pv_entry *npv;
	int cache_needs_cleaning = 0;
	vaddr_t page_to_clean = 0;

	if (pv == NULL)
		/* nothing mapped in so nothing to flush */
		return (0);

	/* Since we flush the cache each time we change curproc, we
	 * only need to flush the page if it is in the current pmap.
	 */
	if (curproc)
		pmap = curproc->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	for (npv = pv; npv; npv = npv->pv_next) {
		if (npv->pv_pmap == pmap) {
			/* The page is mapped non-cacheable in 
			 * this map.  No need to flush the cache.
			 */
			if (npv->pv_flags & PT_NC) {
#ifdef DIAGNOSTIC
				if (cache_needs_cleaning)
					panic("pmap_clean_page: "
							"cache inconsistency");
#endif
				break;
			}
#if 0
			/* This doesn't work, because pmap_protect
			   doesn't flush changes on pages that it
			   has write-protected.  */
			/* If the page is not writeable and this
			   is the source, then there is no need
			   to flush it from the cache.  */
			else if (is_src && ! (npv->pv_flags & PT_Wr))
				continue;
#endif
			if (cache_needs_cleaning){
				page_to_clean = 0;
				break;
			}
			else
				page_to_clean = npv->pv_va;
			cache_needs_cleaning = 1;
		}
	}

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
 * This is a local function that finds a PV head for a given physical page.
 * This is a common op, and this function removes loads of ifdefs in the code.
 */
static __inline struct pv_head *
pmap_find_pvh(phys)
	paddr_t phys;
{
	int bank, off;
	struct pv_head *pvh;

#ifdef DIAGNOSTIC
	if (!pmap_initialized)
		panic("pmap_find_pv: !pmap_initialized");
#endif

	if ((bank = vm_physseg_find(atop(phys), &off)) == -1)
		panic("pmap_find_pv: not a real page, phys=%lx\n", phys);
	pvh = &vm_physmem[bank].pmseg.pvhead[off];
	return (pvh);
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
	struct pv_head *pvh;

	/* Get an entry for this page, and clean it it. */
	PMAP_HEAD_TO_MAP_LOCK();
	pvh = pmap_find_pvh(phys);
	simple_lock(&pvh->pvh_lock);
	pmap_clean_page(pvh->pvh_list, FALSE);
	simple_unlock(&pvh->pvh_lock);
	PMAP_HEAD_TO_MAP_UNLOCK();
	
	/*
	 * Hook in the page, zero it, and purge the cache for that
	 * zeroed page. Invalidate the TLB as needed.
	 */
	*page_hook0.pte = L2_PTE(phys & PG_FRAME, AP_KRW);
	cpu_tlb_flushD_SE(page_hook0.va);
	bzero_page(page_hook0.va);
	cpu_cache_purgeD_rng(page_hook0.va, NBPG);
}

/* pmap_pageidlezero()
 *
 * The same as above, except that we assume that the page is not
 * mapped.  This means we never have to flush the cache first.  Called
 * from the idle loop.
 */
boolean_t
pmap_pageidlezero(phys)
    paddr_t phys;
{
	int i, *ptr;
	boolean_t rv = TRUE;
	
#ifdef DIAGNOSTIC
	struct pv_head *pvh;
	
	pvh = pmap_find_pvh(phys);
	if (pvh->pvh_list != NULL)
		panic("pmap_pageidlezero: zeroing mapped page\n");
#endif
	
	/*
	 * Hook in the page, zero it, and purge the cache for that
	 * zeroed page. Invalidate the TLB as needed.
	 */
	*page_hook0.pte = L2_PTE(phys & PG_FRAME, AP_KRW);
	cpu_tlb_flushD_SE(page_hook0.va);
	
	for (i = 0, ptr = (int *)page_hook0.va;
			i < (NBPG / sizeof(int)); i++) {
		if (sched_whichqs != 0) {
			/*
			 * A process has become ready.  Abort now,
			 * so we don't keep it waiting while we
			 * do slow memory access to finish this
			 * page.
			 */
			rv = FALSE;
			break;
		}
		*ptr++ = 0;
	}

	if (rv)
		/* 
		 * if we aborted we'll rezero this page again later so don't
		 * purge it unless we finished it
		 */
		cpu_cache_purgeD_rng(page_hook0.va, NBPG);
	return (rv);
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
	struct pv_head *src_pvh, *dest_pvh;
	
	PMAP_HEAD_TO_MAP_LOCK();
	/* Get PV entries for the pages, and clean them if needed. */
	src_pvh = pmap_find_pvh(src);
	simple_lock(&src_pvh->pvh_lock);
	dest_pvh = pmap_find_pvh(dest);
	simple_lock(&dest_pvh->pvh_lock);
	if (!pmap_clean_page(src_pvh->pvh_list, TRUE))
		pmap_clean_page(dest_pvh->pvh_list, FALSE);
	
	simple_unlock(&dest_pvh->pvh_lock);
	simple_unlock(&src_pvh->pvh_lock);
	PMAP_HEAD_TO_MAP_UNLOCK();

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
	struct pmap *pmap;
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
	struct pmap *pmap;
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
pmap_vac_me_harder(struct pmap *pmap, struct pv_head *pvh, pt_entry_t *ptes,
	boolean_t clear_cache)
{
	struct pv_entry *pv, *npv;
	pt_entry_t *pte;
	int entries = 0;
	int writeable = 0;
	int cacheable_entries = 0;

	pv = pvh->pvh_list;
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
	struct pmap *pmap;
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
	struct pv_head *pvh;

	/* Exit quick if there is no pmap */
	if (!pmap)
		return;

	PDEBUG(0, printf("pmap_remove: pmap=%p sva=%08lx eva=%08lx\n", pmap, sva, eva));

	sva &= PG_FRAME;
	eva &= PG_FRAME;

	/*
	 * we lock in the pmap => pv_head direction
	 */
	PMAP_MAP_TO_HEAD_LOCK();
	
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
	    || (pmap == pmap_kernel()))
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
				struct pv_entry *pve;
				pvh = &vm_physmem[bank].pmseg.pvhead[off];
				simple_lock(&pvh->pvh_lock);
				pve = pmap_remove_pv(pvh, pmap, sva);
				pmap_free_pv(pmap, pve);
				pmap_vac_me_harder(pmap, pvh, ptes, FALSE);
				simple_unlock(&pvh->pvh_lock);
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
	PMAP_MAP_TO_HEAD_UNLOCK();
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
	struct pv_entry *pv, *npv;
	struct pv_head *pvh;
	struct pmap *pmap;
	pt_entry_t *pte, *ptes;

	PDEBUG(0, printf("pmap_remove_all: pa=%lx ", pa));

	/* set pv_head => pmap locking */
	PMAP_HEAD_TO_MAP_LOCK();

	pvh = pmap_find_pvh(pa);
	simple_lock(&pvh->pvh_lock);
	
	pv = pvh->pvh_list;
	if (pv == NULL)
	{
	    PDEBUG(0, printf("free page\n"));
	    simple_unlock(&pvh->pvh_lock);
	    PMAP_HEAD_TO_MAP_UNLOCK();
	    return;
	}
	pmap_clean_page(pv, FALSE);

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
		pmap_free_pv(pmap, pv);
		pv = npv;
		pmap_unmap_ptes(pmap);
	}
	pvh->pvh_list = NULL;
	simple_unlock(&pvh->pvh_lock);
	PMAP_HEAD_TO_MAP_UNLOCK();	

	PDEBUG(0, printf("done\n"));
	cpu_tlb_flushID();
}


/*
 * Set the physical protection on the specified range of this map as requested.
 */

void
pmap_protect(pmap, sva, eva, prot)
	struct pmap *pmap;
	vaddr_t sva;
	vaddr_t eva;
	vm_prot_t prot;
{
	pt_entry_t *pte = NULL, *ptes;
	int armprot;
	int flush = 0;
	paddr_t pa;
	int bank, off;
	struct pv_head *pvh;

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

	/* Need to lock map->head */
	PMAP_MAP_TO_HEAD_LOCK();
	
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
			pvh = &vm_physmem[bank].pmseg.pvhead[off];
			simple_lock(&pvh->pvh_lock);
			(void) pmap_modify_pv(pmap, sva, pvh, PT_Wr, 0);
			pmap_vac_me_harder(pmap, pvh, ptes, FALSE);
			simple_unlock(&pvh->pvh_lock);
		}

next:
		sva += NBPG;
		pte++;
	}
	pmap_unmap_ptes(pmap);
	PMAP_MAP_TO_HEAD_UNLOCK();
	if (flush)
		cpu_tlb_flushID();
}

/*
 * void pmap_enter(struct pmap *pmap, vaddr_t va, paddr_t pa, vm_prot_t prot,
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
	struct pmap *pmap;
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
	int flags;
{
	pt_entry_t *pte, *ptes;
	u_int npte;
	int bank, off;
	paddr_t opa;
	int nflags;
	boolean_t wired = (flags & PMAP_WIRED) != 0;
	struct pv_entry *pve;
	struct pv_head	*pvh;
	int error;

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
	/* get lock */
	PMAP_MAP_TO_HEAD_LOCK();
	/*
	 * Get a pointer to the pte for this virtual address. If the
	 * pte pointer is NULL then we are missing the L2 page table
	 * so we need to create one.
	 */
	pte = pmap_pte(pmap, va);
	if (!pte) {
		struct vm_page *ptp;

		/* if failure is allowed then don't try too hard */
		ptp = pmap_get_ptp(pmap, va, flags & PMAP_CANFAIL);
		if (ptp == NULL) {
			if (flags & PMAP_CANFAIL) {
				error = ENOMEM;
				goto out;
			}
			panic("pmap_enter: get ptp failed");
		}
		
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
				pvh = &vm_physmem[bank].pmseg.pvhead[off];
				simple_lock(&pvh->pvh_lock);
				(void) pmap_modify_pv(pmap, va, pvh,
				    PT_Wr | PT_W, nflags);
				simple_unlock(&pvh->pvh_lock);
 			} else {
				pvh = NULL;
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
				pvh = &vm_physmem[bank].pmseg.pvhead[off];
				simple_lock(&pvh->pvh_lock);
				pve = pmap_remove_pv(pvh, pmap, va);
				simple_unlock(&pvh->pvh_lock);
			} else {
				pve = NULL;
			}

			goto enter;
		}
	} else {
		opa = 0;
		pve = NULL;
		pmap_pte_addref(pmap, va);

		/* pte is not valid so we must be hooking in a new page */
		++pmap->pm_stats.resident_count;

	enter:
		/*
		 * Enter on the PV list if part of our managed memory
		 */
		bank = vm_physseg_find(atop(pa), &off);
		
		if (pmap_initialized && (bank != -1)) {
			pvh = &vm_physmem[bank].pmseg.pvhead[off];
			if (pve == NULL) {
				pve = pmap_alloc_pv(pmap, ALLOCPV_NEED);
				if (pve == NULL) {
					if (flags & PMAP_CANFAIL) {
						error = ENOMEM;
						goto out;
					}
					panic("pmap_enter: no pv entries available");
				}
			}
			/* enter_pv locks pvh when adding */
			pmap_enter_pv(pvh, pve, pmap, va, NULL, nflags);
		} else {
			pvh = NULL;
			if (pve != NULL)
				pmap_free_pv(pmap, pve);
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

	if (pmap_initialized && bank != -1) {
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

	if (pmap_initialized && bank != -1)
	{
		boolean_t pmap_active = FALSE;
		/* XXX this will change once the whole of pmap_enter uses
		 * map_ptes
		 */
		ptes = pmap_map_ptes(pmap);
		if ((curproc && curproc->p_vmspace->vm_map.pmap == pmap)
		    || (pmap == pmap_kernel()))
			pmap_active = TRUE;
		simple_lock(&pvh->pvh_lock);
 		pmap_vac_me_harder(pmap, pvh, ptes, pmap_active);
		simple_unlock(&pvh->pvh_lock);
		pmap_unmap_ptes(pmap);
	}

	/* Better flush the TLB ... */
	cpu_tlb_flushID_SE(va);
	error = 0;
out:
	PMAP_MAP_TO_HEAD_UNLOCK();
	PDEBUG(5, printf("pmap_enter: pte = V%p %08x\n", pte, *pte));

	return error;
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
		pg = uvm_pagealloc(&(pmap_kernel()->pm_obj), 0, NULL,
		    UVM_PGA_USERESERVE | UVM_PGA_ZERO);
		if (pg == NULL) {
			panic("pmap_kenter_pa: no free pages");
		}
		pg->flags &= ~PG_BUSY;	/* never busy */

		/* Wire this page table into the L1. */
		pmap_map_in_l1(pmap, va, VM_PAGE_TO_PHYS(pg), TRUE);
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
	case VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE:
	case VM_PROT_READ|VM_PROT_WRITE:
		return;

	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		pmap_copy_on_write(pa);
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
	struct pmap *pmap;
	vaddr_t va;
{
	pt_entry_t *pte;
	paddr_t pa;
	int bank, off;
	struct pv_head *pvh;

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
	pvh = &vm_physmem[bank].pmseg.pvhead[off];
	simple_lock(&pvh->pvh_lock);
	/* Update the wired bit in the pv entry for this page. */
	(void) pmap_modify_pv(pmap, va, pvh, PT_W, 0);
	simple_unlock(&pvh->pvh_lock);
}

/*
 * pt_entry_t *pmap_pte(struct pmap *pmap, vaddr_t va)
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
	struct pmap *pmap;
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
	if (pmap == pmap_kernel() || pmap->pm_pptpt
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
		    pmap->pm_pptpt, FALSE);
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
	struct pmap *pmap;
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
	struct pmap *dst_pmap;
	struct pmap *src_pmap;
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
	struct pv_head *pvh;
	struct pv_entry *pv;
	int bank, off;

	if ((bank = vm_physseg_find(atop(phys), &off)) == -1) {
		printf("INVALID PA\n");
		return;
	}
	pvh = &vm_physmem[bank].pmseg.pvhead[off];
	simple_lock(&pvh->pvh_lock);
	printf("%s %08lx:", m, phys);
	if (pvh->pvh_list == NULL) {
		printf(" no mappings\n");
		return;
	}

	for (pv = pvh->pvh_list; pv; pv = pv->pv_next)
		printf(" pmap %p va %08lx flags %08x", pv->pv_pmap,
		    pv->pv_va, pv->pv_flags);

	printf("\n");
	simple_unlock(&pvh->pvh_lock);
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
	
	if (pmap_is_curpmap(pmap)) {
		simple_lock(&pmap->pm_obj.vmobjlock);
		return (pt_entry_t *)PROCESS_PAGE_TBLS_BASE;
	}
	
	p = curproc;
	
	if (p == NULL)
		p = &proc0;

	/* need to lock both curpmap and pmap: use ordered locking */
	if ((unsigned) pmap < (unsigned) curproc->p_vmspace->vm_map.pmap) {
		simple_lock(&pmap->pm_obj.vmobjlock);
		simple_lock(&curproc->p_vmspace->vm_map.pmap->pm_obj.vmobjlock);
	} else {
		simple_lock(&curproc->p_vmspace->vm_map.pmap->pm_obj.vmobjlock);
		simple_lock(&pmap->pm_obj.vmobjlock);
	}
    
	pmap_map_in_l1(p->p_vmspace->vm_map.pmap, ALT_PAGE_TBLS_BASE,
			pmap->pm_pptpt, FALSE);
	cpu_tlb_flushD();
	return (pt_entry_t *)ALT_PAGE_TBLS_BASE;
}

/*
 * pmap_unmap_ptes: unlock the PTE mapping of "pmap"
 */

static void
pmap_unmap_ptes(pmap)
	struct pmap *pmap;
{
	if (pmap == pmap_kernel()) {
		return;
	}
	if (pmap_is_curpmap(pmap)) {
		simple_unlock(&pmap->pm_obj.vmobjlock);
	} else {
		simple_unlock(&pmap->pm_obj.vmobjlock);
		simple_unlock(&curproc->p_vmspace->vm_map.pmap->pm_obj.vmobjlock);
	}
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
	struct pv_head *pvh;
	pt_entry_t *pte;
	vaddr_t va;
	int bank, off;

	PDEBUG(1, printf("pmap_clearbit: pa=%08lx mask=%08x\n",
	    pa, maskbits));
	if ((bank = vm_physseg_find(atop(pa), &off)) == -1)
		return;
	PMAP_HEAD_TO_MAP_LOCK();
	pvh = &vm_physmem[bank].pmseg.pvhead[off];
	simple_lock(&pvh->pvh_lock);
	
	/*
	 * Clear saved attributes (modify, reference)
	 */
	vm_physmem[bank].pmseg.attrs[off] &= ~maskbits;

	if (pvh->pvh_list == NULL) {
		simple_unlock(&pvh->pvh_lock);
		PMAP_HEAD_TO_MAP_UNLOCK();
		return;
	}

	/*
	 * Loop over all current mappings setting/clearing as appropos
	 */
	for (pv = pvh->pvh_list; pv; pv = pv->pv_next) {
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
		KASSERT(pte != NULL);
		if (maskbits & (PT_Wr|PT_M))
			*pte &= ~PT_AP(AP_W);
		if (maskbits & PT_H)
			*pte = (*pte & ~L2_MASK) | L2_INVAL;
	}
	simple_unlock(&pvh->pvh_lock);
	PMAP_HEAD_TO_MAP_UNLOCK();
	cpu_tlb_flushID();

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
	PDEBUG(1, printf("pmap_is_modified pa=%08lx %x\n", pa, result));
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
	struct pmap *pmap;
	vaddr_t va;
{
	pt_entry_t *pte;
	paddr_t pa;
	int bank, off;
	struct pv_head *pvh;
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

	PMAP_HEAD_TO_MAP_LOCK();
	/* Get the current flags for this page. */
	pvh = &vm_physmem[bank].pmseg.pvhead[off];
	/* XXX: needed if we hold head->map lock? */
	simple_lock(&pvh->pvh_lock);
	
	flags = pmap_modify_pv(pmap, va, pvh, 0, 0);
	PDEBUG(2, printf("pmap_modified_emulation: flags = %08x\n", flags));

	/*
	 * Do the flags say this page is writable ? If not then it is a
	 * genuine write fault. If yes then the write fault is our fault
	 * as we did not reflect the write access in the PTE. Now we know
	 * a write has occurred we can correct this and also set the
	 * modified bit
	 */
	if (~flags & PT_Wr) {
	    	simple_unlock(&pvh->pvh_lock);
		PMAP_HEAD_TO_MAP_UNLOCK();
		return(0);
	}

	PDEBUG(0, printf("pmap_modified_emulation: Got a hit va=%08lx, pte = %p (%08x)\n",
	    va, pte, *pte));
	vm_physmem[bank].pmseg.attrs[off] |= PT_H | PT_M;
	*pte = (*pte & ~L2_MASK) | L2_SPAGE | PT_AP(AP_W);
	PDEBUG(0, printf("->(%08x)\n", *pte));

	simple_unlock(&pvh->pvh_lock);
	PMAP_HEAD_TO_MAP_UNLOCK();
	/* Return, indicating the problem has been dealt with */
	cpu_tlb_flushID_SE(va);
	return(1);
}


int
pmap_handled_emulation(pmap, va)
	struct pmap *pmap;
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
	struct pmap *pmap;
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
/*
 * PTP functions
 */
  
/*
 * pmap_steal_ptp: Steal a PTP from somewhere else.
 *
 * This is just a placeholder, for now we never steal.
 */
 
static struct vm_page *
pmap_steal_ptp(struct pmap *pmap, vaddr_t va)
{
    return (NULL);
}

/*
 * pmap_get_ptp: get a PTP (if there isn't one, allocate a new one)
 *
 * => pmap should NOT be pmap_kernel()
 * => pmap should be locked
 */

static struct vm_page *
pmap_get_ptp(struct pmap *pmap, vaddr_t va, boolean_t just_try)
{
    struct vm_page *ptp;

    if (pmap_pde_v(pmap_pde(pmap, va))) {

	/* valid... check hint (saves us a PA->PG lookup) */
#if 0
	if (pmap->pm_ptphint &&
    		((unsigned)pmap_pde(pmap, va) & PG_FRAME) ==
		VM_PAGE_TO_PHYS(pmap->pm_ptphint))
	    return (pmap->pm_ptphint);
#endif
	ptp = uvm_pagelookup(&pmap->pm_obj, va);
#ifdef DIAGNOSTIC
	if (ptp == NULL)
    	    panic("pmap_get_ptp: unmanaged user PTP");
#endif
//	pmap->pm_ptphint = ptp;
	return(ptp);
    }

    /* allocate a new PTP (updates ptphint) */
    return(pmap_alloc_ptp(pmap, va, just_try));
}

/*
 * pmap_alloc_ptp: allocate a PTP for a PMAP
 *
 * => pmap should already be locked by caller
 * => we use the ptp's wire_count to count the number of active mappings
 *	in the PTP (we start it at one to prevent any chance this PTP
 *	will ever leak onto the active/inactive queues)
 */

/*__inline */ static struct vm_page *
pmap_alloc_ptp(struct pmap *pmap, vaddr_t va, boolean_t just_try)
{
	struct vm_page *ptp;

	ptp = uvm_pagealloc(&pmap->pm_obj, va, NULL,
		UVM_PGA_USERESERVE|UVM_PGA_ZERO);
	if (ptp == NULL) {
	    if (just_try)
		return (NULL);

	    ptp = pmap_steal_ptp(pmap, va);

	    if (ptp == NULL)
		return (NULL);
	    /* Stole a page, zero it.  */
	    pmap_zero_page(VM_PAGE_TO_PHYS(ptp));
	}
	    
	/* got one! */
	ptp->flags &= ~PG_BUSY;	/* never busy */
	ptp->wire_count = 1;	/* no mappings yet */
	pmap_map_in_l1(pmap, va, VM_PAGE_TO_PHYS(ptp), TRUE);
	pmap->pm_stats.resident_count++;	/* count PTP as resident */
//	pmap->pm_ptphint = ptp;
	return (ptp);
}

/* End of pmap.c */
