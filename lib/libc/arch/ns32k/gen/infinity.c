/*	$NetBSD: infinity.c,v 1.5 1997/07/13 18:45:39 christos Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: infinity.c,v 1.5 1997/07/13 18:45:39 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <math.h>

/* bytes for +Infinity on a ns32k */
char __infinity[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
