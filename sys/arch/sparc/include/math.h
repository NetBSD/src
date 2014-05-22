/*	$NetBSD: math.h,v 1.5.14.1 2014/05/22 11:40:08 yamt Exp $	*/

#define	__HAVE_NANF

#if defined(_LP64) || defined(_KERNEL)
#define	__HAVE_LONG_DOUBLE	128
#endif
