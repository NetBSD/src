/*
 * Copyright (c) 1993 Adam Glass
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
 * $Header: /cvsroot/src/sys/arch/sun3/sun3/pmap.c,v 1.16 1993/12/12 09:08:51 glass Exp $
 */
#include "systm.h"
#include "param.h"
#include "proc.h"
#include "malloc.h"
#include "user.h"

#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"
#include "vm/vm_statistics.h"

#include "machine/pte.h"
#include "machine/control.h"
    
#include "machine/cpu.h"
#include "machine/mon.h"
#include "machine/vmparam.h"
#include "machine/pmap.h"

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

#define splpmap splbio

#ifdef CONFUSED
#define PMAP_LOCK() s = splpmap()
#define PMAP_UNLOCK() splx(s)
#else
#define PMAP_LOCK()		/* pmap lock */
#define PMAP_UNLOCK()		/* pmap unlock */
#endif

#define dequeue_first(val, type, queue) { \
	val = (type) queue_first(queue); \
	    dequeue_head(queue); \
}

/*
 * locking issues:
 *
 */

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
#define pa_to_pvp(pa) &pv_head_table[PA_PGNUM(pa)]

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

#define save_modified_bits(pte) \
    pv_modified_table[PG_PGNUM(pte)] |= \
    (pte & (PG_OBIO|PG_VME16D|PG_VME32D) ? 0 : \
     ((pte & PG_MOD) >>PG_MOD_SHIFT))

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
queue_head_t pmeg_free_queue;
queue_head_t pmeg_inactive_queue;
queue_head_t pmeg_active_queue;
#define pmeg_p(x) &pmeg_array[x]
#define is_pmeg_wired(pmegp) (pmegp->pmeg_wired_count)
static struct pmeg_state pmeg_array[NPMEG];

/* context structures, and queues */
queue_head_t context_free_queue;
queue_head_t context_active_queue;

static struct context_state context_array[NCONTEXT];

/* location to store virtual addresses
 * to be used in copy/zero operations
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
void pmeg_release_empty __P((pmeg_t pmegp, int segnum, int update));
pmeg_t pmeg_cache __P((pmap_t pmap, vm_offset_t va, int update));
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

int get_pte_val(pmap, va, ptep)
     pmap_t pmap;
     vm_offset_t va, *ptep;
{
    int saved_context,s;
    unsigned char sme;
    pmeg_t pmegp;
    int module_debug;

    if (pmap->pm_context) {
	PMAP_LOCK();
	saved_context = get_context();
	set_context(pmap->pm_context->context_num);
	sme = get_segmap(va);
	if (sme == SEGINV) {
	    PMAP_UNLOCK();
	    return 0;
	}
	*ptep = get_pte(va);
	set_context(saved_context);
    } else {
	/* we don't have a context */
	pmegp = pmeg_cache(pmap, sun3_trunc_seg(va), PM_UPDATE_CACHE);
	if (!pmegp) {
	    PMAP_UNLOCK();
	    return 0;
	}
	*ptep = get_pte_pmeg(pmegp->pmeg_index, VA_PTE_NUM(va));
	pmeg_release(pmegp);
    }
    PMAP_UNLOCK();
    return 1;
}

void set_pte_val(pmap, va,pte)
     pmap_t pmap;
     vm_offset_t va,pte;
{
    int saved_context, s;
    pmeg_t pmegp;
    unsigned char sme;

    PMAP_LOCK();
    if (pmap->pm_context) {
	saved_context = get_context();
	set_context(pmap->pm_context->context_num);
	sme = get_segmap(va);
	if (sme == SEGINV) goto outta_here;
	set_pte(va, pte);
	set_context(saved_context);
    }
    else {
    /* we don't have a context */
	pmegp = pmeg_cache(pmap, sun3_trunc_seg(va), PM_UPDATE_CACHE);
	if (!pmegp) panic("pmap: no pmeg to set pte in");
	set_pte_pmeg(pmegp->pmeg_index, VA_PTE_NUM(va), pte);
	pmeg_release(pmegp);
    }
outta_here:
    PMAP_UNLOCK();
}

void context_allocate(pmap)
     pmap_t pmap;
{
    context_t context;
    int s;

    PMAP_LOCK();
#ifdef CONTEXT_DEBUG    
    printf("context_allocate: for pmap %x\n", pmap);
#endif
    if (pmap->pm_context)
	panic("pmap: pmap already has context allocated to it");
    if (queue_empty(&context_free_queue)) { /* steal one from active*/
	if (queue_empty(&context_active_queue))
	    panic("pmap: no contexts to be found");
	dequeue_first(context, context_t, &context_active_queue);
	context_free(context->context_upmap);
#ifdef CONTEXT_DEBUG    
    printf("context_allocate: pmap %x, stealing context %x num %d\n",
	   pmap, context, context->context_num);
#endif
    } else 
	dequeue_first(context, context_t, &context_free_queue);	
    enqueue_tail(&context_active_queue,context);
    if (context->context_upmap != NULL)
	panic("pmap: context in use???");
    pmap->pm_context = context;
    context->context_upmap = pmap;
#ifdef CONTEXT_DEBUG    
    printf("context_allocate: pmap %x associated with context %x num %d\n",
	   pmap, context, context->context_num);
#endif
    PMAP_UNLOCK();
}
void context_free(pmap)		/* :) */
     pmap_t pmap;
{
    int saved_context, i, s;
    context_t context;

    vm_offset_t va;

    PMAP_LOCK();
#ifdef CONTEXT_DEBUG
    printf("context_free: freeing pmap %x of context %x num %d\n",
	   pmap, pmap->pm_context, pmap->pm_context->context_num);
#endif
    saved_context = get_context();
    if (!pmap->pm_context)
	panic("pmap: can't free a non-existent context");
    context = pmap->pm_context;
    set_context(context->context_num);
    va = 0;
    for (i=0; i < NUSEG; i++) {
	set_segmap(va, SEGINV);
	if (pmap->pm_segmap[i] != SEGINV) 
	    pmeg_release(pmeg_p(pmap->pm_segmap[i]));
	va += NBSG;
    }
    set_context(saved_context);
    context->context_upmap = NULL;
    enqueue_tail(&context_free_queue, context);
    pmap->pm_context = NULL;
#ifdef CONTEXT_DEBUG
    printf("context_free: pmap %x context removed\n", pmap);
#endif
    PMAP_UNLOCK();
}

void context_init()
{
    int i;

    queue_init(&context_free_queue);
    queue_init(&context_active_queue);

    for (i=0; i <NCONTEXT; i++) {
	context_array[i].context_num = i;
	context_array[i].context_upmap = NULL;
	enqueue_tail(&context_free_queue, (queue_entry_t) &context_array[i]);
#ifdef CONTEXT_DEBUG
	printf("context_init: context num %d is %x\n", i, &context_array[i]);
#endif
    }
}

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
    pmegp->pmeg_owner = NULL;
    pmegp->pmeg_reserved++;	/* keep count, just in case */
    remqueue(&pmeg_free_queue, pmegp);
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
    int loop = 0;

    if (queue_empty(&pmeg_free_queue))
	panic("pmap: no free pmegs available to clean");

    pmegp_first = NULL;
    dequeue_first(pmegp, pmeg_t, &pmeg_free_queue);
    while (pmegp != pmegp_first) {
	pmeg_clean(pmegp);
	if (pmegp_first == NULL)
	    pmegp_first = pmegp;
	enqueue_tail(&pmeg_free_queue, pmegp);
	dequeue_first(pmegp, pmeg_t, &pmeg_free_queue);
    }
}
void pmeg_flush(pmegp)
     pmeg_t pmegp;
{
    vm_offset_t pte;
    int i;
    
/*    if (!pmegp->pmeg_vpages)
	printf("pmap: pmeg_flush() on clean pmeg\n");*/
    for (i = 0; i < NPAGSEG; i++) {
	if (pmegp->pmeg_owner) {
	    pte = get_pte_pmeg(pmegp->pmeg_index, i);
	    if (pte & PG_VALID) {
		if (pv_initialized)
		    save_modified_bits(pte);
		pv_unlink(pmegp->pmeg_owner, PG_PA(pte), pmegp->pmeg_va);
		pmegp->pmeg_vpages--;
		set_pte_pmeg(pmegp->pmeg_index, i, PG_INVAL);
	    }
	}
	else {
	    pmegp->pmeg_vpages = 0;
	    set_pte_pmeg(pmegp->pmeg_index, i, PG_INVAL);
	}
    }
    if (pmegp->pmeg_vpages != 0)
	printf("pmap: pmeg_flush() didn't result in a clean pmeg\n");
}

pmeg_t pmeg_allocate_invalid(pmap, va)
     pmap_t pmap;
     vm_offset_t va;
{
    pmeg_t pmegp;    

    if (!queue_empty(&pmeg_free_queue)) 
	dequeue_first(pmegp, pmeg_t, &pmeg_free_queue)
    else if (!queue_empty(&pmeg_inactive_queue)) {
	dequeue_first(pmegp, pmeg_t, &pmeg_inactive_queue);
	pmeg_flush(pmegp);
    }
    else if (!queue_empty(&pmeg_active_queue)) {
	dequeue_first(pmegp, pmeg_t, &pmeg_active_queue);
	while (pmegp->pmeg_owner == kernel_pmap) {
	    enqueue_tail(&pmeg_active_queue, pmegp);
	    dequeue_first(pmegp, pmeg_t, &pmeg_active_queue);
	}
	pmap_remove_range(pmegp->pmeg_owner, pmegp->pmeg_va,
			  pmegp->pmeg_va+NBSG);
    } else
	panic("pmeg_allocate_invalid: failed\n");
    if (!pmegp)
	panic("pmeg_allocate_invalid: unable to allocate pmeg");
    pmegp->pmeg_owner = pmap;
    pmegp->pmeg_owner_version = pmap->pm_version;
    pmegp->pmeg_va = va;
    pmegp->pmeg_wired_count = 0;
    pmegp->pmeg_reserved  = 0;
    pmegp->pmeg_vpages  = 0;
    enqueue_tail(&pmeg_active_queue, pmegp);
    return pmegp;
}

void pmeg_release(pmegp)
     pmeg_t pmegp;
{
    remqueue(&pmeg_active_queue, pmegp);
    enqueue_tail(&pmeg_inactive_queue, pmegp);
}

void pmeg_release_empty(pmegp, segnum, update)
     pmeg_t pmegp;
     int segnum;
     int update;
{
    remqueue(&pmeg_active_queue, pmegp); /*   XXX bug here  */
    remqueue(&pmeg_inactive_queue, pmegp); /* XXX */
    enqueue_tail(&pmeg_free_queue, pmegp);
    if (update && (pmegp->pmeg_owner != kernel_pmap))
	pmegp->pmeg_owner->pm_segmap[segnum] = SEGINV;
}

pmeg_t pmeg_cache(pmap, va, update)
     pmap_t pmap;
     vm_offset_t va;
     int update;
{
    unsigned char seg;
    pmeg_t pmegp;

    if (!pmap->pm_segmap || (pmap == kernel_pmap)) return PMEG_NULL;
    seg = VA_SEGNUM(va);
    if (seg < NUSEG)
	return PMEG_NULL;
    if (pmap->pm_segmap[seg] == SEGINV)
	return PMEG_NULL;
    if (update)
	pmap->pm_segmap[seg] = SEGINV;
    pmegp = pmeg_p(pmap->pm_segmap[seg]);

    if ((pmegp->pmeg_owner != pmap) || 
	(pmegp->pmeg_owner_version != pmap->pm_version) ||
	(pmegp->pmeg_va != va)) return PMEG_NULL; /* cache lookup failed */
    remqueue(&pmeg_inactive_queue, pmegp);
    enqueue_tail(&pmeg_active_queue, pmegp);
    if (update)
	pmap->pm_segmap[seg] = pmegp->pmeg_index;
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

    queue_init(&pmeg_free_queue);
    queue_init(&pmeg_inactive_queue);    
    queue_init(&pmeg_active_queue);    

    bzero(pmeg_array, NPMEG*sizeof(struct pmeg_state));
    for (x =0 ; x<NPMEG; x++) {
	enqueue_tail(&pmeg_free_queue,(queue_entry_t) &pmeg_array[x]);
	pmeg_array[x].pmeg_index = x;
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
	if (pv->pv_pmap->pm_segmap[seg] == SEGINV) continue;
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
    pv_entry_t npv,head,last;

    if (!pv_initialized) return;
    head = pa_to_pvp(pa);
    for (npv = head ; npv != NULL; last= npv, npv = npv->pv_next ) {
	if (npv->pv_pmap == NULL) continue; /* empty */
	pmap_remove_range(npv->pv_pmap, npv->pv_va, npv->pv_va+NBPG);
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

    if (!pv_initialized) return 0;
    head = pv = pa_to_pvp(pa);
    PMAP_LOCK();

    if (pv->pv_pmap == NULL) {	/* not currently "managed" */
 	pv->pv_va = va;
	pv->pv_pmap = pmap,
	pv->pv_next = NULL;
	pv->pv_flags = flags;
	force_cache_flags(pa, flags);
	return flags & PV_NC;
    }
    for (npv = pv ; npv != NULL; last= npv, npv = npv->pv_next ) {
	if ((npv->pv_pmap != pmap) || (npv->pv_va != va)) continue;
	if (flags == npv->pv_flags) /* no change */
	    return get_cache_flags(pa);
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
	return flags & PV_NC;
    }
    nflags = pv_compute_cache(head);
    force_cache_flags(pa, nflags);
    pv_change_pte(head, MAKE_PV_REAL(nflags), 0); /*  */
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
	if (pv->pv_pmap->pm_segmap[seg] == SEGINV) continue;
	if (!get_pte_val(pv->pv_pmap, pv->pv_va, &pte)) continue;
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
    pv_list = pa_to_pvp(pa);
    if (pv_list->pv_pmap == NULL) {
	printf("pv_unlinking too many times\n");
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
    panic("pv_unlink: couldn't find entry");
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
    vm_offset_t temp_seg, va, eva, pte;
    extern vm_size_t page_size;
    extern void vm_set_page_size();
    unsigned int sme;

    page_size = NBPG;
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
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void
pmap_init(phys_start, phys_end)
	vm_offset_t	phys_start, phys_end;
{
    vm_offset_t tmp,end;
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
    pmap = (pmap_t) malloc(sizeof(struct pmap), M_VMPMAP, M_WAITOK);
    pmap_common_init(pmap);
    pmap_user_pmap_init(pmap);
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
    
    if (pmap->pm_context)
	context_free(pmap);
    free(pmap->pm_segmap, M_VMPMAP);
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

    if (pmap == NULL) return;
    if (pmap == kernel_pmap)
	panic("pmap: attempted to destroy kernel_pmap");
    pmap_lock(pmap);
    count = pmap_del_ref(pmap);
    pmap_unlock(pmap);
    if (count == 0) {
	pmap_release(pmap);
	free((caddr_t)pmap, M_VMPMAP); /* XXXX -- better make sure we
					  it this way allocate */
    }
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
    PMAP_UNLOCK();
}

/*
 *	Add a reference to the specified pmap.
 */
void
pmap_reference(pmap)
	pmap_t	pmap;
{
    if (pmap != NULL) {
	pmap_lock(pmap);
	pmap_add_ref(pmap);
	pmap_unlock(pmap);
    }
}
void pmap_remove_range_mmu(pmap, sva, eva)
     pmap_t pmap;
     vm_offset_t sva, eva;
{
    int saved_context,i;
    unsigned int sme;
    pmeg_t pmegp;
    vm_offset_t va,pte;
    
    saved_context = get_context();
    if (pmap != kernel_pmap) 
	set_context(pmap->pm_context->context_num);

    sme = get_segmap(sva);
    if (sme == SEGINV) {
	if (pmap == kernel_pmap) return;
	pmegp = pmeg_cache(pmap, VA_SEGNUM(sva), PM_UPDATE_CACHE);
	if (!pmegp) goto outta_here;
	set_segmap(sva, pmegp->pmeg_index);
    }
    else
	pmegp = pmeg_p(sme);
    /* have pmeg, will travel */
    if (!pmegp->pmeg_vpages) goto outta_here; /* no valid pages anyway */

    va = sva;
    while (va < eva) {
	pte = get_pte(va);
	if (pte & PG_VALID) {
	    if (pv_initialized)
		save_modified_bits(pte);
	    pv_unlink(pmap, PG_PA(pte), va);
	    pmegp->pmeg_vpages--;
	    set_pte(va, PG_INVAL);
	}
	va+= NBPG;
    }
    if (pmegp->pmeg_vpages <= 0) {
	if (is_pmeg_wired(pmegp))
	    printf("pmap: removing wired pmeg\n");
	pmeg_release_empty(pmegp, VA_SEGNUM(sva), PM_UPDATE_CACHE);
	if (kernel_pmap == pmap) {
	    for (i=0; i < NCONTEXT; i++) { /* map out of all segments */
		set_context(i);
		set_segmap(sva,SEGINV);
	    }
	}
	else
	    set_segmap(sva, SEGINV);
    }
outta_here:
    set_context(saved_context);
}

void pmap_remove_range_contextless(pmap, sva, eva,pmegp)
     pmap_t pmap;
     vm_offset_t sva, eva;
     pmeg_t pmegp;
{
    int sp,ep,i;
    vm_offset_t pte,va;

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
	    printf("pmap: removing wired pmeg\n");
	pmeg_release_empty(pmegp,VA_SEGNUM(sva), PM_UPDATE_CACHE);
    }
    else pmeg_release(pmegp);
}
/* guaraunteed to be within one pmeg */

void pmap_remove_range(pmap, sva, eva)
     pmap_t pmap;
     vm_offset_t sva, eva;
{
    pmeg_t pmegp;
   /* cases: kernel: always has context, always available
    *
    *        user: has context, is available
    *        user: has no context, is available
    *        user: has no context, is not available (NOTHING) |_ together
    *        user: has context, isn't available (NOTHING)     |
    */
            
    if ((pmap != kernel_pmap))
	if (get_pmeg_cache(pmap, VA_SEGNUM(sva)) == SEGINV) return;
    if ((pmap == kernel_pmap) ||
 	(pmap->pm_context)) {
	pmap_remove_range_mmu(pmap, sva, eva);
	return;
    }
    /* we are a user pmap without a context, possibly without a pmeg to
     * operate upon
     *
     */
    pmegp = pmeg_cache(pmap, sun3_trunc_seg(sva),PM_UPDATE_CACHE);
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
    }
    else {
	if (eva > VM_MAX_ADDRESS)
	    eva = VM_MAX_ADDRESS;
    }
    PMAP_LOCK();
    va = sva;
    pmegp = NULL;
    while (va < eva) {
	neva = sun3_round_up_seg(va);
	if (neva > eva)
	    neva = eva;
	pmap_remove_range(pmap, va, neva);
	va = neva;
    }
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
    unsigned int pmeg;
    vm_offset_t current_pte;
    unsigned char sme,nflags;
    int c;
    pmeg_t pmegp;
    /*
      keep in hardware only, since its mapped into all contexts anyway;
      need to handle possibly allocating additional pmegs
      need to make sure they cant be stolen from the kernel;
      map any new pmegs into all contexts, make sure rest of pmeg is null;
      deal with pv_stuff; poossibly caching problems;
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
	panic("pmap: kernel trying to allocate virtual space below itself\n");
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
	goto add_pte;
    }
    else 
	pmegp = pmeg_p(sme);

    /* does mapping already exist */
    /*    (a) if so, is it same pa then really a protection change
     *    (b) if not same, pa then we have to unlink from old pa
     *    (c) 
     */
    current_pte = get_pte(va);
    if (!(current_pte & PG_VALID)) goto add_pte; /* just adding */
    pmegp->pmeg_vpages--;	/* removing valid page here, way lame XXX*/
    if (pv_initialized)
	save_modified_bits(current_pte);
    if (PG_PGNUM(current_pte) != PG_PGNUM(pte_proto)) /* !same physical addr*/
	pv_unlink(kernel_pmap, PG_PA(current_pte), va); 

add_pte:	/* can be destructive */
    if (wired && !is_pmeg_wired(pmegp)) {
	kernel_pmap->pm_stats.wired_count++;
	pmeg_wire(pmegp);
    }
    else if (!wired && is_pmeg_wired(pmegp)) {
	kernel_pmap->pm_stats.wired_count--;
	pmeg_unwire(pmegp);
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
    if (va < VM_MIN_ADDRESS)
	panic("pmap: user trying to allocate virtual space below itself\n");
    if ((va+NBPG) > VM_MAXUSER_ADDRESS)
	panic("pmap: user trying to allocate virtual space above itself\n");
    if (wired)
	printf("pmap_enter_user: attempt to wire user page, ignored\n");
    pte_proto |= MAKE_PGTYPE(PG_MMEM); /* unnecessary */
    PMAP_LOCK();
    saved_context = get_context();
    if (!pmap->pm_context)
	panic("pmap: pmap_enter_user() on pmap without a context");
    if (saved_context != pmap->pm_context->context_num)
	set_context(pmap->pm_context->context_num);

    sme = get_segmap(va);
    if (sme == SEGINV) {
	pmegp = pmeg_cache(pmap, sun3_trunc_seg(va),PM_UPDATE_CACHE);
	if (!pmegp)		/* no cached pmeg */
	    pmegp = pmeg_allocate_invalid(pmap, va);
	set_segmap(va,pmegp->pmeg_index);
    } else 
	pmegp = pmeg_p(sme);

    /* does mapping already exist */
    /*    (a) if so, is it same pa then really a protection change
     *    (b) if not same, pa then we have to unlink from old pa
     *    (c) 
     */
    current_pte = get_pte(va);
    if (!(current_pte & PG_VALID)) goto add_pte;
    if (pv_initialized)
	save_modified_bits(current_pte);
    pmegp->pmeg_vpages--;	/* removing valid page here, way lame XXX*/
    if (PG_PGNUM(current_pte) != PG_PGNUM(pte_proto)) 
	pv_unlink(pmap, PG_PA(current_pte), va); 

add_pte:
    /* if we did wiring on user pmaps, then the code would be here */
    nflags = pv_link(pmap, pa, va, PG_TO_PV_FLAGS(pte_proto));
    if (nflags & PV_NC)
	set_pte(va, pte_proto | PG_NC);
    else
	set_pte(va, pte_proto);
    pmegp->pmeg_vpages++;	/* assumes pmap_enter can never insert
				   a non-valid page */
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
    if (pmap == kernel_pmap)
	pmap_enter_kernel(va, pa, prot, wired, pte_proto, mem_type);
    else
	pmap_enter_user(pmap, va, pa, prot, wired, pte_proto, mem_type);
    PMAP_UNLOCK();
}

void
pmap_clear_modify(pa)
     vm_offset_t	pa;
{
    int s;

    if (!pv_initialized) return;

    PMAP_LOCK();
    pv_modified_table[PA_PGNUM(pa)] = 0;
    pv_change_pte(&pv_head_table[PA_PGNUM(pa)], 0, PG_MOD);
    PMAP_UNLOCK();
}
boolean_t
pmap_is_modified(pa)
     vm_offset_t	pa;
{
    int s;

    if (!pv_initialized) return 0;
    if (pv_modified_table[PA_PGNUM(pa)]) return 1;
    PMAP_LOCK();
    pv_modified_table[PA_PGNUM(pa)] = pv_compute_modified(pa_to_pvp(pa));
    PMAP_UNLOCK();
    return pv_modified_table[PA_PGNUM(pa)];
}

void pmap_clear_reference(pa)
     vm_offset_t	pa;
{
    int s;
    
    if (!pv_initialized) return;
    PMAP_LOCK();
    pv_remove_all(pa);
    PMAP_UNLOCK();
}

boolean_t
pmap_is_referenced(pa)
     vm_offset_t	pa;
{
    return FALSE;
}


void pmap_activate(pmap, pcbp)
     pmap_t pmap;
     struct pcb *pcbp;
{
    int s;

    PMAP_LOCK();
    if (pmap->pm_context) {
	set_context(pmap->pm_context->context_num);
	PMAP_UNLOCK();
	return;
    }
    context_allocate(pmap);
    set_context(pmap->pm_context->context_num);
    PMAP_UNLOCK();
}
void pmap_deactivate(pmap, pcbp)
     pmap_t pmap;
     struct pcb *pcbp;
{
    enqueue_tail(&context_active_queue,
		 (queue_entry_t) pmap->pm_context);
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
    printf("pmap: call to pmap_change_wiring().  ignoring.\n");
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

    PMAP_LOCK();
    pte = PG_VALID |PG_SYSTEM|PG_WRITE|PG_NC|PG_MMEM| PA_PGNUM(src);
    set_pte(tmp_vpages[0], pte);
    pte = PG_VALID |PG_SYSTEM|PG_WRITE|PG_NC|PG_MMEM| PA_PGNUM(dst);
    set_pte(tmp_vpages[1], pte);
    bcopy((char *) tmp_vpages[0], (char *) tmp_vpages[1], NBPG);
    set_pte(tmp_vpages[0], PG_INVAL);
    set_pte(tmp_vpages[0], PG_INVAL);
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

void pmap_protect_range_mmu(pmap, sva, eva,pte_proto)
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
    if (sme == SEGINV) {
	if (pmap == kernel_pmap) return;
	pmegp = pmeg_cache(pmap, VA_SEGNUM(sva), PM_UPDATE_CACHE);
	if (!pmegp) return;
	set_segmap(sva, pmegp->pmeg_index);
    }
    else
	pmegp = pmeg_p(sme);
    /* have pmeg, will travel */
    if (!pmegp->pmeg_vpages) return; /* no valid pages anyway */
    va = sva;
    while (va < eva) {
	pte = get_pte(va);
	if (pte & PG_VALID) {
	    if (pv_initialized)
		save_modified_bits(pte);
	    pte_proto |= (PG_MOD|PG_SYSTEM|PG_TYPE|PG_ACCESS|PG_FRAME) & pte;
	    nflags = pv_link(pmap, PG_PA(pte), va, PG_TO_PV_FLAGS(pte_proto));
	    if (nflags & PV_NC)
		set_pte(va, pte_proto | PG_NC);
	    else 
		set_pte(va, pte_proto);
	}
	va+= NBPG;
    }
    set_context(saved_context);
}
				/* within one pmeg */
void pmap_protect_range(pmap, sva, eva, pte_proto)
     pmap_t	pmap;
     vm_offset_t	sva, eva;
     vm_offset_t pte_proto;
{
    pmeg_t pmegp;

    if ((pmap != kernel_pmap))    
	if (get_pmeg_cache(pmap, VA_SEGNUM(sva)) == SEGINV) return;
    if ((pmap == kernel_pmap) ||
 	(pmap->pm_context)) 
	pmap_protect_range_mmu(pmap, sva, eva,pte_proto);
    else {
	pmegp = pmeg_cache(pmap, sun3_trunc_seg(sva),PM_UPDATE_CACHE);
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
    if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
	pmap_remove(pmap, sva, eva);
	return;
    }
    PMAP_LOCK();
    pte_proto = pmap_pte_prot(prot);
    va = sva;
    while (va < eva) {
	neva = sun3_round_up_seg(va);
	if (neva > eva)
	    neva = eva;
	pmap_protect_range(pmap, va, neva, pte_proto);
	va = neva;
    }
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

    PMAP_LOCK();
    pte = PG_VALID |PG_SYSTEM|PG_WRITE|PG_NC|PG_MMEM| PA_PGNUM(pa);
    set_pte(tmp_vpages[0], pte);
    bzero((char *) tmp_vpages[0], NBPG);
    set_pte(tmp_vpages[0], PG_INVAL);
    PMAP_UNLOCK();
}


