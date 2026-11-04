/*	$NetBSD: flt_rounds.c,v 1.1.2.2 2026/11/04 23:19:22 perseant Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: flt_rounds.c,v 1.1.2.2 2026/11/04 23:19:22 perseant Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <machine/float.h>
#include <ieeefp.h>

static const int map[] = {
	1,	/* round to nearest */
	0,	/* round to zero */
#ifdef __m68k__
	3,	/* round to positive infinity */
	2	/* round to negative infinity */
#else
	2,	/* round to positive infinity */
	3	/* round to negative infinity */
#endif
};

int
__flt_rounds(void)
{
	return map[fpgetround()];
}
