/* $NetBSD: math.h,v 1.2 2019/04/13 15:57:31 maya Exp $ */

#define __HAVE_NANF

#if defined(_LP64) || defined(_KERNEL)
#define	__HAVE_LONG_DOUBLE	128
#endif
