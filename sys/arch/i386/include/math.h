/*	$NetBSD: math.h,v 1.2 2000/01/04 14:20:11 kleink Exp $	*/

/*
 * ISO C99
 */
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || defined(_ISOC99_SOURCE)
extern __const char	__nanf[];
#define	NAN		(*(__const float *)(__const void *)__nanf)
#endif
