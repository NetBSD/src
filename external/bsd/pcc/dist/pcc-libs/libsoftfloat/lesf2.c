/* $NetBSD: lesf2.c,v 1.1.1.1.2.2 2008/09/18 05:15:48 wrstuden Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

flag __lesf2(float32, float32);

flag
__lesf2(float32 a, float32 b)
{

	/* libgcc1.c says 1 - (a <= b) */
	return 1 - float32_le(a, b);
}
