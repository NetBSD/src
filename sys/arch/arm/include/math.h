/*	$NetBSD: math.h,v 1.1 2001/01/10 19:02:07 bjh21 Exp $	*/

/*
 * ISO C99
 */
#if !defined(_ANSI_SOURCE) && \
    (!defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE) || \
     defined(_ISOC99_SOURCE) || (__STDC_VERSION__ - 0) >= 199901L)
extern __const char	__nanf[];
#define	NAN		(*(__const float *)(__const void *)__nanf)
#endif
