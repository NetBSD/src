/* $NetBSD: nexf2.c,v 1.3 2025/09/29 02:47:19 nat Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: nexf2.c,v 1.3 2025/09/29 02:47:19 nat Exp $");
#endif /* LIBC_SCCS and not lint */

#ifdef FLOATX80

flag __nexf2(floatx80, floatx80);

flag
__nexf2(floatx80 a, floatx80 b)
{

	/* libgcc1.c says a != b */
#ifdef X80M68K
	return floatx80_eq(a, b) ? 1 : 0;
#else
	return !floatx80_eq(a, b);
#endif
}
#endif /* FLOATX80 */
