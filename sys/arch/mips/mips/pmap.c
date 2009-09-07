/*	$NetBSD: pmap.c,v 1.179.16.3 2009/09/07 22:08:31 matt Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.179.16.3 2009/09/07 22:08:31 matt Exp $");

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
#include "opt_mips_cache.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/pool.h>
#include <sys/mutex.h>
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

CTASSERT(NBPG >= sizeof(struct segtab));
#ifdef DEBUG
struct {
	int kernel;	/* entering kernel mapping */
	int user;	/* entering user mapping */
	int ptpneeded;	/* needed to allocate a PT page */
	int pwchange;	/* no mapping change, just wiring or protection */
	int wchange;	/* no mapping change, just wiring */
	int mchange;	/* was mapped but mapping to different page */
	int managed;	/* a managed page */
	int firstpv;	/* first mapping for this PA */
	int secondpv;	/* second mapping for this PA */
	int ci;		/* cache inhibited */
	int unmanaged;	/* not a managed page */
	int flushes;	/* cache flushes */
	int cachehit;	/* new entry forced valid entry out */
} enter_stats;
struct {
	int calls;
	int removes;
	int flushes;
	int pidflushes;	/* HW pid stolen */
	int pvfirst;
	int pvsearch;
} remove_stats;

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

#endif

struct pmap	kernel_pmap_store;

paddr_t avail_start;	/* PA of first available physical page */
paddr_t avail_end;	/* PA of last available physical page */
vaddr_t virtual_end;	/* VA of last avail page (end of kernel AS) */

struct pv_entry	*pv_table;
int		 pv_table_npages;

struct segtab	*free_segtab;		/* free list kept locally */
pt_entry_t	*Sysmap;		/* kernel pte table */
unsigned int	Sysmapsize;		/* number of pte's in Sysmap */

unsigned pmap_max_asid;			/* max ASID supported by the system */
unsigned pmap_next_asid;		/* next free ASID to use */
unsigned pmap_asid_generation;		/* current ASID generation */
#define PMAP_ASID_RESERVED 0

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

#define PAGE_IS_MANAGED(pa)	\
	(pmap_initialized == true && vm_physseg_find(atop(pa), NULL) != -1)

#define PMAP_IS_ACTIVE(pm)						\
	((pm) == pmap_kernel() || 					\
	 (pm) == curlwp->l_proc->p_vmspace->vm_map.pmap)

/* Forward function declarations */
void pmap_remove_pv(pmap_t, vaddr_t, struct vm_page *);
void pmap_asid_alloc(pmap_t pmap);
void pmap_enter_pv(pmap_t, vaddr_t, struct vm_page *, u_int *);
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

#if defined(MIPS3_PLUS)	/* XXX mmu XXX */
void mips_dump_segtab(struct proc *);
static void mips_flushcache_allpvh(paddr_t);

/*
 * Flush virtual addresses associated with a given physical address
 */
static void
mips_flushcache_allpvh(paddr_t pa)
{
	struct vm_page *pg;
	struct pv_entry *pv;

	pg = PHYS_TO_VM_PAGE(pa);
	if (pg == NULL) {
		/* page is unmanaged */
#ifdef DIAGNOSTIC
		printf("mips_flushcache_allpvh(): unmanaged pa = %#"PRIxPADDR"\n",
		    pa);
#endif
		return;
	}

	pv = pg->mdpage.pvh_list;

#if defined(MIPS3_NO_PV_UNCACHED)
	/* No current mapping.  Cache was flushed by pmap_remove_pv() */
	if (pv->pv_pmap == NULL)
		return;

	/* Only one index is allowed at a time */
	if (mips_cache_indexof(pa) != mips_cache_indexof(pv->pv_va))
		mips_dcache_wbinv_range_index(pv->pv_va, NBPG);
#else
	while (pv) {
		mips_dcache_wbinv_range_index(pv->pv_va, NBPG);
		pv = pv->pv_next;
	}
#endif
}
#endif /* MIPS3_PLUS */

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	firstaddr is the first unused kseg0 address (not page aligned).
 */
void
pmap_bootstrap(void)
{
	vsize_t bufsz;

	/*
	 * Compute the number of pages kmem_map will have.
	 */
	kmeminit_nkmempages();

	/*
	 * Figure out how many PTE's are necessary to map the kernel.
	 * We also reserve space for kmem_alloc_pageable() for vm_fork().
	 */

	/* Get size of buffer cache and set an upper limit */
	bufsz = buf_memcalc();
	buf_setvalimit(bufsz);

	Sysmapsize = (VM_PHYS_SIZE + (ubc_nwins << ubc_winshift) +
	    bufsz + 16 * NCARGS + pager_map_size) / NBPG +
	    (maxproc * UPAGES) + nkmempages;

#ifdef SYSVSHM
	Sysmapsize += shminfo.shmall;
#endif
#ifdef KSEG2IOBUFSIZE
	Sysmapsize += (KSEG2IOBUFSIZE >> PGSHIFT);
#endif
	/* XXX: else runs out of space on 256MB sbmips!! */
	Sysmapsize += 20000;

	/*
	 * Initialize `FYI' variables.	Note we're relying on
	 * the fact that BSEARCH sorts the vm_physmem[] array
	 * for us.  Must do this before uvm_pageboot_alloc()
	 * can be called.
	 */
	avail_start = ptoa(vm_physmem[0].start);
	avail_end = ptoa(vm_physmem[vm_nphysseg - 1].end);
	virtual_end = VM_MIN_KERNEL_ADDRESS + Sysmapsize * NBPG;

	/*
	 * Now actually allocate the kernel PTE array (must be done
	 * after virtual_end is initialized).
	 */
	Sysmap = (pt_entry_t *)
	    uvm_pageboot_alloc(sizeof(pt_entry_t) * Sysmapsize);

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
	pv_table = (struct pv_entry *)
	    uvm_pageboot_alloc(sizeof(struct pv_entry) * pv_table_npages);

	/*
	 * Initialize the pools.
	 */
	pool_init(&pmap_pmap_pool, sizeof(struct pmap), 0, 0, 0, "pmappl",
	    &pool_allocator_nointr, IPL_NONE);
	pool_init(&pmap_pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pvpl",
	    &pmap_pv_page_allocator, IPL_NONE);

	/*
	 * Initialize the kernel pmap.
	 */
	pmap_kernel()->pm_count = 1;
	pmap_kernel()->pm_asid = PMAP_ASID_RESERVED;
	pmap_kernel()->pm_asidgen = 0;
	pmap_kernel()->pm_segtab = (void *)(MIPS_KSEG2_START + 0x1eadbeef);

	pmap_max_asid = MIPS_TLB_NUM_PIDS;
	pmap_next_asid = 1;
	pmap_asid_generation = 0;

	MachSetPID(0);

#ifdef MIPS3_PLUS	/* XXX mmu XXX */
	/*
	 * The R4?00 stores only one copy of the Global bit in the
	 * translation lookaside buffer for each 2 page entry.
	 * Thus invalid entrys must have the Global bit set so
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
	*vendp = trunc_page(virtual_end);	/* XXX need pmap_growkernel() */
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
	int bank, x;
	u_int npgs;
	paddr_t pa;
	vaddr_t va;

	size = round_page(size);
	npgs = atop(size);

	for (bank = 0; bank < vm_nphysseg; bank++) {
		if (uvm.page_init_done == true)
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

		if (pa <= MIPS_PHYS_MASK)
			va = MIPS_PHYS_TO_KSEG0(pa);
		else
#ifdef _LP64
			va = MIPS_PHYS_TO_XKPHYS(
			    MIPS3_PG_TO_CCA(mips3_pg_cached), pa);
#else
			panic("pmap_steal_memory: "
			      "pa can not be mapped into K0");
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
	vsize_t		s;
	int		bank, i;
	pv_entry_t	pv;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_INIT))
		printf("pmap_init()\n");
#endif

	/*
	 * Memory for the pv entry heads has
	 * already been allocated.  Initialize the physical memory
	 * segments.
	 */
	pv = pv_table;
	for (bank = 0; bank < vm_nphysseg; bank++) {
		s = vm_physmem[bank].end - vm_physmem[bank].start;
		for (i = 0; i < s; i++)
			vm_physmem[bank].pgs[i].mdpage.pvh_list = pv++;
	}

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
	int i;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_create()\n");
#endif

	pmap = pool_get(&pmap_pmap_pool, PR_WAITOK);
	memset(pmap, 0, sizeof(*pmap));

	pmap->pm_count = 1;
	if (free_segtab) {
		pmap->pm_segtab = free_segtab;
		free_segtab = *(struct segtab **)free_segtab;
		pmap->pm_segtab->seg_tab[0] = NULL;
	} else {
		struct segtab *stp;
		struct vm_page *stp_pg;
		paddr_t stp_pa;

		for (;;) {
			stp_pg = uvm_pagealloc(NULL, 0, NULL,
			    UVM_PGA_USERESERVE|UVM_PGA_ZERO);
			if (stp_pg != NULL)
				break;
			/*
			 * XXX What else can we do?  Could we
			 * XXX deadlock here?
			 */
			uvm_wait("pmap_create");
		}

		stp_pa = VM_PAGE_TO_PHYS(stp_pg);
#ifdef _LP64
		if (stp_pa > MIPS_PHYS_MASK)
			stp = (struct segtab *)MIPS_PHYS_TO_XKPHYS(
			    stp_pa, MIPS3_PG_TO_CCA(mips3_pg_cached));
		else
#endif
			stp = (struct segtab *)MIPS_PHYS_TO_KSEG0(stp_pa);
		pmap->pm_segtab = stp;
		i = NBPG / sizeof(struct segtab);
		while (--i != 0) {
			stp++;
			*(struct segtab **)stp = free_segtab;
			free_segtab = stp;
		}
	}
#ifdef PARANOIADIAG
	for (i = 0; i < PMAP_SEGTABSIZE; i++)
		if (pmap->pm_segtab->seg_tab[i] != 0)
			panic("pmap_create: pm_segtab != 0");
#endif
	pmap->pm_asid = PMAP_ASID_RESERVED;
	pmap->pm_asidgen = pmap_asid_generation;

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
	int count;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_destroy(%p)\n", pmap);
#endif
	count = --pmap->pm_count;
	if (count > 0)
		return;

	if (pmap->pm_segtab) {
		pt_entry_t *pte;
		int i;
#ifdef PARANOIADIAG
		int j;
#endif

		for (i = 0; i < PMAP_SEGTABSIZE; i++) {
			paddr_t pa;
			/* get pointer to segment map */
			pte = pmap->pm_segtab->seg_tab[i];
			if (!pte)
				continue;
#ifdef PARANOIADIAG
			for (j = 0; j < NPTEPG; j++) {
				if ((pte + j)->pt_entry)
					panic("pmap_destroy: segmap not empty");
			}
#endif

#ifdef MIPS3_PLUS	/* XXX mmu XXX */
			/*
			 * The pica pmap.c flushed the segmap pages here.  I'm
			 * not sure why, but I suspect it's because the page(s)
			 * were being accessed by KSEG0 (cached) addresses and
			 * may cause cache coherency problems when the page
			 * is reused with KSEG2 (mapped) addresses.  This may
			 * cause problems on machines without VCED/VCEI.
			 */
			if (mips_cache_virtual_alias)
				mips_dcache_inv_range((vaddr_t)pte, PAGE_SIZE);
#endif	/* MIPS3_PLUS */
#ifdef _LP64
			if (MIPS_XKPHYS_P(pte))
				pa = MIPS_XKPHYS_TO_PHYS(pte);
			else
#endif
				pa = MIPS_KSEG0_TO_PHYS(pte);
			uvm_pagefree(PHYS_TO_VM_PAGE(pa));

			pmap->pm_segtab->seg_tab[i] = NULL;
		}
		*(struct segtab **)pmap->pm_segtab = free_segtab;
		free_segtab = pmap->pm_segtab;
		pmap->pm_segtab = NULL;
	}

	pool_put(&pmap_pmap_pool, pmap);
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
		pmap->pm_count++;
	}
}

/*
 *	Make a new pmap (vmspace) active for the given process.
 */
void
pmap_activate(struct lwp *l)
{
	pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;

	pmap_asid_alloc(pmap);
	if (l == curlwp) {
		segbase = pmap->pm_segtab;
		MachSetPID(pmap->pm_asid);
	}
}

/*
 *	Make a previously active pmap (vmspace) inactive.
 */
void
pmap_deactivate(struct lwp *l)
{

	/* Nothing to do. */
}

/*
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the page size.
 */
void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	struct vm_page *pg;
	vaddr_t nssva;
	pt_entry_t *pte;
	unsigned entry;
	unsigned asid, needflush;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove(%p, %#"PRIxVADDR", %#"PRIxVADDR")\n", pmap, sva, eva);
	remove_stats.calls++;
#endif
	if (pmap == pmap_kernel()) {
		/* remove entries from kernel pmap */
#ifdef PARANOIADIAG
		if (sva < VM_MIN_KERNEL_ADDRESS || eva >= virtual_end)
			panic("pmap_remove: kva not in range");
#endif
		pte = kvtopte(sva);
		for (; sva < eva; sva += NBPG, pte++) {
			entry = pte->pt_entry;
			if (!mips_pg_v(entry))
				continue;
			if (mips_pg_wired(entry))
				pmap->pm_stats.wired_count--;
			pmap->pm_stats.resident_count--;
			pg = PHYS_TO_VM_PAGE(mips_tlbpfn_to_paddr(entry));
			if (pg)
				pmap_remove_pv(pmap, sva, pg);
			if (MIPS_HAS_R4K_MMU)
				/* See above about G bit */
				pte->pt_entry = MIPS3_PG_NV | MIPS3_PG_G;
			else
				pte->pt_entry = MIPS1_PG_NV;

			/*
			 * Flush the TLB for the given address.
			 */
			MIPS_TBIS(sva);
#ifdef DEBUG
			remove_stats.flushes++;

#endif
		}
		return;
	}

#ifdef PARANOIADIAG
	if (eva > VM_MAXUSER_ADDRESS)
		panic("pmap_remove: uva not in range");
	if (PMAP_IS_ACTIVE(pmap)) {
		unsigned asid;

		__asm volatile("mfc0 %0,$10; nop" : "=r"(asid));
		asid = (MIPS_HAS_R4K_MMU) ? (asid & 0xff) : (asid & 0xfc0) >> 6;
		if (asid != pmap->pm_asid) {
			panic("inconsistency for active TLB flush: %d <-> %d",
			    asid, pmap->pm_asid);
		}
	}
#endif
	asid = pmap->pm_asid << MIPS_TLB_PID_SHIFT;
	needflush = (pmap->pm_asidgen == pmap_asid_generation);
	while (sva < eva) {
		nssva = mips_trunc_seg(sva) + NBSEG;
		if (nssva == 0 || nssva > eva)
			nssva = eva;
		/*
		 * If VA belongs to an unallocated segment,
		 * skip to the next segment boundary.
		 */
		if (!(pte = pmap_segmap(pmap, sva))) {
			sva = nssva;
			continue;
		}
		/*
		 * Invalidate every valid mapping within this segment.
		 */
		pte += (sva >> PGSHIFT) & (NPTEPG - 1);
		for (; sva < nssva; sva += NBPG, pte++) {
			entry = pte->pt_entry;
			if (!mips_pg_v(entry))
				continue;
			if (mips_pg_wired(entry))
				pmap->pm_stats.wired_count--;
			pmap->pm_stats.resident_count--;
			pg = PHYS_TO_VM_PAGE(mips_tlbpfn_to_paddr(entry));
			if (pg)
				pmap_remove_pv(pmap, sva, pg);
			pte->pt_entry = mips_pg_nv_bit();
			/*
			 * Flush the TLB for the given address.
			 */
			if (needflush) {
				MIPS_TBIS(sva | asid);
#ifdef DEBUG
				remove_stats.flushes++;
#endif
			}
		}
	}
}

/*
 *	pmap_page_protect:
 *
 *	Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	pv_entry_t pv;
	vaddr_t va;

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
		pv = pg->mdpage.pvh_list;
		/*
		 * Loop over all current mappings setting/clearing as appropos.
		 */
		if (pv->pv_pmap != NULL) {
			for (; pv; pv = pv->pv_next) {
				va = pv->pv_va;
				pmap_protect(pv->pv_pmap, va, va + PAGE_SIZE,
				    prot);
				pmap_update(pv->pv_pmap);
			}
		}
		break;

	/* remove_all */
	default:
		pv = pg->mdpage.pvh_list;
		while (pv->pv_pmap != NULL) {
			pmap_remove(pv->pv_pmap, pv->pv_va,
			    pv->pv_va + PAGE_SIZE);
		}
		pmap_update(pv->pv_pmap);
	}
}

/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 */
void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	vaddr_t nssva;
	pt_entry_t *pte;
	unsigned entry;
	u_int p;
	unsigned asid, needupdate;

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
		if (sva < VM_MIN_KERNEL_ADDRESS || eva >= virtual_end)
			panic("pmap_protect: kva not in range");
#endif
		pte = kvtopte(sva);
		for (; sva < eva; sva += NBPG, pte++) {
			entry = pte->pt_entry;
			if (!mips_pg_v(entry))
				continue;
			if (MIPS_HAS_R4K_MMU && entry & mips_pg_m_bit())
				mips_dcache_wb_range(sva, PAGE_SIZE);
			entry &= ~(mips_pg_m_bit() | mips_pg_ro_bit());
			entry |= p;
			pte->pt_entry = entry;
			MachTLBUpdate(sva, entry);
		}
		return;
	}

#ifdef PARANOIADIAG
	if (eva > VM_MAXUSER_ADDRESS)
		panic("pmap_protect: uva not in range");
	if (PMAP_IS_ACTIVE(pmap)) {
		unsigned asid;

		__asm volatile("mfc0 %0,$10; nop" : "=r"(asid));
		asid = (MIPS_HAS_R4K_MMU) ? (asid & 0xff) : (asid & 0xfc0) >> 6;
		if (asid != pmap->pm_asid) {
			panic("inconsistency for active TLB update: %d <-> %d",
			    asid, pmap->pm_asid);
		}
	}
#endif
	asid = pmap->pm_asid << MIPS_TLB_PID_SHIFT;
	needupdate = (pmap->pm_asidgen == pmap_asid_generation);
	while (sva < eva) {
		nssva = mips_trunc_seg(sva) + NBSEG;
		if (nssva == 0 || nssva > eva)
			nssva = eva;
		/*
		 * If VA belongs to an unallocated segment,
		 * skip to the next segment boundary.
		 */
		if (!(pte = pmap_segmap(pmap, sva))) {
			sva = nssva;
			continue;
		}
		/*
		 * Change protection on every valid mapping within this segment.
		 */
		pte += (sva >> PGSHIFT) & (NPTEPG - 1);
		for (; sva < nssva; sva += NBPG, pte++) {
			entry = pte->pt_entry;
			if (!mips_pg_v(entry))
				continue;
			if (MIPS_HAS_R4K_MMU && entry & mips_pg_m_bit())
				mips_dcache_wbinv_range_index(sva, PAGE_SIZE);
			entry = (entry & ~(mips_pg_m_bit() |
			    mips_pg_ro_bit())) | p;
			pte->pt_entry = entry;
			/*
			 * Update the TLB if the given address is in the cache.
			 */
			if (needupdate)
				MachTLBUpdate(sva | asid, entry);
		}
	}
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
		if (p == curlwp->l_proc && mips_pdcache_way_mask < PAGE_SIZE)
		    /* XXX check icache mask too? */
			mips_icache_sync_range(va, len);
		else
			mips_icache_sync_range_index(va, len);
#endif /* MIPS3_PLUS */	/* XXX mmu XXX */
	} else {
#ifdef MIPS1
		pt_entry_t *pte;
		unsigned entry;

		if (pmap == pmap_kernel()) {
			pte = kvtopte(va);
		} else {
			if (!(pte = pmap_segmap(pmap, va))) {
				return;
			}
			pte += (va >> PGSHIFT) & (NPTEPG - 1);
		}
		entry = pte->pt_entry;
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
int
pmap_is_page_ro(pmap_t pmap, vaddr_t va, int entry)
{

	return entry & mips_pg_ro_bit();
}

#if defined(MIPS3_PLUS) && !defined(MIPS3_NO_PV_UNCACHED)	/* XXX mmu XXX */
/*
 *	pmap_page_cache:
 *
 *	Change all mappings of a managed page to cached/uncached.
 */
static void
pmap_page_cache(struct vm_page *pg, int mode)
{
	pv_entry_t pv;
	pt_entry_t *pte;
	unsigned entry;
	unsigned newmode;
	unsigned asid, needupdate;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_page_uncache(%#"PRIxPADDR")\n", VM_PAGE_TO_PHYS(pg));
#endif
	newmode = mode & PV_UNCACHED ? MIPS3_PG_UNCACHED : MIPS3_PG_CACHED;
	pv = pg->mdpage.pvh_list;
	asid = pv->pv_pmap->pm_asid;
	needupdate = (pv->pv_pmap->pm_asidgen == pmap_asid_generation);

	while (pv) {
		pv->pv_flags = (pv->pv_flags & ~PV_UNCACHED) | mode;
		if (pv->pv_pmap == pmap_kernel()) {
		/*
		 * Change entries in kernel pmap.
		 */
			pte = kvtopte(pv->pv_va);
			entry = pte->pt_entry;
			if (entry & MIPS3_PG_V) {
				entry = (entry & ~MIPS3_PG_CACHEMODE) | newmode;
				pte->pt_entry = entry;
				MachTLBUpdate(pv->pv_va, entry);
			}
		} else {

			pte = pmap_segmap(pv->pv_pmap, pv->pv_va);
			if (pte == NULL)
				continue;
			pte += (pv->pv_va >> PGSHIFT) & (NPTEPG - 1);
			entry = pte->pt_entry;
			if (entry & MIPS3_PG_V) {
				entry = (entry & ~MIPS3_PG_CACHEMODE) | newmode;
				pte->pt_entry = entry;
				if (needupdate)
					MachTLBUpdate(pv->pv_va | asid, entry);
			}
		}
		pv = pv->pv_next;
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
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, int flags)
{
	pt_entry_t *pte;
	u_int npte;
	struct vm_page *pg, *mem;
	unsigned asid;
#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
	int cached = 1;
#endif
	bool wired = (flags & PMAP_WIRED) != 0;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_enter(%p, %#"PRIxVADDR", %#"PRIxPADDR", %x, %x)\n",
		    pmap, va, pa, prot, wired);
#endif
#if defined(DEBUG) || defined(DIAGNOSTIC) || defined(PARANOIADIAG)
	if (pmap == pmap_kernel()) {
#ifdef DEBUG
		enter_stats.kernel++;
#endif
		if (va < VM_MIN_KERNEL_ADDRESS || va >= virtual_end)
			panic("pmap_enter: kva too big");
	} else {
#ifdef DEBUG
		enter_stats.user++;
#endif
		if (va >= VM_MAXUSER_ADDRESS)
			panic("pmap_enter: uva too big");
	}
#endif
#ifdef PARANOIADIAG
#if defined(cobalt) || defined(newsmips) || defined(pmax) /* otherwise ok */
	if (pa & 0x80000000)	/* this is not error in general. */
		panic("pmap_enter: pa");
#endif

#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
	if (pa & PMAP_NOCACHE) {
		cached = 0;
		pa &= ~PMAP_NOCACHE;
	}
#endif

	if (!(prot & VM_PROT_READ))
		panic("pmap_enter: prot");
#endif
	pg = PHYS_TO_VM_PAGE(pa);

	if (pg) {
		int *attrs = &pg->mdpage.pvh_attrs;

		/* Set page referenced/modified status based on flags */
		if (flags & VM_PROT_WRITE)
			*attrs |= PV_MODIFIED | PV_REFERENCED;
		else if (flags & VM_PROT_ALL)
			*attrs |= PV_REFERENCED;
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
			if (cached == 0) {
				if (*attrs & PV_MODIFIED) {
					npte = mips_pg_rwncpage_bit();
				} else {
					npte = mips_pg_cwncpage_bit();
				}
			} else {
#endif
				if (*attrs & PV_MODIFIED) {
					npte = mips_pg_rwpage_bit();
				} else {
					npte = mips_pg_cwpage_bit();
				}
#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
			}
#endif
		}
#ifdef DEBUG
		enter_stats.managed++;
#endif
	} else {
		/*
		 * Assumption: if it is not part of our managed memory
		 * then it must be device memory which may be volatile.
		 */
#ifdef DEBUG
		enter_stats.unmanaged++;
#endif
		if (MIPS_HAS_R4K_MMU) {
			npte = MIPS3_PG_IOPAGE(PMAP_CCA_FOR_PA(pa)) &
			    ~MIPS3_PG_G;
			if ((prot & VM_PROT_WRITE) == 0) {
				npte |= MIPS3_PG_RO;
				npte &= ~MIPS3_PG_D;
			}
		} else {
			npte = (prot & VM_PROT_WRITE) ?
			    (MIPS1_PG_D | MIPS1_PG_N) :
			    (MIPS1_PG_RO | MIPS1_PG_N);
		}
	}

	/*
	 * The only time we need to flush the cache is if we
	 * execute from a physical address and then change the data.
	 * This is the best place to do this.
	 * pmap_protect() and pmap_remove() are mostly used to switch
	 * between R/W and R/O pages.
	 * NOTE: we only support cache flush for read only text.
	 */
#ifdef MIPS1
	if ((!MIPS_HAS_R4K_MMU) && prot == (VM_PROT_READ | VM_PROT_EXECUTE)) {
		mips_icache_sync_range(MIPS_PHYS_TO_KSEG0(pa), PAGE_SIZE);
	}
#endif

	if (pmap == pmap_kernel()) {
		if (pg)
			pmap_enter_pv(pmap, va, pg, &npte);

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
		if (mips_pg_v(pte->pt_entry) &&
		    mips_tlbpfn_to_paddr(pte->pt_entry) != pa) {
			pmap_remove(pmap, va, va + NBPG);
#ifdef DEBUG
			enter_stats.mchange++;
#endif
		}
		if (!mips_pg_v(pte->pt_entry))
			pmap->pm_stats.resident_count++;
		pte->pt_entry = npte;

		/*
		 * Update the same virtual address entry.
		 */

		MachTLBUpdate(va, npte);
		return 0;
	}

	if (!(pte = pmap_segmap(pmap, va))) {
		paddr_t phys;
		mem = uvm_pagealloc(NULL, 0, NULL,
				    UVM_PGA_USERESERVE|UVM_PGA_ZERO);
		if (mem == NULL) {
			if (flags & PMAP_CANFAIL)
				return ENOMEM;
			panic("pmap_enter: cannot allocate segmap");
		}

		phys = VM_PAGE_TO_PHYS(mem);
#ifdef _LP64
		if ((vaddr_t)pte > MIPS_PHYS_MASK)
			pte = (pt_entry_t *)MIPS_PHYS_TO_XKPHYS(phys,
			    MIPS3_PG_TO_CCA(mips3_pg_cached));
		else
#endif
			pte = (pt_entry_t *)MIPS_PHYS_TO_KSEG0(phys);
		pmap_segmap(pmap, va) = pte;
#ifdef PARANOIADIAG
	    {
		int i;
		for (i = 0; i < NPTEPG; i++) {
			if ((pte+i)->pt_entry)
				panic("pmap_enter: new segmap not empty");
		}
	    }
#endif
	}

	/* Done after case that may sleep/return. */
	if (pg)
		pmap_enter_pv(pmap, va, pg, &npte);

	pte += (va >> PGSHIFT) & (NPTEPG - 1);

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
		if (pmap->pm_asidgen == pmap_asid_generation)
			printf(" asid %u (%#x)", pmap->pm_asid, pmap->pm_asid);
		printf("\n");
	}
#endif

#ifdef PARANOIADIAG
	if (PMAP_IS_ACTIVE(pmap)) {
		unsigned int asid;

		__asm volatile("mfc0 %0,$10; nop" : "=r"(asid));
		asid = (MIPS_HAS_R4K_MMU) ? (asid & 0xff) : (asid & 0xfc0) >> 6;
		if (asid != pmap->pm_asid) {
			panic("inconsistency for active TLB update: %u <-> %u",
			    asid, pmap->pm_asid);
		}
	}
#endif

	asid = pmap->pm_asid << MIPS_TLB_PID_SHIFT;
	if (mips_pg_v(pte->pt_entry) &&
	    mips_tlbpfn_to_paddr(pte->pt_entry) != pa) {
		pmap_remove(pmap, va, va + NBPG);
#ifdef DEBUG
		enter_stats.mchange++;
#endif
	}

	if (!mips_pg_v(pte->pt_entry))
		pmap->pm_stats.resident_count++;
	pte->pt_entry = npte;

	if (pmap->pm_asidgen == pmap_asid_generation)
		MachTLBUpdate(va | asid, npte);

#ifdef MIPS3_PLUS	/* XXX mmu XXX */
	if (MIPS_HAS_R4K_MMU && (prot == (VM_PROT_READ | VM_PROT_EXECUTE))) {
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("pmap_enter: flush I cache va %#"PRIxVADDR" (%#"PRIxPADDR")\n",
			    va - NBPG, pa);
#endif
		/* XXXJRT */
		mips_icache_sync_range_index(va, PAGE_SIZE);
	}
#endif

	return 0;
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	pt_entry_t *pte;
	u_int npte;
	bool managed = PAGE_IS_MANAGED(pa);

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_kenter_pa(%#"PRIxVADDR", %#"PRIxPADDR", %x)\n", va, pa, prot);
#endif

	if (MIPS_HAS_R4K_MMU) {
		npte = mips3_paddr_to_tlbpfn(pa) | MIPS3_PG_WIRED;
		if (prot & VM_PROT_WRITE) {
			npte |= MIPS3_PG_D;
		} else {
			npte |= MIPS3_PG_RO;
		}
		if (managed) {
			npte |= MIPS3_PG_CACHED;
		} else {
			npte |= MIPS3_PG_UNCACHED;
		}
		npte |= MIPS3_PG_V | MIPS3_PG_G;
	} else {
		npte = mips1_paddr_to_tlbpfn(pa) | MIPS1_PG_WIRED;
		if (prot & VM_PROT_WRITE) {
			npte |= MIPS1_PG_D;
		} else {
			npte |= MIPS1_PG_RO;
		}
		if (managed) {
			npte |= 0;
		} else {
			npte |= MIPS1_PG_N;
		}
		npte |= MIPS1_PG_V | MIPS1_PG_G;
	}
	pte = kvtopte(va);
	KASSERT(!mips_pg_v(pte->pt_entry));
	pte->pt_entry = npte;
	MachTLBUpdate(va, npte);
}

void
pmap_kremove(vaddr_t va, vsize_t len)
{
	pt_entry_t *pte;
	vaddr_t eva;
	u_int entry;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE))
		printf("pmap_kremove(%#"PRIxVADDR", %#"PRIxVSIZE")\n", va, len);
#endif

	pte = kvtopte(va);
	eva = va + len;
	for (; va < eva; va += PAGE_SIZE, pte++) {
		entry = pte->pt_entry;
		if (!mips_pg_v(entry)) {
			continue;
		}
		if (MIPS_HAS_R4K_MMU) {
#ifndef sbmips	/* XXX XXX if (dcache_is_virtual) - should also check icache virtual && EXEC mapping */
			mips_dcache_wbinv_range(va, PAGE_SIZE);
#endif
			pte->pt_entry = MIPS3_PG_NV | MIPS3_PG_G;
		} else {
			pte->pt_entry = MIPS1_PG_NV;
		}
		MIPS_TBIS(va);
	}
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

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_WIRING))
		printf("pmap_unwire(%p, %#"PRIxVADDR")\n", pmap, va);
#endif
	/*
	 * Don't need to flush the TLB since PG_WIRED is only in software.
	 */
	if (pmap == pmap_kernel()) {
		/* change entries in kernel pmap */
#ifdef PARANOIADIAG
		if (va < VM_MIN_KERNEL_ADDRESS || va >= virtual_end)
			panic("pmap_unwire");
#endif
		pte = kvtopte(va);
	} else {
		pte = pmap_segmap(pmap, va);
#ifdef DIAGNOSTIC
		if (pte == NULL)
			panic("pmap_unwire: pmap %p va %#"PRIxVADDR" invalid STE",
			    pmap, va);
#endif
		pte += (va >> PGSHIFT) & (NPTEPG - 1);
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
		else
			pte = kvtopte(va);
	} else {
		if (!(pte = pmap_segmap(pmap, va))) {
#ifdef DEBUG
			if (pmapdebug & PDB_FOLLOW)
				printf("not in segmap\n");
#endif
			return false;
		}
		pte += (va >> PGSHIFT) & (NPTEPG - 1);
	}
	if (!mips_pg_v(pte->pt_entry)) {
#ifdef DEBUG
		if (pmapdebug & PDB_FOLLOW)
			printf("PTE not valid\n");
#endif
		return false;
	}
	pa = mips_tlbpfn_to_paddr(pte->pt_entry) | (va & PGOFSET);
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

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy(%p, %p, %#"PRIxVADDR", %#"PRIxVSIZE", %#"PRIxVADDR")\n",
		    dst_pmap, src_pmap, dst_addr, len, src_addr);
#endif
}

/*
 *	Routine:	pmap_collect
 *	Function:
 *		Garbage collects the physical map system for
 *		pages which are no longer used.
 *		Success need not be guaranteed -- that is, there
 *		may well be pages which are not referenced, but
 *		others may be collected.
 *	Usage:
 *		Called by the pageout daemon when pages are scarce.
 */
void
pmap_collect(pmap_t pmap)
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_collect(%p)\n", pmap);
#endif
}

/*
 *	pmap_zero_page zeros the specified page.
 */
void
pmap_zero_page(paddr_t phys)
{
	vaddr_t va;
#if defined(MIPS3_PLUS)
	struct vm_page *pg;
	pv_entry_t pv;
#endif

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_zero_page(%#"PRIxPADDR")\n", phys);
#endif
#ifdef PARANOIADIAG
	if (!(phys < MIPS_MAX_MEM_ADDR))
		printf("pmap_zero_page(%#"PRIxPADDR") nonphys\n", phys);
#endif
#ifdef _LP64
	if (phys > MIPS_PHYS_MASK)
		va = MIPS_PHYS_TO_XKPHYS(phys, MIPS3_PG_TO_CCA(mips3_pg_cached));
	else
#endif
		va = MIPS_PHYS_TO_KSEG0(phys);

#if defined(MIPS3_PLUS)	/* XXX mmu XXX */
	pg = PHYS_TO_VM_PAGE(phys);
	if (mips_cache_virtual_alias) {
		pv = pg->mdpage.pvh_list;
		if ((pv->pv_flags & PV_UNCACHED) == 0 &&
		    mips_cache_indexof(pv->pv_va) != mips_cache_indexof(va))
			mips_dcache_wbinv_range_index(pv->pv_va, PAGE_SIZE);
	}
#endif

	mips_pagezero((void *)va);

#if defined(MIPS3_PLUS)	/* XXX mmu XXX */
	/*
	 * If we have a virtually-indexed, physically-tagged WB cache,
	 * and no L2 cache to warn of aliased mappings,	we must force a
	 * writeback of the destination out of the L1 cache.  If we don't,
	 * later reads (from virtual addresses mapped to the destination PA)
	 * might read old stale DRAM footprint, not the just-written data.
	 *
	 * XXXJRT This is totally disgusting.
	 */
	if (MIPS_HAS_R4K_MMU)	/* XXX VCED on kernel stack is not allowed */
		mips_dcache_wbinv_range(va, PAGE_SIZE);
#endif	/* MIPS3_PLUS */
}

/*
 *	pmap_copy_page copies the specified page.
 */
void
pmap_copy_page(paddr_t src, paddr_t dst)
{
	vaddr_t src_va, dst_va;
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy_page(%#"PRIxPADDR", %#"PRIxPADDR")\n", src, dst);
#endif
#ifdef _LP64
	if (src > MIPS_PHYS_MASK)
		src_va = MIPS_PHYS_TO_XKPHYS(src, MIPS3_PG_TO_CCA(mips3_pg_cached));
	else
#endif
		src_va = MIPS_PHYS_TO_KSEG0(src);
#ifdef _LP64
	if (dst > MIPS_PHYS_MASK)
		dst_va = MIPS_PHYS_TO_XKPHYS(dst, MIPS3_PG_TO_CCA(mips3_pg_cached));
	else
#endif
		dst_va = MIPS_PHYS_TO_KSEG0(dst);
#if !defined(_LP64) && defined(PARANOIADIAG)
	if (!(src < MIPS_MAX_MEM_ADDR))
		printf("pmap_copy_page(%#"PRIxPADDR") src nonphys\n", src);
	if (!(dst < MIPS_MAX_MEM_ADDR))
		printf("pmap_copy_page(%#"PRIxPADDR") dst nonphys\n", dst);
#endif

#if defined(MIPS3_PLUS) /* XXX mmu XXX */
	/*
	 * If we have a virtually-indexed, physically-tagged cache,
	 * and no L2 cache to warn of aliased mappings, we must force an
	 * write-back of all L1 cache lines of the source physical address,
	 * irrespective of their virtual address (cache indexes).
	 * If we don't, our copy loop might read and copy stale DRAM
	 * footprint instead of the fresh (but dirty) data in a WB cache.
	 * XXX invalidate any cached lines of the destination PA
	 *     here also?
	 *
	 * It would probably be better to map the destination as a
	 * write-through no allocate to reduce cache thrash.
	 */
	if (mips_cache_virtual_alias) {
		/*XXX FIXME Not very sophisticated */
		mips_flushcache_allpvh(src);
#if 0
		mips_flushcache_allpvh(dst);
#endif
	}
#endif	/* MIPS3_PLUS */

	mips_pagecopy((void *)dst_va, (void *)src_va);

#if defined(MIPS3_PLUS) /* XXX mmu XXX */
	/*
	 * If we have a virtually-indexed, physically-tagged WB cache,
	 * and no L2 cache to warn of aliased mappings,	we must force a
	 * writeback of the destination out of the L1 cache.  If we don't,
	 * later reads (from virtual addresses mapped to the destination PA)
	 * might read old stale DRAM footprint, not the just-written data.
	 * XXX  Do we need to also invalidate any cache lines matching
	 *      the destination as well?
	 *
	 * XXXJRT -- This is totally disgusting.
	 */
	if (mips_cache_virtual_alias) {
		mips_dcache_wbinv_range(src_va, PAGE_SIZE);
		mips_dcache_wbinv_range(dst_va, PAGE_SIZE);
	}
#endif	/* MIPS3_PLUS */
}

/*
 *	pmap_clear_reference:
 *
 *	Clear the reference bit on the specified physical page.
 */
bool
pmap_clear_reference(struct vm_page *pg)
{
	int *attrp;
	bool rv;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_reference(%#"PRIxPADDR")\n",
		    VM_PAGE_TO_PHYS(pg));
#endif
	attrp = &pg->mdpage.pvh_attrs;
	rv = *attrp & PV_REFERENCED;
	*attrp &= ~PV_REFERENCED;
	return rv;
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

	return pg->mdpage.pvh_attrs & PV_REFERENCED;
}

/*
 *	Clear the modify bits on the specified physical page.
 */
bool
pmap_clear_modify(struct vm_page *pg)
{
	struct pmap *pmap;
	struct pv_entry *pv;
	pt_entry_t *pte;
	int *attrp;
	vaddr_t va;
	unsigned asid;
	bool rv;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_modify(%#"PRIxPADDR")\n", VM_PAGE_TO_PHYS(pg));
#endif
	attrp = &pg->mdpage.pvh_attrs;
	rv = *attrp & PV_MODIFIED;
	*attrp &= ~PV_MODIFIED;
	if (!rv) {
		return rv;
	}
	pv = pg->mdpage.pvh_list;
	if (pv->pv_pmap == NULL) {
		return true;
	}

	/*
	 * remove write access from any pages that are dirty
	 * so we can tell if they are written to again later.
	 * flush the VAC first if there is one.
	 */

	for (; pv; pv = pv->pv_next) {
		pmap = pv->pv_pmap;
		va = pv->pv_va;
		if (pmap == pmap_kernel()) {
			pte = kvtopte(va);
			asid = 0;
		} else {
			pte = pmap_segmap(pmap, va);
			KASSERT(pte);
			pte += ((va >> PGSHIFT) & (NPTEPG - 1));
			asid = pmap->pm_asid << MIPS_TLB_PID_SHIFT;
		}
		if ((pte->pt_entry & mips_pg_m_bit()) == 0) {
			continue;
		}
		if (MIPS_HAS_R4K_MMU) {
			if (PMAP_IS_ACTIVE(pmap)) {
				mips_dcache_wbinv_range(va, PAGE_SIZE);
			} else {
				mips_dcache_wbinv_range_index(va, PAGE_SIZE);
			}
		}
		pte->pt_entry &= ~mips_pg_m_bit();
		if (pmap->pm_asidgen == pmap_asid_generation) {
			MIPS_TBIS(va | asid);
		}
	}
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

	return pg->mdpage.pvh_attrs & PV_MODIFIED;
}

/*
 *	pmap_set_modified:
 *
 *	Sets the page modified reference bit for the specified page.
 */
void
pmap_set_modified(paddr_t pa)
{
	struct vm_page *pg;

	pg = PHYS_TO_VM_PAGE(pa);
	pg->mdpage.pvh_attrs |= PV_MODIFIED | PV_REFERENCED;
}

/******************** misc. functions ********************/

/*
 * Allocate TLB address space tag (called ASID or TLBPID) and return it.
 * It takes almost as much or more time to search the TLB for a
 * specific ASID and flush those entries as it does to flush the entire TLB.
 * Therefore, when we allocate a new ASID, we just take the next number. When
 * we run out of numbers, we flush the TLB, increment the generation count
 * and start over. ASID zero is reserved for kernel use.
 */
void
pmap_asid_alloc(pmap_t pmap)
{

	if (pmap->pm_asid == PMAP_ASID_RESERVED ||
	    pmap->pm_asidgen != pmap_asid_generation) {
		if (pmap_next_asid == pmap_max_asid) {
			MIPS_TBIAP();
			pmap_asid_generation++; /* ok to wrap to 0 */
			pmap_next_asid = 1;	/* 0 means invalid */
		}
		pmap->pm_asid = pmap_next_asid++;
		pmap->pm_asidgen = pmap_asid_generation;
	}

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_TLBPID)) {
		if (curlwp)
			printf("pmap_asid_alloc: curlwp %d.%d '%s' ",
			    curlwp->l_proc->p_pid, curlwp->l_lid,
			    curlwp->l_proc->p_comm);
		else
			printf("pmap_asid_alloc: curlwp <none> ");
		printf("segtab %p asid %d\n", pmap->pm_segtab, pmap->pm_asid);
	}
#endif
}

/******************** pv_entry management ********************/

/*
 * Enter the pmap and virtual address into the
 * physical to virtual map table.
 */
void
pmap_enter_pv(pmap_t pmap, vaddr_t va, struct vm_page *pg, u_int *npte)
{
	pv_entry_t pv, npv;

	pv = pg->mdpage.pvh_list;
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("pmap_enter: pv %p: was %#"PRIxVADDR"/%p/%p\n",
		    pv, pv->pv_va, pv->pv_pmap, pv->pv_next);
#endif
#if defined(MIPS3_NO_PV_UNCACHED)
again:
#endif
	if (pv->pv_pmap == NULL) {

		/*
		 * No entries yet, use header as the first entry
		 */

#ifdef DEBUG
		if (pmapdebug & PDB_PVENTRY)
			printf("pmap_enter: first pv: pmap %p va %#"PRIxVADDR"\n",
			    pmap, va);
		enter_stats.firstpv++;
#endif
		pv->pv_va = va;
		pv->pv_flags &= ~PV_UNCACHED;
		pv->pv_pmap = pmap;
		pv->pv_next = NULL;
	} else {
#if defined(MIPS3_PLUS) /* XXX mmu XXX */
		if (mips_cache_virtual_alias) {
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
				if (mips_cache_indexof(npv->pv_va) !=
				    mips_cache_indexof(va)) {
					pmap_remove(npv->pv_pmap, npv->pv_va,
					    npv->pv_va + PAGE_SIZE);
					pmap_update(npv->pv_pmap);
					goto again;
				}
			}
#else	/* !MIPS3_NO_PV_UNCACHED */
			if (!(pv->pv_flags & PV_UNCACHED)) {
				for (npv = pv; npv; npv = npv->pv_next) {

					/*
					 * Check cache aliasing incompatibility.
					 * If one exists, re-map this page
					 * uncached until all mappings have
					 * the same index again.
					 */
					if (mips_cache_indexof(npv->pv_va) !=
					    mips_cache_indexof(va)) {
						pmap_page_cache(pg,PV_UNCACHED);
						mips_dcache_wbinv_range_index(
						    pv->pv_va, PAGE_SIZE);
						*npte = (*npte &
						    ~MIPS3_PG_CACHEMODE) |
						    MIPS3_PG_UNCACHED;
#ifdef DEBUG
						enter_stats.ci++;
#endif
						break;
					}
				}
			} else {
				*npte = (*npte & ~MIPS3_PG_CACHEMODE) |
				    MIPS3_PG_UNCACHED;
			}
#endif	/* !MIPS3_NO_PV_UNCACHED */
		}
#endif /* MIPS3_PLUS */

		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 *
		 * Note: the entry may already be in the table if
		 * we are only changing the protection bits.
		 */

		for (npv = pv; npv; npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && va == npv->pv_va) {
#ifdef PARANOIADIAG
				pt_entry_t *pte;
				unsigned entry;

				if (pmap == pmap_kernel())
					entry = kvtopte(va)->pt_entry;
				else {
					pte = pmap_segmap(pmap, va);
					if (pte) {
						pte += (va >> PGSHIFT) &
						    (NPTEPG - 1);
						entry = pte->pt_entry;
					} else
						entry = 0;
				}
				if (!mips_pg_v(entry) ||
				    mips_tlbpfn_to_paddr(entry) !=
				    VM_PAGE_TO_PHYS(pg))
					printf(
		"pmap_enter: found va %#"PRIxVADDR" pa %#"PRIxPADDR" in pv_table but != %x\n",
					    va, VM_PAGE_TO_PHYS(pg),
					    entry);
#endif
				return;
			}
		}
#ifdef DEBUG
		if (pmapdebug & PDB_PVENTRY)
			printf("pmap_enter: new pv: pmap %p va %#"PRIxVADDR"\n",
			    pmap, va);
#endif
		npv = (pv_entry_t)pmap_pv_alloc();
		if (npv == NULL)
			panic("pmap_enter_pv: pmap_pv_alloc() failed");
		npv->pv_va = va;
		npv->pv_pmap = pmap;
		npv->pv_flags = pv->pv_flags;
		npv->pv_next = pv->pv_next;
		pv->pv_next = npv;
#ifdef DEBUG
		if (!npv->pv_next)
			enter_stats.secondpv++;
#endif
	}
}

/*
 * Remove a physical to virtual address translation.
 * If cache was inhibited on this page, and there are no more cache
 * conflicts, restore caching.
 * Flush the cache if the last page is removed (should always be cached
 * at this point).
 */
void
pmap_remove_pv(pmap_t pmap, vaddr_t va, struct vm_page *pg)
{
	pv_entry_t pv, npv;
	int last;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PVENTRY))
		printf("pmap_remove_pv(%p, %#"PRIxVADDR", %#"PRIxPADDR")\n", pmap, va,
		    VM_PAGE_TO_PHYS(pg));
#endif

	pv = pg->mdpage.pvh_list;

	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */

	last = 0;
	if (pmap == pv->pv_pmap && va == pv->pv_va) {
		npv = pv->pv_next;
		if (npv) {

			/*
			 * Copy current modified and referenced status to
			 * the following entry before copying.
			 */

			npv->pv_flags |=
			    pv->pv_flags & (PV_MODIFIED | PV_REFERENCED);
			*pv = *npv;
			pmap_pv_free(npv);
		} else {
			pv->pv_pmap = NULL;
			last = 1;	/* Last mapping removed */
		}
#ifdef DEBUG
		remove_stats.pvfirst++;
#endif
	} else {
		for (npv = pv->pv_next; npv; pv = npv, npv = npv->pv_next) {
#ifdef DEBUG
			remove_stats.pvsearch++;
#endif
			if (pmap == npv->pv_pmap && va == npv->pv_va)
				break;
		}
		if (npv) {
			pv->pv_next = npv->pv_next;
			pmap_pv_free(npv);
		}
	}
#ifdef MIPS3_PLUS	/* XXX mmu XXX */
#if !defined(MIPS3_NO_PV_UNCACHED)
	if (MIPS_HAS_R4K_MMU && pv->pv_flags & PV_UNCACHED) {

		/*
		 * Page is currently uncached, check if alias mapping has been
		 * removed.  If it was, then reenable caching.
		 */

		pv = pg->mdpage.pvh_list;
		for (npv = pv->pv_next; npv; npv = npv->pv_next) {
			if (mips_cache_indexof(pv->pv_va ^ npv->pv_va))
				break;
		}
		if (npv == NULL)
			pmap_page_cache(pg, 0);
	}
#endif
	if (MIPS_HAS_R4K_MMU && last != 0)
		mips_dcache_wbinv_range_index(va, PAGE_SIZE);
#endif	/* MIPS3_PLUS */
}

/*
 * pmap_pv_page_alloc:
 *
 *	Allocate a page for the pv_entry pool.
 */
void *
pmap_pv_page_alloc(struct pool *pp, int flags)
{
	struct vm_page *pg;
	paddr_t phys;
#if defined(MIPS3_PLUS)
	pv_entry_t pv;
#endif
	vaddr_t va;

	pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE);
	if (pg == NULL) {
		return NULL;
	}
	phys = VM_PAGE_TO_PHYS(pg);
#ifdef _LP64
	if (phys > MIPS_PHYS_MASK)
		va = MIPS_PHYS_TO_XKPHYS(phys, MIPS3_PG_TO_CCA(mips3_pg_cached));
	else
#endif
		va = MIPS_PHYS_TO_KSEG0(phys);
#if defined(MIPS3_PLUS)
	if (mips_cache_virtual_alias) {
		pg = PHYS_TO_VM_PAGE(phys);
		pv = pg->mdpage.pvh_list;
		if ((pv->pv_flags & PV_UNCACHED) == 0 &&
		    mips_cache_indexof(pv->pv_va) != mips_cache_indexof(va))
			mips_dcache_wbinv_range_index(pv->pv_va, PAGE_SIZE);
	}
#endif
	return (void *)va;
}

/*
 * pmap_pv_page_free:
 *
 *	Free a pv_entry pool page.
 */
void
pmap_pv_page_free(struct pool *pp, void *v)
{
	paddr_t phys;

#ifdef MIPS3_PLUS
	if (mips_cache_virtual_alias)
		mips_dcache_inv_range((vaddr_t)v, PAGE_SIZE);
#endif
#ifdef _LP64
	if (MIPS_XKPHYS_P(v))
		phys = MIPS_XKPHYS_TO_PHYS((vaddr_t)v);
	else
#endif
		phys = MIPS_KSEG0_TO_PHYS((vaddr_t)v);
	uvm_pagefree(PHYS_TO_VM_PAGE(phys));
}

pt_entry_t *
pmap_pte(pmap_t pmap, vaddr_t va)
{
	pt_entry_t *pte = NULL;

	if (pmap == pmap_kernel())
		pte = kvtopte(va);
	else if ((pte = pmap_segmap(pmap, va)) != NULL)
		pte += (va >> PGSHIFT) & (NPTEPG - 1);
	return pte;
}

#ifdef MIPS3_PLUS	/* XXX mmu XXX */
/*
 * Find first virtual address >= *vap that doesn't cause
 * a cache alias conflict.
 */
void
pmap_prefer(vaddr_t foff, vaddr_t *vap, int td)
{
	vaddr_t	va;
	vsize_t d;

	if (MIPS_HAS_R4K_MMU) {
		va = *vap;

		d = foff - va;
		d &= mips_cache_prefer_mask;
		if (td && d)
			d = -((-d) & mips_cache_prefer_mask);
		*vap = va + d;
	}
}
#endif	/* MIPS3_PLUS */

vaddr_t
mips_pmap_map_poolpage(paddr_t pa)
{
	vaddr_t va;
#if defined(MIPS3_PLUS)
	struct vm_page *pg;
	pv_entry_t pv;
#endif

	if (pa <= MIPS_PHYS_MASK)
		va = MIPS_PHYS_TO_KSEG0(pa);
	else
#ifdef _LP64
		va = MIPS_PHYS_TO_XKPHYS(MIPS3_PG_TO_CCA(mips3_pg_cached), pa);
#else
		panic("mips_pmap_map_poolpage: "
		    "pa #%"PRIxPADDR" can not be mapped into KSEG0", pa);
#endif
#if defined(MIPS3_PLUS)
	if (mips_cache_virtual_alias) {
		pg = PHYS_TO_VM_PAGE(pa);
		pv = pg->mdpage.pvh_list;
		if ((pv->pv_flags & PV_UNCACHED) == 0 &&
		    mips_cache_indexof(pv->pv_va) != mips_cache_indexof(va))
			mips_dcache_wbinv_range_index(pv->pv_va, PAGE_SIZE);
	}
#endif
	return va;
}

paddr_t
mips_pmap_unmap_poolpage(vaddr_t va)
{
	paddr_t pa;

#ifdef _LP64
	if (MIPS_XKPHYS_P(va))
		pa = MIPS_XKPHYS_TO_PHYS(va);
	else
#endif
		pa = MIPS_KSEG0_TO_PHYS(va);
#if defined(MIPS3_PLUS)
	if (mips_cache_virtual_alias) {
		mips_dcache_inv_range(va, PAGE_SIZE);
	}
#endif
	return pa;
}

/******************** page table page management ********************/

/* TO BE DONE */
