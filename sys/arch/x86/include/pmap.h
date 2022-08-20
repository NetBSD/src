/*	$NetBSD: pmap.h,v 1.133 2022/08/20 23:48:50 riastradh Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * pmap.h: see pmap.c for the history of this pmap module.
 */

#ifndef _X86_PMAP_H_
#define	_X86_PMAP_H_

#if defined(_KERNEL)
#include <sys/kcpuset.h>
#include <sys/rwlock.h>
#include <x86/pmap_pv.h>
#include <uvm/pmap/pmap_pvt.h>
#include <uvm/uvm_object.h>

/*
 * the pmap structure
 *
 * note that the pm_obj contains the lock pointer, the reference count,
 * page list, and number of PTPs within the pmap.
 *
 * pm_lock is the same as the lock for vm object 0.  Changes to
 * the other objects may only be made if that lock has been taken
 * (the other object locks are only used when uvm_pagealloc is called)
 */

struct pv_page;

struct pmap {
	struct uvm_object pm_obj[PTP_LEVELS-1];/* objects for lvl >= 1) */
	LIST_ENTRY(pmap) pm_list;	/* list of all pmaps */
	pd_entry_t *pm_pdir;		/* VA of PD */
	paddr_t pm_pdirpa[PDP_SIZE];	/* PA of PDs (read-only after create) */
	struct vm_page *pm_ptphint[PTP_LEVELS-1];
					/* pointer to a PTP in our pmap */
	struct pmap_statistics pm_stats;  /* pmap stats */
	struct pv_entry *pm_pve;	/* spare pv_entry */
	LIST_HEAD(, pv_page) pm_pvp_part;
	LIST_HEAD(, pv_page) pm_pvp_empty;
	LIST_HEAD(, pv_page) pm_pvp_full;

#if !defined(__x86_64__)
	vaddr_t pm_hiexec;		/* highest executable mapping */
#endif /* !defined(__x86_64__) */

	union descriptor *pm_ldt;	/* user-set LDT */
	size_t pm_ldt_len;		/* XXX unused, remove */
	int pm_ldt_sel;			/* LDT selector */

	kcpuset_t *pm_cpus;		/* mask of CPUs using pmap */
	kcpuset_t *pm_kernel_cpus;	/* mask of CPUs using kernel part
					 of pmap */
	kcpuset_t *pm_xen_ptp_cpus;	/* mask of CPUs which have this pmap's
					 ptp mapped */
	uint64_t pm_ncsw;		/* for assertions */
	LIST_HEAD(,vm_page) pm_gc_ptp;	/* PTPs queued for free */

	/* Used by NVMM and Xen */
	int (*pm_enter)(struct pmap *, vaddr_t, paddr_t, vm_prot_t, u_int);
	bool (*pm_extract)(struct pmap *, vaddr_t, paddr_t *);
	void (*pm_remove)(struct pmap *, vaddr_t, vaddr_t);
	int (*pm_sync_pv)(struct vm_page *, vaddr_t, paddr_t, int, uint8_t *,
	    pt_entry_t *);
	void (*pm_pp_remove_ent)(struct pmap *, struct vm_page *, pt_entry_t,
	    vaddr_t);
	void (*pm_write_protect)(struct pmap *, vaddr_t, vaddr_t, vm_prot_t);
	void (*pm_unwire)(struct pmap *, vaddr_t);

	void (*pm_tlb_flush)(struct pmap *);
	void *pm_data;

	kmutex_t pm_lock		/* locks for pm_objs */
	    __aligned(64);		/* give lock own cache line */
	krwlock_t pm_dummy_lock;	/* ugly hack for abusing uvm_object */
};

/*
 * MD flags that we use for pmap_enter and pmap_kenter_pa:
 */

/*
 * macros
 */

#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

#define pmap_clear_modify(pg)		pmap_clear_attrs(pg, PP_ATTRS_D)
#define pmap_clear_reference(pg)	pmap_clear_attrs(pg, PP_ATTRS_A)
#define pmap_copy(DP,SP,D,L,S)		__USE(L)
#define pmap_is_modified(pg)		pmap_test_attrs(pg, PP_ATTRS_D)
#define pmap_is_referenced(pg)		pmap_test_attrs(pg, PP_ATTRS_A)
#define pmap_move(DP,SP,D,L,S)
#define pmap_phys_address(ppn)		(x86_ptob(ppn) & ~X86_MMAP_FLAG_MASK)
#define pmap_mmap_flags(ppn)		x86_mmap_flags(ppn)

#if defined(__x86_64__) || defined(PAE)
#define X86_MMAP_FLAG_SHIFT	(64 - PGSHIFT)
#else
#define X86_MMAP_FLAG_SHIFT	(32 - PGSHIFT)
#endif

#define X86_MMAP_FLAG_MASK	0xf
#define X86_MMAP_FLAG_PREFETCH	0x1

/*
 * prototypes
 */

void		pmap_activate(struct lwp *);
void		pmap_bootstrap(vaddr_t);
bool		pmap_clear_attrs(struct vm_page *, unsigned);
bool		pmap_pv_clear_attrs(paddr_t, unsigned);
void		pmap_deactivate(struct lwp *);
void		pmap_page_remove(struct vm_page *);
void		pmap_pv_remove(paddr_t);
void		pmap_remove(struct pmap *, vaddr_t, vaddr_t);
bool		pmap_test_attrs(struct vm_page *, unsigned);
void		pmap_write_protect(struct pmap *, vaddr_t, vaddr_t, vm_prot_t);
void		pmap_load(void);
paddr_t		pmap_init_tmp_pgtbl(paddr_t);
bool		pmap_remove_all(struct pmap *);
void		pmap_ldt_cleanup(struct lwp *);
void		pmap_ldt_sync(struct pmap *);
void		pmap_kremove_local(vaddr_t, vsize_t);

#define	__HAVE_PMAP_PV_TRACK	1
void		pmap_pv_init(void);
void		pmap_pv_track(paddr_t, psize_t);
void		pmap_pv_untrack(paddr_t, psize_t);

u_int		x86_mmap_flags(paddr_t);

#define PMAP_GROWKERNEL		/* turn on pmap_growkernel interface */
#define PMAP_FORK		/* turn on pmap_fork interface */

/*
 * inline functions
 */

/*
 * pmap_page_protect: change the protection of all recorded mappings
 *	of a managed page
 *
 * => this function is a frontend for pmap_page_remove/pmap_clear_attrs
 * => we only have to worry about making the page more protected.
 *	unprotecting a page is done on-demand at fault time.
 */

__inline static void __unused
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	if ((prot & VM_PROT_WRITE) == 0) {
		if (prot & (VM_PROT_READ|VM_PROT_EXECUTE)) {
			(void)pmap_clear_attrs(pg, PP_ATTRS_W);
		} else {
			pmap_page_remove(pg);
		}
	}
}

/*
 * pmap_pv_protect: change the protection of all recorded mappings
 *	of an unmanaged page
 */

__inline static void __unused
pmap_pv_protect(paddr_t pa, vm_prot_t prot)
{
	if ((prot & VM_PROT_WRITE) == 0) {
		if (prot & (VM_PROT_READ|VM_PROT_EXECUTE)) {
			(void)pmap_pv_clear_attrs(pa, PP_ATTRS_W);
		} else {
			pmap_pv_remove(pa);
		}
	}
}

/*
 * pmap_protect: change the protection of pages in a pmap
 *
 * => this function is a frontend for pmap_remove/pmap_write_protect
 * => we only have to worry about making the page more protected.
 *	unprotecting a page is done on-demand at fault time.
 */

__inline static void __unused
pmap_protect(struct pmap *pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	if ((prot & VM_PROT_WRITE) == 0) {
		if (prot & (VM_PROT_READ|VM_PROT_EXECUTE)) {
			pmap_write_protect(pmap, sva, eva, prot);
		} else {
			pmap_remove(pmap, sva, eva);
		}
	}
}

paddr_t vtophys(vaddr_t);
vaddr_t	pmap_map(vaddr_t, paddr_t, paddr_t, vm_prot_t);
void	pmap_cpu_init_late(struct cpu_info *);

/* pmap functions with machine addresses */
void	pmap_kenter_ma(vaddr_t, paddr_t, vm_prot_t, u_int);
int	pmap_enter_ma(struct pmap *, vaddr_t, paddr_t, paddr_t,
	    vm_prot_t, u_int, int);
bool	pmap_extract_ma(pmap_t, vaddr_t, paddr_t *);

paddr_t pmap_get_physpage(void);

/*
 * Hooks for the pool allocator.
 */
#define	POOL_VTOPHYS(va)	vtophys((vaddr_t) (va))

#ifdef __HAVE_DIRECT_MAP

extern vaddr_t pmap_direct_base;
extern vaddr_t pmap_direct_end;

#define PMAP_DIRECT_BASE	pmap_direct_base
#define PMAP_DIRECT_END		pmap_direct_end

#define PMAP_DIRECT_MAP(pa)	((vaddr_t)PMAP_DIRECT_BASE + (pa))
#define PMAP_DIRECT_UNMAP(va)	((paddr_t)(va) - PMAP_DIRECT_BASE)

/*
 * Alternate mapping hooks for pool pages.
 */
#define PMAP_MAP_POOLPAGE(pa)	PMAP_DIRECT_MAP((pa))
#define PMAP_UNMAP_POOLPAGE(va)	PMAP_DIRECT_UNMAP((va))

#endif /* __HAVE_DIRECT_MAP */

#define	__HAVE_VM_PAGE_MD
#define	VM_MDPAGE_INIT(pg) \
	memset(&(pg)->mdpage, 0, sizeof((pg)->mdpage)); \
	PMAP_PAGE_INIT(&(pg)->mdpage.mp_pp)

struct vm_page_md {
	struct pmap_page mp_pp;
};

#endif /* _KERNEL */

#endif /* _X86_PMAP_H_ */
