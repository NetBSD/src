/*	$NetBSD: pmap.h,v 1.13 2008/02/07 00:36:57 matt Exp $	*/

/*-
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_POWERPC_OEA_PMAP_H_
#define	_POWERPC_OEA_PMAP_H_

#include "opt_ppcarch.h"
#include <powerpc/oea/pte.h>

#ifndef _LOCORE
/*
 * Pmap stuff
 */
struct pmap {
#ifdef PPC_OEA64
	struct steg *pm_steg_table;		/* segment table pointer */
	/* XXX need way to track exec pages */
#endif

#if defined(PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	register_t pm_sr[16];			/* segments used in this pmap */
	int pm_exec[16];			/* counts of exec mappings */
#endif
	register_t pm_vsid;			/* VSID bits */
	int pm_refs;				/* ref count */
	struct pmap_statistics pm_stats;	/* pmap statistics */
	unsigned int pm_evictions;		/* pvo's not in page table */

#ifdef PPC_OEA64
	unsigned int pm_ste_evictions;
#endif
};

typedef	struct pmap *pmap_t;

struct pmap_ops {
	int (*pmapop_pte_spill)(struct pmap *, vaddr_t, bool);
	void (*pmapop_real_memory)(paddr_t *, psize_t *);
	void (*pmapop_init)(void);
	void (*pmapop_virtual_space)(vaddr_t *, vaddr_t *);
	pmap_t (*pmapop_create)(void);
	void (*pmapop_reference)(pmap_t);
	void (*pmapop_destroy)(pmap_t);
	void (*pmapop_copy)(pmap_t, pmap_t, vaddr_t, vsize_t, vaddr_t);
	void (*pmapop_update)(pmap_t);
	void (*pmapop_collect)(pmap_t);
	int (*pmapop_enter)(pmap_t, vaddr_t, paddr_t, vm_prot_t, int);
	void (*pmapop_remove)(pmap_t, vaddr_t, vaddr_t);
	void (*pmapop_kenter_pa)(vaddr_t, paddr_t, vm_prot_t);
	void (*pmapop_kremove)(vaddr_t, vsize_t);
	bool (*pmapop_extract)(pmap_t, vaddr_t, paddr_t *);

	void (*pmapop_protect)(pmap_t, vaddr_t, vaddr_t, vm_prot_t);
	void (*pmapop_unwire)(pmap_t, vaddr_t);
	void (*pmapop_page_protect)(struct vm_page *, vm_prot_t);
	bool (*pmapop_query_bit)(struct vm_page *, int);
	bool (*pmapop_clear_bit)(struct vm_page *, int);

	void (*pmapop_activate)(struct lwp *);
	void (*pmapop_deactivate)(struct lwp *);

	void (*pmapop_pinit)(pmap_t);
	void (*pmapop_procwr)(struct proc *, vaddr_t, size_t);

	void (*pmapop_pte_print)(volatile struct pte *);
	void (*pmapop_pteg_check)(void);
	void (*pmapop_print_mmuregs)(void);
	void (*pmapop_print_pte)(pmap_t, vaddr_t);
	void (*pmapop_pteg_dist)(void);
	void (*pmapop_pvo_verify)(void);
	vaddr_t (*pmapop_steal_memory)(vsize_t, vaddr_t *, vaddr_t *);
	void (*pmapop_bootstrap)(paddr_t, paddr_t);
};

#ifdef	_KERNEL
#include <sys/cdefs.h>
__BEGIN_DECLS
#include <sys/param.h>
#include <sys/systm.h>

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
extern register_t iosrtable[];
#endif
extern int pmap_use_altivec;
extern struct pmap kernel_pmap_;
#define	pmap_kernel()	(&kernel_pmap_)

#define pmap_clear_modify(pg)		(pmap_clear_bit((pg), PTE_CHG))
#define	pmap_clear_reference(pg)	(pmap_clear_bit((pg), PTE_REF))
#define	pmap_is_modified(pg)		(pmap_query_bit((pg), PTE_CHG))
#define	pmap_is_referenced(pg)		(pmap_query_bit((pg), PTE_REF))

#define	pmap_phys_address(x)		(x)

#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

/* ARGSUSED */
static inline void
pmap_remove_all(struct pmap *pmap)
{
	/* Nothing. */
}

#if (defined(PPC_OEA) + defined(PPC_OEA64) + defined(PPC_OEA64_BRIDGE)) != 1
#define	PMAP_EXCLUDE_DECLS
#endif

#ifndef PMAP_NOOPNAMES

#if !defined(PMAP_EXCLUDE_DECLS)
void pmap_bootstrap(vaddr_t, vaddr_t);
bool pmap_extract(pmap_t, vaddr_t, paddr_t *);
bool pmap_query_bit(struct vm_page *, int);
bool pmap_clear_bit(struct vm_page *, int);
void pmap_real_memory(paddr_t *, psize_t *);
void pmap_procwr(struct proc *, vaddr_t, size_t);
int pmap_pte_spill(pmap_t, vaddr_t, bool);
void pmap_pinit(pmap_t);

#else
#define	PMAPOPNAME(name) (*pmapops->pmapop_##name)
extern const struct pmap_ops *pmapops;
#define	pmap_pte_spill		PMAPOPNAME(pte_spill)
#define	pmap_real_memory	PMAPOPNAME(real_memory)
#define	pmap_init		PMAPOPNAME(init)
#define	pmap_virtual_space	PMAPOPNAME(virtual_space)
#define	pmap_create		PMAPOPNAME(create)
#define	pmap_reference		PMAPOPNAME(reference)
#define	pmap_destroy		PMAPOPNAME(destroy)
#define	pmap_copy		PMAPOPNAME(copy)
#define	pmap_update		PMAPOPNAME(update)
#define	pmap_collect		PMAPOPNAME(collect)
#define	pmap_enter		PMAPOPNAME(enter)
#define	pmap_remove		PMAPOPNAME(remove)
#define	pmap_kenter_pa		PMAPOPNAME(kenter_pa)
#define	pmap_kremove		PMAPOPNAME(kremove)
#define	pmap_extract		PMAPOPNAME(extract)
#define	pmap_protect		PMAPOPNAME(protect)
#define	pmap_unwire		PMAPOPNAME(unwire)
#define	pmap_page_protect	PMAPOPNAME(page_protect)
#define	pmap_query_bit		PMAPOPNAME(query_bit)
#define	pmap_clear_bit		PMAPOPNAME(clear_bit)

#define	pmap_activate		PMAPOPNAME(activate)
#define	pmap_deactivate		PMAPOPNAME(deactivate)

#define	pmap_pinit		PMAPOPNAME(pinit)
#define	pmap_procwr		PMAPOPNAME(procwr)

#define	pmap_pte_print		PMAPOPNAME(pte_print)
#define	pmap_pteg_check		PMAPOPNAME(pteg_check)
#define	pmap_print_mmuregs	PMAPOPNAME(print_mmuregs)
#define	pmap_print_pte		PMAPOPNAME(print_pte)
#define	pmap_pteg_dist		PMAPOPNAME(pteg_dist)
#define	pmap_pvo_verify		PMAPOPNAME(pvo_verify)
#define	pmap_steal_memory	PMAPOPNAME(steal_memory)
#define	pmap_bootstrap		PMAPOPNAME(bootstrap)
#endif /* PMAP_EXCLUDE_DECLS */

static inline paddr_t vtophys (vaddr_t);

/*
 * Alternate mapping hooks for pool pages.  Avoids thrashing the TLB.
 *
 * Note: This won't work if we have more memory than can be direct-mapped
 * VA==PA all at once.  But pmap_copy_page() and pmap_zero_page() will have
 * this problem, too.
 */
#if !defined(PPC_OEA64) && !defined (PPC_OEA64_BRIDGE)
#define	PMAP_MAP_POOLPAGE(pa)	(pa)
#define	PMAP_UNMAP_POOLPAGE(pa)	(pa)
#define POOL_VTOPHYS(va)	vtophys((vaddr_t) va)
#endif

static inline paddr_t
vtophys(vaddr_t va)
{
	paddr_t pa;

	if (pmap_extract(pmap_kernel(), va, &pa))
		return pa;
	KASSERT(0);
	return (paddr_t) -1;
}

extern const struct pmap_ops *pmapops;
extern const struct pmap_ops pmap32_ops;
extern const struct pmap_ops pmap64_ops;
extern const struct pmap_ops pmap64bridge_ops;

static inline void
pmap_setup32(void)
{
	pmapops = &pmap32_ops;
}

static inline void
pmap_setup64(void)
{
	pmapops = &pmap64_ops;
}

static inline void
pmap_setup64bridge(void)
{
	pmapops = &pmap64bridge_ops;
}

#endif /* !PMAP_NOOPNAMES */

bool pmap_pageidlezero (paddr_t);
void pmap_syncicache (paddr_t, psize_t);
#ifdef PPC_OEA64
vaddr_t pmap_setusr (vaddr_t);
vaddr_t pmap_unsetusr (void);
#endif

#ifdef PPC_OEA64_BRIDGE
int pmap_setup_segment0_map(int use_large_pages, ...);
#endif

#define	PMAP_NC			0x1000
#define PMAP_STEAL_MEMORY
#define PMAP_NEED_PROCWR

void pmap_zero_page(paddr_t);
void pmap_copy_page(paddr_t, paddr_t);

__END_DECLS
#endif	/* _KERNEL */
#endif	/* _LOCORE */

#endif	/* _POWERPC_OEA_PMAP_H_ */
