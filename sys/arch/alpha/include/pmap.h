/* $NetBSD: pmap.h,v 1.79 2014/01/01 16:09:04 matt Exp $ */

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
 * by processor ID (from `whami').
 *
 * The kernel pmap is a special case; it gets statically-allocated
 * arrays which hold enough for ALPHA_MAXPROCS.
 */
struct pmap_asn_info {
	unsigned int		pma_asn;	/* address space number */
	unsigned long		pma_asngen;	/* ASN generation number */
};

struct pmap {
	TAILQ_ENTRY(pmap)	pm_list;	/* list of all pmaps */
	pt_entry_t		*pm_lev1map;	/* level 1 map */
	int			pm_count;	/* pmap reference count */
	kmutex_t		pm_lock;	/* lock on pmap */
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	unsigned long		pm_cpus;	/* mask of CPUs using pmap */
	unsigned long		pm_needisync;	/* mask of CPUs needing isync */
	struct pmap_asn_info	pm_asni[];	/* ASN information */
			/*	variable length		*/
};

/*
 * Compute the sizeof of a pmap structure.
 */
#define	PMAP_SIZEOF(x)							\
	(ALIGN(offsetof(struct pmap, pm_asni[(x)])))

#define	PMAP_ASN_RESERVED	0	/* reserved for Lev1map users */

/*
 * For each struct vm_page, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 */
typedef struct pv_entry {
	struct pv_entry	*pv_next;	/* next pv_entry on list */
	struct pmap	*pv_pmap;	/* pmap where mapping lies */
	vaddr_t		pv_va;		/* virtual address for mapping */
	pt_entry_t	*pv_pte;	/* PTE that maps the VA */
} *pv_entry_t;

/* pvh_attrs */
#define	PGA_MODIFIED		0x01		/* modified */
#define	PGA_REFERENCED		0x02		/* referenced */

/* pvh_usage */
#define	PGU_NORMAL		0		/* free or normal use */
#define	PGU_PVENT		1		/* PV entries */
#define	PGU_L1PT		2		/* level 1 page table */
#define	PGU_L2PT		3		/* level 2 page table */
#define	PGU_L3PT		4		/* level 3 page table */

#ifdef _KERNEL

#include <sys/atomic.h>

#ifdef _KERNEL_OPT
#include "opt_dec_kn8ae.h"			/* XXX */
#if defined(DEC_KN8AE)
#define	_PMAP_MAY_USE_PROM_CONSOLE
#endif
#else
#define	_PMAP_MAY_USE_PROM_CONSOLE
#endif

#ifndef _LKM
#if defined(MULTIPROCESSOR)
struct cpu_info;
struct trapframe;

void	pmap_tlb_shootdown(pmap_t, vaddr_t, pt_entry_t, u_long *);
void	pmap_tlb_shootnow(u_long);
void	pmap_do_tlb_shootdown(struct cpu_info *, struct trapframe *);
#define	PMAP_TLB_SHOOTDOWN_CPUSET_DECL		u_long shootset = 0;
#define	PMAP_TLB_SHOOTDOWN(pm, va, pte)					\
	pmap_tlb_shootdown((pm), (va), (pte), &shootset)
#define	PMAP_TLB_SHOOTNOW()						\
	pmap_tlb_shootnow(shootset)
#else
#define	PMAP_TLB_SHOOTDOWN_CPUSET_DECL		/* nothing */
#define	PMAP_TLB_SHOOTDOWN(pm, va, pte)		/* nothing */
#define	PMAP_TLB_SHOOTNOW()			/* nothing */
#endif /* MULTIPROCESSOR */
#endif /* _LKM */

#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

#define	pmap_copy(dp, sp, da, l, sa)	/* nothing */
#define	pmap_update(pmap)		/* nothing (yet) */

static __inline void
pmap_remove_all(struct pmap *pmap)
{
	/* Nothing. */
}

#define	pmap_is_referenced(pg)						\
	(((pg)->mdpage.pvh_attrs & PGA_REFERENCED) != 0)
#define	pmap_is_modified(pg)						\
	(((pg)->mdpage.pvh_attrs & PGA_MODIFIED) != 0)

#define	PMAP_STEAL_MEMORY		/* enable pmap_steal_memory() */
#define	PMAP_GROWKERNEL			/* enable pmap_growkernel() */

/*
 * Alternate mapping hooks for pool pages.  Avoids thrashing the TLB.
 */
#define	PMAP_MAP_POOLPAGE(pa)		ALPHA_PHYS_TO_K0SEG((pa))
#define	PMAP_UNMAP_POOLPAGE(va)		ALPHA_K0SEG_TO_PHYS((va))

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
#ifdef _PMAP_MAY_USE_PROM_CONSOLE
int	pmap_uses_prom_console(void);
#endif

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

static __inline pt_entry_t *pmap_l2pte(pmap_t, vaddr_t, pt_entry_t *);
static __inline pt_entry_t *pmap_l3pte(pmap_t, vaddr_t, pt_entry_t *);

#define	pmap_l1pte(pmap, v)						\
	(&(pmap)->pm_lev1map[l1pte_index((vaddr_t)(v))])

static __inline pt_entry_t *
pmap_l2pte(pmap_t pmap, vaddr_t v, pt_entry_t *l1pte)
{
	pt_entry_t *lev2map;

	if (l1pte == NULL) {
		l1pte = pmap_l1pte(pmap, v);
		if (pmap_pte_v(l1pte) == 0)
			return (NULL);
	}

	lev2map = (pt_entry_t *)ALPHA_PHYS_TO_K0SEG(pmap_pte_pa(l1pte));
	return (&lev2map[l2pte_index(v)]);
}

static __inline pt_entry_t *
pmap_l3pte(pmap_t pmap, vaddr_t v, pt_entry_t *l2pte)
{
	pt_entry_t *l1pte, *lev2map, *lev3map;

	if (l2pte == NULL) {
		l1pte = pmap_l1pte(pmap, v);
		if (pmap_pte_v(l1pte) == 0)
			return (NULL);

		lev2map = (pt_entry_t *)ALPHA_PHYS_TO_K0SEG(pmap_pte_pa(l1pte));
		l2pte = &lev2map[l2pte_index(v)];
		if (pmap_pte_v(l2pte) == 0)
			return (NULL);
	}

	lev3map = (pt_entry_t *)ALPHA_PHYS_TO_K0SEG(pmap_pte_pa(l2pte));
	return (&lev3map[l3pte_index(v)]);
}

/*
 * Macros for locking pmap structures.
 *
 * Note that we if we access the kernel pmap in interrupt context, it
 * is only to update statistics.  Since stats are updated using atomic
 * operations, locking the kernel pmap is not necessary.  Therefore,
 * it is not necessary to block interrupts when locking pmap strucutres.
 */
#define	PMAP_LOCK(pmap)		mutex_enter(&(pmap)->pm_lock)
#define	PMAP_UNLOCK(pmap)	mutex_exit(&(pmap)->pm_lock)

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
	u_long cpu_mask = (1UL << cpu_number());			\
									\
	if ((pmap)->pm_needisync & cpu_mask) {				\
		atomic_and_ulong(&(pmap)->pm_needisync,	~cpu_mask);	\
		alpha_pal_imb();					\
	}								\
} while (0)

/*
 * pmap-specific data store in the vm_page structure.
 */
#define	__HAVE_VM_PAGE_MD
struct vm_page_md {
	struct pv_entry *pvh_list;		/* pv_entry list */
	int pvh_attrs;				/* page attributes */
	unsigned pvh_refcnt;
};

#define	VM_MDPAGE_INIT(pg)						\
do {									\
	(pg)->mdpage.pvh_list = NULL;					\
	(pg)->mdpage.pvh_refcnt = 0;					\
} while (/*CONSTCOND*/0)

#endif /* _KERNEL */

#endif /* _PMAP_MACHINE_ */
