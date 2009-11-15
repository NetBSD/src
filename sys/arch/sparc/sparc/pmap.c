/*	$NetBSD: pmap.c,v 1.322.20.2 2009/11/15 05:58:38 snj Exp $ */

/*
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Aaron Brown and
 *	Harvard University.
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
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
 *	@(#)pmap.c	8.4 (Berkeley) 2/5/94
 *
 */

/*
 * SPARC physical map management code.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.322.20.2 2009/11/15 05:58:38 snj Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/pool.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/kernel.h>
#include <sys/atomic.h>

#include <uvm/uvm.h>

#include <machine/autoconf.h>
#include <machine/bsd_openprom.h>
#include <machine/oldmon.h>
#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <machine/kcore.h>

#include <sparc/sparc/asm.h>
#include <sparc/sparc/cache.h>
#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/cpuvar.h>

/*
 * The SPARCstation offers us the following challenges:
 *
 *   1. A virtual address cache.  This is, strictly speaking, not
 *	part of the architecture, but the code below assumes one.
 *	This is a write-through cache on the 4c and a write-back cache
 *	on others.
 *
 *   2. (4/4c only) An MMU that acts like a cache.  There is not enough
 *	space in the MMU to map everything all the time.  Instead, we need
 *	to load MMU with the `working set' of translations for each
 *	process. The sun4m does not act like a cache; tables are maintained
 *	in physical memory.
 *
 *   3.	Segmented virtual and physical spaces.  The upper 12 bits of
 *	a virtual address (the virtual segment) index a segment table,
 *	giving a physical segment.  The physical segment selects a
 *	`Page Map Entry Group' (PMEG) and the virtual page number---the
 *	next 5 or 6 bits of the virtual address---select the particular
 *	`Page Map Entry' for the page.  We call the latter a PTE and
 *	call each Page Map Entry Group a pmeg (for want of a better name).
 *	Note that the sun4m has an unsegmented 36-bit physical space.
 *
 *	Since there are no valid bits in the segment table, the only way
 *	to have an invalid segment is to make one full pmeg of invalid PTEs.
 *	We use the last one (since the ROM does as well) (sun4/4c only)
 *
 *   4. Discontiguous physical pages.  The Mach VM expects physical pages
 *	to be in one sequential lump.
 *
 *   5. The MMU is always on: it is not possible to disable it.  This is
 *	mainly a startup hassle.
 */

struct pmap_stats {
	int	ps_unlink_pvfirst;	/* # of pv_unlinks on head */
	int	ps_unlink_pvsearch;	/* # of pv_unlink searches */
	int	ps_changeprots;		/* # of calls to changeprot */
	int	ps_enter_firstpv;	/* pv heads entered */
	int	ps_enter_secondpv;	/* pv nonheads entered */
	int	ps_useless_changewire;	/* useless wiring changes */
	int	ps_npg_prot_all;	/* # of active pages protected */
	int	ps_npg_prot_actual;	/* # pages actually affected */
	int	ps_npmeg_free;		/* # of free pmegs */
	int	ps_npmeg_locked;	/* # of pmegs on locked list */
	int	ps_npmeg_lru;		/* # of pmegs on lru list */
} pmap_stats;

#if defined(SUN4) || defined(SUN4C)
struct evcnt mmu_stolenpmegs_evcnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR,0,"mmu","stln pmgs");
EVCNT_ATTACH_STATIC(mmu_stolenpmegs_evcnt);

struct evcnt mmu_pagein_evcnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR,0,"mmu","pagein");
EVCNT_ATTACH_STATIC(mmu_pagein_evcnt);
#endif /* SUN4 || SUN4C */

#ifdef DEBUG
#define	PDB_CREATE	0x0001
#define	PDB_DESTROY	0x0002
#define	PDB_REMOVE	0x0004
#define	PDB_CHANGEPROT	0x0008
#define	PDB_ENTER	0x0010
#define	PDB_FOLLOW	0x0020

#define	PDB_MMU_ALLOC	0x0100
#define	PDB_MMU_STEAL	0x0200
#define	PDB_CTX_ALLOC	0x0400
#define	PDB_CTX_STEAL	0x0800
#define	PDB_MMUREG_ALLOC	0x1000
#define	PDB_MMUREG_STEAL	0x2000
#define	PDB_CACHESTUFF	0x4000
#define	PDB_SWITCHMAP	0x8000
#define	PDB_SANITYCHK	0x10000
int	pmapdebug = 0;
#endif

/*
 * Bounds on managed physical addresses. Used by (MD) users
 * of uvm_pglistalloc() to provide search hints.
 */
paddr_t	vm_first_phys = (paddr_t)-1;
paddr_t	vm_last_phys = 0;
psize_t vm_num_phys;

#define	PMAP_LOCK()	KERNEL_LOCK(1, NULL)
#define	PMAP_UNLOCK()	KERNEL_UNLOCK_ONE(NULL)

/*
 * Flags in pvlist.pv_flags.  Note that PV_MOD must be 1 and PV_REF must be 2
 * since they must line up with the bits in the hardware PTEs (see pte.h).
 * SUN4M bits are at a slightly different location in the PTE.
 *
 * Note: the REF, MOD and ANC flag bits occur only in the head of a pvlist.
 * The NC bit is meaningful in each individual pv entry and reflects the
 * requested non-cacheability at the time the entry was made through
 * pv_link() or when subsequently altered by kvm_uncache() (but the latter
 * does not happen in kernels as of the time of this writing (March 2001)).
 */
#define PV_MOD		1	/* page modified */
#define PV_REF		2	/* page referenced */
#define PV_NC		4	/* page cannot be cached */
#define PV_REF4M	1	/* page referenced (SRMMU) */
#define PV_MOD4M	2	/* page modified (SRMMU) */
#define PV_ANC		0x10	/* page has incongruent aliases */

static struct pool pv_pool;

/*
 * pvhead(pte): find a VM page given a PTE entry.
 */
#if defined(SUN4) || defined(SUN4C)
static struct vm_page *
pvhead4_4c(u_int pte)
{
	paddr_t pa = (pte & PG_PFNUM) << PGSHIFT;

	return (PHYS_TO_VM_PAGE(pa));
}
#endif

#if defined(SUN4M) || defined(SUN4D)
static struct vm_page *
pvhead4m(u_int pte)
{
	paddr_t pa = (pte & SRMMU_PPNMASK) << SRMMU_PPNPASHIFT;

	return (PHYS_TO_VM_PAGE(pa));
}
#endif

/*
 * Each virtual segment within each pmap is either valid or invalid.
 * It is valid if pm_npte[VA_VSEG(va)] is not 0.  This does not mean
 * it is in the MMU, however; that is true iff pm_segmap[VA_VSEG(va)]
 * does not point to the invalid PMEG.
 *
 * In the older SPARC architectures (sun4/sun4c), page tables are cached in
 * the MMU. The following discussion applies to these architectures:
 *
 * If a virtual segment is valid and loaded, the correct PTEs appear
 * in the MMU only.  If it is valid and unloaded, the correct PTEs appear
 * in the pm_pte[VA_VSEG(va)] only.  However, some effort is made to keep
 * the software copies consistent enough with the MMU so that libkvm can
 * do user address translations.  In particular, pv_changepte() and
 * pmap_enu() maintain consistency, while less critical changes are
 * not maintained.  pm_pte[VA_VSEG(va)] always points to space for those
 * PTEs.
 *
 * Each PMEG in the MMU is either free or contains PTEs corresponding to
 * some pmap and virtual segment.  If it contains some PTEs, it also contains
 * reference and modify bits that belong in the pv_table.  If we need
 * to steal a PMEG from some process (if we need one and none are free)
 * we must copy the ref and mod bits, and update pm_segmap in the other
 * pmap to show that its virtual segment is no longer in the MMU.
 *
 * There are 128 PMEGs in a small Sun-4, of which only a few dozen are
 * tied down permanently, leaving `about' 100 to be spread among
 * running processes.  These are managed as an LRU cache.  Before
 * calling the VM paging code for a user page fault, the fault handler
 * calls mmu_load(pmap, va) to try to get a set of PTEs put into the
 * MMU.  mmu_load will check the validity of the segment and tell whether
 * it did something.
 *
 * Since I hate the name PMEG I call this data structure an `mmu entry'.
 * Each mmuentry is on exactly one of three `usage' lists: free, LRU,
 * or locked.  The locked list is only used for kernel mappings that need
 * to be wired down.
 *
 *
 * In the sun4m architecture using the SPARC Reference MMU (SRMMU), three
 * levels of page tables are maintained in physical memory. We use the same
 * structures as with the 3-level old-style MMU (pm_regmap, pm_segmap,
 * rg_segmap, sg_pte, etc) to maintain kernel-edible page tables; we also
 * build a parallel set of physical tables that can be used by the MMU.
 * (XXX: This seems redundant, but is it necessary for the unified kernel?)
 *
 * If a virtual segment is valid, its entries will be in both parallel lists.
 * If it is not valid, then its entry in the kernel tables will be zero, and
 * its entry in the MMU tables will either be nonexistent or zero as well.
 *
 * The Reference MMU generally uses a Translation Look-aside Buffer (TLB)
 * to cache the result of recently executed page table walks. When
 * manipulating page tables, we need to ensure consistency of the
 * in-memory and TLB copies of the page table entries. This is handled
 * by flushing (and invalidating) a TLB entry when appropriate before
 * altering an in-memory page table entry.
 */
struct mmuentry {
	CIRCLEQ_ENTRY(mmuentry)	me_list;	/* usage list link */
	TAILQ_ENTRY(mmuentry)	me_pmchain;	/* pmap owner link */
	struct	pmap *me_pmap;		/* pmap, if in use */
	u_short	me_vreg;		/* associated virtual region/segment */
	u_short	me_vseg;		/* associated virtual region/segment */
	u_short	me_cookie;		/* hardware SMEG/PMEG number */
#ifdef DIAGNOSTIC
	int *me_statp;/*XXX*/
#endif
};
struct mmuentry *mmusegments;	/* allocated in pmap_bootstrap */
struct mmuentry *mmuregions;	/* allocated in pmap_bootstrap */

CIRCLEQ_HEAD(mmuq, mmuentry);
struct mmuq segm_freelist, segm_lru, segm_locked;
struct mmuq region_freelist, region_lru, region_locked;
/*
 * We use a circular queue, since that allows us to remove an element
 * from a list without knowing the list header.
 */
#define CIRCLEQ_REMOVE_NOH(elm, field) do {				\
	(elm)->field.cqe_next->field.cqe_prev = (elm)->field.cqe_prev;	\
	(elm)->field.cqe_prev->field.cqe_next = (elm)->field.cqe_next;	\
} while (/*CONSTCOND*/0)

#define MMUQ_INIT(head)			CIRCLEQ_INIT(head)
#define MMUQ_REMOVE(elm,field)		CIRCLEQ_REMOVE_NOH(elm,field)
#define MMUQ_INSERT_TAIL(head,elm,field)CIRCLEQ_INSERT_TAIL(head,elm,field)
#define MMUQ_EMPTY(head)		CIRCLEQ_EMPTY(head)
#define MMUQ_FIRST(head)		CIRCLEQ_FIRST(head)


int	seginval;		/* [4/4c] the invalid segment number */
int	reginval;		/* [4/3mmu] the invalid region number */

static kmutex_t demap_lock;

/*
 * (sun4/4c)
 * A context is simply a small number that dictates which set of 4096
 * segment map entries the MMU uses.  The Sun 4c has eight (SS1,IPC) or
 * sixteen (SS2,IPX) such sets. These are alloted in an `almost MRU' fashion.
 * (sun4m)
 * A context is simply a small number that indexes the context table, the
 * root-level page table mapping 4G areas. Each entry in this table points
 * to a 1st-level region table. A SPARC reference MMU will usually use 16
 * such contexts, but some offer as many as 64k contexts; the theoretical
 * maximum is 2^32 - 1, but this would create overlarge context tables.
 *
 * Each context is either free or attached to a pmap.
 *
 * Since the virtual address cache is tagged by context, when we steal
 * a context we have to flush (that part of) the cache.
 */
union ctxinfo {
	union	ctxinfo *c_nextfree;	/* free list (if free) */
	struct	pmap *c_pmap;		/* pmap (if busy) */
};

static kmutex_t	ctx_lock;		/* lock for below */
union	ctxinfo *ctxinfo;		/* allocated at in pmap_bootstrap */
union	ctxinfo *ctx_freelist;		/* context free list */
int	ctx_kick;			/* allocation rover when none free */
int	ctx_kickdir;			/* ctx_kick roves both directions */
int	ncontext;			/* sizeof ctx_freelist */

void	ctx_alloc(struct pmap *);
void	ctx_free(struct pmap *);

void *	vmmap;			/* one reserved MI vpage for /dev/mem */
/*void *	vdumppages;	-* 32KB worth of reserved dump pages */

smeg_t		tregion;	/* [4/3mmu] Region for temporary mappings */

struct pmap	kernel_pmap_store;		/* the kernel's pmap */
struct regmap	kernel_regmap_store[NKREG];	/* the kernel's regmap */
struct segmap	kernel_segmap_store[NKREG*NSEGRG];/* the kernel's segmaps */

#if defined(SUN4M) || defined(SUN4D)
u_int 	*kernel_regtable_store;		/* 1k of storage to map the kernel */
u_int	*kernel_segtable_store;		/* 2k of storage to map the kernel */
u_int	*kernel_pagtable_store;		/* 128k of storage to map the kernel */

/*
 * Memory pools and back-end supplier for SRMMU page tables.
 * Share a pool between the level 2 and level 3 page tables,
 * since these are equal in size.
 */
static struct pool L1_pool;
static struct pool L23_pool;

static void *pgt_page_alloc(struct pool *, int);
static void  pgt_page_free(struct pool *, void *);

static struct pool_allocator pgt_page_allocator = {
	pgt_page_alloc, pgt_page_free, 0,
};

#endif /* SUN4M || SUN4D */

#if defined(SUN4) || defined(SUN4C)
/*
 * Memory pool for user and kernel PTE tables.
 */
static struct pool pte_pool;
#endif

struct	memarr *pmemarr;	/* physical memory regions */
int	npmemarr;		/* number of entries in pmemarr */

static paddr_t	avail_start;	/* first available physical page, other
				   than the `etext gap' defined below */
static vaddr_t	etext_gap_start;/* start of gap between text & data */
static vaddr_t	etext_gap_end;	/* end of gap between text & data */
static vaddr_t	virtual_avail;	/* first free kernel virtual address */
static vaddr_t	virtual_end;	/* last free kernel virtual address */

static void pmap_page_upload(void);

int mmu_has_hole;

vaddr_t prom_vstart;	/* For /dev/kmem */
vaddr_t prom_vend;

/*
 * Memory pool for pmap structures.
 */
static struct pool_cache pmap_cache;
static int	pmap_pmap_pool_ctor(void *, void *, int);
static void	pmap_pmap_pool_dtor(void *, void *);
static struct pool segmap_pool;

#if defined(SUN4)
/*
 * [sun4]: segfixmask: on some systems (4/110) "getsegmap()" returns a
 * partly invalid value. getsegmap returns a 16 bit value on the sun4,
 * but only the first 8 or so bits are valid (the rest are *supposed* to
 * be zero. On the 4/110 the bits that are supposed to be zero are
 * all one instead. e.g. KERNBASE is usually mapped by pmeg number zero.
 * On a 4/300 getsegmap(KERNBASE) == 0x0000, but
 * on a 4/100 getsegmap(KERNBASE) == 0xff00
 *
 * This confuses mmu_reservemon() and causes it to not reserve the PROM's
 * pmegs. Then the PROM's pmegs get used during autoconfig and everything
 * falls apart!  (not very fun to debug, BTW.)
 *
 * solution: mask the invalid bits in the getsetmap macro.
 */

static u_int segfixmask = 0xffffffff; /* all bits valid to start */
#else
#define segfixmask 0xffffffff	/* It's in getsegmap's scope */
#endif

/*
 * pseudo-functions for mnemonic value
 */
#define	getsegmap(va)		(CPU_ISSUN4C \
					? lduba(va, ASI_SEGMAP) \
					: (lduha(va, ASI_SEGMAP) & segfixmask))
#define	setsegmap(va, pmeg)	(CPU_ISSUN4C \
					? stba(va, ASI_SEGMAP, pmeg) \
					: stha(va, ASI_SEGMAP, pmeg))

/* 3-level sun4 MMU only: */
#define	getregmap(va)		((unsigned)lduha((va)+2, ASI_REGMAP) >> 8)
#define	setregmap(va, smeg)	stha((va)+2, ASI_REGMAP, (smeg << 8))


#if defined(SUN4M) || defined(SUN4D)
#if 0
#if VM_PROT_READ != 1 || VM_PROT_WRITE != 2 || VM_PROT_EXECUTE != 4
#error fix protection code translation table
#endif
#endif
/*
 * Translation table for kernel vs. PTE protection bits.
 */
const u_int protection_codes[2][8] = {
	/* kernel */
	{
	PPROT_N_RX,	/* VM_PROT_NONE    | VM_PROT_NONE  | VM_PROT_NONE */
	PPROT_N_RX,	/* VM_PROT_NONE    | VM_PROT_NONE  | VM_PROT_READ */
	PPROT_N_RWX,	/* VM_PROT_NONE    | VM_PROT_WRITE | VM_PROT_NONE */
	PPROT_N_RWX,	/* VM_PROT_NONE    | VM_PROT_WRITE | VM_PROT_READ */
	PPROT_N_RX,	/* VM_PROT_EXECUTE | VM_PROT_NONE  | VM_PROT_NONE */
	PPROT_N_RX,	/* VM_PROT_EXECUTE | VM_PROT_NONE  | VM_PROT_READ */
	PPROT_N_RWX,	/* VM_PROT_EXECUTE | VM_PROT_WRITE | VM_PROT_NONE */
	PPROT_N_RWX,	/* VM_PROT_EXECUTE | VM_PROT_WRITE | VM_PROT_READ */
	},

	/* user */
	{
	PPROT_N_RX,	/* VM_PROT_NONE    | VM_PROT_NONE  | VM_PROT_NONE */
	PPROT_R_R,	/* VM_PROT_NONE    | VM_PROT_NONE  | VM_PROT_READ */
	PPROT_RW_RW,	/* VM_PROT_NONE    | VM_PROT_WRITE | VM_PROT_NONE */
	PPROT_RW_RW,	/* VM_PROT_NONE    | VM_PROT_WRITE | VM_PROT_READ */
	PPROT_X_X,	/* VM_PROT_EXECUTE | VM_PROT_NONE  | VM_PROT_NONE */
	PPROT_RX_RX,	/* VM_PROT_EXECUTE | VM_PROT_NONE  | VM_PROT_READ */
	PPROT_RWX_RWX,	/* VM_PROT_EXECUTE | VM_PROT_WRITE | VM_PROT_NONE */
	PPROT_RWX_RWX,	/* VM_PROT_EXECUTE | VM_PROT_WRITE | VM_PROT_READ */
	}
};
#define pte_kprot4m(prot) (protection_codes[0][(prot)])
#define pte_uprot4m(prot) (protection_codes[1][(prot)])
#define pte_prot4m(pm, prot) \
	(protection_codes[(pm) == pmap_kernel() ? 0 : 1][(prot)])

void		setpte4m(vaddr_t va, int pte);
void		setpgt4m(int *ptep, int pte);
void		setpgt4m_va(vaddr_t, int *, int, int, int, u_int);
int		updatepte4m(vaddr_t, int *, int, int, int, u_int);
#endif /* SUN4M || SUN4D */

#if defined(MULTIPROCESSOR)
#define PMAP_SET_CPUSET(pmap, cpi)	\
	(pmap->pm_cpuset |= (1 << (cpi)->ci_cpuid))
#define PMAP_CLR_CPUSET(pmap, cpi)	\
	(pmap->pm_cpuset &= ~(1 << (cpi)->ci_cpuid))
#define PMAP_CPUSET(pmap)		(pmap->pm_cpuset)
#else
#define PMAP_SET_CPUSET(pmap, cpi)	/* nothing */
#define PMAP_CLR_CPUSET(pmap, cpi)	/* nothing */
#define PMAP_CPUSET(pmap)		1	/* XXX: 1 or 0? */
#endif /* MULTIPROCESSOR */


/* Function pointer messiness for supporting multiple sparc architectures
 * within a single kernel: notice that there are two versions of many of the
 * functions within this file/module, one for the sun4/sun4c and the other
 * for the sun4m. For performance reasons (since things like pte bits don't
 * map nicely between the two architectures), there are separate functions
 * rather than unified functions which test the cputyp variable. If only
 * one architecture is being used, then the non-suffixed function calls
 * are macro-translated into the appropriate xxx4_4c or xxx4m call. If
 * multiple architectures are defined, the calls translate to (*xxx_p),
 * i.e. they indirect through function pointers initialized as appropriate
 * to the run-time architecture in pmap_bootstrap. See also pmap.h.
 */

#if defined(SUN4M) || defined(SUN4D)
static void mmu_setup4m_L1(int, struct pmap *);
static void mmu_setup4m_L2(int, struct regmap *);
static void  mmu_setup4m_L3(int, struct segmap *);
/*static*/ void	mmu_reservemon4m(struct pmap *);

/*static*/ void pmap_changeprot4m(pmap_t, vaddr_t, vm_prot_t, int);
/*static*/ void pmap_rmk4m(struct pmap *, vaddr_t, vaddr_t, int, int);
/*static*/ void pmap_rmu4m(struct pmap *, vaddr_t, vaddr_t, int, int);
/*static*/ int  pmap_enk4m(struct pmap *, vaddr_t, vm_prot_t,
				int, struct vm_page *, int);
/*static*/ int  pmap_enu4m(struct pmap *, vaddr_t, vm_prot_t,
				int, struct vm_page *, int);
/*static*/ void pv_changepte4m(struct vm_page *, int, int);
/*static*/ int  pv_syncflags4m(struct vm_page *);
/*static*/ int  pv_link4m(struct vm_page *, struct pmap *, vaddr_t, u_int *);
/*static*/ void pv_unlink4m(struct vm_page *, struct pmap *, vaddr_t);
#endif

#if defined(SUN4) || defined(SUN4C)
/*static*/ void	mmu_reservemon4_4c(int *, int *);
/*static*/ void pmap_changeprot4_4c(pmap_t, vaddr_t, vm_prot_t, int);
/*static*/ void pmap_rmk4_4c(struct pmap *, vaddr_t, vaddr_t, int, int);
/*static*/ void pmap_rmu4_4c(struct pmap *, vaddr_t, vaddr_t, int, int);
/*static*/ int  pmap_enk4_4c(struct pmap *, vaddr_t, vm_prot_t,
				  int, struct vm_page *, int);
/*static*/ int  pmap_enu4_4c(struct pmap *, vaddr_t, vm_prot_t,
				  int, struct vm_page *, int);
/*static*/ void pv_changepte4_4c(struct vm_page *, int, int);
/*static*/ int  pv_syncflags4_4c(struct vm_page *);
/*static*/ int  pv_link4_4c(struct vm_page *, struct pmap *, vaddr_t, u_int *);
/*static*/ void pv_unlink4_4c(struct vm_page *, struct pmap *, vaddr_t);
#endif

#if !(defined(SUN4M) || defined(SUN4D)) && (defined(SUN4) || defined(SUN4C))
#define		pmap_rmk	pmap_rmk4_4c
#define		pmap_rmu	pmap_rmu4_4c

#elif (defined(SUN4M) || defined(SUN4D)) && !(defined(SUN4) || defined(SUN4C))
#define		pmap_rmk	pmap_rmk4m
#define		pmap_rmu	pmap_rmu4m

#else  /* must use function pointers */

/* function pointer declarations */
/* from pmap.h: */
bool		(*pmap_clear_modify_p)(struct vm_page *);
bool		(*pmap_clear_reference_p)(struct vm_page *);
int		(*pmap_enter_p)(pmap_t, vaddr_t, paddr_t, vm_prot_t, int);
bool		(*pmap_extract_p)(pmap_t, vaddr_t, paddr_t *);
bool		(*pmap_is_modified_p)(struct vm_page *);
bool		(*pmap_is_referenced_p)(struct vm_page *);
void		(*pmap_kenter_pa_p)(vaddr_t, paddr_t, vm_prot_t);
void		(*pmap_kremove_p)(vaddr_t, vsize_t);
void		(*pmap_kprotect_p)(vaddr_t, vsize_t, vm_prot_t);
void		(*pmap_page_protect_p)(struct vm_page *, vm_prot_t);
void		(*pmap_protect_p)(pmap_t, vaddr_t, vaddr_t, vm_prot_t);
/* local: */
void 		(*pmap_rmk_p)(struct pmap *, vaddr_t, vaddr_t, int, int);
void 		(*pmap_rmu_p)(struct pmap *, vaddr_t, vaddr_t, int, int);

#define		pmap_rmk	(*pmap_rmk_p)
#define		pmap_rmu	(*pmap_rmu_p)

#endif

/* --------------------------------------------------------------*/

/*
 * Next we have some sun4m/4d-specific routines which have no 4/4c
 * counterparts, or which are 4/4c macros.
 */

#if defined(SUN4M) || defined(SUN4D)
/*
 * SP versions of the tlb flush operations.
 *
 * Turn off traps to prevent register window overflows
 * from writing user windows to the wrong stack.
 */
static void
sp_tlb_flush(int va, int ctx, int lvl)
{

	/* Traps off */
	__asm("rd	%psr, %o3");
	__asm("wr	%%o3, %0, %%psr" :: "n" (PSR_ET));

	/* Save context */
	__asm("mov	%0, %%o4" :: "n"(SRMMU_CXR));
	__asm("lda	[%%o4]%0, %%o5" :: "n"(ASI_SRMMU));

	/* Set new context and flush type bits */
	__asm("andn	%o0, 0xfff, %o0");
	__asm("sta	%%o1, [%%o4]%0" :: "n"(ASI_SRMMU));
	__asm("or	%o0, %o2, %o0");

	/* Do the TLB flush */
	__asm("sta	%%g0, [%%o0]%0" :: "n"(ASI_SRMMUFP));

	/* Restore context */
	__asm("sta	%%o5, [%%o4]%0" :: "n"(ASI_SRMMU));

	/* and turn traps on again */
	__asm("wr	%o3, 0, %psr");
	__asm("nop");
	__asm("nop");
	__asm("nop");
}

static inline void
sp_tlb_flush_all(void)
{

	sta(ASI_SRMMUFP_LN, ASI_SRMMUFP, 0);
}

#if defined(MULTIPROCESSOR)
/*
 * The SMP versions of the tlb flush routines.  We only need to
 * do a cross call for these on sun4m (Mbus) systems. sun4d systems
 * have an Xbus which broadcasts TLB demaps in hardware.
 */

static inline void	smp_tlb_flush_page (int va, int ctx, u_int cpuset);
static inline void	smp_tlb_flush_segment (int va, int ctx, u_int cpuset);
static inline void	smp_tlb_flush_region (int va, int ctx, u_int cpuset);
static inline void	smp_tlb_flush_context (int ctx, u_int cpuset);
static inline void	smp_tlb_flush_all (void);

/* From locore: */
extern void ft_tlb_flush(int va, int ctx, int lvl);

static inline void
smp_tlb_flush_page(int va, int ctx, u_int cpuset)
{

	if (CPU_ISSUN4D) {
		sp_tlb_flush(va, ctx, ASI_SRMMUFP_L3);
	} else
		FXCALL3(sp_tlb_flush, ft_tlb_flush, va, ctx, ASI_SRMMUFP_L3, cpuset);
}

static inline void
smp_tlb_flush_segment(int va, int ctx, u_int cpuset)
{

	if (CPU_ISSUN4D) {
		sp_tlb_flush(va, ctx, ASI_SRMMUFP_L2);
	} else
		FXCALL3(sp_tlb_flush, ft_tlb_flush, va, ctx, ASI_SRMMUFP_L2, cpuset);
}

static inline void
smp_tlb_flush_region(int va, int ctx, u_int cpuset)
{

	if (CPU_ISSUN4D) {
		sp_tlb_flush(va, ctx, ASI_SRMMUFP_L1);
	} else
		FXCALL3(sp_tlb_flush, ft_tlb_flush, va, ctx, ASI_SRMMUFP_L1, cpuset);
}

static inline void
smp_tlb_flush_context(int ctx, u_int cpuset)
{

	if (CPU_ISSUN4D) {
		sp_tlb_flush(ctx, 0, ASI_SRMMUFP_L0);
	} else
		FXCALL3(sp_tlb_flush, ft_tlb_flush, 0, ctx, ASI_SRMMUFP_L0, cpuset);
}

static inline void
smp_tlb_flush_all(void)
{

	if (CPU_ISSUN4D) {
		sp_tlb_flush_all();
	} else
		XCALL0(sp_tlb_flush_all, CPUSET_ALL);
}
#endif /* MULTIPROCESSOR */

#if defined(MULTIPROCESSOR)
#define tlb_flush_page(va,ctx,s)	smp_tlb_flush_page(va,ctx,s)
#define tlb_flush_segment(va,ctx,s)	smp_tlb_flush_segment(va,ctx,s)
#define tlb_flush_region(va,ctx,s)	smp_tlb_flush_region(va,ctx,s)
#define tlb_flush_context(ctx,s)	smp_tlb_flush_context(ctx,s)
#define tlb_flush_all()			smp_tlb_flush_all()
#else
#define tlb_flush_page(va,ctx,s)	sp_tlb_flush(va,ctx,ASI_SRMMUFP_L3)
#define tlb_flush_segment(va,ctx,s)	sp_tlb_flush(va,ctx,ASI_SRMMUFP_L2)
#define tlb_flush_region(va,ctx,s)	sp_tlb_flush(va,ctx,ASI_SRMMUFP_L1)
#define tlb_flush_context(ctx,s)	sp_tlb_flush(ctx,0,ASI_SRMMUFP_L0)
#define tlb_flush_all()			sp_tlb_flush_all()
#endif /* MULTIPROCESSOR */

static u_int	VA2PA(void *);
static u_long	srmmu_bypass_read(u_long);

/*
 * VA2PA(addr) -- converts a virtual address to a physical address using
 * the MMU's currently-installed page tables. As a side effect, the address
 * translation used may cause the associated pte to be encached. The correct
 * context for VA must be set before this is called.
 *
 * This routine should work with any level of mapping, as it is used
 * during bootup to interact with the ROM's initial L1 mapping of the kernel.
 */
static u_int
VA2PA(void *addr)
{
	u_int pte;

	/*
	 * We'll use that handy SRMMU flush/probe.
	 * Try each level in turn until we find a valid pte. Otherwise panic.
	 */

	pte = lda(((u_int)addr & ~0xfff) | ASI_SRMMUFP_L3, ASI_SRMMUFP);
	/* Unlock fault status; required on Hypersparc modules */
	(void)lda(SRMMU_SFSR, ASI_SRMMU);
	if ((pte & SRMMU_TETYPE) == SRMMU_TEPTE)
	    return (((pte & SRMMU_PPNMASK) << SRMMU_PPNPASHIFT) |
		    ((u_int)addr & 0xfff));

	/* A `TLB Flush Entire' is required before any L0, L1 or L2 probe */
	tlb_flush_all_real();

	pte = lda(((u_int)addr & ~0xfff) | ASI_SRMMUFP_L2, ASI_SRMMUFP);
	if ((pte & SRMMU_TETYPE) == SRMMU_TEPTE)
	    return (((pte & SRMMU_PPNMASK) << SRMMU_PPNPASHIFT) |
		    ((u_int)addr & 0x3ffff));
	pte = lda(((u_int)addr & ~0xfff) | ASI_SRMMUFP_L1, ASI_SRMMUFP);
	if ((pte & SRMMU_TETYPE) == SRMMU_TEPTE)
	    return (((pte & SRMMU_PPNMASK) << SRMMU_PPNPASHIFT) |
		    ((u_int)addr & 0xffffff));
	pte = lda(((u_int)addr & ~0xfff) | ASI_SRMMUFP_L0, ASI_SRMMUFP);
	if ((pte & SRMMU_TETYPE) == SRMMU_TEPTE)
	    return (((pte & SRMMU_PPNMASK) << SRMMU_PPNPASHIFT) |
		    ((u_int)addr & 0xffffffff));

#ifdef DIAGNOSTIC
	panic("VA2PA: Asked to translate unmapped VA %p", addr);
#else
	return (0);
#endif
}

/*
 * Atomically update a PTE entry, coping with hardware updating the
 * PTE at the same time we are.  This is the procedure that is
 * recommended in the SuperSPARC user's manual.
 */
int
updatepte4m(vaddr_t va, int *pte, int bic, int bis, int ctx, u_int cpuset)
{
	int oldval, swapval;
	volatile int *vpte = (volatile int *)pte;

	/*
	 * Can only be one of these happening in the system
	 * at any one time.
	 */
	mutex_spin_enter(&demap_lock);

	/*
	 * The idea is to loop swapping zero into the pte, flushing
	 * it, and repeating until it stays zero.  At this point,
	 * there should be no more hardware accesses to this PTE
	 * so we can modify it without losing any mod/ref info.
	 */
	oldval = 0;
	do {
		swapval = 0;
		swap(vpte, swapval);
		tlb_flush_page(va, ctx, cpuset);
		oldval |= swapval;
	} while (__predict_false(*vpte != 0));

	swapval = (oldval & ~bic) | bis;
	swap(vpte, swapval);

	mutex_spin_exit(&demap_lock);

	return (oldval);
}

inline void
setpgt4m(int *ptep, int pte)
{

	swap(ptep, pte);
}

inline void
setpgt4m_va(vaddr_t va, int *ptep, int pte, int pageflush, int ctx,
	    u_int cpuset)
{

#if defined(MULTIPROCESSOR)
	updatepte4m(va, ptep, 0xffffffff, pte, pageflush ? ctx : 0, cpuset);
#else
	if (__predict_true(pageflush))
		tlb_flush_page(va, ctx, 0);
	setpgt4m(ptep, pte);
#endif /* MULTIPROCESSOR */
}

/* Set the page table entry for va to pte. */
void
setpte4m(vaddr_t va, int pte)
{
	struct pmap *pm;
	struct regmap *rp;
	struct segmap *sp;

#ifdef DEBUG
	if (getcontext4m() != 0)
		panic("setpte4m: user context");
#endif

	pm = pmap_kernel();
	rp = &pm->pm_regmap[VA_VREG(va)];
	sp = &rp->rg_segmap[VA_VSEG(va)];

	tlb_flush_page(va, 0, CPUSET_ALL);
	setpgt4m(sp->sg_pte + VA_SUN4M_VPG(va), pte);
}

/*
 * Page table pool back-end.
 */
void *
pgt_page_alloc(struct pool *pp, int flags)
{
	int cacheit = (cpuinfo.flags & CPUFLG_CACHEPAGETABLES) != 0;
	struct vm_page *pg;
	vaddr_t va;
	paddr_t pa;

	/* Allocate a page of physical memory */
	if ((pg = uvm_pagealloc(NULL, 0, NULL, 0)) == NULL)
		return (NULL);

	/* Allocate virtual memory */
	va = uvm_km_alloc(kmem_map, PAGE_SIZE, 0, UVM_KMF_VAONLY |
		((flags & PR_WAITOK) ? 0 : UVM_KMF_NOWAIT | UVM_KMF_TRYLOCK));
	if (va == 0) {
		uvm_pagefree(pg);
		return (NULL);
	}

	/*
	 * On systems with a physical data cache we need to flush this page
	 * from the cache if the pagetables cannot be cached.
	 * On systems with a virtually indexed data cache, we only need
	 * to map it non-cacheable, since the page is not currently mapped.
	 */
	pa = VM_PAGE_TO_PHYS(pg);
	if (cacheit == 0)
		pcache_flush_page(pa, 1);

	/* Map the page */
	pmap_kenter_pa(va, pa | (cacheit ? 0 : PMAP_NC),
	    VM_PROT_READ | VM_PROT_WRITE);
	pmap_update(pmap_kernel());

	return ((void *)va);
}

void
pgt_page_free(struct pool *pp, void *v)
{
	vaddr_t va;
	paddr_t pa;
	bool rv;

	va = (vaddr_t)v;
	rv = pmap_extract(pmap_kernel(), va, &pa);
	KASSERT(rv);
	uvm_pagefree(PHYS_TO_VM_PAGE(pa));
	pmap_kremove(va, PAGE_SIZE);
	uvm_km_free(kmem_map, va, PAGE_SIZE, UVM_KMF_VAONLY);
}
#endif /* SUN4M || SUN4D */

/*----------------------------------------------------------------*/

/*
 * The following three macros are to be used in sun4/sun4c code only.
 */
#if defined(SUN4_MMU3L)
#define CTX_USABLE(pm,rp) (					\
		((pm)->pm_ctx != NULL &&			\
		 (!HASSUN4_MMU3L || (rp)->rg_smeg != reginval))	\
)
#else
#define CTX_USABLE(pm,rp)	((pm)->pm_ctx != NULL )
#endif

#define GAP_WIDEN(pm,vr) do if (CPU_HAS_SUNMMU) {		\
	if (vr + 1 == pm->pm_gap_start)				\
		pm->pm_gap_start = vr;				\
	if (vr == pm->pm_gap_end)				\
		pm->pm_gap_end = vr + 1;			\
} while (0)

#define GAP_SHRINK(pm,vr) do if (CPU_HAS_SUNMMU) {			\
	int x;								\
	x = pm->pm_gap_start + (pm->pm_gap_end - pm->pm_gap_start) / 2;	\
	if (vr > x) {							\
		if (vr < pm->pm_gap_end)				\
			pm->pm_gap_end = vr;				\
	} else {							\
		if (vr >= pm->pm_gap_start && x != pm->pm_gap_start)	\
			pm->pm_gap_start = vr + 1;			\
	}								\
} while (0)


static void get_phys_mem(void **);
#if 0 /* not used */
void	kvm_iocache(char *, int);
#endif

#ifdef DEBUG
void	pm_check(char *, struct pmap *);
void	pm_check_k(char *, struct pmap *);
void	pm_check_u(char *, struct pmap *);
#endif

/*
 * During the PMAP bootstrap, we can use a simple translation to map a
 * kernel virtual address to a psysical memory address (this is arranged
 * in locore).  Usually, KERNBASE maps to physical address 0. This is always
 * the case on sun4 and sun4c machines. On sun4m machines -- if no memory is
 * installed in the bank corresponding to physical address 0 -- the PROM may
 * elect to load us at some other address, presumably at the start of
 * the first memory bank that is available. We set the up the variable
 * `va2pa_offset' to hold the physical address corresponding to KERNBASE.
 */

static u_long va2pa_offset;
#define PMAP_BOOTSTRAP_VA2PA(v) ((paddr_t)((u_long)(v) - va2pa_offset))
#define PMAP_BOOTSTRAP_PA2VA(p) ((vaddr_t)((u_long)(p) + va2pa_offset))

/*
 * Grab physical memory list.
 * While here, compute `physmem'.
 */
void
get_phys_mem(void **top)
{
	struct memarr *mp;
	char *p;
	int i;

	/* Load the memory descriptor array at the current kernel top */
	p = (void *)ALIGN(*top);
	pmemarr = (struct memarr *)p;
	npmemarr = prom_makememarr(pmemarr, 1000, MEMARR_AVAILPHYS);

	/* Update kernel top */
	p += npmemarr * sizeof(struct memarr);
	*top = p;

	for (physmem = 0, mp = pmemarr, i = npmemarr; --i >= 0; mp++)
		physmem += btoc(mp->len);
}


/*
 * Support functions for vm_page_bootstrap().
 */

/*
 * How much virtual space does this kernel have?
 * (After mapping kernel text, data, etc.)
 */
void
pmap_virtual_space(vaddr_t *v_start, vaddr_t *v_end)
{

        *v_start = virtual_avail;
        *v_end   = virtual_end;
}

#ifdef PMAP_GROWKERNEL
vaddr_t
pmap_growkernel(vaddr_t eva)
{
	struct regmap *rp;
	struct segmap *sp;
	int vr, evr, M, N, i;
	struct vm_page *pg;
	vaddr_t va;

	if (eva <= virtual_end)
		return (virtual_end);

	/* For now, only implemented for sun4/sun4c */
	KASSERT(CPU_HAS_SUNMMU);

	/*
	 * Map in the next region(s)
	 */

	/* Get current end-of-kernel */
	vr = virtual_end >> RGSHIFT;
	evr = (eva + NBPRG - 1) >> RGSHIFT;
	eva = evr << RGSHIFT;

	if (eva > VM_MAX_KERNEL_ADDRESS)
		panic("growkernel: grown too large: %lx", eva);

	/*
	 * Divide a region in N blocks of M segments, where each segment
	 * block can have its PTEs mapped by one page.
	 * N should come out to 1 for 8K pages and to 4 for 4K pages.
	 */
	M = NBPG / (NPTESG * sizeof(int));
	N = (NBPRG/NBPSG) / M;

	while (vr < evr) {
		rp = &pmap_kernel()->pm_regmap[vr];
		for (i = 0; i < N; i++) {
			sp = &rp->rg_segmap[i * M];
			va = (vaddr_t)sp->sg_pte;
			pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE);
			if (pg == NULL)
				panic("growkernel: out of memory");
			pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
					VM_PROT_READ | VM_PROT_WRITE);
		}
	}

	virtual_end = eva;
	return (eva);
}
#endif

/*
 * Helper routine that hands off available physical pages to the VM system.
 */
static void
pmap_page_upload(void)
{
	int	n;
	paddr_t	start, end;

	/* First, the `etext gap' */
	start = PMAP_BOOTSTRAP_VA2PA(etext_gap_start);
	end = PMAP_BOOTSTRAP_VA2PA(etext_gap_end);

#ifdef DIAGNOSTIC
	if (avail_start <= start)
		panic("pmap_page_upload: etext gap overlap: %lx < %lx",
			(u_long)avail_start, (u_long)start);
#endif
	if (etext_gap_start < etext_gap_end) {
		vm_first_phys = start;
		uvm_page_physload(
			atop(start),
			atop(end),
			atop(start),
			atop(end), VM_FREELIST_DEFAULT);
	}

	for (n = 0; n < npmemarr; n++) {

		start = pmemarr[n].addr;
		end = start + pmemarr[n].len;

		/* Update vm_{first_last}_phys */
		if (vm_first_phys > start)
			vm_first_phys = start;
		if (vm_last_phys < end)
			vm_last_phys = end;

		/*
		 * Exclude any memory allocated for the kernel as computed
		 * by pmap_bootstrap(), i.e. the range
		 *	[KERNBASE_PA, avail_start>.
		 * Note that this will also exclude the `etext gap' range
		 * already uploaded above.
		 */
		if (start < PMAP_BOOTSTRAP_VA2PA(KERNBASE)) {
			/*
			 * This segment starts below the kernel load address.
			 * Chop it off at the start of the kernel.
			 */
			paddr_t	chop = PMAP_BOOTSTRAP_VA2PA(KERNBASE);

			if (end < chop)
				chop = end;
#ifdef DEBUG
			prom_printf("bootstrap gap: start %lx, chop %lx, end %lx\n",
				start, chop, end);
#endif
			uvm_page_physload(
				atop(start),
				atop(chop),
				atop(start),
				atop(chop),
				VM_FREELIST_DEFAULT);

			/*
			 * Adjust the start address to reflect the
			 * uploaded portion of this segment.
			 */
			start = chop;
		}

		/* Skip the current kernel address range */
		if (start <= avail_start && avail_start < end)
			start = avail_start;

		if (start == end)
			continue;

		/* Upload (the rest of) this segment */
		uvm_page_physload(
			atop(start),
			atop(end),
			atop(start),
			atop(end), VM_FREELIST_DEFAULT);
	}
}

/*
 * This routine is used by mmrw() to validate access to `/dev/mem'.
 */
int
pmap_pa_exists(paddr_t pa)
{
	int nmem;
	struct memarr *mp;

	for (mp = pmemarr, nmem = npmemarr; --nmem >= 0; mp++) {
		if (pa >= mp->addr && pa < mp->addr + mp->len)
			return 1;
	}

	return 0;
}

/* update pv_flags given a valid pte */
#define	MR4_4C(pte) (((pte) >> PG_M_SHIFT) & (PV_MOD | PV_REF))
#define MR4M(pte) (((pte) >> PG_M_SHIFT4M) & (PV_MOD4M | PV_REF4M))

/*----------------------------------------------------------------*/

/*
 * Agree with the monitor ROM as to how many MMU entries are
 * to be reserved, and map all of its segments into all contexts.
 *
 * Unfortunately, while the Version 0 PROM had a nice linked list of
 * taken virtual memory, the Version 2 PROM provides instead a convoluted
 * description of *free* virtual memory.  Rather than invert this, we
 * resort to two magic constants from the PROM vector description file.
 */
#if defined(SUN4) || defined(SUN4C)
void
mmu_reservemon4_4c(int *nrp, int *nsp)
{
	u_int va = 0, eva = 0;
	int mmuseg, i, nr, ns, vr, lastvr;
	int *pte;
#if defined(SUN4_MMU3L)
	int mmureg;
#endif
	struct regmap *rp;

#if defined(SUN4)
	if (CPU_ISSUN4) {
		prom_vstart = va = OLDMON_STARTVADDR;
		prom_vend = eva = OLDMON_ENDVADDR;
	}
#endif
#if defined(SUN4C)
	if (CPU_ISSUN4C) {
		prom_vstart = va = OPENPROM_STARTVADDR;
		prom_vend = eva = OPENPROM_ENDVADDR;
	}
#endif
	ns = *nsp;
	nr = *nrp;
	lastvr = 0;
	while (va < eva) {
		vr = VA_VREG(va);
		rp = &pmap_kernel()->pm_regmap[vr];

#if defined(SUN4_MMU3L)
		if (HASSUN4_MMU3L && vr != lastvr) {
			lastvr = vr;
			mmureg = getregmap(va);
			if (mmureg < nr)
				rp->rg_smeg = nr = mmureg;
			/*
			 * On 3-level MMU machines, we distribute regions,
			 * rather than segments, amongst the contexts.
			 */
			for (i = ncontext; --i > 0;)
				prom_setcontext(i, (void *)va, mmureg);
		}
#endif
		mmuseg = getsegmap(va);
		if (mmuseg < ns)
			ns = mmuseg;

		if (!HASSUN4_MMU3L)
			for (i = ncontext; --i > 0;)
				prom_setcontext(i, (void *)va, mmuseg);

		if (mmuseg == seginval) {
			va += NBPSG;
			continue;
		}
		/*
		 * Another PROM segment. Enter into region map.
		 * Assume the entire segment is valid.
		 */
		rp->rg_nsegmap += 1;
		rp->rg_segmap[VA_VSEG(va)].sg_pmeg = mmuseg;
		rp->rg_segmap[VA_VSEG(va)].sg_npte = NPTESG;
		pte = rp->rg_segmap[VA_VSEG(va)].sg_pte;

		/* PROM maps its memory user-accessible: fix it. */
		for (i = NPTESG; --i >= 0; va += NBPG, pte++) {
			*pte = getpte4(va) | PG_S;
			setpte4(va, *pte);
		}
	}
	*nsp = ns;
	*nrp = nr;
	return;
}
#endif

#if defined(SUN4M) || defined(SUN4D) /* SRMMU versions of above */

u_long
srmmu_bypass_read(u_long paddr)
{
	unsigned long v;

	if (cpuinfo.mxcc) {
		/*
		 * We're going to have to use MMU passthrough. If we're on
		 * a Viking SuperSPARC with a MultiCache Controller, we
		 * need to set the AC (Alternate Cacheable) bit in the MMU's
		 * control register in order to not by-pass the cache.
		 */

		unsigned long s = lda(SRMMU_PCR, ASI_SRMMU);

		/* set MMU AC bit */
		sta(SRMMU_PCR, ASI_SRMMU, s | VIKING_PCR_AC);
		v = lda(paddr, ASI_BYPASS);
		sta(SRMMU_PCR, ASI_SRMMU, s);
	} else
		v = lda(paddr, ASI_BYPASS);

	return (v);
}


/*
 * Take the monitor's initial page table layout, convert it to 3rd-level pte's
 * (it starts out as a L1 mapping), and install it along with a set of kernel
 * mapping tables as the kernel's initial page table setup. Also create and
 * enable a context table. I suppose we also want to block user-mode access
 * to the new kernel/ROM mappings.
 */

/*
 * mmu_reservemon4m(): Copies the existing (ROM) page tables to kernel space,
 * converting any L1/L2 PTEs to L3 PTEs. Does *not* copy the L1 entry mapping
 * the kernel at KERNBASE since we don't want to map 16M of physical
 * memory for the kernel. Thus the kernel must be installed later!
 * Also installs ROM mappings into the kernel pmap.
 * NOTE: This also revokes all user-mode access to the mapped regions.
 */
void
mmu_reservemon4m(struct pmap *kpmap)
{
	unsigned int rom_ctxtbl;
	int te;

#if !(defined(PROM_AT_F0) || defined(MSIIEP))
	prom_vstart = OPENPROM_STARTVADDR;
	prom_vend = OPENPROM_ENDVADDR;
#else /* OBP3/OFW in JavaStations */
	prom_vstart = 0xf0000000;
#if defined(MSIIEP)
	prom_vend = 0xf0800000;
#else
	prom_vend = 0xf0080000;
#endif
#endif

	/*
	 * XXX: although the sun4m can handle 36 bits of physical
	 * address space, we assume that all these page tables, etc
	 * are in the lower 4G (32-bits) of address space, i.e. out of I/O
	 * space. Eventually this should be changed to support the 36 bit
	 * physical addressing, in case some crazed ROM designer decides to
	 * stick the pagetables up there. In that case, we should use MMU
	 * transparent mode, (i.e. ASI 0x20 to 0x2f) to access
	 * physical memory.
	 */

	rom_ctxtbl = (lda(SRMMU_CXTPTR,ASI_SRMMU) << SRMMU_PPNPASHIFT);

	te = srmmu_bypass_read(rom_ctxtbl);	/* i.e. context 0 */

	switch (te & SRMMU_TETYPE) {
	case SRMMU_TEINVALID:
		cpuinfo.ctx_tbl[0] = SRMMU_TEINVALID;
		panic("mmu_reservemon4m: no existing L0 mapping! "
		      "(How are we running?");
		break;
	case SRMMU_TEPTE:
		panic("mmu_reservemon4m: can't handle ROM 4G page size");
		/* XXX: Should make this work, however stupid it is */
		break;
	case SRMMU_TEPTD:
		mmu_setup4m_L1(te, kpmap);
		break;
	default:
		panic("mmu_reservemon4m: unknown pagetable entry type");
	}
}

/* regtblptd - PTD for region table to be remapped */
void
mmu_setup4m_L1(int regtblptd, struct pmap *kpmap)
{
	unsigned int regtblrover;
	int i;
	unsigned int te;
	struct regmap *rp;
	int j, k;

	/*
	 * Here we scan the region table to copy any entries which appear.
	 * We are only concerned with regions in kernel space and above
	 * (i.e. regions VA_VREG(KERNBASE)+1 to 0xff). We ignore the first
	 * region (at VA_VREG(KERNBASE)), since that is the 16MB L1 mapping
	 * that the ROM used to map the kernel in initially. Later, we will
	 * rebuild a new L3 mapping for the kernel and install it before
	 * switching to the new pagetables.
	 */
	regtblrover =
		((regtblptd & ~SRMMU_TETYPE) << SRMMU_PPNPASHIFT) +
		(VA_VREG(KERNBASE)+1) * sizeof(long);	/* kernel only */

	for (i = VA_VREG(KERNBASE) + 1; i < SRMMU_L1SIZE;
	     i++, regtblrover += sizeof(long)) {

		/* The region we're dealing with */
		rp = &kpmap->pm_regmap[i];

		te = srmmu_bypass_read(regtblrover);
		switch(te & SRMMU_TETYPE) {
		case SRMMU_TEINVALID:
			break;

		case SRMMU_TEPTE:
#ifdef DEBUG
			prom_printf("mmu_setup4m_L1: "
			       "converting region 0x%x from L1->L3\n", i);
#endif
			/*
			 * This region entry covers 64MB of memory -- or
			 * (NSEGRG * NPTESG) pages -- which we must convert
			 * into a 3-level description.
			 */

			for (j = 0; j < SRMMU_L2SIZE; j++) {
				struct segmap *sp = &rp->rg_segmap[j];

				for (k = 0; k < SRMMU_L3SIZE; k++) {
					setpgt4m(&sp->sg_pte[k],
						(te & SRMMU_L1PPNMASK) |
						(j << SRMMU_L2PPNSHFT) |
						(k << SRMMU_L3PPNSHFT) |
						(te & SRMMU_PGBITSMSK) |
						((te & SRMMU_PROT_MASK) |
						 PPROT_U2S_OMASK) |
						SRMMU_TEPTE);
				}
			}
			break;

		case SRMMU_TEPTD:
			mmu_setup4m_L2(te, rp);
			break;

		default:
			panic("mmu_setup4m_L1: unknown pagetable entry type");
		}
	}
}

void
mmu_setup4m_L2(int segtblptd, struct regmap *rp)
{
	unsigned int segtblrover;
	int i, k;
	unsigned int te;
	struct segmap *sp;

	segtblrover = (segtblptd & ~SRMMU_TETYPE) << SRMMU_PPNPASHIFT;
	for (i = 0; i < SRMMU_L2SIZE; i++, segtblrover += sizeof(long)) {

		sp = &rp->rg_segmap[i];

		te = srmmu_bypass_read(segtblrover);
		switch(te & SRMMU_TETYPE) {
		case SRMMU_TEINVALID:
			break;

		case SRMMU_TEPTE:
#ifdef DEBUG
			prom_printf("mmu_setup4m_L2: converting L2 entry at segment 0x%x to L3\n",i);
#endif
			/*
			 * This segment entry covers 256KB of memory -- or
			 * (NPTESG) pages -- which we must convert
			 * into a 3-level description.
			 */
			for (k = 0; k < SRMMU_L3SIZE; k++) {
				setpgt4m(&sp->sg_pte[k],
					(te & SRMMU_L1PPNMASK) |
					(te & SRMMU_L2PPNMASK) |
					(k << SRMMU_L3PPNSHFT) |
					(te & SRMMU_PGBITSMSK) |
					((te & SRMMU_PROT_MASK) |
					 PPROT_U2S_OMASK) |
					SRMMU_TEPTE);
			}
			break;

		case SRMMU_TEPTD:
			mmu_setup4m_L3(te, sp);
			break;

		default:
			panic("mmu_setup4m_L2: unknown pagetable entry type");
		}
	}
}

void
mmu_setup4m_L3(int pagtblptd, struct segmap *sp)
{
	unsigned int pagtblrover;
	int i;
	unsigned int te;

	pagtblrover = (pagtblptd & ~SRMMU_TETYPE) << SRMMU_PPNPASHIFT;
	for (i = 0; i < SRMMU_L3SIZE; i++, pagtblrover += sizeof(long)) {
		te = srmmu_bypass_read(pagtblrover);
		switch(te & SRMMU_TETYPE) {
		case SRMMU_TEINVALID:
			break;
		case SRMMU_TEPTE:
			setpgt4m(&sp->sg_pte[i], te | PPROT_U2S_OMASK);
			pmap_kernel()->pm_stats.resident_count++;
			break;
		case SRMMU_TEPTD:
			panic("mmu_setup4m_L3: PTD found in L3 page table");
		default:
			panic("mmu_setup4m_L3: unknown pagetable entry type");
		}
	}
}
#endif /* defined SUN4M || defined SUN4D */

/*----------------------------------------------------------------*/

#if defined(SUN4) || defined(SUN4C)
/*
 * MMU management.
 */
static int	me_alloc(struct mmuq *, struct pmap *, int, int);
static void	me_free(struct pmap *, u_int);
#if defined(SUN4_MMU3L)
static int	region_alloc(struct mmuq *, struct pmap *, int);
static void	region_free(struct pmap *, u_int);
#endif


/*
 * Allocate an MMU entry (i.e., a PMEG).
 * If necessary, steal one from someone else.
 * Put it on the tail of the given queue
 * (which is either the LRU list or the locked list).
 * The locked list is not actually ordered, but this is easiest.
 * Also put it on the given (new) pmap's chain,
 * enter its pmeg number into that pmap's segmap,
 * and store the pmeg's new virtual segment number (me->me_vseg).
 *
 * This routine is large and complicated, but it must be fast
 * since it implements the dynamic allocation of MMU entries.
 */

static inline int
me_alloc(struct mmuq *mh, struct pmap *newpm, int newvreg, int newvseg)
{
	struct mmuentry *me;
	struct pmap *pm;
	int i, va, *ptep, pte;
	int ctx;
	struct regmap *rp;
	struct segmap *sp;

	/* try free list first */
	if (!MMUQ_EMPTY(&segm_freelist)) {
		me = MMUQ_FIRST(&segm_freelist);
		MMUQ_REMOVE(me, me_list);
#ifdef DEBUG
		if (me->me_pmap != NULL)
			panic("me_alloc: freelist entry has pmap");
		if (pmapdebug & PDB_MMU_ALLOC)
			printf("me_alloc: got pmeg %d\n", me->me_cookie);
#endif
		MMUQ_INSERT_TAIL(mh, me, me_list);

		/* onto on pmap chain; pmap is already locked, if needed */
		TAILQ_INSERT_TAIL(&newpm->pm_seglist, me, me_pmchain);
#ifdef DIAGNOSTIC
		pmap_stats.ps_npmeg_free--;
		if (mh == &segm_locked) {
			pmap_stats.ps_npmeg_locked++;
			me->me_statp = &pmap_stats.ps_npmeg_locked;
		} else {
			pmap_stats.ps_npmeg_lru++;
			me->me_statp = &pmap_stats.ps_npmeg_lru;
		}
#endif

		/* into pmap segment table, with backpointers */
		me->me_pmap = newpm;
		me->me_vseg = newvseg;
		me->me_vreg = newvreg;

		return (me->me_cookie);
	}

	/* no luck, take head of LRU list */
	if (MMUQ_EMPTY(&segm_lru))
		panic("me_alloc: all pmegs gone");

	me = MMUQ_FIRST(&segm_lru);
	pm = me->me_pmap;
#ifdef DEBUG
	if (pmapdebug & (PDB_MMU_ALLOC | PDB_MMU_STEAL))
		printf("me_alloc: stealing pmeg 0x%x from pmap %p\n",
		    me->me_cookie, pm);
#endif

	mmu_stolenpmegs_evcnt.ev_count++;

	/*
	 * Remove from LRU list, and insert at end of new list
	 * (probably the LRU list again, but so what?).
	 */
	MMUQ_REMOVE(me, me_list);
	MMUQ_INSERT_TAIL(mh, me, me_list);

#ifdef DIAGNOSTIC
	if (mh == &segm_locked) {
		pmap_stats.ps_npmeg_lru--;
		pmap_stats.ps_npmeg_locked++;
		me->me_statp = &pmap_stats.ps_npmeg_locked;
	} else {
		me->me_statp = &pmap_stats.ps_npmeg_lru;
	}
#endif

	rp = &pm->pm_regmap[me->me_vreg];
	sp = &rp->rg_segmap[me->me_vseg];
	ptep = sp->sg_pte;

#ifdef DEBUG
	if (sp->sg_pmeg != me->me_cookie)
		panic("me_alloc: wrong sg_pmeg (%d != %d)",
				sp->sg_pmeg, me->me_cookie);
#endif

	/*
	 * The PMEG must be mapped into some context so that we can
	 * read its PTEs.  Use its current context if it has one;
	 * if not, and since context 0 is reserved for the kernel,
	 * the simplest method is to switch to 0 and map the PMEG
	 * to virtual address 0---which, being a user space address,
	 * is by definition not in use.
	 *
	 * XXX do not have to flush cache immediately
	 */
	ctx = getcontext4();

	/*
	 * Even if we're stealing a PMEG from ourselves (i.e. if pm==newpm),
	 * we must make sure there are no user register windows in the CPU
	 * for the following reasons:
	 * (1) if we have a write-allocate cache and the segment we are
	 *     stealing contains stack pages, an interrupt during the
	 *     interval that starts at cache_flush_segment() below and ends
	 *     when the segment is finally removed from the MMU, may cause
	 *     dirty cache lines to reappear.
	 * (2) when re-wiring this PMEG for use by another segment (e.g.
	 *     in mmu_pagein()) a window exists where the PTEs in this PMEG
	 *     point at arbitrary pages allocated to this address space.
	 *     Again, a register window flush at this point is likely to
	 *     cause data corruption in case the segment being rewired
	 *     contains stack virtual addresses.
	 */
	write_user_windows();
	if (CTX_USABLE(pm,rp)) {
		setcontext4(pm->pm_ctxnum);
		va = VSTOVA(me->me_vreg, me->me_vseg);
#ifdef DEBUG
		if (getsegmap(va) != me->me_cookie)
			panic("me_alloc: wrong pmeg in MMU (%d != %d)",
				getsegmap(va), me->me_cookie);
#endif
		cache_flush_segment(me->me_vreg, me->me_vseg, pm->pm_ctxnum);
	} else {
		va = 0;
		setcontext4(0);
		if (HASSUN4_MMU3L)
			setregmap(va, tregion);
		setsegmap(va, me->me_cookie);
		/*
		 * No cache flush needed: it happened earlier when
		 * the old context was taken.
		 */
	}

	/*
	 * Record reference and modify bits for each page,
	 * and copy PTEs into kernel memory so that they can
	 * be reloaded later.
	 */
	i = NPTESG;
	do {
		int swbits = *ptep & PG_MBZ;
		pte = getpte4(va);
		if ((pte & (PG_V | PG_TYPE)) == (PG_V | PG_OBMEM)) {
			struct vm_page *pg;
			if ((pg = pvhead4_4c(pte)) != NULL)
				VM_MDPAGE_PVHEAD(pg)->pv_flags |= MR4_4C(pte);
		}
		*ptep++ = swbits | (pte & ~(PG_U|PG_M));
		va += NBPG;
	} while (--i > 0);

	/* update segment tables */
	if (CTX_USABLE(pm,rp)) {
		va = VSTOVA(me->me_vreg,me->me_vseg);
		if (pm != pmap_kernel() || HASSUN4_MMU3L)
			setsegmap(va, seginval);
		else {
			/* Unmap segment from all contexts */
			for (i = ncontext; --i >= 0;) {
				setcontext4(i);
				setsegmap(va, seginval);
			}
		}
	}
	sp->sg_pmeg = seginval;

	/* off old pmap chain */
	TAILQ_REMOVE(&pm->pm_seglist, me, me_pmchain);
	setcontext4(ctx);

	/* onto new pmap chain; new pmap is already locked, if needed */
	TAILQ_INSERT_TAIL(&newpm->pm_seglist, me, me_pmchain);

	/* into new segment table, with backpointers */
	me->me_pmap = newpm;
	me->me_vseg = newvseg;
	me->me_vreg = newvreg;

	return (me->me_cookie);
}

/*
 * Free an MMU entry.
 *
 * Assumes the corresponding pmap is already locked.
 * Caller must update hardware.
 */
static inline void
me_free(struct pmap *pm, u_int pmeg)
{
	struct mmuentry *me = &mmusegments[pmeg];
#ifdef DEBUG
	struct regmap *rp;
	int i, va, tpte, ctx;
#endif

#ifdef DEBUG
	rp = &pm->pm_regmap[me->me_vreg];
	if (pmapdebug & PDB_MMU_ALLOC)
		printf("me_free: freeing pmeg %d from pmap %p\n",
		    me->me_cookie, pm);
	if (me->me_cookie != pmeg)
		panic("me_free: wrong mmuentry");
	if (pm != me->me_pmap)
		panic("me_free: pm != me_pmap");
	if (rp->rg_segmap[me->me_vseg].sg_pmeg != pmeg &&
	    rp->rg_segmap[me->me_vseg].sg_pmeg != seginval)
		panic("me_free: wrong sg_pmeg (%d != %d)",
			rp->rg_segmap[me->me_vseg].sg_pmeg, pmeg);

	/* check for spurious mappings (using temp. mapping in context 0) */
	ctx = getcontext4();
	setcontext4(0);
	if (HASSUN4_MMU3L)
		setregmap(0, tregion);
	setsegmap(0, me->me_cookie);
	va = 0;
	i = NPTESG;
	do {
		tpte = getpte4(va);
		if ((tpte & PG_V) == PG_V)
			panic("me_free: segment not clean (pte=%x)", tpte);
		va += NBPG;
	} while (--i > 0);
	setcontext4(ctx);
#endif /* DEBUG */

	/* take mmu entry off pmap chain */
	TAILQ_REMOVE(&pm->pm_seglist, me, me_pmchain);

	/* off LRU or lock chain */
	MMUQ_REMOVE(me, me_list);
#ifdef DIAGNOSTIC
	if (me->me_statp == NULL)
		panic("me_statp");
	(*me->me_statp)--;
	me->me_statp = NULL;
#endif

	/* no associated pmap; on free list */
	me->me_pmap = NULL;
	MMUQ_INSERT_TAIL(&segm_freelist, me, me_list);
#ifdef DIAGNOSTIC
	pmap_stats.ps_npmeg_free++;
#endif
}

#if defined(SUN4_MMU3L)

/* XXX - Merge with segm_alloc/segm_free ? */

int
region_alloc(struct mmuq *mh, struct pmap *newpm, int newvr)
{
	struct mmuentry *me;
	struct pmap *pm;
	int ctx;
	struct regmap *rp;

	/* try free list first */
	if (!MMUQ_EMPTY(&region_freelist)) {
		me = MMUQ_FIRST(&region_freelist);
		MMUQ_REMOVE(me, me_list);
#ifdef DEBUG
		if (me->me_pmap != NULL)
			panic("region_alloc: freelist entry has pmap");
		if (pmapdebug & PDB_MMUREG_ALLOC)
			printf("region_alloc: got smeg 0x%x\n", me->me_cookie);
#endif
		MMUQ_INSERT_TAIL(mh, me, me_list);

		/* onto on pmap chain; pmap is already locked, if needed */
		TAILQ_INSERT_TAIL(&newpm->pm_reglist, me, me_pmchain);

		/* into pmap segment table, with backpointers */
		me->me_pmap = newpm;
		me->me_vreg = newvr;

		return (me->me_cookie);
	}

	/* no luck, take head of LRU list */
	if (MMUQ_EMPTY(&region_lru))
		panic("region_alloc: all smegs gone");

	me = MMUQ_FIRST(&region_lru);

	pm = me->me_pmap;
	if (pm == NULL)
		panic("region_alloc: LRU entry has no pmap");
	if (pm == pmap_kernel())
		panic("region_alloc: stealing from kernel");
#ifdef DEBUG
	if (pmapdebug & (PDB_MMUREG_ALLOC | PDB_MMUREG_STEAL))
		printf("region_alloc: stealing smeg 0x%x from pmap %p\n",
		    me->me_cookie, pm);
#endif
	/*
	 * Remove from LRU list, and insert at end of new list
	 * (probably the LRU list again, but so what?).
	 */
	MMUQ_REMOVE(me, me_list);
	MMUQ_INSERT_TAIL(mh, me, me_list);

	rp = &pm->pm_regmap[me->me_vreg];
	ctx = getcontext4();

	/* Flush register windows; see comment in me_alloc() */
	write_user_windows();
	if (pm->pm_ctx) {
		setcontext4(pm->pm_ctxnum);
		cache_flush_region(me->me_vreg, pm->pm_ctxnum);
	}

	/* update region tables */
	if (pm->pm_ctx)
		setregmap(VRTOVA(me->me_vreg), reginval);
	rp->rg_smeg = reginval;

	/* off old pmap chain */
	TAILQ_REMOVE(&pm->pm_reglist, me, me_pmchain);
	setcontext4(ctx);	/* done with old context */

	/* onto new pmap chain; new pmap is already locked, if needed */
	TAILQ_INSERT_TAIL(&newpm->pm_reglist, me, me_pmchain);

	/* into new segment table, with backpointers */
	me->me_pmap = newpm;
	me->me_vreg = newvr;

	return (me->me_cookie);
}

/*
 * Free an MMU entry.
 * Assumes the corresponding pmap is already locked.
 * Caller must update hardware.
 */
void
region_free(struct pmap *pm, u_int smeg)
{
	struct mmuentry *me = &mmuregions[smeg];

#ifdef DEBUG
	if (pmapdebug & PDB_MMUREG_ALLOC)
		printf("region_free: freeing smeg 0x%x from pmap %p\n",
		    me->me_cookie, pm);
	if (me->me_cookie != smeg)
		panic("region_free: wrong mmuentry");
	if (pm != me->me_pmap)
		panic("region_free: pm != me_pmap");
#endif

	/* take mmu entry off pmap chain */
	TAILQ_REMOVE(&pm->pm_reglist, me, me_pmchain);

	/* off LRU or lock chain */
	MMUQ_REMOVE(me, me_list);

	/* no associated pmap; on free list */
	me->me_pmap = NULL;
	MMUQ_INSERT_TAIL(&region_freelist, me, me_list);
}

static void
mmu_pagein_reg(struct pmap *pm, struct regmap *rp, vaddr_t va,
		int vr, struct mmuq *mh)
{
	int i, s, smeg;

	va = VA_ROUNDDOWNTOREG(va);
	rp->rg_smeg = smeg = region_alloc(mh, pm, vr);

	s = splvm();
	if (pm == pmap_kernel()) {
		/* Map region into all contexts */
		int ctx = getcontext4();
		i = ncontext - 1;
		do {
			setcontext4(i);
			setregmap(va, smeg);
		} while (--i >= 0);
		setcontext4(ctx);
	} else
		setregmap(va, smeg);

	/* Load PMEGs into this region */
	for (i = 0; i < NSEGRG; i++) {
		setsegmap(va, rp->rg_segmap[i].sg_pmeg);
		va += NBPSG;
	}
	splx(s);
}
#endif /* SUN4_MMU3L */

static void
mmu_pmeg_lock(int pmeg)
{
	struct mmuentry *me = &mmusegments[pmeg];

	MMUQ_REMOVE(me, me_list);
	MMUQ_INSERT_TAIL(&segm_locked, me, me_list);
#ifdef DIAGNOSTIC
	(*me->me_statp)--;
	pmap_stats.ps_npmeg_locked++;
	me->me_statp = &pmap_stats.ps_npmeg_locked;
#endif
}

static void
mmu_pmeg_unlock(int pmeg)
{
	struct mmuentry *me = &mmusegments[pmeg];

	MMUQ_REMOVE(me, me_list);
	MMUQ_INSERT_TAIL(&segm_lru, me, me_list);
#ifdef DIAGNOSTIC
	(*me->me_statp)--;
	pmap_stats.ps_npmeg_lru++;
	me->me_statp = &pmap_stats.ps_npmeg_lru;
#endif
}

static void
mmu_pagein_seg(struct pmap *pm, struct segmap *sp, vaddr_t va,
		int vr, int vs, struct mmuq *mh)
{
	int s, i, pmeg, *pte;

	mmu_pagein_evcnt.ev_count++;

	va = VA_ROUNDDOWNTOSEG(va);
	s = splvm();		/* paranoid */
	sp->sg_pmeg = pmeg = me_alloc(mh, pm, vr, vs);
	if (pm != pmap_kernel() || HASSUN4_MMU3L)
		setsegmap(va, pmeg);
	else {
		/* Map kernel address into all contexts */
		int ctx = getcontext4();
		i = ncontext - 1;
		do {
			setcontext4(i);
			setsegmap(va, pmeg);
		} while (--i >= 0);
		setcontext4(ctx);
	}

	/* reload segment: write PTEs into a the MMU */
	pte = sp->sg_pte;
	i = NPTESG;
	do {
		setpte4(va, *pte++ & ~PG_MBZ);
		va += NBPG;
	} while (--i > 0);
	splx(s);
}

/*
 * `Page in' (load or inspect) an MMU entry; called on page faults.
 * Returns 1 if we reloaded the segment, -1 if the segment was
 * already loaded and the page was marked valid (in which case the
 * fault must be a bus error or something), or 0 (segment loaded but
 * PTE not valid, or segment not loaded at all).
 */
int
mmu_pagein(struct pmap *pm, vaddr_t va, int prot)
{
	int vr, vs, bits;
	struct regmap *rp;
	struct segmap *sp;

	PMAP_LOCK();

	if (prot != VM_PROT_NONE)
		bits = PG_V | ((prot & VM_PROT_WRITE) ? PG_W : 0);
	else
		bits = 0;

	vr = VA_VREG(va);
	vs = VA_VSEG(va);
	rp = &pm->pm_regmap[vr];

	/* return 0 if we have no PMEGs to load */
	if (rp->rg_nsegmap == 0) {
		PMAP_UNLOCK();
		return (0);
	}

#ifdef DIAGNOSTIC
	if (rp->rg_segmap == NULL)
		panic("pagein: no segmap");
#endif

#if defined(SUN4_MMU3L)
	if (HASSUN4_MMU3L && rp->rg_smeg == reginval)
		mmu_pagein_reg(pm, rp, va, vr, &region_lru);
#endif
	sp = &rp->rg_segmap[vs];

	/* return 0 if we have no PTEs to load */
	if (sp->sg_npte == 0) {
		PMAP_UNLOCK();
		return (0);
	}

	/* return -1 if the fault is `hard', 0 if not */
	if (sp->sg_pmeg != seginval) {
		PMAP_UNLOCK();
		return (bits && (getpte4(va) & bits) == bits ? -1 : 0);
	}

	mmu_pagein_seg(pm, sp, va, vr, vs, &segm_lru);
	PMAP_UNLOCK();
	return (1);
}
#endif /* SUN4 or SUN4C */

/*
 * Allocate a context.  If necessary, steal one from someone else.
 * Changes hardware context number and loads segment map.
 *
 * This routine is only ever called from locore.s just after it has
 * saved away the previous process, so there are no active user windows.
 */
void
ctx_alloc(struct pmap *pm)
{
	union ctxinfo *c;
	int cnum, i = 0, doflush;
	struct regmap *rp;
	int gap_start, gap_end;
	vaddr_t va;
#if defined(SUN4M) || defined(SUN4D)
	struct cpu_info *cpi;
#endif

/*XXX-GCC!*/gap_start=gap_end=0;
#ifdef DEBUG
	if (pm->pm_ctx)
		panic("ctx_alloc pm_ctx");
	if (pmapdebug & PDB_CTX_ALLOC)
		printf("ctx_alloc[%d](%p)\n", cpu_number(), pm);
#endif
	if (CPU_HAS_SUNMMU) {
		gap_start = pm->pm_gap_start;
		gap_end = pm->pm_gap_end;
	}

	mutex_spin_enter(&ctx_lock);
	if ((c = ctx_freelist) != NULL) {
		ctx_freelist = c->c_nextfree;
		cnum = c - ctxinfo;
		doflush = 0;
	} else {
		if ((ctx_kick += ctx_kickdir) >= ncontext) {
			ctx_kick = ncontext - 1;
			ctx_kickdir = -1;
		} else if (ctx_kick < 1) {
			ctx_kick = 1;
			ctx_kickdir = 1;
		}
		c = &ctxinfo[cnum = ctx_kick];
#ifdef DEBUG
		if (c->c_pmap == NULL)
			panic("ctx_alloc cu_pmap");
		if (pmapdebug & (PDB_CTX_ALLOC | PDB_CTX_STEAL))
			printf("ctx_alloc[%d]: steal context %d from %p\n",
			    cpu_number(), cnum, c->c_pmap);
#endif
		c->c_pmap->pm_ctx = NULL;
		c->c_pmap->pm_ctxnum = 0;
		doflush = (CACHEINFO.c_vactype != VAC_NONE);
		if (CPU_HAS_SUNMMU) {
			if (gap_start < c->c_pmap->pm_gap_start)
				gap_start = c->c_pmap->pm_gap_start;
			if (gap_end > c->c_pmap->pm_gap_end)
				gap_end = c->c_pmap->pm_gap_end;
		}
	}

	c->c_pmap = pm;
	pm->pm_ctx = c;
	pm->pm_ctxnum = cnum;

	if (CPU_HAS_SUNMMU) {

		/*
		 * Write pmap's region (3-level MMU) or segment table into
		 * the MMU.
		 *
		 * Only write those entries that actually map something in
		 * this context by maintaining a pair of region numbers in
		 * between which the pmap has no valid mappings.
		 *
		 * If a context was just allocated from the free list, trust
		 * that all its pmeg numbers are `seginval'. We make sure this
		 * is the case initially in pmap_bootstrap(). Otherwise, the
		 * context was freed by calling ctx_free() in pmap_release(),
		 * which in turn is supposedly called only when all mappings
		 * have been removed.
		 *
		 * On the other hand, if the context had to be stolen from
		 * another pmap, we possibly shrink the gap to be the
		 * disjuction of the new and the previous map.
		 */

		setcontext4(cnum);
		if (doflush)
			cache_flush_context(cnum);

		rp = pm->pm_regmap;
		for (va = 0, i = NUREG; --i >= 0; ) {
			if (VA_VREG(va) >= gap_start) {
				va = VRTOVA(gap_end);
				i -= gap_end - gap_start;
				rp += gap_end - gap_start;
				if (i < 0)
					break;
				/* mustn't re-enter this branch */
				gap_start = NUREG;
			}
			if (HASSUN4_MMU3L) {
				setregmap(va, rp++->rg_smeg);
				va += NBPRG;
			} else {
				int j;
				struct segmap *sp = rp->rg_segmap;
				for (j = NSEGRG; --j >= 0; va += NBPSG)
					setsegmap(va,
						  sp?sp++->sg_pmeg:seginval);
				rp++;
			}
		}

	} else if (CPU_HAS_SRMMU) {

#if defined(SUN4M) || defined(SUN4D)
		/*
		 * Reload page and context tables to activate the page tables
		 * for this context.
		 *
		 * The gap stuff isn't really needed in the sun4m architecture,
		 * since we don't have to worry about excessive mappings (all
		 * mappings exist since the page tables must be complete for
		 * the mmu to be happy).
		 *
		 * If a context was just allocated from the free list, trust
		 * that all of its mmu-edible page tables are zeroed out
		 * (except for those associated with the kernel). We make
		 * sure this is the case initially in pmap_bootstrap() and
		 * pmap_init() (?).
		 * Otherwise, the context was freed by calling ctx_free() in
		 * pmap_release(), which in turn is supposedly called only
		 * when all mappings have been removed.
		 *
		 * XXX: Do we have to flush cache after reloading ctx tbl?
		 */

		/*
		 * We need to flush the cache only when stealing a context
		 * from another pmap. In that case it's Ok to switch the
		 * context and leave it set, since the context table
		 * will have a valid region table entry for this context
		 * number.
		 *
		 * Otherwise, we switch to the new context after loading
		 * the context table entry with the new pmap's region.
		 */
		if (doflush) {
			cache_flush_context(cnum);
		}

		/*
		 * The context allocated to a process is the same on all CPUs.
		 * Here we install the per-CPU region table in each CPU's
		 * context table slot.
		 *
		 * Note on multi-threaded processes: a context must remain
		 * valid as long as any thread is still running on a CPU.
		 */
		for (CPU_INFO_FOREACH(i, cpi)) {
			setpgt4m(&cpi->ctx_tbl[cnum],
				 (pm->pm_reg_ptps_pa[i] >> SRMMU_PPNPASHIFT) |
					SRMMU_TEPTD);
		}

		/* And finally switch to the new context */
		(*cpuinfo.pure_vcache_flush)();
		setcontext4m(cnum);
#endif /* SUN4M || SUN4D */
	}
	mutex_spin_exit(&ctx_lock);
}

/*
 * Give away a context.
 */
void
ctx_free(struct pmap *pm)
{
	union ctxinfo *c;
	int ctx;
#if defined(SUN4M) || defined(SUN4D)
	struct cpu_info *cpi;
#endif

	c = pm->pm_ctx;
	ctx = pm->pm_ctxnum;
	pm->pm_ctx = NULL;
	pm->pm_ctxnum = 0;
#if defined(SUN4) || defined(SUN4C)
	if (CPU_HAS_SUNMMU) {
		int octx = getcontext4();
		setcontext4(ctx);
		cache_flush_context(ctx);
		setcontext4(octx);
	}
#endif /* SUN4 || SUN4C */

	mutex_spin_enter(&ctx_lock);

#if defined(SUN4M) || defined(SUN4D)
	if (CPU_HAS_SRMMU) {
		int i;

		cache_flush_context(ctx);
		tlb_flush_context(ctx, PMAP_CPUSET(pm));
		for (CPU_INFO_FOREACH(i, cpi)) {
			setpgt4m(&cpi->ctx_tbl[ctx], SRMMU_TEINVALID);
		}
	}
#endif

	c->c_nextfree = ctx_freelist;
	ctx_freelist = c;
	mutex_spin_exit(&ctx_lock);
}


/*----------------------------------------------------------------*/

/*
 * pvlist functions.
 */

/*
 * Walk the given pv list, and for each PTE, set or clear some bits
 * (e.g., PG_W or PG_NC).
 *
 * This routine flushes the cache for any page whose PTE changes,
 * as long as the process has a context; this is overly conservative.
 * It also copies ref and mod bits to the pvlist, on the theory that
 * this might save work later.  (XXX should test this theory)
 */

#if defined(SUN4) || defined(SUN4C)

void
pv_changepte4_4c(struct vm_page *pg, int bis, int bic)
{
	int pte, *ptep;
	struct pvlist *pv;
	struct pmap *pm;
	int va, vr, vs;
	int ctx, s;
	struct regmap *rp;
	struct segmap *sp;

	pv = VM_MDPAGE_PVHEAD(pg);

	write_user_windows();		/* paranoid? */
	s = splvm();			/* paranoid? */
	if (pv->pv_pmap == NULL) {
		splx(s);
		return;
	}
	ctx = getcontext4();
	for (; pv != NULL; pv = pv->pv_next) {
		pm = pv->pv_pmap;
		va = pv->pv_va;
		vr = VA_VREG(va);
		vs = VA_VSEG(va);
		rp = &pm->pm_regmap[vr];
		sp = &rp->rg_segmap[vs];
		ptep = &sp->sg_pte[VA_VPG(va)];

		if (sp->sg_pmeg == seginval) {
			/* not in hardware: just fix software copy */
			*ptep = (*ptep | bis) & ~bic;
		} else {
			/* in hardware: fix hardware copy */
			if (CTX_USABLE(pm,rp)) {
				setcontext4(pm->pm_ctxnum);
				/* XXX should flush only when necessary */
				pte = getpte4(va);
				/*
				 * XXX: always flush cache; conservative, but
				 * needed to invalidate cache tag protection
				 * bits and when disabling caching.
				 */
				cache_flush_page(va, pm->pm_ctxnum);
			} else {
				/* Make temp map in ctx 0 to access the PTE */
				setcontext4(0);
				if (HASSUN4_MMU3L)
					setregmap(0, tregion);
				setsegmap(0, sp->sg_pmeg);
				va = VA_VPG(va) << PGSHIFT;
				pte = getpte4(va);
			}
			if (pte & PG_V)
				VM_MDPAGE_PVHEAD(pg)->pv_flags |= MR4_4C(pte);
			pte = (pte | bis) & ~bic;
			setpte4(va, pte);
			*ptep = (*ptep & PG_MBZ) | pte;
		}
	}
	setcontext4(ctx);
	splx(s);
}

/*
 * Sync ref and mod bits in pvlist (turns off same in hardware PTEs).
 * Returns the new flags.
 *
 * This is just like pv_changepte, but we never add or remove bits,
 * hence never need to adjust software copies.
 */
int
pv_syncflags4_4c(struct vm_page *pg)
{
	struct pvlist *pv;
	struct pmap *pm;
	int pte, va, vr, vs, pmeg, flags;
	int ctx, s;
	struct regmap *rp;
	struct segmap *sp;

	pv = VM_MDPAGE_PVHEAD(pg);

	s = splvm();			/* paranoid? */
	if (pv->pv_pmap == NULL) {
		/* Page not mapped; pv_flags is already up to date */
		splx(s);
		return (0);
	}
	ctx = getcontext4();
	flags = pv->pv_flags;
	for (; pv != NULL; pv = pv->pv_next) {
		pm = pv->pv_pmap;
		va = pv->pv_va;
		vr = VA_VREG(va);
		vs = VA_VSEG(va);
		rp = &pm->pm_regmap[vr];
		sp = &rp->rg_segmap[vs];
		if ((pmeg = sp->sg_pmeg) == seginval)
			continue;
		if (CTX_USABLE(pm,rp)) {
			setcontext4(pm->pm_ctxnum);
			/* XXX should flush only when necessary */
			pte = getpte4(va);
			if (pte & PG_M)
				cache_flush_page(va, pm->pm_ctxnum);
		} else {
			/* Make temp map in ctx 0 to access the PTE */
			setcontext4(0);
			if (HASSUN4_MMU3L)
				setregmap(0, tregion);
			setsegmap(0, pmeg);
			va = VA_VPG(va) << PGSHIFT;
			pte = getpte4(va);
		}
		if (pte & (PG_M|PG_U) && pte & PG_V) {
			flags |= MR4_4C(pte);
			pte &= ~(PG_M|PG_U);
			setpte4(va, pte);
		}
	}

	VM_MDPAGE_PVHEAD(pg)->pv_flags = flags;
	setcontext4(ctx);
	splx(s);
	return (flags);
}

/*
 * pv_unlink is a helper function for pmap_remove.
 * It takes a pointer to the pv_table head for some physical address
 * and removes the appropriate (pmap, va) entry.
 *
 * Once the entry is removed, if the pv_table head has the cache
 * inhibit bit set, see if we can turn that off; if so, walk the
 * pvlist and turn off PG_NC in each PTE.  (The pvlist is by
 * definition nonempty, since it must have at least two elements
 * in it to have PV_NC set, and we only remove one here.)
 */
/*static*/ void
pv_unlink4_4c(struct vm_page *pg, struct pmap *pm, vaddr_t va)
{
	struct pvlist *pv0, *npv;

	pv0 = VM_MDPAGE_PVHEAD(pg);
	npv = pv0->pv_next;

	/*
	 * First entry is special (sigh).
	 */
	if (pv0->pv_pmap == pm && pv0->pv_va == va) {
		pmap_stats.ps_unlink_pvfirst++;
		if (npv != NULL) {
			/*
			 * Shift next entry into the head.
			 * Make sure to retain the REF, MOD and ANC flags.
			 */
			pv0->pv_next = npv->pv_next;
			pv0->pv_pmap = npv->pv_pmap;
			pv0->pv_va = npv->pv_va;
			pv0->pv_flags &= ~PV_NC;
			pv0->pv_flags |= (npv->pv_flags & PV_NC);
			pool_put(&pv_pool, npv);
		} else {
			/*
			 * No mappings left; we still need to maintain
			 * the REF and MOD flags. since pmap_is_modified()
			 * can still be called for this page.
			 */
			pv0->pv_pmap = NULL;
			pv0->pv_flags &= ~(PV_NC|PV_ANC);
			return;
		}
	} else {
		struct pvlist *prev;

		pmap_stats.ps_unlink_pvsearch++;
		for (prev = pv0;; prev = npv, npv = npv->pv_next) {
			if (npv == NULL) {
				panic("pv_unlink: pm %p is missing on pg %p",
					pm, pg);
			}
			if (npv->pv_pmap == pm && npv->pv_va == va)
				break;
		}
		prev->pv_next = npv->pv_next;
		pool_put(&pv_pool, npv);
	}
	if ((pv0->pv_flags & (PV_NC|PV_ANC)) == PV_ANC) {
		/*
		 * Not cached: check whether we can fix that now.
		 */
		va = pv0->pv_va;
		for (npv = pv0->pv_next; npv != NULL; npv = npv->pv_next)
			if (BADALIAS(va, npv->pv_va) ||
			    (npv->pv_flags & PV_NC) != 0)
				return;
		pv0->pv_flags &= ~PV_ANC;
		pv_changepte4_4c(pg, 0, PG_NC);
	}
}

/*
 * pv_link is the inverse of pv_unlink, and is used in pmap_enter.
 * It returns PG_NC if the (new) pvlist says that the address cannot
 * be cached.
 */
/*static*/ int
pv_link4_4c(struct vm_page *pg, struct pmap *pm, vaddr_t va,
	    unsigned int *pteprotop)
{
	struct pvlist *pv0, *pv, *npv;
	int nc = (*pteprotop & PG_NC) != 0 ? PV_NC : 0;

	pv0 = VM_MDPAGE_PVHEAD(pg);

	if (pv0->pv_pmap == NULL) {
		/* no pvlist entries yet */
		pmap_stats.ps_enter_firstpv++;
		pv0->pv_next = NULL;
		pv0->pv_pmap = pm;
		pv0->pv_va = va;
		pv0->pv_flags |= nc;
		return (0);
	}

	/*
	 * Allocate the new PV entry now, and, if that fails, bail out
	 * before changing the cacheable state of the existing mappings.
	 */
	npv = pool_get(&pv_pool, PR_NOWAIT);
	if (npv == NULL)
		return (ENOMEM);

	pmap_stats.ps_enter_secondpv++;

	/*
	 * Before entering the new mapping, see if
	 * it will cause old mappings to become aliased
	 * and thus need to be `discached'.
	 */
	if (pv0->pv_flags & PV_ANC) {
		/* already uncached, just stay that way */
		*pteprotop |= PG_NC;
		goto link_npv;
	}

	for (pv = pv0; pv != NULL; pv = pv->pv_next) {
		if ((pv->pv_flags & PV_NC) != 0) {
			*pteprotop |= PG_NC;
#ifdef DEBUG
			/* Check currently illegal condition */
			if (nc == 0)
				printf("pv_link: proc %s, va=0x%lx: "
				"unexpected uncached mapping at 0x%lx\n",
				    curproc ? curproc->p_comm : "--",
				    va, pv->pv_va);
#endif
		}
		if (BADALIAS(va, pv->pv_va)) {
#ifdef DEBUG
			if (pmapdebug & PDB_CACHESTUFF)
				printf(
			"pv_link: badalias: proc %s, 0x%lx<=>0x%lx, pg %p\n",
				curproc ? curproc->p_comm : "--",
				va, pv->pv_va, pg);
#endif
			/* Mark list head `uncached due to aliases' */
			pv0->pv_flags |= PV_ANC;
			pv_changepte4_4c(pg, PG_NC, 0);
			*pteprotop |= PG_NC;
			break;
		}
	}

link_npv:
	npv->pv_next = pv0->pv_next;
	npv->pv_pmap = pm;
	npv->pv_va = va;
	npv->pv_flags = nc;
	pv0->pv_next = npv;
	return (0);
}

#endif /* SUN4 || SUN4C */

#if defined(SUN4M) || defined(SUN4D)	/* SRMMU versions of above */
/*
 * Walk the given pv list, and for each PTE, set or clear some bits
 * (e.g., PG_W or PG_NC).
 *
 * This routine flushes the cache for any page whose PTE changes,
 * as long as the process has a context; this is overly conservative.
 * It also copies ref and mod bits to the pvlist, on the theory that
 * this might save work later.  (XXX should test this theory)
 *
 * Called with PV lock and pmap main lock held.
 */
void
pv_changepte4m(struct vm_page *pg, int bis, int bic)
{
	struct pvlist *pv;
	struct pmap *pm;
	vaddr_t va;
	struct regmap *rp;
	struct segmap *sp;

	pv = VM_MDPAGE_PVHEAD(pg);
	if (pv->pv_pmap == NULL)
		return;

	for (; pv != NULL; pv = pv->pv_next) {
		int tpte;
		pm = pv->pv_pmap;
		/* XXXSMP: should lock pm */
		va = pv->pv_va;
		rp = &pm->pm_regmap[VA_VREG(va)];
		sp = &rp->rg_segmap[VA_VSEG(va)];

		if (pm->pm_ctx) {
			/*
			 * XXX: always flush cache; conservative, but
			 * needed to invalidate cache tag protection
			 * bits and when disabling caching.
			 */
			cache_flush_page(va, pm->pm_ctxnum);
		}

		tpte = sp->sg_pte[VA_SUN4M_VPG(va)];
		KASSERT((tpte & SRMMU_TETYPE) == SRMMU_TEPTE);
		VM_MDPAGE_PVHEAD(pg)->pv_flags |= MR4M(updatepte4m(va,
		    &sp->sg_pte[VA_SUN4M_VPG(va)], bic, bis, pm->pm_ctxnum,
		    PMAP_CPUSET(pm)));
	}
}

/*
 * Sync ref and mod bits in pvlist. If page has been ref'd or modified,
 * update ref/mod bits in pvlist, and clear the hardware bits.
 *
 * Return the new flags.
 */
int
pv_syncflags4m(struct vm_page *pg)
{
	struct pvlist *pv;
	struct pmap *pm;
	int va, flags;
	int s;
	struct regmap *rp;
	struct segmap *sp;
	int tpte;

	s = splvm();
	PMAP_LOCK();
	pv = VM_MDPAGE_PVHEAD(pg);
	if (pv->pv_pmap == NULL) {
		/* Page not mapped; pv_flags is already up to date */
		flags = 0;
		goto out;
	}

	flags = pv->pv_flags;
	for (; pv != NULL; pv = pv->pv_next) {
		pm = pv->pv_pmap;
		va = pv->pv_va;
		rp = &pm->pm_regmap[VA_VREG(va)];
		sp = &rp->rg_segmap[VA_VSEG(va)];

		tpte = sp->sg_pte[VA_SUN4M_VPG(va)];
		if ((tpte & SRMMU_TETYPE) == SRMMU_TEPTE &&
		    (tpte & (SRMMU_PG_R|SRMMU_PG_M)) != 0) {
			/*
			 * Flush cache if modified to make sure the PTE
			 * M bit will be set again on the next write access.
			 */
			if (pm->pm_ctx && (tpte & SRMMU_PG_M) == SRMMU_PG_M)
				cache_flush_page(va, pm->pm_ctxnum);

			flags |= MR4M(updatepte4m(va,
					&sp->sg_pte[VA_SUN4M_VPG(va)],
					SRMMU_PG_M | SRMMU_PG_R,
					0, pm->pm_ctxnum, PMAP_CPUSET(pm)));
		}
	}

	VM_MDPAGE_PVHEAD(pg)->pv_flags = flags;
out:
	PMAP_UNLOCK();
	splx(s);
	return (flags);
}

/*
 * Should be called with pmap already locked.
 */
void
pv_unlink4m(struct vm_page *pg, struct pmap *pm, vaddr_t va)
{
	struct pvlist *pv0, *npv;

	pv0 = VM_MDPAGE_PVHEAD(pg);

	npv = pv0->pv_next;
	/*
	 * First entry is special (sigh).
	 */
	if (pv0->pv_pmap == pm && pv0->pv_va == va) {
		pmap_stats.ps_unlink_pvfirst++;
		if (npv != NULL) {
			/*
			 * Shift next entry into the head.
			 * Make sure to retain the REF, MOD and ANC flags
			 * on the list head.
			 */
			pv0->pv_next = npv->pv_next;
			pv0->pv_pmap = npv->pv_pmap;
			pv0->pv_va = npv->pv_va;
			pv0->pv_flags &= ~PV_NC;
			pv0->pv_flags |= (npv->pv_flags & PV_NC);
			pool_put(&pv_pool, npv);
		} else {
			/*
			 * No mappings left; we need to maintain
			 * the REF and MOD flags, since pmap_is_modified()
			 * can still be called for this page.
			 */
			pv0->pv_pmap = NULL;
			pv0->pv_flags &= ~(PV_NC|PV_ANC);
			return;
		}
	} else {
		struct pvlist *prev;

		pmap_stats.ps_unlink_pvsearch++;
		for (prev = pv0;; prev = npv, npv = npv->pv_next) {
			if (npv == NULL) {
				panic("pv_unlink: pm %p is missing on pg %p",
					pm, pg);
				return;
			}
			if (npv->pv_pmap == pm && npv->pv_va == va)
				break;
		}
		prev->pv_next = npv->pv_next;
		pool_put(&pv_pool, npv);
	}

	if ((pv0->pv_flags & (PV_NC|PV_ANC)) == PV_ANC) {

		/*
		 * Not cached: check whether we can fix that now.
		 */
		va = pv0->pv_va;
		for (npv = pv0->pv_next; npv != NULL; npv = npv->pv_next)
			if (BADALIAS(va, npv->pv_va) ||
			    (npv->pv_flags & PV_NC) != 0)
				return;
#ifdef DEBUG
		if (pmapdebug & PDB_CACHESTUFF)
			printf(
			"pv_unlink: alias ok: proc %s, va 0x%lx, pg %p\n",
				curproc ? curproc->p_comm : "--",
				va, pg);
#endif
		pv0->pv_flags &= ~PV_ANC;
		pv_changepte4m(pg, SRMMU_PG_C, 0);
	}
}

/*
 * pv_link is the inverse of pv_unlink, and is used in pmap_enter.
 * May turn off the cacheable bit in the pte prototype for the new mapping.
 * Called with pm locked.
 */
/*static*/ int
pv_link4m(struct vm_page *pg, struct pmap *pm, vaddr_t va,
	  unsigned int *pteprotop)
{
	struct pvlist *pv0, *pv, *npv;
	int nc = (*pteprotop & SRMMU_PG_C) == 0 ? PV_NC : 0;
	int error = 0;

	pv0 = VM_MDPAGE_PVHEAD(pg);

	if (pv0->pv_pmap == NULL) {
		/* no pvlist entries yet */
		pmap_stats.ps_enter_firstpv++;
		pv0->pv_next = NULL;
		pv0->pv_pmap = pm;
		pv0->pv_va = va;
		pv0->pv_flags |= nc;
		goto out;
	}

	/*
	 * Allocate the new PV entry now, and, if that fails, bail out
	 * before changing the cacheable state of the existing mappings.
	 */
	npv = pool_get(&pv_pool, PR_NOWAIT);
	if (npv == NULL) {
		error = ENOMEM;
		goto out;
	}

	pmap_stats.ps_enter_secondpv++;

	/*
	 * See if the new mapping will cause old mappings to
	 * become aliased and thus need to be `discached'.
	 */
	if ((pv0->pv_flags & PV_ANC) != 0) {
		/* already uncached, just stay that way */
		*pteprotop &= ~SRMMU_PG_C;
		goto link_npv;
	}

	for (pv = pv0; pv != NULL; pv = pv->pv_next) {
		if ((pv->pv_flags & PV_NC) != 0) {
			*pteprotop &= ~SRMMU_PG_C;
#ifdef DEBUG
			/* Check currently illegal condition */
			if (nc == 0)
				printf("pv_link: proc %s, va=0x%lx: "
				"unexpected uncached mapping at 0x%lx\n",
				    curproc ? curproc->p_comm : "--",
				    va, pv->pv_va);
#endif
		}
		if (BADALIAS(va, pv->pv_va)) {
#ifdef DEBUG
			if (pmapdebug & PDB_CACHESTUFF)
				printf(
			"pv_link: badalias: proc %s, 0x%lx<=>0x%lx, pg %p\n",
				curproc ? curproc->p_comm : "--",
				va, pv->pv_va, pg);
#endif
			/* Mark list head `uncached due to aliases' */
			pv0->pv_flags |= PV_ANC;
			pv_changepte4m(pg, 0, SRMMU_PG_C);
			*pteprotop &= ~SRMMU_PG_C;
			break;
		}
	}

link_npv:
	/* Now link in the new PV entry */
	npv->pv_next = pv0->pv_next;
	npv->pv_pmap = pm;
	npv->pv_va = va;
	npv->pv_flags = nc;
	pv0->pv_next = npv;

out:
	return (error);
}
#endif

/*
 * Uncache all entries on behalf of kvm_uncache(). In addition to
 * removing the cache bit from the PTE, we are also setting PV_NC
 * in each entry to stop pv_unlink() from re-caching (i.e. when a
 * a bad alias is going away).
 */
static void
pv_uncache(struct vm_page *pg)
{
	struct pvlist *pv;
	int s;

	s = splvm();
	PMAP_LOCK();

	for (pv = VM_MDPAGE_PVHEAD(pg); pv != NULL; pv = pv->pv_next)
		pv->pv_flags |= PV_NC;

#if defined(SUN4M) || defined(SUN4D)
	if (CPU_HAS_SRMMU)
		pv_changepte4m(pg, 0, SRMMU_PG_C);
#endif
#if defined(SUN4) || defined(SUN4C)
	if (CPU_HAS_SUNMMU)
		pv_changepte4_4c(pg, PG_NC, 0);
#endif
	PMAP_UNLOCK();
	splx(s);
}

/*
 * Walk the given list and flush the cache for each (MI) page that is
 * potentially in the cache. Called only if vactype != VAC_NONE.
 */
#if defined(SUN4) || defined(SUN4C)
static void
pv_flushcache4_4c(struct vm_page *pg)
{
	struct pvlist *pv;
	struct pmap *pm;
	int s, ctx;

	pv = VM_MDPAGE_PVHEAD(pg);

	write_user_windows();	/* paranoia? */
	s = splvm();		/* XXX extreme paranoia */
	if ((pm = pv->pv_pmap) != NULL) {
		ctx = getcontext4();
		for (;;) {
			if (pm->pm_ctx) {
				setcontext4(pm->pm_ctxnum);
				cache_flush_page(pv->pv_va, pm->pm_ctxnum);
			}
			pv = pv->pv_next;
			if (pv == NULL)
				break;
			pm = pv->pv_pmap;
		}
		setcontext4(ctx);
	}
	splx(s);
}
#endif /* SUN4 || SUN4C */

#if defined(SUN4M) || defined(SUN4D)
static void
pv_flushcache4m(struct vm_page *pg)
{
	struct pvlist *pv;
	struct pmap *pm;
	int s;

	pv = VM_MDPAGE_PVHEAD(pg);

	s = splvm();		/* XXX extreme paranoia */
	if ((pm = pv->pv_pmap) != NULL) {
		for (;;) {
			if (pm->pm_ctx) {
				cache_flush_page(pv->pv_va, pm->pm_ctxnum);
			}
			pv = pv->pv_next;
			if (pv == NULL)
				break;
			pm = pv->pv_pmap;
		}
	}
	splx(s);
}
#endif /* SUN4M || SUN4D */

/*----------------------------------------------------------------*/

/*
 * At last, pmap code.
 */

#if defined(SUN4) && (defined(SUN4C) || defined(SUN4M) || defined(SUN4D))
int nptesg;
#endif

#if defined(SUN4M) || defined(SUN4D)
static void pmap_bootstrap4m(void *);
#endif
#if defined(SUN4) || defined(SUN4C)
static void pmap_bootstrap4_4c(void *, int, int, int);
#endif

/*
 * Bootstrap the system enough to run with VM enabled.
 *
 * nsegment is the number of mmu segment entries (``PMEGs'');
 * nregion is the number of mmu region entries (``SMEGs'');
 * nctx is the number of contexts.
 */
void
pmap_bootstrap(int nctx, int nregion, int nsegment)
{
	void *p;
	extern char etext[], kernel_data_start[];
	extern char *kernel_top;

	uvmexp.pagesize = NBPG;
	uvm_setpagesize();

#if defined(SUN4) && (defined(SUN4C) || defined(SUN4M) || defined(SUN4D))
	/* In this case NPTESG is a variable */
	nptesg = (NBPSG >> pgshift);
#endif

	/*
	 * Grab physical memory list.
	 */
	p = kernel_top;
	get_phys_mem(&p);

	/*
	 * The data segment in sparc ELF images is aligned to a 64KB
	 * (the maximum page size defined by the ELF/sparc ABI) boundary.
	 * This results in a unused portion of physical memory in between
	 * the text/rodata and the data segment. We pick up that gap
	 * here to remove it from the kernel map and give it to the
	 * VM manager later.
	 */
	etext_gap_start = (vaddr_t)(etext + NBPG - 1) & ~PGOFSET;
	etext_gap_end = (vaddr_t)kernel_data_start & ~PGOFSET;

	if (CPU_HAS_SRMMU) {
#if defined(SUN4M) || defined(SUN4D)
		pmap_bootstrap4m(p);
#endif
	} else if (CPU_HAS_SUNMMU) {
#if defined(SUN4) || defined(SUN4C)
		pmap_bootstrap4_4c(p, nctx, nregion, nsegment);
#endif
	}

	pmap_page_upload();
	mutex_init(&demap_lock, MUTEX_DEFAULT, IPL_VM);
	mutex_init(&ctx_lock, MUTEX_DEFAULT, IPL_SCHED);
}

#if defined(SUN4) || defined(SUN4C)
void
pmap_bootstrap4_4c(void *top, int nctx, int nregion, int nsegment)
{
	union ctxinfo *ci;
	struct mmuentry *mmuseg;
#if defined(SUN4_MMU3L)
	struct mmuentry *mmureg;
#endif
	struct regmap *rp;
	struct segmap *sp;
	int i, j;
	int npte, zseg, vr, vs;
	int startscookie, scookie;
#if defined(SUN4_MMU3L)
	int startrcookie = 0, rcookie = 0;
#endif
	int *kptes;
	int lastpage;
	vaddr_t va;
	vaddr_t p;
	extern char kernel_text[];

	/*
	 * Compute `va2pa_offset'.
	 * Use `kernel_text' to probe the MMU translation since
	 * the pages at KERNBASE might not be mapped.
	 */
	va2pa_offset = (vaddr_t)kernel_text -
			((getpte4(kernel_text) & PG_PFNUM) << PGSHIFT);

	ncontext = nctx;

	switch (cputyp) {
	case CPU_SUN4C:
		mmu_has_hole = 1;
		break;
	case CPU_SUN4:
		if (cpuinfo.cpu_type != CPUTYP_4_400) {
			mmu_has_hole = 1;
			break;
		}
	}

#if defined(SUN4)
	/*
	 * set up the segfixmask to mask off invalid bits
	 */
	segfixmask =  nsegment - 1; /* assume nsegment is a power of 2 */
#ifdef DIAGNOSTIC
	if (((nsegment & segfixmask) | (nsegment & ~segfixmask)) != nsegment) {
		printf("pmap_bootstrap: unsuitable number of segments (%d)\n",
			nsegment);
		callrom();
	}
#endif
#endif

#if defined(SUN4M) || defined(SUN4D) /* We're in a dual-arch kernel.
					Setup 4/4c fn. ptrs */
	pmap_clear_modify_p 	=	pmap_clear_modify4_4c;
	pmap_clear_reference_p 	= 	pmap_clear_reference4_4c;
	pmap_enter_p 		=	pmap_enter4_4c;
	pmap_extract_p 		=	pmap_extract4_4c;
	pmap_is_modified_p 	=	pmap_is_modified4_4c;
	pmap_is_referenced_p	=	pmap_is_referenced4_4c;
	pmap_kenter_pa_p 	=	pmap_kenter_pa4_4c;
	pmap_kremove_p	 	=	pmap_kremove4_4c;
	pmap_kprotect_p	 	=	pmap_kprotect4_4c;
	pmap_page_protect_p	=	pmap_page_protect4_4c;
	pmap_protect_p		=	pmap_protect4_4c;
	pmap_rmk_p		=	pmap_rmk4_4c;
	pmap_rmu_p		=	pmap_rmu4_4c;
#endif /* defined SUN4M || defined SUN4D */

	p = (vaddr_t)top;

	/*
	 * Last segment is the `invalid' one (one PMEG of pte's with !pg_v).
	 * It will never be used for anything else.
	 */
	seginval = --nsegment;

#if defined(SUN4_MMU3L)
	if (HASSUN4_MMU3L)
		reginval = --nregion;
#endif

	/*
	 * Allocate and initialise mmu entries and context structures.
	 */
#if defined(SUN4_MMU3L)
	mmuregions = mmureg = (struct mmuentry *)p;
	p += nregion * sizeof(struct mmuentry);
	bzero(mmuregions, nregion * sizeof(struct mmuentry));
#endif
	mmusegments = mmuseg = (struct mmuentry *)p;
	p += nsegment * sizeof(struct mmuentry);
	bzero(mmusegments, nsegment * sizeof(struct mmuentry));

	pmap_kernel()->pm_ctx = ctxinfo = ci = (union ctxinfo *)p;
	p += nctx * sizeof *ci;

	/* Initialize MMU resource queues */
#if defined(SUN4_MMU3L)
	MMUQ_INIT(&region_freelist);
	MMUQ_INIT(&region_lru);
	MMUQ_INIT(&region_locked);
#endif
	MMUQ_INIT(&segm_freelist);
	MMUQ_INIT(&segm_lru);
	MMUQ_INIT(&segm_locked);


	/*
	 * Intialize the kernel pmap.
	 */
	/* kernel_pmap_store.pm_ctxnum = 0; */
	kernel_pmap_store.pm_refcount = 1;
#if defined(SUN4_MMU3L)
	TAILQ_INIT(&kernel_pmap_store.pm_reglist);
#endif
	TAILQ_INIT(&kernel_pmap_store.pm_seglist);

	/*
	 * Allocate memory for kernel PTEs
	 * XXX Consider allocating memory for only a few regions
	 * and use growkernel() to allocate more as needed.
	 */
	kptes = (int *)p;
	p += NKREG * NSEGRG * NPTESG * sizeof(int);
	bzero(kptes, NKREG * NSEGRG * NPTESG * sizeof(int));

	/*
	 * Set up pm_regmap for kernel to point NUREG *below* the beginning
	 * of kernel regmap storage. Since the kernel only uses regions
	 * above NUREG, we save storage space and can index kernel and
	 * user regions in the same way.
	 */
	kernel_pmap_store.pm_regmap = &kernel_regmap_store[-NUREG];
	for (i = NKREG; --i >= 0;) {
#if defined(SUN4_MMU3L)
		kernel_regmap_store[i].rg_smeg = reginval;
#endif
		kernel_regmap_store[i].rg_segmap =
			&kernel_segmap_store[i * NSEGRG];
		for (j = NSEGRG; --j >= 0;) {
			sp = &kernel_segmap_store[i * NSEGRG + j];
			sp->sg_pmeg = seginval;
			sp->sg_pte = &kptes[(i * NSEGRG + j) * NPTESG];
		}
	}

	/*
	 * Preserve the monitor ROM's reserved VM region, so that
	 * we can use L1-A or the monitor's debugger.  As a side
	 * effect we map the ROM's reserved VM into all contexts
	 * (otherwise L1-A crashes the machine!).
	 */

	mmu_reservemon4_4c(&nregion, &nsegment);

#if defined(SUN4_MMU3L)
	/* Reserve one region for temporary mappings */
	if (HASSUN4_MMU3L)
		tregion = --nregion;
#endif

	/*
	 * Set up the `constants' for the call to vm_init()
	 * in main().  All pages beginning at p (rounded up to
	 * the next whole page) and continuing through the number
	 * of available pages are free, but they start at a higher
	 * virtual address.  This gives us two mappable MD pages
	 * for pmap_zero_page and pmap_copy_page, and one MI page
	 * for /dev/mem, all with no associated physical memory.
	 */
	p = (p + NBPG - 1) & ~PGOFSET;

	avail_start = PMAP_BOOTSTRAP_VA2PA(p);

	i = p;
	cpuinfo.vpage[0] = (void *)p, p += NBPG;
	cpuinfo.vpage[1] = (void *)p, p += NBPG;
	vmmap = (void *)p, p += NBPG;
	p = (vaddr_t)reserve_dumppages((void *)p);

	virtual_avail = p;
	virtual_end = VM_MAX_KERNEL_ADDRESS;

	p = i;			/* retract to first free phys */


	/*
	 * All contexts are free except the kernel's.
	 *
	 * XXX sun4c could use context 0 for users?
	 */
	ci->c_pmap = pmap_kernel();
	ctx_freelist = ci + 1;
	for (i = 1; i < ncontext; i++) {
		ci++;
		ci->c_nextfree = ci + 1;
	}
	ci->c_nextfree = NULL;
	ctx_kick = 0;
	ctx_kickdir = -1;

	/*
	 * Init mmu entries that map the kernel physical addresses.
	 *
	 * All the other MMU entries are free.
	 *
	 * THIS ASSUMES THE KERNEL IS MAPPED BY A CONTIGUOUS RANGE OF
	 * MMU SEGMENTS/REGIONS DURING THE BOOT PROCESS
	 */

	/* Compute the number of segments used by the kernel */
	zseg = (((p + NBPSG - 1) & ~SGOFSET) - KERNBASE) >> SGSHIFT;
	lastpage = VA_VPG(p);
	if (lastpage == 0)
		/*
		 * If the page bits in p are 0, we filled the last segment
		 * exactly; if not, it is the last page filled in the
		 * last segment.
		 */
		lastpage = NPTESG;

	p = KERNBASE;			/* first va */
	vs = VA_VSEG(KERNBASE);		/* first virtual segment */
	vr = VA_VREG(KERNBASE);		/* first virtual region */
	rp = &pmap_kernel()->pm_regmap[vr];

	/* Get region/segment where kernel addresses start */
#if defined(SUN4_MMU3L)
	if (HASSUN4_MMU3L)
		startrcookie = rcookie = getregmap(p);
	mmureg = &mmuregions[rcookie];
#endif

	startscookie = scookie = getsegmap(p);
	mmuseg = &mmusegments[scookie];
	zseg += scookie;	/* First free segment */

	for (;;) {

		/*
		 * Distribute each kernel region/segment into all contexts.
		 * This is done through the monitor ROM, rather than
		 * directly here: if we do a setcontext we will fault,
		 * as we are not (yet) mapped in any other context.
		 */

		if ((vs % NSEGRG) == 0) {
			/* Entering a new region */
			if (VA_VREG(p) > vr) {
#ifdef DEBUG
				printf("note: giant kernel!\n");
#endif
				vr++, rp++;
			}
#if defined(SUN4_MMU3L)
			if (HASSUN4_MMU3L) {
				for (i = 1; i < nctx; i++)
					prom_setcontext(i, (void *)p, rcookie);

				MMUQ_INSERT_TAIL(&region_locked,
						  mmureg, me_list);
				TAILQ_INSERT_TAIL(&pmap_kernel()->pm_reglist,
						  mmureg, me_pmchain);
#ifdef DIAGNOSTIC
				mmuseg->me_statp = NULL;
#endif
				mmureg->me_cookie = rcookie;
				mmureg->me_pmap = pmap_kernel();
				mmureg->me_vreg = vr;
				rp->rg_smeg = rcookie;
				mmureg++;
				rcookie++;
			}
#endif /* SUN4_MMU3L */
		}

#if defined(SUN4_MMU3L)
		if (!HASSUN4_MMU3L)
#endif
			for (i = 1; i < nctx; i++)
				prom_setcontext(i, (void *)p, scookie);

		/* set up the mmu entry */
		MMUQ_INSERT_TAIL(&segm_locked, mmuseg, me_list);
#ifdef DIAGNOSTIC
		mmuseg->me_statp = &pmap_stats.ps_npmeg_locked;
#endif
		TAILQ_INSERT_TAIL(&pmap_kernel()->pm_seglist, mmuseg, me_pmchain);
		pmap_stats.ps_npmeg_locked++;
		mmuseg->me_cookie = scookie;
		mmuseg->me_pmap = pmap_kernel();
		mmuseg->me_vreg = vr;
		mmuseg->me_vseg = vs % NSEGRG;
		sp = &rp->rg_segmap[vs % NSEGRG];
		sp->sg_pmeg = scookie;
		npte = ++scookie < zseg ? NPTESG : lastpage;
		sp->sg_npte = npte;
		sp->sg_nwired = npte;
		pmap_kernel()->pm_stats.resident_count += npte;
		rp->rg_nsegmap += 1;
		for (i = 0; i < npte; i++)
			sp->sg_pte[i] = getpte4(p + i * NBPG) | PG_WIRED;
		mmuseg++;
		vs++;
		if (scookie < zseg) {
			p += NBPSG;
			continue;
		}

		/*
		 * Unmap the pages, if any, that are not part of
		 * the final segment.
		 */
		for (p += npte << PGSHIFT; npte < NPTESG; npte++, p += NBPG)
			setpte4(p, 0);

#if defined(SUN4_MMU3L)
		if (HASSUN4_MMU3L) {
			/*
			 * Unmap the segments, if any, that are not part of
			 * the final region.
			 */
			for (i = rp->rg_nsegmap; i < NSEGRG; i++, p += NBPSG)
				setsegmap(p, seginval);

			/*
			 * Unmap any kernel regions that we aren't using.
			 */
			for (i = 0; i < nctx; i++) {
				setcontext4(i);
				for (va = p;
				     va < (OPENPROM_STARTVADDR & ~(NBPRG - 1));
				     va += NBPRG)
					setregmap(va, reginval);
			}

		} else
#endif
		{
			/*
			 * Unmap any kernel segments that we aren't using.
			 */
			for (i = 0; i < nctx; i++) {
				setcontext4(i);
				for (va = p;
				     va < (OPENPROM_STARTVADDR & ~(NBPSG - 1));
				     va += NBPSG)
					setsegmap(va, seginval);
			}
		}
		break;
	}

#if defined(SUN4_MMU3L)
	if (HASSUN4_MMU3L)
		for (rcookie = 0; rcookie < nregion; rcookie++) {
			if (rcookie == startrcookie)
				/* Kernel must fit in one region! */
				rcookie++;
			mmureg = &mmuregions[rcookie];
			mmureg->me_cookie = rcookie;
			MMUQ_INSERT_TAIL(&region_freelist, mmureg, me_list);
#ifdef DIAGNOSTIC
			mmuseg->me_statp = NULL;
#endif
		}
#endif /* SUN4_MMU3L */

	for (scookie = 0; scookie < nsegment; scookie++) {
		if (scookie == startscookie)
			/* Skip static kernel image */
			scookie = zseg;
		mmuseg = &mmusegments[scookie];
		mmuseg->me_cookie = scookie;
		MMUQ_INSERT_TAIL(&segm_freelist, mmuseg, me_list);
		pmap_stats.ps_npmeg_free++;
#ifdef DIAGNOSTIC
		mmuseg->me_statp = NULL;
#endif
	}

	/* Erase all spurious user-space segmaps */
	for (i = 1; i < ncontext; i++) {
		setcontext4(i);
		if (HASSUN4_MMU3L)
			for (p = 0, j = NUREG; --j >= 0; p += NBPRG)
				setregmap(p, reginval);
		else
			for (p = 0, vr = 0; vr < NUREG; vr++) {
				if (VA_INHOLE(p)) {
					p = MMU_HOLE_END;
					vr = VA_VREG(p);
				}
				for (j = NSEGRG; --j >= 0; p += NBPSG)
					setsegmap(p, seginval);
			}
	}
	setcontext4(0);

	/*
	 * write protect & encache kernel text;
	 * set red zone at kernel base;
	 * enable cache on message buffer and cpuinfo.
	 */
	{
		extern char etext[];

		/* Enable cache on message buffer and cpuinfo */
		for (p = KERNBASE; p < (vaddr_t)trapbase; p += NBPG)
			setpte4(p, getpte4(p) & ~PG_NC);

		/* Enable cache and write protext kernel text */
		for (p = (vaddr_t)trapbase; p < (vaddr_t)etext; p += NBPG)
			setpte4(p, getpte4(p) & ~(PG_NC|PG_W));

		/*
		 * Unmap the `etext gap'; it'll be made available
		 * to the VM manager.
		 */
		for (p = etext_gap_start; p < etext_gap_end; p += NBPG) {
			rp = &pmap_kernel()->pm_regmap[VA_VREG(p)];
			sp = &rp->rg_segmap[VA_VSEG(p)];
			sp->sg_nwired--;
			sp->sg_npte--;
			pmap_kernel()->pm_stats.resident_count--;
			sp->sg_pte[VA_VPG(p)] = 0;
			setpte4(p, 0);
		}

		/* Enable cache on data & bss */
		for (p = etext_gap_end; p < virtual_avail; p += NBPG)
			setpte4(p, getpte4(p) & ~PG_NC);

	}
}
#endif

#if defined(SUN4M) || defined(SUN4D)	/* SRMMU version of pmap_bootstrap */
/*
 * Bootstrap the system enough to run with VM enabled on a sun4m machine.
 *
 * Switches from ROM to kernel page tables, and sets up initial mappings.
 */
static void
pmap_bootstrap4m(void *top)
{
	int i, j;
	vaddr_t p, q;
	union ctxinfo *ci;
	int reg, seg;
	unsigned int ctxtblsize;
	vaddr_t pagetables_start, pagetables_end;
	paddr_t pagetables_start_pa;
	extern char etext[];
	extern char kernel_text[];
	vaddr_t va;
#ifdef MULTIPROCESSOR
	vsize_t off;
	struct vm_page *pg;
#endif

	/*
	 * Compute `va2pa_offset'.
	 * Use `kernel_text' to probe the MMU translation since
	 * the pages at KERNBASE might not be mapped.
	 */
	va2pa_offset = (vaddr_t)kernel_text - VA2PA(kernel_text);

	ncontext = cpuinfo.mmu_ncontext;

#if defined(SUN4) || defined(SUN4C) /* setup SRMMU fn. ptrs for dual-arch
				       kernel */
	pmap_clear_modify_p 	=	pmap_clear_modify4m;
	pmap_clear_reference_p 	= 	pmap_clear_reference4m;
	pmap_enter_p 		=	pmap_enter4m;
	pmap_extract_p 		=	pmap_extract4m;
	pmap_is_modified_p 	=	pmap_is_modified4m;
	pmap_is_referenced_p	=	pmap_is_referenced4m;
	pmap_kenter_pa_p 	=	pmap_kenter_pa4m;
	pmap_kremove_p	 	=	pmap_kremove4m;
	pmap_kprotect_p	 	=	pmap_kprotect4m;
	pmap_page_protect_p	=	pmap_page_protect4m;
	pmap_protect_p		=	pmap_protect4m;
	pmap_rmk_p		=	pmap_rmk4m;
	pmap_rmu_p		=	pmap_rmu4m;
#endif /* defined SUN4/SUN4C */

	/*
	 * p points to top of kernel mem
	 */
	p = (vaddr_t)top;

	/*
	 * Intialize the kernel pmap.
	 */
	/* kernel_pmap_store.pm_ctxnum = 0; */
	kernel_pmap_store.pm_refcount = 1;

	/*
	 * Set up pm_regmap for kernel to point NUREG *below* the beginning
	 * of kernel regmap storage. Since the kernel only uses regions
	 * above NUREG, we save storage space and can index kernel and
	 * user regions in the same way.
	 */
	kernel_pmap_store.pm_regmap = &kernel_regmap_store[-NUREG];
	bzero(kernel_regmap_store, NKREG * sizeof(struct regmap));
	bzero(kernel_segmap_store, NKREG * NSEGRG * sizeof(struct segmap));
	for (i = NKREG; --i >= 0;) {
		kernel_regmap_store[i].rg_segmap =
			&kernel_segmap_store[i * NSEGRG];
		kernel_regmap_store[i].rg_seg_ptps = NULL;
		for (j = NSEGRG; --j >= 0;)
			kernel_segmap_store[i * NSEGRG + j].sg_pte = NULL;
	}

	/* Allocate kernel region pointer tables */
	pmap_kernel()->pm_reg_ptps = (int **)(q = p);
	p += sparc_ncpus * sizeof(int **);
	bzero((void *)q, (u_int)p - (u_int)q);

	pmap_kernel()->pm_reg_ptps_pa = (int *)(q = p);
	p += sparc_ncpus * sizeof(int *);
	bzero((void *)q, (u_int)p - (u_int)q);

	/* Allocate context administration */
	pmap_kernel()->pm_ctx = ctxinfo = ci = (union ctxinfo *)p;
	p += ncontext * sizeof *ci;
	bzero((void *)ci, (u_int)p - (u_int)ci);

	/*
	 * Set up the `constants' for the call to vm_init()
	 * in main().  All pages beginning at p (rounded up to
	 * the next whole page) and continuing through the number
	 * of available pages are free.
	 */
	p = (p + NBPG - 1) & ~PGOFSET;

	/*
	 * Reserve memory for MMU pagetables. Some of these have severe
	 * alignment restrictions. We allocate in a sequence that
	 * minimizes alignment gaps.
	 */

	pagetables_start = p;
	pagetables_start_pa = PMAP_BOOTSTRAP_VA2PA(p);

	/*
	 * Allocate context table.
	 * To keep supersparc happy, minimum aligment is on a 4K boundary.
	 */
	ctxtblsize = max(ncontext,1024) * sizeof(int);
	cpuinfo.ctx_tbl = (int *)roundup((u_int)p, ctxtblsize);
	cpuinfo.ctx_tbl_pa = PMAP_BOOTSTRAP_VA2PA(cpuinfo.ctx_tbl);
	p = (u_int)cpuinfo.ctx_tbl + ctxtblsize;

#if defined(MULTIPROCESSOR)
	/*
	 * Make sure all smp_tlb_flush*() routines for kernel pmap are
	 * broadcast to all CPU's.
	 */
	pmap_kernel()->pm_cpuset = CPUSET_ALL;
#endif

	/*
	 * Reserve memory for segment and page tables needed to map the entire
	 * kernel. This takes (2K + NKREG * 16K) of space, but unfortunately
	 * is necessary since pmap_enter() *must* be able to enter a kernel
	 * mapping without delay.
	 */
	p = (vaddr_t) roundup(p, SRMMU_L1SIZE * sizeof(u_int));
	qzero((void *)p, SRMMU_L1SIZE * sizeof(u_int));
	kernel_regtable_store = (u_int *)p;
	p += SRMMU_L1SIZE * sizeof(u_int);

	p = (vaddr_t) roundup(p, SRMMU_L2SIZE * sizeof(u_int));
	qzero((void *)p, (SRMMU_L2SIZE * sizeof(u_int)) * NKREG);
	kernel_segtable_store = (u_int *)p;
	p += (SRMMU_L2SIZE * sizeof(u_int)) * NKREG;

	p = (vaddr_t) roundup(p, SRMMU_L3SIZE * sizeof(u_int));
	/* zero it: all will be SRMMU_TEINVALID */
	qzero((void *)p, ((SRMMU_L3SIZE * sizeof(u_int)) * NSEGRG) * NKREG);
	kernel_pagtable_store = (u_int *)p;
	p += ((SRMMU_L3SIZE * sizeof(u_int)) * NSEGRG) * NKREG;

	/* Round to next page and mark end of pre-wired kernel space */
	p = (p + NBPG - 1) & ~PGOFSET;
	pagetables_end = p;

	avail_start = PMAP_BOOTSTRAP_VA2PA(p);

	/*
	 * Now wire the region and segment tables of the kernel map.
	 */
	pmap_kernel()->pm_reg_ptps[0] = (int *) kernel_regtable_store;
	pmap_kernel()->pm_reg_ptps_pa[0] =
		 PMAP_BOOTSTRAP_VA2PA(kernel_regtable_store);

	/* Install L1 table in context 0 */
	setpgt4m(&cpuinfo.ctx_tbl[0],
	    (pmap_kernel()->pm_reg_ptps_pa[0] >> SRMMU_PPNPASHIFT) | SRMMU_TEPTD);

	for (reg = 0; reg < NKREG; reg++) {
		struct regmap *rp;
		void *kphyssegtbl;

		/*
		 * Entering new region; install & build segtbl
		 */

		rp = &pmap_kernel()->pm_regmap[reg + VA_VREG(KERNBASE)];

		kphyssegtbl = (void *)
		    &kernel_segtable_store[reg * SRMMU_L2SIZE];

		setpgt4m(&pmap_kernel()->pm_reg_ptps[0][reg + VA_VREG(KERNBASE)],
		    (PMAP_BOOTSTRAP_VA2PA(kphyssegtbl) >> SRMMU_PPNPASHIFT) |
		    SRMMU_TEPTD);

		rp->rg_seg_ptps = (int *)kphyssegtbl;

		for (seg = 0; seg < NSEGRG; seg++) {
			struct segmap *sp;
			void *kphyspagtbl;

			rp->rg_nsegmap++;

			sp = &rp->rg_segmap[seg];
			kphyspagtbl = (void *)
			    &kernel_pagtable_store
				[((reg * NSEGRG) + seg) * SRMMU_L3SIZE];

			setpgt4m(&rp->rg_seg_ptps[seg],
				 (PMAP_BOOTSTRAP_VA2PA(kphyspagtbl) >> SRMMU_PPNPASHIFT) |
				 SRMMU_TEPTD);
			sp->sg_pte = (int *) kphyspagtbl;
		}
	}

	/*
	 * Preserve the monitor ROM's reserved VM region, so that
	 * we can use L1-A or the monitor's debugger.
	 */
	mmu_reservemon4m(&kernel_pmap_store);

	/*
	 * Reserve virtual address space for two mappable MD pages
	 * for pmap_zero_page and pmap_copy_page, one MI page
	 * for /dev/mem, and some more for dumpsys().
	 */
	q = p;
	cpuinfo.vpage[0] = (void *)p, p += NBPG;
	cpuinfo.vpage[1] = (void *)p, p += NBPG;
	vmmap = (void *)p, p += NBPG;
	p = (vaddr_t)reserve_dumppages((void *)p);

	/* Find PTE locations of vpage[] to optimize zero_fill() et.al. */
	for (i = 0; i < 2; i++) {
		struct regmap *rp;
		struct segmap *sp;
		rp = &pmap_kernel()->pm_regmap[VA_VREG(cpuinfo.vpage[i])];
		sp = &rp->rg_segmap[VA_VSEG(cpuinfo.vpage[i])];
		cpuinfo.vpage_pte[i] =
			&sp->sg_pte[VA_SUN4M_VPG(cpuinfo.vpage[i])];
	}

#if !(defined(PROM_AT_F0) || defined(MSIIEP))
	virtual_avail = p;
#elif defined(MSIIEP)
	virtual_avail = (vaddr_t)0xf0800000; /* Krups */
#else
	virtual_avail = (vaddr_t)0xf0080000; /* Mr.Coffee/OFW */
#endif
	virtual_end = VM_MAX_KERNEL_ADDRESS;

	p = q;			/* retract to first free phys */

	/*
	 * Set up the ctxinfo structures (freelist of contexts)
	 */
	ci->c_pmap = pmap_kernel();
	ctx_freelist = ci + 1;
	for (i = 1; i < ncontext; i++) {
		ci++;
		ci->c_nextfree = ci + 1;
	}
	ci->c_nextfree = NULL;
	ctx_kick = 0;
	ctx_kickdir = -1;

	/*
	 * Now map the kernel into our new set of page tables, then
	 * (finally) switch over to our running page tables.
	 * We map from KERNBASE to p into context 0's page tables (and
	 * the kernel pmap).
	 */
#ifdef DEBUG			/* Sanity checks */
	if (p % NBPG != 0)
		panic("pmap_bootstrap4m: p misaligned?!?");
	if (KERNBASE % NBPRG != 0)
		panic("pmap_bootstrap4m: KERNBASE not region-aligned");
#endif

	for (q = KERNBASE; q < p; q += NBPG) {
		struct regmap *rp;
		struct segmap *sp;
		int pte, *ptep;

		/*
		 * Now install entry for current page.
		 */
		rp = &pmap_kernel()->pm_regmap[VA_VREG(q)];
		sp = &rp->rg_segmap[VA_VSEG(q)];
		ptep = &sp->sg_pte[VA_VPG(q)];

		/*
		 * Unmap the `etext gap'; it'll be made available
		 * to the VM manager.
		 */
		if (q >= etext_gap_start && q < etext_gap_end) {
			setpgt4m(ptep, 0);
			continue;
		}

		pte = PMAP_BOOTSTRAP_VA2PA(q) >> SRMMU_PPNPASHIFT;
		pte |= PPROT_N_RX | SRMMU_TEPTE;

		/* Deal with the cacheable bit for pagetable memory */
		if ((cpuinfo.flags & CPUFLG_CACHEPAGETABLES) != 0 ||
		    q < pagetables_start || q >= pagetables_end)
			pte |= SRMMU_PG_C;

		/* write-protect kernel text */
		if (q < (vaddr_t)trapbase || q >= (vaddr_t)etext)
			pte |= PPROT_WRITE;

		setpgt4m(ptep, pte);
		pmap_kernel()->pm_stats.resident_count++;
	}

	if ((cpuinfo.flags & CPUFLG_CACHEPAGETABLES) == 0) {
		/*
		 * The page tables have been setup. Since we're still
		 * running on the PROM's memory map, the memory we
		 * allocated for our page tables might still be cached.
		 * Flush it now, and don't touch it again until we
		 * switch to our own tables (will be done immediately below).
		 */
		int size = pagetables_end - pagetables_start;
		if (CACHEINFO.c_vactype != VAC_NONE) {
			va = (vaddr_t)pagetables_start;
			while (size > 0) {
				cache_flush_page(va, 0);
				va += NBPG;
				size -= NBPG;
			}
		} else if (cpuinfo.pcache_flush_page != NULL) {
			paddr_t pa = pagetables_start_pa;
			while (size > 0) {
				pcache_flush_page(pa, 0);
				pa += NBPG;
				size -= NBPG;
			}
		}
	}

	/*
	 * Now switch to kernel pagetables (finally!)
	 */
	mmu_install_tables(&cpuinfo);

#ifdef MULTIPROCESSOR
	/* Allocate VA for all the cpu_info structurs */
	cpus = (union cpu_info_pg*)uvm_km_alloc(kernel_map,
	    sizeof cpus[sparc_ncpus], 32*1024, UVM_KMF_VAONLY);
	/*
	 * Add an alias mapping for the CPUINFO_VA allocation created
	 * early during bootstrap for the first CPU
	 */
	off = 0;
	for (va = (vaddr_t)&cpus[0].ci;
	     off < sizeof(struct cpu_info);
	     va += NBPG, off += NBPG) {
		paddr_t pa = PMAP_BOOTSTRAP_VA2PA(CPUINFO_VA + off);
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
	}
	/*
	 * Now allocate memory for all other CPUs cpu_info and map
	 * it into the coresponding space in the cpus array. We will
	 * later duplicate the mapping into CPUINFO_VA.
	 */
	for (i = 1; i < sparc_ncpus; i++) {
		off = 0;
		for (va = (vaddr_t)&cpus[i].ci;
		     off < sizeof(struct cpu_info);
		     va += NBPG, off += NBPG) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			paddr_t pa = VM_PAGE_TO_PHYS(pg);
			pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
		}
	}

	/* clear new cpu infos */
	prom_printf("clearing other cpus cpu info\n");
	memset(&cpus[1].ci, 0, (sparc_ncpus-1)*sizeof(union cpu_info_pg));

	/* setup self refernces, and cpu "cpuinfo" */
	prom_printf("setting cpus self reference and mapping\n");
	for (i = 0; i < sparc_ncpus; i++) {

		prom_printf("going to set cpu%d ci_self address: %p\n", i, &cpus[i].ci);
		cpus[i].ci.ci_self = &cpus[i].ci;

		/* mapped above. */
		if (i == 0)
			continue;

		off = 0;
		for (va = (vaddr_t)&cpus[i].ci;
		     off < sizeof(struct cpu_info);
		     va += NBPG, off += NBPG) {
			paddr_t pa = PMAP_BOOTSTRAP_VA2PA(va + off);
			prom_printf("going to pmap_kenter_pa(va=%p, pa=%p)\n", va, pa);
			pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
		}
	}
#endif
	pmap_update(pmap_kernel());
}

static u_long prom_ctxreg;

void
mmu_install_tables(struct cpu_info *sc)
{

#ifdef DEBUG
	prom_printf("pmap_bootstrap: installing kernel page tables...");
#endif
	setcontext4m(0);	/* paranoia? %%%: Make 0x3 a define! below */

	/* Enable MMU tablewalk caching, flush TLB */
	if (sc->mmu_enable != 0)
		sc->mmu_enable();

	tlb_flush_all_real();
	prom_ctxreg = lda(SRMMU_CXTPTR, ASI_SRMMU);

	sta(SRMMU_CXTPTR, ASI_SRMMU,
		(sc->ctx_tbl_pa >> SRMMU_PPNPASHIFT) & ~0x3);

	tlb_flush_all_real();

#ifdef DEBUG
	prom_printf("done.\n");
#endif
}

void srmmu_restore_prom_ctx(void);

void
srmmu_restore_prom_ctx(void)
{

	tlb_flush_all();
	sta(SRMMU_CXTPTR, ASI_SRMMU, prom_ctxreg);
	tlb_flush_all();
}
#endif /* SUN4M || SUN4D */

#if defined(MULTIPROCESSOR)
/*
 * Allocate per-CPU page tables. One region, segment and page table
 * is needed to map CPUINFO_VA to different physical addresses on
 * each CPU. Since the kernel region and segment tables are all
 * pre-wired (in bootstrap() above) and we also assume that the
 * first segment (256K) of kernel space is fully populated with
 * pages from the start, these per-CPU tables will never need
 * to be updated when mapping kernel virtual memory.
 *
 * Note: this routine is called in the context of the boot CPU
 * during autoconfig.
 */
void
pmap_alloc_cpu(struct cpu_info *sc)
{
#if defined(SUN4M) || defined(SUN4D)	/* Only implemented for SUN4M/D */
	vaddr_t va;
	paddr_t pa;
	paddr_t alignment;
	u_int *ctxtable, *regtable, *segtable, *pagtable;
	u_int *ctxtable_pa, *regtable_pa, *segtable_pa, *pagtable_pa;
	psize_t ctxsize, size;
	int vr, vs, vpg;
	struct regmap *rp;
	struct segmap *sp;
	struct pglist mlist;
	int cachebit;
	int pagesz = NBPG;
	int i;

	cachebit = (cpuinfo.flags & CPUFLG_CACHEPAGETABLES) != 0;

	/*
	 * Allocate properly aligned and contiguous physically memory
	 * for the PTE tables.
	 */
	ctxsize = (sc->mmu_ncontext * sizeof(int) + pagesz - 1) & -pagesz;
	alignment = ctxsize;

	/* The region, segment and page table we need fit in one page */
	size = ctxsize + pagesz;

	if (uvm_pglistalloc(size, vm_first_phys, vm_first_phys+vm_num_phys,
			    alignment, 0, &mlist, 1, 0) != 0)
		panic("pmap_alloc_cpu: no memory");

	pa = VM_PAGE_TO_PHYS(TAILQ_FIRST(&mlist));

	/* Allocate virtual memory */
	va = uvm_km_alloc(kernel_map, size, 0, UVM_KMF_VAONLY);
	if (va == 0)
		panic("pmap_alloc_cpu: no memory");

	/*
	 * Layout the page tables in our chunk of memory
	 */
	ctxtable = (u_int *)va;
	regtable = (u_int *)(va + ctxsize);
	segtable = regtable + SRMMU_L1SIZE;
	pagtable = segtable + SRMMU_L2SIZE;

	ctxtable_pa = (u_int *)pa;
	regtable_pa = (u_int *)(pa + ctxsize);
	segtable_pa = regtable_pa + SRMMU_L1SIZE;
	pagtable_pa = segtable_pa + SRMMU_L2SIZE;

	/* Map the pages */
	while (size != 0) {
		pmap_kenter_pa(va, pa | (cachebit ? 0 : PMAP_NC),
		    VM_PROT_READ | VM_PROT_WRITE);
		va += pagesz;
		pa += pagesz;
		size -= pagesz;
	}
	pmap_update(pmap_kernel());

	/*
	 * Store the region table pointer (and its corresponding physical
	 * address) in the CPU's slot in the kernel pmap region table
	 * pointer table.
	 */
	pmap_kernel()->pm_reg_ptps[sc->ci_cpuid] = regtable;
	pmap_kernel()->pm_reg_ptps_pa[sc->ci_cpuid] = (paddr_t)regtable_pa;

	vr = VA_VREG(CPUINFO_VA);
	vs = VA_VSEG(CPUINFO_VA);
	vpg = VA_VPG(CPUINFO_VA);
	rp = &pmap_kernel()->pm_regmap[vr];
	sp = &rp->rg_segmap[vs];

	/*
	 * Copy page tables from CPU #0, then modify entry for CPUINFO_VA
	 * so that it points at the per-CPU pages.
	 */
	qcopy(pmap_kernel()->pm_reg_ptps[0], regtable,
		SRMMU_L1SIZE * sizeof(int));
	qcopy(rp->rg_seg_ptps, segtable, SRMMU_L2SIZE * sizeof(int));
	qcopy(sp->sg_pte, pagtable, SRMMU_L3SIZE * sizeof(int));

	setpgt4m(&ctxtable[0],
		 ((u_long)regtable_pa >> SRMMU_PPNPASHIFT) | SRMMU_TEPTD);
	setpgt4m(&regtable[vr],
		 ((u_long)segtable_pa >> SRMMU_PPNPASHIFT) | SRMMU_TEPTD);
	setpgt4m(&segtable[vs],
		 ((u_long)pagtable_pa >> SRMMU_PPNPASHIFT) | SRMMU_TEPTD);
	setpgt4m(&pagtable[vpg],
		(VA2PA((void *)sc) >> SRMMU_PPNPASHIFT) |
		(SRMMU_TEPTE | PPROT_N_RWX | SRMMU_PG_C));

	/* Install this CPU's context table */
	sc->ctx_tbl = ctxtable;
	sc->ctx_tbl_pa = (paddr_t)ctxtable_pa;

	/* Pre-compute this CPU's vpage[] PTEs */
	for (i = 0; i < 2; i++) {
		rp = &pmap_kernel()->pm_regmap[VA_VREG(sc->vpage[i])];
		sp = &rp->rg_segmap[VA_VSEG(sc->vpage[i])];
		sc->vpage_pte[i] = &sp->sg_pte[VA_SUN4M_VPG(sc->vpage[i])];
	}
#endif /* SUN4M || SUN4D */
}
#endif /* MULTIPROCESSOR */


void
pmap_init(void)
{
	u_int sz;

	if (PAGE_SIZE != NBPG)
		panic("pmap_init: PAGE_SIZE!=NBPG");

	vm_num_phys = vm_last_phys - vm_first_phys;

	/* Setup a pool for additional pvlist structures */
	pool_init(&pv_pool, sizeof(struct pvlist), 0, 0, 0, "pvtable", NULL,
	    IPL_NONE);

	/*
	 * Setup a pool for pmap structures.
	 * The pool size includes space for an array of per-CPU
	 * region table pointers & physical addresses
	 */
	sz = ALIGN(sizeof(struct pmap)) +
	     ALIGN(NUREG * sizeof(struct regmap)) +
	     sparc_ncpus * sizeof(int *) +	/* pm_reg_ptps */
	     sparc_ncpus * sizeof(int);		/* pm_reg_ptps_pa */
	pool_cache_bootstrap(&pmap_cache, sz, 0, 0, 0, "pmappl", NULL,
	    IPL_NONE, pmap_pmap_pool_ctor, pmap_pmap_pool_dtor, NULL);

	sz = NSEGRG * sizeof (struct segmap);
	pool_init(&segmap_pool, sz, 0, 0, 0, "segmap", NULL, IPL_NONE);

#if defined(SUN4M) || defined(SUN4D)
	if (CPU_HAS_SRMMU) {
		/*
		 * The SRMMU only ever needs chunks in one of two sizes:
		 * 1024 (for region level tables) and 256 (for segment
		 * and page level tables).
		 */
		sz = SRMMU_L1SIZE * sizeof(int);
		pool_init(&L1_pool, sz, sz, 0, 0, "L1 pagetable",
			  &pgt_page_allocator, IPL_NONE);

		sz = SRMMU_L2SIZE * sizeof(int);
		pool_init(&L23_pool, sz, sz, 0, 0, "L2/L3 pagetable",
			  &pgt_page_allocator, IPL_NONE);
	}
#endif /* SUN4M || SUN4D */
#if defined(SUN4) || defined(SUN4C)
	if (CPU_HAS_SUNMMU) {
		sz = NPTESG * sizeof(int);
		pool_init(&pte_pool, sz, 0, 0, 0, "ptemap", NULL,
		    IPL_NONE);
	}
#endif /* SUN4 || SUN4C */
}


/*
 * Map physical addresses into kernel VM.
 */
vaddr_t
pmap_map(vaddr_t va, paddr_t pa, paddr_t endpa, int prot)
{
	int pgsize = PAGE_SIZE;

	while (pa < endpa) {
		pmap_kenter_pa(va, pa, prot);
		va += pgsize;
		pa += pgsize;
	}
	pmap_update(pmap_kernel());
	return (va);
}

#ifdef DEBUG
/*
 * Check a pmap for spuriously lingering mappings
 */
static inline void
pmap_quiet_check(struct pmap *pm)
{
	int vs, vr;

	if (CPU_HAS_SUNMMU) {
#if defined(SUN4_MMU3L)
		if (TAILQ_FIRST(&pm->pm_reglist))
			panic("pmap_destroy: region list not empty");
#endif
		if (TAILQ_FIRST(&pm->pm_seglist))
			panic("pmap_destroy: segment list not empty");
	}

	for (vr = 0; vr < NUREG; vr++) {
		struct regmap *rp = &pm->pm_regmap[vr];

		if (HASSUN4_MMU3L) {
			if (rp->rg_smeg != reginval)
				printf("pmap_chk: spurious smeg in "
				       "user region %d\n", vr);
		}
		if (CPU_HAS_SRMMU) {
			int n;
#if defined(MULTIPROCESSOR)
			for (n = 0; n < sparc_ncpus; n++)
#else
			n = 0;
#endif
			{
				if (pm->pm_reg_ptps[n][vr] != SRMMU_TEINVALID)
					printf("pmap_chk: spurious PTP in user "
						"region %d on CPU %d\n", vr, n);
			}
		}
		if (rp->rg_nsegmap != 0)
			printf("pmap_chk: %d segments remain in "
				"region %d\n", rp->rg_nsegmap, vr);
		if (rp->rg_segmap != NULL) {
			printf("pmap_chk: segments still "
				"allocated in region %d\n", vr);
			for (vs = 0; vs < NSEGRG; vs++) {
				struct segmap *sp = &rp->rg_segmap[vs];
				if (sp->sg_npte != 0)
					printf("pmap_chk: %d ptes "
					     "remain in segment %d\n",
						sp->sg_npte, vs);
				if (sp->sg_pte != NULL) {
					printf("pmap_chk: ptes still "
					     "allocated in segment %d\n", vs);
				}
				if (CPU_HAS_SUNMMU) {
					if (sp->sg_pmeg != seginval)
						printf("pmap_chk: pm %p(%d,%d) "
						  "spurious soft pmeg %d\n",
						  pm, vr, vs, sp->sg_pmeg);
				}
			}
		}

		/* Check for spurious pmeg entries in the MMU */
		if (pm->pm_ctx == NULL)
			continue;
		if (CPU_HAS_SUNMMU) {
			int ctx;
			if (mmu_has_hole && (vr >= 32 || vr < (256 - 32)))
				continue;
			ctx = getcontext4();
			setcontext4(pm->pm_ctxnum);
			for (vs = 0; vs < NSEGRG; vs++) {
				vaddr_t va = VSTOVA(vr,vs);
				int pmeg = getsegmap(va);
				if (pmeg != seginval)
					printf("pmap_chk: pm %p(%d,%d:%x): "
						"spurious pmeg %d\n",
						pm, vr, vs, (u_int)va, pmeg);
			}
			setcontext4(ctx);
		}
	}
	if (pm->pm_stats.resident_count) {
		printf("pmap_chk: res count %ld\n",
		       pm->pm_stats.resident_count);
	}
	if (pm->pm_stats.wired_count) {
		printf("pmap_chk: wired count %ld\n",
		       pm->pm_stats.wired_count);
	}
}
#endif /* DEBUG */

int
pmap_pmap_pool_ctor(void *arg, void *object, int flags)
{
	struct pmap *pm = object;
	u_long addr;

	bzero(pm, sizeof *pm);

	/*
	 * `pmap_pool' entries include space for the per-CPU
	 * region table pointer arrays.
	 */
	addr = (u_long)pm + ALIGN(sizeof(struct pmap));
	pm->pm_regmap = (void *)addr;
	addr += ALIGN(NUREG * sizeof(struct regmap));
	pm->pm_reg_ptps = (int **)addr;
	addr += sparc_ncpus * sizeof(int *);
	pm->pm_reg_ptps_pa = (int *)addr;

	qzero((void *)pm->pm_regmap, NUREG * sizeof(struct regmap));

	/* pm->pm_ctx = NULL; // already done */

	if (CPU_HAS_SUNMMU) {
		TAILQ_INIT(&pm->pm_seglist);
#if defined(SUN4_MMU3L)
		TAILQ_INIT(&pm->pm_reglist);
		if (HASSUN4_MMU3L) {
			int i;
			for (i = NUREG; --i >= 0;)
				pm->pm_regmap[i].rg_smeg = reginval;
		}
#endif
	}
#if defined(SUN4M) || defined(SUN4D)
	else {
		int i, n;

		/*
		 * We must allocate and initialize hardware-readable (MMU)
		 * pagetables. We must also map the kernel regions into this
		 * pmap's pagetables, so that we can access the kernel from
		 * this user context.
		 */
#if defined(MULTIPROCESSOR)
		for (n = 0; n < sparc_ncpus; n++)
#else
		n = 0;
#endif
		{
			int *upt, *kpt;

			upt = pool_get(&L1_pool, flags);
			pm->pm_reg_ptps[n] = upt;
			pm->pm_reg_ptps_pa[n] = VA2PA((char *)upt);

			/* Invalidate user space regions */
			for (i = 0; i < NUREG; i++)
				setpgt4m(upt++, SRMMU_TEINVALID);

			/* Copy kernel regions */
			kpt = &pmap_kernel()->pm_reg_ptps[n][VA_VREG(KERNBASE)];
			for (i = 0; i < NKREG; i++)
				setpgt4m(upt++, kpt[i]);
		}
	}
#endif /* SUN4M || SUN4D */

	/* XXX - a peculiar place to do this, but we can't do it in pmap_init
	 * and here at least it's off the beaten code track.
	 */
{static int x; if (x == 0) pool_setlowat(&pv_pool, 512), x = 1; }

	return (0);
}

void
pmap_pmap_pool_dtor(void *arg, void *object)
{
	struct pmap *pm = object;
	union ctxinfo *c;
	int s = splvm();	/* paranoia */

#ifdef DEBUG
	if (pmapdebug & PDB_DESTROY)
		printf("pmap_pmap_pool_dtor(%p)\n", pm);
#endif

	if ((c = pm->pm_ctx) != NULL) {
		ctx_free(pm);
	}

#if defined(SUN4M) || defined(SUN4D)
	if (CPU_HAS_SRMMU) {
		int n;

#if defined(MULTIPROCESSOR)
		for (n = 0; n < sparc_ncpus; n++)
#else
		n = 0;
#endif
		{
			int *pt = pm->pm_reg_ptps[n];
			pm->pm_reg_ptps[n] = NULL;
			pm->pm_reg_ptps_pa[n] = 0;
			pool_put(&L1_pool, pt);
		}
	}
#endif /* SUN4M || SUN4D */
	splx(s);
}

/*
 * Create and return a physical map.
 */
struct pmap *
pmap_create(void)
{
	struct pmap *pm;

	pm = pool_cache_get(&pmap_cache, PR_WAITOK);

	/*
	 * Reset fields that are not preserved in the pmap cache pool.
	 */
	pm->pm_refcount = 1;
#if defined(MULTIPROCESSOR)
	/* reset active CPU set */
	pm->pm_cpuset = 0;
#endif
	if (CPU_HAS_SUNMMU) {
		/* reset the region gap */
		pm->pm_gap_start = 0;
		pm->pm_gap_end = VA_VREG(VM_MAXUSER_ADDRESS);
	}

#ifdef DEBUG
	if (pmapdebug & PDB_CREATE)
		printf("pmap_create[%d]: created %p\n", cpu_number(), pm);
	pmap_quiet_check(pm);
#endif
	return (pm);
}

/*
 * Retire the given pmap from service.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(struct pmap *pm)
{

#ifdef DEBUG
	if (pmapdebug & PDB_DESTROY)
		printf("pmap_destroy[%d](%p)\n", cpu_number(), pm);
#endif
	if (atomic_dec_uint_nv(&pm->pm_refcount) == 0) {
#ifdef DEBUG
		pmap_quiet_check(pm);
#endif
		pool_cache_put(&pmap_cache, pm);
	}
}

/*
 * Add a reference to the given pmap.
 */
void
pmap_reference(struct pmap *pm)
{

	atomic_inc_uint(&pm->pm_refcount);
}

#if defined(SUN4) || defined(SUN4C)
/*
 * helper to deallocate level 2 & 3 page tables.
 */
static void
pgt_lvl23_remove4_4c(struct pmap *pm, struct regmap *rp, struct segmap *sp,
		     int vr, int vs)
{
	vaddr_t va, tva;
	int i, pmeg;

	va = VSTOVA(vr,vs);
	if ((pmeg = sp->sg_pmeg) != seginval) {
		if (CTX_USABLE(pm,rp)) {
			setcontext4(pm->pm_ctxnum);
			setsegmap(va, seginval);
		} else {
			/* no context, use context 0 */
			setcontext4(0);
			if (HASSUN4_MMU3L && rp->rg_smeg != reginval) {
				setregmap(0, rp->rg_smeg);
				tva = vs << SGSHIFT;
				setsegmap(tva, seginval);
			}
		}
		if (!HASSUN4_MMU3L) {
			if (pm == pmap_kernel()) {
				/* Unmap segment from all contexts */
				for (i = ncontext; --i >= 0;) {
					setcontext4(i);
					setsegmap(va, seginval);
				}
			}
		}
		me_free(pm, pmeg);
		sp->sg_pmeg = seginval;
	}
	/* Free software tables for non-kernel maps */
	if (pm != pmap_kernel()) {
		pool_put(&pte_pool, sp->sg_pte);
		sp->sg_pte = NULL;
	}

	if (rp->rg_nsegmap <= 0)
		panic("pgt_rm: pm %p: nsegmap = %d\n", pm, rp->rg_nsegmap);

	if (--rp->rg_nsegmap == 0) {
#if defined(SUN4_MMU3L)
		if (HASSUN4_MMU3L) {
			if (rp->rg_smeg != reginval) {
				if (pm == pmap_kernel()) {
					/* Unmap from all contexts */
					for (i = ncontext; --i >= 0;) {
						setcontext4(i);
						setregmap(va, reginval);
					}
				} else if (pm->pm_ctx) {
					setcontext4(pm->pm_ctxnum);
					setregmap(va, reginval);
				}

				/* Release MMU resource */
				region_free(pm, rp->rg_smeg);
				rp->rg_smeg = reginval;
			}
		}
#endif /* SUN4_MMU3L */
		/* Free software tables for non-kernel maps */
		if (pm != pmap_kernel()) {
			GAP_WIDEN(pm,vr);
			pool_put(&segmap_pool, rp->rg_segmap);
			rp->rg_segmap = NULL;
		}
	}
}
#endif /* SUN4 || SUN4C */

#if defined(SUN4M) || defined(SUN4D)
/*
 * SRMMU helper to deallocate level 2 & 3 page tables.
 */
static void
pgt_lvl23_remove4m(struct pmap *pm, struct regmap *rp, struct segmap *sp,
    int vr, int vs)
{

	/* Invalidate level 2 PTP entry */
	if (pm->pm_ctx)
		tlb_flush_segment(VSTOVA(vr,vs), pm->pm_ctxnum,
				  PMAP_CPUSET(pm));
	setpgt4m(&rp->rg_seg_ptps[vs], SRMMU_TEINVALID);
	pool_put(&L23_pool, sp->sg_pte);
	sp->sg_pte = NULL;

	/* If region is now empty, remove level 2 pagetable as well */
	if (--rp->rg_nsegmap == 0) {
		int n = 0;
		if (pm->pm_ctx)
			tlb_flush_region(VRTOVA(vr), pm->pm_ctxnum,
					 PMAP_CPUSET(pm));
#ifdef MULTIPROCESSOR
		/* Invalidate level 1 PTP entries on all CPUs */
		for (; n < sparc_ncpus; n++) {
			if ((cpus[n].ci.flags & CPUFLG_HATCHED) == 0)
				continue;
#endif
			setpgt4m(&pm->pm_reg_ptps[n][vr], SRMMU_TEINVALID);
#ifdef MULTIPROCESSOR
		}
#endif

		pool_put(&segmap_pool, rp->rg_segmap);
		rp->rg_segmap = NULL;
		pool_put(&L23_pool, rp->rg_seg_ptps);
	}
}
#endif /* SUN4M || SUN4D */

void
pmap_remove_all(struct pmap *pm)
{
	if (pm->pm_ctx == NULL)
		return;

#if defined(SUN4) || defined(SUN4C)
	if (CPU_HAS_SUNMMU) {
		int ctx = getcontext4();
		setcontext4(pm->pm_ctxnum);
		cache_flush_context(pm->pm_ctxnum);
		setcontext4(ctx);
	}
#endif

#if defined(SUN4M) || defined(SUN4D)
	if (CPU_HAS_SRMMU) {
		cache_flush_context(pm->pm_ctxnum);
	}
#endif

	pm->pm_flags |= PMAP_USERCACHECLEAN;
}

/*
 * Remove the given range of mapping entries.
 * The starting and ending addresses are already rounded to pages.
 * Sheer lunacy: pmap_remove is often asked to remove nonexistent
 * mappings.
 */
void
pmap_remove(struct pmap *pm, vaddr_t va, vaddr_t endva)
{
	vaddr_t nva;
	int vr, vs, s, ctx;
	void (*rm)(struct pmap *, vaddr_t, vaddr_t, int, int);

#ifdef DEBUG
	if (pmapdebug & PDB_REMOVE)
		printf("pmap_remove[%d](%p, 0x%lx, 0x%lx)\n",
			cpu_number(), pm, va, endva);
#endif

	if (!CPU_HAS_SRMMU)
		write_user_windows();

	if (pm == pmap_kernel()) {
		/*
		 * Removing from kernel address space.
		 */
		rm = pmap_rmk;
	} else {
		/*
		 * Removing from user address space.
		 */
		rm = pmap_rmu;
	}

	ctx = getcontext();
	s = splvm();		/* XXX conservative */
	PMAP_LOCK();
	for (; va < endva; va = nva) {
		/* do one virtual segment at a time */
		vr = VA_VREG(va);
		vs = VA_VSEG(va);
		nva = VSTOVA(vr, vs + 1);
		if (nva == 0 || nva > endva)
			nva = endva;
		if (pm->pm_regmap[vr].rg_nsegmap != 0)
			(*rm)(pm, va, nva, vr, vs);
	}
	PMAP_UNLOCK();
	splx(s);
	setcontext(ctx);
}

/*
 * It is the same amount of work to cache_flush_page 16 pages
 * as to cache_flush_segment 1 segment, assuming a 64K cache size
 * and a 4K page size or a 128K cache size and 8K page size.
 */
#define	PMAP_SFL_THRESHOLD	16	/* if > magic, use cache_flush_segment */

/*
 * Remove a range contained within a single segment.
 * These are egregiously complicated routines.
 */

#if defined(SUN4) || defined(SUN4C)

/* remove from kernel */
/*static*/ void
pmap_rmk4_4c(struct pmap *pm, vaddr_t va, vaddr_t endva, int vr, int vs)
{
	int pte, mmupte, *ptep, perpage, npg;
	struct vm_page *pg;
	int nleft, pmeg, inmmu;
	struct regmap *rp;
	struct segmap *sp;

	rp = &pm->pm_regmap[vr];
	sp = &rp->rg_segmap[vs];

	if (rp->rg_nsegmap == 0)
		return;
	if ((nleft = sp->sg_npte) == 0)
		return;
	pmeg = sp->sg_pmeg;
	inmmu = pmeg != seginval;
	ptep = &sp->sg_pte[VA_VPG(va)];

	/* decide how to flush cache */
	npg = (endva - va) >> PGSHIFT;
	if (!inmmu) {
		perpage = 0;
	} else if (npg > PMAP_SFL_THRESHOLD) {
		/* flush the whole segment */
		perpage = 0;
		cache_flush_segment(vr, vs, 0);
	} else {
		/* flush each page individually; some never need flushing */
		perpage = (CACHEINFO.c_vactype != VAC_NONE);
	}

	for (; va < endva; va += NBPG, ptep++) {
		pte = *ptep;
		mmupte = inmmu ? getpte4(va) : 0;
		if ((pte & PG_V) == 0) {
#ifdef DIAGNOSTIC
			if (inmmu && (mmupte & PG_V) != 0)
				printf("rmk: inconsistent ptes va=%lx\n", va);
#endif
			continue;
		}
		if ((pte & PG_TYPE) == PG_OBMEM) {
			/* if cacheable, flush page as needed */
			if (perpage && (mmupte & PG_NC) == 0)
				cache_flush_page(va, 0);
			if ((pg = pvhead4_4c(pte)) != NULL) {
				if (inmmu)
					VM_MDPAGE_PVHEAD(pg)->pv_flags |= MR4_4C(mmupte);
				pv_unlink4_4c(pg, pm, va);
			}
		}
		nleft--;
#ifdef DIAGNOSTIC
		if (nleft < 0)
			panic("pmap_rmk: too many PTEs in segment; "
			      "va 0x%lx; endva 0x%lx", va, endva);
#endif
		if (pte & PG_WIRED) {
			sp->sg_nwired--;
			pm->pm_stats.wired_count--;
		}

		if (inmmu)
			setpte4(va, 0);
		*ptep = 0;
		pm->pm_stats.resident_count--;
	}

#ifdef DIAGNOSTIC
	if (sp->sg_nwired > nleft || sp->sg_nwired < 0)
		panic("pmap_rmk: pm %p, va %lx: nleft=%d, nwired=%d",
			pm, va, nleft, sp->sg_nwired);
#endif
	if ((sp->sg_npte = nleft) == 0)
		pgt_lvl23_remove4_4c(pm, rp, sp, vr, vs);
	else if (sp->sg_nwired == 0) {
		if (sp->sg_pmeg != seginval)
			mmu_pmeg_unlock(sp->sg_pmeg);
	}
}

#endif /* SUN4 || SUN4C */

#if defined(SUN4M) || defined(SUN4D)	/* SRMMU version of pmap_rmk */

/* remove from kernel (4m)*/
/* pm is already locked */
/*static*/ void
pmap_rmk4m(struct pmap *pm, vaddr_t va, vaddr_t endva, int vr, int vs)
{
	int tpte, perpage, npg;
	struct vm_page *pg;
	struct regmap *rp;
	struct segmap *sp;

	rp = &pm->pm_regmap[vr];
	sp = &rp->rg_segmap[vs];
	if (rp->rg_nsegmap == 0)
		return;

	/* decide how to flush cache */
	npg = (endva - va) >> PGSHIFT;
	if (npg > PMAP_SFL_THRESHOLD) {
		/* flush the whole segment */
		perpage = 0;
		if (CACHEINFO.c_vactype != VAC_NONE)
			cache_flush_segment(vr, vs, 0);
	} else {
		/* flush each page individually; some never need flushing */
		perpage = (CACHEINFO.c_vactype != VAC_NONE);
	}
	while (va < endva) {
		tpte = sp->sg_pte[VA_SUN4M_VPG(va)];
		if ((tpte & SRMMU_TETYPE) != SRMMU_TEPTE) {
#ifdef DEBUG
			if ((pmapdebug & PDB_SANITYCHK) &&
			    (getpte4m(va) & SRMMU_TETYPE) == SRMMU_TEPTE)
				panic("pmap_rmk: Spurious kTLB entry for 0x%lx",
				      va);
#endif
			va += NBPG;
			continue;
		}
		if ((tpte & SRMMU_PGTYPE) == PG_SUN4M_OBMEM) {
			/* if cacheable, flush page as needed */
			if (perpage && (tpte & SRMMU_PG_C))
				cache_flush_page(va, 0);
			if ((pg = pvhead4m(tpte)) != NULL) {
				VM_MDPAGE_PVHEAD(pg)->pv_flags |= MR4M(tpte);
				pv_unlink4m(pg, pm, va);
			}
		}
		setpgt4m_va(va, &sp->sg_pte[VA_SUN4M_VPG(va)],
		    SRMMU_TEINVALID, 1, 0, CPUSET_ALL);
		pm->pm_stats.resident_count--;
		va += NBPG;
	}
}
#endif /* SUN4M || SUN4D */

#if defined(SUN4) || defined(SUN4C)

/* remove from user */
/*static*/ void
pmap_rmu4_4c(struct pmap *pm, vaddr_t va, vaddr_t endva, int vr, int vs)
{
	int *ptep, pteva, pte, perpage, npg;
	struct vm_page *pg;
	int nleft, pmeg, inmmu;
	struct regmap *rp;
	struct segmap *sp;

	rp = &pm->pm_regmap[vr];
	if (rp->rg_nsegmap == 0)
		return;
	sp = &rp->rg_segmap[vs];
	if ((nleft = sp->sg_npte) == 0)
		return;
	pmeg = sp->sg_pmeg;
	inmmu = pmeg != seginval;

	/*
	 * PTEs are in MMU.  Invalidate in hardware, update ref &
	 * mod bits, and flush cache if required.
	 */
	if (!inmmu) {
		perpage = 0;
		pteva = 0;
	} else if (CTX_USABLE(pm,rp)) {
		/* process has a context, must flush cache */
		npg = (endva - va) >> PGSHIFT;
		setcontext4(pm->pm_ctxnum);
		if ((pm->pm_flags & PMAP_USERCACHECLEAN) != 0)
			perpage = 0;
		else if (npg > PMAP_SFL_THRESHOLD) {
			perpage = 0; /* flush the whole segment */
			cache_flush_segment(vr, vs, pm->pm_ctxnum);
		} else
			perpage = (CACHEINFO.c_vactype != VAC_NONE);
		pteva = va;
	} else {
		/* no context, use context 0; cache flush unnecessary */
		setcontext4(0);
		if (HASSUN4_MMU3L)
			setregmap(0, tregion);
		/* XXX use per-CPU pteva? */
		setsegmap(0, pmeg);
		pteva = VA_VPG(va) << PGSHIFT;
		perpage = 0;
	}

	ptep = sp->sg_pte + VA_VPG(va);
	for (; va < endva; ptep++, pteva += NBPG, va += NBPG) {
		int mmupte;
		pte = *ptep;
		mmupte = inmmu ? getpte4(pteva) : 0;

		if ((pte & PG_V) == 0) {
#ifdef DIAGNOSTIC
			if (inmmu && (mmupte & PG_V) != 0)
				printf("pmap_rmu: pte=%x, mmupte=%x\n",
					pte, getpte4(pteva));
#endif
			continue;
		}
		if ((pte & PG_TYPE) == PG_OBMEM) {
			/* if cacheable, flush page as needed */
			if (perpage && (mmupte & PG_NC) == 0)
				cache_flush_page(va, pm->pm_ctxnum);
			if ((pg = pvhead4_4c(pte)) != NULL) {
				if (inmmu)
					VM_MDPAGE_PVHEAD(pg)->pv_flags |= MR4_4C(mmupte);
				pv_unlink4_4c(pg, pm, va);
			}
		}
		nleft--;
#ifdef DIAGNOSTIC
		if (nleft < 0)
			panic("pmap_rmu: too many PTEs in segment; "
			     "va 0x%lx; endva 0x%lx", va, endva);
#endif
		if (inmmu)
			setpte4(pteva, 0);

		if (pte & PG_WIRED) {
			sp->sg_nwired--;
			pm->pm_stats.wired_count--;
		}
		*ptep = 0;
		pm->pm_stats.resident_count--;
	}

#ifdef DIAGNOSTIC
	if (sp->sg_nwired > nleft || sp->sg_nwired < 0)
		panic("pmap_rmu: pm %p, va %lx: nleft=%d, nwired=%d",
			pm, va, nleft, sp->sg_nwired);
#endif
	if ((sp->sg_npte = nleft) == 0)
		pgt_lvl23_remove4_4c(pm, rp, sp, vr, vs);
	else if (sp->sg_nwired == 0) {
		if (sp->sg_pmeg != seginval)
			mmu_pmeg_unlock(sp->sg_pmeg);
	}
}

#endif /* SUN4 || SUN4C */

#if defined(SUN4M) || defined(SUN4D)	/* SRMMU version of pmap_rmu */
/* remove from user */
/* Note: pm is already locked */
/*static*/ void
pmap_rmu4m(struct pmap *pm, vaddr_t va, vaddr_t endva, int vr, int vs)
{
	int *pte0, perpage, npg;
	struct vm_page *pg;
	int nleft;
	struct regmap *rp;
	struct segmap *sp;

	rp = &pm->pm_regmap[vr];
	if (rp->rg_nsegmap == 0)
		return;
	sp = &rp->rg_segmap[vs];
	if ((nleft = sp->sg_npte) == 0)
		return;
	pte0 = sp->sg_pte;

	/*
	 * Invalidate PTE in MMU pagetables. Flush cache if necessary.
	 */
	if (pm->pm_ctx && (pm->pm_flags & PMAP_USERCACHECLEAN) == 0) {
		/* process has a context, must flush cache */
		if (CACHEINFO.c_vactype != VAC_NONE) {
			npg = (endva - va) >> PGSHIFT;
			if (npg > PMAP_SFL_THRESHOLD) {
				perpage = 0; /* flush the whole segment */
				cache_flush_segment(vr, vs, pm->pm_ctxnum);
			} else
				perpage = 1;
		} else
			perpage = 0;
	} else {
		/* no context; cache flush unnecessary */
		perpage = 0;
	}
	for (; va < endva; va += NBPG) {
		int tpte;

		tpte = pte0[VA_SUN4M_VPG(va)];

		if ((tpte & SRMMU_TETYPE) != SRMMU_TEPTE) {
#ifdef DEBUG
			if ((pmapdebug & PDB_SANITYCHK) &&
			    pm->pm_ctx &&
			    (getpte4m(va) & SRMMU_TEPTE) == SRMMU_TEPTE)
				panic("pmap_rmu: Spurious uTLB entry for 0x%lx",
				      va);
#endif
			continue;
		}

		if ((tpte & SRMMU_PGTYPE) == PG_SUN4M_OBMEM) {
			/* if cacheable, flush page as needed */
			if (perpage && (tpte & SRMMU_PG_C))
				cache_flush_page(va, pm->pm_ctxnum);
			if ((pg = pvhead4m(tpte)) != NULL) {
				VM_MDPAGE_PVHEAD(pg)->pv_flags |= MR4M(tpte);
				pv_unlink4m(pg, pm, va);
			}
		}
		nleft--;
#ifdef DIAGNOSTIC
		if (nleft < 0)
			panic("pmap_rmu: too many PTEs in segment; "
			      "va 0x%lx; endva 0x%lx", va, endva);
#endif
		setpgt4m_va(va, &pte0[VA_SUN4M_VPG(va)], SRMMU_TEINVALID,
		    pm->pm_ctx != NULL, pm->pm_ctxnum, PMAP_CPUSET(pm));
		pm->pm_stats.resident_count--;
		if (sp->sg_wiremap & (1 << VA_SUN4M_VPG(va))) {
			sp->sg_wiremap &= ~(1 << VA_SUN4M_VPG(va));
			pm->pm_stats.wired_count--;
		}
	}

	/*
	 * If the segment is all gone, and the context is loaded, give
	 * the segment back.
	 */
	if ((sp->sg_npte = nleft) == 0)
		pgt_lvl23_remove4m(pm, rp, sp, vr, vs);
}
#endif /* SUN4M || SUN4D */

/*
 * Lower (make more strict) the protection on the specified
 * physical page.
 *
 * There are only two cases: either the protection is going to 0
 * (in which case we do the dirty work here), or it is going from
 * to read-only (in which case pv_changepte does the trick).
 */

#if defined(SUN4) || defined(SUN4C)
void
pmap_page_protect4_4c(struct vm_page *pg, vm_prot_t prot)
{
	struct pvlist *pv, *npv;
	struct pmap *pm;
	vaddr_t va;
	int vr, vs, pteva, pte, *ptep;
	int flags, nleft, s, ctx;
	struct regmap *rp;
	struct segmap *sp;

#ifdef DEBUG
	if ((pmapdebug & PDB_CHANGEPROT) ||
	    (pmapdebug & PDB_REMOVE && prot == VM_PROT_NONE))
		printf("pmap_page_protect(0x%lx, 0x%x)\n",
			VM_PAGE_TO_PHYS(pg), prot);
#endif
	/*
	 * Skip unmanaged pages, or operations that do not take
	 * away write permission.
	 */
	if (prot & VM_PROT_WRITE)
		return;

	write_user_windows();	/* paranoia */
	if (prot & VM_PROT_READ) {
		pv_changepte4_4c(pg, 0, PG_W);
		return;
	}

	/*
	 * Remove all access to all people talking to this page.
	 * Walk down PV list, removing all mappings.
	 * The logic is much like that for pmap_remove,
	 * but we know we are removing exactly one page.
	 */
	s = splvm();
	pv = VM_MDPAGE_PVHEAD(pg);
	if (pv->pv_pmap == NULL) {
		splx(s);
		return;
	}
	ctx = getcontext4();

	/* This pv head will become empty, so clear caching state flags */
	flags = pv->pv_flags & ~(PV_NC|PV_ANC);

	while (pv != NULL) {
		pm = pv->pv_pmap;
		va = pv->pv_va;
		vr = VA_VREG(va);
		vs = VA_VSEG(va);
		rp = &pm->pm_regmap[vr];
		sp = &rp->rg_segmap[vs];
		if ((nleft = sp->sg_npte) <= 0)
			panic("pmap_page_protect: empty vseg");
		sp->sg_npte = --nleft;
		ptep = &sp->sg_pte[VA_VPG(va)];

		if (*ptep & PG_WIRED) {
			sp->sg_nwired--;
			pm->pm_stats.wired_count--;
		}

		if (sp->sg_pmeg != seginval) {
			/* Update PV flags */
			if (CTX_USABLE(pm,rp)) {
				setcontext4(pm->pm_ctxnum);
				pteva = va;
				cache_flush_page(va, pm->pm_ctxnum);
			} else {
				setcontext4(0);
				/* XXX use per-CPU pteva? */
				if (HASSUN4_MMU3L)
					setregmap(0, tregion);
				setsegmap(0, sp->sg_pmeg);
				pteva = VA_VPG(va) << PGSHIFT;
			}

			pte = getpte4(pteva);
#ifdef DIAGNOSTIC
			if ((pte & PG_V) == 0)
				panic("pmap_page_protect !PG_V: pg %p "
				      "ctx %d, va 0x%lx, pte 0x%x",
				      pg, pm->pm_ctxnum, va, pte);
#endif
			flags |= MR4_4C(pte);

			setpte4(pteva, 0);
#ifdef DIAGNOSTIC
			if (sp->sg_nwired > nleft || sp->sg_nwired < 0)
				panic("pmap_page_protect: pm %p, va %lx: nleft=%d, nwired=%d",
					pm, va, nleft, sp->sg_nwired);
#endif
			if (sp->sg_nwired == 0)
				mmu_pmeg_unlock(sp->sg_pmeg);
		}

		*ptep = 0;
		pm->pm_stats.resident_count--;
		if (nleft == 0)
			pgt_lvl23_remove4_4c(pm, rp, sp, vr, vs);
		npv = pv->pv_next;
		if (pv != VM_MDPAGE_PVHEAD(pg))
			pool_put(&pv_pool, pv);
		pv = npv;
	}

	/* Finally, update pv head */
	VM_MDPAGE_PVHEAD(pg)->pv_pmap = NULL;
	VM_MDPAGE_PVHEAD(pg)->pv_next = NULL;
	VM_MDPAGE_PVHEAD(pg)->pv_flags = flags;
	setcontext4(ctx);
	splx(s);
}

/*
 * Lower (make more strict) the protection on the specified
 * range of this pmap.
 *
 * There are only two cases: either the protection is going to 0
 * (in which case we call pmap_remove to do the dirty work), or
 * it is going from read/write to read-only.  The latter is
 * fairly easy.
 */
void
pmap_protect4_4c(struct pmap *pm, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	int va, nva, vr, vs;
	int s, ctx;
	struct regmap *rp;
	struct segmap *sp;

	if ((prot & VM_PROT_READ) == 0) {
		pmap_remove(pm, sva, eva);
		return;
	}

	write_user_windows();
	ctx = getcontext4();
	s = splvm();
	PMAP_LOCK();
	for (va = sva; va < eva;) {
		vr = VA_VREG(va);
		vs = VA_VSEG(va);
		rp = &pm->pm_regmap[vr];
		nva = VSTOVA(vr,vs + 1);
		if (nva > eva)
			nva = eva;
		if (rp->rg_nsegmap == 0) {
			va = nva;
			continue;
		}
#ifdef DEBUG
		if (rp->rg_segmap == NULL)
			panic("pmap_protect: no segments");
#endif
		sp = &rp->rg_segmap[vs];
		if (sp->sg_npte == 0) {
			va = nva;
			continue;
		}
#ifdef DEBUG
		if (sp->sg_pte == NULL)
			panic("pmap_protect: no pages");
#endif
		if (sp->sg_pmeg == seginval) {
			int *ptep = &sp->sg_pte[VA_VPG(va)];

			/* not in MMU; just clear PG_W from core copies */
			for (; va < nva; va += NBPG)
				*ptep++ &= ~PG_W;
		} else {
			/* in MMU: take away write bits from MMU PTEs */
			if (CTX_USABLE(pm,rp)) {
				int pte;

				/*
				 * Flush cache so that any existing cache
				 * tags are updated.  This is really only
				 * needed for PTEs that lose PG_W.
				 */
				pmap_stats.ps_npg_prot_all +=
					(nva - va) >> PGSHIFT;
				setcontext4(pm->pm_ctxnum);
				for (; va < nva; va += NBPG) {
					pte = getpte4(va);
					if ((pte & (PG_W|PG_TYPE)) ==
					    (PG_W|PG_OBMEM)) {
						pmap_stats.ps_npg_prot_actual++;
						cache_flush_page(va, pm->pm_ctxnum);
						setpte4(va, pte & ~PG_W);
					}
				}
			} else {
				int pteva;

				/*
				 * No context, hence not cached;
				 * just update PTEs.
				 */
				setcontext4(0);
				/* XXX use per-CPU pteva? */
				if (HASSUN4_MMU3L)
					setregmap(0, tregion);
				setsegmap(0, sp->sg_pmeg);
				pteva = VA_VPG(va) << PGSHIFT;
				for (; va < nva; pteva += NBPG, va += NBPG)
					setpte4(pteva, getpte4(pteva) & ~PG_W);
			}
		}
	}
	PMAP_UNLOCK();
	splx(s);
	setcontext4(ctx);
}

/*
 * Change the protection and/or wired status of the given (MI) virtual page.
 * XXX: should have separate function (or flag) telling whether only wiring
 * is changing.
 */
void
pmap_changeprot4_4c(struct pmap *pm, vaddr_t va, vm_prot_t prot, int flags)
{
	int vr, vs, newprot, ctx, pte, *ptep;
	int pmeg;
	struct regmap *rp;
	struct segmap *sp;

#ifdef DEBUG
	if (pmapdebug & PDB_CHANGEPROT)
		printf("pmap_changeprot(%p, 0x%lx, 0x%x, 0x%x)\n",
		    pm, va, prot, flags);
#endif

	if (pm == pmap_kernel())
		newprot = prot & VM_PROT_WRITE ? PG_S|PG_W : PG_S;
	else
		newprot = prot & VM_PROT_WRITE ? PG_W : 0;
	vr = VA_VREG(va);
	vs = VA_VSEG(va);
	rp = &pm->pm_regmap[vr];
	sp = &rp->rg_segmap[vs];
	ptep = &sp->sg_pte[VA_VPG(va)];

	pmap_stats.ps_changeprots++;

	pte = *ptep;
	if (pte & PG_WIRED && (flags & PMAP_WIRED) == 0) {
		pte &= ~PG_WIRED;
		sp->sg_nwired--;
		pm->pm_stats.wired_count--;
	} else if ((pte & PG_WIRED) == 0 && flags & PMAP_WIRED) {
		pte |= PG_WIRED;
		sp->sg_nwired++;
		pm->pm_stats.wired_count++;
	}
	pte = (pte & ~PG_PROT) | newprot;
	/* Update S/W pte entry */
	*ptep = pte;

	/* update PTEs in software or hardware */
	if ((pmeg = sp->sg_pmeg) != seginval) {
		/* update in hardware */
		ctx = getcontext4();
		if (CTX_USABLE(pm,rp)) {
			/*
			 * Use current context.
			 * Flush cache if page has been referenced to
			 * avoid stale protection bits in the cache tags.
			 */
			setcontext4(pm->pm_ctxnum);
			pte = getpte4(va);
			if ((pte & (PG_U|PG_NC|PG_TYPE)) == (PG_U|PG_OBMEM))
				cache_flush_page(va, pm->pm_ctxnum);
		} else {
			setcontext4(0);
			/* XXX use per-CPU va? */
			if (HASSUN4_MMU3L)
				setregmap(0, tregion);
			setsegmap(0, pmeg);
			va = VA_VPG(va) << PGSHIFT;
			pte = getpte4(va);
		}
		pte = (pte & ~PG_PROT) | newprot;
		setpte4(va, pte);
		setcontext4(ctx);
#ifdef DIAGNOSTIC
		if (sp->sg_nwired > sp->sg_npte || sp->sg_nwired < 0)
			panic("pmap_protect: pm %p, va %lx: nleft=%d, nwired=%d",
				pm, va, sp->sg_npte, sp->sg_nwired);
#endif
		if (sp->sg_nwired == 0)
			mmu_pmeg_unlock(pmeg);
		else
			mmu_pmeg_lock(pmeg);
	}
}

#endif /* SUN4 || SUN4C */

#if defined(SUN4M) || defined(SUN4D)
/*
 * Lower (make more strict) the protection on the specified
 * physical page.
 *
 * There are only two cases: either the protection is going to 0
 * (in which case we do the dirty work here), or it is going
 * to read-only (in which case pv_changepte does the trick).
 */
void
pmap_page_protect4m(struct vm_page *pg, vm_prot_t prot)
{
	struct pvlist *pv, *npv;
	struct pmap *pm;
	vaddr_t va;
	int vr, vs, tpte;
	int flags, nleft, s;
	struct regmap *rp;
	struct segmap *sp;

#ifdef DEBUG
	if ((pmapdebug & PDB_CHANGEPROT) ||
	    (pmapdebug & PDB_REMOVE && prot == VM_PROT_NONE))
		printf("pmap_page_protect[%d](0x%lx, 0x%x)\n",
			cpu_number(), VM_PAGE_TO_PHYS(pg), prot);
#endif
	s = splvm();
	PMAP_LOCK();

	if (prot & VM_PROT_READ) {
		pv_changepte4m(pg, 0, PPROT_WRITE);
		goto out;
	}

	/*
	 * Remove all access to all people talking to this page.
	 * Walk down PV list, removing all mappings. The logic is much
	 * like that for pmap_remove, but we know we are removing exactly
	 * one page.
	 */
	pv = VM_MDPAGE_PVHEAD(pg);
	if (pv->pv_pmap == NULL)
		goto out;

	/* This pv head will become empty, so clear caching state flags */
	flags = pv->pv_flags & ~(PV_NC|PV_ANC);
	while (pv != NULL) {
		pm = pv->pv_pmap;
		va = pv->pv_va;
		vr = VA_VREG(va);
		vs = VA_VSEG(va);
		rp = &pm->pm_regmap[vr];
		if (rp->rg_nsegmap == 0)
			panic("pmap_remove_all: empty vreg");
		sp = &rp->rg_segmap[vs];
		nleft = sp->sg_npte;
		if (pm != pmap_kernel()) {
			if (nleft <= 0)
				panic("pmap_page_protect: empty vseg");
			sp->sg_npte = --nleft;
		}

		/*
		 * Invalidate PTE in MMU pagetables.
		 * Flush cache if necessary.
		 */
		if (pm->pm_ctx) {
			cache_flush_page(va, pm->pm_ctxnum);
		}

		tpte = sp->sg_pte[VA_SUN4M_VPG(va)];
		setpgt4m_va(va, &sp->sg_pte[VA_SUN4M_VPG(va)], SRMMU_TEINVALID,
		    pm->pm_ctx != NULL, pm->pm_ctxnum, PMAP_CPUSET(pm));

		pm->pm_stats.resident_count--;
		if (sp->sg_wiremap & (1 << VA_SUN4M_VPG(va))) {
			sp->sg_wiremap &= ~(1 << VA_SUN4M_VPG(va));
			pm->pm_stats.wired_count--;
		}

		if ((tpte & SRMMU_TETYPE) != SRMMU_TEPTE)
			panic("pmap_page_protect !PG_V: pg %p va %lx", pg, va);

		flags |= MR4M(tpte);

		if (pm != pmap_kernel() && nleft == 0)
			/*
			 * Entire user mode segment is gone
			 */
			pgt_lvl23_remove4m(pm, rp, sp, vr, vs);

		npv = pv->pv_next;
		if (pv != VM_MDPAGE_PVHEAD(pg))
			pool_put(&pv_pool, pv);
		pv = npv;
	}

	/* Finally, update pv head */
	VM_MDPAGE_PVHEAD(pg)->pv_pmap = NULL;
	VM_MDPAGE_PVHEAD(pg)->pv_next = NULL;
	VM_MDPAGE_PVHEAD(pg)->pv_flags = flags;

out:
	PMAP_UNLOCK();
	splx(s);
}

/*
 * Lower (make more strict) the protection on the specified
 * range of this pmap.
 */
void
pmap_protect4m(struct pmap *pm, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	vaddr_t va, nva;
	int s, vr, vs;
	struct regmap *rp;
	struct segmap *sp;
	int newprot;

	if ((prot & VM_PROT_READ) == 0) {
		pmap_remove(pm, sva, eva);
		return;
	}

#ifdef DEBUG
	if (pmapdebug & PDB_CHANGEPROT)
		printf("pmap_protect[%d][curpid %d, ctx %d,%d](%lx, %lx, %x)\n",
			cpu_number(), curproc->p_pid,
			getcontext4m(), pm->pm_ctx ? pm->pm_ctxnum : -1,
			sva, eva, prot);
#endif

	newprot = pte_prot4m(pm, prot);

	write_user_windows();
	s = splvm();
	PMAP_LOCK();

	for (va = sva; va < eva;) {
		vr = VA_VREG(va);
		vs = VA_VSEG(va);
		rp = &pm->pm_regmap[vr];
		nva = VSTOVA(vr,vs + 1);
		if (nva > eva)
			nva = eva;
		if (rp->rg_nsegmap == 0) {
			va = nva;
			continue;
		}
		sp = &rp->rg_segmap[vs];
		if (pm != pmap_kernel() && sp->sg_npte == 0) {
			va = nva;
			continue;
		}

		/*
		 * pages loaded: take away write bits from MMU PTEs
		 */
		pmap_stats.ps_npg_prot_all += (nva - va) >> PGSHIFT;
		for (; va < nva; va += NBPG) {
			int tpte, npte;

			tpte = sp->sg_pte[VA_SUN4M_VPG(va)];
			if ((tpte & SRMMU_PGTYPE) != PG_SUN4M_OBMEM)
				continue;
			if ((tpte & SRMMU_TETYPE) != SRMMU_TEPTE)
				continue;
			npte = (tpte & ~SRMMU_PROT_MASK) | newprot;
			if (npte == tpte)
				continue;

			/*
			 * Flush cache so that any existing cache
			 * tags are updated.
			 */

			pmap_stats.ps_npg_prot_actual++;
			if (pm->pm_ctx) {
				cache_flush_page(va, pm->pm_ctxnum);
			}
			updatepte4m(va, &sp->sg_pte[VA_SUN4M_VPG(va)],
			    SRMMU_PROT_MASK, newprot, pm->pm_ctxnum,
			    PMAP_CPUSET(pm));
		}
	}
	PMAP_UNLOCK();
	splx(s);
}

/*
 * Change the protection and/or wired status of the given (MI) virtual page.
 * XXX: should have separate function (or flag) telling whether only wiring
 * is changing.
 */
void
pmap_changeprot4m(struct pmap *pm, vaddr_t va, vm_prot_t prot, int flags)
{
	int pte, newprot;
	struct regmap *rp;
	struct segmap *sp;
	bool owired;

#ifdef DEBUG
	if (pmapdebug & PDB_CHANGEPROT)
		printf("pmap_changeprot[%d](%p, 0x%lx, 0x%x, 0x%x)\n",
		    cpu_number(), pm, va, prot, flags);
#endif

	newprot = pte_prot4m(pm, prot);

	pmap_stats.ps_changeprots++;

	rp = &pm->pm_regmap[VA_VREG(va)];
	sp = &rp->rg_segmap[VA_VSEG(va)];

	pte = sp->sg_pte[VA_SUN4M_VPG(va)];
	owired = sp->sg_wiremap & (1 << VA_SUN4M_VPG(va));

	if (owired) {
		pm->pm_stats.wired_count--;
		sp->sg_wiremap &= ~(1 << VA_SUN4M_VPG(va));
	}
	if (flags & PMAP_WIRED) {
		pm->pm_stats.wired_count++;
		sp->sg_wiremap |= (1 << VA_SUN4M_VPG(va));
	}

	if (pm->pm_ctx) {
		/*
		 * Use current context.
		 * Flush cache if page has been referenced to
		 * avoid stale protection bits in the cache tags.
		 */

		if ((pte & (SRMMU_PG_C|SRMMU_PGTYPE)) ==
		    (SRMMU_PG_C|PG_SUN4M_OBMEM))
			cache_flush_page(va, pm->pm_ctxnum);
	}

	setpgt4m_va(va, &sp->sg_pte[VA_SUN4M_VPG(va)],
		 (pte & ~SRMMU_PROT_MASK) | newprot,
		 pm->pm_ctx != NULL, pm->pm_ctxnum, PMAP_CPUSET(pm));

}
#endif /* SUN4M || SUN4D */

/*
 * Insert (MI) physical page pa at virtual address va in the given pmap.
 * NB: the pa parameter includes type bits PMAP_OBIO, PMAP_NC as necessary.
 *
 * If pa is not in the `managed' range it will not be `bank mapped'.
 * This works during bootstrap only because the first 4MB happens to
 * map one-to-one.
 *
 * There may already be something else there, or we might just be
 * changing protections and/or wiring on an existing mapping.
 *	XXX	should have different entry points for changing!
 */

#if defined(SUN4) || defined(SUN4C)

int
pmap_enter4_4c(struct pmap *pm, vaddr_t va, paddr_t pa,
	       vm_prot_t prot, int flags)
{
	struct vm_page *pg;
	int pteproto, ctx;
	int error;

	if (VA_INHOLE(va)) {
#ifdef DEBUG
		printf("pmap_enter: pm %p, va 0x%lx, pa 0x%lx: in MMU hole\n",
			pm, va, pa);
#endif
		return 0;
	}

#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("pmap_enter(%p, 0x%lx, 0x%lx, 0x%x, 0x%x)\n",
		    pm, va, pa, prot, flags);
#endif

	pg = PHYS_TO_VM_PAGE(pa);
	pteproto = PG_V | PMAP_T2PTE_4(pa);
	pa &= ~PMAP_TNC_4;

	/*
	 * Set up prototype for new PTE.  Cannot set PG_NC from PV_NC yet
	 * since the pvlist no-cache bit might change as a result of the
	 * new mapping.
	 */
	pteproto |= atop(pa) & PG_PFNUM;
	if (prot & VM_PROT_WRITE)
		pteproto |= PG_W;
	if ((flags & PMAP_WIRED) != 0)
		pteproto |= PG_WIRED;
	if (flags & VM_PROT_ALL) {
		pteproto |= PG_U;
		if (flags & VM_PROT_WRITE) {
			pteproto |= PG_M;
		}
	}

	write_user_windows();
	ctx = getcontext4();
	if (pm == pmap_kernel())
		error = pmap_enk4_4c(pm, va, prot, flags, pg, pteproto | PG_S);
	else
		error = pmap_enu4_4c(pm, va, prot, flags, pg, pteproto);
	setcontext4(ctx);
	return (error);
}

/* enter new (or change existing) kernel mapping */
int
pmap_enk4_4c(struct pmap *pm, vaddr_t va, vm_prot_t prot, int flags,
	     struct vm_page *pg, int pteproto)
{
	int vr, vs, pte, s, inmmu;
	int *ptep;
	struct regmap *rp;
	struct segmap *sp;
	int error = 0;

	vr = VA_VREG(va);
	vs = VA_VSEG(va);
	rp = &pm->pm_regmap[vr];
	sp = &rp->rg_segmap[vs];
	ptep = &sp->sg_pte[VA_VPG(va)];
	s = splvm();		/* XXX way too conservative */

#if defined(SUN4_MMU3L)
	if (HASSUN4_MMU3L && rp->rg_smeg == reginval)
		mmu_pagein_reg(pm, rp, va, vr, &region_locked);
#endif

	inmmu = sp->sg_pmeg != seginval;
	if ((pte = *ptep) & PG_V) {

		/* old mapping exists, and is of the same pa type */
		if ((pte & (PG_PFNUM|PG_TYPE)) ==
		    (pteproto & (PG_PFNUM|PG_TYPE))) {
			/* just changing protection and/or wiring */
			pmap_changeprot4_4c(pm, va, prot, flags);
			splx(s);
			return (0);
		}

		if ((pte & PG_TYPE) == PG_OBMEM) {
			struct vm_page *opg;

			/*
			 * Switcheroo: changing pa for this va.
			 * If old pa was managed, remove from pvlist.
			 * If old page was cached, flush cache.
			 */
			if ((opg = pvhead4_4c(pte)) != NULL)
				pv_unlink4_4c(opg, pm, va);
			if (inmmu && (pte & PG_NC) == 0) {
				setcontext4(0);	/* ??? */
				cache_flush_page(va, 0);
			}
		}
		*ptep = 0;
		if (inmmu)
			setpte4(va, 0);
		if (pte & PG_WIRED) {
			sp->sg_nwired--;
			pm->pm_stats.wired_count--;
		}
		pm->pm_stats.resident_count--;
	} else {
		/* adding new entry */
		if (sp->sg_npte++ == 0) {
#ifdef DIAGNOSTIC
			int i; for (i = 0; i < NPTESG; i++) {
				if (sp->sg_pte[i] == 0)
					continue;
				panic("pmap_enk: pm %p, va %lx: pte[%d] not empty\n",
					pm, va, i);
			}
#endif
			rp->rg_nsegmap++;
		}
	}

	/*
	 * If the new mapping is for a managed PA, enter into pvlist.
	 */
	if (pg != NULL && (error = pv_link4_4c(pg, pm, va, &pteproto)) != 0) {
		if (--sp->sg_npte == 0)
			pgt_lvl23_remove4_4c(pm, rp, sp, vr, vs);
		if ((flags & PMAP_CANFAIL) != 0)
			goto out;
		panic("pmap_enter: cannot allocate PV entry");
	}

	/* Update S/W page table */
	*ptep = pteproto;
	if (pteproto & PG_WIRED) {
		sp->sg_nwired++;
		pm->pm_stats.wired_count++;
	}
	pm->pm_stats.resident_count++;

#ifdef DIAGNOSTIC
	if (sp->sg_nwired > sp->sg_npte || sp->sg_nwired < 0)
		panic("pmap_enk: pm %p, va %lx: nleft=%d, nwired=%d",
			pm, va, sp->sg_npte, sp->sg_nwired);
#endif
	if (sp->sg_pmeg == seginval)
		mmu_pagein_seg(pm, sp, va, vr, vs,
			(pteproto & PG_WIRED) != 0 ? &segm_locked : &segm_lru);
	else if ((pteproto & PG_WIRED) != 0)
		mmu_pmeg_lock(sp->sg_pmeg);

	/* Update H/W page table */
	setpte4(va, pteproto & ~PG_MBZ);
out:
	splx(s);
	return (error);
}

/* enter new (or change existing) user mapping */
int
pmap_enu4_4c(struct pmap *pm, vaddr_t va, vm_prot_t prot, int flags,
	     struct vm_page *pg, int pteproto)
{
	int vr, vs, *ptep, pte, pmeg, s;
	int error = 0;
	struct regmap *rp;
	struct segmap *sp;

	pm->pm_flags &= ~PMAP_USERCACHECLEAN;

	vr = VA_VREG(va);
	vs = VA_VSEG(va);
	rp = &pm->pm_regmap[vr];
	s = splvm();			/* XXX conservative */

	/*
	 * If there is no space in which the PTEs can be written
	 * while they are not in the hardware, this must be a new
	 * virtual segment.  Get PTE space and count the segment.
	 *
	 * TO SPEED UP CTX ALLOC, PUT SEGMENT BOUNDS STUFF HERE
	 * AND IN pmap_rmu()
	 */

	GAP_SHRINK(pm,vr);

#ifdef DEBUG
	if (pm->pm_gap_end < pm->pm_gap_start) {
		printf("pmap_enu: gap_start 0x%x, gap_end 0x%x",
			pm->pm_gap_start, pm->pm_gap_end);
		panic("pmap_enu: gap botch");
	}
#endif

	if (rp->rg_segmap == NULL) {
		/* definitely a new mapping */
		int i;
		int mflag = PR_NOWAIT;

	rretry:
		sp = (struct segmap *)pool_get(&segmap_pool, mflag);
		if (sp == NULL) {
			if ((flags & PMAP_CANFAIL) != 0) {
				error = ENOMEM;
				goto out;
			}
			mflag = PR_WAITOK;
			goto rretry;
		}
#ifdef DEBUG
		if (rp->rg_segmap != NULL)
			panic("pmap_enter: segment filled during sleep");
#endif
		qzero((void *)sp, NSEGRG * sizeof (struct segmap));
		rp->rg_segmap = sp;
		rp->rg_nsegmap = 0;
		for (i = NSEGRG; --i >= 0;)
			sp++->sg_pmeg = seginval;
	}

	sp = &rp->rg_segmap[vs];

	if ((ptep = sp->sg_pte) == NULL) {
		/* definitely a new mapping */
		int size = NPTESG * sizeof *ptep;
		int mflag = PR_NOWAIT;

	sretry:
		ptep = (int *)pool_get(&pte_pool, mflag);
		if (ptep == NULL) {
			if ((flags & PMAP_CANFAIL) != 0) {
				error = ENOMEM;
				goto out;
			}
			mflag = PR_WAITOK;
			goto sretry;
		}
#ifdef DEBUG
		if (sp->sg_pte != NULL)
			panic("pmap_enter: pte filled during sleep");
		if (sp->sg_pmeg != seginval)
			panic("pmap_enter: new ptes, but not seginval");
#endif
		qzero((void *)ptep, size);
		sp->sg_pte = ptep;
		sp->sg_npte = 1;
		rp->rg_nsegmap++;
	} else {
		/* might be a change: fetch old pte */
		pte = ptep[VA_VPG(va)];
		if (pte & PG_V) {
			/* old mapping exists, and is of the same pa type */
			if ((pte & (PG_PFNUM|PG_TYPE)) ==
			    (pteproto & (PG_PFNUM|PG_TYPE))) {
				/* just changing prot and/or wiring */
				pmap_changeprot4_4c(pm, va, prot, flags);
				splx(s);
				return (0);
			}
			/*
			 * Switcheroo: changing pa for this va.
			 * If old pa was managed, remove from pvlist.
			 * If old page was cached, flush cache.
			 */
#if 0
			printf("%s[%d]: pmap_enu: changing existing "
				"va(0x%lx)=>pa entry\n",
				curproc->p_comm, curproc->p_pid, va);
#endif
			if ((pte & PG_TYPE) == PG_OBMEM) {
				struct vm_page *opg;
				if ((opg = pvhead4_4c(pte)) != NULL)
					pv_unlink4_4c(opg, pm, va);
				if (CACHEINFO.c_vactype != VAC_NONE &&
				    (pmeg = sp->sg_pmeg) != seginval) {
					/* hardware pte */
					if (CTX_USABLE(pm,rp)) {
						setcontext4(pm->pm_ctxnum);
					} else {
						setcontext4(0);
						/* XXX use per-CPU pteva? */
						if (HASSUN4_MMU3L)
							setregmap(0, tregion);
						setsegmap(0, pmeg);
					}
					cache_flush_page(va, pm->pm_ctxnum);
				}
			}
			if (pte & PG_WIRED) {
				sp->sg_nwired--;
				pm->pm_stats.wired_count--;
			}
			pm->pm_stats.resident_count--;
			ptep[VA_VPG(va)] = 0;
			if (sp->sg_pmeg != seginval)
				setpte4(va, 0);
		} else {
			/* adding new entry */
			sp->sg_npte++;
		}
	}

	if (pg != NULL && (error = pv_link4_4c(pg, pm, va, &pteproto)) != 0) {
		if (--sp->sg_npte == 0)
			/* Sigh, undo pgt allocations */
			pgt_lvl23_remove4_4c(pm, rp, sp, vr, vs);

		if ((flags & PMAP_CANFAIL) != 0)
			goto out;
		panic("pmap_enter: cannot allocate PV entry");
	}

	/* Update S/W page table */
	ptep += VA_VPG(va);
	*ptep = pteproto;
	if (pteproto & PG_WIRED) {
		sp->sg_nwired++;
		pm->pm_stats.wired_count++;
	}
	pm->pm_stats.resident_count++;

#ifdef DIAGNOSTIC
	if (sp->sg_nwired > sp->sg_npte || sp->sg_nwired < 0)
		panic("pmap_enu: pm %p, va %lx: nleft=%d, nwired=%d",
			pm, va, sp->sg_npte, sp->sg_nwired);
#endif

	if ((pmeg = sp->sg_pmeg) != seginval) {
		/* Update H/W page table */
		if (CTX_USABLE(pm,rp))
			setcontext4(pm->pm_ctxnum);
		else {
			setcontext4(0);
			if (HASSUN4_MMU3L)
				setregmap(0, tregion);
			setsegmap(0, pmeg);
			va = VA_VPG(va) << PGSHIFT;
		}
		setpte4(va, pteproto & ~PG_MBZ);
	}

out:
	splx(s);
	return (error);
}

void
pmap_kenter_pa4_4c(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	struct pmap *pm = pmap_kernel();
	struct regmap *rp;
	struct segmap *sp;
	int vr, vs, s;
	int *ptep, pteproto;
	int lockit = 1;

	pteproto = PG_S | PG_V | PMAP_T2PTE_4(pa);
	pa &= ~PMAP_TNC_4;
	pteproto |= atop(pa) & PG_PFNUM;
	if (prot & VM_PROT_WRITE)
		pteproto |= PG_W;

	vr = VA_VREG(va);
	vs = VA_VSEG(va);
	rp = &pm->pm_regmap[vr];
	sp = &rp->rg_segmap[vs];
	ptep = &sp->sg_pte[VA_VPG(va)];

	if (lockit) {
		pteproto |= PG_WIRED;
		sp->sg_nwired++;
	}

	KASSERT((*ptep & PG_V) == 0);

	s = splvm();
#if defined(SUN4_MMU3L)
	if (HASSUN4_MMU3L && rp->rg_smeg == reginval)
		mmu_pagein_reg(pm, rp, va, vr, &region_locked);
#endif

	if (sp->sg_npte++ == 0) {
#ifdef DIAGNOSTIC
		int i; for (i = 0; i < NPTESG; i++) {
			if (sp->sg_pte[i] == 0)
				continue;
			panic("pmap_enk: pm %p, va %lx: pte[%d] not empty\n",
				pm, va, i);
		}
#endif
		rp->rg_nsegmap++;
	}

	/* Update S/W page table */
	*ptep = pteproto;

#ifdef DIAGNOSTIC
	if (sp->sg_nwired > sp->sg_npte || sp->sg_nwired < 0)
		panic("pmap_kenter: pm %p, va %lx: nleft=%d, nwired=%d",
			pm, va, sp->sg_npte, sp->sg_nwired);
#endif

	if (sp->sg_pmeg == seginval) {
		mmu_pagein_seg(pm, sp, va, vr, vs,
				lockit ? &segm_locked : &segm_lru);
	} else if (lockit)
		mmu_pmeg_lock(sp->sg_pmeg);

	/* Update H/W page table */
	setpte4(va, pteproto & ~PG_MBZ);
	splx(s);
}

#if notyet
void pmap_lockmmu(vaddr_t sva, size_t sz);

void
pmap_lockmmu(vaddr_t sva, size_t sz)
{
	struct pmap *pm = pmap_kernel();
	vaddr_t va, eva;
	struct regmap *rp;
	struct segmap *sp;
	int vr, vs;

	if (CPU_HAS_SRMMU)
		return;

	eva = sva + sz;
	va = VA_ROUNDDOWNTOSEG(sva);

	for (; va < eva; va += NBPSG) {
		vr = VA_VREG(va);
		vs = VA_VSEG(va);
		rp = &pm->pm_regmap[vr];
		sp = &rp->rg_segmap[vs];

		KASSERT(sp->sg_npte != 0);

		if (sp->sg_pmeg == seginval)
			mmu_pagein_seg(pm, sp, va, vr, vs, &segm_locked);
		else
			mmu_pmeg_lock(sp->sg_pmeg);
	}
}
#endif

void
pmap_kremove4_4c(vaddr_t va, vsize_t len)
{
	struct pmap *pm = pmap_kernel();
	struct regmap *rp;
	struct segmap *sp;
	vaddr_t nva, endva;
	int pte, mmupte, *ptep, perpage, npg, inmmu;
	int nleft, pmeg;
	int vr, vs, s, ctx;

	endva = va + len;
#ifdef DEBUG
	if (pmapdebug & PDB_REMOVE)
		printf("pmap_kremove(0x%lx, 0x%lx)\n", va, endva);
#endif

	write_user_windows();

	s = splvm();
	ctx = getcontext();
	PMAP_LOCK();
	setcontext4(0);
	for (; va < endva; va = nva) {
		/* do one virtual segment at a time */
		vr = VA_VREG(va);
		vs = VA_VSEG(va);
		nva = VSTOVA(vr, vs + 1);
		if (nva == 0 || nva > endva)
			nva = endva;

		rp = &pm->pm_regmap[vr];
		sp = &rp->rg_segmap[vs];

		if (rp->rg_nsegmap == 0)
			continue;
		nleft = sp->sg_npte;
		if (nleft == 0)
			continue;
		pmeg = sp->sg_pmeg;
		inmmu = (pmeg != seginval);
		ptep = &sp->sg_pte[VA_VPG(va)];

		/* decide how to flush cache */
		npg = (nva - va) >> PGSHIFT;
		if (!inmmu) {
			perpage = 0;
		} else if (npg > PMAP_SFL_THRESHOLD) {
			/* flush the whole segment */
			perpage = 0;
			cache_flush_segment(vr, vs, 0);
		} else {
			/*
			 * flush each page individually;
			 * some never need flushing
			 */
			perpage = (CACHEINFO.c_vactype != VAC_NONE);
		}

		for (; va < nva; va += NBPG, ptep++) {
			pte = *ptep;
			mmupte = inmmu ? getpte4(va) : 0;
			if ((pte & PG_V) == 0) {
#ifdef DIAGNOSTIC
				if (inmmu && (mmupte & PG_V) != 0)
					printf("rmk: inconsistent ptes va=%lx\n", va);
#endif
				continue;
			}
			if ((pte & PG_TYPE) == PG_OBMEM) {
				/* if cacheable, flush page as needed */
				if (perpage && (mmupte & PG_NC) == 0)
					cache_flush_page(va, 0);
			}
			nleft--;
#ifdef DIAGNOSTIC
			if (nleft < 0)
				panic("pmap_kremove: too many PTEs in segment; "
				      "va 0x%lx; endva 0x%lx", va, endva);
#endif
			if (pte & PG_WIRED)
				sp->sg_nwired--;

			if (inmmu)
				setpte4(va, 0);
			*ptep = 0;
		}

#ifdef DIAGNOSTIC
		if (sp->sg_nwired > nleft || sp->sg_nwired < 0)
			panic("pmap_kremove: pm %p, va %lx: nleft=%d, nwired=%d",
				pm, va, nleft, sp->sg_nwired);
#endif

		if ((sp->sg_npte = nleft) == 0)
			pgt_lvl23_remove4_4c(pm, rp, sp, vr, vs);
		else if (sp->sg_nwired == 0) {
			if (sp->sg_pmeg != seginval)
				mmu_pmeg_unlock(sp->sg_pmeg);
		}
	}
	PMAP_UNLOCK();
	setcontext4(ctx);
	splx(s);
}

/*
 * Change protection on a range of kernel addresses.
 */
void
pmap_kprotect4_4c(vaddr_t va, vsize_t size, vm_prot_t prot)
{
	int pte, newprot, ctx;

	size = roundup(size,NBPG);
	newprot = prot & VM_PROT_WRITE ? PG_S|PG_W : PG_S;

	ctx = getcontext4();
	setcontext4(0);
	while (size > 0) {
		pte = getpte4(va);

		/*
		 * Flush cache if page has been referenced to
		 * avoid stale protection bits in the cache tags.
		 */
		if ((pte & (PG_NC|PG_TYPE)) == PG_OBMEM)
			cache_flush_page(va, 0);

		pte = (pte & ~PG_PROT) | newprot;
		setpte4(va, pte);

		va += NBPG;
		size -= NBPG;
	}
	setcontext4(ctx);
}
#endif /* SUN4 || SUN4C */

#if defined(SUN4M) || defined(SUN4D)	/* SRMMU versions of enter routines */
/*
 * Insert (MI) physical page pa at virtual address va in the given pmap.
 * NB: the pa parameter includes type bits PMAP_OBIO, PMAP_NC as necessary.
 *
 * If pa is not in the `managed' range it will not be `bank mapped'.
 * This works during bootstrap only because the first 4MB happens to
 * map one-to-one.
 *
 * There may already be something else there, or we might just be
 * changing protections and/or wiring on an existing mapping.
 *	XXX	should have different entry points for changing!
 */

int
pmap_enter4m(struct pmap *pm, vaddr_t va, paddr_t pa,
	     vm_prot_t prot, int flags)
{
	struct vm_page *pg;
	int pteproto;
	int error;

#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("pmap_enter[curcpu %d, curpid %d, ctx %d,%d]"
			"(%p, 0x%lx, 0x%lx, 0x%x, 0x%x)\n",
			cpu_number(), curproc==NULL ? -1 : curproc->p_pid,
			getcontext4m(), pm->pm_ctx==NULL ? -1 : pm->pm_ctxnum,
			pm, va, pa, prot, flags);
#endif

	pg = PHYS_TO_VM_PAGE(pa);

	/* Initialise pteproto with cache bit */
	pteproto = (pa & PMAP_NC) == 0 ? SRMMU_PG_C : 0;

#ifdef DEBUG
	if (pa & PMAP_TYPE_SRMMU) {	/* this page goes in an iospace */
		if (cpuinfo.cpu_type == CPUTYP_MS1)
			panic("pmap_enter4m: attempt to use 36-bit iospace on"
			      " MicroSPARC");
	}
#endif
	pteproto |= SRMMU_TEPTE;
	pteproto |= PMAP_T2PTE_SRMMU(pa);
	pa &= ~PMAP_TNC_SRMMU;

	/*
	 * Set up prototype for new PTE.  Cannot set PG_NC from PV_NC yet
	 * since the pvlist no-cache bit might change as a result of the
	 * new mapping.
	 */
	pteproto |= (atop(pa) << SRMMU_PPNSHIFT);

	/* Make sure we get a pte with appropriate perms! */
	pteproto |= pte_prot4m(pm, prot);
	if (flags & VM_PROT_ALL) {
		pteproto |= SRMMU_PG_R;
		if (flags & VM_PROT_WRITE) {
			pteproto |= SRMMU_PG_M;
		}
	}

	if (pm == pmap_kernel())
		error = pmap_enk4m(pm, va, prot, flags, pg, pteproto | PPROT_S);
	else
		error = pmap_enu4m(pm, va, prot, flags, pg, pteproto);

	return (error);
}

/* enter new (or change existing) kernel mapping */
int
pmap_enk4m(struct pmap *pm, vaddr_t va, vm_prot_t prot, int flags,
	   struct vm_page *pg, int pteproto)
{
	int vr, vs, tpte, s;
	struct regmap *rp;
	struct segmap *sp;
	int error = 0;

#ifdef DEBUG
	if (va < KERNBASE)
		panic("pmap_enk4m: can't enter va 0x%lx below KERNBASE", va);
#endif
	vr = VA_VREG(va);
	vs = VA_VSEG(va);
	rp = &pm->pm_regmap[vr];
	sp = &rp->rg_segmap[vs];

	s = splvm();		/* XXX way too conservative */
	PMAP_LOCK();

	if (rp->rg_seg_ptps == NULL) /* enter new region */
		panic("pmap_enk4m: missing kernel region table for va 0x%lx",va);

	tpte = sp->sg_pte[VA_SUN4M_VPG(va)];
	if ((tpte & SRMMU_TETYPE) == SRMMU_TEPTE) {

		/* old mapping exists, and is of the same pa type */

		if ((tpte & SRMMU_PPNMASK) == (pteproto & SRMMU_PPNMASK)) {
			/* just changing protection and/or wiring */
			pmap_changeprot4m(pm, va, prot, flags);
			PMAP_UNLOCK();
			splx(s);
			return (0);
		}

		if ((tpte & SRMMU_PGTYPE) == PG_SUN4M_OBMEM) {
			struct vm_page *opg;
#ifdef DEBUG
printf("pmap_enk4m: changing existing va=>pa entry: va 0x%lx, pteproto 0x%x, "
       "oldpte 0x%x\n", va, pteproto, tpte);
#endif
			/*
			 * Switcheroo: changing pa for this va.
			 * If old pa was managed, remove from pvlist.
			 * If old page was cached, flush cache.
			 */
			if ((opg = pvhead4m(tpte)) != NULL)
				pv_unlink4m(opg, pm, va);
			if (tpte & SRMMU_PG_C) {
				cache_flush_page(va, 0);
			}
		}

		/*
		 * Invalidate the mapping now, so we can avoid the
		 * de-map and update protocol when setting the new
		 * PTE below.
		 */
		setpgt4m_va(va, &sp->sg_pte[VA_SUN4M_VPG(va)],
			SRMMU_TEINVALID, pm->pm_ctx != NULL,
			pm->pm_ctxnum, PMAP_CPUSET(pm));
		pm->pm_stats.resident_count--;
	}

	/*
	 * If the new mapping is for a managed PA, enter into pvlist.
	 */
	if (pg != NULL && (error = pv_link4m(pg, pm, va, &pteproto)) != 0) {
		if ((flags & PMAP_CANFAIL) != 0)
			goto out;
		panic("pmap_enter: cannot allocate PV entry");
	}

	setpgt4m(&sp->sg_pte[VA_SUN4M_VPG(va)], pteproto);
	pm->pm_stats.resident_count++;
out:
	PMAP_UNLOCK();
	splx(s);
	return (error);
}

/* enter new (or change existing) user mapping */
int
pmap_enu4m(struct pmap *pm, vaddr_t va, vm_prot_t prot, int flags,
	   struct vm_page *pg, int pteproto)
{
	int vr, vs, *pte, tpte, s;
	int error = 0;
	struct regmap *rp;
	struct segmap *sp;
	bool owired;

#ifdef DEBUG
	if (KERNBASE < va)
		panic("pmap_enu4m: can't enter va 0x%lx above KERNBASE", va);
#endif

	pm->pm_flags &= ~PMAP_USERCACHECLEAN;

	vr = VA_VREG(va);
	vs = VA_VSEG(va);
	rp = &pm->pm_regmap[vr];
	s = splvm();			/* XXX conservative */
	PMAP_LOCK();

	if (rp->rg_segmap == NULL) {
		/* definitely a new mapping */
		int mflag = PR_NOWAIT;

	rretry:
		sp = (struct segmap *)pool_get(&segmap_pool, mflag);
		if (sp == NULL) {
			if ((flags & PMAP_CANFAIL) != 0) {
				error = ENOMEM;
				goto out;
			}
			mflag = PR_WAITOK;
			goto rretry;
		}
#ifdef DEBUG
		if (rp->rg_segmap != NULL)
			panic("pmap_enu4m: segment filled during sleep");
#endif
		qzero((void *)sp, NSEGRG * sizeof (struct segmap));
		rp->rg_segmap = sp;
		rp->rg_nsegmap = 0;
		rp->rg_seg_ptps = NULL;
	}
	if (rp->rg_seg_ptps == NULL) {
		/* Need a segment table */
		int i, *ptd;
		int mflag = PR_NOWAIT;

	sretry:
		ptd = pool_get(&L23_pool, mflag);
		if (ptd == NULL) {
			if ((flags & PMAP_CANFAIL) != 0) {
				error = ENOMEM;
				goto out;
			}
			mflag = PR_WAITOK;
			goto sretry;
		}

		rp->rg_seg_ptps = ptd;
		for (i = 0; i < SRMMU_L2SIZE; i++)
			setpgt4m(&ptd[i], SRMMU_TEINVALID);

		/* Replicate segment allocation in each CPU's region table */
#ifdef MULTIPROCESSOR
		for (i = 0; i < sparc_ncpus; i++)
#else
		i = 0;
#endif
		{
#if defined(MULTIPROCESSOR)
			if ((cpus[i].ci.flags & CPUFLG_HATCHED) == 0)
				continue;
#endif
			setpgt4m(&pm->pm_reg_ptps[i][vr],
				 (VA2PA((void *)ptd) >> SRMMU_PPNPASHIFT) |
					SRMMU_TEPTD);
		}
	}

	sp = &rp->rg_segmap[vs];

	owired = false;
	if ((pte = sp->sg_pte) == NULL) {
		/* definitely a new mapping */
		int i;
		int mflag = PR_NOWAIT;

		pte = pool_get(&L23_pool, mflag);
		if (pte == NULL) {
			if ((flags & PMAP_CANFAIL) != 0) {
				error = ENOMEM;
				goto out;
			}
			panic("pmap_enter: cannot allocate PTE table");
		}

		sp->sg_pte = pte;
		sp->sg_npte = 1;
		rp->rg_nsegmap++;
		for (i = 0; i < SRMMU_L3SIZE; i++)
			setpgt4m(&pte[i], SRMMU_TEINVALID);
		setpgt4m(&rp->rg_seg_ptps[vs],
			(VA2PA((void *)pte) >> SRMMU_PPNPASHIFT) | SRMMU_TEPTD);
	} else {
#ifdef DIAGNOSTIC
		if (sp->sg_npte <= 0)
			panic("pm %p: npte %d", pm, sp->sg_npte);
#endif
		/*
		 * Might be a change: fetch old pte
		 */
		tpte = pte[VA_SUN4M_VPG(va)];

		if ((tpte & SRMMU_TETYPE) == SRMMU_TEPTE) {

			/* old mapping exists, and is of the same pa type */
			if ((tpte & SRMMU_PPNMASK) ==
			    (pteproto & SRMMU_PPNMASK)) {
				/* just changing prot and/or wiring */
				/* caller should call this directly: */
				pmap_changeprot4m(pm, va, prot, flags);
				PMAP_UNLOCK();
				splx(s);
				return (0);
			}
			/*
			 * Switcheroo: changing pa for this va.
			 * If old pa was managed, remove from pvlist.
			 * If old page was cached, flush cache.
			 */
#ifdef DEBUG
			if (pmapdebug & PDB_SWITCHMAP)
				printf("%s[%d]: pmap_enu: changing existing "
					"va 0x%x: pte 0x%x=>0x%x\n",
					curproc->p_comm, curproc->p_pid,
					(int)va, tpte, pteproto);
#endif
			if ((tpte & SRMMU_PGTYPE) == PG_SUN4M_OBMEM) {
				struct vm_page *opg;
				if ((opg = pvhead4m(tpte)) != NULL) {
					VM_MDPAGE_PVHEAD(opg)->pv_flags |=
							MR4M(tpte);
					pv_unlink4m(opg, pm, va);
				}
				if (pm->pm_ctx && (tpte & SRMMU_PG_C))
					cache_flush_page(va, pm->pm_ctxnum);
			}
			/*
			 * We end up in this `change map' branch relatively
			 * infrequently.
			 * Invalidate the mapping now, so we can avoid the
			 * de-map and update protocol when setting the new
			 * PTE below.
			 */
			setpgt4m_va(va, &sp->sg_pte[VA_SUN4M_VPG(va)],
				SRMMU_TEINVALID, pm->pm_ctx != NULL,
				pm->pm_ctxnum, PMAP_CPUSET(pm));
			pm->pm_stats.resident_count--;
			owired = sp->sg_wiremap & (1 << VA_SUN4M_VPG(va));
		} else {
			/* adding new entry */
			sp->sg_npte++;
		}
	}

	if (pg != NULL && (error = pv_link4m(pg, pm, va, &pteproto)) != 0) {
		if (--sp->sg_npte == 0)
			/* Sigh, undo pgt allocations */
			pgt_lvl23_remove4m(pm, rp, sp, vr, vs);

		if ((flags & PMAP_CANFAIL) != 0)
			goto out;
		panic("pmap_enter: cannot allocate PV entry");
	}

	/*
	 * Update PTEs, flush TLB as necessary.
	 */
	setpgt4m(&sp->sg_pte[VA_SUN4M_VPG(va)], pteproto);
	pm->pm_stats.resident_count++;
	if (owired) {
		pm->pm_stats.wired_count--;
		sp->sg_wiremap &= ~(1 << VA_SUN4M_VPG(va));
	}
	if (flags & PMAP_WIRED) {
		pm->pm_stats.wired_count++;
		sp->sg_wiremap |= (1 << VA_SUN4M_VPG(va));
	}

out:
	PMAP_UNLOCK();
	splx(s);
	return (error);
}

void
pmap_kenter_pa4m(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	struct pmap *pm = pmap_kernel();
	struct regmap *rp;
	struct segmap *sp;
	int pteproto, vr, vs;

	/* Initialise pteproto with cache bit */
	pteproto = (pa & PMAP_NC) == 0 ? SRMMU_PG_C : 0;
	pteproto |= SRMMU_TEPTE | PPROT_S;
	pteproto |= PMAP_T2PTE_SRMMU(pa);
	pteproto |= (atop(pa & ~PMAP_TNC_SRMMU) << SRMMU_PPNSHIFT);
	pteproto |= pte_kprot4m(prot);

	vr = VA_VREG(va);
	vs = VA_VSEG(va);
	rp = &pm->pm_regmap[vr];
	sp = &rp->rg_segmap[vs];

	KASSERT((sp->sg_pte[VA_SUN4M_VPG(va)] & SRMMU_TETYPE) != SRMMU_TEPTE);

	setpgt4m(&sp->sg_pte[VA_SUN4M_VPG(va)], pteproto);
}

void
pmap_kremove4m(vaddr_t va, vsize_t len)
{
	struct pmap *pm = pmap_kernel();
	struct regmap *rp;
	struct segmap *sp;
	vaddr_t endva, nva;
	int vr, vs;
	int tpte, perpage, npg, s;

	/*
	 * The kernel pmap doesn't need to be locked, but the demap lock
	 * in updatepte() requires interrupt protection.
	 */
	s = splvm();

	endva = va + len;
	for (; va < endva; va = nva) {
		/* do one virtual segment at a time */
		vr = VA_VREG(va);
		vs = VA_VSEG(va);
		nva = VSTOVA(vr, vs + 1);
		if (nva == 0 || nva > endva) {
			nva = endva;
		}

		rp = &pm->pm_regmap[vr];
		sp = &rp->rg_segmap[vs];

		/* decide how to flush the cache */
		npg = (nva - va) >> PGSHIFT;
		if (npg > PMAP_SFL_THRESHOLD) {
			/* flush the whole segment */
			perpage = 0;
			if (CACHEINFO.c_vactype != VAC_NONE) {
				cache_flush_segment(vr, vs, 0);
			}
		} else {
			/*
			 * flush each page individually;
			 * some never need flushing
			 */
			perpage = (CACHEINFO.c_vactype != VAC_NONE);
		}
		for (; va < nva; va += NBPG) {
			tpte = sp->sg_pte[VA_SUN4M_VPG(va)];
			if ((tpte & SRMMU_TETYPE) != SRMMU_TEPTE)
				continue;

			if ((tpte & SRMMU_PGTYPE) == PG_SUN4M_OBMEM) {
				/* if cacheable, flush page as needed */
				if (perpage && (tpte & SRMMU_PG_C))
					cache_flush_page(va, 0);
			}
			setpgt4m_va(va, &sp->sg_pte[VA_SUN4M_VPG(va)],
				 SRMMU_TEINVALID, 1, 0, CPUSET_ALL);
		}
	}
	splx(s);
}

/*
 * Change protection on a range of kernel addresses.
 */
void
pmap_kprotect4m(vaddr_t va, vsize_t size, vm_prot_t prot)
{
	struct pmap *pm = pmap_kernel();
	int pte, newprot, s;
	struct regmap *rp;
	struct segmap *sp;

	size = roundup(size,NBPG);
	newprot = pte_kprot4m(prot);

	/*
	 * The kernel pmap doesn't need to be locked, but the demap lock
	 * in updatepte() requires interrupt protection.
	 */
	s = splvm();

	while (size > 0) {
		rp = &pm->pm_regmap[VA_VREG(va)];
		sp = &rp->rg_segmap[VA_VSEG(va)];
		pte = sp->sg_pte[VA_SUN4M_VPG(va)];

		/*
		 * Flush cache if page has been referenced to
		 * avoid stale protection bits in the cache tags.
		 */
		if ((pte & (SRMMU_PG_C|SRMMU_PGTYPE)) ==
		    (SRMMU_PG_C|PG_SUN4M_OBMEM))
			cache_flush_page(va, 0);

		setpgt4m_va(va, &sp->sg_pte[VA_SUN4M_VPG(va)],
			 (pte & ~SRMMU_PROT_MASK) | newprot,
			 1, pm->pm_ctxnum, PMAP_CPUSET(pm));

		va += NBPG;
		size -= NBPG;
	}
	splx(s);
}
#endif /* SUN4M || SUN4D */

/*
 * Clear the wiring attribute for a map/virtual-address pair.
 */
/* ARGSUSED */
void
pmap_unwire(struct pmap *pm, vaddr_t va)
{
	int vr, vs, *ptep;
	struct regmap *rp;
	struct segmap *sp;
	bool owired;

	vr = VA_VREG(va);
	vs = VA_VSEG(va);
	rp = &pm->pm_regmap[vr];
	sp = &rp->rg_segmap[vs];

	owired = false;
	if (CPU_HAS_SUNMMU) {
		ptep = &sp->sg_pte[VA_VPG(va)];
		owired = *ptep & PG_WIRED;
		*ptep &= ~PG_WIRED;
	}
	if (CPU_HAS_SRMMU) {
		owired = sp->sg_wiremap & (1 << VA_SUN4M_VPG(va));
		sp->sg_wiremap &= ~(1 << VA_SUN4M_VPG(va));
	}
	if (!owired) {
		pmap_stats.ps_useless_changewire++;
		return;
	}

	pm->pm_stats.wired_count--;
#if defined(SUN4) || defined(SUN4C)
	if (CPU_HAS_SUNMMU && --sp->sg_nwired <= 0) {
#ifdef DIAGNOSTIC
		if (sp->sg_nwired > sp->sg_npte || sp->sg_nwired < 0)
			panic("pmap_unwire: pm %p, va %lx: nleft=%d, nwired=%d",
				pm, va, sp->sg_npte, sp->sg_nwired);
#endif
		if (sp->sg_pmeg != seginval)
			mmu_pmeg_unlock(sp->sg_pmeg);
	}
#endif /* SUN4 || SUN4C */
}

/*
 * Extract the physical page address associated
 * with the given map/virtual_address pair.
 * GRR, the vm code knows; we should not have to do this!
 */

#if defined(SUN4) || defined(SUN4C)
bool
pmap_extract4_4c(struct pmap *pm, vaddr_t va, paddr_t *pap)
{
	int vr, vs;
	struct regmap *rp;
	struct segmap *sp;
	int pte, *ptep;

	vr = VA_VREG(va);
	vs = VA_VSEG(va);
	rp = &pm->pm_regmap[vr];
	if (rp->rg_segmap == NULL) {
#ifdef DEBUG
		if (pmapdebug & PDB_FOLLOW)
			printf("pmap_extract: invalid segment (%d)\n", vr);
#endif
		return (false);
	}
	sp = &rp->rg_segmap[vs];
	ptep = sp->sg_pte;
	if (ptep == NULL) {
#ifdef DEBUG
		if (pmapdebug & PDB_FOLLOW)
			printf("pmap_extract: invalid segment\n");
#endif
		return (false);
	}
	pte = ptep[VA_VPG(va)];

	if ((pte & PG_V) == 0) {
#ifdef DEBUG
		if (pmapdebug & PDB_FOLLOW)
			printf("pmap_extract: invalid pte\n");
#endif
		return (false);
	}
	pte &= PG_PFNUM;
	if (pap != NULL)
		*pap = (pte << PGSHIFT) | (va & PGOFSET);
	return (true);
}
#endif /* SUN4 || SUN4C */

#if defined(SUN4M) || defined(SUN4D)	/* SRMMU version of pmap_extract */
/*
 * Extract the physical page address associated
 * with the given map/virtual_address pair.
 * GRR, the vm code knows; we should not have to do this!
 */
bool
pmap_extract4m(struct pmap *pm, vaddr_t va, paddr_t *pap)
{
	struct regmap *rp;
	struct segmap *sp;
	int pte;
	int vr, vs, s, v = false;

	vr = VA_VREG(va);
	vs = VA_VSEG(va);

	/*
	 * The kernel pmap doesn't need to be locked, but the demap lock
	 * requires interrupt protection.
	 */
	s = splvm();
	if (pm != pmap_kernel()) {
		PMAP_LOCK();
	}

	rp = &pm->pm_regmap[vr];
	if (rp->rg_segmap == NULL) {
#ifdef DEBUG
		if (pmapdebug & PDB_FOLLOW)
			printf("pmap_extract: no segmap\n");
#endif
		goto out;
	}

	sp = &rp->rg_segmap[vs];
	if (sp->sg_pte == NULL) {
#ifdef DEBUG
		if (pmapdebug & PDB_FOLLOW)
			printf("pmap_extract: no ptes\n");
#endif
		goto out;
	}

	pte = sp->sg_pte[VA_SUN4M_VPG(va)];
	if ((pte & SRMMU_TETYPE) != SRMMU_TEPTE) {
#ifdef DEBUG
		if (pmapdebug & PDB_FOLLOW)
			printf("pmap_extract: invalid pte of type %d\n",
			       pte & SRMMU_TETYPE);
#endif
		/*
		 * We can read a spurious invalid pte if the system is in
		 * the middle of the PTE update protocol. So, acquire the
		 * demap lock and retry.
		 */
		mutex_spin_enter(&demap_lock);
		pte = sp->sg_pte[VA_SUN4M_VPG(va)];
		mutex_spin_exit(&demap_lock);
		if ((pte & SRMMU_TETYPE) != SRMMU_TEPTE)
			goto out;
	}
#ifdef DIAGNOSTIC
	if (pm != pmap_kernel() && sp->sg_npte <= 0)
		panic("pmap_extract: pm %p: npte = %d\n", pm, sp->sg_npte);
#endif

	if (pap != NULL)
		*pap = ptoa((pte & SRMMU_PPNMASK) >> SRMMU_PPNSHIFT) |
		    VA_OFF(va);

	v = true;
out:
	if (pm != pmap_kernel()) {
		PMAP_UNLOCK();
	}
	splx(s);
	return (v);
}
#endif /* sun4m */

int pmap_copy_disabled=0;

/*
 * Copy the range specified by src_addr/len
 * from the source map to the range dst_addr/len
 * in the destination map.
 *
 * This routine is only advisory and need not do anything.
 */
/* ARGSUSED */
void
pmap_copy(struct pmap *dst_pmap, struct pmap *src_pmap,
	  vaddr_t dst_addr, vsize_t len, vaddr_t src_addr)
{
#if notyet
	struct regmap *rp;
	struct segmap *sp;

	if (pmap_copy_disabled)
		return;
#ifdef DIAGNOSTIC
	if (VA_OFF(src_addr) != 0)
		printf("pmap_copy: addr not page aligned: 0x%lx\n", src_addr);
	if ((len & (NBPG-1)) != 0)
		printf("pmap_copy: length not page aligned: 0x%lx\n", len);
#endif

	if (src_pmap == NULL)
		return;

	if (CPU_HAS_SRMMU) {
		int i, npg, pte;
		paddr_t pa;

		npg = len >> PGSHIFT;
		for (i = 0; i < npg; i++) {
			if ((rp = src_pmap->pm_regmap) == NULL)
				continue;
			rp += VA_VREG(src_addr);

			if ((sp = rp->rg_segmap) == NULL)
				continue;
			sp += VA_VSEG(src_addr);
			if (sp->sg_npte == 0)
				continue;

			pte = sp->sg_pte[VA_SUN4M_VPG(src_addr)];
			if ((pte & SRMMU_TETYPE) != SRMMU_TEPTE)
				continue;

			pa = ptoa((pte & SRMMU_PPNMASK) >> SRMMU_PPNSHIFT);
			pmap_enter(dst_pmap, dst_addr,
				   pa,
				   /* XXX - need to copy VM_PROT_EXEC too */
				   (pte & PPROT_WRITE)
					? (VM_PROT_WRITE | VM_PROT_READ)
					: VM_PROT_READ,
				   0);
			src_addr += NBPG;
			dst_addr += NBPG;
		}
		pmap_update(dst_pmap);
	}
#endif
}

/*
 * Garbage collects the physical map system for
 * pages which are no longer used.
 * Success need not be guaranteed -- that is, there
 * may well be pages which are not referenced, but
 * others may be collected.
 * Called by the pageout daemon when pages are scarce.
 */
/* ARGSUSED */
void
pmap_collect(struct pmap *pm)
{
}

#if defined(SUN4) || defined(SUN4C)
/*
 * Clear the modify bit for the given physical page.
 */
bool
pmap_clear_modify4_4c(struct vm_page *pg)
{
	bool rv;

	(void) pv_syncflags4_4c(pg);
	rv = VM_MDPAGE_PVHEAD(pg)->pv_flags & PV_MOD;
	VM_MDPAGE_PVHEAD(pg)->pv_flags &= ~PV_MOD;
	return (rv);
}

/*
 * Tell whether the given physical page has been modified.
 */
bool
pmap_is_modified4_4c(struct vm_page *pg)
{

	return (VM_MDPAGE_PVHEAD(pg)->pv_flags & PV_MOD ||
		pv_syncflags4_4c(pg) & PV_MOD);
}

/*
 * Clear the reference bit for the given physical page.
 */
bool
pmap_clear_reference4_4c(struct vm_page *pg)
{
	bool rv;

	(void) pv_syncflags4_4c(pg);
	rv = VM_MDPAGE_PVHEAD(pg)->pv_flags & PV_REF;
	VM_MDPAGE_PVHEAD(pg)->pv_flags &= ~PV_REF;
	return (rv);
}

/*
 * Tell whether the given physical page has been referenced.
 */
bool
pmap_is_referenced4_4c(struct vm_page *pg)
{

	return (VM_MDPAGE_PVHEAD(pg)->pv_flags & PV_REF ||
		pv_syncflags4_4c(pg) & PV_REF);
}
#endif /* SUN4 || SUN4C */

#if defined(SUN4M) || defined(SUN4D)

/*
 * SRMMU versions of bit test/set routines
 *
 * Note that the 4m-specific routines should eventually service these
 * requests from their page tables, and the whole pvlist bit mess should
 * be dropped for the 4m (unless this causes a performance hit from
 * tracing down pagetables/regmap/segmaps).
 */

/*
 * Clear the modify bit for the given physical page.
 */
bool
pmap_clear_modify4m(struct vm_page *pg)
{
	bool rv;

	(void) pv_syncflags4m(pg);
	rv = VM_MDPAGE_PVHEAD(pg)->pv_flags & PV_MOD4M;
	VM_MDPAGE_PVHEAD(pg)->pv_flags &= ~PV_MOD4M;
	return (rv);
}

/*
 * Tell whether the given physical page has been modified.
 */
bool
pmap_is_modified4m(struct vm_page *pg)
{

	return (VM_MDPAGE_PVHEAD(pg)->pv_flags & PV_MOD4M ||
		pv_syncflags4m(pg) & PV_MOD4M);
}

/*
 * Clear the reference bit for the given physical page.
 */
bool
pmap_clear_reference4m(struct vm_page *pg)
{
	bool rv;

	(void) pv_syncflags4m(pg);
	rv = VM_MDPAGE_PVHEAD(pg)->pv_flags & PV_REF4M;
	VM_MDPAGE_PVHEAD(pg)->pv_flags &= ~PV_REF4M;
	return (rv);
}

/*
 * Tell whether the given physical page has been referenced.
 */
bool
pmap_is_referenced4m(struct vm_page *pg)
{

	return (VM_MDPAGE_PVHEAD(pg)->pv_flags & PV_REF4M ||
		pv_syncflags4m(pg) & PV_REF4M);
}
#endif /* SUN4M || SUN4D */

/*
 * Fill the given MI physical page with zero bytes.
 *
 * We avoid stomping on the cache.
 * XXX	might be faster to use destination's context and allow cache to fill?
 */

#if defined(SUN4) || defined(SUN4C)

void
pmap_zero_page4_4c(paddr_t pa)
{
	struct vm_page *pg;
	void *va;
	int pte;

	if ((pg = PHYS_TO_VM_PAGE(pa)) != NULL) {
		/*
		 * The following might not be necessary since the page
		 * is being cleared because it is about to be allocated,
		 * i.e., is in use by no one.
		 */
		pv_flushcache4_4c(pg);
	}
	pte = PG_V | PG_S | PG_W | PG_NC | (atop(pa) & PG_PFNUM);

	va = cpuinfo.vpage[0];
	setpte4(va, pte);
	qzero(va, NBPG);
	setpte4(va, 0);
}

/*
 * Copy the given MI physical source page to its destination.
 *
 * We avoid stomping on the cache as above (with same `XXX' note).
 * We must first flush any write-back cache for the source page.
 * We go ahead and stomp on the kernel's virtual cache for the
 * source page, since the cache can read memory MUCH faster than
 * the processor.
 */
void
pmap_copy_page4_4c(paddr_t src, paddr_t dst)
{
	struct vm_page *pg;
	char *sva, *dva;
	int spte, dpte;

	if ((pg = PHYS_TO_VM_PAGE(src)) != NULL) {
		if (CACHEINFO.c_vactype == VAC_WRITEBACK)
			pv_flushcache4_4c(pg);
	}
	spte = PG_V | PG_S | (atop(src) & PG_PFNUM);

	if ((pg = PHYS_TO_VM_PAGE(dst)) != NULL) {
		/* similar `might not be necessary' comment applies */
		if (CACHEINFO.c_vactype != VAC_NONE)
			pv_flushcache4_4c(pg);
	}
	dpte = PG_V | PG_S | PG_W | PG_NC | (atop(dst) & PG_PFNUM);

	sva = cpuinfo.vpage[0];
	dva = cpuinfo.vpage[1];
	setpte4(sva, spte);
	setpte4(dva, dpte);
	qcopy(sva, dva, NBPG);	/* loads cache, so we must ... */
	cache_flush_page((vaddr_t)sva, getcontext4());
	setpte4(sva, 0);
	setpte4(dva, 0);
}
#endif /* SUN4 || SUN4C */

#if defined(SUN4M) || defined(SUN4D)	/* SRMMU version of copy/zero routines */
/*
 * Fill the given MI physical page with zero bytes.
 *
 * We avoid stomping on the cache.
 * XXX	might be faster to use destination's context and allow cache to fill?
 */
void
pmap_zero_page4m(paddr_t pa)
{
	struct vm_page *pg;
	void *va;
	int pte;

	if ((pg = PHYS_TO_VM_PAGE(pa)) != NULL) {
		/*
		 * The following VAC flush might not be necessary since the
		 * page is being cleared because it is about to be allocated,
		 * i.e., is in use by no one.
		 * In the case of a physical cache, a flush (or just an
		 * invalidate, if possible) is usually necessary when using
		 * uncached access to clear it.
		 */
		if (CACHEINFO.c_vactype != VAC_NONE)
			pv_flushcache4m(pg);
		else
			pcache_flush_page(pa, 1);
	}
	pte = SRMMU_TEPTE | PPROT_N_RWX | (pa >> SRMMU_PPNPASHIFT);
	if (cpuinfo.flags & CPUFLG_CACHE_MANDATORY)
		pte |= SRMMU_PG_C;

	va = cpuinfo.vpage[0];
	setpgt4m(cpuinfo.vpage_pte[0], pte);
	qzero(va, NBPG);
	/*
	 * Remove temporary mapping (which is kernel-only, so the
	 * context used for TLB flushing does not matter)
	 */
	sp_tlb_flush((int)va, 0, ASI_SRMMUFP_L3);
	setpgt4m(cpuinfo.vpage_pte[0], SRMMU_TEINVALID);
}

/*
 * Viking/MXCC specific version of pmap_zero_page
 */
void
pmap_zero_page_viking_mxcc(paddr_t pa)
{
	u_int offset;
	u_int stream_data_addr = MXCC_STREAM_DATA;
	uint64_t v = (uint64_t)pa;

	/* Load MXCC stream data register with 0 (bottom 32 bytes only) */
	stda(stream_data_addr+0, ASI_CONTROL, 0);
	stda(stream_data_addr+8, ASI_CONTROL, 0);
	stda(stream_data_addr+16, ASI_CONTROL, 0);
	stda(stream_data_addr+24, ASI_CONTROL, 0);

	/* Then write the stream data register to each block in the page */
	v |= MXCC_STREAM_C;
	for (offset = 0; offset < NBPG; offset += MXCC_STREAM_BLKSZ) {
		stda(MXCC_STREAM_DST, ASI_CONTROL, v | offset);
	}
}

/*
 * HyperSPARC/RT625 specific version of pmap_zero_page
 */
void
pmap_zero_page_hypersparc(paddr_t pa)
{
	struct vm_page *pg;
	void *va;
	int pte;
	int offset;

	/*
	 * We still have to map the page, since ASI_BLOCKFILL
	 * takes virtual addresses. This also means we have to
	 * consider cache aliasing; therefore we still need
	 * to flush the cache here. All we gain is the speed-up
	 * in zero-fill loop itself..
	 */
	if ((pg = PHYS_TO_VM_PAGE(pa)) != NULL) {
		/*
		 * The following might not be necessary since the page
		 * is being cleared because it is about to be allocated,
		 * i.e., is in use by no one.
		 */
		if (CACHEINFO.c_vactype != VAC_NONE)
			pv_flushcache4m(pg);
	}
	pte = SRMMU_TEPTE | SRMMU_PG_C | PPROT_N_RWX | (pa >> SRMMU_PPNPASHIFT);

	va = cpuinfo.vpage[0];
	setpgt4m(cpuinfo.vpage_pte[0], pte);
	for (offset = 0; offset < NBPG; offset += 32) {
		sta((char *)va + offset, ASI_BLOCKFILL, 0);
	}
	/* Remove temporary mapping */
	sp_tlb_flush((int)va, 0, ASI_SRMMUFP_L3);
	setpgt4m(cpuinfo.vpage_pte[0], SRMMU_TEINVALID);
}

/*
 * Copy the given MI physical source page to its destination.
 *
 * We avoid stomping on the cache as above (with same `XXX' note).
 * We must first flush any write-back cache for the source page.
 * We go ahead and stomp on the kernel's virtual cache for the
 * source page, since the cache can read memory MUCH faster than
 * the processor.
 */
void
pmap_copy_page4m(paddr_t src, paddr_t dst)
{
	struct vm_page *pg;
	void *sva, *dva;
	int spte, dpte;

	if ((pg = PHYS_TO_VM_PAGE(src)) != NULL) {
		if (CACHEINFO.c_vactype == VAC_WRITEBACK)
			pv_flushcache4m(pg);
	}

	spte = SRMMU_TEPTE | SRMMU_PG_C | PPROT_N_RX |
		(src >> SRMMU_PPNPASHIFT);

	if ((pg = PHYS_TO_VM_PAGE(dst)) != NULL) {
		/* similar `might not be necessary' comment applies */
		if (CACHEINFO.c_vactype != VAC_NONE)
			pv_flushcache4m(pg);
		else
			pcache_flush_page(dst, 1);
	}

	dpte = SRMMU_TEPTE | PPROT_N_RWX | (dst >> SRMMU_PPNPASHIFT);
	if (cpuinfo.flags & CPUFLG_CACHE_MANDATORY)
		dpte |= SRMMU_PG_C;

	sva = cpuinfo.vpage[0];
	dva = cpuinfo.vpage[1];
	setpgt4m(cpuinfo.vpage_pte[0], spte);
	setpgt4m(cpuinfo.vpage_pte[1], dpte);
	qcopy(sva, dva, NBPG);	/* loads cache, so we must ... */
	cpuinfo.sp_vcache_flush_page((vaddr_t)sva, getcontext4m());
	sp_tlb_flush((int)sva, 0, ASI_SRMMUFP_L3);
	setpgt4m(cpuinfo.vpage_pte[0], SRMMU_TEINVALID);
	sp_tlb_flush((int)dva, 0, ASI_SRMMUFP_L3);
	setpgt4m(cpuinfo.vpage_pte[1], SRMMU_TEINVALID);
}

/*
 * Viking/MXCC specific version of pmap_copy_page
 */
void
pmap_copy_page_viking_mxcc(paddr_t src, paddr_t dst)
{
	u_int offset;
	uint64_t v1 = (uint64_t)src;
	uint64_t v2 = (uint64_t)dst;

	/* Enable cache-coherency */
	v1 |= MXCC_STREAM_C;
	v2 |= MXCC_STREAM_C;

	/* Copy through stream data register */
	for (offset = 0; offset < NBPG; offset += MXCC_STREAM_BLKSZ) {
		stda(MXCC_STREAM_SRC, ASI_CONTROL, v1 | offset);
		stda(MXCC_STREAM_DST, ASI_CONTROL, v2 | offset);
	}
}

/*
 * HyperSPARC/RT625 specific version of pmap_copy_page
 */
void
pmap_copy_page_hypersparc(paddr_t src, paddr_t dst)
{
	struct vm_page *pg;
	void *sva, *dva;
	int spte, dpte;
	int offset;

	/*
	 * We still have to map the pages, since ASI_BLOCKCOPY
	 * takes virtual addresses. This also means we have to
	 * consider cache aliasing; therefore we still need
	 * to flush the cache here. All we gain is the speed-up
	 * in copy loop itself..
	 */

	if ((pg = PHYS_TO_VM_PAGE(src)) != NULL) {
		if (CACHEINFO.c_vactype == VAC_WRITEBACK)
			pv_flushcache4m(pg);
	}

	spte = SRMMU_TEPTE | SRMMU_PG_C | PPROT_N_RX |
		(src >> SRMMU_PPNPASHIFT);

	if ((pg = PHYS_TO_VM_PAGE(dst)) != NULL) {
		/* similar `might not be necessary' comment applies */
		if (CACHEINFO.c_vactype != VAC_NONE)
			pv_flushcache4m(pg);
	}

	dpte = SRMMU_TEPTE | SRMMU_PG_C | PPROT_N_RWX |
		(dst >> SRMMU_PPNPASHIFT);

	sva = cpuinfo.vpage[0];
	dva = cpuinfo.vpage[1];
	setpgt4m(cpuinfo.vpage_pte[0], spte);
	setpgt4m(cpuinfo.vpage_pte[1], dpte);

	for (offset = 0; offset < NBPG; offset += 32) {
		sta((char *)dva + offset, ASI_BLOCKCOPY, (char *)sva + offset);
	}

	sp_tlb_flush((int)sva, 0, ASI_SRMMUFP_L3);
	setpgt4m(cpuinfo.vpage_pte[0], SRMMU_TEINVALID);
	sp_tlb_flush((int)dva, 0, ASI_SRMMUFP_L3);
	setpgt4m(cpuinfo.vpage_pte[1], SRMMU_TEINVALID);
}
#endif /* SUN4M || SUN4D */

/*
 * Turn off cache for a given (va, number of pages).
 *
 * We just assert PG_NC for each PTE; the addresses must reside
 * in locked kernel space.  A cache flush is also done.
 */
void
kvm_uncache(char *va, int npages)
{
	struct vm_page *pg;
	int pte;

	if (CPU_HAS_SRMMU) {
#if defined(SUN4M) || defined(SUN4D)
		for (; --npages >= 0; va = (char *)va + NBPG) {
			pte = getpte4m((vaddr_t) va);
			if ((pte & SRMMU_TETYPE) != SRMMU_TEPTE)
				panic("kvm_uncache: table entry not pte");

			if ((pte & SRMMU_PGTYPE) == PG_SUN4M_OBMEM) {
				if ((pg = pvhead4m(pte)) != NULL) {
					pv_uncache(pg);
					return;
				}
				cache_flush_page((vaddr_t)va, 0);
			}

			pte &= ~SRMMU_PG_C;
			setpte4m((vaddr_t)va, pte);
		}
#endif
	} else {
#if defined(SUN4) || defined(SUN4C)
		for (; --npages >= 0; va += NBPG) {
			pte = getpte4(va);
			if ((pte & PG_V) == 0)
				panic("kvm_uncache !pg_v");

			if ((pte & PG_TYPE) == PG_OBMEM) {
				if ((pg = pvhead4_4c(pte)) != NULL) {
					pv_uncache(pg);
					return;
				}
				cache_flush_page((vaddr_t)va, 0);
			}
			pte |= PG_NC;
			setpte4(va, pte);
		}
#endif
	}
}

#if 0 /* not used */
/*
 * Turn on IO cache for a given (va, number of pages).
 *
 * We just assert PG_NC for each PTE; the addresses must reside
 * in locked kernel space.  A cache flush is also done.
 */
void
kvm_iocache(char *va, int npages)
{

#if defined(SUN4M)
	if (CPU_ISSUN4M) /* %%%: Implement! */
		panic("kvm_iocache: 4m iocache not implemented");
#endif
#if defined(SUN4D)
	if (CPU_ISSUN4D) /* %%%: Implement! */
		panic("kvm_iocache: 4d iocache not implemented");
#endif
#if defined(SUN4) || defined(SUN4C)
	for (; --npages >= 0; va += NBPG) {
		int pte = getpte4(va);
		if ((pte & PG_V) == 0)
			panic("kvm_iocache !pg_v");
		pte |= PG_IOC;
		setpte4(va, pte);
	}
#endif
}
#endif

/*
 * Find first virtual address >= *va that is
 * least likely to cause cache aliases.
 * (This will just seg-align mappings.)
 */
void
pmap_prefer(vaddr_t foff, vaddr_t *vap)
{
	vaddr_t va = *vap;
	long d, m;

	if (VA_INHOLE(va))
		va = MMU_HOLE_END;

	m = CACHE_ALIAS_DIST;
	if (m == 0)		/* m=0 => no cache aliasing */
		return;

	d = foff - va;
	d &= (m - 1);
	*vap = va + d;
}

void
pmap_redzone(void)
{

	pmap_remove(pmap_kernel(), KERNBASE, KERNBASE+NBPG);
}

/*
 * Activate the address space for the specified process.  If the
 * process is the current process, load the new MMU context.
 */
void
pmap_activate(struct lwp *l)
{
	pmap_t pm = l->l_proc->p_vmspace->vm_map.pmap;
	int s;

	/*
	 * This is essentially the same thing that happens in cpu_switch()
	 * when the newly selected process is about to run, except that we
	 * have to make sure to clean the register windows before we set
	 * the new context.
	 */

	s = splvm();
	if (l == curlwp) {
		write_user_windows();
		if (pm->pm_ctx == NULL) {
			ctx_alloc(pm);	/* performs setcontext() */
		} else {
			/* Do any cache flush needed on context switch */
			(*cpuinfo.pure_vcache_flush)();
			setcontext(pm->pm_ctxnum);
		}
#if defined(MULTIPROCESSOR)
		if (pm != pmap_kernel())
			PMAP_SET_CPUSET(pm, &cpuinfo);
#endif
	}
	splx(s);
}

/*
 * Deactivate the address space of the specified process.
 */
void
pmap_deactivate(struct lwp *l)
{
#if defined(MULTIPROCESSOR)
	pmap_t pm;
	struct proc *p;

	p = l->l_proc;
	if (p->p_vmspace &&
	    (pm = p->p_vmspace->vm_map.pmap) != pmap_kernel()) {
#if defined(SUN4M) || defined(SUN4D)
		if (pm->pm_ctx && CPU_HAS_SRMMU)
			sp_tlb_flush(0, pm->pm_ctxnum, ASI_SRMMUFP_L0);
#endif

		/* we no longer need broadcast tlb flushes for this pmap. */
		PMAP_CLR_CPUSET(pm, &cpuinfo);
	}
#endif
}

#ifdef DEBUG
/*
 * Check consistency of a pmap (time consuming!).
 */
void
pm_check(char *s, struct pmap *pm)
{

	if (pm == pmap_kernel())
		pm_check_k(s, pm);
	else
		pm_check_u(s, pm);
}

void
pm_check_u(char *s, struct pmap *pm)
{
	struct regmap *rp;
	struct segmap *sp;
	int cpu, n, vs, vr, j, m, *pte;

	cpu = cpuinfo.ci_cpuid;

	if (pm->pm_regmap == NULL)
		panic("%s: CPU %d: CHK(pmap %p): no region mapping",
			s, cpu, pm);

#if defined(SUN4M) || defined(SUN4D)
	if (CPU_HAS_SRMMU &&
	    (pm->pm_reg_ptps[cpu] == NULL ||
	     pm->pm_reg_ptps_pa[cpu] != VA2PA((void *)pm->pm_reg_ptps[cpu])))
		panic("%s: CPU %d: CHK(pmap %p): no SRMMU region table or bad pa: "
		      "tblva=%p, tblpa=0x%x",
			s, cpu, pm, pm->pm_reg_ptps[cpu], pm->pm_reg_ptps_pa[cpu]);

	if (CPU_HAS_SRMMU && pm->pm_ctx != NULL &&
	    (cpuinfo.ctx_tbl[pm->pm_ctxnum] != ((VA2PA((void *)pm->pm_reg_ptps[cpu])
					      >> SRMMU_PPNPASHIFT) |
					     SRMMU_TEPTD)))
	    panic("%s: CPU %d: CHK(pmap %p): SRMMU region table at 0x%x not installed "
		  "for context %d", s, cpu, pm, pm->pm_reg_ptps_pa[cpu], pm->pm_ctxnum);
#endif

	for (vr = 0; vr < NUREG; vr++) {
		rp = &pm->pm_regmap[vr];
		if (rp->rg_nsegmap == 0)
			continue;
		if (rp->rg_segmap == NULL)
			panic("%s: CPU %d: CHK(vr %d): nsegmap = %d; sp==NULL",
				s, cpu, vr, rp->rg_nsegmap);
#if defined(SUN4M) || defined(SUN4D)
		if (CPU_HAS_SRMMU && rp->rg_seg_ptps == NULL)
		    panic("%s: CPU %d: CHK(vr %d): nsegmap=%d; no SRMMU segment table",
			  s, cpu, vr, rp->rg_nsegmap);
		if (CPU_HAS_SRMMU &&
		    pm->pm_reg_ptps[cpu][vr] != ((VA2PA((void *)rp->rg_seg_ptps) >>
					    SRMMU_PPNPASHIFT) | SRMMU_TEPTD))
		    panic("%s: CPU %d: CHK(vr %d): SRMMU segtbl not installed",
				s, cpu, vr);
#endif
		if ((unsigned int)rp < KERNBASE)
			panic("%s: CPU %d: rp=%p", s, cpu, rp);
		n = 0;
		for (vs = 0; vs < NSEGRG; vs++) {
			sp = &rp->rg_segmap[vs];
			if ((unsigned int)sp < KERNBASE)
				panic("%s: CPU %d: sp=%p", s, cpu, sp);
			if (sp->sg_npte != 0) {
				n++;
				if (sp->sg_pte == NULL)
					panic("%s: CPU %d: CHK(vr %d, vs %d): npte=%d, "
					   "pte=NULL", s, cpu, vr, vs, sp->sg_npte);
#if defined(SUN4M) || defined(SUN4D)
				if (CPU_HAS_SRMMU &&
				    rp->rg_seg_ptps[vs] !=
				     ((VA2PA((void *)sp->sg_pte)
					>> SRMMU_PPNPASHIFT) |
				       SRMMU_TEPTD))
				    panic("%s: CPU %d: CHK(vr %d, vs %d): SRMMU page "
					  "table not installed correctly",
						s, cpu, vr, vs);
#endif
				pte=sp->sg_pte;
				m = 0;
				for (j=0; j<NPTESG; j++,pte++)
				    if ((CPU_HAS_SRMMU
					 ?((*pte & SRMMU_TETYPE) == SRMMU_TEPTE)
					 :(*pte & PG_V)))
					m++;
				if (m != sp->sg_npte)
					printf("%s: CPU %d: user CHK(vr %d, vs %d): "
					    "npte(%d) != # valid(%d)\n",
						s, cpu, vr, vs, sp->sg_npte, m);
			}
		}
		if (n != rp->rg_nsegmap)
			panic("%s: CPU %d: CHK(vr %d): inconsistent "
				"# of pte's: %d, should be %d",
				s, cpu, vr, rp->rg_nsegmap, n);
	}
	return;
}

/* Note: not as extensive as pm_check_u. */
void
pm_check_k(char *s, struct pmap *pm)
{
	struct regmap *rp;
	int cpu, vr, vs, n;

	cpu = cpu_number();

	if (pm->pm_regmap == NULL)
		panic("%s: CHK(pmap %p): no region mapping", s, pm);

#if defined(SUN4M) || defined(SUN4D)
	if (CPU_HAS_SRMMU &&
	    (pm->pm_reg_ptps[cpu] == NULL ||
	     pm->pm_reg_ptps_pa[cpu] != VA2PA((void *)pm->pm_reg_ptps[cpu])))
	    panic("%s: CPU %d: CHK(pmap %p): no SRMMU region table or bad pa: tblva=%p, tblpa=0x%x",
		  s, cpu, pm, pm->pm_reg_ptps[cpu], pm->pm_reg_ptps_pa[cpu]);

	if (CPU_HAS_SRMMU &&
	    (cpuinfo.ctx_tbl[0] != ((VA2PA((void *)pm->pm_reg_ptps[cpu]) >>
					     SRMMU_PPNPASHIFT) | SRMMU_TEPTD)))
	    panic("%s: CPU %d: CHK(pmap %p): SRMMU region table at 0x%x not installed "
		  "for context %d", s, cpu, pm, pm->pm_reg_ptps_pa[cpu], 0);
#endif
	for (vr = NUREG; vr < NUREG+NKREG; vr++) {
		rp = &pm->pm_regmap[vr];
		if (rp->rg_segmap == NULL)
			panic("%s: CPU %d: CHK(vr %d): nsegmap = %d; sp==NULL",
				s, cpu, vr, rp->rg_nsegmap);
		if (rp->rg_nsegmap == 0)
			continue;
#if defined(SUN4M) || defined(SUN4D)
		if (CPU_HAS_SRMMU && rp->rg_seg_ptps == NULL)
		    panic("%s: CPU %d: CHK(vr %d): nsegmap=%d; no SRMMU segment table",
			  s, cpu, vr, rp->rg_nsegmap);

		if (CPU_HAS_SRMMU && vr != NUREG /* 1st kseg is per CPU */ &&
		    pm->pm_reg_ptps[cpu][vr] != ((VA2PA((void *)rp->rg_seg_ptps) >>
					    SRMMU_PPNPASHIFT) | SRMMU_TEPTD))
		    panic("%s: CPU %d: CHK(vr %d): SRMMU segtbl not installed",
				s, cpu, vr);
#endif
		if (CPU_HAS_SRMMU) {
			n = NSEGRG;
		} else {
			for (n = 0, vs = 0; vs < NSEGRG; vs++) {
				if (rp->rg_segmap[vs].sg_npte)
					n++;
			}
		}
		if (n != rp->rg_nsegmap)
			printf("%s: CPU %d: kernel CHK(vr %d): inconsistent "
				"# of pte's: %d, should be %d\n",
				s, cpu, vr, rp->rg_nsegmap, n);
	}
	return;
}
#endif

/*
 * Return the number of disk blocks that pmap_dumpmmu() will dump.
 */
int
pmap_dumpsize(void)
{
	int	sz;

	sz = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t));
	sz += npmemarr * sizeof(phys_ram_seg_t);
	sz += sizeof(kernel_segmap_store);

	if (CPU_HAS_SUNMMU)
		/* For each pmeg in the MMU, we'll write NPTESG PTEs. */
		sz += (seginval + 1) * NPTESG * sizeof(int);

	return btodb(sz + DEV_BSIZE - 1);
}

/*
 * Write the core dump headers and MD data to the dump device.
 * We dump the following items:
 *
 *	kcore_seg_t		 MI header defined in <sys/kcore.h>)
 *	cpu_kcore_hdr_t		 MD header defined in <machine/kcore.h>)
 *	phys_ram_seg_t[npmemarr] physical memory segments
 *	segmap_t[NKREG*NSEGRG]	 the kernel's segment map
 *	the MMU pmegs on sun4/sun4c
 */
int
pmap_dumpmmu(int (*dump)(dev_t, daddr_t, void *, size_t),
	     daddr_t blkno)
{
	kcore_seg_t	*ksegp;
	cpu_kcore_hdr_t	*kcpup;
	phys_ram_seg_t	memseg;
	int		error = 0;
	int		i, memsegoffset, segmapoffset, pmegoffset;
	int		buffer[dbtob(1) / sizeof(int)];
	int		*bp, *ep;
#if defined(SUN4C) || defined(SUN4)
	int	pmeg;
#endif

#define EXPEDITE(p,n) do {						\
	int *sp = (int *)(p);						\
	int sz = (n);							\
	while (sz > 0) {						\
		*bp++ = *sp++;						\
		if (bp >= ep) {						\
			error = (*dump)(dumpdev, blkno,			\
					(void *)buffer, dbtob(1));	\
			if (error != 0)					\
				return (error);				\
			++blkno;					\
			bp = buffer;					\
		}							\
		sz -= 4;						\
	}								\
} while (0)

	setcontext(0);

	/* Setup bookkeeping pointers */
	bp = buffer;
	ep = &buffer[sizeof(buffer) / sizeof(buffer[0])];

	/* Fill in MI segment header */
	ksegp = (kcore_seg_t *)bp;
	CORE_SETMAGIC(*ksegp, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	ksegp->c_size = dbtob(pmap_dumpsize()) - ALIGN(sizeof(kcore_seg_t));

	/* Fill in MD segment header (interpreted by MD part of libkvm) */
	kcpup = (cpu_kcore_hdr_t *)((int)bp + ALIGN(sizeof(kcore_seg_t)));
	kcpup->cputype = cputyp;
	kcpup->kernbase = KERNBASE;
	kcpup->nmemseg = npmemarr;
	kcpup->memsegoffset = memsegoffset = ALIGN(sizeof(cpu_kcore_hdr_t));
	kcpup->nsegmap = NKREG*NSEGRG;
	kcpup->segmapoffset = segmapoffset =
		memsegoffset + npmemarr * sizeof(phys_ram_seg_t);

	kcpup->npmeg = (CPU_HAS_SUNMMU) ? seginval + 1 : 0;
	kcpup->pmegoffset = pmegoffset =
		segmapoffset + kcpup->nsegmap * sizeof(struct segmap);

	/* Note: we have assumed everything fits in buffer[] so far... */
	bp = (int *)((int)kcpup + ALIGN(sizeof(cpu_kcore_hdr_t)));

#if 0
	/* Align storage for upcoming quad-aligned segment array */
	while (bp != (int *)ALIGN(bp)) {
		int dummy = 0;
		EXPEDITE(&dummy, 4);
	}
#endif

	for (i = 0; i < npmemarr; i++) {
		memseg.start = pmemarr[i].addr;
		memseg.size = pmemarr[i].len;
		EXPEDITE((void *)&memseg, sizeof(phys_ram_seg_t));
	}

	EXPEDITE(&kernel_segmap_store, sizeof(kernel_segmap_store));

	if (CPU_HAS_SRMMU)
		goto out;

#if defined(SUN4C) || defined(SUN4)
	/*
	 * dump page table entries
	 *
	 * We dump each pmeg in order (by segment number).  Since the MMU
	 * automatically maps the given virtual segment to a pmeg we must
	 * iterate over the segments by incrementing an unused segment slot
	 * in the MMU.  This fixed segment number is used in the virtual
	 * address argument to getpte().
	 */

	/*
	 * Go through the pmegs and dump each one.
	 */
	for (pmeg = 0; pmeg <= seginval; ++pmeg) {
		int va = 0;

		setsegmap(va, pmeg);
		i = NPTESG;
		do {
			int pte = getpte4(va);
			EXPEDITE(&pte, sizeof(pte));
			va += NBPG;
		} while (--i > 0);
	}
	setsegmap(0, seginval);
#endif

out:
	if (bp != buffer)
		error = (*dump)(dumpdev, blkno++, (void *)buffer, dbtob(1));

	return (error);
}

/*
 * Helper function for debuggers.
 */
void
pmap_writetext(unsigned char *dst, int ch)
{
	int s, pte0, pte, ctx;
	vaddr_t va;

	s = splvm();
	va = (unsigned long)dst & (~PGOFSET);
	cache_flush(dst, 1);

	ctx = getcontext();
	setcontext(0);

#if defined(SUN4M) || defined(SUN4D)
	if (CPU_HAS_SRMMU) {
		pte0 = getpte4m(va);
		if ((pte0 & SRMMU_TETYPE) != SRMMU_TEPTE) {
			splx(s);
			return;
		}
		pte = pte0 | PPROT_WRITE;
		setpte4m(va, pte);
		*dst = (unsigned char)ch;
		setpte4m(va, pte0);

	}
#endif
#if defined(SUN4) || defined(SUN4C)
	if (CPU_ISSUN4C || CPU_ISSUN4) {
		pte0 = getpte4(va);
		if ((pte0 & PG_V) == 0) {
			splx(s);
			return;
		}
		pte = pte0 | PG_W;
		setpte4(va, pte);
		*dst = (unsigned char)ch;
		setpte4(va, pte0);
	}
#endif
	cache_flush(dst, 1);
	setcontext(ctx);
	splx(s);
}

#ifdef EXTREME_DEBUG

void debug_pagetables(void);
void print_fe_map(void);

static void test_region(int, int, int);


void
debug_pagetables(void)
{
	struct promvec *promvec = romp;
	int *regtbl;
	int te;
	int i;

	printf("\nncontext=%d. ", ncontext);
	printf("Context table is at va %p. Level 0 PTP: 0x%x\n",
	       cpuinfo.ctx_tbl, cpuinfo.ctx_tbl[0]);
	printf("Context 0 region table is at va %p, pa 0x%x. Contents:\n",
	       pmap_kernel()->pm_reg_ptps[0], pmap_kernel()->pm_reg_ptps_pa[0]);

	regtbl = pmap_kernel()->pm_reg_ptps[0];

	printf("PROM vector is at %p\n", promvec);
	printf("PROM reboot routine is at %p\n", promvec->pv_reboot);
	printf("PROM abort routine is at %p\n", promvec->pv_abort);
	printf("PROM halt routine is at %p\n", promvec->pv_halt);

	printf("Testing region 0xfe: ");
	test_region(0xfe,0,16*1024*1024);
	printf("Testing region 0xff: ");
	test_region(0xff,0,16*1024*1024);
	printf("Testing kernel region 0x%x: ", VA_VREG(KERNBASE));
	test_region(VA_VREG(KERNBASE), 4096, avail_start);
	cngetc();

	for (i = 0; i < SRMMU_L1SIZE; i++) {
		te = regtbl[i];
		if ((te & SRMMU_TETYPE) == SRMMU_TEINVALID)
		    continue;
		printf("Region 0x%x: PTE=0x%x <%s> L2PA=0x%x kernL2VA=%p\n",
		       i, te, ((te & SRMMU_TETYPE) == SRMMU_TEPTE ? "pte" :
			       ((te & SRMMU_TETYPE) == SRMMU_TEPTD ? "ptd" :
				((te & SRMMU_TETYPE) == SRMMU_TEINVALID ?
				 "invalid" : "reserved"))),
		       (te & ~0x3) << SRMMU_PPNPASHIFT,
		       pmap_kernel()->pm_regmap[i].rg_seg_ptps);
	}
	printf("Press q to halt...\n");
	if (cngetc()=='q')
	    callrom();
}

static u_int
VA2PAsw(int ctx, void *addr, int *pte)
{
	int *curtbl;
	int curpte;

#ifdef EXTREME_EXTREME_DEBUG
	printf("Looking up addr 0x%x in context 0x%x\n",addr,ctx);
#endif
	/* L0 */
	*pte = curpte = cpuinfo.ctx_tbl[ctx];
#ifdef EXTREME_EXTREME_DEBUG
	printf("Got L0 pte 0x%x\n",pte);
#endif
	if ((curpte & SRMMU_TETYPE) == SRMMU_TEPTE) {
		return (((curpte & SRMMU_PPNMASK) << SRMMU_PPNPASHIFT) |
			((u_int)addr & 0xffffffff));
	}
	if ((curpte & SRMMU_TETYPE) != SRMMU_TEPTD) {
		printf("Bad context table entry 0x%x for context 0x%x\n",
		       curpte, ctx);
		return 0;
	}
	/* L1 */
	curtbl = (int *)(((curpte & ~0x3) << 4) | KERNBASE); /* correct for krn */
	*pte = curpte = curtbl[VA_VREG(addr)];
#ifdef EXTREME_EXTREME_DEBUG
	printf("L1 table at 0x%x.\nGot L1 pte 0x%x\n",curtbl,curpte);
#endif
	if ((curpte & SRMMU_TETYPE) == SRMMU_TEPTE)
	    return (((curpte & SRMMU_PPNMASK) << SRMMU_PPNPASHIFT) |
		    ((u_int)addr & 0xffffff));
	if ((curpte & SRMMU_TETYPE) != SRMMU_TEPTD) {
		printf("Bad region table entry 0x%x for region 0x%x\n",
		       curpte, VA_VREG(addr));
		return 0;
	}
	/* L2 */
	curtbl = (int *)(((curpte & ~0x3) << 4) | KERNBASE); /* correct for krn */
	*pte = curpte = curtbl[VA_VSEG(addr)];
#ifdef EXTREME_EXTREME_DEBUG
	printf("L2 table at 0x%x.\nGot L2 pte 0x%x\n",curtbl,curpte);
#endif
	if ((curpte & SRMMU_TETYPE) == SRMMU_TEPTE)
	    return (((curpte & SRMMU_PPNMASK) << SRMMU_PPNPASHIFT) |
		    ((u_int)addr & 0x3ffff));
	if ((curpte & SRMMU_TETYPE) != SRMMU_TEPTD) {
		printf("Bad segment table entry 0x%x for reg 0x%x, seg 0x%x\n",
		       curpte, VA_VREG(addr), VA_VSEG(addr));
		return 0;
	}
	/* L3 */
	curtbl = (int *)(((curpte & ~0x3) << 4) | KERNBASE); /* correct for krn */
	*pte = curpte = curtbl[VA_VPG(addr)];
#ifdef EXTREME_EXTREME_DEBUG
	printf("L3 table at %p.\nGot L3 pte 0x%x\n", curtbl, curpte);
#endif
	if ((curpte & SRMMU_TETYPE) == SRMMU_TEPTE)
	    return (((curpte & SRMMU_PPNMASK) << SRMMU_PPNPASHIFT) |
		    ((u_int)addr & 0xfff));
	else {
		printf("Bad L3 pte 0x%x for reg 0x%x, seg 0x%x, pg 0x%x\n",
		       curpte, VA_VREG(addr), VA_VSEG(addr), VA_VPG(addr));
		return 0;
	}
	printf("Bizarreness with address %p!\n", addr);
}

static void
test_region(int reg, int start, int stop)
{
	int i;
	int addr;
	int pte;
	int ptesw;
/*	int cnt=0;
*/

	for (i = start; i < stop; i += NBPG) {
		addr = (reg << RGSHIFT) | i;
		pte = lda(((u_int)(addr)) | ASI_SRMMUFP_LN, ASI_SRMMUFP);
		if (pte) {
/*			printf("Valid address 0x%x\n",addr);
			if (++cnt == 20) {
				cngetc();
				cnt = 0;
			}
*/
			if (VA2PA((void *)addr) != VA2PAsw(0, (void *)addr, &ptesw)) {
				printf("Mismatch at address 0x%x.\n", addr);
				if (cngetc() == 'q')
					break;
			}
			if (reg == VA_VREG(KERNBASE))
				/* kernel permissions are different */
				continue;
			if ((pte & SRMMU_PROT_MASK) != (ptesw & SRMMU_PROT_MASK)) {
				printf("Mismatched protections at address "
				       "0x%x; pte=0x%x, ptesw=0x%x\n",
				       addr, pte, ptesw);
				if (cngetc() == 'q')
					break;
			}
		}
	}
	printf("done.\n");
}


void
print_fe_map(void)
{
	u_int i, pte;

	printf("map of region 0xfe:\n");
	for (i = 0xfe000000; i < 0xff000000; i += 4096) {
		if (((pte = getpte4m(i)) & SRMMU_TETYPE) != SRMMU_TEPTE)
		    continue;
		printf("0x%x -> 0x%x%x (pte 0x%x)\n", i, pte >> 28,
		       (pte & ~0xff) << 4, pte);
	}
	printf("done\n");
}
#endif /* EXTREME_DEBUG */

#ifdef DDB
int pmap_dump(struct pmap *pm);

int
pmap_dump(struct pmap *pm)
{
	int startvr, endvr, vr, vs, i, n;
	struct regmap *rp;
	struct segmap *sp;

	if (pm == NULL)
		pm = pmap_kernel();

	if (pm == pmap_kernel()) {
		startvr = NUREG;
		endvr = 256;
	} else {
		startvr = 0;
		endvr = NUREG;
	}

	for (vr = startvr; vr < endvr; vr++) {
		rp = &pm->pm_regmap[vr];
		if (rp->rg_nsegmap == 0)
			continue;
		printf("vr %d: %d segments", vr, rp->rg_nsegmap);
		if (rp->rg_segmap == NULL) {
			printf("[no segments]\n");
			continue;
		}
		for (vs = 0; vs < NSEGRG; vs++) {
			sp = &rp->rg_segmap[vs];
			if (sp->sg_npte == 0)
				continue;
			if ((vs & 3) == 0)
				printf("\n   ");
			printf(" %d: n %d w %d p %d,", vs,
				sp->sg_npte, sp->sg_nwired, sp->sg_pmeg);
			if (sp->sg_pte == NULL) {
				printf("[no ptes]");
				continue;
			}
			for (n = 0, i = 0; i < NPTESG; i++) {
				if (CPU_HAS_SUNMMU && sp->sg_pte[i] & PG_WIRED)
					n++;
				if (CPU_HAS_SRMMU && sp->sg_wiremap & (1 << i))
					n++;
			}
			if (n != sp->sg_nwired)
				printf("[wired count %d]", n);
		}
		printf("\n");
	}

	return (0);
}
#endif /* DDB */
