/*	$NetBSD: infinity.c,v 1.6 1998/07/26 14:14:15 mycroft Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: infinity.c,v 1.6 1998/07/26 14:14:15 mycroft Exp $");
#endif /* LIBC_SCCS and not lint */

#include <math.h>

/* bytes for +Infinity on a ns32k */
const char __infinity[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
