/*	$NetBSD: fpgetround.c,v 1.4 2002/01/13 21:45:46 thorpej Exp $	*/

/*
 * Written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpgetround.c,v 1.4 2002/01/13 21:45:46 thorpej Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <ieeefp.h>
#include <machine/cpufunc.h>

#ifdef __weak_alias
__weak_alias(fpgetround,_fpgetround)
#endif

fp_rnd
fpgetround()
{
	int x;

	sfsr(x);
	return (x >> 7) & 0x03;
}
