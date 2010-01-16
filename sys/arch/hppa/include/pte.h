/*	$NetBSD: pte.h,v 1.4 2010/01/16 13:59:42 skrll Exp $	*/

/*	$OpenBSD: pte.h,v 1.11 2002/09/05 18:41:19 mickey Exp $	*/

/* 
 * Copyright (c) 1990,1993,1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: pmap.h 1.24 94/12/14$
 *	Author: Mike Hibler, Bob Wheeler, University of Utah CSL, 9/90
 */

#ifndef	_HPPA_PTE_H_
#define	_HPPA_PTE_H_

typedef	uint32_t	pt_entry_t;

#define	PTE_PROT_SHIFT	19
#define	PTE_PROT(tlb)	((tlb) >> PTE_PROT_SHIFT)
#define	TLB_PROT(pte)	((pte) << PTE_PROT_SHIFT)
#define	PDE_MASK	(0xffc00000)
#define	PDE_SIZE	(0x00400000)
#define	PTE_MASK	(0x003ff000)
#define	PTE_PAGE(pte)	((pte) & ~PGOFSET)

/* TLB access/protection values */
/*			0x80000000 has no pt_entry_t equivalent */
#define	TLB_WIRED	0x40000000	/* software only */
#define	TLB_REFTRAP	0x20000000	/* bit 2, T */
#define	TLB_DIRTY	0x10000000	/* bit 3, D */
#define	TLB_BREAK	0x08000000	/* bit 4, B */
#define	TLB_AR_MASK	0x07f00000	/* bits 5-11, Access Rights */
#define	TLB_READ	0x00000000
#define	TLB_WRITE	0x01000000
#define	TLB_EXECUTE	0x02000000
#define	TLB_GATEWAY	0x04000000
#define	TLB_USER	0x00f00000
#define	 TLB_AR_NA	0x07300000
#define	 TLB_AR_R	TLB_READ
#define	 TLB_AR_RW	TLB_READ|TLB_WRITE
#define	 TLB_AR_RX	TLB_READ|TLB_EXECUTE
#define	 TLB_AR_RWX	TLB_READ|TLB_WRITE|TLB_EXECUTE
#define	TLB_UNCACHEABLE	0x00080000	/* bit 12, U */
					/* bit 13-30, Access ID */
#define	TLB_PID_MASK	0x0000fffe
/*			0x00000001 has no pt_entry_t equivalent */

#define TLB_BITS	\
	"\177\020"	/* New bitmask */		\
	"b\036wired\0"		/* bit 30 (1) */		\
	"b\035T\0"		/* bit 29 (2) */		\
	"b\034D\0"		/* bit 28 (3) */		\
	"b\033B\0"		/* bit 27 (4) */		\
	"b\023U\0"		/* bit 19 (12) */		\
	"f\024\07ar\0"		/* bit 20 (11) .. 26 (5) */	\
	    "=\x73" "------\0"					\
	    "=\x00" "r-----\0"					\
	    "=\x10" "rw----\0"					\
	    "=\x20" "r-x---\0"					\
	    "=\x30" "rwx---\0"					\
	    "=\x0f" "r--r--\0"					\
	    "=\x1f" "rw-rw-\0"					\
	    "=\x2f" "r-xr-x\0"					\
	    "=\x3f" "rwxrwx\0"					\
	    "=\x4c" "gate\0"					\
	    "=\x2c" "break\0"					\
	"f\001\017pid\0\0"	 /* bit 1 (30) .. 15 (16)	*/

/* protection for a gateway page */
#define	TLB_GATE_PROT	0x04c00000

/* protection for break page */
#define	TLB_BREAK_PROT	0x02c00000

#endif	/* _HPPA_PTE_H_ */
