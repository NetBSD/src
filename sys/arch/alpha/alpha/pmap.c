/* $NetBSD: pmap.c,v 1.307 2022/04/09 23:38:31 riastradh Exp $ */

/*-
 * Copyright (c) 1998, 1999, 2000, 2001, 2007, 2008, 2020
 * 	The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, by Andrew Doran and Mindaugas Rasiukevicius,
 * and by Chris G. Demetriou.
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
 *	TLB shootdown code was written (and then subsequently
 *	rewritten some years later, borrowing some ideas from
 *	the x86 pmap) by Jason R. Thorpe.
 *
 *	Multiprocessor modifications by Andrew Doran and
 *	Jason R. Thorpe.
 *
 * Notes:
 *
 *	All user page table access is done via K0SEG.  Kernel
 *	page table access is done via the recursive Virtual Page
 *	Table because kernel PT pages are pre-allocated and never
 *	freed, so no VPT fault handling is required.
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

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.307 2022/04/09 23:38:31 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/buf.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>

#if defined(MULTIPROCESSOR)
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

#if defined(MULTIPROCESSOR)
#define	PMAP_MP(x)	x
#else
#define	PMAP_MP(x)	__nothing
#endif /* MULTIPROCESSOR */

/*
 * Given a map and a machine independent protection code,
 * convert to an alpha protection code.
 */
#define pte_prot(m, p)	(protection_codes[m == pmap_kernel() ? 0 : 1][p])
static int	protection_codes[2][8] __read_mostly;

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
pt_entry_t	*kernel_lev1map __read_mostly;

/*
 * Virtual Page Table.
 */
static pt_entry_t *VPT __read_mostly;

static struct {
	struct pmap k_pmap;
} kernel_pmap_store __cacheline_aligned;

struct pmap *const kernel_pmap_ptr = &kernel_pmap_store.k_pmap;

/* PA of first available physical page */
paddr_t    	avail_start __read_mostly;

/* PA of last available physical page */
paddr_t		avail_end __read_mostly;

/* VA of last avail page (end of kernel AS) */
static vaddr_t	virtual_end __read_mostly;

/* Has pmap_init completed? */
static bool pmap_initialized __read_mostly;

/* Instrumentation */
u_long		pmap_pages_stolen __read_mostly;

/*
 * This variable contains the number of CPU IDs we need to allocate
 * space for when allocating the pmap structure.  It is used to
 * size a per-CPU array of ASN and ASN Generation number.
 */
static u_long 	pmap_ncpuids __read_mostly;

#ifndef PMAP_PV_LOWAT
#define	PMAP_PV_LOWAT	16
#endif
int		pmap_pv_lowat __read_mostly = PMAP_PV_LOWAT;

/*
 * List of all pmaps, used to update them when e.g. additional kernel
 * page tables are allocated.  This list is kept LRU-ordered by
 * pmap_activate().
 */
static TAILQ_HEAD(, pmap) pmap_all_pmaps __cacheline_aligned;

/*
 * Instrument the number of calls to pmap_growkernel().
 */
static struct evcnt pmap_growkernel_evcnt __read_mostly;

/*
 * The pools from which pmap structures and sub-structures are allocated.
 */
static struct pool_cache pmap_pmap_cache __read_mostly;
static struct pool_cache pmap_l1pt_cache __read_mostly;
static struct pool_cache pmap_pv_cache __read_mostly;

CTASSERT(offsetof(struct pmap, pm_percpu[0]) == COHERENCY_UNIT);
CTASSERT(PMAP_SIZEOF(ALPHA_MAXPROCS) < ALPHA_PGBYTES);
CTASSERT(sizeof(struct pmap_percpu) == COHERENCY_UNIT);

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
 * prevents the following scenario to ensure no accidental accesses to
 * user space for LWPs using the kernel pmap.  This is important because
 * the PALcode may use the recursive VPT to service TLB misses.
 *
 * By reserving an ASN for the kernel, we are guaranteeing that an lwp
 * will not see any valid user space TLB entries until it passes through
 * pmap_activate() for the first time.
 *
 * On processors that do not support ASNs, the PALcode invalidates
 * non-ASM TLB entries automatically on swpctx.  We completely skip
 * the ASN machinery in this case because the PALcode neither reads
 * nor writes that field of the HWPCB.
 */

/* max ASN supported by the system */
static u_int	pmap_max_asn __read_mostly;

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
 *	* pmap lock (global hash) - These locks protect the pmap structures.
 *
 *	* pmap activation lock (global hash) - These IPL_SCHED spin locks
 *	  synchronize pmap_activate() and TLB shootdowns.  This has a lock
 *	  ordering constraint with the tlb_lock:
 *
 *		tlb_lock -> pmap activation lock
 *
 *	* pvh_lock (global hash) - These locks protect the PV lists for
 *	  managed pages.
 *
 *	* tlb_lock - This IPL_VM lock serializes local and remote TLB
 *	  invalidation.
 *
 *	* pmap_all_pmaps_lock - This lock protects the global list of
 *	  all pmaps.
 *
 *	* pmap_growkernel_lock - This lock protects pmap_growkernel()
 *	  and the virtual_end variable.
 *
 *	  There is a lock ordering constraint for pmap_growkernel_lock.
 *	  pmap_growkernel() acquires the locks in the following order:
 *
 *		pmap_growkernel_lock (write) -> pmap_all_pmaps_lock ->
 *		    pmap lock
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
static krwlock_t pmap_main_lock __cacheline_aligned;
static kmutex_t pmap_all_pmaps_lock __cacheline_aligned;
static krwlock_t pmap_growkernel_lock __cacheline_aligned;

#define	PMAP_MAP_TO_HEAD_LOCK()		rw_enter(&pmap_main_lock, RW_READER)
#define	PMAP_MAP_TO_HEAD_UNLOCK()	rw_exit(&pmap_main_lock)
#define	PMAP_HEAD_TO_MAP_LOCK()		rw_enter(&pmap_main_lock, RW_WRITER)
#define	PMAP_HEAD_TO_MAP_UNLOCK()	rw_exit(&pmap_main_lock)

static union {
	kmutex_t	lock;
	uint8_t		pad[COHERENCY_UNIT];
} pmap_pvh_locks[64] __cacheline_aligned;

#define	PVH_LOCK_HASH(pg)						\
	((((uintptr_t)(pg)) >> 6) & 63)

static inline kmutex_t *
pmap_pvh_lock(struct vm_page *pg)
{
	return &pmap_pvh_locks[PVH_LOCK_HASH(pg)].lock;
}

static union {
	struct {
		kmutex_t	lock;
		kmutex_t	activation_lock;
	} locks;
	uint8_t		pad[COHERENCY_UNIT];
} pmap_pmap_locks[64] __cacheline_aligned;

#define	PMAP_LOCK_HASH(pm)						\
	((((uintptr_t)(pm)) >> 6) & 63)

static inline kmutex_t *
pmap_pmap_lock(pmap_t const pmap)
{
	return &pmap_pmap_locks[PMAP_LOCK_HASH(pmap)].locks.lock;
}

static inline kmutex_t *
pmap_activation_lock(pmap_t const pmap)
{
	return &pmap_pmap_locks[PMAP_LOCK_HASH(pmap)].locks.activation_lock;
}

#define	PMAP_LOCK(pmap)		mutex_enter(pmap_pmap_lock(pmap))
#define	PMAP_UNLOCK(pmap)	mutex_exit(pmap_pmap_lock(pmap))

#define	PMAP_ACT_LOCK(pmap)	mutex_spin_enter(pmap_activation_lock(pmap))
#define	PMAP_ACT_TRYLOCK(pmap)	mutex_tryenter(pmap_activation_lock(pmap))
#define	PMAP_ACT_UNLOCK(pmap)	mutex_spin_exit(pmap_activation_lock(pmap))

#if defined(MULTIPROCESSOR)
#define	pmap_all_cpus()		cpus_running
#else
#define	pmap_all_cpus()		~0UL
#endif /* MULTIPROCESSOR */

/*
 * TLB context structure; see description in "TLB management" section
 * below.
 */
#define	TLB_CTX_MAXVA		8
#define	TLB_CTX_ALLVA		PAGE_MASK
struct pmap_tlb_context {
	uintptr_t		t_addrdata[TLB_CTX_MAXVA];
	pmap_t			t_pmap;
	struct pmap_pagelist	t_freeptq;
	struct pmap_pvlist	t_freepvq;
};

/*
 * Internal routines
 */
static void	alpha_protection_init(void);
static pt_entry_t pmap_remove_mapping(pmap_t, vaddr_t, pt_entry_t *, bool,
				      pv_entry_t *,
				      struct pmap_tlb_context *);
static void	pmap_changebit(struct vm_page *, pt_entry_t, pt_entry_t,
			       struct pmap_tlb_context *);

/*
 * PT page management functions.
 */
static int	pmap_ptpage_alloc(pmap_t, pt_entry_t *, int);
static void	pmap_ptpage_free(pmap_t, pt_entry_t *,
				 struct pmap_tlb_context *);
static void	pmap_l3pt_delref(pmap_t, vaddr_t, pt_entry_t *,
		     struct pmap_tlb_context *);
static void	pmap_l2pt_delref(pmap_t, pt_entry_t *, pt_entry_t *,
		     struct pmap_tlb_context *);
static void	pmap_l1pt_delref(pmap_t, pt_entry_t *);

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
			      bool, pv_entry_t);
static void	pmap_pv_remove(pmap_t, struct vm_page *, vaddr_t, bool,
			       pv_entry_t *, struct pmap_tlb_context *);
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
 * Generic routine for freeing pages on a pmap_pagelist back to
 * the system.
 */
static void
pmap_pagelist_free(struct pmap_pagelist * const list)
{
	struct vm_page *pg;

	while ((pg = LIST_FIRST(list)) != NULL) {
		LIST_REMOVE(pg, pageq.list);
		/* Fix up ref count; it's not always 0 when we get here. */
		PHYSPAGE_REFCNT_SET(pg, 0);
		uvm_pagefree(pg);
	}
}

/*
 * Generic routine for freeing a list of PV entries back to the
 * system.
 */
static void
pmap_pvlist_free(struct pmap_pvlist * const list)
{
	pv_entry_t pv;

	while ((pv = LIST_FIRST(list)) != NULL) {
		LIST_REMOVE(pv, pv_link);
		pmap_pv_free(pv);
	}
}

/*
 * TLB management.
 *
 * TLB invalidations need to be performed on local and remote CPUs
 * whenever parts of the PTE that the hardware or PALcode understands
 * changes.  In order amortize the cost of these operations, we will
 * queue up to 8 addresses to invalidate in a batch.  Any more than
 * that, and we will hit the entire TLB.
 *
 * Some things that add complexity:
 *
 * ==> ASNs. A CPU may have valid TLB entries for other than the current
 *     address space.  We can only invalidate TLB entries for the current
 *     address space, so when asked to invalidate a VA for the non-current
 *     pmap on a given CPU, we simply invalidate the ASN for that pmap,CPU
 *     tuple so that new one is allocated on the next activation on that
 *     CPU.  N.B. that for CPUs that don't implement ASNs, SWPCTX does all
 *     the work necessary, so we can skip some work in the pmap module
 *     itself.
 *
 *     When a pmap is activated on a given CPU, we set a corresponding
 *     bit in pmap::pm_cpus, indicating that it potentially has valid
 *     TLB entries for that address space.  This bitmap is then used to
 *     determine which remote CPUs need to be notified of invalidations.
 *     The bit is cleared when the ASN is invalidated on that CPU.
 *
 *     In order to serialize with activating an address space on a
 *     given CPU (that we can reliably send notifications only to
 *     relevant remote CPUs), we acquire the pmap lock in pmap_activate()
 *     and also hold the lock while remote shootdowns take place.
 *     This does not apply to the kernel pmap; all CPUs are notified about
 *     invalidations for the kernel pmap, and the pmap lock is not held
 *     in pmap_activate() for the kernel pmap.
 *
 * ==> P->V operations (e.g. pmap_page_protect()) may require sending
 *     invalidations for multiple address spaces.  We only track one
 *     address space at a time, and if we encounter more than one, then
 *     the notification each CPU gets is to hit the entire TLB.  Note
 *     also that we can't serialize with pmap_activate() in this case,
 *     so all CPUs will get the notification, and they check when
 *     processing the notification if the pmap is current on that CPU.
 *
 * Invalidation information is gathered into a pmap_tlb_context structure
 * that includes room for 8 VAs, the pmap the VAs belong to, a bitmap of
 * CPUs to be notified, and a list for PT pages that are freed during
 * removal off mappings.  The number of valid addresses in the list as
 * well as flags are squeezed into the lower bits of the first two VAs.
 * Storage for this structure is allocated on the stack.  We need to be
 * careful to keep the size of this structure under control.
 *
 * When notifying remote CPUs, we acquire the tlb_lock (which also
 * blocks IPIs), record the pointer to our context structure, set a
 * global bitmap off CPUs to be notified, and then send the IPIs to
 * each victim.  While the other CPUs are in-flight, we then perform
 * any invalidations necessary on the local CPU.  Once that is done,
 * we then wait the global context pointer to be cleared, which
 * will be done by the final remote CPU to complete their work. This
 * method reduces cache line contention during processing.
 *
 * When removing mappings in user pmaps, this implementation frees page
 * table pages back to the VM system once they contain no valid mappings.
 * As we do this, we must ensure to invalidate TLB entries that the
 * CPU might hold for the respective recursive VPT mappings.  This must
 * be done whenever an L1 or L2 PTE is invalidated.  Until these VPT
 * translations are invalidated, the PT pages must not be reused.  For
 * this reason, we keep a list of freed PT pages in the context structure
 * and drain them off once all invalidations are complete.
 *
 * NOTE: The value of TLB_CTX_MAXVA is tuned to accommodate the UBC
 * window size (defined as 64KB on alpha in <machine/vmparam.h>).
 */

#define	TLB_CTX_F_ASM		__BIT(0)
#define	TLB_CTX_F_IMB		__BIT(1)
#define	TLB_CTX_F_KIMB		__BIT(2)
#define	TLB_CTX_F_PV		__BIT(3)
#define	TLB_CTX_F_MULTI		__BIT(4)

#define	TLB_CTX_COUNT(ctx)	((ctx)->t_addrdata[0] & PAGE_MASK)
#define	TLB_CTX_INC_COUNT(ctx)	 (ctx)->t_addrdata[0]++
#define	TLB_CTX_SET_ALLVA(ctx)	 (ctx)->t_addrdata[0] |= TLB_CTX_ALLVA

#define	TLB_CTX_FLAGS(ctx)	((ctx)->t_addrdata[1] & PAGE_MASK)
#define	TLB_CTX_SET_FLAG(ctx, f) (ctx)->t_addrdata[1] |= (f)

#define	TLB_CTX_VA(ctx, i)	((ctx)->t_addrdata[(i)] & ~PAGE_MASK)
#define	TLB_CTX_SETVA(ctx, i, va)					\
	(ctx)->t_addrdata[(i)] = (va) | ((ctx)->t_addrdata[(i)] & PAGE_MASK)

static struct {
	kmutex_t	lock;
	struct evcnt	events;
} tlb_shootdown __cacheline_aligned;
#define	tlb_lock	tlb_shootdown.lock
#define	tlb_evcnt	tlb_shootdown.events
#if defined(MULTIPROCESSOR)
static const struct pmap_tlb_context *tlb_context __cacheline_aligned;
static unsigned long tlb_pending __cacheline_aligned;
#endif /* MULTIPROCESSOR */

#if defined(TLB_STATS)
#define	TLB_COUNT_DECL(cnt)	static struct evcnt tlb_stat_##cnt
#define	TLB_COUNT(cnt)		atomic_inc_64(&tlb_stat_##cnt .ev_count)
#define	TLB_COUNT_ATTACH(cnt)						\
	evcnt_attach_dynamic_nozero(&tlb_stat_##cnt, EVCNT_TYPE_MISC,	\
	    NULL, "TLB", #cnt)

TLB_COUNT_DECL(invalidate_multi_tbia);
TLB_COUNT_DECL(invalidate_multi_tbiap);
TLB_COUNT_DECL(invalidate_multi_imb);

TLB_COUNT_DECL(invalidate_kern_tbia);
TLB_COUNT_DECL(invalidate_kern_tbis);
TLB_COUNT_DECL(invalidate_kern_imb);

TLB_COUNT_DECL(invalidate_user_not_current);
TLB_COUNT_DECL(invalidate_user_lazy_imb);
TLB_COUNT_DECL(invalidate_user_tbiap);
TLB_COUNT_DECL(invalidate_user_tbis);

TLB_COUNT_DECL(shootdown_kernel);
TLB_COUNT_DECL(shootdown_user);
TLB_COUNT_DECL(shootdown_imb);
TLB_COUNT_DECL(shootdown_kimb);
TLB_COUNT_DECL(shootdown_overflow);

TLB_COUNT_DECL(shootdown_all_user);
TLB_COUNT_DECL(shootdown_all_user_imb);

TLB_COUNT_DECL(shootdown_pv);
TLB_COUNT_DECL(shootdown_pv_multi);

TLB_COUNT_DECL(shootnow_over_notify);
TLB_COUNT_DECL(shootnow_remote);

TLB_COUNT_DECL(reason_remove_kernel);
TLB_COUNT_DECL(reason_remove_user);
TLB_COUNT_DECL(reason_remove_all_user);
TLB_COUNT_DECL(reason_page_protect_read);
TLB_COUNT_DECL(reason_page_protect_none);
TLB_COUNT_DECL(reason_protect);
TLB_COUNT_DECL(reason_enter_kernel);
TLB_COUNT_DECL(reason_enter_user);
TLB_COUNT_DECL(reason_kenter);
TLB_COUNT_DECL(reason_enter_l2pt_delref);
TLB_COUNT_DECL(reason_enter_l3pt_delref);
TLB_COUNT_DECL(reason_kremove);
TLB_COUNT_DECL(reason_clear_modify);
TLB_COUNT_DECL(reason_clear_reference);
TLB_COUNT_DECL(reason_emulate_reference);

TLB_COUNT_DECL(asn_reuse);
TLB_COUNT_DECL(asn_newgen);
TLB_COUNT_DECL(asn_assign);

TLB_COUNT_DECL(activate_both_change);
TLB_COUNT_DECL(activate_asn_change);
TLB_COUNT_DECL(activate_ptbr_change);
TLB_COUNT_DECL(activate_swpctx);
TLB_COUNT_DECL(activate_skip_swpctx);

#else /* ! TLB_STATS */
#define	TLB_COUNT(cnt)		__nothing
#define	TLB_COUNT_ATTACH(cnt)	__nothing
#endif /* TLB_STATS */

static void
pmap_tlb_init(void)
{
	/* mutex is initialized in pmap_bootstrap(). */

	evcnt_attach_dynamic_nozero(&tlb_evcnt, EVCNT_TYPE_MISC,
	    NULL, "TLB", "shootdown");

	TLB_COUNT_ATTACH(invalidate_multi_tbia);
	TLB_COUNT_ATTACH(invalidate_multi_tbiap);
	TLB_COUNT_ATTACH(invalidate_multi_imb);

	TLB_COUNT_ATTACH(invalidate_kern_tbia);
	TLB_COUNT_ATTACH(invalidate_kern_tbis);
	TLB_COUNT_ATTACH(invalidate_kern_imb);

	TLB_COUNT_ATTACH(invalidate_user_not_current);
	TLB_COUNT_ATTACH(invalidate_user_lazy_imb);
	TLB_COUNT_ATTACH(invalidate_user_tbiap);
	TLB_COUNT_ATTACH(invalidate_user_tbis);

	TLB_COUNT_ATTACH(shootdown_kernel);
	TLB_COUNT_ATTACH(shootdown_user);
	TLB_COUNT_ATTACH(shootdown_imb);
	TLB_COUNT_ATTACH(shootdown_kimb);
	TLB_COUNT_ATTACH(shootdown_overflow);

	TLB_COUNT_ATTACH(shootdown_all_user);
	TLB_COUNT_ATTACH(shootdown_all_user_imb);

	TLB_COUNT_ATTACH(shootdown_pv);
	TLB_COUNT_ATTACH(shootdown_pv_multi);

	TLB_COUNT_ATTACH(shootnow_over_notify);
	TLB_COUNT_ATTACH(shootnow_remote);

	TLB_COUNT_ATTACH(reason_remove_kernel);
	TLB_COUNT_ATTACH(reason_remove_user);
	TLB_COUNT_ATTACH(reason_remove_all_user);
	TLB_COUNT_ATTACH(reason_page_protect_read);
	TLB_COUNT_ATTACH(reason_page_protect_none);
	TLB_COUNT_ATTACH(reason_protect);
	TLB_COUNT_ATTACH(reason_enter_kernel);
	TLB_COUNT_ATTACH(reason_enter_user);
	TLB_COUNT_ATTACH(reason_kenter);
	TLB_COUNT_ATTACH(reason_enter_l2pt_delref);
	TLB_COUNT_ATTACH(reason_enter_l3pt_delref);
	TLB_COUNT_ATTACH(reason_kremove);
	TLB_COUNT_ATTACH(reason_clear_modify);
	TLB_COUNT_ATTACH(reason_clear_reference);

	TLB_COUNT_ATTACH(asn_reuse);
	TLB_COUNT_ATTACH(asn_newgen);
	TLB_COUNT_ATTACH(asn_assign);

	TLB_COUNT_ATTACH(activate_both_change);
	TLB_COUNT_ATTACH(activate_asn_change);
	TLB_COUNT_ATTACH(activate_ptbr_change);
	TLB_COUNT_ATTACH(activate_swpctx);
	TLB_COUNT_ATTACH(activate_skip_swpctx);
}

static inline void
pmap_tlb_context_init(struct pmap_tlb_context * const tlbctx, uintptr_t flags)
{
	/* Initialize the minimum number of fields. */
	tlbctx->t_addrdata[0] = 0;
	tlbctx->t_addrdata[1] = flags;
	tlbctx->t_pmap = NULL;
	LIST_INIT(&tlbctx->t_freeptq);
	LIST_INIT(&tlbctx->t_freepvq);
}

static void
pmap_tlb_shootdown_internal(pmap_t const pmap, vaddr_t const va,
    pt_entry_t const pte_bits, struct pmap_tlb_context * const tlbctx)
{
	KASSERT(pmap != NULL);
	KASSERT((va & PAGE_MASK) == 0);

	/*
	 * Figure out who needs to hear about this, and the scope
	 * of an all-entries invalidate.
	 */
	if (pmap == pmap_kernel()) {
		TLB_COUNT(shootdown_kernel);
		KASSERT(pte_bits & PG_ASM);
		TLB_CTX_SET_FLAG(tlbctx, TLB_CTX_F_ASM);

		/* Note if an I-stream sync is also needed. */
		if (pte_bits & PG_EXEC) {
			TLB_COUNT(shootdown_kimb);
			TLB_CTX_SET_FLAG(tlbctx, TLB_CTX_F_KIMB);
		}
	} else {
		TLB_COUNT(shootdown_user);
		KASSERT((pte_bits & PG_ASM) == 0);

		/* Note if an I-stream sync is also needed. */
		if (pte_bits & PG_EXEC) {
			TLB_COUNT(shootdown_imb);
			TLB_CTX_SET_FLAG(tlbctx, TLB_CTX_F_IMB);
		}
	}

	KASSERT(tlbctx->t_pmap == NULL || tlbctx->t_pmap == pmap);
	tlbctx->t_pmap = pmap;

	/*
	 * If we're already at the max, just tell each active CPU
	 * to nail everything.
	 */
	const uintptr_t count = TLB_CTX_COUNT(tlbctx);
	if (count > TLB_CTX_MAXVA) {
		return;
	}
	if (count == TLB_CTX_MAXVA) {
		TLB_COUNT(shootdown_overflow);
		TLB_CTX_SET_ALLVA(tlbctx);
		return;
	}

	TLB_CTX_SETVA(tlbctx, count, va);
	TLB_CTX_INC_COUNT(tlbctx);
}

static void
pmap_tlb_shootdown(pmap_t const pmap, vaddr_t const va,
    pt_entry_t const pte_bits, struct pmap_tlb_context * const tlbctx)
{
	KASSERT((TLB_CTX_FLAGS(tlbctx) & TLB_CTX_F_PV) == 0);
	pmap_tlb_shootdown_internal(pmap, va, pte_bits, tlbctx);
}

static void
pmap_tlb_shootdown_all_user(pmap_t const pmap, pt_entry_t const pte_bits,
    struct pmap_tlb_context * const tlbctx)
{
	KASSERT(pmap != pmap_kernel());

	TLB_COUNT(shootdown_all_user);

	/* Note if an I-stream sync is also needed. */
	if (pte_bits & PG_EXEC) {
		TLB_COUNT(shootdown_all_user_imb);
		TLB_CTX_SET_FLAG(tlbctx, TLB_CTX_F_IMB);
	}

	if (TLB_CTX_FLAGS(tlbctx) & TLB_CTX_F_PV) {
		if (tlbctx->t_pmap == NULL || tlbctx->t_pmap == pmap) {
			if (tlbctx->t_pmap == NULL) {
				pmap_reference(pmap);
				tlbctx->t_pmap = pmap;
			}
		} else {
			TLB_CTX_SET_FLAG(tlbctx, TLB_CTX_F_MULTI);
		}
	} else {
		KASSERT(tlbctx->t_pmap == NULL || tlbctx->t_pmap == pmap);
		tlbctx->t_pmap = pmap;
	}

	TLB_CTX_SET_ALLVA(tlbctx);
}

static void
pmap_tlb_shootdown_pv(pmap_t const pmap, vaddr_t const va,
    pt_entry_t const pte_bits, struct pmap_tlb_context * const tlbctx)
{

	KASSERT(TLB_CTX_FLAGS(tlbctx) & TLB_CTX_F_PV);

	TLB_COUNT(shootdown_pv);

	if (tlbctx->t_pmap == NULL || tlbctx->t_pmap == pmap) {
		if (tlbctx->t_pmap == NULL) {
			pmap_reference(pmap);
			tlbctx->t_pmap = pmap;
		}
		pmap_tlb_shootdown_internal(pmap, va, pte_bits, tlbctx);
	} else {
		TLB_COUNT(shootdown_pv_multi);
		uintptr_t flags = TLB_CTX_F_MULTI;
		if (pmap == pmap_kernel()) {
			KASSERT(pte_bits & PG_ASM);
			flags |= TLB_CTX_F_ASM;
		} else {
			KASSERT((pte_bits & PG_ASM) == 0);
		}

		/*
		 * No need to distinguish between kernel and user IMB
		 * here; see pmap_tlb_invalidate_multi().
		 */
		if (pte_bits & PG_EXEC) {
			flags |= TLB_CTX_F_IMB;
		}
		TLB_CTX_SET_ALLVA(tlbctx);
		TLB_CTX_SET_FLAG(tlbctx, flags);
	}
}

static void
pmap_tlb_invalidate_multi(const struct pmap_tlb_context * const tlbctx)
{
	if (TLB_CTX_FLAGS(tlbctx) & TLB_CTX_F_ASM) {
		TLB_COUNT(invalidate_multi_tbia);
		ALPHA_TBIA();
	} else {
		TLB_COUNT(invalidate_multi_tbiap);
		ALPHA_TBIAP();
	}
	if (TLB_CTX_FLAGS(tlbctx) & (TLB_CTX_F_IMB | TLB_CTX_F_KIMB)) {
		TLB_COUNT(invalidate_multi_imb);
		alpha_pal_imb();
	}
}

static void
pmap_tlb_invalidate_kernel(const struct pmap_tlb_context * const tlbctx)
{
	const uintptr_t count = TLB_CTX_COUNT(tlbctx);

	if (count == TLB_CTX_ALLVA) {
		TLB_COUNT(invalidate_kern_tbia);
		ALPHA_TBIA();
	} else {
		TLB_COUNT(invalidate_kern_tbis);
		for (uintptr_t i = 0; i < count; i++) {
			ALPHA_TBIS(TLB_CTX_VA(tlbctx, i));
		}
	}
	if (TLB_CTX_FLAGS(tlbctx) & TLB_CTX_F_KIMB) {
		TLB_COUNT(invalidate_kern_imb);
		alpha_pal_imb();
	}
}

static void
pmap_tlb_invalidate(const struct pmap_tlb_context * const tlbctx,
    const struct cpu_info * const ci)
{
	const uintptr_t count = TLB_CTX_COUNT(tlbctx);

	if (TLB_CTX_FLAGS(tlbctx) & TLB_CTX_F_MULTI) {
		pmap_tlb_invalidate_multi(tlbctx);
		return;
	}

	if (TLB_CTX_FLAGS(tlbctx) & TLB_CTX_F_ASM) {
		pmap_tlb_invalidate_kernel(tlbctx);
		return;
	}

	KASSERT(kpreempt_disabled());

	pmap_t const pmap = tlbctx->t_pmap;
	KASSERT(pmap != NULL);

	if (__predict_false(pmap != ci->ci_pmap)) {
		TLB_COUNT(invalidate_user_not_current);

		/*
		 * For CPUs that don't implement ASNs, the SWPCTX call
		 * does all of the TLB invalidation work for us.
		 */
		if (__predict_false(pmap_max_asn == 0)) {
			return;
		}

		const u_long cpu_mask = 1UL << ci->ci_cpuid;

		/*
		 * We cannot directly invalidate the TLB in this case,
		 * so force allocation of a new ASN when the pmap becomes
		 * active again.
		 */
		pmap->pm_percpu[ci->ci_cpuid].pmc_asngen = PMAP_ASNGEN_INVALID;
		atomic_and_ulong(&pmap->pm_cpus, ~cpu_mask);

		/*
		 * This isn't strictly necessary; when we allocate a
		 * new ASN, we're going to clear this bit and skip
		 * syncing the I-stream.  But we will keep this bit
		 * of accounting for internal consistency.
		 */
		if (TLB_CTX_FLAGS(tlbctx) & TLB_CTX_F_IMB) {
			pmap->pm_percpu[ci->ci_cpuid].pmc_needisync = 1;
		}
		return;
	}

	if (TLB_CTX_FLAGS(tlbctx) & TLB_CTX_F_IMB) {
		TLB_COUNT(invalidate_user_lazy_imb);
		pmap->pm_percpu[ci->ci_cpuid].pmc_needisync = 1;
	}

	if (count == TLB_CTX_ALLVA) {
		/*
		 * Another option here for CPUs that implement ASNs is
		 * to allocate a new ASN and do a SWPCTX.  That's almost
		 * certainly faster than a TBIAP, but would require us
		 * to synchronize against IPIs in pmap_activate().
		 */
		TLB_COUNT(invalidate_user_tbiap);
		KASSERT((TLB_CTX_FLAGS(tlbctx) & TLB_CTX_F_ASM) == 0);
		ALPHA_TBIAP();
	} else {
		TLB_COUNT(invalidate_user_tbis);
		for (uintptr_t i = 0; i < count; i++) {
			ALPHA_TBIS(TLB_CTX_VA(tlbctx, i));
		}
	}
}

static void
pmap_tlb_shootnow(const struct pmap_tlb_context * const tlbctx)
{

	if (TLB_CTX_COUNT(tlbctx) == 0) {
		/* No work to do. */
		return;
	}

	/*
	 * Acquire the shootdown mutex.  This will also block IPL_VM
	 * interrupts and disable preemption.  It is critically important
	 * that IPIs not be blocked in this routine.
	 */
	KASSERT(alpha_pal_rdps() < ALPHA_PSL_IPL_CLOCK);
	mutex_spin_enter(&tlb_lock);
	tlb_evcnt.ev_count++;

	const struct cpu_info *ci = curcpu();
	const u_long this_cpu = 1UL << ci->ci_cpuid;
	u_long active_cpus;
	bool activation_locked, activation_lock_tried;

	/*
	 * Figure out who to notify.  If it's for the kernel or
	 * multiple address spaces, we notify everybody.  If
	 * it's a single user pmap, then we try to acquire the
	 * activation lock so we can get an accurate accounting
	 * of who needs to be notified.  If we can't acquire
	 * the activation lock, then just notify everyone and
	 * let them sort it out when they process the IPI.
	 */
	if (TLB_CTX_FLAGS(tlbctx) & (TLB_CTX_F_ASM | TLB_CTX_F_MULTI)) {
		active_cpus = pmap_all_cpus();
		activation_locked = false;
		activation_lock_tried = false;
	} else {
		KASSERT(tlbctx->t_pmap != NULL);
		activation_locked = PMAP_ACT_TRYLOCK(tlbctx->t_pmap);
		if (__predict_true(activation_locked)) {
			active_cpus = tlbctx->t_pmap->pm_cpus;
		} else {
			TLB_COUNT(shootnow_over_notify);
			active_cpus = pmap_all_cpus();
		}
		activation_lock_tried = true;
	}

#if defined(MULTIPROCESSOR)
	/*
	 * If there are remote CPUs that need to do work, get them
	 * started now.
	 */
	const u_long remote_cpus = active_cpus & ~this_cpu;
	KASSERT(tlb_context == NULL);
	if (remote_cpus) {
		TLB_COUNT(shootnow_remote);
		tlb_context = tlbctx;
		tlb_pending = remote_cpus;
		alpha_multicast_ipi(remote_cpus, ALPHA_IPI_SHOOTDOWN);
	}
#endif /* MULTIPROCESSOR */

	/*
	 * Now that the remotes have been notified, release the
	 * activation lock.
	 */
	if (activation_lock_tried) {
		if (activation_locked) {
			KASSERT(tlbctx->t_pmap != NULL);
			PMAP_ACT_UNLOCK(tlbctx->t_pmap);
		}
		/*
		 * When we tried to acquire the activation lock, we
		 * raised IPL to IPL_SCHED (even if we ultimately
		 * failed to acquire the lock), which blocks out IPIs.
		 * Force our IPL back down to IPL_VM so that we can
		 * receive IPIs.
		 */
		alpha_pal_swpipl(IPL_VM);
	}

	/*
	 * Do any work that we might need to do.  We don't need to
	 * synchronize with activation here because we know that
	 * for the current CPU, activation status will not change.
	 */
	if (active_cpus & this_cpu) {
		pmap_tlb_invalidate(tlbctx, ci);
	}

#if defined(MULTIPROCESSOR)
	/* Wait for remote CPUs to finish. */
	if (remote_cpus) {
		int backoff = SPINLOCK_BACKOFF_MIN;
		u_int spins = 0;

		while (atomic_load_acquire(&tlb_context) != NULL) {
			SPINLOCK_BACKOFF(backoff);
			if (spins++ > 0x0fffffff) {
				printf("TLB LOCAL MASK  = 0x%016lx\n",
				    this_cpu);
				printf("TLB REMOTE MASK = 0x%016lx\n",
				    remote_cpus);
				printf("TLB REMOTE PENDING = 0x%016lx\n",
				    tlb_pending);
				printf("TLB CONTEXT = %p\n", tlb_context);
				printf("TLB LOCAL IPL = %lu\n",
				    alpha_pal_rdps());
				panic("pmap_tlb_shootnow");
			}
		}
	}
	KASSERT(tlb_context == NULL);
#endif /* MULTIPROCESSOR */

	mutex_spin_exit(&tlb_lock);

	if (__predict_false(TLB_CTX_FLAGS(tlbctx) & TLB_CTX_F_PV)) {
		/*
		 * P->V TLB operations may operate on multiple pmaps.
		 * The shootdown takes a reference on the first pmap it
		 * encounters, in order to prevent it from disappearing,
		 * in the hope that we end up with a single-pmap P->V
		 * operation (instrumentation shows this is not rare).
		 *
		 * Once this shootdown is finished globally, we need to
		 * release this extra reference.
		 */
		KASSERT(tlbctx->t_pmap != NULL);
		pmap_destroy(tlbctx->t_pmap);
	}
}

#if defined(MULTIPROCESSOR)
void
pmap_tlb_shootdown_ipi(struct cpu_info * const ci,

    struct trapframe * const tf __unused)
{
	KASSERT(tlb_context != NULL);
	pmap_tlb_invalidate(tlb_context, ci);
	if (atomic_and_ulong_nv(&tlb_pending, ~(1UL << ci->ci_cpuid)) == 0) {
		atomic_store_release(&tlb_context, NULL);
	}
}
#endif /* MULTIPROCESSOR */

static inline void
pmap_tlb_context_drain(struct pmap_tlb_context * const tlbctx)
{
	if (! LIST_EMPTY(&tlbctx->t_freeptq)) {
		pmap_pagelist_free(&tlbctx->t_freeptq);
	}
	if (! LIST_EMPTY(&tlbctx->t_freepvq)) {
		pmap_pvlist_free(&tlbctx->t_freepvq);
	}
}

/*
 * ASN management functions.
 */
static u_int	pmap_asn_alloc(pmap_t, struct cpu_info *);

/*
 * Misc. functions.
 */
static struct vm_page *pmap_physpage_alloc(int);
static void	pmap_physpage_free(paddr_t);
static int	pmap_physpage_addref(void *);
static int	pmap_physpage_delref(void *);

static bool	vtophys_internal(vaddr_t, paddr_t *p);

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
	l1pte_ = pmap_l1pte(kernel_lev1map, va);			\
	if (pmap_pte_v(l1pte_) == 0) {					\
		printf("kernel level 1 PTE not valid, va 0x%lx "	\
		    "(line %d)\n", (va), __LINE__);			\
		panic("PMAP_KERNEL_PTE");				\
	}								\
	l2pte_ = pmap_l2pte(kernel_lev1map, va, l1pte_);		\
	if (pmap_pte_v(l2pte_) == 0) {					\
		printf("kernel level 2 PTE not valid, va 0x%lx "	\
		    "(line %d)\n", (va), __LINE__);			\
		panic("PMAP_KERNEL_PTE");				\
	}								\
	pmap_l3pte(kernel_lev1map, va, l2pte_);				\
})
#else
#define	PMAP_KERNEL_PTE(va)	(&VPT[VPT_INDEX((va))])
#endif

/*
 * PMAP_STAT_{INCR,DECR}:
 *
 *	Increment or decrement a pmap statistic.
 */
#define	PMAP_STAT_INCR(s, v)	atomic_add_long((unsigned long *)(&(s)), (v))
#define	PMAP_STAT_DECR(s, v)	atomic_add_long((unsigned long *)(&(s)), -(v))

/*
 * pmap_init_cpu:
 *
 *	Initilize pmap data in the cpu_info.
 */
void
pmap_init_cpu(struct cpu_info * const ci)
{
	pmap_t const pmap = pmap_kernel();

	/* All CPUs start out using the kernel pmap. */
	atomic_or_ulong(&pmap->pm_cpus, 1UL << ci->ci_cpuid);
	pmap_reference(pmap);
	ci->ci_pmap = pmap;

	/* Initialize ASN allocation logic. */
	ci->ci_next_asn = PMAP_ASN_FIRST_USER;
	ci->ci_asn_gen = PMAP_ASNGEN_INITIAL;
}

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

	lev3mapsize = roundup(lev3mapsize, NPTEPG);

	/*
	 * Initialize `FYI' variables.  Note we're relying on
	 * the fact that BSEARCH sorts the vm_physmem[] array
	 * for us.
	 */
	avail_start = ptoa(uvm_physseg_get_avail_start(uvm_physseg_get_first()));
	avail_end = ptoa(uvm_physseg_get_avail_end(uvm_physseg_get_last()));
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
	pool_cache_bootstrap(&pmap_pmap_cache, PMAP_SIZEOF(pmap_ncpuids),
	    COHERENCY_UNIT, 0, 0, "pmap", NULL, IPL_NONE, NULL, NULL, NULL);
	pool_cache_bootstrap(&pmap_l1pt_cache, PAGE_SIZE, 0, 0, 0, "pmapl1pt",
	    &pmap_l1pt_allocator, IPL_NONE, pmap_l1pt_ctor, NULL, NULL);
	pool_cache_bootstrap(&pmap_pv_cache, sizeof(struct pv_entry), 0, 0,
	    PR_LARGECACHE, "pmappv", &pmap_pv_page_allocator, IPL_NONE, NULL,
	    NULL, NULL);

	TAILQ_INIT(&pmap_all_pmaps);

	/* Initialize the ASN logic.  See also pmap_init_cpu(). */
	pmap_max_asn = maxasn;

	/*
	 * Initialize the locks.
	 */
	rw_init(&pmap_main_lock);
	mutex_init(&pmap_all_pmaps_lock, MUTEX_DEFAULT, IPL_NONE);
	for (i = 0; i < __arraycount(pmap_pvh_locks); i++) {
		mutex_init(&pmap_pvh_locks[i].lock, MUTEX_DEFAULT, IPL_NONE);
	}
	for (i = 0; i < __arraycount(pmap_pvh_locks); i++) {
		mutex_init(&pmap_pmap_locks[i].locks.lock,
		    MUTEX_DEFAULT, IPL_NONE);
		mutex_init(&pmap_pmap_locks[i].locks.activation_lock,
		    MUTEX_SPIN, IPL_SCHED);
	}
	
	/*
	 * This must block any interrupt from which a TLB shootdown
	 * could be issued, but must NOT block IPIs.
	 */
	mutex_init(&tlb_lock, MUTEX_SPIN, IPL_VM);

	/*
	 * Initialize kernel pmap.  Note that all kernel mappings
	 * have PG_ASM set, so the ASN doesn't really matter for
	 * the kernel pmap.  Also, since the kernel pmap always
	 * references kernel_lev1map, it always has an invalid ASN
	 * generation.
	 */
	memset(pmap_kernel(), 0, sizeof(struct pmap));
	LIST_INIT(&pmap_kernel()->pm_ptpages);
	LIST_INIT(&pmap_kernel()->pm_pvents);
	atomic_store_relaxed(&pmap_kernel()->pm_count, 1);
	/* Kernel pmap does not have per-CPU info. */
	TAILQ_INSERT_TAIL(&pmap_all_pmaps, pmap_kernel(), pm_list);

	/*
	 * Set up lwp0's PCB such that the ptbr points to the right place
	 * and has the kernel pmap's (really unused) ASN.
	 */
	pcb = lwp_getpcb(&lwp0);
	pcb->pcb_hw.apcb_ptbr =
	    ALPHA_K0SEG_TO_PHYS((vaddr_t)kernel_lev1map) >> PGSHIFT;
	pcb->pcb_hw.apcb_asn = PMAP_ASN_KERNEL;

	struct cpu_info * const ci = curcpu();
	pmap_init_cpu(ci);
}

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
	int npgs;
	vaddr_t va;
	paddr_t pa;

	uvm_physseg_t bank;

	size = round_page(size);
	npgs = atop(size);

#if 0
	printf("PSM: size 0x%lx (npgs 0x%x)\n", size, npgs);
#endif

	for (bank = uvm_physseg_get_first();
	     uvm_physseg_valid_p(bank);
	     bank = uvm_physseg_get_next(bank)) {
		if (uvm.page_init_done == true)
			panic("pmap_steal_memory: called _after_ bootstrap");

#if 0
		printf("     bank %d: avail_start 0x%"PRIxPADDR", start 0x%"PRIxPADDR", "
		    "avail_end 0x%"PRIxPADDR"\n", bank, uvm_physseg_get_avail_start(bank),
		    uvm_physseg_get_start(bank), uvm_physseg_get_avail_end(bank));
#endif

		if (uvm_physseg_get_avail_start(bank) != uvm_physseg_get_start(bank) ||
		    uvm_physseg_get_avail_start(bank) >= uvm_physseg_get_avail_end(bank))
			continue;

#if 0
		printf("             avail_end - avail_start = 0x%"PRIxPADDR"\n",
		    uvm_physseg_get_avail_end(bank) - uvm_physseg_get_avail_start(bank));
#endif

		if (uvm_physseg_get_avail_end(bank) - uvm_physseg_get_avail_start(bank)
		    < npgs)
			continue;

		/*
		 * There are enough pages here; steal them!
		 */
		pa = ptoa(uvm_physseg_get_start(bank));
		uvm_physseg_unplug(atop(pa), npgs);

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

	/* Initialize TLB handling. */
	pmap_tlb_init();

	/* Instrument pmap_growkernel(). */
	evcnt_attach_dynamic_nozero(&pmap_growkernel_evcnt, EVCNT_TYPE_MISC,
	    NULL, "pmap", "growkernel");

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
	for (uvm_physseg_t bank = uvm_physseg_get_first();
	    uvm_physseg_valid_p(bank);
	    bank = uvm_physseg_get_next(bank)) {
		printf("bank %d\n", bank);
		printf("\tstart = 0x%lx\n", ptoa(uvm_physseg_get_start(bank)));
		printf("\tend = 0x%lx\n", ptoa(uvm_physseg_get_end(bank)));
		printf("\tavail_start = 0x%lx\n",
		    ptoa(uvm_physseg_get_avail_start(bank)));
		printf("\tavail_end = 0x%lx\n",
		    ptoa(uvm_physseg_get_avail_end(bank)));
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
	pt_entry_t *lev1map;
	int i;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_create()\n");
#endif

	pmap = pool_cache_get(&pmap_pmap_cache, PR_WAITOK);
	memset(pmap, 0, sizeof(*pmap));
	LIST_INIT(&pmap->pm_ptpages);
	LIST_INIT(&pmap->pm_pvents);

	atomic_store_relaxed(&pmap->pm_count, 1);

 try_again:
	rw_enter(&pmap_growkernel_lock, RW_READER);

	lev1map = pool_cache_get(&pmap_l1pt_cache, PR_NOWAIT);
	if (__predict_false(lev1map == NULL)) {
		rw_exit(&pmap_growkernel_lock);
		(void) kpause("pmap_create", false, hz >> 2, NULL);
		goto try_again;
	}

	/*
	 * There are only kernel mappings at this point; give the pmap
	 * the kernel ASN.  This will be initialized to correct values
	 * when the pmap is activated.
	 *
	 * We stash a pointer to the pmap's lev1map in each CPU's
	 * private data.  It remains constant for the life of the
	 * pmap, and gives us more room in the shared pmap structure.
	 */
	for (i = 0; i < pmap_ncpuids; i++) {
		pmap->pm_percpu[i].pmc_asn = PMAP_ASN_KERNEL;
		pmap->pm_percpu[i].pmc_asngen = PMAP_ASNGEN_INVALID;
		pmap->pm_percpu[i].pmc_lev1map = lev1map;
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

	PMAP_MP(membar_release());
	KASSERT(atomic_load_relaxed(&pmap->pm_count) > 0);
	if (atomic_dec_uint_nv(&pmap->pm_count) > 0)
		return;
	PMAP_MP(membar_acquire());

	pt_entry_t *lev1map = pmap_lev1map(pmap);

	rw_enter(&pmap_growkernel_lock, RW_READER);

	/*
	 * Remove it from the global list of all pmaps.
	 */
	mutex_enter(&pmap_all_pmaps_lock);
	TAILQ_REMOVE(&pmap_all_pmaps, pmap, pm_list);
	mutex_exit(&pmap_all_pmaps_lock);

	pool_cache_put(&pmap_l1pt_cache, lev1map);
#ifdef DIAGNOSTIC
	int i;
	for (i = 0; i < pmap_ncpuids; i++) {
		pmap->pm_percpu[i].pmc_lev1map = (pt_entry_t *)0xdeadbeefUL;
	}
#endif /* DIAGNOSTIC */

	rw_exit(&pmap_growkernel_lock);

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
	unsigned int newcount __diagused;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_reference(%p)\n", pmap);
#endif

	newcount = atomic_inc_uint_nv(&pmap->pm_count);
	KASSERT(newcount != 0);
}

/*
 * pmap_remove:			[ INTERFACE ]
 *
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the page size.
 */
static void
pmap_remove_internal(pmap_t pmap, vaddr_t sva, vaddr_t eva,
    struct pmap_tlb_context * const tlbctx)
{
	pt_entry_t *l1pte, *l2pte, *l3pte;
	pt_entry_t *saved_l2pte, *saved_l3pte;
	vaddr_t l1eva, l2eva, l3vptva;
	pt_entry_t pte_bits;

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
				pte_bits = pmap_remove_mapping(pmap, sva,
				    l3pte, true, NULL, tlbctx);
				pmap_tlb_shootdown(pmap, sva, pte_bits,
				    tlbctx);
			}
			sva += PAGE_SIZE;
		}

		PMAP_MAP_TO_HEAD_UNLOCK();
		PMAP_UNLOCK(pmap);
		pmap_tlb_shootnow(tlbctx);
		/* kernel PT pages are never freed. */
		KASSERT(LIST_EMPTY(&tlbctx->t_freeptq));
		/* ...but we might have freed PV entries. */
		pmap_tlb_context_drain(tlbctx);
		TLB_COUNT(reason_remove_kernel);

		return;
	}

	pt_entry_t * const lev1map = pmap_lev1map(pmap);

	KASSERT(sva < VM_MAXUSER_ADDRESS);
	KASSERT(eva <= VM_MAXUSER_ADDRESS);
	KASSERT(lev1map != kernel_lev1map);

	PMAP_MAP_TO_HEAD_LOCK();
	PMAP_LOCK(pmap);

	l1pte = pmap_l1pte(lev1map, sva);

	for (; sva < eva; sva = l1eva, l1pte++) {
		l1eva = alpha_trunc_l1seg(sva) + ALPHA_L1SEG_SIZE;
		if (pmap_pte_v(l1pte)) {
			saved_l2pte = l2pte = pmap_l2pte(lev1map, sva, l1pte);

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
					    pmap_l3pte(lev1map, sva, l2pte);

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
					l3vptva = sva;

					for (; sva < l2eva && sva < eva;
					     sva += PAGE_SIZE, l3pte++) {
						if (!pmap_pte_v(l3pte)) {
							continue;
						}
						pte_bits =
						    pmap_remove_mapping(
							pmap, sva,
							l3pte, true,
							NULL, tlbctx);
						pmap_tlb_shootdown(pmap,
						    sva, pte_bits, tlbctx);
					}

					/*
					 * Remove the reference to the L3
					 * table that we added above.  This
					 * may free the L3 table.
					 */
					pmap_l3pt_delref(pmap, l3vptva,
					    saved_l3pte, tlbctx);
				}
			}

			/*
			 * Remove the reference to the L2 table that we
			 * added above.  This may free the L2 table.
			 */
			pmap_l2pt_delref(pmap, l1pte, saved_l2pte, tlbctx);
		}
	}

	PMAP_MAP_TO_HEAD_UNLOCK();
	PMAP_UNLOCK(pmap);
	pmap_tlb_shootnow(tlbctx);
	pmap_tlb_context_drain(tlbctx);
	TLB_COUNT(reason_remove_user);
}

void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	struct pmap_tlb_context tlbctx;

	pmap_tlb_context_init(&tlbctx, 0);
	pmap_remove_internal(pmap, sva, eva, &tlbctx);
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
	struct pmap_tlb_context tlbctx;
	struct vm_page *pg;
	pv_entry_t pv;

	KASSERT(pmap != pmap_kernel());

	/*
	 * This process is pretty simple:
	 *
	 * ==> (1) Zero out the user-space portion of the lev1map.
	 *
	 * ==> (2) Copy the PT page list to the tlbctx and re-init.
	 *
	 * ==> (3) Walk the PV entry list and remove each entry.
	 *
	 * ==> (4) Zero the wired and resident count.
	 *
	 * Once we've done that, we just need to free everything
	 * back to the system.
	 */

	pmap_tlb_context_init(&tlbctx, 0);

	PMAP_MAP_TO_HEAD_LOCK();
	PMAP_LOCK(pmap);

	/* Step 1 */
	pt_entry_t * const lev1map = pmap_lev1map(pmap);
	memset(lev1map, 0,
	       l1pte_index(VM_MAXUSER_ADDRESS) * sizeof(pt_entry_t));

	/* Step 2 */
	LIST_MOVE(&pmap->pm_ptpages, &tlbctx.t_freeptq, pageq.list);

	/* Fix up the reference count on the lev1map page. */
	pg = PHYS_TO_VM_PAGE(ALPHA_K0SEG_TO_PHYS((vaddr_t)lev1map));
	PHYSPAGE_REFCNT_SET(pg, 0);

	/* Step 3 */
	while ((pv = LIST_FIRST(&pmap->pm_pvents)) != NULL) {
		KASSERT(pv->pv_pmap == pmap);
		pmap_pv_remove(pmap, PHYS_TO_VM_PAGE(pmap_pte_pa(pv->pv_pte)),
		    pv->pv_va, true, NULL, &tlbctx);
	}

	/* Step 4 */
	atomic_store_relaxed(&pmap->pm_stats.wired_count, 0);
	atomic_store_relaxed(&pmap->pm_stats.resident_count, 0);

	pmap_tlb_shootdown_all_user(pmap, PG_EXEC, &tlbctx);

	PMAP_UNLOCK(pmap);
	PMAP_MAP_TO_HEAD_UNLOCK();

	pmap_tlb_shootnow(&tlbctx);
	pmap_tlb_context_drain(&tlbctx);
	TLB_COUNT(reason_remove_all_user);

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
	pv_entry_t pv, nextpv;
	pt_entry_t opte;
	kmutex_t *lock;
	struct pmap_tlb_context tlbctx;

#ifdef DEBUG
	if ((pmapdebug & (PDB_FOLLOW|PDB_PROTECT)) ||
	    (prot == VM_PROT_NONE && (pmapdebug & PDB_REMOVE)))
		printf("pmap_page_protect(%p, %x)\n", pg, prot);
#endif

	pmap_tlb_context_init(&tlbctx, TLB_CTX_F_PV);

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
		for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
			PMAP_LOCK(pv->pv_pmap);
			opte = atomic_load_relaxed(pv->pv_pte);
			if (opte & (PG_KWE | PG_UWE)) {
				atomic_store_relaxed(pv->pv_pte,
				    opte & ~(PG_KWE | PG_UWE));
				pmap_tlb_shootdown_pv(pv->pv_pmap, pv->pv_va,
				    opte, &tlbctx);
			}
			PMAP_UNLOCK(pv->pv_pmap);
		}
		mutex_exit(lock);
		PMAP_HEAD_TO_MAP_UNLOCK();
		pmap_tlb_shootnow(&tlbctx);
		TLB_COUNT(reason_page_protect_read);
		return;

	/* remove_all */
	default:
		break;
	}

	PMAP_HEAD_TO_MAP_LOCK();
	lock = pmap_pvh_lock(pg);
	mutex_enter(lock);
	for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = nextpv) {
		pt_entry_t pte_bits;
		pmap_t pmap;
		vaddr_t va;

		nextpv = pv->pv_next;

		PMAP_LOCK(pv->pv_pmap);
		pmap = pv->pv_pmap;
		va = pv->pv_va;
		pte_bits = pmap_remove_mapping(pmap, va, pv->pv_pte,
		    false, NULL, &tlbctx);
		pmap_tlb_shootdown_pv(pmap, va, pte_bits, &tlbctx);
		PMAP_UNLOCK(pv->pv_pmap);
	}
	mutex_exit(lock);
	PMAP_HEAD_TO_MAP_UNLOCK();
	pmap_tlb_shootnow(&tlbctx);
	pmap_tlb_context_drain(&tlbctx);
	TLB_COUNT(reason_page_protect_none);
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
	pt_entry_t *l1pte, *l2pte, *l3pte, opte;
	vaddr_t l1eva, l2eva;
	struct pmap_tlb_context tlbctx;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT))
		printf("pmap_protect(%p, %lx, %lx, %x)\n",
		    pmap, sva, eva, prot);
#endif

	pmap_tlb_context_init(&tlbctx, 0);

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove_internal(pmap, sva, eva, &tlbctx);
		return;
	}

	const pt_entry_t bits = pte_prot(pmap, prot);
	pt_entry_t * const lev1map = pmap_lev1map(pmap);

	PMAP_LOCK(pmap);

	l1pte = pmap_l1pte(lev1map, sva);
	for (; sva < eva; sva = l1eva, l1pte++) {
		l1eva = alpha_trunc_l1seg(sva) + ALPHA_L1SEG_SIZE;
		if (pmap_pte_v(l1pte)) {
			l2pte = pmap_l2pte(lev1map, sva, l1pte);
			for (; sva < l1eva && sva < eva; sva = l2eva, l2pte++) {
				l2eva =
				    alpha_trunc_l2seg(sva) + ALPHA_L2SEG_SIZE;
				if (pmap_pte_v(l2pte)) {
					l3pte = pmap_l3pte(lev1map, sva, l2pte);
					for (; sva < l2eva && sva < eva;
					     sva += PAGE_SIZE, l3pte++) {
						if (pmap_pte_v(l3pte) &&
						    pmap_pte_prot_chg(l3pte,
								      bits)) {
							opte = atomic_load_relaxed(l3pte);
							pmap_pte_set_prot(l3pte,
							   bits);
							pmap_tlb_shootdown(pmap,
							    sva, opte, &tlbctx);
						}
					}
				}
			}
		}
	}

	PMAP_UNLOCK(pmap);
	pmap_tlb_shootnow(&tlbctx);
	TLB_COUNT(reason_protect);
}

/*
 * pmap_enter_tlb_shootdown:
 *
 *	Carry out a TLB shootdown on behalf of a pmap_enter()
 *	or a pmap_kenter_pa().  This is factored out separately
 *	because we expect it to be not a common case.
 */
static void __noinline
pmap_enter_tlb_shootdown(pmap_t const pmap, vaddr_t const va,
    pt_entry_t const pte_bits, bool locked)
{
	struct pmap_tlb_context tlbctx;

	pmap_tlb_context_init(&tlbctx, 0);
	pmap_tlb_shootdown(pmap, va, pte_bits, &tlbctx);
	if (locked) {
		PMAP_UNLOCK(pmap);
	}
	pmap_tlb_shootnow(&tlbctx);
}

/*
 * pmap_enter_l2pt_delref:
 *
 *	Release a reference on an L2 PT page for pmap_enter().
 *	This is factored out separately because we expect it
 *	to be a rare case.
 */
static void __noinline
pmap_enter_l2pt_delref(pmap_t const pmap, pt_entry_t * const l1pte,
    pt_entry_t * const l2pte)
{
	struct pmap_tlb_context tlbctx;

	/*
	 * PALcode may have tried to service a TLB miss with
	 * this L2 PTE, so we need to make sure we don't actually
	 * free the PT page until we've shot down any TLB entries
	 * for this VPT index.
	 */

	pmap_tlb_context_init(&tlbctx, 0);
	pmap_l2pt_delref(pmap, l1pte, l2pte, &tlbctx);
	PMAP_UNLOCK(pmap);
	pmap_tlb_shootnow(&tlbctx);
	pmap_tlb_context_drain(&tlbctx);
	TLB_COUNT(reason_enter_l2pt_delref);
}

/*
 * pmap_enter_l3pt_delref:
 *
 *	Release a reference on an L3 PT page for pmap_enter().
 *	This is factored out separately because we expect it
 *	to be a rare case.
 */
static void __noinline
pmap_enter_l3pt_delref(pmap_t const pmap, vaddr_t const va,
    pt_entry_t * const pte)
{
	struct pmap_tlb_context tlbctx;

	/*
	 * PALcode may have tried to service a TLB miss with
	 * this PTE, so we need to make sure we don't actually
	 * free the PT page until we've shot down any TLB entries
	 * for this VPT index.
	 */

	pmap_tlb_context_init(&tlbctx, 0);
	pmap_l3pt_delref(pmap, va, pte, &tlbctx);
	PMAP_UNLOCK(pmap);
	pmap_tlb_shootnow(&tlbctx);
	pmap_tlb_context_drain(&tlbctx);
	TLB_COUNT(reason_enter_l3pt_delref);
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
	pt_entry_t *pte, npte, opte;
	pv_entry_t opv = NULL;
	paddr_t opa;
	bool tflush = false;
	int error = 0;
	kmutex_t *lock;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_enter(%p, %lx, %lx, %x, %x)\n",
		       pmap, va, pa, prot, flags);
#endif
	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	const bool wired = (flags & PMAP_WIRED) != 0;

	PMAP_MAP_TO_HEAD_LOCK();
	PMAP_LOCK(pmap);

	if (pmap == pmap_kernel()) {
		KASSERT(va >= VM_MIN_KERNEL_ADDRESS);
		pte = PMAP_KERNEL_PTE(va);
	} else {
		pt_entry_t *l1pte, *l2pte;
		pt_entry_t * const lev1map = pmap_lev1map(pmap);

		KASSERT(va < VM_MAXUSER_ADDRESS);
		KASSERT(lev1map != kernel_lev1map);

		/*
		 * Check to see if the level 1 PTE is valid, and
		 * allocate a new level 2 page table page if it's not.
		 * A reference will be added to the level 2 table when
		 * the level 3 table is created.
		 */
		l1pte = pmap_l1pte(lev1map, va);
		if (pmap_pte_v(l1pte) == 0) {
			pmap_physpage_addref(l1pte);
			error = pmap_ptpage_alloc(pmap, l1pte, PGU_L2PT);
			if (error) {
				pmap_l1pt_delref(pmap, l1pte);
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
		l2pte = pmap_l2pte(lev1map, va, l1pte);
		if (pmap_pte_v(l2pte) == 0) {
			pmap_physpage_addref(l2pte);
			error = pmap_ptpage_alloc(pmap, l2pte, PGU_L3PT);
			if (error) {
				/* unlocks pmap */
				pmap_enter_l2pt_delref(pmap, l1pte, l2pte);
				if (flags & PMAP_CANFAIL) {
					PMAP_LOCK(pmap);
					goto out;
				}
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
		pte = pmap_l3pte(lev1map, va, l2pte);
	}

	/* Remember all of the old PTE; used for TBI check later. */
	opte = atomic_load_relaxed(pte);

	/*
	 * Check to see if the old mapping is valid.  If not, validate the
	 * new one immediately.
	 */
	if ((opte & PG_V) == 0) {
		/* No TLB invalidations needed for new mappings. */

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
			/* Adjust the wiring count. */
			if (wired)
				PMAP_STAT_INCR(pmap->pm_stats.wired_count, 1);
			else
				PMAP_STAT_DECR(pmap->pm_stats.wired_count, 1);
		}

		/* Set the PTE. */
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
	/* Already have the bits from opte above. */
	(void) pmap_remove_mapping(pmap, va, pte, true, &opv, NULL);

 validate_enterpv:
	/* Enter the mapping into the pv_table if appropriate. */
	if (pg != NULL) {
		error = pmap_pv_enter(pmap, pg, va, pte, true, opv);
		if (error) {
			/* This can only fail if opv == NULL */
			KASSERT(opv == NULL);

			/* unlocks pmap */
			pmap_enter_l3pt_delref(pmap, va, pte);
			if (flags & PMAP_CANFAIL) {
				PMAP_LOCK(pmap);
				goto out;
			}
			panic("pmap_enter: unable to enter mapping in PV "
			    "table");
		}
		opv = NULL;
	}

	/* Increment counters. */
	PMAP_STAT_INCR(pmap->pm_stats.resident_count, 1);
	if (wired)
		PMAP_STAT_INCR(pmap->pm_stats.wired_count, 1);

 validate:
	/* Build the new PTE. */
	npte = ((pa >> PGSHIFT) << PG_SHIFT) | pte_prot(pmap, prot) | PG_V;
	if (pg != NULL) {
		struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
		uintptr_t attrs = 0;

		KASSERT(((flags & VM_PROT_ALL) & ~prot) == 0);

		if (flags & VM_PROT_WRITE)
			attrs |= (PGA_REFERENCED|PGA_MODIFIED);
		else if (flags & VM_PROT_ALL)
			attrs |= PGA_REFERENCED;

		lock = pmap_pvh_lock(pg);
		mutex_enter(lock);
		attrs = (md->pvh_listx |= attrs);
		mutex_exit(lock);

		/* Set up referenced/modified emulation for new mapping. */
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
	 * If the HW / PALcode portion of the new PTE is the same as the
	 * old PTE, no TBI is necessary.
	 */
	if (opte & PG_V) {
		tflush = PG_PALCODE(opte) != PG_PALCODE(npte);
	}

	/* Set the new PTE. */
	atomic_store_relaxed(pte, npte);

out:
	PMAP_MAP_TO_HEAD_UNLOCK();

	/*
	 * Invalidate the TLB entry for this VA and any appropriate
	 * caches.
	 */
	if (tflush) {
		/* unlocks pmap */
		pmap_enter_tlb_shootdown(pmap, va, opte, true);
		if (pmap == pmap_kernel()) {
			TLB_COUNT(reason_enter_kernel);
		} else {
			TLB_COUNT(reason_enter_user);
		}
	} else {
		PMAP_UNLOCK(pmap);
	}

	if (opv)
		pmap_pv_free(opv);

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
	pmap_t const pmap = pmap_kernel();

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_kenter_pa(%lx, %lx, %x)\n",
		    va, pa, prot);
#endif

	KASSERT(va >= VM_MIN_KERNEL_ADDRESS);

	pt_entry_t * const pte = PMAP_KERNEL_PTE(va);

	/* Build the new PTE. */
	const pt_entry_t npte =
	    ((pa >> PGSHIFT) << PG_SHIFT) | pte_prot(pmap_kernel(), prot) |
	    PG_V | PG_WIRED;

	/* Set the new PTE. */
	const pt_entry_t opte = atomic_load_relaxed(pte);
	atomic_store_relaxed(pte, npte);

	PMAP_STAT_INCR(pmap->pm_stats.resident_count, 1);
	PMAP_STAT_INCR(pmap->pm_stats.wired_count, 1);

	/*
	 * There should not have been anything here, previously,
	 * so we can skip TLB shootdowns, etc. in the common case.
	 */
	if (__predict_false(opte & PG_V)) {
		const pt_entry_t diff = npte ^ opte;

		printf_nolog("%s: mapping already present\n", __func__);
		PMAP_STAT_DECR(pmap->pm_stats.resident_count, 1);
		if (diff & PG_WIRED)
			PMAP_STAT_DECR(pmap->pm_stats.wired_count, 1);
		/* XXX Can't handle this case. */
		if (diff & PG_PVLIST)
			panic("pmap_kenter_pa: old mapping was managed");

		pmap_enter_tlb_shootdown(pmap_kernel(), va, opte, false);
		TLB_COUNT(reason_kenter);
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
	pt_entry_t *pte, opte;
	pmap_t const pmap = pmap_kernel();
	struct pmap_tlb_context tlbctx;
	int count = 0;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_kremove(%lx, %lx)\n",
		    va, size);
#endif

	pmap_tlb_context_init(&tlbctx, 0);

	KASSERT(va >= VM_MIN_KERNEL_ADDRESS);

	for (; size != 0; size -= PAGE_SIZE, va += PAGE_SIZE) {
		pte = PMAP_KERNEL_PTE(va);
		opte = atomic_load_relaxed(pte);
		if (opte & PG_V) {
			KASSERT((opte & PG_PVLIST) == 0);

			/* Zap the mapping. */
			atomic_store_relaxed(pte, PG_NV);
			pmap_tlb_shootdown(pmap, va, opte, &tlbctx);

			count++;
		}
	}

	/* Update stats. */
	if (__predict_true(count != 0)) {
		PMAP_STAT_DECR(pmap->pm_stats.resident_count, count);
		PMAP_STAT_DECR(pmap->pm_stats.wired_count, count);
	}

	pmap_tlb_shootnow(&tlbctx);
	TLB_COUNT(reason_kremove);
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

	pte = pmap_l3pte(pmap_lev1map(pmap), va, NULL);

	KASSERT(pte != NULL);
	KASSERT(pmap_pte_v(pte));

	/*
	 * If wiring actually changed (always?) clear the wire bit and
	 * update the wire count.  Note that wiring is not a hardware
	 * characteristic so there is no need to invalidate the TLB.
	 */
	if (pmap_pte_w_chg(pte, 0)) {
		pmap_pte_set_w(pte, false);
		PMAP_STAT_DECR(pmap->pm_stats.wired_count, 1);
	}
#ifdef DEBUG
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
	if (__predict_true(pmap == pmap_kernel())) {
#ifdef DEBUG
		bool address_is_valid = vtophys_internal(va, pap);
		if (pmapdebug & PDB_FOLLOW) {
			if (address_is_valid) {
				printf("0x%lx (kernel vtophys)\n", *pap);
			} else {
				printf("failed (kernel vtophys)\n");
			}
		}
		return address_is_valid;
#else
		return vtophys_internal(va, pap);
#endif
	}

	pt_entry_t * const lev1map = pmap_lev1map(pmap);

	PMAP_LOCK(pmap);

	l1pte = pmap_l1pte(lev1map, va);
	if (pmap_pte_v(l1pte) == 0)
		goto out;

	l2pte = pmap_l2pte(lev1map, va, l1pte);
	if (pmap_pte_v(l2pte) == 0)
		goto out;

	l3pte = pmap_l3pte(lev1map, va, l2pte);
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
 *	reloading the MMU context of the current process, and marking
 *	the pmap in use by the processor.
 */
void
pmap_activate(struct lwp *l)
{
	struct pmap * const pmap = l->l_proc->p_vmspace->vm_map.pmap;
	struct pcb * const pcb = lwp_getpcb(l);

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_activate(%p)\n", l);
#endif

	KASSERT(kpreempt_disabled());

	struct cpu_info * const ci = curcpu();

	KASSERT(l == ci->ci_curlwp);

	u_long const old_ptbr = pcb->pcb_hw.apcb_ptbr;
	u_int const old_asn = pcb->pcb_hw.apcb_asn;

	/*
	 * We hold the activation lock to synchronize with TLB shootdown.
	 * The kernel pmap does not require those tests because shootdowns
	 * for the kernel pmap are always sent to all CPUs.
	 */
	if (pmap != pmap_kernel()) {
		PMAP_ACT_LOCK(pmap);
		pcb->pcb_hw.apcb_asn = pmap_asn_alloc(pmap, ci);
		atomic_or_ulong(&pmap->pm_cpus, (1UL << ci->ci_cpuid));
	} else {
		pcb->pcb_hw.apcb_asn = PMAP_ASN_KERNEL;
	}
	pcb->pcb_hw.apcb_ptbr =
	    ALPHA_K0SEG_TO_PHYS((vaddr_t)pmap_lev1map(pmap)) >> PGSHIFT;

	/*
	 * Check to see if the ASN or page table base has changed; if
	 * so, switch to our own context again so that it will take
	 * effect.
	 *
	 * We test ASN first because it's the most likely value to change.
	 */
	if (old_asn != pcb->pcb_hw.apcb_asn ||
	    old_ptbr != pcb->pcb_hw.apcb_ptbr) {
		if (old_asn != pcb->pcb_hw.apcb_asn &&
		    old_ptbr != pcb->pcb_hw.apcb_ptbr) {
			TLB_COUNT(activate_both_change);
		} else if (old_asn != pcb->pcb_hw.apcb_asn) {
			TLB_COUNT(activate_asn_change);
		} else {
			TLB_COUNT(activate_ptbr_change);
		}
		(void) alpha_pal_swpctx((u_long)l->l_md.md_pcbpaddr);
		TLB_COUNT(activate_swpctx);
	} else {
		TLB_COUNT(activate_skip_swpctx);
	}

	pmap_reference(pmap);
	ci->ci_pmap = pmap;

	if (pmap != pmap_kernel()) {
		PMAP_ACT_UNLOCK(pmap);
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
	struct pmap * const pmap = l->l_proc->p_vmspace->vm_map.pmap;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_deactivate(%p)\n", l);
#endif

	KASSERT(kpreempt_disabled());

	struct cpu_info * const ci = curcpu();

	KASSERT(l == ci->ci_curlwp);
	KASSERT(pmap == ci->ci_pmap);

	/*
	 * There is no need to switch to a different PTBR here,
	 * because a pmap_activate() or SWPCTX is guaranteed
	 * before whatever lev1map we're on now is invalidated
	 * or before user space is accessed again.
	 *
	 * Because only kernel mappings will be accessed before the
	 * next pmap_activate() call, we consider our CPU to be on
	 * the kernel pmap.
	 */
	ci->ci_pmap = pmap_kernel();
	KASSERT(atomic_load_relaxed(&pmap->pm_count) > 1);
	pmap_destroy(pmap);
}

/* pmap_zero_page() is in pmap_subr.s */

/* pmap_copy_page() is in pmap_subr.s */

/*
 * pmap_pageidlezero:		[ INTERFACE ]
 *
 *	Page zero'er for the idle loop.  Returns true if the
 *	page was zero'd, FALSE if we aborted for some reason.
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
	kmutex_t *lock;
	struct pmap_tlb_context tlbctx;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_modify(%p)\n", pg);
#endif

	pmap_tlb_context_init(&tlbctx, TLB_CTX_F_PV);

	PMAP_HEAD_TO_MAP_LOCK();
	lock = pmap_pvh_lock(pg);
	mutex_enter(lock);

	if (md->pvh_listx & PGA_MODIFIED) {
		rv = true;
		pmap_changebit(pg, PG_FOW, ~0UL, &tlbctx);
		md->pvh_listx &= ~PGA_MODIFIED;
	}

	mutex_exit(lock);
	PMAP_HEAD_TO_MAP_UNLOCK();

	pmap_tlb_shootnow(&tlbctx);
	TLB_COUNT(reason_clear_modify);

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
	kmutex_t *lock;
	struct pmap_tlb_context tlbctx;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_reference(%p)\n", pg);
#endif

	pmap_tlb_context_init(&tlbctx, TLB_CTX_F_PV);

	PMAP_HEAD_TO_MAP_LOCK();
	lock = pmap_pvh_lock(pg);
	mutex_enter(lock);

	if (md->pvh_listx & PGA_REFERENCED) {
		rv = true;
		pmap_changebit(pg, PG_FOR | PG_FOW | PG_FOE, ~0UL, &tlbctx);
		md->pvh_listx &= ~PGA_REFERENCED;
	}

	mutex_exit(lock);
	PMAP_HEAD_TO_MAP_UNLOCK();

	pmap_tlb_shootnow(&tlbctx);
	TLB_COUNT(reason_clear_reference);

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
 *	that it can be called when the PV list is already locked.
 *	(pmap_page_protect()).  In this case, the caller must be
 *	careful to get the next PV entry while we remove this entry
 *	from beneath it.  We assume that the pmap itself is already
 *	locked; dolock applies only to the PV list.
 *
 *	Returns important PTE bits that the caller needs to check for
 *	TLB / I-stream invalidation purposes.
 */
static pt_entry_t
pmap_remove_mapping(pmap_t pmap, vaddr_t va, pt_entry_t *pte,
    bool dolock, pv_entry_t *opvp, struct pmap_tlb_context * const tlbctx)
{
	pt_entry_t opte;
	paddr_t pa;
	struct vm_page *pg;		/* if != NULL, page is managed */

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove_mapping(%p, %lx, %p, %d, %p, %p)\n",
		       pmap, va, pte, dolock, opvp, tlbctx);
#endif

	/*
	 * PTE not provided, compute it from pmap and va.
	 */
	if (pte == NULL) {
		pte = pmap_l3pte(pmap_lev1map(pmap), va, NULL);
		if (pmap_pte_v(pte) == 0)
			return 0;
	}

	opte = *pte;

	pa = PG_PFNUM(opte) << PGSHIFT;

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
	atomic_store_relaxed(pte, PG_NV);

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
		pmap_l3pt_delref(pmap, va, pte, tlbctx);
	}

	if (opte & PG_PVLIST) {
		/*
		 * Remove it from the PV table.
		 */
		pg = PHYS_TO_VM_PAGE(pa);
		KASSERT(pg != NULL);
		pmap_pv_remove(pmap, pg, va, dolock, opvp, tlbctx);
		KASSERT(opvp == NULL || *opvp != NULL);
	}

	return opte & (PG_V | PG_ASM | PG_EXEC);
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
pmap_changebit(struct vm_page *pg, pt_entry_t set, pt_entry_t mask,
    struct pmap_tlb_context * const tlbctx)
{
	pv_entry_t pv;
	pt_entry_t *pte, npte, opte;

#ifdef DEBUG
	if (pmapdebug & PDB_BITS)
		printf("pmap_changebit(%p, 0x%lx, 0x%lx)\n",
		    pg, set, mask);
#endif

	/*
	 * Loop over all current mappings setting/clearing as apropos.
	 */
	for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
		PMAP_LOCK(pv->pv_pmap);

		pte = pv->pv_pte;

		opte = atomic_load_relaxed(pte);
		npte = (opte | set) & mask;
		if (npte != opte) {
			atomic_store_relaxed(pte, npte);
			pmap_tlb_shootdown_pv(pv->pv_pmap, pv->pv_va,
			    opte, tlbctx);
		}
		PMAP_UNLOCK(pv->pv_pmap);
	}
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
		pte = pmap_l3pte(pmap_lev1map(pmap), v, NULL);
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
	struct pmap_tlb_context tlbctx;

	pmap_tlb_context_init(&tlbctx, TLB_CTX_F_PV);

	PMAP_HEAD_TO_MAP_LOCK();
	lock = pmap_pvh_lock(pg);
	mutex_enter(lock);

	if (type == ALPHA_MMCSR_FOW) {
		md->pvh_listx |= (PGA_REFERENCED|PGA_MODIFIED);
		faultoff = PG_FOR | PG_FOW;
	} else {
		md->pvh_listx |= PGA_REFERENCED;
		faultoff = PG_FOR;
		if (exec) {
			faultoff |= PG_FOE;
		}
	}
	pmap_changebit(pg, 0, ~faultoff, &tlbctx);

	mutex_exit(lock);
	PMAP_HEAD_TO_MAP_UNLOCK();

	pmap_tlb_shootnow(&tlbctx);
	TLB_COUNT(reason_emulate_reference);

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

	printf("pa 0x%lx (attrs = 0x%lx):\n", pa, md->pvh_listx & PGA_ATTRS);
	for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next)
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
static bool
vtophys_internal(vaddr_t const vaddr, paddr_t * const pap)
{
	paddr_t pa;

	KASSERT(vaddr >= ALPHA_K0SEG_BASE);

	if (vaddr <= ALPHA_K0SEG_END) {
		pa = ALPHA_K0SEG_TO_PHYS(vaddr);
	} else {
		pt_entry_t * const pte = PMAP_KERNEL_PTE(vaddr);
		if (__predict_false(! pmap_pte_v(pte))) {
			return false;
		}
		pa = pmap_pte_pa(pte) | (vaddr & PGOFSET);
	}

	if (pap != NULL) {
		*pap = pa;
	}

	return true;
}

paddr_t
vtophys(vaddr_t const vaddr)
{
	paddr_t pa;

	if (__predict_false(! vtophys_internal(vaddr, &pa)))
		pa = 0;
	return pa;
}

/******************** pv_entry management ********************/

/*
 * pmap_pv_enter:
 *
 *	Add a physical->virtual entry to the pv_table.
 */
static int
pmap_pv_enter(pmap_t pmap, struct vm_page *pg, vaddr_t va, pt_entry_t *pte,
    bool dolock, pv_entry_t newpv)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	kmutex_t *lock;

	/*
	 * Allocate and fill in the new pv_entry.
	 */
	if (newpv == NULL) {
		newpv = pmap_pv_alloc();
		if (newpv == NULL)
			return ENOMEM;
	}
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
	for (pv = VM_MDPAGE_PVS(pg); pv != NULL; pv = pv->pv_next) {
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
	uintptr_t const attrs = md->pvh_listx & PGA_ATTRS;
	newpv->pv_next = (struct pv_entry *)(md->pvh_listx & ~PGA_ATTRS);
	md->pvh_listx = (uintptr_t)newpv | attrs;
	LIST_INSERT_HEAD(&pmap->pm_pvents, newpv, pv_link);

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
pmap_pv_remove(pmap_t pmap, struct vm_page *pg, vaddr_t va, bool dolock,
    pv_entry_t *opvp, struct pmap_tlb_context * const tlbctx)
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
	for (pvp = (struct pv_entry **)&md->pvh_listx, pv = VM_MDPAGE_PVS(pg);
	     pv != NULL; pvp = &pv->pv_next, pv = *pvp)
		if (pmap == pv->pv_pmap && va == pv->pv_va)
			break;

	KASSERT(pv != NULL);

	/*
	 * The page attributes are in the lower 2 bits of the first
	 * PV entry pointer.  Rather than comparing the pointer address
	 * and branching, we just always preserve what might be there
	 * (either attribute bits or zero bits).
	 */
	*pvp = (pv_entry_t)((uintptr_t)pv->pv_next |
			    (((uintptr_t)*pvp) & PGA_ATTRS));
	LIST_REMOVE(pv, pv_link);

	if (dolock) {
		mutex_exit(lock);
	}

	if (opvp != NULL) {
		*opvp = pv;
	} else {
		KASSERT(tlbctx != NULL);
		LIST_INSERT_HEAD(&tlbctx->t_freepvq, pv, pv_link);
	}
}

/*
 * pmap_pv_page_alloc:
 *
 *	Allocate a page for the pv_entry pool.
 */
static void *
pmap_pv_page_alloc(struct pool *pp, int flags)
{
	struct vm_page * const pg = pmap_physpage_alloc(PGU_PVENT);
	if (__predict_false(pg == NULL)) {
		return NULL;
	}
	return (void *)ALPHA_PHYS_TO_K0SEG(VM_PAGE_TO_PHYS(pg));
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
static struct vm_page *
pmap_physpage_alloc(int usage)
{
	struct vm_page *pg;

	/*
	 * Don't ask for a zero'd page in the L1PT case -- we will
	 * properly initialize it in the constructor.
	 */

	pg = uvm_pagealloc(NULL, 0, NULL, usage == PGU_L1PT ?
	    UVM_PGA_USERESERVE : UVM_PGA_USERESERVE|UVM_PGA_ZERO);
	if (pg != NULL) {
		KASSERT(PHYSPAGE_REFCNT(pg) == 0);
	}
	return pg;
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

	KASSERT(PHYSPAGE_REFCNT(pg) == 0);

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
	paddr_t pa;

	pa = ALPHA_K0SEG_TO_PHYS(trunc_page((vaddr_t)kva));
	pg = PHYS_TO_VM_PAGE(pa);

	KASSERT(PHYSPAGE_REFCNT(pg) < UINT32_MAX);

	return PHYSPAGE_REFCNT_INC(pg);
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
	paddr_t pa;

	pa = ALPHA_K0SEG_TO_PHYS(trunc_page((vaddr_t)kva));
	pg = PHYS_TO_VM_PAGE(pa);

	KASSERT(PHYSPAGE_REFCNT(pg) != 0);

	return PHYSPAGE_REFCNT_DEC(pg);
}

/******************** page table page management ********************/

static bool
pmap_kptpage_alloc(paddr_t *pap)
{
	if (uvm.page_init_done == false) {
		/*
		 * We're growing the kernel pmap early (from
		 * uvm_pageboot_alloc()).  This case must
		 * be handled a little differently.
		 */
		*pap = ALPHA_K0SEG_TO_PHYS(
		    pmap_steal_memory(PAGE_SIZE, NULL, NULL));
		return true;
	}

	struct vm_page * const pg = pmap_physpage_alloc(PGU_NORMAL);
	if (__predict_true(pg != NULL)) {
		*pap = VM_PAGE_TO_PHYS(pg);
		return true;
	}
	return false;
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
	struct pmap *pm;
	paddr_t ptaddr;
	pt_entry_t *l1pte, *l2pte, pte;
	pt_entry_t *lev1map;
	vaddr_t va;
	int l1idx;

	rw_enter(&pmap_growkernel_lock, RW_WRITER);

	if (maxkvaddr <= virtual_end)
		goto out;		/* we are OK */

	pmap_growkernel_evcnt.ev_count++;

	va = virtual_end;

	while (va < maxkvaddr) {
		/*
		 * If there is no valid L1 PTE (i.e. no L2 PT page),
		 * allocate a new L2 PT page and insert it into the
		 * L1 map.
		 */
		l1pte = pmap_l1pte(kernel_lev1map, va);
		if (pmap_pte_v(l1pte) == 0) {
			if (!pmap_kptpage_alloc(&ptaddr))
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

				/*
				 * Any pmaps published on the global list
				 * should never be referencing kernel_lev1map.
				 */
				lev1map = pmap_lev1map(pm);
				KASSERT(lev1map != kernel_lev1map);

				PMAP_LOCK(pm);
				lev1map[l1idx] = pte;
				PMAP_UNLOCK(pm);
			}
			mutex_exit(&pmap_all_pmaps_lock);
		}

		/*
		 * Have an L2 PT page now, add the L3 PT page.
		 */
		l2pte = pmap_l2pte(kernel_lev1map, va, l1pte);
		KASSERT(pmap_pte_v(l2pte) == 0);
		if (!pmap_kptpage_alloc(&ptaddr))
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
 *	Page allocator for L1 PT pages.
 */
static void *
pmap_l1pt_alloc(struct pool *pp, int flags)
{
	/*
	 * Attempt to allocate a free page.
	 */
	struct vm_page * const pg = pmap_physpage_alloc(PGU_L1PT);
	if (__predict_false(pg == NULL)) {
		return NULL;
	}
	return (void *)ALPHA_PHYS_TO_K0SEG(VM_PAGE_TO_PHYS(pg));
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
 *	Allocate a level 2 or level 3 page table page for a user
 *	pmap, and initialize the PTE that references it.
 *
 *	Note: the pmap must already be locked.
 */
static int
pmap_ptpage_alloc(pmap_t pmap, pt_entry_t * const pte, int const usage)
{
	/*
	 * Allocate the page table page.
	 */
	struct vm_page * const pg = pmap_physpage_alloc(usage);
	if (__predict_false(pg == NULL)) {
		return ENOMEM;
	}

	LIST_INSERT_HEAD(&pmap->pm_ptpages, pg, pageq.list);

	/*
	 * Initialize the referencing PTE.
	 */
	const pt_entry_t npte = ((VM_PAGE_TO_PHYS(pg) >> PGSHIFT) << PG_SHIFT) |
	    PG_V | PG_KRE | PG_KWE | PG_WIRED;

	atomic_store_relaxed(pte, npte);

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
pmap_ptpage_free(pmap_t pmap, pt_entry_t * const pte,
    struct pmap_tlb_context * const tlbctx)
{

	/*
	 * Extract the physical address of the page from the PTE
	 * and clear the entry.
	 */
	const paddr_t ptpa = pmap_pte_pa(pte);
	atomic_store_relaxed(pte, PG_NV);

	struct vm_page * const pg = PHYS_TO_VM_PAGE(ptpa);
	KASSERT(pg != NULL);

	KASSERT(PHYSPAGE_REFCNT(pg) == 0);
#ifdef DEBUG
	pmap_zero_page(ptpa);
#endif

	LIST_REMOVE(pg, pageq.list);
	LIST_INSERT_HEAD(&tlbctx->t_freeptq, pg, pageq.list);
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
pmap_l3pt_delref(pmap_t pmap, vaddr_t va, pt_entry_t *l3pte,
    struct pmap_tlb_context * const tlbctx)
{
	pt_entry_t *l1pte, *l2pte;
	pt_entry_t * const lev1map = pmap_lev1map(pmap);

	l1pte = pmap_l1pte(lev1map, va);
	l2pte = pmap_l2pte(lev1map, va, l1pte);

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
		/*
		 * You can pass NULL if you know the last reference won't
		 * be dropped.
		 */
		KASSERT(tlbctx != NULL);
		pmap_ptpage_free(pmap, l2pte, tlbctx);

		/*
		 * We've freed a level 3 table, so we must invalidate
		 * any now-stale TLB entries for the corresponding VPT
		 * VA range.  Easiest way to guarantee this is to hit
		 * all of the user TLB entries.
		 */
		pmap_tlb_shootdown_all_user(pmap, PG_V, tlbctx);

		/*
		 * We've freed a level 3 table, so delete the reference
		 * on the level 2 table.
		 */
		pmap_l2pt_delref(pmap, l1pte, l2pte, tlbctx);
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
    struct pmap_tlb_context * const tlbctx)
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
		/*
		 * You can pass NULL if you know the last reference won't
		 * be dropped.
		 */
		KASSERT(tlbctx != NULL);
		pmap_ptpage_free(pmap, l1pte, tlbctx);

		/*
		 * We've freed a level 2 table, so we must invalidate
		 * any now-stale TLB entries for the corresponding VPT
		 * VA range.  Easiest way to guarantee this is to hit
		 * all of the user TLB entries.
		 */
		pmap_tlb_shootdown_all_user(pmap, PG_V, tlbctx);

		/*
		 * We've freed a level 2 table, so delete the reference
		 * on the level 1 table.
		 */
		pmap_l1pt_delref(pmap, l1pte);
	}
}

/*
 * pmap_l1pt_delref:
 *
 *	Delete a reference on a level 1 PT page.
 */
static void
pmap_l1pt_delref(pmap_t pmap, pt_entry_t *l1pte)
{

	KASSERT(pmap != pmap_kernel());

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
static u_int
pmap_asn_alloc(pmap_t const pmap, struct cpu_info * const ci)
{

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ASN))
		printf("pmap_asn_alloc(%p)\n", pmap);
#endif

	KASSERT(pmap != pmap_kernel());
	KASSERT(pmap->pm_percpu[ci->ci_cpuid].pmc_lev1map != kernel_lev1map);
	KASSERT(kpreempt_disabled());

	/* No work to do if the CPU does not implement ASNs. */
	if (pmap_max_asn == 0)
		return 0;

	struct pmap_percpu * const pmc = &pmap->pm_percpu[ci->ci_cpuid];

	/*
	 * Hopefully, we can continue using the one we have...
	 *
	 * N.B. the generation check will fail the first time
	 * any pmap is activated on a given CPU, because we start
	 * the generation counter at 1, but initialize pmaps with
	 * 0; this forces the first ASN allocation to occur.
	 */
	if (pmc->pmc_asngen == ci->ci_asn_gen) {
#ifdef DEBUG
		if (pmapdebug & PDB_ASN)
			printf("pmap_asn_alloc: same generation, keeping %u\n",
			    pmc->pmc_asn);
#endif
		TLB_COUNT(asn_reuse);
		return pmc->pmc_asn;
	}

	/*
	 * Need to assign a new ASN.  Grab the next one, incrementing
	 * the generation number if we have to.
	 */
	if (ci->ci_next_asn > pmap_max_asn) {
		/*
		 * Invalidate all non-PG_ASM TLB entries and the
		 * I-cache, and bump the generation number.
		 */
		ALPHA_TBIAP();
		alpha_pal_imb();

		ci->ci_next_asn = PMAP_ASN_FIRST_USER;
		ci->ci_asn_gen++;
		TLB_COUNT(asn_newgen);

		/*
		 * Make sure the generation number doesn't wrap.  We could
		 * handle this scenario by traversing all of the pmaps,
		 * and invalidating the generation number on those which
		 * are not currently in use by this processor.
		 *
		 * However... considering that we're using an unsigned 64-bit
		 * integer for generation numbers, on non-ASN CPUs, we won't
		 * wrap for approximately 75 billion years on a 128-ASN CPU
		 * (assuming 1000 switch * operations per second).
		 *
		 * So, we don't bother.
		 */
		KASSERT(ci->ci_asn_gen != PMAP_ASNGEN_INVALID);
#ifdef DEBUG
		if (pmapdebug & PDB_ASN)
			printf("pmap_asn_alloc: generation bumped to %lu\n",
			    ci->ci_asn_gen);
#endif
	}

	/*
	 * Assign the new ASN and validate the generation number.
	 */
	pmc->pmc_asn = ci->ci_next_asn++;
	pmc->pmc_asngen = ci->ci_asn_gen;
	TLB_COUNT(asn_assign);

	/*
	 * We have a new ASN, so we can skip any pending I-stream sync
	 * on the way back out to user space.
	 */
	pmc->pmc_needisync = 0;

#ifdef DEBUG
	if (pmapdebug & PDB_ASN)
		printf("pmap_asn_alloc: assigning %u to pmap %p\n",
		    pmc->pmc_asn, pmap);
#endif
	return pmc->pmc_asn;
}
