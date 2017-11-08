/*	$NetBSD: pmap.c,v 1.207.2.1.4.3 2017/11/08 21:28:18 snj Exp $	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 *	@(#)pmap.c	8.4 (Berkeley) 1/26/94
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.207.2.1.4.3 2017/11/08 21:28:18 snj Exp $");

/*
 *	Manages physical address maps.
 *
 *	In addition to hardware address maps, this
 *	module is called upon to provide software-use-only
 *	maps which may or may not be stored in the same
 *	form as hardware maps.  These pseudo-maps are
 *	used to store intermediate results from copy
 *	operations to and from address spaces.
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

/* XXX simonb 2002/02/26
 *
 * MIPS3_PLUS is used to conditionally compile the r4k MMU support.
 * This is bogus - for example, some IDT MIPS-II CPUs have r4k style
 * MMUs (and 32-bit ones at that).
 *
 * On the other hand, it's not likely that we'll ever support the R6000
 * (is it?), so maybe that can be an "if MIPS2 or greater" check.
 *
 * Also along these lines are using totally separate functions for
 * r3k-style and r4k-style MMUs and removing all the MIPS_HAS_R4K_MMU
 * checks in the current functions.
 *
 * These warnings probably applies to other files under sys/arch/mips.
 */

#include "opt_sysv.h"
#include "opt_cputype.h"
#include "opt_multiprocessor.h"
#include "opt_mips_cache.h"

#ifdef MULTIPROCESSOR
#define PMAP_NO_PV_UNCACHED
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/cpu.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/pool.h>
#include <sys/atomic.h>
#include <sys/mutex.h>
#include <sys/atomic.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
#include <sys/socketvar.h>	/* XXX: for sock_loan_thresh */

#include <uvm/uvm.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>
#include <mips/locore.h>
#include <mips/pte.h>

CTASSERT(MIPS_KSEG0_START < 0);
CTASSERT((intptr_t)MIPS_PHYS_TO_KSEG0(0x1000) < 0);
CTASSERT(MIPS_KSEG1_START < 0);
CTASSERT((intptr_t)MIPS_PHYS_TO_KSEG1(0x1000) < 0);
CTASSERT(MIPS_KSEG2_START < 0);
CTASSERT(MIPS_MAX_MEM_ADDR < 0);
CTASSERT(MIPS_RESERVED_ADDR < 0);
CTASSERT((uint32_t)MIPS_KSEG0_START == 0x80000000);
CTASSERT((uint32_t)MIPS_KSEG1_START == 0xa0000000);
CTASSERT((uint32_t)MIPS_KSEG2_START == 0xc0000000);
CTASSERT((uint32_t)MIPS_MAX_MEM_ADDR == 0xbe000000);
CTASSERT((uint32_t)MIPS_RESERVED_ADDR == 0xbfc80000);
CTASSERT(MIPS_KSEG0_P(MIPS_PHYS_TO_KSEG0(0)));
CTASSERT(MIPS_KSEG1_P(MIPS_PHYS_TO_KSEG1(0)));

#define	PMAP_COUNT(name)	(pmap_evcnt_##name.ev_count++ + 0)
#define PMAP_COUNTER(name, desc) \
static struct evcnt pmap_evcnt_##name = \
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", desc); \
EVCNT_ATTACH_STATIC(pmap_evcnt_##name)

PMAP_COUNTER(remove_kernel_calls, "remove kernel calls");
PMAP_COUNTER(remove_kernel_pages, "kernel pages unmapped");
PMAP_COUNTER(remove_user_calls, "remove user calls");
PMAP_COUNTER(remove_user_pages, "user pages unmapped");
PMAP_COUNTER(remove_flushes, "remove cache flushes");
PMAP_COUNTER(remove_tlb_ops, "remove tlb ops");
PMAP_COUNTER(remove_pvfirst, "remove pv first");
PMAP_COUNTER(remove_pvsearch, "remove pv search");

PMAP_COUNTER(prefer_requests, "prefer requests");
PMAP_COUNTER(prefer_adjustments, "prefer adjustments");

PMAP_COUNTER(idlezeroed_pages, "pages idle zeroed");
PMAP_COUNTER(zeroed_pages, "pages zeroed");
PMAP_COUNTER(copied_pages, "pages copied");

PMAP_COUNTER(kenter_pa, "kernel fast mapped pages");
PMAP_COUNTER(kenter_pa_bad, "kernel fast mapped pages (bad color)");
PMAP_COUNTER(kenter_pa_unmanaged, "kernel fast mapped unmanaged pages");
PMAP_COUNTER(kremove_pages, "kernel fast unmapped pages");

PMAP_COUNTER(page_cache_evictions, "pages changed to uncacheable");
PMAP_COUNTER(page_cache_restorations, "pages changed to cacheable");

PMAP_COUNTER(kernel_mappings_bad, "kernel pages mapped (bad color)");
PMAP_COUNTER(user_mappings_bad, "user pages mapped (bad color)");
PMAP_COUNTER(kernel_mappings, "kernel pages mapped");
PMAP_COUNTER(user_mappings, "user pages mapped");
PMAP_COUNTER(user_mappings_changed, "user mapping changed");
PMAP_COUNTER(kernel_mappings_changed, "kernel mapping changed");
PMAP_COUNTER(uncached_mappings, "uncached pages mapped");
PMAP_COUNTER(unmanaged_mappings, "unmanaged pages mapped");
PMAP_COUNTER(managed_mappings, "managed pages mapped");
PMAP_COUNTER(mappings, "pages mapped");
PMAP_COUNTER(remappings, "pages remapped");
PMAP_COUNTER(unmappings, "pages unmapped");
PMAP_COUNTER(primary_mappings, "page initial mappings");
PMAP_COUNTER(primary_unmappings, "page final unmappings");
PMAP_COUNTER(tlb_hit, "page mapping");

PMAP_COUNTER(exec_mappings, "exec pages mapped");
PMAP_COUNTER(exec_synced_mappings, "exec pages synced");
PMAP_COUNTER(exec_synced_remove, "exec pages synced (PR)");
PMAP_COUNTER(exec_synced_clear_modify, "exec pages synced (CM)");
PMAP_COUNTER(exec_synced_page_protect, "exec pages synced (PP)");
PMAP_COUNTER(exec_synced_protect, "exec pages synced (P)");
PMAP_COUNTER(exec_uncached_page_protect, "exec pages uncached (PP)");
PMAP_COUNTER(exec_uncached_clear_modify, "exec pages uncached (CM)");
PMAP_COUNTER(exec_uncached_zero_page, "exec pages uncached (ZP)");
PMAP_COUNTER(exec_uncached_copy_page, "exec pages uncached (CP)");
PMAP_COUNTER(exec_uncached_remove, "exec pages uncached (PR)");

PMAP_COUNTER(create, "creates");
PMAP_COUNTER(reference, "references");
PMAP_COUNTER(dereference, "dereferences");
PMAP_COUNTER(destroy, "destroyed");
PMAP_COUNTER(activate, "activations");
PMAP_COUNTER(deactivate, "deactivations");
PMAP_COUNTER(update, "updates");
#ifdef MULTIPROCESSOR
PMAP_COUNTER(shootdown_ipis, "shootdown IPIs");
#endif
PMAP_COUNTER(unwire, "unwires");
PMAP_COUNTER(copy, "copies");
PMAP_COUNTER(collect, "collects");
PMAP_COUNTER(clear_modify, "clear_modifies");
PMAP_COUNTER(protect, "protects");
PMAP_COUNTER(page_protect, "page_protects");

#define PDB_FOLLOW	0x0001
#define PDB_INIT	0x0002
#define PDB_ENTER	0x0004
#define PDB_REMOVE	0x0008
#define PDB_CREATE	0x0010
#define PDB_PTPAGE	0x0020
#define PDB_PVENTRY	0x0040
#define PDB_BITS	0x0080
#define PDB_COLLECT	0x0100
#define PDB_PROTECT	0x0200
#define PDB_TLBPID	0x0400
#define PDB_PARANOIA	0x2000
#define PDB_WIRING	0x4000
#define PDB_PVDUMP	0x8000
int pmapdebug = 0;

#define PMAP_ASID_RESERVED 0

CTASSERT(PMAP_ASID_RESERVED == 0);
/*
 * Initialize the kernel pmap.
 */
#ifdef MULTIPROCESSOR
#define	PMAP_SIZE	offsetof(struct pmap, pm_pai[MAXCPUS])
#else
#define	PMAP_SIZE	sizeof(struct pmap)
kmutex_t pmap_pvlist_mutex __aligned(COHERENCY_UNIT);
#endif

struct pmap_kernel kernel_pmap_store = {
	.kernel_pmap = {
		.pm_count = 1,
		.pm_segtab = (void *)(MIPS_KSEG2_START + 0x1eadbeef),
#ifdef MULTIPROCESSOR
		.pm_active = 1,
		.pm_onproc = 1,
#endif
	},
};
struct pmap * const kernel_pmap_ptr = &kernel_pmap_store.kernel_pmap;

paddr_t mips_avail_start;	/* PA of first available physical page */
paddr_t mips_avail_end;		/* PA of last available physical page */
vaddr_t mips_virtual_end;	/* VA of last avail page (end of kernel AS) */
vaddr_t iospace;		/* VA of start of I/O space, if needed  */
vsize_t iospace_size = 0;	/* Size of (initial) range of I/O addresses */

pt_entry_t	*Sysmap;		/* kernel pte table */
unsigned int	Sysmapsize;		/* number of pte's in Sysmap */

#ifdef PMAP_POOLPAGE_DEBUG
struct poolpage_info {
	vaddr_t base;
	vaddr_t size;
	vaddr_t hint;
	pt_entry_t *sysmap;
} poolpage;
#endif

static void pmap_pvlist_lock_init(void);

/*
 * The pools from which pmap structures and sub-structures are allocated.
 */
struct pool pmap_pmap_pool;
struct pool pmap_pv_pool;

#ifndef PMAP_PV_LOWAT
#define	PMAP_PV_LOWAT	16
#endif
int		pmap_pv_lowat = PMAP_PV_LOWAT;

bool		pmap_initialized = false;
#define	PMAP_PAGE_COLOROK_P(a, b) \
		((((int)(a) ^ (int)(b)) & pmap_page_colormask) == 0)
u_int		pmap_page_colormask;

#define PAGE_IS_MANAGED(pa)	(pmap_initialized && uvm_pageismanaged(pa))

#define PMAP_IS_ACTIVE(pm)						\
	((pm) == pmap_kernel() || 					\
	 (pm) == curlwp->l_proc->p_vmspace->vm_map.pmap)

/* Forward function declarations */
void pmap_page_remove(struct vm_page *);
void pmap_remove_pv(pmap_t, vaddr_t, struct vm_page *, bool);
void pmap_enter_pv(pmap_t, vaddr_t, struct vm_page *, u_int *, int);
pt_entry_t *pmap_pte(pmap_t, vaddr_t);

/*
 * PV table management functions.
 */
void	*pmap_pv_page_alloc(struct pool *, int);
void	pmap_pv_page_free(struct pool *, void *);

struct pool_allocator pmap_pv_page_allocator = {
	pmap_pv_page_alloc, pmap_pv_page_free, 0,
};

#define	pmap_pv_alloc()		pool_get(&pmap_pv_pool, PR_NOWAIT)
#define	pmap_pv_free(pv)	pool_put(&pmap_pv_pool, (pv))

/*
 * Misc. functions.
 */

static inline bool
pmap_clear_mdpage_attributes(struct vm_page_md *md, u_int clear_attributes)
{
	volatile u_int * const attrp = &md->pvh_attrs;
#ifdef MULTIPROCESSOR
	for (;;) {
		u_int old_attr = *attrp;
		if ((old_attr & clear_attributes) == 0)
			return false;
		u_int new_attr = old_attr & ~clear_attributes;
		if (old_attr == atomic_cas_uint(attrp, old_attr, new_attr))
			return true;
	}
#else
	u_int old_attr = *attrp;
	if ((old_attr & clear_attributes) == 0)
		return false;
	*attrp &= ~clear_attributes;
	return true;
#endif
}

static inline void
pmap_set_mdpage_attributes(struct vm_page_md *md, u_int set_attributes)
{
#ifdef MULTIPROCESSOR
	atomic_or_uint(&md->pvh_attrs, set_attributes);
#else
	md->pvh_attrs |= set_attributes;
#endif
}

static inline void
pmap_page_syncicache(struct vm_page *pg)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
#ifdef MULTIPROCESSOR
	pv_entry_t pv = &md->pvh_first;
	uint32_t onproc = 0;
	(void)PG_MD_PVLIST_LOCK(md, false);
	if (pv->pv_pmap != NULL) {
		for (; pv != NULL; pv = pv->pv_next) {
			onproc |= pv->pv_pmap->pm_onproc;
			if (onproc == cpus_running)
				break;
		}
	}
	PG_MD_PVLIST_UNLOCK(md);
	kpreempt_disable();
	pmap_tlb_syncicache(trunc_page(md->pvh_first.pv_va), onproc);
	kpreempt_enable();
#else
	if (MIPS_HAS_R4K_MMU) {
		if (PG_MD_CACHED_P(md)) {
			mips_icache_sync_range_index(
			    trunc_page(md->pvh_first.pv_va), PAGE_SIZE);
		}
	} else {
		mips_icache_sync_range(MIPS_PHYS_TO_KSEG0(VM_PAGE_TO_PHYS(pg)),
		    PAGE_SIZE);
	}
#endif
}

static vaddr_t
pmap_map_ephemeral_page(struct vm_page *pg, int prot, pt_entry_t *old_pt_entry_p)
{
	const paddr_t pa = VM_PAGE_TO_PHYS(pg);
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pv = &md->pvh_first;

#ifdef _LP64
	vaddr_t va = MIPS_PHYS_TO_XKPHYS_CACHED(pa);
#else
	vaddr_t va;
	if (pa <= MIPS_PHYS_MASK) {
		va = MIPS_PHYS_TO_KSEG0(pa);
	} else {
		KASSERT(pmap_initialized);
		/*
		 * Make sure to use a congruent mapping to the last mapped
		 * address so we don't have to worry about virtual aliases.
		 */
		kpreempt_disable();
		struct cpu_info * const ci = curcpu();

		va = (prot & VM_PROT_WRITE ? ci->ci_pmap_dstbase : ci->ci_pmap_srcbase)
		    + mips_cache_indexof(MIPS_CACHE_VIRTUAL_ALIAS ? pv->pv_va : pa);
		*old_pt_entry_p = *kvtopte(va);
		pmap_kenter_pa(va, pa, prot, 0);
	}
#endif /* _LP64 */
	if (MIPS_CACHE_VIRTUAL_ALIAS) {
		/*
		 * If we are forced to use an incompatible alias, flush the
		 * page from the cache so we will copy the correct contents.
		 */
		(void)PG_MD_PVLIST_LOCK(md, false);
		if (PG_MD_CACHED_P(md)
		    && mips_cache_badalias(pv->pv_va, va)) {
			mips_dcache_wbinv_range_index(trunc_page(pv->pv_va),
			    PAGE_SIZE);
		}
		PG_MD_PVLIST_UNLOCK(md);
	}

	return va;
}

static void
pmap_unmap_ephemeral_page(struct vm_page *pg, vaddr_t va,
	pt_entry_t old_pt_entry)
{

	if (MIPS_CACHE_VIRTUAL_ALIAS) {
		/*
		 * Flush the page to avoid future incompatible aliases
		 */
		KASSERT((va & PAGE_MASK) == 0);
		mips_dcache_wbinv_range(va, PAGE_SIZE);
	}
#ifndef _LP64
	/*
	 * If we had to map using a page table entry, unmap it now.
	 */
	if (va >= VM_MIN_KERNEL_ADDRESS) {
		pmap_kremove(va, PAGE_SIZE);
		if (mips_pg_v(old_pt_entry.pt_entry)) {
			*kvtopte(va) = old_pt_entry;
			pmap_tlb_update_addr(pmap_kernel(), va,
			    old_pt_entry.pt_entry, false);
		}
		kpreempt_enable();
	}
#endif
}

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	firstaddr is the first unused kseg0 address (not page aligned).
 */
void
pmap_bootstrap(void)
{
	vsize_t bufsz;

	pmap_page_colormask = (uvmexp.ncolors -1) << PAGE_SHIFT;

	pmap_tlb_info_init(&pmap_tlb0_info);		/* init the lock */

	/*
	 * Compute the number of pages kmem_arena will have.
	 */
	kmeminit_nkmempages();

	/*
	 * Figure out how many PTE's are necessary to map the kernel.
	 * We also reserve space for kmem_alloc_pageable() for vm_fork().
	 */

	/* Get size of buffer cache and set an upper limit */
	buf_setvalimit((VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS) / 8);
	bufsz = buf_memcalc();
	buf_setvalimit(bufsz);

	Sysmapsize = (VM_PHYS_SIZE + (ubc_nwins << ubc_winshift) +
	    bufsz + 16 * NCARGS + pager_map_size + iospace_size) / NBPG +
	    (maxproc * UPAGES) + nkmempages;
#ifdef DEBUG
	{
		extern int kmem_guard_depth;
		Sysmapsize += kmem_guard_depth;
	}
#endif

#ifdef SYSVSHM
	Sysmapsize += shminfo.shmall;
#endif
#ifdef KSEG2IOBUFSIZE
	Sysmapsize += (KSEG2IOBUFSIZE >> PGSHIFT);
#endif
#ifdef PMAP_POOLPAGE_DEBUG
	poolpage.size = nkmempages + MCLBYTES * nmbclusters;
	Sysmapsize += poolpage.size;
#endif
#ifdef _LP64
	/*
	 * If we are using tmpfs, then we might want to use a great deal of
	 * our memory with it.  Make sure we have enough VM to do that.
	 */
	Sysmapsize += physmem;
#else
	/* XXX: else runs out of space on 256MB sbmips!! */
	Sysmapsize += 20000;
#endif
	/* Rounup to a even number of pte page tables */
	Sysmapsize = (Sysmapsize + NPTEPG - 1) & -NPTEPG;

	/*
	 * Initialize `FYI' variables.	Note we're relying on
	 * the fact that BSEARCH sorts the vm_physmem[] array
	 * for us.  Must do this before uvm_pageboot_alloc()
	 * can be called.
	 */
	mips_avail_start = ptoa(VM_PHYSMEM_PTR(0)->start);
	mips_avail_end = ptoa(VM_PHYSMEM_PTR(vm_nphysseg - 1)->end);
	mips_virtual_end = VM_MIN_KERNEL_ADDRESS + (vaddr_t)Sysmapsize * NBPG;

#ifndef _LP64
	/* Need space for I/O (not in K1SEG) ? */

	if (mips_virtual_end > VM_MAX_KERNEL_ADDRESS) {
		mips_virtual_end = VM_MAX_KERNEL_ADDRESS;
		Sysmapsize =
		    (VM_MAX_KERNEL_ADDRESS -
		     (VM_MIN_KERNEL_ADDRESS + iospace_size)) / NBPG;
	}

	if (iospace_size) {
		iospace = mips_virtual_end - iospace_size;
#ifdef DEBUG
		printf("io: %#"PRIxVADDR".%#"PRIxVADDR" %#"PRIxVADDR"\n",
		    iospace, iospace_size, mips_virtual_end);
#endif
	}
#endif
	pmap_pvlist_lock_init();

	/*
	 * Now actually allocate the kernel PTE array (must be done
	 * after mips_virtual_end is initialized).
	 */
	Sysmap = (pt_entry_t *)
	    uvm_pageboot_alloc(sizeof(pt_entry_t) * Sysmapsize);

#ifdef PMAP_POOLPAGE_DEBUG
	mips_virtual_end -= poolpage.size;
	poolpage.base = mips_virtual_end;
	poolpage.sysmap = Sysmap + atop(poolpage.size);
#endif
	/*
	 * Initialize the pools.
	 */
	pool_init(&pmap_pmap_pool, PMAP_SIZE, 0, 0, 0, "pmappl",
	    &pool_allocator_nointr, IPL_NONE);
	pool_init(&pmap_pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pvpl",
	    &pmap_pv_page_allocator, IPL_NONE);

	tlb_set_asid(0);

#ifdef MIPS3_PLUS	/* XXX mmu XXX */
	/*
	 * The R4?00 stores only one copy of the Global bit in the
	 * translation lookaside buffer for each 2 page entry.
	 * Thus invalid entries must have the Global bit set so
	 * when Entry LO and Entry HI G bits are anded together
	 * they will produce a global bit to store in the tlb.
	 */
	if (MIPS_HAS_R4K_MMU) {
		u_int i;
		pt_entry_t *spte;

		for (i = 0, spte = Sysmap; i < Sysmapsize; i++, spte++)
			spte->pt_entry = MIPS3_PG_G;
	}
#endif	/* MIPS3_PLUS */
}

/*
 * Define the initial bounds of the kernel virtual address space.
 */
void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{

	*vstartp = VM_MIN_KERNEL_ADDRESS;	/* kernel is in K0SEG */
	*vendp = trunc_page(mips_virtual_end);	/* XXX need pmap_growkernel() */
}

/*
 * Bootstrap memory allocator (alternative to vm_bootstrap_steal_memory()).
 * This function allows for early dynamic memory allocation until the virtual
 * memory system has been bootstrapped.  After that point, either kmem_alloc
 * or malloc should be used.  This function works by stealing pages from the
 * (to be) managed page pool, then implicitly mapping the pages (by using
 * their k0seg addresses) and zeroing them.
 *
 * It may be used once the physical memory segments have been pre-loaded
 * into the vm_physmem[] array.  Early memory allocation MUST use this
 * interface!  This cannot be used after vm_page_startup(), and will
 * generate a panic if tried.
 *
 * Note that this memory will never be freed, and in essence it is wired
 * down.
 *
 * We must adjust *vstartp and/or *vendp iff we use address space
 * from the kernel virtual address range defined by pmap_virtual_space().
 */
vaddr_t
pmap_steal_memory(vsize_t size, vaddr_t *vstartp, vaddr_t *vendp)
{
	u_int npgs;
	paddr_t pa;
	vaddr_t va;

	size = round_page(size);
	npgs = atop(size);

	for (u_int bank = 0; bank < vm_nphysseg; bank++) {
		struct vm_physseg * const seg = VM_PHYSMEM_PTR(bank);
		if (uvm.page_init_done == true)
			panic("pmap_steal_memory: called _after_ bootstrap");

		printf("%s: seg %u: %#"PRIxPADDR" %#"PRIxPADDR" %#"PRIxPADDR" %#"PRIxPADDR"\n",
		    __func__, bank,
		    seg->avail_start, seg->start,
		    seg->avail_end, seg->end);

		if (seg->avail_start != seg->start
		    || seg->avail_start >= seg->avail_end) {
			printf("%s: seg %u: bad start\n", __func__, bank);
			continue;
		}

		if (seg->avail_end - seg->avail_start < npgs) {
			printf("%s: seg %u: too small for %u pages\n",
			    __func__, bank, npgs);
			continue;
		}

		/*
		 * There are enough pages here; steal them!
		 */
		pa = ptoa(seg->avail_start);
		seg->avail_start += npgs;
		seg->start += npgs;

		/*
		 * Have we used up this segment?
		 */
		if (seg->avail_start == seg->end) {
			if (vm_nphysseg == 1)
				panic("pmap_steal_memory: out of memory!");

			/* Remove this segment from the list. */
			vm_nphysseg--;
			for (u_int x = bank; x < vm_nphysseg; x++) {
				/* structure copy */
				VM_PHYSMEM_PTR_SWAP(x, x + 1);
			}
		}

#ifdef _LP64
		/*
		 * Use the same CCA as used to access KSEG0 for XKPHYS.
		 */
		uint32_t v;

		__asm __volatile("mfc0 %0,$%1"
		    : "=r"(v)
		    : "n"(MIPS_COP_0_CONFIG));

		va = MIPS_PHYS_TO_XKPHYS(v & MIPS3_CONFIG_K0_MASK, pa);
#else
		if (pa + size > MIPS_PHYS_MASK + 1)
			panic("pmap_steal_memory: pa %"PRIxPADDR
			    " can not be mapped into KSEG0", pa);
		va = MIPS_PHYS_TO_KSEG0(pa);
#endif

		memset((void *)va, 0, size);
		return va;
	}

	/*
	 * If we got here, there was no memory left.
	 */
	panic("pmap_steal_memory: no memory to steal");
}

/*
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void
pmap_init(void)
{
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_INIT))
		printf("pmap_init()\n");
#endif

	/*
	 * Initialize the segtab lock.
	 */
	mutex_init(&pmap_segtab_lock, MUTEX_DEFAULT, IPL_HIGH);

	/*
	 * Set a low water mark on the pv_entry pool, so that we are
	 * more likely to have these around even in extreme memory
	 * starvation.
	 */
	pool_setlowat(&pmap_pv_pool, pmap_pv_lowat);

	/*
	 * Now it is safe to enable pv entry recording.
	 */
	pmap_initialized = true;

#ifndef _LP64
	/*
	 * If we have more memory than can be mapped by KSEG0, we need to
	 * allocate enough VA so we can map pages with the right color
	 * (to avoid cache alias problems).
	 */
	if (mips_avail_end > MIPS_KSEG1_START - MIPS_KSEG0_START) {
		curcpu()->ci_pmap_dstbase = uvm_km_alloc(kernel_map,
		    uvmexp.ncolors * PAGE_SIZE, 0, UVM_KMF_VAONLY);
		KASSERT(curcpu()->ci_pmap_dstbase);
		curcpu()->ci_pmap_srcbase = uvm_km_alloc(kernel_map,
		    uvmexp.ncolors * PAGE_SIZE, 0, UVM_KMF_VAONLY);
		KASSERT(curcpu()->ci_pmap_srcbase);
	}
#endif

#ifdef MIPS3
	if (MIPS_HAS_R4K_MMU) {
		/*
		 * XXX
		 * Disable sosend_loan() in src/sys/kern/uipc_socket.c
		 * on MIPS3 CPUs to avoid possible virtual cache aliases
		 * and uncached mappings in pmap_enter_pv().
		 *
		 * Ideally, read only shared mapping won't cause aliases
		 * so pmap_enter_pv() should handle any shared read only
		 * mappings without uncached ops like ARM pmap.
		 *
		 * On the other hand, R4000 and R4400 have the virtual
		 * coherency exceptions which will happen even on read only
		 * mappings, so we always have to disable sosend_loan()
		 * on such CPUs.
		 */
		sock_loan_thresh = -1;
	}
#endif
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
pmap_create(void)
{
	pmap_t pmap;

	PMAP_COUNT(create);
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_create()\n");
#endif

	pmap = pool_get(&pmap_pmap_pool, PR_WAITOK);
	memset(pmap, 0, PMAP_SIZE);

	pmap->pm_count = 1;

	pmap_segtab_init(pmap);

	return pmap;
}

/*
 *	Retire the given physical map from service.
 *	Should only be called if the map contains
 *	no valid mappings.
 */
void
pmap_destroy(pmap_t pmap)
{
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_destroy(%p)\n", pmap);
#endif
	if (atomic_dec_uint_nv(&pmap->pm_count) > 0) {
		PMAP_COUNT(dereference);
		return;
	}

	KASSERT(pmap->pm_count == 0);
	PMAP_COUNT(destroy);
	kpreempt_disable();
	pmap_tlb_asid_release_all(pmap);
	pmap_segtab_destroy(pmap);

	pool_put(&pmap_pmap_pool, pmap);
	kpreempt_enable();
}

/*
 *	Add a reference to the specified pmap.
 */
void
pmap_reference(pmap_t pmap)
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_reference(%p)\n", pmap);
#endif
	if (pmap != NULL) {
		atomic_inc_uint(&pmap->pm_count);
	}
	PMAP_COUNT(reference);
}

/*
 *	Make a new pmap (vmspace) active for the given process.
 */
void
pmap_activate(struct lwp *l)
{
	pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;

	PMAP_COUNT(activate);

	kpreempt_disable();
	pmap_tlb_asid_acquire(pmap, l);
	if (l == curlwp) {
		pmap_segtab_activate(pmap, l);
	}
	kpreempt_enable();
}

/*
 *	Make a previously active pmap (vmspace) inactive.
 */
void
pmap_deactivate(struct lwp *l)
{
	PMAP_COUNT(deactivate);

	kpreempt_disable();
	KASSERT(l == curlwp || l->l_cpu == curlwp->l_cpu);
#ifdef _LP64
	curcpu()->ci_pmap_segtab = (void *)(MIPS_KSEG2_START + 0x1eadbeef);
	curcpu()->ci_pmap_seg0tab = NULL;
#else
	curcpu()->ci_pmap_seg0tab = (void *)(MIPS_KSEG2_START + 0x1eadbeef);
#endif
	pmap_tlb_asid_deactivate(l->l_proc->p_vmspace->vm_map.pmap);
	kpreempt_enable();
}

void
pmap_update(struct pmap *pm)
{
	PMAP_COUNT(update);

	kpreempt_disable();
#ifdef MULTIPROCESSOR
	u_int pending = atomic_swap_uint(&pm->pm_shootdown_pending, 0);
	if (pending && pmap_tlb_shootdown_bystanders(pm))
		PMAP_COUNT(shootdown_ipis);
#endif
	/*
	 * If pmap_remove_all was called, we deactivated ourselves and nuked
	 * our ASID.  Now we have to reactivate ourselves.
	 */
	if (__predict_false(pm->pm_flags & PMAP_DEFERRED_ACTIVATE)) {
		pm->pm_flags ^= PMAP_DEFERRED_ACTIVATE;
		pmap_tlb_asid_acquire(pm, curlwp);
		pmap_segtab_activate(pm, curlwp);
	}
	kpreempt_enable();
}

/*
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the page size.
 */

static bool
pmap_pte_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva, pt_entry_t *pte,
	uintptr_t flags)
{
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT)) {
		printf("%s: %p, %"PRIxVADDR", %"PRIxVADDR", %p, %"PRIxPTR"\n",
		   __func__, pmap, sva, eva, pte, flags);
	}
#endif
	KASSERT(kpreempt_disabled());

	for (; sva < eva; sva += NBPG, pte++) {
		struct vm_page *pg;
		uint32_t pt_entry = pte->pt_entry;
		if (!mips_pg_v(pt_entry))
			continue;
		PMAP_COUNT(remove_user_pages);
		if (mips_pg_wired(pt_entry))
			pmap->pm_stats.wired_count--;
		pmap->pm_stats.resident_count--;
		pg = PHYS_TO_VM_PAGE(mips_tlbpfn_to_paddr(pt_entry));
		if (pg) {
			pmap_remove_pv(pmap, sva, pg,
			   pt_entry & mips_pg_m_bit());
		}
		pte->pt_entry = mips_pg_nv_bit();
		/*
		 * Flush the TLB for the given address.
		 */
		pmap_tlb_invalidate_addr(pmap, sva);
	}
	return false;
}

void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	struct vm_page *pg;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove(%p, %#"PRIxVADDR", %#"PRIxVADDR")\n", pmap, sva, eva);
#endif

	kpreempt_disable();
	if (pmap == pmap_kernel()) {
		/* remove entries from kernel pmap */
		PMAP_COUNT(remove_kernel_calls);
#ifdef PARANOIADIAG
		if (sva < VM_MIN_KERNEL_ADDRESS || eva >= mips_virtual_end)
			panic("pmap_remove: kva not in range");
#endif
		pt_entry_t *pte = kvtopte(sva);
		for (; sva < eva; sva += NBPG, pte++) {
			uint32_t pt_entry = pte->pt_entry;
			if (!mips_pg_v(pt_entry))
				continue;
			PMAP_COUNT(remove_kernel_pages);
			if (mips_pg_wired(pt_entry))
				pmap->pm_stats.wired_count--;
			pmap->pm_stats.resident_count--;
			pg = PHYS_TO_VM_PAGE(mips_tlbpfn_to_paddr(pt_entry));
			if (pg)
				pmap_remove_pv(pmap, sva, pg, false);
			if (MIPS_HAS_R4K_MMU)
				/* See above about G bit */
				pte->pt_entry = MIPS3_PG_NV | MIPS3_PG_G;
			else
				pte->pt_entry = MIPS1_PG_NV;

			/*
			 * Flush the TLB for the given address.
			 */
			pmap_tlb_invalidate_addr(pmap, sva);
		}
		kpreempt_enable();
		return;
	}

	PMAP_COUNT(remove_user_calls);
#ifdef PARANOIADIAG
	if (eva > VM_MAXUSER_ADDRESS)
		panic("pmap_remove: uva not in range");
	if (PMAP_IS_ACTIVE(pmap)) {
		pmap_tlb_asid_check();
	}
#endif
#ifdef PMAP_FAULTINFO
	curpcb->pcb_faultinfo.pfi_faultaddr = 0;
	curpcb->pcb_faultinfo.pfi_repeats = 0;
	curpcb->pcb_faultinfo.pfi_faultpte = NULL;
#endif
	pmap_pte_process(pmap, sva, eva, pmap_pte_remove, 0);
	kpreempt_enable();
}

/*
 *	pmap_page_protect:
 *
 *	Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pv;
	vaddr_t va;

	PMAP_COUNT(page_protect);
#ifdef DEBUG
	if ((pmapdebug & (PDB_FOLLOW|PDB_PROTECT)) ||
	    (prot == VM_PROT_NONE && (pmapdebug & PDB_REMOVE)))
		printf("pmap_page_protect(%#"PRIxPADDR", %x)\n",
		    VM_PAGE_TO_PHYS(pg), prot);
#endif
	switch (prot) {
	case VM_PROT_READ|VM_PROT_WRITE:
	case VM_PROT_ALL:
		break;

	/* copy_on_write */
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		(void)PG_MD_PVLIST_LOCK(md, false);
		pv = &md->pvh_first;
		/*
		 * Loop over all current mappings setting/clearing as apropos.
		 */
		if (pv->pv_pmap != NULL) {
			while (pv != NULL) {
				const pmap_t pmap = pv->pv_pmap;
				const uint16_t gen = PG_MD_PVLIST_GEN(md);
				if (pv->pv_va & PV_KENTER) {
					pv = pv->pv_next;
					continue;
				}
				va = trunc_page(pv->pv_va);
				PG_MD_PVLIST_UNLOCK(md);
				pmap_protect(pmap, va, va + PAGE_SIZE, prot);
				KASSERT(pv->pv_pmap == pmap);
				pmap_update(pmap);
				if (gen != PG_MD_PVLIST_LOCK(md, false)) {
					pv = &md->pvh_first;
				} else {
					pv = pv->pv_next;
				}
			}
		}
		PG_MD_PVLIST_UNLOCK(md);
		break;

	/* remove_all */
	default:
		/*
		 * Do this first so that for each unmapping, pmap_remove_pv
		 * won't try to sync the icache.
		 */
		if (pmap_clear_mdpage_attributes(md, PG_MD_EXECPAGE)) {
			PMAP_COUNT(exec_uncached_page_protect);
		}
		pmap_page_remove(pg);
	}
}

static bool
pmap_pte_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, pt_entry_t *pte,
	uintptr_t flags)
{
	const uint32_t pg_mask = ~(mips_pg_m_bit() | mips_pg_ro_bit());
	const uint32_t p = (flags & VM_PROT_WRITE) ? mips_pg_rw_bit() : mips_pg_ro_bit();
	KASSERT(kpreempt_disabled());
	KASSERT((sva & PAGE_MASK) == 0);
	KASSERT((eva & PAGE_MASK) == 0);

	/*
	 * Change protection on every valid mapping within this segment.
	 */
	for (; sva < eva; sva += NBPG, pte++) {
		uint32_t pt_entry = pte->pt_entry;
		if (!mips_pg_v(pt_entry))
			continue;
		struct vm_page *pg;
		pg = PHYS_TO_VM_PAGE(mips_tlbpfn_to_paddr(pt_entry));
		if (pg && (pt_entry & mips_pg_m_bit())) {
			struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
			if (MIPS_HAS_R4K_MMU
			    && MIPS_CACHE_VIRTUAL_ALIAS
			    && PG_MD_CACHED_P(md))
				mips_dcache_wbinv_range_index(sva, PAGE_SIZE);
			if (PG_MD_EXECPAGE_P(md)) {
				KASSERT(md->pvh_first.pv_pmap != NULL);
				if (PG_MD_CACHED_P(md)) {
					pmap_page_syncicache(pg);
					PMAP_COUNT(exec_synced_protect);
				}
			}
		}
		pt_entry = (pt_entry & pg_mask) | p;
		pte->pt_entry = pt_entry;
		/*
		 * Update the TLB if needed.
		 */
		pmap_tlb_update_addr(pmap, sva, pt_entry, true);
	}
	return false;
}

/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 */
void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	const uint32_t pg_mask = ~(mips_pg_m_bit() | mips_pg_ro_bit());
	pt_entry_t *pte;
	u_int p;

	KASSERT((sva & PAGE_MASK) == 0);
	KASSERT((eva & PAGE_MASK) == 0);
	PMAP_COUNT(protect);
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT))
		printf("pmap_protect(%p, %#"PRIxVADDR", %#"PRIxVADDR", %x)\n",
		    pmap, sva, eva, prot);
#endif
	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}

	p = (prot & VM_PROT_WRITE) ? mips_pg_rw_bit() : mips_pg_ro_bit();

	kpreempt_disable();
	if (pmap == pmap_kernel()) {
		/*
		 * Change entries in kernel pmap.
		 * This will trap if the page is writable (in order to set
		 * the dirty bit) even if the dirty bit is already set. The
		 * optimization isn't worth the effort since this code isn't
		 * executed much. The common case is to make a user page
		 * read-only.
		 */
#ifdef PARANOIADIAG
		if (sva < VM_MIN_KERNEL_ADDRESS || eva >= mips_virtual_end)
			panic("pmap_protect: kva not in range");
#endif
		pte = kvtopte(sva);
		for (; sva < eva; sva += NBPG, pte++) {
			uint32_t pt_entry = pte->pt_entry;
			if (!mips_pg_v(pt_entry))
				continue;
			if (MIPS_HAS_R4K_MMU && (pt_entry & mips_pg_m_bit()))
				mips_dcache_wb_range(sva, PAGE_SIZE);
			pt_entry &= (pt_entry & pg_mask) | p;
			pte->pt_entry = pt_entry;
			pmap_tlb_update_addr(pmap, sva, pt_entry, true);
		}
		kpreempt_enable();
		return;
	}

#ifdef PARANOIADIAG
	if (eva > VM_MAXUSER_ADDRESS)
		panic("pmap_protect: uva not in range");
	if (PMAP_IS_ACTIVE(pmap)) {
		pmap_tlb_asid_check();
	}
#endif

	/*
	 * Change protection on every valid mapping within this segment.
	 */
	pmap_pte_process(pmap, sva, eva, pmap_pte_protect, p);
	kpreempt_enable();
}

/*
 * XXXJRT -- need a version for each cache type.
 */
void
pmap_procwr(struct proc *p, vaddr_t va, size_t len)
{
#ifdef MIPS1
	pmap_t pmap;

	pmap = p->p_vmspace->vm_map.pmap;
#endif /* MIPS1 */

	if (MIPS_HAS_R4K_MMU) {
#ifdef MIPS3_PLUS	/* XXX mmu XXX */
		/*
		 * XXX
		 * shouldn't need to do this for physical d$?
		 * should need to do this for virtual i$ if prot == EXEC?
		 */
		if (p == curlwp->l_proc
		    && mips_cache_info.mci_pdcache_way_mask < PAGE_SIZE)
		    /* XXX check icache mask too? */
			mips_icache_sync_range(va, len);
		else
			mips_icache_sync_range_index(va, len);
#endif /* MIPS3_PLUS */	/* XXX mmu XXX */
	} else {
#ifdef MIPS1
		pt_entry_t *pte;
		unsigned entry;

		kpreempt_disable();
		if (pmap == pmap_kernel()) {
			pte = kvtopte(va);
		} else {
			pte = pmap_pte_lookup(pmap, va);
		}
		entry = pte->pt_entry;
		kpreempt_enable();
		if (!mips_pg_v(entry))
			return;

		/*
		 * XXXJRT -- Wrong -- since page is physically-indexed, we
		 * XXXJRT need to loop.
		 */
		mips_icache_sync_range(
		    MIPS_PHYS_TO_KSEG0(mips1_tlbpfn_to_paddr(entry)
		    + (va & PGOFSET)),
		    len);
#endif /* MIPS1 */
	}
}

/*
 *	Return RO protection of page.
 */
bool
pmap_is_page_ro_p(pmap_t pmap, vaddr_t va, uint32_t entry)
{

	return (entry & mips_pg_ro_bit()) != 0;
}

#if defined(MIPS3_PLUS) && !defined(MIPS3_NO_PV_UNCACHED)	/* XXX mmu XXX */
/*
 *	pmap_page_cache:
 *
 *	Change all mappings of a managed page to cached/uncached.
 */
static void
pmap_page_cache(struct vm_page *pg, bool cached)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	const uint32_t newmode = cached ? MIPS3_PG_CACHED : MIPS3_PG_UNCACHED;

	KASSERT(kpreempt_disabled());
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_page_uncache(%#"PRIxPADDR")\n", VM_PAGE_TO_PHYS(pg));
#endif

	if (cached) {
		pmap_clear_mdpage_attributes(md, PG_MD_UNCACHED);
		PMAP_COUNT(page_cache_restorations);
	} else {
		pmap_set_mdpage_attributes(md, PG_MD_UNCACHED);
		PMAP_COUNT(page_cache_evictions);
	}

	KASSERT(PG_MD_PVLIST_LOCKED_P(md));
	KASSERT(kpreempt_disabled());
	for (pv_entry_t pv = &md->pvh_first;
	     pv != NULL;
	     pv = pv->pv_next) {
		pmap_t pmap = pv->pv_pmap;
		vaddr_t va = trunc_page(pv->pv_va);
		pt_entry_t *pte;
		uint32_t pt_entry;

		KASSERT(pmap != NULL);
		KASSERT(!MIPS_KSEG0_P(va));
		KASSERT(!MIPS_KSEG1_P(va));
#ifdef _LP64
		KASSERT(!MIPS_XKPHYS_P(va));
#endif
		if (pmap == pmap_kernel()) {
			/*
			 * Change entries in kernel pmap.
			 */
			pte = kvtopte(va);
		} else {
			pte = pmap_pte_lookup(pmap, va);
			if (pte == NULL)
				continue;
		}
		pt_entry = pte->pt_entry;
		if (pt_entry & MIPS3_PG_V) {
			pt_entry = (pt_entry & ~MIPS3_PG_CACHEMODE) | newmode;
			pte->pt_entry = pt_entry;
			pmap_tlb_update_addr(pmap, va, pt_entry, true);
		}
	}
}
#endif	/* MIPS3_PLUS && !MIPS3_NO_PV_UNCACHED */

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
int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	pt_entry_t *pte;
	u_int npte;
	struct vm_page *pg;
	bool wired = (flags & PMAP_WIRED) != 0;
#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
	bool cached = true;
	bool prefetch = false;
#endif

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_enter(%p, %#"PRIxVADDR", %#"PRIxPADDR", %x, %x)\n",
		    pmap, va, pa, prot, wired);
#endif
	const bool good_color = PMAP_PAGE_COLOROK_P(pa, va);
	if (pmap == pmap_kernel()) {
		PMAP_COUNT(kernel_mappings);
		if (!good_color)
			PMAP_COUNT(kernel_mappings_bad);
#if defined(DEBUG) || defined(DIAGNOSTIC) || defined(PARANOIADIAG)
		if (va < VM_MIN_KERNEL_ADDRESS || va >= mips_virtual_end)
			panic("pmap_enter: kva %#"PRIxVADDR"too big", va);
#endif
	} else {
		PMAP_COUNT(user_mappings);
		if (!good_color)
			PMAP_COUNT(user_mappings_bad);
#if defined(DEBUG) || defined(DIAGNOSTIC) || defined(PARANOIADIAG)
		if (va >= VM_MAXUSER_ADDRESS)
			panic("pmap_enter: uva %#"PRIxVADDR" too big", va);
#endif
	}
#ifdef PARANOIADIAG
#if defined(cobalt) || defined(newsmips) || defined(pmax) /* otherwise ok */
	if (pa & 0x80000000)	/* this is not error in general. */
		panic("pmap_enter: pa");
#endif

	if (!(prot & VM_PROT_READ))
		panic("pmap_enter: prot");
#endif

#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
	if (flags & PMAP_NOCACHE)
		cached = 0;

	if (pa & PGC_NOCACHE) {
		cached = false;
		pa &= ~PGC_NOCACHE;
	}
	if (pa & PGC_PREFETCH) {
		prefetch = true;
		pa &= ~PGC_PREFETCH;
	}
#endif
	pg = PHYS_TO_VM_PAGE(pa);

	if (pg) {
		struct vm_page_md * const md = VM_PAGE_TO_MD(pg);

		/* Set page referenced/modified status based on flags */
		if (flags & VM_PROT_WRITE)
			pmap_set_mdpage_attributes(md, PG_MD_MODIFIED|PG_MD_REFERENCED);
		else if (flags & VM_PROT_ALL)
			pmap_set_mdpage_attributes(md, PG_MD_REFERENCED);
		if (!(prot & VM_PROT_WRITE))
			/*
			 * If page is not yet referenced, we could emulate this
			 * by not setting the page valid, and setting the
			 * referenced status in the TLB fault handler, similar
			 * to how page modified status is done for UTLBmod
			 * exceptions.
			 */
			npte = mips_pg_ropage_bit();
		else {
#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
			if (cached == false) {
				if (PG_MD_MODIFIED_P(md)) {
					npte = mips_pg_rwncpage_bit();
				} else {
					npte = mips_pg_cwncpage_bit();
				}
				PMAP_COUNT(uncached_mappings);
			} else
#endif
			 {
				if (PG_MD_MODIFIED_P(md)) {
					npte = mips_pg_rwpage_bit();
				} else {
					npte = mips_pg_cwpage_bit();
				}
			}
		}
		PMAP_COUNT(managed_mappings);
	} else {
		/*
		 * Assumption: if it is not part of our managed memory
		 * then it must be device memory which may be volatile.
		 */
		if (MIPS_HAS_R4K_MMU) {
#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
			u_int cca = PMAP_CCA_FOR_PA(pa);
			if (prefetch) cca = mips_options.mips3_cca_devmem;
			npte = MIPS3_PG_IOPAGE(cca) &
			    ~MIPS3_PG_G;
#else
			npte = MIPS3_PG_IOPAGE(PMAP_CCA_FOR_PA(pa)) &
			    ~MIPS3_PG_G;
#endif
		} else {
			npte = (prot & VM_PROT_WRITE) ?
			    (MIPS1_PG_D | MIPS1_PG_N) :
			    (MIPS1_PG_RO | MIPS1_PG_N);
		}
		PMAP_COUNT(unmanaged_mappings);
	}

#if 0
	/*
	 * The only time we need to flush the cache is if we
	 * execute from a physical address and then change the data.
	 * This is the best place to do this.
	 * pmap_protect() and pmap_remove() are mostly used to switch
	 * between R/W and R/O pages.
	 * NOTE: we only support cache flush for read only text.
	 */
#ifdef MIPS1
	if (!MIPS_HAS_R4K_MMU
	    && pg != NULL
	    && prot == (VM_PROT_READ | VM_PROT_EXECUTE)) {
		struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
		PMAP_COUNT(enter_exec_mapping);
		if (!PG_MD_EXECPAGE_P(md)) {
			KASSERT((pa & PAGE_MASK) == 0);
			mips_icache_sync_range(MIPS_PHYS_TO_KSEG0(pa),
			    PAGE_SIZE);
			pmap_set_mdpage_attributes(md, PG_MD_EXECPAGE);
			PMAP_COUNT(exec_syncicache_entry);
		}
	}
#endif
#endif

	kpreempt_disable();
	if (pmap == pmap_kernel()) {
		if (pg)
			pmap_enter_pv(pmap, va, pg, &npte, 0);

		/* enter entries into kernel pmap */
		pte = kvtopte(va);

		if (MIPS_HAS_R4K_MMU)
			npte |= mips3_paddr_to_tlbpfn(pa) | MIPS3_PG_G;
		else
			npte |= mips1_paddr_to_tlbpfn(pa) |
			    MIPS1_PG_V | MIPS1_PG_G;

		if (wired) {
			pmap->pm_stats.wired_count++;
			npte |= mips_pg_wired_bit();
		}
		if (mips_pg_v(pte->pt_entry)
		    && mips_tlbpfn_to_paddr(pte->pt_entry) != pa) {
			pmap_remove(pmap, va, va + NBPG);
			PMAP_COUNT(kernel_mappings_changed);
		}
		bool resident = mips_pg_v(pte->pt_entry);
		if (!resident)
			pmap->pm_stats.resident_count++;
		pte->pt_entry = npte;

		/*
		 * Update the same virtual address entry.
		 */
		pmap_tlb_update_addr(pmap, va, npte, resident);
		kpreempt_enable();
		return 0;
	}

	pte = pmap_pte_reserve(pmap, va, flags);
	if (__predict_false(pte == NULL)) {
		kpreempt_enable();
		return ENOMEM;
	}

	/* Done after case that may sleep/return. */
	if (pg)
		pmap_enter_pv(pmap, va, pg, &npte, 0);

	/*
	 * Now validate mapping with desired protection/wiring.
	 * Assume uniform modified and referenced status for all
	 * MIPS pages in a MACH page.
	 */

	if (MIPS_HAS_R4K_MMU)
		npte |= mips3_paddr_to_tlbpfn(pa);
	else
		npte |= mips1_paddr_to_tlbpfn(pa) | MIPS1_PG_V;

	if (wired) {
		pmap->pm_stats.wired_count++;
		npte |= mips_pg_wired_bit();
	}
#if defined(DEBUG)
	if (pmapdebug & PDB_ENTER) {
		printf("pmap_enter: %p: %#"PRIxVADDR": new pte %#x (pa %#"PRIxPADDR")", pmap, va, npte, pa);
		printf("\n");
	}
#endif

#ifdef PARANOIADIAG
	if (PMAP_IS_ACTIVE(pmap)) {
		struct pmap_asid_info * const pai = PMAP_PAI(pmap, curcpu());
		uint32_t asid;

		__asm volatile("mfc0 %0,$10; nop" : "=r"(asid));
		asid = (MIPS_HAS_R4K_MMU) ? (asid & 0xff) : (asid & 0xfc0) >> 6;
		if (asid != pai->pai_asid) {
			panic("inconsistency for active TLB update: %u <-> %u",
			    asid, pai->pai_asid);
		}
	}
#endif

	if (mips_pg_v(pte->pt_entry) &&
	    mips_tlbpfn_to_paddr(pte->pt_entry) != pa) {
#ifdef PMAP_FAULTINFO
		struct pcb_faultinfo tmp_fi = curpcb->pcb_faultinfo;
#endif
		pmap_remove(pmap, va, va + NBPG);
#ifdef PMAP_FAULTINFO
		curpcb->pcb_faultinfo = tmp_fi;
#endif
		PMAP_COUNT(user_mappings_changed);
	}

	KASSERT(mips_pg_v(npte));
	bool resident = mips_pg_v(pte->pt_entry);
	if (!resident)
		pmap->pm_stats.resident_count++;
#ifdef PMAP_FAULTINFO
	if (curpcb->pcb_faultinfo.pfi_faultpte == pte
	    && curpcb->pcb_faultinfo.pfi_repeats > 1) {
		printf("%s(%#"PRIxVADDR", %#"PRIxPADDR"): changing pte@%p from %#x to %#x\n",
		    __func__, va, pa, pte, pte->pt_entry, npte);
		cpu_Debugger();
	}
#endif
	pte->pt_entry = npte;

	pmap_tlb_update_addr(pmap, va, npte, resident);
	kpreempt_enable();

	if (pg != NULL && (prot == (VM_PROT_READ | VM_PROT_EXECUTE))) {
		struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("pmap_enter: flush I cache va %#"PRIxVADDR" (%#"PRIxPADDR")\n",
			    va - NBPG, pa);
#endif
		PMAP_COUNT(exec_mappings);
		if (!PG_MD_EXECPAGE_P(md) && PG_MD_CACHED_P(md)) {
			pmap_page_syncicache(pg);
			pmap_set_mdpage_attributes(md, PG_MD_EXECPAGE);
			PMAP_COUNT(exec_synced_mappings);
		}
	}

	return 0;
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	const bool managed = PAGE_IS_MANAGED(pa);
	pt_entry_t *pte;
	u_int npte;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_kenter_pa(%#"PRIxVADDR", %#"PRIxPADDR", %x)\n", va, pa, prot);
#endif
	PMAP_COUNT(kenter_pa);
	if (!PMAP_PAGE_COLOROK_P(pa, va) && managed)
		PMAP_COUNT(kenter_pa_bad);

	if (!managed)
		PMAP_COUNT(kenter_pa_unmanaged);

	if (MIPS_HAS_R4K_MMU) {
		npte = mips3_paddr_to_tlbpfn(pa)
		    | ((prot & VM_PROT_WRITE) ? MIPS3_PG_D : MIPS3_PG_RO)
		    | (managed ? MIPS3_PG_CACHED : MIPS3_PG_UNCACHED)
		    | MIPS3_PG_WIRED | MIPS3_PG_V | MIPS3_PG_G;
	} else {
		npte = mips1_paddr_to_tlbpfn(pa)
		    | ((prot & VM_PROT_WRITE) ? MIPS1_PG_D : MIPS1_PG_RO)
		    | (managed ? 0 : MIPS1_PG_N)
		    | MIPS1_PG_WIRED | MIPS1_PG_V | MIPS1_PG_G;
	}
	kpreempt_disable();
	pte = kvtopte(va);
	KASSERT(!mips_pg_v(pte->pt_entry));

	/*
	 * No need to track non-managed pages or PMAP_KMPAGEs pages for aliases
	 */
	if (managed && (flags & PMAP_KMPAGE) == 0) {
		pmap_t pmap = pmap_kernel();
		struct vm_page *pg = PHYS_TO_VM_PAGE(pa);

		pmap_enter_pv(pmap, va, pg, &npte, PV_KENTER);
	}

	pte->pt_entry = npte;
	pmap_tlb_update_addr(pmap_kernel(), va, npte, false);
	kpreempt_enable();
}

void
pmap_kremove(vaddr_t va, vsize_t len)
{
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE))
		printf("pmap_kremove(%#"PRIxVADDR", %#"PRIxVSIZE")\n", va, len);
#endif

	const uint32_t new_pt_entry =
	    (MIPS_HAS_R4K_MMU ? MIPS3_PG_NV | MIPS3_PG_G : MIPS1_PG_NV);

	kpreempt_disable();
	pt_entry_t *pte = kvtopte(va);
	for (vaddr_t eva = va + len; va < eva; va += PAGE_SIZE, pte++) {
		uint32_t pt_entry = pte->pt_entry;
		if (!mips_pg_v(pt_entry)) {
			continue;
		}

		PMAP_COUNT(kremove_pages);
		struct vm_page * const pg =
		    PHYS_TO_VM_PAGE(mips_tlbpfn_to_paddr(pt_entry));
		if (pg)
			pmap_remove_pv(pmap_kernel(), va, pg, false);
		pte->pt_entry = new_pt_entry;
		pmap_tlb_invalidate_addr(pmap_kernel(), va);
	}
	kpreempt_enable();
}

void
pmap_remove_all(struct pmap *pmap)
{
	KASSERT(pmap != pmap_kernel());

	kpreempt_disable();
	/*
	 * Free all of our ASIDs which means we can skip doing all the
	 * tlb_invalidate_addrs().
	 */
#ifdef MULTIPROCESSOR
	const uint32_t cpu_mask = 1 << cpu_index(curcpu());
	KASSERT((pmap->pm_onproc & ~cpu_mask) == 0);
	if (pmap->pm_onproc & cpu_mask)
		pmap_tlb_asid_deactivate(pmap);
#endif
	pmap_tlb_asid_release_all(pmap);
	pmap->pm_flags |= PMAP_DEFERRED_ACTIVATE;

#ifdef PMAP_FAULTINFO
	curpcb->pcb_faultinfo.pfi_faultaddr = 0;
	curpcb->pcb_faultinfo.pfi_repeats = 0;
	curpcb->pcb_faultinfo.pfi_faultpte = NULL;
#endif
	kpreempt_enable();
}
/*
 *	Routine:	pmap_unwire
 *	Function:	Clear the wired attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 */
void
pmap_unwire(pmap_t pmap, vaddr_t va)
{
	pt_entry_t *pte;

	PMAP_COUNT(unwire);
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_WIRING))
		printf("pmap_unwire(%p, %#"PRIxVADDR")\n", pmap, va);
#endif
	/*
	 * Don't need to flush the TLB since PG_WIRED is only in software.
	 */
	kpreempt_disable();
	if (pmap == pmap_kernel()) {
		/* change entries in kernel pmap */
#ifdef PARANOIADIAG
		if (va < VM_MIN_KERNEL_ADDRESS || va >= mips_virtual_end)
			panic("pmap_unwire");
#endif
		pte = kvtopte(va);
	} else {
		pte = pmap_pte_lookup(pmap, va);
#ifdef DIAGNOSTIC
		if (pte == NULL)
			panic("pmap_unwire: pmap %p va %#"PRIxVADDR" invalid STE",
			    pmap, va);
#endif
	}

#ifdef DIAGNOSTIC
	if (mips_pg_v(pte->pt_entry) == 0)
		panic("pmap_unwire: pmap %p va %#"PRIxVADDR" invalid PTE",
		    pmap, va);
#endif

	if (mips_pg_wired(pte->pt_entry)) {
		pte->pt_entry &= ~mips_pg_wired_bit();
		pmap->pm_stats.wired_count--;
	}
#ifdef DIAGNOSTIC
	else {
		printf("pmap_unwire: wiring for pmap %p va %#"PRIxVADDR" "
		    "didn't change!\n", pmap, va);
	}
#endif
	kpreempt_enable();
}

/*
 *	Routine:	pmap_extract
 *	Function:
 *		Extract the physical page address associated
 *		with the given map/virtual_address pair.
 */
bool
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *pap)
{
	paddr_t pa;
	pt_entry_t *pte;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_extract(%p, %#"PRIxVADDR") -> ", pmap, va);
#endif
	if (pmap == pmap_kernel()) {
		if (MIPS_KSEG0_P(va)) {
			pa = MIPS_KSEG0_TO_PHYS(va);
			goto done;
		}
#ifdef _LP64
		if (MIPS_XKPHYS_P(va)) {
			pa = MIPS_XKPHYS_TO_PHYS(va);
			goto done;
		}
#endif
#ifdef DIAGNOSTIC
		if (MIPS_KSEG1_P(va))
			panic("pmap_extract: kseg1 address %#"PRIxVADDR"", va);
#endif
		if (va >= mips_virtual_end)
			panic("pmap_extract: illegal kernel mapped address %#"PRIxVADDR"", va);
		pte = kvtopte(va);
		kpreempt_disable();
	} else {
		kpreempt_disable();
		if (!(pte = pmap_pte_lookup(pmap, va))) {
#ifdef DEBUG
			if (pmapdebug & PDB_FOLLOW)
				printf("not in segmap\n");
#endif
			kpreempt_enable();
			return false;
		}
	}
	if (!mips_pg_v(pte->pt_entry)) {
#ifdef DEBUG
		if (pmapdebug & PDB_FOLLOW)
			printf("PTE not valid\n");
#endif
		kpreempt_enable();
		return false;
	}
	pa = mips_tlbpfn_to_paddr(pte->pt_entry) | (va & PGOFSET);
	kpreempt_enable();
done:
	if (pap != NULL) {
		*pap = pa;
	}
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pa %#"PRIxPADDR"\n", pa);
#endif
	return true;
}

/*
 *	Copy the range specified by src_addr/len
 *	from the source map to the range dst_addr/len
 *	in the destination map.
 *
 *	This routine is only advisory and need not do anything.
 */
void
pmap_copy(pmap_t dst_pmap, pmap_t src_pmap, vaddr_t dst_addr, vsize_t len,
    vaddr_t src_addr)
{

	PMAP_COUNT(copy);
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy(%p, %p, %#"PRIxVADDR", %#"PRIxVSIZE", %#"PRIxVADDR")\n",
		    dst_pmap, src_pmap, dst_addr, len, src_addr);
#endif
}

/*
 *	pmap_zero_page zeros the specified page.
 */
void
pmap_zero_page(paddr_t dst_pa)
{
	vaddr_t dst_va;
	pt_entry_t dst_tmp;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_zero_page(%#"PRIxPADDR")\n", dst_pa);
#endif
	PMAP_COUNT(zeroed_pages);

	struct vm_page *dst_pg = PHYS_TO_VM_PAGE(dst_pa);

	dst_va = pmap_map_ephemeral_page(dst_pg, VM_PROT_READ|VM_PROT_WRITE, &dst_tmp);

	mips_pagezero((void *)dst_va);

	pmap_unmap_ephemeral_page(dst_pg, dst_va, dst_tmp);
}

/*
 *	pmap_copy_page copies the specified page.
 */
void
pmap_copy_page(paddr_t src_pa, paddr_t dst_pa)
{
	vaddr_t src_va, dst_va;
	pt_entry_t src_tmp, dst_tmp;
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy_page(%#"PRIxPADDR", %#"PRIxPADDR")\n", src_pa, dst_pa);
#endif
	struct vm_page *src_pg = PHYS_TO_VM_PAGE(src_pa);
	struct vm_page *dst_pg = PHYS_TO_VM_PAGE(dst_pa);

	PMAP_COUNT(copied_pages);

	src_va = pmap_map_ephemeral_page(src_pg, VM_PROT_READ, &src_tmp);
	dst_va = pmap_map_ephemeral_page(dst_pg, VM_PROT_READ|VM_PROT_WRITE, &dst_tmp);

	mips_pagecopy((void *)dst_va, (void *)src_va);

	pmap_unmap_ephemeral_page(dst_pg, dst_va, dst_tmp);
	pmap_unmap_ephemeral_page(src_pg, src_va, src_tmp);
}

/*
 *	pmap_clear_reference:
 *
 *	Clear the reference bit on the specified physical page.
 */
bool
pmap_clear_reference(struct vm_page *pg)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_reference(%#"PRIxPADDR")\n",
		    VM_PAGE_TO_PHYS(pg));
#endif
	return pmap_clear_mdpage_attributes(md, PG_MD_REFERENCED);
}

/*
 *	pmap_is_referenced:
 *
 *	Return whether or not the specified physical page is referenced
 *	by any physical maps.
 */
bool
pmap_is_referenced(struct vm_page *pg)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);

	return PG_MD_REFERENCED_P(md);
}

/*
 *	Clear the modify bits on the specified physical page.
 */
bool
pmap_clear_modify(struct vm_page *pg)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pv = &md->pvh_first;
	pv_entry_t pv_next;
	uint16_t gen;

	PMAP_COUNT(clear_modify);
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_modify(%#"PRIxPADDR")\n", VM_PAGE_TO_PHYS(pg));
#endif
	if (PG_MD_EXECPAGE_P(md)) {
		if (pv->pv_pmap == NULL) {
			pmap_clear_mdpage_attributes(md, PG_MD_EXECPAGE);
			PMAP_COUNT(exec_uncached_clear_modify);
		} else {
			pmap_page_syncicache(pg);
			PMAP_COUNT(exec_synced_clear_modify);
		}
	}
	if (!pmap_clear_mdpage_attributes(md, PG_MD_MODIFIED))
		return false;
	if (pv->pv_pmap == NULL) {
		return true;
	}

	/*
	 * remove write access from any pages that are dirty
	 * so we can tell if they are written to again later.
	 * flush the VAC first if there is one.
	 */
	kpreempt_disable();
	gen = PG_MD_PVLIST_LOCK(md, false);
	for (; pv != NULL; pv = pv_next) {
		pmap_t pmap = pv->pv_pmap;
		vaddr_t va = trunc_page(pv->pv_va);
		pt_entry_t *pte;
		uint32_t pt_entry;

		pv_next = pv->pv_next;
		if (pv->pv_va & PV_KENTER)
			continue;
		if (pmap == pmap_kernel()) {
			pte = kvtopte(va);
		} else {
			pte = pmap_pte_lookup(pmap, va);
			KASSERT(pte);
		}
		pt_entry = pte->pt_entry & ~mips_pg_m_bit();
		if (pte->pt_entry == pt_entry) {
			continue;
		}
		KASSERT(mips_pg_v(pt_entry));
		/*
		 * Why? Why?
		 */
		if (MIPS_HAS_R4K_MMU && MIPS_CACHE_VIRTUAL_ALIAS) {
			if (PMAP_IS_ACTIVE(pmap)) {
				mips_dcache_wbinv_range(va, PAGE_SIZE);
			} else {
				mips_dcache_wbinv_range_index(va, PAGE_SIZE);
			}
		}
		pte->pt_entry = pt_entry;
		PG_MD_PVLIST_UNLOCK(md);
		pmap_tlb_invalidate_addr(pmap, va);
		pmap_update(pmap);
		if (__predict_false(gen != PG_MD_PVLIST_LOCK(md, false))) {
			/*
			 * The list changed!  So restart from the beginning.
			 */
			pv_next = &md->pvh_first;
		}
	}
	PG_MD_PVLIST_UNLOCK(md);
	kpreempt_enable();
	return true;
}

/*
 *	pmap_is_modified:
 *
 *	Return whether or not the specified physical page is modified
 *	by any physical maps.
 */
bool
pmap_is_modified(struct vm_page *pg)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);

	return PG_MD_MODIFIED_P(md);
}

/*
 *	pmap_set_modified:
 *
 *	Sets the page modified reference bit for the specified page.
 */
void
pmap_set_modified(paddr_t pa)
{
	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pmap_set_mdpage_attributes(md, PG_MD_MODIFIED | PG_MD_REFERENCED);
}

/******************** pv_entry management ********************/

static void
pmap_check_alias(struct vm_page *pg)
{
#ifdef MIPS3_PLUS	/* XXX mmu XXX */
#ifndef MIPS3_NO_PV_UNCACHED
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);

	if (MIPS_HAS_R4K_MMU && PG_MD_UNCACHED_P(md)) {
		/*
		 * Page is currently uncached, check if alias mapping has been
		 * removed.  If it was, then reenable caching.
		 */
		pv_entry_t pv = &md->pvh_first;
		pv_entry_t pv0 = pv->pv_next;

		for (; pv0; pv0 = pv0->pv_next) {
			if (mips_cache_badalias(pv->pv_va, pv0->pv_va))
				break;
		}
		if (pv0 == NULL)
			pmap_page_cache(pg, true);
	}
#endif
#endif	/* MIPS3_PLUS */
}

static void
pmap_check_pvlist(struct vm_page_md *md)
{
#ifdef PARANOIADIAG
	pv_entry_t pv = &md->pvh_first;
	if (pv->pv_pmap != NULL) {
		for (; pv != NULL; pv = pv->pv_next) {
			KASSERT(!MIPS_KSEG0_P(pv->pv_va));
			KASSERT(!MIPS_KSEG1_P(pv->pv_va));
#ifdef _LP64
			KASSERT(!MIPS_XKPHYS_P(pv->pv_va));
#endif
			pv_entry_t opv = &md->pvh_first;
			for (; opv != NULL; opv = opv->pv_next) {
				if (mips_cache_badalias(pv->pv_va, opv->pv_va)) {
					KASSERT(PG_MD_UNCACHED_P(md));
				}
			}
		}
	}
#endif /* PARANOIADIAG */
}

/*
 * Enter the pmap and virtual address into the
 * physical to virtual map table.
 */
void
pmap_enter_pv(pmap_t pmap, vaddr_t va, struct vm_page *pg, u_int *npte,
    int flags)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pv, npv, apv;
	int16_t gen;

	KASSERT(kpreempt_disabled());
	KASSERT(!MIPS_KSEG0_P(va));
	KASSERT(!MIPS_KSEG1_P(va));
#ifdef _LP64
	KASSERT(!MIPS_XKPHYS_P(va));
#endif

	apv = NULL;
	pv = &md->pvh_first;
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("pmap_enter: pv %p: was %#"PRIxVADDR"/%p/%p\n",
		    pv, pv->pv_va, pv->pv_pmap, pv->pv_next);
#endif
	gen = PG_MD_PVLIST_LOCK(md, true);
	pmap_check_pvlist(md);
#if defined(MIPS3_NO_PV_UNCACHED) || defined(MULTIPROCESSOR)
again:
#endif
	if (pv->pv_pmap == NULL) {
		KASSERT(pv->pv_next == NULL);
		/*
		 * No entries yet, use header as the first entry
		 */
#ifdef DEBUG
		if (pmapdebug & PDB_PVENTRY)
			printf("pmap_enter_pv: first pv: pmap %p va %#"PRIxVADDR"\n",
			    pmap, va);
#endif
		PMAP_COUNT(primary_mappings);
		PMAP_COUNT(mappings);
		pmap_clear_mdpage_attributes(md, PG_MD_UNCACHED);
		pv->pv_pmap = pmap;
		pv->pv_va = va | flags;
	} else {
#if defined(MIPS3_PLUS) && !defined(MULTIPROCESSOR) /* XXX mmu XXX */
		if (MIPS_CACHE_VIRTUAL_ALIAS) {
			/*
			 * There is at least one other VA mapping this page.
			 * Check if they are cache index compatible.
			 */

#if defined(MIPS3_NO_PV_UNCACHED)

			/*
			 * Instead of mapping uncached, which some platforms
			 * cannot support, remove the mapping from the pmap.
			 * When this address is touched again, the uvm will
			 * fault it in.  Because of this, each page will only
			 * be mapped with one index at any given time.
			 */

			for (npv = pv; npv; npv = npv->pv_next) {
				vaddr_t nva = trunc_page(npv->pv_va);
				pmap_t npm = npv->pv_pmap;
				if (mips_cache_badalias(nva, va)) {
					pmap_remove(npm, nva, nva + PAGE_SIZE);
					pmap_update(npm);
					goto again;
				}
			}
#else	/* !MIPS3_NO_PV_UNCACHED */
			if (PG_MD_CACHED_P(md)) {
				/*
				 * If this page is cached, then all mappings
				 * have the same cache alias so we only need
				 * to check the first page to see if it's
				 * incompatible with the new mapping.
				 *
				 * If the mappings are incompatible, map this
				 * page as uncached and re-map all the current
				 * mapping as uncached until all pages can
				 * share the same cache index again.
				 */
				if (mips_cache_badalias(pv->pv_va, va)) {
   					vaddr_t nva = trunc_page(pv->pv_va);
					pmap_page_cache(pg, false);
					mips_dcache_wbinv_range_index(nva,
					    PAGE_SIZE);
					*npte = (*npte &
					    ~MIPS3_PG_CACHEMODE) |
					    MIPS3_PG_UNCACHED;
					PMAP_COUNT(page_cache_evictions);
				}
			} else {
				*npte = (*npte & ~MIPS3_PG_CACHEMODE) |
				    MIPS3_PG_UNCACHED;
				PMAP_COUNT(page_cache_evictions);
			}
#endif	/* !MIPS3_NO_PV_UNCACHED */
		}
#endif /* MIPS3_PLUS && !MULTIPROCESSOR */

		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 *
		 * Note: the entry may already be in the table if
		 * we are only changing the protection bits.
		 */

		for (npv = pv; npv; npv = npv->pv_next) {
			if (pmap == npv->pv_pmap &&
			    va == trunc_page(npv->pv_va)) {
#ifdef PARANOIADIAG
				pt_entry_t *pte;
				uint32_t pt_entry;

				if (pmap == pmap_kernel()) {
					pt_entry = kvtopte(va)->pt_entry;
				} else {
					pte = pmap_pte_lookup(pmap, va);
					if (pte) {
						pt_entry = pte->pt_entry;
					} else
						pt_entry = 0;
				}
				if (!mips_pg_v(pt_entry) ||
				    mips_tlbpfn_to_paddr(pt_entry) !=
				    VM_PAGE_TO_PHYS(pg))
					printf(
		"pmap_enter_pv: found va %#"PRIxVADDR" pa %#"PRIxPADDR" in pv_table but != %x\n",
					    va, VM_PAGE_TO_PHYS(pg),
					    pt_entry);
#endif
				PMAP_COUNT(remappings);
				PG_MD_PVLIST_UNLOCK(md);
				if (__predict_false(apv != NULL))
					pmap_pv_free(apv);
				return;
			}
		}
#ifdef DEBUG
		if (pmapdebug & PDB_PVENTRY)
			printf("pmap_enter_pv: new pv: pmap %p va %#"PRIxVADDR"\n",
			    pmap, va);
#endif
		if (__predict_true(apv == NULL)) {
#if defined(MULTIPROCESSOR) || !defined(_LP64) || defined(PMAP_POOLPAGE_DEBUG) || defined(LOCKDEBUG)
			/*
			 * To allocate a PV, we have to release the PVLIST lock
			 * so get the page generation.  We allocate the PV, and
			 * then reacquire the lock.
			 */
			PG_MD_PVLIST_UNLOCK(md);
#endif
			apv = (pv_entry_t)pmap_pv_alloc();
			if (apv == NULL)
				panic("pmap_enter_pv: pmap_pv_alloc() failed");
#if defined(MULTIPROCESSOR) || !defined(_LP64) || defined(PMAP_POOLPAGE_DEBUG) || defined(LOCKDEBUG)
#ifdef MULTIPROCESSOR
			/*
			 * If the generation has changed, then someone else
			 * tinkered with this page so we should
			 * start over.
			 */
			uint16_t oldgen = gen;
#endif
			gen = PG_MD_PVLIST_LOCK(md, true);
#ifdef MULTIPROCESSOR
			if (gen != oldgen)
				goto again;
#endif
#endif
		}
		npv = apv;
		apv = NULL;
		npv->pv_va = va | flags;
		npv->pv_pmap = pmap;
		npv->pv_next = pv->pv_next;
		pv->pv_next = npv;
		PMAP_COUNT(mappings);
	}
	pmap_check_pvlist(md);
	PG_MD_PVLIST_UNLOCK(md);
	if (__predict_false(apv != NULL))
		pmap_pv_free(apv);
}

/*
 * Remove this page from all physical maps in which it resides.
 * Reflects back modify bits to the pager.
 */
void
pmap_page_remove(struct vm_page *pg)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);

	(void)PG_MD_PVLIST_LOCK(md, true);
	pmap_check_pvlist(md);

	pv_entry_t pv = &md->pvh_first;
	if (pv->pv_pmap == NULL) {
		PG_MD_PVLIST_UNLOCK(md);
		return;
	}

	pv_entry_t npv;
	pv_entry_t pvp = NULL;

	kpreempt_disable();
	for (; pv != NULL; pv = npv) {
		npv = pv->pv_next;
		if (pv->pv_va & PV_KENTER) {
#ifdef DEBUG
			if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT)) {
				printf("%s: %p %p, %"PRIxVADDR" skip\n",
				    __func__, pv, pv->pv_pmap, pv->pv_va);
			}
#endif
			KASSERT(pv->pv_pmap == pmap_kernel());

			/* Assume no more - it'll get fixed if there are */
			pv->pv_next = NULL;

			/*
			 * pvp is non-null when we already have a PV_KENTER
			 * pv in pvh_first; otherwise we haven't seen a
			 * PV_KENTER pv and we need to copy this one to
			 * pvh_first
			 */
			if (pvp) {
				/*
				 * The previous PV_KENTER pv needs to point to
				 * this PV_KENTER pv
				 */
				pvp->pv_next = pv;
			} else {
				pv_entry_t fpv = &md->pvh_first;
				*fpv = *pv;
				KASSERT(fpv->pv_pmap == pmap_kernel());
			}
			pvp = pv;
			continue;
		}

		const pmap_t pmap = pv->pv_pmap;
		vaddr_t va = trunc_page(pv->pv_va);
		pt_entry_t *pte = pmap_pte(pmap, va);

#ifdef DEBUG
		if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT)) {
			printf("%s: %p %p, %"PRIxVADDR", %p\n",
			    __func__, pv, pmap, va, pte);
		}
#endif

		KASSERT(pte);
		if (mips_pg_wired(pte->pt_entry))
			pmap->pm_stats.wired_count--;
		pmap->pm_stats.resident_count--;

		if (pmap == pmap_kernel()) {
			if (MIPS_HAS_R4K_MMU)
				/* See above about G bit */
				pte->pt_entry = MIPS3_PG_NV | MIPS3_PG_G;
			else
				pte->pt_entry = MIPS1_PG_NV;
		} else {
			pte->pt_entry = mips_pg_nv_bit();
		}
		/*
		 * Flush the TLB for the given address.
		 */
		pmap_tlb_invalidate_addr(pmap, va);

		/*
		 * non-null means this is a non-pvh_first pv, so we should
		 * free it.
		 */
		if (pvp) {
			KASSERT(pvp->pv_pmap == pmap_kernel());
			KASSERT(pvp->pv_next == NULL);
			pmap_pv_free(pv);
		} else {
			pv->pv_pmap = NULL;
			pv->pv_next = NULL;
		}
	}
	pmap_check_alias(pg);

	pmap_check_pvlist(md);

	kpreempt_enable();
	PG_MD_PVLIST_UNLOCK(md);
}

/*
 * Remove a physical to virtual address translation.
 * If cache was inhibited on this page, and there are no more cache
 * conflicts, restore caching.
 * Flush the cache if the last page is removed (should always be cached
 * at this point).
 */
void
pmap_remove_pv(pmap_t pmap, vaddr_t va, struct vm_page *pg, bool dirty)
{
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pv_entry_t pv, npv;
	bool last;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PVENTRY))
		printf("pmap_remove_pv(%p, %#"PRIxVADDR", %#"PRIxPADDR")\n", pmap, va,
		    VM_PAGE_TO_PHYS(pg));
#endif
	KASSERT(kpreempt_disabled());
	KASSERT((va & PAGE_MASK) == 0);
	pv = &md->pvh_first;

	(void)PG_MD_PVLIST_LOCK(md, true);
	pmap_check_pvlist(md);

	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */

	last = false;
	if (pmap == pv->pv_pmap && va == trunc_page(pv->pv_va)) {
		npv = pv->pv_next;
		if (npv) {
			*pv = *npv;
			KASSERT(pv->pv_pmap != NULL);
		} else {
			pv->pv_pmap = NULL;
			last = true;	/* Last mapping removed */
		}
		PMAP_COUNT(remove_pvfirst);
	} else {
		for (npv = pv->pv_next; npv; pv = npv, npv = npv->pv_next) {
			PMAP_COUNT(remove_pvsearch);
			if (pmap == npv->pv_pmap &&
			    va == trunc_page(npv->pv_va))
				break;
		}
		if (npv) {
			pv->pv_next = npv->pv_next;
		}
	}
	pmap_check_alias(pg);

	pmap_check_pvlist(md);
	PG_MD_PVLIST_UNLOCK(md);

	/*
	 * Free the pv_entry if needed.
	 */
	if (npv)
		pmap_pv_free(npv);
	if (PG_MD_EXECPAGE_P(md) && dirty) {
		if (last) {
			/*
			 * If this was the page's last mapping, we no longer
			 * care about its execness.
			 */
			pmap_clear_mdpage_attributes(md, PG_MD_EXECPAGE);
			PMAP_COUNT(exec_uncached_remove);
		} else {
			/*
			 * Someone still has it mapped as an executable page
			 * so we must sync it.
			 */
			pmap_page_syncicache(pg);
			PMAP_COUNT(exec_synced_remove);
		}
	}
#ifdef MIPS3_PLUS	/* XXX mmu XXX */
	if (MIPS_HAS_R4K_MMU && last)	/* XXX why */
		mips_dcache_wbinv_range_index(va, PAGE_SIZE);
#endif	/* MIPS3_PLUS */
}

#ifdef MULTIPROCESSOR
struct pmap_pvlist_info {
	kmutex_t *pli_locks[PAGE_SIZE / 32];
	volatile u_int pli_lock_refs[PAGE_SIZE / 32];
	volatile u_int pli_lock_index;
	u_int pli_lock_mask;
} pmap_pvlist_info;

static void
pmap_pvlist_lock_init(void)
{
	struct pmap_pvlist_info * const pli = &pmap_pvlist_info;
	const vaddr_t lock_page = uvm_pageboot_alloc(PAGE_SIZE);
	vaddr_t lock_va = lock_page;
	size_t cache_line_size = mips_cache_info.mci_pdcache_line_size;
	if (sizeof(kmutex_t) > cache_line_size) {
		cache_line_size = roundup2(sizeof(kmutex_t), cache_line_size);
	}
	const size_t nlocks = PAGE_SIZE / cache_line_size;
	KASSERT((nlocks & (nlocks - 1)) == 0);
	/*
	 * Now divide the page into a number of mutexes, one per cacheline.
	 */
	for (size_t i = 0; i < nlocks; lock_va += cache_line_size, i++) {
		kmutex_t * const lock = (kmutex_t *)lock_va;
		mutex_init(lock, MUTEX_DEFAULT, IPL_VM);
		pli->pli_locks[i] = lock;
	}
	pli->pli_lock_mask = nlocks - 1;
}

uint16_t
pmap_pvlist_lock(struct vm_page_md *md, bool list_change)
{
	struct pmap_pvlist_info * const pli = &pmap_pvlist_info;
	kmutex_t *lock = md->pvh_lock;
	int16_t gen;

	/*
	 * Allocate a lock on an as-needed basis.  This will hopefully give us
	 * semi-random distribution not based on page color.
	 */
	if (__predict_false(lock == NULL)) {
		size_t locknum = atomic_add_int_nv(&pli->pli_lock_index, 37);
		size_t lockid = locknum & pli->pli_lock_mask;
		kmutex_t * const new_lock = pli->pli_locks[lockid];
		/*
		 * Set the lock.  If some other thread already did, just use
		 * the one they assigned.
		 */
		lock = atomic_cas_ptr(&md->pvh_lock, NULL, new_lock);
		if (lock == NULL) {
			lock = new_lock;
			atomic_inc_uint(&pli->pli_lock_refs[lockid]);
		}
	}

	/*
	 * Now finally lock the pvlists.
	 */
	mutex_spin_enter(lock);

	/*
	 * If the locker will be changing the list, increment the high 16 bits
	 * of attrs so we use that as a generation number.
	 */
	gen = PG_MD_PVLIST_GEN(md);		/* get old value */
	if (list_change)
		atomic_add_int(&md->pvh_attrs, 0x10000);

	/*
	 * Return the generation number.
	 */
	return gen;
}
#else
static void
pmap_pvlist_lock_init(void)
{
	mutex_init(&pmap_pvlist_mutex, MUTEX_DEFAULT, IPL_VM);
}
#endif /* MULTIPROCESSOR */

/*
 * pmap_pv_page_alloc:
 *
 *	Allocate a page for the pv_entry pool.
 */
void *
pmap_pv_page_alloc(struct pool *pp, int flags)
{
	struct vm_page * const pg = PMAP_ALLOC_POOLPAGE(UVM_PGA_USERESERVE);
	if (pg == NULL)
		return NULL;

	return (void *)mips_pmap_map_poolpage(VM_PAGE_TO_PHYS(pg));
}

/*
 * pmap_pv_page_free:
 *
 *	Free a pv_entry pool page.
 */
void
pmap_pv_page_free(struct pool *pp, void *v)
{
	vaddr_t va = (vaddr_t)v;
	paddr_t pa;

#ifdef _LP64
	KASSERT(MIPS_XKPHYS_P(va));
	pa = MIPS_XKPHYS_TO_PHYS(va);
#else
	KASSERT(MIPS_KSEG0_P(va));
	pa = MIPS_KSEG0_TO_PHYS(va);
#endif
#ifdef MIPS3_PLUS
	if (MIPS_CACHE_VIRTUAL_ALIAS) {
		KASSERT((va & PAGE_MASK) == 0);
		mips_dcache_inv_range(va, PAGE_SIZE);
	}
#endif
	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	KASSERT(pg != NULL);
	pmap_clear_mdpage_attributes(VM_PAGE_TO_MD(pg), PG_MD_POOLPAGE);
	uvm_pagefree(pg);
}

pt_entry_t *
pmap_pte(pmap_t pmap, vaddr_t va)
{
	pt_entry_t *pte;

	if (pmap == pmap_kernel())
		pte = kvtopte(va);
	else
		pte = pmap_pte_lookup(pmap, va);
	return pte;
}

#ifdef MIPS3_PLUS	/* XXX mmu XXX */
/*
 * Find first virtual address >= *vap that doesn't cause
 * a cache alias conflict.
 */
void
pmap_prefer(vaddr_t foff, vaddr_t *vap, vsize_t sz, int td)
{
	const struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t	va;
	vsize_t d;
	vsize_t prefer_mask = ptoa(uvmexp.colormask);

	PMAP_COUNT(prefer_requests);

	if (MIPS_HAS_R4K_MMU) {
		prefer_mask |= mci->mci_cache_prefer_mask;
	}

	if (prefer_mask) {
		va = *vap;

		d = foff - va;
		d &= prefer_mask;
		if (d) {
			if (td)
				*vap = trunc_page(va -((-d) & prefer_mask));
			else
				*vap = round_page(va + d);
			PMAP_COUNT(prefer_adjustments);
		}
	}
}
#endif	/* MIPS3_PLUS */

struct vm_page *
mips_pmap_alloc_poolpage(int flags)
{
	/*
	 * On 32bit kernels, we must make sure that we only allocate pages that
	 * can be mapped via KSEG0.  On 64bit kernels, try to allocated from
	 * the first 4G.  If all memory is in KSEG0/4G, then we can just
	 * use the default freelist otherwise we must use the pool page list.
	 */
	if (mips_poolpage_vmfreelist != VM_FREELIST_DEFAULT)
		return uvm_pagealloc_strat(NULL, 0, NULL, flags,
		    UVM_PGA_STRAT_ONLY, mips_poolpage_vmfreelist);

	return uvm_pagealloc(NULL, 0, NULL, flags);
}

vaddr_t
mips_pmap_map_poolpage(paddr_t pa)
{
	vaddr_t va;

	struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
	KASSERT(pg);
	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	pmap_set_mdpage_attributes(md, PG_MD_POOLPAGE);

#ifdef PMAP_POOLPAGE_DEBUG
	KASSERT((poolpage.hint & MIPS_CACHE_ALIAS_MASK) == 0);
	vaddr_t va_offset = poolpage.hint + mips_cache_indexof(pa);
	pt_entry_t *pte = poolpage.sysmap + atop(va_offset);
	const size_t va_inc = MIPS_CACHE_ALIAS_MASK + PAGE_SIZE;
	const size_t pte_inc = atop(va_inc);

	for (; va_offset < poolpage.size;
	     va_offset += va_inc, pte += pte_inc) {
		if (!mips_pg_v(pte->pt_entry))
			break;
	}
	if (va_offset >= poolpage.size) {
		for (va_offset -= poolpage.size, pte -= atop(poolpage.size);
		     va_offset < poolpage.hint;
		     va_offset += va_inc, pte += pte_inc) {
			if (!mips_pg_v(pte->pt_entry))
				break;
		}
	}
	KASSERT(!mips_pg_v(pte->pt_entry));
	va = poolpage.base + va_offset;
	poolpage.hint = roundup2(va_offset + 1, va_inc);
	pmap_kenter_pa(va, pa, VM_PROT_READ|VM_PROT_WRITE, 0);
#else
#ifdef _LP64
	KASSERT(mips_options.mips3_xkphys_cached);
	va = MIPS_PHYS_TO_XKPHYS_CACHED(pa);
#else
	if (pa > MIPS_PHYS_MASK)
		panic("mips_pmap_map_poolpage: "
		    "pa #%"PRIxPADDR" can not be mapped into KSEG0", pa);

	va = MIPS_PHYS_TO_KSEG0(pa);
#endif
#endif
#if !defined(_LP64) || defined(PMAP_POOLPAGE_DEBUG)
	if (MIPS_CACHE_VIRTUAL_ALIAS) {
		/*
		 * If this page was last mapped with an address that might
		 * cause aliases, flush the page from the cache.
		 */
		(void)PG_MD_PVLIST_LOCK(md, false);
		pv_entry_t pv = &md->pvh_first;
		vaddr_t last_va = trunc_page(pv->pv_va);
		KASSERT(pv->pv_pmap == NULL);
		pv->pv_va = va;
		if (PG_MD_CACHED_P(md) && mips_cache_badalias(last_va, va))
			mips_dcache_wbinv_range_index(last_va, PAGE_SIZE);
		PG_MD_PVLIST_UNLOCK(md);
	}
#endif
	return va;
}

paddr_t
mips_pmap_unmap_poolpage(vaddr_t va)
{
	paddr_t pa;
#ifdef PMAP_POOLPAGE_DEBUG
	KASSERT(poolpage.base <= va && va < poolpage.base + poolpage.size);
	pa = mips_tlbpfn_to_paddr(kvtopte(va)->pt_entry);
#elif defined(_LP64)
	KASSERT(MIPS_XKPHYS_P(va));
	pa = MIPS_XKPHYS_TO_PHYS(va);
#else
	KASSERT(MIPS_KSEG0_P(va));
	pa = MIPS_KSEG0_TO_PHYS(va);
#endif
	struct vm_page *pg = PHYS_TO_VM_PAGE(pa);
	KASSERT(pg);
	pmap_clear_mdpage_attributes(VM_PAGE_TO_MD(pg), PG_MD_POOLPAGE);
#if defined(MIPS3_PLUS)
	if (MIPS_CACHE_VIRTUAL_ALIAS) {
		/*
		 * We've unmapped a poolpage.  Its contents are irrelevant.
		 */

		KASSERT((va & PAGE_MASK) == 0);
		mips_dcache_inv_range(va, PAGE_SIZE);
	}
#endif
#ifdef PMAP_POOLPAGE_DEBUG
	pmap_kremove(va, PAGE_SIZE);
#endif
	return pa;
}



/******************** page table page management ********************/

/* TO BE DONE */
