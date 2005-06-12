/*	$NetBSD: fpgetmask.c,v 1.4 2005/06/12 05:21:26 lukem Exp $	*/

/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpgetmask.c,v 1.4 2005/06/12 05:21:26 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <ieeefp.h>

#ifdef __weak_alias
__weak_alias(fpgetmask,_fpgetmask)
#endif

fp_except
fpgetmask()
{
	int x;

	__asm__("st %%fsr,%0" : "=m" (*&x));
	return (x >> 23) & 0x1f;
}
