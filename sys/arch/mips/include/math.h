/*	$NetBSD: math.h,v 1.1 1999/12/23 10:15:12 kleink Exp $	*/

/*
 * ISO C99
 */
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || defined(_ISOC99_SOURCE)
extern const char	__nanf[];
#define	NAN		(*(const float *)(const void *)__nanf)
#endif
