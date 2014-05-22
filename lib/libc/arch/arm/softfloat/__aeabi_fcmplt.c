/* $NetBSD: __aeabi_fcmplt.c,v 1.1.8.2 2014/05/22 11:36:46 yamt Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __aeabi_fcmplt.c,v 1.1.8.2 2014/05/22 11:36:46 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

int __aeabi_fcmplt(float32, float32);

int
__aeabi_fcmplt(float32 a, float32 b)
{

	return float32_lt(a, b);
}
