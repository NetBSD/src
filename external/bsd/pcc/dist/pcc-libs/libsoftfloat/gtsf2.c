/* $NetBSD: gtsf2.c,v 1.1.1.1 2008/08/24 05:34:48 gmcgarry Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

flag __gtsf2(float32, float32);

flag
__gtsf2(float32 a, float32 b)
{

	/* libgcc1.c says a > b */
	return float32_lt(b, a);
}
