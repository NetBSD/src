/*
 * Copyright (c) 1993, 1994 Adam Glass
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
 *	This product includes software developed by Adam Glass.
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
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
 *	$Id: pmap.c,v 1.35 1994/07/29 04:04:31 gwr Exp $
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <machine/pte.h>
#include <machine/control.h>
    
#include <machine/cpu.h>
#include <machine/mon.h>
#include <machine/vmparam.h>
#include <machine/pmap.h>

extern void printf __P((const char *, ...));

#define VA_SEGNUM(x)    ((u_int)(x) >> SEGSHIFT)

/*
 * globals needed by the vm system
 *
 * [frankly the stupid vm system should allocate these]
 */

vm_offset_t virtual_avail, virtual_end;
vm_offset_t avail_start, avail_end;


/* current_projects:
 * 
 * debugging support
 *
 * need to write/eliminate use of/fix:
 * memavail problem
 * pmap_bootstrap needs to be completed, and fixed.
 * pmap_init does nothing with its arguments....
 * locking protocols
 *
 */

/*
 * Some notes:
 *
 * sun3s have contexts (8).  In our mapping of the world, the kernel is mapped
 * into all contexts.  Processes take up a known portion of the context, 
 * and compete for the available contexts on a LRU basis.
 *
 * sun3s also have this evil "pmeg" crapola.  Essentially each "context"'s
 * address space is defined by the 2048 one-byte entries in the segment map.
 * Each of these 1-byte entries points to a 'pmeg' or page-something-group
 * which contains the mappings for that virtual segment.  A segment is
 * 128Kb wide, and is mapped by 16 8Kb pages.  
 * 
 * As you can tell these "pmeg's" are in short supply and heavy demand.  
 * 'pmeg's allocated to the kernel are "static" in the sense that they can't
 * be stolen from it.  'pmeg's allocated to a particular segment of a
 * pmap's virtual space will be fought over by the other pmaps.
 */

/*
 * wanted attributes:
 *       pmegs that aren't needed by a pmap remain in the MMU.
 *       quick context switches between pmaps
 *       kernel is in all contexts
 *
 *
 */

#define	NKSEG	(NSEGMAP - (KERNBASE / NBSG)) /* is KERNBASE ok? */
#define	NUSEG	(NSEGMAP-NKSEG)			

/*
 * Note that PMAP_LOCK is used in routines called at splnet() and
 * MUST NOT lower the priority.  For this reason we arrange that:
 *    splimp = max(splnet,splbio)
 * Would splvm() be more natural here? (same level as splimp).
 */
#define splpmap splimp
#define PMAP_LOCK() s = splpmap()
#define PMAP_UNLOCK() splx(s)

#define TAILQ_EMPTY(headp) \
        !((headp)->tqh_first)

#define TAILQ_REMOVE_FIRST(result, headp, entries) \
{ \
	result = (headp)->tqh_first; \
	if (result) TAILQ_REMOVE(headp, result, entries); \
	}
	
/*
 * locking issues:
 *
 */
#ifdef	PMAP_DEBUG
int pmap_db_lock;
#define	PMAP_DB_LOCK() do { \
    if (pmap_db_lock) panic("pmap_db_lock: line %d", __LINE__); \
    pmap_db_lock = 1; \
} while (0)
#define PMAP_DB_UNLK() pmap_db_lock = 0
#else	/* PMAP_DEBUG */
#define	PMAP_DB_LOCK() XXX
#define	PMAP_DB_UNLK() XXX
#endif	/* PMAP_DEBUG */

#ifdef	PMAP_DEBUG
int pmeg_lock;
#define	PMEG_LOCK() do { \
    if (pmeg_lock) panic("pmeg_lock: line %d", __LINE__); \
    pmeg_lock = 1; \
} while (0)
#define PMEG_UNLK() pmeg_lock = 0
#else	/* PMAP_DEBUG */
#define	PMEG_LOCK() XXX
#define	PMEG_UNLK() XXX
#endif	/* PMAP_DEBUG */


/*
 * pv support, i.e stuff that maps from physical pages to virtual addresses
 *
 */

struct pv_entry {
    struct pv_entry *pv_next;
    pmap_t           pv_pmap;
    vm_offset_t      pv_va;
    unsigned char    pv_flags;
};

typedef struct pv_entry *pv_entry_t;
pv_entry_t pv_head_table = NULL;
#ifdef	PMAP_DEBUG
struct pv_entry *
pa_to_pvp(pa)
    vm_offset_t pa;
{
    struct pv_entry *pvp;
    if (pa < avail_start || pa >= avail_end) {
	printf("pa_to_pvp: bad pa=0x%x\n", pa);
	Debugger();
    }
    pvp = &pv_head_table[PA_PGNUM(pa)];
    return pvp;
}
#else
#define pa_to_pvp(pa) &pv_head_table[PA_PGNUM(pa)]
#endif

#define PV_VALID  8
#define PV_WRITE  4
#define PV_SYSTEM 2
#define PV_NC     1
#define PV_MASK   0xF

#define MAKE_PV_REAL(pv_flags) ((pv_flags & PV_MASK) << PG_PERM_SHIFT)
#define PG_TO_PV_FLAGS(pte) (((PG_PERM) & pte) >> PG_PERM_SHIFT)

/* cache support */
static unsigned char *pv_cache_table = NULL;
#define set_cache_flags(pa, flags) \
    pv_cache_table[PA_PGNUM(pa)] |= flags & PV_NC
#define force_cache_flags(pa, flags) \
    pv_cache_table[PA_PGNUM(pa)] = flags & PV_NC
#define get_cache_flags(pa) (pv_cache_table[PA_PGNUM(pa)])

/* modified bits support */
static unsigned char *pv_modified_table = NULL;

#if 0
#define save_modified_bits(pte) \
    pv_modified_table[PG_PGNUM(pte)] |= \
    (pte & (PG_OBIO|PG_VME16D|PG_VME32D) ? 0 : \
     ((pte & PG_MOD) >>PG_MOD_SHIFT))
#else
void save_modified_bits(pte)
    vm_offset_t pte;
{
    vm_offset_t pa;
    int pn;

    pa = PG_PA(pte);
    if (pa >= avail_end)
	panic("save_modified_bits: bad pa=%x", pa);

    pn = PG_PGNUM(pte);

    pv_modified_table[pn] |= \
	(pte & (PG_OBIO|PG_VME16D|PG_VME32D) ? 0 : \
	 ((pte & PG_MOD) >>PG_MOD_SHIFT));
}
#endif

int pv_initialized = 0;

#define PMEG_INVAL (NPMEG-1)
#define PMEG_NULL (pmeg_t) NULL
#define pmap_lock(pmap) simple_lock(&pmap->pm_lock)
#define pmap_unlock(pmap) simple_unlock(&pmap->pm_lock)
#define pmap_add_ref(pmap) ++pmap->pm_refcount
#define pmap_del_ref(pmap) --pmap->pm_refcount
#define pmap_refcount(pmap) pmap->pm_refcount
#define get_pmeg_cache(pmap, segnum) (pmap->pm_segmap[segnum])
#define PM_UPDATE_CACHE 1
				/* external structures */
pmap_t kernel_pmap = NULL;
static int pmap_version = 1;
static struct pmap kernel_pmap_store;

/* protection conversion */
static unsigned int protection_converter[8];
#define pmap_pte_prot(x) protection_converter[x]

/* pmeg structures, queues, and macros */
TAILQ_HEAD(pmeg_tailq, pmeg_state);
struct pmeg_tailq pmeg_free_queue, pmeg_inactive_queue,
    pmeg_active_queue, pmeg_kernel_queue;

static struct pmeg_state pmeg_array[NPMEG];

#ifdef	PMAP_DEBUG
pmeg_t pmeg_p(segnum)
    int segnum;
{
    return &pmeg_array[segnum];
}
#else
#define pmeg_p(x) &pmeg_array[x]
#endif

#define is_pmeg_wired(pmegp) (pmegp->pmeg_wired_count > 0)

/* context structures, and queues */
TAILQ_HEAD(context_tailq, context_state);
struct context_tailq context_free_queue, context_active_queue;

static struct context_state context_array[NCONTEXT];

/*
 * location to store virtual addresses
 * to be used in copy/zero operations
 * (set in sun3_startup.c)
 */
vm_offset_t tmp_vpages[2];

/* context support */

/* prototypes */
int get_pte_val __P((pmap_t pmap, vm_offset_t va, vm_offset_t *ptep));
void set_pte_val __P((pmap_t pmap, vm_offset_t va, vm_offset_t pte));

void context_allocate __P((pmap_t pmap));
void context_free __P((pmap_t pmap));
void context_init __P((void));

void pmeg_steal __P((int pmeg_num));
void pmeg_flush __P((pmeg_t pmegp));
pmeg_t pmeg_allocate_invalid __P((pmap_t pmap, vm_offset_t va));
void pmeg_release __P((pmeg_t pmegp));
void pmeg_release_empty __P((pmeg_t pmegp, int segnum));
pmeg_t pmeg_cache __P((pmap_t pmap, vm_offset_t va));
void pmeg_wire __P((pmeg_t pmegp));
void pmeg_unwire __P((pmeg_t pmegp));
void pmeg_init __P((void));

unsigned char pv_compute_cache __P((pv_entry_t head));
int pv_compute_modified __P((pv_entry_t head));
void pv_remove_all __P(( vm_offset_t pa));
unsigned char pv_link  __P((pmap_t pmap, vm_offset_t pa, vm_offset_t va, unsigned char flags));
void pv_change_pte __P((pv_entry_t pv_list, vm_offset_t set_bits, vm_offset_t clear_bits));
void pv_unlink __P((pmap_t pmap, vm_offset_t pa, vm_offset_t va));
void pv_init __P((void));     

void sun3_protection_init __P((void));

void pmap_bootstrap __P((void));
void pmap_init __P((vm_offset_t phys_start, vm_offset_t phys_end));

void pmap_common_init __P((pmap_t pmap));
vm_offset_t pmap_map __P((vm_offset_t virt, vm_offset_t start, vm_offset_t end, int prot));
void pmap_user_pmap_init __P((pmap_t pmap));

pmap_t pmap_create __P((vm_size_t size));
void pmap_release __P((pmap_t pmap));
void pmap_destroy __P((pmap_t pmap));
void pmap_reference __P((pmap_t pmap));
void pmap_pinit __P((pmap_t pmap));

void pmap_page_protect __P((vm_offset_t pa, vm_prot_t prot));

void pmap_remove_range_mmu __P((pmap_t pmap, vm_offset_t sva, vm_offset_t eva));
void pmap_remove_range_contextless __P((pmap_t pmap, vm_offset_t sva, vm_offset_t eva, pmeg_t pmegp));
void pmap_remove_range __P((pmap_t pmap, vm_offset_t sva, vm_offset_t eva));
void pmap_remove __P((pmap_t pmap, vm_offset_t sva, vm_offset_t eva));

void pmap_enter_kernel __P((vm_offset_t va, vm_offset_t pa, vm_prot_t prot, boolean_t wired, vm_offset_t pte_proto, vm_offset_t mem_type));
void pmap_enter_user __P((pmap_t pmap, vm_offset_t va, vm_offset_t pa, vm_prot_t prot, boolean_t wired, vm_offset_t pte_proto, vm_offset_t mem_type));
void pmap_enter __P((pmap_t pmap, vm_offset_t va, vm_offset_t pa, vm_prot_t prot, boolean_t wired));

void pmap_clear_modify __P((vm_offset_t pa));
boolean_t pmap_is_modified __P((vm_offset_t pa));

void pmap_clear_reference __P((vm_offset_t pa));
boolean_t pmap_is_referenced __P((vm_offset_t pa));

void pmap_activate __P((pmap_t pmap, struct pcb *pcbp));
void pmap_deactivate __P((pmap_t pmap, struct pcb *pcbp));

void pmap_change_wiring __P((pmap_t pmap, vm_offset_t va, boolean_t wired));

void pmap_copy __P((pmap_t dst_pmap, pmap_t src_pmap, vm_offset_t dst_addr, vm_size_t len, vm_offset_t src_addr));
void pmap_copy_page __P((vm_offset_t src, vm_offset_t dst));
void pmap_zero_page __P((vm_offset_t pa));

vm_offset_t pmap_extract __P((pmap_t pmap, vm_offset_t va));
vm_offset_t pmap_phys_address __P((int page_number));

void pmap_pageable __P((pmap_t pmap, vm_offset_t sva, vm_offset_t eva, boolean_t pageable));

void pmap_protect_range_contextless __P((pmap_t pmap, vm_offset_t sva, vm_offset_t eva, vm_offset_t pte_proto, pmeg_t pmegp));
void pmap_protect_range_mmu __P((pmap_t pmap, vm_offset_t sva, vm_offset_t eva, vm_offset_t pte_proto));
void pmap_protect_range __P((pmap_t pmap, vm_offset_t sva, vm_offset_t eva, vm_offset_t pte_proto));
void pmap_protect __P((pmap_t pmap, vm_offset_t sva, vm_offset_t eva, vm_prot_t prot));

void pmap_update __P((void));

/*
 * Debugging support.
 */
#define	PMD_ENTER	1
#define	PMD_LINK	2
#define	PMD_PROTECT	4
#define	PMD_SWITCH	8
#define PMD_COW		0x10
#define PMD_MODBIT	0x20
#define PMD_REFBIT	0x40
#define PMD_WIRING	0x80
#define PMD_CONTEXT	0x100
#define PMD_CREATE	0x200
#define PMD_SEGMAP	0x400
#define	PMD_REMOVE	PMD_ENTER
#define	PMD_UNLINK	PMD_LINK
int pmap_debug = 0;

#ifdef	PMAP_DEBUG	/* XXX */
int pmap_db_watchva = -1;
int pmap_db_minphys;
void set_pte_debug(va, pte)
    vm_offset_t va, pte;
{
    if ((pte & PG_VALID) && (PG_PA(pte) < pmap_db_minphys))
    {
	printf("set_pte_debug: va=%x pa=%x\n", va, PG_PA(pte));
	Debugger();
    }
    set_pte(va, pte);
}
#define	set_pte set_pte_debug

int pmap_db_watchpmeg;
void set_segmap_debug(va, sme)
     vm_offset_t va;
     unsigned char sme;
{
    if (sme == pmap_db_watchpmeg) {
	printf("set_segmap_debug: watch pmeg %x\n", sme);
	Debugger();
    }
    set_segmap(va,sme);
}
#define set_segmap set_segmap_debug

#endif	/* PMAP_DEBUG */


/*
 * Get a PTE from either the hardware or the pmeg cache.
 * Return non-zero if PTE was found for this VA.
 */
int get_pte_val(pmap, va, ptep)
     pmap_t pmap;
     vm_offset_t va, *ptep;
{
    int saved_context,s;
    unsigned char sme;
    pmeg_t pmegp;
    int rc=0;

#ifdef	PMAP_DEBUG	/* XXX */
    if (pmap == kernel_pmap)
	panic("get_pte_val: kernel_pmap");
#endif

    PMAP_LOCK();
    if (pmap->pm_context) {
	saved_context = get_context();
	set_context(pmap->pm_context->context_num);
	sme = get_segmap(va);
	if (sme != SEGINV) {
	    *ptep = get_pte(va);
	    rc = 1;
	}
	set_context(saved_context);
    } else {
	/* we don't have a context */
	pmegp = pmeg_cache(pmap, sun3_trunc_seg(va));
	if (pmegp) {
	    *ptep = get_pte_pmeg(pmegp->pmeg_index, VA_PTE_NUM(va));
	    pmeg_release(pmegp);
	    rc = 1;
	}
    }
    PMAP_UNLOCK();
    return rc;
}

void set_pte_val(pmap, va, pte)
     pmap_t pmap;
     vm_offset_t va,pte;
{
    int saved_context, s;
    pmeg_t pmegp;
    unsigned char sme;

#ifdef	PMAP_DEBUG	/* XXX */
    if (pmap == kernel_pmap)
	panic("set_pte_val: kernel_pmap");
#endif

    PMAP_LOCK();
    if (pmap->pm_context) {
	saved_context = get_context();
	set_context(pmap->pm_context->context_num);
	sme = get_segmap(va);
	if (sme != SEGINV)
	    set_pte(va, pte);
	set_context(saved_context);
    } else {
	/* we don't have a context */
	pmegp = pmeg_cache(pmap, sun3_trunc_seg(va));
	if (!pmegp) panic("pmap: no pmeg to set pte in");
	set_pte_pmeg(pmegp->pmeg_index, VA_PTE_NUM(va), pte);
	pmeg_release(pmegp);
    }
    PMAP_UNLOCK();
}

void context_allocate(pmap)
     pmap_t pmap;
{
    context_t context;
    int s;

    PMAP_LOCK();
#ifdef	PMAP_DEBUG
    if (pmap_debug & PMD_CONTEXT)
	printf("context_allocate: for pmap %x\n", pmap);
#endif
    if (pmap == kernel_pmap)
	panic("context_allocate: kernel_pmap");
    if (pmap->pm_context)
	panic("pmap: pmap already has context allocated to it");
    if (TAILQ_EMPTY(&context_free_queue)) { /* steal one from active*/
	if (TAILQ_EMPTY(&context_active_queue))
	    panic("pmap: no contexts to be found");
	context_free((&context_active_queue)->tqh_first->context_upmap);
#ifdef	PMAP_DEBUG
	if (pmap_debug & PMD_CONTEXT)
	    printf("context_allocate: pmap %x, take context %x num %d\n",
		   pmap, context, context->context_num);
#endif
    }
    TAILQ_REMOVE_FIRST(context, &context_free_queue, context_link);
    TAILQ_INSERT_TAIL(&context_active_queue, context, context_link);
    if (context->context_upmap != NULL)
	panic("pmap: context in use???");
    pmap->pm_context = context;
    context->context_upmap = pmap;
#ifdef	PMAP_DEBUG    
    if (pmap_debug & PMD_CONTEXT)
	printf("context_allocate: pmap %x given context %x num %d\n",
	       pmap, context, context->context_num);
#endif
    PMAP_UNLOCK();
}

void context_free(pmap)		/* :) */
     pmap_t pmap;
{
    int saved_context, i, s;
    context_t context;
    unsigned int sme;

    vm_offset_t va;

    PMAP_LOCK();
    if (!pmap->pm_context)
	panic("pmap: can't free a non-existent context");
#ifdef	PMAP_DEBUG
    if (pmap_debug & PMD_CONTEXT)
	printf("context_free: freeing pmap %x of context %x num %d\n",
	       pmap, pmap->pm_context, pmap->pm_context->context_num);
#endif
    saved_context = get_context();
    context = pmap->pm_context;
    set_context(context->context_num);

    /* Unload MMU (but keep in SW segmap). */
    va = 0;
    for (i=0; i < NUSEG; i++) {
	if (pmap->pm_segmap[i] != SEGINV) {
	    /* The MMU might have a valid sme. */
	    sme = get_segmap(va);
	    if (sme != SEGINV) {
#ifdef	PMAP_DEBUG
		/* Validate SME found in MMU. */
		if (sme != pmap->pm_segmap[i])
		    panic("context_free: unknown sme 0x%x at va=0x%x",
			  sme, va);
		if (pmap_debug & PMD_SEGMAP)
		    printf("pmap: set_segmap ctx=%d v=%x old=%x new=ff (cf)\n",
			   context->context_num, sun3_trunc_seg(va), sme);
#endif
		set_segmap(va, SEGINV);
		pmeg_release(pmeg_p(sme));
	    }
	}
#ifdef	DIAGNOSTIC
	if (get_segmap(va) != SEGINV)
	    panic("context_free: did not clean pmap=%x va=%x", pmap, va);
#endif
	va += NBSG;
    }
    set_context(saved_context);
    context->context_upmap = NULL;
    TAILQ_REMOVE(&context_active_queue, context, context_link);
    TAILQ_INSERT_TAIL(&context_free_queue, context,
		      context_link);/* active??? XXX */
    pmap->pm_context = NULL;
#ifdef	PMAP_DEBUG
    if (pmap_debug & PMD_CONTEXT)
	printf("context_free: pmap %x context removed\n", pmap);
#endif
    PMAP_UNLOCK();
}

void context_init()
{
    int i;

    TAILQ_INIT(&context_free_queue);
    TAILQ_INIT(&context_active_queue);

    /* XXX - Can we use context zero?  (Adam says yes.) */
    for (i=0; i < NCONTEXT; i++) {
	context_array[i].context_num = i;
	context_array[i].context_upmap = NULL;
	TAILQ_INSERT_TAIL(&context_free_queue, &context_array[i],
			  context_link);
#ifdef	PMAP_DEBUG
	if (pmap_debug & PMD_CONTEXT)
	    printf("context_init: context num %d is %x\n",
		   i, &context_array[i]);
#endif
    }
}

#ifdef	PMAP_DEBUG
void context_print(context)
    context_t context;
{
    printf(" context_num=  0x%x\n", context->context_num);
    printf(" context_upmap=0x%x\n", context->context_upmap);
}
void pmap_print(pmap)
    pmap_t pmap;
{
    printf(" pm_context=0x%x\n", pmap->pm_context);
    printf(" pm_version=0x%x\n", pmap->pm_version);
    printf(" pm_segmap=0x%x\n", pmap->pm_segmap);
    if (pmap->pm_context) context_print(pmap->pm_context);
}
#endif

/* steal a pmeg without altering its mapping */

void pmeg_steal(pmeg_num)	
     int pmeg_num;
{
    pmeg_t pmegp;

    pmegp = pmeg_p(pmeg_num);
    if (pmegp->pmeg_reserved)
	mon_panic("pmeg_steal: attempt to steal an already reserved pmeg\n");
    if (pmegp->pmeg_owner)
	mon_panic("pmeg_steal: pmeg is already owned\n");

#if 0 /* def	PMAP_DEBUG */
    mon_printf("pmeg_steal: 0x%x\n", pmegp->pmeg_index);
#endif

    /* XXX - Owned by kernel, but not "managed"... */
    pmegp->pmeg_owner = NULL;
    pmegp->pmeg_reserved++;	/* keep count, just in case */
    TAILQ_REMOVE(&pmeg_free_queue, pmegp, pmeg_link);
    pmegp->pmeg_qstate = PMEGQ_NONE;
}

void pmeg_clean(pmegp)
     pmeg_t pmegp;
{
    int i;
    
    for (i = 0; i < NPAGSEG; i++)
	set_pte_pmeg(pmegp->pmeg_index, i, PG_INVAL);
}

/*
 * This routine makes sure that pmegs on the pmeg_free_queue contain
 * no valid ptes.  It pulls things off the queue, cleans them, and
 * puts them at the end.  Ending condition is finding the first queue element
 * at the head of the queue again.
 */

void pmeg_clean_free()
{
    pmeg_t pmegp, pmegp_first;

    if (TAILQ_EMPTY(&pmeg_free_queue))
	panic("pmap: no free pmegs available to clean");

    pmegp_first = NULL;

    PMEG_LOCK();
    for (;;) {

	TAILQ_REMOVE_FIRST(pmegp, &pmeg_free_queue, pmeg_link);
	
#ifdef	PMAP_DEBUG
	if (pmegp->pmeg_index == pmap_db_watchpmeg) {
	    printf("pmeg_clean_free: watch pmeg 0x%x\n", pmegp->pmeg_index);
	    Debugger();
	}
#endif

	pmegp->pmeg_qstate = PMEGQ_NONE;

	pmeg_clean(pmegp);

	TAILQ_INSERT_TAIL(&pmeg_free_queue, pmegp, pmeg_link);
	pmegp->pmeg_qstate = PMEGQ_FREE;

	if (pmegp == pmegp_first)
	    break;
	if (pmegp_first == NULL)
	    pmegp_first = pmegp;

    }
    PMEG_UNLK();
}

/*
 * Clean out an inactive pmeg in preparation for a new owner.
 * (Inactive means referenced in pm_segmap but not hardware.)
 */
void pmeg_flush(pmegp)
     pmeg_t pmegp;
{
    vm_offset_t pte, va;
    pmap_t pmap;
    int i;

    pmap = pmegp->pmeg_owner;
#ifdef	DIAGNOSTIC
    if (pmap == NULL)
	panic("pmeg_flush: no owner, pmeg=0x%x", pmegp);
    if (pmap == kernel_pmap)
	panic("pmeg_flush: kernel_pmap, pmeg=0x%x", pmegp);
#endif
    
#if 0	/* XXX */
    if (!pmegp->pmeg_vpages)
	printf("pmap: pmeg_flush() on clean pmeg\n");
#endif

    va = pmegp->pmeg_va;
    for (i = 0; i < NPAGSEG; i++, va += NBPG) {
	pte = get_pte_pmeg(pmegp->pmeg_index, i);
	if (pte & PG_VALID) {
	    if (pv_initialized)
		save_modified_bits(pte);
	    pv_unlink(pmap, PG_PA(pte), va);
	    pmegp->pmeg_vpages--;
	    set_pte_pmeg(pmegp->pmeg_index, i, PG_INVAL);
	}
    }
#ifdef	PMAP_DEBUG
    if (pmegp->pmeg_vpages != 0) {
	printf("pmap: pmeg_flush() didn't result in a clean pmeg\n");
	Debugger(); /* XXX */
    }
#endif
    /* Invalidate owner's software segmap. (XXX - paranoid) */
    if (pmap->pm_segmap) {
	i = VA_SEGNUM(pmegp->pmeg_va);
#ifdef	DIAGNOSTIC
	if (i >= NUSEG)
	    panic("pmeg_flush: bad va, pmeg=%x", pmegp);
#endif
#ifdef	PMAP_DEBUG
	if (pmap_debug & PMD_SEGMAP) {
	    printf("pm_segmap: pmap=%x i=%x old=%x new=ff (flsh)\n",
		   pmap, i, pmap->pm_segmap[i]);
	}
#endif
	pmap->pm_segmap[i] = SEGINV;
    }
    pmegp->pmeg_owner = NULL;	/* more paranoia */
}

#ifdef	PMAP_DEBUG
void pmeg_verify_empty(va)
    vm_offset_t va;
{
    vm_offset_t pte;
    vm_offset_t eva;

    for (eva = va + NBSG;  va < eva; va += NBPG) {
	pte = get_pte(va);
	if (pte & PG_VALID)
	    panic("pmeg_verify_empty");
    }
}

void pmeg_print(pmegp)
    pmeg_t pmegp;
{
    printf("link_next=0x%x  link_prev=0x%x\n",
	   pmegp->pmeg_link.tqe_next,
	   pmegp->pmeg_link.tqe_prev);
    printf("index=0x%x owner=0x%x own_vers=0x%x\n",
	   pmegp->pmeg_index, pmegp->pmeg_owner,
	   pmegp->pmeg_owner_version);
    printf("va=0x%x wired=0x%x reserved=0x%x vpgs=0x%x qstate=0x%x\n",
	   pmegp->pmeg_va, pmegp->pmeg_wired_count,
	   pmegp->pmeg_reserved, pmegp->pmeg_vpages,
	   pmegp->pmeg_qstate);
}
#endif

pmeg_t pmeg_allocate_invalid(pmap, va)
     pmap_t pmap;
     vm_offset_t va;
{
    pmeg_t pmegp;    

    PMEG_LOCK();
    if (!TAILQ_EMPTY(&pmeg_free_queue)) {
	TAILQ_REMOVE_FIRST(pmegp, &pmeg_free_queue, pmeg_link);
#ifdef	PMAP_DEBUG
	if (pmegp->pmeg_qstate != PMEGQ_FREE)
	    panic("pmeg_alloc_inv: bad on free queue: %x", pmegp);
#endif
    }
    else if (!TAILQ_EMPTY(&pmeg_inactive_queue)) {
	TAILQ_REMOVE_FIRST(pmegp, &pmeg_inactive_queue, pmeg_link);
#ifdef	PMAP_DEBUG
	if (pmap_debug & PMD_SEGMAP)
	    printf("pmeg_alloc_inv: take inactive 0x%x\n", pmegp->pmeg_index);
	if (pmegp->pmeg_qstate != PMEGQ_INACTIVE)
	    panic("pmeg_alloc_inv: bad on inactive queue: %x", pmegp);
#endif
	pmeg_flush(pmegp);
    }
    else if (!TAILQ_EMPTY(&pmeg_active_queue)) {
	TAILQ_REMOVE_FIRST(pmegp, &pmeg_active_queue, pmeg_link);
#ifdef	PMAP_DEBUG
	if (pmap_debug & PMD_SEGMAP)
	    printf("pmeg_alloc_inv: take active %d\n", pmegp->pmeg_index);
	if (pmegp->pmeg_qstate != PMEGQ_ACTIVE)
	    panic("pmeg_alloc_inv: bad on active queue: %x", pmegp);
#endif
	if (pmegp->pmeg_owner == kernel_pmap)
	    panic("pmeg_alloc_invalid: kernel pmeg: %x\n", pmegp);
	pmap_remove_range(pmegp->pmeg_owner, pmegp->pmeg_va,
			  pmegp->pmeg_va+NBSG);
    } else
	panic("pmeg_allocate_invalid: failed");

#ifdef	PMAP_DEBUG
    if (pmegp->pmeg_index == pmap_db_watchpmeg) {
	printf("pmeg_alloc_inv: watch pmeg 0x%x\n", pmegp->pmeg_index);
	Debugger();
    }
#endif
#ifdef	DIAGNOSTIC
    if (pmegp->pmeg_index == SEGINV)
	panic("pmeg_alloc_inv: pmeg_index=ff");
    if (pmegp->pmeg_vpages)
	panic("pmeg_alloc_inv: vpages!=0, pmegp=%x", pmegp);
#endif

    pmegp->pmeg_owner = pmap;
    pmegp->pmeg_owner_version = pmap->pm_version;
    pmegp->pmeg_va = va;
    pmegp->pmeg_wired_count = 0;
    pmegp->pmeg_reserved  = 0;
    pmegp->pmeg_vpages  = 0;
    if (pmap == kernel_pmap) {
	TAILQ_INSERT_TAIL(&pmeg_kernel_queue, pmegp, pmeg_link);
	pmegp->pmeg_qstate = PMEGQ_KERNEL;
    } else {
	TAILQ_INSERT_TAIL(&pmeg_active_queue, pmegp, pmeg_link);
	pmegp->pmeg_qstate = PMEGQ_ACTIVE;
#ifdef	PMAP_DEBUG
	if (pmap_debug & PMD_SEGMAP) {
	    printf("pm_segmap: pmap=%x i=%x old=%x new=%x (ainv)\n",
		   pmap, VA_SEGNUM(va),
		   pmap->pm_segmap[VA_SEGNUM(va)],
		   pmegp->pmeg_index);
	}
#endif
	pmap->pm_segmap[VA_SEGNUM(va)] = pmegp->pmeg_index;
    }
    /* XXX - Make sure pmeg is clean (in caller). */
    PMEG_UNLK();
    return pmegp;
}

/*
 * Put pmeg on the inactive queue.
 * It will be cleaned out when re-allocated.
 */
void pmeg_release(pmegp)
     pmeg_t pmegp;
{
    if (pmegp->pmeg_owner == kernel_pmap)
	panic("pmeg_release: kernel_pmap");

    PMEG_LOCK();
    if (pmegp->pmeg_qstate == PMEGQ_INACTIVE) {
#ifdef	PMAP_DEBUG
	printf("pmeg_release: already inactive\n");
	Debugger();
#endif
    } else {
	if (pmegp->pmeg_qstate != PMEGQ_ACTIVE)
	    panic("pmeg_release: not q_active %x", pmegp);
	TAILQ_REMOVE(&pmeg_active_queue, pmegp, pmeg_link);
	pmegp->pmeg_qstate = PMEGQ_NONE;
	TAILQ_INSERT_TAIL(&pmeg_inactive_queue, pmegp, pmeg_link);
	pmegp->pmeg_qstate = PMEGQ_INACTIVE;
    }
    PMEG_UNLK();
}

/*
 * Put pmeg on the free queue.
 * The pmeg might be in kernel_pmap
 */
void pmeg_release_empty(pmegp, segnum)
     pmeg_t pmegp;
     int segnum;
{
#ifdef	PMAP_DEBUG
    /* XXX - Caller should verify that it's empty. */
    if (pmegp->pmeg_vpages != 0)
	panic("pmeg_release_empty: vpages");
#endif

    PMEG_LOCK();
    switch (pmegp->pmeg_qstate) {
    case PMEGQ_ACTIVE:
	TAILQ_REMOVE(&pmeg_active_queue, pmegp, pmeg_link);
	break;
    case PMEGQ_INACTIVE:
	TAILQ_REMOVE(&pmeg_inactive_queue, pmegp, pmeg_link); /* XXX */
	break;
    case PMEGQ_KERNEL:
	TAILQ_REMOVE(&pmeg_kernel_queue, pmegp, pmeg_link);
	break;
    default:
	panic("pmeg_release_empty: releasing bad pmeg");
	break;
    }

#ifdef	PMAP_DEBUG
    if (pmegp->pmeg_index == pmap_db_watchpmeg) {
	printf("pmeg_release_empty: watch pmeg 0x%x\n",
	       pmegp->pmeg_index);
	Debugger();
    }
#endif
    pmegp->pmeg_qstate = PMEGQ_NONE;

    if (pmegp->pmeg_owner->pm_segmap) {
#ifdef	PMAP_DEBUG
	if (pmap_debug & PMD_SEGMAP) {
	    printf("pm_segmap: pmap=%x i=%x old=%x new=ff (rele)\n",
		   pmegp->pmeg_owner, segnum,
		   pmegp->pmeg_owner->pm_segmap[segnum]);
	}
#endif
	pmegp->pmeg_owner->pm_segmap[segnum] = SEGINV;
    }
#ifdef	PMAP_DEBUG
    else {
	if (pmegp->pmeg_owner != kernel_pmap) {
	    printf("pmeg_release_empty: null segmap\n");
	    Debugger();
	}
    }
#endif
    pmegp->pmeg_owner = NULL;	/* XXX - paranoia */

    TAILQ_INSERT_TAIL(&pmeg_free_queue, pmegp, pmeg_link);
    pmegp->pmeg_qstate = PMEGQ_FREE;

    PMEG_UNLK();
}

pmeg_t pmeg_cache(pmap, va)
     pmap_t pmap;
     vm_offset_t va;
{
    int segnum;
    pmeg_t pmegp;

#ifdef	PMAP_DEBUG
    if (pmap == kernel_pmap)
	panic("pmeg_cache: kernel_pmap");
#endif

    if (pmap->pm_segmap == NULL)
	return PMEG_NULL;
    segnum = VA_SEGNUM(va);
    if (segnum > NUSEG)		/* out of range */
	return PMEG_NULL;
    if (pmap->pm_segmap[segnum] == SEGINV)	/* nothing cached */
	return PMEG_NULL;

    pmegp = pmeg_p(pmap->pm_segmap[segnum]);

#ifdef	PMAP_DEBUG
    if (pmegp->pmeg_index == pmap_db_watchpmeg) {
	printf("pmeg_cache: watch pmeg 0x%x\n", pmegp->pmeg_index);
	Debugger();
    }
#endif

    /* Found a valid pmeg, make sure it's still ours. */
    if ((pmegp->pmeg_owner != pmap) || 
	(pmegp->pmeg_owner_version != pmap->pm_version) ||
	(pmegp->pmeg_va != va))
    {
#ifdef	PMAP_DEBUG
	if (pmap_debug & PMD_SEGMAP) {
	    printf("pm_segmap: pmap=%x i=%x old=%x new=ff (cach)\n",
		   pmap, segnum, pmap->pm_segmap[segnum]);
	}
	/* XXX - Make sure it's not in the MMU? */
	if (pmap->pm_context) {
	    int c, sme;
	    c = get_context();
	    set_context(pmap->pm_context->context_num);
	    sme = get_segmap(va);
	    set_context(c);
	    if (sme != SEGINV)
		panic("pmeg_cache: about to orphan pmeg");
	}
#endif
	pmap->pm_segmap[segnum] = SEGINV;
	return PMEG_NULL; /* cache lookup failed */
    }

    PMEG_LOCK();
#ifdef	PMAP_DEBUG
    /* Make sure it is on the inactive queue. */
    if (pmegp->pmeg_qstate != PMEGQ_INACTIVE)
	panic("pmeg_cache: pmeg was taken: %x", pmegp);
#endif
    TAILQ_REMOVE(&pmeg_inactive_queue, pmegp, pmeg_link);
    pmegp->pmeg_qstate = PMEGQ_NONE;
    TAILQ_INSERT_TAIL(&pmeg_active_queue, pmegp, pmeg_link);
    pmegp->pmeg_qstate = PMEGQ_ACTIVE;

    PMEG_UNLK();
    return pmegp;
}

void pmeg_wire(pmegp)
     pmeg_t pmegp;
{
    pmegp->pmeg_wired_count++;
}
void pmeg_unwire(pmegp)
     pmeg_t pmegp;
{
    pmegp->pmeg_wired_count--;
}

void pmeg_init()
{
    int x;

    /* clear pmeg array, put it all on the free pmeq queue */

    TAILQ_INIT(&pmeg_free_queue);
    TAILQ_INIT(&pmeg_inactive_queue);
    TAILQ_INIT(&pmeg_active_queue);
    TAILQ_INIT(&pmeg_kernel_queue);

    bzero(pmeg_array, NPMEG*sizeof(struct pmeg_state));
    for (x =0 ; x<NPMEG; x++) {
	    TAILQ_INSERT_TAIL(&pmeg_free_queue, &pmeg_array[x],
			      pmeg_link);
	    pmeg_array[x].pmeg_qstate = PMEGQ_FREE;
	    pmeg_array[x].pmeg_index = x;
    }

    /* The last pmeg is not usable. */
    pmeg_steal(SEGINV);
}

void pv_print(pa)
    vm_offset_t pa;
{
    pv_entry_t head;

    printf("pv_list for pa %x\n", pa);

    if (!pv_initialized) return;
    head = pa_to_pvp(pa);
    if (!head->pv_pmap) {
	printf("empty pv_list\n");
	return;
    }
    for (; head != NULL; head = head->pv_next) {
	printf("pv_entry %x pmap %x va %x flags %x next %x\n",
	       head, head->pv_pmap,
	       head->pv_va,
	       head->pv_flags,
	       head->pv_next);
    }
}

unsigned char pv_compute_cache(head)
     pv_entry_t head;
{
    pv_entry_t pv;
    int cread, cwrite, ccache, clen;

    if (!pv_initialized) return 0;
    cread = cwrite = ccache = clen = 0;
    if (!head->pv_pmap) return 0;
    for (pv = head; pv != NULL; pv=pv->pv_next) {
	cread++;
	pv->pv_flags & PV_WRITE ? cwrite++ : 0 ;
	pv->pv_flags & PV_WRITE ? ccache++ : 0 ;
	if (ccache) return PV_NC;
    }
    if ((cread==1) || (cwrite ==0)) return 0;
    return PV_NC;
}
		
int pv_compute_modified(head)
     pv_entry_t head;
{
    pv_entry_t pv;
    int modified;
    vm_offset_t pte;
    unsigned int seg;

    if (!pv_initialized) return 0;
    if (!head->pv_pmap) return 0;
    modified = 0;
    for (pv = head; pv != NULL; pv=pv->pv_next) {
	if (pv->pv_pmap == kernel_pmap) {
	    pte = get_pte(pv->pv_va);
	    if (pte & PG_MOD) return 1;
	    continue;
	}
	seg = VA_SEGNUM(pv->pv_va);
	if (pv->pv_pmap->pm_segmap == NULL) {
#ifdef	PMAP_DEBUG
	    if (pmap_debug & PMD_SEGMAP) {
		printf("pv_compute_modified: null segmap\n");
		Debugger();	/* XXX */
	    }
#endif
	    continue;
	}
	if (pv->pv_pmap->pm_segmap[seg] == SEGINV)
	    continue;
	if (get_pte_val(pv->pv_pmap, pv->pv_va,&pte)) {
	    if (!(pte & PG_VALID)) continue;
	    if (pte & PG_MOD) return 1;
	}
    }
    return 0;
}


/* pv_entry support routines */
void pv_remove_all(pa)
     vm_offset_t pa;
{
    pv_entry_t pv;
    pmap_t pmap;
    vm_offset_t va;

#ifdef PMAP_DEBUG
    if (pmap_debug & PMD_REMOVE)
	printf("pv_remove_all(%x)\n", pa);    
#endif    
    if (!pv_initialized)
	return;

    for (;;) {
	pv = pa_to_pvp(pa);
	if (pv->pv_pmap == NULL)
	    break;
	pmap = pv->pv_pmap;
	va   = pv->pv_va;
	/*
	 * XXX - Have to do pv_unlink here because the call
	 * pmap_remove_range might not unlink the pv entry.
	 * This also implies extra pv_unlink calls...
	 */
	pv_unlink(pmap, pa, va);
	pmap_remove_range(pmap, va, va + NBPG);
	/* The list was modified, so restart at head. */
    }
}

unsigned char pv_link(pmap, pa, va, flags)
     pmap_t pmap;
     vm_offset_t pa, va;
     unsigned char flags;
{
    unsigned char nflags;
    pv_entry_t last,pv,head,npv;
    int s;

#ifdef PMAP_DEBUG
    if ((pmap_debug & PMD_LINK) ||
	(va == pmap_db_watchva))
    {
	printf("pv_link(%x, %x, %x, %x)\n", pmap, pa, va, flags);
	/* pv_print(pa); */
    }
#endif
    if (!pv_initialized) return 0;
    PMAP_LOCK();
    head = pv = pa_to_pvp(pa);
    if (pv->pv_pmap == NULL) {	/* not currently "managed" */
 	pv->pv_va = va;
	pv->pv_pmap = pmap,
	pv->pv_next = NULL;
	pv->pv_flags = flags;
	force_cache_flags(pa, flags);
	PMAP_UNLOCK();
	return flags & PV_NC;
    }
    for (npv = pv ; npv != NULL; last= npv, npv = npv->pv_next ) {
	if ((npv->pv_pmap != pmap) || (npv->pv_va != va)) continue;
	if (flags == npv->pv_flags) {/* no change */
	    PMAP_UNLOCK();
	    return get_cache_flags(pa);
	}
	npv->pv_flags = flags;
	goto recompute;
    }
/*zalloc(pv_zone);*/
    pv = malloc(sizeof(struct pv_entry), M_VMPVENT, M_WAITOK);
    pv->pv_va = va;
    pv->pv_pmap = pmap,
    pv->pv_next = NULL;
    pv->pv_flags = flags;
    last->pv_next = pv;

 recompute:
    if (get_cache_flags(pa) & PG_NC) return flags & PV_NC; /* already NC */
    if (flags & PV_NC) {	/* being NCed, wasn't before */
	force_cache_flags(pa, flags);
	pv_change_pte(head, MAKE_PV_REAL(PV_NC), 0);
	PMAP_UNLOCK();
	return flags & PV_NC;
    }
    nflags = pv_compute_cache(head);
    force_cache_flags(pa, nflags);
    pv_change_pte(head, MAKE_PV_REAL(nflags), 0); /*  */
    PMAP_UNLOCK();
    return nflags & PV_NC;
}

void pv_change_pte(pv_list, set_bits, clear_bits)
     pv_entry_t pv_list;
     vm_offset_t set_bits;
     vm_offset_t clear_bits;
{
    pv_entry_t pv;
    int context;
    vm_offset_t pte;
    unsigned int seg;

    if (!pv_initialized) return;
    if (pv_list->pv_pmap == NULL) /* nothing to hack on */
	return;
    if (!set_bits && !clear_bits) return; /* nothing to do */
    context = get_context();
    for (pv = pv_list; pv != NULL; pv=pv->pv_next) {
	if (pv->pv_pmap == NULL) 
	    panic("pv_list corrupted");
	if (pv->pv_pmap == kernel_pmap) {
	    pte = get_pte(pv->pv_va);
	    pte|=set_bits;
	    pte&=~clear_bits;
	    set_pte(pv->pv_va, pte);
	    continue;
	}
	seg = VA_SEGNUM(pv->pv_va);
	if (pv->pv_pmap->pm_segmap == NULL) {
#if 0 /* def	PMAP_DEBUG */
	    if (pmap_debug & PMD_SEGMAP) {
		printf("pv_change_pte: null segmap\n");
		Debugger();	/* XXX */
	    }
#endif
	    continue;
	}
	if (pv->pv_pmap->pm_segmap[seg] == SEGINV)
	    continue;
	if (!get_pte_val(pv->pv_pmap, pv->pv_va, &pte))
	    continue;
	pte|=set_bits;
	pte&=~clear_bits;
	set_pte_val(pv->pv_pmap, pv->pv_va, pte);
    }
    set_context(context);
}

void pv_unlink(pmap, pa, va)
     pmap_t pmap;
     vm_offset_t pa, va;
{
    pv_entry_t pv,pv_list,dead,last;
    unsigned char saved_flags,nflags;

    if (!pv_initialized) return;
#ifdef PMAP_DEBUG
    if ((pmap_debug & PMD_UNLINK) ||
	(va == pmap_db_watchva))
    {
	printf("pv_unlink(%x, %x, %x)\n", pmap, pa, va);
    }
#endif
    pv_list = pa_to_pvp(pa);

    if (pv_list->pv_pmap == NULL) {
#ifdef PMAP_DEBUG
	if (pmap_debug & PMD_UNLINK) {
	    printf("pv_unlink: empty list\n");
	}
#endif
	return;
    }

    for (pv = pv_list, last = NULL; pv != NULL; last=pv, pv=pv->pv_next) {    
	if ((pv->pv_pmap != pmap) || (pv->pv_va != va))
	    continue;
	/*
	 * right entry now how do we "remove" it, pte will be removed
	 * by code above us. so we should only do deletes, and maybe some cache
	 * recomputation
	 */
	saved_flags = pv->pv_flags;
	if (pv == pv_list) {	/* first entry, mega annoying */
	    if (pv->pv_next) {	/* is something after us, if so copy it
				   forward */
		dead = pv->pv_next;
		pv->pv_va = dead->pv_va;
		pv->pv_pmap = dead->pv_pmap;
		pv->pv_next = dead->pv_next;
		pv->pv_flags = dead->pv_flags;
		/* zfree*/
		free(dead, M_VMPVENT);
	    }
	    else
		pv->pv_pmap = NULL; /* enough to invalidate */
	}
	else {
	    last->pv_next = pv->pv_next;
	    /*	    zfree(pv);*/
	    free(pv, M_VMPVENT);
	}
	if (saved_flags & PV_NC) {/* this entry definitely caused a NC
				  * condition.  
				  */
	    nflags = pv_compute_cache(pv_list);
	    set_cache_flags(pa, nflags);
	    pv_change_pte(pv_list, MAKE_PV_REAL(nflags), 0); /*  */
	}
	return;
    }
#ifdef PMAP_DEBUG
    if (pmap_debug & PMD_UNLINK) {
	printf("pv_unlink: not on list\n");
    }
#endif
}
void pv_init()
{
    int i;

    pv_head_table = (pv_entry_t) kmem_alloc(kernel_map,
					    PA_PGNUM(avail_end) *
					    sizeof(struct pv_entry));
    if (!pv_head_table)
	mon_panic("pmap: kmem_alloc() of pv table failed");
    for (i = 0; i < PA_PGNUM(avail_end); i++) { /* dumb XXX*/
	bzero(&pv_head_table[i], sizeof(struct pv_entry));
    }
    pv_modified_table = (unsigned char *) kmem_alloc(kernel_map,
						     PA_PGNUM(avail_end)*
						     sizeof(char));
    if (!pv_modified_table)
	mon_panic("pmap: kmem_alloc() of pv modified table failed");
    
    bzero(pv_modified_table, sizeof(char)* PA_PGNUM(avail_end));
    pv_cache_table = (unsigned char *) kmem_alloc(kernel_map,
						  PA_PGNUM(avail_end) *
						  sizeof(char));
    if (!pv_cache_table)
	mon_panic("pmap: kmem_alloc() of pv cache table failed");
    bzero(pv_cache_table, sizeof(char)* PA_PGNUM(avail_end));
    pv_initialized++;
}

void sun3_protection_init()
{
    unsigned int *kp, prot;

    kp = protection_converter;
    for (prot = 0; prot < 8; prot++) {
	switch (prot) {
	    /* READ WRITE EXECUTE */
	case VM_PROT_NONE |VM_PROT_NONE |VM_PROT_NONE:
	    *kp++ = PG_INVAL;
	    break;
	case VM_PROT_NONE |VM_PROT_NONE |VM_PROT_EXECUTE:
	case VM_PROT_READ |VM_PROT_NONE |VM_PROT_NONE:
	case VM_PROT_READ |VM_PROT_NONE |VM_PROT_EXECUTE:
	    *kp++ = PG_VALID;
	    break;
	case VM_PROT_NONE |VM_PROT_WRITE |VM_PROT_NONE:
	case VM_PROT_NONE |VM_PROT_WRITE |VM_PROT_EXECUTE:
	case VM_PROT_READ |VM_PROT_WRITE |VM_PROT_NONE:
	case VM_PROT_READ |VM_PROT_WRITE |VM_PROT_EXECUTE:
	    *kp++ = PG_VALID|PG_WRITE;
	    break;
	}
    }
}
/* pmap maintenance routines */

void pmap_common_init(pmap)
     pmap_t pmap;
{
    bzero(pmap, sizeof(struct pmap));
    pmap->pm_refcount=1;
    pmap->pm_version = pmap_version++;
    simple_lock_init(&pmap->pm_lock);
}

void pmap_bootstrap()
{
    extern void vm_set_page_size();

    PAGE_SIZE = NBPG;
    vm_set_page_size();

    sun3_protection_init();

   /* after setting up some structures */

    kernel_pmap = &kernel_pmap_store;
    pmap_common_init(kernel_pmap);

    /* pmeg_init(); done in sun3_vm_init() */
    context_init();
    
    pmeg_clean_free();
}

/*
 * Bootstrap memory allocator. This function allows for early dynamic
 * memory allocation until the virtual memory system has been bootstrapped.
 * After that point, either kmem_alloc or malloc should be used. This
 * function works by stealing pages from the (to be) managed page pool,
 * stealing virtual address space, then mapping the pages and zeroing them.
 *
 * It should be used from pmap_bootstrap till vm_page_startup, afterwards
 * it cannot be used, and will generate a panic if tried. Note that this
 * memory will never be freed, and in essence it is wired down.
 */
void *
pmap_bootstrap_alloc(size)
	int size;
{
	register void *mem;

	extern boolean_t vm_page_startup_initialized;
	if (vm_page_startup_initialized)
		panic("pmap_bootstrap_alloc: called after startup initialized");

	size = round_page(size);
	mem = (void *)virtual_avail;
	virtual_avail = pmap_map(virtual_avail, avail_start,
	    avail_start + size, VM_PROT_READ|VM_PROT_WRITE);
	avail_start += size;
	bzero((void *)mem, size);
	return (mem);
}

/*
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void
pmap_init(phys_start, phys_end)
	vm_offset_t	phys_start, phys_end;
{
    extern int physmem;

    pv_init();
    physmem = btoc(phys_end);
}

vm_offset_t
pmap_map(virt, start, end, prot)
     vm_offset_t	virt;
     vm_offset_t	start;
     vm_offset_t	end;
     int		prot;
{
    while (start < end) {
	pmap_enter(kernel_pmap, virt, start, prot, FALSE);
	virt += NBPG;
	start += NBPG;
    }
    return(virt);
}

void pmap_user_pmap_init(pmap)
     pmap_t pmap;
{
    int i;
    pmap->pm_segmap = malloc(sizeof(char)*NUSEG, M_VMPMAP, M_WAITOK);
    for (i=0; i < NUSEG; i++) {
	pmap->pm_segmap[i] = SEGINV;
    }
}

/*
 *	Create and return a physical map.
 *
 *	If the size specified for the map
 *	is zero, the map is an actual physical
 *	map, and may be referenced by the
 *	hardware.
 *
 *	If the size specified is non-zero,
 *	the map will be used in software only, and
 *	is bounded by that size.
 */
pmap_t
pmap_create(size)
	vm_size_t	size;
{
    pmap_t pmap;

    if (size)
	return NULL;

    PMAP_DB_LOCK();
    pmap = (pmap_t) malloc(sizeof(struct pmap), M_VMPMAP, M_WAITOK);
    pmap_common_init(pmap);
    pmap_user_pmap_init(pmap);
    PMAP_DB_UNLK();
    return pmap;
}

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_release(pmap)
     struct pmap *pmap;
{

    if (pmap == kernel_pmap)
	panic("pmap_release: kernel_pmap!");

    PMAP_DB_LOCK();
    if (pmap->pm_context)
	context_free(pmap);
    free(pmap->pm_segmap, M_VMPMAP);
    pmap->pm_segmap = NULL;
    PMAP_DB_UNLK();
}


/*
 *	Retire the given physical map from service.
 *	Should only be called if the map contains
 *	no valid mappings.
 */
void
pmap_destroy(pmap)
      pmap_t pmap;
{
    int count;

    if (pmap == NULL)
	return;	/* XXX - Duh! */

    PMAP_DB_LOCK();
#ifdef PMAP_DEBUG
    if (pmap_debug & PMD_CREATE)
	printf("pmap_destroy(%x)\n", pmap);
#endif
    if (pmap == kernel_pmap)
	panic("pmap_destroy: kernel_pmap!");
    pmap_lock(pmap);
    count = pmap_del_ref(pmap);
    pmap_unlock(pmap);
    if (count == 0) {
	pmap_release(pmap);
	free((caddr_t)pmap, M_VMPMAP); /* XXXX -- better make sure we
					  it this way allocate */
    }
    PMAP_DB_UNLK();
}

/*
 *      pmap_page_protect:
 *
 *      Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(pa, prot)
        vm_offset_t     pa;
        vm_prot_t       prot;
{
    int s;

    PMAP_LOCK();
    PMAP_DB_LOCK();
#ifdef PMAP_DEBUG
    if (pmap_debug & PMD_PROTECT)
	printf("pmap_page_protect(%x, %x)\n", pa, prot);
#endif
    switch (prot) {
    case VM_PROT_ALL:
	break;
    case VM_PROT_READ:
    case VM_PROT_READ|VM_PROT_EXECUTE:
	pv_change_pte(pa_to_pvp(pa), 0, PG_WRITE);
	break;
    default:
	/* remove mapping for all pmaps that have it:
	 *
	 * follow pv trail to pmaps and temporarily delete it that way.
	 * keep looping till all mappings go away
	 */
	pv_remove_all(pa);
    }
    PMAP_DB_UNLK();
    PMAP_UNLOCK();
}

/*
 *	Add a reference to the specified pmap.
 */
void
pmap_reference(pmap)
	pmap_t	pmap;
{
    PMAP_DB_LOCK();
    if (pmap != NULL) {
	pmap_lock(pmap);
	pmap_add_ref(pmap);
	pmap_unlock(pmap);
    }
    PMAP_DB_UNLK();
}

void pmap_remove_range_mmu(pmap, sva, eva)
     pmap_t pmap;
     vm_offset_t sva, eva;
{
    int saved_context,i;
    unsigned int sme;
    pmeg_t pmegp;
    vm_offset_t va,pte;
    
    /* XXX - Never unmap DVMA space... */
    if (sva >= VM_MAX_KERNEL_ADDRESS)
	return;

    saved_context = get_context();
    if (pmap != kernel_pmap) 
	set_context(pmap->pm_context->context_num);

    sme = get_segmap(sva);
    if (sme != SEGINV) {
	pmegp = pmeg_p(sme);
#ifdef	DIAGNOSTIC
	/* Make sure it is in our software segmap (cache). */
	if (pmap->pm_segmap && (pmap->pm_segmap[VA_SEGNUM(sva)] != sme))
	    panic("pmap_remove_range_mmu: MMU has bad pmeg %x", sme);
#endif
    } else {
	if (pmap == kernel_pmap)
	    return;
	pmegp = pmeg_cache(pmap, sun3_trunc_seg(sva));
	if (!pmegp)
	    goto outta_here;
#ifdef	PMAP_DEBUG
	if (pmap_debug & PMD_SEGMAP) {
	    if (pmap != kernel_pmap)
		printf("pmap: set_segmap ctx=%d v=%x old=ff new=%x (rm1)\n",
		       get_context(), sun3_trunc_seg(sva),
		       pmegp->pmeg_index);
	}
#endif
	set_segmap(sva, pmegp->pmeg_index);
    }
    /* have pmeg, will travel */

#ifdef	DIAGNOSTIC
    /* Make sure we own the pmeg, right va, etc. */
    if (pmegp->pmeg_va != sun3_trunc_seg(sva))
	panic("pmap_remove_range_mmu: wrong va pmeg %x", pmegp);
    if (pmegp->pmeg_owner != pmap)
	panic("pmap_remove_range_mmu: not my pmeg %x", pmegp);
    if (pmegp->pmeg_owner_version != pmap->pm_version)
	panic("pmap_remove_range_mmu: old vers pmeg %x", pmegp);
    if (pmap == kernel_pmap) {
	if (pmegp->pmeg_qstate != PMEGQ_KERNEL)
	    panic("pmap_remove_range_mmu: pmeg not q_kernel %x", pmegp);
    } else {
	if (pmegp->pmeg_qstate != PMEGQ_ACTIVE)
	    panic("pmap_remove_range_mmu: pmeg not q_active %x", pmegp);
    }
#endif

    if (pmegp->pmeg_vpages <= 0)
	goto outta_here; /* no valid pages anyway */

    for (va = sva; va < eva; va += NBPG) {
	pte = get_pte(va);
	if (pte & PG_VALID) {
	    vm_offset_t pa;
	    pa = PG_PA(pte);
#ifdef	PMAP_DEBUG
	    if (pte & PG_TYPE)
		panic("pmap_remove_range_mmu: not memory, va=%x", va);
	    if (pa < avail_start || pa >= avail_end)
		panic("pmap_remove_range_mmu: not managed, va=%x", va);
#endif
	    if (pv_initialized)
		save_modified_bits(pte);
	    pv_unlink(pmap, pa, va);
	    pmegp->pmeg_vpages--;
	    set_pte(va, PG_INVAL);
	}
    }
    if (pmegp->pmeg_vpages <= 0) {
	/* We are done with this pmeg. */
	if (is_pmeg_wired(pmegp)) {
	    printf("pmap: removing wired pmeg: 0x%x\n", pmegp);
	    Debugger(); /* XXX */
	}
	/* First, remove it from the MMU. */
	if (kernel_pmap == pmap) {
	    for (i=0; i < NCONTEXT; i++) { /* map out of all segments */
		set_context(i);
		set_segmap(sva,SEGINV);
	    }
	} else {
#ifdef	PMAP_DEBUG
	    if (pmap_debug & PMD_SEGMAP) {
		printf("pmap: set_segmap ctx=%d v=%x old=%x new=ff (rm2)\n",
		       get_context(), sun3_trunc_seg(sva),
		       pmegp->pmeg_index);
	    }
#endif
	    set_segmap(sva, SEGINV);
	}
	/* Now, put it on the free list. */
	pmeg_release_empty(pmegp, VA_SEGNUM(sva));
    }
outta_here:
    set_context(saved_context);
}

void pmap_remove_range_contextless(pmap, sva, eva, pmegp)
     pmap_t pmap;
     vm_offset_t sva, eva;
     pmeg_t pmegp;
{
    int sp,ep,i;
    vm_offset_t pte,va;

#ifdef	PMAP_DEBUG
    /* Kernel always in a context (actually, in all contexts). */
    if (pmap == kernel_pmap)
	panic("pmap_remove_range_contextless: kernel_pmap");
    /* The pmeg passed here is always from the pmeg_cache */
    if (pmegp->pmeg_qstate != PMEGQ_ACTIVE)
	panic("pmap_remove_range_contextless: pmeg_qstate");
#endif

    sp = VA_PTE_NUM(sva);
    ep = VA_PTE_NUM(eva);
    va = sva;
    for (i = sp; i < ep; i++) {
	pte = get_pte_pmeg(pmegp->pmeg_index, i);
	if (pte & PG_VALID) {
	    if (pv_initialized)
		save_modified_bits(pte);
	    pv_unlink(pmap, PG_PA(pte), va);
	    pmegp->pmeg_vpages--;
	    set_pte_pmeg(pmegp->pmeg_index, i, PG_INVAL);
	}
	va+=NBPG;
    }
    if (pmegp->pmeg_vpages <= 0) {
	if (is_pmeg_wired(pmegp))
	    panic("pmap: removing wired");
	pmeg_release_empty(pmegp, VA_SEGNUM(sva));
    }
    else pmeg_release(pmegp);
}

/* guaraunteed to be within one pmeg */
void pmap_remove_range(pmap, sva, eva)
     pmap_t pmap;
     vm_offset_t sva, eva;
{
    pmeg_t pmegp;

#ifdef	PMAP_DEBUG
    if ((pmap_debug & PMD_REMOVE) ||
	((sva <= pmap_db_watchva && eva > pmap_db_watchva)))
	printf("pmap_remove_range(%x, %x, %x)\n", pmap, sva, eva);
#endif

   /* cases: kernel: always has context, always available
    *
    *        user: has context, is available
    *        user: has no context, is available
    *        user: has no context, is not available (NOTHING) |_ together
    *        user: has context, isn't available (NOTHING)     |
    */
    if (pmap != kernel_pmap) {
	if (pmap->pm_segmap == NULL) {
#ifdef	PMAP_DEBUG
	    if (pmap_debug & PMD_SEGMAP) {
		printf("pmap_remove_range: null segmap\n");
		Debugger(); /* XXX */
	    }
#endif
	    return;
	}
	if (get_pmeg_cache(pmap, VA_SEGNUM(sva)) == SEGINV) {
#ifdef	PMAP_DEBUG
	    /* XXX - Make sure it's not in the MMU? */
	    if (pmap->pm_context) {
		int c, sme;
		c = get_context();
		set_context(pmap->pm_context->context_num);
		sme = get_segmap(sva);
		set_context(c);
		if (sme != SEGINV)
		    panic("pmap_remove_range: not in cache");
	    }
#endif
	    return;
	}
    }

    if ((pmap == kernel_pmap) || (pmap->pm_context)) {
	pmap_remove_range_mmu(pmap, sva, eva);
	return;
    }
    /* we are a user pmap without a context, possibly without a pmeg to
     * operate upon
     *
     */
    pmegp = pmeg_cache(pmap, sun3_trunc_seg(sva));
    if (!pmegp) return;
    pmap_remove_range_contextless(pmap, sva, eva, pmegp);
}

/*
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the page size.
 */
void pmap_remove(pmap, sva, eva)
     pmap_t pmap;
     vm_offset_t sva, eva;
{
    vm_offset_t va,neva;
    pmeg_t pmegp;
    int s;
    /* some form of locking?, when where. */

    if (pmap == NULL)
	return;

    /* do something about contexts */
    if (pmap == kernel_pmap) {
	if (sva < VM_MIN_KERNEL_ADDRESS)
	    sva = VM_MIN_KERNEL_ADDRESS;
	if (eva > VM_MAX_KERNEL_ADDRESS)
	    eva = VM_MAX_KERNEL_ADDRESS;
    } else {
	if (eva > VM_MAXUSER_ADDRESS)
	    eva = VM_MAXUSER_ADDRESS;
    }
    PMAP_LOCK();
    PMAP_DB_LOCK();
    va = sva;
    pmegp = NULL;
    while (va < eva) {
	neva = sun3_round_up_seg(va);
	if (neva > eva)
	    neva = eva;
	pmap_remove_range(pmap, va, neva);
	va = neva;
    }
    PMAP_DB_UNLK();
    PMAP_UNLOCK();
}

void pmap_enter_kernel(va, pa, prot, wired, pte_proto, mem_type)
     vm_offset_t va;
     vm_offset_t pa;
     vm_prot_t prot;
     boolean_t wired;
     vm_offset_t pte_proto;
     vm_offset_t mem_type;
{
    int s,i;
    vm_offset_t current_pte;
    unsigned char sme,nflags;
    int c;
    pmeg_t pmegp;
    /*
      keep in hardware only, since its mapped into all contexts anyway;
      need to handle possibly allocating additional pmegs
      need to make sure they cant be stolen from the kernel;
      map any new pmegs into all contexts, make sure rest of pmeg is null;
      deal with pv_stuff; possibly caching problems;
      must also deal with changes too.
      */
    pte_proto |= PG_SYSTEM | MAKE_PGTYPE(mem_type);
    /*
     * In detail:
     *
     * (a) lock pmap
     * (b) Is the VA in a already mapped segment, if so
     *     look to see if that VA address is "valid".  If it is, then
     *     action is a change to an existing pte
     * (c) if not mapped segment, need to allocate pmeg
     * (d) if adding pte entry or changing physaddr of existing one,
     *        use pv_stuff, for change, pmap_remove() possibly.
     * (e) change/add pte
     */
    if (va < VM_MIN_KERNEL_ADDRESS)
	panic("pmap: kernel trying to allocate virtual space below minkva\n");
    PMAP_LOCK();
    sme = get_segmap(va);
    /* XXXX -- lots of non-defined routines, need to see if pmap has a
     * context
     */
    if (sme == SEGINV) {
	pmegp = pmeg_allocate_invalid(kernel_pmap, sun3_trunc_seg(va));
	c = get_context();	
	for (i=0; i < NCONTEXT; i++) { /* map into all segments */
	    set_context(i);
	    set_segmap(va,pmegp->pmeg_index);
	}
	set_context(c);
#ifdef PMAP_DEBUG
	pmeg_verify_empty(sun3_trunc_seg(va));
#endif
	goto add_pte;
    }
    else {
	/* Found an existing pmeg.  Modify it... */
	pmegp = pmeg_p(sme);
#ifdef	DIAGNOSTIC
	/* Make sure it is ours. */
	if (pmegp->pmeg_owner && (pmegp->pmeg_owner != kernel_pmap))
	    panic("pmap_enter_kernel: MMU has bad pmeg %x", sme);
#endif
	/* Don't try to unlink if in DVMA map. */
	if (va >= VM_MAX_KERNEL_ADDRESS) {
		/* also make sure it is non-cached :) */
		pte_proto |= PG_NC;
		goto add_pte;
	}
	/* Make sure we own it. */
	if (pmegp->pmeg_owner != kernel_pmap)
	    panic("pmap_enter_kernel: found unknown pmeg");
    }

    /* does mapping already exist */
    /*    (a) if so, is it same pa then really a protection change
     *    (b) if not same, pa then we have to unlink from old pa
     *    (c) 
     */
    current_pte = get_pte(va);
    if ((current_pte & PG_VALID) == 0)
	goto add_pte; /* just adding */
    pmegp->pmeg_vpages--;	/* removing valid page here, way lame XXX*/
    if (pv_initialized)
	save_modified_bits(current_pte);
    if (PG_PGNUM(current_pte) != PG_PGNUM(pte_proto)) /* !same physical addr*/
	pv_unlink(kernel_pmap, PG_PA(current_pte), va); 

add_pte:	/* can be destructive */
    if (wired) {
	/* Will pmeg wire count go from zero to one? */
	if (!is_pmeg_wired(pmegp))
	    kernel_pmap->pm_stats.wired_count++;
	pmeg_wire(pmegp);	/* XXX */
    } else {
	pmeg_unwire(pmegp);	/* XXX */
	/* Did pmeg wire count go from one to zero? */
	if (!is_pmeg_wired(pmegp))
	    kernel_pmap->pm_stats.wired_count--;
    }
    if (mem_type & PG_TYPE) 
	set_pte(va, pte_proto | PG_NC);
    else {
	nflags = pv_link(kernel_pmap, pa, va, PG_TO_PV_FLAGS(pte_proto));
	if (nflags & PV_NC)
	    set_pte(va, pte_proto | PG_NC);
	else
	    set_pte(va, pte_proto);
    }
    pmegp->pmeg_vpages++;	/* assumes pmap_enter can never insert
				 a non-valid page*/
    PMAP_UNLOCK();
}


void pmap_enter_user(pmap, va, pa, prot, wired, pte_proto, mem_type)
     pmap_t pmap;
     vm_offset_t va;
     vm_offset_t pa;
     vm_prot_t prot;
     boolean_t wired;
     vm_offset_t pte_proto;
     vm_offset_t mem_type;
{
    int saved_context,s; 
    unsigned char sme,nflags;
    pmeg_t pmegp;
    vm_offset_t current_pte;
    /* sanity checking */
    if (mem_type)
	panic("pmap: attempt to map non main memory page into user space");
    if ((va+NBPG) > VM_MAXUSER_ADDRESS)
	panic("pmap: user trying to allocate virtual space above itself\n");

#ifdef	PMAP_DEBUG
    if ((pmap_debug & PMD_ENTER) && wired) {
	printf("pmap_enter_user: attempt to wire user page, ignored\n");
	printf("pmap=0x%x va=0x%x pa=0x%x\n", pmap, va, pa);
    }
#endif

    pte_proto |= MAKE_PGTYPE(PG_MMEM); /* unnecessary */
    PMAP_LOCK();
    saved_context = get_context();
    if (!pmap->pm_context) {
	/* XXX - Why is this happening? -gwr */
#ifdef	PMAP_DEBUG
	printf("pmap: pmap_enter_user() on pmap without a context\n");
	/* XXX - Why is this happening occasionally? -gwr */
#endif
	context_allocate(pmap);
    }
    if (saved_context != pmap->pm_context->context_num)
	set_context(pmap->pm_context->context_num);

    sme = get_segmap(va);
    if (sme != SEGINV) {
	/* Found an existing pmeg. */
	pmegp = pmeg_p(sme);
#ifdef	DIAGNOSTIC
	/* Make sure it is in our software segmap (cache). */
	/* XXX - We are hitting this one! -gwr */
	if (pmap->pm_segmap[VA_SEGNUM(va)] != sme)
	    panic("pmap_enter_user: MMU has bad pmeg %x", sme);
#endif
    } else {
	pmegp = pmeg_cache(pmap, sun3_trunc_seg(va));
	if (pmegp) {
	    /* found cached pmeg - just reinstall in segmap */
#ifdef	PMAP_DEBUG
	    if (pmap_debug & PMD_SEGMAP) {
		printf("pmap: set_segmap ctx=%d v=%x old=ff new=%x (eu1)\n",
		       get_context(), sun3_trunc_seg(va),
		       pmegp->pmeg_index);
	    }
#endif
	    set_segmap(va, pmegp->pmeg_index);
	} else {
	    /* no cached pmeg - get a new one */
	    pmegp = pmeg_allocate_invalid(pmap, sun3_trunc_seg(va));
#ifdef	PMAP_DEBUG
	    if (pmap_debug & PMD_SEGMAP) {
		printf("pmap: set_segmap ctx=%d v=%x old=ff new=%x (eu2)\n",
		       get_context(), sun3_trunc_seg(va),
		       pmegp->pmeg_index);
	    }
#endif
	    set_segmap(va, pmegp->pmeg_index);
#ifdef PMAP_DEBUG
	    pmeg_verify_empty(sun3_trunc_seg(va));
	    if (pmap->pm_segmap[VA_SEGNUM(va)] != pmegp->pmeg_index)
		panic("pmap_enter_user: pmeg_alloc_inv broken?");
#endif
	}
    }

#ifdef	DIAGNOSTIC
    /* Make sure we own the pmeg, right va, etc. */
    if (pmegp->pmeg_va != sun3_trunc_seg(va))
	panic("pmap_enter_user: wrong va pmeg %x", pmegp);
    if (pmegp->pmeg_owner != pmap)
	panic("pmap_enter_user: not my pmeg %x", pmegp);
    if (pmegp->pmeg_owner_version != pmap->pm_version)
	panic("pmap_enter_user: old vers pmeg %x", pmegp);
    if (pmegp->pmeg_qstate != PMEGQ_ACTIVE)
	panic("pmap_enter_user: pmeg not active %x", pmegp);
#endif

    /* does mapping already exist */
    /*    (a) if so, is it same pa then really a protection change
     *    (b) if not same, pa then we have to unlink from old pa
     *    (c) 
     */
    current_pte = get_pte(va);
    if ((current_pte & PG_VALID) == 0)
	goto add_pte;
    if (pv_initialized)
	save_modified_bits(current_pte);
    pmegp->pmeg_vpages--;	/* removing valid page here, way lame XXX*/
    if (PG_PGNUM(current_pte) != PG_PGNUM(pte_proto)) 
	pv_unlink(pmap, PG_PA(current_pte), va); 

add_pte:
    /* if we did wiring on user pmaps, then the code would be here */
    /* XXX - pv_link calls malloc which calls pmap_enter_kernel... */
    nflags = pv_link(pmap, pa, va, PG_TO_PV_FLAGS(pte_proto));
    if (nflags & PV_NC)
	set_pte(va, pte_proto | PG_NC);
    else
	set_pte(va, pte_proto);
    pmegp->pmeg_vpages++;	/* assumes pmap_enter can never insert
				   a non-valid page */
    set_context(saved_context);
    PMAP_UNLOCK();
}
/* 
 *	Insert the given physical page (p) at
 *	the specified virtual address (v) in the
 *	target physical map with the protection requested.
 *
 *	If specified, the page will be wired down, meaning
 *	that the related pte can not be reclaimed.
 *
 *	NB:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */

void
pmap_enter(pmap, va, pa, prot, wired)
     pmap_t pmap;
     vm_offset_t va;
     vm_offset_t pa;
     vm_prot_t prot;
     boolean_t wired;
{
    vm_offset_t pte_proto, mem_type;
    int s;

    if (pmap == NULL) return;
#ifdef	PMAP_DEBUG
    if ((pmap_debug & PMD_ENTER) ||
	(va == pmap_db_watchva))
	printf("pmap_enter(%x, %x, %x, %x, %x)\n",
	       pmap, va, pa, prot, wired);
#endif
    mem_type = pa & PMAP_MEMMASK;
    pte_proto = PG_VALID | pmap_pte_prot(prot) | (pa & PMAP_NC ? PG_NC : 0);
    pa &= ~PMAP_SPECMASK;
    pte_proto |= PA_PGNUM(pa);

    /* treatment varies significantly:
     *  kernel ptes are in all contexts, and are always in the mmu
     *  user ptes may not necessarily? be in the mmu.  pmap may not
     *   be in the mmu either.
     *
     */
    PMAP_LOCK();
    if (pmap == kernel_pmap) {
	/* This can be called recursively through malloc. */
	pmap_enter_kernel(va, pa, prot, wired, pte_proto, mem_type);
    } else {
	PMAP_DB_LOCK();
	pmap_enter_user(pmap, va, pa, prot, wired, pte_proto, mem_type);
    PMAP_DB_UNLK();
    }
    PMAP_UNLOCK();
}

void
pmap_clear_modify(pa)
     vm_offset_t	pa;
{
    int s;

    if (!pv_initialized) return;
#ifdef	PMAP_DEBUG
    if (pmap_debug & PMD_MODBIT)
	printf("pmap_clear_modified: %x\n", pa);
#endif
    PMAP_LOCK();
    PMAP_DB_LOCK();
    pv_modified_table[PA_PGNUM(pa)] = 0;
    pv_change_pte(pa_to_pvp(pa), 0, PG_MOD);
    PMAP_DB_UNLK();
    PMAP_UNLOCK();
}
boolean_t
pmap_is_modified(pa)
     vm_offset_t	pa;
{
    int s;

#ifdef	PMAP_DEBUG
    if (pmap_debug & PMD_MODBIT)
	printf("pmap_is_modified: %x\n", pa);
#endif
    if (!pv_initialized) return 0;
    if (pv_modified_table[PA_PGNUM(pa)]) return 1;
    PMAP_LOCK();
    PMAP_DB_LOCK();
    pv_modified_table[PA_PGNUM(pa)] = pv_compute_modified(pa_to_pvp(pa));
    PMAP_DB_UNLK();
    PMAP_UNLOCK();
    return pv_modified_table[PA_PGNUM(pa)];
}

void pmap_clear_reference(pa)
     vm_offset_t	pa;
{
    int s;
    
    if (!pv_initialized) return;
#ifdef PMAP_DEBUG
    if (pmap_debug & PMD_REFBIT)
	printf("pmap_clear_referenced: %x\n", pa);
#endif 
    PMAP_LOCK();
    PMAP_DB_LOCK();
    pv_remove_all(pa);
    PMAP_DB_UNLK();
    PMAP_UNLOCK();
}

boolean_t
pmap_is_referenced(pa)
     vm_offset_t	pa;
{
#ifdef PMAP_DEBUG
    if (pmap_debug & PMD_REFBIT)
	printf("pmap_is_referenced: %x\n", pa);
#endif
    return FALSE;	/* XXX */
}


void pmap_activate(pmap, pcbp)
     pmap_t pmap;
     struct pcb *pcbp;
{
    int s, newctx;

#ifdef	PMAP_DEBUG
	if (pmap_debug & PMD_SWITCH)
	    printf("pmap_activate(%x, %x)\n", pmap, pcbp);
#endif

    if (pmap == kernel_pmap)
	panic("pmap_activate: kernel_pmap");

    if (pmap->pm_context == NULL)
	context_allocate(pmap);

    newctx = pmap->pm_context->context_num;

    if (newctx != get_context()) {
#ifdef PMAP_DEBUG
	if (pmap_debug & PMD_SWITCH)
	    printf("pmap_activate(%x, %x) switching to context %d\n",
		   pmap, pcbp, newctx);
#endif
	set_context(newctx);
    }
}
void pmap_deactivate(pmap, pcbp)
     pmap_t pmap;
     struct pcb *pcbp;
{
#ifdef PMAP_DEBUG
    if (pmap_debug & PMD_SWITCH)
	printf("pmap_deactivate(%x, %x)\n", pmap, pcbp);
#endif
    /* XXX - Why bother? -gwr */
    TAILQ_INSERT_TAIL(&context_active_queue, pmap->pm_context, context_link);
}

/*
 *	Routine:	pmap_change_wiring
 *	Function:	Change the wiring attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 */
void
pmap_change_wiring(pmap, va, wired)
	 pmap_t	pmap;
	vm_offset_t	va;
	boolean_t	wired;
{
    int s, sme;
    pmeg_t pmegp;

#ifdef PMAP_DEBUG
    if (pmap_debug & PMD_WIRING)
	printf("pmap_change_wiring(0x%x, 0x%x, %x)\n",
	       pmap, va, wired);
#endif
    PMAP_LOCK();
    PMAP_DB_LOCK();
    if (pmap == kernel_pmap) {
	sme = get_segmap(va);
	if (sme == SEGINV)
	    panic("pmap_change_wiring: invalid va=0x%x", va);
	pmegp = pmeg_p(sme);
	if (wired)
	    pmeg_wire(pmegp);
	else
	    pmeg_unwire(pmegp);
    } else {
	if (pmap_debug & PMD_WIRING)
	    printf("pmap_change_wiring: user pmap (ignored)\n");
    }
    PMAP_DB_UNLK();
    PMAP_UNLOCK();
}
/*
 *	Copy the range specified by src_addr/len
 *	from the source map to the range dst_addr/len
 *	in the destination map.
 *
 *	This routine is only advisory and need not do anything.
 */
void pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	pmap_t		dst_pmap;
	pmap_t		src_pmap;
	vm_offset_t	dst_addr;
	vm_size_t	len;
	vm_offset_t	src_addr;
{
}

/*
 *	pmap_copy_page copies the specified (machine independent)
 *	page by mapping the page into virtual memory and using
 *	bcopy to copy the page, one machine dependent page at a
 *	time.
 */
void pmap_copy_page(src, dst)
	 vm_offset_t	src, dst;
{
    vm_offset_t pte;
    int s;

#ifdef	PMAP_DEBUG
    if (pmap_debug & PMD_COW)
	printf("pmap_copy_page: %x -> %x\n", src, dst);
#endif
    PMAP_LOCK();
    PMAP_DB_LOCK();
    pte = PG_VALID |PG_SYSTEM|PG_WRITE|PG_NC|PG_MMEM| PA_PGNUM(src);
    set_pte(tmp_vpages[0], pte);
    pte = PG_VALID |PG_SYSTEM|PG_WRITE|PG_NC|PG_MMEM| PA_PGNUM(dst);
    set_pte(tmp_vpages[1], pte);
    bcopy((char *) tmp_vpages[0], (char *) tmp_vpages[1], NBPG);
    set_pte(tmp_vpages[0], PG_INVAL);
    set_pte(tmp_vpages[0], PG_INVAL);
    PMAP_DB_UNLK();
    PMAP_UNLOCK();
}
/*
 *	Routine:	pmap_extract
 *	Function:
 *		Extract the physical page address associated
 *		with the given map/virtual_address pair.
 */
vm_offset_t
pmap_extract(pmap, va)
     pmap_t	pmap;
     vm_offset_t va;
{
    unsigned char sme;
    unsigned int seg;
    vm_offset_t pte;
    int s;

    PMAP_LOCK();
    PMAP_DB_LOCK();
    if (pmap == kernel_pmap) {
	sme = get_segmap(va);
	if (sme == SEGINV)
	    panic("pmap: pmap_extract() failed on kernel va");
	pte = get_pte(va);
	if (pte & PG_VALID)
	    goto valid;
	panic("pmap: pmap_extract() failed on invalid kernel va");
    }
    seg = VA_SEGNUM(va);
    if (pmap->pm_segmap[seg] == SEGINV) 
    	panic("pmap: pmap_extract() failed on user va");
    if (get_pte_val(pmap, va,&pte)) {
	if (pte & PG_VALID)
	    goto valid;
    }
    panic("pmap: pmap_extract() failed on invalid user va");
 valid:
    PMAP_DB_UNLK();
    PMAP_UNLOCK();
    return PG_PA(pte);
}
/*
 *	Routine:	pmap_pageable
 *	Function:
 *		Make the specified pages (by pmap, offset)
 *		pageable (or not) as requested.
 *
 *		A page which is not pageable may not take
 *		a fault; therefore, its page table entry
 *		must remain valid for the duration.
 *
 *		This routine is merely advisory; pmap_enter
 *		will specify that these pages are to be wired
 *		down (or not) as appropriate.
 */
void pmap_pageable(pmap, sva, eva, pageable)
	pmap_t		pmap;
	vm_offset_t	sva, eva;
	boolean_t	pageable;
{
/* not implemented, hopefully not needed */
}

vm_offset_t
pmap_phys_address(ppn)
	int ppn;
{
    vm_offset_t pa;

    pa = sun3_ptob(ppn);
    return pa;
}

/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */
void
pmap_pinit(pmap)
     pmap_t pmap;
{
    pmap_common_init(pmap);
    pmap_user_pmap_init(pmap);
}

void pmap_protect_range_contextless(pmap, sva, eva,pte_proto, pmegp)
     pmap_t pmap;
     vm_offset_t sva, eva;
     vm_offset_t pte_proto;
     pmeg_t pmegp;
{
    int sp, ep, i, s;
    unsigned char nflags;
    vm_offset_t pte,va;

    sp = VA_PTE_NUM(sva);
    ep = VA_PTE_NUM(eva);
    va = sva;
    for (i = sp; i < ep; i++) {
	pte = get_pte_pmeg(pmegp->pmeg_index, i);
	if (pte & PG_VALID) {
	    if (pv_initialized)
		save_modified_bits(pte);
	    pte_proto |= (PG_MOD|PG_SYSTEM|PG_TYPE|PG_ACCESS|PG_FRAME) & pte;
	    nflags = pv_link(pmap, PG_PA(pte), va, PG_TO_PV_FLAGS(pte_proto));
	    if (nflags & PV_NC)
		set_pte_pmeg(pmegp->pmeg_index, i, pte_proto|PG_NC);
	    else 
		set_pte_pmeg(pmegp->pmeg_index, i, pte_proto);
	}
	va+=NBPG;
    }
}

void pmap_protect_range_mmu(pmap, sva, eva, pte_proto)
     pmap_t pmap;
     vm_offset_t sva, eva;
     vm_offset_t pte_proto;
{
    int saved_context;
    unsigned int sme,nflags;
    pmeg_t pmegp;
    vm_offset_t va,pte;
    
    saved_context = get_context();
    if (pmap != kernel_pmap)
	set_context(pmap->pm_context->context_num);
    sme = get_segmap(sva);
    if (sme != SEGINV) {
	pmegp = pmeg_p(sme);
#ifdef	DIAGNOSTIC
	/* Make sure it is in our software segmap (cache). */
	if (pmap->pm_segmap[VA_SEGNUM(sva)] != sme)
	    panic("pmap_protect_range_mmu: MMU has bad pmeg %x", sme);
#endif
    } else {
	if (pmap == kernel_pmap)
	    return;
	pmegp = pmeg_cache(pmap, sun3_trunc_seg(sva));
	if (!pmegp)
	    goto out;
#ifdef	PMAP_DEBUG
	if (pmap_debug & PMD_SEGMAP) {
	    printf("pmap: set_segmap ctx=%d v=%x old=ff new=%x (rp1)\n",
		   get_context(), sun3_trunc_seg(sva), pmegp->pmeg_index);
	}
#endif
	set_segmap(sva, pmegp->pmeg_index);
    }
    /* have pmeg, will travel */

    if (pmegp->pmeg_vpages <= 0)
	return; /* no valid pages anyway */
    va = sva;
    while (va < eva) {
	pte = get_pte(va);
	if (pte & PG_VALID) {
	    if (pv_initialized)
		save_modified_bits(pte);
	    pte_proto = (pte_proto & (PG_VALID|PG_WRITE)) |
		((PG_MOD|PG_SYSTEM|PG_TYPE|PG_ACCESS|PG_FRAME) & pte);
	    nflags = pv_link(pmap, PG_PA(pte), va, PG_TO_PV_FLAGS(pte_proto));
	    if (nflags & PV_NC)
		set_pte(va, pte_proto | PG_NC);
	    else 
		set_pte(va, pte_proto);
	}
	va+= NBPG;
    }
 out:
    set_context(saved_context);
}
				/* within one pmeg */
void pmap_protect_range(pmap, sva, eva, pte_proto)
     pmap_t	pmap;
     vm_offset_t	sva, eva;
     vm_offset_t pte_proto;
{
    pmeg_t pmegp;

    if (pmap != kernel_pmap) {
	if (pmap->pm_segmap == NULL) {
#ifdef	PMAP_DEBUG
	    if (pmap_debug & PMD_SEGMAP) {
		printf("pmap_protect_range: null segmap\n");
		Debugger(); /* XXX */
	    }
#endif
	    return;
	}
	if (get_pmeg_cache(pmap, VA_SEGNUM(sva)) == SEGINV)
	    return;
    }

    if ((pmap == kernel_pmap) || (pmap->pm_context))
	pmap_protect_range_mmu(pmap, sva, eva,pte_proto);
    else {
	pmegp = pmeg_cache(pmap, sun3_trunc_seg(sva));
	if (!pmegp) return;
	pmap_protect_range_contextless(pmap, sva, eva, pte_proto, pmegp);
    }
}
/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 */
void
pmap_protect(pmap, sva, eva, prot)
	pmap_t	pmap;
	vm_offset_t	sva, eva;
	vm_prot_t	prot;
{
    vm_offset_t pte_proto, va, neva;
    int s;
    
    if (pmap == NULL) return;
    if (pmap == kernel_pmap) {
	if (sva < VM_MIN_KERNEL_ADDRESS)
	    sva = VM_MIN_KERNEL_ADDRESS;
	if (eva > VM_MAX_KERNEL_ADDRESS)
	    eva = VM_MAX_KERNEL_ADDRESS;
    }
    else {
	if (eva > VM_MAX_ADDRESS)
	    eva = VM_MAX_ADDRESS;
    }
#ifdef	PMAP_DEBUG
    if (pmap_debug & PMD_PROTECT)
	printf("pmap_protect(%x, %x, %x, %x)\n", pmap, sva, eva, prot);
#endif
    if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
	pmap_remove(pmap, sva, eva);
	return;
    }
    PMAP_LOCK();
    PMAP_DB_LOCK();
    pte_proto = pmap_pte_prot(prot);
    va = sva;
    while (va < eva) {
	neva = sun3_round_up_seg(va);
	if (neva > eva)
	    neva = eva;
	pmap_protect_range(pmap, va, neva, pte_proto);
	va = neva;
    }
    PMAP_DB_UNLK();
    PMAP_UNLOCK();
}

/*
 *	Require that all active physical maps contain no
 *	incorrect entries NOW.  [This update includes
 *	forcing updates of any address map caching.]
 *
 *	Generally used to insure that a thread about
 *	to run will see a semantically correct world.
 */
void pmap_update()
{
}
/*
 *	pmap_zero_page zeros the specified (machine independent)
 *	page by mapping the page into virtual memory and using
 *	bzero to clear its contents, one machine dependent page
 *	at a time.
 */
void pmap_zero_page(pa)
	 vm_offset_t	pa;
{
    vm_offset_t pte;
    int s;

#ifdef	PMAP_DEBUG
    if (pmap_debug & PMD_COW)
	printf("pmap_zero_page: %x\n", pa);
#endif
    PMAP_LOCK();
    pte = PG_VALID |PG_SYSTEM|PG_WRITE|PG_NC|PG_MMEM| PA_PGNUM(pa);
    set_pte(tmp_vpages[0], pte);
    bzero((char *) tmp_vpages[0], NBPG);
    set_pte(tmp_vpages[0], PG_INVAL);
    PMAP_UNLOCK();
}

/*
 * Local Variables:
 * tab-width: 8
 * End:
 */
