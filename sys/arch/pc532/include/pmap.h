/*	$NetBSD: pmap.h,v 1.23 1999/03/01 14:27:59 matthias Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Copyright (c) 1991 Regents of the University of California.
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
 *	@(#)pmap.h	7.4 (Berkeley) 5/12/91
 */

/*
 * Derived from hp300 version by Mike Hibler, this version by William
 * Jolitz uses a recursive map [a pde points to the page directory] to
 * map the page tables using the pagetables themselves. This is done to
 * reduce the impact on kernel virtual memory for lots of sparse address
 * space, and to reduce the cost of memory to each process.
 *
 * from hp300:	@(#)pmap.h	7.2 (Berkeley) 12/16/90
 */

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_pmap_new.h"
#endif

#ifdef PMAP_NEW			/* redirect */
#include <machine/pmap.new.h>	/* defines _NS532_PMAP_H_ */
#endif

#ifndef	_NS532_PMAP_H_
#define	_NS532_PMAP_H_

#include <machine/cpufunc.h>
#include <machine/pte.h>

/*
 * 386 page table entry and page table directory
 * W.Jolitz, 8/89
 */

/*
 * PG_AVAIL usage ...
 */

#define PG_W         PG_AVAIL1       /* "wired" mapping */

/*
 * One page directory, shared between
 * kernel and user modes.
 */
#define	PTDPTDI		0x3df		/* ptd entry that points to ptd! */
#define	KPTDI		0x3e0		/* start of kernel virtual pde's */
#define	NKPDE_BASE	4		/* min. # of kernel PDEs */
#define	NKPDE_MAX	30		/* max. # of kernel PDEs */
#define	NKPDE_SCALE	1		/* # of kernel PDEs to add per meg. */
#define	APTDPTDI	0x3fe		/* start of alternate page directory */

#define UPT_MIN_ADDRESS	(PTDPTDI<<PDSHIFT)
#define UPT_MAX_ADDRESS	(UPT_MIN_ADDRESS + (PTDPTDI<<PGSHIFT))

/*
 * Address of current and alternate address space page table maps
 * and directories.
 */
#ifdef _KERNEL
extern pt_entry_t	PTmap[], APTmap[];
extern pd_entry_t	PTD[], APTD[], PTDpde, APTDpde;
extern pt_entry_t	*Sysmap;

void pmap_bootstrap __P((vaddr_t start));
boolean_t pmap_testbit __P((paddr_t, int));
void pmap_changebit __P((paddr_t, int, int));
#endif

/*
 * virtual address to page table entry and
 * to physical address. Likewise for alternate address space.
 * Note: these work recursively, thus vtopte of a pte will give
 * the corresponding pde that in turn maps it.
 */
#define	vtopte(va)	(PTmap + ns532_btop(va))
#define	kvtopte(va)	vtopte(va)
#define	ptetov(pt)	(ns532_ptob(pt - PTmap))
#define	vtophys(va) \
	((*vtopte(va) & PG_FRAME) | ((unsigned)(va) & ~PG_FRAME))

#define	avtopte(va)	(APTmap + ns532_btop(va))
#define	ptetoav(pt)	(ns532_ptob(pt - APTmap))
#define	avtophys(va) \
	((*avtopte(va) & PG_FRAME) | ((unsigned)(va) & ~PG_FRAME))

/*
 * macros to generate page directory/table indicies
 */
#define	pdei(va)	(((va) & PD_MASK) >> PDSHIFT)
#define	ptei(va)	(((va) & PT_MASK) >> PGSHIFT)

/*
 * Pmap stuff
 */
typedef struct pmap {
	pd_entry_t		*pm_pdir;	/* KVA of page directory */
	short			pm_dref;	/* page directory ref count */
	short			pm_count;	/* pmap reference count */
	simple_lock_data_t	pm_lock;	/* lock on pmap */
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	long			pm_ptpages;	/* more stats: PT pages */
} *pmap_t;

/*
 * For each vm_page_t, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry, the list is pv_table.
 */
struct pv_entry {
	struct pv_entry	*pv_next;	/* next pv_entry */
	pmap_t		pv_pmap;	/* pmap where mapping lies */
	vaddr_t		pv_va;		/* virtual address for mapping */
};

struct pv_page;

struct pv_page_info {
	TAILQ_ENTRY(pv_page) pgi_list;
	struct pv_entry *pgi_freelist;
	int pgi_nfree;
};

/*
 * This is basically:
 * ((NBPG - sizeof(struct pv_page_info)) / sizeof(struct pv_entry))
 */
#define	NPVPPG	340

struct pv_page {
	struct pv_page_info pvp_pgi;
	struct pv_entry pvp_pv[NPVPPG];
};

#ifdef	_KERNEL
extern int		nkpde;		/* number of kernel page dir. ents */
extern struct pmap	kernel_pmap_store;

pt_entry_t *pmap_pte __P((pmap_t, vaddr_t));
#define	pmap_kernel()			(&kernel_pmap_store)
#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_update()			tlbflush()

vaddr_t reserve_dumppages __P((vaddr_t));

static __inline void
pmap_clear_modify(paddr_t pa)
{
	pmap_changebit(pa, 0, ~PG_M);
}

static __inline void
pmap_clear_reference(paddr_t pa)
{
	pmap_changebit(pa, 0, ~PG_U);
}

static __inline void
pmap_copy_on_write(paddr_t pa)
{
	pmap_changebit(pa, PG_RO, ~PG_RW);
}

static __inline boolean_t
pmap_is_modified(paddr_t pa)
{
	return pmap_testbit(pa, PG_M);
}

static __inline boolean_t
pmap_is_referenced(paddr_t pa)
{
	return pmap_testbit(pa, PG_U);
}

static __inline paddr_t
pmap_phys_address(int ppn)
{
	return ns532_ptob(ppn);
}

vaddr_t	pmap_map __P((vaddr_t, paddr_t, paddr_t, int));

static __inline void
pmap_procwr(struct proc *p, vaddr_t va, u_long len)
{
	cinv(ia, 0);
}
#define PMAP_NEED_PROCWR

#endif	/* _KERNEL */

#endif /* _NS532_PMAP_H_ */
