/*	$NetBSD: featuretest.h,v 1.1 1998/02/02 15:53:56 kleink Exp $	*/

/*
 * Written by Klaus Klein <kleink@NetBSD.ORG>, February 2, 1998.
 * Public domain.
 */

#ifndef _SYS_FEATURETEST_H_
#define _SYS_FEATURETEST_H_

#if defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE	1L
#endif

#endif /* !defined(_SYS_FEATURETEST_H_) */
