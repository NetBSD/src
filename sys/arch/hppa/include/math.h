/*	$NetBSD: math.h,v 1.3 2003/10/06 05:27:19 matt Exp $	*/

#include <sys/featuretest.h>

#define	__HAVE_NANF
/*
 * ISO C99
 */
#if !defined(_ANSI_SOURCE) && \
    (defined(_ISOC99_SOURCE) || (__STDC_VERSION__ - 0) >= 199901L || \
     defined(_NETBSD_SOURCE))
extern const union __float_u __nanf;
#endif
