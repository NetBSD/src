/*	$NetBSD: infinity.c,v 1.4 1997/05/08 13:38:38 matthias Exp $	*/

#if defined(LIBC_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: infinity.c,v 1.4 1997/05/08 13:38:38 matthias Exp $";
#endif /* LIBC_SCCS and not lint */

#include <math.h>

/* bytes for +Infinity on a ns32k */
char __infinity[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
