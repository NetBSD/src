/*	$NetBSD: alpha_cpu.h,v 1.1 1996/07/09 00:40:58 cgd Exp $	*/

/*
 * Copyright (c) 1996 Carnegie-Mellon University.
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

#ifndef __ALPHA_ALPHA_CPU_H__
#define	__ALPHA_ALPHA_CPU_H__

/*
 * Alpha CPU + OSF/1 PALcode definitions for use by the kernel.
 *
 * Definitions for:
 *
 *	Processor Status Register
 *	Virtual Memory Management
 *	Translation Buffer Invalidation
 *
 * and miscellaneous PALcode operations.
 */


/*
 * Processor Status Register [OSF/1 PALcode Specific]
 *
 * Includes user/kernel mode bit, interrupt priority levels, etc.
 */

#define	ALPHA_PSL_USERMODE	0x0008		/* set -> user mode */
#define	ALPHA_PSL_IPL_MASK	0x0007		/* interrupt level mask */

#define	ALPHA_PSL_IPL_0		0x0000		/* all interrupts enabled */
#define	ALPHA_PSL_IPL_SOFT	0x0001		/* software ints disabled */
#define	ALPHA_PSL_IPL_IO	0x0004		/* I/O dev ints disabled */
#define	ALPHA_PSL_IPL_CLOCK	0x0005		/* clock ints disabled */
#define	ALPHA_PSL_IPL_HIGH	0x0006		/* all but mchecks disabled */

#define	ALPHA_PSL_MUST_BE_ZERO	0xfffffffffffffff0

/* Convenience constants: what must be set/clear in user mode */
#define	ALPHA_PSL_USERSET	ALPHA_PSL_USERMODE
#define	ALPHA_PSL_USERCLR	(ALPHA_PSL_MUST_BE_ZERO | ALPHA_PSL_IPL_MASK)

typedef unsigned long alpha_psl_t;

/*
 * Virtual Memory Management [OSF/1 PALcode Specific]
 *
 * Includes user and kernel space addresses and information,
 * page table entry definitions, etc.
 *
 * NOTE THAT THESE DEFINITIONS MAY CHANGE IN FUTURE ALPHA CPUS!
 */

#define	ALPHA_PGSHIFT		13

#define	ALPHA_USEG_BASE		0			/* virtual */
#define	ALPHA_USEG_END		0x000003ffffffffff

#define	ALPHA_K0SEG_BASE	0xfffffc0000000000	/* direct-mapped */
#define	ALPHA_K0SEG_END		0xfffffe0000000000
#define	ALPHA_K1SEG_BASE	ALPHA_K0SEG_END		/* virtual */
#define	ALPHA_K1SEG_END		0xffffffffffffffff

#define ALPHA_K0SEG_TO_PHYS(x)	((x) & 0x00000003ffffffff)
#define ALPHA_PHYS_TO_K0SEG(x)	((x) | ALPHA_K0SEG_BASE)

#define	ALPHA_PTE_VALID			0x0001

#define	ALPHA_PTE_FAULT_ON_READ		0x0002
#define	ALPHA_PTE_FAULT_ON_WRITE	0x0004
#define	ALPHA_PTE_FAULT_ON_EXECUTE	0x0008

#define	ALPHA_PTE_ASM			0x0010		/* addr. space match */
#define	ALPHA_PTE_GRANULARITY		0x0060		/* granularity hint */

#define	ALPHA_PTE_PROT			0xff00
#define	ALPHA_PTE_KR			0x0100
#define	ALPHA_PTE_UR			0x0200
#define	ALPHA_PTE_KW			0x1000
#define	ALPHA_PTE_UW			0x2000

#define	ALPHA_PTE_WRITE			(ALPHA_PTE_KW | ALPHA_PTE_KW)

#define	ALPHA_PTE_SOFTWARE		0xffff0000

#define	ALPHA_PTE_PFN			0xffffffff00000000

#define	ALPHA_PTE_TO_PFN(pte)		((pte) >> 32)
#define	ALPHA_PTE_FROM_PFN(pfn)		((pfn) << 32)

typedef unsigned long alpha_pt_entry_t;


/*
 * Translation Buffer Invalidation [OSF/1 PALcode Specific]
 */

#define	TBIA()		alpha_pal_tbi(-2, 0)		/* all TB entries */
#define	TBIAP()		alpha_pal_tbi(-1, 0)		/* all per-process */
#define	TBISI(va)	alpha_pal_tbi(1, (va))		/* ITB entry for va */
#define	TBISD(va)	alpha_pal_tbi(2, (va))		/* DTB entry for va */
#define	TBIS(va)	alpha_pal_tbi(3, (va))		/* all for va */


/*
 * Stubs for Alpha instructions normally inaccessible from C.
 */
void		alpha_mb __P((void));
void		alpha_wmb __P((void));

/*
 * Stubs for OSF/1 PALcode operations.
 */
void		alpha_pal_imb __P((void));
alpha_psl_t	alpha_pal_swpipl __P((alpha_psl_t));
alpha_psl_t	_alpha_pal_swpipl __P((alpha_psl_t));	/* for profiling */
void		alpha_pal_tbi __P((unsigned long, vm_offset_t));
void		alpha_pal_halt __P((void)) __attribute__((__noreturn__));

#endif __ALPHA_ALPHA_CPU_H__
