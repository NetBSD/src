/*	$NetBSD: fpsetround.c,v 1.3.14.1 2002/01/28 20:50:00 nathanw Exp $	*/

/*
 * Written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpsetround.c,v 1.3.14.1 2002/01/28 20:50:00 nathanw Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <ieeefp.h>
#include <machine/cpufunc.h>

#ifdef __weak_alias
__weak_alias(fpsetround,_fpsetround)
#endif

fp_rnd
fpsetround(rnd_dir)
	fp_rnd rnd_dir;
{
	fp_rnd old;
	fp_rnd new;

	sfsr(old);

	new = old;
	new &= ~(0x03 << 7); 
	new |= ((rnd_dir & 0x03) << 7);

	lfsr(new);

	return (old >> 7) & 0x03;
}
