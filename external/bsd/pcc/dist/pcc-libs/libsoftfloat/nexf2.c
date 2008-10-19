/* $NetBSD: nexf2.c,v 1.1.1.1.6.2 2008/10/19 22:40:31 haad Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#ifdef FLOATX80

flag __nexf2(floatx80, floatx80);

flag
__nexf2(floatx80 a, floatx80 b)
{

	/* libgcc1.c says a != b */
	return !floatx80_eq(a, b);
}
#endif /* FLOATX80 */
