/*	$NetBSD: math.h,v 1.2.6.2 2000/11/20 20:19:20 bouyer Exp $	*/

/*
 * ISO C99
 */
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || defined(_ISOC99_SOURCE)
extern __const char	__nanf[];
#define	NAN		(*(__const float *)(__const void *)__nanf)
#endif
