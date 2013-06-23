/* $NetBSD: __aeabi_dcmpeq.c,v 1.1.2.2 2013/06/23 06:21:04 tls Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __aeabi_dcmpeq.c,v 1.1.2.2 2013/06/23 06:21:04 tls Exp $");
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
