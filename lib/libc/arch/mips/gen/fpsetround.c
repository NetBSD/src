/*	$NetBSD: fpsetround.c,v 1.5.44.1 2012/04/17 00:05:14 yamt Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpsetround.c,v 1.5.44.1 2012/04/17 00:05:14 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <ieeefp.h>

#ifdef __weak_alias
__weak_alias(fpsetround,_fpsetround)
#endif

fp_rnd
fpsetround(fp_rnd rnd_dir)
{
	fp_rnd old;
	fp_rnd new;

	__asm("cfc1 %0,$31" : "=r" (old));

	new = old & ~0x03;
	new |= rnd_dir & 0x03;

	__asm("ctc1 %0,$31" : : "r" (new));

	return old & 0x03;
}
