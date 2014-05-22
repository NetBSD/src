/* $NetBSD: __aeabi_fcmpun.c,v 1.1.8.2 2014/05/22 11:36:46 yamt Exp $ */

/*
 * Written by Richard Earnshaw, 2003.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __aeabi_fcmpun.c,v 1.1.8.2 2014/05/22 11:36:46 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

int __aeabi_fcmpun(float32, float32);

int
__aeabi_fcmpun(float32 a, float32 b)
{
	/*
	 * The comparison is unordered if either input is a NaN.
	 * Test for this by comparing each operand with itself.
	 * We must perform both comparisons to correctly check for
	 * signalling NaNs.
	 */
	return !float32_eq(a, a) || !float32_eq(b, b);
}
