/*	$NetBSD: featuretest.h,v 1.2.26.1 2002/01/10 20:04:42 thorpej Exp $	*/

/*
 * Written by Klaus Klein <kleink@NetBSD.ORG>, February 2, 1998.
 * Public domain.
 *
 * NOTE: Do not protect this header against multiple inclusion.  Doing
 * so can have subtle side-effects due to header file inclusion order
 * and testing of e.g. _POSIX_SOURCE vs. _POSIX_C_SOURCE.  Instead,
 * protect each CPP macro that we want to supply.
 */

#if defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE	1L
#endif

#if ((_POSIX_C_SOURCE - 0) >= 199506L || (_XOPEN_SOURCE - 0) >= 500) && \
    !defined(_REENTRANT)
#define _REENTRANT
#endif
