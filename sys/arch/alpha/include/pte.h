/* $NetBSD: pte.h,v 1.15 1998/03/07 03:15:06 thorpej Exp $ */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef _ALPHA_PTE_H_
#define	_ALPHA_PTE_H_

/*
 * Alpha page table entry.
 * Things which are in the VMS PALcode but not in the OSF PALcode
 * are marked with "(VMS)".
 *
 * This information derived from pp. (II) 3-3 - (II) 3-6 and
 * (III) 3-3 - (III) 3-5 of the "Alpha Architecture Reference Manual" by
 * Richard L. Sites.
 */

/*
 * Alpha Page Table Entry
 */

#include <machine/alpha_cpu.h>

typedef	alpha_pt_entry_t	pt_entry_t;

#define	PT_ENTRY_NULL	((pt_entry_t *) 0)
#define	PTESHIFT	3			/* pte size == 1 << PTESHIFT */

#define	PG_V		ALPHA_PTE_VALID
#define	PG_NV		0
#define	PG_FOR		ALPHA_PTE_FAULT_ON_READ
#define	PG_FOW		ALPHA_PTE_FAULT_ON_WRITE
#define	PG_FOE		ALPHA_PTE_FAULT_ON_EXECUTE
#define	PG_ASM		ALPHA_PTE_ASM
#define	PG_GH		ALPHA_PTE_GRANULARITY
#define	PG_KRE		ALPHA_PTE_KR
#define	PG_URE		ALPHA_PTE_UR
#define	PG_KWE		ALPHA_PTE_KW
#define	PG_UWE		ALPHA_PTE_UW
#define	PG_PROT		ALPHA_PTE_PROT
#define	PG_RSVD		0x000000000000cc80	/* Reserved fpr hardware */
#define	PG_WIRED	0x0000000000010000	/* Wired. [SOFTWARE] */
#define	PG_FRAME	ALPHA_PTE_RAME
#define	PG_SHIFT	32
#define	PG_PFNUM(x)	ALPHA_PTE_TO_PFN(x)

#ifndef _LOCORE
#define	PTEMASK		(NPTEPG - 1)
#define	l3pte_index(va)	(((va) >> PGSHIFT) & PTEMASK)
#define	l2pte_index(va)	(((va) >> SEGSHIFT) & PTEMASK)
#define l1pte_index(va) \
	(((vm_offset_t)(va) >> (PGSHIFT + 2*(PGSHIFT-PTESHIFT))) & PTEMASK)

#define	ALPHA_STSIZE		((u_long)NBPG)			/* 8k */
#define	ALPHA_MAX_PTSIZE	((u_long)(NPTEPG * NBPG))	/* 8M */

#ifdef _KERNEL
extern	pt_entry_t *Lev1map;		/* kernel level 1 page table */
#endif /* _KERNEL */
#endif /* ! _LOCORE */

#endif /* ! _ALPHA_PTE_H_ */
