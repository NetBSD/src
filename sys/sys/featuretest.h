/*	$NetBSD: featuretest.h,v 1.2 1998/10/24 16:30:56 kleink Exp $	*/

/*
 * Written by Klaus Klein <kleink@NetBSD.ORG>, February 2, 1998.
 * Public domain.
 */

#ifndef _SYS_FEATURETEST_H_
#define _SYS_FEATURETEST_H_

#if defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE	1L
#endif

#if ((_POSIX_C_SOURCE - 0) >= 199506L || (_XOPEN_SOURCE - 0) >= 500) && \
    !defined(_REENTRANT)
#define _REENTRANT
#endif

#endif /* !defined(_SYS_FEATURETEST_H_) */
