/*	$NetBSD: math.h,v 1.1 2001/06/19 00:20:11 fvdl Exp $	*/

/*
 * ISO C99
 */
#if !defined(_ANSI_SOURCE) && \
    (!defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE) || \
     defined(_ISOC99_SOURCE) || (__STDC_VERSION__ - 0) >= 199901L)
extern __const char	__nanf[];
#define	NAN		(*(__const float *)(__const void *)__nanf)
#endif
