/*	$NetBSD: flt_rounds.c,v 1.3 1997/07/13 18:45:30 christos Exp $	*/

/*
 * Written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: flt_rounds.c,v 1.3 1997/07/13 18:45:30 christos Exp $");
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
