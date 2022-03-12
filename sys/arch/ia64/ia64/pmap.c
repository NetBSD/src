/* $NetBSD: pmap.c,v 1.41 2022/03/12 15:32:31 riastradh Exp $ */

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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.41 2022/03/12 15:32:31 riastradh Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/lock.h>
#include <sys/pool.h>
#include <sys/sched.h>
#include <sys/bitops.h>

#include <uvm/uvm.h>
#include <uvm/uvm_physseg.h>

#include <machine/pal.h>
#include <machine/atomic.h>
#include <machine/pte.h>
#include <machine/cpufunc.h>
#include <machine/md_var.h>
#include <machine/vmparam.h>

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

/*
 * Following the Linux model, region IDs are allocated in groups of
 * eight so that a single region ID can be used for as many RRs as we
 * want by encoding the RR number into the low bits of the ID.
 *
 * We reserve region ID 0 for the kernel and allocate the remaining
 * IDs for user pmaps.
 *
 * Region 0-3:	User virtually mapped
 * Region 4:	PBVM and special mappings
 * Region 5:	Kernel virtual memory
 * Region 6:	Direct-mapped uncacheable
 * Region 7:	Direct-mapped cacheable
 */

#if !defined(DIAGNOSTIC)
#define PMAP_INLINE __inline
#else
#define PMAP_INLINE
#endif

#ifdef PV_STATS
#define PV_STAT(x)	do { x ; } while (0)
#else
#define PV_STAT(x)	do { } while (0)
#endif

#define	pmap_accessed(lpte)		((lpte)->pte & PTE_ACCESSED)
#define	pmap_dirty(lpte)		((lpte)->pte & PTE_DIRTY)
#define	pmap_exec(lpte)			((lpte)->pte & PTE_AR_RX)
#define	pmap_managed(lpte)		((lpte)->pte & PTE_MANAGED)
#define	pmap_ppn(lpte)			((lpte)->pte & PTE_PPN_MASK)
#define	pmap_present(lpte)		((lpte)->pte & PTE_PRESENT)
#define	pmap_prot(lpte)			(((lpte)->pte & PTE_PROT_MASK) >> 56)
#define	pmap_wired(lpte)		((lpte)->pte & PTE_WIRED)

#define	pmap_clear_accessed(lpte)	(lpte)->pte &= ~PTE_ACCESSED
#define	pmap_clear_dirty(lpte)		(lpte)->pte &= ~PTE_DIRTY
#define	pmap_clear_present(lpte)	(lpte)->pte &= ~PTE_PRESENT
#define	pmap_clear_wired(lpte)		(lpte)->pte &= ~PTE_WIRED

#define	pmap_set_wired(lpte)		(lpte)->pte |= PTE_WIRED

/*
 * Individual PV entries are stored in per-pmap chunks.  This saves
 * space by eliminating the need to record the pmap within every PV
 * entry.
 */
#if PAGE_SIZE == 8192
#define	_NPCM	6
#define	_NPCPV	337
#define	_NPCS	2
#elif PAGE_SIZE == 16384
#define	_NPCM	11
#define	_NPCPV	677
#define	_NPCS	1
#else
#error "invalid page size"
#endif

struct pv_chunk {
	pmap_t			pc_pmap;
	TAILQ_ENTRY(pv_chunk)	pc_list;
	u_long			pc_map[_NPCM];	/* bitmap; 1 = free */
	TAILQ_ENTRY(pv_chunk)	pc_lru;
	u_long			pc_spare[_NPCS];
	struct pv_entry		pc_pventry[_NPCPV];
};

/*
 * The VHPT bucket head structure.
 */
struct ia64_bucket {
	uint64_t	chain;
	kmutex_t	mutex;
	u_int		length;
};

/*
 * Statically allocated kernel pmap
 */
static struct pmap kernel_pmap_store;/* the kernel's pmap (proc0) */
struct pmap *const kernel_pmap_ptr = &kernel_pmap_store;

vaddr_t virtual_avail;	/* VA of first avail page (after kernel bss) */
vaddr_t virtual_end;	/* VA of last avail page (end of kernel AS) */

/* XXX freebsd, needs to be sorted out */
#define kernel_pmap			pmap_kernel()
#define critical_enter()		kpreempt_disable()
#define critical_exit()			kpreempt_enable()
/* flags the entire page as dirty */
#define vm_page_dirty(page)		(page->flags &= ~PG_CLEAN)
#define vm_page_is_managed(page) (pmap_initialized && uvm_pageismanaged(VM_PAGE_TO_PHYS(page)))

/*
 * Kernel virtual memory management.
 */
static int nkpt;

extern struct ia64_lpte ***ia64_kptdir;

#define KPTE_DIR0_INDEX(va) \
	(((va) >> (3*PAGE_SHIFT-8)) & ((1<<(PAGE_SHIFT-3))-1))
#define KPTE_DIR1_INDEX(va) \
	(((va) >> (2*PAGE_SHIFT-5)) & ((1<<(PAGE_SHIFT-3))-1))
#define KPTE_PTE_INDEX(va) \
	(((va) >> PAGE_SHIFT) & ((1<<(PAGE_SHIFT-5))-1))

#define NKPTEPG		(PAGE_SIZE / sizeof(struct ia64_lpte))

vaddr_t kernel_vm_end;

/* Defaults for ptc.e. */
/*
static uint64_t pmap_ptc_e_base = 0;
static uint32_t pmap_ptc_e_count1 = 1;
static uint32_t pmap_ptc_e_count2 = 1;
static uint32_t pmap_ptc_e_stride1 = 0;
static uint32_t pmap_ptc_e_stride2 = 0;
*/
/* Values for ptc.e. XXX values for SKI, add SKI kernel option methinks */
static uint64_t pmap_ptc_e_base = 0x100000000;
static uint64_t pmap_ptc_e_count1 = 3;
static uint64_t pmap_ptc_e_count2 = 2;
static uint64_t pmap_ptc_e_stride1 = 0x2000;
static uint64_t pmap_ptc_e_stride2 = 0x100000000;

kmutex_t pmap_ptc_mutex;

/*
 * Data for the RID allocator
 */
static int pmap_ridcount;
static int pmap_rididx;
static int pmap_ridmapsz;
static int pmap_ridmax;
static uint64_t *pmap_ridmap;
kmutex_t pmap_ridmutex;

static krwlock_t pvh_global_lock __attribute__ ((aligned (128)));

static pool_cache_t pmap_pool_cache;

/*
 * Data for the pv entry allocation mechanism
 */
static TAILQ_HEAD(pch, pv_chunk) pv_chunks = TAILQ_HEAD_INITIALIZER(pv_chunks);
static int pv_entry_count;

/*
 * Data for allocating PTEs for user processes.
 */
static pool_cache_t pte_pool_cache;

struct ia64_bucket *pmap_vhpt_bucket;

/* XXX For freebsd, these are sysctl variables */
static uint64_t pmap_vhpt_nbuckets = 0;
uint64_t pmap_vhpt_log2size = 0;
static uint64_t pmap_vhpt_inserts = 0;

static bool pmap_initialized = false;   /* Has pmap_init completed? */
static uint64_t pmap_pages_stolen = 0;  /* instrumentation */

static struct ia64_lpte *pmap_find_vhpt(vaddr_t va);

static void free_pv_chunk(struct pv_chunk *pc);
static void free_pv_entry(pmap_t pmap, pv_entry_t pv);
static pv_entry_t get_pv_entry(pmap_t pmap, bool try);
static struct vm_page *pmap_pv_reclaim(pmap_t locked_pmap);

static void	pmap_free_pte(struct ia64_lpte *pte, vaddr_t va);
static int	pmap_remove_pte(pmap_t pmap, struct ia64_lpte *pte,
		    vaddr_t va, pv_entry_t pv, int freepte);
static int	pmap_remove_vhpt(vaddr_t va);

static vaddr_t	pmap_steal_vhpt_memory(vsize_t);

static struct vm_page *vm_page_alloc1(void);
static void vm_page_free1(struct vm_page *pg);

static vm_memattr_t pmap_flags_to_memattr(u_int flags);

#if DEBUG
static void pmap_testout(void);
#endif

static void
pmap_initialize_vhpt(vaddr_t vhpt)
{
	struct ia64_lpte *pte;
	u_int i;

	pte = (struct ia64_lpte *)vhpt;
	for (i = 0; i < pmap_vhpt_nbuckets; i++) {
		pte[i].pte = 0;
		pte[i].itir = 0;
		pte[i].tag = 1UL << 63; /* Invalid tag */
		pte[i].chain = (uintptr_t)(pmap_vhpt_bucket + i);
	}
}

#ifdef MULTIPROCESSOR
vaddr_t
pmap_alloc_vhpt(void)
{
	vaddr_t vhpt;
	struct vm_page *m;
	vsize_t size;

	size = 1UL << pmap_vhpt_log2size;
	m = vm_page_alloc_contig(NULL, 0, VM_ALLOC_SYSTEM | VM_ALLOC_NOOBJ |
	    VM_ALLOC_WIRED, atop(size), 0UL, ~0UL, size, 0UL,
	    VM_MEMATTR_DEFAULT);
	if (m != NULL) {
		vhpt = IA64_PHYS_TO_RR7(VM_PAGE_TO_PHYS(m));
		pmap_initialize_vhpt(vhpt);
		return (vhpt);
	}
	return (0);
}
#endif

/*
 *	Bootstrap the system enough to run with virtual memory.
 */
void
pmap_bootstrap(void)
{
	struct ia64_pal_result res;
	vaddr_t base;
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

	mutex_init(&pmap_ptc_mutex, MUTEX_DEFAULT, IPL_VM);

	/*
	 * Setup RIDs. RIDs 0..7 are reserved for the kernel.
	 *
	 * We currently need at least 19 bits in the RID because PID_MAX
	 * can only be encoded in 17 bits and we need RIDs for 4 regions
	 * per process. With PID_MAX equalling 99999 this means that we
	 * need to be able to encode 399996 (=4*PID_MAX).
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
	mutex_init(&pmap_ridmutex, MUTEX_DEFAULT, IPL_VM);

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

	nkpt = (((ubc_nwins << ubc_winshift) +
		 bufsz + 16 * NCARGS + pager_map_size) / PAGE_SIZE +
		USRIOSIZE + (maxproc * UPAGES) + nkmempages) / NKPTEPG;

	/*
	 * Allocate some memory for initial kernel 'page tables'.
	 */
	ia64_kptdir = (void *)uvm_pageboot_alloc(PAGE_SIZE);
	memset((void *)ia64_kptdir, 0, PAGE_SIZE);
	nkpt = 0;
	kernel_vm_end = VM_INIT_KERNEL_ADDRESS;
	
	/*
	 * Determine a valid (mappable) VHPT size.
	 */
	if (pmap_vhpt_log2size == 0)
		pmap_vhpt_log2size = 20;
	else if (pmap_vhpt_log2size < 16)
		pmap_vhpt_log2size = 16;
	else if (pmap_vhpt_log2size > 28)
		pmap_vhpt_log2size = 28;
	if (pmap_vhpt_log2size & 1)
		pmap_vhpt_log2size--;

	size = 1UL << pmap_vhpt_log2size;
	/* XXX add some retries here */
	base = pmap_steal_vhpt_memory(size);
	
	curcpu()->ci_vhpt = base;

	if (base == 0)
		panic("Unable to allocate VHPT");

	pmap_vhpt_nbuckets = size / sizeof(struct ia64_lpte);
	pmap_vhpt_bucket = (void *)uvm_pageboot_alloc(pmap_vhpt_nbuckets *
						      sizeof(struct ia64_bucket));
	if (bootverbose)
		printf("VHPT: address=%#lx, size=%#lx, buckets=%ld, address=%lx\n",
		       base, size, pmap_vhpt_nbuckets, (long unsigned int)&pmap_vhpt_bucket[0]);

	for (i = 0; i < pmap_vhpt_nbuckets; i++) {
		/* Stolen memory is zeroed. */
		mutex_init(&pmap_vhpt_bucket[i].mutex, MUTEX_DEFAULT, IPL_VM);
	}

	pmap_initialize_vhpt(base);
	map_vhpt(base);
	ia64_set_pta(base + (1 << 8) + (pmap_vhpt_log2size << 2) + 1);
	ia64_srlz_i();

	/* XXX
	 virtual_avail = VM_INIT_KERNEL_ADDRESS;
	 virtual_end = VM_MAX_KERNEL_ADDRESS;
	*/
	
	/*
	 * Initialize the kernel pmap (which is statically allocated).
	 */
	PMAP_LOCK_INIT(kernel_pmap);
	for (i = 0; i < IA64_VM_MINKERN_REGION; i++)
		kernel_pmap->pm_rid[i] = 0;
	TAILQ_INIT(&kernel_pmap->pm_pvchunk);

	bzero(&kernel_pmap->pm_stats, sizeof kernel_pmap->pm_stats);
	kernel_pmap->pm_refcount = 1;

	curcpu()->ci_pmap = kernel_pmap;

 	/*
	 * Initialize the global pv list lock.
	 */
	rw_init(&pvh_global_lock);

	/* Region 5 is mapped via the VHPT. */
	ia64_set_rr(IA64_RR_BASE(5), (5 << 8) | (PAGE_SHIFT << 2) | 1);

	/*
	 * Clear out any random TLB entries left over from booting.
	 */
	pmap_invalidate_all();

	map_gateway_page();
}

vaddr_t
pmap_page_to_va(struct vm_page *m)
{
	paddr_t pa;
	vaddr_t va;

	pa = VM_PAGE_TO_PHYS(m);
	va = (m->mdpage.memattr == VM_MEMATTR_UNCACHEABLE) ? IA64_PHYS_TO_RR6(pa) :
	    IA64_PHYS_TO_RR7(pa);
	return (va);
}

/***************************************************
 * Manipulate TLBs for a pmap
 ***************************************************/

static void
pmap_invalidate_page(vaddr_t va)
{
	struct ia64_lpte *pte;
	//struct pcpu *pc;
	uint64_t tag;
	u_int vhpt_ofs;
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	
	critical_enter();

	vhpt_ofs = ia64_thash(va) - curcpu()->ci_vhpt;
	
	tag = ia64_ttag(va);

	for (CPU_INFO_FOREACH(cii,ci)) {
		pte = (struct ia64_lpte *)(ci->ci_vhpt + vhpt_ofs);
		atomic_cmpset_64(&pte->tag, tag, 1UL << 63);
	}
	
	mutex_spin_enter(&pmap_ptc_mutex);

	ia64_ptc_ga(va, PAGE_SHIFT << 2);
	ia64_mf();
	ia64_srlz_i();

	mutex_spin_exit(&pmap_ptc_mutex);
	
	ia64_invala();

	critical_exit();
}

void
pmap_invalidate_all(void)
{
	uint64_t addr;
	int i, j;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	
	addr = pmap_ptc_e_base;
	for (i = 0; i < pmap_ptc_e_count1; i++) {
		for (j = 0; j < pmap_ptc_e_count2; j++) {
			ia64_ptc_e(addr);
			addr += pmap_ptc_e_stride2;
		}
		addr += pmap_ptc_e_stride1;
	}
	ia64_srlz_i();
}

static uint32_t
pmap_allocate_rid(void)
{
	uint64_t bit, bits;
	int rid;

	mutex_enter(&pmap_ridmutex);
	
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

	mutex_exit(&pmap_ridmutex);
	
	return rid;
}

static void
pmap_free_rid(uint32_t rid)
{
	uint64_t bit;
	int idx;

	idx = rid / 64;
	bit = ~(1UL << (rid & 63));

	mutex_enter(&pmap_ridmutex);
	pmap_ridmap[idx] &= bit;
	pmap_ridcount--;

	mutex_exit(&pmap_ridmutex);
}

/***************************************************
 *  Page table page management routines.....
 ***************************************************/
CTASSERT(sizeof(struct pv_chunk) == PAGE_SIZE);

static __inline struct pv_chunk *
pv_to_chunk(pv_entry_t pv)
{
	return ((struct pv_chunk *)((uintptr_t)pv & ~(uintptr_t)PAGE_MASK));
}

#define PV_PMAP(pv) (pv_to_chunk(pv)->pc_pmap)

#define	PC_FREE_FULL	0xfffffffffffffffful
#define	PC_FREE_PARTIAL	\
	((1UL << (_NPCPV - sizeof(u_long) * 8 * (_NPCM - 1))) - 1)

#if PAGE_SIZE == 8192
static const u_long pc_freemask[_NPCM] = {
	PC_FREE_FULL, PC_FREE_FULL, PC_FREE_FULL,
	PC_FREE_FULL, PC_FREE_FULL, PC_FREE_PARTIAL
};
#elif PAGE_SIZE == 16384
static const u_long pc_freemask[_NPCM] = {
	PC_FREE_FULL, PC_FREE_FULL, PC_FREE_FULL,
	PC_FREE_FULL, PC_FREE_FULL, PC_FREE_FULL,
	PC_FREE_FULL, PC_FREE_FULL, PC_FREE_FULL,
	PC_FREE_FULL, PC_FREE_PARTIAL
};
#endif

#ifdef PV_STATS
static int pc_chunk_count, pc_chunk_allocs, pc_chunk_frees, pc_chunk_tryfail;
static long pv_entry_frees, pv_entry_allocs;
static int pv_entry_spare;
#endif

/*
 * We are in a serious low memory condition.  Resort to
 * drastic measures to free some pages so we can allocate
 * another pv entry chunk.
 */
static struct vm_page
*pmap_pv_reclaim(pmap_t locked_pmap)
{
	struct pch newtail;
	struct pv_chunk *pc;
	struct ia64_lpte *pte;
	pmap_t pmap;
	pv_entry_t pv;
	vaddr_t va;
	struct vm_page *m;
	struct vm_page *m_pc;
	u_long inuse;
	int bit, field, freed, idx;

	PMAP_LOCK_ASSERT(locked_pmap);
	pmap = NULL;
	m_pc = NULL;
	TAILQ_INIT(&newtail);
	while ((pc = TAILQ_FIRST(&pv_chunks)) != NULL) {
		TAILQ_REMOVE(&pv_chunks, pc, pc_lru);
		if (pmap != pc->pc_pmap) {
			if (pmap != NULL) {
				if (pmap != locked_pmap) {
					pmap_switch(locked_pmap);
					PMAP_UNLOCK(pmap);
				}
			}
			pmap = pc->pc_pmap;
			/* Avoid deadlock and lock recursion. */
			if (pmap > locked_pmap)
				PMAP_LOCK(pmap);
			else if (pmap != locked_pmap && !PMAP_TRYLOCK(pmap)) {
				pmap = NULL;
				TAILQ_INSERT_TAIL(&newtail, pc, pc_lru);
				continue;
			}
			pmap_switch(pmap);
		}

		/*
		 * Destroy every non-wired, 8 KB page mapping in the chunk.
		 */
		freed = 0;
		for (field = 0; field < _NPCM; field++) {
			for (inuse = ~pc->pc_map[field] & pc_freemask[field];
			    inuse != 0; inuse &= ~(1UL << bit)) {
				bit = ffs64(inuse) - 1;
				idx = field * sizeof(inuse) * NBBY + bit;
				pv = &pc->pc_pventry[idx];
				va = pv->pv_va;
				pte = pmap_find_vhpt(va);
				KASSERTMSG(pte != NULL, "pte");
				if (pmap_wired(pte))
					continue;
				pmap_remove_vhpt(va);
				pmap_invalidate_page(va);
				m = PHYS_TO_VM_PAGE(pmap_ppn(pte));
				if (pmap_dirty(pte))
					vm_page_dirty(m);
				pmap_free_pte(pte, va);
				TAILQ_REMOVE(&m->mdpage.pv_list, pv, pv_list);
				/* XXX
				if (TAILQ_EMPTY(&m->mdpage.pv_list))
					//vm_page_aflag_clear(m, PGA_WRITEABLE);
					m->flags |= PG_RDONLY;
				*/
				pc->pc_map[field] |= 1UL << bit;
				freed++;
			}
		}
		if (freed == 0) {
			TAILQ_INSERT_TAIL(&newtail, pc, pc_lru);
			continue;
		}
		/* Every freed mapping is for a 8 KB page. */
		pmap->pm_stats.resident_count -= freed;
		PV_STAT(pv_entry_frees += freed);
		PV_STAT(pv_entry_spare += freed);
		pv_entry_count -= freed;
		TAILQ_REMOVE(&pmap->pm_pvchunk, pc, pc_list);
		for (field = 0; field < _NPCM; field++)
			if (pc->pc_map[field] != pc_freemask[field]) {
				TAILQ_INSERT_HEAD(&pmap->pm_pvchunk, pc,
				    pc_list);
				TAILQ_INSERT_TAIL(&newtail, pc, pc_lru);

				/*
				 * One freed pv entry in locked_pmap is
				 * sufficient.
				 */
				if (pmap == locked_pmap)
					goto out;
				break;
			}
		if (field == _NPCM) {
			PV_STAT(pv_entry_spare -= _NPCPV);
			PV_STAT(pc_chunk_count--);
			PV_STAT(pc_chunk_frees++);
			/* Entire chunk is free; return it. */
			m_pc = PHYS_TO_VM_PAGE(IA64_RR_MASK((vaddr_t)pc));
			break;
		}
	}
out:
	TAILQ_CONCAT(&pv_chunks, &newtail, pc_lru);
	if (pmap != NULL) {
		if (pmap != locked_pmap) {
			pmap_switch(locked_pmap);
			PMAP_UNLOCK(pmap);
		}
	}
	return (m_pc);
}

/*
 * free the pv_entry back to the free list
 */
static void
free_pv_entry(pmap_t pmap, pv_entry_t pv)
{
	struct pv_chunk *pc;
	int bit, field, idx;

	KASSERT(rw_write_held(&pvh_global_lock));
	PMAP_LOCK_ASSERT(pmap);
	PV_STAT(pv_entry_frees++);
	PV_STAT(pv_entry_spare++);
	pv_entry_count--;
	pc = pv_to_chunk(pv);
	idx = pv - &pc->pc_pventry[0];
	field = idx / (sizeof(u_long) * NBBY);
	bit = idx % (sizeof(u_long) * NBBY);
	pc->pc_map[field] |= 1ul << bit;
	for (idx = 0; idx < _NPCM; idx++)
		if (pc->pc_map[idx] != pc_freemask[idx]) {
			/*
			 * 98% of the time, pc is already at the head of the
			 * list.  If it isn't already, move it to the head.
			 */
			if (__predict_false(TAILQ_FIRST(&pmap->pm_pvchunk) !=
			    pc)) {
				TAILQ_REMOVE(&pmap->pm_pvchunk, pc, pc_list);
				TAILQ_INSERT_HEAD(&pmap->pm_pvchunk, pc,
				    pc_list);
			}
			return;
		}
	TAILQ_REMOVE(&pmap->pm_pvchunk, pc, pc_list);
	free_pv_chunk(pc);
}

static void
free_pv_chunk(struct pv_chunk *pc)
{
	struct vm_page *m;

 	TAILQ_REMOVE(&pv_chunks, pc, pc_lru);
	PV_STAT(pv_entry_spare -= _NPCPV);
	PV_STAT(pc_chunk_count--);
	PV_STAT(pc_chunk_frees++);
	/* entire chunk is free, return it */
	m = PHYS_TO_VM_PAGE(IA64_RR_MASK((vaddr_t)pc));
#if 0
	/*  XXX freebsd these move pages around in queue */
	vm_page_unwire(m, 0); // releases one wiring and moves page back active/inactive queue
	vm_page_free(m);      // moves page to "free" queue & disassociate with object

	/* XXX might to need locks/other checks here, uvm_unwire, pmap_kremove... */
	uvm_pagefree(m);
#endif	
	vm_page_free1(m);
}

/*
 * get a new pv_entry, allocating a block from the system
 * when needed.
 */
static pv_entry_t
get_pv_entry(pmap_t pmap, bool try)
{
	struct pv_chunk *pc;
	pv_entry_t pv;
	struct vm_page *m;
	int bit, field, idx;

	KASSERT(rw_write_held(&pvh_global_lock));
	PMAP_LOCK_ASSERT(pmap);
	PV_STAT(pv_entry_allocs++);
	pv_entry_count++;
retry:
	pc = TAILQ_FIRST(&pmap->pm_pvchunk);
	if (pc != NULL) {
		for (field = 0; field < _NPCM; field++) {
			if (pc->pc_map[field]) {
				bit = ffs64(pc->pc_map[field]) - 1;
				break;
			}
		}
		if (field < _NPCM) {
			idx = field * sizeof(pc->pc_map[field]) * NBBY + bit;
			pv = &pc->pc_pventry[idx];
			pc->pc_map[field] &= ~(1ul << bit);
			/* If this was the last item, move it to tail */
			for (field = 0; field < _NPCM; field++)
				if (pc->pc_map[field] != 0) {
					PV_STAT(pv_entry_spare--);
					return (pv);	/* not full, return */
				}
			TAILQ_REMOVE(&pmap->pm_pvchunk, pc, pc_list);
			TAILQ_INSERT_TAIL(&pmap->pm_pvchunk, pc, pc_list);
			PV_STAT(pv_entry_spare--);
			return (pv);
		}
	}

	/* No free items, allocate another chunk */
	m = vm_page_alloc1();
	
	if (m == NULL) {
		if (try) {
			pv_entry_count--;
			PV_STAT(pc_chunk_tryfail++);
			return (NULL);
		}
		m = pmap_pv_reclaim(pmap);
		if (m == NULL)
			goto retry;
	}
	
	PV_STAT(pc_chunk_count++);
	PV_STAT(pc_chunk_allocs++);
	pc = (struct pv_chunk *)IA64_PHYS_TO_RR7(VM_PAGE_TO_PHYS(m));
	pc->pc_pmap = pmap;
	pc->pc_map[0] = pc_freemask[0] & ~1ul;	/* preallocated bit 0 */
	for (field = 1; field < _NPCM; field++)
		pc->pc_map[field] = pc_freemask[field];
	TAILQ_INSERT_TAIL(&pv_chunks, pc, pc_lru);
	pv = &pc->pc_pventry[0];
	TAILQ_INSERT_HEAD(&pmap->pm_pvchunk, pc, pc_list);
	PV_STAT(pv_entry_spare += _NPCPV - 1);
	return (pv);
}

/*
 * Conditionally create a pv entry.
 */
#if 0
static bool
pmap_try_insert_pv_entry(pmap_t pmap, vaddr_t va, struct vm_page *m)
{
	pv_entry_t pv;

	PMAP_LOCK_ASSERT(pmap);
	KASSERT(rw_write_held(&pvh_global_lock));
	if ((pv = get_pv_entry(pmap, true)) != NULL) {
		pv->pv_va = va;
		TAILQ_INSERT_TAIL(&m->mdpage.pv_list, pv, pv_list);
		return (true);
	} else
		return (false);
}
#endif

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

	mutex_spin_enter(&bckt->mutex);
	pte->chain = bckt->chain;
	ia64_mf();
	bckt->chain = pte_pa;

	pmap_vhpt_inserts++;
	bckt->length++;
	mutex_spin_exit(&bckt->mutex);
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
	mutex_spin_enter(&bckt->mutex);
	chain = bckt->chain;
	pte = (struct ia64_lpte *)IA64_PHYS_TO_RR7(chain);
	while (chain != 0 && pte->tag != tag) {
		lpte = pte;
		chain = pte->chain;
		pte = (struct ia64_lpte *)IA64_PHYS_TO_RR7(chain);
	}
	if (chain == 0) {
		mutex_spin_exit(&bckt->mutex);
		return (ENOENT);
	}

	/* Snip this pv_entry out of the collision chain. */
	if (lpte == NULL)
		bckt->chain = pte->chain;
	else
		lpte->chain = pte->chain;
	ia64_mf();

	bckt->length--;

	mutex_spin_exit(&bckt->mutex);
	return (0);
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

	mutex_spin_enter(&bckt->mutex);
	chain = bckt->chain;
	pte = (struct ia64_lpte *)IA64_PHYS_TO_RR7(chain);
	while (chain != 0 && pte->tag != tag) {
		chain = pte->chain;
		pte = (struct ia64_lpte *)IA64_PHYS_TO_RR7(chain);
	}

	mutex_spin_exit(&bckt->mutex);
	return ((chain != 0) ? pte : NULL);
}

/*
 * Remove an entry from the list of managed mappings.
 */
static int
pmap_remove_entry(pmap_t pmap, struct vm_page *m, vaddr_t va, pv_entry_t pv)
{

	KASSERT(rw_write_held(&pvh_global_lock));
	if (!pv) {
		TAILQ_FOREACH(pv, &m->mdpage.pv_list, pv_list) {
			if (pmap == PV_PMAP(pv) && va == pv->pv_va) 
				break;
		}
	}

	if (pv) {
		TAILQ_REMOVE(&m->mdpage.pv_list, pv, pv_list);
		/* XXX
		if (TAILQ_FIRST(&m->mdpage.pv_list) == NULL)
			//vm_page_aflag_clear(m, PGA_WRITEABLE);
			m->flags |= PG_RDONLY;
		*/
		free_pv_entry(pmap, pv);
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
pmap_insert_entry(pmap_t pmap, vaddr_t va, struct vm_page *m)
{
	pv_entry_t pv;

	KASSERT(rw_write_held(&pvh_global_lock));
	pv = get_pv_entry(pmap, false);
	pv->pv_va = va;
	TAILQ_INSERT_TAIL(&m->mdpage.pv_list, pv, pv_list);
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
	struct ia64_lpte **dir1;
	struct ia64_lpte *leaf;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(va=%p)", va, 0, 0, 0);

	KASSERTMSG((va >> 61) == 5, "kernel mapping 0x%lx not in region 5", va);

	KASSERTMSG(va < kernel_vm_end, "kernel mapping 0x%lx out of range", va);

	dir1 = ia64_kptdir[KPTE_DIR0_INDEX(va)];
	leaf = dir1[KPTE_DIR1_INDEX(va)];

	UVMHIST_LOG(maphist, "(kpte_dir0=%#lx, kpte_dir1=%#lx, kpte_pte=%#lx)",
		    KPTE_DIR0_INDEX(va), KPTE_DIR1_INDEX(va), KPTE_PTE_INDEX(va), 0);
	UVMHIST_LOG(maphist, "(dir1=%p, leaf=%p ret=%p)",
		    dir1, leaf, &leaf[KPTE_PTE_INDEX(va)], 0);
	
	return (&leaf[KPTE_PTE_INDEX(va)]);
}

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
		pte = pool_cache_get(pte_pool_cache, PR_NOWAIT);
		if (pte != NULL) {
			memset((void *)pte, 0, sizeof(struct ia64_lpte));
			pte->tag = 1UL << 63;
		}
	}

	return (pte);
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
		pool_cache_put(pte_pool_cache, pte);
	else
		pmap_clear_present(pte);
}

static PMAP_INLINE void
pmap_pte_prot(pmap_t pm, struct ia64_lpte *pte, vm_prot_t prot)
{
	static long prot2ar[4] = {
		PTE_AR_R,		/* VM_PROT_NONE */
		PTE_AR_RW,		/* VM_PROT_WRITE */
		PTE_AR_RX|PTE_ED,	/* VM_PROT_EXECUTE */
		PTE_AR_RWX|PTE_ED	/* VM_PROT_WRITE|VM_PROT_EXECUTE */
	};

	pte->pte &= ~(PTE_PROT_MASK | PTE_PL_MASK | PTE_AR_MASK | PTE_ED);
	pte->pte |= (uint64_t)(prot & VM_PROT_ALL) << 56;
	pte->pte |= (prot == VM_PROT_NONE || pm == kernel_pmap)
	    ? PTE_PL_KERN : PTE_PL_USER;
	pte->pte |= prot2ar[(prot & VM_PROT_ALL) >> 1];
}

static PMAP_INLINE void
pmap_pte_attr(struct ia64_lpte *pte, vm_memattr_t ma)
{
	pte->pte &= ~PTE_MA_MASK;
	pte->pte |= (ma & PTE_MA_MASK);
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
	pte->pte &= PTE_PROT_MASK | PTE_MA_MASK | PTE_PL_MASK |
	    PTE_AR_MASK | PTE_ED;
	pte->pte |= PTE_PRESENT;
	pte->pte |= (managed) ? PTE_MANAGED : (PTE_DIRTY | PTE_ACCESSED);
	pte->pte |= (wired) ? PTE_WIRED : 0;
	pte->pte |= pa & PTE_PPN_MASK;

	pte->itir = PAGE_SHIFT << 2;

	ia64_mf();

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
	struct vm_page *m;

	/*
	 * First remove from the VHPT.
	 */
	error = pmap_remove_vhpt(va);
	KASSERTMSG(error == 0, "%s: pmap_remove_vhpt returned %d",__func__, error);
	
	pmap_invalidate_page(va);

	if (pmap_wired(pte))
		pmap->pm_stats.wired_count -= 1;

	pmap->pm_stats.resident_count -= 1;
	if (pmap_managed(pte)) {
		m = PHYS_TO_VM_PAGE(pmap_ppn(pte));
		if (pmap_dirty(pte))
			vm_page_dirty(m);

		error = pmap_remove_entry(pmap, m, va, pv);
	}
	if (freepte)
		pmap_free_pte(pte, va);

	return (error);
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
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	
	pmap_pool_cache = pool_cache_init(sizeof(struct pmap), 0, 0, 0,
					  "pmap_pool_cache", NULL, IPL_VM,
					  NULL, NULL, NULL);
	if (pmap_pool_cache == NULL)
		panic("%s cannot allocate pmap pool", __func__);
	
	pte_pool_cache = pool_cache_init(sizeof(struct ia64_lpte), 0, 0, 0,
					 "pte_pool_cache", NULL, IPL_VM,
					 NULL, NULL, NULL);
	if (pte_pool_cache == NULL)
		panic("%s cannot allocate pte pool", __func__);

	
	pmap_initialized = true;

#if DEBUG
	if (0) pmap_testout();
#endif	
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
	int npgs;
	uvm_physseg_t upm;
	vaddr_t va;
	paddr_t pa;

	size = round_page(size);
	npgs = atop(size);

	for (upm = uvm_physseg_get_first();
	     uvm_physseg_valid_p(upm);
	     upm = uvm_physseg_get_next(upm)) {
		if (uvm.page_init_done == true)
			panic("pmap_steal_memory: called _after_ bootstrap");

		if (uvm_physseg_get_avail_start(upm) != uvm_physseg_get_start(upm) ||
		    uvm_physseg_get_avail_start(upm) >= uvm_physseg_get_avail_end(upm))
			continue;

		if ((uvm_physseg_get_avail_end(upm) - uvm_physseg_get_avail_start(upm))
		    < npgs)
			continue;

		/*
		 * There are enough pages here; steal them!
		 */
		pa = ptoa(uvm_physseg_get_start(upm));
		uvm_physseg_unplug(atop(pa), npgs);

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
	int npgs;
	uvm_physseg_t upm;
	vaddr_t va;
	paddr_t pa = 0;
	paddr_t vhpt_start = 0, start1, start2, end1, end2;

	size = round_page(size);
	npgs = atop(size);

	for (upm = uvm_physseg_get_first();
	     uvm_physseg_valid_p(upm);
	     upm = uvm_physseg_get_next(upm)) {
		if (uvm.page_init_done == true)
			panic("pmap_vhpt_steal_memory: called _after_ bootstrap");

		if (uvm_physseg_get_avail_start(upm) != uvm_physseg_get_start(upm) || /* XXX: ??? */
		    uvm_physseg_get_avail_start(upm) >= uvm_physseg_get_avail_end(upm))
			continue;
		
		/* Break off a VHPT sized, aligned chunk off this segment. */

		start1 = uvm_physseg_get_avail_start(upm);

		/* Align requested start address on requested size boundary */
		end1 = vhpt_start = roundup(start1, npgs);

		start2 = vhpt_start + npgs;
		end2 = uvm_physseg_get_avail_end(upm);

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
		    uvm_physseg_get_first() == uvm_physseg_get_last() /* single segment */) {
#ifdef DEBUG
			printf("pmap_vhpt_steal_memory: out of memory!");
#endif
			return -1;
		}

		/* Remove this segment from the list. */
		if (uvm_physseg_unplug(uvm_physseg_get_start(upm),
				       uvm_physseg_get_end(upm) - uvm_physseg_get_start(upm)) == false) {
			panic("%s: uvm_physseg_unplug(%"PRIxPADDR", %"PRIxPADDR") failed\n",
			      __func__, uvm_physseg_get_start(upm),
			      uvm_physseg_get_end(upm) - uvm_physseg_get_start(upm));
		}

		/* Case 2: Perfect fit - skip segment reload. */

		if (start1 == end1 && start2 == end2) break;

		/* Case 3: Left unfit - reload it.
		 */

		if (start1 != end1)
			uvm_page_physload(start1, end1, start1, end1,
					  VM_FREELIST_DEFAULT);

		/* Case 4: Right unfit - reload it. */

		if (start2 != end2)
			uvm_page_physload(start2, end2, start2, end2,
					  VM_FREELIST_DEFAULT);

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

	pmap = pool_cache_get(pmap_pool_cache, PR_WAITOK);

	if (pmap == NULL)
		panic("%s no pool", __func__);
	
	PMAP_LOCK_INIT(pmap);

	for (i = 0; i < IA64_VM_MINKERN_REGION; i++)
		pmap->pm_rid[i] = pmap_allocate_rid();

	TAILQ_INIT(&pmap->pm_pvchunk);

	pmap->pm_stats.resident_count = 0;
	pmap->pm_stats.wired_count = 0;

	pmap->pm_refcount = 1;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(pm=%p)", pmap, 0, 0, 0);	

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

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(pm=%p)", pmap, 0, 0, 0);
	
	membar_exit();
	if (atomic_dec_64_nv(&pmap->pm_refcount) > 0)
		return;
	membar_enter();

	KASSERT(pmap->pm_stats.resident_count == 0);
	KASSERT(pmap->pm_stats.wired_count == 0);

	KASSERT(TAILQ_EMPTY(&pmap->pm_pvchunk)); /* XXX hmmm */
	KASSERT(!PMAP_LOCKED(pmap)); /* XXX hmmm */
	/*PMAP_LOCK(pmap); */  /* XXX overkill */

	for (i = 0; i < IA64_VM_MINKERN_REGION; i++)
		if (pmap->pm_rid[i])
			pmap_free_rid(pmap->pm_rid[i]);

	/*PMAP_UNLOCK(pmap);*/ /* XXX hmm */
	PMAP_LOCK_DESTROY(pmap);
	
	pool_cache_put(pmap_pool_cache, pmap);
}

/*
 * pmap_reference:		[ INTERFACE ]
 *
 *	Add a reference to the specified pmap.
 */
void
pmap_reference(pmap_t pmap)
{
	atomic_inc_64(&pmap->pm_refcount);
}

/*
 * pmap_resident_count:		[ INTERFACE ]
 *
 *	Query the ``resident pages'' statistic for pmap.
 */
long
pmap_resident_count(pmap_t pmap)
{
	return (pmap->pm_stats.resident_count);
}

/*
 * pmap_wired_count:		[ INTERFACE ]
 *
 *	Query the ``wired pages'' statistic for pmap.
 *
 */
long
pmap_wired_count(pmap_t pmap)
{
	return (pmap->pm_stats.wired_count);
}

/*
 * pmap_growkernel:		[ INTERFACE ]
 *
 */
vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	struct ia64_lpte **dir1;
	struct ia64_lpte *leaf;
	struct vm_page *pg;
#if 0
	struct vm_page *nkpg;
	paddr_t pa;
	vaddr_t va;
#endif
	
	/* XXX this function may still need serious fixin' */
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(va=%#lx, nkpt=%ld, kvm_end=%#lx before)", maxkvaddr, nkpt, kernel_vm_end, 0);

	vaddr_t addr = maxkvaddr;

	/* XXX use uvm_pageboot_alloc if not done? */
	if (!uvm.page_init_done)
		panic("uvm_page init not done");

	while (kernel_vm_end <= addr) {
		if (nkpt == PAGE_SIZE/8 + PAGE_SIZE*PAGE_SIZE/64)
			panic("%s: out of kernel address space", __func__);
		dir1 = ia64_kptdir[KPTE_DIR0_INDEX(kernel_vm_end)];

		if (dir1 == NULL) {
#if 0
			/* FreeBSD does it this way... */
			nkpg = vm_page_alloc(NULL, nkpt++, VM_ALLOC_NOOBJ|VM_ALLOC_INTERRUPT|VM_ALLOC_WIRED);
			pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE|UVM_PGA_ZERO);
#endif			
			pg = vm_page_alloc1();
			if (!pg)
				panic("%s: cannot add dir1 page", __func__);
			nkpt++;

#if 0
			dir1 = (struct ia64_lpte **)pmap_page_to_va(nkpg);
			bzero(dir1, PAGE_SIZE);
#endif			
			dir1 = (struct ia64_lpte **)pmap_page_to_va(pg);

			ia64_kptdir[KPTE_DIR0_INDEX(kernel_vm_end)] = dir1;
		}

#if 0
		nkpg = vm_page_alloc(NULL, nkpt++, VM_ALLOC_NOOBJ|VM_ALLOC_INTERRUPT|VM_ALLOC_WIRED);
		pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE|UVM_PGA_ZERO);
#endif		
		pg = vm_page_alloc1();
		if (!pg)
			panic("%s: cannot add PTE page", __func__);
		nkpt++;
#if 0		
		leaf = (struct ia64_lpte *)pmap_page_to_va(nkpg);
		bzero(leaf, PAGE_SIZE);
#endif		
		leaf = (struct ia64_lpte *)pmap_page_to_va(pg);

		dir1[KPTE_DIR1_INDEX(kernel_vm_end)] = leaf;

		kernel_vm_end += PAGE_SIZE * NKPTEPG;
	}

	UVMHIST_LOG(maphist, "(va=%#lx, nkpt=%ld, kvm_end=%#lx after)", maxkvaddr, nkpt, kernel_vm_end, 0);

	/* XXX fix */
	return kernel_vm_end;
}

/*
 * pmap_enter:			[ INTERFACE ]
 *
 *	Create a mapping in physical map pmap for the physical
 *	address pa at the virtual address va with protection speci-
 *	fied by bits in prot.
 */
int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	pmap_t oldpmap;
	struct vm_page *m;
	vaddr_t opa;
	struct ia64_lpte origpte;
	struct ia64_lpte *pte;
	bool icache_inval, managed, wired, canfail;
	/* vm_memattr_t ma; */

	/* XXX this needs work */
	
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(pm=%p, va=%#lx, pa=%p, prot=%#x)", pmap, va, pa, prot);

	/* wired = (flags & PMAP_ENTER_WIRED) != 0; */
	wired = (flags & PMAP_WIRED) != 0;
	canfail = (flags & PMAP_CANFAIL) != 0;

	/* ma = pmap_flags_to_memattr(flags); */
	
	rw_enter(&pvh_global_lock, RW_WRITER);
	PMAP_LOCK(pmap);
	oldpmap = pmap_switch(pmap);

	va &= ~PAGE_MASK;
 	KASSERTMSG(va <= VM_MAX_KERNEL_ADDRESS, "%s: toobig", __func__);

	/* XXX
	if ((m->oflags & VPO_UNMANAGED) == 0 && !vm_page_xbusied(m))
		VM_OBJECT_ASSERT_LOCKED(m->object);
	*/

	/*
	 * Find (or create) a pte for the given mapping.
	 */
	pte = pmap_find_pte(va);

	if (pte == NULL) {
		pmap_switch(oldpmap);
		PMAP_UNLOCK(pmap);
		rw_exit(&pvh_global_lock);

		if (canfail)
			return (ENOMEM);
		else
			panic("%s: no pte available", __func__);
	}

	origpte = *pte;
	if (!pmap_present(pte)) {
		opa = ~0UL;
		pmap_enter_vhpt(pte, va);
	} else
		opa = pmap_ppn(pte);

	managed = false;
	/* XXX hmm
	pa = VM_PAGE_TO_PHYS(m);
	*/
	
	m = PHYS_TO_VM_PAGE(pa);
	if (m == NULL) {
		/* implies page not managed? */
		panic("%s: new page needed", __func__);
	}
	icache_inval = (prot & VM_PROT_EXECUTE) ? true : false;

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
		 * so we go ahead and sense modify status. Otherwise,
		 * we can avoid I-cache invalidation if the page
		 * already allowed execution.
		 */
		if (managed && pmap_dirty(&origpte))
			vm_page_dirty(m);
		else if (pmap_exec(&origpte))
			icache_inval = false;

		pmap_invalidate_page(va);
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
	if (vm_page_is_managed(m)) {
#if 0		
		KASSERTMSG(va < kmi.clean_sva || va >= kmi.clean_eva,
			   ("pmap_enter: managed mapping within the clean submap"));
#endif		
		pmap_insert_entry(pmap, va, m);
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
	pmap_pte_attr(pte, m->mdpage.memattr);
	pmap_set_pte(pte, va, pa, wired, managed);

	/* Invalidate the I-cache when needed. */
	if (icache_inval)
		ia64_sync_icache(va, PAGE_SIZE);

	/* XXX
	if ((prot & VM_PROT_WRITE) != 0 && managed)
		//vm_page_aflag_set(m, PGA_WRITEABLE);
		m->flags &= ~PG_RDONLY;
	*/
	rw_exit(&pvh_global_lock);
	pmap_switch(oldpmap);
	PMAP_UNLOCK(pmap);
	return (0);
}

/*
 * pmap_remove:			[ INTERFACE ]
 *
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the page size.
 *
 *	Sparsely used ranges are inefficiently removed.  The VHPT is
 *	probed for every page within the range.  XXX
 */
void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	pmap_t oldpmap;
	vaddr_t va;
	struct ia64_lpte *pte;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(pm=%p, sva=%#lx, eva=%#lx)", pmap, sva, eva, 0);

	/*
	 * Perform an unsynchronized read.  This is, however, safe.
	 */
	if (pmap->pm_stats.resident_count == 0)
		return;

	rw_enter(&pvh_global_lock, RW_WRITER);
	PMAP_LOCK(pmap);
	oldpmap = pmap_switch(pmap);
	for (va = sva; va < eva; va += PAGE_SIZE) {
		pte = pmap_find_vhpt(va);
		if (pte != NULL)
			pmap_remove_pte(pmap, pte, va, 0, 1);
	}

	rw_exit(&pvh_global_lock);
	pmap_switch(oldpmap);
	PMAP_UNLOCK(pmap);
}

/*
 * pmap_remove_all:		[ INTERFACE ]
 *
 *	This function is a hint to the pmap implementation that all
 *	entries in pmap will be removed before any more entries are
 *	entered.
 */
bool
pmap_remove_all(pmap_t pmap)
{
	/* XXX do nothing */
	return false;
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
	pmap_t oldpmap;
	struct ia64_lpte *pte;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(pm=%p, sva=%#lx, eva=%#lx, prot=%#x)",
		    pmap, sva, eva, prot);
	UVMHIST_LOG(maphist, "(VM_PROT_READ=%u, VM_PROT_WRITE=%u, VM_PROT_EXECUTE=%u)",
		    (prot & VM_PROT_READ) != 0, (prot & VM_PROT_WRITE) != 0,
		    (prot & VM_PROT_EXECUTE) != 0, 0);

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}

	if ((prot & (VM_PROT_WRITE|VM_PROT_EXECUTE)) ==
	    (VM_PROT_WRITE|VM_PROT_EXECUTE))
		return;

	if ((sva & PAGE_MASK) || (eva & PAGE_MASK))
		panic("pmap_protect: unaligned addresses");
	sva = trunc_page(sva);
	eva = round_page(eva) - 1;
	
	PMAP_LOCK(pmap);
	oldpmap = pmap_switch(pmap);
	for ( ; sva < eva; sva += PAGE_SIZE) {
		/* If page is invalid, skip this page */
		pte = pmap_find_vhpt(sva);
		if (pte == NULL)
			continue;

		/* If there's no change, skip it too */
		if (pmap_prot(pte) == prot)
			continue;

		/* wired pages unaffected by prot changes */
		if (pmap_wired(pte))
			continue;
		
		if ((prot & VM_PROT_WRITE) == 0 &&
		    pmap_managed(pte) && pmap_dirty(pte)) {
			paddr_t pa = pmap_ppn(pte);
			struct vm_page *m = PHYS_TO_VM_PAGE(pa);

			vm_page_dirty(m);
			pmap_clear_dirty(pte);
		}

		if (prot & VM_PROT_EXECUTE)
			ia64_sync_icache(sva, PAGE_SIZE);

		pmap_pte_prot(pmap, pte, prot);
		pmap_invalidate_page(sva);
	}
	pmap_switch(oldpmap);
	PMAP_UNLOCK(pmap);
}

/*
 * pmap_unwire:			[ INTERFACE ]
 *
 *	Clear the wired attribute for a map/virtual-address pair.
 *
 *	The wired attribute of the page table entry is not a hardware feature,
 *	so there is no need to invalidate any TLB entries.
 *
 *	The mapping must already exist in the pmap.
 */
void
pmap_unwire(pmap_t pmap, vaddr_t va)
{
	pmap_t oldpmap;
	struct ia64_lpte *pte;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(pm=%p, va=%#x)", pmap, va, 0, 0);

	PMAP_LOCK(pmap);
	oldpmap = pmap_switch(pmap);

	pte = pmap_find_vhpt(va);

	/* XXX panic if no pte or not wired? */
	if (pte == NULL)
		panic("pmap_unwire: %lx not found in vhpt", va);
	
	if (!pmap_wired(pte))
		panic("pmap_unwire: pte %p isn't wired", pte);
	
	pmap->pm_stats.wired_count--;
	pmap_clear_wired(pte);
	
	pmap_switch(oldpmap);
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

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(pm=%p, va=%#lx, pap=%#lx)", pmap, va, pap, 0);

	pa = 0;

	PMAP_LOCK(pmap);
	oldpmap = pmap_switch(pmap);
	pte = pmap_find_vhpt(va);
	if (pte != NULL && pmap_present(pte))
		pa = pmap_ppn(pte);
	pmap_switch(oldpmap);
	PMAP_UNLOCK(pmap);

	if (pa && (pap != NULL))
		*pap = pa;

	UVMHIST_LOG(maphist, "(pa=%#lx)", pa, 0, 0, 0);
	
	return (pa != 0);
}
/*
 * Extract the physical page address associated with a kernel
 * virtual address.
 */
paddr_t
pmap_kextract(vaddr_t va)
{
	struct ia64_lpte *pte;
	/*uint64_t *pbvm_pgtbl;*/
	paddr_t pa;
	/*u_int idx;*/

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(va=%#lx)", va, 0, 0, 0);
	
	KASSERTMSG(va >= VM_MAXUSER_ADDRESS, "Must be kernel VA");

	/* Regions 6 and 7 are direct mapped. */
	if (va >= IA64_RR_BASE(6)) {
		pa = IA64_RR_MASK(va);
		goto out;
	}

	/* Region 5 is our KVA. Bail out if the VA is beyond our limits. */
	if (va >= kernel_vm_end)
		goto err_out;
	if (va >= VM_INIT_KERNEL_ADDRESS) {
		pte = pmap_find_kpte(va);
		pa = pmap_present(pte) ? pmap_ppn(pte) | (va & PAGE_MASK) : 0;
		goto out;
	}
#if 0 /* XXX fix */
	/* The PBVM page table. */
	if (va >= IA64_PBVM_PGTBL + bootinfo->bi_pbvm_pgtblsz)
		goto err_out;
	if (va >= IA64_PBVM_PGTBL) {
		pa = (va - IA64_PBVM_PGTBL) + bootinfo->bi_pbvm_pgtbl;
		goto out;
	}

	/* The PBVM itself. */
	if (va >= IA64_PBVM_BASE) {
		pbvm_pgtbl = (void *)IA64_PBVM_PGTBL;
		idx = (va - IA64_PBVM_BASE) >> IA64_PBVM_PAGE_SHIFT;
		if (idx >= (bootinfo->bi_pbvm_pgtblsz >> 3))
			goto err_out;
		if ((pbvm_pgtbl[idx] & PTE_PRESENT) == 0)
			goto err_out;
		pa = (pbvm_pgtbl[idx] & PTE_PPN_MASK) +
		    (va & IA64_PBVM_PAGE_MASK);
		goto out;
	}
#endif

 err_out:
	KASSERT(1);
	printf("XXX: %s: va=%#lx is invalid\n", __func__, va);
	pa = 0;
	/* FALLTHROUGH */

 out:
	UVMHIST_LOG(maphist, "(pa=%#lx)", pa, 0, 0, 0);
	return (pa);
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
	struct ia64_lpte *pte;
	vm_memattr_t attr;
	const bool managed = false;  /* don't gather ref/mod info */
	const bool wired = true;     /* pmap_kenter_pa always wired */
	
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(va=%#lx, pa=%#lx, prot=%p, flags=%p)", va, pa, prot, flags);

	KASSERT(va >= VM_MIN_KERNEL_ADDRESS && va < VM_MAX_KERNEL_ADDRESS);
	
	attr = pmap_flags_to_memattr(flags);

	pte = pmap_find_kpte(va);
	if (pmap_present(pte))
		pmap_invalidate_page(va);
	else
		pmap_enter_vhpt(pte, va);

	pmap_pte_prot(kernel_pmap, pte, VM_PROT_ALL);
	pmap_pte_attr(pte, attr);
	pmap_set_pte(pte, va, pa, wired, managed);
}

/*
 * pmap_kremove:		[ INTERFACE ]
 *
 *   Remove all mappings starting at virtual address va for size
 *     bytes from the kernel physical map.  All mappings that are
 *     removed must be the ``unmanaged'' type created with
 *     pmap_kenter_pa().  The implementation may assert this.
 */
void
pmap_kremove(vaddr_t va, vsize_t size)
{
	struct ia64_lpte *pte;
	vaddr_t eva = va + size;
	
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(va=%#lx)", va, 0, 0, 0);

	while (va < eva) {
		pte = pmap_find_kpte(va);
		if (pmap_present(pte)) {
			KASSERT(pmap_managed(pte) != 0);
			pmap_remove_vhpt(va);
			pmap_invalidate_page(va);
			pmap_clear_present(pte);
		}
		va += PAGE_SIZE;
	}
}

/*
 * pmap_copy:			[ INTERFACE ]
 *
 *	This function copies the mappings starting at src_addr in
 *	src_map for len bytes into dst_map starting at dst_addr.
 *
 *	Note that while this function is required to be provided by
 *	a pmap implementation, it is not actually required to do
 *	anything.  pmap_copy() is merely advisory (it is used in
 *	the fork(2) path to ``pre-fault'' the child's address
 *	space).
 */
void
pmap_copy(pmap_t dst_map, pmap_t src_map, vaddr_t dst_addr, vsize_t len,
         vaddr_t src_addr)
{
	/* nothing required */
}

/*
 * pmap_update:			[ INTERFACE ]
 *
 *	If a pmap implementation does not delay virtual-to-physical
 *	mapping updates, pmap_update() has no operation.
 */
void
pmap_update(pmap_t pmap)
{
	/* nothing required */
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
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(lwp=%p)", l, 0, 0, 0);
	KASSERT(l == curlwp);
#if 0	
	/* pmap_switch(vmspace_pmap(td->td_proc->p_vmspace)); */
	pmap_switch(vm_map_pmap(&l->l_proc->p_vmspace->vm_map));
#else
	struct pmap *pmap = l->l_proc->p_vmspace->vm_map.pmap;

	if (pmap == pmap_kernel()) {
 		return;
 	}

	if (l != curlwp) {
    		return;
    	}

	pmap_switch(pmap);
#endif
}

/*
 * pmap_deactivate:		[ INTERFACE ]
 *
 *	Deactivate the physical map used by the process behind lwp
 *	l.  It is generally used in conjunction with
 *	pmap_activate().  Like pmap_activate(), pmap_deactivate()
 *      may not always be called when l is the current lwp.
 *
 */
void
pmap_deactivate(struct lwp *l)
{
	/* XXX ? */
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
	struct vm_page *m;
	vaddr_t va;
	
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(pa=%p)", phys, 0, 0, 0);

	m = PHYS_TO_VM_PAGE(phys);
	KASSERT(m != NULL);

	va = pmap_page_to_va(m);
	KASSERT(trunc_page(va) == va);

	UVMHIST_LOG(maphist, "(pa=%p, va=%p)", phys, va, 0, 0);
	memset((void *)va, 0, PAGE_SIZE);
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
	struct vm_page *md, *ms;
	vaddr_t dst_va, src_va;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(sp=%p, dp=%p)", psrc, pdst, 0, 0);

	md = PHYS_TO_VM_PAGE(pdst);
	ms = PHYS_TO_VM_PAGE(psrc);
	KASSERT(md != NULL && ms != NULL);
	
	dst_va = pmap_page_to_va(md);
	src_va = pmap_page_to_va(ms);
	KASSERT(trunc_page(dst_va) == dst_va && trunc_page(src_va) == src_va);
	
	memcpy((void *)dst_va, (void *)src_va, PAGE_SIZE);
}

/*
 * pmap_page_protect:		[ INTERFACE ]
 *
 *  Lower the permissions for all mappings of the page pg to
 *     prot.  This function is used by the virtual memory system
 *     to implement copy-on-write (called with VM_PROT_READ set in
 *     prot) and to revoke all mappings when cleaning a page
 *     (called with no bits set in prot).  Access permissions must
 *     never be added to a page as a result of this call.
 */
void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	//struct ia64_lpte *pte;
	pmap_t pmap;
	pv_entry_t pv;
	vaddr_t va;
	
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(m=%p, prot=%p)", pg, prot, 0, 0);

	if ((prot & VM_PROT_WRITE) != 0)
		return;
	if (prot & (VM_PROT_READ | VM_PROT_EXECUTE)) {
		/* XXX FreeBSD
		if ((pg->mdpage.aflags & PGA_WRITEABLE) == 0)
			return;
		*/
		if (pg->flags & PG_RDONLY)
			return;
		rw_enter(&pvh_global_lock, RW_WRITER);
		TAILQ_FOREACH(pv, &pg->mdpage.pv_list, pv_list) {
			pmap = PV_PMAP(pv);
			va = pv->pv_va;
			// locking of pmap done in pmap_protect
			pmap_protect(pmap, va, va + PAGE_SIZE, prot);
		}
		/* XXX
		vm_page_aflag_clear(pg, PGA_WRITEABLE);
		*/
		pg->flags |= PG_RDONLY;
		
		rw_exit(&pvh_global_lock);
	} else {
		pmap_remove_all_phys(pg);
	}
}

/*
 * pmap_clear_modify:		[ INTERFACE ]
 *
 *	Clear the modify bits on the specified physical page.
 */
bool
pmap_clear_modify(struct vm_page *pg)
{
	struct ia64_lpte *pte;
	pmap_t oldpmap, pmap;
	pv_entry_t pv;
	bool rv;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(m=%p)", pg, 0, 0, 0);

	KASSERTMSG(vm_page_is_managed(pg), "%s : page %p not managed",
		   __func__, pg);

	rv = false;
	
	//VM_OBJECT_ASSERT_WLOCKED(m->object);
	//KASSERT(!vm_page_xbusied(m),
	//    ("pmap_clear_modify: page %p is exclusive busied", m));

	/*
	 * If the page is not PGA_WRITEABLE, then no PTEs can be modified.
	 * If the object containing the page is locked and the page is not
	 * exclusive busied, then PGA_WRITEABLE cannot be concurrently set.
	 */

#if 0	/* XXX freebsd does this, looks faster but not required */
	if ((m->aflags & PGA_WRITEABLE) == 0)
		return;
	if (pg->flags & PG_RDONLY)
		return (rv);
#endif	
	rw_enter(&pvh_global_lock, RW_WRITER);
	TAILQ_FOREACH(pv, &pg->mdpage.pv_list, pv_list) {
		pmap = PV_PMAP(pv);
		PMAP_LOCK(pmap);
		oldpmap = pmap_switch(pmap);
		pte = pmap_find_vhpt(pv->pv_va);
		KASSERTMSG(pte != NULL, "pte");
		if (pmap_dirty(pte)) {
			pmap_clear_dirty(pte);
			pmap_invalidate_page(pv->pv_va);
			rv = true;
		}
		pmap_switch(oldpmap);
		PMAP_UNLOCK(pmap);
	}

	rw_exit(&pvh_global_lock);

	return (rv);
}

/*
 * pmap_clear_reference:	[ INTERFACE ]
 *
 *	Clear the reference bit on the specified physical page.
 *
 *	returns true or false
 *	indicating whether or not the ``referenced'' attribute was
 *	set on the page before it was cleared
 */
bool
pmap_clear_reference(struct vm_page *pg)
{
	struct ia64_lpte *pte;
	pmap_t oldpmap, pmap;
	pv_entry_t pv;
	bool rv;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(m=%p)", pg, 0, 0, 0);

	KASSERTMSG(vm_page_is_managed(pg), "%s: page %p is not managed",
		   __func__, pg);

	rv = false;
	
	rw_enter(&pvh_global_lock, RW_WRITER);
	TAILQ_FOREACH(pv, &pg->mdpage.pv_list, pv_list) {
		pmap = PV_PMAP(pv);
		PMAP_LOCK(pmap);
		oldpmap = pmap_switch(pmap);
		pte = pmap_find_vhpt(pv->pv_va);
		KASSERTMSG(pte != NULL, "pte");
		if (pmap_accessed(pte)) {
			rv = true;
			pmap_clear_accessed(pte);
			pmap_invalidate_page(pv->pv_va);
		}
		pmap_switch(oldpmap);
		PMAP_UNLOCK(pmap);
	}

	rw_exit(&pvh_global_lock);
	return (rv);	
}

/*
 * pmap_is_modified:		[ INTERFACE ]
 *
 */
bool
pmap_is_modified(struct vm_page *pg)
{
	struct ia64_lpte *pte;
	pmap_t oldpmap, pmap;
	pv_entry_t pv;
	bool rv;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(m=%p)", pg, 0, 0, 0);

	KASSERTMSG(vm_page_is_managed(pg), "%s: page %p is not managed",
		   __func__, pg);
	rv = false;

	/*
	 * If the page is not exclusive busied, then PGA_WRITEABLE cannot be
	 * concurrently set while the object is locked.  Thus, if PGA_WRITEABLE
	 * is clear, no PTEs can be dirty.
	 */
	/* XXX freebsd
	VM_OBJECT_ASSERT_WLOCKED(m->object);
	if (!vm_page_xbusied(m) && (m->aflags & PGA_WRITEABLE) == 0)
		return (rv);
	*/
	rw_enter(&pvh_global_lock, RW_WRITER);
	TAILQ_FOREACH(pv, &pg->mdpage.pv_list, pv_list) {
		pmap = PV_PMAP(pv);
		PMAP_LOCK(pmap);
		oldpmap = pmap_switch(pmap);
		pte = pmap_find_vhpt(pv->pv_va);
		pmap_switch(oldpmap);
		KASSERTMSG(pte != NULL, "pte");

		rv = pmap_dirty(pte) ? true : false;
		PMAP_UNLOCK(pmap);
		if (rv)
			break;
	}

	rw_exit(&pvh_global_lock);
	return (rv);
}

/*
 * pmap_is_referenced:		[ INTERFACE ]
 *
 * 	Test whether or not the ``referenced'' attribute is set on
 *	page pg.
 *
 */
bool
pmap_is_referenced(struct vm_page *pg)
{
	struct ia64_lpte *pte;
	pmap_t oldpmap, pmap;
	pv_entry_t pv;
	bool rv;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(m=%p)", pg, 0, 0, 0);

	KASSERTMSG(vm_page_is_managed(pg), "%s: page %p is not managed",
		__func__, pg);
	
	rv = false;
	rw_enter(&pvh_global_lock, RW_WRITER);
	TAILQ_FOREACH(pv, &pg->mdpage.pv_list, pv_list) {
		pmap = PV_PMAP(pv);
		PMAP_LOCK(pmap);
		oldpmap = pmap_switch(pmap);
		pte = pmap_find_vhpt(pv->pv_va);
		KASSERTMSG(pte != NULL, "pte");		
		rv = pmap_accessed(pte) ? true : false;
		pmap_switch(oldpmap);
		PMAP_UNLOCK(pmap);
		if (rv)
			break;
	}

	rw_exit(&pvh_global_lock);
	return (rv);	
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

pmap_t
pmap_switch(pmap_t pm)
{
	pmap_t prevpm;
	int i;

	critical_enter();

	prevpm = curcpu()->ci_pmap;
	if (prevpm == pm)
		goto out;
	if (pm == NULL) {
		for (i = 0; i < IA64_VM_MINKERN_REGION; i++) {
			ia64_set_rr(IA64_RR_BASE(i),
			    (i << 8)|(PAGE_SHIFT << 2)|1);
		}
	} else {
		for (i = 0; i < IA64_VM_MINKERN_REGION; i++) {
			ia64_set_rr(IA64_RR_BASE(i),
			    (pm->pm_rid[i] << 8)|(PAGE_SHIFT << 2)|1);
		}
	}

	/* XXX */
	ia64_srlz_d();
	curcpu()->ci_pmap = pm;
	ia64_srlz_d();

out:
	critical_exit();
	return (prevpm);
}


/*
 * Synchronize CPU instruction caches of the specified range.
 * Same as freebsd
 *   void pmap_sync_icache(pmap_t pm, vaddr_t va, vsize_t sz)
 */
void
pmap_procwr(struct proc *p, vaddr_t va, vsize_t sz)
{
	pmap_t oldpm;
	struct ia64_lpte *pte;
	vaddr_t lim;
	vsize_t len;

	struct pmap * const pm = p->p_vmspace->vm_map.pmap;
	
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(pm=%p, va=%#lx, sz=%#lx)", pm, va, sz, 0);

	sz += va & 31;
	va &= ~31;
	sz = (sz + 31) & ~31;

	PMAP_LOCK(pm);
	oldpm = pmap_switch(pm);
	while (sz > 0) {
		lim = round_page(va);
		len = MIN(lim - va, sz);
		pte = pmap_find_vhpt(va);
		if (pte != NULL && pmap_present(pte))
			ia64_sync_icache(va, len);
		va += len;
		sz -= len;
	}
	pmap_switch(oldpm);
	PMAP_UNLOCK(pm);
}

/*
 *	Routine:	pmap_remove_all_phys
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
pmap_remove_all_phys(struct vm_page *m)
{
	pmap_t oldpmap;
	pv_entry_t pv;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(m=%p)", m, 0, 0, 0);

	KASSERTMSG(vm_page_is_managed(m), "%s: page %p is not managed",
		   __func__, m);

	rw_enter(&pvh_global_lock, RW_WRITER);
	while ((pv = TAILQ_FIRST(&m->mdpage.pv_list)) != NULL) {
		struct ia64_lpte *pte;
		pmap_t pmap = PV_PMAP(pv);
		vaddr_t va = pv->pv_va;

		PMAP_LOCK(pmap);
		oldpmap = pmap_switch(pmap);
		pte = pmap_find_vhpt(va);
		KASSERTMSG(pte != NULL, "pte");
		if (pmap_ppn(pte) != VM_PAGE_TO_PHYS(m))
			panic("%s: pv_table for %lx is inconsistent",
			      __func__, VM_PAGE_TO_PHYS(m));
		pmap_remove_pte(pmap, pte, va, pv, 1);
		pmap_switch(oldpmap);
		PMAP_UNLOCK(pmap);
	}
	/* XXX freebsd 
	vm_page_aflag_clear(m, PGA_WRITEABLE);
	*/
	m->flags |= PG_RDONLY;

	rw_exit(&pvh_global_lock);
}

/*
 *	vm_page_alloc1:
 *
 *	Allocate and return a memory cell with no associated object.
 */
static struct vm_page
*vm_page_alloc1(void)
{
	struct vm_page *pg;
	
	pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE|UVM_PGA_ZERO);
	if (pg) {
		pg->wire_count = 1;	/* no mappings yet */
		pg->flags &= ~PG_BUSY;	/* never busy */
	}
	return pg;
}

/*
 *	vm_page_free1:
 *
 *	Returns the given page to the free list,
 *	disassociating it with any VM object.
 *
 *	Object and page must be locked prior to entry.
 */
static void
vm_page_free1(struct vm_page *pg)
{
	KASSERT(pg->flags != (PG_CLEAN|PG_FAKE)); /* Freeing invalid page */

	pg->flags |= PG_BUSY;
	pg->wire_count = 0;
	uvm_pagefree(pg);
}

/*
 *	pmap_flag_to_attr
 *
 *	Convert pmap_enter/pmap_kenter_pa flags to memory attributes
 */
static vm_memattr_t
pmap_flags_to_memattr(u_int flags)
{
	u_int cacheflags = flags & PMAP_CACHE_MASK;

#if 0	
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist, "(PMAP_NOCACHE=%u, PMAP_WRITE_COMBINE=%u, "
		    "PMAP_WRITE_BACK=%u, PMAP_NOCACHE_OVR=%u)",
		    (flags & PMAP_NOCACHE) != 0, (flags & PMAP_WRITE_COMBINE) != 0,
		    (flags & PMAP_WRITE_BACK) != 0, (flags & PMAP_NOCACHE_OVR) != 0);
#endif	
	switch (cacheflags) {
	case PMAP_NOCACHE:
		return VM_MEMATTR_UNCACHEABLE;
	case PMAP_WRITE_COMBINE:
		/* XXX implement if possible */
		KASSERT(1);
		return VM_MEMATTR_WRITE_COMBINING;
	case PMAP_WRITE_BACK:
		return VM_MEMATTR_WRITE_BACK;
	case PMAP_NOCACHE_OVR:
	default:
		return VM_MEMATTR_DEFAULT;
	}
}

#ifdef DEBUG
/*
 * Test ref/modify handling.
 */

static void
pmap_testout(void)
{
	vaddr_t va;
	volatile int *loc;
	int val = 0;
	paddr_t pa;
	struct vm_page *pg;
	int ref, mod;
	bool extracted;
	
	/* Allocate a page */
	va = (vaddr_t)uvm_km_alloc(kernel_map, PAGE_SIZE, 0, UVM_KMF_WIRED | UVM_KMF_ZERO);
	KASSERT(va != 0);
	loc = (int*)va;

	extracted = pmap_extract(pmap_kernel(), va, &pa);
	printf("va %p pa %lx extracted %u\n", (void *)(u_long)va, pa, extracted);
	printf("kextract %lx\n", pmap_kextract(va));

	pg = PHYS_TO_VM_PAGE(pa);
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, 0);
	pmap_update(pmap_kernel());

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);

	printf("Entered page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa, ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Check it's properly cleared */
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Checking cleared page: ref %d, mod %d\n",
	       ref, mod);

	/* Reference page */
	val = *loc;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Referenced page: ref %d, mod %d val %x\n",
	       ref, mod, val);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Modify page */
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Check it's properly cleared */
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Checking cleared page: ref %d, mod %d\n",
	       ref, mod);

	/* Modify page */
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Check pmap_protect() */
	pmap_protect(pmap_kernel(), va, va+1, VM_PROT_READ);
	pmap_update(pmap_kernel());
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("pmap_protect(VM_PROT_READ): ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Modify page */
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, 0);
	pmap_update(pmap_kernel());
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Check pmap_protect() */
	pmap_protect(pmap_kernel(), va, va+1, VM_PROT_NONE);
	pmap_update(pmap_kernel());
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("pmap_protect(VM_PROT_READ): ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Modify page */
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, 0);
	pmap_update(pmap_kernel());
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Check pmap_pag_protect() */
	pmap_page_protect(pg, VM_PROT_READ);
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("pmap_protect(): ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);


	/* Modify page */
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, 0);
	pmap_update(pmap_kernel());
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Check pmap_pag_protect() */
	pmap_page_protect(pg, VM_PROT_NONE);
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("pmap_protect(): ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Unmap page */
	pmap_remove(pmap_kernel(), va, va+1);
	pmap_update(pmap_kernel());
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Unmapped page: ref %d, mod %d\n", ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa, ref, mod);

	/* Check it's properly cleared */
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Checking cleared page: ref %d, mod %d\n",
	       ref, mod);

	pmap_remove(pmap_kernel(), va, va+1);
	pmap_update(pmap_kernel());
	//pmap_free_page(pa, cpus_active);
	uvm_km_free(kernel_map, (vaddr_t)va, PAGE_SIZE,
		    UVM_KMF_WIRED | UVM_KMF_ZERO);

	panic("end of testout");
}
#endif
