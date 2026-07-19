/*	$NetBSD: pmap_pgmmu.c,v 1.1 2026/07/19 01:48:24 thorpej Exp $	*/

/*-     
 * Copyright (c) 2025, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *      
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *              
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:                             
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * Pmap module for the 68K Playground MMU, as found on the 68010-based
 * Phaethon 1.
 *
 * The PGMMU is similar to the Sun3 MMU, but there are some differences,
 * and they are not software-compatible.
 *
 * ==> Generally speaking, the MMU must be enabled for the system to
 *     function; if the MMU is disabled, all {User,Supervisor} {Data,Prog}
 *     cycles select the system ROM.  This means not even the stack is
 *     accessible until the firmware sets up the MMU and enbles it, and
 *     the firmware runs in a virtual environment.  When the kernel starts,
 *     it is running in the virtual environment set up by the firmware.
 *
 * ==> PGMMU has 64 contexts.  Supervisor {Data,Prog} cycles are hard-wired
 *     to context 0.  User {Data,Prog} cycles use the context specified by
 *     the Context Register (which, BTW, can be 0!).
 *
 * ==> Context 0's Segment Map has its own selector so that changing the
 *     kernel's Segment Map doesn't require setting the Context Register.
 *
 * ==> Each context has 512 32KB segments in the Segment Map.  Each
 *     16-bit Segment Map entry (SME) has a valid bit and a 15-bit
 *     number representing the Page Map Entry Group (PMEG) that maps
 *     the segment.
 *
 * ==> Each PMEG is comprised of 8 4KB pages, each with independent
 *     Page Map entries (PMEs).  There are 32,768 total PMEGs shared
 *     by all processes in the system.
 *
 * ==> All PMEGs used by the kernel are static.  The kernel pmap makes a
 *     best effort to use PMEGs already used by the system firmware.
 *
 * ==> There is sufficient SRAM in the MMU to fully map all 64 contexts.
 *     If there are more than 64 concurrent processes in the system
 *     (including the kernel), then user processes will have to compete
 *     with one another for contexts and PMEGs.
 *
 * ==> No systems with this MMU have a data cache, so cache management is
 *     not a concern.
 */

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap_pgmmu.c,v 1.1 2026/07/19 01:48:24 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/evcnt.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/cpu.h>
#include <sys/atomic.h>
#include <sys/kmem.h>

#include <machine/pcb.h>

#include <uvm/uvm.h>
#include <uvm/uvm_physseg.h>

/****************************** SERIALIZATION ********************************/

/*
 * XXX Would like to make these do something lightweight-ish in
 * XXX DIAGNOSTIC kernels (and also make ASSERT_SLEEPABLE() trip
 * XXX if we're in a critical section).
 */

#define	PMAP_CRIT_ENTER(code)	code
#define	PMAP_CRIT_EXIT(code)	code
#define	PMAP_CRIT_ASSERT()	__nothing

/**************************** MMU CONFIGURATION ******************************/

#include <hb68k/pg68k/pgmmu.h>

__CTASSERT(PAGE_SIZE == PGMMU_PAGE_SIZE);
__CTASSERT(NBPG == PGMMU_PAGE_SIZE);

#ifdef __mc68010__
#define	KERNEL_MAX_ADDRESS	(1U << 24)
#else
#error KERNEL_MAX_ADDRESS TBD
#endif
static vaddr_t kernel_virtual_start;
       vaddr_t kernel_virtual_max = KERNEL_MAX_ADDRESS;

/***************************** INSTRUMENTATION *******************************/

#ifdef PMAP_EVENT_COUNTERS
static struct evcnt pmap_ctx_alloc_static_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap ctx_alloc", "static");
EVCNT_ATTACH_STATIC(pmap_ctx_alloc_static_ev);

static struct evcnt pmap_ctx_alloc_dynamic_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap ctx_alloc", "dynamic");
EVCNT_ATTACH_STATIC(pmap_ctx_alloc_dynamic_ev);

static struct evcnt pmap_ctx_alloc_steal_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap ctx_alloc", "steal");
EVCNT_ATTACH_STATIC(pmap_ctx_alloc_steal_ev);

static struct evcnt pmap_pv_alloc_wait_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pv_alloc", "wait");
EVCNT_ATTACH_STATIC(pmap_pv_alloc_wait_ev);

static struct evcnt pmap_pv_alloc_nowait_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pv_alloc", "nowait");
EVCNT_ATTACH_STATIC(pmap_pv_alloc_nowait_ev);

static struct evcnt pmap_pv_enter_called_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pv_enter", "called");
EVCNT_ATTACH_STATIC(pmap_pv_enter_called_ev);

static struct evcnt pmap_pv_remove_called_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pv_remove", "called");
EVCNT_ATTACH_STATIC(pmap_pv_remove_called_ev);

static struct evcnt pmap_enter_nowait_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "nowait");
EVCNT_ATTACH_STATIC(pmap_enter_nowait_ev);

static struct evcnt pmap_enter_yeswait_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "yeswait");
EVCNT_ATTACH_STATIC(pmap_enter_yeswait_ev);

static struct evcnt pmap_enter_wire_change_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "wire change");
EVCNT_ATTACH_STATIC(pmap_enter_wire_change_ev);

static struct evcnt pmap_enter_prot_change_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "prot change");
EVCNT_ATTACH_STATIC(pmap_enter_prot_change_ev);

static struct evcnt pmap_enter_pa_change_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "pa change");
EVCNT_ATTACH_STATIC(pmap_enter_pa_change_ev);

static struct evcnt pmap_enter_pv_alloc_fail_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "pv alloc failed");
EVCNT_ATTACH_STATIC(pmap_enter_pv_alloc_fail_ev); 

static struct evcnt pmap_enter_pv_recycle_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "pv recycle");
EVCNT_ATTACH_STATIC(pmap_enter_pv_recycle_ev);

static struct evcnt pmap_prm_got_pg_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap prm", "got pg");
static struct evcnt pmap_prm_lookup_pg_hit_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap prm", "lookup pg hit");
static struct evcnt pmap_prm_lookup_pg_miss_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap prm", "lookup pg miss");
EVCNT_ATTACH_STATIC(pmap_prm_got_pg_ev);
EVCNT_ATTACH_STATIC(pmap_prm_lookup_pg_hit_ev);
EVCNT_ATTACH_STATIC(pmap_prm_lookup_pg_miss_ev);

#define	pmap_evcnt(e)	pmap_ ## e ## _ev.ev_count++
#else
#define	pmap_evcnt(e)	__nothing
#endif

/************************** FORWARD DECLARATIONS *****************************/

struct pmap_completion;

static void	pmap_segment_retain(pmap_t, vaddr_t);
static void	pmap_segment_release(pmap_t, vaddr_t, int);

static void	pmap_remove_mapping(pmap_t, vaddr_t, unsigned int,
		    struct vm_page *pg, struct pmap_completion *);
static void	pmap_remove_all_internal(pmap_t, struct pmap_completion *);

/***************************** PHYS <-> VM PAGE ******************************/

static bool pmap_initialized_p;

static inline struct vm_page *
pmap_pa_to_pg(paddr_t pa)
{
	if (__predict_true(pmap_initialized_p)) {
		return PHYS_TO_VM_PAGE(pa);
	}
	return NULL;
}

static pm_entry_t pmap_changebit(struct vm_page *, pm_entry_t, pm_entry_t);

/*************************** RESOURCE MANAGEMENT *****************************/

static struct pmap kernel_pmap_store;
struct pmap * const kernel_pmap_ptr = &kernel_pmap_store;

/*
 * Avoid a memory load when doing comparisons against pmap_kernel()
 * within this compilation unit.
 */
#undef pmap_kernel
#define	pmap_kernel()	(&kernel_pmap_store)

static struct pool pmap_pool;
static struct pool pmap_pv_pool;

#define	PMAP_PV_LOWAT		16

static void
pmap_alloc_init(void)
{
	pool_init(&pmap_pv_pool, sizeof(struct pv_entry),
	    PVH_ATTR_MASK + 1,		/* align */
	    0,				/* ioff */
	    0,				/* flags */
	    "pmappv",			/* wchan */
	    &pool_allocator_meta,	/* palloc */
	    IPL_VM);			/* ipl */

	/*
	 * Set a low water mark on the pv_entry pool, so that we are
	 * more likely to have these around even in extreme memory
	 * starvation.
	 */
	pool_setlowat(&pmap_pv_pool, PMAP_PV_LOWAT);

	pool_init(&pmap_pool, sizeof(struct pmap),
	    0,				/* align */
	    0,				/* ioff */
	    0,				/* flags */
	    "pmappl",			/* wchan */
	    &pool_allocator_kmem,	/* palloc */
	    IPL_NONE);			/* ipl */
}

static inline pmap_t
pmap_alloc(void)
{
	pmap_t pmap = pool_get(&pmap_pool, PR_WAITOK);
	memset(pmap, 0, sizeof(*pmap));
	return pmap;
}

static inline void
pmap_free(pmap_t pmap)
{
	pool_put(&pmap_pool, pmap);
}

static struct pv_entry *
pmap_pv_alloc(bool nowait)
{
	struct pv_entry *pv;

#ifdef PMAP_EVENT_COUNTERS
	if (nowait) {
		pmap_evcnt(pv_alloc_nowait);
	} else {
		pmap_evcnt(pv_alloc_wait);
	}
#endif

	pv = pool_get(&pmap_pv_pool, nowait ? PR_NOWAIT : 0);
	if (__predict_true(pv != NULL)) {
		KASSERT((((uintptr_t)pv) & PVH_ATTR_MASK) == 0);
	}
	return pv;
}

static void
pmap_pv_free(struct pv_entry *pv)
{
	pool_put(&pmap_pv_pool, pv);
}

/*
 * Whenever we need to free resources back to the system, we want to
 * do it in a batch with any locks released.  So, we have this around
 * to collect the garbage, as needed.
 */
struct pmap_completion {
	struct pv_entry *pc_pvlist;
};

static inline void
pmap_completion_init(struct pmap_completion *pc)
{
	pc->pc_pvlist = NULL;
}

static void
pmap_completion_fini(struct pmap_completion *pc)
{
	struct pv_entry *pv;

	while ((pv = pc->pc_pvlist) != NULL) {
		pc->pc_pvlist = pv->pv_next;
		pmap_pv_free(pv);
	}
}

/*
 * List of all user pmaps, used to identify potential marks for
 * resource theft.  This list is kept LRU-ordered by pmap_activate().
 */
static TAILQ_HEAD(, pmap) pmap_all_user_pmaps;

/*
 * pmap_find_victim:
 *
 *	Identify a pmap we can steal some resources from.
 */
static pmap_t
pmap_find_victim(void)
{
	pmap_t pm, best_victim = NULL;

	/*
	 * Maybe not the best selection criteria, but:
	 * the least-recently-used pmap with the fewest
	 * number of wired mappings.
	 */

	TAILQ_FOREACH(pm, &pmap_all_user_pmaps, pm_list) {
		if (pm->pm_busy) {
			continue;
		}
		if (pm->pm_context == NULL) {
			continue;
		}
		if (pm->pm_stats.wired_count == 0) {
			best_victim = pm;
			break;
		}
		if (best_victim == NULL ||
		    best_victim->pm_stats.wired_count >
						pm->pm_stats.wired_count) {
			best_victim = pm;
		}
	}
	KASSERT(best_victim != NULL);
	KASSERT(best_victim->pm_context != NULL);

	return best_victim;
}

/*
 * Generic bitmap management, used for pmegs.
 */
struct pmap_bitmap {
	uint32_t 	   * const bitmap_words;
	unsigned int const  bitmap_nwords;
	unsigned int	    bitmap_alloc_hint;
};

#define	PMAP_BITMAP_DECL(bm, bits)					\
__CTASSERT((bits) >= 32);						\
__CTASSERT(powerof2(bits));						\
static uint32_t bm ## _bitmap_words[(bits) >> 5];			\
static struct pmap_bitmap bm ## _bitmap = {				\
	.bitmap_words = bm ## _bitmap_words,				\
	.bitmap_nwords = (bits) >> 5,					\
};

#define	BITMAP_NEXT_WORD(bm, x)						\
	(((x) + 1) & ((bm)->bitmap_nwords - 1))

#define	BITMAP_WORD(x)		((x) >> 5)
#define	BITMAP_WORDOFFS(x)	((x) << 5)
#define	BITMAP_BIT(x)		(1U << ((x) & 31))

static void
pmap_bitmap_init(struct pmap_bitmap *bm)
{
	for (unsigned int i = 0; i < bm->bitmap_nwords; i++) {
		bm->bitmap_words[i] = 0xffffffffU;
	}
}

static void
pmap_bitmap_claim(struct pmap_bitmap *bm, unsigned int v)
{
	KASSERT(v < BITMAP_WORDOFFS(bm->bitmap_nwords));
	bm->bitmap_words[BITMAP_WORD(v)] &= ~BITMAP_BIT(v);
}

static bool
pmap_bitmap_claimed_p(struct pmap_bitmap *bm, unsigned int v)
{
	KASSERT(v < BITMAP_WORDOFFS(bm->bitmap_nwords));
	return !(bm->bitmap_words[BITMAP_WORD(v)] & BITMAP_BIT(v));
}

static int
pmap_bitmap_alloc(struct pmap_bitmap *bm)
{
	unsigned int i;
	int v;

	for (i = bm->bitmap_alloc_hint;;
	     i = BITMAP_NEXT_WORD(bm, i)) {
		v = ffs(bm->bitmap_words[i]) - 1;
		if (v != -1) {
			bm->bitmap_alloc_hint = i;
			bm->bitmap_words[i] &= ~BITMAP_BIT(v);
			return BITMAP_WORDOFFS(i) | v;
		}
		if (BITMAP_NEXT_WORD(bm, i) == bm->bitmap_alloc_hint) {
			break;
		}
	}

	return -1;
}

static void
pmap_bitmap_free(struct pmap_bitmap *bm, unsigned int v)
{
	KASSERT(v < BITMAP_WORDOFFS(bm->bitmap_nwords));
	bm->bitmap_words[BITMAP_WORD(v)] |= BITMAP_BIT(v);
	bm->bitmap_alloc_hint = BITMAP_WORD(v);
}

/************************ SME MANIPULATION HELPERS ***************************/

static inline sm_entry_t
pmap_getsme(pmap_t pmap, vaddr_t va)
{
	if (pmap == pmap_kernel()) {
		return pgmmu_getsme0(va);
	}
	KASSERT(pmap->pm_context != NULL);
	KASSERT(pmap->pm_context->ctx_num == pgmmu_getcontext());
	return pgmmu_getsme(va);
}

static inline void
pmap_setsme(pmap_t pmap, vaddr_t va, sm_entry_t sme)
{
	if (pmap == pmap_kernel()) {
		pgmmu_setsme0(va, sme);
	} else {
		KASSERT(pmap->pm_context != NULL);
		KASSERT(pmap->pm_context->ctx_num == pgmmu_getcontext());
		pgmmu_setsme(va, sme);
	}
}

static inline uint16_t
sme_pmeg(sm_entry_t sme)
{
	return sme & SME_PMEG;
}

static inline bool
sme_valid_p(sm_entry_t sme)
{
	return !!(sme & SME_V);
}

/************************ PME MANIPULATION HELPERS ***************************/

static inline paddr_t
pme_pa(pm_entry_t pme)
{
	return pgmmu_ptob(pme & PME_PFN);
}

static inline bool
pme_valid_p(pm_entry_t pme)
{
	return !!(pme & PME_V);
}

static inline bool
pme_wired_p(pm_entry_t pme)
{
	return !!(pme & PME_WIRED);
}

static inline bool
pme_managed_p(pm_entry_t pme)
{
	return !!(pme & PME_PVLIST);
}

static inline pm_entry_t
pme_change_prot(pm_entry_t opme, vm_prot_t prot)
{
	return (opme & ~PME_W) | ((prot & UVM_PROT_WRITE) ? PME_W : 0);
}

static inline unsigned int
pme_index(uint16_t pmeg, vaddr_t va)
{
	return (((uint32_t)pmeg) << PGMMU_PMEG_SHIFT) +
	    (pgmmu_btop(va) & PGMMU_PMEG_OFFSET);
}

static inline pm_entry_t
pmap_make_pme(pmap_t pmap, paddr_t pa, vm_prot_t prot, u_int flags)
{
	pm_entry_t npme = PME_V |
	             (pmap == pmap_kernel()   ? PME_K : 0) |
		     ((prot & UVM_PROT_WRITE) ? PME_W : 0) |
		     pgmmu_btop(pa);
	if (flags & UVM_PROT_WRITE) {
		npme |= PME_M | PME_R;
	} else if (flags & (UVM_PROT_READ | UVM_PROT_EXEC)) {
		npme |= PME_R;
	}
	if (flags & PMAP_WIRED) {
		npme |= PME_WIRED;
	}

	return npme;
}

/* These helpers assume that all kernel segments have valid pmegs. */
static pm_entry_t
pmap_getkpme(vaddr_t va)
{
	sm_entry_t sme = pgmmu_getsme0(va);
	return pgmmu_getpme(pme_index(sme_pmeg(sme), va));
}

static void
pmap_setkpme(vaddr_t va, pm_entry_t pme)
{
	sm_entry_t sme = pgmmu_getsme0(va);
	pgmmu_setpme(pme_index(sme_pmeg(sme), va), pme);
}

/*************************** CONTEXT MANAGEMENT ******************************/

/*
 * This MMU can do 64 contexts, which, to be honest, for a 68010 is kind
 * of a lot!  We're going to just assume that if a context has to be stolen
 * from another pmap, that the PMEGs are going to get slurped up, too.  So,
 * if a context has to get stolen, then we are going to forcefully remove
 * the entire thing, segmap included; it can all be reconstructed from the
 * VM map.  We will TRY to honor pmaps with wired mappings, but ultimately,
 * the resources have to be shared.
 *
 * The upshot of this is that there's really no reason to keep a software
 * copy of the segmap because, realistically, it's not very likely that
 * a context will get stolen from another pmap, and thus there is no need
 * for us to be able to reload a context quickly.
 *
 * So, it's all just kept in the hardware.
 */

/*
 * We have a static context for the kernel (the segrefs aren't used
 * at all, but this makes the logic easier), and we statically allocate
 * a handful of contexts for user pmaps as well.  This ensures that
 * there's at least a few around to steal at any given time.
 */
#define	STATIC_CONTEXTS		9
static struct pmap_context static_contexts[STATIC_CONTEXTS];
static struct pmap_context *context_freelist;
static unsigned int context_last;

static unsigned int pmap_current_context;

static unsigned int
pmap_swap_context(unsigned int ctx)
{
	const unsigned int rv = pmap_current_context;
	if (ctx != rv) {
		pgmmu_setcontext(ctx);
		pmap_current_context = ctx;
	}

	return rv;
}

static inline unsigned int
pmap_context_enter(pmap_t pmap)
{
	struct pmap_context * const ctx = pmap->pm_context;

	KASSERT(ctx != NULL);
	return pmap_swap_context(ctx->ctx_num);
}

static inline void
pmap_context_exit(unsigned int saved_ctx)
{
	(void) pmap_swap_context(saved_ctx);
}

static struct pmap_context *
pmap_context_new(bool nowait)
{
	struct pmap_context *ctx = NULL;
	unsigned int ctx_num;

	ctx_num = context_last + 1;
	if (__predict_false(ctx_num == PGMMU_NUM_CONTEXTS)) {
		return NULL;
	}

	if (ctx_num < STATIC_CONTEXTS) {
		ctx = &static_contexts[ctx_num];
		pmap_evcnt(ctx_alloc_static);
	} else {
		PMAP_CRIT_EXIT();
		ctx = kmem_zalloc(sizeof(*ctx),
				  nowait ? KM_NOSLEEP : KM_SLEEP);
		PMAP_CRIT_ENTER();
		if (!nowait) {
			ctx_num = context_last + 1;
			if (__predict_false(ctx_num >=
					    PGMMU_NUM_CONTEXTS)) {
				kmem_free(ctx, sizeof(*ctx));
				return NULL;
			}
		}
		pmap_evcnt(ctx_alloc_dynamic);
	}

	ctx->ctx_num = context_last = ctx_num;
	return ctx;
}

static void
pmap_context_alloc(pmap_t pmap, bool nowait, struct pmap_completion *pc)
{
	struct pmap_context *ctx;

	KASSERT(pmap != pmap_kernel());
	KASSERT(pmap->pm_context == NULL);

	if (__predict_true((ctx = context_freelist) != NULL)) {
		context_freelist = ctx->ctx_next;
		memset(ctx->ctx_segrefs, 0, sizeof(ctx->ctx_segrefs));
		goto got_one;
	}

	if (__predict_true((ctx = pmap_context_new(true)) != NULL)) {
		/*
		 * We may have blocked while allocating memory, in
		 * which case, another thread may have succeeded in
		 * nabbing a context for this pmap.  If that's the
		 * case, then put the new one we just created onto
		 * the free list and proceed with the one we now
		 * find ourselves in possession of.
		 */
		if (__predict_false(pmap->pm_context != NULL)) {
			ctx->ctx_next = context_freelist;
			context_freelist = ctx;
			return;
		}
		goto got_one;
	}

	/*
	 * We're going to have to steal a context from someone else.
	 */
	pmap_t victim = pmap_find_victim();

	ctx = victim->pm_context;
	pmap_remove_all_internal(victim, pc);
	victim->pm_context = NULL;
 	pmap_evcnt(ctx_alloc_steal);
 got_one:
	pmap->pm_context = ctx;
	if (__predict_true(curproc != NULL &&
			   pmap == curproc->p_vmspace->vm_map.pmap)) {
		pmap_context_enter(pmap);
	}
}

static void
pmap_context_free(pmap_t pmap)
{
	struct pmap_context *ctx;

	if (__predict_true((ctx = pmap->pm_context) != NULL)) {
		pmap->pm_context = NULL;
		ctx->ctx_next = context_freelist;
		context_freelist = ctx;
	}
}

/***************************** PMEG MANAGEMENT *******************************/

/*
 * This MMU has a lot of PMEGs, plenty to go around.  We'll almost always
 * be able to find a free one.  Because of this, the victim selection for
 * the situation where we need to steal one does not need to be particularly
 * sophisticated.
 */

PMAP_BITMAP_DECL(pmeg, PGMMU_NUM_PMEGS)

static unsigned int
pmap_pmeg_alloc(pmap_t pmap, struct pmap_completion *pc)
{
	vaddr_t va, nextva;
	sm_entry_t sme;
	unsigned int seg, i, saved_ctx;
	int pmeg;

	pmeg = pmap_bitmap_alloc(&pmeg_bitmap);
	if (pmeg != -1) {
		return pmeg;
	}

	/*
	 * We're going to have to steal a pmeg from someone else.
	 */
	pmap_t victim = pmap_find_victim();

	/*
	 * Just steal the pmeg of the first valid segment we find.
	 */
	saved_ctx = pmap_context_enter(victim);
	for (seg = 0, va = 0; seg < PGMMU_NUM_SEGS; seg++, va = nextva) {
		nextva = va + PGMMU_SEG_SIZE;
		sme = pgmmu_getsme(va);
		if (sme_valid_p(sme)) {
			pmap_segment_retain(victim, va);
			pmeg = sme_pmeg(sme);
			for (i = 0; i < PGMMU_PMEG_SIZE; i++) {
				pmap_remove_mapping(victim,
				    va + (i * PAGE_SIZE), pmeg, NULL, pc);
			}
			pmap_segment_release(victim, va, -1);
			break;
		}
		/*
		 * We are guaranteed to find something because the victim
		 * will have something to steal.  If we go past the max
		 * user address or roll over back to 0, then we're well and
		 * truly <fill in the blank>.
		 */
		KASSERT(nextva < VM_MAXUSER_ADDRESS);
		KASSERT(nextva != 0);
	}
	pmap_context_exit(saved_ctx);
	KASSERT(pmeg != -1);

	return pmeg;
}

static inline void
pmap_pmeg_free(unsigned int pmeg)
{
	pmap_bitmap_free(&pmeg_bitmap, pmeg);
}

static void
pmap_segment_retain(pmap_t pmap, vaddr_t va)
{
	const unsigned int seg = pgmmu_btos(va);
	struct pmap_context * const ctx = pmap->pm_context;

	if (__predict_true(pmap != pmap_kernel())) {
		KASSERT(ctx != NULL);
		ctx->ctx_segrefs[seg]++;
		KASSERT(ctx->ctx_segrefs[seg] != 0);
	}
}

static void
pmap_segment_release(pmap_t pmap, vaddr_t va, int pmeg)
{
	const unsigned int seg = pgmmu_btos(va);
	struct pmap_context * const ctx = pmap->pm_context;

	if (__predict_true(pmap != pmap_kernel())) {
		KASSERT(ctx != NULL);
		KASSERT(ctx->ctx_segrefs[seg] != 0);
		if (--ctx->ctx_segrefs[seg] == 0 && pmeg != -1) {
			pmap_setsme(pmap, va, 0);
			pmap_pmeg_free(pmeg);
		}
	}
}

/************************** P->V ENTRY MANAGEMENT ****************************/

/*
 * pmap_pv_enter:
 *
 *	Add a physical->virtual entry to the pv table.  Caller must provide
 *	the storage for the new PV entry.
 */
static void
pmap_pv_enter(pmap_t pmap, struct vm_page *pg, vaddr_t va,
    unsigned int pmeg, struct pv_entry *newpv)
{
	pmap_evcnt(pv_enter_called);

	PMAP_CRIT_ASSERT();
	KASSERT(newpv != NULL);

	newpv->pv_pmap = pmap;
	newpv->pv_vf = va;
	newpv->pv_pmeg = pmeg;
	newpv->pv_next = VM_MDPAGE_PVS(pg);
	VM_MDPAGE_SETPVP(VM_MDPAGE_HEAD_PVP(pg), newpv);
}

/*
 * pmap_pv_remove:
 *
 *	Remove a physical->virtual entry from the pv table.
 */
static void
pmap_pv_remove(pmap_t pmap, struct vm_page *pg, vaddr_t va,
    struct pmap_completion *pc)
{
	struct pv_entry **pvp, *pv;

	pmap_evcnt(pv_remove_called);

	PMAP_CRIT_ASSERT();

	for (pvp = VM_MDPAGE_HEAD_PVP(pg), pv = VM_MDPAGE_PVS(pg);
	     pv != NULL;
	     pvp = &pv->pv_next, pv = *pvp) {
		if (pmap == pv->pv_pmap && va == PV_VA(pv)) {
			break;
		}
	}

	KASSERT(pv != NULL);
	VM_MDPAGE_SETPVP(pvp, pv->pv_next);

	KASSERT(pc != NULL);
	pv->pv_next = pc->pc_pvlist;
	pc->pc_pvlist = pv;
}

/***************** PMAP INTERFACE (AND ADJACENT) FUNCTIONS *******************/

static inline void
pmap_stat_update_impl(long *valp, int val)
{
	*valp += val;
}

#define	pmap_stat_update(pm, stat, delta)		\
	pmap_stat_update_impl(&(pm)->pm_stats.stat, (delta))

static inline void
pmap_stat_set_impl(long *valp, int val)
{
	atomic_store_relaxed(valp, val);
}

#define	pmap_stat_set(pm, stat, val)			\
	pmap_stat_set_impl(&(pm)->pm_stats.stat, (val))

/*
 * pmap_pinit:
 *
 *	Common bits of pmap structure initialization shared between
 *	the kernel pmap and user pmaps.
 */
static void
pmap_pinit(pmap_t pmap, struct pmap_context *ctx)
{
	pmap->pm_context = ctx;
	atomic_store_relaxed(&pmap->pm_refcnt, 1);
}

/*
 * pmap_virtual_space:		[ INTERFACE ]
 *
 *	Define the initial bounds of the kernel virtual address space.
 *
 *	In this implementation, the start address we return marks the
 *	end of the statically allocated special kernel virtual addresses
 *	set up in pmap_bootstrap1().  And since we have fixed mapping
 *	resources and thus don't need to have a pmap_growkernel(), we
 *	return the fill limit right away (clamped by whatever top-of
 *	address-space mappings that we need to keep around, like firmware
 *	and device mappings).
 */
void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{
	*vstartp = kernel_virtual_start;
	*vendp = kernel_virtual_max;
}

/*
 * pmap_init:			[ INTERFACE ]
 *
 *	Initialize the pmap module.  Called by vm_init(), to initialize any
 *	structures that the pmap system needs to map virtual memory.
 */
void
pmap_init(void)
{
	/* Initialize the pmap / pv_entry allocators. */
	pmap_alloc_init();

	/* Now it's safe to do P->V entry recording! */
	pmap_initialized_p = true;
}

/*
 * pmap_create:			[ INTERFACE ]
 *
 *	Create and return a physical map.
 */
pmap_t
pmap_create(void)
{
	pmap_t pmap;

	/*
	 * We don't allocate a context until the first mapping is
	 * entered.
	 */
	pmap = pmap_alloc();
	pmap_pinit(pmap, NULL);

	PMAP_CRIT_ENTER();
	TAILQ_INSERT_TAIL(&pmap_all_user_pmaps, pmap, pm_list);
	PMAP_CRIT_EXIT();

	return pmap;
}

/*
 * pmap_destroy:		[ INTERFACE ]
 *
 *	Drop the reference count on the specified pmap, releasing
 *	all resources if the reference count drops to zero.
 */
void
pmap_destroy(pmap_t pmap)
{
	unsigned int newval;

	PMAP_CRIT_ENTER();
	KASSERT(pmap->pm_refcnt > 0);
	newval = --pmap->pm_refcnt;

	if (newval) {
		PMAP_CRIT_EXIT();
		return;
	}

	/* We assume all mappings have been removed. */
	KASSERT(pmap->pm_stats.resident_count == 0);
	if (pmap->pm_context != NULL) {
		pmap_context_free(pmap);
	}

	TAILQ_REMOVE(&pmap_all_user_pmaps, pmap, pm_list);

	PMAP_CRIT_EXIT();

	pmap_free(pmap);
}

/*
 * pmap_reference:		[ INTERFACE ]
 *
 *	Add a reference to the specified pmap.
 */
void
pmap_reference(pmap_t pmap)
{
	PMAP_CRIT_ENTER();
	pmap->pm_refcnt++;
	KASSERT(pmap->pm_refcnt > 0);
	PMAP_CRIT_EXIT();
}

/*
 * pmap_remove_mapping:
 *
 *	Invalidate a single page denoted by pmap/va.
 */
static void
pmap_remove_mapping(pmap_t pmap, vaddr_t va, unsigned int pmeg,
    struct vm_page *pg, struct pmap_completion *pc)
{
	const unsigned int pmeidx = pme_index(pmeg, va);
	const pm_entry_t opme = pgmmu_getpme(pmeidx);

	if (! pme_valid_p(opme)) {
		return;
	}

	const paddr_t pa = pme_pa(opme);
	KASSERT(pg == NULL || pa == VM_PAGE_TO_PHYS(pg));

	/* Update statistics. */
	if (pme_wired_p(opme)) {
		pmap_stat_update(pmap, wired_count, -1);
	}
	pmap_stat_update(pmap, resident_count, -1);

	if (__predict_true(pg == NULL)) {
		pg = pmap_pa_to_pg(pa);
		if (pg != NULL) {
			pmap_evcnt(prm_lookup_pg_hit);
		} else {
			pmap_evcnt(prm_lookup_pg_miss);
		}
	} else {
		pmap_evcnt(prm_got_pg);
	}
	if (__predict_true(pg != NULL)) {
		KASSERT(pme_managed_p(opme));
		/* Update cached M/R bits from mapping that's going away. */
		VM_MDPAGE_ADD_MR(pg, opme);
		pmap_pv_remove(pmap, pg, va, pc);
	} else {
		KASSERT(! pme_managed_p(opme));
	}

	/* Zap the Page Map entry. */
	pgmmu_setpme(pmeidx, 0);
	pmap_segment_release(pmap, va, pmeg);
}

/*
 * pmap_remove:			[ INTERFACE ]
 *
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly rounded
 *	to the page size.
 *
 *	N.B. Callers of pmap_remove_internal() are expected to
 *	provide an initialized completion context, which we
 *	will finalize.
 */
static void
pmap_remove_internal(pmap_t pmap, vaddr_t sva, vaddr_t eva,
    struct pmap_completion *pc)
{
	sm_entry_t sme;
	vaddr_t nextseg;
	unsigned int saved_ctx;
	unsigned int pmeg;

	PMAP_CRIT_ENTER(pmap->pm_busy++);

	if (pmap->pm_context == NULL) {
		KASSERT(pmap->pm_stats.resident_count == 0);
		goto out;
	}
	saved_ctx = pmap_context_enter(pmap);

	while (sva < eva) {
		nextseg = pgmmu_next_seg(sva);
		if (nextseg == 0 || nextseg > eva) {
			nextseg = eva;
		}

		sme = pmap_getsme(pmap, sva);
		if (! sme_valid_p(sme)) {
			/*
			 * No PMEG for this segment; advance to the
			 * next one.
			 */
			sva = nextseg;
			continue;
		}
		pmeg = sme_pmeg(sme);

		for (; sva < nextseg; sva += PAGE_SIZE) {
			pmap_remove_mapping(pmap, sva, pmeg, NULL, pc);
		}
	}

 	pmap_context_exit(saved_ctx);
 out:
	PMAP_CRIT_EXIT(pmap->pm_busy--);
	pmap_completion_fini(pc);
}

void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	struct pmap_completion pc;
	pmap_completion_init(&pc);
	pmap_remove_internal(pmap, sva, eva, &pc);
	/* pmap_remove_internal() calls pmap_completion_fini(). */
}

/*
 * pmap_remove_all:		[ INTERFACE ]
 *
 *	Remove all mappings from a pmap in bulk.  This is only called
 *	when it's known that the address space is no longer visible to
 *	any user process (e.g. during exit or exec).
 */
static void
pmap_remove_all_internal(pmap_t pmap, struct pmap_completion *pc)
{
	unsigned int pmeg, seg, i, saved_ctx;
	vaddr_t va, nextva;
	sm_entry_t sme;

	saved_ctx = pmap_context_enter(pmap);
	for (seg = 0, va = 0; seg < PGMMU_NUM_SEGS; seg++, va = nextva) {
		nextva = va + PGMMU_SEG_SIZE;
		sme = pgmmu_getsme(va);
		if (sme_valid_p(sme)) {
			pmeg = sme_pmeg(sme);
			for (i = 0; i < PGMMU_PMEG_SIZE;
			     i++, va += PAGE_SIZE) {
				pmap_remove_mapping(pmap, va, pmeg, NULL, pc);
			}
		}
	}
	pmap_context_exit(saved_ctx);
}

bool
pmap_remove_all(pmap_t pmap)
{
	struct pmap_completion pc;

	KASSERT(pmap != pmap_kernel());

	pmap_completion_init(&pc);

	PMAP_CRIT_ENTER(pmap->pm_busy++);
	if (pmap->pm_context != NULL) {
		pmap_remove_all_internal(pmap, &pc);
	}
	PMAP_CRIT_EXIT(pmap->pm_busy--);

	pmap_completion_fini(&pc);

	return true;
}

/*
 * pmap_page_protect:		[ INTERFACE ]
 *
 *	Lower the permission for all mappings to a given page to
 *	the permissions specified.
 */
void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	struct pmap_completion pc;
	struct pv_entry *pv;

	if (prot & UVM_PROT_WRITE) {
		/* No protection to revoke. */
		return;
	}

	if (prot & UVM_PROT_READ) {
		/* Making page copy-on-write. */
		pmap_changebit(pg, 0, (pm_entry_t)~PME_W);
		return;
	}

	/* Removing all mappings for a page. */
	pmap_completion_init(&pc);

	PMAP_CRIT_ENTER();

	unsigned int saved_ctx = pmap_current_context;

	while ((pv = VM_MDPAGE_PVS(pg)) != NULL) {
		pmap_context_enter(pv->pv_pmap);
		pv->pv_pmap->pm_busy++;
		pmap_remove_mapping(pv->pv_pmap, PV_VA(pv), pv->pv_pmeg,
		    pg, &pc);
		pv->pv_pmap->pm_busy--;
	}

	pmap_context_exit(saved_ctx);

	PMAP_CRIT_EXIT();

	pmap_completion_fini(&pc);
}

/*
 * pmap_protect:		[ INTERFACE ]
 *
 *	Set the physical protection on the specified range of this map
 *	as requested.
 */
void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	sm_entry_t sme;
	pm_entry_t opme, npme;
	vaddr_t nextseg;
	unsigned int saved_ctx;
	unsigned int pmeg;
	unsigned int pmeidx;

	if ((prot & UVM_PROT_READ) == 0) {
		struct pmap_completion pc;
		pmap_completion_init(&pc);
		pmap_remove_internal(pmap, sva, eva, &pc);
		/* pmap_remove_internal() calls pmap_completion_fini(). */
		return;
	}

	PMAP_CRIT_ENTER(pmap->pm_busy++);

	if (pmap->pm_context == NULL) {
		KASSERT(pmap->pm_stats.resident_count == 0);
		goto out;
	}
	saved_ctx = pmap_context_enter(pmap);

	while (sva < eva) {
		nextseg = pgmmu_next_seg(sva);
		if (nextseg == 0 || nextseg > eva) {
			nextseg = eva;
		}

		sme = pmap_getsme(pmap, sva);
		if (! sme_valid_p(sme)) {
			/*
			 * No PMEG for this segment; advance to the
			 * next one.
			 */
			sva = nextseg;
			continue;
		}
		pmeg = sme_pmeg(sme);

		/*
		 * Change protection on mapping if it is valid and doesn't
		 * already have the correct protection.
		 */
		for (pmeidx = pme_index(pmeg, sva);
		     sva < nextseg; pmeidx++, sva += PAGE_SIZE) {
			opme = pgmmu_getpme(pmeidx);
			if (! pme_valid_p(opme)) {
				continue;
			}
			npme = pme_change_prot(opme, prot);
			if (npme == opme) {
				continue;
			}
			pgmmu_setpme(pmeidx, npme);
		}
	}
 	pmap_context_exit(saved_ctx);
 out:
	PMAP_CRIT_EXIT(pmap->pm_busy--);
}

/*
 * pmap_enter:			[ INTERFACE ]
 *
 *	Insert the given physical address (pa) at the specified
 *	virtual address (va) in the target physical map with the
 *	protection requested.
 *
 *	If specified, the page will be wired down, meaning that
 *	related pme can not be reclaimed.
 *
 *	Note:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */
int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	pm_entry_t npme, opme;
	sm_entry_t sme;
	unsigned int pmeg;
	unsigned int pmeidx;
	unsigned int saved_ctx;
	struct pv_entry *newpv;
	struct pmap_completion pc;
	int error = 0;
	const bool nowait = !!(flags & PMAP_CANFAIL);

	pmap_completion_init(&pc);

	struct vm_page * const pg = pmap_pa_to_pg(pa);

	PMAP_CRIT_ENTER(pmap->pm_busy++);

	if (nowait) {
		pmap_evcnt(enter_nowait);
	} else {
		pmap_evcnt(enter_yeswait);
	}

	/* If we don't already have a context, get one. */
	if (__predict_false(pmap->pm_context == NULL)) {
		pmap_context_alloc(pmap, nowait, &pc);
	}

	saved_ctx = pmap_context_enter(pmap);

	/* Check to see if we already have a valid PMEG for this mapping. */
	sme = pmap_getsme(pmap, va);
	if (sme_valid_p(sme)) {
		/* Yup! */
		pmeg = sme_pmeg(sme);
	} else {
		/* Need to allocate one. */
		pmeg = pmap_pmeg_alloc(pmap, &pc);
		pmap_setsme(pmap, va, SME_V | pmeg);
	}
	pmap_segment_retain(pmap, va);

	pmeidx = pme_index(pmeg, va);

	/* Compute the new PME. */
	npme = pmap_make_pme(pmap, pa, prot, flags);

	/* Fetch the old PME. */
	opme = pgmmu_getpme(pmeidx);

	/*
	 * Check to see if there is an old mapping at this address.
	 * It might simply be a wiring or protection change.
	 */
	if (pme_valid_p(opme)) {
 restart:
		if (pme_pa(opme) == pa) {
			/*
			 * Just a protection or wiring change.
			 *
			 * Since the old PME is handy, go ahead and update
			 * the cached M/R attributes now.  Normally we would
			 * do this in pmap_remove_mapping(), but we're not
			 * taking that path in this case.  We also add in
			 * any M/R attributes hinted by the access type
			 * that brought us to pmap_enter() in the first
			 * place (a write-fault on a writable page mapped
			 * read-only during a page-out, for example).
			 *
			 * Also ensure that the PV list status of the mapping
			 * is consistent.
			 */
			if (__predict_true(pg != NULL)) {
				VM_MDPAGE_ADD_MR(pg, opme | npme);
				KASSERT(pme_managed_p(opme));
				npme |= PME_PVLIST;
			}

			/* Set the new PME. */
			pgmmu_setpme(pmeidx, npme);

			const pm_entry_t diff = opme ^ npme;

#ifdef PMAP_EVENT_COUNTERS
			if (diff & PME_WIRED) {
				pmap_evcnt(enter_wire_change);
			}
			if (diff & PME_W) {
				pmap_evcnt(enter_prot_change);
			}
#endif

			if (pme_wired_p(diff)) {
				pmap_stat_update(pmap, wired_count,
				    pme_wired_p(npme) ? 1 : -1);
			}

			/* All done! */
			goto out_release;
		}

		/*
		 * The mapping has completely changed.  Need to remove
		 * the old one first.
		 *
		 * This will drop the retain count on the segment owned
		 * by the previous mapping, but the newly-entered mapping
		 * will inherit the retain count taken when we validated
		 * the SME.
		 */
		pmap_evcnt(enter_pa_change);
		pmap_remove_mapping(pmap, va, pmeg, NULL, &pc);
	}

	/* Update pmap stats now. */
	pmap_stat_update(pmap, resident_count, 1);
	if (__predict_false(pme_wired_p(npme))) {
		pmap_stat_update(pmap, wired_count, 1);
	}

	if (__predict_true(pg != NULL)) {
		/*
		 * Managed pages also go on the PV list, so we are
		 * going to need a PV entry.
		 */
		newpv = pc.pc_pvlist;
		if (__predict_true(newpv == NULL)) {
			/*
			 * No PV entry to recycle; allocate a new one.
			 * Because this is an extremely common case, we
			 * are first going to attempt allocation while
			 * still in the critical section.  If that fails
			 * and waiting is allowed, we'll leave the critical
			 * section and try a blocking allocation.
			 */
			newpv = pmap_pv_alloc(true/*nowait flag*/);
			if (__predict_false(newpv == NULL)) {
				if (nowait) {
					pmap_evcnt(enter_pv_alloc_fail);
					error = ENOMEM;
					goto out_release;
				}
				/* XXX Should steal a PV */
				PMAP_CRIT_EXIT();
				newpv = pmap_pv_alloc(false/*nowait flag*/);
				KASSERT(newpv != NULL);
				PMAP_CRIT_ENTER();
				/*
				 * Because we may have blocked while allocating
				 * the PV entry, we have to re-validate our
				 * environment, as another thread could have
				 * inserted a mapping here behind our back.
				 */
				opme = pgmmu_getpme(pmeidx);
				if (__predict_false(pme_valid_p(opme))) {
					pmap_stat_update(pmap,
					    resident_count, -1);
					if (pme_wired_p(npme)) {
						pmap_stat_update(pmap,
						    wired_count, -1);
					}
					newpv->pv_next = pc.pc_pvlist;
					pc.pc_pvlist = newpv;
					goto restart;
				}
			}
		} else {
			pmap_evcnt(enter_pv_recycle);
			pc.pc_pvlist = newpv->pv_next;
			newpv->pv_next = NULL;
		}

		/* Enter the mapping into the PV list. */
		pmap_pv_enter(pmap, pg, va, pmeg, newpv);
		npme |= PME_PVLIST;

		/* ...and seed the page attributes. */
		VM_MDPAGE_ADD_MR(pg, npme);
	}

	/*
	 * Set the new PME.  The new mapping takes ownership of the segment
	 * retain count we took earlier.
	 */
	pgmmu_setpme(pmeidx, npme);
	goto out_crit_exit;

 out_release:
	pmap_segment_release(pmap, va, pmeg);
 out_crit_exit:
	pmap_context_exit(saved_ctx);
	PMAP_CRIT_EXIT(pmap->pm_busy--);

	pmap_completion_fini(&pc);
	return error;
}

/*
 * pmap_kenter_pa:		[ INTERFACE ]
 *
 *	Enter a va -> pa mapping into the kernel pmap without any
 *	physical->virtual tracking.
 */
void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	pmap_t const pmap = pmap_kernel();

	const sm_entry_t sme = pgmmu_getsme0(va);

	/* The kernel context is fully loaded with PMEGs. */
	KASSERT(sme_valid_p(sme));
	const unsigned int pmeidx = pme_index(sme_pmeg(sme), va);

	/* Build the new PME. */
	const pm_entry_t npme =
	    pmap_make_pme(pmap, pa, prot, flags | PMAP_WIRED);

	/* There must not be a valid PTE here. */
	KASSERT(! pme_valid_p(pgmmu_getpme(pmeidx)));

	/* Set the new PME. */
	pgmmu_setpme(pmeidx, npme);

	pmap_stat_update(pmap, resident_count, 1);
	pmap_stat_update(pmap, wired_count, 1);
}

/*
 * pmap_kremove:		[ INTERFACE ]
 *
 *	Remove a mapping entered with pmap_kenter_pa() starting at va,
 *	for size bytes (assumed to be page rounded).
 */
void
pmap_kremove(vaddr_t va, vsize_t size)
{
	int count = 0;
	sm_entry_t sme;
	pm_entry_t opme;
	unsigned int pmeidx;
	unsigned int pmeg;
	vaddr_t eva = va + size;
	vaddr_t nextseg;

	while (va < eva) {
		nextseg = pgmmu_next_seg(va);
		if (nextseg == 0 || nextseg > eva) {
			nextseg = eva;
		}

		sme = pgmmu_getsme0(va);
		KASSERT(sme_valid_p(sme));

		pmeg = sme_pmeg(sme);

		for (pmeidx = pme_index(pmeg, va);
		     va < nextseg; pmeidx++, va += PAGE_SIZE) {
			opme = pgmmu_getpme(pmeidx);
			if (pme_valid_p(opme)) {
				KASSERT(! pme_managed_p(opme));
				KASSERT(pme_wired_p(opme));
				/* Zap the mapping. */
				pgmmu_setpme(pmeidx, 0);
				count++;
			}
		}
	}

	/* Update stats. */
	if (__predict_true(count != 0)) {
		pmap_stat_update(pmap_kernel(), resident_count, -count);
		pmap_stat_update(pmap_kernel(), wired_count, -count);
	}
}

/*
 * pmap_unwire:			[ INTERFACE ]
 *
 *	Clear the wired attribute for a map/virtual-address pair.
 *
 *	The mapping must already exist in the pmap.
 *	(Except, it might not if we stole it.)
 */
void
pmap_unwire(pmap_t pmap, vaddr_t va)
{
	PMAP_CRIT_ENTER(pmap->pm_busy++);

	if (pmap->pm_context == NULL) {
		KASSERT(pmap->pm_stats.resident_count == 0);
		goto out;
	}
	const unsigned int saved_ctx = pmap_context_enter(pmap);

	const sm_entry_t sme = pmap_getsme(pmap, va);
	if (sme_valid_p(sme)) {
		const unsigned int pmeidx = pme_index(sme_pmeg(sme), va);
		const pm_entry_t pme = pgmmu_getpme(pmeidx);
		if (pme_valid_p(pme) && pme_wired_p(pme)) {
			pgmmu_setpme(pmeidx, pme & ~PME_WIRED);
			pmap_stat_update(pmap, wired_count, -1);
		}
	}

	pmap_context_exit(saved_ctx);
 out:
	PMAP_CRIT_EXIT(pmap->pm_busy--);
}

/*
 * pmap_extract:		[ INTERFACE ]
 *
 *	Extract the physical address associated with the given
 *	pmap/virtual address pair.
 *
 * pmap_extract_info:
 *
 *	Like pmap_extract(), but also returns information
 *	about the mapping (wired, cache-inhibited, etc.)
 */
bool
pmap_extract_info(pmap_t pmap, vaddr_t va, paddr_t *pap, int *flagsp)
{
	unsigned int saved_ctx;
	bool rv = false;

	PMAP_CRIT_ENTER(pmap->pm_busy++);
	if (pmap->pm_context == NULL) {
		KASSERT(pmap->pm_stats.resident_count == 0);
		goto out;
	}
	saved_ctx = pmap_context_enter(pmap);

	const sm_entry_t sme = pmap_getsme(pmap, va);
	if (__predict_true(sme_valid_p(sme))) {
		const unsigned int pmeidx = pme_index(sme_pmeg(sme), va);
		const pm_entry_t pme = pgmmu_getpme(pmeidx);
		if (__predict_true(pme_valid_p(pme))) {
			if (__predict_true(pap != NULL)) {
				*pap = pme_pa(pme) | (va & PGOFSET);
			}
			if (__predict_false(flagsp != NULL)) {
				/*
				 * No systems with this MMU have a data
				 * cache, so always indicate that the
				 * mappings are not cached.
				 */
				*flagsp = PMAP_NOCACHE |
				    (pme_wired_p(pme) ? PMAP_WIRED : 0);
			}
			rv = true;
		}
	}

	pmap_context_exit(saved_ctx);
 out:
	PMAP_CRIT_EXIT(pmap->pm_busy--);
	return rv;
}

bool
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *pap)
{
	return pmap_extract_info(pmap, va, pap, NULL);
}

/*
 * vtophys:
 *
 *	Dumber version of pmap_extract(pmap_kernel(), ...)
 */
paddr_t
vtophys(vaddr_t va)
{
	paddr_t pa;
	bool rv __diagused;

	rv = pmap_extract_info(pmap_kernel(), va, &pa, NULL);
	KASSERT(rv);
	return rv ? pa : -1;
}

/*
 * kvtop:
 *
 *	Sigh.
 */
int
kvtop(void *v)
{
	return (int)vtophys((vaddr_t)v);
}

/*
 * pmap_copy:			[ INTERFACE ]
 *
 *	Copy the mapping range specified by src_addr/len
 *	from the source map to the range dst_addr/len
 *	in the destination map.
 *
 *	This routine is only advisory and need not do anything.
 */
/* call deleted in <machine/pmap.h> */

/*
 * pmap_update:			[ INTERFACE ]
 *
 *	Require that all active physical maps contain no
 *	incorrect entries NOW, by processing any deferred
 *	pmap operations.
 */
/* call deleted in <machine/pmap.h> */

/*
 * pmap_activate:		[ INTERFACE ]
 *
 *	Activate the pmap used by the specified process.  This includes
 *	reloading the MMU context of the current process, and marking
 *	the pmap in use by the processor.
 */
void
pmap_activate(struct lwp *l)
{
	pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;

	KASSERT(l == curlwp);

	/*
	 * If the pmap doesn't have a valid context, just use
	 * the kernel's context for now (don't worry, the kernel's
	 * mappings are protected with PME_K).
	 */
	PMAP_CRIT_ENTER(pmap->pm_busy++);
	(void) pmap_swap_context(pmap->pm_context != NULL
	    ? pmap->pm_context->ctx_num : 0);
	if (pmap != pmap_kernel()) {
		TAILQ_REMOVE(&pmap_all_user_pmaps, pmap, pm_list);
		TAILQ_INSERT_TAIL(&pmap_all_user_pmaps, pmap, pm_list);
	}
	PMAP_CRIT_EXIT();
}

/*
 * pmap_deactivate:		[ INTERFACE ]
 *
 *	Mark that the pmap used by the specified process is no longer
 *	in use by the processor.
 */
void
pmap_deactivate(struct lwp *l)
{
	pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;

	PMAP_CRIT_ENTER();
	KASSERT(pmap->pm_busy != 0);
	PMAP_CRIT_EXIT(pmap->pm_busy--);
}

static vaddr_t pmap_tmpmap_srcva;
static vaddr_t pmap_tmpmap_dstva;

static unsigned int pmap_tmpmap_srcidx;
static unsigned int pmap_tmpmap_dstidx;

/*
 * pmap_zero_page:		[ INTERFACE ]
 *
 *	Zero the specified VM page by mapping the page into the kernel
 *	and using memset() (or equivalent) to clear its contents.
 */
void
pmap_zero_page(paddr_t pa)
{
	const int flags = PMAP_WIRED;

	/* Build the new PME. */
	const pm_entry_t dst_pme =
	    pmap_make_pme(pmap_kernel(), pa,
			  UVM_PROT_READ | UVM_PROT_WRITE, flags);

	/* Set the new PME. */
	KASSERT(! pme_valid_p(pgmmu_getpme(pmap_tmpmap_dstidx)));
	pgmmu_setpme(pmap_tmpmap_dstidx, dst_pme);

	/* Zero the page. */
	zeropage((void *)pmap_tmpmap_dstva);

	/* Invalidate the PME. */
	pgmmu_setpme(pmap_tmpmap_dstidx, 0);
}

/*
 * pmap_copy_page:		[ INTERFACE ]
 *
 *	Copy the specified VM page by mapping the page(s) into the kernel
 *	and using memcpy() (or equivalent).
 */
void
pmap_copy_page(paddr_t src, paddr_t dst)
{
	const int flags = PMAP_WIRED;

	/* Build the new PMEs. */
	const pm_entry_t src_pme =
	    pmap_make_pme(pmap_kernel(), src,
			  UVM_PROT_READ, flags);
	const pm_entry_t dst_pme =
	    pmap_make_pme(pmap_kernel(), dst,
			  UVM_PROT_READ | UVM_PROT_WRITE, flags);

	/* Set the new PMEs. */
	KASSERT(! pme_valid_p(pgmmu_getpme(pmap_tmpmap_srcidx)));
	pgmmu_setpme(pmap_tmpmap_srcidx, src_pme);
	KASSERT(! pme_valid_p(pgmmu_getpme(pmap_tmpmap_dstidx)));
	pgmmu_setpme(pmap_tmpmap_dstidx, dst_pme);

	/* Copy the page. */
	copypage((void *)pmap_tmpmap_srcva, (void *)pmap_tmpmap_dstva);

	/* Invalidate the PMEs. */
	pgmmu_setpme(pmap_tmpmap_srcidx, 0);
	pgmmu_setpme(pmap_tmpmap_dstidx, 0);
}

/*
 * pmap_testbit:
 *
 *	Test the modified / referenced bits of a physical page.
 */
static bool
pmap_testbit(struct vm_page *pg, pm_entry_t bit)
{
	struct pv_entry *pv;
	pm_entry_t pme;

	PMAP_CRIT_ENTER();

	pme = VM_MDPAGE_MR(pg);

	for (pv = VM_MDPAGE_PVS(pg);
	     (pme & bit) == 0 && pv != NULL; pv = pv->pv_next) {
		pme |= pgmmu_getpme(pme_index(pv->pv_pmeg, PV_VA(pv)));
	}

	VM_MDPAGE_ADD_MR(pg, pme);

	PMAP_CRIT_EXIT();

	return (pme & bit) != 0;
}

/*
 * pmap_is_referenced:		[ INTERFACE ]
 *
 *	Return whether or not the specified physical page has been referenced
 *	by any physical maps.
 */
bool
pmap_is_referenced(struct vm_page *pg)
{
	return pmap_testbit(pg, PME_R);
}

/*
 * pmap_is_modified:		[ INTERFACE ]
 *
 *	Return whether or not the specified physical page has been modified
 *	by any physical maps.
 */
bool
pmap_is_modified(struct vm_page *pg)
{
	return pmap_testbit(pg, PME_M);
}

/*
 * pmap_changebit:
 *
 *	Test-and-change various bits (including mod/ref bits).
 *	Returns the accumulated previously-set PME bits.
 */
static pm_entry_t
pmap_changebit(struct vm_page *pg, pm_entry_t set, pm_entry_t mask)
{
	struct pv_entry *pv;
	pm_entry_t combined_pme, opme, npme;
	unsigned int pmeidx;

	PMAP_CRIT_ENTER();

	/*
	 * Since we need to report if the page was mod/ref'd before
	 * we cleared the bit, we need to seed ourself with the current
	 * state in the vm_page in the event there are no mappings
	 * left to enumerate.
	 */
	combined_pme = VM_MDPAGE_MR(pg);

	/*
	 * Since we're running over every mapping for the page anyway,
	 * we might as well synchronize any attribute bits that we're
	 * not clearing.
	 */
	for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
		pmeidx = pme_index(pv->pv_pmeg, PV_VA(pv));
		opme = pgmmu_getpme(pmeidx);
		npme = (opme | set) & mask;
		combined_pme |= opme;
		if (opme != npme) {
			pgmmu_setpme(pmeidx, npme);
		}
	}

	/*
	 * Update any attributes we looked at, clear the ones we're clearing.
	 */
	VM_MDPAGE_SET_MR(pg, (combined_pme | set) & mask);

	PMAP_CRIT_EXIT();

	return combined_pme;
}

/*
 * pmap_clear_modify:		[ INTERFACE ]
 *
 *	Clear the modify bits on the specified physical page.
 */
bool
pmap_clear_modify(struct vm_page *pg)
{
	return (pmap_changebit(pg, 0, (pm_entry_t)~PME_M) & PME_M) != 0;
}

/*
 * pmap_clear_reference:	[ INTERFACE ]
 *
 *	Clear the reference bit on the specified physical page.
 */
bool
pmap_clear_reference(struct vm_page *pg)
{
	return (pmap_changebit(pg, 0, (pm_entry_t)~PME_R) & PME_R) != 0;
}

/*
 * pmap_phys_address:		[ INTERFACE ]
 *
 *	Return the physical address corresponding to the specified
 *	cookie.  Used by the device pager to decode a device driver's
 *	mmap entry point return value.
 */
paddr_t
pmap_phys_address(paddr_t cookie)
{
	return pgmmu_ptob(cookie);
}

/*
 * pmap_init_kcore_hdr:
 *
 *	Initialize the m68k kernel crash dump header with information
 *	necessary to perform KVA -> phys translations.
 *
 *	Returns a pointer to the crash dump RAM segment entries for
 *	machine-specific code to initialize.
 */
phys_ram_seg_t *
pmap_init_kcore_hdr(cpu_kcore_hdr_t *h)
{
	return NULL;
}

#if defined(DDB) || defined(KGDB)
/*
 * pmap_db_write_text_enter:
 *
 *	Temporarily map a page of kernel text read-write for the
 *	kernel debugger.
 */
bool
pmap_db_write_text_enter(vaddr_t pgva, struct pmap_db_write_text_context *ctx)
{
	sm_entry_t sme = pgmmu_getsme0(pgva);
	if (! sme_valid_p(sme)) {
		return false;
	}

	unsigned int pmeidx = pme_index(sme_pmeg(sme), pgva);
	pm_entry_t opme = pgmmu_getpme(pmeidx);
	if (! pme_valid_p(opme)) {
		return false;
	}

	pm_entry_t npme = opme | PME_W;
	pgmmu_setpme(pmeidx, npme);

	ctx->pmeidx = pmeidx;
	ctx->opme = opme;

	return true;
}

/*
 * pmap_db_write_text_exit:
 *
 *	Undo the effects of pmap_db_write_text_enter().
 */
void
pmap_db_write_text_exit(struct pmap_db_write_text_context *ctx)
{
	pgmmu_setpme(ctx->pmeidx, ctx->opme);
}
#endif /* DDB || KGDB */

/***************************** PMAP BOOTSTRAP ********************************/

extern char *	kernel_text;
extern char *	etext;

static vaddr_t	lwp0uarea;
       char *   vmmap;

/* XXX Doesn't belong here. */
paddr_t		avail_start;	/* PA of first available physical page */
paddr_t		avail_end;	/* PA of last available physical page */

/*
 * This structure is used to save firmware mappings that the kernel
 * can also use.
 */
struct pmap_static_mapping {
	vaddr_t		psm_va;
	paddr_t		psm_pa;
	size_t		psm_size;
	pm_entry_t	psm_pme;
};

#define	MAX_STATIC_MAPPINGS	8
static struct pmap_static_mapping static_mappings[MAX_STATIC_MAPPINGS];
static int num_static_mappings;

/*
 * pmap_add_static_mapping:
 *
 *	Add a VA != PA static mapping to the table.  This is done
 *	page-by-page, and may extend an existing entry.
 */
static bool
pmap_add_static_mapping(vaddr_t va, paddr_t pa, pm_entry_t pme)
{
	int i;

	/* only care about writability */
	pme &= PME_W;

	/*
	 * First check to see if this extends an existing entry.
	 */
	for (i = 0; i < num_static_mappings; i++) {
		if (va  == static_mappings[i].psm_va + PAGE_SIZE &&
		    pa  == static_mappings[i].psm_pa + PAGE_SIZE &&
		    pme == static_mappings[i].psm_pme) {
			static_mappings[i].psm_size += PAGE_SIZE;
			return true;
		}
	}

	/*
	 * Create a new entry.
	 */
	if (num_static_mappings == MAX_STATIC_MAPPINGS) {
		return false;
	}

	static_mappings[i].psm_va = va;
	static_mappings[i].psm_pa = pa;
	static_mappings[i].psm_pme = pme;
	static_mappings[i].psm_size = PAGE_SIZE;
	num_static_mappings++;

	return true;
}

/*
 * pmap_pa_has_static_mapping:
 *
 *	Returns true if the specified PA (and length) has a static mapping
 *	with the requested permission.  PMAP_* flags corresponding to the
 *	mapping's properties are returned via *flagsp.
 */
bool
pmap_pa_has_static_mapping(paddr_t pa, size_t len, vm_prot_t prot,
    vaddr_t *vap, int *flagsp)
{
	paddr_t lastpg = pgmmu_btop(pa + (len - 1));
	paddr_t firstpg = pgmmu_btop(pa);
	paddr_t tfirst, tlast;
	bool need_write = !!(prot & UVM_PROT_WRITE);
	int i;

	for (i = 0; i < num_static_mappings; i++) {
		tfirst = pgmmu_btop(static_mappings[i].psm_pa);
		tlast = pgmmu_btop(static_mappings[i].psm_pa +
		    (static_mappings[i].psm_size - 1));

		if (firstpg >= tfirst && lastpg <= tlast) {
			if (need_write &&
			    (static_mappings[i].psm_pme & PME_W) == 0) {
				return false;
			}
			*vap = static_mappings[i].psm_va +
			    (pa - static_mappings[i].psm_pa);
			/*
			 * No systems with this MMU have a data cache,
			 * so always indocate PMAP_NOCACHE to anyone
			 * making inquiries.
			 */
			*flagsp = PMAP_NOCACHE;
			return true;
		}
	}

	return false;
}

/*
 * pmap_va_is_static_mapping:
 *
 *	Returns true if the specified VA (and length) is a static
 *	mapping.
 */
bool
pmap_va_is_static_mapping(vaddr_t va, size_t len)
{
	vaddr_t lastpg = pgmmu_btop(va + (len - 1));
	vaddr_t firstpg = pgmmu_btop(va);
	vaddr_t tfirst, tlast;
	int i;

	for (i = 0; i < num_static_mappings; i++) {
		tfirst = pgmmu_btop(static_mappings[i].psm_va);
		tlast = pgmmu_btop(static_mappings[i].psm_va +
		    (static_mappings[i].psm_size - 1));

		if (firstpg >= tfirst && lastpg <= tlast) {
			return true;
		}
	}

	return false;
}

/*
 * pmap_bootstrap1:
 *
 *	Phase 1 of bootstrapping virtual memory.  For this implementation,
 *	the MMU is already enabled and we are running on the mappings
 *	set up by the firmware.  We need to initialize some of our
 *	data structures, and preserve / adjust some of the mappings that
 *	already exist.
 *
 *	N.B. reloff is unused in this implementation because the MMU
 *	is already on and thus manual relocations are not necessary.
 */
paddr_t __attribute__((no_instrument_function))
pmap_bootstrap1(paddr_t nextpa, paddr_t reloff __unused)
{
	int i, seg, pmeg;
	paddr_t pa;
	paddr_t lwp0upa;
	sm_entry_t sme;
	pm_entry_t pme;
	vaddr_t va;
	vaddr_t nextva;
	vaddr_t endva;
	vaddr_t alloc_startva;
	int entry_count = 0;

	/* Initialize the kernel pmap. */
	pmap_pinit(pmap_kernel(), &static_contexts[0]);
	pmap_kernel()->pm_busy = 1;	/* kernel pmap starts out busy */

	/* Initialize the pmeg bitmap. */
	pmap_bitmap_init(&pmeg_bitmap);

	TAILQ_INIT(&pmap_all_user_pmaps);

	/*
	 * Time to tidy up all of the various mappings left for us by the
	 * firmware.
	 *
	 * First pass through SegMap0, claim all of the pmegs that are
	 * currently in-use; we'll use them for kernel mappings, and
	 * they're all pre-allocated.
	 */
	for (seg = 0, va = 0; seg < PGMMU_NUM_SEGS;
	     seg++, va += PGMMU_SEG_SIZE) {
		sme = pgmmu_getsme0(va);
		if (sme_valid_p(sme)) {
			pmap_bitmap_claim(&pmeg_bitmap, sme_pmeg(sme));
		}
	}

	/*
	 * Now we know which pmegs are in-use, we can go through the
	 * entire PageMap and zero-initialize all not-in-use entries.
	 */
	for (pmeg = 0; pmeg < PGMMU_NUM_PMEGS; pmeg++) {
		if (pmap_bitmap_claimed_p(&pmeg_bitmap, pmeg)) {
			continue;
		}
		for (i = 0; i < PGMMU_PMEG_SIZE; i++) {
			pgmmu_setpme((pmeg << PGMMU_PMEG_SHIFT) + i, 0);
		}
	}

	/*
	 * Now go back through SegMap0 and assign pmegs to each segment
	 * that doesn't already have one.
	 */
	for (seg = 0, va = 0; seg < PGMMU_NUM_SEGS;
	     seg++, va += PGMMU_SEG_SIZE) {
		sme = pgmmu_getsme0(va);
		if (! sme_valid_p(sme)) {
			pmeg = pmap_bitmap_alloc(&pmeg_bitmap);
			pgmmu_setsme0(va, SME_V | pmeg);
		}
	}

	/*
	 * Now go through the SegMaps for all of the user contexts
	 * and zero-initialize them.
	 */
	for (i = 1; i < PGMMU_NUM_CONTEXTS; i++) {
		pgmmu_setcontext(i);
		for (seg = 0, va = 0; seg < PGMMU_NUM_SEGS;
		     seg++, va += PGMMU_SEG_SIZE) {
			pgmmu_setsme(va, 0);
		}
	}

	pgmmu_setcontext(0);

	/*
	 * The system firmware has done a few things:
	 *
	 * ==> (1) Mapped all base RAM VA==PA.  The kernel has been loaded
	 *     here.  We need to invalidate the mappings before and after
	 *     the kernel so that we can use the VA space for our own purposes.
	 *     NOTE: The stack that we were using when we entered the kernel
	 *     is located somewhere in there, so we need to have switched
	 *     to a temporary stack within the base kernel image before
	 *     getting to pmap_bootstrap1().
	 *
	 * ==> (2) Mapped the ROM somewhere in the top 1MB of Context 0
	 *     (not necessarily the entire 1MB).
	 *
	 * ==> (3) Mapped the on-board devices and firmware reserved memory
	 *     just below the ROM.
	 *
	 * For (2) and (3), the mappings are not VA==PA.  So, what we're
	 * going to do is preserve all VA!=PA mappings, and then re-use them
	 * whenever asked for them by others (like when drivers map devices),
	 * and we'll clear the mappings for areas not-the-kernel in the
	 * VA==PA areas.  For the kernel text, we'll fix up the mappings to
	 * be read-only.
	 *
	 * The physical pages before the kernel will be re-used for the
	 * lwp0 u-area.  We've arranged for the kernel to be linked at
	 * 0 + USPACE in order to faciliate this, and we know this will
	 * be in the first segment.
	 *
	 * For all mappings that we preserve, we also ensure that the K
	 * bit is set.
	 *
	 * XXX We're making assumptions about the system firmware and memory
	 * map here, but I can count on one finger the number of systems that
	 * use this MMU.
	 */

	/* Unmap the region before the kernel text. */
	endva = pgmmu_trunc_page(&kernel_text);
	lwp0upa = endva - USPACE;
	for (va = 0; va < endva; va += PAGE_SIZE) {
		pmap_setkpme(va, 0);
	}

	/* Fix kernel text to be read-only. */
	endva = pgmmu_trunc_page(&etext);
	for (; va < endva; va += PAGE_SIZE) {
		pme = pmap_getkpme(va);
		pmap_setkpme(va, (pme & ~PME_W) | PME_K);
		entry_count++;
	}

	/* Fixup priv on the rest of the kernel. */
	endva = alloc_startva = nextpa = pgmmu_round_page(nextpa);
	for (; va < endva; va += PAGE_SIZE) {
		pme = pmap_getkpme(va);
		pmap_setkpme(va, pme | PME_W | PME_K);
		entry_count++;
	}

	/*
	 * Now walk every remaining kernel PME and check to see if
	 * it's a VA != PA mapping.  If so, preserve it (and clamp
	 * the max kernel virtual address as necessary).
	 */
	for (; va < KERNEL_MAX_ADDRESS && va >= alloc_startva;
	     va += PAGE_SIZE) {
		pme = pmap_getkpme(va);
		if (! pme_valid_p(pme)) {
			continue;
		}
		pa = pme_pa(pme);
		if (va == pa) {
			pmap_setkpme(va, 0);
			continue;
		}

		/* Clamp the max kernel virtual address. */
		if (va < kernel_virtual_max) {
			kernel_virtual_max = va;
		}

		if (! pmap_add_static_mapping(va, pa, pme)) {
			/* XXX log a warning? */
		}

		/* Ensure it's kernel-only. */
		pmap_setkpme(va, pme | PME_K);
		entry_count++;
	}

	/*
	 * Allocate / map some special purpose VAs:
	 */
	nextva = alloc_startva;

	/* lwp0 u-area. */
	lwp0uarea = nextva;
	nextva += USPACE;

	pme = pmap_make_pme(pmap_kernel(), lwp0upa,
	    UVM_PROT_READ|UVM_PROT_WRITE, PMAP_WIRED);
	for (va = lwp0uarea; va < nextva; va += PAGE_SIZE) {
		pmap_setkpme(va, pme);
		pme++;			/* increment PFN field */
		entry_count++;
	}

	/* pmap temporary map addresses */
	pmap_tmpmap_srcva = nextva;
	nextva += PAGE_SIZE;
	sme = pgmmu_getsme0(pmap_tmpmap_srcva);
	pmap_tmpmap_srcidx = pme_index(sme_pmeg(sme), pmap_tmpmap_srcva);

	pmap_tmpmap_dstva = nextva;
	nextva += PAGE_SIZE;
	sme = pgmmu_getsme0(pmap_tmpmap_dstva);
	pmap_tmpmap_dstidx = pme_index(sme_pmeg(sme), pmap_tmpmap_dstva);

	/* vmmap temporary map address */
	vmmap = (char *)nextva;
	nextva += PAGE_SIZE;

	/* kernel message buffer */
	msgbufaddr = (char *)nextva;
	nextva += pgmmu_round_page(MSGBUFSIZE);

	/* UVM-managed kernel virtual starts here. */
	kernel_virtual_start = nextva;

	/*
	 * Record the number of wired mappings we create above
	 * in the kernel pmap stats.
	 */
	pmap_kernel()->pm_stats.resident_count = entry_count;
	pmap_kernel()->pm_stats.wired_count = entry_count;

	return nextpa;
}

/*
 * pmap_bootstrap2:
 *
 *	Phase 2 of bootstrapping virtual memory.  For this implementation,
 *	we just have to finish setting up some run-time-computed global
 *	pmap data, plus the lwp0 u-area, curlwp, and curpcb.
 *
 *	Returns the new kernel %sp value for lwp0.
 */
void *
pmap_bootstrap2(void)
{
	/* Early low-level UVM initialization. */
	uvmexp.pagesize = PAGE_SIZE;
	uvm_md_init();

	/* Initialize lwp0 u-area, curlwp, and curpcb. */
	memset((void *)lwp0uarea, 0, USPACE);
	uvm_lwp_setuarea(&lwp0, lwp0uarea);
	curlwp = &lwp0;
	curpcb = lwp_getpcb(&lwp0);

	/* Create a fake exception frame so that cpu_lwp_fork() can copy it. */
	struct trapframe *tf = (struct trapframe *)(lwp0uarea + USPACE) - 1;
	tf->tf_sr = PSL_USER;
	lwp0.l_md.md_regs = (int *)tf;

	/*
	 * Initialize the source/destination control registers for
	 * movs.
	 */
	setsfc(FC_USERD);
	setdfc(FC_USERD);

	return tf;
}
