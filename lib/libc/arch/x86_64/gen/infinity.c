/*	$NetBSD: infinity.c,v 1.1 2001/06/19 00:25:03 fvdl Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: infinity.c,v 1.1 2001/06/19 00:25:03 fvdl Exp $");
#endif /* LIBC_SCCS and not lint */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a 387 */
const char __infinity[] = { 0, 0, 0, 0, 0, 0, (char)0xf0, 0x7f };
