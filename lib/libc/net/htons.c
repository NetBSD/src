/*	$NetBSD: htons.c,v 1.7 1996/10/13 04:08:36 christos Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$NetBSD: htons.c,v 1.7 1996/10/13 04:08:36 christos Exp $";
#endif

#include <sys/types.h>

#undef htons

in_port_t
htons(x)
	in_port_t x;
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (u_char *) &x;
	return (in_port_t)(s[0] << 8 | s[1]);
#else
	return x;
#endif
}
