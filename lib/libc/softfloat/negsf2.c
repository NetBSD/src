/* $NetBSD: negsf2.c,v 1.1.4.2 2000/06/23 16:18:02 minoura Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: negsf2.c,v 1.1.4.2 2000/06/23 16:18:02 minoura Exp $");
#endif /* LIBC_SCCS and not lint */

float32 __negsf2(float32);

float32
__negsf2(float32 a)
{

	/* libgcc1.c says INTIFY(-a) */
	return a ^ 0x80000000;
}
