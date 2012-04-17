/*	$NetBSD: math.h,v 1.5.112.1 2012/04/17 00:07:01 yamt Exp $	*/

#ifndef _VAX_MATH_H_
#define _VAX_MATH_H_

#include <sys/cdefs.h>

#if __GNUC_PREREQ__(3, 3)
#define	__INFINITY	__builtin_huge_valf()
#else
#define	__INFINITY	1.0E+39F
#endif

#define	__isinf(__x)	(0)
#define	__isnan(__x)	(0)

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || \
    ((__STDC_VERSION__ - 0) >= 199901L) || \
    ((_POSIX_C_SOURCE - 0) >= 200112L) || \
    ((_XOPEN_SOURCE  - 0) >= 600) || \
    defined(_ISOC99_SOURCE) || defined(_NETBSD_SOURCE)
/* 7.12#6 number classification macros; machine-specific classes */
#define	FP_DIRTYZERO	0x80
#define	FP_ROP		0x81
#endif

#endif /* _VAX_MATH_H_ */
