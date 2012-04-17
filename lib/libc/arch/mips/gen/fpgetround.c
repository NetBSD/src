/*	$NetBSD: fpgetround.c,v 1.5.44.1 2012/04/17 00:05:14 yamt Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpgetround.c,v 1.5.44.1 2012/04/17 00:05:14 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <ieeefp.h>

#ifdef __weak_alias
__weak_alias(fpgetround,_fpgetround)
#endif

fp_rnd
fpgetround(void)
{
	fp_rnd x;

	__asm("cfc1 %0,$31" : "=r" (x));
	return x & 0x03;
}
