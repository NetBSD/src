/*	$NetBSD: pmap_motorola.c,v 1.63.2.2 2012/04/17 00:06:36 yamt Exp $        */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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
 * Motorola m68k-family physical map management code.
 *
 * Supports:
 *	68020 with 68851 MMU
 *	68030 with on-chip MMU
 *	68040 with on-chip MMU
 *	68060 with on-chip MMU
 *
 * Notes:
 *	Don't even pay lip service to multiprocessor support.
 *
 *	We assume TLB entries don't have process tags (except for the
 *	supervisor/user distinction) so we only invalidate TLB entries
 *	when changing mappings for the current (or kernel) pmap.  This is
 *	technically not true for the 68851 but we flush the TLB on every
 *	context switch, so it effectively winds up that way.
 *
 *	Bitwise and/or operations are significantly faster than bitfield
 *	references so we use them when accessing STE/PTEs in the pmap_pte_*
 *	macros.  Note also that the two are not always equivalent; e.g.:
 *		(*pte & PG_PROT) [4] != pte->pg_prot [1]
 *	and a couple of routines that deal with protection and wiring take
 *	some shortcuts that assume the and/or definitions.
 */

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

#include "opt_m68k_arch.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap_motorola.c,v 1.63.2.2 2012/04/17 00:06:36 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/cpu.h>
#include <sys/atomic.h>

#include <machine/pte.h>
#include <machine/pcb.h>

#include <uvm/uvm.h>

#include <m68k/cacheops.h>

#ifdef DEBUG
#define PDB_FOLLOW	0x0001
#define PDB_INIT	0x0002
#define PDB_ENTER	0x0004
#define PDB_REMOVE	0x0008
#define PDB_CREATE	0x0010
#define PDB_PTPAGE	0x0020
#define PDB_CACHE	0x0040
#define PDB_BITS	0x0080
#define PDB_COLLECT	0x0100
#define PDB_PROTECT	0x0200
#define PDB_SEGTAB	0x0400
#define PDB_MULTIMAP	0x0800
#define PDB_PARANOIA	0x2000
#define PDB_WIRING	0x4000
#define PDB_PVDUMP	0x8000

int debugmap = 0;
int pmapdebug = PDB_PARANOIA;

#define	PMAP_DPRINTF(l, x)	if (pmapdebug & (l)) printf x
#else /* ! DEBUG */
#define	PMAP_DPRINTF(l, x)	/* nothing */
#endif /* DEBUG */

/*
 * Get STEs and PTEs for user/kernel address space
 */
#if defined(M68040) || defined(M68060)
#define	pmap_ste1(m, v)	\
	(&((m)->pm_stab[(vaddr_t)(v) >> SG4_SHIFT1]))
/* XXX assumes physically contiguous ST pages (if more than one) */
#define pmap_ste2(m, v) \
	(&((m)->pm_stab[(st_entry_t *)(*(u_int *)pmap_ste1(m, v) & SG4_ADDR1) \
			- (m)->pm_stpa + (((v) & SG4_MASK2) >> SG4_SHIFT2)]))
#if defined(M68020) || defined(M68030)
#define	pmap_ste(m, v)	\
	(&((m)->pm_stab[(vaddr_t)(v) \
			>> (mmutype == MMU_68040 ? SG4_SHIFT1 : SG_ISHIFT)]))
#define pmap_ste_v(m, v) \
	(mmutype == MMU_68040 \
	 ? ((*pmap_ste1(m, v) & SG_V) && \
	    (*pmap_ste2(m, v) & SG_V)) \
	 : (*pmap_ste(m, v) & SG_V))
#else
#define	pmap_ste(m, v)	\
	(&((m)->pm_stab[(vaddr_t)(v) >> SG4_SHIFT1]))
#define pmap_ste_v(m, v) \
	((*pmap_ste1(m, v) & SG_V) && (*pmap_ste2(m, v) & SG_V))
#endif
#else
#define	pmap_ste(m, v)	 (&((m)->pm_stab[(vaddr_t)(v) >> SG_ISHIFT]))
#define pmap_ste_v(m, v) (*pmap_ste(m, v) & SG_V)
#endif

#define pmap_pte(m, v)	(&((m)->pm_ptab[(vaddr_t)(v) >> PG_SHIFT]))
#define pmap_pte_pa(pte)	(*(pte) & PG_FRAME)
#define pmap_pte_w(pte)		(*(pte) & PG_W)
#define pmap_pte_ci(pte)	(*(pte) & PG_CI)
#define pmap_pte_m(pte)		(*(pte) & PG_M)
#define pmap_pte_u(pte)		(*(pte) & PG_U)
#define pmap_pte_prot(pte)	(*(pte) & PG_PROT)
#define pmap_pte_v(pte)		(*(pte) & PG_V)

#define pmap_pte_set_w(pte, v) \
	if (v) *(pte) |= PG_W; else *(pte) &= ~PG_W
#define pmap_pte_set_prot(pte, v) \
	if (v) *(pte) |= PG_PROT; else *(pte) &= ~PG_PROT
#define pmap_pte_w_chg(pte, nw)		((nw) ^ pmap_pte_w(pte))
#define pmap_pte_prot_chg(pte, np)	((np) ^ pmap_pte_prot(pte))

/*
 * Given a map and a machine independent protection code,
 * convert to an m68k protection code.
 */
#define pte_prot(m, p)	(protection_codes[p])
u_int	protection_codes[8];

/*
 * Kernel page table page management.
 */
struct kpt_page {
	struct kpt_page *kpt_next;	/* link on either used or free list */
	vaddr_t		kpt_va;		/* always valid kernel VA */
	paddr_t		kpt_pa;		/* PA of this page (for speed) */
};
struct kpt_page *kpt_free_list, *kpt_used_list;
struct kpt_page *kpt_pages;

/*
 * Kernel segment/page table and page table map.
 * The page table map gives us a level of indirection we need to dynamically
 * expand the page table.  It is essentially a copy of the segment table
 * with PTEs instead of STEs.  All are initialized in locore at boot time.
 * Sysmap will initially contain VM_KERNEL_PT_PAGES pages of PTEs.
 * Segtabzero is an empty segment table which all processes share til they
 * reference something.
 */
paddr_t		Sysseg_pa;
st_entry_t	*Sysseg;
pt_entry_t	*Sysmap, *Sysptmap;
st_entry_t	*Segtabzero, *Segtabzeropa;
vsize_t		Sysptsize = VM_KERNEL_PT_PAGES;

static struct pmap kernel_pmap_store;
struct pmap	*const kernel_pmap_ptr = &kernel_pmap_store;
struct vm_map	*st_map, *pt_map;
struct vm_map st_map_store, pt_map_store;

vaddr_t		lwp0uarea;	/* lwp0 u-area VA, initialized in bootstrap */

paddr_t		avail_start;	/* PA of first available physical page */
paddr_t		avail_end;	/* PA of last available physical page */
vsize_t		mem_size;	/* memory size in bytes */
vaddr_t		virtual_avail;  /* VA of first avail page (after kernel bss)*/
vaddr_t		virtual_end;	/* VA of last avail page (end of kernel AS) */
int		page_cnt;	/* number of pages managed by VM system */

bool		pmap_initialized = false;	/* Has pmap_init completed? */

struct pv_header {
	struct pv_entry		pvh_first;	/* first PV entry */
	uint16_t		pvh_attrs;	/* attributes:
						   bits 0-7: PTE bits
						   bits 8-15: flags */
	uint16_t		pvh_cimappings;	/* # caller-specified CI
						   mappings */
};

#define	PVH_CI		0x10	/* all entries are cache-inhibited */
#define	PVH_PTPAGE	0x20	/* entry maps a page table page */

struct pv_header *pv_table;
TAILQ_HEAD(pv_page_list, pv_page) pv_page_freelist;
int		pv_nfree;

#ifdef CACHE_HAVE_VAC
u_int		pmap_aliasmask;	/* seperation at which VA aliasing ok */
#endif
#if defined(M68040) || defined(M68060)
u_int		protostfree;	/* prototype (default) free ST map */
#endif

pt_entry_t	*caddr1_pte;	/* PTE for CADDR1 */
pt_entry_t	*caddr2_pte;	/* PTE for CADDR2 */

struct pool	pmap_pmap_pool;	/* memory pool for pmap structures */
struct pool	pmap_pv_pool;	/* memory pool for pv entries */

#define pmap_alloc_pv()		pool_get(&pmap_pv_pool, PR_NOWAIT)
#define pmap_free_pv(pv)	pool_put(&pmap_pv_pool, (pv))

#define	PAGE_IS_MANAGED(pa)	(pmap_initialized && uvm_pageismanaged(pa))

static inline struct pv_header *
pa_to_pvh(paddr_t pa)
{
	int bank, pg = 0;	/* XXX gcc4 -Wuninitialized */

	bank = vm_physseg_find(atop((pa)), &pg);
	return &VM_PHYSMEM_PTR(bank)->pmseg.pvheader[pg];
}

/*
 * Internal routines
 */
void	pmap_remove_mapping(pmap_t, vaddr_t, pt_entry_t *, int);
bool	pmap_testbit(paddr_t, int);
bool	pmap_changebit(paddr_t, int, int);
int	pmap_enter_ptpage(pmap_t, vaddr_t, bool);
void	pmap_ptpage_addref(vaddr_t);
int	pmap_ptpage_delref(vaddr_t);
void	pmap_pinit(pmap_t);
void	pmap_release(pmap_t);

#ifdef DEBUG
void pmap_pvdump(paddr_t);
void pmap_check_wiring(const char *, vaddr_t);
#endif

/* pmap_remove_mapping flags */
#define	PRM_TFLUSH	0x01
#define	PRM_CFLUSH	0x02
#define	PRM_KEEPPTPAGE	0x04

/*
 * pmap_bootstrap_finalize:	[ INTERFACE ]
 *
 *	Initialize lwp0 uarea, curlwp, and curpcb after MMU is turned on,
 *	using lwp0uarea variable saved during pmap_bootstrap().
 */
void
pmap_bootstrap_finalize(void)
{

#if !defined(amiga) && !defined(atari)
	/*
	 * XXX
	 * amiga and atari have different pmap initialization functions
	 * and they require this earlier.
	 */
	uvmexp.pagesize = NBPG;
	uvm_setpagesize();
#endif

	/*
	 * Initialize protection array.
	 * XXX: Could this have port specific values? Can't this be static?
	 */
	protection_codes[VM_PROT_NONE|VM_PROT_NONE|VM_PROT_NONE]     = 0;
	protection_codes[VM_PROT_READ|VM_PROT_NONE|VM_PROT_NONE]     = PG_RO;
	protection_codes[VM_PROT_READ|VM_PROT_NONE|VM_PROT_EXECUTE]  = PG_RO;
	protection_codes[VM_PROT_NONE|VM_PROT_NONE|VM_PROT_EXECUTE]  = PG_RO;
	protection_codes[VM_PROT_NONE|VM_PROT_WRITE|VM_PROT_NONE]    = PG_RW;
	protection_codes[VM_PROT_NONE|VM_PROT_WRITE|VM_PROT_EXECUTE] = PG_RW;
	protection_codes[VM_PROT_READ|VM_PROT_WRITE|VM_PROT_NONE]    = PG_RW;
	protection_codes[VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE] = PG_RW;

	/*
	 * Initialize pmap_kernel().
	 */
	pmap_kernel()->pm_stpa = (st_entry_t *)Sysseg_pa;
	pmap_kernel()->pm_stab = Sysseg;
	pmap_kernel()->pm_ptab = Sysmap;
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040)
		pmap_kernel()->pm_stfree = protostfree;
#endif
	pmap_kernel()->pm_count = 1;

	/*
	 * Initialize lwp0 uarea, curlwp, and curpcb.
	 */
	memset((void *)lwp0uarea, 0, USPACE);
	uvm_lwp_setuarea(&lwp0, lwp0uarea);
	curlwp = &lwp0;
	curpcb = lwp_getpcb(&lwp0);
}

/*
 * pmap_virtual_space:		[ INTERFACE ]
 *
 *	Report the range of available kernel virtual address
 *	space to the VM system during bootstrap.
 *
 *	This is only an interface function if we do not use
 *	pmap_steal_memory()!
 *
 *	Note: no locking is necessary in this function.
 */
void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{

	*vstartp = virtual_avail;
	*vendp = virtual_end;
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
	vaddr_t		addr, addr2;
	vsize_t		s;
	struct pv_header *pvh;
	int		rv;
	int		npages;
	int		bank;

	PMAP_DPRINTF(PDB_FOLLOW, ("pmap_init()\n"));

	/*
	 * Before we do anything else, initialize the PTE pointers
	 * used by pmap_zero_page() and pmap_copy_page().
	 */
	caddr1_pte = pmap_pte(pmap_kernel(), CADDR1);
	caddr2_pte = pmap_pte(pmap_kernel(), CADDR2);

	PMAP_DPRINTF(PDB_INIT,
	    ("pmap_init: Sysseg %p, Sysmap %p, Sysptmap %p\n",
	    Sysseg, Sysmap, Sysptmap));
	PMAP_DPRINTF(PDB_INIT,
	    ("  pstart %lx, pend %lx, vstart %lx, vend %lx\n",
	    avail_start, avail_end, virtual_avail, virtual_end));

	/*
	 * Allocate memory for random pmap data structures.  Includes the
	 * initial segment table, pv_head_table and pmap_attributes.
	 */
	for (page_cnt = 0, bank = 0; bank < vm_nphysseg; bank++)
		page_cnt += VM_PHYSMEM_PTR(bank)->end - VM_PHYSMEM_PTR(bank)->start;
	s = M68K_STSIZE;					/* Segtabzero */
	s += page_cnt * sizeof(struct pv_header);	/* pv table */
	s = round_page(s);
	addr = uvm_km_alloc(kernel_map, s, 0, UVM_KMF_WIRED | UVM_KMF_ZERO);
	if (addr == 0)
		panic("pmap_init: can't allocate data structures");

	Segtabzero = (st_entry_t *)addr;
	(void)pmap_extract(pmap_kernel(), addr,
	    (paddr_t *)(void *)&Segtabzeropa);
	addr += M68K_STSIZE;

	pv_table = (struct pv_header *) addr;
	addr += page_cnt * sizeof(struct pv_header);

	PMAP_DPRINTF(PDB_INIT, ("pmap_init: %lx bytes: page_cnt %x s0 %p(%p) "
	    "tbl %p\n",
	    s, page_cnt, Segtabzero, Segtabzeropa,
	    pv_table));

	/*
	 * Now that the pv and attribute tables have been allocated,
	 * assign them to the memory segments.
	 */
	pvh = pv_table;
	for (bank = 0; bank < vm_nphysseg; bank++) {
		npages = VM_PHYSMEM_PTR(bank)->end - VM_PHYSMEM_PTR(bank)->start;
		VM_PHYSMEM_PTR(bank)->pmseg.pvheader = pvh;
		pvh += npages;
	}

	/*
	 * Allocate physical memory for kernel PT pages and their management.
	 * We need 1 PT page per possible task plus some slop.
	 */
	npages = min(atop(M68K_MAX_KPTSIZE), maxproc+16);
	s = ptoa(npages) + round_page(npages * sizeof(struct kpt_page));

	/*
	 * Verify that space will be allocated in region for which
	 * we already have kernel PT pages.
	 */
	addr = 0;
	rv = uvm_map(kernel_map, &addr, s, NULL, UVM_UNKNOWN_OFFSET, 0,
	    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
	    UVM_ADV_RANDOM, UVM_FLAG_NOMERGE));
	if (rv != 0 || (addr + s) >= (vaddr_t)Sysmap)
		panic("pmap_init: kernel PT too small");
	uvm_unmap(kernel_map, addr, addr + s);

	/*
	 * Now allocate the space and link the pages together to
	 * form the KPT free list.
	 */
	addr = uvm_km_alloc(kernel_map, s, 0, UVM_KMF_WIRED | UVM_KMF_ZERO);
	if (addr == 0)
		panic("pmap_init: cannot allocate KPT free list");
	s = ptoa(npages);
	addr2 = addr + s;
	kpt_pages = &((struct kpt_page *)addr2)[npages];
	kpt_free_list = NULL;
	do {
		addr2 -= PAGE_SIZE;
		(--kpt_pages)->kpt_next = kpt_free_list;
		kpt_free_list = kpt_pages;
		kpt_pages->kpt_va = addr2;
		(void) pmap_extract(pmap_kernel(), addr2,
		    (paddr_t *)&kpt_pages->kpt_pa);
	} while (addr != addr2);

	PMAP_DPRINTF(PDB_INIT, ("pmap_init: KPT: %ld pages from %lx to %lx\n",
	    atop(s), addr, addr + s));

	/*
	 * Allocate the segment table map and the page table map.
	 */
	s = maxproc * M68K_STSIZE;
	st_map = uvm_km_suballoc(kernel_map, &addr, &addr2, s, 0, false,
	    &st_map_store);

	addr = M68K_PTBASE;
	if ((M68K_PTMAXSIZE / M68K_MAX_PTSIZE) < maxproc) {
		s = M68K_PTMAXSIZE;
		/*
		 * XXX We don't want to hang when we run out of
		 * page tables, so we lower maxproc so that fork()
		 * will fail instead.  Note that root could still raise
		 * this value via sysctl(3).
		 */
		maxproc = (M68K_PTMAXSIZE / M68K_MAX_PTSIZE);
	} else
		s = (maxproc * M68K_MAX_PTSIZE);
	pt_map = uvm_km_suballoc(kernel_map, &addr, &addr2, s, 0,
	    true, &pt_map_store);

#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040) {
		protostfree = ~l2tobm(0);
		for (rv = MAXUL2SIZE; rv < sizeof(protostfree)*NBBY; rv++)
			protostfree &= ~l2tobm(rv);
	}
#endif

	/*
	 * Initialize the pmap pools.
	 */
	pool_init(&pmap_pmap_pool, sizeof(struct pmap), 0, 0, 0, "pmappl",
	    &pool_allocator_nointr, IPL_NONE);

	/*
	 * Initialize the pv_entry pools.
	 */
	pool_init(&pmap_pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pvpl",
	    &pool_allocator_meta, IPL_NONE);

	/*
	 * Now that this is done, mark the pages shared with the
	 * hardware page table search as non-CCB (actually, as CI).
	 *
	 * XXX Hm. Given that this is in the kernel map, can't we just
	 * use the va's?
	 */
#ifdef M68060
#if defined(M68020) || defined(M68030) || defined(M68040)
	if (cputype == CPU_68060)
#endif
	{
		struct kpt_page *kptp = kpt_free_list;
		paddr_t paddr;

		while (kptp) {
			pmap_changebit(kptp->kpt_pa, PG_CI, ~PG_CCB);
			kptp = kptp->kpt_next;
		}

		paddr = (paddr_t)Segtabzeropa;
		while (paddr < (paddr_t)Segtabzeropa + M68K_STSIZE) {
			pmap_changebit(paddr, PG_CI, ~PG_CCB);
			paddr += PAGE_SIZE;
		}

		DCIS();
	}
#endif

	/*
	 * Now it is safe to enable pv_table recording.
	 */
	pmap_initialized = true;
}

/*
 * pmap_map:
 *
 *	Used to map a range of physical addresses into kernel
 *	virtual address space.
 *
 *	For now, VM is already on, we only need to map the
 *	specified memory.
 *
 *	Note: THIS FUNCTION IS DEPRECATED, AND SHOULD BE REMOVED!
 */
vaddr_t
pmap_map(vaddr_t va, paddr_t spa, paddr_t epa, int prot)
{

	PMAP_DPRINTF(PDB_FOLLOW,
	    ("pmap_map(%lx, %lx, %lx, %x)\n", va, spa, epa, prot));

	while (spa < epa) {
		pmap_enter(pmap_kernel(), va, spa, prot, 0);
		va += PAGE_SIZE;
		spa += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());
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
	struct pmap *pmap;

	PMAP_DPRINTF(PDB_FOLLOW|PDB_CREATE,
	    ("pmap_create()\n"));

	pmap = pool_get(&pmap_pmap_pool, PR_WAITOK);
	memset(pmap, 0, sizeof(*pmap));
	pmap_pinit(pmap);
	return pmap;
}

/*
 * pmap_pinit:
 *
 *	Initialize a preallocated and zeroed pmap structure.
 *
 *	Note: THIS FUNCTION SHOULD BE MOVED INTO pmap_create()!
 */
void
pmap_pinit(struct pmap *pmap)
{

	PMAP_DPRINTF(PDB_FOLLOW|PDB_CREATE,
	    ("pmap_pinit(%p)\n", pmap));

	/*
	 * No need to allocate page table space yet but we do need a
	 * valid segment table.  Initially, we point everyone at the
	 * "null" segment table.  On the first pmap_enter, a real
	 * segment table will be allocated.
	 */
	pmap->pm_stab = Segtabzero;
	pmap->pm_stpa = Segtabzeropa;
#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
	if (mmutype == MMU_68040)
#endif
		pmap->pm_stfree = protostfree;
#endif
	pmap->pm_count = 1;
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
	int count;

	PMAP_DPRINTF(PDB_FOLLOW, ("pmap_destroy(%p)\n", pmap));

	count = atomic_dec_uint_nv(&pmap->pm_count);
	if (count == 0) {
		pmap_release(pmap);
		pool_put(&pmap_pmap_pool, pmap);
	}
}

/*
 * pmap_release:
 *
 *	Relese the resources held by a pmap.
 *
 *	Note: THIS FUNCTION SHOULD BE MOVED INTO pmap_destroy().
 */
void
pmap_release(pmap_t pmap)
{

	PMAP_DPRINTF(PDB_FOLLOW, ("pmap_release(%p)\n", pmap));

#ifdef notdef /* DIAGNOSTIC */
	/* count would be 0 from pmap_destroy... */
	if (pmap->pm_count != 1)
		panic("pmap_release count");
#endif

	if (pmap->pm_ptab) {
		pmap_remove(pmap_kernel(), (vaddr_t)pmap->pm_ptab,
		    (vaddr_t)pmap->pm_ptab + M68K_MAX_PTSIZE);
		uvm_km_pgremove((vaddr_t)pmap->pm_ptab,
		    (vaddr_t)pmap->pm_ptab + M68K_MAX_PTSIZE);
		uvm_km_free(pt_map, (vaddr_t)pmap->pm_ptab,
		    M68K_MAX_PTSIZE, UVM_KMF_VAONLY);
	}
	KASSERT(pmap->pm_stab == Segtabzero);
}

/*
 * pmap_reference:		[ INTERFACE ]
 *
 *	Add a reference to the specified pmap.
 */
void
pmap_reference(pmap_t pmap)
{
	PMAP_DPRINTF(PDB_FOLLOW, ("pmap_reference(%p)\n", pmap));

	atomic_inc_uint(&pmap->pm_count);
}

/*
 * pmap_activate:		[ INTERFACE ]
 *
 *	Activate the pmap used by the specified process.  This includes
 *	reloading the MMU context if the current process, and marking
 *	the pmap in use by the processor.
 *
 *	Note: we may only use spin locks here, since we are called
 *	by a critical section in cpu_switch()!
 */
void
pmap_activate(struct lwp *l)
{
	pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;

	PMAP_DPRINTF(PDB_FOLLOW|PDB_SEGTAB,
	    ("pmap_activate(%p)\n", l));

	PMAP_ACTIVATE(pmap, (curlwp->l_flag & LW_IDLE) != 0 ||
	    l->l_proc == curproc);
}

/*
 * pmap_deactivate:		[ INTERFACE ]
 *
 *	Mark that the pmap used by the specified process is no longer
 *	in use by the processor.
 *
 *	The comment above pmap_activate() wrt. locking applies here,
 *	as well.
 */
void
pmap_deactivate(struct lwp *l)
{

	/* No action necessary in this pmap implementation. */
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
	vaddr_t nssva;
	pt_entry_t *pte;
	int flags;
#ifdef CACHE_HAVE_VAC
	bool firstpage = true, needcflush = false;
#endif

	PMAP_DPRINTF(PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT,
	    ("pmap_remove(%p, %lx, %lx)\n", pmap, sva, eva));

	flags = active_pmap(pmap) ? PRM_TFLUSH : 0;
	while (sva < eva) {
		nssva = m68k_trunc_seg(sva) + NBSEG;
		if (nssva == 0 || nssva > eva)
			nssva = eva;

		/*
		 * Invalidate every valid mapping within this segment.
		 */

		pte = pmap_pte(pmap, sva);
		while (sva < nssva) {

			/*
			 * If this segment is unallocated,
			 * skip to the next segment boundary.
			 */

			if (!pmap_ste_v(pmap, sva)) {
				sva = nssva;
				break;
			}

			if (pmap_pte_v(pte)) {
#ifdef CACHE_HAVE_VAC
				if (pmap_aliasmask) {

					/*
					 * Purge kernel side of VAC to ensure
					 * we get the correct state of any
					 * hardware maintained bits.
					 */

					if (firstpage) {
						DCIS();
					}

					/*
					 * Remember if we may need to
					 * flush the VAC due to a non-CI
					 * mapping.
					 */

					if (!needcflush && !pmap_pte_ci(pte))
						needcflush = true;

				}
				firstpage = false;
#endif
				pmap_remove_mapping(pmap, sva, pte, flags);
			}
			pte++;
			sva += PAGE_SIZE;
		}
	}

#ifdef CACHE_HAVE_VAC

	/*
	 * Didn't do anything, no need for cache flushes
	 */

	if (firstpage)
		return;

	/*
	 * In a couple of cases, we don't need to worry about flushing
	 * the VAC:
	 * 	1. if this is a kernel mapping,
	 *	   we have already done it
	 *	2. if it is a user mapping not for the current process,
	 *	   it won't be there
	 */

	if (pmap_aliasmask && !active_user_pmap(pmap))
		needcflush = false;
	if (needcflush) {
		if (pmap == pmap_kernel()) {
			DCIS();
		} else {
			DCIU();
		}
	}
#endif
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
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	struct pv_header *pvh;
	struct pv_entry *pv;
	pt_entry_t *pte;
	int s;

#ifdef DEBUG
	if ((pmapdebug & (PDB_FOLLOW|PDB_PROTECT)) ||
	    (prot == VM_PROT_NONE && (pmapdebug & PDB_REMOVE)))
		printf("pmap_page_protect(%p, %x)\n", pg, prot);
#endif

	switch (prot) {
	case VM_PROT_READ|VM_PROT_WRITE:
	case VM_PROT_ALL:
		return;

	/* copy_on_write */
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		pmap_changebit(pa, PG_RO, ~0);
		return;

	/* remove_all */
	default:
		break;
	}

	pvh = pa_to_pvh(pa);
	pv = &pvh->pvh_first;
	s = splvm();
	while (pv->pv_pmap != NULL) {

		pte = pmap_pte(pv->pv_pmap, pv->pv_va);
#ifdef DEBUG
		if (!pmap_ste_v(pv->pv_pmap, pv->pv_va) ||
		    pmap_pte_pa(pte) != pa)
			panic("pmap_page_protect: bad mapping");
#endif
		pmap_remove_mapping(pv->pv_pmap, pv->pv_va,
		    pte, PRM_TFLUSH|PRM_CFLUSH);
	}
	splx(s);
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
	vaddr_t nssva;
	pt_entry_t *pte;
	bool firstpage, needtflush;
	int isro;

	PMAP_DPRINTF(PDB_FOLLOW|PDB_PROTECT,
	    ("pmap_protect(%p, %lx, %lx, %x)\n",
	    pmap, sva, eva, prot));

#ifdef PMAPSTATS
	protect_stats.calls++;
#endif
	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}
	isro = pte_prot(pmap, prot);
	needtflush = active_pmap(pmap);
	firstpage = true;
	while (sva < eva) {
		nssva = m68k_trunc_seg(sva) + NBSEG;
		if (nssva == 0 || nssva > eva)
			nssva = eva;

		/*
		 * If VA belongs to an unallocated segment,
		 * skip to the next segment boundary.
		 */

		if (!pmap_ste_v(pmap, sva)) {
			sva = nssva;
			continue;
		}

		/*
		 * Change protection on mapping if it is valid and doesn't
		 * already have the correct protection.
		 */

		pte = pmap_pte(pmap, sva);
		while (sva < nssva) {
			if (pmap_pte_v(pte) && pmap_pte_prot_chg(pte, isro)) {
#ifdef CACHE_HAVE_VAC

				/*
				 * Purge kernel side of VAC to ensure we
				 * get the correct state of any hardware
				 * maintained bits.
				 *
				 * XXX do we need to clear the VAC in
				 * general to reflect the new protection?
				 */

				if (firstpage && pmap_aliasmask)
					DCIS();
#endif

#if defined(M68040) || defined(M68060)

				/*
				 * Clear caches if making RO (see section
				 * "7.3 Cache Coherency" in the manual).
				 */

#if defined(M68020) || defined(M68030)
				if (isro && mmutype == MMU_68040)
#else
				if (isro)
#endif
				{
					paddr_t pa = pmap_pte_pa(pte);

					DCFP(pa);
					ICPP(pa);
				}
#endif
				pmap_pte_set_prot(pte, isro);
				if (needtflush)
					TBIS(sva);
				firstpage = false;
			}
			pte++;
			sva += PAGE_SIZE;
		}
	}
}

/*
 * pmap_enter:			[ INTERFACE ]
 *
 *	Insert the given physical page (pa) at
 *	the specified virtual address (va) in the
 *	target physical map with the protection requested.
 *
 *	If specified, the page will be wired down, meaning
 *	that the related pte cannot be reclaimed.
 *
 *	Note: This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  Thatis, this routine must actually
 *	insert this page into the given map NOW.
 */
int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	pt_entry_t *pte;
	int npte;
	paddr_t opa;
	bool cacheable = true;
	bool checkpv = true;
	bool wired = (flags & PMAP_WIRED) != 0;
	bool can_fail = (flags & PMAP_CANFAIL) != 0;

	PMAP_DPRINTF(PDB_FOLLOW|PDB_ENTER,
	    ("pmap_enter(%p, %lx, %lx, %x, %x)\n",
	    pmap, va, pa, prot, wired));

#ifdef DIAGNOSTIC
	/*
	 * pmap_enter() should never be used for CADDR1 and CADDR2.
	 */
	if (pmap == pmap_kernel() &&
	    (va == (vaddr_t)CADDR1 || va == (vaddr_t)CADDR2))
		panic("pmap_enter: used for CADDR1 or CADDR2");
#endif

	/*
	 * For user mapping, allocate kernel VM resources if necessary.
	 */
	if (pmap->pm_ptab == NULL) {
		pmap->pm_ptab = (pt_entry_t *)
		    uvm_km_alloc(pt_map, M68K_MAX_PTSIZE, 0,
		    UVM_KMF_VAONLY | 
		    (can_fail ? UVM_KMF_NOWAIT : UVM_KMF_WAITVA));
		if (pmap->pm_ptab == NULL)
			return ENOMEM;
	}

	/*
	 * Segment table entry not valid, we need a new PT page
	 */
	if (!pmap_ste_v(pmap, va)) {
		int err = pmap_enter_ptpage(pmap, va, can_fail);
		if (err)
			return err;
	}

	pa = m68k_trunc_page(pa);
	pte = pmap_pte(pmap, va);
	opa = pmap_pte_pa(pte);

	PMAP_DPRINTF(PDB_ENTER, ("enter: pte %p, *pte %x\n", pte, *pte));

	/*
	 * Mapping has not changed, must be protection or wiring change.
	 */
	if (opa == pa) {
		/*
		 * Wiring change, just update stats.
		 * We don't worry about wiring PT pages as they remain
		 * resident as long as there are valid mappings in them.
		 * Hence, if a user page is wired, the PT page will be also.
		 */
		if (pmap_pte_w_chg(pte, wired ? PG_W : 0)) {
			PMAP_DPRINTF(PDB_ENTER,
			    ("enter: wiring change -> %x\n", wired));
			if (wired)
				pmap->pm_stats.wired_count++;
			else
				pmap->pm_stats.wired_count--;
		}
		/*
		 * Retain cache inhibition status
		 */
		checkpv = false;
		if (pmap_pte_ci(pte))
			cacheable = false;
		goto validate;
	}

	/*
	 * Mapping has changed, invalidate old range and fall through to
	 * handle validating new mapping.
	 */
	if (opa) {
		PMAP_DPRINTF(PDB_ENTER,
		    ("enter: removing old mapping %lx\n", va));
		pmap_remove_mapping(pmap, va, pte,
		    PRM_TFLUSH|PRM_CFLUSH|PRM_KEEPPTPAGE);
	}

	/*
	 * If this is a new user mapping, increment the wiring count
	 * on this PT page.  PT pages are wired down as long as there
	 * is a valid mapping in the page.
	 */
	if (pmap != pmap_kernel())
		pmap_ptpage_addref(trunc_page((vaddr_t)pte));

	/*
	 * Enter on the PV list if part of our managed memory
	 * Note that we raise IPL while manipulating pv_table
	 * since pmap_enter can be called at interrupt time.
	 */
	if (PAGE_IS_MANAGED(pa)) {
		struct pv_header *pvh;
		struct pv_entry *pv, *npv;
		int s;

		pvh = pa_to_pvh(pa);
		pv = &pvh->pvh_first;
		s = splvm();

		PMAP_DPRINTF(PDB_ENTER,
		    ("enter: pv at %p: %lx/%p/%p\n",
		    pv, pv->pv_va, pv->pv_pmap, pv->pv_next));
		/*
		 * No entries yet, use header as the first entry
		 */
		if (pv->pv_pmap == NULL) {
			pv->pv_va = va;
			pv->pv_pmap = pmap;
			pv->pv_next = NULL;
			pv->pv_ptste = NULL;
			pv->pv_ptpmap = NULL;
			pvh->pvh_attrs = 0;
		}
		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 */
		else {
#ifdef DEBUG
			for (npv = pv; npv; npv = npv->pv_next)
				if (pmap == npv->pv_pmap && va == npv->pv_va)
					panic("pmap_enter: already in pv_tab");
#endif
			npv = pmap_alloc_pv();
			KASSERT(npv != NULL);
			npv->pv_va = va;
			npv->pv_pmap = pmap;
			npv->pv_next = pv->pv_next;
			npv->pv_ptste = NULL;
			npv->pv_ptpmap = NULL;
			pv->pv_next = npv;

#ifdef CACHE_HAVE_VAC

			/*
			 * Since there is another logical mapping for the
			 * same page we may need to cache-inhibit the
			 * descriptors on those CPUs with external VACs.
			 * We don't need to CI if:
			 *
			 * - No two mappings belong to the same user pmaps.
			 *   Since the cache is flushed on context switches
			 *   there is no problem between user processes.
			 *
			 * - Mappings within a single pmap are a certain
			 *   magic distance apart.  VAs at these appropriate
			 *   boundaries map to the same cache entries or
			 *   otherwise don't conflict.
			 *
			 * To keep it simple, we only check for these special
			 * cases if there are only two mappings, otherwise we
			 * punt and always CI.
			 *
			 * Note that there are no aliasing problems with the
			 * on-chip data-cache when the WA bit is set.
			 */

			if (pmap_aliasmask) {
				if (pvh->pvh_attrs & PVH_CI) {
					PMAP_DPRINTF(PDB_CACHE,
					    ("enter: pa %lx already CI'ed\n",
					    pa));
					checkpv = cacheable = false;
				} else if (npv->pv_next ||
					   ((pmap == pv->pv_pmap ||
					     pmap == pmap_kernel() ||
					     pv->pv_pmap == pmap_kernel()) &&
					    ((pv->pv_va & pmap_aliasmask) !=
					     (va & pmap_aliasmask)))) {
					PMAP_DPRINTF(PDB_CACHE,
					    ("enter: pa %lx CI'ing all\n",
					    pa));
					cacheable = false;
					pvh->pvh_attrs |= PVH_CI;
				}
			}
#endif
		}

		/*
		 * Speed pmap_is_referenced() or pmap_is_modified() based
		 * on the hint provided in access_type.
		 */
#ifdef DIAGNOSTIC
		if ((flags & VM_PROT_ALL) & ~prot)
			panic("pmap_enter: access_type exceeds prot");
#endif
		if (flags & VM_PROT_WRITE)
			pvh->pvh_attrs |= (PG_U|PG_M);
		else if (flags & VM_PROT_ALL)
			pvh->pvh_attrs |= PG_U;

		splx(s);
	}
	/*
	 * Assumption: if it is not part of our managed memory
	 * then it must be device memory which may be volitile.
	 */
	else if (pmap_initialized) {
		checkpv = cacheable = false;
	}

	/*
	 * Increment counters
	 */
	pmap->pm_stats.resident_count++;
	if (wired)
		pmap->pm_stats.wired_count++;

validate:
#ifdef CACHE_HAVE_VAC
	/*
	 * Purge kernel side of VAC to ensure we get correct state
	 * of HW bits so we don't clobber them.
	 */
	if (pmap_aliasmask)
		DCIS();
#endif

	/*
	 * Build the new PTE.
	 */

	npte = pa | pte_prot(pmap, prot) | (*pte & (PG_M|PG_U)) | PG_V;
	if (wired)
		npte |= PG_W;
	if (!checkpv && !cacheable)
#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
		npte |= (mmutype == MMU_68040 ? PG_CIN : PG_CI);
#else
		npte |= PG_CIN;
#endif
#else
		npte |= PG_CI;
#endif
#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
	else if (mmutype == MMU_68040 && (npte & (PG_PROT|PG_CI)) == PG_RW)
#else
	else if ((npte & (PG_PROT|PG_CI)) == PG_RW)
#endif
		npte |= PG_CCB;
#endif

	PMAP_DPRINTF(PDB_ENTER, ("enter: new pte value %x\n", npte));

	/*
	 * Remember if this was a wiring-only change.
	 * If so, we need not flush the TLB and caches.
	 */

	wired = ((*pte ^ npte) == PG_W);
#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
	if (mmutype == MMU_68040 && !wired)
#else
	if (!wired)
#endif
	{
		DCFP(pa);
		ICPP(pa);
	}
#endif
	*pte = npte;
	if (!wired && active_pmap(pmap))
		TBIS(va);
#ifdef CACHE_HAVE_VAC
	/*
	 * The following is executed if we are entering a second
	 * (or greater) mapping for a physical page and the mappings
	 * may create an aliasing problem.  In this case we must
	 * cache inhibit the descriptors involved and flush any
	 * external VAC.
	 */
	if (checkpv && !cacheable) {
		pmap_changebit(pa, PG_CI, ~0);
		DCIA();
#ifdef DEBUG
		if ((pmapdebug & (PDB_CACHE|PDB_PVDUMP)) ==
		    (PDB_CACHE|PDB_PVDUMP))
			pmap_pvdump(pa);
#endif
	}
#endif
#ifdef DEBUG
	if ((pmapdebug & PDB_WIRING) && pmap != pmap_kernel())
		pmap_check_wiring("enter", trunc_page((vaddr_t)pte));
#endif

	return 0;
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, u_int flags)
{
	pmap_t pmap = pmap_kernel();
	pt_entry_t *pte;
	int s, npte;

	PMAP_DPRINTF(PDB_FOLLOW|PDB_ENTER,
	    ("pmap_kenter_pa(%lx, %lx, %x)\n", va, pa, prot));

	/*
	 * Segment table entry not valid, we need a new PT page
	 */

	if (!pmap_ste_v(pmap, va)) {
		s = splvm();
		pmap_enter_ptpage(pmap, va, false);
		splx(s);
	}

	pa = m68k_trunc_page(pa);
	pte = pmap_pte(pmap, va);

	PMAP_DPRINTF(PDB_ENTER, ("enter: pte %p, *pte %x\n", pte, *pte));
	KASSERT(!pmap_pte_v(pte));

	/*
	 * Increment counters
	 */

	pmap->pm_stats.resident_count++;
	pmap->pm_stats.wired_count++;

	/*
	 * Build the new PTE.
	 */

	npte = pa | pte_prot(pmap, prot) | PG_V | PG_W;
#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
	if (mmutype == MMU_68040 && (npte & PG_PROT) == PG_RW)
#else
	if ((npte & PG_PROT) == PG_RW)
#endif
		npte |= PG_CCB;

	if (mmutype == MMU_68040) {
		DCFP(pa);
		ICPP(pa);
	}
#endif

	*pte = npte;
	TBIS(va);
}

void
pmap_kremove(vaddr_t va, vsize_t size)
{
	pmap_t pmap = pmap_kernel();
	pt_entry_t *pte;
	vaddr_t nssva;
	vaddr_t eva = va + size;
#ifdef CACHE_HAVE_VAC
	bool firstpage, needcflush;
#endif

	PMAP_DPRINTF(PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT,
	    ("pmap_kremove(%lx, %lx)\n", va, size));

#ifdef CACHE_HAVE_VAC
	firstpage = true;
	needcflush = false;
#endif
	while (va < eva) {
		nssva = m68k_trunc_seg(va) + NBSEG;
		if (nssva == 0 || nssva > eva)
			nssva = eva;

		/*
		 * If VA belongs to an unallocated segment,
		 * skip to the next segment boundary.
		 */

		if (!pmap_ste_v(pmap, va)) {
			va = nssva;
			continue;
		}

		/*
		 * Invalidate every valid mapping within this segment.
		 */

		pte = pmap_pte(pmap, va);
		while (va < nssva) {
			if (!pmap_pte_v(pte)) {
				pte++;
				va += PAGE_SIZE;
				continue;
			}
#ifdef CACHE_HAVE_VAC
			if (pmap_aliasmask) {

				/*
				 * Purge kernel side of VAC to ensure
				 * we get the correct state of any
				 * hardware maintained bits.
				 */

				if (firstpage) {
					DCIS();
					firstpage = false;
				}

				/*
				 * Remember if we may need to
				 * flush the VAC.
				 */

				needcflush = true;
			}
#endif
			pmap->pm_stats.wired_count--;
			pmap->pm_stats.resident_count--;
			*pte = PG_NV;
			TBIS(va);
			pte++;
			va += PAGE_SIZE;
		}
	}

#ifdef CACHE_HAVE_VAC

	/*
	 * In a couple of cases, we don't need to worry about flushing
	 * the VAC:
	 * 	1. if this is a kernel mapping,
	 *	   we have already done it
	 *	2. if it is a user mapping not for the current process,
	 *	   it won't be there
	 */

	if (pmap_aliasmask && !active_user_pmap(pmap))
		needcflush = false;
	if (needcflush) {
		if (pmap == pmap_kernel()) {
			DCIS();
		} else {
			DCIU();
		}
	}
#endif
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

	PMAP_DPRINTF(PDB_FOLLOW,
	    ("pmap_unwire(%p, %lx)\n", pmap, va));

	pte = pmap_pte(pmap, va);

	/*
	 * If wiring actually changed (always?) clear the wire bit and
	 * update the wire count.  Note that wiring is not a hardware
	 * characteristic so there is no need to invalidate the TLB.
	 */

	if (pmap_pte_w_chg(pte, 0)) {
		pmap_pte_set_w(pte, false);
		pmap->pm_stats.wired_count--;
	}
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
	paddr_t pa;
	u_int pte;

	PMAP_DPRINTF(PDB_FOLLOW,
	    ("pmap_extract(%p, %lx) -> ", pmap, va));

	if (pmap_ste_v(pmap, va)) {
		pte = *(u_int *)pmap_pte(pmap, va);
		if (pte) {
			pa = (pte & PG_FRAME) | (va & ~PG_FRAME);
			if (pap != NULL)
				*pap = pa;
#ifdef DEBUG
			if (pmapdebug & PDB_FOLLOW)
				printf("%lx\n", pa);
#endif
			return true;
		}
	}
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("failed\n");
#endif
	return false;
}

/*
 * pmap_copy:		[ INTERFACE ]
 *
 *	Copy the mapping range specified by src_addr/len
 *	from the source map to the range dst_addr/len
 *	in the destination map.
 *
 *	This routine is only advisory and need not do anything.
 */
void
pmap_copy(pmap_t dst_pmap, pmap_t src_pmap, vaddr_t dst_addr, vsize_t len,
    vaddr_t src_addr)
{

	PMAP_DPRINTF(PDB_FOLLOW,
	    ("pmap_copy(%p, %p, %lx, %lx, %lx)\n",
	    dst_pmap, src_pmap, dst_addr, len, src_addr));
}

/*
 * pmap_collect1():
 *
 *	Garbage-collect KPT pages.  Helper for the above (bogus)
 *	pmap_collect().
 *
 *	Note: THIS SHOULD GO AWAY, AND BE REPLACED WITH A BETTER
 *	WAY OF HANDLING PT PAGES!
 */
static inline void
pmap_collect1(pmap_t pmap, paddr_t startpa, paddr_t endpa)
{
	paddr_t pa;
	struct pv_header *pvh;
	struct pv_entry *pv;
	pt_entry_t *pte;
	paddr_t kpa;
#ifdef DEBUG
	st_entry_t *ste;
	int opmapdebug = 0;
#endif

	for (pa = startpa; pa < endpa; pa += PAGE_SIZE) {
		struct kpt_page *kpt, **pkpt;

		/*
		 * Locate physical pages which are being used as kernel
		 * page table pages.
		 */

		pvh = pa_to_pvh(pa);
		pv = &pvh->pvh_first;
		if (pv->pv_pmap != pmap_kernel() ||
		    !(pvh->pvh_attrs & PVH_PTPAGE))
			continue;
		do {
			if (pv->pv_ptste && pv->pv_ptpmap == pmap_kernel())
				break;
		} while ((pv = pv->pv_next));
		if (pv == NULL)
			continue;
#ifdef DEBUG
		if (pv->pv_va < (vaddr_t)Sysmap ||
		    pv->pv_va >= (vaddr_t)Sysmap + M68K_MAX_PTSIZE) {
			printf("collect: kernel PT VA out of range\n");
			pmap_pvdump(pa);
			continue;
		}
#endif
		pte = (pt_entry_t *)(pv->pv_va + PAGE_SIZE);
		while (--pte >= (pt_entry_t *)pv->pv_va && *pte == PG_NV)
			;
		if (pte >= (pt_entry_t *)pv->pv_va)
			continue;

#ifdef DEBUG
		if (pmapdebug & (PDB_PTPAGE|PDB_COLLECT)) {
			printf("collect: freeing KPT page at %lx (ste %x@%p)\n",
			    pv->pv_va, *pv->pv_ptste, pv->pv_ptste);
			opmapdebug = pmapdebug;
			pmapdebug |= PDB_PTPAGE;
		}

		ste = pv->pv_ptste;
#endif
		/*
		 * If all entries were invalid we can remove the page.
		 * We call pmap_remove_entry to take care of invalidating
		 * ST and Sysptmap entries.
		 */

		(void) pmap_extract(pmap, pv->pv_va, &kpa);
		pmap_remove_mapping(pmap, pv->pv_va, NULL,
		    PRM_TFLUSH|PRM_CFLUSH);

		/*
		 * Use the physical address to locate the original
		 * (kmem_alloc assigned) address for the page and put
		 * that page back on the free list.
		 */

		for (pkpt = &kpt_used_list, kpt = *pkpt;
		     kpt != NULL;
		     pkpt = &kpt->kpt_next, kpt = *pkpt)
			if (kpt->kpt_pa == kpa)
				break;
#ifdef DEBUG
		if (kpt == NULL)
			panic("pmap_collect: lost a KPT page");
		if (pmapdebug & (PDB_PTPAGE|PDB_COLLECT))
			printf("collect: %lx (%lx) to free list\n",
			    kpt->kpt_va, kpa);
#endif
		*pkpt = kpt->kpt_next;
		kpt->kpt_next = kpt_free_list;
		kpt_free_list = kpt;
#ifdef DEBUG
		if (pmapdebug & (PDB_PTPAGE|PDB_COLLECT))
			pmapdebug = opmapdebug;

		if (*ste != SG_NV)
			printf("collect: kernel STE at %p still valid (%x)\n",
			    ste, *ste);
		ste = &Sysptmap[ste - pmap_ste(pmap_kernel(), 0)];
		if (*ste != SG_NV)
			printf("collect: kernel PTmap at %p still valid (%x)\n",
			    ste, *ste);
#endif
	}
}

/*
 * pmap_collect:
 *
 *	Helper for pmap_enter_ptpage().
 *
 *	Garbage collects the physical map system for pages which are no
 *	longer used.  Success need not be guaranteed -- that is, there
 *	may well be pages which are not referenced, but others may be
 *	collected.
 */
static void
pmap_collect(void)
{
	int bank, s;

	/*
	 * XXX This is very bogus.  We should handle kernel PT
	 * XXX pages much differently.
	 */

	s = splvm();
	for (bank = 0; bank < vm_nphysseg; bank++) {
		pmap_collect1(pmap_kernel(), ptoa(VM_PHYSMEM_PTR(bank)->start),
		    ptoa(VM_PHYSMEM_PTR(bank)->end));
	}
	splx(s);
}

/*
 * pmap_zero_page:		[ INTERFACE ]
 *
 *	Zero the specified (machine independent) page by mapping the page
 *	into virtual memory and using memset to clear its contents, one
 *	machine dependent page at a time.
 *
 *	Note: WE DO NOT CURRENTLY LOCK THE TEMPORARY ADDRESSES!
 *	      (Actually, we go to splvm(), and since we don't
 *	      support multiple processors, this is sufficient.)
 */
void
pmap_zero_page(paddr_t phys)
{
	int npte;

	PMAP_DPRINTF(PDB_FOLLOW, ("pmap_zero_page(%lx)\n", phys));

	npte = phys | PG_V;
#ifdef CACHE_HAVE_VAC
	if (pmap_aliasmask) {

		/*
		 * Cache-inhibit the mapping on VAC machines, as we would
		 * be wasting the cache load.
		 */

		npte |= PG_CI;
	}
#endif

#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
	if (mmutype == MMU_68040)
#endif
	{
		/*
		 * Set copyback caching on the page; this is required
		 * for cache consistency (since regular mappings are
		 * copyback as well).
		 */

		npte |= PG_CCB;
	}
#endif

	*caddr1_pte = npte;
	TBIS((vaddr_t)CADDR1);

	zeropage(CADDR1);

#ifdef DEBUG
	*caddr1_pte = PG_NV;
	TBIS((vaddr_t)CADDR1);
#endif
}

/*
 * pmap_copy_page:		[ INTERFACE ]
 *
 *	Copy the specified (machine independent) page by mapping the page
 *	into virtual memory and using memcpy to copy the page, one machine
 *	dependent page at a time.
 *
 *	Note: WE DO NOT CURRENTLY LOCK THE TEMPORARY ADDRESSES!
 *	      (Actually, we go to splvm(), and since we don't
 *	      support multiple processors, this is sufficient.)
 */
void
pmap_copy_page(paddr_t src, paddr_t dst)
{
	int npte1, npte2;

	PMAP_DPRINTF(PDB_FOLLOW, ("pmap_copy_page(%lx, %lx)\n", src, dst));

	npte1 = src | PG_RO | PG_V;
	npte2 = dst | PG_V;
#ifdef CACHE_HAVE_VAC
	if (pmap_aliasmask) {

		/*
		 * Cache-inhibit the mapping on VAC machines, as we would
		 * be wasting the cache load.
		 */

		npte1 |= PG_CI;
		npte2 |= PG_CI;
	}
#endif

#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
	if (mmutype == MMU_68040)
#endif
	{
		/*
		 * Set copyback caching on the pages; this is required
		 * for cache consistency (since regular mappings are
		 * copyback as well).
		 */

		npte1 |= PG_CCB;
		npte2 |= PG_CCB;
	}
#endif

	*caddr1_pte = npte1;
	TBIS((vaddr_t)CADDR1);

	*caddr2_pte = npte2;
	TBIS((vaddr_t)CADDR2);

	copypage(CADDR1, CADDR2);

#ifdef DEBUG
	*caddr1_pte = PG_NV;
	TBIS((vaddr_t)CADDR1);

	*caddr2_pte = PG_NV;
	TBIS((vaddr_t)CADDR2);
#endif
}

/*
 * pmap_clear_modify:		[ INTERFACE ]
 *
 *	Clear the modify bits on the specified physical page.
 */
bool
pmap_clear_modify(struct vm_page *pg)
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);

	PMAP_DPRINTF(PDB_FOLLOW, ("pmap_clear_modify(%p)\n", pg));

	return pmap_changebit(pa, 0, ~PG_M);
}

/*
 * pmap_clear_reference:	[ INTERFACE ]
 *
 *	Clear the reference bit on the specified physical page.
 */
bool
pmap_clear_reference(struct vm_page *pg)
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);

	PMAP_DPRINTF(PDB_FOLLOW, ("pmap_clear_reference(%p)\n", pg));

	return pmap_changebit(pa, 0, ~PG_U);
}

/*
 * pmap_is_referenced:		[ INTERFACE ]
 *
 *	Return whether or not the specified physical page is referenced
 *	by any physical maps.
 */
bool
pmap_is_referenced(struct vm_page *pg)
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);

	return pmap_testbit(pa, PG_U);
}

/*
 * pmap_is_modified:		[ INTERFACE ]
 *
 *	Return whether or not the specified physical page is modified
 *	by any physical maps.
 */
bool
pmap_is_modified(struct vm_page *pg)
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);

	return pmap_testbit(pa, PG_M);
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
	return m68k_ptob(ppn);
}

#ifdef CACHE_HAVE_VAC
/*
 * pmap_prefer:			[ INTERFACE ]
 *
 *	Find the first virtual address >= *vap that does not
 *	cause a virtually-addressed cache alias problem.
 */
void
pmap_prefer(vaddr_t foff, vaddr_t *vap)
{
	vaddr_t va;
	vsize_t d;

#ifdef M68K_MMU_MOTOROLA
	if (pmap_aliasmask)
#endif
	{
		va = *vap;
		d = foff - va;
		d &= pmap_aliasmask;
		*vap = va + d;
	}
}
#endif /* CACHE_HAVE_VAC */

/*
 * Miscellaneous support routines follow
 */

/*
 * pmap_remove_mapping:
 *
 *	Invalidate a single page denoted by pmap/va.
 *
 *	If (pte != NULL), it is the already computed PTE for the page.
 *
 *	If (flags & PRM_TFLUSH), we must invalidate any TLB information.
 *
 *	If (flags & PRM_CFLUSH), we must flush/invalidate any cache
 *	information.
 *
 *	If (flags & PRM_KEEPPTPAGE), we don't free the page table page
 *	if the reference drops to zero.
 */
/* static */
void
pmap_remove_mapping(pmap_t pmap, vaddr_t va, pt_entry_t *pte, int flags)
{
	paddr_t pa;
	struct pv_header *pvh;
	struct pv_entry *pv, *npv;
	struct pmap *ptpmap;
	st_entry_t *ste;
	int s, bits;
#ifdef DEBUG
	pt_entry_t opte;
#endif

	PMAP_DPRINTF(PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT,
	    ("pmap_remove_mapping(%p, %lx, %p, %x)\n",
	    pmap, va, pte, flags));

	/*
	 * PTE not provided, compute it from pmap and va.
	 */

	if (pte == NULL) {
		pte = pmap_pte(pmap, va);
		if (*pte == PG_NV)
			return;
	}

#ifdef CACHE_HAVE_VAC
	if (pmap_aliasmask && (flags & PRM_CFLUSH)) {

		/*
		 * Purge kernel side of VAC to ensure we get the correct
		 * state of any hardware maintained bits.
		 */

		DCIS();

		/*
		 * If this is a non-CI user mapping for the current process,
		 * flush the VAC.  Note that the kernel side was flushed
		 * above so we don't worry about non-CI kernel mappings.
		 */

		if (active_user_pmap(pmap) && !pmap_pte_ci(pte)) {
			DCIU();
		}
	}
#endif

	pa = pmap_pte_pa(pte);
#ifdef DEBUG
	opte = *pte;
#endif

	/*
	 * Update statistics
	 */

	if (pmap_pte_w(pte))
		pmap->pm_stats.wired_count--;
	pmap->pm_stats.resident_count--;

#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
	if (mmutype == MMU_68040)
#endif
	if ((flags & PRM_CFLUSH)) {
		DCFP(pa);
		ICPP(pa);
	}
#endif

	/*
	 * Invalidate the PTE after saving the reference modify info.
	 */

	PMAP_DPRINTF(PDB_REMOVE, ("remove: invalidating pte at %p\n", pte));
	bits = *pte & (PG_U|PG_M);
	*pte = PG_NV;
	if ((flags & PRM_TFLUSH) && active_pmap(pmap))
		TBIS(va);

	/*
	 * For user mappings decrement the wiring count on
	 * the PT page.
	 */

	if (pmap != pmap_kernel()) {
		vaddr_t ptpva = trunc_page((vaddr_t)pte);
		int refs = pmap_ptpage_delref(ptpva);
#ifdef DEBUG
		if (pmapdebug & PDB_WIRING)
			pmap_check_wiring("remove", ptpva);
#endif

		/*
		 * If reference count drops to 0, and we're not instructed
		 * to keep it around, free the PT page.
		 */

		if (refs == 0 && (flags & PRM_KEEPPTPAGE) == 0) {
#ifdef DIAGNOSTIC
			struct pv_header *ptppvh;
			struct pv_entry *ptppv;
#endif
			paddr_t ptppa;

			ptppa = pmap_pte_pa(pmap_pte(pmap_kernel(), ptpva));
#ifdef DIAGNOSTIC
			if (PAGE_IS_MANAGED(ptppa) == 0)
				panic("pmap_remove_mapping: unmanaged PT page");
			ptppvh = pa_to_pvh(ptppa);
			ptppv = &ptppvh->pvh_first;
			if (ptppv->pv_ptste == NULL)
				panic("pmap_remove_mapping: ptste == NULL");
			if (ptppv->pv_pmap != pmap_kernel() ||
			    ptppv->pv_va != ptpva ||
			    ptppv->pv_next != NULL)
				panic("pmap_remove_mapping: "
				    "bad PT page pmap %p, va 0x%lx, next %p",
				    ptppv->pv_pmap, ptppv->pv_va,
				    ptppv->pv_next);
#endif
			pmap_remove_mapping(pmap_kernel(), ptpva,
			    NULL, PRM_TFLUSH|PRM_CFLUSH);
			mutex_enter(uvm_kernel_object->vmobjlock);
			uvm_pagefree(PHYS_TO_VM_PAGE(ptppa));
			mutex_exit(uvm_kernel_object->vmobjlock);
			PMAP_DPRINTF(PDB_REMOVE|PDB_PTPAGE,
			    ("remove: PT page 0x%lx (0x%lx) freed\n",
			    ptpva, ptppa));
		}
	}

	/*
	 * If this isn't a managed page, we are all done.
	 */

	if (PAGE_IS_MANAGED(pa) == 0)
		return;

	/*
	 * Otherwise remove it from the PV table
	 * (raise IPL since we may be called at interrupt time).
	 */

	pvh = pa_to_pvh(pa);
	pv = &pvh->pvh_first;
	ste = NULL;
	s = splvm();

	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */

	if (pmap == pv->pv_pmap && va == pv->pv_va) {
		ste = pv->pv_ptste;
		ptpmap = pv->pv_ptpmap;
		npv = pv->pv_next;
		if (npv) {
			*pv = *npv;
			pmap_free_pv(npv);
		} else
			pv->pv_pmap = NULL;
	} else {
		for (npv = pv->pv_next; npv; npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && va == npv->pv_va)
				break;
			pv = npv;
		}
#ifdef DEBUG
		if (npv == NULL)
			panic("pmap_remove: PA not in pv_tab");
#endif
		ste = npv->pv_ptste;
		ptpmap = npv->pv_ptpmap;
		pv->pv_next = npv->pv_next;
		pmap_free_pv(npv);
		pvh = pa_to_pvh(pa);
		pv = &pvh->pvh_first;
	}

#ifdef CACHE_HAVE_VAC

	/*
	 * If only one mapping left we no longer need to cache inhibit
	 */

	if (pmap_aliasmask &&
	    pv->pv_pmap && pv->pv_next == NULL && (pvh->pvh_attrs & PVH_CI)) {
		PMAP_DPRINTF(PDB_CACHE,
		    ("remove: clearing CI for pa %lx\n", pa));
		pvh->pvh_attrs &= ~PVH_CI;
		pmap_changebit(pa, 0, ~PG_CI);
#ifdef DEBUG
		if ((pmapdebug & (PDB_CACHE|PDB_PVDUMP)) ==
		    (PDB_CACHE|PDB_PVDUMP))
			pmap_pvdump(pa);
#endif
	}
#endif

	/*
	 * If this was a PT page we must also remove the
	 * mapping from the associated segment table.
	 */

	if (ste) {
		PMAP_DPRINTF(PDB_REMOVE|PDB_PTPAGE,
		    ("remove: ste was %x@%p pte was %x@%p\n",
		    *ste, ste, opte, pmap_pte(pmap, va)));
#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
		if (mmutype == MMU_68040)
#endif
		{
			st_entry_t *este = &ste[NPTEPG/SG4_LEV3SIZE];

			while (ste < este)
				*ste++ = SG_NV;
#ifdef DEBUG
			ste -= NPTEPG/SG4_LEV3SIZE;
#endif
		}
#if defined(M68020) || defined(M68030)
		else
#endif
#endif
#if defined(M68020) || defined(M68030)
		*ste = SG_NV;
#endif

		/*
		 * If it was a user PT page, we decrement the
		 * reference count on the segment table as well,
		 * freeing it if it is now empty.
		 */

		if (ptpmap != pmap_kernel()) {
			PMAP_DPRINTF(PDB_REMOVE|PDB_SEGTAB,
			    ("remove: stab %p, refcnt %d\n",
			    ptpmap->pm_stab, ptpmap->pm_sref - 1));
#ifdef DEBUG
			if ((pmapdebug & PDB_PARANOIA) &&
			    ptpmap->pm_stab !=
			     (st_entry_t *)trunc_page((vaddr_t)ste))
				panic("remove: bogus ste");
#endif
			if (--(ptpmap->pm_sref) == 0) {
				PMAP_DPRINTF(PDB_REMOVE|PDB_SEGTAB,
				    ("remove: free stab %p\n",
				    ptpmap->pm_stab));
				uvm_km_free(st_map, (vaddr_t)ptpmap->pm_stab,
				    M68K_STSIZE, UVM_KMF_WIRED);
				ptpmap->pm_stab = Segtabzero;
				ptpmap->pm_stpa = Segtabzeropa;
#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
				if (mmutype == MMU_68040)
#endif
					ptpmap->pm_stfree = protostfree;
#endif

				/*
				 * XXX may have changed segment table
				 * pointer for current process so
				 * update now to reload hardware.
				 */

				if (active_user_pmap(ptpmap))
					PMAP_ACTIVATE(ptpmap, 1);
			}
		}
		pvh->pvh_attrs &= ~PVH_PTPAGE;
		ptpmap->pm_ptpages--;
	}

	/*
	 * Update saved attributes for managed page
	 */

	pvh->pvh_attrs |= bits;
	splx(s);
}

/*
 * pmap_testbit:
 *
 *	Test the modified/referenced bits of a physical page.
 */
/* static */
bool
pmap_testbit(paddr_t pa, int bit)
{
	struct pv_header *pvh;
	struct pv_entry *pv;
	pt_entry_t *pte;
	int s;

	pvh = pa_to_pvh(pa);
	pv = &pvh->pvh_first;
	s = splvm();

	/*
	 * Check saved info first
	 */

	if (pvh->pvh_attrs & bit) {
		splx(s);
		return true;
	}

#ifdef CACHE_HAVE_VAC

	/*
	 * Flush VAC to get correct state of any hardware maintained bits.
	 */

	if (pmap_aliasmask && (bit & (PG_U|PG_M)))
		DCIS();
#endif

	/*
	 * Not found.  Check current mappings, returning immediately if
	 * found.  Cache a hit to speed future lookups.
	 */

	if (pv->pv_pmap != NULL) {
		for (; pv; pv = pv->pv_next) {
			pte = pmap_pte(pv->pv_pmap, pv->pv_va);
			if (*pte & bit) {
				pvh->pvh_attrs |= bit;
				splx(s);
				return true;
			}
		}
	}
	splx(s);
	return false;
}

/*
 * pmap_changebit:
 *
 *	Change the modified/referenced bits, or other PTE bits,
 *	for a physical page.
 */
/* static */
bool
pmap_changebit(paddr_t pa, int set, int mask)
{
	struct pv_header *pvh;
	struct pv_entry *pv;
	pt_entry_t *pte, npte;
	vaddr_t va;
	int s;
#if defined(CACHE_HAVE_VAC) || defined(M68040) || defined(M68060)
	bool firstpage = true;
#endif
	bool r;

	PMAP_DPRINTF(PDB_BITS,
	    ("pmap_changebit(%lx, %x, %x)\n", pa, set, mask));

	pvh = pa_to_pvh(pa);
	pv = &pvh->pvh_first;
	s = splvm();

	/*
	 * Clear saved attributes (modify, reference)
	 */

	r = (pvh->pvh_attrs & ~mask) != 0;
	pvh->pvh_attrs &= mask;

	/*
	 * Loop over all current mappings setting/clearing as appropos
	 * If setting RO do we need to clear the VAC?
	 */

	if (pv->pv_pmap != NULL) {
#ifdef DEBUG
		int toflush = 0;
#endif
		for (; pv; pv = pv->pv_next) {
#ifdef DEBUG
			toflush |= (pv->pv_pmap == pmap_kernel()) ? 2 : 1;
#endif
			va = pv->pv_va;
			pte = pmap_pte(pv->pv_pmap, va);
#ifdef CACHE_HAVE_VAC

			/*
			 * Flush VAC to ensure we get correct state of HW bits
			 * so we don't clobber them.
			 */

			if (firstpage && pmap_aliasmask) {
				firstpage = false;
				DCIS();
			}
#endif
			npte = (*pte | set) & mask;
			if (*pte != npte) {
				r = true;
#if defined(M68040) || defined(M68060)
				/*
				 * If we are changing caching status or
				 * protection make sure the caches are
				 * flushed (but only once).
				 */
				if (firstpage &&
#if defined(M68020) || defined(M68030)
				    (mmutype == MMU_68040) &&
#endif
				    ((set == PG_RO) ||
				     (set & PG_CMASK) ||
				     (mask & PG_CMASK) == 0)) {
					firstpage = false;
					DCFP(pa);
					ICPP(pa);
				}
#endif
				*pte = npte;
				if (active_pmap(pv->pv_pmap))
					TBIS(va);
			}
		}
	}
	splx(s);
	return r;
}

/*
 * pmap_enter_ptpage:
 *
 *	Allocate and map a PT page for the specified pmap/va pair.
 */
/* static */
int
pmap_enter_ptpage(pmap_t pmap, vaddr_t va, bool can_fail)
{
	paddr_t ptpa;
	struct vm_page *pg;
	struct pv_header *pvh;
	struct pv_entry *pv;
	st_entry_t *ste;
	int s;

	PMAP_DPRINTF(PDB_FOLLOW|PDB_ENTER|PDB_PTPAGE,
	    ("pmap_enter_ptpage: pmap %p, va %lx\n", pmap, va));

	/*
	 * Allocate a segment table if necessary.  Note that it is allocated
	 * from a private map and not pt_map.  This keeps user page tables
	 * aligned on segment boundaries in the kernel address space.
	 * The segment table is wired down.  It will be freed whenever the
	 * reference count drops to zero.
	 */
	if (pmap->pm_stab == Segtabzero) {
		pmap->pm_stab = (st_entry_t *)
		    uvm_km_alloc(st_map, M68K_STSIZE, 0,
		    UVM_KMF_WIRED | UVM_KMF_ZERO |
		    (can_fail ? UVM_KMF_NOWAIT : 0));
		if (pmap->pm_stab == NULL) {
			pmap->pm_stab = Segtabzero;
			return ENOMEM;
		}
		(void) pmap_extract(pmap_kernel(), (vaddr_t)pmap->pm_stab,
		    (paddr_t *)&pmap->pm_stpa);
#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
		if (mmutype == MMU_68040)
#endif
		{
			pt_entry_t	*pte;

			pte = pmap_pte(pmap_kernel(), pmap->pm_stab);
			*pte = (*pte & ~PG_CMASK) | PG_CI;
			pmap->pm_stfree = protostfree;
		}
#endif
		/*
		 * XXX may have changed segment table pointer for current
		 * process so update now to reload hardware.
		 */
		if (active_user_pmap(pmap))
			PMAP_ACTIVATE(pmap, 1);

		PMAP_DPRINTF(PDB_ENTER|PDB_PTPAGE|PDB_SEGTAB,
		    ("enter: pmap %p stab %p(%p)\n",
		    pmap, pmap->pm_stab, pmap->pm_stpa));
	}

	ste = pmap_ste(pmap, va);
#if defined(M68040) || defined(M68060)
	/*
	 * Allocate level 2 descriptor block if necessary
	 */
#if defined(M68020) || defined(M68030)
	if (mmutype == MMU_68040)
#endif
	{
		if (*ste == SG_NV) {
			int ix;
			void *addr;

			ix = bmtol2(pmap->pm_stfree);
			if (ix == -1)
				panic("enter: out of address space"); /* XXX */
			pmap->pm_stfree &= ~l2tobm(ix);
			addr = (void *)&pmap->pm_stab[ix*SG4_LEV2SIZE];
			memset(addr, 0, SG4_LEV2SIZE*sizeof(st_entry_t));
			addr = (void *)&pmap->pm_stpa[ix*SG4_LEV2SIZE];
			*ste = (u_int)addr | SG_RW | SG_U | SG_V;

			PMAP_DPRINTF(PDB_ENTER|PDB_PTPAGE|PDB_SEGTAB,
			    ("enter: alloc ste2 %d(%p)\n", ix, addr));
		}
		ste = pmap_ste2(pmap, va);
		/*
		 * Since a level 2 descriptor maps a block of SG4_LEV3SIZE
		 * level 3 descriptors, we need a chunk of NPTEPG/SG4_LEV3SIZE
		 * (16) such descriptors (PAGE_SIZE/SG4_LEV3SIZE bytes) to map a
		 * PT page--the unit of allocation.  We set `ste' to point
		 * to the first entry of that chunk which is validated in its
		 * entirety below.
		 */
		ste = (st_entry_t *)((int)ste & ~(PAGE_SIZE/SG4_LEV3SIZE-1));

		PMAP_DPRINTF(PDB_ENTER|PDB_PTPAGE|PDB_SEGTAB,
		    ("enter: ste2 %p (%p)\n", pmap_ste2(pmap, va), ste));
	}
#endif
	va = trunc_page((vaddr_t)pmap_pte(pmap, va));

	/*
	 * In the kernel we allocate a page from the kernel PT page
	 * free list and map it into the kernel page table map (via
	 * pmap_enter).
	 */
	if (pmap == pmap_kernel()) {
		struct kpt_page *kpt;

		s = splvm();
		if ((kpt = kpt_free_list) == NULL) {
			/*
			 * No PT pages available.
			 * Try once to free up unused ones.
			 */
			PMAP_DPRINTF(PDB_COLLECT,
			    ("enter: no KPT pages, collecting...\n"));
			pmap_collect();
			if ((kpt = kpt_free_list) == NULL)
				panic("pmap_enter_ptpage: can't get KPT page");
		}
		kpt_free_list = kpt->kpt_next;
		kpt->kpt_next = kpt_used_list;
		kpt_used_list = kpt;
		ptpa = kpt->kpt_pa;
		memset((void *)kpt->kpt_va, 0, PAGE_SIZE);
		pmap_enter(pmap, va, ptpa, VM_PROT_READ | VM_PROT_WRITE,
		    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);
		pmap_update(pmap);
#ifdef DEBUG
		if (pmapdebug & (PDB_ENTER|PDB_PTPAGE)) {
			int ix = pmap_ste(pmap, va) - pmap_ste(pmap, 0);

			printf("enter: add &Sysptmap[%d]: %x (KPT page %lx)\n",
			    ix, Sysptmap[ix], kpt->kpt_va);
		}
#endif
		splx(s);
	} else {

		/*
		 * For user processes we just allocate a page from the
		 * VM system.  Note that we set the page "wired" count to 1,
		 * which is what we use to check if the page can be freed.
		 * See pmap_remove_mapping().
		 *
		 * Count the segment table reference first so that we won't
		 * lose the segment table when low on memory.
		 */

		pmap->pm_sref++;
		PMAP_DPRINTF(PDB_ENTER|PDB_PTPAGE,
		    ("enter: about to alloc UPT pg at %lx\n", va));
		mutex_enter(uvm_kernel_object->vmobjlock);
		while ((pg = uvm_pagealloc(uvm_kernel_object,
					   va - vm_map_min(kernel_map),
					   NULL, UVM_PGA_ZERO)) == NULL) {
			mutex_exit(uvm_kernel_object->vmobjlock);
			uvm_wait("ptpage");
			mutex_enter(uvm_kernel_object->vmobjlock);
		}
		mutex_exit(uvm_kernel_object->vmobjlock);
		pg->flags &= ~(PG_BUSY|PG_FAKE);
		UVM_PAGE_OWN(pg, NULL);
		ptpa = VM_PAGE_TO_PHYS(pg);
		pmap_enter(pmap_kernel(), va, ptpa,
		    VM_PROT_READ | VM_PROT_WRITE,
		    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);
		pmap_update(pmap_kernel());
	}
#if defined(M68040) || defined(M68060)
	/*
	 * Turn off copyback caching of page table pages,
	 * could get ugly otherwise.
	 */
#if defined(M68020) || defined(M68030)
	if (mmutype == MMU_68040)
#endif
	{
#ifdef DEBUG
		pt_entry_t *pte = pmap_pte(pmap_kernel(), va);
		if ((pmapdebug & PDB_PARANOIA) && (*pte & PG_CCB) == 0)
			printf("%s PT no CCB: kva=%lx ptpa=%lx pte@%p=%x\n",
			    pmap == pmap_kernel() ? "Kernel" : "User",
			    va, ptpa, pte, *pte);
#endif
		if (pmap_changebit(ptpa, PG_CI, ~PG_CCB))
			DCIS();
	}
#endif
	/*
	 * Locate the PV entry in the kernel for this PT page and
	 * record the STE address.  This is so that we can invalidate
	 * the STE when we remove the mapping for the page.
	 */
	pvh = pa_to_pvh(ptpa);
	s = splvm();
	if (pvh) {
		pv = &pvh->pvh_first;
		pvh->pvh_attrs |= PVH_PTPAGE;
		do {
			if (pv->pv_pmap == pmap_kernel() && pv->pv_va == va)
				break;
		} while ((pv = pv->pv_next));
	} else {
		pv = NULL;
	}
#ifdef DEBUG
	if (pv == NULL)
		panic("pmap_enter_ptpage: PT page not entered");
#endif
	pv->pv_ptste = ste;
	pv->pv_ptpmap = pmap;

	PMAP_DPRINTF(PDB_ENTER|PDB_PTPAGE,
	    ("enter: new PT page at PA %lx, ste at %p\n", ptpa, ste));

	/*
	 * Map the new PT page into the segment table.
	 * Also increment the reference count on the segment table if this
	 * was a user page table page.  Note that we don't use vm_map_pageable
	 * to keep the count like we do for PT pages, this is mostly because
	 * it would be difficult to identify ST pages in pmap_pageable to
	 * release them.  We also avoid the overhead of vm_map_pageable.
	 */
#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
	if (mmutype == MMU_68040)
#endif
	{
		st_entry_t *este;

		for (este = &ste[NPTEPG/SG4_LEV3SIZE]; ste < este; ste++) {
			*ste = ptpa | SG_U | SG_RW | SG_V;
			ptpa += SG4_LEV3SIZE * sizeof(st_entry_t);
		}
	}
#if defined(M68020) || defined(M68030)
	else
		*ste = (ptpa & SG_FRAME) | SG_RW | SG_V;
#endif
#else
	*ste = (ptpa & SG_FRAME) | SG_RW | SG_V;
#endif
	if (pmap != pmap_kernel()) {
		PMAP_DPRINTF(PDB_ENTER|PDB_PTPAGE|PDB_SEGTAB,
		    ("enter: stab %p refcnt %d\n",
		    pmap->pm_stab, pmap->pm_sref));
	}
	/*
	 * Flush stale TLB info.
	 */
	if (pmap == pmap_kernel())
		TBIAS();
	else
		TBIAU();
	pmap->pm_ptpages++;
	splx(s);

	return 0;
}

/*
 * pmap_ptpage_addref:
 *
 *	Add a reference to the specified PT page.
 */
void
pmap_ptpage_addref(vaddr_t ptpva)
{
	struct vm_page *pg;

	mutex_enter(uvm_kernel_object->vmobjlock);
	pg = uvm_pagelookup(uvm_kernel_object, ptpva - vm_map_min(kernel_map));
	pg->wire_count++;
	PMAP_DPRINTF(PDB_ENTER|PDB_PTPAGE|PDB_SEGTAB,
	    ("ptpage addref: pg %p now %d\n",
	     pg, pg->wire_count));
	mutex_exit(uvm_kernel_object->vmobjlock);
}

/*
 * pmap_ptpage_delref:
 *
 *	Delete a reference to the specified PT page.
 */
int
pmap_ptpage_delref(vaddr_t ptpva)
{
	struct vm_page *pg;
	int rv;

	mutex_enter(uvm_kernel_object->vmobjlock);
	pg = uvm_pagelookup(uvm_kernel_object, ptpva - vm_map_min(kernel_map));
	rv = --pg->wire_count;
	PMAP_DPRINTF(PDB_ENTER|PDB_PTPAGE|PDB_SEGTAB,
	    ("ptpage delref: pg %p now %d\n",
	     pg, pg->wire_count));
	mutex_exit(uvm_kernel_object->vmobjlock);
	return rv;
}

/*
 *	Routine:        pmap_procwr
 *
 *	Function:
 *		Synchronize caches corresponding to [addr, addr + len) in p.
 */
void
pmap_procwr(struct proc	*p, vaddr_t va, size_t len)
{

	(void)cachectl1(0x80000004, va, len, p);
}

void
_pmap_set_page_cacheable(pmap_t pmap, vaddr_t va)
{

	if (!pmap_ste_v(pmap, va))
		return;

#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
	if (mmutype == MMU_68040) {
#endif
	if (pmap_changebit(pmap_pte_pa(pmap_pte(pmap, va)), PG_CCB, ~PG_CI))
		DCIS();

#if defined(M68020) || defined(M68030)
	} else
		pmap_changebit(pmap_pte_pa(pmap_pte(pmap, va)), 0, ~PG_CI);
#endif
#else
	pmap_changebit(pmap_pte_pa(pmap_pte(pmap, va)), 0, ~PG_CI);
#endif
}

void
_pmap_set_page_cacheinhibit(pmap_t pmap, vaddr_t va)
{

	if (!pmap_ste_v(pmap, va))
		return;

#if defined(M68040) || defined(M68060)
#if defined(M68020) || defined(M68030)
	if (mmutype == MMU_68040) {
#endif
	if (pmap_changebit(pmap_pte_pa(pmap_pte(pmap, va)), PG_CI, ~PG_CCB))
		DCIS();
#if defined(M68020) || defined(M68030)
	} else
		pmap_changebit(pmap_pte_pa(pmap_pte(pmap, va)), PG_CI, ~0);
#endif
#else
	pmap_changebit(pmap_pte_pa(pmap_pte(pmap, va)), PG_CI, ~0);
#endif
}

int
_pmap_page_is_cacheable(pmap_t pmap, vaddr_t va)
{

	if (!pmap_ste_v(pmap, va))
		return 0;

	return (pmap_pte_ci(pmap_pte(pmap, va)) == 0) ? 1 : 0;
}

#ifdef DEBUG
/*
 * pmap_pvdump:
 *
 *	Dump the contents of the PV list for the specified physical page.
 */
void
pmap_pvdump(paddr_t pa)
{
	struct pv_header *pvh;
	struct pv_entry *pv;

	printf("pa %lx", pa);
	pvh = pa_to_pvh(pa);
	for (pv = &pvh->pvh_first; pv; pv = pv->pv_next)
		printf(" -> pmap %p, va %lx, ptste %p, ptpmap %p",
		    pv->pv_pmap, pv->pv_va, pv->pv_ptste, pv->pv_ptpmap);
	printf("\n");
}

/*
 * pmap_check_wiring:
 *
 *	Count the number of valid mappings in the specified PT page,
 *	and ensure that it is consistent with the number of wirings
 *	to that page that the VM system has.
 */
void
pmap_check_wiring(const char *str, vaddr_t va)
{
	pt_entry_t *pte;
	paddr_t pa;
	struct vm_page *pg;
	int count;

	if (!pmap_ste_v(pmap_kernel(), va) ||
	    !pmap_pte_v(pmap_pte(pmap_kernel(), va)))
		return;

	pa = pmap_pte_pa(pmap_pte(pmap_kernel(), va));
	pg = PHYS_TO_VM_PAGE(pa);
	if (pg->wire_count > PAGE_SIZE / sizeof(pt_entry_t)) {
		panic("*%s*: 0x%lx: wire count %d", str, va, pg->wire_count);
	}

	count = 0;
	for (pte = (pt_entry_t *)va; pte < (pt_entry_t *)(va + PAGE_SIZE);
	     pte++)
		if (*pte)
			count++;
	if (pg->wire_count != count)
		panic("*%s*: 0x%lx: w%d/a%d",
		       str, va, pg->wire_count, count);
}
#endif /* DEBUG */
