/* $NetBSD: __aeabi_fcmpeq.c,v 1.1.2.2 2013/06/23 06:21:04 tls Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __aeabi_fcmpeq.c,v 1.1.2.2 2013/06/23 06:21:04 tls Exp $");
#endif /* LIBC_SCCS and not lint */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

int __aeabi_fcmpeq(float32, float32);

int
__aeabi_fcmpeq(float32 a, float32 b)
{

	return float32_eq(a, b);
}
