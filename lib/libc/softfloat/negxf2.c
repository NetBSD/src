/* $NetBSD: negxf2.c,v 1.1 2004/09/26 21:13:27 jmmv Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: negxf2.c,v 1.1 2004/09/26 21:13:27 jmmv Exp $");
#endif /* LIBC_SCCS and not lint */

floatx80 __negxf2(floatx80);

floatx80
__negxf2(floatx80 a)
{

	/* libgcc1.c says -a */
	return __mulxf3(a,__floatsixf(-1));
}
