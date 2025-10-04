/*	$NetBSD: flt_rounds.c,v 1.1 2025/10/04 21:03:16 nat Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: flt_rounds.c,v 1.1 2025/10/04 21:03:16 nat Exp $");
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
