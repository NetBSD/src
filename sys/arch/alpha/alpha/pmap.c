/* $NetBSD: pmap.c,v 1.77 1999/02/04 19:49:22 thorpej Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 *	This pmap implementation only supports NBPG == PAGE_SIZE.
 *	In practice, this is not a problem since PAGE_SIZE is
 *	initialized to the hardware page size in alpha_init().
 *
 * Bugs/misfeatures:
 *
 *	Does not currently support multiple processors.  Problems:
 *
 *		- TLB shootdown code needs to be written.
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
#include "opt_uvm.h"
#include "opt_pmap_new.h"
#include "opt_new_scc_driver.h"
#include "opt_sysv.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.77 1999/02/04 19:49:22 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/user.h>
#include <sys/buf.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#if defined(UVM)
#include <uvm/uvm.h>
#endif

#include <machine/cpu.h>
#ifdef _PMAP_MAY_USE_PROM_CONSOLE
#include <machine/rpb.h>			/* XXX */
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
int	protection_codes[2][8];

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
pt_entry_t	*VPT;

struct pmap	kernel_pmap_store;
u_int		kernel_pmap_asn_store[ALPHA_MAXPROCS];
u_long		kernel_pmap_asngen_store[ALPHA_MAXPROCS];

paddr_t    	avail_start;	/* PA of first available physical page */
paddr_t		avail_end;	/* PA of last available physical page */
vaddr_t		virtual_avail;  /* VA of first avail page (after kernel bss)*/
vaddr_t		virtual_end;	/* VA of last avail page (end of kernel AS) */

boolean_t	pmap_initialized;	/* Has pmap_init completed? */

u_long		pmap_pages_stolen;	/* instrumentation */

/*
 * This variable contains the number of CPU IDs we need to allocate
 * space for when allocating the pmap structure.  It is used to
 * size a per-CPU array of ASN and ASN Generation number.
 */
u_long		pmap_ncpuids;

/*
 * Storage for physical->virtual entries and page attributes.
 */
struct pv_head	*pv_table;
int		pv_table_npages;

int		pmap_npvppg;	/* number of PV entries per page */

/*
 * Free list of pages for use by the physical->virtual entry memory allocator.
 */
TAILQ_HEAD(pv_page_list, pv_page) pv_page_freelist;
int		pv_nfree;

/*
 * List of all pmaps, used to update them when e.g. additional kernel
 * page tables are allocated.
 */
LIST_HEAD(, pmap) pmap_all_pmaps;

/*
 * The pools from which pmap structures and sub-structures are allocated.
 */
struct pool pmap_pmap_pool;
struct pool pmap_asn_pool;
struct pool pmap_asngen_pool;

/*
 * Canonical names for PGU_* constants.
 */
const char *pmap_pgu_strings[] = PGU_STRINGS;

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
u_int	pmap_max_asn;			/* max ASN supported by the system */
u_int	pmap_next_asn[ALPHA_MAXPROCS];	/* next free ASN to use */
u_long	pmap_asn_generation[ALPHA_MAXPROCS]; /* current ASN generation */

/*
 * Locking:
 *
 *	This pmap module uses two types of locks: `normal' (sleep)
 *	locks and `simple' (spin) locks.  They are used as follows:
 *
 *	NORMAL LOCKS
 *	------------
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
 *	SIMPLE LOCKS
 *	------------
 *
 *	* pm_slock (per-pmap) - This lock protects all of the members
 *	  of the pmap structure itself.  This lock will be asserted
 *	  in pmap_activate() and pmap_deactivate() from a critical
 *	  section of cpu_switch(), and must never sleep.
 *
 *	* pvh_slock (per-pv_head) - This lock protects the PV list
 *	  for a specified managed page.
 *
 *	* pmap_pvalloc_slock - This lock protects the data structures
 *	  which manage the PV entry allocator.
 *
 *	* pmap_all_pmaps_slock - This lock protects the global list of
 *	  all pmaps.
 *
 *	Address space number management (global ASN counters and per-pmap
 *	ASN state) are not locked; they use arrays of values indexed
 *	per-processor.
 *
 *	All internal functions which operate on a pmap are called
 *	with the pmap already locked by the caller (which will be
 *	an interface function).
 */
struct lock pmap_main_lock;
struct simplelock pmap_pvalloc_slock;
struct simplelock pmap_all_pmaps_slock;

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
#define	PMAP_MAP_TO_HEAD_LOCK() \
	lockmgr(&pmap_main_lock, LK_SHARED, NULL)
#define	PMAP_MAP_TO_HEAD_UNLOCK() \
	lockmgr(&pmap_main_lock, LK_RELEASE, NULL)
#define	PMAP_HEAD_TO_MAP_LOCK() \
	lockmgr(&pmap_main_lock, LK_EXCLUSIVE, NULL)
#define	PMAP_HEAD_TO_MAP_UNLOCK() \
	lockmgr(&pmap_main_lock, LK_RELEASE, NULL)
#else
#define	PMAP_MAP_TO_HEAD_LOCK()		/* nothing */
#define	PMAP_MAP_TO_HEAD_UNLOCK()	/* nothing */
#define	PMAP_HEAD_TO_MAP_LOCK()		/* nothing */
#define	PMAP_HEAD_TO_MAP_UNLOCK()	/* nothing */
#endif /* MULTIPROCESSOR || LOCKDEBUG */

#define	PAGE_IS_MANAGED(pa)	(vm_physseg_find(atop(pa), NULL) != -1)

static __inline struct pv_head *pa_to_pvh __P((paddr_t));

static __inline struct pv_head *
pa_to_pvh(pa)
	paddr_t pa;
{
	int bank, pg;

	bank = vm_physseg_find(atop(pa), &pg);
	return (&vm_physmem[bank].pmseg.pvhead[pg]);
}

/*
 * Internal routines
 */
void	alpha_protection_init __P((void));
boolean_t pmap_remove_mapping __P((pmap_t, vaddr_t, pt_entry_t *,
	    boolean_t, long));
void	pmap_changebit __P((paddr_t, pt_entry_t, pt_entry_t, long));
#ifndef PMAP_NEW
/* It's an interface function if PMAP_NEW. */
void	pmap_kremove __P((vaddr_t, vsize_t));
#endif

/*
 * PT page management functions.
 */
void	pmap_lev1map_create __P((pmap_t, long));
void	pmap_lev1map_destroy __P((pmap_t, long));
void	pmap_ptpage_alloc __P((pmap_t, pt_entry_t *, int));
void	pmap_ptpage_free __P((pmap_t, pt_entry_t *));
void	pmap_l3pt_delref __P((pmap_t, vaddr_t, pt_entry_t *, pt_entry_t *,
	    pt_entry_t *, long));
void	pmap_l2pt_delref __P((pmap_t, pt_entry_t *, pt_entry_t *, long));
void	pmap_l1pt_delref __P((pmap_t, pt_entry_t *, long));

/*
 * PV table management functions.
 */
void	pmap_pv_enter __P((pmap_t, paddr_t, vaddr_t, boolean_t));
void	pmap_pv_remove __P((pmap_t, paddr_t, vaddr_t, boolean_t));
struct	pv_entry *pmap_pv_alloc __P((void));
void	pmap_pv_free __P((struct pv_entry *));
void	pmap_pv_collect __P((void));
#ifdef DEBUG
void	pmap_pv_dump __P((paddr_t));
#endif

/*
 * ASN management functions.
 */
void	pmap_asn_alloc __P((pmap_t, long));

/*
 * Misc. functions.
 */
paddr_t	pmap_physpage_alloc __P((int));
void	pmap_physpage_free __P((paddr_t));
int	pmap_physpage_addref __P((void *));
int	pmap_physpage_delref __P((void *));

/*
 * PMAP_ISACTIVE{,_TEST}:
 *
 *	Check to see if a pmap is active on the current processor.
 */
#define	PMAP_ISACTIVE_TEST(pm)						\
	(((pm)->pm_cpus & (1UL << alpha_pal_whami())) != 0)

#ifdef DEBUG
#define	PMAP_ISACTIVE(pm)						\
({									\
	int isactive_ = PMAP_ISACTIVE_TEST(pm);				\
									\
	if (curproc != NULL &&						\
	   (isactive_ ^ ((pm) == curproc->p_vmspace->vm_map.pmap)))	\
		panic("PMAP_ISACTIVE");					\
	(isactive_);							\
})
#else
#define	PMAP_ISACTIVE(pm) PMAP_ISACTIVE_TEST(pm)
#endif /* DEBUG */

/*
 * PMAP_ACTIVATE_ASN_SANITY:
 *
 *	DEBUG sanity checks for ASNs within PMAP_ACTIVATE.
 */
#ifdef DEBUG
#define	PMAP_ACTIVATE_ASN_SANITY(pmap, cpu_id)				\
do {									\
	if ((pmap)->pm_lev1map == kernel_lev1map) {			\
		/*							\
		 * This pmap implementation also ensures that pmaps	\
		 * referencing kernel_lev1map use a reserved ASN	\
		 * ASN to prevent the PALcode from servicing a TLB	\
		 * miss	with the wrong PTE.				\
		 */							\
		if ((pmap)->pm_asn[(cpu_id)] != PMAP_ASN_RESERVED) {	\
			printf("kernel_lev1map with non-reserved ASN "	\
			    "(line %ld)\n", __LINE__);			\
			panic("PMAP_ACTIVATE_ASN_SANITY");		\
		}							\
	} else {							\
		if ((pmap)->pm_asngen[(cpu_id)] != 			\
		    pmap_asn_generation[(cpu_id)]) {			\
			/*						\
			 * ASN generation number isn't valid!		\
			 */						\
			printf("pmap asngen %lu, current %lu "		\
			    "(line %ld)\n",				\
			    (pmap)->pm_asngen[(cpu_id)], 		\
			    pmap_asn_generation[(cpu_id)],		\
			    __LINE__);					\
			panic("PMAP_ACTIVATE_ASN_SANITY");		\
		}							\
		if ((pmap)->pm_asn[(cpu_id)] == PMAP_ASN_RESERVED) {	\
			/*						\
			 * DANGER WILL ROBINSON!  We're going to	\
			 * pollute the VPT TLB entries!			\
			 */						\
			printf("Using reserved ASN! (line %ld)\n",	\
			    __LINE__);					\
			panic("PMAP_ACTIVATE_ASN_SANITY");		\
		}							\
	}								\
} while (0)
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
#define	PMAP_ACTIVATE(pmap, p, cpu_id)					\
do {									\
	PMAP_ACTIVATE_ASN_SANITY(pmap, cpu_id);				\
									\
	(p)->p_addr->u_pcb.pcb_hw.apcb_ptbr =				\
	    ALPHA_K0SEG_TO_PHYS((vaddr_t)(pmap)->pm_lev1map) >> PGSHIFT; \
	(p)->p_addr->u_pcb.pcb_hw.apcb_asn = (pmap)->pm_asn[(cpu_id)];	\
									\
	if ((p) == curproc) {						\
		/*							\
		 * Page table base register has changed; switch to	\
		 * our own context again so that it will take effect.	\
		 */							\
		(void) alpha_pal_swpctx((u_long)p->p_md.md_pcbpaddr);	\
	}								\
} while (0)

/*
 * PMAP_INVALIDATE_ASN:
 *
 *	Invalidate the specified pmap's ASN, so as to force allocation
 *	of a new one the next time pmap_asn_alloc() is called.
 *
 *	NOTE: THIS MUST ONLY BE CALLED IF AT LEAST ONE OF THE FOLLOWING
 *	CONDITIONS ARE TRUE:
 *
 *		(1) The pmap references the global kernel_lev1map.
 *
 *		(2) The pmap is not active on the current processor.
 */
#define	PMAP_INVALIDATE_ASN(pmap, cpu_id)				\
do {									\
	(pmap)->pm_asn[(cpu_id)] = PMAP_ASN_RESERVED;			\
} while (0)

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
	} else if ((pmap)->pm_asngen[(cpu_id)] == 			\
	    pmap_asn_generation[(cpu_id)]) {				\
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
} while (0)

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
		    "(line %ld)\n", (va), __LINE__);			\
		panic("PMAP_KERNEL_PTE");				\
	}								\
	l2pte_ = pmap_l2pte(pmap_kernel(), va, l1pte_);			\
	if (pmap_pte_v(l2pte_) == 0) {					\
		printf("kernel level 2 PTE not valid, va 0x%lx "	\
		    "(line %ld)\n", (va), __LINE__);			\
		panic("PMAP_KERNEL_PTE");				\
	}								\
	pmap_l3pte(pmap_kernel(), va, l2pte_);				\
})
#else
#define	PMAP_KERNEL_PTE(va)	(&VPT[VPT_INDEX((va))])
#endif

/*
 * pmap_bootstrap:
 *
 *	Bootstrap the system to run with virtual memory.
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_bootstrap(ptaddr, maxasn, ncpuids)
	paddr_t ptaddr;
	u_int maxasn;
	u_long ncpuids;
{
	vsize_t lev2mapsize, lev3mapsize;
	pt_entry_t *lev2map, *lev3map;
	pt_entry_t pte;
	int i;
	extern int physmem;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_BOOTSTRAP))
		printf("pmap_bootstrap(0x%lx, %u)\n", ptaddr, maxasn);
#endif

	/*
	 * Figure out how many PTE's are necessary to map the kernel.
	 * The '512' comes from PAGER_MAP_SIZE in vm_pager_init().
	 * This should be kept in sync.
	 * We also reserve space for kmem_alloc_pageable() for vm_fork().
	 */
	lev3mapsize = (VM_KMEM_SIZE + VM_MBUF_SIZE + VM_PHYS_SIZE +
		nbuf * MAXBSIZE + 16 * NCARGS) / NBPG + 512 +
		(maxproc * UPAGES);

#ifdef SYSVSHM
	lev3mapsize += shminfo.shmall;
#endif
	lev3mapsize = roundup(lev3mapsize, NPTEPG);

	/*
	 * Allocate a level 1 PTE table for the kernel.
	 * This is always one page long.
	 * IF THIS IS NOT A MULTIPLE OF NBPG, ALL WILL GO TO HELL.
	 */
	kernel_lev1map = (pt_entry_t *)
	    pmap_steal_memory(sizeof(pt_entry_t) * NPTEPG, NULL, NULL);

	/*
	 * Allocate a level 2 PTE table for the kernel.
	 * These must map all of the level3 PTEs.
	 * IF THIS IS NOT A MULTIPLE OF NBPG, ALL WILL GO TO HELL.
	 */
	lev2mapsize = roundup(howmany(lev3mapsize, NPTEPG), NPTEPG);
	lev2map = (pt_entry_t *)
	    pmap_steal_memory(sizeof(pt_entry_t) * lev2mapsize, NULL, NULL);

	/*
	 * Allocate a level 3 PTE table for the kernel.
	 * Contains lev3mapsize PTEs.
	 */
	lev3map = (pt_entry_t *)
	    pmap_steal_memory(sizeof(pt_entry_t) * lev3mapsize, NULL, NULL);

	/*
	 * Allocate memory for the pv_heads.  (A few more of the latter
	 * are allocated than are needed.)
	 *
	 * We could do this in pmap_init when we know the actual
	 * managed page pool size, but its better to use kseg0
	 * addresses rather than kernel virtual addresses mapped
	 * through the TLB.
	 */
	pv_table_npages = physmem;
	pv_table = (struct pv_head *)
	    pmap_steal_memory(sizeof(struct pv_head) * pv_table_npages,
	    NULL, NULL);

	/*
	 * ...and intialize the pv_entry list headers.
	 */
	for (i = 0; i < pv_table_npages; i++) {
		LIST_INIT(&pv_table[i].pvh_list);
		simple_lock_init(&pv_table[i].pvh_slock);
	}

	/*
	 * Set up level 1 page table
	 */

#ifdef _PMAP_MAY_USE_PROM_CONSOLE
    {
	extern pt_entry_t rom_pte;			/* XXX */
	extern int prom_mapped;				/* XXX */

	if (pmap_uses_prom_console()) {
		/* XXX save old pte so that we can remap prom if necessary */
		rom_pte = *(pt_entry_t *)ptaddr & ~PG_ASM;	/* XXX */
	}
	prom_mapped = 0;

	/*
	 * Actually, this code lies.  The prom is still mapped, and will
	 * remain so until the context switch after alpha_init() returns.
	 * Printfs using the firmware before then will end up frobbing
	 * kernel_lev1map unnecessarily, but that's OK.
	 */
    }
#endif

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

	/*
	 * Set up level three page table (lev3map)
	 */
	/* Nothing to do; it's already zero'd */

	/*
	 * Initialize `FYI' variables.  Note we're relying on
	 * the fact that BSEARCH sorts the vm_physmem[] array
	 * for us.
	 */
	avail_start = ptoa(vm_physmem[0].start);
	avail_end = ptoa(vm_physmem[vm_nphysseg - 1].end);
	virtual_avail = VM_MIN_KERNEL_ADDRESS;
	virtual_end = VM_MIN_KERNEL_ADDRESS + lev3mapsize * NBPG;

#if 0
	printf("avail_start = 0x%lx\n", avail_start);
	printf("avail_end = 0x%lx\n", avail_end);
	printf("virtual_avail = 0x%lx\n", virtual_avail);
	printf("virtual_end = 0x%lx\n", virtual_end);
#endif

	/*
	 * Intialize the pmap pools and list.
	 */
	pmap_ncpuids = ncpuids;
	pool_init(&pmap_pmap_pool, sizeof(struct pmap), 0, 0, 0, "pmappl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_VMPMAP);
	pool_init(&pmap_asn_pool, pmap_ncpuids * sizeof(u_int), 0, 0, 0,
	    "pmasnpl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_VMPMAP);
	pool_init(&pmap_asngen_pool, pmap_ncpuids * sizeof(u_long), 0, 0, 0,
	    "pmasngenpl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_VMPMAP);
	LIST_INIT(&pmap_all_pmaps);

	/*
	 * Initialize the ASN logic.
	 */
	pmap_max_asn = maxasn;
	for (i = 0; i < ALPHA_MAXPROCS; i++) {
		pmap_next_asn[i] = 1;
		pmap_asn_generation[i] = 0;
	}

	/*
	 * Initialize the locks.
	 */
	lockinit(&pmap_main_lock, PVM, "pmaplk", 0, 0);
	simple_lock_init(&pmap_pvalloc_slock);
	simple_lock_init(&pmap_all_pmaps_slock);

	/*
	 * Initialize kernel pmap.  Note that all kernel mappings
	 * have PG_ASM set, so the ASN doesn't really matter for
	 * the kernel pmap.  Also, since the kernel pmap always
	 * references kernel_lev1map, it always has an invalid ASN
	 * generation.
	 */
	pmap_kernel()->pm_lev1map = kernel_lev1map;
	pmap_kernel()->pm_count = 1;
	pmap_kernel()->pm_asn = kernel_pmap_asn_store;
	pmap_kernel()->pm_asngen = kernel_pmap_asngen_store;
	for (i = 0; i < ALPHA_MAXPROCS; i++) {
		pmap_kernel()->pm_asn[i] = PMAP_ASN_RESERVED;
		pmap_kernel()->pm_asngen[i] = pmap_asn_generation[i];
	}
	simple_lock_init(&pmap_kernel()->pm_slock);
	LIST_INSERT_HEAD(&pmap_all_pmaps, pmap_kernel(), pm_list);

	/*
	 * Set up proc0's PCB such that the ptbr points to the right place
	 * and has the kernel pmap's (really unused) ASN.
	 */
	proc0.p_addr->u_pcb.pcb_hw.apcb_ptbr =
	    ALPHA_K0SEG_TO_PHYS((vaddr_t)kernel_lev1map) >> PGSHIFT;
	proc0.p_addr->u_pcb.pcb_hw.apcb_asn =
	    pmap_kernel()->pm_asn[alpha_pal_whami()];

	/*
	 * Mark the kernel pmap `active' on this processor.
	 */
	alpha_atomic_setbits_q(&pmap_kernel()->pm_cpus,
	    (1UL << alpha_pal_whami()));
}

#ifdef _PMAP_MAY_USE_PROM_CONSOLE
int
pmap_uses_prom_console()
{
	extern int cputype;

#if defined(NEW_SCC_DRIVER)
	return (cputype == ST_DEC_21000 || cputype == ST_DEC_4100);
#else
	return (cputype == ST_DEC_21000
	    || cputype == ST_DEC_4100
	    || cputype == ST_DEC_3000_300
	    || cputype == ST_DEC_3000_500);
#endif /* NEW_SCC_DRIVER */
}
#endif _PMAP_MAY_USE_PROM_CONSOLE

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
 *	Note: no locking is necessary in this function.
 */
vaddr_t
pmap_steal_memory(size, vstartp, vendp)
	vsize_t size;
	vaddr_t *vstartp, *vendp;
{
	int bank, npgs, x;
	vaddr_t va;
	paddr_t pa;

	size = round_page(size);
	npgs = atop(size);

	for (bank = 0; bank < vm_nphysseg; bank++) {
		if (vm_physmem[bank].pgs)
			panic("pmap_steal_memory: called _after_ bootstrap");

		if (vm_physmem[bank].avail_start != vm_physmem[bank].start ||
		    vm_physmem[bank].avail_start >= vm_physmem[bank].avail_end)
			continue;

		if ((vm_physmem[bank].avail_end - vm_physmem[bank].avail_start)
		    < npgs)
			continue;

		/*
		 * There are enough pages here; steal them!
		 */
		pa = ptoa(vm_physmem[bank].avail_start);
		vm_physmem[bank].avail_start += npgs;
		vm_physmem[bank].start += npgs;

		/*
		 * Have we used up this segment?
		 */
		if (vm_physmem[bank].avail_start == vm_physmem[bank].end) {
			if (vm_nphysseg == 1)
				panic("pmap_steal_memory: out of memory!");

			/* Remove this segment from the list. */
			vm_nphysseg--;
			for (x = bank; x < vm_nphysseg; x++) {
				/* structure copy */
				vm_physmem[x] = vm_physmem[x + 1];
			}
		}

		/*
		 * Fill these in for the caller; we don't modify them,
		 * but the upper layers still want to know.
		 */
		if (vstartp)
			*vstartp = round_page(virtual_avail);
		if (vendp)
			*vendp = trunc_page(virtual_end);

		va = ALPHA_PHYS_TO_K0SEG(pa);
		bzero((caddr_t)va, size);
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
pmap_init()
{
	vsize_t		s;
	int		bank;
	struct pv_head	*pvh;

#ifdef DEBUG
        if (pmapdebug & PDB_FOLLOW)
                printf("pmap_init()\n");
#endif

	/*
	 * Compute how many PV entries will fit in a single
	 * physical page of memory.
	 */
	pmap_npvppg = (PAGE_SIZE - sizeof(struct pv_page_info)) /
	    sizeof(struct pv_entry);

	/* initialize protection array */
	alpha_protection_init();

	/*
	 * Memory for the pv heads has already been allocated.
	 * Initialize the physical memory segments.
	 */
	pvh = pv_table;
	for (bank = 0; bank < vm_nphysseg; bank++) {
		s = vm_physmem[bank].end - vm_physmem[bank].start;
		vm_physmem[bank].pmseg.pvhead = pvh;
		pvh += s;
	}

	/*
	 * Intialize the pv_page free list.
	 */
	TAILQ_INIT(&pv_page_freelist);

	/*
	 * Now it is safe to enable pv entry recording.
	 */
	pmap_initialized = TRUE;

#if 0
	for (bank = 0; bank < vm_nphysseg; bank++) {
		printf("bank %d\n", bank);
		printf("\tstart = 0x%x\n", ptoa(vm_physmem[bank].start));
		printf("\tend = 0x%x\n", ptoa(vm_physmem[bank].end));
		printf("\tavail_start = 0x%x\n",
		    ptoa(vm_physmem[bank].avail_start));
		printf("\tavail_end = 0x%x\n",
		    ptoa(vm_physmem[bank].avail_end));
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
#if defined(PMAP_NEW)
pmap_t
pmap_create()
#else /* ! PMAP_NEW */
pmap_t
pmap_create(size)
	vsize_t		size;
#endif /* PMAP_NEW */
{
	pmap_t pmap;
	int i;

#if defined(PMAP_NEW)
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_create()\n");
#endif
#else /* ! PMAP_NEW */
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_create(%lx)\n", size);
#endif
	/*
	 * Software use map does not need a pmap
	 */
	if (size)
		return(NULL);
#endif /* PMAP_NEW */

	pmap = pool_get(&pmap_pmap_pool, PR_WAITOK);
	bzero(pmap, sizeof(*pmap));

	pmap->pm_asn = pool_get(&pmap_asn_pool, PR_WAITOK);
	pmap->pm_asngen = pool_get(&pmap_asngen_pool, PR_WAITOK);

	/*
	 * Defer allocation of a new level 1 page table until
	 * the first new mapping is entered; just take a reference
	 * to the kernel kernel_lev1map.
	 */
	pmap->pm_lev1map = kernel_lev1map;

	pmap->pm_count = 1;
	for (i = 0; i < pmap_ncpuids; i++) {
		pmap->pm_asn[i] = PMAP_ASN_RESERVED;
		/* XXX Locking? */
		pmap->pm_asngen[i] = pmap_asn_generation[i];
	}
	simple_lock_init(&pmap->pm_slock);

	simple_lock(&pmap_all_pmaps_slock);
	LIST_INSERT_HEAD(&pmap_all_pmaps, pmap, pm_list);
	simple_unlock(&pmap_all_pmaps_slock);

	return (pmap);
}

/*
 * pmap_destroy:		[ INTERFACE ]
 *
 *	Drop the reference count on the specified pmap, releasing
 *	all resources if the reference count drops to zero.
 */
void
pmap_destroy(pmap)
	pmap_t pmap;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_destroy(%p)\n", pmap);
#endif
	if (pmap == NULL)
		return;

	simple_lock(&pmap->pm_slock);
	if (--pmap->pm_count > 0) {
		simple_unlock(&pmap->pm_slock);
		return;
	}

	/*
	 * Remove it from the global list of all pmaps.
	 */
	simple_lock(&pmap_all_pmaps_slock);
	LIST_REMOVE(pmap, pm_list);
	simple_unlock(&pmap_all_pmaps_slock);

#ifdef DIAGNOSTIC
	/*
	 * Since the pmap is supposed to contain no valid
	 * mappings at this point, this should never happen.
	 */
	if (pmap->pm_lev1map != kernel_lev1map) {
		printf("pmap_release: pmap still contains valid mappings!\n");
		if (pmap->pm_nlev2)
			printf("pmap_release: %ld level 2 tables left\n",
			    pmap->pm_nlev2);
		if (pmap->pm_nlev3)
			printf("pmap_release: %ld level 3 tables left\n",
			    pmap->pm_nlev3);
		pmap_remove(pmap, VM_MIN_ADDRESS, VM_MAX_ADDRESS);
		if (pmap->pm_lev1map != kernel_lev1map)
			panic("pmap_release: pmap_remove() didn't");
	}
#endif

	pool_put(&pmap_asn_pool, pmap->pm_asn);
	pool_put(&pmap_asngen_pool, pmap->pm_asngen);
	pool_put(&pmap_pmap_pool, pmap);
}

/*
 * pmap_reference:		[ INTERFACE ]
 *
 *	Add a reference to the specified pmap.
 */
void
pmap_reference(pmap)
	pmap_t	pmap;
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_reference(%p)\n", pmap);
#endif
	if (pmap != NULL) {
		simple_lock(&pmap->pm_slock);
		pmap->pm_count++;
		simple_unlock(&pmap->pm_slock);
	}
}

/*
 * pmap_remove:			[ INTERFACE ]
 *
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the page size.
 *
 *	XXX This routine could be optimized with regard to its
 *	handling of level 3 PTEs.
 */
void
pmap_remove(pmap, sva, eva)
	pmap_t pmap;
	vaddr_t sva, eva;
{
	pt_entry_t *l1pte, *l2pte, *l3pte;
	boolean_t needisync = FALSE;
	long cpu_id = alpha_pal_whami();

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove(%p, %lx, %lx)\n", pmap, sva, eva);
#endif

	if (pmap == NULL)
		return;

	/*
	 * If this is the kernel pmap, use a faster routine that
	 * can make some assumptions.
	 */
	if (pmap == pmap_kernel()) {
		pmap_kremove(sva, eva - sva);
		return;
	}

#ifdef DIAGNOSTIC
	if (sva > VM_MAXUSER_ADDRESS || eva > VM_MAXUSER_ADDRESS)
		panic("pmap_remove: (%p - %p) user pmap, kernel address range",
			sva, eva);
#endif

	PMAP_MAP_TO_HEAD_LOCK();
	simple_lock(&pmap->pm_slock);

	while (sva < eva) {
		/*
		 * If level 1 mapping is invalid, just skip it.
		 */
		l1pte = pmap_l1pte(pmap, sva);
		if (pmap_pte_v(l1pte) == 0) {
			sva = alpha_trunc_l1seg(sva) + ALPHA_L1SEG_SIZE;
			continue;
		}

		/*
		 * If level 2 mapping is invalid, just skip it.
		 */
		l2pte = pmap_l2pte(pmap, sva, l1pte);
		if (pmap_pte_v(l2pte) == 0) {
			sva = alpha_trunc_l2seg(sva) + ALPHA_L2SEG_SIZE;
			continue;
		}

		/*
		 * Invalidate the mapping if it's valid.
		 */
		l3pte = pmap_l3pte(pmap, sva, l2pte);
		if (pmap_pte_v(l3pte))
			needisync |= pmap_remove_mapping(pmap, sva, l3pte,
			    TRUE, cpu_id);
		sva += PAGE_SIZE;
	}

	if (needisync)
		alpha_pal_imb();

	simple_unlock(&pmap->pm_slock);
	PMAP_MAP_TO_HEAD_UNLOCK();
}

/*
 * pmap_page_protect:		[ INTERFACE ]
 *
 *	Lower the permission for all mappings to a given page to
 *	the permissions specified.
 */
#if defined(PMAP_NEW)
void
pmap_page_protect(pg, prot)
	struct vm_page *pg;
	vm_prot_t prot;
#else
void
pmap_page_protect(pa, prot)
	paddr_t		pa;
	vm_prot_t	prot;
#endif /* PMAP_NEW */
{
	struct pv_head *pvh;
	pv_entry_t pv, nextpv;
	int s;
	boolean_t needisync = FALSE;
	long cpu_id = alpha_pal_whami();
#if defined(PMAP_NEW)
	paddr_t pa = VM_PAGE_TO_PHYS(pg);

#ifdef DEBUG
	if ((pmapdebug & (PDB_FOLLOW|PDB_PROTECT)) ||
	    (prot == VM_PROT_NONE && (pmapdebug & PDB_REMOVE)))
		printf("pmap_page_protect(%p, %x)\n", pg, prot);
#endif

#else /* ! PMAP_NEW */

#ifdef DEBUG
	if ((pmapdebug & (PDB_FOLLOW|PDB_PROTECT)) ||
	    (prot == VM_PROT_NONE && (pmapdebug & PDB_REMOVE)))
		printf("pmap_page_protect(%lx, %x)\n", pa, prot);
#endif
	if (!PAGE_IS_MANAGED(pa))
		return;
#endif /* PMAP_NEW */

	/*
	 * Even though we don't change the mapping of the page,
	 * we still flush the I-cache if VM_PROT_EXECUTE is set
	 * because we might be "adding" execute permissions to
	 * a previously non-execute page.
	 */

	switch (prot) {
	case VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE:
		alpha_pal_imb();
	case VM_PROT_READ|VM_PROT_WRITE:
		return;
	/* copy_on_write */
	case VM_PROT_READ|VM_PROT_EXECUTE:
		alpha_pal_imb();
	case VM_PROT_READ:
		pvh = pa_to_pvh(pa);
		PMAP_HEAD_TO_MAP_LOCK();
		simple_lock(&pvh->pvh_slock);
/* XXX */	pmap_changebit(pa, 0, ~(PG_KWE | PG_UWE), cpu_id);
		simple_unlock(&pvh->pvh_slock);
		PMAP_HEAD_TO_MAP_UNLOCK();
		return;
	/* remove_all */
	default:
		break;
	}

	pvh = pa_to_pvh(pa);
	PMAP_HEAD_TO_MAP_LOCK();
	simple_lock(&pvh->pvh_slock);
	s = splimp();			/* XXX needed w/ PMAP_NEW? */
	for (pv = LIST_FIRST(&pvh->pvh_list); pv != NULL; pv = nextpv) {
		pt_entry_t *pte;

		nextpv = LIST_NEXT(pv, pv_list);

		simple_lock(&pv->pv_pmap->pm_slock);
		pte = pmap_l3pte(pv->pv_pmap, pv->pv_va, NULL);
#ifdef DEBUG
		if (pmap_pte_v(pmap_l2pte(pv->pv_pmap, pv->pv_va, NULL)) == 0 ||
		    pmap_pte_pa(pte) != pa)
			panic("pmap_page_protect: bad mapping");
#endif
		if (!pmap_pte_w(pte))
			needisync |= pmap_remove_mapping(pv->pv_pmap,
			    pv->pv_va, pte, FALSE, cpu_id);
#ifdef DEBUG
		else {
			if (pmapdebug & PDB_PARANOIA) {
				printf("%s wired mapping for %lx not removed\n",
				       "pmap_page_protect:", pa);
				printf("vm wire count %d\n", 
					PHYS_TO_VM_PAGE(pa)->wire_count);
			}
		}
#endif
		simple_unlock(&pv->pv_pmap->pm_slock);
	}
	splx(s);

	if (needisync)
		alpha_pal_imb();

	simple_unlock(&pvh->pvh_slock);
	PMAP_HEAD_TO_MAP_UNLOCK();
}

/*
 * pmap_protect:		[ INTERFACE ]
 *
 *	Set the physical protection on the specified range of this map
 *	as requested.
 */
void
pmap_protect(pmap, sva, eva, prot)
	pmap_t	pmap;
	vaddr_t sva, eva;
	vm_prot_t prot;
{
	pt_entry_t *l1pte, *l2pte, *l3pte, bits;
	boolean_t isactive;
	boolean_t hadasm;
	vaddr_t l1eva, l2eva;
	long cpu_id = alpha_pal_whami();

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT))
		printf("pmap_protect(%p, %lx, %lx, %x)\n",
		    pmap, sva, eva, prot);
#endif

	if (pmap == NULL)
		return;

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}

	if (prot & VM_PROT_WRITE)
		return;

	simple_lock(&pmap->pm_slock);

	bits = pte_prot(pmap, prot);
	isactive = PMAP_ISACTIVE(pmap);

	l1pte = pmap_l1pte(pmap, sva);
	for (; sva < eva;
	     sva = alpha_trunc_l1seg(sva) + ALPHA_L1SEG_SIZE, l1pte++) {
		if (pmap_pte_v(l1pte)) {
			l2pte = pmap_l2pte(pmap, sva, l1pte);
			for (l1eva = alpha_trunc_l1seg(sva) + ALPHA_L1SEG_SIZE;
			     sva < l1eva;
			     sva = alpha_trunc_l2seg(sva) + ALPHA_L2SEG_SIZE,
			      l2pte++) {
				if (pmap_pte_v(l2pte)) {
					l3pte = pmap_l3pte(pmap, sva, l2pte);
					for (l2eva = alpha_trunc_l2seg(sva) +
						 ALPHA_L2SEG_SIZE;
					     sva < l2eva;
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
						}
					}
				}
			}
		}
	}

	if (isactive && (prot & VM_PROT_EXECUTE) != 0)
		alpha_pal_imb();

	simple_unlock(&pmap->pm_slock);
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
void
pmap_enter(pmap, va, pa, prot, wired)
	pmap_t pmap;
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
	boolean_t wired;
{
	boolean_t managed;
	pt_entry_t *pte, npte;
	paddr_t opa;
	boolean_t tflush = TRUE;
	boolean_t hadasm = FALSE;	/* XXX gcc -Wuninitialized */
	boolean_t needisync;
	boolean_t isactive;
	long cpu_id = alpha_pal_whami();

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_enter(%p, %lx, %lx, %x, %x)\n",
		       pmap, va, pa, prot, wired);
#endif
	if (pmap == NULL)
		return;

	managed = PAGE_IS_MANAGED(pa);
	isactive = PMAP_ISACTIVE(pmap);
	needisync = isactive && (prot & VM_PROT_EXECUTE) != 0;

	PMAP_MAP_TO_HEAD_LOCK();
	simple_lock(&pmap->pm_slock);

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

		/*
		 * If we're still referencing the kernel kernel_lev1map,
		 * create a new level 1 page table.  A reference will be
		 * added to the level 1 table when the level 2 table is
		 * created.
		 */
		if (pmap->pm_lev1map == kernel_lev1map)
			pmap_lev1map_create(pmap, cpu_id);

		/*
		 * Check to see if the level 1 PTE is valid, and
		 * allocate a new level 2 page table page if it's not.
		 * A reference will be added to the level 2 table when
		 * the level 3 table is created.
		 */
		l1pte = pmap_l1pte(pmap, va);
		if (pmap_pte_v(l1pte) == 0) {
			pmap_ptpage_alloc(pmap, l1pte, PGU_L2PT);
			pmap_physpage_addref(l1pte);
			pmap->pm_nlev2++;
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
			pmap_ptpage_alloc(pmap, l2pte, PGU_L3PT);
			pmap_physpage_addref(l2pte);
			pmap->pm_nlev3++;
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
		tflush = FALSE;

		/*
		 * No need to synchronize the I-stream, either, for basically
		 * the same reason.
		 */
		needisync = FALSE;

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
				pmap->pm_stats.wired_count++;
			else
				pmap->pm_stats.wired_count--;
		}

		/*
		 * Check to see if the PALcode portion of the
		 * PTE is the same.  If so, no TLB invalidation
		 * is necessary.
		 */
		if (PG_PALCODE(pmap_pte_prot(pte)) ==
		    PG_PALCODE(pte_prot(pmap, prot)))
			tflush = FALSE;

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
	needisync |= pmap_remove_mapping(pmap, va, pte, TRUE, cpu_id);

 validate_enterpv:
	/*
	 * Enter the mapping into the pv_table if appropriate.
	 */
	if (managed) {
		int s = splimp();	/* XXX needed w/ PMAP_NEW? */
		pmap_pv_enter(pmap, pa, va, TRUE);
		splx(s);
	}

	/*
	 * Increment counters.
	 */
	pmap->pm_stats.resident_count++;
	if (wired)
		pmap->pm_stats.wired_count++;

 validate:
	/*
	 * Build the new PTE.
	 */
	npte = ((pa >> PGSHIFT) << PG_SHIFT) | pte_prot(pmap, prot) | PG_V;
	if (managed) {
		struct pv_head *pvh = pa_to_pvh(pa);

		/*
		 * Set up referenced/modified emulation for new mapping.
		 */
		if ((pvh->pvh_attrs & PGA_REFERENCED) == 0)
			npte |= PG_FOR | PG_FOW | PG_FOE;
		else if ((pvh->pvh_attrs & PGA_MODIFIED) == 0)
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
	 * Set the new PTE.
	 */
	*pte = npte;

	/*
	 * Invalidate the TLB entry for this VA and any appropriate
	 * caches.
	 */
	if (tflush)
		PMAP_INVALIDATE_TLB(pmap, va, hadasm, isactive, cpu_id);
	if (needisync)
		alpha_pal_imb();

	simple_unlock(&pmap->pm_slock);
	PMAP_MAP_TO_HEAD_UNLOCK();
}

#if defined(PMAP_NEW)
/*
 * pmap_kenter_pa:		[ INTERFACE ]
 *
 *	Enter a va -> pa mapping into the kernel pmap without any
 *	physical->virtual tracking.
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_kenter_pa(va, pa, prot)
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
{
	pt_entry_t *pte, npte;
	long cpu_id = alpha_pal_whami();

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

	/*
	 * Build the new PTE.
	 */
	npte = ((pa >> PGSHIFT) << PG_SHIFT) | pte_prot(pmap_kernel(), prot) |
	    PG_V | PG_WIRED;

	/*
	 * Set the new PTE.
	 */
	*pte = npte;

	/*
	 * Invalidate the TLB entry for this VA and any appropriate
	 * caches.
	 */
	PMAP_INVALIDATE_TLB(pmap_kernel(), va, TRUE, TRUE, cpu_id);
	if (prot & VM_PROT_EXECUTE)
		alpha_pal_imb();
}

/*
 * pmap_kenter_pgs:		[ INTERFACE ]
 *
 *	Enter a va -> pa mapping for the array of vm_page's into the
 *	kernel pmap without any physical->virtual tracking, starting
 *	at address va, for npgs pages.
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_kenter_pgs(va, pgs, npgs)
	vaddr_t va;
	vm_page_t *pgs;
	int npgs;
{
	int i;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_kenter_pgs(%lx, %p, %d)\n",
		    va, pgs, npgs);
#endif

	for (i = 0; i < npgs; i++)
		pmap_kenter_pa(va + (PAGE_SIZE * i),
		    VM_PAGE_TO_PHYS(pgs[i]),
		    VM_PROT_READ|VM_PROT_WRITE);
}
#endif /* PMAP_NEW */

/*
 * pmap_kremove:		[ INTERFACE ]
 *
 *	Remove a mapping entered with pmap_kenter_pa() or pmap_kenter_pgs()
 *	starting at va, for size bytes (assumed to be page rounded).
 *
 *	NOTE: THIS IS AN INTERNAL FUNCTION IF NOT PMAP_NEW.
 */
void
pmap_kremove(va, size)
	vaddr_t va;
	vsize_t size;
{
	pt_entry_t *pte;
	boolean_t needisync = FALSE;
	long cpu_id = alpha_pal_whami();

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_kremove(%lx, %lx)\n",
		    va, size);
#endif

#ifdef DIAGNOSTIC
	if (va < VM_MIN_KERNEL_ADDRESS)
		panic("pmap_kremove: user address");
#endif

	PMAP_MAP_TO_HEAD_LOCK();
	simple_lock(&pmap_kernel()->pm_slock);

	for (; size != 0; size -= PAGE_SIZE, va += PAGE_SIZE) {
		pte = PMAP_KERNEL_PTE(va);
		if (pmap_pte_v(pte))
			needisync |= pmap_remove_mapping(pmap_kernel(), va,
			    pte, TRUE, cpu_id);
	}

	if (needisync)
		alpha_pal_imb();

	simple_unlock(&pmap_kernel()->pm_slock);
	PMAP_MAP_TO_HEAD_UNLOCK();
}

/*
 * pmap_change_wiring:		[ INTERFACE ]
 *
 *	Change the wiring attribute for a map/virtual-address pair.
 *
 *	The mapping must already exist in the pmap.
 */
void
pmap_change_wiring(pmap, va, wired)
	pmap_t		pmap;
	vaddr_t		va;
	boolean_t	wired;
{
	pt_entry_t *pte;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_change_wiring(%p, %lx, %x)\n", pmap, va, wired);
#endif
	if (pmap == NULL)
		return;

	simple_lock(&pmap->pm_slock);

	pte = pmap_l3pte(pmap, va, NULL);
#ifdef DEBUG
	/*
	 * Page table page is not allocated.
	 * Should this ever happen?  Ignore it for now,
	 * we don't want to force allocation of unnecessary PTE pages.
	 */
	if (pmap_pte_v(pmap_l2pte(pmap, va, NULL)) == 0) {
		if (pmapdebug & PDB_PARANOIA)
			printf("pmap_change_wiring: invalid STE for %lx\n", va);
		return;
	}
	/*
	 * Page not valid.  Should this ever happen?
	 * Just continue and change wiring anyway.
	 */
	if (!pmap_pte_v(pte)) {
		if (pmapdebug & PDB_PARANOIA)
			printf("pmap_change_wiring: invalid PTE for %lx\n", va);
	}
#endif
	/*
	 * If wiring actually changed (always?) set the wire bit and
	 * update the wire count.  Note that wiring is not a hardware
	 * characteristic so there is no need to invalidate the TLB.
	 */
	if (pmap_pte_w_chg(pte, wired ? PG_WIRED : 0)) {
		pmap_pte_set_w(pte, wired);
		if (wired)
			pmap->pm_stats.wired_count++;
		else
			pmap->pm_stats.wired_count--;
	}

	simple_unlock(&pmap->pm_slock);
}

/*
 * pmap_extract:		[ INTERFACE ]
 *
 *	Extract the physical address associated with the given
 *	pmap/virtual address pair.
 */
paddr_t
pmap_extract(pmap, va)
	pmap_t	pmap;
	vaddr_t va;
{
	pt_entry_t *l1pte, *l2pte, *l3pte;
	paddr_t pa;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_extract(%p, %lx) -> ", pmap, va);
#endif
	pa = 0;

	simple_lock(&pmap->pm_slock);

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

 out:
	simple_unlock(&pmap->pm_slock);
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("0x%lx\n", pa);
#endif
	return (pa);
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
void
pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	pmap_t		dst_pmap;
	pmap_t		src_pmap;
	vaddr_t		dst_addr;
	vsize_t		len;
	vaddr_t		src_addr;
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy(%p, %p, %lx, %lx, %lx)\n",
		       dst_pmap, src_pmap, dst_addr, len, src_addr);
#endif
}

/*
 * pmap_update:			[ INTERFACE ]
 *
 *	Require that all active physical maps contain no
 *	incorrect entries NOW, by processing any deferred
 *	pmap operations.
 */
void
pmap_update()
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_update()\n");
#endif

	/*
	 * Nothing to do; this pmap module does not defer any operations.
	 */
}

/*
 * pmap_collect:		[ INTERFACE ]
 *
 *	Garbage collects the physical map system for pages which are no
 *	longer used.  Success need not be guaranteed -- that is, there
 *	may well be pages which are not referenced, but others may be
 *	collected.
 *
 *	Called by the pageout daemon when pages are scarce.
 */
void
pmap_collect(pmap)
	pmap_t		pmap;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_collect(%p)\n", pmap);
#endif

	/*
	 * This process is about to be swapped out; free all of
	 * the PT pages by removing the physical mappings for its
	 * entire address space.  Note: pmap_remove() performs
	 * all necessary locking.
	 */
	pmap_remove(pmap, VM_MIN_ADDRESS, VM_MAX_ADDRESS);

#ifdef notyet
	/* Go compact and garbage-collect the pv_table. */
	pmap_pv_collect();
#endif
}

/*
 * pmap_activate:		[ INTERFACE ]
 *
 *	Activate the pmap used by the specified process.  This includes
 *	reloading the MMU context if the current process, and marking
 *	the pmap in use by the processor.
 *
 *	Note: We may use only spin locks here, since we are called
 *	by a critical section in cpu_switch()!
 */
void
pmap_activate(p)
	struct proc *p;
{
	struct pmap *pmap = p->p_vmspace->vm_map.pmap;
	long cpu_id = alpha_pal_whami();

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_activate(%p)\n", p);
#endif

	simple_lock(&pmap->pm_slock);

	/*
	 * Allocate an ASN.
	 */
	pmap_asn_alloc(pmap, cpu_id);

	/*
	 * Mark the pmap in use by this processor.
	 */
	alpha_atomic_setbits_q(&pmap->pm_cpus, (1UL << cpu_id));

	PMAP_ACTIVATE(pmap, p, cpu_id);

	simple_unlock(&pmap->pm_slock);
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
pmap_deactivate(p)
	struct proc *p;
{
	struct pmap *pmap = p->p_vmspace->vm_map.pmap;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_deactivate(%p)\n", p);
#endif

	/*
	 * Mark the pmap no longer in use by this processor.
	 */
	alpha_atomic_clearbits_q(&pmap->pm_cpus, (1UL << alpha_pal_whami()));
}

/*
 * pmap_zero_page:		[ INTERFACE ]
 *
 *	Zero the specified (machine independent) page by mapping the page
 *	into virtual memory and using bzero to clear its contents, one
 *	machine dependent page at a time.
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_zero_page(phys)
	paddr_t phys;
{
	caddr_t p;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_zero_page(%lx)\n", phys);
#endif
	p = (caddr_t)ALPHA_PHYS_TO_K0SEG(phys);
	bzero(p, PAGE_SIZE);
}

/*
 * pmap_copy_page:		[ INTERFACE ]
 *
 *	Copy the specified (machine independent) page by mapping the page
 *	into virtual memory and using bcopy to copy the page, one machine
 *	dependent page at a time.
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_copy_page(src, dst)
	paddr_t src, dst;
{
	caddr_t s, d;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy_page(%lx, %lx)\n", src, dst);
#endif
        s = (caddr_t)ALPHA_PHYS_TO_K0SEG(src);
        d = (caddr_t)ALPHA_PHYS_TO_K0SEG(dst);
	bcopy(s, d, PAGE_SIZE);
}

/*
 * pmap_pageable:		[ INTERFACE ]
 *
 *	Make the specified pages (by pmap, offset) pageable (or not) as
 *	requested.
 *
 *	A page which is not pageable may not take a fault; therefore,
 *	its page table entry must remain valid for the duration.
 *
 *	This routine is merely advisory; pmap_enter() will specify that
 *	these pages are to be wired down (or not) as appropriate.
 */
void
pmap_pageable(pmap, sva, eva, pageable)
	pmap_t		pmap;
	vaddr_t		sva, eva;
	boolean_t	pageable;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_pageable(%p, %lx, %lx, %x)\n",
		       pmap, sva, eva, pageable);
#endif
}

/*
 * pmap_clear_modify:		[ INTERFACE ]
 *
 *	Clear the modify bits on the specified physical page.
 */
#if defined(PMAP_NEW)
boolean_t
pmap_clear_modify(pg)
	struct vm_page *pg;
{
	struct pv_head *pvh;
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	boolean_t rv = FALSE;
	long cpu_id = alpha_pal_whami();

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_modify(%p)\n", pg);
#endif

	pvh = pa_to_pvh(pa);

	PMAP_HEAD_TO_MAP_LOCK();
	simple_lock(&pvh->pvh_slock);

	if (pvh->pvh_attrs & PGA_MODIFIED) {
		rv = TRUE;
		pmap_changebit(pa, PG_FOW, ~0, cpu_id);
		pvh->pvh_attrs &= ~PGA_MODIFIED;
	}

	simple_unlock(&pvh->pvh_slock);
	PMAP_HEAD_TO_MAP_UNLOCK();

	return (rv);
}
#else /* ! PMAP_NEW */
void
pmap_clear_modify(pa)
	paddr_t	pa;
{
	struct pv_head *pvh;
	long cpu_id = alpha_pal_whami();

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_modify(%lx)\n", pa);
#endif
	if (!PAGE_IS_MANAGED(pa))		/* XXX why not panic? */
		return;

	pvh = pa_to_pvh(pa);

	PMAP_HEAD_TO_MAP_LOCK();
	simple_lock(&pvh->pvh_slock);

	if (pvh->pvh_attrs & PGA_MODIFIED) {
		pmap_changebut(pa, PG_FOW, ~0);
		pvh->pvh_attrs &= ~PGA_MODIFIED;
	}

	simple_unlock(&pvh->pvh_slock);
	PMAP_HEAD_TO_MAP_UNLOCK();
}
#endif /* PMAP_NEW */

/*
 * pmap_clear_reference:	[ INTERFACE ]
 *
 *	Clear the reference bit on the specified physical page.
 */
#if defined(PMAP_NEW)
boolean_t
pmap_clear_reference(pg)
	struct vm_page *pg;
{
	struct pv_head *pvh;
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	boolean_t rv = FALSE;
	long cpu_id = alpha_pal_whami();

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_reference(%p)\n", pg);
#endif

	pvh = pa_to_pvh(pa);

	PMAP_HEAD_TO_MAP_LOCK();
	simple_lock(&pvh->pvh_slock);

	if (pvh->pvh_attrs & PGA_REFERENCED) {
		rv = TRUE;
		pmap_changebit(pa, PG_FOR | PG_FOW | PG_FOE, ~0, cpu_id);
		pvh->pvh_attrs &= ~PGA_REFERENCED;
	}

	simple_unlock(&pvh->pvh_slock);
	PMAP_HEAD_TO_MAP_UNLOCK();

	return (rv);
}
#else /* ! PMAP_NEW */
void
pmap_clear_reference(pa)
	paddr_t	pa;
{
	struct pv_head *pvh;
	long cpu_id = alpha_pal_whami();

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_reference(%lx)\n", pa);
#endif
	if (!PAGE_IS_MANAGED(pa))		/* XXX why not panic? */
		return;

	pvh = pa_to_pvh(pa);

	PMAP_HEAD_TO_MAP_LOCK();
	simple_lock(&pvh->pvh_slock);

	if (pvh->pvh_attrs & PGA_REFERENCED) {
		pmap_changebit(pa, PG_FOR | PG_FOW | PG_FOE, ~0, cpu_id);
		pvh->pvh_attrs &= ~PGA_REFERENCED;
	}

	simple_unlock(&pvh->pvh_slock);
	PMAP_HEAD_TO_MAP_UNLOCK();
}
#endif /* PMAP_NEW */

/*
 * pmap_is_referenced:		[ INTERFACE ]
 *
 *	Return whether or not the specified physical page is referenced
 *	by any physical maps.
 */
#if defined(PMAP_NEW)
boolean_t
pmap_is_referenced(pg)
	struct vm_page *pg;
{
	struct pv_head *pvh;
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	boolean_t rv;

	pvh = pa_to_pvh(pa);
	simple_lock(&pvh->pvh_slock);
	rv = ((pvh->pvh_attrs & PGA_REFERENCED) != 0);
	simple_unlock(&pvh->pvh_slock);
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW) {
		printf("pmap_is_referenced(%p) -> %c\n", pg, "FT"[rv]);
	}
#endif
	return (rv);
}
#else /* ! PMAP_NEW */
boolean_t
pmap_is_referenced(pa)
	paddr_t	pa;
{
	struct pv_head *pvh;
	boolean_t rv;

	if (!PAGE_IS_MANAGED(pa))		/* XXX why not panic? */
		return 0;

	pvh = pa_to_pvh(pa);
	simple_lock(&pvh->pvh_slock);
	rv = ((pvh->pvh_attrs & PGA_REFERENCED) != 0);
	simple_unlock(&pvh->pvh_slock);
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW) {
		printf("pmap_is_referenced(%lx) -> %c\n", pa, "FT"[rv]);
	}
#endif
	return rv;
}
#endif /* PMAP_NEW */

/*
 * pmap_is_modified:		[ INTERFACE ]
 *
 *	Return whether or not the specified physical page is modified
 *	by any physical maps.
 */
#if defined(PMAP_NEW)
boolean_t
pmap_is_modified(pg)
	struct vm_page *pg;
{
	struct pv_head *pvh;
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	boolean_t rv;

	pvh = pa_to_pvh(pa);
	simple_lock(&pvh->pvh_slock);
	rv = ((pvh->pvh_attrs & PGA_MODIFIED) != 0);
	simple_unlock(&pvh->pvh_slock);
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW) {
		printf("pmap_is_modified(%p) -> %c\n", pg, "FT"[rv]);
	}
#endif
	return (rv);
}
#else /* ! PMAP_NEW */
boolean_t
pmap_is_modified(pa)
	paddr_t	pa;
{
	struct pv_head *pvh;
	boolean_t rv;

	if (!PAGE_IS_MANAGED(pa))		/* XXX why not panic? */
		return 0;

	pvh = pa_to_pvh(pa);
	simple_lock(&pvh->pvh_slock);
	rv = ((pvh->pvh_attrs & PGA_MODIFIED) != 0);
	simple_unlock(&pvh->pvh_slock);
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW) {
		printf("pmap_is_modified(%lx) -> %c\n", pa, "FT"[rv]);
	}
#endif
	return rv;
}
#endif /* PMAP_NEW */

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
pmap_phys_address(ppn)
	int ppn;
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
void
alpha_protection_init()
{
	int prot, *kp, *up;

	kp = protection_codes[0];
	up = protection_codes[1];

	for (prot = 0; prot < 8; prot++) {
		kp[prot] = 0; up[prot] = 0;
		switch (prot) {
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_NONE:
			kp[prot] |= PG_ASM;
			up[prot] |= 0;
			break;

		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_EXECUTE:
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_EXECUTE:
			kp[prot] |= PG_EXEC;		/* software */
			up[prot] |= PG_EXEC;		/* software */
			/* FALLTHROUGH */

		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_NONE:
			kp[prot] |= PG_ASM | PG_KRE;
			up[prot] |= PG_URE | PG_KRE;
			break;

		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE:
			kp[prot] |= PG_ASM | PG_KWE;
			up[prot] |= PG_UWE | PG_KWE;
			break;

		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_EXECUTE:
		case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE:
			kp[prot] |= PG_EXEC;		/* software */
			up[prot] |= PG_EXEC;		/* software */
			/* FALLTHROUGH */

		case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NONE:
			kp[prot] |= PG_ASM | PG_KWE | PG_KRE;
			up[prot] |= PG_UWE | PG_URE | PG_KWE | PG_KRE;
			break;
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
 *	Returns TRUE or FALSE, indicating if the I-stream needs to
 *	be synchronized.
 */
boolean_t
pmap_remove_mapping(pmap, va, pte, dolock, cpu_id)
	pmap_t pmap;
	vaddr_t va;
	pt_entry_t *pte;
	boolean_t dolock;
	long cpu_id;
{
	paddr_t pa;
	int s;
	boolean_t onpv;
	boolean_t hadasm;
	boolean_t isactive;
	boolean_t needisync;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove_mapping(%p, %lx, %p, %d, %ld)\n",
		       pmap, va, pte, dolock, cpu_id);
#endif

	/*
	 * PTE not provided, compute it from pmap and va.
	 */
	if (pte == PT_ENTRY_NULL) {
		pte = pmap_l3pte(pmap, va, NULL);
		if (pmap_pte_v(pte) == 0)
			return (FALSE);
	}

	pa = pmap_pte_pa(pte);
	onpv = (pmap_pte_pv(pte) != 0);
	hadasm = (pmap_pte_asm(pte) != 0);
	isactive = PMAP_ISACTIVE(pmap);
	needisync = isactive && (pmap_pte_exec(pte) != 0);

	/*
	 * Update statistics
	 */
	if (pmap_pte_w(pte))
		pmap->pm_stats.wired_count--;
	pmap->pm_stats.resident_count--;

	/*
	 * Invalidate the PTE after saving the reference modify info.
	 */
#ifdef DEBUG
	if (pmapdebug & PDB_REMOVE)
		printf("remove: invalidating pte at %p\n", pte);
#endif
	*pte = PG_NV;

	PMAP_INVALIDATE_TLB(pmap, va, hadasm, isactive, cpu_id);

	/*
	 * If we're removing a user mapping, check to see if we
	 * can free page table pages.
	 */
	if (pmap != pmap_kernel()) {
		pt_entry_t *l1pte, *l2pte;

		l1pte = pmap_l1pte(pmap, va);
		l2pte = pmap_l2pte(pmap, va, l1pte);

		/*
		 * Delete the reference on the level 3 table.  It will
		 * delete references on the level 2 and 1 tables as
		 * appropriate.
		 */
		pmap_l3pt_delref(pmap, va, l1pte, l2pte, pte, cpu_id);
	}

	/*
	 * If the mapping wasn't enterd on the PV list, we're all done.
	 */
	if (onpv == FALSE)
		return (needisync);

	/*
	 * Otherwise remove it from the PV table
	 * (raise IPL since we may be called at interrupt time).
	 */
	s = splimp();		/* XXX needed w/ PMAP_NEW? */
	pmap_pv_remove(pmap, pa, va, dolock);
	splx(s);

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
void
pmap_changebit(pa, set, mask, cpu_id)
	paddr_t pa;
	u_long set, mask;
	long cpu_id;
{
	struct pv_head *pvh;
	pv_entry_t pv;
	pt_entry_t *pte, npte;
	vaddr_t va;
	boolean_t hadasm, isactive;
	boolean_t needisync = FALSE;
	int s;

#ifdef DEBUG
	if (pmapdebug & PDB_BITS)
		printf("pmap_changebit(0x%lx, 0x%lx, 0x%lx)\n",
		    pa, set, mask);
#endif
	if (!PAGE_IS_MANAGED(pa))
		return;

	pvh = pa_to_pvh(pa);
	s = splimp();			/* XXX needed w/ PMAP_NEW? */
	/*
	 * Loop over all current mappings setting/clearing as appropos.
	 */
	for (pv = LIST_FIRST(&pvh->pvh_list); pv != NULL;
	     pv = LIST_NEXT(pv, pv_list)) {
		va = pv->pv_va;

		/*
		 * XXX don't write protect pager mappings
		 */
/* XXX */	if (mask == ~(PG_KWE | PG_UWE)) {
#if defined(UVM)
			if (va >= uvm.pager_sva && va < uvm.pager_eva)
				continue;
#else
			extern vaddr_t pager_sva, pager_eva;

			if (va >= pager_sva && va < pager_eva)
				continue;
#endif
		}

		simple_lock(&pv->pv_pmap->pm_slock);

		pte = pmap_l3pte(pv->pv_pmap, va, NULL);
		npte = (*pte | set) & mask;
		if (*pte != npte) {
			hadasm = (pmap_pte_asm(pte) != 0);
			isactive = PMAP_ISACTIVE(pv->pv_pmap);
			needisync |= (isactive && (pmap_pte_exec(pte) != 0));
			*pte = npte;
			PMAP_INVALIDATE_TLB(pv->pv_pmap, va, hadasm, isactive,
			    cpu_id);
		}
		simple_unlock(&pv->pv_pmap->pm_slock);
	}
	splx(s);

	if (needisync)
		alpha_pal_imb();
}

/*
 * pmap_emulate_reference:
 *
 *	Emulate reference and/or modified bit hits.
 */
void
pmap_emulate_reference(p, v, user, write)
	struct proc *p;
	vaddr_t v;
	int user;
	int write;
{
	pt_entry_t faultoff, *pte;
	paddr_t pa;
	struct pv_head *pvh;
	boolean_t didlock = FALSE;
	long cpu_id = alpha_pal_whami();

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_emulate_reference: %p, 0x%lx, %d, %d\n",
		    p, v, user, write);
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
		if (p == NULL)
			panic("pmap_emulate_reference: bad proc");
		if (p->p_vmspace == NULL)
			panic("pmap_emulate_reference: bad p_vmspace");
#endif
		simple_lock(&p->p_vmspace->vm_map.pmap->pm_slock);
		didlock = TRUE;
		pte = pmap_l3pte(p->p_vmspace->vm_map.pmap, v, NULL);
		/*
		 * We'll unlock below where we're done with the PTE.
		 */
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
#if 0
	/*
	 * Can't do these, because cpu_fork and cpu_swapin call
	 * pmap_emulate_reference(), and the bits aren't guaranteed,
	 * for them...
	 */
	if (write) {
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
#endif
	/* Other diagnostics? */
#endif
	pa = pmap_pte_pa(pte);

	/*
	 * We're now done with the PTE.  If it was a user pmap, unlock
	 * it now.
	 */
	if (didlock)
		simple_unlock(&p->p_vmspace->vm_map.pmap->pm_slock);

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("\tpa = 0x%lx\n", pa);
#endif
#ifdef DIAGNOSTIC
	if (!PAGE_IS_MANAGED(pa))
		panic("pmap_emulate_reference(%p, 0x%lx, %d, %d): pa 0x%lx not managed", p, v, user, write, pa);
#endif

	/*
	 * Twiddle the appropriate bits to reflect the reference
	 * and/or modification..
	 *
	 * The rules:
	 * 	(1) always mark page as used, and
	 *	(2) if it was a write fault, mark page as modified.
	 */
	pvh = pa_to_pvh(pa);

	PMAP_HEAD_TO_MAP_LOCK();
	simple_lock(&pvh->pvh_slock);

	pvh->pvh_attrs = PGA_REFERENCED;
	faultoff = PG_FOR | PG_FOE;
	if (write) {
		pvh->pvh_attrs |= PGA_MODIFIED;
		faultoff |= PG_FOW;
	}
	pmap_changebit(pa, 0, ~faultoff, cpu_id);

	simple_unlock(&pvh->pvh_slock);
	PMAP_HEAD_TO_MAP_UNLOCK();

	/* XXX XXX XXX This needs to go away! XXX XXX XXX */
	/* because: pte/pmap is unlocked now */
	if ((*pte & faultoff) != 0) {
#if defined(UVM)
		/*
		 * This is apparently normal.  Why? -- cgd
		 * XXX because was being called on unmanaged pages?
		 */
		printf("warning: pmap_changebit didn't.");
#endif
		*pte &= ~faultoff;
		ALPHA_TBIS(v);
	}
}

#ifdef DEBUG
/*
 * pmap_pv_dump:
 *
 *	Dump the physical->virtual data for the specified page.
 */
void
pmap_pv_dump(pa)
	paddr_t pa;
{
	struct pv_head *pvh;
	pv_entry_t pv;
	static const char *usage[] = {
		"normal", "pvent", "l1pt", "l2pt", "l3pt",
	};

	pvh = pa_to_pvh(pa);

	simple_lock(&pvh->pvh_slock);

	printf("pa 0x%lx (attrs = 0x%x, usage = " /* ) */, pa, pvh->pvh_attrs);
	if (pvh->pvh_usage < PGU_NORMAL || pvh->pvh_usage > PGU_L3PT)
/* ( */		printf("??? %d):\n", pvh->pvh_usage);
	else
/* ( */		printf("%s):\n", usage[pvh->pvh_usage]);

	for (pv = LIST_FIRST(&pvh->pvh_list); pv != NULL;
	     pv = LIST_NEXT(pv, pv_list))
		printf("     pmap %p, va 0x%lx\n",
		    pv->pv_pmap, pv->pv_va);
	printf("\n");

	simple_unlock(&pvh->pvh_slock);
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
vtophys(vaddr)
	vaddr_t vaddr;
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
void
pmap_pv_enter(pmap, pa, va, dolock)
	pmap_t pmap;
	paddr_t pa;
	vaddr_t va;
	boolean_t dolock;
{
	struct pv_head *pvh;
	pv_entry_t newpv;

	/*
	 * Allocate and fill in the new pv_entry.
	 */
	newpv = pmap_pv_alloc();
	newpv->pv_va = va;
	newpv->pv_pmap = pmap;

	pvh = pa_to_pvh(pa);

	if (dolock)
		simple_lock(&pvh->pvh_slock);

#ifdef DEBUG
	{
	pv_entry_t pv;
	/*
	 * Make sure the entry doesn't already exist.
	 */
	for (pv = LIST_FIRST(&pvh->pvh_list); pv != NULL;
	     pv = LIST_NEXT(pv, pv_list))
		if (pmap == pv->pv_pmap && va == pv->pv_va) {
			printf("pmap = %p, va = 0x%lx\n", pmap, va);
			panic("pmap_pv_enter: already in pv table");
		}
	}
#endif

	/*
	 * ...and put it in the list.
	 */
	LIST_INSERT_HEAD(&pvh->pvh_list, newpv, pv_list);

	if (dolock)
		simple_unlock(&pvh->pvh_slock);
}

/*
 * pmap_pv_remove:
 *
 *	Remove a physical->virtual entry from the pv_table.
 */
void
pmap_pv_remove(pmap, pa, va, dolock)
	pmap_t pmap;
	paddr_t pa;
	vaddr_t va;
	boolean_t dolock;
{
	struct pv_head *pvh;
	pv_entry_t pv;

	pvh = pa_to_pvh(pa);

	if (dolock)
		simple_lock(&pvh->pvh_slock);

	/*
	 * Find the entry to remove.
	 */
	for (pv = LIST_FIRST(&pvh->pvh_list); pv != NULL;
	     pv = LIST_NEXT(pv, pv_list))
		if (pmap == pv->pv_pmap && va == pv->pv_va)
			break;

#ifdef DEBUG
	if (pv == NULL)
		panic("pmap_pv_remove: not in pv table");
#endif

	LIST_REMOVE(pv, pv_list);

	if (dolock)
		simple_unlock(&pvh->pvh_slock);

	/*
	 * ...and free the pv_entry.
	 */
	pmap_pv_free(pv);
}

/*
 * pmap_pv_alloc:
 *
 *	Allocate a pv_entry.
 */
struct pv_entry *
pmap_pv_alloc()
{
	paddr_t pvppa;
	struct pv_page *pvp;
	struct pv_entry *pv;
	int i;

	simple_lock(&pmap_pvalloc_slock);

	if (pv_nfree == 0) {
		/*
		 * No free pv_entry's; allocate a new pv_page.
		 */
		pvppa = pmap_physpage_alloc(PGU_PVENT);
		pvp = (struct pv_page *)ALPHA_PHYS_TO_K0SEG(pvppa);
		LIST_INIT(&pvp->pvp_pgi.pgi_freelist);
		for (i = 0; i < pmap_npvppg; i++)
			LIST_INSERT_HEAD(&pvp->pvp_pgi.pgi_freelist,
			    &pvp->pvp_pv[i], pv_list);
		pv_nfree += pvp->pvp_pgi.pgi_nfree = pmap_npvppg;
		TAILQ_INSERT_HEAD(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
	}

	--pv_nfree;
	pvp = TAILQ_FIRST(&pv_page_freelist);
	if (--pvp->pvp_pgi.pgi_nfree == 0)
		TAILQ_REMOVE(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
	pv = LIST_FIRST(&pvp->pvp_pgi.pgi_freelist);
#ifdef DIAGNOSTIC
	if (pv == NULL)
		panic("pmap_pv_alloc: pgi_nfree inconsistent");
#endif
	LIST_REMOVE(pv, pv_list);

	simple_unlock(&pmap_pvalloc_slock);

	return (pv);
}

/*
 * pmap_pv_free:
 *
 *	Free a pv_entry.
 */
void
pmap_pv_free(pv)
	struct pv_entry *pv;
{
	paddr_t pvppa;
	struct pv_page *pvp;
	int nfree;

	simple_lock(&pmap_pvalloc_slock);

	pvp = (struct pv_page *) trunc_page(pv);
	nfree = ++pvp->pvp_pgi.pgi_nfree;

	if (nfree == pmap_npvppg) {
		/*
		 * We've freed all of the pv_entry's in this pv_page.
		 * Free the pv_page back to the system.
		 */
		pv_nfree -= pmap_npvppg - 1;
		TAILQ_REMOVE(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
		pvppa = ALPHA_K0SEG_TO_PHYS((vaddr_t)pvp);
		pmap_physpage_free(pvppa);
	} else {
		if (nfree == 1)
			TAILQ_INSERT_TAIL(&pv_page_freelist, pvp,
			    pvp_pgi.pgi_list);
		LIST_INSERT_HEAD(&pvp->pvp_pgi.pgi_freelist, pv, pv_list);
		++pv_nfree;
	}

	simple_unlock(&pmap_pvalloc_slock);
}

/*
 * pmap_pv_collect:
 *
 *	Compact the pv_table and release any completely free
 *	pv_page's back to the system.
 */
void
pmap_pv_collect()
{
	struct pv_page_list pv_page_collectlist;
	struct pv_page *pvp, *npvp;
	struct pv_head *pvh;
	struct pv_entry *pv, *nextpv, *newpv;
	paddr_t pvppa;
	int s;

	TAILQ_INIT(&pv_page_collectlist);

	simple_lock(&pmap_pvalloc_slock);

	/*
	 * Find pv_page's that have more than 1/3 of their pv_entry's free.
	 * Remove them from the list of available pv_page's and place them
	 * on the collection list.
	 */
	for (pvp = TAILQ_FIRST(&pv_page_freelist); pvp != NULL; pvp = npvp) {
		/*
		 * Abort if we can't allocate enough new pv_entry's
		 * to clean up a pv_page.
		 */
		if (pv_nfree < pmap_npvppg)
			break;
		npvp = TAILQ_NEXT(pvp, pvp_pgi.pgi_list);
		if (pvp->pvp_pgi.pgi_nfree > pmap_npvppg / 3) {
			TAILQ_REMOVE(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
			TAILQ_INSERT_TAIL(&pv_page_collectlist, pvp,
			    pvp_pgi.pgi_list);
			pv_nfree -= pvp->pvp_pgi.pgi_nfree;
			pvp->pvp_pgi.pgi_nfree = -1;
		}
	}

	if (TAILQ_FIRST(&pv_page_collectlist) == NULL) {
		simple_unlock(&pmap_pvalloc_slock);
		return;
	}

	/*
	 * Now, scan all pv_entry's, looking for ones which live in one
	 * of the condemed pv_page's.
	 */
	for (pvh = &pv_table[pv_table_npages - 1]; pvh >= &pv_table[0]; pvh--) {
		if (LIST_FIRST(&pvh->pvh_list) == NULL)
			continue;
		s = splimp();		/* XXX needed w/ PMAP_NEW? */
		for (pv = LIST_FIRST(&pvh->pvh_list); pv != NULL;
		     pv = nextpv) {
			nextpv = LIST_NEXT(pv, pv_list);
			pvp = (struct pv_page *) trunc_page(pv);
			if (pvp->pvp_pgi.pgi_nfree == -1) {
				/*
				 * Found one!  Grab a new pv_entry from
				 * an eligible pv_page.
				 */
				pvp = TAILQ_FIRST(&pv_page_freelist);
				if (--pvp->pvp_pgi.pgi_nfree == 0) {
					TAILQ_REMOVE(&pv_page_freelist, pvp,
					    pvp_pgi.pgi_list);
				}
				newpv = LIST_FIRST(&pvp->pvp_pgi.pgi_freelist);
				LIST_REMOVE(newpv, pv_list);
#ifdef DIAGNOSTIC
				if (newpv == NULL)
					panic("pmap_pv_collect: pgi_nfree inconsistent");
#endif
				/*
				 * Remove the doomed pv_entry from its
				 * pv_list and copy its contents to
				 * the new entry.
				 */
				LIST_REMOVE(pv, pv_list);
				*newpv = *pv;

				/*
				 * Now insert the new pv_entry into
				 * the list.
				 */
				LIST_INSERT_HEAD(&pvh->pvh_list, newpv,
				    pv_list);
			}
		}
		splx(s);
	}

	simple_unlock(&pmap_pvalloc_slock);

	/*
	 * Now free all of the pages we've collected.
	 */
	for (pvp = TAILQ_FIRST(&pv_page_collectlist); pvp != NULL; pvp = npvp) {
		npvp = TAILQ_NEXT(pvp, pvp_pgi.pgi_list);
		pvppa = ALPHA_K0SEG_TO_PHYS((vaddr_t)pvp);
		pmap_physpage_free(pvppa);
	}
}

/******************** misc. functions ********************/

/*
 * pmap_physpage_alloc:
 *
 *	Allocate a single page from the VM system and return the
 *	physical address for that page.
 */
paddr_t
pmap_physpage_alloc(usage)
	int usage;
{
	struct vm_page *pg;
	struct pv_head *pvh;
	paddr_t pa;
	pmap_t pmap;
	int try;

	try = 0;	/* try a few times, but give up eventually */

	do {
#if defined(UVM)
		pg = uvm_pagealloc(NULL, 0, NULL);
#else
		pg = vm_page_alloc1();
#endif
		if (pg != NULL) {
			pa = VM_PAGE_TO_PHYS(pg);
			pmap_zero_page(pa);

			pvh = pa_to_pvh(pa);
			simple_lock(&pvh->pvh_slock);
#ifdef DIAGNOSTIC
			if (pvh->pvh_usage != PGU_NORMAL) {
				printf("pmap_physpage_alloc: page 0x%lx is "
				    "in use (%s)\n", pa,
				    pmap_pgu_strings[pvh->pvh_usage]);
				goto die;
			}
			if (pvh->pvh_refcnt != 0) {
				printf("pmap_physpage_alloc: page 0x%lx has "
				    "%d references", pa, pvh->pvh_refcnt);
				goto die;
			}
#endif
			pvh->pvh_usage = usage;
			simple_unlock(&pvh->pvh_slock);
			return (pa);
		}

		/*
		 * We are in an extreme low memory condition.
		 * Find any inactive pmap and forget all of
		 * the physical mappings (the upper-layer
		 * VM code will rebuild them the next time
		 * the pmap is activated).
		 */
		simple_lock(&pmap_all_pmaps_slock);
		for (pmap = LIST_FIRST(&pmap_all_pmaps); pmap != NULL;
		     pmap = LIST_NEXT(pmap, pm_list)) {
			/*
			 * Don't garbage-collect pmaps that reference
			 * kernel_lev1map.  They don't have any user PT
			 * pages to free.
			 */
			if (pmap->pm_lev1map != kernel_lev1map &&
			    pmap->pm_cpus == 0) {
				pmap_collect(pmap);
				break;
			}
		}
		simple_unlock(&pmap_all_pmaps_slock);
	} while (try++ < 5);

	/*
	 * If we couldn't get any more pages after 5 tries, just
	 * give up.
	 */
	printf("pmap_physpage_alloc: no pages for %s page available after "
	    "5 tries", pmap_pgu_strings[usage]);
#ifdef DIAGNOSTIC
 die:
#endif
	panic("pmap_physpage_alloc");
}

/*
 * pmap_physpage_free:
 *
 *	Free the single page table page at the specified physical address.
 */
void
pmap_physpage_free(pa)
	paddr_t pa;
{
	struct pv_head *pvh;
	struct vm_page *pg;

	if ((pg = PHYS_TO_VM_PAGE(pa)) == NULL)
		panic("pmap_physpage_free: bogus physical page address");

	pvh = pa_to_pvh(pa);

	simple_lock(&pvh->pvh_slock);
#ifdef DIAGNOSTIC
	if (pvh->pvh_usage == PGU_NORMAL)
		panic("pmap_physpage_free: not in use?!");
	if (pvh->pvh_refcnt != 0)
		panic("pmap_physpage_free: page still has references");
#endif
	pvh->pvh_usage = PGU_NORMAL;
	simple_unlock(&pvh->pvh_slock);

#if defined(UVM)
	uvm_pagefree(pg);
#else
	vm_page_free1(pg);
#endif
}

/*
 * pmap_physpage_addref:
 *
 *	Add a reference to the specified special use page.
 */
int
pmap_physpage_addref(kva)
	void *kva;
{
	struct pv_head *pvh;
	paddr_t pa;
	int rval;

	pa = ALPHA_K0SEG_TO_PHYS(trunc_page(kva));
	pvh = pa_to_pvh(pa);

	simple_lock(&pvh->pvh_slock);
#ifdef DIAGNOSTIC
	if (pvh->pvh_usage == PGU_NORMAL)
		panic("pmap_physpage_addref: not a special use page");
#endif

	rval = ++pvh->pvh_refcnt;
	simple_unlock(&pvh->pvh_slock);

	return (rval);
}

/*
 * pmap_physpage_delref:
 *
 *	Delete a reference to the specified special use page.
 */
int
pmap_physpage_delref(kva)
	void *kva;
{
	struct pv_head *pvh;
	paddr_t pa;
	int rval;

	pa = ALPHA_K0SEG_TO_PHYS(trunc_page(kva));
	pvh = pa_to_pvh(pa);

	simple_lock(&pvh->pvh_slock);
#ifdef DIAGNOSTIC
	if (pvh->pvh_usage == PGU_NORMAL)
		panic("pmap_physpage_delref: not a special use page");
#endif

	rval = --pvh->pvh_refcnt;

#ifdef DIAGNOSTIC
	/*
	 * Make sure we never have a negative reference count.
	 */
	if (pvh->pvh_refcnt < 0)
		panic("pmap_physpage_delref: negative reference count");
#endif
	simple_unlock(&pvh->pvh_slock);

	return (rval);
}

/******************** page table page management ********************/

#if defined(PMAP_NEW)
/*
 * pmap_growkernel:		[ INTERFACE ]
 *
 *	Grow the kernel address space.  This is a hint from the
 *	upper layer to pre-allocate more kernel PT pages.
 *
 *	XXX Implement XXX
 */
#endif /* PMAP_NEW */

/*
 * pmap_lev1map_create:
 *
 *	Create a new level 1 page table for the specified pmap.
 *
 *	Note: the pmap must already be locked.
 */
void
pmap_lev1map_create(pmap, cpu_id)
	pmap_t pmap;
	long cpu_id;
{
	paddr_t ptpa;
	pt_entry_t pte;
	int i;

#ifdef DIAGNOSTIC
	if (pmap == pmap_kernel())
		panic("pmap_lev1map_create: got kernel pmap");

	if (pmap->pm_asn[cpu_id] != PMAP_ASN_RESERVED)
		panic("pmap_lev1map_create: pmap uses non-reserved ASN");
#endif

	/*
	 * Allocate a page for the level 1 table.
	 */
	ptpa = pmap_physpage_alloc(PGU_L1PT);
	pmap->pm_lev1map = (pt_entry_t *) ALPHA_PHYS_TO_K0SEG(ptpa);

	/*
	 * Initialize the new level 1 table by copying the
	 * kernel mappings into it.
	 */
	for (i = l1pte_index(VM_MIN_KERNEL_ADDRESS);
	     i <= l1pte_index(VM_MAX_KERNEL_ADDRESS); i++)
		pmap->pm_lev1map[i] = kernel_lev1map[i];

	/*
	 * Now, map the new virtual page table.  NOTE: NO ASM!
	 */
	pte = ((ptpa >> PGSHIFT) << PG_SHIFT) | PG_V | PG_KRE | PG_KWE;
	pmap->pm_lev1map[l1pte_index(VPTBASE)] = pte;

	/*
	 * The page table base has changed; if the pmap was active,
	 * reactivate it.
	 */
	if (PMAP_ISACTIVE(pmap)) {
		pmap_asn_alloc(pmap, cpu_id);
		PMAP_ACTIVATE(pmap, curproc, cpu_id);
	}
}

/*
 * pmap_lev1map_destroy:
 *
 *	Destroy the level 1 page table for the specified pmap.
 *
 *	Note: the pmap must already be locked.
 */
void
pmap_lev1map_destroy(pmap, cpu_id)
	pmap_t pmap;
	long cpu_id;
{
	paddr_t ptpa;

#ifdef DIAGNOSTIC
	if (pmap == pmap_kernel())
		panic("pmap_lev1map_destroy: got kernel pmap");
#endif

	ptpa = ALPHA_K0SEG_TO_PHYS((vaddr_t)pmap->pm_lev1map);

	/*
	 * Go back to referencing the global kernel_lev1map.
	 */
	pmap->pm_lev1map = kernel_lev1map;

	/*
	 * The page table base has changed; if the pmap was active,
	 * reactivate it.  Note that allocation of a new ASN is
	 * not necessary here:
	 *
	 *	(1) We've gotten here because we've deleted all
	 *	    user mappings in the pmap, invalidating the
	 *	    TLB entries for them as we go.
	 *
	 *	(2) kernel_lev1map contains only kernel mappings, which
	 *	    were identical in the user pmap, and all of
	 *	    those mappings have PG_ASM, so the ASN doesn't
	 *	    matter.
	 *
	 * We do, however, ensure that the pmap is using the
	 * reserved ASN, to ensure that no two pmaps never have
	 * clashing TLB entries.
	 */
	PMAP_INVALIDATE_ASN(pmap, cpu_id);
	if (PMAP_ISACTIVE(pmap))
		PMAP_ACTIVATE(pmap, curproc, cpu_id);

	/*
	 * Free the old level 1 page table page.
	 */
	pmap_physpage_free(ptpa);
}

/*
 * pmap_ptpage_alloc:
 *
 *	Allocate a level 2 or level 3 page table page, and
 *	initialize the PTE that references it.
 *
 *	Note: the pmap must already be locked.
 */
void
pmap_ptpage_alloc(pmap, pte, usage)
	pmap_t pmap;
	pt_entry_t *pte;
	int usage;
{
	paddr_t ptpa;

	/*
	 * Allocate the page table page.
	 */
	ptpa = pmap_physpage_alloc(usage);

	/*
	 * Initialize the referencing PTE.
	 */
	*pte = ((ptpa >> PGSHIFT) << PG_SHIFT) | \
	    PG_V | PG_KRE | PG_KWE | PG_WIRED |
	    (pmap == pmap_kernel() ? PG_ASM : 0);
}

/*
 * pmap_ptpage_free:
 *
 *	Free the level 2 or level 3 page table page referenced
 *	be the provided PTE.
 *
 *	Note: the pmap must already be locked.
 */
void
pmap_ptpage_free(pmap, pte)
	pmap_t pmap;
	pt_entry_t *pte;
{
	paddr_t ptpa;

	/*
	 * Extract the physical address of the page from the PTE
	 * and clear the entry.
	 */
	ptpa = pmap_pte_pa(pte);
	*pte = PG_NV;

#ifdef DEBUG
	pmap_zero_page(ptpa);
#endif

	/*
	 * Free the page table page.
	 */
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
void
pmap_l3pt_delref(pmap, va, l1pte, l2pte, l3pte, cpu_id)
	pmap_t pmap;
	vaddr_t va;
	pt_entry_t *l1pte, *l2pte, *l3pte;
	long cpu_id;
{

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
		pmap->pm_nlev3--;

		/*
		 * We've freed a level 3 table, so we must
		 * invalidate the TLB entry for that PT page
		 * in the Virtual Page Table VA range, because
		 * otherwise the PALcode will service a TLB
		 * miss using the stale VPT TLB entry it entered
		 * behind our back to shortcut to the VA's PTE.
		 */
		PMAP_INVALIDATE_TLB(pmap,
		    (vaddr_t)(&VPT[VPT_INDEX(va)]), FALSE,
		    PMAP_ISACTIVE(pmap), cpu_id);

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
void
pmap_l2pt_delref(pmap, l1pte, l2pte, cpu_id)
	pmap_t pmap;
	pt_entry_t *l1pte, *l2pte;
	long cpu_id;
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
		pmap->pm_nlev2--;

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
void
pmap_l1pt_delref(pmap, l1pte, cpu_id)
	pmap_t pmap;
	pt_entry_t *l1pte;
	long cpu_id;
{

#ifdef DIAGNOSTIC
	if (pmap == pmap_kernel())
		panic("pmap_l1pt_delref: kernel pmap");
#endif

	if (pmap_physpage_delref(l1pte) == 0) {
		/*
		 * No more level 2 tables left, go back to the global
		 * kernel_lev1map.
		 */
		pmap_lev1map_destroy(pmap, cpu_id);
	}
}

/******************** Address Space Number management ********************/

/*
 * pmap_asn_alloc:
 *
 *	Allocate and assign an ASN to the specified pmap.
 *
 *	Note: the pmap must already be locked.
 */
void
pmap_asn_alloc(pmap, cpu_id)
	pmap_t pmap;
	long cpu_id;
{

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
	 */
	if (pmap->pm_lev1map == kernel_lev1map) {
#ifdef DEBUG
		if (pmapdebug & PDB_ASN)
			printf("pmap_asn_alloc: still references "
			    "kernel_lev1map\n");
#endif
#ifdef DIAGNOSTIC
		if (pmap->pm_asn[cpu_id] != PMAP_ASN_RESERVED)
			panic("pmap_asn_alloc: kernel_lev1map without "
			    "PMAP_ASN_RESERVED");
#endif
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
		pmap->pm_asngen[cpu_id] = pmap_asn_generation[cpu_id];
#ifdef DEBUG
		if (pmapdebug & PDB_ASN)
			printf("pmap_asn_alloc: no ASNs, using asngen %lu\n",
			    pmap->pm_asngen);
#endif
		return;
	}

	/*
	 * Hopefully, we can continue using the one we have...
	 */
	if (pmap->pm_asn[cpu_id] != PMAP_ASN_RESERVED &&
	    pmap->pm_asngen[cpu_id] == pmap_asn_generation[cpu_id]) {
		/*
		 * ASN is still in the current generation; keep on using it.
		 */
#ifdef DEBUG
		if (pmapdebug & PDB_ASN) 
			printf("pmap_asn_alloc: same generation, keeping %u\n",
			    pmap->pm_asn[cpu_id]);
#endif
		return;
	}

	/*
	 * Need to assign a new ASN.  Grab the next one, incrementing
	 * the generation number if we have to.
	 */
	if (pmap_next_asn[cpu_id] > pmap_max_asn) {
		/*
		 * Invalidate all non-PG_ASM TLB entries and the
		 * I-cache, and bump the generation number.
		 */
		ALPHA_TBIAP();
		alpha_pal_imb();

		pmap_next_asn[cpu_id] = 1;

		pmap_asn_generation[cpu_id]++;
#ifdef DIAGNOSTIC
		if (pmap_asn_generation[cpu_id] == 0) {
			/*
			 * The generation number has wrapped.  We could
			 * handle this scenario by traversing all of
			 * the pmaps, and invaldating the generation
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
			    pmap_asn_generation);
#endif
	}

	/*
	 * Assign the new ASN and validate the generation number.
	 */
	pmap->pm_asn[cpu_id] = pmap_next_asn[cpu_id]++;
	pmap->pm_asngen[cpu_id] = pmap_asn_generation[cpu_id];

#ifdef DEBUG
	if (pmapdebug & PDB_ASN)
		printf("pmap_asn_alloc: assigning %u to pmap %p\n",
		    pmap->pm_asn[cpu_id], pmap);
#endif
}
