#include "../include/pmap.h"

/* current_projects:
 * 
 * prototypes
 *
 * debugging support
 *
 * use pmap_remove when stealing pmap's ptes????
 *
 * need to write/eliminate use of/fix:
 * {get,ste}_pte_pmeg();
 * get_pte_val(pv->pv_pmap, pv->pv_va,&pte))
 * get_pte_if_possible(npv->pv_pmap->pm_va, &pte))
 * set_pte_if_possible(npv->pv_pmap->pm_va, PG_INVAL);
 * why can't pmap_remove be used for pv_remove_all(), or too expensive?
 * but in pv_remove_all, no non-context case, same with pv_changepte
 * does removing part of a non-contexted pmap result in it re-aquiring active
 * pmegs
 * use of zones in pv_stuff
 * set_cache_flags(pa, flags)
 * initialize pv_cache_table
 * memavail problem
 * pmap_bootstrap needs to be completed, and fixed.
 * pmap_init does nothing with its arguments....
 * locking protocosl
 * pmap_page_protect needs to be filled out
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

struct pv_entry {
    struct pv_entry *pv_next;
    pmap_t          *pv_pmap;
    vm_offset_t      pv_va;
    unsigned char    pv_flags;
};

typedef struct pv_entry *pv_entry_t;
				/* macros */
pv_entry_t pv_head_table

static unsigned char *pv_cache_table = NULL;

static unsigned char *pv_modified_table = NULL;

#define save_modified_bits(pte) \
    (pte & (PG_OBIO|PG_VME16D|PG_VME32D) ? : \
     pv_modified_table[PG_PGNUM(pte)] |= ((pte & PG_MOD) >>PG_MOD_SHIFT))

int pmap_version = 1;

#define PMEG_INVAL (NPMEG-1)
#define pmap_lock(pmap) simple_lock(&pmap->pmc.pm_lock)
#define pmap_unlock(pmap) simple_unlock(&pmap->pmc.pm_lock)
#define pmap_add_ref(pmap) ++pmap->pmc.pm_refcount
#define pmap_del_ref(pmap) --pmap->pmc.pm_refcount
#define pmap_refcount(pmap) pmap->pmc.pm_refcount

#define PMEG_CACHE_UPDATE 1;
				/* external structures */
struct kern_pmap *kernel_pmap = NULL;

				/* static structures */
static struct kern_pmap kernel_pmap_store;
static unsigned int protection_converter[8];
static struct pmeg_state pmeg_array[NPMEG];
#define get_pmeg_cache(pmap, segnum) (pmap->pm_segmap[segnum])

#define pmap_pte_prot(x) protection_converter[x];

queue_head_t pmeg_free_queue;
queue_head_t pmeg_inactive_queue;
queue_head_t pmeg_active_queue;
#define pmeg_p(x) &pmeg_array[x]

queue_head_t context_free_queue;
queue_head_t context_active_queue;

static struct context_state context_array[NCONTEXT];

/* context support */

/* prototypes */
void context_allocate __P((pmap_t pmap));
void context_free __P((pmap_t pmap));
void context_init __P((void));

void pmeg_steal __P((int pmeg_num));
void pmeg_flush __P((pmeg_t pmegp));
pmeg_t pmeg_allocate_invalid __P((pmap_t pmap, vm_offset_t va));
void pmeg_release __P((pmet_tpmegp));
void pmeg_release_empty __P((pmeg_t pmegp, int segnum, int update));
pmeg_t pmeg_cache __P((pmap_t pmap, vm_offset_t va, int update));
void pmeg_init __P((void));

unsigned char pv_compute_cache __P((pv_entry_t head));
int pv_compute_modified __P((pv_entry_t head));
void pv_remove_all __P(( vm_offset_t pa));
unsigned char pv_link  __P((pmap_t pmap, vm_offset_t pa, vm_offset_t va, unsigned char flags));
void pv_changepte __P((pv_entry_t pv_list, vm_offset_t set_bits, vm_offset_t clear_bits));
void pv_unlink __P((pmap_t pmap, vm_offset_t pa, vm_offset_va));
void pv_init __P((void));     
void sun3_protection_init __P((void));

void pmap_bootstrap __P((vm_offset_t kernel_end, int hole));
void pmap_init __P((vm_offset_t phys_start, vm_offset_t phys_end));

void pmap_common_init __P((pmap_t pmap));
vm_offset_t pmap_map __P((vm_offset_t virt, vm_offset_t start, vm_offset_t end, int prot));
void pmap_user_pmap_init __P((pmap_t pmap));
pmap_t pmap_create ((vm_size_t size));
void pmap_release __P((pmap_t pmap));
void pmap_destroy __P((pmap_tpmap));
void pmap_reference __P((pmap_t pmap));

void pmap_page_protect __P((vm_offset_t pa, vm_prot_prot));

void pmap_remove_range_mmu __P((pmap_t pmap, vm_offset_t sva, vm_offset_teva));
void pmap_remove_range_contextless __P((pmap_t pmap, vm_offset_t sva, vm_offset_teva, pmeg_t pmegp));
void pmap_remove_range __P((pmap_t pmap, vm_offset_t sva, vm_offset_t eva));
void pmap_remove __P((pmap_t pmap, vm_offset_t sva, vm_offset_t eva));

void pmap_enter_kernel __P((vm_offset_t va, vm_offset_t pa, vm_prot_t prot, boolean_t wired, vm_offset_t pte_proto, vm_offset_t mem_type));
void pmap_enter_user __P((vm_offset_t va, vm_offset_t pa, vm_prot_t prot, boolean_t wired, vm_offset_t pte_proto, vm_offset_t mem_type));
void pmap_enter __P((vm_offset_t va, vm_offset_t pa, vm_prot_t prot, boolean_t wired);

void pmap_clear_modify __P((vm_offset_t pa));
boolean_t pmap_is_modified __P((vm_offset_t pa));

void pmap_clear_reference __P((vm_offset_t pa));
boolean_t pmap_is_referenced __P((vm_offset_t pa));

void pmap_activate __P((pmap_t pmap, struct pcb *pcbp, int is_curproc));
void pmap_deactivate __P((pmap_t pmap, struct pcb *pcbp));


void context_allocate(pmap)
     pmap_t pmap;
{
    context_t context;

    if (pmap->pm_context)
	panic("pmap: pmap already has context allocated to it");
    if (queue_empty(&context_free_queue)) { /* steal one from active*/
	if (queue_empty(&context_active_queue))
	    panic("pmap: no contexts to be found");
	context = dequeue_head(&context_active_queue);
	context_free(context->context_upmap);
    } else 
	context = dequeue_head(&context_free_queue);
    enqueue_tail(&context_active_queue);
    if (context->pmap != NULL)
	panic("pmap: context in use???");
    pmap->context_upmap = context;
    context->pmap = pmap;
}
void context_free(pmap)		/* :) */
     pmap_t pmap;
{
    int saved_context;
    context_t context;
    int i;
    vm_offset_t va;
    saved_context = get_context();
    if (!pmap->pm_context)
	panic("pmap: can't free a non-existent context");
    context = pmap->pm_context;
    set_context(context->context_num);
    va = 0;
    for (i=0; i < NUSEG; i++) {
	set_segmap(va, SEGINV);
	if (pm->pm_segmap[i] != SEGINV) 
	    pmeg_release(pmeg_p(pm->pm_segmap[i]));
    }
    set_context(saved_context);
    enqueue_tail(&context_free_queue, context);
    pmap->pm_context = NULL;
}
void context_init()
{
    int i;

    queue_init(&context_free_queue);
    queue_init(&context_busy_queue);

    for (i=0; i <NCONTEXT; i++) {
	context_array[i].context_num = i;
	context_array[i].upmap = NULL;
	enqueue_tail(&context_free_queue, (queue_entry_t) &context_array[i]);
    }
}

/* steal a pmeg without altering its mapping */

void pmeg_steal(pmeg_num)	
     int pmeg_num
{
    pmeg_t pmegp;    

    pmegp = pmeg_p(pmeg_num);
    if (pmegp->pmeg_reserved)
	panic("pmeg_steal: attempt to steal an already reserved pmeg");
    if (pmegp->pmeg_owner)
	panic("pmeg_steal: pmeg is already owned");
    pmegp->pmap = NULL;
    pmegp->pmeg_reserved++;	/* keep count, just in case */
    remqueue(&pmeg_free_queue, pmegp);
}

void pmeg_flush(pmegp)
     pmeg_t pmegp;
{
    vm_offset_t pte;

    if (!pmegp->pmeg_vpages)
	printf("pmap: pmeg_flush() on clean pmeg");
    for (i = 0; i < NPAGSEG; i++) {
	pte = get_pte_pmeg(pmegp->pmeg_index, i);
	if (pte & PG_V) {
	    save_modified_bits(pte);
	    pg_unlink(pmap, PG_PA(pte), va);
	    pmegp->pmeg_vpages--;
	    set_pte_pmeg(pmegp->pmeg_index, i, PG_INVAL);
	}
    }
    if (pmegp->pmeg_vpages != 0)
	printf("pmap: pmeg_flush() didn't result in a clean pmeg");
}

pmeg_t pmeg_allocate_invalid(pmap, va)
     pmap_t pmap;
     vm_offset_t va;
{
    pmeg_t pmegp;    

    if (!queue_empty(&pmeg_free_queue)) 
	pmegp = (pmeg_t) dequeue_head(&pmeg_free_queue);
    else if (!queue_empty(&pmeg_inactive_queue)){
	pmegp = dequeue_head(&pmeg_inactive_queue);
	pmeg_flush(pmegp);
    }
    else if (!queue_empty(&pmeg_active_queue)) {
	pmegp = dequeue_head(&pmeg_active_queue);
	while (pmegp->pmeg_pmap == kernel_pmap) {
	    enqueue_tail(&pmeg_inactive_queue, pmegp);
	    pmegp = dequeue_head(&pmeg_active_queue);
	}
	pmeg_flush(pmegp);
    }
    pmegp->pmeg_owner = pmap;
    pmegp->pmeg_owner_version = pmap->pm_version;
    pmegp->pmeg_va = va;
    pmegp->pmeg_wired_count = 0;
    pmegp->pmeg_reserved  = 0;
    pmegp->pmeg_vpages  = 0;
    pmegp = enqueue_tail(&pmeg_active_queue, pmegp);
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
    if (update && (pmegp->pmeg_pmap != kernel_pmap)
	pmegp->pmeg_pmap->pmap_segmap[segnum] = SEGINV;
}

pmeg_t pmeg_cache(pmap, va, update)
     pmap_t pmap;
     vm_offset_t va;
     int update;
{
    unsigned char seg;

    if (!pmap->pm_segmap || (pmap == kernel_pmap)) return PMEG_NULL;
    seg = VSEG_USER(va);
    if (pmap->pm_segmap[seg] == SEGINV) return PMEG_NULL;
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


void pmeg_init()
{
    int x;

    /* clear pmeg array, put it all on the free pmeq queue */

    queue_init(&pmeg_free_queue);
    queue_init(&pmeg_inactive_queue);    
    queue_init(&pmeg_active_queue);    

    bzero(pmeg_array, NPMEG*sizeof(struct pmeg_state));
    for (x =0 ; x<NPMEG; x++) {
	enqueue_tail(&pmeg_free_queue (queue_entry_t) &pmeg_array[x]);
	pmeg_array[x].pmeg_index = x;
    }
}

unsigned char pv_compute_cache(head)
     pv_entry_t head;
{
    pv_entry_t pv;
    unsigne char cflags;
    int cread, cwrite, ccache, clen;

    cread = cwrite = ccache = clen = 0;
    if (!pv_list->pv_pmap) return 0;
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
    int modified, saved_context;
    

    if (!pv_list->pv_pmap) return 0;
    modified = 0;
    for (pv = head; pv != NULL; pv=pv->pv_next) {
	if (pv->pv_pmap == kernel_pmap) {
	    pte = get_pte(pv->pv_va);
	    if (pte & PG_MOD) return 1;
	    continue;
	}
	seg = VA_SEG(pv->pv_va);
	if (pv->pv_pmap->pm_segmap[seg] == SEGINV) continue;
	if (get_pte_val(pv->pv_pmap, pv->pv_va,&pte)) {
	    if (pte & PG_MOD) return 1;
	}
    }
    return 0;
}


/* pv_entry support routines */
void pv_remove_all(pa)
     vm_offset_t pa;
{
    pv_entry_t *npv;
    int saved_context;

    head = pa_to_pvp(pa);
    saved_context = get_context();
    for (npv = head ; npv != NULL; last= npv, npv = npv->pv_next ) {
	if (npv->pv_pmap == NULL) continue; /* empty */
	if (npv->pv_pmap == kernel_pmap) { /* kernel */
	    pte = get_pte(npv->pv_va);
	    save_modified_bits(pte);
	    set_pte(npv->pv_va, PG_INVAL);
	    continue;
	}
	if (npv->pv_pmap->pm_context) {	/* has user context */
	    set_context(npv->pv_pmap->pm_context->context_num);
	    if (get_pte_if_possible(npv->pv_pmap->pm_va, &pte)) {
		save_modified_bits(pte);
		set_pte_if_possible(npv->pv_pmap->pm_va, PG_INVAL);
	    }
	}
    }
    set_context(saved_context);
}

unsigned char pv_link(pmap, pa, va, flags)
     pmap_t pmap;
     vm_offset_t pa, va;
     unsigned char flags;
{
    unsigned char nflags;
    pv_entry_t last,pv;

    head = pv = pa_to_pvp(pa);
    PMAP_LOCK();
    if (pv->pv_pmap == NULL) {	/* not currently "managed" */
 	pv->pv_va = va;
	pv->pv_pmap = pmap,
	pv->pv_next = NULL;
	pv->pv_flags = flags;
	set_cache_flags(pa) = flags;
	return flags & PV_NC;
    }
    for (npv = pv ; npv != NULL; last= npv, npv = npv->pv_next ) {
	if ((pv->pv_pmap != pmap) || (pv->pv_va != va)) { 
	    if (flags == npv->pv_flags)	    /* no change */
		return get_cache_flags(pa);
	    npv->pv_flags = flags;
	    goto recompute;
	}
    }
    pv = zalloc(pv_zone);
    pv->pv_va = va;
    pv->pv_pmap = pmap,
    pv->pv_next = NULL;
    pv->pv_flags = flags;
    last->next = pv;

 recompute:
    if (get_cache_flags(pa) & PG_NC) return flags & PV_NC; /* already NC */
    if (flags & NC) {		/* being NCed, wasn't before */
	set_cache_flags(pa, flags);
	pv_change_pte(head, MAKE_PV_REAL(PV_NC), 0);
	return flags & PV_NC:
    }
    nflags = pv_compute_cache(head);
    set_cache_flags(pa, nflags);
    pv_change_pte(head, MAKE_PV_REAL(nflags), 0); /*  */
    return nflags & PV_NC;
}
void pv_changepte(pv_list, set_bits, clear_bits)
     pv_entry_t pv_list;
     vm_offset_t set_bits;
     vm_offset_t clr_bits;
{
    pv_entry_t pv;
    int context;
    vm_offset_t pte;

    if (pv_list->pv_pmap == NULL) /* nothing to hack on */
	return;
    if (!set_bits && !clear_bits) return; /* nothing to do */
    context = get_context();
    for (pv = pv_list; pv != NULL; pv=pv->pv_next) {
	if (pv->pv_pmap == NULL) 
	    panic("pv_list corrupted");
	if (pv->pv_pmap == kernel_pmap) {
	    pte = get_pte(va);
	    pte|=set_bits;
	    pte&=~clr_bits;
	    set_pte(va, pte);
	    continue;
	}
	if (!pv->pv_pmap->pm_context)
	    continue;
	set_context(pv->pv_pmap->pm_context);
	if (!get_pte_if_possible(va, &pte)) continue;
	    pte|=set_bits;
	    pte&=~clr_bits;
	    set_pte(va, pte);
    }
    set_context(context);
}
void pv_unlink(pmap, pa, va)
     pmap_t pmap;
     vm_offset_t pa, va;
{
    pv_entry_t pv,pv_list,dead;
    int context;
    vm_offset_t pte;
    unsigned char saved_flags;
    pv_list = pa_to_pv(pa);
    if (pv_list->pv_pmap == NULL) {
	printf("pv_unlinking too many times");
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
		pv->pv_pmap = dead->pv_pmap,
		pv->pv_next = dead->pv_next
		pv->pv_flags = dead->pv_flags;
		zfree(pv_zone, dead);
	    }
	    else
		pv->pv_pmap = NULL; /* enough to invalidate */
	}
	else {
	    last->pv_next = pv->pv_next;
	    zfree(pv);
	}
	if (saved_flags & PV_NC) {/* this entry definitely caused a NC
				  * condition.  
				  */
	    nflags = pv_compute_cache(head);
	    set_cache_flags(pa, nflags);
	    pv_change_pte(head, MAKE_PV_REAL(nflags), 0); /*  */
	}
	return;
    }
    panic("pv_unlink: couldn't find entry");
}
void pv_init()
{
    int i;
    pv_head_table = kmem_alloc(kernel_map, (memavail >> PG_SHIFT)*
			       (struct pv_entry));
    for (i = 0; i < memavail; i++) { /* dumb XXX*/
	bzero(&pv_head_table[i], sizeof(struct pv_entry));
    }
    pv_modified_table = kmem_alloc(kernel_map, (memavail >> PG_SHIFT)*
				   sizeof(char));
    bzero(pv_modified_table, sizeof(char)* (memavail>>PG_SHIFT));
}

void sun3_protection_init()
{
    unsigned int *kp, prot;

    kp = protection_conv;
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

void pmap_bootstrap(kernel_end, hole)
     vm_offset_t kernel_end;
     int hole;
{
    vm_offset_t seg_begin, seg_end,vmx;
    int i;
    
    virtual_avail = sun3_round_page(kernel_end);    /* set in sun3_init */
    virtual_end = sun3_round_page(VM_MAX_KERNEL_ADDRESS);

    sun3_protection_init();
   /* after setting up some structures */

    kernel_pmap = &kernel_pmap_store;
    pmap_common_init(kernel_pmap);

    pmeg_init();
    context_init();
    
    seg_begin  = sun3_trunc_seg(MONSTART);
    seg_end = sun3_round_seg(MONEND);

    /* protect MMU */
    for (vmx = seg_begin; vmx < seg_end; vmx+=NBSEG) {
	i = mmu_segment_to_pmeg(vmx); /* XXXX */
	pmeg_steal(i);		/* XXXXX */
    }
    /* do something about MONSHORTPAGE/MONSHORTSEG */
    /* do something about device pages in monitor space */
    /* unmap everything else, and copy into all contexts*/
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

    pv_init();
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
    if (size)
	panic("pmap: asked for a software map????");
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

    if (pmap == NULL) return;

    if ((kern_pmap) pmap == kernel_pmap)
	panic("pmap: attempted to destroy kernel_pmap");
    pmap_lock(pmap);
    count = pmap_del_ref(pmap);
    pmap_unlock(pmap);
    simple_unlock(&pmap->pm_lock);
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
    switch (prot) {
    case VM_PROT_ALL:
	break;
    case VM_PROT_READ:
    case VM_PROT_READ|VM_PROT_EXECUTE:
	pv_changepte(pa, 0, PG_W);
	pmap_copy_on_write(pa);
    default:
	/* remove mapping for all pmaps that have it:
	 *
	 * follow pv trail to pmaps and temporarily delete it that way.
	 * keep looping till all mappings go away
	 */
    }
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
    pmap_t pmap;
    vm_offset_t sva, eva;

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
	if (pte & PG_V) {
	    save_modified_bits(pte);
	    pg_unlink(pmap, PG_PA(pte), va);
	    pmegp->pmeg_vpages--;
	    set_pte(va, PG_INVAL);
	}
	va+= NBPG;
    }
    if (pmegp->pmeg_vpages <= 0) {
	if (is_pmeg_wired(pmegp))
	    printf("pmap: removing wired pmeg");
	pmeg_release_empty(pmegp, PM_UPDATE_CACHE);
	if (kernel_pmap == pmap) {
	    for (i=0; i < NCONTEXT; i++) { /* map out of all segments */
		set_context(i);
		set_segmap(sva,SEGINV);
	    }
	}
	else
	    set_segmap(sva, SEGINV);
    }
    set_context(saved_context);
}

void pmap_remove_range_contextless(pmap, sva, eva,pmegp)
     pmap_t pmap;
     vm_offset_t sva, eva;
     pmeg_t pmegp;
{
    int sp,ep,i;
    
    sp = PTE_NUM(sva);
    ep = PTE_NUM(eva);

    for (i = sp; i < ep; i++) {
	pte = get_pte_pmeg(pmegp->pmeg_index, i);
	if (pte & PG_V) {
	    save_modified_bits(pte);
	    pg_unlink(pmap, PG_PA(pte), va);
	    pmegp->pmeg_vpages--;
	    set_pte_pmeg(pmegp->pmeg_index, i, PG_INVAL);
	}
    }
    if (pmegp->pmeg_vpages <= 0) {
	if (is_pmeg_wired(pmegp))
	    printf("pmap: removing wired pmeg");
	pmeg_release_empty(pmegp,VA_SEGNUM(sva), PM_UPDATE_CACHE);
    }
}
/* guaraunteed to be within one pmeg */

void pmap_remove_range(pmap, sva, eva)
     pmap_t pmap;
     vm_offset_t sva, eva;
{
    int left_flush = right_flush = clear = 0;
    int begin, end;

   /* cases: kernel: always has context, always available
    *
    *        user: has context, is available
    *        user: has no context, is available
    *        user: has no context, is not available (NOTHING) |_ together
    *        user: has context, isn't available (NOTHING)     |
    */
            
    if ((pmap != kernel_pmap))
	if (get_pmeg_cache(pmap, VA_SEGNUM(va)) == SEGINV) return;
    if ((pmap == kernel_pmap) ||
 	(pmap->pm_context)) {
	pmap_remove_range_mmu(pmap, sva, eva);
	return;
    }
    /* we are a user pmap without a context, possibly without a pmeg to
     * operate upon
     *
     */
    pmegp = pmeg_cache(pmap, sun3_trunc_seg(sva),PM_UPDATE);
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
    /* some form of locking?, when where. */

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
    last_pmeg = pmegp = NULL;
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
    int s;
    unsigned int pmeg;
    vm_offset_t current_pte;
    unsigned char sme;
    int c;
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
	printf("pmap: kernel trying to allocate virtual space below itself");
    if ((va+NBPG) >= VM_MAX_KERNEL_ADDRESS)
	printf("pmap: kernel trying to allocate virtual space above itself");
    s = splpmap();
    sme = get_segmap(va);
    /* XXXX -- lots of non-defined routines, need to see if pmap has a
     * context
     */
    if (sme == SEGINV) {
	pmegp = pmeg_allocate_invalid(pmap, sun3_trunc_seg(va));
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
    save_modified_bits(current_pte);
    if (PG_PGNUM(current_pte) != PG_PGNUM(pte_proto)) /* !same physical addr*/
	pv_unlink(pmap, PG_PA(current_pte), va); 

add_pte:	/* can be destructive */
    if (wired && !is_pmeg_wired(pmegp)) {
	pmap->pm_stats.wired_count++;
	pmeg_wire(pmegp);
    }
    else if (!wired && is_pmeg_wired(pmegp)) {
	pmap->pm_stats.wired_count--;
	pmeg_unwire(pmegp);
    }
    nflags = pv_link(pmap, pa, va, PG_TO_PV_FLAGS(pteproto));
    if (nflags & PV_NC)
	set_pte(va, pteproto | PG_NC);
    else
	set_pte(va, pteproto);
    pmegp->pmeg_vpages++;	/* assumes pmap_enter can never insert
				 a non-valid page*/
    splx(s);
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
    int current_context; 


    /* sanity checking */
    if (mem_type)
	panic("pmap: attempt to map non main memory page into user space");
    if (va < VM_MIN_USER_ADDRESS)
	printf("pmap: user trying to allocate virtual space below itself");
    if ((va+NBPG) >= VM_MAX_USER_ADDRESS)
	printf("pmap: user trying to allocate virtual space above itself");
    if (wired)
	printf("pmap_enter_user: attempt to wire user page, ignored");
    pte_proto |= MAKE_PGTYPE(PG_MMEM); /* unnecessary */
    s = splpmap();
    saved_context = get_context();
    if (!pmap->pmu_context)
	panic("pmap: pmap_enter_user() on pmap without a context");
    if (saved_context != pmap->pm_context->context_num)
	set_context(pmap->pm_context->context_num);

    sme = get_segmap(va);
    if (sme == SEGINV) {
	pmegp = pmeg_cache(pmap, sun3_trunc_seg(va),PM_CACHE_UPDATE);
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
    save_modified_bits(current_pte);
    pmegp->pmeg_vpages--;	/* removing valid page here, way lame XXX*/
    if (PG_PGNUM(current_pte) != PG_PGNUM(pte_proto)) 
	pv_unlink(pmap, PG_PA(current_pte), va); 

add_pte:
    /* if we did wiring on user pmaps, then the code would be here */
    nflags = pv_link(pmap, pa, va, PG_TO_PV_FLAGS(pteproto));
    if (nflags & PV_NC)
	set_pte(va, pteproto | PG_NC);
    else
	set_pte(va, pteproto);
    pmegp->pmeg_vpages++;	/* assumes pmap_enter can never insert
				   a non-valid page */
    splx(s);
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
    vm_offset_t pte_proto;
    vm_offset_t mem_type;

    if (pmap == NULL) return;

    mem_type = pa & PMAP_MEMMASK;
    pte_proto = PG_VALID | pmap_pte_prot(prot) | (pa & PMAP_NC ? PG_NC: 0);
    pa &= ~PMAP_SPECMASK;
    pte_proto |= (pa >> PG_SHIFT);

    /* treatment varies significantly:
     *  kernel ptes are in all contexts, and are always in the mmu
     *  user ptes may not necessarily? be in the mmu.  pmap may not
     *   be in the mmu either.
     *
     */
    if (pmap == kernel_pmap)
	pmap_enter_kernel(va, pa, prot, wired, pte_proto, mem_type);
    else
	pmap_enter_user(pmap, va, pa, prot, wired, pte_proto, mem_type);
}

void
pmap_clear_modify(pa)
     vm_offset_t	pa;
{
    pv_modified_table[PG_PGNUM(pa)] = 0;
    pv_change_pte(pv_head_table[PG_PGNUM(pa)], 0, PG_MOD);
}
boolean_t
pmap_is_modified(pa)
     vm_offset_t	pa;
{
    if (pv_modified_table[PG_PGNUM(pa)]) return 1;
    pv_modified_table[PG_PGNUM(pa)] = pv_compute_modified(pa_to_pvp(pa));
    return pv_modified_table[PG_PGNUM(pa)];
}

void pmap_clear_reference(pa)
     vm_offset_t	pa;
{
    pv_remove_all(pa);
}

boolean_t
pmap_is_referenced(pa)
     vm_offset_t	pa;
{
    return FALSE;
}


void pmap_activate(pmap, pcbp, is_curproc)
     pmap_t pmap;
     struct pcb *pcbp;
     int is_curproc;
{
    if (is_curproc) return;
    if (pmap->pm_context) {
	set_context(pmap->pm_context->context_num);
	return;
    }
    context_allocate(pmap);
    set_context(pmap->pm_context->context_num);
}
void pmap_deactivate(pmap, pcbp)
     pmap_t pmap;
     struct pcb *pcbp;
{
    enqueue_tail(&context_active_queue,
		 (queue_entry_t) pmap->pm_context);
}

