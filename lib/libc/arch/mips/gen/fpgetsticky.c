/*	$NetBSD: fpgetsticky.c,v 1.7 2012/03/20 10:51:24 he Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpgetsticky.c,v 1.7 2012/03/20 10:51:24 he Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <ieeefp.h>

#ifdef __weak_alias
__weak_alias(fpgetsticky,_fpgetsticky)
#endif

fp_except
fpgetsticky(void)
{
	fp_except x;

	__asm("cfc1 %0,$31" : "=r" (x));
	return ((unsigned int)x >> 2) & 0x1f;
}
