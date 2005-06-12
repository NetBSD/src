/*	$NetBSD: flt_rounds.c,v 1.4 2005/06/12 05:21:26 lukem Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: flt_rounds.c,v 1.4 2005/06/12 05:21:26 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#include <machine/float.h>

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

	__asm__("cfc1 %0,$31" : "=r" (x));
	return map[x & 0x03];
}
