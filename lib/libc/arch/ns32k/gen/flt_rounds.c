/*	$NetBSD: flt_rounds.c,v 1.2 1997/05/08 13:38:33 matthias Exp $	*/

/*
 * Written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: flt_rounds.c,v 1.2 1997/05/08 13:38:33 matthias Exp $";
#endif /* LIBC_SCCS and not lint */

#include <machine/cpufunc.h>

static const int map[] = {
	1,	/* round to nearest */
	0,	/* round to zero */
	2,	/* round to positive infinity */
	3	/* round to negative infinity */
};

int
__flt_rounds()
{
	int x;

	sfsr(x);
	return map[(x >> 7) & 0x03];
}
