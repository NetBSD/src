/*	$NetBSD: pmap.c,v 1.47.2.1 2007/03/13 16:50:03 ad Exp $	*/
/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com> of Allegro Networks, Inc.
 *
 * Support for PPC64 Bridge mode added by Sanjay Lal <sanjayl@kymasys.com>
 * of Kyma Systems LLC.
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
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.47.2.1 2007/03/13 16:50:03 ad Exp $");

#include "opt_ppcarch.h"
#include "opt_altivec.h"
#include "opt_pmap.h"
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <sys/device.h>		/* for evcnt */
#include <sys/systm.h>

#if __NetBSD_Version__ < 105010000
#include <vm/vm.h>
#include <vm/vm_kern.h>
#define	splvm()		splimp()
#endif

#include <uvm/uvm.h>

#include <machine/pcb.h>
#include <machine/powerpc.h>
#include <powerpc/spr.h>
#include <powerpc/oea/sr_601.h>
#include <powerpc/bat.h>
#include <powerpc/stdarg.h>

#if defined(DEBUG) || defined(PMAPCHECK)
#define	STATIC
#else
#define	STATIC	static
#endif

#ifdef ALTIVEC
int pmap_use_altivec;
#endif

volatile struct pteg *pmap_pteg_table;
unsigned int pmap_pteg_cnt;
unsigned int pmap_pteg_mask;
#ifdef PMAP_MEMLIMIT
paddr_t pmap_memlimit = PMAP_MEMLIMIT;
#else
paddr_t pmap_memlimit = -PAGE_SIZE;		/* there is no limit */
#endif

struct pmap kernel_pmap_;
unsigned int pmap_pages_stolen;
u_long pmap_pte_valid;
#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
u_long pmap_pvo_enter_depth;
u_long pmap_pvo_remove_depth;
#endif

int physmem;
#ifndef MSGBUFADDR
extern paddr_t msgbuf_paddr;
#endif

static struct mem_region *mem, *avail;
static u_int mem_cnt, avail_cnt;

#ifdef __HAVE_PMAP_PHYSSEG
/*
 * This is a cache of referenced/modified bits.
 * Bits herein are shifted by ATTRSHFT.
 */
#define	ATTR_SHFT	4
struct pmap_physseg pmap_physseg;
#endif

/*
 * The following structure is aligned to 32 bytes 
 */
struct pvo_entry {
	LIST_ENTRY(pvo_entry) pvo_vlink;	/* Link to common virt page */
	TAILQ_ENTRY(pvo_entry) pvo_olink;	/* Link to overflow entry */
	struct pte pvo_pte;			/* Prebuilt PTE */
	pmap_t pvo_pmap;			/* ptr to owning pmap */
	vaddr_t pvo_vaddr;			/* VA of entry */
#define	PVO_PTEGIDX_MASK	0x0007		/* which PTEG slot */
#define	PVO_PTEGIDX_VALID	0x0008		/* slot is valid */
#define	PVO_WIRED		0x0010		/* PVO entry is wired */
#define	PVO_MANAGED		0x0020		/* PVO e. for managed page */
#define	PVO_EXECUTABLE		0x0040		/* PVO e. for executable page */
#define	PVO_WIRED_P(pvo)	((pvo)->pvo_vaddr & PVO_WIRED)
#define	PVO_MANAGED_P(pvo)	((pvo)->pvo_vaddr & PVO_MANAGED)
#define	PVO_EXECUTABLE_P(pvo)	((pvo)->pvo_vaddr & PVO_EXECUTABLE)
#define	PVO_ENTER_INSERT	0		/* PVO has been removed */
#define	PVO_SPILL_UNSET		1		/* PVO has been evicted */
#define	PVO_SPILL_SET		2		/* PVO has been spilled */
#define	PVO_SPILL_INSERT	3		/* PVO has been inserted */
#define	PVO_PMAP_PAGE_PROTECT	4		/* PVO has changed */
#define	PVO_PMAP_PROTECT	5		/* PVO has changed */
#define	PVO_REMOVE		6		/* PVO has been removed */
#define	PVO_WHERE_MASK		15
#define	PVO_WHERE_SHFT		8
} __attribute__ ((aligned (32)));
#define	PVO_VADDR(pvo)		((pvo)->pvo_vaddr & ~ADDR_POFF)
#define	PVO_PTEGIDX_GET(pvo)	((pvo)->pvo_vaddr & PVO_PTEGIDX_MASK)
#define	PVO_PTEGIDX_ISSET(pvo)	((pvo)->pvo_vaddr & PVO_PTEGIDX_VALID)
#define	PVO_PTEGIDX_CLR(pvo)	\
	((void)((pvo)->pvo_vaddr &= ~(PVO_PTEGIDX_VALID|PVO_PTEGIDX_MASK)))
#define	PVO_PTEGIDX_SET(pvo,i)	\
	((void)((pvo)->pvo_vaddr |= (i)|PVO_PTEGIDX_VALID))
#define	PVO_WHERE(pvo,w)	\
	((pvo)->pvo_vaddr &= ~(PVO_WHERE_MASK << PVO_WHERE_SHFT), \
	 (pvo)->pvo_vaddr |= ((PVO_ ## w) << PVO_WHERE_SHFT))

TAILQ_HEAD(pvo_tqhead, pvo_entry);
struct pvo_tqhead *pmap_pvo_table;	/* pvo entries by ptegroup index */
struct pvo_head pmap_pvo_kunmanaged = LIST_HEAD_INITIALIZER(pmap_pvo_kunmanaged);	/* list of unmanaged pages */
struct pvo_head pmap_pvo_unmanaged = LIST_HEAD_INITIALIZER(pmap_pvo_unmanaged);	/* list of unmanaged pages */

struct pool pmap_pool;		/* pool for pmap structures */
struct pool pmap_upvo_pool;	/* pool for pvo entries for unmanaged pages */
struct pool pmap_mpvo_pool;	/* pool for pvo entries for managed pages */

/*
 * We keep a cache of unmanaged pages to be used for pvo entries for
 * unmanaged pages.
 */
struct pvo_page {
	SIMPLEQ_ENTRY(pvo_page) pvop_link;
};
SIMPLEQ_HEAD(pvop_head, pvo_page);
struct pvop_head pmap_upvop_head = SIMPLEQ_HEAD_INITIALIZER(pmap_upvop_head);
struct pvop_head pmap_mpvop_head = SIMPLEQ_HEAD_INITIALIZER(pmap_mpvop_head);
u_long pmap_upvop_free;
u_long pmap_upvop_maxfree;
u_long pmap_mpvop_free;
u_long pmap_mpvop_maxfree;

STATIC void *pmap_pool_ualloc(struct pool *, int);
STATIC void *pmap_pool_malloc(struct pool *, int);

STATIC void pmap_pool_ufree(struct pool *, void *);
STATIC void pmap_pool_mfree(struct pool *, void *);

static struct pool_allocator pmap_pool_mallocator = {
	.pa_alloc = pmap_pool_malloc,
	.pa_free = pmap_pool_mfree,
	.pa_pagesz = 0,
};

static struct pool_allocator pmap_pool_uallocator = {
	.pa_alloc = pmap_pool_ualloc,
	.pa_free = pmap_pool_ufree,
	.pa_pagesz = 0,
};

#if defined(DEBUG) || defined(PMAPCHECK) || defined(DDB)
void pmap_pte_print(volatile struct pte *);
void pmap_pteg_check(void);
void pmap_pteg_dist(void);
void pmap_print_pte(pmap_t, vaddr_t);
void pmap_print_mmuregs(void);
#endif

#if defined(DEBUG) || defined(PMAPCHECK)
#ifdef PMAPCHECK
int pmapcheck = 1;
#else
int pmapcheck = 0;
#endif
void pmap_pvo_verify(void);
STATIC void pmap_pvo_check(const struct pvo_entry *);
#define	PMAP_PVO_CHECK(pvo)	 		\
	do {					\
		if (pmapcheck)			\
			pmap_pvo_check(pvo);	\
	} while (0)
#else
#define	PMAP_PVO_CHECK(pvo)	do { } while (/*CONSTCOND*/0)
#endif
STATIC int pmap_pte_insert(int, struct pte *);
STATIC int pmap_pvo_enter(pmap_t, struct pool *, struct pvo_head *,
	vaddr_t, paddr_t, register_t, int);
STATIC void pmap_pvo_remove(struct pvo_entry *, int, struct pvo_head *);
STATIC void pmap_pvo_free(struct pvo_entry *);
STATIC void pmap_pvo_free_list(struct pvo_head *);
STATIC struct pvo_entry *pmap_pvo_find_va(pmap_t, vaddr_t, int *); 
STATIC volatile struct pte *pmap_pvo_to_pte(const struct pvo_entry *, int);
STATIC struct pvo_entry *pmap_pvo_reclaim(struct pmap *);
STATIC void pvo_set_exec(struct pvo_entry *);
STATIC void pvo_clear_exec(struct pvo_entry *);

STATIC void tlbia(void);

STATIC void pmap_release(pmap_t);
STATIC void *pmap_boot_find_memory(psize_t, psize_t, int);

static uint32_t pmap_pvo_reclaim_nextidx;
#ifdef DEBUG
static int pmap_pvo_reclaim_debugctr;
#endif

#define	VSID_NBPW	(sizeof(uint32_t) * 8)
static uint32_t pmap_vsid_bitmap[NPMAPS / VSID_NBPW];

static int pmap_initialized;

#if defined(DEBUG) || defined(PMAPDEBUG)
#define	PMAPDEBUG_BOOT		0x0001
#define	PMAPDEBUG_PTE		0x0002
#define	PMAPDEBUG_EXEC		0x0008
#define	PMAPDEBUG_PVOENTER	0x0010
#define	PMAPDEBUG_PVOREMOVE	0x0020
#define	PMAPDEBUG_ACTIVATE	0x0100
#define	PMAPDEBUG_CREATE	0x0200
#define	PMAPDEBUG_ENTER		0x1000
#define	PMAPDEBUG_KENTER	0x2000
#define	PMAPDEBUG_KREMOVE	0x4000
#define	PMAPDEBUG_REMOVE	0x8000

unsigned int pmapdebug = 0;

# define DPRINTF(x)		printf x
# define DPRINTFN(n, x)		if (pmapdebug & PMAPDEBUG_ ## n) printf x
#else
# define DPRINTF(x)
# define DPRINTFN(n, x)
#endif


#ifdef PMAPCOUNTERS
#define	PMAPCOUNT(ev)	((pmap_evcnt_ ## ev).ev_count++)
#define	PMAPCOUNT2(ev)	((ev).ev_count++)

struct evcnt pmap_evcnt_mappings =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "pages mapped");
struct evcnt pmap_evcnt_unmappings =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_mappings,
	    "pmap", "pages unmapped");

struct evcnt pmap_evcnt_kernel_mappings =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "kernel pages mapped");
struct evcnt pmap_evcnt_kernel_unmappings =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_kernel_mappings,
	    "pmap", "kernel pages unmapped");

struct evcnt pmap_evcnt_mappings_replaced =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "page mappings replaced");

struct evcnt pmap_evcnt_exec_mappings =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_mappings,
	    "pmap", "exec pages mapped");
struct evcnt pmap_evcnt_exec_cached =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_mappings,
	    "pmap", "exec pages cached");

struct evcnt pmap_evcnt_exec_synced =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages synced");
struct evcnt pmap_evcnt_exec_synced_clear_modify =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages synced (CM)");
struct evcnt pmap_evcnt_exec_synced_pvo_remove =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages synced (PR)");

struct evcnt pmap_evcnt_exec_uncached_page_protect =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages uncached (PP)");
struct evcnt pmap_evcnt_exec_uncached_clear_modify =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages uncached (CM)");
struct evcnt pmap_evcnt_exec_uncached_zero_page =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages uncached (ZP)");
struct evcnt pmap_evcnt_exec_uncached_copy_page =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages uncached (CP)");
struct evcnt pmap_evcnt_exec_uncached_pvo_remove =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, &pmap_evcnt_exec_mappings,
	    "pmap", "exec pages uncached (PR)");

struct evcnt pmap_evcnt_updates =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "updates");
struct evcnt pmap_evcnt_collects =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "collects");
struct evcnt pmap_evcnt_copies =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "copies");

struct evcnt pmap_evcnt_ptes_spilled =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes spilled from overflow");
struct evcnt pmap_evcnt_ptes_unspilled =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes not spilled");
struct evcnt pmap_evcnt_ptes_evicted =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes evicted");

struct evcnt pmap_evcnt_ptes_primary[8] = {
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[0]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[1]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[2]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[3]"),

    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[4]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[5]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[6]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at primary[7]"),
};
struct evcnt pmap_evcnt_ptes_secondary[8] = {
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[0]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[1]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[2]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[3]"),

    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[4]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[5]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[6]"),
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes added at secondary[7]"),
};
struct evcnt pmap_evcnt_ptes_removed =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes removed");
struct evcnt pmap_evcnt_ptes_changed =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "ptes changed");
struct evcnt pmap_evcnt_pvos_reclaimed =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "pvos reclaimed");
struct evcnt pmap_evcnt_pvos_failed =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL,
	    "pmap", "pvo allocation failures");

/*
 * From pmap_subr.c
 */
extern struct evcnt pmap_evcnt_zeroed_pages;
extern struct evcnt pmap_evcnt_copied_pages;
extern struct evcnt pmap_evcnt_idlezeroed_pages;

EVCNT_ATTACH_STATIC(pmap_evcnt_mappings);
EVCNT_ATTACH_STATIC(pmap_evcnt_mappings_replaced);
EVCNT_ATTACH_STATIC(pmap_evcnt_unmappings);

EVCNT_ATTACH_STATIC(pmap_evcnt_kernel_mappings);
EVCNT_ATTACH_STATIC(pmap_evcnt_kernel_unmappings);

EVCNT_ATTACH_STATIC(pmap_evcnt_exec_mappings);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_cached);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_synced);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_synced_clear_modify);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_synced_pvo_remove);

EVCNT_ATTACH_STATIC(pmap_evcnt_exec_uncached_page_protect);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_uncached_clear_modify);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_uncached_zero_page);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_uncached_copy_page);
EVCNT_ATTACH_STATIC(pmap_evcnt_exec_uncached_pvo_remove);

EVCNT_ATTACH_STATIC(pmap_evcnt_zeroed_pages);
EVCNT_ATTACH_STATIC(pmap_evcnt_copied_pages);
EVCNT_ATTACH_STATIC(pmap_evcnt_idlezeroed_pages);

EVCNT_ATTACH_STATIC(pmap_evcnt_updates);
EVCNT_ATTACH_STATIC(pmap_evcnt_collects);
EVCNT_ATTACH_STATIC(pmap_evcnt_copies);

EVCNT_ATTACH_STATIC(pmap_evcnt_ptes_spilled);
EVCNT_ATTACH_STATIC(pmap_evcnt_ptes_unspilled);
EVCNT_ATTACH_STATIC(pmap_evcnt_ptes_evicted);
EVCNT_ATTACH_STATIC(pmap_evcnt_ptes_removed);
EVCNT_ATTACH_STATIC(pmap_evcnt_ptes_changed);

EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 0);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 1);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 2);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 3);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 4);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 5);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 6);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_primary, 7);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 0);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 1);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 2);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 3);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 4);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 5);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 6);
EVCNT_ATTACH_STATIC2(pmap_evcnt_ptes_secondary, 7);

EVCNT_ATTACH_STATIC(pmap_evcnt_pvos_reclaimed);
EVCNT_ATTACH_STATIC(pmap_evcnt_pvos_failed);
#else
#define	PMAPCOUNT(ev)	((void) 0)
#define	PMAPCOUNT2(ev)	((void) 0)
#endif

#define	TLBIE(va)	__asm volatile("tlbie %0" :: "r"(va))

/* XXXSL: this needs to be moved to assembler */
#define	TLBIEL(va)	__asm __volatile("tlbie %0" :: "r"(va))

#define	TLBSYNC()	__asm volatile("tlbsync")
#define	SYNC()		__asm volatile("sync")
#define	EIEIO()		__asm volatile("eieio")
#define	MFMSR()		mfmsr()
#define	MTMSR(psl)	mtmsr(psl)
#define	MFPVR()		mfpvr()
#define	MFSRIN(va)	mfsrin(va)
#define	MFTB()		mfrtcltbl()

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
static inline register_t
mfsrin(vaddr_t va)
{
	register_t sr;
	__asm volatile ("mfsrin %0,%1" : "=r"(sr) : "r"(va));
	return sr;
}
#endif	/* PPC_OEA*/

#if defined (PPC_OEA64_BRIDGE)
extern void mfmsr64 (register64_t *result);
#endif /* PPC_OEA64_BRIDGE */


static inline register_t
pmap_interrupts_off(void)
{
	register_t msr = MFMSR();
	if (msr & PSL_EE)
		MTMSR(msr & ~PSL_EE);
	return msr;
}

static void
pmap_interrupts_restore(register_t msr)
{
	if (msr & PSL_EE)
		MTMSR(msr);
}

static inline u_int32_t
mfrtcltbl(void)
{

	if ((MFPVR() >> 16) == MPC601)
		return (mfrtcl() >> 7);
	else
		return (mftbl());
}

/*
 * These small routines may have to be replaced,
 * if/when we support processors other that the 604.
 */

void
tlbia(void)
{
	char *i;
	
	SYNC();
#if defined(PPC_OEA)
	/*
	 * Why not use "tlbia"?  Because not all processors implement it.
	 *
	 * This needs to be a per-CPU callback to do the appropriate thing
	 * for the CPU. XXX
	 */
	for (i = 0; i < (char *)0x00040000; i += 0x00001000) {
		TLBIE(i);
		EIEIO();
		SYNC();
	}
#elif defined (PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
	printf("Invalidating ALL TLB entries......\n");
	/* This is specifically for the 970, 970UM v1.6 pp. 140. */
	for (i = 0; i <= (void *)0xFF000; i += 0x00001000) {
		TLBIEL(i);
		EIEIO();
		SYNC();
	}
#endif
	TLBSYNC();
	SYNC();
}

static inline register_t
va_to_vsid(const struct pmap *pm, vaddr_t addr)
{
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	return (pm->pm_sr[addr >> ADDR_SR_SHFT] & SR_VSID) >> SR_VSID_SHFT;
#else /* PPC_OEA64 */
#if 0
	const struct ste *ste;
	register_t hash;
	int i;

	hash = (addr >> ADDR_ESID_SHFT) & ADDR_ESID_HASH;

	/*
	 * Try the primary group first
	 */
	ste = pm->pm_stes[hash].stes;
	for (i = 0; i < 8; i++, ste++) {
		if (ste->ste_hi & STE_V) &&
		   (addr & ~(ADDR_POFF|ADDR_PIDX)) == (ste->ste_hi & STE_ESID))
			return ste;
	}

	/*
	 * Then the secondary group.
	 */
	ste = pm->pm_stes[hash ^ ADDR_ESID_HASH].stes;
	for (i = 0; i < 8; i++, ste++) {
		if (ste->ste_hi & STE_V) &&
		   (addr & ~(ADDR_POFF|ADDR_PIDX)) == (ste->ste_hi & STE_ESID))
			return addr;
	}
		
	return NULL;
#else
	/*
	 * Rather than searching the STE groups for the VSID, we know
	 * how we generate that from the ESID and so do that.
	 */
	return VSID_MAKE(addr >> ADDR_SR_SHFT, pm->pm_vsid) >> SR_VSID_SHFT;
#endif
#endif /* PPC_OEA */
}

static inline register_t
va_to_pteg(const struct pmap *pm, vaddr_t addr)
{
	register_t hash;

	hash = va_to_vsid(pm, addr) ^ ((addr & ADDR_PIDX) >> ADDR_PIDX_SHFT);
	return hash & pmap_pteg_mask;
}

#if defined(DEBUG) || defined(PMAPCHECK) || defined(DDB)
/*
 * Given a PTE in the page table, calculate the VADDR that hashes to it.
 * The only bit of magic is that the top 4 bits of the address doesn't
 * technically exist in the PTE.  But we know we reserved 4 bits of the
 * VSID for it so that's how we get it.
 */
static vaddr_t
pmap_pte_to_va(volatile const struct pte *pt)
{
	vaddr_t va;
	uintptr_t ptaddr = (uintptr_t) pt;

	if (pt->pte_hi & PTE_HID)
		ptaddr ^= (pmap_pteg_mask * sizeof(struct pteg));

	/* PPC Bits 10-19  PPC64 Bits 42-51 */
#if defined(PPC_OEA)
	va = ((pt->pte_hi >> PTE_VSID_SHFT) ^ (ptaddr / sizeof(struct pteg))) & 0x3ff;
#elif defined (PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
	va = ((pt->pte_hi >> PTE_VSID_SHFT) ^ (ptaddr / sizeof(struct pteg))) & 0x7ff;
#endif
	va <<= ADDR_PIDX_SHFT;

	/* PPC Bits 4-9  PPC64 Bits 36-41 */
	va |= (pt->pte_hi & PTE_API) << ADDR_API_SHFT;

#if defined(PPC_OEA64)
	/* PPC63 Bits 0-35 */
	/* va |= VSID_TO_SR(pt->pte_hi >> PTE_VSID_SHFT) << ADDR_SR_SHFT; */
#elif defined(PPC_OEA) || defined(PPC_OEA64_BRIDGE)
	/* PPC Bits 0-3 */
	va |= VSID_TO_SR(pt->pte_hi >> PTE_VSID_SHFT) << ADDR_SR_SHFT;
#endif

	return va;
}
#endif

static inline struct pvo_head *
pa_to_pvoh(paddr_t pa, struct vm_page **pg_p)
{
#ifdef __HAVE_VM_PAGE_MD
	struct vm_page *pg;

	pg = PHYS_TO_VM_PAGE(pa);
	if (pg_p != NULL)
		*pg_p = pg;
	if (pg == NULL)
		return &pmap_pvo_unmanaged;
	return &pg->mdpage.mdpg_pvoh;
#endif
#ifdef __HAVE_PMAP_PHYSSEG
	int bank, pg;

	bank = vm_physseg_find(atop(pa), &pg);
	if (pg_p != NULL)
		*pg_p = pg;
	if (bank == -1)
		return &pmap_pvo_unmanaged;
	return &vm_physmem[bank].pmseg.pvoh[pg];
#endif
}

static inline struct pvo_head *
vm_page_to_pvoh(struct vm_page *pg)
{
#ifdef __HAVE_VM_PAGE_MD
	return &pg->mdpage.mdpg_pvoh;
#endif
#ifdef __HAVE_PMAP_PHYSSEG
	return pa_to_pvoh(VM_PAGE_TO_PHYS(pg), NULL);
#endif
}


#ifdef __HAVE_PMAP_PHYSSEG
static inline char *
pa_to_attr(paddr_t pa)
{
	int bank, pg;

	bank = vm_physseg_find(atop(pa), &pg);
	if (bank == -1)
		return NULL;
	return &vm_physmem[bank].pmseg.attrs[pg];
}
#endif

static inline void
pmap_attr_clear(struct vm_page *pg, int ptebit)
{
#ifdef __HAVE_PMAP_PHYSSEG
	*pa_to_attr(VM_PAGE_TO_PHYS(pg)) &= ~(ptebit >> ATTR_SHFT);
#endif
#ifdef __HAVE_VM_PAGE_MD
	pg->mdpage.mdpg_attrs &= ~ptebit;
#endif
}

static inline int
pmap_attr_fetch(struct vm_page *pg)
{
#ifdef __HAVE_PMAP_PHYSSEG
	return *pa_to_attr(VM_PAGE_TO_PHYS(pg)) << ATTR_SHFT;
#endif
#ifdef __HAVE_VM_PAGE_MD
	return pg->mdpage.mdpg_attrs;
#endif
}

static inline void
pmap_attr_save(struct vm_page *pg, int ptebit)
{
#ifdef __HAVE_PMAP_PHYSSEG
	*pa_to_attr(VM_PAGE_TO_PHYS(pg)) |= (ptebit >> ATTR_SHFT);
#endif
#ifdef __HAVE_VM_PAGE_MD
	pg->mdpage.mdpg_attrs |= ptebit;
#endif
}

static inline int
pmap_pte_compare(const volatile struct pte *pt, const struct pte *pvo_pt)
{
	if (pt->pte_hi == pvo_pt->pte_hi
#if 0
	    && ((pt->pte_lo ^ pvo_pt->pte_lo) &
	        ~(PTE_REF|PTE_CHG)) == 0
#endif
	    )
		return 1;
	return 0;
}

static inline void
pmap_pte_create(struct pte *pt, const struct pmap *pm, vaddr_t va, register_t pte_lo)
{
	/*
	 * Construct the PTE.  Default to IMB initially.  Valid bit
	 * only gets set when the real pte is set in memory.
	 *
	 * Note: Don't set the valid bit for correct operation of tlb update.
	 */
#if defined(PPC_OEA)
	pt->pte_hi = (va_to_vsid(pm, va) << PTE_VSID_SHFT)
	    | (((va & ADDR_PIDX) >> (ADDR_API_SHFT - PTE_API_SHFT)) & PTE_API);
	pt->pte_lo = pte_lo;
#elif defined (PPC_OEA64_BRIDGE)
	pt->pte_hi = ((u_int64_t)va_to_vsid(pm, va) << PTE_VSID_SHFT)
	    | (((va & ADDR_PIDX) >> (ADDR_API_SHFT - PTE_API_SHFT)) & PTE_API);
	pt->pte_lo = (u_int64_t) pte_lo;
#elif defined (PPC_OEA64)
#error PPC_OEA64 not supported
#endif /* PPC_OEA */
}

static inline void
pmap_pte_synch(volatile struct pte *pt, struct pte *pvo_pt)
{
	pvo_pt->pte_lo |= pt->pte_lo & (PTE_REF|PTE_CHG);
}

static inline void
pmap_pte_clear(volatile struct pte *pt, vaddr_t va, int ptebit)
{
	/*
	 * As shown in Section 7.6.3.2.3
	 */
	pt->pte_lo &= ~ptebit;
	TLBIE(va);
	SYNC();
	EIEIO();
	TLBSYNC();
	SYNC();
}

static inline void
pmap_pte_set(volatile struct pte *pt, struct pte *pvo_pt)
{
#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
	if (pvo_pt->pte_hi & PTE_VALID)
		panic("pte_set: setting an already valid pte %p", pvo_pt);
#endif
	pvo_pt->pte_hi |= PTE_VALID;

	/*
	 * Update the PTE as defined in section 7.6.3.1
	 * Note that the REF/CHG bits are from pvo_pt and thus should
	 * have been saved so this routine can restore them (if desired).
	 */
	pt->pte_lo = pvo_pt->pte_lo;
	EIEIO();
	pt->pte_hi = pvo_pt->pte_hi;
	TLBSYNC();
	SYNC();
	pmap_pte_valid++;
}

static inline void
pmap_pte_unset(volatile struct pte *pt, struct pte *pvo_pt, vaddr_t va)
{
#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
	if ((pvo_pt->pte_hi & PTE_VALID) == 0)
		panic("pte_unset: attempt to unset an inactive pte#1 %p/%p", pvo_pt, pt);
	if ((pt->pte_hi & PTE_VALID) == 0)
		panic("pte_unset: attempt to unset an inactive pte#2 %p/%p", pvo_pt, pt);
#endif

	pvo_pt->pte_hi &= ~PTE_VALID;
	/*
	 * Force the ref & chg bits back into the PTEs.
	 */
	SYNC();
	/*
	 * Invalidate the pte ... (Section 7.6.3.3)
	 */
	pt->pte_hi &= ~PTE_VALID;
	SYNC();
	TLBIE(va);
	SYNC();
	EIEIO();
	TLBSYNC();
	SYNC();
	/*
	 * Save the ref & chg bits ...
	 */
	pmap_pte_synch(pt, pvo_pt);
	pmap_pte_valid--;
}

static inline void
pmap_pte_change(volatile struct pte *pt, struct pte *pvo_pt, vaddr_t va)
{
	/*
	 * Invalidate the PTE
	 */
	pmap_pte_unset(pt, pvo_pt, va);
	pmap_pte_set(pt, pvo_pt);
}

/*
 * Try to insert the PTE @ *pvo_pt into the pmap_pteg_table at ptegidx
 * (either primary or secondary location).
 *
 * Note: both the destination and source PTEs must not have PTE_VALID set.
 */

STATIC int
pmap_pte_insert(int ptegidx, struct pte *pvo_pt)
{
	volatile struct pte *pt;
	int i;
	
#if defined(DEBUG)
#if defined (PPC_OEA)
	DPRINTFN(PTE, ("pmap_pte_insert: idx 0x%x, pte 0x%x 0x%x\n",
		ptegidx, (unsigned int) pvo_pt->pte_hi, (unsigned int) pvo_pt->pte_lo));
#elif defined (PPC_OEA64_BRIDGE)
	DPRINTFN(PTE, ("pmap_pte_insert: idx 0x%x, pte 0x%016llx 0x%016llx\n",
		ptegidx, (unsigned long long) pvo_pt->pte_hi, 
		(unsigned long long) pvo_pt->pte_lo));

#endif
#endif
	/*
	 * First try primary hash.
	 */
	for (pt = pmap_pteg_table[ptegidx].pt, i = 0; i < 8; i++, pt++) {
		if ((pt->pte_hi & PTE_VALID) == 0) {
			pvo_pt->pte_hi &= ~PTE_HID;
			pmap_pte_set(pt, pvo_pt);
			return i;
		}
	}

	/*
	 * Now try secondary hash.
	 */
	ptegidx ^= pmap_pteg_mask;
	for (pt = pmap_pteg_table[ptegidx].pt, i = 0; i < 8; i++, pt++) {
		if ((pt->pte_hi & PTE_VALID) == 0) {
			pvo_pt->pte_hi |= PTE_HID;
			pmap_pte_set(pt, pvo_pt);
			return i;
		}
	}
	return -1;
}

/*
 * Spill handler.
 *
 * Tries to spill a page table entry from the overflow area.
 * This runs in either real mode (if dealing with a exception spill)
 * or virtual mode when dealing with manually spilling one of the
 * kernel's pte entries.  In either case, interrupts are already
 * disabled.
 */

int
pmap_pte_spill(struct pmap *pm, vaddr_t addr, bool exec)
{
	struct pvo_entry *source_pvo, *victim_pvo, *next_pvo;
	struct pvo_entry *pvo;
	/* XXX: gcc -- vpvoh is always set at either *1* or *2* */
	struct pvo_tqhead *pvoh, *vpvoh = NULL;
	int ptegidx, i, j;
	volatile struct pteg *pteg;
	volatile struct pte *pt;

	ptegidx = va_to_pteg(pm, addr);

	/*
	 * Have to substitute some entry. Use the primary hash for this.
	 * Use low bits of timebase as random generator.  Make sure we are
	 * not picking a kernel pte for replacement.
	 */
	pteg = &pmap_pteg_table[ptegidx];
	i = MFTB() & 7;
	for (j = 0; j < 8; j++) {
		pt = &pteg->pt[i];
		if ((pt->pte_hi & PTE_VALID) == 0 ||
		    VSID_TO_HASH((pt->pte_hi & PTE_VSID) >> PTE_VSID_SHFT)
				!= KERNEL_VSIDBITS)
			break;
		i = (i + 1) & 7;
	}
	KASSERT(j < 8);

	source_pvo = NULL;
	victim_pvo = NULL;
	pvoh = &pmap_pvo_table[ptegidx];
	TAILQ_FOREACH(pvo, pvoh, pvo_olink) {

		/*
		 * We need to find pvo entry for this address...
		 */
		PMAP_PVO_CHECK(pvo);		/* sanity check */

		/*
		 * If we haven't found the source and we come to a PVO with
		 * a valid PTE, then we know we can't find it because all
		 * evicted PVOs always are first in the list.
		 */
		if (source_pvo == NULL && (pvo->pvo_pte.pte_hi & PTE_VALID))
			break;
		if (source_pvo == NULL && pm == pvo->pvo_pmap &&
		    addr == PVO_VADDR(pvo)) {

			/*
			 * Now we have found the entry to be spilled into the
			 * pteg.  Attempt to insert it into the page table.
			 */
			j = pmap_pte_insert(ptegidx, &pvo->pvo_pte);
			if (j >= 0) {
				PVO_PTEGIDX_SET(pvo, j);
				PMAP_PVO_CHECK(pvo);	/* sanity check */
				PVO_WHERE(pvo, SPILL_INSERT);
				pvo->pvo_pmap->pm_evictions--;
				PMAPCOUNT(ptes_spilled);
				PMAPCOUNT2(((pvo->pvo_pte.pte_hi & PTE_HID)
				    ? pmap_evcnt_ptes_secondary
				    : pmap_evcnt_ptes_primary)[j]);

				/*
				 * Since we keep the evicted entries at the
				 * from of the PVO list, we need move this
				 * (now resident) PVO after the evicted
				 * entries.
				 */
				next_pvo = TAILQ_NEXT(pvo, pvo_olink);

				/*
				 * If we don't have to move (either we were the
				 * last entry or the next entry was valid),
				 * don't change our position.  Otherwise 
				 * move ourselves to the tail of the queue.
				 */
				if (next_pvo != NULL &&
				    !(next_pvo->pvo_pte.pte_hi & PTE_VALID)) {
					TAILQ_REMOVE(pvoh, pvo, pvo_olink);
					TAILQ_INSERT_TAIL(pvoh, pvo, pvo_olink);
				}
				return 1;
			}
			source_pvo = pvo;
			if (exec && !PVO_EXECUTABLE_P(source_pvo)) {
				return 0;
			}
			if (victim_pvo != NULL)
				break;
		}

		/*
		 * We also need the pvo entry of the victim we are replacing
		 * so save the R & C bits of the PTE.
		 */
		if ((pt->pte_hi & PTE_HID) == 0 && victim_pvo == NULL &&
		    pmap_pte_compare(pt, &pvo->pvo_pte)) {
			vpvoh = pvoh;			/* *1* */
			victim_pvo = pvo;
			if (source_pvo != NULL)
				break;
		}
	}

	if (source_pvo == NULL) {
		PMAPCOUNT(ptes_unspilled);
		return 0;
	}

	if (victim_pvo == NULL) {
		if ((pt->pte_hi & PTE_HID) == 0)
			panic("pmap_pte_spill: victim p-pte (%p) has "
			    "no pvo entry!", pt);

		/*
		 * If this is a secondary PTE, we need to search
		 * its primary pvo bucket for the matching PVO.
		 */
		vpvoh = &pmap_pvo_table[ptegidx ^ pmap_pteg_mask]; /* *2* */
		TAILQ_FOREACH(pvo, vpvoh, pvo_olink) {
			PMAP_PVO_CHECK(pvo);		/* sanity check */

			/*
			 * We also need the pvo entry of the victim we are
			 * replacing so save the R & C bits of the PTE.
			 */
			if (pmap_pte_compare(pt, &pvo->pvo_pte)) {
				victim_pvo = pvo;
				break;
			}
		}
		if (victim_pvo == NULL)
			panic("pmap_pte_spill: victim s-pte (%p) has "
			    "no pvo entry!", pt);
	}

	/*
	 * The victim should be not be a kernel PVO/PTE entry.
	 */
	KASSERT(victim_pvo->pvo_pmap != pmap_kernel());
	KASSERT(PVO_PTEGIDX_ISSET(victim_pvo));
	KASSERT(PVO_PTEGIDX_GET(victim_pvo) == i);

	/*
	 * We are invalidating the TLB entry for the EA for the
	 * we are replacing even though its valid; If we don't
	 * we lose any ref/chg bit changes contained in the TLB
	 * entry.
	 */
	source_pvo->pvo_pte.pte_hi &= ~PTE_HID;

	/*
	 * To enforce the PVO list ordering constraint that all
	 * evicted entries should come before all valid entries,
	 * move the source PVO to the tail of its list and the
	 * victim PVO to the head of its list (which might not be
	 * the same list, if the victim was using the secondary hash).
	 */
	TAILQ_REMOVE(pvoh, source_pvo, pvo_olink);
	TAILQ_INSERT_TAIL(pvoh, source_pvo, pvo_olink);
	TAILQ_REMOVE(vpvoh, victim_pvo, pvo_olink);
	TAILQ_INSERT_HEAD(vpvoh, victim_pvo, pvo_olink);
	pmap_pte_unset(pt, &victim_pvo->pvo_pte, victim_pvo->pvo_vaddr);
	pmap_pte_set(pt, &source_pvo->pvo_pte);
	victim_pvo->pvo_pmap->pm_evictions++;
	source_pvo->pvo_pmap->pm_evictions--;
	PVO_WHERE(victim_pvo, SPILL_UNSET);
	PVO_WHERE(source_pvo, SPILL_SET);

	PVO_PTEGIDX_CLR(victim_pvo);
	PVO_PTEGIDX_SET(source_pvo, i);
	PMAPCOUNT2(pmap_evcnt_ptes_primary[i]);
	PMAPCOUNT(ptes_spilled);
	PMAPCOUNT(ptes_evicted);
	PMAPCOUNT(ptes_removed);

	PMAP_PVO_CHECK(victim_pvo);
	PMAP_PVO_CHECK(source_pvo);
	return 1;
}

/*
 * Restrict given range to physical memory
 */
void
pmap_real_memory(paddr_t *start, psize_t *size)
{
	struct mem_region *mp;
	
	for (mp = mem; mp->size; mp++) {
		if (*start + *size > mp->start
		    && *start < mp->start + mp->size) {
			if (*start < mp->start) {
				*size -= mp->start - *start;
				*start = mp->start;
			}
			if (*start + *size > mp->start + mp->size)
				*size = mp->start + mp->size - *start;
			return;
		}
	}
	*size = 0;
}

/*
 * Initialize anything else for pmap handling.
 * Called during vm_init().
 */
void
pmap_init(void)
{
#ifdef __HAVE_PMAP_PHYSSEG
	struct pvo_tqhead *pvoh;
	int bank;
	long sz;
	char *attr;

	pvoh = pmap_physseg.pvoh;
	attr = pmap_physseg.attrs;
	for (bank = 0; bank < vm_nphysseg; bank++) {
		sz = vm_physmem[bank].end - vm_physmem[bank].start;
		vm_physmem[bank].pmseg.pvoh = pvoh;
		vm_physmem[bank].pmseg.attrs = attr;
		for (; sz > 0; sz--, pvoh++, attr++) {
			TAILQ_INIT(pvoh);
			*attr = 0;
		}
	}
#endif

	pool_init(&pmap_mpvo_pool, sizeof(struct pvo_entry),
	    sizeof(struct pvo_entry), 0, 0, "pmap_mpvopl",
	    &pmap_pool_mallocator, IPL_NONE);

	pool_setlowat(&pmap_mpvo_pool, 1008);

	pmap_initialized = 1;

}

/*
 * How much virtual space does the kernel get?
 */
void
pmap_virtual_space(vaddr_t *start, vaddr_t *end)
{
	/*
	 * For now, reserve one segment (minus some overhead) for kernel
	 * virtual memory
	 */
	*start = VM_MIN_KERNEL_ADDRESS;
	*end = VM_MAX_KERNEL_ADDRESS;
}

/*
 * Allocate, initialize, and return a new physical map.
 */
pmap_t
pmap_create(void)
{
	pmap_t pm;

	pm = pool_get(&pmap_pool, PR_WAITOK);
	memset((void *)pm, 0, sizeof *pm);
	pmap_pinit(pm);
	
	DPRINTFN(CREATE,("pmap_create: pm %p:\n"
	    "\t%06x %06x %06x %06x    %06x %06x %06x %06x\n"
	    "\t%06x %06x %06x %06x    %06x %06x %06x %06x\n", pm,
	    (unsigned int) pm->pm_sr[0], (unsigned int) pm->pm_sr[1],
	    (unsigned int) pm->pm_sr[2], (unsigned int) pm->pm_sr[3], 
	    (unsigned int) pm->pm_sr[4], (unsigned int) pm->pm_sr[5],
	    (unsigned int) pm->pm_sr[6], (unsigned int) pm->pm_sr[7],
	    (unsigned int) pm->pm_sr[8], (unsigned int) pm->pm_sr[9],
	    (unsigned int) pm->pm_sr[10], (unsigned int) pm->pm_sr[11], 
	    (unsigned int) pm->pm_sr[12], (unsigned int) pm->pm_sr[13],
	    (unsigned int) pm->pm_sr[14], (unsigned int) pm->pm_sr[15]));
	return pm;
}

/*
 * Initialize a preallocated and zeroed pmap structure.
 */
void
pmap_pinit(pmap_t pm)
{
	register_t entropy = MFTB();
	register_t mask;
	int i;

	/*
	 * Allocate some segment registers for this pmap.
	 */
	pm->pm_refs = 1;
	for (i = 0; i < NPMAPS; i += VSID_NBPW) {
		static register_t pmap_vsidcontext;
		register_t hash;
		unsigned int n;

		/* Create a new value by multiplying by a prime adding in
		 * entropy from the timebase register.  This is to make the
		 * VSID more random so that the PT Hash function collides
		 * less often. (note that the prime causes gcc to do shifts
		 * instead of a multiply)
		 */
		pmap_vsidcontext = (pmap_vsidcontext * 0x1105) + entropy;
		hash = pmap_vsidcontext & (NPMAPS - 1);
		if (hash == 0) {		/* 0 is special, avoid it */
			entropy += 0xbadf00d;
			continue;
		}
		n = hash >> 5;
		mask = 1L << (hash & (VSID_NBPW-1));
		hash = pmap_vsidcontext;
		if (pmap_vsid_bitmap[n] & mask) {	/* collision? */
			/* anything free in this bucket? */
			if (~pmap_vsid_bitmap[n] == 0) {
				entropy = hash ^ (hash >> 16);
				continue;
			}
			i = ffs(~pmap_vsid_bitmap[n]) - 1;
			mask = 1L << i;
			hash &= ~(VSID_NBPW-1);
			hash |= i;
		}
		hash &= PTE_VSID >> PTE_VSID_SHFT;
		pmap_vsid_bitmap[n] |= mask;
		pm->pm_vsid = hash;
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
		for (i = 0; i < 16; i++)
			pm->pm_sr[i] = VSID_MAKE(i, hash) | SR_PRKEY |
			    SR_NOEXEC;
#endif
		return;
	}
	panic("pmap_pinit: out of segments");
}

/*
 * Add a reference to the given pmap.
 */
void
pmap_reference(pmap_t pm)
{
	pm->pm_refs++;
}

/*
 * Retire the given pmap from service.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(pmap_t pm)
{
	if (--pm->pm_refs == 0) {
		pmap_release(pm);
		pool_put(&pmap_pool, pm);
	}
}

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 */
void
pmap_release(pmap_t pm)
{
	int idx, mask;

	KASSERT(pm->pm_stats.resident_count == 0);
	KASSERT(pm->pm_stats.wired_count == 0);
	
	if (pm->pm_sr[0] == 0)
		panic("pmap_release");
	idx = pm->pm_vsid & (NPMAPS-1);
	mask = 1 << (idx % VSID_NBPW);
	idx /= VSID_NBPW;

	KASSERT(pmap_vsid_bitmap[idx] & mask);
	pmap_vsid_bitmap[idx] &= ~mask;
}

/*
 * Copy the range specified by src_addr/len
 * from the source map to the range dst_addr/len
 * in the destination map.
 *
 * This routine is only advisory and need not do anything.
 */
void
pmap_copy(pmap_t dst_pmap, pmap_t src_pmap, vaddr_t dst_addr,
	vsize_t len, vaddr_t src_addr)
{
	PMAPCOUNT(copies);
}

/*
 * Require that all active physical maps contain no
 * incorrect entries NOW.
 */
void
pmap_update(struct pmap *pmap)
{
	PMAPCOUNT(updates);
	TLBSYNC();
}

/*
 * Garbage collects the physical map system for
 * pages which are no longer used.
 * Success need not be guaranteed -- that is, there
 * may well be pages which are not referenced, but
 * others may be collected.
 * Called by the pageout daemon when pages are scarce.
 */
void
pmap_collect(pmap_t pm)
{
	PMAPCOUNT(collects);
}

static inline int
pmap_pvo_pte_index(const struct pvo_entry *pvo, int ptegidx)
{
	int pteidx;
	/*
	 * We can find the actual pte entry without searching by
	 * grabbing the PTEG index from 3 unused bits in pte_lo[11:9]
	 * and by noticing the HID bit.
	 */
	pteidx = ptegidx * 8 + PVO_PTEGIDX_GET(pvo);
	if (pvo->pvo_pte.pte_hi & PTE_HID)
		pteidx ^= pmap_pteg_mask * 8;
	return pteidx;
}

volatile struct pte *
pmap_pvo_to_pte(const struct pvo_entry *pvo, int pteidx)
{
	volatile struct pte *pt;

#if !defined(DIAGNOSTIC) && !defined(DEBUG) && !defined(PMAPCHECK)
	if ((pvo->pvo_pte.pte_hi & PTE_VALID) == 0)
		return NULL;
#endif

	/*
	 * If we haven't been supplied the ptegidx, calculate it.
	 */
	if (pteidx == -1) {
		int ptegidx;
		ptegidx = va_to_pteg(pvo->pvo_pmap, pvo->pvo_vaddr);
		pteidx = pmap_pvo_pte_index(pvo, ptegidx);
	}

	pt = &pmap_pteg_table[pteidx >> 3].pt[pteidx & 7];

#if !defined(DIAGNOSTIC) && !defined(DEBUG) && !defined(PMAPCHECK)
	return pt;
#else
	if ((pvo->pvo_pte.pte_hi & PTE_VALID) && !PVO_PTEGIDX_ISSET(pvo)) {
		panic("pmap_pvo_to_pte: pvo %p: has valid pte in "
		    "pvo but no valid pte index", pvo);
	}
	if ((pvo->pvo_pte.pte_hi & PTE_VALID) == 0 && PVO_PTEGIDX_ISSET(pvo)) {
		panic("pmap_pvo_to_pte: pvo %p: has valid pte index in "
		    "pvo but no valid pte", pvo);
	}

	if ((pt->pte_hi ^ (pvo->pvo_pte.pte_hi & ~PTE_VALID)) == PTE_VALID) {
		if ((pvo->pvo_pte.pte_hi & PTE_VALID) == 0) {
#if defined(DEBUG) || defined(PMAPCHECK)
			pmap_pte_print(pt);
#endif
			panic("pmap_pvo_to_pte: pvo %p: has valid pte in "
			    "pmap_pteg_table %p but invalid in pvo",
			    pvo, pt);
		}
		if (((pt->pte_lo ^ pvo->pvo_pte.pte_lo) & ~(PTE_CHG|PTE_REF)) != 0) {
#if defined(DEBUG) || defined(PMAPCHECK)
			pmap_pte_print(pt);
#endif
			panic("pmap_pvo_to_pte: pvo %p: pvo pte does "
			    "not match pte %p in pmap_pteg_table",
			    pvo, pt);
		}
		return pt;
	}

	if (pvo->pvo_pte.pte_hi & PTE_VALID) {
#if defined(DEBUG) || defined(PMAPCHECK)
		pmap_pte_print(pt);
#endif
		panic("pmap_pvo_to_pte: pvo %p: has nomatching pte %p in "
		    "pmap_pteg_table but valid in pvo", pvo, pt);
	}
	return NULL;
#endif	/* !(!DIAGNOSTIC && !DEBUG && !PMAPCHECK) */
}

struct pvo_entry *
pmap_pvo_find_va(pmap_t pm, vaddr_t va, int *pteidx_p)
{
	struct pvo_entry *pvo;
	int ptegidx;

	va &= ~ADDR_POFF;
	ptegidx = va_to_pteg(pm, va);

	TAILQ_FOREACH(pvo, &pmap_pvo_table[ptegidx], pvo_olink) {
#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
		if ((uintptr_t) pvo >= SEGMENT_LENGTH)
			panic("pmap_pvo_find_va: invalid pvo %p on "
			    "list %#x (%p)", pvo, ptegidx,
			     &pmap_pvo_table[ptegidx]);
#endif
		if (pvo->pvo_pmap == pm && PVO_VADDR(pvo) == va) {
			if (pteidx_p)
				*pteidx_p = pmap_pvo_pte_index(pvo, ptegidx);
			return pvo;
		}
	}
	if ((pm == pmap_kernel()) && (va < SEGMENT_LENGTH))
		panic("%s: returning NULL for %s pmap, va: 0x%08lx\n", __FUNCTION__,
		(pm == pmap_kernel() ? "kernel" : "user"), va);
	return NULL;
}

#if defined(DEBUG) || defined(PMAPCHECK)
void
pmap_pvo_check(const struct pvo_entry *pvo)
{
	struct pvo_head *pvo_head;
	struct pvo_entry *pvo0;
	volatile struct pte *pt;
	int failed = 0;

	if ((uintptr_t)(pvo+1) >= SEGMENT_LENGTH)
		panic("pmap_pvo_check: pvo %p: invalid address", pvo);

	if ((uintptr_t)(pvo->pvo_pmap+1) >= SEGMENT_LENGTH) {
		printf("pmap_pvo_check: pvo %p: invalid pmap address %p\n",
		    pvo, pvo->pvo_pmap);
		failed = 1;
	}

	if ((uintptr_t)TAILQ_NEXT(pvo, pvo_olink) >= SEGMENT_LENGTH ||
	    (((uintptr_t)TAILQ_NEXT(pvo, pvo_olink)) & 0x1f) != 0) {
		printf("pmap_pvo_check: pvo %p: invalid ovlink address %p\n",
		    pvo, TAILQ_NEXT(pvo, pvo_olink));
		failed = 1;
	}

	if ((uintptr_t)LIST_NEXT(pvo, pvo_vlink) >= SEGMENT_LENGTH ||
	    (((uintptr_t)LIST_NEXT(pvo, pvo_vlink)) & 0x1f) != 0) {
		printf("pmap_pvo_check: pvo %p: invalid ovlink address %p\n",
		    pvo, LIST_NEXT(pvo, pvo_vlink));
		failed = 1;
	}

	if (PVO_MANAGED_P(pvo)) {
		pvo_head = pa_to_pvoh(pvo->pvo_pte.pte_lo & PTE_RPGN, NULL);
	} else {
		if (pvo->pvo_vaddr < VM_MIN_KERNEL_ADDRESS) {
			printf("pmap_pvo_check: pvo %p: non kernel address "
			    "on kernel unmanaged list\n", pvo);
			failed = 1;
		}
		pvo_head = &pmap_pvo_kunmanaged;
	}
	LIST_FOREACH(pvo0, pvo_head, pvo_vlink) {
		if (pvo0 == pvo)
			break;
	}
	if (pvo0 == NULL) {
		printf("pmap_pvo_check: pvo %p: not present "
		    "on its vlist head %p\n", pvo, pvo_head);
		failed = 1;
	}
	if (pvo != pmap_pvo_find_va(pvo->pvo_pmap, pvo->pvo_vaddr, NULL)) {
		printf("pmap_pvo_check: pvo %p: not present "
		    "on its olist head\n", pvo);
		failed = 1;
	}
	pt = pmap_pvo_to_pte(pvo, -1);
	if (pt == NULL) {
		if (pvo->pvo_pte.pte_hi & PTE_VALID) {
			printf("pmap_pvo_check: pvo %p: pte_hi VALID but "
			    "no PTE\n", pvo);
			failed = 1;
		}
	} else {
		if ((uintptr_t) pt < (uintptr_t) &pmap_pteg_table[0] ||
		    (uintptr_t) pt >=
		    (uintptr_t) &pmap_pteg_table[pmap_pteg_cnt]) {
			printf("pmap_pvo_check: pvo %p: pte %p not in "
			    "pteg table\n", pvo, pt);
			failed = 1;
		}
		if (((((uintptr_t) pt) >> 3) & 7) != PVO_PTEGIDX_GET(pvo)) {
			printf("pmap_pvo_check: pvo %p: pte_hi VALID but "
			    "no PTE\n", pvo);
			failed = 1;
		}
		if (pvo->pvo_pte.pte_hi != pt->pte_hi) {
			printf("pmap_pvo_check: pvo %p: pte_hi differ: "
			    "%#x/%#x\n", pvo, (unsigned int) pvo->pvo_pte.pte_hi, (unsigned int) pt->pte_hi);
			failed = 1;
		}
		if (((pvo->pvo_pte.pte_lo ^ pt->pte_lo) &
		    (PTE_PP|PTE_WIMG|PTE_RPGN)) != 0) {
			printf("pmap_pvo_check: pvo %p: pte_lo differ: "
			    "%#x/%#x\n", pvo,
			    (unsigned int) (pvo->pvo_pte.pte_lo & (PTE_PP|PTE_WIMG|PTE_RPGN)),
			    (unsigned int) (pt->pte_lo & (PTE_PP|PTE_WIMG|PTE_RPGN)));
			failed = 1;
		}
		if ((pmap_pte_to_va(pt) ^ PVO_VADDR(pvo)) & 0x0fffffff) {
			printf("pmap_pvo_check: pvo %p: PTE %p derived VA %#lx"
			    " doesn't not match PVO's VA %#lx\n",
			    pvo, pt, pmap_pte_to_va(pt), PVO_VADDR(pvo));
			failed = 1;
		}
		if (failed)
			pmap_pte_print(pt);
	}
	if (failed)
		panic("pmap_pvo_check: pvo %p, pm %p: bugcheck!", pvo,
		    pvo->pvo_pmap);
}
#endif /* DEBUG || PMAPCHECK */

/*
 * Search the PVO table looking for a non-wired entry.
 * If we find one, remove it and return it.
 */

struct pvo_entry *
pmap_pvo_reclaim(struct pmap *pm)
{
	struct pvo_tqhead *pvoh;
	struct pvo_entry *pvo;
	uint32_t idx, endidx;

	endidx = pmap_pvo_reclaim_nextidx;
	for (idx = (endidx + 1) & pmap_pteg_mask; idx != endidx;
	     idx = (idx + 1) & pmap_pteg_mask) {
		pvoh = &pmap_pvo_table[idx];
		TAILQ_FOREACH(pvo, pvoh, pvo_olink) {
			if (!PVO_WIRED_P(pvo)) {
				pmap_pvo_remove(pvo, -1, NULL);
				pmap_pvo_reclaim_nextidx = idx;
				PMAPCOUNT(pvos_reclaimed);
				return pvo;
			}
		}
	}
	return NULL;
}

/*
 * This returns whether this is the first mapping of a page.
 */
int
pmap_pvo_enter(pmap_t pm, struct pool *pl, struct pvo_head *pvo_head,
	vaddr_t va, paddr_t pa, register_t pte_lo, int flags)
{
	struct pvo_entry *pvo;
	struct pvo_tqhead *pvoh;
	register_t msr;
	int ptegidx;
	int i;
	int poolflags = PR_NOWAIT;

	/*
	 * Compute the PTE Group index.
	 */
	va &= ~ADDR_POFF;
	ptegidx = va_to_pteg(pm, va);

	msr = pmap_interrupts_off();

#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
	if (pmap_pvo_remove_depth > 0)
		panic("pmap_pvo_enter: called while pmap_pvo_remove active!");
	if (++pmap_pvo_enter_depth > 1)
		panic("pmap_pvo_enter: called recursively!");
#endif

	/*
	 * Remove any existing mapping for this page.  Reuse the
	 * pvo entry if there a mapping.
	 */
	TAILQ_FOREACH(pvo, &pmap_pvo_table[ptegidx], pvo_olink) {
		if (pvo->pvo_pmap == pm && PVO_VADDR(pvo) == va) {
#ifdef DEBUG
			if ((pmapdebug & PMAPDEBUG_PVOENTER) &&
			    ((pvo->pvo_pte.pte_lo ^ (pa|pte_lo)) &
			    ~(PTE_REF|PTE_CHG)) == 0 &&
			   va < VM_MIN_KERNEL_ADDRESS) {
				printf("pmap_pvo_enter: pvo %p: dup %#x/%#lx\n",
				    pvo, (unsigned int) pvo->pvo_pte.pte_lo, (unsigned int) pte_lo|pa);
				printf("pmap_pvo_enter: pte_hi=%#x sr=%#x\n",
				    (unsigned int) pvo->pvo_pte.pte_hi,
				    (unsigned int) pm->pm_sr[va >> ADDR_SR_SHFT]);
				pmap_pte_print(pmap_pvo_to_pte(pvo, -1));
#ifdef DDBX
				Debugger();
#endif
			}
#endif
			PMAPCOUNT(mappings_replaced);
			pmap_pvo_remove(pvo, -1, NULL);
			break;
		}
	}

	/*
	 * If we aren't overwriting an mapping, try to allocate
	 */
#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
	--pmap_pvo_enter_depth;
#endif
	pmap_interrupts_restore(msr);
	if (pvo) {
		pmap_pvo_free(pvo);
	}
	pvo = pool_get(pl, poolflags);

#ifdef DEBUG
	/*
	 * Exercise pmap_pvo_reclaim() a little.
	 */
	if (pvo && (flags & PMAP_CANFAIL) != 0 &&
	    pmap_pvo_reclaim_debugctr++ > 0x1000 &&
	    (pmap_pvo_reclaim_debugctr & 0xff) == 0) {
		pool_put(pl, pvo);
		pvo = NULL;
	}
#endif

	msr = pmap_interrupts_off();
#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
	++pmap_pvo_enter_depth;
#endif
	if (pvo == NULL) {
		pvo = pmap_pvo_reclaim(pm);
		if (pvo == NULL) {
			if ((flags & PMAP_CANFAIL) == 0)
				panic("pmap_pvo_enter: failed");
#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
			pmap_pvo_enter_depth--;
#endif
			PMAPCOUNT(pvos_failed);
			pmap_interrupts_restore(msr);
			return ENOMEM;
		}
	}

	pvo->pvo_vaddr = va;
	pvo->pvo_pmap = pm;
	pvo->pvo_vaddr &= ~ADDR_POFF;
	if (flags & VM_PROT_EXECUTE) {
		PMAPCOUNT(exec_mappings);
		pvo_set_exec(pvo);
	}
	if (flags & PMAP_WIRED)
		pvo->pvo_vaddr |= PVO_WIRED;
	if (pvo_head != &pmap_pvo_kunmanaged) {
		pvo->pvo_vaddr |= PVO_MANAGED; 
		PMAPCOUNT(mappings);
	} else {
		PMAPCOUNT(kernel_mappings);
	}
	pmap_pte_create(&pvo->pvo_pte, pm, va, pa | pte_lo);

	LIST_INSERT_HEAD(pvo_head, pvo, pvo_vlink);
	if (PVO_WIRED_P(pvo))
		pvo->pvo_pmap->pm_stats.wired_count++;
	pvo->pvo_pmap->pm_stats.resident_count++;
#if defined(DEBUG)
/*	if (pm != pmap_kernel() && va < VM_MIN_KERNEL_ADDRESS) */
		DPRINTFN(PVOENTER,
		    ("pmap_pvo_enter: pvo %p: pm %p va %#lx pa %#lx\n",
		    pvo, pm, va, pa));
#endif

	/*
	 * We hope this succeeds but it isn't required.
	 */
	pvoh = &pmap_pvo_table[ptegidx];
	i = pmap_pte_insert(ptegidx, &pvo->pvo_pte);
	if (i >= 0) {
		PVO_PTEGIDX_SET(pvo, i);
		PVO_WHERE(pvo, ENTER_INSERT);
		PMAPCOUNT2(((pvo->pvo_pte.pte_hi & PTE_HID)
		    ? pmap_evcnt_ptes_secondary : pmap_evcnt_ptes_primary)[i]);
		TAILQ_INSERT_TAIL(pvoh, pvo, pvo_olink);

	} else {
		/*
		 * Since we didn't have room for this entry (which makes it
		 * and evicted entry), place it at the head of the list.
		 */
		TAILQ_INSERT_HEAD(pvoh, pvo, pvo_olink);
		PMAPCOUNT(ptes_evicted);
		pm->pm_evictions++;
		/*
		 * If this is a kernel page, make sure it's active.
		 */
		if (pm == pmap_kernel()) {
			i = pmap_pte_spill(pm, va, false);
			KASSERT(i);
		}
	}
	PMAP_PVO_CHECK(pvo);		/* sanity check */
#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
	pmap_pvo_enter_depth--;
#endif
	pmap_interrupts_restore(msr);
	return 0;
}

void
pmap_pvo_remove(struct pvo_entry *pvo, int pteidx, struct pvo_head *pvol)
{
	volatile struct pte *pt;
	int ptegidx;

#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
	if (++pmap_pvo_remove_depth > 1)
		panic("pmap_pvo_remove: called recursively!");
#endif

	/*
	 * If we haven't been supplied the ptegidx, calculate it.
	 */
	if (pteidx == -1) {
		ptegidx = va_to_pteg(pvo->pvo_pmap, pvo->pvo_vaddr);
		pteidx = pmap_pvo_pte_index(pvo, ptegidx);
	} else {
		ptegidx = pteidx >> 3;
		if (pvo->pvo_pte.pte_hi & PTE_HID)
			ptegidx ^= pmap_pteg_mask;
	}
	PMAP_PVO_CHECK(pvo);		/* sanity check */

	/* 
	 * If there is an active pte entry, we need to deactivate it
	 * (and save the ref & chg bits).
	 */
	pt = pmap_pvo_to_pte(pvo, pteidx);
	if (pt != NULL) {
		pmap_pte_unset(pt, &pvo->pvo_pte, pvo->pvo_vaddr);
		PVO_WHERE(pvo, REMOVE);
		PVO_PTEGIDX_CLR(pvo);
		PMAPCOUNT(ptes_removed);
	} else {
		KASSERT(pvo->pvo_pmap->pm_evictions > 0);
		pvo->pvo_pmap->pm_evictions--;
	}

	/*
	 * Account for executable mappings.
	 */
	if (PVO_EXECUTABLE_P(pvo))
		pvo_clear_exec(pvo);

	/*
	 * Update our statistics.
	 */
	pvo->pvo_pmap->pm_stats.resident_count--;
	if (PVO_WIRED_P(pvo))
		pvo->pvo_pmap->pm_stats.wired_count--;

	/*
	 * Save the REF/CHG bits into their cache if the page is managed.
	 */
	if (PVO_MANAGED_P(pvo)) {
		register_t ptelo = pvo->pvo_pte.pte_lo;
		struct vm_page *pg = PHYS_TO_VM_PAGE(ptelo & PTE_RPGN);

		if (pg != NULL) {
			/*
			 * If this page was changed and it is mapped exec,
			 * invalidate it.
			 */
			if ((ptelo & PTE_CHG) &&
			    (pmap_attr_fetch(pg) & PTE_EXEC)) {
				struct pvo_head *pvoh = vm_page_to_pvoh(pg);
				if (LIST_EMPTY(pvoh)) {
					DPRINTFN(EXEC, ("[pmap_pvo_remove: "
					    "%#lx: clear-exec]\n",
					    VM_PAGE_TO_PHYS(pg)));
					pmap_attr_clear(pg, PTE_EXEC);
					PMAPCOUNT(exec_uncached_pvo_remove);
				} else {
					DPRINTFN(EXEC, ("[pmap_pvo_remove: "
					    "%#lx: syncicache]\n",
					    VM_PAGE_TO_PHYS(pg)));
					pmap_syncicache(VM_PAGE_TO_PHYS(pg),
					    PAGE_SIZE);
					PMAPCOUNT(exec_synced_pvo_remove);
				}
			}

			pmap_attr_save(pg, ptelo & (PTE_REF|PTE_CHG));
		}
		PMAPCOUNT(unmappings);
	} else {
		PMAPCOUNT(kernel_unmappings);
	}

	/*
	 * Remove the PVO from its lists and return it to the pool.
	 */
	LIST_REMOVE(pvo, pvo_vlink);
	TAILQ_REMOVE(&pmap_pvo_table[ptegidx], pvo, pvo_olink);
	if (pvol) {
		LIST_INSERT_HEAD(pvol, pvo, pvo_vlink);
	}
#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
	pmap_pvo_remove_depth--;
#endif
}

void
pmap_pvo_free(struct pvo_entry *pvo)
{

	pool_put(PVO_MANAGED_P(pvo) ? &pmap_mpvo_pool : &pmap_upvo_pool, pvo);
}

void
pmap_pvo_free_list(struct pvo_head *pvol)
{
	struct pvo_entry *pvo, *npvo;

	for (pvo = LIST_FIRST(pvol); pvo != NULL; pvo = npvo) {
		npvo = LIST_NEXT(pvo, pvo_vlink);
		LIST_REMOVE(pvo, pvo_vlink);
		pmap_pvo_free(pvo);
	}
}

/*
 * Mark a mapping as executable.
 * If this is the first executable mapping in the segment,
 * clear the noexec flag.
 */
STATIC void
pvo_set_exec(struct pvo_entry *pvo)
{
	struct pmap *pm = pvo->pvo_pmap;

	if (pm == pmap_kernel() || PVO_EXECUTABLE_P(pvo)) {
		return;
	}
	pvo->pvo_vaddr |= PVO_EXECUTABLE;
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	{
		int sr = PVO_VADDR(pvo) >> ADDR_SR_SHFT;
		if (pm->pm_exec[sr]++ == 0) {
			pm->pm_sr[sr] &= ~SR_NOEXEC;
		}
	}
#endif
}

/*
 * Mark a mapping as non-executable.
 * If this was the last executable mapping in the segment,
 * set the noexec flag.
 */
STATIC void
pvo_clear_exec(struct pvo_entry *pvo)
{
	struct pmap *pm = pvo->pvo_pmap;

	if (pm == pmap_kernel() || !PVO_EXECUTABLE_P(pvo)) {
		return;
	}
	pvo->pvo_vaddr &= ~PVO_EXECUTABLE;
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	{
		int sr = PVO_VADDR(pvo) >> ADDR_SR_SHFT;
		if (--pm->pm_exec[sr] == 0) {
			pm->pm_sr[sr] |= SR_NOEXEC;
		}
	}
#endif
}

/*
 * Insert physical page at pa into the given pmap at virtual address va.
 */
int
pmap_enter(pmap_t pm, vaddr_t va, paddr_t pa, vm_prot_t prot, int flags)
{
	struct mem_region *mp;
	struct pvo_head *pvo_head;
	struct vm_page *pg;
	struct pool *pl;
	register_t pte_lo;
	int error;
	u_int pvo_flags;
	u_int was_exec = 0;

	if (__predict_false(!pmap_initialized)) {
		pvo_head = &pmap_pvo_kunmanaged;
		pl = &pmap_upvo_pool;
		pvo_flags = 0;
		pg = NULL;
		was_exec = PTE_EXEC;
	} else {
		pvo_head = pa_to_pvoh(pa, &pg);
		pl = &pmap_mpvo_pool;
		pvo_flags = PVO_MANAGED;
	}

	DPRINTFN(ENTER,
	    ("pmap_enter(%p, 0x%lx, 0x%lx, 0x%x, 0x%x):",
	    pm, va, pa, prot, flags));

	/*
	 * If this is a managed page, and it's the first reference to the
	 * page clear the execness of the page.  Otherwise fetch the execness.
	 */
	if (pg != NULL)
		was_exec = pmap_attr_fetch(pg) & PTE_EXEC;

	DPRINTFN(ENTER, (" was_exec=%d", was_exec));

	/*
	 * Assume the page is cache inhibited and access is guarded unless
	 * it's in our available memory array.  If it is in the memory array,
	 * asssume it's in memory coherent memory.
	 */
	pte_lo = PTE_IG;
	if ((flags & PMAP_NC) == 0) {
		for (mp = mem; mp->size; mp++) {
			if (pa >= mp->start && pa < mp->start + mp->size) {
				pte_lo = PTE_M;
				break;
			}
		}
	}

	if (prot & VM_PROT_WRITE)
		pte_lo |= PTE_BW;
	else
		pte_lo |= PTE_BR;

	/*
	 * If this was in response to a fault, "pre-fault" the PTE's
	 * changed/referenced bit appropriately.
	 */
	if (flags & VM_PROT_WRITE)
		pte_lo |= PTE_CHG;
	if (flags & VM_PROT_ALL)
		pte_lo |= PTE_REF;

	/*
	 * We need to know if this page can be executable
	 */
	flags |= (prot & VM_PROT_EXECUTE);

	/*
	 * Record mapping for later back-translation and pte spilling.
	 * This will overwrite any existing mapping.
	 */
	error = pmap_pvo_enter(pm, pl, pvo_head, va, pa, pte_lo, flags);

	/* 
	 * Flush the real page from the instruction cache if this page is
	 * mapped executable and cacheable and has not been flushed since
	 * the last time it was modified.
	 */
	if (error == 0 &&
            (flags & VM_PROT_EXECUTE) &&
            (pte_lo & PTE_I) == 0 &&
	    was_exec == 0) {
		DPRINTFN(ENTER, (" syncicache"));
		PMAPCOUNT(exec_synced);
		pmap_syncicache(pa, PAGE_SIZE);
		if (pg != NULL) {
			pmap_attr_save(pg, PTE_EXEC);
			PMAPCOUNT(exec_cached);
#if defined(DEBUG) || defined(PMAPDEBUG)
			if (pmapdebug & PMAPDEBUG_ENTER)
				printf(" marked-as-exec");
			else if (pmapdebug & PMAPDEBUG_EXEC)
				printf("[pmap_enter: %#lx: marked-as-exec]\n",
				    VM_PAGE_TO_PHYS(pg));
				
#endif
		}
	}

	DPRINTFN(ENTER, (": error=%d\n", error));

	return error;
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	struct mem_region *mp;
	register_t pte_lo;
	int error;

#if defined (PPC_OEA64_BRIDGE)
	if (va < VM_MIN_KERNEL_ADDRESS)
		panic("pmap_kenter_pa: attempt to enter "
		    "non-kernel address %#lx!", va);
#endif

	DPRINTFN(KENTER,
	    ("pmap_kenter_pa(%#lx,%#lx,%#x)\n", va, pa, prot));

	/*
	 * Assume the page is cache inhibited and access is guarded unless
	 * it's in our available memory array.  If it is in the memory array,
	 * asssume it's in memory coherent memory.
	 */
	pte_lo = PTE_IG;
	if ((prot & PMAP_NC) == 0) {
		for (mp = mem; mp->size; mp++) {
			if (pa >= mp->start && pa < mp->start + mp->size) {
				pte_lo = PTE_M;
				break;
			}
		}
	}

	if (prot & VM_PROT_WRITE)
		pte_lo |= PTE_BW;
	else
		pte_lo |= PTE_BR;

	/*
	 * We don't care about REF/CHG on PVOs on the unmanaged list.
	 */
	error = pmap_pvo_enter(pmap_kernel(), &pmap_upvo_pool,
	    &pmap_pvo_kunmanaged, va, pa, pte_lo, prot|PMAP_WIRED);

	if (error != 0)
		panic("pmap_kenter_pa: failed to enter va %#lx pa %#lx: %d",
		      va, pa, error);
}

void
pmap_kremove(vaddr_t va, vsize_t len)
{
	if (va < VM_MIN_KERNEL_ADDRESS)
		panic("pmap_kremove: attempt to remove "
		    "non-kernel address %#lx!", va);

	DPRINTFN(KREMOVE,("pmap_kremove(%#lx,%#lx)\n", va, len));
	pmap_remove(pmap_kernel(), va, va + len);
}

/*
 * Remove the given range of mapping entries.
 */
void
pmap_remove(pmap_t pm, vaddr_t va, vaddr_t endva)
{
	struct pvo_head pvol;
	struct pvo_entry *pvo;
	register_t msr;
	int pteidx;

	LIST_INIT(&pvol);
	msr = pmap_interrupts_off();
	for (; va < endva; va += PAGE_SIZE) {
		pvo = pmap_pvo_find_va(pm, va, &pteidx);
		if (pvo != NULL) {
			pmap_pvo_remove(pvo, pteidx, &pvol);
		}
	}
	pmap_interrupts_restore(msr);
	pmap_pvo_free_list(&pvol);
}

/*
 * Get the physical page address for the given pmap/virtual address.
 */
bool
pmap_extract(pmap_t pm, vaddr_t va, paddr_t *pap)
{
	struct pvo_entry *pvo;
	register_t msr;


	/*
	 * If this is a kernel pmap lookup, also check the battable
	 * and if we get a hit, translate the VA to a PA using the
	 * BAT entries.  Don't check for VM_MAX_KERNEL_ADDRESS is
	 * that will wrap back to 0.
	 */
	if (pm == pmap_kernel() &&
	    (va < VM_MIN_KERNEL_ADDRESS ||
	     (KERNEL2_SR < 15 && VM_MAX_KERNEL_ADDRESS <= va))) {
		KASSERT((va >> ADDR_SR_SHFT) != USER_SR);
#if defined (PPC_OEA)
		if ((MFPVR() >> 16) != MPC601) {
			register_t batu = battable[va >> ADDR_SR_SHFT].batu;
			if (BAT_VALID_P(batu,0) && BAT_VA_MATCH_P(batu,va)) {
				register_t batl =
				    battable[va >> ADDR_SR_SHFT].batl;
				register_t mask =
				    (~(batu & BAT_BL) << 15) & ~0x1ffffL;
				if (pap)
					*pap = (batl & mask) | (va & ~mask);
				return true;
			}
		} else {
			register_t batu = battable[va >> 23].batu;
			register_t batl = battable[va >> 23].batl;
			register_t sr = iosrtable[va >> ADDR_SR_SHFT];
			if (BAT601_VALID_P(batl) &&
			    BAT601_VA_MATCH_P(batu, batl, va)) {
				register_t mask =
				    (~(batl & BAT601_BSM) << 17) & ~0x1ffffL;
				if (pap)
					*pap = (batl & mask) | (va & ~mask);
				return true;
			} else if (SR601_VALID_P(sr) &&
				   SR601_PA_MATCH_P(sr, va)) {
				if (pap)
					*pap = va;
				return true;
			}
		}
		return false;
#elif defined (PPC_OEA64_BRIDGE)
	panic("%s: pm: %s, va: 0x%08lx\n", __FUNCTION__, 
		(pm == pmap_kernel() ? "kernel" : "user"), va);
#elif defined (PPC_OEA64)
#error PPC_OEA64 not supported
#endif /* PPC_OEA */
	}
	
	msr = pmap_interrupts_off();
	pvo = pmap_pvo_find_va(pm, va & ~ADDR_POFF, NULL);
	if (pvo != NULL) {
		PMAP_PVO_CHECK(pvo);		/* sanity check */
		if (pap)
			*pap = (pvo->pvo_pte.pte_lo & PTE_RPGN)
			    | (va & ADDR_POFF);
	}
	pmap_interrupts_restore(msr);
	return pvo != NULL;
}

/*
 * Lower the protection on the specified range of this pmap.
 */
void
pmap_protect(pmap_t pm, vaddr_t va, vaddr_t endva, vm_prot_t prot)
{
	struct pvo_entry *pvo;
	volatile struct pte *pt;
	register_t msr;
	int pteidx;

	/*
	 * Since this routine only downgrades protection, we should
	 * always be called with at least one bit not set.
	 */
	KASSERT(prot != VM_PROT_ALL);

	/*
	 * If there is no protection, this is equivalent to
	 * remove the pmap from the pmap.
	 */
	if ((prot & VM_PROT_READ) == 0) {
		pmap_remove(pm, va, endva);
		return;
	}

	msr = pmap_interrupts_off();
	for (; va < endva; va += PAGE_SIZE) {
		pvo = pmap_pvo_find_va(pm, va, &pteidx);
		if (pvo == NULL)
			continue;
		PMAP_PVO_CHECK(pvo);		/* sanity check */

		/*
		 * Revoke executable if asked to do so.
		 */
		if ((prot & VM_PROT_EXECUTE) == 0)
			pvo_clear_exec(pvo);

#if 0
		/*
		 * If the page is already read-only, no change
		 * needs to be made.
		 */
		if ((pvo->pvo_pte.pte_lo & PTE_PP) == PTE_BR)
			continue;
#endif
		/*
		 * Grab the PTE pointer before we diddle with
		 * the cached PTE copy.
		 */
		pt = pmap_pvo_to_pte(pvo, pteidx);
		/*
		 * Change the protection of the page.
		 */
		pvo->pvo_pte.pte_lo &= ~PTE_PP;
		pvo->pvo_pte.pte_lo |= PTE_BR;

		/*
		 * If the PVO is in the page table, update
		 * that pte at well.
		 */
		if (pt != NULL) {
			pmap_pte_change(pt, &pvo->pvo_pte, pvo->pvo_vaddr);
			PVO_WHERE(pvo, PMAP_PROTECT);
			PMAPCOUNT(ptes_changed);
		}

		PMAP_PVO_CHECK(pvo);		/* sanity check */
	}
	pmap_interrupts_restore(msr);
}

void
pmap_unwire(pmap_t pm, vaddr_t va)
{
	struct pvo_entry *pvo;
	register_t msr;

	msr = pmap_interrupts_off();
	pvo = pmap_pvo_find_va(pm, va, NULL);
	if (pvo != NULL) {
		if (PVO_WIRED_P(pvo)) {
			pvo->pvo_vaddr &= ~PVO_WIRED;
			pm->pm_stats.wired_count--;
		}
		PMAP_PVO_CHECK(pvo);		/* sanity check */
	}
	pmap_interrupts_restore(msr);
}

/*
 * Lower the protection on the specified physical page.
 */
void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	struct pvo_head *pvo_head, pvol;
	struct pvo_entry *pvo, *next_pvo;
	volatile struct pte *pt;
	register_t msr;

	KASSERT(prot != VM_PROT_ALL);
	LIST_INIT(&pvol);
	msr = pmap_interrupts_off();

	/*
	 * When UVM reuses a page, it does a pmap_page_protect with
	 * VM_PROT_NONE.  At that point, we can clear the exec flag
	 * since we know the page will have different contents.
	 */
	if ((prot & VM_PROT_READ) == 0) {
		DPRINTFN(EXEC, ("[pmap_page_protect: %#lx: clear-exec]\n",
		    VM_PAGE_TO_PHYS(pg)));
		if (pmap_attr_fetch(pg) & PTE_EXEC) {
			PMAPCOUNT(exec_uncached_page_protect);
			pmap_attr_clear(pg, PTE_EXEC);
		}
	}

	pvo_head = vm_page_to_pvoh(pg);
	for (pvo = LIST_FIRST(pvo_head); pvo != NULL; pvo = next_pvo) {
		next_pvo = LIST_NEXT(pvo, pvo_vlink);
		PMAP_PVO_CHECK(pvo);		/* sanity check */

		/*
		 * Downgrading to no mapping at all, we just remove the entry.
		 */
		if ((prot & VM_PROT_READ) == 0) {
			pmap_pvo_remove(pvo, -1, &pvol);
			continue;
		} 

		/*
		 * If EXEC permission is being revoked, just clear the
		 * flag in the PVO.
		 */
		if ((prot & VM_PROT_EXECUTE) == 0)
			pvo_clear_exec(pvo);

		/*
		 * If this entry is already RO, don't diddle with the
		 * page table.
		 */
		if ((pvo->pvo_pte.pte_lo & PTE_PP) == PTE_BR) {
			PMAP_PVO_CHECK(pvo);
			continue;
		}

		/*
		 * Grab the PTE before the we diddle the bits so
		 * pvo_to_pte can verify the pte contents are as
		 * expected.
		 */
		pt = pmap_pvo_to_pte(pvo, -1);
		pvo->pvo_pte.pte_lo &= ~PTE_PP;
		pvo->pvo_pte.pte_lo |= PTE_BR;
		if (pt != NULL) {
			pmap_pte_change(pt, &pvo->pvo_pte, pvo->pvo_vaddr);
			PVO_WHERE(pvo, PMAP_PAGE_PROTECT);
			PMAPCOUNT(ptes_changed);
		}
		PMAP_PVO_CHECK(pvo);		/* sanity check */
	}
	pmap_interrupts_restore(msr);
	pmap_pvo_free_list(&pvol);
}

/*
 * Activate the address space for the specified process.  If the process
 * is the current process, load the new MMU context.
 */
void
pmap_activate(struct lwp *l)
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;

	DPRINTFN(ACTIVATE,
	    ("pmap_activate: lwp %p (curlwp %p)\n", l, curlwp));

	/*
	 * XXX Normally performed in cpu_fork().
	 */
	pcb->pcb_pm = pmap;

	/*
	* In theory, the SR registers need only be valid on return
	* to user space wait to do them there.
	*/
	if (l == curlwp) {
		/* Store pointer to new current pmap. */
		curpm = pmap;
	}
}

/*
 * Deactivate the specified process's address space.
 */
void
pmap_deactivate(struct lwp *l)
{
}

bool
pmap_query_bit(struct vm_page *pg, int ptebit)
{
	struct pvo_entry *pvo;
	volatile struct pte *pt;
	register_t msr;

	if (pmap_attr_fetch(pg) & ptebit)
		return true;

	msr = pmap_interrupts_off();
	LIST_FOREACH(pvo, vm_page_to_pvoh(pg), pvo_vlink) {
		PMAP_PVO_CHECK(pvo);		/* sanity check */
		/*
		 * See if we saved the bit off.  If so cache, it and return
		 * success.
		 */
		if (pvo->pvo_pte.pte_lo & ptebit) {
			pmap_attr_save(pg, ptebit);
			PMAP_PVO_CHECK(pvo);		/* sanity check */
			pmap_interrupts_restore(msr);
			return true;
		}
	}
	/*
	 * No luck, now go thru the hard part of looking at the ptes
	 * themselves.  Sync so any pending REF/CHG bits are flushed
	 * to the PTEs.
	 */
	SYNC();
	LIST_FOREACH(pvo, vm_page_to_pvoh(pg), pvo_vlink) {
		PMAP_PVO_CHECK(pvo);		/* sanity check */
		/*
		 * See if this pvo have a valid PTE.  If so, fetch the
		 * REF/CHG bits from the valid PTE.  If the appropriate
		 * ptebit is set, cache, it and return success.
		 */
		pt = pmap_pvo_to_pte(pvo, -1);
		if (pt != NULL) {
			pmap_pte_synch(pt, &pvo->pvo_pte);
			if (pvo->pvo_pte.pte_lo & ptebit) {
				pmap_attr_save(pg, ptebit);
				PMAP_PVO_CHECK(pvo);		/* sanity check */
				pmap_interrupts_restore(msr);
				return true;
			}
		}
	}
	pmap_interrupts_restore(msr);
	return false;
}

bool
pmap_clear_bit(struct vm_page *pg, int ptebit)
{
	struct pvo_head *pvoh = vm_page_to_pvoh(pg);
	struct pvo_entry *pvo;
	volatile struct pte *pt;
	register_t msr;
	int rv = 0;

	msr = pmap_interrupts_off();

	/*
	 * Fetch the cache value
	 */
	rv |= pmap_attr_fetch(pg);

	/*
	 * Clear the cached value.
	 */
	pmap_attr_clear(pg, ptebit);

	/*
	 * Sync so any pending REF/CHG bits are flushed to the PTEs (so we
	 * can reset the right ones).  Note that since the pvo entries and
	 * list heads are accessed via BAT0 and are never placed in the 
	 * page table, we don't have to worry about further accesses setting
	 * the REF/CHG bits.
	 */
	SYNC();

	/*
	 * For each pvo entry, clear pvo's ptebit.  If this pvo have a
	 * valid PTE.  If so, clear the ptebit from the valid PTE.
	 */
	LIST_FOREACH(pvo, pvoh, pvo_vlink) {
		PMAP_PVO_CHECK(pvo);		/* sanity check */
		pt = pmap_pvo_to_pte(pvo, -1);
		if (pt != NULL) {
			/*
			 * Only sync the PTE if the bit we are looking
			 * for is not already set.
			 */
			if ((pvo->pvo_pte.pte_lo & ptebit) == 0)
				pmap_pte_synch(pt, &pvo->pvo_pte);
			/*
			 * If the bit we are looking for was already set,
			 * clear that bit in the pte.
			 */
			if (pvo->pvo_pte.pte_lo & ptebit)
				pmap_pte_clear(pt, PVO_VADDR(pvo), ptebit);
		}
		rv |= pvo->pvo_pte.pte_lo & (PTE_CHG|PTE_REF);
		pvo->pvo_pte.pte_lo &= ~ptebit;
		PMAP_PVO_CHECK(pvo);		/* sanity check */
	}
	pmap_interrupts_restore(msr);

	/*
	 * If we are clearing the modify bit and this page was marked EXEC
	 * and the user of the page thinks the page was modified, then we
	 * need to clean it from the icache if it's mapped or clear the EXEC
	 * bit if it's not mapped.  The page itself might not have the CHG
	 * bit set if the modification was done via DMA to the page.
	 */
	if ((ptebit & PTE_CHG) && (rv & PTE_EXEC)) {
		if (LIST_EMPTY(pvoh)) {
			DPRINTFN(EXEC, ("[pmap_clear_bit: %#lx: clear-exec]\n",
			    VM_PAGE_TO_PHYS(pg)));
			pmap_attr_clear(pg, PTE_EXEC);
			PMAPCOUNT(exec_uncached_clear_modify);
		} else {
			DPRINTFN(EXEC, ("[pmap_clear_bit: %#lx: syncicache]\n",
			    VM_PAGE_TO_PHYS(pg)));
			pmap_syncicache(VM_PAGE_TO_PHYS(pg), PAGE_SIZE);
			PMAPCOUNT(exec_synced_clear_modify);
		}
	}
	return (rv & ptebit) != 0;
}

void
pmap_procwr(struct proc *p, vaddr_t va, size_t len)
{
	struct pvo_entry *pvo;
	size_t offset = va & ADDR_POFF;
	int s;

	s = splvm();
	while (len > 0) {
		size_t seglen = PAGE_SIZE - offset;
		if (seglen > len)
			seglen = len;
		pvo = pmap_pvo_find_va(p->p_vmspace->vm_map.pmap, va, NULL);
		if (pvo != NULL && PVO_EXECUTABLE_P(pvo)) {
			pmap_syncicache(
			    (pvo->pvo_pte.pte_lo & PTE_RPGN) | offset, seglen);
			PMAP_PVO_CHECK(pvo);
		}
		va += seglen;
		len -= seglen;
		offset = 0;
	}
	splx(s);
}

#if defined(DEBUG) || defined(PMAPCHECK) || defined(DDB)
void
pmap_pte_print(volatile struct pte *pt)
{
	printf("PTE %p: ", pt);

#if defined(PPC_OEA)
	/* High word: */
	printf("0x%08lx: [", pt->pte_hi);
#elif defined (PPC_OEA64_BRIDGE)
	printf("0x%016llx: [", pt->pte_hi);
#else /* PPC_OEA64 */
	printf("0x%016lx: [", pt->pte_hi);
#endif /* PPC_OEA */

	printf("%c ", (pt->pte_hi & PTE_VALID) ? 'v' : 'i');
	printf("%c ", (pt->pte_hi & PTE_HID) ? 'h' : '-');

#if defined (PPC_OEA)
	printf("0x%06lx 0x%02lx",
	    (pt->pte_hi &~ PTE_VALID)>>PTE_VSID_SHFT,
	    pt->pte_hi & PTE_API);
	printf(" (va 0x%08lx)] ", pmap_pte_to_va(pt));
#elif defined (PPC_OEA64)
	printf("0x%06lx 0x%02lx",
	    (pt->pte_hi &~ PTE_VALID)>>PTE_VSID_SHFT,
	    pt->pte_hi & PTE_API);
	printf(" (va 0x%016lx)] ", pmap_pte_to_va(pt));
#else
	/* PPC_OEA64_BRIDGE */
	printf("0x%06llx 0x%02llx",
	    (pt->pte_hi &~ PTE_VALID)>>PTE_VSID_SHFT,
	    pt->pte_hi & PTE_API);
	printf(" (va 0x%08lx)] ", pmap_pte_to_va(pt));
#endif /* PPC_OEA */

	/* Low word: */
#if defined (PPC_OEA)
	printf(" 0x%08lx: [", pt->pte_lo);
	printf("0x%05lx... ", pt->pte_lo >> 12);
#elif defined (PPC_OEA64)
	printf(" 0x%016lx: [", pt->pte_lo);
	printf("0x%012lx... ", pt->pte_lo >> 12);
#else	/* PPC_OEA64_BRIDGE */
	printf(" 0x%016llx: [", pt->pte_lo);
	printf("0x%012llx... ", pt->pte_lo >> 12);
#endif
	printf("%c ", (pt->pte_lo & PTE_REF) ? 'r' : 'u');
	printf("%c ", (pt->pte_lo & PTE_CHG) ? 'c' : 'n');
	printf("%c", (pt->pte_lo & PTE_W) ? 'w' : '.');
	printf("%c", (pt->pte_lo & PTE_I) ? 'i' : '.');
	printf("%c", (pt->pte_lo & PTE_M) ? 'm' : '.');
	printf("%c ", (pt->pte_lo & PTE_G) ? 'g' : '.');
	switch (pt->pte_lo & PTE_PP) {
	case PTE_BR: printf("br]\n"); break;
	case PTE_BW: printf("bw]\n"); break;
	case PTE_SO: printf("so]\n"); break;
	case PTE_SW: printf("sw]\n"); break;
	}
}
#endif

#if defined(DDB)
void
pmap_pteg_check(void)
{
	volatile struct pte *pt;
	int i;
	int ptegidx;
	u_int p_valid = 0;
	u_int s_valid = 0;
	u_int invalid = 0;

	for (ptegidx = 0; ptegidx < pmap_pteg_cnt; ptegidx++) {
		for (pt = pmap_pteg_table[ptegidx].pt, i = 8; --i >= 0; pt++) {
			if (pt->pte_hi & PTE_VALID) {
				if (pt->pte_hi & PTE_HID)
					s_valid++;
				else
				{
					p_valid++;
				}
			} else
				invalid++;
		}
	}
	printf("pteg_check: v(p) %#x (%d), v(s) %#x (%d), i %#x (%d)\n",
		p_valid, p_valid, s_valid, s_valid,
		invalid, invalid);
}

void
pmap_print_mmuregs(void)
{
	int i;
	u_int cpuvers;
#ifndef PPC_OEA64
	vaddr_t addr;
	register_t soft_sr[16];
#endif
#if defined (PPC_OEA) && !defined (PPC_OEA64) && !defined (PPC_OEA64_BRIDGE)
	struct bat soft_ibat[4];
	struct bat soft_dbat[4];
#endif
	register_t sdr1;
	
	cpuvers = MFPVR() >> 16;
	__asm volatile ("mfsdr1 %0" : "=r"(sdr1));
#ifndef PPC_OEA64
	addr = 0;
	for (i = 0; i < 16; i++) {
		soft_sr[i] = MFSRIN(addr);
		addr += (1 << ADDR_SR_SHFT);
	}
#endif

#if defined(PPC_OEA) && !defined (PPC_OEA64) && !defined (PPC_OEA64_BRIDGE)
	/* read iBAT (601: uBAT) registers */
	__asm volatile ("mfibatu %0,0" : "=r"(soft_ibat[0].batu));
	__asm volatile ("mfibatl %0,0" : "=r"(soft_ibat[0].batl));
	__asm volatile ("mfibatu %0,1" : "=r"(soft_ibat[1].batu));
	__asm volatile ("mfibatl %0,1" : "=r"(soft_ibat[1].batl));
	__asm volatile ("mfibatu %0,2" : "=r"(soft_ibat[2].batu));
	__asm volatile ("mfibatl %0,2" : "=r"(soft_ibat[2].batl));
	__asm volatile ("mfibatu %0,3" : "=r"(soft_ibat[3].batu));
	__asm volatile ("mfibatl %0,3" : "=r"(soft_ibat[3].batl));


	if (cpuvers != MPC601) {
		/* read dBAT registers */
		__asm volatile ("mfdbatu %0,0" : "=r"(soft_dbat[0].batu));
		__asm volatile ("mfdbatl %0,0" : "=r"(soft_dbat[0].batl));
		__asm volatile ("mfdbatu %0,1" : "=r"(soft_dbat[1].batu));
		__asm volatile ("mfdbatl %0,1" : "=r"(soft_dbat[1].batl));
		__asm volatile ("mfdbatu %0,2" : "=r"(soft_dbat[2].batu));
		__asm volatile ("mfdbatl %0,2" : "=r"(soft_dbat[2].batl));
		__asm volatile ("mfdbatu %0,3" : "=r"(soft_dbat[3].batu));
		__asm volatile ("mfdbatl %0,3" : "=r"(soft_dbat[3].batl));
	}
#endif

	printf("SDR1:\t0x%lx\n", (long) sdr1);
#ifndef PPC_OEA64
	printf("SR[]:\t");
	for (i = 0; i < 4; i++)
		printf("0x%08lx,   ", (long) soft_sr[i]);
	printf("\n\t");
	for ( ; i < 8; i++)
		printf("0x%08lx,   ", (long) soft_sr[i]);
	printf("\n\t");
	for ( ; i < 12; i++)
		printf("0x%08lx,   ", (long) soft_sr[i]);
	printf("\n\t");
	for ( ; i < 16; i++)
		printf("0x%08lx,   ", (long) soft_sr[i]);
	printf("\n");
#endif

#if defined(PPC_OEA) && !defined (PPC_OEA64) && !defined (PPC_OEA64_BRIDGE)
	printf("%cBAT[]:\t", cpuvers == MPC601 ? 'u' : 'i');
	for (i = 0; i < 4; i++) {
		printf("0x%08lx 0x%08lx, ",
			soft_ibat[i].batu, soft_ibat[i].batl);
		if (i == 1)
			printf("\n\t");
	}
	if (cpuvers != MPC601) {
		printf("\ndBAT[]:\t");
		for (i = 0; i < 4; i++) {
			printf("0x%08lx 0x%08lx, ",
				soft_dbat[i].batu, soft_dbat[i].batl);
			if (i == 1)
				printf("\n\t");
		}
	}
	printf("\n");
#endif /* PPC_OEA... */
}

void
pmap_print_pte(pmap_t pm, vaddr_t va)
{
	struct pvo_entry *pvo;
	volatile struct pte *pt;
	int pteidx;

	pvo = pmap_pvo_find_va(pm, va, &pteidx);
	if (pvo != NULL) {
		pt = pmap_pvo_to_pte(pvo, pteidx);
		if (pt != NULL) {
#if defined (PPC_OEA) || defined (PPC_OEA64)
			printf("VA %#lx -> %p -> %s %#lx, %#lx\n",
				va, pt,
				pt->pte_hi & PTE_HID ? "(sec)" : "(pri)",
				pt->pte_hi, pt->pte_lo);
#else	/* PPC_OEA64_BRIDGE */
			printf("VA %#lx -> %p -> %s %#llx, %#llx\n",
				va, pt,
				pt->pte_hi & PTE_HID ? "(sec)" : "(pri)",
				pt->pte_hi, pt->pte_lo);
#endif
		} else {
			printf("No valid PTE found\n");
		}
	} else {
		printf("Address not in pmap\n");
	}
}

void
pmap_pteg_dist(void)
{
	struct pvo_entry *pvo;
	int ptegidx;
	int depth;
	int max_depth = 0;
	unsigned int depths[64];

	memset(depths, 0, sizeof(depths));
	for (ptegidx = 0; ptegidx < pmap_pteg_cnt; ptegidx++) {
		depth = 0;
		TAILQ_FOREACH(pvo, &pmap_pvo_table[ptegidx], pvo_olink) {
			depth++;
		}
		if (depth > max_depth)
			max_depth = depth;
		if (depth > 63)
			depth = 63;
		depths[depth]++;
	}

	for (depth = 0; depth < 64; depth++) {
		printf("  [%2d]: %8u", depth, depths[depth]);
		if ((depth & 3) == 3)
			printf("\n");
		if (depth == max_depth)
			break;
	}
	if ((depth & 3) != 3)
		printf("\n");
	printf("Max depth found was %d\n", max_depth);
}
#endif /* DEBUG */

#if defined(PMAPCHECK) || defined(DEBUG)
void
pmap_pvo_verify(void)
{
	int ptegidx;
	int s;

	s = splvm();
	for (ptegidx = 0; ptegidx < pmap_pteg_cnt; ptegidx++) {
		struct pvo_entry *pvo;
		TAILQ_FOREACH(pvo, &pmap_pvo_table[ptegidx], pvo_olink) {
			if ((uintptr_t) pvo >= SEGMENT_LENGTH)
				panic("pmap_pvo_verify: invalid pvo %p "
				    "on list %#x", pvo, ptegidx);
			pmap_pvo_check(pvo);
		}
	}
	splx(s);
}
#endif /* PMAPCHECK */


void *
pmap_pool_ualloc(struct pool *pp, int flags)
{
	struct pvo_page *pvop;

	pvop = SIMPLEQ_FIRST(&pmap_upvop_head);
	if (pvop != NULL) {
		pmap_upvop_free--;
		SIMPLEQ_REMOVE_HEAD(&pmap_upvop_head, pvop_link);
		return pvop;
	}
	if (uvm.page_init_done != true) {
		return (void *) uvm_pageboot_alloc(PAGE_SIZE);
	}
	return pmap_pool_malloc(pp, flags);
}

void *
pmap_pool_malloc(struct pool *pp, int flags)
{
	struct pvo_page *pvop;
	struct vm_page *pg;

	pvop = SIMPLEQ_FIRST(&pmap_mpvop_head);
	if (pvop != NULL) {
		pmap_mpvop_free--;
		SIMPLEQ_REMOVE_HEAD(&pmap_mpvop_head, pvop_link);
		return pvop;
	}
 again:
	pg = uvm_pagealloc_strat(NULL, 0, NULL, UVM_PGA_USERESERVE,
	    UVM_PGA_STRAT_ONLY, VM_FREELIST_FIRST256);
	if (__predict_false(pg == NULL)) {
		if (flags & PR_WAITOK) {
			uvm_wait("plpg");
			goto again;
		} else {
			return (0);
		}
	}
	return (void *) VM_PAGE_TO_PHYS(pg);
}

void
pmap_pool_ufree(struct pool *pp, void *va)
{
	struct pvo_page *pvop;
#if 0
	if (PHYS_TO_VM_PAGE((paddr_t) va) != NULL) {
		pmap_pool_mfree(va, size, tag);
		return;
	}
#endif
	pvop = va;
	SIMPLEQ_INSERT_HEAD(&pmap_upvop_head, pvop, pvop_link);
	pmap_upvop_free++;
	if (pmap_upvop_free > pmap_upvop_maxfree)
		pmap_upvop_maxfree = pmap_upvop_free;
}

void
pmap_pool_mfree(struct pool *pp, void *va)
{
	struct pvo_page *pvop;

	pvop = va;
	SIMPLEQ_INSERT_HEAD(&pmap_mpvop_head, pvop, pvop_link);
	pmap_mpvop_free++;
	if (pmap_mpvop_free > pmap_mpvop_maxfree)
		pmap_mpvop_maxfree = pmap_mpvop_free;
#if 0
	uvm_pagefree(PHYS_TO_VM_PAGE((paddr_t) va));
#endif
}

/*
 * This routine in bootstraping to steal to-be-managed memory (which will
 * then be unmanaged).  We use it to grab from the first 256MB for our
 * pmap needs and above 256MB for other stuff.
 */
vaddr_t
pmap_steal_memory(vsize_t vsize, vaddr_t *vstartp, vaddr_t *vendp)
{
	vsize_t size;
	vaddr_t va;
	paddr_t pa = 0;
	int npgs, bank;
	struct vm_physseg *ps;

	if (uvm.page_init_done == true)
		panic("pmap_steal_memory: called _after_ bootstrap");

	*vstartp = VM_MIN_KERNEL_ADDRESS;
	*vendp = VM_MAX_KERNEL_ADDRESS;

	size = round_page(vsize);
	npgs = atop(size);

	/*
	 * PA 0 will never be among those given to UVM so we can use it
	 * to indicate we couldn't steal any memory.
	 */
	for (ps = vm_physmem, bank = 0; bank < vm_nphysseg; bank++, ps++) {
		if (ps->free_list == VM_FREELIST_FIRST256 && 
		    ps->avail_end - ps->avail_start >= npgs) {
			pa = ptoa(ps->avail_start);
			break;
		}
	}

	if (pa == 0)
		panic("pmap_steal_memory: no approriate memory to steal!");

	ps->avail_start += npgs;
	ps->start += npgs;

	/*
	 * If we've used up all the pages in the segment, remove it and
	 * compact the list.
	 */
	if (ps->avail_start == ps->end) {
		/*
		 * If this was the last one, then a very bad thing has occurred
		 */
		if (--vm_nphysseg == 0)
			panic("pmap_steal_memory: out of memory!");

		printf("pmap_steal_memory: consumed bank %d\n", bank);
		for (; bank < vm_nphysseg; bank++, ps++) {
			ps[0] = ps[1];
		}
	}

	va = (vaddr_t) pa;
	memset((void *) va, 0, size);
	pmap_pages_stolen += npgs;
#ifdef DEBUG
	if (pmapdebug && npgs > 1) {
		u_int cnt = 0;
		for (bank = 0, ps = vm_physmem; bank < vm_nphysseg; bank++, ps++)
			cnt += ps->avail_end - ps->avail_start;
		printf("pmap_steal_memory: stole %u (total %u) pages (%u left)\n",
		    npgs, pmap_pages_stolen, cnt);
	}
#endif

	return va;
}

/*
 * Find a chuck of memory with right size and alignment.
 */
void *
pmap_boot_find_memory(psize_t size, psize_t alignment, int at_end)
{
	struct mem_region *mp;
	paddr_t s, e;
	int i, j;

	size = round_page(size);

	DPRINTFN(BOOT,
	    ("pmap_boot_find_memory: size=%lx, alignment=%lx, at_end=%d",
	    size, alignment, at_end));

	if (alignment < PAGE_SIZE || (alignment & (alignment-1)) != 0)
		panic("pmap_boot_find_memory: invalid alignment %lx",
		    alignment);

	if (at_end) {
		if (alignment != PAGE_SIZE)
			panic("pmap_boot_find_memory: invalid ending "
			    "alignment %lx", alignment);
		
		for (mp = &avail[avail_cnt-1]; mp >= avail; mp--) {
			s = mp->start + mp->size - size;
			if (s >= mp->start && mp->size >= size) {
				DPRINTFN(BOOT,(": %lx\n", s));
				DPRINTFN(BOOT,
				    ("pmap_boot_find_memory: b-avail[%d] start "
				     "0x%lx size 0x%lx\n", mp - avail,
				     mp->start, mp->size));
				mp->size -= size;
				DPRINTFN(BOOT,
				    ("pmap_boot_find_memory: a-avail[%d] start "
				     "0x%lx size 0x%lx\n", mp - avail,
				     mp->start, mp->size));
				return (void *) s;
			}
		}
		panic("pmap_boot_find_memory: no available memory");
	}
			
	for (mp = avail, i = 0; i < avail_cnt; i++, mp++) {
		s = (mp->start + alignment - 1) & ~(alignment-1);
		e = s + size;

		/*
		 * Is the calculated region entirely within the region?
		 */
		if (s < mp->start || e > mp->start + mp->size)
			continue;

		DPRINTFN(BOOT,(": %lx\n", s));
		if (s == mp->start) {
			/*
			 * If the block starts at the beginning of region,
			 * adjust the size & start. (the region may now be
			 * zero in length)
			 */
			DPRINTFN(BOOT,
			    ("pmap_boot_find_memory: b-avail[%d] start "
			     "0x%lx size 0x%lx\n", i, mp->start, mp->size));
			mp->start += size;
			mp->size -= size;
			DPRINTFN(BOOT,
			    ("pmap_boot_find_memory: a-avail[%d] start "
			     "0x%lx size 0x%lx\n", i, mp->start, mp->size));
		} else if (e == mp->start + mp->size) {
			/*
			 * If the block starts at the beginning of region,
			 * adjust only the size.
			 */
			DPRINTFN(BOOT,
			    ("pmap_boot_find_memory: b-avail[%d] start "
			     "0x%lx size 0x%lx\n", i, mp->start, mp->size));
			mp->size -= size;
			DPRINTFN(BOOT,
			    ("pmap_boot_find_memory: a-avail[%d] start "
			     "0x%lx size 0x%lx\n", i, mp->start, mp->size));
		} else {
			/*
			 * Block is in the middle of the region, so we
			 * have to split it in two.
			 */
			for (j = avail_cnt; j > i + 1; j--) {
				avail[j] = avail[j-1];
			}
			DPRINTFN(BOOT,
			    ("pmap_boot_find_memory: b-avail[%d] start "
			     "0x%lx size 0x%lx\n", i, mp->start, mp->size));
			mp[1].start = e;
			mp[1].size = mp[0].start + mp[0].size - e;
			mp[0].size = s - mp[0].start;
			avail_cnt++;
			for (; i < avail_cnt; i++) {
				DPRINTFN(BOOT,
				    ("pmap_boot_find_memory: a-avail[%d] "
				     "start 0x%lx size 0x%lx\n", i,
				     avail[i].start, avail[i].size));
			}
		}
		return (void *) s;
	}
	panic("pmap_boot_find_memory: not enough memory for "
	    "%lx/%lx allocation?", size, alignment);
}

/* XXXSL: we dont have any BATs to do this, map in Segment 0 1:1 using page tables */
#if defined (PPC_OEA64_BRIDGE)
int
pmap_setup_segment0_map(int use_large_pages, ...)
{
    vaddr_t va;

    register_t pte_lo = 0x0;
    int ptegidx = 0, i = 0;
    struct pte pte;
    va_list ap;

    /* Coherent + Supervisor RW, no user access */
    pte_lo = PTE_M;

    /* XXXSL
     * Map in 1st segment 1:1, we'll be careful not to spill kernel entries later,
     * these have to take priority.
     */
    for (va = 0x0; va < SEGMENT_LENGTH; va += 0x1000) {
        ptegidx = va_to_pteg(pmap_kernel(), va);
        pmap_pte_create(&pte, pmap_kernel(), va, va | pte_lo);
        i = pmap_pte_insert(ptegidx, &pte);
    }

    va_start(ap, use_large_pages);
    while (1) {
        paddr_t pa;
        size_t size;

        va = va_arg(ap, vaddr_t);

        if (va == 0)
            break;

        pa = va_arg(ap, paddr_t);
        size = va_arg(ap, size_t);

        for (; va < (va + size); va += 0x1000, pa += 0x1000) {
#if 0
	    printf("%s: Inserting: va: 0x%08lx, pa: 0x%08lx\n", __FUNCTION__,  va, pa);
#endif
            ptegidx = va_to_pteg(pmap_kernel(), va);
            pmap_pte_create(&pte, pmap_kernel(), va, pa | pte_lo);
            i = pmap_pte_insert(ptegidx, &pte);
        }
    }

    TLBSYNC();
    SYNC();
    return (0);
}
#endif /* PPC_OEA64_BRIDGE */

/*
 * This is not part of the defined PMAP interface and is specific to the
 * PowerPC architecture.  This is called during initppc, before the system
 * is really initialized.
 */
void
pmap_bootstrap(paddr_t kernelstart, paddr_t kernelend)
{
	struct mem_region *mp, tmp;
	paddr_t s, e;
	psize_t size;
	int i, j;

	/*
	 * Get memory.
	 */
	mem_regions(&mem, &avail);
#if defined(DEBUG)
	if (pmapdebug & PMAPDEBUG_BOOT) {
		printf("pmap_bootstrap: memory configuration:\n");
		for (mp = mem; mp->size; mp++) {
			printf("pmap_bootstrap: mem start 0x%lx size 0x%lx\n",
				mp->start, mp->size);
		}
		for (mp = avail; mp->size; mp++) {
			printf("pmap_bootstrap: avail start 0x%lx size 0x%lx\n",
				mp->start, mp->size);
		}
	}
#endif

	/*
	 * Find out how much physical memory we have and in how many chunks.
	 */
	for (mem_cnt = 0, mp = mem; mp->size; mp++) {
		if (mp->start >= pmap_memlimit)
			continue;
		if (mp->start + mp->size > pmap_memlimit) {
			size = pmap_memlimit - mp->start;
			physmem += btoc(size);
		} else {
			physmem += btoc(mp->size);
		}
		mem_cnt++;
	}

	/*
	 * Count the number of available entries.
	 */
	for (avail_cnt = 0, mp = avail; mp->size; mp++)
		avail_cnt++;

	/*
	 * Page align all regions.
	 */
	kernelstart = trunc_page(kernelstart);
	kernelend = round_page(kernelend);
	for (mp = avail, i = 0; i < avail_cnt; i++, mp++) {
		s = round_page(mp->start);
		mp->size -= (s - mp->start);
		mp->size = trunc_page(mp->size);
		mp->start = s;
		e = mp->start + mp->size;

		DPRINTFN(BOOT,
		    ("pmap_bootstrap: b-avail[%d] start 0x%lx size 0x%lx\n",
		    i, mp->start, mp->size));

		/*
		 * Don't allow the end to run beyond our artificial limit
		 */
		if (e > pmap_memlimit)
			e = pmap_memlimit;

		/*
		 * Is this region empty or strange?  skip it.
		 */
		if (e <= s) {
			mp->start = 0;
			mp->size = 0;
			continue;
		}

		/*
		 * Does this overlap the beginning of kernel?
		 *   Does extend past the end of the kernel?
		 */
		else if (s < kernelstart && e > kernelstart) {
			if (e > kernelend) {
				avail[avail_cnt].start = kernelend;
				avail[avail_cnt].size = e - kernelend;
				avail_cnt++;
			}
			mp->size = kernelstart - s;
		}
		/*
		 * Check whether this region overlaps the end of the kernel.
		 */
		else if (s < kernelend && e > kernelend) {
			mp->start = kernelend;
			mp->size = e - kernelend;
		}
		/*
		 * Look whether this regions is completely inside the kernel.
		 * Nuke it if it does.
		 */
		else if (s >= kernelstart && e <= kernelend) {
			mp->start = 0;
			mp->size = 0;
		}
		/*
		 * If the user imposed a memory limit, enforce it.
		 */
		else if (s >= pmap_memlimit) {
			mp->start = -PAGE_SIZE;	/* let's know why */
			mp->size = 0;
		}
		else {
			mp->start = s;
			mp->size = e - s;
		}
		DPRINTFN(BOOT,
		    ("pmap_bootstrap: a-avail[%d] start 0x%lx size 0x%lx\n",
		    i, mp->start, mp->size));
	}

	/*
	 * Move (and uncount) all the null return to the end.
	 */
	for (mp = avail, i = 0; i < avail_cnt; i++, mp++) {
		if (mp->size == 0) {
			tmp = avail[i];
			avail[i] = avail[--avail_cnt];
			avail[avail_cnt] = avail[i];
		}
	}

	/*
	 * (Bubble)sort them into asecnding order.
	 */
	for (i = 0; i < avail_cnt; i++) {
		for (j = i + 1; j < avail_cnt; j++) {
			if (avail[i].start > avail[j].start) {
				tmp = avail[i];
				avail[i] = avail[j];
				avail[j] = tmp;
			}
		}
	}

	/*
	 * Make sure they don't overlap.
	 */
	for (mp = avail, i = 0; i < avail_cnt - 1; i++, mp++) {
		if (mp[0].start + mp[0].size > mp[1].start) {
			mp[0].size = mp[1].start - mp[0].start;
		}
		DPRINTFN(BOOT,
		    ("pmap_bootstrap: avail[%d] start 0x%lx size 0x%lx\n",
		    i, mp->start, mp->size));
	}
	DPRINTFN(BOOT,
	    ("pmap_bootstrap: avail[%d] start 0x%lx size 0x%lx\n",
	    i, mp->start, mp->size));

#ifdef	PTEGCOUNT
	pmap_pteg_cnt = PTEGCOUNT;
#else /* PTEGCOUNT */

	pmap_pteg_cnt = 0x1000;
	
	while (pmap_pteg_cnt < physmem)
		pmap_pteg_cnt <<= 1;

	pmap_pteg_cnt >>= 1;
#endif /* PTEGCOUNT */

#ifdef DEBUG
	DPRINTFN(BOOT,
		("pmap_pteg_cnt: 0x%x\n", pmap_pteg_cnt));
#endif

	/*
	 * Find suitably aligned memory for PTEG hash table.
	 */
	size = pmap_pteg_cnt * sizeof(struct pteg);
	pmap_pteg_table = pmap_boot_find_memory(size, size, 0);

#ifdef DEBUG
	DPRINTFN(BOOT,
		("PTEG cnt: 0x%x HTAB size: 0x%08x bytes, address: %p\n", pmap_pteg_cnt, (unsigned int)size, pmap_pteg_table));
#endif


#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
	if ( (uintptr_t) pmap_pteg_table + size > SEGMENT_LENGTH)
		panic("pmap_bootstrap: pmap_pteg_table end (%p + %lx) > 256MB",
		    pmap_pteg_table, size);
#endif

	memset(__UNVOLATILE(pmap_pteg_table), 0,
		pmap_pteg_cnt * sizeof(struct pteg));
	pmap_pteg_mask = pmap_pteg_cnt - 1;

	/*
	 * We cannot do pmap_steal_memory here since UVM hasn't been loaded
	 * with pages.  So we just steal them before giving them to UVM.
	 */
	size = sizeof(pmap_pvo_table[0]) * pmap_pteg_cnt;
	pmap_pvo_table = pmap_boot_find_memory(size, PAGE_SIZE, 0);
#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
	if ( (uintptr_t) pmap_pvo_table + size > SEGMENT_LENGTH)
		panic("pmap_bootstrap: pmap_pvo_table end (%p + %lx) > 256MB",
		    pmap_pvo_table, size);
#endif

	for (i = 0; i < pmap_pteg_cnt; i++)
		TAILQ_INIT(&pmap_pvo_table[i]);

#ifndef MSGBUFADDR
	/*
	 * Allocate msgbuf in high memory.
	 */
	msgbuf_paddr =
	    (paddr_t) pmap_boot_find_memory(MSGBUFSIZE, PAGE_SIZE, 1);
#endif

#ifdef __HAVE_PMAP_PHYSSEG
	{
		u_int npgs = 0;
		for (i = 0, mp = avail; i < avail_cnt; i++, mp++)
			npgs += btoc(mp->size);
		size = (sizeof(struct pvo_head) + 1) * npgs;
		pmap_physseg.pvoh = pmap_boot_find_memory(size, PAGE_SIZE, 0);
		pmap_physseg.attrs = (char *) &pmap_physseg.pvoh[npgs];
#if defined(DIAGNOSTIC) || defined(DEBUG) || defined(PMAPCHECK)
		if ((uintptr_t)pmap_physseg.pvoh + size > SEGMENT_LENGTH)
			panic("pmap_bootstrap: PVO list end (%p + %lx) > 256MB",
			    pmap_physseg.pvoh, size);
#endif
	}
#endif


	for (mp = avail, i = 0; i < avail_cnt; mp++, i++) {
		paddr_t pfstart = atop(mp->start);
		paddr_t pfend = atop(mp->start + mp->size);
		if (mp->size == 0)
			continue;
		if (mp->start + mp->size <= SEGMENT_LENGTH) {
			uvm_page_physload(pfstart, pfend, pfstart, pfend,
				VM_FREELIST_FIRST256);
		} else if (mp->start >= SEGMENT_LENGTH) {
			uvm_page_physload(pfstart, pfend, pfstart, pfend,
				VM_FREELIST_DEFAULT);
		} else {
			pfend = atop(SEGMENT_LENGTH);
			uvm_page_physload(pfstart, pfend, pfstart, pfend,
				VM_FREELIST_FIRST256);
			pfstart = atop(SEGMENT_LENGTH);
			pfend = atop(mp->start + mp->size);
			uvm_page_physload(pfstart, pfend, pfstart, pfend,
				VM_FREELIST_DEFAULT);
		}
	}

	/*
	 * Make sure kernel vsid is allocated as well as VSID 0.
	 */
	pmap_vsid_bitmap[(KERNEL_VSIDBITS & (NPMAPS-1)) / VSID_NBPW]
		|= 1 << (KERNEL_VSIDBITS % VSID_NBPW);
	pmap_vsid_bitmap[0] |= 1;

	/*
	 * Initialize kernel pmap and hardware.
	 */

/* PPC_OEA64_BRIDGE does support these instructions */
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	for (i = 0; i < 16; i++) {
 		pmap_kernel()->pm_sr[i] = KERNELN_SEGMENT(i)|SR_PRKEY;
		__asm volatile ("mtsrin %0,%1"
 			      :: "r"(KERNELN_SEGMENT(i)|SR_PRKEY), "r"(i << ADDR_SR_SHFT));
	}

	pmap_kernel()->pm_sr[KERNEL_SR] = KERNEL_SEGMENT|SR_SUKEY|SR_PRKEY;
	__asm volatile ("mtsr %0,%1"
		      :: "n"(KERNEL_SR), "r"(KERNEL_SEGMENT));
#ifdef KERNEL2_SR
	pmap_kernel()->pm_sr[KERNEL2_SR] = KERNEL2_SEGMENT|SR_SUKEY|SR_PRKEY;
	__asm volatile ("mtsr %0,%1"
		      :: "n"(KERNEL2_SR), "r"(KERNEL2_SEGMENT));
#endif
	for (i = 0; i < 16; i++) {
		if (iosrtable[i] & SR601_T) {
			pmap_kernel()->pm_sr[i] = iosrtable[i];
			__asm volatile ("mtsrin %0,%1"
			    :: "r"(iosrtable[i]), "r"(i << ADDR_SR_SHFT));
		}
	}
#endif /* PPC_OEA || PPC_OEA64_BRIDGE */
#if defined (PPC_OEA)
	__asm volatile ("sync; mtsdr1 %0; isync"
		      :: "r"((uintptr_t)pmap_pteg_table | (pmap_pteg_mask >> 10)));
#elif defined (PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
 	__asm __volatile ("sync; mtsdr1 %0; isync"
 		      :: "r"((uintptr_t)pmap_pteg_table | (32 - cntlzw(pmap_pteg_mask >> 11))));
#endif
	tlbia();

#ifdef ALTIVEC
	pmap_use_altivec = cpu_altivec;
#endif

#ifdef DEBUG
	if (pmapdebug & PMAPDEBUG_BOOT) {
		u_int cnt;
		int bank;
		char pbuf[9];
		for (cnt = 0, bank = 0; bank < vm_nphysseg; bank++) {
			cnt += vm_physmem[bank].avail_end - vm_physmem[bank].avail_start;
			printf("pmap_bootstrap: vm_physmem[%d]=%#lx-%#lx/%#lx\n",
			    bank,
			    ptoa(vm_physmem[bank].avail_start),
			    ptoa(vm_physmem[bank].avail_end),
			    ptoa(vm_physmem[bank].avail_end - vm_physmem[bank].avail_start));
		}
		format_bytes(pbuf, sizeof(pbuf), ptoa((u_int64_t) cnt));
		printf("pmap_bootstrap: UVM memory = %s (%u pages)\n",
		    pbuf, cnt);
	}
#endif

	pool_init(&pmap_upvo_pool, sizeof(struct pvo_entry),
	    sizeof(struct pvo_entry), 0, 0, "pmap_upvopl",
	    &pmap_pool_uallocator, IPL_NONE);

	pool_setlowat(&pmap_upvo_pool, 252);

	pool_init(&pmap_pool, sizeof(struct pmap),
	    sizeof(void *), 0, 0, "pmap_pl", &pmap_pool_uallocator,
	    IPL_NONE);

#if defined(PMAP_NEED_MAPKERNEL)
	{
		extern int etext[], kernel_text[];
		vaddr_t va, va_etext = (paddr_t) etext;
		paddr_t pa;
		register_t sr;

		sr = KERNELN_SEGMENT(kernelstart >> ADDR_SR_SHFT)
		    |SR_SUKEY|SR_PRKEY;

		va = (vaddr_t) kernel_text;

		for (pa = kernelstart; va < va_etext;
		     pa += PAGE_SIZE, va += PAGE_SIZE)
			pmap_enter(pmap_kernel(), va, pa,
			    VM_PROT_READ|VM_PROT_EXECUTE, 0);

		for (; pa < kernelend;
		     pa += PAGE_SIZE, va += PAGE_SIZE)
			pmap_enter(pmap_kernel(), va, pa,
			    VM_PROT_READ|VM_PROT_WRITE, 0);

		pmap_kernel()->pm_sr[kernelstart >> ADDR_SR_SHFT] = sr;
		__asm volatile ("mtsrin %0,%1"
 			      :: "r"(sr), "r"(kernelstart));
	}
#endif
}
