/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: s_isinff.c,v 1.4 1997/10/09 11:32:22 lukem Exp $");
#endif

/*
 * isinff(x) returns 1 is x is inf, else 0;
 * no branching!
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	int isinff(float x)
#else
	int isinff(x)
	float x;
#endif
{
	int32_t ix;
	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;
	ix ^= 0x7f800000;
	return (ix == 0);
}
