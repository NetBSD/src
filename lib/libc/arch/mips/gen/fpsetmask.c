/*	$NetBSD: fpsetmask.c,v 1.9.4.1 2017/04/21 16:53:07 bouyer Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpsetmask.c,v 1.9.4.1 2017/04/21 16:53:07 bouyer Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <ieeefp.h>

#ifdef __weak_alias
__weak_alias(fpsetmask,_fpsetmask)
#endif

fp_except
fpsetmask(fp_except mask)
{
	fp_except old;
	fp_except new;

	__asm(".set push; .set noat; cfc1 %0,$31; .set pop" : "=r" (old));

	new = old & ~(0x1f << 7); 
	new |= ((mask & 0x1f) << 7);

	__asm(".set push; .set noat; ctc1 %0,$31; .set pop" : : "r" (new));

	return (old >> 7) & 0x1f;
}
