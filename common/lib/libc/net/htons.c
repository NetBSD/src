/*	$NetBSD: htons.c,v 1.2 2011/07/04 21:29:16 joerg Exp $	*/

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: htons.c,v 1.2 2011/07/04 21:29:16 joerg Exp $");
#endif

#include <sys/types.h>

#undef htons

uint16_t
htons(uint16_t x)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (u_char *) &x;
	return (uint16_t)(s[0] << 8 | s[1]);
#else
	return x;
#endif
}
