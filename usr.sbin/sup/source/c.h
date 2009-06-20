/*	$NetBSD: c.h,v 1.7 2009/06/20 17:03:25 christos Exp $	*/

/*
 * Copyright (c) 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator   or   Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the rights
 * to redistribute these changes.
 */
/*
 * Standard C macros
 *
 **********************************************************************
 * HISTORY
 * 02-Feb-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added check to allow multiple or recursive inclusion of this
 *	file.  Added bool enum from machine/types.h for regular users
 *	that want a real boolean type.
 *
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Also change spacing of MAX and MIN to coincide with that of
 *	sys/param.h.
 *
 * 19-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed the number of tabs between TRUE, FALSE and their
 *	respective values to match those in sys/types.h.
 *
 * 17-Dec-84  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Only define TRUE and FALSE if not defined.  Added caseE macro
 *	for using enumerated types in switch statements.
 *
 * 23-Apr-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added "sizeofS" and "sizeofA" macros which expand to the size
 *	of a string constant and array respectively.
 *
 **********************************************************************
 */

#ifndef	_C_INCLUDE_
#define	_C_INCLUDE_

#include <sys/cdefs.h>
#include <sys/param.h>

#ifndef	FALSE
#define FALSE	0
#endif	/* FALSE */

#ifndef	TRUE
#define TRUE	1
#endif	/* TRUE */

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifdef __arraycount
#define sizeofA(array)	__arraycount(array)
#else
#define sizeofA(array) (sizeof(array)/sizeof(array[0]))
#endif

#ifndef __unused
#ifndef __GNUC__
#define __unused
#else
#define __unused __attribute__((__unused__))
#endif
#endif

#endif	/* _C_INCLUDE_ */
