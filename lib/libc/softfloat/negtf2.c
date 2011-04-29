/* $NetBSD: negtf2.c,v 1.1.4.2 2011/04/29 07:48:36 matt Exp $ */

/*
 * Written by Matt Thomas, 2011.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: negtf2.c,v 1.1.4.2 2011/04/29 07:48:36 matt Exp $");
#endif /* LIBC_SCCS and not lint */

#ifdef FLOAT128

float128 __negtf2(float128);

float128
__negtf2(float128 a)
{

	/* libgcc1.c says -a */
	a.high ^= FLOAT64_MANGLE(0x8000000000000000ULL);
	return a;
}

#endif /* FLOAT128 */
