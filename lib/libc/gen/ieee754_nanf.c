/*	$NetBSD: ieee754_nanf.c,v 1.1.2.2 2002/03/08 21:35:10 nathanw Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: ieee754_nanf.c,v 1.1.2.2 2002/03/08 21:35:10 nathanw Exp $");
#endif /* LIBC_SCCS and not lint */

#include <math.h>
#include <machine/endian.h>

/* bytes for quiet NaN (IEEE single precision) */
const union __float_u __nanf =
#if BYTE_ORDER == BIG_ENDIAN
		{ { 0x7f, 0xc0,    0,    0 } };
#else
		{ {    0,    0, 0xc0, 0x7f } };
#endif
const union __float_u __nanf2 = {{ 0, 0, 0, 0}};
