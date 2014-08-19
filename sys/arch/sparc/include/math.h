/*	$NetBSD: math.h,v 1.5.24.1 2014/08/20 00:03:24 tls Exp $	*/

#define	__HAVE_NANF

#if defined(_LP64) || defined(_KERNEL)
#define	__HAVE_LONG_DOUBLE	128
#endif
