/*	$NetBSD: nanf.c,v 1.1 1999/12/23 10:15:07 kleink Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: nanf.c,v 1.1 1999/12/23 10:15:07 kleink Exp $");
#endif /* LIBC_SCCS and not lint */

/* nanf.c */

#include <math.h>
#include <machine/endian.h>

/* bytes for quiet NaN on a MIPS (IEEE single precision) */
#if BYTE_ORDER == BIG_ENDIAN
const char __nanf[] = { 0x7f, (char)0xc0, 0, 0 };
#else
const char __nanf[] = { 0, 0, (char)0xc0, 0x7f };
#endif
