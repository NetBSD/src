/*	$NetBSD: fpgetmask.c,v 1.2 1997/05/08 13:38:34 matthias Exp $	*/

/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: fpgetmask.c,v 1.2 1997/05/08 13:38:34 matthias Exp $";
#endif /* LIBC_SCCS and not lint */

#include <ieeefp.h>
#include <machine/cpufunc.h>
#include <machine/fpu.h>

fp_except
fpgetmask()
{
	fp_except x;
	fp_except ebits = FPC_IEN | FPC_OVE | FPC_IVE | FPC_DZE | FPC_UNDE;

	sfsr(x);

	return x & ebits;
}
