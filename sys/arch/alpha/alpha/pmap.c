/* $NetBSD: pmap.c,v 1.255.8.1 2012/04/17 00:05:54 yamt Exp $ */

/*-
 * Copyright (c) 1998, 1999, 2000, 2001, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Chris G. Demetriou.
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
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)pmap.c	8.6 (Berkeley) 5/27/94
 */

/*
 * DEC Alpha physical map management code.
 *
 * History:
 *
 *	This pmap started life as a Motorola 68851/68030 pmap,
 *	written by Mike Hibler at the University of Utah.
 *
 *	It was modified for the DEC Alpha by Chris Demetriou
 *	at Carnegie Mellon University.
 *
 *	Support for non-contiguous physical memory was added by
 *	Jason R. Thorpe of the Numerical Aerospace Simulation
 *	Facility, NASA Ames Research Center and Chris Demetriou.
 *
 *	Page table management and a major cleanup were undertaken
 *	by Jason R. Thorpe, with lots of help from Ross Harvey of
 *	Avalon Computer Systems and from Chris Demetriou.
 *
 *	Support for the new UVM pmap interface was written by
 *	Jason R. Thorpe.
 *
 *	Support for ASNs was written by Jason R. Thorpe, again
 *	with help from Chris Demetriou and Ross Harvey.
 *
 *	The locking protocol was written by Jason R. Thorpe,
 *	using Chuck Cranor's i386 pmap for UVM as a model.
 *
 *	TLB shootdown code was written by Jason R. Thorpe.
 *
 *	Multiprocessor modifications by Andrew Doran.
 *
 * Notes:
 *
 *	All page table access is done via K0SEG.  The one exception
 *	to this is for kernel mappings.  Since all kernel page
 *	tables are pre-allocated, we can use the Virtual Page Table
 *	to access PTEs that map K1SEG addresses.
 *
 *	Kernel page table pages are statically allocated in
 *	pmap_bootstrap(), and are never freed.  In the future,
 *	support for dynamically adding additional kernel page
 *	table pages may be added.  User page table pages are
 *	dynamically allocated and freed.
 *
 * Bugs/misfeatures:
 *
 *	- Some things could be optimized.
 */

/*
 *	Manages physical address maps.
 *
 *	Since the information managed by this module is
 *	also stored by the logical address mapping module,
 *	this module may throw away valid virtual-to-physical
 *	mappings at almost any time.  However, invalidations
 *	of virtual-to-physical mappings must be done as
 *	requested.
 *
 *	In order to cope with hardware architectures which
 *	make virtual-to-physical map invalidates expensive,
 *	this module may delay invalidate or reduced protection
 *	operations until such time as they are actually
 *	necessary.  This module is given full information as
 *	to which processors are currently using which maps,
 *	and to when physical maps must be made correct.
 */

#include "opt_lockdebug.h"
#include "opt_sysv.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.255.8.1 2012/04/17 00:05:54 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/buf.h>
#include <sys/shm.h>
#include <sys/atomic.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>

#if defined(_PMAP_MAY_USE_PROM_CONSOLE) || defined(MULTIPROCESSOR)
#include <machine/rpb.h>
#endif

#ifdef DEBUG
#define	PDB_FOLLOW	0x0001
#define	PDB_INIT	0x0002
#define	PDB_ENTER	0x0004
#define	PDB_REMOVE	0x0008
#define	PDB_CREATE	0x0010
#define	PDB_PTPAGE	0x0020
#define	PDB_ASN		0x0040
#define	PDB_BITS	0x0080
#define	PDB_COLLECT	0x0100
#define	PDB_PROTECT	0x0200
#define	PDB_BOOTSTRAP	0x1000
#define	PDB_PARANOIA	0x2000
#define	PDB_WIRING	0x4000
#define	PDB_PVDUMP	0x8000

int debugmap = 0;
int pmapdebug = PDB_PARANOIA;
#endif

/*
 * Given a map and a machine independent protection code,
 * convert to an alpha protection code.
 */
#define pte_prot(m, p)	(protection_codes[m == pmap_kernel() ? 0 : 1][p])
static int	protection_codes[2][8];

/*
 * kernel_lev1map:
 *
 *	Kernel level 1 page table.  This maps all kernel level 2
 *	page table pages, and is used as a template for all user
 *	pmap level 1 page tables.  When a new user level 1 page
 *	table is allocated, all kernel_lev1map PTEs for kernel
 *	addresses are copied to the new map.
 *
 *	The kernel also has an initial set of kernel level 2 page
 *	table pages.  These map the kernel level 3 page table pages.
 *	As kernel level 3 page table pages are added, more level 2
 *	page table pages may be added to map them.  These pages are
 *	never freed.
 *
 *	Finally, the kernel also has an initial set of kernel level
 *	3 page table pages.  These map pages in K1SEG.  More level
 *	3 page table pages may be added at run-time if additional
 *	K1SEG address space is required.  These pages are never freed.
 *
 * NOTE: When mappings are inserted into the kernel pmap, all
 * level 2 and level 3 page table pages must already be allocated
 * and mapped into the parent page table.
 */
pt_entry_t	*kernel_lev1map;

/*
 * Virtual Page Table.
 */
static pt_entry_t *VPT;

static struct pmap	kernel_pmap_store
	[(PMAP_SIZEOF(ALPHA_MAXPROCS) + sizeof(struct pmap) - 1)
		/ sizeof(struct pmap)];
struct pmap *const kernel_pmap_ptr = kernel_pmap_store;

paddr_t    	avail_start;	/* PA of first available physical page */
paddr_t		avail_end;	/* PA of last available physical page */
static vaddr_t	virtual_end;	/* VA of last avail page (end of kernel AS) */

static bool pmap_initialized;	/* Has pmap_init completed? */

u_long		pmap_pages_stolen;	/* instrumentation */

/*
 * This variable contains the number of CPU IDs we need to allocate
 * space for when allocating the pmap structure.  It is used to
 * size a per-CPU array of ASN and ASN Generation number.
 */
static u_long 	pmap_ncpuids;

#ifndef PMAP_PV_LOWAT
#define	PMAP_PV_LOWAT	16
#endif
int		pmap_pv_lowat = PMAP_PV_LOWAT;

/*
 * List of all pmaps, used to update them when e.g. additional kernel
 * page tables are allocated.  This list is kept LRU-ordered by
 * pmap_activate().
 */
static TAILQ_HEAD(, pmap) pmap_all_pmaps;

/*
 * The pools from which pmap structures and sub-structures are allocated.
 */
static struct pool_cache pmap_pmap_cache;
static struct pool_cache pmap_l1pt_cache;
static struct pool_cache pmap_pv_cache;

/*
 * Address Space Numbers.
 *
 * On many implementations of the Alpha architecture, the TLB entries and
 * I-cache blocks are tagged with a unique number within an implementation-
 * specified range.  When a process context becomes active, the ASN is used
 * to match TLB entries; if a TLB entry for a particular VA does not match
 * the current ASN, it is ignored (one could think of the processor as
 * having a collection of <max ASN> separate TLBs).  This allows operating
 * system software to skip the TLB flush that would otherwise be necessary
 * at context switch time.
 *
 * Alpha PTEs have a bit in them (PG_ASM - Address Space Match) that
 * causes TLB entries to match any ASN.  The PALcode also provides
 * a TBI (Translation Buffer Invalidate) operation that flushes all
 * TLB entries that _do not_ have PG_ASM.  We use this bit for kernel
 * mappings, so that invalidation of all user mappings does not invalidate
 * kernel mappings (which are consistent across all processes).
 *
 * pmap_next_asn always indicates to the next ASN to use.  When
 * pmap_next_asn exceeds pmap_max_asn, we start a new ASN generation.
 *
 * When a new ASN generation is created, the per-process (i.e. non-PG_ASM)
 * TLB entries and the I-cache are flushed, the generation number is bumped,
 * and pmap_next_asn is changed to indicate the first non-reserved ASN.
 *
 * We reserve ASN #0 for pmaps that use the global kernel_lev1map.  This
 * prevents the following scenario:
 *
 *	* New ASN generation starts, and process A is given ASN #0.
 *
 *	* A new process B (and thus new pmap) is created.  The ASN,
 *	  for lack of a better value, is initialized to 0.
 *
 *	* Process B runs.  It is now using the TLB entries tagged
 *	  by process A.  *poof*
 *
 * In the scenario above, in addition to the processor using using incorrect
 * TLB entires, the PALcode might use incorrect information to service a
 * TLB miss.  (The PALcode uses the recursively mapped Virtual Page Table
 * to locate the PTE for a faulting address, and tagged TLB entires exist
 * for the Virtual Page Table addresses in order to speed up this procedure,
 * as well.)
 *
 * By reserving an ASN for kernel_lev1map users, we are guaranteeing that
 * new pmaps will initially run with no TLB entries for user addresses
 * or VPT mappings that map user page tables.  Since kernel_lev1map only
 * contains mappings for kernel addresses, and since those mappings
 * are always made with PG_ASM, sharing an ASN for kernel_lev1map users is
 * safe (since PG_ASM mappings match any ASN).
 *
 * On processors that do not support ASNs, the PALcode invalidates
 * the TLB and I-cache automatically on swpctx.  We still still go
 * through the motions of assigning an ASN (really, just refreshing
 * the ASN generation in this particular case) to keep the logic sane
 * in other parts of the code.
 */
static u_int	pmap_max_asn;		/* max ASN supported by the system */
					/* next ASN and cur ASN generation */
static struct pmap_asn_info pmap_asn_info[ALPHA_MAXPROCS];

/*
 * Locking:
 *
 *	READ/WRITE LOCKS
 *	----------------
 *
 *	* pmap_main_lock - This lock is used to prevent deadlock and/or
 *	  provide mutex access to the pmap module.  Most operations lock
 *	  the pmap first, then PV lists as needed.  However, some operations,
 *	  such as pmap_page_protect(), lock the PV lists before locking
 *	  the pmaps.  To prevent deadlock, we require a mutex lock on the
 *	  pmap module if locking in the PV->pmap direction.  This is
 *	  implemented by acquiring a (shared) read lock on pmap_main_lock
 *	  if locking pmap->PV and a (exclusive) write lock if locking in
 *	  the PV->pmap direction.  Since only one thread can hold a write
 *	  lock at a time, this provides the mutex.
 *
 *	MUTEXES
 *	-------
 *
 *	* pm_lock (per-pmap) - This lock protects all of the members
 *	  of the pmap structure itself.  This lock will be asserted
 *	  in pmap_activate() and pmap_deactivate() from a critical
 *	  section of mi_switch(), and must never sleep.  Note that
 *	  in the case of the kernel pmap, interrupts which cause
 *	  memory allocation *must* be blocked while this lock is
 *	  asserted.
 *
 *	* pvh_lock (global hash) - These locks protects the PV lists
 *	  for managed pages.
 *
 *	* pmap_all_pmaps_lock - This lock protects the global list of
 *	  all pmaps.  Note that a pm_lock must never be held while this
 *	  lock is held.
 *
 *	* pmap_growkernel_lock - This lock protects pmap_growkernel()
 *	  and the virtual_end variable.
 *
 *	  There is a lock ordering constraint for pmap_growkernel_lock.
 *	  pmap_growkernel() acquires the locks in the following order:
 *
 *		pmap_growkernel_lock (write) -> pmap_all_pmaps_lock ->
 *		    pmap->pm_lock
 *
 *	  We need to ensure consistency between user pmaps and the
 *	  kernel_lev1map.  For this reason, pmap_growkernel_lock must
 *	  be held to prevent kernel_lev1map changing across pmaps
 *	  being added to / removed from the global pmaps list.
 *
 *	Address space number management (global ASN counters and per-pmap
 *	ASN state) are not locked; they use arrays of values indexed
 *	per-processor.
 *
 *	All internal functions which operate on a pmap are called
 *	with the pmap already locked by the caller (which will be
 *	an interface function).
 */
static krwlock_t pmap_main_lock;
static kmutex_t pmap_all_pmaps_lock;
static krwlock_t pmap_growkernel_lock;

#define	PMAP_MAP_TO_HEAD_LOCK()		rw_enter(&pmap_main_lock, RW_READER)
#define	PMAP_MAP_TO_HEAD_UNLOCK()	rw_exit(&pmap_main_lock)
#define	PMAP_HEAD_TO_MAP_LOCK()		rw_enter(&pmap_main_lock, RW_WRITER)
#define	PMAP_HEAD_TO_MAP_UNLOCK()	rw_exit(&pmap_main_lock)

struct {
	kmutex_t lock;
} __aligned(64) static pmap_pvh_locks[64] __aligned(64);

static inline kmutex_t *
pmap_pvh_lock(struct vm_page *pg)
{

	/* Cut bits 11-6 out of page address and use directly as offset. */
	return (kmutex_t *)((uintptr_t)&pmap_pvh_locks +
	    ((uintptr_t)pg & (63 << 6)));
}

#if defined(MULTIPROCESSOR)
/*
 * TLB Shootdown:
 *
 * When a mapping is changed in a pmap, the TLB entry corresponding to
 * the virtual address must be invalidated on all processors.  In order
 * to accomplish this on systems with multiple processors, messages are
 * sent from the processor which performs the mapping change to all
 * processors on which the pmap is active.  For other processors, the
 * ASN generation numbers for that processor is invalidated, so that
 * the next time the pmap is activated on that processor, a new ASN
 * will be allocated (which implicitly invalidates all TLB entries).
 *
 * Note, we can use the pool allocator to allocate job entries
 * since pool pages are mapped with K0SEG, not with the TLB.
 */
struct pmap_tlb_shootdown_job {
	TAILQ_ENTRY(pmap_tlb_shootdown_job) pj_list;
	vaddr_t pj_va;			/* virtual address */
	pmap_t pj_pmap;			/* the pmap which maps the address */
	pt_entry_t pj_pte;		/* the PTE bits */
};

static struct pmap_tlb_shootdown_q {
	TAILQ_HEAD(, pmap_tlb_shootdown_job) pq_head;	/* queue 16b */
	kmutex_t pq_lock;		/* spin lock on queue 16b */
	int pq_pte;			/* aggregate PTE bits 4b */
	int pq_count;			/* number of pending requests 4b */
	int pq_tbia;			/* pending global flush 4b */
	uint8_t pq_pad[64-16-16-4-4-4];	/* pad to 64 bytes */
} pmap_tlb_shootdown_q[ALPHA_MAXPROCS] __aligned(CACHE_LINE_SIZE);

/* If we have more pending jobs than this, we just nail the whole TLB. */
#define	PMAP_TLB_SHOOTDOWN_MAXJOBS	6

static struct pool_cache pmap_tlb_shootdown_job_cache;
#endif /* MULTIPROCESSOR */

/*
 * Internal routines
 */
static void	alpha_protection_init(void);
static bool	pmap_remove_mapping(pmap_t, vaddr_t, pt_entry_t *, bool, long);
static void	pmap_changebit(struct vm_page *, pt_entry_t, pt_entry_t, long);

/*
 * PT page management functions.
 */
static int	pmap_lev1map_create(pmap_t, long);
static void	pmap_lev1map_destroy(pmap_t, long);
static int	pmap_ptpage_alloc(pmap_t, pt_entry_t *, int);
static void	pmap_ptpage_free(pmap_t, pt_entry_t *);
static void	pmap_l3pt_delref(pmap_t, vaddr_t, pt_entry_t *, long);
static void	pmap_l2pt_delref(pmap_t, pt_entry_t *, pt_entry_t *, long);
static void	pmap_l1pt_delref(pmap_t, pt_entry_t *, long);

static void	*pmap_l1pt_alloc(struct pool *, int);
static void	pmap_l1pt_free(struct pool *, void *);

static struct pool_allocator pmap_l1pt_allocator = {
	pmap_l1pt_alloc, pmap_l1pt_free, 0,
};

static int	pmap_l1pt_ctor(void *, void *, int);

/*
 * PV table management functions.
 */
static int	pmap_pv_enter(pmap_t, struct vm_page *, vaddr_t, pt_entry_t *,
			      bool);
static void	pmap_pv_remove(pmap_t, struct vm_page *, vaddr_t, bool);
static void	*pmap_pv_page_alloc(struct pool *, int);
static void	pmap_pv_page_free(struct pool *, void *);

static struct pool_allocator pmap_pv_page_allocator = {
	pmap_pv_page_alloc, pmap_pv_page_free, 0,
};

#ifdef DEBUG
void	pmap_pv_dump(paddr_t);
#endif

#define	pmap_pv_alloc()		pool_cache_get(&pmap_pv_cache, PR_NOWAIT)
#define	pmap_pv_free(pv)	pool_cache_put(&pmap_pv_cache, (pv))

/*
 * ASN management functions.
 */
static void	pmap_asn_alloc(pmap_t, long);

/*
 * Misc. functions.
 */
static bool	pmap_physpage_alloc(int, paddr_t *);
static void	pmap_physpage_free(paddr_t);
static int	pmap_physpage_addref(void *);
static int	pmap_physpage_delref(void *);

/*
 * PMAP_ISACTIVE{,_TEST}:
 *
 *	Check to see if a pmap is active on the current processor.
 */
#define	PMAP_ISACTIVE_TEST(pm, cpu_id)					\
	(((pm)->pm_cpus & (1UL << (cpu_id))) != 0)

#if defined(DEBUG) && !defined(MULTIPROCESSOR)
#define	PMAP_ISACTIVE(pm, cpu_id)					\
({									\
	/*								\
	 * XXX This test is not MP-safe.				\
	 */								\
	int isactive_ = PMAP_ISACTIVE_TEST(pm, cpu_id);			\
									\
	if ((curlwp->l_flag & LW_IDLE) != 0 &&				\
	    curproc->p_vmspace != NULL &&				\
	   ((curproc->p_sflag & PS_WEXIT) == 0) &&			\
	   (isactive_ ^ ((pm) == curproc->p_vmspace->vm_map.pmap)))	\
		panic("PMAP_ISACTIVE");					\
	(isactive_);							\
})
#else
#define	PMAP_ISACTIVE(pm, cpu_id)	PMAP_ISACTIVE_TEST(pm, cpu_id)
#endif /* DEBUG && !MULTIPROCESSOR */

/*
 * PMAP_ACTIVATE_ASN_SANITY:
 *
 *	DEBUG sanity checks for ASNs within PMAP_ACTIVATE.
 */
#ifdef DEBUG
#define	PMAP_ACTIVATE_ASN_SANITY(pmap, cpu_id)				\
do {									\
	struct pmap_asn_info *__pma = &(pmap)->pm_asni[(cpu_id)];	\
	struct pmap_asn_info *__cpma = &pmap_asn_info[(cpu_id)];	\
									\
	if ((pmap)->pm_lev1map == kernel_lev1map) {			\
		/*							\
		 * This pmap implementation also ensures that pmaps	\
		 * referencing kernel_lev1map use a reserved ASN	\
		 * ASN to prevent the PALcode from servicing a TLB	\
		 * miss	with the wrong PTE.				\
		 */							\
		if (__pma->pma_asn != PMAP_ASN_RESERVED) {		\
			printf("kernel_lev1map with non-reserved ASN "	\
			    "(line %d)\n", __LINE__);			\
			panic("PMAP_ACTIVATE_ASN_SANITY");		\
		}							\
	} else {							\
		if (__pma->pma_asngen != __cpma->pma_asngen) {		\
			/*						\
			 * ASN generation number isn't valid!		\
			 */						\
			printf("pmap asngen %lu, current %lu "		\
			    "(line %d)\n",				\
			    __pma->pma_asngen,				\
			    __cpma->pma_asngen,				\
			    __LINE__);					\
			panic("PMAP_ACTIVATE_ASN_SANITY");		\
		}							\
		if (__pma->pma_asn == PMAP_ASN_RESERVED) {		\
			/*						\
			 * DANGER WILL ROBINSON!  We're going to	\
			 * pollute the VPT TLB entries!			\
			 */						\
			printf("Using reserved ASN! (line %d)\n",	\
			    __LINE__);					\
			panic("PMAP_ACTIVATE_ASN_SANITY");		\
		}							\
	}								\
} while (/*CONSTCOND*/0)
#else
#define	PMAP_ACTIVATE_ASN_SANITY(pmap, cpu_id)	/* nothing */
#endif

/*
 * PMAP_ACTIVATE:
 *
 *	This is essentially the guts of pmap_activate(), without
 *	ASN allocation.  This is used by pmap_activate(),
 *	pmap_lev1map_create(), and pmap_lev1map_destroy().
 *
 *	This is called only when it is known that a pmap is "active"
 *	on the current processor; the ASN must already be valid.
 */
#define	PMAP_ACTIVATE(pmap, l, cpu_id)					\
do {									\
	struct pcb *pcb = lwp_getpcb(l);				\
	PMAP_ACTIVATE_ASN_SANITY(pmap, cpu_id);				\
									\
	pcb->pcb_hw.apcb_ptbr =				\
	    ALPHA_K0SEG_TO_PHYS((vaddr_t)(pmap)->pm_lev1map) >> PGSHIFT; \
	pcb->pcb_hw.apcb_asn = (pmap)->pm_asni[(cpu_id)].pma_asn;	\
									\
	if ((l) == curlwp) {						\
		/*							\
		 * Page table base register has changed; switch to	\
		 * our own context again so that it will take effect.	\
		 */							\
		(void) alpha_pal_swpctx((u_long)l->l_md.md_pcbpaddr);	\
	}								\
} while (/*CONSTCOND*/0)

/*
 * PMAP_SET_NEEDISYNC:
 *
 *	Mark that a user pmap needs an I-stream synch on its
 *	way back out to userspace.
 */
#define	PMAP_SET_NEEDISYNC(pmap)	(pmap)->pm_needisync = ~0UL

/*
 * PMAP_SYNC_ISTREAM:
 *
 *	Synchronize the I-stream for the specified pmap.  For user
 *	pmaps, this is deferred until a process using the pmap returns
 *	to userspace.
 */
#if defined(MULTIPROCESSOR)
#define	PMAP_SYNC_ISTREAM_KERNEL()					\
do {									\
	alpha_pal_imb();						\
	alpha_broadcast_ipi(ALPHA_IPI_IMB);				\
} while (/*CONSTCOND*/0)

#define	PMAP_SYNC_ISTREAM_USER(pmap)					\
do {									\
	alpha_multicast_ipi((pmap)->pm_cpus, ALPHA_IPI_AST);		\
	/* for curcpu, will happen in userret() */			\
} while (/*CONSTCOND*/0)
#else
#define	PMAP_SYNC_ISTREAM_KERNEL()	alpha_pal_imb()
#define	PMAP_SYNC_ISTREAM_USER(pmap)	/* will happen in userret() */
#endif /* MULTIPROCESSOR */

#define	PMAP_SYNC_ISTREAM(pmap)						\
do {									\
	if ((pmap) == pmap_kernel())					\
		PMAP_SYNC_ISTREAM_KERNEL();				\
	else								\
		PMAP_SYNC_ISTREAM_USER(pmap);				\
} while (/*CONSTCOND*/0)

/*
 * PMAP_INVALIDATE_ASN:
 *
 *	Invalidate the specified pmap's ASN, so as to force allocation
 *	of a new one the next time pmap_asn_alloc() is called.
 *
 *	NOTE: THIS MUST ONLY BE CALLED IF AT LEAST ONE OF THE FOLLOWING
 *	CONDITIONS ARE true:
 *
 *		(1) The pmap references the global kernel_lev1map.
 *
 *		(2) The pmap is not active on the current processor.
 */
#define	PMAP_INVALIDATE_ASN(pmap, cpu_id)				\
do {									\
	(pmap)->pm_asni[(cpu_id)].pma_asn = PMAP_ASN_RESERVED;		\
} while (/*CONSTCOND*/0)

/*
 * PMAP_INVALIDATE_TLB:
 *
 *	Invalidate the TLB entry for the pmap/va pair.
 */
#define	PMAP_INVALIDATE_TLB(pmap, va, hadasm, isactive, cpu_id)		\
do {									\
	if ((hadasm) || (isactive)) {					\
		/*							\
		 * Simply invalidating the TLB entry and I-cache	\
		 * works in this case.					\
		 */							\
		ALPHA_TBIS((va));					\
	} else if ((pmap)->pm_asni[(cpu_id)].pma_asngen ==		\
		   pmap_asn_info[(cpu_id)].pma_asngen) {		\
		/*							\
		 * We can't directly invalidate the TLB entry		\
		 * in this case, so we have to force allocation		\
		 * of a new ASN the next time this pmap becomes		\
		 * active.						\
		 */							\
		PMAP_INVALIDATE_ASN((pmap), (cpu_id));			\
	}								\
		/*							\
		 * Nothing to do in this case; the next time the	\
		 * pmap becomes active on this processor, a new		\
		 * ASN will be allocated anyway.			\
		 */							\
} while (/*CONSTCOND*/0)

/*
 * PMAP_KERNEL_PTE:
 *
 *	Get a kernel PTE.
 *
 *	If debugging, do a table walk.  If not debugging, just use
 *	the Virtual Page Table, since all kernel page tables are
 *	pre-allocated and mapped in.
 */
#ifdef DEBUG
#define	PMAP_KERNEL_PTE(va)						\
({									\
	pt_entry_t *l1pte_, *l2pte_;					\
									\
	l1pte_ = pmap_l1pte(pmap_kernel(), va);				\
	if (pmap_pte_v(l1pte_) == 0) {					\
		printf("kernel level 1 PTE not valid, va 0x%lx "	\
		    "(line %d)\n", (va), __LINE__);			\
		panic("PMAP_KERNEL_PTE");				\
	}								\
	l2pte_ = pmap_l2pte(pmap_kernel(), va, l1pte_);			\
	if (pmap_pte_v(l2pte_) == 0) {					\
		printf("kernel level 2 PTE not valid, va 0x%lx "	\
		    "(line %d)\n", (va), __LINE__);			\
		panic("PMAP_KERNEL_PTE");				\
	}								\
	pmap_l3pte(pmap_kernel(), va, l2pte_);				\
})
#else
#define	PMAP_KERNEL_PTE(va)	(&VPT[VPT_INDEX((va))])
#endif

/*
 * PMAP_SET_PTE:
 *
 *	Set a PTE to a specified value.
 */
#define	PMAP_SET_PTE(ptep, val)	*(ptep) = (val)

/*
 * PMAP_STAT_{INCR,DECR}:
 *
 *	Increment or decrement a pmap statistic.
 */
#define	PMAP_STAT_INCR(s, v)	atomic_add_long((unsigned long *)(&(s)), (v))
#define	PMAP_STAT_DECR(s, v)	atomic_add_long((unsigned long *)(&(s)), -(v))

/*
 * pmap_bootstrap:
 *
 *	Bootstrap the system to run with virtual memory.
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_bootstrap(paddr_t ptaddr, u_int maxasn, u_long ncpuids)
{
	vsize_t lev2mapsize, lev3mapsize;
	pt_entry_t *lev2map, *lev3map;
	pt_entry_t pte;
	vsize_t bufsz;
	struct pcb *pcb;
	int i;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_BOOTSTRAP))
		printf("pmap_bootstrap(0x%lx, %u)\n", ptaddr, maxasn);
#endif

	/*
	 * Compute the number of pages kmem_arena will have.
	 */
	kmeminit_nkmempages();

	/*
	 * Figure out how many initial PTE's are necessary to map the
	 * kernel.  We also reserve space for kmem_alloc_pageable()
	 * for vm_fork().
	 */

	/* Get size of buffer cache and set an upper limit */
	bufsz = buf_memcalc();
	buf_setvalimit(bufsz);

	lev3mapsize =
		(VM_PHYS_SIZE + (ubc_nwins << ubc_winshift) +
		 bufsz + 16 * NCARGS + pager_map_size) / PAGE_SIZE +
		(maxproc * UPAGES) + nkmempages;

#ifdef SYSVSHM
	lev3mapsize += shminfo.shmall;
#endif
	lev3mapsize = roundup(lev3mapsize, NPTEPG);

	/*
	 * Initialize `FYI' variables.  Note we're relying on
	 * the fact that BSEARCH sorts the vm_physmem[] array
	 * for us.
	 */
	avail_start = ptoa(VM_PHYSMEM_PTR(0)->start);
	avail_end = ptoa(VM_PHYSMEM_PTR(vm_nphysseg - 1)->end);
	virtual_end = VM_MIN_KERNEL_ADDRESS + lev3mapsize * PAGE_SIZE;

#if 0
	printf("avail_start = 0x%lx\n", avail_start);
	printf("avail_end = 0x%lx\n", avail_end);
	printf("virtual_end = 0x%lx\n", virtual_end);
#endif

	/*
	 * Allocate a level 1 PTE table for the kernel.
	 * This is always one page long.
	 * IF THIS IS NOT A MULTIPLE OF PAGE_SIZE, ALL WILL GO TO HELL.
	 */
	kernel_lev1map = (pt_entry_t *)
	    uvm_pageboot_alloc(sizeof(pt_entry_t) * NPTEPG);

	/*
	 * Allocate a level 2 PTE table for the kernel.
	 * These must map all of the level3 PTEs.
	 * IF THIS IS NOT A MULTIPLE OF PAGE_SIZE, ALL WILL GO TO HELL.
	 */
	lev2mapsize = roundup(howmany(lev3mapsize, NPTEPG), NPTEPG);
	lev2map = (pt_entry_t *)
	    uvm_pageboot_alloc(sizeof(pt_entry_t) * lev2mapsize);

	/*
	 * Allocate a level 3 PTE table for the kernel.
	 * Contains lev3mapsize PTEs.
	 */
	lev3map = (pt_entry_t *)
	    uvm_pageboot_alloc(sizeof(pt_entry_t) * lev3mapsize);

	/*
	 * Set up level 1 page table
	 */

	/* Map all of the level 2 pte pages */
	for (i = 0; i < howmany(lev2mapsize, NPTEPG); i++) {
		pte = (ALPHA_K0SEG_TO_PHYS(((vaddr_t)lev2map) +
		    (i*PAGE_SIZE)) >> PGSHIFT) << PG_SHIFT;
		pte |= PG_V | PG_ASM | PG_KRE | PG_KWE | PG_WIRED;
		kernel_lev1map[l1pte_index(VM_MIN_KERNEL_ADDRESS +
		    (i*PAGE_SIZE*NPTEPG*NPTEPG))] = pte;
	}

	/* Map the virtual page table */
	pte = (ALPHA_K0SEG_TO_PHYS((vaddr_t)kernel_lev1map) >> PGSHIFT)
	    << PG_SHIFT;
	pte |= PG_V | PG_KRE | PG_KWE; /* NOTE NO ASM */
	kernel_lev1map[l1pte_index(VPTBASE)] = pte;
	VPT = (pt_entry_t *)VPTBASE;

#ifdef _PMAP_MAY_USE_PROM_CONSOLE
    {
	extern pt_entry_t prom_pte;			/* XXX */
	extern int prom_mapped;				/* XXX */

	if (pmap_uses_prom_console()) {
		/*
		 * XXX Save old PTE so we can remap the PROM, if
		 * XXX necessary.
		 */
		prom_pte = *(pt_entry_t *)ptaddr & ~PG_ASM;
	}
	prom_mapped = 0;

	/*
	 * Actually, this code lies.  The prom is still mapped, and will
	 * remain so until the context switch after alpha_init() returns.
	 */
    }
#endif

	/*
	 * Set up level 2 page table.
	 */
	/* Map all of the level 3 pte pages */
	for (i = 0; i < howmany(lev3mapsize, NPTEPG); i++) {
		pte = (ALPHA_K0SEG_TO_PHYS(((vaddr_t)lev3map) +
		    (i*PAGE_SIZE)) >> PGSHIFT) << PG_SHIFT;
		pte |= PG_V | PG_ASM | PG_KRE | PG_KWE | PG_WIRED;
		lev2map[l2pte_index(VM_MIN_KERNEL_ADDRESS+
		    (i*PAGE_SIZE*NPTEPG))] = pte;
	}

	/* Initialize the pmap_growkernel_lock. */
	rw_init(&pmap_growkernel_lock);

	/*
	 * Set up level three page table (lev3map)
	 */
	/* Nothing to do; it's already zero'd */

	/*
	 * Initialize the pmap pools and list.
	 */
	pmap_ncpuids = ncpuids;
	pool_cache_bootstrap(&pmap_pmap_cache, PMAP_SIZEOF(pmap_ncpuids), 0,
	    0, 0, "pmap", NULL, IPL_NONE, NULL, NULL, NULL);
	pool_cache_bootstrap(&pmap_l1pt_cache, PAGE_SIZE, 0, 0, 0, "pmapl1pt",
	    &pmap_l1pt_allocator, IPL_NONE, pmap_l1pt_ctor, NULL, NULL);
	pool_cache_bootstrap(&pmap_pv_cache, sizeof(struct pv_entry), 0, 0,
	    PR_LARGECACHE, "pmappv", &pmap_pv_page_allocator, IPL_NONE, NULL,
	    NULL, NULL);

	TAILQ_INIT(&pmap_all_pmaps);

	/*
	 * Initialize the ASN logic.
	 */
	pmap_max_asn = maxasn;
	for (i = 0; i < ALPHA_MAXPROCS; i++) {
		pmap_asn_info[i].pma_asn = 1;
		pmap_asn_info[i].pma_asngen = 0;
	}

	/*
	 * Initialize the locks.
	 */
	rw_init(&pmap_main_lock);
	mutex_init(&pmap_all_pmaps_lock, MUTEX_DEFAULT, IPL_NONE);
	for (i = 0; i < __arraycount(pmap_pvh_locks); i++) {
		mutex_init(&pmap_pvh_locks[i].lock, MUTEX_DEFAULT, IPL_NONE);
	}

	/*
	 * Initialize kernel pmap.  Note that all kernel mappings
	 * have PG_ASM set, so the ASN doesn't really matter for
	 * the kernel pmap.  Also, since the kernel pmap always
	 * references kernel_lev1map, it always has an invalid ASN
	 * generation.
	 */
	memset(pmap_kernel(), 0, sizeof(struct pmap));
	pmap_kernel()->pm_lev1map = kernel_lev1map;
	pmap_kernel()->pm_count = 1;
	for (i = 0; i < ALPHA_MAXPROCS; i++) {
		pmap_kernel()->pm_asni[i].pma_asn = PMAP_ASN_RESERVED;
		pmap_kernel()->pm_asni[i].pma_asngen =
		    pmap_asn_info[i].pma_asngen;
	}
	mutex_init(&pmap_kernel()->pm_lock, MUTEX_DEFAULT, IPL_NONE);
	TAILQ_INSERT_TAIL(&pmap_all_pmaps, pmap_kernel(), pm_list);

#if defined(MULTIPROCESSOR)
	/*
	 * Initialize the TLB shootdown queues.
	 */
	pool_cache_bootstrap(&pmap_tlb_shootdown_job_cache,
	    sizeof(struct pmap_tlb_shootdown_job), CACHE_LINE_SIZE,
	     0, PR_LARGECACHE, "pmaptlb", NULL, IPL_VM, NULL, NULL, NULL);
	for (i = 0; i < ALPHA_MAXPROCS; i++) {
		TAILQ_INIT(&pmap_tlb_shootdown_q[i].pq_head);
		mutex_init(&pmap_tlb_shootdown_q[i].pq_lock, MUTEX_DEFAULT,
		    IPL_SCHED);
	}
#endif

	/*
	 * Set up lwp0's PCB such that the ptbr points to the right place
	 * and has the kernel pmap's (really unused) ASN.
	 */
	pcb = lwp_getpcb(&lwp0);
	pcb->pcb_hw.apcb_ptbr =
	    ALPHA_K0SEG_TO_PHYS((vaddr_t)kernel_lev1map) >> PGSHIFT;
	pcb->pcb_hw.apcb_asn = pmap_kernel()->pm_asni[cpu_number()].pma_asn;

	/*
	 * Mark the kernel pmap `active' on this processor.
	 */
	atomic_or_ulong(&pmap_kernel()->pm_cpus,
	    (1UL << cpu_number()));
}

#ifdef _PMAP_MAY_USE_PROM_CONSOLE
int
pmap_uses_prom_console(void)
{

	return (cputype == ST_DEC_21000);
}
#endif /* _PMAP_MAY_USE_PROM_CONSOLE */

/*
 * pmap_virtual_space:		[ INTERFACE ]
 *
 *	Define the initial bounds of the kernel virtual address space.
 */
void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{

	*vstartp = VM_MIN_KERNEL_ADDRESS;	/* kernel is in K0SEG */
	*vendp = VM_MAX_KERNEL_ADDRESS;		/* we use pmap_growkernel */
}

/*
 * pmap_steal_memory:		[ INTERFACE ]
 *
 *	Bootstrap memory allocator (alternative to vm_bootstrap_steal_memory()).
 *	This function allows for early dynamic memory allocation until the
 *	virtual memory system has been bootstrapped.  After that point, either
 *	kmem_alloc or malloc should be used.  This function works by stealing
 *	pages from the (to be) managed page pool, then implicitly mapping the
 *	pages (by using their k0seg addresses) and zeroing them.
 *
 *	It may be used once the physical memory segments have been pre-loaded
 *	into the vm_physmem[] array.  Early memory allocation MUST use this
 *	interface!  This cannot be used after vm_page_startup(), and will
 *	generate a panic if tried.
 *
 *	Note that this memory will never be freed, and in essence it is wired
 *	down.
 *
 *	We must adjust *vstartp and/or *vendp iff we use address space
 *	from the kernel virtual address range defined by pmap_virtual_space().
 *
 *	Note: no locking is necessary in this function.
 */
vaddr_t
pmap_steal_memory(vsize_t size, vaddr_t *vstartp, vaddr_t *vendp)
{
	int bank, npgs, x;
	vaddr_t va;
	paddr_t pa;

	size = round_page(size);
	npgs = atop(size);

#if 0
	printf("PSM: size 0x%lx (npgs 0x%x)\n", size, npgs);
#endif

	for (bank = 0; bank < vm_nphysseg; bank++) {
		if (uvm.page_init_done == true)
			panic("pmap_steal_memory: called _after_ bootstrap");

#if 0
		printf("     bank %d: avail_start 0x%lx, start 0x%lx, "
		    "avail_end 0x%lx\n", bank, VM_PHYSMEM_PTR(bank)->avail_start,
		    VM_PHYSMEM_PTR(bank)->start, VM_PHYSMEM_PTR(bank)->avail_end);
#endif

		if (VM_PHYSMEM_PTR(bank)->avail_start != VM_PHYSMEM_PTR(bank)->start ||
		    VM_PHYSMEM_PTR(bank)->avail_start >= VM_PHYSMEM_PTR(bank)->avail_end)
			continue;

#if 0
		printf("             avail_end - avail_start = 0x%lx\n",
		    VM_PHYSMEM_PTR(bank)->avail_end - VM_PHYSMEM_PTR(bank)->avail_start);
#endif

		if ((VM_PHYSMEM_PTR(bank)->avail_end - VM_PHYSMEM_PTR(bank)->avail_start)
		    < npgs)
			continue;

		/*
		 * There are enough pages here; steal them!
		 */
		pa = ptoa(VM_PHYSMEM_PTR(bank)->avail_start);
		VM_PHYSMEM_PTR(bank)->avail_start += npgs;
		VM_PHYSMEM_PTR(bank)->start += npgs;

		/*
		 * Have we used up this segment?
		 */
		if (VM_PHYSMEM_PTR(bank)->avail_start == VM_PHYSMEM_PTR(bank)->end) {
			if (vm_nphysseg == 1)
				panic("pmap_steal_memory: out of memory!");

			/* Remove this segment from the list. */
			vm_nphysseg--;
			for (x = bank; x < vm_nphysseg; x++) {
				/* structure copy */
				VM_PHYSMEM_PTR_SWAP(x, x + 1);
			}
		}

		va = ALPHA_PHYS_TO_K0SEG(pa);
		memset((void *)va, 0, size);
		pmap_pages_stolen += npgs;
		return (va);
	}

	/*
	 * If we got here, this was no memory left.
	 */
	panic("pmap_steal_memory: no memory to steal");
}

/*
 * pmap_init:			[ INTERFACE ]
 *
 *	Initialize the pmap module.  Called by vm_init(), to initialize any
 *	structures that the pmap system needs to map virtual memory.
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_init(void)
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
	        printf("pmap_init()\n");
#endif

	/* initialize protection array */
	alpha_protection_init();

	/*
	 * Set a low water mark on the pv_entry pool, so that we are
	 * more likely to have these around even in extreme memory
	 * starvation.
	 */
	pool_cache_setlowat(&pmap_pv_cache, pmap_pv_lowat);

	/*
	 * Now it is safe to enable pv entry recording.
	 */
	pmap_initialized = true;

#if 0
	for (bank = 0; bank < vm_nphysseg; bank++) {
		printf("bank %d\n", bank);
		printf("\tstart = 0x%x\n", ptoa(VM_PHYSMEM_PTR(bank)->start));
		printf("\tend = 0x%x\n", ptoa(VM_PHYSMEM_PTR(bank)->end));
		printf("\tavail_start = 0x%x\n",
		    ptoa(VM_PHYSMEM_PTR(bank)->avail_start));
		printf("\tavail_end = 0x%x\n",
		    ptoa(VM_PHYSMEM_PTR(bank)->avail_end));
	}
#endif
}

/*
 * pmap_create:			[ INTERFACE ]
 *
 *	Create and return a physical map.
 *
 *	Note: no locking is necessary in this function.
 */
pmap_t
pmap_create(void)
{
	pmap_t pmap;
	int i;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_create()\n");
#endif

	pmap = pool_cache_get(&pmap_pmap_cache, PR_WAITOK);
	memset(pmap, 0, sizeof(*pmap));

	/*
	 * Defer allocation of a new level 1 page table until
	 * the first new mapping is entered; just take a reference
	 * to the kernel kernel_lev1map.
	 */
	pmap->pm_lev1map = kernel_lev1map;

	pmap->pm_count = 1;
	for (i = 0; i < pmap_ncpuids; i++) {
		pmap->pm_asni[i].pma_asn = PMAP_ASN_RESERVED;
		/* XXX Locking? */
		pmap->pm_asni[i].pma_asngen = pmap_asn_info[i].pma_asngen;
	}
	mutex_init(&pmap->pm_lock, MUTEX_DEFAULT, IPL_NONE);

 try_again:
	rw_enter(&pmap_growkernel_lock, RW_READER);

	if (pmap_lev1map_create(pmap, cpu_number()) != 0) {
		rw_exit(&pmap_growkernel_lock);
		(void) kpause("pmap_create", false, hz >> 2, NULL);
		goto try_again;
	}

	mutex_enter(&pmap_all_pmaps_lock);
	TAILQ_INSERT_TAIL(&pmap_all_pmaps, pmap, pm_list);
	mutex_exit(&pmap_all_pmaps_lock);

	rw_exit(&pmap_growkernel_lock);

	return (pmap);
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

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_destroy(%p)\n", pmap);
#endif

	if (atomic_dec_uint_nv(&pmap->pm_count) > 0)
		return;

	rw_enter(&pmap_growkernel_lock, RW_READER);

	/*
	 * Remove it from the global list of all pmaps.
	 */
	mutex_enter(&pmap_all_pmaps_lock);
	TAILQ_REMOVE(&pmap_all_pmaps, pmap, pm_list);
	mutex_exit(&pmap_all_pmaps_lock);

	pmap_lev1map_destroy(pmap, cpu_number());

	rw_exit(&pmap_growkernel_lock);

	/*
	 * Since the pmap is supposed to contain no valid
	 * mappings at this point, we should always see
	 * kernel_lev1map here.
	 */
	KASSERT(pmap->pm_lev1map == kernel_lev1map);

	mutex_destroy(&pmap->pm_lock);
	pool_cache_put(&pmap_pmap_cache, pmap);
}

/*
 * pmap_reference:		[ INTERFACE ]
 *
 *	Add a reference to the specified pmap.
 */
void
pmap_reference(pmap_t pmap)
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_reference(%p)\n", pmap);
#endif

	atomic_inc_uint(&pmap->pm_count);
}

/*
 * pmap_remove:			[ INTERFACE ]
 *
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the page size.
 */
void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	pt_entry_t *l1pte, *l2pte, *l3pte;
	pt_entry_t *saved_l1pte, *saved_l2pte, *saved_l3pte;
	vaddr_t l1eva, l2eva, vptva;
	bool needisync = false;
	long cpu_id = cpu_number();

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove(%p, %lx, %lx)\n", pmap, sva, eva);
#endif

	/*
	 * If this is the kernel pmap, we can use a faster method
	 * for accessing the PTEs (since the PT pages are always
	 * resident).
	 *
	 * Note that this routine should NEVER be called from an
	 * interrupt context; pmap_kremove() is used for that.
	 */
	if (pmap == pmap_kernel()) {
		PMAP_MAP_TO_HEAD_LOCK();
		PMAP_LOCK(pmap);

		while (sva < eva) {
			l3pte = PMAP_KERNEL_PTE(sva);
			if (pmap_pte_v(l3pte)) {
#ifdef DIAGNOSTIC
				if (uvm_pageismanaged(pmap_pte_pa(l3pte)) &&
				    pmap_pte_pv(l3pte) == 0)
					panic("pmap_remove: managed page "
					    "without PG_PVLIST for 0x%lx",
					    sva);
#endif
				needisync |= pmap_remove_mapping(pmap, sva,
				    l3pte, true, cpu_id);
			}
			sva += PAGE_SIZE;
		}

		PMAP_UNLOCK(pmap);
		PMAP_MAP_TO_HEAD_UNLOCK();

		if (needisync)
			PMAP_SYNC_ISTREAM_KERNEL();
		return;
	}

#ifdef DIAGNOSTIC
	if (sva > VM_MAXUSER_ADDRESS || eva > VM_MAXUSER_ADDRESS)
		panic("pmap_remove: (0x%lx - 0x%lx) user pmap, kernel "
		    "address range", sva, eva);
#endif

	PMAP_MAP_TO_HEAD_LOCK();
	PMAP_LOCK(pmap);

	/*
	 * If we're already referencing the kernel_lev1map, there
	 * is no work for us to do.
	 */
	if (pmap->pm_lev1map == kernel_lev1map)
		goto out;

	saved_l1pte = l1pte = pmap_l1pte(pmap, sva);

	/*
	 * Add a reference to the L1 table to it won't get
	 * removed from under us.
	 */
	pmap_physpage_addref(saved_l1pte);

	for (; sva < eva; sva = l1eva, l1pte++) {
		l1eva = alpha_trunc_l1seg(sva) + ALPHA_L1SEG_SIZE;
		if (pmap_pte_v(l1pte)) {
			saved_l2pte = l2pte = pmap_l2pte(pmap, sva, l1pte);

			/*
			 * Add a reference to the L2 table so it won't
			 * get removed from under us.
			 */
			pmap_physpage_addref(saved_l2pte);

			for (; sva < l1eva && sva < eva; sva = l2eva, l2pte++) {
				l2eva =
				    alpha_trunc_l2seg(sva) + ALPHA_L2SEG_SIZE;
				if (pmap_pte_v(l2pte)) {
					saved_l3pte = l3pte =
					    pmap_l3pte(pmap, sva, l2pte);

					/*
					 * Add a reference to the L3 table so
					 * it won't get removed from under us.
					 */
					pmap_physpage_addref(saved_l3pte);

					/*
					 * Remember this sva; if the L3 table
					 * gets removed, we need to invalidate
					 * the VPT TLB entry for it.
					 */
					vptva = sva;

					for (; sva < l2eva && sva < eva;
					     sva += PAGE_SIZE, l3pte++) {
						if (!pmap_pte_v(l3pte)) {
							continue;
						}
						needisync |=
						    pmap_remove_mapping(
							pmap, sva,
							l3pte, true,
							cpu_id);
					}

					/*
					 * Remove the reference to the L3
					 * table that we added above.  This
					 * may free the L3 table.
					 */
					pmap_l3pt_delref(pmap, vptva,
					    saved_l3pte, cpu_id);
				}
			}

			/*
			 * Remove the reference to the L2 table that we
			 * added above.  This may free the L2 table.
			 */
			pmap_l2pt_delref(pmap, l1pte, saved_l2pte, cpu_id);
		}
	}

	/*
	 * Remove the reference to the L1 table that we added above.
	 * This may free the L1 table.
	 */
	pmap_l1pt_delref(pmap, saved_l1pte, cpu_id);

	if (needisync)
		PMAP_SYNC_ISTREAM_USER(pmap);

 out:
	PMAP_UNLOCK(pmap);
	PMAP_MAP_TO_HEAD_UNLOCK();
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
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pmap_t pmap;
	pv_entry_t pv, nextpv;
	bool needkisync = false;
	long cpu_id = cpu_number();
	kmutex_t *lock;
	PMAP_TLB_SHOOTDOWN_CPUSET_DECL
#ifdef DEBUG
	paddr_t pa = VM_PAGE_TO_PHYS(pg);


	if ((pmapdebug & (PDB_FOLLOW|PDB_PROTECT)) ||
	    (prot == VM_PROT_NONE && (pmapdebug & PDB_REMOVE)))
		printf("pmap_page_protect(%p, %x)\n", pg, prot);
#endif

	switch (prot) {
	case VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE:
	case VM_PROT_READ|VM_PROT_WRITE:
		return;

	/* copy_on_write */
	case VM_PROT_READ|VM_PROT_EXECUTE:
	case VM_PROT_READ:
		PMAP_HEAD_TO_MAP_LOCK();
		lock = pmap_pvh_lock(pg);
		mutex_enter(lock);
		for (pv = md->pvh_list; pv != NULL; pv = pv->pv_next) {
			PMAP_LOCK(pv->pv_pmap);
			if (*pv->pv_pte & (PG_KWE | PG_UWE)) {
				*pv->pv_pte &= ~(PG_KWE | PG_UWE);
				PMAP_INVALIDATE_TLB(pv->pv_pmap, pv->pv_va,
				    pmap_pte_asm(pv->pv_pte),
				    PMAP_ISACTIVE(pv->pv_pmap, cpu_id), cpu_id);
				PMAP_TLB_SHOOTDOWN(pv->pv_pmap, pv->pv_va,
				    pmap_pte_asm(pv->pv_pte));
			}
			PMAP_UNLOCK(pv->pv_pmap);
		}
		mutex_exit(lock);
		PMAP_HEAD_TO_MAP_UNLOCK();
		PMAP_TLB_SHOOTNOW();
		return;

	/* remove_all */
	default:
		break;
	}

	PMAP_HEAD_TO_MAP_LOCK();
	lock = pmap_pvh_lock(pg);
	mutex_enter(lock);
	for (pv = md->pvh_list; pv != NULL; pv = nextpv) {
		nextpv = pv->pv_next;
		pmap = pv->pv_pmap;

		PMAP_LOCK(pmap);
#ifdef DEBUG
		if (pmap_pte_v(pmap_l2pte(pv->pv_pmap, pv->pv_va, NULL)) == 0 ||
		    pmap_pte_pa(pv->pv_pte) != pa)
			panic("pmap_page_protect: bad mapping");
#endif
		if (pmap_remove_mapping(pmap, pv->pv_va, pv->pv_pte,
		    false, cpu_id) == true) {
			if (pmap == pmap_kernel())
				needkisync |= true;
			else
				PMAP_SYNC_ISTREAM_USER(pmap);
		}
		PMAP_UNLOCK(pmap);
	}

	if (needkisync)
		PMAP_SYNC_ISTREAM_KERNEL();

	mutex_exit(lock);
	PMAP_HEAD_TO_MAP_UNLOCK();
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
	pt_entry_t *l1pte, *l2pte, *l3pte, bits;
	bool isactive;
	bool hadasm;
	vaddr_t l1eva, l2eva;
	long cpu_id = cpu_number();
	PMAP_TLB_SHOOTDOWN_CPUSET_DECL

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT))
		printf("pmap_protect(%p, %lx, %lx, %x)\n",
		    pmap, sva, eva, prot);
#endif

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}

	PMAP_LOCK(pmap);

	bits = pte_prot(pmap, prot);
	isactive = PMAP_ISACTIVE(pmap, cpu_id);

	l1pte = pmap_l1pte(pmap, sva);
	for (; sva < eva; sva = l1eva, l1pte++) {
		l1eva = alpha_trunc_l1seg(sva) + ALPHA_L1SEG_SIZE;
		if (pmap_pte_v(l1pte)) {
			l2pte = pmap_l2pte(pmap, sva, l1pte);
			for (; sva < l1eva && sva < eva; sva = l2eva, l2pte++) {
				l2eva =
				    alpha_trunc_l2seg(sva) + ALPHA_L2SEG_SIZE;
				if (pmap_pte_v(l2pte)) {
					l3pte = pmap_l3pte(pmap, sva, l2pte);
					for (; sva < l2eva && sva < eva;
					     sva += PAGE_SIZE, l3pte++) {
						if (pmap_pte_v(l3pte) &&
						    pmap_pte_prot_chg(l3pte,
						    bits)) {
							hadasm =
							   (pmap_pte_asm(l3pte)
							    != 0);
							pmap_pte_set_prot(l3pte,
							   bits);
							PMAP_INVALIDATE_TLB(
							   pmap, sva, hadasm,
							   isactive, cpu_id);
							PMAP_TLB_SHOOTDOWN(
							   pmap, sva,
							   hadasm ? PG_ASM : 0);
						}
					}
				}
			}
		}
	}

	PMAP_TLB_SHOOTNOW();

	if (prot & VM_PROT_EXECUTE)
		PMAP_SYNC_ISTREAM(pmap);

	PMAP_UNLOCK(pmap);
}

/*
 * pmap_enter:			[ INTERFACE ]
 *
 *	Insert the given physical page (p) at
 *	the specified virtual address (v) in the
 *	target physical map with the protection requested.
 *
 *	If specified, the page will be wired down, meaning
 *	that the related pte can not be reclaimed.
 *
 *	Note:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */
int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	struct vm_page *pg;			/* if != NULL, managed page */
	pt_entry_t *pte, npte, opte;
	paddr_t opa;
	bool tflush = true;
	bool hadasm = false;	/* XXX gcc -Wuninitialized */
	bool needisync = false;
	bool setisync = false;
	bool isactive;
	bool wired;
	long cpu_id = cpu_number();
	int error = 0;
	kmutex_t *lock;
	PMAP_TLB_SHOOTDOWN_CPUSET_DECL

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_enter(%p, %lx, %lx, %x, %x)\n",
		       pmap, va, pa, prot, flags);
#endif
	pg = PHYS_TO_VM_PAGE(pa);
	isactive = PMAP_ISACTIVE(pmap, cpu_id);
	wired = (flags & PMAP_WIRED) != 0;

	/*
	 * Determine what we need to do about the I-stream.  If
	 * VM_PROT_EXECUTE is set, we mark a user pmap as needing
	 * an I-sync on the way back out to userspace.  We always
	 * need an immediate I-sync for the kernel pmap.
	 */
	if (prot & VM_PROT_EXECUTE) {
		if (pmap == pmap_kernel())
			needisync = true;
		else {
			setisync = true;
			needisync = (pmap->pm_cpus != 0);
		}
	}

	PMAP_MAP_TO_HEAD_LOCK();
	PMAP_LOCK(pmap);

	if (pmap == pmap_kernel()) {
#ifdef DIAGNOSTIC
		/*
		 * Sanity check the virtual address.
		 */
		if (va < VM_MIN_KERNEL_ADDRESS)
			panic("pmap_enter: kernel pmap, invalid va 0x%lx", va);
#endif
		pte = PMAP_KERNEL_PTE(va);
	} else {
		pt_entry_t *l1pte, *l2pte;

#ifdef DIAGNOSTIC
		/*
		 * Sanity check the virtual address.
		 */
		if (va >= VM_MAXUSER_ADDRESS)
			panic("pmap_enter: user pmap, invalid va 0x%lx", va);
#endif

		KASSERT(pmap->pm_lev1map != kernel_lev1map);

		/*
		 * Check to see if the level 1 PTE is valid, and
		 * allocate a new level 2 page table page if it's not.
		 * A reference will be added to the level 2 table when
		 * the level 3 table is created.
		 */
		l1pte = pmap_l1pte(pmap, va);
		if (pmap_pte_v(l1pte) == 0) {
			pmap_physpage_addref(l1pte);
			error = pmap_ptpage_alloc(pmap, l1pte, PGU_L2PT);
			if (error) {
				pmap_l1pt_delref(pmap, l1pte, cpu_id);
				if (flags & PMAP_CANFAIL)
					goto out;
				panic("pmap_enter: unable to create L2 PT "
				    "page");
			}
#ifdef DEBUG
			if (pmapdebug & PDB_PTPAGE)
				printf("pmap_enter: new level 2 table at "
				    "0x%lx\n", pmap_pte_pa(l1pte));
#endif
		}

		/*
		 * Check to see if the level 2 PTE is valid, and
		 * allocate a new level 3 page table page if it's not.
		 * A reference will be added to the level 3 table when
		 * the mapping is validated.
		 */
		l2pte = pmap_l2pte(pmap, va, l1pte);
		if (pmap_pte_v(l2pte) == 0) {
			pmap_physpage_addref(l2pte);
			error = pmap_ptpage_alloc(pmap, l2pte, PGU_L3PT);
			if (error) {
				pmap_l2pt_delref(pmap, l1pte, l2pte, cpu_id);
				if (flags & PMAP_CANFAIL)
					goto out;
				panic("pmap_enter: unable to create L3 PT "
				    "page");
			}
#ifdef DEBUG
			if (pmapdebug & PDB_PTPAGE)
				printf("pmap_enter: new level 3 table at "
				    "0x%lx\n", pmap_pte_pa(l2pte));
#endif
		}

		/*
		 * Get the PTE that will map the page.
		 */
		pte = pmap_l3pte(pmap, va, l2pte);
	}

	/* Remember all of the old PTE; used for TBI check later. */
	opte = *pte;

	/*
	 * Check to see if the old mapping is valid.  If not, validate the
	 * new one immediately.
	 */
	if (pmap_pte_v(pte) == 0) {
		/*
		 * No need to invalidate the TLB in this case; an invalid
		 * mapping won't be in the TLB, and a previously valid
		 * mapping would have been flushed when it was invalidated.
		 */
		tflush = false;

		/*
		 * No need to synchronize the I-stream, either, for basically
		 * the same reason.
		 */
		setisync = needisync = false;

		if (pmap != pmap_kernel()) {
			/*
			 * New mappings gain a reference on the level 3
			 * table.
			 */
			pmap_physpage_addref(pte);
		}
		goto validate_enterpv;
	}

	opa = pmap_pte_pa(pte);
	hadasm = (pmap_pte_asm(pte) != 0);

	if (opa == pa) {
		/*
		 * Mapping has not changed; must be a protection or
		 * wiring change.
		 */
		if (pmap_pte_w_chg(pte, wired ? PG_WIRED : 0)) {
#ifdef DEBUG
			if (pmapdebug & PDB_ENTER)
				printf("pmap_enter: wiring change -> %d\n",
				    wired);
#endif
			/*
			 * Adjust the wiring count.
			 */
			if (wired)
				PMAP_STAT_INCR(pmap->pm_stats.wired_count, 1);
			else
				PMAP_STAT_DECR(pmap->pm_stats.wired_count, 1);
		}

		/*
		 * Set the PTE.
		 */
		goto validate;
	}

	/*
	 * The mapping has changed.  We need to invalidate the
	 * old mapping before creating the new one.
	 */
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("pmap_enter: removing old mapping 0x%lx\n", va);
#endif
	if (pmap != pmap_kernel()) {
		/*
		 * Gain an extra reference on the level 3 table.
		 * pmap_remove_mapping() will delete a reference,
		 * and we don't want the table to be erroneously
		 * freed.
		 */
		pmap_physpage_addref(pte);
	}
	needisync |= pmap_remove_mapping(pmap, va, pte, true, cpu_id);

 validate_enterpv:
	/*
	 * Enter the mapping into the pv_table if appropriate.
	 */
	if (pg != NULL) {
		error = pmap_pv_enter(pmap, pg, va, pte, true);
		if (error) {
			pmap_l3pt_delref(pmap, va, pte, cpu_id);
			if (flags & PMAP_CANFAIL)
				goto out;
			panic("pmap_enter: unable to enter mapping in PV "
			    "table");
		}
	}

	/*
	 * Increment counters.
	 */
	PMAP_STAT_INCR(pmap->pm_stats.resident_count, 1);
	if (wired)
		PMAP_STAT_INCR(pmap->pm_stats.wired_count, 1);

 validate:
	/*
	 * Build the new PTE.
	 */
	npte = ((pa >> PGSHIFT) << PG_SHIFT) | pte_prot(pmap, prot) | PG_V;
	if (pg != NULL) {
		struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
		int attrs;

#ifdef DIAGNOSTIC
		if ((flags & VM_PROT_ALL) & ~prot)
			panic("pmap_enter: access type exceeds prot");
#endif
		lock = pmap_pvh_lock(pg);
		mutex_enter(lock);
		if (flags & VM_PROT_WRITE)
			md->pvh_attrs |= (PGA_REFERENCED|PGA_MODIFIED);
		else if (flags & VM_PROT_ALL)
			md->pvh_attrs |= PGA_REFERENCED;
		attrs = md->pvh_attrs;
		mutex_exit(lock);

		/*
		 * Set up referenced/modified emulation for new mapping.
		 */
		if ((attrs & PGA_REFERENCED) == 0)
			npte |= PG_FOR | PG_FOW | PG_FOE;
		else if ((attrs & PGA_MODIFIED) == 0)
			npte |= PG_FOW;

		/*
		 * Mapping was entered on PV list.
		 */
		npte |= PG_PVLIST;
	}
	if (wired)
		npte |= PG_WIRED;
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("pmap_enter: new pte = 0x%lx\n", npte);
#endif

	/*
	 * If the PALcode portion of the new PTE is the same as the
	 * old PTE, no TBI is necessary.
	 */
	if (PG_PALCODE(opte) == PG_PALCODE(npte))
		tflush = false;

	/*
	 * Set the new PTE.
	 */
	PMAP_SET_PTE(pte, npte);

	/*
	 * Invalidate the TLB entry for this VA and any appropriate
	 * caches.
	 */
	if (tflush) {
		PMAP_INVALIDATE_TLB(pmap, va, hadasm, isactive, cpu_id);
		PMAP_TLB_SHOOTDOWN(pmap, va, hadasm ? PG_ASM : 0);
		PMAP_TLB_SHOOTNOW();
	}
	if (setisync)
		PMAP_SET_NEEDISYNC(pmap);
	if (needisync)
		PMAP_SYNC_ISTREAM(pmap);

out:
	PMAP_UNLOCK(pmap);
	PMAP_MAP_TO_HEAD_UNLOCK();
	
	return error;
}

/*
 * pmap_kenter_pa:		[ INTERFACE ]
 *
 *	Enter a va -> pa mapping into the kernel pmap without any
 *	physical->virtual tracking.
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	pt_entry_t *pte, npte;
	long cpu_id = cpu_number();
	bool needisync = false;
	pmap_t pmap = pmap_kernel();
	PMAP_TLB_SHOOTDOWN_CPUSET_DECL

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_kenter_pa(%lx, %lx, %x)\n",
		    va, pa, prot);
#endif

#ifdef DIAGNOSTIC
	/*
	 * Sanity check the virtual address.
	 */
	if (va < VM_MIN_KERNEL_ADDRESS)
		panic("pmap_kenter_pa: kernel pmap, invalid va 0x%lx", va);
#endif

	pte = PMAP_KERNEL_PTE(va);

	if (pmap_pte_v(pte) == 0)
		PMAP_STAT_INCR(pmap->pm_stats.resident_count, 1);
	if (pmap_pte_w(pte) == 0)
		PMAP_STAT_DECR(pmap->pm_stats.wired_count, 1);

	if ((prot & VM_PROT_EXECUTE) != 0 || pmap_pte_exec(pte))
		needisync = true;

	/*
	 * Build the new PTE.
	 */
	npte = ((pa >> PGSHIFT) << PG_SHIFT) | pte_prot(pmap_kernel(), prot) |
	    PG_V | PG_WIRED;

	/*
	 * Set the new PTE.
	 */
	PMAP_SET_PTE(pte, npte);
#if defined(MULTIPROCESSOR)
	alpha_mb();		/* XXX alpha_wmb()? */
#endif

	/*
	 * Invalidate the TLB entry for this VA and any appropriate
	 * caches.
	 */
	PMAP_INVALIDATE_TLB(pmap, va, true, true, cpu_id);
	PMAP_TLB_SHOOTDOWN(pmap, va, PG_ASM);
	PMAP_TLB_SHOOTNOW();

	if (needisync)
		PMAP_SYNC_ISTREAM_KERNEL();
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
	pt_entry_t *pte;
	bool needisync = false;
	long cpu_id = cpu_number();
	pmap_t pmap = pmap_kernel();
	PMAP_TLB_SHOOTDOWN_CPUSET_DECL

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_kremove(%lx, %lx)\n",
		    va, size);
#endif

#ifdef DIAGNOSTIC
	if (va < VM_MIN_KERNEL_ADDRESS)
		panic("pmap_kremove: user address");
#endif

	for (; size != 0; size -= PAGE_SIZE, va += PAGE_SIZE) {
		pte = PMAP_KERNEL_PTE(va);
		if (pmap_pte_v(pte)) {
#ifdef DIAGNOSTIC
			if (pmap_pte_pv(pte))
				panic("pmap_kremove: PG_PVLIST mapping for "
				    "0x%lx", va);
#endif
			if (pmap_pte_exec(pte))
				needisync = true;

			/* Zap the mapping. */
			PMAP_SET_PTE(pte, PG_NV);
#if defined(MULTIPROCESSOR)
			alpha_mb();		/* XXX alpha_wmb()? */
#endif
			PMAP_INVALIDATE_TLB(pmap, va, true, true, cpu_id);
			PMAP_TLB_SHOOTDOWN(pmap, va, PG_ASM);

			/* Update stats. */
			PMAP_STAT_DECR(pmap->pm_stats.resident_count, 1);
			PMAP_STAT_DECR(pmap->pm_stats.wired_count, 1);
		}
	}

	PMAP_TLB_SHOOTNOW();

	if (needisync)
		PMAP_SYNC_ISTREAM_KERNEL();
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
	pt_entry_t *pte;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_unwire(%p, %lx)\n", pmap, va);
#endif

	PMAP_LOCK(pmap);

	pte = pmap_l3pte(pmap, va, NULL);
#ifdef DIAGNOSTIC
	if (pte == NULL || pmap_pte_v(pte) == 0)
		panic("pmap_unwire");
#endif

	/*
	 * If wiring actually changed (always?) clear the wire bit and
	 * update the wire count.  Note that wiring is not a hardware
	 * characteristic so there is no need to invalidate the TLB.
	 */
	if (pmap_pte_w_chg(pte, 0)) {
		pmap_pte_set_w(pte, false);
		PMAP_STAT_DECR(pmap->pm_stats.wired_count, 1);
	}
#ifdef DIAGNOSTIC
	else {
		printf("pmap_unwire: wiring for pmap %p va 0x%lx "
		    "didn't change!\n", pmap, va);
	}
#endif

	PMAP_UNLOCK(pmap);
}

/*
 * pmap_extract:		[ INTERFACE ]
 *
 *	Extract the physical address associated with the given
 *	pmap/virtual address pair.
 */
bool
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *pap)
{
	pt_entry_t *l1pte, *l2pte, *l3pte;
	paddr_t pa;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_extract(%p, %lx) -> ", pmap, va);
#endif

	/*
	 * Take a faster path for the kernel pmap.  Avoids locking,
	 * handles K0SEG.
	 */
	if (pmap == pmap_kernel()) {
		pa = vtophys(va);
		if (pap != NULL)
			*pap = pa;
#ifdef DEBUG
		if (pmapdebug & PDB_FOLLOW)
			printf("0x%lx (kernel vtophys)\n", pa);
#endif
		return (pa != 0);	/* XXX */
	}

	PMAP_LOCK(pmap);

	l1pte = pmap_l1pte(pmap, va);
	if (pmap_pte_v(l1pte) == 0)
		goto out;

	l2pte = pmap_l2pte(pmap, va, l1pte);
	if (pmap_pte_v(l2pte) == 0)
		goto out;

	l3pte = pmap_l3pte(pmap, va, l2pte);
	if (pmap_pte_v(l3pte) == 0)
		goto out;

	pa = pmap_pte_pa(l3pte) | (va & PGOFSET);
	PMAP_UNLOCK(pmap);
	if (pap != NULL)
		*pap = pa;
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("0x%lx\n", pa);
#endif
	return (true);

 out:
	PMAP_UNLOCK(pmap);
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("failed\n");
#endif
	return (false);
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
 *	reloading the MMU context if the current process, and marking
 *	the pmap in use by the processor.
 */
void
pmap_activate(struct lwp *l)
{
	struct pmap *pmap = l->l_proc->p_vmspace->vm_map.pmap;
	long cpu_id = cpu_number();

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_activate(%p)\n", l);
#endif

	/* Mark the pmap in use by this processor. */
	atomic_or_ulong(&pmap->pm_cpus, (1UL << cpu_id));

	/* Allocate an ASN. */
	pmap_asn_alloc(pmap, cpu_id);

	PMAP_ACTIVATE(pmap, l, cpu_id);
}

/*
 * pmap_deactivate:		[ INTERFACE ]
 *
 *	Mark that the pmap used by the specified process is no longer
 *	in use by the processor.
 *
 *	The comment above pmap_activate() wrt. locking applies here,
 *	as well.  Note that we use only a single `atomic' operation,
 *	so no locking is necessary.
 */
void
pmap_deactivate(struct lwp *l)
{
	struct pmap *pmap = l->l_proc->p_vmspace->vm_map.pmap;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_deactivate(%p)\n", l);
#endif

	/*
	 * Mark the pmap no longer in use by this processor.
	 */
	atomic_and_ulong(&pmap->pm_cpus, ~(1UL << cpu_number()));
}

/*
 * pmap_zero_page:		[ INTERFACE ]
 *
 *	Zero the specified (machine independent) page by mapping the page
 *	into virtual memory and clear its contents, one machine dependent
 *	page at a time.
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_zero_page(paddr_t phys)
{
	u_long *p0, *p1, *pend;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_zero_page(%lx)\n", phys);
#endif

	p0 = (u_long *)ALPHA_PHYS_TO_K0SEG(phys);
	p1 = NULL;
	pend = (u_long *)((u_long)p0 + PAGE_SIZE);

	/*
	 * Unroll the loop a bit, doing 16 quadwords per iteration.
	 * Do only 8 back-to-back stores, and alternate registers.
	 */
	do {
		__asm volatile(
		"# BEGIN loop body\n"
		"	addq	%2, (8 * 8), %1		\n"
		"	stq	$31, (0 * 8)(%0)	\n"
		"	stq	$31, (1 * 8)(%0)	\n"
		"	stq	$31, (2 * 8)(%0)	\n"
		"	stq	$31, (3 * 8)(%0)	\n"
		"	stq	$31, (4 * 8)(%0)	\n"
		"	stq	$31, (5 * 8)(%0)	\n"
		"	stq	$31, (6 * 8)(%0)	\n"
		"	stq	$31, (7 * 8)(%0)	\n"
		"					\n"
		"	addq	%3, (8 * 8), %0		\n"
		"	stq	$31, (0 * 8)(%1)	\n"
		"	stq	$31, (1 * 8)(%1)	\n"
		"	stq	$31, (2 * 8)(%1)	\n"
		"	stq	$31, (3 * 8)(%1)	\n"
		"	stq	$31, (4 * 8)(%1)	\n"
		"	stq	$31, (5 * 8)(%1)	\n"
		"	stq	$31, (6 * 8)(%1)	\n"
		"	stq	$31, (7 * 8)(%1)	\n"
		"	# END loop body"
		: "=r" (p0), "=r" (p1)
		: "0" (p0), "1" (p1)
		: "memory");
	} while (p0 < pend);
}

/*
 * pmap_copy_page:		[ INTERFACE ]
 *
 *	Copy the specified (machine independent) page by mapping the page
 *	into virtual memory and using memcpy to copy the page, one machine
 *	dependent page at a time.
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_copy_page(paddr_t src, paddr_t dst)
{
	const void *s;
	void *d;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy_page(%lx, %lx)\n", src, dst);
#endif
	s = (const void *)ALPHA_PHYS_TO_K0SEG(src);
	d = (void *)ALPHA_PHYS_TO_K0SEG(dst);
	memcpy(d, s, PAGE_SIZE);
}

/*
 * pmap_pageidlezero:		[ INTERFACE ]
 *
 *	Page zero'er for the idle loop.  Returns true if the
 *	page was zero'd, FLASE if we aborted for some reason.
 */
bool
pmap_pageidlezero(paddr_t pa)
{
	u_long *ptr;
	int i, cnt = PAGE_SIZE / sizeof(u_long);

	for (i = 0, ptr = (u_long *) ALPHA_PHYS_TO_K0SEG(pa); i < cnt; i++) {
		if (sched_curcpu_runnable_p()) {
			/*
			 * An LWP has become ready.  Abort now,
			 * so we don't keep it waiting while we
			 * finish zeroing the page.
			 */
			return (false);
		}
		*ptr++ = 0;
	}

	return (true);
}

/*
 * pmap_clear_modify:		[ INTERFACE ]
 *
 *	Clear the modify bits on the specified physical page.
 */
bool
pmap_clear_modify(struct vm_page *pg)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	bool rv = false;
	long cpu_id = cpu_number();
	kmutex_t *lock;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_modify(%p)\n", pg);
#endif

	PMAP_HEAD_TO_MAP_LOCK();
	lock = pmap_pvh_lock(pg);
	mutex_enter(lock);

	if (md->pvh_attrs & PGA_MODIFIED) {
		rv = true;
		pmap_changebit(pg, PG_FOW, ~0, cpu_id);
		md->pvh_attrs &= ~PGA_MODIFIED;
	}

	mutex_exit(lock);
	PMAP_HEAD_TO_MAP_UNLOCK();

	return (rv);
}

/*
 * pmap_clear_reference:	[ INTERFACE ]
 *
 *	Clear the reference bit on the specified physical page.
 */
bool
pmap_clear_reference(struct vm_page *pg)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	bool rv = false;
	long cpu_id = cpu_number();
	kmutex_t *lock;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_reference(%p)\n", pg);
#endif

	PMAP_HEAD_TO_MAP_LOCK();
	lock = pmap_pvh_lock(pg);
	mutex_enter(lock);

	if (md->pvh_attrs & PGA_REFERENCED) {
		rv = true;
		pmap_changebit(pg, PG_FOR | PG_FOW | PG_FOE, ~0, cpu_id);
		md->pvh_attrs &= ~PGA_REFERENCED;
	}

	mutex_exit(lock);
	PMAP_HEAD_TO_MAP_UNLOCK();

	return (rv);
}

/*
 * pmap_is_referenced:		[ INTERFACE ]
 *
 *	Return whether or not the specified physical page is referenced
 *	by any physical maps.
 */
/* See <machine/pmap.h> */

/*
 * pmap_is_modified:		[ INTERFACE ]
 *
 *	Return whether or not the specified physical page is modified
 *	by any physical maps.
 */
/* See <machine/pmap.h> */

/*
 * pmap_phys_address:		[ INTERFACE ]
 *
 *	Return the physical address corresponding to the specified
 *	cookie.  Used by the device pager to decode a device driver's
 *	mmap entry point return value.
 *
 *	Note: no locking is necessary in this function.
 */
paddr_t
pmap_phys_address(paddr_t ppn)
{

	return (alpha_ptob(ppn));
}

/*
 * Miscellaneous support routines follow
 */

/*
 * alpha_protection_init:
 *
 *	Initialize Alpha protection code array.
 *
 *	Note: no locking is necessary in this function.
 */
static void
alpha_protection_init(void)
{
	int prot, *kp, *up;

	kp = protection_codes[0];
	up = protection_codes[1];

	for (prot = 0; prot < 8; prot++) {
		kp[prot] = PG_ASM;
		up[prot] = 0;

		if (prot & VM_PROT_READ) {
			kp[prot] |= PG_KRE;
			up[prot] |= PG_KRE | PG_URE;
		}
		if (prot & VM_PROT_WRITE) {
			kp[prot] |= PG_KWE;
			up[prot] |= PG_KWE | PG_UWE;
		}
		if (prot & VM_PROT_EXECUTE) {
			kp[prot] |= PG_EXEC | PG_KRE;
			up[prot] |= PG_EXEC | PG_KRE | PG_URE;
		} else {
			kp[prot] |= PG_FOE;
			up[prot] |= PG_FOE;
		}
	}
}

/*
 * pmap_remove_mapping:
 *
 *	Invalidate a single page denoted by pmap/va.
 *
 *	If (pte != NULL), it is the already computed PTE for the page.
 *
 *	Note: locking in this function is complicated by the fact
 *	that we can be called when the PV list is already locked.
 *	(pmap_page_protect()).  In this case, the caller must be
 *	careful to get the next PV entry while we remove this entry
 *	from beneath it.  We assume that the pmap itself is already
 *	locked; dolock applies only to the PV list.
 *
 *	Returns true or false, indicating if an I-stream sync needs
 *	to be initiated (for this CPU or for other CPUs).
 */
static bool
pmap_remove_mapping(pmap_t pmap, vaddr_t va, pt_entry_t *pte,
    bool dolock, long cpu_id)
{
	paddr_t pa;
	struct vm_page *pg;		/* if != NULL, page is managed */
	bool onpv;
	bool hadasm;
	bool isactive;
	bool needisync = false;
	PMAP_TLB_SHOOTDOWN_CPUSET_DECL

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove_mapping(%p, %lx, %p, %d, %ld)\n",
		       pmap, va, pte, dolock, cpu_id);
#endif

	/*
	 * PTE not provided, compute it from pmap and va.
	 */
	if (pte == NULL) {
		pte = pmap_l3pte(pmap, va, NULL);
		if (pmap_pte_v(pte) == 0)
			return (false);
	}

	pa = pmap_pte_pa(pte);
	onpv = (pmap_pte_pv(pte) != 0);
	hadasm = (pmap_pte_asm(pte) != 0);
	isactive = PMAP_ISACTIVE(pmap, cpu_id);

	/*
	 * Determine what we need to do about the I-stream.  If
	 * PG_EXEC was set, we mark a user pmap as needing an
	 * I-sync on the way out to userspace.  We always need
	 * an immediate I-sync for the kernel pmap.
	 */
	if (pmap_pte_exec(pte)) {
		if (pmap == pmap_kernel())
			needisync = true;
		else {
			PMAP_SET_NEEDISYNC(pmap);
			needisync = (pmap->pm_cpus != 0);
		}
	}

	/*
	 * Update statistics
	 */
	if (pmap_pte_w(pte))
		PMAP_STAT_DECR(pmap->pm_stats.wired_count, 1);
	PMAP_STAT_DECR(pmap->pm_stats.resident_count, 1);

	/*
	 * Invalidate the PTE after saving the reference modify info.
	 */
#ifdef DEBUG
	if (pmapdebug & PDB_REMOVE)
		printf("remove: invalidating pte at %p\n", pte);
#endif
	PMAP_SET_PTE(pte, PG_NV);

	PMAP_INVALIDATE_TLB(pmap, va, hadasm, isactive, cpu_id);
	PMAP_TLB_SHOOTDOWN(pmap, va, hadasm ? PG_ASM : 0);
	PMAP_TLB_SHOOTNOW();

	/*
	 * If we're removing a user mapping, check to see if we
	 * can free page table pages.
	 */
	if (pmap != pmap_kernel()) {
		/*
		 * Delete the reference on the level 3 table.  It will
		 * delete references on the level 2 and 1 tables as
		 * appropriate.
		 */
		pmap_l3pt_delref(pmap, va, pte, cpu_id);
	}

	/*
	 * If the mapping wasn't entered on the PV list, we're all done.
	 */
	if (onpv == false)
		return (needisync);

	/*
	 * Remove it from the PV table.
	 */
	pg = PHYS_TO_VM_PAGE(pa);
	KASSERT(pg != NULL);
	pmap_pv_remove(pmap, pg, va, dolock);

	return (needisync);
}

/*
 * pmap_changebit:
 *
 *	Set or clear the specified PTE bits for all mappings on the
 *	specified page.
 *
 *	Note: we assume that the pv_head is already locked, and that
 *	the caller has acquired a PV->pmap mutex so that we can lock
 *	the pmaps as we encounter them.
 */
static void
pmap_changebit(struct vm_page *pg, u_long set, u_long mask, long cpu_id)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pv;
	pt_entry_t *pte, npte;
	vaddr_t va;
	bool hadasm, isactive;
	PMAP_TLB_SHOOTDOWN_CPUSET_DECL

#ifdef DEBUG
	if (pmapdebug & PDB_BITS)
		printf("pmap_changebit(%p, 0x%lx, 0x%lx)\n",
		    pg, set, mask);
#endif

	/*
	 * Loop over all current mappings setting/clearing as apropos.
	 */
	for (pv = md->pvh_list; pv != NULL; pv = pv->pv_next) {
		va = pv->pv_va;

		PMAP_LOCK(pv->pv_pmap);

		pte = pv->pv_pte;
		npte = (*pte | set) & mask;
		if (*pte != npte) {
			hadasm = (pmap_pte_asm(pte) != 0);
			isactive = PMAP_ISACTIVE(pv->pv_pmap, cpu_id);
			PMAP_SET_PTE(pte, npte);
			PMAP_INVALIDATE_TLB(pv->pv_pmap, va, hadasm, isactive,
			    cpu_id);
			PMAP_TLB_SHOOTDOWN(pv->pv_pmap, va,
			    hadasm ? PG_ASM : 0);
		}
		PMAP_UNLOCK(pv->pv_pmap);
	}

	PMAP_TLB_SHOOTNOW();
}

/*
 * pmap_emulate_reference:
 *
 *	Emulate reference and/or modified bit hits.
 *	Return 1 if this was an execute fault on a non-exec mapping,
 *	otherwise return 0.
 */
int
pmap_emulate_reference(struct lwp *l, vaddr_t v, int user, int type)
{
	struct pmap *pmap = l->l_proc->p_vmspace->vm_map.pmap;
	pt_entry_t faultoff, *pte;
	struct vm_page *pg;
	paddr_t pa;
	bool didlock = false;
	bool exec = false;
	long cpu_id = cpu_number();
	kmutex_t *lock;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_emulate_reference: %p, 0x%lx, %d, %d\n",
		    l, v, user, type);
#endif

	/*
	 * Convert process and virtual address to physical address.
	 */
	if (v >= VM_MIN_KERNEL_ADDRESS) {
		if (user)
			panic("pmap_emulate_reference: user ref to kernel");
		/*
		 * No need to lock here; kernel PT pages never go away.
		 */
		pte = PMAP_KERNEL_PTE(v);
	} else {
#ifdef DIAGNOSTIC
		if (l == NULL)
			panic("pmap_emulate_reference: bad proc");
		if (l->l_proc->p_vmspace == NULL)
			panic("pmap_emulate_reference: bad p_vmspace");
#endif
		PMAP_LOCK(pmap);
		didlock = true;
		pte = pmap_l3pte(pmap, v, NULL);
		/*
		 * We'll unlock below where we're done with the PTE.
		 */
	}
	exec = pmap_pte_exec(pte);
	if (!exec && type == ALPHA_MMCSR_FOE) {
		if (didlock)
			PMAP_UNLOCK(pmap);
	       return (1);
	}
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW) {
		printf("\tpte = %p, ", pte);
		printf("*pte = 0x%lx\n", *pte);
	}
#endif
#ifdef DEBUG				/* These checks are more expensive */
	if (!pmap_pte_v(pte))
		panic("pmap_emulate_reference: invalid pte");
	if (type == ALPHA_MMCSR_FOW) {
		if (!(*pte & (user ? PG_UWE : PG_UWE | PG_KWE)))
			panic("pmap_emulate_reference: write but unwritable");
		if (!(*pte & PG_FOW))
			panic("pmap_emulate_reference: write but not FOW");
	} else {
		if (!(*pte & (user ? PG_URE : PG_URE | PG_KRE)))
			panic("pmap_emulate_reference: !write but unreadable");
		if (!(*pte & (PG_FOR | PG_FOE)))
			panic("pmap_emulate_reference: !write but not FOR|FOE");
	}
	/* Other diagnostics? */
#endif
	pa = pmap_pte_pa(pte);

	/*
	 * We're now done with the PTE.  If it was a user pmap, unlock
	 * it now.
	 */
	if (didlock)
		PMAP_UNLOCK(pmap);

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("\tpa = 0x%lx\n", pa);
#endif
#ifdef DIAGNOSTIC
	if (!uvm_pageismanaged(pa))
		panic("pmap_emulate_reference(%p, 0x%lx, %d, %d): "
		      "pa 0x%lx not managed", l, v, user, type, pa);
#endif

	/*
	 * Twiddle the appropriate bits to reflect the reference
	 * and/or modification..
	 *
	 * The rules:
	 * 	(1) always mark page as used, and
	 *	(2) if it was a write fault, mark page as modified.
	 */
	pg = PHYS_TO_VM_PAGE(pa);
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);

	PMAP_HEAD_TO_MAP_LOCK();
	lock = pmap_pvh_lock(pg);
	mutex_enter(lock);

	if (type == ALPHA_MMCSR_FOW) {
		md->pvh_attrs |= (PGA_REFERENCED|PGA_MODIFIED);
		faultoff = PG_FOR | PG_FOW;
	} else {
		md->pvh_attrs |= PGA_REFERENCED;
		faultoff = PG_FOR;
		if (exec) {
			faultoff |= PG_FOE;
		}
	}
	pmap_changebit(pg, 0, ~faultoff, cpu_id);

	mutex_exit(lock);
	PMAP_HEAD_TO_MAP_UNLOCK();
	return (0);
}

#ifdef DEBUG
/*
 * pmap_pv_dump:
 *
 *	Dump the physical->virtual data for the specified page.
 */
void
pmap_pv_dump(paddr_t pa)
{
	struct vm_page *pg;
	struct vm_page_md *md;
	pv_entry_t pv;
	kmutex_t *lock;

	pg = PHYS_TO_VM_PAGE(pa);
	md = VM_PAGE_TO_MD(pg);

	lock = pmap_pvh_lock(pg);
	mutex_enter(lock);

	printf("pa 0x%lx (attrs = 0x%x):\n", pa, md->pvh_attrs);
	for (pv = md->pvh_list; pv != NULL; pv = pv->pv_next)
		printf("     pmap %p, va 0x%lx\n",
		    pv->pv_pmap, pv->pv_va);
	printf("\n");

	mutex_exit(lock);
}
#endif

/*
 * vtophys:
 *
 *	Return the physical address corresponding to the K0SEG or
 *	K1SEG address provided.
 *
 *	Note: no locking is necessary in this function.
 */
paddr_t
vtophys(vaddr_t vaddr)
{
	pt_entry_t *pte;
	paddr_t paddr = 0;

	if (vaddr < ALPHA_K0SEG_BASE)
		printf("vtophys: invalid vaddr 0x%lx", vaddr);
	else if (vaddr <= ALPHA_K0SEG_END)
		paddr = ALPHA_K0SEG_TO_PHYS(vaddr);
	else {
		pte = PMAP_KERNEL_PTE(vaddr);
		if (pmap_pte_v(pte))
			paddr = pmap_pte_pa(pte) | (vaddr & PGOFSET);
	}

#if 0
	printf("vtophys(0x%lx) -> 0x%lx\n", vaddr, paddr);
#endif

	return (paddr);
}

/******************** pv_entry management ********************/

/*
 * pmap_pv_enter:
 *
 *	Add a physical->virtual entry to the pv_table.
 */
static int
pmap_pv_enter(pmap_t pmap, struct vm_page *pg, vaddr_t va, pt_entry_t *pte,
    bool dolock)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t newpv;
	kmutex_t *lock;

	/*
	 * Allocate and fill in the new pv_entry.
	 */
	newpv = pmap_pv_alloc();
	if (newpv == NULL)
		return ENOMEM;
	newpv->pv_va = va;
	newpv->pv_pmap = pmap;
	newpv->pv_pte = pte;

	if (dolock) {
		lock = pmap_pvh_lock(pg);
		mutex_enter(lock);
	}

#ifdef DEBUG
    {
	pv_entry_t pv;
	/*
	 * Make sure the entry doesn't already exist.
	 */
	for (pv = md->pvh_list; pv != NULL; pv = pv->pv_next) {
		if (pmap == pv->pv_pmap && va == pv->pv_va) {
			printf("pmap = %p, va = 0x%lx\n", pmap, va);
			panic("pmap_pv_enter: already in pv table");
		}
	}
    }
#endif

	/*
	 * ...and put it in the list.
	 */
	newpv->pv_next = md->pvh_list;
	md->pvh_list = newpv;

	if (dolock) {
		mutex_exit(lock);
	}

	return 0;
}

/*
 * pmap_pv_remove:
 *
 *	Remove a physical->virtual entry from the pv_table.
 */
static void
pmap_pv_remove(pmap_t pmap, struct vm_page *pg, vaddr_t va, bool dolock)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pv, *pvp;
	kmutex_t *lock;

	if (dolock) {
		lock = pmap_pvh_lock(pg);
		mutex_enter(lock);
	} else {
		lock = NULL; /* XXX stupid gcc */
	}

	/*
	 * Find the entry to remove.
	 */
	for (pvp = &md->pvh_list, pv = *pvp;
	     pv != NULL; pvp = &pv->pv_next, pv = *pvp)
		if (pmap == pv->pv_pmap && va == pv->pv_va)
			break;

#ifdef DEBUG
	if (pv == NULL)
		panic("pmap_pv_remove: not in pv table");
#endif

	*pvp = pv->pv_next;

	if (dolock) {
		mutex_exit(lock);
	}

	pmap_pv_free(pv);
}

/*
 * pmap_pv_page_alloc:
 *
 *	Allocate a page for the pv_entry pool.
 */
static void *
pmap_pv_page_alloc(struct pool *pp, int flags)
{
	paddr_t pg;

	if (pmap_physpage_alloc(PGU_PVENT, &pg))
		return ((void *)ALPHA_PHYS_TO_K0SEG(pg));
	return (NULL);
}

/*
 * pmap_pv_page_free:
 *
 *	Free a pv_entry pool page.
 */
static void
pmap_pv_page_free(struct pool *pp, void *v)
{

	pmap_physpage_free(ALPHA_K0SEG_TO_PHYS((vaddr_t)v));
}

/******************** misc. functions ********************/

/*
 * pmap_physpage_alloc:
 *
 *	Allocate a single page from the VM system and return the
 *	physical address for that page.
 */
static bool
pmap_physpage_alloc(int usage, paddr_t *pap)
{
	struct vm_page *pg;
	paddr_t pa;

	/*
	 * Don't ask for a zero'd page in the L1PT case -- we will
	 * properly initialize it in the constructor.
	 */

	pg = uvm_pagealloc(NULL, 0, NULL, usage == PGU_L1PT ?
	    UVM_PGA_USERESERVE : UVM_PGA_USERESERVE|UVM_PGA_ZERO);
	if (pg != NULL) {
		pa = VM_PAGE_TO_PHYS(pg);
#ifdef DEBUG
		struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
		if (md->pvh_refcnt != 0) {
			printf("pmap_physpage_alloc: page 0x%lx has "
			    "%d references\n", pa, md->pvh_refcnt);
			panic("pmap_physpage_alloc");
		}
#endif
		*pap = pa;
		return (true);
	}
	return (false);
}

/*
 * pmap_physpage_free:
 *
 *	Free the single page table page at the specified physical address.
 */
static void
pmap_physpage_free(paddr_t pa)
{
	struct vm_page *pg;

	if ((pg = PHYS_TO_VM_PAGE(pa)) == NULL)
		panic("pmap_physpage_free: bogus physical page address");

#ifdef DEBUG
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	if (md->pvh_refcnt != 0)
		panic("pmap_physpage_free: page still has references");
#endif

	uvm_pagefree(pg);
}

/*
 * pmap_physpage_addref:
 *
 *	Add a reference to the specified special use page.
 */
static int
pmap_physpage_addref(void *kva)
{
	struct vm_page *pg;
	struct vm_page_md *md;
	paddr_t pa;

	pa = ALPHA_K0SEG_TO_PHYS(trunc_page((vaddr_t)kva));
	pg = PHYS_TO_VM_PAGE(pa);
	md = VM_PAGE_TO_MD(pg);

	KASSERT((int)md->pvh_refcnt >= 0);

	return atomic_inc_uint_nv(&md->pvh_refcnt);
}

/*
 * pmap_physpage_delref:
 *
 *	Delete a reference to the specified special use page.
 */
static int
pmap_physpage_delref(void *kva)
{
	struct vm_page *pg;
	struct vm_page_md *md;
	paddr_t pa;

	pa = ALPHA_K0SEG_TO_PHYS(trunc_page((vaddr_t)kva));
	pg = PHYS_TO_VM_PAGE(pa);
	md = VM_PAGE_TO_MD(pg);

	KASSERT((int)md->pvh_refcnt > 0);

	return atomic_dec_uint_nv(&md->pvh_refcnt);
}

/******************** page table page management ********************/

/*
 * pmap_growkernel:		[ INTERFACE ]
 *
 *	Grow the kernel address space.  This is a hint from the
 *	upper layer to pre-allocate more kernel PT pages.
 */
vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	struct pmap *kpm = pmap_kernel(), *pm;
	paddr_t ptaddr;
	pt_entry_t *l1pte, *l2pte, pte;
	vaddr_t va;
	int l1idx;

	rw_enter(&pmap_growkernel_lock, RW_WRITER);

	if (maxkvaddr <= virtual_end)
		goto out;		/* we are OK */

	va = virtual_end;

	while (va < maxkvaddr) {
		/*
		 * If there is no valid L1 PTE (i.e. no L2 PT page),
		 * allocate a new L2 PT page and insert it into the
		 * L1 map.
		 */
		l1pte = pmap_l1pte(kpm, va);
		if (pmap_pte_v(l1pte) == 0) {
			/*
			 * XXX PGU_NORMAL?  It's not a "traditional" PT page.
			 */
			if (uvm.page_init_done == false) {
				/*
				 * We're growing the kernel pmap early (from
				 * uvm_pageboot_alloc()).  This case must
				 * be handled a little differently.
				 */
				ptaddr = ALPHA_K0SEG_TO_PHYS(
				    pmap_steal_memory(PAGE_SIZE, NULL, NULL));
			} else if (pmap_physpage_alloc(PGU_NORMAL,
				   &ptaddr) == false)
				goto die;
			pte = (atop(ptaddr) << PG_SHIFT) |
			    PG_V | PG_ASM | PG_KRE | PG_KWE | PG_WIRED;
			*l1pte = pte;

			l1idx = l1pte_index(va);

			/* Update all the user pmaps. */
			mutex_enter(&pmap_all_pmaps_lock);
			for (pm = TAILQ_FIRST(&pmap_all_pmaps);
			     pm != NULL; pm = TAILQ_NEXT(pm, pm_list)) {
				/* Skip the kernel pmap. */
				if (pm == pmap_kernel())
					continue;

				PMAP_LOCK(pm);
				if (pm->pm_lev1map == kernel_lev1map) {
					PMAP_UNLOCK(pm);
					continue;
				}
				pm->pm_lev1map[l1idx] = pte;
				PMAP_UNLOCK(pm);
			}
			mutex_exit(&pmap_all_pmaps_lock);
		}

		/*
		 * Have an L2 PT page now, add the L3 PT page.
		 */
		l2pte = pmap_l2pte(kpm, va, l1pte);
		KASSERT(pmap_pte_v(l2pte) == 0);
		if (uvm.page_init_done == false) {
			/*
			 * See above.
			 */
			ptaddr = ALPHA_K0SEG_TO_PHYS(
			    pmap_steal_memory(PAGE_SIZE, NULL, NULL));
		} else if (pmap_physpage_alloc(PGU_NORMAL, &ptaddr) == false)
			goto die;
		*l2pte = (atop(ptaddr) << PG_SHIFT) |
		    PG_V | PG_ASM | PG_KRE | PG_KWE | PG_WIRED;
		va += ALPHA_L2SEG_SIZE;
	}

	/* Invalidate the L1 PT cache. */
	pool_cache_invalidate(&pmap_l1pt_cache);

	virtual_end = va;

 out:
	rw_exit(&pmap_growkernel_lock);

	return (virtual_end);

 die:
	panic("pmap_growkernel: out of memory");
}

/*
 * pmap_lev1map_create:
 *
 *	Create a new level 1 page table for the specified pmap.
 *
 *	Note: growkernel must already be held and the pmap either
 *	already locked or unreferenced globally.
 */
static int
pmap_lev1map_create(pmap_t pmap, long cpu_id)
{
	pt_entry_t *l1pt;

	KASSERT(pmap != pmap_kernel());

	KASSERT(pmap->pm_lev1map == kernel_lev1map);
	KASSERT(pmap->pm_asni[cpu_id].pma_asn == PMAP_ASN_RESERVED);

	/* Don't sleep -- we're called with locks held. */
	l1pt = pool_cache_get(&pmap_l1pt_cache, PR_NOWAIT);
	if (l1pt == NULL)
		return (ENOMEM);

	pmap->pm_lev1map = l1pt;
	return (0);
}

/*
 * pmap_lev1map_destroy:
 *
 *	Destroy the level 1 page table for the specified pmap.
 *
 *	Note: growkernel must be held and the pmap must already be
 *	locked or not globally referenced.
 */
static void
pmap_lev1map_destroy(pmap_t pmap, long cpu_id)
{
	pt_entry_t *l1pt = pmap->pm_lev1map;

	KASSERT(pmap != pmap_kernel());

	/*
	 * Go back to referencing the global kernel_lev1map.
	 */
	pmap->pm_lev1map = kernel_lev1map;

	/*
	 * Free the old level 1 page table page.
	 */
	pool_cache_put(&pmap_l1pt_cache, l1pt);
}

/*
 * pmap_l1pt_ctor:
 *
 *	Pool cache constructor for L1 PT pages.
 *
 *	Note: The growkernel lock is held across allocations
 *	from our pool_cache, so we don't need to acquire it
 *	ourselves.
 */
static int
pmap_l1pt_ctor(void *arg, void *object, int flags)
{
	pt_entry_t *l1pt = object, pte;
	int i;

	/*
	 * Initialize the new level 1 table by zeroing the
	 * user portion and copying the kernel mappings into
	 * the kernel portion.
	 */
	for (i = 0; i < l1pte_index(VM_MIN_KERNEL_ADDRESS); i++)
		l1pt[i] = 0;

	for (i = l1pte_index(VM_MIN_KERNEL_ADDRESS);
	     i <= l1pte_index(VM_MAX_KERNEL_ADDRESS); i++)
		l1pt[i] = kernel_lev1map[i];

	/*
	 * Now, map the new virtual page table.  NOTE: NO ASM!
	 */
	pte = ((ALPHA_K0SEG_TO_PHYS((vaddr_t) l1pt) >> PGSHIFT) << PG_SHIFT) |
	    PG_V | PG_KRE | PG_KWE;
	l1pt[l1pte_index(VPTBASE)] = pte;

	return (0);
}

/*
 * pmap_l1pt_alloc:
 *
 *	Page alloctaor for L1 PT pages.
 */
static void *
pmap_l1pt_alloc(struct pool *pp, int flags)
{
	paddr_t ptpa;

	/*
	 * Attempt to allocate a free page.
	 */
	if (pmap_physpage_alloc(PGU_L1PT, &ptpa) == false)
		return (NULL);

	return ((void *) ALPHA_PHYS_TO_K0SEG(ptpa));
}

/*
 * pmap_l1pt_free:
 *
 *	Page freer for L1 PT pages.
 */
static void
pmap_l1pt_free(struct pool *pp, void *v)
{

	pmap_physpage_free(ALPHA_K0SEG_TO_PHYS((vaddr_t) v));
}

/*
 * pmap_ptpage_alloc:
 *
 *	Allocate a level 2 or level 3 page table page, and
 *	initialize the PTE that references it.
 *
 *	Note: the pmap must already be locked.
 */
static int
pmap_ptpage_alloc(pmap_t pmap, pt_entry_t *pte, int usage)
{
	paddr_t ptpa;

	/*
	 * Allocate the page table page.
	 */
	if (pmap_physpage_alloc(usage, &ptpa) == false)
		return (ENOMEM);

	/*
	 * Initialize the referencing PTE.
	 */
	PMAP_SET_PTE(pte, ((ptpa >> PGSHIFT) << PG_SHIFT) |
	    PG_V | PG_KRE | PG_KWE | PG_WIRED |
	    (pmap == pmap_kernel() ? PG_ASM : 0));

	return (0);
}

/*
 * pmap_ptpage_free:
 *
 *	Free the level 2 or level 3 page table page referenced
 *	be the provided PTE.
 *
 *	Note: the pmap must already be locked.
 */
static void
pmap_ptpage_free(pmap_t pmap, pt_entry_t *pte)
{
	paddr_t ptpa;

	/*
	 * Extract the physical address of the page from the PTE
	 * and clear the entry.
	 */
	ptpa = pmap_pte_pa(pte);
	PMAP_SET_PTE(pte, PG_NV);

#ifdef DEBUG
	pmap_zero_page(ptpa);
#endif
	pmap_physpage_free(ptpa);
}

/*
 * pmap_l3pt_delref:
 *
 *	Delete a reference on a level 3 PT page.  If the reference drops
 *	to zero, free it.
 *
 *	Note: the pmap must already be locked.
 */
static void
pmap_l3pt_delref(pmap_t pmap, vaddr_t va, pt_entry_t *l3pte, long cpu_id)
{
	pt_entry_t *l1pte, *l2pte;
	PMAP_TLB_SHOOTDOWN_CPUSET_DECL

	l1pte = pmap_l1pte(pmap, va);
	l2pte = pmap_l2pte(pmap, va, l1pte);

#ifdef DIAGNOSTIC
	if (pmap == pmap_kernel())
		panic("pmap_l3pt_delref: kernel pmap");
#endif

	if (pmap_physpage_delref(l3pte) == 0) {
		/*
		 * No more mappings; we can free the level 3 table.
		 */
#ifdef DEBUG
		if (pmapdebug & PDB_PTPAGE)
			printf("pmap_l3pt_delref: freeing level 3 table at "
			    "0x%lx\n", pmap_pte_pa(l2pte));
#endif
		pmap_ptpage_free(pmap, l2pte);

		/*
		 * We've freed a level 3 table, so we must
		 * invalidate the TLB entry for that PT page
		 * in the Virtual Page Table VA range, because
		 * otherwise the PALcode will service a TLB
		 * miss using the stale VPT TLB entry it entered
		 * behind our back to shortcut to the VA's PTE.
		 */
		PMAP_INVALIDATE_TLB(pmap,
		    (vaddr_t)(&VPT[VPT_INDEX(va)]), false,
		    PMAP_ISACTIVE(pmap, cpu_id), cpu_id);
		PMAP_TLB_SHOOTDOWN(pmap,
		    (vaddr_t)(&VPT[VPT_INDEX(va)]), 0);
		PMAP_TLB_SHOOTNOW();

		/*
		 * We've freed a level 3 table, so delete the reference
		 * on the level 2 table.
		 */
		pmap_l2pt_delref(pmap, l1pte, l2pte, cpu_id);
	}
}

/*
 * pmap_l2pt_delref:
 *
 *	Delete a reference on a level 2 PT page.  If the reference drops
 *	to zero, free it.
 *
 *	Note: the pmap must already be locked.
 */
static void
pmap_l2pt_delref(pmap_t pmap, pt_entry_t *l1pte, pt_entry_t *l2pte,
    long cpu_id)
{

#ifdef DIAGNOSTIC
	if (pmap == pmap_kernel())
		panic("pmap_l2pt_delref: kernel pmap");
#endif

	if (pmap_physpage_delref(l2pte) == 0) {
		/*
		 * No more mappings in this segment; we can free the
		 * level 2 table.
		 */
#ifdef DEBUG
		if (pmapdebug & PDB_PTPAGE)
			printf("pmap_l2pt_delref: freeing level 2 table at "
			    "0x%lx\n", pmap_pte_pa(l1pte));
#endif
		pmap_ptpage_free(pmap, l1pte);

		/*
		 * We've freed a level 2 table, so delete the reference
		 * on the level 1 table.
		 */
		pmap_l1pt_delref(pmap, l1pte, cpu_id);
	}
}

/*
 * pmap_l1pt_delref:
 *
 *	Delete a reference on a level 1 PT page.  If the reference drops
 *	to zero, free it.
 *
 *	Note: the pmap must already be locked.
 */
static void
pmap_l1pt_delref(pmap_t pmap, pt_entry_t *l1pte, long cpu_id)
{

#ifdef DIAGNOSTIC
	if (pmap == pmap_kernel())
		panic("pmap_l1pt_delref: kernel pmap");
#endif

	(void)pmap_physpage_delref(l1pte);
}

/******************** Address Space Number management ********************/

/*
 * pmap_asn_alloc:
 *
 *	Allocate and assign an ASN to the specified pmap.
 *
 *	Note: the pmap must already be locked.  This may be called from
 *	an interprocessor interrupt, and in that case, the sender of
 *	the IPI has the pmap lock.
 */
static void
pmap_asn_alloc(pmap_t pmap, long cpu_id)
{
	struct pmap_asn_info *pma = &pmap->pm_asni[cpu_id];
	struct pmap_asn_info *cpma = &pmap_asn_info[cpu_id];

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ASN))
		printf("pmap_asn_alloc(%p)\n", pmap);
#endif

	/*
	 * If the pmap is still using the global kernel_lev1map, there
	 * is no need to assign an ASN at this time, because only
	 * kernel mappings exist in that map, and all kernel mappings
	 * have PG_ASM set.  If the pmap eventually gets its own
	 * lev1map, an ASN will be allocated at that time.
	 *
	 * Only the kernel pmap will reference kernel_lev1map.  Do the
	 * same old fixups, but note that we no longer need the pmap
	 * to be locked if we're in this mode, since pm_lev1map will
	 * never change.
	 * #endif
	 */
	if (pmap->pm_lev1map == kernel_lev1map) {
#ifdef DEBUG
		if (pmapdebug & PDB_ASN)
			printf("pmap_asn_alloc: still references "
			    "kernel_lev1map\n");
#endif
#if defined(MULTIPROCESSOR)
		/*
		 * In a multiprocessor system, it's possible to
		 * get here without having PMAP_ASN_RESERVED in
		 * pmap->pm_asni[cpu_id].pma_asn; see pmap_lev1map_destroy().
		 *
		 * So, what we do here, is simply assign the reserved
		 * ASN for kernel_lev1map users and let things
		 * continue on.  We do, however, let uniprocessor
		 * configurations continue to make its assertion.
		 */
		pma->pma_asn = PMAP_ASN_RESERVED;
#else
		KASSERT(pma->pma_asn == PMAP_ASN_RESERVED);
#endif /* MULTIPROCESSOR */
		return;
	}

	/*
	 * On processors which do not implement ASNs, the swpctx PALcode
	 * operation will automatically invalidate the TLB and I-cache,
	 * so we don't need to do that here.
	 */
	if (pmap_max_asn == 0) {
		/*
		 * Refresh the pmap's generation number, to
		 * simplify logic elsewhere.
		 */
		pma->pma_asngen = cpma->pma_asngen;
#ifdef DEBUG
		if (pmapdebug & PDB_ASN)
			printf("pmap_asn_alloc: no ASNs, using asngen %lu\n",
			    pma->pma_asngen);
#endif
		return;
	}

	/*
	 * Hopefully, we can continue using the one we have...
	 */
	if (pma->pma_asn != PMAP_ASN_RESERVED &&
	    pma->pma_asngen == cpma->pma_asngen) {
		/*
		 * ASN is still in the current generation; keep on using it.
		 */
#ifdef DEBUG
		if (pmapdebug & PDB_ASN)
			printf("pmap_asn_alloc: same generation, keeping %u\n",
			    pma->pma_asn);
#endif
		return;
	}

	/*
	 * Need to assign a new ASN.  Grab the next one, incrementing
	 * the generation number if we have to.
	 */
	if (cpma->pma_asn > pmap_max_asn) {
		/*
		 * Invalidate all non-PG_ASM TLB entries and the
		 * I-cache, and bump the generation number.
		 */
		ALPHA_TBIAP();
		alpha_pal_imb();

		cpma->pma_asn = 1;
		cpma->pma_asngen++;
#ifdef DIAGNOSTIC
		if (cpma->pma_asngen == 0) {
			/*
			 * The generation number has wrapped.  We could
			 * handle this scenario by traversing all of
			 * the pmaps, and invalidating the generation
			 * number on those which are not currently
			 * in use by this processor.
			 *
			 * However... considering that we're using
			 * an unsigned 64-bit integer for generation
			 * numbers, on non-ASN CPUs, we won't wrap
			 * for approx. 585 million years, or 75 billion
			 * years on a 128-ASN CPU (assuming 1000 switch
			 * operations per second).
			 *
			 * So, we don't bother.
			 */
			panic("pmap_asn_alloc: too much uptime");
		}
#endif
#ifdef DEBUG
		if (pmapdebug & PDB_ASN)
			printf("pmap_asn_alloc: generation bumped to %lu\n",
			    cpma->pma_asngen);
#endif
	}

	/*
	 * Assign the new ASN and validate the generation number.
	 */
	pma->pma_asn = cpma->pma_asn++;
	pma->pma_asngen = cpma->pma_asngen;

#ifdef DEBUG
	if (pmapdebug & PDB_ASN)
		printf("pmap_asn_alloc: assigning %u to pmap %p\n",
		    pma->pma_asn, pmap);
#endif

	/*
	 * Have a new ASN, so there's no need to sync the I-stream
	 * on the way back out to userspace.
	 */
	atomic_and_ulong(&pmap->pm_needisync, ~(1UL << cpu_id));
}

#if defined(MULTIPROCESSOR)
/******************** TLB shootdown code ********************/

/*
 * pmap_tlb_shootdown:
 *
 *	Cause the TLB entry for pmap/va to be shot down.
 *
 *	NOTE: The pmap must be locked here.
 */
void
pmap_tlb_shootdown(pmap_t pmap, vaddr_t va, pt_entry_t pte, u_long *cpumaskp)
{
	struct pmap_tlb_shootdown_q *pq;
	struct pmap_tlb_shootdown_job *pj;
	struct cpu_info *ci, *self = curcpu();
	u_long cpumask;
	CPU_INFO_ITERATOR cii;

	KASSERT((pmap == pmap_kernel()) || mutex_owned(&pmap->pm_lock));

	cpumask = 0;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci == self)
			continue;

		/*
		 * The pmap must be locked (unless its the kernel
		 * pmap, in which case it is okay for it to be
		 * unlocked), which prevents it from  becoming
		 * active on any additional processors.  This makes
		 * it safe to check for activeness.  If it's not
		 * active on the processor in question, then just
		 * mark it as needing a new ASN the next time it
		 * does, saving the IPI.  We always have to send
		 * the IPI for the kernel pmap.
		 *
		 * Note if it's marked active now, and it becomes
		 * inactive by the time the processor receives
		 * the IPI, that's okay, because it does the right
		 * thing with it later.
		 */
		if (pmap != pmap_kernel() &&
		    PMAP_ISACTIVE(pmap, ci->ci_cpuid) == 0) {
			PMAP_INVALIDATE_ASN(pmap, ci->ci_cpuid);
			continue;
		}

		cpumask |= 1UL << ci->ci_cpuid;

		pq = &pmap_tlb_shootdown_q[ci->ci_cpuid];
		mutex_spin_enter(&pq->pq_lock);

		/*
		 * Allocate a job.
		 */
		if (pq->pq_count < PMAP_TLB_SHOOTDOWN_MAXJOBS) {
			pj = pool_cache_get(&pmap_tlb_shootdown_job_cache,
			    PR_NOWAIT);
		} else {
			pj = NULL;
		}

		/*
		 * If a global flush is already pending, we
		 * don't really have to do anything else.
		 */
		pq->pq_pte |= pte;
		if (pq->pq_tbia) {
			mutex_spin_exit(&pq->pq_lock);
			if (pj != NULL) {
				pool_cache_put(&pmap_tlb_shootdown_job_cache,
				    pj);
			}
			continue;
		}
		if (pj == NULL) {
			/*
			 * Couldn't allocate a job entry.  Just
			 * tell the processor to kill everything.
			 */
			pq->pq_tbia = 1;
		} else {
			pj->pj_pmap = pmap;
			pj->pj_va = va;
			pj->pj_pte = pte;
			pq->pq_count++;
			TAILQ_INSERT_TAIL(&pq->pq_head, pj, pj_list);
		}
		mutex_spin_exit(&pq->pq_lock);
	}

	*cpumaskp |= cpumask;
}

/*
 * pmap_tlb_shootnow:
 *
 *	Process the TLB shootdowns that we have been accumulating
 *	for the specified processor set.
 */
void
pmap_tlb_shootnow(u_long cpumask)
{

	alpha_multicast_ipi(cpumask, ALPHA_IPI_SHOOTDOWN);
}

/*
 * pmap_do_tlb_shootdown:
 *
 *	Process pending TLB shootdown operations for this processor.
 */
void
pmap_do_tlb_shootdown(struct cpu_info *ci, struct trapframe *framep)
{
	u_long cpu_id = ci->ci_cpuid;
	u_long cpu_mask = (1UL << cpu_id);
	struct pmap_tlb_shootdown_q *pq = &pmap_tlb_shootdown_q[cpu_id];
	struct pmap_tlb_shootdown_job *pj, *next;
	TAILQ_HEAD(, pmap_tlb_shootdown_job) jobs;

	TAILQ_INIT(&jobs);

	mutex_spin_enter(&pq->pq_lock);
	TAILQ_CONCAT(&jobs, &pq->pq_head, pj_list);
	if (pq->pq_tbia) {
		if (pq->pq_pte & PG_ASM)
			ALPHA_TBIA();
		else
			ALPHA_TBIAP();
		pq->pq_tbia = 0;
		pq->pq_pte = 0;
	} else {
		TAILQ_FOREACH(pj, &jobs, pj_list) {
			PMAP_INVALIDATE_TLB(pj->pj_pmap, pj->pj_va,
			    pj->pj_pte & PG_ASM,
			    pj->pj_pmap->pm_cpus & cpu_mask, cpu_id);
		}
		pq->pq_pte = 0;
	}
	pq->pq_count = 0;
	mutex_spin_exit(&pq->pq_lock);

	/* Free jobs back to the cache. */
	for (pj = TAILQ_FIRST(&jobs); pj != NULL; pj = next) {
		next = TAILQ_NEXT(pj, pj_list);
		pool_cache_put(&pmap_tlb_shootdown_job_cache, pj);
	}
}
#endif /* MULTIPROCESSOR */
