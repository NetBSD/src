/*	$NetBSD: ntohs.c,v 1.9.20.1 2004/08/03 10:53:31 skrll Exp $	*/

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: ntohs.c,v 1.9.20.1 2004/08/03 10:53:31 skrll Exp $");
#endif

#include <sys/types.h>

#undef ntohs

uint16_t
ntohs(x)
	uint16_t x;
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (u_char *) &x;
	return (uint16_t)(s[0] << 8 | s[1]);
#else
	return x;
#endif
}
