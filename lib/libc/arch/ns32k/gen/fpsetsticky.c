/*	$NetBSD: fpsetsticky.c,v 1.3 1997/05/08 13:38:37 matthias Exp $	*/

/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: fpsetsticky.c,v 1.3 1997/05/08 13:38:37 matthias Exp $";
#endif /* LIBC_SCCS and not lint */

#include <ieeefp.h>
#include <machine/cpufunc.h>
#include <machine/fpu.h>

fp_except
fpsetsticky(sticky)
	fp_except sticky;
{
	fp_except old;
	fp_except new;
	fp_except ebits = FPC_IEN | FPC_OVE | FPC_IVE | FPC_DZE | FPC_UEN | FPC_UNDE;

	if (sticky & FPC_UNDE) {
		sticky |= FPC_UEN;
		sticky &= ~FPC_UNDE;
	}

	sfsr(old);

	new = old;
	new &= ebits;
	new |= (sticky & ebits) << 1;

	lfsr(new);

	/* Map FPC_UF to soft underflow enable */
	if (old & FPC_UF) {
		old |= FPC_UNDE << 1;
		old &= FPC_UF;
	} else
		old &= ~(FPC_UNDE << 1);
	old >>= 1;

	return old & ebits;
}
