/*	$NetBSD: nanf_ieee754.c,v 1.1 2003/05/12 15:15:16 kleink Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: nanf_ieee754.c,v 1.1 2003/05/12 15:15:16 kleink Exp $");
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
