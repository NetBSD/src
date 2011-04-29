/*	$NetBSD: math.h,v 1.4.140.1 2011/04/29 07:48:37 matt Exp $	*/

#define	__HAVE_NANF
#if defined(__mips_n32) || defined(__mips_n64)
#define __HAVE_LONG_DOUBLE
#endif
