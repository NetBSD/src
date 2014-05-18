/*	$NetBSD: math.h,v 1.5.28.1 2014/05/18 17:45:25 rmind Exp $	*/

#define	__HAVE_NANF

#if defined(_LP64) || defined(_KERNEL)
#define	__HAVE_LONG_DOUBLE	128
#endif
