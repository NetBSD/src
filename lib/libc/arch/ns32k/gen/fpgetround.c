/*	$NetBSD: fpgetround.c,v 1.3.14.1 2002/01/28 20:49:59 nathanw Exp $	*/

/*
 * Written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpgetround.c,v 1.3.14.1 2002/01/28 20:49:59 nathanw Exp $");
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
