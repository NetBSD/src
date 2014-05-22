/* $NetBSD: __aeabi_dcmpge.c,v 1.2.8.2 2014/05/22 11:36:46 yamt Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __aeabi_dcmpge.c,v 1.2.8.2 2014/05/22 11:36:46 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

int __aeabi_dcmpge(float64, float64);

int
__aeabi_dcmpge(float64 a, float64 b)
{

	return !float64_lt(a, b) && float64_eq(a, a) && float64_eq(b, b);
}
