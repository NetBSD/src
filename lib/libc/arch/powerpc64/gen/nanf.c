/*	$NetBSD: nanf.c,v 1.1 2006/07/01 16:37:20 ross Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: nanf.c,v 1.1 2006/07/01 16:37:20 ross Exp $");
#endif /* LIBC_SCCS and not lint */

#include <math.h>
#include <machine/endian.h>

/* bytes for quiet NaN (IEEE single precision) */
const union __float_u __nanf =
		{ { 0x7f, 0xc0,    0,    0 } };
