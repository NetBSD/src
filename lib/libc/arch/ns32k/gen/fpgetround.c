/*	$NetBSD: fpgetround.c,v 1.2 1997/05/08 13:38:34 matthias Exp $	*/

/*
 * Written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: fpgetround.c,v 1.2 1997/05/08 13:38:34 matthias Exp $";
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
