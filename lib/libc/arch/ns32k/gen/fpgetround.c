/*	$NetBSD: fpgetround.c,v 1.3 1997/07/13 18:45:32 christos Exp $	*/

/*
 * Written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpgetround.c,v 1.3 1997/07/13 18:45:32 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <ieeefp.h>
#include <machine/cpufunc.h>

fp_rnd
fpgetround()
{
	int x;

	sfsr(x);
	return (x >> 7) & 0x03;
}
