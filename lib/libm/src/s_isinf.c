/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: s_isinf.c,v 1.4 1997/10/09 11:32:19 lukem Exp $");
#endif

/*
 * isinf(x) returns 1 is x is inf, else 0;
 * no branching!
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	int isinf(double x)
#else
	int isinf(x)
	double x;
#endif
{
	int32_t hx,lx;
	EXTRACT_WORDS(hx,lx,x);
	hx &= 0x7fffffff;
	hx ^= 0x7ff00000;
	hx |= lx;
	return (hx == 0);
}
