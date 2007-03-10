/*	$NetBSD: pmap.h,v 1.68.8.1 2007/03/10 18:39:35 bouyer Exp $	   */

/* 
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * Changed for the VAX port. /IC
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
 *	@(#)pmap.h	7.6 (Berkeley) 5/10/91
 */

/* 
 * Copyright (c) 1987 Carnegie-Mellon University
 *
 * Changed for the VAX port. /IC
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
 *	@(#)pmap.h	7.6 (Berkeley) 5/10/91
 */


#ifndef PMAP_H
#define PMAP_H

#include <machine/pte.h>
#include <machine/mtpr.h>
#include <machine/pcb.h>

/*
 * Some constants to make life easier.
 */
#define LTOHPS		(PGSHIFT - VAX_PGSHIFT)
#define LTOHPN		(1 << LTOHPS)

/*
 * Link struct if more than one process share pmap (like vfork).
 * This is rarely used.
 */
struct pm_share {
	struct pm_share	*ps_next;
	struct pcb	*ps_pcb;
};

/*
 * Pmap structure
 *  pm_stack holds lowest allocated memory for the process stack.
 */

typedef struct pmap {
	struct pte	*pm_p1ap;	/* Base of alloced p1 pte space */
	int		 pm_count;	/* reference count */
	struct pm_share	*pm_share;	/* PCBs using this pmap */
	struct pte	*pm_p0br;	/* page 0 base register */
	long		 pm_p0lr;	/* page 0 length register */
	struct pte	*pm_p1br;	/* page 1 base register */
	long		 pm_p1lr;	/* page 1 length register */
	struct simplelock pm_lock;	/* Lock entry in MP environment */
	struct pmap_statistics	 pm_stats;	/* Some statistics */
} *pmap_t;

/*
 * For each struct vm_page, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 */

struct pv_entry {
	struct pv_entry *pv_next;	/* next pv_entry */
	vaddr_t		 pv_vaddr;	/* address for this physical page */
	struct pmap	*pv_pmap;	/* pmap this entry belongs to */
	int		 pv_attr;	/* write/modified bits */
};

extern	struct  pv_entry *pv_table;

/* Mapping macros used when allocating SPT */
#define MAPVIRT(ptr, count)				\
	ptr = virtual_avail;		\
	virtual_avail += (count) * VAX_NBPG;

#define MAPPHYS(ptr, count, perm)			\
	ptr = avail_start + KERNBASE;	\
	avail_start += (count) * VAX_NBPG;

#ifdef	_KERNEL

extern	struct pmap kernel_pmap_store;

#define pmap_kernel()			(&kernel_pmap_store)

#endif	/* _KERNEL */


/*
 * Real nice (fast) routines to get the virtual address of a physical page
 * (and vice versa).
 */
#define PMAP_MAP_POOLPAGE(pa)	((pa) | KERNBASE)
#define PMAP_UNMAP_POOLPAGE(va) ((va) & ~KERNBASE)

#define PMAP_STEAL_MEMORY

/*
 * This is the by far most used pmap routine. Make it inline.
 */
__inline static boolean_t
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *pap)
{
	int	*pte, sva;

	if (va & KERNBASE) {
		paddr_t pa;

		pa = kvtophys(va); /* Is 0 if not mapped */
		if (pap)
			*pap = pa;
		if (pa)
			return (TRUE);
		return (FALSE);
	}

	sva = PG_PFNUM(va);
	if (va < 0x40000000) {
		if (sva > (pmap->pm_p0lr & ~AST_MASK))
			goto fail;
		pte = (int *)pmap->pm_p0br;
	} else {
		if (sva < pmap->pm_p1lr)
			goto fail;
		pte = (int *)pmap->pm_p1br;
	}
	if (kvtopte(&pte[sva])->pg_pfn && pte[sva]) {
		if (pap)
			*pap = (pte[sva] & PG_FRAME) << VAX_PGSHIFT;
		return (TRUE);
	}
  fail:
	if (pap)
		*pap = 0;
	return (FALSE);
}

boolean_t pmap_clear_modify_long(struct pv_entry *);
boolean_t pmap_clear_reference_long(struct pv_entry *);
boolean_t pmap_is_modified_long(struct pv_entry *);
void pmap_page_protect_long(struct pv_entry *, vm_prot_t);
void pmap_protect_long(pmap_t, vaddr_t, vaddr_t, vm_prot_t);

__inline static boolean_t
pmap_is_referenced(struct vm_page *pg)
{
	struct pv_entry *pv = pv_table + (VM_PAGE_TO_PHYS(pg) >> PGSHIFT);
	boolean_t rv = (pv->pv_attr & PG_V) != 0;

	return rv;
}

__inline static boolean_t
pmap_clear_reference(struct vm_page *pg)
{
	struct pv_entry *pv = pv_table + (VM_PAGE_TO_PHYS(pg) >> PGSHIFT);
	boolean_t rv = (pv->pv_attr & PG_V) != 0;

	pv->pv_attr &= ~PG_V;
	if (pv->pv_pmap != NULL || pv->pv_next != NULL)
		rv |= pmap_clear_reference_long(pv);
	return rv;
}

__inline static boolean_t
pmap_clear_modify(struct vm_page *pg)
{
	struct  pv_entry *pv = pv_table + (VM_PAGE_TO_PHYS(pg) >> PGSHIFT);
	boolean_t rv = (pv->pv_attr & PG_M) != 0;

	pv->pv_attr &= ~PG_M;
	if (pv->pv_pmap != NULL || pv->pv_next != NULL)
		rv |= pmap_clear_modify_long(pv);
	return rv;
}

__inline static boolean_t
pmap_is_modified(struct vm_page *pg)
{
	struct pv_entry *pv = pv_table + (VM_PAGE_TO_PHYS(pg) >> PGSHIFT);
	if (pv->pv_attr & PG_M)
		return 1;
	else
		return pmap_is_modified_long(pv);
}

__inline static void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	struct  pv_entry *pv = pv_table + (VM_PAGE_TO_PHYS(pg) >> PGSHIFT);

	if (pv->pv_pmap != NULL || pv->pv_next != NULL)
		pmap_page_protect_long(pv, prot);
}

__inline static void
pmap_protect(pmap_t pmap, vaddr_t start, vaddr_t end, vm_prot_t prot)
{
	if (pmap->pm_p0lr != 0 || pmap->pm_p1lr != 0x200000 ||
	    (start & KERNBASE) != 0)
		pmap_protect_long(pmap, start, end, prot);
}

static __inline void
pmap_remove_all(struct pmap *pmap)
{
	/* Nothing. */
}

/* Routines that are best to define as macros */
#define pmap_phys_address(phys)		((u_int)(phys) << PGSHIFT)
#define pmap_copy(a,b,c,d,e)		/* Dont do anything */
#define pmap_update(pmap)		/* nothing (yet) */
#define pmap_collect(pmap)		/* No need so far */
#define pmap_remove(pmap, start, slut)	pmap_protect(pmap, start, slut, 0)
#define pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)
#define pmap_reference(pmap)		(pmap)->pm_count++

/* These can be done as efficient inline macros */
#define pmap_copy_page(src, dst)			\
	__asm("addl3 $0x80000000,%0,%%r0;"		\
		"addl3 $0x80000000,%1,%%r1;"		\
		"movc3 $4096,(%%r0),(%%r1)"		\
	    :: "r"(src), "r"(dst)			\
	    : "r0","r1","r2","r3","r4","r5");

#define pmap_zero_page(phys)				\
	__asm("addl3 $0x80000000,%0,%%r0;"		\
		"movc5 $0,(%%r0),$0,$4096,(%%r0)"	\
	    :: "r"(phys)				\
	    : "r0","r1","r2","r3","r4","r5");

/* Prototypes */
void	pmap_bootstrap __P((void));
vaddr_t pmap_map __P((vaddr_t, vaddr_t, vaddr_t, int));

#endif /* PMAP_H */
