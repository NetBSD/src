/*	$NetBSD: math.h,v 1.1.30.3 2004/09/21 13:23:43 skrll Exp $	*/

#define	__INFINITY	1.0E+39F

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
