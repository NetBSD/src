/*	$NetBSD: pmap_68k.c,v 1.22 2025/11/24 06:19:56 thorpej Exp $	*/

/*-     
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
 * Pmap module for the Motorola 68851 / 68030 / 68040 / 68060 MMUs.
 * (...and HP 68851-like MMU.)
 *
 * This implementation supports both 2-level and 3-level page table
 * layouts.  The 3-level is mandated by 68040 / 68060, and the 2-level
 * is mandated by the HP MMU.  The 68851 and 68030 can do either, and
 * for now, the 2-level arrangement is retained for those MMUs, although
 * eventually we will switch them to the 3-level configuration.
 *
 * To support both configurations, page tables are abstracted away from
 * the page table pages that contain them.  The interface pmap operations
 * operate on "leaf" (page) tables, and only when one of those tables needs
 * to be allocated or freed, do the differences between the two configurations
 * need to be dealt with.  All of the tables are kept in a red-black tree
 * that's indexed by their "segment" number (where "segment" is defined as
 * "the amount of space mapped by a single leaf table").  This avoids having
 * to burn large amounts of kernel address space to access tables which are
 * expected to be sparsely-populated.
 *
 * In order to reduce the number of tree lookups, the most recently used
 * leaf table is cached, and the interface contract is such that bulk
 * operations are allowed to access subsequent PTEs within a given table
 * (segment) without having to perform another PTE lookup.
 *
 * This illustrates the initial table layout for a simple program
 * (/usr/bin/yes) using the standard m68k address space layout (based
 * on the historical 4.3BSD-on-hp300 layout, which was itself based on
 * HP-UX in order to facilitate HP-UX binary compatibility back when
 * that was considered to be important).  This example uses a 4K page
 * size.
 *
 * TEXTADDR is $0000.2000 (not always strictly true, but close enough)
 * USRSTACK is $FFF0.0000 (grows down, first used page VA is $FFEF.F000)
 *
 * (TEXTADDR is $0000.2000 because the linker uses 8K page size for
 * broader compatibility and keeps the 0-page unmapped so that NULL
 * pointer dereferences blow up.)
 *
 * This is to say: the text / data / heap of this program are in the
 * bottom 1MB of the address space, and the stack is in the second-from-
 * the-top 1MB of the address space.
 *
 * In the 2-level layout, the level-1 table is 4KB in size, and has 1024
 * entries.  Those 1024 entries together represent the 4GB user address
 * space, and each entry thus maps a 4MB "segment" by itself pointing to
 * a level-2 table which themselves are 4KB in size and have 1024 entries
 * (4MB / 1024 -> 4KB, which is the page size ... convenient!)  So, when
 * our very simple program is loaded, we have a table structure that looks
 * like this:
 *
 *                             (4KB)
 *                    +----------------------+
 *                    |       Level-1        |
 *                    |0                 1023|
 *                    +----------------------+
 *                     |                    |
 *                     |                    |
 *           +---------+                    +---------+
 *           |                                        |
 *           v                                        v
 *         (4KB)                                    (4KB)
 * +----------------------+                 +----------------------+
 * |       Level-2        |                 |       Level-2        |
 * | 2 4                  |                 |             767      |
 * +----------------------+                 +----------------------+
 *   | |                                                   |
 *   | +-+                                                 |
 *   v   v                                                 v
 * TEXT DATA/bss/heap                                    stack
 *
 * As you can see, this requires 3 tables (1 level-1 and 2 level-2).  Each
 * table consumes a full 4KB page, so mapping this address space requires
 * 3 total pages.
 *
 * In the 3-level layout, the level-1 and level-2 tables each contain 128
 * entries, making them 512 bytes in size.  When using 4KB pages, the level-3
 * tables contain 64 entries, making them 256 bytes in size.
 *
 * So, assuming the same address space layout, the 3-level structure looks
 * like this:
 *
 *                              (512B)
 *                         +--------------+
 *                         |   Level-1    |
 *                         |0          127|
 *                         +--------------+
 *                          |           |
 *                      +---+           +---+
 *                      v                   v
 *                    (512B)              (512B)
 *               +--------------+    +--------------+
 *               |   Level-2    |    |   Level-2    |
 *               |0             |    |          123 |
 *               +--------------+    +--------------+
 *                |                              |
 *      +---------+                              +-----+
 *      v                                              v
 *    (256B)                                         (256B)
 * +------------+                                 +------------+
 * |  Level-3   |                                 |  Level-3   |
 * | 2 4        |                                 |          63|
 * +------------+                                 +------------+
 *   | |                                                      |
 *   | +-+                                                    |
 *   v   v                                                    v
 * TEXT DATA/bss/heap                                       stack
 *
 * The table allocator has two pools of memory for tables in the 3-level
 * configuration: one for "segment" tables (always 512 bytes) and one for
 * "page" or "leaf" tables (256 bytes in size for 4K pages).  Pages are
 * allocated to the pools one at a time, and then the tables are allocated
 * from the pages.  Because of this, we only need two pages, 33% less (!),
 * than the 2-level configuration to map the same address space.
 *
 * There is a cost, however: each access that misses the Address Translation
 * Cache costs one extra memory cycle in the 3-level configuration.
 *
 * LOCKING IN THIS PMAP MODULE:
 *
 * MULTIPROCESSING IS NOT SUPPORTED IN THIS PMAP MODULE.  Adding support
 * for it would not be terribly difficult, but there is little value in
 * doing that work until such time as a multiprocessor m68k machine exists
 * that NetBSD runs on.
 *
 * As such, there is **no** locking performed of any data structures here.
 * We do actually reap a benefit from this perceived laziness: we do not
 * have to worry about lock ordering, which means we can take some shortcuts
 * in some places (especially around pv_entry manipulation).
 *
 * THERE IS A CAVEAT, HOWEVER!  Because there are no guard rails, we cannot,
 * under any circumstances, yield the CPU during the critical section of a
 * pmap operation, as doing so could cause the world to change beneath our
 * feet, possibly rendering our work, for lack of a better term, "crashy".
 * Specifically, this means:
 *
 *	- Adaptive mutexes must not be acquired (e.g. when calling into
 *	  other code, e.g. UVM to get a VA or a page).
 *	- Waiting for memory is not allowed.
 *	- The current thread may not be preempted.
 *
 * If any of those things are required, they must be performed outside of
 * a critical section.  If we discover that this is required while inside
 * a critical section, then we must exit the critical section, perform the
 * blocking work, re-enter the critical section and re-evaluate everything.
 * Macros are provided to mark the boundaries of critical sections:
 *
 *	- PMAP_CRIT_ENTER()
 *	- PMAP_CRIT_EXIT()
 *
 * XXX Alas, doesn't seem to be a way for us to hook into ASSERT_SLEEPABLE()
 * XXX when inside a critical section.  We should explore that for a future
 * XXX enhancement.
 */

/*
 * XXX TODO XXX
 *
 * - Solicit real 68040 testers.
 * - Test on real and emulated 68030.
 * - Finish HP MMU support and test on real HP MMU.
 * - Convert '851 / '030 to 3-level.
 * - Kcore / libkvm support.
 * - Inline asm for the atomic ops used.
 * - Optimize ATC / cache manipulation.
 * - Add some more instrumentation.
 * - Eventually disable instrumentation by default.
 * - ...
 * - PROFIT!
 */

#include "opt_m68k_arch.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap_68k.c,v 1.22 2025/11/24 06:19:56 thorpej Exp $");

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

#include <m68k/cacheops.h>

#if !defined(M68K_MMU_MOTOROLA) && !defined(M68K_MMU_HP)
#error Hit the road, Jack...
#endif

/****************************** SERIALIZATION ********************************/

/*
 * XXX Would like to make these do something lightweight-ish in
 * XXX DIAGNOSTIC kernels (and also make ASSERT_SLEEPABLE() trip
 * XXX if we're in a critical section).
 */

#define	PMAP_CRIT_ENTER()	__nothing
#define	PMAP_CRIT_EXIT()	__nothing
#define	PMAP_CRIT_ASSERT()	__nothing

/**************************** MMU CONFIGURATION ******************************/

#include "opt_m68k_arch.h"

#if defined(M68K_MMU_68030)
#include <m68k/mmu_30.h>	/* for cpu_kcore_hdr_t */
#endif

/*
 * We consider 3 different MMU classes:
 * - 68851 (includes 68030)
 * - 68040 (includes 68060)
 * - HP MMU for 68020 (68851-like, 2-level 4K only, external VAC)
 */

#define	MMU_CLASS_68851		0
#define	MMU_CLASS_68040		1
#define	MMU_CLASS_HP		3

static int	pmap_mmuclass __read_mostly;

#if defined(M68K_MMU_68851) || defined(M68K_MMU_68030)
#define	MMU_CONFIG_68851_CLASS	1
#else
#define	MMU_CONFIG_68851_CLASS	0
#endif

#if defined(M68K_MMU_68040) || defined(M68K_MMU_68060)
#define	MMU_CONFIG_68040_CLASS	1
#else
#define	MMU_CONFIG_68040_CLASS	0
#endif

#if defined(M68K_MMU_HP)
#define	MMU_CONFIG_HP_CLASS	1
#else
#define	MMU_CONFIG_HP_CLASS	0
#endif

#define	MMU_CONFIG_NCLASSES	(MMU_CONFIG_68851_CLASS + \
				 MMU_CONFIG_68040_CLASS + \
				 MMU_CONFIG_HP_CLASS)

#if MMU_CONFIG_NCLASSES == 1

#if MMU_CONFIG_68851_CLASS
#define	MMU_IS_68851_CLASS	1
#elif MMU_CONFIG_68040_CLASS
#define	MMU_IS_68040_CLASS	1
#elif MMU_CONFIG_HP_CLASS
#define	MMU_IS_HP_CLASS		1
#else
#error Single MMU config predicate error.
#endif

#else /* MMU_CONFIG_NCLASSES != 1 */

#if MMU_CONFIG_68851_CLASS
#define	MMU_IS_68851_CLASS	(pmap_mmuclass == MMU_CLASS_68851)
#endif

#if MMU_CONFIG_68040_CLASS
#define	MMU_IS_68040_CLASS	(pmap_mmuclass == MMU_CLASS_68040)
#endif

#if MMU_CONFIG_HP_CLASS
#define	MMU_IS_HP_CLASS		(pmap_mmuclass == MMU_CLASS_HP)
#endif

#endif /* MMU_CONFIG_NCLASSES == 1 */

#ifndef MMU_IS_68851_CLASS
#define	MMU_IS_68851_CLASS	0
#endif

#ifndef MMU_IS_68040_CLASS
#define	MMU_IS_68040_CLASS	0
#endif

#ifndef MMU_IS_HP_CLASS
#define	MMU_IS_HP_CLASS		0
#endif

/*
 * 68040 must always use 3-level.  Eventually, we will switch the '851
 * type over to 3-level as well, for for now, it gets 2-level.  The
 * HP MMU is stuck there for all eternity.
 */
#define	MMU_USE_3L		(MMU_IS_68040_CLASS)
#define	MMU_USE_2L		(!MMU_USE_3L)

/***************************** INSTRUMENTATION *******************************/

#define	PMAP_EVENT_COUNTERS

static struct evcnt pmap_nkptpages_initial_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap nkptpages", "initial");
static struct evcnt pmap_nkptpages_current_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap nkptpages", "current");
EVCNT_ATTACH_STATIC(pmap_nkptpages_initial_ev);
EVCNT_ATTACH_STATIC(pmap_nkptpages_current_ev);

static struct evcnt pmap_nkstpages_initial_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap nkstpages", "initial");
static struct evcnt pmap_nkstpages_current_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap nkstpages", "current");
EVCNT_ATTACH_STATIC(pmap_nkstpages_initial_ev);
EVCNT_ATTACH_STATIC(pmap_nkstpages_current_ev);

static struct evcnt pmap_maxkva_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", "maxkva");
EVCNT_ATTACH_STATIC(pmap_maxkva_ev);

static struct evcnt pmap_kvalimit_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", "kvalimit");
EVCNT_ATTACH_STATIC(pmap_kvalimit_ev);

#ifdef PMAP_EVENT_COUNTERS
static struct evcnt pmap_pv_alloc_wait_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pv_alloc", "wait");
EVCNT_ATTACH_STATIC(pmap_pv_alloc_wait_ev);

static struct evcnt pmap_pv_alloc_nowait_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pv_alloc", "nowait");
EVCNT_ATTACH_STATIC(pmap_pv_alloc_nowait_ev);

static struct evcnt pmap_pv_enter_called_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pv_enter", "called");
EVCNT_ATTACH_STATIC(pmap_pv_enter_called_ev);

static struct evcnt pmap_pv_enter_usr_ci_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pv_enter", "usr_ci");
EVCNT_ATTACH_STATIC(pmap_pv_enter_usr_ci_ev);

#if MMU_CONFIG_HP_CLASS
static struct evcnt pmap_pv_enter_vac_ci_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pv_enter", "vac_ci");
EVCNT_ATTACH_STATIC(pmap_pv_enter_vac_ci_ev);
#endif

static struct evcnt pmap_pv_enter_ci_multi_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pv_enter", "ci_multi");
EVCNT_ATTACH_STATIC(pmap_pv_enter_ci_multi_ev);

static struct evcnt pmap_pv_remove_called_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pv_remove", "called");
EVCNT_ATTACH_STATIC(pmap_pv_remove_called_ev);

static struct evcnt pmap_pv_remove_ci_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pv_remove", "ci");
EVCNT_ATTACH_STATIC(pmap_pv_remove_ci_ev);

static struct evcnt pmap_pt_cache_hit_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pt_cache", "hit");
EVCNT_ATTACH_STATIC(pmap_pt_cache_hit_ev);

static struct evcnt pmap_pt_cache_miss_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap pt_cache", "miss");
EVCNT_ATTACH_STATIC(pmap_pt_cache_miss_ev);

static struct evcnt pmap_enter_valid_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "valid");
EVCNT_ATTACH_STATIC(pmap_enter_valid_ev);

static struct evcnt pmap_enter_wire_change_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "wire change");
EVCNT_ATTACH_STATIC(pmap_enter_wire_change_ev);

static struct evcnt pmap_enter_prot_change_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "prot change");
EVCNT_ATTACH_STATIC(pmap_enter_prot_change_ev);

static struct evcnt pmap_enter_pa_change_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "pa change");
EVCNT_ATTACH_STATIC(pmap_enter_pa_change_ev);

static struct evcnt pmap_enter_pv_recycle_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "pv recycle");
EVCNT_ATTACH_STATIC(pmap_enter_pv_recycle_ev);

#define	pmap_evcnt(e)		pmap_ ## e ## _ev.ev_count++
#else
#define	pmap_evcnt(e)		__nothing
#endif

static void (*pmap_load_urp_func)(paddr_t) __read_mostly;

static void
pmap_mmuclass_init(void)
{
	switch (mmutype) {
#if MMU_CONFIG_68040_CLASS
	case MMU_68040:
	case MMU_68060:
		pmap_mmuclass = MMU_CLASS_68040;
		/*
		 * XXX This is messy because 68060 frequently gets
		 * XXX initialize to MMU_68040.  Should be cleaned
		 * XXX up once the Hibler pmap is obsoleted.
		 */
#if defined(M68040)
		if (cputype == CPU_68040) {
			pmap_load_urp_func = mmu_load_urp40;
		}
#endif
#if defined(M68060)
		if (cputype == CPU_68060) {
			pmap_load_urp_func = mmu_load_urp60;
		}
#endif
		break;
#endif
#if MMU_CONFIG_68851_CLASS
	case MMU_68851:
	case MMU_68030:
		pmap_mmuclass = MMU_CLASS_68851;
		protorp[0] = MMU51_CRP_BITS;
		pmap_load_urp_func = mmu_load_urp51;
		break;
#endif
#if MMU_CONFIG_HP_CLASS
	case MMU_HP:
		pmap_mmuclass = MMU_CLASS_HP;
		pmap_load_urp_func = mmu_load_urp20hp;
		break;
#endif
	default:
		panic("%s: mmutype=%d not configured?", __func__, mmutype);
	}

	if (pmap_load_urp_func == NULL) {
		panic("%s: No mmu_load_*() for cputype=%d mmutype=%d",
		    __func__, cputype, mmutype);
	}
}

/*
 * pmap_load_urp:
 *
 *	Load the user root table into the MMU.
 */
static inline void
pmap_load_urp(paddr_t urp)
{
	(*pmap_load_urp_func)(urp);
}

#if MMU_CONFIG_HP_CLASS
static vaddr_t	pmap_aliasmask __read_mostly;
#endif

/*
 * pmap_init_vac:
 *
 *	Set up virtually-addressed cache information.  Only relevant
 *	for the HP MMU.
 */
void
pmap_init_vac(size_t vacsize)
{
#if MMU_CONFIG_HP_CLASS
	KASSERT(pmap_aliasmask == 0);
	KASSERT(powerof2(vacsize));
	pmap_aliasmask = vacsize - 1;
#endif
}

/***************************** PHYS <-> VM PAGE ******************************/

static bool pmap_initialized_p;

static inline struct vm_page *
pmap_pa_to_pg(paddr_t pa)
{
	return pmap_initialized_p ? PHYS_TO_VM_PAGE(pa) : NULL;
}

/*************************** RESOURCE MANAGEMENT *****************************/

static struct pmap kernel_pmap_store;
struct pmap * const kernel_pmap_ptr = &kernel_pmap_store;

/*
 * Physical address of kernel level 1 table.  This name is compatible
 * with the Hibler pmap's name.
 */
paddr_t		Sysseg_pa;

/*
 * Avoid a memory load when doing comparisons against pmap_kernel()
 * within this compilation unit.
 */
#undef pmap_kernel
#define	pmap_kernel()	(&kernel_pmap_store)

static inline bool
active_pmap(pmap_t pmap)
{
	return pmap == pmap_kernel() ||
	       pmap == curproc->p_vmspace->vm_map.pmap;
}

static inline bool
active_user_pmap(pmap_t pmap)
{
	return curproc != NULL &&
	       pmap != pmap_kernel() &&
	       pmap == curproc->p_vmspace->vm_map.pmap;
}

/*
 * Number of tables per page table page:
 * 0 - number of leaf page tables per page
 * 1 - number of segment tables per page
 */
static unsigned int pmap_ptpage_table_counts[2];

__CTASSERT(LA40_L1_COUNT == LA40_L2_COUNT);

static void
pmap_ptpage_init(void)
{
	if (MMU_USE_3L) {
		pmap_ptpage_table_counts[0] = PAGE_SIZE / TBL40_L3_SIZE;
		pmap_ptpage_table_counts[1] = PAGE_SIZE / TBL40_L2_SIZE;
	} else {
		pmap_ptpage_table_counts[0] = 1;
		pmap_ptpage_table_counts[1] = 1;
	}
}

static struct vm_page *
pmap_page_alloc(bool nowait)
{
	struct vm_page *pg;
	const int flags = nowait ? UVM_PGA_USERESERVE : 0;

	while ((pg = uvm_pagealloc(NULL, 0, NULL, flags)) == NULL) {
		if (nowait) {
			return NULL;
		}
		uvm_wait("pmappg");
	}
	pg->flags &= ~PG_BUSY;	/* never busy */

	return pg;
}

static struct pmap_ptpage *
pmap_ptpage_alloc(bool segtab, bool nowait)
{
	const unsigned int tabcnt = pmap_ptpage_table_counts[segtab];
	const size_t size = sizeof(struct pmap_ptpage) +
	    (sizeof(struct pmap_table) * tabcnt);
	const size_t tabsize = PAGE_SIZE / tabcnt;
	struct pmap_ptpage *ptp;
	struct pmap_table *pt;
	struct vm_page *pg;
	vaddr_t ptpva;

	ptp = kmem_zalloc(size, nowait ? 0 : KM_SLEEP);
	if (__predict_false(ptp == NULL)) {
		return NULL;
	}

	/* Allocate a VA for the PT page. */
	ptpva = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
			     UVM_KMF_VAONLY | (nowait ? UVM_KMF_NOWAIT : 0));
	if (__predict_false(ptpva == 0)) {
		kmem_free(ptp, size);
		return NULL;
	}

	/* Get a page. */
	pg = pmap_page_alloc(nowait);
	if (__predict_false(pg == NULL)) {
		kmem_free(ptp, size);
		return NULL;
	}

	/* Map the page cache-inhibited and zero it out. */
	pmap_kenter_pa(ptpva, VM_PAGE_TO_PHYS(pg),
	    UVM_PROT_READ | UVM_PROT_WRITE, PMAP_NOCACHE);
	zeropage((void *)ptpva);

	/*
	 * All resources for the PT page have been allocated.
	 * Now initialize it and the individual table descriptors.
	 */
	LIST_INIT(&ptp->ptp_freelist);
	ptp->ptp_pg = pg;
	ptp->ptp_vpagenum = m68k_btop(ptpva);
	ptp->ptp_freecnt = tabcnt;
	ptp->ptp_segtab = segtab;

	for (unsigned int i = 0; i < tabcnt; ptpva += tabsize, i++) {
		pt = &ptp->ptp_tables[i];
		pt->pt_ptpage = ptp;
		pt->pt_entries = (pt_entry_t *)ptpva;
		LIST_INSERT_HEAD(&ptp->ptp_freelist, pt, pt_freelist);
	}

	return ptp;
}

static void
pmap_ptpage_free(struct pmap_ptpage *ptp)
{
	const unsigned int tabcnt = pmap_ptpage_table_counts[ptp->ptp_segtab];
	const size_t size = sizeof(struct pmap_ptpage) +
	    (sizeof(struct pmap_table) * tabcnt);

	uvm_km_free(kernel_map, m68k_ptob(ptp->ptp_vpagenum), PAGE_SIZE,
		    UVM_KMF_WIRED);
	kmem_free(ptp, size);
}

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
	struct pmap_ptpage_list pc_ptpages;
	struct pmap_pv_list pc_pvlist;
};

static inline void
pmap_completion_init(struct pmap_completion *pc)
{
	TAILQ_INIT(&pc->pc_ptpages);
	LIST_INIT(&pc->pc_pvlist);
}

static void
pmap_completion_fini(struct pmap_completion *pc)
{
	struct pmap_ptpage *ptp;
	struct pv_entry *pv;

	while ((ptp = TAILQ_FIRST(&pc->pc_ptpages)) != NULL) {
		TAILQ_REMOVE(&pc->pc_ptpages, ptp, ptp_list);
		/*
		 * Can't assert ptp_freecnt here; it won't match up
		 * in the pmap_remove_all() case.
		 *
		 * KASSERT(ptp->ptp_freecnt ==
		 *     pmap_ptpage_table_counts[ptp->ptp_segtab]);
		 */
		pmap_ptpage_free(ptp);
	}

	while ((pv = LIST_FIRST(&pc->pc_pvlist)) != NULL) {
		LIST_REMOVE(pv, pv_pmlist);
		pmap_pv_free(pv);
	}
}

/************************ PTE MANIPULATION HELPERS ***************************/

/* Assert assumptions made in <machine/pmap.h>. */
__CTASSERT(DT51_PAGE == PTE40_RESIDENT);
__CTASSERT(PTE51_WP == PTE40_W);
__CTASSERT(PTE51_U == PTE40_U);
__CTASSERT(PTE51_M == PTE40_M);
__CTASSERT(PTE51_CI == PTE40_CM_NC_SER);

static pt_entry_t	pmap_pte_proto[UVM_PROT_ALL + 1];
static pt_entry_t	pmap_pte_proto_ci[UVM_PROT_ALL + 1];
static pt_entry_t	pmap_pte_proto_um[UVM_PROT_ALL + 1];
static pt_entry_t	pmap_ste_proto;

static inline paddr_t
pte_pa(pt_entry_t pte)
{
	return pte & PTE40_PGA;
}

/*
 * These predicate inlines compile down into BFEXTU, so are quite fast.
 */

static inline bool
pte_valid_p(pt_entry_t pte)
{
	return !!(pte & PTE_VALID);
}

static inline bool
pte_wired_p(pt_entry_t pte)
{
	return !!(pte & PTE_WIRED);
}

static inline bool
pte_managed_p(pt_entry_t pte)
{
	return !!(pte & PTE_PVLIST);
}

static inline bool
pte_ci_p(pt_entry_t pte)
{
	/*
	 * Happily, PTE51_CI is bit 6, which is set for both of the
	 * cache-inhibited modes on 68040, so we can just check for
	 * that.
	 */
	return !!(pte & PTE51_CI);
}

#define	PTE_PROT_CHANGE_BITS	(PTE_WP | PTE_CMASK)

static inline pt_entry_t
pte_change_prot(pt_entry_t opte, vm_prot_t prot)
{
	pt_entry_t *pte_proto = pte_ci_p(opte) ? pmap_pte_proto_ci
					       : pmap_pte_proto;

	return (opte & ~PTE_PROT_CHANGE_BITS) | pte_proto[prot];
}

static inline pt_entry_t
pte_load(pt_entry_t *ptep)
{
	return atomic_load_relaxed(ptep);
}

static inline void
pte_store(pt_entry_t *ptep, pt_entry_t npte)
{
	atomic_store_relaxed(ptep, npte);
}

static inline bool
pte_update(pt_entry_t *ptep, pt_entry_t opte, pt_entry_t npte)
{
	/*
	 * Use compare-and-swap to update the PTE.  This ensures there's
	 * no possibility of losing any hardware-maintained bits when
	 * updating the PTE.
	 */
	return atomic_cas_uint(ptep, opte, npte) == opte;
}

static inline void
pte_set(pt_entry_t *ptep, pt_entry_t bits)
{
	atomic_or_uint(ptep, bits);
}

static inline void
pte_mask(pt_entry_t *ptep, pt_entry_t mask)
{
	atomic_and_uint(ptep, mask);
}

static inline pt_entry_t
pte_set_ci(pt_entry_t pte)
{
	return (pte & ~PTE_CMASK) | (MMU_IS_68040_CLASS ? PTE40_CM_NC_SER
							: PTE51_CI);
}

static inline pt_entry_t
pte_clr_ci(pt_entry_t pte)
{
	pte &= ~PTE_CMASK;
	if (MMU_IS_68040_CLASS) {
		pte |= (pte & PTE_WP) ? PTE40_CM_WT
				      : PTE40_CM_CB;
	}
	return pte;
}

static void
pmap_pte_proto_init(void)
{
	pt_entry_t c_bits, ro_c_bits, rw_c_bits, ci_bits, prot_bits, um_bits;
	int prot;

	if (MMU_IS_68040_CLASS) {
		ro_c_bits = PTE40_CM_WT; /* this is what the Hibler pmap did */
		rw_c_bits = PTE40_CM_CB;
		ci_bits = PTE40_CM_NC_SER;
	} else {
		ro_c_bits = rw_c_bits = 0;
		ci_bits = PTE51_CI;
	}

	for (prot = 1; prot <= UVM_PROT_ALL; prot++) {
		prot_bits = um_bits = 0;
		if (prot & UVM_PROT_WRITE) {
			um_bits = PTE_U | PTE_M;
		} else if (prot & (UVM_PROT_READ|UVM_PROT_EXEC)) {
			prot_bits = PTE_WP;
			um_bits = PTE_U;
		}
		c_bits = (prot & UVM_PROT_WRITE) ? rw_c_bits : ro_c_bits;
		pmap_pte_proto[prot]    = PTE_VALID | prot_bits | c_bits;
		pmap_pte_proto_ci[prot] = PTE_VALID | prot_bits | ci_bits;
		pmap_pte_proto_um[prot] = um_bits;
	}

	/*
	 * from hp300/DOC/HPMMU.notes:
	 *
	 * Segment table entries:
	 *
	 * bits 31-12:	Physical page frame number of PT page
	 * bits 11-4:	Reserved at zero (can software use them?)
	 * bit 3:	Reserved at one
	 * bits 1-0:	Valid bits (hardware uses bit 1)
	 *
	 * This is all roughly compatible with 68851 and 68040:
	 *
	 * bit 3:	DTE51_U / UTE40_U (used)
	 * bits 1-0:	DT51_SHORT / UTE40_RESIDENT
	 *
	 * The Hibler pmap set "SG_U" in the 68040 case, but not in
	 * any others (??), which seems at odds with HPMMU.notes, but
	 * whatever.  It does not seem to cause any harm to set the
	 * "used" bit in all cases, so that's what we'll do.  If it
	 * does prove to be problematic, we can make adjustments.
	 */
	pmap_ste_proto = DTE51_U | DT51_SHORT;
}

static inline pt_entry_t
pmap_make_pte(paddr_t pa, vm_prot_t prot, u_int flags)
{
	pt_entry_t *pte_proto = (flags & PMAP_NOCACHE) ? pmap_pte_proto_ci
						       : pmap_pte_proto;

	prot &= UVM_PROT_ALL;
	KASSERT(prot != 0);

	pt_entry_t npte = pa | pte_proto[prot] |
	    pmap_pte_proto_um[flags & UVM_PROT_ALL];

	if (flags & PMAP_WIRED) {
		npte |= PTE_WIRED;
	}

	return npte;
}

/************************** PAGE TABLE MANAGEMENT ****************************/

/*
 * Kernel page table management works differently from user page table
 * management.  An initial set of kernel PTs are allocated during early
 * bootstrap (enough to map the virtual addresses set up at that time,
 * plus a little extra to give the kernel some breathing room while
 * UVM gets initialized -- see pmap_bootstrap1()).  If more PTs are
 * needed in order to expand the kernel address space, pmap_growkernel()
 * is called to allocate some more.  We always allocate kernel PTs in
 * chunks of one page, allocating more inner segment tables as needed
 * to link them into the MMU tree (3-level), or just poking them in
 * directly to the level-1 table (2-level).
 *
 * The kernel PTs are mapped into a single linear array to make that
 * makes it possible to simply index by virtual page number to find
 * the PTE that maps that virtual address.
 */
#define	PTPAGEVASZ	((PAGE_SIZE / sizeof(pt_entry_t)) * PAGE_SIZE)
#define	PTPAGEVAOFS	(PTPAGEVASZ - 1)

#define	pmap_round_ptpage(va)	(((va) + PTPAGEVAOFS) & ~PTPAGEVAOFS)

/*
 * kernel_virtual_start marks the first kernel virtual address that
 * is handed off to UVM to manage.  kernel_virtual_end marks the end
 * of the kernel address space that is currently mappable with the
 * number of pages allocated to kernel PTs.
 *
 * kernel_virtual_start is fixed once pmap_bootstrap1() completes.
 * kernel_virtual_end can be extended by calling pmap_growkernel().
 *
 * kernel_virtual_max represents the absolute maximum.  It starts at
 * KERNEL_MAX_ADDRESS, but may get clamped by fixed mappings that
 * start beyond the end of kernel virtual address space.
 *
 * kernel_virtual_max is exported to the rest of the kernel via
 * pmap_virtual_space() and VM_MAX_KERNEL_ADDRESS.
 */
#define	KERNEL_MAX_ADDRESS	((vaddr_t)0 - PAGE_SIZE)
static vaddr_t kernel_virtual_start, kernel_virtual_end;
       vaddr_t kernel_virtual_max = KERNEL_MAX_ADDRESS;

/*
 * kernel_stnext_pa and kernel_stnext_endpa together implement a
 * simple allocator for inner segment tables used in the 3-level
 * configuration.  When the initial level-1 table is allocated
 * the remained of that page is set in kernel_stnext_pa, and
 * kernel_stnext_endpa is set to the next page boundary.  When
 * a segment table is needed, kernel_stnext_pa is the address
 * of the next free table and is advanced by the L2 table size
 * (512 bytes).  If that allocation attempt finds that kernel_stnext_pa
 * is equal to kernel_stnext_endpa, a new page is allocated and
 * kernel_stnext_pa and kernel_stnext_endpa updated to reflect
 * the newly-allocated page before the table is taken from it.
 */
static paddr_t kernel_stnext_pa, kernel_stnext_endpa;

/*
 * Null segment table that every pmap gets as its initial level 1
 * map.  This is a single page allocated in pmap_bootstrap1(), and
 * we zero it out in pmap_init().
 */
static paddr_t null_segtab_pa __read_mostly;

static inline void
pmap_set_lev1map(pmap_t pmap, struct pmap_table *pt, paddr_t pa)
{
	pmap->pm_lev1map = pt;
	pmap->pm_lev1pa = pa;
	if (active_user_pmap(pmap)) {
#if MMU_CONFIG_HP_CLASS
		/*
		 * N.B. re-loading the user segment table pointer also
		 * invalidates the user side of the VAC, so no additional
		 * work is necessary.
		 */
#endif
		pmap_load_urp(pmap->pm_lev1pa);
		TBIAU();		/* XXX optimize? */
		ICIA();			/* XXX optimize? */
	}
}

/*
 * Table accessors.
 */
static inline unsigned int
pmap_pagenum(vaddr_t va)
{
	return ((va) >> PGSHIFT);
}

static inline unsigned int
pmap_segnum(vaddr_t va)
{
	return MMU_USE_3L ? ((va) >> SEGSHIFT3L) : ((va) >> SEGSHIFT2L);
}

static inline unsigned int
pmap_st1_index(vaddr_t va)
{
	return MMU_USE_3L ? LA40_RI(va) : LA2L_RI(va);
}

static inline unsigned int
pmap_st_index(vaddr_t va)
{
	return MMU_USE_3L ? LA40_PI(va) : LA2L_RI(va);
}

static inline unsigned int
pmap_pt_index(vaddr_t va)
{
	return MMU_USE_3L ? LA40_PGI(va) : LA2L_PGI(va);
}

static inline vaddr_t
pmap_trunc_seg(vaddr_t va)
{
	return MMU_USE_3L ? pmap_trunc_seg_3L(va) : pmap_trunc_seg_2L(va);
}

static inline vaddr_t
pmap_trunc_seg1(vaddr_t va)
{
	KASSERT(MMU_USE_3L);
	return pmap_trunc_seg1_3L(va);
}

static inline vaddr_t
pmap_round_seg(vaddr_t va)
{
	return MMU_USE_3L ? pmap_round_seg_3L(va) : pmap_round_seg_2L(va);
}

static inline vaddr_t
pmap_next_seg(vaddr_t va)
{
	return pmap_round_seg(va + PAGE_SIZE);
}

static paddr_t
pmap_table_pa(const struct pmap_table * const pt)
{
	const struct pmap_ptpage * const ptp = pt->pt_ptpage;
	const vaddr_t ptpva = m68k_ptob(ptp->ptp_vpagenum);
	const vaddr_t ptva = (vaddr_t)pt->pt_entries;

	return VM_PAGE_TO_PHYS(ptp->ptp_pg) + (ptva - ptpva);
}

static inline unsigned int
pmap_table_make_key(unsigned int segnum, bool segtab)
{
	KASSERT((segnum & 0x80000000) == 0);
	return (segnum << 1) | (unsigned int)segtab;
}

static int
pmap_table_rb_compare_key(void *v __unused, const void *n, const void *k)
{
	const struct pmap_table * const pt1 = n;
	const unsigned int k1 = pt1->pt_key;
	const unsigned int k2 = *(const unsigned int *)k;

	return (int)(k1 - k2);
}

static int
pmap_table_rb_compare_nodes(void *v, const void *n1, const void *n2)
{
	const struct pmap_table * const pt2 = n2;

	return pmap_table_rb_compare_key(v, n1, &pt2->pt_key);
}

static const rb_tree_ops_t pmap_table_rb_ops = {
	.rbto_compare_nodes = pmap_table_rb_compare_nodes,
	.rbto_compare_key   = pmap_table_rb_compare_key,
	.rbto_node_offset   = offsetof(struct pmap_table, pt_node),
};

static struct pmap_table *
pmap_table_alloc(pmap_t pmap, bool segtab, bool nowait,
    struct pmap_completion *pc)
{
	struct pmap_ptpage_list *pmlist = &pmap->pm_ptpages[segtab];
	struct pmap_ptpage *ptp, *newptp = NULL;
	struct pmap_table *pt;

	KASSERT(pc != NULL);

 try_again:
	if ((ptp = TAILQ_FIRST(pmlist)) == NULL || ptp->ptp_freecnt == 0) {
		/*
		 * No PT pages with free tables (empty PT pages are moved
		 * to the tail of the list).  Allocate a new PT page and
		 * try again.  If someone else successfully allocates one
		 * while we're sleeping, then we'll use it and free what
		 * we allocated back to the system.
		 */
		KASSERT(ptp == NULL || LIST_FIRST(&ptp->ptp_freelist) == NULL);
		if (newptp == NULL) {
			newptp = pmap_ptpage_alloc(segtab, nowait);
			if (newptp == NULL) {
				/*
				 * If we didn't wait, then no one would
				 * have allocted one behind our back.
				 */
				KASSERT(nowait);
				return NULL;
			}
			goto try_again;
		}
		ptp = newptp;
		TAILQ_INSERT_HEAD(pmlist, newptp, ptp_list);
	}
	if (__predict_false(newptp != NULL && ptp != newptp)) {
		/* Not using newly-allocated PT page; free it back. */
		TAILQ_INSERT_TAIL(&pc->pc_ptpages, newptp, ptp_list);
	}
	pt = LIST_FIRST(&ptp->ptp_freelist);
	KASSERT(pt != NULL);
	LIST_REMOVE(pt, pt_freelist);
	ptp->ptp_freecnt--;
	if (ptp->ptp_freecnt == 0 &&
	    TAILQ_NEXT(ptp, ptp_list) != NULL) {
		TAILQ_REMOVE(pmlist, ptp, ptp_list);
		TAILQ_INSERT_TAIL(pmlist, ptp, ptp_list);
	}
	KASSERT(pt->pt_st == NULL);
	pt->pt_holdcnt = 1;

	return pt;
}

static void
pmap_table_free(pmap_t pmap, struct pmap_table *pt,
		struct pmap_completion *pc)
{
	struct pmap_ptpage *ptp = pt->pt_ptpage;
	struct pmap_ptpage_list *pmlist = &pmap->pm_ptpages[ptp->ptp_segtab];

	KASSERT(pt->pt_st == NULL);

	LIST_INSERT_HEAD(&ptp->ptp_freelist, pt, pt_freelist);
	KASSERT(ptp->ptp_freecnt < pmap_ptpage_table_counts[ptp->ptp_segtab]);
	ptp->ptp_freecnt++;

	/*
	 * If the PT page no longer has any active tables, then
	 * remove it from the pmap and queue it up to be given
	 * back to the system.
	 */
	if (ptp->ptp_freecnt == pmap_ptpage_table_counts[ptp->ptp_segtab]) {
		TAILQ_REMOVE(pmlist, ptp, ptp_list);
		TAILQ_INSERT_TAIL(&pc->pc_ptpages, ptp, ptp_list);
	}
	/*
	 * If the PT page now has exactly one free table, then
	 * put it at the head of its list so that it is allocated
	 * from first the next time a table is needed.
	 */
	else if (ptp->ptp_freecnt == 1) {
		TAILQ_REMOVE(pmlist, ptp, ptp_list);
		TAILQ_INSERT_HEAD(pmlist, ptp, ptp_list);
	}
	/*
	 * Push this PT page down the list if it has more free tables
	 * than the ones that come after.  The goal is to keep PT pages
	 * with the fewest free tables at the head of the list so that
	 * they're allocated from first.  This is an effort to keep
	 * fragmentation at bay so as to increase the likelihood that
	 * we can free PT pages back to the system.
	 */
	else {
		struct pmap_ptpage *next_ptp;
		for (next_ptp = TAILQ_NEXT(ptp, ptp_list);
		     next_ptp != NULL;
		     next_ptp = TAILQ_NEXT(next_ptp, ptp_list)) {
			if (next_ptp->ptp_freecnt < ptp->ptp_freecnt) {
				break;
			}
		}
		if (next_ptp != NULL &&
		    next_ptp != TAILQ_NEXT(ptp, ptp_list) &&
		    next_ptp->ptp_freecnt != 0) {
			TAILQ_REMOVE(pmlist, ptp, ptp_list);
			TAILQ_INSERT_AFTER(pmlist, next_ptp, ptp, ptp_list);
		}
	}
}

/*
 * pmap_table_retain:
 *
 *	Take a retain count on the specified table.  Retain counts
 *	are used to ensure the table remains stable while working
 *	on it, and each mapping placed into the table also gets
 *	a retain count.
 */
static inline void
pmap_table_retain(struct pmap_table *pt)
{
	if (__predict_true(pt != NULL)) {
		pt->pt_holdcnt++;
		KASSERT(pt->pt_holdcnt != 0);
	}
}

/*
 * pmap_table_release:
 *
 *	Release a previously-taken retain count on the specified
 *	table.  If the retain count drops to zero, the table is
 *	unlinked from the lookup tree and the MMU tree and freed.
 */
static __noinline void
pmap_table_release_slow(pmap_t pmap, struct pmap_table *pt,
			struct pmap_completion *pc)
{
	KASSERT(pt != NULL);
	KASSERT(pt->pt_holdcnt != 0);
	pt->pt_holdcnt--;
	if (__predict_false(pt->pt_holdcnt != 0)) {
		return;
	}

	/*
	 * If the caller doesn't expect the count to go to zero,
	 * they won't have bothered with a completion context.
	 * Going to zero is unexpected in this case, so blow up
	 * if it happens.
	 */
	KASSERT(pc != NULL);
	if (__predict_true(pt == pmap->pm_pt_cache)) {
		pmap->pm_pt_cache = NULL;
	}
	if (__predict_true(pt->pt_st != NULL)) {
		/*
		 * This table needs to be unlinked from the lookup
		 * tree and the MMU tree.
		 */
		pte_store(&pt->pt_st->pt_entries[pt->pt_stidx], 0);
		rb_tree_remove_node(&pmap->pm_tables, pt);
		pmap_table_release_slow(pmap, pt->pt_st, pc);
		pt->pt_st = NULL;
	} else if (pt == pmap->pm_lev1map) {
		pmap_set_lev1map(pmap, NULL, null_segtab_pa);
	}
	pmap_table_free(pmap, pt, pc);
}

static inline void
pmap_table_release(pmap_t pmap, struct pmap_table *pt,
		   struct pmap_completion *pc)
{
	if (__predict_true(pt != NULL)) {
		if (__predict_true(pt->pt_holdcnt > 1)) {
			pt->pt_holdcnt--;
			return;
		}
		pmap_table_release_slow(pmap, pt, pc);
	}
}

/*
 * pmap_table_lookup:
 *
 *	Lookup the table corresponding to the specified segment.
 */
static struct pmap_table *
pmap_table_lookup(pmap_t pmap, unsigned int segnum, bool segtab)
{
	const unsigned int key = pmap_table_make_key(segnum, segtab);
	struct pmap_table *pt;

	if ((pt = pmap->pm_pt_cache) == NULL || pt->pt_key != key) {
		pmap_evcnt(pt_cache_miss);
		pt = rb_tree_find_node(&pmap->pm_tables, &key);
		if (__predict_true(!segtab)) {
			pmap->pm_pt_cache = pt;
		}
	} else {
		pmap_evcnt(pt_cache_hit);
	}
	if (pt != NULL) {
		pmap_table_retain(pt);
	}
	return pt;
}

/*
 * pmap_table_insert:
 *
 *	Allocate and insert a table into the tree at the specified
 *	location.
 */
static struct pmap_table *
pmap_table_insert(pmap_t pmap, struct pmap_table *t1, unsigned int stidx,
    unsigned int segnum, bool segtab, bool nowait, struct pmap_completion *pc)
{
	struct pmap_table *t2, *ret_t;

	t2 = pmap_table_lookup(pmap, segnum, segtab);
	if (t2 != NULL) {
		/*
		 * Table at this level already exists, and looking
		 * it up gave us a retain count, so we no longer need
		 * the retain count on the upper level table (it is
		 * retained-by-proxy by the table we just found).
		 * We pass NULL for the completion context because
		 * we don't expect the upper level table's retain count
		 * to drop to zero, and we want things to blow up
		 * loudly if it does!
		 */
		pmap_table_release(pmap, t1, NULL);
		return t2;
	}

	/* Allocate the new table. */
	PMAP_CRIT_EXIT();
	t2 = pmap_table_alloc(pmap, segtab, nowait, pc);
	PMAP_CRIT_ENTER();
	if (__predict_false(t2 == NULL)) {
		pmap_table_release(pmap, t1, pc);
		return NULL;
	}
	t2->pt_key = pmap_table_make_key(segnum, segtab);

	/*
	 * Now that we have the new table, we need to insert it into the
	 * table lookup tree.  If we blocked while allocating, it's possible
	 * someone raced with us and inserted one behind our back, so we need
	 * to check for that.
	 */
	ret_t = rb_tree_insert_node(&pmap->pm_tables, t2);
	if (__predict_false(ret_t != t2)) {
		/*
		 * Someone beat us to the punch.  If this happens,
		 * then we also need to drop the retain count on
		 * t1 because the table we just found already has
		 * a retain count on it.
		 */
		pmap_table_retain(ret_t);
		pmap_table_release(pmap, t2, pc);
		pmap_table_release(pmap, t1, NULL);
		return ret_t;
	}

	/*
	 * Table has been successfully inserted into the lookup
	 * tree, now link it into the MMU's tree.  The new table
	 * takes ownership of the retain count that was taken on
	 * the upper level table while working.
	 */
	t2->pt_st = t1;
	t2->pt_stidx = (unsigned short)stidx;
	pte_store(&t1->pt_entries[stidx], pmap_ste_proto | pmap_table_pa(t2));

	return t2;
}

/*************************** PTE LOOKUP FUNCTIONS ****************************/

static pt_entry_t *kernel_ptes;

/*
 * pmap_kernel_pte:
 *
 *	Get the PTE that maps the specified kernel virtual address.
 *
 *	Take note: the caller *may assume* they they can linearly
 *	access adjacent PTEs up until the address indicated by
 *	virtual_end!  That means, "pte++" is totally fine until you
 *	get to the current limit of the kernel virtual address space!
 *
 *	XXX This is exported because db_memrw.c needs it.
 */
pt_entry_t *
pmap_kernel_pte(vaddr_t va)
{
	/*
	 * The kernel PTEs are mapped as a linear array, whose entries
	 * represent the entire possible 4GB supervisor address space.
	 *
	 * Kernel PT pages are pre-allocated and mapped into this linear
	 * space (via pmap_growkernel(), as needed) and never freed back.
	 * So, as long as the VA is below virtual_end, we know that a PTE
	 * exists to back it.
	 *
	 * We don't assert that the VA < virtual_end, however; there may
	 * be special cases where we need to get a PTE that has been
	 * statically-allocated out beyond where virtual space is allowed
	 * to grow.  We'll find out soon enough if a PT page doesn't back
	 * it, because a fault will occur when the PTE is accessed.
	 */
	return &kernel_ptes[va >> PGSHIFT];
}

/*
 * pmap_pte_lookup:
 *
 *	Lookup the PTE for the given address, returning a retained
 *	reference to the table containing the PTE.
 *
 *	Take note: the caller *may assume* they they can linearly
 *	access adjacent PTEs that map addresses within the same
 *	segment!  That means, "pte++" is totally fine until you
 *	get to the next segment boundary!
 */
static pt_entry_t *
pmap_pte_lookup(pmap_t pmap, vaddr_t va, struct pmap_table **out_pt)
{
	if (pmap == pmap_kernel()) {
		*out_pt = NULL;
		return pmap_kernel_pte(va);
	}

	const unsigned int segnum = pmap_segnum(va);

	struct pmap_table *pt = pmap_table_lookup(pmap, segnum, false);
	if (__predict_true(pt != NULL)) {
		*out_pt = pt;	/* already retained */
		return &pt->pt_entries[pmap_pt_index(va)];
	}

	*out_pt = NULL;
	return NULL;
}

/*
 * pmap_pte_alloc:
 *
 *	Like pmap_pte_lookup(), but allocates tables as necessary.
 *
 *	We enter in a critical section, but may drop that along
 *	the way and re-validate our own assumptions.  Callers
 *	(pmap_enter(), basically), should be aware of this.
 */
static pt_entry_t *
pmap_pte_alloc(pmap_t pmap, vaddr_t va, struct pmap_table **out_pt,
    bool nowait, struct pmap_completion *pc)
{
	struct pmap_table *st, *pt;
	pt_entry_t *ptep;

	PMAP_CRIT_ASSERT();

	ptep = pmap_pte_lookup(pmap, va, out_pt);
	if (__predict_true(ptep != NULL)) {
		return ptep;
	}

	/*
	 * First get a reference on the top-level segment table and
	 * retain it so that it's stable while we work.
	 */
	if (__predict_true((st = pmap->pm_lev1map) != NULL)) {
		pmap_table_retain(st);
	} else {
		/*
		 * Oh look!  Baby pmap's first mapping!  Allocate
		 * a segment table.
		 */
		PMAP_CRIT_EXIT();
		st = pmap_table_alloc(pmap, true/*segtab*/, nowait, pc);
		PMAP_CRIT_ENTER();
		if (__predict_false(st == NULL)) {
			return NULL;
		}

		/* Re-validate that we still need the segment table. */
		if (__predict_false(pmap->pm_lev1map != NULL)) {
			/* Raced and lost. */
			pmap_table_release(pmap, st, pc);
			st = pmap->pm_lev1map;
			pmap_table_retain(st);
		} else {
			/* New table is returned to us retained. */
			pmap_set_lev1map(pmap, st, pmap_table_pa(st));
		}
	}

	/*
	 * Now we know that st points to a valid segment table with a
	 * retain count that lets us safely reference it.
	 */

	if (MMU_USE_3L) {
		/* Get the inner segment table for this virtual address. */
		struct pmap_table * const st1 = st;
		st = pmap_table_insert(pmap, st1, pmap_st1_index(va),
		    pmap_st1_index(va), true/*segtab*/, nowait, pc);
		if (__predict_false(st == NULL)) {
			pmap_table_release(pmap, st1, pc);
			return NULL;
		}
	}

	/* We can now allocate and insert the leaf page table. */
	pt = pmap_table_insert(pmap, st, pmap_st_index(va), pmap_segnum(va),
	    false/*segtab*/, nowait, pc);
	if (__predict_false(pt == NULL)) {
		pmap_table_release(pmap, st, pc);
		return NULL;
	}

	*out_pt = pt;
	return &pt->pt_entries[pmap_pt_index(va)];
}

/************************** P->V ENTRY MANAGEMENT ****************************/

static inline pt_entry_t *
pmap_pv_pte(struct pv_entry * const pv)
{
	const vaddr_t va = PV_VA(pv);

	if (__predict_true(pv->pv_pmap != pmap_kernel())) {
		KASSERT(pv->pv_pt != NULL);
		return &pv->pv_pt->pt_entries[pmap_pt_index(va)];
	}
	return pmap_kernel_pte(va);
}

#define	MATCHING_PMAP(p1, p2)			\
	((p1) == (p2) ||			\
	 (p1) == pmap_kernel() || (p2) == pmap_kernel())

#define	CONFLICTING_ALIAS(va1, va2)		\
	(((va1) & pmap_aliasmask) != ((va2) & pmap_aliasmask))

/*
 * pmap_pv_enter:
 *
 *	Add a physical->virtual entry to the pv table.  Caller must provide
 *	the storage for the new PV entry.
 *
 *	We are responsible for storing the new PTE into the destination
 *	table.  We are also guaranteed that no mapping exists there, so
 *	no ATC invlidation for the new mapping is required.
 */
static void
pmap_pv_enter(pmap_t pmap, struct vm_page *pg, vaddr_t va,
    struct pmap_table *pt, pt_entry_t npte, struct pv_entry *newpv)
{
	const bool usr_ci = pte_ci_p(npte);
	struct pv_entry *pv;
	pt_entry_t opte;

	pmap_evcnt(pv_enter_called);

	PMAP_CRIT_ASSERT();
	KASSERT(newpv != NULL);

	npte |= PTE_PVLIST;

	newpv->pv_pmap = pmap;
	newpv->pv_vf = va;
	newpv->pv_pt = pt;

	pt_entry_t *ptep = pmap_pv_pte(newpv);

#ifdef DEBUG
	/*
	 * Make sure the entry doesn't already exist.
	 */
	for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
		if (pmap == pv->pv_pmap && va == PV_VA(pv)) {
			panic("%s: pmap=%p va=0x%08lx already in PV table",
			    __func__, pmap, va);
		}
	}
#endif

	if (__predict_false(usr_ci)) {
		newpv->pv_vf |= PV_F_CI_USR;
	}

	newpv->pv_next = VM_MDPAGE_PVS(pg);
	VM_MDPAGE_SETPVP(VM_MDPAGE_HEAD_PVP(pg), newpv);
	LIST_INSERT_HEAD(&pmap->pm_pvlist, newpv, pv_pmlist);

#if MMU_CONFIG_HP_CLASS
	if (MMU_IS_HP_CLASS) {
		/* Go handle the HP MMU's VAC. */
		goto hp_mmu_vac_shenanigans;
	}
#endif

	/*
	 * If the page is marked as being cache-inhibited, it means
	 * there is at least one user-requested CI mapping already
	 * (and that all of the extant mappings are thus CI).
	 *
	 * In this case, we need to make sure that the one we're
	 * establishing now is CI as well.
	 */
	if (__predict_false(VM_MDPAGE_CI_P(pg))) {
		npte = pte_set_ci(npte);
		pte_store(ptep, npte);
		return;
	}

	/* Set the PTE for the new mapping. */
	pte_store(ptep, npte);

	/*
	 * If this is a user-requested CI mapping, we need to make
	 * sure the page is purged from the cache and mark any other
	 * mappings of this page CI as well.
	 */
	if (__predict_false(usr_ci)) {
		VM_MDPAGE_SET_CI(pg);

		pmap_evcnt(pv_enter_usr_ci);

		/*
		 * There shouldn't be very many of these; CI mappings
		 * of managed pages are typically only for coherent DMA
		 * purposes, and multiple mappings of the same page are
		 * extremely uncommon in that scenario.
		 */
		for (pv = newpv->pv_next; pv != NULL; pv = pv->pv_next) {
			pmap_evcnt(pv_enter_ci_multi);
			ptep = pmap_pv_pte(pv);
			for (;;) {
				opte = pte_load(ptep);
				npte = pte_set_ci(opte);
				if (pte_update(ptep, opte, npte)) {
					if (active_pmap(pv->pv_pmap)) {
						TBIS(PV_VA(pv));
					}
					break;
				}
			}
		}
#if MMU_CONFIG_68040_CLASS
		if (MMU_IS_68040_CLASS) {
			const paddr_t pa = VM_PAGE_TO_PHYS(pg);
			DCFP(pa);
			ICPP(pa);
		}
#endif
	}
	return;

#if MMU_CONFIG_HP_CLASS
 hp_mmu_vac_shenanigans:
	/*
	 * We have ourselves a VAC, so in addition to checking for
	 * user-requested-CI mappings, we have to check for cache
	 * aliases and cache-inhibit all mappings for a page that
	 * have a cache alias conflict.
	 *
	 * - All mappings of a given page within the same pmap must
	 *   not collide.  (The VAC is flushed when switching pmaps
	 *   by virtue of a new segment table pointer being loaded
	 *   into the user segment table register.)
	 *
	 * - The Hibler pmap check to see that the kernel doesn't have
	 *   conflicting mappings with any user pmap.  We'll do the same,
	 *   which seems reasonable on the surface if you think about it
	 *   for a couple of minutes.
	 *
	 * - The Hibler pmap also just punts and cache-inhibits all
	 *   mappings once it becomes > 2, but we do NOT do that because
	 *   it will severely penalize shared libraries.
	 *
	 * N.B. The method used here will not universally render all
	 * mappings for a given page uncached; only address spaces with
	 * conflicts are penalized.
	 *
	 * XXX This probably only matters if one of the mappings is
	 * XXX writable, as this is the only situation where data
	 * XXX inconsistency could arise.  There is probably room
	 * XXX for further optimization if someone with one of these
	 * XXX machines cares to take it up.
	 */
	bool flush_s_vac = false;
	bool flush_u_vac = false;

	/* Set the PTE for the new mapping. */
	pte_store(ptep, npte);

	vaddr_t pv_flags = newpv->pv_vf & PV_F_CI_USR;
	if (usr_ci) {
		pmap_evcnt(pv_enter_usr_ci);
	}

	for (pv = newpv->pv_next; pv != NULL; pv = pv->pv_next) {
		if (MATCHING_PMAP(pmap, pv->pv_pmap) &&
		    CONFLICTING_ALIAS(va, PV_VA(pv))) {
			pmap_evcnt(pv_enter_vac_ci);
			pv_flags |= PV_F_CI_VAC;
			break;
		}
	}

	if (__predict_true(pv_flags == 0)) {
		/* No new inhibitions! */
		return;
	}

	VM_MDPAGE_SET_CI(pg);
	for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
		if (MATCHING_PMAP(pmap, pv->pv_pmap)) {
			pmap_evcnt(pv_enter_ci_multi);
			pv->pv_vf |= pv_flags;
			pte_set(pmap_pv_pte(pv), PTE51_CI);
			if (active_pmap(pv->pv_pmap)) {
				TBIS(PV_VA(pv));
				if (pv->pv_pmap == pmap_kernel()) {
					flush_s_vac = true;
				} else {
					flush_u_vac = true;
				}
			}
		}
	}
	if (flush_u_vac && flush_s_vac) {
		DCIA();
	} else if (flush_u_vac) {
		DCIU();
	} else if (flush_s_vac) {
		DCIS();
	}
#endif /* MMU_CONFIG_HP_CLASS */
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
	pt_entry_t *ptep, opte, npte;

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
	LIST_REMOVE(pv, pv_pmlist);

	KASSERT(pc != NULL);
	LIST_INSERT_HEAD(&pc->pc_pvlist, pv, pv_pmlist);

#if MMU_CONFIG_HP_CLASS
	if (MMU_IS_HP_CLASS) {
		/* Go handle the HP MMU's VAC. */
		goto hp_mmu_vac_shenanigans;
	}
#endif

	/*
	 * If the page is marked as being cache-inhibited, then it
	 * means there was at least one user-requested CI mapping
	 * for the page.  In that case, we need to scan the P->V
	 * list to see if any remain, and if not, clear the CI
	 * status for the page.
	 *
	 * N.B. This requires traversing the list twice: once to
	 * check if any of the mappings are user-requested-CI,
	 * and one again to fix them up.  But, we're making a
	 * classical space-vs-time trade-off here: Assuming that
	 * this is a rare situation, it's better to pay the cpu
	 * cost on the rare edge transitions rather than always pay
	 * the memory cost of having a counter to track something
	 * that almost never happens (and, when it does, the list
	 * will be very short).
	 */
	if (__predict_false(VM_MDPAGE_CI_P(pg))) {
		pmap_evcnt(pv_remove_ci); 
		for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
			if (pv->pv_vf & PV_F_CI_USR) {
				/*
				 * There is still at least one user-requested
				 * CI mapping, so we can't change the page's CI
				 * status.
				 */
				return;
			}
		}
		KASSERT(pv == NULL);
		VM_MDPAGE_CLR_CI(pg);
		for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
			ptep = pmap_pv_pte(pv);
			for (;;) {
				opte = pte_load(ptep);
				npte = pte_clr_ci(opte);
				if (pte_update(ptep, opte, npte)) {
					if (active_pmap(pv->pv_pmap)) {
						TBIS(PV_VA(pv));
					}
					break;
				}
			}
		}
	}
	return;

#if MMU_CONFIG_HP_CLASS
 hp_mmu_vac_shenanigans:
	/*
	 * If we have a VAC and the page was cache-inhibited due to
	 * a cache alias conflict, we can re-enable the cache if there
	 * is just one such mapping left.
	 */
	if (__predict_false(VM_MDPAGE_CI_P(pg))) {
		vaddr_t all_ci_flags = PV_F_CI_USR;

		pmap_evcnt(pv_remove_ci); 

		for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
			if (! MATCHING_PMAP(pmap, pv->pv_pmap)) {
				continue;
			}
			if (pv->pv_vf & all_ci_flags) {
				/*
				 * There is at least one CI_USR mapping
				 * or more than one CI_VAC mapping, so
				 * the CI status of the page remains
				 * unchanged.
				 */
				return;
			}
			all_ci_flags |= pv->pv_vf & PV_F_CI_VAC;
		}
		KASSERT(pv == NULL);
		/*
		 * We now know we can remove CI from the page mappings
		 * in the matching address space.  If no CI mappings
		 * remain, then we can clear the CI indicator on the
		 * page.
		 */
		all_ci_flags = 0;
		for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
			if (! MATCHING_PMAP(pmap, pv->pv_pmap)) {
				all_ci_flags |= pv->pv_vf;
				continue;
			}
			pte_mask(pmap_pv_pte(pv), ~((uint32_t)PTE51_CI));
			if (active_pmap(pv->pv_pmap)) {
				TBIS(PV_VA(pv));
			}
		}
		all_ci_flags &= PV_F_CI_USR | PV_F_CI_VAC;
		if (__predict_true(all_ci_flags == 0)) {
			VM_MDPAGE_CLR_CI(pg);
		}
	}
#endif /* MMU_CONFIG_HP_CLASS */
}

#undef CONFLICTING_ALIAS
#undef MATCHING_PMAP

/***************** PMAP INTERFACE (AND ADJACENT) FUNCTIONS *******************/

static inline void
pmap_stat_update_impl(long *valp, int val)
{
	atomic_add_long(valp, val);
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
pmap_pinit(pmap_t pmap, paddr_t lev1pa)
{
	pmap->pm_lev1pa = lev1pa;
	rb_tree_init(&pmap->pm_tables, &pmap_table_rb_ops);
	TAILQ_INIT(&pmap->pm_ptpages[0]);
	TAILQ_INIT(&pmap->pm_ptpages[1]);
	LIST_INIT(&pmap->pm_pvlist);

	atomic_store_relaxed(&pmap->pm_refcnt, 1);
}

/*
 * pmap_virtual_space:		[ INTERFACE ]
 *
 *	Define the initial bounds of the kernel virtual address space.
 *
 *	In this implementation, the start address we return marks the
 *	end of the statically allocated special kernel virtual addresses
 *	set up in pmap_bootstrap1().  We return kernel_vitual_max as
 *	the end because we can grow the kernel address space using
 *	pmap_growkernel().
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
	/* Zero out the null segment table. */
	pmap_zero_page(null_segtab_pa);
#if MMU_CONFIG_68040_CLASS
	if (MMU_IS_68040_CLASS) {
		DCFP(null_segtab_pa);
	}
#endif

	/* Initialize the pmap / pv_entry allocators. */
	pmap_alloc_init();

	/* Initialize the PT page allocator. */
	pmap_ptpage_init();

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
	 * We reference the null segment table and and have a NULL
	 * lev1map pointer until the first mapping is entered.
	 */
	pmap = pmap_alloc();
	pmap_pinit(pmap, null_segtab_pa);

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
	KASSERT(atomic_load_relaxed(&pmap->pm_refcnt) > 0);
	if (atomic_dec_uint_nv(&pmap->pm_refcnt) > 0) {
		return;
	}

	/* We assume all mappings have been removed. */
	KASSERT(pmap->pm_lev1map == NULL);
	KASSERT(pmap->pm_lev1pa == null_segtab_pa);

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
	atomic_inc_uint(&pmap->pm_refcnt);
	KASSERT(atomic_load_relaxed(&pmap->pm_refcnt) > 0);
}

/*
 * pmap_remove_mapping:
 *
 *	Invalidate a single page denoted by pmap/va.
 *
 *	If (ptep != NULL), it is the already computed PTE for the mapping.
 *
 *	If (flags & PRM_TFLUSH), we must invalidate any TLB information.
 *
 *	If (flags & PRM_CFLUSH), we must flush/invalidate any cache
 *	information.
 *
 *	If the caller wishes to prevent the page table from being freed,
 *	they should perform an extra retain.
 */
#define	PRM_TFLUSH	__BIT(0)
#define	PRM_CFLUSH	__BIT(1)
static void
pmap_remove_mapping(pmap_t pmap, vaddr_t va, pt_entry_t *ptep,
    struct pmap_table *pt, int flags, struct pmap_completion *pc)
{
	KASSERT(ptep != NULL);

	const paddr_t opte = pte_load(ptep);
	if (! pte_valid_p(opte)) {
		return;
	}

	const paddr_t pa = pte_pa(opte);

	/* Update statistics. */
	if (pte_wired_p(opte)) {
		pmap_stat_update(pmap, wired_count, -1);
	}
	pmap_stat_update(pmap, resident_count, -1);

	if (flags & PRM_CFLUSH) {
#if MMU_CONFIG_68040_CLASS
		if (MMU_IS_68040_CLASS) {
			DCFP(pa);
			ICPP(pa);
		}
#endif
#if MMU_CONFIG_HP_CLASS
		if (MMU_IS_HP_CLASS) {
			if (pmap == pmap_kernel()) {
				DCIS();
			} else if (active_user_pmap(pmap)) {
				DCIU();
			}
		}
#endif
	}

	/*
	 * Zap the PTE and drop the retain count that the mapping
	 * had on the table.
	 */
	pte_store(ptep, 0);
	pmap_table_release(pmap, pt, pc);

	/*
	 * Now that the ATC can't be reloaded from the PTE, invalidate
	 * the ATC entry.
	 */
	if (__predict_true((flags & PRM_TFLUSH) != 0 && active_pmap(pmap))) {
		TBIS(va);
	}

	struct vm_page * const pg = pmap_pa_to_pg(pa);
	if (__predict_true(pg != NULL)) {
		KASSERT(pte_managed_p(opte));
		/* Update cached U/M bits from mapping that's going away. */
		VM_MDPAGE_ADD_UM(pg, opte);
		pmap_pv_remove(pmap, pg, va, pc);
	} else {
		KASSERT(! pte_managed_p(opte));
	}
}

/*
 * pmap_remove:			[ INTERFACE ]
 *
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly rounded
 *	to the page size.
 */
static void
pmap_remove_internal(pmap_t pmap, vaddr_t sva, vaddr_t eva,
    struct pmap_completion *pc)
{
	pt_entry_t opte, *ptep;
	struct pmap_table *pt;
	vaddr_t nextseg;
	int prm_flags;
#if MMU_CONFIG_HP_CLASS
	pt_entry_t all_ci = PTE51_CI;
#endif

	/*
	 * If this is the kernel pmap, we can use a faster method
	 * for accessing the PTEs (since the PT pages are always
	 * resident).
	 *
	 * Note that this routine should NEVER be called from an
	 * interrupt context; pmap_kremove() is used for that.
	 */
	prm_flags = active_pmap(pmap) ? PRM_TFLUSH : 0;
	if (pmap == pmap_kernel()) {
		PMAP_CRIT_ENTER();

		for (ptep = pmap_kernel_pte(sva); sva < eva;
		     ptep++, sva += PAGE_SIZE) {
			opte = pte_load(ptep);
			if (pte_valid_p(opte)) {
#if MMU_CONFIG_HP_CLASS
				/*
				 * If all of the PTEs we're zapping have the
				 * cache-inhibit bit set, ci_pte will remain
				 * non-zero and we'll be able to skip flushing
				 * the VAC when we're done.
				 */
				all_ci &= opte;
#endif
				pmap_remove_mapping(pmap, sva, ptep, NULL,
				    prm_flags, pc);
			}
		}
#if MMU_CONFIG_HP_CLASS
		if (MMU_IS_HP_CLASS && !all_ci) {
			/*
			 * Cacheable mappings were removed, so invalidate
			 * the cache.
			 */
			DCIS();
		}
#endif
		PMAP_CRIT_EXIT();

		/* kernel PT pages are never freed. */
		KASSERT(TAILQ_EMPTY(&pc->pc_ptpages));

		/* ...but we might have freed PV entries. */
		pmap_completion_fini(pc);

		return;
	}

	PMAP_CRIT_ENTER();

	while (sva < eva) {
		nextseg = pmap_next_seg(sva);
		if (nextseg == 0 || nextseg > eva) {
			nextseg = eva;
		}

		ptep = pmap_pte_lookup(pmap, sva, &pt);
		if (ptep == NULL) {
			/*
			 * No table for this address, meaning nothing
			 * within this segment; advance to the next
			 * one.
			 */
			sva = nextseg;
			continue;
		}

		for (; sva < nextseg; ptep++, sva += PAGE_SIZE) {
			opte = pte_load(ptep);
			if (! pte_valid_p(opte)) {
				continue;
			}
#if MMU_CONFIG_HP_CLASS
			/*
			 * If all of the PTEs we're zapping have the
			 * cache-inhibit bit set, ci_pte will remain
			 * non-zero and we'll be able to skip flushing
			 * the VAC when we're done.
			 */
			all_ci &= opte;
#endif
			pmap_remove_mapping(pmap, sva, ptep, pt, prm_flags, pc);
		}
		pmap_table_release(pmap, pt, pc);
	}
#if MMU_CONFIG_HP_CLASS
	if (MMU_IS_HP_CLASS && !all_ci) {
		/*
		 * Cacheable mappings were removed, so invalidate
		 * the cache.
		 */
		if (pmap == pmap_kernel()) {
			DCIS();
		} else if (active_user_pmap(pmap)) {
			DCIU();
		}
	}
#endif
	PMAP_CRIT_EXIT();
}

void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	struct pmap_completion pc;
	pmap_completion_init(&pc);
	pmap_remove_internal(pmap, sva, eva, &pc);
}

/*
 * pmap_remove_all:		[ INTERFACE ]
 *
 *	Remove all mappings from a pmap in bulk.  This is only called
 *	when it's known that the address space is no longer visible to
 *	any user process (e.g. during exit or exec).
 */
bool
pmap_remove_all(pmap_t pmap)
{
	struct pmap_completion pc;
	struct pv_entry *pv;

	KASSERT(pmap != pmap_kernel());

	/*
	 * This process is pretty simple:
	 *
	 * ==> (1) Set the segment table pointer to the NULL segment table.
	 *
	 * ==> (2) Copy the PT page list to a tempory list and re-init.
	 *
	 * ==> (3) Walk the PV entry list and remove each entry.
	 *
	 * ==> (4) Zero the wired and resident count.
	 *
	 * Once we've done that, we just need to free everything
	 * back to the system.
	 */

	pmap_completion_init(&pc);

	PMAP_CRIT_ENTER();

	/* Step 1. */
	pmap_set_lev1map(pmap, NULL, null_segtab_pa);

	/* Step 2. */
	pmap->pm_pt_cache = NULL;
	TAILQ_CONCAT(&pc.pc_ptpages, &pmap->pm_ptpages[0], ptp_list);
	TAILQ_CONCAT(&pc.pc_ptpages, &pmap->pm_ptpages[1], ptp_list);
	memset(&pmap->pm_tables, 0, sizeof(pmap->pm_tables));
	rb_tree_init(&pmap->pm_tables, &pmap_table_rb_ops);
	KASSERT(RB_TREE_MIN(&pmap->pm_tables) == NULL);

	/* Step 3. */
	while ((pv = LIST_FIRST(&pmap->pm_pvlist)) != NULL) {
		KASSERT(pv->pv_pmap == pmap);
		pmap_pv_remove(pmap,
		    pmap_pa_to_pg(pte_pa(pte_load(pmap_pv_pte(pv)))),
		    PV_VA(pv), &pc);
	}

	/* Step 4. */
	atomic_store_relaxed(&pmap->pm_stats.wired_count, 0);
	atomic_store_relaxed(&pmap->pm_stats.resident_count, 0);

	PMAP_CRIT_EXIT();

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
		pmap_changebit(pg, PTE_WP, ~0U);
		return;
	}

	/* Removing all mappings for a page. */
	pmap_completion_init(&pc);

	PMAP_CRIT_ENTER();

	while ((pv = VM_MDPAGE_PVS(pg)) != NULL) {
		pmap_remove_mapping(pv->pv_pmap, PV_VA(pv), pmap_pv_pte(pv),
		    pv->pv_pt, PRM_TFLUSH|PRM_CFLUSH, &pc);
	}

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
	pt_entry_t *ptep, opte, npte;
	struct pmap_table *pt;
	vaddr_t nextseg;
#if MMU_CONFIG_68040_CLASS
	bool removing_write;
#endif
	bool need_tflush;

	if ((prot & UVM_PROT_READ) == 0) {
		struct pmap_completion pc;
		pmap_completion_init(&pc);
		pmap_remove_internal(pmap, sva, eva, &pc);
		return;
	}

	PMAP_CRIT_ENTER();

#if MMU_CONFIG_68040_CLASS
	removing_write = (prot & UVM_PROT_WRITE) == 0;
#endif
	need_tflush = active_pmap(pmap);
	while (sva < eva) {
		nextseg = pmap_next_seg(sva);
		if (nextseg == 0 || nextseg > eva) {
			nextseg = eva;
		}

		ptep = pmap_pte_lookup(pmap, sva, &pt);
		if (ptep == NULL) {
			/*
			 * No table for this address, meaning nothing
			 * within this segment; advance to the next
			 * one.
			 */
			sva = nextseg;
			continue;
		}

		/*
		 * Change protection on mapping if it is valid and doesn't
		 * already have the correct protection.
		 */
		for (; sva < nextseg; ptep++, sva += PAGE_SIZE) {
 try_again:
			opte = pte_load(ptep);
			if (! pte_valid_p(opte)) {
				continue;
			}
			npte = pte_change_prot(opte, prot);
			if (npte == opte) {
				continue;
			}
#if MMU_CONFIG_68040_CLASS
			if (MMU_IS_68040_CLASS && removing_write) {
				/*
				 * Clear caches if making RO (see section
				 * "7.3 Cache Coherency" in the manual).
				 */
				paddr_t pa = pte_pa(opte);
				DCFP(pa);
				ICPP(pa);
			}
#endif
			if (! pte_update(ptep, opte, npte)) {
				/* Lost race updating PTE; try again. */
				goto try_again;
			}
			if (need_tflush) {
				TBIS(sva);
			}
		}
		pmap_table_release(pmap, pt, NULL);
	}

	PMAP_CRIT_EXIT();
}

/*
 * pmap_enter:			[ INTERFACE ]
 *
 *	Insert the given physical address (pa) at the specified
 *	virtual address (va) in the target physical map with the
 *	protection requested.
 *
 *	If specified, the page will be wired down, meaning that
 *	related pte can not be reclaimed.
 *
 *	Note:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */
int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	struct pmap_table *pt;
	pt_entry_t *ptep, npte, opte;
	struct pv_entry *newpv;
	struct pmap_completion pc;
	int error = 0;
	bool nowait = false;

	pmap_completion_init(&pc);

	struct vm_page * const pg = pmap_pa_to_pg(pa);
	if (__predict_false(pg == NULL)) {
		/*
		 * PA is not part of managed memory.  Make the mapping
		 * cache-inhibited on the assumption that it's a device.
		 */
		flags |= PMAP_NOCACHE;
	}

	PMAP_CRIT_ENTER();

	/* Get the destination table. */
	ptep = pmap_pte_alloc(pmap, va, &pt, nowait, &pc);
	if (__predict_false(ptep == NULL)) {
		error = ENOMEM;
		goto out;
	}

	/* Compute the new PTE. */
	npte = pmap_make_pte(pa, prot, flags);

	/* Fetch old PTE. */
	opte = pte_load(ptep);

	/*
	 * Check to see if there is a valid mapping at this address.
	 * It might simply be a wiring or protection change.
	 */
	if (pte_valid_p(opte)) {
 		pmap_evcnt(enter_valid);
 restart:
		if (pte_pa(opte) == pa) {
			/*
			 * Just a protection or wiring change.
			 *
			 * Since the old PTE is handy, go ahead and update
			 * the cached U/M attributes now.  Normally we would
			 * do this in pmap_remove_mapping(), but we're not
			 * taking that path in this case.  We also add in
			 * any U/M attributes hinted by the access type
			 * that brought us to pmap_enter() in the first
			 * place (a write-fault on a writable page mapped
			 * read-only during a page-out, for example).
			 *
			 * Also ensure that the PV list status of the mapping
			 * is consistent.
			 */
			if (__predict_true(pg != NULL)) {
				VM_MDPAGE_ADD_UM(pg, opte | npte);
				KASSERT(pte_managed_p(opte));
				npte |= PTE_PVLIST;
			}

			/* Preserve cache-inhibited status. */
			if (__predict_false(pte_ci_p(opte))) {
				npte =
				    (npte & ~PTE_CMASK) | (opte & PTE_CMASK);
			}

			/* Set the new PTE. */
			pte_store(ptep, npte);

			const pt_entry_t diff = opte ^ npte;

#ifdef PMAP_EVENT_COUNTERS
			if (diff & PTE_WIRED) {
				pmap_evcnt(enter_wire_change);
			}
			if (diff & PTE_WP) {
				pmap_evcnt(enter_prot_change);
			}
#endif

			if (pte_wired_p(diff)) {
				pmap_stat_update(pmap, wired_count,
				    pte_wired_p(npte) ? 1 : -1);
			}
			if (diff & PTE_CRIT_BITS) {
#if MMU_CONFIG_68040_CLASS
				/*
				 * Protection or caching status is changing;
				 * flush the page from the cache.
				 */
				if (MMU_IS_68040_CLASS) {
					DCFP(pa);
					ICPP(pa);
				}
#endif
				if (active_pmap(pmap)) {
					TBIS(va);
#if MMU_CONFIG_HP_CLASS
					/*
					 * If the new mapping is CI and the old
					 * one is not, then flush the VAC.
					 */
					if (__predict_false(MMU_IS_HP_CLASS &&
							    pte_ci_p(diff) &&
							    pte_ci_p(npte))) {
						DCIA();
					}
#endif
				}
			}

			/* All done! */
			goto out_release;
		}

		/*
		 * The mapping has completely changed.  Need to remove
		 * the old one first.
		 *
		 * This drops the retain count on the PT owned by the
		 * previous mapping, but the newly-entered mapping will
		 * inherit the retain count taken when we looked up the
		 * PTE.
		 */
		pmap_evcnt(enter_pa_change);
		pmap_remove_mapping(pmap, va, ptep, pt,
		    PRM_TFLUSH|PRM_CFLUSH, &pc);
	}

	/*
	 * By the time we get here, we should be assured that the
	 * PTE at ptep is invalid.
	 */
	KASSERT(! pte_valid_p(pte_load(ptep)));

	/* Update pmap stats now. */
	pmap_stat_update(pmap, resident_count, 1);
	if (__predict_false(pte_wired_p(npte))) {
		pmap_stat_update(pmap, wired_count, 1);
	}

	if (__predict_true(pg != NULL)) {
		/*
		 * Managed pages also go on the PV list, so we are
		 * going to need a PV entry.
		 */
		newpv = LIST_FIRST(&pc.pc_pvlist);
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
					error = ENOMEM;
					goto out_release;
				}
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
				opte = pte_load(ptep);
				if (__predict_false(pte_valid_p(opte))) {
					pmap_stat_update(pmap,
					    resident_count, -1);
					if (pte_wired_p(npte)) {
						pmap_stat_update(pmap,
						    wired_count, -1);
					}
					LIST_INSERT_HEAD(&pc.pc_pvlist,
					    newpv, pv_pmlist);
					goto restart;
				}
			}
		} else {
			pmap_evcnt(enter_pv_recycle);
			LIST_REMOVE(newpv, pv_pmlist);
		}

		/*
		 * Enter the mapping into the PV list.  pmap_pv_enter()
		 * will also set the PTE in the table.
		 */
		pmap_pv_enter(pmap, pg, va, pt, npte, newpv);

		/*
		 * The new mapping takes ownership of the PT
		 * retain count we took while looking up the PTE.
		 */
		goto out_crit_exit;
	}

	/*
	 * Not a managed mapping, so set the new PTE.  As with managed
	 * mappings, the new mapping takes ownership of the PT retain
	 * count we took while looking up the PTE.
	 */
	pte_store(ptep, npte);
	goto out_crit_exit;

 out_release:
	pmap_table_release(pmap, pt, &pc);
 out_crit_exit:
	PMAP_CRIT_EXIT();
 out:
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

	KASSERT(va >= VM_MIN_KERNEL_ADDRESS);

	pt_entry_t * const ptep = pmap_kernel_pte(va);

	/* Build the new PTE. */
	const pt_entry_t npte = pmap_make_pte(pa, prot, flags | PMAP_WIRED);

	/* Set the new PTE. */
	const pt_entry_t opte = pte_load(ptep);
	pte_store(ptep, npte);

	/*
	 * There should not have been anything here, previously,
	 * so we can skip ATC invalidation in the common case.
	 */
	if (__predict_false(pte_valid_p(opte))) {
		if (__predict_false(pte_managed_p(opte))) {
			/*
			 * Can't handle this case and it's a legitimate
			 * error if it happens.
			 */
			panic("%s: old mapping was managed", __func__);
		}
		if (__predict_false(! pte_wired_p(opte))) {
			pmap_stat_update(pmap, wired_count, 1);
		}
		TBIS(va);
	} else {
		pmap_stat_update(pmap, resident_count, 1);
		pmap_stat_update(pmap, wired_count, 1);
	}
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
	pt_entry_t *ptep, opte;
	pmap_t const pmap = pmap_kernel();
	int count = 0;
#if MMU_CONFIG_HP_CLASS
	pt_entry_t all_ci = PTE51_CI;
#endif

	KASSERT(va >= VM_MIN_KERNEL_ADDRESS);

	for (ptep = pmap_kernel_pte(va); size != 0;
	     ptep++, size -= PAGE_SIZE, va += PAGE_SIZE) {
		opte = pte_load(ptep);
		if (pte_valid_p(opte)) {
			KASSERT(! pte_managed_p(opte));
			KASSERT(pte_wired_p(opte));
#if MMU_CONFIG_HP_CLASS
			/*
			 * If all of the PTEs we're zapping have the
			 * cache-inhibit bit set, ci_pte will remain
			 * non-zero and we'll be able to skip flushing
			 * the VAC when we're done.
			 */
			all_ci &= opte;
#endif
			/* Zap the mapping. */
			pte_store(ptep, 0);
			TBIS(va);
			count++;
		}
	}
#if MMU_CONFIG_HP_CLASS
	if (MMU_IS_HP_CLASS && !all_ci) {
		/*
		 * Cacheable mappings were removed, so invalidate
		 * the cache.
		 */
		DCIS();
	}
#endif
	/* Update stats. */
	if (__predict_true(count != 0)) {
		pmap_stat_update(pmap, resident_count, -count);
		pmap_stat_update(pmap, wired_count, -count);
	}
}

/*
 * pmap_unwire:			[ INTERFACE ]
 *
 *	Clear the wired attribute for a map/virtual-address pair.
 *
 *	The mapping must already exist in the pmap.
 */
void
pmap_unwire(pmap_t pmap, vaddr_t va)
{
	struct pmap_table *pt;
	pt_entry_t opte, npte, *ptep;

	PMAP_CRIT_ENTER();

	ptep = pmap_pte_lookup(pmap, va, &pt);
	KASSERT(ptep != NULL);

	for (;;) {
		opte = pte_load(ptep);
		KASSERT(pte_valid_p(opte));

		/*
		 * If the wiring actually changed (always?), clear the wire
		 * bit and update the wire count.  Note that the wiring is
		 * not a hardware characteristic so there is no need to
		 * invalidate the ATC.
		 */
		if (! pte_wired_p(opte)) {
			break;
		}
		npte = opte & ~PTE_WIRED;
		if (pte_update(ptep, opte, npte)) {
			pmap_stat_update(pmap, wired_count, -1);
			break;
		}
	}

	pmap_table_release(pmap, pt, NULL);

	PMAP_CRIT_EXIT();
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
	struct pmap_table *pt;
	pt_entry_t pte, *ptep;
	bool rv = false;

	if (__predict_false(pmap == pmap_kernel() &&
			    va >= kernel_virtual_end)) {
		return false;
	}

	PMAP_CRIT_ENTER();

	ptep = pmap_pte_lookup(pmap, va, &pt);
	if (__predict_true(ptep != NULL)) {
		pte = pte_load(ptep);
		if (__predict_true(pte_valid_p(pte))) {
			if (__predict_true(pap != NULL)) {
				*pap = pte_pa(pte) | (va & PGOFSET);
			}
			if (__predict_false(flagsp != NULL)) {
				*flagsp =
				    (pte_wired_p(pte) ? PMAP_WIRED : 0) |
				    (pte_ci_p(pte) ? PMAP_NOCACHE : 0);
			}
			rv = true;
		}
		pmap_table_release(pmap, pt, NULL);
	}

	PMAP_CRIT_EXIT();

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
	 * Because the kernel has a separate root pointer, we don't
	 * need to activate the kernel pmap.
	 */
	if (pmap != pmap_kernel()) {
		PMAP_CRIT_ENTER();
		pmap_load_urp(pmap->pm_lev1pa);
		PMAP_CRIT_EXIT();
	}
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
	/* No action necessary in this pmap implementation. */
}

static vaddr_t pmap_tmpmap_srcva;
static vaddr_t pmap_tmpmap_dstva;

/*
 * pmap_zero_page:		[ INTERFACE ]
 *
 *	Zero the specified VM page by mapping the page into the kernel
 *	and using memset() (or equivalent) to clear its contents.
 */
void
pmap_zero_page(paddr_t pa)
{
	const int flags = MMU_IS_HP_CLASS ? PMAP_NOCACHE|PMAP_WIRED
					  : PMAP_WIRED;
	pt_entry_t * const dst_ptep = pmap_kernel_pte(pmap_tmpmap_dstva);

	/* Build the new PTE. */
	const pt_entry_t dst_pte =
	    pmap_make_pte(pa, UVM_PROT_READ | UVM_PROT_WRITE, flags);

	/* Set the new PTE. */
	KASSERT(! pte_valid_p(pte_load(dst_ptep)));
	pte_store(dst_ptep, dst_pte);

	/* Zero the page. */
	zeropage((void *)pmap_tmpmap_dstva);

	/* Invalidate the PTEs. */
	pte_store(dst_ptep, 0);
	TBIS(pmap_tmpmap_dstva);
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
	const int flags = MMU_IS_HP_CLASS ? PMAP_NOCACHE|PMAP_WIRED
					  : PMAP_WIRED;
	pt_entry_t * const src_ptep = pmap_kernel_pte(pmap_tmpmap_srcva);
	pt_entry_t * const dst_ptep = pmap_kernel_pte(pmap_tmpmap_dstva);

	/* Build the new PTEs. */
	const pt_entry_t src_pte =
	    pmap_make_pte(src, UVM_PROT_READ, flags);
	const pt_entry_t dst_pte =
	    pmap_make_pte(dst, UVM_PROT_READ | UVM_PROT_WRITE, flags);

	/* Set the new PTEs. */
	KASSERT(! pte_valid_p(pte_load(src_ptep)));
	pte_store(src_ptep, src_pte);
	KASSERT(! pte_valid_p(pte_load(dst_ptep)));
	pte_store(dst_ptep, dst_pte);

	/* Copy the page. */
	copypage((void *)pmap_tmpmap_srcva, (void *)pmap_tmpmap_dstva);

	/* Invalidate the PTEs. */
	pte_store(src_ptep, 0);
	TBIS(pmap_tmpmap_srcva);
	pte_store(dst_ptep, 0);
	TBIS(pmap_tmpmap_dstva);
}

/*
 * pmap_clear_modify:		[ INTERFACE ]
 *
 *	Clear the modify bits on the specified physical page.
 */
/* See <machine/pmap.h> */

/*
 * pmap_clear_reference:	[ INTERFACE ]
 *
 *	Clear the reference bit on the specified physical page.
 */
/* See <machine/pmap.h> */

/*
 * pmap_is_referenced:		[ INTERFACE ]
 *
 *	Return whether or not the specified physical page has been referenced
 *	by any physical maps.
 */
/* See <machine/pmap.h> */

/*
 * pmap_is_modified:		[ INTERFACE ]
 *
 *	Return whether or not the specified physical page has been modified
 *	by any physical maps.
 */
/* See <machine/pmap.h> */

/*
 * pmap_testbit:
 *
 *	Test the modified / referenced bits of a physical page.
 */
bool
pmap_testbit(struct vm_page *pg, pt_entry_t bit)
{
	struct pv_entry *pv;
	pt_entry_t pte = 0;
	bool rv = false;

	PMAP_CRIT_ENTER();

	if (VM_MDPAGE_UM(pg) & bit) {
		rv = true;
		goto out;
	}

	for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
		pte |= pte_load(pmap_pv_pte(pv));
		if (pte & bit) {
			rv = true;
			break;
		}
	}
	VM_MDPAGE_ADD_UM(pg, pte);
 out:
	PMAP_CRIT_EXIT();

	return rv;
}

/*
 * pmap_changebit:
 *
 *	Test-and-change various bits (including mod/ref bits).
 */
bool
pmap_changebit(struct vm_page *pg, pt_entry_t set, pt_entry_t mask)
{
	struct pv_entry *pv;
	pt_entry_t *ptep, combined_pte = 0, diff, opte, npte;
	bool rv = false;

#if MMU_CONFIG_68040_CLASS
	/*
	 * If we're making the page read-only or changing the caching
	 * status of the page, we need to flush it the first time we
	 * change a mapping.
	 */
	bool cflush_040;
	if (MMU_IS_68040_CLASS &&
	    ((set & PTE_WP) != 0 ||
	     (set & PTE_CMASK) != 0 ||
	     (mask & PTE_CMASK) == 0)) {
		cflush_040 = true;
	} else {
		cflush_040 = false;
	}
#endif

	PMAP_CRIT_ENTER();

	/*
	 * Since we're running over every mapping for the page anyway,
	 * we might as well synchronize any attribute bits that we're
	 * not clearing.
	 */
	for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
		for (;;) {
			ptep = pmap_pv_pte(pv);
			opte = pte_load(ptep);
			npte = (opte | set) & mask;
			if ((diff = (opte ^ npte)) == 0) {
				break;
			}
#if MMU_CONFIG_68040_CLASS
			if (__predict_false(cflush_040)) {
				paddr_t pa = VM_PAGE_TO_PHYS(pg);
				DCFP(pa);
				ICPP(pa);
				cflush_040 = false;
			}
#endif
			if (pte_update(ptep, opte, npte)) {
				rv = true;
				break;
			}
			/* Lost race, try again. */
		}
		combined_pte |= opte;
		if ((diff & PTE_CRIT_BITS) != 0 && active_pmap(pv->pv_pmap)) {
			TBIS(PV_VA(pv));
		}
	}

	/*
	 * Update any attributes we looked at, clear the ones we're clearing.
	 */
	VM_MDPAGE_SET_UM(pg,
	    (VM_MDPAGE_UM(pg) | combined_pte | set) & mask);

	PMAP_CRIT_EXIT();

	return rv;
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
	return m68k_ptob(cookie);
}

static pt_entry_t *kernel_lev1map;

/*
 * pmap_growkernel_alloc_page:
 *
 *	Helper for pmap_growkernel().
 */
static paddr_t
pmap_growkernel_alloc_page(void)
{
	/*
	 * XXX Needs more work if we're going to do this during
	 * XXX early bootstrap.
	 */
	if (! uvm.page_init_done) {
		panic("%s: called before UVM initialized", __func__);
	}

	struct vm_page *pg = pmap_page_alloc(true/*nowait*/);
	if (pg == NULL) {
		panic("%s: out of memory", __func__);
	}

	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	pmap_zero_page(pa);
#if MMU_CONFIG_68040_CLASS
	if (MMU_IS_68040_CLASS) {
		DCFP(pa);
	}
#endif
	return pa;
}

/*
 * pmap_growkernel_link_kptpage:
 *
 *	Helper for pmap_growkernel().
 */
static void
pmap_growkernel_link_kptpage(vaddr_t va, paddr_t ptp_pa)
{
	/*
	 * This is trivial for the 2-level MMU configuration.
	 */
	if (MMU_USE_2L) {
		KASSERT((kernel_lev1map[LA2L_RI(va)] & DT51_SHORT) == 0);
		kernel_lev1map[LA2L_RI(va)] = pmap_ste_proto | ptp_pa;
		return;
	}

	/*
	 * N.B. pmap_zero_page() is used in this process, which
	 * uses pmap_tmpmap_dstva.  pmap_tmpmap_srcva is available
	 * for our use, however, so that's what we used to temporarily
	 * map inner segment table pages.
	 */
	const vaddr_t stpg_va = pmap_tmpmap_srcva;

	paddr_t stpa, stpg_pa, stpgoff, last_stpg_pa = (paddr_t)-1;
	paddr_t pa = ptp_pa, end_pa = ptp_pa + PAGE_SIZE;
	pt_entry_t *stes;

	for (; pa < end_pa; va += NBSEG3L, pa += TBL40_L3_SIZE) {
		if ((kernel_lev1map[LA40_RI(va)] & UTE40_RESIDENT) == 0) {
			/* Level-2 table for this segment needed. */
			if (kernel_stnext_pa == kernel_stnext_endpa) {
				/*
				 * No more slots left in the last page
				 * we allocated for segment tables.  Grab
				 * another one.
				 */
				kernel_stnext_pa = pmap_growkernel_alloc_page();
				kernel_stnext_endpa =
				    kernel_stnext_pa + PAGE_SIZE;
				pmap_nkstpages_current_ev.ev_count++;
			}
			kernel_lev1map[LA40_RI(va)] =
			    pmap_ste_proto | kernel_stnext_pa;
			kernel_stnext_pa += TBL40_L2_SIZE;
		}
		stpa = kernel_lev1map[LA40_RI(va)] & UTE40_PTA;
		stpg_pa = m68k_trunc_page(stpa);
		if (stpg_pa != last_stpg_pa) {
			if (last_stpg_pa != (paddr_t)-1) {
				pmap_kremove(stpg_va, PAGE_SIZE);
			}
			pmap_kenter_pa(stpg_va, stpg_pa,
			    UVM_PROT_READ | UVM_PROT_WRITE,
			    PMAP_WIRED | PMAP_NOCACHE);
			last_stpg_pa = stpg_pa;
		}
		stpgoff = stpa - stpg_pa;
		stes = (pt_entry_t *)(stpg_va + stpgoff);
		stes[LA40_PI(va)] = pmap_ste_proto | pa;
	}
	if (last_stpg_pa != (paddr_t)-1) {
		pmap_kremove(stpg_va, PAGE_SIZE);
	}
}

/*
 * pmap_growkernel:		[ INTERFACE ]
 *
 *	Grow the kernel address space.  This is a hint from the
 *	upper layer to pre-allocate more kernel PT pages.
 */
vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	PMAP_CRIT_ENTER();

	KASSERT((kernel_virtual_end & PTPAGEVAOFS) == 0);

	/*
	 * We first calculate how many leaf tables are required
	 * to map up to the requested max address.  Even if we're
	 * on a 68040 or 68060, we allocate leaf tables in whole
	 * pages to simplify the logic (as was done in pmap_bootstrap1()).
	 */
	vaddr_t new_maxkva = pmap_round_ptpage(maxkvaddr);
	if (new_maxkva < kernel_virtual_end) {
		/*
		 * Great news!  We already have what we need to map
		 * the requested max address.  This happens one during
		 * early bootstrap before UVM's notion of "maxkvaddr"
		 * has been initialized.
		 */
		new_maxkva = kernel_virtual_end;
		goto done;
	}

	if (new_maxkva > kernel_virtual_max) {
		panic("%s: out of kernel VA space (req=0x%08lx limit=0x%08lx)",
		    __func__, maxkvaddr, kernel_virtual_max);
	}

	/*
	 * Allocate PT pages and link them into the MMU tree as we
	 * go.
	 */
	vaddr_t va, ptp_pa;
	for (va = kernel_virtual_end; va < new_maxkva; va += PTPAGEVASZ) {
		/* Allocate page and link it into the MMU tree. */
		ptp_pa = pmap_growkernel_alloc_page();
		pmap_growkernel_link_kptpage(va, ptp_pa);
		pmap_nkptpages_current_ev.ev_count++;

		/* Map the PT page into the kernel PTE array. */
		pmap_kenter_pa((vaddr_t)pmap_kernel_pte(va),
		    ptp_pa, UVM_PROT_READ | UVM_PROT_WRITE,
		    PMAP_WIRED | PMAP_NOCACHE);
	}
 	kernel_virtual_end = new_maxkva;
 done:
	pmap_maxkva_ev.ev_count32 = new_maxkva;
	pmap_kvalimit_ev.ev_count32 = kernel_virtual_max;
	PMAP_CRIT_EXIT();
	return new_maxkva;
}

/*
 * pmap_prefer:			[ INTERFACE ]
 *
 *	Attempt to arrange for pages at a given VM object offset
 *	to occupy the same virtually-addressed cache footprint
 *	in order to avoid cache aliases.
 */
#if MMU_CONFIG_HP_CLASS
static struct evcnt pmap_prefer_nochange_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap prefer", "nochange");
static struct evcnt pmap_prefer_change_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap prefer", "change");

EVCNT_ATTACH_STATIC(pmap_prefer_change_ev);
EVCNT_ATTACH_STATIC(pmap_prefer_nochange_ev);
#endif
void
pmap_prefer(vaddr_t hint, vaddr_t *vap, int td)
{
#if MMU_CONFIG_HP_CLASS
	if (MMU_IS_HP_CLASS) {
		vaddr_t va = *vap;
		ptrdiff_t diff = (hint - va) & pmap_aliasmask;

		if (diff == 0) {
			pmap_prefer_nochange_ev.ev_count++;
		} else {
			pmap_prefer_change_ev.ev_count++;
			if (__predict_false(td)) {
				va -= pmap_aliasmask + 1;
			}
			*vap = va + diff;
		}
	}
#endif
}

/*
 * pmap_procwr:			[ INTERFACE ]
 *
 *	Perform any cache synchronization required after writing
 *	to a process's address space.
 */
void
pmap_procwr(struct proc *p, vaddr_t va, size_t len)
{
	/*
	 * This is just a wrapper around the "cachectl" machdep
	 * system call.
	 *
	 * XXX This is kind of gross, to be honest.
	 */
	(void)cachectl1(0x80000004, va, len, p);
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
	struct gen68k_kcore_hdr *m = &h->un._gen68k;

	memset(h, 0, sizeof(*h));

	/*
	 * Initialize the `dispatcher' portion of the header.
	 */
	strcpy(h->name, "gen68k");
	h->page_size = PAGE_SIZE;
	h->kernbase = VM_MIN_KERNEL_ADDRESS;

	/*
	 * Fill in information about our MMU configuration.
	 *
	 * We essentially pretend to be a 68851 as far as table-
	 * walks are concerned.
	 *
	 * We provide the kernel's MMU_* constant so that the TT
	 * registers can be interpreted correctly.
	 */
	m->mmutype = mmutype;
	m->tcr = MMU_USE_3L ? MMU51_3L_TCR_BITS : MMU51_TCR_BITS;
	m->srp[0] = MMU51_SRP_BITS;
	m->srp[1] = Sysseg_pa;

#if MMU_CONFIG_68040_CLASS
	if (MMU_IS_68040_CLASS) {
		m->itt0 = mmu_tt40[MMU_TTREG_ITT0];
		m->itt1 = mmu_tt40[MMU_TTREG_ITT1];
		m->tt0  = mmu_tt40[MMU_TTREG_DTT0];
		m->tt1  = mmu_tt40[MMU_TTREG_DTT1];
	}
#endif
#if defined(M68K_MMU_68030)
	if (mmutype == MMU_68030) {
		m->tt0  = mmu_tt30[MMU_TTREG_TT0];
		m->tt1  = mmu_tt30[MMU_TTREG_TT1];
	}
#endif

	return m->ram_segs;
}

/***************************** PMAP BOOTSTRAP ********************************/

/*
 * The kernel virtual address space layout that this implementation is tuned
 * for assumes that KVA space begins at $0000.0000, that the static kernel
 * image (text/data/bss, etc.) resides at or near the bottom of this space,
 * and that all additional KVA that's mapped by PTEs grows upwards from there.
 *
 * Regions mapped by Transparent Translation registers (68030 and up)
 * are assumed to lie beyond where the KVA space is expected to grow.  When
 * we encounter these regions in the machine_bootmap[] (represented by a
 * KEEPOUT entry), we clamp the maximum KVA to prevent its growth into that
 * region.  The TT mechanism is not terribly precise, and only supports
 * VA==PA mappings, so it's only really suitable for device regions that
 * are in the upper reaches of the physical address space (at or beyond 1GB
 * or so).
 *
 * This implementation certainly could be adjusted to work with other address
 * space layouts, but the assumption asserted here is a bit baked-in.
 */
__CTASSERT(VM_MIN_KERNEL_ADDRESS == 0);

/*
 * The virtual kernel PTE array covers the entire 4GB kernel supervisor
 * address space, but is sparsely populated.  The amount of VA space required
 * for this linear array is:
 *
 *	(4GB / PAGE_SIZE) * sizeof(pt_entry_t)
 * -or-
 *	4KB: 4MB (1024 pages)
 *	8KB: 2MB (512 pages)
 *
 * To avoid doing 64-bit math, we calculate it like so:
 *
 *	((0xffffffff >> PGSHIFT) + 1) * sizeof(pt_entry_t)
 *
 * The traditional name for this virtual array is "Sysmap".
 */
#define	SYSMAP_VA_SIZE	(((0xffffffffU >> PGSHIFT) + 1) * sizeof(pt_entry_t))

/*
 * In the Hibler/Utah pmap, the kernel PTE array was placed right near
 * the very top of the kernel virtual address space.  This was because
 * of the hp300's unique physical memory arrangement: the last page of
 * memory is always located at PA $FFFF.F000 and the physical address
 * of the beginning of RAM varied based on the RAM size.  This meant that
 * VA $FFFF.F000 is a convenient place to map the RAM VA==PA, making
 * transition between "MMU off" and "MMU on" (and vice versa) easier.
 * Since VA $FFFF.F000 was already going to be mapped, it made sense to
 * put something else along side of it in order to minimize waste in
 * PT pages.
 *
 * As noted above, this implementation is tuned for a growing-from-0
 * virtual space layout.  However, we have a special case for this
 * particular requirement: if a platform defines SYSMAP_VA, then we
 * will assume it is as a high address, place the kernel PTE array at
 * that KVA, and ensure sufficient page tables to map from that VA until
 * the very end of the 4GB supervisor address space.  These tables will
 * be allocated before the machine_bootmap[] is processed to map physical
 * addresses, thus allowing the machine_bootmap[] use it to map physical
 * addresses into one of these high virtual addresses if necessary.  The
 * beginning of this region will also serve to clamp the maximum kernel
 * virtual address, in the same way as a KEEPOUT region in machine_bootmap[].
 *
 * For reference, the traditional hp300 definition is:
 *
 *	#define	SYSMAP_VA	((vaddr_t)(0-PAGE_SIZE*NPTEPG*2))
 *
 * ...and because the hp300 always used a 4KB page size (restriction
 * of HP MMU), this is: 0 - 4096*1024*2
 *                   -> 0 - 8388608 (8MB)
 *                   -> $FF80.0000
 *
 * Unfortunately (for the hp300), this means 2 PT pages for the top of
 * the address space (in the 2-level case), but that's unavoidable anyway
 * because of the last page being a separate mapping and the kernel PTE
 * array needs 4MB of space on its own.
 */

static vaddr_t	lwp0uarea;
       char *	vmmap;
       void *	msgbufaddr;

/* XXX Doesn't belong here. */
paddr_t		avail_start;	/* PA of first available physical page */
paddr_t		avail_end;	/* PA of last available physical page */

extern char *	kernel_text;
extern char *	etext;

/*
 * pmap_bootstrap1:
 *
 *	Phase 1 of bootstrapping virtual memory.  This is called before
 *	the MMU is enabled to set up the initial kernel MMU tables and
 *	allocate other important data structures.
 *
 *	Because the MMU has not yet been turned on, and we don't know if
 *	we're running VA==PA, we have to manually relocate all global
 *	symbol references.
 *
 *	Arguments:	nextpa		Physical address immediately
 *					following the kernel / symbols /
 *					etc.  This will be page-rounded
 *					before use.
 *
 *			reloff		VA<->PA relocation offset
 *
 *	Returns:	nextpa		Updated value after all of the
 *					allocations performed.
 */
paddr_t __attribute__((no_instrument_function))
pmap_bootstrap1(paddr_t nextpa, paddr_t reloff)
{
	paddr_t lwp0upa, stnext_endpa, stnext_pa;
	paddr_t pa, kernimg_endpa, kern_lev1pa;
	vaddr_t va, nextva, kern_lev1va;
	pt_entry_t *ptes, *pte, *epte;
	int entry_count = 0;

#ifdef SYSMAP_VA
#define	NRANGES		2
#else
#define	NRANGES		1
#endif

	struct va_range {
		vaddr_t start_va;
		vaddr_t end_va;
		paddr_t start_ptp;
		paddr_t end_ptp;
	} va_ranges[NRANGES], *var;
	int r;

#define	VA_IN_RANGE(va, var)				\
	((va) >= (var)->start_va &&			\
	 ((va) < (var)->end_va || (var)->end_va == 0))

#define	PA_TO_VA(pa)	(VM_MIN_KERNEL_ADDRESS + ((pa) - reloff))
#define	VA_TO_PA(va)	((((vaddr_t)(va)) - VM_MIN_KERNEL_ADDRESS) + reloff)
#define	RELOC(v, t)	*((t *)VA_TO_PA(&(v)))

	/*
	 * First determination we have to make is our configuration:
	 * Are we using a 2-level or 3-level table?  For the purposes
	 * of bootstrapping the kernel, it's "68040-class" and "other",
	 * the former getting the 3-level table.
	 */
	const bool is_68040_class = RELOC(mmutype, int) == MMU_68040 ||
				    RELOC(mmutype, int) == MMU_68060;
	const bool use_3l = is_68040_class;

	/*
	 * Based on MMU class, figure out what the constant values of
	 * segment / page table entries look like.
	 *
	 * See pmap_pte_proto_init().
	 */
	pt_entry_t proto_ro_pte;	/* read-only */
	pt_entry_t proto_rw_pte;	/* read-write */
	pt_entry_t proto_rw_ci_pte;	/* read-write, cache-inhibited */
	pt_entry_t proto_ste;

	if (is_68040_class) {
		proto_ro_pte    = PTE_VALID|PTE_WIRED|PTE_WP|PTE40_CM_WT;
		proto_rw_pte    = PTE_VALID|PTE_WIRED       |PTE40_CM_CB;
		proto_rw_ci_pte = PTE_VALID|PTE_WIRED       |PTE40_CM_NC_SER;
	} else {
		proto_ro_pte    = PTE_VALID|PTE_WIRED|PTE_WP;
		proto_rw_pte    = PTE_VALID|PTE_WIRED;
		proto_rw_ci_pte = PTE_VALID|PTE_WIRED       |PTE51_CI;
	}
	proto_ste = DTE51_U | DT51_SHORT;

	/*
	 * Allocate some important fixed virtual (and physical) addresses.
	 * We use the sum total of this initial mapped kernel space to
	 * determine how many inital kernel PT pages to allocate.  The
	 * things that consume physical space will come first, and the
	 * virtual-space-{only,mostly} things come at the end.
	 *
	 *	lwp0upa		lwp0 u-area	USPACE	(p)
	 *	lwp0uarea				(v)
	 *
	 *	Sysseg_pa	kernel lev1map	PAGE_SIZE (p)
	 *	kernel_lev1map			PAGE_SIZE (v, ci)
	 *
	 *	null_segtab_pa	null segtab	PAGE_SIZE (p)
	 *
	 *	tmpmap_srcva	temp map, src	PAGE_SIZE (v)
	 *	tmpmap_dstva	temp map, dst	PAGE_SIZE (v)
	 *
	 *	vmmap		ya tmp map	PAGE_SIZE (v)
	 *
	 *	msgbufaddr	kernel msg buf	round_page(MSGBUFSIZE) (v)
	 *
	 *	kernel_ptes	kernel PTEs	SYSMAP_VA_SIZE (v, ci)
	 *					(see comments above)
	 *
	 * When we allocate the kernel lev1map, for the 2-level
	 * configuration, there is no inner segment tables to allocate,
	 * the leaf PT pages get poked directly into the level-1 table.
	 *
	 * In the 3-level configuration, to map all of the leaf tables,
	 * inner segment table pages are allocated as necessary.  We
	 * first take those tables from the page containing the level-1
	 * table, and allocate additional pages as necessary.
	 */

	nextpa = m68k_round_page(nextpa);
	nextva = PA_TO_VA(nextpa);

	/*
	 * nextpa now represents the end of the loaded kernel image.
	 * This includes the .data + .bss segments, the debugger symbols,
	 * and any other ancillary data loaded after the kernel.
	 *
	 * N.B. This represents the start of our dynamic memory allocation,
	 * which will be referenced below when we zero the memory we've
	 * allocated.
	 */
	kernimg_endpa = nextpa;

	/*
	 * lwp0 u-area.  We allocate it here, and finish setting it
	 * up in pmap_bootstrap2().
	 */
	lwp0upa = nextpa;
	nextpa += USPACE;
	RELOC(lwp0uarea, vaddr_t) = nextva;
	nextva += USPACE;

	size_t nstpages = 0;

	/* kernel level-1 map */
	RELOC(Sysseg_pa, paddr_t) = kern_lev1pa = nextpa;
	nextpa += PAGE_SIZE;
	RELOC(kernel_lev1map, vaddr_t) = kern_lev1va = nextva;
	nextva += PAGE_SIZE;
	nstpages++;

	/*
	 * For 3-level configs, we now have space to allocate
	 * inner segment tables.
	 */
	stnext_pa = kern_lev1pa + TBL40_L1_SIZE;
	stnext_endpa = m68k_round_page(stnext_pa);

	/* null segment table */
	RELOC(null_segtab_pa, paddr_t) = nextpa;
	nextpa += PAGE_SIZE;

	/* pmap temporary map addresses */
	RELOC(pmap_tmpmap_srcva, vaddr_t) = nextva;
	nextva += PAGE_SIZE;
	RELOC(pmap_tmpmap_dstva, vaddr_t) = nextva;
	nextva += PAGE_SIZE;

	/* vmmap temporary map address */
	RELOC(vmmap, vaddr_t) = nextva;
	nextva += PAGE_SIZE;

	/* kernel message buffer */
	RELOC(msgbufaddr, vaddr_t) = nextva;
	nextva += m68k_round_page(MSGBUFSIZE);

	/* Kernel PTE array. */
#ifdef SYSMAP_VA
	if ((vaddr_t)SYSMAP_VA < RELOC(kernel_virtual_max, vaddr_t)) {
		RELOC(kernel_virtual_max, vaddr_t) = (vaddr_t)SYSMAP_VA;
	}
	RELOC(kernel_ptes, vaddr_t) = (vaddr_t)SYSMAP_VA;
	va_ranges[1].start_va = (vaddr_t)SYSMAP_VA;
	va_ranges[1].end_va = 0; /* until the end of the address space */
#else
	RELOC(kernel_ptes, vaddr_t) = nextva;
	nextva += SYSMAP_VA_SIZE;
#endif /* SYSMAP_VA */

#ifdef __HAVE_MACHINE_BOOTMAP
	/*
	 * Allocate machine-specific VAs.
	 */
	extern const struct pmap_bootmap machine_bootmap[];
	const struct pmap_bootmap *pmbm =
	    (const struct pmap_bootmap *)VA_TO_PA(machine_bootmap);
	for (; pmbm->pmbm_vaddr != (vaddr_t)-1; pmbm++) {
		if (pmbm->pmbm_flags & (PMBM_F_FIXEDVA | PMBM_F_KEEPOUT)) {
			va = m68k_trunc_page(pmbm->pmbm_vaddr);
			if (va < RELOC(kernel_virtual_max, vaddr_t)) {
				RELOC(kernel_virtual_max, vaddr_t) = va;
			}
		} else {
			*(vaddr_t *)VA_TO_PA(pmbm->pmbm_vaddr_ptr) = nextva;
			nextva += m68k_round_page(pmbm->pmbm_size);
		}
	}
#endif /* __HAVE_MACHINE_BOOTMAP */

	/* UVM-managed kernel virtual starts here. */
	RELOC(kernel_virtual_start, vaddr_t) = nextva;

	/*
	 * Allocate enough PT pages to map all of physical memory.
	 * This should be sufficient to prevent pmap_growkernel()
	 * from having to do any work before the VM system is set
	 * up.
	 */
	nextva += RELOC(physmem, u_int) << PGSHIFT;
	nextva = pmap_round_ptpage(nextva);
	if (nextva > RELOC(kernel_virtual_max, vaddr_t) ||
	    nextva < RELOC(kernel_virtual_start, vaddr_t)) {
		/* clamp it. */
		nextva = RELOC(kernel_virtual_max, vaddr_t);
	}

	/*
	 * This marks the end of UVM-managed kernel virtual space,
	 * until such time as pmap_growkernel() is called to expand
	 * it.
	 */
	va_ranges[0].start_va = VM_MIN_KERNEL_ADDRESS;
	va_ranges[0].end_va = nextva;
	RELOC(kernel_virtual_end, vaddr_t) = nextva;

	/*
	 * Now, compute the number of PT pages required to map the
	 * required VA ranges and allocate them.
	 */
	size_t nptpages, total_ptpages = 0;
	for (r = 0; r < NRANGES; r++) {
		var = &va_ranges[r];
		nptpages = (var->end_va - var->start_va) / PTPAGEVASZ;
		var->start_ptp = nextpa;
		nextpa += nptpages * PAGE_SIZE;
		var->end_ptp = nextpa;
		total_ptpages += nptpages;
	}

	/*
	 * The bulk of the dynamic memory allocation is done (there
	 * may be more below if we have to allocate more inner segment
	 * table pages, but we'll burn that bridge when we come to it).
	 *
	 * Zero out all of these freshly-allocated pages.
	 */
	pte = (pt_entry_t *)kernimg_endpa;
	while ((paddr_t)pte < nextpa) {
		*pte++ = 0;
	}

	/*
	 * Ok, let's get to mapping stuff!  Almost everything is in
	 * the first VA range.
	 */
	ptes = (pt_entry_t *)va_ranges[0].start_ptp;

	/* Kernel text - read-only. */
	pa = VA_TO_PA(m68k_trunc_page(&kernel_text));
	pte = &ptes[m68k_btop(&kernel_text)];	/* btop implies trunc_page */
	epte = &ptes[m68k_btop(&etext)];
	while (pte < epte) {
		*pte++ = proto_ro_pte | pa;
		pa += PAGE_SIZE;
		entry_count++;
	}

	/* Remainder of kernel image - read-write. */
	epte = &ptes[m68k_btop(PA_TO_VA(kernimg_endpa))];
	while (pte < epte) {
		*pte++ = proto_rw_pte | pa;
		pa += PAGE_SIZE;
		entry_count++;
	}

	/* lwp0 u-area - read-write. */
	pa = lwp0upa;
	pte = &ptes[m68k_btop(RELOC(lwp0uarea, vaddr_t))];
	epte = &ptes[m68k_btop(RELOC(lwp0uarea, vaddr_t) + USPACE)];
	while (pte < epte) {
		*pte++ = proto_rw_pte | pa;
		pa += PAGE_SIZE;
		entry_count++;
	}

	/* Kernel lev1map - read-write, cache-inhibited. */
	pte = &ptes[m68k_btop(kern_lev1va)];
	*pte = proto_rw_ci_pte | kern_lev1pa;
	entry_count++;

	/* Kernel leaf PT pages - read-write, cache-inhibited. */
	va = RELOC(kernel_ptes, vaddr_t);
	pt_entry_t *kptes = (pt_entry_t *)va;
	struct va_range *kpt_var = NULL;
	for (r = 0; r < NRANGES; r++) {
		kpt_var = &va_ranges[r];
		if (VA_IN_RANGE(va, kpt_var)) {
			break;
		}
	}
	ptes = (pt_entry_t *)kpt_var->start_ptp;

	for (r = 0; r < NRANGES; r++) {
		var = &va_ranges[r];
		va = (vaddr_t)(&kptes[m68k_btop(var->start_va)]);
		pte = &ptes[m68k_btop(va - kpt_var->start_va)];
		for (pa = var->start_ptp; pa < var->end_ptp; pa += PAGE_SIZE) {
			*pte++ = proto_rw_ci_pte | pa;
			entry_count++;
		}
	}

#ifdef __HAVE_MACHINE_BOOTMAP
	/*
	 * Now perform any machine-specific mappings at VAs
	 * allocated earlier.
	 */
	pmbm = (const struct pmap_bootmap *)VA_TO_PA(machine_bootmap);
	for (; pmbm->pmbm_vaddr != (vaddr_t)-1; pmbm++) {
		if (pmbm->pmbm_flags & (PMBM_F_VAONLY | PMBM_F_KEEPOUT)) {
			continue;
		}
		if (pmbm->pmbm_flags & PMBM_F_FIXEDVA) {
			va = pmbm->pmbm_vaddr;
		} else {
			va = *(vaddr_t *)VA_TO_PA(pmbm->pmbm_vaddr_ptr);
		}
		for (r = 0; r < NRANGES; r++) {
			var = &va_ranges[r];
			if (VA_IN_RANGE(va, var)) {
				break;
			}
		}
		ptes = (pt_entry_t *)var->start_ptp;
		pa = pmbm->pmbm_paddr;
		pte = &ptes[m68k_btop(va - var->start_va)];
		pt_entry_t proto = (pmbm->pmbm_flags & PMBM_F_CI) ?
		    proto_rw_ci_pte : proto_rw_pte;
		for (vsize_t size = m68k_round_page(pmbm->pmbm_size);
		     size != 0;
		     va += PAGE_SIZE, pa += PAGE_SIZE, size -= PAGE_SIZE) {
			*pte++ = proto | pa;
			entry_count++;
		}
	}
#endif /* __HAVE_MACHINE_BOOTMAP */

	/*
	 * Now that all of the invidual VAs are mapped in the leaf
	 * tables, it's time to link those tables into the segment
	 * table.
	 *
	 * For the 2-level case, this is trivial.  For the 3-level
	 * case, we will have to allocate inner segment tables.
	 */
	for (r = 0; r < NRANGES; r++) {
		var = &va_ranges[r];
		if (use_3l) {
			pt_entry_t *stes, *stes1 = (pt_entry_t *)kern_lev1pa;
			for (va = var->start_va, pa = var->start_ptp;
			     pa < var->end_ptp;
			     va += NBSEG3L, pa += TBL40_L3_SIZE) {
				unsigned int ri = LA40_RI(va);
				if ((stes1[ri] & UTE40_RESIDENT) == 0) {
					/*
					 * Level-2 table for this segment
					 * needed.
					 */
					if (stnext_pa == stnext_endpa) {
						/*
						 * No more slots left in the
						 * last page we allocated for
						 * segment tables.  Grab
						 * another one.
						 */
						stnext_pa = nextpa;
						nextpa += PAGE_SIZE;
						stnext_endpa = nextpa;
						nstpages++;
						/*
						 * Zero out the new inner
						 * segment table page.
						 */
						pte = (pt_entry_t *)stnext_pa;
						while ((vaddr_t)pte <
								stnext_endpa) {
							*pte++ = 0;
						}
					}
					stes1[ri] = proto_ste | stnext_pa;
					stnext_pa += TBL40_L2_SIZE;
				}
				stes = (pt_entry_t *)
				    (uintptr_t)(stes1[ri] & UTE40_PTA);
				stes[LA40_PI(va)] = proto_ste | pa;
			}
		} else {
			pt_entry_t *stes = (pt_entry_t *)kern_lev1pa;
			for (va = var->start_va, pa = var->start_ptp;
			     pa < var->end_ptp;
			     va += NBSEG2L, pa += PAGE_SIZE) {
				stes[LA2L_RI(va)] = proto_ste | pa;
			}
		}
	}

	/* Instrumentation. */
	RELOC(pmap_nkptpages_initial_ev.ev_count32, uint32_t) =
	RELOC(pmap_nkptpages_current_ev.ev_count32, uint32_t) = total_ptpages;
	RELOC(pmap_nkstpages_initial_ev.ev_count32, uint32_t) =
	RELOC(pmap_nkstpages_current_ev.ev_count32, uint32_t) = nstpages;

	/*
	 * Record the number of wired mappings we created above
	 * in the kernel pmap stats.
	 */
	RELOC(kernel_pmap_store.pm_stats.resident_count, long) = entry_count;
	RELOC(kernel_pmap_store.pm_stats.wired_count, long) = entry_count;

	/*
	 * Stash any left-over segment table space for use by
	 * pmap_growkernel() later.
	 */
	RELOC(kernel_stnext_pa, paddr_t) = stnext_pa;
	RELOC(kernel_stnext_endpa, paddr_t) = stnext_endpa;

	return nextpa;
}

#undef RELOC

/*
 * pmap_bootstrap2:
 *
 *	Phase 2 of bootstrapping virtual memory.  This is called after
 *	the MMU has been enabled to finish setting up run-time-computed
 *	global pmap data, plus the lwp0 u-area, curlwp, and curpcb.
 */
void *
pmap_bootstrap2(void)
{
	/* Setup the MMU class; needed before anything else. */
	pmap_mmuclass_init();

	/* Early low-level UVM initialization. */
	uvmexp.pagesize = NBPG;				/* XXX ick, NBPG */
	uvm_md_init();

	/* Initialize prototype PTEs; needed before anything else is mapped. */
	pmap_pte_proto_init();

	/* Initialize the kernel pmap. */
	pmap_pinit(pmap_kernel(), Sysseg_pa);

	/* Initialize lwp0 u-area, curlwp, and curpcb. */
	memset((void *)lwp0uarea, 0, USPACE);
	uvm_lwp_setuarea(&lwp0, lwp0uarea);
	curlwp = &lwp0;
	curpcb = lwp_getpcb(&lwp0);

	return (void *)lwp0uarea;
}
