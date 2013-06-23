/* $NetBSD: __aeabi_dcmpun.c,v 1.1.2.2 2013/06/23 06:21:04 tls Exp $ */

/*
 * Written by Richard Earnshaw, 2003.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __aeabi_dcmpun.c,v 1.1.2.2 2013/06/23 06:21:04 tls Exp $");
#endif /* LIBC_SCCS and not lint */

int __aeabi_dcmpun(float64, float64);

int
__aeabi_dcmpun(float64 a, float64 b)
{
	/*
	 * The comparison is unordered if either input is a NaN.
	 * Test for this by comparing each operand with itself.
	 * We must perform both comparisons to correctly check for
	 * signalling NaNs.
	 */
	return !float64_eq(a, a) || !float64_eq(b, b);
}
