/*	$NetBSD: htons.c,v 1.9.26.1 2001/09/21 22:36:30 nathanw Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: htons.c,v 1.9.26.1 2001/09/21 22:36:30 nathanw Exp $");
#endif

#include <sys/types.h>

#undef htons

uint16_t
htons(x)
	uint16_t x;
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (u_char *) &x;
	return (uint16_t)(s[0] << 8 | s[1]);
#else
	return x;
#endif
}
