/*	$NetBSD: fpgetsticky.c,v 1.3 1997/07/13 18:45:34 christos Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpgetsticky.c,v 1.3 1997/07/13 18:45:34 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <ieeefp.h>
#include <machine/cpufunc.h>
#include <machine/fpu.h>

fp_except
fpgetsticky()
{
	fp_except x;
	fp_except ebits = FPC_IEN | FPC_OVE | FPC_IVE | FPC_DZE | FPC_UNDE;

	sfsr(x);
	/* Map FPC_UF to soft underflow enable */
	if (x & FPC_UF)
		x |= FPC_UNDE << 1;
	else
		x &= ~(FPC_UNDE << 1);
	x >>= 1;

	return x & ebits;
}
