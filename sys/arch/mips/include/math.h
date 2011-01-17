/*	$NetBSD: math.h,v 1.5 2011/01/17 23:53:03 matt Exp $	*/

#define	__HAVE_NANF
#if defined(__mips_n32) || defined(__mips_n64)
#define __HAVE_LONG_DOUBLE
#endif
