/*	$NetBSD: pmap.h,v 1.2 2003/02/05 07:05:19 matt Exp $	*/

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

#include <powerpc/oea/pte.h>

#ifndef _LOCORE
/*
 * Pmap stuff
 */
struct pmap {
	register_t pm_sr[16];			/* segments used in this pmap */
	int pm_refs;				/* ref count */
	struct pmap_statistics pm_stats;	/* pmap statistics */
	unsigned int pm_evictions;		/* pvo's not in page table */
};

typedef	struct pmap *pmap_t;

#ifdef	_KERNEL
extern register_t iosrtable[];
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

static __inline void
pmap_remove_all(struct pmap *pmap)
{
	/* Nothing. */
}

void pmap_bootstrap (vaddr_t kernelstart, vaddr_t kernelend);
boolean_t pmap_extract (struct pmap *, vaddr_t, paddr_t *);
boolean_t pmap_query_bit (struct vm_page *, int);
boolean_t pmap_clear_bit (struct vm_page *, int);
void pmap_real_memory (paddr_t *, psize_t *);
void pmap_pinit (struct pmap *);
boolean_t pmap_pageidlezero (paddr_t);
void pmap_syncicache (paddr_t, psize_t);

#define PMAP_NEED_PROCWR
void pmap_procwr (struct proc *, vaddr_t, size_t);

int pmap_pte_spill(struct pmap *, vaddr_t);

#define	PMAP_NC			0x1000

#define PMAP_STEAL_MEMORY
#if 1
/*
 * Alternate mapping hooks for pool pages.  Avoids thrashing the TLB.
 *
 * Note: This won't work if we have more memory than can be direct-mapped
 * VA==PA all at once.  But pmap_copy_page() and pmap_zero_page() will have
 * this problem, too.
 */
#define	PMAP_MAP_POOLPAGE(pa)	(pa)
#define	PMAP_UNMAP_POOLPAGE(pa)	(pa)
#endif

static __inline paddr_t vtophys (vaddr_t);

static __inline paddr_t
vtophys(vaddr_t va)
{
	paddr_t pa;

	/* XXX should check battable */

	if (pmap_extract(pmap_kernel(), va, &pa))
		return pa;
	return va;
}

#endif	/* _KERNEL */
#endif	/* _LOCORE */

#endif	/* _POWERPC_OEA_PMAP_H_ */
