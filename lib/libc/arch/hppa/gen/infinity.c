/*	$NetBSD: infinity.c,v 1.1 2002/06/06 20:31:21 fredette Exp $	*/

/*	$OpenBSD: infinity.c,v 1.2 2001/01/24 08:19:02 mickey Exp $	*/

/* infinity.c */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$OpenBSD: infinity.c,v 1.2 2001/01/24 08:19:02 mickey Exp $");
#endif /* LIBC_SCCS and not lint */

#include <math.h>

/* bytes for +Infinity on a hppa */
__const union __double_u __infinity __attribute__((__aligned__(sizeof(double)))) =
    { { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 } };
