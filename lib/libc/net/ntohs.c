/*	$NetBSD: ntohs.c,v 1.7 1996/10/13 04:08:38 christos Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$NetBSD: ntohs.c,v 1.7 1996/10/13 04:08:38 christos Exp $";
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
