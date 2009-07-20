/* $NetBSD: pmap.c,v 1.21 2009/07/20 04:41:37 kiyohara Exp $ */


/*-
 * Copyright (c) 1998, 1999, 2000, 2001 The NetBSD Foundation, Inc.
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

/*-
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 * Copyright (c) 1994 John S. Dyson
 * All rights reserved.
 * Copyright (c) 1994 David Greenman
 * All rights reserved.
 * Copyright (c) 1998,2000 Doug Rabson
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and William Jolitz of UUNET Technologies Inc.
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
 *	from:	@(#)pmap.c	7.7 (Berkeley)	5/12/91
 *	from:	i386 Id: pmap.c,v 1.193 1998/04/19 15:22:48 bde Exp
 *		with some ideas from NetBSD's alpha pmap
 */

/* __FBSDID("$FreeBSD: src/sys/ia64/ia64/pmap.c,v 1.172 2005/11/20 06:09:48 alc Exp $"); */


/* XXX: This module is a mess. Need to clean up Locking, list traversal. etc....... */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.21 2009/07/20 04:41:37 kiyohara Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/lock.h>

#include <uvm/uvm.h>

#include <machine/pal.h>
#include <machine/atomic.h>
#include <machine/pte.h>
#include <sys/sched.h>
#include <machine/cpufunc.h>
#include <machine/md_var.h>


/*
 * Kernel virtual memory management.
 */
static int nkpt;
struct ia64_lpte **ia64_kptdir;
#define KPTE_DIR_INDEX(va) \
	((va >> (2*PAGE_SHIFT-5)) & ((1<<(PAGE_SHIFT-3))-1))
#define KPTE_PTE_INDEX(va) \
	((va >> PAGE_SHIFT) & ((1<<(PAGE_SHIFT-5))-1))
#define NKPTEPG		(PAGE_SIZE / sizeof(struct ia64_lpte))


/* Values for ptc.e. XXX values for SKI. */
static uint64_t pmap_ptc_e_base = 0x100000000;
static uint64_t pmap_ptc_e_count1 = 3;
static uint64_t pmap_ptc_e_count2 = 2;
static uint64_t pmap_ptc_e_stride1 = 0x2000;
static uint64_t pmap_ptc_e_stride2 = 0x100000000;
kmutex_t pmap_ptc_lock;			/* Global PTC lock */

/* VHPT Base */

vaddr_t vhpt_base;
vaddr_t pmap_vhpt_log2size;

struct ia64_bucket *pmap_vhpt_bucket;
int pmap_vhpt_nbuckets;
kmutex_t pmap_vhptlock;		       /* VHPT collision chain lock */

int pmap_vhpt_inserts;
int pmap_vhpt_resident;
int pmap_vhpt_collisions;

#ifdef DEBUG
static void dump_vhpt(void);
#endif

/*
 * Data for the RID allocator
 */
static int pmap_ridcount;
static int pmap_rididx;
static int pmap_ridmapsz;
static int pmap_ridmax;
static uint64_t *pmap_ridmap;
kmutex_t pmap_rid_lock;			/* RID allocator lock */


bool		pmap_initialized;	/* Has pmap_init completed? */
u_long		pmap_pages_stolen;	/* instrumentation */

static struct pmap kernel_pmap_store;	/* the kernel's pmap (proc0) */
struct pmap *const kernel_pmap_ptr = &kernel_pmap_store;

static vaddr_t	kernel_vm_end;	/* VA of last avail page ( end of kernel Address Space ) */

/*
 * This variable contains the number of CPU IDs we need to allocate
 * space for when allocating the pmap structure.  It is used to
 * size a per-CPU array of ASN and ASN Generation number.
 */
u_long		pmap_ncpuids;

#ifndef PMAP_PV_LOWAT
#define	PMAP_PV_LOWAT	16
#endif
int		pmap_pv_lowat = PMAP_PV_LOWAT;

/*
 * PV table management functions.
 */
void	*pmap_pv_page_alloc(struct pool *, int);
void	pmap_pv_page_free(struct pool *, void *);

struct pool_allocator pmap_pv_page_allocator = {
	pmap_pv_page_alloc, pmap_pv_page_free, 0,
};

bool pmap_poolpage_alloc(paddr_t *);
void pmap_poolpage_free(paddr_t);

/*
 * List of all pmaps, used to update them when e.g. additional kernel
 * page tables are allocated.  This list is kept LRU-ordered by
 * pmap_activate(). XXX: Check on this.....
 */
TAILQ_HEAD(, pmap) pmap_all_pmaps;

/*
 * The pools from which pmap structures and sub-structures are allocated.
 */
struct pool pmap_pmap_pool;
struct pool pmap_ia64_lpte_pool;
struct pool pmap_pv_pool;

kmutex_t pmap_main_lock;
kmutex_t pmap_all_pmaps_slock;

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
/* XXX(kochi) need to use only spin lock? */
#define	PMAP_MAP_TO_HEAD_LOCK() \
	spinlockmgr(&pmap_main_lock, LK_SHARED, NULL)
#define	PMAP_MAP_TO_HEAD_UNLOCK() \
	spinlockmgr(&pmap_main_lock, LK_RELEASE, NULL)
#define	PMAP_HEAD_TO_MAP_LOCK() \
	spinlockmgr(&pmap_main_lock, LK_EXCLUSIVE, NULL)
#define	PMAP_HEAD_TO_MAP_UNLOCK() \
	spinlockmgr(&pmap_main_lock, LK_RELEASE, NULL)
#else
#define	PMAP_MAP_TO_HEAD_LOCK()		/* nothing */
#define	PMAP_MAP_TO_HEAD_UNLOCK()	/* nothing */
#define	PMAP_HEAD_TO_MAP_LOCK()		/* nothing */
#define	PMAP_HEAD_TO_MAP_UNLOCK()	/* nothing */
#endif /* MULTIPROCESSOR || LOCKDEBUG */


#define pmap_accessed(lpte)             ((lpte)->pte & PTE_ACCESSED)
#define pmap_dirty(lpte)                ((lpte)->pte & PTE_DIRTY)
#define pmap_managed(lpte)              ((lpte)->pte & PTE_MANAGED)
#define pmap_ppn(lpte)                  ((lpte)->pte & PTE_PPN_MASK)
#define pmap_present(lpte)              ((lpte)->pte & PTE_PRESENT)
#define pmap_prot(lpte)                 (((lpte)->pte & PTE_PROT_MASK) >> 56)
#define pmap_wired(lpte)                ((lpte)->pte & PTE_WIRED)

#define pmap_clear_accessed(lpte)       (lpte)->pte &= ~PTE_ACCESSED
#define pmap_clear_dirty(lpte)          (lpte)->pte &= ~PTE_DIRTY
#define pmap_clear_present(lpte)        (lpte)->pte &= ~PTE_PRESENT
#define pmap_clear_wired(lpte)          (lpte)->pte &= ~PTE_WIRED

#define pmap_set_wired(lpte)            (lpte)->pte |= PTE_WIRED


/*
 * The VHPT bucket head structure.
 */
struct ia64_bucket {
	uint64_t	chain;
	kmutex_t	lock;
	u_int		length;
};


/* Local Helper functions */

static void	pmap_invalidate_all(pmap_t);
static void	pmap_invalidate_page(pmap_t, vaddr_t);

static pmap_t pmap_switch(pmap_t pm);
static pmap_t	pmap_install(pmap_t);

static struct ia64_lpte *pmap_find_kpte(vaddr_t);

static void
pmap_set_pte(struct ia64_lpte *, vaddr_t, vaddr_t, bool, bool);
static void
pmap_free_pte(struct ia64_lpte *pte, vaddr_t va);

static __inline void
pmap_pte_prot(pmap_t pm, struct ia64_lpte *pte, vm_prot_t prot);
static int
pmap_remove_pte(pmap_t pmap, struct ia64_lpte *pte, vaddr_t va,
		pv_entry_t pv, int freepte);

static struct ia64_lpte *
pmap_find_pte(vaddr_t va);
static int
pmap_remove_entry(pmap_t pmap, struct vm_page * pg, vaddr_t va, pv_entry_t pv);
static void
pmap_insert_entry(pmap_t pmap, vaddr_t va, struct vm_page *pg);

static __inline int
pmap_track_modified(vaddr_t va);

static void
pmap_enter_vhpt(struct ia64_lpte *, vaddr_t);
static int pmap_remove_vhpt(vaddr_t);
static struct ia64_lpte *
pmap_find_vhpt(vaddr_t);
void
pmap_page_purge(struct vm_page * pg);
static void
pmap_remove_page(pmap_t pmap, vaddr_t va);


static uint32_t pmap_allocate_rid(void);
static void pmap_free_rid(uint32_t rid);

static vaddr_t
pmap_steal_vhpt_memory(vsize_t);

/*
 * pmap_steal_memory:		[ INTERFACE ]
 *
 *	Bootstrap memory allocator (alternative to uvm_pageboot_alloc()).
 *	This function allows for early dynamic memory allocation until the
 *	virtual memory system has been bootstrapped.  After that point, either
 *	kmem_alloc or malloc should be used.  This function works by stealing
 *	pages from the (to be) managed page pool, then implicitly mapping the
 *	pages (by using their RR7 addresses) and zeroing them.
 *
 *	It may be used once the physical memory segments have been pre-loaded
 *	into the vm_physmem[] array.  Early memory allocation MUST use this
 *	interface!  This cannot be used after uvm_page_init(), and will
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
	int lcv, npgs, x;
	vaddr_t va;
	paddr_t pa;

	size = round_page(size);
	npgs = atop(size);

#if 0
	printf("PSM: size 0x%lx (npgs 0x%x)\n", size, npgs);
#endif

	for (lcv = 0; lcv < vm_nphysseg; lcv++) {
		if (uvm.page_init_done == true)
			panic("pmap_steal_memory: called _after_ bootstrap");

#if 0
		printf("     bank %d: avail_start 0x%lx, start 0x%lx, "
		    "avail_end 0x%lx\n", lcv, vm_physmem[lcv].avail_start,
		    vm_physmem[lcv].start, vm_physmem[lcv].avail_end);
#endif

		if (vm_physmem[lcv].avail_start != vm_physmem[lcv].start ||
		    vm_physmem[lcv].avail_start >= vm_physmem[lcv].avail_end)
			continue;

#if 0
		printf("             avail_end - avail_start = 0x%lx\n",
		    vm_physmem[lcv].avail_end - vm_physmem[lcv].avail_start);
#endif

		if ((vm_physmem[lcv].avail_end - vm_physmem[lcv].avail_start)
		    < npgs)
			continue;

		/*
		 * There are enough pages here; steal them!
		 */
		pa = ptoa(vm_physmem[lcv].avail_start);
		vm_physmem[lcv].avail_start += npgs;
		vm_physmem[lcv].start += npgs;


		/*
		 * Have we used up this segment?
		 */
		if (vm_physmem[lcv].avail_start == vm_physmem[lcv].end) {
			if (vm_nphysseg == 1)
				panic("pmap_steal_memory: out of memory!");

			/* Remove this segment from the list. */
			vm_nphysseg--;
			for (x = lcv; x < vm_nphysseg; x++) {
				/* structure copy */
				vm_physmem[x] = vm_physmem[x + 1];
			}
		}

		va = IA64_PHYS_TO_RR7(pa);
		memset((void *)va, 0, size);
		pmap_pages_stolen += npgs;
		return va;
	}

	/*
	 * If we got here, this was no memory left.
	 */
	panic("pmap_steal_memory: no memory to steal");
}


/*
 * pmap_steal_vhpt_memory:	Derived from alpha/pmap.c:pmap_steal_memory()
 * Note: This function is not visible outside the pmap module.
 * Based on pmap_steal_memory();
 * Assumptions: size is always a power of 2.
 * Returns: Allocated memory at a naturally aligned address
 */

static vaddr_t
pmap_steal_vhpt_memory(vsize_t size)
{
	int lcv, npgs, x;
	vaddr_t va;
	paddr_t pa;

	paddr_t vhpt_start = 0, start1, start2, end1, end2;

	size = round_page(size);
	npgs = atop(size);

#if 1
	printf("VHPTPSM: size 0x%lx (npgs 0x%x)\n", size, npgs);
#endif

	for (lcv = 0; lcv < vm_nphysseg; lcv++) {
		if (uvm.page_init_done == true)
			panic("pmap_vhpt_steal_memory: called _after_ bootstrap");

#if 1
		printf("     lcv %d: avail_start 0x%lx, start 0x%lx, "
		    "avail_end 0x%lx\n", lcv, vm_physmem[lcv].avail_start,
		    vm_physmem[lcv].start, vm_physmem[lcv].avail_end);
		printf("             avail_end - avail_start = 0x%lx\n",
		    vm_physmem[lcv].avail_end - vm_physmem[lcv].avail_start);
#endif

		if (vm_physmem[lcv].avail_start != vm_physmem[lcv].start || /* XXX: ??? */
		    vm_physmem[lcv].avail_start >= vm_physmem[lcv].avail_end)
			continue;

		/* Break off a VHPT sized, aligned chunk off this segment. */

		start1 = vm_physmem[lcv].avail_start;

		/* Align requested start address on requested size boundary */
		end1 = vhpt_start = roundup(start1, npgs);

		start2 = vhpt_start + npgs;
		end2 = vm_physmem[lcv].avail_end;


		/* Case 1: Doesn't fit. skip this segment */

		if (start2 > end2) {
			vhpt_start = 0;
			continue;
		}

		/* For all cases of fit:
		 *	- Remove segment.
		 *	- Re-insert fragments via uvm_page_physload();
		 */

		/* 
		 * We _fail_ on a vhpt request which exhausts memory.
		 */
		if (start1 == end1 &&
		    start2 == end2 &&
		    vm_nphysseg == 1) {
#ifdef DEBUG				
				printf("pmap_vhpt_steal_memory: out of memory!");
#endif
				return -1;
			}

		/* Remove this segment from the list. */
		vm_nphysseg--;
		//		physmem -= end2 - start1;
		for (x = lcv; x < vm_nphysseg; x++) {
			/* structure copy */
			vm_physmem[x] = vm_physmem[x + 1];
		}

		/* Case 2: Perfect fit - skip segment reload. */

		if (start1 == end1 && start2 == end2) break;

		/* Case 3: Left unfit - reload it. 
		 */	

		if (start1 != end1) {
			uvm_page_physload(start1, end1, start1, end1,
					  VM_FREELIST_DEFAULT);
		}					
	
		/* Case 4: Right unfit - reload it. */

		if (start2 != end2) {
			uvm_page_physload(start2, end2, start2, end2,
					  VM_FREELIST_DEFAULT);
		}

		/* Case 5: Both unfit - Redundant, isn't it ?  */
		break;
	}

	/*
	 * If we got here, we couldn't find a fit.
	 */
	if (vhpt_start == 0) {
#ifdef DEBUG
		printf("pmap_steal_vhpt_memory: no VHPT aligned fit found.");
#endif
		return -1;
	}

	/*
	 * There are enough pages here; steal them!
	 */
	pa = ptoa(vhpt_start);
	va = IA64_PHYS_TO_RR7(pa);
	memset((void *)va, 0, size);
	pmap_pages_stolen += npgs;
	return va;
}





/*
 * pmap_bootstrap:
 *
 *	Bootstrap the system to run with virtual memory.
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_bootstrap(void)
{
	struct ia64_pal_result res;
	vaddr_t base, limit;
	size_t size;
	vsize_t bufsz;

	int i, ridbits;

	/*
	 * Query the PAL Code to find the loop parameters for the
	 * ptc.e instruction.
	 */
	res = ia64_call_pal_static(PAL_PTCE_INFO, 0, 0, 0);
	if (res.pal_status != 0)
		panic("Can't configure ptc.e parameters");
	pmap_ptc_e_base = res.pal_result[0];
	pmap_ptc_e_count1 = res.pal_result[1] >> 32;
	pmap_ptc_e_count2 = res.pal_result[1] & ((1L<<32) - 1);
	pmap_ptc_e_stride1 = res.pal_result[2] >> 32;
	pmap_ptc_e_stride2 = res.pal_result[2] & ((1L<<32) - 1);
	if (bootverbose)
		printf("ptc.e base=0x%lx, count1=%ld, count2=%ld, "
		       "stride1=0x%lx, stride2=0x%lx\n",
		       pmap_ptc_e_base,
		       pmap_ptc_e_count1,
		       pmap_ptc_e_count2,
		       pmap_ptc_e_stride1,
		       pmap_ptc_e_stride2);
	mutex_init(&pmap_ptc_lock, MUTEX_DEFAULT, IPL_VM);

	/*
	 * Setup RIDs. RIDs 0..7 are reserved for the kernel.
	 *
	 * We currently need at least 19 bits in the RID because PID_MAX
	 * can only be encoded in 17 bits and we need RIDs for 5 regions
	 * per process. With PID_MAX equalling 99999 this means that we
	 * need to be able to encode 499995 (=5*PID_MAX).
	 * The Itanium processor only has 18 bits and the architected
	 * minimum is exactly that. So, we cannot use a PID based scheme
	 * in those cases. Enter pmap_ridmap...
	 * We should avoid the map when running on a processor that has
	 * implemented enough bits. This means that we should pass the
	 * process/thread ID to pmap. This we currently don't do, so we
	 * use the map anyway. However, we don't want to allocate a map
	 * that is large enough to cover the range dictated by the number
	 * of bits in the RID, because that may result in a RID map of
	 * 2MB in size for a 24-bit RID. A 64KB map is enough.
	 * The bottomline: we create a 32KB map when the processor only
	 * implements 18 bits (or when we can't figure it out). Otherwise
	 * we create a 64KB map.
	 */
	res = ia64_call_pal_static(PAL_VM_SUMMARY, 0, 0, 0);
	if (res.pal_status != 0) {
		if (bootverbose)
			printf("Can't read VM Summary - assuming 18 Region ID bits\n");
		ridbits = 18; /* guaranteed minimum */
	} else {
		ridbits = (res.pal_result[1] >> 8) & 0xff;
		if (bootverbose)
			printf("Processor supports %d Region ID bits\n",
			    ridbits);
	}
	if (ridbits > 19)
		ridbits = 19;

	pmap_ridmax = (1 << ridbits);
	pmap_ridmapsz = pmap_ridmax / 64;
	pmap_ridmap = (uint64_t *)uvm_pageboot_alloc(pmap_ridmax / 8);
	pmap_ridmap[0] |= 0xff;
	pmap_rididx = 0;
	pmap_ridcount = 8;

	/* XXX: The FreeBSD pmap.c defines initialises this like this:
	 *      mtx_init(&pmap_ridmutex, "RID allocator lock", NULL, MTX_DEF);
	 *	MTX_DEF can *sleep*.
	 */
	mutex_init(&pmap_rid_lock, MUTEX_DEFAULT, IPL_VM);


	/*
	 * Compute the number of pages kmem_map will have.
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

	nkpt = (((ubc_nwins << ubc_winshift) + uvm_emap_size +
		bufsz + 16 * NCARGS + pager_map_size) / PAGE_SIZE +
		USRIOSIZE + (maxproc * UPAGES) + nkmempages) / NKPTEPG;

	/*
	 * Allocate some memory for initial kernel 'page tables'.
	 */
	ia64_kptdir = (void *)uvm_pageboot_alloc((nkpt + 1) * PAGE_SIZE);
	for (i = 0; i < nkpt; i++) {
		ia64_kptdir[i] = (void*)( (vaddr_t)ia64_kptdir + PAGE_SIZE * (i + 1));
	}

	kernel_vm_end = nkpt * PAGE_SIZE * NKPTEPG + VM_MIN_KERNEL_ADDRESS -
		VM_GATEWAY_SIZE;
	
	/*
	 * Initialize the pmap pools and list.
	 */
	pmap_ncpuids = pmap_ridmax;
	pool_init(&pmap_pmap_pool, sizeof(struct pmap), 0, 0, 0, "pmappl",
	    &pool_allocator_nointr, IPL_NONE); /* This may block. */

	/* XXX: Need to convert ia64_kptdir[][] to a pool. ????*/

	/* The default pool allocator uses uvm_km_alloc & friends. 
	 * XXX: We should be using regular vm_alloced mem for regular, non-kernel ptesl
	 */

	pool_init(&pmap_ia64_lpte_pool, sizeof (struct ia64_lpte),
	    sizeof(void *), 0, 0, "ptpl", NULL, IPL_NONE); 

	pool_init(&pmap_pv_pool, sizeof (struct pv_entry), sizeof(void *),
	    0, 0, "pvpl", &pmap_pv_page_allocator, IPL_NONE);

	TAILQ_INIT(&pmap_all_pmaps);


	/*
	 * Figure out a useful size for the VHPT, based on the size of
	 * physical memory and try to locate a region which is large
	 * enough to contain the VHPT (which must be a power of two in
	 * size and aligned to a natural boundary).
	 * We silently bump up the VHPT size to the minimum size if the
	 * user has set the tunable too small. Likewise, the VHPT size
	 * is silently capped to the maximum allowed.
	 */

	pmap_vhpt_log2size = PMAP_VHPT_LOG2SIZE;

	if (pmap_vhpt_log2size == 0) {
		pmap_vhpt_log2size = 15;
		size = 1UL << pmap_vhpt_log2size;
		while (size < physmem * 32) {
			pmap_vhpt_log2size++;
			size <<= 1;
		} 
	}
	else 
		if (pmap_vhpt_log2size < 15) pmap_vhpt_log2size = 15;

	if (pmap_vhpt_log2size > 61) pmap_vhpt_log2size = 61;

	vhpt_base = 0;
	base = limit = 0;
	size = 1UL << pmap_vhpt_log2size;
	while (vhpt_base == 0 && size) {
		if (bootverbose)
			printf("Trying VHPT size 0x%lx\n", size);

		/* allocate size bytes aligned at size */
		/* #ifdef MULTIPROCESSOR, then (size * MAXCPU) bytes */
		base = pmap_steal_vhpt_memory(size); 

		if (!base) {
			/* Can't fit, try next smaller size. */
			pmap_vhpt_log2size--;
			size >>= 1;
		} else
			vhpt_base = IA64_PHYS_TO_RR7(base);
	}
	if (pmap_vhpt_log2size < 15)
		panic("Can't find space for VHPT");

	if (bootverbose)
		printf("Putting VHPT at 0x%lx\n", base);

	mutex_init(&pmap_vhptlock, MUTEX_DEFAULT, IPL_VM);

	__asm __volatile("mov cr.pta=%0;; srlz.i;;" ::
	    "r" (vhpt_base + (1<<8) + (pmap_vhpt_log2size<<2) + 1));

#ifdef DEBUG
	dump_vhpt();
#endif

	/*
	 * Initialise vhpt pte entries.
	 */

	pmap_vhpt_nbuckets = size / sizeof(struct ia64_lpte);

	pmap_vhpt_bucket = (void *)uvm_pageboot_alloc(pmap_vhpt_nbuckets *
	    sizeof(struct ia64_bucket));

	struct ia64_lpte *pte;

	pte = (struct ia64_lpte *)vhpt_base;
	for (i = 0; i < pmap_vhpt_nbuckets; i++) {
		pte[i].pte = 0;
		pte[i].itir = 0;
		pte[i].tag = 1UL << 63;	/* Invalid tag */
		pte[i].chain = (uintptr_t)(pmap_vhpt_bucket + i);
		/* Stolen memory is zeroed! */
		mutex_init(&pmap_vhpt_bucket[i].lock, MUTEX_DEFAULT,
		    IPL_VM);
	}

	/*
	 * Initialize the locks.
	 */
	mutex_init(&pmap_main_lock, MUTEX_DEFAULT, IPL_VM);
	mutex_init(&pmap_all_pmaps_slock, MUTEX_DEFAULT, IPL_VM);

	/*
	 * Initialize the kernel pmap (which is statically allocated).
	 */
	memset(pmap_kernel(), 0, sizeof(struct pmap));

	mutex_init(&pmap_kernel()->pm_slock, MUTEX_DEFAULT, IPL_VM);
	for (i = 0; i < 5; i++)
		pmap_kernel()->pm_rid[i] = 0;
	pmap_kernel()->pm_active = 1;
	TAILQ_INIT(&pmap_kernel()->pm_pvlist);
	
	TAILQ_INSERT_TAIL(&pmap_all_pmaps, pmap_kernel(), pm_list);

	/*
	 * Region 5 is mapped via the vhpt.
	 */
	ia64_set_rr(IA64_RR_BASE(5),
		    (5 << 8) | (PAGE_SHIFT << 2) | 1);

	/*
	 * Region 6 is direct mapped UC and region 7 is direct mapped
	 * WC. The details of this is controlled by the Alt {I,D}TLB
	 * handlers. Here we just make sure that they have the largest 
	 * possible page size to minimise TLB usage.
	 */
	ia64_set_rr(IA64_RR_BASE(6), (6 << 8) | (IA64_ID_PAGE_SHIFT << 2));
	ia64_set_rr(IA64_RR_BASE(7), (7 << 8) | (IA64_ID_PAGE_SHIFT << 2));
	ia64_srlz_d();

	/*
	 * Clear out any random TLB entries left over from booting.
	 */
	pmap_invalidate_all(pmap_kernel());

	map_gateway_page();
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

}


/*
 * vtophys: virtual address to physical address.  For use by
 * machine-dependent code only.
 */

paddr_t
vtophys(vaddr_t va)
{
	paddr_t pa;

	if (pmap_extract(pmap_kernel(), va, &pa) == true)
		return pa;
	return 0;
}

/*
 * pmap_virtual_space:		[ INTERFACE ]
 *
 *	Define the initial bounds of the kernel virtual address space.
 */
void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{
	*vstartp = VM_MIN_KERNEL_ADDRESS;	
	*vendp = VM_MAX_KERNEL_ADDRESS;		
}

/*
 * pmap_remove_all:		[ INTERFACE ]
 *
 *	This function is a hint to the pmap implementation that all
 *	entries in pmap will be removed before any more entries are
 *	entered.
 */

void
pmap_remove_all(pmap_t pmap)
{
	/* Nothing Yet */
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
        pmap_t oldpmap;
        vaddr_t va;
        pv_entry_t pv;
        struct ia64_lpte *pte;

        if (pmap->pm_stats.resident_count == 0)
                return;

        PMAP_MAP_TO_HEAD_LOCK();
        PMAP_LOCK(pmap);
        oldpmap = pmap_install(pmap);

        /*
         * special handling of removing one page.  a very
         * common operation and easy to short circuit some
         * code.
         */
        if (sva + PAGE_SIZE == eva) {
                pmap_remove_page(pmap, sva);
                goto out;
        }

        if (pmap->pm_stats.resident_count < ((eva - sva) >> PAGE_SHIFT)) {
                TAILQ_FOREACH(pv, &pmap->pm_pvlist, pv_plist) {
                        va = pv->pv_va;
                        if (va >= sva && va < eva) {
                                pte = pmap_find_vhpt(va);
                                KASSERT(pte != NULL);
                                pmap_remove_pte(pmap, pte, va, pv, 1);
				pmap_invalidate_page(pmap, va);
                        }
                }

        } else {
                for (va = sva; va < eva; va += PAGE_SIZE) {
                        pte = pmap_find_vhpt(va);
                        if (pte != NULL) {
                                pmap_remove_pte(pmap, pte, va, 0, 1);
				pmap_invalidate_page(pmap, va);
			}
		}
	}
		
out:
        pmap_install(oldpmap);
        PMAP_UNLOCK(pmap);
	PMAP_MAP_TO_HEAD_UNLOCK();

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
	vaddr_t va = IA64_PHYS_TO_RR7(phys);
	memset((void *) va, 0, PAGE_SIZE);
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
pmap_copy_page(paddr_t psrc, paddr_t pdst)
{
	vaddr_t vsrc = IA64_PHYS_TO_RR7(psrc);
	vaddr_t vdst = IA64_PHYS_TO_RR7(pdst);
	memcpy( (void *) vdst, (void *) vsrc, PAGE_SIZE);
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
pmap_collect(pmap_t pmap)
{

#ifdef DEBUG
		printf("pmap_collect(%p)\n", pmap);
#endif

	/*
	 * If called for the kernel pmap, just return.  We
	 * handle this case in the event that we ever want
	 * to have swappable kernel threads.
	 */
	if (pmap == pmap_kernel())
		return;

	/*
	 * This process is about to be swapped out; free all of
	 * the PT pages by removing the physical mappings for its
	 * entire address space.  Note: pmap_remove() performs
	 * all necessary locking.
	 *	XXX: Removes wired pages as well via pmap_remove(). Fixme.!!!!!
	 */
	pmap_remove(pmap, VM_MIN_ADDRESS, VM_MAX_ADDRESS);
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
	pmap_t oldpmap;
	struct ia64_lpte *pte;

	if (pmap == NULL)
		return;

	PMAP_LOCK(pmap);
	oldpmap = pmap_install(pmap);

	pte = pmap_find_vhpt(va);

	KASSERT(pte != NULL);

	/*
	 * If wiring actually changed (always?) clear the wire bit and
	 * update the wire count.  Note that wiring is not a hardware
	 * characteristic so there is no need to invalidate the TLB.
	 */

	if (pmap_wired(pte)) {
		pmap->pm_stats.wired_count--;
		pmap_clear_wired(pte);
	}
#ifdef DIAGNOSTIC
	else {
		printf("pmap_unwire: wiring for pmap %p va 0x%lx "
		    "didn't change!\n", pmap, va);
	}
#endif
	pmap_install(oldpmap);
	PMAP_UNLOCK(pmap);
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
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
        struct ia64_lpte *pte;

        pte = pmap_find_kpte(va);
        if (pmap_present(pte))
                pmap_invalidate_page(pmap_kernel(), va);
        else
                pmap_enter_vhpt(pte, va);
        pmap_pte_prot(pmap_kernel(), pte, prot);
        pmap_set_pte(pte, va, pa, false, false);

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
        struct ia64_lpte *pte;

        pte = pmap_find_kpte(va);
        if (pmap_present(pte)) {
                pmap_remove_vhpt(va);
                pmap_invalidate_page(pmap_kernel(), va);
                pmap_clear_present(pte);
        }
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
		printf("pmap_create()\n");
#endif

	pmap = pool_get(&pmap_pmap_pool, PR_WAITOK);
	memset(pmap, 0, sizeof(*pmap));

        for (i = 0; i < 5; i++)
                pmap->pm_rid[i] = pmap_allocate_rid();
        pmap->pm_active = 0;
        TAILQ_INIT(&pmap->pm_pvlist);
        memset(&pmap->pm_stats, 0, sizeof (pmap->pm_stats) );

	mutex_init(&pmap->pm_slock, MUTEX_DEFAULT, IPL_VM);

	mutex_enter(&pmap_all_pmaps_slock);
	TAILQ_INSERT_TAIL(&pmap_all_pmaps, pmap, pm_list);
	mutex_exit(&pmap_all_pmaps_slock);

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
	int i;

#ifdef DEBUG
		printf("pmap_destroy(%p)\n", pmap);
#endif

        for (i = 0; i < 5; i++)
                if (pmap->pm_rid[i])
                        pmap_free_rid(pmap->pm_rid[i]);
	/*
	 * Remove it from the global list of all pmaps.
	 */
	mutex_enter(&pmap_all_pmaps_slock);
	TAILQ_REMOVE(&pmap_all_pmaps, pmap, pm_list);
	mutex_exit(&pmap_all_pmaps_slock);

	pool_put(&pmap_pmap_pool, pmap);

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
pmap_activate(struct lwp *l)
{
	pmap_install(vm_map_pmap(&l->l_proc->p_vmspace->vm_map));
}

/*
 * pmap_deactivate:		[ INTERFACE ]
 *
 *	Mark that the pmap used by the specified process is no longer
 *	in use by the processor.
 *
 */

void
pmap_deactivate(struct lwp *l)
{
}

/*
 * pmap_protect:		[ INTERFACE ]
 *
 *	Set the physical protection on the specified range of this map
 *	as requested.
 */
/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 */
void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	pmap_t oldpmap;
	struct ia64_lpte *pte;
	vaddr_t pa;
	struct vm_page *pg;

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}

	if (prot & VM_PROT_WRITE)
		return;

	if ((sva & PAGE_MASK) || (eva & PAGE_MASK))
		panic("pmap_protect: unaligned addresses");

	//uvm_lock_pageq();
	PMAP_LOCK(pmap);
	oldpmap = pmap_install(pmap);
	while (sva < eva) {
		/* 
		 * If page is invalid, skip this page
		 */
		pte = pmap_find_vhpt(sva);
		if (pte == NULL) {
			sva += PAGE_SIZE;
			continue;
		}

		if (pmap_prot(pte) != prot) {
			if (pmap_managed(pte)) {
				pa = pmap_ppn(pte);
				pg = PHYS_TO_VM_PAGE(pa);
				if (pmap_dirty(pte)) pmap_clear_dirty(pte);
				if (pmap_accessed(pte)) {
					pmap_clear_accessed(pte);
				}
			}
			pmap_pte_prot(pmap, pte, prot);
			pmap_invalidate_page(pmap, sva);
		}

		sva += PAGE_SIZE;
	}
	//uvm_unlock_pageq();
	pmap_install(oldpmap);
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
        struct ia64_lpte *pte;
        pmap_t oldpmap;
        paddr_t pa;

        pa = 0;
        mutex_enter(&pmap->pm_slock);
        oldpmap = pmap_install(pmap); /* XXX: isn't this a little inefficient ? */
        pte = pmap_find_vhpt(va);
        if (pte != NULL && pmap_present(pte))
                pap = (paddr_t *) pmap_ppn(pte);
	else {
        	mutex_exit(&pmap->pm_slock);
		return false;	
	}
        pmap_install(oldpmap);
        mutex_exit(&pmap->pm_slock);
        return true;

}

/*
 * pmap_clear_modify:		[ INTERFACE ]
 *
 *	Clear the modify bits on the specified physical page.
 */
bool
pmap_clear_modify(struct vm_page *pg)
{
	bool rv = false;
	struct ia64_lpte *pte;
	pmap_t oldpmap;
	pv_entry_t pv;

	if (pg->flags & PG_FAKE)
		return rv;

	TAILQ_FOREACH(pv, &pg->mdpage.pv_list, pv_list) {
		PMAP_LOCK(pv->pv_pmap);
		oldpmap = pmap_install(pv->pv_pmap);
		pte = pmap_find_vhpt(pv->pv_va);
		KASSERT(pte != NULL);
		if (pmap_dirty(pte)) {
			rv = true;
			pmap_clear_dirty(pte);
			pmap_invalidate_page(pv->pv_pmap, pv->pv_va);
		}
		pmap_install(oldpmap);
		PMAP_UNLOCK(pv->pv_pmap);
	}
	return rv;
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
        struct ia64_lpte *pte;
        pmap_t oldpmap, pmap;
        pv_entry_t pv;

        if ((prot & VM_PROT_WRITE) != 0)
                return;
        if (prot & (VM_PROT_READ | VM_PROT_EXECUTE)) {
                if (pg->flags & PG_RDONLY)
                        return;
                TAILQ_FOREACH(pv, &pg->mdpage.pv_list, pv_list) {
                        pmap = pv->pv_pmap;
                        PMAP_LOCK(pmap);
                        oldpmap = pmap_install(pmap);
                        pte = pmap_find_vhpt(pv->pv_va);
                        KASSERT(pte != NULL);
                        pmap_pte_prot(pmap, pte, prot);
                        pmap_invalidate_page(pmap, pv->pv_va);
                        pmap_install(oldpmap);
                        PMAP_UNLOCK(pmap);
                }

		//UVM_LOCK_ASSERT_PAGEQ(); 

                pg->flags |= PG_RDONLY;
        } else {
                pmap_page_purge(pg);
        }
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
	printf("pmap_reference(%p)\n", pmap);
#endif

	PMAP_LOCK(pmap);
	pmap->pm_count++;
	PMAP_UNLOCK(pmap);
}

/*
 * pmap_clear_reference:	[ INTERFACE ]
 *
 *	Clear the reference bit on the specified physical page.
 */
bool
pmap_clear_reference(struct vm_page *pg)
{
	return false;
}

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

	return ia64_ptob(ppn);
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
        pmap_t oldpmap;
        vaddr_t opa;
        struct ia64_lpte origpte;
        struct ia64_lpte *pte;
        bool managed, wired;
	struct vm_page *pg;
	int error = 0;

        PMAP_MAP_TO_HEAD_LOCK();
        PMAP_LOCK(pmap);
        oldpmap = pmap_install(pmap);
                        
        va &= ~PAGE_MASK;

        managed = false;

	wired = (flags & PMAP_WIRED) !=0;

	pg = PHYS_TO_VM_PAGE(pa);



#ifdef DIAGNOSTIC       
        if (va > VM_MAX_KERNEL_ADDRESS)
                panic("pmap_enter: toobig");
#endif
                
        /*
         * Find (or create) a pte for the given mapping.
         */
        while ((pte = pmap_find_pte(va)) == NULL) { 
                pmap_install(oldpmap);
                PMAP_UNLOCK(pmap);
                PMAP_MAP_TO_HEAD_UNLOCK();
                uvm_kick_pdaemon();
                PMAP_MAP_TO_HEAD_LOCK();
                PMAP_LOCK(pmap);
                oldpmap = pmap_install(pmap);
        }
        origpte = *pte;
        if (!pmap_present(pte)) {
                opa = ~0UL;
                pmap_enter_vhpt(pte, va);
        } else  
                opa = pmap_ppn(pte); 

        /*
         * Mapping has not changed, must be protection or wiring change.
         */
        if (opa == pa) {
                /*
                 * Wiring change, just update stats. We don't worry about
                 * wiring PT pages as they remain resident as long as there
                 * are valid mappings in them. Hence, if a user page is wired,
                 * the PT page will be also.
                 */
                if (wired && !pmap_wired(&origpte))
                        pmap->pm_stats.wired_count++;
                else if (!wired && pmap_wired(&origpte))
                        pmap->pm_stats.wired_count--;

                managed = (pmap_managed(&origpte)) ? true : false;


                /*
                 * We might be turning off write access to the page,
                 * so we go ahead and sense modify status.
                 */
                if (managed && pmap_dirty(&origpte) && pmap_track_modified(va)) 
			pg->flags &= ~PG_CLEAN;

                pmap_invalidate_page(pmap, va);
                goto validate;
        }

        /*
         * Mapping has changed, invalidate old range and fall
         * through to handle validating new mapping.
         */
        if (opa != ~0UL) {
                pmap_remove_pte(pmap, pte, va, 0, 0);
                pmap_enter_vhpt(pte, va);
        }

        /*
         * Enter on the PV list if part of our managed memory.
         */

        if ((flags & (PG_FAKE)) == 0) {
                pmap_insert_entry(pmap, va, pg);
                managed = true;
        }

        /*
         * Increment counters
         */
        pmap->pm_stats.resident_count++;
        if (wired)
                pmap->pm_stats.wired_count++;

validate:

        /*
         * Now validate mapping with desired protection/wiring. This
         * adds the pte to the VHPT if necessary.
         */
        pmap_pte_prot(pmap, pte, prot);
        pmap_set_pte(pte, va, pa, wired, managed);

        PMAP_MAP_TO_HEAD_UNLOCK();
        pmap_install(oldpmap);
        PMAP_UNLOCK(pmap);

	return error; /* XXX: Look into this. */
}


/*
 *	Routine:	pmap_page_purge: => was: pmap_remove_all
 *	Function:
 *		Removes this physical page from
 *		all physical maps in which it resides.
 *		Reflects back modify bits to the pager.
 *
 *	Notes:
 *		Original versions of this routine were very
 *		inefficient because they iteratively called
 *		pmap_remove (slow...)
 */

void
pmap_page_purge(struct vm_page * pg)
{
	pmap_t oldpmap;
	pv_entry_t pv;

#if defined(DIAGNOSTIC)
	/*
	 * XXX this makes pmap_page_protect(NONE) illegal for non-managed
	 * pages!
	 */
	if (pg->flags & PG_FAKE) {
		panic("pmap_page_protect: illegal for unmanaged page, va: 0x%lx", VM_PAGE_TO_PHYS(pg));
	}
#endif
	//UVM_LOCK_ASSERT_PAGEQ();

	while ((pv = TAILQ_FIRST(&pg->mdpage.pv_list)) != NULL) {
		struct ia64_lpte *pte;
		pmap_t pmap = pv->pv_pmap;
		vaddr_t va = pv->pv_va;

		PMAP_LOCK(pmap);
		oldpmap = pmap_install(pmap);
		pte = pmap_find_vhpt(va);
		KASSERT(pte != NULL);
		if (pmap_ppn(pte) != VM_PAGE_TO_PHYS(pg))
			panic("pmap_remove_all: pv_table for %lx is inconsistent", VM_PAGE_TO_PHYS(pg));
		pmap_remove_pte(pmap, pte, va, pv, 1);
		pmap_install(oldpmap);
		PMAP_UNLOCK(pmap);
	}

	//UVM_LOCK_ASSERT_PAGEQ(); 
	pg->flags |= PG_RDONLY;

}


pmap_t
pmap_switch(pmap_t pm)
{
        pmap_t prevpm;
        int i;

        //LOCK_ASSERT(simple_lock_held(&sched_lock));
		    
	prevpm = curcpu()->ci_pmap;
        if (prevpm == pm)
                return prevpm;
//        if (prevpm != NULL)
//                atomic_clear_32(&prevpm->pm_active, PCPU_GET(cpumask));
        if (pm == NULL) {
                for (i = 0; i < 5; i++) {
                        ia64_set_rr(IA64_RR_BASE(i),
                            (i << 8)|(PAGE_SHIFT << 2)|1);
                }
        } else {
                for (i = 0; i < 5; i++) {
                        ia64_set_rr(IA64_RR_BASE(i),
                            (pm->pm_rid[i] << 8)|(PAGE_SHIFT << 2)|1);
                }
//                atomic_set_32(&pm->pm_active, PCPU_GET(cpumask));
        }
        curcpu()->ci_pmap = pm;
	ia64_srlz_d();
        return prevpm;
}

static pmap_t
pmap_install(pmap_t pm)
{
        pmap_t prevpm;

	int splsched;

        splsched = splsched();
        prevpm = pmap_switch(pm);
	splx(splsched);
        return prevpm;
}

static uint32_t
pmap_allocate_rid(void)
{
	uint64_t bit, bits;
	int rid;

	mutex_enter(&pmap_rid_lock);
	if (pmap_ridcount == pmap_ridmax)
		panic("pmap_allocate_rid: All Region IDs used");

	/* Find an index with a free bit. */
	while ((bits = pmap_ridmap[pmap_rididx]) == ~0UL) {
		pmap_rididx++;
		if (pmap_rididx == pmap_ridmapsz)
			pmap_rididx = 0;
	}
	rid = pmap_rididx * 64;

	/* Find a free bit. */
	bit = 1UL;
	while (bits & bit) {
		rid++;
		bit <<= 1;
	}

	pmap_ridmap[pmap_rididx] |= bit;
	pmap_ridcount++;
	mutex_exit(&pmap_rid_lock);

	return rid;
}

static void
pmap_free_rid(uint32_t rid)
{
	uint64_t bit;
	int idx;

	idx = rid / 64;
	bit = ~(1UL << (rid & 63));

	mutex_enter(&pmap_rid_lock);
	pmap_ridmap[idx] &= bit;
	pmap_ridcount--;
	mutex_exit(&pmap_rid_lock);
}

/***************************************************
 * Manipulate TLBs for a pmap
 ***************************************************/

static void
pmap_invalidate_page(pmap_t pmap, vaddr_t va)
{
	KASSERT((pmap == pmap_kernel() || pmap == curcpu()->ci_pmap));
	ia64_ptc_g(va, PAGE_SHIFT << 2);
}

static void
pmap_invalidate_all_1(void *arg)
{
	uint64_t addr;
	int i, j;
	register_t psr;

	psr = intr_disable();
	addr = pmap_ptc_e_base;
	for (i = 0; i < pmap_ptc_e_count1; i++) {
		for (j = 0; j < pmap_ptc_e_count2; j++) {
			ia64_ptc_e(addr);
			addr += pmap_ptc_e_stride2;
		}
		addr += pmap_ptc_e_stride1;
	}
	intr_restore(psr);
}

static void
pmap_invalidate_all(pmap_t pmap)
{
	KASSERT(pmap == pmap_kernel() || pmap == curcpu()->ci_pmap);


#ifdef MULTIPROCESSOR
	smp_rendezvous(0, pmap_invalidate_all_1, 0, 0);
#else
	pmap_invalidate_all_1(0);
#endif
}

/***************************************************
 * Low level mapping routines.....
 ***************************************************/

/*
 * Find the kernel lpte for mapping the given virtual address, which
 * must be in the part of region 5 which we can cover with our kernel
 * 'page tables'.
 */
static struct ia64_lpte *
pmap_find_kpte(vaddr_t va)
{
	KASSERT((va >> 61) == 5);
	KASSERT(IA64_RR_MASK(va) < (nkpt * PAGE_SIZE * NKPTEPG));
	return &ia64_kptdir[KPTE_DIR_INDEX(va)][KPTE_PTE_INDEX(va)];
}


/***************************************************
 * Low level helper routines.....
 ***************************************************/

/*
 * Find a pte suitable for mapping a user-space address. If one exists 
 * in the VHPT, that one will be returned, otherwise a new pte is
 * allocated.
 */
static struct ia64_lpte *
pmap_find_pte(vaddr_t va)
{
	struct ia64_lpte *pte;

	if (va >= VM_MAXUSER_ADDRESS)
		return pmap_find_kpte(va);

	pte = pmap_find_vhpt(va);
	if (pte == NULL) {
		pte = pool_get(&pmap_ia64_lpte_pool, PR_NOWAIT);
		pte->tag = 1UL << 63;
	}
	
	return pte;
}

static __inline void
pmap_pte_prot(pmap_t pm, struct ia64_lpte *pte, vm_prot_t prot)
{
        static int prot2ar[4] = {
                PTE_AR_R,       /* VM_PROT_NONE */
                PTE_AR_RW,      /* VM_PROT_WRITE */
                PTE_AR_RX,      /* VM_PROT_EXECUTE */
                PTE_AR_RWX      /* VM_PROT_WRITE|VM_PROT_EXECUTE */
        };

        pte->pte &= ~(PTE_PROT_MASK | PTE_PL_MASK | PTE_AR_MASK);
        pte->pte |= (uint64_t)(prot & VM_PROT_ALL) << 56;
        pte->pte |= (prot == VM_PROT_NONE || pm == pmap_kernel())
            ? PTE_PL_KERN : PTE_PL_USER;
        pte->pte |= prot2ar[(prot & VM_PROT_ALL) >> 1];
}



/*
 * Set a pte to contain a valid mapping and enter it in the VHPT. If
 * the pte was orginally valid, then its assumed to already be in the
 * VHPT.
 * This functions does not set the protection bits.  It's expected
 * that those have been set correctly prior to calling this function.
 */
static void
pmap_set_pte(struct ia64_lpte *pte, vaddr_t va, vaddr_t pa,
    bool wired, bool managed)
{

        pte->pte &= PTE_PROT_MASK | PTE_PL_MASK | PTE_AR_MASK;
        pte->pte |= PTE_PRESENT | PTE_MA_WB;
        pte->pte |= (managed) ? PTE_MANAGED : (PTE_DIRTY | PTE_ACCESSED);
        pte->pte |= (wired) ? PTE_WIRED : 0;
        pte->pte |= pa & PTE_PPN_MASK;

        pte->itir = PAGE_SHIFT << 2;

        pte->tag = ia64_ttag(va);
}

/*
 * Remove the (possibly managed) mapping represented by pte from the
 * given pmap.
 */
static int
pmap_remove_pte(pmap_t pmap, struct ia64_lpte *pte, vaddr_t va,
		pv_entry_t pv, int freepte)
{
	int error;
	struct vm_page *pg;

	KASSERT(pmap == pmap_kernel() || pmap == curcpu()->ci_pmap);

	/*
	 * First remove from the VHPT.
	 */
	error = pmap_remove_vhpt(va);
	if (error)
		return error;

	pmap_invalidate_page(pmap, va);

	if (pmap_wired(pte))
		pmap->pm_stats.wired_count -= 1;

	pmap->pm_stats.resident_count -= 1;
	if (pmap_managed(pte)) {
		pg = PHYS_TO_VM_PAGE(pmap_ppn(pte));
		if (pmap_dirty(pte))
			if (pmap_track_modified(va))
				pg->flags &= ~(PG_CLEAN);
		if (pmap_accessed(pte))
			pg->flags &= ~PG_CLEAN; /* XXX: Do we need this ? */


		if (freepte)
			pmap_free_pte(pte, va);

		error = pmap_remove_entry(pmap, pg, va, pv);

	}
	if (freepte)
		pmap_free_pte(pte, va);
	return 0;
}



/*
 * Free a pte which is now unused. This simply returns it to the zone
 * allocator if it is a user mapping. For kernel mappings, clear the
 * valid bit to make it clear that the mapping is not currently used.
 */
static void
pmap_free_pte(struct ia64_lpte *pte, vaddr_t va)
{
	if (va < VM_MAXUSER_ADDRESS)
	  while (0);
	  //		pool_put(pool_ia64_lpte_pool, pte); XXX: Fixme for userspace
	else
		pmap_clear_present(pte);
}


/*
 * this routine defines the region(s) of memory that should
 * not be tested for the modified bit.
 */
static __inline int
pmap_track_modified(vaddr_t va)
{
	extern char *kmembase, kmemlimit;
	if ((va < (vaddr_t) kmembase) || (va >= (vaddr_t) kmemlimit)) 
		return 1;
	else
		return 0;
}


/***************************************************
 * page management routines.
 ***************************************************/


/*
 * get a new pv_entry, allocating a block from the system
 * when needed.
 * the memory allocation is performed bypassing the malloc code
 * because of the possibility of allocations at interrupt time.
 */
/*
 * get a new pv_entry, allocating a block from the system
 * when needed.
 */
static pv_entry_t
get_pv_entry(pmap_t locked_pmap)
{
	pv_entry_t allocated_pv;

	//LOCK_ASSERT(simple_lock_held(locked_pmap->slock));
	//UVM_LOCK_ASSERT_PAGEQ();
	allocated_pv = 	pool_get(&pmap_pv_pool, PR_NOWAIT);
	return allocated_pv;


	/* XXX: Nice to have all this stuff later:
	 * Reclaim pv entries: At first, destroy mappings to inactive
	 * pages.  After that, if a pv entry is still needed, destroy
	 * mappings to active pages.
	 */
}

/*
 * free the pv_entry back to the free list
 */
static __inline void
free_pv_entry(pv_entry_t pv)
{
	pool_put(&pmap_pv_pool, pv);
}


/*
 * Add an ia64_lpte to the VHPT.
 */
static void
pmap_enter_vhpt(struct ia64_lpte *pte, vaddr_t va)
{
	struct ia64_bucket *bckt;
	struct ia64_lpte *vhpte;
	uint64_t pte_pa;

	/* Can fault, so get it out of the way. */
	pte_pa = ia64_tpa((vaddr_t)pte);

	vhpte = (struct ia64_lpte *)ia64_thash(va);
	bckt = (struct ia64_bucket *)vhpte->chain;
	/* XXX: fixme */
	mutex_enter(&bckt->lock);
	pte->chain = bckt->chain;
	ia64_mf();
	bckt->chain = pte_pa;

	pmap_vhpt_inserts++;
	bckt->length++;
	/*XXX : fixme */
	mutex_exit(&bckt->lock);

}

/*
 * Remove the ia64_lpte matching va from the VHPT. Return zero if it
 * worked or an appropriate error code otherwise.
 */
static int
pmap_remove_vhpt(vaddr_t va)
{
	struct ia64_bucket *bckt;
	struct ia64_lpte *pte;
	struct ia64_lpte *lpte;
	struct ia64_lpte *vhpte;
	uint64_t chain, tag;

	tag = ia64_ttag(va);
	vhpte = (struct ia64_lpte *)ia64_thash(va);
	bckt = (struct ia64_bucket *)vhpte->chain;

	lpte = NULL;
	mutex_enter(&bckt->lock);


	chain = bckt->chain;
	pte = (struct ia64_lpte *)IA64_PHYS_TO_RR7(chain);
	while (chain != 0 && pte->tag != tag) {
		lpte = pte;
		chain = pte->chain;
		pte = (struct ia64_lpte *)IA64_PHYS_TO_RR7(chain);
	}
	if (chain == 0) {
		mutex_exit(&bckt->lock);
		return ENOENT;
	}

	/* Snip this pv_entry out of the collision chain. */
	if (lpte == NULL)
		bckt->chain = pte->chain;
	else
		lpte->chain = pte->chain;
	ia64_mf();

	bckt->length--;
	mutex_exit(&bckt->lock);
	return 0;
}


/*
 * Find the ia64_lpte for the given va, if any.
 */
static struct ia64_lpte *
pmap_find_vhpt(vaddr_t va)
{
	struct ia64_bucket *bckt;
	struct ia64_lpte *pte;
	uint64_t chain, tag;

	tag = ia64_ttag(va);
	pte = (struct ia64_lpte *)ia64_thash(va);
	bckt = (struct ia64_bucket *)pte->chain;

	mutex_enter(&bckt->lock);
	chain = bckt->chain;
	pte = (struct ia64_lpte *)IA64_PHYS_TO_RR7(chain);
	while (chain != 0 && pte->tag != tag) {
		chain = pte->chain;
		pte = (struct ia64_lpte *)IA64_PHYS_TO_RR7(chain);
	}
	mutex_exit(&bckt->lock);
	return (chain != 0) ? pte : NULL;
}


/*
 * Remove an entry from the list of managed mappings.
 */
static int
pmap_remove_entry(pmap_t pmap, struct vm_page * pg, vaddr_t va, pv_entry_t pv)
{
	if (!pv) {
		if (pg->mdpage.pv_list_count < pmap->pm_stats.resident_count) {
			TAILQ_FOREACH(pv, &pg->mdpage.pv_list, pv_list) {
				if (pmap == pv->pv_pmap && va == pv->pv_va) 
					break;
			}
		} else {
			TAILQ_FOREACH(pv, &pmap->pm_pvlist, pv_plist) {
				if (va == pv->pv_va) 
					break;
			}
		}
	}

	if (pv) {
		TAILQ_REMOVE(&pg->mdpage.pv_list, pv, pv_list);
		pg->mdpage.pv_list_count--;
		if (TAILQ_FIRST(&pg->mdpage.pv_list) == NULL) {
			//UVM_LOCK_ASSERT_PAGEQ(); 
			pg->flags |= PG_RDONLY;
		}

		TAILQ_REMOVE(&pmap->pm_pvlist, pv, pv_plist);
		free_pv_entry(pv);
		return 0;
	} else {
		return ENOENT;
	}
}


/*
 * Create a pv entry for page at pa for
 * (pmap, va).
 */
static void
pmap_insert_entry(pmap_t pmap, vaddr_t va, struct vm_page *pg)
{
	pv_entry_t pv;

	pv = get_pv_entry(pmap);
	pv->pv_pmap = pmap;
	pv->pv_va = va;

	//LOCK_ASSERT(simple_lock_held(pmap->slock));
	//UVM_LOCK_ASSERT_PAGEQ(); 
	TAILQ_INSERT_TAIL(&pmap->pm_pvlist, pv, pv_plist);
	TAILQ_INSERT_TAIL(&pg->mdpage.pv_list, pv, pv_list);
	pg->mdpage.pv_list_count++;
}


/*
 * Remove a single page from a process address space
 */
static void
pmap_remove_page(pmap_t pmap, vaddr_t va)
{
	struct ia64_lpte *pte;

	KASSERT(pmap == pmap_kernel() || pmap == curcpu()->ci_pmap);

	pte = pmap_find_vhpt(va);
	if (pte) {
		pmap_remove_pte(pmap, pte, va, 0, 1);
		pmap_invalidate_page(pmap, va);
	}
	return;
}



/*
 * pmap_pv_page_alloc:
 *
 *	Allocate a page for the pv_entry pool.
 */
void *
pmap_pv_page_alloc(struct pool *pp, int flags)
{
	paddr_t pg;

	if (pmap_poolpage_alloc(&pg))
		return (void *)IA64_PHYS_TO_RR7(pg);
	return NULL;
}

/*
 * pmap_pv_page_free:
 *
 *	Free a pv_entry pool page.
 */
void
pmap_pv_page_free(struct pool *pp, void *v)
{

	pmap_poolpage_free(IA64_RR_MASK((vaddr_t)v));
}

/******************** misc. functions ********************/

/*
 * pmap_poolpage_alloc: based on alpha/pmap_physpage_alloc
 *
 *	Allocate a single page from the VM system and return the
 *	physical address for that page.
 */
bool
pmap_poolpage_alloc(paddr_t *pap)
{
	struct vm_page *pg;
	paddr_t pa;

	pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE|UVM_PGA_ZERO);
	if (pg != NULL) {
		pa = VM_PAGE_TO_PHYS(pg);

#ifdef DEBUG
		mutex_enter(&pg->mdpage.pv_mutex);
		if (pg->wire_count != 0) {
			printf("pmap_physpage_alloc: page 0x%lx has "
			    "%d references\n", pa, pg->wire_count);
			panic("pmap_physpage_alloc");
		}
		mutex_exit(&pg->mdpage.pv_mutex);
#endif
		*pap = pa;
		return true;
	}
	return false;
}

/*
 * pmap_poolpage_free: based on alpha/pmap_physpage_free:
 *
 *	Free the single page table page at the specified physical address.
 */
void
pmap_poolpage_free(paddr_t pa)
{
	struct vm_page *pg;

	if ((pg = PHYS_TO_VM_PAGE(pa)) == NULL)
		panic("pmap_physpage_free: bogus physical page address");

#ifdef DEBUG
	mutex_enter(&pg->mdpage.pv_mutex);
	if (pg->wire_count != 0)
		panic("pmap_physpage_free: page still has references");
	mutex_exit(&pg->mdpage.pv_mutex);
#endif

	uvm_pagefree(pg);
}

#ifdef DEBUG

static void dump_vhpt(void)
{

	vaddr_t base;
	vsize_t size, i;
	struct ia64_lpte *pte;

	__asm __volatile("mov %0=cr.pta;; srlz.i;;" :
			 "=r" (base));

#define VHPTBASE(x) ( (x) & (~0x7fffUL) )
#define VHPTSIZE(x) ( (vsize_t) (1 << (((x) & 0x7cUL) >> 2)))

	size = VHPTSIZE(base);
	base = VHPTBASE(base);

	pte = (void *) base;

	printf("vhpt base = %lx \n", base);
	printf("vhpt size = %lx \n", size);

	for(i = 0; i < size/sizeof(struct ia64_lpte);i++ ) {
		if(pte[i].pte & PTE_PRESENT) {
			printf("PTE_PRESENT ");

			if(pte[i].pte & PTE_MA_MASK) printf("MA: ");
			if(pte[i].pte & PTE_MA_WB) printf("WB ");
			if(pte[i].pte & PTE_MA_UC) printf("UC ");
			if(pte[i].pte & PTE_MA_UCE) printf("UCE ");
			if(pte[i].pte & PTE_MA_WC) printf("WC ");
			if(pte[i].pte & PTE_MA_NATPAGE) printf("NATPAGE ");

			if(pte[i].pte & PTE_ACCESSED) printf("PTE_ACCESSED ");
			if(pte[i].pte & PTE_DIRTY) printf("PTE_DIRTY ");

			if(pte[i].pte & PTE_PL_MASK) printf("PL: ");
			if(pte[i].pte & PTE_PL_KERN) printf("KERN");
			if(pte[i].pte & PTE_PL_USER) printf("USER");

			if(pte[i].pte & PTE_AR_MASK) printf("AR: ");
			if(pte[i].pte & PTE_AR_R) printf("R ");
			if(pte[i].pte & PTE_AR_RX) printf("RX ");
			if(pte[i].pte & PTE_AR_RWX) printf("RWX ");
			if(pte[i].pte & PTE_AR_R_RW) printf("R RW ");
			if(pte[i].pte & PTE_AR_RX_RWX) printf("RX RWX ");

			printf("ppn = %lx", (pte[i].pte & PTE_PPN_MASK) >> 12);

			if(pte[i].pte & PTE_ED) printf("ED ");

			if(pte[i].pte & PTE_IG_MASK) printf("OS: ");
			if(pte[i].pte & PTE_WIRED) printf("WIRED ");
			if(pte[i].pte & PTE_MANAGED) printf("MANAGED ");
			printf("\n");
		}

	}



}
#endif
