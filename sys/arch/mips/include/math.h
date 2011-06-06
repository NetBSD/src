/*	$NetBSD: math.h,v 1.4.150.1 2011/06/06 09:06:03 jruoho Exp $	*/

#define	__HAVE_NANF
#if defined(__mips_n32) || defined(__mips_n64)
#define __HAVE_LONG_DOUBLE
#endif
