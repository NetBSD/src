/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: s_isinff.c,v 1.5 2002/05/26 22:01:56 wiz Exp $");
#endif

/*
 * isinff(x) returns 1 is x is inf, else 0;
 * no branching!
 */

#include "math.h"
#include "math_private.h"

int
isinff(float x)
{
	int32_t ix;
	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;
	ix ^= 0x7f800000;
	return (ix == 0);
}
