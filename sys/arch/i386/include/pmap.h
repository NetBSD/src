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
 *	from: @(#)pmap.h	7.4 (Berkeley) 5/12/91
 *	$Id: pmap.h,v 1.6 1993/12/14 05:31:38 mycroft Exp $
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

#ifndef	_I386_PMAP_H_
#define	_I386_PMAP_H_

#include <machine/pte.h>

/*
 * 386 page table entry and page table directory
 * W.Jolitz, 8/89
 */

/*
 * One page directory, shared between
 * kernel and user modes.
 */
#define	UPTDI		0x3f6		/* ptd entry for u./kernel&user stack */
#define	PTDPTDI		0x3f7		/* ptd entry that points to ptd! */
#define	KPTDI		0x3f8		/* start of kernel virtual pde's */
#define	NKPDE		7
#define	APTDPTDI	0x3ff		/* start of alternate page directory */

/*
 * Address of current and alternate address space page table maps
 * and directories.
 */
#ifdef KERNEL
extern struct pte	PTmap[], APTmap[], Upte;
extern struct pde	PTD[], APTD[], PTDpde, APTDpde, Upde;
extern pt_entry_t	*Sysmap;

extern int	IdlePTD;	/* physical address of "Idle" state directory */
#endif

/*
 * virtual address to page table entry and
 * to physical address. Likewise for alternate address space.
 * Note: these work recursively, thus vtopte of a pte will give
 * the corresponding pde that in turn maps it.
 */
#define	vtopte(va)	(PTmap + i386_btop(va))
#define	kvtopte(va)	vtopte(va)
#define	ptetov(pt)	(i386_ptob(pt - PTmap)) 
#define	vtophys(va) \
	(i386_ptob(vtopte(va)->pg_pfnum) | ((int)(va) & PGOFSET))

#define	avtopte(va)	(APTmap + i386_btop(va))
#define	ptetoav(pt)	(i386_ptob(pt - APTmap)) 
#define	avtophys(va) \
	(i386_ptob(avtopte(va)->pg_pfnum) | ((int)(va) & PGOFSET))

/*
 * macros to generate page directory/table indicies
 */
#define	pdei(va)	(((va)&PD_MASK)>>PDSHIFT)
#define	ptei(va)	(((va)&PT_MASK)>>PGSHIFT)

/*
 * Pmap stuff
 */
struct pmap {
	pd_entry_t		*pm_pdir;	/* KVA of page directory */
	boolean_t		pm_pdchanged;	/* pdir changed */
	short			pm_dref;	/* page directory ref count */
	short			pm_count;	/* pmap reference count */
	simple_lock_data_t	pm_lock;	/* lock on pmap */
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	long			pm_ptpages;	/* more stats: PT pages */
};

typedef struct pmap	*pmap_t;

#ifdef KERNEL
extern pmap_t		kernel_pmap;
#endif

/*
 * Macros for speed
 */
#define PMAP_ACTIVATE(pmapp, pcbp) \
	if ((pmapp) != NULL /*&& (pmapp)->pm_pdchanged */) {  \
		(pcbp)->pcb_cr3 = \
		    pmap_extract(kernel_pmap, (pmapp)->pm_pdir); \
		if ((pmapp) == &curproc->p_vmspace->vm_pmap) \
			lcr3((pcbp)->pcb_cr3); \
		(pmapp)->pm_pdchanged = FALSE; \
	}

#define PMAP_DEACTIVATE(pmapp, pcbp)

/*
 * For each vm_page_t, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 */
typedef struct pv_entry {
	struct pv_entry	*pv_next;	/* next pv_entry */
	pmap_t		pv_pmap;	/* pmap where mapping lies */
	vm_offset_t	pv_va;		/* virtual address for mapping */
	int		pv_flags;	/* flags */
} *pv_entry_t;

#ifdef	KERNEL

pv_entry_t	pv_table;		/* array of entries, one per page */

#define pa_to_pvh(pa)	(&pv_table[pmap_page_index(pa)])
void pmap_bootstrap __P((vm_offset_t start));

#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)

#endif	/* KERNEL */

#endif /* _I386_PMAP_H_ */
