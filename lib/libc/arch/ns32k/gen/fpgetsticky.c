/*	$NetBSD: fpgetsticky.c,v 1.3.14.1 2002/01/28 20:49:59 nathanw Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpgetsticky.c,v 1.3.14.1 2002/01/28 20:49:59 nathanw Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <ieeefp.h>
#include <machine/cpufunc.h>
#include <machine/fpu.h>

#ifdef __weak_alias
__weak_alias(fpgetsticky,_fpgetsticky)
#endif

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
