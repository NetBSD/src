/*	$NetBSD: math.h,v 1.2 2003/04/28 23:16:19 bjh21 Exp $	*/

#include <sys/featuretest.h>

/*
 * ISO C99
 */
#if !defined(_ANSI_SOURCE) && \
    (defined(_ISOC99_SOURCE) || (__STDC_VERSION__ - 0) >= 199901L || \
     defined(_NETBSD_SOURCE))
extern __const char	__nanf[];
#define	NAN		(*(__const float *)(__const void *)__nanf)
#endif
