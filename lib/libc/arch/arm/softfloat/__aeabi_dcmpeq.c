/* $NetBSD: __aeabi_dcmpeq.c,v 1.1.8.2 2014/05/22 11:36:46 yamt Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __aeabi_dcmpeq.c,v 1.1.8.2 2014/05/22 11:36:46 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

int __aeabi_dcmpeq(float64, float64);

int
__aeabi_dcmpeq(float64 a, float64 b)
{

	return float64_eq(a, b);
}
