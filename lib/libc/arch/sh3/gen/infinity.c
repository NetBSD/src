/*	$NetBSD: infinity.c,v 1.3 2000/09/13 22:32:27 msaitoh Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: infinity.c,v 1.3 2000/09/13 22:32:27 msaitoh Exp $");
#endif /* LIBC_SCCS and not lint */

/* infinity.c */

#include <math.h>
#include <machine/endian.h>

/* bytes for +Infinity on a SH3 */
#if BYTE_ORDER == LITTLE_ENDIAN
const char __infinity[] = { 0, 0, 0, 0, 0, 0, (char)0xf0, 0x7f };
#else
const char __infinity[] = { 0x7f, (char)0xf0, 0, 0, 0, 0, 0, 0 };
#endif
