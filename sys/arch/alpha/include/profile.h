/* $NetBSD: profile.h,v 1.10 2000/09/05 16:28:30 thorpej Exp $ */

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

#ifdef _KERNEL
/*
 * The following two macros do splhigh and splx respectively.
 * _alpha_pal_swpipl is a special version of alpha_pal_swpipl which
 * doesn't include profiling support.
 *
 * XXX These macros should probably use inline assembly.
 */
#define MCOUNT_ENTER							\
	s = _alpha_pal_swpipl(ALPHA_PSL_IPL_HIGH)
#define MCOUNT_EXIT							\
	(void)_alpha_pal_swpipl(s);
#endif /* _KERNEL */

#define	_MCOUNT_DECL	void mcount
#define	_MCOUNT_FUNC	mcount
