/*	$NetBSD: pmap.h,v 1.80 2011/05/24 23:30:30 matt Exp $	   */

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

#include <sys/atomic.h>

#include <uvm/uvm_page.h>

#include <machine/pte.h>
#include <machine/mtpr.h>
#include <machine/pcb.h>

/*
 * Some constants to make life easier.
 */
#define LTOHPS		(PGSHIFT - VAX_PGSHIFT)
#define LTOHPN		(1 << LTOHPS)

/*
 * Pmap structure
 *  pm_stack holds lowest allocated memory for the process stack.
 */

struct pmap {
	struct pte	*pm_p1ap;	/* Base of alloced p1 pte space */
	u_int		 pm_count;	/* reference count */
	struct pcb	*pm_pcbs;	/* PCBs using this pmap */
	struct pte	*pm_p0br;	/* page 0 base register */
	long		 pm_p0lr;	/* page 0 length register */
	struct pte	*pm_p1br;	/* page 1 base register */
	long		 pm_p1lr;	/* page 1 length register */
	struct pmap_statistics	 pm_stats;	/* Some statistics */
};

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


/*
 * Real nice (fast) routines to get the virtual address of a physical page
 * (and vice versa).
 */
#define	PMAP_VTOPHYS(va)	((va) & ~KERNBASE)
#define PMAP_MAP_POOLPAGE(pa)	((pa) | KERNBASE)
#define PMAP_UNMAP_POOLPAGE(va) ((va) & ~KERNBASE)

#define PMAP_STEAL_MEMORY

/*
 * This is the by far most used pmap routine. Make it inline.
 */
static __inline bool
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *pap)
{
	int	*pte, sva;

	if (va & KERNBASE) {
		paddr_t pa;

		pa = kvtophys(va); /* Is 0 if not mapped */
		if (pap)
			*pap = pa;
		if (pa)
			return (true);
		return (false);
	}

	sva = PG_PFNUM(va);
	if (va < 0x40000000) {
		if (sva >= (pmap->pm_p0lr & ~AST_MASK))
			goto fail;
		pte = (int *)pmap->pm_p0br;
	} else {
		if (sva < pmap->pm_p1lr)
			goto fail;
		pte = (int *)pmap->pm_p1br;
	}
	/*
	 * Since the PTE tables are sparsely allocated, make sure the page
	 * table page actually exists before deferencing the pte itself.
	 */
	if (kvtopte(&pte[sva])->pg_v && (pte[sva] & PG_FRAME)) {
		if (pap)
			*pap = (pte[sva] & PG_FRAME) << VAX_PGSHIFT;
		return (true);
	}
  fail:
	if (pap)
		*pap = 0;
	return (false);
}

bool pmap_clear_modify_long(const struct pv_entry *);
bool pmap_clear_reference_long(const struct pv_entry *);
bool pmap_is_modified_long_p(const struct pv_entry *);
void pmap_page_protect_long(struct pv_entry *, vm_prot_t);
void pmap_protect_long(pmap_t, vaddr_t, vaddr_t, vm_prot_t);

static __inline struct pv_entry *
pmap_pg_to_pv(const struct vm_page *pg)
{
	return pv_table + (VM_PAGE_TO_PHYS(pg) >> PGSHIFT);
}

static __inline bool
pmap_is_referenced(struct vm_page *pg)
{
	const struct pv_entry * const pv = pmap_pg_to_pv(pg);

	return (pv->pv_attr & PG_V) != 0;
}

static __inline bool
pmap_clear_reference(struct vm_page *pg)
{
	struct pv_entry * const pv = pmap_pg_to_pv(pg);
	bool rv = (pv->pv_attr & PG_V) != 0;

	pv->pv_attr &= ~PG_V;
	if (pv->pv_pmap != NULL || pv->pv_next != NULL)
		rv |= pmap_clear_reference_long(pv);
	return rv;
}

static __inline bool
pmap_clear_modify(struct vm_page *pg)
{
	struct pv_entry * const pv = pmap_pg_to_pv(pg);
	bool rv = (pv->pv_attr & PG_M) != 0;

	pv->pv_attr &= ~PG_M;
	if (pv->pv_pmap != NULL || pv->pv_next != NULL)
		rv |= pmap_clear_modify_long(pv);
	return rv;
}

static __inline bool
pmap_is_modified(struct vm_page *pg)
{
	const struct pv_entry * const pv = pmap_pg_to_pv(pg);

	return (pv->pv_attr & PG_M) != 0 || pmap_is_modified_long_p(pv);
}

static __inline void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	struct pv_entry * const pv = pmap_pg_to_pv(pg);

	if (pv->pv_pmap != NULL || pv->pv_next != NULL)
		pmap_page_protect_long(pv, prot);
}

static __inline void
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
#define pmap_remove(pmap, start, end)	pmap_protect(pmap, start, end, 0)
#define pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)
#define pmap_reference(pmap)		atomic_inc_uint(&(pmap)->pm_count)

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
void	pmap_bootstrap(void);
vaddr_t pmap_map(vaddr_t, vaddr_t, vaddr_t, int);

#if 0
#define	__HAVE_VM_PAGE_MD

struct vm_page_md {
	unsigned int md_attrs;
};

#define	VM_MDPAGE_INIT(pg)	((pg)->mdpage.md_attrs = 0)
#endif

#endif /* PMAP_H */
