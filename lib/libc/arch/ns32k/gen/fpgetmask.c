/*	$NetBSD: fpgetmask.c,v 1.4 2002/01/13 21:45:46 thorpej Exp $	*/

/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpgetmask.c,v 1.4 2002/01/13 21:45:46 thorpej Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <ieeefp.h>
#include <machine/cpufunc.h>
#include <machine/fpu.h>

#ifdef __weak_alias
__weak_alias(fpgetmask,_fpgetmask)
#endif

fp_except
fpgetmask()
{
	fp_except x;
	fp_except ebits = FPC_IEN | FPC_OVE | FPC_IVE | FPC_DZE | FPC_UNDE;

	sfsr(x);

	return x & ebits;
}
