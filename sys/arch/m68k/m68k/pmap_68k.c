/*	$NetBSD: pmap_68k.c,v 1.110 2026/11/22 15:14:20 thorpej Exp $	*/

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
 * This pmap module supports fine-grained locking for multiprocessor
 * systems.
 *
 * There are two data structures that require access serialization: the
 * pmap structures and the PV lists contained within the vm_page structures
 * ("pvh").
 *
 * ** pmap locking **
 * The general idea is that the pmap is locked for the duration of a
 * pmap operation.  For the most part, pmap operations are non-blocking.
 * and calls to memory allocators to free resources are deferred to
 * pmap_update() calls which happen outside the critical path.
 *
 * There is an exception, however: pmap_enter() frequently needs to
 * allocate resources to perform its work -- page tables and PV entries.
 * The code around these allocations is structured in such a way that if
 * the allocation may block, the pmap lock is dropped, the allocation
 * performed, and then the operation is restarted from the beginning.
 *
 * ** PV list locking **
 * PV lists are locked when mappings are entered or removed via the
 * usual map-oriented entry points (pmap_enter(), pmap_remove(), etc.),
 * also via the page-orgiented entry points (pmap_page_protect(),
 * pmap_clear_reference(), etc.).  In the former case, the locks are
 * acquired in the pmap -> pvh order.  In the latter case, the locks
 * are acquired in the pvh -> pmap order.
 *
 * In order to avoid a deadlock, we set forth that the canonical ordering
 * is pmap -> pvh (i.e. virtual -> physical).  This is the vast majority
 * of pmap interface calls.
 *
 * In oder to handle this situation, we have the following rules:
 *
 * ==> To /modify/ a PTE's attributes, you must hold /either/ the pmap lock
 *     or the pvh lock, and an atomic update to that PTE must be performed
 *     (this is generally handled with pte_update()).
 *
 * ==> To /invalidate/ a PTE and remove a mapping (including changing the
 *     PA mapped by the PTE), you must hold /both/ the pmap lock /and/ the
 *     pvh lock.
 *
 * ==> The canonocal locking order is pmap -> pvh.  pvh -> pmap must be
 *     done using try-locking with a recovery path.  The place where
 *     this matters is pmap_page_protect(), which might have to deal
 *     with PROT_NONE, in which case it needs to acquire a pmap's lock
 *     while holding the pvh lock for that page in order to remove all
 *     mappings of the page.
 *
 * Notably, pmap_testbit() and pmap_changebit() (used to implement
 * pmap_is_{modified,referenced}() and pmap_clear_{modify,reference}()
 * respectively) do not have to lock any pmaps at all.
 *
 * Also note: the kernel pmap's lock is not acquired in pmap_kenter_pa()
 * or pmap_kremove().  Those functions can be called from interrupt
 * context, and their mappings are explcitiy not PV-tracked per the API
 * contract.  Since the kernel pmap's PTEs are always made available
 * ahead of time and never freed, there is no need to serialize any
 * access within those functions.
 *
 * Now, as you might expect, there aren't exactly a lot of MP-capable
 * m68k systems just floating around out there.  And so, this code has
 * been structured such that, when built without the MULTIPROCESSOR
 * option, no memory or cpu-time costs are paid -- all of the calls and
 * initializations are compiled out and what remains is perfectly safe
 * to run on a uni-processor system.  The blocking-for-allocation handling
 * remains, however, since it's evert bit as relevant on a UP system as
 * an MP system.
 */

/*
 * Current status:
 * - Very stable multi-user on virt68k (qemu 68040; does not accurately
 *   model cache or ATC, but suitable for exercising large memory configs).
 *
 * - Very stable multi-user running memory pressure stress tests on
 *   small-memory configs in virt68k (qemu 68040 **with fixed MMU
 *   emulation**) and mac68k (10MB 68040, 68030).  Very stable on
 *   diskless next68k (32MB 68040).
 *
 * - Ports that have been adapted: cesfic (default), hp300 (default),
 *   luna68k (default), mac68k (default), mvme68k (default), news68k (default),
 *   next68k (default), virt68k (default), x68k.
 *
 * XXX TODO XXX
 *
 * - Adapt amiga (hard), atari (hard).
 * - Test on 68020 + 68851.  It turns out, this is a pretty rare
 *   combination to find out in the wild.  No problems are anticipated,
 *   however, since the 68030's MMU is essentially identical for our
 *   purposes.
 * - Test on real HP MMU.
 * - More rigorous testing in various emulators (Nono, UAE?).
 * - Convert '851 / '030 to 3-level (maybe?).
 * - Tune ATC batch thresholds for each MMU type.
 * - Add some more instrumentation.
 * - Eventually disable instrumentation by default.
 * - ...
 * - PROFIT!
 *
 * Oh, yah, and there's some work going on here to support multiprocessor
 * machines.  It's not ready yet, so don't get too excited.
 */

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_m68k_arch.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap_68k.c,v 1.110 2026/11/22 15:14:20 thorpej Exp $");

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
#include <m68k/mmu.h>

#if !defined(M68K_MMU_MOTOROLA) && !defined(M68K_MMU_HP)
#error Hit the road, Jack...
#endif

/****************************** SERIALIZATION ********************************/

/*
 * A NOTE ON V->P vs P->V LOCKING.
 *
 */

/**************************** MMU CONFIGURATION ******************************/

#include "opt_m68k_arch.h"

#if defined(M68K_MMU_68030)
#include <m68k/mmu_30.h>	/* for cpu_kcore_hdr_t */
#endif

#define	ATC_MAXVA	8	/* max ATC batch */

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

static struct evcnt pmap_nptpages_current_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap nptpages", "current");
static struct evcnt pmap_nptpages_hiwat_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap nptpages", "hiwat");
EVCNT_ATTACH_STATIC(pmap_nptpages_current_ev);
EVCNT_ATTACH_STATIC(pmap_nptpages_hiwat_ev);

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

static struct evcnt pmap_enter_nowait_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "nowait");
EVCNT_ATTACH_STATIC(pmap_enter_nowait_ev);

static struct evcnt pmap_enter_yeswait_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "yeswait");
EVCNT_ATTACH_STATIC(pmap_enter_yeswait_ev);

static struct evcnt pmap_enter_pte_alloc_fail_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "pte alloc failed");
EVCNT_ATTACH_STATIC(pmap_enter_pte_alloc_fail_ev);

static struct evcnt pmap_enter_pv_alloc_fail_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap enter", "pv alloc failed");
EVCNT_ATTACH_STATIC(pmap_enter_pv_alloc_fail_ev);

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

static struct evcnt pmap_atc_ic_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "ic all");
EVCNT_ATTACH_STATIC(pmap_atc_ic_ev);
static struct evcnt pmap_atc_dc_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "dc all");
EVCNT_ATTACH_STATIC(pmap_atc_dc_ev);
static struct evcnt pmap_atc_bc_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "bc all");
EVCNT_ATTACH_STATIC(pmap_atc_bc_ev);

#if MMU_CONFIG_HP_CLASS
static struct evcnt pmap_atc_vac_all_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "vac all");
EVCNT_ATTACH_STATIC(pmap_atc_vac_all_ev);
static struct evcnt pmap_atc_vac_super_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "vac super");
EVCNT_ATTACH_STATIC(pmap_atc_vac_super_ev);
static struct evcnt pmap_atc_vac_user_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "vac user");
EVCNT_ATTACH_STATIC(pmap_atc_vac_user_ev);
#endif

static struct evcnt pmap_atc_bc60inv_all_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "bc60 all");
EVCNT_ATTACH_STATIC(pmap_atc_bc60inv_all_ev);
static struct evcnt pmap_atc_bc60inv_user_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "bc60 user");
EVCNT_ATTACH_STATIC(pmap_atc_bc60inv_user_ev);

static struct evcnt pmap_atc_ic_pg_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "ic pg");
EVCNT_ATTACH_STATIC(pmap_atc_ic_pg_ev);
static struct evcnt pmap_atc_dc_pg_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "dc pg");
EVCNT_ATTACH_STATIC(pmap_atc_dc_pg_ev);
static struct evcnt pmap_atc_bc_pg_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "bc pg");
EVCNT_ATTACH_STATIC(pmap_atc_bc_pg_ev);

static struct evcnt pmap_atc_all_user_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "all user");
EVCNT_ATTACH_STATIC(pmap_atc_all_user_ev);
static struct evcnt pmap_atc_all_super_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "all super");
EVCNT_ATTACH_STATIC(pmap_atc_all_super_ev);
static struct evcnt pmap_atc_all_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "all");
EVCNT_ATTACH_STATIC(pmap_atc_all_ev);

static struct evcnt pmap_atc_req_all_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "req all");
EVCNT_ATTACH_STATIC(pmap_atc_req_all_ev);
static struct evcnt pmap_atc_req_saturate_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "req saturate");
EVCNT_ATTACH_STATIC(pmap_atc_req_saturate_ev);

static struct evcnt pmap_atc_one_user_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "one user");
EVCNT_ATTACH_STATIC(pmap_atc_one_user_ev);
static struct evcnt pmap_atc_one_super_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "one super");
EVCNT_ATTACH_STATIC(pmap_atc_one_super_ev);
static struct evcnt pmap_atc_one_skipped_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "one skipped");
EVCNT_ATTACH_STATIC(pmap_atc_one_skipped_ev);
static struct evcnt pmap_atc_dfc_switch_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "dfc switch");
EVCNT_ATTACH_STATIC(pmap_atc_dfc_switch_ev);

static struct evcnt pmap_atc_pv_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "pv");
EVCNT_ATTACH_STATIC(pmap_atc_pv_ev);
static struct evcnt pmap_atc_pv_skipped_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "pv skipped");
EVCNT_ATTACH_STATIC(pmap_atc_pv_skipped_ev);
static struct evcnt pmap_atc_pv_cpushp_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "pv cpushp");
EVCNT_ATTACH_STATIC(pmap_atc_pv_cpushp_ev);

__CTASSERT(ATC_MAXVA == 8);
static struct evcnt pmap_atc_0_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "batch 0");
EVCNT_ATTACH_STATIC(pmap_atc_0_ev);
static struct evcnt pmap_atc_1_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "batch 1");
EVCNT_ATTACH_STATIC(pmap_atc_1_ev);
static struct evcnt pmap_atc_2_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "batch 2");
EVCNT_ATTACH_STATIC(pmap_atc_2_ev);
static struct evcnt pmap_atc_3_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "batch 3");
EVCNT_ATTACH_STATIC(pmap_atc_3_ev);
static struct evcnt pmap_atc_4_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "batch 4");
EVCNT_ATTACH_STATIC(pmap_atc_4_ev);
static struct evcnt pmap_atc_5_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "batch 5");
EVCNT_ATTACH_STATIC(pmap_atc_5_ev);
static struct evcnt pmap_atc_6_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "batch 6");
EVCNT_ATTACH_STATIC(pmap_atc_6_ev);
static struct evcnt pmap_atc_7_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "batch 7");
EVCNT_ATTACH_STATIC(pmap_atc_7_ev);
static struct evcnt pmap_atc_8_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap atc", "batch 8");
EVCNT_ATTACH_STATIC(pmap_atc_8_ev);

#define	pmap_evcnt(e)		pmap_ ## e ## _ev.ev_count++
#else
#define	pmap_evcnt(e)		__nothing
#endif

#if MMU_CONFIG_HP_CLASS
static void pmap_atc_process_hp(void);
#endif
#if MMU_CONFIG_68851_CLASS
static void pmap_atc_process_51(void);
#endif
#if MMU_CONFIG_68040_CLASS
static void pmap_atc_process_40(void);
#endif

static void (*pmap_load_urp_func)(paddr_t) __read_mostly;
static void (*pmap_atc_process_local_func)(void) __read_mostly;

static void
pmap_mmuclass_init(void)
{
	switch (mmutype) {
#if MMU_CONFIG_68040_CLASS
	case MMU_68040:
	case MMU_68060:
		pmap_mmuclass = MMU_CLASS_68040;
		pmap_atc_process_local_func = pmap_atc_process_40;
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
		pmap_atc_process_local_func = pmap_atc_process_51;
		protorp[0] = MMU51_CRP_BITS;
		pmap_load_urp_func = mmu_load_urp51;
		break;
#endif
#if MMU_CONFIG_HP_CLASS
	case MMU_HP:
		pmap_mmuclass = MMU_CLASS_HP;
		pmap_atc_process_local_func = pmap_atc_process_hp;
		pmap_load_urp_func = mmu_load_urp20hp;
		break;
#endif
	default:
		panic("%s: mmutype=%d not configured?", __func__, mmutype);
	}

	if (pmap_atc_process_local_func == NULL) {
		panic("%s: No pmap_atc_process_*() for cputype=%d mmutype=%d",
		    __func__, cputype, mmutype);
	}

	if (pmap_load_urp_func == NULL) {
		panic("%s: No mmu_load_*() for cputype=%d mmutype=%d",
		    __func__, cputype, mmutype);
	}
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

static bool pmap_initialized_p __read_mostly;

static inline struct vm_page *
pmap_pa_to_pg(paddr_t pa)
{
	return pmap_initialized_p ? PHYS_TO_VM_PAGE(pa) : NULL;
}

static pt_entry_t pmap_changebit(struct vm_page *, pt_entry_t, pt_entry_t);

/*************************** RESOURCE MANAGEMENT *****************************/

static struct pmap kernel_pmap_store;
struct pmap * const kernel_pmap_ptr = &kernel_pmap_store;

/*
 * Physical address of kernel level 1 table.  This name is compatible
 * with the Hibler pmap's name.
 */
paddr_t		Sysseg_pa __read_mostly;

/*
 * Avoid a memory load when doing comparisons against pmap_kernel()
 * within this compilation unit.
 */
#undef pmap_kernel
#define	pmap_kernel()	(&kernel_pmap_store)

/*
 * Number of tables per page table page:
 * 0 - number of leaf page tables per page
 * 1 - number of segment tables per page
 */
static unsigned int pmap_ptpage_table_counts[2] __read_mostly;

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
	uint64_t ticket;
	struct vm_page *pg;
	const int flags = nowait ? UVM_PGA_USERESERVE : 0;

	while (ticket = uvm_wait_prepare(),
	    (pg = uvm_pagealloc(NULL, 0, NULL, flags)) == NULL) {
		if (nowait) {
			return NULL;
		}
		uvm_wait("pmappg", ticket);
	}
	pg->flags &= ~PG_BUSY;	/* never busy */

	return pg;
}

static vaddr_t
pmap_vpage_alloc(bool nowait, int flags, struct vm_page **pg_out)
{
	struct vm_page *pg;
	const int uvm_f_nowait = nowait ? UVM_KMF_NOWAIT : 0;
	vaddr_t pgva;

	/* Allocate a VA for the page. */
	pgva = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
			    UVM_KMF_VAONLY | uvm_f_nowait);
	if (__predict_false(pgva == 0)) {
		return 0;
	}

	/* Get a page. */
	pg = pmap_page_alloc(nowait);
	if (__predict_false(pg == NULL)) {
		uvm_km_free(kernel_map, pgva, PAGE_SIZE, UVM_KMF_VAONLY);
		return 0;
	}

	/* Map the page according to the flags and zero it out. */
	pmap_kenter_pa(pgva, VM_PAGE_TO_PHYS(pg),
	    UVM_PROT_READ | UVM_PROT_WRITE, flags);
	zeropage((void *)pgva);

	if (__predict_true(pg_out != NULL)) {
		*pg_out = pg;
	}

	return pgva;
}

static struct pmap_ptpage *
pmap_ptpage_alloc(pmap_t pmap, bool segtab, bool nowait)
{
	const unsigned int tabcnt = pmap_ptpage_table_counts[segtab];
	const size_t size = sizeof(struct pmap_ptpage) +
	    (sizeof(struct pmap_table) * tabcnt);
	const size_t tabsize = PAGE_SIZE / tabcnt;
	struct vm_page *pg;
	struct pmap_ptpage *ptp;
	struct pmap_table *pt;
	vaddr_t ptpva;

	/* First check to see if we have one in the garbage bin. */
	ptp = TAILQ_FIRST(&pmap->pm_garbage.g_ptpages);
	if (__predict_false(ptp != NULL)) {
		ptpva = m68k_ptob(ptp->ptp_vpagenum);
		pg = ptp->ptp_pg;
		goto gotit;
	}

	KASSERT(PMAP_LOCK_OWNED(pmap));
	PMAP_UNLOCK(pmap);

	ptp = kmem_zalloc(size, nowait ? KM_NOSLEEP : KM_SLEEP);
	if (__predict_false(ptp == NULL)) {
		goto fail;
	}

	/*
	 * Allocate a mapped page for the PT page.  PT pages
	 * are always mapped cache-inhibited.
	 */
	ptpva = pmap_vpage_alloc(nowait, PMAP_NOCACHE, &pg);
	if (ptpva == 0) {
		kmem_free(ptp, size);
		goto fail;
	}

	PMAP_LOCK(pmap);

	/*
	 * All resources for the PT page have been allocated.
	 * Now initialize it and the individual table descriptors.
	 */
 gotit:
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

#ifdef PMAP_EVENT_COUNTERS
	pmap_nptpages_current_ev.ev_count32++;
	if (pmap_nptpages_current_ev.ev_count32 >
	    pmap_nptpages_hiwat_ev.ev_count32) {
		pmap_nptpages_hiwat_ev.ev_count32 =
		    pmap_nptpages_current_ev.ev_count32;
	}
#endif

	return ptp;
 fail:
	PMAP_LOCK(pmap);
	return NULL;
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

#ifdef PMAP_EVENT_COUNTERS
	pmap_nptpages_current_ev.ev_count32--;
#endif
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
#ifdef PMAP_ENABLE_LOCKS
	    CACHELINE_SIZE,		/* cacheline-aligned if using locks */
#else
	    0,				/* align */
#endif
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
pmap_pv_alloc(pmap_t pmap, bool nowait)
{
	struct pv_entry *pv;

#ifdef PMAP_EVENT_COUNTERS
	if (nowait) {
		pmap_evcnt(pv_alloc_nowait);
	} else {
		pmap_evcnt(pv_alloc_wait);
	}
#endif

	/* Check for one in the garbage bin. */
	pv = LIST_FIRST(&pmap->pm_garbage.g_pvlist);
	if (__predict_false(pv != NULL)) {
		return pv;
	}

	KASSERT(PMAP_LOCK_OWNED(pmap));

	if (! nowait) {
		PMAP_UNLOCK(pmap);
	}
	pv = pool_get(&pmap_pv_pool, nowait ? PR_NOWAIT : 0);
	if (__predict_true(pv != NULL)) {
		KASSERT((((uintptr_t)pv) & PVH_ATTR_MASK) == 0);
	}
	if (! nowait) {
		PMAP_LOCK(pmap);
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
static inline void
pmap_garbage_init(struct pmap_garbage *g)
{
	TAILQ_INIT(&g->g_ptpages);
	LIST_INIT(&g->g_pvlist);
}

static inline void
pmap_garbage_transfer(struct pmap_garbage *to, struct pmap_garbage *from)
{
	TAILQ_INIT(&to->g_ptpages);
	TAILQ_CONCAT(&to->g_ptpages, &from->g_ptpages, ptp_list);
	LIST_MOVE(&from->g_pvlist, &to->g_pvlist, pv_pmlist);
}

static void
pmap_garbage_collect(struct pmap_garbage *g)
{
	struct pmap_ptpage *ptp;
	struct pv_entry *pv;

	while ((ptp = TAILQ_FIRST(&g->g_ptpages)) != NULL) {
		TAILQ_REMOVE(&g->g_ptpages, ptp, ptp_list);
		/*
		 * Can't assert ptp_freecnt here; it won't match up
		 * in the pmap_remove_all() case.
		 *
		 * KASSERT(ptp->ptp_freecnt ==
		 *     pmap_ptpage_table_counts[ptp->ptp_segtab]);
		 */
		pmap_ptpage_free(ptp);
	}

	while ((pv = LIST_FIRST(&g->g_pvlist)) != NULL) {
		LIST_REMOVE(pv, pv_pmlist);
		pmap_pv_free(pv);
	}
}

/*************************** PER-CPU PMAP DATA *******************************/

/*
 * We keep some per-cpu information immediately following the cpu_info
 * structure for this CPU.
 */

#define	ATC_ALLVA	((uint16_t)-1)
struct pmap_atc {
	uint16_t	atc_count;
	uint16_t	atc_flags;
	struct {
		pmap_t	atc_pmap;
		vaddr_t	atc_addrflags;
		paddr_t atc_pa;		/* for 68040/68060 cache flush */
	} atc_data[ATC_MAXVA];
};

#define	ATC_F_ALL	__BIT(0)	/* hit it all */
#define	ATC_F_PV	__BIT(1)	/* result of a P->V operation */
#define	ATC_F_USER	__BIT(2)	/* user VAs involved */
#define	ATC_F_SUPER	__BIT(3)	/* supervisor VAs involved */
#define	ATC_F_IC	__BIT(4)	/* also flush the I cache */
#define	ATC_F_DC	__BIT(5)	/* also flush the D cache */
#define	ATC_F_BC	(ATC_F_IC|ATC_F_DC)

#define	ATC_DAT_VA(a, i)	((a)->atc_data[i].atc_addrflags & ~PAGE_MASK)

struct pmap_percpu {
	/* Never de-reference pc_pmap -- only use for pointer comparisons! */
	pmap_t          pc_pmap;
	struct pmap_atc	pc_atc;		/* ATC invalidation info */
#if defined(MULTIPROCESSOR)
	paddr_t		pc_lev1pa;	/* value currently loaded into %urp */
#endif
};

/*
 * m68k_machdep.c CTASSERT's that sizeof(struct cpu_info) is <= 2KB.
 * We'll do one better and assert that sizeof(struct cpu_info) PLUS
 * sizeof(struct pmap_percpu) is <= 2KB.  Take that, slackers!
 */
__CTASSERT(sizeof(struct cpu_info) + sizeof(struct pmap_percpu) <= 2048);

static inline struct pmap_percpu *
pmap_percpu(void)
{
	vaddr_t va = (vaddr_t)&cpuinfo;

	va += sizeof(struct cpu_info);
	return (struct pmap_percpu *)va;
}

/******************** ADDRESS TRANSLATION CACHE HANDLING  ********************/

#if defined(M68020) || defined(M68030)
static inline void
ic20_inval(void)
{
	__asm __volatile("movc %0,%%cacr"
		:
		: "d" (IC_CLEAR));
}
#endif

#if MMU_CONFIG_HP_CLASS
extern vaddr_t MMUbase;

static inline void
hp_vac_inv_all(void)
{
	uint32_t *mmureg = (uint32_t *)(MMUbase + MMUCMD);

	__asm __volatile("andl %0,%2;"
			 "orl  %1,%2"
		:
		: "i" (~MMU_CEN), "i" (MMU_CEN), "m" (*mmureg));
}

static inline void
hp_vac_inv_super(void)
{
	uint32_t *mmureg = (uint32_t *)(MMUbase + MMUSSTP);
	int tmp;

	__asm __volatile("movl %1,%0;"
			 "movl %0,%1"
		: "=d" (tmp)
		: "m" (*mmureg));
}

static inline void
hp_vac_inv_user(void)
{
	uint32_t *mmureg = (uint32_t *)(MMUbase + MMUUSTP);
	int tmp;

	__asm __volatile("movl %1,%0;"
			 "movl %0,%1"
		: "=d" (tmp)
		: "m" (*mmureg));
}

static inline void
hp_tbia(void)
{
	uint32_t *mmureg = (uint32_t *)(MMUbase + MMUTBINVAL);
	int tmp;

	__asm __volatile("movl %1,%0"
		: "=d" (tmp)
		: "m" (*mmureg));
}

static inline void
hp_tbias(void)
{
	uint32_t *mmureg = (uint32_t *)(MMUbase + MMUTBINVAL);

	__asm __volatile("movl %0,%1"
		:
		: "i" (0x8000), "m" (*mmureg));
}

static inline void
hp_tbiau(void)
{
	uint32_t *mmureg = (uint32_t *)(MMUbase + MMUTBINVAL);

	__asm __volatile("movl %0,%1"
		:
		: "i" (0), "m" (*mmureg));
}

static inline void
hp_tbis(vaddr_t va)
{
	__asm __volatile("movsl %0,(%1)"
		:
		: "d" (0), "a" (va));
}

static void
pmap_atc_process_hp(void)
{
	const struct pmap_percpu *pc = pmap_percpu();
	const struct pmap_atc *atc = &pc->pc_atc;

	if (atc->atc_flags & ATC_F_IC) {
		pmap_evcnt(atc_ic);
		ic20_inval();
	}

	if (atc->atc_flags & ATC_F_DC) {
		switch (atc->atc_flags & (ATC_F_SUPER|ATC_F_USER)) {
		case ATC_F_SUPER:
			pmap_evcnt(atc_vac_super);
			hp_vac_inv_super();
			/* this also invalidates the TLB */
			return;
		case ATC_F_USER:
			pmap_evcnt(atc_vac_user);
			hp_vac_inv_user();
			/* this also invalidates the TLB */
			return;
		default:
			pmap_evcnt(atc_vac_all);
			hp_vac_inv_all();
			if (atc->atc_flags & ATC_F_ALL) {
				pmap_evcnt(atc_all);
				hp_tbia();
				return;
			}
			break;
		}
	}

	switch (atc->atc_flags & (ATC_F_ALL|ATC_F_SUPER|ATC_F_USER)) {
	case ATC_F_ALL|ATC_F_SUPER|ATC_F_USER:
		pmap_evcnt(atc_all);
		hp_tbia();
		return;
	case ATC_F_ALL|ATC_F_SUPER:
		pmap_evcnt(atc_all_super);
		hp_tbias();
		return;
	case ATC_F_ALL|ATC_F_USER:
		pmap_evcnt(atc_all_user);
		hp_tbiau();
		return;
	}

	setdfc(FC_PURGE);
	for (int i = 0; i < atc->atc_count; i++) {
		if (atc->atc_data[i].atc_pmap == pmap_kernel()) {
			pmap_evcnt(atc_one_super);
		} else if (atc->atc_data[i].atc_pmap == pc->pc_pmap) {
			pmap_evcnt(atc_one_user);
		} else {
			pmap_evcnt(atc_one_skipped);
			continue;
		}
		vaddr_t va = ATC_DAT_VA(atc, i);
		hp_tbis(va);
	}
	setdfc(FC_USERD);
}
#endif /* MMU_CONFIG_HP_CLASS */

#if MMU_CONFIG_68851_CLASS
static inline void
dc30_inval(void)
{
	__asm __volatile("movc %0,%%cacr"
		:
		: "d" (DC_CLEAR));
}

static inline void
bc30_inval(void)
{
	__asm __volatile("movc %0,%%cacr"
		:
		: "d" (CACHE_CLR));
}

static inline void
pflusha51(void)
{
	__asm __volatile("pflusha");
}

static inline void
pflush_all_user51(void)
{
	__asm __volatile("pflush %0,#4"
		:
		: "i" (FC_USERD));
}

static inline void
pflush_all_super51(void)
{
	__asm __volatile("pflush %0,#4"
		:
		: "i" (FC_SUPERD));
}

static inline void
pflush_one_user51(vaddr_t va)
{
	__asm __volatile("pflush %0,#4,(%1)"
		:
		: "i" (FC_USERD), "a" (va));
}

static inline void
pflush_one_super51(vaddr_t va)
{
	__asm __volatile("pflush %0,#4,(%1)"
		:
		: "i" (FC_SUPERD), "a" (va));
}

static void
pmap_atc_process_51(void)
{
	const struct pmap_percpu *pc = pmap_percpu();
	const struct pmap_atc *atc = &pc->pc_atc;

	if (atc->atc_flags & ATC_F_ALL) {
		switch (atc->atc_flags & (ATC_F_USER|ATC_F_SUPER)) {
		case ATC_F_USER:
			pmap_evcnt(atc_all_user);
			pflush_all_user51();
			break;
		case ATC_F_SUPER:
			pmap_evcnt(atc_all_super);
			pflush_all_super51();
			break;
		default:
			pmap_evcnt(atc_all);
			pflusha51();
			break;
		}
		goto done;
	}

	for (int i = 0; i < atc->atc_count; i++) {
		vaddr_t va = ATC_DAT_VA(atc, i);
		if (atc->atc_data[i].atc_pmap == pmap_kernel()) {
			pmap_evcnt(atc_one_super);
			pflush_one_super51(va);
		} else if (atc->atc_data[i].atc_pmap == pc->pc_pmap) {
			pmap_evcnt(atc_one_user);
			pflush_one_user51(va);
		} else {
			pmap_evcnt(atc_one_skipped);
		}
	}
 done:
	switch (atc->atc_flags & (ATC_F_IC | ATC_F_DC)) {
	case ATC_F_IC | ATC_F_DC:
#ifdef PMAP_EVENT_COUNTERS
		if (cputype == CPU_68020) {
			pmap_evcnt(atc_ic);
		} else {
			pmap_evcnt(atc_bc);
		}
#endif
		bc30_inval();		/* ok to do on 68020, too */
		break;
	case ATC_F_IC:
		pmap_evcnt(atc_ic);
		ic20_inval();
		break;
#ifdef M68030
	case ATC_F_DC:
		pmap_evcnt(atc_dc);
		dc30_inval();
		break;
#endif
	default:
		break;
	}
}
#endif /* MMU_CONFIG_68851_CLASS */

#if MMU_CONFIG_68040_CLASS
static inline void
cpusha_bc40(void)
{
	__asm __volatile(".chip 68040; cpusha %bc; .chip 68030");
}

static inline void
cpushp_bc40(paddr_t pa)
{
	__asm __volatile(".chip 68040; cpushp %%bc,(%0); .chip 68030"
		:
		: "a" (pa));
}

static inline void
cinva_ic40(void)
{
	__asm __volatile(".chip 68040; cinva %ic; .chip 68030");
}

static inline void
cinvp_ic40(paddr_t pa)
{
	__asm __volatile(".chip 68040; cinvp %%ic,(%0); .chip 68030"
		:
		: "a" (pa));
}

static inline void
cpusha_dc40(void)
{
	__asm __volatile(".chip 68040; cpusha %dc; .chip 68030");
}

static inline void
cpushp_dc40(paddr_t pa)
{
	__asm __volatile(".chip 68040; cpushp %%dc,(%0); .chip 68030"
		:
		: "a" (pa));
}

static inline void
pflusha40(void)
{
	__asm __volatile(".chip 68040; pflusha; .chip 68030");
}

static inline void
pflush40(vaddr_t va)
{
	/* We assume %dfc is already set up for us. */
	__asm __volatile(".chip 68040; pflush (%0); .chip 68030"
		:
		: "a" (va));
}

#if defined(M68060)
static inline void
bc60_inval(int which)
{
	int tmp;

	__asm __volatile(" movc %%cacr,%0;"
			 " orl  %1,%0;"
			 " movc %0,%%cacr"
		: "=d" (tmp)
		: "d" (which));
}
#endif /* M68060 */

static void
pmap_atc_process_40(void)
{
	const struct pmap_percpu *pc = pmap_percpu();
	const struct pmap_atc *atc = &pc->pc_atc;

	if (atc->atc_flags & ATC_F_ALL) {
		switch (atc->atc_flags & (ATC_F_IC|ATC_F_DC)) {
		case ATC_F_IC|ATC_F_DC:
			pmap_evcnt(atc_bc);
			cpusha_bc40();
			break;
		case ATC_F_IC:
			pmap_evcnt(atc_ic);
			cinva_ic40();
			break;
		case ATC_F_DC:
			pmap_evcnt(atc_dc);
			cpusha_dc40();
			break;
		default:
			break;
		}
		pmap_evcnt(atc_all);
		pflusha40();
	} else {
		int i, fc, prev_fc = FC_USERD;	/* USERD is steady state */
		for (i = 0; i < atc->atc_count; i++) {
			switch (atc->atc_data[i].atc_addrflags &
				(ATC_F_IC|ATC_F_DC)) {
			case ATC_F_IC|ATC_F_DC:
				pmap_evcnt(atc_bc_pg);
				cpushp_bc40(atc->atc_data[i].atc_pa);
				break;
			case ATC_F_IC:
				pmap_evcnt(atc_ic_pg);
				cinvp_ic40(atc->atc_data[i].atc_pa);
				break;
			case ATC_F_DC:
				pmap_evcnt(atc_dc_pg);
				cpushp_dc40(atc->atc_data[i].atc_pa);
				break;
			default:
				break;
			}
			if (atc->atc_data[i].atc_pmap == pmap_kernel()) {
				pmap_evcnt(atc_one_super);
				fc = FC_SUPERD;
			} else if (atc->atc_data[i].atc_pmap == pc->pc_pmap) {
				pmap_evcnt(atc_one_user);
				fc = FC_USERD;
			} else {
				pmap_evcnt(atc_one_skipped);
				continue;
			}
			if (fc != prev_fc) {
				pmap_evcnt(atc_dfc_switch);
				setdfc(fc);
				prev_fc = fc;
			}
			pflush40(ATC_DAT_VA(atc, i));
		}
		if (prev_fc != FC_USERD) {
			pmap_evcnt(atc_dfc_switch);
			setdfc(FC_USERD);
		}
	}
#if defined(M68060)
	if (cputype == M68060) {
		int which;

		if (atc->atc_flags & ATC_F_SUPER) {
			pmap_evcnt(atc_bc60inv_all);
			which = IC60_CABC;
		} else {
			pmap_evcnt(atc_bc60inv_user);
			which = IC60_CUBC;
		}
		bc60_inval(which);
	}
#endif
}
#endif /* MMU_CONFIG_68040_CLASS */

static void
pmap_atc_invalidate(pmap_t pmap, vaddr_t va, paddr_t pa, uint16_t flags)
{
	struct pmap_atc *atc = &pmap_percpu()->pc_atc;
	int i;

#ifdef PMAP_EVENT_COUNTERS
	if (__predict_false(flags & ATC_F_PV)) {
		pmap_evcnt(atc_pv);
	}
#endif

	if (__predict_true(pmap != NULL)) {
		flags |= (pmap == pmap_kernel() ? ATC_F_SUPER : ATC_F_USER);
	}

	if ((i = atc->atc_count) == ATC_MAXVA || (flags & ATC_F_ALL) != 0) {
#ifdef PMAP_EVENT_COUNTERS
		if ((atc->atc_flags & ATC_F_ALL) == 0) {
			if (atc->atc_count == ATC_MAXVA) {
				pmap_evcnt(atc_req_saturate);
			} else if (atc->atc_count != ATC_ALLVA) {
				pmap_evcnt(atc_req_all);
			}
		}
#endif
		if (flags & ATC_F_ALL) {
			atc->atc_count = ATC_ALLVA;
		}
		atc->atc_flags |= flags | ATC_F_ALL;
		return;
	}

	atc->atc_count++;
	atc->atc_flags |= flags;
	atc->atc_data[i].atc_pmap = pmap;
	atc->atc_data[i].atc_addrflags = va | flags;
	atc->atc_data[i].atc_pa = pa;
}

static inline void
pmap_atc_invalidate_all(uint16_t flags)
{
	KASSERT((flags & (ATC_F_USER | ATC_F_SUPER)) != 0);
	pmap_atc_invalidate(NULL, 0, 0, flags | ATC_F_ALL);
}

static inline void
pmap_atc_invalidate_pv(struct pv_entry *pv, paddr_t pa, uint16_t atc_flags)
{
	/*
	 * The ATC is flushed on each context switch, so if this is not
	 * the currently loaded pmap, we don't have to do any work.
	 *
	 * Considerations:
	 * - 68030's D-cache is flushed on context switch.
	 *
	 * - HP MMU VAC is also flushed on context switch.
	 *
	 * - 68040 and 68060 D-cache is physically addressed
	 *   and might need to be hit here.
	 */
	if (__predict_true(pv->pv_pmap != pmap_kernel() &&
			   pv->pv_pmap != pmap_percpu()->pc_pmap)) {
#if MMU_CONFIG_68040_CLASS
		/* push the page, if needed. */
		if (MMU_IS_68040_CLASS && (atc_flags & ATC_F_DC) != 0) {
			pmap_evcnt(atc_pv_cpushp);
			cpushp_dc40(pa);
		}
#endif
		pmap_evcnt(atc_pv_skipped);
		return;
	}
	pmap_atc_invalidate(pv->pv_pmap, PV_VA(pv), pa, atc_flags | ATC_F_PV);
}

static inline void
pmap_atc_process(void)
{
	struct pmap_atc *atc = &pmap_percpu()->pc_atc;

#ifdef PMAP_EVENT_COUNTERS
	switch (atc->atc_count) {
	case 0: pmap_evcnt(atc_0); break;
	case 1: pmap_evcnt(atc_1); break;
	case 2: pmap_evcnt(atc_2); break;
	case 3: pmap_evcnt(atc_3); break;
	case 4: pmap_evcnt(atc_4); break;
	case 5: pmap_evcnt(atc_5); break;
	case 6: pmap_evcnt(atc_6); break;
	case 7: pmap_evcnt(atc_7); break;
	case 8: pmap_evcnt(atc_8); break;
	default: break;
	}
#endif

	if (atc->atc_count) {
		(*pmap_atc_process_local_func)();
		atc->atc_count = 0;
	}
	atc->atc_flags = 0;
}

/************************ PTE MANIPULATION HELPERS ***************************/

/* Assert assumptions made in <machine/pmap.h>. */
__CTASSERT(DT51_PAGE == PTE40_RESIDENT);
__CTASSERT(PTE51_WP == PTE40_W);
__CTASSERT(PTE51_U == PTE40_U);
__CTASSERT(PTE51_M == PTE40_M);
__CTASSERT(PTE51_CI == PTE40_CM_NC_SER);
__CTASSERT(PMBM_F_U0 == PTE40_U0);
__CTASSERT(PMBM_F_U1 == PTE40_U1);

static pt_entry_t	pmap_pte_proto[UVM_PROT_ALL + 1] __read_mostly;
static pt_entry_t	pmap_pte_proto_ci[UVM_PROT_ALL + 1] __read_mostly;
static pt_entry_t	pmap_pte_proto_cwt[UVM_PROT_ALL + 1] __read_mostly;
static pt_entry_t	pmap_pte_proto_um[UVM_PROT_ALL + 1] __read_mostly;
static pt_entry_t	pmap_ste_proto __read_mostly;

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
pte_load(volatile pt_entry_t *ptep)
{
	return *ptep;
}

static inline void
pte_store(volatile pt_entry_t *ptep, pt_entry_t npte)
{
	*ptep = npte;
}

/*
 * Don't inline the CAS.L instruction; some systems have non-working
 * READ-MODIFY-WRITE cycle logic.  This will ensure that we'll use
 * restartable atomic sequence, if required.
 *
 * AND.L and OR.L don't use the RMC signal, so they aren't subject
 * to the same constraints.
 */
static inline pt_entry_t
pte_update(volatile pt_entry_t *ptep, pt_entry_t opte, pt_entry_t npte)
{
	/*
	 * Use compare-and-swap to update the PTE.  This ensures there's
	 * no possibility of losing any hardware-maintained bits when
	 * updating the PTE.
	 */
#if !defined(__HAVE_M68K_BROKEN_RMC)
	__asm __volatile("casl %1,%2,%0"
		: "+m" (*ptep), "+d" (opte)
		: "d" (npte)
		: "cc", "memory");
	return opte;
#else
	return atomic_cas_uint(ptep, opte, npte);
#endif
}

#if MMU_CONFIG_HP_CLASS
/*
 * These are only used for HP MMU VAC shenanigans.  There is no need
 * for these to be truly atomic, and systems with an HP MMU can't do
 * truly atomic operations anyway.
 */
static inline void
pte_set(volatile pt_entry_t *ptep, pt_entry_t bits)
{
	*ptep |= bits;
}

static inline void
pte_mask(volatile pt_entry_t *ptep, pt_entry_t mask)
{
	*ptep &= mask;
}
#endif /* MMU_CONFIG_HP_CLASS */

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
	pt_entry_t c_bits, rw_c_bits, ci_bits, prot_bits, um_bits;
	int prot;

	/*
	 * N.B. PTE40_CM_WT == 0, so "write-though" caching has the same
	 * PTE bits on all variants of the MMU.
	 */

	if (MMU_IS_68040_CLASS) {
		rw_c_bits = PTE40_CM_CB;
		ci_bits = PTE40_CM_NC_SER;
	} else {
		rw_c_bits = 0;
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
		c_bits = (prot & UVM_PROT_WRITE) ? rw_c_bits : 0;
		pmap_pte_proto[prot]     = PTE_VALID | prot_bits | c_bits;
		pmap_pte_proto_ci[prot]  = PTE_VALID | prot_bits | ci_bits;
		pmap_pte_proto_cwt[prot] = PTE_VALID | prot_bits;
		pmap_pte_proto_um[prot]  = um_bits;
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
	pt_entry_t *pte_proto;

	switch (flags & PMAP_CACHE_MASK) {
	case PMAP_NOCACHE:
	case PMAP_WRITE_COMBINE:
	case PMAP_NOCACHE_OVR:
		pte_proto = pmap_pte_proto_ci;
		break;

	case PMAP_WRITE_THROUGH:
		pte_proto = pmap_pte_proto_cwt;
		break;

	default:
		pte_proto = pmap_pte_proto;
		break;
	}

	prot &= UVM_PROT_ALL;
	KASSERT(prot != 0);

	/*
	 * N.B. We seed the U and M bits in the PTE based on the access
	 * that was used to produce the page fault because the MMU will
	 * skip the write-back of a PTE upon access if the copy in the
	 * ATC already has the bit set.
	 */
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
#define	pmap_trunc_ptpage(va)	((va) & ~PTPAGEVAOFS)

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
static vaddr_t kernel_virtual_start __read_mostly;
static vaddr_t kernel_virtual_end __read_mostly;
       vaddr_t kernel_virtual_max __read_mostly = KERNEL_MAX_ADDRESS;

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
static paddr_t kernel_stnext_pa __read_mostly;
static paddr_t kernel_stnext_endpa __read_mostly;

/*
 * Null segment table that every pmap gets as its initial level 1
 * map.  This is a single page allocated in pmap_bootstrap1(), and
 * we zero it out in pmap_init().
 */
static paddr_t null_segtab_pa __read_mostly;

/*
 * pmap_load_urp:
 *
 *	Load the user root table into the MMU.
 */
static inline void
pmap_load_urp(paddr_t urp)
{
#if defined(MULTIPROCESSOR)
	pmap_percpu()->pc_lev1pa = urp;
#endif
	(*pmap_load_urp_func)(urp);
}

#if defined(MULTIPROCESSOR)
/*
 * pmap_reload_urp:
 *
 *	Re-load the user root table into the MMU for this pmap
 *	if it's gotten a new one since the last time.
 */
static void
pmap_reload_urp(pmap_t pmap)
{
	struct pmap_percpu *pc = pmap_percpu();

	/*
	 * The sender of the reload message holds the pmap lock,
	 * ensuring the validity of pmap->pm_lev1pa.
	 */

	if (pmap == pc->pc_pmap && pmap->pm_lev1pa != pc->pc_lev1pa) {
		pmap_load_urp(pmap->pm_lev1pa);
	}
}
#endif /* MULTIPROCESSOR */

static inline void
pmap_set_lev1map(pmap_t pmap, struct pmap_table *pt, paddr_t pa)
{
	pmap->pm_lev1map = pt;
	pmap->pm_lev1pa = pa;

	if (pmap == pmap_percpu()->pc_pmap) {
		/*
		 * The MMU-specific load-urp functions take care of
		 * any necessary ATC / cache invalidations.
		 */
		pmap_load_urp(pmap->pm_lev1pa);
	}
}

/*
 * Table accessors.
 */
static inline unsigned int
pmap_pagenum(vaddr_t va)
{
	return ((va) >> PAGE_SHIFT);
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
pmap_table_alloc(pmap_t pmap, bool segtab, bool nowait)
{
	struct pmap_ptpage_list *pmlist = &pmap->pm_ptpages[segtab];
	struct pmap_ptpage *ptp, *newptp = NULL;
	struct pmap_table *pt;

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
			/*
			 * N.B. pmap_ptpage_alloc() will temporarily exit
			 * the critical section.
			 */
			newptp = pmap_ptpage_alloc(pmap, segtab, nowait);
			if (newptp == NULL) {
				/*
				 * If we didn't wait, then no one would
				 * have allocated one behind our back.
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
		TAILQ_INSERT_TAIL(&pmap->pm_garbage.g_ptpages, newptp,
				  ptp_list);
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
pmap_table_free(pmap_t pmap, struct pmap_table *pt, struct pmap_garbage *g)
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
		TAILQ_INSERT_TAIL(&g->g_ptpages, ptp, ptp_list);
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
			struct pmap_garbage *g)
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
	KASSERT(g != NULL);
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
		pmap_table_release_slow(pmap, pt->pt_st, g);
		pt->pt_st = NULL;
	} else if (pt == pmap->pm_lev1map) {
		pmap_set_lev1map(pmap, NULL, null_segtab_pa);
	}
	pmap_table_free(pmap, pt, g);
}

static inline void
pmap_table_release(pmap_t pmap, struct pmap_table *pt,
		   struct pmap_garbage *g)
{
	if (__predict_true(pt != NULL)) {
		if (__predict_true(pt->pt_holdcnt > 1)) {
			pt->pt_holdcnt--;
			return;
		}
		pmap_table_release_slow(pmap, pt, g);
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
    unsigned int segnum, bool segtab, bool nowait)
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

	/*
	 * Allocate the new table.
	 *
	 * N.B. pmap_table_alloc() will temporarily exit the critical section.
	 */
	t2 = pmap_table_alloc(pmap, segtab, nowait);
	if (__predict_false(t2 == NULL)) {
		pmap_table_release(pmap, t1, &pmap->pm_garbage);
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
		pmap_table_release(pmap, t2, &pmap->pm_garbage);
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

static pt_entry_t *kernel_ptes __read_mostly;

/*
 * pmap_kernel_pte:
 *
 *	Get the PTE that maps the specified kernel virtual address.
 *
 *	Take note: the caller *may assume* they they can linearly
 *	access adjacent PTEs up until the address indicated by
 *	virtual_end!  That means, "pte++" is totally fine until you
 *	get to the current limit of the kernel virtual address space!
 */
static pt_entry_t *
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
	KASSERT(va >= VM_MIN_KERNEL_ADDRESS);
	return &kernel_ptes[m68k_btop(va - VM_MIN_KERNEL_ADDRESS)];
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
pmap_pte_alloc(pmap_t pmap, vaddr_t va, struct pmap_table **out_pt, bool nowait)
{
	struct pmap_table *st, *pt;
	pt_entry_t *ptep;

	KASSERT(PMAP_LOCK_OWNED(pmap));

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
		 *
		 * N.B. pmap_table_alloc() will temporarily exit the
		 * critical section.
		 */
		st = pmap_table_alloc(pmap, true/*segtab*/, nowait);
		if (__predict_false(st == NULL)) {
			return NULL;
		}

		/* Re-validate that we still need the segment table. */
		if (__predict_false(pmap->pm_lev1map != NULL)) {
			/* Raced and lost. */
			pmap_table_release(pmap, st, &pmap->pm_garbage);
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
		    pmap_st1_index(va), true/*segtab*/, nowait);
		if (__predict_false(st == NULL)) {
			pmap_table_release(pmap, st1, &pmap->pm_garbage);
			return NULL;
		}
	}

	/* We can now allocate and insert the leaf page table. */
	pt = pmap_table_insert(pmap, st, pmap_st_index(va), pmap_segnum(va),
	    false/*segtab*/, nowait);
	if (__predict_false(pt == NULL)) {
		pmap_table_release(pmap, st, &pmap->pm_garbage);
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
 *	table.  We are also guaranteed that no mapping exists there, but
 *	the MMU has a negative cache in the ATC (see 68030UM Figure 9-8.
 *	Address Translation General Flowchart, ATC hit, B==1 case, as well
 *	as 68040UM Figure 3-21. ATC Entry and Tag Fields, R bit and the
 *	associated descriptive text), so we still have to handle ATC entry
 *	invalidation.
 */
static void
pmap_pv_enter(pmap_t pmap, struct vm_page *pg, vaddr_t va, vm_prot_t prot,
    struct pmap_table *pt, pt_entry_t npte, struct pv_entry *newpv)
{
	const bool usr_ci = pte_ci_p(npte);
	const paddr_t pa = VM_PAGE_TO_PHYS(pg);
	uint16_t atc_flags = 0;
	struct pv_entry *pv;
	pt_entry_t opte;

	pmap_evcnt(pv_enter_called);

	KASSERT(newpv != NULL);
	KASSERT(PMAP_LOCK_OWNED(pmap));

	npte |= PTE_PVLIST;

	newpv->pv_pmap = pmap;
	newpv->pv_vf = va;
	newpv->pv_pt = pt;

	if (__predict_false(usr_ci)) {
		newpv->pv_vf |= PV_F_CI_USR;
	}

	/*
	 * If this is an EXEC mapping, then we have to ensure that
	 * the I$ doesn't load stale data.
	 *
	 * XXX Should have a soft-PTE bit for this.
	 */
	if (prot & UVM_PROT_EXEC) {
#if MMU_CONFIG_68040_CLASS
		if (MMU_IS_68040_CLASS) {
			atc_flags |= ATC_F_BC;
		}
#endif
	}

	pt_entry_t *ptep = pmap_pv_pte(newpv);

	PVH_LOCK(pg);

#ifdef DEBUG
	/*
	 * Make sure the entry doesn't already exist.
	 */
	for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
		if (pmap == pv->pv_pmap && va == PV_VA(pv)) {
			PVH_UNLOCK(pg);
			panic("%s: pmap=%p va=0x%08lx already in PV table",
			    __func__, pmap, va);
			/* NOTREACHED */
		}
	}
#endif

	newpv->pv_next = VM_MDPAGE_PVS(pg);
	VM_MDPAGE_SETPVP(VM_MDPAGE_HEAD_PVP(pg), newpv);
	LIST_INSERT_HEAD(&pmap->pm_pvlist, newpv, pv_pmlist);
	VM_MDPAGE_ADD_UM(pg, npte);

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
		goto out;
	}

	/* Set the PTE for the new mapping. */
	pte_store(ptep, npte);

	/*
	 * If this is a user-requested CI mapping, we need to make
	 * sure the page is purged from the cache and mark any other
	 * mappings of this page CI as well.
	 */
	if (__predict_false(usr_ci)) {
		uint16_t atc_c_flags = ATC_F_BC;

		VM_MDPAGE_SET_CI(pg);
		pmap_evcnt(pv_enter_usr_ci);

		/*
		 * There shouldn't be very many of these; CI mappings
		 * of managed pages are typically only for coherent DMA
		 * purposes, and multiple mappings of the same page are
		 * extremely uncommon in that scenario.
		 */
		for (pv = newpv->pv_next; pv != NULL; pv = pv->pv_next) {
			pt_entry_t actual;
			pmap_evcnt(pv_enter_ci_multi);
			ptep = pmap_pv_pte(pv);
			for (opte = pte_load(ptep);; opte = actual) {
				npte = pte_set_ci(opte);
				actual = pte_update(ptep, opte, npte);
				if (__predict_true(actual == opte)) {
					pmap_atc_invalidate_pv(pv, pa,
					    atc_flags | atc_c_flags);
					atc_c_flags = 0;
					break;
				}
			}
		}
	}
 out:
	PVH_UNLOCK(pg);
	pmap_atc_invalidate(pmap, va, pa, atc_flags);
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
	 * XXX
	 * XXX ALSO... Hibler's pmap checked for incompatible aliases
	 * XXX between user and kernel... but doesn't the cache have
	 * XXX a USER/SUPER discriminator on the tag?  You can selectively
	 * XXX invalidate that way!
	 */

	/* Set the PTE for the new mapping. */
	pte_store(ptep, npte);
	pmap_atc_invalidate(pmap, va, pa, atc_flags);

	/*
	 * Invalidate the ATC entry **after** storing the PTE so that
	 * there is no window where another MMU table walk finds the
	 * stale invalid entry.
	 */

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
		PVH_UNLOCK(pg);
		return;
	}

	VM_MDPAGE_SET_CI(pg);
	for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
		if (MATCHING_PMAP(pmap, pv->pv_pmap)) {
			pmap_evcnt(pv_enter_ci_multi);
			pv->pv_vf |= pv_flags;
			pte_set(pmap_pv_pte(pv), PTE51_CI);
			pmap_atc_invalidate_pv(pv, pa, ATC_F_DC);
		}
	}
	PVH_UNLOCK(pg);
#endif /* MMU_CONFIG_HP_CLASS */
}

/*
 * pmap_pv_remove:
 *
 *	Remove a physical->virtual entry from the pv table.
 */
static void
pmap_pv_remove(pmap_t pmap, struct vm_page *pg, vaddr_t va, pt_entry_t opte,
    struct pmap_garbage *g)
{
	struct pv_entry **pvp, *pv;
	pt_entry_t *ptep, npte;
	paddr_t pa = VM_PAGE_TO_PHYS(pg);

	pmap_evcnt(pv_remove_called);

	KASSERT(PVH_LOCK_OWNED(pg));
	KASSERT(PMAP_LOCK_OWNED(pmap));

	/* Update cached U/M bits from mapping that's going away. */
	VM_MDPAGE_ADD_UM(pg, opte);

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

	KASSERT(g != NULL);
	LIST_INSERT_HEAD(&g->g_pvlist, pv, pv_pmlist);

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
			pt_entry_t actual;
			ptep = pmap_pv_pte(pv);
			for (opte = pte_load(ptep);; opte = actual) {
				opte = pte_load(ptep);
				npte = pte_clr_ci(opte);
				actual = pte_update(ptep, opte, npte);
				if (__predict_true(actual == opte)) {
					pmap_atc_invalidate_pv(pv, pa, 0);
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
			pmap_atc_invalidate_pv(pv, pa, 0);
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
pmap_stat_update_impl(volatile long *valp, int val)
{
#if defined(MULTIPROCESSOR)
	atomic_add_long(valp, val);
#else
	/* Atomic vis a vis interrupts. */
	__asm __volatile("addl %1,%0"
		: "+m" (*valp)
		: "d" (val));
#endif
}

#define	pmap_stat_update(pm, stat, delta)		\
	pmap_stat_update_impl(&(pm)->pm_stats.stat, (delta))

static inline void
pmap_stat_set_impl(volatile long *valp, int val)
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
	PMAP_MUTEX_INIT(&pmap->pm_mutex, MUTEX_DEFAULT, IPL_NONE);
	pmap->pm_lev1pa = lev1pa;
	rb_tree_init(&pmap->pm_tables, &pmap_table_rb_ops);
	TAILQ_INIT(&pmap->pm_ptpages[0]);
	TAILQ_INIT(&pmap->pm_ptpages[1]);
	LIST_INIT(&pmap->pm_pvlist);

	pmap_garbage_init(&pmap->pm_garbage);

	pmap->pm_refcnt = 1;
}

/*
 * pmap_virtual_space:		[ INTERFACE ]
 *
 *	Define the initial bounds of the kernel virtual address space.
 *
 *	In this implementation, the start address we return marks the
 *	end of the statically allocated special kernel virtual addresses
 *	set up in pmap_bootstrap1().  We return kernel_virtual_max as
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
	uint32_t newval;

	KASSERT(pmap->pm_refcnt > 0);
#if defined(MULTIPROCESSOR)
	newval = atomic_dec_32_nv(&pmap->pm_refcnt);
#else
	newval = --pmap->pm_refcnt;
#endif
	if (newval) {
		return;
	}

	/* We assume all mappings have been removed. */
	KASSERT(pmap->pm_lev1map == NULL);
	KASSERT(pmap->pm_lev1pa == null_segtab_pa);

	/* ...and all the garbage has been collected. */
	KASSERT(TAILQ_EMPTY(&pmap->pm_garbage.g_ptpages));
	KASSERT(LIST_EMPTY(&pmap->pm_garbage.g_pvlist));

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
#if defined(MULTIPROCESSOR)
	atomic_inc_32(&pmap->pm_refcnt);
#else
	pmap->pm_refcnt++;
#endif
	KASSERT(pmap->pm_refcnt > 0);
}

/*
 * pmap_remove_mapping:
 *
 *	Invalidate a single page denoted by pmap/va.
 *
 *	If (ptep != NULL), it is the already computed PTE for the mapping.
 *
 *	If the caller wishes to prevent the page table from being freed,
 *	they should perform an extra retain.
 */
static void
pmap_remove_mapping(pmap_t pmap, vaddr_t va, pt_entry_t *ptep,
    struct pmap_table *pt, struct vm_page *pg,
    uint16_t atc_flags, struct pmap_garbage *g)
{
	KASSERT(pt != NULL || pmap == pmap_kernel());
	KASSERT(ptep != NULL);
	const paddr_t opte = pte_load(ptep);
	KASSERT(pte_valid_p(opte));
	const paddr_t pa = pte_pa(opte);
	KASSERT(pg == NULL || pa == VM_PAGE_TO_PHYS(pg));

	KASSERT(pg == NULL || PVH_LOCK_OWNED(pg));
	KASSERT(PMAP_LOCK_OWNED(pmap));

	/* Update statistics. */
	if (pte_wired_p(opte)) {
		pmap_stat_update(pmap, wired_count, -1);
	}
	pmap_stat_update(pmap, resident_count, -1);

	/*
	 * 68020/68030 I-cache and 68030 D-cache are virtually-addressed,
	 * so removing a mapping for a page requires invalidating the
	 * cache.
	 *
	 * We can skip this if the mapping was cache-inhibited.
	 *
	 * N.B. This also covers the external VAC on HP MMU systems.
	 *
	 * XXX For I-cache, should check EXEC'ness of the page.
	 */
	if (cputype < CPU_68040 && !pte_ci_p(opte)) {
		atc_flags |= ATC_F_DC | ATC_F_IC;
	}

	if (__predict_true(pg != NULL)) {
		KASSERT(pte_managed_p(opte));
		pmap_pv_remove(pmap, pg, va, opte, g);
	} else {
		KASSERT(! pte_managed_p(opte));
	}

	/*
	 * Zap the PTE and drop the retain count that the mapping
	 * had on the table.
	 */
	pte_store(ptep, 0);
	pmap_table_release(pmap, pt, g);

	/*
	 * Now that the ATC can't be reloaded from the PTE, invalidate
	 * the ATC entry.
	 */
	pmap_atc_invalidate(pmap, va, pa, atc_flags);
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
void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	pt_entry_t opte, *ptep;
	struct pmap_table *pt;
	struct vm_page *pg;
	vaddr_t nextseg;

	/*
	 * If this is the kernel pmap, we can use a faster method
	 * for accessing the PTEs (since the PT pages are always
	 * resident).
	 *
	 * Note that this routine should NEVER be called from an
	 * interrupt context; pmap_kremove() is used for that.
	 */
	if (pmap == pmap_kernel()) {
		struct pmap_garbage kg;

		/*
		 * We can't safely defer freeing resources for the
		 * kernel pmap because pmap_update() can be called
		 * in an interrupt context, so we have to use our
		 * own garbage can.
		 */
		pmap_garbage_init(&kg);

		PMAP_LOCK(pmap);
		for (ptep = pmap_kernel_pte(sva); sva < eva;
		     ptep++, sva += PAGE_SIZE) {
			opte = pte_load(ptep);
			if (pte_valid_p(opte)) {
				pg = pmap_pa_to_pg(pte_pa(opte));
				if (pg != NULL) {
					PVH_LOCK(pg);
				}
				pmap_remove_mapping(pmap, sva,
				    ptep, NULL/*pt*/, pg, 0, &kg);
				if (pg != NULL) {
					PVH_UNLOCK(pg);
				}
			}
		}
		PMAP_UNLOCK(pmap);

		/* kernel PT pages are never freed. */
		KASSERT(TAILQ_EMPTY(&kg.g_ptpages));

		/* ...but we might have freed PV entries. */
		pmap_garbage_collect(&kg);

		return;
	}

	PMAP_LOCK(pmap);
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
			pg = pmap_pa_to_pg(pte_pa(opte));
			if (pg != NULL) {
				PVH_LOCK(pg);
			}
			pmap_remove_mapping(pmap, sva,
			    ptep, pt, pg, 0, &pmap->pm_garbage);
			if (pg != NULL) {
				PVH_UNLOCK(pg);
			}
		}
		pmap_table_release(pmap, pt, &pmap->pm_garbage);
	}
	PMAP_UNLOCK(pmap);
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
	struct pmap_garbage g;
	struct pv_entry *pv;
	struct vm_page *pg;
	pt_entry_t opte;

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
	 * ==> (5) Grab any garbage lingering in the pmap itself.
	 *
	 * Once we've done that, we just need to free everything
	 * back to the system.
	 */

	pmap_garbage_init(&g);

	PMAP_LOCK(pmap);

	/* Step 1. */
	pmap_set_lev1map(pmap, NULL, null_segtab_pa);

	/* Step 2. */
	pmap->pm_pt_cache = NULL;
	TAILQ_CONCAT(&g.g_ptpages, &pmap->pm_ptpages[0], ptp_list);
	TAILQ_CONCAT(&g.g_ptpages, &pmap->pm_ptpages[1], ptp_list);
	memset(&pmap->pm_tables, 0, sizeof(pmap->pm_tables));
	rb_tree_init(&pmap->pm_tables, &pmap_table_rb_ops);
	KASSERT(RB_TREE_MIN(&pmap->pm_tables) == NULL);

	/* Step 3. */
	while ((pv = LIST_FIRST(&pmap->pm_pvlist)) != NULL) {
		KASSERT(pv->pv_pmap == pmap);
		opte = pte_load(pmap_pv_pte(pv));
		pg = pmap_pa_to_pg(pte_pa(opte));
		PVH_LOCK(pg);
		pmap_pv_remove(pmap, pg, PV_VA(pv), opte, &pmap->pm_garbage);
		PVH_UNLOCK(pg);
	}

	/* Step 4. */
	pmap_stat_set(pmap, wired_count, 0);
	pmap_stat_set(pmap, resident_count, 0);

	/* Step 5. */
	TAILQ_CONCAT(&g.g_ptpages, &pmap->pm_garbage.g_ptpages, ptp_list);
	LIST_MOVE(&pmap->pm_garbage.g_pvlist, &g.g_pvlist, pv_pmlist);

	PMAP_UNLOCK(pmap);

	pmap_garbage_collect(&g);

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
	struct pmap_garbage g;
	struct pv_entry *pv;
	pmap_t pmap;

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
	pmap_garbage_init(&g);

 restart:
	PVH_LOCK(pg);
	while ((pv = VM_MDPAGE_PVS(pg)) != NULL) {
		pmap = pv->pv_pmap;
		if (! PMAP_TRYLOCK(pmap)) {
			/*
			 * Deadlock avoidance.  Take an extra reference
			 * on the pmap so that it doesn't disappear on
			 * us, then drop the pvh lock, acquire (and
			 * release) the pmap lock, then go around again.
			 */
			pmap_reference(pmap);
			PVH_UNLOCK(pg);
			PMAP_LOCK(pmap);
			PMAP_UNLOCK(pmap);
			pmap_destroy(pmap);
			goto restart;
		}
		pmap_remove_mapping(pmap, PV_VA(pv), pmap_pv_pte(pv),
		    pv->pv_pt, pg, ATC_F_PV, &g);
		PMAP_UNLOCK(pmap);
	}
	PVH_UNLOCK(pg);

	/* Page-oriented operations must collect the garbage immediately. */
	pmap_atc_process();
	pmap_garbage_collect(&g);
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
	uint16_t atc_flags;

	if ((prot & UVM_PROT_READ) == 0) {
		pmap_remove(pmap, sva, eva);
		return;
	}

	PMAP_LOCK(pmap);

#if MMU_CONFIG_68040_CLASS
	removing_write = (prot & UVM_PROT_WRITE) == 0;
#endif
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
			pt_entry_t actual;
			opte = pte_load(ptep);
			paddr_t pa = pte_pa(opte);
 try_again:
			if (! pte_valid_p(opte)) {
				continue;
			}
			npte = pte_change_prot(opte, prot);
			if (npte == opte) {
				continue;
			}
			atc_flags = 0;
#if MMU_CONFIG_68040_CLASS
			if (MMU_IS_68040_CLASS && removing_write) {
				/*
				 * Clear caches if making RO (see section
				 * "7.3 Cache Coherency" in the manual).
				 */
				atc_flags |= ATC_F_DC;
			}
#endif
			actual = pte_update(ptep, opte, npte);
			if (__predict_false(actual != opte)) {
				/* Lost race updating PTE; try again. */
				opte = actual;
				goto try_again;
			}
			pmap_atc_invalidate(pmap, sva, pa, atc_flags);
		}
		pmap_table_release(pmap, pt, NULL);
	}

	PMAP_UNLOCK(pmap);
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
	int error = 0;
	const bool nowait = !!(flags & PMAP_CANFAIL);

	/* cpu-local mappings never allowed via this path. */
	KASSERT((flags & PMAP_CPU_LOCAL) == 0);

	struct vm_page * const pg = pmap_pa_to_pg(pa);
	if (__predict_false(pg == NULL)) {
		/*
		 * PA is not part of managed memory.  Make the mapping
		 * cache-inhibited on the assumption that it's a device
		 * unless there cache mode has been explcitly called out.
		 */
		if (__predict_true((flags & PMAP_CACHE_MASK) == 0)) {
			flags |= PMAP_NOCACHE;
		}
	}

	PMAP_LOCK(pmap);

	if (nowait) {
		pmap_evcnt(enter_nowait);
	} else {
		pmap_evcnt(enter_yeswait);
	}

	/* Get the destination table. */
	ptep = pmap_pte_alloc(pmap, va, &pt, nowait);
	if (__predict_false(ptep == NULL)) {
		pmap_evcnt(enter_pte_alloc_fail);
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
			if (diff) {
				/*
				 * Protection or caching status is changing;
				 * flush the page from the cache.
				 */
				uint16_t atc_flags = MMU_IS_68040_CLASS
				    ? ATC_F_BC : 0;

				/*
				 * If the new mapping is CI and the old
				 * one is not, then flush D-cache on
				 * CPUs where the cache is virtually-
				 * addressed.
				 */
				if (cputype < CPU_68040 &&
				    pte_ci_p(diff) && pte_ci_p(npte)) {
					atc_flags |= ATC_F_DC;
				}
				pmap_atc_invalidate(pmap, va, pa, atc_flags);
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
		 *
		 * XXX Can we elide the ATC flush here?  We're going to
		 * XXX hit the ATC after setting the new PTE anyway.
		 */
		pmap_evcnt(enter_pa_change);
		if (pg != NULL) {
			PVH_LOCK(pg);
		}
		pmap_remove_mapping(pmap, va, ptep, pt, pg, 0,
		    &pmap->pm_garbage);
		if (pg != NULL) {
			PVH_UNLOCK(pg);
		}
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
		 *
		 * Because this is an extremely common case, we are
		 * first going to attempt allocation while still in
		 * the critical section.  If that fails and waiting
		 * is allowed, we'll leave the critical section and
		 * try a blocking allocation.
		 *
		 * N.B. pmap_pv_alloc() will check for a free entry
		 * in the garbage bin and do the critical section
		 * exit/re-entry for us.
		 */
		newpv = pmap_pv_alloc(pmap, true/*nowait flag*/);
		if (__predict_false(newpv == NULL)) {
			if (nowait) {
				pmap_evcnt(enter_pv_alloc_fail);
				error = ENOMEM;
				goto out_release;
			}
			newpv = pmap_pv_alloc(pmap, false/*nowait flag*/);
			KASSERT(newpv != NULL);
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
				LIST_INSERT_HEAD(&pmap->pm_garbage.g_pvlist,
				    newpv, pv_pmlist);
				goto restart;
			}
		}

		/*
		 * Enter the mapping into the PV list.  pmap_pv_enter()
		 * will also set the PTE in the table.
		 *
		 * The new mapping takes ownership of the PT
		 * retain count we took while looking up the PTE.
		 */
		pmap_pv_enter(pmap, pg, va, prot, pt, npte, newpv);
		goto out;
	}

	/*
	 * Not a managed mapping, so set the new PTE.  As with managed
	 * mappings, the new mapping takes ownership of the PT retain
	 * count we took while looking up the PTE.
	 */
	pte_store(ptep, npte);

	/*
	 * See comments in pmap_pv_enter() for why we have to hit
	 * the ATC here.
	 */
	pmap_atc_invalidate(pmap, va, pa, 0);
	goto out;

 out_release:
	pmap_table_release(pmap, pt, &pmap->pm_garbage);
 out:
	if (__predict_false(error != 0)) {
		/* Don't rely on a pmap_update() call after a failure. */
		struct pmap_garbage g;
		pmap_garbage_transfer(&g, &pmap->pm_garbage);
		PMAP_UNLOCK(pmap);
		pmap_atc_process();
		pmap_garbage_collect(&g);
		return error;
	}
	PMAP_UNLOCK(pmap);
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
	int atc_flags = 0;

	KASSERT(va >= VM_MIN_KERNEL_ADDRESS);

	pt_entry_t * const ptep = pmap_kernel_pte(va);

	/* Build the new PTE. */
	const pt_entry_t npte = pmap_make_pte(pa, prot, flags | PMAP_WIRED);

	/*
	 * If this is an EXEC mapping, then we have to ensure that
	 * the I$ doesn't load stale data.
	 */
	if (__predict_false(prot & UVM_PROT_EXEC)) {
#if MMU_CONFIG_68040_CLASS
		if (MMU_IS_68040_CLASS) {
			atc_flags |= ATC_F_BC;
		}
#endif
	}

	/* There must not be a valid PTE here. */
	KASSERT(! pte_valid_p(pte_load(ptep)));

	/* Set the new PTE. */
	pte_store(ptep, npte);

	pmap_stat_update(pmap, resident_count, 1);
	pmap_stat_update(pmap, wired_count, 1);

	/*
	 * See comments in pmap_pv_enter() as for why we hit the ATC here.
	 * This *should* be unnecessary because this is a wired kernel
	 * mapping and no demand-page-ins should have happened at this
	 * VA, but we're erring on the side of caution for now.
	 */
	pmap_atc_invalidate(pmap_kernel(), va, pa, atc_flags);

	/* if PMAP_CPU_LOCAL, we can avoid sending a TLB shootdown */
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
	uint16_t atc_flags;

	KASSERT(va >= VM_MIN_KERNEL_ADDRESS);

	for (ptep = pmap_kernel_pte(va); size != 0;
	     ptep++, size -= PAGE_SIZE, va += PAGE_SIZE) {
		opte = pte_load(ptep);
		if (pte_valid_p(opte)) {
			KASSERT(! pte_managed_p(opte));
			KASSERT(pte_wired_p(opte));

			atc_flags = 0;

			/* See pmap_remove_mapping(). */
			/* XXX We skip IC here.  On purpose. */
			if (cputype < CPU_68040 && !pte_ci_p(opte)) {
				atc_flags |= ATC_F_DC;
			}

			/* Zap the mapping. */
			pte_store(ptep, 0);
			pmap_atc_invalidate(pmap_kernel(), va, 0, atc_flags);
			count++;
		}
	}

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
	pt_entry_t actual, opte, npte, *ptep;

	PMAP_LOCK(pmap);

	ptep = pmap_pte_lookup(pmap, va, &pt);
	KASSERT(ptep != NULL);

	for (opte = pte_load(ptep);; opte = actual) {
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
		actual = pte_update(ptep, opte, npte);
		if (__predict_true(actual == opte)) {
			pmap_stat_update(pmap, wired_count, -1);
			break;
		}
	}

	pmap_table_release(pmap, pt, NULL);

	PMAP_UNLOCK(pmap);
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
static bool
pmap_extract_pte(pt_entry_t *ptep, vaddr_t va, paddr_t *pap, int *flagsp)
{
	pt_entry_t pte;

	pte = pte_load(ptep);
	if (__predict_true(pte_valid_p(pte))) {
		if (__predict_true(pap != NULL)) {
			*pap = pte_pa(pte) | (va & PAGE_MASK);
		}
		if (__predict_false(flagsp != NULL)) {
			*flagsp =
			    (pte_wired_p(pte) ? PMAP_WIRED : 0) |
			    (pte_ci_p(pte) ? PMAP_NOCACHE : 0);
		}
		return true;
	}
	return false;
}

bool
pmap_extract_info(pmap_t pmap, vaddr_t va, paddr_t *pap, int *flagsp)
{
	struct pmap_table *pt;
	pt_entry_t *ptep;
	bool rv = false;

	if (__predict_true(pmap == pmap_kernel())) {
		if (__predict_true(va < kernel_virtual_end)) {
			ptep = pmap_kernel_pte(va);
			rv = pmap_extract_pte(ptep, va, pap, flagsp);
		}
		return rv;
	}

	PMAP_LOCK(pmap);
	ptep = pmap_pte_lookup(pmap, va, &pt);
	if (__predict_true(ptep != NULL)) {
		rv = pmap_extract_pte(ptep, va, pap, flagsp);
		pmap_table_release(pmap, pt, NULL);
	}
	PMAP_UNLOCK(pmap);

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
void
pmap_update(pmap_t pmap)
{
	struct pmap_garbage g;

	/* Process outstanding ATC invalidations. */
	pmap_atc_process();

	/*
	 * The kernel pmap is special and doesn't free page tables
	 * or defer freeing of PV entries, because pmap_update() can
	 * be called in an interrupt context.
	 */
	if (pmap == pmap_kernel()) {
		KASSERT(TAILQ_EMPTY(&pmap->pm_garbage.g_ptpages));
		KASSERT(LIST_EMPTY(&pmap->pm_garbage.g_pvlist));
		return;
	}

	KASSERT(cpuinfo.ci_idepth == 0);
	PMAP_LOCK(pmap);
	pmap_garbage_transfer(&g, &pmap->pm_garbage);
	PMAP_UNLOCK(pmap);

	pmap_garbage_collect(&g);
}

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
	struct pmap_percpu *pc = pmap_percpu();

	KASSERT(l == curlwp);
	KASSERT(pc->pc_atc.atc_count == 0);

	/*
	 * Because the kernel has a separate root pointer, we don't
	 * need to activate the kernel pmap.
	 */
	if (pmap != pmap_kernel()) {
		PMAP_LOCK(pmap);
		pmap_load_urp(pmap->pm_lev1pa);
		PMAP_UNLOCK(pmap);
	}
	pc->pc_pmap = pmap;
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
	/*
	 * There's no real work for us to do here.  We continue to live
	 * on this lev1map until the next pass through pmap_activate()
	 * on this CPU.  All pmap_deactivate() calls are paired with a
	 * nearly-immediate pmap_activate().
	 */
}

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
	vaddr_t const dstva = cpuinfo.ci_tmpva_dst;
	pt_entry_t * const dst_ptep = pmap_kernel_pte(dstva);

	/* Build the new PTE. */
	const pt_entry_t dst_pte =
	    pmap_make_pte(pa, UVM_PROT_READ | UVM_PROT_WRITE, flags);

	/* Set the new PTE. */
	KASSERT(! pte_valid_p(pte_load(dst_ptep)));
	pte_store(dst_ptep, dst_pte);
	/* XXX Possibly being over-cautious here; see pmap_kenter_pa(). */
	TBIS(dstva);

	/* Zero the page. */
	zeropage((void *)dstva);

	/* Invalidate the PTEs. */
	pte_store(dst_ptep, 0);
	TBIS(dstva);
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
	vaddr_t const srcva = cpuinfo.ci_tmpva_src;
	vaddr_t const dstva = cpuinfo.ci_tmpva_dst;
	pt_entry_t * const src_ptep = pmap_kernel_pte(srcva);
	pt_entry_t * const dst_ptep = pmap_kernel_pte(dstva);

	/* Build the new PTEs. */
	const pt_entry_t src_pte =
	    pmap_make_pte(src, UVM_PROT_READ, flags);
	const pt_entry_t dst_pte =
	    pmap_make_pte(dst, UVM_PROT_READ | UVM_PROT_WRITE, flags);

	/* Set the new PTEs. */
	KASSERT(! pte_valid_p(pte_load(src_ptep)));
	pte_store(src_ptep, src_pte);
	/* XXX Possibly being over-cautious here; see pmap_kenter_pa(). */
	TBIS(srcva);

	KASSERT(! pte_valid_p(pte_load(dst_ptep)));
	pte_store(dst_ptep, dst_pte);
	/* XXX Possibly being over-cautious here; see pmap_kenter_pa(). */
	TBIS(dstva);

	/* Copy the page. */
	copypage((void *)srcva, (void *)dstva);

	/* Invalidate the PTEs. */
	pte_store(src_ptep, 0);
	TBIS(srcva);

	pte_store(dst_ptep, 0);
	TBIS(dstva);
}

/*
 * pmap_testbit:
 *
 *	Test the modified / referenced bits of a physical page.
 */
static bool
pmap_testbit(struct vm_page *pg, pt_entry_t bit)
{
	struct pv_entry *pv;
	pt_entry_t combined_pte = 0;

	PVH_LOCK(pg);

	combined_pte |= VM_MDPAGE_UM(pg);

	for (pv = VM_MDPAGE_PVS(pg);
	     (combined_pte & bit) == 0 && pv != NULL;
	     pv = pv->pv_next) {
		combined_pte |= pte_load(pmap_pv_pte(pv));
	}

	VM_MDPAGE_ADD_UM(pg, combined_pte);

	PVH_UNLOCK(pg);

	return (combined_pte & bit) != 0;
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
	return pmap_testbit(pg, PTE_U);
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
	return pmap_testbit(pg, PTE_M);
}

/*
 * pmap_changebit:
 *
 *	Test-and-change various bits (including mod/ref bits).
 *	Returns the accumulated previously-set PTE bits.
 */
static pt_entry_t
pmap_changebit(struct vm_page *pg, pt_entry_t set, pt_entry_t mask)
{
	struct pv_entry *pv;
	pt_entry_t *ptep, combined_pte = 0, opte, npte;
	paddr_t pa = VM_PAGE_TO_PHYS(pg);

	PVH_LOCK(pg);

	/*
	 * Since we need to report if the page was mod/ref'd before
	 * we cleared the bit, we need to seed ourself with the current
	 * state in the vm_page in the event there are no mappings
	 * left to enumerate.
	 */
	combined_pte |= VM_MDPAGE_UM(pg);

	/*
	 * Since we're running over every mapping for the page anyway,
	 * we might as well synchronize any attribute bits that we're
	 * not clearing.
	 */
	for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
		pt_entry_t actual;
		ptep = pmap_pv_pte(pv);
		for (opte = pte_load(ptep);; opte = actual) {
			npte = (opte | set) & mask;
			combined_pte |= opte;

			if (opte == npte) {
				break;
			}

			actual = pte_update(ptep, opte, npte);
			if (__predict_true(actual == opte)) {
				/*
				 * We must flush the ATC even if it's not a
				 * "critical" bit, because the MMU will only
				 * write-back a PTE when the U or M bits
				 * transition from 0 to 1.  If we clear them
				 * in our tracking structure and in the tables
				 * in memory but not the ATC, then a sticky
				 * ATC entry might cause a future U or M
				 * event to be missed.
				 * 
				 * Cache push is only necessary if
				 * the critical page attribute (W, CM)
				 * were changed.  If we're just clearing
				 * U or M, no push is necessary.
				 */
				pmap_atc_invalidate_pv(pv, pa,
				    ((opte ^ npte) & PTE_CRIT_BITS)
				     ? ATC_F_BC : 0);
				break;
			}
			/* Lost race, try again. */
		}
	}

	/*
	 * Update any attributes we looked at, clear the ones we're clearing.
	 */
	VM_MDPAGE_SET_UM(pg, (combined_pte | set) & mask);

	PVH_UNLOCK(pg);

	pmap_atc_process();

	return combined_pte;
}

/*
 * pmap_clear_modify:		[ INTERFACE ]
 *
 *	Clear the modify bits on the specified physical page.
 */
bool
pmap_clear_modify(struct vm_page *pg)
{
	return (pmap_changebit(pg, 0, (pt_entry_t)~PTE_M) & PTE_M) != 0;
}

/*
 * pmap_clear_reference:	[ INTERFACE ]
 *
 *	Clear the reference bit on the specified physical page.
 */
bool
pmap_clear_reference(struct vm_page *pg)
{
	return (pmap_changebit(pg, 0, (pt_entry_t)~PTE_U) & PTE_U) != 0;
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

/*
 * The kernel_lev1map is a little tricky.  This one is used only by
 * the primary CPU.  Each secondary CPU has its own lev1map and its
 * own lev2 and lev3 table that map its cpu_info at a fixed address
 * (CPUINFO_VA).  These are all allocated in a single page that also
 * holds that CPU's cpu_info.  It looks like this:
 *
 *	(high address)
 *	+----------------------------------------+
 *	|                                        |
 *	|                                        |
 *	|         CPUn lev1map (512 bytes)       |
 *	|                                        |
 *	|                                        |
 *	+----------------------------------------+
 *	|                                        |
 *	|                                        |
 *	| CPUn CPUINFO_VA lev2 table (512 bytes) |
 *	|                                        |
 *	|                                        |
 *	+----------------------------------------+
 *	|                                        |
 *	| CPUn CPUINFO_VA lev3 table (256 bytes) |
 *	|                                        |
 *	+----------------------------------------+
 *	/                                        /
 *	\                                        \
 *	/                                        /
 *	\        (unused space ??? bytes)        \
 *	/                                        /
 *	\                                        \
 *	/                                        /
 *	+----------------------------------------+
 *	|                                        |
 *	|                                        |
 *	|                                        |
 *	|      CPUn cpu_info (<= 2048 bytes)     |
 *	|                                        |
 *	|                                        |
 *	|                                        |
 *	+----------------------------------------+
 *	(low address)
 *
 * Each secondary CPU gets its own kernel lev1map to stash in its %srp,
 * but it is initialized with the contents of the main kernel_lev1map
 * except for the L1 slot that maps CPUINFO_VA.  That particular slot
 * points to that CPU's CPUINFO_VA lev2 table, which in turn has a
 * slot that points to that CPU's CPUINFO_VA lev3 table, which has a
 * slot that maps this particular page at CPUINFO_VA.
 *
 * AND BECAUSE YOU ASKED!  YES!  If we happen to be running on a
 * MP system that has a 68020+68851 or a 68030, then the MMU will
 * be configured to use the same 3-level table structure as the
 * 68040/68060, and thus we can use this same layout.
 */
static pt_entry_t *kernel_lev1map __read_mostly;

#if defined(PMAP_ENABLE_LOCKS)
static kmutex_t pmap_growkernel_lock __cacheline_aligned;

/*
 * pmap_percpu_alloc:
 *
 *	Allocate a per-cpu page.
 */
struct cpu_info *
pmap_percpu_alloc(void)
{
	/*
	 * We're supposed to map PTs cache-inhibited, but we do not want
	 * the cpu_info to be cache-inhibited, so what do we do?
	 *
	 * Well, these PTs aren't going to be accessed very frequently
	 * at all.  Once at setup, and maybe again when pmap_growkernel()
	 * is called.  So we will map the page as write-through.
	 */
	vaddr_t va = pmap_vpage_alloc(true/*nowait*/, PMAP_WRITE_THROUGH, NULL);
	if (__predict_false(va == 0)) {
		panic("%s: unable to allocate per-cpu page", __func__);
	}

	return (struct cpu_info *)va;
}

/*
 * pmap_percpu_init:
 *
 *	Initialize a per-cpu page.  Must be called only after the
 *	cpu_info associated with this per-cpu page has been linked
 *	into cpu_info_list.
 */
void
pmap_percpu_init(struct cpu_info *ci)
{
	vaddr_t va = (vaddr_t)ci, lev1, lev2, lev3;
	paddr_t pgpa, lev1pa, lev2pa, lev3pa;

	KASSERT(! CPU_IS_PRIMARY(ci));

	if (! pmap_extract(pmap_kernel(), va, &pgpa)) {
		panic("%s: can't get PA for per-cpu VA=0x%lx",
		    __func__, va);
	}

	/* Compute the addresses of the L1, L2, and L3 tables. */
	lev1   = (va   + PAGE_SIZE) - TBL40_L1_SIZE;
	lev1pa = (pgpa + PAGE_SIZE) - TBL40_L1_SIZE;

	lev2   = lev1   - TBL40_L2_SIZE;
	lev2pa = lev1pa - TBL40_L2_SIZE;

	lev3   = lev2   - TBL40_L3_SIZE;
	lev3pa = lev2pa - TBL40_L3_SIZE;

	mutex_enter(&pmap_growkernel_lock);

	/* Copy the base kernel_lev1map into the lev1 map. */
	memcpy((void *)lev1, kernel_lev1map, TBL40_L1_SIZE);

	/* Enter the lev2 table into the lev1 map. */
	((pt_entry_t *)lev1)[LA40_RI(CPUINFO_VA)] = pmap_ste_proto | lev2pa;

	/* Enter the lev3 table into the lev2 table. */
	((pt_entry_t *)lev2)[LA40_PI(CPUINFO_VA)] = pmap_ste_proto | lev3pa;

	/* Enter the page into the lev3 table. */
	((pt_entry_t *)lev3)[LA40_PGI(CPUINFO_VA)] =
	    pmap_make_pte(pgpa, VM_PROT_READ | VM_PROT_WRITE,
			  PMAP_WRITE_THROUGH);

	mutex_exit(&pmap_growkernel_lock);

	/* Allocate per-cpu temp VAs. */
	va = uvm_km_alloc(kernel_map, PAGE_SIZE * 3, 0, UVM_KMF_VAONLY);
	if (__predict_false(va == 0)) {
		panic("%s: unable to allocate per-cpu VAs", __func__);
	}

	ci->ci_tmpva_vmmap = va;
	va += PAGE_SIZE;
	ci->ci_tmpva_src = va;
	va += PAGE_SIZE;
	ci->ci_tmpva_dst = va;
	va += PAGE_SIZE;

	/* Point at the kernel pmap. */
	va = (vaddr_t)ci;
	va += sizeof(struct cpu_info);
	((struct pmap_percpu *)va)->pc_pmap = pmap_kernel();

	/* Fin. */
}
#endif /* MULTIPROCESSOR */

/*
 * pmap_percpu_start:
 *
 *	Tasks to do for the pmap when a CPU starts.
 */
void
pmap_percpu_start(void)
{
	/* Load the null user segment table. */
	pmap_load_urp(null_segtab_pa);
}

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
 * pmap_growkernel_insert_lev1map:
 *
 *	Helper for pmap_growkernel_link_kptpage().
 */
static void
pmap_growkernel_insert_lev1map(vaddr_t va, pt_entry_t lev1ste)
{
	unsigned int idx = LA40_RI(va);

	kernel_lev1map[idx] = lev1ste;

#if defined(MULTIPROCESSOR)
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	vaddr_t pgva, lev1;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (CPU_IS_PRIMARY(ci)) {
			continue;
		}
		pgva = (vaddr_t)ci;
		lev1 = (pgva + PAGE_SIZE) - TBL40_L1_SIZE;
		((pt_entry_t *)lev1)[idx] = lev1ste;
	}
#endif /* MULTIPROCESSOR */
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
	 * uses cpuinfo.ci_tmpva_dst.  cpuinfo.ci_tmpva_src is available
	 * for our use, however, so that's what we used to temporarily
	 * map inner segment table pages.
	 */
	const vaddr_t stpg_va = cpuinfo.ci_tmpva_src;
	pt_entry_t *stpg_pte = pmap_kernel_pte(stpg_va);

	paddr_t stpa, stpg_pa, stpgoff;
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
			pmap_growkernel_insert_lev1map(va,
			    pmap_ste_proto | kernel_stnext_pa);
			kernel_stnext_pa += TBL40_L2_SIZE;
		}
		stpa = kernel_lev1map[LA40_RI(va)] & UTE40_PTA;
		stpg_pa = m68k_trunc_page(stpa);

		pte_store(stpg_pte,
		    pmap_make_pte(stpg_pa, UVM_PROT_READ | UVM_PROT_WRITE,
				  PMAP_WIRED | PMAP_NOCACHE));
		TBIS(stpg_va);

		stpgoff = stpa - stpg_pa;
		stes = (pt_entry_t *)(stpg_va + stpgoff);
		stes[LA40_PI(va)] = pmap_ste_proto | pa;
	}
	pte_store(stpg_pte, 0);
	TBIS(stpg_va);
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
	PMAP_MUTEX_ENTER(&pmap_growkernel_lock);

	KASSERT((kernel_virtual_end & PTPAGEVAOFS) == 0);

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
	vaddr_t va, ptp_pa, pte_va;
	for (va = kernel_virtual_end; va < new_maxkva; va += PTPAGEVASZ) {
		/* Allocate page and link it into the MMU tree. */
		ptp_pa = pmap_growkernel_alloc_page();
		pmap_growkernel_link_kptpage(va, ptp_pa);
		pmap_nkptpages_current_ev.ev_count++;

		/* Map the PT page into the kernel PTE array. */
		pte_va = (vaddr_t)pmap_kernel_pte(va);
		pmap_kenter_pa(pte_va, ptp_pa, UVM_PROT_READ | UVM_PROT_WRITE,
		    PMAP_WIRED | PMAP_NOCACHE);
		/* We invalidate_all below. */
	}
 	kernel_virtual_end = new_maxkva;
 done:
	pmap_maxkva_ev.ev_count32 = new_maxkva;
	pmap_kvalimit_ev.ev_count32 = kernel_virtual_max;

	PMAP_MUTEX_EXIT(&pmap_growkernel_lock);

	/* Also catches new lev1map entries on secondary CPUs. */
	pmap_atc_invalidate_all(ATC_F_SUPER | ATC_F_DC);
	pmap_atc_process();

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
 * pmap_write_text_enter:
 *
 *	Temporarily map a page of kernel text read-write for the
 *	kernel debugger.
 */
bool
pmap_write_text_enter(vaddr_t pgva, struct pmap_write_text_context *ctx)
{
	pt_entry_t *pte = pmap_kernel_pte(pgva);

	pt_entry_t opte = pte_load(pte);
	if (! pte_valid_p(opte)) {
		return false;
	}

	/*
	 * N.B. we use the 68851 PTE bit names here, but in
	 * the case of the kernel text, it all works out vis
	 * a vis the 68040 PTE bits.
	 *
	 * Note the mapping is cache-inhibited to save hair.
	 */
	pt_entry_t npte = (opte & ~PTE51_WP) | PTE51_CI;
	pte_store(pte, npte);
	TBIS(pgva);

	ctx->pgva = pgva;
	ctx->ptep = pte;
	ctx->opte = opte;

	return true;
}

/*
 * pmap_write_text_exit:
 *
 *	Undo the effects of pmap_write_text_enter().
 */
void
pmap_write_text_exit(struct pmap_write_text_context *ctx)
{
	pte_store(ctx->ptep, ctx->opte);
	TBIS(ctx->pgva);
	ICIA();
}

static paddr_t kernel_reloc_offset __read_mostly;
static vaddr_t kernel_reloc_end __read_mostly;

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

	/* Fixed relocation information. */
	m->reloc    = kernel_reloc_offset;
	m->relocend = kernel_reloc_end;

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
 * pmap_pa_has_static_mapping:
 *
 *	Returns true if the specified PA (and length) has a static
 *	mapping, and what that mapping is and its properties.
 */
bool
pmap_pa_has_static_mapping(paddr_t pa, size_t len, vm_prot_t prot,
    vaddr_t *vap, int *flagsp)
{
#if defined(M68030) || defined(M68040) || defined(M68060)
	/* First, check for Transparent Translation. */
	if (cputype >= CPU_68030 &&
	    mmu_range_is_tt(pa, len, prot, flagsp)) {
		*vap = pa;
		return true;
	}
#endif

	const struct pmap_bootmap *pmbm = machine_bootmap;
	paddr_t lastpg = m68k_btop(pa + (len - 1));
	paddr_t firstpg = m68k_btop(pa);
	paddr_t tfirst, tlast;
	vaddr_t tblva;
	bool need_write = !!(prot & UVM_PROT_WRITE);

	for (; pmbm->pmbm_vaddr != (vaddr_t)-1; pmbm++) {
		if (pmbm->pmbm_flags & (PMBM_F_VAONLY|PMBM_F_KEEPOUT)) {
			continue;
		}
		if (pmbm->pmbm_flags & PMBM_F_FIXEDVA) {
			tblva = pmbm->pmbm_vaddr;
		} else {
			tblva = *pmbm->pmbm_vaddr_ptr;
		}
		tfirst = m68k_btop(pmbm->pmbm_paddr);
		tlast = m68k_btop(pmbm->pmbm_paddr + (pmbm->pmbm_size - 1));

		if (firstpg >= tfirst && lastpg <= tlast) {
			if (need_write &&
			    (pmbm->pmbm_flags & PMBM_F_RO) != 0) {
				return false;
			}
			*vap = tblva + (pa - pmbm->pmbm_paddr);
			if (pmbm->pmbm_flags & PMBM_F_CI) {
				*flagsp = PMAP_NOCACHE;
			} else {
				*flagsp = 0;
			}
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
#if defined(M68030) || defined(M68040) || defined(M68060)
	int map_flags;

	/* First, check for Transparent Translation. */
	if (cputype >= CPU_68030 &&
	    (mmu_range_is_tt(va, len, UVM_PROT_READ, &map_flags) ||
	     mmu_range_is_tt(va, len, UVM_PROT_WRITE, &map_flags) ||
	     mmu_range_is_tt(va, len, UVM_PROT_EXEC, &map_flags))) {
		return true;
	}
#endif

	const struct pmap_bootmap *pmbm = machine_bootmap;
	vaddr_t lastpg = m68k_btop(va + (len - 1));
	vaddr_t firstpg = m68k_btop(va);
	vaddr_t tblva, tfirst, tlast;
	for (; pmbm->pmbm_vaddr != (vaddr_t)-1; pmbm++) {
		if (pmbm->pmbm_flags & (PMBM_F_VAONLY|PMBM_F_KEEPOUT)) {
			continue;
		}
		if (pmbm->pmbm_flags & PMBM_F_FIXEDVA) {
			tblva = pmbm->pmbm_vaddr;
		} else {
			tblva = *pmbm->pmbm_vaddr_ptr;
		}
		tfirst = m68k_btop(tblva);
		tlast = m68k_btop(tblva + (pmbm->pmbm_size - 1));

		if (firstpg >= tfirst && lastpg <= tlast) {
			return true;
		}
	}

	return false;
}

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
 *	((0xffffffff >> PAGE_SHIFT) + 1) * sizeof(pt_entry_t)
 *
 * The traditional name for this virtual array is "Sysmap".
 */
#define	SYSMAP_VA_SIZE	(((0xffffffffU >> PAGE_SHIFT) + 1) * sizeof(pt_entry_t))

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

static vaddr_t	lwp0uarea __read_mostly;
       paddr_t	lwp0upa __read_mostly = (paddr_t)-1;

/* XXX Doesn't belong here. */
paddr_t		avail_start __read_mostly;/* PA of first available page */
paddr_t		avail_end __read_mostly;  /* PA of last available page */

extern char *	kernel_text;
extern char *	etext;
extern void *	msgbufaddr;

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
	paddr_t stnext_endpa, stnext_pa;
	paddr_t pa, kernimg_endpa, kern_lev1pa;
	vaddr_t va, nextva, kern_lev1va;
	pt_entry_t *pte, *epte;
	const struct pmap_bootmap *pmbm;
	int entry_count = 0;

#ifdef SYSMAP_VA
#define	VA_RANGE_DEFAULT	0
#define	VA_RANGE_KPTES		1
#define	INIT_NRANGES		2
#else
#define	VA_RANGE_DEFAULT	0
#define	VA_RANGE_KPTES		0
#define	INIT_NRANGES		1
#endif
#define	MAX_RANGES		8

	struct va_range {
		vaddr_t start_va;
		vaddr_t end_va;
		paddr_t start_ptp;
		paddr_t end_ptp;
	} va_ranges[MAX_RANGES], *var;
	int nranges = INIT_NRANGES, r;

#define	VA_IN_RANGE(va, var)				\
	((va) >= (var)->start_va &&			\
	 ((va) < (var)->end_va || (var)->end_va == 0))

#define	VA_PTE_BASE(va, var)				\
	(&((pt_entry_t *)				\
	    PMAP_BOOTSTRAP_RELOC_PA((var)->start_ptp))[	\
	    m68k_btop((va) - (var)->start_va)])

#define	RELOC(v, t)	*((t *)PMAP_BOOTSTRAP_RELOC_GLOB(&(v)))

	/* Record the relocation offset for kernel crash dumps. */
	RELOC(kernel_reloc_offset, paddr_t) = reloff;

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
	pt_entry_t proto_rw_cwt_pte;	/* read-write, cache-write-through */
	pt_entry_t proto_rw_ci_pte;	/* read-write, cache-inhibited */
	pt_entry_t proto_ste;

	/*
	 * N.B. PTE40_CM_WT == 0, so "write-though" caching has the same
	 * PTE bits on all variants of the MMU.
	 */
	proto_ro_pte     = PTE_VALID|PTE_WIRED|PTE_WP;
	proto_rw_cwt_pte = PTE_VALID|PTE_WIRED;

	if (is_68040_class) {
		proto_rw_pte    = PTE_VALID|PTE_WIRED|PTE40_CM_CB;
		proto_rw_ci_pte = PTE_VALID|PTE_WIRED|PTE40_CM_NC_SER;
	} else {
		proto_rw_pte    = PTE_VALID|PTE_WIRED;
		proto_rw_ci_pte = PTE_VALID|PTE_WIRED|PTE51_CI;
	}
	proto_ste = DTE51_U | DT51_SHORT;

	/*
	 * Allocate some important fixed virtual (and physical) addresses.
	 * We use the sum total of this initial mapped kernel space to
	 * determine how many initial kernel PT pages to allocate.  The
	 * things that consume physical space will come first, and the
	 * virtual-space-{only,mostly} things come at the end.
	 *
	 *	lwp0upa		lwp0 u-area	USPACE	(p)
	 *	lwp0uarea				(v)
	 *
	 *	Sysseg_pa	kernel lev1map	PAGE_SIZE (p)
	 *	kernel_lev1map			PAGE_SIZE (v, ci)
	 *
	 *	^^^^ end of simple relocation region ^^^^
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
	nextva = PMAP_BOOTSTRAP_PA_TO_VA(nextpa);

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
	 * lwp0 u-area.  If the caller hasn't already told us where
	 * to put it (heh), then allocate it here.
	 *
	 * In either case, we finish setting it up in pmap_bootstrap2().
	 */
	if (RELOC(lwp0upa, paddr_t) == (paddr_t)-1) {
		RELOC(lwp0upa, paddr_t) = nextpa;
		nextpa += USPACE;
	}
	RELOC(lwp0uarea, vaddr_t) = nextva;
	nextva += USPACE;

	size_t nstpages = 0;

	/* kernel level-1 map */
	RELOC(Sysseg_pa, paddr_t) = kern_lev1pa = nextpa;
	nextpa += PAGE_SIZE;
	RELOC(kernel_lev1map, vaddr_t) = kern_lev1va = nextva;
	nextva += PAGE_SIZE;
	nstpages++;

	/* This is the end of the simple relocation region. */
	RELOC(kernel_reloc_end, vaddr_t) = nextva;

	/*
	 * For 3-level configs, we now have space to allocate
	 * inner segment tables.
	 */
	stnext_pa = kern_lev1pa + TBL40_L1_SIZE;
	stnext_endpa = m68k_round_page(stnext_pa);

	/* null segment table */
#ifdef NULL_SEGTAB_PA
	RELOC(null_segtab_pa, paddr_t) = (paddr_t)NULL_SEGTAB_PA;
#else
	RELOC(null_segtab_pa, paddr_t) = nextpa;
	nextpa += PAGE_SIZE;
#endif

	/* pmap temporary map addresses */
	RELOC(cpu_info_primary.ci_tmpva_src, vaddr_t) = nextva;
	nextva += PAGE_SIZE;
	RELOC(cpu_info_primary.ci_tmpva_dst, vaddr_t) = nextva;
	nextva += PAGE_SIZE;

	/* vmmap temporary map address */
	RELOC(cpu_info_primary.ci_tmpva_vmmap, vaddr_t) = nextva;
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
	va_ranges[VA_RANGE_KPTES].start_va = (vaddr_t)SYSMAP_VA;
	va_ranges[VA_RANGE_KPTES].end_va = 0; /* end of the address space */
#else
	RELOC(kernel_ptes, vaddr_t) = nextva;
	nextva += SYSMAP_VA_SIZE;
#endif /* SYSMAP_VA */

	/*
	 * Look through the machine_bootmap looking for any non-fixed
	 * VAs we need to allocate.  These will go into the default
	 * VA range.
	 */
	pmbm = (const struct pmap_bootmap *)
	    PMAP_BOOTSTRAP_RELOC_GLOB(machine_bootmap);
	for (; pmbm->pmbm_vaddr != (vaddr_t)-1; pmbm++) {
		if (pmbm->pmbm_size == 0) {
			continue;
		}
		if (pmbm->pmbm_flags & (PMBM_F_FIXEDVA | PMBM_F_KEEPOUT)) {
			va = m68k_trunc_page(pmbm->pmbm_vaddr);
			if (va < RELOC(kernel_virtual_max, vaddr_t)) {
				RELOC(kernel_virtual_max, vaddr_t) = va;
			}
		} else {
			*(vaddr_t *)
			    PMAP_BOOTSTRAP_RELOC_GLOB(pmbm->pmbm_vaddr_ptr) =
			    nextva;
			nextva += m68k_round_page(pmbm->pmbm_size);
		}
	}

	/* UVM-managed kernel virtual starts here. */
	RELOC(kernel_virtual_start, vaddr_t) = nextva;

	/*
	 * Allocate enough PT pages to map all of physical memory.
	 * This should be sufficient to prevent pmap_growkernel()
	 * from having to do any work before the VM system is set
	 * up.
	 *
	 * XXX mac68k also relies on being able to map the last page
	 * XXX of RAM VA==PA for the mmu-switchoff dance.  Unlike hp300,
	 * XXX this is not at a fixed location.  However, RAM generally
	 * XXX starts at $0000.0000 on Macs, so this calculation should
	 * XXX be sufficient to ensure there is a PTE available for this
	 * XXX purpose.
	 * XXX TODO: Provide a way for cpu_startup() on mac68k to assert
	 * XXX this (export kernel_virtual_end?).
	 */
	nextva += RELOC(physmem, psize_t) << PAGE_SHIFT;
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
	va_ranges[VA_RANGE_DEFAULT].start_va =
	    pmap_trunc_ptpage(VM_MIN_KERNEL_ADDRESS);
	va_ranges[VA_RANGE_DEFAULT].end_va =
	    pmap_round_ptpage(nextva);
	RELOC(kernel_virtual_end, vaddr_t) = nextva;

	/*
	 * Now that we know the end of the default range, look through
	 * the machine_bootmap to see if there are any fixed VA ranges
	 * we need to allocate static PTs for.
	 */
	pmbm = (const struct pmap_bootmap *)
	    PMAP_BOOTSTRAP_RELOC_GLOB(machine_bootmap);
	for (; pmbm->pmbm_vaddr != (vaddr_t)-1; pmbm++) {
		if (pmbm->pmbm_size == 0) {
			continue;
		}
		if ((pmbm->pmbm_flags & (PMBM_F_FIXEDVA | PMBM_F_KEEPOUT)) !=
		    PMBM_F_FIXEDVA) {
			continue;
		}
		/*
		 * Found one; see if there's a range already allocated
		 * for it.
		 */
		for (r = 0; r < nranges; r++) {
			var = &va_ranges[r];
			if (VA_IN_RANGE(pmbm->pmbm_vaddr, var)) {
				break;
			}
		}
		if (r < nranges) {
			/* Range of PTs already exists. */
			continue;
		}
		/*
		 * We need to allocate a range.  If we're already at
		 * the max, I guess we lose.
		 */
		if (r == MAX_RANGES) {
			break;
		}
		r = nranges++;
		va_ranges[r].start_va = pmap_trunc_ptpage(pmbm->pmbm_vaddr);
		va_ranges[r].end_va =
		    pmap_round_ptpage(pmbm->pmbm_vaddr + pmbm->pmbm_size);
	}

	/*
	 * Now, compute the number of PT pages required to map the
	 * required VA ranges and allocate them.
	 */
	size_t nptpages, total_ptpages = 0;
	for (r = 0; r < nranges; r++) {
		var = &va_ranges[r];
		nptpages = (var->end_va - var->start_va) / PTPAGEVASZ;
		var->start_ptp = nextpa;
		nextpa += nptpages * PAGE_SIZE;
		var->end_ptp = nextpa;
		total_ptpages += nptpages;
	}

#ifdef PMAP_MACHINE_CHECK_BOOTSTRAP_ALLOCATIONS
	/*
	 * Right here, the old mac68k Utah pmap_bootstrap1() has a
	 * check to see if the kernel + bootstrap allocations fit
	 * within one of the memory segments mapped by the loader.
	 * This is a hook to accommodate that requirement.
	 */
	void (*alloc_checkfn)(paddr_t, paddr_t) = (void *)
	    PMAP_BOOTSTRAP_RELOC_GLOB(pmap_machine_check_bootstrap_allocations);
	(*alloc_checkfn)(nextpa, reloff);
#endif

	/*
	 * The bulk of the dynamic memory allocation is done (there
	 * may be more below if we have to allocate more inner segment
	 * table pages, but we'll burn that bridge when we come to it).
	 *
	 * Zero out all of these freshly-allocated pages.
	 */
	pte = (pt_entry_t *)PMAP_BOOTSTRAP_RELOC_PA(kernimg_endpa);
	epte = (pt_entry_t *)PMAP_BOOTSTRAP_RELOC_PA(nextpa);
	while (pte < epte) {
		*pte++ = 0;
	}

	/*
	 * Ok, let's get to mapping stuff!  Almost everything is in
	 * the default VA range.
	 */
	var = &va_ranges[VA_RANGE_DEFAULT];

	/*
	 * Kernel text - read-only.
	 *
	 * ...that is, unless, a platform as some quirky requirement
	 * (hello mac68k!).  This hook lets a platform specify an
	 * alternate proto PTE for the kernel text (in the mac68k case,
	 * it will be read/write write-through-cacheable).  Once the
	 * kernel is up and running on its own mappings, machine-specific
	 * code can perform any fixups as necessary.
	 */
#ifndef PMAP_BOOTSTRAP_TEXT_PROTO_PTE
#define	PMAP_BOOTSTRAP_TEXT_PROTO_PTE	proto_ro_pte
#endif
	pa = PMAP_BOOTSTRAP_VA_TO_PA(m68k_trunc_page(&kernel_text));
	pte = VA_PTE_BASE(&kernel_text, var);
	epte = VA_PTE_BASE(&etext, var);
	while (pte < epte) {
		*pte++ = PMAP_BOOTSTRAP_TEXT_PROTO_PTE | pa;
		pa += PAGE_SIZE;
		entry_count++;
	}

	/* Remainder of kernel image - read-write. */
	epte = VA_PTE_BASE(PMAP_BOOTSTRAP_PA_TO_VA(kernimg_endpa), var);
	while (pte < epte) {
		*pte++ = proto_rw_pte | pa;
		pa += PAGE_SIZE;
		entry_count++;
	}

	/* lwp0 u-area - read-write. */
	pa = RELOC(lwp0upa, paddr_t);
	pte = VA_PTE_BASE(RELOC(lwp0uarea, vaddr_t), var);
	epte = VA_PTE_BASE(RELOC(lwp0uarea, vaddr_t) + USPACE, var);
	while (pte < epte) {
		*pte++ = proto_rw_pte | pa;
		pa += PAGE_SIZE;
		entry_count++;
	}

	/* Kernel lev1map - read-write, cache-inhibited. */
	pte = VA_PTE_BASE(kern_lev1va, var);
	*pte = proto_rw_ci_pte | kern_lev1pa;
	entry_count++;

	/*
	 * Kernel leaf PT pages - read-write, cache-inhibited.
	 *
	 * These will be in a different VA range if the machine
	 * defines SYSMAP_VA.
	 */
	va = RELOC(kernel_ptes, vaddr_t);
	pt_entry_t *kptes = (pt_entry_t *)va;
	struct va_range *kpte_var = &va_ranges[VA_RANGE_KPTES];

	for (r = 0; r < nranges; r++) {
		var = &va_ranges[r];
		va = (vaddr_t)(&kptes[m68k_btop(var->start_va)]);
		pte = VA_PTE_BASE(va, kpte_var);
		for (pa = var->start_ptp; pa < var->end_ptp; pa += PAGE_SIZE) {
			*pte++ = proto_rw_ci_pte | pa;
			entry_count++;
		}
	}

	/*
	 * Now perform any machine-specific mappings at VAs
	 * allocated earlier.
	 */
	pmbm = (const struct pmap_bootmap *)
	    PMAP_BOOTSTRAP_RELOC_GLOB(machine_bootmap);
	for (; pmbm->pmbm_vaddr != (vaddr_t)-1; pmbm++) {
		pt_entry_t proto;

		if (pmbm->pmbm_size == 0 ||
		    (pmbm->pmbm_flags & (PMBM_F_VAONLY | PMBM_F_KEEPOUT))) {
			continue;
		}
		if (pmbm->pmbm_flags & PMBM_F_FIXEDVA) {
			va = pmbm->pmbm_vaddr;
		} else {
			va = *(vaddr_t *)
			    PMAP_BOOTSTRAP_RELOC_GLOB(pmbm->pmbm_vaddr_ptr);
		}
		for (r = 0; r < nranges; r++) {
			var = &va_ranges[r];
			if (VA_IN_RANGE(va, var)) {
				break;
			}
		}
		pa = pmbm->pmbm_paddr;
		pte = VA_PTE_BASE(va, var);
		switch (pmbm->pmbm_flags & (PMBM_F_CI|PMBM_F_CWT|PMBM_F_RO)) {
		case PMBM_F_CI|PMBM_F_RO:
			proto = proto_rw_ci_pte | PTE_WP;
			break;
		case PMBM_F_CWT:
			proto = proto_rw_cwt_pte;
			break;
		case PMBM_F_CI:
			proto = proto_rw_ci_pte;
			break;
		case PMBM_F_RO:
			proto = proto_ro_pte;
			break;
		default:
			proto = proto_rw_pte;
			break;
		}
		/* Add in any special PTE bits that might be required. */
		proto |= (pmbm->pmbm_flags & PMBM_F_PTEBITS);
		for (vsize_t size = m68k_round_page(pmbm->pmbm_size);
		     size != 0;
		     va += PAGE_SIZE, pa += PAGE_SIZE, size -= PAGE_SIZE) {
			*pte++ = proto | pa;
			entry_count++;
		}
	}

	/*
	 * Now that all of the individual VAs are mapped in the leaf
	 * tables, it's time to link those tables into the segment
	 * table.
	 *
	 * For the 2-level case, this is trivial.  For the 3-level
	 * case, we will have to allocate inner segment tables.
	 */
	for (r = 0; r < nranges; r++) {
		var = &va_ranges[r];
		if (use_3l) {
			pt_entry_t *stes, *stes1 = (pt_entry_t *)
			    PMAP_BOOTSTRAP_RELOC_PA(kern_lev1pa);
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
#ifdef PMAP_MACHINE_CHECK_BOOTSTRAP_ALLOCATIONS
						(*alloc_checkfn)(nextpa,
						    reloff);
#endif
						/*
						 * Zero out the new inner
						 * segment table page.
						 */
						pte = (pt_entry_t *)
						    PMAP_BOOTSTRAP_RELOC_PA(
						    stnext_pa);
						epte = (pt_entry_t *)
						    PMAP_BOOTSTRAP_RELOC_PA(
						    stnext_endpa);
						while (pte < epte) {
							*pte++ = 0;
						}
					}
					stes1[ri] = proto_ste | stnext_pa;
					stnext_pa += TBL40_L2_SIZE;
				}
				stes = (pt_entry_t *)
				    PMAP_BOOTSTRAP_RELOC_PA(
				    stes1[ri] & UTE40_PTA);
				stes[LA40_PI(va)] = proto_ste | pa;
			}
		} else {
			pt_entry_t *stes = (pt_entry_t *)
			    PMAP_BOOTSTRAP_RELOC_PA(kern_lev1pa);
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

/*
 * PAGE_SIZE should always evaluate to a compile-time constant in this
 * context.
 */
__CTASSERT(PAGE_SIZE != 0);

/*
 * This serves to provide assurance that other bits of m68k code can
 * assume that USPACE is 8KB in size.
 */
__CTASSERT(USPACE == 8192);

/*
 * pmap_bootstrap2:
 *
 *	Phase 2 of bootstrapping virtual memory.  This is called after
 *	the MMU has been enabled to finish setting up run-time-computed
 *	global pmap data and the lwp0 u-area.
 *
 *	Returns the new kernel %sp value for lwp0.
 */
void *
pmap_bootstrap2(void)
{
	/* Setup the MMU class; needed before anything else. */
	pmap_mmuclass_init();

	/* reference the kernel pmap */
	pmap_percpu()->pc_pmap = pmap_kernel();
	pmap_percpu_start();

	/* Early low-level UVM initialization. */
	uvmexp.pagesize = PAGE_SIZE;
	uvm_md_init();

	/* Initialize prototype PTEs; needed before anything else is mapped. */
	pmap_pte_proto_init();

	/* Initialize the kernel pmap. */
	pmap_pinit(pmap_kernel(), Sysseg_pa);

	/* Initialize lwp0 u-area. */
	memset((void *)lwp0uarea, 0, USPACE);
	uvm_lwp_setuarea(&lwp0, lwp0uarea);

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

	PMAP_MUTEX_INIT(&pmap_growkernel_lock, MUTEX_DEFAULT, IPL_NONE);

	return tf;
}

/***************************** PMAP DEBUGGING ********************************/

#ifdef PMAP_DEBUG

void	pmap_test_mod_ref(void);

void
pmap_test_mod_ref(void)
{
	struct vm_page *pg = pmap_page_alloc(false/*nowait*/);
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	vaddr_t va = (vaddr_t)vmmap;
	volatile int *loc = (volatile int *)va;
	int val;
	bool mod, ref;
	bool exp_mod, exp_ref;

	/* Initialize page and mod/ref state to pristine. */
	pmap_zero_page(pa);
	pmap_clear_modify(pg);
	pmap_clear_reference(pg);

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = false;
	exp_ref = false;
	printf("%s: validating pristine page: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/* Enter non-seeded R/W mapping. */
	pmap_enter(pmap_kernel(), va, pa, UVM_PROT_ALL, 0);
	pmap_update(pmap_kernel());

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = false;
	exp_ref = false;
	printf("%s: enter(ALL, 0): mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/* reference page */
	val = *loc;

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = false;
	exp_ref = true;
	printf("%s: ref val=%d: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    val,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/* validate clear behavior. */
	exp_mod = mod;
	exp_ref = ref;
	mod = pmap_clear_modify(pg);
	ref = pmap_clear_reference(pg);
	printf("%s: checking clear 1: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = false;
	exp_ref = false;
	printf("%s: checking clear 2: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/* modify page */
	*loc = 0xff;

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = true;
	exp_ref = true;
	printf("%s: mod 1: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/* write-protect page */
	pmap_page_protect(pg, UVM_PROT_READ);

	mod = pmap_clear_modify(pg);
	ref = pmap_clear_reference(pg);
	exp_mod = true;
	exp_ref = true;
	printf("%s: page_protect(READ) mod 2: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = false;
	exp_ref = false;
	printf("%s: checking clear 3: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/* modify page again */
	pmap_enter(pmap_kernel(), va, pa, UVM_PROT_ALL, 0);
	pmap_update(pmap_kernel());
	*loc = 0xaa;

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = true;
	exp_ref = true;
	printf("%s: mod 3: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/* remove all mappings of page */
	pmap_page_protect(pg, UVM_PROT_NONE);

	mod = pmap_clear_modify(pg);
	ref = pmap_clear_reference(pg);
	exp_mod = true;
	exp_ref = true;
	printf("%s: page_protect(NONE) mod 4: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/* modify page again */
	pmap_enter(pmap_kernel(), va, pa, UVM_PROT_ALL, 0);
	pmap_update(pmap_kernel());
	*loc = 0xaa;

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = true;
	exp_ref = true;
	printf("%s: mod 5: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/* read-protect VA */
	pmap_protect(pmap_kernel(), va, va+PAGE_SIZE, UVM_PROT_READ);
	pmap_update(pmap_kernel());

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = true;
	exp_ref = true;
	printf("%s: va prot(READ) mod 6: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/* none-protect VA */
	pmap_protect(pmap_kernel(), va, va+PAGE_SIZE, UVM_PROT_NONE);
	pmap_update(pmap_kernel());

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = true;
	exp_ref = true;
	printf("%s: va prot(NONE) mod 7: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/* Reset the page. */
	pmap_clear_modify(pg);
	pmap_clear_reference(pg);

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = false;
	exp_ref = false;
	printf("%s: validating pristine page: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	printf("%s: verifying mapping does not exist: (%s)\n",
	    __func__,
	    pmap_extract(pmap_kernel(), va, NULL) ? "FAIL" : "OK");

	/* Enter R-seeded R/W mapping. */
	pmap_enter(pmap_kernel(), va, pa, UVM_PROT_READ|UVM_PROT_WRITE,
	    UVM_PROT_READ);
	pmap_update(pmap_kernel());

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = false;
	exp_ref = true;
	printf("%s: enter(R|W, R): mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/* Enter W-seeded R/W mapping. */
	pmap_enter(pmap_kernel(), va, pa, UVM_PROT_READ|UVM_PROT_WRITE,
	    UVM_PROT_WRITE);
	pmap_update(pmap_kernel());

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = true;
	exp_ref = true;
	printf("%s: enter(R|W, W): mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	pmap_clear_modify(pg);
	pmap_clear_reference(pg);

	mod = pmap_is_modified(pg);
	ref = pmap_is_referenced(pg);
	exp_mod = false;
	exp_ref = false;
	printf("%s: validating pristine page: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/*
	 * Now we're going to reference the page before modifying it
	 * without any intervening clear.  The MMU should update the
	 * PTE once for the ref, and a second time for the mod, *even
	 * if the ATC entry is still valid*.
	 *
	 * We are going to fetch the PTEs directly to validate the
	 * MMU's behavior.
	 */
	pt_entry_t *ptep = pmap_kernel_pte(va);
	pt_entry_t ref_pte, mod_pte;
	int s = splhigh();
	val = *loc;
	ref_pte = pte_load(ptep);
	*loc = 0;
	mod_pte = pte_load(ptep);
	splx(s);

	mod = (mod_pte & PTE_M) != 0;
	ref = (ref_pte & PTE_U) != 0;
	exp_mod = true;
	exp_ref = true;
	printf("%s: validating MMU behavior: mod=%d(%d) ref=%d(%d) (%s)\n",
	    __func__,
	    mod, exp_mod, ref, exp_ref,
	    mod == exp_mod && ref == exp_ref ? "OK" : "FAIL");

	/* all done. */
	pmap_remove(pmap_kernel(), va, va+PAGE_SIZE);
	pmap_update(pmap_kernel());

	printf("%s: done\n", __func__);
}

#endif /* PMAP_DEBUG */
