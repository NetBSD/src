/* $NetBSD: pte.h,v 1.8.2.1 1997/06/01 04:12:32 cgd Exp $ */

/*
 * Copyright Notice:
 *
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
 *
 * License:
 *
 * This License applies to this software ("Software"), created
 * by Christopher G. Demetriou ("Author").
 *
 * You may use, copy, modify and redistribute this Software without
 * charge, in either source code form, binary form, or both, on the
 * following conditions:
 *
 * 1.  (a) Binary code: (i) a complete copy of the above copyright notice
 * must be included within each copy of the Software in binary code form,
 * and (ii) a complete copy of the above copyright notice and all terms
 * of this License as presented here must be included within each copy of
 * all documentation accompanying or associated with binary code, in any
 * medium, along with a list of the software modules to which the license
 * applies.
 *
 * (b) Source Code: A complete copy of the above copyright notice and all
 * terms of this License as presented here must be included within: (i)
 * each copy of the Software in source code form, and (ii) each copy of
 * all accompanying or associated documentation, in any medium.
 *
 * 2. The following Acknowledgment must be used in communications
 * involving the Software as described below:
 *
 *      This product includes software developed by
 *      Christopher G. Demetriou for the NetBSD Project.
 *
 * The Acknowledgment must be conspicuously and completely displayed
 * whenever the Software, or any software, products or systems containing
 * the Software, are mentioned in advertising, marketing, informational
 * or publicity materials of any kind, whether in print, electronic or
 * other media (except for information provided to support use of
 * products containing the Software by existing users or customers).
 *
 * 3. The name of the Author may not be used to endorse or promote
 * products derived from this Software without specific prior written
 * permission (conditions (1) and (2) above are not considered
 * endorsement or promotion).
 *
 * 4.  This license applies to: (a) all copies of the Software, whether
 * partial or whole, original or modified, and (b) your actions, and the
 * actions of all those who may act on your behalf.  All uses not
 * expressly permitted are reserved to the Author.
 *
 * 5.  Disclaimer.  THIS SOFTWARE IS MADE AVAILABLE BY THE AUTHOR TO THE
 * PUBLIC FOR FREE AND "AS IS.''  ALL USERS OF THIS FREE SOFTWARE ARE
 * SOLELY AND ENTIRELY RESPONSIBLE FOR THEIR OWN CHOICE AND USE OF THIS
 * SOFTWARE FOR THEIR OWN PURPOSES.  BY USING THIS SOFTWARE, EACH USER
 * AGREES THAT THE AUTHOR SHALL NOT BE LIABLE FOR DAMAGES OF ANY KIND IN
 * RELATION TO ITS USE OR PERFORMANCE.
 *
 * 6.  If you have a special need for a change in one or more of these
 * license conditions, please contact the Author via electronic mail to
 *
 *     cgd@NetBSD.ORG
 *
 * or via the contact information on
 *
 *     http://www.NetBSD.ORG/People/Pages/cgd.html
 */

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

#if 0 /* XXX NOT HERE */
#define	K0SEG_BEGIN	0xfffffc0000000000	/* unmapped, cached */
#define	K0SEG_END	0xfffffe0000000000
#define PHYS_UNCACHED	0x0000000040000000
#endif

#ifndef _LOCORE
#if 0 /* XXX NOT HERE */
#define	k0segtophys(x)	((vm_offset_t)(x) & 0x00000003ffffffff)
#define	phystok0seg(x)	((vm_offset_t)(x) | K0SEG_BEGIN)

#define phystouncached(x) ((vm_offset_t)(x) | PHYS_UNCACHED)
#define uncachedtophys(x) ((vm_offset_t)(x) & ~PHYS_UNCACHED)
#endif

#define	PTEMASK		(NPTEPG - 1)
#define	vatopte(va)	(((va) >> PGSHIFT) & PTEMASK)
#define	vatoste(va)	(((va) >> SEGSHIFT) & PTEMASK)
#define kvtol1pte(va) \
	(((vm_offset_t)(va) >> (PGSHIFT + 2*(PGSHIFT-PTESHIFT))) & PTEMASK)

#define	vatopa(va) \
	((PG_PFNUM(*kvtopte(va)) << PGSHIFT) | ((vm_offset_t)(va) & PGOFSET))

#define	ALPHA_STSIZE		((u_long)NBPG)			/* 8k */
#define	ALPHA_MAX_PTSIZE	((u_long)(NPTEPG * NBPG))	/* 8M */

#ifdef _KERNEL
/*
 * Kernel virtual address to Sysmap entry and visa versa.
 */
#define	kvtopte(va) \
	(Sysmap + (((vm_offset_t)(va) - VM_MIN_KERNEL_ADDRESS) >> PGSHIFT))
#define	ptetokv(pte) \
	((((pt_entry_t *)(pte) - Sysmap) << PGSHIFT) + VM_MIN_KERNEL_ADDRESS)

#define loadustp(stpte) {					\
	Lev1map[kvtol1pte(VM_MIN_ADDRESS)] = stpte;		\
	ALPHA_TBIAP();						\
	alpha_pal_imb();					\
}

extern	pt_entry_t *Lev1map;		/* Alpha Level One page table */
extern	pt_entry_t *Sysmap;		/* kernel pte table */
extern	vm_size_t Sysmapsize;		/* number of pte's in Sysmap */
#endif
#endif
