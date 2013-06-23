/* $NetBSD: __aeabi_dcmplt.c,v 1.1.2.2 2013/06/23 06:21:04 tls Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __aeabi_dcmplt.c,v 1.1.2.2 2013/06/23 06:21:04 tls Exp $");
#endif /* LIBC_SCCS and not lint */

int __aeabi_dcmplt(float64, float64);

int
__aeabi_dcmplt(float64 a, float64 b)
{

	return float64_lt(a, b);
}
