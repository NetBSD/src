/*	$NetBSD: pmap.c,v 1.130 2003/04/01 23:19:09 thorpej Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * Copyright (c) 2001 Richard Earnshaw
 * Copyright (c) 2001-2002 Christopher Gilbert
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
#include "opt_cpuoptions.h"

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
#include <arm/arm32/katelib.h>

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.130 2003/04/01 23:19:09 thorpej Exp $");

#ifdef PMAP_DEBUG
#define	PDEBUG(_lev_,_stat_) \
	if (pmap_debug_level >= (_lev_)) \
        	((_stat_))
int pmap_debug_level = -2;
void pmap_dump_pvlist(vaddr_t phys, char *m);

/*
 * for switching to potentially finer grained debugging
 */
#define	PDB_FOLLOW	0x0001
#define	PDB_INIT	0x0002
#define	PDB_ENTER	0x0004
#define	PDB_REMOVE	0x0008
#define	PDB_CREATE	0x0010
#define	PDB_PTPAGE	0x0020
#define	PDB_GROWKERN	0x0040
#define	PDB_BITS	0x0080
#define	PDB_COLLECT	0x0100
#define	PDB_PROTECT	0x0200
#define	PDB_MAP_L1	0x0400
#define	PDB_BOOTSTRAP	0x1000
#define	PDB_PARANOIA	0x2000
#define	PDB_WIRING	0x4000
#define	PDB_PVDUMP	0x8000

int debugmap = 0;
int pmapdebug = PDB_PARANOIA | PDB_FOLLOW | PDB_GROWKERN | PDB_ENTER | PDB_REMOVE;
#define	NPDEBUG(_lev_,_stat_) \
	if (pmapdebug & (_lev_)) \
        	((_stat_))
    
#else	/* PMAP_DEBUG */
#define	PDEBUG(_lev_,_stat_) /* Nothing */
#define NPDEBUG(_lev_,_stat_) /* Nothing */
#endif	/* PMAP_DEBUG */

struct pmap     kernel_pmap_store;

/*
 * linked list of all non-kernel pmaps
 */

static LIST_HEAD(, pmap) pmaps;

/*
 * pool that pmap structures are allocated from
 */

struct pool pmap_pmap_pool;

/*
 * pool/cache that PT-PT's are allocated from
 */

struct pool pmap_ptpt_pool;
struct pool_cache pmap_ptpt_cache;
u_int pmap_ptpt_cache_generation;

static void *pmap_ptpt_page_alloc(struct pool *, int);
static void pmap_ptpt_page_free(struct pool *, void *);

struct pool_allocator pmap_ptpt_allocator = {
	pmap_ptpt_page_alloc, pmap_ptpt_page_free,
};

static int pmap_ptpt_ctor(void *, void *, int);

static pt_entry_t *csrc_pte, *cdst_pte;
static vaddr_t csrcp, cdstp;

char *memhook;
extern caddr_t msgbufaddr;

boolean_t pmap_initialized = FALSE;	/* Has pmap_init completed? */
/*
 * locking data structures
 */

static struct lock pmap_main_lock;
static struct simplelock pvalloc_lock;
static struct simplelock pmaps_lock;
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
static unsigned int pv_nfpvents;	/* # of free pv entries */
static struct pv_page *pv_initpage;	/* bootstrap page from kernel_map */
static vaddr_t pv_cachedva;		/* cached VA for later use */

#define PVE_LOWAT (PVE_PER_PVPAGE / 2)	/* free pv_entry low water mark */
#define PVE_HIWAT (PVE_LOWAT + (PVE_PER_PVPAGE * 2))
					/* high water mark */

/*
 * local prototypes
 */

static struct pv_entry	*pmap_add_pvpage(struct pv_page *, boolean_t);
static struct pv_entry	*pmap_alloc_pv(struct pmap *, unsigned int);
#define ALLOCPV_NEED	0	/* need PV now */
#define ALLOCPV_TRY	1	/* just try to allocate, don't steal */
#define ALLOCPV_NONEED	2	/* don't need PV, just growing cache */
static struct pv_entry	*pmap_alloc_pvpage(struct pmap *, unsigned int);
static void		 pmap_enter_pv(struct vm_page *,
				       struct pv_entry *, struct pmap *,
				       vaddr_t, struct vm_page *, unsigned int);
static void		 pmap_free_pv(struct pmap *, struct pv_entry *);
static void		 pmap_free_pvs(struct pmap *, struct pv_entry *);
static void		 pmap_free_pv_doit(struct pv_entry *);
static void		 pmap_free_pvpage(void);
static boolean_t	 pmap_is_curpmap(struct pmap *);
static struct pv_entry	*pmap_remove_pv(struct vm_page *, struct pmap *, 
					vaddr_t);
#define PMAP_REMOVE_ALL		0	/* remove all mappings */
#define PMAP_REMOVE_SKIPWIRED	1	/* skip wired mappings */

static u_int pmap_modify_pv(struct pmap *, vaddr_t, struct vm_page *,
			    u_int, u_int);

/*
 * Structure that describes and L1 table.
 */
struct l1pt {
	SIMPLEQ_ENTRY(l1pt)	pt_queue;	/* Queue pointers */
	struct pglist		pt_plist;	/* Allocated page list */
	vaddr_t			pt_va;		/* Allocated virtual address */
	unsigned int		pt_flags;	/* Flags */ 
};
#define	PTFLAG_STATIC		0x01		/* Statically allocated */
#define	PTFLAG_KPT		0x02		/* Kernel pt's are mapped */
#define	PTFLAG_CLEAN		0x04		/* L1 is clean */

static void	pmap_free_l1pt(struct l1pt *);
static int	pmap_allocpagedir(struct pmap *);
static int	pmap_clean_page(struct pv_entry *, boolean_t);
static void	pmap_page_remove(struct vm_page *);

static struct vm_page	*pmap_alloc_ptp(struct pmap *, vaddr_t);
static struct vm_page	*pmap_get_ptp(struct pmap *, vaddr_t);
__inline static void 	 pmap_clearbit(struct vm_page *, unsigned int);

extern paddr_t physical_start;
extern paddr_t physical_end;
extern int max_processes;

vaddr_t virtual_avail;
vaddr_t virtual_end;
vaddr_t pmap_curmaxkvaddr;

vaddr_t avail_start;
vaddr_t avail_end;

extern pv_addr_t systempage;

/* Variables used by the L1 page table queue code */
SIMPLEQ_HEAD(l1pt_queue, l1pt);
static struct l1pt_queue l1pt_static_queue; /* head of our static l1 queue */
static u_int l1pt_static_queue_count;	    /* items in the static l1 queue */
static u_int l1pt_static_create_count;	    /* static l1 items created */
static struct l1pt_queue l1pt_queue;	    /* head of our l1 queue */
static u_int l1pt_queue_count;		    /* items in the l1 queue */
static u_int l1pt_create_count;		    /* stat - L1's create count */
static u_int l1pt_reuse_count;		    /* stat - L1's reused count */

/* Local function prototypes (not used outside this file) */
void pmap_pinit(struct pmap *);
void pmap_freepagedir(struct pmap *);

/* Other function prototypes */
extern void bzero_page(vaddr_t);
extern void bcopy_page(vaddr_t, vaddr_t);

struct l1pt *pmap_alloc_l1pt(void);
static __inline void pmap_map_in_l1(struct pmap *pmap, vaddr_t va,
     				    vaddr_t l2pa, unsigned int);

static pt_entry_t *pmap_map_ptes(struct pmap *);
static void 	   pmap_unmap_ptes(struct pmap *);

__inline static void pmap_vac_me_harder(struct pmap *, struct vm_page *,
					pt_entry_t *, boolean_t);
static void pmap_vac_me_kpmap(struct pmap *, struct vm_page *,
			      pt_entry_t *, boolean_t);
static void pmap_vac_me_user(struct pmap *, struct vm_page *,
			     pt_entry_t *, boolean_t);

/*
 * real definition of pv_entry.
 */

struct pv_entry {
	struct pv_entry *pv_next;	/* next pv_entry */
	struct pmap     *pv_pmap;	/* pmap where mapping lies */
	vaddr_t         pv_va;		/* virtual address for mapping */
	unsigned int    pv_flags;	/* flags */
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
	unsigned int pvpi_nfree;
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

#ifdef MYCROFT_HACK
int mycroft_hack = 0;
#endif

/* Function to set the debug level of the pmap code */

#ifdef PMAP_DEBUG
void
pmap_debug(int level)
{
	pmap_debug_level = level;
	printf("pmap_debug: level=%d\n", pmap_debug_level);
}
#endif	/* PMAP_DEBUG */

__inline static boolean_t
pmap_is_curpmap(struct pmap *pmap)
{

	if ((curproc && curproc->p_vmspace->vm_map.pmap == pmap) ||
	    pmap == pmap_kernel())
		return (TRUE);

	return (FALSE);
}

/*
 * PTE_SYNC_CURRENT:
 *
 *	Make sure the pte is flushed to RAM.  If the pmap is
 *	not the current pmap, then also evict the pte from
 *	any cache lines.
 */
#define	PTE_SYNC_CURRENT(pmap, pte)					\
do {									\
	if (pmap_is_curpmap(pmap))					\
		PTE_SYNC(pte);						\
	else								\
		PTE_FLUSH(pte);						\
} while (/*CONSTCOND*/0)

/*
 * PTE_FLUSH_ALT:
 *
 *	Make sure the pte is not in any cache lines.  We expect
 *	this to be used only when a pte has not been modified.
 */
#define	PTE_FLUSH_ALT(pmap, pte)					\
do {									\
	if (pmap_is_curpmap(pmap) == 0)					\
		PTE_FLUSH(pte);						\
} while (/*CONSTCOND*/0)

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
pmap_alloc_pv(struct pmap *pmap, unsigned int mode)
{
	struct pv_page *pvpage;
	struct pv_entry *pv;

	simple_lock(&pvalloc_lock);

	pvpage = TAILQ_FIRST(&pv_freepages);

	if (pvpage != NULL) {
		pvpage->pvinfo.pvpi_nfree--;
		if (pvpage->pvinfo.pvpi_nfree == 0) {
			/* nothing left in this one? */
			TAILQ_REMOVE(&pv_freepages, pvpage, pvinfo.pvpi_list);
		}
		pv = pvpage->pvinfo.pvpi_pvfree;
		KASSERT(pv);
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
pmap_alloc_pvpage(struct pmap *pmap, unsigned int mode)
{
	struct vm_page *pg;
	struct pv_page *pvpage;
	struct pv_entry *pv;

	/*
	 * if we need_entry and we've got unused pv_pages, allocate from there
	 */

	pvpage = TAILQ_FIRST(&pv_unusedpgs);
	if (mode != ALLOCPV_NONEED && pvpage != NULL) {

		/* move it to pv_freepages list */
		TAILQ_REMOVE(&pv_unusedpgs, pvpage, pvinfo.pvpi_list);
		TAILQ_INSERT_HEAD(&pv_freepages, pvpage, pvinfo.pvpi_list);

		/* allocate a pv_entry */
		pvpage->pvinfo.pvpi_nfree--;	/* can't go to zero */
		pv = pvpage->pvinfo.pvpi_pvfree;
		KASSERT(pv);
		pvpage->pvinfo.pvpi_pvfree = pv->pv_next;

		pv_nfpvents--;  /* took one from pool */
		return(pv);
	}

	/*
	 *  see if we've got a cached unmapped VA that we can map a page in.
	 * if not, try to allocate one.
	 */


	if (pv_cachedva == 0) {
		int s;
		s = splvm();
		pv_cachedva = uvm_km_kmemalloc(kmem_map, NULL,
		    PAGE_SIZE, UVM_KMF_TRYLOCK|UVM_KMF_VALLOC);
		splx(s);
		if (pv_cachedva == 0) {
			return (NULL);
		}
	}

	pg = uvm_pagealloc(NULL, pv_cachedva - vm_map_min(kernel_map), NULL,
	    UVM_PGA_USERESERVE);

	if (pg == NULL)
		return (NULL);
	pg->flags &= ~PG_BUSY;	/* never busy */

	/*
	 * add a mapping for our new pv_page and free its entrys (save one!)
	 *
	 * NOTE: If we are allocating a PV page for the kernel pmap, the
	 * pmap is already locked!  (...but entering the mapping is safe...)
	 */

	pmap_kenter_pa(pv_cachedva, VM_PAGE_TO_PHYS(pg),
		VM_PROT_READ|VM_PROT_WRITE);
	pmap_update(pmap_kernel());
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
pmap_add_pvpage(struct pv_page *pvp, boolean_t need_entry)
{
	unsigned int tofree, lcv;

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
pmap_free_pv_doit(struct pv_entry *pv)
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
pmap_free_pv(struct pmap *pmap, struct pv_entry *pv)
{
	simple_lock(&pvalloc_lock);
	pmap_free_pv_doit(pv);

	/*
	 * Can't free the PV page if the PV entries were associated with
	 * the kernel pmap; the pmap is already locked.
	 */
	if (pv_nfpvents > PVE_HIWAT && TAILQ_FIRST(&pv_unusedpgs) != NULL &&
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
pmap_free_pvs(struct pmap *pmap, struct pv_entry *pvs)
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
	if (pv_nfpvents > PVE_HIWAT && TAILQ_FIRST(&pv_unusedpgs) != NULL &&
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
 */

static void
pmap_free_pvpage(void)
{
	int s;
	struct vm_map *map;
	struct vm_map_entry *dead_entries;
	struct pv_page *pvp;

	s = splvm(); /* protect kmem_map */

	pvp = TAILQ_FIRST(&pv_unusedpgs);

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
 *   pmap_enter_pv: enter a mapping onto a vm_page list
 *   pmap_remove_pv: remove a mappiing from a vm_page list
 *
 * NOTE: pmap_enter_pv expects to lock the pvh itself
 *       pmap_remove_pv expects te caller to lock the pvh before calling
 */

/*
 * pmap_enter_pv: enter a mapping onto a vm_page lst
 *
 * => caller should hold the proper lock on pmap_main_lock
 * => caller should have pmap locked
 * => we will gain the lock on the vm_page and allocate the new pv_entry
 * => caller should adjust ptp's wire_count before calling
 * => caller should not adjust pmap's wire_count
 */

__inline static void
pmap_enter_pv(struct vm_page *pg, struct pv_entry *pve, struct pmap *pmap,
    vaddr_t va, struct vm_page *ptp, unsigned int flags)
{
	pve->pv_pmap = pmap;
	pve->pv_va = va;
	pve->pv_ptp = ptp;			/* NULL for kernel pmap */
	pve->pv_flags = flags;
	simple_lock(&pg->mdpage.pvh_slock);	/* lock vm_page */
	pve->pv_next = pg->mdpage.pvh_list;	/* add to ... */
	pg->mdpage.pvh_list = pve;		/* ... locked list */
	simple_unlock(&pg->mdpage.pvh_slock);	/* unlock, done! */
	if (pve->pv_flags & PVF_WIRED)
		++pmap->pm_stats.wired_count;
#ifdef PMAP_ALIAS_DEBUG
    {
	int s = splhigh();
	if (pve->pv_flags & PVF_WRITE)
		pg->mdpage.rw_mappings++;
	else
		pg->mdpage.ro_mappings++;
	if (pg->mdpage.rw_mappings != 0 &&
	    (pg->mdpage.kro_mappings != 0 || pg->mdpage.krw_mappings != 0)) {
		printf("pmap_enter_pv: rw %u, kro %u, krw %u\n",
		    pg->mdpage.rw_mappings, pg->mdpage.kro_mappings,
		    pg->mdpage.krw_mappings);
	}
	splx(s);
    }
#endif /* PMAP_ALIAS_DEBUG */
}

/*
 * pmap_remove_pv: try to remove a mapping from a pv_list
 *
 * => caller should hold proper lock on pmap_main_lock
 * => pmap should be locked
 * => caller should hold lock on vm_page [so that attrs can be adjusted]
 * => caller should adjust ptp's wire_count and free PTP if needed
 * => caller should NOT adjust pmap's wire_count
 * => we return the removed pve
 */

__inline static struct pv_entry *
pmap_remove_pv(struct vm_page *pg, struct pmap *pmap, vaddr_t va)
{
	struct pv_entry *pve, **prevptr;

	prevptr = &pg->mdpage.pvh_list;		/* previous pv_entry pointer */
	pve = *prevptr;
	while (pve) {
		if (pve->pv_pmap == pmap && pve->pv_va == va) {	/* match? */
			*prevptr = pve->pv_next;		/* remove it! */
			if (pve->pv_flags & PVF_WIRED)
			    --pmap->pm_stats.wired_count;
#ifdef PMAP_ALIAS_DEBUG
    {
			int s = splhigh();
			if (pve->pv_flags & PVF_WRITE) {
				KASSERT(pg->mdpage.rw_mappings != 0);
				pg->mdpage.rw_mappings--;
			} else {
				KASSERT(pg->mdpage.ro_mappings != 0);
				pg->mdpage.ro_mappings--;
			}
			splx(s);
    }
#endif /* PMAP_ALIAS_DEBUG */
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
 * => caller should hold lock on vm_page [so that attrs can be adjusted]
 * => caller should NOT adjust pmap's wire_count
 * => caller must call pmap_vac_me_harder() if writable status of a page
 *    may have changed.
 * => we return the old flags
 * 
 * Modify a physical-virtual mapping in the pv table
 */

static /* __inline */ u_int
pmap_modify_pv(struct pmap *pmap, vaddr_t va, struct vm_page *pg,
    u_int bic_mask, u_int eor_mask)
{
	struct pv_entry *npv;
	u_int flags, oflags;

	/*
	 * There is at least one VA mapping this page.
	 */

	for (npv = pg->mdpage.pvh_list; npv; npv = npv->pv_next) {
		if (pmap == npv->pv_pmap && va == npv->pv_va) {
			oflags = npv->pv_flags;
			npv->pv_flags = flags =
			    ((oflags & ~bic_mask) ^ eor_mask);
			if ((flags ^ oflags) & PVF_WIRED) {
				if (flags & PVF_WIRED)
					++pmap->pm_stats.wired_count;
				else
					--pmap->pm_stats.wired_count;
			}
#ifdef PMAP_ALIAS_DEBUG
    {
			int s = splhigh();
			if ((flags ^ oflags) & PVF_WRITE) {
				if (flags & PVF_WRITE) {
					pg->mdpage.rw_mappings++;
					pg->mdpage.ro_mappings--;
					if (pg->mdpage.rw_mappings != 0 &&
					    (pg->mdpage.kro_mappings != 0 ||
					     pg->mdpage.krw_mappings != 0)) {
						printf("pmap_modify_pv: rw %u, "
						    "kro %u, krw %u\n",
						    pg->mdpage.rw_mappings,
						    pg->mdpage.kro_mappings,
						    pg->mdpage.krw_mappings);
					}
				} else {
					KASSERT(pg->mdpage.rw_mappings != 0);
					pg->mdpage.rw_mappings--;
					pg->mdpage.ro_mappings++;
				}
			}
			splx(s);
    }
#endif /* PMAP_ALIAS_DEBUG */
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
#define	PMAP_PTP_SELFREF	0x01
#define	PMAP_PTP_CACHEABLE	0x02

static __inline void
pmap_map_in_l1(struct pmap *pmap, vaddr_t va, paddr_t l2pa, unsigned int flags)
{
	vaddr_t ptva;

	KASSERT((va & PD_OFFSET) == 0);		/* XXX KDASSERT */

	/* Calculate the index into the L1 page table. */
	ptva = va >> L1_S_SHIFT;

	/* Map page table into the L1. */
	pmap->pm_pdir[ptva + 0] = L1_C_PROTO | (l2pa + 0x000);
	pmap->pm_pdir[ptva + 1] = L1_C_PROTO | (l2pa + 0x400);
	pmap->pm_pdir[ptva + 2] = L1_C_PROTO | (l2pa + 0x800);
	pmap->pm_pdir[ptva + 3] = L1_C_PROTO | (l2pa + 0xc00);
	cpu_dcache_wb_range((vaddr_t) &pmap->pm_pdir[ptva + 0], 16);

	/* Map the page table into the page table area. */
	if (flags & PMAP_PTP_SELFREF) {
		*((pt_entry_t *)(pmap->pm_vptpt + ptva)) = L2_S_PROTO | l2pa |
		    L2_S_PROT(PTE_KERNEL, VM_PROT_READ|VM_PROT_WRITE) |
		    ((flags & PMAP_PTP_CACHEABLE) ? pte_l2_s_cache_mode : 0);
		PTE_SYNC_CURRENT(pmap, (pt_entry_t *)(pmap->pm_vptpt + ptva));
	}
}

#if 0
static __inline void
pmap_unmap_in_l1(struct pmap *pmap, vaddr_t va)
{
	vaddr_t ptva;

	KASSERT((va & PD_OFFSET) == 0);		/* XXX KDASSERT */

	/* Calculate the index into the L1 page table. */
	ptva = va >> L1_S_SHIFT;

	/* Unmap page table from the L1. */
	pmap->pm_pdir[ptva + 0] = 0;
	pmap->pm_pdir[ptva + 1] = 0;
	pmap->pm_pdir[ptva + 2] = 0;
	pmap->pm_pdir[ptva + 3] = 0;
	cpu_dcache_wb_range((vaddr_t) &pmap->pm_pdir[ptva + 0], 16);

	/* Unmap the page table from the page table area. */
	*((pt_entry_t *)(pmap->pm_vptpt + ptva)) = 0;
	PTE_SYNC_CURRENT(pmap, (pt_entry_t *)(pmap->pm_vptpt + ptva));
}
#endif

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

char *boot_head;

void
pmap_bootstrap(pd_entry_t *kernel_l1pt, pv_addr_t kernel_ptpt)
{
	pt_entry_t *pte;

	pmap_kernel()->pm_pdir = kernel_l1pt;
	pmap_kernel()->pm_pptpt = kernel_ptpt.pv_pa;
	pmap_kernel()->pm_vptpt = kernel_ptpt.pv_va;
	simple_lock_init(&pmap_kernel()->pm_lock);
	pmap_kernel()->pm_obj.pgops = NULL;
	TAILQ_INIT(&(pmap_kernel()->pm_obj.memq));
	pmap_kernel()->pm_obj.uo_npages = 0;
	pmap_kernel()->pm_obj.uo_refs = 1;

	virtual_avail = KERNEL_VM_BASE;
	virtual_end = KERNEL_VM_BASE + KERNEL_VM_SIZE;

	/*
	 * now we allocate the "special" VAs which are used for tmp mappings
	 * by the pmap (and other modules).  we allocate the VAs by advancing
	 * virtual_avail (note that there are no pages mapped at these VAs).
	 * we find the PTE that maps the allocated VA via the linear PTE
	 * mapping.
	 */

	pte = ((pt_entry_t *) PTE_BASE) + atop(virtual_avail);

	csrcp = virtual_avail; csrc_pte = pte;
	virtual_avail += PAGE_SIZE; pte++;

	cdstp = virtual_avail; cdst_pte = pte;
	virtual_avail += PAGE_SIZE; pte++;

	memhook = (char *) virtual_avail;	/* don't need pte */
	*pte = 0;
	virtual_avail += PAGE_SIZE; pte++;

	msgbufaddr = (caddr_t) virtual_avail;	/* don't need pte */
	virtual_avail += round_page(MSGBUFSIZE);
	pte += atop(round_page(MSGBUFSIZE));

	/*
	 * init the static-global locks and global lists.
	 */
	spinlockinit(&pmap_main_lock, "pmaplk", 0);
	simple_lock_init(&pvalloc_lock);
	simple_lock_init(&pmaps_lock);
	LIST_INIT(&pmaps);
	TAILQ_INIT(&pv_freepages);
	TAILQ_INIT(&pv_unusedpgs);

	/*
	 * initialize the pmap pool.
	 */

	pool_init(&pmap_pmap_pool, sizeof(struct pmap), 0, 0, 0, "pmappl",
		  &pool_allocator_nointr);

	/*
	 * initialize the PT-PT pool and cache.
	 */

	pool_init(&pmap_ptpt_pool, PAGE_SIZE, 0, 0, 0, "ptptpl",
		  &pmap_ptpt_allocator);
	pool_cache_init(&pmap_ptpt_cache, &pmap_ptpt_pool,
			pmap_ptpt_ctor, NULL, NULL);

	cpu_dcache_wbinv_all();
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
pmap_init(void)
{

	/*
	 * Set the available memory vars - These do not map to real memory
	 * addresses and cannot as the physical memory is fragmented.
	 * They are used by ps for %mem calculations.
	 * One could argue whether this should be the entire memory or just
	 * the memory that is useable in a user process.
	 */
	avail_start = 0;
	avail_end = physmem * PAGE_SIZE;

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
pmap_postinit(void)
{
	unsigned int loop;
	struct l1pt *pt;

#ifdef PMAP_STATIC_L1S
	for (loop = 0; loop < PMAP_STATIC_L1S; ++loop) {
#else	/* PMAP_STATIC_L1S */
	for (loop = 0; loop < max_processes; ++loop) {
#endif	/* PMAP_STATIC_L1S */
		/* Allocate a L1 page table */
		pt = pmap_alloc_l1pt();
		if (!pt)
			panic("Cannot allocate static L1 page tables");

		/* Clean it */
		bzero((void *)pt->pt_va, L1_TABLE_SIZE);
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
pmap_create(void)
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
	pmap->pm_ptphint = NULL;

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

	/* Allocate virtual address space for the L1 page table */
	va = uvm_km_valloc(kernel_map, L1_TABLE_SIZE);
	if (va == 0) {
#ifdef DIAGNOSTIC
		PDEBUG(0,
		    printf("pmap: Cannot allocate pageable memory for L1\n"));
#endif	/* DIAGNOSTIC */
		return(NULL);
	}

	/* Allocate memory for the l1pt structure */
	pt = (struct l1pt *)malloc(sizeof(struct l1pt), M_VMPMAP, M_WAITOK);

	/*
	 * Allocate pages from the VM system.
	 */
	error = uvm_pglistalloc(L1_TABLE_SIZE, physical_start, physical_end,
	    L1_TABLE_SIZE, 0, &pt->pt_plist, 1, M_WAITOK);
	if (error) {
#ifdef DIAGNOSTIC
		PDEBUG(0,
		    printf("pmap: Cannot allocate physical mem for L1 (%d)\n",
		    error));
#endif	/* DIAGNOSTIC */
		/* Release the resources we already have claimed */
		free(pt, M_VMPMAP);
		uvm_km_free(kernel_map, va, L1_TABLE_SIZE);
		return(NULL);
	}

	/* Map our physical pages into our virtual space */
	pt->pt_va = va;
	m = TAILQ_FIRST(&pt->pt_plist);
	while (m && va < (pt->pt_va + L1_TABLE_SIZE)) {
		pa = VM_PAGE_TO_PHYS(m);

		pmap_kenter_pa(va, pa, VM_PROT_READ|VM_PROT_WRITE);

		va += PAGE_SIZE;
		m = m->pageq.tqe_next;
	}

#ifdef DIAGNOSTIC
	if (m)
		panic("pmap_alloc_l1pt: pglist not empty");
#endif	/* DIAGNOSTIC */

	pt->pt_flags = 0;
	return(pt);
}

/*
 * Free a L1 page table previously allocated with pmap_alloc_l1pt().
 */
static void
pmap_free_l1pt(struct l1pt *pt)
{
	/* Separate the physical memory for the virtual space */
	pmap_kremove(pt->pt_va, L1_TABLE_SIZE);
	pmap_update(pmap_kernel());

	/* Return the physical memory */
	uvm_pglistfree(&pt->pt_plist);

	/* Free the virtual space */
	uvm_km_free(kernel_map, pt->pt_va, L1_TABLE_SIZE);

	/* Free the l1pt structure */
	free(pt, M_VMPMAP);
}

/*
 * pmap_ptpt_page_alloc:
 *
 *	Back-end page allocator for the PT-PT pool.
 */
static void *
pmap_ptpt_page_alloc(struct pool *pp, int flags)
{
	struct vm_page *pg;
	pt_entry_t *pte;
	vaddr_t va;

	/* XXX PR_WAITOK? */
	va = uvm_km_valloc(kernel_map, L2_TABLE_SIZE);
	if (va == 0)
		return (NULL);

	for (;;) {
		pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO);
		if (pg != NULL)
			break;
		if ((flags & PR_WAITOK) == 0) {
			uvm_km_free(kernel_map, va, L2_TABLE_SIZE);
			return (NULL);
		}
		uvm_wait("pmap_ptpt");
	}

	pte = vtopte(va);
	KDASSERT(pmap_pte_v(pte) == 0);

	*pte = L2_S_PROTO | VM_PAGE_TO_PHYS(pg) |
	     L2_S_PROT(PTE_KERNEL, VM_PROT_READ|VM_PROT_WRITE);
	PTE_SYNC(pte);
#ifdef PMAP_ALIAS_DEBUG
    {
	int s = splhigh();
	pg->mdpage.krw_mappings++;
	splx(s);
    }
#endif /* PMAP_ALIAS_DEBUG */

	return ((void *) va);
}

/*
 * pmap_ptpt_page_free:
 *
 *	Back-end page free'er for the PT-PT pool.
 */
static void
pmap_ptpt_page_free(struct pool *pp, void *v)
{
	vaddr_t va = (vaddr_t) v;
	paddr_t pa;

	pa = vtophys(va);

	pmap_kremove(va, L2_TABLE_SIZE);
	pmap_update(pmap_kernel());

	uvm_pagefree(PHYS_TO_VM_PAGE(pa));

	uvm_km_free(kernel_map, va, L2_TABLE_SIZE);
}

/*
 * pmap_ptpt_ctor:
 *
 *	Constructor for the PT-PT cache.
 */
static int
pmap_ptpt_ctor(void *arg, void *object, int flags)
{
	caddr_t vptpt = object;

	/* Page is already zero'd. */

	/*
	 * Map in kernel PTs.
	 *
	 * XXX THIS IS CURRENTLY DONE AS UNCACHED MEMORY ACCESS.
	 */
	memcpy(vptpt + ((L1_TABLE_SIZE - KERNEL_PD_SIZE) >> 2),
	       (char *)(PTE_BASE + (PTE_BASE >> (PGSHIFT - 2)) +
			((L1_TABLE_SIZE - KERNEL_PD_SIZE) >> 2)),
	       (KERNEL_PD_SIZE >> 2));

	return (0);
}

/*
 * Allocate a page directory.
 * This routine will either allocate a new page directory from the pool
 * of L1 page tables currently held by the kernel or it will allocate
 * a new one via pmap_alloc_l1pt().
 * It will then initialise the l1 page table for use.
 */
static int
pmap_allocpagedir(struct pmap *pmap)
{
	vaddr_t vptpt;
	struct l1pt *pt;
	u_int gen;

	PDEBUG(0, printf("pmap_allocpagedir(%p)\n", pmap));

	/* Do we have any spare L1's lying around ? */
	if (l1pt_static_queue_count) {
		--l1pt_static_queue_count;
		pt = SIMPLEQ_FIRST(&l1pt_static_queue);
		SIMPLEQ_REMOVE_HEAD(&l1pt_static_queue, pt_queue);
	} else if (l1pt_queue_count) {
		--l1pt_queue_count;
		pt = SIMPLEQ_FIRST(&l1pt_queue);
		SIMPLEQ_REMOVE_HEAD(&l1pt_queue, pt_queue);
		++l1pt_reuse_count;
	} else {
		pt = pmap_alloc_l1pt();
		if (!pt)
			return(ENOMEM);
		++l1pt_create_count;
	}

	/* Store the pointer to the l1 descriptor in the pmap. */
	pmap->pm_l1pt = pt;

	/* Store the virtual address of the l1 in the pmap. */
	pmap->pm_pdir = (pd_entry_t *)pt->pt_va;

	/* Clean the L1 if it is dirty */
	if (!(pt->pt_flags & PTFLAG_CLEAN)) {
		bzero((void *)pmap->pm_pdir, (L1_TABLE_SIZE - KERNEL_PD_SIZE));
		cpu_dcache_wb_range((vaddr_t) pmap->pm_pdir,
		    (L1_TABLE_SIZE - KERNEL_PD_SIZE));
	}

	/* Allocate a page table to map all the page tables for this pmap */
	KASSERT(pmap->pm_vptpt == 0);

 try_again:
	gen = pmap_ptpt_cache_generation;
	vptpt = (vaddr_t) pool_cache_get(&pmap_ptpt_cache, PR_WAITOK);
	if (vptpt == NULL) {
		PDEBUG(0, printf("pmap_alloc_pagedir: no KVA for PTPT\n"));
		pmap_freepagedir(pmap);
		return (ENOMEM);
	}

	/* need to lock this all up for growkernel */
	simple_lock(&pmaps_lock);

	if (gen != pmap_ptpt_cache_generation) {
		simple_unlock(&pmaps_lock);
		pool_cache_destruct_object(&pmap_ptpt_cache, (void *) vptpt);
		goto try_again;
	}

	pmap->pm_vptpt = vptpt;
	pmap->pm_pptpt = vtophys(vptpt);

	/* Duplicate the kernel mappings. */
	bcopy((char *)pmap_kernel()->pm_pdir + (L1_TABLE_SIZE - KERNEL_PD_SIZE),
		(char *)pmap->pm_pdir + (L1_TABLE_SIZE - KERNEL_PD_SIZE),
		KERNEL_PD_SIZE);
	cpu_dcache_wb_range((vaddr_t)pmap->pm_pdir +
	    (L1_TABLE_SIZE - KERNEL_PD_SIZE), KERNEL_PD_SIZE);

	/* Wire in this page table */
	pmap_map_in_l1(pmap, PTE_BASE, pmap->pm_pptpt, PMAP_PTP_SELFREF);

	pt->pt_flags &= ~PTFLAG_CLEAN;	/* L1 is dirty now */

	LIST_INSERT_HEAD(&pmaps, pmap, pm_list);
	simple_unlock(&pmaps_lock);
	
	return(0);
}


/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */

void
pmap_pinit(struct pmap *pmap)
{
	unsigned int backoff = 6;
	unsigned int retry = 10;

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
		 *
		 * Searching for a suitable L1 PT is expensive:
		 * to avoid hogging the system when memory is really
		 * scarce, use an exponential back-off so that
		 * eventually we won't retry more than once every 8
		 * seconds.  This should allow other processes to run
		 * to completion and free up resources.
		 */
		(void) ltsleep(&lbolt, PVM, "l1ptwait", (hz << 3) >> backoff,
		    NULL);
		if (--retry == 0) {
			retry = 10;
			if (backoff)
				--backoff;
		}
	}

	if (vector_page < KERNEL_BASE) {
		/*
		 * Map the vector page.  This will also allocate and map
		 * an L2 table for it.
		 */
		pmap_enter(pmap, vector_page, systempage.pv_pa,
		    VM_PROT_READ, VM_PROT_READ | PMAP_WIRED);
		pmap_update(pmap);
	}
}

void
pmap_freepagedir(struct pmap *pmap)
{
	/* Free the memory used for the page table mapping */
	if (pmap->pm_vptpt != 0) {
		/*
		 * XXX Objects freed to a pool cache must be in constructed
		 * XXX form when freed, but we don't free page tables as we
		 * XXX go, so we need to zap the mappings here.
		 *
		 * XXX THIS IS CURRENTLY DONE AS UNCACHED MEMORY ACCESS.
		 */
		memset((caddr_t) pmap->pm_vptpt, 0,
		       ((L1_TABLE_SIZE - KERNEL_PD_SIZE) >> 2));
		pool_cache_put(&pmap_ptpt_cache, (void *) pmap->pm_vptpt);
	}

	/* junk the L1 page table */
	if (pmap->pm_l1pt->pt_flags & PTFLAG_STATIC) {
		/* Add the page table to the queue */
		SIMPLEQ_INSERT_TAIL(&l1pt_static_queue,
				    pmap->pm_l1pt, pt_queue);
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
pmap_destroy(struct pmap *pmap)
{
	struct vm_page *page;
	unsigned int count;

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

	/*
	 * remove it from global list of pmaps
	 */

	simple_lock(&pmaps_lock);
	LIST_REMOVE(pmap, pm_list);
	simple_unlock(&pmaps_lock);
	
	if (vector_page < KERNEL_BASE) {
		/* Remove the vector page mapping */
		pmap_remove(pmap, vector_page, vector_page + PAGE_SIZE);
		pmap_update(pmap);
	}

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
	/*
	 * vmobjlock must be held while freeing pages
	 */
	simple_lock(&pmap->pm_obj.vmobjlock);
	while ((page = TAILQ_FIRST(&pmap->pm_obj.memq)) != NULL) {
		KASSERT((page->flags & PG_BUSY) == 0);

		/* Freeing a PT page?  The contents are a throw-away. */
		KASSERT((page->offset & PD_OFFSET) == 0);/* XXX KDASSERT */
		cpu_dcache_inv_range((vaddr_t)vtopte(page->offset), PAGE_SIZE);

		page->wire_count = 0;
		uvm_pagefree(page);
	}
	simple_unlock(&pmap->pm_obj.vmobjlock);

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
pmap_reference(struct pmap *pmap)
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
pmap_virtual_space(vaddr_t *start, vaddr_t *end)
{
	*start = virtual_avail;
	*end = virtual_end;
}

/*
 * Activate the address space for the specified process.  If the process
 * is the current process, load the new MMU context.
 */
void
pmap_activate(struct lwp *l)
{
	struct pmap *pmap = l->l_proc->p_vmspace->vm_map.pmap;
	struct pcb *pcb = &l->l_addr->u_pcb;

	(void) pmap_extract(pmap_kernel(), (vaddr_t)pmap->pm_pdir,
	    &pcb->pcb_pagedir);

	PDEBUG(0,
	    printf("pmap_activate: l=%p pmap=%p pcb=%p pdir=%p l1=0x%lx\n",
	    l, pmap, pcb, pmap->pm_pdir, (u_long) pcb->pcb_pagedir));

	if (l == curlwp) {
		PDEBUG(0, printf("pmap_activate: setting TTB\n"));
		setttb(pcb->pcb_pagedir);
	}
}

/*
 * Deactivate the address space of the specified process.
 */
void
pmap_deactivate(struct lwp *l)
{
}

/*
 * Perform any deferred pmap operations.
 */
void
pmap_update(struct pmap *pmap)
{

	/*
	 * We haven't deferred any pmap operations, but we do need to
	 * make sure TLB/cache operations have completed.
	 */
	cpu_cpwait();
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
pmap_clean_page(struct pv_entry *pv, boolean_t is_src)
{
	struct pmap *pmap;
	struct pv_entry *npv;
	boolean_t cache_needs_cleaning = FALSE;
	vaddr_t page_to_clean = 0;

	if (pv == NULL) {
		/* nothing mapped in so nothing to flush */
		return (0);
	}

	/*
	 * Since we flush the cache each time we change curlwp, we
	 * only need to flush the page if it is in the current pmap.
	 */
	if (curproc)
		pmap = curproc->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	for (npv = pv; npv; npv = npv->pv_next) {
		if (npv->pv_pmap == pmap) {
			/*
			 * The page is mapped non-cacheable in 
			 * this map.  No need to flush the cache.
			 */
			if (npv->pv_flags & PVF_NC) {
#ifdef DIAGNOSTIC
				if (cache_needs_cleaning)
					panic("pmap_clean_page: "
					    "cache inconsistency");
#endif
				break;
			} else if (is_src && (npv->pv_flags & PVF_WRITE) == 0)
				continue;
			if (cache_needs_cleaning) {
				page_to_clean = 0;
				break;
			} else
				page_to_clean = npv->pv_va;
			cache_needs_cleaning = TRUE;
		}
	}

	if (page_to_clean) {
		/*
		 * XXX If is_src, we really only need to write-back,
		 * XXX not invalidate, too.  Investigate further.
		 * XXX --thorpej@netbsd.org
		 */
		cpu_idcache_wbinv_range(page_to_clean, PAGE_SIZE);
	} else if (cache_needs_cleaning) {
		cpu_idcache_wbinv_all();
		return (1);
	}
	return (0);
}

/*
 * pmap_zero_page()
 * 
 * Zero a given physical page by mapping it at a page hook point.
 * In doing the zero page op, the page we zero is mapped cachable, as with
 * StrongARM accesses to non-cached pages are non-burst making writing
 * _any_ bulk data very slow.
 */
#if ARM_MMU_GENERIC == 1
void
pmap_zero_page_generic(paddr_t phys)
{
#ifdef DEBUG
	struct vm_page *pg = PHYS_TO_VM_PAGE(phys);

	if (pg->mdpage.pvh_list != NULL)
		panic("pmap_zero_page: page has mappings");
#endif

	KDASSERT((phys & PGOFSET) == 0);

	/*
	 * Hook in the page, zero it, and purge the cache for that
	 * zeroed page. Invalidate the TLB as needed.
	 */
	*cdst_pte = L2_S_PROTO | phys |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) | pte_l2_s_cache_mode;
	PTE_SYNC(cdst_pte);
	cpu_tlb_flushD_SE(cdstp);
	cpu_cpwait();
	bzero_page(cdstp);
	cpu_dcache_wbinv_range(cdstp, PAGE_SIZE);
}
#endif /* ARM_MMU_GENERIC == 1 */

#if ARM_MMU_XSCALE == 1
void
pmap_zero_page_xscale(paddr_t phys)
{
#ifdef DEBUG
	struct vm_page *pg = PHYS_TO_VM_PAGE(phys);

	if (pg->mdpage.pvh_list != NULL)
		panic("pmap_zero_page: page has mappings");
#endif

	KDASSERT((phys & PGOFSET) == 0);

	/*
	 * Hook in the page, zero it, and purge the cache for that
	 * zeroed page. Invalidate the TLB as needed.
	 */
	*cdst_pte = L2_S_PROTO | phys |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) |
	    L2_C | L2_XSCALE_T_TEX(TEX_XSCALE_X);	/* mini-data */
	PTE_SYNC(cdst_pte);
	cpu_tlb_flushD_SE(cdstp);
	cpu_cpwait();
	bzero_page(cdstp);
	xscale_cache_clean_minidata();
}
#endif /* ARM_MMU_XSCALE == 1 */

/* pmap_pageidlezero()
 *
 * The same as above, except that we assume that the page is not
 * mapped.  This means we never have to flush the cache first.  Called
 * from the idle loop.
 */
boolean_t
pmap_pageidlezero(paddr_t phys)
{
	unsigned int i;
	int *ptr;
	boolean_t rv = TRUE;
#ifdef DEBUG
	struct vm_page *pg;
	
	pg = PHYS_TO_VM_PAGE(phys);
	if (pg->mdpage.pvh_list != NULL)
		panic("pmap_pageidlezero: page has mappings");
#endif

	KDASSERT((phys & PGOFSET) == 0);

	/*
	 * Hook in the page, zero it, and purge the cache for that
	 * zeroed page. Invalidate the TLB as needed.
	 */
	*cdst_pte = L2_S_PROTO | phys |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) | pte_l2_s_cache_mode;
	PTE_SYNC(cdst_pte);
	cpu_tlb_flushD_SE(cdstp);
	cpu_cpwait();

	for (i = 0, ptr = (int *)cdstp;
			i < (PAGE_SIZE / sizeof(int)); i++) {
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
		cpu_dcache_wbinv_range(cdstp, PAGE_SIZE);
	return (rv);
}
 
/*
 * pmap_copy_page()
 *
 * Copy one physical page into another, by mapping the pages into
 * hook points. The same comment regarding cachability as in
 * pmap_zero_page also applies here.
 */
#if ARM_MMU_GENERIC == 1
void
pmap_copy_page_generic(paddr_t src, paddr_t dst)
{
	struct vm_page *src_pg = PHYS_TO_VM_PAGE(src);
#ifdef DEBUG
	struct vm_page *dst_pg = PHYS_TO_VM_PAGE(dst);

	if (dst_pg->mdpage.pvh_list != NULL)
		panic("pmap_copy_page: dst page has mappings");
#endif

	KDASSERT((src & PGOFSET) == 0);
	KDASSERT((dst & PGOFSET) == 0);

	/*
	 * Clean the source page.  Hold the source page's lock for
	 * the duration of the copy so that no other mappings can
	 * be created while we have a potentially aliased mapping.
	 */
	simple_lock(&src_pg->mdpage.pvh_slock);
	(void) pmap_clean_page(src_pg->mdpage.pvh_list, TRUE);

	/*
	 * Map the pages into the page hook points, copy them, and purge
	 * the cache for the appropriate page. Invalidate the TLB
	 * as required.
	 */
	*csrc_pte = L2_S_PROTO | src |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_READ) | pte_l2_s_cache_mode;
	PTE_SYNC(csrc_pte);
	*cdst_pte = L2_S_PROTO | dst |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) | pte_l2_s_cache_mode;
	PTE_SYNC(cdst_pte);
	cpu_tlb_flushD_SE(csrcp);
	cpu_tlb_flushD_SE(cdstp);
	cpu_cpwait();
	bcopy_page(csrcp, cdstp);
	cpu_dcache_inv_range(csrcp, PAGE_SIZE);
	simple_unlock(&src_pg->mdpage.pvh_slock); /* cache is safe again */
	cpu_dcache_wbinv_range(cdstp, PAGE_SIZE);
}
#endif /* ARM_MMU_GENERIC == 1 */

#if ARM_MMU_XSCALE == 1
void
pmap_copy_page_xscale(paddr_t src, paddr_t dst)
{
	struct vm_page *src_pg = PHYS_TO_VM_PAGE(src);
#ifdef DEBUG
	struct vm_page *dst_pg = PHYS_TO_VM_PAGE(dst);

	if (dst_pg->mdpage.pvh_list != NULL)
		panic("pmap_copy_page: dst page has mappings");
#endif

	KDASSERT((src & PGOFSET) == 0);
	KDASSERT((dst & PGOFSET) == 0);

	/*
	 * Clean the source page.  Hold the source page's lock for
	 * the duration of the copy so that no other mappings can
	 * be created while we have a potentially aliased mapping.
	 */
	simple_lock(&src_pg->mdpage.pvh_slock);
	(void) pmap_clean_page(src_pg->mdpage.pvh_list, TRUE);

	/*
	 * Map the pages into the page hook points, copy them, and purge
	 * the cache for the appropriate page. Invalidate the TLB
	 * as required.
	 */
	*csrc_pte = L2_S_PROTO | src |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_READ) |
	    L2_C | L2_XSCALE_T_TEX(TEX_XSCALE_X);	/* mini-data */
	PTE_SYNC(csrc_pte);
	*cdst_pte = L2_S_PROTO | dst |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) |
	    L2_C | L2_XSCALE_T_TEX(TEX_XSCALE_X);	/* mini-data */
	PTE_SYNC(cdst_pte);
	cpu_tlb_flushD_SE(csrcp);
	cpu_tlb_flushD_SE(cdstp);
	cpu_cpwait();
	bcopy_page(csrcp, cdstp);
	simple_unlock(&src_pg->mdpage.pvh_slock); /* cache is safe again */
	xscale_cache_clean_minidata();
}
#endif /* ARM_MMU_XSCALE == 1 */

#if 0
void
pmap_pte_addref(struct pmap *pmap, vaddr_t va)
{
	pd_entry_t *pde;
	paddr_t pa;
	struct vm_page *m;

	if (pmap == pmap_kernel())
		return;

	pde = pmap_pde(pmap, va & PD_FRAME);
	pa = pmap_pte_pa(pde);
	m = PHYS_TO_VM_PAGE(pa);
	m->wire_count++;
#ifdef MYCROFT_HACK
	printf("addref pmap=%p va=%08lx pde=%p pa=%08lx m=%p wire=%d\n",
	    pmap, va, pde, pa, m, m->wire_count);
#endif
}

void
pmap_pte_delref(struct pmap *pmap, vaddr_t va)
{
	pd_entry_t *pde;
	paddr_t pa;
	struct vm_page *m;

	if (pmap == pmap_kernel())
		return;

	pde = pmap_pde(pmap, va & PD_FRAME);
	pa = pmap_pte_pa(pde);
	m = PHYS_TO_VM_PAGE(pa);
	m->wire_count--;
#ifdef MYCROFT_HACK
	printf("delref pmap=%p va=%08lx pde=%p pa=%08lx m=%p wire=%d\n",
	    pmap, va, pde, pa, m, m->wire_count);
#endif
	if (m->wire_count == 0) {
#ifdef MYCROFT_HACK
		printf("delref pmap=%p va=%08lx pde=%p pa=%08lx m=%p\n",
		    pmap, va, pde, pa, m);
#endif
		pmap_unmap_in_l1(pmap, va & PD_FRAME);
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
 * The code implements the following logic, where:
 *
 * KW = # of kernel read/write pages
 * KR = # of kernel read only pages
 * UW = # of user read/write pages
 * UR = # of user read only pages
 * OW = # of user read/write pages in another pmap, then
 * 
 * KC = kernel mapping is cacheable
 * UC = user mapping is cacheable
 *
 *                     KW=0,KR=0  KW=0,KR>0  KW=1,KR=0  KW>1,KR>=0
 *                   +---------------------------------------------
 * UW=0,UR=0,OW=0    | ---        KC=1       KC=1       KC=0
 * UW=0,UR>0,OW=0    | UC=1       KC=1,UC=1  KC=0,UC=0  KC=0,UC=0
 * UW=0,UR>0,OW>0    | UC=1       KC=0,UC=1  KC=0,UC=0  KC=0,UC=0
 * UW=1,UR=0,OW=0    | UC=1       KC=0,UC=0  KC=0,UC=0  KC=0,UC=0
 * UW>1,UR>=0,OW>=0  | UC=0       KC=0,UC=0  KC=0,UC=0  KC=0,UC=0
 *
 * Note that the pmap must have it's ptes mapped in, and passed with ptes.
 */
__inline static void
pmap_vac_me_harder(struct pmap *pmap, struct vm_page *pg, pt_entry_t *ptes,
	boolean_t clear_cache)
{
	if (pmap == pmap_kernel())
		pmap_vac_me_kpmap(pmap, pg, ptes, clear_cache);
	else
		pmap_vac_me_user(pmap, pg, ptes, clear_cache);
}

static void
pmap_vac_me_kpmap(struct pmap *pmap, struct vm_page *pg, pt_entry_t *ptes,
	boolean_t clear_cache)
{
	unsigned int user_entries = 0;
	unsigned int user_writable = 0;
	unsigned int user_cacheable = 0;
	unsigned int kernel_entries = 0;
	unsigned int kernel_writable = 0;
	unsigned int kernel_cacheable = 0;
	struct pv_entry *pv;
	struct pmap *last_pmap = pmap;

#ifdef DIAGNOSTIC
	if (pmap != pmap_kernel())
		panic("pmap_vac_me_kpmap: pmap != pmap_kernel()");
#endif

	/* 
	 * Pass one, see if there are both kernel and user pmaps for
	 * this page.  Calculate whether there are user-writable or
	 * kernel-writable pages.
	 */
	for (pv = pg->mdpage.pvh_list; pv != NULL; pv = pv->pv_next) {
		if (pv->pv_pmap != pmap) {
			user_entries++;
			if (pv->pv_flags & PVF_WRITE)
				user_writable++;
			if ((pv->pv_flags & PVF_NC) == 0)
				user_cacheable++;
		} else {
			kernel_entries++;
			if (pv->pv_flags & PVF_WRITE)
				kernel_writable++;
			if ((pv->pv_flags & PVF_NC) == 0)
				kernel_cacheable++;
		}
	}

	/* 
	 * We know we have just been updating a kernel entry, so if
	 * all user pages are already cacheable, then there is nothing
	 * further to do.
	 */
	if (kernel_entries == 0 &&
	    user_cacheable == user_entries)
		return;

	if (user_entries) {
		/* 
		 * Scan over the list again, for each entry, if it
		 * might not be set correctly, call pmap_vac_me_user
		 * to recalculate the settings.
		 */
		for (pv = pg->mdpage.pvh_list; pv; pv = pv->pv_next) {
			/* 
			 * We know kernel mappings will get set
			 * correctly in other calls.  We also know
			 * that if the pmap is the same as last_pmap
			 * then we've just handled this entry.
			 */
			if (pv->pv_pmap == pmap || pv->pv_pmap == last_pmap)
				continue;
			/* 
			 * If there are kernel entries and this page
			 * is writable but non-cacheable, then we can
			 * skip this entry also.  
			 */
			if (kernel_entries > 0 &&
			    (pv->pv_flags & (PVF_NC | PVF_WRITE)) ==
			    (PVF_NC | PVF_WRITE))
				continue;
			/* 
			 * Similarly if there are no kernel-writable 
			 * entries and the page is already 
			 * read-only/cacheable.
			 */
			if (kernel_writable == 0 &&
			    (pv->pv_flags & (PVF_NC | PVF_WRITE)) == 0)
				continue;
			/* 
			 * For some of the remaining cases, we know
			 * that we must recalculate, but for others we
			 * can't tell if they are correct or not, so
			 * we recalculate anyway.
			 */
			pmap_unmap_ptes(last_pmap);
			last_pmap = pv->pv_pmap;
			ptes = pmap_map_ptes(last_pmap);
			pmap_vac_me_user(last_pmap, pg, ptes, 
			    pmap_is_curpmap(last_pmap));
		}
		/* Restore the pte mapping that was passed to us.  */
		if (last_pmap != pmap) {
			pmap_unmap_ptes(last_pmap);
			ptes = pmap_map_ptes(pmap);
		}
		if (kernel_entries == 0)
			return;
	}

	pmap_vac_me_user(pmap, pg, ptes, clear_cache);
	return;
}

static void
pmap_vac_me_user(struct pmap *pmap, struct vm_page *pg, pt_entry_t *ptes,
	boolean_t clear_cache)
{
	struct pmap *kpmap = pmap_kernel();
	struct pv_entry *pv, *npv;
	unsigned int entries = 0;
	unsigned int writable = 0;
	unsigned int cacheable_entries = 0;
	unsigned int kern_cacheable = 0;
	unsigned int other_writable = 0;

	pv = pg->mdpage.pvh_list;
	KASSERT(ptes != NULL);

	/*
	 * Count mappings and writable mappings in this pmap.
	 * Include kernel mappings as part of our own.
	 * Keep a pointer to the first one.
	 */
	for (npv = pv; npv; npv = npv->pv_next) {
		/* Count mappings in the same pmap */
		if (pmap == npv->pv_pmap ||
		    kpmap == npv->pv_pmap) {
			if (entries++ == 0)
				pv = npv;
			/* Cacheable mappings */
			if ((npv->pv_flags & PVF_NC) == 0) {
				cacheable_entries++;
				if (kpmap == npv->pv_pmap)
					kern_cacheable++;
			}
			/* Writable mappings */
			if (npv->pv_flags & PVF_WRITE)
				++writable;
		} else if (npv->pv_flags & PVF_WRITE)
			other_writable = 1;
	}

	PDEBUG(3,printf("pmap_vac_me_harder: pmap %p Entries %d, "
		"writable %d cacheable %d %s\n", pmap, entries, writable,
	    	cacheable_entries, clear_cache ? "clean" : "no clean"));
	
	/*
	 * Enable or disable caching as necessary.
	 * Note: the first entry might be part of the kernel pmap,
	 * so we can't assume this is indicative of the state of the
	 * other (maybe non-kpmap) entries.
	 */
	if ((entries > 1 && writable) ||
	    (entries > 0 && pmap == kpmap && other_writable)) {
		if (cacheable_entries == 0)
		    return;
		for (npv = pv; npv; npv = npv->pv_next) {
			if ((pmap == npv->pv_pmap 
			    || kpmap == npv->pv_pmap) && 
			    (npv->pv_flags & PVF_NC) == 0) {
				ptes[arm_btop(npv->pv_va)] &= ~L2_S_CACHE_MASK;
				PTE_SYNC_CURRENT(pmap,
				    &ptes[arm_btop(npv->pv_va)]);
 				npv->pv_flags |= PVF_NC;
				/*
				 * If this page needs flushing from the
				 * cache, and we aren't going to do it
				 * below, do it now.
				 */
				if ((cacheable_entries < 4 &&
				    (clear_cache || npv->pv_pmap == kpmap)) ||
				    (npv->pv_pmap == kpmap &&
				    !clear_cache && kern_cacheable < 4)) {
					cpu_idcache_wbinv_range(npv->pv_va,
					    PAGE_SIZE);
					cpu_tlb_flushID_SE(npv->pv_va);
				}
			}
		}
		if ((clear_cache && cacheable_entries >= 4) ||
		    kern_cacheable >= 4) {
			cpu_idcache_wbinv_all();
			cpu_tlb_flushID();
		}
		cpu_cpwait();
	} else if (entries > cacheable_entries) {
		/*
		 * Turn cacheing back on for some pages.  If it is a kernel
		 * page, only do so if there are no other writable pages.
		 */
		for (npv = pv; npv; npv = npv->pv_next) {
			if ((pmap == npv->pv_pmap ||
			    (kpmap == npv->pv_pmap && other_writable == 0)) && 
			    (npv->pv_flags & PVF_NC)) {
				ptes[arm_btop(npv->pv_va)] |=
				    pte_l2_s_cache_mode;
				PTE_SYNC_CURRENT(pmap,
				    &ptes[arm_btop(npv->pv_va)]);
				npv->pv_flags &= ~PVF_NC;
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
pmap_remove(struct pmap *pmap, vaddr_t sva, vaddr_t eva)
{
	unsigned int cleanlist_idx = 0;
	struct pagelist {
		vaddr_t va;
		pt_entry_t *pte;
	} cleanlist[PMAP_REMOVE_CLEAN_LIST_SIZE];
	pt_entry_t *pte = 0, *ptes;
	paddr_t pa;
	int pmap_active;
	struct vm_page *pg;
	struct pv_entry *pv_tofree = NULL;

	/* Exit quick if there is no pmap */
	if (!pmap)
		return;

	NPDEBUG(PDB_REMOVE, printf("pmap_remove: pmap=%p sva=%08lx eva=%08lx\n",
	    pmap, sva, eva));

	/*
	 * we lock in the pmap => vm_page direction
	 */
	PMAP_MAP_TO_HEAD_LOCK();
	
	ptes = pmap_map_ptes(pmap);
	/* Get a page table pointer */
	while (sva < eva) {
		if (pmap_pde_page(pmap_pde(pmap, sva)))
			break;
		sva = (sva & L1_S_FRAME) + L1_S_SIZE;
	}
	
	pte = &ptes[arm_btop(sva)];
	/* Note if the pmap is active thus require cache and tlb cleans */
	pmap_active = pmap_is_curpmap(pmap);

	/* Now loop along */
	while (sva < eva) {
		/* Check if we can move to the next PDE (l1 chunk) */
		if ((sva & L2_ADDR_BITS) == 0) {
			if (!pmap_pde_page(pmap_pde(pmap, sva))) {
				sva += L1_S_SIZE;
				pte += arm_btop(L1_S_SIZE);
				continue;
			}
		}

		/* We've found a valid PTE, so this page of PTEs has to go. */
		if (pmap_pte_v(pte)) {
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
				unsigned int cnt;

				/* Nuke everything if needed. */
				if (pmap_active) {
					cpu_idcache_wbinv_all();
					cpu_tlb_flushID();
				}

				/*
				 * Roll back the previous PTE list,
				 * and zero out the current PTE.
				 */
				for (cnt = 0;
				     cnt < PMAP_REMOVE_CLEAN_LIST_SIZE;
				     cnt++) {
					*cleanlist[cnt].pte = 0;
					if (pmap_active)
						PTE_SYNC(cleanlist[cnt].pte);
					else
						PTE_FLUSH(cleanlist[cnt].pte);
					pmap_pte_delref(pmap,
					    cleanlist[cnt].va);
				}
				*pte = 0;
				if (pmap_active)
					PTE_SYNC(pte);
				else
					PTE_FLUSH(pte);
				pmap_pte_delref(pmap, sva);
				cleanlist_idx++;
			} else {
				/*
				 * We've already nuked the cache and
				 * TLB, so just carry on regardless,
				 * and we won't need to do it again
				 */
				*pte = 0;
				if (pmap_active)
					PTE_SYNC(pte);
				else
					PTE_FLUSH(pte);
				pmap_pte_delref(pmap, sva);
			}

			/*
			 * Update flags. In a number of circumstances,
			 * we could cluster a lot of these and do a
			 * number of sequential pages in one go.
			 */
			if ((pg = PHYS_TO_VM_PAGE(pa)) != NULL) {
				struct pv_entry *pve;
				simple_lock(&pg->mdpage.pvh_slock);
				pve = pmap_remove_pv(pg, pmap, sva);
				pmap_vac_me_harder(pmap, pg, ptes, FALSE);
				simple_unlock(&pg->mdpage.pvh_slock);
				if (pve != NULL) {
					pve->pv_next = pv_tofree;
					pv_tofree = pve;
				}
			}
		} else if (pmap_active == 0)
			PTE_FLUSH(pte);
		sva += PAGE_SIZE;
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
				cpu_idcache_wbinv_range(cleanlist[cnt].va,
				    PAGE_SIZE);
				*cleanlist[cnt].pte = 0;
				cpu_tlb_flushID_SE(cleanlist[cnt].va);
				PTE_SYNC(cleanlist[cnt].pte);
			} else {
				*cleanlist[cnt].pte = 0;
				PTE_FLUSH(cleanlist[cnt].pte);
			}
			pmap_pte_delref(pmap, cleanlist[cnt].va);
		}
	}

	/* Delete pv entries */
	if (pv_tofree != NULL)
		pmap_free_pvs(pmap, pv_tofree);
	
	pmap_unmap_ptes(pmap);

	PMAP_MAP_TO_HEAD_UNLOCK();
}

/*
 * Routine:	pmap_page_remove
 * Function:
 *		Removes this physical page from
 *		all physical maps in which it resides.
 *		Reflects back modify bits to the pager.
 */

static void
pmap_page_remove(struct vm_page *pg)
{
	struct pv_entry *pv, *npv;
	struct pmap *pmap;
	pt_entry_t *pte, *ptes;

	PDEBUG(0, printf("pmap_page_remove: pa=%lx ", VM_PAGE_TO_PHYS(pg)));

	/* set vm_page => pmap locking */
	PMAP_HEAD_TO_MAP_LOCK();

	simple_lock(&pg->mdpage.pvh_slock);
	
	pv = pg->mdpage.pvh_list;
	if (pv == NULL) {
		PDEBUG(0, printf("free page\n"));
		simple_unlock(&pg->mdpage.pvh_slock);
		PMAP_HEAD_TO_MAP_UNLOCK();
		return;
	}
	pmap_clean_page(pv, FALSE);

	while (pv) {
		pmap = pv->pv_pmap;
		ptes = pmap_map_ptes(pmap);
		pte = &ptes[arm_btop(pv->pv_va)];

		PDEBUG(0, printf("[%p,%08x,%08lx,%08x] ", pmap, *pte,
		    pv->pv_va, pv->pv_flags));
#ifdef DEBUG
		if (pmap_pde_page(pmap_pde(pmap, pv->pv_va)) == 0 ||
		    pmap_pte_v(pte) == 0 ||
		    pmap_pte_pa(pte) != VM_PAGE_TO_PHYS(pg))
			panic("pmap_page_remove: bad mapping");
#endif	/* DEBUG */

		/*
		 * Update statistics
		 */
		--pmap->pm_stats.resident_count;

		/* Wired bit */
		if (pv->pv_flags & PVF_WIRED)
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
		PTE_SYNC_CURRENT(pmap, pte);
		pmap_pte_delref(pmap, pv->pv_va);

		npv = pv->pv_next;
		pmap_free_pv(pmap, pv);
		pv = npv;
		pmap_unmap_ptes(pmap);
	}
	pg->mdpage.pvh_list = NULL;
	simple_unlock(&pg->mdpage.pvh_slock);
	PMAP_HEAD_TO_MAP_UNLOCK();	

	PDEBUG(0, printf("done\n"));
	cpu_tlb_flushID();
	cpu_cpwait();
}


/*
 * Set the physical protection on the specified range of this map as requested.
 */

void
pmap_protect(struct pmap *pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	pt_entry_t *pte = NULL, *ptes;
	struct vm_page *pg;
	boolean_t flush = FALSE;

	PDEBUG(0, printf("pmap_protect: pmap=%p %08lx->%08lx %x\n",
	    pmap, sva, eva, prot));

	if (~prot & VM_PROT_READ) {
		/*
		 * Just remove the mappings.  pmap_update() is not required
		 * here since the caller should do it.
		 */
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

	/* Need to lock map->head */
	PMAP_MAP_TO_HEAD_LOCK();
	
	ptes = pmap_map_ptes(pmap);

	/*
	 * OK, at this point, we know we're doing write-protect operation.
	 * If the pmap is active, write-back the range.
	 */
	if (pmap_is_curpmap(pmap))
		cpu_dcache_wb_range(sva, eva - sva);

	/*
	 * We need to acquire a pointer to a page table page before entering
	 * the following loop.
	 */
	while (sva < eva) {
		if (pmap_pde_page(pmap_pde(pmap, sva)))
			break;
		sva = (sva & L1_S_FRAME) + L1_S_SIZE;
	}
	
	pte = &ptes[arm_btop(sva)];
	
	while (sva < eva) {
		/* only check once in a while */
		if ((sva & L2_ADDR_BITS) == 0) {
			if (!pmap_pde_page(pmap_pde(pmap, sva))) {
				/* We can race ahead here, to the next pde. */
				sva += L1_S_SIZE;
				pte += arm_btop(L1_S_SIZE);
				continue;
			}
		}

		if (!pmap_pte_v(pte)) {
			PTE_FLUSH_ALT(pmap, pte);
			goto next;
		}

		flush = TRUE;

		pg = PHYS_TO_VM_PAGE(pmap_pte_pa(pte));

		*pte &= ~L2_S_PROT_W;		/* clear write bit */
		PTE_SYNC_CURRENT(pmap, pte);	/* XXXJRT optimize */

		/* Clear write flag */
		if (pg != NULL) {
			simple_lock(&pg->mdpage.pvh_slock);
			(void) pmap_modify_pv(pmap, sva, pg, PVF_WRITE, 0);
			pmap_vac_me_harder(pmap, pg, ptes, FALSE);
			simple_unlock(&pg->mdpage.pvh_slock);
		}

 next:
		sva += PAGE_SIZE;
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
pmap_enter(struct pmap *pmap, vaddr_t va, paddr_t pa, vm_prot_t prot,
    int flags)
{
	pt_entry_t *ptes, opte, npte;
	paddr_t opa;
	boolean_t wired = (flags & PMAP_WIRED) != 0;
	struct vm_page *pg;
	struct pv_entry *pve;
	int error;
	unsigned int nflags;
	struct vm_page *ptp = NULL;

	NPDEBUG(PDB_ENTER, printf("pmap_enter: V%08lx P%08lx in pmap %p prot=%08x, flags=%08x, wired = %d\n",
	    va, pa, pmap, prot, flags, wired));

	KDASSERT((flags & PMAP_WIRED) == 0 || (flags & VM_PROT_ALL) != 0);
	
#ifdef DIAGNOSTIC
	/* Valid address ? */
	if (va >= (pmap_curmaxkvaddr))
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

	KDASSERT(((va | pa) & PGOFSET) == 0);

	/*
	 * Get a pointer to the page.  Later on in this function, we
	 * test for a managed page by checking pg != NULL.
	 */
	pg = pmap_initialized ? PHYS_TO_VM_PAGE(pa) : NULL;

	/* get lock */
	PMAP_MAP_TO_HEAD_LOCK();

	/*
	 * map the ptes.  If there's not already an L2 table for this
	 * address, allocate one.
	 */
	ptes = pmap_map_ptes(pmap);		/* locks pmap */
	/* kernel should be pre-grown */
	if (pmap != pmap_kernel())
	{
		/* if failure is allowed then don't try too hard */
		ptp = pmap_get_ptp(pmap, va & PD_FRAME);
		if (ptp == NULL) {
			if (flags & PMAP_CANFAIL) {
				error = ENOMEM;
				goto out;
			}
			panic("pmap_enter: get ptp failed");
		}
	}
	opte = ptes[arm_btop(va)];

	nflags = 0;
	if (prot & VM_PROT_WRITE)
		nflags |= PVF_WRITE;
	if (wired)
		nflags |= PVF_WIRED;

	/* Is the pte valid ? If so then this page is already mapped */
	if (l2pte_valid(opte)) {
		/* Get the physical address of the current page mapped */
		opa = l2pte_pa(opte);

		/* Are we mapping the same page ? */
		if (opa == pa) {
			/* Check to see if we're doing rw->ro. */
			if ((opte & L2_S_PROT_W) != 0 &&
			    (prot & VM_PROT_WRITE) == 0) {
				/* Yup, flush the cache if current pmap. */
				if (pmap_is_curpmap(pmap))
					cpu_dcache_wb_range(va, PAGE_SIZE);
			}

			/* Has the wiring changed ? */
			if (pg != NULL) {
				simple_lock(&pg->mdpage.pvh_slock);
				(void) pmap_modify_pv(pmap, va, pg,
				    PVF_WRITE | PVF_WIRED, nflags);
				simple_unlock(&pg->mdpage.pvh_slock);
 			}
		} else {
			struct vm_page *opg;

			/* We are replacing the page with a new one. */
			cpu_idcache_wbinv_range(va, PAGE_SIZE);

			/*
			 * If it is part of our managed memory then we
			 * must remove it from the PV list
			 */
			if ((opg = PHYS_TO_VM_PAGE(opa)) != NULL) {
				simple_lock(&opg->mdpage.pvh_slock);
				pve = pmap_remove_pv(opg, pmap, va);
				simple_unlock(&opg->mdpage.pvh_slock);
			} else {
				pve = NULL;
			}

			goto enter;
		}
	} else {
		opa = 0;
		pve = NULL;

		/* bump ptp ref */
		if (ptp != NULL)
			ptp->wire_count++;

		/* pte is not valid so we must be hooking in a new page */
		++pmap->pm_stats.resident_count;

	enter:
		/*
		 * Enter on the PV list if part of our managed memory
		 */
		if (pg != NULL) {
			if (pve == NULL) {
				pve = pmap_alloc_pv(pmap, ALLOCPV_NEED);
				if (pve == NULL) {
					if (flags & PMAP_CANFAIL) {
						PTE_FLUSH_ALT(pmap,
						    ptes[arm_btop(va)]);
						error = ENOMEM;
						goto out;
					}
					panic("pmap_enter: no pv entries "
					    "available");
				}
			}
			/* enter_pv locks pvh when adding */
			pmap_enter_pv(pg, pve, pmap, va, ptp, nflags);
		} else {
			if (pve != NULL)
				pmap_free_pv(pmap, pve);
		}
	}

	/* Construct the pte, giving the correct access. */
	npte = pa;

	/* VA 0 is magic. */
	if (pmap != pmap_kernel() && va != vector_page)
		npte |= L2_S_PROT_U;

	if (pg != NULL) {
#ifdef DIAGNOSTIC
		if ((flags & VM_PROT_ALL) & ~prot)
			panic("pmap_enter: access_type exceeds prot");
#endif
		npte |= pte_l2_s_cache_mode;
		if (flags & VM_PROT_WRITE) {
			npte |= L2_S_PROTO | L2_S_PROT_W;
			pg->mdpage.pvh_attrs |= PVF_REF | PVF_MOD;
		} else if (flags & VM_PROT_ALL) {
			npte |= L2_S_PROTO;
			pg->mdpage.pvh_attrs |= PVF_REF;
		} else
			npte |= L2_TYPE_INV;
	} else {
		if (prot & VM_PROT_WRITE)
			npte |= L2_S_PROTO | L2_S_PROT_W;
		else if (prot & VM_PROT_ALL)
			npte |= L2_S_PROTO;
		else
			npte |= L2_TYPE_INV;
	}

#if ARM_MMU_XSCALE == 1 && defined(XSCALE_CACHE_READ_WRITE_ALLOCATE)
#if ARM_NMMUS > 1
# error "XXX Unable to use read/write-allocate and configure non-XScale"
#endif
	/*
	 * XXX BRUTAL HACK!  This allows us to limp along with
	 * XXX the read/write-allocate cache mode.
	 */
	if (pmap == pmap_kernel())
		npte &= ~L2_XSCALE_T_TEX(TEX_XSCALE_X);
#endif
	ptes[arm_btop(va)] = npte;
	PTE_SYNC_CURRENT(pmap, &ptes[arm_btop(va)]);

	if (pg != NULL) {
		simple_lock(&pg->mdpage.pvh_slock);
 		pmap_vac_me_harder(pmap, pg, ptes, pmap_is_curpmap(pmap));
		simple_unlock(&pg->mdpage.pvh_slock);
	}

	/* Better flush the TLB ... */
	cpu_tlb_flushID_SE(va);
	error = 0;
out:
	pmap_unmap_ptes(pmap);			/* unlocks pmap */
	PMAP_MAP_TO_HEAD_UNLOCK();

	return error;
}

/*
 * pmap_kenter_pa: enter a kernel mapping
 *
 * => no need to lock anything assume va is already allocated
 * => should be faster than normal pmap enter function
 */
void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	pt_entry_t *pte;

	pte = vtopte(va);
	KASSERT(!pmap_pte_v(pte));

#ifdef PMAP_ALIAS_DEBUG
    {
	struct vm_page *pg;
	int s;

	pg = PHYS_TO_VM_PAGE(pa);
	if (pg != NULL) {
		s = splhigh();
		if (pg->mdpage.ro_mappings == 0 &&
		    pg->mdpage.rw_mappings == 0 &&
		    pg->mdpage.kro_mappings == 0 &&
		    pg->mdpage.krw_mappings == 0) {
			/* This case is okay. */
		} else if (pg->mdpage.rw_mappings == 0 &&
			   pg->mdpage.krw_mappings == 0 &&
			   (prot & VM_PROT_WRITE) == 0) {
			/* This case is okay. */
		} else {
			/* Something is awry. */
			printf("pmap_kenter_pa: ro %u, rw %u, kro %u, krw %u "
			    "prot 0x%x\n", pg->mdpage.ro_mappings,
			    pg->mdpage.rw_mappings, pg->mdpage.kro_mappings,
			    pg->mdpage.krw_mappings, prot);
			Debugger();
		}
		if (prot & VM_PROT_WRITE)
			pg->mdpage.krw_mappings++;
		else
			pg->mdpage.kro_mappings++;
		splx(s);
	}
    }
#endif /* PMAP_ALIAS_DEBUG */

	*pte = L2_S_PROTO | pa |
	    L2_S_PROT(PTE_KERNEL, prot) | pte_l2_s_cache_mode;
	PTE_SYNC(pte);
}

void
pmap_kremove(vaddr_t va, vsize_t len)
{
	pt_entry_t *pte;
	vaddr_t ova = va;
	vaddr_t olen = len;

	for (len >>= PAGE_SHIFT; len > 0; len--, va += PAGE_SIZE) {

		/*
		 * We assume that we will only be called with small
		 * regions of memory.
		 */

		KASSERT(pmap_pde_page(pmap_pde(pmap_kernel(), va)));
		pte = vtopte(va);
#ifdef PMAP_ALIAS_DEBUG
    {
		struct vm_page *pg;
		int s;

		if ((*pte & L2_TYPE_MASK) != L2_TYPE_INV &&
		    (pg = PHYS_TO_VM_PAGE(*pte & L2_S_FRAME)) != NULL) {
			s = splhigh();
			if (*pte & L2_S_PROT_W) {
				KASSERT(pg->mdpage.krw_mappings != 0);
				pg->mdpage.krw_mappings--;
			} else {
				KASSERT(pg->mdpage.kro_mappings != 0);
				pg->mdpage.kro_mappings--;
			}
			splx(s);
		}
    }
#endif /* PMAP_ALIAS_DEBUG */
		cpu_idcache_wbinv_range(va, PAGE_SIZE);
		*pte = 0;
		cpu_tlb_flushID_SE(va);
	}
	PTE_SYNC_RANGE(vtopte(ova), olen >> PAGE_SHIFT);
}

/*
 * pmap_page_protect:
 *
 * Lower the permission for all mappings to a given page.
 */

void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{

	PDEBUG(0, printf("pmap_page_protect(pa=%lx, prot=%d)\n",
	    VM_PAGE_TO_PHYS(pg), prot));

	switch(prot) {
	case VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE:
	case VM_PROT_READ|VM_PROT_WRITE:
		return;

	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		pmap_clearbit(pg, PVF_WRITE);
		break;

	default:
		pmap_page_remove(pg);
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
pmap_unwire(struct pmap *pmap, vaddr_t va)
{
	pt_entry_t *ptes;
	struct vm_page *pg;
	paddr_t pa;

	PMAP_MAP_TO_HEAD_LOCK();
	ptes = pmap_map_ptes(pmap);		/* locks pmap */

	if (pmap_pde_v(pmap_pde(pmap, va))) {
#ifdef DIAGNOSTIC
		if (l2pte_valid(ptes[arm_btop(va)]) == 0)
			panic("pmap_unwire: invalid L2 PTE");
#endif
		/* Extract the physical address of the page */
		pa = l2pte_pa(ptes[arm_btop(va)]);
		PTE_FLUSH_ALT(pmap, &ptes[arm_btop(va)]);

		if ((pg = PHYS_TO_VM_PAGE(pa)) == NULL)
			goto out;

		/* Update the wired bit in the pv entry for this page. */
		simple_lock(&pg->mdpage.pvh_slock);
		(void) pmap_modify_pv(pmap, va, pg, PVF_WIRED, 0);
		simple_unlock(&pg->mdpage.pvh_slock);
	}
#ifdef DIAGNOSTIC
	else {
		panic("pmap_unwire: invalid L1 PTE");
	}
#endif
 out:
	pmap_unmap_ptes(pmap);			/* unlocks pmap */
	PMAP_MAP_TO_HEAD_UNLOCK();
}

/*
 * Routine:  pmap_extract
 * Function:
 *           Extract the physical page address associated
 *           with the given map/virtual_address pair.
 */
boolean_t
pmap_extract(struct pmap *pmap, vaddr_t va, paddr_t *pap)
{
	pd_entry_t *pde;
	pt_entry_t *pte, *ptes;
	paddr_t pa;

	PDEBUG(5, printf("pmap_extract: pmap=%p, va=0x%08lx -> ", pmap, va));

	ptes = pmap_map_ptes(pmap);		/* locks pmap */

	pde = pmap_pde(pmap, va);
	pte = &ptes[arm_btop(va)]; 

	if (pmap_pde_section(pde)) {
		pa = (*pde & L1_S_FRAME) | (va & L1_S_OFFSET);
		PDEBUG(5, printf("section pa=0x%08lx\n", pa));
		goto out;
	} else if (pmap_pde_page(pde) == 0 || pmap_pte_v(pte) == 0) {
		PDEBUG(5, printf("no mapping\n"));
		goto failed;
	}

	if ((*pte & L2_TYPE_MASK) == L2_TYPE_L) {
		pa = (*pte & L2_L_FRAME) | (va & L2_L_OFFSET);
		PDEBUG(5, printf("large page pa=0x%08lx\n", pa));
		goto out;
	}

	pa = (*pte & L2_S_FRAME) | (va & L2_S_OFFSET);
	PDEBUG(5, printf("small page pa=0x%08lx\n", pa));

 out:
	if (pap != NULL)
		*pap = pa;

	PTE_FLUSH_ALT(pmap, &ptes[arm_btop(va)]);
	pmap_unmap_ptes(pmap);			/* unlocks pmap */
	return (TRUE);

 failed:
	PTE_FLUSH_ALT(pmap, &ptes[arm_btop(va)]);
	pmap_unmap_ptes(pmap);			/* unlocks pmap */
	return (FALSE);
}


/*
 * pmap_copy:
 *
 *	Copy the range specified by src_addr/len from the source map to the
 *	range dst_addr/len in the destination map.
 *
 *	This routine is only advisory and need not do anything.
 */
/* Call deleted in <arm/arm32/pmap.h> */

#if defined(PMAP_DEBUG)
void
pmap_dump_pvlist(phys, m)
	vaddr_t phys;
	char *m;
{
	struct vm_page *pg;
	struct pv_entry *pv;

	if ((pg = PHYS_TO_VM_PAGE(phys)) == NULL) {
		printf("INVALID PA\n");
		return;
	}
	simple_lock(&pg->mdpage.pvh_slock);
	printf("%s %08lx:", m, phys);
	if (pg->mdpage.pvh_list == NULL) {
		simple_unlock(&pg->mdpage.pvh_slock);
		printf(" no mappings\n");
		return;
	}

	for (pv = pg->mdpage.pvh_list; pv; pv = pv->pv_next)
		printf(" pmap %p va %08lx flags %08x", pv->pv_pmap,
		    pv->pv_va, pv->pv_flags);

	printf("\n");
	simple_unlock(&pg->mdpage.pvh_slock);
}

#endif	/* PMAP_DEBUG */

static pt_entry_t *
pmap_map_ptes(struct pmap *pmap)
{
	struct proc *p;

    	/* the kernel's pmap is always accessible */
	if (pmap == pmap_kernel()) {
		return (pt_entry_t *)PTE_BASE;
	}
	
	if (pmap_is_curpmap(pmap)) {
		simple_lock(&pmap->pm_obj.vmobjlock);
		return (pt_entry_t *)PTE_BASE;
	}

	p = curproc;
	KDASSERT(p != NULL);

	/* need to lock both curpmap and pmap: use ordered locking */
	if ((vaddr_t) pmap < (vaddr_t) p->p_vmspace->vm_map.pmap) {
		simple_lock(&pmap->pm_obj.vmobjlock);
		simple_lock(&p->p_vmspace->vm_map.pmap->pm_obj.vmobjlock);
	} else {
		simple_lock(&p->p_vmspace->vm_map.pmap->pm_obj.vmobjlock);
		simple_lock(&pmap->pm_obj.vmobjlock);
	}
    
	pmap_map_in_l1(p->p_vmspace->vm_map.pmap, APTE_BASE,
	    pmap->pm_pptpt, 0);
	cpu_tlb_flushD();
	cpu_cpwait();
	return (pt_entry_t *)APTE_BASE;
}

/*
 * pmap_unmap_ptes: unlock the PTE mapping of "pmap"
 */

static void
pmap_unmap_ptes(struct pmap *pmap)
{

	if (pmap == pmap_kernel()) {
		return;
	}
	if (pmap_is_curpmap(pmap)) {
		simple_unlock(&pmap->pm_obj.vmobjlock);
	} else {
		KDASSERT(curproc != NULL);
		simple_unlock(&pmap->pm_obj.vmobjlock);
		simple_unlock(
		    &curproc->p_vmspace->vm_map.pmap->pm_obj.vmobjlock);
	}
}

/*
 * Modify pte bits for all ptes corresponding to the given physical address.
 * We use `maskbits' rather than `clearbits' because we're always passing
 * constants and the latter would require an extra inversion at run-time.
 */

static void
pmap_clearbit(struct vm_page *pg, u_int maskbits)
{
	struct pv_entry *pv;
	pt_entry_t *ptes, npte, opte;
	vaddr_t va;

	PDEBUG(1, printf("pmap_clearbit: pa=%08lx mask=%08x\n",
	    VM_PAGE_TO_PHYS(pg), maskbits));

	PMAP_HEAD_TO_MAP_LOCK();
	simple_lock(&pg->mdpage.pvh_slock);
	
	/*
	 * Clear saved attributes (modify, reference)
	 */
	pg->mdpage.pvh_attrs &= ~maskbits;

	if (pg->mdpage.pvh_list == NULL) {
		simple_unlock(&pg->mdpage.pvh_slock);
		PMAP_HEAD_TO_MAP_UNLOCK();
		return;
	}

	/*
	 * Loop over all current mappings setting/clearing as appropos
	 */
	for (pv = pg->mdpage.pvh_list; pv; pv = pv->pv_next) {
#ifdef PMAP_ALIAS_DEBUG
    {
		int s = splhigh();
		if ((maskbits & PVF_WRITE) != 0 &&
		    (pv->pv_flags & PVF_WRITE) != 0) {
			KASSERT(pg->mdpage.rw_mappings != 0);
			pg->mdpage.rw_mappings--;
			pg->mdpage.ro_mappings++;
		}
		splx(s);
    }
#endif /* PMAP_ALIAS_DEBUG */
		va = pv->pv_va;
		pv->pv_flags &= ~maskbits;
		ptes = pmap_map_ptes(pv->pv_pmap);	/* locks pmap */
		KASSERT(pmap_pde_v(pmap_pde(pv->pv_pmap, va)));
		npte = opte = ptes[arm_btop(va)];
		if (maskbits & (PVF_WRITE|PVF_MOD)) {
			if ((pv->pv_flags & PVF_NC)) {
				/* 
				 * Entry is not cacheable: reenable
				 * the cache, nothing to flush
				 *
				 * Don't turn caching on again if this
				 * is a modified emulation.  This
				 * would be inconsitent with the
				 * settings created by
				 * pmap_vac_me_harder(). 
				 *
				 * There's no need to call
				 * pmap_vac_me_harder() here: all
				 * pages are loosing their write
				 * permission.
				 *
				 */
				if (maskbits & PVF_WRITE) {
					npte |= pte_l2_s_cache_mode;
					pv->pv_flags &= ~PVF_NC;
				}
			} else if (pmap_is_curpmap(pv->pv_pmap)) {
				/* 
				 * Entry is cacheable: check if pmap is
				 * current if it is flush it,
				 * otherwise it won't be in the cache
				 */
				cpu_idcache_wbinv_range(pv->pv_va, PAGE_SIZE);
			}

			/* make the pte read only */
			npte &= ~L2_S_PROT_W;
		}

		if (maskbits & PVF_REF) {
			if (pmap_is_curpmap(pv->pv_pmap) &&
			    (pv->pv_flags & PVF_NC) == 0) {
				/*
				 * Check npte here; we may have already
				 * done the wbinv above, and the validity
				 * of the PTE is the same for opte and
				 * npte.
				 */
				if (npte & L2_S_PROT_W) {
					cpu_idcache_wbinv_range(pv->pv_va,
					    PAGE_SIZE);
				} else if ((npte & L2_TYPE_MASK)
					   != L2_TYPE_INV) {
					/* XXXJRT need idcache_inv_range */
					cpu_idcache_wbinv_range(pv->pv_va,
					    PAGE_SIZE);
				}
			}

			/* make the pte invalid */
			npte = (npte & ~L2_TYPE_MASK) | L2_TYPE_INV;
		}

		if (npte != opte) {
			ptes[arm_btop(va)] = npte;
			PTE_SYNC_CURRENT(pv->pv_pmap, &ptes[arm_btop(va)]);
			/* Flush the TLB entry if a current pmap. */
			if (pmap_is_curpmap(pv->pv_pmap))
				cpu_tlb_flushID_SE(pv->pv_va);
		} else
			PTE_FLUSH_ALT(pv->pv_pmap, &ptes[arm_btop(va)]);

		pmap_unmap_ptes(pv->pv_pmap);		/* unlocks pmap */
	}
	cpu_cpwait();

	simple_unlock(&pg->mdpage.pvh_slock);
	PMAP_HEAD_TO_MAP_UNLOCK();
}

/*
 * pmap_clear_modify:
 *
 *	Clear the "modified" attribute for a page.
 */
boolean_t
pmap_clear_modify(struct vm_page *pg)
{
	boolean_t rv;

	if (pg->mdpage.pvh_attrs & PVF_MOD) {
		rv = TRUE;
		pmap_clearbit(pg, PVF_MOD);
	} else
		rv = FALSE;

	PDEBUG(0, printf("pmap_clear_modify pa=%08lx -> %d\n",
	    VM_PAGE_TO_PHYS(pg), rv));

	return (rv);
}

/*
 * pmap_clear_reference:
 *
 *	Clear the "referenced" attribute for a page.
 */
boolean_t
pmap_clear_reference(struct vm_page *pg)
{
	boolean_t rv;

	if (pg->mdpage.pvh_attrs & PVF_REF) {
		rv = TRUE;
		pmap_clearbit(pg, PVF_REF);
	} else
		rv = FALSE;

	PDEBUG(0, printf("pmap_clear_reference pa=%08lx -> %d\n",
	    VM_PAGE_TO_PHYS(pg), rv));

	return (rv);
}

/*
 * pmap_is_modified:
 *
 *	Test if a page has the "modified" attribute.
 */
/* See <arm/arm32/pmap.h> */

/*
 * pmap_is_referenced:
 *
 *	Test if a page has the "referenced" attribute.
 */
/* See <arm/arm32/pmap.h> */

int
pmap_modified_emulation(struct pmap *pmap, vaddr_t va)
{
	pt_entry_t *ptes;
	struct vm_page *pg;
	paddr_t pa;
	u_int flags;
	int rv = 0;

	PDEBUG(2, printf("pmap_modified_emulation\n"));

	PMAP_MAP_TO_HEAD_LOCK();
	ptes = pmap_map_ptes(pmap);		/* locks pmap */

	if (pmap_pde_v(pmap_pde(pmap, va)) == 0) {
		PDEBUG(2, printf("L1 PTE invalid\n"));
		goto out;
	}

	PDEBUG(1, printf("pte=%08x\n", ptes[arm_btop(va)]));

	/*
	 * Don't need to PTE_FLUSH_ALT() here; this is always done
	 * with the current pmap.
	 */

	/* Check for a invalid pte */
	if (l2pte_valid(ptes[arm_btop(va)]) == 0)
		goto out;

	/* This can happen if user code tries to access kernel memory. */
	if ((ptes[arm_btop(va)] & L2_S_PROT_W) != 0)
		goto out;

	/* Extract the physical address of the page */
	pa = l2pte_pa(ptes[arm_btop(va)]);
	if ((pg = PHYS_TO_VM_PAGE(pa)) == NULL)
		goto out;

	/* Get the current flags for this page. */
	simple_lock(&pg->mdpage.pvh_slock);
	
	flags = pmap_modify_pv(pmap, va, pg, 0, 0);
	PDEBUG(2, printf("pmap_modified_emulation: flags = %08x\n", flags));

	/*
	 * Do the flags say this page is writable ? If not then it is a
	 * genuine write fault. If yes then the write fault is our fault
	 * as we did not reflect the write access in the PTE. Now we know
	 * a write has occurred we can correct this and also set the
	 * modified bit
	 */
	if (~flags & PVF_WRITE) {
	    	simple_unlock(&pg->mdpage.pvh_slock);
		goto out;
	}

	PDEBUG(0,
	    printf("pmap_modified_emulation: Got a hit va=%08lx, pte = %08x\n",
	    va, ptes[arm_btop(va)]));
	pg->mdpage.pvh_attrs |= PVF_REF | PVF_MOD;

	/* 
	 * Re-enable write permissions for the page.  No need to call
	 * pmap_vac_me_harder(), since this is just a
	 * modified-emulation fault, and the PVF_WRITE bit isn't changing.
	 * We've already set the cacheable bits based on the assumption
	 * that we can write to this page.
	 */
	ptes[arm_btop(va)] =
	    (ptes[arm_btop(va)] & ~L2_TYPE_MASK) | L2_S_PROTO | L2_S_PROT_W;
	PTE_SYNC(&ptes[arm_btop(va)]);
	PDEBUG(0, printf("->(%08x)\n", ptes[arm_btop(va)]));

	simple_unlock(&pg->mdpage.pvh_slock);

	cpu_tlb_flushID_SE(va);
	cpu_cpwait();
	rv = 1;
 out:
	pmap_unmap_ptes(pmap);			/* unlocks pmap */
	PMAP_MAP_TO_HEAD_UNLOCK();
	return (rv);
}

int
pmap_handled_emulation(struct pmap *pmap, vaddr_t va)
{
	pt_entry_t *ptes;
	struct vm_page *pg;
	paddr_t pa;
	int rv = 0;

	PDEBUG(2, printf("pmap_handled_emulation\n"));

	PMAP_MAP_TO_HEAD_LOCK();
	ptes = pmap_map_ptes(pmap);		/* locks pmap */

	if (pmap_pde_v(pmap_pde(pmap, va)) == 0) {
		PDEBUG(2, printf("L1 PTE invalid\n"));
		goto out;
	}

	PDEBUG(1, printf("pte=%08x\n", ptes[arm_btop(va)]));

	/*
	 * Don't need to PTE_FLUSH_ALT() here; this is always done
	 * with the current pmap.
	 */

	/* Check for invalid pte */
	if (l2pte_valid(ptes[arm_btop(va)]) == 0)
		goto out;

	/* This can happen if user code tries to access kernel memory. */
	if ((ptes[arm_btop(va)] & L2_TYPE_MASK) != L2_TYPE_INV)
		goto out;

	/* Extract the physical address of the page */
	pa = l2pte_pa(ptes[arm_btop(va)]);
	if ((pg = PHYS_TO_VM_PAGE(pa)) == NULL)
		goto out;

	simple_lock(&pg->mdpage.pvh_slock);

	/*
	 * Ok we just enable the pte and mark the attibs as handled
	 * XXX Should we traverse the PV list and enable all PTEs?
	 */
	PDEBUG(0,
	    printf("pmap_handled_emulation: Got a hit va=%08lx pte = %08x\n",
	    va, ptes[arm_btop(va)]));
	pg->mdpage.pvh_attrs |= PVF_REF;

	ptes[arm_btop(va)] = (ptes[arm_btop(va)] & ~L2_TYPE_MASK) | L2_S_PROTO;
	PTE_SYNC(&ptes[arm_btop(va)]);
	PDEBUG(0, printf("->(%08x)\n", ptes[arm_btop(va)]));

	simple_unlock(&pg->mdpage.pvh_slock);

	cpu_tlb_flushID_SE(va);
	cpu_cpwait();
	rv = 1;
 out:
	pmap_unmap_ptes(pmap);			/* unlocks pmap */
	PMAP_MAP_TO_HEAD_UNLOCK();
	return (rv);
}

/*
 * pmap_collect: free resources held by a pmap
 *
 * => optional function.
 * => called when a process is swapped out to free memory.
 */

void
pmap_collect(struct pmap *pmap)
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
pmap_procwr(struct proc *p, vaddr_t va, int len)
{
	/* We only need to do anything if it is the current process. */
	if (p == curproc)
		cpu_icache_sync_range(va, len);
}
/*
 * PTP functions
 */
  
/*
 * pmap_get_ptp: get a PTP (if there isn't one, allocate a new one)
 *
 * => pmap should NOT be pmap_kernel()
 * => pmap should be locked
 */

static struct vm_page *
pmap_get_ptp(struct pmap *pmap, vaddr_t va)
{
	struct vm_page *ptp;
	pd_entry_t	*pde;

	KASSERT((va & PD_OFFSET) == 0);		/* XXX KDASSERT */

	pde = pmap_pde(pmap, va);
	if (pmap_pde_v(pde)) {
		/* valid... check hint (saves us a PA->PG lookup) */
		if (pmap->pm_ptphint &&
		    ((*pde) & L2_S_FRAME) ==
		    VM_PAGE_TO_PHYS(pmap->pm_ptphint))
			return (pmap->pm_ptphint);
		ptp = uvm_pagelookup(&pmap->pm_obj, va);
#ifdef DIAGNOSTIC
		if (ptp == NULL)
			panic("pmap_get_ptp: unmanaged user PTP");
#endif
		pmap->pm_ptphint = ptp;
		return(ptp);
	}

	/* allocate a new PTP (updates ptphint) */
	return (pmap_alloc_ptp(pmap, va));
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
pmap_alloc_ptp(struct pmap *pmap, vaddr_t va)
{
	struct vm_page *ptp;

	KASSERT((va & PD_OFFSET) == 0);		/* XXX KDASSERT */

	ptp = uvm_pagealloc(&pmap->pm_obj, va, NULL,
		UVM_PGA_USERESERVE|UVM_PGA_ZERO);
	if (ptp == NULL)
		return (NULL);

	/* got one! */
	ptp->flags &= ~PG_BUSY;	/* never busy */
	ptp->wire_count = 1;	/* no mappings yet */
	pmap_map_in_l1(pmap, va, VM_PAGE_TO_PHYS(ptp),
	    PMAP_PTP_SELFREF | PMAP_PTP_CACHEABLE);
	pmap->pm_stats.resident_count++;	/* count PTP as resident */
	pmap->pm_ptphint = ptp;
	return (ptp);
}

vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	struct pmap *kpm = pmap_kernel(), *pm;
	int s;
	paddr_t ptaddr;
	struct vm_page *ptp;

	if (maxkvaddr <= pmap_curmaxkvaddr)
		goto out;		/* we are OK */
	NPDEBUG(PDB_GROWKERN, printf("pmap_growkernel: growing kernel from %lx to %lx\n",
		    pmap_curmaxkvaddr, maxkvaddr));

	/*
	 * whoops!   we need to add kernel PTPs
	 */

	s = splhigh();	/* to be safe */
	simple_lock(&kpm->pm_obj.vmobjlock);
	/* due to the way the arm pmap works we map 4MB at a time */
	for (/*null*/ ; pmap_curmaxkvaddr < maxkvaddr;
	     pmap_curmaxkvaddr += 4 * L1_S_SIZE) {

		if (uvm.page_init_done == FALSE) {

			/*
			 * we're growing the kernel pmap early (from
			 * uvm_pageboot_alloc()).  this case must be
			 * handled a little differently.
			 */

			if (uvm_page_physget(&ptaddr) == FALSE)
				panic("pmap_growkernel: out of memory");
			pmap_zero_page(ptaddr);

			/* map this page in */
			pmap_map_in_l1(kpm, pmap_curmaxkvaddr, ptaddr,
			    PMAP_PTP_SELFREF | PMAP_PTP_CACHEABLE);

			/* count PTP as resident */
			kpm->pm_stats.resident_count++;
			continue;
		}

		/*
		 * THIS *MUST* BE CODED SO AS TO WORK IN THE
		 * pmap_initialized == FALSE CASE!  WE MAY BE
		 * INVOKED WHILE pmap_init() IS RUNNING!
		 */

		if ((ptp = pmap_alloc_ptp(kpm, pmap_curmaxkvaddr)) == NULL)
			panic("pmap_growkernel: alloc ptp failed");

		/* distribute new kernel PTP to all active pmaps */
		simple_lock(&pmaps_lock);
		LIST_FOREACH(pm, &pmaps, pm_list) {
			pmap_map_in_l1(pm, pmap_curmaxkvaddr,
			    VM_PAGE_TO_PHYS(ptp),
			    PMAP_PTP_SELFREF | PMAP_PTP_CACHEABLE); 
		}

		/* Invalidate the PTPT cache. */
		pool_cache_invalidate(&pmap_ptpt_cache);
		pmap_ptpt_cache_generation++;

		simple_unlock(&pmaps_lock);
	}

	/* 
	 * flush out the cache, expensive but growkernel will happen so
	 * rarely
	 */
	cpu_tlb_flushD();
	cpu_cpwait();

	simple_unlock(&kpm->pm_obj.vmobjlock);
	splx(s);

out:
	return (pmap_curmaxkvaddr);
}

/************************ Utility routines ****************************/

/*
 * vector_page_setprot:
 *
 *	Manipulate the protection of the vector page.
 */
void
vector_page_setprot(int prot)
{
	pt_entry_t *pte;

	pte = vtopte(vector_page);

	*pte = (*pte & ~L1_S_PROT_MASK) | L2_S_PROT(PTE_KERNEL, prot);
	PTE_SYNC(pte);
	cpu_tlb_flushD_SE(vector_page);
	cpu_cpwait();
}

/************************ Bootstrapping routines ****************************/

/*
 * This list exists for the benefit of pmap_map_chunk().  It keeps track
 * of the kernel L2 tables during bootstrap, so that pmap_map_chunk() can
 * find them as necessary.
 *
 * Note that the data on this list is not valid after initarm() returns.
 */
SLIST_HEAD(, pv_addr) kernel_pt_list = SLIST_HEAD_INITIALIZER(kernel_pt_list);

static vaddr_t
kernel_pt_lookup(paddr_t pa)
{
	pv_addr_t *pv;

	SLIST_FOREACH(pv, &kernel_pt_list, pv_list) {
		if (pv->pv_pa == pa)
			return (pv->pv_va);
	}
	return (0);
}

/*
 * pmap_map_section:
 *
 *	Create a single section mapping.
 */
void
pmap_map_section(vaddr_t l1pt, vaddr_t va, paddr_t pa, int prot, int cache)
{
	pd_entry_t *pde = (pd_entry_t *) l1pt;
	pd_entry_t fl = (cache == PTE_CACHE) ? pte_l1_s_cache_mode : 0;

	KASSERT(((va | pa) & L1_S_OFFSET) == 0);

	pde[va >> L1_S_SHIFT] = L1_S_PROTO | pa |
	    L1_S_PROT(PTE_KERNEL, prot) | fl;
}

/*
 * pmap_map_entry:
 *
 *	Create a single page mapping.
 */
void
pmap_map_entry(vaddr_t l1pt, vaddr_t va, paddr_t pa, int prot, int cache)
{
	pd_entry_t *pde = (pd_entry_t *) l1pt;
	pt_entry_t fl = (cache == PTE_CACHE) ? pte_l2_s_cache_mode : 0;
	pt_entry_t *pte;

	KASSERT(((va | pa) & PGOFSET) == 0);

	if ((pde[va >> L1_S_SHIFT] & L1_TYPE_MASK) != L1_TYPE_C)
		panic("pmap_map_entry: no L2 table for VA 0x%08lx", va);

	pte = (pt_entry_t *)
	    kernel_pt_lookup(pde[va >> L1_S_SHIFT] & L2_S_FRAME);
	if (pte == NULL)
		panic("pmap_map_entry: can't find L2 table for VA 0x%08lx", va);

	pte[(va >> PGSHIFT) & 0x3ff] = L2_S_PROTO | pa |
	    L2_S_PROT(PTE_KERNEL, prot) | fl;
}

/*
 * pmap_link_l2pt:
 *
 *	Link the L2 page table specified by "pa" into the L1
 *	page table at the slot for "va".
 */
void
pmap_link_l2pt(vaddr_t l1pt, vaddr_t va, pv_addr_t *l2pv)
{
	pd_entry_t *pde = (pd_entry_t *) l1pt;
	u_int slot = va >> L1_S_SHIFT;

	KASSERT((l2pv->pv_pa & PGOFSET) == 0);

	pde[slot + 0] = L1_C_PROTO | (l2pv->pv_pa + 0x000);
	pde[slot + 1] = L1_C_PROTO | (l2pv->pv_pa + 0x400);
	pde[slot + 2] = L1_C_PROTO | (l2pv->pv_pa + 0x800);
	pde[slot + 3] = L1_C_PROTO | (l2pv->pv_pa + 0xc00);

	SLIST_INSERT_HEAD(&kernel_pt_list, l2pv, pv_list);
}

/*
 * pmap_map_chunk:
 *
 *	Map a chunk of memory using the most efficient mappings
 *	possible (section, large page, small page) into the
 *	provided L1 and L2 tables at the specified virtual address.
 */
vsize_t
pmap_map_chunk(vaddr_t l1pt, vaddr_t va, paddr_t pa, vsize_t size,
    int prot, int cache)
{
	pd_entry_t *pde = (pd_entry_t *) l1pt;
	pt_entry_t *pte, fl;
	vsize_t resid;  
	u_int i;

	resid = (size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);

	if (l1pt == 0)
		panic("pmap_map_chunk: no L1 table provided");

#ifdef VERBOSE_INIT_ARM     
	printf("pmap_map_chunk: pa=0x%lx va=0x%lx size=0x%lx resid=0x%lx "
	    "prot=0x%x cache=%d\n", pa, va, size, resid, prot, cache);
#endif

	size = resid;

	while (resid > 0) {
		/* See if we can use a section mapping. */
		if (((pa | va) & L1_S_OFFSET) == 0 &&
		    resid >= L1_S_SIZE) {
			fl = (cache == PTE_CACHE) ? pte_l1_s_cache_mode : 0;
#ifdef VERBOSE_INIT_ARM
			printf("S");
#endif
			pde[va >> L1_S_SHIFT] = L1_S_PROTO | pa |
			    L1_S_PROT(PTE_KERNEL, prot) | fl;
			va += L1_S_SIZE;
			pa += L1_S_SIZE;
			resid -= L1_S_SIZE;
			continue;
		}

		/*
		 * Ok, we're going to use an L2 table.  Make sure
		 * one is actually in the corresponding L1 slot
		 * for the current VA.
		 */
		if ((pde[va >> L1_S_SHIFT] & L1_TYPE_MASK) != L1_TYPE_C)
			panic("pmap_map_chunk: no L2 table for VA 0x%08lx", va);

		pte = (pt_entry_t *)
		    kernel_pt_lookup(pde[va >> L1_S_SHIFT] & L2_S_FRAME);
		if (pte == NULL)
			panic("pmap_map_chunk: can't find L2 table for VA"
			    "0x%08lx", va);

		/* See if we can use a L2 large page mapping. */
		if (((pa | va) & L2_L_OFFSET) == 0 &&
		    resid >= L2_L_SIZE) {
			fl = (cache == PTE_CACHE) ? pte_l2_l_cache_mode : 0;
#ifdef VERBOSE_INIT_ARM
			printf("L");
#endif
			for (i = 0; i < 16; i++) {
				pte[((va >> PGSHIFT) & 0x3f0) + i] =
				    L2_L_PROTO | pa |
				    L2_L_PROT(PTE_KERNEL, prot) | fl;
			}
			va += L2_L_SIZE;
			pa += L2_L_SIZE;
			resid -= L2_L_SIZE;
			continue;
		}

		/* Use a small page mapping. */
		fl = (cache == PTE_CACHE) ? pte_l2_s_cache_mode : 0;
#ifdef VERBOSE_INIT_ARM
		printf("P");
#endif
		pte[(va >> PGSHIFT) & 0x3ff] = L2_S_PROTO | pa |
		    L2_S_PROT(PTE_KERNEL, prot) | fl;
		va += PAGE_SIZE;
		pa += PAGE_SIZE;
		resid -= PAGE_SIZE;
	}
#ifdef VERBOSE_INIT_ARM
	printf("\n");
#endif
	return (size);
}

/********************** PTE initialization routines **************************/

/*
 * These routines are called when the CPU type is identified to set up
 * the PTE prototypes, cache modes, etc.
 *
 * The variables are always here, just in case LKMs need to reference
 * them (though, they shouldn't).
 */

pt_entry_t	pte_l1_s_cache_mode;
pt_entry_t	pte_l1_s_cache_mask;

pt_entry_t	pte_l2_l_cache_mode;
pt_entry_t	pte_l2_l_cache_mask;

pt_entry_t	pte_l2_s_cache_mode;
pt_entry_t	pte_l2_s_cache_mask;

pt_entry_t	pte_l2_s_prot_u;
pt_entry_t	pte_l2_s_prot_w;
pt_entry_t	pte_l2_s_prot_mask;

pt_entry_t	pte_l1_s_proto;
pt_entry_t	pte_l1_c_proto;
pt_entry_t	pte_l2_s_proto;

void		(*pmap_copy_page_func)(paddr_t, paddr_t);
void		(*pmap_zero_page_func)(paddr_t);

#if ARM_MMU_GENERIC == 1
void
pmap_pte_init_generic(void)
{

	pte_l1_s_cache_mode = L1_S_B|L1_S_C;
	pte_l1_s_cache_mask = L1_S_CACHE_MASK_generic;

	pte_l2_l_cache_mode = L2_B|L2_C;
	pte_l2_l_cache_mask = L2_L_CACHE_MASK_generic;

	pte_l2_s_cache_mode = L2_B|L2_C;
	pte_l2_s_cache_mask = L2_S_CACHE_MASK_generic;

	pte_l2_s_prot_u = L2_S_PROT_U_generic;
	pte_l2_s_prot_w = L2_S_PROT_W_generic;
	pte_l2_s_prot_mask = L2_S_PROT_MASK_generic;

	pte_l1_s_proto = L1_S_PROTO_generic;
	pte_l1_c_proto = L1_C_PROTO_generic;
	pte_l2_s_proto = L2_S_PROTO_generic;

	pmap_copy_page_func = pmap_copy_page_generic;
	pmap_zero_page_func = pmap_zero_page_generic;
}

#if defined(CPU_ARM9)
void
pmap_pte_init_arm9(void)
{

	/*
	 * ARM9 is compatible with generic, but we want to use
	 * write-through caching for now.
	 */
	pmap_pte_init_generic();

	pte_l1_s_cache_mode = L1_S_C;
	pte_l2_l_cache_mode = L2_C;
	pte_l2_s_cache_mode = L2_C;
}
#endif /* CPU_ARM9 */
#endif /* ARM_MMU_GENERIC == 1 */

#if ARM_MMU_XSCALE == 1
void
pmap_pte_init_xscale(void)
{
	uint32_t auxctl;
	int	write_through = 0;

	pte_l1_s_cache_mode = L1_S_B|L1_S_C;
	pte_l1_s_cache_mask = L1_S_CACHE_MASK_xscale;

	pte_l2_l_cache_mode = L2_B|L2_C;
	pte_l2_l_cache_mask = L2_L_CACHE_MASK_xscale;

	pte_l2_s_cache_mode = L2_B|L2_C;
	pte_l2_s_cache_mask = L2_S_CACHE_MASK_xscale;

#ifdef XSCALE_CACHE_READ_WRITE_ALLOCATE
	/*
	 * The XScale core has an enhanced mode where writes that
	 * miss the cache cause a cache line to be allocated.  This
	 * is significantly faster than the traditional, write-through
	 * behavior of this case.
	 *
	 * However, there is a bug lurking in this pmap module, or in
	 * other parts of the VM system, or both, which causes corruption
	 * of NFS-backed files when this cache mode is used.  We have
	 * an ugly work-around for this problem (disable r/w-allocate
	 * for managed kernel mappings), but the bug is still evil enough
	 * to consider this cache mode "experimental".
	 */
	pte_l1_s_cache_mode |= L1_S_XSCALE_TEX(TEX_XSCALE_X);
	pte_l2_l_cache_mode |= L2_XSCALE_L_TEX(TEX_XSCALE_X);
	pte_l2_s_cache_mode |= L2_XSCALE_T_TEX(TEX_XSCALE_X);
#endif /* XSCALE_CACHE_READ_WRITE_ALLOCATE */

#ifdef XSCALE_CACHE_WRITE_THROUGH
	/*
	 * Some versions of the XScale core have various bugs in
	 * their cache units, the work-around for which is to run
	 * the cache in write-through mode.  Unfortunately, this
	 * has a major (negative) impact on performance.  So, we
	 * go ahead and run fast-and-loose, in the hopes that we
	 * don't line up the planets in a way that will trip the
	 * bugs.
	 *
	 * However, we give you the option to be slow-but-correct.
	 */
	write_through = 1;
#elif defined(XSCALE_CACHE_WRITE_BACK)
	/* force to use write back cache */
	write_through = 0;
#elif defined(CPU_XSCALE_PXA2X0)
	/*
	 * Intel PXA2[15]0 processors are known to have a bug in
	 * write-back cache on revision 4 and earlier (stepping
	 * A[01] and B[012]).  Fixed for C0 and later.
	 */
	{
		uint32_t id , type;

		id = cpufunc_id();
		type = id & ~(CPU_ID_XSCALE_COREREV_MASK|CPU_ID_REVISION_MASK);

		if (type == CPU_ID_PXA250 || type == CPU_ID_PXA210) {

			if ((id & CPU_ID_REVISION_MASK) < 5) {
				/* write through for stepping A0-1 and B0-2 */
				write_through = 1;
			}
		}
	}
#endif /* XSCALE_CACHE_WRITE_THROUGH */


	if (write_through) {
		pte_l1_s_cache_mode = L1_S_C;
		pte_l2_l_cache_mode = L2_C;
		pte_l2_s_cache_mode = L2_C;
	}

	pte_l2_s_prot_u = L2_S_PROT_U_xscale;
	pte_l2_s_prot_w = L2_S_PROT_W_xscale;
	pte_l2_s_prot_mask = L2_S_PROT_MASK_xscale;

	pte_l1_s_proto = L1_S_PROTO_xscale;
	pte_l1_c_proto = L1_C_PROTO_xscale;
	pte_l2_s_proto = L2_S_PROTO_xscale;

	pmap_copy_page_func = pmap_copy_page_xscale;
	pmap_zero_page_func = pmap_zero_page_xscale;

	/*
	 * Disable ECC protection of page table access, for now.
	 */
	__asm __volatile("mrc p15, 0, %0, c1, c0, 1"
		: "=r" (auxctl));
	auxctl &= ~XSCALE_AUXCTL_P;
	__asm __volatile("mcr p15, 0, %0, c1, c0, 1"
		:
		: "r" (auxctl));
}

/*
 * xscale_setup_minidata:
 *
 *	Set up the mini-data cache clean area.  We require the
 *	caller to allocate the right amount of physically and
 *	virtually contiguous space.
 */
void
xscale_setup_minidata(vaddr_t l1pt, vaddr_t va, paddr_t pa)
{
	extern vaddr_t xscale_minidata_clean_addr;
	extern vsize_t xscale_minidata_clean_size; /* already initialized */
	pd_entry_t *pde = (pd_entry_t *) l1pt;
	pt_entry_t *pte;
	vsize_t size;
	uint32_t auxctl;

	xscale_minidata_clean_addr = va;

	/* Round it to page size. */
	size = (xscale_minidata_clean_size + L2_S_OFFSET) & L2_S_FRAME;

	for (; size != 0;
	     va += L2_S_SIZE, pa += L2_S_SIZE, size -= L2_S_SIZE) {
		pte = (pt_entry_t *)
		    kernel_pt_lookup(pde[va >> L1_S_SHIFT] & L2_S_FRAME);
		if (pte == NULL)
			panic("xscale_setup_minidata: can't find L2 table for "
			    "VA 0x%08lx", va);
		pte[(va >> PGSHIFT) & 0x3ff] = L2_S_PROTO | pa |
		    L2_S_PROT(PTE_KERNEL, VM_PROT_READ) |
		    L2_C | L2_XSCALE_T_TEX(TEX_XSCALE_X);
	}

	/*
	 * Configure the mini-data cache for write-back with
	 * read/write-allocate.
	 *
	 * NOTE: In order to reconfigure the mini-data cache, we must
	 * make sure it contains no valid data!  In order to do that,
	 * we must issue a global data cache invalidate command!
	 *
	 * WE ASSUME WE ARE RUNNING UN-CACHED WHEN THIS ROUTINE IS CALLED!
	 * THIS IS VERY IMPORTANT!
	 */
	
	/* Invalidate data and mini-data. */
	__asm __volatile("mcr p15, 0, %0, c7, c6, 0"
		:
		: "r" (auxctl));


	__asm __volatile("mrc p15, 0, %0, c1, c0, 1"
		: "=r" (auxctl));
	auxctl = (auxctl & ~XSCALE_AUXCTL_MD_MASK) | XSCALE_AUXCTL_MD_WB_RWA;
	__asm __volatile("mcr p15, 0, %0, c1, c0, 1"
		:
		: "r" (auxctl));
}
#endif /* ARM_MMU_XSCALE == 1 */
