/*	$NetBSD: fpgetmask.c,v 1.5.44.1 2012/04/17 00:05:14 yamt Exp $	*/

/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpgetmask.c,v 1.5.44.1 2012/04/17 00:05:14 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <ieeefp.h>

#ifdef __weak_alias
__weak_alias(fpgetmask,_fpgetmask)
#endif

fp_except
fpgetmask(void)
{
	unsigned int x;

	__asm("st %%fsr,%0" : "=m" (*&x));
	return (x >> 23) & 0x1f;
}
