/* $NetBSD: pmap.old.h,v 1.21 1998/03/01 08:17:37 ross Exp $ */

/* 
 * Copyright (c) 1987 Carnegie-Mellon University
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

#include <sys/queue.h>
#include <machine/pte.h>

extern vm_offset_t       vtophys(vm_offset_t);

#define	ALPHA_PAGE_SIZE	NBPG
#define	ALPHA_SEG_SIZE	NBSEG

#define alpha_trunc_seg(x)	(((u_long)(x)) & ~(ALPHA_SEG_SIZE-1))
#define alpha_round_seg(x)	alpha_trunc_seg((u_long)(x) + ALPHA_SEG_SIZE-1)

/*
 * Pmap stuff
 */
struct pmap {
	LIST_ENTRY(pmap)	pm_list;	/* list of all pmaps */
	pt_entry_t		*pm_lev1map;	/* level 1 map */
	pt_entry_t		*pm_ptab;	/* KVA of page table */
	pt_entry_t		*pm_stab;	/* KVA of segment table */
	short			pm_sref;	/* segment table ref count */
	short			pm_count;	/* pmap reference count */
	simple_lock_data_t	pm_lock;	/* lock on pmap */
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	long			pm_ptpages;	/* more stats: PT pages */
};

typedef struct pmap	*pmap_t;

extern struct pmap	kernel_pmap_store;

#define pmap_kernel()	(&kernel_pmap_store)
#define	active_pmap(pm) \
	((pm) == pmap_kernel()	\
	|| curproc == NULL	\
	|| (pm) == curproc->p_vmspace->vm_map.pmap)
#define	active_user_pmap(pm) \
	(curproc && \
	 (pm) != pmap_kernel() && (pm) == curproc->p_vmspace->vm_map.pmap)

/*
 * For each vm_page_t, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 */
typedef struct pv_entry {
	LIST_ENTRY(pv_entry) pv_list;	/* pv_entry list */
	struct pmap	*pv_pmap;	/* pmap where mapping lies */
	vm_offset_t	pv_va;		/* virtual address for mapping */
	pt_entry_t	*pv_ptpte;	/* non-zero if VA maps a PT page */
	struct pmap	*pv_ptpmap;	/* if pv_ptpte, pmap for PT page */
} *pv_entry_t;

/*
 * The head of the list of pv_entry_t's, also contains page attributes.
 */
struct pv_head {
	LIST_HEAD(, pv_entry) pvh_list;		/* pv_entry list */
	int pvh_attrs;				/* page attributes */
};

/* pvh_attrs */
#define	PMAP_ATTR_MOD		0x01		/* modified */
#define	PMAP_ATTR_REF		0x02		/* referenced */
#define	PMAP_ATTR_PTPAGE	0x04		/* maps a PT page */

struct pv_page_info {
	TAILQ_ENTRY(pv_page) pgi_list;
	LIST_HEAD(, pv_entry) pgi_freelist;
	int pgi_nfree;
};

#define	NPVPPG ((NBPG - sizeof(struct pv_page_info)) / sizeof(struct pv_entry))

struct pv_page {
	struct pv_page_info pvp_pgi;
	struct pv_entry pvp_pv[NPVPPG];
};

#ifdef _KERNEL

#include "opt_avalon_a12.h"			/* XXX */
#include "opt_dec_3000_300.h"			/* XXX */
#include "opt_dec_3000_500.h"			/* XXX */
#include "opt_dec_kn8ae.h"			/* XXX */

#if defined(DEC_3000_300)	\
 || defined(DEC_3000_500)	\
 || defined(DEC_KN8AE) 				/* XXX */
#define _PMAP_MAY_USE_PROM_CONSOLE		/* XXX */
#endif						/* XXX */
 
#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

extern	pt_entry_t *Sysmap;
extern	char *vmmap;			/* map for mem, dumps, etc. */

#define	PMAP_STEAL_MEMORY		/* enable pmap_steal_memory() */

/* Machine-specific functions. */
void	pmap_bootstrap __P((vm_offset_t ptaddr));
void	pmap_emulate_reference __P((struct proc *p, vm_offset_t v,
		int user, int write));
#ifdef _PMAP_MAY_USE_PROM_CONSOLE
int	pmap_uses_prom_console __P((void));
#endif
#endif /* _KERNEL */

#endif /* _PMAP_MACHINE_ */
