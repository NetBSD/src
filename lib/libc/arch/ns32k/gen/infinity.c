/*	$NetBSD: infinity.c,v 1.7 1998/11/14 19:31:02 christos Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: infinity.c,v 1.7 1998/11/14 19:31:02 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <math.h>

/* bytes for +Infinity on a ns32k */
const char __infinity[] = { 0, 0, 0, 0, 0, 0, (char)0xf0, 0x7f };
