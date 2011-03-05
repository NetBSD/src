/*	$NetBSD: math.h,v 1.4.144.1 2011/03/05 20:51:03 rmind Exp $	*/

#define	__HAVE_NANF
#if defined(__mips_n32) || defined(__mips_n64)
#define __HAVE_LONG_DOUBLE
#endif
