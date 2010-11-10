/*	$NetBSD: pmap.c,v 1.218 2010/11/10 09:27:22 uebayasi Exp $	*/

/*
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 * Copyright (c) 2002-2003 Wasabi Systems, Inc.
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
 * armv6 and VIPT cache support by 3am Software Foundry,
 * Copyright (c) 2007 Microsoft
 */

/*
 * Performance improvements, UVM changes, overhauls and part-rewrites
 * were contributed by Neil A. Carson <neil@causality.com>.
 */

/*
 * Overhauled again to speedup the pmap, use MMU Domains so that L1 tables
 * can be shared, and re-work the KVM layout, by Steve Woodford of Wasabi
 * Systems, Inc.
 *
 * There are still a few things outstanding at this time:
 *
 *   - There are some unresolved issues for MP systems:
 *
 *     o The L1 metadata needs a lock, or more specifically, some places
 *       need to acquire an exclusive lock when modifying L1 translation
 *       table entries.
 *
 *     o When one cpu modifies an L1 entry, and that L1 table is also
 *       being used by another cpu, then the latter will need to be told
 *       that a tlb invalidation may be necessary. (But only if the old
 *       domain number in the L1 entry being over-written is currently
 *       the active domain on that cpu). I guess there are lots more tlb
 *       shootdown issues too...
 *
 *     o If the vector_page is at 0x00000000 instead of 0xffff0000, then
 *       MP systems will lose big-time because of the MMU domain hack.
 *       The only way this can be solved (apart from moving the vector
 *       page to 0xffff0000) is to reserve the first 1MB of user address
 *       space for kernel use only. This would require re-linking all
 *       applications so that the text section starts above this 1MB
 *       boundary.
 *
 *     o Tracking which VM space is resident in the cache/tlb has not yet
 *       been implemented for MP systems.
 *
 *     o Finally, there is a pathological condition where two cpus running
 *       two separate processes (not lwps) which happen to share an L1
 *       can get into a fight over one or more L1 entries. This will result
 *       in a significant slow-down if both processes are in tight loops.
 */

/*
 * Special compilation symbols
 * PMAP_DEBUG		- Build in pmap_debug_level code
 */

/* Include header files */

#include "opt_cpuoptions.h"
#include "opt_pmap_debug.h"
#include "opt_ddb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/cdefs.h>
#include <sys/cpu.h>
#include <sys/sysctl.h>
 
#include <uvm/uvm.h>

#include <machine/bus.h>
#include <machine/pmap.h>
#include <machine/pcb.h>
#include <machine/param.h>
#include <arm/arm32/katelib.h>

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.218 2010/11/10 09:27:22 uebayasi Exp $");

#define	VM_PAGE_TO_MD(pg)	(&(pg)->mdpage)

#ifdef PMAP_DEBUG

/* XXX need to get rid of all refs to this */
int pmap_debug_level = 0;

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
#define	PDB_VAC		0x10000
#define	PDB_KENTER	0x20000
#define	PDB_KREMOVE	0x40000
#define	PDB_EXEC	0x80000

int debugmap = 1;
int pmapdebug = 0; 
#define	NPDEBUG(_lev_,_stat_) \
	if (pmapdebug & (_lev_)) \
        	((_stat_))
    
#else	/* PMAP_DEBUG */
#define NPDEBUG(_lev_,_stat_) /* Nothing */
#endif	/* PMAP_DEBUG */

/*
 * pmap_kernel() points here
 */
static struct pmap	kernel_pmap_store;
struct pmap		*const kernel_pmap_ptr = &kernel_pmap_store;

/*
 * Which pmap is currently 'live' in the cache
 *
 * XXXSCW: Fix for SMP ...
 */
static pmap_t pmap_recent_user;

/*
 * Pointer to last active lwp, or NULL if it exited.
 */
struct lwp *pmap_previous_active_lwp;

/*
 * Pool and cache that pmap structures are allocated from.
 * We use a cache to avoid clearing the pm_l2[] array (1KB)
 * in pmap_create().
 */
static struct pool_cache pmap_cache;
static LIST_HEAD(, pmap) pmap_pmaps;

/*
 * Pool of PV structures
 */
static struct pool pmap_pv_pool;
static void *pmap_bootstrap_pv_page_alloc(struct pool *, int);
static void pmap_bootstrap_pv_page_free(struct pool *, void *);
static struct pool_allocator pmap_bootstrap_pv_allocator = {
	pmap_bootstrap_pv_page_alloc, pmap_bootstrap_pv_page_free
};

/*
 * Pool and cache of l2_dtable structures.
 * We use a cache to avoid clearing the structures when they're
 * allocated. (196 bytes)
 */
static struct pool_cache pmap_l2dtable_cache;
static vaddr_t pmap_kernel_l2dtable_kva;

/*
 * Pool and cache of L2 page descriptors.
 * We use a cache to avoid clearing the descriptor table
 * when they're allocated. (1KB)
 */
static struct pool_cache pmap_l2ptp_cache;
static vaddr_t pmap_kernel_l2ptp_kva;
static paddr_t pmap_kernel_l2ptp_phys;

#ifdef PMAPCOUNTERS
#define	PMAP_EVCNT_INITIALIZER(name) \
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", name)

#ifdef PMAP_CACHE_VIPT
static struct evcnt pmap_ev_vac_clean_one =
   PMAP_EVCNT_INITIALIZER("clean page (1 color)");
static struct evcnt pmap_ev_vac_flush_one =
   PMAP_EVCNT_INITIALIZER("flush page (1 color)");
static struct evcnt pmap_ev_vac_flush_lots =
   PMAP_EVCNT_INITIALIZER("flush page (2+ colors)");
static struct evcnt pmap_ev_vac_flush_lots2 =
   PMAP_EVCNT_INITIALIZER("flush page (2+ colors, kmpage)");
EVCNT_ATTACH_STATIC(pmap_ev_vac_clean_one);
EVCNT_ATTACH_STATIC(pmap_ev_vac_flush_one);
EVCNT_ATTACH_STATIC(pmap_ev_vac_flush_lots);
EVCNT_ATTACH_STATIC(pmap_ev_vac_flush_lots2);

static struct evcnt pmap_ev_vac_color_new =
   PMAP_EVCNT_INITIALIZER("new page color");
static struct evcnt pmap_ev_vac_color_reuse =
   PMAP_EVCNT_INITIALIZER("ok first page color");
static struct evcnt pmap_ev_vac_color_ok =
   PMAP_EVCNT_INITIALIZER("ok page color");
static struct evcnt pmap_ev_vac_color_blind =
   PMAP_EVCNT_INITIALIZER("blind page color");
static struct evcnt pmap_ev_vac_color_change =
   PMAP_EVCNT_INITIALIZER("change page color");
static struct evcnt pmap_ev_vac_color_erase =
   PMAP_EVCNT_INITIALIZER("erase page color");
static struct evcnt pmap_ev_vac_color_none =
   PMAP_EVCNT_INITIALIZER("no page color");
static struct evcnt pmap_ev_vac_color_restore =
   PMAP_EVCNT_INITIALIZER("restore page color");

EVCNT_ATTACH_STATIC(pmap_ev_vac_color_new);
EVCNT_ATTACH_STATIC(pmap_ev_vac_color_reuse);
EVCNT_ATTACH_STATIC(pmap_ev_vac_color_ok);
EVCNT_ATTACH_STATIC(pmap_ev_vac_color_blind);
EVCNT_ATTACH_STATIC(pmap_ev_vac_color_change);
EVCNT_ATTACH_STATIC(pmap_ev_vac_color_erase);
EVCNT_ATTACH_STATIC(pmap_ev_vac_color_none);
EVCNT_ATTACH_STATIC(pmap_ev_vac_color_restore);
#endif

static struct evcnt pmap_ev_mappings =
   PMAP_EVCNT_INITIALIZER("pages mapped");
static struct evcnt pmap_ev_unmappings =
   PMAP_EVCNT_INITIALIZER("pages unmapped");
static struct evcnt pmap_ev_remappings =
   PMAP_EVCNT_INITIALIZER("pages remapped");

EVCNT_ATTACH_STATIC(pmap_ev_mappings);
EVCNT_ATTACH_STATIC(pmap_ev_unmappings);
EVCNT_ATTACH_STATIC(pmap_ev_remappings);

static struct evcnt pmap_ev_kernel_mappings =
   PMAP_EVCNT_INITIALIZER("kernel pages mapped");
static struct evcnt pmap_ev_kernel_unmappings =
   PMAP_EVCNT_INITIALIZER("kernel pages unmapped");
static struct evcnt pmap_ev_kernel_remappings =
   PMAP_EVCNT_INITIALIZER("kernel pages remapped");

EVCNT_ATTACH_STATIC(pmap_ev_kernel_mappings);
EVCNT_ATTACH_STATIC(pmap_ev_kernel_unmappings);
EVCNT_ATTACH_STATIC(pmap_ev_kernel_remappings);

static struct evcnt pmap_ev_kenter_mappings =
   PMAP_EVCNT_INITIALIZER("kenter pages mapped");
static struct evcnt pmap_ev_kenter_unmappings =
   PMAP_EVCNT_INITIALIZER("kenter pages unmapped");
static struct evcnt pmap_ev_kenter_remappings =
   PMAP_EVCNT_INITIALIZER("kenter pages remapped");
static struct evcnt pmap_ev_pt_mappings =
   PMAP_EVCNT_INITIALIZER("page table pages mapped");

EVCNT_ATTACH_STATIC(pmap_ev_kenter_mappings);
EVCNT_ATTACH_STATIC(pmap_ev_kenter_unmappings);
EVCNT_ATTACH_STATIC(pmap_ev_kenter_remappings);
EVCNT_ATTACH_STATIC(pmap_ev_pt_mappings);

#ifdef PMAP_CACHE_VIPT
static struct evcnt pmap_ev_exec_mappings =
   PMAP_EVCNT_INITIALIZER("exec pages mapped");
static struct evcnt pmap_ev_exec_cached =
   PMAP_EVCNT_INITIALIZER("exec pages cached");

EVCNT_ATTACH_STATIC(pmap_ev_exec_mappings);
EVCNT_ATTACH_STATIC(pmap_ev_exec_cached);

static struct evcnt pmap_ev_exec_synced =
   PMAP_EVCNT_INITIALIZER("exec pages synced");
static struct evcnt pmap_ev_exec_synced_map =
   PMAP_EVCNT_INITIALIZER("exec pages synced (MP)");
static struct evcnt pmap_ev_exec_synced_unmap =
   PMAP_EVCNT_INITIALIZER("exec pages synced (UM)");
static struct evcnt pmap_ev_exec_synced_remap =
   PMAP_EVCNT_INITIALIZER("exec pages synced (RM)");
static struct evcnt pmap_ev_exec_synced_clearbit =
   PMAP_EVCNT_INITIALIZER("exec pages synced (DG)");
static struct evcnt pmap_ev_exec_synced_kremove =
   PMAP_EVCNT_INITIALIZER("exec pages synced (KU)");

EVCNT_ATTACH_STATIC(pmap_ev_exec_synced);
EVCNT_ATTACH_STATIC(pmap_ev_exec_synced_map);
EVCNT_ATTACH_STATIC(pmap_ev_exec_synced_unmap);
EVCNT_ATTACH_STATIC(pmap_ev_exec_synced_remap);
EVCNT_ATTACH_STATIC(pmap_ev_exec_synced_clearbit);
EVCNT_ATTACH_STATIC(pmap_ev_exec_synced_kremove);

static struct evcnt pmap_ev_exec_discarded_unmap =
   PMAP_EVCNT_INITIALIZER("exec pages discarded (UM)");
static struct evcnt pmap_ev_exec_discarded_zero =
   PMAP_EVCNT_INITIALIZER("exec pages discarded (ZP)");
static struct evcnt pmap_ev_exec_discarded_copy =
   PMAP_EVCNT_INITIALIZER("exec pages discarded (CP)");
static struct evcnt pmap_ev_exec_discarded_page_protect =
   PMAP_EVCNT_INITIALIZER("exec pages discarded (PP)");
static struct evcnt pmap_ev_exec_discarded_clearbit =
   PMAP_EVCNT_INITIALIZER("exec pages discarded (DG)");
static struct evcnt pmap_ev_exec_discarded_kremove =
   PMAP_EVCNT_INITIALIZER("exec pages discarded (KU)");

EVCNT_ATTACH_STATIC(pmap_ev_exec_discarded_unmap);
EVCNT_ATTACH_STATIC(pmap_ev_exec_discarded_zero);
EVCNT_ATTACH_STATIC(pmap_ev_exec_discarded_copy);
EVCNT_ATTACH_STATIC(pmap_ev_exec_discarded_page_protect);
EVCNT_ATTACH_STATIC(pmap_ev_exec_discarded_clearbit);
EVCNT_ATTACH_STATIC(pmap_ev_exec_discarded_kremove);
#endif /* PMAP_CACHE_VIPT */

static struct evcnt pmap_ev_updates = PMAP_EVCNT_INITIALIZER("updates");
static struct evcnt pmap_ev_collects = PMAP_EVCNT_INITIALIZER("collects");
static struct evcnt pmap_ev_activations = PMAP_EVCNT_INITIALIZER("activations");

EVCNT_ATTACH_STATIC(pmap_ev_updates);
EVCNT_ATTACH_STATIC(pmap_ev_collects);
EVCNT_ATTACH_STATIC(pmap_ev_activations);

#define	PMAPCOUNT(x)	((void)(pmap_ev_##x.ev_count++))
#else
#define	PMAPCOUNT(x)	((void)0)
#endif

/*
 * pmap copy/zero page, and mem(5) hook point
 */
static pt_entry_t *csrc_pte, *cdst_pte;
static vaddr_t csrcp, cdstp;
vaddr_t memhook;			/* used by mem.c */
kmutex_t memlock;			/* used by mem.c */
void *zeropage;				/* used by mem.c */
extern void *msgbufaddr;
int pmap_kmpages;
/*
 * Flag to indicate if pmap_init() has done its thing
 */
bool pmap_initialized;

/*
 * Misc. locking data structures
 */

#if 0 /* defined(MULTIPROCESSOR) || defined(LOCKDEBUG) */
static struct lock pmap_main_lock;

#define PMAP_MAP_TO_HEAD_LOCK() \
     (void) spinlockmgr(&pmap_main_lock, LK_SHARED, NULL)
#define PMAP_MAP_TO_HEAD_UNLOCK() \
     (void) spinlockmgr(&pmap_main_lock, LK_RELEASE, NULL)
#define PMAP_HEAD_TO_MAP_LOCK() \
     (void) spinlockmgr(&pmap_main_lock, LK_EXCLUSIVE, NULL)
#define PMAP_HEAD_TO_MAP_UNLOCK() \
     spinlockmgr(&pmap_main_lock, LK_RELEASE, (void *) 0)
#else
#define PMAP_MAP_TO_HEAD_LOCK()		/* null */
#define PMAP_MAP_TO_HEAD_UNLOCK()	/* null */
#define PMAP_HEAD_TO_MAP_LOCK()		/* null */
#define PMAP_HEAD_TO_MAP_UNLOCK()	/* null */
#endif

#define	pmap_acquire_pmap_lock(pm)			\
	do {						\
		if ((pm) != pmap_kernel())		\
			mutex_enter(&(pm)->pm_lock);	\
	} while (/*CONSTCOND*/0)

#define	pmap_release_pmap_lock(pm)			\
	do {						\
		if ((pm) != pmap_kernel())		\
			mutex_exit(&(pm)->pm_lock);	\
	} while (/*CONSTCOND*/0)


/*
 * Metadata for L1 translation tables.
 */
struct l1_ttable {
	/* Entry on the L1 Table list */
	SLIST_ENTRY(l1_ttable) l1_link;

	/* Entry on the L1 Least Recently Used list */
	TAILQ_ENTRY(l1_ttable) l1_lru;

	/* Track how many domains are allocated from this L1 */
	volatile u_int l1_domain_use_count;

	/*
	 * A free-list of domain numbers for this L1.
	 * We avoid using ffs() and a bitmap to track domains since ffs()
	 * is slow on ARM.
	 */
	u_int8_t l1_domain_first;
	u_int8_t l1_domain_free[PMAP_DOMAINS];

	/* Physical address of this L1 page table */
	paddr_t l1_physaddr;

	/* KVA of this L1 page table */
	pd_entry_t *l1_kva;
};

/*
 * Convert a virtual address into its L1 table index. That is, the
 * index used to locate the L2 descriptor table pointer in an L1 table.
 * This is basically used to index l1->l1_kva[].
 *
 * Each L2 descriptor table represents 1MB of VA space.
 */
#define	L1_IDX(va)		(((vaddr_t)(va)) >> L1_S_SHIFT)

/*
 * L1 Page Tables are tracked using a Least Recently Used list.
 *  - New L1s are allocated from the HEAD.
 *  - Freed L1s are added to the TAIl.
 *  - Recently accessed L1s (where an 'access' is some change to one of
 *    the userland pmaps which owns this L1) are moved to the TAIL.
 */
static TAILQ_HEAD(, l1_ttable) l1_lru_list;
static struct simplelock l1_lru_lock;

/*
 * A list of all L1 tables
 */
static SLIST_HEAD(, l1_ttable) l1_list;

/*
 * The l2_dtable tracks L2_BUCKET_SIZE worth of L1 slots.
 *
 * This is normally 16MB worth L2 page descriptors for any given pmap.
 * Reference counts are maintained for L2 descriptors so they can be
 * freed when empty.
 */
struct l2_dtable {
	/* The number of L2 page descriptors allocated to this l2_dtable */
	u_int l2_occupancy;

	/* List of L2 page descriptors */
	struct l2_bucket {
		pt_entry_t *l2b_kva;	/* KVA of L2 Descriptor Table */
		paddr_t l2b_phys;	/* Physical address of same */
		u_short l2b_l1idx;	/* This L2 table's L1 index */
		u_short l2b_occupancy;	/* How many active descriptors */
	} l2_bucket[L2_BUCKET_SIZE];
};

/*
 * Given an L1 table index, calculate the corresponding l2_dtable index
 * and bucket index within the l2_dtable.
 */
#define	L2_IDX(l1idx)		(((l1idx) >> L2_BUCKET_LOG2) & \
				 (L2_SIZE - 1))
#define	L2_BUCKET(l1idx)	((l1idx) & (L2_BUCKET_SIZE - 1))

/*
 * Given a virtual address, this macro returns the
 * virtual address required to drop into the next L2 bucket.
 */
#define	L2_NEXT_BUCKET(va)	(((va) & L1_S_FRAME) + L1_S_SIZE)

/*
 * L2 allocation.
 */
#define	pmap_alloc_l2_dtable()		\
	    pool_cache_get(&pmap_l2dtable_cache, PR_NOWAIT)
#define	pmap_free_l2_dtable(l2)		\
	    pool_cache_put(&pmap_l2dtable_cache, (l2))
#define pmap_alloc_l2_ptp(pap)		\
	    ((pt_entry_t *)pool_cache_get_paddr(&pmap_l2ptp_cache,\
	    PR_NOWAIT, (pap)))

/*
 * We try to map the page tables write-through, if possible.  However, not
 * all CPUs have a write-through cache mode, so on those we have to sync
 * the cache when we frob page tables.
 *
 * We try to evaluate this at compile time, if possible.  However, it's
 * not always possible to do that, hence this run-time var.
 */
int	pmap_needs_pte_sync;

/*
 * Real definition of pv_entry.
 */
struct pv_entry {
	SLIST_ENTRY(pv_entry) pv_link;	/* next pv_entry */
	pmap_t		pv_pmap;        /* pmap where mapping lies */
	vaddr_t		pv_va;          /* virtual address for mapping */
	u_int		pv_flags;       /* flags */
};

/*
 * Macro to determine if a mapping might be resident in the
 * instruction cache and/or TLB
 */
#define	PV_BEEN_EXECD(f)  (((f) & (PVF_REF | PVF_EXEC)) == (PVF_REF | PVF_EXEC))
#define	PV_IS_EXEC_P(f)   (((f) & PVF_EXEC) != 0)

/*
 * Macro to determine if a mapping might be resident in the
 * data cache and/or TLB
 */
#define	PV_BEEN_REFD(f)   (((f) & PVF_REF) != 0)

/*
 * Local prototypes
 */
static int		pmap_set_pt_cache_mode(pd_entry_t *, vaddr_t);
static void		pmap_alloc_specials(vaddr_t *, int, vaddr_t *,
			    pt_entry_t **);
static bool		pmap_is_current(pmap_t);
static bool		pmap_is_cached(pmap_t);
static void		pmap_enter_pv(struct vm_page_md *, paddr_t, struct pv_entry *,
			    pmap_t, vaddr_t, u_int);
static struct pv_entry *pmap_find_pv(struct vm_page_md *, pmap_t, vaddr_t);
static struct pv_entry *pmap_remove_pv(struct vm_page_md *, paddr_t, pmap_t, vaddr_t);
static u_int		pmap_modify_pv(struct vm_page_md *, paddr_t, pmap_t, vaddr_t,
			    u_int, u_int);

static void		pmap_pinit(pmap_t);
static int		pmap_pmap_ctor(void *, void *, int);

static void		pmap_alloc_l1(pmap_t);
static void		pmap_free_l1(pmap_t);
static void		pmap_use_l1(pmap_t);

static struct l2_bucket *pmap_get_l2_bucket(pmap_t, vaddr_t);
static struct l2_bucket *pmap_alloc_l2_bucket(pmap_t, vaddr_t);
static void		pmap_free_l2_bucket(pmap_t, struct l2_bucket *, u_int);
static int		pmap_l2ptp_ctor(void *, void *, int);
static int		pmap_l2dtable_ctor(void *, void *, int);

static void		pmap_vac_me_harder(struct vm_page_md *, paddr_t, pmap_t, vaddr_t);
#ifdef PMAP_CACHE_VIVT
static void		pmap_vac_me_kpmap(struct vm_page_md *, paddr_t, pmap_t, vaddr_t);
static void		pmap_vac_me_user(struct vm_page_md *, paddr_t, pmap_t, vaddr_t);
#endif

static void		pmap_clearbit(struct vm_page_md *, paddr_t, u_int);
#ifdef PMAP_CACHE_VIVT
static int		pmap_clean_page(struct pv_entry *, bool);
#endif
#ifdef PMAP_CACHE_VIPT
static void		pmap_syncicache_page(struct vm_page_md *, paddr_t);
enum pmap_flush_op {
	PMAP_FLUSH_PRIMARY,
	PMAP_FLUSH_SECONDARY,
	PMAP_CLEAN_PRIMARY
};
static void		pmap_flush_page(struct vm_page_md *, paddr_t, enum pmap_flush_op);
#endif
static void		pmap_page_remove(struct vm_page_md *, paddr_t);

static void		pmap_init_l1(struct l1_ttable *, pd_entry_t *);
static vaddr_t		kernel_pt_lookup(paddr_t);


/*
 * External function prototypes
 */
extern void bzero_page(vaddr_t);
extern void bcopy_page(vaddr_t, vaddr_t);

/*
 * Misc variables
 */
vaddr_t virtual_avail;
vaddr_t virtual_end;
vaddr_t pmap_curmaxkvaddr;

paddr_t avail_start;
paddr_t avail_end;

pv_addrqh_t pmap_boot_freeq = SLIST_HEAD_INITIALIZER(&pmap_boot_freeq);
pv_addr_t kernelpages;
pv_addr_t kernel_l1pt;
pv_addr_t systempage;

/* Function to set the debug level of the pmap code */

#ifdef PMAP_DEBUG
void
pmap_debug(int level)
{
	pmap_debug_level = level;
	printf("pmap_debug: level=%d\n", pmap_debug_level);
}
#endif	/* PMAP_DEBUG */

/*
 * A bunch of routines to conditionally flush the caches/TLB depending
 * on whether the specified pmap actually needs to be flushed at any
 * given time.
 */
static inline void
pmap_tlb_flushID_SE(pmap_t pm, vaddr_t va)
{

	if (pm->pm_cstate.cs_tlb_id)
		cpu_tlb_flushID_SE(va);
}

static inline void
pmap_tlb_flushD_SE(pmap_t pm, vaddr_t va)
{

	if (pm->pm_cstate.cs_tlb_d)
		cpu_tlb_flushD_SE(va);
}

static inline void
pmap_tlb_flushID(pmap_t pm)
{

	if (pm->pm_cstate.cs_tlb_id) {
		cpu_tlb_flushID();
		pm->pm_cstate.cs_tlb = 0;
	}
}

static inline void
pmap_tlb_flushD(pmap_t pm)
{

	if (pm->pm_cstate.cs_tlb_d) {
		cpu_tlb_flushD();
		pm->pm_cstate.cs_tlb_d = 0;
	}
}

#ifdef PMAP_CACHE_VIVT
static inline void
pmap_idcache_wbinv_range(pmap_t pm, vaddr_t va, vsize_t len)
{
	if (pm->pm_cstate.cs_cache_id) {
		cpu_idcache_wbinv_range(va, len);
	}
}

static inline void
pmap_dcache_wb_range(pmap_t pm, vaddr_t va, vsize_t len,
    bool do_inv, bool rd_only)
{

	if (pm->pm_cstate.cs_cache_d) {
		if (do_inv) {
			if (rd_only)
				cpu_dcache_inv_range(va, len);
			else
				cpu_dcache_wbinv_range(va, len);
		} else
		if (!rd_only)
			cpu_dcache_wb_range(va, len);
	}
}

static inline void
pmap_idcache_wbinv_all(pmap_t pm)
{
	if (pm->pm_cstate.cs_cache_id) {
		cpu_idcache_wbinv_all();
		pm->pm_cstate.cs_cache = 0;
	}
}

static inline void
pmap_dcache_wbinv_all(pmap_t pm)
{
	if (pm->pm_cstate.cs_cache_d) {
		cpu_dcache_wbinv_all();
		pm->pm_cstate.cs_cache_d = 0;
	}
}
#endif /* PMAP_CACHE_VIVT */

static inline bool
pmap_is_current(pmap_t pm)
{

	if (pm == pmap_kernel() || curproc->p_vmspace->vm_map.pmap == pm)
		return true;

	return false;
}

static inline bool
pmap_is_cached(pmap_t pm)
{

	if (pm == pmap_kernel() || pmap_recent_user == NULL ||
	    pmap_recent_user == pm)
		return (true);

	return false;
}

/*
 * PTE_SYNC_CURRENT:
 *
 *     Make sure the pte is written out to RAM.
 *     We need to do this for one of two cases:
 *       - We're dealing with the kernel pmap
 *       - There is no pmap active in the cache/tlb.
 *       - The specified pmap is 'active' in the cache/tlb.
 */
#ifdef PMAP_INCLUDE_PTE_SYNC
#define	PTE_SYNC_CURRENT(pm, ptep)	\
do {					\
	if (PMAP_NEEDS_PTE_SYNC && 	\
	    pmap_is_cached(pm))		\
		PTE_SYNC(ptep);		\
} while (/*CONSTCOND*/0)
#else
#define	PTE_SYNC_CURRENT(pm, ptep)	/* nothing */
#endif

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
static void
pmap_enter_pv(struct vm_page_md *md, paddr_t pa, struct pv_entry *pv, pmap_t pm,
    vaddr_t va, u_int flags)
{
	struct pv_entry **pvp;

	NPDEBUG(PDB_PVDUMP,
	    printf("pmap_enter_pv: pm %p, md %p, flags 0x%x\n", pm, md, flags));

	pv->pv_pmap = pm;
	pv->pv_va = va;
	pv->pv_flags = flags;

	simple_lock(&md->pvh_slock);	/* lock vm_page */
	pvp = &SLIST_FIRST(&md->pvh_list);
#ifdef PMAP_CACHE_VIPT
	/*
	 * Insert unmanaged entries, writeable first, at the head of
	 * the pv list.
	 */
	if (__predict_true((flags & PVF_KENTRY) == 0)) {
		while (*pvp != NULL && (*pvp)->pv_flags & PVF_KENTRY)
			pvp = &SLIST_NEXT(*pvp, pv_link);
	} else if ((flags & PVF_WRITE) == 0) {
		while (*pvp != NULL && (*pvp)->pv_flags & PVF_WRITE)
			pvp = &SLIST_NEXT(*pvp, pv_link);
	}
#endif
	SLIST_NEXT(pv, pv_link) = *pvp;		/* add to ... */
	*pvp = pv;				/* ... locked list */
	md->pvh_attrs |= flags & (PVF_REF | PVF_MOD);
#ifdef PMAP_CACHE_VIPT
	if ((pv->pv_flags & PVF_KWRITE) == PVF_KWRITE)
		md->pvh_attrs |= PVF_KMOD;
	if ((md->pvh_attrs & (PVF_DMOD|PVF_NC)) != PVF_NC)
		md->pvh_attrs |= PVF_DIRTY;
	KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
#endif
	if (pm == pmap_kernel()) {
		PMAPCOUNT(kernel_mappings);
		if (flags & PVF_WRITE)
			md->krw_mappings++;
		else
			md->kro_mappings++;
	} else {
		if (flags & PVF_WRITE)
			md->urw_mappings++;
		else
			md->uro_mappings++;
	}

#ifdef PMAP_CACHE_VIPT
	/*
	 * If this is an exec mapping and its the first exec mapping
	 * for this page, make sure to sync the I-cache.
	 */
	if (PV_IS_EXEC_P(flags)) {
		if (!PV_IS_EXEC_P(md->pvh_attrs)) {
			pmap_syncicache_page(md, pa);
			PMAPCOUNT(exec_synced_map);
		}
		PMAPCOUNT(exec_mappings);
	}
#endif

	PMAPCOUNT(mappings);
	simple_unlock(&md->pvh_slock);	/* unlock, done! */

	if (pv->pv_flags & PVF_WIRED)
		++pm->pm_stats.wired_count;
}

/*
 *
 * pmap_find_pv: Find a pv entry
 *
 * => caller should hold lock on vm_page
 */
static inline struct pv_entry *
pmap_find_pv(struct vm_page_md *md, pmap_t pm, vaddr_t va)
{
	struct pv_entry *pv;

	SLIST_FOREACH(pv, &md->pvh_list, pv_link) {
		if (pm == pv->pv_pmap && va == pv->pv_va)
			break;
	}

	return (pv);
}

/*
 * pmap_remove_pv: try to remove a mapping from a pv_list
 *
 * => caller should hold proper lock on pmap_main_lock
 * => pmap should be locked
 * => caller should hold lock on vm_page [so that attrs can be adjusted]
 * => caller should adjust ptp's wire_count and free PTP if needed
 * => caller should NOT adjust pmap's wire_count
 * => we return the removed pv
 */
static struct pv_entry *
pmap_remove_pv(struct vm_page_md *md, paddr_t pa, pmap_t pm, vaddr_t va)
{
	struct pv_entry *pv, **prevptr;

	NPDEBUG(PDB_PVDUMP,
	    printf("pmap_remove_pv: pm %p, md %p, va 0x%08lx\n", pm, md, va));

	prevptr = &SLIST_FIRST(&md->pvh_list); /* prev pv_entry ptr */
	pv = *prevptr;

	while (pv) {
		if (pv->pv_pmap == pm && pv->pv_va == va) {	/* match? */
			NPDEBUG(PDB_PVDUMP, printf("pmap_remove_pv: pm %p, md "
			    "%p, flags 0x%x\n", pm, md, pv->pv_flags));
			if (pv->pv_flags & PVF_WIRED) {
				--pm->pm_stats.wired_count;
			}
			*prevptr = SLIST_NEXT(pv, pv_link);	/* remove it! */
			if (pm == pmap_kernel()) {
				PMAPCOUNT(kernel_unmappings);
				if (pv->pv_flags & PVF_WRITE)
					md->krw_mappings--;
				else
					md->kro_mappings--;
			} else {
				if (pv->pv_flags & PVF_WRITE)
					md->urw_mappings--;
				else
					md->uro_mappings--;
			}

			PMAPCOUNT(unmappings);
#ifdef PMAP_CACHE_VIPT
			if (!(pv->pv_flags & PVF_WRITE))
				break;
			/*
			 * If this page has had an exec mapping, then if
			 * this was the last mapping, discard the contents,
			 * otherwise sync the i-cache for this page.
			 */
			if (PV_IS_EXEC_P(md->pvh_attrs)) {
				if (SLIST_EMPTY(&md->pvh_list)) {
					md->pvh_attrs &= ~PVF_EXEC;
					PMAPCOUNT(exec_discarded_unmap);
				} else {
					pmap_syncicache_page(md, pa);
					PMAPCOUNT(exec_synced_unmap);
				}
			}
#endif /* PMAP_CACHE_VIPT */
			break;
		}
		prevptr = &SLIST_NEXT(pv, pv_link);	/* previous pointer */
		pv = *prevptr;				/* advance */
	}

#ifdef PMAP_CACHE_VIPT
	/*
	 * If we no longer have a WRITEABLE KENTRY at the head of list,
	 * clear the KMOD attribute from the page.
	 */
	if (SLIST_FIRST(&md->pvh_list) == NULL
	    || (SLIST_FIRST(&md->pvh_list)->pv_flags & PVF_KWRITE) != PVF_KWRITE)
		md->pvh_attrs &= ~PVF_KMOD;

	/*
	 * If this was a writeable page and there are no more writeable
	 * mappings (ignoring KMPAGE), clear the WRITE flag and writeback
	 * the contents to memory.
	 */
	if (md->krw_mappings + md->urw_mappings == 0)
		md->pvh_attrs &= ~PVF_WRITE;
	KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
#endif /* PMAP_CACHE_VIPT */

	return(pv);				/* return removed pv */
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
static u_int
pmap_modify_pv(struct vm_page_md *md, paddr_t pa, pmap_t pm, vaddr_t va,
    u_int clr_mask, u_int set_mask)
{
	struct pv_entry *npv;
	u_int flags, oflags;

	KASSERT((clr_mask & PVF_KENTRY) == 0);
	KASSERT((set_mask & PVF_KENTRY) == 0);

	if ((npv = pmap_find_pv(md, pm, va)) == NULL)
		return (0);

	NPDEBUG(PDB_PVDUMP,
	    printf("pmap_modify_pv: pm %p, md %p, clr 0x%x, set 0x%x, flags 0x%x\n", pm, md, clr_mask, set_mask, npv->pv_flags));

	/*
	 * There is at least one VA mapping this page.
	 */

	if (clr_mask & (PVF_REF | PVF_MOD)) {
		md->pvh_attrs |= set_mask & (PVF_REF | PVF_MOD);
#ifdef PMAP_CACHE_VIPT
		if ((md->pvh_attrs & (PVF_DMOD|PVF_NC)) != PVF_NC)
			md->pvh_attrs |= PVF_DIRTY;
		KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
#endif
	}

	oflags = npv->pv_flags;
	npv->pv_flags = flags = (oflags & ~clr_mask) | set_mask;

	if ((flags ^ oflags) & PVF_WIRED) {
		if (flags & PVF_WIRED)
			++pm->pm_stats.wired_count;
		else
			--pm->pm_stats.wired_count;
	}

	if ((flags ^ oflags) & PVF_WRITE) {
		if (pm == pmap_kernel()) {
			if (flags & PVF_WRITE) {
				md->krw_mappings++;
				md->kro_mappings--;
			} else {
				md->kro_mappings++;
				md->krw_mappings--;
			}
		} else {
			if (flags & PVF_WRITE) {
				md->urw_mappings++;
				md->uro_mappings--;
			} else {
				md->uro_mappings++;
				md->urw_mappings--;
			}
		}
	}
#ifdef PMAP_CACHE_VIPT
	if (md->urw_mappings + md->krw_mappings == 0)
		md->pvh_attrs &= ~PVF_WRITE;
	/*
	 * We have two cases here: the first is from enter_pv (new exec
	 * page), the second is a combined pmap_remove_pv/pmap_enter_pv.
	 * Since in latter, pmap_enter_pv won't do anything, we just have
	 * to do what pmap_remove_pv would do.
	 */
	if ((PV_IS_EXEC_P(flags) && !PV_IS_EXEC_P(md->pvh_attrs))
	    || (PV_IS_EXEC_P(md->pvh_attrs)
		|| (!(flags & PVF_WRITE) && (oflags & PVF_WRITE)))) {
		pmap_syncicache_page(md, pa);
		PMAPCOUNT(exec_synced_remap);
	}
	KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
#endif

	PMAPCOUNT(remappings);

	return (oflags);
}

/*
 * Allocate an L1 translation table for the specified pmap.
 * This is called at pmap creation time.
 */
static void
pmap_alloc_l1(pmap_t pm)
{
	struct l1_ttable *l1;
	u_int8_t domain;

	/*
	 * Remove the L1 at the head of the LRU list
	 */
	simple_lock(&l1_lru_lock);
	l1 = TAILQ_FIRST(&l1_lru_list);
	KDASSERT(l1 != NULL);
	TAILQ_REMOVE(&l1_lru_list, l1, l1_lru);

	/*
	 * Pick the first available domain number, and update
	 * the link to the next number.
	 */
	domain = l1->l1_domain_first;
	l1->l1_domain_first = l1->l1_domain_free[domain];

	/*
	 * If there are still free domain numbers in this L1,
	 * put it back on the TAIL of the LRU list.
	 */
	if (++l1->l1_domain_use_count < PMAP_DOMAINS)
		TAILQ_INSERT_TAIL(&l1_lru_list, l1, l1_lru);

	simple_unlock(&l1_lru_lock);

	/*
	 * Fix up the relevant bits in the pmap structure
	 */
	pm->pm_l1 = l1;
	pm->pm_domain = domain;
}

/*
 * Free an L1 translation table.
 * This is called at pmap destruction time.
 */
static void
pmap_free_l1(pmap_t pm)
{
	struct l1_ttable *l1 = pm->pm_l1;

	simple_lock(&l1_lru_lock);

	/*
	 * If this L1 is currently on the LRU list, remove it.
	 */
	if (l1->l1_domain_use_count < PMAP_DOMAINS)
		TAILQ_REMOVE(&l1_lru_list, l1, l1_lru);

	/*
	 * Free up the domain number which was allocated to the pmap
	 */
	l1->l1_domain_free[pm->pm_domain] = l1->l1_domain_first;
	l1->l1_domain_first = pm->pm_domain;
	l1->l1_domain_use_count--;

	/*
	 * The L1 now must have at least 1 free domain, so add
	 * it back to the LRU list. If the use count is zero,
	 * put it at the head of the list, otherwise it goes
	 * to the tail.
	 */
	if (l1->l1_domain_use_count == 0)
		TAILQ_INSERT_HEAD(&l1_lru_list, l1, l1_lru);
	else
		TAILQ_INSERT_TAIL(&l1_lru_list, l1, l1_lru);

	simple_unlock(&l1_lru_lock);
}

static inline void
pmap_use_l1(pmap_t pm)
{
	struct l1_ttable *l1;

	/*
	 * Do nothing if we're in interrupt context.
	 * Access to an L1 by the kernel pmap must not affect
	 * the LRU list.
	 */
	if (cpu_intr_p() || pm == pmap_kernel())
		return;

	l1 = pm->pm_l1;

	/*
	 * If the L1 is not currently on the LRU list, just return
	 */
	if (l1->l1_domain_use_count == PMAP_DOMAINS)
		return;

	simple_lock(&l1_lru_lock);

	/*
	 * Check the use count again, now that we've acquired the lock
	 */
	if (l1->l1_domain_use_count == PMAP_DOMAINS) {
		simple_unlock(&l1_lru_lock);
		return;
	}

	/*
	 * Move the L1 to the back of the LRU list
	 */
	TAILQ_REMOVE(&l1_lru_list, l1, l1_lru);
	TAILQ_INSERT_TAIL(&l1_lru_list, l1, l1_lru);

	simple_unlock(&l1_lru_lock);
}

/*
 * void pmap_free_l2_ptp(pt_entry_t *, paddr_t *)
 *
 * Free an L2 descriptor table.
 */
static inline void
#ifndef PMAP_INCLUDE_PTE_SYNC
pmap_free_l2_ptp(pt_entry_t *l2, paddr_t pa)
#else
pmap_free_l2_ptp(bool need_sync, pt_entry_t *l2, paddr_t pa)
#endif
{
#ifdef PMAP_INCLUDE_PTE_SYNC
#ifdef PMAP_CACHE_VIVT
	/*
	 * Note: With a write-back cache, we may need to sync this
	 * L2 table before re-using it.
	 * This is because it may have belonged to a non-current
	 * pmap, in which case the cache syncs would have been
	 * skipped for the pages that were being unmapped. If the
	 * L2 table were then to be immediately re-allocated to
	 * the *current* pmap, it may well contain stale mappings
	 * which have not yet been cleared by a cache write-back
	 * and so would still be visible to the mmu.
	 */
	if (need_sync)
		PTE_SYNC_RANGE(l2, L2_TABLE_SIZE_REAL / sizeof(pt_entry_t));
#endif /* PMAP_CACHE_VIVT */
#endif /* PMAP_INCLUDE_PTE_SYNC */
	pool_cache_put_paddr(&pmap_l2ptp_cache, (void *)l2, pa);
}

/*
 * Returns a pointer to the L2 bucket associated with the specified pmap
 * and VA, or NULL if no L2 bucket exists for the address.
 */
static inline struct l2_bucket *
pmap_get_l2_bucket(pmap_t pm, vaddr_t va)
{
	struct l2_dtable *l2;
	struct l2_bucket *l2b;
	u_short l1idx;

	l1idx = L1_IDX(va);

	if ((l2 = pm->pm_l2[L2_IDX(l1idx)]) == NULL ||
	    (l2b = &l2->l2_bucket[L2_BUCKET(l1idx)])->l2b_kva == NULL)
		return (NULL);

	return (l2b);
}

/*
 * Returns a pointer to the L2 bucket associated with the specified pmap
 * and VA.
 *
 * If no L2 bucket exists, perform the necessary allocations to put an L2
 * bucket/page table in place.
 *
 * Note that if a new L2 bucket/page was allocated, the caller *must*
 * increment the bucket occupancy counter appropriately *before* 
 * releasing the pmap's lock to ensure no other thread or cpu deallocates
 * the bucket/page in the meantime.
 */
static struct l2_bucket *
pmap_alloc_l2_bucket(pmap_t pm, vaddr_t va)
{
	struct l2_dtable *l2;
	struct l2_bucket *l2b;
	u_short l1idx;

	l1idx = L1_IDX(va);

	if ((l2 = pm->pm_l2[L2_IDX(l1idx)]) == NULL) {
		/*
		 * No mapping at this address, as there is
		 * no entry in the L1 table.
		 * Need to allocate a new l2_dtable.
		 */
		if ((l2 = pmap_alloc_l2_dtable()) == NULL)
			return (NULL);

		/*
		 * Link it into the parent pmap
		 */
		pm->pm_l2[L2_IDX(l1idx)] = l2;
	}

	l2b = &l2->l2_bucket[L2_BUCKET(l1idx)];

	/*
	 * Fetch pointer to the L2 page table associated with the address.
	 */
	if (l2b->l2b_kva == NULL) {
		pt_entry_t *ptep;

		/*
		 * No L2 page table has been allocated. Chances are, this
		 * is because we just allocated the l2_dtable, above.
		 */
		if ((ptep = pmap_alloc_l2_ptp(&l2b->l2b_phys)) == NULL) {
			/*
			 * Oops, no more L2 page tables available at this
			 * time. We may need to deallocate the l2_dtable
			 * if we allocated a new one above.
			 */
			if (l2->l2_occupancy == 0) {
				pm->pm_l2[L2_IDX(l1idx)] = NULL;
				pmap_free_l2_dtable(l2);
			}
			return (NULL);
		}

		l2->l2_occupancy++;
		l2b->l2b_kva = ptep;
		l2b->l2b_l1idx = l1idx;
	}

	return (l2b);
}

/*
 * One or more mappings in the specified L2 descriptor table have just been
 * invalidated.
 *
 * Garbage collect the metadata and descriptor table itself if necessary.
 *
 * The pmap lock must be acquired when this is called (not necessary
 * for the kernel pmap).
 */
static void
pmap_free_l2_bucket(pmap_t pm, struct l2_bucket *l2b, u_int count)
{
	struct l2_dtable *l2;
	pd_entry_t *pl1pd, l1pd;
	pt_entry_t *ptep;
	u_short l1idx;

	KDASSERT(count <= l2b->l2b_occupancy);

	/*
	 * Update the bucket's reference count according to how many
	 * PTEs the caller has just invalidated.
	 */
	l2b->l2b_occupancy -= count;

	/*
	 * Note:
	 *
	 * Level 2 page tables allocated to the kernel pmap are never freed
	 * as that would require checking all Level 1 page tables and
	 * removing any references to the Level 2 page table. See also the
	 * comment elsewhere about never freeing bootstrap L2 descriptors.
	 *
	 * We make do with just invalidating the mapping in the L2 table.
	 *
	 * This isn't really a big deal in practice and, in fact, leads
	 * to a performance win over time as we don't need to continually
	 * alloc/free.
	 */
	if (l2b->l2b_occupancy > 0 || pm == pmap_kernel())
		return;

	/*
	 * There are no more valid mappings in this level 2 page table.
	 * Go ahead and NULL-out the pointer in the bucket, then
	 * free the page table.
	 */
	l1idx = l2b->l2b_l1idx;
	ptep = l2b->l2b_kva;
	l2b->l2b_kva = NULL;

	pl1pd = &pm->pm_l1->l1_kva[l1idx];

	/*
	 * If the L1 slot matches the pmap's domain
	 * number, then invalidate it.
	 */
	l1pd = *pl1pd & (L1_TYPE_MASK | L1_C_DOM_MASK);
	if (l1pd == (L1_C_DOM(pm->pm_domain) | L1_TYPE_C)) {
		*pl1pd = 0;
		PTE_SYNC(pl1pd);
	}

	/*
	 * Release the L2 descriptor table back to the pool cache.
	 */
#ifndef PMAP_INCLUDE_PTE_SYNC
	pmap_free_l2_ptp(ptep, l2b->l2b_phys);
#else
	pmap_free_l2_ptp(!pmap_is_cached(pm), ptep, l2b->l2b_phys);
#endif

	/*
	 * Update the reference count in the associated l2_dtable
	 */
	l2 = pm->pm_l2[L2_IDX(l1idx)];
	if (--l2->l2_occupancy > 0)
		return;

	/*
	 * There are no more valid mappings in any of the Level 1
	 * slots managed by this l2_dtable. Go ahead and NULL-out
	 * the pointer in the parent pmap and free the l2_dtable.
	 */
	pm->pm_l2[L2_IDX(l1idx)] = NULL;
	pmap_free_l2_dtable(l2);
}

/*
 * Pool cache constructors for L2 descriptor tables, metadata and pmap
 * structures.
 */
static int
pmap_l2ptp_ctor(void *arg, void *v, int flags)
{
#ifndef PMAP_INCLUDE_PTE_SYNC
	struct l2_bucket *l2b;
	pt_entry_t *ptep, pte;
	vaddr_t va = (vaddr_t)v & ~PGOFSET;

	/*
	 * The mappings for these page tables were initially made using
	 * pmap_kenter_pa() by the pool subsystem. Therefore, the cache-
	 * mode will not be right for page table mappings. To avoid
	 * polluting the pmap_kenter_pa() code with a special case for
	 * page tables, we simply fix up the cache-mode here if it's not
	 * correct.
	 */
	l2b = pmap_get_l2_bucket(pmap_kernel(), va);
	KDASSERT(l2b != NULL);
	ptep = &l2b->l2b_kva[l2pte_index(va)];
	pte = *ptep;

	if ((pte & L2_S_CACHE_MASK) != pte_l2_s_cache_mode_pt) {
		/*
		 * Page tables must have the cache-mode set to Write-Thru.
		 */
		*ptep = (pte & ~L2_S_CACHE_MASK) | pte_l2_s_cache_mode_pt;
		PTE_SYNC(ptep);
		cpu_tlb_flushD_SE(va);
		cpu_cpwait();
	}
#endif

	memset(v, 0, L2_TABLE_SIZE_REAL);
	PTE_SYNC_RANGE(v, L2_TABLE_SIZE_REAL / sizeof(pt_entry_t));
	return (0);
}

static int
pmap_l2dtable_ctor(void *arg, void *v, int flags)
{

	memset(v, 0, sizeof(struct l2_dtable));
	return (0);
}

static int
pmap_pmap_ctor(void *arg, void *v, int flags)
{

	memset(v, 0, sizeof(struct pmap));
	return (0);
}

static void
pmap_pinit(pmap_t pm)
{
	struct l2_bucket *l2b;

	if (vector_page < KERNEL_BASE) {
		/*
		 * Map the vector page.
		 */
		pmap_enter(pm, vector_page, systempage.pv_pa,
		    VM_PROT_READ, VM_PROT_READ | PMAP_WIRED);
		pmap_update(pm);

		pm->pm_pl1vec = &pm->pm_l1->l1_kva[L1_IDX(vector_page)];
		l2b = pmap_get_l2_bucket(pm, vector_page);
		KDASSERT(l2b != NULL);
		pm->pm_l1vec = l2b->l2b_phys | L1_C_PROTO |
		    L1_C_DOM(pm->pm_domain);
	} else
		pm->pm_pl1vec = NULL;
}

#ifdef PMAP_CACHE_VIVT
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
 * 
 * KC = kernel mapping is cacheable
 * UC = user mapping is cacheable
 *
 *               KW=0,KR=0  KW=0,KR>0  KW=1,KR=0  KW>1,KR>=0
 *             +---------------------------------------------
 * UW=0,UR=0   | ---        KC=1       KC=1       KC=0
 * UW=0,UR>0   | UC=1       KC=1,UC=1  KC=0,UC=0  KC=0,UC=0
 * UW=1,UR=0   | UC=1       KC=0,UC=0  KC=0,UC=0  KC=0,UC=0
 * UW>1,UR>=0  | UC=0       KC=0,UC=0  KC=0,UC=0  KC=0,UC=0
 */

static const int pmap_vac_flags[4][4] = {
	{-1,		0,		0,		PVF_KNC},
	{0,		0,		PVF_NC,		PVF_NC},
	{0,		PVF_NC,		PVF_NC,		PVF_NC},
	{PVF_UNC,	PVF_NC,		PVF_NC,		PVF_NC}
};

static inline int
pmap_get_vac_flags(const struct vm_page_md *md)
{
	int kidx, uidx;

	kidx = 0;
	if (md->kro_mappings || md->krw_mappings > 1)
		kidx |= 1;
	if (md->krw_mappings)
		kidx |= 2;

	uidx = 0;
	if (md->uro_mappings || md->urw_mappings > 1)
		uidx |= 1;
	if (md->urw_mappings)
		uidx |= 2;

	return (pmap_vac_flags[uidx][kidx]);
}

static inline void
pmap_vac_me_harder(struct vm_page_md *md, paddr_t pa, pmap_t pm, vaddr_t va)
{
	int nattr;

	nattr = pmap_get_vac_flags(md);

	if (nattr < 0) {
		md->pvh_attrs &= ~PVF_NC;
		return;
	}

	if (nattr == 0 && (md->pvh_attrs & PVF_NC) == 0)
		return;

	if (pm == pmap_kernel())
		pmap_vac_me_kpmap(md, pa, pm, va);
	else
		pmap_vac_me_user(md, pa, pm, va);

	md->pvh_attrs = (md->pvh_attrs & ~PVF_NC) | nattr;
}

static void
pmap_vac_me_kpmap(struct vm_page_md *md, paddr_t pa, pmap_t pm, vaddr_t va)
{
	u_int u_cacheable, u_entries;
	struct pv_entry *pv;
	pmap_t last_pmap = pm;

	/* 
	 * Pass one, see if there are both kernel and user pmaps for
	 * this page.  Calculate whether there are user-writable or
	 * kernel-writable pages.
	 */
	u_cacheable = 0;
	SLIST_FOREACH(pv, &md->pvh_list, pv_link) {
		if (pv->pv_pmap != pm && (pv->pv_flags & PVF_NC) == 0)
			u_cacheable++;
	}

	u_entries = md->urw_mappings + md->uro_mappings;

	/* 
	 * We know we have just been updating a kernel entry, so if
	 * all user pages are already cacheable, then there is nothing
	 * further to do.
	 */
	if (md->k_mappings == 0 && u_cacheable == u_entries)
		return;

	if (u_entries) {
		/* 
		 * Scan over the list again, for each entry, if it
		 * might not be set correctly, call pmap_vac_me_user
		 * to recalculate the settings.
		 */
		SLIST_FOREACH(pv, &md->pvh_list, pv_link) {
			/* 
			 * We know kernel mappings will get set
			 * correctly in other calls.  We also know
			 * that if the pmap is the same as last_pmap
			 * then we've just handled this entry.
			 */
			if (pv->pv_pmap == pm || pv->pv_pmap == last_pmap)
				continue;

			/* 
			 * If there are kernel entries and this page
			 * is writable but non-cacheable, then we can
			 * skip this entry also.  
			 */
			if (md->k_mappings &&
			    (pv->pv_flags & (PVF_NC | PVF_WRITE)) ==
			    (PVF_NC | PVF_WRITE))
				continue;

			/* 
			 * Similarly if there are no kernel-writable 
			 * entries and the page is already 
			 * read-only/cacheable.
			 */
			if (md->krw_mappings == 0 &&
			    (pv->pv_flags & (PVF_NC | PVF_WRITE)) == 0)
				continue;

			/* 
			 * For some of the remaining cases, we know
			 * that we must recalculate, but for others we
			 * can't tell if they are correct or not, so
			 * we recalculate anyway.
			 */
			pmap_vac_me_user(md, pa, (last_pmap = pv->pv_pmap), 0);
		}

		if (md->k_mappings == 0)
			return;
	}

	pmap_vac_me_user(md, pa, pm, va);
}

static void
pmap_vac_me_user(struct vm_page_md *md, paddr_t pa, pmap_t pm, vaddr_t va)
{
	pmap_t kpmap = pmap_kernel();
	struct pv_entry *pv, *npv = NULL;
	struct l2_bucket *l2b;
	pt_entry_t *ptep, pte;
	u_int entries = 0;
	u_int writable = 0;
	u_int cacheable_entries = 0;
	u_int kern_cacheable = 0;
	u_int other_writable = 0;

	/*
	 * Count mappings and writable mappings in this pmap.
	 * Include kernel mappings as part of our own.
	 * Keep a pointer to the first one.
	 */
	npv = NULL;
	SLIST_FOREACH(pv, &md->pvh_list, pv_link) {
		/* Count mappings in the same pmap */
		if (pm == pv->pv_pmap || kpmap == pv->pv_pmap) {
			if (entries++ == 0)
				npv = pv;

			/* Cacheable mappings */
			if ((pv->pv_flags & PVF_NC) == 0) {
				cacheable_entries++;
				if (kpmap == pv->pv_pmap)
					kern_cacheable++;
			}

			/* Writable mappings */
			if (pv->pv_flags & PVF_WRITE)
				++writable;
		} else
		if (pv->pv_flags & PVF_WRITE)
			other_writable = 1;
	}

	/*
	 * Enable or disable caching as necessary.
	 * Note: the first entry might be part of the kernel pmap,
	 * so we can't assume this is indicative of the state of the
	 * other (maybe non-kpmap) entries.
	 */
	if ((entries > 1 && writable) ||
	    (entries > 0 && pm == kpmap && other_writable)) {
		if (cacheable_entries == 0)
			return;

		for (pv = npv; pv; pv = SLIST_NEXT(pv, pv_link)) {
			if ((pm != pv->pv_pmap && kpmap != pv->pv_pmap) ||
			    (pv->pv_flags & PVF_NC))
				continue;

			pv->pv_flags |= PVF_NC;

			l2b = pmap_get_l2_bucket(pv->pv_pmap, pv->pv_va);
			KDASSERT(l2b != NULL);
			ptep = &l2b->l2b_kva[l2pte_index(pv->pv_va)];
			pte = *ptep & ~L2_S_CACHE_MASK;

			if ((va != pv->pv_va || pm != pv->pv_pmap) &&
			    l2pte_valid(pte)) {
				if (PV_BEEN_EXECD(pv->pv_flags)) {
#ifdef PMAP_CACHE_VIVT
					pmap_idcache_wbinv_range(pv->pv_pmap,
					    pv->pv_va, PAGE_SIZE);
#endif
					pmap_tlb_flushID_SE(pv->pv_pmap,
					    pv->pv_va);
				} else
				if (PV_BEEN_REFD(pv->pv_flags)) {
#ifdef PMAP_CACHE_VIVT
					pmap_dcache_wb_range(pv->pv_pmap,
					    pv->pv_va, PAGE_SIZE, true,
					    (pv->pv_flags & PVF_WRITE) == 0);
#endif
					pmap_tlb_flushD_SE(pv->pv_pmap,
					    pv->pv_va);
				}
			}

			*ptep = pte;
			PTE_SYNC_CURRENT(pv->pv_pmap, ptep);
		}
		cpu_cpwait();
	} else
	if (entries > cacheable_entries) {
		/*
		 * Turn cacheing back on for some pages.  If it is a kernel
		 * page, only do so if there are no other writable pages.
		 */
		for (pv = npv; pv; pv = SLIST_NEXT(pv, pv_link)) {
			if (!(pv->pv_flags & PVF_NC) || (pm != pv->pv_pmap &&
			    (kpmap != pv->pv_pmap || other_writable)))
				continue;

			pv->pv_flags &= ~PVF_NC;

			l2b = pmap_get_l2_bucket(pv->pv_pmap, pv->pv_va);
			KDASSERT(l2b != NULL);
			ptep = &l2b->l2b_kva[l2pte_index(pv->pv_va)];
			pte = (*ptep & ~L2_S_CACHE_MASK) | pte_l2_s_cache_mode;

			if (l2pte_valid(pte)) {
				if (PV_BEEN_EXECD(pv->pv_flags)) {
					pmap_tlb_flushID_SE(pv->pv_pmap,
					    pv->pv_va);
				} else
				if (PV_BEEN_REFD(pv->pv_flags)) {
					pmap_tlb_flushD_SE(pv->pv_pmap,
					    pv->pv_va);
				}
			}

			*ptep = pte;
			PTE_SYNC_CURRENT(pv->pv_pmap, ptep);
		}
	}
}
#endif

#ifdef PMAP_CACHE_VIPT
static void
pmap_vac_me_harder(struct vm_page_md *md, paddr_t pa, pmap_t pm, vaddr_t va)
{
	struct pv_entry *pv;
	vaddr_t tst_mask;
	bool bad_alias;
	struct l2_bucket *l2b;
	pt_entry_t *ptep, pte, opte;
	const u_int
	    rw_mappings = md->urw_mappings + md->krw_mappings,
	    ro_mappings = md->uro_mappings + md->kro_mappings;

	/* do we need to do anything? */
	if (arm_cache_prefer_mask == 0)
		return;

	NPDEBUG(PDB_VAC, printf("pmap_vac_me_harder: md=%p, pmap=%p va=%08lx\n",
	    md, pm, va));

	KASSERT(!va || pm);
	KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));

	/* Already a conflict? */
	if (__predict_false(md->pvh_attrs & PVF_NC)) {
		/* just an add, things are already non-cached */
		KASSERT(!(md->pvh_attrs & PVF_DIRTY));
		KASSERT(!(md->pvh_attrs & PVF_MULTCLR));
		bad_alias = false;
		if (va) {
			PMAPCOUNT(vac_color_none);
			bad_alias = true;
			KASSERT((rw_mappings == 0) == !(md->pvh_attrs & PVF_WRITE));
			goto fixup;
		}
		pv = SLIST_FIRST(&md->pvh_list);
		/* the list can't be empty because it would be cachable */
		if (md->pvh_attrs & PVF_KMPAGE) {
			tst_mask = md->pvh_attrs;
		} else {
			KASSERT(pv);
			tst_mask = pv->pv_va;
			pv = SLIST_NEXT(pv, pv_link);
		}
		/*
		 * Only check for a bad alias if we have writable mappings.
		 */
		tst_mask &= arm_cache_prefer_mask;
		if (rw_mappings > 0 && arm_cache_prefer_mask) {
			for (; pv && !bad_alias; pv = SLIST_NEXT(pv, pv_link)) {
				/* if there's a bad alias, stop checking. */
				if (tst_mask != (pv->pv_va & arm_cache_prefer_mask))
					bad_alias = true;
			}
			md->pvh_attrs |= PVF_WRITE;
			if (!bad_alias)
				md->pvh_attrs |= PVF_DIRTY;
		} else {
			/*
			 * We have only read-only mappings.  Let's see if there
			 * are multiple colors in use or if we mapped a KMPAGE.
			 * If the latter, we have a bad alias.  If the former,
			 * we need to remember that.
			 */
			for (; pv; pv = SLIST_NEXT(pv, pv_link)) {
				if (tst_mask != (pv->pv_va & arm_cache_prefer_mask)) {
					if (md->pvh_attrs & PVF_KMPAGE)
						bad_alias = true;
					break;
				}
			}
			md->pvh_attrs &= ~PVF_WRITE;
			/*
			 * No KMPAGE and we exited early, so we must have 
			 * multiple color mappings.
			 */
			if (!bad_alias && pv != NULL)
				md->pvh_attrs |= PVF_MULTCLR;
		}

		/* If no conflicting colors, set everything back to cached */
		if (!bad_alias) {
#ifdef DEBUG
			if ((md->pvh_attrs & PVF_WRITE)
			    || ro_mappings < 2) {
				SLIST_FOREACH(pv, &md->pvh_list, pv_link)
					KDASSERT(((tst_mask ^ pv->pv_va) & arm_cache_prefer_mask) == 0);
			}
#endif
			md->pvh_attrs &= (PAGE_SIZE - 1) & ~PVF_NC;
			md->pvh_attrs |= tst_mask | PVF_COLORED;
			/*
			 * Restore DIRTY bit if page is modified
			 */
			if (md->pvh_attrs & PVF_DMOD)
				md->pvh_attrs |= PVF_DIRTY;
			PMAPCOUNT(vac_color_restore);
		} else {
			KASSERT(SLIST_FIRST(&md->pvh_list) != NULL);
			KASSERT(SLIST_NEXT(SLIST_FIRST(&md->pvh_list), pv_link) != NULL);
		}
		KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
		KASSERT((rw_mappings == 0) == !(md->pvh_attrs & PVF_WRITE));
	} else if (!va) {
		KASSERT(arm_cache_prefer_mask == 0 || pmap_is_page_colored_p(md));
		KASSERT(!(md->pvh_attrs & PVF_WRITE)
		    || (md->pvh_attrs & PVF_DIRTY));
		if (rw_mappings == 0) {
			md->pvh_attrs &= ~PVF_WRITE;
			if (ro_mappings == 1
			    && (md->pvh_attrs & PVF_MULTCLR)) {
				/*
				 * If this is the last readonly mapping
				 * but it doesn't match the current color
				 * for the page, change the current color
				 * to match this last readonly mapping.
				 */
				pv = SLIST_FIRST(&md->pvh_list);
				tst_mask = (md->pvh_attrs ^ pv->pv_va)
				    & arm_cache_prefer_mask;
				if (tst_mask) {
					md->pvh_attrs ^= tst_mask;
					PMAPCOUNT(vac_color_change);
				}
			}
		}
		KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
		KASSERT((rw_mappings == 0) == !(md->pvh_attrs & PVF_WRITE));
		return;
	} else if (!pmap_is_page_colored_p(md)) {
		/* not colored so we just use its color */
		KASSERT(md->pvh_attrs & (PVF_WRITE|PVF_DIRTY));
		KASSERT(!(md->pvh_attrs & PVF_MULTCLR));
		PMAPCOUNT(vac_color_new);
		md->pvh_attrs &= PAGE_SIZE - 1;
		md->pvh_attrs |= PVF_COLORED
		    | (va & arm_cache_prefer_mask)
		    | (rw_mappings > 0 ? PVF_WRITE : 0);
		KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
		KASSERT((rw_mappings == 0) == !(md->pvh_attrs & PVF_WRITE));
		return;
	} else if (((md->pvh_attrs ^ va) & arm_cache_prefer_mask) == 0) {
		bad_alias = false;
		if (rw_mappings > 0) {
			/*
			 * We now have writeable mappings and if we have
			 * readonly mappings in more than once color, we have
			 * an aliasing problem.  Regardless mark the page as
			 * writeable.
			 */
			if (md->pvh_attrs & PVF_MULTCLR) {
				if (ro_mappings < 2) {
					/*
					 * If we only have less than two
					 * read-only mappings, just flush the
					 * non-primary colors from the cache.
					 */
					pmap_flush_page(md, pa,
					    PMAP_FLUSH_SECONDARY);
				} else {
					bad_alias = true;
				}
			}
			md->pvh_attrs |= PVF_WRITE;
		}
		/* If no conflicting colors, set everything back to cached */
		if (!bad_alias) {
#ifdef DEBUG
			if (rw_mappings > 0
			    || (md->pvh_attrs & PMAP_KMPAGE)) {
				tst_mask = md->pvh_attrs & arm_cache_prefer_mask;
				SLIST_FOREACH(pv, &md->pvh_list, pv_link)
					KDASSERT(((tst_mask ^ pv->pv_va) & arm_cache_prefer_mask) == 0);
			}
#endif
			if (SLIST_EMPTY(&md->pvh_list))
				PMAPCOUNT(vac_color_reuse);
			else
				PMAPCOUNT(vac_color_ok);

			/* matching color, just return */
			KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
			KASSERT((rw_mappings == 0) == !(md->pvh_attrs & PVF_WRITE));
			return;
		}
		KASSERT(SLIST_FIRST(&md->pvh_list) != NULL);
		KASSERT(SLIST_NEXT(SLIST_FIRST(&md->pvh_list), pv_link) != NULL);

		/* color conflict.  evict from cache. */

		pmap_flush_page(md, pa, PMAP_FLUSH_PRIMARY);
		md->pvh_attrs &= ~PVF_COLORED;
		md->pvh_attrs |= PVF_NC;
		KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
		KASSERT(!(md->pvh_attrs & PVF_MULTCLR));
		PMAPCOUNT(vac_color_erase);
	} else if (rw_mappings == 0
		   && (md->pvh_attrs & PVF_KMPAGE) == 0) {
		KASSERT((md->pvh_attrs & PVF_WRITE) == 0);

		/*
		 * If the page has dirty cache lines, clean it.
		 */
		if (md->pvh_attrs & PVF_DIRTY)
			pmap_flush_page(md, pa, PMAP_CLEAN_PRIMARY);

		/*
		 * If this is the first remapping (we know that there are no
		 * writeable mappings), then this is a simple color change.
		 * Otherwise this is a seconary r/o mapping, which means
		 * we don't have to do anything.
		 */
		if (ro_mappings == 1) {
			KASSERT(((md->pvh_attrs ^ va) & arm_cache_prefer_mask) != 0);
			md->pvh_attrs &= PAGE_SIZE - 1;
			md->pvh_attrs |= (va & arm_cache_prefer_mask);
			PMAPCOUNT(vac_color_change);
		} else {
			PMAPCOUNT(vac_color_blind);
		}
		md->pvh_attrs |= PVF_MULTCLR;
		KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
		KASSERT((rw_mappings == 0) == !(md->pvh_attrs & PVF_WRITE));
		return;
	} else {
		if (rw_mappings > 0)
			md->pvh_attrs |= PVF_WRITE;

		/* color conflict.  evict from cache. */
		pmap_flush_page(md, pa, PMAP_FLUSH_PRIMARY);

		/* the list can't be empty because this was a enter/modify */
		pv = SLIST_FIRST(&md->pvh_list);
		if ((md->pvh_attrs & PVF_KMPAGE) == 0) {
			KASSERT(pv);
			/*
			 * If there's only one mapped page, change color to the
			 * page's new color and return.  Restore the DIRTY bit
			 * that was erased by pmap_flush_page.
			 */
			if (SLIST_NEXT(pv, pv_link) == NULL) {
				md->pvh_attrs &= PAGE_SIZE - 1;
				md->pvh_attrs |= (va & arm_cache_prefer_mask);
				if (md->pvh_attrs & PVF_DMOD)
					md->pvh_attrs |= PVF_DIRTY;
				PMAPCOUNT(vac_color_change);
				KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
				KASSERT((rw_mappings == 0) == !(md->pvh_attrs & PVF_WRITE));
				KASSERT(!(md->pvh_attrs & PVF_MULTCLR));
				return;
			}
		}
		bad_alias = true;
		md->pvh_attrs &= ~PVF_COLORED;
		md->pvh_attrs |= PVF_NC;
		PMAPCOUNT(vac_color_erase);
		KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
	}

  fixup:
	KASSERT((rw_mappings == 0) == !(md->pvh_attrs & PVF_WRITE));

	/*
	 * Turn cacheing on/off for all pages.
	 */
	SLIST_FOREACH(pv, &md->pvh_list, pv_link) {
		l2b = pmap_get_l2_bucket(pv->pv_pmap, pv->pv_va);
		KDASSERT(l2b != NULL);
		ptep = &l2b->l2b_kva[l2pte_index(pv->pv_va)];
		opte = *ptep;
		pte = opte & ~L2_S_CACHE_MASK;
		if (bad_alias) {
			pv->pv_flags |= PVF_NC;
		} else {
			pv->pv_flags &= ~PVF_NC;
			pte |= pte_l2_s_cache_mode;
		}

		if (opte == pte)	/* only update is there's a change */
			continue;

		if (l2pte_valid(pte)) {
			if (PV_BEEN_EXECD(pv->pv_flags)) {
				pmap_tlb_flushID_SE(pv->pv_pmap, pv->pv_va);
			} else if (PV_BEEN_REFD(pv->pv_flags)) {
				pmap_tlb_flushD_SE(pv->pv_pmap, pv->pv_va);
			}
		}

		*ptep = pte;
		PTE_SYNC_CURRENT(pv->pv_pmap, ptep);
	}
}
#endif	/* PMAP_CACHE_VIPT */


/*
 * Modify pte bits for all ptes corresponding to the given physical address.
 * We use `maskbits' rather than `clearbits' because we're always passing
 * constants and the latter would require an extra inversion at run-time.
 */
static void
pmap_clearbit(struct vm_page_md *md, paddr_t pa, u_int maskbits)
{
	struct l2_bucket *l2b;
	struct pv_entry *pv;
	pt_entry_t *ptep, npte, opte;
	pmap_t pm;
	vaddr_t va;
	u_int oflags;
#ifdef PMAP_CACHE_VIPT
	const bool want_syncicache = PV_IS_EXEC_P(md->pvh_attrs);
	bool need_syncicache = false;
	bool did_syncicache = false;
	bool need_vac_me_harder = false;
#endif

	NPDEBUG(PDB_BITS,
	    printf("pmap_clearbit: md %p mask 0x%x\n",
	    md, maskbits));

	PMAP_HEAD_TO_MAP_LOCK();
	simple_lock(&md->pvh_slock);

#ifdef PMAP_CACHE_VIPT
	/*
	 * If we might want to sync the I-cache and we've modified it,
	 * then we know we definitely need to sync or discard it.
	 */
	if (want_syncicache)
		need_syncicache = md->pvh_attrs & PVF_MOD;
#endif
	/*
	 * Clear saved attributes (modify, reference)
	 */
	md->pvh_attrs &= ~(maskbits & (PVF_MOD | PVF_REF));

	if (SLIST_EMPTY(&md->pvh_list)) {
#ifdef PMAP_CACHE_VIPT
		if (need_syncicache) {
			/*
			 * No one has it mapped, so just discard it.  The next
			 * exec remapping will cause it to be synced.
			 */
			md->pvh_attrs &= ~PVF_EXEC;
			PMAPCOUNT(exec_discarded_clearbit);
		}
#endif
		simple_unlock(&md->pvh_slock);
		PMAP_HEAD_TO_MAP_UNLOCK();
		return;
	}

	/*
	 * Loop over all current mappings setting/clearing as appropos
	 */
	SLIST_FOREACH(pv, &md->pvh_list, pv_link) {
		va = pv->pv_va;
		pm = pv->pv_pmap;
		oflags = pv->pv_flags;
		/*
		 * Kernel entries are unmanaged and as such not to be changed.
		 */
		if (oflags & PVF_KENTRY)
			continue;
		pv->pv_flags &= ~maskbits;

		pmap_acquire_pmap_lock(pm);

		l2b = pmap_get_l2_bucket(pm, va);
		KDASSERT(l2b != NULL);

		ptep = &l2b->l2b_kva[l2pte_index(va)];
		npte = opte = *ptep;

		NPDEBUG(PDB_BITS,
		    printf(
		    "pmap_clearbit: pv %p, pm %p, va 0x%08lx, flag 0x%x\n",
		    pv, pv->pv_pmap, pv->pv_va, oflags));

		if (maskbits & (PVF_WRITE|PVF_MOD)) {
#ifdef PMAP_CACHE_VIVT
			if ((pv->pv_flags & PVF_NC)) {
				/* 
				 * Entry is not cacheable:
				 *
				 * Don't turn caching on again if this is a 
				 * modified emulation. This would be
				 * inconsitent with the settings created by
				 * pmap_vac_me_harder(). Otherwise, it's safe
				 * to re-enable cacheing.
				 *
				 * There's no need to call pmap_vac_me_harder()
				 * here: all pages are losing their write
				 * permission.
				 */
				if (maskbits & PVF_WRITE) {
					npte |= pte_l2_s_cache_mode;
					pv->pv_flags &= ~PVF_NC;
				}
			} else
			if (l2pte_writable_p(opte)) {
				/* 
				 * Entry is writable/cacheable: check if pmap
				 * is current if it is flush it, otherwise it
				 * won't be in the cache
				 */
				if (PV_BEEN_EXECD(oflags))
					pmap_idcache_wbinv_range(pm, pv->pv_va,
					    PAGE_SIZE);
				else
				if (PV_BEEN_REFD(oflags))
					pmap_dcache_wb_range(pm, pv->pv_va,
					    PAGE_SIZE,
					    (maskbits & PVF_REF) != 0, false);
			}
#endif

			/* make the pte read only */
			npte = l2pte_set_readonly(npte);

			if (maskbits & oflags & PVF_WRITE) {
				/*
				 * Keep alias accounting up to date
				 */
				if (pv->pv_pmap == pmap_kernel()) {
					md->krw_mappings--;
					md->kro_mappings++;
				} else {
					md->urw_mappings--;
					md->uro_mappings++;
				}
#ifdef PMAP_CACHE_VIPT
				if (md->urw_mappings + md->krw_mappings == 0)
					md->pvh_attrs &= ~PVF_WRITE;
				if (want_syncicache)
					need_syncicache = true;
				need_vac_me_harder = true;
#endif
			}
		}

		if (maskbits & PVF_REF) {
			if ((pv->pv_flags & PVF_NC) == 0 &&
			    (maskbits & (PVF_WRITE|PVF_MOD)) == 0 &&
			    l2pte_valid(npte)) {
#ifdef PMAP_CACHE_VIVT
				/*
				 * Check npte here; we may have already
				 * done the wbinv above, and the validity
				 * of the PTE is the same for opte and
				 * npte.
				 */
				/* XXXJRT need idcache_inv_range */
				if (PV_BEEN_EXECD(oflags))
					pmap_idcache_wbinv_range(pm,
					    pv->pv_va, PAGE_SIZE);
				else
				if (PV_BEEN_REFD(oflags))
					pmap_dcache_wb_range(pm,
					    pv->pv_va, PAGE_SIZE,
					    true, true);
#endif
			}

			/*
			 * Make the PTE invalid so that we will take a
			 * page fault the next time the mapping is
			 * referenced.
			 */
			npte &= ~L2_TYPE_MASK;
			npte |= L2_TYPE_INV;
		}

		if (npte != opte) {
			*ptep = npte;
			PTE_SYNC(ptep);
			/* Flush the TLB entry if a current pmap. */
			if (PV_BEEN_EXECD(oflags))
				pmap_tlb_flushID_SE(pm, pv->pv_va);
			else
			if (PV_BEEN_REFD(oflags))
				pmap_tlb_flushD_SE(pm, pv->pv_va);
		}

		pmap_release_pmap_lock(pm);

		NPDEBUG(PDB_BITS,
		    printf("pmap_clearbit: pm %p va 0x%lx opte 0x%08x npte 0x%08x\n",
		    pm, va, opte, npte));
	}

#ifdef PMAP_CACHE_VIPT
	/*
	 * If we need to sync the I-cache and we haven't done it yet, do it.
	 */
	if (need_syncicache && !did_syncicache) {
		pmap_syncicache_page(md, pa);
		PMAPCOUNT(exec_synced_clearbit);
	}
	/*
	 * If we are changing this to read-only, we need to call vac_me_harder
	 * so we can change all the read-only pages to cacheable.  We pretend
	 * this as a page deletion.
	 */
	if (need_vac_me_harder) {
		if (md->pvh_attrs & PVF_NC)
			pmap_vac_me_harder(md, pa, NULL, 0);
	}
#endif

	simple_unlock(&md->pvh_slock);
	PMAP_HEAD_TO_MAP_UNLOCK();
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
#ifdef PMAP_CACHE_VIVT
static int
pmap_clean_page(struct pv_entry *pv, bool is_src)
{
	pmap_t pm_to_clean = NULL;
	struct pv_entry *npv;
	u_int cache_needs_cleaning = 0;
	u_int flags = 0;
	vaddr_t page_to_clean = 0;

	if (pv == NULL) {
		/* nothing mapped in so nothing to flush */
		return (0);
	}

	/*
	 * Since we flush the cache each time we change to a different
	 * user vmspace, we only need to flush the page if it is in the
	 * current pmap.
	 */

	for (npv = pv; npv; npv = SLIST_NEXT(npv, pv_link)) {
		if (pmap_is_current(npv->pv_pmap)) {
			flags |= npv->pv_flags;
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
			} else {
				page_to_clean = npv->pv_va;
				pm_to_clean = npv->pv_pmap;
			}
			cache_needs_cleaning = 1;
		}
	}

	if (page_to_clean) {
		if (PV_BEEN_EXECD(flags))
			pmap_idcache_wbinv_range(pm_to_clean, page_to_clean,
			    PAGE_SIZE);
		else
			pmap_dcache_wb_range(pm_to_clean, page_to_clean,
			    PAGE_SIZE, !is_src, (flags & PVF_WRITE) == 0);
	} else if (cache_needs_cleaning) {
		pmap_t const pm = curproc->p_vmspace->vm_map.pmap;

		if (PV_BEEN_EXECD(flags))
			pmap_idcache_wbinv_all(pm);
		else
			pmap_dcache_wbinv_all(pm);
		return (1);
	}
	return (0);
}
#endif

#ifdef PMAP_CACHE_VIPT
/*
 * Sync a page with the I-cache.  Since this is a VIPT, we must pick the
 * right cache alias to make sure we flush the right stuff.
 */
void
pmap_syncicache_page(struct vm_page_md *md, paddr_t pa)
{
	const vsize_t va_offset = md->pvh_attrs & arm_cache_prefer_mask;
	pt_entry_t * const ptep = &cdst_pte[va_offset >> PGSHIFT];

	NPDEBUG(PDB_EXEC, printf("pmap_syncicache_page: md=%p (attrs=%#x)\n",
	    md, md->pvh_attrs));
	/*
	 * No need to clean the page if it's non-cached.
	 */
	if (md->pvh_attrs & PVF_NC)
		return;
	KASSERT(arm_cache_prefer_mask == 0 || md->pvh_attrs & PVF_COLORED);

	pmap_tlb_flushID_SE(pmap_kernel(), cdstp + va_offset);
	/*
	 * Set up a PTE with the right coloring to flush existing cache lines.
	 */
	*ptep = L2_S_PROTO |
	    pa
	    | L2_S_PROT(PTE_KERNEL, VM_PROT_READ|VM_PROT_WRITE)
	    | pte_l2_s_cache_mode;
	PTE_SYNC(ptep);

	/*
	 * Flush it.
	 */
	cpu_icache_sync_range(cdstp + va_offset, PAGE_SIZE);
	/*
	 * Unmap the page.
	 */
	*ptep = 0;
	PTE_SYNC(ptep);
	pmap_tlb_flushID_SE(pmap_kernel(), cdstp + va_offset);

	md->pvh_attrs |= PVF_EXEC;
	PMAPCOUNT(exec_synced);
}

void
pmap_flush_page(struct vm_page_md *md, paddr_t pa, enum pmap_flush_op flush)
{
	vsize_t va_offset, end_va;
	void (*cf)(vaddr_t, vsize_t);

	if (arm_cache_prefer_mask == 0)
		return;

	switch (flush) {
	case PMAP_FLUSH_PRIMARY:
		if (md->pvh_attrs & PVF_MULTCLR) {
			va_offset = 0;
			end_va = arm_cache_prefer_mask;
			md->pvh_attrs &= ~PVF_MULTCLR;
			PMAPCOUNT(vac_flush_lots);
		} else {
			va_offset = md->pvh_attrs & arm_cache_prefer_mask;
			end_va = va_offset;
			PMAPCOUNT(vac_flush_one);
		}
		/*
		 * Mark that the page is no longer dirty.
		 */
		md->pvh_attrs &= ~PVF_DIRTY;
		cf = cpufuncs.cf_idcache_wbinv_range;
		break;
	case PMAP_FLUSH_SECONDARY:
		va_offset = 0;
		end_va = arm_cache_prefer_mask;
		cf = cpufuncs.cf_idcache_wbinv_range;
		md->pvh_attrs &= ~PVF_MULTCLR;
		PMAPCOUNT(vac_flush_lots);
		break;
	case PMAP_CLEAN_PRIMARY:
		va_offset = md->pvh_attrs & arm_cache_prefer_mask;
		end_va = va_offset;
		cf = cpufuncs.cf_dcache_wb_range;
		/*
		 * Mark that the page is no longer dirty.
		 */
		if ((md->pvh_attrs & PVF_DMOD) == 0)
			md->pvh_attrs &= ~PVF_DIRTY;
		PMAPCOUNT(vac_clean_one);
		break;
	default:
		return;
	}

	KASSERT(!(md->pvh_attrs & PVF_NC));

	NPDEBUG(PDB_VAC, printf("pmap_flush_page: md=%p (attrs=%#x)\n",
	    md, md->pvh_attrs));

	for (; va_offset <= end_va; va_offset += PAGE_SIZE) {
		const size_t pte_offset = va_offset >> PGSHIFT;
		pt_entry_t * const ptep = &cdst_pte[pte_offset];
		const pt_entry_t oldpte = *ptep;

		if (flush == PMAP_FLUSH_SECONDARY
		    && va_offset == (md->pvh_attrs & arm_cache_prefer_mask))
			continue;

		pmap_tlb_flushID_SE(pmap_kernel(), cdstp + va_offset);
		/*
		 * Set up a PTE with the right coloring to flush
		 * existing cache entries.
		 */
		*ptep = L2_S_PROTO
		    | pa
		    | L2_S_PROT(PTE_KERNEL, VM_PROT_READ|VM_PROT_WRITE)
		    | pte_l2_s_cache_mode;
		PTE_SYNC(ptep);

		/*
		 * Flush it.
		 */
		(*cf)(cdstp + va_offset, PAGE_SIZE);

		/*
		 * Restore the page table entry since we might have interrupted
		 * pmap_zero_page or pmap_copy_page which was already using
		 * this pte.
		 */
		*ptep = oldpte;
		PTE_SYNC(ptep);
		pmap_tlb_flushID_SE(pmap_kernel(), cdstp + va_offset);
	}
}
#endif /* PMAP_CACHE_VIPT */

/*
 * Routine:	pmap_page_remove
 * Function:
 *		Removes this physical page from
 *		all physical maps in which it resides.
 *		Reflects back modify bits to the pager.
 */
static void
pmap_page_remove(struct vm_page_md *md, paddr_t pa)
{
	struct l2_bucket *l2b;
	struct pv_entry *pv, *npv, **pvp;
	pmap_t pm;
	pt_entry_t *ptep;
	bool flush;
	u_int flags;

	NPDEBUG(PDB_FOLLOW,
	    printf("pmap_page_remove: md %p (0x%08lx)\n", md,
	    pa));

	PMAP_HEAD_TO_MAP_LOCK();
	simple_lock(&md->pvh_slock);

	pv = SLIST_FIRST(&md->pvh_list);
	if (pv == NULL) {
#ifdef PMAP_CACHE_VIPT
		/*
		 * We *know* the page contents are about to be replaced.
		 * Discard the exec contents
		 */
		if (PV_IS_EXEC_P(md->pvh_attrs))
			PMAPCOUNT(exec_discarded_page_protect);
		md->pvh_attrs &= ~PVF_EXEC;
		KASSERT((md->urw_mappings + md->krw_mappings == 0) == !(md->pvh_attrs & PVF_WRITE));
#endif
		simple_unlock(&md->pvh_slock);
		PMAP_HEAD_TO_MAP_UNLOCK();
		return;
	}
#ifdef PMAP_CACHE_VIPT
	KASSERT(arm_cache_prefer_mask == 0 || pmap_is_page_colored_p(md));
#endif

	/*
	 * Clear alias counts
	 */
#ifdef PMAP_CACHE_VIVT
	md->k_mappings = 0;
#endif
	md->urw_mappings = md->uro_mappings = 0;

	flush = false;
	flags = 0;

#ifdef PMAP_CACHE_VIVT
	pmap_clean_page(pv, false);
#endif

	pvp = &SLIST_FIRST(&md->pvh_list);
	while (pv) {
		pm = pv->pv_pmap;
		npv = SLIST_NEXT(pv, pv_link);
		if (flush == false && pmap_is_current(pm))
			flush = true;

		if (pm == pmap_kernel()) {
#ifdef PMAP_CACHE_VIPT
			/*
			 * If this was unmanaged mapping, it must be preserved.
			 * Move it back on the list and advance the end-of-list
			 * pointer.
			 */
			if (pv->pv_flags & PVF_KENTRY) {
				*pvp = pv;
				pvp = &SLIST_NEXT(pv, pv_link);
				pv = npv;
				continue;
			}
			if (pv->pv_flags & PVF_WRITE)
				md->krw_mappings--;
			else
				md->kro_mappings--;
#endif
			PMAPCOUNT(kernel_unmappings);
		}
		PMAPCOUNT(unmappings);

		pmap_acquire_pmap_lock(pm);

		l2b = pmap_get_l2_bucket(pm, pv->pv_va);
		KDASSERT(l2b != NULL);

		ptep = &l2b->l2b_kva[l2pte_index(pv->pv_va)];

		/*
		 * Update statistics
		 */
		--pm->pm_stats.resident_count;

		/* Wired bit */
		if (pv->pv_flags & PVF_WIRED)
			--pm->pm_stats.wired_count;

		flags |= pv->pv_flags;

		/*
		 * Invalidate the PTEs.
		 */
		*ptep = 0;
		PTE_SYNC_CURRENT(pm, ptep);
		pmap_free_l2_bucket(pm, l2b, 1);

		pool_put(&pmap_pv_pool, pv);
		pv = npv;
		/*
		 * if we reach the end of the list and there are still
		 * mappings, they might be able to be cached now.
		 */
		if (pv == NULL) {
			*pvp = NULL;
			if (!SLIST_EMPTY(&md->pvh_list))
				pmap_vac_me_harder(md, pa, pm, 0);
		}
		pmap_release_pmap_lock(pm);
	}
#ifdef PMAP_CACHE_VIPT
	/*
	 * Its EXEC cache is now gone.
	 */
	if (PV_IS_EXEC_P(md->pvh_attrs))
		PMAPCOUNT(exec_discarded_page_protect);
	md->pvh_attrs &= ~PVF_EXEC;
	KASSERT(md->urw_mappings == 0);
	KASSERT(md->uro_mappings == 0);
	if (md->krw_mappings == 0)
		md->pvh_attrs &= ~PVF_WRITE;
	KASSERT((md->urw_mappings + md->krw_mappings == 0) == !(md->pvh_attrs & PVF_WRITE));
#endif
	simple_unlock(&md->pvh_slock);
	PMAP_HEAD_TO_MAP_UNLOCK();

	if (flush) {
		/*
		 * Note: We can't use pmap_tlb_flush{I,D}() here since that
		 * would need a subsequent call to pmap_update() to ensure
		 * curpm->pm_cstate.cs_all is reset. Our callers are not
		 * required to do that (see pmap(9)), so we can't modify
		 * the current pmap's state.
		 */
		if (PV_BEEN_EXECD(flags))
			cpu_tlb_flushID();
		else
			cpu_tlb_flushD();
	}
	cpu_cpwait();
}

/*
 * pmap_t pmap_create(void)
 *  
 *      Create a new pmap structure from scratch.
 */
pmap_t
pmap_create(void)
{
	pmap_t pm;

	pm = pool_cache_get(&pmap_cache, PR_WAITOK);

	UVM_OBJ_INIT(&pm->pm_obj, NULL, 1);
	pm->pm_stats.wired_count = 0;
	pm->pm_stats.resident_count = 1;
	pm->pm_cstate.cs_all = 0;
	pmap_alloc_l1(pm);

	/*
	 * Note: The pool cache ensures that the pm_l2[] array is already
	 * initialised to zero.
	 */

	pmap_pinit(pm);

	LIST_INSERT_HEAD(&pmap_pmaps, pm, pm_list);

	return (pm);
}

/*
 * int pmap_enter(pmap_t pm, vaddr_t va, paddr_t pa, vm_prot_t prot,
 *      u_int flags)
 *  
 *      Insert the given physical page (p) at
 *      the specified virtual address (v) in the
 *      target physical map with the protection requested.
 *
 *      NB:  This is the only routine which MAY NOT lazy-evaluate
 *      or lose information.  That is, this routine must actually
 *      insert this page into the given map NOW.
 */
int
pmap_enter(pmap_t pm, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	struct l2_bucket *l2b;
	struct vm_page *pg, *opg;
	struct pv_entry *pv;
	pt_entry_t *ptep, npte, opte;
	u_int nflags;
	u_int oflags;

	NPDEBUG(PDB_ENTER, printf("pmap_enter: pm %p va 0x%lx pa 0x%lx prot %x flag %x\n", pm, va, pa, prot, flags));

	KDASSERT((flags & PMAP_WIRED) == 0 || (flags & VM_PROT_ALL) != 0);
	KDASSERT(((va | pa) & PGOFSET) == 0);

	/*
	 * Get a pointer to the page.  Later on in this function, we
	 * test for a managed page by checking pg != NULL.
	 */
	pg = pmap_initialized ? PHYS_TO_VM_PAGE(pa) : NULL;

	nflags = 0;
	if (prot & VM_PROT_WRITE)
		nflags |= PVF_WRITE;
	if (prot & VM_PROT_EXECUTE)
		nflags |= PVF_EXEC;
	if (flags & PMAP_WIRED)
		nflags |= PVF_WIRED;

	PMAP_MAP_TO_HEAD_LOCK();
	pmap_acquire_pmap_lock(pm);

	/*
	 * Fetch the L2 bucket which maps this page, allocating one if
	 * necessary for user pmaps.
	 */
	if (pm == pmap_kernel())
		l2b = pmap_get_l2_bucket(pm, va);
	else
		l2b = pmap_alloc_l2_bucket(pm, va);
	if (l2b == NULL) {
		if (flags & PMAP_CANFAIL) {
			pmap_release_pmap_lock(pm);
			PMAP_MAP_TO_HEAD_UNLOCK();
			return (ENOMEM);
		}
		panic("pmap_enter: failed to allocate L2 bucket");
	}
	ptep = &l2b->l2b_kva[l2pte_index(va)];
	opte = *ptep;
	npte = pa;
	oflags = 0;

	if (opte) {
		/*
		 * There is already a mapping at this address.
		 * If the physical address is different, lookup the
		 * vm_page.
		 */
		if (l2pte_pa(opte) != pa)
			opg = PHYS_TO_VM_PAGE(l2pte_pa(opte));
		else
			opg = pg;
	} else
		opg = NULL;

	if (pg) {
		struct vm_page_md *md = VM_PAGE_TO_MD(pg);

		/*
		 * This is to be a managed mapping.
		 */
		if ((flags & VM_PROT_ALL) ||
		    (md->pvh_attrs & PVF_REF)) {
			/*
			 * - The access type indicates that we don't need
			 *   to do referenced emulation.
			 * OR
			 * - The physical page has already been referenced
			 *   so no need to re-do referenced emulation here.
			 */
			npte |= l2pte_set_readonly(L2_S_PROTO);

			nflags |= PVF_REF;

			if ((prot & VM_PROT_WRITE) != 0 &&
			    ((flags & VM_PROT_WRITE) != 0 ||
			     (md->pvh_attrs & PVF_MOD) != 0)) {
				/*
				 * This is a writable mapping, and the
				 * page's mod state indicates it has
				 * already been modified. Make it
				 * writable from the outset.
				 */
				npte = l2pte_set_writable(npte);
				nflags |= PVF_MOD;
			}
		} else {
			/*
			 * Need to do page referenced emulation.
			 */
			npte |= L2_TYPE_INV;
		}

		npte |= pte_l2_s_cache_mode;

		if (pg == opg) {
			/*
			 * We're changing the attrs of an existing mapping.
			 */
			simple_lock(&md->pvh_slock);
			oflags = pmap_modify_pv(md, pa, pm, va,
			    PVF_WRITE | PVF_EXEC | PVF_WIRED |
			    PVF_MOD | PVF_REF, nflags);
			simple_unlock(&md->pvh_slock);

#ifdef PMAP_CACHE_VIVT
			/*
			 * We may need to flush the cache if we're
			 * doing rw-ro...
			 */
			if (pm->pm_cstate.cs_cache_d &&
			    (oflags & PVF_NC) == 0 &&
			    l2pte_writable_p(opte) &&
			    (prot & VM_PROT_WRITE) == 0)
				cpu_dcache_wb_range(va, PAGE_SIZE);
#endif
		} else {
			/*
			 * New mapping, or changing the backing page
			 * of an existing mapping.
			 */
			if (opg) {
				struct vm_page_md *omd = VM_PAGE_TO_MD(opg);
				paddr_t opa = VM_PAGE_TO_PHYS(opg);

				/*
				 * Replacing an existing mapping with a new one.
				 * It is part of our managed memory so we
				 * must remove it from the PV list
				 */
				simple_lock(&omd->pvh_slock);
				pv = pmap_remove_pv(omd, opa, pm, va);
				pmap_vac_me_harder(omd, opa, pm, 0);
				simple_unlock(&omd->pvh_slock);
				oflags = pv->pv_flags;

#ifdef PMAP_CACHE_VIVT
				/*
				 * If the old mapping was valid (ref/mod
				 * emulation creates 'invalid' mappings
				 * initially) then make sure to frob
				 * the cache.
				 */
				if ((oflags & PVF_NC) == 0 &&
				    l2pte_valid(opte)) {
					if (PV_BEEN_EXECD(oflags)) {
						pmap_idcache_wbinv_range(pm, va,
						    PAGE_SIZE);
					} else
					if (PV_BEEN_REFD(oflags)) {
						pmap_dcache_wb_range(pm, va,
						    PAGE_SIZE, true,
						    (oflags & PVF_WRITE) == 0);
					}
				}
#endif
			} else
			if ((pv = pool_get(&pmap_pv_pool, PR_NOWAIT)) == NULL){
				if ((flags & PMAP_CANFAIL) == 0)
					panic("pmap_enter: no pv entries");

				if (pm != pmap_kernel())
					pmap_free_l2_bucket(pm, l2b, 0);
				pmap_release_pmap_lock(pm);
				PMAP_MAP_TO_HEAD_UNLOCK();
				NPDEBUG(PDB_ENTER,
				    printf("pmap_enter: ENOMEM\n"));
				return (ENOMEM);
			}

			pmap_enter_pv(md, pa, pv, pm, va, nflags);
		}
	} else {
		/*
		 * We're mapping an unmanaged page.
		 * These are always readable, and possibly writable, from
		 * the get go as we don't need to track ref/mod status.
		 */
		npte |= l2pte_set_readonly(L2_S_PROTO);
		if (prot & VM_PROT_WRITE)
			npte = l2pte_set_writable(npte);

		/*
		 * Make sure the vector table is mapped cacheable
		 */
		if (pm != pmap_kernel() && va == vector_page)
			npte |= pte_l2_s_cache_mode;

		if (opg) {
			/*
			 * Looks like there's an existing 'managed' mapping
			 * at this address.
			 */
			struct vm_page_md *omd = VM_PAGE_TO_MD(opg);
			paddr_t opa = VM_PAGE_TO_PHYS(opg);

			simple_lock(&omd->pvh_slock);
			pv = pmap_remove_pv(omd, opa, pm, va);
			pmap_vac_me_harder(omd, opa, pm, 0);
			simple_unlock(&omd->pvh_slock);
			oflags = pv->pv_flags;

#ifdef PMAP_CACHE_VIVT
			if ((oflags & PVF_NC) == 0 && l2pte_valid(opte)) {
				if (PV_BEEN_EXECD(oflags))
					pmap_idcache_wbinv_range(pm, va,
					    PAGE_SIZE);
				else
				if (PV_BEEN_REFD(oflags))
					pmap_dcache_wb_range(pm, va, PAGE_SIZE,
					    true, (oflags & PVF_WRITE) == 0);
			}
#endif
			pool_put(&pmap_pv_pool, pv);
		}
	}

	/*
	 * Make sure userland mappings get the right permissions
	 */
	if (pm != pmap_kernel() && va != vector_page)
		npte |= L2_S_PROT_U;

	/*
	 * Keep the stats up to date
	 */
	if (opte == 0) {
		l2b->l2b_occupancy++;
		pm->pm_stats.resident_count++;
	} 

	NPDEBUG(PDB_ENTER,
	    printf("pmap_enter: opte 0x%08x npte 0x%08x\n", opte, npte));

	/*
	 * If this is just a wiring change, the two PTEs will be
	 * identical, so there's no need to update the page table.
	 */
	if (npte != opte) {
		bool is_cached = pmap_is_cached(pm);

		*ptep = npte;
		if (is_cached) {
			/*
			 * We only need to frob the cache/tlb if this pmap
			 * is current
			 */
			PTE_SYNC(ptep);
			if (va != vector_page && l2pte_valid(npte)) {
				/*
				 * This mapping is likely to be accessed as
				 * soon as we return to userland. Fix up the
				 * L1 entry to avoid taking another
				 * page/domain fault.
				 */
				pd_entry_t *pl1pd, l1pd;

				pl1pd = &pm->pm_l1->l1_kva[L1_IDX(va)];
				l1pd = l2b->l2b_phys | L1_C_DOM(pm->pm_domain) |
				    L1_C_PROTO;
				if (*pl1pd != l1pd) {
					*pl1pd = l1pd;
					PTE_SYNC(pl1pd);
				}
			}
		}

		if (PV_BEEN_EXECD(oflags))
			pmap_tlb_flushID_SE(pm, va);
		else
		if (PV_BEEN_REFD(oflags))
			pmap_tlb_flushD_SE(pm, va);

		NPDEBUG(PDB_ENTER,
		    printf("pmap_enter: is_cached %d cs 0x%08x\n",
		    is_cached, pm->pm_cstate.cs_all));

		if (pg != NULL) {
			struct vm_page_md *md = VM_PAGE_TO_MD(pg);

			simple_lock(&md->pvh_slock);
			pmap_vac_me_harder(md, pa, pm, va);
			simple_unlock(&md->pvh_slock);
		}
	}
#if defined(PMAP_CACHE_VIPT) && defined(DIAGNOSTIC)
	if (pg) {
		struct vm_page_md *md = VM_PAGE_TO_MD(pg);

		simple_lock(&md->pvh_slock);
		KASSERT((md->pvh_attrs & PVF_DMOD) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
		KASSERT(((md->pvh_attrs & PVF_WRITE) == 0) == (md->urw_mappings + md->krw_mappings == 0));
		simple_unlock(&md->pvh_slock);
	}
#endif

	pmap_release_pmap_lock(pm);
	PMAP_MAP_TO_HEAD_UNLOCK();

	return (0);
}

/*
 * pmap_remove()
 *
 * pmap_remove is responsible for nuking a number of mappings for a range
 * of virtual address space in the current pmap. To do this efficiently
 * is interesting, because in a number of cases a wide virtual address
 * range may be supplied that contains few actual mappings. So, the
 * optimisations are:
 *  1. Skip over hunks of address space for which no L1 or L2 entry exists.
 *  2. Build up a list of pages we've hit, up to a maximum, so we can
 *     maybe do just a partial cache clean. This path of execution is
 *     complicated by the fact that the cache must be flushed _before_
 *     the PTE is nuked, being a VAC :-)
 *  3. If we're called after UVM calls pmap_remove_all(), we can defer
 *     all invalidations until pmap_update(), since pmap_remove_all() has
 *     already flushed the cache.
 *  4. Maybe later fast-case a single page, but I don't think this is
 *     going to make _that_ much difference overall.
 */

#define	PMAP_REMOVE_CLEAN_LIST_SIZE	3

void
pmap_remove(pmap_t pm, vaddr_t sva, vaddr_t eva)
{
	struct l2_bucket *l2b;
	vaddr_t next_bucket;
	pt_entry_t *ptep;
	u_int cleanlist_idx, total, cnt;
	struct {
		vaddr_t va;
		pt_entry_t *ptep;
	} cleanlist[PMAP_REMOVE_CLEAN_LIST_SIZE];
	u_int mappings, is_exec, is_refd;

	NPDEBUG(PDB_REMOVE, printf("pmap_do_remove: pmap=%p sva=%08lx "
	    "eva=%08lx\n", pm, sva, eva));

	/*
	 * we lock in the pmap => pv_head direction
	 */
	PMAP_MAP_TO_HEAD_LOCK();
	pmap_acquire_pmap_lock(pm);

	if (pm->pm_remove_all || !pmap_is_cached(pm)) {
		cleanlist_idx = PMAP_REMOVE_CLEAN_LIST_SIZE + 1;
		if (pm->pm_cstate.cs_tlb == 0)
			pm->pm_remove_all = true;
	} else
		cleanlist_idx = 0;

	total = 0;

	while (sva < eva) {
		/*
		 * Do one L2 bucket's worth at a time.
		 */
		next_bucket = L2_NEXT_BUCKET(sva);
		if (next_bucket > eva)
			next_bucket = eva;

		l2b = pmap_get_l2_bucket(pm, sva);
		if (l2b == NULL) {
			sva = next_bucket;
			continue;
		}

		ptep = &l2b->l2b_kva[l2pte_index(sva)];

		for (mappings = 0; sva < next_bucket; sva += PAGE_SIZE, ptep++){
			struct vm_page *pg;
			pt_entry_t pte;
			paddr_t pa;

			pte = *ptep;

			if (pte == 0) {
				/* Nothing here, move along */
				continue;
			}

			pa = l2pte_pa(pte);
			is_exec = 0;
			is_refd = 1;

			/*
			 * Update flags. In a number of circumstances,
			 * we could cluster a lot of these and do a
			 * number of sequential pages in one go.
			 */
			if ((pg = PHYS_TO_VM_PAGE(pa)) != NULL) {
				struct vm_page_md *md = VM_PAGE_TO_MD(pg);
				struct pv_entry *pv;

				simple_lock(&md->pvh_slock);
				pv = pmap_remove_pv(md, pa, pm, sva);
				pmap_vac_me_harder(md, pa, pm, 0);
				simple_unlock(&md->pvh_slock);
				if (pv != NULL) {
					if (pm->pm_remove_all == false) {
						is_exec =
						   PV_BEEN_EXECD(pv->pv_flags);
						is_refd =
						   PV_BEEN_REFD(pv->pv_flags);
					}
					pool_put(&pmap_pv_pool, pv);
				}
			}
			mappings++;

			if (!l2pte_valid(pte)) {
				/*
				 * Ref/Mod emulation is still active for this
				 * mapping, therefore it is has not yet been
				 * accessed. No need to frob the cache/tlb.
				 */
				*ptep = 0;
				PTE_SYNC_CURRENT(pm, ptep);
				continue;
			}

			if (cleanlist_idx < PMAP_REMOVE_CLEAN_LIST_SIZE) {
				/* Add to the clean list. */
				cleanlist[cleanlist_idx].ptep = ptep;
				cleanlist[cleanlist_idx].va =
				    sva | (is_exec & 1);
				cleanlist_idx++;
			} else
			if (cleanlist_idx == PMAP_REMOVE_CLEAN_LIST_SIZE) {
				/* Nuke everything if needed. */
#ifdef PMAP_CACHE_VIVT
				pmap_idcache_wbinv_all(pm);
#endif
				pmap_tlb_flushID(pm);

				/*
				 * Roll back the previous PTE list,
				 * and zero out the current PTE.
				 */
				for (cnt = 0;
				     cnt < PMAP_REMOVE_CLEAN_LIST_SIZE; cnt++) {
					*cleanlist[cnt].ptep = 0;
					PTE_SYNC(cleanlist[cnt].ptep);
				}
				*ptep = 0;
				PTE_SYNC(ptep);
				cleanlist_idx++;
				pm->pm_remove_all = true;
			} else {
				*ptep = 0;
				PTE_SYNC(ptep);
				if (pm->pm_remove_all == false) {
					if (is_exec)
						pmap_tlb_flushID_SE(pm, sva);
					else
					if (is_refd)
						pmap_tlb_flushD_SE(pm, sva);
				}
			}
		}

		/*
		 * Deal with any left overs
		 */
		if (cleanlist_idx <= PMAP_REMOVE_CLEAN_LIST_SIZE) {
			total += cleanlist_idx;
			for (cnt = 0; cnt < cleanlist_idx; cnt++) {
				if (pm->pm_cstate.cs_all != 0) {
					vaddr_t clva = cleanlist[cnt].va & ~1;
					if (cleanlist[cnt].va & 1) {
#ifdef PMAP_CACHE_VIVT
						pmap_idcache_wbinv_range(pm,
						    clva, PAGE_SIZE);
#endif
						pmap_tlb_flushID_SE(pm, clva);
					} else {
#ifdef PMAP_CACHE_VIVT
						pmap_dcache_wb_range(pm,
						    clva, PAGE_SIZE, true,
						    false);
#endif
						pmap_tlb_flushD_SE(pm, clva);
					}
				}
				*cleanlist[cnt].ptep = 0;
				PTE_SYNC_CURRENT(pm, cleanlist[cnt].ptep);
			}

			/*
			 * If it looks like we're removing a whole bunch
			 * of mappings, it's faster to just write-back
			 * the whole cache now and defer TLB flushes until
			 * pmap_update() is called.
			 */
			if (total <= PMAP_REMOVE_CLEAN_LIST_SIZE)
				cleanlist_idx = 0;
			else {
				cleanlist_idx = PMAP_REMOVE_CLEAN_LIST_SIZE + 1;
#ifdef PMAP_CACHE_VIVT
				pmap_idcache_wbinv_all(pm);
#endif
				pm->pm_remove_all = true;
			}
		}

		pmap_free_l2_bucket(pm, l2b, mappings);
		pm->pm_stats.resident_count -= mappings;
	}

	pmap_release_pmap_lock(pm);
	PMAP_MAP_TO_HEAD_UNLOCK();
}

#ifdef PMAP_CACHE_VIPT
static struct pv_entry *
pmap_kremove_pg(struct vm_page *pg, vaddr_t va)
{
	struct vm_page_md *md = VM_PAGE_TO_MD(pg);
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	struct pv_entry *pv;

	simple_lock(&md->pvh_slock);
	KASSERT(arm_cache_prefer_mask == 0 || md->pvh_attrs & (PVF_COLORED|PVF_NC));
	KASSERT((md->pvh_attrs & PVF_KMPAGE) == 0);

	pv = pmap_remove_pv(md, pa, pmap_kernel(), va);
	KASSERT(pv);
	KASSERT(pv->pv_flags & PVF_KENTRY);

	/*
	 * If we are removing a writeable mapping to a cached exec page,
	 * if it's the last mapping then clear it execness other sync
	 * the page to the icache.
	 */
	if ((md->pvh_attrs & (PVF_NC|PVF_EXEC)) == PVF_EXEC
	    && (pv->pv_flags & PVF_WRITE) != 0) {
		if (SLIST_EMPTY(&md->pvh_list)) {
			md->pvh_attrs &= ~PVF_EXEC;
			PMAPCOUNT(exec_discarded_kremove);
		} else {
			pmap_syncicache_page(md, pa);
			PMAPCOUNT(exec_synced_kremove);
		}
	}
	pmap_vac_me_harder(md, pa, pmap_kernel(), 0);
	simple_unlock(&md->pvh_slock);

	return pv;
}
#endif /* PMAP_CACHE_VIPT */

/*
 * pmap_kenter_pa: enter an unmanaged, wired kernel mapping
 *
 * We assume there is already sufficient KVM space available
 * to do this, as we can't allocate L2 descriptor tables/metadata
 * from here.
 */
void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	struct l2_bucket *l2b;
	pt_entry_t *ptep, opte;
#ifdef PMAP_CACHE_VIVT
	struct vm_page *pg = (flags & PMAP_KMPAGE) ? PHYS_TO_VM_PAGE(pa) : NULL;
#endif
#ifdef PMAP_CACHE_VIPT
	struct vm_page *pg = PHYS_TO_VM_PAGE(pa);
	struct vm_page *opg;
	struct pv_entry *pv = NULL;
#endif
	struct vm_page_md *md = VM_PAGE_TO_MD(pg);

	NPDEBUG(PDB_KENTER,
	    printf("pmap_kenter_pa: va 0x%08lx, pa 0x%08lx, prot 0x%x\n",
	    va, pa, prot));

	l2b = pmap_get_l2_bucket(pmap_kernel(), va);
	KDASSERT(l2b != NULL);

	ptep = &l2b->l2b_kva[l2pte_index(va)];
	opte = *ptep;

	if (opte == 0) {
		PMAPCOUNT(kenter_mappings);
		l2b->l2b_occupancy++;
	} else {
		PMAPCOUNT(kenter_remappings);
#ifdef PMAP_CACHE_VIPT
		opg = PHYS_TO_VM_PAGE(l2pte_pa(opte));
		struct vm_page_md *omd = VM_PAGE_TO_MD(opg);
		if (opg) {
			KASSERT(opg != pg);
			KASSERT((omd->pvh_attrs & PVF_KMPAGE) == 0);
			KASSERT((flags & PMAP_KMPAGE) == 0);
			simple_lock(&omd->pvh_slock);
			pv = pmap_kremove_pg(opg, va);
			simple_unlock(&omd->pvh_slock);
		}
#endif
		if (l2pte_valid(opte)) {
#ifdef PMAP_CACHE_VIVT
			cpu_dcache_wbinv_range(va, PAGE_SIZE);
#endif
			cpu_tlb_flushD_SE(va);
			cpu_cpwait();
		}
	}

	*ptep = L2_S_PROTO | pa | L2_S_PROT(PTE_KERNEL, prot) |
	    pte_l2_s_cache_mode;
	PTE_SYNC(ptep);

	if (pg) {
		if (flags & PMAP_KMPAGE) {
			simple_lock(&md->pvh_slock);
			KASSERT(md->urw_mappings == 0);
			KASSERT(md->uro_mappings == 0);
			KASSERT(md->krw_mappings == 0);
			KASSERT(md->kro_mappings == 0);
#ifdef PMAP_CACHE_VIPT
			KASSERT(pv == NULL);
			KASSERT(arm_cache_prefer_mask == 0 || (va & PVF_COLORED) == 0);
			KASSERT((md->pvh_attrs & PVF_NC) == 0);
			/* if there is a color conflict, evict from cache. */
			if (pmap_is_page_colored_p(md)
			    && ((va ^ md->pvh_attrs) & arm_cache_prefer_mask)) {
				PMAPCOUNT(vac_color_change);
				pmap_flush_page(md, pa, PMAP_FLUSH_PRIMARY);
			} else if (md->pvh_attrs & PVF_MULTCLR) {
				/*
				 * If this page has multiple colors, expunge
				 * them.
				 */
				PMAPCOUNT(vac_flush_lots2);
				pmap_flush_page(md, pa, PMAP_FLUSH_SECONDARY);
			}
			md->pvh_attrs &= PAGE_SIZE - 1;
			md->pvh_attrs |= PVF_KMPAGE
			    | PVF_COLORED | PVF_DIRTY
			    | (va & arm_cache_prefer_mask);
#endif
#ifdef PMAP_CACHE_VIVT
			md->pvh_attrs |= PVF_KMPAGE;
#endif
			pmap_kmpages++;
			simple_unlock(&md->pvh_slock);
#ifdef PMAP_CACHE_VIPT
		} else {
			if (pv == NULL) {
				pv = pool_get(&pmap_pv_pool, PR_NOWAIT);
				KASSERT(pv != NULL);
			}
			pmap_enter_pv(md, pa, pv, pmap_kernel(), va,
			    PVF_WIRED | PVF_KENTRY
			    | (prot & VM_PROT_WRITE ? PVF_WRITE : 0));
			if ((prot & VM_PROT_WRITE)
			    && !(md->pvh_attrs & PVF_NC))
				md->pvh_attrs |= PVF_DIRTY;
			KASSERT((prot & VM_PROT_WRITE) == 0 || (md->pvh_attrs & (PVF_DIRTY|PVF_NC)));
			simple_lock(&md->pvh_slock);
			pmap_vac_me_harder(md, pa, pmap_kernel(), va);
			simple_unlock(&md->pvh_slock);
#endif
		}
#ifdef PMAP_CACHE_VIPT
	} else {
		if (pv != NULL)
			pool_put(&pmap_pv_pool, pv);
#endif
	}
}

void
pmap_kremove(vaddr_t va, vsize_t len)
{
	struct l2_bucket *l2b;
	pt_entry_t *ptep, *sptep, opte;
	vaddr_t next_bucket, eva;
	u_int mappings;
	struct vm_page *opg;

	PMAPCOUNT(kenter_unmappings);

	NPDEBUG(PDB_KREMOVE, printf("pmap_kremove: va 0x%08lx, len 0x%08lx\n",
	    va, len));

	eva = va + len;

	while (va < eva) {
		next_bucket = L2_NEXT_BUCKET(va);
		if (next_bucket > eva)
			next_bucket = eva;

		l2b = pmap_get_l2_bucket(pmap_kernel(), va);
		KDASSERT(l2b != NULL);

		sptep = ptep = &l2b->l2b_kva[l2pte_index(va)];
		mappings = 0;

		while (va < next_bucket) {
			opte = *ptep;
			opg = PHYS_TO_VM_PAGE(l2pte_pa(opte));
			if (opg) {
				struct vm_page_md *omd = VM_PAGE_TO_MD(opg);

				if (omd->pvh_attrs & PVF_KMPAGE) {
					simple_lock(&omd->pvh_slock);
					KASSERT(omd->urw_mappings == 0);
					KASSERT(omd->uro_mappings == 0);
					KASSERT(omd->krw_mappings == 0);
					KASSERT(omd->kro_mappings == 0);
					omd->pvh_attrs &= ~PVF_KMPAGE;
#ifdef PMAP_CACHE_VIPT
					omd->pvh_attrs &= ~PVF_WRITE;
#endif
					pmap_kmpages--;
					simple_unlock(&omd->pvh_slock);
#ifdef PMAP_CACHE_VIPT
				} else {
					pool_put(&pmap_pv_pool,
					    pmap_kremove_pg(opg, va));
#endif
				}
			}
			if (l2pte_valid(opte)) {
#ifdef PMAP_CACHE_VIVT
				cpu_dcache_wbinv_range(va, PAGE_SIZE);
#endif
				cpu_tlb_flushD_SE(va);
			}
			if (opte) {
				*ptep = 0;
				mappings++;
			}
			va += PAGE_SIZE;
			ptep++;
		}
		KDASSERT(mappings <= l2b->l2b_occupancy);
		l2b->l2b_occupancy -= mappings;
		PTE_SYNC_RANGE(sptep, (u_int)(ptep - sptep));
	}
	cpu_cpwait();
}

bool
pmap_extract(pmap_t pm, vaddr_t va, paddr_t *pap)
{
	struct l2_dtable *l2;
	pd_entry_t *pl1pd, l1pd;
	pt_entry_t *ptep, pte;
	paddr_t pa;
	u_int l1idx;

	pmap_acquire_pmap_lock(pm);

	l1idx = L1_IDX(va);
	pl1pd = &pm->pm_l1->l1_kva[l1idx];
	l1pd = *pl1pd;

	if (l1pte_section_p(l1pd)) {
		/*
		 * These should only happen for pmap_kernel()
		 */
		KDASSERT(pm == pmap_kernel());
		pmap_release_pmap_lock(pm);
		pa = (l1pd & L1_S_FRAME) | (va & L1_S_OFFSET);
	} else {
		/*
		 * Note that we can't rely on the validity of the L1
		 * descriptor as an indication that a mapping exists.
		 * We have to look it up in the L2 dtable.
		 */
		l2 = pm->pm_l2[L2_IDX(l1idx)];

		if (l2 == NULL ||
		    (ptep = l2->l2_bucket[L2_BUCKET(l1idx)].l2b_kva) == NULL) {
			pmap_release_pmap_lock(pm);
			return false;
		}

		ptep = &ptep[l2pte_index(va)];
		pte = *ptep;
		pmap_release_pmap_lock(pm);

		if (pte == 0)
			return false;

		switch (pte & L2_TYPE_MASK) {
		case L2_TYPE_L:
			pa = (pte & L2_L_FRAME) | (va & L2_L_OFFSET);
			break;

		default:
			pa = (pte & L2_S_FRAME) | (va & L2_S_OFFSET);
			break;
		}
	}

	if (pap != NULL)
		*pap = pa;

	return true;
}

void
pmap_protect(pmap_t pm, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	struct l2_bucket *l2b;
	pt_entry_t *ptep, pte;
	vaddr_t next_bucket;
	u_int flags;
	u_int clr_mask;
	int flush;

	NPDEBUG(PDB_PROTECT,
	    printf("pmap_protect: pm %p sva 0x%lx eva 0x%lx prot 0x%x\n",
	    pm, sva, eva, prot));

	if ((prot & VM_PROT_READ) == 0) {
		pmap_remove(pm, sva, eva);
		return;
	}

	if (prot & VM_PROT_WRITE) {
		/*
		 * If this is a read->write transition, just ignore it and let
		 * uvm_fault() take care of it later.
		 */
		return;
	}

	PMAP_MAP_TO_HEAD_LOCK();
	pmap_acquire_pmap_lock(pm);

	flush = ((eva - sva) >= (PAGE_SIZE * 4)) ? 0 : -1;
	flags = 0;
	clr_mask = PVF_WRITE | ((prot & VM_PROT_EXECUTE) ? 0 : PVF_EXEC);

	while (sva < eva) {
		next_bucket = L2_NEXT_BUCKET(sva);
		if (next_bucket > eva)
			next_bucket = eva;

		l2b = pmap_get_l2_bucket(pm, sva);
		if (l2b == NULL) {
			sva = next_bucket;
			continue;
		}

		ptep = &l2b->l2b_kva[l2pte_index(sva)];

		while (sva < next_bucket) {
			pte = *ptep;
			if (l2pte_valid(pte) != 0 && l2pte_writable_p(pte)) {
				struct vm_page *pg;
				u_int f;

#ifdef PMAP_CACHE_VIVT
				/*
				 * OK, at this point, we know we're doing
				 * write-protect operation.  If the pmap is
				 * active, write-back the page.
				 */
				pmap_dcache_wb_range(pm, sva, PAGE_SIZE,
				    false, false);
#endif

				pg = PHYS_TO_VM_PAGE(l2pte_pa(pte));
				pte = l2pte_set_readonly(pte);
				*ptep = pte;
				PTE_SYNC(ptep);

				if (pg != NULL) {
					struct vm_page_md *md = VM_PAGE_TO_MD(pg);
					paddr_t pa = VM_PAGE_TO_PHYS(pg);

					simple_lock(&md->pvh_slock);
					f = pmap_modify_pv(md, pa, pm, sva,
					    clr_mask, 0);
					pmap_vac_me_harder(md, pa, pm, sva);
					simple_unlock(&md->pvh_slock);
				} else
					f = PVF_REF | PVF_EXEC;

				if (flush >= 0) {
					flush++;
					flags |= f;
				} else
				if (PV_BEEN_EXECD(f))
					pmap_tlb_flushID_SE(pm, sva);
				else
				if (PV_BEEN_REFD(f))
					pmap_tlb_flushD_SE(pm, sva);
			}

			sva += PAGE_SIZE;
			ptep++;
		}
	}

	pmap_release_pmap_lock(pm);
	PMAP_MAP_TO_HEAD_UNLOCK();

	if (flush) {
		if (PV_BEEN_EXECD(flags))
			pmap_tlb_flushID(pm);
		else
		if (PV_BEEN_REFD(flags))
			pmap_tlb_flushD(pm);
	}
}

void
pmap_icache_sync_range(pmap_t pm, vaddr_t sva, vaddr_t eva)
{
	struct l2_bucket *l2b;
	pt_entry_t *ptep;
	vaddr_t next_bucket;
	vsize_t page_size = trunc_page(sva) + PAGE_SIZE - sva;

	NPDEBUG(PDB_EXEC,
	    printf("pmap_icache_sync_range: pm %p sva 0x%lx eva 0x%lx\n",
	    pm, sva, eva));

	PMAP_MAP_TO_HEAD_LOCK();
	pmap_acquire_pmap_lock(pm);

	while (sva < eva) {
		next_bucket = L2_NEXT_BUCKET(sva);
		if (next_bucket > eva)
			next_bucket = eva;

		l2b = pmap_get_l2_bucket(pm, sva);
		if (l2b == NULL) {
			sva = next_bucket;
			continue;
		}

		for (ptep = &l2b->l2b_kva[l2pte_index(sva)];
		     sva < next_bucket;
		     sva += page_size, ptep++, page_size = PAGE_SIZE) {
			if (l2pte_valid(*ptep)) {
				cpu_icache_sync_range(sva,
				    min(page_size, eva - sva));
			}
		}
	}

	pmap_release_pmap_lock(pm);
	PMAP_MAP_TO_HEAD_UNLOCK();
}

void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	struct vm_page_md *md = VM_PAGE_TO_MD(pg);
	paddr_t pa = VM_PAGE_TO_PHYS(pg);

	NPDEBUG(PDB_PROTECT,
	    printf("pmap_page_protect: md %p (0x%08lx), prot 0x%x\n",
	    md, pa, prot));

	switch(prot) {
	case VM_PROT_READ|VM_PROT_WRITE:
#if defined(PMAP_CHECK_VIPT) && defined(PMAP_APX)
		pmap_clearbit(md, pa, PVF_EXEC);
		break;
#endif
	case VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE:
		break;

	case VM_PROT_READ:
#if defined(PMAP_CHECK_VIPT) && defined(PMAP_APX)
		pmap_clearbit(md, pa, PVF_WRITE|PVF_EXEC);
		break;
#endif
	case VM_PROT_READ|VM_PROT_EXECUTE:
		pmap_clearbit(md, pa, PVF_WRITE);
		break;

	default:
		pmap_page_remove(md, pa);
		break;
	}
}

/*
 * pmap_clear_modify:
 *
 *	Clear the "modified" attribute for a page.
 */
bool
pmap_clear_modify(struct vm_page *pg)
{
	struct vm_page_md *md = VM_PAGE_TO_MD(pg);
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	bool rv;

	if (md->pvh_attrs & PVF_MOD) {
		rv = true;
#ifdef PMAP_CACHE_VIPT
		/*
		 * If we are going to clear the modified bit and there are
		 * no other modified bits set, flush the page to memory and
		 * mark it clean.
		 */
		if ((md->pvh_attrs & (PVF_DMOD|PVF_NC)) == PVF_MOD)
			pmap_flush_page(md, pa, PMAP_CLEAN_PRIMARY);
#endif
		pmap_clearbit(md, pa, PVF_MOD);
	} else
		rv = false;

	return (rv);
}

/*
 * pmap_clear_reference:
 *
 *	Clear the "referenced" attribute for a page.
 */
bool
pmap_clear_reference(struct vm_page *pg)
{
	struct vm_page_md *md = VM_PAGE_TO_MD(pg);
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	bool rv;

	if (md->pvh_attrs & PVF_REF) {
		rv = true;
		pmap_clearbit(md, pa, PVF_REF);
	} else
		rv = false;

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
pmap_fault_fixup(pmap_t pm, vaddr_t va, vm_prot_t ftype, int user)
{
	struct l2_dtable *l2;
	struct l2_bucket *l2b;
	pd_entry_t *pl1pd, l1pd;
	pt_entry_t *ptep, pte;
	paddr_t pa;
	u_int l1idx;
	int rv = 0;

	PMAP_MAP_TO_HEAD_LOCK();
	pmap_acquire_pmap_lock(pm);

	l1idx = L1_IDX(va);

	/*
	 * If there is no l2_dtable for this address, then the process
	 * has no business accessing it.
	 *
	 * Note: This will catch userland processes trying to access
	 * kernel addresses.
	 */
	l2 = pm->pm_l2[L2_IDX(l1idx)];
	if (l2 == NULL)
		goto out;

	/*
	 * Likewise if there is no L2 descriptor table
	 */
	l2b = &l2->l2_bucket[L2_BUCKET(l1idx)];
	if (l2b->l2b_kva == NULL)
		goto out;

	/*
	 * Check the PTE itself.
	 */
	ptep = &l2b->l2b_kva[l2pte_index(va)];
	pte = *ptep;
	if (pte == 0)
		goto out;

	/*
	 * Catch a userland access to the vector page mapped at 0x0
	 */
	if (user && (pte & L2_S_PROT_U) == 0)
		goto out;

	pa = l2pte_pa(pte);

	if ((ftype & VM_PROT_WRITE) && !l2pte_writable_p(pte)) {
		/*
		 * This looks like a good candidate for "page modified"
		 * emulation...
		 */
		struct pv_entry *pv;
		struct vm_page *pg;

		/* Extract the physical address of the page */
		if ((pg = PHYS_TO_VM_PAGE(pa)) == NULL)
			goto out;

		struct vm_page_md *md = VM_PAGE_TO_MD(pg);

		/* Get the current flags for this page. */
		simple_lock(&md->pvh_slock);

		pv = pmap_find_pv(md, pm, va);
		if (pv == NULL) {
	    		simple_unlock(&md->pvh_slock);
			goto out;
		}

		/*
		 * Do the flags say this page is writable? If not then it
		 * is a genuine write fault. If yes then the write fault is
		 * our fault as we did not reflect the write access in the
		 * PTE. Now we know a write has occurred we can correct this
		 * and also set the modified bit
		 */
		if ((pv->pv_flags & PVF_WRITE) == 0) {
		    	simple_unlock(&md->pvh_slock);
			goto out;
		}

		NPDEBUG(PDB_FOLLOW,
		    printf("pmap_fault_fixup: mod emul. pm %p, va 0x%08lx, pa 0x%08lx\n",
		    pm, va, pa));

		md->pvh_attrs |= PVF_REF | PVF_MOD;
		pv->pv_flags |= PVF_REF | PVF_MOD;
#ifdef PMAP_CACHE_VIPT
		/*
		 * If there are cacheable mappings for this page, mark it dirty.
		 */
		if ((md->pvh_attrs & PVF_NC) == 0)
			md->pvh_attrs |= PVF_DIRTY;
#endif
		simple_unlock(&md->pvh_slock);

		/* 
		 * Re-enable write permissions for the page.  No need to call
		 * pmap_vac_me_harder(), since this is just a
		 * modified-emulation fault, and the PVF_WRITE bit isn't
		 * changing. We've already set the cacheable bits based on
		 * the assumption that we can write to this page.
		 */
		*ptep = l2pte_set_writable((pte & ~L2_TYPE_MASK) | L2_S_PROTO);
		PTE_SYNC(ptep);
		rv = 1;
	} else
	if ((pte & L2_TYPE_MASK) == L2_TYPE_INV) {
		/*
		 * This looks like a good candidate for "page referenced"
		 * emulation.
		 */
		struct pv_entry *pv;
		struct vm_page *pg;

		/* Extract the physical address of the page */
		if ((pg = PHYS_TO_VM_PAGE(pa)) == NULL)
			goto out;

		struct vm_page_md *md = VM_PAGE_TO_MD(pg);

		/* Get the current flags for this page. */
		simple_lock(&md->pvh_slock);

		pv = pmap_find_pv(md, pm, va);
		if (pv == NULL) {
	    		simple_unlock(&md->pvh_slock);
			goto out;
		}

		md->pvh_attrs |= PVF_REF;
		pv->pv_flags |= PVF_REF;
		simple_unlock(&md->pvh_slock);

		NPDEBUG(PDB_FOLLOW,
		    printf("pmap_fault_fixup: ref emul. pm %p, va 0x%08lx, pa 0x%08lx\n",
		    pm, va, pa));

		*ptep = l2pte_set_readonly((pte & ~L2_TYPE_MASK) | L2_S_PROTO);
		PTE_SYNC(ptep);
		rv = 1;
	}

	/*
	 * We know there is a valid mapping here, so simply
	 * fix up the L1 if necessary.
	 */
	pl1pd = &pm->pm_l1->l1_kva[l1idx];
	l1pd = l2b->l2b_phys | L1_C_DOM(pm->pm_domain) | L1_C_PROTO;
	if (*pl1pd != l1pd) {
		*pl1pd = l1pd;
		PTE_SYNC(pl1pd);
		rv = 1;
	}

#ifdef CPU_SA110
	/*
	 * There are bugs in the rev K SA110.  This is a check for one
	 * of them.
	 */
	if (rv == 0 && curcpu()->ci_arm_cputype == CPU_ID_SA110 &&
	    curcpu()->ci_arm_cpurev < 3) {
		/* Always current pmap */
		if (l2pte_valid(pte)) {
			extern int kernel_debug;
			if (kernel_debug & 1) {
				struct proc *p = curlwp->l_proc;
				printf("prefetch_abort: page is already "
				    "mapped - pte=%p *pte=%08x\n", ptep, pte);
				printf("prefetch_abort: pc=%08lx proc=%p "
				    "process=%s\n", va, p, p->p_comm);
				printf("prefetch_abort: far=%08x fs=%x\n",
				    cpu_faultaddress(), cpu_faultstatus());
			}
#ifdef DDB
			if (kernel_debug & 2)
				Debugger();
#endif
			rv = 1;
		}
	}
#endif /* CPU_SA110 */

#ifdef DEBUG
	/*
	 * If 'rv == 0' at this point, it generally indicates that there is a
	 * stale TLB entry for the faulting address. This happens when two or
	 * more processes are sharing an L1. Since we don't flush the TLB on
	 * a context switch between such processes, we can take domain faults
	 * for mappings which exist at the same VA in both processes. EVEN IF
	 * WE'VE RECENTLY FIXED UP THE CORRESPONDING L1 in pmap_enter(), for
	 * example.
	 *
	 * This is extremely likely to happen if pmap_enter() updated the L1
	 * entry for a recently entered mapping. In this case, the TLB is
	 * flushed for the new mapping, but there may still be TLB entries for
	 * other mappings belonging to other processes in the 1MB range
	 * covered by the L1 entry.
	 *
	 * Since 'rv == 0', we know that the L1 already contains the correct
	 * value, so the fault must be due to a stale TLB entry.
	 *
	 * Since we always need to flush the TLB anyway in the case where we
	 * fixed up the L1, or frobbed the L2 PTE, we effectively deal with
	 * stale TLB entries dynamically.
	 *
	 * However, the above condition can ONLY happen if the current L1 is
	 * being shared. If it happens when the L1 is unshared, it indicates
	 * that other parts of the pmap are not doing their job WRT managing
	 * the TLB.
	 */
	if (rv == 0 && pm->pm_l1->l1_domain_use_count == 1) {
		extern int last_fault_code;
		printf("fixup: pm %p, va 0x%lx, ftype %d - nothing to do!\n",
		    pm, va, ftype);
		printf("fixup: l2 %p, l2b %p, ptep %p, pl1pd %p\n",
		    l2, l2b, ptep, pl1pd);
		printf("fixup: pte 0x%x, l1pd 0x%x, last code 0x%x\n",
		    pte, l1pd, last_fault_code);
#ifdef DDB
		Debugger();
#endif
	}
#endif

	cpu_tlb_flushID_SE(va);
	cpu_cpwait();

	rv = 1;

out:
	pmap_release_pmap_lock(pm);
	PMAP_MAP_TO_HEAD_UNLOCK();

	return (rv);
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
 * Routine:	pmap_unwire
 * Function:	Clear the wired attribute for a map/virtual-address pair.
 *
 * In/out conditions:
 *		The mapping must already exist in the pmap.
 */
void
pmap_unwire(pmap_t pm, vaddr_t va)
{
	struct l2_bucket *l2b;
	pt_entry_t *ptep, pte;
	struct vm_page *pg;
	paddr_t pa;

	NPDEBUG(PDB_WIRING, printf("pmap_unwire: pm %p, va 0x%08lx\n", pm, va));

	PMAP_MAP_TO_HEAD_LOCK();
	pmap_acquire_pmap_lock(pm);

	l2b = pmap_get_l2_bucket(pm, va);
	KDASSERT(l2b != NULL);

	ptep = &l2b->l2b_kva[l2pte_index(va)];
	pte = *ptep;

	/* Extract the physical address of the page */
	pa = l2pte_pa(pte);

	if ((pg = PHYS_TO_VM_PAGE(pa)) != NULL) {
		/* Update the wired bit in the pv entry for this page. */
		struct vm_page_md *md = VM_PAGE_TO_MD(pg);

		simple_lock(&md->pvh_slock);
		(void) pmap_modify_pv(md, pa, pm, va, PVF_WIRED, 0);
		simple_unlock(&md->pvh_slock);
	}

	pmap_release_pmap_lock(pm);
	PMAP_MAP_TO_HEAD_UNLOCK();
}

void
pmap_activate(struct lwp *l)
{
	extern int block_userspace_access;
	pmap_t opm, npm, rpm;
	uint32_t odacr, ndacr;
	int oldirqstate;

	/*
	 * If activating a non-current lwp or the current lwp is
	 * already active, just return.
	 */
	if (l != curlwp ||
	    l->l_proc->p_vmspace->vm_map.pmap->pm_activated == true)
		return;

	npm = l->l_proc->p_vmspace->vm_map.pmap;
	ndacr = (DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL * 2)) |
	    (DOMAIN_CLIENT << (npm->pm_domain * 2));

	/*
	 * If TTB and DACR are unchanged, short-circuit all the
	 * TLB/cache management stuff.
	 */
	if (pmap_previous_active_lwp != NULL) {
		opm = pmap_previous_active_lwp->l_proc->p_vmspace->vm_map.pmap;
		odacr = (DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL * 2)) |
		    (DOMAIN_CLIENT << (opm->pm_domain * 2));

		if (opm->pm_l1 == npm->pm_l1 && odacr == ndacr)
			goto all_done;
	} else
		opm = NULL;

	PMAPCOUNT(activations);
	block_userspace_access = 1;

	/*
	 * If switching to a user vmspace which is different to the
	 * most recent one, and the most recent one is potentially
	 * live in the cache, we must write-back and invalidate the
	 * entire cache.
	 */
	rpm = pmap_recent_user;

/*
 * XXXSCW: There's a corner case here which can leave turds in the cache as
 * reported in kern/41058. They're probably left over during tear-down and
 * switching away from an exiting process. Until the root cause is identified
 * and fixed, zap the cache when switching pmaps. This will result in a few
 * unnecessary cache flushes, but that's better than silently corrupting data.
 */
#if 0
	if (npm != pmap_kernel() && rpm && npm != rpm &&
	    rpm->pm_cstate.cs_cache) {
		rpm->pm_cstate.cs_cache = 0;
#ifdef PMAP_CACHE_VIVT
		cpu_idcache_wbinv_all();
#endif
	}
#else
	if (rpm) {
		rpm->pm_cstate.cs_cache = 0;
		if (npm == pmap_kernel())
			pmap_recent_user = NULL;
#ifdef PMAP_CACHE_VIVT
		cpu_idcache_wbinv_all();
#endif
	}
#endif

	/* No interrupts while we frob the TTB/DACR */
	oldirqstate = disable_interrupts(IF32_bits);

	/*
	 * For ARM_VECTORS_LOW, we MUST, I repeat, MUST fix up the L1
	 * entry corresponding to 'vector_page' in the incoming L1 table
	 * before switching to it otherwise subsequent interrupts/exceptions
	 * (including domain faults!) will jump into hyperspace.
	 */
	if (npm->pm_pl1vec != NULL) {
		cpu_tlb_flushID_SE((u_int)vector_page);
		cpu_cpwait();
		*npm->pm_pl1vec = npm->pm_l1vec;
		PTE_SYNC(npm->pm_pl1vec);
	}

	cpu_domains(ndacr);

	if (npm == pmap_kernel() || npm == rpm) {
		/*
		 * Switching to a kernel thread, or back to the
		 * same user vmspace as before... Simply update
		 * the TTB (no TLB flush required)
		 */
		__asm volatile("mcr p15, 0, %0, c2, c0, 0" ::
		    "r"(npm->pm_l1->l1_physaddr));
		cpu_cpwait();
	} else {
		/*
		 * Otherwise, update TTB and flush TLB
		 */
		cpu_context_switch(npm->pm_l1->l1_physaddr);
		if (rpm != NULL)
			rpm->pm_cstate.cs_tlb = 0;
	}

	restore_interrupts(oldirqstate);

	block_userspace_access = 0;

 all_done:
	/*
	 * The new pmap is resident. Make sure it's marked
	 * as resident in the cache/TLB.
	 */
	npm->pm_cstate.cs_all = PMAP_CACHE_STATE_ALL;
	if (npm != pmap_kernel())
		pmap_recent_user = npm;

	/* The old pmap is not longer active */
	if (opm != NULL)
		opm->pm_activated = false;

	/* But the new one is */
	npm->pm_activated = true;
}

void
pmap_deactivate(struct lwp *l)
{

	/*
	 * If the process is exiting, make sure pmap_activate() does
	 * a full MMU context-switch and cache flush, which we might
	 * otherwise skip. See PR port-arm/38950.
	 */
	if (l->l_proc->p_sflag & PS_WEXIT)
		pmap_previous_active_lwp = NULL;

	l->l_proc->p_vmspace->vm_map.pmap->pm_activated = false;
}

void
pmap_update(pmap_t pm)
{

	if (pm->pm_remove_all) {
		/*
		 * Finish up the pmap_remove_all() optimisation by flushing
		 * the TLB.
		 */
		pmap_tlb_flushID(pm);
		pm->pm_remove_all = false;
	}

	if (pmap_is_current(pm)) {
		/*
		 * If we're dealing with a current userland pmap, move its L1
		 * to the end of the LRU.
		 */
		if (pm != pmap_kernel())
			pmap_use_l1(pm);

		/*
		 * We can assume we're done with frobbing the cache/tlb for
		 * now. Make sure any future pmap ops don't skip cache/tlb
		 * flushes.
		 */
		pm->pm_cstate.cs_all = PMAP_CACHE_STATE_ALL;
	}

	PMAPCOUNT(updates);

	/*
	 * make sure TLB/cache operations have completed.
	 */
	cpu_cpwait();
}

void
pmap_remove_all(pmap_t pm)
{

	/*
	 * The vmspace described by this pmap is about to be torn down.
	 * Until pmap_update() is called, UVM will only make calls
	 * to pmap_remove(). We can make life much simpler by flushing
	 * the cache now, and deferring TLB invalidation to pmap_update().
	 */
#ifdef PMAP_CACHE_VIVT
	pmap_idcache_wbinv_all(pm);
#endif
	pm->pm_remove_all = true;
}

/*
 * Retire the given physical map from service.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(pmap_t pm)
{
	u_int count;

	if (pm == NULL)
		return;

	if (pm->pm_remove_all) {
		pmap_tlb_flushID(pm);
		pm->pm_remove_all = false;
	}

	/*
	 * Drop reference count
	 */
	mutex_enter(&pm->pm_lock);
	count = --pm->pm_obj.uo_refs;
	mutex_exit(&pm->pm_lock);
	if (count > 0) {
		if (pmap_is_current(pm)) {
			if (pm != pmap_kernel())
				pmap_use_l1(pm);
			pm->pm_cstate.cs_all = PMAP_CACHE_STATE_ALL;
		}
		return;
	}

	/*
	 * reference count is zero, free pmap resources and then free pmap.
	 */

	if (vector_page < KERNEL_BASE) {
		KDASSERT(!pmap_is_current(pm));

		/* Remove the vector page mapping */
		pmap_remove(pm, vector_page, vector_page + PAGE_SIZE);
		pmap_update(pm);
	}

	LIST_REMOVE(pm, pm_list);

	pmap_free_l1(pm);

	if (pmap_recent_user == pm)
		pmap_recent_user = NULL;

	UVM_OBJ_DESTROY(&pm->pm_obj);

	/* return the pmap to the pool */
	pool_cache_put(&pmap_cache, pm);
}


/*
 * void pmap_reference(pmap_t pm)
 *
 * Add a reference to the specified pmap.
 */
void
pmap_reference(pmap_t pm)
{

	if (pm == NULL)
		return;

	pmap_use_l1(pm);

	mutex_enter(&pm->pm_lock);
	pm->pm_obj.uo_refs++;
	mutex_exit(&pm->pm_lock);
}

#if (ARM_MMU_V6 + ARM_MMU_V7) > 0

static struct evcnt pmap_prefer_nochange_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap prefer", "nochange");
static struct evcnt pmap_prefer_change_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap prefer", "change");

EVCNT_ATTACH_STATIC(pmap_prefer_change_ev);
EVCNT_ATTACH_STATIC(pmap_prefer_nochange_ev);

void
pmap_prefer(vaddr_t hint, vaddr_t *vap, int td)
{
	vsize_t mask = arm_cache_prefer_mask | (PAGE_SIZE - 1);
	vaddr_t va = *vap;
	vaddr_t diff = (hint - va) & mask;
	if (diff == 0) {
		pmap_prefer_nochange_ev.ev_count++;
	} else {
		pmap_prefer_change_ev.ev_count++;
		if (__predict_false(td))
			va -= mask + 1;
		*vap = va + diff;
	}
}
#endif /* ARM_MMU_V6 | ARM_MMU_V7 */

/*
 * pmap_zero_page()
 * 
 * Zero a given physical page by mapping it at a page hook point.
 * In doing the zero page op, the page we zero is mapped cachable, as with
 * StrongARM accesses to non-cached pages are non-burst making writing
 * _any_ bulk data very slow.
 */
#if (ARM_MMU_GENERIC + ARM_MMU_SA1 + ARM_MMU_V6 + ARM_MMU_V7) != 0
void
pmap_zero_page_generic(paddr_t phys)
{
#if defined(PMAP_CACHE_VIPT) || defined(DEBUG)
	struct vm_page *pg = PHYS_TO_VM_PAGE(phys);
	struct vm_page_md *md = VM_PAGE_TO_MD(pg);
#endif
#ifdef PMAP_CACHE_VIPT
	/* Choose the last page color it had, if any */
	const vsize_t va_offset = md->pvh_attrs & arm_cache_prefer_mask;
#else
	const vsize_t va_offset = 0;
#endif
	pt_entry_t * const ptep = &cdst_pte[va_offset >> PGSHIFT];

#ifdef DEBUG
	if (!SLIST_EMPTY(&md->pvh_list))
		panic("pmap_zero_page: page has mappings");
#endif

	KDASSERT((phys & PGOFSET) == 0);

	/*
	 * Hook in the page, zero it, and purge the cache for that
	 * zeroed page. Invalidate the TLB as needed.
	 */
	*ptep = L2_S_PROTO | phys |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) | pte_l2_s_cache_mode;
	PTE_SYNC(ptep);
	cpu_tlb_flushD_SE(cdstp + va_offset);
	cpu_cpwait();
	bzero_page(cdstp + va_offset);
	/*
	 * Unmap the page.
	 */
	*ptep = 0;
	PTE_SYNC(ptep);
	cpu_tlb_flushD_SE(cdstp + va_offset);
#ifdef PMAP_CACHE_VIVT
	cpu_dcache_wbinv_range(cdstp + va_offset, PAGE_SIZE);
#endif
#ifdef PMAP_CACHE_VIPT
	/*
	 * This page is now cache resident so it now has a page color.
	 * Any contents have been obliterated so clear the EXEC flag.
	 */
	if (!pmap_is_page_colored_p(md)) {
		PMAPCOUNT(vac_color_new);
		md->pvh_attrs |= PVF_COLORED;
	}
	if (PV_IS_EXEC_P(md->pvh_attrs)) {
		md->pvh_attrs &= ~PVF_EXEC;
		PMAPCOUNT(exec_discarded_zero);
	}
	md->pvh_attrs |= PVF_DIRTY;
#endif
}
#endif /* (ARM_MMU_GENERIC + ARM_MMU_SA1 + ARM_MMU_V6) != 0 */

#if ARM_MMU_XSCALE == 1
void
pmap_zero_page_xscale(paddr_t phys)
{
#ifdef DEBUG
	struct vm_page *pg = PHYS_TO_VM_PAGE(phys);
	struct vm_page_md *md = VM_PAGE_TO_MD(pg);

	if (!SLIST_EMPTY(&md->pvh_list))
		panic("pmap_zero_page: page has mappings");
#endif

	KDASSERT((phys & PGOFSET) == 0);

	/*
	 * Hook in the page, zero it, and purge the cache for that
	 * zeroed page. Invalidate the TLB as needed.
	 */
	*cdst_pte = L2_S_PROTO | phys |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) |
	    L2_C | L2_XS_T_TEX(TEX_XSCALE_X);	/* mini-data */
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
bool
pmap_pageidlezero(paddr_t phys)
{
	unsigned int i;
	int *ptr;
	bool rv = true;
#if defined(PMAP_CACHE_VIPT) || defined(DEBUG)
	struct vm_page * const pg = PHYS_TO_VM_PAGE(phys);
	struct vm_page_md *md = VM_PAGE_TO_MD(pg);
#endif
#ifdef PMAP_CACHE_VIPT
	/* Choose the last page color it had, if any */
	const vsize_t va_offset = md->pvh_attrs & arm_cache_prefer_mask;
#else
	const vsize_t va_offset = 0;
#endif
	pt_entry_t * const ptep = &csrc_pte[va_offset >> PGSHIFT];


#ifdef DEBUG
	if (!SLIST_EMPTY(&md->pvh_list))
		panic("pmap_pageidlezero: page has mappings");
#endif

	KDASSERT((phys & PGOFSET) == 0);

	/*
	 * Hook in the page, zero it, and purge the cache for that
	 * zeroed page. Invalidate the TLB as needed.
	 */
	*ptep = L2_S_PROTO | phys |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) | pte_l2_s_cache_mode;
	PTE_SYNC(ptep);
	cpu_tlb_flushD_SE(cdstp + va_offset);
	cpu_cpwait();

	for (i = 0, ptr = (int *)(cdstp + va_offset);
			i < (PAGE_SIZE / sizeof(int)); i++) {
		if (sched_curcpu_runnable_p() != 0) {
			/*
			 * A process has become ready.  Abort now,
			 * so we don't keep it waiting while we
			 * do slow memory access to finish this
			 * page.
			 */
			rv = false;
			break;
		}
		*ptr++ = 0;
	}

#ifdef PMAP_CACHE_VIVT
	if (rv)
		/* 
		 * if we aborted we'll rezero this page again later so don't
		 * purge it unless we finished it
		 */
		cpu_dcache_wbinv_range(cdstp, PAGE_SIZE);
#elif defined(PMAP_CACHE_VIPT)
	/*
	 * This page is now cache resident so it now has a page color.
	 * Any contents have been obliterated so clear the EXEC flag.
	 */
	if (!pmap_is_page_colored_p(md)) {
		PMAPCOUNT(vac_color_new);
		md->pvh_attrs |= PVF_COLORED;
	}
	if (PV_IS_EXEC_P(md->pvh_attrs)) {
		md->pvh_attrs &= ~PVF_EXEC;
		PMAPCOUNT(exec_discarded_zero);
	}
#endif
	/*
	 * Unmap the page.
	 */
	*ptep = 0;
	PTE_SYNC(ptep);
	cpu_tlb_flushD_SE(cdstp + va_offset);

	return (rv);
}
 
/*
 * pmap_copy_page()
 *
 * Copy one physical page into another, by mapping the pages into
 * hook points. The same comment regarding cachability as in
 * pmap_zero_page also applies here.
 */
#if (ARM_MMU_GENERIC + ARM_MMU_SA1 + ARM_MMU_V6 + ARM_MMU_V7) != 0
void
pmap_copy_page_generic(paddr_t src, paddr_t dst)
{
	struct vm_page * const src_pg = PHYS_TO_VM_PAGE(src);
	struct vm_page_md *src_md = VM_PAGE_TO_MD(src_pg);
#if defined(PMAP_CACHE_VIPT) || defined(DEBUG)
	struct vm_page * const dst_pg = PHYS_TO_VM_PAGE(dst);
	struct vm_page_md *dst_md = VM_PAGE_TO_MD(dst_pg);
#endif
#ifdef PMAP_CACHE_VIPT
	const vsize_t src_va_offset = src_md->pvh_attrs & arm_cache_prefer_mask;
	const vsize_t dst_va_offset = dst_md->pvh_attrs & arm_cache_prefer_mask;
#else
	const vsize_t src_va_offset = 0;
	const vsize_t dst_va_offset = 0;
#endif
	pt_entry_t * const src_ptep = &csrc_pte[src_va_offset >> PGSHIFT];
	pt_entry_t * const dst_ptep = &cdst_pte[dst_va_offset >> PGSHIFT];

#ifdef DEBUG
	if (!SLIST_EMPTY(&dst_md->pvh_list))
		panic("pmap_copy_page: dst page has mappings");
#endif

#ifdef PMAP_CACHE_VIPT
	KASSERT(arm_cache_prefer_mask == 0 || src_md->pvh_attrs & (PVF_COLORED|PVF_NC));
#endif
	KDASSERT((src & PGOFSET) == 0);
	KDASSERT((dst & PGOFSET) == 0);

	/*
	 * Clean the source page.  Hold the source page's lock for
	 * the duration of the copy so that no other mappings can
	 * be created while we have a potentially aliased mapping.
	 */
	simple_lock(&src_md->pvh_slock);
#ifdef PMAP_CACHE_VIVT
	(void) pmap_clean_page(SLIST_FIRST(&src_md->pvh_list), true);
#endif

	/*
	 * Map the pages into the page hook points, copy them, and purge
	 * the cache for the appropriate page. Invalidate the TLB
	 * as required.
	 */
	*src_ptep = L2_S_PROTO
	    | src
#ifdef PMAP_CACHE_VIPT
	    | ((src_md->pvh_attrs & PVF_NC) ? 0 : pte_l2_s_cache_mode)
#endif
#ifdef PMAP_CACHE_VIVT
	    | pte_l2_s_cache_mode
#endif
	    | L2_S_PROT(PTE_KERNEL, VM_PROT_READ);
	*dst_ptep = L2_S_PROTO | dst |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) | pte_l2_s_cache_mode;
	PTE_SYNC(src_ptep);
	PTE_SYNC(dst_ptep);
	cpu_tlb_flushD_SE(csrcp + src_va_offset);
	cpu_tlb_flushD_SE(cdstp + dst_va_offset);
	cpu_cpwait();
	bcopy_page(csrcp + src_va_offset, cdstp + dst_va_offset);
#ifdef PMAP_CACHE_VIVT
	cpu_dcache_inv_range(csrcp + src_va_offset, PAGE_SIZE);
#endif
	simple_unlock(&src_md->pvh_slock); /* cache is safe again */
#ifdef PMAP_CACHE_VIVT
	cpu_dcache_wbinv_range(cdstp + dst_va_offset, PAGE_SIZE);
#endif
	/*
	 * Unmap the pages.
	 */
	*src_ptep = 0;
	*dst_ptep = 0;
	PTE_SYNC(src_ptep);
	PTE_SYNC(dst_ptep);
	cpu_tlb_flushD_SE(csrcp + src_va_offset);
	cpu_tlb_flushD_SE(cdstp + dst_va_offset);
#ifdef PMAP_CACHE_VIPT
	/*
	 * Now that the destination page is in the cache, mark it as colored.
	 * If this was an exec page, discard it.
	 */
	if (!pmap_is_page_colored_p(dst_md)) {
		PMAPCOUNT(vac_color_new);
		dst_md->pvh_attrs |= PVF_COLORED;
	}
	if (PV_IS_EXEC_P(dst_md->pvh_attrs)) {
		dst_md->pvh_attrs &= ~PVF_EXEC;
		PMAPCOUNT(exec_discarded_copy);
	}
	dst_md->pvh_attrs |= PVF_DIRTY;
#endif
}
#endif /* (ARM_MMU_GENERIC + ARM_MMU_SA1 + ARM_MMU_V6) != 0 */

#if ARM_MMU_XSCALE == 1
void
pmap_copy_page_xscale(paddr_t src, paddr_t dst)
{
	struct vm_page_md *src_md = VM_PAGE_TO_MD(PHYS_TO_VM_PAGE(src));
#ifdef DEBUG
	struct vm_page_md *dst_md = VM_PAGE_TO_MD(PHYS_TO_VM_PAGE(dst));

	if (!SLIST_EMPTY(&dst_md->pvh_list))
		panic("pmap_copy_page: dst page has mappings");
#endif

	KDASSERT((src & PGOFSET) == 0);
	KDASSERT((dst & PGOFSET) == 0);

	/*
	 * Clean the source page.  Hold the source page's lock for
	 * the duration of the copy so that no other mappings can
	 * be created while we have a potentially aliased mapping.
	 */
	simple_lock(&src_md->pvh_slock);
#ifdef PMAP_CACHE_VIVT
	(void) pmap_clean_page(SLIST_FIRST(&src_md->pvh_list), true);
#endif

	/*
	 * Map the pages into the page hook points, copy them, and purge
	 * the cache for the appropriate page. Invalidate the TLB
	 * as required.
	 */
	*csrc_pte = L2_S_PROTO | src |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_READ) |
	    L2_C | L2_XS_T_TEX(TEX_XSCALE_X);	/* mini-data */
	PTE_SYNC(csrc_pte);
	*cdst_pte = L2_S_PROTO | dst |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_WRITE) |
	    L2_C | L2_XS_T_TEX(TEX_XSCALE_X);	/* mini-data */
	PTE_SYNC(cdst_pte);
	cpu_tlb_flushD_SE(csrcp);
	cpu_tlb_flushD_SE(cdstp);
	cpu_cpwait();
	bcopy_page(csrcp, cdstp);
	simple_unlock(&src_md->pvh_slock); /* cache is safe again */
	xscale_cache_clean_minidata();
}
#endif /* ARM_MMU_XSCALE == 1 */

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
 * Helper function for pmap_grow_l2_bucket()
 */
static inline int
pmap_grow_map(vaddr_t va, pt_entry_t cache_mode, paddr_t *pap)
{
	struct l2_bucket *l2b;
	pt_entry_t *ptep;
	paddr_t pa;

	if (uvm.page_init_done == false) {
#ifdef PMAP_STEAL_MEMORY
		pv_addr_t pv;
		pmap_boot_pagealloc(PAGE_SIZE,
#ifdef PMAP_CACHE_VIPT
		    arm_cache_prefer_mask,
		    va & arm_cache_prefer_mask,
#else
		    0, 0,
#endif
		    &pv);
		pa = pv.pv_pa;
#else
		if (uvm_page_physget(&pa) == false)
			return (1);
#endif	/* PMAP_STEAL_MEMORY */
	} else {
		struct vm_page *pg;
		pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE);
		if (pg == NULL)
			return (1);
		pa = VM_PAGE_TO_PHYS(pg);
#ifdef PMAP_CACHE_VIPT
#ifdef DIAGNOSTIC
		struct vm_page_md *md = VM_PAGE_TO_MD(pg);
#endif
		/*
		 * This new page must not have any mappings.  Enter it via
		 * pmap_kenter_pa and let that routine do the hard work.
		 */
		KASSERT(SLIST_EMPTY(&md->pvh_list));
		pmap_kenter_pa(va, pa,
		    VM_PROT_READ|VM_PROT_WRITE, PMAP_KMPAGE);
#endif
	}

	if (pap)
		*pap = pa;

	PMAPCOUNT(pt_mappings);
	l2b = pmap_get_l2_bucket(pmap_kernel(), va);
	KDASSERT(l2b != NULL);

	ptep = &l2b->l2b_kva[l2pte_index(va)];
	*ptep = L2_S_PROTO | pa | cache_mode |
	    L2_S_PROT(PTE_KERNEL, VM_PROT_READ | VM_PROT_WRITE);
	PTE_SYNC(ptep);
	memset((void *)va, 0, PAGE_SIZE);
	return (0);
}

/*
 * This is the same as pmap_alloc_l2_bucket(), except that it is only
 * used by pmap_growkernel().
 */
static inline struct l2_bucket *
pmap_grow_l2_bucket(pmap_t pm, vaddr_t va)
{
	struct l2_dtable *l2;
	struct l2_bucket *l2b;
	u_short l1idx;
	vaddr_t nva;

	l1idx = L1_IDX(va);

	if ((l2 = pm->pm_l2[L2_IDX(l1idx)]) == NULL) {
		/*
		 * No mapping at this address, as there is
		 * no entry in the L1 table.
		 * Need to allocate a new l2_dtable.
		 */
		nva = pmap_kernel_l2dtable_kva;
		if ((nva & PGOFSET) == 0) {
			/*
			 * Need to allocate a backing page
			 */
			if (pmap_grow_map(nva, pte_l2_s_cache_mode, NULL))
				return (NULL);
		}

		l2 = (struct l2_dtable *)nva;
		nva += sizeof(struct l2_dtable);

		if ((nva & PGOFSET) < (pmap_kernel_l2dtable_kva & PGOFSET)) {
			/*
			 * The new l2_dtable straddles a page boundary.
			 * Map in another page to cover it.
			 */
			if (pmap_grow_map(nva, pte_l2_s_cache_mode, NULL))
				return (NULL);
		}

		pmap_kernel_l2dtable_kva = nva;

		/*
		 * Link it into the parent pmap
		 */
		pm->pm_l2[L2_IDX(l1idx)] = l2;
	}

	l2b = &l2->l2_bucket[L2_BUCKET(l1idx)];

	/*
	 * Fetch pointer to the L2 page table associated with the address.
	 */
	if (l2b->l2b_kva == NULL) {
		pt_entry_t *ptep;

		/*
		 * No L2 page table has been allocated. Chances are, this
		 * is because we just allocated the l2_dtable, above.
		 */
		nva = pmap_kernel_l2ptp_kva;
		ptep = (pt_entry_t *)nva;
		if ((nva & PGOFSET) == 0) {
			/*
			 * Need to allocate a backing page
			 */
			if (pmap_grow_map(nva, pte_l2_s_cache_mode_pt,
			    &pmap_kernel_l2ptp_phys))
				return (NULL);
			PTE_SYNC_RANGE(ptep, PAGE_SIZE / sizeof(pt_entry_t));
		}

		l2->l2_occupancy++;
		l2b->l2b_kva = ptep;
		l2b->l2b_l1idx = l1idx;
		l2b->l2b_phys = pmap_kernel_l2ptp_phys;

		pmap_kernel_l2ptp_kva += L2_TABLE_SIZE_REAL;
		pmap_kernel_l2ptp_phys += L2_TABLE_SIZE_REAL;
	}

	return (l2b);
}

vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	pmap_t kpm = pmap_kernel();
	struct l1_ttable *l1;
	struct l2_bucket *l2b;
	pd_entry_t *pl1pd;
	int s;

	if (maxkvaddr <= pmap_curmaxkvaddr)
		goto out;		/* we are OK */

	NPDEBUG(PDB_GROWKERN,
	    printf("pmap_growkernel: growing kernel from 0x%lx to 0x%lx\n",
	    pmap_curmaxkvaddr, maxkvaddr));

	KDASSERT(maxkvaddr <= virtual_end);

	/*
	 * whoops!   we need to add kernel PTPs
	 */

	s = splhigh();	/* to be safe */
	mutex_enter(&kpm->pm_lock);

	/* Map 1MB at a time */
	for (; pmap_curmaxkvaddr < maxkvaddr; pmap_curmaxkvaddr += L1_S_SIZE) {

		l2b = pmap_grow_l2_bucket(kpm, pmap_curmaxkvaddr);
		KDASSERT(l2b != NULL);

		/* Distribute new L1 entry to all other L1s */
		SLIST_FOREACH(l1, &l1_list, l1_link) {
			pl1pd = &l1->l1_kva[L1_IDX(pmap_curmaxkvaddr)];
			*pl1pd = l2b->l2b_phys | L1_C_DOM(PMAP_DOMAIN_KERNEL) |
			    L1_C_PROTO;
			PTE_SYNC(pl1pd);
		}
	}

	/*
	 * flush out the cache, expensive but growkernel will happen so
	 * rarely
	 */
	cpu_dcache_wbinv_all();
	cpu_tlb_flushD();
	cpu_cpwait();

	mutex_exit(&kpm->pm_lock);
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
	struct l2_bucket *l2b;
	pt_entry_t *ptep;

	l2b = pmap_get_l2_bucket(pmap_kernel(), vector_page);
	KDASSERT(l2b != NULL);

	ptep = &l2b->l2b_kva[l2pte_index(vector_page)];

	*ptep = (*ptep & ~L1_S_PROT_MASK) | L2_S_PROT(PTE_KERNEL, prot);
	PTE_SYNC(ptep);
	cpu_tlb_flushD_SE(vector_page);
	cpu_cpwait();
}

/*
 * Fetch pointers to the PDE/PTE for the given pmap/VA pair.
 * Returns true if the mapping exists, else false.
 *
 * NOTE: This function is only used by a couple of arm-specific modules.
 * It is not safe to take any pmap locks here, since we could be right
 * in the middle of debugging the pmap anyway...
 *
 * It is possible for this routine to return false even though a valid
 * mapping does exist. This is because we don't lock, so the metadata
 * state may be inconsistent.
 *
 * NOTE: We can return a NULL *ptp in the case where the L1 pde is
 * a "section" mapping.
 */
bool
pmap_get_pde_pte(pmap_t pm, vaddr_t va, pd_entry_t **pdp, pt_entry_t **ptp)
{
	struct l2_dtable *l2;
	pd_entry_t *pl1pd, l1pd;
	pt_entry_t *ptep;
	u_short l1idx;

	if (pm->pm_l1 == NULL)
		return false;

	l1idx = L1_IDX(va);
	*pdp = pl1pd = &pm->pm_l1->l1_kva[l1idx];
	l1pd = *pl1pd;

	if (l1pte_section_p(l1pd)) {
		*ptp = NULL;
		return true;
	}

	if (pm->pm_l2 == NULL)
		return false;

	l2 = pm->pm_l2[L2_IDX(l1idx)];

	if (l2 == NULL ||
	    (ptep = l2->l2_bucket[L2_BUCKET(l1idx)].l2b_kva) == NULL) {
		return false;
	}

	*ptp = &ptep[l2pte_index(va)];
	return true;
}

bool
pmap_get_pde(pmap_t pm, vaddr_t va, pd_entry_t **pdp)
{
	u_short l1idx;

	if (pm->pm_l1 == NULL)
		return false;

	l1idx = L1_IDX(va);
	*pdp = &pm->pm_l1->l1_kva[l1idx];

	return true;
}

/************************ Bootstrapping routines ****************************/

static void
pmap_init_l1(struct l1_ttable *l1, pd_entry_t *l1pt)
{
	int i;

	l1->l1_kva = l1pt;
	l1->l1_domain_use_count = 0;
	l1->l1_domain_first = 0;

	for (i = 0; i < PMAP_DOMAINS; i++)
		l1->l1_domain_free[i] = i + 1;

	/*
	 * Copy the kernel's L1 entries to each new L1.
	 */
	if (pmap_initialized)
		memcpy(l1pt, pmap_kernel()->pm_l1->l1_kva, L1_TABLE_SIZE);

	if (pmap_extract(pmap_kernel(), (vaddr_t)l1pt,
	    &l1->l1_physaddr) == false)
		panic("pmap_init_l1: can't get PA of L1 at %p", l1pt);

	SLIST_INSERT_HEAD(&l1_list, l1, l1_link);
	TAILQ_INSERT_TAIL(&l1_lru_list, l1, l1_lru);
}

/*
 * pmap_bootstrap() is called from the board-specific initarm() routine
 * once the kernel L1/L2 descriptors tables have been set up.
 *
 * This is a somewhat convoluted process since pmap bootstrap is, effectively,
 * spread over a number of disparate files/functions.
 *
 * We are passed the following parameters
 *  - kernel_l1pt
 *    This is a pointer to the base of the kernel's L1 translation table.
 *  - vstart
 *    1MB-aligned start of managed kernel virtual memory.
 *  - vend
 *    1MB-aligned end of managed kernel virtual memory.
 *
 * We use the first parameter to build the metadata (struct l1_ttable and
 * struct l2_dtable) necessary to track kernel mappings.
 */
#define	PMAP_STATIC_L2_SIZE 16
void
pmap_bootstrap(vaddr_t vstart, vaddr_t vend)
{
	static struct l1_ttable static_l1;
	static struct l2_dtable static_l2[PMAP_STATIC_L2_SIZE];
	struct l1_ttable *l1 = &static_l1;
	struct l2_dtable *l2;
	struct l2_bucket *l2b;
	pd_entry_t *l1pt = (pd_entry_t *) kernel_l1pt.pv_va;
	pmap_t pm = pmap_kernel();
	pd_entry_t pde;
	pt_entry_t *ptep;
	paddr_t pa;
	vaddr_t va;
	vsize_t size;
	int nptes, l1idx, l2idx, l2next = 0;

	/*
	 * Initialise the kernel pmap object
	 */
	pm->pm_l1 = l1;
	pm->pm_domain = PMAP_DOMAIN_KERNEL;
	pm->pm_activated = true;
	pm->pm_cstate.cs_all = PMAP_CACHE_STATE_ALL;
	UVM_OBJ_INIT(&pm->pm_obj, NULL, 1);

	/*
	 * Scan the L1 translation table created by initarm() and create
	 * the required metadata for all valid mappings found in it.
	 */
	for (l1idx = 0; l1idx < (L1_TABLE_SIZE / sizeof(pd_entry_t)); l1idx++) {
		pde = l1pt[l1idx];

		/*
		 * We're only interested in Coarse mappings.
		 * pmap_extract() can deal with section mappings without
		 * recourse to checking L2 metadata.
		 */
		if ((pde & L1_TYPE_MASK) != L1_TYPE_C)
			continue;

		/*
		 * Lookup the KVA of this L2 descriptor table
		 */
		pa = (paddr_t)(pde & L1_C_ADDR_MASK);
		ptep = (pt_entry_t *)kernel_pt_lookup(pa);
		if (ptep == NULL) {
			panic("pmap_bootstrap: No L2 for va 0x%x, pa 0x%lx",
			    (u_int)l1idx << L1_S_SHIFT, pa);
		}

		/*
		 * Fetch the associated L2 metadata structure.
		 * Allocate a new one if necessary.
		 */
		if ((l2 = pm->pm_l2[L2_IDX(l1idx)]) == NULL) {
			if (l2next == PMAP_STATIC_L2_SIZE)
				panic("pmap_bootstrap: out of static L2s");
			pm->pm_l2[L2_IDX(l1idx)] = l2 = &static_l2[l2next++];
		}

		/*
		 * One more L1 slot tracked...
		 */
		l2->l2_occupancy++;

		/*
		 * Fill in the details of the L2 descriptor in the
		 * appropriate bucket.
		 */
		l2b = &l2->l2_bucket[L2_BUCKET(l1idx)];
		l2b->l2b_kva = ptep;
		l2b->l2b_phys = pa;
		l2b->l2b_l1idx = l1idx;

		/*
		 * Establish an initial occupancy count for this descriptor
		 */
		for (l2idx = 0;
		    l2idx < (L2_TABLE_SIZE_REAL / sizeof(pt_entry_t));
		    l2idx++) {
			if ((ptep[l2idx] & L2_TYPE_MASK) != L2_TYPE_INV) {
				l2b->l2b_occupancy++;
			}
		}

		/*
		 * Make sure the descriptor itself has the correct cache mode.
		 * If not, fix it, but whine about the problem. Port-meisters
		 * should consider this a clue to fix up their initarm()
		 * function. :)
		 */
		if (pmap_set_pt_cache_mode(l1pt, (vaddr_t)ptep)) {
			printf("pmap_bootstrap: WARNING! wrong cache mode for "
			    "L2 pte @ %p\n", ptep);
		}
	}

	/*
	 * Ensure the primary (kernel) L1 has the correct cache mode for
	 * a page table. Bitch if it is not correctly set.
	 */
	for (va = (vaddr_t)l1pt;
	    va < ((vaddr_t)l1pt + L1_TABLE_SIZE); va += PAGE_SIZE) {
		if (pmap_set_pt_cache_mode(l1pt, va))
			printf("pmap_bootstrap: WARNING! wrong cache mode for "
			    "primary L1 @ 0x%lx\n", va);
	}

	cpu_dcache_wbinv_all();
	cpu_tlb_flushID();
	cpu_cpwait();

	/*
	 * now we allocate the "special" VAs which are used for tmp mappings
	 * by the pmap (and other modules).  we allocate the VAs by advancing
	 * virtual_avail (note that there are no pages mapped at these VAs).
	 *
	 * Managed KVM space start from wherever initarm() tells us.
	 */
	virtual_avail = vstart;
	virtual_end = vend;

#ifdef PMAP_CACHE_VIPT
	/*
	 * If we have a VIPT cache, we need one page/pte per possible alias
	 * page so we won't violate cache aliasing rules.
	 */
	virtual_avail = (virtual_avail + arm_cache_prefer_mask) & ~arm_cache_prefer_mask; 
	nptes = (arm_cache_prefer_mask >> PGSHIFT) + 1;
#else
	nptes = 1;
#endif
	pmap_alloc_specials(&virtual_avail, nptes, &csrcp, &csrc_pte);
	pmap_set_pt_cache_mode(l1pt, (vaddr_t)csrc_pte);
	pmap_alloc_specials(&virtual_avail, nptes, &cdstp, &cdst_pte);
	pmap_set_pt_cache_mode(l1pt, (vaddr_t)cdst_pte);
	pmap_alloc_specials(&virtual_avail, nptes, &memhook, NULL);
	pmap_alloc_specials(&virtual_avail, round_page(MSGBUFSIZE) / PAGE_SIZE,
	    (void *)&msgbufaddr, NULL);

	/*
	 * Allocate a range of kernel virtual address space to be used
	 * for L2 descriptor tables and metadata allocation in
	 * pmap_growkernel().
	 */
	size = ((virtual_end - pmap_curmaxkvaddr) + L1_S_OFFSET) / L1_S_SIZE;
	pmap_alloc_specials(&virtual_avail,
	    round_page(size * L2_TABLE_SIZE_REAL) / PAGE_SIZE,
	    &pmap_kernel_l2ptp_kva, NULL);

	size = (size + (L2_BUCKET_SIZE - 1)) / L2_BUCKET_SIZE;
	pmap_alloc_specials(&virtual_avail,
	    round_page(size * sizeof(struct l2_dtable)) / PAGE_SIZE,
	    &pmap_kernel_l2dtable_kva, NULL);

	/*
	 * init the static-global locks and global pmap list.
	 */
	/* spinlockinit(&pmap_main_lock, "pmaplk", 0); */

	/*
	 * We can now initialise the first L1's metadata.
	 */
	SLIST_INIT(&l1_list);
	TAILQ_INIT(&l1_lru_list);
	simple_lock_init(&l1_lru_lock);
	pmap_init_l1(l1, l1pt);

	/* Set up vector page L1 details, if necessary */
	if (vector_page < KERNEL_BASE) {
		pm->pm_pl1vec = &pm->pm_l1->l1_kva[L1_IDX(vector_page)];
		l2b = pmap_get_l2_bucket(pm, vector_page);
		KDASSERT(l2b != NULL);
		pm->pm_l1vec = l2b->l2b_phys | L1_C_PROTO |
		    L1_C_DOM(pm->pm_domain);
	} else
		pm->pm_pl1vec = NULL;

	/*
	 * Initialize the pmap cache
	 */
	pool_cache_bootstrap(&pmap_cache, sizeof(struct pmap), 0, 0, 0,
	    "pmappl", NULL, IPL_NONE, pmap_pmap_ctor, NULL, NULL);
	LIST_INIT(&pmap_pmaps);
	LIST_INSERT_HEAD(&pmap_pmaps, pm, pm_list);

	/*
	 * Initialize the pv pool.
	 */
	pool_init(&pmap_pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pvepl",
	    &pmap_bootstrap_pv_allocator, IPL_NONE);

	/*
	 * Initialize the L2 dtable pool and cache.
	 */
	pool_cache_bootstrap(&pmap_l2dtable_cache, sizeof(struct l2_dtable), 0,
	    0, 0, "l2dtblpl", NULL, IPL_NONE, pmap_l2dtable_ctor, NULL, NULL);

	/*
	 * Initialise the L2 descriptor table pool and cache
	 */
	pool_cache_bootstrap(&pmap_l2ptp_cache, L2_TABLE_SIZE_REAL, 0,
	    L2_TABLE_SIZE_REAL, 0, "l2ptppl", NULL, IPL_NONE,
	    pmap_l2ptp_ctor, NULL, NULL);

	cpu_dcache_wbinv_all();
}

static int
pmap_set_pt_cache_mode(pd_entry_t *kl1, vaddr_t va)
{
	pd_entry_t *pdep, pde;
	pt_entry_t *ptep, pte;
	vaddr_t pa;
	int rv = 0;

	/*
	 * Make sure the descriptor itself has the correct cache mode
	 */
	pdep = &kl1[L1_IDX(va)];
	pde = *pdep;

	if (l1pte_section_p(pde)) {
		if ((pde & L1_S_CACHE_MASK) != pte_l1_s_cache_mode_pt) {
			*pdep = (pde & ~L1_S_CACHE_MASK) |
			    pte_l1_s_cache_mode_pt;
			PTE_SYNC(pdep);
			cpu_dcache_wbinv_range((vaddr_t)pdep, sizeof(*pdep));
			rv = 1;
		}
	} else {
		pa = (paddr_t)(pde & L1_C_ADDR_MASK);
		ptep = (pt_entry_t *)kernel_pt_lookup(pa);
		if (ptep == NULL)
			panic("pmap_bootstrap: No L2 for L2 @ va %p\n", ptep);

		ptep = &ptep[l2pte_index(va)];
		pte = *ptep;
		if ((pte & L2_S_CACHE_MASK) != pte_l2_s_cache_mode_pt) {
			*ptep = (pte & ~L2_S_CACHE_MASK) |
			    pte_l2_s_cache_mode_pt;
			PTE_SYNC(ptep);
			cpu_dcache_wbinv_range((vaddr_t)ptep, sizeof(*ptep));
			rv = 1;
		}
	}

	return (rv);
}

static void
pmap_alloc_specials(vaddr_t *availp, int pages, vaddr_t *vap, pt_entry_t **ptep)
{
	vaddr_t va = *availp;
	struct l2_bucket *l2b;

	if (ptep) {
		l2b = pmap_get_l2_bucket(pmap_kernel(), va);
		if (l2b == NULL)
			panic("pmap_alloc_specials: no l2b for 0x%lx", va);

		if (ptep)
			*ptep = &l2b->l2b_kva[l2pte_index(va)];
	}

	*vap = va;
	*availp = va + (PAGE_SIZE * pages);
}

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
	avail_start = ptoa(VM_PHYSMEM_PTR(0)->start);
	avail_end = ptoa(VM_PHYSMEM_PTR(vm_nphysseg - 1)->end);

	/*
	 * Now we need to free enough pv_entry structures to allow us to get
	 * the kmem_map/kmem_object allocated and inited (done after this
	 * function is finished).  to do this we allocate one bootstrap page out
	 * of kernel_map and use it to provide an initial pool of pv_entry
	 * structures.   we never free this page.
	 */
	pool_setlowat(&pmap_pv_pool,
	    (PAGE_SIZE / sizeof(struct pv_entry)) * 2);

	mutex_init(&memlock, MUTEX_DEFAULT, IPL_NONE);
	zeropage = (void *)uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_WIRED|UVM_KMF_ZERO);

	pmap_initialized = true;
}

static vaddr_t last_bootstrap_page = 0;
static void *free_bootstrap_pages = NULL;

static void *
pmap_bootstrap_pv_page_alloc(struct pool *pp, int flags)
{
	extern void *pool_page_alloc(struct pool *, int);
	vaddr_t new_page;
	void *rv;

	if (pmap_initialized)
		return (pool_page_alloc(pp, flags));

	if (free_bootstrap_pages) {
		rv = free_bootstrap_pages;
		free_bootstrap_pages = *((void **)rv);
		return (rv);
	}

	new_page = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_WIRED | ((flags & PR_WAITOK) ? 0 : UVM_KMF_NOWAIT));

	KASSERT(new_page > last_bootstrap_page);
	last_bootstrap_page = new_page;
	return ((void *)new_page);
}

static void
pmap_bootstrap_pv_page_free(struct pool *pp, void *v)
{
	extern void pool_page_free(struct pool *, void *);

	if ((vaddr_t)v <= last_bootstrap_page) {
		*((void **)v) = free_bootstrap_pages;
		free_bootstrap_pages = v;
		return;
	}

	if (pmap_initialized) {
		pool_page_free(pp, v);
		return;
	}
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
	extern paddr_t physical_start, physical_end;
	struct l2_bucket *l2b;
	struct l1_ttable *l1;
	struct pglist plist;
	struct vm_page *m;
	pd_entry_t *pl1pt;
	pt_entry_t *ptep, pte;
	vaddr_t va, eva;
	u_int loop, needed;
	int error;

	pool_cache_setlowat(&pmap_l2ptp_cache,
	    (PAGE_SIZE / L2_TABLE_SIZE_REAL) * 4);
	pool_cache_setlowat(&pmap_l2dtable_cache,
	    (PAGE_SIZE / sizeof(struct l2_dtable)) * 2);

	needed = (maxproc / PMAP_DOMAINS) + ((maxproc % PMAP_DOMAINS) ? 1 : 0);
	needed -= 1;

	l1 = malloc(sizeof(*l1) * needed, M_VMPMAP, M_WAITOK);

	for (loop = 0; loop < needed; loop++, l1++) {
		/* Allocate a L1 page table */
		va = uvm_km_alloc(kernel_map, L1_TABLE_SIZE, 0, UVM_KMF_VAONLY);
		if (va == 0)
			panic("Cannot allocate L1 KVM");

		error = uvm_pglistalloc(L1_TABLE_SIZE, physical_start,
		    physical_end, L1_TABLE_SIZE, 0, &plist, 1, M_WAITOK);
		if (error)
			panic("Cannot allocate L1 physical pages");

		m = TAILQ_FIRST(&plist);
		eva = va + L1_TABLE_SIZE;
		pl1pt = (pd_entry_t *)va;

		while (m && va < eva) {
			paddr_t pa = VM_PAGE_TO_PHYS(m);

			pmap_kenter_pa(va, pa,
			    VM_PROT_READ|VM_PROT_WRITE, PMAP_KMPAGE);

			/*
			 * Make sure the L1 descriptor table is mapped
			 * with the cache-mode set to write-through.
			 */
			l2b = pmap_get_l2_bucket(pmap_kernel(), va);
			KDASSERT(l2b != NULL);
			ptep = &l2b->l2b_kva[l2pte_index(va)];
			pte = *ptep;
			pte = (pte & ~L2_S_CACHE_MASK) | pte_l2_s_cache_mode_pt;
			*ptep = pte;
			PTE_SYNC(ptep);
			cpu_tlb_flushD_SE(va);

			va += PAGE_SIZE;
			m = TAILQ_NEXT(m, pageq.queue);
		}

#ifdef DIAGNOSTIC
		if (m)
			panic("pmap_alloc_l1pt: pglist not empty");
#endif	/* DIAGNOSTIC */

		pmap_init_l1(l1, pl1pt);
	}

#ifdef DEBUG
	printf("pmap_postinit: Allocated %d static L1 descriptor tables\n",
	    needed);
#endif
}

/*
 * Note that the following routines are used by board-specific initialisation
 * code to configure the initial kernel page tables.
 *
 * If ARM32_NEW_VM_LAYOUT is *not* defined, they operate on the assumption that
 * L2 page-table pages are 4KB in size and use 4 L1 slots. This mimics the
 * behaviour of the old pmap, and provides an easy migration path for
 * initial bring-up of the new pmap on existing ports. Fortunately,
 * pmap_bootstrap() compensates for this hackery. This is only a stop-gap and
 * will be deprecated.
 *
 * If ARM32_NEW_VM_LAYOUT *is* defined, these functions deal with 1KB L2 page
 * tables.
 */

/*
 * This list exists for the benefit of pmap_map_chunk().  It keeps track
 * of the kernel L2 tables during bootstrap, so that pmap_map_chunk() can
 * find them as necessary.
 *
 * Note that the data on this list MUST remain valid after initarm() returns,
 * as pmap_bootstrap() uses it to contruct L2 table metadata.
 */
SLIST_HEAD(, pv_addr) kernel_pt_list = SLIST_HEAD_INITIALIZER(kernel_pt_list);

static vaddr_t
kernel_pt_lookup(paddr_t pa)
{
	pv_addr_t *pv;

	SLIST_FOREACH(pv, &kernel_pt_list, pv_list) {
#ifndef ARM32_NEW_VM_LAYOUT
		if (pv->pv_pa == (pa & ~PGOFSET))
			return (pv->pv_va | (pa & PGOFSET));
#else
		if (pv->pv_pa == pa)
			return (pv->pv_va);
#endif
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
	pd_entry_t fl;

	KASSERT(((va | pa) & L1_S_OFFSET) == 0);

	switch (cache) {
	case PTE_NOCACHE:
	default:
		fl = 0;
		break;

	case PTE_CACHE:
		fl = pte_l1_s_cache_mode;
		break;

	case PTE_PAGETABLE:
		fl = pte_l1_s_cache_mode_pt;
		break;
	}

	pde[va >> L1_S_SHIFT] = L1_S_PROTO | pa |
	    L1_S_PROT(PTE_KERNEL, prot) | fl | L1_S_DOM(PMAP_DOMAIN_KERNEL);
	PTE_SYNC(&pde[va >> L1_S_SHIFT]);
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
	pt_entry_t fl;
	pt_entry_t *pte;

	KASSERT(((va | pa) & PGOFSET) == 0);

	switch (cache) {
	case PTE_NOCACHE:
	default:
		fl = 0;
		break;

	case PTE_CACHE:
		fl = pte_l2_s_cache_mode;
		break;

	case PTE_PAGETABLE:
		fl = pte_l2_s_cache_mode_pt;
		break;
	}

	if ((pde[va >> L1_S_SHIFT] & L1_TYPE_MASK) != L1_TYPE_C)
		panic("pmap_map_entry: no L2 table for VA 0x%08lx", va);

#ifndef ARM32_NEW_VM_LAYOUT
	pte = (pt_entry_t *)
	    kernel_pt_lookup(pde[va >> L1_S_SHIFT] & L2_S_FRAME);
#else
	pte = (pt_entry_t *) kernel_pt_lookup(pde[L1_IDX(va)] & L1_C_ADDR_MASK);
#endif
	if (pte == NULL)
		panic("pmap_map_entry: can't find L2 table for VA 0x%08lx", va);

	fl |= L2_S_PROTO | pa | L2_S_PROT(PTE_KERNEL, prot);
#ifndef ARM32_NEW_VM_LAYOUT
	pte += (va >> PGSHIFT) & 0x3ff;
#else
	pte += l2pte_index(va);
	    L2_S_PROTO | pa | L2_S_PROT(PTE_KERNEL, prot) | fl;
#endif
	*pte = fl;
	PTE_SYNC(pte);
}

/*
 * pmap_link_l2pt:
 *
 *	Link the L2 page table specified by "l2pv" into the L1
 *	page table at the slot for "va".
 */
void
pmap_link_l2pt(vaddr_t l1pt, vaddr_t va, pv_addr_t *l2pv)
{
	pd_entry_t *pde = (pd_entry_t *) l1pt, proto;
	u_int slot = va >> L1_S_SHIFT;

#ifndef ARM32_NEW_VM_LAYOUT
	KASSERT((va & ((L1_S_SIZE * 4) - 1)) == 0);
	KASSERT((l2pv->pv_pa & PGOFSET) == 0);
#endif

	proto = L1_S_DOM(PMAP_DOMAIN_KERNEL) | L1_C_PROTO;

	pde[slot + 0] = proto | (l2pv->pv_pa + 0x000);
#ifdef ARM32_NEW_VM_LAYOUT
	PTE_SYNC(&pde[slot]);
#else
	pde[slot + 1] = proto | (l2pv->pv_pa + 0x400);
	pde[slot + 2] = proto | (l2pv->pv_pa + 0x800);
	pde[slot + 3] = proto | (l2pv->pv_pa + 0xc00);
	PTE_SYNC_RANGE(&pde[slot + 0], 4);
#endif

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
	pt_entry_t *pte, f1, f2s, f2l;
	vsize_t resid;  
	int i;

	resid = (size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);

	if (l1pt == 0)
		panic("pmap_map_chunk: no L1 table provided");

#ifdef VERBOSE_INIT_ARM     
	printf("pmap_map_chunk: pa=0x%lx va=0x%lx size=0x%lx resid=0x%lx "
	    "prot=0x%x cache=%d\n", pa, va, size, resid, prot, cache);
#endif

	switch (cache) {
	case PTE_NOCACHE:
	default:
		f1 = 0;
		f2l = 0;
		f2s = 0;
		break;

	case PTE_CACHE:
		f1 = pte_l1_s_cache_mode;
		f2l = pte_l2_l_cache_mode;
		f2s = pte_l2_s_cache_mode;
		break;

	case PTE_PAGETABLE:
		f1 = pte_l1_s_cache_mode_pt;
		f2l = pte_l2_l_cache_mode_pt;
		f2s = pte_l2_s_cache_mode_pt;
		break;
	}

	size = resid;

	while (resid > 0) {
		/* See if we can use a section mapping. */
		if (L1_S_MAPPABLE_P(va, pa, resid)) {
#ifdef VERBOSE_INIT_ARM
			printf("S");
#endif
			pde[va >> L1_S_SHIFT] = L1_S_PROTO | pa |
			    L1_S_PROT(PTE_KERNEL, prot) | f1 |
			    L1_S_DOM(PMAP_DOMAIN_KERNEL);
			PTE_SYNC(&pde[va >> L1_S_SHIFT]);
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

#ifndef ARM32_NEW_VM_LAYOUT
		pte = (pt_entry_t *)
		    kernel_pt_lookup(pde[va >> L1_S_SHIFT] & L2_S_FRAME);
#else
		pte = (pt_entry_t *) kernel_pt_lookup(
		    pde[L1_IDX(va)] & L1_C_ADDR_MASK);
#endif
		if (pte == NULL)
			panic("pmap_map_chunk: can't find L2 table for VA"
			    "0x%08lx", va);

		/* See if we can use a L2 large page mapping. */
		if (L2_L_MAPPABLE_P(va, pa, resid)) {
#ifdef VERBOSE_INIT_ARM
			printf("L");
#endif
			for (i = 0; i < 16; i++) {
#ifndef ARM32_NEW_VM_LAYOUT
				pte[((va >> PGSHIFT) & 0x3f0) + i] =
				    L2_L_PROTO | pa |
				    L2_L_PROT(PTE_KERNEL, prot) | f2l;
				PTE_SYNC(&pte[((va >> PGSHIFT) & 0x3f0) + i]);
#else
				pte[l2pte_index(va) + i] =
				    L2_L_PROTO | pa |
				    L2_L_PROT(PTE_KERNEL, prot) | f2l;
				PTE_SYNC(&pte[l2pte_index(va) + i]);
#endif
			}
			va += L2_L_SIZE;
			pa += L2_L_SIZE;
			resid -= L2_L_SIZE;
			continue;
		}

		/* Use a small page mapping. */
#ifdef VERBOSE_INIT_ARM
		printf("P");
#endif
#ifndef ARM32_NEW_VM_LAYOUT
		pte[(va >> PGSHIFT) & 0x3ff] =
		    L2_S_PROTO | pa | L2_S_PROT(PTE_KERNEL, prot) | f2s;
		PTE_SYNC(&pte[(va >> PGSHIFT) & 0x3ff]);
#else
		pte[l2pte_index(va)] =
		    L2_S_PROTO | pa | L2_S_PROT(PTE_KERNEL, prot) | f2s;
		PTE_SYNC(&pte[l2pte_index(va)]);
#endif
		va += PAGE_SIZE;
		pa += PAGE_SIZE;
		resid -= PAGE_SIZE;
	}
#ifdef VERBOSE_INIT_ARM
	printf("\n");
#endif
	return (size);
}

/********************** Static device map routines ***************************/

static const struct pmap_devmap *pmap_devmap_table;

/*
 * Register the devmap table.  This is provided in case early console
 * initialization needs to register mappings created by bootstrap code
 * before pmap_devmap_bootstrap() is called.
 */
void
pmap_devmap_register(const struct pmap_devmap *table)
{

	pmap_devmap_table = table;
}

/*
 * Map all of the static regions in the devmap table, and remember
 * the devmap table so other parts of the kernel can look up entries
 * later.
 */
void
pmap_devmap_bootstrap(vaddr_t l1pt, const struct pmap_devmap *table)
{
	int i;

	pmap_devmap_table = table;

	for (i = 0; pmap_devmap_table[i].pd_size != 0; i++) {
#ifdef VERBOSE_INIT_ARM
		printf("devmap: %08lx -> %08lx @ %08lx\n",
		    pmap_devmap_table[i].pd_pa,
		    pmap_devmap_table[i].pd_pa +
			pmap_devmap_table[i].pd_size - 1,
		    pmap_devmap_table[i].pd_va);
#endif
		pmap_map_chunk(l1pt, pmap_devmap_table[i].pd_va,
		    pmap_devmap_table[i].pd_pa,
		    pmap_devmap_table[i].pd_size,
		    pmap_devmap_table[i].pd_prot,
		    pmap_devmap_table[i].pd_cache);
	}
}

const struct pmap_devmap *
pmap_devmap_find_pa(paddr_t pa, psize_t size)
{
	uint64_t endpa;
	int i;

	if (pmap_devmap_table == NULL)
		return (NULL);

	endpa = (uint64_t)pa + (uint64_t)(size - 1);

	for (i = 0; pmap_devmap_table[i].pd_size != 0; i++) {
		if (pa >= pmap_devmap_table[i].pd_pa &&
		    endpa <= (uint64_t)pmap_devmap_table[i].pd_pa +
			     (uint64_t)(pmap_devmap_table[i].pd_size - 1))
			return (&pmap_devmap_table[i]);
	}

	return (NULL);
}

const struct pmap_devmap *
pmap_devmap_find_va(vaddr_t va, vsize_t size)
{
	int i;

	if (pmap_devmap_table == NULL)
		return (NULL);

	for (i = 0; pmap_devmap_table[i].pd_size != 0; i++) {
		if (va >= pmap_devmap_table[i].pd_va &&
		    va + size - 1 <= pmap_devmap_table[i].pd_va +
				     pmap_devmap_table[i].pd_size - 1)
			return (&pmap_devmap_table[i]);
	}

	return (NULL);
}

/********************** PTE initialization routines **************************/

/*
 * These routines are called when the CPU type is identified to set up
 * the PTE prototypes, cache modes, etc.
 *
 * The variables are always here, just in case modules need to reference
 * them (though, they shouldn't).
 */

pt_entry_t	pte_l1_s_cache_mode;
pt_entry_t	pte_l1_s_cache_mode_pt;
pt_entry_t	pte_l1_s_cache_mask;

pt_entry_t	pte_l2_l_cache_mode;
pt_entry_t	pte_l2_l_cache_mode_pt;
pt_entry_t	pte_l2_l_cache_mask;

pt_entry_t	pte_l2_s_cache_mode;
pt_entry_t	pte_l2_s_cache_mode_pt;
pt_entry_t	pte_l2_s_cache_mask;

pt_entry_t	pte_l1_s_prot_u;
pt_entry_t	pte_l1_s_prot_w;
pt_entry_t	pte_l1_s_prot_ro;
pt_entry_t	pte_l1_s_prot_mask;

pt_entry_t	pte_l2_s_prot_u;
pt_entry_t	pte_l2_s_prot_w;
pt_entry_t	pte_l2_s_prot_ro;
pt_entry_t	pte_l2_s_prot_mask;

pt_entry_t	pte_l2_l_prot_u;
pt_entry_t	pte_l2_l_prot_w;
pt_entry_t	pte_l2_l_prot_ro;
pt_entry_t	pte_l2_l_prot_mask;

pt_entry_t	pte_l1_s_proto;
pt_entry_t	pte_l1_c_proto;
pt_entry_t	pte_l2_s_proto;

void		(*pmap_copy_page_func)(paddr_t, paddr_t);
void		(*pmap_zero_page_func)(paddr_t);

#if (ARM_MMU_GENERIC + ARM_MMU_SA1 + ARM_MMU_V6 + ARM_MMU_V7) != 0
void
pmap_pte_init_generic(void)
{

	pte_l1_s_cache_mode = L1_S_B|L1_S_C;
	pte_l1_s_cache_mask = L1_S_CACHE_MASK_generic;

	pte_l2_l_cache_mode = L2_B|L2_C;
	pte_l2_l_cache_mask = L2_L_CACHE_MASK_generic;

	pte_l2_s_cache_mode = L2_B|L2_C;
	pte_l2_s_cache_mask = L2_S_CACHE_MASK_generic;

	/*
	 * If we have a write-through cache, set B and C.  If
	 * we have a write-back cache, then we assume setting
	 * only C will make those pages write-through.
	 */
	if (cpufuncs.cf_dcache_wb_range == (void *) cpufunc_nullop) {
		pte_l1_s_cache_mode_pt = L1_S_B|L1_S_C;
		pte_l2_l_cache_mode_pt = L2_B|L2_C;
		pte_l2_s_cache_mode_pt = L2_B|L2_C;
	} else {
#if ARM_MMU_V6 > 1
		pte_l1_s_cache_mode_pt = L1_S_B|L1_S_C; /* arm116 errata 399234 */
		pte_l2_l_cache_mode_pt = L2_B|L2_C; /* arm116 errata 399234 */
		pte_l2_s_cache_mode_pt = L2_B|L2_C; /* arm116 errata 399234 */
#else
		pte_l1_s_cache_mode_pt = L1_S_C;
		pte_l2_l_cache_mode_pt = L2_C;
		pte_l2_s_cache_mode_pt = L2_C;
#endif
	}

	pte_l1_s_prot_u = L1_S_PROT_U_generic;
	pte_l1_s_prot_w = L1_S_PROT_W_generic;
	pte_l1_s_prot_ro = L1_S_PROT_RO_generic;
	pte_l1_s_prot_mask = L1_S_PROT_MASK_generic;

	pte_l2_s_prot_u = L2_S_PROT_U_generic;
	pte_l2_s_prot_w = L2_S_PROT_W_generic;
	pte_l2_s_prot_ro = L2_S_PROT_RO_generic;
	pte_l2_s_prot_mask = L2_S_PROT_MASK_generic;

	pte_l2_l_prot_u = L2_L_PROT_U_generic;
	pte_l2_l_prot_w = L2_L_PROT_W_generic;
	pte_l2_l_prot_ro = L2_L_PROT_RO_generic;
	pte_l2_l_prot_mask = L2_L_PROT_MASK_generic;

	pte_l1_s_proto = L1_S_PROTO_generic;
	pte_l1_c_proto = L1_C_PROTO_generic;
	pte_l2_s_proto = L2_S_PROTO_generic;

	pmap_copy_page_func = pmap_copy_page_generic;
	pmap_zero_page_func = pmap_zero_page_generic;
}

#if defined(CPU_ARM8)
void
pmap_pte_init_arm8(void)
{

	/*
	 * ARM8 is compatible with generic, but we need to use
	 * the page tables uncached.
	 */
	pmap_pte_init_generic();

	pte_l1_s_cache_mode_pt = 0;
	pte_l2_l_cache_mode_pt = 0;
	pte_l2_s_cache_mode_pt = 0;
}
#endif /* CPU_ARM8 */

#if defined(CPU_ARM9) && defined(ARM9_CACHE_WRITE_THROUGH)
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

	pte_l1_s_cache_mode_pt = L1_S_C;
	pte_l2_l_cache_mode_pt = L2_C;
	pte_l2_s_cache_mode_pt = L2_C;
}
#endif /* CPU_ARM9 && ARM9_CACHE_WRITE_THROUGH */
#endif /* (ARM_MMU_GENERIC + ARM_MMU_SA1 + ARM_MMU_V6) != 0 */

#if defined(CPU_ARM10)
void
pmap_pte_init_arm10(void)
{

	/*
	 * ARM10 is compatible with generic, but we want to use
	 * write-through caching for now.
	 */
	pmap_pte_init_generic();

	pte_l1_s_cache_mode = L1_S_B | L1_S_C;
	pte_l2_l_cache_mode = L2_B | L2_C;
	pte_l2_s_cache_mode = L2_B | L2_C;

	pte_l1_s_cache_mode_pt = L1_S_C;
	pte_l2_l_cache_mode_pt = L2_C;
	pte_l2_s_cache_mode_pt = L2_C;

}
#endif /* CPU_ARM10 */

#if defined(CPU_ARM11) && defined(ARM11_CACHE_WRITE_THROUGH)
void
pmap_pte_init_arm11(void)
{

	/*
	 * ARM11 is compatible with generic, but we want to use
	 * write-through caching for now.
	 */
	pmap_pte_init_generic();

	pte_l1_s_cache_mode = L1_S_C;
	pte_l2_l_cache_mode = L2_C;
	pte_l2_s_cache_mode = L2_C;

	pte_l1_s_cache_mode_pt = L1_S_C;
	pte_l2_l_cache_mode_pt = L2_C;
	pte_l2_s_cache_mode_pt = L2_C;
}
#endif /* CPU_ARM11 && ARM11_CACHE_WRITE_THROUGH */

#if ARM_MMU_SA1 == 1
void
pmap_pte_init_sa1(void)
{

	/*
	 * The StrongARM SA-1 cache does not have a write-through
	 * mode.  So, do the generic initialization, then reset
	 * the page table cache mode to B=1,C=1, and note that
	 * the PTEs need to be sync'd.
	 */
	pmap_pte_init_generic();

	pte_l1_s_cache_mode_pt = L1_S_B|L1_S_C;
	pte_l2_l_cache_mode_pt = L2_B|L2_C;
	pte_l2_s_cache_mode_pt = L2_B|L2_C;

	pmap_needs_pte_sync = 1;
}
#endif /* ARM_MMU_SA1 == 1*/

#if ARM_MMU_XSCALE == 1
#if (ARM_NMMUS > 1)
static u_int xscale_use_minidata;
#endif

void
pmap_pte_init_xscale(void)
{
	uint32_t auxctl;
	int write_through = 0;

	pte_l1_s_cache_mode = L1_S_B|L1_S_C;
	pte_l1_s_cache_mask = L1_S_CACHE_MASK_xscale;

	pte_l2_l_cache_mode = L2_B|L2_C;
	pte_l2_l_cache_mask = L2_L_CACHE_MASK_xscale;

	pte_l2_s_cache_mode = L2_B|L2_C;
	pte_l2_s_cache_mask = L2_S_CACHE_MASK_xscale;

	pte_l1_s_cache_mode_pt = L1_S_C;
	pte_l2_l_cache_mode_pt = L2_C;
	pte_l2_s_cache_mode_pt = L2_C;

#ifdef XSCALE_CACHE_READ_WRITE_ALLOCATE
	/*
	 * The XScale core has an enhanced mode where writes that
	 * miss the cache cause a cache line to be allocated.  This
	 * is significantly faster than the traditional, write-through
	 * behavior of this case.
	 */
	pte_l1_s_cache_mode |= L1_S_XS_TEX(TEX_XSCALE_X);
	pte_l2_l_cache_mode |= L2_XS_L_TEX(TEX_XSCALE_X);
	pte_l2_s_cache_mode |= L2_XS_T_TEX(TEX_XSCALE_X);
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
	/* force write back cache mode */
	write_through = 0;
#elif defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
	/*
	 * Intel PXA2[15]0 processors are known to have a bug in
	 * write-back cache on revision 4 and earlier (stepping
	 * A[01] and B[012]).  Fixed for C0 and later.
	 */
	{
		uint32_t id, type;

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

#if (ARM_NMMUS > 1)
	xscale_use_minidata = 1;
#endif

	pte_l1_s_prot_u = L1_S_PROT_U_xscale;
	pte_l1_s_prot_w = L1_S_PROT_W_xscale;
	pte_l1_s_prot_ro = L1_S_PROT_RO_xscale;
	pte_l1_s_prot_mask = L1_S_PROT_MASK_xscale;

	pte_l2_s_prot_u = L2_S_PROT_U_xscale;
	pte_l2_s_prot_w = L2_S_PROT_W_xscale;
	pte_l2_s_prot_ro = L2_S_PROT_RO_xscale;
	pte_l2_s_prot_mask = L2_S_PROT_MASK_xscale;

	pte_l2_l_prot_u = L2_L_PROT_U_xscale;
	pte_l2_l_prot_w = L2_L_PROT_W_xscale;
	pte_l2_l_prot_ro = L2_L_PROT_RO_xscale;
	pte_l2_l_prot_mask = L2_L_PROT_MASK_xscale;

	pte_l1_s_proto = L1_S_PROTO_xscale;
	pte_l1_c_proto = L1_C_PROTO_xscale;
	pte_l2_s_proto = L2_S_PROTO_xscale;

	pmap_copy_page_func = pmap_copy_page_xscale;
	pmap_zero_page_func = pmap_zero_page_xscale;

	/*
	 * Disable ECC protection of page table access, for now.
	 */
	__asm volatile("mrc p15, 0, %0, c1, c0, 1" : "=r" (auxctl));
	auxctl &= ~XSCALE_AUXCTL_P;
	__asm volatile("mcr p15, 0, %0, c1, c0, 1" : : "r" (auxctl));
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
#ifndef ARM32_NEW_VM_LAYOUT
		pte = (pt_entry_t *)
		    kernel_pt_lookup(pde[va >> L1_S_SHIFT] & L2_S_FRAME);
#else
		pte = (pt_entry_t *) kernel_pt_lookup(
		    pde[L1_IDX(va)] & L1_C_ADDR_MASK);
#endif
		if (pte == NULL)
			panic("xscale_setup_minidata: can't find L2 table for "
			    "VA 0x%08lx", va);
#ifndef ARM32_NEW_VM_LAYOUT
		pte[(va >> PGSHIFT) & 0x3ff] =
#else
		pte[l2pte_index(va)] =
#endif
		    L2_S_PROTO | pa | L2_S_PROT(PTE_KERNEL, VM_PROT_READ) |
		    L2_C | L2_XS_T_TEX(TEX_XSCALE_X);
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
	__asm volatile("mcr p15, 0, %0, c7, c6, 0" : : "r" (0));
	__asm volatile("mrc p15, 0, %0, c1, c0, 1" : "=r" (auxctl));
	auxctl = (auxctl & ~XSCALE_AUXCTL_MD_MASK) | XSCALE_AUXCTL_MD_WB_RWA;
	__asm volatile("mcr p15, 0, %0, c1, c0, 1" : : "r" (auxctl));
}

/*
 * Change the PTEs for the specified kernel mappings such that they
 * will use the mini data cache instead of the main data cache.
 */
void
pmap_uarea(vaddr_t va)
{
	struct l2_bucket *l2b;
	pt_entry_t *ptep, *sptep, pte;
	vaddr_t next_bucket, eva;

#if (ARM_NMMUS > 1)
	if (xscale_use_minidata == 0)
		return;
#endif

	eva = va + USPACE;

	while (va < eva) {
		next_bucket = L2_NEXT_BUCKET(va);
		if (next_bucket > eva)
			next_bucket = eva;

		l2b = pmap_get_l2_bucket(pmap_kernel(), va);
		KDASSERT(l2b != NULL);

		sptep = ptep = &l2b->l2b_kva[l2pte_index(va)];

		while (va < next_bucket) {
			pte = *ptep;
			if (!l2pte_minidata(pte)) {
				cpu_dcache_wbinv_range(va, PAGE_SIZE);
				cpu_tlb_flushD_SE(va);
				*ptep = pte & ~L2_B;
			}
			ptep++;
			va += PAGE_SIZE;
		}
		PTE_SYNC_RANGE(sptep, (u_int)(ptep - sptep));
	}
	cpu_cpwait();
}
#endif /* ARM_MMU_XSCALE == 1 */

#if ARM_MMU_V7 == 1
void
pmap_pte_init_armv7(void)
{
	/*
	 * The ARMv7-A MMU is mostly compatible with generic. If the
	 * AP field is zero, that now means "no access" rather than
	 * read-only. The prototypes are a little different because of
	 * the XN bit.
	 */
	pmap_pte_init_generic();

	pte_l1_s_cache_mask = L1_S_CACHE_MASK_armv7;
	pte_l2_l_cache_mask = L2_L_CACHE_MASK_armv7;
	pte_l2_s_cache_mask = L2_S_CACHE_MASK_armv7;

	pte_l1_s_prot_u = L1_S_PROT_U_armv7;
	pte_l1_s_prot_w = L1_S_PROT_W_armv7;
	pte_l1_s_prot_ro = L1_S_PROT_RO_armv7;
	pte_l1_s_prot_mask = L1_S_PROT_MASK_armv7;

	pte_l2_s_prot_u = L2_S_PROT_U_armv7;
	pte_l2_s_prot_w = L2_S_PROT_W_armv7;
	pte_l2_s_prot_ro = L2_S_PROT_RO_armv7;
	pte_l2_s_prot_mask = L2_S_PROT_MASK_armv7;

	pte_l2_l_prot_u = L2_L_PROT_U_armv7;
	pte_l2_l_prot_w = L2_L_PROT_W_armv7;
	pte_l2_l_prot_ro = L2_L_PROT_RO_armv7;
	pte_l2_l_prot_mask = L2_L_PROT_MASK_armv7;

	pte_l1_s_proto = L1_S_PROTO_armv7;
	pte_l1_c_proto = L1_C_PROTO_armv7;
	pte_l2_s_proto = L2_S_PROTO_armv7;
}
#endif /* ARM_MMU_V7 */

/*
 * return the PA of the current L1 table, for use when handling a crash dump
 */
uint32_t pmap_kernel_L1_addr(void)
{
	return pmap_kernel()->pm_l1->l1_physaddr;
}

#if defined(DDB)
/*
 * A couple of ddb-callable functions for dumping pmaps
 */
void pmap_dump_all(void);
void pmap_dump(pmap_t);

void
pmap_dump_all(void)
{
	pmap_t pm;

	LIST_FOREACH(pm, &pmap_pmaps, pm_list) {
		if (pm == pmap_kernel())
			continue;
		pmap_dump(pm);
		printf("\n");
	}
}

static pt_entry_t ncptes[64];
static void pmap_dump_ncpg(pmap_t);

void
pmap_dump(pmap_t pm)
{
	struct l2_dtable *l2;
	struct l2_bucket *l2b;
	pt_entry_t *ptep, pte;
	vaddr_t l2_va, l2b_va, va;
	int i, j, k, occ, rows = 0;

	if (pm == pmap_kernel())
		printf("pmap_kernel (%p): ", pm);
	else
		printf("user pmap (%p): ", pm);

	printf("domain %d, l1 at %p\n", pm->pm_domain, pm->pm_l1->l1_kva);

	l2_va = 0;
	for (i = 0; i < L2_SIZE; i++, l2_va += 0x01000000) {
		l2 = pm->pm_l2[i];

		if (l2 == NULL || l2->l2_occupancy == 0)
			continue;

		l2b_va = l2_va;
		for (j = 0; j < L2_BUCKET_SIZE; j++, l2b_va += 0x00100000) {
			l2b = &l2->l2_bucket[j];

			if (l2b->l2b_occupancy == 0 || l2b->l2b_kva == NULL)
				continue;

			ptep = l2b->l2b_kva;
			
			for (k = 0; k < 256 && ptep[k] == 0; k++)
				;

			k &= ~63;
			occ = l2b->l2b_occupancy;
			va = l2b_va + (k * 4096);
			for (; k < 256; k++, va += 0x1000) {
				char ch = ' ';
				if ((k % 64) == 0) {
					if ((rows % 8) == 0) {
						printf(
"          |0000   |8000   |10000  |18000  |20000  |28000  |30000  |38000\n");
					}
					printf("%08lx: ", va);
				}

				ncptes[k & 63] = 0;
				pte = ptep[k];
				if (pte == 0) {
					ch = '.';
				} else {
					occ--;
					switch (pte & 0x0c) {
					case 0x00:
						ch = 'D'; /* No cache No buff */
						break;
					case 0x04:
						ch = 'B'; /* No cache buff */
						break;
					case 0x08:
						if (pte & 0x40)
							ch = 'm';
						else
						   ch = 'C'; /* Cache No buff */
						break;
					case 0x0c:
						ch = 'F'; /* Cache Buff */
						break;
					}

					if ((pte & L2_S_PROT_U) == L2_S_PROT_U)
						ch += 0x20;

					if ((pte & 0xc) == 0)
						ncptes[k & 63] = pte;
				}

				if ((k % 64) == 63) {
					rows++;
					printf("%c\n", ch);
					pmap_dump_ncpg(pm);
					if (occ == 0)
						break;
				} else
					printf("%c", ch);
			}
		}
	}
}

static void
pmap_dump_ncpg(pmap_t pm)
{
	struct vm_page *pg;
	struct vm_page_md *md;
	struct pv_entry *pv;
	int i;

	for (i = 0; i < 63; i++) {
		if (ncptes[i] == 0)
			continue;

		pg = PHYS_TO_VM_PAGE(l2pte_pa(ncptes[i]));
		if (pg == NULL)
			continue;
		md = VM_PAGE_TO_MD(pg);

		printf(" pa 0x%08lx: krw %d kro %d urw %d uro %d\n",
		    VM_PAGE_TO_PHYS(pg),
		    md->krw_mappings, md->kro_mappings,
		    md->urw_mappings, md->uro_mappings);

		SLIST_FOREACH(pv, &md->pvh_list, pv_link) {
			printf("   %c va 0x%08lx, flags 0x%x\n",
			    (pm == pv->pv_pmap) ? '*' : ' ',
			    pv->pv_va, pv->pv_flags);
		}
	}
}
#endif

#ifdef PMAP_STEAL_MEMORY
void
pmap_boot_pageadd(pv_addr_t *newpv)
{
	pv_addr_t *pv, *npv;

	if ((pv = SLIST_FIRST(&pmap_boot_freeq)) != NULL) {
		if (newpv->pv_pa < pv->pv_va) {
			KASSERT(newpv->pv_pa + newpv->pv_size <= pv->pv_pa);
			if (newpv->pv_pa + newpv->pv_size == pv->pv_pa) {
				newpv->pv_size += pv->pv_size;
				SLIST_REMOVE_HEAD(&pmap_boot_freeq, pv_list);
			}
			pv = NULL;
		} else {
			for (; (npv = SLIST_NEXT(pv, pv_list)) != NULL;
			     pv = npv) {
				KASSERT(pv->pv_pa + pv->pv_size < npv->pv_pa);
				KASSERT(pv->pv_pa < newpv->pv_pa);
				if (newpv->pv_pa > npv->pv_pa)
					continue;
				if (pv->pv_pa + pv->pv_size == newpv->pv_pa) {
					pv->pv_size += newpv->pv_size;
					return;
				}
				if (newpv->pv_pa + newpv->pv_size < npv->pv_pa)
					break;
				newpv->pv_size += npv->pv_size;
				SLIST_INSERT_AFTER(pv, newpv, pv_list);
				SLIST_REMOVE_AFTER(newpv, pv_list);
				return;
			}
		}
	}

	if (pv) {
		SLIST_INSERT_AFTER(pv, newpv, pv_list);
	} else {
		SLIST_INSERT_HEAD(&pmap_boot_freeq, newpv, pv_list);
	}
}

void
pmap_boot_pagealloc(psize_t amount, psize_t mask, psize_t match,
	pv_addr_t *rpv)
{
	pv_addr_t *pv, **pvp;
	struct vm_physseg *ps;
	size_t i;

	KASSERT(amount & PGOFSET);
	KASSERT((mask & PGOFSET) == 0);
	KASSERT((match & PGOFSET) == 0);
	KASSERT(amount != 0);

	for (pvp = &SLIST_FIRST(&pmap_boot_freeq);
	     (pv = *pvp) != NULL;
	     pvp = &SLIST_NEXT(pv, pv_list)) {
		pv_addr_t *newpv;
		psize_t off;
		/*
		 * If this entry is too small to satify the request...
		 */
		KASSERT(pv->pv_size > 0);
		if (pv->pv_size < amount)
			continue;

		for (off = 0; off <= mask; off += PAGE_SIZE) {
			if (((pv->pv_pa + off) & mask) == match
			    && off + amount <= pv->pv_size)
				break;
		}
		if (off > mask)
			continue;

		rpv->pv_va = pv->pv_va + off;
		rpv->pv_pa = pv->pv_pa + off;
		rpv->pv_size = amount;
		pv->pv_size -= amount;
		if (pv->pv_size == 0) {
			KASSERT(off == 0);
			KASSERT((vaddr_t) pv == rpv->pv_va);
			*pvp = SLIST_NEXT(pv, pv_list);
		} else if (off == 0) {
			KASSERT((vaddr_t) pv == rpv->pv_va);
			newpv = (pv_addr_t *) (rpv->pv_va + amount);
			*newpv = *pv;
			newpv->pv_pa += amount;
			newpv->pv_va += amount;
			*pvp = newpv;
		} else if (off < pv->pv_size) {
			newpv = (pv_addr_t *) (rpv->pv_va + amount);
			*newpv = *pv;
			newpv->pv_size -= off;
			newpv->pv_pa += off + amount;
			newpv->pv_va += off + amount;

			SLIST_NEXT(pv, pv_list) = newpv;
			pv->pv_size = off;
		} else {
			KASSERT((vaddr_t) pv != rpv->pv_va);
		}
		memset((void *)rpv->pv_va, 0, amount);
		return;
	}

	if (vm_nphysseg == 0)
		panic("pmap_boot_pagealloc: couldn't allocate memory");

	for (pvp = &SLIST_FIRST(&pmap_boot_freeq);
	     (pv = *pvp) != NULL;
	     pvp = &SLIST_NEXT(pv, pv_list)) {
		if (SLIST_NEXT(pv, pv_list) == NULL)
			break;
	}
	KASSERT(mask == 0);
	for (i = 0; i < vm_nphysseg; i++) {
		ps = VM_PHYSMEM_PTR(i);
		if (ps->avail_start == atop(pv->pv_pa + pv->pv_size)
		    && pv->pv_va + pv->pv_size <= ptoa(ps->avail_end)) {
			rpv->pv_va = pv->pv_va;
			rpv->pv_pa = pv->pv_pa;
			rpv->pv_size = amount;
			*pvp = NULL;
			pmap_map_chunk(kernel_l1pt.pv_va,
			     ptoa(ps->avail_start) + (pv->pv_va - pv->pv_pa), 
			     ptoa(ps->avail_start),
			     amount - pv->pv_size,
			     VM_PROT_READ|VM_PROT_WRITE,
			     PTE_CACHE);
			ps->avail_start += atop(amount - pv->pv_size);
			/*
			 * If we consumed the entire physseg, remove it.
			 */
			if (ps->avail_start == ps->avail_end) {
				for (--vm_nphysseg; i < vm_nphysseg; i++)
					VM_PHYSMEM_PTR_SWAP(i, i + 1);
			}
			memset((void *)rpv->pv_va, 0, rpv->pv_size);
			return;
		}
	} 

	panic("pmap_boot_pagealloc: couldn't allocate memory");
}

vaddr_t
pmap_steal_memory(vsize_t size, vaddr_t *vstartp, vaddr_t *vendp)
{
	pv_addr_t pv;

	pmap_boot_pagealloc(size, 0, 0, &pv);

	return pv.pv_va;
}
#endif /* PMAP_STEAL_MEMORY */

SYSCTL_SETUP(sysctl_machdep_pmap_setup, "sysctl machdep.kmpages setup")
{
	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "machdep", NULL,
			NULL, 0, NULL, 0,
			CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT,
			CTLTYPE_INT, "kmpages",
			SYSCTL_DESCR("count of pages allocated to kernel memory allocators"),
			NULL, 0, &pmap_kmpages, 0,
			CTL_MACHDEP, CTL_CREATE, CTL_EOL);
}
