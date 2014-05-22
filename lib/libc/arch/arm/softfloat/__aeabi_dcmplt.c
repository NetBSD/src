/* $NetBSD: __aeabi_dcmplt.c,v 1.1.8.2 2014/05/22 11:36:46 yamt Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __aeabi_dcmplt.c,v 1.1.8.2 2014/05/22 11:36:46 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

int __aeabi_dcmplt(float64, float64);

int
__aeabi_dcmplt(float64 a, float64 b)
{

	return float64_lt(a, b);
}
