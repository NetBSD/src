/*	$NetBSD: cdefs.h,v 1.8 1999/01/14 18:45:46 castor Exp $	*/

/*
 * Copyright (c) 1995 Carnegie-Mellon University.
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

#ifndef _MIPS_CDEFS_H_
#define	_MIPS_CDEFS_H_

/*      MIPS Subprogram Interface Model */
#define _MIPS_SIM_ABIX32	4	/* 64 bit safe, ILP32 o32 model */
#define _MIPS_SIM_ABI64		3
#define _MIPS_SIM_NABI32	2	/* 64bit safe, ILP32 n32 model */
#define _MIPS_SIM_ABI32		1

#define	_C_LABEL(x)	x

#ifdef __GNUC__
#define	__RENAME(x)	__asm__(___STRING(_C_LABEL(x)))
#endif

#define	__indr_references(sym,msg)	/* nothing */

#if defined __GNUC__ && defined __STDC__
#define __warn_references(sym, msg)                  \
  static const char __evoke_link_warning_##sym[]     \
    __attribute__ ((section (".gnu.warning." #sym))) = msg;
#else
#define	__warn_references(sym,msg)	/* nothing */
#endif

/* Kernel-only .sections for kernel copyright */
#ifdef _KERNEL

#ifdef __STDC__
#define	__KERNEL_SECTIONSTRING(_sec, _str)				\
	__asm__(".section " #_sec " ; .asciz \"" _str "\" ; .text")
#else
#define	__KERNEL_SECTIONSTRING(_sec, _str)				\
	__asm__(".section _sec ; .asciz _str ; .text")
#endif

#define	__KERNEL_RCSID(_n, _s)		__KERNEL_SECTIONSTRING(.ident, _s)
#define	__KERNEL_COPYRIGHT(_n, _s)	__KERNEL_SECTIONSTRING(.copyright, _s)

#ifdef NO_KERNEL_RCSIDS
#undef __KERNEL_RCSID
#define	__KERNEL_RCSID(_n, _s)		/* nothing */
#endif

#endif /* _KERNEL */

#endif /* !_MIPS_CDEFS_H_ */
