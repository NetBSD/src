/* $NetBSD: pmap.h,v 1.99 2022/07/19 22:04:14 riastradh Exp $ */

/*-
 * Copyright (c) 1998, 1999, 2000, 2001, 2007 The NetBSD Foundation, Inc.
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
 *	@(#)pmap.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Copyright (c) 1987 Carnegie-Mellon University
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
 *	@(#)pmap.h	8.1 (Berkeley) 6/10/93
 */

#ifndef	_PMAP_MACHINE_
#define	_PMAP_MACHINE_

#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/mutex.h>
#include <sys/queue.h>

#include <machine/pte.h>

/*
 * Machine-dependent virtual memory state.
 *
 * If we ever support processor numbers higher than 63, we'll have to
 * rethink the CPU mask.
 *
 * Note pm_asn and pm_asngen are arrays allocated in pmap_create().
 * Their size is based on the PCS count from the HWRPB, and indexed
 * by processor ID (from `whami').  This is all padded to COHERENCY_UNIT
 * to avoid false sharing.
 *
 * The kernel pmap is a special case; since the kernel uses only ASM
 * mappings and uses a reserved ASN to keep the TLB clean, we don't
 * allocate any ASN info for the kernel pmap at all.
 * arrays which hold enough for ALPHA_MAXPROCS.
 */

LIST_HEAD(pmap_pagelist, vm_page);
LIST_HEAD(pmap_pvlist, pv_entry);

struct pmap_percpu {
	unsigned int		pmc_asn;	/* address space number */
	unsigned int		pmc_pad0;
	unsigned long		pmc_asngen;	/* ASN generation number */
	unsigned int		pmc_needisync;	/* CPU needes isync */
	unsigned int		pmc_pad1;
	pt_entry_t		*pmc_lev1map;	/* level 1 map */
	unsigned long		pmc_padN[(COHERENCY_UNIT / 8) - 4];
};

struct pmap {	/* pmaps are aligned to COHERENCY_UNIT boundaries */
		/* pmaps are locked by hashed mutexes */
	unsigned long		pm_cpus;	/* [ 0] CPUs using pmap */
	struct pmap_statistics	pm_stats;	/* [ 8] statistics */
	unsigned int		pm_count;	/* [24] reference count */
	unsigned int		__pm_spare0;	/* [28] spare field */
	struct pmap_pagelist	pm_ptpages;	/* [32] list of PT pages */
	struct pmap_pvlist	pm_pvents;	/* [40] list of PV entries */
	TAILQ_ENTRY(pmap)	pm_list;	/* [48] list of all pmaps */
	/* -- COHERENCY_UNIT boundary -- */
	struct pmap_percpu	pm_percpu[];	/* [64] per-CPU data */
			/*	variable length		*/
};

#define	PMAP_SIZEOF(x)							\
	(ALIGN(offsetof(struct pmap, pm_percpu[(x)])))

#define	PMAP_ASN_KERNEL		0	/* kernel-reserved ASN */
#define	PMAP_ASN_FIRST_USER	1	/* first user ASN */
#define	PMAP_ASNGEN_INVALID	0	/* reserved (invalid) ASN generation */
#define	PMAP_ASNGEN_INITIAL	1	/* first valid generatation */

/*
 * For each struct vm_page, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 */
typedef struct pv_entry {
	struct pv_entry	*pv_next;	/* next pv_entry on page list */
	LIST_ENTRY(pv_entry) pv_link;	/* link on owning pmap's list */
	struct pmap	*pv_pmap;	/* pmap where mapping lies */
	vaddr_t		pv_va;		/* virtual address for mapping */
	pt_entry_t	*pv_pte;	/* PTE that maps the VA */
} *pv_entry_t;

/* attrs in pvh_listx */
#define	PGA_MODIFIED		0x01UL		/* modified */
#define	PGA_REFERENCED		0x02UL		/* referenced */
#define	PGA_ATTRS		(PGA_MODIFIED | PGA_REFERENCED)

/* pvh_usage */
#define	PGU_NORMAL		0		/* free or normal use */
#define	PGU_PVENT		1		/* PV entries */
#define	PGU_L1PT		2		/* level 1 page table */
#define	PGU_L2PT		3		/* level 2 page table */
#define	PGU_L3PT		4		/* level 3 page table */

#ifdef _KERNEL

#include <sys/atomic.h>

struct cpu_info;
struct trapframe;

void	pmap_init_cpu(struct cpu_info *);
#if defined(MULTIPROCESSOR)
void	pmap_tlb_shootdown_ipi(struct cpu_info *, struct trapframe *);
#endif /* MULTIPROCESSOR */

#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

#define	pmap_copy(dp, sp, da, l, sa)	/* nothing */
#define	pmap_update(pmap)		/* nothing (yet) */

#define	pmap_is_referenced(pg)						\
	(((pg)->mdpage.pvh_listx & PGA_REFERENCED) != 0)
#define	pmap_is_modified(pg)						\
	(((pg)->mdpage.pvh_listx & PGA_MODIFIED) != 0)

#define	PMAP_STEAL_MEMORY		/* enable pmap_steal_memory() */
#define	PMAP_GROWKERNEL			/* enable pmap_growkernel() */

#define	PMAP_DIRECT
#define	PMAP_DIRECT_MAP(pa)		ALPHA_PHYS_TO_K0SEG((pa))
#define	PMAP_DIRECT_UNMAP(va)		ALPHA_K0SEG_TO_PHYS((va))

static __inline int
pmap_direct_process(paddr_t pa, voff_t pgoff, size_t len,
    int (*process)(void *, size_t, void *), void *arg)
{
	vaddr_t va = PMAP_DIRECT_MAP(pa);

	return process((void *)(va + pgoff), len, arg);
}

/*
 * Alternate mapping hooks for pool pages.  Avoids thrashing the TLB.
 */
#define	PMAP_MAP_POOLPAGE(pa)		PMAP_DIRECT_MAP(pa)
#define	PMAP_UNMAP_POOLPAGE(va)		PMAP_DIRECT_UNMAP(va)

/*
 * Other hooks for the pool allocator.
 */
#define	POOL_VTOPHYS(va)		ALPHA_K0SEG_TO_PHYS((vaddr_t) (va))

bool	pmap_pageidlezero(paddr_t);
#define	PMAP_PAGEIDLEZERO(pa)	pmap_pageidlezero((pa))

paddr_t vtophys(vaddr_t);

/* Machine-specific functions. */
void	pmap_bootstrap(paddr_t, u_int, u_long);
int	pmap_emulate_reference(struct lwp *, vaddr_t, int, int);

#define	pmap_pte_pa(pte)	(PG_PFNUM(*(pte)) << PGSHIFT)
#define	pmap_pte_prot(pte)	(*(pte) & PG_PROT)
#define	pmap_pte_w(pte)		(*(pte) & PG_WIRED)
#define	pmap_pte_v(pte)		(*(pte) & PG_V)
#define	pmap_pte_pv(pte)	(*(pte) & PG_PVLIST)
#define	pmap_pte_asm(pte)	(*(pte) & PG_ASM)
#define	pmap_pte_exec(pte)	(*(pte) & PG_EXEC)

#define	pmap_pte_set_w(pte, v)						\
do {									\
	if (v)								\
		*(pte) |= PG_WIRED;					\
	else								\
		*(pte) &= ~PG_WIRED;					\
} while (0)

#define	pmap_pte_w_chg(pte, nw)	((nw) ^ pmap_pte_w(pte))

#define	pmap_pte_set_prot(pte, np)					\
do {									\
	*(pte) &= ~PG_PROT;						\
	*(pte) |= (np);							\
} while (0)

#define	pmap_pte_prot_chg(pte, np) ((np) ^ pmap_pte_prot(pte))

static __inline pt_entry_t *
pmap_lev1map(pmap_t pmap)
{
	if (__predict_false(pmap == pmap_kernel())) {
		return kernel_lev1map;
	}
	/*
	 * We're just reading a per-CPU field that's the same on
	 * all CPUs, so don't bother disabling preemption around
	 * this.
	 */
	return pmap->pm_percpu[cpu_number()].pmc_lev1map;
}

static __inline pt_entry_t *
pmap_l1pte(pt_entry_t *lev1map, vaddr_t v)
{
	KASSERT(lev1map != NULL);
	return &lev1map[l1pte_index(v)];
}

static __inline pt_entry_t *
pmap_l2pte(pt_entry_t *lev1map, vaddr_t v, pt_entry_t *l1pte)
{
	pt_entry_t *lev2map;

	if (l1pte == NULL) {
		l1pte = pmap_l1pte(lev1map, v);
		if (pmap_pte_v(l1pte) == 0)
			return NULL;
	}

	lev2map = (pt_entry_t *)ALPHA_PHYS_TO_K0SEG(pmap_pte_pa(l1pte));
	return &lev2map[l2pte_index(v)];
}

static __inline pt_entry_t *
pmap_l3pte(pt_entry_t *lev1map, vaddr_t v, pt_entry_t *l2pte)
{
	pt_entry_t *l1pte, *lev2map, *lev3map;

	if (l2pte == NULL) {
		l1pte = pmap_l1pte(lev1map, v);
		if (pmap_pte_v(l1pte) == 0)
			return NULL;

		lev2map = (pt_entry_t *)ALPHA_PHYS_TO_K0SEG(pmap_pte_pa(l1pte));
		l2pte = &lev2map[l2pte_index(v)];
		if (pmap_pte_v(l2pte) == 0)
			return NULL;
	}

	lev3map = (pt_entry_t *)ALPHA_PHYS_TO_K0SEG(pmap_pte_pa(l2pte));
	return &lev3map[l3pte_index(v)];
}

/*
 * Macro for processing deferred I-stream synchronization.
 *
 * The pmap module may defer syncing the user I-stream until the
 * return to userspace, since the IMB PALcode op can be quite
 * expensive.  Since user instructions won't be executed until
 * the return to userspace, this can be deferred until userret().
 */
#define	PMAP_USERRET(pmap)						\
do {									\
	const unsigned long cpu_id = cpu_number();			\
									\
	if ((pmap)->pm_percpu[cpu_id].pmc_needisync) {			\
		(pmap)->pm_percpu[cpu_id].pmc_needisync = 0;		\
		alpha_pal_imb();					\
	}								\
} while (0)

/*
 * pmap-specific data store in the vm_page structure.
 */
#define	__HAVE_VM_PAGE_MD
struct vm_page_md {
	uintptr_t pvh_listx;		/* pv_entry list + attrs */
	/*
	 * XXX These fields are only needed for pages that are used
	 * as PT pages.  It would be nice to find safely-unused fields
	 * in the vm_page structure that could be used instead.
	 *
	 * (Only 11 bits are needed ... we need to be able to count from
	 * 0-1025 ... 1025 because sometimes we need to take an extra
	 * reference temporarily in pmap_enter().)
	 */
	unsigned int pvh_physpgrefs;	/* # refs as a PT page */
	unsigned int pvh_spare0;	/* XXX spare field */
};

/* Reference counting for page table pages. */
#define	PHYSPAGE_REFCNT(pg)						\
	atomic_load_relaxed(&(pg)->mdpage.pvh_physpgrefs)
#define	PHYSPAGE_REFCNT_SET(pg, v)					\
	atomic_store_relaxed(&(pg)->mdpage.pvh_physpgrefs, (v))
#define	PHYSPAGE_REFCNT_INC(pg)						\
	atomic_inc_uint_nv(&(pg)->mdpage.pvh_physpgrefs)
#define	PHYSPAGE_REFCNT_DEC(pg)						\
	atomic_dec_uint_nv(&(pg)->mdpage.pvh_physpgrefs)

#define	VM_MDPAGE_PVS(pg)						\
	((struct pv_entry *)((pg)->mdpage.pvh_listx & ~3UL))

#define	VM_MDPAGE_INIT(pg)						\
do {									\
	(pg)->mdpage.pvh_listx = 0UL;					\
} while (/*CONSTCOND*/0)

#endif /* _KERNEL */

#endif /* _PMAP_MACHINE_ */
