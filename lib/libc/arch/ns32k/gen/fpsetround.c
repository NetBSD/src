/*	$NetBSD: fpsetround.c,v 1.2 1997/05/08 13:38:36 matthias Exp $	*/

/*
 * Written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: fpsetround.c,v 1.2 1997/05/08 13:38:36 matthias Exp $";
#endif /* LIBC_SCCS and not lint */

#include <ieeefp.h>
#include <machine/cpufunc.h>

fp_rnd
fpsetround(rnd_dir)
	fp_rnd rnd_dir;
{
	fp_rnd old;
	fp_rnd new;

	sfsr(old);

	new = old;
	new &= ~(0x03 << 7); 
	new |= ((rnd_dir & 0x03) << 7);

	lfsr(new);

	return (old >> 7) & 0x03;
}
