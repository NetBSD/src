/* $NetBSD: nexf2.c,v 1.1 2004/09/26 21:13:27 jmmv Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: nexf2.c,v 1.1 2004/09/26 21:13:27 jmmv Exp $");
#endif /* LIBC_SCCS and not lint */

flag __nexf2(floatx80, floatx80);

flag
__nexf2(floatx80 a, floatx80 b)
{

	/* libgcc1.c says a != b */
	return !floatx80_eq(a, b);
}
