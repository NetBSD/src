/*	$NetBSD: ntohs.c,v 1.8.14.1 2001/10/08 20:20:22 nathanw Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: ntohs.c,v 1.8.14.1 2001/10/08 20:20:22 nathanw Exp $");
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
