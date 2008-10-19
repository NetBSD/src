/* $NetBSD: unordsf2.c,v 1.1.1.1.6.2 2008/10/19 22:40:31 haad Exp $ */

/*
 * Written by Richard Earnshaw, 2003.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

flag __unordsf2(float32, float32);

flag
__unordsf2(float32 a, float32 b)
{
	/*
	 * The comparison is unordered if either input is a NaN.
	 * Test for this by comparing each operand with itself.
	 * We must perform both comparisons to correctly check for
	 * signalling NaNs.
	 */
	return 1 ^ (float32_eq(a, a) & float32_eq(b, b));
}
