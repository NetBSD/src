/*	$NetBSD: nanf.c,v 1.3.32.1 2009/05/13 19:18:21 jym Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: nanf.c,v 1.3.32.1 2009/05/13 19:18:21 jym Exp $");
#endif /* LIBC_SCCS and not lint */

#include <math.h>
#include <machine/endian.h>

/* bytes for quiet NaN (IEEE single precision) */
const union __float_u __nanf =
#if BYTE_ORDER == BIG_ENDIAN
		{ { 0x7f, 0xa0,    0,    0 } };
#else
		{ {    0,    0, 0xa0, 0x7f } };
#endif

__warn_references(__nanf, "warning: <math.h> defines NAN incorrectly for your compiler.")
