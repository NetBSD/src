/*	$NetBSD: fpsetmask.c,v 1.3 1997/07/13 18:45:35 christos Exp $	*/

/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpsetmask.c,v 1.3 1997/07/13 18:45:35 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <ieeefp.h>
#include <machine/cpufunc.h>
#include <machine/fpu.h>

fp_except
fpsetmask(mask)
	fp_except mask;
{
	fp_except old;
	fp_except new;
	fp_except ebits = FPC_IEN | FPC_OVE | FPC_IVE | FPC_DZE | FPC_UNDE;

	sfsr(old);

	new = old;
	new &= ~ebits;
	new |= mask & ebits;

	lfsr(new);

	return old & ebits;
}
