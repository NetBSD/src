/*	$NetBSD: fpsetsticky.c,v 1.4 2005/06/12 05:21:26 lukem Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpsetsticky.c,v 1.4 2005/06/12 05:21:26 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <ieeefp.h>

#ifdef __weak_alias
__weak_alias(fpsetsticky,_fpsetsticky)
#endif

fp_except
fpsetsticky(sticky)
	fp_except sticky;
{
	fp_except old;
	fp_except new;

	__asm__("cfc1 %0,$31" : "=r" (old));

	new = old;
	new &= ~(0x1f << 2); 
	new |= ((sticky & 0x1f) << 2);

	__asm__("ctc1 %0,$31" : : "r" (new));

	return (old >> 2) & 0x1f;
}
