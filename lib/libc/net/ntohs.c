/*	$NetBSD: ntohs.c,v 1.8 1997/07/13 19:57:56 christos Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: ntohs.c,v 1.8 1997/07/13 19:57:56 christos Exp $");
#endif

#include <sys/types.h>

#undef ntohs

in_port_t
ntohs(x)
	in_port_t x;
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (u_char *) &x;
	return (in_port_t)(s[0] << 8 | s[1]);
#else
	return x;
#endif
}
