/*	$NetBSD: htons.c,v 1.2.2.1 2012/04/17 00:01:39 yamt Exp $	*/

/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: htons.c,v 1.2.2.1 2012/04/17 00:01:39 yamt Exp $");
#endif

#include <sys/types.h>

#undef htons

uint16_t
htons(uint16_t x)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (void *) &x;
	return (uint16_t)(s[0] << 8 | s[1]);
#else
	return x;
#endif
}
