/*	$NetBSD: ntohs.c,v 1.3.54.1 2026/11/04 22:36:30 perseant Exp $	*/

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: ntohs.c,v 1.3.54.1 2026/11/04 22:36:30 perseant Exp $");
#endif

#include <sys/types.h>
#include <sys/endian.h>

#undef ntohs

uint16_t
ntohs(uint16_t x)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (void *) &x;
	return (uint16_t)(s[0] << 8 | s[1]);
#else
	return x;
#endif
}
